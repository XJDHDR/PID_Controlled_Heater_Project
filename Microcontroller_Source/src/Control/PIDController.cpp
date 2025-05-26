// This code is provided under the MPL v2.0 license. Copyright 2025 Xavier du Hecquet de Rauville
// Details may be found in License.txt
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
//  This Source Code Form is "Incompatible With Secondary Licenses", as
//  defined by the Mozilla Public License, v. 2.0.


#include "PIDController.h"
#include "Misc/SerialHandler.h"
#include "Misc/Utils.h"

#include <Arduino.h>
#include <unordered_map>


#define MAX_TEMPERATURE_SET_POINT   40.0
#define MIN_TEMPERATURE_SET_POINT   (-20.0)
#define ERROR_RANGE                 0.1
#define TIME_UNTIL_TEMP_ERROR_LOCKOUT_MS    (30 * 1000)


bool PIDController::debug_Update = false;
bool PIDController::debug_SetControlLoopActiveStatus = false;
bool PIDController::debug_ChangeFloatSettings = false;
bool PIDController::debug_ChangeIntSettings = false;
bool PIDController::debug_outputGraph = false;
bool PIDController::debug_updateLoopEarlyReturnChecks = false;
bool PIDController::debug_pidCalculations = false;
bool PIDController::debug_calculateProportionalTerm = false;
bool PIDController::debug_calculateIntegralAccumulation = false;
bool PIDController::debug_calculateDerivativeTerm = false;

bool PIDController::hasCurrentTemperatureBeenUpdatedSinceLastLoop = false;
bool PIDController::isControlLoopEnabled = false;
bool PIDController::isTemperatureErrorLockoutActive = true;     // Initialise as locked out until first temp reading arrives.
bool PIDController::newLoopHasRun = false;
uint32_t PIDController::millisValueAtEndOfLastLoop = 0;
uint32_t PIDController::millisValueAtLastTempReading = 0;
float PIDController::currentDutyCyclePercent = 0.0;
float PIDController::currentTemperatureReadingDegCent = 0.0;
float PIDController::currentTemperatureSetPointDegCent = 0.0;
float PIDController::integralAccumulator = 0.0;
float PIDController::previousError = 0.0;
std::array<float, 3> PIDController::mostRecentDerivativeTerms = {};

int32_t PIDController::loopTimeStepMs;
float PIDController::loopTimeStepMinutes;
float PIDController::proportionalGain;
float PIDController::integralGain;
float PIDController::integralWindupLimitMax;
float PIDController::integralWindupLimitMin;
float PIDController::derivativeGain;
float PIDController::derivativeTermMaxValue;
float PIDController::derivativeTermMinValue;
float PIDController::outputMaxValue;


/**
 * @brief             Initialises the PID Controller class.
 *
 * @param  InputData  A struct containing the PID Controller's settings.
*/
void PIDController::Init(PIDControllerInitData InputData)
{
	enableDebugTriggers();

	currentTemperatureSetPointDegCent = InputData.TemperatureSetPointDegCent;
	loopTimeStepMs = InputData.LoopTimeStepMs;
	convertLoopTimeStepMsToMinutes();
	proportionalGain = InputData.ProportionalGain;
	integralGain = InputData.IntegralGain;
	integralWindupLimitMax = InputData.IntegralWindupLimitMax;
	integralWindupLimitMin = InputData.IntegralWindupLimitMin;
	derivativeGain = InputData.DerivativeGain;
	derivativeTermMaxValue = InputData.DerivativeTermMaxValue;
	derivativeTermMinValue = InputData.DerivativeTermMinValue;
	outputMaxValue = InputData.OutputMaxValue;

	previousError = currentTemperatureSetPointDegCent - currentTemperatureReadingDegCent;
}

/**
 * @brief  Updates the internal state of the PID Controller.
*/
void PIDController::Update()
{
	if (updateLoopEarlyReturnChecks())
	{
		return;
	}

	pidCalculations calculations = doPIDCalculations();

	float output = calculations.ProportionalTerm + integralAccumulator + calculations.DerivativeTerm;

	if (debug_Update)
	{
		std::string desiredTemperatureMsg = Utils::StringFormat("PID loop done with calculated output: %0.2f", output);
		SerialHandler::SafeWriteLn(desiredTemperatureMsg, true);
	}

	if (output <= 0)
	{
		currentDutyCyclePercent = 0.0;
	}
	else if (output >= outputMaxValue)
	{
		currentDutyCyclePercent = 100.0;
	}
	else
	{
		currentDutyCyclePercent = output / outputMaxValue * 100.0f;
	}

	outputGraph(calculations, output);

	millisValueAtEndOfLastLoop = millis();
	hasCurrentTemperatureBeenUpdatedSinceLastLoop = false;
	newLoopHasRun = true;
}

/**
 * @brief  Tells the PID Controller that there was a fault in the temperature measurement class.
*/
void PIDController::ActivateTemperatureLockout()
{
	isTemperatureErrorLockoutActive = true;
}

/**
 * @brief                       Increase or decrease the temperature set point.
 *
 * @param  ChangeAmountDegCent  How much the set point should be changed.
 *
 * @return                      The new temperature set point.
*/
float PIDController::ChangeTemperatureSetPoint(float ChangeAmountDegCent)
{
	const float newTemperatureSetPointDegCent = currentTemperatureSetPointDegCent + ChangeAmountDegCent;
	if (newTemperatureSetPointDegCent >= MAX_TEMPERATURE_SET_POINT)
	{
		currentTemperatureSetPointDegCent = MAX_TEMPERATURE_SET_POINT;
	}
	else if (newTemperatureSetPointDegCent <= MIN_TEMPERATURE_SET_POINT)
	{
		currentTemperatureSetPointDegCent = MIN_TEMPERATURE_SET_POINT;
	}
	else
	{
		currentTemperatureSetPointDegCent = newTemperatureSetPointDegCent;
	}

	return currentTemperatureSetPointDegCent;
}

/**
 * @brief   Gets the duty cycle currently calculated by the PID Controller.
 *
 * @return  The current duty cycle as a percentage.
*/
float PIDController::GetCurrentDutyCyclePercent()
{
	return currentDutyCyclePercent;
}

/**
 * @brief   Gets the temperature set point the PID Controller is currently using.
 *
 * @return  The current set point in °C.
*/
float PIDController::GetTemperatureSetPoint()
{
	return currentTemperatureSetPointDegCent;
}

/**
 * @brief   Checks if the PID Controller has calculated a new duty cycle value since the last call to this function.
 *
 * @return  True if a new calculation has occurred. False otherwise.
*/
bool PIDController::HasNewLoopRunSinceLastCheck()
{
	if (newLoopHasRun)
	{
		newLoopHasRun = false;
		return true;
	}

	return false;
}

/**
 * @brief   Checks if the PID Controller's loop is currently active.
 *
 * @return  True if the loop is active. False if it is paused.
*/
bool PIDController::IsLoopActive()
{
	return (isControlLoopEnabled && !isTemperatureErrorLockoutActive);
}

/**
 * @brief                      Provides the Controller with a new temperature reading.
 *
 * @param  CurrentTemperature  The new temperature reading in °C.
*/
void PIDController::SetCurrentTemperature(float CurrentTemperature)
{
	isTemperatureErrorLockoutActive = false;
	hasCurrentTemperatureBeenUpdatedSinceLastLoop = true;
	currentTemperatureReadingDegCent = CurrentTemperature;
	millisValueAtLastTempReading = millis();
}

/**
 * @brief                  Tells the Controller whether or not to pause the control loop.
 *
 * @param  ShouldActivate  True if the loop should be active. False if it should be paused.
*/
void PIDController::SetControlLoopIsEnabled(bool ShouldActivate)
{
	if (ShouldActivate == isControlLoopEnabled)
	{
		return;
	}

	if (debug_SetControlLoopActiveStatus)
	{
		std::string newActiveStateMsg = Utils::StringFormat("Changing PI Active state to: %s", ShouldActivate ? "On" : "Off");
		SerialHandler::SafeWriteLn(newActiveStateMsg, true);
	}

	previousError = currentTemperatureSetPointDegCent - currentTemperatureReadingDegCent;
	isControlLoopEnabled = ShouldActivate;
}

/**
 * @brief                        Used to provide the Controller with changes to any of its floating point settings.
 *
 * @param  ChangedFloatSettings  A Vector containing the changed float settings.
*/
void PIDController::ChangeFloatSettings(std::vector<PIDFloatDataPacket>* ChangedFloatSettings)
{
	for (PIDFloatDataPacket packet : *ChangedFloatSettings)
	{
		switch (packet.Setting)
		{
			case TemperatureSetPoint:
				currentTemperatureSetPointDegCent = packet.Value;
				break;

			case ProportionalGain:
				proportionalGain = packet.Value;
				break;

			case IntegralGain:
				integralGain = packet.Value;
				break;

			case IntegralWindupLimitMax:
				integralWindupLimitMax = packet.Value;
				break;

			case IntegralWindupLimitMin:
				integralWindupLimitMin = packet.Value;
				break;

			case DerivativeGain:
				derivativeGain = packet.Value;
				break;

			case DerivativeTermMaxValue:
				derivativeTermMaxValue = packet.Value;
				break;

			case DerivativeTermMinValue:
				derivativeTermMinValue = packet.Value;
				break;

			case OutputMaxValue:
				outputMaxValue = packet.Value;
				break;


			case LoopTimeStep:
				break;
		}

		if (debug_ChangeFloatSettings)
		{
			std::unordered_map<PIDSettings, std::string> enumMap = {
					{TemperatureSetPoint, "TemperatureSetPoint"},
					{LoopTimeStep, "LoopTimeStep"},
					{ProportionalGain, "ProportionalGain"},
					{IntegralGain, "IntegralGain"},
					{IntegralWindupLimitMax, "IntegralWindupLimitMax"},
					{IntegralWindupLimitMin, "IntegralWindupLimitMin"},
					{DerivativeGain, "DerivativeGain"},
					{DerivativeTermMaxValue, "DerivativeTermMaxValue"},
					{DerivativeTermMinValue, "DerivativeTermMinValue"},
					{OutputMaxValue, "OutputMaxValue"}
			};
			std::string newActiveStateMsg = Utils::StringFormat("Setting %s was changed to: %0.1f", enumMap.find(packet.Setting)->second.c_str(), packet.Value);
			SerialHandler::SafeWriteLn(newActiveStateMsg, true);
		}
	}
}

/**
 * @brief                      Used to provide the Controller with changes to any of its integer settings.
 *
 * @param  ChangedIntSettings  A Vector containing the changed int settings.
*/
void PIDController::ChangeIntSettings(std::vector<PIDIntDataPacket>* ChangedIntSettings)
{
	for (PIDIntDataPacket packet : *ChangedIntSettings)
	{
		switch (packet.Setting)
		{
			case LoopTimeStep:
				loopTimeStepMs = packet.Value;
				convertLoopTimeStepMsToMinutes();
				break;


			case TemperatureSetPoint:
			case ProportionalGain:
			case IntegralGain:
			case IntegralWindupLimitMax:
			case IntegralWindupLimitMin:
			case DerivativeGain:
			case DerivativeTermMaxValue:
			case DerivativeTermMinValue:
			case OutputMaxValue:
				break;
		}

		if (debug_ChangeIntSettings)
		{
			std::unordered_map<PIDSettings, std::string> enumMap = {
					{TemperatureSetPoint, "TemperatureSetPoint"},
					{LoopTimeStep, "LoopTimeStep"},
					{ProportionalGain, "ProportionalGain"},
					{IntegralGain, "IntegralGain"},
					{IntegralWindupLimitMax, "IntegralWindupLimitMax"},
					{IntegralWindupLimitMin, "IntegralWindupLimitMin"},
					{DerivativeGain, "DerivativeGain"},
					{DerivativeTermMaxValue, "DerivativeTermMaxValue"},
					{DerivativeTermMinValue, "DerivativeTermMinValue"},
					{OutputMaxValue, "OutputMaxValue"}
			};
			std::string newActiveStateMsg = Utils::StringFormat("Setting %s was changed to: %i", enumMap.find(packet.Setting)->second.c_str(), packet.Value);
			SerialHandler::SafeWriteLn(newActiveStateMsg, true);
		}
	}
}

/**
 * @brief    Check if there is a reason the Controller should not do an update (e.g. paused, not enough time elapsed, etc.).
 *
 * @returns  True if the class is not ready for an updated calculation for any reason. False otherwise.
*/
bool PIDController::updateLoopEarlyReturnChecks()
{
	if (isTemperatureErrorLockoutActive)
	{
		SerialHandler::SafeWriteLn("PID temperature lockout is active.", debug_updateLoopEarlyReturnChecks);
		millisValueAtEndOfLastLoop = millis();
		currentDutyCyclePercent = 0.0;
		integralAccumulator = 0.0;
		return true;
	}

	if (!isControlLoopEnabled)
	{
		SerialHandler::SafeWriteLn("PID Control loop is inactive.", debug_updateLoopEarlyReturnChecks);
		millisValueAtEndOfLastLoop = millis();
		currentDutyCyclePercent = 0.0;
		integralAccumulator = 0.0;
		return true;
	}

	if ((millis() - millisValueAtLastTempReading) >= TIME_UNTIL_TEMP_ERROR_LOCKOUT_MS)
	{
		SerialHandler::SafeWriteLn("PID temperature lockout check has just activated.", debug_updateLoopEarlyReturnChecks);
		isTemperatureErrorLockoutActive = true;
		return true;
	}

	if ((millis() - millisValueAtEndOfLastLoop) < loopTimeStepMs)
	{
		return true;
	}

	if (!hasCurrentTemperatureBeenUpdatedSinceLastLoop)
	{
		return true;
	}

	return false;
}

/**
 * @brief    Performs the PID calculations.
 *
 * @returns  A struct containing the calculated Proportional and Derivative terms.
*/
PIDController::pidCalculations PIDController::doPIDCalculations()
{
	pidCalculations results = {};

	float error = currentTemperatureSetPointDegCent - currentTemperatureReadingDegCent;
	if ((error < ERROR_RANGE) && (error > -ERROR_RANGE))
	{
		error = 0;
	}

	if (debug_pidCalculations)
	{
		std::string calculatedErrorMsg = Utils::StringFormat("Calculated Error: %0.2f", error);
		SerialHandler::SafeWriteLn(calculatedErrorMsg, true);
	}

	results.ProportionalTerm = calculateProportionalTerm(error);
	calculateIntegralAccumulation(error);
	results.DerivativeTerm = calculateDerivativeTerm(error);

	return results;
}

/**
 * @brief         Calculate the Proportional term.
 *
 * @param  Error  The calculated error value.
 *
 * @return        The Proportional term that was calculated.
*/
float PIDController::calculateProportionalTerm(float Error)
{
	if (proportionalGain < 0.0001)
	{
		return 0.0;
	}

	float proportionalTerm = proportionalGain * Error;

	if (debug_calculateProportionalTerm)
	{
		std::string proportionalCalculationsMsg = Utils::StringFormat("PTerm of %0.2f calculated from PGain of %0.2f",
		                                                              proportionalTerm, proportionalGain);
		SerialHandler::SafeWriteLn(proportionalCalculationsMsg, true);
	}

	return proportionalTerm;
}

/**
 * @brief         Calculate the Integral accumulation.
 *
 * @param  Error  The calculated error value.
*/
void PIDController::calculateIntegralAccumulation(const float Error)
{
	if (integralGain < 0.0001)
	{
		return;
	}

	const float integralAccumulatorChange = Error * integralGain * loopTimeStepMinutes;
	integralAccumulator += integralAccumulatorChange;
	if (integralAccumulator > integralWindupLimitMax)
	{
		integralAccumulator = integralWindupLimitMax;
	}
	else if (integralAccumulator < integralWindupLimitMin)
	{
		integralAccumulator = integralWindupLimitMin;
	}

	if (debug_calculateIntegralAccumulation)
	{
		std::string proportionalCalculationsMsg = Utils::StringFormat("IAccum is %0.2f (%0.2f change) calculated from IGain of %0.2f",
		                                                              integralAccumulator, integralAccumulatorChange, integralGain);
		SerialHandler::SafeWriteLn(proportionalCalculationsMsg, true);
	}
}

/**
 * @brief         Calculate the Derivative term.
 *
 * @param  Error  The calculated error value.
 *
 * @return        The Derivative term that was calculated.
*/
float PIDController::calculateDerivativeTerm(float Error)
{
	if (derivativeGain < 0.0001)
	{
		return 0.0;
	}

	const float errorDifference = Error - previousError;
	previousError = Error;
	if (debug_calculateDerivativeTerm)
	{
		std::string errorDiffMsg = Utils::StringFormat("Error difference: %0.4f", errorDifference);
		SerialHandler::SafeWriteLn(errorDiffMsg, debug_calculateDerivativeTerm);
	}

	float derivativeTerm = derivativeGain * errorDifference / loopTimeStepMinutes;
	if (derivativeTerm > derivativeTermMaxValue)
	{
		derivativeTerm = derivativeTermMaxValue;
	}
	else if (derivativeTerm < derivativeTermMinValue)
	{
		derivativeTerm = derivativeTermMinValue;
	}

	if (debug_calculateDerivativeTerm)
	{
		std::string derivativeCalculationsMsg = Utils::StringFormat("DTerm of %0.4f calculated from DGain of %0.2f", derivativeTerm, derivativeGain);
		SerialHandler::SafeWriteLn(derivativeCalculationsMsg, true);
	}

	return derivativeTerm;
}

void PIDController::convertLoopTimeStepMsToMinutes()
{
	loopTimeStepMinutes = static_cast<float>(loopTimeStepMs) / 1000 / 60;
}

/**
 * @brief                Create graph data and print it to the serial port for the Arduino IDE Serial Plotter.
 *
 * @param  Calculations  The calculated Proportional and Derivative terms.
 * @param  Output        The calculated Output value.
*/
void PIDController::outputGraph(pidCalculations Calculations, float Output)
{
	if (!debug_outputGraph)
	{
		return;
	}

	std::string graphingOutputMsg = Utils::StringFormat("Temperature:%0.2f,", currentTemperatureReadingDegCent);
	graphingOutputMsg += Utils::StringFormat("TemperatureSetPoint:%0.2f,", currentTemperatureSetPointDegCent);
	if (proportionalGain > 0.001)
	{
		graphingOutputMsg += Utils::StringFormat("PTerm:%0.2f,", (Calculations.ProportionalTerm > -10) ? Calculations.ProportionalTerm : -10);
	}
	if (integralGain > 0.001)
	{
		graphingOutputMsg += Utils::StringFormat("IAccumulator:%0.2f,", (integralAccumulator > -10) ? integralAccumulator : -10);
	}
	if (derivativeGain > 0.001)
	{
		graphingOutputMsg += Utils::StringFormat("DTerm:%0.2f,", (Calculations.DerivativeTerm > -10) ? Calculations.DerivativeTerm : -10);
	}
	graphingOutputMsg += Utils::StringFormat("Output:%0.2f,", (Output > -10) ? Output : -10);
	graphingOutputMsg += Utils::StringFormat("LoopTimeStability:%0.2f", (static_cast<float>(millis() - millisValueAtEndOfLastLoop) / loopTimeStepMs * 10));
	SerialHandler::SafeWriteLn(graphingOutputMsg, true);
}

/**
 * @brief  Used to instruct given functions to use their debug code.
 *
 * @note   Uncomment the booleans that represent the functions you want to debug.
*/
void PIDController::enableDebugTriggers()
{
//	debug_Update = true;
//	debug_SetControlLoopActiveStatus = true;
//	debug_ChangeFloatSettings = true;
//	debug_ChangeIntSettings = true;

//	debug_updateLoopEarlyReturnChecks = true;
//	debug_pidCalculations = true;
//	debug_calculateProportionalTerm = true;
//	debug_calculateIntegralAccumulation = true;
//	debug_calculateDerivativeTerm = true;
//	debug_outputGraph = true;
}
