include_directories("${PROJECT_BINARY_DIR}/src_generated/tests")

###############################################
# Test Companion Specs that need the full NS0 #
###############################################

if(UA_NAMESPACE_ZERO STREQUAL "FULL")

    set(GENERATE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/src_generated/tests")

    # Generate types and namespace for DI
    ua_generate_nodeset_and_datatypes(
        NAME "tests-di"
        FILE_CSV "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/OpcUaDiModel.csv"
        FILE_BSD "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/Opc.Ua.Di.Types.bsd"
        NAMESPACE_IDX 2
        OUTPUT_DIR "${GENERATE_OUTPUT_DIR}"
        FILE_NS "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/Opc.Ua.Di.NodeSet2.xml"
        INTERNAL
    )

    # Generate types and namespace for ADI
    ua_generate_nodeset_and_datatypes(
        NAME "tests-adi"
        FILE_CSV "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/ADI/OpcUaAdiModel.csv"
        FILE_BSD "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/ADI/Opc.Ua.Adi.Types.bsd"
        NAMESPACE_IDX 3
        OUTPUT_DIR "${GENERATE_OUTPUT_DIR}"
        FILE_NS "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/ADI/Opc.Ua.Adi.NodeSet2.xml"
        DEPENDS "tests-di"
        INTERNAL
    )

    add_executable(check_nodeset_compiler_adi check_nodeset_compiler_adi.c
                   ${UA_NODESET_TESTS_DI_SOURCES}
                   ${UA_NODESET_TESTS_ADI_SOURCES}
                   ${UA_TYPES_TESTS_DI_SOURCES}
                   ${UA_TYPES_TESTS_ADI_SOURCES}
                   $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    add_dependencies(check_nodeset_compiler_adi open62541-generator-ns-tests-adi)
    target_link_libraries(check_nodeset_compiler_adi ${LIBS})
    add_test_valgrind(nodeset_compiler_adi ${TESTS_BINARY_DIR}/check_nodeset_compiler_adi)


    # generate PLCopen namespace which is using DI
    ua_generate_nodeset_and_datatypes(
        NAME "tests-plc"
        # PLCopen does not define custom types. Only generate the nodeset
        FILE_NS "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/PLCopen/Opc.Ua.Plc.NodeSet2.xml"
        # PLCopen depends on the di nodeset, which must be generated before
        OUTPUT_DIR "${GENERATE_OUTPUT_DIR}"
        DEPENDS "tests-di"
        INTERNAL
    )

    add_executable(check_nodeset_compiler_plc check_nodeset_compiler_plc.c
                   ${UA_NODESET_TESTS_DI_SOURCES}
                   ${UA_NODESET_TESTS_PLC_SOURCES}
                   ${UA_TYPES_TESTS_DI_SOURCES}
                   ${UA_TYPES_TESTS_PLC_SOURCES}
                   $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    add_dependencies(check_nodeset_compiler_plc open62541-generator-ns-tests-plc)
    target_link_libraries(check_nodeset_compiler_plc ${LIBS})
    add_test_valgrind(nodeset_compiler_plc ${TESTS_BINARY_DIR}/check_nodeset_compiler_plc)

    
endif()

#generate testnodeset
if(UA_NAMESPACE_ZERO STREQUAL "FULL")


    # Generate types and namespace for ADI
    ua_generate_nodeset_and_datatypes(
        NAME "tests-testnodeset"
        FILE_CSV "${PROJECT_SOURCE_DIR}/tests/nodeset-compiler/testnodeset.csv"
        FILE_BSD "${PROJECT_SOURCE_DIR}/tests/nodeset-compiler/testtypes.bsd"
        NAMESPACE_IDX 2
        OUTPUT_DIR "${GENERATE_OUTPUT_DIR}"
        FILE_NS "${PROJECT_SOURCE_DIR}/tests/nodeset-compiler/testnodeset.xml"
    )

    add_executable(check_nodeset_compiler_testnodeset check_nodeset_compiler_testnodeset.c
                   ${UA_NODESET_TESTS_TESTNODESET_SOURCES}
                   ${UA_TYPES_TESTS_TESTNODESET_SOURCES}
                $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    add_dependencies(check_nodeset_compiler_testnodeset open62541-generator-ns-tests-testnodeset)
    target_link_libraries(check_nodeset_compiler_testnodeset ${LIBS})
    add_test_valgrind(nodeset_compiler_testnodeset ${TESTS_BINARY_DIR}/check_nodeset_compiler_testnodeset)
endif()
