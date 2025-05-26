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


#include "ConfigPIDControlPart1.h"

#include <tuple>


#include "Display/LvglHelpers/LvglHelpers.h"
#include "Misc/Utils.h"


bool ConfigPIDControlPart1::screenSwitchRequired = false;
Screens ConfigPIDControlPart1::desiredScreen = Screens::Invalid;

std::array<int32_t, 6> ConfigPIDControlPart1::rootScreenContainerColumns = {8, LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), 8, LV_GRID_TEMPLATE_LAST};
std::array<int32_t, 5> ConfigPIDControlPart1::rootScreenContainerRows = {8, LV_GRID_FR(1), LV_GRID_CONTENT, 8, LV_GRID_TEMPLATE_LAST};
std::array<int32_t, 5> ConfigPIDControlPart1::settingsWidgetsContainerColumns = {LV_GRID_FR(1), LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
std::array<int32_t, 6> ConfigPIDControlPart1::settingsWidgetsContainerRows = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};

ConfigScreenHelpers::IntSpinboxData ConfigPIDControlPart1::loopTimeStep = {};
ConfigScreenHelpers::FloatSpinboxData ConfigPIDControlPart1::proportionalGain = {};
ConfigScreenHelpers::FloatSpinboxData ConfigPIDControlPart1::integralGain = {};
ConfigScreenHelpers::FloatSpinboxData ConfigPIDControlPart1::integralWindupLimitMax = {};
ConfigScreenHelpers::FloatSpinboxData ConfigPIDControlPart1::integralWindupLimitMin = {};

lv_obj_t* ConfigPIDControlPart1::rootScreenContainer;


/**
 * @brief                        Initialises the Config screen class.
 *
 * @param  TargetScreen          The display that this screen will be parented to.
 * @param  ButtonLabelTextStyle  The style that will be applied to the labels of large buttons.
 * @param  ConfigData            A struct containing the PID Controller's settings.
*/
void ConfigPIDControlPart1::Init(lv_obj_t* TargetScreen, lv_style_t* ButtonLabelTextStyle, PIDControllerInitData ConfigData)
{
	enableDebugTriggers();

	const int32_t screenHeight = lv_obj_get_height(TargetScreen);
	const int32_t screenWidth = lv_obj_get_width(TargetScreen);

	rootScreenContainer = LvglHelpers::CreateWidgetContainer(
			TargetScreen, LV_OPA_0, 4, true, screenWidth, screenHeight,
			rootScreenContainerColumns.data(), rootScreenContainerRows.data(),
			false, 0, 0, 0, 0
	);
	Hide();

	loopTimeStep.CurrentValue = ConfigData.LoopTimeStepMs;
	proportionalGain.CurrentValue = ConfigData.ProportionalGain;
	proportionalGain.DecimalPosition = 2;
	integralGain.CurrentValue = ConfigData.IntegralGain;
	integralGain.DecimalPosition = 2;
	integralWindupLimitMax.CurrentValue = ConfigData.IntegralWindupLimitMax;
	integralWindupLimitMax.DecimalPosition = 3;
	integralWindupLimitMin.CurrentValue = ConfigData.IntegralWindupLimitMin;
	integralWindupLimitMin.DecimalPosition = 3;

	const int32_t widgetsContainerWidth = screenWidth - rootScreenContainerColumns[0] - rootScreenContainerColumns.rbegin()[1];

	settingsConfigBuilder(widgetsContainerWidth);
	navigationButtonsBuilder(ButtonLabelTextStyle);
}

/**
 * @brief    Used to figure out if the user has requested a switch to a different screen.
 *
 * @returns  The screen that needs to be switched to, or Invalid if no switch is required.
*/
Screens ConfigPIDControlPart1::IsScreenSwitchRequired()
{
	if (!screenSwitchRequired)
	{
		return Screens::Invalid;
	}

	return desiredScreen;
}

/**
 * @brief    Hides the screen from view.
*/
void ConfigPIDControlPart1::Hide()
{
	lv_obj_add_flag(rootScreenContainer, LV_OBJ_FLAG_HIDDEN);
	screenSwitchRequired = false;
	desiredScreen = Screens::Invalid;
}

/**
 * @brief    Unhides the screen.
*/
void ConfigPIDControlPart1::Show()
{
	lv_obj_remove_flag(rootScreenContainer, LV_OBJ_FLAG_HIDDEN);
	screenSwitchRequired = false;
	desiredScreen = Screens::ConfigPidControlPart1;
}

/**
 * @brief                        Fetches any float settings that have been changed since the last time this function was invoked.
 *
 * @param  ChangedFloatSettings  A Vector that the changed float settings will be added to.
*/
void ConfigPIDControlPart1::GetAllChangedFloatSettings(std::vector<PIDFloatDataPacket>* ChangedFloatSettings)
{
	if (proportionalGain.HasValueBeenChangedSinceLastCheck)
	{
		proportionalGain.HasValueBeenChangedSinceLastCheck = false;
		ChangedFloatSettings->push_back({ProportionalGain, proportionalGain.CurrentValue});
	}
	if (integralGain.HasValueBeenChangedSinceLastCheck)
	{
		integralGain.HasValueBeenChangedSinceLastCheck = false;
		ChangedFloatSettings->push_back({IntegralGain, integralGain.CurrentValue});
	}
	if (integralWindupLimitMax.HasValueBeenChangedSinceLastCheck)
	{
		integralWindupLimitMax.HasValueBeenChangedSinceLastCheck = false;
		ChangedFloatSettings->push_back({IntegralWindupLimitMax, integralWindupLimitMax.CurrentValue});
	}
	if (integralWindupLimitMin.HasValueBeenChangedSinceLastCheck)
	{
		integralWindupLimitMin.HasValueBeenChangedSinceLastCheck = false;
		ChangedFloatSettings->push_back({IntegralWindupLimitMin, integralWindupLimitMin.CurrentValue});
	}
}

/**
 * @brief                        Fetches any integer settings that have been changed since the last time this function was invoked.
 *
 * @param  ChangedFloatSettings  A Vector that the changed integer settings will be added to.
*/
void ConfigPIDControlPart1::GetAllChangedIntSettings(std::vector<PIDIntDataPacket>* ChangedIntSettings)
{
	if (loopTimeStep.HasValueBeenChangedSinceLastCheck)
	{
		loopTimeStep.HasValueBeenChangedSinceLastCheck = false;
		ChangedIntSettings->push_back({LoopTimeStep, loopTimeStep.CurrentValue});
	}
}

/**
 * @brief                         Creates the widgets in the config screen.
 *
 * @param  WidgetsContainerWidth  The width that the widget container needs to be.
*/
void ConfigPIDControlPart1::settingsConfigBuilder(const int32_t WidgetsContainerWidth)
{
	lv_obj_t* settingsWidgetsContainer = LvglHelpers::CreateWidgetContainer(
			rootScreenContainer, LV_OPA_100, 6, false, WidgetsContainerWidth, LV_SIZE_CONTENT,
			settingsWidgetsContainerColumns.data(), settingsWidgetsContainerRows.data(),
			true, 1, 3, 1, 1
	);

	ConfigScreenHelpers::CreateSettingRow(
			settingsWidgetsContainer, "Loop Time\nStep (ms):", 0,
			1, 10000, &loopTimeStep
	);
	ConfigScreenHelpers::CreateSettingRow(
			settingsWidgetsContainer, "Proportion.\nGain:", 1,
			0, 10000, &proportionalGain
	);
	ConfigScreenHelpers::CreateSettingRow(
			settingsWidgetsContainer, "Integral\nGain:", 2,
			0, 10000, &integralGain
	);
	ConfigScreenHelpers::CreateSettingRow(
			settingsWidgetsContainer, "Int. Wind.\nLimit Max:", 3,
			-10000, 10000, &integralWindupLimitMax
	);
	ConfigScreenHelpers::CreateSettingRow(
			settingsWidgetsContainer, "Int. Wind.\nLimit Min:", 4,
			-10000, 10000, &integralWindupLimitMin
	);
}

/**
 * @brief                        Creates the buttons that are used for navigation.
 *
 * @param  ButtonLabelTextStyle  The style that will be applied to the labels of large buttons.
*/
void ConfigPIDControlPart1::navigationButtonsBuilder(lv_style_t* ButtonLabelTextStyle)
{
	lv_obj_t* toPreviousConfigScreenButton = lv_button_create(rootScreenContainer);
	lv_obj_add_event_cb(toPreviousConfigScreenButton, toPreviousConfigScreenButtonPressedEventHandler, LV_EVENT_CLICKED, nullptr);
	lv_obj_set_size(toPreviousConfigScreenButton, 60, 60);
	lv_obj_set_grid_cell(toPreviousConfigScreenButton,
	                     LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 2, 1
	);

	lv_obj_t* toPreviousConfigScreenButtonText = lv_label_create(toPreviousConfigScreenButton);
	lv_label_set_text(toPreviousConfigScreenButtonText, LV_SYMBOL_PREV);
	lv_obj_add_style(toPreviousConfigScreenButtonText, ButtonLabelTextStyle, LV_PART_MAIN);
	lv_obj_align(toPreviousConfigScreenButtonText, LV_ALIGN_CENTER, 0, 0);

	lv_obj_t* returnToMainScreenButton = lv_button_create(rootScreenContainer);
	lv_obj_add_event_cb(returnToMainScreenButton, returnToMainScreenButtonPressedEventHandler, LV_EVENT_CLICKED, nullptr);
	lv_obj_set_size(returnToMainScreenButton, 60, 60);
	lv_obj_set_grid_cell(returnToMainScreenButton,
	                     LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 2, 1
	);

	lv_obj_t* returnToMainScreenButtonText = lv_label_create(returnToMainScreenButton);
	lv_label_set_text(returnToMainScreenButtonText, LV_SYMBOL_HOME);
	lv_obj_add_style(returnToMainScreenButtonText, ButtonLabelTextStyle, LV_PART_MAIN);
	lv_obj_align(returnToMainScreenButtonText, LV_ALIGN_CENTER, 0, 0);

	lv_obj_t* toNextConfigScreenButton = lv_button_create(rootScreenContainer);
	lv_obj_add_event_cb(toNextConfigScreenButton, toNextConfigScreenButtonPressedEventHandler, LV_EVENT_CLICKED, nullptr);
	lv_obj_set_size(toNextConfigScreenButton, 60, 60);
	lv_obj_set_grid_cell(toNextConfigScreenButton,
	                     LV_GRID_ALIGN_CENTER, 3, 1, LV_GRID_ALIGN_CENTER, 2, 1
	);

	lv_obj_t* toNextConfigScreenButtonText = lv_label_create(toNextConfigScreenButton);
	lv_label_set_text(toNextConfigScreenButtonText, LV_SYMBOL_NEXT);
	lv_obj_add_style(toNextConfigScreenButtonText, ButtonLabelTextStyle, LV_PART_MAIN);
	lv_obj_align(toNextConfigScreenButtonText, LV_ALIGN_CENTER, 0, 0);
}

/**
 * @brief         Event handler function that is invoked when the "previous screen" button is pressed.
 *
 * @param  Event  The data passed by the event caller. Unused in this case.
*/
void ConfigPIDControlPart1::toPreviousConfigScreenButtonPressedEventHandler(__attribute__((unused)) lv_event_t* Event)
{
	screenSwitchRequired = true;
	desiredScreen = Screens::ConfigPidControlPart2;
	resetAllCursorPositions();
}

/**
 * @brief         Event handler function that is invoked when the "return to main screen" button is pressed.
 *
 * @param  Event  The data passed by the event caller. Unused in this case.
*/
void ConfigPIDControlPart1::returnToMainScreenButtonPressedEventHandler(__attribute__((unused)) lv_event_t* Event)
{
	screenSwitchRequired = true;
	desiredScreen = Screens::StatusAkaMain;
	resetAllCursorPositions();
}

/**
 * @brief         Event handler function that is invoked when the "next screen" button is pressed.
 *
 * @param  Event  The data passed by the event caller. Unused in this case.
*/
void ConfigPIDControlPart1::toNextConfigScreenButtonPressedEventHandler(__attribute__((unused)) lv_event_t* Event)
{
	screenSwitchRequired = true;
	desiredScreen = Screens::ConfigPidControlPart2;
	resetAllCursorPositions();
}

/**
 * @brief  Resets the cursor positions of all SpinBoxes.
*/
void ConfigPIDControlPart1::resetAllCursorPositions()
{
	lv_spinbox_set_cursor_pos(loopTimeStep.Spinbox, 0);
	lv_spinbox_set_cursor_pos(proportionalGain.Spinbox, 0);
	lv_spinbox_set_cursor_pos(integralGain.Spinbox, 0);
	lv_spinbox_set_cursor_pos(integralWindupLimitMax.Spinbox, 0);
	lv_spinbox_set_cursor_pos(integralWindupLimitMin.Spinbox, 0);
}

/**
 * @brief  Used to instruct given functions to use their debug code.
 *
 * @note   Uncomment the booleans that represent the functions you want to debug.
*/
void ConfigPIDControlPart1::enableDebugTriggers()
{
}

#pragma clang diagnostic pop