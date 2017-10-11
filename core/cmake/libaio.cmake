# https://github.com/weicao/cascadb
# - Try to find Libaio
# Once done, this will define
#
#  LIBAIO_FOUND - system has Libaio installed
#  LIBAIO_INCLUDE_DIR - the Libaio include directories
#  LIBAIO_LIBRARIES - link these to use Libaio
#
# The user may wish to set, in the CMake GUI or otherwise, this variable:
#  LIBAIO_DIR - path to start searching for the module

find_path(LIBAIO_INCLUDE_DIR
        libaio.h
        HINTS
        PATH_SUFFIXES
        include
        )

find_library(LIBAIO_LIBRARY
        aio
        HINTS
        PATH_SUFFIXES
        lib
        )

mark_as_advanced(LIBAIO_INCLUDE_DIR LIBAIO_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libaio
        DEFAULT_MSG
        LIBAIO_INCLUDE_DIR
        LIBAIO_LIBRARY)

if(LIBAIO_FOUND)
    set(LIBAIO_LIBRARIES "${LIBAIO_LIBRARY}") # Add any dependencies here
endif()
