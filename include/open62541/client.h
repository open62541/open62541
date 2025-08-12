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
struct UA_Client;
typedef struct UA_Client UA_Client;

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
 * **Attention**: The client does not start its own thread. The user has to
 * periodically call `UA_Client_run_iterate` to ensure that asynchronous events
 * and housekeeping tasks are handled, including keeping a secure connection
 * established. See more about :ref:`asynchronicity<client-async-services>` and
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

    /* Connection configuration
     * ~~~~~~~~~~~~~~~~~~~~~~~~
     * The following configuration elements reduce the "degrees of freedom" the
     * client has when connecting to a server. If no connection can be made
     * under these restrictions, then the connection will abort with an error
     * message. */
    UA_ExtensionObject userIdentityToken; /* Configured User-Identity Token */
    UA_MessageSecurityMode securityMode;  /* None, Sign, SignAndEncrypt. The
                                           * default is "invalid". This
                                           * indicates the client to select any
                                           * matching endpoint. */
    UA_String securityPolicyUri; /* SecurityPolicy for the SecureChannel. An
                                  * empty string indicates the client to select
                                  * any matching SecurityPolicy. */

    UA_Boolean noSession;   /* Only open a SecureChannel, but no Session */
    UA_Boolean noReconnect; /* Don't reconnect SecureChannel when the connection
                             * is lost without explicitly closing. */
    UA_Boolean noNewSession; /* Don't automatically create a new Session when
                              * the intial one is lost. Instead abort the
                              * connection when the Session is lost. */

    /* If either endpoint or userTokenPolicy has been set, then they are used
     * directly. Otherwise this information comes from the GetEndpoints response
     * from the server (filtered and selected for the SecurityMode, etc.). */
    UA_EndpointDescription endpoint;
    UA_UserTokenPolicy userTokenPolicy;

    /* If the EndpointDescription has not been defined, the ApplicationURI
     * filters the servers considered in the FindServers service and the
     * Endpoints considered in the GetEndpoints service. */
    UA_String applicationUri;

    /* The following settings are specific to OPC UA with TCP transport. */
    UA_Boolean tcpReuseAddr;

    /* Custom Data Types
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

    /* Namespace Mapping
     * ~~~~~~~~~~~~~~~~~
     * The namespaces index is "just" a mapping to the Uris in the namespace
     * array of the server. In order to have stable NodeIds across servers, the
     * client keeps a list of predefined namespaces. Use
     * ``UA_Client_addNamespaceUri``, ``UA_Client_getNamespaceUri`` and
     * ``UA_Client_getNamespaceIndex`` to interact with the local namespace
     * mapping.
     *
     * The namespace indices are assigned internally in the client as follows:
     *
     * - Ns0 and Ns1 are pre-defined by the standard. Ns0 is always
     *   ```http://opcfoundation.org/UA/``` and used for standard-defined
     *   NodeIds. Ns1 corresponds to the application uri of the individual
     *   server.
     * - The next namespaces are added in-order from the list below at startup
     *   (starting at index 2).
     * - The local API ``UA_Client_addNamespaceUri`` can be used to add more
     *   namespaces.
     * - When the client connects, the namespace array of the server is read.
     *   All previously unknown namespaces are added from this to the internal
     *   array of the client. */
    UA_String *namespaces;
    size_t namespacesSize;

    /* Advanced Client Configuration
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

#ifdef UA_ENABLE_ENCRYPTION
    /* Limits for TrustList */
    UA_UInt32 maxTrustListSize; /* in bytes, 0 => unlimited */
    UA_UInt32 maxRejectedListSize; /* 0 => unlimited */
#endif

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

/* Makes a partial deep copy of the clientconfig. The copies of the plugins
 * (logger, eventloop, securitypolicy) are shallow. Therefore calling _clear on
 * the dst object will also delete the plugins in src object. */
UA_EXPORT UA_StatusCode
UA_ClientConfig_copy(UA_ClientConfig const *src, UA_ClientConfig *dst);

/* Cleans the client config and frees the pointer */
UA_EXPORT void
UA_ClientConfig_delete(UA_ClientConfig *config);

/* Cleans the client config */
UA_EXPORT void
UA_ClientConfig_clear(UA_ClientConfig *config);

/* Configure Username/Password for the Session authentication. Also see
 * UA_ClientConfig_setAuthenticationCert for x509-based authentication, which is
 * implemented as a plugin (as it can be based on different crypto
 * libraries). */
UA_EXPORT UA_StatusCode
UA_ClientConfig_setAuthenticationUsername(UA_ClientConfig *config,
                                          const char *username,
                                          const char *password);

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

/* Get the client context (inside the context) */
#define UA_Client_getContext(client) \
    UA_Client_getConfig(client)->clientContext

/* (Disconnect and) delete the client */
void UA_EXPORT
UA_Client_delete(UA_Client *client);

/* Listen on the network and process arriving asynchronous responses in the
 * background. Internal housekeeping, renewal of SecureChannels and subscription
 * management is done as well. Running _run_iterate is required for asynchronous
 * operations. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_run_iterate(UA_Client *client, UA_UInt32 timeout);

/**
 * Connect to a Server
 * -------------------
 *
 * A Client connecting to an OPC UA Server first opens a SecureChannel and on
 * top of that opens a Session. The Session can also be moved to another
 * SecureChannel. The Session lifetime is typically longer than the
 * SecureChannel.
 *
 * The methods below are used to connect a client with a server. The supplied
 * arguments (for example the EndpointUrl or username/password) are first copied
 * into the client config. This is the same as modifying the client config first
 * and then connecting without these extra arguments.
 *
 * Once a connection is established, the client keeps the connection open and
 * reconnects if necessary. But as the client has no dedicated thread, it might
 * be necessary to call ``UA_Client_run_iterate`` in an interval for the
 * housekeeping tasks. Note normal Service-calls (like reading a value) also
 * triggers the scheduled house-keeping tasks. */

/* Copy the given endpointUrl into the configuration and connect. If the
 * endpointURL is NULL, then the client configuration is not modified. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_connect(UA_Client *client, const char *endpointUrl);

/* Connect to the server and create+activate a Session with the given username
 * and password. This first set the UserIdentityToken in the client config and
 * then calls the regular connect method. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_connectUsername(UA_Client *client, const char *endpointUrl,
                          const char *username, const char *password);

/* Connect to the server with a SecureChannel, but without creating a Session */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_connectSecureChannel(UA_Client *client, const char *endpointUrl);

/* Connect async (non-blocking) to the server. After initiating the connection,
 * call UA_Client_run_iterate repeatedly until the connection is fully
 * established. You can set a callback to client->config.stateCallback to be
 * notified when the connection status changes. Or use UA_Client_getState to get
 * the state manually. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_connectAsync(UA_Client *client, const char *endpointUrl);

/* Connect async to the server with a SecureChannel, but without creating a
 * Session */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_connectSecureChannelAsync(UA_Client *client, const char *endpointUrl);

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
UA_Client_startListeningForReverseConnect(UA_Client *client,
                                          const UA_String *listenHostnames,
                                          size_t listenHostnamesLength,
                                          UA_UInt16 port);

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
UA_Client_getSessionAuthenticationToken(UA_Client *client,
                                        UA_NodeId *authenticationToken,
                                        UA_ByteString *serverNonce);

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
 * The raw OPC UA services are exposed to the client. A Request-message
 * typically contains an array of operations to be performed by the server. The
 * convenience-methods from ``ua_client_highlevel.h`` are used to call
 * individual operations and are often easier to use. For performance reasons it
 * might however be better to request many operations in a single message.
 *
 * The raw Service-calls return the entire Response-message. Check the
 * ResponseHeader for the overall StatusCode. The operations within the
 * Service-call may return individual StatusCodes in addition.
 *
 * Note that some Services are not exposed here. For example, there is a
 * dedicated client API for Subscriptions and for Session management.
 *
 * Attribute Service Set
 * ~~~~~~~~~~~~~~~~~~~~~
 * This Service Set provides Services to access Attributes that are part of
 * Nodes. */

UA_ReadResponse UA_EXPORT UA_THREADSAFE
UA_Client_Service_read(UA_Client *client, const UA_ReadRequest req);

UA_WriteResponse UA_EXPORT UA_THREADSAFE
UA_Client_Service_write(UA_Client *client, const UA_WriteRequest req);

UA_HistoryReadResponse UA_EXPORT UA_THREADSAFE
UA_Client_Service_historyRead(UA_Client *client,
                              const UA_HistoryReadRequest req);

UA_HistoryUpdateResponse UA_EXPORT UA_THREADSAFE
UA_Client_Service_historyUpdate(UA_Client *client,
                                const UA_HistoryUpdateRequest req);

/**
 * Method Service Set
 * ~~~~~~~~~~~~~~~~~~
 * Methods represent the function calls of Objects. The Method Service Set
 * defines the means to invoke Methods. */

UA_CallResponse UA_EXPORT UA_THREADSAFE
UA_Client_Service_call(UA_Client *client, const UA_CallRequest req);

/**
 * NodeManagement Service Set
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~
 * This Service Set defines Services to add and delete AddressSpace Nodes and
 * References between them. */

UA_AddNodesResponse UA_EXPORT UA_THREADSAFE
UA_Client_Service_addNodes(UA_Client *client, const UA_AddNodesRequest req);

UA_AddReferencesResponse UA_EXPORT UA_THREADSAFE
UA_Client_Service_addReferences(UA_Client *client,
                                const UA_AddReferencesRequest req);

UA_DeleteNodesResponse UA_EXPORT UA_THREADSAFE
UA_Client_Service_deleteNodes(UA_Client *client,
                              const UA_DeleteNodesRequest req);

UA_DeleteReferencesResponse UA_EXPORT UA_THREADSAFE
UA_Client_Service_deleteReferences(UA_Client *client,
                                   const UA_DeleteReferencesRequest req);

/**
 * View Service Set
 * ~~~~~~~~~~~~~~~~
 * Clients use the browse Services of the View Service Set to navigate through
 * the AddressSpace or through a View which is a subset of the AddressSpace. */

UA_BrowseResponse UA_EXPORT UA_THREADSAFE
UA_Client_Service_browse(UA_Client *client, const UA_BrowseRequest req);

UA_BrowseNextResponse UA_EXPORT UA_THREADSAFE
UA_Client_Service_browseNext(UA_Client *client,
                             const UA_BrowseNextRequest req);

UA_TranslateBrowsePathsToNodeIdsResponse UA_EXPORT UA_THREADSAFE
UA_Client_Service_translateBrowsePathsToNodeIds(UA_Client *client,
    const UA_TranslateBrowsePathsToNodeIdsRequest req);

UA_RegisterNodesResponse UA_EXPORT UA_THREADSAFE
UA_Client_Service_registerNodes(UA_Client *client,
                                const UA_RegisterNodesRequest req);

UA_UnregisterNodesResponse UA_EXPORT UA_THREADSAFE
UA_Client_Service_unregisterNodes(UA_Client *client,
                                  const UA_UnregisterNodesRequest req);

/**
 * Query Service Set
 * ~~~~~~~~~~~~~~~~~
 * This Service Set is used to issue a Query to a Server. */

#ifdef UA_ENABLE_QUERY

UA_QueryFirstResponse UA_EXPORT UA_THREADSAFE
UA_Client_Service_queryFirst(UA_Client *client,
                             const UA_QueryFirstRequest req);

UA_QueryNextResponse UA_EXPORT UA_THREADSAFE
UA_Client_Service_queryNext(UA_Client *client,
                            const UA_QueryNextRequest req);

#endif

/**
 * Client Utility Functions
 * ------------------------ */

/* Lookup a datatype by its NodeId. Takes the custom types in the client
 * configuration into account. Return NULL if none found. */
UA_EXPORT const UA_DataType *
UA_Client_findDataType(UA_Client *client, const UA_NodeId *typeId);

/* The string is allocated and needs to be cleared */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Client_getNamespaceUri(UA_Client *client, UA_UInt16 index,
                          UA_String *nsUri);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Client_getNamespaceIndex(UA_Client *client, const UA_String nsUri,
                            UA_UInt16 *outIndex);

/* Returns the old index of the namespace already exists */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Client_addNamespace(UA_Client *client, const UA_String nsUri,
                       UA_UInt16 *outIndex);

/**
 * Connection Attributes
 * ~~~~~~~~~~~~~~~~~~~~~
 *
 * Besides the client configuration, some attributes of the connection are
 * defined only at runtime. For example the choice of SecurityPolicy or the
 * ApplicationDescripton from the server. This API allows to access such
 * connection attributes.
 *
 * The currently defined connection attributes are:
 *
 * - ``0:serverDescription`` (``UA_ApplicationDescription``): Server description
 * - ``0:securityPolicyUri`` (``UA_String``): Uri of the SecurityPolicy used
 * - ``0:securityMode`` (``UA_MessageSecurityMode``): SecurityMode of the SecureChannel
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
 * Timed Callbacks
 * ~~~~~~~~~~~~~~~
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

_UA_END_DECLS

#endif /* UA_CLIENT_H_ */
