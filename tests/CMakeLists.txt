if(MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W1") # Reduce warnings level
else()
    add_compile_options(-Wno-unused-variable -Wno-unused-function)
endif()

# Load CMake Packages
find_package(Check REQUIRED)
set(LIBS ${CHECK_LIBRARIES} ${open62541_LIBRARIES})
if(NOT WIN32 AND NOT APPLE AND NOT (CMAKE_HOST_SYSTEM_NAME MATCHES "OpenBSD"))
  list(APPEND LIBS subunit)
endif()

include_directories(${CHECK_INCLUDE_DIR})
#find_package(Threads REQUIRED)
if(NOT MSVC AND UA_ENABLE_UNIT_TESTS_MEMCHECK)
    find_package(Valgrind REQUIRED)
endif()

get_property(open62541_BUILD_INCLUDE_DIRS TARGET open62541 PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
include_directories(${open62541_BUILD_INCLUDE_DIRS})
# ua_server_internal.h
include_directories("${PROJECT_SOURCE_DIR}/src")
include_directories("${PROJECT_SOURCE_DIR}/src/server")
include_directories("${PROJECT_SOURCE_DIR}/arch/common")
# testing_clock.h
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/testing-plugins")
# #include <src_generated/<...>.h>
include_directories("${PROJECT_BINARY_DIR}")
include_directories("${PROJECT_BINARY_DIR}/../plugins")

if(UA_ENABLE_ENCRYPTION_MBEDTLS OR UA_ENABLE_PUBSUB_ENCRYPTION)
    # mbedtls includes
    include_directories(${MBEDTLS_INCLUDE_DIRS})
endif()

if(UA_ENABLE_ENCRYPTION_OPENSSL)
    # openssl includes
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

if(UA_ENABLE_ENCRYPTION_LIBRESSL)
    # openssl includes
    include_directories(${LIBRESSL_INCLUDE_DIR})
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "OpenBSD")
    include_directories(AFTER /usr/local/include)
    link_directories(AFTER /usr/local/lib)
    add_definitions(-Wno-gnu-zero-variadic-macro-arguments)
endif()

#############################
# Compiled binaries folders #
#############################

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/tests)
if (MSVC)
    set(TESTS_BINARY_DIR ${CMAKE_BINARY_DIR}/bin/tests/${CMAKE_BUILD_TYPE})
else()
    set(TESTS_BINARY_DIR ${CMAKE_BINARY_DIR}/bin/tests)
endif()

# Use additional plugins for testing
set(test_plugin_sources
    ${PROJECT_SOURCE_DIR}/tests/testing-plugins/testing_clock.c
    ${PROJECT_SOURCE_DIR}/tests/testing-plugins/testing_policy.c
    ${PROJECT_SOURCE_DIR}/tests/testing-plugins/testing_networklayers.c)

if(UA_ENABLE_PUBSUB)
    # Add ethernet_config.h to ensure the tests are rebuild when it changes
    list(APPEND test_plugin_sources ${PROJECT_SOURCE_DIR}/tests/pubsub/ethernet_config.h)
endif()

if(UA_NAMESPACE_ZERO STREQUAL "FULL")
    set(NODESET_COMPILER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/src_generated/tests")

    # Generate namespace for interfaces test
    ua_generate_nodeset_and_datatypes(
        NAME "tests-interfaces"
        OUTPUT_DIR "${NODESET_COMPILER_OUTPUT_DIR}"
        FILE_NS "${CMAKE_CURRENT_SOURCE_DIR}/server/interface-testmodel.xml"
        INTERNAL
    )
endif()

add_library(open62541-testplugins OBJECT ${test_plugin_sources})
add_dependencies(open62541-testplugins open62541)
target_compile_definitions(open62541-testplugins PRIVATE -DUA_DYNAMIC_LINKING_EXPORT)

# Unit Test Definition Macro
# For now we need to disable the libc freeres. See https://github.com/open62541/open62541/pull/1003#issuecomment-315045143
# This also requires to disable the phtread cache with no-nptl-pthread-stackcache
set(VALGRIND_FLAGS --quiet --trace-children=yes --leak-check=full --run-libc-freeres=yes --sim-hints=no-nptl-pthread-stackcache --track-fds=yes)
macro(add_test_valgrind TEST_NAME)
    if(UA_ENABLE_UNIT_TESTS_MEMCHECK)
        if(MSVC)
            add_test(${TEST_NAME} drmemory -batch -exit_code_if_errors 1 -results_to_stderr -summary -- ${ARGN})
        else()
            set(VALGRIND_LOG ${TESTS_BINARY_DIR}/${TEST_NAME}.log)
            set(VALGRIND_CMD valgrind --error-exitcode=1 --suppressions=${PROJECT_SOURCE_DIR}/tools/valgrind_suppressions.supp ${VALGRIND_FLAGS} --log-file=${VALGRIND_LOG} ${ARGN})
            add_test(${TEST_NAME} ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/valgrind_check_error.py ${VALGRIND_LOG} ${VALGRIND_CMD})
        endif()
    else()
        add_test(${TEST_NAME} ${ARGN})
    endif()
    if(UA_BUILD_FUZZING_CORPUS)
        target_sources(check_${TEST_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/tests/fuzz/ua_debug_dump_pkgs_file.c)
        file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/corpus/${TEST_NAME})
        target_compile_definitions(check_${TEST_NAME} PRIVATE UA_CORPUS_OUTPUT_DIR="${PROJECT_BINARY_DIR}/corpus/${TEST_NAME}")
    endif()
endmacro()

macro(add_test_no_valgrind TEST_NAME)
    if(NOT UA_ENABLE_UNIT_TESTS_MEMCHECK)
        add_test(${TEST_NAME} ${ARGN})
    endif()
    if(UA_BUILD_FUZZING_CORPUS)
        target_sources(check_${TEST_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/tests/fuzz/ua_debug_dump_pkgs_file.c)
        file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/corpus/${TEST_NAME})
        target_compile_definitions(check_${TEST_NAME} PRIVATE UA_CORPUS_OUTPUT_DIR="${PROJECT_BINARY_DIR}/corpus/${TEST_NAME}")
    endif()
endmacro()

add_custom_target(test-verbose COMMAND ${CMAKE_CTEST_COMMAND} --verbose)

# the unit test are built directly on the open62541 object files. so they can
# access symbols that are hidden/not exported to the shared library

function(ua_add_test test_path_relative)
    string(REPLACE "." ";" PARTS ${test_path_relative})
    list(POP_FRONT PARTS PATH_WITHOUT_EXTENSION)
    string(REPLACE "/" ";" QUALIFIED_PATHNAME ${PATH_WITHOUT_EXTENSION})
    list(POP_BACK QUALIFIED_PATHNAME TEST_NAME)

    add_executable(${TEST_NAME} ${test_path_relative} ${ARGN} $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-plugins> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(${TEST_NAME} ${LIBS})
    add_test_valgrind(${TEST_NAME} ${TESTS_BINARY_DIR}/${TEST_NAME})
    if(UA_ENABLE_NODESET_INJECTOR)
        set(UA_NODESETINJECTOR_TEST_NAMES ${TEST_NAME} ${UA_NODESETINJECTOR_TEST_NAMES} PARENT_SCOPE)
    endif()
endfunction()

ua_add_test(check_types_builtin.c)
ua_add_test(check_ziptree.c)

if(UA_ENABLE_JSON_ENCODING)
    ua_add_test(check_cj5.c)
    ua_add_test(check_types_builtin_json.c)

    if(UA_ENABLE_PUBSUB)
        ua_add_test(pubsub/check_pubsub_encoding_json.c)
        ua_add_test(pubsub/check_pubsub_publish_json.c)
    endif()
endif()

if(UA_ENABLE_XML_ENCODING)
    ua_add_test(check_types_builtin_xml.c)
endif()

ua_add_test(check_types_memory.c)
ua_add_test(check_types_range.c)

if(UA_ENABLE_PARSING)
    ua_add_test(check_types_parse.c)
    if(UA_ENABLE_SUBSCRIPTIONS_EVENTS)
        ua_add_test(server/check_eventfilter_parser.c)
    endif()
endif()

ua_add_test(check_types_custom.c)
ua_add_test(check_chunking.c)
ua_add_test(check_utils.c)
ua_add_test(check_kvm_utils.c)
ua_add_test(check_securechannel.c)
ua_add_test(check_timer.c)
ua_add_test(check_eventloop.c)
ua_add_test(check_eventloop_tcp.c)
ua_add_test(check_eventloop_udp.c)
ua_add_test(check_eventloop_interrupt.c)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" AND NOT UA_ENABLE_UNIT_TESTS_MEMCHECK)
    # Requires raw socket capability, currently not possible with valgrind
    ua_add_test(check_eventloop_eth.c)
endif()

if(UA_ENABLE_MQTT)
    ua_add_test(check_eventloop_mqtt.c)
endif()

# Test Server

ua_add_test(server/check_accesscontrol.c)
ua_add_test(server/check_services_view.c)
ua_add_test(server/check_services_attributes.c)
ua_add_test(server/check_services_nodemanagement.c)
ua_add_test(server/check_server_callbacks.c)

add_executable(check_server_password server/check_server_password.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-plugins> $<TARGET_OBJECTS:open62541-testplugins>)
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries(check_server_password ${LIBS} -lcrypt)
else()
target_link_libraries(check_server_password ${LIBS})
endif()
add_test_valgrind(server_password ${TESTS_BINARY_DIR}/check_server_password)

if (UA_MULTITHREADING GREATER_EQUAL 100)
    ua_add_test(multithreading/check_mt_addVariableNode.c)
    ua_add_test(multithreading/check_mt_addVariableTypeNode.c)
    ua_add_test(multithreading/check_mt_addObjectNode.c)
    ua_add_test(multithreading/check_mt_readValueAttribute.c)
    ua_add_test(multithreading/check_mt_writeValueAttribute.c)
    ua_add_test(multithreading/check_mt_readWriteDelete.c)
    ua_add_test(multithreading/check_mt_readWriteDeleteCallback.c)
    ua_add_test(multithreading/check_mt_addDeleteObject.c)
    ua_add_test(server/check_server_asyncop.c)
endif()

if(UA_ENABLE_METHODCALLS)
  ua_add_test(server/check_services_call.c)
endif()

if(UA_ENABLE_SUBSCRIPTIONS)
  ua_add_test(server/check_services_subscriptions.c)
  ua_add_test(server/check_monitoreditem_filter.c)
if(UA_ENABLE_SUBSCRIPTIONS_EVENTS)
  ua_add_test(server/check_subscription_events.c)
  ua_add_test(server/check_subscription_events_local.c)
  ua_add_test(server/check_subscription_event_filter.c)
endif()
endif()

ua_add_test(server/check_nodestore.c)

if(UA_ENABLE_HISTORIZING)
    ua_add_test(server/check_server_historical_data.c)
    ua_add_test(server/check_server_historical_data_circular.c)
endif()

ua_add_test(server/check_session.c)
ua_add_test(server/check_server.c)
ua_add_test(server/check_server_jobs.c)
ua_add_test(server/check_server_userspace.c)
ua_add_test(server/check_node_inheritance.c)

if(UA_ENABLE_SUBSCRIPTIONS)
  ua_add_test(server/check_local_monitored_item.c)
endif()

if(UA_ENABLE_PUBSUB)
    ua_add_test(pubsub/check_pubsub_encoding.c)
    ua_add_test(pubsub/check_pubsub_encoding_custom.c)
    ua_add_test(pubsub/check_pubsub_pds.c)
    ua_add_test(pubsub/check_pubsub_connection_udp.c)
    ua_add_test(pubsub/check_pubsub_publish.c)
    ua_add_test(pubsub/check_pubsub_get_state.c)
    ua_add_test(pubsub/check_pubsub_udp_unicast.c)
    ua_add_test(pubsub/check_pubsub_publisherid.c)

    #Link libraries for executing subscriber unit test
    ua_add_test(pubsub/check_pubsub_subscribe.c)
    ua_add_test(pubsub/check_pubsub_publishspeed.c)
    ua_add_test(pubsub/check_pubsub_config_freeze.c)
    ua_add_test(pubsub/check_pubsub_publish_rt_levels.c)
    ua_add_test(pubsub/check_pubsub_subscribe_config_freeze.c)
    ua_add_test(pubsub/check_pubsub_subscribe_rt_levels.c)
    ua_add_test(pubsub/check_pubsub_multiple_subscribe_rt_levels.c)

    if(UA_ENABLE_PUBSUB_ENCRYPTION)
        ua_add_test(pubsub/check_pubsub_encryption.c)
        ua_add_test(pubsub/check_pubsub_encryption_aes256.c)
        ua_add_test(pubsub/check_pubsub_decryption.c)
        ua_add_test(pubsub/check_pubsub_subscribe_encrypted.c)
        ua_add_test(pubsub/check_pubsub_encrypted_rt_levels.c)
        if(UA_ENABLE_PUBSUB_SKS)
            ua_add_test(pubsub/check_pubsub_sks_keystorage.c)
            ua_add_test(pubsub/check_pubsub_sks_push.c)
            ua_add_test(pubsub/check_pubsub_sks_pull.c)
            ua_add_test(pubsub/check_pubsub_sks_securitygroups.c)
            ua_add_test(pubsub/check_pubsub_sks_client.c)
        endif()
    endif()

    if(UA_ENABLE_PUBSUB_MONITORING)
       ua_add_test(pubsub/check_pubsub_subscribe_msgrcvtimeout.c)
    endif()

    if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        ua_add_test(pubsub/check_pubsub_connection_ethernet.c)
        ua_add_test(pubsub/check_pubsub_publish_ethernet.c)
        ua_add_test(pubsub/check_pubsub_connection_ethernet_etf.c)
        ua_add_test(pubsub/check_pubsub_publish_ethernet_etf.c)
        if(LIB_BPF)
            ua_add_test(pubsub/check_pubsub_connection_xdp.c)
        endif()
    endif()

    if(UA_ENABLE_PUBSUB_INFORMATIONMODEL)
        ua_add_test(pubsub/check_pubsub_informationmodel.c)
        ua_add_test(pubsub/check_pubsub_informationmodel_methods.c)
    endif()
    if(UA_ENABLE_MQTT)
        ua_add_test(pubsub/check_pubsub_connection_mqtt.c)
        ua_add_test(pubsub/check_pubsub_mqtt.c)
    endif()
    if(UA_ENABLE_PUBSUB_FILE_CONFIG)
        ua_add_test(pubsub/check_pubsub_configuration.c)
    endif()
endif()

ua_add_test(server/check_server_readspeed.c)
ua_add_test(server/check_server_speed_addnodes.c)

if(UA_ENABLE_SUBSCRIPTIONS)
    ua_add_test(server/check_server_monitoringspeed.c)
endif()

if(UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS)
    ua_add_test(server/check_server_alarmsconditions.c)
endif()

if(UA_ENABLE_ASYNCOPERATIONS)
    ua_add_test(server/check_server_asyncop.c)
endif()

ua_add_test(server/check_server_reverseconnect.c)

# Test Client

ua_add_test(client/check_client.c)
ua_add_test(client/check_client_discovery.c)
ua_add_test(client/check_activateSession.c)
ua_add_test(client/check_activateSessionAsync.c)
ua_add_test(client/check_client_securechannel.c)
ua_add_test(client/check_client_async.c)
ua_add_test(client/check_client_async_connect.c)
ua_add_test(client/check_client_highlevel.c)

if(UA_ENABLE_SUBSCRIPTIONS)
  ua_add_test(client/check_client_subscriptions.c)
  ua_add_test(client/check_subscriptionWithactivateSession.c)
endif()

if(UA_ENABLE_HISTORIZING)
    ua_add_test(client/check_client_historical_data.c)
endif()

# Test Encryption and Authentication

if(UA_ENABLE_ENCRYPTION)
    ua_add_test(client/check_client_encryption.c)

    if(UA_ENABLE_ENCRYPTION_OPENSSL OR UA_ENABLE_ENCRYPTION_MBEDTLS)
        ua_add_test(client/check_client_authentication.c)
    else()
        MESSAGE(WARNING "Certificate authentication with LibreSSL as crypto backend is not supported.")
    endif()

    if(UA_ENABLE_DISCOVERY)
        ua_add_test(server/check_discovery.c)
    endif()
endif()

if(UA_ENABLE_ENCRYPTION_MBEDTLS)
    ua_add_test(encryption/check_encryption_basic128rsa15.c)
    ua_add_test(encryption/check_encryption_basic256.c)
    ua_add_test(encryption/check_encryption_basic256sha256.c)
    ua_add_test(encryption/check_encryption_aes128sha256rsaoaep.c)
    ua_add_test(encryption/check_encryption_aes256sha256rsapss.c)
    ua_add_test(encryption/check_username_connect_none.c)
    ua_add_test(encryption/check_encryption_key_password.c)
    ua_add_test(encryption/check_cert_generation.c)
endif()

if(UA_ENABLE_ENCRYPTION_MBEDTLS AND UA_ENABLE_CERT_REJECTED_DIR)
    ua_add_test(encryption/check_save_rejected_cert.c)
endif()

if(UA_ENABLE_ENCRYPTION_OPENSSL OR UA_ENABLE_ENCRYPTION_LIBRESSL)
    ua_add_test(encryption/check_encryption_basic128rsa15.c)
    ua_add_test(encryption/check_encryption_basic256.c)
    ua_add_test(encryption/check_encryption_basic256sha256.c)
    ua_add_test(encryption/check_encryption_aes128sha256rsaoaep.c)
    ua_add_test(encryption/check_encryption_aes256sha256rsapss.c)
    ua_add_test(encryption/check_encryption_key_password.c)
    ua_add_test(encryption/check_cert_generation.c)
    ua_add_test(encryption/check_username_connect_none.c)
endif()

# Tests for Nodeset Compiler
add_subdirectory(nodeset-compiler)

# Tests for Nodeset Loader
if(UA_ENABLE_NODESETLOADER)
    add_subdirectory(nodeset-loader)
endif()

# Tests for interfaces
if(UA_NAMESPACE_ZERO STREQUAL "FULL")
    ua_add_test(server/check_interfaces.c ${NODESET_COMPILER_OUTPUT_DIR}/namespace_tests_interfaces_generated.c)
    target_include_directories(check_interfaces PRIVATE ${NODESET_COMPILER_OUTPUT_DIR})
endif()

if(UA_ENABLE_NODESET_INJECTOR)
    set(UA_NODESETINJECTOR_TEST_NAMES ${UA_NODESETINJECTOR_TEST_NAMES} PARENT_SCOPE)
endif()
