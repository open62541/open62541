/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Using access_control_server
 */

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>

#include <stdlib.h>

int main(void) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    UA_StatusCode retval = UA_Client_connect_username(client, "opc.tcp://localhost:4840", "paula", "paula123");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }

    UA_NodeId newVariableIdRequest = UA_NODEID_NUMERIC(1, 1001);
    UA_NodeId newVariableId = UA_NODEID_NULL;

    UA_VariableAttributes newVariableAttributes = UA_VariableAttributes_default;

    newVariableAttributes.accessLevel = UA_ACCESSLEVELMASK_READ;
    newVariableAttributes.description = UA_LOCALIZEDTEXT_ALLOC("en-US", "NewVariable desc");
    newVariableAttributes.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US", "NewVariable");
    newVariableAttributes.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    UA_UInt32 value = 50;
    UA_Variant_setScalarCopy(&newVariableAttributes.value, &value, &UA_TYPES[UA_TYPES_UINT32]);

    UA_StatusCode retCode;

    retCode = UA_Client_addVariableNode(client, newVariableIdRequest,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "newVariable"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                            newVariableAttributes, &newVariableId);

    printf("addVariable returned: %s\n", UA_StatusCode_name(retCode));

    UA_ExpandedNodeId extNodeId = UA_EXPANDEDNODEID_NUMERIC(0, 0);
    extNodeId.nodeId = newVariableId;

    retCode = UA_Client_addReference(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_TRUE,
                            UA_STRING_NULL, extNodeId, UA_NODECLASS_VARIABLE);

    printf("addReference returned: %s\n", UA_StatusCode_name(retCode));

    retCode = UA_Client_deleteReference(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_TRUE, extNodeId,
                            UA_TRUE);

    printf("deleteReference returned: %s\n", UA_StatusCode_name(retCode));

    retCode = UA_Client_deleteNode(client, newVariableId, UA_TRUE);
    printf("deleteNode returned: %s\n", UA_StatusCode_name(retCode));

    /* Clean up */
    UA_VariableAttributes_clear(&newVariableAttributes);
    UA_Client_delete(client); /* Disconnects the client internally */
    return EXIT_SUCCESS;
}
