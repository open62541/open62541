####################
# Nodeset Examples #
####################

###################
# Custom XML      #
###################

# Generate types for ADI namespace
ua_generate_datatypes(
    NAME "datatypes"
    TARGET_SUFFIX "example-datatypes"
    NAMESPACE_IDX 1
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}"
    FILE_CSV "${CMAKE_CURRENT_SOURCE_DIR}/datatypes.csv"
    FILES_BSD "${CMAKE_CURRENT_SOURCE_DIR}/datatypes.bsd"
)

# generate namespace from XML file
#ua_generate_nodeset_and_datatypes(
#    NAME "example"
#    FILE_NS "${CMAKE_CURRENT_SOURCE_DIR}/nodeset.xml"
#)

# The .csv file can be created from within UaModeler or manually
#ua_generate_nodeid_header(
#    NAME "example_nodeids"
#    ID_PREFIX "EXAMPLE_NS"
#    TARGET_SUFFIX "gendat_ids_example"
#    FILE_CSV "${CMAKE_CURRENT_SOURCE_DIR}/datatypes.csv"
#)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

#add_example(generated_datatypes_client client_datatypes.c)
add_example(generated_datatypes_server server_datatypes.c ${CMAKE_CURRENT_BINARY_DIR}/datatypes_generated.c)
add_dependencies(generated_datatypes_server open62541-generator-example-datatypes)