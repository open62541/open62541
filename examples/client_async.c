/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel_async.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server_config_default.h>

#include <stdlib.h>
#include "common.h"

#define NODES_EXIST
/* async connection callback, it only gets called after the completion of the whole
 * connection process*/
static void
onConnect(UA_Client *client, UA_SecureChannelState channelState,
          UA_SessionState sessionState, UA_StatusCode connectStatus) {
    printf("Async connect returned with status code %s\n",
           UA_StatusCode_name(connectStatus));
}

static
void
fileBrowsed(UA_Client *client, void *userdata, UA_UInt32 requestId,
            UA_BrowseResponse *response) {
    printf("%-50s%u\n", "Received BrowseResponse for request ", requestId);
    UA_String us = *(UA_String *) userdata;
    printf("---%.*s passed safely \n", (int) us.length, us.data);
}

/*high-level function callbacks*/
static
void
readValueAttributeCallback(UA_Client *client, void *userdata,
                           UA_UInt32 requestId, UA_StatusCode status,
                           UA_DataValue *var) {
    printf("%-50s%u\n", "Read value attribute for request", requestId);
    if(UA_Variant_hasScalarType(&var->value, &UA_TYPES[UA_TYPES_INT32])) {
        UA_Int32 int_val = *(UA_Int32*) var->value.data;
        printf("---%-40s%-8i\n",
               "Reading the value of node (1, \"the.answer\"):", int_val);
    }
}

static
void
attrWritten(UA_Client *client, void *userdata, UA_UInt32 requestId,
            UA_WriteResponse *response) {
    /*assuming no data to be retrieved by writing attributes*/
    printf("%-50s%u\n", "Wrote value attribute for request ", requestId);
    UA_WriteResponse_clear(response);
}

#ifdef NODES_EXIST
#ifdef UA_ENABLE_METHODCALLS
static void
methodCalled(UA_Client *client, void *userdata, UA_UInt32 requestId,
             UA_CallResponse *response) {

    printf("%-50s%u\n", "Called method for request ", requestId);
    size_t outputSize;
    UA_Variant *output;
    UA_StatusCode retval = response->responseHeader.serviceResult;
    if(retval == UA_STATUSCODE_GOOD) {
        if(response->resultsSize == 1)
            retval = response->results[0].statusCode;
        else
            retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    if(retval != UA_STATUSCODE_GOOD) {
        UA_CallResponse_clear(response);
        printf("---Method call was unsuccessful, returned %x values.\n", retval);
    } else {
        /* Move the output arguments */
        output = response->results[0].outputArguments;
        outputSize = response->results[0].outputArgumentsSize;
        response->results[0].outputArguments = NULL;
        response->results[0].outputArgumentsSize = 0;
        printf("---Method call was successful, returned %lu values.\n",
               (unsigned long)outputSize);
        UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
    }

    UA_CallResponse_clear(response);
}

#endif
#endif

int
main(int argc, char *argv[]) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(cc);
    UA_UInt32 reqId = 0;
    UA_String userdata = UA_STRING("userdata");

    UA_BrowseRequest bReq;
    UA_BrowseRequest_init(&bReq);
    bReq.requestedMaxReferencesPerNode = 0;
    bReq.nodesToBrowse = UA_BrowseDescription_new();
    bReq.nodesToBrowseSize = 1;
    bReq.nodesToBrowse[0].nodeId = UA_NS0ID(OBJECTSFOLDER);
    bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL; /* return everything */

    cc->stateCallback = onConnect;
    UA_Client_connectAsync(client, "opc.tcp://localhost:4840");

    /*Windows needs time to response*/
    sleep_ms(100);

    /* What happens if client tries to send request before connected? */
    UA_Client_sendAsyncBrowseRequest(client, &bReq, fileBrowsed, &userdata, &reqId);

    UA_DateTime startTime = UA_DateTime_nowMonotonic();
    do {
        UA_SessionState ss;
        UA_Client_getState(client, NULL, &ss, NULL);
        if(ss == UA_SESSIONSTATE_ACTIVATED) {
            /* If not connected requests are not sent */
            UA_Client_sendAsyncBrowseRequest(client, &bReq, fileBrowsed, &userdata, &reqId);
        }
        /* Requests are processed */
        UA_BrowseRequest_clear(&bReq);
        UA_Client_run_iterate(client, 0);
        sleep_ms(100);

        /* Break loop if server cannot be connected within 2s -- prevents build timeout */
        if(UA_DateTime_nowMonotonic() - startTime > 2000 * UA_DATETIME_MSEC)
            break;
    } while(reqId < 10);

    /* Demo: high-level functions */
    UA_Int32 value = 0;
    UA_Variant myVariant;
    UA_Variant_init(&myVariant);

    UA_Variant input;
    UA_Variant_init(&input);

    for(UA_UInt16 i = 0; i < 5; i++) {
        UA_SessionState ss;
        UA_Client_getState(client, NULL, &ss, NULL);
        if(ss == UA_SESSIONSTATE_ACTIVATED) {
            /* writing and reading value 1 to 5 */
            UA_Variant_setScalarCopy(&myVariant, &value, &UA_TYPES[UA_TYPES_INT32]);
            value++;
            UA_Client_writeValueAttribute_async(client,
                                                UA_NODEID_STRING(1, "the.answer"),
                                                &myVariant, attrWritten, NULL,
                                                &reqId);
            UA_Variant_clear(&myVariant);

            UA_Client_readValueAttribute_async(client,
                                               UA_NODEID_STRING(1, "the.answer"),
                                               readValueAttributeCallback, NULL,
                                               &reqId);

//TODO: check the existance of the nodes inside these functions (otherwise seg faults)
#ifdef NODES_EXIST
#ifdef UA_ENABLE_METHODCALLS
            UA_String stringValue = UA_String_fromChars("World");
            UA_Variant_setScalar(&input, &stringValue, &UA_TYPES[UA_TYPES_STRING]);

            UA_Client_call_async(client, UA_NS0ID(OBJECTSFOLDER),
                                 UA_NODEID_NUMERIC(1, 62541), 1, &input,
                                 methodCalled, NULL, &reqId);
            UA_String_clear(&stringValue);
#endif /* UA_ENABLE_METHODCALLS */
#endif
            /* How often UA_Client_run_iterate is called depends on the number of request sent */
            UA_Client_run_iterate(client, 0);
            UA_Client_run_iterate(client, 0);
        }
    }
    UA_Client_run_iterate(client, 0);

    /* Async disconnect kills unprocessed requests */
    // UA_Client_disconnect_async (client, &reqId); //can only be used when connected = true
    // UA_Client_run_iterate (client, &timedOut);
    UA_Client_disconnect(client);
    UA_Client_delete(client);

    return EXIT_SUCCESS;
}
