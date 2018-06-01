# Load CMake Packages
find_package(Check REQUIRED)
set(LIBS ${CHECK_LIBRARIES} ${open62541_LIBRARIES})
include_directories(${CHECK_INCLUDE_DIRS})
find_package(Threads REQUIRED)
if(NOT MSVC AND UA_ENABLE_UNIT_TESTS_MEMCHECK)
    find_package(Valgrind REQUIRED)
endif()

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/deps)
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/src/server)
include_directories(${PROJECT_SOURCE_DIR}/src/pubsub)
include_directories(${PROJECT_SOURCE_DIR}/plugins)
include_directories(${PROJECT_BINARY_DIR}/src_generated)
include_directories(${PROJECT_SOURCE_DIR}/tests/testing-plugins)

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
set(test_plugin_sources ${PROJECT_SOURCE_DIR}/plugins/ua_network_tcp.c
                        ${PROJECT_SOURCE_DIR}/tests/testing-plugins/testing_clock.c
                        ${PROJECT_SOURCE_DIR}/plugins/ua_log_stdout.c
                        ${PROJECT_SOURCE_DIR}/plugins/ua_config_default.c
                        ${PROJECT_SOURCE_DIR}/plugins/ua_accesscontrol_default.c
                        ${PROJECT_SOURCE_DIR}/plugins/ua_pki_certificate.c
                        ${PROJECT_SOURCE_DIR}/plugins/ua_nodestore_default.c
                        ${PROJECT_SOURCE_DIR}/tests/testing-plugins/testing_clock.c
                        ${PROJECT_SOURCE_DIR}/plugins/ua_securitypolicy_none.c
                        ${PROJECT_SOURCE_DIR}/tests/testing-plugins/testing_policy.c
                        ${PROJECT_SOURCE_DIR}/tests/testing-plugins/testing_networklayers.c
)

if(UA_ENABLE_ENCRYPTION)
    set(test_plugin_sources ${test_plugin_sources}
        ${PROJECT_SOURCE_DIR}/plugins/ua_securitypolicy_basic128rsa15.c)
    set(test_plugin_sources ${test_plugin_sources}
        ${PROJECT_SOURCE_DIR}/plugins/ua_securitypolicy_basic256sha256.c)
endif()

add_library(open62541-testplugins OBJECT ${test_plugin_sources})
add_dependencies(open62541-testplugins open62541)
target_compile_definitions(open62541-testplugins PRIVATE -DUA_DYNAMIC_LINKING_EXPORT)

if(NOT MSVC)
    add_definitions(-Wno-deprecated-declarations)
endif()
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
    if (UA_BUILD_FUZZING_CORPUS)
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

add_executable(check_types_memory check_types_memory.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_types_memory ${LIBS})
add_test_valgrind(types_memory ${TESTS_BINARY_DIR}/check_types_memory)

add_executable(check_types_range check_types_range.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_types_range ${LIBS})
add_test_valgrind(types_range ${TESTS_BINARY_DIR}/check_types_range)

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

add_executable(check_services_subscriptions server/check_services_subscriptions.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_services_subscriptions ${LIBS})
add_test_valgrind(services_subscriptions ${TESTS_BINARY_DIR}/check_services_subscriptions)

add_executable(check_monitoreditem_filter server/check_monitoreditem_filter.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_monitoreditem_filter ${LIBS})
add_test_valgrind(monitoreditem_filter ${TESTS_BINARY_DIR}/check_monitoreditem_filter)

add_executable(check_subscription_events server/check_subscription_events.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_subscription_events ${LIBS})
add_test_valgrind(subscription_events ${TESTS_BINARY_DIR}/check_subscription_events)

add_executable(check_nodestore server/check_nodestore.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_nodestore ${LIBS})
add_test_valgrind(nodestore ${TESTS_BINARY_DIR}/check_nodestore)

add_executable(check_session server/check_session.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_session ${LIBS})
add_test_valgrind(session ${TESTS_BINARY_DIR}/check_session)

add_executable(check_server_jobs server/check_server_jobs.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_server_jobs ${LIBS})
add_test_valgrind(server_jobs ${TESTS_BINARY_DIR}/check_server_jobs)

add_executable(check_server_userspace server/check_server_userspace.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_server_userspace ${LIBS})
add_test_valgrind(server_userspace ${TESTS_BINARY_DIR}/check_server_userspace)

add_executable(check_node_inheritance server/check_node_inheritance.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_node_inheritance ${LIBS})
add_test_valgrind(node_inheritance ${TESTS_BINARY_DIR}/check_node_inheritance)

if(UA_ENABLE_DISCOVERY)
    add_executable(check_discovery server/check_discovery.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_discovery ${LIBS})
    add_test_valgrind(discovery ${TESTS_BINARY_DIR}/check_discovery)
endif()

if(UA_ENABLE_PUBSUB)
    add_executable(check_pubsub_encoding pubsub/check_pubsub_encoding.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_pubsub_encoding ${LIBS})
    add_test_valgrind(pubsub_encoding ${TESTS_BINARY_DIR}/check_pubsub_encoding)
    add_executable(check_pubsub_connection_udp pubsub/check_pubsub_connection_udp.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-plugins>)
    target_link_libraries(check_pubsub_connection_udp ${LIBS})
    add_test(NAME check_pubsub_connection_udp COMMAND check_pubsub_connection_udp)
    add_executable(check_pubsub_pds pubsub/check_pubsub_pds.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-plugins>)
    target_link_libraries(check_pubsub_pds ${LIBS})
    add_test(check_pubsub_pds ${TESTS_BINARY_DIR}/check_pubsub_pds)
    add_executable(check_pubsub_publish pubsub/check_pubsub_publish.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-plugins>)
    target_link_libraries(check_pubsub_publish ${LIBS})
    add_test(check_pubsub_publish ${TESTS_BINARY_DIR}/check_pubsub_publish)
    if(UA_ENABLE_PUBSUB_INFORMATIONMODEL)
        add_executable(check_pubsub_informationmodel pubsub/check_pubsub_informationmodel.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-plugins>)
        target_link_libraries(check_pubsub_informationmodel ${LIBS})
        add_test(check_pubsub_informationmodel ${TESTS_BINARY_DIR}/check_pubsub_informationmodel)
    endif()
endif()

add_executable(check_server_readspeed server/check_server_readspeed.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_server_readspeed ${LIBS})
add_test_valgrind(server_readspeed ${TESTS_BINARY_DIR}/check_server_readspeed)

# Test Client

add_executable(check_client client/check_client.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_client ${LIBS})
add_test_valgrind(client ${TESTS_BINARY_DIR}/check_client)

add_executable(check_client_securechannel client/check_client_securechannel.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_client_securechannel ${LIBS})
add_test_valgrind(client_securechannel ${TESTS_BINARY_DIR}/check_client_securechannel)

add_executable(check_client_async client/check_client_async.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_client_async ${LIBS})
add_test_valgrind(client_async ${TESTS_BINARY_DIR}/check_client_async)

add_executable(check_client_async_connect client/check_client_async_connect.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_client_async_connect ${LIBS})
add_test_valgrind(client_async_connect ${TESTS_BINARY_DIR}/check_client_async_connect)

add_executable(check_client_subscriptions client/check_client_subscriptions.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_client_subscriptions ${LIBS})
add_test_valgrind(client_subscriptions ${TESTS_BINARY_DIR}/check_client_subscriptions)

add_executable(check_client_highlevel client/check_client_highlevel.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_client_highlevel ${LIBS})
add_test_valgrind(client_highlevel ${TESTS_BINARY_DIR}/check_client_highlevel)

# Test Encryption

if(UA_ENABLE_ENCRYPTION)
    add_executable(check_encryption_basic128rsa15 encryption/check_encryption_basic128rsa15.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_encryption_basic128rsa15 ${LIBS})
    add_test_valgrind(encryption_basic128rsa15 ${TESTS_BINARY_DIR}/check_encryption_basic128rsa15)

    add_executable(check_encryption_basic256sha256 encryption/check_encryption_basic256sha256.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_encryption_basic256sha256 ${LIBS})
    add_test_valgrind(encryption_basic256sha256 ${TESTS_BINARY_DIR}/check_encryption_basic256sha256)
endif()

# Tests for Nodeset Compiler
add_subdirectory(nodeset-compiler)
