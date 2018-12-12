file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/src_generated/tests")
include_directories(
    "${PROJECT_BINARY_DIR}/src_generated/tests"
    ${CMAKE_CURRENT_BINARY_DIR}
    )

# Generate types for ADI namespace
ua_generate_datatypes(
    NAME "test_types"
    TARGET_SUFFIX "test-types"
    NAMESPACE_IDX 2
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}"
    FILE_CSV "${CMAKE_CURRENT_SOURCE_DIR}/test.csv"
    FILES_BSD "${CMAKE_CURRENT_SOURCE_DIR}/test.bsd"
)

add_executable(check_datatype_generator
               check_datatype_generator.c
               ${CMAKE_CURRENT_BINARY_DIR}/test_types_generated.c
               $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
add_dependencies(check_datatype_generator open62541-generator-test-types)
target_link_libraries(check_datatype_generator ${LIBS})
add_test_valgrind(check_datatype_generator ${TESTS_BINARY_DIR}/check_datatype_generator)
