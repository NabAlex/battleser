#ifndef MAIN_BIMA_ALLOCA_H
#define MAIN_BIMA_ALLOCA_H

typedef struct bima_chain
{
    bool is_use;
    struct bima_chain *next;

    size_t size;
    char data[];
} bima_chain_t;

typedef bima_chain_t * bchain;

/* last_chain ~ NULL => new object */
bima_chain_t *
bima_chain_add(bima_chain_t *root);
void
bima_chain_release(bima_chain_t *chain);

void
bima_chain_alloca_init(size_t count, size_t default_size);


#endif //MAIN_BIMA_ALLOCA_H
