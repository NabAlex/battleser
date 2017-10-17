#include "core.h"

#define BIMA_AIO_POLL_SIZE 30

#define NUM_EVENTS 128

#include <sys/ioctl.h>
#include <syscall.h>

bima_aio_context_t **aio_poll_context;

static bima_aio_context_t *
bima_aio_context_init(int32_t _fd, io_context_t _ctx)
{
    bima_aio_context_t *aio = calloc(1, sizeof(*aio));
    aio->ctx = _ctx;
    aio->fd = _fd;
    aio->active = false;
    return aio;
}

/* long operation */
static void
bima_aio_context_release(bima_aio_context_t *ctx)
{
    if (ctx->ctx)
        io_destroy(ctx->ctx);
    close(ctx->fd);
    free(ctx);
}

static bima_aio_context_t *
bima_aio_context_get_new()
{
    int32_t aio_fd;
    io_context_t context = 0;
    if (io_setup(100, &context) != 0)
        goto error;

    if ((aio_fd = syscall(SYS_eventfd, 0)) == -1)
        goto error;

    int n = 1;
    if (ioctl(aio_fd, FIONBIO, &n) == -1)
        goto error;

    log_w("create ctx descriptor %d", aio_fd);
    return bima_aio_context_init(aio_fd, context);
error:
    log_e("cannot create aio context");
    return NULL;
}

static void
bima_aio_close(void)
{
    log_w("aio: start close");
    for (int32_t i = 0; i < BIMA_AIO_POLL_SIZE; i++)
        bima_aio_context_release(aio_poll_context[i]);
    free(aio_poll_context);
}

int32_t
bima_aio_init(int32_t (*aio_attach_context)(bima_aio_context_t *ctx))
{
    aio_poll_context = calloc(BIMA_AIO_POLL_SIZE, sizeof(*aio_poll_context));
    for (int32_t i = 0; i < BIMA_AIO_POLL_SIZE; i++)
    {
        if ((aio_poll_context[i] = bima_aio_context_get_new()) == NULL)
        {
            log_e("aio: cannot create context");
            goto err;
        }
        xassert(aio_attach_context(aio_poll_context[i]) == RES_OK, "cannot attach aio");
    }

    atexit(bima_aio_close);
    return RES_OK;
err:
    for (int32_t i = 0; i < BIMA_AIO_POLL_SIZE; i++)
        bima_aio_context_release(aio_poll_context[i]);
    free(aio_poll_context);
    return RES_ERR;
}

int32_t
bima_aio_get_events(bima_aio_context_t *aio, struct io_event **events_out)
{
    /* todo check max num */
    static struct io_event aio_events[NUM_EVENTS];
    int32_t aio_event_count = 0;

    ssize_t r;
    u_int64_t eval = 0;

    for (;;)
    {
        r = read(aio->fd, &eval, sizeof(eval));
        if (r != sizeof(eval))
        {
            if (r == -1 && errno == EAGAIN)
                break;

            log_e("aio: cannot read count");
            return -1;
        }

        log_d("count events %"PRIu64, eval);
        for (aio_event_count = 0; aio_event_count < eval; ++aio_event_count)
        {
            if (io_getevents(aio->ctx, 1, 1, &aio_events[aio_event_count], NULL) != 1) {
                log_e("io_getevents error");
                assert(0);
                return -1;
            }
        }
    }

    *events_out = aio_events;
    return aio_event_count;
}

static bima_aio_context_t *
bima_aio_get_next_robin()
{
    static int32_t aio_poll_cur = 0;
    for (int32_t i = aio_poll_cur;;)
    {
        if (!aio_poll_context[i]->active)
        {
            aio_poll_context[i]->active = true;
            aio_poll_cur = (i + 1) % BIMA_AIO_POLL_SIZE;
            return aio_poll_context[i];
        }

        i = (i + 1) % BIMA_AIO_POLL_SIZE;
        if (i == aio_poll_cur)
            return NULL;
    }
}

int32_t
bima_aio_write(int cn_fd, char *buf, size_t buf_len, void *save_ptr)
{
#ifdef DEBUG
    if (buf_len >= 512 && buf_len % 512)
        log_w("wrong use aio");
#endif
    bima_aio_context_t *current = bima_aio_get_next_robin();
    log_w("aio: choose %d", current->fd);
    xassert(current, "test!");

    int32_t r;
    struct iocb cb;
    struct iocb *cbs[] = { &cb };

    io_prep_pwrite(&cb, cn_fd, buf, buf_len, 0);
    cb.data = save_ptr;

    io_set_eventfd(&cb, current->fd);

    // TODO cb.data
    // TODO 512 bytes http://nginx.org/ru/docs/http/ngx_http_core_module.html#aio

    if ((r = io_submit(current->ctx, 1, cbs)) != 1)
    {
        if (r < 0)
            log_e("wrong context!");
        else
            log_e("io_submit fail");

        return RES_ERR;
    }

    return RES_OK;
}
