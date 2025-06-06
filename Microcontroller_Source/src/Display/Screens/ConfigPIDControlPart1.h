// This code is provided under the MPL v2.0 license. Copyright 2025 Xavier du Hecquet de Rauville
// Details may be found in License.txt
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
//  This Source Code Form is "Incompatible With Secondary Licenses", as
//  defined by the Mozilla Public License, v. 2.0.

#ifndef ENGINEERING_PROJECT_CONFIGPIDCONTROL_PART1_H
#define ENGINEERING_PROJECT_CONFIGPIDCONTROL_PART1_H

#include <lvgl.h>
#include <array>
#include <cmath>
#include <vector>

#include "Display/LvglHelpers/ConfigScreenHelpers.h"
#include "Display/Screens/AllScreens.h"
#include "InitDataTypes/PIDControllerData.h"

/**
 * @brief  Contains the logic for the screen which contains the PID Time Step, Proportional and Integral Control settings.
*/
class ConfigPIDControlPart1
{
public:
	static void Init(lv_obj_t* TargetScreen, lv_style_t* ButtonLabelTextStyle, PIDControllerInitData ConfigData);
	static Screens IsScreenSwitchRequired();
	static void Hide();
	static void Show();
	static void GetAllChangedFloatSettings(std::vector<PIDFloatDataPacket>* ChangedFloatSettings);
	static void GetAllChangedIntSettings(std::vector<PIDIntDataPacket>* ChangedIntSettings);

private:
	static bool screenSwitchRequired;
	static Screens desiredScreen;

	static std::array<int32_t, 6> rootScreenContainerColumns;
	static std::array<int32_t, 5> rootScreenContainerRows;
	static std::array<int32_t, 5> settingsWidgetsContainerColumns;
	static std::array<int32_t, 6> settingsWidgetsContainerRows;

	static ConfigScreenHelpers::IntSpinboxData loopTimeStep;
	static ConfigScreenHelpers::FloatSpinboxData proportionalGain;
	static ConfigScreenHelpers::FloatSpinboxData integralGain;
	static ConfigScreenHelpers::FloatSpinboxData integralWindupLimitMax;
	static ConfigScreenHelpers::FloatSpinboxData integralWindupLimitMin;

	static lv_obj_t* rootScreenContainer;

	static void settingsConfigBuilder(int32_t WidgetsContainerWidth);
	static void navigationButtonsBuilder(lv_style_t* ButtonLabelTextStyle);
	static void toPreviousConfigScreenButtonPressedEventHandler(__attribute__((unused)) lv_event_t* Event);
	static void returnToMainScreenButtonPressedEventHandler(__attribute__((unused)) lv_event_t* Event);
	static void toNextConfigScreenButtonPressedEventHandler(__attribute__((unused)) lv_event_t* Event);
	static void resetAllCursorPositions();

	static void enableDebugTriggers();
};


#endif //ENGINEERING_PROJECT_CONFIGPIDCONTROL_PART1_H
