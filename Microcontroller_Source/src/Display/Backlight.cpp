// This code is provided under the MPL v2.0 license. Copyright 2025 Xavier du Hecquet de Rauville
// Details may be found in License.txt
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
//  This Source Code Form is "Incompatible With Secondary Licenses", as
//  defined by the Mozilla Public License, v. 2.0.


#include "Backlight.h"

#include <Arduino.h>


#define TFT_BACKLIGHT_PIN   1

#define BACKLIGHT_IDLE_TIMEOUT_LENGTH_MS    30000


bool Backlight::wasScreenRecentlyWoken = false;
uint32_t Backlight::millisValueAtLastScreenWake = 0;
uint32_t Backlight::millisValueAtStartOfTimeout = 0;
Backlight::backlightState Backlight::currentState = Off;


/**
 * @brief  Initialises the backlight class.
*/
void Backlight::Init()
{
	pinMode(TFT_BACKLIGHT_PIN, OUTPUT);
	SwitchOn();
}

/**
 * @brief  Checks if the idle timer has exceeded the timeout length, and turn the backlight off if so.
*/
void Backlight::CheckForIdleTimeout()
{
	if (wasScreenRecentlyWoken && ((millis() - millisValueAtLastScreenWake) > 500))
	{
		wasScreenRecentlyWoken = false;
	}

	if (currentState == Off)
	{
		return;
	}

	if ((millis() - millisValueAtStartOfTimeout) < BACKLIGHT_IDLE_TIMEOUT_LENGTH_MS)
	{
		return;
	}

	SwitchOff();
}

/**
 * @brief   Checks if the backlight is currently timed out.
 *
 * @return  True if the backlight is off, or has been recently woken up. False otherwise.
*/
bool Backlight::IsTimedOut()
{
	if (currentState == Off)
	{
		return true;
	}

	if (wasScreenRecentlyWoken && ((millis() - millisValueAtLastScreenWake) < 500))
	{
		return true;
	}

	return false;
}

/**
 * @brief  Resets the idle timer.
*/
void Backlight::ResetIdleTimeout()
{
	millisValueAtStartOfTimeout = millis();
}

/**
 * @brief  Switches off the backlight.
*/
void Backlight::SwitchOff()
{
	if (currentState == Off)
	{
		return;
	}

	digitalWrite(TFT_BACKLIGHT_PIN, HIGH);
	currentState = Off;
	wasScreenRecentlyWoken = false;
}

/**
 * @brief  Switches on the backlight.
*/
void Backlight::SwitchOn()
{
	if (currentState == On)
	{
		return;
	}

	digitalWrite(TFT_BACKLIGHT_PIN, LOW);
	currentState = On;

	ResetIdleTimeout();
	wasScreenRecentlyWoken = true;
	millisValueAtLastScreenWake = millis();
}
