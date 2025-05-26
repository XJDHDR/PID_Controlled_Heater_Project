// This code is provided under the MPL v2.0 license. Copyright 2025 Xavier du Hecquet de Rauville
// Details may be found in License.txt
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
//  This Source Code Form is "Incompatible With Secondary Licenses", as
//  defined by the Mozilla Public License, v. 2.0.


#include "FanControl.h"

#include <Arduino.h>

#include "Misc/SerialHandler.h"
#include "Misc/Utils.h"


#define FAN_POWER_MOSFET_PIN        4
#define FAN_PWM_SPEED_CONTROL_PIN   5
#define FAN_SPEED_SENSE_PIN         6

#define PWM_RESOLUTION_BITS         10
#define PWM_RESOLUTION              (2^PWM_RESOLUTION_BITS)

#define FAN_MIN_STARTUP_SPEED_PERCENT   30.0f
#define FAN_MIN_RUNNING_SPEED_PERCENT   23.0f
#define FAN_MIN_STARTUP_RPM             100
#define PERIOD_BETWEEN_RPM_CHECKS_MS    1000
#define SLOWDOWN_WAIT_TIMER_MS          (90 * 1000)


bool FanControl::debug_GetFanRpm = false;
bool FanControl::debug_SetFanDutyCycle = false;
bool FanControl::debug_UpdateSlowdownState = false;

bool FanControl::isSlowdownQueued = false;

uint32_t FanControl::lastRpmMeasurement = 0;
uint32_t FanControl::millisValueAtLastRpmCheck = 0;
uint32_t FanControl::millisValueAtSlowDownDelayTimerStart = 0;
uint32_t FanControl::speedSensePinPulseCountSinceLastCheck = 0;

float FanControl::currentlySetFanDutyCycle = 0.0;
float FanControl::queuedSlowdownFanDutyCycle = 0.0;

FanControl::FanStates FanControl::currentState = SwitchedOff;


/**
 * @brief  Initialises the Fan Controller class.
*/
void FanControl::Init()
{
	enableDebugTriggers();

	pinMode(FAN_POWER_MOSFET_PIN, OUTPUT);
	pinMode(FAN_PWM_SPEED_CONTROL_PIN, OUTPUT);
	pinMode(FAN_SPEED_SENSE_PIN, INPUT);

	analogWriteFrequency(25000);
	analogWriteResolution(PWM_RESOLUTION_BITS);

	attachInterrupt(digitalPinToInterrupt(FAN_SPEED_SENSE_PIN), fanSpeedPulseInterruptHandler, RISING);
}

/**
 * @brief   Gets data regarding the fan's state.
 *
 * @return  Struct containing the fan data.
*/
FanControl::FanRpmData FanControl::GetFanRpm()
{
	FanRpmData fanRpmData{};

	if ((millis() - millisValueAtLastRpmCheck) < PERIOD_BETWEEN_RPM_CHECKS_MS)
	{
		return fanRpmData;
	}
	millisValueAtLastRpmCheck = millis();
	fanRpmData.WasMeasurementTaken = true;

	if (debug_GetFanRpm)
	{
		std::string fanRevolutionsCountMsg = Utils::StringFormat("Fan pulses counted: %i, time: %ims", speedSensePinPulseCountSinceLastCheck, PERIOD_BETWEEN_RPM_CHECKS_MS);
		SerialHandler::SafeWriteLn(fanRevolutionsCountMsg, true);
	}

	noInterrupts();
	const uint32_t fanRevolutionsCompleted = speedSensePinPulseCountSinceLastCheck / 2;     // Fan generates 2 pulses per revolution
	speedSensePinPulseCountSinceLastCheck = 0;
	interrupts();

	lastRpmMeasurement = fanRevolutionsCompleted * (60 * 1000 / PERIOD_BETWEEN_RPM_CHECKS_MS);

	if (currentState == SwitchedOff)
	{
		SerialHandler::SafeWriteLn("Fan is switched off", debug_GetFanRpm);
		return fanRpmData;
	}
	fanRpmData.IsFanSwitchedOn = true;

	if (fanRevolutionsCompleted == 0)
	{
		SerialHandler::SafeWriteLn("Fan is not spinning", debug_GetFanRpm);
		return fanRpmData;
	}

	fanRpmData.IsFanSpinning = true;
	fanRpmData.Rpm = lastRpmMeasurement;

	if (debug_GetFanRpm)
	{
		std::string fanRpmMsg = Utils::StringFormat("Fan RPM: %u", fanRpmData.Rpm);
		SerialHandler::SafeWriteLn(fanRpmMsg, true);
	}

	return fanRpmData;
}

/**
 * @brief   Gets the duty cycle currently set for the fan.
 *
 * @return  The duty cycle as a percentage.
*/
float FanControl::GetFanCurrentDutyCycle()
{
	return currentlySetFanDutyCycle;
}

/**
 * @brief             Checks whether the fan's duty cycle needs to be changed, and how the change should be handled.
 *
 * @param  InputData  The new duty cycle as a percentage between 0.0 and 100.0
*/
void FanControl::SetFanDutyCycle(const float NewDutyCyclePercent)
{
	if ((currentState == SwitchedOff) && (NewDutyCyclePercent < 0.1))
	{
		// Fan is already switched off.
		return;
	}

	if (NewDutyCyclePercent < 0.1)
	{
		if ((currentlySetFanDutyCycle >= 0.1) && (queuedSlowdownFanDutyCycle >= 0.1))
		{
			if (debug_SetFanDutyCycle)
			{
				const uint32_t switchOffTimeRemainingMs = SLOWDOWN_WAIT_TIMER_MS - (millis() - millisValueAtSlowDownDelayTimerStart);
				std::string fanSpeedReductionMsg = Utils::StringFormat("Fan will be switched off in %ims", switchOffTimeRemainingMs);
				SerialHandler::SafeWriteLn(fanSpeedReductionMsg, true);
			}
			queueFanSpeedReduction(0);
		}
		return;
	}

	const float newDutyCycleMinSpeedCorrection = (NewDutyCyclePercent > FAN_MIN_RUNNING_SPEED_PERCENT) ?
	                                             NewDutyCyclePercent :
	                                             FAN_MIN_RUNNING_SPEED_PERCENT;

	if (lastRpmMeasurement < FAN_MIN_STARTUP_RPM)
	{
		currentState = StartingUp;
	}
	else
	{
		currentState = Running;

		const float differenceBetweenNewAndCurrentDutyCycles = newDutyCycleMinSpeedCorrection - currentlySetFanDutyCycle;
		if ((differenceBetweenNewAndCurrentDutyCycles < 0.1) && (differenceBetweenNewAndCurrentDutyCycles > -0.1)) {
			const float differenceBetweenNewAndQueuedDutyCycles = NewDutyCyclePercent - queuedSlowdownFanDutyCycle;
			if (isSlowdownQueued && (differenceBetweenNewAndQueuedDutyCycles < 0.1) &&
			    (differenceBetweenNewAndQueuedDutyCycles > -0.1)) {
				// Not enough difference between old and new Duty Cycles to justify change.
				return;
			}
		}
	}

	if (newDutyCycleMinSpeedCorrection > currentlySetFanDutyCycle)
	{
		if (debug_SetFanDutyCycle)
		{
			std::string fanSpeedIncreaseMsg = (NewDutyCyclePercent > FAN_MIN_RUNNING_SPEED_PERCENT) ?
				Utils::StringFormat("Fan duty cycle will be increased to %0.1f", newDutyCycleMinSpeedCorrection) :
				Utils::StringFormat("Fan duty cycle of %0.1f requested, but will be increased to %0.1f instead.", NewDutyCyclePercent, newDutyCycleMinSpeedCorrection);
			SerialHandler::SafeWriteLn(fanSpeedIncreaseMsg, true);
		}
		changeFanDutyCycle(newDutyCycleMinSpeedCorrection);
		return;
	}

	if (debug_SetFanDutyCycle)
	{
		std::string fanSpeedReductionMsg = Utils::StringFormat("Fan duty cycle will be reduced to %0.1f in %ims", newDutyCycleMinSpeedCorrection, SLOWDOWN_WAIT_TIMER_MS);
		SerialHandler::SafeWriteLn(fanSpeedReductionMsg, true);
	}
	queueFanSpeedReduction(newDutyCycleMinSpeedCorrection);
}

/**
 * @brief  Checks if it is time to slow down the fan, if a slowdown is queued.
*/
void FanControl::UpdateSlowdownState()
{
	if (!isSlowdownQueued)
	{
		return;
	}

	if ((millis() - millisValueAtSlowDownDelayTimerStart) < SLOWDOWN_WAIT_TIMER_MS)
	{
		return;
	}

	if (queuedSlowdownFanDutyCycle < 0.1)
	{
		SerialHandler::SafeWriteLn("Fan has been shut down.", debug_UpdateSlowdownState);
		switchOffFan();
		return;
	}

	if (debug_UpdateSlowdownState)
	{
		std::string fanSpeedReductionMsg = Utils::StringFormat("Fan duty cycle decreased to %0.1f", queuedSlowdownFanDutyCycle);
		SerialHandler::SafeWriteLn(fanSpeedReductionMsg, true);
	}
	changeFanDutyCycle(queuedSlowdownFanDutyCycle);
	queuedSlowdownFanDutyCycle = 0;
	isSlowdownQueued = false;
}

/**
 * @brief                       Change the fan's duty cycle.
 *
 * @param  NewDutyCyclePercent  The new duty cycle as a percentage between 0.0 and 100.0
*/
void FanControl::changeFanDutyCycle(const float NewDutyCyclePercent)
{
	const float newDutyCyclePercent = (currentState == StartingUp) ?
		FAN_MIN_STARTUP_SPEED_PERCENT :
        NewDutyCyclePercent;

	int32_t newFanSpeedAnalog = static_cast<int32_t>(newDutyCyclePercent * PWM_RESOLUTION);
	analogWrite(FAN_PWM_SPEED_CONTROL_PIN, newFanSpeedAnalog);
	digitalWrite(FAN_POWER_MOSFET_PIN, HIGH);
	currentlySetFanDutyCycle = NewDutyCyclePercent;
}

/**
 * @brief  Interrupt handler function that is triggered when a rising edge occurs on the fan's speed sense pin.'
*/
void IRAM_ATTR FanControl::fanSpeedPulseInterruptHandler()
{
	speedSensePinPulseCountSinceLastCheck++;
}

/**
 * @brief                       Queues a reduction in the fan's speed.
 *
 * @param  NewDutyCyclePercent  The new duty cycle as a percentage between 0.0 and 100.0
*/
void FanControl::queueFanSpeedReduction(const float NewDutyCyclePercent)
{
	isSlowdownQueued = true;

	float differenceInFanDutyCycles = queuedSlowdownFanDutyCycle - NewDutyCyclePercent;
	if (differenceInFanDutyCycles < 0.1 && differenceInFanDutyCycles > -0.1)
	{
		return;
	}

	queuedSlowdownFanDutyCycle = NewDutyCyclePercent;
	millisValueAtSlowDownDelayTimerStart = millis();
}

/**
 * @brief  Switch the fan off.
*/
void FanControl::switchOffFan()
{
	analogWrite(FAN_PWM_SPEED_CONTROL_PIN, 0);
	digitalWrite(FAN_POWER_MOSFET_PIN, LOW);
	currentState = SwitchedOff;
	isSlowdownQueued = false;
	currentlySetFanDutyCycle = 0.0;
	queuedSlowdownFanDutyCycle = 0.0;
}

/**
 * @brief  Used to instruct given functions to use their debug code.
 *
 * @note   Uncomment the booleans that represent the functions you want to debug.
*/
void FanControl::enableDebugTriggers()
{
//	debug_GetFanRpm = true;
//	debug_SetFanDutyCycle = true;
//	debug_UpdateSlowdownState = true;
}
