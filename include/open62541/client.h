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
 *    Copyright 2022 (c) Linutronix GmbH (Author: Muddasir Shakil)
 */

#ifndef UA_CLIENT_H_
#define UA_CLIENT_H_

#include <open62541/types.h>
#include <open62541/common.h>
#include <open62541/util.h>

#include <open62541/plugin/log.h>
#include <open62541/plugin/eventloop.h>
#include <open62541/plugin/securitypolicy.h>

/* Forward declarations */
struct UA_ClientConfig;
typedef struct UA_ClientConfig UA_ClientConfig;

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

struct UA_ClientConfig {
    void *clientContext; /* User-defined pointer attached to the client */
    UA_Logger *logging;  /* Plugin for log output */

    /* Response timeout in ms (0 -> no timeout). If the server does not answer a
     * request within this time a StatusCode UA_STATUSCODE_BADTIMEOUT is
     * returned. This timeout can be overridden for individual requests by
     * setting a non-null "timeoutHint" in the request header. */
    UA_UInt32 timeout;

    /* The description must be internally consistent.
     * - The ApplicationUri set in the ApplicationDescription must match the
     *   URI set in the certificate */
    UA_ApplicationDescription clientDescription;

    /* The endpoint for the client to connect to.
     * Such as "opc.tcp://host:port". */
    UA_String endpointUrl;

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

    UA_Boolean noSession;   /* Only open a SecureChannel, but no Session */
    UA_Boolean noReconnect; /* Don't reconnect SecureChannel when the connection
                             * is lost without explicitly closing. */
    UA_Boolean noNewSession; /* Don't automatically create a new Session when
                              * the intial one is lost. Instead abort the
                              * connection when the Session is lost. */

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
     * If the EndpointDescription has not been defined, the ApplicationURI
     * constrains the servers considered in the FindServers service and the
     * Endpoints considered in the GetEndpoints service.
     *
     * If empty the applicationURI is not used to filter.
     */
    UA_String applicationUri;

    /**
     * The following settings are specific to OPC UA with TCP transport. */
    UA_Boolean tcpReuseAddr;

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
    UA_CertificateGroup certificateVerification;

    /* Available SecurityPolicies for Authentication. The policy defined by the
     * AccessControl is selected. If no policy is defined, the policy of the
     * secure channel is selected.*/
    size_t authSecurityPoliciesSize;
    UA_SecurityPolicy *authSecurityPolicies;
    /* SecurityPolicyUri for the Authentication. */
    UA_String authSecurityPolicyUri;

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

    /* Number of PublishResponse queued up in the server */
    UA_UInt16 outStandingPublishRequests;

    /* If the client does not receive a PublishResponse after the defined delay
     * of ``(sub->publishingInterval * sub->maxKeepAliveCount) +
     * client->config.timeout)``, then subscriptionInactivityCallback is called
     * for the subscription.. */
    void (*subscriptionInactivityCallback)(UA_Client *client,
                                           UA_UInt32 subscriptionId,
                                           void *subContext);

    /* Session config */
    UA_String sessionName;
    UA_LocaleId *sessionLocaleIds;
    size_t sessionLocaleIdsSize;

#ifdef UA_ENABLE_ENCRYPTION
    /* If the private key is in PEM format and password protected, this callback
     * is called during initialization to get the password to decrypt the
     * private key. The memory containing the password is freed by the client
     * after use. The callback should be set early, other parts of the client
     * config setup may depend on it. */
    UA_StatusCode (*privateKeyPasswordCallback)(UA_ClientConfig *cc,
                                                UA_ByteString *password);
#endif
};

/**
 * @brief It makes a partial deep copy of the clientconfig. It makes a shallow
 * copies of the plugins (logger, eventloop, securitypolicy).
 *
 * NOTE: It makes a shallow copy of all the plugins from source to destination.
 * Therefore calling _clear on the dst object will also delete the plugins in src
 * object.
 */
UA_EXPORT UA_StatusCode
UA_ClientConfig_copy(UA_ClientConfig const *src, UA_ClientConfig *dst);

/**
 * @brief It cleans the client config and frees the pointer.
 */
UA_EXPORT void
UA_ClientConfig_delete(UA_ClientConfig *config);

/**
 * @brief It cleans the client config and deletes the plugins, whereas
 * _copy makes a shallow copy of the plugins.
 */
UA_EXPORT void
UA_ClientConfig_clear(UA_ClientConfig *config);

/* Configure Username/Password for the Session authentication. Also see
 * UA_ClientConfig_setAuthenticationCert for x509-based authentication, which is
 * implemented as a plugin (as it can be based on different crypto
 * libraries). */
UA_INLINABLE( UA_StatusCode
UA_ClientConfig_setAuthenticationUsername(UA_ClientConfig *config,
                                          const char *username,
                                          const char *password) ,{
    UA_UserNameIdentityToken* identityToken = UA_UserNameIdentityToken_new();
    if(!identityToken)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    identityToken->userName = UA_STRING_ALLOC(username);
    identityToken->password = UA_STRING_ALLOC(password);

    UA_ExtensionObject_clear(&config->userIdentityToken);
    UA_ExtensionObject_setValue(&config->userIdentityToken, identityToken,
                                &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN]);
    return UA_STATUSCODE_GOOD;
})

/**
 * Client Lifecycle
 * ---------------- */

/* Create a new client with a default configuration that adds plugins for
 * networking, security, logging and so on. See `client_config_default.h` for
 * more detailed options.
 *
 * The default configuration can be used as the starting point to adjust the
 * client configuration to individual needs. UA_Client_new is implemented in the
 * /plugins folder under the CC0 license. Furthermore the client confiugration
 * only uses the public server API.
 *
 * @return Returns the configured client or NULL if an error occurs. */
UA_EXPORT UA_Client * UA_Client_new(void);

/* Creates a new client. Moves the config into the client with a shallow copy.
 * The config content is cleared together with the client. */
UA_Client UA_EXPORT *
UA_Client_newWithConfig(const UA_ClientConfig *config);

/* Returns the current state. All arguments except ``client`` can be NULL. */
void UA_EXPORT UA_THREADSAFE
UA_Client_getState(UA_Client *client,
                   UA_SecureChannelState *channelState,
                   UA_SessionState *sessionState,
                   UA_StatusCode *connectStatus);

/* Get the client configuration */
UA_EXPORT UA_ClientConfig *
UA_Client_getConfig(UA_Client *client);

/* Get the client context */
UA_INLINABLE( void *
UA_Client_getContext(UA_Client *client) ,{
    return UA_Client_getConfig(client)->clientContext; /* Cannot fail */
})

/* (Disconnect and) delete the client */
void UA_EXPORT
UA_Client_delete(UA_Client *client);

/**
 * Connection Attrbiutes
 * ---------------------
 *
 * Besides the client configuration, some attributes of the connection are
 * defined only at runtime. For example the choice of SecurityPolicy or the
 * ApplicationDescripton from the server. This API allows to access such
 * connection attributes.
 *
 * The currently defined connection attributes are:
 *
 * - 0:serverDescription [UA_ApplicationDescription]: Server description
 * - 0:securityPolicyUri [UA_String]: Uri of the SecurityPolicy used
 * - 0:securityMode [UA_MessageSecurityMode]: SecurityMode of the SecureChannel
 */

/* Returns a shallow copy of the attribute. Don't _clear or _delete the value
 * variant. Don't use the value after returning the control flow to the client.
 * Also don't use this in a multi-threaded application. */
UA_EXPORT UA_StatusCode
UA_Client_getConnectionAttribute(UA_Client *client, const UA_QualifiedName key,
                                 UA_Variant *outValue);

/* Return a deep copy of the attribute */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Client_getConnectionAttributeCopy(UA_Client *client, const UA_QualifiedName key,
                                     UA_Variant *outValue);

/* Returns NULL if the attribute is not defined or not a scalar or not of the
 * right datatype. Otherwise a shallow copy of the scalar value is created at
 * the target location of the void pointer. Hence don't use this in a
 * multi-threaded application. */
UA_EXPORT UA_StatusCode
UA_Client_getConnectionAttribute_scalar(UA_Client *client,
                                        const UA_QualifiedName key,
                                        const UA_DataType *type,
                                        void *outValue);

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

/* Connect with the client configuration. For the async connection, finish
 * connecting via UA_Client_run_iterate (or manually running a configured
 * external EventLoop). */
UA_StatusCode UA_EXPORT UA_THREADSAFE
__UA_Client_connect(UA_Client *client, UA_Boolean async);

/* Connect to the server. First a SecureChannel is opened, then a Session. The
 * client configuration restricts the SecureChannel selection and contains the
 * UserIdentityToken for the Session.
 *
 * @param client to use
 * @param endpointURL to connect (for example "opc.tcp://localhost:4840")
 * @return Indicates whether the operation succeeded or returns an error code */
UA_INLINABLE( UA_StatusCode
UA_Client_connect(UA_Client *client, const char *endpointUrl), {
    /* Update the configuration */
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    cc->noSession = false; /* Open a Session */
    UA_String_clear(&cc->endpointUrl);
    cc->endpointUrl = UA_STRING_ALLOC(endpointUrl);

    /* Connect */
    return __UA_Client_connect(client, false);
})

/* Connect async (non-blocking) to the server. After initiating the connection,
 * call UA_Client_run_iterate repeatedly until the connection is fully
 * established. You can set a callback to client->config.stateCallback to be
 * notified when the connection status changes. Or use UA_Client_getState to get
 * the state manually. */
UA_INLINABLE( UA_StatusCode
UA_Client_connectAsync(UA_Client *client, const char *endpointUrl) ,{
    /* Update the configuration */
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    cc->noSession = false; /* Open a Session */
    UA_String_clear(&cc->endpointUrl);
    cc->endpointUrl = UA_STRING_ALLOC(endpointUrl);

    /* Connect */
    return __UA_Client_connect(client, true);
})

/* Connect to the server without creating a session
 *
 * @param client to use
 * @param endpointURL to connect (for example "opc.tcp://localhost:4840")
 * @return Indicates whether the operation succeeded or returns an error code */
UA_INLINABLE( UA_StatusCode
UA_Client_connectSecureChannel(UA_Client *client, const char *endpointUrl) ,{
    /* Update the configuration */
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    cc->noSession = true; /* Don't open a Session */
    UA_String_clear(&cc->endpointUrl);
    cc->endpointUrl = UA_STRING_ALLOC(endpointUrl);

    /* Connect */
    return __UA_Client_connect(client, false);
})

/* Connect async (non-blocking) only the SecureChannel */
UA_INLINABLE( UA_StatusCode
UA_Client_connectSecureChannelAsync(UA_Client *client, const char *endpointUrl) ,{
    /* Update the configuration */
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    cc->noSession = true; /* Don't open a Session */
    UA_String_clear(&cc->endpointUrl);
    cc->endpointUrl = UA_STRING_ALLOC(endpointUrl);

    /* Connect */
    return __UA_Client_connect(client, false);
})

/* Connect to the server and create+activate a Session with the given username
 * and password. This first set the UserIdentityToken in the client config and
 * then calls the regular connect method. */
UA_INLINABLE( UA_StatusCode
UA_Client_connectUsername(UA_Client *client, const char *endpointUrl,
                          const char *username, const char *password) ,{
    /* Set the user identity token */
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_StatusCode res =
        UA_ClientConfig_setAuthenticationUsername(cc, username, password);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Connect */
    return UA_Client_connect(client, endpointUrl);
})

/* Sets up a listening socket for incoming reverse connect requests by OPC UA
 * servers. After the first server has connected, the listening socket is
 * removed. The client state callback is also used for reverse connect. An
 * implementation could for example issue a new call to
 * UA_Client_startListeningForReverseConnect after the server has closed the
 * connection. If the client is connected to any server while
 * UA_Client_startListeningForReverseConnect is called, the connection will be
 * closed.
 *
 * The reverse connect is closed by calling the standard disconnect functions
 * like for a "normal" connection that was initiated by the client. Calling one
 * of the connect methods will also close the listening socket and the
 * connection to the remote server. */
UA_StatusCode UA_EXPORT
UA_Client_startListeningForReverseConnect(
    UA_Client *client, const UA_String *listenHostnames,
    size_t listenHostnamesLength, UA_UInt16 port);

/* Disconnect and close a connection to the selected server. Disconnection is
 * always performed async (without blocking). */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_disconnect(UA_Client *client);

/* Disconnect async. Run UA_Client_run_iterate until the callback notifies that
 * all connections are closed. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_disconnectAsync(UA_Client *client);

/* Disconnect the SecureChannel but keep the Session intact (if it exists). */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_disconnectSecureChannel(UA_Client *client);

/* Disconnect the SecureChannel but keep the Session intact (if it exists). This
 * is an async operation. Iterate the client until the SecureChannel was fully
 * cleaned up. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_disconnectSecureChannelAsync(UA_Client *client);

/* Get the AuthenticationToken and ServerNonce required to activate the current
 * Session on a different SecureChannel. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_getSessionAuthenticationToken(
    UA_Client *client, UA_NodeId *authenticationToken, UA_ByteString *serverNonce);

/* Re-activate the current session. A change of prefered locales can be done by
 * updating the client configuration. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_activateCurrentSession(UA_Client *client);

/* Async version of UA_Client_activateCurrentSession */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_activateCurrentSessionAsync(UA_Client *client);

/* Activate an already created Session. This allows a Session to be transferred
 * from a different client instance. The AuthenticationToken and ServerNonce
 * must be provided for this. Both can be retrieved for an activated Session
 * with UA_Client_getSessionAuthenticationToken.
 *
 * The UserIdentityToken used for authentication must be identical to the
 * original activation of the Session. The UserIdentityToken is set in the
 * client configuration.
 *
 * Note the noNewSession option if there should not be a new Session
 * automatically created when this one closes. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_activateSession(UA_Client *client,
                          const UA_NodeId authenticationToken,
                          const UA_ByteString serverNonce);

/* Async version of UA_Client_activateSession */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_activateSessionAsync(UA_Client *client,
                               const UA_NodeId authenticationToken,
                               const UA_ByteString serverNonce);

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
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_getEndpoints(UA_Client *client, const char *serverUrl,
                       size_t* endpointDescriptionsSize,
                       UA_EndpointDescription** endpointDescriptions);

/* Gets a list of all registered servers at the given server.
 *
 * You can pass an optional filter for serverUris. If the given server is not
 * registered, an empty array will be returned. If the server is registered,
 * only that application description will be returned.
 *
 * Additionally you can optionally indicate which locale you want for the server
 * name in the returned application description. The array indicates the order
 * of preference. A server may have localized names.
 *
 * @param client to use. Must be connected to the same endpoint given in
 *        serverUrl or otherwise in disconnected state.
 * @param serverUrl url to connect (for example "opc.tcp://localhost:4840")
 * @param serverUrisSize Optional filter for specific server uris
 * @param serverUris Optional filter for specific server uris
 * @param localeIdsSize Optional indication which locale you prefer
 * @param localeIds Optional indication which locale you prefer
 * @param registeredServersSize size of returned array, i.e., number of
 *        found/registered servers
 * @param registeredServers array containing found/registered servers
 * @return Indicates whether the operation succeeded or returns an error code */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_findServers(UA_Client *client, const char *serverUrl,
                      size_t serverUrisSize, UA_String *serverUris,
                      size_t localeIdsSize, UA_String *localeIds,
                      size_t *registeredServersSize,
                      UA_ApplicationDescription **registeredServers);

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
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_findServersOnNetwork(UA_Client *client, const char *serverUrl,
                               UA_UInt32 startingRecordId,
                               UA_UInt32 maxRecordsToReturn,
                               size_t serverCapabilityFilterSize,
                               UA_String *serverCapabilityFilter,
                               size_t *serverOnNetworkSize,
                               UA_ServerOnNetwork **serverOnNetwork);

/**
 * .. _client-services:
 *
 * Services
 * --------
 *
 * The raw OPC UA services are exposed to the client. But most of the time, it
 * is better to use the convenience functions from ``ua_client_highlevel.h``
 * that wrap the raw services. */
/* Don't use this function. Use the type versions below instead. */
void UA_EXPORT UA_THREADSAFE
__UA_Client_Service(UA_Client *client, const void *request,
                    const UA_DataType *requestType, void *response,
                    const UA_DataType *responseType);

/*
 * Attribute Service Set
 * ^^^^^^^^^^^^^^^^^^^^^ */
UA_INLINABLE( UA_THREADSAFE UA_ReadResponse
UA_Client_Service_read(UA_Client *client, const UA_ReadRequest request) ,{
    UA_ReadResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_READREQUEST],
                        &response, &UA_TYPES[UA_TYPES_READRESPONSE]);
    return response;
})

UA_INLINABLE( UA_THREADSAFE UA_WriteResponse
UA_Client_Service_write(UA_Client *client, const UA_WriteRequest request) ,{
    UA_WriteResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_WRITEREQUEST],
                        &response, &UA_TYPES[UA_TYPES_WRITERESPONSE]);
    return response;
})

/*
* Historical Access Service Set
* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
UA_INLINABLE( UA_THREADSAFE UA_HistoryReadResponse
UA_Client_Service_historyRead(UA_Client *client,
                              const UA_HistoryReadRequest request) ,{
    UA_HistoryReadResponse response;
    __UA_Client_Service(
        client, &request, &UA_TYPES[UA_TYPES_HISTORYREADREQUEST],
        &response, &UA_TYPES[UA_TYPES_HISTORYREADRESPONSE]);
    return response;
})

UA_INLINABLE( UA_THREADSAFE UA_HistoryUpdateResponse
UA_Client_Service_historyUpdate(UA_Client *client,
                                const UA_HistoryUpdateRequest request) ,{
    UA_HistoryUpdateResponse response;
    __UA_Client_Service(
        client, &request, &UA_TYPES[UA_TYPES_HISTORYUPDATEREQUEST],
        &response, &UA_TYPES[UA_TYPES_HISTORYUPDATERESPONSE]);
    return response;
})

/*
 * Method Service Set
 * ^^^^^^^^^^^^^^^^^^ */
UA_INLINABLE( UA_THREADSAFE UA_CallResponse
UA_Client_Service_call(UA_Client *client,
                       const UA_CallRequest request) ,{
    UA_CallResponse response;
    __UA_Client_Service(
        client, &request, &UA_TYPES[UA_TYPES_CALLREQUEST],
        &response, &UA_TYPES[UA_TYPES_CALLRESPONSE]);
    return response;
})

/*
 * NodeManagement Service Set
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^ */
UA_INLINABLE( UA_THREADSAFE UA_AddNodesResponse
UA_Client_Service_addNodes(UA_Client *client,
                           const UA_AddNodesRequest request) ,{
    UA_AddNodesResponse response;
    __UA_Client_Service(
        client, &request, &UA_TYPES[UA_TYPES_ADDNODESREQUEST],
        &response, &UA_TYPES[UA_TYPES_ADDNODESRESPONSE]);
    return response;
})

UA_INLINABLE( UA_THREADSAFE UA_AddReferencesResponse
UA_Client_Service_addReferences(UA_Client *client,
                                const UA_AddReferencesRequest request) ,{
    UA_AddReferencesResponse response;
    __UA_Client_Service(
        client, &request, &UA_TYPES[UA_TYPES_ADDREFERENCESREQUEST],
        &response, &UA_TYPES[UA_TYPES_ADDREFERENCESRESPONSE]);
    return response;
})

UA_INLINABLE( UA_THREADSAFE UA_DeleteNodesResponse
UA_Client_Service_deleteNodes(UA_Client *client,
                              const UA_DeleteNodesRequest request) ,{
    UA_DeleteNodesResponse response;
    __UA_Client_Service(
        client, &request, &UA_TYPES[UA_TYPES_DELETENODESREQUEST],
        &response, &UA_TYPES[UA_TYPES_DELETENODESRESPONSE]);
    return response;
})

UA_INLINABLE( UA_THREADSAFE UA_DeleteReferencesResponse
UA_Client_Service_deleteReferences(
    UA_Client *client, const UA_DeleteReferencesRequest request) ,{
    UA_DeleteReferencesResponse response;
    __UA_Client_Service(
        client, &request, &UA_TYPES[UA_TYPES_DELETEREFERENCESREQUEST],
        &response, &UA_TYPES[UA_TYPES_DELETEREFERENCESRESPONSE]);
    return response;
})

/*
 * View Service Set
 * ^^^^^^^^^^^^^^^^ */
UA_INLINABLE( UA_THREADSAFE UA_BrowseResponse
UA_Client_Service_browse(UA_Client *client,
                         const UA_BrowseRequest request) ,{
    UA_BrowseResponse response;
    __UA_Client_Service(
        client, &request, &UA_TYPES[UA_TYPES_BROWSEREQUEST],
        &response, &UA_TYPES[UA_TYPES_BROWSERESPONSE]);
    return response;
})

UA_INLINABLE( UA_THREADSAFE UA_BrowseNextResponse
UA_Client_Service_browseNext(UA_Client *client,
                             const UA_BrowseNextRequest request) ,{
    UA_BrowseNextResponse response;
    __UA_Client_Service(
        client, &request, &UA_TYPES[UA_TYPES_BROWSENEXTREQUEST],
        &response, &UA_TYPES[UA_TYPES_BROWSENEXTRESPONSE]);
    return response;
})

UA_INLINABLE( UA_THREADSAFE UA_TranslateBrowsePathsToNodeIdsResponse
UA_Client_Service_translateBrowsePathsToNodeIds(
    UA_Client *client,
    const UA_TranslateBrowsePathsToNodeIdsRequest request) ,{
    UA_TranslateBrowsePathsToNodeIdsResponse response;
    __UA_Client_Service(
        client, &request,
        &UA_TYPES[UA_TYPES_TRANSLATEBROWSEPATHSTONODEIDSREQUEST],
        &response,
        &UA_TYPES[UA_TYPES_TRANSLATEBROWSEPATHSTONODEIDSRESPONSE]);
    return response;
})

UA_INLINABLE( UA_THREADSAFE UA_RegisterNodesResponse
UA_Client_Service_registerNodes(
    UA_Client *client, const UA_RegisterNodesRequest request) ,{
    UA_RegisterNodesResponse response;
    __UA_Client_Service(
        client, &request, &UA_TYPES[UA_TYPES_REGISTERNODESREQUEST],
        &response, &UA_TYPES[UA_TYPES_REGISTERNODESRESPONSE]);
    return response;
})

UA_INLINABLE( UA_THREADSAFE UA_UnregisterNodesResponse
UA_Client_Service_unregisterNodes(
    UA_Client *client, const UA_UnregisterNodesRequest request) ,{
    UA_UnregisterNodesResponse response;
    __UA_Client_Service(
        client, &request, &UA_TYPES[UA_TYPES_UNREGISTERNODESREQUEST],
        &response, &UA_TYPES[UA_TYPES_UNREGISTERNODESRESPONSE]);
    return response;
})

/*
 * Query Service Set
 * ^^^^^^^^^^^^^^^^^ */
#ifdef UA_ENABLE_QUERY

UA_INLINABLE( UA_THREADSAFE UA_QueryFirstResponse
UA_Client_Service_queryFirst(UA_Client *client,
                             const UA_QueryFirstRequest request) ,{
    UA_QueryFirstResponse response;
    __UA_Client_Service(
        client, &request, &UA_TYPES[UA_TYPES_QUERYFIRSTREQUEST],
        &response, &UA_TYPES[UA_TYPES_QUERYFIRSTRESPONSE]);
    return response;
})

UA_INLINABLE( UA_THREADSAFE UA_QueryNextResponse
UA_Client_Service_queryNext(UA_Client *client,
                            const UA_QueryNextRequest request) ,{
    UA_QueryNextResponse response;
    __UA_Client_Service(
        client, &request, &UA_TYPES[UA_TYPES_QUERYFIRSTREQUEST],
        &response, &UA_TYPES[UA_TYPES_QUERYFIRSTRESPONSE]);
    return response;
})

#endif

/**
 * .. _client-async-services:
 *
 * Asynchronous Services
 * ---------------------
 * All OPC UA services are asynchronous in nature. So several service calls can
 * be made without waiting for the individual responses. Depending on the
 * server's priorities responses may come in a different ordering than sent. Use
 * the typed wrappers for async service requests instead of
 * `__UA_Client_AsyncService` directly. See :ref:`client_async`. However, the
 * general mechanism of async service calls is explained here.
 *
 * Connection and session management are performed in `UA_Client_run_iterate`,
 * so to keep a connection healthy any client needs to consider how and when it
 * is appropriate to do the call. This is especially true for the periodic
 * renewal of a SecureChannel's SecurityToken which is designed to have a
 * limited lifetime and will invalidate the connection if not renewed.
 *
 * We say that an async service call has been dispatched once
 * __UA_Client_AsyncService returns UA_STATUSCODE_GOOD. If there is an error
 * after an async service has been dispatched, the callback is called with an
 * "empty" response where the StatusCode has been set accordingly. This is also
 * done if the client is shutting down and the list of dispatched async services
 * is emptied.
 *
 * The StatusCode received when the client is shutting down is
 * UA_STATUSCODE_BADSHUTDOWN. The StatusCode received when the client doesn't
 * receive response after the specified in config->timeout (can be overridden
 * via the "timeoutHint" in the request header) is UA_STATUSCODE_BADTIMEOUT.
 *
 * The userdata and requestId arguments can be NULL. The (optional) requestId
 * output can be used to cancel the service while it is still pending. The
 * requestId is unique for each service request. Alternatively the requestHandle
 * can be manually set (non necessarily unique) in the request header for full
 * service call. This can be used to cancel all outstanding requests using that
 * handle together. Note that the client will auto-generate a requestHandle
 * >100,000 if none is defined. Avoid these when manually setting a requetHandle
 * in the requestHeader to avoid clashes. */

typedef void
(*UA_ClientAsyncServiceCallback)(UA_Client *client, void *userdata,
                                 UA_UInt32 requestId, void *response);

UA_StatusCode UA_EXPORT UA_THREADSAFE
__UA_Client_AsyncService(UA_Client *client, const void *request,
                         const UA_DataType *requestType,
                         UA_ClientAsyncServiceCallback callback,
                         const UA_DataType *responseType,
                         void *userdata, UA_UInt32 *requestId);

/* Cancel all dispatched requests with the given requestHandle.
 * The number if cancelled requests is returned by the server.
 * The output argument cancelCount is not set if NULL. */
UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_cancelByRequestHandle(UA_Client *client, UA_UInt32 requestHandle,
                                UA_UInt32 *cancelCount);

/* Map the requestId to the requestHandle used for that request and call the
 * Cancel service for that requestHandle. */
UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_cancelByRequestId(UA_Client *client, UA_UInt32 requestId,
                            UA_UInt32 *cancelCount);

/* Set new userdata and callback for an existing request.
 *
 * @param client Pointer to the UA_Client
 * @param requestId RequestId of the request, which was returned by
 *        __UA_Client_AsyncService before
 * @param userdata The new userdata
 * @param callback The new callback
 * @return UA_StatusCode UA_STATUSCODE_GOOD on success
 *         UA_STATUSCODE_BADNOTFOUND when no request with requestId is found. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_modifyAsyncCallback(UA_Client *client, UA_UInt32 requestId,
                              void *userdata, UA_ClientAsyncServiceCallback callback);

/* Listen on the network and process arriving asynchronous responses in the
 * background. Internal housekeeping, renewal of SecureChannels and subscription
 * management is done as well. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_run_iterate(UA_Client *client, UA_UInt32 timeout);

/* Force the manual renewal of the SecureChannel. This is useful to renew the
 * SecureChannel during a downtime when no time-critical operations are
 * performed. This method is asynchronous. The renewal is triggered (the OPN
 * message is sent) but not completed. The OPN response is handled with
 * ``UA_Client_run_iterate`` or a synchronous service-call operation.
 *
 * @return The return value is UA_STATUSCODE_GOODCALLAGAIN if the SecureChannel
 *         has not elapsed at least 75% of its lifetime. Otherwise the
 *         ``connectStatus`` is returned. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_renewSecureChannel(UA_Client *client);

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
 * @param callbackId Set to the identifier of the repeated callback. This can
 *        be used to cancel the callback later on. If the pointer is null, the
 *        identifier is not set.
 * @return Upon success, UA_STATUSCODE_GOOD is returned. An error code
 *         otherwise. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
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
 * @param callbackId Set to the identifier of the repeated callback. This can
 *        be used to cancel the callback later on. If the pointer is null, the
 *        identifier is not set.
 * @return Upon success, UA_STATUSCODE_GOOD is returned. An error code
 *         otherwise. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_addRepeatedCallback(UA_Client *client, UA_ClientCallback callback,
                              void *data, UA_Double interval_ms,
                              UA_UInt64 *callbackId);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_changeRepeatedCallbackInterval(UA_Client *client,
                                         UA_UInt64 callbackId,
                                         UA_Double interval_ms);

void UA_EXPORT UA_THREADSAFE
UA_Client_removeCallback(UA_Client *client, UA_UInt64 callbackId);

#define UA_Client_removeRepeatedCallback(server, callbackId)    \
    UA_Client_removeCallback(server, callbackId);

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
 *    client_highlevel_async
 *    client_subscriptions */

_UA_END_DECLS

#endif /* UA_CLIENT_H_ */
