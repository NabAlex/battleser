#ifndef MAIN_HTTP_RESPONSE_H
#define MAIN_HTTP_RESPONSE_H

typedef enum http_status
{
    HTTP_OK         = 200,
    HTTP_FAIL       = 400,
    HTTP_NOTFOUND   = 404,
    HTTP_ERR        = 500
} http_status_t;

int32_t
http_send_data(conn_t *cn, http_status_t status);

int32_t
http_send_file(conn_t *cn, http_status_t status, int32_t in_fd);

#endif //MAIN_HTTP_RESPONSE_H
