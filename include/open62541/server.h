/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2025 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
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

#include <open62541/common.h>
#include <open62541/util.h>
#include <open62541/types.h>
#include <open62541/client.h>

#include <open62541/plugin/log.h>
#include <open62541/plugin/certificategroup.h>
#include <open62541/plugin/eventloop.h>
#include <open62541/plugin/accesscontrol.h>
#include <open62541/plugin/securitypolicy.h>

#ifdef UA_ENABLE_HISTORIZING
#include <open62541/plugin/historydatabase.h>
#endif

#ifdef UA_ENABLE_PUBSUB
#include <open62541/server_pubsub.h>
#endif

/* Forward Declarations */
struct UA_Nodestore;
typedef struct UA_Nodestore UA_Nodestore;

struct UA_ServerConfig;
typedef struct UA_ServerConfig UA_ServerConfig;

_UA_BEGIN_DECLS

/**
 * .. _server:
 *
 * Server
 * ======
 * An OPC UA server contains an object-oriented information model and makes it
 * accessible to clients over the network via the OPC UA :ref:`services`. The
 * information model can be used either used to store "passive data" or as an
 * "active database" that integrates with data-sources and devices. For the
 * latter, user-defined callbacks can be attached to VariableNodes and
 * MethodNodes.
 *
 * .. _server-lifecycle:
 *
 * Server Lifecycle
 * ----------------
 * This section describes the API for creating, running and deleting a server.
 * At runtime, the server continuously listens on the network, acceppts incoming
 * connections and processes received messages. Furthermore, timed (cyclic)
 * callbacks are executed. */

/* Create a new server with a default configuration that adds plugins for
 * networking, security, logging and so on. See the "server_config_default.h"
 * for more detailed options.
 *
 * The default configuration can be used as the starting point to adjust the
 * server configuration to individual needs. UA_Server_new is implemented in the
 * /plugins folder under the CC0 license. Furthermore the server confiugration
 * only uses the public server API.
 *
 * Returns the configured server or NULL if an error occurs. */
UA_EXPORT UA_Server *
UA_Server_new(void);

/* Creates a new server. Moves the config into the server with a shallow copy.
 * The config content is cleared together with the server. */
UA_EXPORT UA_Server *
UA_Server_newWithConfig(UA_ServerConfig *config);

/* Delete the server and its configuration */
UA_EXPORT UA_StatusCode
UA_Server_delete(UA_Server *server);

/* Get the configuration. Always succeeds as this simplfy resolves a pointer.
 * Attention! Do not adjust the configuration while the server is running! */
UA_EXPORT UA_ServerConfig *
UA_Server_getConfig(UA_Server *server);

/* Get the current server lifecycle state */
UA_EXPORT UA_LifecycleState
UA_Server_getLifecycleState(UA_Server *server);

/* Runs the server until until "running" is set to false. The logical sequence
 * is as follows:
 *
 * - UA_Server_run_startup
 * - Loop UA_Server_run_iterate while "running" is true
 * - UA_Server_run_shutdown */
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
 * /plugins/ua_config_default.c under the CC0 license. Adjust as needed. */
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
 * Timed callback are executed at their defined timestamp. The callback can also
 * be registered with a cyclic repetition interval. */

typedef void (*UA_ServerCallback)(UA_Server *server, void *data);

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

/* Remove a repeated callback. Does nothing if the callback is not found. */
void UA_EXPORT UA_THREADSAFE
UA_Server_removeCallback(UA_Server *server, UA_UInt64 callbackId);

#define UA_Server_removeRepeatedCallback(server, callbackId) \
    UA_Server_removeCallback(server, callbackId)

/**
 * Application Notification
 * ------------------------
 * The server defines callbacks to notify the application on defined triggering
 * points. These callbacks are executed with the (re-entrant) server-mutex held.
 *
 * The different types of callback are disambiguated by their type enum. Besides
 * the global notification callback (which is always triggered), the server
 * configuration contains specialized callbacks that trigger only for specific
 * notifications. This can reduce the burden of high-frequency notifications.
 *
 * If a specialized notification callback is set, it always gets called before
 * the global notification callback for the same triggering point.
 *
 * See the section on the :ref:`Application Notification` enum for more
 * documentation on the notifications and their defined payload. */

typedef void (*UA_ServerNotificationCallback)(UA_Server *server,
                                              UA_ApplicationNotificationType type,
                                              const UA_KeyValueMap payload);

/**
 * .. _server-session-handling:
 *
 * Session Handling
 * ----------------
 * Sessions are managed via the OPC UA Session Service Set (CreateSession,
 * ActivateSession, CloseSession). The identifier of sessions is generated
 * internally in the server and is always a Guid-NodeId.
 *
 * The creation of sessions is passed to the :ref:`access-control`. There, the
 * authentication information is evaluated and a context-pointer is attached to
 * the new session. The context pointer (and the session identifier) are then
 * forwarded to all user-defined callbacks that can be triggere by a session.
 *
 * When the operations from the OPC UA Services are invoked locally via the
 * C-API, this implies that the operations are executed with the access rights
 * of the "admin-session" that is always present in a server. Any AccessControl
 * checks are omitted for the admin-session.
 *
 * The admin-session has the identifier
 * ``g=00000001-0000-0000-0000-000000000000``. Its session context pointer needs
 * to be manually set (NULL by default). */

void UA_EXPORT
UA_Server_setAdminSessionContext(UA_Server *server, void *context);

/* Manually close a session */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_closeSession(UA_Server *server, const UA_NodeId *sessionId);

/**
 * Besides the session context pointer from the AccessControl plugin, a session
 * carries attributes in a key-value map. Always defined (and read-only) session
 * attributes are:
 *
 * - ``0:localeIds`` (``UA_String``): List of preferred languages
 * - ``0:clientDescription`` (``UA_ApplicationDescription``): Client description
 * - ``0:sessionName`` (``String``): Client-defined name of the session
 * - ``0:clientUserId`` (``String``): User identifier used to activate the session
 *
 * Additional attributes can be set manually with the API below. */

/* Returns a shallow copy of the attribute (don't _clear or _delete manually).
 * While the method is thread-safe, the returned value is not protected. Only
 * use it in a (callback) context where the server is locked for the current
 * thread. */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_getSessionAttribute(UA_Server *server, const UA_NodeId *sessionId,
                              const UA_QualifiedName key, UA_Variant *outValue);

/* Return a deep copy of the attribute */
UA_EXPORT UA_StatusCode UA_THREADSAFE
UA_Server_getSessionAttributeCopy(UA_Server *server, const UA_NodeId *sessionId,
                                  const UA_QualifiedName key, UA_Variant *outValue);

/* Returns NULL if the attribute is not defined or not a scalar or not of the
 * right datatype. Otherwise a shallow copy of the scalar value is created at
 * the target location of the void pointer (don't _clear or _delete manually).
 * While the method is thread-safe, the returned value is not protected. Only
 * use it in a (callback) context where the server is locked for the current
 * thread. */
UA_EXPORT UA_StatusCode UA_THREADSAFE
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
 * Attribute Service Set
 * ---------------------
 * The functions for reading and writing node attributes call the regular read
 * and write service in the background that are also used over the network.
 *
 * The following attributes cannot be read, since the local "admin" user always
 * has full rights.
 *
 * - UserWriteMask
 * - UserAccessLevel
 * - UserExecutable */

/* Read an attribute of a node. Returns a deep copy. */
UA_DataValue UA_EXPORT UA_THREADSAFE
UA_Server_read(UA_Server *server, const UA_ReadValueId *item,
               UA_TimestampsToReturn timestamps);

/**
 * The following specialized read methods are a shorthand for the regular read
 * and set a deep copy of the attribute to the ``out`` pointer (when
 * successful). */

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_readNodeId(UA_Server *server, const UA_NodeId nodeId,
                     UA_NodeId *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_readNodeClass(UA_Server *server, const UA_NodeId nodeId,
                        UA_NodeClass *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_readBrowseName(UA_Server *server, const UA_NodeId nodeId,
                         UA_QualifiedName *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_readDisplayName(UA_Server *server, const UA_NodeId nodeId,
                          UA_LocalizedText *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_readDescription(UA_Server *server, const UA_NodeId nodeId,
                          UA_LocalizedText *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_readWriteMask(UA_Server *server, const UA_NodeId nodeId,
                        UA_UInt32 *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_readIsAbstract(UA_Server *server, const UA_NodeId nodeId,
                         UA_Boolean *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_readSymmetric(UA_Server *server, const UA_NodeId nodeId,
                        UA_Boolean *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_readInverseName(UA_Server *server, const UA_NodeId nodeId,
                          UA_LocalizedText *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_readContainsNoLoops(UA_Server *server, const UA_NodeId nodeId,
                              UA_Boolean *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_readEventNotifier(UA_Server *server, const UA_NodeId nodeId,
                            UA_Byte *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_readValue(UA_Server *server, const UA_NodeId nodeId,
                    UA_Variant *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_readDataType(UA_Server *server, const UA_NodeId nodeId,
                       UA_NodeId *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_readValueRank(UA_Server *server, const UA_NodeId nodeId,
                        UA_Int32 *out);

/* Returns a variant with an uint32 array */
UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_readArrayDimensions(UA_Server *server, const UA_NodeId nodeId,
                              UA_Variant *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_readAccessLevel(UA_Server *server, const UA_NodeId nodeId,
                          UA_Byte *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_readAccessLevelEx(UA_Server *server, const UA_NodeId nodeId,
                            UA_UInt32 *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_readMinimumSamplingInterval(UA_Server *server, const UA_NodeId nodeId,
                                      UA_Double *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_readHistorizing(UA_Server *server, const UA_NodeId nodeId,
                          UA_Boolean *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_readExecutable(UA_Server *server, const UA_NodeId nodeId,
                         UA_Boolean *out);

/**
 * The following node attributes cannot be written once a node has been created:
 *
 * - NodeClass
 * - NodeId
 * - Symmetric
 * - ContainsNoLoops
 *
 * The following attributes cannot be written from C-API, as they are specific
 * to the session (context set by the access control callback):
 *
 * - UserWriteMask
 * - UserAccessLevel
 * - UserExecutable
 */

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_write(UA_Server *server, const UA_WriteValue *value);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_writeBrowseName(UA_Server *server, const UA_NodeId nodeId,
                          const UA_QualifiedName browseName);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_writeDisplayName(UA_Server *server, const UA_NodeId nodeId,
                           const UA_LocalizedText displayName);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_writeDescription(UA_Server *server, const UA_NodeId nodeId,
                           const UA_LocalizedText description);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_writeWriteMask(UA_Server *server, const UA_NodeId nodeId,
                         const UA_UInt32 writeMask);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_writeIsAbstract(UA_Server *server, const UA_NodeId nodeId,
                          const UA_Boolean isAbstract);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_writeInverseName(UA_Server *server, const UA_NodeId nodeId,
                           const UA_LocalizedText inverseName);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_writeEventNotifier(UA_Server *server, const UA_NodeId nodeId,
                             const UA_Byte eventNotifier);

/* The value attribute is a DataValue. Here only a variant is provided. The
 * StatusCode is set to UA_STATUSCODE_GOOD, sourceTimestamp and serverTimestamp
 * are set to UA_DateTime_now(). See below for setting the full DataValue. */
UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_writeValue(UA_Server *server, const UA_NodeId nodeId,
                     const UA_Variant value);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_writeDataValue(UA_Server *server, const UA_NodeId nodeId,
                         const UA_DataValue value);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_writeDataType(UA_Server *server, const UA_NodeId nodeId,
                        const UA_NodeId dataType);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_writeValueRank(UA_Server *server, const UA_NodeId nodeId,
                         const UA_Int32 valueRank);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_writeArrayDimensions(UA_Server *server, const UA_NodeId nodeId,
                               const UA_Variant arrayDimensions);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_writeAccessLevel(UA_Server *server, const UA_NodeId nodeId,
                           const UA_Byte accessLevel);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_writeAccessLevelEx(UA_Server *server, const UA_NodeId nodeId,
                             const UA_UInt32 accessLevelEx);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_writeMinimumSamplingInterval(UA_Server *server, const UA_NodeId nodeId,
                                       const UA_Double miniumSamplingInterval);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_writeHistorizing(UA_Server *server, const UA_NodeId nodeId,
                           const UA_Boolean historizing);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_writeExecutable(UA_Server *server, const UA_NodeId nodeId,
                          const UA_Boolean executable);

/**
 * Method Service Set
 * ------------------
 * The Method Service Set defines the means to invoke methods. A MethodNode
 * shall be a component of an ObjectNode. Since the same MethodNode can be
 * referenced from multiple ObjectNodes, for calling a method, both method and
 * object need to be defined by their NodeId.
 *
 * The input and output arguments of a method are a list of ``UA_Variant``. The
 * type- and size-requirements of the arguments can be retrieved from the
 * **InputArguments** and **OutputArguments** variable below the MethodNode. */

#ifdef UA_ENABLE_METHODCALLS
UA_CallMethodResult UA_EXPORT UA_THREADSAFE
UA_Server_call(UA_Server *server, const UA_CallMethodRequest *request);
#endif

/**
 * View Service Set
 * ----------------
 * The View Service Set allows Clients to discover Nodes by browsing the
 * information model. */

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

/* Translate abrowse path to (potentially several) NodeIds. Each browse path is
 * constructed of a starting Node and a RelativePath. The specified starting
 * Node identifies the Node from which the RelativePath is based. The
 * RelativePath contains a sequence of ReferenceTypes and BrowseNames. */
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

/**
 * .. _local-monitoreditems:
 *
 * MonitoredItem Service Set
 * -------------------------
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
 *   /Severity   => UInt16(1000)
 *   /Message    => LocalizedText("en-US", "My Event Message")
 *   /EventType  => NodeId(i=50831)
 *   /SourceNode => NodeId(i=2253)
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

#endif /* UA_ENABLE_SUBSCRIPTIONS_EVENTS */

#endif /* UA_ENABLE_SUBSCRIPTIONS */

/**
 * .. _server-node-management:
 *
 * Node Management Service Set
 * ---------------------------
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
 * :ref:`variant`) pointing to a memory location on the stack.
 *
 * .. _variable-node:
 *
 * VariableNode
 * ~~~~~~~~~~~~
 * Variables store values as well as contraints for possible values. There are
 * three options for storing the value: Internal in the VariableNode data
 * structure itself, external with a double-pointer (to switch to an updated
 * value with an atomic pointer-replacing operation) or with a callback
 * registered by the application. */

typedef enum {
    UA_VALUESOURCETYPE_INTERNAL = 0,
    UA_VALUESOURCETYPE_EXTERNAL = 1,
    UA_VALUESOURCETYPE_CALLBACK = 2
} UA_ValueSourceType;

typedef struct {
    /* Notify the application before the value attribute is read. Ignored if
     * NULL. It is possible to write into the value attribute during onRead
     * (using the write service). The node is re-retrieved from the Nodestore
     * afterwards so that changes are considered in the following read
     * operation.
     *
     * @param handle Points to user-provided data for the callback.
     * @param nodeid The identifier of the node.
     * @param data Points to the current node value.
     * @param range Points to the numeric range the client wants to read from
     *        (or NULL). */
    void (*onRead)(UA_Server *server, const UA_NodeId *sessionId,
                   void *sessionContext, const UA_NodeId *nodeid,
                   void *nodeContext, const UA_NumericRange *range,
                   const UA_DataValue *value);

    /* Notify the application after writing the value attribute. Ignored if
     * NULL. The node is re-retrieved after writing, so that the new value is
     * visible in the callback.
     *
     * @param server The server executing the callback
     * @sessionId The identifier of the session
     * @sessionContext Additional data attached to the session
     *                 in the access control layer
     * @param nodeid The identifier of the node.
     * @param nodeUserContext Additional data attached to the node by
     *        the user.
     * @param nodeConstructorContext Additional data attached to the node
     *        by the type constructor(s).
     * @param range Points to the numeric range the client wants to write to (or
     *        NULL). */
    void (*onWrite)(UA_Server *server, const UA_NodeId *sessionId,
                    void *sessionContext, const UA_NodeId *nodeId,
                    void *nodeContext, const UA_NumericRange *range,
                    const UA_DataValue *data);
} UA_ValueSourceNotifications;

typedef struct {
    /* Copies the data from the source into the provided value.
     *
     * !! ZERO-COPY OPERATIONS POSSIBLE !!
     * It is not required to return a copy of the actual content data. You can
     * return a pointer to memory owned by the user. Memory can be reused
     * between read callbacks of a DataSource, as the result is already encoded
     * on the network buffer between each read operation.
     *
     * To use zero-copy reads, set the value of the `value->value` Variant
     * without copying, e.g. with `UA_Variant_setScalar`. Then, also set
     * `value->value.storageType` to `UA_VARIANT_DATA_NODELETE` to prevent the
     * memory being cleaned up. Don't forget to also set `value->hasValue` to
     * true to indicate the presence of a value.
     *
     * To make an async read, return UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY.
     * The result can then be set at a later time using
     * UA_Server_setAsyncReadResult. Note that the server might cancel the async
     * read by calling serverConfig->asyncOperationCancelCallback.
     *
     * @param server The server executing the callback
     * @param sessionId The identifier of the session
     * @param sessionContext Additional data attached to the session in the
     *        access control layer
     * @param nodeId The identifier of the node being read from
     * @param nodeContext Additional data attached to the node by the user
     * @param includeSourceTimeStamp If true, then the datasource is expected to
     *        set the source timestamp in the returned value
     * @param range If not null, then the datasource shall return only a
     *        selection of the (nonscalar) data. Set
     *        UA_STATUSCODE_BADINDEXRANGEINVALID in the value if this does not
     *        apply
     * @param value The (non-null) DataValue that is returned to the client. The
     *        data source sets the read data, the result status and optionally a
     *        sourcetimestamp.
     * @return Returns a status code for logging. Error codes intended for the
     *         original caller are set in the value. If an error is returned,
     *         then no releasing of the value is done. */
    UA_StatusCode (*read)(UA_Server *server, const UA_NodeId *sessionId,
                          void *sessionContext, const UA_NodeId *nodeId,
                          void *nodeContext, UA_Boolean includeSourceTimeStamp,
                          const UA_NumericRange *range, UA_DataValue *value);

    /* Write into a data source. This method pointer can be NULL if the
     * operation is unsupported.
     *
     * To make an async write, return UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY.
     * The result can then be set at a later time using
     * UA_Server_setAsyncWriteResult. Note that the server might cancel the
     * async read by calling serverConfig->asyncOperationCancelCallback.
     *
     * @param server The server executing the callback
     * @param sessionId The identifier of the session
     * @param sessionContext Additional data attached to the session in the
     *        access control layer
     * @param nodeId The identifier of the node being written to
     * @param nodeContext Additional data attached to the node by the user
     * @param range If not NULL, then the datasource shall return only a
     *        selection of the (nonscalar) data. Set
     *        UA_STATUSCODE_BADINDEXRANGEINVALID in the value if this does not
     *        apply
     * @param value The (non-NULL) DataValue that has been written by the client.
     *        The data source contains the written data, the result status and
     *        optionally a sourcetimestamp
     * @return Returns a status code for logging. Error codes intended for the
     *         original caller are set in the value. If an error is returned,
     *         then no releasing of the value is done. */
    UA_StatusCode (*write)(UA_Server *server, const UA_NodeId *sessionId,
                           void *sessionContext, const UA_NodeId *nodeId,
                           void *nodeContext, const UA_NumericRange *range,
                           const UA_DataValue *value);
} UA_CallbackValueSource;

/**
 * By default, when adding a VariableNode, the value from the
 * ``UA_VariableAttributes`` is used. The methods following afterwards can be
 * used to override the value source. */

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_addVariableNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                          const UA_NodeId parentNodeId,
                          const UA_NodeId referenceTypeId,
                          const UA_QualifiedName browseName,
                          const UA_NodeId typeDefinition,
                          const UA_VariableAttributes attr,
                          void *nodeContext, UA_NodeId *outNewNodeId);

/* Add a VariableNode with a callback value-source */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_addCallbackValueSourceVariableNode(UA_Server *server,
                                             const UA_NodeId requestedNewNodeId,
                                             const UA_NodeId parentNodeId,
                                             const UA_NodeId referenceTypeId,
                                             const UA_QualifiedName browseName,
                                             const UA_NodeId typeDefinition,
                                             const UA_VariableAttributes attr,
                                             const UA_CallbackValueSource evs,
                                             void *nodeContext, UA_NodeId *outNewNodeId);

/* Legacy API */
#define UA_Server_addDataSourceVariableNode(server, requestedNewNodeId, parentNodeId,    \
                                            referenceTypeId, browseName, typeDefinition, \
                                            attr, dataSource, nodeContext, outNewNodeId) \
    UA_Server_addCallbackValueSourceVariableNode(server, requestedNewNodeId,             \
                                                 parentNodeId, referenceTypeId,          \
                                                 browseName, typeDefinition,             \
                                                 attr, dataSource, nodeContext,          \
                                                 outNewNodeId)

/* Set an internal value source. Both the value argument and the notifications
 * argument can be NULL. If value is NULL, the Read service is used to get the
 * latest value before switching from a callback to an internal value source. If
 * notifications is NULL, then all onRead/onWrite notifications are disabled. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_setVariableNode_internalValueSource(UA_Server *server,
    const UA_NodeId nodeId, const UA_DataValue *value,
    const UA_ValueSourceNotifications *notifications);

/* For the external value, no initial copy is made. The node "just" points to
 * the provided double-pointer. Otherwise identical to the internal data
 * source. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_setVariableNode_externalValueSource(UA_Server *server,
    const UA_NodeId nodeId, UA_DataValue **value,
    const UA_ValueSourceNotifications *notifications);

/* It is expected that the read callback is implemented. Whenever the value
 * attribute is read, the function will be called and asked to fill a
 * UA_DataValue structure that contains the value content and additional
 * metadata like timestamps.
 *
 * The write callback can be set to a null-pointer. Then writing into the value
 * is disabled. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_setVariableNode_callbackValueSource(UA_Server *server,
    const UA_NodeId nodeId, const UA_CallbackValueSource evs);

/* Deprecated API */
typedef UA_CallbackValueSource UA_DataSource;
#define UA_Server_setVariableNode_dataSource(server, nodeId, dataSource) \
    UA_Server_setVariableNode_callbackValueSource(server, nodeId, dataSource);

/* Deprecated API */
typedef UA_ValueSourceNotifications UA_ValueCallback;
#define UA_Server_setVariableNode_valueCallback(server, nodeId, callback) \
    UA_Server_setVariableNode_internalValueSource(server, nodeId, NULL, &callback)

/* VariableNodes that are "dynamic" (default for user-created variables) receive
 * and store a SourceTimestamp. For non-dynamic VariableNodes the current time
 * is used for the SourceTimestamp. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_setVariableNodeDynamic(UA_Server *server, const UA_NodeId nodeId,
                                 UA_Boolean isDynamic);

/**
 * VariableTypeNode
 * ~~~~~~~~~~~~~~~~ */

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_addVariableTypeNode(UA_Server *server,
                              const UA_NodeId requestedNewNodeId,
                              const UA_NodeId parentNodeId,
                              const UA_NodeId referenceTypeId,
                              const UA_QualifiedName browseName,
                              const UA_NodeId typeDefinition,
                              const UA_VariableTypeAttributes attr,
                              void *nodeContext, UA_NodeId *outNewNodeId);

/**
 * MethodNode
 * ~~~~~~~~~~ */

typedef UA_StatusCode
(*UA_MethodCallback)(UA_Server *server,
                     const UA_NodeId *sessionId, void *sessionContext,
                     const UA_NodeId *methodId, void *methodContext,
                     const UA_NodeId *objectId, void *objectContext,
                     size_t inputSize, const UA_Variant *input,
                     size_t outputSize, UA_Variant *output);

#ifdef UA_ENABLE_METHODCALLS

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_addMethodNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                        const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                        const UA_QualifiedName browseName, const UA_MethodAttributes attr,
                        UA_MethodCallback method,
                        size_t inputArgumentsSize, const UA_Argument *inputArguments,
                        size_t outputArgumentsSize, const UA_Argument *outputArguments,
                        void *nodeContext, UA_NodeId *outNewNodeId);

/* Extended version, allows the additional definition of fixed NodeIds for the
 * InputArgument/OutputArgument child variables */
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

#endif

/**
 * ObjectNode
 * ~~~~~~~~~~ */

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_addObjectNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                        const UA_NodeId parentNodeId,
                        const UA_NodeId referenceTypeId,
                        const UA_QualifiedName browseName,
                        const UA_NodeId typeDefinition,
                        const UA_ObjectAttributes attr,
                        void *nodeContext, UA_NodeId *outNewNodeId);

/**
 * ObjectTypeNode
 * ~~~~~~~~~~~~~~ */

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_addObjectTypeNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                            const UA_NodeId parentNodeId,
                            const UA_NodeId referenceTypeId,
                            const UA_QualifiedName browseName,
                            const UA_ObjectTypeAttributes attr,
                            void *nodeContext, UA_NodeId *outNewNodeId);

/**
 * ReferenceTypeNode
 * ~~~~~~~~~~~~~~~~~ */

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_addReferenceTypeNode(UA_Server *server,
                               const UA_NodeId requestedNewNodeId,
                               const UA_NodeId parentNodeId,
                               const UA_NodeId referenceTypeId,
                               const UA_QualifiedName browseName,
                               const UA_ReferenceTypeAttributes attr,
                               void *nodeContext, UA_NodeId *outNewNodeId);

/**
 * DataTypeNode
 * ~~~~~~~~~~~~ */

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_addDataTypeNode(UA_Server *server,
                          const UA_NodeId requestedNewNodeId,
                          const UA_NodeId parentNodeId,
                          const UA_NodeId referenceTypeId,
                          const UA_QualifiedName browseName,
                          const UA_DataTypeAttributes attr,
                          void *nodeContext, UA_NodeId *outNewNodeId);

/**
 * ViewNode
 * ~~~~~~~~ */

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_addViewNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                      const UA_NodeId parentNodeId,
                      const UA_NodeId referenceTypeId,
                      const UA_QualifiedName browseName,
                      const UA_ViewAttributes attr,
                      void *nodeContext, UA_NodeId *outNewNodeId);

/**
 * .. _node-lifecycle:
 *
 * Node Lifecycle: Constructors, Destructors and Node Contexts
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * To finalize the instantiation of a node, a (user-defined) constructor
 * callback is executed. There can be both a global constructor for all nodes
 * and node-type constructor specific to the TypeDefinition of the new node
 * (attached to an ObjectTypeNode or VariableTypeNode).
 *
 * In the hierarchy of ObjectTypes and VariableTypes, only the constructor of
 * the (lowest) type defined for the new node is executed. Note that every
 * Object and Variable can have only one ``isTypeOf`` reference. But type-nodes
 * can technically have several ``hasSubType`` references to implement multiple
 * inheritance. Issues of (multiple) inheritance in the constructor need to be
 * solved by the user.
 *
 * When a node is destroyed, the node-type destructor is called before the
 * global destructor. So the overall node lifecycle is as follows:
 *
 * 1. Global Constructor (set in the server config)
 * 2. Node-Type Constructor (for VariableType or ObjectTypes)
 * 3. (Usage-period of the Node)
 * 4. Node-Type Destructor
 * 5. Global Destructor
 *
 * The constructor and destructor callbacks can be set to ``NULL`` and are not
 * used in that case. If the node-type constructor fails, the global destructor
 * will be called before removing the node. The destructors are assumed to never
 * fail.
 *
 * Every node carries a user-context and a constructor-context pointer. The
 * user-context is used to attach custom data to a node. But the (user-defined)
 * constructors and destructors may replace the user-context pointer if they
 * wish to do so. The initial value for the constructor-context is ``NULL``.
 * When the ``AddNodes`` service is used over the network, the user-context
 * pointer of the new node is also initially set to ``NULL``. */

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_getNodeContext(UA_Server *server, UA_NodeId nodeId, void **nodeContext);

/* Careful! The user has to ensure that the destructor callbacks still work. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_setNodeContext(UA_Server *server, UA_NodeId nodeId, void *nodeContext);

/**
 * Global constructor and destructor callbacks used for every node type.
 * It gets set in the server config. */

typedef struct {
    /* Can be NULL. May replace the nodeContext */
    UA_StatusCode (*constructor)(UA_Server *server,
                                 const UA_NodeId *sessionId, void *sessionContext,
                                 const UA_NodeId *nodeId, void **nodeContext);

    /* Can be NULL. The context cannot be replaced since the node is destroyed
     * immediately afterwards anyway. */
    void (*destructor)(UA_Server *server,
                       const UA_NodeId *sessionId, void *sessionContext,
                       const UA_NodeId *nodeId, void *nodeContext);

    /* Can be NULL. Called during recursive node instantiation. While mandatory
     * child nodes are automatically created if not already present, optional child
     * nodes are not. This callback can be used to define whether an optional child
     * node should be created.
     *
     * @param server The server executing the callback
     * @param sessionId The identifier of the session
     * @param sessionContext Additional data attached to the session in the
     *        access control layer
     * @param sourceNodeId Source node from the type definition. If the new node
     *        shall be created, it will be a copy of this node.
     * @param targetParentNodeId Parent of the potential new child node
     * @param referenceTypeId Identifies the reference type which that the parent
     *        node has to the new node.
     * @return Return UA_TRUE if the child node shall be instantiated,
     *         UA_FALSE otherwise. */
    UA_Boolean (*createOptionalChild)(UA_Server *server,
                                      const UA_NodeId *sessionId,
                                      void *sessionContext,
                                      const UA_NodeId *sourceNodeId,
                                      const UA_NodeId *targetParentNodeId,
                                      const UA_NodeId *referenceTypeId);

    /* Can be NULL. Called when a node is to be copied during recursive
     * node instantiation. Allows definition of the NodeId for the new node.
     * If the callback is set to NULL or the resulting NodeId is UA_NODEID_NUMERIC(X,0)
     * an unused nodeid in namespace X will be used. E.g. passing UA_NODEID_NULL will
     * result in a NodeId in namespace 0.
     *
     * @param server The server executing the callback
     * @param sessionId The identifier of the session
     * @param sessionContext Additional data attached to the session in the
     *        access control layer
     * @param sourceNodeId Source node of the copy operation
     * @param targetParentNodeId Parent node of the new node
     * @param referenceTypeId Identifies the reference type which that the parent
     *        node has to the new node. */
    UA_StatusCode (*generateChildNodeId)(UA_Server *server,
                                         const UA_NodeId *sessionId, void *sessionContext,
                                         const UA_NodeId *sourceNodeId,
                                         const UA_NodeId *targetParentNodeId,
                                         const UA_NodeId *referenceTypeId,
                                         UA_NodeId *targetNodeId);
} UA_GlobalNodeLifecycle;

/**
 * The following node-type lifecycle can be set for VariableTypeNodes and
 * ObjectTypeNodes. It gets called for instances of this node-type. */

typedef struct {
    /* Can be NULL. May replace the nodeContext */
    UA_StatusCode (*constructor)(UA_Server *server,
                                 const UA_NodeId *sessionId, void *sessionContext,
                                 const UA_NodeId *typeNodeId, void *typeNodeContext,
                                 const UA_NodeId *nodeId, void **nodeContext);

    /* Can be NULL. May replace the nodeContext. */
    void (*destructor)(UA_Server *server,
                       const UA_NodeId *sessionId, void *sessionContext,
                       const UA_NodeId *typeNodeId, void *typeNodeContext,
                       const UA_NodeId *nodeId, void **nodeContext);
} UA_NodeTypeLifecycle;

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_setNodeTypeLifecycle(UA_Server *server, UA_NodeId nodeId,
                               UA_NodeTypeLifecycle lifecycle);

/**
 * Detailed Node Construction
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~
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
 * ~~~~~~~~~~~~~~~~~~~~ */

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
 * .. _async-operations:
 *
 * Async Operations
 * ----------------
 * Some operations can take time, such as reading a sensor that needs to warm up
 * first. In order not to block the server, a long-running operation can be
 * handled asynchronously and the result returned at a later time. The core idea
 * is that a userland callback can return
 * UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY as the statuscode to signal that it
 * wishes to complete the operation later.
 *
 * Currently, async operations are supported for the services
 *
 * - Read
 * - Write
 * - Call
 *
 * with the caveat that read/write need a CallbackValueSource registered for the
 * variable. Values that are stored directly in a VariableNode are written and
 * read immediately.
 *
 * Note that an async operation can be cancelled (e.g. after a timeout period or
 * if the caller cannot wait for the result). This is signaled in the configured
 * ``asyncOperationCancelCallback``. The provided memory locations to store the
 * operation output are then no longer valid. */

/* When the UA_MethodCallback returns UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY,
 * then an async operation is created in the server for later completion. The
 * output pointer from the method callback is used to identify the async
 * operation. Do not access the output pointer after the operation has been
 * cancelled or after setting the result. */
UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_setAsyncCallMethodResult(UA_Server *server, UA_Variant *output,
                                   UA_StatusCode result);

/* See the UA_CallbackValueSource documentation */
UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_setAsyncReadResult(UA_Server *server, UA_DataValue *result);

/* See the UA_CallbackValueSource documentation. The value needs to be the
 * pointer used in the write callback. The statuscode is the result signal to be
 * returned asynchronously. */
UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_setAsyncWriteResult(UA_Server *server, const UA_DataValue *value,
                              UA_StatusCode result);

/**
 * The server supports asynchronous "local" read/write/call operations. The user
 * supplies a result-callback that gets called either synchronously (if the
 * operation terminates right away) or asynchronously at a later time. The
 * result-callback is called exactly one time for each operation, also if the
 * operation is cancelled. In this case a StatusCode like
 * ``UA_STATUSCODE_BADTIMEOUT`` or ``UA_STATUSCODE_BADSHUTDOWN`` is set.
 *
 * If an operation returns asynchronously, then the result-callback is executed
 * only in the next iteration of the Eventloop. An exception to this is
 * UA_Server_cancelAsync, which can optionally call the result-callback right
 * away (e.g. as part of a cleanup where the context of the result-callback gets
 * removed).
 *
 * Async operations incur a small overhead since memory is allocated to persist
 * the operation over time.
 *
 * The operation timeout is defined in milliseconds. A timeout of zero means
 * infinite. */

typedef void(*UA_ServerAsyncReadResultCallback)
    (UA_Server *server, void *asyncOpContext, const UA_DataValue *result);
typedef void(*UA_ServerAsyncWriteResultCallback)
    (UA_Server *server, void *asyncOpContext, UA_StatusCode result);
typedef void(*UA_ServerAsyncMethodResultCallback)
    (UA_Server *server, void *asyncOpContext, const UA_CallMethodResult *result);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_read_async(UA_Server *server, const UA_ReadValueId *operation,
                     UA_TimestampsToReturn timestamps,
                     UA_ServerAsyncReadResultCallback callback,
                     void *asyncOpContext, UA_UInt32 timeout);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_write_async(UA_Server *server, const UA_WriteValue *operation,
                      UA_ServerAsyncWriteResultCallback callback,
                      void *asyncOpContext, UA_UInt32 timeout);

#ifdef UA_ENABLE_METHODCALLS
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_call_async(UA_Server *server, const UA_CallMethodRequest *operation,
                     UA_ServerAsyncMethodResultCallback callback,
                     void *asyncOpContext, UA_UInt32 timeout);
#endif

/**
 * Local async operations can be manually cancelled (besides an internal cancel
 * due to a timeout or server shutdown). The local async operations to be
 * cancelled are selected by matching their asyncOpContext pointer. This can
 * cancel multiple operations that use the same context pointer.
 *
 * For operations where the async result was not yet set, the
 * asyncOperationCancelCallback from the server-config gets called and the
 * cancel-status is set in the operation result.
 *
 * For async operations where the result has already been set, but not yet
 * notified with the result-callback (to be done in the next EventLoop
 * iteration), the asyncOperationCancelCallback is not called and no cancel
 * status is set in the result.
 *
 * Each operation's result-callback gets called exactly once. When the operation
 * is cancelled, the result-callback can be called synchronously using the
 * synchronousResultCallback flag. Otherwise the result gets returned "normally"
 * in the next EventLoop iteration. The synchronous option ensures that all
 * (matching) async operations are fully cancelled right away. This can be
 * important in a cleanup situation where the asyncOpContext is no longer valid
 * in the future. */

void UA_EXPORT UA_THREADSAFE
UA_Server_cancelAsync(UA_Server *server, void *asyncOpContext,
                      UA_StatusCode status,
                      UA_Boolean synchronousResultCallback);

/**
 * .. _events:
 *
 * Events
 * ------
 * Events are emitted by objects in the OPC UA information model. Starting at
 * the source-node, the events "bubble up" in the hierarchy of objects and are
 * caught by MonitoredItems listening for them.
 *
 * EventTypes are special ObjectTypeNodes that describe the (data) fields of an
 * event instance. An EventType can simply contain a flat list of VariableNodes.
 * But (deep) nesting of objects and variables is also allowed. The individual
 * MonitoredItems then contain an EventFilter (with a select-clause) that
 * defines the event fields to be transmitted to a particular client.
 *
 * In open62541, there are three possible sources for the event fields. When the
 * select-clause of an EventFilter is resolved, the sources are evaluated in the
 * following order:
 *
 * 1. An key-value map that defines event fields. The key of its entries is a
 *    "path-string", a :ref:``human-readable encoding of a
 *    SimpleAttributeOperand<parse-sao>`. For example ``/SourceNode`` or
 *    ``/EventType``.
 * 2. An NodeId pointing to an ObjectNode that instantiates an EventType. The
 *    ``SimpleAttributeOperands`` from the EventFilter are resolved in its
 *    context.
 * 3. The event fields defined as mandatory for the *BaseEventType* have a
 *    default that gets used if they are not defined otherwise:
 *
 *    /EventId
 *       ByteString to uniquely identify the event instance
 *       (default: random 16-byte ByteString)
 *
 *    /EventType
 *       NodeId of the EventType (default: argument of ``_createEvent``)
 *
 *    /SourceNode
 *       NodeId of the emitting node (default: argument of ``_createEvent``)
 *
 *    /SourceName
 *       LocalizedText with the DisplayName of the source node
 *       (default: read from the information model)
 *
 *    /Time
 *       DateTime with the timestamp when the event occurred
 *       (default: current time)
 *
 *    /ReceiveTime
 *       DateTime when the server received the information about the event from an
 *       underlying device (default: current time)
 *
 *    /Message
 *       LocalizedText with a human-readable description of the event (default:
 *       argument of ``_createEvent``)
 *
 *    /Severity
 *       UInt16 for the urgency of the event defined to be between 1 (lowest) and
 *       1000 (catastrophic) (default: argument of ``_createEvent``)
 *
 * An event field that is missing from all sources resolves to an empty variant.
 *
 * It is typically faster to define event-fields in the key-value map than to
 * look them up from an event instance in the information model. This is
 * particularly important for events emitted at a high frequency. */

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS

/* Create an event in the server. The eventFields and eventInstance pointer can
 * be NULL and are then not considered as a source of event fields. The
 * outEventId pointer can be NULL. If set, the EventId of a successfully created
 * Event gets copied into the argument. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_createEvent(UA_Server *server, const UA_NodeId sourceNode,
                      const UA_NodeId eventType, UA_UInt16 severity,
                      const UA_LocalizedText message,
                      const UA_KeyValueMap *eventFields,
                      const UA_NodeId *eventInstance,
                      UA_ByteString *outEventId);

/* Extended version of the _createEvent API. The members of the
 * UA_EventDescription structure have the same meaning as above.
 *
 * In addition, the extended version allows the filtering of Events to be only
 * transmitted to a particular Session/Subscription/MonitoredItem. The filtering
 * criteria can be NULL. But the subscriptionId requires a sessionId and the
 * monitoredItemId requires a subscriptionId as context. */

typedef struct {
    /* Event fields */
    UA_NodeId sourceNode;
    UA_NodeId eventType;
    UA_UInt16 severity;
    UA_LocalizedText message;
    const UA_KeyValueMap *eventFields;
    const UA_NodeId *eventInstance;

    /* Restrict who can receive the event */
    const UA_NodeId *sessionId;
    const UA_UInt32 *subscriptionId;
    const UA_UInt32 *monitoredItemId;
} UA_EventDescription;

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_createEventEx(UA_Server *server,
                        const UA_EventDescription *ed,
                        UA_ByteString *outEventId);

#endif /* UA_ENABLE_SUBSCRIPTIONS_EVENTS */

#ifdef UA_ENABLE_DISCOVERY

/**
 * Discovery
 * ---------
 *
 * Registering at a Discovery Server
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

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
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

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
 * Statistics
 * ----------
 * Statistic counters keeping track of the current state of the stack. Counters
 * are structured per OPC UA communication layer. */

typedef struct {
   UA_SecureChannelStatistics scs;
   UA_SessionStatistics ss;
} UA_ServerStatistics;

UA_ServerStatistics UA_EXPORT UA_THREADSAFE
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
 * Some convenience functions are provided to simplify the interaction with
 * objects. */

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

    /* Server Description
     * ~~~~~~~~~~~~~~~~~~
     * The description must be internally consistent. The ApplicationUri set in
     * the ApplicationDescription must match the URI set in the server
     * certificate.
     * The applicationType is not just descriptive, it changes the actual
     * functionality of the server. The RegisterServer service is available only
     * if the server is a DiscoveryServer and the applicationType is set to the
     * appropriate value.*/
    UA_BuildInfo buildInfo;
    UA_ApplicationDescription applicationDescription;

    /* Server Lifecycle
     * ~~~~~~~~~~~~~~~~
     * Delay in ms from the shutdown signal (ctrl-c) until the actual shutdown.
     * Clients need to be able to get a notification ahead of time. */
    UA_Double shutdownDelay;

    /* If an asynchronous server shutdown is used, this callback notifies about
     * the current lifecycle state (notably the STOPPING -> STOPPED
     * transition). */
    void (*notifyLifecycleState)(UA_Server *server, UA_LifecycleState state);

    /* Rule Handling
     * ~~~~~~~~~~~~~
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
    UA_DataTypeArray *customDataTypes;

    /* EventLoop
     * ~~~~~~~~~
     * The sever can be plugged into an external EventLoop. Otherwise the
     * EventLoop is considered to be attached to the server's lifecycle and will
     * be destroyed when the config is cleaned up. */
    UA_EventLoop *eventLoop;
    UA_Boolean externalEventLoop; /* The EventLoop is not deleted with the config */

    /* Application Notification
     * ~~~~~~~~~~~~~~~~~~~~~~~~
     * The notification callbacks can be NULL. The global callback receives all
     * notifications. The specialized callbacks receive only the subset
     * indicated by their name. */
    UA_ServerNotificationCallback globalNotificationCallback;
    UA_ServerNotificationCallback lifecycleNotificationCallback;
    UA_ServerNotificationCallback serviceNotificationCallback;

    /* Networking
     * ~~~~~~~~~~
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

    /* The following settings are specific to OPC UA with TCP transport. */
    UA_Boolean tcpEnabled;
    UA_UInt32 tcpBufSize;    /* Max length of sent and received chunks (packets)
                              * (default: 64kB) */
    UA_UInt32 tcpMaxMsgSize; /* Max length of messages
                              * (default: 0 -> unbounded) */
    UA_UInt32 tcpMaxChunks;  /* Max number of chunks per message
                              * (default: 0 -> unbounded) */
    UA_Boolean tcpReuseAddr;

    /* Security and Encryption
     * ~~~~~~~~~~~~~~~~~~~~~~~ */
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

    /* See the AccessControl Plugin API */
    UA_AccessControl accessControl;

    /* Nodes and Node Lifecycle
     * ~~~~~~~~~~~~~~~~~~~~~~~~
     * See the section for :ref:`node lifecycle handling<node-lifecycle>`. */
    UA_Nodestore *nodestore;
    UA_GlobalNodeLifecycle *nodeLifecycle;

    /* Copy the HasModellingRule reference in instances from the type
     * definition in UA_Server_addObjectNode and UA_Server_addVariableNode.
     *
     * Part 3 - 6.4.4: [...] it is not required that newly created or referenced
     * instances based on InstanceDeclarations have a ModellingRule, however, it
     * is allowed that they have any ModellingRule independent of the
     * ModellingRule of their InstanceDeclaration */
    UA_Boolean modellingRulesOnInstances;

    /* Limits
     * ~~~~~~ */
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

#ifdef UA_ENABLE_ENCRYPTION
    /* Limits for TrustList */
    UA_UInt32 maxTrustListSize; /* in bytes, 0 => unlimited */
    UA_UInt32 maxRejectedListSize; /* 0 => unlimited */
#endif

    /* Async Operations
     * ~~~~~~~~~~~~~~~~
     * See the section for :ref:`async operations<async-operations>`. */
    UA_Double asyncOperationTimeout;   /* in ms, 0 => unlimited */
    size_t maxAsyncOperationQueueSize; /* 0 => unlimited */

    /* Notifies the userland that an async operation has been canceled. The
     * memory for setting the output value is then freed internally and should
     * not be touched afterwards. */
    void (*asyncOperationCancelCallback)(UA_Server *server, const void *out);

    /* Discovery
     * ~~~~~~~~~ */
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
#  ifdef UA_ENABLE_DISCOVERY_MULTICAST_MDNSD
    UA_String mdnsInterfaceIP;
    UA_Boolean mdnsSendToAllInterfaces;
#   if !defined(UA_HAS_GETIFADDR)
    size_t mdnsIpAddressListSize;
    UA_UInt32 *mdnsIpAddressList;
#   endif
#  endif
# endif
#endif

    /* Subscriptions
     * ~~~~~~~~~~~~~ */
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

    /* PubSub
     * ~~~~~~ */
#ifdef UA_ENABLE_PUBSUB
    UA_Boolean pubsubEnabled;
    UA_PubSubConfiguration pubSubConfig;
#endif

    /* Historical Access
     * ~~~~~~~~~~~~~~~~~ */
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

    /* Reverse Connect
     * ~~~~~~~~~~~~~~~ */
    UA_UInt32 reverseReconnectInterval; /* Default is 15000 ms */

    /* Certificate Password Callback
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
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
UA_ServerConfig_clear(UA_ServerConfig *config);

UA_DEPRECATED static UA_INLINE void
UA_ServerConfig_clean(UA_ServerConfig *config) {
    UA_ServerConfig_clear(config);
}

/**
 * Update the Server Certificate at Runtime
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * If certificateGroupId is null the DefaultApplicationGroup is used.
 */

UA_StatusCode UA_EXPORT
UA_Server_updateCertificate(UA_Server *server,
                            const UA_NodeId certificateGroupId,
                            const UA_NodeId certificateTypeId,
                            const UA_ByteString certificate,
                            const UA_ByteString *privateKey);

/* Creates a PKCS #10 DER encoded certificate request signed with the server's
 * private key.
 * If certificateGroupId is null the DefaultApplicationGroup is used.
 */
UA_StatusCode UA_EXPORT
UA_Server_createSigningRequest(UA_Server *server,
                               const UA_NodeId certificateGroupId,
                               const UA_NodeId certificateTypeId,
                               const UA_String *subjectName,
                               const UA_Boolean *regenerateKey,
                               const UA_ByteString *nonce,
                               UA_ByteString *csr);

/* Adds certificates and Certificate Revocation Lists (CRLs) to a specific
 * certificate group on the server.
 *
 * @param server The server object
 * @param certificateGroupId The NodeId of the certificate group where
 *        certificates will be added
 * @param certificates The certificates to be added
 * @param certificatesSize The number of certificates
 * @param crls The associated CRLs for the certificates, required when adding
 *        issuer certificates
 * @param crlsSize The number of CRLs
 * @param isTrusted Indicates whether the certificates should be added to the
 *        trusted list or the issuer list
 * @param appendCertificates Indicates whether the certificates should be added
 *        to the list or replace the existing list
 * @return ``UA_STATUSCODE_GOOD`` on success */
UA_StatusCode UA_EXPORT
UA_Server_addCertificates(UA_Server *server,
                          const UA_NodeId certificateGroupId,
                          UA_ByteString *certificates,
                          size_t certificatesSize,
                          UA_ByteString *crls,
                          size_t crlsSize,
                          const UA_Boolean isTrusted,
                          const UA_Boolean appendCertificates);

/* Removes certificates from a specific certificate group on the server. The
 * corresponding CRLs are removed automatically.
 *
 * @param server The server object
 * @param certificateGroupId The NodeId of the certificate group from which
 *        certificates will be removed
 * @param certificates The certificates to be removed
 * @param certificatesSize The number of certificates
 * @param isTrusted Indicates whether the certificates are being removed from
 *        the trusted list or the issuer list
 * @return ``UA_STATUSCODE_GOOD`` on success */
UA_StatusCode UA_EXPORT
UA_Server_removeCertificates(UA_Server *server,
                             const UA_NodeId certificateGroupId,
                             UA_ByteString *certificates,
                             size_t certificatesSize,
                             const UA_Boolean isTrusted);


_UA_END_DECLS

#ifdef UA_ENABLE_PUBSUB
#include <open62541/server_pubsub.h>
#endif

#endif /* UA_SERVER_H_ */
