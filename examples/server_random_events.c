/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>

/**
 * Generating events
 * -----------------
 * To make sense of the many things going on in a server, monitoring items can be useful.
 * Though in many cases, data change does not convey enough information to be the optimal
 * solution. Events can be generated at any time, hold a lot of information and can be
 * filtered so the client only receives the specific attributes he is interested in.
 *
 * Emitting events by calling methods
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * The following example is an extension of the server event tutorial. We will be
 * creating two method nodes. One which generates multiple event from the server node and
 * one which generates a random event
 *
 * The event we want to generate should not be simple, in order to allow the client to set
 * a meaningful filter. `EventTypes` are saved internally as `ObjectTypes`,
 * so add the a few types as you would new `ObjectTypes`.
 */

static const size_t eventTypesCount = 5;
static UA_NodeId* eventTypes;

static UA_StatusCode
addEventType(UA_Server *server, char* name, UA_NodeId parentNodeId, UA_NodeId* eventType) {
    UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", name);
    attr.description = UA_LOCALIZEDTEXT("en-US", "This is a sample event type we created");
    UA_StatusCode retval = UA_Server_addObjectTypeNode(server, UA_NODEID_NULL,
                                                       parentNodeId,
                                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                                       UA_QUALIFIEDNAME(0, name),
                                                       attr, NULL, eventType);
    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                       "add EventType failed. StatusCode %s", UA_StatusCode_name(retval));
    }
    return retval;
}

static UA_StatusCode
addSampleEventTypes(UA_Server *server) {
    eventTypes = (UA_NodeId*)
        UA_Array_new(eventTypesCount, &UA_TYPES[UA_TYPES_NODEID]);
    UA_NodeId_init(eventTypes);
    UA_StatusCode retval = addEventType(server, "SampleBaseEventType",
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE),&eventTypes[0]);
    if (retval != UA_STATUSCODE_GOOD) return retval;
    retval = addEventType(server, "SampleDeviceFailureEventType",
                          UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE), &eventTypes[1]);
    if (retval != UA_STATUSCODE_GOOD) return retval;
    retval = addEventType(server, "SampleEventQueueOverflowEventType",
                          UA_NODEID_NUMERIC(0, UA_NS0ID_EVENTQUEUEOVERFLOWEVENTTYPE),&eventTypes[2]);
    if (retval != UA_STATUSCODE_GOOD) return retval;
    retval = addEventType(server, "SampleProgressEventType",
                          UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE), &eventTypes[3]);
    if (retval != UA_STATUSCODE_GOOD) return retval;
    retval = addEventType(server, "SampleAuditSecurityEventType",
                          UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE), &eventTypes[4]);
    if (retval != UA_STATUSCODE_GOOD) return retval;
    return UA_STATUSCODE_GOOD;
}

/**
 * Setting up an event
 * ^^^^^^^^^^^^^^^^^^^
 * In order to set up the event, we can first use ``UA_Server_createEvent`` to give us node representations of the events.
 * All we need for this is our `EventTypes`. Once we have our event nodes, which are saved internally as `ObjectNodes`,
 * we can define the attributes the different events have the same way we would define the attributes of an object node. It is not
 * necessary to define the attributes `EventId`, `ReceiveTime`, `SourceNode` or `EventType` since these are set
 * automatically by the server. In this example, we will be setting the fields 'Message' and 'Severity' in addition
 * to `Time` which is needed to make the example UaExpert compliant.
 */

static UA_StatusCode
setUpEvent(UA_Server *server, UA_NodeId *outId, UA_NodeId eventType, UA_Boolean random,size_t index) {
    UA_StatusCode retval = UA_Server_createEvent(server, eventType, outId);
    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                       "createEvent failed. StatusCode %s", UA_StatusCode_name(retval));
        return retval;
    }
    /* Set the Event Attributes */
    /* Setting the Time is required or else the event will not show up in UAExpert! */
    UA_DateTime eventTime = UA_DateTime_now();
    UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "Time"),
                                         &eventTime, &UA_TYPES[UA_TYPES_DATETIME]);
    UA_UInt16 eventSeverity;
    if(random) eventSeverity = (UA_UInt16) rand() % 1000;
    else eventSeverity = 100;
    UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "Severity"),
                                         &eventSeverity, &UA_TYPES[UA_TYPES_UINT16]);
    UA_LocalizedText eventMessage = UA_LOCALIZEDTEXT("en-US", "An event has been generated.");
    UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "Message"),
                                         &eventMessage, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    UA_UInt16 progress;
    UA_UInt32 context;
    UA_String eventSourceName;
    UA_StatusCode statusCode;
    UA_Boolean Estatus = UA_Boolean_new();
    UA_Boolean_init(&Estatus);
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
            if(random) progress = (UA_UInt16) rand() % 100;
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
            Estatus = UA_TRUE;
            UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "Status"),
                                                 &Estatus, &UA_TYPES[UA_TYPES_DATETIME]);
            serverId = UA_STRING("SampleServer");
            UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "ServerId"),
                                                 &serverId, &UA_TYPES[UA_TYPES_DATETIME]);
            clientUserId = UA_STRING("SampleClient");
            UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "ClientUserId"),
                                                 &clientUserId, &UA_TYPES[UA_TYPES_DATETIME]);
            clientAuditEntryId = UA_STRING("SampleAudit");
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

/**
 * Triggering an event
 * ^^^^^^^^^^^^^^^^^^^
 * First a node representing an event is generated using ``setUpEvent``. Once our event is good to go, we specify
 * a node which emits the event - in this case the server node. We can use ``UA_Server_triggerEvent`` to trigger our
 * event onto said node. Passing ``NULL`` as the second-last argument means we will not receive the `EventId`.
 * The last boolean argument states whether the node should be deleted.
 */

static UA_StatusCode
generateEventsMethodCallback(UA_Server *server,
                             const UA_NodeId *sessionId, void *sessionHandle,
                             const UA_NodeId *methodId, void *methodContext,
                             const UA_NodeId *objectId, void *objectContext,
                             size_t inputSize, const UA_Variant *input,
                             size_t outputSize, UA_Variant *output) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Creating event");
    UA_StatusCode retval;
    UA_NodeId* eventNodeId;
    eventNodeId = (UA_NodeId*)
        UA_Array_new(eventTypesCount, &UA_TYPES[UA_TYPES_NODEID]);
    /* set up event */
    for(size_t i = 0 ; i < eventTypesCount; i++) {
        retval = setUpEvent(server, &eventNodeId[i], eventTypes[i], false,i);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                           "Creating event failed. StatusCode %s", UA_StatusCode_name(retval));
            return retval;
        }
    }
    for(size_t i = 0 ; i < eventTypesCount; i++) {
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
    size_t random = (size_t) rand() % eventTypesCount;
    /* set up event */
    UA_NodeId eventNodeId;
    UA_StatusCode retval = setUpEvent(server, &eventNodeId, eventTypes[random], true,random);
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

/**
 * Now, all that is left to do is to create a method node which uses our callback. We do not
 * require any input and as output we will be using the `EventId` we receive from ``triggerEvent``. The `EventId` is
 * generated by the server internally and is a random unique ID which identifies that specific event.
 * This method node will be added to the server.
 */

static void
addGenerateEventsMethod(UA_Server *server) {
    UA_MethodAttributes generateAttr = UA_MethodAttributes_default;
    generateAttr.description = UA_LOCALIZEDTEXT("en-US","Generate all possible events on the Server.");
    generateAttr.displayName = UA_LOCALIZEDTEXT("en-US","Generate Events");
    generateAttr.executable = true;
    generateAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, 62541),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "Generate Events"),
                            generateAttr, &generateEventsMethodCallback,
                            0, NULL, 0, NULL, NULL, NULL);
}

static void
addGenerateRandomEventMethod(UA_Server *server) {
    UA_MethodAttributes generateAttr = UA_MethodAttributes_default;
    generateAttr.description = UA_LOCALIZEDTEXT("en-US","Generate a random event out of all possible events on the Server.");
    generateAttr.displayName = UA_LOCALIZEDTEXT("en-US","Generate random Event");
    generateAttr.executable = true;
    generateAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, 62541),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "Generate random Event"),
                            generateAttr, &generateRandomEventMethodCallback,
                            0, NULL, 0, NULL, NULL, NULL);
}

/** It follows the main server code, making use of the above definitions. */

static volatile UA_Boolean running = true;
static void stopHandler(int sig) {
    running = false;
}

int main (void) {
    /* default server values */
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    addSampleEventTypes(server);
    addGenerateRandomEventMethod(server);
    addGenerateEventsMethod(server);

    UA_StatusCode retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
