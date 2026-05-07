/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/util.h>

#include <stdio.h>

static void
handler_events(UA_Client *client, UA_UInt32 subId, void *subContext,
               UA_UInt32 monId, void *monContext,
               const UA_KeyValueMap eventFields) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "Notification");

    for(size_t i = 0; i < eventFields.mapSize; ++i) {
        UA_String out = UA_STRING_NULL;
        UA_print(&eventFields.map[i].value, &UA_TYPES[UA_TYPES_VARIANT], &out);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                    "%S: '%S", eventFields.map[i].key.name, out);
        UA_String_clear(&out);
    }
}

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("Usage: tutorial_client_events <opc.tcp://server-url>\n");
        return 0;
    }

    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    /* opc.tcp://uademo.prosysopc.com:53530/OPCUA/SimulationServer */
    /* opc.tcp://opcua.demo-this.com:51210/UA/SampleServer */
    UA_StatusCode retval = UA_Client_connect(client, argv[1]);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "Could not connect");
        UA_Client_delete(client);
        return 0;
    }

    /* Create a subscription */
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response =
        UA_Client_Subscriptions_create(client, request, NULL, NULL, NULL);
    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_Client_disconnect(client);
        UA_Client_delete(client);
        return 0;
    }
    UA_UInt32 subId = response.subscriptionId;
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                "Create subscription succeeded, id %u", subId);

    /* Add a MonitoredItem */
    UA_MonitoredItemCreateRequest item;
    UA_MonitoredItemCreateRequest_init(&item);
    item.itemToMonitor.nodeId = UA_NS0ID(SERVER);
    item.itemToMonitor.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
    item.monitoringMode = UA_MONITORINGMODE_REPORTING;

    /* Set the EventFilter. See the documentation for the human-readable
     * EventFilter expression developed for open62541. */
    char *eventFilter = "SELECT /Message, /Severity WHERE /Severity >= 100";
    UA_EventFilter filter;
    retval = UA_EventFilter_parse(&filter, UA_STRING(eventFilter), NULL);
    UA_assert(retval == UA_STATUSCODE_GOOD);
    UA_ExtensionObject_setValue(&item.requestedParameters.filter,
                                &filter, &UA_TYPES[UA_TYPES_EVENTFILTER]);

    UA_MonitoredItemCreateResult result =
        UA_Client_MonitoredItems_createEvent(client, subId,
                                             UA_TIMESTAMPSTORETURN_BOTH, item,
                                             NULL, handler_events, NULL);

    if(result.statusCode == UA_STATUSCODE_GOOD) {
        /* Run the client until ctrl-c */
        UA_Client_runUntilInterrupt(client);
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                    "Could not add the MonitoredItem with %s",
                    UA_StatusCode_name(result.statusCode));
    }

    /* Disconnect and clean up */
    UA_Client_Subscriptions_deleteSingle(client, response.subscriptionId);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    UA_MonitoredItemCreateResult_clear(&result);
    UA_EventFilter_clear(&filter);

    return 0;
}
