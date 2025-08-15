/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 * Copyright (c) 2022 Linutronix GmbH (Author: Muddasir Shakil)
 * Copyright (c) 2025 Construction Future Lab gGmbH (Author: Jianbin Liu)
 */
/*
 * A simple server instance which registers with the discovery server (see server_lds.c).
 * Before shutdown it has to unregister itself.
 */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>

#define DISCOVERY_SERVER_ENDPOINT "opc.tcp://localhost:4840"

static UA_Boolean running = true;

/* Signal handler for graceful shutdown */
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

/*
 * Read callback for the data source variable
 * Called whenever a client reads the variable node.
 * The value is retrieved from the nodeContext pointer.
 */
static UA_StatusCode
readInteger(UA_Server *server, const UA_NodeId *sessionId,
            void *sessionContext, const UA_NodeId *nodeId,
            void *nodeContext, UA_Boolean includeSourceTimeStamp,
            const UA_NumericRange *range, UA_DataValue *value) {
    UA_Int32 *myInteger = (UA_Int32*)nodeContext;
    value->hasValue = true;
    UA_Variant_setScalarCopy(&value->value, myInteger, &UA_TYPES[UA_TYPES_INT32]);
    
    /* Optional: provide a source timestamp so clients see “live” data */
    value->hasSourceTimestamp = true;
    value->sourceTimestamp = UA_DateTime_now();

    /* Log the NodeId only if it is a string NodeId (defensive logging) */
    if(nodeId->identifierType == UA_NODEIDTYPE_STRING) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Node read %.*s",
                    (int)nodeId->identifier.string.length,
                    nodeId->identifier.string.data);
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "read value %i", *myInteger);
    return UA_STATUSCODE_GOOD;
}

/*
 * Write callback for the data source variable
 * Called whenever a client writes to the variable node.
 * The new value is stored in the nodeContext pointer.
 */
static UA_StatusCode
writeInteger(UA_Server *server, const UA_NodeId *sessionId,
             void *sessionContext, const UA_NodeId *nodeId,
             void *nodeContext, const UA_NumericRange *range,
             const UA_DataValue *value) {
    UA_Int32 *myInteger = (UA_Int32*)nodeContext;
    if(value->hasValue && UA_Variant_isScalar(&value->value) &&
       value->value.type == &UA_TYPES[UA_TYPES_INT32] && value->value.data)
        *myInteger = *(UA_Int32 *)value->value.data;

    /* Log the NodeId only if it is a string NodeId (defensive logging) */
    if(nodeId->identifierType == UA_NODEIDTYPE_STRING) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Node written %.*s",
                    (int)nodeId->identifier.string.length,
                    nodeId->identifier.string.data);
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "written value %i", *myInteger);
    return UA_STATUSCODE_GOOD;
}

/* Simple wrapper for UA_Server_registerDiscovery.
 * Creates a local UA_ClientConfig, sets it to default values and calls
 * UA_Server_registerDiscovery. The config is consumed internally. */
static UA_StatusCode
UA_Server_registerDiscovery_Simple(UA_Server *server,
                                   const UA_String discoveryEndpoint,
                                   const UA_String semaphoreFilePath) {
    UA_ClientConfig cc = (UA_ClientConfig){0};
    UA_ClientConfig_setDefault(&cc);
    return UA_Server_registerDiscovery(server, &cc, discoveryEndpoint, semaphoreFilePath);
}

/* Simple wrapper for UA_Server_deregisterDiscovery.
 * Creates a local UA_ClientConfig, sets it to default values and calls
 * UA_Server_deregisterDiscovery. The config is consumed internally. */
static UA_StatusCode
UA_Server_deregisterDiscovery_Simple(UA_Server *server,
                                     const UA_String discoveryServerUrl) {
    UA_ClientConfig cc = (UA_ClientConfig){0};
    UA_ClientConfig_setDefault(&cc);
    return UA_Server_deregisterDiscovery(server, &cc, discoveryServerUrl);
}

int main(int argc, char **argv) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */
    signal(SIGTERM, stopHandler);

    /* Tracks whether the server has successfully completed UA_Server_run_startup().
     * Used in cleanup to decide if UA_Server_run_shutdown() should be called. */
    UA_Boolean isServerRunning = false;

    /* Create and configure server instance */
    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setMinimal(config, 4841, NULL);

    /* Set application identification for discovery registration */
    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
        UA_String_fromChars("urn:open62541.example.server_register");
    config->mdnsConfig.mdnsServerName = UA_String_fromChars("Sample-Server");
    // See http://www.opcfoundation.org/UA/schemas/1.04/ServerCapabilities.csv
    //config.serverCapabilitiesSize = 1;
    //UA_String caps = UA_String_fromChars("LDS");
    //config.serverCapabilities = &caps;

    /* Create a data source variable node with read/write callbacks */
    UA_Int32 myInteger = 42;
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    
    /* Configure data source callbacks */
    UA_DataSource integerDataSource;
    integerDataSource.read = readInteger;
    integerDataSource.write = writeInteger;
    
    /* Set variable attributes */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en-US", "the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "the answer");

    /* Add the variable node to the address space */
    UA_StatusCode retval = UA_Server_addDataSourceVariableNode(server, myIntegerNodeId,
                                        UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(ORGANIZES),
                                        myIntegerName, UA_NODEID_NULL, attr, integerDataSource,
                                        &myInteger, NULL);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Could not add variable node. StatusCode %s",
                     UA_StatusCode_name(retval));
        goto cleanup;
    }

    /* Ensure the server stack finishes initialization successfully
     * before entering the main loop. If startup fails, clean up and exit. */
    retval = UA_Server_run_startup(server);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                    "startup failed: %s", UA_StatusCode_name(retval));
        goto cleanup;
    }
    isServerRunning = true;
    /* Register server with the discovery server using Simple wrapper */
    retval = UA_Server_registerDiscovery_Simple(
        server, UA_STRING(DISCOVERY_SERVER_ENDPOINT), UA_STRING_NULL);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Could not create periodic job for server register. StatusCode %s",
                     UA_StatusCode_name(retval));
        goto cleanup;
    }

    /* Main server loop - process client requests */
    while(running)
        UA_Server_run_iterate(server, true);

    /* Deregister server before shutdown */
    retval = UA_Server_deregisterDiscovery_Simple(server,
                                           UA_STRING(DISCOVERY_SERVER_ENDPOINT));
    if(retval != UA_STATUSCODE_GOOD){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Could not unregister server from discovery server. StatusCode %s",
                     UA_StatusCode_name(retval));
        goto cleanup;
    }

    /* Cleanup resources */
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    return EXIT_SUCCESS;

cleanup:
    if(isServerRunning)
        UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    return EXIT_FAILURE;
}
