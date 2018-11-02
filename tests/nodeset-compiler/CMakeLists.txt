file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/src_generated/tests")
include_directories("${PROJECT_BINARY_DIR}/src_generated/tests")

###############################################
# Test Companion Specs that need the full NS0 #
###############################################

if(UA_NAMESPACE_ZERO STREQUAL "FULL")

    # Generate types for DI namespace
    ua_generate_datatypes(
        NAME "ua_types_di"
        TARGET_SUFFIX "tests-types-di"
        NAMESPACE_IDX 2
        OUTPUT_DIR "${PROJECT_BINARY_DIR}/src_generated/tests"
        FILE_CSV "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/OpcUaDiModel.csv"
        FILES_BSD "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/Opc.Ua.Di.Types.bsd"
    )

    # Generate types for ADI namespace
    ua_generate_datatypes(
        NAME "ua_types_adi"
        TARGET_SUFFIX "tests-types-adi"
        NAMESPACE_IDX 3
        OUTPUT_DIR "${PROJECT_BINARY_DIR}/src_generated/tests"
        FILE_CSV "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/ADI/OpcUaAdiModel.csv"
        FILES_BSD "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/ADI/Opc.Ua.Adi.Types.bsd"
    )

    # generate DI namespace
    ua_generate_nodeset(
        NAME "tests-di"
        FILE "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/Opc.Ua.Di.NodeSet2.xml"
        TYPES_ARRAY "UA_TYPES_DI"
        INTERNAL
        OUTPUT_DIR "${PROJECT_BINARY_DIR}/src_generated/tests"
        DEPENDS_TYPES "UA_TYPES"
        DEPENDS_NS    "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/Schema/Opc.Ua.NodeSet2.xml"
        DEPENDS_TARGET "open62541-generator-tests-types-di"
    )

    # generate PLCopen namespace which is using DI
    ua_generate_nodeset(
        NAME "tests-adi"
        FILE "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/ADI/Opc.Ua.Adi.NodeSet2.xml"
        TYPES_ARRAY "UA_TYPES_ADI"
        INTERNAL
        OUTPUT_DIR "${PROJECT_BINARY_DIR}/src_generated/tests"
        DEPENDS_TYPES
        "UA_TYPES" "UA_TYPES_DI"
        DEPENDS_NS
        "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/Schema/Opc.Ua.NodeSet2.xml"
        "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/Opc.Ua.Di.NodeSet2.xml"
        DEPENDS_TARGET "open62541-generator-ns-tests-di" "open62541-generator-tests-types-adi"
    )


    # generate PLCopen namespace which is using DI
    ua_generate_nodeset(
        NAME "tests-plc"
        FILE "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/PLCopen/Opc.Ua.Plc.NodeSet2.xml"
        INTERNAL
        OUTPUT_DIR "${PROJECT_BINARY_DIR}/src_generated/tests"
        DEPENDS_TYPES
        "UA_TYPES" "UA_TYPES_DI"
        DEPENDS_NS
        "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/Schema/Opc.Ua.NodeSet2.xml"
        "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/Opc.Ua.Di.NodeSet2.xml"
        DEPENDS_TARGET "open62541-generator-ns-tests-di"
    )

    add_executable(check_nodeset_compiler_adi check_nodeset_compiler_adi.c
                   ${PROJECT_BINARY_DIR}/src_generated/tests/ua_types_di_generated.c
                   ${PROJECT_BINARY_DIR}/src_generated/tests/ua_namespace_tests_di.c
                   ${PROJECT_BINARY_DIR}/src_generated/tests/ua_types_adi_generated.c
                   ${PROJECT_BINARY_DIR}/src_generated/tests/ua_namespace_tests_adi.c
                   $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    add_dependencies(check_nodeset_compiler_adi open62541-generator-ns-tests-adi)
    target_link_libraries(check_nodeset_compiler_adi ${LIBS})
    add_test_valgrind(nodeset_compiler_adi ${TESTS_BINARY_DIR}/check_nodeset_compiler_adi)

    add_executable(check_nodeset_compiler_plc check_nodeset_compiler_plc.c
                   ${PROJECT_BINARY_DIR}/src_generated/tests/ua_types_di_generated.c
                   ${PROJECT_BINARY_DIR}/src_generated/tests/ua_namespace_tests_di.c
                   ${PROJECT_BINARY_DIR}/src_generated/tests/ua_namespace_tests_plc.c
                   $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    add_dependencies(check_nodeset_compiler_plc open62541-generator-ns-tests-plc)
    target_link_libraries(check_nodeset_compiler_plc ${LIBS})
    add_test_valgrind(nodeset_compiler_plc ${TESTS_BINARY_DIR}/check_nodeset_compiler_plc)
endif()
