// This code is provided under the MPL v2.0 license. Copyright 2025 Xavier du Hecquet de Rauville
// Details may be found in License.txt
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
//  This Source Code Form is "Incompatible With Secondary Licenses", as
//  defined by the Mozilla Public License, v. 2.0.

#ifndef ENGINEERING_PROJECT_HEATER_CONTROL_H
#define ENGINEERING_PROJECT_HEATER_CONTROL_H

#include <cstdint>

/**
 * @brief  Contains the logic for the Heater Controller.
*/
class HeaterControl
{
public:
	static void Init();
	static uint32_t GetCurrentPowerLevel();
	static void SetFanIsRunning(bool IsFanRunning);
	static void SetHeaterPowerLevel(float NewPowerLevelPercent);
	static void UpdatePwmState();


private:
	enum HeaterStates
	{
		SwitchedOff,
		SwitchedOnPwmState,
		SwitchedOnMaxPower
	};

	static bool debug_Init;
	static bool debug_UpdatePwmState;
	static bool debug_SetHeaterPowerLevel;
	static bool debug_setGpioState;

	static bool isFanRunning;
	static uint8_t currentGpioState;
	static uint32_t currentPowerLevelPercent;
	static uint32_t microsValueAtStartOfThisCycle;
	static uint32_t currentDutyCycleOnTimeUs;
	static HeaterStates heaterState;

	static void setHeaterSwitchedOffState();
	static void setHeaterMaxPowerState();
	static void setGpioState(uint8_t State);

	static void enableDebugTriggers();
};

#endif //ENGINEERING_PROJECT_HEATER_CONTROL_H
