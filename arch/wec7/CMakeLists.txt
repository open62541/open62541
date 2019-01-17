SET(SOURCE_GROUP ${SOURCE_GROUP}\\wec7)

ua_add_architecture("wec7")

list (FIND UA_AMALGAMATION_ARCHITECTURES "wec7" _index)
if (${_index} GREATER -1 OR "${UA_ARCHITECTURE}" STREQUAL "wec7")

    ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_clock.c)
    ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_architecture_functions.c)

    if("${UA_ARCHITECTURE}" STREQUAL "wec7")
        ua_architecture_append_to_library(ws2)
        ua_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

    endif()

endif()
