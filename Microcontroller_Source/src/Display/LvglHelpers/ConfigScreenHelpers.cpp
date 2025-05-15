// This code is provided under the MPL v2.0 license. Copyright 2025 Xavier du Hecquet de Rauville
// Details may be found in License.txt
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
//  This Source Code Form is "Incompatible With Secondary Licenses", as
//  defined by the Mozilla Public License, v. 2.0.


#include "ConfigScreenHelpers.h"

#include <tuple>

#include "LvglHelpers.h"
#include "Misc/SerialHandler.h"
#include "Misc/Utils.h"


bool ConfigScreenHelpers::debug_decrementSpinboxButtonPressedEventHandler = false;
bool ConfigScreenHelpers::debug_incrementSpinboxButtonPressedEventHandler = false;


void ConfigScreenHelpers::CreateSettingRow(
		lv_obj_t* ParentWidget, const char* RowDescription, const int32_t RowPos, const int32_t RangeMin, const int32_t RangeMax, SpinboxData* SpinboxData
)
{
	std::ignore = LvglHelpers::CreateTextLabel(
			ParentWidget, RowDescription, true, LV_GRID_ALIGN_START, 0, 1, RowPos, 1
	);

	std::ignore = LvglHelpers::CreateTextLabelButton(
			ParentWidget, nullptr,
			incrementSpinboxButtonPressedEventHandler, LV_EVENT_CLICKED, SpinboxData,
			20, 26, 1, 1, RowPos, 1, LV_GRID_ALIGN_CENTER,
			LV_SYMBOL_PLUS, false, false
	);

	SpinboxData->Spinbox = lv_spinbox_create(ParentWidget);
	lv_spinbox_set_range(SpinboxData->Spinbox, RangeMin, RangeMax);
	lv_spinbox_set_value(SpinboxData->Spinbox, SpinboxData->GetCurrentValueAsInt());
	lv_spinbox_set_digit_format(SpinboxData->Spinbox, spinboxDigitCount, SpinboxData->GetDecimalPos());
	lv_obj_set_size(SpinboxData->Spinbox, 75, 26);
	lv_obj_set_style_pad_all(SpinboxData->Spinbox, 1, LV_PART_MAIN);
	lv_obj_set_style_text_align(SpinboxData->Spinbox, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
	lv_obj_set_grid_cell(
			SpinboxData->Spinbox, LV_GRID_ALIGN_END, 2, 1, LV_GRID_ALIGN_CENTER, RowPos, 1
	);

	std::ignore = LvglHelpers::CreateTextLabelButton(
			ParentWidget, nullptr,
			decrementSpinboxButtonPressedEventHandler, LV_EVENT_CLICKED, SpinboxData,
			20, 26, 3, 1, RowPos, 1, LV_GRID_ALIGN_CENTER,
			LV_SYMBOL_MINUS, false, false
	);
}

void ConfigScreenHelpers::decrementSpinboxButtonPressedEventHandler(lv_event_t* Event)
{
	SpinboxData* spinboxData = static_cast<SpinboxData*>(lv_event_get_user_data(Event));
	changeSpinboxValue(spinboxData, false, debug_decrementSpinboxButtonPressedEventHandler, "decremented");
}

void ConfigScreenHelpers::incrementSpinboxButtonPressedEventHandler(lv_event_t* Event)
{
	SpinboxData* spinboxData = static_cast<SpinboxData*>(lv_event_get_user_data(Event));
	changeSpinboxValue(spinboxData, true, debug_incrementSpinboxButtonPressedEventHandler, "incremented");
}

void ConfigScreenHelpers::changeSpinboxValue(SpinboxData* SpinboxData, const bool ShouldIncrement, const bool ShouldDebug, const char* SpinboxChangeTypeDebugMsg)
{
	if (!SpinboxData)
	{
		return;
	}

	if (ShouldIncrement)
	{
		lv_spinbox_increment(SpinboxData->Spinbox);
	}
	else
	{
		lv_spinbox_decrement(SpinboxData->Spinbox);
	}

	const int32_t newValue = lv_spinbox_get_value(SpinboxData->Spinbox);
	SpinboxData->SetCurrentValueWithInt(newValue);
	SpinboxData->HasValueBeenChangedSinceLastCheck = true;

	if (ShouldDebug)
	{
		std::string newSpinboxValueMsg = Utils::StringFormat("Spinbox %s. Value is now: %i", SpinboxChangeTypeDebugMsg, SpinboxData->GetCurrentValueAsInt());
		SerialHandler::SafeWriteLn(newSpinboxValueMsg, true);
	}
}

void ConfigScreenHelpers::enableDebugTriggers()
{
//	debug_decrementSpinboxButtonPressedEventHandler = true;
//	debug_incrementSpinboxButtonPressedEventHandler = true;
}
