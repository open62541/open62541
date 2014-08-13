find_package(Check REQUIRED)
find_package(Threads REQUIRED)

set(LIBS open62541 ${CHECK_LIBRARIES} m rt pthread)

add_executable(check_builtin EXCLUDE_FROM_ALL check_builtin.c)
target_link_libraries(check_builtin ${LIBS})
add_test(builtin ${CMAKE_CURRENT_BINARY_DIR}/check_builtin)
add_dependencies(check check_builtin)

add_executable(check_memory EXCLUDE_FROM_ALL check_memory.c)
target_link_libraries(check_memory ${LIBS})
add_test(memory ${CMAKE_CURRENT_BINARY_DIR}/check_memory)
add_dependencies(check check_memory)

add_executable(check_stack EXCLUDE_FROM_ALL check_stack.c)
target_link_libraries(check_stack ${LIBS})
add_test(stack ${CMAKE_CURRENT_BINARY_DIR}/check_stack)
add_dependencies(check check_stack)

add_executable(check_list EXCLUDE_FROM_ALL check_list.c)
target_link_libraries(check_list ${LIBS})
add_test(list ${CMAKE_CURRENT_BINARY_DIR}/check_list)
add_dependencies(check check_list)

add_executable(check_indexedList EXCLUDE_FROM_ALL check_indexedList.c)
target_link_libraries(check_indexedList ${LIBS})
add_test(indexedList ${CMAKE_CURRENT_BINARY_DIR}/check_indexedList)
add_dependencies(check check_indexedList)

add_executable(check_base64 EXCLUDE_FROM_ALL check_base64.c)
target_link_libraries(check_base64 ${LIBS})
add_test(base64 ${CMAKE_CURRENT_BINARY_DIR}/check_base64)
add_dependencies(check check_base64)

add_executable(check_services_view EXCLUDE_FROM_ALL check_services_view.c)
target_link_libraries(check_services_view ${LIBS})
add_test(services_view ${CMAKE_CURRENT_BINARY_DIR}/check_services_view)
add_dependencies(check check_services_view)

add_executable(check_namespace EXCLUDE_FROM_ALL check_namespace.c)
target_link_libraries(check_namespace ${LIBS})
add_test(namespace ${CMAKE_CURRENT_BINARY_DIR}/check_namespace)
add_dependencies(check check_namespace)

add_executable(check_startup EXCLUDE_FROM_ALL check_startup.c)
target_link_libraries(check_startup ${LIBS})
add_test(startup ${CMAKE_CURRENT_BINARY_DIR}/check_startup)
add_dependencies(check check_startup)
