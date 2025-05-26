// This code is provided under the MPL v2.0 license. Copyright 2025 Xavier du Hecquet de Rauville
// Details may be found in License.txt
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
//  This Source Code Form is "Incompatible With Secondary Licenses", as
//  defined by the Mozilla Public License, v. 2.0.


#include "LvglHelpers.h"


/**
 * @brief                       Create a new text label widget using a char array.
 *
 * @param  ParentWidget         The widget that the text label will be parented to.
 * @param  TextBuffer           An array that contains the text the label will display.
 * @param  IsParentGridAligned  Does the parent widget use grid alignment?
 * @param  ColumnAlignment      How will the widget be horizontally aligned in its cell?
 * @param  ColumnPos            The (first?) column in the grid this widget will occupy.
 * @param  ColumnSpan           How many columns this widget will occupy, starting from ColumnPos.
 * @param  RowPos               The (first?) row in the grid this widget will occupy.
 * @param  RowSpan              How many rows this widget will occupy, starting from RowPos.
 *
 * @returns                     The text label widget created by this function.
*/
lv_obj_t* LvglHelpers::CreateTextLabel(
		lv_obj_t* ParentWidget, char* TextBuffer, bool IsParentGridAligned, lv_grid_align_t ColumnAlignment,
		int32_t ColumnPos, int32_t ColumnSpan, int32_t RowPos, int32_t RowSpan
)
{
	lv_obj_t* textLabel = lv_label_create(ParentWidget);
	lv_label_set_text_static(textLabel, TextBuffer);

	if (IsParentGridAligned)
	{
		lv_obj_set_grid_cell(
				textLabel, ColumnAlignment, ColumnPos, ColumnSpan,
			LV_GRID_ALIGN_CENTER, RowPos, RowSpan
		);
	}

	return textLabel;
}

/**
 * @brief                       Create a new text label widget using a string.
 *
 * @param  ParentWidget         The widget that the text label will be parented to.
 * @param  Text                 A string with the text the label will display.
 * @param  IsParentGridAligned  Does the parent widget use grid alignment?
 * @param  ColumnAlignment      How will the widget be horizontally aligned in its cell?
 * @param  ColumnPos            The (first?) column in the grid this widget will occupy.
 * @param  ColumnSpan           How many columns this widget will occupy, starting from ColumnPos.
 * @param  RowPos               The (first?) row in the grid this widget will occupy.
 * @param  RowSpan              How many rows this widget will occupy, starting from RowPos.
 *
 * @returns                     The text label widget created by this function.
*/
lv_obj_t* LvglHelpers::CreateTextLabel(
	lv_obj_t* ParentWidget, const char* Text, bool IsParentGridAligned, lv_grid_align_t ColumnAlignment,
	int32_t ColumnPos, int32_t ColumnSpan, int32_t RowPos, int32_t RowSpan
)
{
	lv_obj_t* textLabel = lv_label_create(ParentWidget);
	lv_label_set_text(textLabel, Text);

	if (IsParentGridAligned)
	{
		lv_obj_set_grid_cell(
				textLabel, ColumnAlignment, ColumnPos, ColumnSpan,
				LV_GRID_ALIGN_CENTER, RowPos, RowSpan
		);
	}

	return textLabel;
}

/**
 * @brief                        Create a new button widget that contains a text label.
 *
 * @param  ParentWidget          The widget that the text label will be parented to.
 * @param  ButtonLabelTextStyle  The style that will be applied to the text.
 * @param  EventHandler          The event handler function that will be executed when specified events occur.
 * @param  EventFilter           The events that will trigger the function provided to EventHandler.
 * @param  UserData              The user data that will be passed to the event handler function.
 * @param  Width                 Button width in pixels.
 * @param  Height                Button height in pixels.
 * @param  ColumnPos             The (first?) column in the grid this widget will occupy.
 * @param  ColumnSpan            How many columns this widget will occupy, starting from ColumnPos.
 * @param  RowPos                The (first?) row in the grid this widget will occupy.
 * @param  RowSpan               How many rows this widget will occupy, starting from RowPos.
 * @param  ColumnAlignment       How will the widget be horizontally aligned in its cell?
 * @param  LabelText             A string with the text that will be drawn on the button.
 * @param  IncreaseTextSize      Applies style defined in ButtonLabelTextStyle if true.
 * @param  IsToggleButton        True creates a toggle button. False creates a regular press button.
 *
 * @returns                      The button widget created by this function.
*/
lv_obj_t* LvglHelpers::CreateTextLabelButton(
		lv_obj_t* ParentWidget, lv_style_t* ButtonLabelTextStyle,
		void EventHandler(lv_event_t*), lv_event_code_t EventFilter, void* UserData,
		int32_t Width, int32_t Height, int32_t ColumnPos, int32_t ColumnSpan, int32_t RowPos, int32_t RowSpan,
		lv_grid_align_t ColumnAlignment, const char* LabelText, bool IncreaseTextSize, bool IsToggleButton
)
{
	lv_obj_t* button = lv_button_create(ParentWidget);
	lv_obj_add_event_cb(button, EventHandler, EventFilter, UserData);
	lv_obj_set_size(button, Width, Height);
	lv_obj_set_grid_cell(button,
	                     ColumnAlignment, ColumnPos, ColumnSpan,
						 LV_GRID_ALIGN_CENTER, RowPos, RowSpan
	);
	lv_obj_set_grid_align(button, LV_GRID_ALIGN_SPACE_EVENLY, LV_GRID_ALIGN_CENTER);

	lv_obj_t* buttonText = lv_label_create(button);
	lv_label_set_text(buttonText, LabelText);
	lv_obj_align(buttonText, LV_ALIGN_CENTER, 0, 0);

	if (IncreaseTextSize)
	{
		lv_obj_add_style(buttonText, ButtonLabelTextStyle, LV_PART_MAIN);
	}

	if (IsToggleButton)
	{
		lv_obj_add_flag(button, LV_OBJ_FLAG_CHECKABLE);
	}

	return button;
}

/**
 * @brief                         Create an LVGL Object that is designed to hold other widgets.
 *
 * @param  ParentWidget           The widget that the text label will be parented to.
 * @param  Opacity                How opaque the widget's background will be.
 * @param  Padding                The size of the widget's padding in pixels.
 * @param  SharpCorners           Whether the widget will have sharp or rounded corners.
 * @param  Width                  Button width in pixels.
 * @param  Height                 Button height in pixels.
 * @param  GridColumnDescriptors  An array describing the grid columns in this widget.
 * @param  GridRowDescriptors     An array describing the grid rows in this widget.
 * @param  IsParentGridAligned    Does the parent widget use grid alignment?
 * @param  ColumnPos              The (first?) column in the grid this widget will occupy.
 * @param  ColumnSpan             How many columns this widget will occupy, starting from ColumnPos.
 * @param  RowPos                 The (first?) row in the grid this widget will occupy.
 * @param  RowSpan                How many rows this widget will occupy, starting from RowPos.
 *
 * @returns                       The LVGL Object widget created by this function.
*/
lv_obj_t* LvglHelpers::CreateWidgetContainer(
		lv_obj_t* ParentWidget, lv_opa_t Opacity, int32_t Padding, bool SharpCorners, int32_t Width, int32_t Height,
		const int32_t* GridColumnDescriptors, const int32_t* GridRowDescriptors,
		bool IsParentGridAligned, int32_t ColumnPos, int32_t ColumnSpan, int32_t RowPos, int32_t RowSpan
)
{
	lv_obj_t* widgetContainer = lv_obj_create(ParentWidget);
	lv_obj_set_style_border_width(widgetContainer, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_all(widgetContainer, Padding, LV_PART_MAIN);
	lv_obj_set_style_bg_opa(widgetContainer, Opacity, LV_PART_MAIN);

	lv_obj_set_style_size(widgetContainer, Width, Height, LV_PART_MAIN);
	lv_obj_set_layout(widgetContainer, LV_LAYOUT_GRID);
	lv_obj_set_style_pad_column(widgetContainer, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_row(widgetContainer, 4, LV_PART_MAIN);
	lv_obj_set_grid_align(widgetContainer, LV_GRID_ALIGN_SPACE_EVENLY, LV_GRID_ALIGN_CENTER);

	if (SharpCorners)
	{
		lv_obj_set_style_radius(widgetContainer, 0, LV_PART_MAIN);
	}

	if ((GridColumnDescriptors != nullptr) && (GridRowDescriptors != nullptr))
	{
		lv_obj_set_grid_dsc_array(widgetContainer, GridColumnDescriptors, GridRowDescriptors);
	}

	if (IsParentGridAligned)
	{
		lv_obj_set_grid_cell(
			widgetContainer,
			LV_GRID_ALIGN_CENTER, ColumnPos, ColumnSpan,
			LV_GRID_ALIGN_CENTER, RowPos, RowSpan
		);
	}

	return widgetContainer;
}
