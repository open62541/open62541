SET(SOURCE_GROUP ${SOURCE_GROUP}\\win32)

ua_add_architecture("win32")
SET(FIND_LIBRARY_USE_LIB64_PATHS TRUE)

list (FIND UA_AMALGAMATION_ARCHITECTURES "win32" _index)
if (${_index} GREATER -1 OR "${UA_ARCHITECTURE}" STREQUAL "win32")

    # set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    # set(THREADS_PREFER_PTHREAD_FLAG ON)
    # find_package(Threads REQUIRED)

    ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_clock.c)
    ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_architecture_functions.c)

    if("${UA_ARCHITECTURE}" STREQUAL "win32")
        find_library(WinSock REQUIRED)
        ua_include_directories(${WINSOCK_INCLUDE_DIRS})
        ua_architecture_append_to_library(${WINSOCK_LIBRARIES})
        ua_architecture_append_to_library(iphlpapi)
        # ua_architecture_append_to_library(${CMAKE_THREAD_LIBS_INIT})
	#append iphlpapi permanently. Problem: PUBSUB Flag is configured after execution.
	#if(UA_ENABLE_DISCOVERY_MULTICAST OR UA_ENABLE_PUBSUB)
        #    ua_architecture_append_to_library(iphlpapi)
        #endif()
    endif()

    ua_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

endif()
