#include "core.h"

static char __buf[1 KiB];
static int32_t __buf_len;

static char *
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
    default:
        return "UNKNOWN";
    }
}

int32_t
http_send_data(conn_t *cn, http_status_t status)
{
    __buf_len = snprintf(__buf, sizeof(__buf),
            "HTTP/1.1 %d %s\r\n"
            "Server: bima\r\n"
            "Content-Length: 5\r\n"
            "\r\n"
            "Error",
            status, http_response_get_status_name(status));

    xassert(__buf_len > 0, "buffer is too short");
    bima_write(cn, __buf, __buf_len);
    return RES_OK;
}

int32_t
http_send_file(conn_t *cn, http_status_t status, int32_t in_fd)
{
    struct stat stat_buf;

    fstat(in_fd, &stat_buf);

    __buf_len = snprintf(__buf, sizeof(__buf),
            "HTTP/1.1 %d %s\r\n"
            "Server: bima\r\n"
            "Content-Length: %ld\r\n\r\n",
            status, http_response_get_status_name(status),
            stat_buf.st_size);

    assert(bima_write(cn, __buf, __buf_len) == RES_OK);
    assert(bima_write_descriptor(cn, in_fd, stat_buf.st_size) == RES_OK);
    return RES_OK;
}
