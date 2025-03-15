/* SPI Master example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////// Please update the following configuration according to your
/// HardWare spec /////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
#define SPI_HOST SPI2_HOST

#define PIN_NUM_MISO 36 // Not used by the e-paper module
#define PIN_NUM_MOSI 37
#define PIN_NUM_CLK 38
#define PIN_NUM_CS 39
#define PIN_NUM_DC 40
#define PIN_NUM_RST 41
#define PIN_NUM_BUSY 42

#define MAX_XFER 4096

spi_device_handle_t spi;

void epaper_send_request(const uint8_t *data, size_t len, bool is_command) {
	esp_err_t ret;
	spi_transaction_t t;

	if (len == 0) {
		return; // no need to send anything
	}

	memset(&t, 0, sizeof(t)); // Zero out the transaction
	t.length = len * 8; // Len is in bytes, transaction length is in bits.
	t.tx_buffer = data; // Data
	t.user = (void *)!is_command;		    // D/C needs to be set to 1
	ret = spi_device_polling_transmit(spi, &t); // Transmit!
	assert(ret == ESP_OK); // Should have had no issues.
}

void epaper_send_command(const uint8_t cmd) {
	epaper_send_request(&cmd, 1, true);
}

void epaper_send_data(const uint8_t *data, int len) {
	epaper_send_request(data, len, false);
}

void epaper_send_byte(uint8_t data) { epaper_send_data(&data, 1); }

bool epaper_is_busy(void) { return gpio_get_level(PIN_NUM_BUSY); }

// This function is called (in irq context!) just before a transmission starts.
// It will set the D/C line to the value indicated in the user field.
static void epaper_spi_pre_transfer_callback(spi_transaction_t *t) {
	int dc = (bool)t->user;
	gpio_set_level(PIN_NUM_DC, dc);
}

// Initialize the GPIO interface
static void epaper_init_gpio(spi_device_handle_t spi) {
	// Initialize non-SPI GPIOs
	gpio_config_t io_conf = {};
	io_conf.pin_bit_mask = ((1ULL << PIN_NUM_DC) | (1ULL << PIN_NUM_RST));
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pull_up_en = true;
	gpio_config(&io_conf);

	// Hold the e-paper display in reset
	gpio_set_level(PIN_NUM_RST, 0);
}

void epaper_reset(void) {
	gpio_set_level(PIN_NUM_RST, 0);
	vTaskDelay(20 / portTICK_PERIOD_MS);
	gpio_set_level(PIN_NUM_RST, 1);
	vTaskDelay(20 / portTICK_PERIOD_MS);
}

void epaper_init_spi(void) {
	esp_err_t ret;
	spi_bus_config_t buscfg = {
	    .miso_io_num = PIN_NUM_MISO,
	    .mosi_io_num = PIN_NUM_MOSI,
	    .sclk_io_num = PIN_NUM_CLK,
	    .quadwp_io_num = -1,
	    .quadhd_io_num = -1,
	    .max_transfer_sz = MAX_XFER,
	};
	spi_device_interface_config_t devcfg = {
#ifdef CONFIG_LCD_OVERCLOCK
	    .clock_speed_hz = 26 * 1000 * 1000, // Clock out at 26 MHz
#else
	    .clock_speed_hz = 10 * 1000 * 1000, // Clock out at 10 MHz
#endif
	    .mode = 0,			// SPI mode 0
	    .spics_io_num = PIN_NUM_CS, // CS pin
	    .queue_size =
		7, // We want to be able to queue 7 transactions at a time
	    .pre_cb =
		epaper_spi_pre_transfer_callback, // Specify pre-transfer
						  // callback to handle D/C line
	};
	// Initialize the SPI bus
	ret = spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
	ESP_ERROR_CHECK(ret);

	// Attach the LCD to the SPI bus
	ret = spi_bus_add_device(SPI_HOST, &devcfg, &spi);
	ESP_ERROR_CHECK(ret);

	epaper_init_gpio(spi);
}
