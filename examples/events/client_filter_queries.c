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
#include <open62541/eventfilter_parser_examples.h>

#include <signal.h>
#include <stdio.h>

/*
 * This Tutorial repeats the client_eventfilter.c tutorial,
 * however the filter are created based on the Query Language for Eventfilter
 */
static void
check_eventfilter(UA_EventFilter *filter, UA_Boolean print_filter){
    UA_EventFilter empty_filter;
    UA_EventFilter_init(&empty_filter);
    if(memcmp(&empty_filter, filter, sizeof(UA_EventFilter)) == 0){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Failed to parse the EventFilter");
    }
    else{
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "EventFilter parsing succeeded");
        if(print_filter){
            UA_String out = UA_STRING_NULL;
            UA_print(filter, &UA_TYPES[UA_TYPES_EVENTFILTER], &out);
            printf("%.*s\n", (int)out.length, out.data);
            UA_String_clear(&out);
        }
    }
}

static UA_Boolean running = true;

static UA_StatusCode
read_queries(UA_UInt16 filterSelection, UA_EventFilter *filter){
    switch(filterSelection){
        case 0 : {
            char *inp = "SELECT /Message, /0:Severity /EventType "
                        "WHERE OR($ref_1, $ref_2) "
                        "FOR "
                        "$ref2 := OFTYPE(ns=1;i=5003) AND "
                        "$ref1 := (OFTYPE i=3035)";

            UA_ByteString case_0 = UA_String_fromChars(inp);
            UA_EventFilter_parse(filter, case_0, NULL);
            check_eventfilter(filter, UA_FALSE);
            UA_ByteString_clear(&case_0);
            break;
        }
        case 1 : {
            /*query can be found in the file example_queries/case_1 */
            UA_ByteString case_1 = UA_String_fromChars(CASE_1);
            UA_EventFilter_parse(filter, case_1, NULL);
            check_eventfilter(filter, UA_FALSE);
            UA_ByteString_clear(&case_1);
            break;
        }
        case 2 : {
            /*query can be found in the file example_queries/case_2 */
            UA_ByteString case_2 = UA_String_fromChars(CASE_2);
            UA_EventFilter_parse(filter, case_2, NULL);
            check_eventfilter(filter, UA_FALSE);
            UA_ByteString_clear(&case_2);
            break;
        }
        case 3 : {
            char *inp = "SELECT /Message, /Severity, /EventType "
                         "WHERE AND(OFTYPE(ns=1;i=5001), $abc) "
                         "FOR "
                         "$abc := AND($xyz, $uvw) AND "
                         "$xyz := ==(INT64 99, 99) AND "
                         "$uvw := GREATER(i=5000/Severity, 99)";

            UA_ByteString case_3 = UA_String_fromChars(inp);
            UA_EventFilter_parse(filter, case_3, NULL);
            check_eventfilter(filter, UA_FALSE);
            UA_ByteString_clear(&case_3);
            break;
        }
        case 4 : {
            char *inp = "SELECT /Message, /0:Severity, /EventType "
                        "WHERE "
                        "AND($a, GREATER(i=5000/Severity, $ref)) "
                        "FOR "
                        "$ref := 99 AND "
                        "$a := OFTYPE(ns=1;i=5000)";
            UA_ByteString case_4 = UA_String_fromChars(inp);
            UA_EventFilter_parse(filter, case_4, NULL);
            check_eventfilter(filter, UA_FALSE);
            UA_ByteString_clear(&case_4);
            break;
        }
        default:
            UA_EventFilter_clear(filter);
            return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }
    return UA_STATUSCODE_GOOD;
}

static void
handler_events_filter(UA_Client *client, UA_UInt32 subId, void *subContext,
                      UA_UInt32 monId, void *monContext,
                      size_t nEventFields, UA_Variant *eventFields) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Received Event Notification (Filter passed)");
    for(size_t i = 0; i < nEventFields; ++i) {
        if(UA_Variant_hasScalarType(&eventFields[i], &UA_TYPES[UA_TYPES_UINT16])) {
            UA_UInt16 severity = *(UA_UInt16 *)eventFields[i].data;
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Severity: %u", severity);
        } else if (UA_Variant_hasScalarType(&eventFields[i], &UA_TYPES[UA_TYPES_LOCALIZEDTEXT])) {
            UA_LocalizedText *lt = (UA_LocalizedText *)eventFields[i].data;
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Message: '%.*s'", (int)lt->text.length, lt->text.data);
        } else if (UA_Variant_hasScalarType(&eventFields[i], &UA_TYPES[UA_TYPES_NODEID])) {
            UA_String nodeIdName = UA_STRING_ALLOC("");
            UA_NodeId_print((UA_NodeId *)eventFields[i].data, &nodeIdName);
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "TypeNodeId: '%.*s'", (int)nodeIdName.length, nodeIdName.data);
            UA_String_clear(&nodeIdName);
        } else {
#ifdef UA_ENABLE_TYPEDESCRIPTION
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Don't know how to handle type: '%s'", eventFields[i].type->typeName);
#else
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Don't know how to handle type, enable UA_ENABLE_TYPEDESCRIPTION "
                        "for typename");
#endif
        }
    }
}


static UA_StatusCode
create_event_filter_with_monitored_item(UA_UInt16 filterSelection, UA_Client *client,
                                        UA_EventFilter *filter,
                                        UA_CreateSubscriptionResponse *response,
                                        UA_MonitoredItemCreateResult *result){
    /* read the eventfilter query string and create the corresponding eventfilter */
    UA_StatusCode retval = read_queries(filterSelection, filter);
    if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                         "Failed to parse the filter query with statuscode %s \n",
                         UA_StatusCode_name(retval));
            return retval;
    }
    /* Create a subscription */
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    *response = UA_Client_Subscriptions_create(client, request, NULL, NULL, NULL);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        return response->responseHeader.serviceResult;

    UA_UInt32 subId = response->subscriptionId;
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Create subscription succeeded, id %u", subId);

    /* Add a MonitoredItem */
    UA_MonitoredItemCreateRequest item;
    UA_MonitoredItemCreateRequest_init(&item);
    item.itemToMonitor.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER); // Root->Objects->Server
    item.itemToMonitor.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
    item.monitoringMode = UA_MONITORINGMODE_REPORTING;
    item.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;
    item.requestedParameters.filter.content.decoded.data = filter;
    item.requestedParameters.filter.content.decoded.type = &UA_TYPES[UA_TYPES_EVENTFILTER];

    UA_UInt32 monId = 0;
    *result = UA_Client_MonitoredItems_createEvent(client, subId,
                                                     UA_TIMESTAMPSTORETURN_BOTH,
                                                     item,
                                                     &monId, handler_events_filter, NULL);

    if(result->statusCode != UA_STATUSCODE_GOOD) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                            "Could not add the MonitoredItem with %s",
                            UA_StatusCode_name(result->statusCode));
            //UA_MonitoredItemCreateResult_clear(&result);
            return UA_STATUSCODE_BAD;
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                            "Monitoring 'Root->Objects->Server', id %u",
                            response->subscriptionId);
    monId = result->monitoredItemId;
    return UA_STATUSCODE_GOOD;
}

static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

int main(int argc, char *argv[]) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    if(argc < 2) {
        printf("Usage: tutorial_client_event_filter <opc.tcp://server-url>\n");
        return 0;
    }
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_StatusCode retval = UA_Client_connect(client, argv[1]);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Could not connect");
        UA_Client_delete(client);
        return 0;
    }

    UA_EventFilter filter;
    UA_EventFilter_init(&filter);
    UA_CreateSubscriptionResponse *response = UA_CreateSubscriptionResponse_new();
    UA_MonitoredItemCreateResult *result = UA_MonitoredItemCreateResult_new();
    retval = create_event_filter_with_monitored_item(3, client, &filter, response, result);
    if(retval == UA_STATUSCODE_GOOD){
        while(running)
            UA_Client_run_iterate(client, true);
    }
    /* Delete the subscription */
    UA_Client_Subscriptions_deleteSingle(client, response->subscriptionId);
    UA_MonitoredItemCreateResult_delete(result);
    UA_CreateSubscriptionResponse_delete(response);
    UA_EventFilter_clear(&filter);
    UA_Client_disconnect(client);
    UA_Client_delete(client);

    return 0;
}
