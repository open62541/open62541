/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2015-2020 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2015-2016 (c) Chris Iatrou
 *    Copyright 2015-2017 (c) Florian Palm
 *    Copyright 2015 (c) Holger Jeromin
 *    Copyright 2015 (c) Oleksiy Vasylyev
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2018 (c) Thomas Stalder, Blue Time Concept SA
 *    Copyright 2018 (c) Kalycito Infotech Private Limited
 *    Copyright 2020 (c) Christian von Arnim, ISW University of Stuttgart
 */

#ifndef UA_CLIENT_H_
#define UA_CLIENT_H_

#include <open62541/config.h>
#include <open62541/nodeids.h>
#include <open62541/types.h>
#include <open62541/types_generated.h>
#include <open62541/types_generated_handling.h>

#include <open62541/plugin/log.h>
#include <open62541/plugin/network.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/plugin/eventloop.h>

_UA_BEGIN_DECLS

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
 * event-driven main-loop, meaning that the client will not perform any actions
 * automatically in the background. This is especially relevant for
 * connection/session management and subscriptions. The user will have to
 * periodically call `UA_Client_run_iterate` to ensure that asynchronous events
 * are handled, including keeping a secure connection established.
 * See more about :ref:`asynchronicity<client-async-services>` and
 * :ref:`subscriptions<client-subscriptions>`.
 *
 * .. _client-config:
 *
 * Client Configuration
 * --------------------
 *
 * The client configuration is used for setting connection parameters and
 * additional settings used by the client.
 * The configuration should not be modified after it is passed to a client.
 * Currently, only one client can use a configuration at a time.
 *
 * Examples for configurations are provided in the ``/plugins`` folder.
 * The usual usage is as follows:
 *
 * 1. Create a client configuration with default settings as a starting point
 * 2. Modifiy the configuration, e.g. modifying the timeout
 * 3. Instantiate a client with it
 * 4. After shutdown of the client, clean up the configuration (free memory)
 *
 * The :ref:`tutorials` provide a good starting point for this. */

typedef struct {
    void *clientContext; /* User-defined pointer attached to the client */
    UA_Logger logger;    /* Logger used by the client */
    UA_UInt32 timeout;   /* Response timeout in ms */

    /* The description must be internally consistent.
     * - The ApplicationUri set in the ApplicationDescription must match the
     *   URI set in the certificate */
    UA_ApplicationDescription clientDescription;

    /**
     * Connection configuration
     * ~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * The following configuration elements reduce the "degrees of freedom" the
     * client has when connecting to a server. If no connection can be made
     * under these restrictions, then the connection will abort with an error
     * message. */
    UA_ExtensionObject userIdentityToken; /* Configured User-Identity Token */
    UA_MessageSecurityMode securityMode;  /* None, Sign, SignAndEncrypt. The
                                           * default is invalid. This indicates
                                           * the client to select any matching
                                           * endpoint. */
    UA_String securityPolicyUri; /* SecurityPolicy for the SecureChannel. An
                                  * empty string indicates the client to select
                                  * any matching SecurityPolicy. */

    /**
     * If either endpoint or userTokenPolicy has been set (at least one non-zero
     * byte in either structure), then the selected Endpoint and UserTokenPolicy
     * overwrite the settings in the basic connection configuration. The
     * userTokenPolicy array in the EndpointDescription is ignored. The selected
     * userTokenPolicy is set in the dedicated configuration field.
     *
     * If the advanced configuration is not set, the client will write to it the
     * selected Endpoint and UserTokenPolicy during GetEndpoints.
     *
     * The information in the advanced configuration is used during reconnect
     * when the SecureChannel was broken. */
    UA_EndpointDescription endpoint;
    UA_UserTokenPolicy userTokenPolicy;

    /**
     * Custom Data Types
     * ~~~~~~~~~~~~~~~~~
     * The following is a linked list of arrays with custom data types. All data
     * types that are accessible from here are automatically considered for the
     * decoding of received messages. Custom data types are not cleaned up
     * together with the configuration. So it is possible to allocate them on
     * ROM.
     *
     * See the section on :ref:`generic-types`. Examples for working with custom
     * data types are provided in ``/examples/custom_datatype/``. */
    const UA_DataTypeArray *customDataTypes;

    /**
     * Advanced Client Configuration
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

    UA_UInt32 secureChannelLifeTime; /* Lifetime in ms (then the channel needs
                                        to be renewed) */
    UA_UInt32 requestedSessionTimeout; /* Session timeout in ms */
    UA_ConnectionConfig localConnectionConfig;
    UA_UInt32 connectivityCheckInterval;     /* Connectivity check interval in ms.
                                              * 0 = background task disabled */

    /* EventLoop */
    UA_EventLoop *eventLoop;
    UA_Boolean externalEventLoop; /* The EventLoop is not deleted with the config */

    /* Available SecurityPolicies */
    size_t securityPoliciesSize;
    UA_SecurityPolicy *securityPolicies;

    /* Certificate Verification Plugin */
    UA_CertificateVerification certificateVerification;

    /* Callbacks for async connection handshakes */
    UA_ConnectClientConnection initConnectionFunc;
    UA_StatusCode (*pollConnectionFunc)(UA_Connection *connection,
                                        UA_UInt32 timeout,
                                        const UA_Logger *logger);

    /* Callback for state changes. The client state is differentated into the
     * SecureChannel state and the Session state. The connectStatus is set if
     * the client connection (including reconnects) has failed and the client
     * has to "give up". If the connectStatus is not set, the client still has
     * hope to connect or recover. */
    void (*stateCallback)(UA_Client *client,
                          UA_SecureChannelState channelState,
                          UA_SessionState sessionState,
                          UA_StatusCode connectStatus);

    /* When connectivityCheckInterval is greater than 0, every
     * connectivityCheckInterval (in ms), an async read request is performed on
     * the server. inactivityCallback is called when the client receive no
     * response for this read request The connection can be closed, this in an
     * attempt to recreate a healthy connection. */
    void (*inactivityCallback)(UA_Client *client);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* Number of PublishResponse queued up in the server */
    UA_UInt16 outStandingPublishRequests;

    /* If the client does not receive a PublishResponse after the defined delay
     * of ``(sub->publishingInterval * sub->maxKeepAliveCount) +
     * client->config.timeout)``, then subscriptionInactivityCallback is called
     * for the subscription.. */
    void (*subscriptionInactivityCallback)(UA_Client *client,
                                           UA_UInt32 subscriptionId,
                                           void *subContext);
#endif

    UA_LocaleId *sessionLocaleIds;
    size_t sessionLocaleIdsSize;
} UA_ClientConfig;

 /**
 * Client Lifecycle
 * ---------------- */

/* The method UA_Client_new is defined in client_config_default.h. So default
 * plugins outside of the core library (for logging, etc) are already available
 * during the initialization.
 *
 * UA_Client UA_EXPORT * UA_Client_new(void);
 */

/* Creates a new client. Moves the config into the client with a shallow copy.
 * The config content is cleared together with the client. */
UA_Client UA_EXPORT *
UA_Client_newWithConfig(const UA_ClientConfig *config);

/* Returns the current state. All arguments except ``client`` can be NULL. */
void UA_EXPORT
UA_Client_getState(UA_Client *client,
                   UA_SecureChannelState *channelState,
                   UA_SessionState *sessionState,
                   UA_StatusCode *connectStatus);

/* Get the client configuration */
UA_EXPORT UA_ClientConfig *
UA_Client_getConfig(UA_Client *client);

/* Get the client context */
static UA_INLINE void *
UA_Client_getContext(UA_Client *client) {
    return UA_Client_getConfig(client)->clientContext; /* Cannot fail */
}

/* (Disconnect and) delete the client */
void UA_EXPORT
UA_Client_delete(UA_Client *client);

/**
 * Connect to a Server
 * -------------------
 *
 * Once a client is connected to an endpointUrl, it is not possible to switch to
 * another server. A new client has to be created for that.
 *
 * Once a connection is established, the client keeps the connection open and
 * reconnects if necessary.
 *
 * If the connection fails unrecoverably (state->connectStatus is set to an
 * error), the client is no longer usable. Create a new client if required. */

/* Connect to the server. First a SecureChannel is opened, then a Session. The
 * client configuration restricts the SecureChannel selection and contains the
 * UserIdentityToken for the Session.
 *
 * @param client to use
 * @param endpointURL to connect (for example "opc.tcp://localhost:4840")
 * @return Indicates whether the operation succeeded or returns an error code */
UA_StatusCode UA_EXPORT
UA_Client_connect(UA_Client *client, const char *endpointUrl);

/* Connect async (non-blocking) to the server. After initiating the connection,
 * call UA_Client_run_iterate repeatedly until the connection is fully
 * established. You can set a callback to client->config.stateCallback to be
 * notified when the connection status changes. Or use UA_Client_getState to get
 * the state manually. */
UA_StatusCode UA_EXPORT
UA_Client_connectAsync(UA_Client *client, const char *endpointUrl);

/* Connect to the server without creating a session
 *
 * @param client to use
 * @param endpointURL to connect (for example "opc.tcp://localhost:4840")
 * @return Indicates whether the operation succeeded or returns an error code */
UA_StatusCode UA_EXPORT
UA_Client_connectSecureChannel(UA_Client *client, const char *endpointUrl);

/* Connect async (non-blocking) only the SecureChannel */
UA_StatusCode UA_EXPORT
UA_Client_connectSecureChannelAsync(UA_Client *client, const char *endpointUrl);

/* Connect to the server and create+activate a Session with the given username
 * and password. This first set the UserIdentityToken in the client config and
 * then calls the regular connect method. */
static UA_INLINE UA_StatusCode
UA_Client_connectUsername(UA_Client *client, const char *endpointUrl,
                          const char *username, const char *password) {
    UA_UserNameIdentityToken* identityToken = UA_UserNameIdentityToken_new();
    if(!identityToken)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    identityToken->userName = UA_STRING_ALLOC(username);
    identityToken->password = UA_STRING_ALLOC(password);
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ExtensionObject_clear(&cc->userIdentityToken);
    cc->userIdentityToken.encoding = UA_EXTENSIONOBJECT_DECODED;
    cc->userIdentityToken.content.decoded.type = &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN];
    cc->userIdentityToken.content.decoded.data = identityToken;
    return UA_Client_connect(client, endpointUrl);
}

/* Disconnect and close a connection to the selected server. Disconnection is
 * always performed async (without blocking). */
UA_StatusCode UA_EXPORT
UA_Client_disconnect(UA_Client *client);

/* Disconnect async. Run UA_Client_run_iterate until the callback notifies that
 * all connections are closed. */
UA_StatusCode UA_EXPORT
UA_Client_disconnectAsync(UA_Client *client);

/* Disconnect the SecureChannel but keep the Session intact (if it exists).
 * This is always an async (non-blocking) operation. */
UA_StatusCode UA_EXPORT
UA_Client_disconnectSecureChannel(UA_Client *client);

/**
 * Discovery
 * --------- */

/* Gets a list of endpoints of a server
 *
 * @param client to use. Must be connected to the same endpoint given in
 *        serverUrl or otherwise in disconnected state.
 * @param serverUrl url to connect (for example "opc.tcp://localhost:4840")
 * @param endpointDescriptionsSize size of the array of endpoint descriptions
 * @param endpointDescriptions array of endpoint descriptions that is allocated
 *        by the function (you need to free manually)
 * @return Indicates whether the operation succeeded or returns an error code */
UA_StatusCode UA_EXPORT
UA_Client_getEndpoints(UA_Client *client, const char *serverUrl,
                       size_t* endpointDescriptionsSize,
                       UA_EndpointDescription** endpointDescriptions);

/* Gets a list of all registered servers at the given server.
 *
 * You can pass an optional filter for serverUris. If the given server is not registered,
 * an empty array will be returned. If the server is registered, only that application
 * description will be returned.
 *
 * Additionally you can optionally indicate which locale you want for the server name
 * in the returned application description. The array indicates the order of preference.
 * A server may have localized names.
 *
 * @param client to use. Must be connected to the same endpoint given in
 *        serverUrl or otherwise in disconnected state.
 * @param serverUrl url to connect (for example "opc.tcp://localhost:4840")
 * @param serverUrisSize Optional filter for specific server uris
 * @param serverUris Optional filter for specific server uris
 * @param localeIdsSize Optional indication which locale you prefer
 * @param localeIds Optional indication which locale you prefer
 * @param registeredServersSize size of returned array, i.e., number of found/registered servers
 * @param registeredServers array containing found/registered servers
 * @return Indicates whether the operation succeeded or returns an error code */
UA_StatusCode UA_EXPORT
UA_Client_findServers(UA_Client *client, const char *serverUrl,
                      size_t serverUrisSize, UA_String *serverUris,
                      size_t localeIdsSize, UA_String *localeIds,
                      size_t *registeredServersSize,
                      UA_ApplicationDescription **registeredServers);

#ifdef UA_ENABLE_DISCOVERY
/* Get a list of all known server in the network. Only supported by LDS servers.
 *
 * @param client to use. Must be connected to the same endpoint given in
 * serverUrl or otherwise in disconnected state.
 * @param serverUrl url to connect (for example "opc.tcp://localhost:4840")
 * @param startingRecordId optional. Only return the records with an ID higher
 *        or equal the given. Can be used for pagination to only get a subset of
 *        the full list
 * @param maxRecordsToReturn optional. Only return this number of records

 * @param serverCapabilityFilterSize optional. Filter the returned list to only
 *        get servers with given capabilities, e.g. "LDS"
 * @param serverCapabilityFilter optional. Filter the returned list to only get
 *        servers with given capabilities, e.g. "LDS"
 * @param serverOnNetworkSize size of returned array, i.e., number of
 *        known/registered servers
 * @param serverOnNetwork array containing known/registered servers
 * @return Indicates whether the operation succeeded or returns an error code */
UA_StatusCode UA_EXPORT
UA_Client_findServersOnNetwork(UA_Client *client, const char *serverUrl,
                               UA_UInt32 startingRecordId, UA_UInt32 maxRecordsToReturn,
                               size_t serverCapabilityFilterSize, UA_String *serverCapabilityFilter,
                               size_t *serverOnNetworkSize, UA_ServerOnNetwork **serverOnNetwork);
#endif

/**
 * .. _client-services:
 *
 * Services
 * --------
 *
 * The raw OPC UA services are exposed to the client. But most of them time, it
 * is better to use the convenience functions from ``ua_client_highlevel.h``
 * that wrap the raw services. */
/* Don't use this function. Use the type versions below instead. */
void UA_EXPORT
__UA_Client_Service(UA_Client *client, const void *request,
                    const UA_DataType *requestType, void *response,
                    const UA_DataType *responseType);

/*
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

/*
* Historical Access Service Set
* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
#ifdef UA_ENABLE_HISTORIZING
static UA_INLINE UA_HistoryReadResponse
UA_Client_Service_historyRead(UA_Client *client, const UA_HistoryReadRequest request) {
    UA_HistoryReadResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_HISTORYREADREQUEST],
        &response, &UA_TYPES[UA_TYPES_HISTORYREADRESPONSE]);
    return response;
}

static UA_INLINE UA_HistoryUpdateResponse
UA_Client_Service_historyUpdate(UA_Client *client, const UA_HistoryUpdateRequest request) {
    UA_HistoryUpdateResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_HISTORYUPDATEREQUEST],
        &response, &UA_TYPES[UA_TYPES_HISTORYUPDATERESPONSE]);
    return response;
}
#endif

/*
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

/*
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
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_ADDREFERENCESREQUEST],
                        &response, &UA_TYPES[UA_TYPES_ADDREFERENCESRESPONSE]);
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
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_DELETEREFERENCESREQUEST],
                        &response, &UA_TYPES[UA_TYPES_DELETEREFERENCESRESPONSE]);
    return response;
}

/*
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

/*
 * Query Service Set
 * ^^^^^^^^^^^^^^^^^ */
#ifdef UA_ENABLE_QUERY

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

#endif

/**
 * .. _client-async-services:
 *
 * Asynchronous Services
 * ---------------------
 * All OPC UA services are asynchronous in nature. So several service calls can
 * be made without waiting for the individual responses. Depending on the
 * server's priorities responses may come in a different ordering than sent.
 *
 * As noted in :ref:`the client overview<client>` currently no means
 * of handling asynchronous events automatically is provided. However, some
 * synchronous function calls will trigger handling, but to ensure this
 * happens a client should periodically call `UA_Client_run_iterate`
 * explicitly.
 *
 * Connection and session management are also performed in
 * `UA_Client_run_iterate`, so to keep a connection healthy any client need to
 * consider how and when it is appropriate to do the call.
 * This is especially true for the periodic renewal of a SecureChannel's
 * SecurityToken which is designed to have a limited lifetime and will
 * invalidate the connection if not renewed.
 */

/* Use the type versions of this method. See below. However, the general
 * mechanism of async service calls is explained here.
 *
 * We say that an async service call has been dispatched once this method
 * returns UA_STATUSCODE_GOOD. If there is an error after an async service has
 * been dispatched, the callback is called with an "empty" response where the
 * statusCode has been set accordingly. This is also done if the client is
 * shutting down and the list of dispatched async services is emptied.
 *
 * The statusCode received when the client is shutting down is
 * UA_STATUSCODE_BADSHUTDOWN.
 *
 * The statusCode received when the client don't receive response
 * after specified config->timeout (in ms) is
 * UA_STATUSCODE_BADTIMEOUT.
 *
 * Instead, you can use __UA_Client_AsyncServiceEx to specify
 * a custom timeout
 *
 * The userdata and requestId arguments can be NULL. */

typedef void (*UA_ClientAsyncServiceCallback)(UA_Client *client, void *userdata,
                                              UA_UInt32 requestId, void *response);

UA_StatusCode UA_EXPORT
__UA_Client_AsyncService(UA_Client *client, const void *request,
                         const UA_DataType *requestType,
                         UA_ClientAsyncServiceCallback callback,
                         const UA_DataType *responseType,
                         void *userdata, UA_UInt32 *requestId);

UA_StatusCode UA_EXPORT
UA_Client_sendAsyncRequest(UA_Client *client, const void *request,
        const UA_DataType *requestType, UA_ClientAsyncServiceCallback callback,
        const UA_DataType *responseType, void *userdata, UA_UInt32 *requestId);

/* Set new userdata and callback for an existing request.
 *
 * @param client Pointer to the UA_Client
 * @param requestId RequestId of the request, which was returned by
 *        UA_Client_sendAsyncRequest before
 * @param userdata The new userdata.
 * @param callback The new callback
 * @return UA_StatusCode UA_STATUSCODE_GOOD on success
 *         UA_STATUSCODE_BADNOTFOUND when no request with requestId is found. */
UA_StatusCode UA_EXPORT
UA_Client_modifyAsyncCallback(UA_Client *client, UA_UInt32 requestId,
        void *userdata, UA_ClientAsyncServiceCallback callback);

/* Listen on the network and process arriving asynchronous responses in the
 * background. Internal housekeeping, renewal of SecureChannels and subscription
 * management is done as well. */
UA_StatusCode UA_EXPORT
UA_Client_run_iterate(UA_Client *client, UA_UInt32 timeout);

/* Force the manual renewal of the SecureChannel. This is useful to renew the
 * SecureChannel during a downtime when no time-critical operations are
 * performed. This method is asynchronous. The renewal is triggered (the OPN
 * message is sent) but not completed. The OPN response is handled with
 * ``UA_Client_run_iterate`` or a synchronous servica-call operation.
 *
 * @return The return value is UA_STATUSCODE_GOODCALLAGAIN if the SecureChannel
 *         has not elapsed at least 75% of its lifetime. Otherwise the
 *         ``connectStatus`` is returned. */
UA_StatusCode UA_EXPORT
UA_Client_renewSecureChannel(UA_Client *client);

/* Use the type versions of this method. See below. However, the general
 * mechanism of async service calls is explained here.
 *
 * We say that an async service call has been dispatched once this method
 * returns UA_STATUSCODE_GOOD. If there is an error after an async service has
 * been dispatched, the callback is called with an "empty" response where the
 * statusCode has been set accordingly. This is also done if the client is
 * shutting down and the list of dispatched async services is emptied.
 *
 * The statusCode received when the client is shutting down is
 * UA_STATUSCODE_BADSHUTDOWN.
 *
 * The statusCode received when the client don't receive response
 * after specified timeout (in ms) is
 * UA_STATUSCODE_BADTIMEOUT.
 *
 * The timeout can be disabled by setting timeout to 0
 *
 * The userdata and requestId arguments can be NULL. */
UA_StatusCode UA_EXPORT
__UA_Client_AsyncServiceEx(UA_Client *client, const void *request,
                           const UA_DataType *requestType,
                           UA_ClientAsyncServiceCallback callback,
                           const UA_DataType *responseType,
                           void *userdata, UA_UInt32 *requestId,
                           UA_UInt32 timeout);

/**
 * Timed Callbacks
 * ---------------
 * Repeated callbacks can be attached to a client and will be executed in the
 * defined interval. */

typedef void (*UA_ClientCallback)(UA_Client *client, void *data);

/* Add a callback for execution at a specified time. If the indicated time lies
 * in the past, then the callback is executed at the next iteration of the
 * server's main loop.
 *
 * @param client The client object.
 * @param callback The callback that shall be added.
 * @param data Data that is forwarded to the callback.
 * @param date The timestamp for the execution time.
 * @param callbackId Set to the identifier of the repeated callback . This can
 *        be used to cancel the callback later on. If the pointer is null, the
 *        identifier is not set.
 * @return Upon success, UA_STATUSCODE_GOOD is returned. An error code
 *         otherwise. */
UA_StatusCode UA_EXPORT
UA_Client_addTimedCallback(UA_Client *client, UA_ClientCallback callback,
                           void *data, UA_DateTime date, UA_UInt64 *callbackId);

/* Add a callback for cyclic repetition to the client.
 *
 * @param client The client object.
 * @param callback The callback that shall be added.
 * @param data Data that is forwarded to the callback.
 * @param interval_ms The callback shall be repeatedly executed with the given
 *        interval (in ms). The interval must be positive. The first execution
 *        occurs at now() + interval at the latest.
 * @param callbackId Set to the identifier of the repeated callback . This can
 *        be used to cancel the callback later on. If the pointer is null, the
 *        identifier is not set.
 * @return Upon success, UA_STATUSCODE_GOOD is returned. An error code
 *         otherwise. */
UA_StatusCode UA_EXPORT
UA_Client_addRepeatedCallback(UA_Client *client, UA_ClientCallback callback,
                              void *data, UA_Double interval_ms,
                              UA_UInt64 *callbackId);

UA_StatusCode UA_EXPORT
UA_Client_changeRepeatedCallbackInterval(UA_Client *client,
                                         UA_UInt64 callbackId,
                                         UA_Double interval_ms);

void UA_EXPORT
UA_Client_removeCallback(UA_Client *client, UA_UInt64 callbackId);

/**
 * Client Utility Functions
 * ------------------------ */

/* Lookup a datatype by its NodeId. Takes the custom types in the client
 * configuration into account. Return NULL if none found. */
UA_EXPORT const UA_DataType *
UA_Client_findDataType(UA_Client *client, const UA_NodeId *typeId);

/**
 * .. toctree::
 *
 *    client_highlevel
 *    client_subscriptions */

_UA_END_DECLS

#endif /* UA_CLIENT_H_ */
