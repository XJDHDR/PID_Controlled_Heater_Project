// This code is provided under the MPL v2.0 license. Copyright 2025 Xavier du Hecquet de Rauville
// Details may be found in License.txt
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
//  This Source Code Form is "Incompatible With Secondary Licenses", as
//  defined by the Mozilla Public License, v. 2.0.

#ifndef ENGINEERING_PROJECT_UTILS_H
#define ENGINEERING_PROJECT_UTILS_H

#include <cstdint>
#include <string>

class Utils
{
public:
	static std::string StringFormat(std::string FormatStr, ...);
	static uint8_t CalcCrc8(uint8_t InitialCrc, uint8_t byte);

	[[noreturn]] static void ErrorState(const std::string& ErrorMsg);
};

#endif //ENGINEERING_PROJECT_UTILS_H
