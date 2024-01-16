cmake_minimum_required(VERSION 3.0...3.12)
if(UA_BUILD_FUZZING OR UA_BUILD_OSS_FUZZ OR UA_BUILD_FUZZING_CORPUS)
    project(open62541) # We need to have C++ support configured for fuzzing
else()
    project(open62541 C) # Do not look for a C++ compiler
endif()
# set(CMAKE_VERBOSE_MAKEFILE ON)
if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

string(TOLOWER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_LOWER_CASE)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/tools/cmake")
find_package(Python3 REQUIRED)
set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
find_package(Git)
include(AssignSourceGroup)
include(GNUInstallDirs)

# Set when installed via make install
set(open62541_TOOLS_DIR ${PROJECT_SOURCE_DIR}/tools)
set(open62541_NODESET_DIR ${PROJECT_SOURCE_DIR}/deps/ua-nodeset)
set(open62541_MQTT_DIR ${PROJECT_SOURCE_DIR}/deps/mqtt-c)

include(macros_internal)
include(macros_public)

#############################
# Compiled binaries folders #
#############################

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

###########
# Version #
###########

# The current version information. On the master branch, we take the version
# number from the latest release plus the "-undefined" label. Will be
# overwritten with more detailed information if git is available.
set(OPEN62541_VER_MAJOR 1)
set(OPEN62541_VER_MINOR 3)
set(OPEN62541_VER_PATCH 9)
set(OPEN62541_VER_LABEL "-undefined") # like "-rc1" or "-g4538abcd" or "-g4538abcd-dirty"
set(OPEN62541_VER_COMMIT "unknown-commit")

# Overwrite the version information based on git if available
include(SetGitBasedVersion)
set_open62541_version()

# Examples for the version string are:
# v1.2
# v1.2.3
# v1.2.3-rc1
# v1.2.3-rc1-dirty
# v1.2.3-5-g4538abcd
# v1.2.3-5-g4538abcd-dirty
set(OPEN62541_VERSION "v${OPEN62541_VER_MAJOR}.${OPEN62541_VER_MINOR}.${OPEN62541_VER_PATCH}${OPEN62541_VER_LABEL}")
MESSAGE(STATUS "open62541 Version: ${OPEN62541_VERSION}")

#################
# Build Options #
#################

# Set default build type.
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "CMAKE_BUILD_TYPE not given; setting to 'Debug'")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build" FORCE)
endif()

option(UA_ENABLE_AMALGAMATION "Concatenate the library to a single file open62541.h/.c" OFF)
set(UA_AMALGAMATION_ARCHITECTURES "" CACHE STRING "List of architectures to include in amalgamation")
mark_as_advanced(UA_AMALGAMATION_ARCHITECTURES)

# Platform. This is at the beginning in case the architecture changes some UA options
set(UA_ARCHITECTURE "None" CACHE STRING "Architecture to build open62541 on")

if(UA_ENABLE_AMALGAMATION)
    if("${UA_AMALGAMATION_ARCHITECTURES}" STREQUAL "")
        if(NOT "${UA_ARCHITECTURE}" STREQUAL "None")
            set(UA_AMALGAMATION_ARCHITECTURES "${UA_ARCHITECTURE}")
        else()
            # select some default architectures which should be included
            set(UA_AMALGAMATION_ARCHITECTURES "win32;posix")
        endif()
    endif()
    message(STATUS "Architectures included in amalgamation: ${UA_AMALGAMATION_ARCHITECTURES}")
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

GET_PROPERTY(ua_architecture_directories_to_include GLOBAL PROPERTY UA_INCLUDE_DIRECTORIES)

GET_PROPERTY(ua_architecture_headers GLOBAL PROPERTY UA_ARCHITECTURE_HEADERS)

GET_PROPERTY(ua_architecture_headers_beginning GLOBAL PROPERTY UA_ARCHITECTURE_HEADERS_BEGINNING)

GET_PROPERTY(ua_architecture_sources GLOBAL PROPERTY UA_ARCHITECTURE_SOURCES)

set(ua_architecture_sources ${ua_architecture_sources}
    ${PROJECT_SOURCE_DIR}/arch/network_tcp.c
)

set(ua_architecture_headers ${ua_architecture_headers}
    ${PROJECT_SOURCE_DIR}/include/open62541/network_tcp.h
    ${PROJECT_SOURCE_DIR}/include/open62541/architecture_functions.h
)

if(UA_ENABLE_WEBSOCKET_SERVER)
    set(ua_architecture_sources ${ua_architecture_sources}
                                ${PROJECT_SOURCE_DIR}/arch/network_ws.c)

    set(ua_architecture_headers ${ua_architecture_headers}
                                ${PROJECT_SOURCE_DIR}/include/open62541/network_ws.h)
endif()

if(${UA_ARCHITECTURE} STREQUAL "None")
  message(FATAL_ERROR "No architecture was selected. Please select the architecture of your target platform")
endif(${UA_ARCHITECTURE} STREQUAL "None")

# Create a list of ifdefs for all the architectures.
# This is needed to enable a default architecture if none is selected through gcc compiler def.
# Especially if someone is using the amalgamated file on Linux/Windows he should not need to define an architecture.
set(UA_ARCHITECTURES_NODEF "1 ") #to make it easier to append later the && symbol
foreach(arch_ ${architectures})
    string(TOUPPER ${arch_} UA_ARCHITECTURE_UPPER_)
    set(UA_ARCHITECTURES_NODEF "${UA_ARCHITECTURES_NODEF} && !defined(UA_ARCHITECTURE_${UA_ARCHITECTURE_UPPER_})")
endforeach(arch_ ${architectures})

# Options
option(BUILD_SHARED_LIBS "Enable building of shared libraries (dll/so)" OFF)
set(UA_LOGLEVEL 300 CACHE STRING "Level at which logs shall be reported")
set(UA_MULTITHREADING 100 CACHE STRING "Level of multithreading (0-99: No Multithreading, >=100: Thread-Safe API")
option(UA_ENABLE_DIAGNOSTICS "Enable diagnostics information exposed by the server" OFF)
option(UA_ENABLE_METHODCALLS "Enable the Method service set" ON)
option(UA_ENABLE_SUBSCRIPTIONS "Enable subscriptions support" ON)
option(UA_ENABLE_SUBSCRIPTIONS_EVENTS "Enable event monitoring" ON)
option(UA_ENABLE_DA "Enable OPC UA DataAccess (Part 8) definitions" ON)
option(UA_ENABLE_HISTORIZING "Enable basic support for historical access (client and server)" OFF)
option(UA_ENABLE_DISCOVERY "Enable Discovery Service (LDS)" OFF)
option(UA_ENABLE_JSON_ENCODING "Enable JSON encoding" OFF)

option(UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS "Enable the use of A&C. (EXPERIMENTAL)" OFF)
mark_as_advanced(UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS)

option(UA_ENABLE_NODEMANAGEMENT "Enable dynamic addition and removal of nodes at runtime" ON)
mark_as_advanced(UA_ENABLE_NODEMANAGEMENT)

option(UA_ENABLE_PARSING "Utility functions that require parsing (e.g. NodeId expressions)" ON)
mark_as_advanced(UA_ENABLE_PARSING)

option(UA_ENABLE_DISCOVERY_MULTICAST "Enable Discovery Service with multicast support (LDS-ME)" OFF)
mark_as_advanced(UA_ENABLE_DISCOVERY_MULTICAST)

option(UA_ENABLE_WEBSOCKET_SERVER "Enable websocket support (uses libwebsockets)" OFF)
mark_as_advanced(UA_ENABLE_WEBSOCKET_SERVER)

# security provider
set(UA_ENCRYPTION_PLUGINS "MBEDTLS" "OPENSSL" "LIBRESSL")
set(UA_ENABLE_ENCRYPTION "OFF" CACHE STRING "Encryption support (Libressl EXPERIMENTAL)")
SET_PROPERTY(CACHE UA_ENABLE_ENCRYPTION PROPERTY STRINGS "OFF" ${UA_ENCRYPTION_PLUGINS})
option(UA_ENABLE_ENCRYPTION_OPENSSL "Deprecated: Enable encryption support (uses openssl)" OFF)
mark_as_advanced(UA_ENABLE_ENCRYPTION_OPENSSL)
option(UA_ENABLE_ENCRYPTION_MBEDTLS "Deprecated: Enable encryption support (uses mbedTLS)" OFF)
mark_as_advanced(UA_ENABLE_ENCRYPTION_MBEDTLS)

list (FIND UA_ENCRYPTION_PLUGINS ${UA_ENABLE_ENCRYPTION} _tmp)
if(UA_ENABLE_ENCRYPTION STREQUAL "OFF" OR ${_tmp} GREATER -1)
    set(UA_ENABLE_ENCRYPTION_OPENSSL OFF)
    set(UA_ENABLE_ENCRYPTION_MBEDTLS OFF)
    set(UA_ENABLE_ENCRYPTION_LIBRESSL OFF)
    if(UA_ENABLE_ENCRYPTION STREQUAL "MBEDTLS")
        set(UA_ENABLE_ENCRYPTION_MBEDTLS ON)
    elseif(UA_ENABLE_ENCRYPTION STREQUAL "OPENSSL")
        set(UA_ENABLE_ENCRYPTION_OPENSSL ON)
    elseif(UA_ENABLE_ENCRYPTION STREQUAL "LIBRESSL")
        set(UA_ENABLE_ENCRYPTION_LIBRESSL ON)
    endif()
# Only for backward compatability
elseif(UA_ENABLE_ENCRYPTION)
    message(DEPRECATION "Set UA_ENABLE_ENCRYPTION to the desired encryption library." )
    if(NOT UA_ENABLE_ENCRYPTION_OPENSSL)
    set(UA_ENABLE_ENCRYPTION_MBEDTLS ON)
    endif()
else()
    message(DEPRECATION "Set UA_ENABLE_ENCRYPTION to the desired encryption library." )
    if(UA_ENABLE_ENCRYPTION_MBEDTLS)
        set(UA_ENABLE_ENCRYPTION "MBEDTLS")
        set(UA_ENABLE_ENCRYPTION_OPENSSL OFF)
    endif()
    if(UA_ENABLE_ENCRYPTION_OPENSSL)
        set(UA_ENABLE_ENCRYPTION "OPENSSL")
        set(UA_ENABLE_ENCRYPTION_MBEDTLS OFF)
    endif()
endif()

# TPM Security
set(UA_ENABLE_ENCRYPTION_TPM2 "OFF" CACHE STRING "TPM encryption support")
SET_PROPERTY(CACHE UA_ENABLE_ENCRYPTION_TPM2 PROPERTY STRINGS "ON" "OFF")

if(UA_ENABLE_ENCRYPTION_TPM2 STREQUAL "OFF")
    set(UA_ENABLE_TPM2_SECURITY OFF)
    set(UA_ENABLE_TPM2_KEYSTORE OFF)
else()
    set(UA_ENABLE_TPM2_SECURITY ON)
    set(UA_ENABLE_TPM2_KEYSTORE ON)
endif()

if(UA_ENABLE_TPM2_SECURITY)
    find_library(TPM2_LIB tpm2_pkcs11)
    message(${TPM2_LIB})
endif()

if(UA_ENABLE_TPM2_SECURITY)
    if(NOT UA_ENABLE_PUBSUB_ENCRYPTION)
        message(FATAL_ERROR "TPM2 encryption cannot be used with disabled UA_ENABLE_PUBSUB_ENCRYPTION")
    endif()
endif()

if(UA_ENABLE_TPM2_KEYSTORE)
    if(UA_ENABLE_PUBSUB)
        if(NOT UA_ENABLE_PUBSUB_ENCRYPTION)
            message(FATAL_ERROR "TPM2 Keystore for PubSub cannot be used with disabled UA_ENABLE_PUBSUB_ENCRYPTION")
        endif()
    else()
        if(NOT UA_ENABLE_ENCRYPTION)
            message(FATAL_ERROR "TPM2 Keystore cannot be used with disabled UA_ENABLE_ENCRYPTION")
        endif()
    endif()
endif()

# Namespace Zero
set(UA_NAMESPACE_ZERO "REDUCED" CACHE STRING "Completeness of the generated namespace zero (minimal/reduced/full/latest_1_05)")
SET_PROPERTY(CACHE UA_NAMESPACE_ZERO PROPERTY STRINGS "MINIMAL" "REDUCED" "FULL" "LATEST_1_05")
if(UA_NAMESPACE_ZERO STREQUAL "MINIMAL")
    set(UA_GENERATED_NAMESPACE_ZERO OFF)
else()
    set(UA_GENERATED_NAMESPACE_ZERO ON)
endif()

if(UA_NAMESPACE_ZERO STREQUAL "FULL")
    set(UA_GENERATED_NAMESPACE_ZERO_FULL ON)
else()
    set(UA_GENERATED_NAMESPACE_ZERO_FULL OFF)
endif()

if(UA_NAMESPACE_ZERO STREQUAL "LATEST_1_05")
    set(UA_GENERATED_NAMESPACE_ZERO_LATEST_1_05 ON)
else()
    set(UA_GENERATED_NAMESPACE_ZERO_LATEST_1_05 OFF)
endif()

if(MSVC AND UA_NAMESPACE_ZERO STREQUAL "FULL")
    # For the full NS0 we need a stack size of 8MB (as it is default on linux)
    # See https://github.com/open62541/open62541/issues/1326
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:8000000")
endif()

if(UA_BUILD_FUZZING OR UA_BUILD_OSS_FUZZ OR UA_BUILD_FUZZING_CORPUS)
    # Force enable options not passed in the build script, to also fuzzy-test this code
    set(UA_ENABLE_DISCOVERY ON CACHE STRING "" FORCE)
    set(UA_ENABLE_DISCOVERY_MULTICAST ON CACHE STRING "" FORCE)
    set(UA_ENABLE_ENCRYPTION ON CACHE STRING "OFF" FORCE)
    set(UA_ENABLE_ENCRYPTION_MBEDTLS ON CACHE STRING "" FORCE)
    set(UA_ENABLE_HISTORIZING ON CACHE STRING "" FORCE)
    set(UA_ENABLE_JSON_ENCODING ON CACHE STRING "" FORCE)
    set(UA_ENABLE_SUBSCRIPTIONS ON CACHE STRING "" FORCE)
    set(UA_ENABLE_SUBSCRIPTIONS_EVENTS ON CACHE STRING "" FORCE)
endif()

# It should not be possible to enable events without enabling subscriptions and full ns0
#if((UA_ENABLE_SUBSCRIPTIONS_EVENTS) AND (NOT (UA_ENABLE_SUBSCRIPTIONS AND UA_NAMESPACE_ZERO STREQUAL "FULL")))
#    message(FATAL_ERROR "Unable to enable events without UA_ENABLE_SUBSCRIPTIONS and full namespace 0")
#endif()

# It should not be possible to enable Alarms and Condition without enabling Events and full ns0
if((UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS) AND (NOT (UA_ENABLE_SUBSCRIPTIONS_EVENTS AND UA_NAMESPACE_ZERO STREQUAL "FULL")))
    message(FATAL_ERROR "Unable to enable A&C without UA_ENABLE_SUBSCRIPTIONS_EVENTS and full namespace 0")
endif()

if(UA_ENABLE_COVERAGE)
  set(CMAKE_BUILD_TYPE DEBUG)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage -lgcov")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
endif()

if(UA_ENABLE_CLANG_COV)
    if(NOT "x${CMAKE_C_COMPILER_ID}" STREQUAL "xClang")
        message(FATAL_ERROR "Compiler must be clang when compiling for llvm-cov")
    endif()
    if(UA_ENABLE_COVERAGE)
        message(FATAL_ERROR "Only either clang cov or normal coverage is allowed.")
    endif()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
endif()

if(UA_ENABLE_DISCOVERY_MULTICAST AND NOT UA_ENABLE_DISCOVERY)
    MESSAGE(WARNING "UA_ENABLE_DISCOVERY_MULTICAST is enabled, but not UA_ENABLE_DISCOVERY. UA_ENABLE_DISCOVERY_MULTICAST will be set to OFF")
    SET(UA_ENABLE_DISCOVERY_MULTICAST OFF CACHE BOOL "Enable Discovery Service with multicast support (LDS-ME)" FORCE)
endif()

# Advanced options
option(UA_ENABLE_COVERAGE "Enable gcov coverage" OFF)
mark_as_advanced(UA_ENABLE_COVERAGE)

option(UA_ENABLE_CLANG_COV "Enable clang coverage" OFF)
mark_as_advanced(UA_ENABLE_CLANG_COV)

option(UA_ENABLE_QUERY "Enable query support in the client (most servers don't support it)" OFF)
mark_as_advanced(UA_ENABLE_QUERY)

option(UA_ENABLE_IMMUTABLE_NODES "Nodes in the information model are not edited but copied and replaced" OFF)
mark_as_advanced(UA_ENABLE_IMMUTABLE_NODES)

option(UA_ENABLE_EXPERIMENTAL_HISTORIZING "Enable support for experimental historical access features (client)" OFF)
mark_as_advanced(UA_ENABLE_EXPERIMENTAL_HISTORIZING)

option(UA_FORCE_32BIT "Force compilation as 32-bit executable" OFF)
mark_as_advanced(UA_FORCE_32BIT)

option(UA_FORCE_CPP "Force compilation with a C++ compiler" OFF)
mark_as_advanced(UA_FORCE_CPP)
if(UA_FORCE_CPP)
    enable_language(CXX)
    add_definitions(-D__STDC_CONSTANT_MACROS) # We need the UINT32_C define
endif()

option(UA_FORCE_WERROR "Force compilation with -Werror (or /WX on MSVC)" ON)

#General PubSub setup
option(UA_ENABLE_PUBSUB "Enable the PubSub protocol" OFF)

option(UA_ENABLE_PUBSUB_ENCRYPTION "Enable encryption of the PubSub payload" OFF)
mark_as_advanced(UA_ENABLE_PUBSUB_ENCRYPTION)

option(UA_ENABLE_PUBSUB_INFORMATIONMODEL "Enable PubSub information model twin" OFF)

option(UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS "Enable PubSub informationmodel methods" OFF)

option(UA_ENABLE_PUBSUB_DELTAFRAMES "Enable sending of delta frames with only the changes" OFF)

option(UA_ENABLE_PUBSUB_FILE_CONFIG "Enable loading PubSub Config from file extension" OFF)
mark_as_advanced(UA_ENABLE_PUBSUB_FILE_CONFIG)

option(UA_ENABLE_PUBSUB_MONITORING "Enable monitoring of PubSub components (e.g. MessageReceiveTimeout)" OFF)
mark_as_advanced(UA_ENABLE_PUBSUB_MONITORING)

option(UA_ENABLE_PUBSUB_BUFMALLOC "Enable allocation with static memory buffer for time critical PubSub parts" OFF)
mark_as_advanced(UA_ENABLE_PUBSUB_BUFMALLOC)

#RT and Transport PubSub settings
option(UA_ENABLE_PUBSUB_ETH_UADP "Enable publish/subscribe UADP over Ethernet" OFF)

if(UA_ENABLE_PUBSUB_ETH_UADP)
    if(NOT CMAKE_SYSTEM MATCHES "Linux")
    message(FATAL_ERROR "UADP over Ethernet is only available on Linux.")
    endif()
endif()

if(UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS)
    if(NOT UA_ENABLE_PUBSUB_INFORMATIONMODEL)
        message(FATAL_ERROR "PubSub information model methods cannot be used with disabled PubSub information model.")
    endif()
endif()

if(UA_ENABLE_PUBSUB_INFORMATIONMODEL)
    if(NOT UA_ENABLE_PUBSUB)
        message(FATAL_ERROR "PubSub information model representation cannot be used with disabled PubSub function.")
    endif()
    if(UA_NAMESPACE_ZERO STREQUAL "MINIMAL")
        message(FATAL_ERROR "PubSub information model representation cannot be used with MINIMAL namespace zero.")
    endif()
endif()

if(UA_ENABLE_PUBSUB AND UA_MULTITHREADING GREATER_EQUAL 100)
    MESSAGE(WARNING "PubSub is not thread-safe for the 1.3 releases. Switch to a more current release for this feature.")
endif()

if(UA_ENABLE_PUBSUB_FILE_CONFIG)
    if(NOT UA_ENABLE_PUBSUB)
        message(FATAL_ERROR "PubSub needs to be enabled")
    endif()
endif()

if(UA_ENABLE_PUBSUB_MONITORING)
    if(NOT UA_ENABLE_PUBSUB)
        message(FATAL_ERROR "PubSub monitoring cannot be used with PubSub function disabled")
    endif()
endif()

if(UA_ENABLE_PUBSUB_BUFMALLOC)
    if(NOT UA_ENABLE_PUBSUB)
        message(FATAL_ERROR "PubSub buffer allocation cannot be used with PubSub function disabled")
    endif()
    if(NOT UA_ENABLE_MALLOC_SINGLETON)
        message(FATAL_ERROR "PubSub buffer allocation cannot be used without 'UA_ENABLE_MALLOC_SINGLETON' enabled")
    endif()
endif()

option(UA_ENABLE_PUBSUB_MQTT "Enable publish/subscribe with mqtt (experimental)" OFF)
mark_as_advanced(UA_ENABLE_PUBSUB_MQTT)
if(UA_ENABLE_PUBSUB_MQTT)
    if(NOT UA_FILE_MQTT)
        set(UA_FILE_MQTT ${open62541_MQTT_DIR}/src/mqtt.c)
    endif()
    if(NOT EXISTS "${UA_FILE_MQTT}")
        message(FATAL_ERROR "File ${UA_FILE_MQTT} not found.You probably need to initialize the git submodule for deps/mqtt-c.Therefore execute the following command \ngit submodule update --init --recursive")
    else()
        # Warn if run in Windows
        if(WIN32)
            MESSAGE(WARNING "Multithreading is enabled in MQTT plugin. This feature is under development and marked as EXPERIMENTAL")
        endif()
        if(NOT UA_ENABLE_PUBSUB)
        message(FATAL_ERROR "Mqtt cannot be used with disabled PubSub function.")
        endif()
        configure_file(${PROJECT_SOURCE_DIR}/deps/mqtt-c/include/mqtt.h "${PROJECT_BINARY_DIR}/src_generated/mqtt.h" COPYONLY)
        configure_file(${PROJECT_SOURCE_DIR}/deps/mqtt-c/include/mqtt_pal.h "${PROJECT_BINARY_DIR}/src_generated/mqtt_pal.h" COPYONLY)
        configure_file(${PROJECT_SOURCE_DIR}/plugins/mqtt/templates/posix_sockets.h "${PROJECT_BINARY_DIR}/src_generated/posix_sockets.h" COPYONLY)
    endif()
endif()

option(UA_ENABLE_MQTT_TLS_OPENSSL "Enable TLS support for publish/subscribe with mqtt (use OpenSSL)" OFF)
mark_as_advanced(UA_ENABLE_MQTT_TLS_OPENSSL)
option(UA_ENABLE_MQTT_TLS_MBEDTLS "Enable TLS support for publish/subscribe with mqtt (use MBEDTLS)" OFF)
mark_as_advanced(UA_ENABLE_MQTT_TLS_MBEDTLS)

option(UA_ENABLE_MQTT_TLS "Enable TLS support for publish/subscribe with mqtt" OFF)
mark_as_advanced(UA_ENABLE_MQTT_TLS)
if(UA_ENABLE_MQTT_TLS)
    if(NOT UA_ENABLE_PUBSUB_MQTT)
        message(FATAL_ERROR "Mqtt with TLS cannot be used without UA_ENABLE_PUBSUB_MQTT.")
    endif()
    if(NOT UA_ENABLE_ENCRYPTION)
        message(FATAL_ERROR "Mqtt with TLS cannot be used without UA_ENABLE_ENCRYPTION.")
    elseif(UA_ENABLE_ENCRYPTION STREQUAL "OPENSSL")
        set(UA_ENABLE_MQTT_TLS_OPENSSL ON)
        add_definitions(-DMQTT_USE_BIO)
        configure_file(${PROJECT_SOURCE_DIR}/plugins/mqtt/templates/openssl_sockets.h "${PROJECT_BINARY_DIR}/src_generated/openssl_sockets.h" COPYONLY)
    elseif(UA_ENABLE_ENCRYPTION STREQUAL "MBEDTLS")
        set(UA_ENABLE_MQTT_TLS_MBEDTLS ON)
        add_definitions(-DMQTT_USE_MBEDTLS)
        configure_file(${PROJECT_SOURCE_DIR}/plugins/mqtt/templates/mbedtls_sockets.h "${PROJECT_BINARY_DIR}/src_generated/mbedtls_sockets.h" COPYONLY)
    else()
        message(FATAL_ERROR "UA_ENABLE_ENCRYPTION is set to an undefined value")
    endif()
endif()

option(UA_ENABLE_STATUSCODE_DESCRIPTIONS "Enable conversion of StatusCode to human-readable error message" ON)
mark_as_advanced(UA_ENABLE_STATUSCODE_DESCRIPTIONS)

option(UA_ENABLE_TYPEDESCRIPTION "Add the type and member names to the UA_DataType structure" ON)
mark_as_advanced(UA_ENABLE_TYPEDESCRIPTION)

option(UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS "Set node description attribute for nodeset compiler generated nodes" ON)
mark_as_advanced(UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS)

option(UA_ENABLE_DETERMINISTIC_RNG "Do not seed the random number generator (e.g. for unit tests)." OFF)
mark_as_advanced(UA_ENABLE_DETERMINISTIC_RNG)

option(UA_ENABLE_MALLOC_SINGLETON
       "Use a global variable pointer for malloc (and free, ...) that can be switched at runtime" OFF)
mark_as_advanced(UA_ENABLE_MALLOC_SINGLETON)

option(UA_ENABLE_VALGRIND_INTERACTIVE "Enable dumping valgrind every iteration. CAUTION! SLOWDOWN!" OFF)
mark_as_advanced(UA_ENABLE_VALGRIND_INTERACTIVE)

option(UA_MSVC_FORCE_STATIC_CRT "Force linking with the static C-runtime library when compiling to static library with MSVC" ON)
mark_as_advanced(UA_MSVC_FORCE_STATIC_CRT)

option(UA_FILE_NS0 "Override the NodeSet xml file used to generate namespace zero")
mark_as_advanced(UA_FILE_NS0)

# Blacklist file passed as --blacklist to the nodeset compiler. All the given nodes will be removed from the generated
# nodeset, including all the references to and from that node. The format is a node id per line.
# Supported formats: "i=123" (for NS0), "ns=2;s=asdf" (matches NS2 in that specific file), or recommended
# "ns=http://opcfoundation.org/UA/DI/;i=123" namespace index independent node id
option(UA_FILE_NS0_BLACKLIST "File containing blacklisted nodes which should not be included in the generated nodeset code.")
mark_as_advanced(UA_FILE_NS0_BLACKLIST)

# Semaphores/file system may not be available on embedded devices. It can be
# disabled with the following option
option(UA_ENABLE_DISCOVERY_SEMAPHORE "Enable Discovery Semaphore support" ON)
mark_as_advanced(UA_ENABLE_DISCOVERY_SEMAPHORE)

option(UA_ENABLE_UNIT_TESTS_MEMCHECK "Use Valgrind (Linux) or DrMemory (Windows) to detect memory leaks when running the unit tests" OFF)
mark_as_advanced(UA_ENABLE_UNIT_TESTS_MEMCHECK)

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

option(UA_ENABLE_HARDENING "Enable Hardening measures (e.g. Stack-Protectors and Fortify)" ON)
mark_as_advanced(UA_ENABLE_HARDENING)

if(CMAKE_VERSION VERSION_GREATER 3.6)
    set(UA_ENABLE_STATIC_ANALYZER "OFF" CACHE STRING "Enable installed static analyzer during build process (off/minimal/reduced/full)")
    mark_as_advanced(UA_ENABLE_STATIC_ANALYZER)
    SET_PROPERTY(CACHE UA_ENABLE_STATIC_ANALYZER PROPERTY STRINGS "OFF" "MINIMAL" "REDUCED" "FULL")
endif()

option(UA_DEBUG_FILE_LINE_INFO "Enable file and line information as additional debugging output for error messages" OFF)
mark_as_advanced(UA_DEBUG_FILE_LINE_INFO)

if(CMAKE_BUILD_TYPE MATCHES DEBUG)
    set(UA_DEBUG_FILE_LINE_INFO ON)
endif()

# Build Targets
option(UA_BUILD_EXAMPLES "Build example servers and clients" OFF)
option(UA_BUILD_TOOLS "Build OPC UA shell tools" OFF)
option(UA_BUILD_UNIT_TESTS "Build the unit tests" OFF)
option(UA_BUILD_FUZZING "Build the fuzzing executables" OFF)
mark_as_advanced(UA_BUILD_FUZZING)
if(UA_BUILD_FUZZING)
    # oss-fuzz already defines this by default
    add_definitions(-DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
endif()

if(UA_ENABLE_EXPERIMENTAL_HISTORIZING)
    if(NOT UA_ENABLE_HISTORIZING)
        message(FATAL_ERROR "UA_ENABLE_EXPERIMENTAL_HISTORIZING cannot be used with disabled UA_ENABLE_HISTORIZING.")
    endif()
endif()

option(UA_BUILD_FUZZING_CORPUS "Build the fuzzing corpus" OFF)
mark_as_advanced(UA_BUILD_FUZZING_CORPUS)
if(UA_BUILD_FUZZING_CORPUS)
    add_definitions(-DUA_DEBUG_DUMP_PKGS_FILE)
    set(UA_ENABLE_TYPEDESCRIPTION ON CACHE STRING "" FORCE)
    #set(UA_DEBUG_DUMP_PKGS ON CACHE STRING "" FORCE)
endif()

option(UA_BUILD_OSS_FUZZ "Special build switch used in oss-fuzz" OFF)
mark_as_advanced(UA_BUILD_OSS_FUZZ)

# Advanced Build Targets
option(UA_PACK_DEBIAN "Special build switch used in .deb packaging" OFF)
mark_as_advanced(UA_PACK_DEBIAN)

# Building shared libs (dll, so). This option is written into ua_config.h.
set(UA_DYNAMIC_LINKING OFF)
if(BUILD_SHARED_LIBS)
  set(UA_DYNAMIC_LINKING ON)
  if(UA_ENABLE_DISCOVERY_MULTICAST)
      set(MDNSD_DYNAMIC_LINKING ON)
  endif()
endif()

if(UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS)
    MESSAGE(WARNING "UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS is enabled. The feature is under development and marked as EXPERIMENTAL")
endif()

if(UA_ENABLE_SUBSCRIPTIONS_EVENTS AND (NOT UA_ENABLE_SUBSCRIPTIONS))
    MESSAGE(WARNING "UA_ENABLE_SUBSCRIPTIONS_EVENTS needs Subscriptions enabled")
    set(UA_ENABLE_SUBSCRIPTIONS_EVENTS OFF)
endif()

########################
# Linting during build #
########################
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(linting_build)

######################
# External Libraries #
######################

list(APPEND open62541_LIBRARIES "")

if(UA_ENABLE_ENCRYPTION_OPENSSL OR UA_ENABLE_MQTT_TLS_OPENSSL)
    # use the OpenSSL encryption library
    # https://cmake.org/cmake/help/v3.0/module/FindOpenSSL.html
    find_package(OpenSSL REQUIRED)
    list(APPEND open62541_LIBRARIES ${OPENSSL_LIBRARIES})
    endif ()

if(UA_ENABLE_ENCRYPTION_LIBRESSL)
    # See https://github.com/libressl-portable/portable/blob/master/FindLibreSSL.cmake
    find_package(LibreSSL REQUIRED)
    list(APPEND open62541_LIBRARIES ${LIBRESSL_LIBRARIES})
    if(WIN32)
        # Add bestcrypt for random generator
        list(APPEND open62541_LIBRARIES bcrypt)
    endif()
endif()

if(UA_ENABLE_ENCRYPTION_MBEDTLS OR UA_ENABLE_PUBSUB_ENCRYPTION)
    # The recommended way is to install mbedtls via the OS package manager. If
    # that is not possible, manually compile mbedTLS and set the cmake variables
    # defined in /tools/cmake/FindMbedTLS.cmake.
    find_package(MbedTLS REQUIRED)
    list(APPEND open62541_LIBRARIES ${MBEDTLS_LIBRARIES})
endif()

if(UA_ENABLE_TPM2_SECURITY)
    list(APPEND open62541_LIBRARIES ${TPM2_LIB})
endif()

if(UA_ENABLE_WEBSOCKET_SERVER)
    # The recommended way is to install libwebsockets via the OS package manager. If
    # that is not possible, manually compile libwebsockets and set the cmake variables
    # defined in /tools/cmake/Findlibwebsockets.cmake
    find_package(libwebsockets REQUIRED)
    list(APPEND open62541_LIBRARIES ${LIBWEBSOCKETS_LIBRARIES})

    set(ua_architecture_directories_to_include ${ua_architecture_directories_to_include}
                                               ${LIBWEBSOCKETS_INCLUDE_DIR})
endif()

if(MINGW)
    # GCC stack protector support
    list(APPEND open62541_LIBRARIES ssp)
endif()

#####################
# Compiler Settings #
#####################

# Check if a C compiler flag is supported and add it (if supported)
# Taken from https://stackoverflow.com/a/33266748
include(CheckCCompilerFlag)
function(check_add_cc_flag CC_FLAG)
    string(FIND "${CMAKE_C_FLAGS}" "${CC_FLAG}" flag_already_set)
    if(flag_already_set EQUAL -1)
        message(STATUS "Test CC flag ${CC_FLAG}")
        check_c_compiler_flag("${CC_FLAG}" flag_supported)
        if(flag_supported)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CC_FLAG}" PARENT_SCOPE)
        endif()
        unset(flag_supported CACHE)
    endif()
endfunction()

if(NOT UA_FORCE_CPP AND (CMAKE_COMPILER_IS_GNUCC OR "x${CMAKE_C_COMPILER_ID}" STREQUAL "xClang"))
    check_add_cc_flag("-std=c99")   # C99 mode
    check_add_cc_flag("-pipe")      # Avoid writing temporary files (for compiler speed)
    check_add_cc_flag("-Wall")      # Warnings
    check_add_cc_flag("-Wextra")    # More warnings
    check_add_cc_flag("-Wpedantic") # Standard compliance
    if(UA_FORCE_WERROR)
        check_add_cc_flag("-Werror")    # All warnings are errors
    endif()

    check_add_cc_flag("-Wno-static-in-inline") # Clang doesn't like the use of static inline methods inside static inline methods
    check_add_cc_flag("-Wno-overlength-strings") # May happen in the nodeset compiler when complex values are directly encoded
    check_add_cc_flag("-Wno-unused-parameter") # some methods may require unused arguments to cast to a method pointer

    # Use a strict subset of the C and C++ languages
    check_add_cc_flag("-Wc++-compat")

    # Check that format strings (printf/scanf) are sane
    check_add_cc_flag("-Wformat")
    check_add_cc_flag("-Wformat-security")
    check_add_cc_flag("-Wformat-nonliteral")

    # Check prototype definitions
    check_add_cc_flag("-Wmissing-prototypes")
    check_add_cc_flag("-Wstrict-prototypes")
    check_add_cc_flag("-Wredundant-decls")

    check_add_cc_flag("-Wuninitialized")
    check_add_cc_flag("-Winit-self")
    check_add_cc_flag("-Wcast-qual")
    check_add_cc_flag("-Wstrict-overflow")
    check_add_cc_flag("-Wnested-externs")
    check_add_cc_flag("-Wmultichar")
    check_add_cc_flag("-Wundef")
    check_add_cc_flag("-fno-strict-aliasing") # fewer compiler assumptions about pointer types
    check_add_cc_flag("-fexceptions") # recommended for multi-threaded C code, also in combination with C++ code

    # Generate position-independent code for shared libraries (adds a performance penalty)
    if(BUILD_SHARED_LIBS)
        check_add_cc_flag("-fPIC")
    endif()

    # Force 32bit build
    if(UA_FORCE_32BIT)
	check_add_cc_flag("-m32")
    endif()

    if(NOT MINGW AND NOT UA_BUILD_OSS_FUZZ)
        if(UA_ENABLE_HARDENING)
            check_add_cc_flag("-fstack-protector-strong") # more performant stack protector, available since gcc 4.9
            check_add_cc_flag("-fstack-clash-protection") # increased reliability of stack overflow detection, available since gcc 8
            # future use (control flow integrity protection)
            if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
                check_add_cc_flag("-mcet")
                check_add_cc_flag("-fcf-protection")
            endif()
        endif()

        # IPO requires too much memory for unit tests
        # GCC docu recommends to compile all files with the same options, therefore ignore it completely
        if(NOT UA_BUILD_UNIT_TESTS AND NOT DEFINED CMAKE_INTERPROCEDURAL_OPTIMIZATION)
            # needed to check if IPO is supported (check needs cmake > 3.9)
            if("${CMAKE_VERSION}" VERSION_GREATER 3.9)
                cmake_policy(SET CMP0069 NEW) # needed as long as required cmake < 3.9
                include(CheckIPOSupported)
                check_ipo_supported(RESULT CC_HAS_IPO) # Inter Procedural Optimization / Link Time Optimization (should be same as -flto)
                if(CC_HAS_IPO)
                    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
                endif()
            endif()
        endif()
    endif()

    if(UA_ENABLE_AMALGAMATION)
        add_definitions(-Wno-unused-function)
    endif()

    if(UA_PACK_DEBIAN)
        remove_definitions(-Wno-static-in-inline)
    endif()

    # Linker
    set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "") # cmake sets -rdynamic by default

    # Debug
    if(BUILD_TYPE_LOWER_CASE STREQUAL "debug" AND UNIX AND NOT UA_BUILD_OSS_FUZZ AND
       "x${CMAKE_C_COMPILER_ID}" STREQUAL "xClang" AND NOT UA_ENABLE_UNIT_TESTS_MEMCHECK)
        # Add default sanitizer settings when using clang and Debug build.
        # This allows e.g. CLion to find memory locations for SegFaults
        message(STATUS "Sanitizer enabled")
        set(SANITIZER_FLAGS "-g -fno-omit-frame-pointer -gline-tables-only -fsanitize=address -fsanitize-address-use-after-scope -fsanitize=leak -fsanitize=undefined")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10.0)
            set(SANITIZER_FLAGS "${SANITIZER_FLAGS} -fsanitize-coverage=trace-pc-guard")
        endif()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZER_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
    endif()

    if(NOT MINGW AND UA_ENABLE_HARDENING AND ((CMAKE_BUILD_TYPE STREQUAL "Release") OR (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")))
        check_add_cc_flag("-D_FORTIFY_SOURCE=2") # run-time buffer overflow detection (needs at least -O1)
    endif()

    # Strip release builds
    if(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel" OR CMAKE_BUILD_TYPE STREQUAL "Release")
        check_add_cc_flag("-ffunction-sections")
        check_add_cc_flag("-fdata-sections")
        check_add_cc_flag("-fno-unwind-tables")
        check_add_cc_flag("-fno-asynchronous-unwind-tables")
        check_add_cc_flag("-fno-math-errno")
#        check_add_cc_flag("-fno-ident")

        # remove stack-protector with MinSizeRel
        if(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
            check_add_cc_flag("-fno-stack-protector")
        endif()
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
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3 /w44996")
  if(UA_FORCE_WERROR)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /WX") # Compiler warnings, error on warning
  endif()

  if(UA_MSVC_FORCE_STATIC_CRT AND NOT BUILD_SHARED_LIBS)
    set(CompilerFlags CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE)
    foreach(CompilerFlag ${CompilerFlags})
      string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
    endforeach()
  endif()
endif()

if(UA_BUILD_FUZZING OR UA_BUILD_OSS_FUZZ)
    set(UA_ENABLE_MALLOC_SINGLETON ON)
endif()

#########################
# Generate Main Library #
#########################

# Directory for generated sources
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/src_generated")

# Generate the config.h
configure_file(include/open62541/config.h.in ${PROJECT_BINARY_DIR}/src_generated/open62541/config.h)

# Generate a .pc file for pkg-config
configure_file(tools/open62541.pc.in ${PROJECT_BINARY_DIR}/src_generated/open62541.pc @ONLY)

if(UA_ENABLE_DISCOVERY_MULTICAST)
  include(GenerateExportHeader)
    set(MDNSD_LOGLEVEL 300 CACHE STRING "Level at which logs shall be reported" FORCE)
    # create a "fake" empty library to generate the export header macros
    add_library(libmdnsd ${PROJECT_SOURCE_DIR}/deps/mdnsd/libmdnsd/mdnsd.h)
    if (UA_FORCE_CPP)
        set_property(TARGET libmdnsd PROPERTY LINKER_LANGUAGE CXX)
    else()
        set_property(TARGET libmdnsd PROPERTY LINKER_LANGUAGE C)
    endif()
    set_property(TARGET libmdnsd PROPERTY DEFINE_SYMBOL "MDNSD_DYNAMIC_LINKING_EXPORT")
    configure_file("deps/mdnsd/libmdnsd/mdnsd_config_extra.in"
                   "${PROJECT_BINARY_DIR}/src_generated/mdnsd_config_extra")
    file(READ "${PROJECT_BINARY_DIR}/src_generated/mdnsd_config_extra" MDNSD_CONFIG_EXTRA)
    generate_export_header(libmdnsd
      EXPORT_FILE_NAME "${PROJECT_BINARY_DIR}/src_generated/mdnsd_config.h"
      BASE_NAME MDNSD
      DEFINE_NO_DEPRECATED
      CUSTOM_CONTENT_FROM_VARIABLE MDNSD_CONFIG_EXTRA)
endif()

set(exported_headers ${ua_architecture_headers_beginning})

if(NOT "${UA_AMALGAMATION_ARCHITECTURES}" STREQUAL "")
    foreach(arch ${UA_AMALGAMATION_ARCHITECTURES})
        list(APPEND exported_headers ${PROJECT_SOURCE_DIR}/arch/${arch}/ua_architecture.h)
    endforeach()
else()
    list(APPEND exported_headers ${PROJECT_SOURCE_DIR}/arch/${UA_ARCHITECTURE}/ua_architecture.h)
endif()


if(UA_ENABLE_HISTORIZING)
    # Historizing needs to be included before server.h
    set(historizing_exported_headers
         ${PROJECT_SOURCE_DIR}/include/open62541/plugin/historydatabase.h)
endif()

set(exported_headers ${PROJECT_BINARY_DIR}/src_generated/open62541/config.h
                     ${PROJECT_SOURCE_DIR}/deps/ms_stdint.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/architecture_definitions.h
                     ${exported_headers}
                     ${PROJECT_BINARY_DIR}/src_generated/open62541/statuscodes.h
                     ${PROJECT_BINARY_DIR}/src_generated/open62541/nodeids.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/common.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/types.h
                     ${PROJECT_BINARY_DIR}/src_generated/open62541/types_generated.h
                     ${PROJECT_BINARY_DIR}/src_generated/open62541/types_generated_handling.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/util.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/plugin/log.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/plugin/network.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/plugin/accesscontrol.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/plugin/pki.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/plugin/securitypolicy.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/plugin/pubsub.h
                     ${PROJECT_SOURCE_DIR}/deps/ziptree.h
                     ${PROJECT_SOURCE_DIR}/deps/aa_tree.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/plugin/nodestore.h
                     ${historizing_exported_headers}
                     ${PROJECT_SOURCE_DIR}/include/open62541/server_pubsub.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/server.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/client.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/client_highlevel.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/client_subscriptions.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/client_highlevel_async.h)

set(internal_headers ${PROJECT_SOURCE_DIR}/deps/open62541_queue.h
                     ${PROJECT_SOURCE_DIR}/deps/pcg_basic.h
                     ${PROJECT_SOURCE_DIR}/deps/libc_time.h
                     ${PROJECT_SOURCE_DIR}/deps/base64.h
                     ${PROJECT_SOURCE_DIR}/src/ua_types_encoding_binary.h
                     ${PROJECT_SOURCE_DIR}/src/ua_util_internal.h
                     ${PROJECT_BINARY_DIR}/src_generated/open62541/transport_generated.h
                     ${PROJECT_BINARY_DIR}/src_generated/open62541/transport_generated_handling.h
                     ${PROJECT_SOURCE_DIR}/src/ua_connection_internal.h
                     ${PROJECT_SOURCE_DIR}/src/ua_securechannel.h
                     ${PROJECT_SOURCE_DIR}/src/ua_timer.h
                     ${PROJECT_SOURCE_DIR}/src/server/ua_session.h
                     ${PROJECT_SOURCE_DIR}/src/server/ua_subscription.h
                     ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_networkmessage.h
                     ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub.h
                     ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_manager.h
                     ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_ns0.h
                     ${PROJECT_SOURCE_DIR}/src/server/ua_server_async.h
                     ${PROJECT_SOURCE_DIR}/src/server/ua_server_internal.h
                     ${PROJECT_SOURCE_DIR}/src/server/ua_services.h
                     ${PROJECT_SOURCE_DIR}/src/client/ua_client_internal.h
                     ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_config.h)

# TODO: make client optional
set(lib_sources ${PROJECT_SOURCE_DIR}/src/ua_types.c
                ${PROJECT_SOURCE_DIR}/src/ua_types_encoding_binary.c
                ${PROJECT_SOURCE_DIR}/src/ua_types_print.c
                ${PROJECT_BINARY_DIR}/src_generated/open62541/types_generated.c
                ${PROJECT_BINARY_DIR}/src_generated/open62541/transport_generated.c
                ${PROJECT_BINARY_DIR}/src_generated/open62541/statuscodes.c
                ${PROJECT_SOURCE_DIR}/src/ua_util.c
                ${PROJECT_SOURCE_DIR}/src/ua_timer.c
                ${PROJECT_SOURCE_DIR}/src/ua_connection.c
                ${PROJECT_SOURCE_DIR}/src/ua_securechannel.c
                ${PROJECT_SOURCE_DIR}/src/ua_securechannel_crypto.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_session.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_nodes.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_server.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_server_ns0.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_server_ns0_diagnostics.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_server_config.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_server_binary.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_server_utils.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_server_discovery.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_server_async.c
                ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_networkmessage.c
                ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_writer.c
                ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_writergroup.c
                ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_reader.c
                ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_readergroup.c
                ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_manager.c
                ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_ns0.c
                # services
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_view.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_method.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_session.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_attribute.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_discovery.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_subscription.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_monitoreditem.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_securechannel.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_nodemanagement.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_discovery_multicast.c
                # client
                ${PROJECT_SOURCE_DIR}/src/client/ua_client.c
                ${PROJECT_SOURCE_DIR}/src/client/ua_client_connect.c
                ${PROJECT_SOURCE_DIR}/src/client/ua_client_discovery.c
                ${PROJECT_SOURCE_DIR}/src/client/ua_client_highlevel.c
                ${PROJECT_SOURCE_DIR}/src/client/ua_client_subscriptions.c

                # dependencies
                ${PROJECT_SOURCE_DIR}/deps/libc_time.c
                ${PROJECT_SOURCE_DIR}/deps/pcg_basic.c
                ${PROJECT_SOURCE_DIR}/deps/base64.c
                ${PROJECT_SOURCE_DIR}/deps/aa_tree.c
                ${PROJECT_SOURCE_DIR}/deps/ziptree.c

                ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_config.c)

set(default_plugin_headers ${PROJECT_SOURCE_DIR}/plugins/include/open62541/plugin/accesscontrol_default.h
                           ${PROJECT_SOURCE_DIR}/plugins/include/open62541/plugin/pki_default.h
                           ${PROJECT_SOURCE_DIR}/plugins/include/open62541/plugin/log_stdout.h
                           ${PROJECT_SOURCE_DIR}/plugins/include/open62541/plugin/nodestore_default.h
                           ${PROJECT_SOURCE_DIR}/plugins/include/open62541/server_config_default.h
                           ${PROJECT_SOURCE_DIR}/plugins/include/open62541/client_config_default.h
                           ${PROJECT_SOURCE_DIR}/plugins/include/open62541/plugin/securitypolicy_default.h
)

set(default_plugin_sources ${PROJECT_SOURCE_DIR}/plugins/ua_log_stdout.c
                           ${PROJECT_SOURCE_DIR}/plugins/ua_accesscontrol_default.c
                           ${PROJECT_SOURCE_DIR}/plugins/ua_nodestore_ziptree.c
                           ${PROJECT_SOURCE_DIR}/plugins/ua_nodestore_hashmap.c
                           ${PROJECT_SOURCE_DIR}/plugins/ua_config_default.c
                           ${PROJECT_SOURCE_DIR}/plugins/crypto/ua_pki_none.c
                           ${PROJECT_SOURCE_DIR}/plugins/crypto/ua_securitypolicy_none.c
)

# Syslog-logging on Linux and Unices
if(UNIX)
    list(APPEND default_plugin_headers
        ${PROJECT_SOURCE_DIR}/plugins/include/open62541/plugin/log_syslog.h)
    list(APPEND default_plugin_sources ${PROJECT_SOURCE_DIR}/plugins/ua_log_syslog.c)
endif()

if(UA_GENERATED_NAMESPACE_ZERO)
    list(APPEND internal_headers ${PROJECT_BINARY_DIR}/src_generated/open62541/namespace0_generated.h)
    list(APPEND lib_sources ${PROJECT_BINARY_DIR}/src_generated/open62541/namespace0_generated.c)
endif()

if(UA_ENABLE_PARSING)
    list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/src/ua_types_lex.c)
endif()

# Always include encryption plugins into the amalgamation
# Use guards in the files to ensure that UA_ENABLE_ENCRYPTON_MBEDTLS and UA_ENABLE_ENCRYPTION_OPENSSL are honored.

if(UA_ENABLE_ENCRYPTION_MBEDTLS OR UA_ENABLE_PUBSUB_ENCRYPTION OR UA_ENABLE_AMALGAMATION)
  list(INSERT default_plugin_sources 0
    ${PROJECT_SOURCE_DIR}/plugins/crypto/mbedtls/securitypolicy_mbedtls_common.h
    ${PROJECT_SOURCE_DIR}/plugins/crypto/mbedtls/securitypolicy_mbedtls_common.c)
endif()

if(UA_ENABLE_PUBSUB_ENCRYPTION)
  list(INSERT default_plugin_sources 1
    ${PROJECT_SOURCE_DIR}/plugins/crypto/mbedtls/securitypolicy_pubsub_aes128ctr.c
    ${PROJECT_SOURCE_DIR}/plugins/crypto/mbedtls/securitypolicy_pubsub_aes256ctr.c)
endif()

if(UA_ENABLE_ENCRYPTION_MBEDTLS OR UA_ENABLE_ENCRYPTION_OPENSSL OR UA_ENABLE_ENCRYPTION_LIBRESSL OR UA_ENABLE_AMALGAMATION)
  list(APPEND default_plugin_headers
      ${PROJECT_SOURCE_DIR}/plugins/include/open62541/plugin/create_certificate.h
  )
endif()

if(UA_ENABLE_ENCRYPTION_MBEDTLS OR UA_ENABLE_AMALGAMATION)
list(INSERT default_plugin_sources 1
     ${PROJECT_SOURCE_DIR}/plugins/crypto/mbedtls/ua_securitypolicy_basic128rsa15.c
     ${PROJECT_SOURCE_DIR}/plugins/crypto/mbedtls/ua_securitypolicy_basic256.c
     ${PROJECT_SOURCE_DIR}/plugins/crypto/mbedtls/ua_securitypolicy_basic256sha256.c
     ${PROJECT_SOURCE_DIR}/plugins/crypto/mbedtls/ua_securitypolicy_aes128sha256rsaoaep.c
     ${PROJECT_SOURCE_DIR}/plugins/crypto/mbedtls/ua_pki_mbedtls.c)
endif()

if(UA_ENABLE_TPM2_SECURITY)
list(INSERT default_plugin_sources 0
     ${PROJECT_SOURCE_DIR}/plugins/crypto/pkcs11/securitypolicy_pubsub_aes128ctr_tpm.c
     ${PROJECT_SOURCE_DIR}/plugins/crypto/pkcs11/securitypolicy_pubsub_aes256ctr_tpm.c)
endif()

if(UA_ENABLE_ENCRYPTION_OPENSSL OR UA_ENABLE_ENCRYPTION_LIBRESSL OR UA_ENABLE_AMALGAMATION)
list(INSERT default_plugin_sources 0
     ${PROJECT_SOURCE_DIR}/plugins/crypto/openssl/securitypolicy_openssl_common.h
     ${PROJECT_SOURCE_DIR}/plugins/crypto/openssl/ua_openssl_version_abstraction.h
     ${PROJECT_SOURCE_DIR}/plugins/crypto/openssl/securitypolicy_openssl_common.c
     ${PROJECT_SOURCE_DIR}/plugins/crypto/openssl/ua_openssl_basic128rsa15.c
     ${PROJECT_SOURCE_DIR}/plugins/crypto/openssl/ua_openssl_basic256.c
     ${PROJECT_SOURCE_DIR}/plugins/crypto/openssl/ua_openssl_basic256sha256.c
     ${PROJECT_SOURCE_DIR}/plugins/crypto/openssl/ua_openssl_aes128sha256rsaoaep.c
     ${PROJECT_SOURCE_DIR}/plugins/crypto/openssl/ua_openssl_create_certificate.c
     ${PROJECT_SOURCE_DIR}/plugins/crypto/openssl/ua_pki_openssl.c)
endif()

if(UA_ENABLE_HISTORIZING)

    list(APPEND default_plugin_headers
         ${PROJECT_SOURCE_DIR}/plugins/include/open62541/plugin/historydata/history_data_backend.h
         ${PROJECT_SOURCE_DIR}/plugins/include/open62541/plugin/historydata/history_data_gathering.h
         ${PROJECT_SOURCE_DIR}/plugins/include/open62541/plugin/historydata/history_database_default.h
         ${PROJECT_SOURCE_DIR}/plugins/include/open62541/plugin/historydata/history_data_gathering_default.h
         ${PROJECT_SOURCE_DIR}/plugins/include/open62541/plugin/historydata/history_data_backend_memory.h
         )
    list(APPEND default_plugin_sources
         ${PROJECT_SOURCE_DIR}/plugins/historydata/ua_history_data_backend_memory.c
         ${PROJECT_SOURCE_DIR}/plugins/historydata/ua_history_data_gathering_default.c
         ${PROJECT_SOURCE_DIR}/plugins/historydata/ua_history_database_default.c
         )
endif()

if(UA_ENABLE_DISCOVERY)
    list(INSERT internal_headers 13 ${PROJECT_SOURCE_DIR}/src/server/ua_discovery_manager.h)
    list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/src/server/ua_discovery_manager.c)
endif()

if(UA_ENABLE_PUBSUB)
    list(APPEND default_plugin_headers ${PROJECT_SOURCE_DIR}/plugins/include/open62541/plugin/pubsub_udp.h)
    list(APPEND default_plugin_sources ${PROJECT_SOURCE_DIR}/plugins/ua_pubsub_udp.c)
    if(UA_ENABLE_PUBSUB_ETH_UADP)
        list(APPEND default_plugin_headers ${PROJECT_SOURCE_DIR}/plugins/include/open62541/plugin/pubsub_ethernet.h)
        list(APPEND default_plugin_sources ${PROJECT_SOURCE_DIR}/plugins/ua_pubsub_ethernet.c)
        find_library(LIB_BPF bpf)
        if(LIB_BPF)
            list(APPEND open62541_LIBRARIES bpf)
        else()
            message("bpf library not found. XDP functionalities will not be available.")
        endif()
    endif()
    if(UA_ENABLE_PUBSUB_MQTT)
        if(WIN32)
            MESSAGE(WARNING "Multithreading is enabled in MQTT plugin. This feature is under development and marked as EXPERIMENTAL")
        else()
            list(APPEND default_plugin_headers ${PROJECT_SOURCE_DIR}/deps/mqtt-c/include/mqtt_pal.h
                 ${PROJECT_SOURCE_DIR}/deps/mqtt-c/include/mqtt.h
                 ${PROJECT_SOURCE_DIR}/plugins/include/open62541/plugin/pubsub_mqtt.h
                 ${PROJECT_SOURCE_DIR}/plugins/mqtt/ua_mqtt-c_adapter.h)
            list(APPEND default_plugin_sources ${PROJECT_SOURCE_DIR}/deps/mqtt-c/src/mqtt_pal.c
                 ${PROJECT_SOURCE_DIR}/deps/mqtt-c/src/mqtt.c
                 ${PROJECT_SOURCE_DIR}/plugins/ua_pubsub_mqtt.c
                 ${PROJECT_SOURCE_DIR}/plugins/mqtt/ua_mqtt-c_adapter.c)
        endif()
    endif()
    if(UA_ENABLE_MALLOC_SINGLETON AND UA_ENABLE_PUBSUB_BUFMALLOC)
        list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_bufmalloc.c)
        list(APPEND internal_headers ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_bufmalloc.h)
    endif()
    if(UA_ENABLE_PUBSUB_ENCRYPTION)
        list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_security.c)
    endif()
endif()

if(UA_ENABLE_JSON_ENCODING)
    if(UA_ENABLE_PUBSUB)
        list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_networkmessage_json.c)
    endif()
    list(APPEND internal_headers ${PROJECT_SOURCE_DIR}/deps/jsmn/jsmn.h
                                 ${PROJECT_SOURCE_DIR}/deps/string_escape.h
                                 ${PROJECT_SOURCE_DIR}/deps/itoa.h
                                 ${PROJECT_SOURCE_DIR}/deps/atoi.h
                                 ${PROJECT_SOURCE_DIR}/src/ua_types_encoding_json.h)
    list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/deps/jsmn/jsmn.c
                            ${PROJECT_SOURCE_DIR}/deps/string_escape.c
                            ${PROJECT_SOURCE_DIR}/deps/itoa.c
                            ${PROJECT_SOURCE_DIR}/deps/atoi.c
                            ${PROJECT_SOURCE_DIR}/src/ua_types_encoding_json.c)
endif()

if(UA_ENABLE_SUBSCRIPTIONS)
    list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/src/server/ua_subscription.c
                            ${PROJECT_SOURCE_DIR}/src/server/ua_subscription_monitoreditem.c
                            ${PROJECT_SOURCE_DIR}/src/server/ua_subscription_datachange.c)
    if(UA_ENABLE_SUBSCRIPTIONS_EVENTS)
        if(UA_NAMESPACE_ZERO STREQUAL "MINIMAL")
            message(FATAL_ERROR "Events require at least the reduced Namespace Zero")
        endif()
        list(APPEND lib_sources
                    ${PROJECT_SOURCE_DIR}/src/server/ua_subscription_events.c
                    ${PROJECT_SOURCE_DIR}/src/server/ua_subscription_events_filter.c)
        if(UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS)
            list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/src/server/ua_subscription_alarms_conditions.c)
        endif()
    endif()
endif()

if(UA_DEBUG_DUMP_PKGS)
    list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/plugins/ua_debug_dump_pkgs.c)
endif()

if(UA_ENABLE_DISCOVERY_MULTICAST)
    # prepend in list, otherwise it complains that winsock2.h has to be included before windows.h
    set(internal_headers ${PROJECT_BINARY_DIR}/src_generated/mdnsd_config.h
                         ${PROJECT_SOURCE_DIR}/deps/mdnsd/libmdnsd/1035.h
                         ${PROJECT_SOURCE_DIR}/deps/mdnsd/libmdnsd/xht.h
                         ${PROJECT_SOURCE_DIR}/deps/mdnsd/libmdnsd/sdtxt.h
                         ${PROJECT_SOURCE_DIR}/deps/mdnsd/libmdnsd/mdnsd.h
                         ${internal_headers} )
    set(lib_sources ${PROJECT_SOURCE_DIR}/src/server/ua_server_discovery_mdns.c
        ${PROJECT_SOURCE_DIR}/deps/mdnsd/libmdnsd/1035.c
        ${PROJECT_SOURCE_DIR}/deps/mdnsd/libmdnsd/xht.c
        ${PROJECT_SOURCE_DIR}/deps/mdnsd/libmdnsd/sdtxt.c
        ${PROJECT_SOURCE_DIR}/deps/mdnsd/libmdnsd/mdnsd.c
        ${lib_sources})
endif()

if(UA_BUILD_FUZZING OR UA_BUILD_OSS_FUZZ)
    set(lib_sources
        ${lib_sources}
        ${PROJECT_SOURCE_DIR}/tests/fuzz/custom_memory_manager.c)
endif()

#########################
# Generate source files #
#########################

# List of nodeset files combined into NS0 generated file
set(UA_FILE_NODESETS)

if(UA_NAMESPACE_ZERO STREQUAL "FULL" OR UA_NAMESPACE_ZERO STREQUAL "LATEST_1_05")
    if(NOT UA_FILE_NS0)
        set(UA_FILE_NS0 ${open62541_NODESET_DIR}/Schema/Opc.Ua.NodeSet2.xml)
    endif()
    set(UA_FILE_NODESETS "${UA_FILE_NS0}")

    if(NOT EXISTS "${UA_FILE_NS0}")
        message(FATAL_ERROR "File ${UA_FILE_NS0} not found. You probably need to initialize the git submodule for deps/ua-nodeset or set open62541_NODESET_DIR.")
    else()
        # If the "LATEST_1_05" flag is set, it must be checked whether the nodeset NS0 is on the latest version.
        if(UA_NAMESPACE_ZERO STREQUAL "LATEST_1_05")
            file(READ ${UA_FILE_NS0} ver LIMIT 5000)
            string(REGEX MATCH ".*http:\\/\\/opcfoundation\\.org\\/UA\\/\" Version=\"1\\.05" match ${ver})
            if(NOT match)
                message(FATAL_ERROR "File ${UA_FILE_NS0} is set incorrectly. You probably need to checkout the git submodule for deps/ua-nodeset to version 1.05. Therefore execute the following command inside the deps/ua-nodeset directory \ngit checkout v1.05")
            endif()
        endif()
    endif()


    set(UA_FILE_NODEIDS ${open62541_NODESET_DIR}/Schema/NodeIds.csv)
    set(UA_FILE_STATUSCODES ${open62541_NODESET_DIR}/Schema/StatusCode.csv)
    set(UA_FILE_TYPES_BSD ${open62541_NODESET_DIR}/Schema/Opc.Ua.Types.bsd)
else()
    if(NOT UA_FILE_NS0)
        set(UA_FILE_NS0 ${PROJECT_SOURCE_DIR}/tools/schema/Opc.Ua.NodeSet2.Reduced.xml)
    endif()
    set(UA_FILE_NODESETS "${UA_FILE_NS0}")

    set(UA_FILE_DATATYPES ${PROJECT_SOURCE_DIR}/tools/schema/datatypes_minimal.txt)
    set(UA_FILE_NODEIDS ${PROJECT_SOURCE_DIR}/tools/schema/NodeIds.csv)
    set(UA_FILE_STATUSCODES ${PROJECT_SOURCE_DIR}/tools/schema/StatusCode.csv)
    set(UA_FILE_TYPES_BSD ${PROJECT_SOURCE_DIR}/tools/schema/Opc.Ua.Types.bsd)

    if(UA_ENABLE_METHODCALLS)
        list(APPEND UA_FILE_DATATYPES ${PROJECT_SOURCE_DIR}/tools/schema/datatypes_method.txt)
    endif()

    if(UA_ENABLE_DIAGNOSTICS)
        list(APPEND UA_FILE_DATATYPES ${PROJECT_SOURCE_DIR}/tools/schema/datatypes_diagnostics.txt)
        list(APPEND UA_FILE_NODESETS ${PROJECT_SOURCE_DIR}/tools/schema/Opc.Ua.NodeSet2.DiagnosticsMinimal.xml)
    endif()

    if(UA_ENABLE_SUBSCRIPTIONS)
        list(APPEND UA_FILE_DATATYPES ${PROJECT_SOURCE_DIR}/tools/schema/datatypes_subscriptions.txt)
    endif()

    if(UA_ENABLE_SUBSCRIPTIONS_EVENTS)
        list(APPEND UA_FILE_DATATYPES ${PROJECT_SOURCE_DIR}/tools/schema/datatypes_events.txt)
        list(APPEND UA_FILE_NODESETS ${PROJECT_SOURCE_DIR}/tools/schema/Opc.Ua.NodeSet2.EventsMinimal.xml)
    endif()

    if(UA_ENABLE_HISTORIZING)
        list(APPEND UA_FILE_DATATYPES ${PROJECT_SOURCE_DIR}/tools/schema/datatypes_historizing.txt)
        list(APPEND UA_FILE_NODESETS ${PROJECT_SOURCE_DIR}/tools/schema/Opc.Ua.NodeSet2.HistorizingMinimal.xml)
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
            list(APPEND UA_FILE_NODESETS ${PROJECT_SOURCE_DIR}/tools/schema/Opc.Ua.NodeSet2.PubSubMinimal.xml)
        endif()
    endif()
	if(UA_ENABLE_DA)
		list(APPEND UA_FILE_DATATYPES ${PROJECT_SOURCE_DIR}/tools/schema/datatypes_dataaccess.txt)
        set(UA_NODESET_FILE_DA ${PROJECT_SOURCE_DIR}/tools/schema/Opc.Ua.NodeSet2.Part8_Subset.xml)
    endif()
    if(UA_ENABLE_TYPEDESCRIPTION)
        list(APPEND UA_FILE_DATATYPES ${PROJECT_SOURCE_DIR}/tools/schema/datatypes_typedescription.txt)
    endif()
endif()

# standard-defined data types
ua_generate_datatypes(
    BUILTIN
    NAME "types"
    TARGET_SUFFIX "types"
    NAMESPACE_IDX 0
    FILE_CSV "${UA_FILE_NODEIDS}"
    FILES_BSD "${UA_FILE_TYPES_BSD}"
    FILES_SELECTED ${UA_FILE_DATATYPES}
)

# transport data types
ua_generate_datatypes(
    INTERNAL
    NAME "transport"
    TARGET_SUFFIX "transport"
    NAMESPACE_IDX 1
    FILE_CSV "${UA_FILE_NODEIDS}"
    IMPORT_BSD "TYPES#${UA_FILE_TYPES_BSD}"
    FILES_BSD "${PROJECT_SOURCE_DIR}/tools/schema/Custom.Opc.Ua.Transport.bsd"
    FILES_SELECTED "${PROJECT_SOURCE_DIR}/tools/schema/datatypes_transport.txt"
)

# statuscode explanation
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/open62541/statuscodes.h
                          ${PROJECT_BINARY_DIR}/src_generated/open62541/statuscodes.c
        PRE_BUILD
        COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/generate_statuscode_descriptions.py
        ${UA_FILE_STATUSCODES} ${PROJECT_BINARY_DIR}/src_generated/open62541/statuscodes
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_statuscode_descriptions.py
                ${UA_FILE_STATUSCODES})

# Header containing defines for all NodeIds
ua_generate_nodeid_header(
    NAME "nodeids"
    ID_PREFIX "NS0"
    TARGET_SUFFIX "ids-ns0"
    FILE_CSV "${UA_FILE_NODEIDS}"
)

# we need a custom target to avoid that the generator is called concurrently and
# thus overwriting files while the other thread is compiling
add_custom_target(open62541-generator-statuscode DEPENDS
        ${PROJECT_BINARY_DIR}/src_generated/open62541/nodeids.h
        ${PROJECT_BINARY_DIR}/src_generated/open62541/statuscodes.h
        ${PROJECT_BINARY_DIR}/src_generated/open62541/statuscodes.c)

if(UA_ENABLE_AMALGAMATION)
    # single-file release
    add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/open62541.h
                       PRE_BUILD
                       COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/amalgamate.py
                               ${OPEN62541_VERSION} ${CMAKE_CURRENT_BINARY_DIR}/open62541.h
                               ${exported_headers} ${default_plugin_headers} ${ua_architecture_headers}
                       DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/amalgamate.py
                               ${exported_headers} ${default_plugin_headers} ${ua_architecture_headers})

    add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/open62541.c
                       PRE_BUILD
                       COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/amalgamate.py
                               ${OPEN62541_VERSION} ${CMAKE_CURRENT_BINARY_DIR}/open62541.c
                               ${internal_headers} ${lib_sources} ${default_plugin_sources} ${ua_architecture_sources}
                       DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/amalgamate.py ${internal_headers}
                               ${lib_sources} ${default_plugin_sources} ${ua_architecture_sources} )

    add_custom_target(open62541-amalgamation-source DEPENDS ${PROJECT_BINARY_DIR}/open62541.c)
    add_custom_target(open62541-amalgamation-header DEPENDS ${PROJECT_BINARY_DIR}/open62541.h)

    add_dependencies(open62541-amalgamation-header open62541-generator-types)
    add_dependencies(open62541-amalgamation-source open62541-generator-types
                     open62541-generator-transport open62541-generator-statuscode)
endif()

ua_generate_nodeset(
    NAME "ns0"
    FILE ${UA_FILE_NODESETS} ${UA_NODESET_FILE_DA}
    INTERNAL
    BLACKLIST ${UA_FILE_NS0_BLACKLIST}
    IGNORE "${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/NodeID_NS0_Base.txt"
    DEPENDS_TARGET "open62541-generator-types"
)

# stack protector and optimization needs to be disabled for the huge ns0 file, otherwise debian packaging fails due to long build times.
# We also disable optimization on Appveyor builds, since they take almost an hour otherwise
if(UA_PACK_DEBIAN OR (NOT "$ENV{APPVEYOR}" STREQUAL "") OR (
        (CMAKE_BUILD_TYPE STREQUAL "MinSizeRel" OR CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo") AND (
            # List of compilers which have problems with the huge ns0 optimization
            (("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang") AND (CMAKE_C_COMPILER_VERSION VERSION_LESS 7.0))
        )
   ))
    set_source_files_properties(${PROJECT_BINARY_DIR}/src_generated/open62541/namespace0_generated.c PROPERTIES COMPILE_FLAGS "-fno-stack-protector -O0")
endif()

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
    target_include_directories(open62541-object PRIVATE "${ua_architecture_directories_to_include}")
    if(UA_ENABLE_ENCRYPTION_MBEDTLS)
        target_include_directories(open62541-object PRIVATE ${MBEDTLS_INCLUDE_DIRS})
    endif()
    if(UA_ENABLE_ENCRYPTION_OPENSSL OR UA_ENABLE_MQTT_TLS_OPENSSL)
        target_include_directories(open62541-object PRIVATE ${OPENSSL_INCLUDE_DIR})
    endif()
    if(UA_ENABLE_ENCRYPTION_LIBRESSL)
        target_include_directories(open62541-object PRIVATE ${LIBRESSL_INCLUDE_DIR})
    endif()

    # make sure the open62541_amalgamation target builds before so that amalgamation is finished and it is not executed again for open62541-object
    # and thus may overwrite the amalgamation result during multiprocessor compilation
    # the header is already a dependency of open62541 target itself
    add_custom_target(open62541-code-generation DEPENDS
                      open62541-amalgamation-header
                      open62541-generator-types
                      open62541-generator-transport
                      open62541-generator-statuscode
                      open62541-amalgamation-source
    )

    add_library(open62541 $<TARGET_OBJECTS:open62541-object>)
    # the only directory that needs to be included if open62541 (amalgameted) target from
    # the build directory is ${PROJECT_BINARY_DIR}, that contains the generated open62541.h
    target_include_directories(open62541 PUBLIC $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>)

    if(UA_FORCE_CPP)
        set_source_files_properties(${PROJECT_BINARY_DIR}/open62541.c PROPERTIES LANGUAGE CXX)
    endif()

    add_dependencies(open62541-amalgamation-source open62541-generator-namespace)
    add_dependencies(open62541-amalgamation-header open62541-generator-namespace)
else()
    add_library(open62541-object OBJECT ${lib_sources} ${internal_headers} ${exported_headers})

    add_custom_target(open62541-code-generation DEPENDS
                      open62541-generator-types
                      open62541-generator-transport
                      open62541-generator-statuscode
                      open62541-generator-namespace
                      )

    target_include_directories(open62541-object PRIVATE ${PROJECT_SOURCE_DIR}/src)

    add_library(open62541-plugins OBJECT ${default_plugin_sources} ${ua_architecture_sources} ${exported_headers})
    add_dependencies(open62541-plugins open62541-generator-types open62541-generator-transport open62541-generator-namespace)
    target_include_directories(open62541-plugins PRIVATE ${PROJECT_SOURCE_DIR}/plugins)
    target_include_directories(open62541-plugins PRIVATE ${PROJECT_BINARY_DIR}/src_generated)
    target_compile_definitions(open62541-plugins PRIVATE -DUA_DYNAMIC_LINKING_EXPORT)
    set_target_properties(open62541-plugins PROPERTIES FOLDER "open62541/lib")

    if(UA_PACK_DEBIAN)
        add_library(open62541-static STATIC $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-plugins>)
        set_target_properties(open62541-static PROPERTIES OUTPUT_NAME open62541)
    endif()
    add_library(open62541 $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-plugins>)

    if(UA_FORCE_CPP)
        set_source_files_properties(${lib_sources} PROPERTIES LANGUAGE CXX)
        set_source_files_properties(${default_plugin_sources} ${ua_architecture_sources} PROPERTIES LANGUAGE CXX)
    endif()

    if(UA_ENABLE_ENCRYPTION_LIBRESSL)
        # Prevent Wincrypt override warning.
        target_compile_definitions(open62541-plugins PUBLIC NOCRYPT=1)
        target_compile_definitions(open62541-object PUBLIC NOCRYPT=1)
        target_compile_definitions(open62541 PUBLIC NOCRYPT=1)
    endif()

    # Declare include directories
    function(include_directories_private)
        foreach(_include_dir IN ITEMS ${ARGN})
            target_include_directories(open62541-object PRIVATE ${_include_dir})
            target_include_directories(open62541-plugins PRIVATE ${_include_dir})
        endforeach()
    endfunction()

    function(include_directories_public)
        include_directories_private(${ARGN})
        foreach(_include_dir IN ITEMS ${ARGN})
            target_include_directories(open62541 PUBLIC $<BUILD_INTERFACE:${_include_dir}>)
        endforeach()
    endfunction()

    # Public includes
    include_directories_public(${ua_architecture_directories_to_include}
                               "${PROJECT_SOURCE_DIR}/include"
                               "${PROJECT_SOURCE_DIR}/plugins/include"
                               "${PROJECT_SOURCE_DIR}/deps"
                               "${PROJECT_SOURCE_DIR}/src/pubsub"
                               "${PROJECT_BINARY_DIR}/src_generated")

    # Private includes
    include_directories_private("${PROJECT_BINARY_DIR}")

    if(UA_ENABLE_ENCRYPTION_MBEDTLS)
        include_directories_private(${MBEDTLS_INCLUDE_DIRS})
    endif()
    if(UA_ENABLE_ENCRYPTION_OPENSSL OR UA_ENABLE_MQTT_TLS_OPENSSL)
        include_directories_private(${OPENSSL_INCLUDE_DIR})
    endif()
    if(UA_ENABLE_ENCRYPTION_LIBRESSL)
        include_directories_private(${LIBRESSL_INCLUDE_DIR})
    endif()

    # Option-specific includes
    if(UA_ENABLE_DISCOVERY)
        include_directories_private("${PROJECT_SOURCE_DIR}/src/client")
    endif()

endif()

add_dependencies(open62541-object open62541-code-generation)

# Ensure that the open62541::open62541 alias can be used inside open62541's build
add_library(open62541::open62541 ALIAS open62541)

# Export Symbols
target_compile_definitions(open62541-object PRIVATE -DUA_DYNAMIC_LINKING_EXPORT)
target_compile_definitions(open62541 PRIVATE -DUA_DYNAMIC_LINKING_EXPORT)
if(UA_ENABLE_DISCOVERY_MULTICAST)
    target_compile_definitions(open62541-object PRIVATE -DMDNSD_DYNAMIC_LINKING_EXPORT)
    target_compile_definitions(open62541 PRIVATE -DMDNSD_DYNAMIC_LINKING_EXPORT)
endif()
# Generate properly versioned shared library links on Linux
SET_TARGET_PROPERTIES(open62541 PROPERTIES SOVERSION "${OPEN62541_VER_MAJOR}" VERSION "${OPEN62541_VER_MAJOR}.${OPEN62541_VER_MINOR}.${OPEN62541_VER_PATCH}")

##################################
#     Architectures changes      #
##################################

GET_PROPERTY(ua_architecture_add_definitions GLOBAL PROPERTY UA_ARCHITECTURE_ADD_DEFINITIONS)
add_definitions(${ua_architecture_add_definitions})

GET_PROPERTY(ua_architecture_remove_definitions GLOBAL PROPERTY UA_ARCHITECTURE_REMOVE_DEFINITIONS)
if(NOT "${ua_architecture_remove_definitions}" STREQUAL "")
  string(REPLACE " " ";" ua_architecture_remove_definitions_list ${ua_architecture_remove_definitions})
  remove_definitions(${ua_architecture_remove_definitions_list})
  foreach ( flag ${ua_architecture_remove_definitions} )
      string(REPLACE "${flag}" "" CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
  endforeach()
endif(NOT "${ua_architecture_remove_definitions}" STREQUAL "")

GET_PROPERTY(ua_architecture_append_to_library GLOBAL PROPERTY UA_ARCHITECTURE_APPEND_TO_LIBRARY)
list(APPEND open62541_LIBRARIES ${ua_architecture_append_to_library})

target_compile_definitions(open62541 PUBLIC UA_ARCHITECTURE_${UA_ARCHITECTURE_UPPER})

# DLL requires linking to dependencies
target_link_libraries(open62541 ${open62541_LIBRARIES})

##########################
# Build Selected Targets #
##########################

# always include, builds with make doc
add_subdirectory(doc)

if(UA_BUILD_EXAMPLES)
    if(UA_ENABLE_AMALGAMATION)
        # Cannot compile tests with amalgamation. Not prepared for single header include
        message(FATAL_ERROR "Examples cannot be built with source amalgamation enabled")
    endif()
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

if(UA_BUILD_TOOLS)
    add_subdirectory(tools/ua-tool)
    if(UA_ENABLE_JSON_ENCODING)
        add_subdirectory(tools/ua2json)
    endif()
    if(UA_ENABLE_TPM2_KEYSTORE)
        add_subdirectory(tools/tpm_keystore)
    endif()
endif()

########################
# Linting as target    #
########################
include(linting_target)

##########################
# Installation           #
##########################
# invoke via `make install`
# specify install location with `-DCMAKE_INSTALL_PREFIX=xyz`
# Enable shared library with `-DBUILD_SHARED_LIBS=ON`

set(cmake_configfile_install ${CMAKE_INSTALL_LIBDIR}/cmake/open62541)
set(open62541_install_tools_dir share/open62541/tools)
set(open62541_install_nodeset_dir share/open62541/tools/ua-nodeset)

# This list of components allows to define a find_package requirement.
# E.g.:
# find_package(open62541 1.0.0 REQUIRED COMPONENTS Events Methods FullNamespace)
set(open62541_enabled_components "")
if(UA_NAMESPACE_ZERO STREQUAL "FULL")
    list(APPEND open62541_enabled_components "FullNamespace")
endif()
if(UA_ENABLE_METHODCALLS)
    list(APPEND open62541_enabled_components "Methods")
endif()
if(UA_ENABLE_SUBSCRIPTIONS)
    list(APPEND open62541_enabled_components "Subscriptions")
endif()
if(UA_ENABLE_PUBSUB)
    list(APPEND open62541_enabled_components "PubSub")
endif()
list (FIND UA_ENCRYPTION_PLUGINS ${UA_ENABLE_ENCRYPTION} _tmp)
if(${_tmp} GREATER -1)
    list(APPEND open62541_enabled_components "Encryption")
endif()
if(UA_ENABLE_ENCRYPTION_MBEDTLS)
    list(APPEND open62541_enabled_components "EncryptionMbedTLS")
endif()
if(UA_ENABLE_ENCRYPTION_OPENSSL)
    list(APPEND open62541_enabled_components "EncryptionOpenSSL")
endif()
if(UA_ENABLE_ENCRYPTION_LIBRESSL)
    list(APPEND open62541_enabled_components "EncryptionLibreSSL")
endif()
if(UA_ENABLE_AMALGAMATION)
    list(APPEND open62541_enabled_components "Amalgamation")
endif()
if(UA_ENABLE_HISTORIZING)
    list(APPEND open62541_enabled_components "Historizing")
endif()
if(UA_ENABLE_EXPERIMENTAL_HISTORIZING)
    list(APPEND open62541_enabled_components "ExperimentalHistorizing")
endif()
if(UA_ENABLE_SUBSCRIPTIONS_EVENTS)
    list(APPEND open62541_enabled_components "Events")
endif()
if(UA_MULTITHREADING GREATER_EQUAL 100)
    list(APPEND open62541_enabled_components "Multithreading")
endif()
if(UA_ENABLE_DISCOVERY)
    list(APPEND open62541_enabled_components "Discovery")
endif()
if(UA_ENABLE_DISCOVERY_MULTICAST)
    list(APPEND open62541_enabled_components "DiscoveryMulticast")
endif()

# export library (either static or shared depending on BUILD_SHARED_LIBS)
if(UA_PACK_DEBIAN)
    set(EXTRATARGETS "open62541-static")
else()
    set(EXTRATARGETS "")
endif()
install(TARGETS open62541 ${EXTRATARGETS}
        EXPORT open62541Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION include)

if(UA_ENABLE_AMALGAMATION)
# Our default way of installation is the non-amalgamated version.
# See also https://github.com/open62541/open62541/pull/2292#discussion_r241106424
install(CODE "MESSAGE(WARNING \"Installation with UA_ENABLE_AMALGAMATION=ON is not recommended.\")")
endif()

include(CMakePackageConfigHelpers)
configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/tools/cmake/open62541Config.cmake.in"
                              "${CMAKE_CURRENT_BINARY_DIR}/open62541Config.cmake"
                              INSTALL_DESTINATION "${cmake_configfile_install}"
                              PATH_VARS open62541_install_tools_dir
                                        open62541_install_nodeset_dir
                                        open62541_enabled_components
                              )

set(open62541_VERSION)
get_target_property(open62541_VERSION open62541 VERSION)

write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/open62541ConfigVersion.cmake"
                                 VERSION ${open62541_VERSION}
                                 COMPATIBILITY AnyNewerVersion)

install(EXPORT open62541Targets
        FILE open62541Targets.cmake
        DESTINATION "${cmake_configfile_install}"
        NAMESPACE open62541::)

export(
    TARGETS open62541
    NAMESPACE open62541::
    FILE ${CMAKE_CURRENT_BINARY_DIR}/open62541Targets.cmake
)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/tools/cmake/macros_public.cmake" "${CMAKE_CURRENT_BINARY_DIR}/open62541Macros.cmake" COPYONLY)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/open62541Config.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/open62541ConfigVersion.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/open62541Macros.cmake"
        DESTINATION "${cmake_configfile_install}")

if(UA_ENABLE_ENCRYPTION_MBEDTLS)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/tools/cmake/FindMbedTLS.cmake"
            DESTINATION "${cmake_configfile_install}")
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    install(FILES "${PROJECT_BINARY_DIR}/src_generated/open62541.pc"
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()

set(UA_install_tools_dirs "tools/certs"
    "tools/nodeset_compiler"
    "tools/schema"
    ${open62541_NODESET_DIR})

set(UA_install_tools_files "tools/generate_datatypes.py"
    "tools/generate_nodeid_header.py"
    "tools/generate_statuscode_descriptions.py")

install(DIRECTORY ${UA_install_tools_dirs}
    DESTINATION ${open62541_install_tools_dir}
    USE_SOURCE_PERMISSIONS
    FILES_MATCHING
    PATTERN "*"
    PATTERN "*.pyc" EXCLUDE
    )

install(FILES ${UA_install_tools_files} DESTINATION ${open62541_install_tools_dir})

if(UA_PACK_DEBIAN)
    set(UA_install_examples_dirs "examples/"
        "examples/access_control"
        "examples/custom_datatype"
        "examples/discovery"
        "examples/encryption"
        "examples/nodeset"
        "examples/pubsub")

    set(open62541_install_examples_dir share/open62541/examples)
    install(DIRECTORY ${UA_install_examples_dirs} COMPONENT examples DESTINATION ${open62541_install_examples_dir} USE_SOURCE_PERMISSIONS)
endif()

if(NOT UA_ENABLE_AMALGAMATION)
    # Recreate the include folder structure from the source also in /usr/lib/include/open62541

    set(FILES_TO_INSTALL
        ${exported_headers}
        ${default_plugin_headers}
        ${ua_architecture_headers})

    set(BASE_PATH_MAIN "${PROJECT_SOURCE_DIR}/include/open62541")
    set(BASE_PATH_PLUGINS "${PROJECT_SOURCE_DIR}/plugins/include/open62541")
    set(BASE_PATH_ARCH "${PROJECT_SOURCE_DIR}/arch")
    set(BASE_PATH_GENERATED "${PROJECT_BINARY_DIR}/src_generated/open62541")
    set(BASE_PATH_DEPS "${PROJECT_SOURCE_DIR}/deps")

    foreach ( file ${FILES_TO_INSTALL} )
        # Construct a relative path by replacing any occurence of the absolute path
        set(full_path ${file})
        string(REPLACE ${BASE_PATH_MAIN} "" file ${file})
        string(REPLACE ${BASE_PATH_PLUGINS} "" file ${file})
        string(REPLACE ${BASE_PATH_ARCH} "" file ${file})
        string(REPLACE ${BASE_PATH_GENERATED} "" file ${file})
        string(REPLACE ${BASE_PATH_DEPS} "" file ${file})

        get_filename_component(dir ${file} DIRECTORY)

        string(FIND "${full_path}" "${BASE_PATH_DEPS}" has_base_path)
        if("${has_base_path}" EQUAL 0)
            install(FILES ${full_path} DESTINATION include${dir})
        else()
            install(FILES ${full_path} DESTINATION include/open62541${dir})
        endif()
    endforeach()
else()
    # Export amalgamated header open62541.h which is generated due to build of
    # open62541-object
    install(FILES ${PROJECT_BINARY_DIR}/open62541.h DESTINATION include)
endif()

add_subdirectory(tools/packaging)

##################################
# Visual studio solution folders #
##################################

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "_CmakePredifinedTargets")

set_target_properties(open62541 PROPERTIES FOLDER "open62541/lib")
set_target_properties(open62541-object PROPERTIES FOLDER "open62541/lib")
if(UA_ENABLE_AMALGAMATION)
    set_target_properties(open62541-amalgamation-header PROPERTIES FOLDER "open62541/lib")
    set_target_properties(open62541-amalgamation-source PROPERTIES FOLDER "open62541/lib")
endif()

set_target_properties(open62541-generator-namespace PROPERTIES FOLDER "open62541/generators")
set_target_properties(open62541-generator-statuscode PROPERTIES FOLDER "open62541/generators")
set_target_properties(open62541-generator-transport PROPERTIES FOLDER "open62541/generators")
set_target_properties(open62541-generator-types PROPERTIES FOLDER "open62541/generators")
