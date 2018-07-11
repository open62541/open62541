cmake_minimum_required(VERSION 3.0)
project(open62541)
# set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/tools/cmake")
find_package(PythonInterp REQUIRED)
find_package(Git)
include(AssignSourceGroup)
include(macros)

#############################
# Compiled binaries folders #
#############################

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

###########
# Version #
###########

set(OPEN62541_VER_MAJOR 0)
set(OPEN62541_VER_MINOR 4)
set(OPEN62541_VER_PATCH 0)
set(OPEN62541_VER_LABEL "-dev") # Appended to the X.Y.Z version format. For example "-rc1" or an empty string

#  If a relative path is specified, it is treated as relative to the $<INSTALL_PREFIX>
set(LIB_INSTALL_DIR lib CACHE PATH "Installation path of libraries")

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


option(UA_ENABLE_AMALGAMATION "Concatenate the library to a single file open62541.h/.c" OFF)
set(UA_AMALGAMATION_ARCHITECUTRES "" CACHE STRING "List of architectures to include in amalgamation")
mark_as_advanced(UA_AMALGAMATION_ARCHITECUTRES)

# Platform. This is at the beginning in case the architecture changes some UA options
set(UA_ARCHITECTURE "None" CACHE STRING "Architecture to build open62541 on")

if(UA_ENABLE_AMALGAMATION)
    if(NOT "${UA_ARCHITECTURE}" STREQUAL "None")
        set(UA_AMALGAMATION_ARCHITECUTRES "${UA_ARCHITECTURE}")
    else()
        if("${UA_AMALGAMATION_ARCHITECUTRES}" STREQUAL "")
            # select some default architectures which should be included
            set(UA_AMALGAMATION_ARCHITECUTRES "win32;posix")
        endif()
    endif()
    message(STATUS "Architectures included in amalgamation: ${UA_AMALGAMATION_ARCHITECUTRES}")
endif()

if("${UA_ARCHITECTURE}" STREQUAL "None")
    if(UNIX)
        set(UA_ARCHITECTURE "posix" CACHE STRING "" FORCE)
    elseif(WIN32)
        set(UA_ARCHITECTURE "win32" CACHE STRING ""  FORCE)
    endif(UNIX)
endif()

message(STATUS "The selected architecture is: ${UA_ARCHITECTURE}")
string(TOUPPER ${UA_ARCHITECTURE} UA_ARCHITECTURE_UPPER)
add_definitions(-DUA_ARCHITECTURE_${UA_ARCHITECTURE_UPPER})

add_subdirectory(arch)

GET_PROPERTY(architectures GLOBAL PROPERTY UA_ARCHITECTURES)
list(SORT architectures)
set_property(CACHE UA_ARCHITECTURE PROPERTY STRINGS None ${architectures})

GET_PROPERTY(ua_directories_to_include GLOBAL PROPERTY UA_INCLUDE_DIRECTORIES)
include_directories(${ua_directories_to_include})

GET_PROPERTY(ua_architecture_headers GLOBAL PROPERTY UA_ARCHITECTURE_HEADERS)

GET_PROPERTY(ua_architecture_headers_beginning GLOBAL PROPERTY UA_ARCHITECTURE_HEADERS_BEGINNING)

GET_PROPERTY(ua_architecture_sources GLOBAL PROPERTY UA_ARCHITECTURE_SOURCES)

set(ua_architecture_sources ${ua_architecture_sources}
            ${PROJECT_SOURCE_DIR}/arch/ua_network_tcp.c
)

set(ua_architecture_headers ${ua_architecture_headers}
            ${PROJECT_SOURCE_DIR}/arch/ua_network_tcp.h
            ${PROJECT_SOURCE_DIR}/arch/ua_architecture_functions.h
)



if(${UA_ARCHITECTURE} STREQUAL "None")
  message(FATAL_ERROR "No architecture was selected. Please select the architecture of your target platform")
endif(${UA_ARCHITECTURE} STREQUAL "None")

# Options
set(UA_LOGLEVEL 300 CACHE STRING "Level at which logs shall be reported")
option(UA_ENABLE_HISTORIZING "Enable server to provide historical access." ON)
option(UA_ENABLE_METHODCALLS "Enable the Method service set" ON)
option(UA_ENABLE_NODEMANAGEMENT "Enable dynamic addition and removal of nodes at runtime" ON)
option(UA_ENABLE_SUBSCRIPTIONS "Enable subscriptions support." ON)
option(UA_ENABLE_SUBSCRIPTIONS_EVENTS "Enable the use of events. (EXPERIMENTAL)" OFF)
option(UA_ENABLE_DISCOVERY "Enable Discovery Service (LDS)" ON)
option(UA_ENABLE_DISCOVERY_MULTICAST "Enable Discovery Service with multicast support (LDS-ME)" OFF)
option(UA_ENABLE_QUERY "Enable query support." OFF)
option(UA_ENABLE_COVERAGE "Enable gcov coverage" OFF)
option(UA_ENABLE_ENCRYPTION "Enable encryption support (uses mbedTLS)" OFF)
option(BUILD_SHARED_LIBS "Enable building of shared libraries (dll/so)" OFF)

# Namespace Zero
set(UA_NAMESPACE_ZERO "REDUCED" CACHE STRING "Completeness of the generated namespace zero (minimal/reduced/full)")
SET_PROPERTY(CACHE UA_NAMESPACE_ZERO PROPERTY STRINGS "MINIMAL" "REDUCED" "FULL")
if(UA_NAMESPACE_ZERO STREQUAL "MINIMAL")
    set(UA_GENERATED_NAMESPACE_ZERO OFF)
else()
    set(UA_GENERATED_NAMESPACE_ZERO ON)
endif()

if(MSVC AND UA_NAMESPACE_ZERO STREQUAL "FULL")
    # For the full NS0 we need a stack size of 8MB (as it is default on linux)
    # See https://github.com/open62541/open62541/issues/1326
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:8000000")
endif()

# It should not be possible to enable events without enabling subscriptions and full ns0
if((UA_ENABLE_SUBSCRIPTIONS_EVENTS) AND (NOT (UA_ENABLE_SUBSCRIPTIONS AND UA_NAMESPACE_ZERO STREQUAL "FULL")))
    message(FATAL_ERROR "Unable to enable events without UA_ENABLE_SUBSCRIPTIONS and full namespace 0")
endif()

if(UA_ENABLE_COVERAGE)
  set(CMAKE_BUILD_TYPE DEBUG)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage -lgcov")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
endif()

if(UA_BUILD_FUZZING OR UA_BUILD_OSS_FUZZ OR UA_BUILD_FUZZING_CORPUS)
    # Force enable options not passed in the build script, to also fuzzy-test this code
    set(UA_ENABLE_DISCOVERY ON CACHE STRING "" FORCE)
    set(UA_ENABLE_DISCOVERY_MULTICAST ON CACHE STRING "" FORCE)
    set(UA_ENABLE_ENCRYPTION ON CACHE STRING "" FORCE)
endif()

if(UA_ENABLE_DISCOVERY_MULTICAST AND NOT UA_ENABLE_DISCOVERY)
    MESSAGE(WARNING "UA_ENABLE_DISCOVERY_MULTICAST is enabled, but not UA_ENABLE_DISCOVERY. UA_ENABLE_DISCOVERY_MULTICAST will be set to OFF")
    SET(UA_ENABLE_DISCOVERY_MULTICAST OFF CACHE BOOL "Enable Discovery Service with multicast support (LDS-ME)" FORCE)
endif()

# Advanced options
option(UA_ENABLE_MULTITHREADING "Enable multithreading (experimental)" OFF)
mark_as_advanced(UA_ENABLE_MULTITHREADING)

option(UA_ENABLE_IMMUTABLE_NODES "Nodes in the information model are not edited but copied and replaced" OFF)
mark_as_advanced(UA_ENABLE_IMMUTABLE_NODES)
if(UA_ENABLE_MULTITHREADING)
    set(UA_ENABLE_IMMUTABLE_NODES ON)
endif()

option(UA_ENABLE_PUBSUB "Enable publish/subscribe (experimental)" OFF)
mark_as_advanced(UA_ENABLE_PUBSUB)
option(UA_ENABLE_PUBSUB_DELTAFRAMES "Enable sending of delta frames with only the changes" ON)
mark_as_advanced(UA_ENABLE_PUBSUB_DELTAFRAMES)
option(UA_ENABLE_PUBSUB_INFORMATIONMODEL "Enable PubSub information model twin" OFF)
mark_as_advanced(UA_ENABLE_PUBSUB_INFORMATIONMODEL)
if(UA_ENABLE_PUBSUB_INFORMATIONMODEL)
    if(NOT UA_ENABLE_PUBSUB)
    message(FATAL_ERROR "PubSub information model representation cannot be used with disabled PubSub function.")
    endif()
endif()

option(UA_ENABLE_STATUSCODE_DESCRIPTIONS "Enable conversion of StatusCode to human-readable error message" ON)
mark_as_advanced(UA_ENABLE_STATUSCODE_DESCRIPTIONS)

option(UA_ENABLE_TYPENAMES "Add the type and member names to the UA_DataType structure" ON)
mark_as_advanced(UA_ENABLE_TYPENAMES)

option(UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS "Set node description attribute for nodeset compiler generated nodes" ON)
mark_as_advanced(UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS)

option(UA_ENABLE_DETERMINISTIC_RNG "Do not seed the random number generator (e.g. for unit tests)." OFF)
mark_as_advanced(UA_ENABLE_DETERMINISTIC_RNG)

option(UA_ENABLE_VALGRIND_INTERACTIVE "Enable dumping valgrind every iteration. CAUTION! SLOWDOWN!" OFF)
mark_as_advanced(UA_ENABLE_VALGRIND_INTERACTIVE)

option(UA_MSVC_FORCE_STATIC_CRT "Force linking with the static C-runtime library when compiling to static library with MSVC" ON)
mark_as_advanced(UA_MSVC_FORCE_STATIC_CRT)

option(UA_FILE_NS0 "Override the NodeSet xml file used to generate namespace zero")
mark_as_advanced(UA_FILE_NS0)

# Semaphores/file system may not be available on embedded devices. It can be
# disabled with the following option
option(UA_ENABLE_DISCOVERY_SEMAPHORE "Enable Discovery Semaphore support" ON)
mark_as_advanced(UA_ENABLE_DISCOVERY_SEMAPHORE)

option(UA_ENABLE_UNIT_TESTS_MEMCHECK "Use Valgrind (Linux) or DrMemory (Windows) to detect memory leaks when running the unit tests" OFF)
mark_as_advanced(UA_ENABLE_UNIT_TESTS_MEMCHECK)

option(UA_ENABLE_NONSTANDARD_UDP "Enable udp extension (non-standard)" OFF)
mark_as_advanced(UA_ENABLE_NONSTANDARD_UDP)

option(UA_ENABLE_UNIT_TEST_FAILURE_HOOKS
       "Add hooks to force failure modes for additional unit tests. Not for production use!" OFF)
mark_as_advanced(UA_ENABLE_UNIT_TEST_FAILURE_HOOKS)

set(UA_VALGRIND_INTERACTIVE_INTERVAL 1000 CACHE STRING "The number of iterations to wait before creating the next dump")
mark_as_advanced(UA_VALGRIND_INTERACTIVE_INTERVAL)

# Build options for debugging
option(UA_DEBUG "Enable assertions and additional functionality that should not be included in release builds" OFF)
mark_as_advanced(UA_DEBUG)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(UA_DEBUG ON)
endif()

option(UA_DEBUG_DUMP_PKGS "Dump every package received by the server as hexdump format" OFF)
mark_as_advanced(UA_DEBUG_DUMP_PKGS)

# Build Targets
option(UA_BUILD_EXAMPLES "Build example servers and clients" OFF)
option(UA_BUILD_UNIT_TESTS "Build the unit tests" OFF)
option(UA_BUILD_FUZZING "Build the fuzzing executables" OFF)
mark_as_advanced(UA_BUILD_FUZZING)
if(UA_BUILD_FUZZING)
    # oss-fuzz already defines this by default
    add_definitions(-DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
endif()

option(UA_BUILD_FUZZING_CORPUS "Build the fuzzing corpus" OFF)
mark_as_advanced(UA_BUILD_FUZZING_CORPUS)
if(UA_BUILD_FUZZING_CORPUS)
    add_definitions(-DUA_DEBUG_DUMP_PKGS_FILE)
    set(UA_ENABLE_TYPENAMES ON CACHE STRING "" FORCE)
    set(UA_DEBUG_DUMP_PKGS ON CACHE STRING "" FORCE)
endif()

option(UA_BUILD_OSS_FUZZ "Special build switch used in oss-fuzz" OFF)
mark_as_advanced(UA_BUILD_OSS_FUZZ)

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

######################
# External Libraries #
######################

list(APPEND open62541_LIBRARIES "")

# Force compilation with as C++
option(UA_COMPILE_AS_CXX "Force compilation with a C++ compiler" OFF)
mark_as_advanced(UA_COMPILE_AS_CXX)
if (UA_COMPILE_AS_CXX)
    # We need the UINT32_C define
    add_definitions(-D__STDC_CONSTANT_MACROS)
endif()

if(UA_ENABLE_ENCRYPTION)
    # The recommended way is to install mbedtls via the OS package manager. If
    # that is not possible, manually compile mbedTLS and set the cmake variables
    # defined in /tools/cmake/FindMbedTLS.cmake.
    find_package(MbedTLS REQUIRED)
    list(APPEND open62541_LIBRARIES ${MBEDTLS_LIBRARIES})
endif()

#####################
# Compiler Settings #
#####################

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
        if(NOT OS9)
            set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -s")
            set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -s")
        endif()
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
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3 /WX /w44996") # Compiler warnings, error on warning

  if(UA_MSVC_FORCE_STATIC_CRT AND NOT BUILD_SHARED_LIBS)
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

if(UA_ENABLE_DISCOVERY)
    include_directories(${PROJECT_SOURCE_DIR}/src/client)
endif()

include_directories(${PROJECT_SOURCE_DIR}/include
                    ${PROJECT_SOURCE_DIR}/plugins
                    ${PROJECT_SOURCE_DIR}/deps
                    ${PROJECT_SOURCE_DIR}/src/pubsub
                    ${PROJECT_BINARY_DIR}
                    ${PROJECT_BINARY_DIR}/src_generated
                    ${MBEDTLS_INCLUDE_DIRS})

if(NOT "${UA_AMALGAMATION_ARCHITECUTRES}" STREQUAL "")
    set(exported_headers)
    foreach(arch ${UA_AMALGAMATION_ARCHITECUTRES})
        list(APPEND exported_headers ${PROJECT_SOURCE_DIR}/arch/${arch}/ua_architecture.h)
    endforeach()
else()
    set(exported_headers ${PROJECT_SOURCE_DIR}/arch/${UA_ARCHITECTURE}/ua_architecture.h)
endif()

set(exported_headers ${exported_headers}
                     ${ua_architecture_headers_beginning}
                     ${PROJECT_BINARY_DIR}/src_generated/ua_config.h
                     ${PROJECT_SOURCE_DIR}/deps/ms_stdint.h
                     ${PROJECT_BINARY_DIR}/src_generated/ua_statuscodes.h
                     ${PROJECT_BINARY_DIR}/src_generated/ua_nodeids.h
                     ${PROJECT_SOURCE_DIR}/include/ua_constants.h
                     ${PROJECT_SOURCE_DIR}/include/ua_types.h
                     ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated.h
                     ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated_handling.h
                     ${PROJECT_SOURCE_DIR}/include/ua_util.h
                     ${PROJECT_SOURCE_DIR}/include/ua_server.h
                     ${PROJECT_SOURCE_DIR}/include/ua_plugin_log.h
                     ${PROJECT_SOURCE_DIR}/include/ua_plugin_network.h
                     ${PROJECT_SOURCE_DIR}/include/ua_plugin_access_control.h
                     ${PROJECT_SOURCE_DIR}/include/ua_plugin_pki.h
                     ${PROJECT_SOURCE_DIR}/include/ua_plugin_securitypolicy.h
                     ${PROJECT_SOURCE_DIR}/include/ua_server_pubsub.h
                     ${PROJECT_SOURCE_DIR}/include/ua_plugin_pubsub.h
                     ${PROJECT_SOURCE_DIR}/include/ua_plugin_nodestore.h
                     ${PROJECT_SOURCE_DIR}/include/ua_server_config.h
                     ${PROJECT_SOURCE_DIR}/include/ua_client_config.h
                     ${PROJECT_SOURCE_DIR}/include/ua_client.h
                     ${PROJECT_SOURCE_DIR}/include/ua_client_highlevel.h
                     ${PROJECT_SOURCE_DIR}/include/ua_client_subscriptions.h
                     ${PROJECT_SOURCE_DIR}/include/ua_client_highlevel_async.h)

set(internal_headers ${PROJECT_SOURCE_DIR}/deps/queue.h
                     ${PROJECT_SOURCE_DIR}/deps/pcg_basic.h
                     ${PROJECT_SOURCE_DIR}/deps/libc_time.h
                     ${PROJECT_SOURCE_DIR}/deps/base64.h
                     ${PROJECT_SOURCE_DIR}/src/ua_util_internal.h
                     ${PROJECT_SOURCE_DIR}/src/ua_types_encoding_binary.h
                     ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated_encoding_binary.h
                     ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated.h
                     ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated_handling.h
                     ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated_encoding_binary.h
                     ${PROJECT_SOURCE_DIR}/src/ua_connection_internal.h
                     ${PROJECT_SOURCE_DIR}/src/ua_securechannel.h
                     ${PROJECT_SOURCE_DIR}/src/ua_timer.h
                     ${PROJECT_SOURCE_DIR}/src/server/ua_session.h
                     ${PROJECT_SOURCE_DIR}/src/server/ua_subscription.h
                     ${PROJECT_SOURCE_DIR}/src/server/ua_session_manager.h
                     ${PROJECT_SOURCE_DIR}/src/server/ua_securechannel_manager.h
                     ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_networkmessage.h
                     ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub.h
                     ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_manager.h
                     ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_ns0.h
                     ${PROJECT_SOURCE_DIR}/src/server/ua_server_internal.h
                     ${PROJECT_SOURCE_DIR}/src/server/ua_services.h
                     ${PROJECT_BINARY_DIR}/src_generated/ua_namespace0.h
                     ${PROJECT_SOURCE_DIR}/src/client/ua_client_internal.h)

# TODO: make client optional
set(lib_sources ${PROJECT_SOURCE_DIR}/src/ua_types.c
                ${PROJECT_SOURCE_DIR}/src/ua_types_encoding_binary.c
                ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated.c
                ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated.c
                ${PROJECT_BINARY_DIR}/src_generated/ua_statuscodes.c
                ${PROJECT_SOURCE_DIR}/src/ua_util.c
                ${PROJECT_SOURCE_DIR}/src/ua_timer.c
                ${PROJECT_SOURCE_DIR}/src/ua_connection.c
                ${PROJECT_SOURCE_DIR}/src/ua_securechannel.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_session.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_nodes.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_server.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_server_ns0.c
                ${PROJECT_BINARY_DIR}/src_generated/ua_namespace0.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_server_binary.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_server_utils.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_server_worker.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_server_discovery.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_securechannel_manager.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_session_manager.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_subscription.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_subscription_datachange.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_subscription_events.c
                ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_networkmessage.c
                ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub.c
                ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_manager.c
                ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_ns0.c
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
                ${PROJECT_SOURCE_DIR}/src/client/ua_client_connect_async.c
                ${PROJECT_SOURCE_DIR}/src/client/ua_client_discovery.c
                ${PROJECT_SOURCE_DIR}/src/client/ua_client_highlevel.c
                ${PROJECT_SOURCE_DIR}/src/client/ua_client_subscriptions.c
                ${PROJECT_SOURCE_DIR}/src/client/ua_client_worker.c

                # dependencies
                ${PROJECT_SOURCE_DIR}/deps/libc_time.c
                ${PROJECT_SOURCE_DIR}/deps/pcg_basic.c
                ${PROJECT_SOURCE_DIR}/deps/base64.c)

set(default_plugin_headers ${PROJECT_SOURCE_DIR}/plugins/ua_accesscontrol_default.h
                           ${PROJECT_SOURCE_DIR}/plugins/ua_pki_certificate.h
                           ${PROJECT_SOURCE_DIR}/plugins/ua_log_stdout.h
                           ${PROJECT_SOURCE_DIR}/plugins/ua_nodestore_default.h
                           ${PROJECT_SOURCE_DIR}/plugins/ua_config_default.h
                           ${PROJECT_SOURCE_DIR}/plugins/ua_securitypolicy_none.h
)

set(default_plugin_sources ${PROJECT_SOURCE_DIR}/plugins/ua_log_stdout.c
                           ${PROJECT_SOURCE_DIR}/plugins/ua_accesscontrol_default.c
                           ${PROJECT_SOURCE_DIR}/plugins/ua_pki_certificate.c
                           ${PROJECT_SOURCE_DIR}/plugins/ua_nodestore_default.c
                           ${PROJECT_SOURCE_DIR}/plugins/ua_config_default.c
                           ${PROJECT_SOURCE_DIR}/plugins/ua_securitypolicy_none.c
)

if(UA_ENABLE_ENCRYPTION)
    list(APPEND default_plugin_headers ${PROJECT_SOURCE_DIR}/plugins/ua_securitypolicy_basic128rsa15.h
                                       ${PROJECT_SOURCE_DIR}/plugins/ua_securitypolicy_basic256sha256.h)
    list(APPEND default_plugin_sources ${PROJECT_SOURCE_DIR}/plugins/ua_securitypolicy_basic128rsa15.c
                                       ${PROJECT_SOURCE_DIR}/plugins/ua_securitypolicy_basic256sha256.c)
endif()

if(UA_ENABLE_PUBSUB)
    list(APPEND default_plugin_headers ${PROJECT_SOURCE_DIR}/plugins/ua_network_pubsub_udp.h)
    list(APPEND default_plugin_sources ${PROJECT_SOURCE_DIR}/plugins/ua_network_pubsub_udp.c)
endif()


if(UA_DEBUG_DUMP_PKGS)
    list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/plugins/ua_debug_dump_pkgs.c)
endif()

if(UA_ENABLE_NONSTANDARD_UDP)
    list(APPEND exported_headers ${PROJECT_SOURCE_DIR}/arch/ua_network_udp.h)
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

if(UA_NAMESPACE_ZERO STREQUAL "FULL")
    if(NOT UA_FILE_NS0)
        set(UA_FILE_NS0 ${PROJECT_SOURCE_DIR}/deps/ua-nodeset/Schema/Opc.Ua.NodeSet2.xml)
    endif()

    set(UA_FILE_NODEIDS ${PROJECT_SOURCE_DIR}/deps/ua-nodeset/Schema/NodeIds.csv)
    set(UA_FILE_STATUSCODES ${PROJECT_SOURCE_DIR}/deps/ua-nodeset/Schema/StatusCode.csv)
    set(UA_FILE_TYPES_BSD ${PROJECT_SOURCE_DIR}/deps/ua-nodeset/Schema/Opc.Ua.Types.bsd)
else()
    if(NOT UA_FILE_NS0)
        set(UA_FILE_NS0 ${PROJECT_SOURCE_DIR}/tools/schema/Opc.Ua.NodeSet2.Minimal.xml)
    endif()
    set(UA_FILE_DATATYPES ${PROJECT_SOURCE_DIR}/tools/schema/datatypes_minimal.txt)
    set(UA_FILE_NODEIDS ${PROJECT_SOURCE_DIR}/tools/schema/NodeIds.csv)
    set(UA_FILE_STATUSCODES ${PROJECT_SOURCE_DIR}/tools/schema/StatusCode.csv)
    set(UA_FILE_TYPES_BSD ${PROJECT_SOURCE_DIR}/tools/schema/Opc.Ua.Types.bsd)

    if(UA_ENABLE_METHODCALLS)
        list(APPEND UA_FILE_DATATYPES ${PROJECT_SOURCE_DIR}/tools/schema/datatypes_method.txt)
    endif()

    if(UA_ENABLE_SUBSCRIPTIONS)
        list(APPEND UA_FILE_DATATYPES ${PROJECT_SOURCE_DIR}/tools/schema/datatypes_subscriptions.txt)
    endif()

    if(UA_ENABLE_DISCOVERY)
        list(APPEND UA_FILE_DATATYPES ${PROJECT_SOURCE_DIR}/tools/schema/datatypes_discovery.txt)
    endif()

    if(UA_ENABLE_QUERY)
        list(APPEND UA_FILE_DATATYPES ${PROJECT_SOURCE_DIR}/tools/schema/datatypes_query.txt)
    endif()

    if(UA_ENABLE_PUBSUB)
        list(APPEND UA_FILE_DATATYPES ${PROJECT_SOURCE_DIR}/tools/schema/datatypes_pubsub.txt)
        if(UA_ENABLE_PUBSUB_INFORMATIONMODEL)
            set(UA_FILE_NSPUBSUB --xml ${PROJECT_SOURCE_DIR}/tools/schema/Opc.Ua.NodeSet2.PubSubMinimal.xml)
        endif()
    endif()
endif()

if(NOT EXISTS "${UA_FILE_NS0}")
    message(FATAL_ERROR "File ${UA_FILE_NS0} not found. You probably need to initialize the git submodule for deps/ua-nodeset.")
endif()

set(SELECTED_TYPES_TMP "")
foreach(f ${UA_FILE_DATATYPES})
    set(SELECTED_TYPES_TMP ${SELECTED_TYPES_TMP} "--selected-types=${f}")
endforeach()

# standard-defined data types
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated.c
                          ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated.h
                          ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated_handling.h
                          ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated_encoding_binary.h
                   PRE_BUILD
                   COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/generate_datatypes.py
                           --type-csv=${UA_FILE_NODEIDS}
                           ${SELECTED_TYPES_TMP}
                           --type-bsd=${UA_FILE_TYPES_BSD}
                           ${PROJECT_BINARY_DIR}/src_generated/ua_types
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_datatypes.py
                           ${UA_FILE_NODEIDS}
                           ${UA_FILE_TYPES_BSD}
                           ${UA_FILE_DATATYPES})
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
                           --type-bsd=${UA_FILE_TYPES_BSD}
                           --type-bsd=${PROJECT_SOURCE_DIR}/tools/schema/Custom.Opc.Ua.Transport.bsd
                           --no-builtin
                           ${PROJECT_BINARY_DIR}/src_generated/ua_transport
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_datatypes.py
                           ${PROJECT_SOURCE_DIR}/tools/schema/datatypes_transport.txt
                           ${UA_FILE_TYPES_BSD}
                           ${PROJECT_SOURCE_DIR}/tools/schema/Custom.Opc.Ua.Transport.bsd)

# we need a custom target to avoid that the generator is called concurrently and thus overwriting files while the other thread is compiling
add_custom_target(open62541-generator-transport DEPENDS
        ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated.c
        ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated.h
        ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated_handling.h
        ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated_encoding_binary.h)

# statuscode explanation
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/ua_statuscodes.h
                          ${PROJECT_BINARY_DIR}/src_generated/ua_statuscodes.c
        PRE_BUILD
        COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/generate_statuscode_descriptions.py
        ${UA_FILE_STATUSCODES} ${PROJECT_BINARY_DIR}/src_generated/ua_statuscodes
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_statuscode_descriptions.py
                ${UA_FILE_STATUSCODES})

# Header containing defines for all NodeIds
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/ua_nodeids.h
        PRE_BUILD
        COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/generate_nodeid_header.py
        ${UA_FILE_NODEIDS}  ${PROJECT_BINARY_DIR}/src_generated/ua_nodeids NS0
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_nodeid_header.py
        ${UA_FILE_NODEIDS})

# we need a custom target to avoid that the generator is called concurrently and
# thus overwriting files while the other thread is compiling
add_custom_target(open62541-generator-statuscode DEPENDS
        ${PROJECT_BINARY_DIR}/src_generated/ua_nodeids.h
        ${PROJECT_BINARY_DIR}/src_generated/ua_statuscodes.h
        ${PROJECT_BINARY_DIR}/src_generated/ua_statuscodes.c)

# single-file release
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/open62541.h
                   PRE_BUILD
                   COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/amalgamate.py
                           ${OPEN62541_VER_COMMIT} ${CMAKE_CURRENT_BINARY_DIR}/open62541.h
                           ${exported_headers} ${default_plugin_headers} ${ua_architecture_headers}
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/amalgamate.py
                           ${exported_headers} ${default_plugin_headers} ${ua_architecture_headers})

add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/open62541.c
                   PRE_BUILD
                   COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/amalgamate.py
                           ${OPEN62541_VER_COMMIT} ${CMAKE_CURRENT_BINARY_DIR}/open62541.c
                           ${internal_headers} ${lib_sources} ${default_plugin_sources} ${ua_architecture_sources}
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/amalgamate.py ${internal_headers}
                           ${lib_sources} ${default_plugin_sources} ${ua_architecture_sources} )

add_custom_target(open62541-amalgamation-source DEPENDS ${PROJECT_BINARY_DIR}/open62541.c)
add_custom_target(open62541-amalgamation-header DEPENDS ${PROJECT_BINARY_DIR}/open62541.h)

add_dependencies(open62541-amalgamation-header open62541-generator-types)
add_dependencies(open62541-amalgamation-source open62541-generator-types
                 open62541-generator-transport open62541-generator-statuscode)

if(NOT UA_NODESET_ENCODE_BINARY_SIZE)
    set(UA_NODESET_ENCODE_BINARY_SIZE 32000)
endif()

# generated namespace 0
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/ua_namespace0.c
                          ${PROJECT_BINARY_DIR}/src_generated/ua_namespace0.h
                   PRE_BUILD
                   COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodeset_compiler.py
                           --generate-ns0
                           --internal-headers
                           --encode-binary-size=${UA_NODESET_ENCODE_BINARY_SIZE}
                           --ignore ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/NodeID_NS0_Base.txt
                           --xml ${UA_FILE_NS0}
                           ${UA_FILE_NSPUBSUB}
                           ${PROJECT_BINARY_DIR}/src_generated/ua_namespace0
                   DEPENDS ${UA_FILE_NS0}
                           ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodeset_compiler.py
                           ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodes.py
                           ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodeset.py
                           ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/datatypes.py
                           ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/backend_open62541.py
                           ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/backend_open62541_nodes.py
                           ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/backend_open62541_datatypes.py)

# stack protector needs to be disabled for the huge ns0 file, otherwise it may take many minutes to compile the file.
if(NOT MSVC)
    set_source_files_properties(${PROJECT_BINARY_DIR}/src_generated/ua_namespace0.c PROPERTIES COMPILE_FLAGS -fno-stack-protector)
endif()

# we need a custom target to avoid that the generator is called concurrently and thus overwriting files while the other thread is compiling
add_custom_target(open62541-generator-namespace DEPENDS
        ${PROJECT_BINARY_DIR}/src_generated/ua_namespace0.c
        ${PROJECT_BINARY_DIR}/src_generated/ua_namespace0.h)

#####################
# Build the Library #
#####################

assign_source_group(${lib_sources})
assign_source_group(${internal_headers})
assign_source_group(${exported_headers})
assign_source_group(${default_plugin_sources})
assign_source_group(${ua_architecture_sources})

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
                     open62541-amalgamation-source)

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

    add_library(open62541-plugins OBJECT ${default_plugin_sources} ${ua_architecture_sources} ${exported_headers})
    add_dependencies(open62541-plugins open62541-generator-types open62541-generator-transport)
    target_include_directories(open62541-plugins PRIVATE ${PROJECT_SOURCE_DIR}/plugins)
    target_include_directories(open62541-plugins PRIVATE ${PROJECT_BINARY_DIR}/src_generated)
    target_compile_definitions(open62541-plugins PRIVATE -DUA_DYNAMIC_LINKING_EXPORT)
    set_target_properties(open62541-plugins PROPERTIES FOLDER "open62541/lib")

    add_library(open62541 $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-plugins>)

    if(UA_COMPILE_AS_CXX)
        set_source_files_properties(${lib_sources} PROPERTIES LANGUAGE CXX)
        set_source_files_properties(${default_plugin_sources} ${ua_architecture_sources} PROPERTIES LANGUAGE CXX)
    endif()
endif()

add_dependencies(open62541-amalgamation-source open62541-generator-namespace)
add_dependencies(open62541-amalgamation-header open62541-generator-namespace)
if(NOT UA_ENABLE_AMALGAMATION)
    add_dependencies(open62541-object open62541-generator-namespace)
endif()

# Export Symbols
target_compile_definitions(open62541-object PRIVATE -DUA_DYNAMIC_LINKING_EXPORT)
target_compile_definitions(open62541 PRIVATE -DUA_DYNAMIC_LINKING_EXPORT)
if (UA_ENABLE_DISCOVERY_MULTICAST)
    target_compile_definitions(open62541-object PRIVATE -DMDNSD_DYNAMIC_LINKING_EXPORT)
    target_compile_definitions(open62541 PRIVATE -DMDNSD_DYNAMIC_LINKING_EXPORT)
endif()
# Generate properly versioned shared library links on Linux
SET_TARGET_PROPERTIES(open62541 PROPERTIES SOVERSION 0 VERSION "${OPEN62541_VER_MAJOR}.${OPEN62541_VER_MINOR}.${OPEN62541_VER_PATCH}")

##################################
#     Architectures changes      #
##################################

GET_PROPERTY(ua_architecture_add_definitions GLOBAL PROPERTY UA_ARCHITECTURE_ADD_DEFINITIONS)
add_definitions(${ua_architecture_add_definitions})

GET_PROPERTY(ua_architecture_remove_definitions GLOBAL PROPERTY UA_ARCHITECTURE_REMOVE_DEFINITIONS)
if (NOT "${ua_architecture_remove_definitions}" STREQUAL "")
  string(REPLACE " " ";" ua_architecture_remove_definitions_list ${ua_architecture_remove_definitions})
  remove_definitions(${ua_architecture_remove_definitions_list})
endif(NOT "${ua_architecture_remove_definitions}" STREQUAL "")

GET_PROPERTY(ua_architecture_append_to_library GLOBAL PROPERTY UA_ARCHITECTURE_APPEND_TO_LIBRARY)
list(APPEND open62541_LIBRARIES ${ua_architecture_append_to_library})

# DLL requires linking to dependencies
target_link_libraries(open62541 ${open62541_LIBRARIES})

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

if(UA_BUILD_FUZZING OR UA_BUILD_OSS_FUZZ OR UA_BUILD_FUZZING_CORPUS)
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

add_custom_target(cpplint cpplint
                  ${lib_sources}
                  ${internal_headers}
                  ${default_plugin_headers}
                  ${default_plugin_sources}
                  ${ua_architecture_headers}
                  ${ua_architecture_sources}
                  DEPENDS ${lib_sources}
                          ${internal_headers}
                          ${default_plugin_headers}
                          ${default_plugin_sources}
                          ${ua_architecture_headers}
                          ${ua_architecture_sources}

                  COMMENT "Run cpplint code style checker on the library")

##########################
# Installation           #
##########################
# invoke via `make install`
# specify install location with `-DCMAKE_INSTALL_PREFIX=xyz`
# Enable shared library with `-DBUILD_SHARED_LIBS=ON`

set(cmake_configfile_install ${LIB_INSTALL_DIR}/cmake)
set(target_install_dest_name "${cmake_configfile_install}/open62541Targets.cmake")
set(open62541_tools_dir share/open62541/tools)
set(open62541_deps_dir include/open62541/deps)

# export library (either static or shared depending on BUILD_SHARED_LIBS)
install(TARGETS open62541
        EXPORT open62541Targets
        LIBRARY DESTINATION ${LIB_INSTALL_DIR}
        ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
        INCLUDES DESTINATION include/open62541 ${open62541_deps_dir})

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

set_target_properties(open62541-generator-namespace PROPERTIES FOLDER "open62541/generators")
set_target_properties(open62541-generator-statuscode PROPERTIES FOLDER "open62541/generators")
set_target_properties(open62541-generator-transport PROPERTIES FOLDER "open62541/generators")
set_target_properties(open62541-generator-types PROPERTIES FOLDER "open62541/generators")
