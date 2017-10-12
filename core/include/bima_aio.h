#ifndef MAIN_LIB_AIO_H
#define MAIN_LIB_AIO_H

typedef struct bima_aio_context
{
    bool active;

    int32_t fd;
    io_context_t ctx;
} bima_aio_context_t;

int32_t
bima_aio_init(int (*aio_attach_context)(bima_aio_context_t *ctx));

int32_t
bima_aio_get_events(bima_aio_context_t *aio, struct io_event **events_out);

int32_t
bima_aio_write(int cn_fd, char *buf, size_t buf_len, void *save_ptr);

#endif //MAIN_LIB_AIO_H
