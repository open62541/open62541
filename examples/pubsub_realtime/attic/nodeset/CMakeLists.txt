####################
# Nodeset Examples PubSub#
####################

###################
# Custom XML      #
###################

set(FILE_CSV_DIRPREFIX ${PROJECT_SOURCE_DIR}/pubsub_realtime/nodeset)
set(FILE_BSD_DIRPREFIX ${PROJECT_SOURCE_DIR}/pubsub_realtime/nodeset)
set(FILE_NS_DIRPREFIX ${PROJECT_SOURCE_DIR}/pubsub_realtime/nodeset)

get_target_property(OPEN62541_BIN_DIR open62541::open62541 BINARY_DIR)
set(PROJECT_BINARY_DIR ${OPEN62541_BIN_DIR})

# generate namespace from XML file
ua_generate_nodeset_and_datatypes(
    NAME "example_publisher"
    FILE_NS "${FILE_NS_DIRPREFIX}/pubDataModel.xml"
    DEPENDS "${CMAKE_CURRENT_LIST_DIR}/../../../tools/schema/Opc.Ua.NodeSet2.Reduced.xml"
)
ua_generate_nodeset_and_datatypes(
    NAME "example_subscriber"
    FILE_NS "${FILE_NS_DIRPREFIX}/subDataModel.xml"
    DEPENDS "${CMAKE_CURRENT_LIST_DIR}/../../../tools/schema/Opc.Ua.NodeSet2.Reduced.xml"
)
# The .csv file can be created from within UaModeler or manually
ua_generate_nodeid_header(
    NAME "example_nodeids_publisher"
    ID_PREFIX "EXAMPLE_NS_PUBLISHER"
    TARGET_SUFFIX "ids_example_publisher"
    FILE_CSV "${FILE_CSV_DIRPREFIX}/pubDataModel.csv"
)
ua_generate_nodeid_header(
    NAME "example_nodeids_subscriber"
    ID_PREFIX "EXAMPLE_NS_SUBSCRIBER"
    TARGET_SUFFIX "ids_example_subscriber"
    FILE_CSV "${FILE_CSV_DIRPREFIX}/subDataModel.csv"
)

add_example(pubsub_nodeset_rt_publisher pubsub_nodeset_rt_publisher.c
            ${UA_NODESET_EXAMPLE_PUBLISHER_SOURCES}
            ${PROJECT_BINARY_DIR}/src_generated/open62541/example_nodeids_publisher.h)
add_example(pubsub_nodeset_rt_subscriber pubsub_nodeset_rt_subscriber.c
            ${UA_NODESET_EXAMPLE_SUBSCRIBER_SOURCES}
            ${PROJECT_BINARY_DIR}/src_generated/open62541/example_nodeids_subscriber.h)
target_link_libraries(pubsub_nodeset_rt_publisher rt pthread)
target_link_libraries(pubsub_nodeset_rt_subscriber rt pthread)

