# Several models attach test roles to the standard RoleSet. Preserve it just
# as the model injector does for GDS, Onboarding and UAFX-Data.
target_compile_definitions(open62541-object PRIVATE UA_NODESET_INJECTOR_NEEDS_ROLESET)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(companion_output "${PROJECT_BINARY_DIR}/src_generated/companion")
file(MAKE_DIRECTORY "${companion_output}")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/catalog.txt" "${CMAKE_CURRENT_SOURCE_DIR}/generate.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/dependency_overrides.json")
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/generate.py"
    --nodesets "${UA_NODESET_DIR}" --output "${companion_output}"
    RESULT_VARIABLE companion_result)
if(NOT companion_result EQUAL 0)
    message(FATAL_ERROR "Failed to configure the companion test catalog")
endif()
include_directories("${CMAKE_CURRENT_SOURCE_DIR}" "${companion_output}")
file(GLOB companion_generator_dependencies "${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/*.py")
add_library(companion_test_support STATIC test_support.c)
target_compile_definitions(companion_test_support PRIVATE COMPANION_NODESET_DIR="${UA_NODESET_DIR}/")
target_link_libraries(companion_test_support PRIVATE open62541)
include("${companion_output}/targets.cmake")
