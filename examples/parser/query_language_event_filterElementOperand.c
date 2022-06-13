//
// Created by flo47663 on 10.10.2022.
//
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_subscriptions.h>
#include <open62541/util.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>


#include "parser/min_examples_query/event_filter_setup_functions.h"
#include "parser/min_examples_query/LEG_Single/EventFilter/event.leg.c"



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



static void create_eventfilter(char *path_to_file, UA_EventFilter *filter){
    freopen(path_to_file,"r",stdin);

    //printf("the value of test is %d\n", test);

    printf("start parsing\n");

    yycontext ctx;
    memset(&ctx, 0, sizeof(yycontext));

    while (yyparse(&ctx));


    UA_String out = UA_STRING_NULL;
    UA_print(&ctx.filt, &UA_TYPES[UA_TYPES_EVENTFILTER], &out);
    printf("The parsed event Filter inside the ctr variable: %.*s\n", (int)out.length, out.data);
    UA_String_clear(&out);


    //clear the event filter
    //clear the select clauses
    /*for(size_t i=0; i< ctx.filt.selectClausesSize; i++){
        for(size_t j=0; j< ctx.filt.selectClauses[i].browsePathSize; j++){
            UA_QualifiedName_clear(&ctx.filt.selectClauses[i].browsePath[j]);
        }
        UA_NodeId_clear(&ctx.filt.selectClauses[i].typeDefinitionId);
        UA_String_clear(&ctx.filt.selectClauses[i].indexRange);
        UA_SimpleAttributeOperand_clear(&ctx.filt.selectClauses[i]);
    }
    //clear the contetFilter
    for(size_t i=0; i<ctx.filt.whereClause.elementsSize; i++){
        for(size_t j=0; j< ctx.filt.whereClause.elements[i].filterOperandsSize; j++){
            UA_ExtensionObject_clear(&ctx.filt.whereClause.elements[i].filterOperands[j]);
        }
        UA_ContentFilterElement_clear(&ctx.filt.whereClause.elements[i]);
    }*/
    //UA_EventFilter_clear(&ctx.filt);
    yyrelease(&ctx);

}

static volatile UA_Boolean running = true;
static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

int main(int argc, char *argv[]) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);
    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    if (argc >= 2) {
        printf("\nNumber Of Arguments Passed: %d", argc);
        printf("\n----Following Are The Command Line "
               "Arguments Passed----");
        for (int i = 0; i < argc; i++)
            printf("\nargv[%d]: %s\n", i, argv[i]);
    }
    //program arguments opc.tcp://127.0.1.1:4840 /mnt/c/Users/flo47663/Desktop/OPCUA/open62541_Aufgaben/EventFilter/open62541-QueryLanguage/examples/parser/min_examples_query/LEG_Single/EventFilter/examples/example_1.txt
    //create_eventfilter(argv[2]);
    char *path = "/mnt/c/Users/flo47663/Desktop/OPCUA/open62541_Aufgaben/EventFilter/open62541-QueryLanguage/examples/parser/min_examples_query/LEG_Single/EventFilter/examples/example_2.txt";
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);
    create_eventfilter(path, &filter);

    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    UA_StatusCode retval = UA_Client_connect(client, argv[1]);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }

    /* Create a subscription */
    /*UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                            NULL, NULL, NULL);
    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_Client_disconnect(client);
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }
    UA_UInt32 subId = response.subscriptionId;
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Create subscription succeeded, id %u", subId);*/

    /* Add a MonitoredItem */
    /*UA_MonitoredItemCreateRequest item;
    UA_MonitoredItemCreateRequest_init(&item);
    item.itemToMonitor.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER); // Root->Objects->Server
    item.itemToMonitor.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
    item.monitoringMode = UA_MONITORINGMODE_REPORTING;


    item.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;
    item.requestedParameters.filter.content.decoded.data = &parsed_filter;
    item.requestedParameters.filter.content.decoded.type = &UA_TYPES[UA_TYPES_EVENTFILTER];

    UA_UInt32 monId = 0;
    UA_MonitoredItemCreateResult result =
        UA_Client_MonitoredItems_createEvent(client, subId,
                                             UA_TIMESTAMPSTORETURN_BOTH, item,
                                             &monId, handler_events_filter, NULL);

    if(result.statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Could not add the MonitoredItem with %s", UA_StatusCode_name(result.statusCode));
        goto cleanup;
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Monitoring 'Root->Objects->Server', id %u", response.subscriptionId);
    }

    monId = result.monitoredItemId;*/

    while(running)
        UA_Client_run_iterate(client, 100);

    /* Delete the subscription */
/*cleanup:
    UA_MonitoredItemCreateResult_clear(&result);
    UA_Client_Subscriptions_deleteSingle(client, response.subscriptionId);
    //UA_Array_delete(filter.selectClauses, SELECT_CLAUSE_FIELD_COUNT, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
    //UA_Array_delete(filter.whereClause.elements, filter.whereClause.elementsSize, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENT]);*/

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}