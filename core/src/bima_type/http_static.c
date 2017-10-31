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

    /* stat */
    int32_t errors;
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
http_static_closer(any_t *ignore, file_info_t *info)
{
    switch (info->type)
    {
        case USE_DESC:
            close(info->desc);
        case USE_MMAP:
            munmap(info->ptr, info->size);
    }
}

void
http_static_release(void)
{
    log_w("delete static dir");
    free(dir);
    free(dfile);
    free(desc_array);

    hashmap_iterate(descriptors, (PFany) http_static_closer, NULL);
    hashmap_free(descriptors);
}

static int32_t
read_parser(conn_t *cn, http_request_t *request)
{
    if (strcmp(request->head.type, "HEAD") && strcmp(request->head.type, "GET"))
        return http_send_data(cn, HTTP_METHOD_NOT_ALLOWED, STRSZ("Method not allowed"));

    bool index_html = false;
    char *filename;
    size_t filename_len = 0;

    filename = request->head.url + 1;
    if (!*filename)
        filename = dfile;

    char *url_end;
    if ((url_end = strstr(filename, "?")) != NULL)
        *url_end = '\0';
    else
        url_end = filename + strlen(filename);

    filename_len = url_end - filename;

    if (*(url_end - 1) == '/')
    {
        __buf_len = snprintf(__buf, sizeof(__buf), "%s%s",
            filename,
            dfile);
        assert(__buf_len > 0);
        filename = __buf;
        filename_len = __buf_len;
        index_html = true;
    }

    filename = (char *) normalize_directory(filename, filename_len);
    log_d("%s", filename);

    file_info_t *info = NULL;
    int32_t r = hashmap_get(descriptors, filename, &info);
    if (r != MAP_OK)
        return http_send_data(cn, (!index_html) ? HTTP_NOTFOUND : HTTP_FORBIDDEN, STRSZ("Not found"));


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
    {
        if ((r = bima_aio_write(cn->fd, info->ptr, info->size, cn)) != RES_OK)
            info->errors++;

        return r;
    }

    if (info->type == USE_DESC)
    {
        if ((r = http_send_file(cn, HTTP_OK, info->desc, info->size)) != RES_OK)
            info->errors++;

        return r;
    }

    xassert(0, "wrong static answer");
    return RES_ERR;
}

static void
http_static_timer_check(file_info_t *info)
{
    xassert(info->errors == 0, "file error");
    timer_add(info,
            (timer_handler_t) http_static_timer_check,
            3 SECONDS);
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

#ifdef DEBUG
        timer_add(&desc_array[desc_cur],
                (timer_handler_t) http_static_timer_check,
                rand() % 4 SECONDS);
#endif
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
    http_start_server(read_parser);

    atexit(http_static_release);

    dir = strdup(filedir);
    dfile = (default_file) ? strdup(default_file) : strdup("index.html");

    descriptors = hashmap_new();
    extensions = http_static_extentions();

    desc_array = calloc(MAX_FILES, sizeof(*desc_array));

    http_static_loader(filedir, "");
    log_w("start server: %s:3000", dir);
}
