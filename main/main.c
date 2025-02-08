
#include <stdio.h>
#include <fcntl.h>

#include "sdcard.h"
#include "msc.h"

static int storage_fd = -1;

static void open_storage(void)
{
    storage_fd = open("/sdcard/storage.img", O_RDWR);
    printf("storage_fd = %i\n", storage_fd);
}

void app_main(void)
{
    sdcard_init();
    open_storage();
    msc_init(storage_fd);
}
