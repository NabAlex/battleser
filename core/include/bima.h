#ifndef BIMA_H
#define BIMA_H

#define MOD_BIMA "[BIMA] "

#define CHAIN_SIZE          1 KiB

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
    BIMA_USER,
    BIMA_AIO_CTX,
    BIMA_TIMER,
} bima_epoll_type_t;

typedef struct conn
{
    /**
     *  id ~            unique id
     */
    bima_epoll_type_t type;
    void *ptr;

    int32_t id;

    /**
     * set value if get_connection
     */
    int32_t fd;

    void *lib_space;
    client_t client_info;

    map_t store_map;

    bchain root;

    bchain in;
    size_t in_len;
} conn_t;

typedef int32_t (*reader_callback)(conn_t *cn, bchain chain);
typedef int32_t (*response_callback)(conn_t *cn);

#if defined(__cplusplus)
extern "C" {
#endif

int
bima_init(response_callback callback);

/* enter in epoll */
void
bima_main_loop();

void
bima_set_reader(reader_callback _reader);

int32_t
bima_write_and_close(conn_t *cn, char *buf, size_t buf_len);

int
bima_add_event_with_ptr(int _fd, uint32_t events, bima_epoll_type_t type, void *ptr);
int32_t
bima_remove_event_with_ptr(int _fd);

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
