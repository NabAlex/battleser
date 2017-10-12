#ifndef MAIN_BIMA_ALLOCA_H
#define MAIN_BIMA_ALLOCA_H

void
bima_alloca_init();

void *
bima_malloc(size_t size);

void
bima_free(void *ptr);

#endif //MAIN_BIMA_ALLOCA_H
