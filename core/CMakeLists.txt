project(core)

include_directories(include)

# libaio
include(cmake/libaio.cmake)
if (LIBAIO_FOUND)
    include_directories(${LIBAIO_INCLUDE_DIR})
    link_libraries(${LIBAIO_LIBRARIES})
else (LIBAIO_FOUND)
    message(WARNING "cannot find libaio")
endif (LIBAIO_FOUND)

add_library(core src/bima.c
                 src/log.c
                 src/mem.c
                 src/bima_type/http.c
                 src/hashmap.c
                 src/bima_type/http_response.c
                 src/bima_type/http_static.c
        src/bima_aio.c
                 src/util.c)
