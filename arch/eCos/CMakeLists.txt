SET(SOURCE_GROUP ${SOURCE_GROUP}\\eCos)

ua_add_architecture("eCos")


list (FIND UA_AMALGAMATION_ARCHITECTURES "eCos" _index)
if (${_index} GREATER -1 OR "${UA_ARCHITECTURE}" STREQUAL "eCos")

    ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_clock.c)
    ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_architecture_functions.c)

    if("${UA_ARCHITECTURE}" STREQUAL "eCos")

        ua_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
        
        set(UA_ECOS_HOSTNAME "eCos-host" CACHE STRING "Define a custom hostname/ip for the endpoint.")
        ua_architecture_add_definitions(-DUA_ECOS_HOSTNAME="${UA_ECOS_HOSTNAME}")
    endif()

endif()
