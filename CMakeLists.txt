cmake_minimum_required(VERSION 3.6)
project(main)

add_definitions(-DDEBUG=1)

set(CMAKE_C_STANDARD 11)

MACRO(ADD_PROJECT project_name)
    include_directories(${project_name})
    include_directories(${project_name}/include)
    add_subdirectory(${project_name})
ENDMACRO()

# add projects
ADD_PROJECT(core)

# add target project
set(TARGET_PROJECT battleser)
add_definitions(-D_NEED_BATTLESER_MAIN_=1)

ADD_PROJECT(${TARGET_PROJECT})

include_directories(${TARGET_PROJECT})
add_executable(main ${TARGET_PROJECT}/src/${TARGET_PROJECT}.c)

# extern all libraries
target_link_libraries(main core)
