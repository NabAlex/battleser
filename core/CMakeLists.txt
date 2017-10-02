project(core)

include_directories(include)

add_library(core src/bima.c
                 src/log.c
                 src/mem.c
                 src/bima_type/http.c include/hashmap.h src/hashmap.c include/bima_type/http_response.h src/bima_type/http_response.c include/bima_type/http_static.h src/bima_type/http_static.c)
