// This code is provided under the MPL v2.0 license. Copyright 2025 Xavier du Hecquet de Rauville
// Details may be found in License.txt
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
//  This Source Code Form is "Incompatible With Secondary Licenses", as
//  defined by the Mozilla Public License, v. 2.0.

#ifndef ENGINEERING_PROJECT_SERIALHANDLER_H
#define ENGINEERING_PROJECT_SERIALHANDLER_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

/**
 * @brief  Provides an intermediary between the Serial port and rest of the code.
 *
 * Arduino's Serial class uses buffers of 64 bytes for sending and receiving data.
 * Any data written to the Serial port is placed in the output buffer, where it is then sent out the Serial port
 * when it is possible to do so. However, trying to write data that doesn't fit into the buffer's currently available space
 * will cause the write command to stall until all the requested data is either in the buffer or sent out the Serial port.
 *
 * Additionally, the functions for writing data to the Serial port also don't consider whether there is anything receiving the
 * serial data, or even anything plugged in. Hence, messages can get lost as a result.
 *
 * This class attempts to solve both of these problems by providing its own buffer for output data. Data is stored in the
 * buffer until the Serial port is ready to receive it, and until there is a device ready to receive it.
 */
class SerialHandler
{
public:
	static void Init(bool IsUsbConnected);
	static std::vector<uint8_t> ReadAllData();
	static std::string ReadAllDataAsString();
	static void SetState(bool Enable);
	static void SafeWriteLn(const std::string& TextOut, bool ShouldWrite);
	static void TryWriteBufferToSerial();

private:
#define BUFFER_SIZE 2048

	static bool BufferOverflowHappened;
	static bool receiverConnectionDelayHasElapsed;
	static bool IsSerialEnabled;
	static uint16_t unsentDataInBufferLastByte;
	static uint16_t unsentDataInBufferFirstByte;
	static uint32_t millisValueAtStartOfReceiverConnectionDelay;
	static std::array<uint8_t, BUFFER_SIZE> outputBuffer;

	static void enableDebugTriggers();
	static std::string getTimeSinceMicrocontrollerStartup();
	static bool writeBufferToSerial();
};


#endif //ENGINEERING_PROJECT_SERIALHANDLER_H
