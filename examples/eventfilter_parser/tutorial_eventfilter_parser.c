//
// Created by flo47663 on 10.10.2022.
//
#include <open62541/plugin/log_stdout.h>
#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/util.h>
#include <common.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include "eventfilter_parser/eventfilter_parser.h"

static void clear_event_filter(UA_EventFilter *filter){
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
}

static void check_eventfilter(UA_EventFilter *filter){
    UA_EventFilter empty_filter;
    UA_EventFilter_init(&empty_filter);
    if(memcmp(&empty_filter, filter, sizeof(UA_EventFilter)) == 0){
        printf("failed to parse the filter\n");
    }
    else{
        printf("parsing succeeded\n");
        UA_String out = UA_STRING_NULL;
        UA_print(filter, &UA_TYPES[UA_TYPES_EVENTFILTER], &out);
        printf("%.*s\n", (int)out.length, out.data);
        UA_String_clear(&out);
    }
}

static volatile UA_Boolean running = true;
static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

int main(int argc, char *argv[]) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);


    char *path = "../../../examples/eventfilter_parser/examples/";
    char *examples[13] = {
        "example_1.txt",
        "example_2.txt",
        "example_3.txt",
        "example_4.txt",
        "example_5.txt",
        "example_6.txt",
        "example_7.txt",
        "example_8.txt",
        "example_9.txt",
        "example_10.txt",
        "example_11.txt",
        "example_12.txt",
        "example_literal.txt"
    };
    for( size_t i=0; i< 13; i++){
        char example_file [512];
        strcpy(example_file, path);
        strcat(example_file, examples[i]);
        UA_ByteString content = loadFile(example_file);

        printf("example %s \n", example_file);

        UA_EventFilter filter;
        UA_StatusCode retval = UA_EventFilter_parse(&content, &filter);
        if(retval!= UA_STATUSCODE_GOOD){
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Parsing Query failed with StatusCode %s, Please check previous output", UA_StatusCode_name(retval));
        }
        else{
            check_eventfilter(&filter);
            clear_event_filter(&filter);
            UA_ByteString_clear(&content);
        }
    }

    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    UA_StatusCode retval = UA_Client_connect(client, argv[1]);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }
    while(running)
        UA_Client_run_iterate(client, 100);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
