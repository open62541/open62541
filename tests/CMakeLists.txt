include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/deps)
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/plugins)
include_directories(${PROJECT_BINARY_DIR}/src_generated)
include_directories(${CHECK_INCLUDE_DIRS})

find_package(Check REQUIRED)
find_package(Threads REQUIRED)

set(LIBS ${CHECK_LIBRARIES} ${open62541_LIBRARIES} open62541)
if(NOT WIN32)
  list(APPEND LIBS pthread m)
  if (NOT APPLE)
    list(APPEND LIBS rt subunit)
  endif()
else()
    list(APPEND LIBS ws2_32)
endif()
if(UA_ENABLE_MULTITHREADING)
    list(APPEND LIBS urcu-cds urcu urcu-common)
endif()

if(CMAKE_COMPILER_IS_GNUCC OR "x${CMAKE_C_COMPILER_ID}" STREQUAL "xClang")
    add_definitions(-Wno-sign-conversion)
endif()

# Valgrind definition
set(UA_TEST_WITH_VALGRIND ON)
SET(VALGRIND_FLAGS --quiet --trace-children=yes --leak-check=full)
macro(add_test_valgrind TEST_NAME)
    IF(UA_TEST_WITH_VALGRIND)
        add_test(${TEST_NAME}
                valgrind --error-exitcode=1 ${VALGRIND_FLAGS} ${ARGN} )
    ELSE()
        add_test(${TEST_NAME} ${ARGN})
    ENDIF()
endmacro()



# the unit test are built directly on the open62541 object files. so they can
# access symbols that are hidden/not exported to the shared library

add_executable(check_types_builtin check_types_builtin.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(check_types_builtin ${LIBS})
add_test(types_builtin ${CMAKE_CURRENT_BINARY_DIR}/check_types_builtin)

add_executable(check_types_memory check_types_memory.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(check_types_memory ${LIBS})
add_test(types_memory ${CMAKE_CURRENT_BINARY_DIR}/check_types_memory)

add_executable(check_types_range check_types_range.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(check_types_range ${LIBS})
add_test(types_range ${CMAKE_CURRENT_BINARY_DIR}/check_types_range)

add_executable(check_chunking check_chunking.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(check_chunking ${LIBS})
add_test_valgrind(chunking ${CMAKE_CURRENT_BINARY_DIR}/check_chunking)

add_executable(check_services_view check_services_view.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(check_services_view ${LIBS})
add_test_valgrind(services_view ${CMAKE_CURRENT_BINARY_DIR}/check_services_view)

add_executable(check_services_attributes check_services_attributes.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(check_services_attributes ${LIBS})
add_test_valgrind(services_attributes ${CMAKE_CURRENT_BINARY_DIR}/check_services_attributes)

add_executable(check_services_nodemanagement check_services_nodemanagement.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(check_services_nodemanagement ${LIBS})
add_test_valgrind(services_nodemanagement ${CMAKE_CURRENT_BINARY_DIR}/check_services_nodemanagement)

add_executable(check_services_subscriptions check_services_subscriptions.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(check_services_subscriptions ${LIBS})
add_test(check_services_subscriptions ${CMAKE_CURRENT_BINARY_DIR}/check_services_subscriptions)

add_executable(check_nodestore check_nodestore.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(check_nodestore ${LIBS})
add_test_valgrind(nodestore ${CMAKE_CURRENT_BINARY_DIR}/check_nodestore)

add_executable(check_session check_session.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(check_session ${LIBS})
add_test_valgrind(session ${CMAKE_CURRENT_BINARY_DIR}/check_session)

add_executable(check_server_jobs check_server_jobs.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(check_server_jobs ${LIBS})
add_test(check_server_jobs ${CMAKE_CURRENT_BINARY_DIR}/check_server_jobs)

add_executable(check_server_userspace check_server_userspace.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(check_server_userspace ${LIBS})
add_test_valgrind(check_server_userspace ${CMAKE_CURRENT_BINARY_DIR}/check_server_userspace)

add_executable(check_discovery check_discovery.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(check_discovery ${LIBS})
add_test_valgrind(discovery ${CMAKE_CURRENT_BINARY_DIR}/check_discovery)

# test with canned interactions from files

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

add_executable(check_server_binary_messages check_server_binary_messages.c testing_networklayers.c $<TARGET_OBJECTS:open62541-object>)
target_include_directories(check_server_binary_messages PRIVATE ${PROJECT_SOURCE_DIR}/src/server)
target_link_libraries(check_server_binary_messages ${LIBS})
add_dependencies(check_server_binary_messages client_HELOPN.bin)

add_test_valgrind(check_server_binary_messages_browse ${CMAKE_CURRENT_BINARY_DIR}/check_server_binary_messages
                                             ${CMAKE_CURRENT_BINARY_DIR}/client_HELOPN.bin
                                             ${CMAKE_CURRENT_BINARY_DIR}/client_CreateActivateSession.bin
                                             ${CMAKE_CURRENT_BINARY_DIR}/client_Browse.bin
                                             ${CMAKE_CURRENT_BINARY_DIR}/client_CLO.bin)

add_test_valgrind(check_server_binary_messages_read ${CMAKE_CURRENT_BINARY_DIR}/check_server_binary_messages
                                           ${CMAKE_CURRENT_BINARY_DIR}/client_HELOPN.bin
                                           ${CMAKE_CURRENT_BINARY_DIR}/client_CreateActivateSession.bin
                                           ${CMAKE_CURRENT_BINARY_DIR}/client_Read.bin
                                           ${CMAKE_CURRENT_BINARY_DIR}/client_CLO.bin)

add_test_valgrind(check_server_binary_messages_write ${CMAKE_CURRENT_BINARY_DIR}/check_server_binary_messages
                                           ${CMAKE_CURRENT_BINARY_DIR}/client_HELOPN.bin
                                           ${CMAKE_CURRENT_BINARY_DIR}/client_CreateActivateSession.bin
                                           ${CMAKE_CURRENT_BINARY_DIR}/client_Write.bin
                                           ${CMAKE_CURRENT_BINARY_DIR}/client_CLO.bin)

add_executable(check_client check_client.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(check_client ${LIBS})
add_test_valgrind(check_client ${CMAKE_CURRENT_BINARY_DIR}/check_client)

add_executable(check_client_subscriptions check_client_subscriptions.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(check_client_subscriptions ${LIBS})
add_test_valgrind(check_client_subscriptions ${CMAKE_CURRENT_BINARY_DIR}/check_client_subscriptions)

add_executable(check_utils check_utils.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(check_utils ${LIBS})
add_test(check_utils ${CMAKE_CURRENT_BINARY_DIR}/check_utils)
