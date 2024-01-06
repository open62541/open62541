add_compile_definitions(OPEN62541_NODESET_DIR="${UA_NODESET_DIR}/")
ua_add_test(check_nodeset_loader_di.c)
ua_add_test(check_nodeset_loader_autoid.c)
ua_add_test(check_nodeset_loader_plc.c)
ua_add_test(check_nodeset_loader_input.c)

if(UA_NAMESPACE_ZERO STREQUAL "FULL")
    ua_add_test(check_nodeset_loader_adi.c)
    ua_add_test(check_nodeset_loader_ua_nodeset.c)

    set(GENERATE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/src_generated/tests")

    # Generate types and namespace for DI
    ua_generate_nodeset_and_datatypes(
        NAME "nodesetloader-di"
        FILE_CSV "${UA_NODESET_DIR}/DI/Opc.Ua.Di.NodeIds.csv"
        FILE_BSD "${UA_NODESET_DIR}/DI/Opc.Ua.Di.Types.bsd"
        OUTPUT_DIR "${GENERATE_OUTPUT_DIR}"
        FILE_NS "${UA_NODESET_DIR}/DI/Opc.Ua.Di.NodeSet2.xml"
        INTERNAL)

    add_executable(check_nodeset_loader_compare_di check_nodeset_loader_compare_di.c
                   ${UA_NODESET_NODESETLOADER_DI_SOURCES} ${UA_TYPES_NODESETLOADER_DI_SOURCES}
                   $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-plugins> $<TARGET_OBJECTS:open62541-testplugins>)
    add_dependencies(check_nodeset_loader_compare_di open62541-generator-ns-nodesetloader-di)
    target_link_libraries(check_nodeset_loader_compare_di ${LIBS})
    add_test_valgrind(check_nodeset_loader_compare_di ${TESTS_BINARY_DIR}/check_nodeset_loader_compare_di)

    add_compile_definitions(OPEN62541_TESTNODESET_DIR="${CMAKE_BINARY_DIR}/../deps/nodesetLoader/nodesets/open62541/")
    ua_add_test(check_nodeset_loader_testnodeset.c)

    add_compile_definitions(OPEN62541_ORDERING_DIR="${CMAKE_BINARY_DIR}/../deps/nodesetLoader/nodesets/open62541/")
    ua_add_test(check_nodeset_loader_ordering_di.c)

    #####################
    # Integration tests #
    #####################

    add_executable(add_node_integration_test_client client.c
                   $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-plugins> $<TARGET_OBJECTS:open62541-testplugins>)
    set_target_properties(add_node_integration_test_client PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    target_link_libraries(add_node_integration_test_client ${LIBS})

    add_executable(add_node_integration_test_server server.c
                   $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-plugins> $<TARGET_OBJECTS:open62541-testplugins>)
    set_target_properties(add_node_integration_test_server PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    target_link_libraries(add_node_integration_test_server ${LIBS})

    find_program(BASH_PROGRAM bash)

    add_test(check_nodeset_loader_add_node_integration
             ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/run_test.sh
             "${CMAKE_CURRENT_BINARY_DIR}/add_node_integration_test_client"
             "${CMAKE_CURRENT_BINARY_DIR}/add_node_integration_test_server")

    add_test(check_nodeset_loader_DI_ordering_integration
             ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/run_test_ordering.sh
             "${CMAKE_CURRENT_BINARY_DIR}/add_node_integration_test_client"
             "${CMAKE_CURRENT_BINARY_DIR}/add_node_integration_test_server")

endif()
