#include "core.h"

static void *start_memory, *end_memory;
static size_t chain_count;

bima_chain_t **memmap;

static size_t robin_cursor;

static bima_chain_t *
bima_chain_get()
{
    /* robin */
    size_t check_loop = 0;
    do
    {
        while (robin_cursor < chain_count)
        {
            if (!memmap[robin_cursor]->is_use)
            {
                memmap[robin_cursor]->is_use = true;
                return memmap[robin_cursor++];
            }

            robin_cursor++;
        }

        robin_cursor = 0;
        ++check_loop;
        if (check_loop > 1)
            /* this is loop */
            return NULL;
    } while (1);
}

bima_chain_t *
bima_chain_add(bima_chain_t *root)
{
    bima_chain_t *chain = bima_chain_get();
    if (root)
    {
        bima_chain_t *tmp = root;
        do {
            tmp->size += chain->size;
            if (!tmp->next)
                break;

            tmp = tmp->next;
        } while (1);

        tmp->next = chain;
    }

    return chain;
}

void
bima_chain_release(bima_chain_t *chain)
{
    if (!chain)
        return;

    bima_chain_t *tmp;
    while (chain)
    {
        tmp = chain->next;
        chain->is_use = false;
        chain->next = NULL;
        chain = tmp;
    }
}

/* POST ~ 1000 and GET */
void
bima_chain_alloca_init(size_t count, size_t default_size)
{
    log_w("start alloc bima chain memory");
    start_memory = sbrk(0);
    void *cursor = start_memory;

    size_t sc = sizeof(bima_chain_t) + default_size;
    end_memory = sbrk(sc * count);
    // TODO check end_memory

    memmap = calloc(count, sizeof(*memmap));
    for (int32_t i = 0; i < count; ++i)
    {
        memset(cursor, 0, sc);

        memmap[i] = cursor;
        memmap[i]->size = default_size;
        cursor += sc;
    }

    chain_count = count;
    robin_cursor = 0;
}
