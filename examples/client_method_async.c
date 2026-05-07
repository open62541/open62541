/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel_async.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server_config_default.h>

static void
methodCalled(UA_Client *client, void *userdata, UA_UInt32 requestId,
             UA_CallResponse *response) {
    UA_StatusCode retval = response->responseHeader.serviceResult;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "CallRequest FAILED");
        return;
    }

    for(size_t i = 0; i < response->resultsSize; i++) {
        retval = response->results[i].statusCode;
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                        "CallRequest Response - %u failed",
                        (unsigned long)i);
            continue;
        }

        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                    "Method call was successful, returned %lu values",
                    (unsigned long)response->results[i].outputArgumentsSize);
    }
}

int
main(int argc, char *argv[]) {
    UA_Client *client = UA_Client_new();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "Not connected");
        UA_Client_delete(client);
        return 0;
    }

    /* Initiate Call 1 */
    UA_Variant input;
    UA_String stringValue = UA_STRING("World 1");
    UA_Variant_setScalar(&input, &stringValue, &UA_TYPES[UA_TYPES_STRING]);
    UA_Client_call_async(client, UA_NS0ID(OBJECTSFOLDER),
                         UA_NODEID_NUMERIC(1, 62541), 1, &input,
                         methodCalled, NULL, NULL);

    /* Initiate Call 2 */
    stringValue = UA_STRING("World 2");
    UA_Variant_setScalar(&input, &stringValue, &UA_TYPES[UA_TYPES_STRING]);
    UA_Client_call_async(client, UA_NS0ID(OBJECTSFOLDER),
                         UA_NODEID_NUMERIC(1, 62542), 1, &input,
                         methodCalled, NULL, NULL);

    /* Run the client until ctrl-c */
    UA_Client_runUntilInterrupt(client);

    /* Clean up */
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return 0;
}
