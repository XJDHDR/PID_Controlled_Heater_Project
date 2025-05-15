// This code is provided under the MPL v2.0 license. Copyright 2025 Xavier du Hecquet de Rauville
// Details may be found in License.txt
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
//  This Source Code Form is "Incompatible With Secondary Licenses", as
//  defined by the Mozilla Public License, v. 2.0.

#ifndef ENGINEERING_PROJECT_LVGLHELPERS_H
#define ENGINEERING_PROJECT_LVGLHELPERS_H

#include "lvgl.h"

class LvglHelpers
{
public:
	static lv_obj_t* CreateTextLabel(
			lv_obj_t* ParentWidget, char* TextBuffer, bool IsParentGridAligned, lv_grid_align_t ColumnAlignment,
			int32_t ColumnPos, int32_t ColumnSpan, int32_t RowPos, int32_t RowSpan
	);
	static lv_obj_t* CreateTextLabel(
			lv_obj_t* ParentWidget, const char* Text, bool IsParentGridAligned, lv_grid_align_t ColumnAlignment,
			int32_t ColumnPos, int32_t ColumnSpan, int32_t RowPos, int32_t RowSpan
	);
	static lv_obj_t* CreateTextLabelButton(
			lv_obj_t* ParentWidget, lv_style_t* ButtonLabelTextStyle,
			void EventHandler(lv_event_t*), lv_event_code_t EventFilter, void* UserData,
			int32_t Width, int32_t Height, int32_t ColumnPos, int32_t ColumnSpan, int32_t RowPos, int32_t RowSpan,
			lv_grid_align_t ColumnAlignment, const char* LabelText, bool IncreaseTextSize, bool IsToggleButton
	);
	static lv_obj_t* CreateWidgetContainer(
			lv_obj_t* ParentWidget, lv_opa_t Opacity, int32_t Padding, bool SharpCorners, int32_t Width, int32_t Height,
			const int32_t* GridColumnDescriptors, const int32_t* GridRowDescriptors,
			bool IsParentGridAligned, int32_t ColumnPos, int32_t ColumnSpan, int32_t RowPos, int32_t RowSpan
	);
};

#endif //ENGINEERING_PROJECT_LVGLHELPERS_H
