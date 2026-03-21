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

static void
createEvent(UA_Server *server, UA_Boolean random) {
    /* Set an event field via the key-value map. The variable /SourceName is a
     * component of the BaseEventType and hence can be used. */
    UA_KeyValueMap eventFields = UA_KEYVALUEMAP_NULL;
    UA_String eventSourceName = UA_STRING("Service/Sample");
    UA_KeyValueMap_setScalar(&eventFields, UA_QUALIFIEDNAME(0, "/SourceName"),
                             &eventSourceName, &UA_TYPES[UA_TYPES_STRING]);

    /* Define the mandatory fields for all events */
    UA_UInt16 severity = 100;
    if(random)
        severity = (UA_UInt16) UA_UInt32_random() % UA_UINT16_MAX;
    UA_LocalizedText eventMessage =
        UA_LOCALIZEDTEXT("en-US", "An event has been generated.");

    /* Create the event and clean up */
    UA_Server_createEvent(server, UA_NS0ID(SERVER),
                          UA_NS0ID(BASEEVENTTYPE), severity,
                          eventMessage, &eventFields, NULL, NULL);
    UA_KeyValueMap_clear(&eventFields);
}

static UA_StatusCode
addGenerateSampleEventsMethodCallback(UA_Server *server,
                                      const UA_NodeId *sessionId, void *sessionHandle,
                                      const UA_NodeId *methodId, void *methodContext,
                                      const UA_NodeId *objectId, void *objectContext,
                                      size_t inputSize, const UA_Variant *input,
                                      size_t outputSize, UA_Variant *output) {
    createEvent(server, false);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
generateRandomEventMethodCallback(UA_Server *server,
                                  const UA_NodeId *sessionId, void *sessionHandle,
                                  const UA_NodeId *methodId, void *methodContext,
                                  const UA_NodeId *objectId, void *objectContext,
                                  size_t inputSize, const UA_Variant *input,
                                  size_t outputSize, UA_Variant *output) {
    createEvent(server, true);
    return UA_STATUSCODE_GOOD;
}

static void
addGenerateSampleEventsMethod(UA_Server *server) {
    UA_MethodAttributes generateAttr = UA_MethodAttributes_default;
    generateAttr.description =
        UA_LOCALIZEDTEXT("en-US","Generate a sample events.");
    generateAttr.displayName = UA_LOCALIZEDTEXT("en-US","Generate event");
    generateAttr.executable = true;
    generateAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, 65000),
                            UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "Generate Event"),
                            generateAttr, &addGenerateSampleEventsMethodCallback,
                            0, NULL, 0, NULL, NULL, NULL);
}

static void
addGenerateSingleRandomEventMethod(UA_Server *server) {
    UA_MethodAttributes generateAttr = UA_MethodAttributes_default;
    generateAttr.description =
        UA_LOCALIZEDTEXT("en-US","Generate a random event.");
    generateAttr.displayName = UA_LOCALIZEDTEXT("en-US","Generate random event");
    generateAttr.executable = true;
    generateAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, 65001),
                            UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "Generate random Event"),
                            generateAttr, &generateRandomEventMethodCallback,
                            0, NULL, 0, NULL, NULL, NULL);
}

int main(int argc, char *argv[]) {
    UA_Server *server = UA_Server_new();
    addGenerateSampleEventsMethod(server);
    addGenerateSingleRandomEventMethod(server);
    UA_Server_runUntilInterrupt(server);
    UA_Server_delete(server);
    return 0;
}
