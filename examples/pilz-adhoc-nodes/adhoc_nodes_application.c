/**
 * Custom Nodestore Server Example
 * 
 * This example demonstrates how to configure an OPC UA server to use the
 * custom Pilz nodestore implementation instead of the default one.
 * 
 * The server starts with a standard configuration but replaces the nodestore
 * with our custom hashmap-based implementation. Currently this implementation
 * is identical to the default hashmap nodestore and will be extended step by step.
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include "adhoc_nodestore.h"

#include <signal.h>
#include <stdlib.h>

static volatile UA_Boolean running = true;

static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

/**
 * Test dynamic node retrieval from mock backend
 */
static UA_StatusCode
testDynamicNodes(UA_Server *server) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "Testing dynamic node retrieval from mock backend...");

    /* Try to read a mock backend node using the server API */
    UA_NodeId testNodeId = UA_NODEID_NUMERIC(1, 1000); /* PilzDeviceRoot */
    
    /* Use UA_Server_readBrowseName to test if the node can be retrieved */
    UA_QualifiedName browseName;
    UA_QualifiedName_init(&browseName);
    
    UA_StatusCode retval = UA_Server_readBrowseName(server, testNodeId, &browseName);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                    "SUCCESS: Retrieved mock backend node ID 1000: %.*s",
                    (int)browseName.name.length,
                    browseName.name.data);
        UA_QualifiedName_clear(&browseName);
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "FAILED: Could not retrieve node 1000 from mock backend: %s",
                     UA_StatusCode_name(retval));
    }

    /* Test a few more nodes */
    for(UA_UInt32 nodeId = 1001; nodeId <= 1005; nodeId++) {
        UA_NodeId testId = UA_NODEID_NUMERIC(1, nodeId);
        UA_QualifiedName testBrowseName;
        UA_QualifiedName_init(&testBrowseName);
        
        UA_StatusCode testRetval = UA_Server_readBrowseName(server, testId, &testBrowseName);
        if(testRetval == UA_STATUSCODE_GOOD) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                        "SUCCESS: Retrieved node %u: %.*s",
                        nodeId,
                        (int)testBrowseName.name.length,
                        testBrowseName.name.data);
            UA_QualifiedName_clear(&testBrowseName);
        } else {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                        "Node %u not found (expected for some nodes): %s",
                        nodeId, UA_StatusCode_name(testRetval));
        }
    }

    return UA_STATUSCODE_GOOD;
}

/**
 * Add some test nodes to demonstrate nodestore functionality
 */
static UA_StatusCode
addTestNodes(UA_Server *server) {
    /* Test dynamic node retrieval instead of adding conflicting nodes */
    return testDynamicNodes(server);
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    /* Create a server configuration */
    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_StatusCode retval = UA_ServerConfig_setDefault(config);
    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Could not create default server configuration: %s",
                     UA_StatusCode_name(retval));
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    /* Replace the default nodestore with our custom Pilz implementation */
    config->nodestore->free(config->nodestore);
    config->nodestore = UA_Nodestore_PilzAdHoc();;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    /* Set the server description */
    config->applicationDescription.applicationName =
        UA_LOCALIZEDTEXT_ALLOC("en-US", "OPC UA Server with Custom Pilz Nodestore");
    config->applicationDescription.applicationUri =
        UA_STRING_ALLOC("urn:open62541.example.pilz_nodestore");

    /* Add test nodes to demonstrate the custom nodestore functionality */
    retval = addTestNodes(server);
    if (retval != UA_STATUSCODE_GOOD) {
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    /* Run the server */
    UA_Server_runUntilInterrupt(server);

    /* Shutdown and clean up */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Server shutdown complete");
    UA_Server_delete(server);
    return EXIT_SUCCESS;
}
