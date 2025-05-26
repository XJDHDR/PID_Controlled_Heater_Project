// This code is provided under the MPL v2.0 license. Copyright 2025 Xavier du Hecquet de Rauville
// Details may be found in License.txt
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
//  This Source Code Form is "Incompatible With Secondary Licenses", as
//  defined by the Mozilla Public License, v. 2.0.

#ifndef ENGINEERING_PROJECT_STATUSAKAMAIN_H
#define ENGINEERING_PROJECT_STATUSAKAMAIN_H

#include <lvgl.h>
#include <array>

#include "AllScreens.h"

/**
 * @brief  Contains the logic for the main screen.
*/
class StatusAkaMain
{
public:
	enum ErrorMessages
	{
		NoErrors                    = 0b000,
		FanStuck                    = 0b001,
		ThermoResistorShortCircuit  = 0b010,
		ThermoResistorUnplugged     = 0b100,
	};

	static void Init(lv_obj_t* TargetScreen, lv_style_t* ButtonLabelTextStyle, float TargetTemperature);
	static void UpdateErrorMessage();
	static Screens IsScreenSwitchRequired();
	static void Hide();
	static void Show();

	static float GetTargetTemperatureChangeDesiredByUser();
	static bool IsOnOffButtonInOffState();
	static void SetCurrentTemperature(float Temperature);
	static void SetCurrentTargetTemperature(float Temperature);
	static void SetPiControllerStatusIndicator(bool IsActive);
	static void SetCurrentFanRpm(bool IsSwitchedOn, uint32_t Rpm);
	static void SetCurrentDutyCycles(float FanDutyCycle, float HeaterDutyCycle);

	static void AddErrorCondition(ErrorMessages NewError);
	static void RemoveErrorCondition(ErrorMessages OutdatedError);


private:
	static bool debug_GetTargetTemperatureChangeDesiredByUser;
	static bool debug_onOffButtonEventHandler;
	static bool debug_SetCurrentTemperature;
	static bool debug_SetCurrentTargetTemperature;

	static bool screenSwitchRequired;
	static Screens desiredScreen;

	static bool currentDisplayedPiControllerActiveIndication;
	static bool currentOnOffButtonSwitchedOffState;

	static uint32_t millisValueAtLastErrorMessageUpdate;

	static float currentTemperatureDegCent;
	static float targetTemperatureChangeDesiredByUser;

	static uint8_t allErrorConditionsPresent;       // Can't define as ErrorMessages type. Otherwise, can't add or remove flags from it.
	static ErrorMessages currentDisplayedErrorMessage;

	static std::array<int32_t, 5> rootScreenContainerColumns;
	static std::array<int32_t, 6> rootScreenContainerRows;
	static std::array<int32_t, 5> outputWidgetsContainerRows;
	static std::array<int32_t, 5> temperatureWidgetsContainerRows;
	static std::array<int32_t, 3> widgetsContainerColumns;

	static char currentTemperatureText[];
	static char targetTemperatureText[];
	static char currentFanRpmText[];
	static char currentFanDutyCycleText[];
	static char currentHeaterDutyCycleText[];

	static lv_obj_t* rootScreenContainer;
	static lv_obj_t* currentTemperatureValueTextLabel;
	static lv_obj_t* targetTemperatureValueTextLabel;
	static lv_obj_t* currentPiControllerStatusValueTextLabel;
	static lv_obj_t* currentFanRpmValueTextLabel;
	static lv_obj_t* currentFanOutputValueTextLabel;
	static lv_obj_t* currentHeaterOutputValueTextLabel;
	static lv_obj_t* onOffButton;

	static lv_obj_t* errorMessagesLabel;


	static void buildTemperatureUi(lv_style_t* ButtonLabelTextStyle, int32_t WidgetsContainerWidth);
	static void buildOutputUi(int32_t WidgetsContainerWidth);
	static void configButtonEventHandler(__attribute__((unused)) lv_event_t* event);
	static void onOffButtonEventHandler(__attribute__((unused)) lv_event_t* event);
	static void targetTemperatureDecrementButtonEventHandler(__attribute__((unused)) lv_event_t* event);
	static void targetTemperatureIncrementButtonEventHandler(__attribute__((unused)) lv_event_t* event);
	static bool changeErrorMessage(uint8_t NewErrorCondition);

	static void enableDebugTriggers();
};

#endif //ENGINEERING_PROJECT_STATUSAKAMAIN_H
