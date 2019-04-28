cmake_minimum_required(VERSION 3.0...3.12)
project(open62541-examples)
if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()
# This examples folder can also be built standalone.
# First install open62541 using `make install` then
# copy this folder to any other location and call CMake directly:
#
# cp -r open62541/examples $HOME/open62541_examples
# cd $HOME/open62541_examples
# mkdir build && cd build
# cmake -DUA_NAMESPACE_ZERO=FULL ..
# make -j

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # Examples are built standalone. Find installed open62541

    if(UA_NAMESPACE_ZERO STREQUAL "FULL")
        find_package(open62541 REQUIRED COMPONENTS FullNamespace)
    else()
        find_package(open62541 REQUIRED)
    endif()

    if(NOT UA_TOOLS_DIR)
        set(UA_TOOLS_DIR ${open62541_TOOLS_DIR})
    endif()

    if(NOT UA_NODESET_DIR)
        set(UA_NODESET_DIR ${open62541_NODESET_DIR})
    endif()

    function(assign_source_group)
        # define empty function. We don't need it in standalone
    endfunction(assign_source_group)

    include_directories(${PROJECT_BINARY_DIR}/src_generated)
endif()

# Required for common.h header file used in examples
include_directories(${CMAKE_CURRENT_LIST_DIR})

if(UA_ENABLE_AMALGAMATION)
    add_definitions(-DUA_ENABLE_AMALGAMATION)
endif()

#############################
# Compiled binaries folders #
#############################

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/examples)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/examples)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/examples)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin/examples)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin/examples)

macro(add_example EXAMPLE_NAME EXAMPLE_SOURCE)
  add_executable(${EXAMPLE_NAME} ${STATIC_OBJECTS} ${EXAMPLE_SOURCE} ${ARGN} ${PROJECT_SOURCE_DIR}/common.h)
  target_link_libraries(${EXAMPLE_NAME} open62541::open62541)
  assign_source_group(${EXAMPLE_SOURCE})
  set_target_properties(${EXAMPLE_NAME} PROPERTIES FOLDER "open62541/examples")
  set_target_properties(${EXAMPLE_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
  if(UA_COMPILE_AS_CXX)
      set_source_files_properties(${EXAMPLE_SOURCE} PROPERTIES LANGUAGE CXX)
  endif()
endmacro()

#############
# Tutorials #
#############

add_example(tutorial_datatypes tutorial_datatypes.c)

add_example(tutorial_server_firststeps tutorial_server_firststeps.c)

add_example(tutorial_server_variable tutorial_server_variable.c)

add_example(tutorial_server_datasource tutorial_server_datasource.c)

if(UA_ENABLE_SUBSCRIPTIONS)
    add_example(tutorial_server_monitoreditems tutorial_server_monitoreditems.c)
endif()

add_example(tutorial_server_variabletype tutorial_server_variabletype.c)

add_example(tutorial_server_object tutorial_server_object.c)

if(UA_ENABLE_METHODCALLS)
    add_example(tutorial_server_method tutorial_server_method.c)
endif()

add_example(tutorial_client_firststeps tutorial_client_firststeps.c)

add_example(tutorial_client_events tutorial_client_events.c)

if(UA_ENABLE_SUBSCRIPTIONS_EVENTS)
    add_example(tutorial_server_events tutorial_server_events.c)
endif()

##################
# Example Server #
##################

add_example(server_ctt server_ctt.c)

install(PROGRAMS $<TARGET_FILE:server_ctt>
        DESTINATION bin
        RENAME ua_server_ctt.exe)

##################
# Example Client #
##################

add_example(client client.c)

add_example(client_connect client_connect.c)

if(UA_ENABLE_HISTORIZING)
    add_example(client_historical client_historical.c)
endif()

install(PROGRAMS $<TARGET_FILE:client>
        DESTINATION bin
        RENAME ua_client)

add_example(client_async client_async.c)

add_example(client_connect_loop client_connect_loop.c)

add_example(client_connectivitycheck_loop client_connectivitycheck_loop.c)

if(UA_ENABLE_SUBSCRIPTIONS)
    add_example(client_subscription_loop client_subscription_loop.c)
endif()

####################
# Feature Examples #
####################

add_example(server_mainloop server_mainloop.c)

add_example(server_instantiation server_instantiation.c)

add_example(server_repeated_job server_repeated_job.c)

add_example(server_inheritance server_inheritance.c)

if(UA_ENABLE_HISTORIZING)
    add_example(tutorial_server_historicaldata tutorial_server_historicaldata.c)
endif()

if(UA_ENABLE_ENCRYPTION)
    add_example(server_encryption encryption/server_encryption.c)
    add_example(client_encryption encryption/client_encryption.c)
    target_include_directories(server_encryption PRIVATE "${PROJECT_SOURCE_DIR}/examples")
    target_include_directories(client_encryption PRIVATE "${PROJECT_SOURCE_DIR}/examples")
endif()

add_example(custom_datatype_client custom_datatype/client_types_custom.c)
add_example(custom_datatype_server custom_datatype/server_types_custom.c)

if(UA_ENABLE_NODEMANAGEMENT)
    add_example(access_control_server access_control/server_access_control.c)
    add_example(access_control_client access_control/client_access_control.c)
endif()

if(UA_ENABLE_DISCOVERY_MULTICAST)
    add_example(discovery_server_lds discovery/server_lds.c)
    add_example(discovery_server_register discovery/server_register.c)
    add_example(discovery_client_find_servers discovery/client_find_servers.c)
    add_example(discovery_server_multicast discovery/server_multicast.c)
endif()

add_subdirectory(nodeset)

####################
#  Example PubSub  #
####################

if(UA_ENABLE_PUBSUB)
    add_example(tutorial_pubsub_connection pubsub/tutorial_pubsub_connection.c)
    add_example(tutorial_pubsub_publish pubsub/tutorial_pubsub_publish.c)
    if(UA_ENABLE_AMALGAMATION)
        message(WARNING "PubSub subscriber tutorial (preview) can not be used with AMALGAMATION. Skipping tutorial_pubsub_subscribe.")
    else(NOT UA_ENABLE_AMALGAMATION)
        add_example(tutorial_pubsub_subscribe pubsub/tutorial_pubsub_subscribe.c)
        add_example(pubsub_subscribe_standalone pubsub/pubsub_subscribe_standalone.c)
    endif()
endif()
