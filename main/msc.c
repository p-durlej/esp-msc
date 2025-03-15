/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

/* DESCRIPTION:
 * This example contains code to make ESP32 based device recognizable by
 * USB-hosts as a USB Mass Storage Device. It either allows the embedded
 * application i.e. example to access the partition or Host PC accesses the
 * partition over USB MSC. They can't be allowed to access the partition at the
 * same time. For different scenarios and behaviour, Refer to README of this
 * example.
 */

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_partition.h"
#include "sdkconfig.h"
#include "tinyusb.h"
#include "tusb_msc_storage.h"
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#ifdef CONFIG_EXAMPLE_STORAGE_MEDIA_SDMMC
#include "diskio_impl.h"
#include "diskio_sdmmc.h"
#if CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_INTERNAL_IO
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#endif // CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_INTERNAL_IO
#endif

/*
 * We warn if a secondary serial console is enabled. A secondary serial console
 * is always output-only and hence not very useful for interactive console
 * applications. If you encounter this warning, consider disabling the secondary
 * serial console in menuconfig unless you know what you are doing.
 */
#if SOC_USB_SERIAL_JTAG_SUPPORTED
#if !CONFIG_ESP_CONSOLE_SECONDARY_NONE
#warning                                                                       \
    "A secondary serial console is not useful when using the console component. Please disable it in menuconfig."
#endif
#endif

static const char *TAG = "usbstorage";

/* TinyUSB descriptors
 ********************************************************************* */
#define EPNUM_MSC 1
#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN)

enum { ITF_NUM_MSC = 0, ITF_NUM_TOTAL };

enum {
	EDPT_CTRL_OUT = 0x00,
	EDPT_CTRL_IN = 0x80,

	EDPT_MSC_OUT = 0x01,
	EDPT_MSC_IN = 0x81,
};

static tusb_desc_device_t descriptor_config = {
    .bLength = sizeof(descriptor_config),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = 0x303A, // This is Espressif VID. This needs to be changed
			// according to Users / Customers
    .idProduct = 0x4002,
    .bcdDevice = 0x100,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01};

static uint8_t const msc_fs_configuration_desc[] = {
    // Config number, interface count, string index, total length, attribute,
    // power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, TUSB_DESC_TOTAL_LEN,
			  TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // Interface number, string index, EP Out & EP In address, EP size
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 0, EDPT_MSC_OUT, EDPT_MSC_IN, 64),
};

#if (TUD_OPT_HIGH_SPEED)
static const tusb_desc_device_qualifier_t device_qualifier = {
    .bLength = sizeof(tusb_desc_device_qualifier_t),
    .bDescriptorType = TUSB_DESC_DEVICE_QUALIFIER,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .bNumConfigurations = 0x01,
    .bReserved = 0};

static uint8_t const msc_hs_configuration_desc[] = {
    // Config number, interface count, string index, total length, attribute,
    // power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, TUSB_DESC_TOTAL_LEN,
			  TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // Interface number, string index, EP Out & EP In address, EP size
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 0, EDPT_MSC_OUT, EDPT_MSC_IN, 512),
};
#endif // TUD_OPT_HIGH_SPEED

static char const *string_desc_arr[] = {
    (const char[]){0x09, 0x04}, // 0: is supported language is English (0x0409)
    "TinyUSB",			// 1: Manufacturer
    "TinyUSB Device",		// 2: Product
    "123456",			// 3: Serials
    "Example MSC",		// 4. MSC
};
/*********************************************************************** TinyUSB
 * descriptors*/

static int msc_running;

void msc_init(int storage_fd) {
	if (msc_running) {
		ESP_LOGE(TAG, "USB MSC already running");
		return;
	}

	if (storage_fd < 0) {
		ESP_LOGE(TAG, "Invalid file descriptor");
		return;
	}

	ESP_LOGI(TAG, "Initializing storage...");
	ESP_ERROR_CHECK(tinyusb_msc_storage_init_fildes(storage_fd));

	ESP_LOGI(TAG, "USB MSC initialization");
	const tinyusb_config_t tusb_cfg = {
	    .device_descriptor = &descriptor_config,
	    .string_descriptor = string_desc_arr,
	    .string_descriptor_count =
		sizeof(string_desc_arr) / sizeof(string_desc_arr[0]),
	    .external_phy = false,
#if (TUD_OPT_HIGH_SPEED)
	    .fs_configuration_descriptor = msc_fs_configuration_desc,
	    .hs_configuration_descriptor = msc_hs_configuration_desc,
	    .qualifier_descriptor = &device_qualifier,
#else
	    .configuration_descriptor = msc_fs_configuration_desc,
#endif // TUD_OPT_HIGH_SPEED
	};
	ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
	ESP_LOGI(TAG, "USB MSC initialization DONE");

	msc_running = 1;
}

void msc_shutdown(void) {
	if (!msc_running) {
		ESP_LOGE(TAG, "USB MSC not currently running");
		return;
	}

	ESP_LOGI(TAG, "Shutting down USB MSC\n");
	tinyusb_msc_unregister_callback(TINYUSB_MSC_EVENT_MOUNT_CHANGED);
	tinyusb_msc_storage_deinit();
	tinyusb_driver_uninstall();

	msc_running = 0;
}
