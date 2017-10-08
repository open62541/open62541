cmake_minimum_required(VERSION 3.0)
project(open62541)
# set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/tools/cmake")
find_package(PythonInterp REQUIRED)
find_package(Git)
include(AssignSourceGroup)

#############################
# Compiled binaries folders #
#############################

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(LIB_INSTALL_DIR /usr/lib CACHE PATH "Installation path of libraries")

###########
# Version #
###########

set(OPEN62541_VER_MAJOR 0)
set(OPEN62541_VER_MINOR 3)
set(OPEN62541_VER_PATCH 0)
set(OPEN62541_VER_LABEL "dev") # Appended to the X.Y.Z version format. For example "-rc1" or an empty string

# Set OPEN62541_VER_COMMIT
if(GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --always --tags
                    RESULT_VARIABLE res_var OUTPUT_VARIABLE GIT_COM_ID WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    if(${res_var} EQUAL 0)
        string(REPLACE "\n" "" OPEN62541_VER_COMMIT ${GIT_COM_ID} )
    endif()
endif()
if(NOT OPEN62541_VER_COMMIT OR OPEN62541_VER_COMMIT STREQUAL "")
    set(OPEN62541_VER_COMMIT "undefined")
endif()

#################
# Build Options #
#################

# Set default build type.
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "CMAKE_BUILD_TYPE not given; setting to 'Debug'")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build" FORCE)
endif()

# Options
set(UA_LOGLEVEL 300 CACHE STRING "Level at which logs shall be reported")
option(UA_ENABLE_METHODCALLS "Enable the Method service set" ON)
option(UA_ENABLE_NODEMANAGEMENT "Enable dynamic addition and removal of nodes at runtime" ON)
option(UA_ENABLE_SUBSCRIPTIONS "Enable subscriptions support." ON)
option(UA_ENABLE_DISCOVERY "Enable Discovery Service (LDS)" ON)
option(UA_ENABLE_DISCOVERY_MULTICAST "Enable Discovery Service with multicast support (LDS-ME)" OFF)
# Semaphores/file system may not be available on embedded devices. It can be disabled with the following option
option(UA_ENABLE_DISCOVERY_SEMAPHORE "Enable Discovery Semaphore support" ON)
mark_as_advanced(UA_ENABLE_DISCOVERY_SEMAPHORE)
option(UA_ENABLE_AMALGAMATION "Concatenate the library to a single file open62541.h/.c" OFF)
option(UA_ENABLE_COVERAGE "Enable gcov coverage" OFF)
option(BUILD_SHARED_LIBS "Enable building of shared libraries (dll/so)" OFF)

if(UA_ENABLE_COVERAGE)
  set(CMAKE_BUILD_TYPE DEBUG)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage -lgcov")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
endif()

if(UA_ENABLE_DISCOVERY_MULTICAST AND NOT UA_ENABLE_DISCOVERY)
    MESSAGE(WARNING "UA_ENABLE_DISCOVERY_MULTICAST is enabled, but not UA_ENABLE_DISCOVERY. UA_ENABLE_DISCOVERY_MULTICAST will be set to OFF")
    SET(UA_ENABLE_DISCOVERY_MULTICAST OFF CACHE BOOL "Enable Discovery Service with multicast support (LDS-ME)" FORCE)
endif()

# Advanced options
option(UA_ENABLE_MULTITHREADING "Enable multithreading (experimental)" OFF)
mark_as_advanced(UA_ENABLE_MULTITHREADING)

option(UA_ENABLE_STATUSCODE_DESCRIPTIONS "Enable conversion of StatusCode to human-readable error message" ON)
mark_as_advanced(UA_ENABLE_STATUSCODE_DESCRIPTIONS)

option(UA_ENABLE_TYPENAMES "Add the type and member names to the UA_DataType structure" ON)
mark_as_advanced(UA_ENABLE_TYPENAMES)

option(UA_ENABLE_EMBEDDED_LIBC "Use a custom implementation of some libc functions that might be missing on embedded targets (e.g. string handling)." OFF)
mark_as_advanced(UA_ENABLE_EMBEDDED_LIBC)

option(UA_ENABLE_DETERMINISTIC_RNG "Do not seed the random number generator (e.g. for unit tests)." OFF)
mark_as_advanced(UA_ENABLE_DETERMINISTIC_RNG)

option(UA_ENABLE_GENERATE_NAMESPACE0 "Generate and load UA XML Namespace 0 definition (experimental)" OFF)
mark_as_advanced(UA_ENABLE_GENERATE_NAMESPACE0)
set(UA_DATATYPES_FILE ${PROJECT_SOURCE_DIR}/tools/schema/datatypes_minimal.txt CACHE FILEPATH "File containing the list of datatypes added to the server")
mark_as_advanced(UA_DATATYPES_FILE)

option(UA_ENABLE_VALGRIND_UNIT_TESTS "Use Valgrind to detect memory leaks when running the unit tests" OFF)
mark_as_advanced(UA_ENABLE_VALGRIND_UNIT_TESTS)

option(UA_ENABLE_NONSTANDARD_UDP "Enable udp extension (non-standard)" OFF)
mark_as_advanced(UA_ENABLE_NONSTANDARD_UDP)

# Build Targets
option(UA_BUILD_EXAMPLES "Build example servers and clients" OFF)
option(UA_BUILD_UNIT_TESTS "Build the unit tests" OFF)
option(UA_BUILD_FUZZING "Build the fuzzing executables" OFF)
mark_as_advanced(UA_BUILD_FUZZING)
if (UA_BUILD_FUZZING)
    # oss-fuzz already defines this by default
    add_definitions(-DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
endif()
option(UA_BUILD_OSS_FUZZ "Special build switch used in oss-fuzz" OFF)
mark_as_advanced(UA_BUILD_OSS_FUZZ)
option(UA_DEBUG_DUMP_PKGS "Dump every package received by the server as hexdump format" OFF)
mark_as_advanced(UA_DEBUG_DUMP_PKGS)
option(UA_BUILD_EXAMPLES_NODESET_COMPILER "Generate an OPC UA information model from a nodeset XML (experimental)" OFF)

# Advanced Build Targets
option(UA_BUILD_SELFSIGNED_CERTIFICATE "Generate self-signed certificate" OFF)
mark_as_advanced(UA_BUILD_SELFSIGNED_CERTIFICATE)

# Building shared libs (dll, so). This option is written into ua_config.h.
set(UA_DYNAMIC_LINKING OFF)
if(BUILD_SHARED_LIBS)
  set(UA_DYNAMIC_LINKING ON)
  if (UA_ENABLE_DISCOVERY_MULTICAST)
      set(MDNSD_DYNAMIC_LINKING ON)
  endif()
endif()

# Force compilation with as C++
option(UA_COMPILE_AS_CXX "Force compilation with a C++ compiler" OFF)
mark_as_advanced(UA_COMPILE_AS_CXX)

#####################
# Compiler Settings #
#####################

# Collect libraries
list(APPEND open62541_LIBRARIES "")
if(NOT WIN32)
  if(QNXNTO)
    list(APPEND open62541_LIBRARIES socket)
    list(APPEND open62541_LIBRARIES c)
    list(APPEND open62541_LIBRARIES stdc++)
  else()
    list(APPEND open62541_LIBRARIES pthread m)
    if(NOT APPLE)
      list(APPEND open62541_LIBRARIES rt)
    endif()
  endif()
else()
  list(APPEND open62541_LIBRARIES ws2_32)
    if(UA_ENABLE_DISCOVERY_MULTICAST)
        list(APPEND open62541_LIBRARIES iphlpapi)
    endif()
endif()
if(UA_ENABLE_MULTITHREADING)
  list(APPEND open62541_LIBRARIES urcu-cds urcu-bp urcu-common)
endif(UA_ENABLE_MULTITHREADING)

if(NOT UA_COMPILE_AS_CXX AND (CMAKE_COMPILER_IS_GNUCC OR "x${CMAKE_C_COMPILER_ID}" STREQUAL "xClang"))
    # Compiler
    add_definitions(-std=c99 -pipe
                    -Wall -Wextra -Werror -Wpedantic
                    -Wno-static-in-inline # clang doesn't like the use of static inline methods inside static inline methods
                    -Wno-overlength-strings # may happen in the nodeset compiler when complex values are directly encoded
                    -Wno-unused-parameter # some methods may require unused arguments to cast to a method pointer
                    -Wmissing-prototypes -Wstrict-prototypes -Wredundant-decls
                    -Wformat -Wformat-security -Wformat-nonliteral
                    -Wuninitialized -Winit-self
                    -Wcast-qual
                    -Wstrict-overflow
                    -Wnested-externs
                    -Wmultichar
                    -Wundef
                    -Wc++-compat)
     if(NOT WIN32 AND NOT CYGWIN AND NOT QNXNTO)
        add_definitions(-Wshadow -Wconversion -fvisibility=hidden -fPIC)
     endif()

     if(UA_ENABLE_AMALGAMATION)
         add_definitions(-Wno-unused-function)
     endif()

    # Linker
    set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "") # cmake sets -rdynamic by default

    # Debug
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        # add_definitions(-fsanitize=address)
        # list(APPEND open62541_LIBRARIES asan)
        # add_definitions(-fsanitize=undefined)
        # list(APPEND open62541_LIBRARIES ubsan)
    endif()

    # Strip release builds
    if(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel" OR CMAKE_BUILD_TYPE STREQUAL "Release")
        add_definitions(-ffunction-sections -fdata-sections -fno-stack-protector -fno-unwind-tables
                        -fno-asynchronous-unwind-tables -fno-math-errno -fmerge-all-constants -fno-ident)
        set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -s")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -s")
        if(APPLE)
            set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,-dead_strip")
            set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-dead_strip")
        else()
            set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--gc-sections")
            set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections")
        endif()
        if(NOT WIN32 AND NOT CYGWIN AND NOT APPLE)
            # these settings reduce the binary size by ~2kb
            set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,-z,norelro -Wl,--hash-style=gnu -Wl,--build-id=none")
        endif()
    endif()
endif()

if(APPLE)
    set(CMAKE_MACOSX_RPATH 1)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_DARWIN_C_SOURCE=1")
endif()

if(MSVC)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3 /WX") # Compiler warnings, error on warning

  if(NOT BUILD_SHARED_LIB)
    set(CompilerFlags CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE)
    foreach(CompilerFlag ${CompilerFlags})
      string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
    endforeach()
  endif()
endif()

#########################
# Generate Main Library #
#########################

file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/src_generated")
configure_file("include/ua_config.h.in" "${PROJECT_BINARY_DIR}/src_generated/ua_config.h")
configure_file("include/open62541.pc.in" "${PROJECT_BINARY_DIR}/src_generated/open62541.pc" @ONLY)

if(UA_ENABLE_DISCOVERY_MULTICAST)
    set(MDNSD_LOGLEVEL 300 CACHE STRING "Level at which logs shall be reported" FORCE)
    configure_file("deps/mdnsd/libmdnsd/mdnsd_config.h.in" "${PROJECT_BINARY_DIR}/src_generated/mdnsd_config.h")
endif()

include_directories(${PROJECT_BINARY_DIR}/src_generated
                    ${PROJECT_SOURCE_DIR}/include
                    ${PROJECT_SOURCE_DIR}/plugins # TODO: discovery depends on the default config
                    ${PROJECT_SOURCE_DIR}/deps)

set(exported_headers ${PROJECT_BINARY_DIR}/src_generated/ua_config.h
                     ${PROJECT_SOURCE_DIR}/deps/ms_stdint.h
                     ${PROJECT_SOURCE_DIR}/include/ua_constants.h
                     ${PROJECT_SOURCE_DIR}/include/ua_types.h
                     ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated.h
                     ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated_handling.h
                     ${PROJECT_SOURCE_DIR}/include/ua_server.h
                     ${PROJECT_SOURCE_DIR}/include/ua_plugin_network.h
                     ${PROJECT_SOURCE_DIR}/include/ua_plugin_log.h
                     ${PROJECT_SOURCE_DIR}/include/ua_plugin_access_control.h
                     ${PROJECT_SOURCE_DIR}/include/ua_plugin_securitypolicy.h
                     ${PROJECT_SOURCE_DIR}/include/ua_plugin_nodestore.h
                     ${PROJECT_SOURCE_DIR}/include/ua_server_config.h
                     ${PROJECT_SOURCE_DIR}/include/ua_client.h
                     ${PROJECT_SOURCE_DIR}/include/ua_client_highlevel.h
)
set(internal_headers ${PROJECT_SOURCE_DIR}/deps/queue.h
                     ${PROJECT_SOURCE_DIR}/deps/pcg_basic.h
                     ${PROJECT_SOURCE_DIR}/deps/libc_time.h
                     ${PROJECT_SOURCE_DIR}/src/ua_util.h
                     ${PROJECT_SOURCE_DIR}/src/ua_types_encoding_binary.h
                     ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated_encoding_binary.h
                     ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated.h
                     ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated_handling.h
                     ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated_encoding_binary.h
                     ${PROJECT_SOURCE_DIR}/src/ua_connection_internal.h
                     ${PROJECT_SOURCE_DIR}/src/ua_securechannel.h
                     ${PROJECT_SOURCE_DIR}/src/ua_session.h
                     ${PROJECT_SOURCE_DIR}/src/ua_timer.h
                     ${PROJECT_SOURCE_DIR}/src/server/ua_subscription.h
                     ${PROJECT_SOURCE_DIR}/src/server/ua_session_manager.h
                     ${PROJECT_SOURCE_DIR}/src/server/ua_securechannel_manager.h
                     ${PROJECT_SOURCE_DIR}/src/server/ua_server_internal.h
                     ${PROJECT_SOURCE_DIR}/src/server/ua_services.h
                     ${PROJECT_SOURCE_DIR}/src/client/ua_client_internal.h)

# TODO: make client optional
set(lib_sources ${PROJECT_SOURCE_DIR}/src/ua_types.c
                ${PROJECT_SOURCE_DIR}/src/ua_types_encoding_binary.c
                ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated.c
                ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated.c
                ${PROJECT_BINARY_DIR}/src_generated/ua_statuscode_descriptions.c
                ${PROJECT_SOURCE_DIR}/src/ua_util.c
                ${PROJECT_SOURCE_DIR}/src/ua_timer.c
                ${PROJECT_SOURCE_DIR}/src/ua_session.c
                ${PROJECT_SOURCE_DIR}/src/ua_connection.c
                ${PROJECT_SOURCE_DIR}/src/ua_securechannel.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_nodes.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_server.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_server_ns0.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_server_binary.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_server_utils.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_server_worker.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_server_discovery.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_securechannel_manager.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_session_manager.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_subscription.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_subscription_datachange.c
                # services
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_view.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_call.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_session.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_attribute.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_discovery.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_subscription.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_securechannel.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_nodemanagement.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_discovery_multicast.c
                # client
                ${PROJECT_SOURCE_DIR}/src/client/ua_client.c
                ${PROJECT_SOURCE_DIR}/src/client/ua_client_connect.c
                ${PROJECT_SOURCE_DIR}/src/client/ua_client_discovery.c
                ${PROJECT_SOURCE_DIR}/src/client/ua_client_highlevel.c
                ${PROJECT_SOURCE_DIR}/src/client/ua_client_highlevel_subscriptions.c

                # dependencies
                ${PROJECT_SOURCE_DIR}/deps/libc_time.c
                ${PROJECT_SOURCE_DIR}/deps/pcg_basic.c)

set(default_plugin_headers ${PROJECT_SOURCE_DIR}/plugins/ua_network_tcp.h
                           ${PROJECT_SOURCE_DIR}/plugins/ua_accesscontrol_default.h
                           ${PROJECT_SOURCE_DIR}/plugins/ua_log_stdout.h
                           ${PROJECT_SOURCE_DIR}/plugins/ua_nodestore_default.h
                           ${PROJECT_SOURCE_DIR}/plugins/ua_config_default.h
                           ${PROJECT_SOURCE_DIR}/plugins/ua_securitypolicy_none.h)

set(default_plugin_sources ${PROJECT_SOURCE_DIR}/plugins/ua_network_tcp.c
                           ${PROJECT_SOURCE_DIR}/plugins/ua_clock.c
                           ${PROJECT_SOURCE_DIR}/plugins/ua_log_stdout.c
                           ${PROJECT_SOURCE_DIR}/plugins/ua_accesscontrol_default.c
                           ${PROJECT_SOURCE_DIR}/plugins/ua_nodestore_default.c
                           ${PROJECT_SOURCE_DIR}/plugins/ua_config_default.c
                           ${PROJECT_SOURCE_DIR}/plugins/ua_securitypolicy_none.c)

if(UA_DEBUG_DUMP_PKGS)
    list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/plugins/ua_debug_dump_pkgs.c)
endif()

if(UA_ENABLE_EMBEDDED_LIBC)
  list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/deps/libc_string.c)
endif()

if(UA_ENABLE_GENERATE_NAMESPACE0)
  set(GENERATE_NAMESPACE0_FILE "Opc.Ua.NodeSet2.xml" CACHE STRING "Namespace definition XML file")
  set_property(CACHE GENERATE_NAMESPACE0_FILE PROPERTY STRINGS Opc.Ua.NodeSet2.xml Opc.Ua.NodeSet2.Minimal.xml)
  list(APPEND internal_headers ${PROJECT_BINARY_DIR}/src_generated/ua_namespaceinit_generated.h)
  list(APPEND lib_sources ${PROJECT_BINARY_DIR}/src_generated/ua_namespaceinit_generated.c)
endif()

if(UA_ENABLE_NONSTANDARD_UDP)
    list(APPEND exported_headers ${PROJECT_SOURCE_DIR}/plugins/ua_network_udp.h)
endif()

if(UA_ENABLE_DISCOVERY_MULTICAST)
    # prepend in list, otherwise it complains that winsock2.h has to be included before windows.h
    set(internal_headers ${PROJECT_BINARY_DIR}/src_generated/mdnsd_config.h
                         ${PROJECT_SOURCE_DIR}/deps/mdnsd/libmdnsd/1035.h
                         ${PROJECT_SOURCE_DIR}/deps/mdnsd/libmdnsd/xht.h
                         ${PROJECT_SOURCE_DIR}/deps/mdnsd/libmdnsd/sdtxt.h
                         ${PROJECT_SOURCE_DIR}/deps/mdnsd/libmdnsd/mdnsd.h
                         ${internal_headers} )
    list(APPEND internal_headers ${PROJECT_SOURCE_DIR}/src/server/ua_mdns_internal.h)
    set(lib_sources ${PROJECT_SOURCE_DIR}/src/server/ua_mdns.c
                    ${PROJECT_SOURCE_DIR}/deps/mdnsd/libmdnsd/1035.c
                    ${PROJECT_SOURCE_DIR}/deps/mdnsd/libmdnsd/xht.c
                    ${PROJECT_SOURCE_DIR}/deps/mdnsd/libmdnsd/sdtxt.c
                    ${PROJECT_SOURCE_DIR}/deps/mdnsd/libmdnsd/mdnsd.c
                    ${lib_sources})
endif()

#########################
# Generate source files #
#########################

if(UA_DATATYPES_FILE STREQUAL "")
  set(SELECTED_TYPES_TMP "")
else()
  set(SELECTED_TYPES_TMP "--selected-types=${UA_DATATYPES_FILE}")
endif()

# standard-defined data types
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated.c
                          ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated.h
                          ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated_handling.h
                          ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated_encoding_binary.h
                   PRE_BUILD
                   COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/generate_datatypes.py
                           --type-csv=${PROJECT_SOURCE_DIR}/tools/schema/NodeIds.csv
                           ${SELECTED_TYPES_TMP}
                           --type-bsd=${PROJECT_SOURCE_DIR}/tools/schema/Opc.Ua.Types.bsd
                           ${PROJECT_BINARY_DIR}/src_generated/ua_types
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_datatypes.py
                           ${PROJECT_SOURCE_DIR}/tools/schema/NodeIds.csv
                           ${PROJECT_SOURCE_DIR}/tools/schema/Opc.Ua.Types.bsd
                           ${SELECTED_TYPES})
# we need a custom target to avoid that the generator is called concurrently and thus overwriting files while the other thread is compiling
add_custom_target(open62541-generator-types DEPENDS
                  ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated.c
                  ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated.h
                  ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated_handling.h
                  ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated_encoding_binary.h)

# transport data types
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated.c
                          ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated.h
                          ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated_handling.h
                          ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated_encoding_binary.h
                   PRE_BUILD
                   COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/generate_datatypes.py
                           --namespace=1
                           --selected-types=${PROJECT_SOURCE_DIR}/tools/schema/datatypes_transport.txt
                           --type-bsd=${PROJECT_SOURCE_DIR}/tools/schema/Opc.Ua.Types.bsd
                           --type-bsd=${PROJECT_SOURCE_DIR}/tools/schema/Custom.Opc.Ua.Transport.bsd
                           --no-builtin
                           ${PROJECT_BINARY_DIR}/src_generated/ua_transport
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_datatypes.py
                           ${PROJECT_SOURCE_DIR}/tools/schema/datatypes_transport.txt
                           ${PROJECT_SOURCE_DIR}/tools/schema/Opc.Ua.Types.bsd
                           ${PROJECT_SOURCE_DIR}/tools/schema/Custom.Opc.Ua.Transport.bsd)
# we need a custom target to avoid that the generator is called concurrently and thus overwriting files while the other thread is compiling
add_custom_target(open62541-generator-transport DEPENDS
        ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated.c
        ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated.h
        ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated_handling.h
        ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated_encoding_binary.h)

# statuscode explanation
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/ua_statuscode_descriptions.c
        PRE_BUILD
        COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/generate_statuscode_descriptions.py
        ${PROJECT_SOURCE_DIR}/tools/schema/Opc.Ua.StatusCodes.csv ${PROJECT_BINARY_DIR}/src_generated/ua_statuscode_descriptions
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_statuscode_descriptions.py
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/schema/Opc.Ua.StatusCodes.csv)
# we need a custom target to avoid that the generator is called concurrently and thus overwriting files while the other thread is compiling
add_custom_target(open62541-generator-statuscode DEPENDS
        ${PROJECT_BINARY_DIR}/src_generated/ua_statuscode_descriptions.c)

# single-file release
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/open62541.h
                   PRE_BUILD
                   COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/amalgamate.py
                           ${OPEN62541_VER_COMMIT} ${CMAKE_CURRENT_BINARY_DIR}/open62541.h
                           ${exported_headers} ${default_plugin_headers}
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/amalgamate.py
                           ${exported_headers} ${default_plugin_headers})

add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/open62541.c
                   PRE_BUILD
                   COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/amalgamate.py
                           ${OPEN62541_VER_COMMIT} ${CMAKE_CURRENT_BINARY_DIR}/open62541.c
                           ${internal_headers} ${lib_sources} ${default_plugin_sources}
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/amalgamate.py ${internal_headers}
                           ${lib_sources})

add_custom_target(open62541-amalgamation-source DEPENDS ${PROJECT_BINARY_DIR}/open62541.c)
add_custom_target(open62541-amalgamation-header DEPENDS ${PROJECT_BINARY_DIR}/open62541.h)

add_dependencies(open62541-amalgamation-header open62541-generator-types)
add_dependencies(open62541-amalgamation-source open62541-generator-types
                 open62541-generator-transport open62541-generator-statuscode)

# generated namespace 0
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/ua_namespaceinit_generated.c
        ${PROJECT_BINARY_DIR}/src_generated/ua_namespaceinit_generated.h
        PRE_BUILD
        COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/generate_open62541CCode.py
        -i ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/NodeID_AssumeExternal.txt
        -s description -b ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/NodeID_Blacklist.txt
        ${PROJECT_SOURCE_DIR}/tools/schema/namespace0/${GENERATE_NAMESPACE0_FILE}
        ${PROJECT_BINARY_DIR}/src_generated/ua_namespaceinit_generated
        DEPENDS ${PROJECT_SOURCE_DIR}/tools/schema/namespace0/${GENERATE_NAMESPACE0_FILE}
        ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/generate_open62541CCode.py
        ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/open62541_MacroHelper.py
        ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/ua_builtin_types.py
        ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/ua_constants.py
        ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/ua_namespace.py
        ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/ua_node_types.py)
# we need a custom target to avoid that the generator is called concurrently and thus overwriting files while the other thread is compiling
add_custom_target(open62541-generator-namespace DEPENDS
        ${PROJECT_BINARY_DIR}/src_generated/ua_namespaceinit_generated.c
        ${PROJECT_BINARY_DIR}/src_generated/ua_namespaceinit_generated.h)

#####################
# Build the Library #
#####################

assign_source_group(${lib_sources})
assign_source_group(${internal_headers})
assign_source_group(${exported_headers})
assign_source_group(${default_plugin_sources})

if(UA_ENABLE_AMALGAMATION)
    add_library(open62541-object OBJECT ${PROJECT_BINARY_DIR}/open62541.c ${PROJECT_BINARY_DIR}/open62541.h)
    target_include_directories(open62541-object PRIVATE ${PROJECT_BINARY_DIR})

    # make sure the open62541_amalgamation target builds before so that amalgamation is finished and it is not executed again for open62541-object
    # and thus may overwrite the amalgamation result during multiprocessor compilation
    # the header is already a dependency of open62541 target itself
    add_dependencies(open62541-object
                     open62541-amalgamation-header
                     open62541-generator-types
                     open62541-generator-transport
                     open62541-generator-statuscode
                     open62541-amalgamation-source
                     )

    add_library(open62541 $<TARGET_OBJECTS:open62541-object>)

    if(UA_COMPILE_AS_CXX)
        set_source_files_properties(${PROJECT_BINARY_DIR}/open62541.c PROPERTIES LANGUAGE CXX)
    endif()
else()
    add_definitions(-DUA_NO_AMALGAMATION)
    add_library(open62541-object OBJECT ${lib_sources} ${internal_headers} ${exported_headers})
    add_dependencies(open62541-object
                     open62541-amalgamation-header
                     open62541-generator-types
                     open62541-generator-transport
                     open62541-generator-statuscode)
    target_include_directories(open62541-object PRIVATE ${PROJECT_SOURCE_DIR}/src)

    add_library(open62541-plugins OBJECT ${default_plugin_sources} ${exported_headers})
    add_dependencies(open62541-plugins open62541-generator-types open62541-generator-transport)
    target_include_directories(open62541-plugins PRIVATE ${PROJECT_SOURCE_DIR}/plugins)
    target_include_directories(open62541-plugins PRIVATE ${PROJECT_BINARY_DIR}/src_generated)
    target_compile_definitions(open62541-plugins PRIVATE -DUA_DYNAMIC_LINKING_EXPORT)
    set_target_properties(open62541-plugins PROPERTIES FOLDER "open62541/lib")

    add_library(open62541 $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-plugins>)

    if(UA_COMPILE_AS_CXX)
        set_source_files_properties(${lib_sources} PROPERTIES LANGUAGE CXX)
        set_source_files_properties(${default_plugin_sources} PROPERTIES LANGUAGE CXX)
    endif()
endif()

if(UA_ENABLE_GENERATE_NAMESPACE0)
    add_dependencies(open62541-amalgamation-source open62541-generator-namespace)
    add_dependencies(open62541-amalgamation-header open62541-generator-namespace)
    if(NOT UA_ENABLE_AMALGAMATION)
        add_dependencies(open62541-object open62541-generator-namespace)
    endif()
endif()

# Export Symbols
target_compile_definitions(open62541-object PRIVATE -DUA_DYNAMIC_LINKING_EXPORT)
target_compile_definitions(open62541 PRIVATE -DUA_DYNAMIC_LINKING_EXPORT)
if (UA_ENABLE_DISCOVERY_MULTICAST)
    target_compile_definitions(open62541-object PRIVATE -DMDNSD_DYNAMIC_LINKING_EXPORT)
    target_compile_definitions(open62541 PRIVATE -DMDNSD_DYNAMIC_LINKING_EXPORT)
endif()

# DLL requires linking to dependencies
target_link_libraries(open62541 ${open62541_LIBRARIES})

# Generate properly versioned shared library links on Linux
SET_TARGET_PROPERTIES(open62541 PROPERTIES SOVERSION 0 VERSION "${OPEN62541_VER_MAJOR}.${OPEN62541_VER_MINOR}.${OPEN62541_VER_PATCH}")

##########################
# Build Selected Targets #
##########################

# always include, builds with make doc
add_subdirectory(doc)

if(UA_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(UA_BUILD_UNIT_TESTS)
    if(UA_ENABLE_AMALGAMATION)
        # Cannot compile tests with amalgamation. Amalgamation uses the default plugins, not the testing plugins
        message(FATAL_ERROR "Unit tests cannot be generated with source amalgamation enabled")
    endif()
    enable_testing()
    add_subdirectory(tests)
endif()

if(UA_BUILD_FUZZING OR UA_BUILD_OSS_FUZZ)
    # Force enable discovery, to also fuzzy-test this code
    set(UA_ENABLE_DISCOVERY ON CACHE STRING "" FORCE)
    set(UA_ENABLE_DISCOVERY_MULTICAST ON CACHE STRING "" FORCE)
    add_subdirectory(tests/fuzz)
endif()

############################
# Linting run (clang-tidy) #
############################

find_package(ClangTools)
add_custom_target(lint ${CLANG_TIDY_PROGRAM}
                  ${lib_sources}
                  -checks=cert-*,performance-*,readability-*,-readability-braces-around-statements
                  -warnings-as-errors=cert-*,performance-*,readability-*,-readability-braces-around-statements
                  --
                  -std=c99
                  -I${PROJECT_SOURCE_DIR}/include
                  -I${PROJECT_SOURCE_DIR}/plugins
                  -I${PROJECT_SOURCE_DIR}/deps
                  -I${PROJECT_SOURCE_DIR}/src
                  -I${PROJECT_SOURCE_DIR}/src/server
                  -I${PROJECT_SOURCE_DIR}/src/client
                  -I${PROJECT_BINARY_DIR}/src_generated
                  -DUA_NO_AMALGAMATION
                  DEPENDS ${lib_sources}
                  COMMENT "Run clang-tidy on the library")
add_dependencies(lint open62541)

##########################
# Installation           #
##########################
# invoke via `make install`
# specify install location with `-DCMAKE_INSTALL_PREFIX=xyz`
# Enable shared library with `-DBUILD_SHARED_LIBS=ON`

set(cmake_configfile_install ${LIB_INSTALL_DIR}/cmake3)
set(target_install_dest_name "${cmake_configfile_install}/open62541Targets.cmake")
set(open62541_tools_dir share/open62541/tools)
set(open62541_deps_dir include/open62541/deps)

# export library (either static or shared depending on BUILD_SHARED_LIBS)
install(TARGETS open62541
        EXPORT open62541Targets
        LIBRARY DESTINATION ${LIB_INSTALL_DIR}
        ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
        INCLUDES DESTINATION ${INCLUDE_INSTALL_DIR}/open62541 ${open62541_deps_dir})

include(CMakePackageConfigHelpers)
configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/tools/cmake/open62541-config.cmake.in"
                              "${CMAKE_CURRENT_BINARY_DIR}/cmake/open62541-config.cmake"
                              INSTALL_DESTINATION "${cmake_configfile_install}"
                              PATH_VARS target_install_dest_name open62541_tools_dir)

set(open62541_VERSION)
get_target_property(open62541_VERSION open62541 VERSION)

write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/open62541ConfigVersion.cmake"
                                 VERSION ${open62541_VERSION}
                                 COMPATIBILITY AnyNewerVersion)

install(EXPORT open62541Targets
        FILE open62541Targets.cmake
        DESTINATION "${cmake_configfile_install}")

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/cmake/open62541-config.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/open62541ConfigVersion.cmake"
        DESTINATION "${cmake_configfile_install}")


if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    install(FILES "${PROJECT_BINARY_DIR}/src_generated/open62541.pc"
            DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)
endif()
# export amalgamated header open62541.h which is generated due to build of open62541-object
if(UA_ENABLE_AMALGAMATION)
    install(FILES ${PROJECT_BINARY_DIR}/open62541.h DESTINATION include/open62541)
endif()
install(DIRECTORY deps/ DESTINATION ${open62541_deps_dir})
install(DIRECTORY tools/ DESTINATION ${open62541_tools_dir} USE_SOURCE_PERMISSIONS)

##########################
# Packaging (DEB/RPM)    #
##########################
# invoke via `make package`

set(CPACK_GENERATOR "TGZ;DEB;RPM")
set(CPACK_PACKAGE_VENDOR "open62541 team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OPC UA implementation")
set(CPACK_PACKAGE_DESCRIPTION "open62541 is a C-based library (linking with C++ projects is possible) with all necessary tools to implement dedicated OPC UA clients and servers, or to integrate OPC UA-based communication into existing applications.")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "2")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "open62541 team") #required

include(CPack)

##################################
# Visual studio solution folders #
##################################

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "_CmakePredifinedTargets")

set_target_properties(open62541 PROPERTIES FOLDER "open62541/lib")
set_target_properties(open62541-object PROPERTIES FOLDER "open62541/lib")
set_target_properties(lint PROPERTIES FOLDER "CodeAnalysis")
set_target_properties(open62541-amalgamation-header PROPERTIES FOLDER "open62541/lib")
set_target_properties(open62541-amalgamation-source PROPERTIES FOLDER "open62541/lib")
