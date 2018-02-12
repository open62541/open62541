SET(SOURCE_GROUP ${SOURCE_GROUP}\\win32)

ua_add_architecture("win32")

if("${UA_ARCHITECTURE}" STREQUAL "win32")

ua_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_clock.c)

endif()