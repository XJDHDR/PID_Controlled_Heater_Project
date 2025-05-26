// This code is provided under the MPL v2.0 license. Copyright 2025 Xavier du Hecquet de Rauville
// Details may be found in License.txt
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
//  This Source Code Form is "Incompatible With Secondary Licenses", as
//  defined by the Mozilla Public License, v. 2.0.

#ifndef ENGINEERING_PROJECT_FAN_CONTROL_H
#define ENGINEERING_PROJECT_FAN_CONTROL_H

#include <cstdint>

/**
 * @brief  Contains the logic for the Fan Controller.
*/
class FanControl
{
public:
	/**
	 * @brief  Contains data about whether the fan is on, whether the fan was spinning and the measured RPM.
	*/
	struct FanRpmData
	{
		bool IsFanSwitchedOn;
		bool WasMeasurementTaken;
		bool IsFanSpinning;
		uint32_t Rpm;
	};

	static void Init();
	static FanRpmData GetFanRpm();
	static float GetFanCurrentDutyCycle();
	static void SetFanDutyCycle(float NewDutyCyclePercent);
	static void UpdateSlowdownState();

private:
	enum FanStates
	{
		SwitchedOff,
		StartingUp,
		Running
	};

	static bool debug_GetFanRpm;
	static bool debug_SetFanDutyCycle;
	static bool debug_UpdateSlowdownState;

	static bool isSlowdownQueued;
	static uint32_t lastRpmMeasurement;
	static uint32_t millisValueAtLastRpmCheck;
	static uint32_t millisValueAtSlowDownDelayTimerStart;
	static uint32_t speedSensePinPulseCountSinceLastCheck;
	static float currentlySetFanDutyCycle;
	static float queuedSlowdownFanDutyCycle;

	static FanStates currentState;

	static void changeFanDutyCycle(float NewDutyCyclePercent);
	static void fanSpeedPulseInterruptHandler();
	static void queueFanSpeedReduction(float NewDutyCyclePercent);
	static void switchOffFan();

	static void enableDebugTriggers();
};

#endif //ENGINEERING_PROJECT_FAN_CONTROL_H
