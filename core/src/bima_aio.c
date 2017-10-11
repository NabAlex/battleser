#include "core.h"

#define BIMA_AIO_EVENTS 100

static int32_t aio_fd       = -1;
static int32_t epoll_fd     = -1;

static io_context_t context = 0;

#include <sys/eventfd.h>
#include <sys/ioctl.h>

static void
bima_aio_close(void)
{
    if (context)
        io_destroy(context);
}

int32_t bima_aio_get_ctx(int32_t _epoll_fd)
{
    epoll_fd = _epoll_fd;
    aio_fd = eventfd(0, 0);
    if (aio_fd == -1)
        goto error;

    int n = 1;
    if (ioctl(aio_fd, FIONBIO, &n) == -1)
        goto error;

    memset(&context, 0, sizeof(context));
    if (io_setup(BIMA_AIO_EVENTS, &context) != 0)
        goto error;

    log_w("create ctx descriptor %d", aio_fd);

    return aio_fd;
error:
    log_e("cannot create aio context");
    return -1;
}

int32_t bima_aio_write(int cn_fd, char *buf, size_t buf_len)
{
    atexit(bima_aio_close);
    if (!context || aio_fd < 0 || epoll_fd < 0)
        return RES_ERR;

    int32_t r;
    struct iocb cb;
    struct iocb *cbs[] = { &cb };

    io_prep_pwrite(&cb, cn_fd, buf, buf_len, 0);
    bima_aio_t *tmp = calloc(1, sizeof(bima_aio_t));
    tmp->id = cn_fd;
    cb.data = tmp;

    io_set_eventfd(&cb, aio_fd);

    // TODO cb.data
    // TODO 512 bytes http://nginx.org/ru/docs/http/ngx_http_core_module.html#aio

    if ((r = io_submit(context, 1, cbs)) != 1)
    {
        if (r < 0)
            log_e("wrong context!");
        else
            log_e("io_submit fail");

        return RES_ERR;
    }

    return RES_OK;
}
