cmake_minimum_required(VERSION 2.8.11)
project(open62541 C)
# set(CMAKE_VERBOSE_MAKEFILE ON)

######################
# Check Dependencies #
######################

# Find Python and Python-lxml
find_package(PythonInterp REQUIRED)
execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import lxml"
                RESULT_VARIABLE IMPORT_LXML_RESULT)
if(NOT "${IMPORT_LXML_RESULT}" STREQUAL "0")
    message(FATAL_ERROR "Python-lxml is not installed.")
endif()

# Check for git
find_package(Git)
if(GIT_FOUND)
  execute_process(COMMAND ${GIT_EXECUTABLE} describe --abbrev=7 --dirty --always --tags
                  RESULT_VARIABLE res_var
                  OUTPUT_VARIABLE GIT_COM_ID )
    if(NOT ${res_var} EQUAL 0)
        set(GIT_COMMIT_ID "unknown--git-commit-id-unknown")
        message(STATUS "Git failed (not a repo, or no tags). Build will not contain git revision info." )
    else()
        string(REPLACE "\n" "" GIT_COMMIT_ID ${GIT_COM_ID} )
    endif()
else()
    set(GIT_COMMIT_ID "unknown--no-git-found")
    message(STATUS "Git not found. Build will not contain git revision info." )
endif()
add_definitions("-DVERSION=${GIT_COMMIT_ID}")

message(STATUS "Git version: "  ${GIT_COMMIT_ID})

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

####################################
# Build Type and Compiler Settings #
####################################

# Set default build type.
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "CMAKE_BUILD_TYPE not given; setting to 'Debug'.")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
endif()

# Collect libraries
list(APPEND open62541_LIBRARIES "")

# compiler flags
if(CMAKE_COMPILER_IS_GNUCC OR "x${CMAKE_C_COMPILER_ID}" STREQUAL "xClang")
    add_definitions(-std=c99 -pipe -Wall -Wextra -Werror -Wformat -Wno-unused-parameter
                    -Wno-unused-function -Wno-unused-label -Wpointer-arith -Wreturn-type -Wsign-compare
                    -Wmultichar -Wstrict-overflow -Wcast-qual -Wmissing-prototypes -Wstrict-prototypes
                    -Winit-self -Wuninitialized -Wformat-security -Wformat-nonliteral)

  # library linking
  set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "") # cmake sets -rdynamic by default
  if(NOT WIN32 AND NOT CYGWIN)
	add_definitions(-Wshadow -Wconversion -fvisibility=hidden -fPIC)
    if(NOT APPLE)
      set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,-z,norelro -Wl,--hash-style=gnu -Wl,--build-id=none")
    endif()
  endif()

  # Debug
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	#add_definitions(-fsanitize=address)
    #list(APPEND open62541_LIBRARIES asan)

	#add_definitions(-fsanitize=undefined)
    #list(APPEND open62541_LIBRARIES ubsan)

  elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel" OR
         CMAKE_BUILD_TYPE STREQUAL "Release")
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
      if(NOT WIN32 AND NOT CYGWIN)
        # these settings reduce the binary size by ~2kb
        set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,-z,norelro -Wl,--hash-style=gnu -Wl,--build-id=none")
      endif()
    endif()
  endif()
  
  if(APPLE)
    set(CMAKE_MACOSX_RPATH 1)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_DARWIN_C_SOURCE=1")
  endif()
elseif(MSVC)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
endif()

############
# Settings #
############

# Options
set(UA_LOGLEVEL 300 CACHE STRING "Level at which logs shall be reported")
option(UA_ENABLE_SUBSCRIPTIONS "Enable compilation of subscription and monitoring support." OFF)
option(UA_ENABLE_METHODCALLS "Enable CallMethod/MethodCall service set" OFF)
option(UA_ENABLE_MULTITHREADING "Enable multithreading" OFF)
option(UA_ENABLE_NODEMANAGEMENT "Enable dynamic addition and removal of nodes" ON)
option(UA_ENABLE_AMALGAMATION "Concatenate the library to a single file open62541.h/.c" OFF)
option(UA_ENABLE_COVERAGE "Enable gcov coverage" OFF)
if(UA_ENABLE_COVERAGE)
  set(CMAKE_BUILD_TYPE DEBUG)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
endif()

# Advanced options
option(UA_ENABLE_TYPENAMES "Add the type and member names to the UA_DataType structure" OFF)
mark_as_advanced(UA_ENABLE_TYPENAMES)

option(UA_ENABLE_GENERATE_NAMESPACE0 "Generate and load UA XML Namespace 0 definition" OFF)

option(UA_ENABLE_EMBEDDED_LIBC "Target has no libc, use internal definitions" OFF)
mark_as_advanced(UA_ENABLE_EMBEDDED_LIBC)

option(UA_ENABLE_EXTERNAL_NAMESPACES "Enable namespace handling by an external component (experimental)" OFF)
mark_as_advanced(UA_ENABLE_EXTERNAL_NAMESPACES)

option(UA_ENABLE_NONSTANDARD_STATELESS "Enable stateless extension" OFF)
mark_as_advanced(UA_ENABLE_NONSTANDARD_STATELESS)

option(UA_ENABLE_NONSTANDARD_UDP "Enable udp extension" OFF)
mark_as_advanced(UA_ENABLE_NONSTANDARD_UDP)
if(UA_ENABLE_NONSTANDARD_UDP)
  set(UA_ENABLE_NONSTANDARD_STATELESS ON)
endif()

# Build Targets
option(UA_BUILD_EXAMPLESERVER "Build the example server" OFF)
option(UA_BUILD_EXAMPLECLIENT "Build a test client" OFF)
option(UA_BUILD_UNIT_TESTS "Run unit tests after building" OFF)
option(UA_BUILD_EXAMPLES "Build example servers and clients" OFF)
option(UA_BUILD_DOCUMENTATION "Generate doxygen/sphinx documentation" OFF)

# Advanced Build Targets
option(UA_BUILD_SELFSIGNED_CERTIFICATE "Generate self-signed certificate" OFF)
mark_as_advanced(UA_BUILD_SELFSIGNED_CERTIFICATE)

#########################
# Generate Main Library #
#########################

file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/src_generated")
include_directories(${PROJECT_BINARY_DIR}/src_generated) #needed to locate an include of examples/server_nodeset.c
configure_file("include/ua_config.h.in" "${PROJECT_BINARY_DIR}/src_generated/ua_config.h")

set(exported_headers ${PROJECT_BINARY_DIR}/src_generated/ua_config.h
                     ${PROJECT_SOURCE_DIR}/include/ua_statuscodes.h
                     ${PROJECT_SOURCE_DIR}/include/ua_types.h
                     ${PROJECT_BINARY_DIR}/src_generated/ua_nodeids.h
                     ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated.h
                     ${PROJECT_SOURCE_DIR}/include/ua_job.h
                     ${PROJECT_SOURCE_DIR}/include/ua_connection.h
                     ${PROJECT_SOURCE_DIR}/include/ua_log.h
                     ${PROJECT_SOURCE_DIR}/include/ua_server.h
                     ${PROJECT_SOURCE_DIR}/include/ua_server_external_ns.h
                     ${PROJECT_SOURCE_DIR}/include/ua_client.h
                     ${PROJECT_SOURCE_DIR}/include/ua_client_highlevel.h
                     ${PROJECT_SOURCE_DIR}/plugins/networklayer_tcp.h
                     ${PROJECT_SOURCE_DIR}/plugins/logger_stdout.h)
set(internal_headers ${PROJECT_SOURCE_DIR}/deps/queue.h
                     ${PROJECT_SOURCE_DIR}/deps/pcg_basic.h
                     ${PROJECT_SOURCE_DIR}/deps/libc_time.h
                     ${PROJECT_SOURCE_DIR}/src/ua_util.h
                     ${PROJECT_SOURCE_DIR}/src/ua_types_encoding_binary.h
                     ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated_encoding_binary.h
                     ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated.h
                     ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated_encoding_binary.h
                     ${PROJECT_SOURCE_DIR}/src/ua_securechannel.h
                     ${PROJECT_SOURCE_DIR}/src/server/ua_nodes.h
                     ${PROJECT_SOURCE_DIR}/src/ua_session.h
                     ${PROJECT_SOURCE_DIR}/src/server/ua_nodestore.h
                     ${PROJECT_SOURCE_DIR}/src/server/ua_session_manager.h
                     ${PROJECT_SOURCE_DIR}/src/server/ua_securechannel_manager.h
                     ${PROJECT_SOURCE_DIR}/src/server/ua_server_internal.h
                     ${PROJECT_SOURCE_DIR}/src/server/ua_services.h
                     ${PROJECT_SOURCE_DIR}/src/client/ua_client_internal.h)
set(lib_sources ${PROJECT_SOURCE_DIR}/src/ua_types.c
                ${PROJECT_SOURCE_DIR}/src/ua_types_encoding_binary.c
                ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated.c
                ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated.c
                ${PROJECT_SOURCE_DIR}/src/ua_connection.c
                ${PROJECT_SOURCE_DIR}/src/ua_securechannel.c
                ${PROJECT_SOURCE_DIR}/src/ua_session.c
                ${PROJECT_SOURCE_DIR}/src/server/ua_server.c
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
                ${PROJECT_SOURCE_DIR}/src/client/ua_client_highlevel.c
                ${PROJECT_SOURCE_DIR}/plugins/networklayer_tcp.c
                ${PROJECT_SOURCE_DIR}/plugins/logger_stdout.c
                ${PROJECT_SOURCE_DIR}/deps/libc_time.c
                ${PROJECT_SOURCE_DIR}/deps/pcg_basic.c)
                ##TODO: make client stuff optional

if(UA_ENABLE_METHODCALLS)
  list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/src/server/ua_services_call.c)
endif()

if(UA_ENABLE_EMBEDDED_LIBC)
  list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/deps/libc_string.c)
endif()

if(UA_ENABLE_MULTITHREADING)
  find_package(Threads REQUIRED)
  list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/src/server/ua_nodestore_concurrent.c)
else()
  list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/src/server/ua_nodestore.c)
endif()

set(generate_typeintrospection "")
if(UA_ENABLE_TYPENAMES)
  set(generate_typeintrospection "--typeintrospection")
endif()

set(generate_subscriptiontypes "")
if(UA_ENABLE_SUBSCRIPTIONS)
  list(APPEND lib_sources ${PROJECT_SOURCE_DIR}/src/server/ua_services_subscription.c
                          ${PROJECT_SOURCE_DIR}/src/server/ua_subscription.c
                          ${PROJECT_SOURCE_DIR}/src/server/ua_subscription_manager.c
                          ${PROJECT_SOURCE_DIR}/src/client/ua_client_highlevel_subscriptions.c)
  #append subscription headers at before ua_session
  list(FIND internal_headers "${PROJECT_SOURCE_DIR}/src/ua_session.h" UaSessionPos)
  list(INSERT internal_headers  ${UaSessionPos} ${PROJECT_SOURCE_DIR}/src/server/ua_subscription.h
                                                ${PROJECT_SOURCE_DIR}/src/server/ua_subscription_manager.h)
  set(generate_subscriptiontypes "--enable-subscription-types=1")
endif()

if(UA_ENABLE_GENERATE_NAMESPACE0)
  set(GENERATE_NAMESPACE0_FILE "Opc.Ua.NodeSet2.xml" CACHE STRING "Namespace definition XML file")
  set_property(CACHE GENERATE_NAMESPACE0_FILE PROPERTY STRINGS Opc.Ua.NodeSet2.xml Opc.Ua.NodeSet2.Minimal.xml)
  list(APPEND internal_headers ${PROJECT_BINARY_DIR}/src_generated/ua_namespaceinit_generated.h)
  list(APPEND lib_sources ${PROJECT_BINARY_DIR}/src_generated/ua_namespaceinit_generated.c)
endif()

#########################
# Generate source files #
#########################

# standard data types
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated.c
                          ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated.h
                          ${PROJECT_BINARY_DIR}/src_generated/ua_types_generated_encoding_binary.h
                   PRE_BUILD
                   COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/generate_datatypes.py
                                                ${generate_subscriptiontypes}
                                                ${generate_typeintrospection}
                                                --typedescriptions ${PROJECT_SOURCE_DIR}/tools/schema/NodeIds.csv
                                                0
                                                ${PROJECT_SOURCE_DIR}/tools/schema/Opc.Ua.Types.bsd
                                                ${PROJECT_BINARY_DIR}/src_generated/ua_types
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_datatypes.py
                           ${CMAKE_CURRENT_SOURCE_DIR}/tools/schema/Opc.Ua.Types.bsd
                           ${CMAKE_CURRENT_SOURCE_DIR}/tools/schema/NodeIds.csv)

# transport data types
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated.c
                          ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated.h
                          ${PROJECT_BINARY_DIR}/src_generated/ua_transport_generated_encoding_binary.h
                   PRE_BUILD
                   COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/generate_datatypes.py
                                                ${generate_typeintrospection}
                                                --ns0-types-xml ${PROJECT_SOURCE_DIR}/tools/schema/Opc.Ua.Types.bsd
                                                1
                                                ${PROJECT_SOURCE_DIR}/tools/schema/Custom.Opc.Ua.Transport.bsd
                                                ${PROJECT_BINARY_DIR}/src_generated/ua_transport
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_datatypes.py
                           ${CMAKE_CURRENT_SOURCE_DIR}/tools/schema/Custom.Opc.Ua.Transport.bsd)

# nodeids
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/ua_nodeids.h
                   PRE_BUILD
                   COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/generate_nodeids.py
                                                ${PROJECT_SOURCE_DIR}/tools/schema/NodeIds.csv
                                                ${PROJECT_BINARY_DIR}/src_generated/ua_nodeids
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_nodeids.py
                           ${CMAKE_CURRENT_SOURCE_DIR}/tools/schema/NodeIds.csv)

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
                           ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/logger.py
                           ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/open62541_MacroHelper.py
                           ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/ua_builtin_types.py
                           ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/ua_constants.py
                           ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/ua_namespace.py
                           ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/ua_node_types.py)

# single-file release
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/open62541.h
                   PRE_BUILD
                   COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/amalgamate.py
                                                ${GIT_COMMIT_ID}
                                                ${CMAKE_CURRENT_BINARY_DIR}/open62541.h ${exported_headers}
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/amalgamate.py
                           ${exported_headers}
                           ${internal_headers})

add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/open62541.c
                   PRE_BUILD
                   COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/amalgamate.py
                                                ${GIT_COMMIT_ID}
                                                ${CMAKE_CURRENT_BINARY_DIR}/open62541.c
                                                ${internal_headers}
                                                ${PROJECT_SOURCE_DIR}/src/server/ua_nodestore_hash.inc
                                                ${lib_sources}
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/amalgamate.py
                           ${internal_headers}
                           ${PROJECT_SOURCE_DIR}/src/server/ua_nodestore_hash.inc
                           ${lib_sources})

#################
# Build Targets #
#################

# build shared library
if(UA_ENABLE_AMALGAMATION)
    add_library(open62541-object OBJECT ${PROJECT_BINARY_DIR}/open62541.c ${PROJECT_BINARY_DIR}/open62541.h)
    include_directories(${PROJECT_BINARY_DIR})
else()
    add_definitions(-DUA_NO_AMALGAMATION)
    add_library(open62541-object OBJECT ${lib_sources} ${internal_headers} ${exported_headers})
    include_directories(${PROJECT_SOURCE_DIR}/include)
    include_directories(${PROJECT_SOURCE_DIR}/src)
    include_directories(${PROJECT_SOURCE_DIR}/plugins)
    include_directories(${PROJECT_SOURCE_DIR}/deps)
endif()
target_compile_definitions(open62541-object PRIVATE -DUA_DYNAMIC_LINKING)
add_library(open62541 SHARED $<TARGET_OBJECTS:open62541-object>)
add_library(open62541-static STATIC $<TARGET_OBJECTS:open62541-object>)
target_compile_definitions(open62541 PRIVATE -DUA_DYNAMIC_LINKING)
target_compile_definitions(open62541-static PRIVATE -DUA_DYNAMIC_LINKING)
target_link_libraries(open62541-static ${open62541_LIBRARIES})
if(WIN32)
  target_link_libraries(open62541 ws2_32)
  target_link_libraries(open62541-static ws2_32)
endif()

if(UA_BUILD_EXAMPLESERVER)
    set(server_source $<TARGET_OBJECTS:open62541-object>)
    add_executable(server_static examples/server.c ${server_source})
    add_executable(server examples/server.c)
    target_link_libraries(server ${open62541_LIBRARIES} open62541)
    target_link_libraries(server_static ${open62541_LIBRARIES})
    if(WIN32)
        target_link_libraries(server_static ws2_32)
        target_link_libraries(server ws2_32)
    elseif(NOT APPLE)
        target_link_libraries(server_static rt)
        target_link_libraries(server rt)
    endif()
    if(UA_ENABLE_MULTITHREADING)
        target_link_libraries(server_static urcu-cds urcu urcu-common pthread)
        target_link_libraries(server urcu-cds urcu urcu-common pthread)
    endif()

    if(UA_ENABLE_NONSTANDARD_UDP)
      add_executable(exampleServerUDP $<TARGET_OBJECTS:open62541-object> examples/networklayer_udp.c examples/server_udp.c)
      target_link_libraries(exampleServerUDP ${open62541_LIBRARIES} open62541)
      if(UA_ENABLE_MULTITHREADING)
  	    target_link_libraries(exampleServerUDP urcu-cds urcu urcu-common)
      endif()
      if (NOT APPLE)
        target_link_libraries(exampleServerUDP rt)
      endif()
    endif()
endif()

if(UA_BUILD_SELFSIGNED_CERTIFICATE)
    message(STATUS "Enabling self-signed certificates")
    find_package(OpenSSL REQUIRED)
    add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/server_cert.der
                              ${PROJECT_BINARY_DIR}/ca.crt
                   COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/certs/create_self-signed.py ${PROJECT_BINARY_DIR}
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/certs/create_self-signed.py
                           ${CMAKE_CURRENT_SOURCE_DIR}/tools/certs/localhost.cnf)
    add_custom_target(selfsigned ALL DEPENDS ${PROJECT_BINARY_DIR}/server_cert.der ${PROJECT_BINARY_DIR}/ca.crt)
endif()

if(UA_BUILD_EXAMPLECLIENT)
	add_definitions(-DBENCHMARK)
    set(client_source $<TARGET_OBJECTS:open62541-object>)
	add_executable(client_static examples/client.c ${client_source})
    add_executable(client examples/client.c)
	target_link_libraries(client_static ${open62541_LIBRARIES})
	target_link_libraries(client open62541 ${open62541_LIBRARIES})
    if(WIN32)
        target_link_libraries(client_static ws2_32)
        target_link_libraries(client ws2_32)
    elseif(NOT APPLE)
        target_link_libraries(client_static rt)
        target_link_libraries(client rt)
    endif()
    if(UA_ENABLE_MULTITHREADING)
        target_link_libraries(client_static urcu-cds urcu urcu-common pthread)
        target_link_libraries(client urcu-cds urcu urcu-common pthread)
    endif()
    if(UA_ENABLE_NONSTANDARD_STATELESS AND NOT UA_ENABLE_AMALGAMATION)
        add_executable(client_stateless examples/client_stateless.c ${client_source})
	    target_link_libraries(client_stateless open62541 ${open62541_LIBRARIES})
        if(UA_ENABLE_MULTITHREADING)
            target_link_libraries(client_stateless urcu-cds urcu urcu-common pthread)
        endif()
    endif()
endif()

if(UA_BUILD_UNIT_TESTS)
	add_definitions(-DUA_BUILD_UNIT_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(UA_BUILD_EXAMPLES)
    #add_subdirectory(examples)
    #FIXME: we had problem with static linking for msvs, here a quick and dirty workaround
    #http://stackoverflow.com/questions/3704374/linking-error-lnk2019-in-msvc-unresolved-symbols-with-imp-prefix-but-shoul
    #http://stackoverflow.com/questions/1089828/same-header-file-for-both-dll-and-static-library
	list(APPEND LIBS ${open62541_LIBRARIES})
    if(NOT WIN32)
		list(APPEND LIBS pthread)
		if (NOT APPLE)
		    list(APPEND LIBS rt)
		endif()
	else()
		list(APPEND LIBS ws2_32)
	endif()
	if(UA_ENABLE_MULTITHREADING)
		list(APPEND LIBS urcu-cds urcu urcu-common)
	endif(UA_ENABLE_MULTITHREADING)
	
    add_executable(server_variable ${PROJECT_SOURCE_DIR}/examples/server_variable.c $<TARGET_OBJECTS:open62541-object>)
	target_link_libraries(server_variable ${LIBS})

    add_executable(server_mainloop ${PROJECT_SOURCE_DIR}/examples/server_mainloop.c $<TARGET_OBJECTS:open62541-object>)
	target_link_libraries(server_mainloop ${LIBS})

	add_executable(server_datasource ${PROJECT_SOURCE_DIR}/examples/server_datasource.c $<TARGET_OBJECTS:open62541-object>)
	target_link_libraries(server_datasource ${LIBS})

	# add_executable(server_readspeed ${PROJECT_SOURCE_DIR}/examples/server_readspeed.c $<TARGET_OBJECTS:open62541-object>)
	# target_link_libraries(server_readspeed ${LIBS})

	add_executable(server_firstSteps ${PROJECT_SOURCE_DIR}/examples/server_firstSteps.c $<TARGET_OBJECTS:open62541-object>)
	target_link_libraries(server_firstSteps ${LIBS})

	add_executable(client_firstSteps ${PROJECT_SOURCE_DIR}/examples/client_firstSteps.c $<TARGET_OBJECTS:open62541-object>)
	target_link_libraries(client_firstSteps ${LIBS})

	add_executable(server_repeated_job ${PROJECT_SOURCE_DIR}/examples/server_repeated_job.c $<TARGET_OBJECTS:open62541-object>)
	target_link_libraries(server_repeated_job ${LIBS})

	add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/nodeset.h ${PROJECT_BINARY_DIR}/src_generated/nodeset.c
					   PRE_BUILD
					   COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/generate_open62541CCode.py
                                                    -i ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/NodeID_Blacklist_FullNS0.txt
                                                    ${PROJECT_SOURCE_DIR}/tools/schema/namespace0/Opc.Ua.NodeSet2.xml
                                                    ${PROJECT_SOURCE_DIR}/examples/server_nodeset.xml
                                                    ${PROJECT_BINARY_DIR}/src_generated/nodeset
					   DEPENDS ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/generate_open62541CCode.py
					           ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/logger.py
					           ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/open62541_MacroHelper.py
					           ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/ua_builtin_types.py
					           ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/ua_constants.py
					           ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/ua_namespace.py
					           ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/ua_node_types.py
					           ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/NodeID_Blacklist_FullNS0.txt
					           ${PROJECT_SOURCE_DIR}/examples/server_nodeset.xml)
					   
	add_executable(server_nodeset ${PROJECT_SOURCE_DIR}/examples/server_nodeset.c
                                  ${PROJECT_BINARY_DIR}/src_generated/nodeset.c
                                  $<TARGET_OBJECTS:open62541-object>)
	target_link_libraries(server_nodeset ${LIBS})

	if(UA_ENABLE_METHODCALLS)
	  add_executable(server_method ${PROJECT_SOURCE_DIR}/examples/server_method.c $<TARGET_OBJECTS:open62541-object>)
	  target_link_libraries(server_method ${LIBS})
	endif()
endif()

if(UA_BUILD_DOCUMENTATION)
    add_subdirectory(doc)
endif()
