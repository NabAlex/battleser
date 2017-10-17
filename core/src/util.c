#include "core.h"

char *strnstr(const char *haystack, const char *needle, size_t len)
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

char *url_encode(char *buf, size_t buf_len)
{
    return NULL;
}

char *url_decode(char *buf, size_t buf_len)
{
    return NULL;
}