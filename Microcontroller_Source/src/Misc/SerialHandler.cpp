// This code is provided under the MPL v2.0 license. Copyright 2025 Xavier du Hecquet de Rauville
// Details may be found in License.txt
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
//  This Source Code Form is "Incompatible With Secondary Licenses", as
//  defined by the Mozilla Public License, v. 2.0.


#include "SerialHandler.h"

#include <HWCDC.h>
#include <tuple>
#include <Arduino.h>


#define MAX_END_INDEX_VALUE                         (BUFFER_SIZE - 1)
#define MICROSECONDS_ALLOWED_FOR_WRITING_TO_SERIAL  50

#define CARRIAGE_RETURN 0x0d
#define LINE_FEED       0x0a
#define CRLF_LENGTH     2


const bool printMsgArrivalTime = false;
const uint16_t millisecondsInSecond = 1000;
const uint8_t secondsInMinute = 60;
const uint8_t minutesInHour = 60;
const uint8_t hoursInDays = 24;

bool SerialHandler::BufferOverflowHappened = false;
bool SerialHandler::receiverConnectionDelayHasElapsed = false;
bool SerialHandler::IsSerialEnabled = false;
uint16_t SerialHandler::unsentDataInBufferLastByte = 0;
uint16_t SerialHandler::unsentDataInBufferFirstByte = 0;
uint32_t SerialHandler::millisValueAtStartOfReceiverConnectionDelay = 0;
std::array<uint8_t, BUFFER_SIZE> SerialHandler::outputBuffer = {};


/**
 * @brief                  Initialises the SerialHandler class.
 *
 * @param  IsUsbConnected  True if a USB cable is connected. False otherwise.
*/
void SerialHandler::Init(const bool IsUsbConnected)
{
	enableDebugTriggers();

	SetState(IsUsbConnected);
}

/**
 * @brief          Sets whether or not serial communication is possible.
 *
 * @param  Enable  True if a USB cable is connected. False otherwise.
*/
void SerialHandler::SetState(const bool Enable)
{
	switch (Enable)
	{
		case true:
			if (!IsSerialEnabled)
			{
				Serial.begin(115200);
				IsSerialEnabled = true;
				millisValueAtStartOfReceiverConnectionDelay = millis();
			}
			return;

		case false:
			if (IsSerialEnabled)
			{
				Serial.end();
				IsSerialEnabled = false;
				receiverConnectionDelayHasElapsed = false;
			}
			return;
	}
}

/**
 * @brief   Reads all the bytes from the serial port in raw bytes.
 *
 * @return  A Vector containing the read data.
*/
std::vector<uint8_t> SerialHandler::ReadAllData()
{
	std::vector<uint8_t> result = {};

	const int32_t dataLength = Serial.available();
	if (dataLength <= 0)
	{
		return result;
	}

	result.reserve(dataLength);
	std::ignore = Serial.readBytes(result.data(), dataLength);

	return result;
}

/**
 * @brief   Reads all the bytes from the serial port as a string.
 *
 * @return  A string representing all the read data.
*/
std::string SerialHandler::ReadAllDataAsString()
{
	const int32_t dataLength = Serial.available();
	if (dataLength <= 0)
	{
		return "";
	}

	char serialData[dataLength];
	std::ignore = Serial.readBytes(serialData, dataLength);

	std::string result;
	result.assign(serialData, dataLength);

	return result;
}

/**
 * @brief               Write a line of text to the buffer.
 *
 * @param  TextOut      The text to add.
 * @param  ShouldWrite  True if the data should actually be added. False otherwise.
*/
void SerialHandler::SafeWriteLn(const std::string& TextOut, const bool ShouldWrite)
{
	if (!ShouldWrite || BufferOverflowHappened)
	{
		return;
	}

	const std::string msgToOutput = (printMsgArrivalTime) ?
		getTimeSinceMicrocontrollerStartup() + TextOut :
		TextOut;

	const size_t stringLength = msgToOutput.length();

	if (
		(unsentDataInBufferLastByte < unsentDataInBufferFirstByte) &&
		((unsentDataInBufferLastByte + stringLength + CRLF_LENGTH) > unsentDataInBufferFirstByte) ||
		((unsentDataInBufferLastByte + stringLength + CRLF_LENGTH) >= BUFFER_SIZE)
	)
	{
		BufferOverflowHappened = true;
		return;
	}

	size_t i = 0;
	while (true)
	{
		if (i >= stringLength)
		{
			outputBuffer[(unsentDataInBufferLastByte + i    ) & MAX_END_INDEX_VALUE] = LINE_FEED;
			outputBuffer[(unsentDataInBufferLastByte + i + 1) & MAX_END_INDEX_VALUE] = CARRIAGE_RETURN;
			unsentDataInBufferLastByte = (unsentDataInBufferLastByte + i + 1) & MAX_END_INDEX_VALUE;
			break;
		}

		const size_t targetIndex = (unsentDataInBufferLastByte + i) & MAX_END_INDEX_VALUE;
		outputBuffer[targetIndex] = msgToOutput[i];

		i++;
	}

	std::ignore = writeBufferToSerial();
}

/**
 * @brief  Attempts to write the contents of the buffer to the serial port.
*/
void SerialHandler::TryWriteBufferToSerial()
{
	if (!receiverConnectionDelayHasElapsed)
	{
		if ((millis() - millisValueAtStartOfReceiverConnectionDelay) <= 3000)
		{
			return;
		}
		receiverConnectionDelayHasElapsed = true;
	}

	const uint32_t initialMicrosValue = micros();

	while ((micros() - initialMicrosValue) <= MICROSECONDS_ALLOWED_FOR_WRITING_TO_SERIAL)
	{
		const bool wasWriteAttempted = writeBufferToSerial();
		if (!wasWriteAttempted)
		{
			return;
		}
	}
}

/**
 * @brief   Write data from the buffer into the serial port until it is full.
 *
 * @return  True if data was sent to the serial port. False otherwise.
*/
bool SerialHandler::writeBufferToSerial()
{
	if (unsentDataInBufferFirstByte == unsentDataInBufferLastByte)
	{
		return false;
	}

	if (!IsSerialEnabled)
	{
		return false;
	}

	if (BufferOverflowHappened)
	{
		Serial.println("Buffer overflow occurred\r\n");
		BufferOverflowHappened = false;
	}

	while (Serial.availableForWrite() > 0)
	{
		switch (outputBuffer[unsentDataInBufferFirstByte])
		{
			// Workaround for Serial.write not supporting CRs or LFs.
			case CARRIAGE_RETURN:
				Serial.print("\r");
				break;

			case LINE_FEED:
				Serial.print("\n");
				break;

			default:
				Serial.write(outputBuffer[unsentDataInBufferFirstByte]);
				break;
		}

		unsentDataInBufferFirstByte++;
		unsentDataInBufferFirstByte &= MAX_END_INDEX_VALUE;

		if (unsentDataInBufferFirstByte == unsentDataInBufferLastByte)
		{
			unsentDataInBufferFirstByte = 0;
			unsentDataInBufferLastByte = 0;
			break;
		}
	}

	return true;
}

/**
 * @brief   Get the amount of time elapsed since the microcontroller booted up.
 *
 * @return  A formatted string containing the time period.
*/
std::string SerialHandler::getTimeSinceMicrocontrollerStartup()
{
	char currentTimeMsg[17];
	const uint32_t currentMillisValue = millis();

	const uint16_t numberOfDays = currentMillisValue / (hoursInDays * minutesInHour * secondsInMinute * millisecondsInSecond);
	const uint32_t millisValueWithoutDays = currentMillisValue % (hoursInDays * minutesInHour * secondsInMinute * millisecondsInSecond);

	const uint8_t numberOfHours = millisValueWithoutDays / (minutesInHour * secondsInMinute * millisecondsInSecond);
	const uint32_t millisValueWithoutHours = millisValueWithoutDays % (minutesInHour * secondsInMinute * millisecondsInSecond);

	const uint8_t numberOfMinutes = millisValueWithoutHours / (secondsInMinute * millisecondsInSecond);
	const uint32_t millisValueWithoutMinutes = millisValueWithoutHours % (secondsInMinute * millisecondsInSecond);

	const uint8_t numberOfSeconds = millisValueWithoutMinutes / millisecondsInSecond;
	const uint16_t numberOfMilliseconds = millisValueWithoutMinutes % millisecondsInSecond;

	sprintf(currentTimeMsg, "[%02u:%02u:%02u:%02u:%03u] ", numberOfDays, numberOfHours, numberOfMinutes, numberOfSeconds, numberOfMilliseconds);

	return currentTimeMsg;
}

/**
 * @brief  Used to instruct given functions to use their debug code.
 *
 * @note   Uncomment the booleans that represent the functions you want to debug.
*/
void SerialHandler::enableDebugTriggers()
{
}
