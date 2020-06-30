/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2014-2015, 2017 (c) Florian Palm
 *    Copyright 2015-2016 (c) Chris Iatrou
 *    Copyright 2015-2016 (c) Oleksiy Vasylyev
 *    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017-2020 (c) HMS Industrial Networks AB (Author: Jonas Green)
 */

#ifndef UA_SERVER_H_
#define UA_SERVER_H_

#include <open62541/nodeids.h>
#include <open62541/types.h>
#include <open62541/types_generated.h>
#include <open62541/types_generated_handling.h>

_UA_BEGIN_DECLS

/* Forward declarations */
struct UA_ServerConfig;
typedef struct UA_ServerConfig UA_ServerConfig;

struct UA_Server;
typedef struct UA_Server UA_Server;

struct UA_Client;

/**
 * .. _server:
 *
 * Server
 * ======
 *
 * .. include:: server_config.rst
 *
 * .. _server-lifecycle:
 *
 * Server Lifecycle
 * ---------------- */

/* The method UA_Server_new is defined in server_config_default.h. So default
 * plugins outside of the core library (for logging, etc) are already available
 * during the initialization.
 *
 * UA_Server UA_EXPORT * UA_Server_new(void);
 */

/* Creates a new server. Moves the config into the server with a shallow copy.
 * The config content is cleared together with the server. */
UA_Server UA_EXPORT *
UA_Server_newWithConfig(const UA_ServerConfig *config);

void UA_EXPORT UA_Server_delete(UA_Server *server);

UA_ServerConfig UA_EXPORT *
UA_Server_getConfig(UA_Server *server);

/* Runs the main loop of the server. In each iteration, this calls into the
 * networklayers to see if messages have arrived.
 *
 * @param server The server object.
 * @param running The loop is run as long as *running is true.
 *        Otherwise, the server shuts down.
 * @return Returns the statuscode of the UA_Server_run_shutdown method */
UA_StatusCode UA_EXPORT
UA_Server_run(UA_Server *server, const volatile UA_Boolean *running);

/* The prologue part of UA_Server_run (no need to use if you call
 * UA_Server_run) */
UA_StatusCode UA_EXPORT
UA_Server_run_startup(UA_Server *server);

/* Executes a single iteration of the server's main loop.
 *
 * @param server The server object.
 * @param waitInternal Should we wait for messages in the networklayer?
 *        Otherwise, the timouts for the networklayers are set to zero.
 *        The default max wait time is 50millisec.
 * @return Returns how long we can wait until the next scheduled
 *         callback (in ms) */
UA_UInt16 UA_EXPORT
UA_Server_run_iterate(UA_Server *server, UA_Boolean waitInternal);

/* The epilogue part of UA_Server_run (no need to use if you call
 * UA_Server_run) */
UA_StatusCode UA_EXPORT
UA_Server_run_shutdown(UA_Server *server);

/**
 * Timed Callbacks
 * --------------- */
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
 * @return Upon success, UA_STATUSCODE_GOOD is returned. An error code
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
 * @return Upon success, UA_STATUSCODE_GOOD is returned. An error code
 *         otherwise. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_addRepeatedCallback(UA_Server *server, UA_ServerCallback callback,
                              void *data, UA_Double interval_ms, UA_UInt64 *callbackId);

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

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_readNodeId(UA_Server *server, const UA_NodeId nodeId,
                     UA_NodeId *outNodeId) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_NODEID, outNodeId);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_readNodeClass(UA_Server *server, const UA_NodeId nodeId,
                        UA_NodeClass *outNodeClass) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_NODECLASS,
                            outNodeClass);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_readBrowseName(UA_Server *server, const UA_NodeId nodeId,
                         UA_QualifiedName *outBrowseName) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_BROWSENAME,
                            outBrowseName);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_readDisplayName(UA_Server *server, const UA_NodeId nodeId,
                          UA_LocalizedText *outDisplayName) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_DISPLAYNAME,
                            outDisplayName);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_readDescription(UA_Server *server, const UA_NodeId nodeId,
                          UA_LocalizedText *outDescription) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_DESCRIPTION,
                            outDescription);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_readWriteMask(UA_Server *server, const UA_NodeId nodeId,
                        UA_UInt32 *outWriteMask) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_WRITEMASK,
                            outWriteMask);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_readIsAbstract(UA_Server *server, const UA_NodeId nodeId,
                         UA_Boolean *outIsAbstract) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_ISABSTRACT,
                            outIsAbstract);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_readSymmetric(UA_Server *server, const UA_NodeId nodeId,
                        UA_Boolean *outSymmetric) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_SYMMETRIC,
                            outSymmetric);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_readInverseName(UA_Server *server, const UA_NodeId nodeId,
                          UA_LocalizedText *outInverseName) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_INVERSENAME,
                            outInverseName);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_readContainsNoLoops(UA_Server *server, const UA_NodeId nodeId,
                              UA_Boolean *outContainsNoLoops) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_CONTAINSNOLOOPS,
                            outContainsNoLoops);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_readEventNotifier(UA_Server *server, const UA_NodeId nodeId,
                            UA_Byte *outEventNotifier) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_EVENTNOTIFIER,
                            outEventNotifier);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_readValue(UA_Server *server, const UA_NodeId nodeId,
                    UA_Variant *outValue) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_VALUE, outValue);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_readDataType(UA_Server *server, const UA_NodeId nodeId,
                       UA_NodeId *outDataType) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_DATATYPE,
                            outDataType);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_readValueRank(UA_Server *server, const UA_NodeId nodeId,
                        UA_Int32 *outValueRank) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_VALUERANK,
                            outValueRank);
}

/* Returns a variant with an int32 array */
static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_readArrayDimensions(UA_Server *server, const UA_NodeId nodeId,
                              UA_Variant *outArrayDimensions) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_ARRAYDIMENSIONS,
                            outArrayDimensions);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_readAccessLevel(UA_Server *server, const UA_NodeId nodeId,
                          UA_Byte *outAccessLevel) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_ACCESSLEVEL,
                            outAccessLevel);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_readMinimumSamplingInterval(UA_Server *server, const UA_NodeId nodeId,
                                      UA_Double *outMinimumSamplingInterval) {
    return __UA_Server_read(server, &nodeId,
                            UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL,
                            outMinimumSamplingInterval);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_readHistorizing(UA_Server *server, const UA_NodeId nodeId,
                          UA_Boolean *outHistorizing) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_HISTORIZING,
                            outHistorizing);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_readExecutable(UA_Server *server, const UA_NodeId nodeId,
                         UA_Boolean *outExecutable) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_EXECUTABLE,
                            outExecutable);
}

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

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_writeBrowseName(UA_Server *server, const UA_NodeId nodeId,
                          const UA_QualifiedName browseName) {
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_BROWSENAME,
                             &UA_TYPES[UA_TYPES_QUALIFIEDNAME], &browseName);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_writeDisplayName(UA_Server *server, const UA_NodeId nodeId,
                           const UA_LocalizedText displayName) {
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_DISPLAYNAME,
                             &UA_TYPES[UA_TYPES_LOCALIZEDTEXT], &displayName);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_writeDescription(UA_Server *server, const UA_NodeId nodeId,
                           const UA_LocalizedText description) {
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_DESCRIPTION,
                             &UA_TYPES[UA_TYPES_LOCALIZEDTEXT], &description);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_writeWriteMask(UA_Server *server, const UA_NodeId nodeId,
                         const UA_UInt32 writeMask) {
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_WRITEMASK,
                             &UA_TYPES[UA_TYPES_UINT32], &writeMask);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_writeIsAbstract(UA_Server *server, const UA_NodeId nodeId,
                          const UA_Boolean isAbstract) {
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_ISABSTRACT,
                             &UA_TYPES[UA_TYPES_BOOLEAN], &isAbstract);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_writeInverseName(UA_Server *server, const UA_NodeId nodeId,
                           const UA_LocalizedText inverseName) {
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_INVERSENAME,
                             &UA_TYPES[UA_TYPES_LOCALIZEDTEXT], &inverseName);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_writeEventNotifier(UA_Server *server, const UA_NodeId nodeId,
                             const UA_Byte eventNotifier) {
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_EVENTNOTIFIER,
                             &UA_TYPES[UA_TYPES_BYTE], &eventNotifier);
}

/**
 * Writes an UA_Variant to a variable/variableType node.
 * StatusCode is set to UA_STATUSCODE_GOOD, sourceTimestamp and
 * serverTimestamp are set to UA_DateTime_now()
 */
static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_writeValue(UA_Server *server, const UA_NodeId nodeId,
                     const UA_Variant value) {
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_VALUE,
                             &UA_TYPES[UA_TYPES_VARIANT], &value);
}

/**
 * Writes an UA_DataValue to a variable/variableType node.
 * In contrast to UA_Server_writeValue, this functions can also write
 * sourceTimestamp, serverTimestamp and statusCode.
 */
static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_writeDataValue(UA_Server *server, const UA_NodeId nodeId,
                     const UA_DataValue value) {
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_VALUE,
                             &UA_TYPES[UA_TYPES_DATAVALUE], &value);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_writeDataType(UA_Server *server, const UA_NodeId nodeId,
                        const UA_NodeId dataType) {
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_DATATYPE,
                             &UA_TYPES[UA_TYPES_NODEID], &dataType);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_writeValueRank(UA_Server *server, const UA_NodeId nodeId,
                         const UA_Int32 valueRank) {
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_VALUERANK,
                             &UA_TYPES[UA_TYPES_INT32], &valueRank);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_writeArrayDimensions(UA_Server *server, const UA_NodeId nodeId,
                               const UA_Variant arrayDimensions) {
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_ARRAYDIMENSIONS,
                             &UA_TYPES[UA_TYPES_VARIANT], &arrayDimensions);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_writeAccessLevel(UA_Server *server, const UA_NodeId nodeId,
                           const UA_Byte accessLevel) {
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_ACCESSLEVEL,
                             &UA_TYPES[UA_TYPES_BYTE], &accessLevel);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_writeMinimumSamplingInterval(UA_Server *server, const UA_NodeId nodeId,
                                       const UA_Double miniumSamplingInterval) {
    return __UA_Server_write(server, &nodeId,
                             UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL,
                             &UA_TYPES[UA_TYPES_DOUBLE],
                             &miniumSamplingInterval);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_writeHistorizing(UA_Server *server, const UA_NodeId nodeId,
                          const UA_Boolean historizing) {
    return __UA_Server_write(server, &nodeId,
                             UA_ATTRIBUTEID_HISTORIZING,
                             &UA_TYPES[UA_TYPES_BOOLEAN],
                             &historizing);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_writeExecutable(UA_Server *server, const UA_NodeId nodeId,
                          const UA_Boolean executable) {
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_EXECUTABLE,
                             &UA_TYPES[UA_TYPES_BOOLEAN], &executable); }

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

/* Nonstandard version of the browse service that recurses into child nodes.
 * Possible loops (that can occur for non-hierarchical references) are handled
 * by adding every target node at most once to the results array. */
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
 * --------- */
/* Register the given server instance at the discovery server.
 * This should be called periodically.
 * The semaphoreFilePath is optional. If the given file is deleted,
 * the server will automatically be unregistered. This could be
 * for example a pid file which is deleted if the server crashes.
 *
 * When the server shuts down you need to call unregister.
 *
 * @param server
 * @param client the client which is used to call the RegisterServer. It must
 *        already be connected to the correct endpoint
 * @param semaphoreFilePath optional parameter pointing to semaphore file. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_register_discovery(UA_Server *server, struct UA_Client *client,
                             const char* semaphoreFilePath);

/* Unregister the given server instance from the discovery server.
 * This should only be called when the server is shutting down.
 * @param server
 * @param client the client which is used to call the RegisterServer. It must
 *        already be connected to the correct endpoint */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_unregister_discovery(UA_Server *server, struct UA_Client *client);

 /* Adds a periodic callback to register the server with the LDS (local discovery server)
  * periodically. The interval between each register call is given as second parameter.
  * It should be 10 minutes by default (= 10*60*1000).
  *
  * The delayFirstRegisterMs parameter indicates the delay for the first register call.
  * If it is 0, the first register call will be after intervalMs milliseconds,
  * otherwise the server's first register will be after delayFirstRegisterMs.
  *
  * When you manually unregister the server, you also need to cancel the
  * periodic callback, otherwise it will be automatically be registered again.
  *
  * If you call this method multiple times for the same discoveryServerUrl, the older
  * periodic callback will be removed.
  *
  * @param server
  * @param client the client which is used to call the RegisterServer.
  *         It must not yet be connected and will be connected for every register call
  *         to the given discoveryServerUrl.
  * @param discoveryServerUrl where this server should register itself.
  *        The string will be copied internally. Therefore you can free it after calling this method.
  * @param intervalMs
  * @param delayFirstRegisterMs
  * @param periodicCallbackId */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_addPeriodicServerRegisterCallback(UA_Server *server, struct UA_Client *client,
                                            const char* discoveryServerUrl,
                                            UA_Double intervalMs,
                                            UA_Double delayFirstRegisterMs,
                                            UA_UInt64 *periodicCallbackId);

/* Callback for RegisterServer. Data is passed from the register call */
typedef void (*UA_Server_registerServerCallback)(const UA_RegisteredServer *registeredServer,
                                                 void* data);

/* Set the callback which is called if another server registeres or unregisters
 * with this instance. This callback is called every time the server gets a register
 * call. This especially means that for every periodic server register the callback will
 * be called.
 *
 * @param server
 * @param cb the callback
 * @param data data passed to the callback
 * @return UA_STATUSCODE_SUCCESS on success */
void UA_EXPORT UA_THREADSAFE
UA_Server_setRegisterServerCallback(UA_Server *server, UA_Server_registerServerCallback cb,
                                    void* data);

#ifdef UA_ENABLE_DISCOVERY_MULTICAST

/* Callback for server detected through mDNS. Data is passed from the register
 * call
 *
 * @param isServerAnnounce indicates if the server has just been detected. If
 *        set to false, this means the server is shutting down.
 * @param isTxtReceived indicates if we already received the corresponding TXT
 *        record with the path and caps data */
typedef void (*UA_Server_serverOnNetworkCallback)(const UA_ServerOnNetwork *serverOnNetwork,
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
 * @return UA_STATUSCODE_SUCCESS on success */
void UA_EXPORT UA_THREADSAFE
UA_Server_setServerOnNetworkCallback(UA_Server *server,
                                     UA_Server_serverOnNetworkCallback cb,
                                     void* data);

#endif /* UA_ENABLE_DISCOVERY_MULTICAST */

#endif /* UA_ENABLE_DISCOVERY */

/**
 * Information Model Callbacks
 * ---------------------------
 *
 * There are three places where a callback from an information model to
 * user-defined code can happen.
 *
 * - Custom node constructors and destructors
 * - Linking VariableNodes with an external data source
 * - MethodNode callbacks
 *
 * .. _node-lifecycle:
 *
 * Node Lifecycle: Constructors, Destructors and Node Contexts
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *
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

/* To be set in the server config. */
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
     * @return Return UA_TRUE if the child node shall be instantiatet, 
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

void UA_EXPORT
UA_Server_setAdminSessionContext(UA_Server *server,
                                 void *context);

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
 *
 * The server has a unique way of dealing with the content of variables. Instead
 * of storing a variant attached to the variable node, the node can point to a
 * function with a local data provider. Whenever the value attribute is read,
 * the function will be called and asked to provide a UA_DataValue return value
 * that contains the value content and additional timestamps.
 *
 * It is expected that the read callback is implemented. The write callback can
 * be set to a null-pointer. */
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
     *         then no releasing of the value is done
     */
    UA_StatusCode (*read)(UA_Server *server, const UA_NodeId *sessionId,
                          void *sessionContext, const UA_NodeId *nodeId,
                          void *nodeContext, UA_Boolean includeSourceTimeStamp,
                          const UA_NumericRange *range, UA_DataValue *value);

    /* Write into a data source. This method pointer can be NULL if the
     * operation is unsupported.
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
     *         then no releasing of the value is done
     */
    UA_StatusCode (*write)(UA_Server *server, const UA_NodeId *sessionId,
                           void *sessionContext, const UA_NodeId *nodeId,
                           void *nodeContext, const UA_NumericRange *range,
                           const UA_DataValue *value);
} UA_DataSource;

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_setVariableNode_dataSource(UA_Server *server, const UA_NodeId nodeId,
                                     const UA_DataSource dataSource);

/**
 * .. _value-callback:
 *
 * Value Callback
 * ^^^^^^^^^^^^^^
 * Value Callbacks can be attached to variable and variable type nodes. If
 * not ``NULL``, they are called before reading and after writing respectively. */
typedef struct {
    /* Called before the value attribute is read. It is possible to write into the
     * value attribute during onRead (using the write service). The node is
     * re-opened afterwards so that changes are considered in the following read
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

    /* Called after writing the value attribute. The node is re-opened after
     * writing so that the new value is visible in the callback.
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
} UA_ValueCallback;

/**
 * .. _value-callback:
 *
 * Value Callback
 * ^^^^^^^^^^^^^^
 * Value Callbacks can be attached to variable and variable type nodes. If
 * not ``NULL``, they are called before reading and after writing respectively. */
typedef struct {
    /* Called before the value attribute is read. The external value source can be
     * be updated and/or locked during this notification call. After this function returns
     * to the core, the external value source is readed immediately.
    */
    UA_StatusCode (*notificationRead)(UA_Server *server, const UA_NodeId *sessionId,
                                      void *sessionContext, const UA_NodeId *nodeid,
                                      void *nodeContext, const UA_NumericRange *range);

    /* Called after writing the value attribute. The node is re-opened after
     * writing so that the new value is visible in the callback.
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
    UA_StatusCode (*userWrite)(UA_Server *server, const UA_NodeId *sessionId,
                               void *sessionContext, const UA_NodeId *nodeId,
                               void *nodeContext, const UA_NumericRange *range,
                               const UA_DataValue *data);
} UA_ExternalValueCallback;

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_setVariableNode_valueCallback(UA_Server *server,
                                        const UA_NodeId nodeId,
                                        const UA_ValueCallback callback);

typedef enum {
    UA_VALUEBACKENDTYPE_NONE,
    UA_VALUEBACKENDTYPE_INTERNAL,
    UA_VALUEBACKENDTYPE_DATA_SOURCE_CALLBACK,
    UA_VALUEBACKENDTYPE_EXTERNAL
} UA_ValueBackendType;

typedef struct {
    UA_ValueBackendType backendType;
    union {
        struct {
            UA_DataValue value;
            UA_ValueCallback callback;
        } internal;
        UA_DataSource dataSource;
        struct {
            UA_DataValue **value;
            UA_ExternalValueCallback callback;
        } external;
    } backend;
} UA_ValueBackend;

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_setVariableNode_valueBackend(UA_Server *server,
                                       const UA_NodeId nodeId,
                                       const UA_ValueBackend valueBackend);

/**
 * .. _local-monitoreditems:
 *
 * Local MonitoredItems
 * ^^^^^^^^^^^^^^^^^^^^
 *
 * MonitoredItems are used with the Subscription mechanism of OPC UA to
 * transported notifications for data changes and events. MonitoredItems can
 * also be registered locally. Notifications are then forwarded to a
 * user-defined callback instead of a remote client. */

#ifdef UA_ENABLE_SUBSCRIPTIONS

typedef void (*UA_Server_DataChangeNotificationCallback)
    (UA_Server *server, UA_UInt32 monitoredItemId, void *monitoredItemContext,
     const UA_NodeId *nodeId, void *nodeContext, UA_UInt32 attributeId,
     const UA_DataValue *value);

typedef void (*UA_Server_EventNotificationCallback)
    (UA_Server *server, UA_UInt32 monId, void *monContext,
     size_t nEventFields, const UA_Variant *eventFields);

/* Create a local MonitoredItem with a sampling interval that detects data
 * changes.
 *
 * @param server The server executing the MonitoredItem
 * @timestampsToReturn Shall timestamps be added to the value for the callback?
 * @item The parameters of the new MonitoredItem. Note that the attribute of the
 *       ReadValueId (the node that is monitored) can not be
 *       ``UA_ATTRIBUTEID_EVENTNOTIFIER``. A different callback type needs to be
 *       registered for event notifications.
 * @monitoredItemContext A pointer that is forwarded with the callback
 * @callback The callback that is executed on detected data changes
 *
 * @return Returns a description of the created MonitoredItem. The structure
 * also contains a StatusCode (in case of an error) and the identifier of the
 * new MonitoredItem. */
UA_MonitoredItemCreateResult UA_EXPORT UA_THREADSAFE
UA_Server_createDataChangeMonitoredItem(UA_Server *server,
          UA_TimestampsToReturn timestampsToReturn,
          const UA_MonitoredItemCreateRequest item,
          void *monitoredItemContext,
          UA_Server_DataChangeNotificationCallback callback);

/* UA_MonitoredItemCreateResult UA_EXPORT */
/* UA_Server_createEventMonitoredItem(UA_Server *server, */
/*           UA_TimestampsToReturn timestampsToReturn, */
/*           const UA_MonitoredItemCreateRequest item, void *context, */
/*           UA_Server_EventNotificationCallback callback); */

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_deleteMonitoredItem(UA_Server *server, UA_UInt32 monitoredItemId);

#endif

/**
 * Method Callbacks
 * ^^^^^^^^^^^^^^^^
 * Method callbacks are set to `NULL` (not executable) when a method node is
 * added over the network. In theory, it is possible to add a callback via
 * ``UA_Server_setMethodNode_callback`` within the global constructor when
 * adding methods over the network is really wanted. See the Section
 * :ref:`object-interaction` for calling methods on an object. */

typedef UA_StatusCode
(*UA_MethodCallback)(UA_Server *server, const UA_NodeId *sessionId,
                     void *sessionContext, const UA_NodeId *methodId,
                     void *methodContext, const UA_NodeId *objectId,
                     void *objectContext, size_t inputSize,
                     const UA_Variant *input, size_t outputSize,
                     UA_Variant *output);

#ifdef UA_ENABLE_METHODCALLS
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_setMethodNode_callback(UA_Server *server,
                                 const UA_NodeId methodNodeId,
                                 UA_MethodCallback methodCallback);
#endif

/**
 * .. _object-interaction:
 *
 * Interacting with Objects
 * ------------------------
 * Objects in the information model are represented as ObjectNodes. Some
 * convenience functions are provided to simplify the interaction with objects.
 */

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

#ifdef UA_ENABLE_METHODCALLS
UA_CallMethodResult UA_EXPORT UA_THREADSAFE
UA_Server_call(UA_Server *server, const UA_CallMethodRequest *request);
#endif

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
 * The methods for node addition and deletion take mostly const arguments that
 * are not modified. When creating a node, a deep copy of the node identifier,
 * node attributes, etc. is created. Therefore, it is possible to call for
 * example ``UA_Server_addVariablenode`` with a value attribute (a
 * :ref:`variant`) pointing to a memory location on the stack. If you need
 * changes to a variable value to manifest at a specific memory location, please
 * use a :ref:`datasource` or a :ref:`value-callback`. */

/* Protect against redundant definitions for server/client */
#ifndef UA_DEFAULT_ATTRIBUTES_DEFINED
#define UA_DEFAULT_ATTRIBUTES_DEFINED
/* The default for variables is "BaseDataType" for the datatype, -2 for the
 * valuerank and a read-accesslevel. */
UA_EXPORT extern const UA_VariableAttributes UA_VariableAttributes_default;
UA_EXPORT extern const UA_VariableTypeAttributes UA_VariableTypeAttributes_default;
/* Methods are executable by default */
UA_EXPORT extern const UA_MethodAttributes UA_MethodAttributes_default;
/* The remaining attribute definitions are currently all zeroed out */
UA_EXPORT extern const UA_ObjectAttributes UA_ObjectAttributes_default;
UA_EXPORT extern const UA_ObjectTypeAttributes UA_ObjectTypeAttributes_default;
UA_EXPORT extern const UA_ReferenceTypeAttributes UA_ReferenceTypeAttributes_default;
UA_EXPORT extern const UA_DataTypeAttributes UA_DataTypeAttributes_default;
UA_EXPORT extern const UA_ViewAttributes UA_ViewAttributes_default;
#endif

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

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_addVariableNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                          const UA_NodeId parentNodeId,
                          const UA_NodeId referenceTypeId,
                          const UA_QualifiedName browseName,
                          const UA_NodeId typeDefinition,
                          const UA_VariableAttributes attr,
                          void *nodeContext, UA_NodeId *outNewNodeId) {
    return __UA_Server_addNode(server, UA_NODECLASS_VARIABLE, &requestedNewNodeId,
                               &parentNodeId, &referenceTypeId, browseName,
                               &typeDefinition, (const UA_NodeAttributes*)&attr,
                               &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
                               nodeContext, outNewNodeId);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_addVariableTypeNode(UA_Server *server,
                              const UA_NodeId requestedNewNodeId,
                              const UA_NodeId parentNodeId,
                              const UA_NodeId referenceTypeId,
                              const UA_QualifiedName browseName,
                              const UA_NodeId typeDefinition,
                              const UA_VariableTypeAttributes attr,
                              void *nodeContext, UA_NodeId *outNewNodeId) {
    return __UA_Server_addNode(server, UA_NODECLASS_VARIABLETYPE,
                               &requestedNewNodeId, &parentNodeId, &referenceTypeId,
                               browseName, &typeDefinition,
                               (const UA_NodeAttributes*)&attr,
                               &UA_TYPES[UA_TYPES_VARIABLETYPEATTRIBUTES],
                               nodeContext, outNewNodeId);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_addObjectNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                        const UA_NodeId parentNodeId,
                        const UA_NodeId referenceTypeId,
                        const UA_QualifiedName browseName,
                        const UA_NodeId typeDefinition,
                        const UA_ObjectAttributes attr,
                        void *nodeContext, UA_NodeId *outNewNodeId) {
    return __UA_Server_addNode(server, UA_NODECLASS_OBJECT, &requestedNewNodeId,
                               &parentNodeId, &referenceTypeId, browseName,
                               &typeDefinition, (const UA_NodeAttributes*)&attr,
                               &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                               nodeContext, outNewNodeId);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_addObjectTypeNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                            const UA_NodeId parentNodeId,
                            const UA_NodeId referenceTypeId,
                            const UA_QualifiedName browseName,
                            const UA_ObjectTypeAttributes attr,
                            void *nodeContext, UA_NodeId *outNewNodeId) {
    return __UA_Server_addNode(server, UA_NODECLASS_OBJECTTYPE, &requestedNewNodeId,
                               &parentNodeId, &referenceTypeId, browseName,
                               &UA_NODEID_NULL, (const UA_NodeAttributes*)&attr,
                               &UA_TYPES[UA_TYPES_OBJECTTYPEATTRIBUTES],
                               nodeContext, outNewNodeId);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_addViewNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                      const UA_NodeId parentNodeId,
                      const UA_NodeId referenceTypeId,
                      const UA_QualifiedName browseName,
                      const UA_ViewAttributes attr,
                      void *nodeContext, UA_NodeId *outNewNodeId) {
    return __UA_Server_addNode(server, UA_NODECLASS_VIEW, &requestedNewNodeId,
                               &parentNodeId, &referenceTypeId, browseName,
                               &UA_NODEID_NULL, (const UA_NodeAttributes*)&attr,
                               &UA_TYPES[UA_TYPES_VIEWATTRIBUTES],
                               nodeContext, outNewNodeId);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_addReferenceTypeNode(UA_Server *server,
                               const UA_NodeId requestedNewNodeId,
                               const UA_NodeId parentNodeId,
                               const UA_NodeId referenceTypeId,
                               const UA_QualifiedName browseName,
                               const UA_ReferenceTypeAttributes attr,
                               void *nodeContext, UA_NodeId *outNewNodeId) {
    return __UA_Server_addNode(server, UA_NODECLASS_REFERENCETYPE,
                               &requestedNewNodeId, &parentNodeId, &referenceTypeId,
                               browseName, &UA_NODEID_NULL,
                               (const UA_NodeAttributes*)&attr,
                               &UA_TYPES[UA_TYPES_REFERENCETYPEATTRIBUTES],
                               nodeContext, outNewNodeId);
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_addDataTypeNode(UA_Server *server,
                          const UA_NodeId requestedNewNodeId,
                          const UA_NodeId parentNodeId,
                          const UA_NodeId referenceTypeId,
                          const UA_QualifiedName browseName,
                          const UA_DataTypeAttributes attr,
                          void *nodeContext, UA_NodeId *outNewNodeId) {
    return __UA_Server_addNode(server, UA_NODECLASS_DATATYPE, &requestedNewNodeId,
                               &parentNodeId, &referenceTypeId, browseName,
                               &UA_NODEID_NULL, (const UA_NodeAttributes*)&attr,
                               &UA_TYPES[UA_TYPES_DATATYPEATTRIBUTES],
                               nodeContext, outNewNodeId);
}

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

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Server_addMethodNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                        const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                        const UA_QualifiedName browseName, const UA_MethodAttributes attr,
                        UA_MethodCallback method,
                        size_t inputArgumentsSize, const UA_Argument *inputArguments,
                        size_t outputArgumentsSize, const UA_Argument *outputArguments,
                        void *nodeContext, UA_NodeId *outNewNodeId) {
    return UA_Server_addMethodNodeEx(server, requestedNewNodeId,  parentNodeId,
                                     referenceTypeId, browseName, attr, method,
                                     inputArgumentsSize, inputArguments, UA_NODEID_NULL, NULL,
                                     outputArgumentsSize, outputArguments, UA_NODEID_NULL, NULL,
                                     nodeContext, outNewNodeId);
}

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
 * The special UA_Server_addMethodNode_finish method needs to be used for
 * method nodes, since there you need to explicitly specifiy the input
 * and output arguments which are added in the finish step (if not yet already there)
 **/

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
                         size_t inputArgumentsSize, const UA_Argument* inputArguments,
                         size_t outputArgumentsSize, const UA_Argument* outputArguments);

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
 * The method ``UA_Server_createEvent`` creates an event and represents it as node. The node receives a unique `EventId`
 * which is automatically added to the node.
 * The method returns a `NodeId` to the object node which represents the event through ``outNodeId``. The `NodeId` can
 * be used to set the attributes of the event. The generated `NodeId` is always numeric. ``outNodeId`` cannot be
 * ``NULL``.
 *
 * Note: In order to see an event in UAExpert, the field `Time` must be given a value!
 *
 * The method ``UA_Server_triggerEvent`` "triggers" an event by adding it to all monitored items of the specified
 * origin node and those of all its parents. Any filters specified by the monitored items are automatically applied.
 * Using this method deletes the node generated by ``UA_Server_createEvent``. The `EventId` for the new event is
 * generated automatically and is returned through ``outEventId``. ``NULL`` can be passed if the `EventId` is not
 * needed. ``deleteEventNode`` specifies whether the node representation of the event should be deleted after invoking
 * the method. This can be useful if events with the similar attributes are triggered frequently. ``UA_TRUE`` would
 * cause the node to be deleted. */
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS

/* The EventQueueOverflowEventType is defined as abstract, therefore we can not
 * create an instance of that type directly, but need to create a subtype. The
 * following is an arbitrary number which shall refer to our internal overflow
 * type. This is already posted on the OPC Foundation bug tracker under the
 * following link for clarification:
 * https://opcfoundation-onlineapplications.org/mantis/view.php?id=4206 */
# define UA_NS0ID_SIMPLEOVERFLOWEVENTTYPE 4035

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
   adding the event to the appropriate queues.
 * @param server The server object
 * @param eventNodeId The NodeId of the node representation of the event which should be triggered
 * @param outEvent the EventId of the new event
 * @param deleteEventNode Specifies whether the node representation of the event should be deleted
 * @return The StatusCode of the UA_Server_triggerEvent method */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_triggerEvent(UA_Server *server, const UA_NodeId eventNodeId, const UA_NodeId originId,
                       UA_ByteString *outEventId, const UA_Boolean deleteEventNode);

#endif /* UA_ENABLE_SUBSCRIPTIONS_EVENTS */

#ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS
typedef enum UA_TwoStateVariableCallbackType {
  UA_ENTERING_ENABLEDSTATE,
  UA_ENTERING_ACKEDSTATE,
  UA_ENTERING_CONFIRMEDSTATE,
  UA_ENTERING_ACTIVESTATE
} UA_TwoStateVariableCallbackType;

/**
 * callback prototype to set user specific callbacks
 */
typedef UA_StatusCode
(*UA_TwoStateVariableChangeCallback)(UA_Server *server, const UA_NodeId *condition);

/**
 * create condition instance. The function checks first whether the passed conditionType
 * is a subType of ConditionType. Then checks whether the condition source has HasEventSource
 * reference to its parent. If not, a HasEventSource reference will be created between condition
 * source and server object. To expose the condition in address space, a hierarchical ReferenceType
 * should be passed to create the reference to condition source. Otherwise, UA_NODEID_NULL should be
 * passed to make the condition not exposed.
 * @param server The server object
 * @param conditionId The NodeId of the requested Condition Object. When passing UA_NODEID_NUMERIC(X,0)
 *                      an unused nodeid in namespace X will be used. E.g. passing UA_NODEID_NULL will
 *                      result in a NodeId in namespace 0.
 * @param conditionType The NodeId of the node representation of the ConditionType
 * @param conditionName The name of the condition to be created
 * @param conditionSource The NodeId of the Condition Source (Parent of the Condition)
 * @param hierarchialReferenceType The NodeId of Hierarchical ReferenceType between Condition and its source
 * @param outConditionId The NodeId of the created Condition
 * @return The StatusCode of the UA_Server_createCondition method */
UA_StatusCode UA_EXPORT
UA_Server_createCondition(UA_Server *server,
                          const UA_NodeId conditionId, const UA_NodeId conditionType,
                          UA_QualifiedName conditionName, const UA_NodeId conditionSource,
                          const UA_NodeId hierarchialReferenceType, UA_NodeId *outConditionId);

/**
 * set the value of condition field.
 * @param server The server object
 * @param condition The NodeId of the node representation of the Condition Instance
 * @param value Variant Value to be written to the Field
 * @param fieldName Name of the Field in which the value should be written
 * @return The StatusCode of the UA_Server_setConditionField method*/
UA_StatusCode UA_EXPORT
UA_Server_setConditionField(UA_Server *server,
                            const UA_NodeId condition,
                            const UA_Variant* value,
                            const UA_QualifiedName fieldName);

/**
 * set the value of property of condition field.
 * @param server The server object
 * @param condition The NodeId of the node representation of the Condition Instance
 * @param value Variant Value to be written to the Field
 * @param variableFieldName Name of the Field which has a property
 * @param variablePropertyName Name of the Field Property in which the value should be written
 * @return The StatusCode of the UA_Server_setConditionVariableFieldProperty*/
UA_StatusCode UA_EXPORT
UA_Server_setConditionVariableFieldProperty(UA_Server *server,
                                            const UA_NodeId condition,
                                            const UA_Variant* value,
                                            const UA_QualifiedName variableFieldName,
                                            const UA_QualifiedName variablePropertyName);

/**
 * triggers an event only for an enabled condition. The condition list is updated then with the
 * last generated EventId.
 * @param server The server object
 * @param condition The NodeId of the node representation of the Condition Instance
 * @param conditionSource The NodeId of the node representation of the Condition Source
 * @param outEventId last generated EventId
 * @return The StatusCode of the UA_Server_triggerConditionEvent method*/
UA_StatusCode UA_EXPORT
UA_Server_triggerConditionEvent(UA_Server *server, const UA_NodeId condition,
                                const UA_NodeId conditionSource, UA_ByteString *outEventId);

/**
 * add an optional condition field using its name. (TODO Adding optional methods
 * is not implemented yet)
 * @param server The server object
 * @param condition The NodeId of the node representation of the Condition Instance
 * @param conditionType The NodeId of the node representation of the Condition Type
 * from which the optional field comes
 * @param fieldName Name of the optional field
 * @param outOptionalVariable The NodeId of the created field (Variable Node)
 * @return The StatusCode of the UA_Server_addConditionOptionalField method*/
UA_StatusCode UA_EXPORT
UA_Server_addConditionOptionalField(UA_Server *server, const UA_NodeId condition,
                                    const UA_NodeId conditionType, const UA_QualifiedName fieldName,
                                    UA_NodeId *outOptionalVariable);

/**
 * Function used to set a user specific callback to TwoStateVariable Fields of
 * a condition. The callbacks will be called before triggering the events when
 * transition to true State of EnabledState/Id, AckedState/Id, ConfirmedState/Id
 * and ActiveState/Id occurs.
 * @param server The server object
 * @param condition The NodeId of the node representation of the Condition Instance
 * @param conditionSource The NodeId of the node representation of the Condition Source
 * @param removeBranch (Not Implemented yet)
 * @param callback User specific callback function
 * @param callbackType Callback function type, indicates where it should be called
 * @return The StatusCode of the UA_Server_setConditionTwoStateVariableCallback method*/
UA_StatusCode UA_EXPORT
UA_Server_setConditionTwoStateVariableCallback(UA_Server *server, const UA_NodeId condition,
                                               const UA_NodeId conditionSource, UA_Boolean removeBranch,
                                               UA_TwoStateVariableChangeCallback callback,
                                               UA_TwoStateVariableCallbackType callbackType);

/**
 * Delete a condition from the address space and the internal lists.
 * 
 * @param server The server object
 * @param condition The NodeId of the node representation of the Condition Instance
 * @param conditionSource The NodeId of the node representation of the Condition Source
 * @return UA_STATUSCODE_GOOD on success
 */
UA_StatusCode UA_EXPORT
UA_Server_deleteCondition(UA_Server *server, const UA_NodeId condition, const UA_NodeId conditionSource);

#endif//UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS

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
/* Add a new namespace to the server. Returns the index of the new namespace */
UA_UInt16 UA_EXPORT UA_THREADSAFE UA_Server_addNamespace(UA_Server *server, const char* name);

/* Get namespace by name from the server. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_getNamespaceByName(UA_Server *server, const UA_String namespaceUri,
                             size_t* foundIndex);

#ifdef UA_ENABLE_HISTORIZING
UA_Boolean UA_EXPORT UA_THREADSAFE
UA_Server_AccessControl_allowHistoryUpdateUpdateData(UA_Server *server,
                                                     const UA_NodeId *sessionId, void *sessionContext,
                                                     const UA_NodeId *nodeId,
                                                     UA_PerformUpdateType performInsertReplace,
                                                     const UA_DataValue *value);

UA_Boolean UA_EXPORT UA_THREADSAFE
UA_Server_AccessControl_allowHistoryUpdateDeleteRawModified(UA_Server *server,
                                                            const UA_NodeId *sessionId, void *sessionContext,
                                                            const UA_NodeId *nodeId,
                                                            UA_DateTime startTimestamp,
                                                            UA_DateTime endTimestamp,
                                                            bool isDeleteModified);
#endif // UA_ENABLE_HISTORIZING

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
UA_Server_getAsyncOperationNonBlocking(UA_Server *server, UA_AsyncOperationType *type,
                                       const UA_AsyncOperationRequest **request,
                                       void **context, UA_DateTime *timeout);

/* UA_Boolean UA_EXPORT */
/* UA_Server_getAsyncOperationBlocking(UA_Server *server, UA_AsyncOperationType *type, */
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

/* Get the next async operation. Attention! This method is deprecated and has
 * been replaced by UA_Server_getAsyncOperationNonBlocking! */
UA_DEPRECATED UA_Boolean UA_EXPORT
UA_Server_getAsyncOperation(UA_Server *server, UA_AsyncOperationType *type,
                            const UA_AsyncOperationRequest **request,
                            void **context);

#endif /* !UA_MULTITHREADING >= 100 */

/**
* Statistics
* ----------
*
* Statistic counters keeping track of the current state of the stack. Counters
* are structured per OPC UA communication layer. */

typedef struct {
   UA_NetworkStatistics ns;
   UA_SecureChannelStatistics scs;
   UA_SessionStatistics ss;
} UA_ServerStatistics;

UA_ServerStatistics UA_Server_getStatistics(UA_Server *server);

_UA_END_DECLS

#endif /* UA_SERVER_H_ */
