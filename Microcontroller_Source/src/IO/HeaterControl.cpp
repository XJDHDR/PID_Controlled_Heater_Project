// This code is provided under the MPL v2.0 license. Copyright 2025 Xavier du Hecquet de Rauville
// Details may be found in License.txt
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
//  This Source Code Form is "Incompatible With Secondary Licenses", as
//  defined by the Mozilla Public License, v. 2.0.


#include "HeaterControl.h"

#include <Arduino.h>

#include "Misc/SerialHandler.h"
#include "Misc/Utils.h"


#define MIN_POWER_LEVEL             0.5
#define MAX_POWER_LEVEL             99.5
#define ONE_50HZ_WAVE_PERIOD_IN_US          20000
#define ONE_HUNDRED_50HZ_WAVE_PERIODS_IN_US (ONE_50HZ_WAVE_PERIOD_IN_US * 100)

#define SSR_CONTROL_PIN 7


bool HeaterControl::debug_Init = false;
bool HeaterControl::debug_UpdatePwmState = false;
bool HeaterControl::debug_SetHeaterPowerLevel = false;
bool HeaterControl::debug_setGpioState = false;

bool HeaterControl::isFanRunning = false;
uint8_t HeaterControl::currentGpioState = LOW;
uint32_t HeaterControl::currentPowerLevelPercent = 0;
uint32_t HeaterControl::microsValueAtStartOfThisCycle = 0;
uint32_t HeaterControl::currentDutyCycleOnTimeUs = 0;
HeaterControl::HeaterStates HeaterControl::heaterState = HeaterStates::SwitchedOff;


void HeaterControl::Init()
{
	enableDebugTriggers();

	pinMode(SSR_CONTROL_PIN, OUTPUT);
	setHeaterSwitchedOffState();
	setGpioState(LOW);
	SerialHandler::SafeWriteLn("Heater control initialised.", debug_Init);
}

uint32_t HeaterControl::GetCurrentPowerLevel()
{
	return currentPowerLevelPercent;
}

void HeaterControl::SetFanIsRunning(bool IsFanRunning)
{
	isFanRunning = IsFanRunning;
}

void HeaterControl::SetHeaterPowerLevel(const float NewPowerLevelPercent)
{
	// Don't allow the heater to switch on if the fan is not running.
	const uint32_t newPowerLevelPercentFloored = (isFanRunning) ?
		static_cast<uint32_t>(NewPowerLevelPercent) :
		0;

	if (newPowerLevelPercentFloored <= MIN_POWER_LEVEL)
	{
		if (heaterState != SwitchedOff)
		{
			setHeaterSwitchedOffState();
			SerialHandler::SafeWriteLn("Heater switched off.", debug_SetHeaterPowerLevel);
		}
		return;
	}
	if (newPowerLevelPercentFloored >= MAX_POWER_LEVEL)
	{
		if (heaterState != SwitchedOnMaxPower)
		{
			setHeaterMaxPowerState();
			SerialHandler::SafeWriteLn("Heater set to max power.", debug_SetHeaterPowerLevel);
		}
		return;
	}

	if (newPowerLevelPercentFloored == currentPowerLevelPercent)
	{
		return;
	}

	heaterState = SwitchedOnPwmState;
	currentPowerLevelPercent = newPowerLevelPercentFloored;
	currentDutyCycleOnTimeUs = newPowerLevelPercentFloored * ONE_50HZ_WAVE_PERIOD_IN_US;

	if (debug_SetHeaterPowerLevel)
	{
		std::string newPowerLevelMsg = Utils::StringFormat("Heater power level set to: %u%%", currentPowerLevelPercent);
		SerialHandler::SafeWriteLn(newPowerLevelMsg, true);
	}
}

void HeaterControl::UpdatePwmState()
{
	if ((micros() - microsValueAtStartOfThisCycle) >= ONE_HUNDRED_50HZ_WAVE_PERIODS_IN_US)
	{
		microsValueAtStartOfThisCycle = micros();
	}

	switch (heaterState)
	{
		case SwitchedOff:
			SerialHandler::SafeWriteLn("Heater switched into off state.", debug_UpdatePwmState && (currentGpioState == HIGH));
//			SerialHandler::SafeWriteLn("Heater in off state.", debug_UpdatePwmState);
			setGpioState(LOW);
			return;

		case SwitchedOnMaxPower:
			SerialHandler::SafeWriteLn("Heater switched into max power state.", debug_UpdatePwmState && (currentGpioState == LOW));
//			SerialHandler::SafeWriteLn("Heater in max power state.", debug_UpdatePwmState);
			setGpioState(HIGH);
			return;

		case SwitchedOnPwmState:
		default:
//			SerialHandler::SafeWriteLn("Heater in a duty cycle state.", debug_UpdatePwmState);
			if ((micros() - microsValueAtStartOfThisCycle) <= currentDutyCycleOnTimeUs)
			{
				SerialHandler::SafeWriteLn("Heater switched into duty cycle high state.", debug_UpdatePwmState && (currentGpioState == LOW));
				setGpioState(HIGH);
				return;
			}

			SerialHandler::SafeWriteLn("Heater switched into duty cycle low state.", debug_UpdatePwmState && (currentGpioState == HIGH));
			setGpioState(LOW);
			return;
	}
}

void HeaterControl::setHeaterSwitchedOffState()
{
	heaterState = SwitchedOff;
	currentPowerLevelPercent = 0;
}

void HeaterControl::setHeaterMaxPowerState()
{
	currentPowerLevelPercent = 100.0;
	heaterState = SwitchedOnMaxPower;
}

void HeaterControl::setGpioState(const uint8_t State)
{
	if (currentGpioState == State)
	{
		return;
	}

	digitalWrite(SSR_CONTROL_PIN, State);
	currentGpioState = State;

	if (debug_setGpioState)
	{
		std::string newGpioStateMsg = Utils::StringFormat("GPIO state set to: %s", (currentGpioState == HIGH) ? "HIGH" : "LOW");
		SerialHandler::SafeWriteLn(newGpioStateMsg, true);
	}
}

void HeaterControl::enableDebugTriggers()
{
//	debug_Init = true;
//	debug_UpdatePwmState = true;
//	debug_SetHeaterPowerLevel = true;
//	debug_setGpioState = true;
}
