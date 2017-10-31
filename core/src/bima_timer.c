#include "core.h"

#define TIMER_INIT_COUNT 200

timer_data_t *timers;

static void
timer_release()
{
    log_w("start release timer");
    for (int32_t i = 0; i < TIMER_INIT_COUNT; i++)
        close(timers[i].dc);
    free(timers);
}

void
timer_init()
{
    timers = calloc(TIMER_INIT_COUNT, sizeof(*timers));
    for (int32_t i = 0; i < TIMER_INIT_COUNT; i++)
    {
        timers[i].dc = timerfd_create(CLOCK_MONOTONIC, 0);
        // log_d("Create timer %d", timers[i].dc);
    }

    atexit(timer_release);
}

static timer_data_t *
timer_get_next_robin()
{
    static int32_t aio_poll_cur = 0;
    for (int32_t i = aio_poll_cur;;)
    {
        if (!timers[i].active)
        {
            timers[i].active = true;
            aio_poll_cur = (i + 1) % TIMER_INIT_COUNT;
            return &timers[i];
        }

        i = (i + 1) % TIMER_INIT_COUNT;
        if (i == aio_poll_cur)
            return NULL;
    }
}

void
timer_add(void *data, timer_handler_t callback, size_t mc)
{
    static struct itimerspec ts;

    timer_data_t *timer;
    assert((timer = timer_get_next_robin()) != NULL);

    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = 0;
    ts.it_value.tv_sec = mc / 1000;
    ts.it_value.tv_nsec = (mc % 1000) * 1000000;

    if (timerfd_settime(timer->dc, 0, &ts, NULL) < 0) {
        xassert(0, "cannot set timer");
        return;
    }

    timer->data = data;
    timer->handler = callback;

    if (bima_add_event_with_ptr(timer->dc, EPOLLIN | EPOLLET, BIMA_TIMER, timer) != RES_OK)
        log_e("cannot add timer in epoll");
}
