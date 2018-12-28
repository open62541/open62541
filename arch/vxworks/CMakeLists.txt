SET(SOURCE_GROUP ${SOURCE_GROUP}\\vxworks)

ua_add_architecture("vxworks")

list (FIND UA_AMALGAMATION_ARCHITECTURES "vxworks" _index)
if (${_index} GREATER -1 OR "${UA_ARCHITECTURE}" STREQUAL "vxworks")

    ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_clock.c)
    ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_architecture_functions.c)

    if("${UA_ARCHITECTURE}" STREQUAL "vxworks")

        ua_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

    endif()

endif()
