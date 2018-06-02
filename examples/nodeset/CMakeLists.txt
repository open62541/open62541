####################
# Nodeset Examples #
####################

###################
# Custom XML      #
###################

# generate namespace from XML file
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/example_nodeset.c
                   ${PROJECT_BINARY_DIR}/src_generated/example_nodeset.h
                   PRE_BUILD
                   COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodeset_compiler.py
                   --types-array=UA_TYPES
                   --existing ${UA_FILE_NS0}
                   --xml ${PROJECT_SOURCE_DIR}/examples/nodeset/server_nodeset.xml
                   ${PROJECT_BINARY_DIR}/src_generated/example_nodeset
                   DEPENDS ${UA_FILE_NS0}
                   ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodeset_compiler.py
                   ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodes.py
                   ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodeset.py
                   ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/datatypes.py
                   ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/backend_open62541.py
                   ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/backend_open62541_nodes.py
                   ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/backend_open62541_datatypes.py
                   ${PROJECT_SOURCE_DIR}/examples/nodeset/server_nodeset.xml)


# The .csv file can be created from within UaModeler or manually

add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/example_nodeset_ids.h
                   PRE_BUILD
                   COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/generate_nodeid_header.py
                   ${PROJECT_SOURCE_DIR}/examples/nodeset/server_nodeset.csv  ${PROJECT_BINARY_DIR}/src_generated/example_nodeset_ids EXAMPLE_NS
                   DEPENDS ${PROJECT_SOURCE_DIR}/tools/generate_nodeid_header.py
                   ${PROJECT_SOURCE_DIR}/examples/nodeset/server_nodeset.csv)

add_example(server_nodeset server_nodeset.c ${PROJECT_BINARY_DIR}/src_generated/example_nodeset.c ${PROJECT_BINARY_DIR}/src_generated/example_nodeset_ids.h)
if(UA_COMPILE_AS_CXX)
    set_source_files_properties(${PROJECT_BINARY_DIR}/src_generated/example_nodeset.c PROPERTIES LANGUAGE CXX)
endif()

###################
# PLCopen Nodeset #
###################

# PLCopen requires the full ns0 as basis
if(UA_ENABLE_FULL_NS0)

    # Generate types for DI namespace
    set(UA_TYPES_OUT "ua_types_di")
    add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/${UA_TYPES_OUT}_generated.c
                       ${PROJECT_BINARY_DIR}/src_generated/${UA_TYPES_OUT}_generated.h
                       ${PROJECT_BINARY_DIR}/src_generated/${UA_TYPES_OUT}_generated_handling.h
                       ${PROJECT_BINARY_DIR}/src_generated/${UA_TYPES_OUT}_generated_encoding_binary.h
                       PRE_BUILD
                       COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/generate_datatypes.py
                       --namespace=2
                       --type-csv=${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/OpcUaDiModel.csv
                       --type-bsd=${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/Opc.Ua.Di.Types.bsd
                       --no-builtin
                       ${PROJECT_BINARY_DIR}/src_generated/${UA_TYPES_OUT}
                       DEPENDS ${PROJECT_SOURCE_DIR}/tools/generate_datatypes.py
                       ${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/OpcUaDiModel.csv
                       ${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/Opc.Ua.Di.Types.bsd)
    add_custom_target(open62541-generator-types-di DEPENDS ${PROJECT_BINARY_DIR}/src_generated/${UA_TYPES_OUT}_generated.c)

    # generate DI namespace
    add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/ua_namespace_di.c
                       ${PROJECT_BINARY_DIR}/src_generated/ua_namespace_di.h
                       PRE_BUILD
                       COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodeset_compiler.py
                       --internal-headers
                       --types-array=UA_TYPES
                       --types-array=UA_TYPES_DI
                       --existing ${PROJECT_SOURCE_DIR}/deps/ua-nodeset/Schema/Opc.Ua.NodeSet2.xml
                       --xml ${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/Opc.Ua.Di.NodeSet2.xml
                       ${PROJECT_BINARY_DIR}/src_generated/ua_namespace_di
                       DEPENDS ${UA_FILE_NS0}
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
    add_custom_target(open62541-generator-ns-di DEPENDS ${PROJECT_BINARY_DIR}/src_generated/ua_namespace_di.c)
    add_dependencies(open62541-generator-ns-di open62541-generator-types-di)

    # generate PLCopen namespace which is using DI
    add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/ua_namespace_plc.c
                       ${PROJECT_BINARY_DIR}/src_generated/ua_namespace_plc.h
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
                       ${PROJECT_BINARY_DIR}/src_generated/ua_namespace_plc
                       DEPENDS ${UA_FILE_NS0}
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodeset_compiler.py
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodes.py
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodeset.py
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/datatypes.py
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/backend_open62541.py
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/backend_open62541_nodes.py
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/backend_open62541_datatypes.py
                       ${PROJECT_SOURCE_DIR}/deps/ua-nodeset/Schema/Opc.Ua.NodeSet2.xml
                       ${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/Opc.Ua.Di.NodeSet2.xml
                       ${PROJECT_SOURCE_DIR}/deps/ua-nodeset/PLCopen/Opc.Ua.Plc.NodeSet2.xml
                       )
    add_custom_target(open62541-generator-ns-plc DEPENDS ${PROJECT_BINARY_DIR}/src_generated/ua_namespace_plc.c)
    add_dependencies(open62541-generator-ns-plc open62541-generator-ns-di)

    add_example(server_nodeset_plcopen server_nodeset_plcopen.c
                ${PROJECT_BINARY_DIR}/src_generated/ua_types_di_generated.c
                ${PROJECT_BINARY_DIR}/src_generated/ua_namespace_di.c
                ${PROJECT_BINARY_DIR}/src_generated/ua_namespace_plc.c)
    add_dependencies(server_nodeset_plcopen open62541-generator-ns-plc)
    target_include_directories(server_nodeset_plcopen PRIVATE ${PROJECT_SOURCE_DIR}/src) # needs an internal header

    if(UA_COMPILE_AS_CXX)
        set_source_files_properties(${PROJECT_BINARY_DIR}/src_generated/ua_types_di_generated.c PROPERTIES LANGUAGE CXX)
        set_source_files_properties(${PROJECT_BINARY_DIR}/src_generated/ua_namespace_di.c PROPERTIES LANGUAGE CXX)
        set_source_files_properties(${PROJECT_BINARY_DIR}/src_generated/ua_namespace_plc.c PROPERTIES LANGUAGE CXX)
    endif()
endif()
