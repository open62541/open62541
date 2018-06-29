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

#include "ua_securechannel.h"
#include "ua_client_highlevel.h"
#include "ua_client_subscriptions.h"
#include "ua_timer.h"
#include "../../deps/queue.h"

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
    UA_UInt32 sequenceNumber;
    UA_DateTime lastActivity;
    LIST_HEAD(UA_ListOfClientMonitoredItems, UA_Client_MonitoredItem) monitoredItems;
} UA_Client_Subscription;

void
UA_Client_Subscriptions_clean(UA_Client *client);

void
UA_Client_MonitoredItem_remove(UA_Client *client, UA_Client_Subscription *sub,
                               UA_Client_MonitoredItem *mon);

void
UA_Client_Subscriptions_processPublishResponse(UA_Client *client,
                                               UA_PublishRequest *request,
                                               UA_PublishResponse *response);

UA_StatusCode
UA_Client_preparePublishRequest(UA_Client *client, UA_PublishRequest *request);

UA_StatusCode
UA_Client_Subscriptions_backgroundPublish(UA_Client *client);

void
UA_Client_Subscriptions_backgroundPublishInactivityCheck(UA_Client *client);

#endif /* UA_ENABLE_SUBSCRIPTIONS */

/**************/
/* Encryption */
/**************/

UA_StatusCode
checkClientSignature(const UA_SecureChannel *channel, const UA_CreateSessionResponse *response);

UA_StatusCode
signActivateSessionRequest(UA_SecureChannel *channel,
                           UA_ActivateSessionRequest *request);
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

void UA_Client_AsyncService_cancel(UA_Client *client, AsyncServiceCall *ac,
                                   UA_StatusCode statusCode);

void UA_Client_AsyncService_removeAll(UA_Client *client, UA_StatusCode statusCode);

typedef struct CustomCallback {
    LIST_ENTRY(CustomCallback)
    pointers;
    //to find the correct callback
    UA_UInt32 callbackId;

    UA_ClientAsyncServiceCallback callback;

    UA_AttributeId attributeId;
    const UA_DataType *outDataType;
} CustomCallback;

typedef enum {
    UA_CHUNK_COMPLETED,
    UA_CHUNK_NOT_COMPLETED
} UA_ChunkState;

typedef enum {
    UA_CLIENTAUTHENTICATION_NONE,
    UA_CLIENTAUTHENTICATION_USERNAME
} UA_Client_Authentication;

struct UA_Client {
    /* State */
    UA_ClientState state;

    UA_ClientConfig config;
    UA_Timer timer;
    UA_StatusCode connectStatus;

    /* Connection */
    UA_Connection connection;
    UA_String endpointUrl;

    /* SecureChannel */
    UA_SecurityPolicy securityPolicy; /* TODO: Move supported policies to the config */
    UA_SecureChannel channel;
    UA_UInt32 requestId;
    UA_DateTime nextChannelRenewal;

    /* Authentication */
    UA_Client_Authentication authenticationMethod;
    UA_String username;
    UA_String password;

    /* Session */
    UA_UserTokenPolicy token;
    UA_NodeId authenticationToken;
    UA_UInt32 requestHandle;
    /* Connection Establishment (async) */
    UA_Connection_processChunk ackResponseCallback;
    UA_Connection_processChunk openSecureChannelResponseCallback;
    UA_Boolean endpointsHandshake;

    /* Async Service */
    AsyncServiceCall asyncConnectCall;
    LIST_HEAD(ListOfAsyncServiceCall, AsyncServiceCall) asyncServiceCalls;
    /*When using highlevel functions these are the callbacks that can be accessed by the user*/
    LIST_HEAD(ListOfCustomCallback, CustomCallback) customCallbacks;

    /* Delayed callbacks */
    SLIST_HEAD(DelayedClientCallbacksList, UA_DelayedClientCallback) delayedClientCallbacks;
    /* Subscriptions */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_UInt32 monitoredItemHandles;
    LIST_HEAD(ListOfUnacknowledgedNotifications, UA_Client_NotificationsAckNumber) pendingNotificationsAcks;
    LIST_HEAD(ListOfClientSubscriptionItems, UA_Client_Subscription) subscriptions;
    UA_UInt16 currentlyOutStandingPublishRequests;
#endif

    /* Connectivity check */
    UA_DateTime lastConnectivityCheck;
    UA_Boolean pendingConnectivityCheck;
};

void
setClientState(UA_Client *client, UA_ClientState state);

UA_StatusCode
UA_Client_connectInternal(UA_Client *client, const char *endpointUrl,
                          UA_Boolean endpointsHandshake, UA_Boolean createNewSession);

UA_StatusCode
UA_Client_connectInternalAsync(UA_Client *client, const char *endpointUrl,
                               UA_ClientAsyncServiceCallback callback,
                               void *connected, UA_Boolean endpointsHandshake,
                               UA_Boolean createNewSession);

UA_StatusCode
UA_Client_getEndpointsInternal(UA_Client *client,
                               size_t* endpointDescriptionsSize,
                               UA_EndpointDescription** endpointDescriptions);

/* Receive and process messages until a synchronous message arrives or the
 * timout finishes */
UA_StatusCode
receivePacketAsync(UA_Client *client);

UA_StatusCode
receiveServiceResponse(UA_Client *client, void *response,
                       const UA_DataType *responseType, UA_DateTime maxDate,
                       UA_UInt32 *synchronousRequestId);

UA_StatusCode
receiveServiceResponseAsync(UA_Client *client, void *response,
                             const UA_DataType *responseType);
void
UA_Client_workerCallback(UA_Client *client, UA_ClientCallback callback,
                         void *data);
UA_StatusCode
UA_Client_delayedCallback(UA_Client *client, UA_ClientCallback callback,
                          void *data);
UA_StatusCode
UA_Client_connect_iterate (UA_Client *client);
#endif /* UA_CLIENT_INTERNAL_H_ */
