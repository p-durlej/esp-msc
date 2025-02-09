extern struct config
{
    char image_name[256];
} config;

void load_config(void);
void save_config(void);

void create_image(const char *name, long block_count);
void change_image(const char *name);

const char **image_names(void);

void reinit(void);
