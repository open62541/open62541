/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>

/**
 * Generating events
 * -----------------
 * To make sense of the many things going on in a server, monitoring items can
 * be useful. Though in many cases, data change does not convey enough
 * information to be the optimal solution. Events can be generated at any time,
 * hold a lot of information and can be filtered so the client only receives the
 * specific attributes of interest.
 *
 * Emitting events by calling methods
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * The following example will be based on the server method tutorial. We will be
 * creating a method node which generates an event from the server node.
 *
 * The event we want to generate should be very simple. Since the `BaseEventType` is abstract,
 * we will have to create our own event type. `EventTypes` are saved internally as `ObjectTypes`,
 * so add the type as you would a new `ObjectType`. */

static UA_NodeId eventType;

static UA_StatusCode
addNewEventType(UA_Server *server) {
    UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "SimpleEventType");
    attr.description = UA_LOCALIZEDTEXT("en-US", "The simple event type we created");
    return UA_Server_addObjectTypeNode(server, UA_NODEID_NULL,
                                       UA_NS0ID(BASEEVENTTYPE), UA_NS0ID(HASSUBTYPE),
                                       UA_QUALIFIEDNAME(0, "SimpleEventType"),
                                       attr, NULL, &eventType);
}

/**
 * Creating an event
 * ^^^^^^^^^^^^^^^^^
 * In order to set up the event, we can first use ``UA_Server_createEvent`` to
 * give us a node representation of the event. All we need for this is our
 * `EventType`. Once we have our event node, which is saved internally as an
 * `ObjectNode`, we can define the attributes the event has the same way we
 * would define the attributes of an object node. It is not necessary to define
 * the attributes `EventId`, `ReceiveTime`, `SourceNode` or `EventType` since
 * these are set automatically by the server. In this example, we will be
 * setting the fields 'Message' and 'Severity' in addition to `Time` which is
 * needed to make the example UaExpert compliant.
 */

static UA_StatusCode
createEventMethodCallback(UA_Server *server,
                          const UA_NodeId *sessionId, void *sessionHandle,
                          const UA_NodeId *methodId, void *methodContext,
                          const UA_NodeId *objectId, void *objectContext,
                          size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Creating event");

    UA_UInt16 severity = 100;
    UA_LocalizedText message = UA_LOCALIZEDTEXT("en-US", "An event has been generated.");
    UA_StatusCode res = UA_Server_createEvent(server, UA_NS0ID(SERVER), eventType,
                                              severity, message, NULL, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "Creating event failed. StatusCode %s",
                       UA_StatusCode_name(res));
    }
    return res;
}

/**
 * Now, all that is left to do is to create a method node which triggers the
 * event creation for the example.
 */

int main(void) {
    UA_Server *server = UA_Server_new();

    addNewEventType(server);

    UA_MethodAttributes generateAttr = UA_MethodAttributes_default;
    generateAttr.description = UA_LOCALIZEDTEXT("en-US","Generate an event.");
    generateAttr.displayName = UA_LOCALIZEDTEXT("en-US","Generate Event");
    generateAttr.executable = true;
    generateAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, 62541),
                            UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "Generate Event"),
                            generateAttr, &createEventMethodCallback,
                            0, NULL, 0, NULL, NULL, NULL);

    UA_Server_runUntilInterrupt(server);
    UA_Server_delete(server);
    return 0;
}
