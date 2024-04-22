cmake_minimum_required(VERSION 3.13)
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
if(${CMAKE_VERSION} VERSION_LESS 3.12)
    set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${PROJECT_SOURCE_DIR}/tools/cmake3.12")
endif()

find_package(Python3 COMPONENTS Interpreter REQUIRED)

find_package(Git)
include(AssignSourceGroup)
include(GNUInstallDirs)
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
set(OPEN62541_VER_MINOR 4)
set(OPEN62541_VER_PATCH 0)
set(OPEN62541_VER_LABEL "-rc2") # like "-rc1" or "-g4538abcd" or "-g4538abcd-dirty"
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

################
# Architecture #
################

set(UA_ARCHITECTURES "none" "posix" "win32")
set(UA_ARCHITECTURE "" CACHE STRING "Architecture to build open62541 for")
SET_PROPERTY(CACHE UA_ARCHITECTURE PROPERTY STRINGS "" ${UA_ARCHITECTURES})

if("${UA_ARCHITECTURE}" STREQUAL "")
    if(UNIX)
        set(UA_ARCHITECTURE "posix" CACHE STRING "" FORCE)
    elseif(WIN32)
        set(UA_ARCHITECTURE "win32" CACHE STRING ""  FORCE)
    endif(UNIX)
endif()

if (NOT UA_ENABLE_AMALGAMATION OR (UA_ENABLE_AMALGAMATION AND NOT UA_AMALGAMATION_MULTIARCH))
    message(STATUS "The selected architecture is: ${UA_ARCHITECTURE}")

    if("${UA_ARCHITECTURE}" STREQUAL "posix")
        set(UA_ARCHITECTURE_POSIX 1)
    elseif("${UA_ARCHITECTURE}" STREQUAL "win32")
        set(UA_ARCHITECTURE_WIN32 1)
    endif()
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
option(UA_AMALGAMATION_MULTIARCH "Create an architecture-independent amalgamation if UA_ENABLE_AMALGAMATION is ON" OFF)
option(BUILD_SHARED_LIBS "Enable building of shared libraries (dll/so)" OFF)
set(UA_LOGLEVEL 100 CACHE STRING "Minimal level for logs (in addition to the log plugin settings) (100=TRACE, 200=DEBUG, 300=INFO, 400=WARNING, 500=ERROR, 600=FATAL)")
option(UA_ENABLE_DIAGNOSTICS "Enable diagnostics information exposed by the server" ON)
option(UA_ENABLE_METHODCALLS "Enable the Method service set" ON)
option(UA_ENABLE_SUBSCRIPTIONS "Enable subscriptions support" ON)
option(UA_ENABLE_SUBSCRIPTIONS_EVENTS "Enable event monitoring" ON)
option(UA_ENABLE_DA "Enable OPC UA DataAccess (Part 8) definitions" ON)
option(UA_ENABLE_HISTORIZING "Enable basic support for historical access (client and server)" ON)
option(UA_ENABLE_DISCOVERY "Enable Discovery Service (LDS)" ON)
option(UA_ENABLE_JSON_ENCODING "Enable JSON encoding" ON)
option(UA_ENABLE_XML_ENCODING "Enable XML encoding (EXPERIMENTAL)" OFF)
option(UA_ENABLE_NODESETLOADER "Enable nodesetLoader public API" OFF)

if(UA_INFORMATION_MODEL_AUTOLOAD AND NOT UA_BUILD_FUZZING)
    set(UA_ENABLE_NODESET_INJECTOR ON)
endif()

set(MULTITHREADING_DEFAULT 0)
if(WIN32 OR UNIX)
  # Enable multithreading by default on Windows and POSIX-like (Unix) systems
  set(MULTITHREADING_DEFAULT 100)
endif()
set(UA_MULTITHREADING ${MULTITHREADING_DEFAULT} CACHE STRING
    "Multithreading support (<100: No multithreading support, >=100: Thread-safety enabled (internal mutexes)")

option(UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS "Enable the use of A&C. (EXPERIMENTAL)" OFF)
mark_as_advanced(UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS)

option(UA_ENABLE_NODEMANAGEMENT "Enable dynamic addition and removal of nodes at runtime" ON)
mark_as_advanced(UA_ENABLE_NODEMANAGEMENT)

option(UA_ENABLE_PARSING "Utility functions that require parsing (e.g. NodeId expressions)" ON)
mark_as_advanced(UA_ENABLE_PARSING)

option(UA_ENABLE_INLINABLE_EXPORT "Export 'static inline' methods as regular API" OFF)
mark_as_advanced(UA_ENABLE_INLINABLE_EXPORT)

option(UA_ENABLE_DISCOVERY_MULTICAST "Enable Discovery Service with multicast support (LDS-ME)" OFF)
mark_as_advanced(UA_ENABLE_DISCOVERY_MULTICAST)

# security provider
set(UA_ENCRYPTION_PLUGINS "MBEDTLS" "OPENSSL" "LIBRESSL")
set(UA_ENABLE_ENCRYPTION "OFF" CACHE STRING "Encryption support (LibreSSL EXPERIMENTAL)")
SET_PROPERTY(CACHE UA_ENABLE_ENCRYPTION PROPERTY STRINGS "OFF" ${UA_ENCRYPTION_PLUGINS})
option(UA_ENABLE_ENCRYPTION_OPENSSL "Deprecated: Enable encryption support (uses openssl)" OFF)
mark_as_advanced(UA_ENABLE_ENCRYPTION_OPENSSL)
option(UA_ENABLE_ENCRYPTION_MBEDTLS "Deprecated: Enable encryption support (uses mbedTLS)" OFF)
mark_as_advanced(UA_ENABLE_ENCRYPTION_MBEDTLS)

option(UA_ENABLE_CERT_REJECTED_DIR  "Enable specifying directory for rejected certificates (with mbedTLS)" OFF)
mark_as_advanced(UA_ENABLE_CERT_REJECTED_DIR)

list (FIND UA_ENCRYPTION_PLUGINS ${UA_ENABLE_ENCRYPTION} _tmp)
if(UA_ENABLE_ENCRYPTION STREQUAL "OFF" OR ${_tmp} GREATER -1)
    set(UA_ENABLE_ENCRYPTION_OPENSSL OFF)
    set(UA_ENABLE_ENCRYPTION_MBEDTLS OFF)
    set(UA_ENABLE_ENCRYPTION_LIBRESSL OFF)
    if(UA_ENABLE_ENCRYPTION STREQUAL "MBEDTLS")
        set(UA_ENABLE_ENCRYPTION_MBEDTLS ON)
    elseif(UA_ENABLE_ENCRYPTION STREQUAL "OPENSSL")
        set(UA_ENABLE_ENCRYPTION_OPENSSL ON)
        set(UA_ENABLE_CERT_REJECTED_DIR OFF)
    elseif(UA_ENABLE_ENCRYPTION STREQUAL "LIBRESSL")
        set(UA_ENABLE_ENCRYPTION_LIBRESSL ON)
        set(UA_ENABLE_CERT_REJECTED_DIR OFF)
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
        set(UA_ENABLE_CERT_REJECTED_DIR OFF)
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
set(UA_NAMESPACE_ZERO "REDUCED" CACHE STRING "Completeness of the generated namespace zero (minimal/reduced/full)")
SET_PROPERTY(CACHE UA_NAMESPACE_ZERO PROPERTY STRINGS "MINIMAL" "REDUCED" "FULL")
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
  # We are using the scripts provided at for coverage testing: https://github.com/RWTH-HPC/CMake-codecov
  set(ENABLE_COVERAGE ON)
  find_package(codecov REQUIRED)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage -lgcov")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
endif()

if(UA_ENABLE_CLANG_COV)
    if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
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

option(UA_FORCE_32BIT "Force compilation as 32-bit executable" OFF)
mark_as_advanced(UA_FORCE_32BIT)

option(UA_FORCE_WERROR "Force compilation with -Werror (or /WX on MSVC)" OFF)

option(UA_ENABLE_DEBUG_SANITIZER "Use sanitizer in debug mode" ON)
mark_as_advanced(UA_ENABLE_DEBUG_SANITIZER)

#General PubSub setup
option(UA_ENABLE_PUBSUB "Enable the PubSub protocol" OFF)

option(UA_ENABLE_PUBSUB_ENCRYPTION "Enable encryption of the PubSub payload" OFF)
mark_as_advanced(UA_ENABLE_PUBSUB_ENCRYPTION)

option(UA_ENABLE_PUBSUB_SKS "Enable Security Key Service support for publisher and subscriber" OFF)
mark_as_advanced(UA_ENABLE_PUBSUB_SKS)
if(UA_ENABLE_PUBSUB_SKS)
    message(WARNING "The PubSub SKS feature is under development and not yet ready.")
    if(NOT UA_ENABLE_PUBSUB_ENCRYPTION)
        message(FATAL_ERROR "PubSub SKS cannot be used with disabled UA_ENABLE_PUBSUB_ENCRYPTION")
    endif()
    if(NOT UA_NAMESPACE_ZERO STREQUAL "FULL")
        message(FATAL_ERROR "PubSub SKS needs the full Namespace Zero")
    endif()
endif()

option(UA_ENABLE_PUBSUB_INFORMATIONMODEL "Enable PubSub information model twin" OFF)
if(UA_ENABLE_PUBSUB_INFORMATIONMODEL)
    if(NOT UA_ENABLE_PUBSUB)
        message(FATAL_ERROR "PubSub information model representation cannot be used with disabled PubSub function.")
    endif()
    if(UA_NAMESPACE_ZERO STREQUAL "MINIMAL")
        message(FATAL_ERROR "PubSub information model representation cannot be used with MINIMAL namespace zero.")
    endif()
endif()

option(UA_ENABLE_PUBSUB_FILE_CONFIG "Enable loading PubSub Config from file extension" OFF)
mark_as_advanced(UA_ENABLE_PUBSUB_FILE_CONFIG)
if(UA_ENABLE_PUBSUB_FILE_CONFIG)
    if(NOT UA_ENABLE_PUBSUB)
        message(FATAL_ERROR "For UA_ENABLE_PUBSUB_FILE_CONFIG PubSub needs to be enabled")
    endif()
endif()

option(UA_ENABLE_PUBSUB_MONITORING "Enable monitoring of PubSub components (e.g. MessageReceiveTimeout)" OFF)
mark_as_advanced(UA_ENABLE_PUBSUB_MONITORING)
if(UA_ENABLE_PUBSUB_MONITORING)
    if(NOT UA_ENABLE_PUBSUB)
        message(FATAL_ERROR "PubSub monitoring cannot be used with PubSub function disabled")
    endif()
endif()

option(UA_ENABLE_PUBSUB_BUFMALLOC "Enable allocation with static memory buffer for time critical PubSub parts" OFF)
mark_as_advanced(UA_ENABLE_PUBSUB_BUFMALLOC)
if(UA_ENABLE_PUBSUB_BUFMALLOC)
    if(NOT UA_ENABLE_PUBSUB)
        message(FATAL_ERROR "PubSub buffer allocation cannot be used with PubSub function disabled")
    endif()
    if(NOT UA_ENABLE_MALLOC_SINGLETON)
        message(FATAL_ERROR "PubSub buffer allocation cannot be used without 'UA_ENABLE_MALLOC_SINGLETON' enabled")
    endif()
    if(UA_ENABLE_PUBSUB_MONITORING)
        message(WARNING "PubSub monitoring option 'UA_ENABLE_PUBSUB_MONITORING' can only be used with option 'UA_ENABLE_PUBSUB_BUFMALLOC' if "
            "a custom monitoring interface is provided, which does not allocate/deallocate memory at start/stopMonitoring callback. "
            "It can't be used with the built-in ua_timer implementation, because this leads to memory deallocation problems.")
    endif()
endif()

option(UA_ENABLE_MQTT "Enable MQTT connections for the EventLoop" OFF)
mark_as_advanced(UA_ENABLE_MQTT)
if(UA_ENABLE_MQTT)
    if(NOT EXISTS "${UA_FILE_MQTT}")
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}")
        endif()
    endif()
    include_directories("${PROJECT_SOURCE_DIR}/deps/mqtt-c/include")
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

if(UA_ENABLE_HISTORIZING AND (NOT UA_ENABLE_SUBSCRIPTIONS))
    MESSAGE(FATAL_ERROR "UA_ENABLE_HISTORIZING needs Subscriptions enabled")
endif()

######################
# External Libraries #
######################

set(open62541_LIBRARIES "")
if("${UA_ARCHITECTURE}" STREQUAL "posix")
    list(APPEND open62541_LIBRARIES "m")
    if(UA_MULTITHREADING OR UA_BUILD_UNIT_TESTS)
        list(APPEND open62541_LIBRARIES "pthread")
    endif()
    if(NOT APPLE AND (NOT ${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD"))
        list(APPEND open62541_LIBRARIES "rt")
    endif()
elseif("${UA_ARCHITECTURE}" STREQUAL "win32")
    list(APPEND open62541_LIBRARIES "ws2_32")
    list(APPEND open62541_LIBRARIES "iphlpapi")
endif()

if(UA_ENABLE_ENCRYPTION_OPENSSL)
    # use the OpenSSL encryption library
    # https://cmake.org/cmake/help/v3.0/module/FindOpenSSL.html
    find_package(OpenSSL REQUIRED)
    list(APPEND open62541_LIBRARIES OpenSSL::Crypto OpenSSL::SSL)
endif()

if(UA_ENABLE_ENCRYPTION_LIBRESSL)
    # See https://github.com/libressl-portable/portable/blob/master/FindLibreSSL.cmake
    find_package(LibreSSL REQUIRED)
    list(APPEND open62541_LIBRARIES ${LIBRESSL_LIBRARIES})
    if(WIN32)
        # Add bestcrypt for random generator and ws2_32 for crypto
        list(APPEND open62541_LIBRARIES ws2_32 bcrypt)
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
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CC_FLAG}" CACHE INTERNAL "C Compiler Flags")
        endif()
        unset(flag_supported CACHE)
    endif()
endfunction()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    check_add_cc_flag("-std=c99")   # C99 mode
    check_add_cc_flag("-pipe")      # Avoid writing temporary files (for compiler speed)
    check_add_cc_flag("-Wall")      # Warnings
    check_add_cc_flag("-Wextra")    # More warnings
    check_add_cc_flag("-Wpedantic") # Standard compliance
    if(UA_FORCE_WERROR)
        check_add_cc_flag("-Werror") # All warnings are errors
        check_add_cc_flag("-Wunused-command-line-argument") # Warning for command line args instead of error
    endif()

    check_add_cc_flag("-Wno-static-in-inline") # Clang doesn't like the use of static inline methods inside static inline methods
    check_add_cc_flag("-Wno-overlength-strings") # May happen in the nodeset compiler when complex values are directly encoded
    check_add_cc_flag("-Wno-unused-parameter") # some methods may require unused arguments to cast to a method pointer
    check_add_cc_flag("-Wno-maybe-uninitialized") # too many false positives

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

    if(UA_MULTITHREADING GREATER_EQUAL 100 AND NOT WIN32)
        check_add_cc_flag("-pthread")
    endif()

    # Force 32bit build
    if(UA_FORCE_32BIT)
        if(MSVC)
            message(FATAL_ERROR "Select the 32bit (cross-) compiler instead of forcing compiler options")
        endif()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32") # GCC and Clang, possibly more
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

    # Linker
    set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "") # cmake sets -rdynamic by default

    # Debug
    if(UA_ENABLE_DEBUG_SANITIZER AND BUILD_TYPE_LOWER_CASE STREQUAL "debug" AND UNIX AND NOT UA_BUILD_OSS_FUZZ AND
       CMAKE_C_COMPILER_ID STREQUAL "Clang" AND NOT UA_ENABLE_UNIT_TESTS_MEMCHECK)
        # Add default sanitizer settings when using clang and Debug build.
        # This allows e.g. CLion to find memory locations for SegFaults
        message(STATUS "Sanitizer enabled")
        set(SANITIZER_FLAGS "-g -fno-omit-frame-pointer -gline-tables-only -fsanitize=address -fsanitize-address-use-after-scope -fsanitize=leak -fsanitize=undefined")
        if(CMAKE_CXX_COMPILER_VERSION AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10.0)
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
if(UA_ENABLE_AMALGAMATION)
    set(PC_EXTRA_CFLAGS "-DUA_ENABLE_AMALGAMATION")
endif()
configure_file(tools/open62541.pc.in ${PROJECT_BINARY_DIR}/src_generated/open62541.pc @ONLY)

if(UA_ENABLE_DISCOVERY_MULTICAST)
  include(GenerateExportHeader)
    set(MDNSD_LOGLEVEL 300 CACHE STRING "Level at which logs shall be reported" FORCE)
    
    # create a "fake" empty library to generate the export header macros
    if (APPLE)
    	add_library(libmdnsd INTERFACE ${PROJECT_SOURCE_DIR}/deps/mdnsd/libmdnsd/mdnsd.h)
    else()
    	add_library(libmdnsd ${PROJECT_SOURCE_DIR}/deps/mdnsd/libmdnsd/mdnsd.h)
    endif()
    
    set_property(TARGET libmdnsd PROPERTY LINKER_LANGUAGE C)
    set_property(TARGET libmdnsd PROPERTY DEFINE_SYMBOL "MDNSD_DYNAMIC_LINKING_EXPORT")
    configure_file("deps/mdnsd/libmdnsd/mdnsd_config_extra.in"
                   "${PROJECT_BINARY_DIR}/src_generated/mdnsd_config_extra")
    file(READ "${PROJECT_BINARY_DIR}/src_generated/mdnsd_config_extra" MDNSD_CONFIG_EXTRA)
    generate_export_header(libmdnsd EXPORT_FILE_NAME "${PROJECT_BINARY_DIR}/src_generated/mdnsd_config.h"
                           BASE_NAME MDNSD DEFINE_NO_DEPRECATED CUSTOM_CONTENT_FROM_VARIABLE MDNSD_CONFIG_EXTRA)
endif()

# Exported headers
set(exported_headers ${PROJECT_BINARY_DIR}/src_generated/open62541/config.h
                     ${PROJECT_BINARY_DIR}/src_generated/open62541/statuscodes.h
                     ${PROJECT_BINARY_DIR}/src_generated/open62541/nodeids.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/common.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/types.h
                     ${PROJECT_BINARY_DIR}/src_generated/open62541/types_generated.h
                     ${PROJECT_BINARY_DIR}/src_generated/open62541/types_generated_handling.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/util.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/plugin/log.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/plugin/accesscontrol.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/plugin/certificategroup.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/plugin/securitypolicy.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/plugin/eventloop.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/plugin/nodestore.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/plugin/historydatabase.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/server_pubsub.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/pubsub.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/client.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/server.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/client_highlevel.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/client_subscriptions.h
                     ${PROJECT_SOURCE_DIR}/include/open62541/client_highlevel_async.h)

# Main Library

set(lib_headers ${PROJECT_SOURCE_DIR}/deps/open62541_queue.h
                ${PROJECT_SOURCE_DIR}/deps/pcg_basic.h
                ${PROJECT_SOURCE_DIR}/deps/libc_time.h
                ${PROJECT_SOURCE_DIR}/deps/base64.h
                ${PROJECT_SOURCE_DIR}/deps/dtoa.h
                ${PROJECT_SOURCE_DIR}/deps/mp_printf.h
                ${PROJECT_SOURCE_DIR}/deps/itoa.h
                ${PROJECT_SOURCE_DIR}/deps/ziptree.h
                ${PROJECT_SOURCE_DIR}/src/ua_types_encoding_binary.h
                ${PROJECT_SOURCE_DIR}/src/util/ua_util_internal.h
                ${PROJECT_BINARY_DIR}/src_generated/open62541/transport_generated.h
                ${PROJECT_BINARY_DIR}/src_generated/open62541/transport_generated_handling.h
                ${PROJECT_SOURCE_DIR}/src/ua_securechannel.h
                ${PROJECT_SOURCE_DIR}/src/server/ua_session.h
                ${PROJECT_SOURCE_DIR}/src/server/ua_subscription.h
                ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_networkmessage.h
                ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub.h
                ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_ns0.h
                ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_keystorage.h
                ${PROJECT_SOURCE_DIR}/src/server/ua_services.h
                ${PROJECT_SOURCE_DIR}/src/server/ua_server_async.h
                ${PROJECT_SOURCE_DIR}/src/server/ua_server_internal.h
                ${PROJECT_SOURCE_DIR}/src/client/ua_client_internal.h)

set(lib_sources ${PROJECT_SOURCE_DIR}/src/ua_types.c
                ${PROJECT_SOURCE_DIR}/src/ua_types_encoding_binary.c
                ${PROJECT_BINARY_DIR}/src_generated/open62541/types_generated.c
                ${PROJECT_BINARY_DIR}/src_generated/open62541/transport_generated.c
                ${PROJECT_BINARY_DIR}/src_generated/open62541/statuscodes.c
                ${PROJECT_SOURCE_DIR}/src/util/ua_util.c
                ${PROJECT_SOURCE_DIR}/src/ua_securechannel.c
                ${PROJECT_SOURCE_DIR}/src/ua_securechannel_crypto.c
                # server
                ${PROJECT_SOURCE_DIR}/src/server/ua_session.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_nodes.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_server.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_server_ns0.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_server_ns0_diagnostics.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_server_config.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_server_binary.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_server_utils.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_server_async.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_view.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_method.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_session.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_attribute.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_discovery.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_subscription.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_monitoreditem.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_securechannel.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_services_nodemanagement.c
                # pubsub
                ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_networkmessage.c
                ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_eventloop.c
                ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_connection.c
                ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_dataset.c
                ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_writer.c
                ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_writergroup.c
                ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_reader.c
                ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_readergroup.c
                ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_manager.c
                ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_ns0.c
                ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_keystorage.c
                ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_securitygroup.c
                ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_config.c
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
                ${PROJECT_SOURCE_DIR}/deps/dtoa.c
                ${PROJECT_SOURCE_DIR}/deps/mp_printf.c
                ${PROJECT_SOURCE_DIR}/deps/itoa.c
                ${PROJECT_SOURCE_DIR}/deps/ziptree.c)

if(UA_GENERATED_NAMESPACE_ZERO)
    list(APPEND lib_headers ${PROJECT_BINARY_DIR}/src_generated/open62541/namespace0_generated.h)
    list(APPEND lib_sources ${PROJECT_BINARY_DIR}/src_generated/open62541/namespace0_generated.c)
endif()

if(UA_ENABLE_PARSING)
    list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/src/util/ua_types_lex.c)
    if(UA_ENABLE_SUBSCRIPTIONS_EVENTS)
        list(APPEND lib_headers ${PROJECT_SOURCE_DIR}/src/util/ua_eventfilter_parser.h)
        list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/src/util/ua_eventfilter_parser.c
                                ${PROJECT_SOURCE_DIR}/src/util/ua_eventfilter_grammar.h
                                ${PROJECT_SOURCE_DIR}/src/util/ua_eventfilter_lex.c
                                ${PROJECT_SOURCE_DIR}/src/util/ua_eventfilter_grammar.c)
    endif()
endif()

if(UA_ENABLE_PUBSUB)
    if(UA_ENABLE_MALLOC_SINGLETON AND UA_ENABLE_PUBSUB_BUFMALLOC)
        list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_bufmalloc.c)
        list(APPEND lib_headers ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_bufmalloc.h)
    endif()
    if(UA_ENABLE_PUBSUB_ENCRYPTION)
        list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_security.c)
    endif()
endif()

if(UA_ENABLE_JSON_ENCODING)
    list(APPEND lib_headers ${PROJECT_SOURCE_DIR}/deps/cj5.h
                            ${PROJECT_SOURCE_DIR}/deps/parse_num.h
                            ${PROJECT_SOURCE_DIR}/src/ua_types_encoding_json.h)
    list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/deps/cj5.c
                            ${PROJECT_SOURCE_DIR}/deps/parse_num.c
                            ${PROJECT_SOURCE_DIR}/src/ua_types_encoding_json.c)
    if(UA_ENABLE_PUBSUB)
        list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/src/pubsub/ua_pubsub_networkmessage_json.c)
    endif()
endif()

if(UA_ENABLE_XML_ENCODING)
    if(NOT UA_ENABLE_JSON_ENCODING)
        list(APPEND lib_headers ${PROJECT_SOURCE_DIR}/deps/parse_num.h)
        list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/deps/parse_num.c)
    endif()
    list(APPEND lib_headers ${PROJECT_SOURCE_DIR}/src/ua_types_encoding_xml.h)
    list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/src/ua_types_encoding_xml.c)
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
    list(APPEND lib_headers
         ${PROJECT_BINARY_DIR}/src_generated/mdnsd_config.h
         ${PROJECT_SOURCE_DIR}/deps/mdnsd/libmdnsd/1035.h
         ${PROJECT_SOURCE_DIR}/deps/mdnsd/libmdnsd/xht.h
         ${PROJECT_SOURCE_DIR}/deps/mdnsd/libmdnsd/sdtxt.h
         ${PROJECT_SOURCE_DIR}/deps/mdnsd/libmdnsd/mdnsd.h)
    list(APPEND lib_sources
         ${PROJECT_SOURCE_DIR}/src/server/ua_discovery_mdns.c
         ${PROJECT_SOURCE_DIR}/deps/mdnsd/libmdnsd/1035.c
         ${PROJECT_SOURCE_DIR}/deps/mdnsd/libmdnsd/xht.c
         ${PROJECT_SOURCE_DIR}/deps/mdnsd/libmdnsd/sdtxt.c
         ${PROJECT_SOURCE_DIR}/deps/mdnsd/libmdnsd/mdnsd.c)
endif()

if(UA_BUILD_FUZZING OR UA_BUILD_OSS_FUZZ)
    list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/tests/fuzz/custom_memory_manager.c)
endif()


# Plugins
set(plugin_headers ${PROJECT_SOURCE_DIR}/plugins/include/open62541/plugin/accesscontrol_default.h
    ${PROJECT_SOURCE_DIR}/plugins/include/open62541/plugin/certificategroup_default.h
                   ${PROJECT_SOURCE_DIR}/plugins/include/open62541/plugin/log_stdout.h
                   ${PROJECT_SOURCE_DIR}/plugins/include/open62541/plugin/nodestore_default.h
                   ${PROJECT_SOURCE_DIR}/plugins/include/open62541/server_config_default.h
                   ${PROJECT_SOURCE_DIR}/plugins/include/open62541/client_config_default.h
                   ${PROJECT_SOURCE_DIR}/plugins/include/open62541/plugin/securitypolicy_default.h)

set(plugin_sources ${PROJECT_SOURCE_DIR}/plugins/ua_log_stdout.c
                   ${PROJECT_SOURCE_DIR}/plugins/ua_accesscontrol_default.c
                   ${PROJECT_SOURCE_DIR}/plugins/ua_nodestore_ziptree.c
                   ${PROJECT_SOURCE_DIR}/plugins/ua_nodestore_hashmap.c
                   ${PROJECT_SOURCE_DIR}/plugins/ua_config_default.c
    ${PROJECT_SOURCE_DIR}/plugins/crypto/ua_certificategroup_none.c
                   ${PROJECT_SOURCE_DIR}/plugins/crypto/ua_securitypolicy_none.c)

# Always add the POSIX eventloop. The files internally check for the configured architecture
list(APPEND plugin_sources
     ${PROJECT_SOURCE_DIR}/arch/clock.c
     ${PROJECT_SOURCE_DIR}/arch/eventloop_common/timer.h
     ${PROJECT_SOURCE_DIR}/arch/eventloop_common/timer.c
     ${PROJECT_SOURCE_DIR}/arch/eventloop_common/eventloop_common.h
     ${PROJECT_SOURCE_DIR}/arch/eventloop_common/eventloop_common.c
     ${PROJECT_SOURCE_DIR}/arch/eventloop_posix/eventloop_posix.h
     ${PROJECT_SOURCE_DIR}/arch/eventloop_posix/eventloop_posix.c
     ${PROJECT_SOURCE_DIR}/arch/eventloop_posix/eventloop_posix_tcp.c
     ${PROJECT_SOURCE_DIR}/arch/eventloop_posix/eventloop_posix_udp.c
     ${PROJECT_SOURCE_DIR}/arch/eventloop_posix/eventloop_posix_eth.c
     ${PROJECT_SOURCE_DIR}/arch/eventloop_posix/eventloop_posix_interrupt.c
     ${PROJECT_SOURCE_DIR}/arch/eventloop_common/eventloop_mqtt.c)

# For file based server configuration
if(UA_ENABLE_JSON_ENCODING)
    list(APPEND plugin_headers ${PROJECT_SOURCE_DIR}/plugins/include/open62541/server_config_file_based.h)
    list(APPEND plugin_sources ${PROJECT_SOURCE_DIR}/plugins/ua_config_json.c)
endif()

if(UA_ENABLE_HISTORIZING)
    list(APPEND plugin_headers
         ${PROJECT_SOURCE_DIR}/plugins/include/open62541/plugin/historydata/history_data_backend.h
         ${PROJECT_SOURCE_DIR}/plugins/include/open62541/plugin/historydata/history_data_gathering.h
         ${PROJECT_SOURCE_DIR}/plugins/include/open62541/plugin/historydata/history_database_default.h
         ${PROJECT_SOURCE_DIR}/plugins/include/open62541/plugin/historydata/history_data_gathering_default.h
         ${PROJECT_SOURCE_DIR}/plugins/include/open62541/plugin/historydata/history_data_backend_memory.h)
    list(APPEND plugin_sources
         ${PROJECT_SOURCE_DIR}/plugins/historydata/ua_history_data_backend_memory.c
         ${PROJECT_SOURCE_DIR}/plugins/historydata/ua_history_data_gathering_default.c
         ${PROJECT_SOURCE_DIR}/plugins/historydata/ua_history_database_default.c)
endif()

# Syslog-logging on Linux and Unices
if(UNIX)
    list(APPEND plugin_headers ${PROJECT_SOURCE_DIR}/plugins/include/open62541/plugin/log_syslog.h)
    list(APPEND plugin_sources ${PROJECT_SOURCE_DIR}/plugins/ua_log_syslog.c)
endif()

# Always include encryption plugins into the amalgamation
# Use guards in the files to ensure that UA_ENABLE_ENCRYPTON_MBEDTLS and UA_ENABLE_ENCRYPTION_OPENSSL are honored.

if(UA_ENABLE_ENCRYPTION_MBEDTLS OR UA_ENABLE_PUBSUB_ENCRYPTION OR UA_ENABLE_AMALGAMATION)
  list(INSERT plugin_sources 0
       ${PROJECT_SOURCE_DIR}/plugins/crypto/mbedtls/securitypolicy_mbedtls_common.h
       ${PROJECT_SOURCE_DIR}/plugins/crypto/mbedtls/securitypolicy_mbedtls_common.c)
endif()

if(UA_ENABLE_PUBSUB_ENCRYPTION)
  list(APPEND plugin_sources
       ${PROJECT_SOURCE_DIR}/plugins/crypto/mbedtls/securitypolicy_pubsub_aes128ctr.c
       ${PROJECT_SOURCE_DIR}/plugins/crypto/mbedtls/securitypolicy_pubsub_aes256ctr.c)
endif()

if(UA_ENABLE_ENCRYPTION_MBEDTLS OR UA_ENABLE_ENCRYPTION_OPENSSL OR UA_ENABLE_ENCRYPTION_LIBRESSL OR UA_ENABLE_AMALGAMATION)
    list(APPEND plugin_headers ${PROJECT_SOURCE_DIR}/plugins/include/open62541/plugin/create_certificate.h)
endif()

if(UA_ENABLE_ENCRYPTION_MBEDTLS OR UA_ENABLE_AMALGAMATION)
list(APPEND plugin_sources
     ${PROJECT_SOURCE_DIR}/plugins/crypto/mbedtls/ua_securitypolicy_basic128rsa15.c
     ${PROJECT_SOURCE_DIR}/plugins/crypto/mbedtls/ua_securitypolicy_basic256.c
     ${PROJECT_SOURCE_DIR}/plugins/crypto/mbedtls/ua_securitypolicy_basic256sha256.c
     ${PROJECT_SOURCE_DIR}/plugins/crypto/mbedtls/ua_securitypolicy_aes128sha256rsaoaep.c
     ${PROJECT_SOURCE_DIR}/plugins/crypto/mbedtls/ua_securitypolicy_aes256sha256rsapss.c
     ${PROJECT_SOURCE_DIR}/plugins/crypto/mbedtls/ua_mbedtls_create_certificate.c
     ${PROJECT_SOURCE_DIR}/plugins/crypto/mbedtls/ua_certificategroup_mbedtls.c)
endif()

if(UA_ENABLE_TPM2_SECURITY)
list(INSERT plugin_sources 0
     ${PROJECT_SOURCE_DIR}/plugins/crypto/pkcs11/securitypolicy_pubsub_aes128ctr_tpm.c
     ${PROJECT_SOURCE_DIR}/plugins/crypto/pkcs11/securitypolicy_pubsub_aes256ctr_tpm.c)
endif()

if(UA_ENABLE_ENCRYPTION_OPENSSL OR UA_ENABLE_ENCRYPTION_LIBRESSL OR UA_ENABLE_AMALGAMATION)
list(APPEND plugin_sources
     ${PROJECT_SOURCE_DIR}/plugins/crypto/openssl/securitypolicy_openssl_common.h
     ${PROJECT_SOURCE_DIR}/plugins/crypto/openssl/ua_openssl_version_abstraction.h
     ${PROJECT_SOURCE_DIR}/plugins/crypto/openssl/securitypolicy_openssl_common.c
     ${PROJECT_SOURCE_DIR}/plugins/crypto/openssl/ua_openssl_basic128rsa15.c
     ${PROJECT_SOURCE_DIR}/plugins/crypto/openssl/ua_openssl_basic256.c
     ${PROJECT_SOURCE_DIR}/plugins/crypto/openssl/ua_openssl_basic256sha256.c
     ${PROJECT_SOURCE_DIR}/plugins/crypto/openssl/ua_openssl_aes128sha256rsaoaep.c
     ${PROJECT_SOURCE_DIR}/plugins/crypto/openssl/ua_openssl_aes256sha256rsapss.c
     ${PROJECT_SOURCE_DIR}/plugins/crypto/openssl/ua_openssl_create_certificate.c
     ${PROJECT_SOURCE_DIR}/plugins/crypto/openssl/ua_certificategroup_openssl.c)
endif()

if(UA_ENABLE_DISCOVERY)
    list(APPEND lib_headers ${PROJECT_SOURCE_DIR}/src/server/ua_discovery.h)
    list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/src/server/ua_discovery.c)
endif()

if(UA_ENABLE_NODESETLOADER)
    # This is required because of issues with policy CMP0077.
    # Source: https://stackoverflow.com/a/54404924
    option(ENABLE_BUILD_INTO_OPEN62541 "make nodesetLoader part of the open62541 library" off)
    set(ENABLE_BUILD_INTO_OPEN62541 on)

    add_subdirectory(${PROJECT_SOURCE_DIR}/deps/nodesetLoader)

    list(APPEND lib_sources ${NODESETLOADER_SOURCES})
    list(APPEND plugin_headers ${PROJECT_SOURCE_DIR}/plugins/include/open62541/plugin/nodesetloader.h)
    list(APPEND plugin_sources ${PROJECT_SOURCE_DIR}/plugins/ua_nodesetloader.c)
    list(APPEND open62541_LIBRARIES ${NODESETLOADER_DEPS_LIBS})

    if(UA_ENABLE_AMALGAMATION)
        list(APPEND exported_headers ${NODESETLOADER_PUBLIC_HEADERS})
        list(APPEND lib_headers ${NODESETLOADER_PRIVATE_HEADERS})
    endif()
endif()

#########################
# Generate source files #
#########################

# List of nodeset files combined into NS0 generated file
set(UA_FILE_NODESETS)
set(UA_NODESET_DIR ${PROJECT_SOURCE_DIR}/deps/ua-nodeset CACHE STRING "The path to the nodetset directory.")

if(UA_NAMESPACE_ZERO STREQUAL "FULL")
    if(NOT UA_FILE_NS0)
        set(UA_FILE_NS0 ${UA_NODESET_DIR}/Schema/Opc.Ua.NodeSet2.xml)
    endif()
    set(UA_FILE_NODESETS "${UA_FILE_NS0}")

    if(NOT EXISTS "${UA_FILE_NS0}")
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}")
        endif()
    endif()

    set(UA_FILE_NODEIDS ${UA_NODESET_DIR}/Schema/NodeIds.csv)
    set(UA_FILE_STATUSCODES ${UA_NODESET_DIR}/Schema/StatusCode.csv)
    set(UA_FILE_TYPES_BSD ${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd)
else()
    if(NOT UA_FILE_NS0)
        set(UA_FILE_NS0 ${PROJECT_SOURCE_DIR}/tools/schema/Opc.Ua.NodeSet2.Reduced.xml)
    endif()
    set(UA_FILE_NODESETS "${UA_FILE_NS0}")

    set(UA_FILE_DATATYPES ${PROJECT_SOURCE_DIR}/tools/schema/datatypes_minimal.txt)
    set(UA_FILE_NODEIDS ${PROJECT_SOURCE_DIR}/tools/schema/NodeIds.csv)
    set(UA_FILE_STATUSCODES ${PROJECT_SOURCE_DIR}/tools/schema/StatusCode.csv)
    set(UA_FILE_TYPES_BSD ${PROJECT_SOURCE_DIR}/tools/schema/Opc.Ua.Types.bsd)

    # always enable  subscriptions, method, discovery and historizing datatypes (for the client)
    list(APPEND UA_FILE_DATATYPES ${PROJECT_SOURCE_DIR}/tools/schema/datatypes_subscriptions.txt)
    list(APPEND UA_FILE_DATATYPES ${PROJECT_SOURCE_DIR}/tools/schema/datatypes_method.txt)
    list(APPEND UA_FILE_DATATYPES ${PROJECT_SOURCE_DIR}/tools/schema/datatypes_discovery.txt)
    list(APPEND UA_FILE_DATATYPES ${PROJECT_SOURCE_DIR}/tools/schema/datatypes_historizing.txt)

    if(UA_ENABLE_DIAGNOSTICS)
        list(APPEND UA_FILE_DATATYPES ${PROJECT_SOURCE_DIR}/tools/schema/datatypes_diagnostics.txt)
        list(APPEND UA_FILE_NODESETS ${PROJECT_SOURCE_DIR}/tools/schema/Opc.Ua.NodeSet2.DiagnosticsMinimal.xml)
    endif()

    if(UA_ENABLE_SUBSCRIPTIONS_EVENTS)
        list(APPEND UA_FILE_NODESETS ${PROJECT_SOURCE_DIR}/tools/schema/Opc.Ua.NodeSet2.EventsMinimal.xml)
    endif()

    if(UA_ENABLE_HISTORIZING)
        list(APPEND UA_FILE_NODESETS ${PROJECT_SOURCE_DIR}/tools/schema/Opc.Ua.NodeSet2.HistorizingMinimal.xml)
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
ua_generate_datatypes(BUILTIN GEN_DOC NAME "types" TARGET_SUFFIX "types" NAMESPACE_IDX 0
                      FILE_CSV "${UA_FILE_NODEIDS}"
                      FILES_BSD "${UA_FILE_TYPES_BSD}"
                      FILES_SELECTED ${UA_FILE_DATATYPES})

# transport data types
ua_generate_datatypes(INTERNAL NAME "transport" TARGET_SUFFIX "transport" NAMESPACE_IDX 1
                      FILE_CSV "${UA_FILE_NODEIDS}"
                      IMPORT_BSD "TYPES#${UA_FILE_TYPES_BSD}"
                      FILES_BSD "${PROJECT_SOURCE_DIR}/tools/schema/Custom.Opc.Ua.Transport.bsd"
                      FILES_SELECTED "${PROJECT_SOURCE_DIR}/tools/schema/datatypes_transport.txt")

# statuscode explanation
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/open62541/statuscodes.h
                          ${PROJECT_BINARY_DIR}/src_generated/open62541/statuscodes.c
                   PRE_BUILD
                   COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/generate_statuscode_descriptions.py
                           ${UA_FILE_STATUSCODES} ${PROJECT_BINARY_DIR}/src_generated/open62541/statuscodes
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_statuscode_descriptions.py
                           ${UA_FILE_STATUSCODES})

# Header containing defines for all NodeIds
ua_generate_nodeid_header(NAME "nodeids" ID_PREFIX "NS0" TARGET_SUFFIX "ids-ns0"
                          FILE_CSV "${UA_FILE_NODEIDS}")

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
                       COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/amalgamate.py
                               ${OPEN62541_VERSION} ${CMAKE_CURRENT_BINARY_DIR}/open62541.h
                               ${exported_headers} ${plugin_headers}
                       DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/amalgamate.py
                               ${exported_headers} ${plugin_headers})

    add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/open62541.c
                       PRE_BUILD
                       COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/amalgamate.py
                               ${OPEN62541_VERSION} ${CMAKE_CURRENT_BINARY_DIR}/open62541.c
                               ${lib_headers} ${lib_sources} ${plugin_sources}
                       DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/amalgamate.py ${lib_headers}
                               ${lib_sources} ${plugin_sources})

    add_custom_target(open62541-amalgamation-source DEPENDS ${PROJECT_BINARY_DIR}/open62541.c)
    add_custom_target(open62541-amalgamation-header DEPENDS ${PROJECT_BINARY_DIR}/open62541.h)

    add_dependencies(open62541-amalgamation-header open62541-generator-types)
    add_dependencies(open62541-amalgamation-source open62541-generator-types
                     open62541-generator-transport open62541-generator-statuscode)
endif()

ua_generate_nodeset(NAME "ns0" FILE ${UA_FILE_NODESETS} ${UA_NODESET_FILE_DA}
                    INTERNAL BLACKLIST ${UA_FILE_NS0_BLACKLIST}
                    IGNORE "${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/NodeID_NS0_Base.txt"
                    DEPENDS_TARGET "open62541-generator-types")

if(UA_ENABLE_NODESET_INJECTOR)
    message(STATUS "Nodesetinjector feature enabled")
    cmake_minimum_required(VERSION 3.20)
    add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/open62541/nodesetinjector.h
                       ${PROJECT_BINARY_DIR}/src_generated/open62541/nodesetinjector.c PRE_BUILD
                       COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/nodeset_injector/generate_nodesetinjector.py
                       ${PROJECT_BINARY_DIR}/src_generated/open62541/nodesetinjector)
    add_custom_target(open62541-generator-nodesetinjector DEPENDS
                      ${PROJECT_BINARY_DIR}/src_generated/open62541/nodesetinjector.c
                      ${PROJECT_BINARY_DIR}/src_generated/open62541/nodesetinjector.h)
    set(UA_NODESETINJECTOR_GENERATORS "")
    set(UA_NODESETINJECTOR_SOURCE_FILES "")
    set(UA_NODESETINJECTOR_EXAMPLE_NAMES "")
    set(UA_NODESETINJECTOR_TEST_NAMES "")
endif()

#####################
# Build the Library #
#####################

assign_source_group(${exported_headers})
assign_source_group(${lib_headers} ${lib_sources})
assign_source_group(${plugin_headers} ${plugin_sources})

if(UA_ENABLE_AMALGAMATION)
    add_library(open62541-object OBJECT ${PROJECT_BINARY_DIR}/open62541.c ${PROJECT_BINARY_DIR}/open62541.h)
    target_include_directories(open62541-object PRIVATE ${PROJECT_BINARY_DIR})
    if(UA_ENABLE_ENCRYPTION_MBEDTLS)
        target_include_directories(open62541-object PRIVATE ${MBEDTLS_INCLUDE_DIRS})
    endif()
    if(UA_ENABLE_ENCRYPTION_OPENSSL)
        target_include_directories(open62541-object PRIVATE ${OPENSSL_INCLUDE_DIR})
    endif()
    if(UA_ENABLE_ENCRYPTION_LIBRESSL)
        target_include_directories(open62541-object PRIVATE ${LIBRESSL_INCLUDE_DIR})
    endif()
    if(UA_ENABLE_NODESETLOADER)
        target_include_directories(open62541-object PRIVATE
                                   ${NODESETLOADER_PUBLIC_INCLUDES}
                                   ${NODESETLOADER_PRIVATE_INCLUDES})
    endif()

    # make sure the open62541_amalgamation target builds before so that amalgamation is finished and it is not executed again for open62541-object
    # and thus may overwrite the amalgamation result during multiprocessor compilation
    # the header is already a dependency of open62541 target itself
    add_custom_target(open62541-code-generation DEPENDS
                      open62541-amalgamation-header
                      open62541-generator-types
                      open62541-generator-transport
                      open62541-generator-statuscode
                      open62541-amalgamation-source)

    add_library(open62541 $<TARGET_OBJECTS:open62541-object>)
    # the only directory that needs to be included if open62541 (amalgameted) target from
    # the build directory is ${PROJECT_BINARY_DIR}, that contains the generated open62541.h
    target_include_directories(open62541 PUBLIC $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>)

    add_dependencies(open62541-amalgamation-source open62541-generator-namespace)
    add_dependencies(open62541-amalgamation-header open62541-generator-namespace)
else()
    add_library(open62541-object OBJECT ${lib_sources} ${lib_headers} ${exported_headers})
    target_include_directories(open62541-object PRIVATE ${PROJECT_SOURCE_DIR}/src)

    add_custom_target(open62541-code-generation DEPENDS
                      open62541-generator-types
                      open62541-generator-transport
                      open62541-generator-statuscode
                      open62541-generator-namespace)

    if(UA_ENABLE_NODESET_INJECTOR)
        add_dependencies(open62541-code-generation open62541-generator-nodesetinjector)
    endif()

    # stack protector and optimization are disabled for the huge ns0 file
    if(UA_NAMESPACE_ZERO STREQUAL "FULL" AND NOT MSVC)
        set_source_files_properties(${PROJECT_BINARY_DIR}/src_generated/open62541/namespace0_generated.c
                                    PROPERTIES COMPILE_FLAGS "-fno-stack-protector -O0")
    endif()

    add_library(open62541-plugins OBJECT ${plugin_sources} ${exported_headers})
    add_dependencies(open62541-plugins open62541-generator-types open62541-generator-transport open62541-generator-namespace)
    target_include_directories(open62541-plugins PRIVATE ${PROJECT_SOURCE_DIR}/plugins)
    target_include_directories(open62541-plugins PRIVATE ${PROJECT_BINARY_DIR}/src_generated)
    target_compile_definitions(open62541-plugins PRIVATE -DUA_DYNAMIC_LINKING_EXPORT)
    set_target_properties(open62541-plugins PROPERTIES FOLDER "open62541/lib")

    if(UA_ENABLE_COVERAGE)
        add_coverage(open62541-object)
        add_coverage(open62541-plugins)
    endif()

    add_library(open62541 $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-plugins>)

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
    include_directories_public("${PROJECT_SOURCE_DIR}/include"
                               "${PROJECT_SOURCE_DIR}/plugins/include"
                               "${PROJECT_SOURCE_DIR}/deps"
                               "${PROJECT_SOURCE_DIR}/src/pubsub"
                               "${PROJECT_BINARY_DIR}/src_generated")

    if(UA_ENABLE_NODESETLOADER)
        include_directories_public(${NODESETLOADER_PUBLIC_INCLUDES})
    endif()

    # Private includes
    include_directories_private("${PROJECT_BINARY_DIR}")

    if(UA_ENABLE_ENCRYPTION_MBEDTLS)
        include_directories_private(${MBEDTLS_INCLUDE_DIRS})
    endif()
    if(UA_ENABLE_ENCRYPTION_OPENSSL)
        include_directories_private(${OPENSSL_INCLUDE_DIR})
    endif()
    if(UA_ENABLE_ENCRYPTION_LIBRESSL)
        include_directories_private(${LIBRESSL_INCLUDE_DIR})
    endif()
    if(UA_ENABLE_NODESETLOADER)
        include_directories_private(${NODESETLOADER_PRIVATE_INCLUDES})
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

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(linting_build)
include(linting_target)

##########################
# Installation           #
##########################

# invoke via `make install`
# specify install location with `-DCMAKE_INSTALL_PREFIX=xyz`
# Enable shared library with `-DBUILD_SHARED_LIBS=ON`

if(UA_ENABLE_AMALGAMATION)
    install(CODE "MESSAGE(FATAL_ERROR \"Installation with UA_ENABLE_AMALGAMATION=ON is not possible.\")")
endif()

# export library (either static or shared depending on BUILD_SHARED_LIBS)
install(TARGETS open62541
        EXPORT open62541Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION include)

# Create open62541Config.cmake
include(CMakePackageConfigHelpers)
set(cmake_configfile_install ${CMAKE_INSTALL_LIBDIR}/cmake/open62541)
configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/tools/cmake/open62541Config.cmake.in"
                              "${CMAKE_CURRENT_BINARY_DIR}/open62541Config.cmake"
                              INSTALL_DESTINATION "${cmake_configfile_install}")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/open62541Config.cmake"
        DESTINATION "${cmake_configfile_install}")

# Create open62541ConfigVersion.cmake
set(open62541_VERSION)
get_target_property(open62541_VERSION open62541 VERSION)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/open62541ConfigVersion.cmake"
                                 VERSION ${open62541_VERSION} COMPATIBILITY AnyNewerVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/open62541ConfigVersion.cmake"
        DESTINATION "${cmake_configfile_install}")

# Create open62541Targets.cmake
install(EXPORT open62541Targets
        FILE open62541Targets.cmake
        DESTINATION "${cmake_configfile_install}"
        NAMESPACE open62541::)
export(TARGETS open62541 NAMESPACE open62541::
       FILE ${CMAKE_CURRENT_BINARY_DIR}/open62541Targets.cmake)

# Create open62541Targets.cmake
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    install(FILES "${PROJECT_BINARY_DIR}/src_generated/open62541.pc"
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()

# Recreate the include folder structure from the source also in /usr/lib/include/open62541
if(NOT UA_ENABLE_AMALGAMATION)
    set(FILES_TO_INSTALL ${exported_headers} ${plugin_headers})
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
endif()

if(UA_PACK_DEBIAN)
    add_subdirectory(tools/packaging)
endif()

##################################
# Visual Studio Solution Folders #
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
if(UA_ENABLE_NODESET_INJECTOR)
    set_target_properties(open62541-generator-nodesetinjector PROPERTIES FOLDER "open62541/generators")
    add_subdirectory(tools/nodeset_injector)
    set(UA_NODESETINJECTOR_GENERATORS ${UA_NODESETINJECTOR_GENERATORS} PARENT_SCOPE)
endif()
