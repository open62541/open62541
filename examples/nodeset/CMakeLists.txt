####################
# Nodeset Examples #
####################

###################
# Custom XML      #
###################

# generate namespace from XML file
ua_generate_nodeset_and_datatypes(
    NAME "example"
    FILE_NS "${PROJECT_SOURCE_DIR}/examples/nodeset/server_nodeset.xml"
)

# The .csv file can be created from within UaModeler or manually
ua_generate_nodeid_header(
    NAME "example_nodeids"
    ID_PREFIX "EXAMPLE_NS"
    FILE_CSV "${PROJECT_SOURCE_DIR}/examples/nodeset/server_nodeset.csv"
)

add_example(server_nodeset server_nodeset.c ${PROJECT_BINARY_DIR}/src_generated/ua_namespace_example.c ${PROJECT_BINARY_DIR}/src_generated/example_nodeids.h)
if(UA_COMPILE_AS_CXX)
    set_source_files_properties(${PROJECT_BINARY_DIR}/src_generated/ua_namespace_example.c PROPERTIES LANGUAGE CXX)
endif()

###################
# PLCopen Nodeset #
###################

# PLCopen requires the full ns0 as basis
if(UA_NAMESPACE_ZERO STREQUAL "FULL")

    # Generate types and namespace for DI
    ua_generate_nodeset_and_datatypes(
        NAME "di"
        FILE_CSV "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/OpcUaDiModel.csv"
        FILE_BSD "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/Opc.Ua.Di.Types.bsd"
        NAMESPACE_IDX 2
        FILE_NS "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/Opc.Ua.Di.NodeSet2.xml"
        INTERNAL
    )

    # generate PLCopen namespace which is using DI
    ua_generate_nodeset_and_datatypes(
        NAME "plc"
        # PLCopen does not define custom types. Only generate the nodeset
        FILE_NS "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/PLCopen/Opc.Ua.Plc.NodeSet2.xml"
        # PLCopen depends on the di nodeset, which must be generated before
        DEPENDS "di"
        INTERNAL
    )

    add_example(server_nodeset_plcopen server_nodeset_plcopen.c
                ${PROJECT_BINARY_DIR}/src_generated/ua_types_di_generated.c
                ${PROJECT_BINARY_DIR}/src_generated/ua_namespace_di.c
                ${PROJECT_BINARY_DIR}/src_generated/ua_namespace_plc.c)
    add_dependencies(server_nodeset_plcopen open62541-generator-ns-plc)
    target_include_directories(server_nodeset_plcopen PRIVATE ${PROJECT_SOURCE_DIR}/src) # needs an internal header

endif()
