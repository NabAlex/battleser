#include "core.h"

bfork_info_t info;

bfork_info_t *
bima_fork_info() { return &info; }

int32_t
bima_fork_id() { return info.fork_number; }

bool
bima_fork_is_main() { return info.fork_number == 0; }

void
bima_fork_init(int32_t forks)
{
#ifndef DEBUG
    for (int32_t i = 1; i <= (forks - 1); i++)
        if (fork() == 0)
        {
            info.fork_number = i;
            log_w("Create fork %d", info.fork_number);
            break;
        }
#endif
}
