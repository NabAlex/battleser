#include "core.h"

typedef enum file_alloc
{
    USE_DESC, USE_MMAP
} file_alloc_t;

typedef struct file_info
{
    file_alloc_t type;
    union {
        int32_t desc;
        void *ptr;
    };

    char *file_type;
    ssize_t size;
} file_info_t;

static char __buf[512];
static int32_t __buf_len;

static char *dir;
static char *dfile;

#define MAX_FILES 1000

static map_t descriptors;
static map_t extensions;

file_info_t *desc_array;
int32_t desc_cur = -1;

static char *default_extention = "text/plain";
static char *extention_array[] = {
    "text/html",
    "text/css",
    "text/javascript",
    "image/jpeg",
    "image/jpeg",
    "image/png",
    "image/gif",
    "application/x-shockwave-flash",
    "text/plain"
};

static map_t
http_static_extentions()
{
    map_t ex = hashmap_new();
    hashmap_put(ex, "html", extention_array[0]);
    hashmap_put(ex, "css",  extention_array[1]);
    hashmap_put(ex, "js",   extention_array[2]);

    hashmap_put(ex, "jpg",  extention_array[3]);
    hashmap_put(ex, "JPG",  extention_array[3]);

    hashmap_put(ex, "jpeg", extention_array[4]);

    hashmap_put(ex, "png",  extention_array[5]);
    hashmap_put(ex, "PNG",  extention_array[5]);

    hashmap_put(ex, "gif",  extention_array[6]);
    hashmap_put(ex, "swf",  extention_array[7]);
    hashmap_put(ex, "txt",  extention_array[8]);
    return ex;
}

void
http_static_release(void)
{
    log_w("delete static dir");
    free(dir);
    free(dfile);
    free(desc_array);
    /* todo remove hashmap */
}

static int32_t
read_parser(conn_t *cn, http_request_t *request)
{
    char *file;

    /* TODO normal path !!! */
    file = request->head.url + 1;
    if (!*file)
        file = dfile;

    log_d("%s", file);

    char *query;
    if ((query = strstr(file, "?")) != NULL)
    {
        *query = '\0';
    }

    if (strcmp(request->head.type, "GET") || strstr(file, ".."))
        return http_send_data(cn, HTTP_METHOD_NOT_ALLOWED, STRSZ("Method not allowed"));

    file_info_t *info = NULL;
    int32_t r = hashmap_get(descriptors, file, &info);
    if (r != MAP_OK)
        return http_send_data(cn, HTTP_NOTFOUND, STRSZ("Not found"));


    __buf_len = snprintf(__buf, sizeof(__buf),
            "%s %d %s\r\n"
            "Server: bima\r\n"
            "Content-Length: %lu\r\n"
            "Content-Type: %s\r\n\r\n",
            request->head.http_version,
            HTTP_OK, http_response_get_status_name(HTTP_OK),
            info->size, info->file_type);

    xassert(bima_write(cn, __buf, __buf_len) == RES_OK, "cannot write in socket");

    if (!strcmp(request->head.type, "HEAD"))
    {
        bima_connection_close(cn);
        return RES_OK;
    };

    if (info->type == USE_MMAP)
        return bima_aio_write(cn->fd, info->ptr, info->size, cn);

    if (http_send_file(cn, HTTP_OK, info->desc, info->size) != RES_OK)
        return RES_ERR;

    return RES_OK;
}

static void
http_static_loader(char *filedir, char *addition)
{
    snprintf(__buf, sizeof(__buf), "%s%s", filedir, addition);
    char *fulldir = strdup(__buf);

    char *delim = (dir[strlen(dir) - 1] == '/') ? "" : "/";
    int32_t desc;

    DIR *dp;
    struct dirent *ep;
    struct stat stat_buf;

    dp = opendir(fulldir);
    if (!dp)
    {
        log_e("cannot find dir: %s", dir);
        return;
    }

    while (ep = readdir(dp))
    {
        if (!strcmp(ep->d_name, ".") || !strcmp(ep->d_name, ".."))
            continue;

        if (ep->d_type == DT_DIR) {
            __buf_len = snprintf(__buf, sizeof(__buf), "%s/%s", addition, ep->d_name);
            log_w("new dir %s", __buf);
            http_static_loader(filedir, strdup(__buf)); // mem leak
            continue;
        }

        __buf_len = snprintf(__buf, sizeof(__buf), "%s%s%s", fulldir, delim, ep->d_name);
        log_w("start %s", __buf);

        xassert(__buf_len > 0, "too short buffer");
        desc = open(__buf, O_RDONLY);
        xassert(desc > 0, "cannot open file");
        fstat(desc, &stat_buf);
        log_w("size %ld", stat_buf.st_size);

        ++desc_cur;

        // TODO (a.naberezhnyi) normal
        char *point = __buf + __buf_len, *file_type = NULL;
        while (*(--point) != '.' && point >= __buf);
        if (point <= __buf)
            desc_array[desc_cur].file_type = default_extention;
        else
        {
            point++;
            xassert(hashmap_get(extensions, point, &file_type) == MAP_OK, "cannot find extention");
            desc_array[desc_cur].file_type = file_type;
        }

        if (stat_buf.st_size >= 2 KiB)
        {
            desc_array[desc_cur].type = USE_MMAP;
            desc_array[desc_cur].ptr = mmap(0, stat_buf.st_size, PROT_READ, MAP_SHARED, desc, 0);
            if (desc_array[desc_cur].ptr == MAP_FAILED)
            {
                log_e("%s", strerror(errno));
                assert(0);
            }

            close(desc);
        }
        else
        {
            desc_array[desc_cur].type = USE_DESC;
            desc_array[desc_cur].desc = desc;
        }
        desc_array[desc_cur].size = stat_buf.st_size;

        snprintf(__buf, sizeof(__buf), "%s/%s", addition, ep->d_name);
        log_w("to hash %s", __buf + 1);
        hashmap_put(descriptors, __buf + 1 /* TODO change */, desc_array + desc_cur);
    }

    closedir(dp);
    free(fulldir);
}

void
http_start_static_server(char *filedir, char *default_file)
{
    atexit(http_static_release);

    dir = strdup(filedir);
    dfile = (default_file) ? strdup(default_file) : strdup("index.html");

    descriptors = hashmap_new();
    extensions = http_static_extentions();

    desc_array = calloc(MAX_FILES, sizeof(*desc_array));

    http_static_loader(filedir, "");

    log_w("start server: %s", dir);
    http_start_server(read_parser);
}
