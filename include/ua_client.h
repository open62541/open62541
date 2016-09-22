/* Copyright (C) 2014-2016 the contributors as stated in the AUTHORS file
 *
 * This file is part of open62541. open62541 is free software: you can
 * redistribute it and/or modify it under the terms of the GNU Lesser General
 * Public License, version 3 (as published by the Free Software Foundation) with
 * a static linking exception as stated in the LICENSE file provided with
 * open62541.
 *
 * open62541 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details. */

#ifndef UA_CLIENT_H_
#define UA_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_config.h"
#include "ua_types.h"
#include "ua_connection.h"
#include "ua_log.h"
#include "ua_types_generated.h"
#include "ua_types_generated_handling.h"

/**
 * .. _client:
 *
 * Client
 * ======
 *
 * The client implementation allows remote access to all OPC UA services. For
 * convenience, some functionality has been wrapped in :ref:`high-level
 * abstractions <client-highlevel>`.
 *
 * **However**: At this time, the client does not yet contain its own thread or
 * event-driven main-loop. So the client will not perform any actions
 * automatically in the background. This is especially relevant for
 * subscriptions. The user will have to periodically call
 * `UA_Client_Subscriptions_manuallySendPublishRequest`. See also :ref:`here
 * <client-subscriptions>`.
 *
 * Client Configuration
 * -------------------- */
typedef UA_Connection
(*UA_ConnectClientConnection)(UA_ConnectionConfig localConf,
                              const char *endpointUrl, UA_Logger logger);

typedef struct UA_ClientConfig {
    UA_UInt32 timeout;               /* Sync response timeout */
    UA_UInt32 secureChannelLifeTime; /* Lifetime in ms (then the channel needs
                                        to be renewed) */
    UA_Logger logger;
    UA_ConnectionConfig localConnectionConfig;
    UA_ConnectClientConnection connectionFunc;
} UA_ClientConfig;

/**
 * Client Lifecycle
 * ---------------- */
typedef enum {
     UA_CLIENTSTATE_READY,     /* The client is not connected but initialized
                                  and ready to use. */
     UA_CLIENTSTATE_CONNECTED, /* The client is connected to a server. */
     UA_CLIENTSTATE_FAULTED,   /* An error has occured that might have
                                  influenced the connection state. A successfull
                                  service call or renewal of the secure channel
                                  will reset the state to CONNECTED. */
     UA_CLIENTSTATE_ERRORED    /* A non-recoverable error has occured and the
                                  connection is no longer reliable. The client
                                  needs to be disconnected and reinitialized to
                                  recover into a CONNECTED state. */
} UA_ClientState;

struct UA_Client;
typedef struct UA_Client UA_Client;

/* Create a new client
 *
 * @param config for the new client. You can use UA_ClientConfig_standard
 *        which has sane defaults
 * @param logger function pointer to a logger function. See
 *        examples/logger_stdout.c for a simple implementation
 * @return return the new Client object */
UA_Client UA_EXPORT * UA_Client_new(UA_ClientConfig config);

/* Get the client connection status */
UA_ClientState UA_EXPORT UA_Client_getState(UA_Client *client);

/* Reset a client */
void UA_EXPORT UA_Client_reset(UA_Client *client);

/* Delete a client */
void UA_EXPORT UA_Client_delete(UA_Client *client);

/**
 * Manage the Connection
 * --------------------- */
/* Gets a list of endpoints of a server
 *
 * @param client to use
 * @param server url to connect (for example "opc.tcp://localhost:16664")
 * @param endpointDescriptionsSize size of the array of endpoint descriptions
 * @param endpointDescriptions array of endpoint descriptions that is allocated
 *        by the function (you need to free manually)
 * @return Indicates whether the operation succeeded or returns an error code */
UA_StatusCode UA_EXPORT
UA_Client_getEndpoints(UA_Client *client, const char *serverUrl,
                       size_t* endpointDescriptionsSize,
                       UA_EndpointDescription** endpointDescriptions);

/* Connect to the selected server
 *
 * @param client to use
 * @param endpointURL to connect (for example "opc.tcp://localhost:16664")
 * @return Indicates whether the operation succeeded or returns an error code */
UA_StatusCode UA_EXPORT
UA_Client_connect(UA_Client *client, const char *endpointUrl);

/* Connect to the selected server with the given username and password
 *
 * @param client to use
 * @param endpointURL to connect (for example "opc.tcp://localhost:16664")
 * @param username
 * @param password
 * @return Indicates whether the operation succeeded or returns an error code */
UA_StatusCode UA_EXPORT
UA_Client_connect_username(UA_Client *client, const char *endpointUrl,
                           const char *username, const char *password);

/* Close a connection to the selected server */
UA_StatusCode UA_EXPORT UA_Client_disconnect(UA_Client *client);

/* Renew the underlying secure channel */
UA_StatusCode UA_EXPORT UA_Client_manuallyRenewSecureChannel(UA_Client *client);

/**
 * .. _client-services:
 *
 * Raw Services
 * ------------
 * The raw OPC UA services are exposed to the client. But most of them time, it
 * is better to use the convenience functions from ``ua_client_highlevel.h``
 * that wrap the raw services. */
/* Don't use this function. Use the type versions below instead. */
void UA_EXPORT
__UA_Client_Service(UA_Client *client, const void *request,
                    const UA_DataType *requestType, void *response,
                    const UA_DataType *responseType);

/**
 * Attribute Service Set
 * ^^^^^^^^^^^^^^^^^^^^^ */
static UA_INLINE UA_ReadResponse
UA_Client_Service_read(UA_Client *client, const UA_ReadRequest request) {
    UA_ReadResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_READREQUEST],
                        &response, &UA_TYPES[UA_TYPES_READRESPONSE]);
    return response;
}

static UA_INLINE UA_WriteResponse
UA_Client_Service_write(UA_Client *client, const UA_WriteRequest request) {
    UA_WriteResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_WRITEREQUEST],
                        &response, &UA_TYPES[UA_TYPES_WRITERESPONSE]);
    return response;
}

/**
 * Method Service Set
 * ^^^^^^^^^^^^^^^^^^ */
#ifdef UA_ENABLE_METHODCALLS
static UA_INLINE UA_CallResponse
UA_Client_Service_call(UA_Client *client, const UA_CallRequest request) {
    UA_CallResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_CALLREQUEST],
                        &response, &UA_TYPES[UA_TYPES_CALLRESPONSE]);
    return response;
}
#endif

/**
 * NodeManagement Service Set
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^ */
static UA_INLINE UA_AddNodesResponse
UA_Client_Service_addNodes(UA_Client *client, const UA_AddNodesRequest request) {
    UA_AddNodesResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_ADDNODESREQUEST],
                        &response, &UA_TYPES[UA_TYPES_ADDNODESRESPONSE]);
    return response;
}

static UA_INLINE UA_AddReferencesResponse
UA_Client_Service_addReferences(UA_Client *client,
                                const UA_AddReferencesRequest request) {
    UA_AddReferencesResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_ADDNODESREQUEST],
                        &response, &UA_TYPES[UA_TYPES_ADDNODESRESPONSE]);
    return response;
}

static UA_INLINE UA_DeleteNodesResponse
UA_Client_Service_deleteNodes(UA_Client *client,
                              const UA_DeleteNodesRequest request) {
    UA_DeleteNodesResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_DELETENODESREQUEST],
                        &response, &UA_TYPES[UA_TYPES_DELETENODESRESPONSE]);
    return response;
}

static UA_INLINE UA_DeleteReferencesResponse
UA_Client_Service_deleteReferences(UA_Client *client,
                                   const UA_DeleteReferencesRequest request) {
    UA_DeleteReferencesResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_DELETENODESREQUEST],
                        &response, &UA_TYPES[UA_TYPES_DELETENODESRESPONSE]);
    return response;
}

/**
 * View Service Set
 * ^^^^^^^^^^^^^^^^ */
static UA_INLINE UA_BrowseResponse
UA_Client_Service_browse(UA_Client *client, const UA_BrowseRequest request) {
    UA_BrowseResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_BROWSEREQUEST],
                        &response, &UA_TYPES[UA_TYPES_BROWSERESPONSE]);
    return response;
}

static UA_INLINE UA_BrowseNextResponse
UA_Client_Service_browseNext(UA_Client *client,
                             const UA_BrowseNextRequest request) {
    UA_BrowseNextResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_BROWSENEXTREQUEST],
                        &response, &UA_TYPES[UA_TYPES_BROWSENEXTRESPONSE]);
    return response;
}

static UA_INLINE UA_TranslateBrowsePathsToNodeIdsResponse
UA_Client_Service_translateBrowsePathsToNodeIds(UA_Client *client,
                        const UA_TranslateBrowsePathsToNodeIdsRequest request) {
    UA_TranslateBrowsePathsToNodeIdsResponse response;
    __UA_Client_Service(client, &request,
                        &UA_TYPES[UA_TYPES_TRANSLATEBROWSEPATHSTONODEIDSREQUEST],
                        &response,
                        &UA_TYPES[UA_TYPES_TRANSLATEBROWSEPATHSTONODEIDSRESPONSE]);
    return response;
}

static UA_INLINE UA_RegisterNodesResponse
UA_Client_Service_registerNodes(UA_Client *client,
                                const UA_RegisterNodesRequest request) {
    UA_RegisterNodesResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_REGISTERNODESREQUEST],
                        &response, &UA_TYPES[UA_TYPES_REGISTERNODESRESPONSE]);
    return response;
}

static UA_INLINE UA_UnregisterNodesResponse
UA_Client_Service_unregisterNodes(UA_Client *client,
                                  const UA_UnregisterNodesRequest request) {
    UA_UnregisterNodesResponse response;
    __UA_Client_Service(client, &request,
                        &UA_TYPES[UA_TYPES_UNREGISTERNODESREQUEST],
                        &response, &UA_TYPES[UA_TYPES_UNREGISTERNODESRESPONSE]);
    return response;
}

/**
 * Query Service Set
 * ^^^^^^^^^^^^^^^^^ */
static UA_INLINE UA_QueryFirstResponse
UA_Client_Service_queryFirst(UA_Client *client,
                             const UA_QueryFirstRequest request) {
    UA_QueryFirstResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_QUERYFIRSTREQUEST],
                        &response, &UA_TYPES[UA_TYPES_QUERYFIRSTRESPONSE]);
    return response;
}

static UA_INLINE UA_QueryNextResponse
UA_Client_Service_queryNext(UA_Client *client,
                            const UA_QueryNextRequest request) {
    UA_QueryNextResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_QUERYFIRSTREQUEST],
                        &response, &UA_TYPES[UA_TYPES_QUERYFIRSTRESPONSE]);
    return response;
}

#ifdef UA_ENABLE_SUBSCRIPTIONS

/**
 * MonitoredItem Service Set
 * ^^^^^^^^^^^^^^^^^^^^^^^^^ */
static UA_INLINE UA_CreateMonitoredItemsResponse
UA_Client_Service_createMonitoredItems(UA_Client *client,
                                 const UA_CreateMonitoredItemsRequest request) {
    UA_CreateMonitoredItemsResponse response;
    __UA_Client_Service(client, &request,
                        &UA_TYPES[UA_TYPES_CREATEMONITOREDITEMSREQUEST], &response,
                        &UA_TYPES[UA_TYPES_CREATEMONITOREDITEMSRESPONSE]);
    return response;
}

static UA_INLINE UA_DeleteMonitoredItemsResponse
UA_Client_Service_deleteMonitoredItems(UA_Client *client,
                                 const UA_DeleteMonitoredItemsRequest request) {
    UA_DeleteMonitoredItemsResponse response;
    __UA_Client_Service(client, &request,
                        &UA_TYPES[UA_TYPES_DELETEMONITOREDITEMSREQUEST], &response,
                        &UA_TYPES[UA_TYPES_DELETEMONITOREDITEMSRESPONSE]);
    return response;
}

/**
 * Subscription Service Set
 * ^^^^^^^^^^^^^^^^^^^^^^^^ */
static UA_INLINE UA_CreateSubscriptionResponse
UA_Client_Service_createSubscription(UA_Client *client,
                                   const UA_CreateSubscriptionRequest request) {
    UA_CreateSubscriptionResponse response;
    __UA_Client_Service(client, &request,
                        &UA_TYPES[UA_TYPES_CREATESUBSCRIPTIONREQUEST], &response,
                        &UA_TYPES[UA_TYPES_CREATESUBSCRIPTIONRESPONSE]);
    return response;
}

static UA_INLINE UA_ModifySubscriptionResponse
UA_Client_Service_modifySubscription(UA_Client *client,
                                   const UA_ModifySubscriptionRequest request) {
    UA_ModifySubscriptionResponse response;
    __UA_Client_Service(client, &request,
                        &UA_TYPES[UA_TYPES_MODIFYSUBSCRIPTIONREQUEST], &response,
                        &UA_TYPES[UA_TYPES_MODIFYSUBSCRIPTIONRESPONSE]);
    return response;
}

static UA_INLINE UA_DeleteSubscriptionsResponse
UA_Client_Service_deleteSubscriptions(UA_Client *client,
                                  const UA_DeleteSubscriptionsRequest request) {
    UA_DeleteSubscriptionsResponse response;
    __UA_Client_Service(client, &request,
                        &UA_TYPES[UA_TYPES_DELETESUBSCRIPTIONSREQUEST], &response,
                        &UA_TYPES[UA_TYPES_DELETESUBSCRIPTIONSRESPONSE]);
    return response;
}

static UA_INLINE UA_PublishResponse
UA_Client_Service_publish(UA_Client *client, const UA_PublishRequest request) {
    UA_PublishResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_PUBLISHREQUEST],
                        &response, &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]);
    return response;
}

#endif

/**
 * .. toctree::
 *
 *    client_highlevel */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_CLIENT_H_ */
