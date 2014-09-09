cmake_minimum_required(VERSION 2.6)
# set(CMAKE_VERBOSE_MAKEFILE on )

project(open62541)
set(open62541_VERSION_MAJOR 0)
set(open62541_VERSION_MINOR 1)

# main sources of libopen62541
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")
file(GLOB_RECURSE headers "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
file(GLOB generated_headers "${PROJECT_BINARY_DIR}/src_generated/*.h")
set(lib_sources src/ua_types.c
                src/ua_types_encoding_binary.c
                ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated.c
                ${PROJECT_BINARY_DIR}/src_generated/ua_namespace_0.c
                src/ua_transport.c
				src/ua_connection.c
                src/ua_securechannel.c
                src/ua_session.c
                src/util/ua_util.c
                src/util/ua_list.c
                src/util/ua_base64.c
                src/server/ua_namespace.c
                src/server/ua_server.c
                src/server/ua_securechannel_manager.c
                src/server/ua_session_manager.c
                src/server/ua_server_binary.c
                src/server/ua_services_attribute.c
                src/server/ua_services_session.c
                src/server/ua_services_discovery.c
                src/server/ua_services_securechannel.c
                src/server/ua_services_nodemanagement.c
                src/server/ua_services_view.c
                src/server/ua_services_subscription.c
                src/server/ua_services_monitoreditems.c
                ${headers}
                ${generated_headers})

# compiler flags
if(CMAKE_COMPILER_IS_GNUCC)
add_definitions(-std=c99 -pedantic -pipe -fstack-protector -Wall -Wextra
                -Wno-unused-parameter -Wno-unused-function -Wno-unused-label
                -Wpointer-arith -Wformat -Wreturn-type -Wsign-compare -Wmultichar
                -Wformat-nonliteral -Winit-self -Wuninitialized -Wno-deprecated
                -Wformat-security -Werror -ffunction-sections -fdata-sections
                -Wl,--gc-sections)
	if(WIN32)
	    add_definitions(-fno-stack-protector)
	endif(WIN32)
endif(CMAKE_COMPILER_IS_GNUCC)

# build settings
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules")
set(generate_src_options) # the options for the tools that generate code from xml-schema definitions

## auto-generate all types or only the relevant subset?
option(TYPES_ONLY_NEEDED "Include only compile-needed types" OFF)
if(TYPES_ONLY_NEEDED)
    list(APPEND generate_src_options "--only-needed")
endif(TYPES_ONLY_NEEDED)

## encodings
set(UA_ENCODING_AMOUNT 1) # binary encoding

### xml
option(ENABLE_XML_ENCODING "Enable XML-encoding of the UA types" OFF)
if(ENABLE_XML_ENCODING)
    MATH(EXPR UA_ENCODING_AMOUNT "${UA_ENCODING_AMOUNT}+1")
    find_package(EXPAT REQUIRED)
    if(EXPAT_FOUND)
        include_directories(${EXPAT_INCLUDE_DIRS})
    else(EXPAT_FOUND)
        message(FATAL_ERROR "Expat library not found.")
    endif(EXPAT_FOUND)
	include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/ongoing")
    list(APPEND lib_sources src/ongoing/ua_types_encoding_xml.c
                            src/ongoing/ua_namespace_xml.c
                            src/ongoing/ua_xml.c)
    list(APPEND generate_src_options "--with-xml")
endif(ENABLE_XML_ENCODING)

### json
option(ENABLE_JSON_ENCODING "Enable JSON-encoding of the UA types" OFF)
if(ENABLE_JSON_ENCODING)
    MATH(EXPR UA_ENCODING_AMOUNT "${UA_ENCODING_AMOUNT}+1")
	include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/ongoing")
    list(APPEND lib_sources src/ongoing/ua_types_encoding_json.c)
    list(APPEND generate_src_options "--with-json")
endif(ENABLE_JSON_ENCODING)

configure_file("src/ua_config.h.in" "${PROJECT_BINARY_DIR}/src_generated/ua_config.h")

## multithreading
option(ENABLE_MULTITHREADING "Enable multithreading" OFF)
if(ENABLE_MULTITHREADING)
    find_package(Threads REQUIRED)
    list(APPEND lib_sources src/server/ua_namespace_concurrent.c)
else()
    list(APPEND lib_sources src/server/ua_namespace.c)
endif(ENABLE_MULTITHREADING)

add_library(open62541 ${lib_sources}) # we can add more files to lib_sources later on

## logging
set(UA_LOGLEVEL 400 CACHE STRING "Level at which logs shall be reported")

## coverage
option(ENABLE_COVERAGE "Enable gcov coverage" OFF)
if(ENABLE_COVERAGE)
    message(STATUS "Enabling gcov support")
    set(CMAKE_BUILD_TYPE DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
endif(ENABLE_COVERAGE)

# build generated code
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/src_generated")
include_directories("${PROJECT_BINARY_DIR}/src_generated") 
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated.c
                          ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated.h
                   COMMAND python ${PROJECT_SOURCE_DIR}/tools/generate_builtin.py ${generate_src_options} ${PROJECT_SOURCE_DIR}/schema/Opc.Ua.Types.bsd ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_builtin.py
                           ${CMAKE_CURRENT_SOURCE_DIR}/schema/Opc.Ua.Types.bsd)

add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/ua_namespace_0.c
                          ${PROJECT_BINARY_DIR}/src_generated/ua_namespace_0.h
                   COMMAND python ${PROJECT_SOURCE_DIR}/tools/generate_namespace.py ${generate_src_options} ${PROJECT_SOURCE_DIR}/schema/NodeIds.csv ${PROJECT_BINARY_DIR}/src_generated/ua_namespace_0
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_namespace.py
                           ${CMAKE_CURRENT_SOURCE_DIR}/schema/NodeIds.csv)

# build example server
add_executable(exampleServer examples/opcuaServer.c
                             examples/logger_stdout.c
                             examples/networklayer_tcp.c)
target_link_libraries(exampleServer open62541)
if(WIN32)
    target_link_libraries(exampleServer ws2_32)
endif(WIN32)
if(ENABLE_MULTITHREADING)
    target_link_libraries(exampleServer urcu-cds urcu)
endif(ENABLE_MULTITHREADING)

# build unit tests
option(ENABLE_UNIT_TESTS "Run unit tests after building" OFF)
if(ENABLE_UNIT_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif(ENABLE_UNIT_TESTS)

# build documentation
option(GENERATE_DOCUMENTATION "Generate doxygen documentation" OFF)
if(GENERATE_DOCUMENTATION)
    find_package(Doxygen)
        if(NOT DOXYGEN_FOUND)
            message(FATAL_ERROR "Doxygen is not installed or not properly configured")
        endif(NOT DOXYGEN_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(doc
                      ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
                      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                      COMMENT "Generating API documentation with Doxygen")
endif(GENERATE_DOCUMENTATION)

# download queue.h if required
if(WIN32)
    if(NOT EXISTS "${PROJECT_BINARY_DIR}/src_generated/queue.h")
        file(DOWNLOAD "http://openbsd.cs.toronto.edu/cgi-bin/cvsweb/~checkout~/src/sys/sys/queue.h" "${PROJECT_BINARY_DIR}/src_generated/queue.h" STATUS result)
        list(GET result 0 download_ok)
        if(NOT ${download_ok} MATCHES 0)
            file(REMOVE "${PROJECT_BINARY_DIR}/src_generated/queue.h") # remove empty file if created
            message(FATAL_ERROR "queue.h could not be downloaded")
        endif()
    endif()
endif(WIN32)

# build api server specificaion
#add_executable(api-design examples/api-design/server.c)
#target_link_libraries(api-design open62541)
#if(WIN32)
#    target_link_libraries(api-design ws2_32)
#endif(WIN32)
#if(MULTITHREADING)
#    target_link_libraries(api-design urcu-cds urcu)
#endif(MULTITHREADING)

