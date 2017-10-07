#include "core.h"

static char __buf[256];
static int32_t __buf_len;

static char *dir;
static char *dfile;

#define MAX_FILES 100

static map_t descriptors;
int32_t *desc_array;

void
http_static_release(void)
{
    log_w("delete static dir");
    free(dir);
    free(dfile);
    free(desc_array);
    /* todo remove hashmap */
}

static int32_t d;

static int32_t
read_parser(conn_t *cn, http_request_t *request)
{
    char *file;

    /* TODO normal path !!! */
    file = request->head.url + 1;
    if (!*file)
        file = dfile;

    log_w("%s", dfile);

    int32_t *desc = NULL;
    int32_t r = hashmap_get(descriptors, file, &desc);
    if (r != MAP_OK)
        return http_send_data(cn, HTTP_NOTFOUND);

    if (http_send_file(cn, HTTP_OK, *desc) != RES_OK)
        return RES_ERR;

    return RES_OK;
}

void
http_start_static_server(char *filedir, char *default_file)
{
    dir = strdup(filedir);
    dfile = (default_file) ? strdup(default_file) : strdup("index.html");

    descriptors = hashmap_new();

    DIR *dp;
    struct dirent *ep;

    dp = opendir(dir);
    if (!dp)
    {
        log_e("cannot find dir: %s", dir);
        return;
    }

    char *delim = (dir[strlen(dir) - 1] == '/') ? "" : "/";

    int32_t desc_cur = -1;
    desc_array = calloc(MAX_FILES, sizeof(int32_t));
    while (ep = readdir(dp))
    {
        if (!strcmp(ep->d_name, ".") || !strcmp(ep->d_name, ".."))
            continue;

        log_w("start %s", ep->d_name);

        __buf_len = snprintf(__buf, sizeof(__buf), "%s%s%s", dir, delim, ep->d_name);
        xassert(__buf_len > 0, "too short buffer");
        desc_array[++desc_cur] = open(__buf, O_RDONLY);
        xassert(desc_array[desc_cur], "cannot open file");

        hashmap_put(descriptors, ep->d_name, desc_array + desc_cur);
    }

    closedir(dp);

    atexit(http_static_release);

    log_w("start server: %s", dir);
    http_start_server(read_parser);
}
