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

    # One const array containing two namespaces, used by nodesets with both
    # a matching name and an independently chosen name.
    ua_generate_datatypes(
        NAME "types-tests-pinned-multi"
        TARGET_SUFFIX "types-tests-pinned-multi"
        FILE_CSV "${CMAKE_CURRENT_SOURCE_DIR}/pinned_namespaces.csv"
        FILES_BSD "${CMAKE_CURRENT_SOURCE_DIR}/pinned_namespaces_a.bsd"
                  "${CMAKE_CURRENT_SOURCE_DIR}/pinned_namespaces_b.bsd"
        NAMESPACE_MAP "2:urn:open62541:test:pinned-a"
                      "3:urn:open62541:test:pinned-b"
        OUTPUT_DIR "${GENERATE_OUTPUT_DIR}")
    foreach(name tests-pinned-multi tests-pinned-renamed)
        ua_generate_nodeset(
            NAME "${name}"
            FILE "${CMAKE_CURRENT_SOURCE_DIR}/pinned_namespaces.xml"
            TYPES_ARRAY "UA_TYPES_TESTS_PINNED_MULTI"
            DEPENDS_NS ${open62541_NS0_NODESETS}
            DEPENDS_TYPES "UA_TYPES"
            OUTPUT_DIR "${GENERATE_OUTPUT_DIR}")
    endforeach()
    ua_add_test(check_nodeset_compiler_pinned_namespaces.c EXTRASOURCES
                ${UA_TYPES_TESTS_PINNED_MULTI_SOURCES}
                ${UA_NODESET_TESTS_PINNED_MULTI_SOURCES}
                ${UA_NODESET_TESTS_PINNED_RENAMED_SOURCES})

    # Generate types and namespace for DI. The namespace index is pinned via
    # NAMESPACE_MAP, so the generated type array is const and the tests
    # exercise the const type array path including the generated runtime
    # namespace-order check (DI is always the first namespace added to the
    # test servers -> index 2).
    ua_generate_nodeset_and_datatypes(
        NAME "tests-di"
        FILE_CSV "${UA_NODESET_DIR}/DI/Opc.Ua.Di.NodeIds.csv"
        FILE_BSD "${UA_NODESET_DIR}/DI/Opc.Ua.Di.Types.bsd"
        NAMESPACE_MAP "2:http://opcfoundation.org/UA/DI/"
        OUTPUT_DIR "${GENERATE_OUTPUT_DIR}"
        FILE_NS "${UA_NODESET_DIR}/DI/Opc.Ua.Di.NodeSet2.xml"
        INTERNAL
    )

    # Covers both outcomes of the generated namespace-order check: the pinned
    # index matching the server, and the mismatch that must be rejected.
    ua_add_test(check_nodeset_compiler_di_pinned_ns.c EXTRASOURCES
                ${UA_NODESET_TESTS_DI_SOURCES}
                ${UA_TYPES_TESTS_DI_SOURCES})
    add_dependencies(check_nodeset_compiler_di_pinned_ns open62541-generator-ns-tests-di)

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

add_test(NAME test_generate_datatypes_symbolic_names
         COMMAND "${Python3_EXECUTABLE}"
                 "${CMAKE_CURRENT_SOURCE_DIR}/test_symbolic_names.py")
# Values from different input files keep their original namespace tables.
set(mapping_output "${CMAKE_BINARY_DIR}/src_generated/tests")
set(mapping_large "${mapping_output}/namespace_mapping_large.xml")
add_custom_command(OUTPUT "${mapping_large}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/generate_namespace_mapping_fixture.py" "${mapping_large}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/generate_namespace_mapping_fixture.py")
ua_generate_nodeset(NAME "tests-value-mapping" OUTPUT_DIR "${mapping_output}"
    DEPENDS_NS ${open62541_NS0_NODESETS} "${CMAKE_CURRENT_SOURCE_DIR}/namespace_mapping_custom.xml"
    DEPENDS_TYPES "UA_TYPES"
    FILE "${CMAKE_CURRENT_SOURCE_DIR}/namespace_mapping_a.xml"
         "${CMAKE_CURRENT_SOURCE_DIR}/namespace_mapping_b.xml" "${mapping_large}")
ua_add_test(check_nodeset_value_mapping.c EXTRASOURCES ${UA_NODESET_TESTS_VALUE_MAPPING_SOURCES})
add_dependencies(check_nodeset_value_mapping open62541-generator-ns-tests-value-mapping)
add_test(NAME test_nodeset_compiler_regressions
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_compiler_regressions.py")

# Exercise the existing BSD frontend and one-argument namespace loader API.
if(UA_NAMESPACE_ZERO STREQUAL "FULL" AND UA_ENABLE_XML_ENCODING AND UA_ENABLE_TYPEDESCRIPTION)
    ua_generate_datatypes(NAME "types-tests-compiler-fixes"
        TARGET_SUFFIX "types-tests-compiler-fixes"
        FILE_CSV "${CMAKE_CURRENT_SOURCE_DIR}/compiler_regressions.csv"
        FILES_BSD "${CMAKE_CURRENT_SOURCE_DIR}/compiler_regressions.bsd"
        IMPORT_BSD "UA_TYPES#${UA_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
        OUTPUT_DIR "${GENERATE_OUTPUT_DIR}")
    ua_generate_nodeset(NAME "compiler-fixes"
        FILE "${CMAKE_CURRENT_SOURCE_DIR}/compiler_regressions.xml"
        FILES_BSD "${CMAKE_CURRENT_SOURCE_DIR}/compiler_regressions.bsd"
        TYPES_ARRAY "UA_TYPES_TESTS_COMPILER_FIXES"
        DEPENDS_NS ${open62541_NS0_NODESETS}
        DEPENDS_TYPES "UA_TYPES"
        OUTPUT_DIR "${GENERATE_OUTPUT_DIR}")
    ua_generate_nodeset(NAME "compiler-negative"
        FILE "${CMAKE_CURRENT_SOURCE_DIR}/compiler_regressions_negative.xml"
        FILES_BSD "${CMAKE_CURRENT_SOURCE_DIR}/compiler_regressions.bsd"
        DEPENDS_NS ${open62541_NS0_NODESETS} "${CMAKE_CURRENT_SOURCE_DIR}/compiler_regressions.xml"
        DEPENDS_TYPES "UA_TYPES" "UA_TYPES_TESTS_COMPILER_FIXES"
        OUTPUT_DIR "${GENERATE_OUTPUT_DIR}")
    ua_add_test(check_nodeset_compiler_regressions.c EXTRASOURCES
        ${UA_TYPES_TESTS_COMPILER_FIXES_SOURCES}
        ${UA_NODESET_COMPILER_FIXES_SOURCES}
        ${UA_NODESET_COMPILER_NEGATIVE_SOURCES})
    add_dependencies(check_nodeset_compiler_regressions open62541-generator-ns-compiler-fixes
                     open62541-generator-ns-compiler-negative open62541-generator-types-tests-compiler-fixes)
endif()
add_test(NAME test_nodeset_datatype_definitions
         COMMAND "${Python3_EXECUTABLE}"
                 "${CMAKE_CURRENT_SOURCE_DIR}/test_nodeset_datatype_definitions.py")
add_test(NAME test_datatype_definition_parser
         COMMAND "${Python3_EXECUTABLE}"
                 "${CMAKE_CURRENT_SOURCE_DIR}/test_datatype_definition_parser.py")
