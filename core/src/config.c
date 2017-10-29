#include "core.h"

static config_t *bconfig = NULL;

config_t *
config_parse(const char *filename)
{
    config_t *cfg = calloc(1, sizeof(*cfg));
    cfg->_config = hashmap_new();
    cfg->filename = strdup(filename);

    char key[256], value[1 KiB];
    FILE *f = fopen(filename, "r");
    log_w("try open %s", filename);
    xassert(f, "cannot open config file");

    do
    {
        int32_t r = fscanf(f, "%s%s", key, value);
        if (r != 2)
            break;

        /* TODO remove value from strdup */
        char *value_end = value + strlen(value), *tmp = NULL;
        if (*value == '"' && *(value_end - 1) == '"')
        {
            *(value_end - 1) = '\0';
            hashmap_put(cfg->_config, key, strdup(value + 1));
            continue;
        }

        int32_t maybe_int = (int32_t) strtol(value, &tmp, 10);
        if (tmp == value_end)
        {
            hashmap_put(cfg->_config, key, (any_t) maybe_int);
            continue;
        }

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

static void
bima_config_release()
{
    log_w("bima: config release");
    config_release(bconfig);
}

#define CONFIG_IF_INIT() do { if (!bconfig) bima_config_init(); } while (0)

void
bima_config_init()
{
    bconfig = config_parse(BIMA_CONFIG_DIR "/" _MODULE_NAME_ ".conf");
    atexit(bima_config_release);
}

config_t *
bima_config_self()
{
    CONFIG_IF_INIT();
    return bconfig;
}

int32_t
bima_config_int(char *name, int32_t default_)
{
    CONFIG_IF_INIT();

    any_t tmp;
    if (hashmap_get(bconfig->_config, name, &tmp) != MAP_OK)
        return default_;

    return (int32_t) tmp;
}

const char *
bima_config_str(char *name, char *default_)
{
    CONFIG_IF_INIT();

    any_t tmp;
    if (hashmap_get(bconfig->_config, name, &tmp) != MAP_OK)
        return default_;

    return tmp;
}

long
config_int(config_t *cfg)
{
    assert(0);
    return 0;
}

const char *
config_str(config_t *cfg)
{
    assert(0);
    return NULL;
}