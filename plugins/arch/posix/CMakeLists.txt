SET(SOURCE_GROUP ${SOURCE_GROUP}\\posix)

ua_add_architecture("posix")

if("${UA_ARCHITECTURE}" STREQUAL "posix")

ua_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
ua_add_architecture_header(${CMAKE_CURRENT_SOURCE_DIR}/ua_architecture.h)

endif()