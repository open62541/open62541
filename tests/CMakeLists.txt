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

if(APPLE)
    # CLang on Apple complains about many cases like this:
    # /Users/travis/build/open62541/open62541/tests/check_chunking.c:148:22: warning: implicit conversion changes signedness: 'size_t' (aka 'unsigned long') to 'intmax_t' (aka 'long') [-Wsign-conversion]
    #    ck_assert_int_eq(counter,9); //10 chunks allocated - callback called 4 times
    add_definitions(-Wno-sign-conversion)
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

add_definitions(-DUA_sleep_ms=UA_comboSleep)

#############################
# Compiled binaries folders #
#############################

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/tests)
if (MSVC)
    set(TESTS_BINARY_DIR ${CMAKE_BINARY_DIR}/bin/tests/${CMAKE_BUILD_TYPE})
else()
    set(TESTS_BINARY_DIR ${CMAKE_BINARY_DIR}/bin/tests)
endif()


# Use different plugins for testing
set(test_plugin_sources
    ${PROJECT_SOURCE_DIR}/arch/network_tcp.c
    ${PROJECT_SOURCE_DIR}/arch/eventloop_posix.c
    ${PROJECT_SOURCE_DIR}/arch/eventloop_posix_select.c
    ${PROJECT_SOURCE_DIR}/arch/eventloop_posix_epoll.c
    ${PROJECT_SOURCE_DIR}/arch/eventloop_posix_tcp.c
    ${PROJECT_SOURCE_DIR}/arch/eventloop_posix_udp.c
    ${PROJECT_SOURCE_DIR}/arch/eventloop_posix_interrupt.c
    ${PROJECT_SOURCE_DIR}/tests/testing-plugins/testing_clock.c
    ${PROJECT_SOURCE_DIR}/plugins/ua_log_stdout.c
    ${PROJECT_SOURCE_DIR}/plugins/ua_config_default.c
    ${PROJECT_SOURCE_DIR}/plugins/ua_accesscontrol_default.c
    ${PROJECT_SOURCE_DIR}/plugins/ua_nodestore_ziptree.c
    ${PROJECT_SOURCE_DIR}/plugins/ua_nodestore_hashmap.c
    ${PROJECT_SOURCE_DIR}/plugins/crypto/ua_securitypolicy_none.c
    ${PROJECT_SOURCE_DIR}/plugins/crypto/ua_pki_none.c
    ${PROJECT_SOURCE_DIR}/tests/testing-plugins/testing_policy.c
    ${PROJECT_SOURCE_DIR}/tests/testing-plugins/testing_networklayers.c
    )

if(UA_ENABLE_HISTORIZING)
    set(test_plugin_sources ${test_plugin_sources}
        ${PROJECT_SOURCE_DIR}/plugins/historydata/ua_history_data_backend_memory.c
        ${PROJECT_SOURCE_DIR}/plugins/historydata/ua_history_data_gathering_default.c
        ${PROJECT_SOURCE_DIR}/plugins/historydata/ua_history_database_default.c)
endif()

if(UA_ENABLE_ENCRYPTION_MBEDTLS OR UA_ENABLE_PUBSUB_ENCRYPTION)
  list(INSERT test_plugin_sources 0
       ${PROJECT_SOURCE_DIR}/plugins/crypto/mbedtls/securitypolicy_mbedtls_common.h
       ${PROJECT_SOURCE_DIR}/plugins/crypto/mbedtls/securitypolicy_mbedtls_common.c)
endif()

if(UA_ENABLE_PUBSUB_ENCRYPTION)
  list(INSERT test_plugin_sources 1
    ${PROJECT_SOURCE_DIR}/plugins/crypto/mbedtls/securitypolicy_pubsub_aes128ctr.c
    ${PROJECT_SOURCE_DIR}/plugins/crypto/mbedtls/securitypolicy_pubsub_aes256ctr.c)
endif()

if(UA_ENABLE_ENCRYPTION_MBEDTLS)
  list(INSERT test_plugin_sources 1
       ${PROJECT_SOURCE_DIR}/plugins/crypto/mbedtls/ua_securitypolicy_basic128rsa15.c
       ${PROJECT_SOURCE_DIR}/plugins/crypto/mbedtls/ua_securitypolicy_basic256.c
       ${PROJECT_SOURCE_DIR}/plugins/crypto/mbedtls/ua_securitypolicy_basic256sha256.c
       ${PROJECT_SOURCE_DIR}/plugins/crypto/mbedtls/ua_securitypolicy_aes128sha256rsaoaep.c
       ${PROJECT_SOURCE_DIR}/plugins/crypto/mbedtls/ua_pki_mbedtls.c)
endif()

if(UA_ENABLE_ENCRYPTION_OPENSSL OR UA_ENABLE_ENCRYPTION_LIBRESSL)
  list(INSERT test_plugin_sources 0
              ${PROJECT_SOURCE_DIR}/plugins/crypto/openssl/securitypolicy_openssl_common.h
              ${PROJECT_SOURCE_DIR}/plugins/crypto/openssl/securitypolicy_openssl_common.c
              ${PROJECT_SOURCE_DIR}/plugins/crypto/openssl/ua_openssl_basic128rsa15.c
              ${PROJECT_SOURCE_DIR}/plugins/crypto/openssl/ua_openssl_basic256.c
              ${PROJECT_SOURCE_DIR}/plugins/crypto/openssl/ua_openssl_basic256sha256.c
              ${PROJECT_SOURCE_DIR}/plugins/crypto/openssl/ua_openssl_aes128sha256rsaoaep.c
              ${PROJECT_SOURCE_DIR}/plugins/crypto/openssl/ua_openssl_create_certificate.c
              ${PROJECT_SOURCE_DIR}/plugins/crypto/openssl/ua_pki_openssl.c)
endif()

if(UA_ENABLE_PUBSUB)
  list(APPEND test_plugin_sources
              ${PROJECT_SOURCE_DIR}/plugins/ua_pubsub_udp.c)
    if(UA_ENABLE_PUBSUB_ETH_UADP)
        list(APPEND test_plugin_sources ${PROJECT_SOURCE_DIR}/plugins/include/open62541/plugin/pubsub_ethernet.h)
        list(APPEND test_plugin_sources ${PROJECT_SOURCE_DIR}/plugins/ua_pubsub_ethernet.c)

        # Add ethernet_config.h to ensure the tests are rebuild when it changes
        list(APPEND test_plugin_sources ${PROJECT_SOURCE_DIR}/tests/pubsub/ethernet_config.h)
    endif()

    if(UA_ENABLE_PUBSUB_MQTT)
      list(APPEND test_plugin_sources ${PROJECT_SOURCE_DIR}/deps/mqtt-c/include/mqtt_pal.h
           ${PROJECT_SOURCE_DIR}/deps/mqtt-c/include/mqtt.h
           ${PROJECT_SOURCE_DIR}/plugins/include/open62541/plugin/pubsub_mqtt.h
           ${PROJECT_SOURCE_DIR}/plugins/mqtt/ua_mqtt-c_adapter.h
           ${PROJECT_SOURCE_DIR}/deps/mqtt-c/src/mqtt.c
           ${PROJECT_SOURCE_DIR}/deps/mqtt-c/src/mqtt_pal.c
           ${PROJECT_SOURCE_DIR}/plugins/ua_pubsub_mqtt.c
           ${PROJECT_SOURCE_DIR}/plugins/mqtt/ua_mqtt-c_adapter.c)
    endif()
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

add_library(open62541-testplugins OBJECT ${test_plugin_sources} ${PROJECT_SOURCE_DIR}/arch/${UA_ARCHITECTURE}/ua_architecture_functions.c)
add_dependencies(open62541-testplugins open62541)
target_compile_definitions(open62541-testplugins PRIVATE -DUA_DYNAMIC_LINKING_EXPORT)
if(UA_ENABLE_COVERAGE)
    add_coverage(open62541-testplugins)
endif()

# Workaround some clang warnings in the uni tests
if((NOT ${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD") AND (CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang"))
    add_definitions(-Wno-sign-conversion)
endif()

# Unit Test Definition Macro
# For now we need to disable the libc freeres. See https://github.com/open62541/open62541/pull/1003#issuecomment-315045143
# This also requires to disable the phtread cache with no-nptl-pthread-stackcache
set(VALGRIND_FLAGS --quiet --trace-children=yes --leak-check=full --run-libc-freeres=no --sim-hints=no-nptl-pthread-stackcache --track-fds=yes)
macro(add_test_valgrind TEST_NAME)
    if(UA_ENABLE_UNIT_TESTS_MEMCHECK)
        if(MSVC)
            add_test(${TEST_NAME} drmemory -batch -exit_code_if_errors 1 -results_to_stderr -summary -- ${ARGN})
        else()
            set(VALGRIND_LOG ${TESTS_BINARY_DIR}/${TEST_NAME}.log)
            set(VALGRIND_CMD valgrind --error-exitcode=1 --suppressions=${PROJECT_SOURCE_DIR}/tools/valgrind_suppressions.supp ${VALGRIND_FLAGS} --log-file=${VALGRIND_LOG} ${ARGN})
            add_test(${TEST_NAME} ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/valgrind_check_error.py ${VALGRIND_LOG} ${VALGRIND_CMD})
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

    add_executable(${TEST_NAME} ${test_path_relative} $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(${TEST_NAME} ${LIBS})
    add_test_valgrind(${TEST_NAME} ${TESTS_BINARY_DIR}/${TEST_NAME})
endfunction()

ua_add_test(check_types_builtin.c)

if(UA_ENABLE_JSON_ENCODING)
    ua_add_test(check_cj5.c)
    ua_add_test(check_types_builtin_json.c)

    if(UA_ENABLE_PUBSUB)
        ua_add_test(pubsub/check_pubsub_encoding_json.c)
        ua_add_test(pubsub/check_pubsub_publish_json.c)
    endif()
endif()

ua_add_test(check_types_memory.c)
ua_add_test(check_types_range.c)

if(UA_ENABLE_PARSING)
    ua_add_test(check_types_parse.c)
endif()

ua_add_test(check_types_custom.c)
ua_add_test(check_chunking.c)
ua_add_test(check_utils.c)
ua_add_test(check_securechannel.c)
ua_add_test(check_timer.c)
ua_add_test(check_eventloop.c)
ua_add_test(check_eventloop_tcp.c)
ua_add_test(check_eventloop_udp.c)
ua_add_test(check_eventloop_interrupt.c)

# Test Server

ua_add_test(server/check_accesscontrol.c)
ua_add_test(server/check_services_view.c)
ua_add_test(server/check_services_attributes.c)
ua_add_test(server/check_services_nodemanagement.c)
ua_add_test(server/check_server_callbacks.c)

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
  ua_add_test(server/check_subscription_events.c)
  ua_add_test(server/check_subscription_event_filter.c)
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

if(UA_ENABLE_DISCOVERY)
    ua_add_test(server/check_discovery.c)
endif()

if(UA_ENABLE_PUBSUB)
    ua_add_test(pubsub/check_pubsub_encoding.c)
    ua_add_test(pubsub/check_pubsub_pds.c)
    ua_add_test(pubsub/check_pubsub_connection_udp.c)
    ua_add_test(pubsub/check_pubsub_publish.c)
    ua_add_test(pubsub/check_pubsub_publish_uadp.c)
    ua_add_test(pubsub/check_pubsub_get_state.c)
    ua_add_test(pubsub/check_pubsub_publisherid.c)

    #Link libraries for executing subscriber unit test
    ua_add_test(pubsub/check_pubsub_subscribe.c)
    ua_add_test(pubsub/check_pubsub_publishspeed.c)
    ua_add_test(pubsub/check_pubsub_config_freeze.c)
    ua_add_test(pubsub/check_pubsub_publish_rt_levels.c)
    ua_add_test(pubsub/check_pubsub_subscribe_config_freeze.c)
    ua_add_test(pubsub/check_pubsub_subscribe_rt_levels.c)
    ua_add_test(pubsub/check_pubsub_multiple_subscribe_rt_levels.c)
    ua_add_test(pubsub/check_pubsub_multiple_layer.c)

    if(UA_ENABLE_PUBSUB_ENCRYPTION)
        ua_add_test(pubsub/check_pubsub_encryption.c)
        ua_add_test(pubsub/check_pubsub_encryption_aes256.c)
        ua_add_test(pubsub/check_pubsub_decryption.c)
        ua_add_test(pubsub/check_pubsub_subscribe_encrypted.c)
        ua_add_test(pubsub/check_pubsub_encrypted_rt_levels.c)
    endif()

    if (UA_ENABLE_PUBSUB_MONITORING)
        if(NOT UA_ENABLE_PUBSUB_ENCRYPTION) #ToDO: Multiple Receive handling for PubsubEncryption
            ua_add_test(pubsub/check_pubsub_subscribe_msgrcvtimeout.c)
        endif()
    endif()

    if(UA_ENABLE_PUBSUB_ETH_UADP)
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
        if(UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS)
            ua_add_test(pubsub/check_pubsub_informationmodel_methods.c)

        endif()
    endif()
    if(UA_ENABLE_PUBSUB_MQTT)
        if(NOT WIN32)
            ua_add_test(pubsub/check_pubsub_connection_mqtt.c)
        endif()
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

# Test Client

ua_add_test(client/check_client.c)
ua_add_test(client/check_client_securechannel.c)
ua_add_test(client/check_client_async.c)
ua_add_test(client/check_client_async_connect.c)

if(UA_ENABLE_SUBSCRIPTIONS)
  ua_add_test(client/check_client_subscriptions.c)
endif()

ua_add_test(client/check_client_highlevel.c)

if(UA_ENABLE_HISTORIZING)
    ua_add_test(client/check_client_historical_data.c)
endif()

# Test Encryption

if(UA_ENABLE_ENCRYPTION)
    add_executable(check_client_encryption client/check_client_encryption.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_client_encryption ${LIBS})
    add_test_valgrind(client_encryption ${TESTS_BINARY_DIR}/check_client_encryption)
endif()

if(UA_ENABLE_ENCRYPTION_MBEDTLS)
    ua_add_test(encryption/check_encryption_basic128rsa15.c)
    ua_add_test(encryption/check_encryption_basic256.c)
    ua_add_test(encryption/check_encryption_basic256sha256.c)
    ua_add_test(encryption/check_encryption_aes128sha256rsaoaep.c)
endif()

if(UA_ENABLE_ENCRYPTION_OPENSSL OR UA_ENABLE_ENCRYPTION_LIBRESSL)
    ua_add_test(encryption/check_encryption_basic128rsa15.c)
    ua_add_test(encryption/check_encryption_basic256.c)
    ua_add_test(encryption/check_encryption_basic256sha256.c)
    ua_add_test(encryption/check_encryption_aes128sha256rsaoaep.c)
    ua_add_test(encryption/check_cert_generation.c)
endif()

# Tests for Nodeset Compiler
add_subdirectory(nodeset-compiler)

# Tests for interfaces
if(UA_NAMESPACE_ZERO STREQUAL "FULL")
    add_executable(check_interfaces server/check_interfaces.c ${NODESET_COMPILER_OUTPUT_DIR}/namespace_tests_interfaces_generated.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_include_directories(check_interfaces PRIVATE ${NODESET_COMPILER_OUTPUT_DIR})
    target_link_libraries(check_interfaces ${LIBS})
    add_test_valgrind(check_interfaces ${TESTS_BINARY_DIR}/check_interfaces)
endif()
