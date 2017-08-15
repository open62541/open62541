/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <stdio.h>
#include <signal.h>
#include "open62541.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS
static void
handler_events(UA_UInt32 monId, size_t nEventFields, UA_Variant *eventFields, void *context) {
    printf("got an event! %d, %zu\n", monId, nEventFields);

    for(size_t i = 0; i < nEventFields; ++i) {
        printf("%s\n", eventFields[i].type->typeName);
        if (UA_Variant_hasScalarType(&eventFields[i], &UA_TYPES[UA_TYPES_BYTESTRING])) {
            UA_ByteString *string = (UA_ByteString *)eventFields[i].data;
            printf("'%.*s'\n", (int)string->length, string->data);
            for (size_t u = 0; u < string->length; ++u)
                printf("%x", string->data[u]);
            printf("\n");
        } else if (UA_Variant_hasScalarType(&eventFields[i], &UA_TYPES[UA_TYPES_LOCALIZEDTEXT])) {
            UA_LocalizedText *lt = (UA_LocalizedText *)eventFields[i].data;
            printf("'%.*s'\n", (int)lt->text.length, lt->text.data);
            printf("size: %zu\n", lt->text.length);
        }
    }
}

static UA_Boolean running = true;
static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}
#endif

int main(int argc, char *argv[]) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Client *client = UA_Client_new(UA_ClientConfig_standard);

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://uademo.prosysopc.com:53530/OPCUA/SimulationServer");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return (int)retval;
    }

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* Create a subscription */
    UA_UInt32 subId = 0;
    retval = UA_Client_Subscriptions_new(client, UA_SubscriptionSettings_standard, &subId);
    if(!subId) {
        UA_Client_disconnect(client);
        UA_Client_delete(client);
        return (int)retval;
    }
    printf("Create subscription succeeded, id %u\n", subId);

    /* Add a MonitoredItem */
    UA_NodeId monitorThis = UA_NODEID_NUMERIC(0, 2253); // Root->Objects->Server
    UA_UInt32 monId = 0;


    // creating a single element array for the select clause of length 1
    const size_t nSelectClauses = 1;
    UA_SimpleAttributeOperand *selectClauses = (UA_SimpleAttributeOperand *)UA_Array_new(nSelectClauses, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
    if(!selectClauses){
        UA_Client_Subscriptions_remove(client, subId);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
        return (int)UA_STATUSCODE_BADOUTOFMEMORY;
    }

    for(size_t i =0; i<nSelectClauses; ++i) {
        UA_SimpleAttributeOperand_init(&selectClauses[i]);
    }

    selectClauses[0].typeDefinitionId = UA_NODEID_NUMERIC(0, 2041) ; // BaseEventType
    selectClauses[0].browsePathSize = 1;
    //selectClauses[0].browsePathSize = 2; // crashes
    selectClauses[0].browsePath = (UA_QualifiedName*)UA_Array_new(selectClauses[0].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[0].browsePath) {
        UA_Array_delete(selectClauses, nSelectClauses, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
        UA_Client_Subscriptions_remove(client, subId);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
        return (int)UA_STATUSCODE_BADOUTOFMEMORY;
    }
    selectClauses[0].attributeId = UA_ATTRIBUTEID_VALUE;
    selectClauses[0].browsePath[0] = UA_QUALIFIEDNAME(0, "Message");
    //selectClauses[0].browsePath[1] = UA_QUALIFIEDNAME(0, "Severity");  // crashes

    UA_Client_Subscriptions_addMonitoredEvent(client, subId, monitorThis, UA_ATTRIBUTEID_EVENTNOTIFIER,
                                              selectClauses, nSelectClauses,
                                              &handler_events, NULL, &monId);
    if (!monId) {
        UA_Array_delete(selectClauses, nSelectClauses, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
        UA_Client_Subscriptions_remove(client, subId);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
        return (int)retval;
    }
    printf("Monitoring 'Root->Objects->Server', id %u\n", subId);

    while (running)
        UA_Client_Subscriptions_manuallySendPublishRequest(client);

    /* Delete the subscription */
    if(!UA_Client_Subscriptions_remove(client, subId))
        printf("Subscription removed\n");
#endif

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return (int) UA_STATUSCODE_GOOD;
}
