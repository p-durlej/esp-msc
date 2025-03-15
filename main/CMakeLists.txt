set(priv_requires fatfs console)

if(CONFIG_EXAMPLE_STORAGE_MEDIA_SPIFLASH)
    list(APPEND priv_requires wear_levelling esp_partition)
endif()

idf_component_register(
    SRCS "shell.c"
    SRCS "sdcard.c"
    SRCS "msc.c"
    SRCS "main.c"
    SRCS "epaper_ws.c"
    SRCS "epaper.c"
    SRCS "epaper_spi.c"
    INCLUDE_DIRS .
    REQUIRES fatfs
    PRIV_REQUIRES "${priv_requires}"
)
