/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2024 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2014-2015, 2017 (c) Florian Palm
 *    Copyright 2015-2016 (c) Chris Iatrou
 *    Copyright 2015-2016 (c) Oleksiy Vasylyev
 *    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Henrik Norrman
 *    Copyright 2018 (c) Fabian Arndt, Root-Core
 *    Copyright 2017-2020 (c) HMS Industrial Networks AB (Author: Jonas Green)
 *    Copyright 2020-2022 (c) Christian von Arnim, ISW University of Stuttgart  (for VDW and umati)
 */

#ifndef UA_SERVER_H_
#define UA_SERVER_H_

#include <open62541/types.h>
#include <open62541/common.h>
#include <open62541/util.h>

#include <open62541/plugin/log.h>
#include <open62541/plugin/certificategroup.h>
#include <open62541/plugin/nodestore.h>
#include <open62541/plugin/eventloop.h>
#include <open62541/plugin/accesscontrol.h>
#include <open62541/plugin/securitypolicy.h>

#include <open62541/client.h>

#ifdef UA_ENABLE_PUBSUB
#include <open62541/server_pubsub.h>
#endif

#ifdef UA_ENABLE_HISTORIZING
#include <open62541/plugin/historydatabase.h>
#endif

_UA_BEGIN_DECLS

/* Forward declarations */
struct UA_PubSubConfiguration;
typedef struct UA_PubSubConfiguration UA_PubSubConfiguration;
typedef void (*UA_Server_AsyncOperationNotifyCallback)(UA_Server *server);

/**
 * .. _server:
 *
 * Server
 * ======
 *
 * .. _server-configuration:
 *
 * Server Configuration
 * --------------------
 * The configuration structure is passed to the server during initialization.
 * The server expects that the configuration is not modified during runtime.
 * Currently, only one server can use a configuration at a time. During
 * shutdown, the server will clean up the parts of the configuration that are
 * modified at runtime through the provided API.
 *
 * Examples for configurations are provided in the ``/plugins`` folder.
 * The usual usage is as follows:
 *
 * 1. Create a server configuration with default settings as a starting point
 * 2. Modifiy the configuration, e.g. by adding a server certificate
 * 3. Instantiate a server with it
 * 4. After shutdown of the server, clean up the configuration (free memory)
 *
 * The :ref:`tutorials` provide a good starting point for this. */

struct UA_ServerConfig {
    void *context; /* Used to attach custom data to a server config. This can
                    * then be retrieved e.g. in a callback that forwards a
                    * pointer to the server. */
    UA_Logger *logging; /* Plugin for log output */

    /**
     * Server Description
     * ^^^^^^^^^^^^^^^^^^
     * The description must be internally consistent. The ApplicationUri set in
     * the ApplicationDescription must match the URI set in the server
     * certificate.
     * The applicationType is not just descriptive, it changes the actual
     * functionality of the server. The RegisterServer service is available only
     * if the server is a DiscoveryServer and the applicationType is set to the
     * appropriate value.*/
    UA_BuildInfo buildInfo;
    UA_ApplicationDescription applicationDescription;

    /**
     * Server Lifecycle
     * ^^^^^^^^^^^^^^^^ */
    /* Delay in ms from the shutdown signal (ctrl-c) until the actual shutdown.
     * Clients need to be able to get a notification ahead of time. */
    UA_Double shutdownDelay;

    /* If an asynchronous server shutdown is used, this callback notifies about
     * the current lifecycle state (notably the STOPPING -> STOPPED
     * transition). */
    void (*notifyLifecycleState)(UA_Server *server, UA_LifecycleState state);

    /**
     * Rule Handling
     * ^^^^^^^^^^^^^
     * Override the handling of standard-defined behavior. These settings are
     * used to balance the following contradicting requirements:
     *
     * - Strict conformance with the standard (for certification).
     * - Ensure interoperability with old/non-conforming implementations
     *   encountered in the wild.
     *
     * The defaults are set for compatibility with the largest number of OPC UA
     * vendors (with log warnings activated). Cf. Postel's Law "be conservative
     * in what you send, be liberal in what you accept".
     *
     * See the section :ref:`rule-handling` for the possible settings. */

    /* Verify that the server sends a timestamp in the request header */
    UA_RuleHandling verifyRequestTimestamp;

    /* Variables (that don't have a DataType of BaseDataType) must not have an
     * empty variant value. The default behaviour is to auto-create a matching
     * zeroed-out value for empty VariableNodes when they are added. */
    UA_RuleHandling allowEmptyVariables;

    UA_RuleHandling allowAllCertificateUris;

    /**
     * Custom Data Types
     * ^^^^^^^^^^^^^^^^^
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
     * .. note:: See the section on :ref:`generic-types`. Examples for working
     *    with custom data types are provided in
     *    ``/examples/custom_datatype/``. */

    /**
     * EventLoop
     * ^^^^^^^^^
     * The sever can be plugged into an external EventLoop. Otherwise the
     * EventLoop is considered to be attached to the server's lifecycle and will
     * be destroyed when the config is cleaned up. */
    UA_EventLoop *eventLoop;
    UA_Boolean externalEventLoop; /* The EventLoop is not deleted with the config */

    /**
     * Networking
     * ^^^^^^^^^^
     * The `severUrls` array contains the server URLs like
     * `opc.tcp://my-server:4840` or `opc.wss://localhost:443`. The URLs are
     * used both for discovery and to set up the server sockets based on the
     * defined hostnames (and ports).
     *
     * - If the list is empty: Listen on all network interfaces with TCP port 4840.
     * - If the hostname of a URL is empty: Use the define protocol and port and
     *   listen on all interfaces. */
    UA_String *serverUrls;
    size_t serverUrlsSize;

    /**
     * The following settings are specific to OPC UA with TCP transport. */
    UA_Boolean tcpEnabled;
    UA_UInt32 tcpBufSize;    /* Max length of sent and received chunks (packets)
                              * (default: 64kB) */
    UA_UInt32 tcpMaxMsgSize; /* Max length of messages
                              * (default: 0 -> unbounded) */
    UA_UInt32 tcpMaxChunks;  /* Max number of chunks per message
                              * (default: 0 -> unbounded) */
    UA_Boolean tcpReuseAddr;

    /**
     * Security and Encryption
     * ^^^^^^^^^^^^^^^^^^^^^^^ */
    size_t securityPoliciesSize;
    UA_SecurityPolicy* securityPolicies;

    /* Endpoints with combinations of SecurityPolicy and SecurityMode. If the
     * UserIdentityToken array of the Endpoint is not set, then it will be
     * filled by the server for all UserTokenPolicies that are configured in the
     * AccessControl plugin. */
    size_t endpointsSize;
    UA_EndpointDescription *endpoints;

    /* Only allow the following discovery services to be executed on a
     * SecureChannel with SecurityPolicyNone: GetEndpointsRequest,
     * FindServersRequest and FindServersOnNetworkRequest.
     *
     * Only enable this option if there is no endpoint with SecurityPolicy#None
     * in the endpoints list. The SecurityPolicy#None must be present in the
     * securityPolicies list. */
    UA_Boolean securityPolicyNoneDiscoveryOnly;

    /* Allow clients without encryption support to connect with username and password.
     * This requires to transmit the password in plain text over the network which is
     * why this option is disabled by default.
     * Make sure you really need this before enabling plain text passwords. */
    UA_Boolean allowNonePolicyPassword;

    /* Different sets of certificates are trusted for SecureChannel / Session */
    UA_CertificateGroup secureChannelPKI;
    UA_CertificateGroup sessionPKI;

    /**
     * See the section for :ref:`access-control
     * handling<access-control>`. */
    UA_AccessControl accessControl;

    /**
     * Nodes and Node Lifecycle
     * ^^^^^^^^^^^^^^^^^^^^^^^^
     * See the section for :ref:`node lifecycle handling<node-lifecycle>`. */
    UA_Nodestore nodestore;
    UA_GlobalNodeLifecycle nodeLifecycle;

    /**
     * Copy the HasModellingRule reference in instances from the type
     * definition in UA_Server_addObjectNode and UA_Server_addVariableNode.
     *
     * Part 3 - 6.4.4: [...] it is not required that newly created or referenced
     * instances based on InstanceDeclarations have a ModellingRule, however, it
     * is allowed that they have any ModellingRule independent of the
     * ModellingRule of their InstanceDeclaration */
    UA_Boolean modellingRulesOnInstances;

    /**
     * Limits
     * ^^^^^^ */
    /* Limits for SecureChannels */
    UA_UInt16 maxSecureChannels;
    UA_UInt32 maxSecurityTokenLifetime; /* in ms */

    /* Limits for Sessions */
    UA_UInt16 maxSessions;
    UA_Double maxSessionTimeout; /* in ms */

    /* Operation limits */
    UA_UInt32 maxNodesPerRead;
    UA_UInt32 maxNodesPerWrite;
    UA_UInt32 maxNodesPerMethodCall;
    UA_UInt32 maxNodesPerBrowse;
    UA_UInt32 maxNodesPerRegisterNodes;
    UA_UInt32 maxNodesPerTranslateBrowsePathsToNodeIds;
    UA_UInt32 maxNodesPerNodeManagement;
    UA_UInt32 maxMonitoredItemsPerCall;

    /* Limits for Requests */
    UA_UInt32 maxReferencesPerNode;

    /**
     * Async Operations
     * ^^^^^^^^^^^^^^^^
     * See the section for :ref:`async operations<async-operations>`. */
#if UA_MULTITHREADING >= 100
    UA_Double asyncOperationTimeout; /* in ms, 0 => unlimited */
    size_t maxAsyncOperationQueueSize; /* 0 => unlimited */
    /* Notify workers when an async operation was enqueued */
    UA_Server_AsyncOperationNotifyCallback asyncOperationNotifyCallback;
#endif

    /**
     * Discovery
     * ^^^^^^^^^ */
#ifdef UA_ENABLE_DISCOVERY
    /* Timeout in seconds when to automatically remove a registered server from
     * the list, if it doesn't re-register within the given time frame. A value
     * of 0 disables automatic removal. Default is 60 Minutes (60*60). Must be
     * bigger than 10 seconds, because cleanup is only triggered approximately
     * every 10 seconds. The server will still be removed depending on the
     * state of the semaphore file. */
    UA_UInt32 discoveryCleanupTimeout;

# ifdef UA_ENABLE_DISCOVERY_MULTICAST
    UA_Boolean mdnsEnabled;
    UA_MdnsDiscoveryConfiguration mdnsConfig;
    UA_String mdnsInterfaceIP;
#  if !defined(UA_HAS_GETIFADDR)
    size_t mdnsIpAddressListSize;
    UA_UInt32 *mdnsIpAddressList;
#  endif
# endif
#endif

    /**
     * Subscriptions
     * ^^^^^^^^^^^^^ */
    UA_Boolean subscriptionsEnabled;
#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* Limits for Subscriptions */
    UA_UInt32 maxSubscriptions;
    UA_UInt32 maxSubscriptionsPerSession;
    UA_DurationRange publishingIntervalLimits; /* in ms (must not be less than 5) */
    UA_UInt32Range lifeTimeCountLimits;
    UA_UInt32Range keepAliveCountLimits;
    UA_UInt32 maxNotificationsPerPublish;
    UA_Boolean enableRetransmissionQueue;
    UA_UInt32 maxRetransmissionQueueSize; /* 0 -> unlimited size */
# ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    UA_UInt32 maxEventsPerNode; /* 0 -> unlimited size */
# endif

    /* Limits for MonitoredItems */
    UA_UInt32 maxMonitoredItems;
    UA_UInt32 maxMonitoredItemsPerSubscription;
    UA_DurationRange samplingIntervalLimits; /* in ms (must not be less than 5) */
    UA_UInt32Range queueSizeLimits; /* Negotiated with the client */

    /* Limits for PublishRequests */
    UA_UInt32 maxPublishReqPerSession;

    /* Register MonitoredItem in Userland
     *
     * @param server Allows the access to the server object
     * @param sessionId The session id, represented as an node id
     * @param sessionContext An optional pointer to user-defined data for the
     *        specific data source
     * @param nodeid Id of the node in question
     * @param nodeidContext An optional pointer to user-defined data, associated
     *        with the node in the nodestore. Note that, if the node has already
     *        been removed, this value contains a NULL pointer.
     * @param attributeId Identifies which attribute (value, data type etc.) is
     *        monitored
     * @param removed Determines if the MonitoredItem was removed or created. */
    void (*monitoredItemRegisterCallback)(UA_Server *server,
                                          const UA_NodeId *sessionId,
                                          void *sessionContext,
                                          const UA_NodeId *nodeId,
                                          void *nodeContext,
                                          UA_UInt32 attibuteId,
                                          UA_Boolean removed);
#endif

    /**
     * PubSub
     * ^^^^^^ */
    UA_Boolean pubsubEnabled;
#ifdef UA_ENABLE_PUBSUB
    UA_PubSubConfiguration pubSubConfig;
#endif

    /**
     * Historical Access
     * ^^^^^^^^^^^^^^^^^ */
    UA_Boolean historizingEnabled;
#ifdef UA_ENABLE_HISTORIZING
    UA_HistoryDatabase historyDatabase;

    UA_Boolean accessHistoryDataCapability;
    UA_UInt32  maxReturnDataValues; /* 0 -> unlimited size */

    UA_Boolean accessHistoryEventsCapability;
    UA_UInt32  maxReturnEventValues; /* 0 -> unlimited size */

    UA_Boolean insertDataCapability;
    UA_Boolean insertEventCapability;
    UA_Boolean insertAnnotationsCapability;

    UA_Boolean replaceDataCapability;
    UA_Boolean replaceEventCapability;

    UA_Boolean updateDataCapability;
    UA_Boolean updateEventCapability;

    UA_Boolean deleteRawCapability;
    UA_Boolean deleteEventCapability;
    UA_Boolean deleteAtTimeDataCapability;
#endif

    /**
     * Reverse Connect
     * ^^^^^^^^^^^^^^^ */
    UA_UInt32 reverseReconnectInterval; /* Default is 15000 ms */

    /**
     * Certificate Password Callback
     * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
#ifdef UA_ENABLE_ENCRYPTION
    /* If the private key is in PEM format and password protected, this callback
     * is called during initialization to get the password to decrypt the
     * private key. The memory containing the password is freed by the client
     * after use. The callback should be set early, other parts of the client
     * config setup may depend on it. */
    UA_StatusCode (*privateKeyPasswordCallback)(UA_ServerConfig *sc,
                                                UA_ByteString *password);
#endif
};

void UA_EXPORT
UA_ServerConfig_clean(UA_ServerConfig *config);

/**
 * .. _server-lifecycle:
 *
 * Server Lifecycle
 * ---------------- */

/* Create a new server with a default configuration that adds plugins for
 * networking, security, logging and so on. See `server_config_default.h` for
 * more detailed options.
 *
 * The default configuration can be used as the starting point to adjust the
 * server configuration to individual needs. UA_Server_new is implemented in the
 * /plugins folder under the CC0 license. Furthermore the server confiugration
 * only uses the public server API.
 *
 * @return Returns the configured server or NULL if an error occurs. */
UA_EXPORT UA_Server *
UA_Server_new(void);

/* Creates a new server. Moves the config into the server with a shallow copy.
 * The config content is cleared together with the server. */
UA_EXPORT UA_Server *
UA_Server_newWithConfig(UA_ServerConfig *config);

/* Delete the server. */
UA_EXPORT UA_StatusCode
UA_Server_delete(UA_Server *server);

/* Get the configuration. Always succeeds as this simplfy resolves a pointer.
 * Attention! Do not adjust the configuration while the server is running! */
UA_EXPORT UA_ServerConfig *
UA_Server_getConfig(UA_Server *server);

/* Get the current server lifecycle state */
UA_EXPORT UA_LifecycleState
UA_Server_getLifecycleState(UA_Server *server);

/* Runs the server until interrupted. On Unix/Windows this registers an
 * interrupt for SIGINT (ctrl-c). The method only returns after having received
 * the interrupt. The logical sequence is as follows:
 *
 * - UA_Server_run_startup
 * - Loop until interrupt: UA_Server_run_iterate
 * - UA_Server_run_shutdown
 *
 * @param server The server object.
 * @return Returns a bad statuscode if an error occurred internally. */
UA_EXPORT UA_StatusCode
UA_Server_run(UA_Server *server, const volatile UA_Boolean *running);

/* Runs the server until interrupted. On Unix/Windows this registers an
 * interrupt for SIGINT (ctrl-c). The method only returns after having received
 * the interrupt or upon an error condition. The logical sequence is as follows:
 *
 * - Register the interrupt
 * - UA_Server_run_startup
 * - Loop until interrupt: UA_Server_run_iterate
 * - UA_Server_run_shutdown
 * - Deregister the interrupt
 *
 * Attention! This method is implemented individually for the different
 * platforms (POSIX/Win32/etc.). The default implementation is in
 * /plugins/ua_config_default.c under the CC0 license. Adjust as needed.
 *
 * @param server The server object.
 * @return Returns a bad statuscode if an error occurred internally. */
UA_EXPORT UA_StatusCode
UA_Server_runUntilInterrupt(UA_Server *server);

/* The prologue part of UA_Server_run (no need to use if you call
 * UA_Server_run or UA_Server_runUntilInterrupt) */
UA_EXPORT UA_StatusCode
UA_Server_run_startup(UA_Server *server);

/* Executes a single iteration of the server's main loop.
 *
 * @param server The server object.
 * @param waitInternal Should we wait for messages in the networklayer?
 *        Otherwise, the timeouts for the networklayers are set to zero.
 *        The default max wait time is 200ms.
 * @return Returns how long we can wait until the next scheduled
 *         callback (in ms) */
UA_EXPORT UA_UInt16
UA_Server_run_iterate(UA_Server *server, UA_Boolean waitInternal);

/* The epilogue part of UA_Server_run (no need to use if you call
 * UA_Server_run or UA_Server_runUntilInterrupt) */
UA_EXPORT UA_StatusCode
UA_Server_run_shutdown(UA_Server *server);

/**
 * Timed Callbacks
 * ---------------
 * Add a callback to the server that is executed at a defined time.
 * The callback can also be registered with a cyclic interval. */

/* Add a callback for execution at a specified time. If the indicated time lies
 * in the past, then the callback is executed at the next iteration of the
 * server's main loop.
 *
 * @param server The server object.
 * @param callback The callback that shall be added.
 * @param data Data that is forwarded to the callback.
 * @param date The timestamp for the execution time.
 * @param callbackId Set to the identifier of the repeated callback . This can
 *        be used to cancel the callback later on. If the pointer is null, the
 *        identifier is not set.
 * @return Upon success, ``UA_STATUSCODE_GOOD`` is returned. An error code
 *         otherwise. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_addTimedCallback(UA_Server *server, UA_ServerCallback callback,
                           void *data, UA_DateTime date, UA_UInt64 *callbackId);

/* Add a callback for cyclic repetition to the server.
 *
 * @param server The server object.
 * @param callback The callback that shall be added.
 * @param data Data that is forwarded to the callback.
 * @param interval_ms The callback shall be repeatedly executed with the given
 *        interval (in ms). The interval must be positive. The first execution
 *        occurs at now() + interval at the latest.
 * @param callbackId Set to the identifier of the repeated callback . This can
 *        be used to cancel the callback later on. If the pointer is null, the
 *        identifier is not set.
 * @return Upon success, ``UA_STATUSCODE_GOOD`` is returned. An error code
 *         otherwise. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_addRepeatedCallback(UA_Server *server, UA_ServerCallback callback,
                              void *data, UA_Double interval_ms,
                              UA_UInt64 *callbackId);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_changeRepeatedCallbackInterval(UA_Server *server, UA_UInt64 callbackId,
                                         UA_Double interval_ms);

/* Remove a repeated callback. Does nothing if the callback is not found.
 *
 * @param server The server object.
 * @param callbackId The id of the callback */
void UA_EXPORT UA_THREADSAFE
UA_Server_removeCallback(UA_Server *server, UA_UInt64 callbackId);

#define UA_Server_removeRepeatedCallback(server, callbackId) \
    UA_Server_removeCallback(server, callbackId);

/**
 * Session Handling
 * ----------------
 * A new session is announced via the AccessControl plugin. The session
 * identifier is forwarded to the relevant callbacks back into userland. The
 * following methods enable an interaction with a particular session. */

/* Manually close a session */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_closeSession(UA_Server *server, const UA_NodeId *sessionId);

/**
 * Session attributes: Besides the user-definable session context pointer (set
 * by the AccessControl plugin when the Session is created), a session carries
 * attributes in a key-value list. Some attributes are present in every session
 * and shown in the list below. Additional attributes can be manually set as
 * meta-data.
 *
 * Always present as session attributes are:
 *
 * - 0:localeIds [UA_String]: List of preferred languages (read-only)
 * - 0:clientDescription [UA_ApplicationDescription]: Client description (read-only)
 * - 0:sessionName [String] Client-defined name of the session (read-only)
 * - 0:clientUserId [String] User identifier used to activate the session (read-only) */

/* Returns a shallow copy of the attribute. Don't _clear or _delete the value
 * variant. Don't use the value once the Session could be already closed in the
 * background or the attribute of the session replaced. Hence don't use this in a
 * multi-threaded application. */
UA_EXPORT UA_StatusCode
UA_Server_getSessionAttribute(UA_Server *server, const UA_NodeId *sessionId,
                              const UA_QualifiedName key, UA_Variant *outValue);

/* Return a deep copy of the attribute */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_getSessionAttributeCopy(UA_Server *server, const UA_NodeId *sessionId,
                                  const UA_QualifiedName key, UA_Variant *outValue);

/* Returns NULL if the attribute is not defined or not a scalar or not of the
 * right datatype. Otherwise a shallow copy of the scalar value is created at
 * the target location of the void pointer. Hence don't use this in a
 * multi-threaded application. */
UA_EXPORT UA_StatusCode
UA_Server_getSessionAttribute_scalar(UA_Server *server,
                                     const UA_NodeId *sessionId,
                                     const UA_QualifiedName key,
                                     const UA_DataType *type,
                                     void *outValue);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_setSessionAttribute(UA_Server *server, const UA_NodeId *sessionId,
                              const UA_QualifiedName key,
                              const UA_Variant *value);

UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_deleteSessionAttribute(UA_Server *server, const UA_NodeId *sessionId,
                                 const UA_QualifiedName key);

/**
 * Reading and Writing Node Attributes
 * -----------------------------------
 * The functions for reading and writing node attributes call the regular read
 * and write service in the background that are also used over the network.
 *
 * The following attributes cannot be read, since the local "admin" user always
 * has full rights.
 *
 * - UserWriteMask
 * - UserAccessLevel
 * - UserExecutable */

/* Read an attribute of a node. The specialized functions below provide a more
 * concise syntax.
 *
 * @param server The server object.
 * @param item ReadValueIds contain the NodeId of the target node, the id of the
 *             attribute to read and (optionally) an index range to read parts
 *             of an array only. See the section on NumericRange for the format
 *             used for array ranges.
 * @param timestamps Which timestamps to return for the attribute.
 * @return Returns a DataValue that contains either an error code, or a variant
 *         with the attribute value and the timestamps. */
UA_DataValue UA_EXPORT UA_THREADSAFE
UA_Server_read(UA_Server *server, const UA_ReadValueId *item,
               UA_TimestampsToReturn timestamps);

/* Don't use this function. There are typed versions for every supported
 * attribute. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
__UA_Server_read(UA_Server *server, const UA_NodeId *nodeId,
                 UA_AttributeId attributeId, void *v);

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_readNodeId(UA_Server *server, const UA_NodeId nodeId,
                     UA_NodeId *outNodeId) ,{
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_NODEID, outNodeId);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_readNodeClass(UA_Server *server, const UA_NodeId nodeId,
                        UA_NodeClass *outNodeClass) ,{
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_NODECLASS,
                            outNodeClass);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_readBrowseName(UA_Server *server, const UA_NodeId nodeId,
                         UA_QualifiedName *outBrowseName) ,{
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_BROWSENAME,
                            outBrowseName);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_readDisplayName(UA_Server *server, const UA_NodeId nodeId,
                          UA_LocalizedText *outDisplayName) ,{
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_DISPLAYNAME,
                            outDisplayName);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_readDescription(UA_Server *server, const UA_NodeId nodeId,
                          UA_LocalizedText *outDescription) ,{
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_DESCRIPTION,
                            outDescription);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_readWriteMask(UA_Server *server, const UA_NodeId nodeId,
                        UA_UInt32 *outWriteMask) ,{
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_WRITEMASK,
                            outWriteMask);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_readIsAbstract(UA_Server *server, const UA_NodeId nodeId,
                         UA_Boolean *outIsAbstract) ,{
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_ISABSTRACT,
                            outIsAbstract);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_readSymmetric(UA_Server *server, const UA_NodeId nodeId,
                        UA_Boolean *outSymmetric) ,{
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_SYMMETRIC,
                            outSymmetric);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_readInverseName(UA_Server *server, const UA_NodeId nodeId,
                          UA_LocalizedText *outInverseName) ,{
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_INVERSENAME,
                            outInverseName);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_readContainsNoLoops(UA_Server *server, const UA_NodeId nodeId,
                              UA_Boolean *outContainsNoLoops) ,{
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_CONTAINSNOLOOPS,
                            outContainsNoLoops);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_readEventNotifier(UA_Server *server, const UA_NodeId nodeId,
                            UA_Byte *outEventNotifier) ,{
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_EVENTNOTIFIER,
                            outEventNotifier);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_readValue(UA_Server *server, const UA_NodeId nodeId,
                    UA_Variant *outValue) ,{
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_VALUE, outValue);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_readDataType(UA_Server *server, const UA_NodeId nodeId,
                       UA_NodeId *outDataType) ,{
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_DATATYPE,
                            outDataType);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_readValueRank(UA_Server *server, const UA_NodeId nodeId,
                        UA_Int32 *outValueRank) ,{
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_VALUERANK,
                            outValueRank);
})

/* Returns a variant with an int32 array */
UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_readArrayDimensions(UA_Server *server, const UA_NodeId nodeId,
                              UA_Variant *outArrayDimensions) ,{
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_ARRAYDIMENSIONS,
                            outArrayDimensions);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_readAccessLevel(UA_Server *server, const UA_NodeId nodeId,
                          UA_Byte *outAccessLevel) ,{
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_ACCESSLEVEL,
                            outAccessLevel);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_readAccessLevelEx(UA_Server *server, const UA_NodeId nodeId,
                            UA_UInt32 *outAccessLevelEx), {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_ACCESSLEVELEX,
                            outAccessLevelEx);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_readMinimumSamplingInterval(UA_Server *server, const UA_NodeId nodeId,
                                      UA_Double *outMinimumSamplingInterval) ,{
    return __UA_Server_read(server, &nodeId,
                            UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL,
                            outMinimumSamplingInterval);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_readHistorizing(UA_Server *server, const UA_NodeId nodeId,
                          UA_Boolean *outHistorizing) ,{
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_HISTORIZING,
                            outHistorizing);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_readExecutable(UA_Server *server, const UA_NodeId nodeId,
                         UA_Boolean *outExecutable) ,{
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_EXECUTABLE,
                            outExecutable);
})

/**
 * The following node attributes cannot be changed once a node has been created:
 *
 * - NodeClass
 * - NodeId
 * - Symmetric
 * - ContainsNoLoops
 *
 * The following attributes cannot be written from the server, as they are
 * specific to the different users and set by the access control callback:
 *
 * - UserWriteMask
 * - UserAccessLevel
 * - UserExecutable
 */

/* Overwrite an attribute of a node. The specialized functions below provide a
 * more concise syntax.
 *
 * @param server The server object.
 * @param value WriteValues contain the NodeId of the target node, the id of the
 *              attribute to overwritten, the actual value and (optionally) an
 *              index range to replace parts of an array only. of an array only.
 *              See the section on NumericRange for the format used for array
 *              ranges.
 * @return Returns a status code. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_write(UA_Server *server, const UA_WriteValue *value);

/* Don't use this function. There are typed versions with no additional
 * overhead. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
__UA_Server_write(UA_Server *server, const UA_NodeId *nodeId,
                  const UA_AttributeId attributeId,
                  const UA_DataType *attr_type, const void *attr);

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_writeBrowseName(UA_Server *server, const UA_NodeId nodeId,
                          const UA_QualifiedName browseName) ,{
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_BROWSENAME,
                             &UA_TYPES[UA_TYPES_QUALIFIEDNAME], &browseName);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_writeDisplayName(UA_Server *server, const UA_NodeId nodeId,
                           const UA_LocalizedText displayName) ,{
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_DISPLAYNAME,
                             &UA_TYPES[UA_TYPES_LOCALIZEDTEXT], &displayName);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_writeDescription(UA_Server *server, const UA_NodeId nodeId,
                           const UA_LocalizedText description) ,{
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_DESCRIPTION,
                             &UA_TYPES[UA_TYPES_LOCALIZEDTEXT], &description);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_writeWriteMask(UA_Server *server, const UA_NodeId nodeId,
                         const UA_UInt32 writeMask) ,{
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_WRITEMASK,
                             &UA_TYPES[UA_TYPES_UINT32], &writeMask);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_writeIsAbstract(UA_Server *server, const UA_NodeId nodeId,
                          const UA_Boolean isAbstract) ,{
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_ISABSTRACT,
                             &UA_TYPES[UA_TYPES_BOOLEAN], &isAbstract);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_writeInverseName(UA_Server *server, const UA_NodeId nodeId,
                           const UA_LocalizedText inverseName) ,{
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_INVERSENAME,
                             &UA_TYPES[UA_TYPES_LOCALIZEDTEXT], &inverseName);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_writeEventNotifier(UA_Server *server, const UA_NodeId nodeId,
                             const UA_Byte eventNotifier) ,{
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_EVENTNOTIFIER,
                             &UA_TYPES[UA_TYPES_BYTE], &eventNotifier);
})

/**
 * Writes an UA_Variant to a variable/variableType node.
 * StatusCode is set to ``UA_STATUSCODE_GOOD``, sourceTimestamp and
 * serverTimestamp are set to UA_DateTime_now()
 */
UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_writeValue(UA_Server *server, const UA_NodeId nodeId,
                     const UA_Variant value) ,{
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_VALUE,
                             &UA_TYPES[UA_TYPES_VARIANT], &value);
})

/**
 * Writes an UA_DataValue to a variable/variableType node.
 * In contrast to UA_Server_writeValue, this functions can also write
 * sourceTimestamp, serverTimestamp and statusCode.
 */
UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_writeDataValue(UA_Server *server, const UA_NodeId nodeId,
                     const UA_DataValue value) ,{
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_VALUE,
                             &UA_TYPES[UA_TYPES_DATAVALUE], &value);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_writeDataType(UA_Server *server, const UA_NodeId nodeId,
                        const UA_NodeId dataType) ,{
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_DATATYPE,
                             &UA_TYPES[UA_TYPES_NODEID], &dataType);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_writeValueRank(UA_Server *server, const UA_NodeId nodeId,
                         const UA_Int32 valueRank) ,{
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_VALUERANK,
                             &UA_TYPES[UA_TYPES_INT32], &valueRank);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_writeArrayDimensions(UA_Server *server, const UA_NodeId nodeId,
                               const UA_Variant arrayDimensions) ,{
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_ARRAYDIMENSIONS,
                             &UA_TYPES[UA_TYPES_VARIANT], &arrayDimensions);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_writeAccessLevel(UA_Server *server, const UA_NodeId nodeId,
                           const UA_Byte accessLevel) ,{
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_ACCESSLEVEL,
                             &UA_TYPES[UA_TYPES_BYTE], &accessLevel);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_writeAccessLevelEx(UA_Server *server, const UA_NodeId nodeId,
                             const UA_UInt32 accessLevelEx), {
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_ACCESSLEVELEX,
                             &UA_TYPES[UA_TYPES_UINT32], &accessLevelEx);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_writeMinimumSamplingInterval(UA_Server *server, const UA_NodeId nodeId,
                                       const UA_Double miniumSamplingInterval) ,{
    return __UA_Server_write(server, &nodeId,
                             UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL,
                             &UA_TYPES[UA_TYPES_DOUBLE],
                             &miniumSamplingInterval);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_writeHistorizing(UA_Server *server, const UA_NodeId nodeId,
                          const UA_Boolean historizing) ,{
    return __UA_Server_write(server, &nodeId,
                             UA_ATTRIBUTEID_HISTORIZING,
                             &UA_TYPES[UA_TYPES_BOOLEAN],
                             &historizing);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_writeExecutable(UA_Server *server, const UA_NodeId nodeId,
                          const UA_Boolean executable) ,{
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_EXECUTABLE,
                             &UA_TYPES[UA_TYPES_BOOLEAN], &executable); 
})

/**
 * Browsing
 * -------- */

/* Browse the references of a particular node. See the definition of
 * BrowseDescription structure for details. */
UA_BrowseResult UA_EXPORT UA_THREADSAFE
UA_Server_browse(UA_Server *server, UA_UInt32 maxReferences,
                 const UA_BrowseDescription *bd);

UA_BrowseResult UA_EXPORT UA_THREADSAFE
UA_Server_browseNext(UA_Server *server, UA_Boolean releaseContinuationPoint,
                     const UA_ByteString *continuationPoint);

/* Non-standard version of the Browse service that recurses into child nodes.
 *
 * Possible loops (that can occur for non-hierarchical references) are handled
 * internally. Every node is added at most once to the results array.
 *
 * Nodes are only added if they match the NodeClassMask in the
 * BrowseDescription. However, child nodes are still recursed into if the
 * NodeClass does not match. So it is possible, for example, to get all
 * VariableNodes below a certain ObjectNode, with additional objects in the
 * hierarchy below. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_browseRecursive(UA_Server *server, const UA_BrowseDescription *bd,
                          size_t *resultsSize, UA_ExpandedNodeId **results);

UA_BrowsePathResult UA_EXPORT UA_THREADSAFE
UA_Server_translateBrowsePathToNodeIds(UA_Server *server,
                                       const UA_BrowsePath *browsePath);

/* A simplified TranslateBrowsePathsToNodeIds based on the
 * SimpleAttributeOperand type (Part 4, 7.4.4.5).
 *
 * This specifies a relative path using a list of BrowseNames instead of the
 * RelativePath structure. The list of BrowseNames is equivalent to a
 * RelativePath that specifies forward references which are subtypes of the
 * HierarchicalReferences ReferenceType. All Nodes followed by the browsePath
 * shall be of the NodeClass Object or Variable. */
UA_BrowsePathResult UA_EXPORT UA_THREADSAFE
UA_Server_browseSimplifiedBrowsePath(UA_Server *server, const UA_NodeId origin,
                                     size_t browsePathSize,
                                     const UA_QualifiedName *browsePath);

#ifndef HAVE_NODEITER_CALLBACK
#define HAVE_NODEITER_CALLBACK
/* Iterate over all nodes referenced by parentNodeId by calling the callback
 * function for each child node (in ifdef because GCC/CLANG handle include order
 * differently) */
typedef UA_StatusCode
(*UA_NodeIteratorCallback)(UA_NodeId childId, UA_Boolean isInverse,
                           UA_NodeId referenceTypeId, void *handle);
#endif

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_forEachChildNodeCall(UA_Server *server, UA_NodeId parentNodeId,
                               UA_NodeIteratorCallback callback, void *handle);

#ifdef UA_ENABLE_DISCOVERY

/**
 * Discovery
 * ---------
 *
 * Registering at a Discovery Server
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */

/* Register the given server instance at the discovery server. This should be
 * called periodically, for example every 10 minutes, depending on the
 * configuration of the discovery server. You should also call
 * _unregisterDiscovery when the server shuts down.
 *
 * The supplied client configuration is used to create a new client to connect
 * to the discovery server. The client configuration is moved over to the server
 * and eventually cleaned up internally. The structure pointed at by `cc` is
 * zeroed to avoid accessing outdated information.
 *
 * The eventloop and logging plugins in the client configuration are replaced by
 * those configured in the server. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_registerDiscovery(UA_Server *server, UA_ClientConfig *cc,
                            const UA_String discoveryServerUrl,
                            const UA_String semaphoreFilePath);

/* Deregister the given server instance from the discovery server.
 * This should be called when the server is shutting down. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_deregisterDiscovery(UA_Server *server, UA_ClientConfig *cc,
                              const UA_String discoveryServerUrl);

/**
 * Operating a Discovery Server
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */

/* Callback for RegisterServer. Data is passed from the register call */
typedef void
(*UA_Server_registerServerCallback)(const UA_RegisteredServer *registeredServer,
                                    void* data);

/* Set the callback which is called if another server registeres or unregisters
 * with this instance. This callback is called every time the server gets a
 * register call. This especially means that for every periodic server register
 * the callback will be called.
 *
 * @param server
 * @param cb the callback
 * @param data data passed to the callback
 * @return ``UA_STATUSCODE_SUCCESS`` on success */
void UA_EXPORT UA_THREADSAFE
UA_Server_setRegisterServerCallback(UA_Server *server,
                                    UA_Server_registerServerCallback cb, void* data);

#ifdef UA_ENABLE_DISCOVERY_MULTICAST

/* Callback for server detected through mDNS. Data is passed from the register
 * call
 *
 * @param isServerAnnounce indicates if the server has just been detected. If
 *        set to false, this means the server is shutting down.
 * @param isTxtReceived indicates if we already received the corresponding TXT
 *        record with the path and caps data */
typedef void
(*UA_Server_serverOnNetworkCallback)(const UA_ServerOnNetwork *serverOnNetwork,
                                     UA_Boolean isServerAnnounce,
                                     UA_Boolean isTxtReceived, void* data);

/* Set the callback which is called if another server is found through mDNS or
 * deleted. It will be called for any mDNS message from the remote server, thus
 * it may be called multiple times for the same instance. Also the SRV and TXT
 * records may arrive later, therefore for the first call the server
 * capabilities may not be set yet. If called multiple times, previous data will
 * be overwritten.
 *
 * @param server
 * @param cb the callback
 * @param data data passed to the callback
 * @return ``UA_STATUSCODE_SUCCESS`` on success */
void UA_EXPORT UA_THREADSAFE
UA_Server_setServerOnNetworkCallback(UA_Server *server,
                                     UA_Server_serverOnNetworkCallback cb,
                                     void* data);

#endif /* UA_ENABLE_DISCOVERY_MULTICAST */

#endif /* UA_ENABLE_DISCOVERY */

/**
 * Information Model Callbacks
 * ---------------------------
 * There are three places where a callback from an information model to
 * user-defined code can happen.
 *
 * - Custom node constructors and destructors
 * - Linking VariableNodes with an external data source
 * - MethodNode callbacks */

void UA_EXPORT
UA_Server_setAdminSessionContext(UA_Server *server,
                                 void *context);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_setNodeTypeLifecycle(UA_Server *server, UA_NodeId nodeId,
                               UA_NodeTypeLifecycle lifecycle);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_getNodeContext(UA_Server *server, UA_NodeId nodeId,
                         void **nodeContext);

/* Careful! The user has to ensure that the destructor callbacks still work. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_setNodeContext(UA_Server *server, UA_NodeId nodeId,
                         void *nodeContext);

/**
 * .. _datasource:
 *
 * Data Source Callback
 * ^^^^^^^^^^^^^^^^^^^^
 * The server has a unique way of dealing with the content of variables. Instead
 * of storing a variant attached to the variable node, the node can point to a
 * function with a local data provider. Whenever the value attribute is read,
 * the function will be called and asked to provide a UA_DataValue return value
 * that contains the value content and additional timestamps.
 *
 * It is expected that the read callback is implemented. The write callback can
 * be set to a null-pointer. */

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_setVariableNode_dataSource(UA_Server *server, const UA_NodeId nodeId,
                                     const UA_DataSource dataSource);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_setVariableNode_valueCallback(UA_Server *server,
                                        const UA_NodeId nodeId,
                                        const UA_ValueCallback callback);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_setVariableNode_valueBackend(UA_Server *server,
                                       const UA_NodeId nodeId,
                                       const UA_ValueBackend valueBackend);

/**
 * .. _local-monitoreditems:
 *
 * Local MonitoredItems
 * ^^^^^^^^^^^^^^^^^^^^
 * MonitoredItems are used with the Subscription mechanism of OPC UA to
 * transported notifications for data changes and events. MonitoredItems can
 * also be registered locally. Notifications are then forwarded to a
 * user-defined callback instead of a remote client.
 *
 * Local MonitoredItems are delivered asynchronously. That is, the notification
 * is inserted as a *Delayed Callback* for the EventLoop. The callback is then
 * triggered when the control flow next returns to the EventLoop. */

#ifdef UA_ENABLE_SUBSCRIPTIONS

/* Delete a local MonitoredItem. Used for both DataChange- and
 * Event-MonitoredItems. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_deleteMonitoredItem(UA_Server *server, UA_UInt32 monitoredItemId);

typedef void (*UA_Server_DataChangeNotificationCallback)
    (UA_Server *server, UA_UInt32 monitoredItemId, void *monitoredItemContext,
     const UA_NodeId *nodeId, void *nodeContext, UA_UInt32 attributeId,
     const UA_DataValue *value);

/**
 * DataChange MonitoredItem use a sampling interval and filter criteria to
 * notify the userland about value changes. Note that the sampling interval can
 * also be zero to be notified about changes "right away". For this we hook the
 * MonitoredItem into the observed Node and check the filter after every call of
 * the Write-Service. */

/* Create a local MonitoredItem to detect data changes.
 *
 * @param server The server executing the MonitoredItem
 * @param timestampsToReturn Shall timestamps be added to the value for the
 *        callback?
 * @param item The parameters of the new MonitoredItem. Note that the attribute
 *        of the ReadValueId (the node that is monitored) can not be
 *        ``UA_ATTRIBUTEID_EVENTNOTIFIER``. See below for event notifications.
 * @param monitoredItemContext A pointer that is forwarded with the callback
 * @param callback The callback that is executed on detected data changes
 * @return Returns a description of the created MonitoredItem. The structure
 *         also contains a StatusCode (in case of an error) and the identifier
 *         of the new MonitoredItem. */
UA_MonitoredItemCreateResult UA_EXPORT UA_THREADSAFE
UA_Server_createDataChangeMonitoredItem(UA_Server *server,
          UA_TimestampsToReturn timestampsToReturn,
          const UA_MonitoredItemCreateRequest item,
          void *monitoredItemContext,
          UA_Server_DataChangeNotificationCallback callback);

/**
 * See the section on :ref`events` for how to emit events in the server.
 *
 * Event-MonitoredItems emit notifications with a list of "fields" (variants).
 * The fields are specified as *SimpleAttributeOperands* in the select-clause of
 * the MonitoredItem's event filter. For the local event callback, instead of
 * using a list of variants, we use a key-value map for the event fields. They
 * key names are generated with ``UA_SimpleAttributeOperand_print`` to get a
 * human-readable representation.
 *
 * The received event-fields map could look like this::
 *
 *   0:/Severity   => UInt16(1000)
 *   0:/Message    => LocalizedText("en-US", "My Event Message")
 *   0:/EventType  => NodeId(i=50831)
 *   0:/SourceNode => NodeId(i=2253)
 *
 * The order of the keys is identical to the order of SimpleAttributeOperands in
 * the select-clause. This feature requires the build flag ``UA_ENABLE_PARSING``
 * enabled. Otherwise the key-value map uses empty keys (the order of fields is
 * still the same as the specified select-clauses). */

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS

typedef void (*UA_Server_EventNotificationCallback)
    (UA_Server *server, UA_UInt32 monitoredItemId, void *monitoredItemContext,
     const UA_KeyValueMap eventFields);

/* Create a local MonitoredItem for Events. The API is simplifed compared to a
 * UA_MonitoredItemCreateRequest. The unavailable options are not relevant for
 * local MonitoredItems (e.g. the queue size) or not relevant for Event
 * MonitoredItems (e.g. the sampling interval).
 *
 * @param server The server executing the MonitoredItem
 * @param nodeId The node where events are collected. Note that events "bubble
 *        up" to their parents (via hierarchical references).
 * @param filter The filter defined which event fields are selected (select
 *        clauses) and which events are considered for this particular
 *        MonitoredItem (where clause).
 * @param monitoredItemContext A pointer that is forwarded with the callback
 * @param callback The callback that is executed for each event
 * @return Returns a description of the created MonitoredItem. The structure
 *         also contains a StatusCode (in case of an error) and the identifier
 *         of the new MonitoredItem. */
UA_MonitoredItemCreateResult UA_EXPORT UA_THREADSAFE
UA_Server_createEventMonitoredItem(UA_Server *server, const UA_NodeId nodeId,
                                   const UA_EventFilter filter,
                                   void *monitoredItemContext,
                                   UA_Server_EventNotificationCallback callback);

/* Extended version UA_Server_createEventMonitoredItem that allows setting of
 * uncommon parameters (for local MonitoredItems) like the MonitoringMode and
 * queue sizes.
 *
 * @param server The server executing the MonitoredItem
 * @param item The description of the MonitoredItem. Must use
 *        UA_ATTRIBUTEID_EVENTNOTIFIER and an EventFilter.
 * @param monitoredItemContext A pointer that is forwarded with the callback
 * @param callback The callback that is executed for each event
 * @return Returns a description of the created MonitoredItem. The structure
 *         also contains a StatusCode (in case of an error) and the identifier
 *         of the new MonitoredItem. */
UA_MonitoredItemCreateResult UA_EXPORT UA_THREADSAFE
UA_Server_createEventMonitoredItemEx(UA_Server *server,
                                     const UA_MonitoredItemCreateRequest item,
                                     void *monitoredItemContext,
                                     UA_Server_EventNotificationCallback callback);

#endif

#endif

/**
 * Method Callbacks
 * ^^^^^^^^^^^^^^^^
 * Method callbacks are set to `NULL` (not executable) when a method node is
 * added over the network. In theory, it is possible to add a callback via
 * ``UA_Server_setMethodNode_callback`` within the global constructor when
 * adding methods over the network is really wanted. See the Section
 * :ref:`object-interaction` for calling methods on an object. */

#ifdef UA_ENABLE_METHODCALLS
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_setMethodNodeCallback(UA_Server *server,
                                const UA_NodeId methodNodeId,
                                UA_MethodCallback methodCallback);

/* Backwards compatibility definition */
#define UA_Server_setMethodNode_callback(server, methodNodeId, methodCallback) \
    UA_Server_setMethodNodeCallback(server, methodNodeId, methodCallback)

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_getMethodNodeCallback(UA_Server *server,
                                const UA_NodeId methodNodeId,
                                UA_MethodCallback *outMethodCallback);

UA_CallMethodResult UA_EXPORT UA_THREADSAFE
UA_Server_call(UA_Server *server, const UA_CallMethodRequest *request);
#endif

/**
 * .. _object-interaction:
 *
 * Interacting with Objects
 * ------------------------
 * Objects in the information model are represented as ObjectNodes. Some
 * convenience functions are provided to simplify the interaction with objects. */

/* Write an object property. The property is represented as a VariableNode with
 * a ``HasProperty`` reference from the ObjectNode. The VariableNode is
 * identified by its BrowseName. Writing the property sets the value attribute
 * of the VariableNode.
 *
 * @param server The server object
 * @param objectId The identifier of the object (node)
 * @param propertyName The name of the property
 * @param value The value to be set for the event attribute
 * @return The StatusCode for setting the event attribute */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_writeObjectProperty(UA_Server *server, const UA_NodeId objectId,
                              const UA_QualifiedName propertyName,
                              const UA_Variant value);

/* Directly point to the scalar value instead of a variant */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_writeObjectProperty_scalar(UA_Server *server, const UA_NodeId objectId,
                                     const UA_QualifiedName propertyName,
                                     const void *value, const UA_DataType *type);

/* Read an object property.
 *
 * @param server The server object
 * @param objectId The identifier of the object (node)
 * @param propertyName The name of the property
 * @param value Contains the property value after reading. Must not be NULL.
 * @return The StatusCode for setting the event attribute */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_readObjectProperty(UA_Server *server, const UA_NodeId objectId,
                             const UA_QualifiedName propertyName,
                             UA_Variant *value);

/**
 * .. _addnodes:
 *
 * Node Addition and Deletion
 * --------------------------
 * When creating dynamic node instances at runtime, chances are that you will
 * not care about the specific NodeId of the new node, as long as you can
 * reference it later. When passing numeric NodeIds with a numeric identifier 0,
 * the stack evaluates this as "select a random unassigned numeric NodeId in
 * that namespace". To find out which NodeId was actually assigned to the new
 * node, you may pass a pointer `outNewNodeId`, which will (after a successful
 * node insertion) contain the nodeId of the new node. You may also pass a
 * ``NULL`` pointer if this result is not needed.
 *
 * See the Section :ref:`node-lifecycle` on constructors and on attaching
 * user-defined data to nodes.
 *
 * The Section :ref:`default-node-attributes` contains useful starting points
 * for defining node attributes. Forgetting to set the ValueRank or the
 * AccessLevel leads to errors that can be hard to track down for new users. The
 * default attributes have a high likelihood to "do the right thing".
 *
 * The methods for node addition and deletion take mostly const arguments that
 * are not modified. When creating a node, a deep copy of the node identifier,
 * node attributes, etc. is created. Therefore, it is possible to call for
 * example ``UA_Server_addVariablenode`` with a value attribute (a
 * :ref:`variant`) pointing to a memory location on the stack. If you need
 * changes to a variable value to manifest at a specific memory location, please
 * use a :ref:`datasource` or a :ref:`value-callback`. */

/* Don't use this function. There are typed versions as inline functions. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
__UA_Server_addNode(UA_Server *server, const UA_NodeClass nodeClass,
                    const UA_NodeId *requestedNewNodeId,
                    const UA_NodeId *parentNodeId,
                    const UA_NodeId *referenceTypeId,
                    const UA_QualifiedName browseName,
                    const UA_NodeId *typeDefinition,
                    const UA_NodeAttributes *attr,
                    const UA_DataType *attributeType,
                    void *nodeContext, UA_NodeId *outNewNodeId);

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_addVariableNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                          const UA_NodeId parentNodeId,
                          const UA_NodeId referenceTypeId,
                          const UA_QualifiedName browseName,
                          const UA_NodeId typeDefinition,
                          const UA_VariableAttributes attr,
                          void *nodeContext, UA_NodeId *outNewNodeId) ,{
    return __UA_Server_addNode(server, UA_NODECLASS_VARIABLE, &requestedNewNodeId,
                               &parentNodeId, &referenceTypeId, browseName,
                               &typeDefinition, (const UA_NodeAttributes*)&attr,
                               &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
                               nodeContext, outNewNodeId);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_addVariableTypeNode(UA_Server *server,
                              const UA_NodeId requestedNewNodeId,
                              const UA_NodeId parentNodeId,
                              const UA_NodeId referenceTypeId,
                              const UA_QualifiedName browseName,
                              const UA_NodeId typeDefinition,
                              const UA_VariableTypeAttributes attr,
                              void *nodeContext, UA_NodeId *outNewNodeId) ,{
    return __UA_Server_addNode(server, UA_NODECLASS_VARIABLETYPE,
                               &requestedNewNodeId, &parentNodeId, &referenceTypeId,
                               browseName, &typeDefinition,
                               (const UA_NodeAttributes*)&attr,
                               &UA_TYPES[UA_TYPES_VARIABLETYPEATTRIBUTES],
                               nodeContext, outNewNodeId);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_addObjectNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                        const UA_NodeId parentNodeId,
                        const UA_NodeId referenceTypeId,
                        const UA_QualifiedName browseName,
                        const UA_NodeId typeDefinition,
                        const UA_ObjectAttributes attr,
                        void *nodeContext, UA_NodeId *outNewNodeId) ,{
    return __UA_Server_addNode(server, UA_NODECLASS_OBJECT, &requestedNewNodeId,
                               &parentNodeId, &referenceTypeId, browseName,
                               &typeDefinition, (const UA_NodeAttributes*)&attr,
                               &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                               nodeContext, outNewNodeId);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_addObjectTypeNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                            const UA_NodeId parentNodeId,
                            const UA_NodeId referenceTypeId,
                            const UA_QualifiedName browseName,
                            const UA_ObjectTypeAttributes attr,
                            void *nodeContext, UA_NodeId *outNewNodeId) ,{
    return __UA_Server_addNode(server, UA_NODECLASS_OBJECTTYPE, &requestedNewNodeId,
                               &parentNodeId, &referenceTypeId, browseName,
                               &UA_NODEID_NULL, (const UA_NodeAttributes*)&attr,
                               &UA_TYPES[UA_TYPES_OBJECTTYPEATTRIBUTES],
                               nodeContext, outNewNodeId);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_addViewNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                      const UA_NodeId parentNodeId,
                      const UA_NodeId referenceTypeId,
                      const UA_QualifiedName browseName,
                      const UA_ViewAttributes attr,
                      void *nodeContext, UA_NodeId *outNewNodeId) ,{
    return __UA_Server_addNode(server, UA_NODECLASS_VIEW, &requestedNewNodeId,
                               &parentNodeId, &referenceTypeId, browseName,
                               &UA_NODEID_NULL, (const UA_NodeAttributes*)&attr,
                               &UA_TYPES[UA_TYPES_VIEWATTRIBUTES],
                               nodeContext, outNewNodeId);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_addReferenceTypeNode(UA_Server *server,
                               const UA_NodeId requestedNewNodeId,
                               const UA_NodeId parentNodeId,
                               const UA_NodeId referenceTypeId,
                               const UA_QualifiedName browseName,
                               const UA_ReferenceTypeAttributes attr,
                               void *nodeContext, UA_NodeId *outNewNodeId) ,{
    return __UA_Server_addNode(server, UA_NODECLASS_REFERENCETYPE,
                               &requestedNewNodeId, &parentNodeId, &referenceTypeId,
                               browseName, &UA_NODEID_NULL,
                               (const UA_NodeAttributes*)&attr,
                               &UA_TYPES[UA_TYPES_REFERENCETYPEATTRIBUTES],
                               nodeContext, outNewNodeId);
})

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_addDataTypeNode(UA_Server *server,
                          const UA_NodeId requestedNewNodeId,
                          const UA_NodeId parentNodeId,
                          const UA_NodeId referenceTypeId,
                          const UA_QualifiedName browseName,
                          const UA_DataTypeAttributes attr,
                          void *nodeContext, UA_NodeId *outNewNodeId) ,{
    return __UA_Server_addNode(server, UA_NODECLASS_DATATYPE, &requestedNewNodeId,
                               &parentNodeId, &referenceTypeId, browseName,
                               &UA_NODEID_NULL, (const UA_NodeAttributes*)&attr,
                               &UA_TYPES[UA_TYPES_DATATYPEATTRIBUTES],
                               nodeContext, outNewNodeId);
})

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_addDataSourceVariableNode(UA_Server *server,
                                    const UA_NodeId requestedNewNodeId,
                                    const UA_NodeId parentNodeId,
                                    const UA_NodeId referenceTypeId,
                                    const UA_QualifiedName browseName,
                                    const UA_NodeId typeDefinition,
                                    const UA_VariableAttributes attr,
                                    const UA_DataSource dataSource,
                                    void *nodeContext, UA_NodeId *outNewNodeId);

/* VariableNodes that are "dynamic" (default for user-created variables) receive
 * and store a SourceTimestamp. For non-dynamic VariableNodes the current time
 * is used for the SourceTimestamp. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_setVariableNodeDynamic(UA_Server *server, const UA_NodeId nodeId,
                                 UA_Boolean isDynamic);

#ifdef UA_ENABLE_METHODCALLS

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_addMethodNodeEx(UA_Server *server, const UA_NodeId requestedNewNodeId,
                          const UA_NodeId parentNodeId,
                          const UA_NodeId referenceTypeId,
                          const UA_QualifiedName browseName,
                          const UA_MethodAttributes attr, UA_MethodCallback method,
                          size_t inputArgumentsSize, const UA_Argument *inputArguments,
                          const UA_NodeId inputArgumentsRequestedNewNodeId,
                          UA_NodeId *inputArgumentsOutNewNodeId,
                          size_t outputArgumentsSize, const UA_Argument *outputArguments,
                          const UA_NodeId outputArgumentsRequestedNewNodeId,
                          UA_NodeId *outputArgumentsOutNewNodeId,
                          void *nodeContext, UA_NodeId *outNewNodeId);

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Server_addMethodNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                        const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                        const UA_QualifiedName browseName, const UA_MethodAttributes attr,
                        UA_MethodCallback method,
                        size_t inputArgumentsSize, const UA_Argument *inputArguments,
                        size_t outputArgumentsSize, const UA_Argument *outputArguments,
                        void *nodeContext, UA_NodeId *outNewNodeId) ,{
    return UA_Server_addMethodNodeEx(server, requestedNewNodeId,  parentNodeId,
                                     referenceTypeId, browseName, attr, method,
                                     inputArgumentsSize, inputArguments,
                                     UA_NODEID_NULL, NULL,
                                     outputArgumentsSize, outputArguments,
                                     UA_NODEID_NULL, NULL,
                                     nodeContext, outNewNodeId);
})

#endif


/**
 * The method pair UA_Server_addNode_begin and _finish splits the AddNodes
 * service in two parts. This is useful if the node shall be modified before
 * finish the instantiation. For example to add children with specific NodeIds.
 * Otherwise, mandatory children (e.g. of an ObjectType) are added with
 * pseudo-random unique NodeIds. Existing children are detected during the
 * _finish part via their matching BrowseName.
 *
 * The _begin method:
 *  - prepares the node and adds it to the nodestore
 *  - copies some unassigned attributes from the TypeDefinition node internally
 *  - adds the references to the parent (and the TypeDefinition if applicable)
 *  - performs type-checking of variables.
 *
 * You can add an object node without a parent if you set the parentNodeId and
 * referenceTypeId to UA_NODE_ID_NULL. Then you need to add the parent reference
 * and hasTypeDef reference yourself before calling the _finish method.
 * Not that this is only allowed for object nodes.
 *
 * The _finish method:
 *  - copies mandatory children
 *  - calls the node constructor(s) at the end
 *  - may remove the node if it encounters an error.
 *
 * The special UA_Server_addMethodNode_finish method needs to be used for method
 * nodes, since there you need to explicitly specifiy the input and output
 * arguments which are added in the finish step (if not yet already there) */

/* The ``attr`` argument must have a type according to the NodeClass.
 * ``VariableAttributes`` for variables, ``ObjectAttributes`` for objects, and
 * so on. Missing attributes are taken from the TypeDefinition node if
 * applicable. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_addNode_begin(UA_Server *server, const UA_NodeClass nodeClass,
                        const UA_NodeId requestedNewNodeId,
                        const UA_NodeId parentNodeId,
                        const UA_NodeId referenceTypeId,
                        const UA_QualifiedName browseName,
                        const UA_NodeId typeDefinition,
                        const void *attr, const UA_DataType *attributeType,
                        void *nodeContext, UA_NodeId *outNewNodeId);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_addNode_finish(UA_Server *server, const UA_NodeId nodeId);

#ifdef UA_ENABLE_METHODCALLS

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_addMethodNode_finish(UA_Server *server, const UA_NodeId nodeId,
                         UA_MethodCallback method,
                         size_t inputArgumentsSize, const UA_Argument *inputArguments,
                         size_t outputArgumentsSize, const UA_Argument *outputArguments);

#endif

/* Deletes a node and optionally all references leading to the node. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_deleteNode(UA_Server *server, const UA_NodeId nodeId,
                     UA_Boolean deleteReferences);

/**
 * Reference Management
 * -------------------- */

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_addReference(UA_Server *server, const UA_NodeId sourceId,
                       const UA_NodeId refTypeId,
                       const UA_ExpandedNodeId targetId, UA_Boolean isForward);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_deleteReference(UA_Server *server, const UA_NodeId sourceNodeId,
                          const UA_NodeId referenceTypeId, UA_Boolean isForward,
                          const UA_ExpandedNodeId targetNodeId,
                          UA_Boolean deleteBidirectional);

/**
 * .. _events:
 *
 * Events
 * ------
 * The method ``UA_Server_createEvent`` creates an event and represents it as
 * node. The node receives a unique `EventId` which is automatically added to
 * the node. The method returns a `NodeId` to the object node which represents
 * the event through ``outNodeId``. The `NodeId` can be used to set the
 * attributes of the event. The generated `NodeId` is always numeric.
 * ``outNodeId`` cannot be ``NULL``.
 *
 * Note: In order to see an event in UAExpert, the field `Time` must be given a
 * value!
 *
 * The method ``UA_Server_triggerEvent`` "triggers" an event by adding it to all
 * monitored items of the specified origin node and those of all its parents.
 * Any filters specified by the monitored items are automatically applied. Using
 * this method deletes the node generated by ``UA_Server_createEvent``. The
 * `EventId` for the new event is generated automatically and is returned
 * through ``outEventId``. ``NULL`` can be passed if the `EventId` is not
 * needed. ``deleteEventNode`` specifies whether the node representation of the
 * event should be deleted after invoking the method. This can be useful if
 * events with the similar attributes are triggered frequently. ``UA_TRUE``
 * would cause the node to be deleted. */

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS

/* Creates a node representation of an event
 *
 * @param server The server object
 * @param eventType The type of the event for which a node should be created
 * @param outNodeId The NodeId of the newly created node for the event
 * @return The StatusCode of the UA_Server_createEvent method */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_createEvent(UA_Server *server, const UA_NodeId eventType,
                      UA_NodeId *outNodeId);

/* Triggers a node representation of an event by applying EventFilters and
 * adding the event to the appropriate queues.
 *
 * @param server The server object
 * @param eventNodeId The NodeId of the node representation of the event which
 *        should be triggered
 * @param outEvent the EventId of the new event
 * @param deleteEventNode Specifies whether the node representation of the event
 *        should be deleted
 * @return The StatusCode of the UA_Server_triggerEvent method */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_triggerEvent(UA_Server *server, const UA_NodeId eventNodeId,
                       const UA_NodeId originId, UA_ByteString *outEventId,
                       const UA_Boolean deleteEventNode);

#endif /* UA_ENABLE_SUBSCRIPTIONS_EVENTS */

/**
 * Alarms & Conditions (Experimental)
 * ---------------------------------- */

#ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS
typedef enum UA_TwoStateVariableCallbackType {
  UA_ENTERING_ENABLEDSTATE,
  UA_ENTERING_ACKEDSTATE,
  UA_ENTERING_CONFIRMEDSTATE,
  UA_ENTERING_ACTIVESTATE
} UA_TwoStateVariableCallbackType;

/* Callback prototype to set user specific callbacks */
typedef UA_StatusCode
(*UA_TwoStateVariableChangeCallback)(UA_Server *server, const UA_NodeId *condition);

/* Create condition instance. The function checks first whether the passed
 * conditionType is a subType of ConditionType. Then checks whether the
 * condition source has HasEventSource reference to its parent. If not, a
 * HasEventSource reference will be created between condition source and server
 * object. To expose the condition in address space, a hierarchical
 * ReferenceType should be passed to create the reference to condition source.
 * Otherwise, UA_NODEID_NULL should be passed to make the condition not exposed.
 *
 * @param server The server object
 * @param conditionId The NodeId of the requested Condition Object. When passing
 *        UA_NODEID_NUMERIC(X,0) an unused nodeid in namespace X will be used.
 *        E.g. passing UA_NODEID_NULL will result in a NodeId in namespace 0.
 * @param conditionType The NodeId of the node representation of the ConditionType
 * @param conditionName The name of the condition to be created
 * @param conditionSource The NodeId of the Condition Source (Parent of the Condition)
 * @param hierarchialReferenceType The NodeId of Hierarchical ReferenceType
 *                                 between Condition and its source
 * @param outConditionId The NodeId of the created Condition
 * @return The StatusCode of the UA_Server_createCondition method */
UA_StatusCode UA_EXPORT
UA_Server_createCondition(UA_Server *server,
                          const UA_NodeId conditionId,
                          const UA_NodeId conditionType,
                          const UA_QualifiedName conditionName,
                          const UA_NodeId conditionSource,
                          const UA_NodeId hierarchialReferenceType,
                          UA_NodeId *outConditionId);

/* The method pair UA_Server_addCondition_begin and _finish splits the
 * UA_Server_createCondtion in two parts similiar to the
 * UA_Server_addNode_begin / _finish pair. This is useful if the node shall be
 * modified before finish the instantiation. For example to add children with
 * specific NodeIds.
 * For details refer to the UA_Server_addNode_begin / _finish methods.
 *
 * Additionally to UA_Server_addNode_begin UA_Server_addCondition_begin checks
 * if the passed condition type is a subtype of the OPC UA ConditionType.
 *
 * @param server The server object
 * @param conditionId The NodeId of the requested Condition Object. When passing
 *        UA_NODEID_NUMERIC(X,0) an unused nodeid in namespace X will be used.
 *        E.g. passing UA_NODEID_NULL will result in a NodeId in namespace 0.
 * @param conditionType The NodeId of the node representation of the ConditionType
 * @param conditionName The name of the condition to be added
 * @param outConditionId The NodeId of the added Condition
 * @return The StatusCode of the UA_Server_addCondition_begin method */
UA_StatusCode UA_EXPORT
UA_Server_addCondition_begin(UA_Server *server,
                             const UA_NodeId conditionId,
                             const UA_NodeId conditionType,
                             const UA_QualifiedName conditionName,
                             UA_NodeId *outConditionId);

/* Second call of the UA_Server_addCondition_begin and _finish pair.
 * Additionally to UA_Server_addNode_finish UA_Server_addCondition_finish:
 *  - checks whether the condition source has HasEventSource reference to its
 *    parent. If not, a HasEventSource reference will be created between
 *    condition source and server object
 *  - exposes the condition in the address space if hierarchialReferenceType is
 *    not UA_NODEID_NULL by adding a reference of this type from the condition
 *    source to the condition instance
 *  - initializes the standard condition fields and callbacks
 *
 * @param server The server object
 * @param conditionId The NodeId of the unfinished Condition Object
 * @param conditionSource The NodeId of the Condition Source (Parent of the Condition)
 * @param hierarchialReferenceType The NodeId of Hierarchical ReferenceType
 *                                 between Condition and its source
 * @return The StatusCode of the UA_Server_addCondition_finish method */

UA_StatusCode UA_EXPORT
UA_Server_addCondition_finish(UA_Server *server,
                              const UA_NodeId conditionId,
                              const UA_NodeId conditionSource,
                              const UA_NodeId hierarchialReferenceType);

/* Set the value of condition field.
 *
 * @param server The server object
 * @param condition The NodeId of the node representation of the Condition Instance
 * @param value Variant Value to be written to the Field
 * @param fieldName Name of the Field in which the value should be written
 * @return The StatusCode of the UA_Server_setConditionField method*/
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_setConditionField(UA_Server *server,
                            const UA_NodeId condition,
                            const UA_Variant *value,
                            const UA_QualifiedName fieldName);

/* Set the value of property of condition field.
 *
 * @param server The server object
 * @param condition The NodeId of the node representation of the Condition
 *        Instance
 * @param value Variant Value to be written to the Field
 * @param variableFieldName Name of the Field which has a property
 * @param variablePropertyName Name of the Field Property in which the value
 *        should be written
 * @return The StatusCode of the UA_Server_setConditionVariableFieldProperty*/
UA_StatusCode UA_EXPORT
UA_Server_setConditionVariableFieldProperty(UA_Server *server,
                                            const UA_NodeId condition,
                                            const UA_Variant *value,
                                            const UA_QualifiedName variableFieldName,
                                            const UA_QualifiedName variablePropertyName);

/* Triggers an event only for an enabled condition. The condition list is
 * updated then with the last generated EventId.
 *
 * @param server The server object
 * @param condition The NodeId of the node representation of the Condition Instance
 * @param conditionSource The NodeId of the node representation of the Condition Source
 * @param outEventId last generated EventId
 * @return The StatusCode of the UA_Server_triggerConditionEvent method */
UA_StatusCode UA_EXPORT
UA_Server_triggerConditionEvent(UA_Server *server,
                                const UA_NodeId condition,
                                const UA_NodeId conditionSource,
                                UA_ByteString *outEventId);

/* Add an optional condition field using its name. (TODO Adding optional methods
 * is not implemented yet)
 *
 * @param server The server object
 * @param condition The NodeId of the node representation of the Condition Instance
 * @param conditionType The NodeId of the node representation of the Condition Type
 * from which the optional field comes
 * @param fieldName Name of the optional field
 * @param outOptionalVariable The NodeId of the created field (Variable Node)
 * @return The StatusCode of the UA_Server_addConditionOptionalField method */
UA_StatusCode UA_EXPORT
UA_Server_addConditionOptionalField(UA_Server *server,
                                    const UA_NodeId condition,
                                    const UA_NodeId conditionType,
                                    const UA_QualifiedName fieldName,
                                    UA_NodeId *outOptionalVariable);

/* Function used to set a user specific callback to TwoStateVariable Fields of a
 * condition. The callbacks will be called before triggering the events when
 * transition to true State of EnabledState/Id, AckedState/Id, ConfirmedState/Id
 * and ActiveState/Id occurs.
 *
 * @param server The server object
 * @param condition The NodeId of the node representation of the Condition Instance
 * @param conditionSource The NodeId of the node representation of the Condition Source
 * @param removeBranch (Not Implemented yet)
 * @param callback User specific callback function
 * @param callbackType Callback function type, indicates where it should be called
 * @return The StatusCode of the UA_Server_setConditionTwoStateVariableCallback method */
UA_StatusCode UA_EXPORT
UA_Server_setConditionTwoStateVariableCallback(UA_Server *server,
                                               const UA_NodeId condition,
                                               const UA_NodeId conditionSource,
                                               UA_Boolean removeBranch,
                                               UA_TwoStateVariableChangeCallback callback,
                                               UA_TwoStateVariableCallbackType callbackType);

/* Delete a condition from the address space and the internal lists.
 *
 * @param server The server object
 * @param condition The NodeId of the node representation of the Condition Instance
 * @param conditionSource The NodeId of the node representation of the Condition Source
 * @return ``UA_STATUSCODE_GOOD`` on success */
UA_StatusCode UA_EXPORT
UA_Server_deleteCondition(UA_Server *server,
                          const UA_NodeId condition,
                          const UA_NodeId conditionSource);

/* Set the LimitState of the LimitAlarmType
 *
 * @param server The server object
 * @param conditionId NodeId of the node representation of the Condition Instance
 * @param limitValue The value from the trigger node */
UA_StatusCode UA_EXPORT
UA_Server_setLimitState(UA_Server *server, const UA_NodeId conditionId,
                        UA_Double limitValue);

/* Parse the certifcate and set Expiration date
 *
 * @param server The server object
 * @param conditionId NodeId of the node representation of the Condition Instance
 * @param cert The certificate for parsing */
UA_StatusCode UA_EXPORT
UA_Server_setExpirationDate(UA_Server *server, const UA_NodeId conditionId,
                            UA_ByteString  cert);

#endif /* UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS */

/**
 * Update the Server Certificate at Runtime
 * ---------------------------------------- */

UA_StatusCode UA_EXPORT
UA_Server_updateCertificate(UA_Server *server,
                            const UA_ByteString *oldCertificate,
                            const UA_ByteString *newCertificate,
                            const UA_ByteString *newPrivateKey,
                            UA_Boolean closeSessions,
                            UA_Boolean closeSecureChannels);

/**
 * Utility Functions
 * ----------------- */

/* Lookup a datatype by its NodeId. Takes the custom types in the server
 * configuration into account. Return NULL if none found. */
UA_EXPORT const UA_DataType *
UA_Server_findDataType(UA_Server *server, const UA_NodeId *typeId);

/* Add a new namespace to the server. Returns the index of the new namespace */
UA_UInt16 UA_EXPORT UA_THREADSAFE
UA_Server_addNamespace(UA_Server *server, const char* name);

/* Get namespace by name from the server. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_getNamespaceByName(UA_Server *server, const UA_String namespaceUri,
                             size_t* foundIndex);

/* Get namespace by id from the server. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_getNamespaceByIndex(UA_Server *server, const size_t namespaceIndex,
                              UA_String *foundUri);

/**
 * .. _async-operations:
 *
 * Async Operations
 * ----------------
 * Some operations (such as reading out a sensor that needs to warm up) can take
 * quite some time. In order not to block the server during such an operation, it
 * can be "outsourced" to a worker thread.
 *
 * Take the example of a CallRequest. It is split into the individual method call
 * operations. If the method is marked as async, then the operation is put into a
 * queue where it is be retrieved by a worker. The worker returns the result when
 * ready. See the examples in ``/examples/tutorial_server_method_async.c`` for
 * the usage.
 *
 * Note that the operation can time out (see the asyncOperationTimeout setting in
 * the server config) also when it has been retrieved by the worker. */

#if UA_MULTITHREADING >= 100

/* Set the async flag in a method node */
UA_StatusCode UA_EXPORT
UA_Server_setMethodNodeAsync(UA_Server *server, const UA_NodeId id,
                             UA_Boolean isAsync);

typedef enum {
    UA_ASYNCOPERATIONTYPE_INVALID, /* 0, the default */
    UA_ASYNCOPERATIONTYPE_CALL
    /* UA_ASYNCOPERATIONTYPE_READ, */
    /* UA_ASYNCOPERATIONTYPE_WRITE, */
} UA_AsyncOperationType;

typedef union {
    UA_CallMethodRequest callMethodRequest;
    /* UA_ReadValueId readValueId; */
    /* UA_WriteValue writeValue; */
} UA_AsyncOperationRequest;

typedef union {
    UA_CallMethodResult callMethodResult;
    /* UA_DataValue readResult; */
    /* UA_StatusCode writeResult; */
} UA_AsyncOperationResponse;

/* Get the next async operation without blocking
 *
 * @param server The server object
 * @param type The type of the async operation
 * @param request Receives pointer to the operation
 * @param context Receives the pointer to the operation context
 * @param timeout The timestamp when the operation times out and can
 *        no longer be returned to the client. The response has to
 *        be set in UA_Server_setAsyncOperationResult in any case.
 * @return false if queue is empty, true else */
UA_Boolean UA_EXPORT
UA_Server_getAsyncOperationNonBlocking(UA_Server *server,
                                       UA_AsyncOperationType *type,
                                       const UA_AsyncOperationRequest **request,
                                       void **context, UA_DateTime *timeout);

/* UA_Boolean UA_EXPORT */
/* UA_Server_getAsyncOperationBlocking(UA_Server *server, */
/*                                     UA_AsyncOperationType *type, */
/*                                     const UA_AsyncOperationRequest **request, */
/*                                     void **context, UA_DateTime *timeout); */

/* Submit an async operation result
 *
 * @param server The server object
 * @param response Pointer to the operation result
 * @param context Pointer to the operation context */
void UA_EXPORT
UA_Server_setAsyncOperationResult(UA_Server *server,
                                  const UA_AsyncOperationResponse *response,
                                  void *context);

#endif /* !UA_MULTITHREADING >= 100 */

/**
 * Statistics
 * ----------
 * Statistic counters keeping track of the current state of the stack. Counters
 * are structured per OPC UA communication layer. */

typedef struct {
   UA_SecureChannelStatistics scs;
   UA_SessionStatistics ss;
} UA_ServerStatistics;

UA_ServerStatistics UA_EXPORT
UA_Server_getStatistics(UA_Server *server);

/**
 * Reverse Connect
 * ---------------
 * The reverse connect feature of OPC UA permits the server instead of the
 * client to establish the connection. The client must expose the listening port
 * so the server is able to reach it. */

/* The reverse connect state change callback is called whenever the state of a
 * reverse connect is changed by a connection attempt, a successful connection
 * or a connection loss.
 *
 * The reverse connect states reflect the state of the secure channel currently
 * associated with a reverse connect. The state will remain
 * UA_SECURECHANNELSTATE_CONNECTING while the server attempts repeatedly to
 * establish a connection. */
typedef void (*UA_Server_ReverseConnectStateCallback)(UA_Server *server,
                                                      UA_UInt64 handle,
                                                      UA_SecureChannelState state,
                                                      void *context);

/* Registers a reverse connect in the server. The server periodically attempts
 * to establish a connection if the initial connect fails or if the connection
 * breaks.
 *
 * @param server The server object
 * @param url The URL of the remote client
 * @param stateCallback The callback which will be called on state changes
 * @param callbackContext The context for the state callback
 * @param handle Is set to the handle of the reverse connect if not NULL
 * @return Returns UA_STATUSCODE_GOOD if the reverse connect has been registered */
UA_StatusCode UA_EXPORT
UA_Server_addReverseConnect(UA_Server *server, UA_String url,
                            UA_Server_ReverseConnectStateCallback stateCallback,
                            void *callbackContext, UA_UInt64 *handle);

/* Removes a reverse connect from the server and closes the connection if it is
 * currently open.
 *
 * @param server The server object
 * @param handle The handle of the reverse connect to remove
 * @return Returns UA_STATUSCODE_GOOD if the reverse connect has been
 *         successfully removed */
UA_StatusCode UA_EXPORT
UA_Server_removeReverseConnect(UA_Server *server, UA_UInt64 handle);

_UA_END_DECLS

#ifdef UA_ENABLE_PUBSUB
#include <open62541/server_pubsub.h>
#endif

#endif /* UA_SERVER_H_ */
