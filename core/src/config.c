#include "core.h"

static config_t *bconfig;

config_t *
config_parse(const char *filename)
{
    config_t *cfg = calloc(1, sizeof(*cfg));
    cfg->_config = hashmap_new();

    char *key, *value;
    FILE *f = fopen(filename, "r");
    log_w("try open %s", filename);
    xassert(f, "cannot open config file");

    do
    {
        int32_t r = fscanf("%s%s", &key, &value);
        if (r != 2)
            break;

        /* TODO remove value from strdup */
        hashmap_put(cfg->_config, key, strdup(value));
    } while (1);

    fclose(f);

    log_w("file was parse (%s)", cfg->filename);
    return cfg;
}

void
config_release(config_t *cfg)
{
    xassert(cfg, "try release NULL config");

    hashmap_free(cfg->_config);

    free(cfg->filename);
    free(cfg);
}

void
bima_config_init()
{
    bconfig = config_parse(BIMA_CONFIG_DIR "/" _MODULE_NAME_ ".conf");
}

config_t *
bima_config_self()
{ return bconfig; }

int32_t
bima_config_int()
{
    assert(0);
    return 0;
}

const char *
bima_config_str()
{
    return NULL;
}

int32_t
config_int(config_t *cfg)
{
    assert(0);
    return 0;
}

const char *
config_str(config_t *cfg)
{
    return NULL;
}