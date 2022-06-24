/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel_async.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server_config_default.h>
#include <open62541/client_subscriptions.h>

#include <stdlib.h>
#include <signal.h>

UA_Boolean running = true;
static void InitCallMulti(UA_Client* client);

#ifdef UA_ENABLE_METHODCALLS

static void
methodCalled(UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_CallResponse *response) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "**** CallRequest Response - Req:%u with %u results",
                requestId, (UA_UInt32)response->resultsSize);
    UA_StatusCode retval = response->responseHeader.serviceResult;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "**** CallRequest Response - Req:%u FAILED", requestId);
        return;
    }

    for(size_t i = 0; i < response->resultsSize; i++) {
        retval = response->results[i].statusCode;
        if(retval != UA_STATUSCODE_GOOD) {
            UA_CallResponse_clear(response);
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "**** CallRequest Response - Req: %u (%lu) failed",
                        requestId, (unsigned long)i);
            continue;
        }

        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "---Method call was successful, returned %lu values.\n",
                    (unsigned long)response->results[i].outputArgumentsSize);
    }

    /* We initiate the MultiCall (2 methods within one CallRequest) */
    InitCallMulti(client);
}

#ifdef UA_ENABLE_METHODCALLS
/* Workaround because we do not have an API for that yet */

static UA_StatusCode
UA_Client_call_asyncMulti(UA_Client *client,
    const UA_NodeId objectId1, const UA_NodeId methodId1, size_t inputSize1, const UA_Variant *input1,
    const UA_NodeId objectId2, const UA_NodeId methodId2, size_t inputSize2, const UA_Variant *input2,
    UA_ClientAsyncServiceCallback callback, void *userdata, UA_UInt32 *reqId) {
    UA_CallRequest request;
    UA_CallRequest_init(&request);
    UA_CallMethodRequest item[2];
    UA_CallMethodRequest_init(&item[0]);
    item[0].methodId = methodId1;
    item[0].objectId = objectId1;
    item[0].inputArguments = (UA_Variant *)(void*)(uintptr_t)input1; // cast const...
    item[0].inputArgumentsSize = inputSize1;

    UA_CallMethodRequest_init(&item[1]);
    item[1].methodId = methodId2;
    item[1].objectId = objectId2;
    item[1].inputArguments = (UA_Variant *)(void*)(uintptr_t)input2; // cast const...
    item[1].inputArgumentsSize = inputSize2;

    request.methodsToCall = &item[0];
    request.methodsToCallSize = 2;

    return __UA_Client_AsyncService(client, &request,
        &UA_TYPES[UA_TYPES_CALLREQUEST], callback,
        &UA_TYPES[UA_TYPES_CALLRESPONSE], userdata, reqId);
}
/* End Workaround */

static void InitCallMulti(UA_Client* client) {
    UA_UInt32 reqId = 0;
    UA_Variant input;
    UA_Variant_init(&input);
    UA_String stringValue = UA_String_fromChars("World 3 (multi)");
    UA_Variant_setScalar(&input, &stringValue, &UA_TYPES[UA_TYPES_STRING]);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "**** Initiating CallRequest 3");
    UA_Client_call_asyncMulti(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(1, 62542), 1, &input,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(1, 62541), 1, &input,
        (UA_ClientAsyncServiceCallback)methodCalled, NULL, &reqId);
    UA_String_clear(&stringValue);
}
#endif
#endif /* UA_ENABLE_METHODCALLS */

static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Received Ctrl-C");
    running = 0;
}

#ifdef UA_ENABLE_SUBSCRIPTIONS
static void
handler_currentTimeChanged(UA_Client *client, UA_UInt32 subId, void *subContext,
    UA_UInt32 monId, void *monContext, UA_DataValue *value) {
    if(UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_DATETIME])) {
        UA_DateTime raw_date = *(UA_DateTime *)value->value.data;
        UA_DateTimeStruct dts = UA_DateTime_toStruct(raw_date);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "date is: %02u-%02u-%04u %02u:%02u:%02u.%03u",
                    dts.day, dts.month, dts.year, dts.hour, dts.min, dts.sec, dts.milliSec);
    }
}

static void
deleteSubscriptionCallback(UA_Client *client, UA_UInt32 subscriptionId, void *subscriptionContext) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Subscription Id %u was deleted", subscriptionId);
}

static void
subscriptionInactivityCallback(UA_Client *client, UA_UInt32 subId, void *subContext) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Inactivity for subscription %u", subId);
}
#endif

static void
stateCallback(UA_Client *client, UA_SecureChannelState channelState,
              UA_SessionState sessionState, UA_StatusCode connectStatus) {
    if(sessionState == UA_SESSIONSTATE_ACTIVATED) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "A session with the server is activated");

#ifdef UA_ENABLE_SUBSCRIPTIONS
        /* A new session was created. We need to create the subscription. */
        /* Create a subscription */
        UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
        UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
            NULL, NULL, deleteSubscriptionCallback);

        if(response.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Create subscription succeeded, id %u", response.subscriptionId);
        else
            return;

        /* Add a MonitoredItem */
        UA_NodeId target =
            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
        UA_MonitoredItemCreateRequest monRequest =
            UA_MonitoredItemCreateRequest_default(target);

        UA_MonitoredItemCreateResult monResponse =
            UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
                                                      UA_TIMESTAMPSTORETURN_BOTH,
                monRequest, NULL, handler_currentTimeChanged, NULL);
        if(monResponse.statusCode == UA_STATUSCODE_GOOD)
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Monitoring UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME', id %u",
                        monResponse.monitoredItemId);
#endif

#ifdef UA_ENABLE_METHODCALLS		
        UA_UInt32 reqId = 0;
        UA_Variant input;
        UA_Variant_init(&input);
        UA_String stringValue = UA_String_fromChars("World 1");
        UA_Variant_setScalar(&input, &stringValue, &UA_TYPES[UA_TYPES_STRING]);

        /* Initiate Call 1 */
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "**** Initiating CallRequest 1");
        UA_Client_call_async(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                             UA_NODEID_NUMERIC(1, 62541), 1, &input,
                             methodCalled, NULL, &reqId);
        UA_String_clear(&stringValue);

        /* Initiate Call 2 */
        UA_Variant_init(&input);
        stringValue = UA_String_fromChars("World 2");
        UA_Variant_setScalar(&input, &stringValue, &UA_TYPES[UA_TYPES_STRING]);

        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "**** Initiating CallRequest 2");
        UA_Client_call_async(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                             UA_NODEID_NUMERIC(1, 62542), 1, &input,
                             methodCalled, NULL, &reqId);
        UA_String_clear(&stringValue);

#endif /* UA_ENABLE_METHODCALLS */
    }

    if(sessionState == UA_SESSIONSTATE_CLOSED)
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Session disconnected");
}

int
main(int argc, char *argv[]) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */

    UA_Client *client = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(cc);
    /* we use a high timeout because there may be other client and
    * processing may take long if many method calls are waiting */
    cc->timeout = 60000;

    /* Set stateCallback */
    cc->stateCallback = stateCallback;
#ifdef UA_ENABLE_SUBSCRIPTIONS
    cc->subscriptionInactivityCallback = subscriptionInactivityCallback;
#endif

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Not connected. Retrying to connect in 1 second");
        UA_Client_delete(client);
        return EXIT_SUCCESS;
    }

    /* Endless loop runAsync */
    while(running) {
        UA_Client_run_iterate(client, 100);
    }

    /* Clean up */
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return EXIT_SUCCESS;
}
