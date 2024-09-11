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
set(test_plugin_sources ${PROJECT_SOURCE_DIR}/arch/network_tcp.c
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

# Workaround some clang warnings in the uni tests
if((NOT ${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD") AND (CMAKE_COMPILER_IS_GNUCC OR "x${CMAKE_C_COMPILER_ID}" STREQUAL "xClang"))
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

add_executable(check_types_builtin check_types_builtin.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_types_builtin ${LIBS})
add_test_valgrind(types_builtin ${TESTS_BINARY_DIR}/check_types_builtin)

if(UA_ENABLE_JSON_ENCODING)
    add_executable(check_types_builtin_json check_types_builtin_json.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_types_builtin_json ${LIBS})
    add_test_valgrind(types_builtin_json ${TESTS_BINARY_DIR}/check_types_builtin_json)

    if(UA_ENABLE_PUBSUB)
        add_executable(check_pubsub_encoding_json pubsub/check_pubsub_encoding_json.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
        target_link_libraries(check_pubsub_encoding_json ${LIBS})
        add_test_valgrind(pubsub_encoding_json ${TESTS_BINARY_DIR}/check_pubsub_encoding_json)

        add_executable(check_pubsub_publish_json pubsub/check_pubsub_publish_json.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
        target_link_libraries(check_pubsub_publish_json ${LIBS})
        add_test_valgrind(pubsub_publish_json ${TESTS_BINARY_DIR}/check_pubsub_publish_json)
    endif()
endif()

add_executable(check_types_memory check_types_memory.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_types_memory ${LIBS})
add_test_valgrind(types_memory ${TESTS_BINARY_DIR}/check_types_memory)

add_executable(check_types_range check_types_range.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_types_range ${LIBS})
add_test_valgrind(types_range ${TESTS_BINARY_DIR}/check_types_range)

if(UA_ENABLE_PARSING)
    add_executable(check_types_parse check_types_parse.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_types_parse ${LIBS})
    add_test_valgrind(types_parse ${TESTS_BINARY_DIR}/check_types_parse)
endif()

add_executable(check_types_custom check_types_custom.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_types_custom ${LIBS})
add_test_valgrind(types_custom ${TESTS_BINARY_DIR}/check_types_custom)

add_executable(check_chunking check_chunking.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_chunking ${LIBS})
add_test_valgrind(chunking ${TESTS_BINARY_DIR}/check_chunking)

add_executable(check_utils check_utils.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_utils ${LIBS})
add_test_valgrind(utils ${TESTS_BINARY_DIR}/check_utils)

add_executable(check_securechannel check_securechannel.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_securechannel ${LIBS})
add_test_valgrind(securechannel ${TESTS_BINARY_DIR}/check_securechannel)

add_executable(check_timer check_timer.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_timer ${LIBS})
add_test_valgrind(timer ${TESTS_BINARY_DIR}/check_timer)

# Test Server

add_executable(check_accesscontrol server/check_accesscontrol.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_accesscontrol ${LIBS})
add_test_valgrind(accesscontrol ${TESTS_BINARY_DIR}/check_accesscontrol)

add_executable(check_services_view server/check_services_view.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_services_view ${LIBS})
add_test_valgrind(services_view ${TESTS_BINARY_DIR}/check_services_view)

add_executable(check_services_attributes server/check_services_attributes.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_services_attributes ${LIBS})
add_test_valgrind(services_attributes ${TESTS_BINARY_DIR}/check_services_attributes)

add_executable(check_services_nodemanagement server/check_services_nodemanagement.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_services_nodemanagement ${LIBS})
add_test_valgrind(services_nodemanagement ${TESTS_BINARY_DIR}/check_services_nodemanagement)

add_executable(check_server_callbacks server/check_server_callbacks.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_server_callbacks ${LIBS})
add_test_valgrind(server_callbacks ${TESTS_BINARY_DIR}/check_server_callbacks)

add_executable(check_server_password server/check_server_password.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries(check_server_password ${LIBS} -lcrypt)
else()
target_link_libraries(check_server_password ${LIBS})
endif()
add_test_valgrind(server_password ${TESTS_BINARY_DIR}/check_server_password)

if (UA_MULTITHREADING GREATER_EQUAL 100)
    add_executable(check_mt_addVariableNode multithreading/check_mt_addVariableNode.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_mt_addVariableNode ${LIBS})
    add_test_valgrind(mt_addVariableNode ${TESTS_BINARY_DIR}/check_mt_addVariableNode)

    add_executable(check_mt_addVariableTypeNode multithreading/check_mt_addVariableTypeNode.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_mt_addVariableTypeNode ${LIBS})
    add_test_valgrind(mt_addVariableTypeNode ${TESTS_BINARY_DIR}/check_mt_addVariableTypeNode)

    add_executable(check_mt_addObjectNode multithreading/check_mt_addObjectNode.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_mt_addObjectNode ${LIBS})
    add_test_valgrind(mt_addObjectNode ${TESTS_BINARY_DIR}/check_mt_addObjectNode)

    add_executable(check_mt_readValueAttribute multithreading/check_mt_readValueAttribute.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_mt_readValueAttribute ${LIBS})
    add_test_valgrind(mt_readValueAttribute ${TESTS_BINARY_DIR}/check_mt_readValueAttribute)

    add_executable(check_mt_writeValueAttribute multithreading/check_mt_writeValueAttribute.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_mt_writeValueAttribute ${LIBS})
    add_test_valgrind(mt_writeValueAttribute ${TESTS_BINARY_DIR}/check_mt_readWriteDelete)

    add_executable(check_mt_readWriteDelete multithreading/check_mt_readWriteDelete.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_mt_readWriteDelete ${LIBS})
    add_test_valgrind(mt_readWriteDelete ${TESTS_BINARY_DIR}/check_mt_readWriteDelete)

    add_executable(check_mt_readWriteDeleteCallback multithreading/check_mt_readWriteDeleteCallback.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_mt_readWriteDeleteCallback ${LIBS})
    add_test_valgrind(mt_readWriteDeleteCallback ${TESTS_BINARY_DIR}/check_mt_readWriteDeleteCallback)

    add_executable(check_mt_addDeleteObject multithreading/check_mt_addDeleteObject.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_mt_addDeleteObject ${LIBS})
    add_test_valgrind(mt_addDeleteObject ${TESTS_BINARY_DIR}/check_mt_addDeleteObject)

    add_executable(check_server_asyncop server/check_server_asyncop.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_server_asyncop ${LIBS})
    add_test_valgrind(server_asyncop ${TESTS_BINARY_DIR}/check_server_asyncop)
endif()

if(UA_ENABLE_METHODCALLS)
  add_executable(check_services_call server/check_services_call.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
  target_link_libraries(check_services_call ${LIBS})
  add_test_valgrind(services_call ${TESTS_BINARY_DIR}/check_services_call)
endif()

if(UA_ENABLE_SUBSCRIPTIONS)
  add_executable(check_services_subscriptions server/check_services_subscriptions.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
  target_link_libraries(check_services_subscriptions ${LIBS})
  add_test_valgrind(services_subscriptions ${TESTS_BINARY_DIR}/check_services_subscriptions)

  add_executable(check_monitoreditem_filter server/check_monitoreditem_filter.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
  target_link_libraries(check_monitoreditem_filter ${LIBS})
  add_test_valgrind(monitoreditem_filter ${TESTS_BINARY_DIR}/check_monitoreditem_filter)

  add_executable(check_subscription_events server/check_subscription_events.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
  target_link_libraries(check_subscription_events ${LIBS})
  add_test_valgrind(subscription_events ${TESTS_BINARY_DIR}/check_subscription_events)

  add_executable(check_subscription_event_filter server/check_subscription_event_filter.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
  target_link_libraries(check_subscription_event_filter ${LIBS})
  add_test_valgrind(check_subscription_event_filter ${TESTS_BINARY_DIR}/check_subscription_event_filter)
endif()

add_executable(check_nodestore server/check_nodestore.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_nodestore ${LIBS})
add_test_valgrind(nodestore ${TESTS_BINARY_DIR}/check_nodestore)

if(UA_ENABLE_HISTORIZING)
    add_executable(check_server_historical_data server/check_server_historical_data.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_server_historical_data ${LIBS})
    add_test_valgrind(server_historical_data ${TESTS_BINARY_DIR}/check_server_historical_data)
endif()

add_executable(check_session server/check_session.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_session ${LIBS})
add_test_valgrind(session ${TESTS_BINARY_DIR}/check_session)

add_executable(check_server server/check_server.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_server ${LIBS})
add_test_valgrind(server ${TESTS_BINARY_DIR}/check_server)

add_executable(check_server_jobs server/check_server_jobs.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_server_jobs ${LIBS})
add_test_valgrind(server_jobs ${TESTS_BINARY_DIR}/check_server_jobs)

add_executable(check_server_userspace server/check_server_userspace.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_server_userspace ${LIBS})
add_test_valgrind(server_userspace ${TESTS_BINARY_DIR}/check_server_userspace)

add_executable(check_node_inheritance server/check_node_inheritance.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_node_inheritance ${LIBS})
add_test_valgrind(node_inheritance ${TESTS_BINARY_DIR}/check_node_inheritance)

if(UA_ENABLE_SUBSCRIPTIONS)
  add_executable(check_local_monitored_item server/check_local_monitored_item.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
  target_link_libraries(check_local_monitored_item ${LIBS})
  add_test_valgrind(local_monitored_item ${TESTS_BINARY_DIR}/check_local_monitored_item)
endif()

if(UA_ENABLE_DISCOVERY)
    add_executable(check_discovery server/check_discovery.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_discovery ${LIBS})
    add_test_valgrind(discovery ${TESTS_BINARY_DIR}/check_discovery)
endif()

if(UA_ENABLE_PUBSUB)
    add_executable(check_pubsub_encoding pubsub/check_pubsub_encoding.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_pubsub_encoding ${LIBS})
    add_test_valgrind(pubsub_encoding ${TESTS_BINARY_DIR}/check_pubsub_encoding)
    add_executable(check_pubsub_pds pubsub/check_pubsub_pds.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_pubsub_pds ${LIBS})
    add_test_valgrind(pubsub_pds ${TESTS_BINARY_DIR}/check_pubsub_pds)
    add_executable(check_pubsub_connection_udp pubsub/check_pubsub_connection_udp.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_pubsub_connection_udp ${LIBS})
    add_test_valgrind(pubsub_connection_udp ${TESTS_BINARY_DIR}/check_pubsub_connection_udp)
    add_executable(check_pubsub_publish pubsub/check_pubsub_publish.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_pubsub_publish ${LIBS})
    add_test_valgrind(pubsub_publish ${TESTS_BINARY_DIR}/check_pubsub_publish)
    add_executable(check_pubsub_publish_uadp pubsub/check_pubsub_publish_uadp.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_pubsub_publish_uadp ${LIBS})
    add_test_valgrind(pubsub_publish ${TESTS_BINARY_DIR}/check_pubsub_publish_uadp)
    add_executable(check_pubsub_get_state pubsub/check_pubsub_get_state.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_pubsub_get_state ${LIBS})
    add_test_valgrind(check_pubsub_get_state ${TESTS_BINARY_DIR}/check_pubsub_get_state)

    #Link libraries for executing subscriber unit test
    add_executable(check_pubsub_subscribe pubsub/check_pubsub_subscribe.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_pubsub_subscribe ${LIBS})
    add_executable(check_pubsub_publishspeed pubsub/check_pubsub_publishspeed.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_pubsub_publishspeed ${LIBS})
    add_test_valgrind(pubsub_publishspeed ${TESTS_BINARY_DIR}/check_pubsub_publish)
    add_executable(check_pubsub_config_freeze pubsub/check_pubsub_config_freeze.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_pubsub_config_freeze ${LIBS})
    add_test_valgrind(check_pubsub_config_freeze ${TESTS_BINARY_DIR}/check_pubsub_config_freeze)
    add_executable(check_pubsub_publish_rt_levels pubsub/check_pubsub_publish_rt_levels.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_pubsub_publish_rt_levels ${LIBS})
    add_test_valgrind(check_pubsub_publish_rt_levels ${TESTS_BINARY_DIR}/check_pubsub_publish_rt_levels)
    add_executable(check_pubsub_subscribe_config_freeze pubsub/check_pubsub_subscribe_config_freeze.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_pubsub_subscribe_config_freeze ${LIBS})
    add_test_valgrind(check_pubsub_subscribe_config_freeze ${TESTS_BINARY_DIR}/check_pubsub_subscribe_config_freeze)
    add_executable(check_pubsub_subscribe_rt_levels pubsub/check_pubsub_subscribe_rt_levels.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_pubsub_subscribe_rt_levels ${LIBS})
    add_test_valgrind(check_pubsub_subscribe_rt_levels ${TESTS_BINARY_DIR}/check_pubsub_subscribe_rt_levels)
    add_executable(check_pubsub_multiple_subscribe_rt_levels pubsub/check_pubsub_multiple_subscribe_rt_levels.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_pubsub_multiple_subscribe_rt_levels ${LIBS})
    add_test_valgrind(check_pubsub_multiple_subscribe_rt_levels ${TESTS_BINARY_DIR}/check_pubsub_multiple_subscribe_rt_levels)

    add_executable(check_pubsub_multiple_layer pubsub/check_pubsub_multiple_layer.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_pubsub_multiple_layer ${LIBS})
    add_test_valgrind(pubsub_multiple_layer ${TESTS_BINARY_DIR}/check_pubsub_multiple_layer)

    if(UA_ENABLE_PUBSUB_ENCRYPTION)
        add_executable(check_pubsub_encryption pubsub/check_pubsub_encryption.c
            $<TARGET_OBJECTS:open62541-object>
            $<TARGET_OBJECTS:open62541-testplugins>)
        target_link_libraries(check_pubsub_encryption ${LIBS})
        add_test_valgrind(check_pubsub_encryption ${TESTS_BINARY_DIR}/check_pubsub_encryption)

        add_executable(check_pubsub_encryption_aes256 pubsub/check_pubsub_encryption_aes256.c
            $<TARGET_OBJECTS:open62541-object>
            $<TARGET_OBJECTS:open62541-testplugins>)
        target_link_libraries(check_pubsub_encryption_aes256 ${LIBS})
        add_test_valgrind(check_pubsub_encryption_aes256 ${TESTS_BINARY_DIR}/check_pubsub_encryption_aes256)
    
        add_executable(check_pubsub_decryption pubsub/check_pubsub_decryption.c
                $<TARGET_OBJECTS:open62541-object>
                $<TARGET_OBJECTS:open62541-testplugins>)
        target_link_libraries(check_pubsub_decryption ${LIBS})
        add_test_valgrind(check_pubsub_decryption ${TESTS_BINARY_DIR}/check_pubsub_decryption)

        add_executable(check_pubsub_subscribe_encrypted pubsub/check_pubsub_subscribe_encrypted.c
                $<TARGET_OBJECTS:open62541-object>
                $<TARGET_OBJECTS:open62541-testplugins>)
        target_link_libraries(check_pubsub_subscribe_encrypted ${LIBS})
        add_test_valgrind(check_pubsub_subscribe_encrypted ${TESTS_BINARY_DIR}/check_pubsub_subscribe_encrypted)
    endif()

    if (UA_ENABLE_PUBSUB_MONITORING)
        add_executable(check_pubsub_subscribe_msgrcvtimeout pubsub/check_pubsub_subscribe_msgrcvtimeout.c 
            $<TARGET_OBJECTS:open62541-object>
            $<TARGET_OBJECTS:open62541-testplugins>)
        target_link_libraries(check_pubsub_subscribe_msgrcvtimeout ${LIBS})
        add_test_valgrind(check_pubsub_subscribe_msgrcvtimeout ${TESTS_BINARY_DIR}/check_pubsub_subscribe_msgrcvtimeout)
    endif()

    if(UA_ENABLE_PUBSUB_ETH_UADP)
        add_executable(check_pubsub_connection_ethernet pubsub/check_pubsub_connection_ethernet.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
        target_link_libraries(check_pubsub_connection_ethernet ${LIBS})
        add_test_valgrind(pubsub_connection_ethernet ${TESTS_BINARY_DIR}/check_pubsub_connection_ethernet)
        add_executable(check_pubsub_publish_ethernet pubsub/check_pubsub_publish_ethernet.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
        target_link_libraries(check_pubsub_publish_ethernet ${LIBS})
        add_test_valgrind(pubsub_publish_ethernet ${TESTS_BINARY_DIR}/check_pubsub_publish_ethernet)
        add_executable(check_pubsub_connection_ethernet_etf pubsub/check_pubsub_connection_ethernet_etf.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
        target_link_libraries(check_pubsub_connection_ethernet_etf ${LIBS})
        add_test_valgrind(pubsub_connection_ethernet_etf ${TESTS_BINARY_DIR}/check_pubsub_connection_ethernet_etf)
        add_executable(check_pubsub_publish_ethernet_etf pubsub/check_pubsub_publish_ethernet_etf.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
        target_link_libraries(check_pubsub_publish_ethernet_etf ${LIBS})
        add_test_valgrind(pubsub_publish_ethernet_etf ${TESTS_BINARY_DIR}/check_pubsub_publish_ethernet_etf)
        if(LIB_BPF)
            add_executable(check_pubsub_connection_xdp pubsub/check_pubsub_connection_xdp.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
            target_link_libraries(check_pubsub_connection_xdp ${LIBS})
            add_test_valgrind(pubsub_connection_xdp ${TESTS_BINARY_DIR}/check_pubsub_connection_xdp)
        endif()
    endif()

    if(UA_ENABLE_PUBSUB_INFORMATIONMODEL)
        add_executable(check_pubsub_informationmodel pubsub/check_pubsub_informationmodel.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
        target_link_libraries(check_pubsub_informationmodel ${LIBS})
        add_test_valgrind(check_pubsub_informationmodel ${TESTS_BINARY_DIR}/check_pubsub_informationmodel)
        if(UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS)
            add_executable(check_pubsub_informationmodel_methods pubsub/check_pubsub_informationmodel_methods.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
            target_link_libraries(check_pubsub_informationmodel_methods ${LIBS})
            add_test_valgrind(check_pubsub_informationmodel_methods ${TESTS_BINARY_DIR}/check_pubsub_informationmodel_methods)

        endif()
    endif()
    if(UA_ENABLE_PUBSUB_MQTT)
        if(NOT WIN32)
            add_executable(check_pubsub_connection_mqtt pubsub/check_pubsub_connection_mqtt.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
            target_link_libraries(check_pubsub_connection_mqtt ${LIBS})
            add_test_valgrind(pubsub_connection_mqtt ${TESTS_BINARY_DIR}/check_pubsub_connection_mqtt)
        endif()
    endif()
    if(UA_ENABLE_PUBSUB_FILE_CONFIG)
        add_executable(check_pubsub_configuration pubsub/check_pubsub_configuration.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
        target_link_libraries(check_pubsub_configuration ${LIBS})
        add_test_valgrind(pubsub_configuration ${TESTS_BINARY_DIR}/check_pubsub_configuration)
    endif()
endif()

add_executable(check_server_readspeed server/check_server_readspeed.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_server_readspeed ${LIBS})
add_test_no_valgrind(server_readspeed ${TESTS_BINARY_DIR}/check_server_readspeed)

add_executable(check_server_speed_addnodes server/check_server_speed_addnodes.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_server_speed_addnodes ${LIBS})
add_test_no_valgrind(server_speed_addnodes ${TESTS_BINARY_DIR}/check_server_speed_addnodes)

if(UA_ENABLE_SUBSCRIPTIONS)
    add_executable(check_server_monitoringspeed server/check_server_monitoringspeed.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_server_monitoringspeed ${LIBS})
    add_test_no_valgrind(server_monitoringspeed ${TESTS_BINARY_DIR}/check_server_monitoringspeed)
endif()

if(UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS)
    add_executable(check_server_alarmsconditions server/check_server_alarmsconditions.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_server_alarmsconditions ${LIBS})
    add_test_no_valgrind(check_server_alarmsconditions ${TESTS_BINARY_DIR}/check_server_alarmsconditions)
endif()

if(UA_ENABLE_ASYNCOPERATIONS)
    add_executable(check_server_asyncop server/check_server_asyncop.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_server_asyncop ${LIBS})
    add_test_valgrind(server_asyncop ${TESTS_BINARY_DIR}/check_server_asyncop)
endif()

# Test Client

add_executable(check_client client/check_client.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_client ${LIBS})
add_test_valgrind(client ${TESTS_BINARY_DIR}/check_client)

add_executable(check_client_discovery client/check_client_discovery.c
               $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_client_discovery ${LIBS})
add_test_valgrind(client ${TESTS_BINARY_DIR}/check_client_discovery)

add_executable(check_client_securechannel client/check_client_securechannel.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_client_securechannel ${LIBS})
add_test_valgrind(client_securechannel ${TESTS_BINARY_DIR}/check_client_securechannel)

add_executable(check_client_async client/check_client_async.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_client_async ${LIBS})
add_test_valgrind(client_async ${TESTS_BINARY_DIR}/check_client_async)

add_executable(check_client_async_connect client/check_client_async_connect.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_client_async_connect ${LIBS})
add_test_valgrind(client_async_connect ${TESTS_BINARY_DIR}/check_client_async_connect)

if(UA_ENABLE_SUBSCRIPTIONS)
  add_executable(check_client_subscriptions client/check_client_subscriptions.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
  target_link_libraries(check_client_subscriptions ${LIBS})
  add_test_valgrind(client_subscriptions ${TESTS_BINARY_DIR}/check_client_subscriptions)
endif()

add_executable(check_client_highlevel client/check_client_highlevel.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_client_highlevel ${LIBS})
add_test_valgrind(client_highlevel ${TESTS_BINARY_DIR}/check_client_highlevel)

if(UA_ENABLE_HISTORIZING)
    add_executable(check_client_historical_data client/check_client_historical_data.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_client_historical_data ${LIBS})
    add_test_valgrind(client_historical_data ${TESTS_BINARY_DIR}/check_client_historical_data)
endif()

# Test Encryption

if(UA_ENABLE_ENCRYPTION)
    add_executable(check_client_encryption client/check_client_encryption.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_client_encryption ${LIBS})
    add_test_valgrind(client_encryption ${TESTS_BINARY_DIR}/check_client_encryption)

    add_executable(check_crl_validation encryption/check_crl_validation.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_crl_validation ${LIBS})
    add_test_valgrind(crl_validation ${TESTS_BINARY_DIR}/check_crl_validation)
endif()

if(UA_ENABLE_ENCRYPTION_MBEDTLS)
    add_executable(check_encryption_basic128rsa15 encryption/check_encryption_basic128rsa15.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_encryption_basic128rsa15 ${LIBS})
    add_test_valgrind(encryption_basic128rsa15 ${TESTS_BINARY_DIR}/check_encryption_basic128rsa15)

    add_executable(check_encryption_basic256 encryption/check_encryption_basic256.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_encryption_basic256 ${LIBS})
    add_test_valgrind(encryption_basic256 ${TESTS_BINARY_DIR}/check_encryption_basic256)

    add_executable(check_encryption_basic256sha256 encryption/check_encryption_basic256sha256.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_encryption_basic256sha256 ${LIBS})
    add_test_valgrind(encryption_basic256sha256 ${TESTS_BINARY_DIR}/check_encryption_basic256sha256)

    add_executable(check_encryption_aes128sha256rsaoaep encryption/check_encryption_aes128sha256rsaoaep.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_encryption_aes128sha256rsaoaep ${LIBS})
    add_test_valgrind(encryption_aes128sha256rsaoaep ${TESTS_BINARY_DIR}/check_encryption_aes128sha256rsaoaep)
endif()

if(UA_ENABLE_ENCRYPTION_OPENSSL OR UA_ENABLE_ENCRYPTION_LIBRESSL)
    add_executable(check_encryption_basic128rsa15 encryption/check_encryption_basic128rsa15.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_encryption_basic128rsa15 ${LIBS})
    add_test_valgrind(encryption_basic128rsa15 ${TESTS_BINARY_DIR}/check_encryption_basic128rsa15)

    add_executable(check_encryption_basic256 encryption/check_encryption_basic256.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_encryption_basic256 ${LIBS})
    add_test_valgrind(encryption_basic256 ${TESTS_BINARY_DIR}/check_encryption_basic256)

    add_executable(check_encryption_basic256sha256 encryption/check_encryption_basic256sha256.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_encryption_basic256sha256 ${LIBS})
    add_test_valgrind(encryption_basic256sha256 ${TESTS_BINARY_DIR}/check_encryption_basic256sha256)

    add_executable(check_encryption_aes128sha256rsaoaep encryption/check_encryption_aes128sha256rsaoaep.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_encryption_aes128sha256rsaoaep ${LIBS})
    add_test_valgrind(encryption_aes128sha256rsaoaep ${TESTS_BINARY_DIR}/check_encryption_aes128sha256rsaoaep)

    add_executable(check_cert_generation encryption/check_cert_generation.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_cert_generation ${LIBS})
    add_test_valgrind(check_cert_generation ${TESTS_BINARY_DIR}/check_cert_generation)
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
