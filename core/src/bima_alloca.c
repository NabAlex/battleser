#include "core.h"

int has_initialized = 0;

void *managed_memory_start;
void *last_valid_address;

/* POST ~ 1000 and GET */
void
bima_alloca_init()
{
    last_valid_address = sbrk(0);
    managed_memory_start = 0;

    has_initialized = 1;
}

void *
bima_malloc(size_t size)
{

}

void
bima_free(void *ptr)
{

}