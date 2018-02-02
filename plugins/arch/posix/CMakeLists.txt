SET(SOURCE_GROUP ${SOURCE_GROUP}\\posix)

ua_add_architecture("posix")

if("${UA_ARCHITECTURE}" STREQUAL "posix")

ua_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

endif()