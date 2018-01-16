/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef UA_CLIENT_INTERNAL_H_
#define UA_CLIENT_INTERNAL_H_

#include "ua_securechannel.h"
#include "queue.h"
#include "ua_timer.h"
/**************************/
/* Subscriptions Handling */
/**************************/

#ifdef UA_ENABLE_SUBSCRIPTIONS

typedef struct UA_Client_NotificationsAckNumber {
	LIST_ENTRY(UA_Client_NotificationsAckNumber)
	listEntry;
	UA_SubscriptionAcknowledgement subAck;
} UA_Client_NotificationsAckNumber;

typedef struct UA_Client_MonitoredItem {
	LIST_ENTRY(UA_Client_MonitoredItem)
	listEntry;
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
	void (*handlerEvents)(const UA_UInt32 monId, const size_t nEventFields,
			const UA_Variant *eventFields, void *context);
	void *handlerEventsContext;
} UA_Client_MonitoredItem;

typedef struct UA_Client_Subscription {
	LIST_ENTRY(UA_Client_Subscription)
	listEntry;
	UA_UInt32 lifeTime;
	UA_UInt32 keepAliveCount;
	UA_Double publishingInterval;
	UA_UInt32 subscriptionID;
	UA_UInt32 notificationsPerPublish;
	UA_UInt32 priority;LIST_HEAD(UA_ListOfClientMonitoredItems, UA_Client_MonitoredItem) monitoredItems;
} UA_Client_Subscription;

void UA_Client_Subscriptions_forceDelete(UA_Client *client,
		UA_Client_Subscription *sub);

#endif

/**********/
/* Client */
/**********/

typedef struct AsyncServiceCall {
	LIST_ENTRY(AsyncServiceCall)
	pointers;
	UA_UInt32 requestId;
	UA_ClientAsyncServiceCallback callback;
	const UA_DataType *responseType;
	void *userdata;
	void *responsedata;
} AsyncServiceCall;

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
	UA_CHUNK_COMPLETED, UA_CHUNK_NOT_COMPLETED
} UA_ChunkState;

typedef enum {
	UA_CLIENTAUTHENTICATION_NONE, UA_CLIENTAUTHENTICATION_USERNAME
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
    UA_SecurityPolicy securityPolicy;
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
	SLIST_HEAD(DelayedCallbacksList, UA_DelayedCallback) delayedCallbacks;
    /* Subscriptions */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_UInt32 monitoredItemHandles;
    LIST_HEAD(ListOfUnacknowledgedNotifications, UA_Client_NotificationsAckNumber) pendingNotificationsAcks;
    LIST_HEAD(ListOfClientSubscriptionItems, UA_Client_Subscription) subscriptions;
    UA_UInt16 currentlyOutStandingPublishRequests;
#endif
};

UA_StatusCode
UA_Client_connectInternal(UA_Client *client, const char *endpointUrl,
		UA_Boolean endpointsHandshake, UA_Boolean createNewSession);

UA_StatusCode
UA_Client_connectInternalAsync(UA_Client *client, const char *endpointUrl,
		UA_ClientAsyncServiceCallback callback, void *userdata,
		UA_Boolean endpointsHandshake, UA_Boolean createNewSession);

UA_StatusCode
UA_Client_getEndpointsInternal(UA_Client *client,
		size_t* endpointDescriptionsSize,
		UA_EndpointDescription** endpointDescriptions);

UA_StatusCode receivePacket_async(UA_Client *client);

UA_StatusCode
receiveServiceResponse(UA_Client *client, void *response,
		const UA_DataType *responseType, UA_DateTime maxDate,
		UA_UInt32 *synchronousRequestId);

UA_StatusCode
receiveServiceResponse_async(UA_Client *client, void *response,
		const UA_DataType *responseType);
void
UA_Client_workerCallback(UA_Client *client, UA_ClientCallback callback,
		void *data);
UA_StatusCode
UA_Client_delayedCallback(UA_Client *client, UA_ClientCallback callback,
		void *data);
UA_StatusCode UA_Client_connect_iterate(UA_Client *client);
#endif /* UA_CLIENT_INTERNAL_H_ */
