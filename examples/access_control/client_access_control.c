/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Using access_control_server
 */

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>

#include <stdlib.h>

int main(int argc, char **argv) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    if (argc < 3) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Error: Please provide Endpoint eg., opc.tcp://172.17.4.10:4840, \
                      Username and password as command line arguments");
        return EXIT_FAILURE;
    }

    UA_StatusCode retval = UA_Client_connectUsername(client, argv[1], argv[2], argv[3]);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }

    UA_VariableAttributes newVariableAttributes = UA_VariableAttributes_default;

    newVariableAttributes.accessLevel = UA_ACCESSLEVELMASK_READ;
    newVariableAttributes.description = UA_LOCALIZEDTEXT_ALLOC("en-US", "NewVariable desc");
    newVariableAttributes.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US", "NewVariable");
    newVariableAttributes.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    UA_UInt32 value = 50;
    UA_Variant_setScalarCopy(&newVariableAttributes.value, &value, &UA_TYPES[UA_TYPES_UINT32]);

    UA_StatusCode retCode;

    retCode = UA_Client_addVariableNode(client, UA_NODEID_NUMERIC(1, 3001),
                                        UA_NODEID_NUMERIC(1, 200000),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                        UA_QUALIFIEDNAME(1, "newVariable_1"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                        newVariableAttributes, NULL);

    printf("addVariable 1 returned: %s\n", UA_StatusCode_name(retCode));

    retCode = UA_Client_addVariableNode(client, UA_NODEID_NUMERIC(1, 3002),
                                        UA_NODEID_NUMERIC(1, 200000),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                        UA_QUALIFIEDNAME(1, "newVariable_2"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                        newVariableAttributes, NULL);

    printf("addVariable 2 returned: %s\n", UA_StatusCode_name(retCode));

    retCode = UA_Client_addVariableNode(client, UA_NODEID_NUMERIC(1, 3003),
                                        UA_NODEID_NUMERIC(1, 200000),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                        UA_QUALIFIEDNAME(1, "newVariable_3"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                        newVariableAttributes, NULL);

    printf("addVariable 3 returned: %s\n", UA_StatusCode_name(retCode));

    retCode = UA_Client_addVariableNode(client, UA_NODEID_NUMERIC(1, 3004),
                                        UA_NODEID_NUMERIC(1, 200000),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                        UA_QUALIFIEDNAME(1, "newVariable_4"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                        newVariableAttributes, NULL);

    printf("addVariable 4 returned: %s\n", UA_StatusCode_name(retCode));

    /* Add References for 4 nodes */
    UA_ExpandedNodeId extNodeId_1 = UA_EXPANDEDNODEID_NUMERIC(0, 0);
    extNodeId_1.nodeId = UA_NODEID_NUMERIC(1, 3001);

    retCode = UA_Client_addReference(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_TRUE,
                                     UA_STRING_NULL, extNodeId_1, UA_NODECLASS_VARIABLE);

    printf("addReference 1 returned: %s\n", UA_StatusCode_name(retCode));

    UA_ExpandedNodeId extNodeId_2 = UA_EXPANDEDNODEID_NUMERIC(0, 0);
    extNodeId_2.nodeId = UA_NODEID_NUMERIC(1, 3002);

    retCode = UA_Client_addReference(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_TRUE,
                                     UA_STRING_NULL, extNodeId_2, UA_NODECLASS_VARIABLE);

    printf("addReference 2 returned: %s\n", UA_StatusCode_name(retCode));

    UA_ExpandedNodeId extNodeId_3 = UA_EXPANDEDNODEID_NUMERIC(0, 0);
    extNodeId_3.nodeId = UA_NODEID_NUMERIC(1, 3003);

    retCode = UA_Client_addReference(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_TRUE,
                                     UA_STRING_NULL, extNodeId_3, UA_NODECLASS_VARIABLE);

    printf("addReference 3 returned: %s\n", UA_StatusCode_name(retCode));

    UA_ExpandedNodeId extNodeId_4 = UA_EXPANDEDNODEID_NUMERIC(0, 0);
    extNodeId_4.nodeId = UA_NODEID_NUMERIC(1, 3004);

    retCode = UA_Client_addReference(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_TRUE,
                                     UA_STRING_NULL, extNodeId_4, UA_NODECLASS_VARIABLE);

    printf("addReference 4 returned: %s\n", UA_StatusCode_name(retCode));
    /* Added 4 references */

    /* Delete first two references */
    retCode = UA_Client_deleteReference(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_TRUE, extNodeId_1,
                                        UA_TRUE);

    printf("deleteReference 1 returned: %s\n", UA_StatusCode_name(retCode));

    retCode = UA_Client_deleteReference(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_TRUE, extNodeId_2,
                                        UA_TRUE);

    printf("deleteReference 2 returned: %s\n", UA_StatusCode_name(retCode));
    /* Deleted first two references */

    /* Delete first two newly added nodes */
    retCode = UA_Client_deleteNode(client, UA_NODEID_NUMERIC(1, 3001), UA_TRUE);
    printf("deleteNode 1 returned: %s\n", UA_StatusCode_name(retCode));

    retCode = UA_Client_deleteNode(client, UA_NODEID_NUMERIC(1, 3002), UA_TRUE);
    printf("deleteNode 2 returned: %s\n", UA_StatusCode_name(retCode));
    /* Deleted first two newly added nodes */

    /* Clean up */
    UA_VariableAttributes_clear(&newVariableAttributes);
    UA_Client_delete(client); /* Disconnects the client internally */
    return EXIT_SUCCESS;
}
