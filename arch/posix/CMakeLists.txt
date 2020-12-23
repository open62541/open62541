SET(SOURCE_GROUP ${SOURCE_GROUP}\\posix)

ua_add_architecture("posix")

list (FIND UA_AMALGAMATION_ARCHITECTURES "posix" _index)
if (${_index} GREATER -1 OR "${UA_ARCHITECTURE}" STREQUAL "posix")

    ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_clock.c)
    ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_architecture_functions.c)


    if("${UA_ARCHITECTURE}" STREQUAL "posix")
        if(NOT CYGWIN AND NOT QNXNTO AND (NOT ${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD"))
            ua_architecture_add_definitions(-Wshadow -Wconversion -fvisibility=hidden -fPIC)
        endif()

        if(QNXNTO)
          ua_architecture_append_to_library(socket c stdc++)
        elseif(OS9)
          ua_architecture_append_to_library(netdb ndblib socket)
        else()
          ua_architecture_append_to_library(m)
          #TODO - Error on first make run if pthread is included conditional?
          ua_architecture_append_to_library(pthread)
          if(UA_ENABLE_MULTITHREADING OR UA_BUILD_UNIT_TESTS)
            ua_architecture_append_to_library(pthread)
          endif()
          if(NOT APPLE AND (NOT ${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD"))
            ua_architecture_append_to_library(rt)
          endif()
        endif()

    endif()


endif()
