/*****************************************************************************
* | File      	:   EPD_1in54_V2.c
* | Author      :   Waveshare team
* | Function    :   1.54inch e-paper V2
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2019-06-11
* | Info        :
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/

/* Modifications copyright (c) Piotr Durlej
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>

#include "FreeRTOS.h"
#include "epaper_spi.h"
#include "epaper_ws.h"

// XXX TODO: Clean up this shit...
typedef uint8_t uint8_t;
typedef uint16_t uint16_t;
typedef uint32_t uint32_t;

// waveform full refresh
unsigned char WF_Full_1IN54[159] = {
    0x80, 0x48, 0x40, 0x0,  0x0,  0x0,	0x0,  0x0, 0x0, 0x0, 0x0,  0x0,	 0x40,
    0x48, 0x80, 0x0,  0x0,  0x0,  0x0,	0x0,  0x0, 0x0, 0x0, 0x0,  0x80, 0x48,
    0x40, 0x0,	0x0,  0x0,  0x0,  0x0,	0x0,  0x0, 0x0, 0x0, 0x40, 0x48, 0x80,
    0x0,  0x0,	0x0,  0x0,  0x0,  0x0,	0x0,  0x0, 0x0, 0x0, 0x0,  0x0,	 0x0,
    0x0,  0x0,	0x0,  0x0,  0x0,  0x0,	0x0,  0x0, 0xA, 0x0, 0x0,  0x0,	 0x0,
    0x0,  0x0,	0x8,  0x1,  0x0,  0x8,	0x1,  0x0, 0x2, 0xA, 0x0,  0x0,	 0x0,
    0x0,  0x0,	0x0,  0x0,  0x0,  0x0,	0x0,  0x0, 0x0, 0x0, 0x0,  0x0,	 0x0,
    0x0,  0x0,	0x0,  0x0,  0x0,  0x0,	0x0,  0x0, 0x0, 0x0, 0x0,  0x0,	 0x0,
    0x0,  0x0,	0x0,  0x0,  0x0,  0x0,	0x0,  0x0, 0x0, 0x0, 0x0,  0x0,	 0x0,
    0x0,  0x0,	0x0,  0x0,  0x0,  0x0,	0x0,  0x0, 0x0, 0x0, 0x0,  0x0,	 0x0,
    0x0,  0x0,	0x0,  0x0,  0x0,  0x0,	0x0,  0x0, 0x0, 0x0, 0x0,  0x0,	 0x0,
    0x0,  0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x0, 0x0, 0x0, 0x22, 0x17, 0x41,
    0x0,  0x32, 0x20};

// waveform partial refresh(fast)
unsigned char WF_PARTIAL_1IN54_0[159] = {
    0x0,  0x40, 0x0,  0x0,  0x0,  0x0,	0x0,  0x0, 0x0, 0x0, 0x0,  0x0,	 0x80,
    0x80, 0x0,	0x0,  0x0,  0x0,  0x0,	0x0,  0x0, 0x0, 0x0, 0x0,  0x40, 0x40,
    0x0,  0x0,	0x0,  0x0,  0x0,  0x0,	0x0,  0x0, 0x0, 0x0, 0x0,  0x80, 0x0,
    0x0,  0x0,	0x0,  0x0,  0x0,  0x0,	0x0,  0x0, 0x0, 0x0, 0x0,  0x0,	 0x0,
    0x0,  0x0,	0x0,  0x0,  0x0,  0x0,	0x0,  0x0, 0xF, 0x0, 0x0,  0x0,	 0x0,
    0x0,  0x0,	0x1,  0x1,  0x0,  0x0,	0x0,  0x0, 0x0, 0x0, 0x0,  0x0,	 0x0,
    0x0,  0x0,	0x0,  0x0,  0x0,  0x0,	0x0,  0x0, 0x0, 0x0, 0x0,  0x0,	 0x0,
    0x0,  0x0,	0x0,  0x0,  0x0,  0x0,	0x0,  0x0, 0x0, 0x0, 0x0,  0x0,	 0x0,
    0x0,  0x0,	0x0,  0x0,  0x0,  0x0,	0x0,  0x0, 0x0, 0x0, 0x0,  0x0,	 0x0,
    0x0,  0x0,	0x0,  0x0,  0x0,  0x0,	0x0,  0x0, 0x0, 0x0, 0x0,  0x0,	 0x0,
    0x0,  0x0,	0x0,  0x0,  0x0,  0x0,	0x0,  0x0, 0x0, 0x0, 0x0,  0x0,	 0x0,
    0x0,  0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x0, 0x0, 0x0, 0x02, 0x17, 0x41,
    0xB0, 0x32, 0x28,
};

void Debug(const char *str) { fputs(str, stderr); }

/******************************************************************************
function :	Wait until the busy_pin goes LOW
parameter:
******************************************************************************/
static void EPD_1IN54_V2_ReadBusy(void) {
	Debug("e-Paper busy\r\n");
	while (epaper_is_busy()) { // LOW: idle, HIGH: busy
		vTaskDelay(1);
	}
	Debug("e-Paper busy release\r\n");
}

/******************************************************************************
function :	Turn On Display full
parameter:
******************************************************************************/
static void EPD_1IN54_V2_TurnOnDisplay(void) {
	epaper_send_command(0x22);
	epaper_send_byte(0xc7);
	epaper_send_command(0x20);
	EPD_1IN54_V2_ReadBusy();
}

/******************************************************************************
function :	Turn On Display part
parameter:
******************************************************************************/
static void EPD_1IN54_V2_TurnOnDisplayPart(void) {
	epaper_send_command(0x22);
	epaper_send_byte(0xcF);
	epaper_send_command(0x20);
	EPD_1IN54_V2_ReadBusy();
}

static void EPD_1IN54_V2_Lut(uint8_t *lut) {
	epaper_send_command(0x32);
	for (uint8_t i = 0; i < 153; i++)
		epaper_send_byte(lut[i]);
	EPD_1IN54_V2_ReadBusy();
}

static void EPD_1IN54_V2_SetLut(uint8_t *lut) {
	EPD_1IN54_V2_Lut(lut);

	epaper_send_command(0x3f);
	epaper_send_byte(lut[153]);

	epaper_send_command(0x03);
	epaper_send_byte(lut[154]);

	epaper_send_command(0x04);
	epaper_send_byte(lut[155]);
	epaper_send_byte(lut[156]);
	epaper_send_byte(lut[157]);

	epaper_send_command(0x2c);
	epaper_send_byte(lut[158]);
}

static void EPD_1IN54_V2_SetWindows(uint16_t Xstart, uint16_t Ystart,
				    uint16_t Xend, uint16_t Yend) {
	epaper_send_command(0x44); // SET_RAM_X_ADDRESS_START_END_POSITION
	epaper_send_byte((Xstart >> 3) & 0xFF);
	epaper_send_byte((Xend >> 3) & 0xFF);

	epaper_send_command(0x45); // SET_RAM_Y_ADDRESS_START_END_POSITION
	epaper_send_byte(Ystart & 0xFF);
	epaper_send_byte((Ystart >> 8) & 0xFF);
	epaper_send_byte(Yend & 0xFF);
	epaper_send_byte((Yend >> 8) & 0xFF);
}

static void EPD_1IN54_V2_SetCursor(uint16_t Xstart, uint16_t Ystart) {
	epaper_send_command(0x4E); // SET_RAM_X_ADDRESS_COUNTER
	epaper_send_byte(Xstart & 0xFF);

	epaper_send_command(0x4F); // SET_RAM_Y_ADDRESS_COUNTER
	epaper_send_byte(Ystart & 0xFF);
	epaper_send_byte((Ystart >> 8) & 0xFF);
}

/******************************************************************************
function :	Initialize the e-Paper register
parameter:
******************************************************************************/
void EPD_1IN54_V2_Init(void) {
	epaper_reset();

	EPD_1IN54_V2_ReadBusy();
	epaper_send_command(0x12); // SWRESET
	EPD_1IN54_V2_ReadBusy();

	epaper_send_command(0x01); // Driver output control
	epaper_send_byte(0xC7);
	epaper_send_byte(0x00);
	epaper_send_byte(0x01);

	epaper_send_command(0x11); // data entry mode
	epaper_send_byte(0x01);

	EPD_1IN54_V2_SetWindows(0, EPD_1IN54_V2_HEIGHT - 1,
				EPD_1IN54_V2_WIDTH - 1, 0);

	epaper_send_command(0x3C); // BorderWavefrom
	epaper_send_byte(0x01);

	epaper_send_command(0x18);
	epaper_send_byte(0x80);

	epaper_send_command(0x22); // //Load Temperature and waveform setting.
	epaper_send_byte(0XB1);
	epaper_send_command(0x20);

	EPD_1IN54_V2_SetCursor(0, EPD_1IN54_V2_HEIGHT - 1);
	EPD_1IN54_V2_ReadBusy();

	EPD_1IN54_V2_SetLut(WF_Full_1IN54);
}

/******************************************************************************
function :	Initialize the e-Paper register (Partial display)
parameter:
******************************************************************************/
void EPD_1IN54_V2_Init_Partial(void) {
	epaper_reset();
	EPD_1IN54_V2_ReadBusy();

	EPD_1IN54_V2_SetLut(WF_PARTIAL_1IN54_0);
	epaper_send_command(0x37);
	epaper_send_byte(0x00);
	epaper_send_byte(0x00);
	epaper_send_byte(0x00);
	epaper_send_byte(0x00);
	epaper_send_byte(0x00);
	epaper_send_byte(0x40);
	epaper_send_byte(0x00);
	epaper_send_byte(0x00);
	epaper_send_byte(0x00);
	epaper_send_byte(0x00);

	epaper_send_command(0x3C); // BorderWavefrom
	epaper_send_byte(0x80);

	epaper_send_command(0x22);
	epaper_send_byte(0xc0);
	epaper_send_command(0x20);
	EPD_1IN54_V2_ReadBusy();
}

/******************************************************************************
function :	Clear screen
parameter:
******************************************************************************/
void EPD_1IN54_V2_Clear(void) {
	uint16_t Width, Height;
	Width = (EPD_1IN54_V2_WIDTH % 8 == 0) ? (EPD_1IN54_V2_WIDTH / 8)
					      : (EPD_1IN54_V2_WIDTH / 8 + 1);
	Height = EPD_1IN54_V2_HEIGHT;

	epaper_send_command(0x24);
	for (uint16_t j = 0; j < Height; j++) {
		for (uint16_t i = 0; i < Width; i++) {
			epaper_send_byte(0XFF);
		}
	}
	epaper_send_command(0x26);
	for (uint16_t j = 0; j < Height; j++) {
		for (uint16_t i = 0; i < Width; i++) {
			epaper_send_byte(0XFF);
		}
	}
	EPD_1IN54_V2_TurnOnDisplay();
}

/******************************************************************************
function :	Sends the image buffer in RAM to e-Paper and displays
parameter:
******************************************************************************/
void EPD_1IN54_V2_Display(const uint8_t *Image) {
	uint16_t Width, Height;
	Width = (EPD_1IN54_V2_WIDTH % 8 == 0) ? (EPD_1IN54_V2_WIDTH / 8)
					      : (EPD_1IN54_V2_WIDTH / 8 + 1);
	Height = EPD_1IN54_V2_HEIGHT;

	uint32_t Addr = 0;
	epaper_send_command(0x24);
	for (uint16_t j = 0; j < Height; j++) {
		for (uint16_t i = 0; i < Width; i++) {
			Addr = i + j * Width;
			epaper_send_byte(Image[Addr]);
		}
	}
	EPD_1IN54_V2_TurnOnDisplay();
}

/******************************************************************************
function :	 The image of the previous frame must be uploaded, otherwise the
			 first few seconds will display an exception.
parameter:
******************************************************************************/
void EPD_1IN54_V2_DisplayPartBaseImage(const uint8_t *Image) {
	uint16_t Width, Height;
	Width = (EPD_1IN54_V2_WIDTH % 8 == 0) ? (EPD_1IN54_V2_WIDTH / 8)
					      : (EPD_1IN54_V2_WIDTH / 8 + 1);
	Height = EPD_1IN54_V2_HEIGHT;

	uint32_t Addr = 0;
	epaper_send_command(0x24);
	for (uint16_t j = 0; j < Height; j++) {
		for (uint16_t i = 0; i < Width; i++) {
			Addr = i + j * Width;
			epaper_send_byte(Image[Addr]);
		}
	}
	epaper_send_command(0x26);
	for (uint16_t j = 0; j < Height; j++) {
		for (uint16_t i = 0; i < Width; i++) {
			Addr = i + j * Width;
			epaper_send_byte(Image[Addr]);
		}
	}
	EPD_1IN54_V2_TurnOnDisplay();
}

/******************************************************************************
function :	Sends the image buffer in RAM to e-Paper and displays
parameter:
******************************************************************************/
void EPD_1IN54_V2_DisplayPart(const uint8_t *Image) {
	uint16_t Width, Height;
	Width = (EPD_1IN54_V2_WIDTH % 8 == 0) ? (EPD_1IN54_V2_WIDTH / 8)
					      : (EPD_1IN54_V2_WIDTH / 8 + 1);
	Height = EPD_1IN54_V2_HEIGHT;

	uint32_t Addr = 0;
	epaper_send_command(0x24);
	for (uint16_t j = 0; j < Height; j++) {
		for (uint16_t i = 0; i < Width; i++) {
			Addr = i + j * Width;
			epaper_send_byte(Image[Addr]);
		}
	}
	EPD_1IN54_V2_TurnOnDisplayPart();
}

/******************************************************************************
function :	Enter sleep mode
parameter:
******************************************************************************/
void EPD_1IN54_V2_Sleep(void) {
	epaper_send_command(0x10); // enter deep sleep
	epaper_send_byte(0x01);

	vTaskDelay(100 / portTICK_PERIOD_MS);
}
