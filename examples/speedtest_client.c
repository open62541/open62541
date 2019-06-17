/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Building a Simple Client
 * ------------------------
 * You should already have a basic server from the previous tutorials. open62541
 * provides both a server- and clientside API, so creating a client is as easy as
 * creating a server. Copy the following into a file `myClient.c`: */

#include <signal.h>
#include <open62541/types.h>
#include <open62541/plugin/log.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/client_config.h>
#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>


UA_Boolean running = true;

static void
stopHandler(int sign) {
    (void)sign;
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Received Ctrl-C");
    running = 0;
}

#define MAX_TEST_CLIENTS 10000

int main(void) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */
    signal(SIGTERM, stopHandler);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    UA_Client **clients = (UA_Client **)UA_calloc(MAX_TEST_CLIENTS, sizeof(UA_Client *));
    for(int i = 0; i < MAX_TEST_CLIENTS; ++i) {
        UA_Client *client = UA_Client_new();
        UA_ClientConfig_setDefault(UA_Client_getConfig(client));
        retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
        if(retval != UA_STATUSCODE_GOOD) {
            UA_Client_delete(client);
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "%s",UA_StatusCode_name(retval));
            goto cleanup;
        }

        clients[i] = client;
    }

    /* Read the value attribute of the node. UA_Client_readValueAttribute is a
     * wrapper for the raw read service available as UA_Client_Service_read. */
    UA_Variant value; /* Variants can hold scalar values and arrays of any type */
    UA_Variant_init(&value);

    /* NodeId of the variable holding the current time */
    //const UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    char *testString = (char *)UA_malloc(10);
    memset(testString, 'A', 10);
    testString[9] = 0;
    const UA_NodeId nodeId = UA_NODEID_BYTESTRING(0, testString);

    while(running) {
        for(int i = 0; i < MAX_TEST_CLIENTS; ++i) {
            UA_Client_readValueAttribute(clients[i], nodeId, &value);
            UA_Variant_clear(&value);
        }
    }

    UA_free(testString);

cleanup:
    for(int i = 0; i < MAX_TEST_CLIENTS; ++i) {
        if(clients[i] != NULL)
            UA_Client_delete(clients[i]); /* Disconnects the client internally */
    }
    return (int)retval;
}
