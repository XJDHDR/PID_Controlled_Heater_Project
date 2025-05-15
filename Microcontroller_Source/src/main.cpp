// This code is provided under the MPL v2.0 license. Copyright 2025 Xavier du Hecquet de Rauville
// Details may be found in License.txt
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
//  This Source Code Form is "Incompatible With Secondary Licenses", as
//  defined by the Mozilla Public License, v. 2.0.


#include "main.h"

#include <Arduino.h>

#include "InitDataTypes/PIDControllerData.h"
#include "Control/PIDController.h"
#include "Display/Display.h"
#include "Display/Screens/StatusAkaMain.h"
#include "IO/FanControl.h"
#include "IO/HeaterControl.h"
#include "IO/Temperature.h"
#include "Misc/SerialHandler.h"
#include "Misc/Usb.h"
#include "Misc/Utils.h"


/**
 * @brief  The Arduino API executes this function once after the microcontroller boots up.
*/
void setup()
{
	try
	{
		Main::TrySetup();
	}
	catch (...)
	{
		Utils::ErrorState("An exception occurred in the TrySetup function.");
	}
}

/**
 * @brief  The Arduino API continuously executes this function over and over again while the microcontroller is powered up,
 * starting from after the setup function above returns.
*/
void loop()
{
	try
	{
		Main::TryLoop();
	}
	catch (...)
	{
		Utils::ErrorState("An exception occurred in the TryLoop function.");
	}
}

// TODO: Move this data into a flash read/write class.
float pidControllerTemperatureSetPointDegCent = 22.0;
int32_t pidControllerLoopTimeStepMs = 500;
float pidControllerProportionalGain = 2.5;
float pidControllerIntegralGain = 2.0;
float pidControllerIntegralWindupLimitMax = 4.0;
float pidControllerIntegralWindupLimitMin = -0.5;
float pidControllerDerivativeGain = 0.1;
float pidControllerDerivativeTermMaxValue = 0.5;
float pidControllerDerivativeTermMinValue = -10.0;
float pidControllerOutputMaxValue = 100.0;


/**
 * @brief  Initialises the other classes in this firmware inside an exception handler.
*/
void Main::TrySetup()
{
	Usb::Init();
	SerialHandler::Init(Usb::IsUsbPluggedIn());

	FanControl::Init();
	HeaterControl::Init();
	Temperature::Init(pidControllerLoopTimeStepMs);

	const PIDControllerInitData pIDControllerInitData = {
			pidControllerTemperatureSetPointDegCent,
			pidControllerLoopTimeStepMs,
			pidControllerProportionalGain,
			pidControllerIntegralGain,
			pidControllerIntegralWindupLimitMax,
			pidControllerIntegralWindupLimitMin,
			pidControllerDerivativeGain,
			pidControllerDerivativeTermMaxValue,
			pidControllerDerivativeTermMinValue,
			pidControllerOutputMaxValue
	};
	PIDController::Init(pIDControllerInitData);
	const float targetTemperature = PIDController::GetTemperatureSetPoint();

	Display::Init(targetTemperature, pIDControllerInitData);
}

/**
 * @brief  Executes the firmware's main loop inside an exception handler.
*/
void Main::TryLoop()
{
//	SerialHandler::SetState(Usb::IsUsbPluggedIn());

	const bool isUnitSwitchedOff = StatusAkaMain::IsOnOffButtonInOffState();
	PIDController::SetControlLoopIsEnabled(!isUnitSwitchedOff);

	std::vector<PIDFloatDataPacket> changedFloatSettings = {};
	Display::GetAllChangedFloatSettings(&changedFloatSettings);
	PIDController::ChangeFloatSettings(&changedFloatSettings);

	std::vector<PIDIntDataPacket> changedIntSettings = {};
	Display::GetAllChangedIntSettings(&changedIntSettings);
	PIDController::ChangeIntSettings(&changedIntSettings);

	temperatureReading();

	const float desiredTemperatureChange = StatusAkaMain::GetTargetTemperatureChangeDesiredByUser();
	if ((desiredTemperatureChange >= 0.2) || (desiredTemperatureChange <= -0.2))
	{
//		std::string desiredTemperatureMsg = Utils::StringFormat("Target Temp change desired: %0.1f", desiredTemperatureChange);
//		SerialHandler::SafeWriteLn(desiredTemperatureMsg, true);

		const float newTargetTemperature = PIDController::ChangeTemperatureSetPoint(desiredTemperatureChange);
		StatusAkaMain::SetCurrentTargetTemperature(newTargetTemperature);
	}

	PIDController::Update();

	StatusAkaMain::SetPiControllerStatusIndicator(PIDController::IsLoopActive());

	const float currentPiControllerDutyCycle = PIDController::GetCurrentDutyCyclePercent();

	FanControl::SetFanDutyCycle(currentPiControllerDutyCycle);
	FanControl::UpdateSlowdownState();
	fanSpeedUpdates();

	HeaterControl::SetHeaterPowerLevel(currentPiControllerDutyCycle);
	HeaterControl::UpdatePwmState();
	StatusAkaMain::SetCurrentDutyCycles(FanControl::GetFanCurrentDutyCycle(), HeaterControl::GetCurrentPowerLevel());

	Display::Update();

	SerialHandler::TryWriteBufferToSerial();
}

/**
 * @brief  Gets a temperature reading from the Temperature class, and passes that data to the PID Controller and Display Manager.
*/
void Main::temperatureReading()
{
	if (PIDController::HasNewLoopRunSinceLastCheck())
	{
		Temperature::SetPidReadyForNextTempReading();
	}

	const TempReadData tempResult = Temperature::Read();
	switch (tempResult.Result)
	{
		case TempReadSuccessfully:
			PIDController::SetCurrentTemperature(tempResult.Temp);
			StatusAkaMain::SetCurrentTemperature(tempResult.Temp);
			StatusAkaMain::RemoveErrorCondition(StatusAkaMain::ErrorMessages::ThermoResistorShortCircuit);
			StatusAkaMain::RemoveErrorCondition(StatusAkaMain::ErrorMessages::ThermoResistorUnplugged);
			break;

		case ProbeShortCircuit:
			PIDController::ActivateTemperatureLockout();
			StatusAkaMain::RemoveErrorCondition(StatusAkaMain::ErrorMessages::ThermoResistorUnplugged);
			StatusAkaMain::AddErrorCondition(StatusAkaMain::ErrorMessages::ThermoResistorShortCircuit);
			break;

		case ProbeUnplugged:
			PIDController::ActivateTemperatureLockout();
			StatusAkaMain::RemoveErrorCondition(StatusAkaMain::ErrorMessages::ThermoResistorShortCircuit);
			StatusAkaMain::AddErrorCondition(StatusAkaMain::ErrorMessages::ThermoResistorUnplugged);
			break;

		default:
			break;
	}
}

/**
 * @brief  Gets the fan's speed from the Fan Control class, and passes that data to the classes which use that info.
*/
void Main::fanSpeedUpdates()
{
	FanControl::FanRpmData fanRpmData = FanControl::GetFanRpm();
	if (!fanRpmData.WasMeasurementTaken)
	{
		return;
	}

	Temperature::SetFanPowerState(fanRpmData.IsFanSwitchedOn);
	HeaterControl::SetFanIsRunning(fanRpmData.IsFanSpinning);
	StatusAkaMain::SetCurrentFanRpm(fanRpmData.IsFanSwitchedOn, fanRpmData.Rpm);

	if (fanRpmData.IsFanSpinning || !fanRpmData.IsFanSwitchedOn)
	{
		StatusAkaMain::RemoveErrorCondition(StatusAkaMain::FanStuck);
	}
	else
	{
		StatusAkaMain::AddErrorCondition(StatusAkaMain::FanStuck);
	}
}
