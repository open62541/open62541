include_directories("${PROJECT_BINARY_DIR}/src_generated/tests")

###############################################
# Test Companion Specs that need the full NS0 #
###############################################

if(UA_NAMESPACE_ZERO STREQUAL "FULL")

    set(GENERATE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/src_generated/tests")

    # Generate types and namespace for DI
    ua_generate_nodeset_and_datatypes(
        NAME "tests-di"
        FILE_CSV "${UA_NODESET_DIR}/DI/OpcUaDiModel.csv"
        FILE_BSD "${UA_NODESET_DIR}/DI/Opc.Ua.Di.Types.bsd"
        NAMESPACE_MAP "2:http://opcfoundation.org/UA/DI/"
        OUTPUT_DIR "${GENERATE_OUTPUT_DIR}"
        FILE_NS "${UA_NODESET_DIR}/DI/Opc.Ua.Di.NodeSet2.xml"
        INTERNAL
    )

    # Generate types and namespace for ADI
    ua_generate_nodeset_and_datatypes(
        NAME "tests-adi"
        FILE_CSV "${UA_NODESET_DIR}/ADI/OpcUaAdiModel.csv"
        FILE_BSD "${UA_NODESET_DIR}/ADI/Opc.Ua.Adi.Types.bsd"
        NAMESPACE_MAP "3:http://opcfoundation.org/UA/ADI/"
        OUTPUT_DIR "${GENERATE_OUTPUT_DIR}"
        FILE_NS "${UA_NODESET_DIR}/ADI/Opc.Ua.Adi.NodeSet2.xml"
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
        FILE_NS "${UA_NODESET_DIR}/PLCopen/Opc.Ua.PLCopen.NodeSet2_V1.02.xml"
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

    # generate AutoID namespace which is using DI (test e.g. for structures with optional fields)
    ua_generate_nodeset_and_datatypes(
            NAME "tests-autoid"
            FILE_CSV "${PROJECT_SOURCE_DIR}/tests/nodeset-compiler/Opc.Ua.AutoID.NodeIds.csv"
            FILE_BSD "${PROJECT_SOURCE_DIR}/tests/nodeset-compiler/Opc.Ua.AutoID.Types.bsd"
            NAMESPACE_MAP "3:http://opcfoundation.org/UA/AutoID/"
            OUTPUT_DIR "${GENERATE_OUTPUT_DIR}"
            FILE_NS "${UA_NODESET_DIR}/AutoID/Opc.Ua.AutoID.NodeSet2.xml"
            DEPENDS "tests-di"
            INTERNAL
    )

    add_executable(check_nodeset_compiler_autoid check_nodeset_compiler_autoid.c
            ${UA_NODESET_TESTS_DI_SOURCES}
            ${UA_NODESET_TESTS_AUTOID_SOURCES}
            ${UA_TYPES_TESTS_DI_SOURCES}
            ${UA_TYPES_TESTS_AUTOID_SOURCES}
            $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    add_dependencies(check_nodeset_compiler_autoid open62541-generator-ns-tests-autoid)
    target_link_libraries(check_nodeset_compiler_autoid ${LIBS})
    add_test_valgrind(nodeset_compiler_autoid ${TESTS_BINARY_DIR}/check_nodeset_compiler_autoid)


endif()

#[[ua_generate_datatypes(
        NAME "optstruct-union"
        TARGET_SUFFIX "types-optstruct-union"
        NAMESPACE_IDX 2
        FILE_CSV "${PROJECT_SOURCE_DIR}/tests/nodeset-compiler/testnodeset.csv"
        FILES_BSD "${PROJECT_SOURCE_DIR}/tests/nodeset-compiler/testtypes.bsd"
)

add_executable(check_nodeset_compiler_optstructs_union check_nodeset_compiler_optstructs_union.c
        ${UA_NODESET_TESTS_DI_SOURCES}
        ${UA_NODESET_TESTS_PLC_SOURCES}
        ${UA_TYPES_TESTS_DI_SOURCES}
        ${UA_TYPES_TESTS_PLC_SOURCES}
        $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
add_dependencies(check_nodeset_compiler_optstructs_union open62541-generator-types-optstruct-union)
target_link_libraries(check_nodeset_compiler_optstructs_union ${LIBS})
add_test_valgrind(check_nodeset_compiler_optstructs_union ${TESTS_BINARY_DIR}/check_nodeset_compiler_optstructs_union)]]

#generate testnodeset
if(UA_NAMESPACE_ZERO STREQUAL "FULL")

    # Generate types and namespace for ADI
    ua_generate_nodeset_and_datatypes(
        NAME "tests-testnodeset"
        FILE_CSV "${PROJECT_SOURCE_DIR}/tests/nodeset-compiler/testnodeset.csv"
        FILE_BSD "${PROJECT_SOURCE_DIR}/tests/nodeset-compiler/testtypes.bsd"
        NAMESPACE_MAP "3:http://yourorganisation.org/test/"
        OUTPUT_DIR "${GENERATE_OUTPUT_DIR}"
        FILE_NS "${PROJECT_SOURCE_DIR}/tests/nodeset-compiler/testnodeset.xml"
        DEPENDS "tests-di"
    )

    add_executable(check_nodeset_compiler_testnodeset check_nodeset_compiler_testnodeset.c
                   ${UA_NODESET_TESTS_DI_SOURCES}
                   ${UA_NODESET_TESTS_TESTNODESET_SOURCES}
                   ${UA_TYPES_TESTS_DI_SOURCES}
                   ${UA_TYPES_TESTS_TESTNODESET_SOURCES}
                $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    add_dependencies(check_nodeset_compiler_testnodeset open62541-generator-ns-tests-testnodeset)
    target_link_libraries(check_nodeset_compiler_testnodeset ${LIBS})
    add_test_valgrind(nodeset_compiler_testnodeset ${TESTS_BINARY_DIR}/check_nodeset_compiler_testnodeset)
endif()
