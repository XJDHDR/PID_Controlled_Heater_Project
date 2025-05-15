// This code is provided under the MPL v2.0 license. Copyright 2025 Xavier du Hecquet de Rauville
// Details may be found in License.txt
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
//  This Source Code Form is "Incompatible With Secondary Licenses", as
//  defined by the Mozilla Public License, v. 2.0.

#ifndef ENGINEERING_PROJECT_TEMPERATURE_H
#define ENGINEERING_PROJECT_TEMPERATURE_H

#include <array>
#include <cstdint>
#include <utility>

enum TempReadingResult
{
	ProcessingCurrentRequest,
	TempReadSuccessfully,
	ProbeUnplugged,
	ProbeShortCircuit,
	LockoutActive
};

struct TempReadData
{
	TempReadingResult Result;
	float Temp;
};

class Temperature
{
public:
	static void Init(uint32_t WaitTimeAfterReadingDoneMs);
	static TempReadData Read();
	static void SetFanPowerState(bool IsFanSwitchedOn);
	static void SetPidReadyForNextTempReading();

private:
	enum TempReadingStage
	{
		Idle,
		ChargingFilterCapacitor,
		CollectingTempSample,
		WaitingForNextTempSample,
		WaitingForPIDController,
		FaultLockOut
	};

	static bool isFanSwitchedOn;
	static bool isPidControllerReadyForTempMeasurement;
	static bool isMostRecentTemperatureReadingsArrayInitialised;
	static int16_t numVoltageReadingsOutstanding;
	static uint32_t startingMillisValue;
	static uint32_t startingMicrosValue;
	static uint32_t targetWaitTimeMs;
	static uint32_t targetWaitTimeUs;
	static uint32_t waitTimeAfterReadingDoneMs;
	static uint64_t thermoresistorReadingsAccumulatedSoFar;
	static TempReadingStage currentStage;
	static std::array<float, 3> mostRecentTemperatureReadings;

	static void startChargingFilterCapacitor();
	static void beginCollectingTempReadings();
	static TempReadData collectTempReading();
	static uint32_t getThermoresistorVoltageReading();
	static TempReadingResult checkVoltageReadingForFaults(uint32_t VoltageBitmask);
	static bool checkIfAllVoltageReadingsDone(uint32_t VoltageBitmask);
	static float convertThermoresistorVoltagesToTemperature();
	static bool hasEnoughMillisecondsElapsed();
	static bool hasEnoughMicrosecondsElapsed();
	static void lockoutAfterThermoresistorFault();
};


#endif //ENGINEERING_PROJECT_TEMPERATURE_H
