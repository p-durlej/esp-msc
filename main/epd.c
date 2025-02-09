/* This file provides a rudimentary set of functions to drive a Waveshare
 * e-paper display, specifically EPD_1in54_V2. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

/* TODO: these come from idf components, make sure they're included */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"


#define EPD_HOST    SPI2_HOST

#define PIN_NUM_MISO 25
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  19
#define PIN_NUM_CS   22
#define PIN_NUM_BUSY 22

#define PIN_NUM_DC   21
#define PIN_NUM_RST  18
/* #define PIN_NUM_BCKL 5 */          /* useless? */

#define LCD_BK_LIGHT_ON_LEVEL   0

/* Copy paste from the IDF example, adjust later:
 *
 * To speed up transfers, every SPI transfer sends a bunch of lines. This
 * define specifies how many. More means more memory use, but less overhead
 * for setting up / finishing transfers. Make sure 240 is dividable by
 * this. */

/* FIXME: should it be divisible by 200 in our case since the panel is
 * 200x200 */
#define PARALLEL_LINES 16

/* TODO: should these be in a header instead? */
#define epd_width  200
#define epd_height 200

#define epd_cmd_swreset                           0x12
#define epd_cmd_driver_output_control             0x01
#define epd_cmd_data_entry_mode                   0x11
#define epd_cmd_border_waveform                   0x3C
#define epd_cmd_load_temperature_waveform_setting 0x22

/* Place these into DRAM so they're accessible by DMA. */

/* waveform full refresh */
DRAM_ATTR static const unsigned char epd_wf_full[159] =
{
    0x80, 0x48, 0x40, 0x0,  0x0,  0x0,  0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x40, 0x48, 0x80, 0x0,  0x0,  0x0,  0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x80, 0x48, 0x40, 0x0,  0x0,  0x0,  0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x40, 0x48, 0x80, 0x0,  0x0,  0x0,  0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0xA,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
    0x8,  0x1,  0x0,  0x8,  0x1,  0x0,  0x2,
    0xA,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
    0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
    0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
    0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
    0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
    0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
    0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
    0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
    0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
    0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
    0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x0, 0x0, 0x0,
    0x22, 0x17, 0x41, 0x0,  0x32, 0x20
};

/* waveform partial refresh(fast) */
DRAM_ATTR static const unsigned char epd_wf_partial[159] =
{
    0x0,  0x40, 0x0,  0x0,  0x0,  0x0,  0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x80, 0x80, 0x0,  0x0,  0x0,  0x0,  0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x40, 0x40, 0x0,  0x0,  0x0,  0x0,  0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0,  0x80, 0x0,  0x0,  0x0,  0x0,  0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0xF,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
    0x1,  0x1,  0x0,  0x0,  0x0,  0x0,  0x0,
    0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
    0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
    0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
    0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
    0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
    0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
    0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
    0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
    0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
    0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
    0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x0, 0x0, 0x0,
    0x02, 0x17, 0x41, 0xB0, 0x32, 0x28,
};

/* FIXME */
#define Debug puts

static void delay_ms(int ms)
{
    vTaskDelay(100 / portTICK_PERIOD_MS);
}

static void epd_reset(void)
{
    gpio_set_level(PIN_NUM_RST, 1);
    delay_ms(20);
    gpio_set_level(PIN_NUM_RST, 0);
    delay_ms(2);            /* taken verbatim, huh? */
    gpio_set_level(PIN_NUM_RST, 1);
    delay_ms(20);
}

static void epd_read_busy(void)
{
    Debug("e-Paper busy");
    while(gpio_get_level(PIN_NUM_BUSY) == 1) {      //LOW: idle, HIGH: busy
        delay_ms(1);
    }
    Debug("e-Paper busy release");
}

void epd_send_cmd(spi_device_handle_t spi, const uint8_t cmd, bool keep_cs_active)
{
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length = 8;                   //Command is 8 bits
    t.tx_buffer = &cmd;             //The data is the cmd itself
    t.user = (void*)0;              //D/C needs to be set to 0
    if (keep_cs_active) {
        t.flags = SPI_TRANS_CS_KEEP_ACTIVE;   //Keep CS active after data transfer
    }
    ret = spi_device_polling_transmit(spi, &t); //Transmit!
    assert(ret == ESP_OK);          //Should have had no issues.
}

void epd_send_data(spi_device_handle_t spi, const uint8_t *data, int len)
{
    esp_err_t ret;
    spi_transaction_t t;
    if (len == 0) {
        return;    //no need to send anything
    }
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length = len * 8;             //Len is in bytes, transaction length is in bits.
    t.tx_buffer = data;             //Data
    t.user = (void*)1;              //D/C needs to be set to 1
    ret = spi_device_polling_transmit(spi, &t); //Transmit!
    assert(ret == ESP_OK);          //Should have had no issues.
}

/* FIXME: these are pointless, see functions above */
/* static void epd_send_command(uint8_t cmd) */
/* { */
/*     gpio_set_level(PIN_NUM_DC, 0); */
/*     /\* FIXME: setting CS might be pointless as it's a part of a SPI */
/*      * transaction. *\/ */
/*     gpio_set_level(PIN_NUM_CS, 0); */
/*     DEV_SPI_WriteByte(reg); */
/*     gpio_set_level(PIN_NUM_CS, 1); */
/* } */

/* static void epd_send_data(uint8_t data) */
/* { */
/*     gpio_set_level(PIN_NUM_DC, 1); */
/*     gpio_set_level(PIN_NUM_CS, 0); */
/*     DEV_SPI_WriteByte(data); */
/*     gpio_set_level(PIN_NUM_CS, 1); */
/* } */

static void epd_set_windows(UWORD xstart, UWORD ystart, UWORD xend, UWORD yend)
{
    epd_send_command(0x44); // SET_RAM_X_ADDRESS_START_END_POSITION
    epd_send_data((xstart>>3) & 0xFF);
    epd_send_data((xend>>3) & 0xFF);

    epd_send_command(0x45); // SET_RAM_Y_ADDRESS_START_END_POSITION
    epd_send_data(ystart & 0xFF);
    epd_send_data((ystart >> 8) & 0xFF);
    epd_send_data(yend & 0xFF);
    epd_send_data((yend >> 8) & 0xFF);
}

static void epd_set_cursor(UWORD xstart, UWORD ystart)
{
    epd_send_command(0x4E); // SET_RAM_X_ADDRESS_COUNTER

    /* This is taken verbatim, x offset is 8 bits? */
    epd_send_data(xstart & 0xFF);

    epd_send_command(0x4F); // SET_RAM_Y_ADDRESS_COUNTER
    epd_send_data(ystart & 0xFF);
    epd_send_data((ystart >> 8) & 0xFF);
}

void epd_gpio_init(void)
{
    gpio_config_t io_conf = {};

    /* inputs */
    io_conf.pin_bit_mask = (1ULL << PIN_NUM_BUSY);
    io_conf.mode = GPIO_MODE_INPUT;
    /* io_conf.pull_up_en = true; */
    gpio_config(&io_conf);

    /* outputs */
    io_conf.pin_bit_mask = ((1ULL << PIN_NUM_DC) | (1ULL << PIN_NUM_RST));
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = true;
    gpio_config(&io_conf);
}

/* This function is called (in irq context!) just before a transmission
 * starts. It will set the D/C line to the value indicated in the user
 * field. */
void epd_spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc = (int)t->user;
    gpio_set_level(PIN_NUM_DC, dc);
}

/* FIXME */
spi_device_handle_t epd_spi_init(void)
{
    esp_err_t ret;
    spi_device_handle_t spi;
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = PARALLEL_LINES * 320 * 2 + 8
    };
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000,      // Clock out at 10 MHz
        .mode = 0,                               // SPI mode 0
        .spics_io_num = PIN_NUM_CS,              // CS pin
        .queue_size = 7,                         // We want to be able to queue 7 transactions at a time
        .pre_cb = epd_spi_pre_transfer_callback, // Specify pre-transfer callback to handle D/C line
    };
    //Initialize the SPI bus
    ret = spi_bus_initialize(EPD_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    //Attach the EPD to the SPI bus
    ret = spi_bus_add_device(EPD_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);

    /* FIXME */
    return spi;
}

void epd_init(void)
{
    epd_gpio_init();

    spi_device_handle_t spi = epd_spi_init();

    /* epd stuff */
    epd_reset();

    epd_read_busy();
    epd_send_command(epd_cmd_swreset);
    epd_read_busy();

    epd_send_command(epd_cmd_driver_output_control);
    epd_send_data(0xC7);
    epd_send_data(0x00);
    epd_send_data(0x01);

    epd_send_command(epd_cmd_data_entry_mode);
    epd_send_data(0x01);

    epd_set_windows(0, epd_height - 1, epd_width - 1, 0);

    epd_send_command(epd_cmd_border_waveform);
    epd_send_data(0x01);

    epd_send_command(0x18);
    epd_send_data(0x80);

    epd_send_command(epd_cmd_load_temperature_waveform_setting);
    epd_send_data(0XB1);
    epd_send_command(0x20);

    epd_set_cursor(0, epd_height - 1);
    epd_read_busy();

    epd_set_lut(epd_wf_full);
}
