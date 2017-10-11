#ifndef MAIN_LIB_AIO_H
#define MAIN_LIB_AIO_H

typedef struct bima_aio
{
    int32_t id;
} bima_aio_t;

int32_t bima_aio_get_ctx(int32_t epoll_fd);
int32_t bima_aio_write(int cn_fd, char *buf, size_t buf_len);

#endif //MAIN_LIB_AIO_H
