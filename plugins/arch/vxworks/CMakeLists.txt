SET(SOURCE_GROUP ${SOURCE_GROUP}\\vxworks)

ua_add_architecture("vxworks")

if("${UA_ARCHITECTURE}" STREQUAL "vxworks")

    ua_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
    ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_clock.c)
    
    ua_architecture_remove_definitions(-Werror -Wpedantic -Wno-static-in-inline -fPIC)
    ua_architecture_add_definitions(-D_WRS_KERNEL)
    
endif()