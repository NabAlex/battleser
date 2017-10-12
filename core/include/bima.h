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

#define CONNECT_UNUSED      -1
#define CONNECT_DONE        -2

typedef enum bima_epoll_type
{
    BIMA_AIO_CTX
} bima_epoll_type_t;

typedef struct bima_epoll_data
{
    bima_epoll_type_t type;
    void *alloc_ptr;
} bima_epoll_data_t;

typedef struct conn
{
    /**
     *  id ~            unique id
     */
    int32_t id;
    int32_t fd;

    bima_epoll_data_t *epoll_ptr;

    char *out;
    ssize_t out_len;

    void *lib_space;
    client_t client_info;

    char *add_buf;
    ssize_t add_buf_len;
} conn_t;

typedef int32_t (*reader_callback)(conn_t *cn, const char *buf, uint32_t buf_len);
typedef int32_t (*response_callback)(conn_t *cn);

#if defined(__cplusplus)
extern "C" {
#endif

int
bima_init(response_callback callback);

void
bima_set_reader(reader_callback _reader);

void
bima_main_loop();

static int
bima_add_event(int _fd, uint32_t events);
int
bima_remove_event(int _fd);
int
bima_change_event(int _fd, uint32_t events);

int32_t
bima_write(conn_t *cn, char *buf, size_t buf_len);

int32_t
bima_write_descriptor(conn_t *cn, int32_t dscr, size_t dscr_size);

void
bima_connection_close(conn_t *cn);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif //UNTITLED1_BIMA_H
