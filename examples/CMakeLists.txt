cmake_minimum_required(VERSION 3.13)
project(open62541-examples C)
if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

# The examples folder can be built standalone.
# First install open62541 system-wide using `make install`.
# Then copy this folder to any other location and call CMake directly:
#
# cd ./open62541_examples
# mkdir build && cd build
# cmake ..
# make -j

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # Examples are built standalone. Find installed open62541
    find_package(open62541 REQUIRED)

    # Define empty function. We don't need it in standalone
    function(assign_source_group)
    endfunction(assign_source_group)
endif()

#####################
# CMake Definitions #
#####################

# Required for common.h header file used in examples
include_directories(${CMAKE_CURRENT_LIST_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/examples)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/examples)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/examples)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin/examples)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin/examples)

macro(add_example EXAMPLE_NAME EXAMPLE_SOURCE)
    add_executable(${EXAMPLE_NAME} ${STATIC_OBJECTS}
                   ${EXAMPLE_SOURCE} ${ARGN} ${PROJECT_SOURCE_DIR}/common.h)
    target_link_libraries(${EXAMPLE_NAME} open62541::open62541)
    assign_source_group(${EXAMPLE_SOURCE})
    set_target_properties(${EXAMPLE_NAME} PROPERTIES FOLDER "open62541/examples")
    set_target_properties(${EXAMPLE_NAME} PROPERTIES
                          VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endmacro()

#############
# Tutorials #
#############

add_example(tutorial_datatypes tutorial_datatypes.c)
add_example(tutorial_server_firststeps tutorial_server_firststeps.c)
add_example(tutorial_server_variable tutorial_server_variable.c)
add_example(tutorial_server_datasource tutorial_server_datasource.c)
add_example(tutorial_server_variabletype tutorial_server_variabletype.c)
add_example(tutorial_server_object tutorial_server_object.c)
add_example(tutorial_server_reverseconnect tutorial_server_reverseconnect.c)

if(UA_ENABLE_SUBSCRIPTIONS)
    add_example(tutorial_server_monitoreditems tutorial_server_monitoreditems.c)
endif()

if(UA_ENABLE_METHODCALLS)
    add_example(tutorial_server_method tutorial_server_method.c)
    if (UA_MULTITHREADING GREATER_EQUAL 100)
        add_example(tutorial_server_method_async tutorial_server_method_async.c)
    endif()
endif()

# Batch operation examples using async API
if (UA_MULTITHREADING GREATER_EQUAL 100)
    add_example(tutorial_server_batch_rw tutorial_server_batch_rw.c)
    add_example(tutorial_client_batch_read_trigger tutorial_client_batch_read_trigger.c)
endif()

if(UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS)
    add_example(tutorial_server_alarms_conditions tutorial_server_alarms_conditions.c)
endif()

add_example(tutorial_client_firststeps tutorial_client_firststeps.c)

if(UA_ENABLE_SUBSCRIPTIONS_EVENTS)
    add_example(tutorial_client_events tutorial_client_events.c)
    add_example(tutorial_server_events tutorial_server_events.c)
endif()

##################
# Example Client #
##################

add_example(client client.c)
add_example(client_connect client_connect.c)
add_example(client_async client_async.c)
add_example(client_connect_loop client_connect_loop.c)

if(UA_ENABLE_HISTORIZING)
    add_example(client_historical client_historical.c)
endif()

if(UA_ENABLE_METHODCALLS AND UA_MULTITHREADING GREATER_EQUAL 100)
    add_example(client_method_async client_method_async.c)
endif()

if(UA_ENABLE_SUBSCRIPTIONS)
    add_example(client_subscription_loop client_subscription_loop.c)
endif()

####################
# Feature Examples #
####################

add_example(ci_server ci_server.c)
add_example(server_settimestamp server_settimestamp.c)
add_example(server_mainloop server_mainloop.c)
add_example(server_instantiation server_instantiation.c)
add_example(server_repeated_job server_repeated_job.c)
add_example(server_inheritance server_inheritance.c)
add_example(server_loglevel server_loglevel.c)
add_example(custom_datatype_client custom_datatype/client_types_custom.c)
add_example(custom_datatype_server custom_datatype/server_types_custom.c)

if(UA_ENABLE_SUBSCRIPTIONS_EVENTS)
    add_example(server_events_random events/server_random_events.c)
    add_example(client_event_filter events/client_eventfilter.c)
    if(UA_ENABLE_PARSING)
        add_example(client_event_filter_queries events/client_filter_queries.c)
    endif()
endif()

if(UA_ENABLE_JSON_ENCODING)
    add_example(server_json_config server_json_config.c)
endif()

if(UA_ENABLE_HISTORIZING)
    add_example(tutorial_server_historicaldata tutorial_server_historicaldata.c)
    add_example(tutorial_server_historicaldata_circular tutorial_server_historicaldata_circular.c)
endif()

if(UA_ENABLE_ENCRYPTION OR
   UA_ENABLE_ENCRYPTION STREQUAL "MBEDTLS" OR
   UA_ENABLE_ENCRYPTION STREQUAL "OPENSSL")
    add_example(server_encryption encryption/server_encryption.c)
    add_example(server_encryption_filestore encryption/server_encryption_filestore.c)
    add_example(client_encryption encryption/client_encryption.c)
    target_include_directories(server_encryption PRIVATE "${PROJECT_SOURCE_DIR}/examples")
    target_include_directories(client_encryption PRIVATE "${PROJECT_SOURCE_DIR}/examples")
    if(UA_ENABLE_TPM2_KEYSTORE)
        add_example(server_encryption_tpm_keystore encryption/server_encryption_tpm_keystore.c)
        add_example(client_encryption_tpm_keystore encryption/client_encryption_tpm_keystore.c)
        target_link_libraries(server_encryption_tpm_keystore tpm2_pkcs11 ssl crypto)
        target_link_libraries(client_encryption_tpm_keystore tpm2_pkcs11 ssl crypto)
    endif()
endif()

if(UA_ENABLE_NODEMANAGEMENT)
    add_example(access_control_server access_control/server_access_control.c)
    add_example(access_control_client access_control/client_access_control.c)
    if(UA_ENABLE_ENCRYPTION OR
       UA_ENABLE_ENCRYPTION STREQUAL "MBEDTLS" OR
       UA_ENABLE_ENCRYPTION STREQUAL "OPENSSL")
        add_example(access_control_client_encrypt access_control/client_access_control_encrypt.c)
    endif()
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
    add_example(tutorial_pubsub_subscribe pubsub/tutorial_pubsub_subscribe.c)
    add_example(server_pubsub_publish_on_demand pubsub/server_pubsub_publisher_on_demand.c)
    add_example(server_pubsub_publisher_iop pubsub/server_pubsub_publisher_iop.c)
    add_example(pubsub_subscribe_standalone_dataset pubsub/pubsub_subscribe_standalone_dataset.c)

    add_example(server_pubsub_publish_rt_offsets pubsub_realtime/server_pubsub_publish_rt_offsets.c)
    add_example(server_pubsub_subscribe_rt_offsets pubsub_realtime/server_pubsub_subscribe_rt_offsets.c)

    ## Apple does not implement timer functions required by one of the examples
    ## And apple does not implement librt posix extension
    if(UA_ARCHITECTURE_POSIX AND NOT APPLE)
        add_example(server_pubsub_publish_rt_state_machine
                    pubsub_realtime/server_pubsub_publish_rt_state_machine.c)
        target_link_libraries(server_pubsub_publish_rt_state_machine "rt")
        add_example(server_pubsub_subscribe_rt_state_machine
                    pubsub_realtime/server_pubsub_subscribe_rt_state_machine.c)
        target_link_libraries(server_pubsub_subscribe_rt_state_machine "rt")
    endif()

    if(UA_ENABLE_ENCRYPTION_MBEDTLS)
        add_example(pubsub_publish_encrypted pubsub/pubsub_publish_encrypted.c)
        add_example(pubsub_subscribe_encrypted pubsub/pubsub_subscribe_encrypted.c)
        if(UA_ENABLE_TPM2_SECURITY)
            add_example(pubsub_publish_encrypted_tpm pubsub/pubsub_publish_encrypted_tpm.c)
            add_example(pubsub_subscribe_encrypted_tpm pubsub/pubsub_subscribe_encrypted_tpm.c)
        endif()
        if(UA_ENABLE_TPM2_KEYSTORE)
            add_example(pubsub_publish_encrypted_tpm_keystore
                        pubsub/pubsub_publish_encrypted_tpm_keystore.c)
            add_example(pubsub_subscribe_encrypted_tpm_keystore
                        pubsub/pubsub_subscribe_encrypted_tpm_keystore.c)
            target_link_libraries(pubsub_publish_encrypted_tpm_keystore tpm2_pkcs11 ssl crypto)
            target_link_libraries(pubsub_subscribe_encrypted_tpm_keystore tpm2_pkcs11 ssl crypto)
        endif()
        if(UA_ENABLE_PUBSUB_SKS)
            add_subdirectory(pubsub/sks)
        endif()
    endif()

    if(UA_ENABLE_MQTT)
        add_example(tutorial_pubsub_mqtt_publish pubsub/tutorial_pubsub_mqtt_publish.c)
        add_example(tutorial_pubsub_mqtt_subscribe pubsub/tutorial_pubsub_mqtt_subscribe.c)
    endif()

    if(UA_ENABLE_PUBSUB_FILE_CONFIG)
        add_example(server_pubsub_file_configuration pubsub/server_pubsub_file_configuration.c)
    endif()

    add_example(server_pubsub_subscribe_custom_monitoring
                pubsub/server_pubsub_subscribe_custom_monitoring.c)
endif()

###########################
# Nodeser Loader Examples #
###########################

if(UA_ENABLE_NODESETLOADER)
    add_subdirectory(nodeset_loader)
endif()

if(UA_ENABLE_NODESET_INJECTOR)
    set(UA_NODESETINJECTOR_SOURCE_FILES ${UA_NODESETINJECTOR_SOURCE_FILES} PARENT_SCOPE)
endif()
