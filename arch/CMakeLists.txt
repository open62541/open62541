SET(SOURCE_GROUP ${SOURCE_GROUP}\\arch)

ua_include_directories(${CMAKE_CURRENT_SOURCE_DIR}) #to have access to ua_network_tcp.h and udp.h

add_subdirectory(posix)
add_subdirectory(win32)
add_subdirectory(arduino)
add_subdirectory(freertos)
add_subdirectory(vxworks)
add_subdirectory(eCos)
