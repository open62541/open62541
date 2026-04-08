include_directories(${CMAKE_CURRENT_LIST_DIR})

list(APPEND plugin_sources
     ${CMAKE_CURRENT_LIST_DIR}/eventloop_posix.h
     ${CMAKE_CURRENT_LIST_DIR}/clock_posix.c
     ${CMAKE_CURRENT_LIST_DIR}/eventloop_posix.c
     ${CMAKE_CURRENT_LIST_DIR}/eventloop_posix_tcp.c
     ${CMAKE_CURRENT_LIST_DIR}/eventloop_posix_udp.c
     ${CMAKE_CURRENT_LIST_DIR}/eventloop_posix_eth.c
     ${CMAKE_CURRENT_LIST_DIR}/eventloop_posix_interrupt.c)
