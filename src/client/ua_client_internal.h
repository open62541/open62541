/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef UA_CLIENT_INTERNAL_H_
#define UA_CLIENT_INTERNAL_H_

#include "ua_securechannel.h"
#include "queue.h"
#include "ua_timer.h"
#include "ua_client.h"
/**************************/
/* Subscriptions Handling */
/**************************/



/**********/
/* Client */
/**********/
typedef enum ConnectState{
	NO_ACK,
	HEL_ACK,
	SECURECHANNEL_ACK,
	ENDPOINTS_ACK,
	SESSION_ACK,
	ACTIVATE_ACK
}ConnectState;


typedef struct AsyncServiceCall {
    LIST_ENTRY(AsyncServiceCall) pointers;
    UA_UInt32 requestId;
    UA_ClientAsyncServiceCallback callback;
    const UA_DataType *responseType;
    void *userdata;
} AsyncServiceCall;

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
	ConnectState connectState;
	ConnectState lastConnectState;
    UA_ClientState state;
    UA_ClientConfig config;

    /* Connection */
    UA_Connection connection;
    UA_String endpointUrl;

    /* chunking */
    UA_ByteString reply;
    UA_Boolean realloced;
    UA_Int32 chunkState;

    /* SecureChannel */
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

    /* Async Service */
    LIST_HEAD(ListOfAsyncServiceCall, AsyncServiceCall) asyncServiceCalls;
    

    /* Callbacks with a repetition interval */
    UA_Timer timer;

    /* Delayed callbacks */
    SLIST_HEAD(DelayedCallbacksList, UA_DelayedCallback) delayedCallbacks;

    /* Subscriptions */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_UInt32 monitoredItemHandles;
    LIST_HEAD(ListOfUnacknowledgedNotifications, UA_Client_NotificationsAckNumber) pendingNotificationsAcks;
    LIST_HEAD(ListOfClientSubscriptionItems, UA_Client_Subscription) subscriptions;
#endif
};

/* Connect to the selected server.
 * This will not create a session.
 *
 * @param client to use
 * @param endpointURL to connect (for example "opc.tcp://localhost:16664")
 * @return Indicates whether the operation succeeded or returns an error code */
UA_StatusCode UA_EXPORT
UA_Client_connect_no_session(UA_Client *client, const char *endpointUrl);

UA_StatusCode
__UA_Client_connect(UA_Client *client, const char *endpointUrl,
                    UA_Boolean endpointsHandshake, UA_Boolean createSession, ConnectState *last_cs,UA_Boolean *waiting, UA_Boolean *connected);

UA_StatusCode
__UA_Client_getEndpoints(UA_Client *client, size_t* endpointDescriptionsSize,
                         UA_EndpointDescription** endpointDescriptions);

#ifdef UA_ENABLE_SUBSCRIPTIONS

typedef struct UA_Client_NotificationsAckNumber {
    LIST_ENTRY(UA_Client_NotificationsAckNumber) listEntry;
    UA_SubscriptionAcknowledgement subAck;
} UA_Client_NotificationsAckNumber;

typedef struct UA_Client_MonitoredItem {
    LIST_ENTRY(UA_Client_MonitoredItem)  listEntry;
    UA_UInt32 monitoredItemId;
    UA_UInt32 monitoringMode;
    UA_NodeId monitoredNodeId;
    UA_UInt32 attributeID;
    UA_UInt32 clientHandle;
    UA_Double samplingInterval;
    UA_UInt32 queueSize;
    UA_Boolean discardOldest;
    void (*handler)(UA_UInt32 monId, UA_DataValue *value, void *context);
    void *handlerContext;
} UA_Client_MonitoredItem;

typedef struct UA_Client_Subscription {
    LIST_ENTRY(UA_Client_Subscription) listEntry;
    UA_UInt32 lifeTime;
    UA_UInt32 keepAliveCount;
    UA_Double publishingInterval;
    UA_UInt32 subscriptionID;
    UA_UInt32 notificationsPerPublish;
    UA_UInt32 priority;
    LIST_HEAD(UA_ListOfClientMonitoredItems, UA_Client_MonitoredItem) monitoredItems;
} UA_Client_Subscription;

void UA_Client_Subscriptions_forceDelete(UA_Client *client, UA_Client_Subscription *sub);

UA_StatusCode
receiveServiceResponse_async(UA_Client *client, void *response,
                       const UA_DataType *responseType);
void
UA_Client_workerCallback(UA_Client *client, UA_ClientCallback callback,
                         void *data);
UA_StatusCode
UA_Client_delayedCallback(UA_Client *client, UA_ClientCallback callback,
                          void *data);
#endif

#endif /* UA_CLIENT_INTERNAL_H_ */
