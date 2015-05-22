include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/deps)
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_BINARY_DIR}/src_generated)

find_package(Check REQUIRED)
find_package(Threads REQUIRED)

set(LIBS ${CHECK_LIBRARIES})
if(NOT WIN32)
    list(APPEND LIBS rt pthread)
else()
    list(APPEND LIBS ws2_32)
endif()
if(MULTITHREADING)
    list(APPEND LIBS urcu-cds urcu urcu-common)
endif(MULTITHREADING)

# the unit test are built directly on the open62541 object files. so they can
# access symbols that are hidden/not exported to the shared library

add_executable(check_builtin check_builtin.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(check_builtin ${LIBS})
add_test(builtin ${CMAKE_CURRENT_BINARY_DIR}/check_builtin)

add_executable(check_memory check_memory.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(check_memory ${LIBS})
add_test(memory ${CMAKE_CURRENT_BINARY_DIR}/check_memory)

# add_executable(check_stack check_stack.c)
# target_link_libraries(check_stack ${LIBS})
# add_test(stack ${CMAKE_CURRENT_BINARY_DIR}/check_stack)

# add_executable(check_base64 check_base64.c)
# target_link_libraries(check_base64 ${LIBS})
# add_test(base64 ${CMAKE_CURRENT_BINARY_DIR}/check_base64)

add_executable(check_services_view check_services_view.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(check_services_view ${LIBS})
add_test(services_view ${CMAKE_CURRENT_BINARY_DIR}/check_services_view)

add_executable(check_nodestore check_nodestore.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(check_nodestore ${LIBS})
add_test(nodestore ${CMAKE_CURRENT_BINARY_DIR}/check_nodestore)

# add_executable(check_startup check_startup.c)
# target_link_libraries(check_startup ${LIBS})
# add_test(startup ${CMAKE_CURRENT_BINARY_DIR}/check_startup)

# test with canned interactions from files

#add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/client_HELOPN.bin
#                   PRE_BUILD
#                   COMMAND python ${PROJECT_SOURCE_DIR}/tools/hex2bin.py ${CMAKE_CURRENT_SOURCE_DIR}/dumps/client_HELOPN.hex
#                                  ${CMAKE_CURRENT_SOURCE_DIR}/dumps/client_CreateActivateSession.hex
#                   DEPENDS ${PROJECT_SOURCE_DIR}/tools/hex2bin.py
#                           ${CMAKE_CURRENT_SOURCE_DIR}/dumps/client_HELOPN.hex)

#set(check_fileinput_source check_server_interaction_fileinput.c testing_networklayers.c $<TARGET_OBJECTS:open62541-object>)
#if(NOT ENABLE_AMALGAMATION)
#    list(APPEND check_fileinput_source ${PROJECT_SOURCE_DIR}/examples/logger_stdout.c)
#endif()
#add_executable(check_server_interaction_fileinput ${check_fileinput_source})

#target_include_directories(check_server_interaction_fileinput PRIVATE ${PROJECT_SOURCE_DIR}/examples)
#target_include_directories(check_server_interaction_fileinput PRIVATE ${PROJECT_BINARY_DIR})
#target_link_libraries(check_server_interaction_fileinput ${LIBS})
#add_test(server_interaction_fileinput ${CMAKE_CURRENT_BINARY_DIR}/check_server_interaction_fileinput ${CMAKE_CURRENT_BINARY_DIR}/client_HELOPN.bin)
#add_custom_target(server_interaction_fileinput ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/client_HELOPN.bin)
