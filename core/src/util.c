#include "core.h"

char *
strnstr(const char *haystack, const char *needle, size_t len)
{
    int i;
    size_t needle_len;

    if (0 == (needle_len = strnlen(needle, len)))
        return (char *)haystack;

    for (i=0; i<=(int)(len-needle_len); i++)
    {
        if ((haystack[0] == needle[0]) &&
            (0 == strncmp(haystack, needle, needle_len)))
            return (char *)haystack;

        haystack++;
    }
    return NULL;
}

void
url_decode_me(char *me)
{
    char *tmp = me, _1, _2;
    while (*tmp)
    {
        if (*tmp == '%' && (_1 = *(tmp + 1)) && (_2 = *(tmp + 2)))
        {
            char dig[] = { _1, _2, '\0' };
            char *err = NULL;

            long val = strtol(dig, &err, 16);
            if (err == dig + 2)
            {
                /* ok, read all */
                *me = (char) val;
                ++me;

                tmp += 3;
                continue;
            }
        }

        *me = *tmp;
        ++me;
        ++tmp;
    }

    *me = '\0';
}