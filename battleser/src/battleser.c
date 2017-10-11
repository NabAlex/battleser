#include "core.h"

#ifdef _NEED_BATTLESER_MAIN_

int main(int argc, char **argv)
{
    bool isCaseInsensitive = false;

    int32_t opt;

    enum { CHARACTER_MODE, WORD_MODE, LINE_MODE } mode = CHARACTER_MODE;

    while ((opt = getopt(argc, argv, "ilw")) != -1) {
        switch (opt) {
            case 'i': isCaseInsensitive = true; break;
            case 'l': mode = LINE_MODE; break;
            case 'w': mode = WORD_MODE; break;
            default:
                fprintf(stderr, "Usage: %s [-ilw] [file...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    http_start_static_server("/home/alex/CLionProjects/bfcom/battleser/static", NULL);
}

#endif