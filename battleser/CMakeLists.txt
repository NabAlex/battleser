project(battleser)

set(MYNAME battleser)

unset(ADD_LIB_MAIN)

# module
if ((DEFINED TARGET_PROJECT) AND (TARGET_PROJECT STREQUAL battleser))
    # module run
    # add_definitions, please
    message("${MYNAME} is execute module!")
else()
    set(ADD_LIB_MAIN src/battleser.c)
endif()

if (DEFINED ADD_LIB_MAIN)
    if (NOT ((ADD_LIB_MAIN STREQUAL "") AND (OTHER_LIB_FILES STREQUAL "")))
        add_library(battleser ${ADD_LIB_MAIN} ${OTHER_LIB_FILES})
    else()
        message("${MYNAME} haven't files for library")
    endif()
endif()
