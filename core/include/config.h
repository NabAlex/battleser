#ifndef MAIN_CONFIG_H
#define MAIN_CONFIG_H

#define BIMA_CONFIG_DIR "/etc"

typedef struct config
{
    char *filename;
    map_t _config;
} config_t;

config_t *
config_parse(const char *filename);

void
config_release(config_t *cfg);

void
bima_config_init();

config_t *
bima_config_self();

int32_t
bima_config_int();

const char *
bima_config_str();

int32_t
config_int(config_t *cfg);

const char *
config_str(config_t *cfg);

#endif //MAIN_CONFIG_H
