include_directories("${PROJECT_BINARY_DIR}/src_generated/tests")

###############################################
# Test Companion Specs that need the full NS0 #
###############################################

set(GENERATE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/src_generated/tests")

# Generate datatypes directly from NodeSet2 Definition elements
ua_generate_nodeset_and_datatypes(
    NAME "typedescriptions"
    OUTPUT_DIR "${GENERATE_OUTPUT_DIR}"
    FILE_NS "${PROJECT_SOURCE_DIR}/tests/nodeset-compiler/typedescriptions.xml"
)

ua_add_test(check_nodeset_typedescriptions.c EXTRASOURCES ${UA_NODESET_TYPEDESCRIPTIONS_SOURCES})
add_dependencies(check_nodeset_typedescriptions open62541-generator-ns-typedescriptions)

if(UA_NAMESPACE_ZERO STREQUAL "FULL")
    # Generate types and namespace for DI
    ua_generate_nodeset_and_datatypes(
        NAME "tests-di"
        OUTPUT_DIR "${GENERATE_OUTPUT_DIR}"
        FILE_NS "${UA_NODESET_DIR}/DI/Opc.Ua.Di.NodeSet2.xml"
        INTERNAL
    )

    # Generate types and namespace for ADI
    ua_generate_nodeset_and_datatypes(
        NAME "tests-adi"
        OUTPUT_DIR "${GENERATE_OUTPUT_DIR}"
        FILE_NS "${UA_NODESET_DIR}/ADI/Opc.Ua.Adi.NodeSet2.xml"
        DEPENDS "tests-di"
        INTERNAL
    )

    ua_add_test(check_nodeset_compiler_adi.c EXTRASOURCES
                ${UA_NODESET_TESTS_DI_SOURCES}
                ${UA_NODESET_TESTS_ADI_SOURCES}
                ${UA_TYPES_TESTS_DI_SOURCES}
                ${UA_TYPES_TESTS_ADI_SOURCES})
    add_dependencies(check_nodeset_compiler_adi open62541-generator-ns-tests-adi)

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

    ua_add_test(check_nodeset_compiler_plc.c EXTRASOURCES
                ${UA_NODESET_TESTS_DI_SOURCES}
                ${UA_NODESET_TESTS_PLC_SOURCES}
                ${UA_TYPES_TESTS_DI_SOURCES}
                ${UA_TYPES_TESTS_PLC_SOURCES})
    add_dependencies(check_nodeset_compiler_plc open62541-generator-ns-tests-plc)

    ua_add_test(check_client_nsMapping.c EXTRASOURCES
            ${UA_NODESET_TESTS_DI_SOURCES}
            ${UA_NODESET_TESTS_PLC_SOURCES}
            ${UA_TYPES_TESTS_DI_SOURCES}
            ${UA_TYPES_TESTS_PLC_SOURCES})
    add_dependencies(check_client_nsMapping open62541-generator-ns-tests-plc)

    ua_add_test(check_client_get_remote_datatypes.c EXTRASOURCES
            ${UA_NODESET_TESTS_DI_SOURCES}
            ${UA_TYPES_TESTS_DI_SOURCES})
    add_dependencies(check_client_get_remote_datatypes open62541-generator-ns-tests-di)


    # generate AutoID namespace which is using DI (test e.g. for structures with optional fields)
    ua_generate_nodeset_and_datatypes(
            NAME "tests-autoid"
            OUTPUT_DIR "${GENERATE_OUTPUT_DIR}"
            FILE_NS "${UA_NODESET_DIR}/AutoID/Opc.Ua.AutoID.NodeSet2.xml"
            DEPENDS "tests-di"
            INTERNAL
    )

    ua_add_test(check_nodeset_compiler_autoid.c EXTRASOURCES
                ${UA_NODESET_TESTS_DI_SOURCES}
                ${UA_NODESET_TESTS_AUTOID_SOURCES}
                ${UA_TYPES_TESTS_DI_SOURCES}
                ${UA_TYPES_TESTS_AUTOID_SOURCES})
    add_dependencies(check_nodeset_compiler_autoid open62541-generator-ns-tests-autoid)
endif()

#generate testnodeset
if(UA_NAMESPACE_ZERO STREQUAL "FULL")

    # Generate types and namespace for ADI
    ua_generate_nodeset_and_datatypes(
        NAME "tests-testnodeset"
        OUTPUT_DIR "${GENERATE_OUTPUT_DIR}"
        FILE_NS "${PROJECT_SOURCE_DIR}/tests/nodeset-compiler/testnodeset.xml"
        DEPENDS "tests-di"
    )

    ua_add_test(check_nodeset_compiler_testnodeset.c EXTRASOURCES
                ${UA_NODESET_TESTS_DI_SOURCES}
                ${UA_NODESET_TESTS_TESTNODESET_SOURCES}
                ${UA_TYPES_TESTS_DI_SOURCES}
                ${UA_TYPES_TESTS_TESTNODESET_SOURCES})
    add_dependencies(check_nodeset_compiler_testnodeset open62541-generator-ns-tests-testnodeset)
endif()
# Test cross-namespace same-name type handling in generate_datatypes.py.
# Does not require a full NS0 build - runs standalone against the tool.
# find_package is called explicitly so that Python3_EXECUTABLE is available
# in this directory's CMake scope (the top-level already calls it, but the
# explicit call here makes the dependency clear).
find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME test_generate_datatypes_cross_ns
         COMMAND "${Python3_EXECUTABLE}"
                 "${CMAKE_CURRENT_SOURCE_DIR}/test_cross_ns_types.py")
add_test(NAME test_nodeset_datatype_definitions
         COMMAND "${Python3_EXECUTABLE}"
                 "${CMAKE_CURRENT_SOURCE_DIR}/test_nodeset_datatype_definitions.py")
add_test(NAME test_datatype_definition_parser
         COMMAND "${Python3_EXECUTABLE}"
                 "${CMAKE_CURRENT_SOURCE_DIR}/test_datatype_definition_parser.py")
