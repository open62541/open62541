cmake_minimum_required(VERSION 2.8.8)
# set(CMAKE_VERBOSE_MAKEFILE on )

project(open62541 C)
set(open62541_VERSION_MAJOR 0)
set(open62541_VERSION_MINOR 0)
set(NVIM_VERSION_PATCH 0)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Set default build type.
if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "CMAKE_BUILD_TYPE not given; setting to 'RelWithDebInfo'.")
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build." FORCE)
endif()

# main sources of libopen62541
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")
file(GLOB_RECURSE exported_headers "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB_RECURSE headers "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
file(GLOB generated_headers "${PROJECT_BINARY_DIR}/src_generated/*.h")
set(lib_sources src/ua_types.c
                src/ua_types_encoding_binary.c
                ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated.c
                ${PROJECT_BINARY_DIR}/src_generated/ua_namespace_0.c
                src/ua_transport.c
                ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated.c
                src/ua_securechannel.c
                src/ua_session.c
                src/client/ua_client.c
                src/server/ua_server.c
				src/server/ua_server_addressspace.c
				src/server/ua_server_binary.c
                src/server/ua_server_worker.c
                src/server/ua_securechannel_manager.c
                src/server/ua_session_manager.c
                src/server/ua_services_attribute.c
                src/server/ua_services_session.c
                src/server/ua_services_discovery.c
                src/server/ua_services_securechannel.c
                src/server/ua_services_nodemanagement.c
                src/server/ua_services_view.c
				${exported_headers}
				${generated_headers}
                ${headers})

# compiler flags
if(CMAKE_COMPILER_IS_GNUCC OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
add_definitions(-std=c99 -pedantic -pipe -Wall -Wextra -Werror -Wformat
                -Wno-unused-parameter -Wno-unused-function -Wno-unused-label -Wpointer-arith -Wreturn-type -Wsign-compare -Wmultichar
                -Wshadow -Wcast-align -Wcast-qual -Wmissing-prototypes -Wstrict-prototypes # -Wconversion
                -Winit-self -Wuninitialized -Wno-deprecated -Wformat-security -ffunction-sections -fdata-sections)
    if(NOT "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
        add_definitions(-Wformat-nonliteral)
        set (CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--gc-sections")
        set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections")
    else()
        add_definitions(-Wno-gnu-statement-expression)
    endif()
	if(NOT WIN32)
	    add_definitions(-fstack-protector -fPIC -fvisibility=hidden)
	endif()
endif()

# build settings
set(generate_src_options) # the options for the tools that generate code from xml-schema definitions

## set debug flag
if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(UA_DEBUG ON)
else()
    set(UA_DEBUG OFF)
endif()

## extensions
option(EXTENSION_STATELESS "Enable stateless extension" OFF)
option(EXTENSION_UDP "Enable udp extension" OFF)

if(EXTENSION_UDP)
	set(EXTENSION_STATELESS ON)
	message(STATUS "Extensions: enabling udp")
	add_definitions(-DEXTENSION_UDP)
endif()

if(EXTENSION_STATELESS)
	message(STATUS "Extensions: enabling stateless")
	add_definitions(-DEXTENSION_STATELESS)
	add_executable(statelessClient $<TARGET_OBJECTS:open62541-objects> examples/statelessClient.c)
    if(MULTITHREADING)
        target_link_libraries(statelessClient urcu-cds urcu urcu-common pthread)
    endif()
endif()

## self-signed certificates
option(ENABLE_SELFSIGNED "Enable self-signed certificates" OFF)
if(ENABLE_SELFSIGNED)
    message(STATUS "Enabling self-signed certificates")
    SET(lib_sources ${lib_sources} ${PROJECT_BINARY_DIR}/localhost.der ${PROJECT_BINARY_DIR}/ca.crt)
    add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/localhost.der
                              ${PROJECT_BINARY_DIR}/ca.crt
                   COMMAND python ${PROJECT_SOURCE_DIR}/tools/certs/create_self-signed.py ${PROJECT_BINARY_DIR}
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/certs/create_self-signed.py
                           ${CMAKE_CURRENT_SOURCE_DIR}/tools/certs/localhost.cnf)
endif()

## auto-generate all types or only the relevant subset?
option(TYPES_ONLY_NEEDED "Include only compile-needed types" OFF)
if(TYPES_ONLY_NEEDED)
    list(APPEND generate_src_options "--only-needed")
endif()

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
endif()

# ### json
# option(ENABLE_JSON_ENCODING "Enable JSON-encoding of the UA types" OFF)
# if(ENABLE_JSON_ENCODING)
#     MATH(EXPR UA_ENCODING_AMOUNT "${UA_ENCODING_AMOUNT}+1")
# 	include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/ongoing")
#     list(APPEND lib_sources src/ongoing/ua_types_encoding_json.c)
#     list(APPEND generate_src_options "--with-json")
# endif(ENABLE_JSON_ENCODING)

## multithreading
option(MULTITHREADING "Enable multithreading" OFF)
if(MULTITHREADING)
    set(UA_MULTITHREADING ON)
    find_package(Threads REQUIRED)
    list(APPEND lib_sources src/server/ua_nodestore_concurrent.c)
else()
    list(APPEND lib_sources src/server/ua_nodestore.c)
endif()

add_library(open62541-objects OBJECT ${lib_sources}) 
add_library(open62541 SHARED $<TARGET_OBJECTS:open62541-objects>)
add_library(open62541-static STATIC $<TARGET_OBJECTS:open62541-objects>)
SET_TARGET_PROPERTIES(open62541-static PROPERTIES OUTPUT_NAME open62541 CLEAN_DIRECT_OUTPUT 1) # static version that exports all symbols
target_compile_definitions(open62541-objects PUBLIC UA_DYNAMIC_LINKING)

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
endif()

# set the precompiler flags
configure_file("src/ua_config.h.in" "${PROJECT_BINARY_DIR}/src_generated/ua_config.h")

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

# generate code from xml definitions
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/src_generated")
include_directories("${PROJECT_BINARY_DIR}/src_generated") 
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated.c
                          ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated.h
                   PRE_BUILD
                   COMMAND python ${PROJECT_SOURCE_DIR}/tools/generate_builtin.py --export-prototypes ${generate_src_options} ${PROJECT_SOURCE_DIR}/tools/schema/Opc.Ua.Types.bsd ${PROJECT_BINARY_DIR}/src_generated/ua_types
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_builtin.py
                           ${CMAKE_CURRENT_SOURCE_DIR}/tools/schema/Opc.Ua.Types.bsd)

add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/ua_namespace_0.c
                          ${PROJECT_BINARY_DIR}/src_generated/ua_namespace_0.h
                   PRE_BUILD
                   COMMAND python ${PROJECT_SOURCE_DIR}/tools/generate_namespace.py ${generate_src_options} ${PROJECT_SOURCE_DIR}/tools/schema/NodeIds.csv ${PROJECT_BINARY_DIR}/src_generated/ua_namespace_0
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_namespace.py
                           ${CMAKE_CURRENT_SOURCE_DIR}/tools/schema/NodeIds.csv)

add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated.c
                          ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated.h
                   PRE_BUILD
                   COMMAND python ${PROJECT_SOURCE_DIR}/tools/generate_builtin.py --additional-includes=ua_transport.h ${PROJECT_SOURCE_DIR}/tools/schema/Custom.Opc.Ua.Transport.bsd ${PROJECT_BINARY_DIR}/src_generated/ua_transport
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_builtin.py
                           ${CMAKE_CURRENT_SOURCE_DIR}/tools/schema/Custom.Opc.Ua.Transport.bsd)

# build example client
option(CLIENT "Build a test client" OFF)
if(CLIENT)
	message(STATUS "Extensions: enabling client")
	add_definitions( -DBENCHMARK=1 )
    # the client is built directly with the .o files as it currently uses
    # internal functions that are not exported to the shared lib.
	add_executable(exampleClient $<TARGET_OBJECTS:open62541-objects> examples/opcuaClient.c)
    if(MULTITHREADING)
        target_link_libraries(exampleClient urcu-cds urcu urcu-common pthread)
    endif()
endif()

# build example server
option(EXAMPLESERVER "Build a test server" OFF)
if(EXAMPLESERVER)
set(server_sources examples/opcuaServer.c
                   examples/networklayer_tcp.c
                   examples/logger_stdout.c)
if(EXTENSION_UDP)
	list(APPEND server_sources examples/networklayer_udp.c)
endif()
add_executable(exampleServer ${server_sources} ${exported_headers} ${generated_headers})
target_link_libraries(exampleServer open62541)
if(WIN32)
    target_link_libraries(exampleServer ws2_32)
else()
    target_link_libraries(exampleServer rt)
endif(WIN32)
if(MULTITHREADING)
    target_link_libraries(exampleServer urcu-cds urcu urcu-common pthread)
endif()
endif()

# build unit tests
option(ENABLE_UNIT_TESTS "Run unit tests after building" OFF)
if(ENABLE_UNIT_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

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
endif()
