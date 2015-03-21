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
if(NOT CMAKE_BUILD_TYPE)
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
set(exported_headers ${PROJECT_BINARY_DIR}/src_generated/ua_config.h
                     ${PROJECT_SOURCE_DIR}/include/ua_statuscodes.h
                     ${PROJECT_SOURCE_DIR}/include/ua_types.h
                     ${PROJECT_BINARY_DIR}/src_generated/ua_nodeids.h
                     ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated.h
                     ${PROJECT_SOURCE_DIR}/include/ua_connection.h
                     ${PROJECT_SOURCE_DIR}/include/ua_log.h
                     ${PROJECT_SOURCE_DIR}/include/ua_server.h)
set(internal_headers ${PROJECT_SOURCE_DIR}/src/ua_util.h
                     ${PROJECT_SOURCE_DIR}/deps/queue.h
                     ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated.h
                     ${PROJECT_SOURCE_DIR}/src/ua_securechannel.h
                     ${PROJECT_SOURCE_DIR}/src/ua_session.h
                     ${PROJECT_SOURCE_DIR}/src/ua_types_encoding_binary.h
                     ${PROJECT_SOURCE_DIR}/src/server/ua_nodes.h
                     ${PROJECT_SOURCE_DIR}/src/server/ua_nodestore.h
                     ${PROJECT_SOURCE_DIR}/src/server/ua_session_manager.h
                     ${PROJECT_SOURCE_DIR}/src/server/ua_securechannel_manager.h
                     ${PROJECT_SOURCE_DIR}/src/server/ua_server_internal.h
                     ${PROJECT_SOURCE_DIR}/src/server/ua_services.h)
set(lib_sources ${PROJECT_SOURCE_DIR}/src/ua_types.c
                ${PROJECT_SOURCE_DIR}/src/ua_types_encoding_binary.c
                ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated.c
                ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated.c
                ${PROJECT_SOURCE_DIR}/src/ua_securechannel.c
                ${PROJECT_SOURCE_DIR}/src/ua_session.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_server.c
				${PROJECT_SOURCE_DIR}/src/server/ua_server_addressspace.c
				${PROJECT_SOURCE_DIR}/src/server/ua_server_binary.c
				${PROJECT_SOURCE_DIR}/src/server/ua_nodes.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_server_worker.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_securechannel_manager.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_session_manager.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_discovery.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_securechannel.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_session.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_attribute.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_nodemanagement.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_view.c)

## generate code from xml definitions
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/src_generated")
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

## set the precompiler flags
configure_file("src/ua_config.h.in" "${PROJECT_BINARY_DIR}/src_generated/ua_config.h")

## multithreading
option(MULTITHREADING "Enable multithreading" OFF)
if(MULTITHREADING)
    set(UA_MULTITHREADING ON)
    find_package(Threads REQUIRED)
    list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/src/server/ua_nodestore_concurrent.c)
else()
    list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/src/server/ua_nodestore.c)
endif()

## build amalgamated source files
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/open62541.h ${PROJECT_BINARY_DIR}/open62541.c
                   PRE_BUILD
                   COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/tools/amalgamate.py ${CMAKE_CURRENT_BINARY_DIR}/open62541.h ${exported_headers}
                   COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/tools/amalgamate.py ${CMAKE_CURRENT_BINARY_DIR}/open62541.c
                                  ${internal_headers} ${PROJECT_SOURCE_DIR}/src/server/ua_nodestore_hash.inc ${lib_sources}
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/amalgamate.py ${exported_headers} ${internal_headers}
                           ${PROJECT_SOURCE_DIR}/src/server/ua_nodestore_hash.inc ${lib_sources})

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

option(BUILD_AMALGAMATION "Build the single-file distribution" ON)
if(BUILD_AMALGAMATION)
    add_custom_target(open62541-amalgamation DEPENDS ${PROJECT_BINARY_DIR}/open62541.h ${PROJECT_BINARY_DIR}/open62541.c)
endif()

option(BUILD_LIBRARY "Build a shared library (.so/.dll)" ON)
if(BUILD_LIBRARY)
    add_library(open62541 SHARED ${lib_sources} ${exported_headers} ${internal_headers}) 
    target_include_directories(open62541 PRIVATE ${PROJECT_SOURCE_DIR}/include)
    target_include_directories(open62541 PRIVATE ${PROJECT_SOURCE_DIR}/deps)
    target_include_directories(open62541 PRIVATE ${PROJECT_SOURCE_DIR}/src)
    target_include_directories(open62541 PRIVATE ${PROJECT_BINARY_DIR}/src_generated)
    target_compile_definitions(open62541 PRIVATE UA_DYNAMIC_LINKING)
endif()

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

# build example server
option(BUILD_EXAMPLESERVER "Build the example server" OFF)
if(BUILD_EXAMPLESERVER)
    add_executable(exampleServer examples/server.c ${PROJECT_BINARY_DIR}/open62541.c examples/networklayer_tcp.c examples/logger_stdout.c)
    add_executable(exampleServer_datasource examples/server_datasource.c ${PROJECT_BINARY_DIR}/open62541.c examples/networklayer_tcp.c examples/logger_stdout.c)
    target_include_directories(exampleServer PRIVATE ${PROJECT_BINARY_DIR})
    target_include_directories(exampleServer_datasource PRIVATE ${PROJECT_BINARY_DIR})
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
    if((CMAKE_COMPILER_IS_GNUCC OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang") AND (CMAKE_BUILD_TYPE STREQUAL "MinSizeRel" OR CMAKE_BUILD_TYPE STREQUAL "Release"))
    	add_custom_command(TARGET exampleServer POST_BUILD COMMAND ${CMAKE_STRIP} $<TARGET_FILE:exampleServer>)
		add_custom_command(TARGET exampleServer_datasource POST_BUILD COMMAND ${CMAKE_STRIP} $<TARGET_FILE:exampleServer_datasource>)
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

# build example client
option(BUILD_EXAMPLECLIENT "Build a test client" OFF)
if(BUILD_EXAMPLECLIENT)
	message(STATUS "Extensions: enabling client")
	add_definitions( -DBENCHMARK=1 )
	add_executable(exampleClient ${PROJECT_BINARY_DIR}/open62541.c examples/client.c)
    target_include_directories(exampleClient PRIVATE ${PROJECT_BINARY_DIR})
    if(MULTITHREADING)
        target_link_libraries(exampleClient urcu-cds urcu urcu-common pthread)
    endif()
    if ((CMAKE_COMPILER_IS_GNUCC OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang") AND (CMAKE_BUILD_TYPE STREQUAL "MinSizeRel" OR CMAKE_BUILD_TYPE STREQUAL "Release"))
    	add_custom_command(TARGET exampleClient POST_BUILD
    		COMMAND ${CMAKE_STRIP} $<TARGET_FILE:exampleClient>
		)
	endif()
    if(EXTENSION_STATELESS)
        add_executable(statelessClient ${PROJECT_BINARY_DIR}/open62541.c examples/client_stateless.c)
        target_include_directories(statelessClient PRIVATE ${PROJECT_BINARY_DIR})
        if(MULTITHREADING)
            target_link_libraries(statelessClient urcu-cds urcu urcu-common pthread)
        endif()
        if ((CMAKE_COMPILER_IS_GNUCC OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang") AND (CMAKE_BUILD_TYPE STREQUAL "MinSizeRel" OR CMAKE_BUILD_TYPE STREQUAL "Release"))
    	add_custom_command(TARGET statelessClient POST_BUILD COMMAND ${CMAKE_STRIP} $<TARGET_FILE:statelessClient>)
		endif()
    endif()
endif()

# build unit tests
option(BUILD_UNIT_TESTS "Run unit tests after building" OFF)
if(BUILD_UNIT_TESTS)
    if(NOT BUILD_LIBRARY)
        message(SEND_ERROR "Can't build unit tests without building the library")
    endif()
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
