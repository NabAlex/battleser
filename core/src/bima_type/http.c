#include "core.h"

http_callback http_local_callback;

http_request_t *
http_response_init()
{
    http_request_t *request = xmalloc( sizeof(http_header_t) );
    memset(request, 0, sizeof(http_header_t));
    request->head.headers = hashmap_new();
    return request;
}

void
http_response_release(http_request_t *response)
{
    if (!response)
        return;

    /* TODO body release */
    int32_t map_len = hashmap_length(response->head.headers);

    // TODO pls delete element from map
    hashmap_free(response->head.headers);
    free(response);
}

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
    if (!cn || !cn->out_len)
        return RES_ERR;

    assert(cn->out);

    http_request_t *request = http_response_init();

    char *ptr = cn->out;
    char *ptr_end = cn->out + cn->out_len;

    request->head.type = cn->out;
    NEXT_CHR_AND_SET_NULL(ptr, ptr_end, ' ');

    request->head.url = ptr;
    NEXT_CHR_AND_SET_NULL(ptr, ptr_end, ' ');

    request->head.http_version = ptr;
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
        hashmap_put(request->head.headers, key, value);

        NEXT_LINE(ptr, ptr_end);
        if (!ptr || IS_NEWLINE(ptr, ptr_end))
            break;
    }

    request->body = ptr;
    request->body_len = (cn->out + cn->out_len) - ptr;
out:
    if (res != RES_OK)
        return RES_ERR;

    return http_local_callback(cn, request);
}

void
http_start_server(http_callback callback)
{
    http_local_callback = callback;

    bima_init(http_parser);
    bima_main_loop();
}