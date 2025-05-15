// This code is provided under the MPL v2.0 license. Copyright 2025 Xavier du Hecquet de Rauville
// Details may be found in License.txt
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
//  This Source Code Form is "Incompatible With Secondary Licenses", as
//  defined by the Mozilla Public License, v. 2.0.


#include "StatusAkaMain.h"

#include <cstdio>
#include <tuple>
#include <Arduino.h>

#include "Display/LvglHelpers/LvglHelpers.h"
#include "Misc/SerialHandler.h"
#include "Misc/Utils.h"


#define DATA_STRING_BUFFER_MAX_SIZE 8
#define TIME_BETWEEN_ERROR_MESSAGE_UPDATES_MS   (3 * 1000)


bool StatusAkaMain::debug_GetTargetTemperatureChangeDesiredByUser = false;
bool StatusAkaMain::debug_onOffButtonEventHandler = false;
bool StatusAkaMain::debug_SetCurrentTemperature = false;
bool StatusAkaMain::debug_SetCurrentTargetTemperature = false;

bool StatusAkaMain::screenSwitchRequired = false;
Screens StatusAkaMain::desiredScreen = Invalid;

bool StatusAkaMain::currentDisplayedPiControllerActiveIndication = false;
bool StatusAkaMain::currentOnOffButtonSwitchedOffState = false;

uint32_t StatusAkaMain::millisValueAtLastErrorMessageUpdate = 0;

float StatusAkaMain::currentTemperatureDegCent = -1000.0;
float StatusAkaMain::targetTemperatureChangeDesiredByUser = 0.0;

uint8_t StatusAkaMain::allErrorConditionsPresent = NoErrors;
StatusAkaMain::ErrorMessages StatusAkaMain::currentDisplayedErrorMessage = NoErrors;

std::array<int32_t, 5> StatusAkaMain::rootScreenContainerColumns = {8, LV_GRID_FR(1), LV_GRID_FR(1), 8, LV_GRID_TEMPLATE_LAST};
std::array<int32_t, 6> StatusAkaMain::rootScreenContainerRows = {4, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
std::array<int32_t, 5> StatusAkaMain::outputWidgetsContainerRows = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
std::array<int32_t, 5> StatusAkaMain::temperatureWidgetsContainerRows = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
std::array<int32_t, 3> StatusAkaMain::widgetsContainerColumns = {158, 50, LV_GRID_TEMPLATE_LAST};

char StatusAkaMain::currentTemperatureText[DATA_STRING_BUFFER_MAX_SIZE];
char StatusAkaMain::targetTemperatureText[DATA_STRING_BUFFER_MAX_SIZE];
char StatusAkaMain::currentFanRpmText[DATA_STRING_BUFFER_MAX_SIZE];
char StatusAkaMain::currentFanDutyCycleText[DATA_STRING_BUFFER_MAX_SIZE];
char StatusAkaMain::currentHeaterDutyCycleText[DATA_STRING_BUFFER_MAX_SIZE];

lv_obj_t* StatusAkaMain::rootScreenContainer;
lv_obj_t* StatusAkaMain::currentTemperatureValueTextLabel;
lv_obj_t* StatusAkaMain::targetTemperatureValueTextLabel;
lv_obj_t* StatusAkaMain::currentPiControllerStatusValueTextLabel;
lv_obj_t* StatusAkaMain::currentFanRpmValueTextLabel;
lv_obj_t* StatusAkaMain::currentFanOutputValueTextLabel;
lv_obj_t* StatusAkaMain::currentHeaterOutputValueTextLabel;
lv_obj_t* StatusAkaMain::onOffButton;

lv_obj_t* StatusAkaMain::errorMessagesLabel;


void StatusAkaMain::Init(lv_obj_t* TargetScreen, lv_style_t* ButtonLabelTextStyle, float TargetTemperature)
{
	enableDebugTriggers();

	std::ignore = snprintf(targetTemperatureText, DATA_STRING_BUFFER_MAX_SIZE, "%0.1f", TargetTemperature);
	const int32_t screenHeight = lv_obj_get_height(TargetScreen);
	const int32_t screenWidth = lv_obj_get_width(TargetScreen);

	rootScreenContainer = LvglHelpers::CreateWidgetContainer(
		TargetScreen, LV_OPA_0, 0, true, screenWidth, screenHeight,
		rootScreenContainerColumns.data(), rootScreenContainerRows.data(),
		false, 0, 0, 0, 0
	);
	Hide();

	const int32_t widgetsContainerWidth = screenWidth - rootScreenContainerColumns[0] - rootScreenContainerColumns.rbegin()[1];
	buildTemperatureUi(ButtonLabelTextStyle, widgetsContainerWidth);
	buildOutputUi(widgetsContainerWidth);

	onOffButton = LvglHelpers::CreateTextLabelButton(
			rootScreenContainer, ButtonLabelTextStyle,
			onOffButtonEventHandler, LV_EVENT_VALUE_CHANGED, nullptr,
			80, 60, 1, 1, 3, 1, LV_GRID_ALIGN_CENTER,
			LV_SYMBOL_POWER, true, true
	);
	lv_obj_set_state(onOffButton, LV_STATE_CHECKED, true);
	currentOnOffButtonSwitchedOffState = lv_obj_has_state(onOffButton, LV_STATE_CHECKED);

	std::ignore = LvglHelpers::CreateTextLabelButton(
			rootScreenContainer, ButtonLabelTextStyle,
			configButtonEventHandler, LV_EVENT_CLICKED, nullptr,
			80, 60, 2, 1, 3, 1, LV_GRID_ALIGN_CENTER,
			LV_SYMBOL_SETTINGS, true, false
	);

	lv_obj_t* errorMessagesLabelContainer = LvglHelpers::CreateWidgetContainer(
			rootScreenContainer, LV_OPA_100, 4, true, screenWidth, LV_SIZE_CONTENT,
			nullptr, nullptr,
			true, 0, 4, 4, 1
	);
	lv_obj_set_scrollbar_mode(errorMessagesLabelContainer, LV_SCROLLBAR_MODE_OFF);

	errorMessagesLabel = LvglHelpers::CreateTextLabel(
			errorMessagesLabelContainer, currentTemperatureText,
			false, LV_GRID_ALIGN_END, 0, 0, 0, 0
	);
}

void StatusAkaMain::UpdateErrorMessage()
{
	if ((millis() - millisValueAtLastErrorMessageUpdate) < TIME_BETWEEN_ERROR_MESSAGE_UPDATES_MS)
	{
		return;
	}
	millisValueAtLastErrorMessageUpdate = millis();

	if (currentDisplayedErrorMessage == allErrorConditionsPresent)
	{
		return;
	}

	// If there is only a single error condition present, we just need to set the message to this one and nothing else.
	if (changeErrorMessage(allErrorConditionsPresent))
	{
		return;
	}

	// Otherwise, cycle to the next error message that needs to be shown.
	uint8_t nextErrorMessage = currentDisplayedErrorMessage;
	while (true)
	{
		nextErrorMessage <<= 1;
		if (nextErrorMessage > 0b100)
		{
			nextErrorMessage = 0b001;
		}

		if (allErrorConditionsPresent & nextErrorMessage)
		{
			std::ignore = changeErrorMessage(nextErrorMessage);
			return;
		}
	}
}

Screens StatusAkaMain::IsScreenSwitchRequired()
{
	if (!screenSwitchRequired)
	{
		return Screens::Invalid;
	}

	return desiredScreen;
}

void StatusAkaMain::Hide()
{
	lv_obj_add_flag(rootScreenContainer, LV_OBJ_FLAG_HIDDEN);
	screenSwitchRequired = false;
	desiredScreen = Screens::Invalid;
}

void StatusAkaMain::Show()
{
	lv_obj_remove_flag(rootScreenContainer, LV_OBJ_FLAG_HIDDEN);
	screenSwitchRequired = false;
	desiredScreen = Screens::StatusAkaMain;
}

float StatusAkaMain::GetTargetTemperatureChangeDesiredByUser()
{
	const float desiredTemperatureUnitChangeAmount = targetTemperatureChangeDesiredByUser;
	targetTemperatureChangeDesiredByUser = 0.0;

	if (debug_GetTargetTemperatureChangeDesiredByUser && ((desiredTemperatureUnitChangeAmount >= 0.2) || (desiredTemperatureUnitChangeAmount <= -0.2)))
	{
		std::string newTemperatureMsg = Utils::StringFormat("User wants to change the temperature: %0.1f", desiredTemperatureUnitChangeAmount);
		SerialHandler::SafeWriteLn(newTemperatureMsg, true);
	}

	return desiredTemperatureUnitChangeAmount;
}

bool StatusAkaMain::IsOnOffButtonInOffState()
{
	return currentOnOffButtonSwitchedOffState;
}

void StatusAkaMain::SetCurrentTemperature(const float Temperature)
{
	const float differenceBetweenNewAndCurrentTemperature = Temperature - currentTemperatureDegCent;
	if ((differenceBetweenNewAndCurrentTemperature <= 0.01) && (differenceBetweenNewAndCurrentTemperature >= -0.01))
	{
		// Not enough difference between the old and new numbers to justify a screen update.
		return;
	}

	currentTemperatureDegCent = Temperature;
	std::ignore = snprintf(currentTemperatureText, DATA_STRING_BUFFER_MAX_SIZE, "%0.1f", Temperature);
	lv_label_set_text_static(currentTemperatureValueTextLabel, nullptr);

	if (debug_SetCurrentTemperature)
	{
		std::string newTemperatureMsg = Utils::StringFormat("New temperature set: %0.1f", Temperature);
		SerialHandler::SafeWriteLn(newTemperatureMsg, true);
	}
}

void StatusAkaMain::SetCurrentTargetTemperature(const float Temperature)
{
	std::ignore = snprintf(targetTemperatureText, DATA_STRING_BUFFER_MAX_SIZE, "%0.1f", Temperature);
	lv_label_set_text_static(targetTemperatureValueTextLabel, nullptr);

	if (debug_SetCurrentTargetTemperature)
	{
		std::string newTemperatureMsg = Utils::StringFormat("New target temperature set: %0.1f", Temperature);
		SerialHandler::SafeWriteLn(newTemperatureMsg, true);
	}
}

void StatusAkaMain::SetPiControllerStatusIndicator(bool IsActive)
{
	if (currentDisplayedPiControllerActiveIndication == IsActive)
	{
		return;
	}

	if (IsActive)
	{
		lv_label_set_text(currentPiControllerStatusValueTextLabel, "On");
		currentDisplayedPiControllerActiveIndication = true;
		return;
	}

	lv_label_set_text(currentPiControllerStatusValueTextLabel, "Off");
	currentDisplayedPiControllerActiveIndication = false;
}

void StatusAkaMain::SetCurrentFanRpm(const bool IsSwitchedOn, const uint32_t Rpm)
{
	if (IsSwitchedOn)
	{
		std::ignore = snprintf(currentFanRpmText, DATA_STRING_BUFFER_MAX_SIZE, "%i", Rpm);
	}
	else
	{
		std::ignore = snprintf(currentFanRpmText, DATA_STRING_BUFFER_MAX_SIZE, "Off");
	}

	lv_label_set_text_static(currentFanRpmValueTextLabel, nullptr);
}

void StatusAkaMain::SetCurrentDutyCycles(const float FanDutyCycle, const float HeaterDutyCycle)
{
	std::ignore = snprintf(currentFanDutyCycleText, DATA_STRING_BUFFER_MAX_SIZE, "%0.0f", FanDutyCycle);
	lv_label_set_text_static(currentFanOutputValueTextLabel, nullptr);
	std::ignore = snprintf(currentHeaterDutyCycleText, DATA_STRING_BUFFER_MAX_SIZE, "%0.0f", HeaterDutyCycle);
	lv_label_set_text_static(currentHeaterOutputValueTextLabel, nullptr);
}

void StatusAkaMain::AddErrorCondition(StatusAkaMain::ErrorMessages NewError)
{
	allErrorConditionsPresent |= NewError;
}

void StatusAkaMain::RemoveErrorCondition(StatusAkaMain::ErrorMessages OutdatedError)
{
	allErrorConditionsPresent &= ~OutdatedError;
}

void StatusAkaMain::buildTemperatureUi(lv_style_t* ButtonLabelTextStyle, const int32_t WidgetsContainerWidth)
{
	lv_obj_t* temperatureWidgetsContainer = LvglHelpers::CreateWidgetContainer(
			rootScreenContainer, LV_OPA_100, 6, false, WidgetsContainerWidth, LV_SIZE_CONTENT,
			widgetsContainerColumns.data(), temperatureWidgetsContainerRows.data(),
			true, 1, 2, 1, 1
	);

	std::ignore = LvglHelpers::CreateTextLabel(temperatureWidgetsContainer, "Temperature (°C)",
		true, LV_GRID_ALIGN_CENTER, 0, 2, 0, 1
	);

	std::ignore = LvglHelpers::CreateTextLabel(
			temperatureWidgetsContainer, "Current:",
			true, LV_GRID_ALIGN_START, 0, 1, 1, 1
	);

	currentTemperatureValueTextLabel = LvglHelpers::CreateTextLabel(
			temperatureWidgetsContainer, currentTemperatureText,
			true, LV_GRID_ALIGN_END, 1, 1, 1, 1
	);

	std::ignore = LvglHelpers::CreateTextLabel(
			temperatureWidgetsContainer, "Target:",
			true, LV_GRID_ALIGN_START, 0, 1, 2, 1
	);

	targetTemperatureValueTextLabel = LvglHelpers::CreateTextLabel(
			temperatureWidgetsContainer, targetTemperatureText,
			true, LV_GRID_ALIGN_END, 1, 1, 2, 1
	);

	std::ignore = LvglHelpers::CreateTextLabelButton(
			temperatureWidgetsContainer, ButtonLabelTextStyle,
			targetTemperatureIncrementButtonEventHandler, LV_EVENT_CLICKED, nullptr,
			90, 40, 0, 2, 3, 1, LV_GRID_ALIGN_START,
			LV_SYMBOL_PLUS, false, false
	);

	std::ignore = LvglHelpers::CreateTextLabelButton(
			temperatureWidgetsContainer, ButtonLabelTextStyle,
			targetTemperatureDecrementButtonEventHandler, LV_EVENT_CLICKED, nullptr,
			90, 40, 0, 2, 3, 1, LV_GRID_ALIGN_END,
			LV_SYMBOL_MINUS, false, false
	);
}

void StatusAkaMain::buildOutputUi(int32_t WidgetsContainerWidth)
{
	lv_obj_t* outputWidgetsContainer = LvglHelpers::CreateWidgetContainer(
			rootScreenContainer, LV_OPA_100, 6, false, WidgetsContainerWidth, LV_SIZE_CONTENT,
			widgetsContainerColumns.data(), outputWidgetsContainerRows.data(),
			true, 1, 2, 2, 1
	);

	std::ignore = LvglHelpers::CreateTextLabel(
			outputWidgetsContainer, "PI Controller status:",
			true, LV_GRID_ALIGN_START, 0, 1, 0, 1
	);

	currentPiControllerStatusValueTextLabel = LvglHelpers::CreateTextLabel(
			outputWidgetsContainer, "Off",
			true, LV_GRID_ALIGN_END, 1, 1, 0, 1
	);

	std::ignore = LvglHelpers::CreateTextLabel(
			outputWidgetsContainer, "Fan speed (RPM):",
			true, LV_GRID_ALIGN_START, 0, 1, 1, 1
	);

	currentFanRpmValueTextLabel = LvglHelpers::CreateTextLabel(
			outputWidgetsContainer, currentFanRpmText,
			true, LV_GRID_ALIGN_END, 1, 1, 1, 1
	);

	std::ignore = LvglHelpers::CreateTextLabel(
			outputWidgetsContainer, "Fan output (%):",
			true, LV_GRID_ALIGN_START, 0, 1, 2, 1
	);

	currentFanOutputValueTextLabel = LvglHelpers::CreateTextLabel(
			outputWidgetsContainer, currentFanDutyCycleText,
			true, LV_GRID_ALIGN_END, 1, 1, 2, 1
	);

	std::ignore = LvglHelpers::CreateTextLabel(
			outputWidgetsContainer, "Heater output (%):",
			true, LV_GRID_ALIGN_START, 0, 1, 3, 1
	);

	currentHeaterOutputValueTextLabel = LvglHelpers::CreateTextLabel(
			outputWidgetsContainer, currentHeaterDutyCycleText,
			true, LV_GRID_ALIGN_END, 1, 1, 3, 1
	);
}

void StatusAkaMain::configButtonEventHandler(__attribute__((unused)) lv_event_t* event)
{
	screenSwitchRequired = true;
	desiredScreen = Screens::ConfigPidControlPart1;
}

void StatusAkaMain::onOffButtonEventHandler(__attribute__((unused)) lv_event_t* event)
{
	currentOnOffButtonSwitchedOffState = lv_obj_has_state(onOffButton, LV_STATE_CHECKED);

	if (debug_onOffButtonEventHandler)
	{
		std::string newOnOffButtonStateMsg = Utils::StringFormat("On/Off button changed state: %s", currentOnOffButtonSwitchedOffState ? "Off" : "On");
		SerialHandler::SafeWriteLn(newOnOffButtonStateMsg, true);
	}
}

void StatusAkaMain::targetTemperatureDecrementButtonEventHandler(__attribute__((unused)) lv_event_t* event)
{
	targetTemperatureChangeDesiredByUser -= 0.5;
}

void StatusAkaMain::targetTemperatureIncrementButtonEventHandler(__attribute__((unused)) lv_event_t* event)
{
	targetTemperatureChangeDesiredByUser += 0.5;
}

bool StatusAkaMain::changeErrorMessage(uint8_t NewErrorCondition)
{
	switch (NewErrorCondition)
	{
		case NoErrors:
		{
			lv_label_set_text(errorMessagesLabel, "");
			currentDisplayedErrorMessage = NoErrors;
			return true;
		}

		case FanStuck:
		{
			lv_label_set_text(errorMessagesLabel, "Fan stuck or unplugged");
			currentDisplayedErrorMessage = FanStuck;
			return true;
		}

		case ThermoResistorShortCircuit:
		{
			lv_label_set_text(errorMessagesLabel, "Temp probe shorted");
			currentDisplayedErrorMessage = ThermoResistorShortCircuit;
			return true;
		}

		case ThermoResistorUnplugged:
		{
			lv_label_set_text(errorMessagesLabel, "Temp probe unplugged");
			currentDisplayedErrorMessage = ThermoResistorUnplugged;
			return true;
		}

		default:
		{
			return false;
		}
	}
}

void StatusAkaMain::enableDebugTriggers()
{
//	debug_Update = true;
//	debug_GetTargetTemperatureChangeDesiredByUser = true;
//	debug_onOffButtonEventHandler = true;
//	debug_SetCurrentTemperature = true;
//	debug_SetCurrentTargetTemperature = true;
//	debug_CheckForLvglUpdate = true;
}
