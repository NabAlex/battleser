#ifndef MAIN_BIMA_FORKER_H_H
#define MAIN_BIMA_FORKER_H_H

typedef struct bfork_info
{
    int32_t fork_number;
} bfork_info_t;

bfork_info_t *
bima_fork_info();

int32_t
bima_fork_id();
bool
bima_fork_is_main();

void
bima_fork_init(int32_t forks);

#endif //MAIN_BIMA_FORKER_H_H
