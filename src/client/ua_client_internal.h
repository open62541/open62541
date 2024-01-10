/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2015-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2015 (c) Oleksiy Vasylyev
 *    Copyright 2016-2017 (c) Florian Palm
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Mark Giraud, Fraunhofer IOSB
 */

#ifndef UA_CLIENT_INTERNAL_H_
#define UA_CLIENT_INTERNAL_H_

#define UA_INTERNAL
#include <open62541/client.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_subscriptions.h>

#include "open62541_queue.h"
#include "ua_securechannel.h"
#include "ua_timer.h"

_UA_BEGIN_DECLS

/**************************/
/* Subscriptions Handling */
/**************************/

#ifdef UA_ENABLE_SUBSCRIPTIONS

typedef struct UA_Client_NotificationsAckNumber {
    LIST_ENTRY(UA_Client_NotificationsAckNumber) listEntry;
    UA_SubscriptionAcknowledgement subAck;
} UA_Client_NotificationsAckNumber;

typedef struct UA_Client_MonitoredItem {
    LIST_ENTRY(UA_Client_MonitoredItem) listEntry;
    UA_UInt32 monitoredItemId;
    UA_UInt32 clientHandle;
    void *context;
    UA_Client_DeleteMonitoredItemCallback deleteCallback;
    union {
        UA_Client_DataChangeNotificationCallback dataChangeCallback;
        UA_Client_EventNotificationCallback eventCallback;
    } handler;
    UA_Boolean isEventMonitoredItem; /* Otherwise a DataChange MoniitoredItem */
} UA_Client_MonitoredItem;

typedef struct UA_Client_Subscription {
    LIST_ENTRY(UA_Client_Subscription) listEntry;
    UA_UInt32 subscriptionId;
    void *context;
    UA_Double publishingInterval;
    UA_UInt32 maxKeepAliveCount;
    UA_Client_StatusChangeNotificationCallback statusChangeCallback;
    UA_Client_DeleteSubscriptionCallback deleteCallback;
    UA_Client_DataChangeCallback dataChangeCallback;
    UA_UInt32 sequenceNumber;
    UA_DateTime lastActivity;
    LIST_HEAD(, UA_Client_MonitoredItem) monitoredItems;
} UA_Client_Subscription;

void
UA_Client_Subscriptions_clean(UA_Client *client);

/* Exposed for fuzzing */
UA_StatusCode
UA_Client_preparePublishRequest(UA_Client *client, UA_PublishRequest *request);

void
UA_Client_Subscriptions_backgroundPublish(UA_Client *client);

void
UA_Client_Subscriptions_backgroundPublishInactivityCheck(UA_Client *client);

#endif /* UA_ENABLE_SUBSCRIPTIONS */

/**********/
/* Client */
/**********/

typedef struct AsyncServiceCall {
    LIST_ENTRY(AsyncServiceCall) pointers;
    UA_UInt32 requestId;
    UA_ClientAsyncServiceCallback callback;
    const UA_DataType *responseType;
    void *userdata;
    UA_DateTime start;
    UA_UInt32 timeout;
    void *responsedata;
} AsyncServiceCall;

void
UA_Client_AsyncService_cancel(UA_Client *client, AsyncServiceCall *ac,
                              UA_StatusCode statusCode);

void
UA_Client_AsyncService_removeAll(UA_Client *client, UA_StatusCode statusCode);

typedef struct CustomCallback {
    UA_UInt32 callbackId;

    UA_ClientAsyncServiceCallback userCallback;
    void *userData;

    void *clientData;
} CustomCallback;

struct UA_Client {
    UA_ClientConfig config;
    UA_Timer timer;

    /* Overall connection status */
    UA_StatusCode connectStatus;

    /* Old status to notify only changes */
    UA_SecureChannelState oldChannelState;
    UA_SessionState oldSessionState;
    UA_StatusCode oldConnectStatus;

    UA_Boolean findServersHandshake;   /* Ongoing FindServers */
    UA_Boolean endpointsHandshake;     /* Ongoing GetEndpoints */
    UA_Boolean noSession;              /* Don't open a session */

    /* Connection */
    UA_Connection connection;
    UA_String endpointUrl;  /* Used to extract address and port */
    UA_String discoveryUrl; /* The discoveryUrl (also used to signal which
                               application we want to connect to in the HEL/ACK
                               handshake). */

    /* SecureChannel */
    UA_SecureChannel channel;
    UA_UInt32 requestId;
    UA_DateTime nextChannelRenewal;

    /* Session */
    UA_SessionState sessionState;
    UA_NodeId authenticationToken;
    UA_UInt32 requestHandle;
    UA_ByteString remoteNonce;
    UA_ByteString localNonce;

    /* Connectivity check */
    UA_DateTime lastConnectivityCheck;
    UA_Boolean pendingConnectivityCheck;

    /* Async Service */
    LIST_HEAD(, AsyncServiceCall) asyncServiceCalls;

    /* Subscriptions */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    LIST_HEAD(, UA_Client_NotificationsAckNumber) pendingNotificationsAcks;
    LIST_HEAD(, UA_Client_Subscription) subscriptions;
    UA_UInt32 monitoredItemHandles;
    UA_UInt16 currentlyOutStandingPublishRequests;
#endif
};

UA_StatusCode connectSync(UA_Client *client); /* Only exposed for unit tests */
void notifyClientState(UA_Client *client);
void processERRResponse(UA_Client *client, const UA_ByteString *chunk);
void processACKResponse(UA_Client *client, const UA_ByteString *chunk);
void processOPNResponse(UA_Client *client, const UA_ByteString *message);
void closeSecureChannel(UA_Client *client);

UA_StatusCode
connectIterate(UA_Client *client, UA_UInt32 timeout);

UA_StatusCode
receiveResponseAsync(UA_Client *client, UA_UInt32 timeout);

void
Client_warnEndpointsResult(UA_Client *client,
                           const UA_GetEndpointsResponse *response,
                           const UA_String *endpointUrl);

_UA_END_DECLS

#endif /* UA_CLIENT_INTERNAL_H_ */
