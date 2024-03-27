# This module tries to find libWebsockets library and include files
#
# LIBWEBSOCKETS_INCLUDE_DIR, path where to find libwebsockets.h
# LIBWEBSOCKETS_LIBRARY_DIR, path where to find libwebsockets.so
# LIBWEBSOCKETS_LIBRARIES, the library to link against
# LIBWEBSOCKETS_FOUND, If false, do not try to use libWebSockets
#
# This currently works probably only for Linux
set(FPHSA_NAME_MISMATCHED 1) # Suppress warnings, see https://cmake.org/cmake/help/v3.17/module/FindPackageHandleStandardArgs.html
include(FindPkgConfig)

pkg_search_module(LIBWEBSOCKETS libwebsockets)
if(LIBWEBSOCKETS_FOUND)
    message(STATUS "Got libwebsockets ${LIBWEBSOCKETS_VERSION}")
endif()

unset(FPHSA_NAME_MISMATCHED)
