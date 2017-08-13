include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/deps)
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/src/server)
include_directories(${PROJECT_SOURCE_DIR}/plugins)
include_directories(${PROJECT_BINARY_DIR}/src_generated)
include_directories(${CHECK_INCLUDE_DIRS})

#############################
# Compiled binaries folders #
#############################

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/tests)
set(TESTS_BINARY_DIR ${CMAKE_BINARY_DIR}/bin/tests)

# Load CMake Packages
find_package(Check REQUIRED)
find_package(Threads REQUIRED)
if(UA_ENABLE_VALGRIND_UNIT_TESTS)
    find_package(Valgrind REQUIRED)
endif()

set(LIBS subunit ${CHECK_LIBRARIES} ${open62541_LIBRARIES})

# Use different plugins for testing
set(test_plugin_sources ${PROJECT_SOURCE_DIR}/plugins/ua_network_tcp.c
                        ${PROJECT_SOURCE_DIR}/tests/testing_clock.c
                        ${PROJECT_SOURCE_DIR}/plugins/ua_log_stdout.c
                        ${PROJECT_SOURCE_DIR}/plugins/ua_config_standard.c
                        ${PROJECT_SOURCE_DIR}/plugins/ua_accesscontrol_default.c
                        ${PROJECT_SOURCE_DIR}/tests/testing_clock.c)

add_library(open62541-testplugins OBJECT ${test_plugin_sources})
add_dependencies(open62541-testplugins open62541)

# Workaround some clang warnings in the unit tests
if(CMAKE_COMPILER_IS_GNUCC OR "x${CMAKE_C_COMPILER_ID}" STREQUAL "xClang")
    add_definitions(-Wno-sign-conversion)
endif()

# Unit Test Definition Macro
set(VALGRIND_FLAGS --quiet --trace-children=yes --leak-check=full)
macro(add_test_valgrind TEST_NAME)
    if(UA_ENABLE_VALGRIND_UNIT_TESTS)
        add_test(${TEST_NAME} valgrind --error-exitcode=1 --suppressions=${PROJECT_SOURCE_DIR}/tools/valgrind_suppressions.supp ${VALGRIND_FLAGS} ${ARGN})
    else()
        add_test(${TEST_NAME} ${ARGN})
    endif()
endmacro()

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
add_test_valgrind(check_utils ${TESTS_BINARY_DIR}/check_utils)

# Test Server

add_executable(check_services_view check_services_view.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_services_view ${LIBS})
add_test_valgrind(services_view ${TESTS_BINARY_DIR}/check_services_view)

add_executable(check_services_attributes check_services_attributes.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_services_attributes ${LIBS})
add_test_valgrind(services_attributes ${TESTS_BINARY_DIR}/check_services_attributes)

add_executable(check_services_nodemanagement check_services_nodemanagement.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_services_nodemanagement ${LIBS})
add_test_valgrind(services_nodemanagement ${TESTS_BINARY_DIR}/check_services_nodemanagement)

add_executable(check_services_subscriptions check_services_subscriptions.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_services_subscriptions ${LIBS})
add_test_valgrind(check_services_subscriptions ${TESTS_BINARY_DIR}/check_services_subscriptions)

add_executable(check_nodestore check_nodestore.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_nodestore ${LIBS})
add_test_valgrind(nodestore ${TESTS_BINARY_DIR}/check_nodestore)

add_executable(check_session check_session.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_session ${LIBS})
add_test_valgrind(session ${TESTS_BINARY_DIR}/check_session)

add_executable(check_server_jobs check_server_jobs.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_server_jobs ${LIBS})
add_test_valgrind(check_server_jobs ${TESTS_BINARY_DIR}/check_server_jobs)

add_executable(check_server_userspace check_server_userspace.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_server_userspace ${LIBS})
add_test_valgrind(check_server_userspace ${TESTS_BINARY_DIR}/check_server_userspace)

if(UA_ENABLE_DISCOVERY)
    add_executable(check_discovery check_discovery.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    target_link_libraries(check_discovery ${LIBS})
    add_test_valgrind(discovery ${TESTS_BINARY_DIR}/check_discovery)
endif()

# Readspeed server
add_executable(check_server_readspeed check_server_readspeed.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_server_readspeed ${LIBS})
add_test_valgrind(check_server_readspeed ${TESTS_BINARY_DIR}/check_server_readspeed)

# Test server with network dumps from files

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/client_HELOPN.bin
                          ${CMAKE_CURRENT_BINARY_DIR}/client_CLO.bin
                          ${CMAKE_CURRENT_BINARY_DIR}/client_CreateActivateSession.bin
                          ${CMAKE_CURRENT_BINARY_DIR}/client_Browse.bin
                          ${CMAKE_CURRENT_BINARY_DIR}/client_Read.bin
                          ${CMAKE_CURRENT_BINARY_DIR}/client_Write.bin
                  PRE_BUILD
                  COMMAND python ${PROJECT_SOURCE_DIR}/tools/hex2bin.py
                                 ${CMAKE_CURRENT_SOURCE_DIR}/dumps/client_HELOPN.hex
                                 ${CMAKE_CURRENT_SOURCE_DIR}/dumps/client_CLO.hex
                                 ${CMAKE_CURRENT_SOURCE_DIR}/dumps/client_CreateActivateSession.hex
                                 ${CMAKE_CURRENT_SOURCE_DIR}/dumps/client_Browse.hex
                                 ${CMAKE_CURRENT_SOURCE_DIR}/dumps/client_Read.hex
                                 ${CMAKE_CURRENT_SOURCE_DIR}/dumps/client_Write.hex
                  DEPENDS ${PROJECT_SOURCE_DIR}/tools/hex2bin.py
                          ${CMAKE_CURRENT_SOURCE_DIR}/dumps/client_HELOPN.hex
                          ${CMAKE_CURRENT_SOURCE_DIR}/dumps/client_CLO.hex
                          ${CMAKE_CURRENT_SOURCE_DIR}/dumps/client_CreateActivateSession.hex
                          ${CMAKE_CURRENT_SOURCE_DIR}/dumps/client_Browse.hex
                          ${CMAKE_CURRENT_SOURCE_DIR}/dumps/client_Read.hex
                          ${CMAKE_CURRENT_SOURCE_DIR}/dumps/client_Write.hex)
add_custom_target(client_HELOPN.bin DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/client_HELOPN.bin")
add_custom_target(client_CreateActivateSession.bin DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/client_CreateActivateSession.bin")

add_executable(check_server_binary_messages check_server_binary_messages.c testing_networklayers.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_include_directories(check_server_binary_messages PRIVATE ${PROJECT_SOURCE_DIR}/src/server)
target_link_libraries(check_server_binary_messages ${LIBS})
add_dependencies(check_server_binary_messages client_HELOPN.bin)

add_test_valgrind(check_server_binary_messages_browse ${TESTS_BINARY_DIR}/check_server_binary_messages
                                             ${CMAKE_CURRENT_BINARY_DIR}/client_HELOPN.bin
                                             ${CMAKE_CURRENT_BINARY_DIR}/client_CreateActivateSession.bin
                                             ${CMAKE_CURRENT_BINARY_DIR}/client_Browse.bin
                                             ${CMAKE_CURRENT_BINARY_DIR}/client_CLO.bin)

add_test_valgrind(check_server_binary_messages_read ${TESTS_BINARY_DIR}/check_server_binary_messages
                                           ${CMAKE_CURRENT_BINARY_DIR}/client_HELOPN.bin
                                           ${CMAKE_CURRENT_BINARY_DIR}/client_CreateActivateSession.bin
                                           ${CMAKE_CURRENT_BINARY_DIR}/client_Read.bin
                                           ${CMAKE_CURRENT_BINARY_DIR}/client_CLO.bin)

add_test_valgrind(check_server_binary_messages_write ${TESTS_BINARY_DIR}/check_server_binary_messages
                                           ${CMAKE_CURRENT_BINARY_DIR}/client_HELOPN.bin
                                           ${CMAKE_CURRENT_BINARY_DIR}/client_CreateActivateSession.bin
                                           ${CMAKE_CURRENT_BINARY_DIR}/client_Write.bin
                                           ${CMAKE_CURRENT_BINARY_DIR}/client_CLO.bin)

# Test Client

add_executable(check_client check_client.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_client ${LIBS})
add_test_valgrind(check_client ${TESTS_BINARY_DIR}/check_client)

add_executable(check_client_async check_client_async.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_client_async ${LIBS})
add_test_valgrind(check_client_async ${TESTS_BINARY_DIR}/check_client_async)

add_executable(check_client_subscriptions check_client_subscriptions.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_client_subscriptions ${LIBS})
add_test_valgrind(check_client_subscriptions ${TESTS_BINARY_DIR}/check_client_subscriptions)

add_executable(check_client_highlevel check_client_highlevel.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_client_highlevel ${LIBS})
add_test_valgrind(check_client_highlevel ${TESTS_BINARY_DIR}/check_client_highlevel)
