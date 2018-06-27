file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/src_generated/tests")
include_directories("${PROJECT_BINARY_DIR}/src_generated/tests")

macro(generate_dataset xml source)
    add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/tests/${source}.c
                              ${PROJECT_BINARY_DIR}/src_generated/tests/${source}.h
                       PRE_BUILD
                       COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodeset_compiler.py
                               --types-array=UA_TYPES
                               --existing ${UA_FILE_NS0}
                               --xml ${xml}
                               ${PROJECT_BINARY_DIR}/src_generated/tests/${source}
                       DEPENDS ${UA_FILE_NS0} ${xml}
                               ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodeset_compiler.py
                               ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodes.py
                               ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodeset.py
                               ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/datatypes.py
                               ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/backend_open62541.py
                               ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/backend_open62541_nodes.py
                               ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/backend_open62541_datatypes.py)
endmacro()

# Check ObjectType
generate_dataset(${PROJECT_SOURCE_DIR}/tests/nodeset-compiler/objecttype.xml check_nodeset_objecttype_generated)
add_executable(check_nodeset_objecttype check_nodeset_objecttype.c
               ${PROJECT_BINARY_DIR}/src_generated/tests/check_nodeset_objecttype_generated.c
               $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_nodeset_objecttype ${LIBS})
add_test_valgrind(nodeset_objecttype ${TESTS_BINARY_DIR}/check_nodeset_objecttype)

###############################################
# Test Companion Specs that need the full NS0 #
###############################################

if(UA_ENABLE_FULL_NS0)

    # Generate types for DI namespace
    set(UA_TYPES_OUT "ua_types_di")
    add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/tests/${UA_TYPES_OUT}_generated.c
                       ${PROJECT_BINARY_DIR}/src_generated/tests/${UA_TYPES_OUT}_generated.h
                       ${PROJECT_BINARY_DIR}/src_generated/tests/${UA_TYPES_OUT}_generated_handling.h
                       ${PROJECT_BINARY_DIR}/src_generated/tests/${UA_TYPES_OUT}_generated_encoding_binary.h
                       PRE_BUILD
                       COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/generate_datatypes.py
                       --namespace=2
                       --type-csv=${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/OpcUaDiModel.csv
                       --type-bsd=${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/Opc.Ua.Di.Types.bsd
                       --no-builtin
                       ${PROJECT_BINARY_DIR}/src_generated/tests/${UA_TYPES_OUT}
                       DEPENDS ${PROJECT_SOURCE_DIR}/tools/generate_datatypes.py
                       ${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/OpcUaDiModel.csv
                       ${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/Opc.Ua.Di.Types.bsd)
    add_custom_target(open62541-generator-tests-types-di DEPENDS ${PROJECT_BINARY_DIR}/src_generated/tests/${UA_TYPES_OUT}_generated.c)

    # Generate types for ADI namespace
    set(UA_TYPES_OUT "ua_types_adi")
    add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/tests/${UA_TYPES_OUT}_generated.c
                       ${PROJECT_BINARY_DIR}/src_generated/tests/${UA_TYPES_OUT}_generated.h
                       ${PROJECT_BINARY_DIR}/src_generated/tests/${UA_TYPES_OUT}_generated_handling.h
                       ${PROJECT_BINARY_DIR}/src_generated/tests/${UA_TYPES_OUT}_generated_encoding_binary.h
                       PRE_BUILD
                       COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/generate_datatypes.py
                       --namespace=3
                       --type-csv=${PROJECT_SOURCE_DIR}/deps/ua-nodeset/ADI/OpcUaAdiModel.csv
                       --type-bsd=${PROJECT_SOURCE_DIR}/deps/ua-nodeset/ADI/Opc.Ua.Adi.Types.bsd
                       --no-builtin
                       ${PROJECT_BINARY_DIR}/src_generated/tests/${UA_TYPES_OUT}
                       DEPENDS ${PROJECT_SOURCE_DIR}/tools/generate_datatypes.py
                       ${PROJECT_SOURCE_DIR}/deps/ua-nodeset/ADI/OpcUaAdiModel.csv
                       ${PROJECT_SOURCE_DIR}/deps/ua-nodeset/ADI/Opc.Ua.Adi.Types.bsd)
    add_custom_target(open62541-generator-tests-types-adi DEPENDS ${PROJECT_BINARY_DIR}/src_generated/tests/${UA_TYPES_OUT}_generated.c)

    # generate DI namespace
    add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/tests/ua_namespace_di.c
                       ${PROJECT_BINARY_DIR}/src_generated/tests/ua_namespace_di.h
                       PRE_BUILD
                       COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodeset_compiler.py
                       --internal-headers
                       --types-array=UA_TYPES
                       --types-array=UA_TYPES_DI
                       --existing ${PROJECT_SOURCE_DIR}/deps/ua-nodeset/Schema/Opc.Ua.NodeSet2.xml
                       --xml ${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/Opc.Ua.Di.NodeSet2.xml
                       ${PROJECT_BINARY_DIR}/src_generated/tests/ua_namespace_di
                       DEPENDS ${UA_NAMESPACE0_XML}
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodeset_compiler.py
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodes.py
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodeset.py
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/datatypes.py
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/backend_open62541.py
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/backend_open62541_nodes.py
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/backend_open62541_datatypes.py
                       ${PROJECT_SOURCE_DIR}/deps/ua-nodeset/Schema/Opc.Ua.NodeSet2.xml
                       ${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/Opc.Ua.Di.NodeSet2.xml
                       )
    add_custom_target(open62541-generator-tests-ns-di DEPENDS ${PROJECT_BINARY_DIR}/src_generated/tests/ua_namespace_di.c)
    add_dependencies(open62541-generator-tests-ns-di open62541-generator-tests-types-di)

    # generate ADI namespace which is using DI
    add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/tests/ua_namespace_adi.c
                       ${PROJECT_BINARY_DIR}/src_generated/tests/ua_namespace_adi.h
                       PRE_BUILD
                       COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodeset_compiler.py
                       --internal-headers
                       --types-array=UA_TYPES
                       --types-array=UA_TYPES_DI
                       --types-array=UA_TYPES_ADI
                       --existing ${PROJECT_SOURCE_DIR}/deps/ua-nodeset/Schema/Opc.Ua.NodeSet2.xml
                       --existing ${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/Opc.Ua.Di.NodeSet2.xml
                       --xml ${PROJECT_SOURCE_DIR}/deps/ua-nodeset/ADI/Opc.Ua.Adi.NodeSet2.xml
                       ${PROJECT_BINARY_DIR}/src_generated/tests/ua_namespace_adi
                       DEPENDS ${UA_NAMESPACE0_XML}
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodeset_compiler.py
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodes.py
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodeset.py
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/datatypes.py
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/backend_open62541.py
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/backend_open62541_nodes.py
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/backend_open62541_datatypes.py
                       ${PROJECT_SOURCE_DIR}/deps/ua-nodeset/Schema/Opc.Ua.NodeSet2.xml
                       ${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/Opc.Ua.Di.NodeSet2.xml
                       ${PROJECT_SOURCE_DIR}/deps/ua-nodeset/ADI/Opc.Ua.Adi.NodeSet2.xml
                       )
    add_custom_target(open62541-generator-tests-ns-adi DEPENDS ${PROJECT_BINARY_DIR}/src_generated/tests/ua_namespace_adi.c)
    add_dependencies(open62541-generator-tests-ns-adi open62541-generator-tests-types-adi open62541-generator-tests-ns-di)

    # generate PLCopen namespace which is using DI
    add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/tests/ua_namespace_plc.c
                       ${PROJECT_BINARY_DIR}/src_generated/tests/ua_namespace_plc.h
                       PRE_BUILD
                       COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodeset_compiler.py
                       --internal-headers
                       --types-array=UA_TYPES
                       --types-array=UA_TYPES_DI
                       # PLCopen has no specific type definition, thus use the default UA_TYPES to ignore it
                       --types-array=UA_TYPES
                       --existing ${PROJECT_SOURCE_DIR}/deps/ua-nodeset/Schema/Opc.Ua.NodeSet2.xml
                       --existing ${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/Opc.Ua.Di.NodeSet2.xml
                       --xml ${PROJECT_SOURCE_DIR}/deps/ua-nodeset/PLCopen/Opc.Ua.Plc.NodeSet2.xml
                       ${PROJECT_BINARY_DIR}/src_generated/tests/ua_namespace_plc
                       DEPENDS ${UA_NAMESPACE0_XML}
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodeset_compiler.py
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodes.py
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodeset.py
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/datatypes.py
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/backend_open62541.py
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/backend_open62541_nodes.py
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/backend_open62541_datatypes.py
                       ${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/Opc.Ua.Di.NodeSet2.xml
                       ${PROJECT_SOURCE_DIR}/deps/ua-nodeset/PLCopen/Opc.Ua.Plc.NodeSet2.xml
                       )
    add_custom_target(open62541-generator-tests-ns-plc DEPENDS ${PROJECT_BINARY_DIR}/src_generated/tests/ua_namespace_plc.c)
    add_dependencies(open62541-generator-tests-ns-plc open62541-generator-tests-ns-di)

    add_executable(check_nodeset_compiler_adi check_nodeset_compiler_adi.c
                   ${PROJECT_BINARY_DIR}/src_generated/tests/ua_types_di_generated.c
                   ${PROJECT_BINARY_DIR}/src_generated/tests/ua_namespace_di.c
                   ${PROJECT_BINARY_DIR}/src_generated/tests/ua_types_adi_generated.c
                   ${PROJECT_BINARY_DIR}/src_generated/tests/ua_namespace_adi.c
                   $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    add_dependencies(check_nodeset_compiler_adi open62541-generator-tests-ns-adi)
    target_link_libraries(check_nodeset_compiler_adi ${LIBS})
    add_test_valgrind(nodeset_compiler_adi ${TESTS_BINARY_DIR}/check_nodeset_compiler_adi)

    add_executable(check_nodeset_compiler_plc check_nodeset_compiler_plc.c
                   ${PROJECT_BINARY_DIR}/src_generated/tests/ua_types_di_generated.c
                   ${PROJECT_BINARY_DIR}/src_generated/tests/ua_namespace_di.c
                   ${PROJECT_BINARY_DIR}/src_generated/tests/ua_namespace_plc.c
                   $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
    add_dependencies(check_nodeset_compiler_plc open62541-generator-tests-ns-plc)
    target_link_libraries(check_nodeset_compiler_plc ${LIBS})
    add_test_valgrind(nodeset_compiler_plc ${TESTS_BINARY_DIR}/check_nodeset_compiler_plc)
endif()
