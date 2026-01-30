/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Observing Attributes with Local MonitoredItems
 * ----------------------------------------------
 *
 * A client that is interested in the current value of a variable does not need
 * to regularly poll the variable. Instead, the client can use the Subscription
 * mechanism to be notified about changes.
 *
 * So-called MonitoredItems define which values (node attributes) and events the
 * client wants to monitor. Under the right conditions, a notification is
 * created and added to the Subscription. The notifications currently in the
 * queue are regularly sent to the client.
 *
 * The local user can add MonitoredItems as well. Locally, the MonitoredItems do
 * not go via a Subscription and each have an individual callback method and a
 * context pointer.
 */

#include <open62541/client_subscriptions.h>
#include <open62541/server.h>
#include <open62541/plugin/log_stdout.h>

static void
dataChangeNotificationCallback(UA_Server *server, UA_UInt32 monitoredItemId,
                               void *monitoredItemContext, const UA_NodeId *nodeId,
                               void *nodeContext, UA_UInt32 attributeId,
                               const UA_DataValue *value) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "Received Notification");
}

static void
addMonitoredItemToCurrentTimeVariable(UA_Server *server) {
    UA_NodeId currentTimeNodeId = UA_NS0ID(SERVER_SERVERSTATUS_CURRENTTIME);
    UA_MonitoredItemCreateRequest monRequest =
        UA_MonitoredItemCreateRequest_default(currentTimeNodeId);
    monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
    UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, NULL,
                                            dataChangeNotificationCallback);
}

/** It follows the main server code, making use of the above definitions. */

int main(void) {
    UA_Server *server = UA_Server_new();

    addMonitoredItemToCurrentTimeVariable(server);

    UA_Server_runUntilInterrupt(server);
    UA_Server_delete(server);
    return 0;
}
