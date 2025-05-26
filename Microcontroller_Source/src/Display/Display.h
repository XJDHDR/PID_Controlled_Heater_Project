// This code is provided under the MPL v2.0 license. Copyright 2025 Xavier du Hecquet de Rauville
// Details may be found in License.txt
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
//  This Source Code Form is "Incompatible With Secondary Licenses", as
//  defined by the Mozilla Public License, v. 2.0.

#ifndef ENGINEERING_PROJECT_DISPLAY_H
#define ENGINEERING_PROJECT_DISPLAY_H

#include <lvgl.h>
#include "LovyanGfxConfig.h"

#include "InitDataTypes/PIDControllerData.h"
#include "Display/Screens/AllScreens.h"
#include "Display/Screens/ConfigPIDControlPart1.h"

/**
 * @brief  Contains the logic for interfacing with the display.
 * This includes managing the screen currently being displayed, backlight, touch and LVGL.
*/
class Display
{
public:
	static void Init(float TargetTemperature, PIDControllerInitData ConfigData);
	static void Update();
	static void GetAllChangedFloatSettings(std::vector<PIDFloatDataPacket>* ChangedFloatSettings);
	static void GetAllChangedIntSettings(std::vector<PIDIntDataPacket>* ChangedIntSettings);

private:
	static bool debug_Update;
	static bool debug_CheckForLvglUpdate;

	static uint32_t millisValueAtLastLvglUpdate;
	static uint32_t timeUntilNextLvglUpdateMs;

	static uint8_t displayBuffer[];

	static Screens currentScreen;

	static LovyanGfxConfig displayDriver;
	static lv_display_t* display;
	static lv_indev_t* touchscreen;

	static lv_style_t buttonLabelTextStyle;


	static void checkForLvglUpdate();
	static void checkForScreenSwitchRequired();
	static void checkIfSwitchRequiredOnCurrentScreen(Screens (*currentScreenIsSwitchRequiredFunction)(), void (*currentScreenHideFunction)());

	static void getTouchData(lv_indev_t* Indev, lv_indev_data_t* Data);
	static void flushDisplay(lv_display_t* TargetDisplay, const lv_area_t* CoordinatesForScreenUpdateArea, uint8_t* NewPixelColourBytes);
	static uint32_t tickCounter();

	static void enableDebugTriggers();
};

#endif //ENGINEERING_PROJECT_DISPLAY_H
