include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/plugins)
include_directories(${PROJECT_BINARY_DIR})
include_directories(${PROJECT_SOURCE_DIR}/examples)
include_directories(${PROJECT_SOURCE_DIR}/arch)
include_directories(${PROJECT_SOURCE_DIR}/deps/mqtt-c)
include_directories(${PROJECT_SOURCE_DIR}/examples/mqtt)

set(examples_headers
    ${examples_headers}
    ${PROJECT_SOURCE_DIR}/examples/common.h
    )

#############################
# Compiled binaries folders #
#############################

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/examples)

macro(add_example EXAMPLE_NAME EXAMPLE_SOURCE)
  add_executable(${EXAMPLE_NAME} ${STATIC_OBJECTS} ${EXAMPLE_SOURCE} ${ARGN} ${examples_headers})
  target_link_libraries(${EXAMPLE_NAME} open62541 ${open62541_LIBRARIES})
  assign_source_group(${EXAMPLE_SOURCE})
  add_dependencies(${EXAMPLE_NAME} open62541-amalgamation-header open62541-amalgamation-source)
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

##################
# Example Client #
##################

add_example(client client.c)

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

if(UA_ENABLE_ENCRYPTION)
    add_example(server_basic128rsa15 encryption/server_basic128rsa15.c)
    add_example(server_basic256sha256 encryption/server_basic256sha256.c)
    # Add secure client example application
    add_example(client_basic128rsa15 encryption/client_basic128rsa15.c)
    add_example(client_basic256sha256 encryption/client_basic256sha256.c)
endif()

add_example(custom_datatype_client custom_datatype/client_types_custom.c)
add_example(custom_datatype_server custom_datatype/server_types_custom.c)

if(UA_ENABLE_NODEMANAGEMENT)
    add_example(access_control_server access_control/server_access_control.c)
    add_example(access_control_client access_control/client_access_control.c)
endif()

if(UA_BUILD_SELFSIGNED_CERTIFICATE)
  find_package(OpenSSL REQUIRED)
  add_custom_command(OUTPUT server_cert.der
                     COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/certs/create_self-signed.py ${CMAKE_CURRENT_BINARY_DIR}
                     DEPENDS ${PROJECT_SOURCE_DIR}/tools/certs/create_self-signed.py
                             ${PROJECT_SOURCE_DIR}/tools/certs/localhost.cnf)
  add_custom_target(selfsigned ALL DEPENDS server_cert.der)
  add_executable(server_certificate server_certificate.c ${STATIC_OBJECTS} server_cert.der)
  target_link_libraries(server_certificate open62541 ${open62541_LIBRARIES})
endif()

if(UA_ENABLE_DISCOVERY)
    add_example(discovery_server_lds discovery/server_lds.c)

    add_example(discovery_server_register discovery/server_register.c)

    add_example(discovery_client_find_servers discovery/client_find_servers.c)
    if(UA_ENABLE_DISCOVERY_MULTICAST)
        add_example(discovery_server_multicast discovery/server_multicast.c)
    endif()
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
    endif()
    if(UA_ENABLE_PUBSUB_MQTT)
        add_example(tutorial_pubsub_mqtt pubsub/tutorial_pubsub_mqtt.c)
    endif()
endif()
