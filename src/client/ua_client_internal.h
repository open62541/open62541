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
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#ifndef UA_CLIENT_INTERNAL_H_
#define UA_CLIENT_INTERNAL_H_

#define UA_INTERNAL
#include <open62541/client.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_subscriptions.h>

#include "../ua_securechannel.h"
#include "../util/ua_util_internal.h"
#include "open62541_queue.h"
#include "ziptree.h"

_UA_BEGIN_DECLS

/**************************/
/* Subscriptions Handling */
/**************************/

typedef struct UA_Client_NotificationsAckNumber {
    LIST_ENTRY(UA_Client_NotificationsAckNumber) listEntry;
    UA_SubscriptionAcknowledgement subAck;
} UA_Client_NotificationsAckNumber;

typedef struct UA_Client_MonitoredItem {
    ZIP_ENTRY(UA_Client_MonitoredItem) zipfields;
    UA_UInt32 monitoredItemId;
    UA_MonitoringParameters parameters;
    UA_MonitoringParameters pendingParameters;
    void *context;
    UA_Client_DeleteMonitoredItemCallback deleteCallback;
    union {
        UA_Client_DataChangeNotificationCallback dataChangeCallback;
        UA_Client_EventNotificationCallback eventCallback;
    } handler;
    UA_Boolean isEventMonitoredItem; /* Otherwise a DataChange MoniitoredItem */
    UA_KeyValueMap eventFields; /* Lazily cached names for the active filter */
} UA_Client_MonitoredItem;

ZIP_HEAD(MonitorItemsTree, UA_Client_MonitoredItem);
typedef struct MonitorItemsTree MonitorItemsTree;

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
    MonitorItemsTree monitoredItems;
    size_t pendingRekeys; /* MonitoredItems with a pending clientHandle
                           * re-key (pendingParameters.clientHandle != 0).
                           * Gates the O(n) fallback scan in the
                           * notification dispatch. */
} UA_Client_Subscription;

void
__Client_Subscriptions_clear(UA_Client *client);

/* Exposed for fuzzing */
UA_StatusCode
__Client_preparePublishRequest(UA_Client *client, UA_PublishRequest *request);

void
__Client_Subscriptions_backgroundPublish(UA_Client *client);

void
__Client_Subscriptions_backgroundPublishInactivityCheck(UA_Client *client);

/* Exposed for unit tests and fuzzing of notification ordering */
void
__Client_Subscriptions_processPublishResponse(UA_Client *client,
                                              UA_PublishRequest *request,
                                              UA_PublishResponse *response);

/**********/
/* Client */
/**********/

typedef union {
    UA_ClientAsyncCallCallback call;
    UA_ClientAsyncAddNodesCallback addNodes;
    UA_ClientAsyncReadCallback read;
    UA_ClientAsyncWriteCallback write;
    UA_ClientAsyncBrowseCallback browse;
    UA_ClientAsyncBrowseNextCallback browseNext;
    UA_ClientAsyncSetMonitoringModeCallback setMonitoringMode;
    UA_ClientAsyncSetTriggeringCallback setTriggering;
    UA_ClientAsyncReadAttributeCallback dataValue;
    UA_ClientAsyncReadDataTypeAttributeCallback nodeId;
    UA_ClientReadArrayDimensionsAttributeCallback variant;
    UA_ClientAsyncReadNodeClassAttributeCallback nodeClass;
    UA_ClientAsyncReadBrowseNameAttributeCallback qualifiedName;
    UA_ClientAsyncReadDisplayNameAttributeCallback localizedText;
    UA_ClientAsyncReadWriteMaskAttributeCallback uint32;
    UA_ClientAsyncReadAccessRestrictionsAttributeCallback uint16;
    UA_ClientAsyncReadIsAbstractAttributeCallback boolean;
    UA_ClientAsyncReadEventNotifierAttributeCallback byte;
    UA_ClientAsyncReadValueRankAttributeCallback int32;
    UA_ClientAsyncReadMinimumSamplingIntervalAttributeCallback doubleValue;
} UA_AsyncCallback;

typedef struct {
    UA_AsyncCallback callback;
    void *userdata;
    const UA_DataType *resultType;
    UA_UInt32 attributeId;
} UA_AsyncCallbackContext;

typedef struct AsyncServiceCall {
    LIST_ENTRY(AsyncServiceCall) pointers;
    UA_UInt32 requestId;     /* Unique id */
    UA_UInt32 requestHandle; /* Potentially non-unique if manually defined in
                              * the request header*/
    UA_ClientAsyncServiceCallback callback;
    const UA_DataType *responseType;
    void *userdata;
    UA_DateTime start;
    UA_UInt32 timeout;
    UA_Boolean applicationCall; /* Counts towards maxAsyncServiceCalls */
    UA_Response *syncResponse; /* If non-null, then this is the synchronous
                                * response to be filled. Set back to null to
                                * indicate that the response was filled. */
    UA_AsyncCallbackContext context;

    /* Direct HTTP responses arrive as carrier metadata and body fragments.
     * Keep their assembly state on the canonical service-call record. */
    UA_UInt16 httpStatusCode;
    UA_ByteString httpResponseBody;
} AsyncServiceCall;

typedef LIST_HEAD(UA_AsyncServiceList, AsyncServiceCall) UA_AsyncServiceList;

void
__Client_AsyncService_removeAll(UA_Client *client, UA_StatusCode statusCode);

void
__Client_AsyncService_fail(UA_Client *client, UA_UInt32 requestId,
                           UA_StatusCode statusCode);

AsyncServiceCall *
__Client_AsyncService_find(UA_Client *client, UA_UInt32 requestId);

UA_UInt32
__Client_nextRequestId(UA_Client *client);

/* A known RequestId is consumed exactly once, including when decoding fails.
 * An unknown or already completed RequestId is ignored. */
UA_StatusCode
__Client_processServiceResponsePayload(UA_Client *client, UA_UInt32 requestId,
                                       const UA_ByteString *message,
                                       UA_SecureChannelEncoding encoding);

typedef struct CustomCallback {
    UA_UInt32 callbackId;

    union {
        UA_ClientAsyncCreateSubscriptionCallback createSubscription;
        UA_ClientAsyncModifySubscriptionCallback modifySubscription;
        UA_ClientAsyncCreateMonitoredItemsCallback createMonitoredItems;
        UA_ClientAsyncModifyMonitoredItemsCallback modifyMonitoredItems;
        UA_ClientAsyncDeleteMonitoredItemsCallback deleteMonitoredItems;
    } callback;
    void *userData;

    void *clientData;
} CustomCallback;

struct UA_Client {
    UA_ClientConfig config;

    /* Callback ID to remove it from the EventLoop */
    UA_UInt64 houseKeepingCallbackId;

    /* Overall connection status */
    UA_StatusCode connectStatus;

    /* Old status to notify only changes */
    UA_SecureChannelState oldChannelState;
    UA_SessionState oldSessionState;
    UA_StatusCode oldConnectStatus;

    UA_Boolean findServersHandshake;   /* Ongoing FindServers */
    UA_Boolean endpointsHandshake;     /* Ongoing GetEndpoints */
    UA_Boolean namespacesHandshake;    /* Ongoing Namespaces read */
    UA_Boolean haveNamespaces;         /* Do we have the namespaces? */

    /* The discoveryUrl can be different from the EndpointUrl in the client
     * configuration. The EndpointUrl is used to connect initially, then the
     * DiscoveryUrl is selected via FindServers. This triggers a reconnect if
     * EndpointUrl != DiscoveryUrl. */
    UA_String discoveryUrl;

    /* Contains the Server description, etc. */
    UA_EndpointDescription endpoint;

    UA_RuleHandling allowAllCertificateUris;

    /* SecureChannel */
    UA_SecureChannel channel;
    UA_UInt32 requestId; /* Unique, internally defined for each request */
    UA_DateTime nextChannelRenewal;
    UA_UInt32 httpChannelGeneration;

    /* Reverse connect (listen) connections */
    UA_ConnectionManager *reverseConnectionCM;
    uintptr_t reverseConnectionIds[16];

    /* Session */
    UA_NodeId sessionId;
    UA_SessionState sessionState;
    UA_NodeId authenticationToken;
    UA_UInt32 requestHandle; /* Unique handles >100,000 are generated if the
                              * request header contains a zero-handle. */
    UA_ByteString serverSessionNonce;
    UA_ByteString clientSessionNonce;

    /* The SecurityPolicy corresponding to the UserTokenPolicy. Either for
     * encrypting the password (secret) or for signing with the private key of
     * the certificate in the UserIdentityToken. */
    UA_SecurityPolicy *utpSp;
    void *utpSpContext;

    /* For authentication with an ECC SecurityPolicy. This is needed to save the
     * server's ephemeral public key between the session creation (when the key
     * is received) and session activation, when the key is used. */
    UA_ByteString serverEphemeralPubKey;

    /* Connectivity check */
    UA_DateTime lastConnectivityCheck;
    UA_Boolean pendingConnectivityCheck;

    /* Async Service */
    UA_AsyncServiceList asyncServiceCalls;
    size_t outstandingAsyncServiceCalls;

    /* Subscriptions */
    LIST_HEAD(, UA_Client_NotificationsAckNumber) pendingNotificationsAcks;
    LIST_HEAD(, UA_Client_Subscription) subscriptions;
    UA_UInt32 monitoredItemHandles;
    UA_UInt16 currentlyOutStandingPublishRequests;

    /* Internal namespaces. The table maps the namespace Uri to its index. This
     * is used for the automatic namespace mapping in de/encoding. */
    UA_String *namespaces;
    size_t namespacesSize;

    /* Internal locking for thread-safety. Methods starting with UA_Client_ that
     * are marked with UA_THREADSAFE take the lock. The lock is released before
     * dropping into the EventLoop and before calling user-defined callbacks.
     * That way user-defined callbacks can themselves call thread-safe client
     * methods. */
#if UA_MULTITHREADING >= 100
    UA_Lock clientMutex;
#endif
};

UA_StatusCode
verifyServerCertificateEku(const UA_ClientConfig *config,
                           const UA_SecurityPolicy *securityPolicy,
                           const UA_ByteString *certificate);

/* In order to prevent deadlocks between the EventLoop mutex and the
 * client-mutex, we always take the EventLoop mutex first. */

void lockClient(UA_Client *client);
void unlockClient(UA_Client *client);

UA_StatusCode
__Client_AsyncService(UA_Client *client, const void *request,
                      const UA_DataType *requestType,
                      UA_ClientAsyncServiceCallback callback,
                      const UA_DataType *responseType,
                      void *userdata, UA_UInt32 *requestId);

/* Wait for application-level async service capacity before preparing local
 * client state. The client lock must be held until the admitted call is sent. */
UA_StatusCode
__Client_AsyncServiceAdmission(UA_Client *client);

UA_StatusCode
__Client_AsyncServiceAdmitted(UA_Client *client, const void *request,
                              const UA_DataType *requestType,
                              UA_ClientAsyncServiceCallback callback,
                              const UA_DataType *responseType,
                              void *userdata, UA_UInt32 *requestId);

/* Async service calls required for client-internal progress. These bypass the
 * application-level maxAsyncServiceCalls limit. */
UA_StatusCode
__Client_AsyncServiceInternal(UA_Client *client, const void *request,
                              const UA_DataType *requestType,
                              UA_ClientAsyncServiceCallback callback,
                              const UA_DataType *responseType,
                              void *userdata, UA_UInt32 *requestId);

UA_StatusCode
__Client_AsyncServiceWithContext(UA_Client *client, const void *request,
                                 const UA_DataType *requestType,
                                 UA_ClientAsyncServiceCallback callback,
                                 const UA_DataType *responseType,
                                 void *userdata,
                                 const UA_AsyncCallbackContext *context,
                                 UA_UInt32 *requestId);

UA_StatusCode
__Client_AsyncServiceWithContextAdmitted(
    UA_Client *client, const void *request, const UA_DataType *requestType,
    UA_ClientAsyncServiceCallback callback, const UA_DataType *responseType,
    void *userdata, const UA_AsyncCallbackContext *context,
    UA_UInt32 *requestId);

void
__Client_Service(UA_Client *client, const void *request,
                 const UA_DataType *requestType, void *response,
                 const UA_DataType *responseType);

UA_StatusCode
__UA_Client_startup(UA_Client *client);

/* Connect with the client configuration. For the async connection, finish
 * connecting via UA_Client_run_iterate (or manually running a configured
 * external EventLoop). */
UA_StatusCode
__UA_Client_connect(UA_Client *client, UA_Boolean async, const char *endpointUrl);

void
__UA_Client_Service(UA_Client *client, const void *request,
                    const UA_DataType *requestType, void *response,
                    const UA_DataType *responseType);

UA_StatusCode
__Client_renewSecureChannel(UA_Client *client);

UA_StatusCode
processServiceResponse(UA_Client *client, UA_SecureChannel *channel,
                       UA_MessageType messageType, UA_UInt32 requestId,
                       UA_ByteString *message);

UA_StatusCode connectInternal(UA_Client *client, UA_Boolean async);
UA_StatusCode connectSecureChannel(UA_Client *client, const char *endpointUrl);
UA_Boolean isFullyConnected(UA_Client *client);
void connectSync(UA_Client *client);
void connectActivity(UA_Client *client);
void setConnectStatus(UA_Client *client, UA_StatusCode status);
void notifyClientState(UA_Client *client);
void processRHEMessage(UA_Client *client, const UA_ByteString *chunk);
void processERRResponse(UA_Client *client, const UA_ByteString *chunk);
void processACKResponse(UA_Client *client, const UA_ByteString *chunk);
void processOPNResponse(UA_Client *client, const UA_ByteString *message);
void closeSecureChannel(UA_Client *client);
void cleanupSession(UA_Client *client);
UA_StatusCode __Client_validateHttpConnection(UA_Client *client,
                                              UA_Boolean useTls);
UA_StatusCode __Client_openHttpConnection(
    UA_Client *client, UA_ConnectionManager *cm, const UA_String *hostname,
    UA_UInt16 port, const UA_String *requestPath, UA_Boolean useTls,
    UA_SecureChannelEncoding encoding);
void __Client_httpConnectionCallback(
    UA_ConnectionManager *cm, uintptr_t connectionId, void *application,
    void **connectionContext, UA_ConnectionState state,
    const UA_KeyValueMap *params, UA_ByteString msg);

void
Client_warnEndpointsResult(UA_Client *client,
                           const UA_GetEndpointsResponse *response,
                           const UA_String *endpointUrl);

_UA_END_DECLS

#endif /* UA_CLIENT_INTERNAL_H_ */
