find_package(Check REQUIRED)
find_package(Threads REQUIRED)

set(LIBS open62541 ${CHECK_LIBRARIES} m)
if(NOT WIN32)
    list(APPEND LIBS rt pthread)
endif(NOT WIN32)
if(ENABLE_MULTITHREADING)
    list(APPEND LIBS urcu-cds urcu)
endif(ENABLE_MULTITHREADING)

add_executable(check_builtin check_builtin.c)
target_link_libraries(check_builtin ${LIBS})
add_test(builtin ${CMAKE_CURRENT_BINARY_DIR}/check_builtin)

add_executable(check_memory check_memory.c)
target_link_libraries(check_memory ${LIBS})
add_test(memory ${CMAKE_CURRENT_BINARY_DIR}/check_memory)

add_executable(check_stack check_stack.c)
target_link_libraries(check_stack ${LIBS})
add_test(stack ${CMAKE_CURRENT_BINARY_DIR}/check_stack)

add_executable(check_list check_list.c)
target_link_libraries(check_list ${LIBS})
add_test(list ${CMAKE_CURRENT_BINARY_DIR}/check_list)

add_executable(check_indexedList check_indexedList.c)
target_link_libraries(check_indexedList ${LIBS})
add_test(indexedList ${CMAKE_CURRENT_BINARY_DIR}/check_indexedList)

add_executable(check_base64 check_base64.c)
target_link_libraries(check_base64 ${LIBS})
add_test(base64 ${CMAKE_CURRENT_BINARY_DIR}/check_base64)

add_executable(check_services_view check_services_view.c)
target_link_libraries(check_services_view ${LIBS})
add_test(services_view ${CMAKE_CURRENT_BINARY_DIR}/check_services_view)

add_executable(check_namespace check_namespace.c)
target_link_libraries(check_namespace ${LIBS})
add_test(namespace ${CMAKE_CURRENT_BINARY_DIR}/check_namespace)

add_executable(check_startup check_startup.c)
target_link_libraries(check_startup ${LIBS})
add_test(startup ${CMAKE_CURRENT_BINARY_DIR}/check_startup)
