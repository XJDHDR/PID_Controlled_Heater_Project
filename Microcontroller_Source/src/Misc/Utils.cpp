// This code is provided under the MPL v2.0 license. Copyright 2025 Xavier du Hecquet de Rauville
// Details may be found in License.txt
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
//  This Source Code Form is "Incompatible With Secondary Licenses", as
//  defined by the Mozilla Public License, v. 2.0.


#include "Utils.h"

#include <Arduino.h>
#include <thread>


/**
 * @brief             Convert a formatted string into a plain string with the arguments inserted.
 *
 * @param  FormatStr  The formatted string.
 * @param  ...        All of the values to insert into the formatted string.
 *
 * @return            The string with the data added in.
*/
std::string Utils::StringFormat(const std::string FormatStr, ...)
{
	int final_n, n = ((int)FormatStr.size()) * 2; /* Reserve two times as much as the length of the FormatStr */
	std::unique_ptr<char[]> formatted;
	va_list ap;

	while (true)
	{
		formatted.reset(new char[n]); /* Wrap the plain char array into the unique_ptr */
		strcpy(&formatted[0], FormatStr.c_str());
		va_start(ap, FormatStr);
		final_n = vsnprintf(&formatted[0], n, FormatStr.c_str(), ap);
		va_end(ap);

		if (final_n < 0 || final_n >= n)
		{
			n += abs(final_n - n + 1);
		}
		else
		{
			break;
		}
	}

	return std::string(formatted.get());
}

/**
 * @brief              Calculate a CRC-8 checksum for a byte.
 *
 * @param  InitialCrc  The CRC value to start with.
 * @param  byte        The byte to calculate the CRC for.
 *
 * @return             The calculated CRC value.
*/
uint8_t Utils::CalcCrc8(const uint8_t InitialCrc, uint8_t byte)	// CCITT-CRC-8 Polygon x8+x4+x3+x2+1
{
	uint8_t finalCrc = InitialCrc;

	for(uint8_t count = 0; count < 8; ++count)
	{
		if (((finalCrc & 0x01) ^ (byte & 0x01)) != 0)
		{
			finalCrc ^= 0x70;
			finalCrc >>= 1;
			finalCrc |= 0x80;
		}
		else
		{
			finalCrc >>= 1;
			finalCrc &= 0x7F;
		}
		byte >>= 1;
	}
	return finalCrc;
}

/**
 * @brief            Puts the microcontroller into an error state.
 *
 * @param  ErrorMsg  The message to print to the serial port.
*/
[[noreturn]]
void Utils::ErrorState(const std::string& ErrorMsg)
{
	const char* errorMsg = ErrorMsg.c_str();
	while (true)
	{
		Serial.println(errorMsg);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}
