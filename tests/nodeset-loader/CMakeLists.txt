####################################################
# Test nodeset loader on a subset of nodeset files #
####################################################

if(UA_NAMESPACE_ZERO STREQUAL "FULL")

    add_compile_definitions(OPEN62541_NODESET_DIR="${UA_NODESET_DIR}/")
    ua_add_test(check_nodeset_loader_di.c)
    ua_add_test(check_nodeset_loader_adi.c)
    ua_add_test(check_nodeset_loader_autoid.c)
    ua_add_test(check_nodeset_loader_plc.c)

    add_compile_definitions(OPEN62541_TESTNODESET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/")
    ua_add_test(check_nodeset_loader_testnodeset.c)

    # Specific test for the OPCFoundation UA-Nodesets:
    #   branch: latest
    #   commit a2208e8
    ua_add_test(check_nodeset_loader_input.c)

endif()