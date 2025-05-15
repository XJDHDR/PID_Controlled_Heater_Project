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


#include "ConfigPIDControlPart2.h"

#include <tuple>

#include "Display/LvglHelpers/LvglHelpers.h"
#include "Misc/Utils.h"


bool ConfigPIDControlPart2::screenSwitchRequired = false;
Screens ConfigPIDControlPart2::desiredScreen = Screens::Invalid;

std::array<int32_t, 6> ConfigPIDControlPart2::rootScreenContainerColumns = {8, LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), 8, LV_GRID_TEMPLATE_LAST};
std::array<int32_t, 5> ConfigPIDControlPart2::rootScreenContainerRows = {8, LV_GRID_FR(1), LV_GRID_CONTENT, 8, LV_GRID_TEMPLATE_LAST};
std::array<int32_t, 5> ConfigPIDControlPart2::settingsWidgetsContainerColumns = {LV_GRID_FR(1), LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
std::array<int32_t, 6> ConfigPIDControlPart2::settingsWidgetsContainerRows = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};

ConfigScreenHelpers::FloatSpinboxData ConfigPIDControlPart2::derivativeGain = {};
ConfigScreenHelpers::FloatSpinboxData ConfigPIDControlPart2::derivativeTermLimitMax = {};
ConfigScreenHelpers::FloatSpinboxData ConfigPIDControlPart2::derivativeTermLimitMin = {};
ConfigScreenHelpers::FloatSpinboxData ConfigPIDControlPart2::outputMax = {};

lv_obj_t* ConfigPIDControlPart2::rootScreenContainer;


void ConfigPIDControlPart2::Init(lv_obj_t* TargetScreen, lv_style_t* ButtonLabelTextStyle, PIDControllerInitData ConfigData)
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

	derivativeGain.CurrentValue = ConfigData.DerivativeGain;
	derivativeGain.DecimalPosition = 2;
	derivativeTermLimitMax.CurrentValue = ConfigData.DerivativeTermMaxValue;
	derivativeTermLimitMax.DecimalPosition = 3;
	derivativeTermLimitMin.CurrentValue = ConfigData.DerivativeTermMinValue;
	derivativeTermLimitMin.DecimalPosition = 3;
	outputMax.CurrentValue = ConfigData.OutputMaxValue;
	outputMax.DecimalPosition = 3;

	const int32_t widgetsContainerWidth = screenWidth - rootScreenContainerColumns[0] - rootScreenContainerColumns.rbegin()[1];

	settingsConfigBuilder(widgetsContainerWidth);
	navigationButtonsBuilder(ButtonLabelTextStyle);
}

Screens ConfigPIDControlPart2::IsScreenSwitchRequired()
{
	if (!screenSwitchRequired)
	{
		return Screens::Invalid;
	}

	return desiredScreen;
}

void ConfigPIDControlPart2::Hide()
{
	lv_obj_add_flag(rootScreenContainer, LV_OBJ_FLAG_HIDDEN);
	screenSwitchRequired = false;
	desiredScreen = Screens::Invalid;
}

void ConfigPIDControlPart2::Show()
{
	lv_obj_remove_flag(rootScreenContainer, LV_OBJ_FLAG_HIDDEN);
	screenSwitchRequired = false;
	desiredScreen = Screens::ConfigPidControlPart2;
}

void ConfigPIDControlPart2::GetAllChangedFloatSettings(std::vector<PIDFloatDataPacket>* ChangedFloatSettings)
{
	if (derivativeGain.HasValueBeenChangedSinceLastCheck)
	{
		derivativeGain.HasValueBeenChangedSinceLastCheck = false;
		ChangedFloatSettings->push_back({DerivativeGain, derivativeGain.CurrentValue});
	}
	if (derivativeTermLimitMax.HasValueBeenChangedSinceLastCheck)
	{
		derivativeTermLimitMax.HasValueBeenChangedSinceLastCheck = false;
		ChangedFloatSettings->push_back({DerivativeTermMaxValue, derivativeTermLimitMax.CurrentValue});
	}
	if (derivativeTermLimitMin.HasValueBeenChangedSinceLastCheck)
	{
		derivativeTermLimitMin.HasValueBeenChangedSinceLastCheck = false;
		ChangedFloatSettings->push_back({DerivativeTermMinValue, derivativeTermLimitMin.CurrentValue});
	}
	if (outputMax.HasValueBeenChangedSinceLastCheck)
	{
		outputMax.HasValueBeenChangedSinceLastCheck = false;
		ChangedFloatSettings->push_back({OutputMaxValue, outputMax.CurrentValue});
	}
}

void ConfigPIDControlPart2::settingsConfigBuilder(const int32_t WidgetsContainerWidth)
{
	lv_obj_t* settingsWidgetsContainer = LvglHelpers::CreateWidgetContainer(
			rootScreenContainer, LV_OPA_100, 6, false, WidgetsContainerWidth, LV_SIZE_CONTENT,
			settingsWidgetsContainerColumns.data(), settingsWidgetsContainerRows.data(),
			true, 1, 3, 1, 1
	);

	ConfigScreenHelpers::CreateSettingRow(
			settingsWidgetsContainer, "Derivative\nGain:", 0,
			0, 10000, &derivativeGain
	);
	ConfigScreenHelpers::CreateSettingRow(
			settingsWidgetsContainer, "Deri. Term.\nLimit Max:", 1,
			-10000, 10000, &derivativeTermLimitMax
	);
	ConfigScreenHelpers::CreateSettingRow(
			settingsWidgetsContainer, "Deri. Term\nLimit Min:", 2,
			-10000, 10000, &derivativeTermLimitMin
	);
	ConfigScreenHelpers::CreateSettingRow(
			settingsWidgetsContainer, "Output\nMax:", 3,
			0, 10000, &outputMax
	);
}

void ConfigPIDControlPart2::navigationButtonsBuilder(lv_style_t* ButtonLabelTextStyle)
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

void ConfigPIDControlPart2::toPreviousConfigScreenButtonPressedEventHandler(__attribute__((unused)) lv_event_t* Event)
{
	screenSwitchRequired = true;
	desiredScreen = Screens::ConfigPidControlPart1;
	resetAllCursorPositions();
}

void ConfigPIDControlPart2::returnToMainScreenButtonPressedEventHandler(__attribute__((unused)) lv_event_t* Event)
{
	screenSwitchRequired = true;
	desiredScreen = Screens::StatusAkaMain;
	resetAllCursorPositions();
}

void ConfigPIDControlPart2::toNextConfigScreenButtonPressedEventHandler(__attribute__((unused)) lv_event_t* Event)
{
	screenSwitchRequired = true;
	desiredScreen = Screens::ConfigPidControlPart1;
	resetAllCursorPositions();
}

void ConfigPIDControlPart2::resetAllCursorPositions()
{
	lv_spinbox_set_cursor_pos(derivativeGain.Spinbox, 0);
	lv_spinbox_set_cursor_pos(derivativeTermLimitMax.Spinbox, 0);
	lv_spinbox_set_cursor_pos(derivativeTermLimitMin.Spinbox, 0);
	lv_spinbox_set_cursor_pos(outputMax.Spinbox, 0);
}

void ConfigPIDControlPart2::enableDebugTriggers()
{
}

#pragma clang diagnostic pop