#ifndef MAIN_BIMA_TIMER_H
#define MAIN_BIMA_TIMER_H

#define SECONDS * 1000
#define MINUTE  * 1000 * 60
#define HOUR    * 1000 * 60 * 60

typedef void (*timer_handler_t)(void *);

typedef struct timer_data
{
    bool active;

    int32_t dc;
    void *data;
    timer_handler_t handler;
} timer_data_t;

void
timer_init();

void
timer_add(void *data, timer_handler_t callback, size_t mc);

#endif //MAIN_BIMA_TIMER_H
