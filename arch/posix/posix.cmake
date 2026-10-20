# Build settings for the POSIX architectures. Included after the build options and
# library lists have been initialized in the top-level CMakeLists.txt.

option(UA_ARCH_LOCAL_TIME_IS_UTC "Treat local time as UTC without timezone conversion" OFF)

set(UA_ARCHITECTURE_INCLUDE "arch/posix.h")
configure_file("${CMAKE_CURRENT_LIST_DIR}/posix.h.in"
               "${PROJECT_BINARY_DIR}/src_generated/open62541/${UA_ARCHITECTURE_INCLUDE}")

# posix-lwip shares the clock configuration but retains its own library settings.
if(NOT UA_ARCHITECTURE STREQUAL "posix")
    return()
endif()

list(APPEND open62541_LIBRARIES "m")
list(APPEND pkgcfg_libsprivate "-lm")
if(UA_MULTITHREADING GREATER_EQUAL 100 OR UA_BUILD_UNIT_TESTS AND NOT ANDROID_NDK_TOOLCHAIN_INCLUDED)
    list(APPEND open62541_PUBLIC_LIBRARIES "pthread")
    list(APPEND pkgcfg_libsprivate "-lpthread")
endif()
if(NOT APPLE AND (NOT ${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD") AND NOT ANDROID_NDK_TOOLCHAIN_INCLUDED)
    list(APPEND open62541_LIBRARIES "rt")
    list(APPEND pkgcfg_libsprivate "-lrt")
endif()
