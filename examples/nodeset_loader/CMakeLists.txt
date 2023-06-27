###########################
# Nodeset Loader Examples #
###########################

if(UA_NAMESPACE_ZERO STREQUAL "FULL")
    add_example(nodeset_loader nodeset_loader.c)
    add_example(server_nodeset_loader server_nodeset_loader.c)
endif()
