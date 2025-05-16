/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <signal.h>

/* Build Instructions (Linux)
 * - g++ server.cpp -lopen62541 -o server */

using namespace std;

UA_Boolean running = true;

static void
stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "stopping server");
}

int main() {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    // Configure server endpoint
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->endpointsSize = 1;
    config->endpoints = (UA_EndpointDescription*)UA_Array_new(1, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    UA_EndpointDescription_init(&config->endpoints[0]);
    config->endpoints[0].endpointUrl = UA_STRING_ALLOC("opc.tcp://localhost:4840");
    config->endpoints[0].securityMode = UA_MESSAGESECURITYMODE_NONE;
    config->endpoints[0].securityPolicyUri = UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#None");

    // Add server description
    config->applicationDescription.applicationUri = UA_STRING_ALLOC("urn:open62541.server.application");
    config->applicationDescription.productUri = UA_STRING_ALLOC("urn:open62541.server");
    config->applicationDescription.applicationName = UA_LOCALIZEDTEXT_ALLOC("en-US", "open62541-based OPC UA Server");

    // Fix timestamp warning
    config->verifyRequestTimestamp = UA_RULEHANDLING_ACCEPT;

    // add a variable node to the adresspace
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalarCopy(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT_ALLOC("en-US","the answer");
    attr.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US","the answer");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING_ALLOC(1, "the.answer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME_ALLOC(1, "the answer");
    UA_NodeId parentNodeId = UA_NS0ID(OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NS0ID(ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                              parentReferenceNodeId, myIntegerName,
                              UA_NODEID_NULL, attr, NULL, NULL);

    // Add a second variable that changes periodically
    UA_VariableAttributes attr2 = UA_VariableAttributes_default;
    UA_Double myDouble = 0.0;
    UA_Variant_setScalarCopy(&attr2.value, &myDouble, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr2.description = UA_LOCALIZEDTEXT_ALLOC("en-US","counter");
    attr2.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US","counter");
    attr2.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_NodeId myDoubleNodeId = UA_NODEID_STRING_ALLOC(1, "counter");
    UA_QualifiedName myDoubleName = UA_QUALIFIEDNAME_ALLOC(1, "counter");
    UA_Server_addVariableNode(server, myDoubleNodeId, parentNodeId,
                             parentReferenceNodeId, myDoubleName,
                             UA_NODEID_NULL, attr2, NULL, NULL);

    /* allocations on the heap need to be freed */
    UA_VariableAttributes_clear(&attr);
    UA_VariableAttributes_clear(&attr2);
    UA_NodeId_clear(&myIntegerNodeId);
    UA_NodeId_clear(&myDoubleNodeId);
    UA_QualifiedName_clear(&myIntegerName);
    UA_QualifiedName_clear(&myDoubleName);

    UA_StatusCode retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}