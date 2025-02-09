#include <esp_console.h>

#include "main.h"

static esp_console_repl_t *repl;

static int cmd_reinit(int argc, char **argv)
{
    reinit();
    return 0;
}

static int cmd_loadconf(int argc, char **argv)
{
    load_config();
    reinit();
    return 0;
}

static int cmd_saveconf(int argc, char **argv)
{
    save_config();
    return 0;
}

static int cmd_showconf(int argc, char **argv)
{
    printf("image_name = %s\n", config.image_name);
    return 0;
}

static int cmd_change(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("arg count\n");
        return 1;
    }
    change_image(argv[1]);
    return 0;
}

static int cmd_create(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("arg count\n");
        return 1;
    }
    create_image(argv[1], atol(argv[2]));
    return 0;
}

static int cmd_remove(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("arg count\n");
        return 1;
    }
    remove_image(argv[1]);
    return 0;
}

static int cmd_list(int argc, char **argv)
{
    const char **images;
    int i;

    images = image_names();
    for (i = 0; images[i]; i++)
        printf("%s\n", images[i]);
    return 0;
}

static const esp_console_cmd_t cmds[] =
{
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
        .command = "list",
        .help = "list disk images",
        .hint = NULL,
        .func = &cmd_list,
    },
};

void shell_init(void)
{
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "vreader>";
    repl_config.max_cmdline_length = 256;

    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));

    for (int i = 0; i < sizeof cmds / sizeof *cmds; i++) {
        ESP_ERROR_CHECK(esp_console_cmd_register(&cmds[i]));
    }

    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
