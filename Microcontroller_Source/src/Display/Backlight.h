// This code is provided under the MPL v2.0 license. Copyright 2025 Xavier du Hecquet de Rauville
// Details may be found in License.txt
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
//  This Source Code Form is "Incompatible With Secondary Licenses", as
//  defined by the Mozilla Public License, v. 2.0.

#ifndef ENGINEERING_PROJECT_BACKLIGHT_H
#define ENGINEERING_PROJECT_BACKLIGHT_H

#include <cstdint>

/**
 * @brief  Contains the logic for managing the display's backlight.
*/
class Backlight
{
public:
	static void Init();
	static void CheckForIdleTimeout();
	static bool IsTimedOut();
	static void ResetIdleTimeout();
	static void SwitchOff();
	static void SwitchOn();


private:
	enum backlightState
	{
		Off,
		On
	};

	static bool wasScreenRecentlyWoken;
	static uint32_t millisValueAtLastScreenWake;
	static uint32_t millisValueAtStartOfTimeout;
	static backlightState currentState;
};

#endif //ENGINEERING_PROJECT_BACKLIGHT_H
