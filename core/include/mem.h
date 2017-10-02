#ifndef MAIN_MEMORY_H
#define MAIN_MEMORY_H

#define STRSZ(str) str, sizeof(str) - 1
#define STRLN(str) str, strlen(str)

#define SZ(str) sizeof(str) - 1

#define KiB * 1024

#define STATICSZ(array) sizeof(array) / sizeof(*array)
#define FOREACH_STATIC(array, index) for (index = 0; index < STATICSZ(array); index++)

#define MEM_ERROR_P "[MEM ERROR]"
#define xassert(expr, text) do {                                        \
        if(!(expr))                                                     \
        {                                                               \
            printf("\n");                                               \
            printf("%s line: %d\n", text, __LINE__);          \
            assert(0);                                                  \
        }                                                               \
    } while(0);

typedef struct string
{
    int32_t str_len;
    char *str;
} string_t;

void
string_init(string_t *string, char *str, int32_t str_len);
void
string_release(string_t *str);

void *
xmemdup(void *src, size_t size);

void
xmemmove(void *dst, void *src, size_t size);

void *
xmalloc(size_t size);

#endif //MAIN_MEMORY_H
