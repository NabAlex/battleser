#include "core.h"

http_callback http_local_callback;

enum http_state_impl { PARSE_URL, PARSE_HEADERS, PARSE_BODY };
typedef struct http_struct_impl
{
    enum http_state_impl state;
    int32_t last_len;
} http_struct_impl;

static inline bool
http_is_line_delim(char ch)
{
    return ch == '\n'  ||
           ch == '\r';
}

/* delete '\r' ^_^ */
#define NEXT_LINE_AND_SET_NULL(ptr, ptr_end)                                          \
        while ( (ptr < ptr_end) && !http_is_line_delim( *(ptr++)) );                \
        *(ptr - 1) = '\0';

#define NEXT_CHR(ptr, ptr_end, chr)                                                 \
        while ( (ptr < ptr_end) && (*(ptr++) != chr) );                             \
        --ptr;

#define NEXT_CHR_AND_SET_NULL(ptr, ptr_end, chr)                                    \
        while ( (ptr < ptr_end) && (*(ptr++) != chr) );                             \
        *(ptr - 1) = '\0';

#define NEXT_TRIM(ptr, ptr_end, chr)                                                \
        while ( (ptr < ptr_end) && (*(ptr++) == chr) );                             \
        --ptr;

#define NEXT_LINE(ptr, ptr_end)                                                     \
        if ( (ptr < ptr_end) && (*ptr == '\n') ) ++ptr;                             \
        else ptr = NULL;

#define IS_NEWLINE(ptr, ptr_end)                                                    \
        ( (ptr < ptr_end + 1) && (*ptr == '\r') && ptr++ && (*(ptr++) == '\n') )

static int32_t
http_parser(conn_t *cn)
{
    int32_t res = RES_OK;
    if (!cn || !cn->root)
        return RES_ERR;

    static http_request_t request;

    char *ptr = cn->root->data;
    char *ptr_end = ptr + cn->in_len;

    request.head.type = ptr;
    NEXT_CHR_AND_SET_NULL(ptr, ptr_end, ' ');

    request.head.url = ptr;
    NEXT_CHR_AND_SET_NULL(ptr, ptr_end, ' ');

    url_decode_me(request.head.url);

    request.head.http_version = ptr;
    NEXT_LINE_AND_SET_NULL(ptr, ptr_end);

    NEXT_LINE(ptr, ptr_end);
    if (!ptr || IS_NEWLINE(ptr, ptr_end))
        goto out;

    char *key, *value, *tmp;
    for (;;) {
        if (ptr >= ptr_end)
            break;

        key = tmp = ptr;

        NEXT_CHR(ptr, ptr_end, ':');
        *(ptr++) = '\0';

        NEXT_TRIM(ptr, ptr_end, ' ');

        value = ptr;
        NEXT_LINE_AND_SET_NULL(ptr, ptr_end);

        xassert(key && *key, "wtf?");
        xassert(value && *value, "wtf?");
        do
            *tmp = toupper(*tmp);
        while (*(++tmp));

        hashmap_put_with_prefix(cn->store_map, "HEAD", key, value);

        NEXT_LINE(ptr, ptr_end);
        if (!ptr || IS_NEWLINE(ptr, ptr_end))
            break;
    }

    request.body = ptr;
    request.body_len = ptr_end - ptr;
out:
    if (res != RES_OK)
        return RES_ERR;

    return http_local_callback(cn, &request);
}

int32_t
http_reader(conn_t *cn, bchain chain)
{
    return (strnstr(chain->data, "\r\n\r\n", CHAIN_SIZE)) ? RES_OK : RES_AGAIN;
}

void
http_start_server(http_callback callback)
{
    http_local_callback = callback;

    bima_init(http_parser);
    bima_set_reader(http_reader);
}