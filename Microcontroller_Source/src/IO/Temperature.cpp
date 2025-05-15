// This code is provided under the MPL v2.0 license. Copyright 2025 Xavier du Hecquet de Rauville
// Details may be found in License.txt
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
//  This Source Code Form is "Incompatible With Secondary Licenses", as
//  defined by the Mozilla Public License, v. 2.0.


#pragma clang diagnostic push
#pragma ide diagnostic ignored "modernize-use-auto"


#include "Temperature.h"

#include <Arduino.h>


#define THERMO_RESISTOR_VOLTAGE_READ_GPIO   0
#define THERMO_RESISTOR_VSS_GPIO            10

#define NUM_TEMP_SAMPLES_PER_READING            200
#define ZERO_DEGREES_C_THERMORESISTOR_VOLTAGE   170
#define VOLTAGE_STEPS_BETWEEN_DEGREES_C         9

#define CAPACITOR_CHARGING_TIME_MS          (2)
#define WAIT_TIME_AFTER_FAULT_MS            (100)
#define WAIT_TIME_BETWEEN_SAMPLES_US        (100)

#define PROBE_UNPLUGGED_MAX_VALUE 30
#define PROBE_SHORT_CIRCUIT_MIN_VALUE 3900


bool Temperature::isFanSwitchedOn = false;
bool Temperature::isPidControllerReadyForTempMeasurement = false;
bool Temperature::isMostRecentTemperatureReadingsArrayInitialised = false;
int16_t Temperature::numVoltageReadingsOutstanding = 0;
uint32_t Temperature::startingMillisValue = 0;
uint32_t Temperature::startingMicrosValue = 0;
uint32_t Temperature::targetWaitTimeMs = 0;
uint32_t Temperature::targetWaitTimeUs = 0;
uint32_t Temperature::waitTimeAfterReadingDoneMs = 0;
uint64_t Temperature::thermoresistorReadingsAccumulatedSoFar = 0;
Temperature::TempReadingStage Temperature::currentStage = Temperature::TempReadingStage::Idle;
std::array<float, 3> Temperature::mostRecentTemperatureReadings = {};


void Temperature::Init(uint32_t WaitTimeAfterReadingDoneMs)
{
	pinMode(THERMO_RESISTOR_VSS_GPIO, OUTPUT);
	digitalWrite(THERMO_RESISTOR_VSS_GPIO, LOW);

	pinMode(THERMO_RESISTOR_VOLTAGE_READ_GPIO, INPUT);
	analogReadResolution(12);

	waitTimeAfterReadingDoneMs = WaitTimeAfterReadingDoneMs - CAPACITOR_CHARGING_TIME_MS - (WAIT_TIME_BETWEEN_SAMPLES_US * NUM_TEMP_SAMPLES_PER_READING / 1000) + 1;
}

TempReadData Temperature::Read()
{
	TempReadData tempReadData {};
	tempReadData.Result = TempReadingResult::ProcessingCurrentRequest;

	switch (currentStage)
	{
		case Idle:
		{
			startChargingFilterCapacitor();
			return tempReadData;
		}

		case ChargingFilterCapacitor:
		{
			if (hasEnoughMillisecondsElapsed())
			{
				beginCollectingTempReadings();
			}
			return tempReadData;
		}

		case CollectingTempSample:
		{
			return collectTempReading();
		}

		case WaitingForNextTempSample:
		{
			if (hasEnoughMicrosecondsElapsed())
			{
				currentStage = TempReadingStage::CollectingTempSample;
			}
			return tempReadData;
		}

		case WaitingForPIDController:
		{
			tempReadData.Result = TempReadingResult::LockoutActive;
			if (isPidControllerReadyForTempMeasurement)
			{
				currentStage = TempReadingStage::Idle;
				isPidControllerReadyForTempMeasurement = false;
			}
			return tempReadData;
		}

		case FaultLockOut:
		{
			tempReadData.Result = TempReadingResult::LockoutActive;
			if (hasEnoughMillisecondsElapsed())
			{
				currentStage = TempReadingStage::Idle;
			}
			return tempReadData;
		}


		default:
		{
			return tempReadData;
		}
	}
}

void Temperature::SetFanPowerState(bool IsFanSwitchedOn)
{
	isFanSwitchedOn = IsFanSwitchedOn;
}

void Temperature::SetPidReadyForNextTempReading()
{
	isPidControllerReadyForTempMeasurement = true;
}

void Temperature::startChargingFilterCapacitor()
{
	digitalWrite(THERMO_RESISTOR_VSS_GPIO, HIGH);
	startingMillisValue = millis();
	targetWaitTimeMs = CAPACITOR_CHARGING_TIME_MS;
	currentStage = TempReadingStage::ChargingFilterCapacitor;
}

void Temperature::beginCollectingTempReadings()
{
	numVoltageReadingsOutstanding = NUM_TEMP_SAMPLES_PER_READING;
	currentStage = TempReadingStage::CollectingTempSample;
}

TempReadData Temperature::collectTempReading()
{
	TempReadData tempReadData {};

	const uint32_t thermoVoltageBitmask = getThermoresistorVoltageReading();

	const TempReadingResult tempReadingResult = checkVoltageReadingForFaults(thermoVoltageBitmask);
	if (tempReadingResult != TempReadingResult::TempReadSuccessfully)
	{
		tempReadData.Result = tempReadingResult;
		lockoutAfterThermoresistorFault();
		return tempReadData;
	}

	if (!checkIfAllVoltageReadingsDone(thermoVoltageBitmask))
	{
		tempReadData.Result = TempReadingResult::ProcessingCurrentRequest;
		currentStage = TempReadingStage::WaitingForNextTempSample;
		return tempReadData;
	}

	const float mostRecentTemperatureReading = convertThermoresistorVoltagesToTemperature();
	float averageOfRecentTemperatureReadings = 0.0;
	for (int i = 0; i < mostRecentTemperatureReadings.size(); ++i)
	{
		if ((i >= (mostRecentTemperatureReadings.size() - 1)) || (!isMostRecentTemperatureReadingsArrayInitialised))
		{
			mostRecentTemperatureReadings[i] = mostRecentTemperatureReading;
		}
		else
		{
			mostRecentTemperatureReadings[i] = mostRecentTemperatureReadings[i + 1];
		}
		averageOfRecentTemperatureReadings += mostRecentTemperatureReadings[i];
	}

	isMostRecentTemperatureReadingsArrayInitialised = true;
	tempReadData.Temp = averageOfRecentTemperatureReadings / mostRecentTemperatureReadings.size();
	tempReadData.Result = TempReadingResult::TempReadSuccessfully;

	currentStage = TempReadingStage::WaitingForPIDController;
	startingMillisValue = millis();
	targetWaitTimeMs = waitTimeAfterReadingDoneMs;
	return tempReadData;
}

uint32_t Temperature::getThermoresistorVoltageReading()
{
	return analogRead(THERMO_RESISTOR_VOLTAGE_READ_GPIO);
}

TempReadingResult Temperature::checkVoltageReadingForFaults(const uint32_t VoltageBitmask)
{
	if (VoltageBitmask <= PROBE_UNPLUGGED_MAX_VALUE)
	{
		return TempReadingResult::ProbeUnplugged;
	}

	if (VoltageBitmask >= PROBE_SHORT_CIRCUIT_MIN_VALUE)
	{
		return TempReadingResult::ProbeShortCircuit;
	}

	return TempReadingResult::TempReadSuccessfully;
}

bool Temperature::checkIfAllVoltageReadingsDone(const uint32_t VoltageBitmask)
{
	thermoresistorReadingsAccumulatedSoFar += VoltageBitmask;
	numVoltageReadingsOutstanding -= 1;
	if (numVoltageReadingsOutstanding <= 0)
	{
		return true;
	}

	startingMicrosValue = micros();
	targetWaitTimeUs = WAIT_TIME_BETWEEN_SAMPLES_US;
	return false;
}

float Temperature::convertThermoresistorVoltagesToTemperature()
{
	digitalWrite(THERMO_RESISTOR_VSS_GPIO, LOW);

	float averageVoltageReading = static_cast<float>(thermoresistorReadingsAccumulatedSoFar);
	averageVoltageReading /= NUM_TEMP_SAMPLES_PER_READING;
	thermoresistorReadingsAccumulatedSoFar = 0;

	const float temperature = (averageVoltageReading - ZERO_DEGREES_C_THERMORESISTOR_VOLTAGE) / VOLTAGE_STEPS_BETWEEN_DEGREES_C;
	return temperature;
}

bool Temperature::hasEnoughMillisecondsElapsed()
{
	if ((millis() - startingMillisValue) >= targetWaitTimeMs)
	{
		return true;
	}

	return false;
}

bool Temperature::hasEnoughMicrosecondsElapsed()
{
	if ((micros() - startingMicrosValue) >= targetWaitTimeUs)
	{
		return true;
	}

	return false;
}

void Temperature::lockoutAfterThermoresistorFault()
{
	digitalWrite(THERMO_RESISTOR_VSS_GPIO, LOW);
	startingMillisValue = millis();
	targetWaitTimeMs = WAIT_TIME_AFTER_FAULT_MS;
	currentStage = TempReadingStage::FaultLockOut;

	numVoltageReadingsOutstanding = 0;
	thermoresistorReadingsAccumulatedSoFar = 0;
}

#pragma clang diagnostic pop
