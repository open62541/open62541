SET(SOURCE_GROUP ${SOURCE_GROUP}\\arduino)

ua_add_architecture("arduino")

list (FIND UA_AMALGAMATION_ARCHITECUTRES "arduino" _index)
if (${_index} GREATER -1 OR "${UA_ARCHITECTURE}" STREQUAL "arduino")

    ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_clock.c)
    ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_architecture_functions.c)

    if("${UA_ARCHITECTURE}" STREQUAL "arduino")
        ua_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
    endif()

endif()
