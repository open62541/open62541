/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/* Enable POSIX features */
#if !defined(_XOPEN_SOURCE) && !defined(_WRS_KERNEL)
# define _XOPEN_SOURCE 600
#endif
#ifndef _DEFAULT_SOURCE
# define _DEFAULT_SOURCE
#endif
/* On older systems we need to define _BSD_SOURCE.
 * _DEFAULT_SOURCE is an alias for that. */
#ifndef _BSD_SOURCE
# define _BSD_SOURCE
#endif

/**
 * Client disconnect handling
 * --------------------------
 * This example shows you how to handle a client disconnect, e.g., if the server
 * is shut down while the client is connected. You just need to call connect
 * again and the client will automatically reconnect.
 *
 * This example is very similar to the tutorial_client_firststeps.c. */

#include "open62541.h"
#include <signal.h>

#ifdef _WIN32
# include <windows.h>
# define UA_sleep_ms(X) Sleep(X)
#else
# include <unistd.h>
# define UA_sleep_ms(X) usleep(X * 1000)
#endif

UA_Boolean running = true;
UA_Logger logger = UA_Log_Stdout;

static void stopHandler(int sign) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "Received Ctrl-C");
    running = 0;
}

static void
handler_currentTimeChanged(UA_Client *client, UA_UInt32 subId, void *subContext,
                           UA_UInt32 monId, void *monContext, UA_DataValue *value) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "currentTime has changed!");
    if(UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_DATETIME])) {
        UA_DateTime raw_date = *(UA_DateTime *) value->value.data;
        UA_DateTimeStruct dts = UA_DateTime_toStruct(raw_date);
        UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "date is: %02u-%02u-%04u %02u:%02u:%02u.%03u",
                        dts.day, dts.month, dts.year, dts.hour, dts.min, dts.sec, dts.milliSec);
    }
}

static void
deleteSubscriptionCallback(UA_Client *client, UA_UInt32 subscriptionId, void *subscriptionContext) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "Subscription Id %u was deleted", subscriptionId);
}

static void
subscriptionInactivityCallback (UA_Client *client, UA_UInt32 subId, void *subContext) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "Inactivity for subscription %u", subId);
}

static void
stateCallback (UA_Client *client, UA_ClientState clientState) {
    switch(clientState) {
        case UA_CLIENTSTATE_DISCONNECTED:
            UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "The client is disconnected");
        break;
        case UA_CLIENTSTATE_WAITING_FOR_ACK:
            UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "Waiting for ack");
        break;
        case UA_CLIENTSTATE_CONNECTED:
            UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "A TCP connection to the server is open");
        break;
        case UA_CLIENTSTATE_SECURECHANNEL:
            UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "A SecureChannel to the server is open");
        break;
        case UA_CLIENTSTATE_SESSION:{
            UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "A session with the server is open");
            /* A new session was created. We need to create the subscription. */
            /* Create a subscription */
            UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
            UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                                    NULL, NULL, deleteSubscriptionCallback);

            if(response.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
                UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "Create subscription succeeded, id %u", response.subscriptionId);
            else
                return;

            /* Add a MonitoredItem */
            UA_MonitoredItemCreateRequest monRequest =
                UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME));

            UA_MonitoredItemCreateResult monResponse =
                UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
                                                          UA_TIMESTAMPSTORETURN_BOTH,
                                                          monRequest, NULL, handler_currentTimeChanged, NULL);
            if(monResponse.statusCode == UA_STATUSCODE_GOOD)
                UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "Monitoring UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME', id %u", monResponse.monitoredItemId);
        }
        break;
        case UA_CLIENTSTATE_SESSION_RENEWED:
            UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "A session with the server is open (renewed)");
            /* The session was renewed. We don't need to recreate the subscription. */
        break;
        case UA_CLIENTSTATE_SESSION_DISCONNECTED:
            UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "Session disconnected");
        break;
    }
    return;
}

int main(void) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */

    UA_ClientConfig config = UA_ClientConfig_default;
    /* Set stateCallback */
    config.stateCallback = stateCallback;
    config.subscriptionInactivityCallback = subscriptionInactivityCallback;

    UA_Client *client = UA_Client_new(config);

    /* Endless loop runAsync */
    while (running) {
        /* if already connected, this will return GOOD and do nothing */
        /* if the connection is closed/errored, the connection will be reset and then reconnected */
        /* Alternatively you can also use UA_Client_getState to get the current state */
        UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_USERLAND, "Not connected. Retrying to connect in 1 second");
            /* The connect may timeout after 1 second (see above) or it may fail immediately on network errors */
            /* E.g. name resolution errors or unreachable network. Thus there should be a small sleep here */
            UA_sleep_ms(1000);
            continue;
        }

        UA_Client_run_iterate(client, 1000);
    };

    /* Clean up */
    UA_Client_delete(client); /* Disconnects the client internally */
    return UA_STATUSCODE_GOOD;
}
