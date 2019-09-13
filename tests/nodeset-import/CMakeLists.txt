add_executable(check_nodeset_import check_nodeset_import.c
            $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_nodeset_import ${LIBS})
add_test(check_nodeset_import ${TESTS_BINARY_DIR}/check_nodeset_import)
add_test_valgrind(check_nodeset_import ${TESTS_BINARY_DIR}/check_nodeset_import)

add_executable(check_nodeset_importSpeed check_nodeset_importSpeed.c
            $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_nodeset_importSpeed ${LIBS})
add_test(check_nodeset_importSpeed ${TESTS_BINARY_DIR}/check_nodeset_import)

get_filename_component(NODESETPATH ${CMAKE_CURRENT_SOURCE_DIR}/testNodesets ABSOLUTE)
target_compile_definitions(check_nodeset_import PRIVATE NODESETPATH="${NODESETPATH}")
target_compile_definitions(check_nodeset_importSpeed PRIVATE NODESETPATH="${NODESETPATH}")

add_executable(check_nodeset_conversion check_nodeset_conversion.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_include_directories(check_nodeset_conversion PRIVATE "${PROJECT_SOURCE_DIR}/plugins/xmlimport")
target_link_libraries(check_nodeset_conversion ${LIBS})
add_test(check_nodeset_conversion ${TESTS_BINARY_DIR}/check_nodeset_conversion)