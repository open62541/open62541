# Architecture-specific eventloop files for POSIX (Linux, BSD, Mac) and Win32.
# This file is included from the main CMakeLists.txt via:
#   include(${UA_ARCH_FOLDER}/CMakeLists.txt)

# Make the POSIX eventloop header findable without a path prefix
include_directories(${CMAKE_CURRENT_LIST_DIR})

list(APPEND plugin_sources
     ${CMAKE_CURRENT_LIST_DIR}/eventloop_posix.h
     ${CMAKE_CURRENT_LIST_DIR}/clock_posix.c
     ${CMAKE_CURRENT_LIST_DIR}/eventloop_posix.c
     ${CMAKE_CURRENT_LIST_DIR}/eventloop_posix_tcp.c
     ${CMAKE_CURRENT_LIST_DIR}/eventloop_posix_udp.c
     ${CMAKE_CURRENT_LIST_DIR}/eventloop_posix_eth.c
     ${CMAKE_CURRENT_LIST_DIR}/eventloop_posix_interrupt.c)
