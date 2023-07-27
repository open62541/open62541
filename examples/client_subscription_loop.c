/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Client disconnect handling
 * --------------------------
 * This example shows you how to handle a client disconnect, e.g., if the server
 * is shut down while the client is connected. You just need to call connect
 * again and the client will automatically reconnect.
 *
 * This example is very similar to the tutorial_client_firststeps.c. */

#include <open62541/client_config_default.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>

#include <signal.h>
#include <stdlib.h>

UA_Boolean running = true;

static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Received Ctrl-C");
    running = false;
}

static void
handler_currentTimeChanged(UA_Client *client, UA_UInt32 subId, void *subContext,
                           UA_UInt32 monId, void *monContext, UA_DataValue *value) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "currentTime has changed!");
    if(UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_DATETIME])) {
        UA_DateTime raw_date = *(UA_DateTime *) value->value.data;
        UA_DateTimeStruct dts = UA_DateTime_toStruct(raw_date);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "date is: %02u-%02u-%04u %02u:%02u:%02u.%03u",
                    dts.day, dts.month, dts.year, dts.hour, dts.min, dts.sec, dts.milliSec);
    }
}

static void
deleteSubscriptionCallback(UA_Client *client, UA_UInt32 subscriptionId,
                           void *subscriptionContext) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Subscription Id %u was deleted", subscriptionId);
}

static void
subscriptionInactivityCallback(UA_Client *client, UA_UInt32 subId, void *subContext) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Inactivity for subscription %u", subId);
}

int main(void) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */

    UA_Client *client = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(cc);
    cc->subscriptionInactivityCallback = subscriptionInactivityCallback;

    /* Connect */
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Could not connect with error %s", UA_StatusCode_name(retval));
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }

    /* Create a Subscription */
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response =
        UA_Client_Subscriptions_create(client, request, NULL, NULL, deleteSubscriptionCallback);
    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Could not create a Subscription with error %s",
                     UA_StatusCode_name(response.responseHeader.serviceResult));
        UA_Client_delete(client); /* Disconnects the client internally */
        return EXIT_FAILURE;
    }

    /* Add a MonitoredItem */
    UA_NodeId currentTimeNode = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    UA_MonitoredItemCreateRequest monRequest =
        UA_MonitoredItemCreateRequest_default(currentTimeNode);
    UA_MonitoredItemCreateResult monResponse =
        UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
                                                  UA_TIMESTAMPSTORETURN_BOTH, monRequest,
                                                  NULL, handler_currentTimeChanged, NULL);
    if(monResponse.statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Could not create a MonitoredItem with error %s",
                     UA_StatusCode_name(response.responseHeader.serviceResult));
        UA_Client_delete(client); /* Disconnects the client internally */
        return EXIT_FAILURE;
    }

    /* Loop until interrupt */
    while(running) {
        UA_Client_run_iterate(client, 100);
    }

    /* Clean up */
    UA_Client_delete(client); /* Disconnects the client internally */
    return EXIT_SUCCESS;
}
