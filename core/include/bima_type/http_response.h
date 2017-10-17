#ifndef MAIN_HTTP_RESPONSE_H
#define MAIN_HTTP_RESPONSE_H

typedef enum http_status
{
    HTTP_OK                 = 200,
    HTTP_FAIL               = 400,
    HTTP_NOTFOUND           = 404,
    HTTP_METHOD_NOT_ALLOWED = 405,
    HTTP_ERR                = 500
} http_status_t;

char *
http_response_get_status_name(http_status_t st);

int32_t
http_send_data(conn_t *cn, http_status_t status, char *data, size_t data_len);

int32_t
http_send_file(conn_t *cn, http_status_t status, int32_t in_fd, size_t len);

#endif //MAIN_HTTP_RESPONSE_H
