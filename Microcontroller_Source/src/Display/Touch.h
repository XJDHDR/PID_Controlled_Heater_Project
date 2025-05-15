// This code is provided under the MPL v2.0 license. Copyright 2025 Xavier du Hecquet de Rauville
// Details may be found in License.txt
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
//  This Source Code Form is "Incompatible With Secondary Licenses", as
//  defined by the Mozilla Public License, v. 2.0.

#ifndef ENGINEERING_PROJECT_TOUCH_H
#define ENGINEERING_PROJECT_TOUCH_H

#include <cstdint>

class Touch
{
public:
	struct TouchPoint
	{
		bool WasValid;
		uint16_t HorizCoord;
		uint16_t VertCoord;
	};

    static bool Init(int16_t ScreenWidth, int16_t ScreenHeight);
	static bool HasScreenBeenTouched();
    static TouchPoint GetLastTouchPoint();

private:
	struct I2CData
	{
		bool WasSuccessful;
		uint16_t Data;
	};

	static bool debug_Init;
	static bool debug_HasScreenBeenTouched;
	static bool debug_GetLastTouchPoint;
	static bool debug_translateFromTouchToScreenCoordinate;

	static int16_t screenHeight;
	static int16_t screenWidth;

	static I2CData readData(uint8_t CommandByte);
	static bool isTouchDataInvalid();
	static bool isTouchDataOutOfBounds(uint16_t touchCoordinate);
	static uint16_t translateFromTouchToScreenCoordinate(
		uint16_t TouchCoordinate, uint16_t TouchMin, uint16_t TouchMax, uint16_t ScreenMin, uint16_t ScreenMax, bool IsFlipped
	);

	static void enableDebugTriggers();
};

#endif //ENGINEERING_PROJECT_TOUCH_H
