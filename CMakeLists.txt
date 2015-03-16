cmake_minimum_required(VERSION 2.8.8)
# set(CMAKE_VERBOSE_MAKEFILE on)

project(open62541 C)
set(open62541_VERSION_MAJOR 0)
set(open62541_VERSION_MINOR 0)
set(open62541_VERSION_PATCH 0)
add_definitions(-DOPEN62541_VERSION_MAJOR=${open62541_VERSION_MAJOR})
add_definitions(-DOPEN62541_VERSION_MINOR=${open62541_VERSION_MINOR})
add_definitions(-DOPEN62541_VERSION_PATCH=${open62541_VERSION_PATCH})

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Set default build type.
if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "CMAKE_BUILD_TYPE not given; setting to 'RelWithDebInfo'.")
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build." FORCE)
endif()

# compiler flags
if(CMAKE_COMPILER_IS_GNUCC OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
add_definitions(-std=c99 -pipe -Wall -Wextra -Werror -Wformat
                -Wno-unused-parameter -Wno-unused-function -Wno-unused-label -Wpointer-arith -Wreturn-type -Wsign-compare -Wmultichar
                -Wshadow -Wcast-qual -Wmissing-prototypes -Wstrict-prototypes # -Wconversion
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

# build the main library
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")
file(GLOB_RECURSE exported_headers "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB generated_headers "${PROJECT_BINARY_DIR}/src_generated/*.h")
set(lib_sources src/ua_types.c
                src/ua_types_encoding_binary.c
                ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated.c
                ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated.c
                ${PROJECT_BINARY_DIR}/src_generated/ua_nodeids.h
                src/ua_securechannel.c
                src/ua_session.c
                src/server/ua_server.c
				src/server/ua_server_addressspace.c
				src/server/ua_server_binary.c
				src/server/ua_nodes.c
                src/server/ua_server_worker.c
                src/server/ua_securechannel_manager.c
                src/server/ua_session_manager.c
                src/server/ua_services_discovery.c
                src/server/ua_services_securechannel.c
                src/server/ua_services_session.c
                src/server/ua_services_attribute.c
                src/server/ua_services_nodemanagement.c
                src/server/ua_services_view.c
				${exported_headers}
				${generated_headers} )

## set the precompiler flags
configure_file("src/ua_config.h.in" "${PROJECT_BINARY_DIR}/src_generated/ua_config.h")

## generate code from xml definitions
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/src_generated")
include_directories("${PROJECT_BINARY_DIR}/src_generated")

add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated.c
                          ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated.h
                   PRE_BUILD
                   COMMAND python ${PROJECT_SOURCE_DIR}/tools/generate_datatypes.py --typedescriptions ${PROJECT_SOURCE_DIR}/tools/schema/NodeIds.csv 0 ${PROJECT_SOURCE_DIR}/tools/schema/Opc.Ua.Types.bsd ${PROJECT_BINARY_DIR}/src_generated/ua_types
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_datatypes.py
                           ${CMAKE_CURRENT_SOURCE_DIR}/tools/schema/Opc.Ua.Types.bsd
                           ${CMAKE_CURRENT_SOURCE_DIR}/tools/schema/NodeIds.csv)

add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated.c
                          ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated.h
                   PRE_BUILD
                   COMMAND python ${PROJECT_SOURCE_DIR}/tools/generate_datatypes.py --ns0-types-xml ${PROJECT_SOURCE_DIR}/tools/schema/Opc.Ua.Types.bsd 1 ${PROJECT_SOURCE_DIR}/tools/schema/Custom.Opc.Ua.Transport.bsd ${PROJECT_BINARY_DIR}/src_generated/ua_transport
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_datatypes.py
                           ${CMAKE_CURRENT_SOURCE_DIR}/tools/schema/Custom.Opc.Ua.Transport.bsd)

add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/ua_nodeids.h
                   PRE_BUILD
                   COMMAND python ${PROJECT_SOURCE_DIR}/tools/generate_nodeids.py ${PROJECT_SOURCE_DIR}/tools/schema/NodeIds.csv ${PROJECT_BINARY_DIR}/src_generated/ua_nodeids
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_nodeids.py
                           ${CMAKE_CURRENT_SOURCE_DIR}/tools/schema/NodeIds.csv)

## logging
set(UA_LOGLEVEL 300 CACHE STRING "Level at which logs shall be reported")

## multithreading
option(MULTITHREADING "Enable multithreading" OFF)
if(MULTITHREADING)
    set(UA_MULTITHREADING ON)
    find_package(Threads REQUIRED)
    list(APPEND lib_sources src/server/ua_nodestore_concurrent.c)
else()
    list(APPEND lib_sources src/server/ua_nodestore.c)
endif()

## extensions
option(EXTENSION_UDP "Enable udp extension" OFF)
if(EXTENSION_UDP)
	set(EXTENSION_STATELESS ON)
	message(STATUS "Extensions: enabling udp")
	add_definitions(-DEXTENSION_UDP)
endif()

option(EXTENSION_STATELESS "Enable stateless extension" OFF)
if(EXTENSION_STATELESS)
	message(STATUS "Extensions: enabling stateless")
	add_definitions(-DEXTENSION_STATELESS)
endif()

option(DEMO_NODESET "Create a demo node for every built-in datatype" OFF)
if(DEMO_NODESET)
	message(STATUS "Enabling demo nodeset")
	add_definitions(-DDEMO_NODESET)
endif()

add_library(open62541-objects OBJECT ${lib_sources})
add_library(open62541 SHARED $<TARGET_OBJECTS:open62541-objects>)
add_library(open62541-static STATIC $<TARGET_OBJECTS:open62541-objects>)
SET_TARGET_PROPERTIES(open62541-static PROPERTIES OUTPUT_NAME open62541 CLEAN_DIRECT_OUTPUT 1) # static version that exports all symbols
target_compile_definitions(open62541-objects PRIVATE UA_DYNAMIC_LINKING)
target_compile_definitions(open62541-static PRIVATE UA_DYNAMIC_LINKING)

# build language bindings for the library
option(ENABLE_BINDING_LUA "Build Lua bindings" OFF)
option(ENABLE_BINDING_PYTHON "Build Python bindings" OFF)
if(ENABLE_BINDING_LUA OR ENABLE_BINDING_PYTHON)
    add_subdirectory(bindings)
endif()

# coverage
option(ENABLE_COVERAGE "Enable gcov coverage" OFF)
if(ENABLE_COVERAGE)
    message(STATUS "Enabling gcov support")
    set(CMAKE_BUILD_TYPE DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
endif()

# build example client
option(CLIENT "Build a test client" OFF)
if(CLIENT)
	message(STATUS "Extensions: enabling client")
	add_definitions( -DBENCHMARK=1 )
    # the client is built directly with the .o files as it currently uses
    # internal functions that are not exported to the shared lib.
	add_executable(exampleClient $<TARGET_OBJECTS:open62541-objects> examples/client.c)
    if(MULTITHREADING)
        target_link_libraries(exampleClient urcu-cds urcu urcu-common pthread)
    endif()
    if ((CMAKE_COMPILER_IS_GNUCC OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang") AND (CMAKE_BUILD_TYPE STREQUAL "MinSizeRel" OR CMAKE_BUILD_TYPE STREQUAL "Release"))
    	add_custom_command(TARGET exampleClient POST_BUILD
    		COMMAND ${CMAKE_STRIP} $<TARGET_FILE:exampleClient>
		)
	endif()
    if(EXTENSION_STATELESS)
        add_executable(statelessClient $<TARGET_OBJECTS:open62541-objects> examples/client_stateless.c)
        if(MULTITHREADING)
            target_link_libraries(statelessClient urcu-cds urcu urcu-common pthread)
        endif()
        if ((CMAKE_COMPILER_IS_GNUCC OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang") AND (CMAKE_BUILD_TYPE STREQUAL "MinSizeRel" OR CMAKE_BUILD_TYPE STREQUAL "Release"))
    	add_custom_command(TARGET statelessClient POST_BUILD
    		COMMAND ${CMAKE_STRIP} $<TARGET_FILE:statelessClient>
		)
		endif()
    endif()
endif()

# build example server
option(EXAMPLESERVER "Build a test server" OFF)
if(EXAMPLESERVER)
    add_executable(exampleServer examples/server.c examples/networklayer_tcp.c examples/logger_stdout.c ${exported_headers} ${generated_headers})
    add_executable(exampleServer_datasource examples/server_datasource.c examples/networklayer_tcp.c examples/logger_stdout.c ${exported_headers} ${generated_headers})
    target_link_libraries(exampleServer open62541-static)
    target_link_libraries(exampleServer_datasource open62541-static)
    if(WIN32)
        target_link_libraries(exampleServer ws2_32)
        target_link_libraries(exampleServer_datasource ws2_32)
    else()
        target_link_libraries(exampleServer rt)
        target_link_libraries(exampleServer_datasource rt)
    endif()
    if(MULTITHREADING)
        target_link_libraries(exampleServer urcu-cds urcu urcu-common pthread)
        target_link_libraries(exampleServer_datasource urcu-cds urcu urcu-common pthread)
    endif()
    if ((CMAKE_COMPILER_IS_GNUCC OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang") AND (CMAKE_BUILD_TYPE STREQUAL "MinSizeRel" OR CMAKE_BUILD_TYPE STREQUAL "Release"))
    	add_custom_command(TARGET exampleServer POST_BUILD
    		COMMAND ${CMAKE_STRIP} $<TARGET_FILE:exampleServer>
		)
		add_custom_command(TARGET exampleServer_datasource POST_BUILD
    		COMMAND ${CMAKE_STRIP} $<TARGET_FILE:exampleServer_datasource>
		)
	endif()
endif()

## self-signed certificates
option(GENERATE_SELFSIGNED "Generate self-signed certificates" OFF)
if(GENERATE_SELFSIGNED)
    message(STATUS "Enabling self-signed certificates")
    add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/localhost.der
                              ${PROJECT_BINARY_DIR}/ca.crt
                   COMMAND python ${PROJECT_SOURCE_DIR}/tools/certs/create_self-signed.py ${PROJECT_BINARY_DIR}
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/certs/create_self-signed.py
                           ${CMAKE_CURRENT_SOURCE_DIR}/tools/certs/localhost.cnf)
    add_custom_target(selfsigned ALL DEPENDS ${PROJECT_BINARY_DIR}/localhost.der ${PROJECT_BINARY_DIR}/ca.crt)
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
