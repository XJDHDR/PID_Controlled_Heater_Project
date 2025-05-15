// This code is provided under the MPL v2.0 license. Copyright 2025 Xavier du Hecquet de Rauville
// Details may be found in License.txt
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
//  This Source Code Form is "Incompatible With Secondary Licenses", as
//  defined by the Mozilla Public License, v. 2.0.


#ifndef ENGINEERING_PROJECT_PI_CONTROLLER_H
#define ENGINEERING_PROJECT_PI_CONTROLLER_H

#include <array>
#include <cstdint>
#include <vector>

#include "InitDataTypes/PIDControllerData.h"

class PIDController
{
public:
	static void Init(PIDControllerInitData InputData);
	static void Update();
	static void ActivateTemperatureLockout();
	static float ChangeTemperatureSetPoint(float ChangeAmountDegCent);
	static float GetCurrentDutyCyclePercent();
	static float GetTemperatureSetPoint();
	static bool HasNewLoopRunSinceLastCheck();
	static bool IsLoopActive();
	static void SetCurrentTemperature(float CurrentTemperature);
	static void SetControlLoopIsEnabled(bool ShouldActivate);
	static void ChangeFloatSettings(std::vector<PIDFloatDataPacket>* ChangedFloatSettings);
	static void ChangeIntSettings(std::vector<PIDIntDataPacket>* ChangedIntSettings);

private:
	struct pidCalculations
	{
		float ProportionalTerm;
		float DerivativeTerm;
	};

	static bool debug_Update;
	static bool debug_SetControlLoopActiveStatus;
	static bool debug_ChangeFloatSettings;
	static bool debug_ChangeIntSettings;
	static bool debug_outputGraph;
	static bool debug_updateLoopEarlyReturnChecks;
	static bool debug_pidCalculations;
	static bool debug_calculateProportionalTerm;
	static bool debug_calculateIntegralAccumulation;
	static bool debug_calculateDerivativeTerm;

	static bool hasCurrentTemperatureBeenUpdatedSinceLastLoop;
	static bool isControlLoopEnabled;
	static bool isTemperatureErrorLockoutActive;
	static bool newLoopHasRun;
	static uint32_t millisValueAtEndOfLastLoop;
	static uint32_t millisValueAtLastTempReading;
	static float currentDutyCyclePercent;
	static float currentTemperatureReadingDegCent;
	static float currentTemperatureSetPointDegCent;
	static float integralAccumulator;
	static float previousError;
	static std::array<float, 3> mostRecentDerivativeTerms;

	static int32_t loopTimeStepMs;
	static float loopTimeStepMinutes;
	static float proportionalGain;
	static float integralGain;
	static float integralWindupLimitMax;
	static float integralWindupLimitMin;
	static float derivativeGain;
	static float derivativeTermMaxValue;
	static float derivativeTermMinValue;
	static float outputMaxValue;

	static bool updateLoopEarlyReturnChecks();
	static pidCalculations doPIDCalculations();
	static float calculateProportionalTerm(float Error);
	static void calculateIntegralAccumulation(float Error);
	static float calculateDerivativeTerm(float Error);
	static void convertLoopTimeStepMsToMinutes();
	static void outputGraph(pidCalculations Calculations, float Output);
	static void enableDebugTriggers();
};

#endif //ENGINEERING_PROJECT_PI_CONTROLLER_H
