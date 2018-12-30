set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
include_directories(${PROJECT_SOURCE_DIR}/src)

add_executable(ua2json ua2json.c)
target_link_libraries(ua2json open62541 ${open62541_LIBRARIES})
assign_source_group(ua2json)
add_dependencies(ua2json open62541-object)
set_target_properties(ua2json PROPERTIES FOLDER "open62541/tools/ua2json")
set_target_properties(ua2json PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
