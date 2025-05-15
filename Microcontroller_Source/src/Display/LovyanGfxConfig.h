// This code is provided under the MPL v2.0 license. Copyright 2025 Xavier du Hecquet de Rauville
// Details may be found in License.txt
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
//  This Source Code Form is "Incompatible With Secondary Licenses", as
//  defined by the Mozilla Public License, v. 2.0.

#ifndef ENGINEERING_PROJECT_LOVYANGFXCONFIG_H
#define ENGINEERING_PROJECT_LOVYANGFXCONFIG_H

#define DISPLAY_WIDTH_PX        240
#define DISPLAY_HEIGHT_PX       320

#include <LovyanGFX.hpp>

class LovyanGfxConfig : public lgfx::LGFX_Device
{
	lgfx::Bus_SPI spiInterface;
	lgfx::Panel_ILI9341 ili9341;

public:
	LovyanGfxConfig()
	{
		{
			lgfx::Bus_SPI::config_t busConfig = spiInterface.config();

			busConfig.spi_host = SPI2_HOST;
			busConfig.spi_mode = 0;
			busConfig.freq_write = 20000000;
			busConfig.spi_3wire = false;
			busConfig.use_lock = true;
			busConfig.dma_channel = 1;
			busConfig.pin_sclk = 20;
			busConfig.pin_mosi = 21;
			busConfig.pin_dc = 3;

			spiInterface.config(busConfig);
			ili9341.setBus(&spiInterface);
		}

		{
			lgfx::Panel_Device::config_t panelConfig = ili9341.config();

			panelConfig.pin_cs = 9;

			panelConfig.panel_width = DISPLAY_WIDTH_PX;
			panelConfig.panel_height = DISPLAY_HEIGHT_PX;
			panelConfig.offset_x = 0;
			panelConfig.offset_y = 0;
			panelConfig.offset_rotation = 0;
			panelConfig.dummy_read_pixel = 8;
			panelConfig.dummy_read_bits = 1;
			panelConfig.invert = false;
			panelConfig.rgb_order = false;
			panelConfig.dlen_16bit = false;
			panelConfig.bus_shared = false;

			ili9341.config(panelConfig);
		}

		setPanel(&ili9341);
	}
};

#endif //ENGINEERING_PROJECT_LOVYANGFXCONFIG_H
