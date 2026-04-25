/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2023-2024 (c) Fraunhofer IOSB (Author: Florian Düwel)
 */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/types.h>

#include <stdio.h>

static void
handler_events_filter(UA_Client *client, UA_UInt32 subId, void *subContext,
                      UA_UInt32 monId, void *monContext,
                      const UA_KeyValueMap eventFields) {
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
        printf("Usage: tutorial_client_event_filter <opc.tcp://server-url>\n");
        return 0;
    }

    /* Parse the EventFilter */
    char *input = "SELECT /Message, /Severity, /EventType, /SourceName "
        "WHERE /Severity >= 100";
    UA_EventFilter filter;
    UA_StatusCode retval = UA_EventFilter_parse(&filter, UA_STRING(input), NULL);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Failed to parse the filter query with StatusCode %s \n",
                     UA_StatusCode_name(retval));
        return 0;
    }

    /* Create and connect the client */
    UA_Client *client = UA_Client_new();
    retval = UA_Client_connect(client, argv[1]);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "Could not connect");
        UA_EventFilter_clear(&filter);
        UA_Client_delete(client);
        return 0;
    }

    /* Create a Subscription */
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response =
        UA_Client_Subscriptions_create(client, request, NULL, NULL, NULL);
    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                    "Could not create a Subscription");
        UA_EventFilter_clear(&filter);
        UA_Client_delete(client);
        return 0;
    }

    UA_UInt32 subId = response.subscriptionId;
    UA_CreateSubscriptionResponse_clear(&response);

    /* Create a MonitoredItem */
    UA_MonitoredItemCreateRequest item;
    UA_MonitoredItemCreateRequest_init(&item);
    item.itemToMonitor.nodeId = UA_NS0ID(SERVER);
    item.itemToMonitor.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
    item.monitoringMode = UA_MONITORINGMODE_REPORTING;
    UA_ExtensionObject_setValue(&item.requestedParameters.filter, &filter,
                                &UA_TYPES[UA_TYPES_EVENTFILTER]);

    UA_MonitoredItemCreateResult result =
        UA_Client_MonitoredItems_createEvent(client, subId, UA_TIMESTAMPSTORETURN_BOTH,
                                             item, NULL, handler_events_filter, NULL);

    if(result.statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                    "Could not create the MonitoredItem");
        UA_MonitoredItemCreateResult_clear(&result);
        UA_EventFilter_clear(&filter);
        UA_Client_delete(client);
        return 0;
    }

    UA_MonitoredItemCreateResult_clear(&result);

    /* Run the client until ctrl-c */
    if(retval == UA_STATUSCODE_GOOD) {
        UA_Client_runUntilInterrupt(client);
    }

    /* Disconnect, clean up and delete the client */
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    UA_EventFilter_clear(&filter);

    return 0;
}
