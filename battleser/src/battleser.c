#include "core.h"

static int32_t
print_usage()
{
    log_w("Usage [dir_name]");
    return 0;
}

#ifdef _NEED_BATTLESER_MAIN_

int32_t
main(int argc, char **argv)
{
    const unsigned int NEED_PARAMS = 2;
    if (argc < NEED_PARAMS)
        return print_usage();

    http_start_static_server(argv[1], NULL);
    bima_main_loop();
}

#endif