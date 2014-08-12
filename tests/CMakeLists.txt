find_package(Check REQUIRED)

add_executable(check_builtin check_builtin.c)
target_link_libraries(check_builtin open62541 check)
add_test(builtin ${CMAKE_CURRENT_BINARY_DIR}/check_builtin)

add_executable(check_memory check_memory.c)
target_link_libraries(check_memory open62541 check)
add_test(memory ${CMAKE_CURRENT_BINARY_DIR}/check_memory)

add_executable(check_stack check_stack.c)
target_link_libraries(check_stack open62541 check)
add_test(stack ${CMAKE_CURRENT_BINARY_DIR}/check_stack)

add_executable(check_list check_list.c)
target_link_libraries(check_list open62541 check)
add_test(list ${CMAKE_CURRENT_BINARY_DIR}/check_list)

add_executable(check_indexedList check_indexedList.c)
target_link_libraries(check_indexedList open62541 check)
add_test(indexedList ${CMAKE_CURRENT_BINARY_DIR}/check_indexedList)

add_executable(check_base64 check_base64.c)
target_link_libraries(check_base64 open62541 check)
add_test(base64 ${CMAKE_CURRENT_BINARY_DIR}/check_base64)

add_executable(check_services_view check_services_view.c)
target_link_libraries(check_services_view open62541 check)
add_test(services_view ${CMAKE_CURRENT_BINARY_DIR}/check_services_view)

add_executable(check_namespace check_namespace.c)
target_link_libraries(check_namespace open62541 check)
add_test(namespace ${CMAKE_CURRENT_BINARY_DIR}/check_namespace)
