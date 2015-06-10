cmake_minimum_required(VERSION 2.8.8)
# set(CMAKE_VERBOSE_MAKEFILE ON)

project(open62541 C)

FIND_PACKAGE(PythonInterp REQUIRED)

# Find Python-lxml
execute_process ( COMMAND ${PYTHON_EXECUTABLE} -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())" OUTPUT_VARIABLE PYTHON_SITE_PACKAGES OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT EXISTS "${PYTHON_SITE_PACKAGES}/lxml")
    execute_process ( COMMAND ${PYTHON_EXECUTABLE} -c "from distutils.sysconfig import get_python_lib; print(get_python_lib(True))" OUTPUT_VARIABLE PYTHON_SITE_PACKAGES OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT EXISTS "${PYTHON_SITE_PACKAGES}/lxml")
        message( FATAL_ERROR "Python-lxml is not installed.")
    endif()
endif()

find_package(Git)
if(GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --abbrev=7 --dirty --always --tags RESULT_VARIABLE res_var OUTPUT_VARIABLE GIT_COM_ID )
    if( NOT ${res_var} EQUAL 0 )
        set( GIT_COMMIT_ID "git commit id unknown")
        message( WARNING "Git failed (not a repo, or no tags). Build will not contain git revision info." )
    else()
        string( REPLACE "\n" "" GIT_COMMIT_ID ${GIT_COM_ID} )
    endif()
else()
    set( GIT_COMMIT_ID "unknown (git not found!)")
    message( WARNING "Git not found. Build will not contain git revision info." )
endif()
add_definitions("-DVERSION=${GIT_COMMIT_ID}")

MESSAGE (STATUS "Git version: "  ${GIT_COMMIT_ID})

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Set default build type.
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "CMAKE_BUILD_TYPE not given; setting to 'RelWithDebInfo'.")
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build." FORCE)
endif()

# compiler flags
if(CMAKE_COMPILER_IS_GNUCC OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
    add_definitions(-std=c99 -pipe -Wall -Wextra -Werror -Wformat -Wno-unused-parameter
                    -Wno-unused-function -Wno-unused-label -Wpointer-arith -Wreturn-type -Wsign-compare -Wmultichar
                    -Wcast-qual -Wmissing-prototypes -Wstrict-prototypes # -Wshadow -Wconversion
                    -Winit-self -Wuninitialized -Wformat-security -Wformat-nonliteral)
    # binary size reduction settings
	add_definitions(-ffunction-sections -fdata-sections -fno-stack-protector -fno-unwind-tables
                    -fno-asynchronous-unwind-tables -fno-math-errno -fmerge-all-constants -fno-ident)
    set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--gc-sections")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections")
    set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "") # cmake sets -rdynamic by default
	if(NOT WIN32)
	    add_definitions(-fvisibility=hidden -fPIC)
        set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,-z,norelro -Wl,--hash-style=gnu -Wl,--build-id=none")
	endif()
    if(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel" OR CMAKE_BUILD_TYPE STREQUAL "Release")
#        add_definitions(-flto)
        set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -s")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -s")
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
                     ${PROJECT_SOURCE_DIR}/include/ua_server.h
                     ${PROJECT_SOURCE_DIR}/include/ua_client.h
                     ${PROJECT_SOURCE_DIR}/examples/networklayer_tcp.h 
                     ${PROJECT_SOURCE_DIR}/examples/logger_stdout.h)
set(internal_headers ${PROJECT_SOURCE_DIR}/src/ua_util.h
                     ${PROJECT_SOURCE_DIR}/deps/queue.h
                     ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated.h
                     ${PROJECT_SOURCE_DIR}/src/ua_types_encoding_binary.h
                     ${PROJECT_SOURCE_DIR}/src/ua_securechannel.h
                     ${PROJECT_SOURCE_DIR}/src/ua_session.h
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
                ${PROJECT_SOURCE_DIR}/src/ua_connection.c
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
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_view.c
                ${PROJECT_SOURCE_DIR}/src/client/ua_client.c
                ${PROJECT_SOURCE_DIR}/examples/networklayer_tcp.c 
                ${PROJECT_SOURCE_DIR}/examples/logger_stdout.c)
                ##TODO: make client stuff optional

## generate code from xml definitions
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/src_generated")
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated.c
                          ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated.h
                   PRE_BUILD
                   COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/generate_datatypes.py --typedescriptions ${PROJECT_SOURCE_DIR}/tools/schema/NodeIds.csv 0 ${PROJECT_SOURCE_DIR}/tools/schema/Opc.Ua.Types.bsd ${PROJECT_BINARY_DIR}/src_generated/ua_types
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_datatypes.py
                           ${CMAKE_CURRENT_SOURCE_DIR}/tools/schema/Opc.Ua.Types.bsd
                           ${CMAKE_CURRENT_SOURCE_DIR}/tools/schema/NodeIds.csv)

add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated.c
                          ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated.h
                   PRE_BUILD
                   COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/generate_datatypes.py --ns0-types-xml ${PROJECT_SOURCE_DIR}/tools/schema/Opc.Ua.Types.bsd 1 ${PROJECT_SOURCE_DIR}/tools/schema/Custom.Opc.Ua.Transport.bsd ${PROJECT_BINARY_DIR}/src_generated/ua_transport
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_datatypes.py
                           ${CMAKE_CURRENT_SOURCE_DIR}/tools/schema/Custom.Opc.Ua.Transport.bsd)

add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/ua_nodeids.h
                   PRE_BUILD
                   COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/generate_nodeids.py ${PROJECT_SOURCE_DIR}/tools/schema/NodeIds.csv ${PROJECT_BINARY_DIR}/src_generated/ua_nodeids
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_nodeids.py
                           ${CMAKE_CURRENT_SOURCE_DIR}/tools/schema/NodeIds.csv)

option(ENABLE_GENERATE_NAMESPACE0 "Generate and load UA XML Namespace 0 definition" OFF)
if(ENABLE_GENERATE_NAMESPACE0)
add_definitions(-DENABLE_GENERATE_NAMESPACE0)
set(GENERATE_NAMESPACE0_FILE "Opc.Ua.NodeSet2.xml" CACHE STRING "Namespace definition XML file")
set_property(CACHE GENERATE_NAMESPACE0_FILE PROPERTY STRINGS Opc.Ua.NodeSet2.xml Opc.Ua.NodeSet2.Minimal.xml)
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/ua_namespaceinit_generated.c
                          ${PROJECT_BINARY_DIR}/src_generated/ua_namespaceinit_generated.h
                   PRE_BUILD
                   COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/generate_open62541CCode.py -i ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/NodeID_AssumeExternal.txt -s description -b ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/NodeID_Blacklist.txt ${PROJECT_SOURCE_DIR}/tools/schema/namespace0/${GENERATE_NAMESPACE0_FILE} ${PROJECT_BINARY_DIR}/src_generated/ua_namespaceinit_generated
                   DEPENDS ${PROJECT_SOURCE_DIR}/tools/schema/namespace0/${GENERATE_NAMESPACE0_FILE}
                           ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/generate_open62541CCode.py  
                           ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/logger.py  
                           ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/open62541_MacroHelper.py  
                           ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/ua_builtin_types.py  
                           ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/ua_constants.py  
                           ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/ua_namespace.py  
                           ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/ua_node_types.py)

list(APPEND internal_headers ${PROJECT_BINARY_DIR}/src_generated/ua_namespaceinit_generated.h)
list(APPEND lib_sources ${PROJECT_BINARY_DIR}/src_generated/ua_namespaceinit_generated.c)
endif()

## logging
set(UA_LOGLEVEL 300 CACHE STRING "Level at which logs shall be reported")

## multithreading
option(ENABLE_MULTITHREADING "Enable multithreading (experimental)" OFF)
if(ENABLE_MULTITHREADING)
    add_definitions(-DUA_MULTITHREADING)
    find_package(Threads REQUIRED)
    list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/src/server/ua_nodestore_concurrent.c)
else()
    list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/src/server/ua_nodestore.c)
endif()

option(ENABLE_EXTERNAL_NAMESPACES "Enable namespace handling by an external component (experimental)" OFF)
if(ENABLE_EXTERNAL_NAMESPACES)
    add_definitions(-DUA_EXTERNAL_NAMESPACES)
endif()

## set the precompiler flags
configure_file("include/ua_config.h.in" "${PROJECT_BINARY_DIR}/src_generated/ua_config.h")

## extensions
option(EXTENSION_UDP "Enable udp extension" OFF)
if(EXTENSION_UDP)
	add_definitions(-DEXTENSION_STATELESS)
	message(STATUS "Extensions: enabling udp")
	add_definitions(-DEXTENSION_UDP)
	add_executable(exampleServerUDP $<TARGET_OBJECTS:open62541-object> examples/networklayer_udp.c examples/server_udp.c)
    target_link_libraries(exampleServerUDP rt)
endif()

option(EXTENSION_STATELESS "Enable stateless extension" OFF)
if(EXTENSION_STATELESS)
	message(STATUS "Extensions: enabling stateless")
	add_definitions(-DEXTENSION_STATELESS)
endif()

option(ENABLE_AMALGAMATION "Concatenate the library to a single file open62541.h/.c" OFF)
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/open62541.h
               PRE_BUILD
               COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/amalgamate.py ${GIT_COMMIT_ID} ${CMAKE_CURRENT_BINARY_DIR}/open62541.h ${exported_headers}


               DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/amalgamate.py ${exported_headers} ${internal_headers})

add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/open62541.c
               PRE_BUILD

               COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/amalgamate.py ${GIT_COMMIT_ID} ${CMAKE_CURRENT_BINARY_DIR}/open62541.c
                              ${PROJECT_BINARY_DIR}/src_generated/ua_config.h ${internal_headers} ${PROJECT_SOURCE_DIR}/src/server/ua_nodestore_hash.inc ${lib_sources}

               DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/amalgamate.py ${PROJECT_BINARY_DIR}/src_generated/ua_config.h ${PROJECT_SOURCE_DIR}/src/server/ua_nodestore_hash.inc ${lib_sources})

if(ENABLE_AMALGAMATION)
    add_custom_target(amalgamation ALL DEPENDS ${PROJECT_BINARY_DIR}/open62541.h)
    add_library(open62541-object OBJECT ${PROJECT_BINARY_DIR}/open62541.c)
    include_directories(${PROJECT_BINARY_DIR})
else()
    add_definitions(-DNOT_AMALGATED)
    add_library(open62541-object OBJECT ${lib_sources} ${internal_headers} ${exported_headers})
    include_directories(${PROJECT_SOURCE_DIR}/include)
    include_directories(${PROJECT_SOURCE_DIR}/deps)
    include_directories(${PROJECT_SOURCE_DIR}/src)
    include_directories(${PROJECT_BINARY_DIR}/src_generated)
endif()
target_compile_definitions(open62541-object PRIVATE -DUA_DYNAMIC_LINKING)
add_library(open62541 SHARED $<TARGET_OBJECTS:open62541-object>)
target_compile_definitions(open62541 PRIVATE -DUA_DYNAMIC_LINKING)
if(WIN32)
    target_link_libraries(open62541 ws2_32) #since networklayer_tcp is linked into the amalgate
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
    set(server_source $<TARGET_OBJECTS:open62541-object>)
    add_executable(server_static examples/server.c ${server_source})
    add_executable(server_simple examples/server_simple.c ${server_source})
    add_executable(server examples/server.c)
	target_link_libraries(server open62541)
    if(WIN32)
        target_link_libraries(server_static ws2_32)
        target_link_libraries(server_simple ws2_32)
        target_link_libraries(server ws2_32)
    else()
        target_link_libraries(server_static rt)
        target_link_libraries(server_simple rt)
        target_link_libraries(server rt)
    endif()
    if(ENABLE_MULTITHREADING)
        target_link_libraries(server_static urcu-cds urcu urcu-common pthread)
        target_link_libraries(server_simple urcu-cds urcu urcu-common pthread)
        target_link_libraries(server urcu-cds urcu urcu-common pthread)
    endif()
endif()

## self-signed certificates
option(GENERATE_SELFSIGNED "Generate self-signed certificates" OFF)
if(GENERATE_SELFSIGNED)
    message(STATUS "Enabling self-signed certificates")
    find_package(OpenSSL REQUIRED)
    add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/server_cert.der
                              ${PROJECT_BINARY_DIR}/ca.crt
                   COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/certs/create_self-signed.py ${PROJECT_BINARY_DIR}
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/certs/create_self-signed.py
                           ${CMAKE_CURRENT_SOURCE_DIR}/tools/certs/localhost.cnf)
    add_custom_target(selfsigned ALL DEPENDS ${PROJECT_BINARY_DIR}/server_cert.der ${PROJECT_BINARY_DIR}/ca.crt)
endif()

# build example client
option(BUILD_EXAMPLECLIENT "Build a test client" OFF)
if(BUILD_EXAMPLECLIENT)
	add_definitions(-DBENCHMARK)
    set(client_source $<TARGET_OBJECTS:open62541-object>)
	add_executable(client_static examples/client.c ${client_source})
	add_executable(client examples/client.c)
	target_link_libraries(client open62541)
    if(WIN32)
        target_link_libraries(client_static ws2_32)
        target_link_libraries(client ws2_32)
    else()
        target_link_libraries(client_static rt)
        target_link_libraries(client rt)
    endif()
    if(ENABLE_MULTITHREADING)
        target_link_libraries(client_static urcu-cds urcu urcu-common pthread)
        target_link_libraries(client urcu-cds urcu urcu-common pthread)
    endif()
    if(EXTENSION_STATELESS)
        add_executable(client_stateless examples/client_stateless.c ${client_source})
        if(ENABLE_MULTITHREADING)
            target_link_libraries(client_stateless urcu-cds urcu urcu-common pthread)
        endif()
    endif()
endif()

# build unit tests
option(BUILD_UNIT_TESTS "Run unit tests after building" OFF)
if(BUILD_UNIT_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# build documentation
option(BUILD_DOCUMENTATION "Generate doxygen documentation" OFF)
if(BUILD_DOCUMENTATION)
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
