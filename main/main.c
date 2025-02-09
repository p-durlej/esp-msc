
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>

#include "main.h"
#include "shell.h"
#include "sdcard.h"
#include "msc.h"

#define DEFAULT_IMAGE "storage.img"
#define SDROOT "/sdcard"
#define CFGNAME SDROOT "/vreader.cfg"

struct config config;

static int storage_fd = -1;

void load_config(void)
{
    int fd;

    memset(&config, 0, sizeof config);
    strcpy(config.image_name, DEFAULT_IMAGE);

    fd = open(CFGNAME, O_RDONLY);
    read(fd, &config, sizeof config);
    close(fd);
}

void save_config(void)
{
    int fd;

    fd = open(CFGNAME, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    write(fd, &config, sizeof config);
    close(fd);
}

static const char *storage_pathname(void)
{
    static char pathname[512];
    
    strcpy(pathname, SDROOT "/");
    strcat(pathname, config.image_name);
    return pathname;
}

static void close_storage(void)
{
    if (storage_fd < 0)
        return;

    close(storage_fd);
    storage_fd = -1;
}

static void open_storage(void)
{
    close_storage();

    const char *pathname = storage_pathname();
    printf("pathname = %s\n", pathname);

    storage_fd = open(pathname, O_RDWR);
    printf("storage_fd = %i\n", storage_fd);
}

static int is_image_name(const char *name)
{
    const char *p;

    p = strrchr(name, '.');
    if (p == NULL)
        return 0;
    
    return !strcmp(p, ".img") || !strcmp(p, ".fs") || !strcmp(p, ".flp");
}

const char **image_names(void)
{
    const struct dirent *de;
    int cnt = 0;
    DIR *dir;
    
    dir = opendir(SDROOT);
    while (de = readdir(dir), de != NULL)
        if (is_image_name(de->d_name))
            cnt++;
    rewinddir(dir);

    char **names = calloc(cnt + 1, sizeof *names);
    int i = 0;
    while (de = readdir(dir), de != NULL)
        if (is_image_name(de->d_name))
        {
            names[i++] = strdup(de->d_name);
        }
    names[cnt] = NULL;
    closedir(dir);

    return (const char **)names;
}

void create_image(const char *name, long block_count)
{
    static char buf[512];

    printf("create_image: name = %s, block_count = %li\n", name, block_count);
    close_storage();

    strcpy(config.image_name, name);
    int fd = open(storage_pathname(), O_RDWR | O_CREAT | O_TRUNC, 0666);
    for (long i = 0; i < block_count; i++)
        write(fd, buf, 512);
    close(fd);

    reinit();
}

void change_image(const char *name)
{
    printf("change_image: name = %s\n", name);
    strcpy(config.image_name, name);
    reinit();
}

void remove_image(const char *name)
{
    static char pathname[512];

    strcpy(pathname, SDROOT "/");
    strcat(pathname, name);
    unlink(pathname);
}

void close_image(void)
{
    close_storage();
    reinit();
}

void reinit(void)
{
    msc_shutdown();
    open_storage();
    msc_init(storage_fd);
}

void app_main(void)
{
    shell_init();
    sdcard_init();
    load_config();
    open_storage();
    msc_init(storage_fd);
}
