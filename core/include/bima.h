#ifndef BIMA_H
#define BIMA_H

#define MOD_BIMA "[BIMA] "

#define BIMA_USE_SIMPLE  0s
#define BIMA_USE_THREADS (1 << 0)

typedef struct client
{
    struct sockaddr uaddr;
    void *udata;
} client_t;

typedef int32_t (*read_parser_t)(string_t stream);

typedef struct conn
{
    /**
     *  id ~            unique id
     */
    int32_t id;
    int32_t fd;

    char *out;
    ssize_t out_len;

    void *lib_space;
    client_t client_info;
} conn_t;

typedef int32_t (*response_callback)(conn_t *cn);

#if defined(__cplusplus)
extern "C" {
#endif

int
bima_init(response_callback callback);

void
bima_main_loop();

int32_t
bima_write(conn_t *cn, char *buf, size_t buf_len);

void
bima_connection_close(conn_t *cn);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif //UNTITLED1_BIMA_H
