# This file is included from the top-level CMakeLists.txt. A custom
# architecture folder (UA_ARCH_FOLDER) provides its own copy of it. The
# in-tree folder remains on the include path so that the relative includes
# (../common, ../../deps) also resolve for folders outside the source tree.
# The paths are quoted as UA_ARCH_FOLDER may contain spaces.
include_directories("${CMAKE_CURRENT_LIST_DIR}" "${PROJECT_SOURCE_DIR}/arch/posix")

list(APPEND plugin_sources
     "${CMAKE_CURRENT_LIST_DIR}/eventloop_posix.h"
     "${CMAKE_CURRENT_LIST_DIR}/clock_posix.c"
     "${CMAKE_CURRENT_LIST_DIR}/eventloop_posix.c"
     "${CMAKE_CURRENT_LIST_DIR}/eventloop_posix_tcp.c"
     "${CMAKE_CURRENT_LIST_DIR}/eventloop_posix_udp.c"
     "${CMAKE_CURRENT_LIST_DIR}/eventloop_posix_eth.c"
     "${CMAKE_CURRENT_LIST_DIR}/eventloop_posix_interrupt.c")
