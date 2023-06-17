SET(SOURCE_GROUP ${SOURCE_GROUP}\\freertosLWIP)

ua_add_architecture("freertosLWIP")

list (FIND UA_AMALGAMATION_ARCHITECTURES "freertosLWIP" _index)
if (${_index} GREATER -1 OR "${UA_ARCHITECTURE}" STREQUAL "freertosLWIP")

    ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_clock.c)

    if("${UA_ARCHITECTURE}" STREQUAL "freertosLWIP")
        ua_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
        ua_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../common)
    endif()
endif()
