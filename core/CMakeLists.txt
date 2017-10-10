project(core)

include_directories(include)

add_library(core src/bima.c
                 src/log.c
                 src/mem.c
                 src/bima_type/http.c
                 src/hashmap.c
                 src/bima_type/http_response.c
                 src/bima_type/http_static.c
                 src/lib_aio.c
                 src/util.c)
