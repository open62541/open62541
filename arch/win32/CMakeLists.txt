SET(SOURCE_GROUP ${SOURCE_GROUP}\\win32)

ua_add_architecture("win32")

list (FIND UA_AMALGAMATION_ARCHITECUTRES "win32" _index)
if (${_index} GREATER -1 OR "${UA_ARCHITECTURE}" STREQUAL "win32")

    ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_clock.c)
    ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_architecture_functions.c)

    if("${UA_ARCHITECTURE}" STREQUAL "win32")
        ua_architecture_append_to_library(ws2_32)

        if(UA_ENABLE_DISCOVERY_MULTICAST OR UA_ENABLE_PUBSUB)
            ua_architecture_append_to_library(iphlpapi)
        endif()
    endif()

    ua_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

endif()
