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


#include "Touch.h"

#include <Wire.h>

#include "Misc/SerialHandler.h"
#include "Misc/Utils.h"


#define I2C_SCL 8
#define I2C_SDA 2
#define TS_I2C_ADDRESS 0x48

#define TOUCH_REG_COORD_X 0xC0
#define TOUCH_REG_COORD_Y 0xD0
#define TOUCH_REG_COORD_Z 0xE0

#define MINIMUM_TOUCH_PRESSURE 200

// TODO: Move these to a calibration function
const uint16_t smallestValidTouchCoordinate = 450;
const uint16_t largestValidTouchCoordinate = 3800;


bool Touch::debug_Init = false;
bool Touch::debug_HasScreenBeenTouched = false;
bool Touch::debug_GetLastTouchPoint = false;
bool Touch::debug_translateFromTouchToScreenCoordinate = false;

int16_t Touch::screenHeight = 0;
int16_t Touch::screenWidth = 0;


/**
 * @brief                Initialises the Touchscreen class.
 *
 * @param  ScreenWidth   The display's width.
 * @param  ScreenHeight  The display's height.
 *
 * @return               True if the initialisation was successful. False otherwise.
*/
bool Touch::Init(int16_t ScreenWidth, int16_t ScreenHeight)
{
	enableDebugTriggers();

	screenWidth = ScreenWidth;
	if (debug_Init)
	{
		std::string screenWasTouchedMessage = Utils::StringFormat("Screen width set to: %i", screenWidth);
		SerialHandler::SafeWriteLn(screenWasTouchedMessage, true);
	}

	screenHeight = ScreenHeight;
	if (debug_Init)
	{
		std::string screenWasTouchedMessage = Utils::StringFormat("Screen height set to: %i", screenHeight);
		SerialHandler::SafeWriteLn(screenWasTouchedMessage, true);
	}

	if (!Wire.begin(I2C_SDA, I2C_SCL))
	{
		SerialHandler::SafeWriteLn("Wire begin failed.", debug_Init);
		return false;
	}

	SerialHandler::SafeWriteLn("Wire successfully initialised.", debug_Init);

	return true;
}

/**
 * @brief   Checks if the screen has been touched since the last time this function was called.
 *
 * @return  True if the screen was touched. False otherwise.
*/
bool Touch::HasScreenBeenTouched()
{
	const I2CData touchPressureZData = readData(TOUCH_REG_COORD_Z);

	if (!touchPressureZData.WasSuccessful)
	{
		return false;
	}

	if (touchPressureZData.Data < MINIMUM_TOUCH_PRESSURE)
	{
		if (debug_HasScreenBeenTouched)
		{
			std::string screenWasTouchedMessage = Utils::StringFormat("No touch event spotted with Z value of: %i", touchPressureZData.Data);
			SerialHandler::SafeWriteLn(screenWasTouchedMessage, true);
		}
		return false;
	}

	if (isTouchDataInvalid())
	{
		SerialHandler::SafeWriteLn("No touch event registered due to invalid touch data", debug_HasScreenBeenTouched);
		return false;
	}

	if (debug_HasScreenBeenTouched)
	{
		std::string screenWasTouchedMessage = Utils::StringFormat("Screen was touched with Z value of: %i", touchPressureZData.Data);
		SerialHandler::SafeWriteLn(screenWasTouchedMessage, true);
	}

	return true;
}

/**
 * @brief   Gets data for the last touch event.
 *
 * @return  Struct containing the coordinates of the last touch event.
*/
Touch::TouchPoint Touch::GetLastTouchPoint()
{
	TouchPoint result{};

	const I2CData horizData = readData(TOUCH_REG_COORD_X);
	if (!horizData.WasSuccessful || isTouchDataOutOfBounds(horizData.Data))
	{
		return result;
	}

	const I2CData vertData = readData(TOUCH_REG_COORD_Y);
	if (!vertData.WasSuccessful || isTouchDataOutOfBounds(vertData.Data))
	{
		return result;
	}

	const uint16_t touchedScreenCoordHoriz = translateFromTouchToScreenCoordinate(
		horizData.Data, smallestValidTouchCoordinate, largestValidTouchCoordinate, 0, screenWidth, false
	);
	if (debug_GetLastTouchPoint)
	{
		std::string readTouchDataMessage = Utils::StringFormat("Touch read horizontal data: %i", touchedScreenCoordHoriz);
		SerialHandler::SafeWriteLn(readTouchDataMessage, true);
	}

	const uint16_t touchedScreenCoordVert = translateFromTouchToScreenCoordinate(
		vertData.Data, smallestValidTouchCoordinate, largestValidTouchCoordinate, 0, screenHeight, true
	);
	if (debug_GetLastTouchPoint)
	{
		std::string readTouchDataMessage = Utils::StringFormat("Touch read vertical data: %i", touchedScreenCoordVert);
		SerialHandler::SafeWriteLn(readTouchDataMessage, true);
	}

	result.WasValid = true;
	result.HorizCoord = touchedScreenCoordHoriz;
	result.VertCoord = touchedScreenCoordVert;
	return result;
}

/**
 * @brief               Communicates with the touch IC and reads data from it.
 *
 * @param  CommandByte  The register to read data from.
 *
 * @return              Data concerning the I2C communication.
*/
Touch::I2CData Touch::readData(uint8_t CommandByte)
{
	I2CData i2CData{};

	std::array<uint8_t, 2> ReadData{};
	uint8_t i = 0;
	Wire.beginTransmission(TS_I2C_ADDRESS);
	Wire.write(CommandByte);
	const bool transmissionResult = Wire.endTransmission();

	if (transmissionResult != 0)
	{
		return i2CData;
	}

	const uint8_t bytesRead = Wire.requestFrom(TS_I2C_ADDRESS, 2);
	if (bytesRead != 2)
	{
		std::string readTouchDataMessage = Utils::StringFormat("I2C failed to read the correct number of bytes. Read: %i", bytesRead);
		SerialHandler::SafeWriteLn(readTouchDataMessage, true);
		return i2CData;
	}

	while (Wire.available())
		ReadData[i++] = Wire.read();

	i2CData.WasSuccessful = true;
	i2CData.Data = (ReadData[0] << 4) | (ReadData[1] >> 4);

	return i2CData;
}

/**
 * @brief   Determines if the last touch event was invalid.
 *
 * @return  True if the touch event was invalid. False otherwise.
*/
bool Touch::isTouchDataInvalid()
{
	// Sometimes, the screen provides some invalid touch registrations that have coordinates of (4095, 4095).
	// This function is used to filter these out.
	for (int i = 0; i < 2; ++i)
	{
		const uint8_t targetRegistry = (i == 0) ?
           TOUCH_REG_COORD_X :
           TOUCH_REG_COORD_Y;

		const I2CData touchPressureData = readData(targetRegistry);

		if (!touchPressureData.WasSuccessful)
		{
			return true;
		}

		if (isTouchDataOutOfBounds(touchPressureData.Data))
		{
			return true;
		}
	}

	return false;
}

/**
 * @brief                   Determines if the specified touch coordinate is out of bounds.
 *
 * @param  touchCoordinate  The touch data.
 *
 * @return                  True if the touch is out of bounds. False otherwise.
*/
bool Touch::isTouchDataOutOfBounds(uint16_t touchCoordinate)
{
	if ((touchCoordinate > 4000) || (touchCoordinate < 150))
	{
		return true;
	}

	return false;
}

/**
 * @brief                   Translates a given touch coordinate into the equivalent screen coordinate.
 *
 * @param  TouchCoordinate  The touch coordinate.
 * @param  TouchMin         The minimum value for the touch coordinate.
 * @param  TouchMax         The maximum value for the touch coordinate.
 * @param  ScreenMin        The minimum screen coordinate value.
 * @param  ScreenMax        The maximum screen coordinate value.
 * @param  IsFlipped        Whether or not this coordinate is inverted.
 *
 * @return                  The translated screen coordinate.
*/
uint16_t Touch::translateFromTouchToScreenCoordinate(
	const uint16_t TouchCoordinate, const uint16_t TouchMin, const uint16_t TouchMax,
	const uint16_t ScreenMin, const uint16_t ScreenMax, const bool IsFlipped
)
{
	const float calibratedTouchRange = static_cast<float>(TouchMax - TouchMin);
	const float calibratedTouchCoordinate = (IsFlipped) ?
        static_cast<float>(TouchMax - TouchCoordinate) / calibratedTouchRange :
		static_cast<float>(TouchCoordinate - TouchMin) / calibratedTouchRange;
	if (debug_translateFromTouchToScreenCoordinate)
	{
		std::string readTouchDataMessage = Utils::StringFormat("Calibrated touch coordinate: %f", calibratedTouchCoordinate);
		SerialHandler::SafeWriteLn(readTouchDataMessage, true);
	}

	const float screenRange = static_cast<float>(ScreenMax - ScreenMin);
	const float screenCoordinate = calibratedTouchCoordinate * screenRange;
	if (debug_translateFromTouchToScreenCoordinate)
	{
		std::string readTouchDataMessage = Utils::StringFormat("Screen coordinate: %f", screenCoordinate);
		SerialHandler::SafeWriteLn(readTouchDataMessage, true);
	}

	return static_cast<uint16_t>(screenCoordinate);
}

/**
 * @brief  Used to instruct given functions to use their debug code.
 *
 * @note   Uncomment the booleans that represent the functions you want to debug.
*/
void Touch::enableDebugTriggers()
{
//	debug_Init = true;
//	debug_HasScreenBeenTouched = true;
//	debug_GetLastTouchPoint = true;
//	debug_translateFromTouchToScreenCoordinate = true;
}

#pragma clang diagnostic pop
