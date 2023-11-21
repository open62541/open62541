#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/util.h>
//#include "eventfilter_parser_examples.h"

#include "../../deps/eventfilter_parser/eventfilter_parser_grammar.c"
#include <common.h>
#include <signal.h>
#include <stdio.h>

/*
 * This Tutorial repeats the client_eventfilter.c tutorial,
 * however the filter are created based on the Query Language for Eventfilter
 */

static void check_eventfilter(UA_EventFilter *filter){
    UA_EventFilter empty_filter;
    UA_EventFilter_init(&empty_filter);
    if(memcmp(&empty_filter, filter, sizeof(UA_EventFilter)) == 0){
        printf("failed to parse the filter\n");
    }
    else{
        printf("parsing succeeded\n");
        /*UA_String out = UA_STRING_NULL;
        UA_print(filter, &UA_TYPES[UA_TYPES_EVENTFILTER], &out);
        printf("%.*s\n", (int)out.length, out.data);
        UA_String_clear(&out);*/
    }
}

/*static void clear_event_filter(UA_EventFilter *filter){
    for(size_t i=0; i< filter->selectClausesSize; i++){
        for(size_t j=0; j< filter->selectClauses[i].browsePathSize; j++){
            UA_QualifiedName_clear(&filter->selectClauses[i].browsePath[j]);
        }
        UA_NodeId_clear(&filter->selectClauses[i].typeDefinitionId);
        UA_String_clear(&filter->selectClauses[i].indexRange);
        UA_SimpleAttributeOperand_clear(&filter->selectClauses[i]);
    }
    for(size_t i=0; i<filter->whereClause.elementsSize; i++){
        for(size_t j=0; j< filter->whereClause.elements[i].filterOperandsSize; j++){
            UA_ExtensionObject_clear(&filter->whereClause.elements[i].filterOperands[j]);
        }
        UA_ContentFilterElement_clear(&filter->whereClause.elements[i]);
    }
    UA_ContentFilter_clear(&filter->whereClause);
    UA_EventFilter_clear(filter);
}*/

static UA_Boolean running = true;

static UA_StatusCode
read_queries(UA_UInt16 filterSelection, UA_EventFilter *filter){
    switch(filterSelection){
        case 0 : {
            /*the executable is located in build/bin/examples/... */
            char *path_to_query = "../../../examples/events/example_queries/case_0.txt";
            UA_ByteString content = loadFile(path_to_query);
            //UA_ByteString case_0 = UA_String_fromChars(CASE_0);
            UA_EventFilter_parse(&content, filter);
            check_eventfilter(filter);
            UA_ByteString_clear(&content);
            break;
        }
        case 1 : {
            /*the executable is located in build/bin/examples/... */
            char *path_to_query = "../../../examples/events/example_queries/case_1.txt";
            UA_ByteString content = loadFile(path_to_query);
            UA_EventFilter_parse(&content, filter);
            check_eventfilter(filter);
            UA_ByteString_clear(&content);
            break;
        }
        case 2 : {
            /*the executable is located in build/bin/examples/... */
            char *path_to_query = "../../../examples/events/example_queries/case_2.txt";
            UA_ByteString content = loadFile(path_to_query);
            UA_EventFilter_parse(&content, filter);
            check_eventfilter(filter);
            UA_ByteString_clear(&content);
            break;
        }
        case 3 : {
            /*the executable is located in build/bin/examples/... */
            char *path_to_query = "../../../examples/events/example_queries/case_3.txt";
            UA_ByteString content = loadFile(path_to_query);
            UA_EventFilter_parse(&content, filter);
            check_eventfilter(filter);
            UA_ByteString_clear(&content);
            break;
        }
        case 4 : {
            /*the executable is located in build/bin/examples/... */
            char *path_to_query = "../../../examples/events/example_queries/case_4.txt";
            UA_ByteString content = loadFile(path_to_query);
            UA_EventFilter_parse(&content, filter);
            check_eventfilter(filter);
            UA_ByteString_clear(&content);
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
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Received Event Notification (Filter passed)");
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
    /*use the utility function UA_Client_EventFilter_createSubscription*/
    char *path_to_query = "../../../examples/events/example_queries/case_4.txt";
    UA_ByteString content = loadFile(path_to_query);
    UA_EventFilter filt;
    UA_EventFilter_init(&filt);
    retval = UA_Client_EventFilter_createSubscription(client, handler_events_filter, &content, &filt);
    if(retval != UA_STATUSCODE_GOOD){
        return 0;
    }
    check_eventfilter(&filt);

    /*use utility function UA_EventFilter_parse*/
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);
    retval = read_queries(4, &filter);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return 0;
    }

    /* Create a subscription */
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                            NULL, NULL, NULL);
    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_Client_disconnect(client);
        UA_Client_delete(client);
        return 0;
    }

    UA_UInt32 subId = response.subscriptionId;
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Create subscription succeeded, id %u", subId);

    /* Add a MonitoredItem */
    UA_MonitoredItemCreateRequest item;
    UA_MonitoredItemCreateRequest_init(&item);
    item.itemToMonitor.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER); // Root->Objects->Server
    item.itemToMonitor.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
    item.monitoringMode = UA_MONITORINGMODE_REPORTING;


    item.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;
    item.requestedParameters.filter.content.decoded.data = &filter;
    item.requestedParameters.filter.content.decoded.type = &UA_TYPES[UA_TYPES_EVENTFILTER];

    UA_UInt32 monId = 0;
    UA_MonitoredItemCreateResult result =
        UA_Client_MonitoredItems_createEvent(client, subId,
                                             UA_TIMESTAMPSTORETURN_BOTH, item,
                                             &monId, handler_events_filter, NULL);

    if(result.statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Could not add the MonitoredItem with %s",
                    UA_StatusCode_name(result.statusCode));
        goto cleanup;
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Monitoring 'Root->Objects->Server', id %u",
                    response.subscriptionId);
    }

    monId = result.monitoredItemId;


    while(running)
        UA_Client_run_iterate(client, 100);

    /* Delete the subscription */
cleanup:
    UA_MonitoredItemCreateResult_clear(&result);
    UA_Client_Subscriptions_deleteSingle(client, response.subscriptionId);
    clear_event_filter(&filter);
    clear_event_filter(&filt);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return 0;
}
