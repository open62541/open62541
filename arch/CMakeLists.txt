SET(SOURCE_GROUP ${SOURCE_GROUP}\\arch)

ua_include_directories(${CMAKE_CURRENT_SOURCE_DIR}) #to have access to ua_network_tcp.h and udp.h

add_subdirectory(posix)
add_subdirectory(win32)
add_subdirectory(freertosLWIP)
add_subdirectory(vxworks)
add_subdirectory(eCos)

SET(UA_ARCH_EXTRA_INCLUDES "" CACHE STRING "Folders to include from the architecture")
ua_include_directories(${UA_ARCH_EXTRA_INCLUDES})

SET(UA_ARCH_REMOVE_FLAGS "" CACHE STRING "Flags to be removed from compilation that depends on the architecure")
ua_architecture_remove_definitions(${UA_ARCH_REMOVE_FLAGS})

SET(UA_ARCH_ADD_FLAGS "" CACHE STRING "Flags to be added to compilation that depends on the architecure")
ua_architecture_add_definitions(${UA_ARCH_ADD_FLAGS})

SET(UA_ARCH_LINKER_FLAGS "" CACHE STRING "Linker Flags to be added to compilation that depends on the architecure")
ua_architecture_append_to_library(${UA_ARCH_LINKER_FLAGS})
