/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Faking source timestamp
 * ----------------------------
 *
 * This example shows how to write data with a modified source timestamp
 * When showing the data in UAExpert, the source timestamp will always be half an hour behind the server timestamp
 */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>

static void
addVariable(UA_Server *server) {
    /* Define the attribute of the myInteger variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 0;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US","fake source_timestamp");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","fake source_timestamp");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    /* Add the variable node to the information model */
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "fake.source_timestamp");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "fake source_timestamp");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                              parentReferenceNodeId, myIntegerName,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);
}


/**
 * Now we change the value with the write service. This uses the same service
 * implementation that can also be reached over the network by an OPC UA client.
 */

static void
writeVariable(UA_Server *server, UA_Int32 myInteger ) {
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "fake.source_timestamp");

    /* Write a different integer value */
    UA_VariableAttributes Attr;
    UA_Variant_init(&Attr.value);
    UA_Variant_setScalar(&Attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);

    // Use a more detailed write function than UA_Server_writeValue
    UA_WriteValue wv;
    UA_WriteValue_init(&wv);
    wv.nodeId = myIntegerNodeId;
    wv.attributeId = UA_ATTRIBUTEID_VALUE;
    wv.value.hasStatus = false;
    wv.value.value = Attr.value;
    wv.value.hasValue = true;

    UA_DateTime currentTime = UA_DateTime_now();
    wv.value.hasSourceTimestamp = true;
    wv.value.sourceTimestamp = currentTime - 1800 * UA_DATETIME_SEC;

    UA_Server_writeDataValue(server, myIntegerNodeId, wv.value );

}

/** It follows the main server code, making use of the above definitions. */

static volatile UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    addVariable(server);
    writeVariable(server,42);

    UA_StatusCode retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
