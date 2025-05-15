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


#include "Display.h"

#include <lvgl.h>       // Font compilation doesn't work without this, even though Display.h should already be including it.

#include "Display/Screens/StatusAkaMain.h"
#include "Display/Screens/ConfigPIDControlPart1.h"
#include "Display/Screens/ConfigPIDControlPart2.h"
#include "Misc/SerialHandler.h"
#include "Misc/Utils.h"
#include "Backlight.h"
#include "Touch.h"


#define LVGL_COLOUR_FORMAT_SIZE     (2 * 3)     // 3 colours, 2 bytes per colour
// Sadly, the ESP32-C3 doesn't have enough RAM for a screen-sized buffer, so make it 1/5th of the screen.
#define LVGL_BUFFER_SIZE            (DISPLAY_WIDTH_PX * DISPLAY_HEIGHT_PX * LVGL_COLOUR_FORMAT_SIZE / 5)


bool Display::debug_Update = false;
bool Display::debug_CheckForLvglUpdate = false;

uint32_t Display::millisValueAtLastLvglUpdate = 0;
uint32_t Display::timeUntilNextLvglUpdateMs = 0;

uint8_t Display::displayBuffer[LVGL_BUFFER_SIZE];

Screens Display::currentScreen = Screens::Invalid;

LovyanGfxConfig Display::displayDriver;
lv_display_t* Display::display;
lv_indev_t* Display::touchscreen;

lv_style_t Display::buttonLabelTextStyle;   // Do NOT declare this as a pointer. Otherwise, firmware encounters exceptions.


void Display::Init(float TargetTemperature, PIDControllerInitData ConfigData)
{
	enableDebugTriggers();

	displayDriver.init();
	displayDriver.setRotation(2);       // Screen was installed upside down on its PCB, so rotate render by 180°

	Backlight::Init();

	lv_init();
	lv_tick_set_cb(tickCounter);

	display = lv_display_create(DISPLAY_WIDTH_PX, DISPLAY_HEIGHT_PX);
	lv_display_set_rotation(display, LV_DISPLAY_ROTATION_0);
	lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
	lv_display_set_buffers(display, displayBuffer, nullptr, LVGL_BUFFER_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);
	lv_display_set_flush_cb(display, flushDisplay);

	Touch::Init(DISPLAY_WIDTH_PX, DISPLAY_HEIGHT_PX);
	touchscreen = lv_indev_create();
	lv_indev_set_type(touchscreen, LV_INDEV_TYPE_POINTER);   // A touchscreen is a pointer-like device.
	lv_indev_set_read_cb(touchscreen, getTouchData);

	lv_style_init(&buttonLabelTextStyle);
	lv_style_set_text_font(&buttonLabelTextStyle, &lv_font_montserrat_36);

	StatusAkaMain::Init(lv_screen_active(), &buttonLabelTextStyle, TargetTemperature);
	ConfigPIDControlPart1::Init(lv_screen_active(), &buttonLabelTextStyle, ConfigData);
	ConfigPIDControlPart2::Init(lv_screen_active(), &buttonLabelTextStyle, ConfigData);
	StatusAkaMain::Show();
	currentScreen = Screens::StatusAkaMain;
}

void Display::Update()
{
	StatusAkaMain::UpdateErrorMessage();
	checkForScreenSwitchRequired();

	checkForLvglUpdate();
	Backlight::CheckForIdleTimeout();
}

void Display::GetAllChangedFloatSettings(std::vector<PIDFloatDataPacket>* ChangedFloatSettings)
{
	if (currentScreen != StatusAkaMain)
	{
		// Don't send updated settings while in the options menu.
		return;
	}

	ConfigPIDControlPart1::GetAllChangedFloatSettings(ChangedFloatSettings);
	ConfigPIDControlPart2::GetAllChangedFloatSettings(ChangedFloatSettings);
}

void Display::GetAllChangedIntSettings(std::vector<PIDIntDataPacket>* ChangedIntSettings)
{
	if (currentScreen != StatusAkaMain)
	{
		// Don't send updated settings while in the options menu.
		return;
	}

	ConfigPIDControlPart1::GetAllChangedIntSettings(ChangedIntSettings);
}

void Display::checkForLvglUpdate()
{
	if ((millis() - millisValueAtLastLvglUpdate) < timeUntilNextLvglUpdateMs)
	{
		return;
	}

	timeUntilNextLvglUpdateMs = lv_timer_handler();
	millisValueAtLastLvglUpdate = millis();

	if (debug_CheckForLvglUpdate)
	{
		std::string aboutToWaitMsg = Utils::StringFormat("LVGL updates paused for: %ums", timeUntilNextLvglUpdateMs);
		SerialHandler::SafeWriteLn(aboutToWaitMsg, true);
	}
}

void Display::checkForScreenSwitchRequired()
{
	switch (currentScreen)
	{
		case Invalid:
			return;

		case StatusAkaMain:
		{
			checkIfSwitchRequiredOnCurrentScreen(StatusAkaMain::IsScreenSwitchRequired, StatusAkaMain::Hide);
			return;
		}

		case ConfigPidControlPart1:
		{
			checkIfSwitchRequiredOnCurrentScreen(ConfigPIDControlPart1::IsScreenSwitchRequired, ConfigPIDControlPart1::Hide);
			return;
		}

		case ConfigPidControlPart2:
		{
			checkIfSwitchRequiredOnCurrentScreen(ConfigPIDControlPart2::IsScreenSwitchRequired, ConfigPIDControlPart2::Hide);
			return;
		}
	}
}

void Display::checkIfSwitchRequiredOnCurrentScreen(Screens (*currentScreenIsSwitchRequiredFunction)(), void (*currentScreenHideFunction)())
{
	const Screens desiredScreen = currentScreenIsSwitchRequiredFunction();
	if (desiredScreen == currentScreen)
	{
		return;
	}
	switch (desiredScreen)
	{
		case Invalid:
			return;

		case StatusAkaMain:
			StatusAkaMain::Show();
			break;

		case ConfigPidControlPart1:
			ConfigPIDControlPart1::Show();
			break;

		case ConfigPidControlPart2:
			ConfigPIDControlPart2::Show();
			break;
	}

	currentScreenHideFunction();
	currentScreen = desiredScreen;
}

void Display::getTouchData(lv_indev_t* Indev, lv_indev_data_t* Data)
{
	if (Indev == nullptr)
	{
		return;
	}

	if (!Touch::HasScreenBeenTouched())
	{
		Data->state = LV_INDEV_STATE_RELEASED;
		return;
	}

	const Touch::TouchPoint lastTouchData = Touch::GetLastTouchPoint();
	if (!lastTouchData.WasValid)
	{
		Data->state = LV_INDEV_STATE_RELEASED;
		return;
	}

	if (Backlight::IsTimedOut())
	{
		Backlight::SwitchOn();
		Data->state = LV_INDEV_STATE_RELEASED;
		return;
	}

	Data->point.x = lastTouchData.HorizCoord;
	Data->point.y = lastTouchData.VertCoord;
	Data->state = LV_INDEV_STATE_PRESSED;

	Backlight::ResetIdleTimeout();
}

void Display::flushDisplay(lv_display_t* TargetDisplay, const lv_area_t* CoordinatesForScreenUpdateArea, uint8_t* NewPixelColourBytes)
{
	int32_t x1 = CoordinatesForScreenUpdateArea->x1;
	int32_t y1 = CoordinatesForScreenUpdateArea->y1;
	int32_t x2 = CoordinatesForScreenUpdateArea->x2;
	int32_t y2 = CoordinatesForScreenUpdateArea->y2;

	int32_t width = x2 - x1 + 1;
	int32_t height = y2 - y1 + 1;

	uint16_t* color_buffer = (uint16_t*)NewPixelColourBytes;

	lv_draw_sw_rgb565_swap(color_buffer, width * height);

	displayDriver.setAddrWindow(x1, y1, width, height);
	displayDriver.pushPixels(color_buffer, width * height);

	lv_display_flush_ready(TargetDisplay);
}

uint32_t Display::tickCounter()
{
	return millis();
}

void Display::enableDebugTriggers()
{
//	debug_Update = true;
//	debug_GetTargetTemperatureChangeDesiredByUser = true;
//	debug_SetCurrentTemperature = true;
//	debug_SetCurrentTargetTemperature = true;
//	debug_CheckForLvglUpdate = true;
}

#pragma clang diagnostic pop
