#######################################################
# Integration test for adding nodes via nodesetLoader #
#######################################################

if(UA_NAMESPACE_ZERO STREQUAL "FULL")
    if(UA_ENABLE_XML_ENCODING)

        add_executable(add_node_integration_test_client client.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
        set_target_properties(add_node_integration_test_client PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
        target_link_libraries(add_node_integration_test_client ${LIBS})

        add_executable(add_node_integration_test_server server.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
        set_target_properties(add_node_integration_test_server PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
        target_link_libraries(add_node_integration_test_server ${LIBS})

        find_program(BASH_PROGRAM bash)

        add_test(check_nodeset_loader_add_node_integration
                 ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/run_test.sh 
                 "${CMAKE_CURRENT_BINARY_DIR}/add_node_integration_test_client"
                 "${CMAKE_CURRENT_BINARY_DIR}/add_node_integration_test_server"
        )

        add_test(check_nodeset_loader_DI_ordering_integration
                 ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/run_test_ordering.sh
                 "${CMAKE_CURRENT_BINARY_DIR}/add_node_integration_test_client"
                 "${CMAKE_CURRENT_BINARY_DIR}/add_node_integration_test_server"
        )
    endif()

endif()