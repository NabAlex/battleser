#include "core.h"

#ifdef _NEED_BATTLESER_MAIN_

static int32_t
print_usage()
{
    log_w("Usage [dir_name]");
    return 0;
}

int
main(int argc, char **argv)
{
    const unsigned int NEED_PARAMS = 2;
    if (argc < NEED_PARAMS)
        return print_usage();

    http_start_static_server(argv[1], NULL);
}

#endif