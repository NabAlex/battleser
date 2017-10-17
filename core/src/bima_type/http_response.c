#include "core.h"

static char __buf[1 KiB];
static int32_t __buf_len;

char *
http_response_get_status_name(http_status_t st)
{
    switch (st)
    {
    case HTTP_OK:
        return "OK";
    case HTTP_ERR:
        return "ERR";
    case HTTP_FAIL:
        return "FAIL";
    case HTTP_NOTFOUND:
        return "NOT FOUND";
    case HTTP_METHOD_NOT_ALLOWED:
        return "METHOD NOT ALLOWED";
    default:
        return "UNKNOWN";
    }
}

int32_t
http_send_data(conn_t *cn, http_status_t status, char *data, size_t data_len)
{
    __buf_len = snprintf(__buf, sizeof(__buf),
            "HTTP/1.1 %d %s\r\n"
            "Server: bima\r\n"
            "Content-Length: %ld\r\n"
            "\r\n"
            "%.*s",
            status,
            http_response_get_status_name(status),
            data_len,
            data_len, data);

    xassert(__buf_len > 0, "buffer is too short");
    return bima_write_and_close(cn, __buf, __buf_len);
}

int32_t
http_send_file(conn_t *cn, http_status_t status, int32_t in_fd, size_t len)
{
    assert(bima_write_descriptor(cn, in_fd, len) == RES_OK);
    return RES_OK;
}
