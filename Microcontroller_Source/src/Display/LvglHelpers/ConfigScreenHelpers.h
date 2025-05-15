// This code is provided under the MPL v2.0 license. Copyright 2025 Xavier du Hecquet de Rauville
// Details may be found in License.txt
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
//  This Source Code Form is "Incompatible With Secondary Licenses", as
//  defined by the Mozilla Public License, v. 2.0.

#ifndef ENGINEERING_PROJECT_CONFIGSCREENHELPERS_H
#define ENGINEERING_PROJECT_CONFIGSCREENHELPERS_H

#include <cmath>
#include <cstdint>

#include "lvgl.h"

class ConfigScreenHelpers
{
private:
	static const uint32_t spinboxDigitCount = 5;


public:
	struct SpinboxData
	{
		bool HasValueBeenChangedSinceLastCheck = false;
		lv_obj_t* Spinbox = nullptr;

		virtual int32_t GetCurrentValueAsInt()
		{
			return 0;
		}

		virtual uint32_t GetDecimalPos()
		{
			return 0;
		}

		virtual void SetCurrentValueWithInt(int32_t NewValue)
		{}
	};

	struct IntSpinboxData final : SpinboxData
	{
		int32_t CurrentValue = 0;

		int32_t GetCurrentValueAsInt() final
		{
			return CurrentValue;
		}

		void SetCurrentValueWithInt(int32_t NewValue) final
		{
			CurrentValue = NewValue;
		}
	};

	struct FloatSpinboxData final : SpinboxData
	{
		uint32_t DecimalPosition = 0;
		float CurrentValue = 0.0;

		int32_t GetCurrentValueAsInt() final
		{
			return static_cast<int32_t>(CurrentValue * std::pow(10, (spinboxDigitCount - DecimalPosition)));
		}

		uint32_t GetDecimalPos() final
		{
			return DecimalPosition;
		}

		void SetCurrentValueWithInt(int32_t NewValue) final
		{
			CurrentValue = static_cast<float>(NewValue / std::pow(10, (spinboxDigitCount - DecimalPosition)));
		}
	};


	static void CreateSettingRow(
			lv_obj_t* ParentWidget, const char* RowDescription, int32_t RowPos,
			int32_t RangeMin, int32_t RangeMax, SpinboxData* FloatSpinbox
	);


private:
	static bool debug_decrementSpinboxButtonPressedEventHandler;
	static bool debug_incrementSpinboxButtonPressedEventHandler;

	static void decrementSpinboxButtonPressedEventHandler(lv_event_t* Event);
	static void incrementSpinboxButtonPressedEventHandler(lv_event_t* Event);
	static void changeSpinboxValue(SpinboxData* SpinboxData, bool ShouldIncrement, bool ShouldDebug, const char* SpinboxChangeTypeDebugMsg);

	void enableDebugTriggers();
};


#endif //ENGINEERING_PROJECT_CONFIGSCREENHELPERS_H
