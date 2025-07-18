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

#include <stdlib.h>

#define SAMPLE_EVENT_TYPES_COUNT 5
static UA_NodeId eventTypes[SAMPLE_EVENT_TYPES_COUNT];

static void
addEventType(UA_Server *server, char* name, UA_NodeId parentNodeId,
             UA_NodeId requestedId, UA_NodeId* eventType) {
    UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", name);
    attr.description = UA_LOCALIZEDTEXT("en-US", "Sample event type");
    UA_StatusCode retval = UA_Server_addObjectTypeNode(server, requestedId,
                                                       parentNodeId,
                                                       UA_NS0ID(HASSUBTYPE),
                                                       UA_QUALIFIEDNAME(0, name),
                                                       attr, NULL, eventType);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                       "Add EventType failed. StatusCode %s",
                       UA_StatusCode_name(retval));
    }
}

static void
addSampleEventTypes(UA_Server *server) {
    addEventType(server, "SampleBaseEventType", UA_NS0ID(BASEEVENTTYPE),
                 UA_NODEID_NUMERIC(1, 5000), &eventTypes[0]);
    addEventType(server, "SampleDeviceFailureEventType", UA_NS0ID(BASEEVENTTYPE),
                 UA_NODEID_NUMERIC(1, 5001), &eventTypes[1]);
    addEventType(server, "SampleEventQueueOverflowEventType",
                 UA_NS0ID(EVENTQUEUEOVERFLOWEVENTTYPE),
                 UA_NODEID_NUMERIC(1, 5002), &eventTypes[2]);
    addEventType(server, "SampleProgressEventType", UA_NS0ID(BASEEVENTTYPE),
                 UA_NODEID_NUMERIC(1, 5003), &eventTypes[3]);
    addEventType(server, "SampleAuditSecurityEventType", UA_NS0ID(BASEEVENTTYPE),
                 UA_NODEID_NUMERIC(1, 5004), &eventTypes[4]);
}

static void
createEvent(UA_Server *server, UA_NodeId eventType,
            UA_UInt16 severity, UA_Boolean random, size_t index) {
    /* Set the Event Properties */
    UA_KeyValueMap eventFields = UA_KEYVALUEMAP_NULL;

    if(random)
        severity = (UA_UInt16) UA_UInt32_random() % UA_UINT16_MAX;

    UA_LocalizedText eventMessage =
        UA_LOCALIZEDTEXT("en-US", "An event has been generated.");
    UA_KeyValueMap_setScalar(&eventFields, UA_QUALIFIEDNAME(0, "/Message"),
                             &eventMessage, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);

    switch(index) {
    case 0: // SampleBaseEventType
    case 1: // SampleDeviceFailureEventType
        break;
    case 2: { // SampleEventQueueOverflowEventType
        UA_String eventSourceName = UA_STRING("Internal/EventQueueOverflow");
        UA_KeyValueMap_setScalar(&eventFields, UA_QUALIFIEDNAME(0, "/SourceName"),
                                 &eventSourceName, &UA_TYPES[UA_TYPES_STRING]);
        break;
    }
    case 3: { // SampleProgressEventType
        UA_String eventSourceName = UA_STRING("Service/Sample");
        UA_KeyValueMap_setScalar(&eventFields, UA_QUALIFIEDNAME(0, "/SourceName"),
                                 &eventSourceName, &UA_TYPES[UA_TYPES_STRING]);

        UA_UInt16 progress = (random) ? (UA_UInt16) UA_UInt32_random() % 100 : 100;
        UA_KeyValueMap_setScalar(&eventFields, UA_QUALIFIEDNAME(0, "/Progress"),
                                 &progress, &UA_TYPES[UA_TYPES_UINT16]);

        UA_UInt32 context = 0;
        UA_KeyValueMap_setScalar(&eventFields, UA_QUALIFIEDNAME(0, "/Context"),
                                 &context, &UA_TYPES[UA_TYPES_UINT32]);
        break;
    }
    case 4: { // SampleAuditSecurityEventType
        UA_DateTime actionTime = UA_DateTime_now();
        UA_KeyValueMap_setScalar(&eventFields, UA_QUALIFIEDNAME(0, "/ActionTimeStamp"),
                                 &actionTime, &UA_TYPES[UA_TYPES_DATETIME]);

        UA_Boolean eventStatus = true;
        UA_KeyValueMap_setScalar(&eventFields, UA_QUALIFIEDNAME(0, "/Status"),
                                 &eventStatus, &UA_TYPES[UA_TYPES_BOOLEAN]);

        UA_String serverId = UA_STRING("SampleServerId");
        UA_KeyValueMap_setScalar(&eventFields, UA_QUALIFIEDNAME(0, "/ServerId"),
                                 &serverId, &UA_TYPES[UA_TYPES_STRING]);

        UA_String clientUserId = UA_STRING("SampleClientUserId");
        UA_KeyValueMap_setScalar(&eventFields, UA_QUALIFIEDNAME(0, "/ClientUserId"),
                                 &clientUserId, &UA_TYPES[UA_TYPES_STRING]);

        UA_String clientAuditEntryId = UA_STRING("SampleAuditEntryId");
        UA_KeyValueMap_setScalar(&eventFields, UA_QUALIFIEDNAME(0, "/ClientAuditEntryId"),
                                 &clientAuditEntryId, &UA_TYPES[UA_TYPES_STRING]);

        UA_StatusCode statusCode = UA_STATUSCODE_GOOD;
        UA_KeyValueMap_setScalar(&eventFields, UA_QUALIFIEDNAME(0, "/StatusCodeId"),
                                 &statusCode, &UA_TYPES[UA_TYPES_STATUSCODE]);
        break;
    }
    default:
        break;
    }

    UA_StatusCode res = UA_Server_createEvent(server, eventType, UA_NS0ID(SERVER),
                                              severity, eventFields);
    UA_KeyValueMap_clear(&eventFields);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                       "CreateEvent failed. StatusCode %s",
                       UA_StatusCode_name(res));
    }
}

static UA_StatusCode
addGenerateSampleEventsMethodCallback(UA_Server *server,
                                      const UA_NodeId *sessionId, void *sessionHandle,
                                      const UA_NodeId *methodId, void *methodContext,
                                      const UA_NodeId *objectId, void *objectContext,
                                      size_t inputSize, const UA_Variant *input,
                                      size_t outputSize, UA_Variant *output) {
    for(size_t i = 0 ; i < SAMPLE_EVENT_TYPES_COUNT; i++) {
        createEvent(server, eventTypes[i], 100, false, i);
    }
    return UA_STATUSCODE_GOOD;
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
    createEvent(server, eventTypes[random], 100, true, random);
    return UA_STATUSCODE_GOOD;
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
    createEvent(server, eventTypes[random], *severity, true, random);
    return UA_STATUSCODE_GOOD;
}

static void
addGenerateSampleEventsMethod(UA_Server *server) {
    UA_MethodAttributes generateAttr = UA_MethodAttributes_default;
    generateAttr.description =
        UA_LOCALIZEDTEXT("en-US","Generate all configured sample events.");
    generateAttr.displayName = UA_LOCALIZEDTEXT("en-US","Generate sample events");
    generateAttr.executable = true;
    generateAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, 65000),
                            UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "Generate Sample Events"),
                            generateAttr, &addGenerateSampleEventsMethodCallback,
                            0, NULL, 0, NULL, NULL, NULL);
}

static void
addGenerateSingleRandomEventMethod(UA_Server *server) {
    UA_MethodAttributes generateAttr = UA_MethodAttributes_default;
    generateAttr.description =
        UA_LOCALIZEDTEXT("en-US","Generate a random event out of all possible events on the Server.");
    generateAttr.displayName = UA_LOCALIZEDTEXT("en-US","Generate single random event");
    generateAttr.executable = true;
    generateAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, 65001),
                            UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(HASCOMPONENT),
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
                            UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "Generate customized Event"),
                            generateAttr, &generateCustomizedEventMethodCallback,
                            1, &inputArgument, 0, NULL, NULL, NULL);
}

int main(int argc, char *argv[]) {
    UA_Server *server = UA_Server_new();

    addSampleEventTypes(server);
    addGenerateSampleEventsMethod(server);
    addGenerateSingleRandomEventMethod(server);
    addGenerateSingleCustomizedEventMethod(server);

    UA_Server_runUntilInterrupt(server);
    UA_Server_delete(server);
    return 0;
}
