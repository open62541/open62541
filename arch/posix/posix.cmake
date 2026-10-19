# Build settings for the POSIX architectures. Included after the build options and
# library lists have been initialized in the top-level CMakeLists.txt.

configure_file("${CMAKE_CURRENT_LIST_DIR}/posix.h.in"
               "${PROJECT_BINARY_DIR}/src_generated/arch/posix/posix.h")

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
