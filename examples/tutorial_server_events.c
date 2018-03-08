/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Generating events
 * -----------------
 * To make sense of the many things going on in a server, monitoring items can be useful. Though in many cases, data
 * change does not convey enough information to be the optimal solution. Events can be generated at any time,
 * hold a lot of information and can be filtered so the client only receives the specific attributes he is interested in.
 *
 * Emitting events by calling methods
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * The following example will be based on the server method tutorial. We will be
 * creating a method node which generates an event from the server node.
 */

#include <signal.h>
#include "open62541.h"

UA_Boolean running = true;
static void stopHandler(int sig) {
    running = false;
}

/** The event we want to generate should be very simple. Since the `BaseEventType` is abstract,
 * we will have to create our own event type. `EventTypes` are saved internally as `ObjectTypes`,
 * so add the type as you would a new `ObjectType`.
 */

static UA_NodeId eventType;

static UA_StatusCode addNewEventType(UA_Server *server) {
    UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US", "SimpleEventType");
    attr.description = UA_LOCALIZEDTEXT_ALLOC("en-US", "The simple event type we created");

    UA_Server_addObjectTypeNode(server, UA_NODEID_NULL,
                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE),
                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                UA_QUALIFIEDNAME(0, "SimpleEventType"),
                                attr, NULL, &eventType);
    UA_LocalizedText_deleteMembers(&attr.displayName);
    UA_LocalizedText_deleteMembers(&attr.description);
    return UA_STATUSCODE_GOOD;
}

/** Setting up an event
 * ^^^^^^^^^^^^^^^^^^^^^^
 * In order to set up the event, we can first use ``UA_Server_createEvent`` to give us a node representation of the event.
 * All we need for this is our `EventType`. Once we have our event node, which is saved internally as an `ObjectNode`,
 * we can define the attributes the event has the same way we would define the attributes of an object node. It is not
 * necessary to define the attributes `EventId`, `ReceiveTime`, `SourceNode` or `EventType` since these are set
 * automatically by the server. In this example, we will only be setting `Severity` and `Message`.
 */
static UA_StatusCode setUpEvent(UA_Server *server, UA_NodeId *outId) {
    UA_StatusCode retval;
    retval = UA_Server_createEvent(server, eventType, outId, NULL);
    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                       "createEvent failed. StatusCode %s", UA_StatusCode_name(retval));
        return retval;
    }

    UA_Variant value;
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    rpe.isInverse = false;
    rpe.includeSubtypes = false;

    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = *outId;
    bp.relativePath.elementsSize = 1;
    bp.relativePath.elements = &rpe;

    /* severity */
    rpe.targetName = UA_QUALIFIEDNAME(0, "Severity");
    UA_BrowsePathResult bpr = UA_Server_translateBrowsePathToNodeIds(server, &bp);
    if (bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Event is missing severity attribute.\n");
        return bpr.statusCode;
    }
    UA_UInt16 eventSeverity = 100;
    UA_Variant_setScalar(&value, &eventSeverity, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_writeValue(server, bpr.targets[0].targetId.nodeId, value);

    UA_BrowsePathResult_deleteMembers(&bpr);

    /* message */
    rpe.targetName = UA_QUALIFIEDNAME(0, "Message");
    bpr = UA_Server_translateBrowsePathToNodeIds(server, &bp);
    if (bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Event is missing message attribute.\n");
        return bpr.statusCode;
    }
    UA_LocalizedText eventMessage = UA_LOCALIZEDTEXT("en-US", "An event has been generated.");
    UA_Variant_setScalar(&value, &eventMessage, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    UA_Server_writeValue(server, bpr.targets[0].targetId.nodeId, value);

    UA_BrowsePathResult_deleteMembers(&bpr);

    return UA_STATUSCODE_GOOD;
}

/** Triggering an event
 * ^^^^^^^^^^^^^^^^^^^^
 * First a node representing an event is generated using ``setUpEvent``. Once our event is good to go, we specify
 * a node which emits the event - in this case the server node. We can use ``UA_Server_triggerEvent`` to trigger our
 * event onto said node. Note that once ``UA_Sever_triggerEvent`` has been called, the node representation of the event
 * is deleted.
 */
static UA_StatusCode
generateEventMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output) {
    /* set up event */
    UA_NodeId eventNodeId;
    UA_StatusCode retval = setUpEvent(server, &eventNodeId);
    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "Creating event failed. StatusCode %s", UA_StatusCode_name(retval));
        return retval;
    }

    retval = UA_Server_triggerEvent(server, &eventNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER));
    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                       "Triggering event failed. StatusCode %s", UA_StatusCode_name(retval));
        return retval;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Generate Event was called.");
    return retval;
}

/** Now, all that is left to do is to create a method node which uses our callback. We do not
 * require any input and as output we will be using the `EventId` we receive from ``triggerEvent``. The `EventId` is
 * generated by the server internally and is a random unique ID which identifies that specific event.
 *
 * This method node will be added to a basic server setup.
 */

static void
addGenerateEventMethod(UA_Server *server) {
    UA_MethodAttributes generateAttr = UA_MethodAttributes_default;
    generateAttr.description = UA_LOCALIZEDTEXT("en-US","Generate an event.");
    generateAttr.displayName = UA_LOCALIZEDTEXT("en-US","Generate Event");
    generateAttr.executable = true;
    generateAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, 62541),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT),
                            UA_QUALIFIEDNAME(1, "Generate Event"),
                            generateAttr, &generateEventMethodCallback,
                            0, NULL, 0, NULL, NULL, NULL);
}

int main (void) {
    /* default server values */
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_ServerConfig *config = UA_ServerConfig_new_default();
    UA_Server *server = UA_Server_new(config);

    addNewEventType(server);
    addGenerateEventMethod(server);

    /* return value */
    UA_StatusCode retval = UA_Server_run(server, &running);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);

    return (int) retval;
}