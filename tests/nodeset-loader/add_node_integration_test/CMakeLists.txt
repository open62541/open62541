#######################################################
# Integration test for adding nodes via nodesetLoader #
#######################################################

if(UA_NAMESPACE_ZERO STREQUAL "FULL")

    add_executable(client client.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    set_target_properties(client PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    target_link_libraries(client ${LIBS})

    add_executable(server server.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    set_target_properties(server PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    target_link_libraries(server ${LIBS})

    find_program(BASH_PROGRAM bash)

    add_test(check_nodeset_loader_add_node_integration
             ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/run_test.sh 
             "${CMAKE_CURRENT_BINARY_DIR}/client"
             "${CMAKE_CURRENT_BINARY_DIR}/server"
    )

endif()