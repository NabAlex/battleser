#ifndef CORE_H_H
#define CORE_H_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include<ctype.h>

#include <signal.h>
#include <dirent.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/epoll.h>

#include "util.h"

/* if need http server */
#include <linux/aio_abi.h>

/* libraries */
#include "lib_aio.h"
/*************/

#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <unistd.h>

#define RES_OK      0
#define RES_ERR     1
#define RES_FAIL    2
#define RES_AGAIN   3

/*** local std ***/
#include "hashmap.h"
#include "mem.h"
#include "log.h"
/*** local std ***/

#include "bima.h"
#include "bima_type/http.h"
#include "bima_type/http_response.h"
#include "bima_type/http_static.h"

#endif // CORE_H_H
