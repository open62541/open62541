find_package(Check REQUIRED)
find_package(Threads REQUIRED)

set(LIBS ${CHECK_LIBRARIES})
if(NOT WIN32)
    list(APPEND LIBS rt pthread)
endif(NOT WIN32)
if(MULTITHREADING)
    list(APPEND LIBS urcu-cds urcu urcu-common)
endif(MULTITHREADING)

# the unit test are built directly on the open62541 object files. so they can
# access symbols that are hidden/not exported to the shared library

add_executable(check_builtin $<TARGET_OBJECTS:open62541-objects> check_builtin.c)
target_link_libraries(check_builtin ${LIBS})
add_test(builtin ${CMAKE_CURRENT_BINARY_DIR}/check_builtin)

add_executable(check_memory $<TARGET_OBJECTS:open62541-objects> check_memory.c)
target_link_libraries(check_memory ${LIBS})
add_test(memory ${CMAKE_CURRENT_BINARY_DIR}/check_memory)

# add_executable(check_stack check_stack.c)
# target_link_libraries(check_stack ${LIBS})
# add_test(stack ${CMAKE_CURRENT_BINARY_DIR}/check_stack)

# add_executable(check_base64 check_base64.c)
# target_link_libraries(check_base64 ${LIBS})
# add_test(base64 ${CMAKE_CURRENT_BINARY_DIR}/check_base64)

add_executable(check_services_view $<TARGET_OBJECTS:open62541-objects> check_services_view.c)
target_link_libraries(check_services_view ${LIBS})
add_test(services_view ${CMAKE_CURRENT_BINARY_DIR}/check_services_view)

add_executable(check_nodestore $<TARGET_OBJECTS:open62541-objects> check_nodestore.c)
target_link_libraries(check_nodestore ${LIBS})
add_test(nodestore ${CMAKE_CURRENT_BINARY_DIR}/check_nodestore)

# add_executable(check_startup check_startup.c)
# target_link_libraries(check_startup ${LIBS})
# add_test(startup ${CMAKE_CURRENT_BINARY_DIR}/check_startup)
