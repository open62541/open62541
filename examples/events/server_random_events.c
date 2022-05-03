/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Generating sample events by method calls
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * The following example is an extension of the server event tutorial. The information
 * model contains two methods to generate all (in the example configured) sample
 * events or a random one.
 *
 * This example is intended to be used with the "client_eventfilter" example and allows
 * the easy generation of the filtered event types.
 */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>

//If more sample events are needed, "addSampleEventTypes" and "setUpEvent" must be extended
#define SAMPLE_EVENT_TYPES_COUNT 5

static volatile UA_Boolean running = true;
static UA_NodeId* eventTypes;

static UA_StatusCode
addEventType(UA_Server *server, char* name, UA_NodeId parentNodeId, UA_NodeId requestedId, UA_NodeId* eventType) {
    UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", name);
    attr.description = UA_LOCALIZEDTEXT("en-US", "Sample event type");
    UA_StatusCode retval = UA_Server_addObjectTypeNode(server, requestedId,
                                                       parentNodeId,
                                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                                       UA_QUALIFIEDNAME(0, name),
                                                       attr, NULL, eventType);
    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                       "Add EventType failed. StatusCode %s", UA_StatusCode_name(retval));
    }
    return retval;
}

static UA_StatusCode
addSampleEventTypes(UA_Server *server) {
    eventTypes = (UA_NodeId *)
        UA_Array_new(SAMPLE_EVENT_TYPES_COUNT, &UA_TYPES[UA_TYPES_NODEID]);
    UA_StatusCode retval = addEventType(server, "SampleBaseEventType",
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE),
                                        UA_NODEID_NUMERIC(1, 5000),
                                        &eventTypes[0]);
    if (retval != UA_STATUSCODE_GOOD) return retval;
    retval = addEventType(server, "SampleDeviceFailureEventType",
                          UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE),
                          UA_NODEID_NUMERIC(1, 5001),
                          &eventTypes[1]);
    if (retval != UA_STATUSCODE_GOOD) return retval;
    retval = addEventType(server, "SampleEventQueueOverflowEventType",
                          UA_NODEID_NUMERIC(0, UA_NS0ID_EVENTQUEUEOVERFLOWEVENTTYPE),
                          UA_NODEID_NUMERIC(1, 5002),
                          &eventTypes[2]);
    if (retval != UA_STATUSCODE_GOOD) return retval;
    retval = addEventType(server, "SampleProgressEventType",
                          UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE),
                          UA_NODEID_NUMERIC(1, 5003),
                          &eventTypes[3]);
    if (retval != UA_STATUSCODE_GOOD) return retval;
    retval = addEventType(server, "SampleAuditSecurityEventType",
                          UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE),
                          UA_NODEID_NUMERIC(1, 5004),
                          &eventTypes[4]);
    if (retval != UA_STATUSCODE_GOOD) return retval;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setUpEvent(UA_Server *server, UA_NodeId *outId, UA_NodeId eventType,
           UA_Boolean random ,size_t index) {
    UA_StatusCode retval = UA_Server_createEvent(server, eventType, outId);
    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                       "CreateEvent failed. StatusCode %s", UA_StatusCode_name(retval));
        return retval;
    }
    /* Set the Event Attributes */
    UA_DateTime eventTime = UA_DateTime_now();
    UA_Server_writeObjectProperty_scalar(server, *outId,
                                         UA_QUALIFIEDNAME(0, "Time"),
                                         &eventTime, &UA_TYPES[UA_TYPES_DATETIME]);
    UA_UInt16 eventSeverity;
    if(random)
        eventSeverity = (UA_UInt16) UA_UInt32_random() % UA_UINT16_MAX;
    else
        eventSeverity = 100;
    UA_Server_writeObjectProperty_scalar(server, *outId,
                                         UA_QUALIFIEDNAME(0, "Severity"),
                                         &eventSeverity, &UA_TYPES[UA_TYPES_UINT16]);
    UA_LocalizedText eventMessage =
        UA_LOCALIZEDTEXT("en-US", "An event has been generated.");
    UA_Server_writeObjectProperty_scalar(server, *outId,
                                         UA_QUALIFIEDNAME(0, "Message"),
                                         &eventMessage, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    UA_UInt16 progress;
    UA_UInt32 context;
    UA_String eventSourceName;
    UA_StatusCode statusCode;
    UA_Boolean eventStatus;
    UA_String serverId;
    UA_String clientUserId;
    UA_String clientAuditEntryId;
    switch(index) {
        case 0: // SampleBaseEventType
        case 1: // SampleDeviceFailureEventType
            eventSourceName = UA_STRING("Server");
            break;
        case 2: // SampleEventQueueOverflowEventType
            eventSourceName = UA_STRING("Internal/EventQueueOverflow");
            break;
        case 3: // SampleProgressEventType
            eventSourceName = UA_STRING("Service/Sample");
            if(random) progress = (UA_UInt16) UA_UInt32_random() % 100;
            else progress = 100;
            UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "Progress"),
                                                 &progress, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
            context = 0;
            UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "Context"),
                                                 &context, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
            break;
        case 4: // SampleAuditSecurityEventType
            eventSourceName = UA_STRING("Server");
            UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "ActionTimeStamp"),
                                                 &eventTime, &UA_TYPES[UA_TYPES_DATETIME]);
            eventStatus = true;
            UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "Status"),
                                                 &eventStatus, &UA_TYPES[UA_TYPES_DATETIME]);
            serverId = UA_STRING("SampleServerId");
            UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "ServerId"),
                                                 &serverId, &UA_TYPES[UA_TYPES_DATETIME]);
            clientUserId = UA_STRING("SampleClientUserId");
            UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "ClientUserId"),
                                                 &clientUserId, &UA_TYPES[UA_TYPES_DATETIME]);
            clientAuditEntryId = UA_STRING("SampleAuditEntryId");
            UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "ClientAuditEntryId"),
                                                 &clientAuditEntryId, &UA_TYPES[UA_TYPES_DATETIME]);
            statusCode = UA_STATUSCODE_GOOD;
            UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "StatusCodeId"),
                                                 &statusCode, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
            break;
        default:
            eventSourceName = UA_STRING("Server");
            break;
    }
    UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "SourceName"),
                                         &eventSourceName, &UA_TYPES[UA_TYPES_STRING]);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
addGenerateSampleEventsMethodCallback(UA_Server *server,
                             const UA_NodeId *sessionId, void *sessionHandle,
                             const UA_NodeId *methodId, void *methodContext,
                             const UA_NodeId *objectId, void *objectContext,
                             size_t inputSize, const UA_Variant *input,
                             size_t outputSize, UA_Variant *output) {
    UA_StatusCode retval;
    UA_NodeId* eventNodeId;
    eventNodeId = (UA_NodeId*)
        UA_Array_new(SAMPLE_EVENT_TYPES_COUNT, &UA_TYPES[UA_TYPES_NODEID]);
    /* set up events */
    for(size_t i = 0 ; i < SAMPLE_EVENT_TYPES_COUNT; i++) {
        retval = setUpEvent(server, &eventNodeId[i], eventTypes[i], false,i);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                           "Creating event failed. StatusCode %s", UA_StatusCode_name(retval));
            return retval;
        }
    }
    for(size_t i = 0 ; i < SAMPLE_EVENT_TYPES_COUNT; i++) {
        retval = UA_Server_triggerEvent(server, eventNodeId[i],
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                                        NULL, UA_TRUE);
        if(retval != UA_STATUSCODE_GOOD)
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                           "Triggering event failed. StatusCode %s", UA_StatusCode_name(retval));
    }
    return retval;
}

static UA_StatusCode
generateRandomEventMethodCallback(UA_Server *server,
                                  const UA_NodeId *sessionId, void *sessionHandle,
                                  const UA_NodeId *methodId, void *methodContext,
                                  const UA_NodeId *objectId, void *objectContext,
                                  size_t inputSize, const UA_Variant *input,
                                  size_t outputSize, UA_Variant *output) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Creating event");
    UA_UInt32 random = (UA_UInt32) UA_UInt32_random() % SAMPLE_EVENT_TYPES_COUNT;
    /* set up event */
    UA_NodeId eventNodeId;
    UA_StatusCode retval = setUpEvent(server, &eventNodeId, eventTypes[random], true, random);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "Creating event failed. StatusCode %s", UA_StatusCode_name(retval));
        return retval;
    }
    retval = UA_Server_triggerEvent(server, eventNodeId,
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                                    NULL, UA_TRUE);
    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "Triggering event failed. StatusCode %s", UA_StatusCode_name(retval));
    return retval;
}

static UA_StatusCode
generateCustomizedEventMethodCallback(UA_Server *server,
                                      const UA_NodeId *sessionId, void *sessionHandle,
                                      const UA_NodeId *methodId, void *methodContext,
                                      const UA_NodeId *objectId, void *objectContext,
                                      size_t inputSize, const UA_Variant *input,
                                      size_t outputSize, UA_Variant *output) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Creating customized event");
    UA_UInt16 *severity = (UA_UInt16*)input->data;
    UA_UInt32 random = (UA_UInt32) UA_UInt32_random() % SAMPLE_EVENT_TYPES_COUNT;
    UA_NodeId eventNodeId;
    UA_StatusCode retval = setUpEvent(server, &eventNodeId, eventTypes[random], true, random);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "Creating event failed. StatusCode %s", UA_StatusCode_name(retval));
        return retval;
    }
    //Overwrite severity
    UA_Server_writeObjectProperty_scalar(server, eventNodeId,
                                         UA_QUALIFIEDNAME(0, "Severity"),
                                         severity, &UA_TYPES[UA_TYPES_UINT16]);
    retval = UA_Server_triggerEvent(server, eventNodeId,
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                                    NULL, UA_TRUE);
    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "Triggering event failed. StatusCode %s", UA_StatusCode_name(retval));

    return UA_STATUSCODE_GOOD;
}

static void
addGenerateSampleEventsMethod(UA_Server *server) {
    UA_MethodAttributes generateAttr = UA_MethodAttributes_default;
    generateAttr.description = UA_LOCALIZEDTEXT("en-US","Generate all configured sample events.");
    generateAttr.displayName = UA_LOCALIZEDTEXT("en-US","Generate sample events");
    generateAttr.executable = true;
    generateAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, 65000),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "Generate Sample Events"),
                            generateAttr, &addGenerateSampleEventsMethodCallback,
                            0, NULL, 0, NULL, NULL, NULL);
}

static void
addGenerateSingleRandomEventMethod(UA_Server *server) {
    UA_MethodAttributes generateAttr = UA_MethodAttributes_default;
    generateAttr.description = UA_LOCALIZEDTEXT("en-US","Generate a random event out of all possible events on the Server.");
    generateAttr.displayName = UA_LOCALIZEDTEXT("en-US","Generate single random event");
    generateAttr.executable = true;
    generateAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, 65001),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "Generate random Event"),
                            generateAttr, &generateRandomEventMethodCallback,
                            0, NULL, 0, NULL, NULL, NULL);
}

static void
addGenerateSingleCustomizedEventMethod(UA_Server *server) {
    UA_Argument inputArgument;
    UA_Argument_init(&inputArgument);
    inputArgument.description = UA_LOCALIZEDTEXT("en-US", "Severity");
    inputArgument.name = UA_STRING("Severity");
    inputArgument.dataType = UA_TYPES[UA_TYPES_UINT16].typeId;
    inputArgument.valueRank = UA_VALUERANK_SCALAR;

    UA_MethodAttributes generateAttr = UA_MethodAttributes_default;
    generateAttr.description = UA_LOCALIZEDTEXT("en-US","Generate a customized event.");
    generateAttr.displayName = UA_LOCALIZEDTEXT("en-US","Generate customized event");
    generateAttr.executable = true;
    generateAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, 65002),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "Generate customized Event"),
                            generateAttr, &generateCustomizedEventMethodCallback,
                            1, &inputArgument, 0, NULL, NULL, NULL);
}

static void stopHandler(int sig) {
    running = false;
}

int main(int argc, char *argv[]) {
    /* default server values */
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    //setup events
    addSampleEventTypes(server);
    addGenerateSampleEventsMethod(server);
    addGenerateSingleRandomEventMethod(server);
    addGenerateSingleCustomizedEventMethod(server);

    UA_StatusCode retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
