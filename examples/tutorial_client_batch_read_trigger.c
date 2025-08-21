/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Batch Read Trigger Client
 * -------------------------
 *
 * This client example demonstrates how to trigger batch read operations
 * on the batch RW server. It performs multiple read requests containing
 * multiple nodes to demonstrate the batching behavior.
 *
 * Usage:
 * 1. Start the batch RW server first: ./tutorial_server_batch_rw
 * 2. Run this client: ./tutorial_client_batch_read_trigger
 *
 * The client will perform 5 batch read operations with 3-second intervals.
 */

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Configuration */
#define SERVER_URL "opc.tcp://localhost:4840"
#define VARIABLE_COUNT 5
#define READ_ITERATIONS 5
#define READ_INTERVAL_SEC 3

/* Variable names to read */
static const char *variableNames[] = {
    "variable-1",
    "variable-2", 
    "variable-3",
    "variable-4",
    "variable-5"
};

/* Perform a batch read of all variables */
static UA_StatusCode
performBatchRead(UA_Client *client, int iteration) {
    printf("\n--- Batch Read Iteration %d ---\n", iteration + 1);
    
    /* Create read request with all variables */
    UA_ReadValueId items[VARIABLE_COUNT];
    for (int i = 0; i < VARIABLE_COUNT; i++) {
        UA_ReadValueId_init(&items[i]);
        items[i].nodeId = UA_NODEID_STRING(1, variableNames[i]);
        items[i].attributeId = UA_ATTRIBUTEID_VALUE;
    }
    
    UA_ReadRequest req;
    UA_ReadRequest_init(&req);
    req.nodesToRead = items;
    req.nodesToReadSize = VARIABLE_COUNT;
    
    printf("Requesting batch read of %d variables...\n", VARIABLE_COUNT);
    
    /* Perform the read request */
    UA_ReadResponse resp = UA_Client_Service_read(client, req);
    
    /* Process results */
    if (resp.responseHeader.serviceResult == UA_STATUSCODE_GOOD) {
        printf("Batch read completed successfully!\n");
        for (size_t i = 0; i < resp.resultsSize; ++i) {
            if (resp.results[i].hasValue && resp.results[i].value.type == &UA_TYPES[UA_TYPES_UINT32]) {
                UA_UInt32 val = *(UA_UInt32*)resp.results[i].value.data;
                printf("  %s = %u\n", variableNames[i], val);
            } else {
                printf("  %s = <error or no value>\n", variableNames[i]);
            }
        }
    } else {
        printf("Batch read failed: %s\n", UA_StatusCode_name(resp.responseHeader.serviceResult));
    }
    
    /* Cleanup */
    UA_ReadResponse_clear(&resp);
    
    return UA_STATUSCODE_GOOD;
}

int main(void) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    
    printf("Batch Read Trigger Client (Sync API)\n");
    printf("===================================\n");
    printf("This client will trigger batch read operations on the server.\n");
    printf("Make sure the batch RW server is running first!\n\n");
    
    /* Connect to the server */
    UA_StatusCode retval = UA_Client_connect(client, SERVER_URL);
    if (retval != UA_STATUSCODE_GOOD) {
        printf("Failed to connect to server: %s\n", UA_StatusCode_name(retval));
        printf("Make sure the batch RW server is running at %s\n", SERVER_URL);
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }
    
    printf("Connected to server successfully!\n");
    printf("Will perform %d batch read operations with %d-second intervals.\n", 
           READ_ITERATIONS, READ_INTERVAL_SEC);
    
    /* Perform batch reads with intervals */
    for (int i = 0; i < READ_ITERATIONS; i++) {
        retval = performBatchRead(client, i);
        if (retval != UA_STATUSCODE_GOOD) {
            printf("Failed to perform batch read: %s\n", UA_StatusCode_name(retval));
            break;
        }
        
        /* Wait between iterations (except for the last one) */
        if (i < READ_ITERATIONS - 1) {
            printf("Waiting %d seconds until next batch read...\n", READ_INTERVAL_SEC);
            sleep(READ_INTERVAL_SEC);
        }
    }
    
    /* Disconnect and cleanup */
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    
    printf("\nBatch read trigger client completed successfully!\n");
    printf("Check the server console to see the batching behavior.\n");
    
    return EXIT_SUCCESS;
}
