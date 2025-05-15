// This code is provided under the MPL v2.0 license. Copyright 2025 Xavier du Hecquet de Rauville
// Details may be found in License.txt
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
//  This Source Code Form is "Incompatible With Secondary Licenses", as
//  defined by the Mozilla Public License, v. 2.0.


#include "Usb.h"

#include <Arduino.h>
#include <soc/usb_serial_jtag_reg.h>


uint32_t Usb::previousMillisValue = 0;
uint32_t Usb::previousUsbFrameNumber = 0;

const uint32_t* Usb::usbFrameNumberAddress = reinterpret_cast<uint32_t*>(USB_SERIAL_JTAG_FRAM_NUM_REG);

Usb::State Usb::previousUsbConnectedState = State::Uninitialised;

void Usb::Init()
{
	previousUsbFrameNumber = usbFrameNumberAddress[0];
	previousMillisValue = millis();
}

Usb::State Usb::IsUsbPluggedIn()
{
	const uint32_t currentUsbFrameNumber = usbFrameNumberAddress[0];

	if ((millis() - previousMillisValue) < 2)
	{
		return previousUsbConnectedState;
	}

	previousMillisValue = millis();

	if (currentUsbFrameNumber == previousUsbFrameNumber)
	{
		previousUsbConnectedState = State::Unplugged;
		return State::Unplugged;
	}

	previousUsbFrameNumber = currentUsbFrameNumber;
	previousUsbConnectedState = State::Plugged;
	return State::Plugged;
}
