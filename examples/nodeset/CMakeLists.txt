####################
# Nodeset Examples #
####################

###################
# Custom XML      #
###################

# generate namespace from XML file
ua_generate_nodeset(
    NAME "example"
    FILE "${PROJECT_SOURCE_DIR}/examples/nodeset/server_nodeset.xml"
    DEPENDS_TYPES "UA_TYPES"
    DEPENDS_NS    "${UA_FILE_NS0}"
)

# The .csv file can be created from within UaModeler or manually

add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/example_nodeset_ids.h
                   PRE_BUILD
                   COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/generate_nodeid_header.py
                   ${PROJECT_SOURCE_DIR}/examples/nodeset/server_nodeset.csv  ${PROJECT_BINARY_DIR}/src_generated/example_nodeset_ids EXAMPLE_NS
                   DEPENDS ${PROJECT_SOURCE_DIR}/tools/generate_nodeid_header.py
                   ${PROJECT_SOURCE_DIR}/examples/nodeset/server_nodeset.csv)

add_example(server_nodeset server_nodeset.c ${PROJECT_BINARY_DIR}/src_generated/ua_namespace_example.c ${PROJECT_BINARY_DIR}/src_generated/example_nodeset_ids.h)
if(UA_COMPILE_AS_CXX)
    set_source_files_properties(${PROJECT_BINARY_DIR}/src_generated/ua_namespace_example.c PROPERTIES LANGUAGE CXX)
endif()

###################
# PLCopen Nodeset #
###################

# PLCopen requires the full ns0 as basis
if(UA_NAMESPACE_ZERO STREQUAL "FULL")

    # Generate types for DI namespace
    ua_generate_datatypes(
        NAME "ua_types_di"
        TARGET_SUFFIX "types-di"
        NAMESPACE_IDX 2
        FILE_CSV "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/OpcUaDiModel.csv"
        FILES_BSD "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/Opc.Ua.Di.Types.bsd"
    )

    # generate DI namespace
    ua_generate_nodeset(
        NAME "di"
        FILE "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/Opc.Ua.Di.NodeSet2.xml"
        TYPES_ARRAY "UA_TYPES_DI"
        INTERNAL
        DEPENDS_TYPES "UA_TYPES"
        DEPENDS_NS    "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/Schema/Opc.Ua.NodeSet2.xml"
        DEPENDS_TARGET "open62541-generator-types-di"
    )

    # generate PLCopen namespace which is using DI
    ua_generate_nodeset(
        NAME "plc"
        FILE "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/PLCopen/Opc.Ua.Plc.NodeSet2.xml"
        INTERNAL
        DEPENDS_TYPES
            "UA_TYPES" "UA_TYPES_DI"
        DEPENDS_NS
            "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/Schema/Opc.Ua.NodeSet2.xml"
            "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/DI/Opc.Ua.Di.NodeSet2.xml"
        DEPENDS_TARGET "open62541-generator-ns-di"
    )

    add_example(server_nodeset_plcopen server_nodeset_plcopen.c
                ${PROJECT_BINARY_DIR}/src_generated/ua_types_di_generated.c
                ${PROJECT_BINARY_DIR}/src_generated/ua_namespace_di.c
                ${PROJECT_BINARY_DIR}/src_generated/ua_namespace_plc.c)
    add_dependencies(server_nodeset_plcopen open62541-generator-ns-plc)
    target_include_directories(server_nodeset_plcopen PRIVATE ${PROJECT_SOURCE_DIR}/src) # needs an internal header

endif()
