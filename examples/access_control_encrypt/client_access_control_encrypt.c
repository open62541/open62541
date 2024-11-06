/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 *
 */

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/securitypolicy.h>

#include <stdlib.h>
#include "common.h"

#define MIN_ARGS 4

int main(int argc, char* argv[]) {
    if(argc < MIN_ARGS) {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Arguments are missing. The required arguments are "
                     "<opc.tcp://host:port> "
                     "<client-certificate.der> <client-private-key.der> "
                     "[<trustlist1.crl>, ...]");
        return EXIT_SUCCESS;
    }

    const char *endpointUrl = argv[1];

    /* Load certificate and private key */
    UA_ByteString certificate = loadFile(argv[2]);
    UA_ByteString privateKey  = loadFile(argv[3]);

    /* Load the trustList */
    size_t trustListSize = 0;
    if(argc > MIN_ARGS)
        trustListSize = (size_t)argc-MIN_ARGS;
    UA_STACKARRAY(UA_ByteString, trustList, trustListSize+1);
    for(size_t trustListCount = 0; trustListCount < trustListSize; trustListCount++)
        trustList[trustListCount] = loadFile(argv[trustListCount+4]);

    /* Revocation lists are supported, but not used for the example here */
    UA_ByteString *revocationList = NULL;
    size_t revocationListSize = 0;

    UA_Client *client = UA_Client_new();
    UA_ClientConfig *config = UA_Client_getConfig(client);
    config->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    UA_String_clear(&config->clientDescription.applicationUri);
    config->clientDescription.applicationUri = UA_STRING_ALLOC("urn:open62541.server.application");
    UA_ClientConfig_setDefaultEncryption(config, certificate, privateKey,
                                         trustList, trustListSize,
                                         revocationList, revocationListSize);

    UA_ByteString_clear(&certificate);
    UA_ByteString_clear(&privateKey);
    for(size_t deleteCount = 0; deleteCount < trustListSize; deleteCount++) {
        UA_ByteString_clear(&trustList[deleteCount]);
    }

    UA_StatusCode retval = UA_Client_connectUsername(client, endpointUrl, "paula", "paula123");
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
                                        UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(ORGANIZES),
                                        UA_QUALIFIEDNAME(1, "newVariable"),
                                        UA_NS0ID(BASEDATAVARIABLETYPE),
                                        newVariableAttributes, &newVariableId);

    printf("addVariable returned: %s\n", UA_StatusCode_name(retCode));

    UA_ExpandedNodeId extNodeId = UA_EXPANDEDNODEID_NUMERIC(0, 0);
    extNodeId.nodeId = newVariableId;

    retCode = UA_Client_addReference(client, UA_NS0ID(OBJECTSFOLDER),
                                     UA_NS0ID(HASCOMPONENT), UA_TRUE,
                                     UA_STRING_NULL, extNodeId, UA_NODECLASS_VARIABLE);

    printf("addReference returned: %s\n", UA_StatusCode_name(retCode));

    retCode = UA_Client_deleteReference(client, UA_NS0ID(OBJECTSFOLDER),
                                        UA_NS0ID(ORGANIZES), UA_TRUE, extNodeId, UA_TRUE);

    printf("deleteReference returned: %s\n", UA_StatusCode_name(retCode));

    retCode = UA_Client_deleteNode(client, newVariableId, UA_TRUE);
    printf("deleteNode returned: %s\n", UA_StatusCode_name(retCode));

    /* Clean up */
    UA_VariableAttributes_clear(&newVariableAttributes);
    UA_Client_delete(client); /* Disconnects the client internally */
    return EXIT_SUCCESS;
}
