cmake_minimum_required(VERSION 3.5)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../../ ${CMAKE_BINARY_DIR}/lib)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

get_target_property(OPEN62541_SOURCES open62541-object SOURCES)
get_target_property(OPEN62541_INCLUDES open62541-object INCLUDE_DIRECTORIES)

# Create an open62541 component for ESP IDF
set(open62541_component_dir ${CMAKE_CURRENT_LIST_DIR}/components/open62541-lib)
file(MAKE_DIRECTORY ${open62541_component_dir})
file(WRITE ${open62541_component_dir}/CMakeLists.txt "
set(COMPONENT_SRCS ${OPEN62541_SOURCES})
set(COMPONENT_ADD_INCLUDEDIRS ${OPEN62541_INCLUDES})
set(COMPONENT_REQUIRES lwip mbedtls freertos)
register_component()
")

add_definitions(-DUA_ARCHITECTURE_FREERTOSLWIP)

project(hello-world)

add_dependencies(open62541-lib open62541-code-generation)

