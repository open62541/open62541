SET(SOURCE_GROUP ${SOURCE_GROUP}\\nano)

ua_add_architecture("nano")

if("${UA_ARCHITECTURE}" STREQUAL "nano")

    ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_clock.c)
    ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_architecture_functions.c)

    ua_architecture_add_definitions(-Wshadow -Wconversion -fvisibility=hidden -fPIC)
    ua_architecture_append_to_library(m)

    if(UA_MULTITHREADING OR UA_BUILD_UNIT_TESTS)
        ua_architecture_append_to_library(pthread)
    endif()

    ua_architecture_append_to_library(rt)

endif()
