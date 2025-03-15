/* Copyright (c) Piotr Durlej
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

#include <esp_console.h>
#include <string.h>

#include "epaper.h"
#include "main.h"
#include "msc.h"
#include "sdcard.h"

static esp_console_repl_t *repl;

static int cmd_disable(int argc, char **argv) {
	config.msc_disabled = true;
	msc_shutdown();
	return 0;
}

static int cmd_enable(int argc, char **argv) {
	config.msc_disabled = false;
	reinit();
	return 0;
}

static int cmd_reinit(int argc, char **argv) {
	reinit();
	return 0;
}

static int cmd_loadconf(int argc, char **argv) {
	load_config();
	reinit();
	return 0;
}

static int cmd_saveconf(int argc, char **argv) {
	save_config();
	return 0;
}

static int cmd_showconf(int argc, char **argv) {
	printf("image_name = %s\n", config.image_name);
	printf("msc_disabled = %i\n", config.msc_disabled);
	return 0;
}

static int cmd_change(int argc, char **argv) {
	if (argc != 2) {
		printf("arg count\n");
		return 1;
	}
	change_image(argv[1]);
	return 0;
}

static int cmd_create(int argc, char **argv) {
	if (argc != 3) {
		printf("arg count\n");
		return 1;
	}
	create_image(argv[1], atol(argv[2]));
	return 0;
}

static int cmd_remove(int argc, char **argv) {
	if (argc != 2) {
		printf("arg count\n");
		return 1;
	}
	remove_image(argv[1]);
	return 0;
}

static int cmd_format(int argc, char **argv) {
	close_image();
	sdcard_format();
	reinit();
	return 0;
}

static int cmd_list(int argc, char **argv) {
	const char **images;
	int i;

	images = image_names();
	for (i = 0; images[i]; i++)
		printf("%s\n", images[i]);
	return 0;
}

static int cmd_display(int argc, char **argv) {
	static uint8_t image[200 * 200 / 8];

	int x, y;
	int i;

	memset(image, 0x55, sizeof image);

	uint8_t data = 0;
	for (i = 0, y = 0; y < 200; y++) {
		if (y % 16 == 0)
			data ^= 0xff;

		uint8_t rowdata = data;
		for (x = 0; x < 25; x++, i++) {
			if (x % 2 == 0)
				rowdata ^= 0xff;
			image[i] = rowdata;
		}
	}

	epaper_update(image);
	return 0;
}

static int cmd_clear(int argc, char **argv) {
	static uint8_t image[200 * 200 / 8];

	memset(image, 255, sizeof image);

	epaper_update(image);
	return 0;
}

static const esp_console_cmd_t cmds[] = {
    {
	.command = "disable",
	.help = "disable the USB MSC device",
	.hint = NULL,
	.func = &cmd_disable,
    },
    {
	.command = "enable",
	.help = "enable the USB MSC device",
	.hint = NULL,
	.func = &cmd_enable,
    },
    {
	.command = "reinit",
	.help = "reinitialize the USB MSC device",
	.hint = NULL,
	.func = &cmd_reinit,
    },
    {
	.command = "loadconf",
	.help = "reload configuration from the persistent storage",
	.hint = NULL,
	.func = &cmd_loadconf,
    },
    {
	.command = "saveconf",
	.help = "save configuration to the persistent storage",
	.hint = NULL,
	.func = &cmd_saveconf,
    },
    {
	.command = "showconf",
	.help = "show the running configuration",
	.hint = NULL,
	.func = &cmd_showconf,
    },
    {
	.command = "change",
	.help = "change the current image",
	.hint = NULL,
	.func = &cmd_change,
    },
    {
	.command = "create",
	.help = "change the current image",
	.hint = NULL,
	.func = &cmd_create,
    },
    {
	.command = "remove",
	.help = "remove an image file",
	.hint = NULL,
	.func = &cmd_remove,
    },
    {
	.command = "format",
	.help = "format the microsd card",
	.hint = NULL,
	.func = &cmd_format,
    },
    {
	.command = "list",
	.help = "list disk images",
	.hint = NULL,
	.func = &cmd_list,
    },
    {
	.command = "display",
	.help = "display a picture on the internal display",
	.hint = NULL,
	.func = &cmd_display,
    },
    {
	.command = "clear",
	.help = "clear the internal display",
	.hint = NULL,
	.func = &cmd_clear,
    },
};

void shell_init(void) {
	esp_console_repl_config_t repl_config =
	    ESP_CONSOLE_REPL_CONFIG_DEFAULT();
	repl_config.prompt = "vreader>";
	repl_config.max_cmdline_length = 256;

	esp_console_dev_uart_config_t hw_config =
	    ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(
	    esp_console_new_repl_uart(&hw_config, &repl_config, &repl));

	for (int i = 0; i < sizeof cmds / sizeof *cmds; i++) {
		ESP_ERROR_CHECK(esp_console_cmd_register(&cmds[i]));
	}

	ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
