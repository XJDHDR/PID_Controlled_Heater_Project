// This code is provided under the MPL v2.0 license. Copyright 2025 Xavier du Hecquet de Rauville
// Details may be found in License.txt
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
//  This Source Code Form is "Incompatible With Secondary Licenses", as
//  defined by the Mozilla Public License, v. 2.0.

#ifndef ENGINEERING_PROJECT_USB_H
#define ENGINEERING_PROJECT_USB_H

#include <cstdint>

class Usb
{
public:
	enum State
	{
		Plugged,
		Unplugged,
		Uninitialised
	};

private:
	static State previousUsbConnectedState;
	static uint32_t previousMillisValue;
	static uint32_t previousUsbFrameNumber;

	const static uint32_t* usbFrameNumberAddress;

public:
	static void Init();
	static State IsUsbPluggedIn();
};

#endif //ENGINEERING_PROJECT_USB_H
