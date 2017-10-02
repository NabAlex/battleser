#ifndef MAIN_HTTP_H
#define MAIN_HTTP_H

typedef enum http_protocol
{
    HTTP, HTTPS
} http_protocol_t;

typedef struct http_header
{
    char *type;
    char *url;
    char *http_version;

    map_t headers;
} http_header_t;

typedef struct http_response
{
    http_protocol_t protocol;
    http_header_t head;

    /* pointer on string, json, ... */
    char *body;
    size_t body_len;
} http_request_t;

http_request_t *
http_response_init();

void
http_response_release(http_request_t *response);

typedef int32_t (*http_callback)(conn_t *cn, http_request_t *response);

void
http_start_server(http_callback callback);

#endif //MAIN_HTTP_H
