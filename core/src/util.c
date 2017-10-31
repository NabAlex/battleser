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

const char *
normalize_directory(char *dir, size_t dir_len)
{
    static char buf[1 KiB];
    char *buf_end = buf + sizeof(buf);
    int32_t buf_len = 0, lost_count = 0;

    char *left_name = dir + dir_len, *right_name = dir + dir_len;

#define ADD_STRING(str, len) do { memcpy(buf_end - buf_len - (len) - 1, (str), (len)); buf[sizeof(buf) - buf_len - 1] = '/'; buf_len += (len) + 1; } while(0)

    do
    {
        while (--left_name >= dir && *left_name != '/');
        if (left_name < dir)
            left_name = dir - 1;

        if (right_name == left_name)
            ADD_STRING("/", 1);
        else if (right_name - left_name == sizeof("..") && *(left_name + 1)== '.' && *(left_name + 2) == '.')
            lost_count += 1;
        else if (lost_count > 0)
            lost_count -= 1;
        else
            ADD_STRING(left_name + 1, right_name - left_name - 1);

        if (left_name <= dir)
            break;

        right_name = left_name;
    } while (true);

    buf[sizeof(buf) - 1] = '\0';
    return buf_end - buf_len;
}