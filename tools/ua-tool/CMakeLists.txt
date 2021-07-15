set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(ua ua.c)
target_link_libraries(ua open62541 ${open62541_LIBRARIES})
assign_source_group(ua-tool)
add_dependencies(ua open62541-object)
set_target_properties(ua PROPERTIES FOLDER "open62541/tools/ua-tool")
set_target_properties(ua PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
