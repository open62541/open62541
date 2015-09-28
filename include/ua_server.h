 /*
 * Copyright (C) 2014 the contributors as stated in the AUTHORS file
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
 * details.
 */

#ifndef UA_SERVER_H_
#define UA_SERVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_config.h"
#include "ua_types.h"
#include "ua_types_generated.h"
#include "ua_nodeids.h"
#include "ua_log.h"
#include "ua_job.h"
#include "ua_connection.h"

/**
 * @defgroup server Server
 *
 * @{
 */

typedef struct UA_ServerConfig {
    UA_Boolean  Login_enableAnonymous;

    UA_Boolean  Login_enableUsernamePassword;
    char**      Login_usernames;
    char**      Login_passwords;
    UA_UInt32   Login_loginsCount;

    char*       Application_applicationURI;
    char*       Application_applicationName;
} UA_ServerConfig;

extern UA_EXPORT const UA_ServerConfig UA_ServerConfig_standard;

UA_Server UA_EXPORT * UA_Server_new(UA_ServerConfig config);
void UA_EXPORT UA_Server_setServerCertificate(UA_Server *server, UA_ByteString certificate);
void UA_EXPORT UA_Server_delete(UA_Server *server);

/** Sets the logger used by the server */
void UA_EXPORT UA_Server_setLogger(UA_Server *server, UA_Logger logger);
UA_Logger UA_EXPORT UA_Server_getLogger(UA_Server *server);

/**
 * Runs the main loop of the server. In each iteration, this calls into the networklayers to see if
 * jobs have arrived and checks if repeated jobs need to be triggered.
 *
 * @param server The server object
 *
 * @param nThreads The number of worker threads. Is ignored if MULTITHREADING is not activated.
 *
 * @param running Points to a boolean value on the heap. When running is set to false, the worker
 * threads and the main loop close and the server is shut down.
 *
 * @return Indicates whether the server shut down cleanly
 *
 */
UA_StatusCode UA_EXPORT UA_Server_run(UA_Server *server, UA_UInt16 nThreads, UA_Boolean *running);

/* The prologue part of UA_Server_run (no need to use if you call UA_Server_run) */
UA_StatusCode UA_EXPORT UA_Server_run_startup(UA_Server *server, UA_UInt16 nThreads, UA_Boolean *running);
/* The epilogue part of UA_Server_run (no need to use if you call UA_Server_run) */
UA_StatusCode UA_EXPORT UA_Server_run_shutdown(UA_Server *server, UA_UInt16 nThreads);
/* One iteration of UA_Server_run (no need to use if you call UA_Server_run) */
UA_StatusCode UA_EXPORT UA_Server_run_mainloop(UA_Server *server, UA_Boolean *running);

/**
 * Datasources are the interface to local data providers. It is expected that
 * the read and release callbacks are implemented. The write callback can be set
 * to null.
 */
typedef struct {
    void *handle; ///> A custom pointer to reuse the same datasource functions for multiple sources

    /**
     * Copies the data from the source into the provided value.
     *
     * @param handle An optional pointer to user-defined data for the specific data source
     * @param nodeid Id of the read node
     * @param includeSourceTimeStamp If true, then the datasource is expected to set the source
     *        timestamp in the returned value
     * @param range If not null, then the datasource shall return only a selection of the (nonscalar)
     *        data. Set UA_STATUSCODE_BADINDEXRANGEINVALID in the value if this does not apply.
     * @param value The (non-null) DataValue that is returned to the client. The data source sets the
     *        read data, the result status and optionally a sourcetimestamp.
     * @return Returns a status code for logging. Error codes intended for the original caller are set
     *         in the value. If an error is returned, then no releasing of the value is done.
     */
    UA_StatusCode (*read)(void *handle, const UA_NodeId nodeid, UA_Boolean includeSourceTimeStamp,
                          const UA_NumericRange *range, UA_DataValue *value);

    /**
     * Write into a data source. The write member of UA_DataSource can be empty if the operation
     * is unsupported.
     *
     * @param handle An optional pointer to user-defined data for the specific data source
     * @param nodeid Id of the node being written to
     * @param data The data to be written into the data source
     * @param range An optional data range. If the data source is scalar or does not support writing
     *        of ranges, then an error code is returned.
     * @return Returns a status code that is returned to the user
     */
    UA_StatusCode (*write)(void *handle, const UA_NodeId nodeid, const UA_Variant *data, const UA_NumericRange *range);
} UA_DataSource;

/* Value Callbacks can be attach to value and value type nodes. If not-null, they are called before
   reading and after writing respectively */
typedef struct {
    void *handle;
    void (*onRead)(void *handle, const UA_NodeId nodeid, const UA_Variant *data, const UA_NumericRange *range);
    void (*onWrite)(void *handle, const UA_NodeId nodeid, const UA_Variant *data, const UA_NumericRange *range);
} UA_ValueCallback;

/** @brief Add a new namespace to the server. Returns the index of the new namespace */
UA_UInt16 UA_EXPORT UA_Server_addNamespace(UA_Server *server, const char* name);

/** Add a reference to the server's address space */
UA_StatusCode UA_EXPORT UA_Server_addReference(UA_Server *server, const UA_NodeId sourceId,
                                               const UA_NodeId refTypeId, const UA_ExpandedNodeId targetId);

/** Deletes a node from the nodestore.
 *
 * @param server The server object
 * @param nodeId ID of the node to be deleted
 * @return Return UA_STATUSCODE_GOOD if the node was deleted or an appropriate errorcode if the node was not found
 *         or cannot be deleted.
 */
UA_StatusCode UA_EXPORT UA_Server_deleteNode(UA_Server *server, UA_NodeId nodeId);

/** A new variable Node with a value passed in variant.
 *
 * @param server The server object
 * @param nodeId        The requested nodeId of the new node. Use the numeric id with i=0 to get a new ID from the server.
 * @param browseName    The qualified name of this node
 * @param displayName   The localized text shown when displaying the node
 * @param description   The localized human readable description
 * @param userWriteMask Bitmask defining the user write permissions
 * @param writeMask     Bitmask defining the write permissions
 * @param parentNodeId  The node under which this node exists ("parent")
 * @param referenceTypeId Reference type used by the parent to reference this node
 * @param value         A variant containing the value to be assigned to this node.
 * @param createdNodeId Pointer to a NULL pointer that will hold the copy of the nodeId on a successfull return.
 * @return Return UA_STATUSCODE_GOOD if the node was created or an appropriate error code if not.
 */
UA_StatusCode UA_EXPORT
UA_Server_addVariableNode(UA_Server *server, const UA_NodeId nodeId, const UA_QualifiedName browseName,
                          const UA_LocalizedText displayName, const UA_LocalizedText description,
						  const UA_UInt32 userWriteMask, const UA_UInt32 writeMask,
                          const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                          UA_Variant *value, UA_NodeId *createdNodeId);

// Missing: eventNotifier
UA_StatusCode UA_EXPORT
UA_Server_addObjectNode(UA_Server *server, const UA_NodeId nodeId, const UA_QualifiedName browseName,
                        const UA_LocalizedText displayName, const UA_LocalizedText description,
						const UA_UInt32 userWriteMask, const UA_UInt32 writeMask,
                        const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                        const UA_ExpandedNodeId typeDefinition, UA_NodeId *createdNodeId);

// Missing: isAbstract, symmetric
UA_StatusCode UA_EXPORT 
UA_Server_addReferenceTypeNode(UA_Server *server, const UA_NodeId nodeId, const UA_QualifiedName browseName,
                               const UA_LocalizedText displayName, const UA_LocalizedText description,
							   const UA_UInt32 userWriteMask, const UA_UInt32 writeMask,
                               const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                               const UA_ExpandedNodeId typeDefinition, const UA_LocalizedText inverseName,
                               UA_NodeId *createdNodeId );

UA_StatusCode UA_EXPORT
UA_Server_addObjectTypeNode(UA_Server *server, const UA_NodeId nodeId, const UA_QualifiedName browseName,
                            const UA_LocalizedText displayName, const UA_LocalizedText description,
							const UA_UInt32 userWriteMask, const UA_UInt32 writeMask,
                            const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                            const UA_ExpandedNodeId typeDefinition, const UA_Boolean isAbstract,
                            UA_NodeId *createdNodeId );

UA_StatusCode UA_EXPORT 
UA_Server_addVariableTypeNode(UA_Server *server, const UA_NodeId nodeId, const UA_QualifiedName browseName,
                              const UA_LocalizedText displayName, const UA_LocalizedText description,
							  const UA_UInt32 userWriteMask, const UA_UInt32 writeMask,
                              const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                              UA_Variant *value, const UA_Int32 valueRank, const UA_Boolean isAbstract,
                              UA_NodeId *createdNodeId);

UA_StatusCode UA_EXPORT
UA_Server_addDataTypeNode(UA_Server *server, const UA_NodeId nodeId, const UA_QualifiedName browseName,
                          const UA_LocalizedText displayName, const UA_LocalizedText description,
						  const UA_UInt32 userWriteMask, const UA_UInt32 writeMask,
                          const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                          const UA_ExpandedNodeId typeDefinition, const UA_Boolean isAbstract,
                          UA_NodeId *createdNodeId);


UA_StatusCode UA_EXPORT
UA_Server_addViewNode(UA_Server *server, const UA_NodeId nodeId, const UA_QualifiedName browseName,
                      const UA_LocalizedText displayName, const UA_LocalizedText description,
					  const UA_UInt32 userWriteMask, const UA_UInt32 writeMask,
                      const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                      const UA_ExpandedNodeId typeDefinition, UA_NodeId *createdNodeId);

UA_StatusCode UA_EXPORT
UA_Server_addDataSourceVariableNode(UA_Server *server, const UA_NodeId nodeId, const UA_QualifiedName browseName,
                                    const UA_LocalizedText displayName, const UA_LocalizedText description,
									const UA_UInt32 userWriteMask, const UA_UInt32 writeMask, const UA_NodeId parentNodeId,
                                    const UA_NodeId referenceTypeId, const UA_DataSource dataSource, UA_NodeId *createdNodeId);

UA_StatusCode UA_EXPORT
UA_Server_addMonodirectionalReference(UA_Server *server, UA_NodeId sourceNodeId,
                                      UA_ExpandedNodeId targetNodeId, UA_NodeId referenceTypeId,
                                      UA_Boolean isforward);

#ifdef ENABLE_METHODCALLS
typedef UA_StatusCode (*UA_MethodCallback)(const UA_NodeId objectId, const UA_Variant *input,
                                           UA_Variant *output, void *handle);
/** Creates a serverside method including input- and output variable descriptions
 * 
 * @param server The server object.
 * 
 * @param browseName BrowseName to be used for the new method.
 * 
 * @param nodeId Requested NodeId for the new method. If a numeric ID with i=0 is used, the server
 * will assign a random unused id.
 * 
 * @param parentNodeId Parent node containing this method. Note that an ObjectNode needs to
 * reference the method with hasProperty in order for the method to be callable.
 * 
 * @param referenceTypeId Reference type ID to be used by the parent to reference the new method.
 * 
 * @param method Userspace Method/Function of type UA_MethodCallback to be called when a client
 * invokes the method using the Call Service Set.
 * 
 * @param inputArgumentsSize Number of input arguments expected to be passed by a calling client.
 * 
 * @param inputArguments Description of input arguments expected to be passed by a calling client.
 * 
 * @param outputArgumentsSize Description of output arguments expected to be passed by a calling client.
 * 
 * @param outputArguments Description of output arguments expected to be passed by a calling client.
 * 
 * @param createdNodeId Actual nodeId of the new method node if UA_StatusCode indicates success. Can
 * be used to determine the random unique ID assigned by the server if i=0 was passed as a nodeId.
 * 
 */
UA_StatusCode UA_EXPORT
UA_Server_addMethodNode(UA_Server *server, const UA_NodeId nodeId, const UA_QualifiedName browseName,
                        UA_LocalizedText displayName, UA_LocalizedText description, const UA_NodeId parentNodeId, 
                        const UA_NodeId referenceTypeId, UA_UInt32 userWriteMask, UA_UInt32 writeMask, 
                        UA_MethodCallback method, void *handle, UA_Int32 inputArgumentsSize, const UA_Argument *inputArguments, 
                        UA_Int32 outputArgumentsSize, const UA_Argument *outputArguments,
                        UA_NodeId *createdNodeId);
#endif

#ifndef _HAVE_UA_NODEITERATORCALLBACK_D
#define _HAVE_UA_NODEITERATORCALLBACK_D
typedef UA_StatusCode (*UA_NodeIteratorCallback)(UA_NodeId childId, UA_Boolean isInverse, UA_NodeId referenceTypeId, void *handle);
#endif

/** Iterate over all nodes referenced by parentNodeId by calling the callback function for each child node
 * 
 * @param server The server object.
 *
 * @param parentNodeId The NodeId of the parent whose references are to be iterated over
 *
 * @param callback The function of type UA_NodeIteratorCallback to be called for each referenced child
 *
 * @return Upon success, UA_STATUSCODE_GOOD is returned. An error code otherwise.
 */
UA_StatusCode UA_EXPORT UA_Server_forEachChildNodeCall(UA_Server *server, UA_NodeId parentNodeId, UA_NodeIteratorCallback callback, void *handle);

/***********************/
/* Set Node Attributes */
/***********************/

/* The following node attributes cannot be changed once the node is created
   - NodeClass
   - NodeId
   - Symmetric
   
   The following attributes will eventually be managed by a userrights layer and are unsupported yet
   - WriteMask
   - UserWriteMask
   - AccessLevel
   - UserAccessLevel
   - UserExecutable

   The following attributes are currently taken from the value variant:
   - DataType
   - ValueRank
   - ArrayDimensions
   
   - Historizing is currently unsupported
  */

UA_StatusCode UA_EXPORT
UA_Server_setNodeAttribute_value(UA_Server *server, const UA_NodeId nodeId,
                                 const UA_DataType *type, const UA_Variant *value);

/* The value is moved into the node (not copied). The value variant is _inited internally. */
UA_StatusCode UA_EXPORT
UA_Server_setNodeAttribute_value_destructive(UA_Server *server, const UA_NodeId nodeId,
                                             const UA_DataType *type, UA_Variant *value);

/* Succeeds only if the node contains a variant value */
UA_StatusCode UA_EXPORT
UA_Server_setAttribute_value_callback(UA_Server *server, UA_NodeId nodeId, UA_ValueCallback callback);

UA_StatusCode UA_EXPORT
UA_Server_setNodeAttribute_value_dataSource(UA_Server *server, UA_NodeId nodeId, UA_DataSource dataSource);

/* Don't use this function. There are typed versions with no additional overhead. */
UA_StatusCode UA_EXPORT
UA_Server_setNodeAttribute(UA_Server *server, const UA_NodeId nodeId, const UA_AttributeId attributeId,
                           const UA_DataType *type, const void *value);

static UA_INLINE UA_StatusCode
UA_Server_setNodeAttribute_browseName(UA_Server *server, const UA_NodeId nodeId, const UA_QualifiedName *browseName) {
    return UA_Server_setNodeAttribute(server, nodeId, UA_ATTRIBUTEID_BROWSENAME, &UA_TYPES[UA_TYPES_QUALIFIEDNAME], browseName); }

static UA_INLINE UA_StatusCode
UA_Server_setNodeAttribute_displayName(UA_Server *server, const UA_NodeId nodeId, const UA_LocalizedText *displayName) {
    return UA_Server_setNodeAttribute(server, nodeId, UA_ATTRIBUTEID_DISPLAYNAME, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT], displayName); }

static UA_INLINE UA_StatusCode
UA_Server_setNodeAttribute_description(UA_Server *server, const UA_NodeId nodeId, const UA_LocalizedText *description) {
    return UA_Server_setNodeAttribute(server, nodeId, UA_ATTRIBUTEID_DESCRIPTION, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT], description); }

static UA_INLINE UA_StatusCode
UA_Server_setNodeAttribute_isAbstract(UA_Server *server, const UA_NodeId nodeId, const UA_Boolean *isAbstract) {
    return UA_Server_setNodeAttribute(server, nodeId, UA_ATTRIBUTEID_ISABSTRACT, &UA_TYPES[UA_TYPES_BOOLEAN], isAbstract); }

static UA_INLINE UA_StatusCode
UA_Server_setNodeAttribute_inverseName(UA_Server *server, const UA_NodeId nodeId, const UA_LocalizedText *inverseName) {
    return UA_Server_setNodeAttribute(server, nodeId, UA_ATTRIBUTEID_INVERSENAME, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT], inverseName); }

static UA_INLINE UA_StatusCode
UA_Server_setNodeAttribute_containtsNoLoops(UA_Server *server, const UA_NodeId nodeId, const UA_Boolean *containsNoLoops) {
    return UA_Server_setNodeAttribute(server, nodeId, UA_ATTRIBUTEID_CONTAINSNOLOOPS, &UA_TYPES[UA_TYPES_BOOLEAN], containsNoLoops); }

static UA_INLINE UA_StatusCode
UA_Server_setNodeAttribute_eventNotifier(UA_Server *server, const UA_NodeId nodeId, const UA_Byte *eventNotifier) {
    return UA_Server_setNodeAttribute(server, nodeId, UA_ATTRIBUTEID_EVENTNOTIFIER, &UA_TYPES[UA_TYPES_BYTE], eventNotifier); }

static UA_INLINE UA_StatusCode
UA_Server_setNodeAttribute_minimumSamplingInterval(UA_Server *server, const UA_NodeId nodeId, const UA_Double *miniumSamplingInterval) {
    return UA_Server_setNodeAttribute(server, nodeId, UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL, &UA_TYPES[UA_TYPES_DOUBLE], miniumSamplingInterval); }

static UA_INLINE UA_StatusCode
UA_Server_setNodeAttribute_executable(UA_Server *server, const UA_NodeId nodeId, const UA_Boolean *executable) {
    return UA_Server_setNodeAttribute(server, nodeId, UA_ATTRIBUTEID_EXECUTABLE, &UA_TYPES[UA_TYPES_BOOLEAN], executable); }

#ifdef ENABLE_METHODCALLS
UA_StatusCode UA_EXPORT
UA_Server_setNodeAttribute_method(UA_Server *server, UA_NodeId methodNodeId, UA_MethodCallback method, void *handle);
#endif

/***********************/
/* Get Node Attributes */
/***********************/

/* The following attributes cannot be read. They make no sense to read internally since the "admin"
   user always has all rights.
   - UserWriteMask
   - UserAccessLevel
   - UserExecutable
*/

/* Don't use this function. There are typed versions for every supported attribute. */
UA_StatusCode UA_EXPORT
UA_Server_getNodeAttribute(UA_Server *server, UA_NodeId nodeId, UA_AttributeId attributeId, void *v);
  
static UA_INLINE UA_StatusCode
UA_Server_getNodeAttribute_nodeId(UA_Server *server, UA_NodeId nodeId, UA_NodeId *outNodeId) {
    return UA_Server_getNodeAttribute(server, nodeId, UA_ATTRIBUTEID_NODEID, outNodeId); }

static UA_INLINE UA_StatusCode
UA_Server_getNodeAttribute_nodeClass(UA_Server *server, UA_NodeId nodeId, UA_NodeClass *outNodeClass) {
    return UA_Server_getNodeAttribute(server, nodeId, UA_ATTRIBUTEID_NODECLASS, outNodeClass); }

static UA_INLINE UA_StatusCode
UA_Server_getNodeAttribute_browseName(UA_Server *server, UA_NodeId nodeId, UA_QualifiedName *outBrowseName) {
    return UA_Server_getNodeAttribute(server, nodeId, UA_ATTRIBUTEID_BROWSENAME, outBrowseName); }

static UA_INLINE UA_StatusCode
UA_Server_getNodeAttribute_displayName(UA_Server *server, UA_NodeId nodeId, UA_LocalizedText *outDisplayName) {
    return UA_Server_getNodeAttribute(server, nodeId, UA_ATTRIBUTEID_DISPLAYNAME, outDisplayName); }

static UA_INLINE UA_StatusCode
UA_Server_getNodeAttribute_description(UA_Server *server, UA_NodeId nodeId, UA_LocalizedText *outDescription) {
    return UA_Server_getNodeAttribute(server, nodeId, UA_ATTRIBUTEID_DESCRIPTION, outDescription); }

static UA_INLINE UA_StatusCode
UA_Server_getNodeAttribute_writeMask(UA_Server *server, UA_NodeId nodeId, UA_UInt32 *outWriteMask) {
    return UA_Server_getNodeAttribute(server, nodeId, UA_ATTRIBUTEID_WRITEMASK, outWriteMask); }

static UA_INLINE UA_StatusCode
UA_Server_getNodeAttribute_isAbstract(UA_Server *server, UA_NodeId nodeId, UA_Boolean *outIsAbstract) {
    return UA_Server_getNodeAttribute(server, nodeId, UA_ATTRIBUTEID_ISABSTRACT, outIsAbstract); }

static UA_INLINE UA_StatusCode
UA_Server_getNodeAttribute_symmetric(UA_Server *server, UA_NodeId nodeId, UA_Boolean *outSymmetric) {
    return UA_Server_getNodeAttribute(server, nodeId, UA_ATTRIBUTEID_SYMMETRIC, outSymmetric); }

static UA_INLINE UA_StatusCode
UA_Server_getNodeAttribute_inverseName(UA_Server *server, UA_NodeId nodeId, UA_LocalizedText *outInverseName) {
    return UA_Server_getNodeAttribute(server, nodeId, UA_ATTRIBUTEID_INVERSENAME, outInverseName); }

static UA_INLINE UA_StatusCode
UA_Server_getNodeAttribute_containsNoLoops(UA_Server *server, UA_NodeId nodeId, UA_Boolean *outContainsNoLoops) {
    return UA_Server_getNodeAttribute(server, nodeId, UA_ATTRIBUTEID_CONTAINSNOLOOPS, outContainsNoLoops); }

static UA_INLINE UA_StatusCode
UA_Server_getNodeAttribute_eventNotifier(UA_Server *server, UA_NodeId nodeId, UA_Byte *outEventNotifier) {
    return UA_Server_getNodeAttribute(server, nodeId, UA_ATTRIBUTEID_EVENTNOTIFIER, outEventNotifier); }

static UA_INLINE UA_StatusCode
UA_Server_getNodeAttribute_value(UA_Server *server, UA_NodeId nodeId, UA_Variant *outValue) {
    return UA_Server_getNodeAttribute(server, nodeId, UA_ATTRIBUTEID_VALUE, outValue); }

UA_StatusCode UA_EXPORT
UA_Server_getNodeAttribute_value_dataSource(UA_Server *server, UA_NodeId nodeId, UA_DataSource *dataSource);

static UA_INLINE UA_StatusCode
UA_Server_getNodeAttribute_dataType(UA_Server *server, UA_NodeId nodeId, UA_Variant *outDataType) {
    return UA_Server_getNodeAttribute(server, nodeId, UA_ATTRIBUTEID_DATATYPE, outDataType); }

static UA_INLINE UA_StatusCode
UA_Server_getNodeAttribute_valueRank(UA_Server *server, UA_NodeId nodeId, UA_Int32 *outValueRank) {
    return UA_Server_getNodeAttribute(server, nodeId, UA_ATTRIBUTEID_VALUERANK, outValueRank); }

static UA_INLINE UA_StatusCode
UA_Server_getNodeAttribute_arrayDimensions(UA_Server *server, UA_NodeId nodeId, UA_Int32 *outArrayDimensions) {
    return UA_Server_getNodeAttribute(server, nodeId, UA_ATTRIBUTEID_ARRAYDIMENSIONS, outArrayDimensions); }

static UA_INLINE UA_StatusCode
UA_Server_getNodeAttribute_accessLevel(UA_Server *server, UA_NodeId nodeId, UA_UInt32 *outAccessLevel) {
    return UA_Server_getNodeAttribute(server, nodeId, UA_ATTRIBUTEID_ACCESSLEVEL, outAccessLevel); }

static UA_INLINE UA_StatusCode
UA_Server_getNodeAttribute_minimumSamplingInterval(UA_Server *server, UA_NodeId nodeId, UA_Double *outMinimumSamplingInterval) {
    return UA_Server_getNodeAttribute(server, nodeId, UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL, outMinimumSamplingInterval); }

static UA_INLINE UA_StatusCode
UA_Server_getNodeAttribute_historizing(UA_Server *server, UA_NodeId nodeId, UA_Double *outHistorizing) {
    return UA_Server_getNodeAttribute(server, nodeId, UA_ATTRIBUTEID_HISTORIZING, outHistorizing); }

static UA_INLINE UA_StatusCode
UA_Server_getNodeAttribute_executable(UA_Server *server, UA_NodeId nodeId, UA_Boolean *outExecutable) {
    return UA_Server_getNodeAttribute(server, nodeId, UA_ATTRIBUTEID_EXECUTABLE, outExecutable); }

#ifdef ENABLE_METHODCALLS
UA_StatusCode UA_EXPORT
UA_Server_getNodeAttribute_method(UA_Server *server, UA_NodeId methodNodeId, UA_MethodCallback *method);
#endif

/**
 * @param server The server object.
 *
 * @param job Pointer to the job that shall be added. The pointer is not freed but copied to an
 *        internal representation.
 *
 * @param interval The job shall be repeatedly executed with the given interval (in ms). The
 *        interval must be larger than 5ms. The first execution occurs at now() + interval at the
 *        latest.
 *
 * @param jobId Set to the guid of the repeated job. This can be used to cancel the job later on. If
 *        the pointer is null, the guid is not set.
 *
 * @return Upon success, UA_STATUSCODE_GOOD is returned. An error code otherwise.
 */
UA_StatusCode UA_EXPORT UA_Server_addRepeatedJob(UA_Server *server, UA_Job job, UA_UInt32 interval,
                                                 UA_Guid *jobId);

/**
 * Remove repeated job. The entry will be removed asynchronously during the next iteration of the
 * server main loop.
 *
 * @param server The server object.
 *
 * @param jobId The id of the job that shall be removed.
 *
 * @return Upon sucess, UA_STATUSCODE_GOOD is returned. An error code otherwise.
 */
UA_StatusCode UA_EXPORT UA_Server_removeRepeatedJob(UA_Server *server, UA_Guid jobId);

/**
 * Interface to the binary network layers. This structure is returned from the
 * function that initializes the network layer. The layer is already bound to a
 * specific port and listening. The functions in the structure are never called
 * in parallel but only sequentially from the server's main loop. So the network
 * layer does not need to be thread-safe.
 */
typedef struct UA_ServerNetworkLayer {
    UA_String discoveryUrl;
    UA_Logger logger; ///< Set during _start

    /**
     * Starts listening on the the networklayer.
     *
     * @param nl The network layer
     * @param logger The logger
     * @return Returns UA_STATUSCODE_GOOD or an error code.
     */
    UA_StatusCode (*start)(struct UA_ServerNetworkLayer *nl, UA_Logger logger);
    
    /**
     * Gets called from the main server loop and returns the jobs (accumulated messages and close
     * events) for dispatch.
     *
     * @param nl The network layer
     * @param jobs When the returned integer is >0, *jobs points to an array of UA_Job of the
     * returned size.
     * @param timeout The timeout during which an event must arrive in microseconds
     * @return The size of the jobs array. If the result is negative, an error has occurred.
     */
    size_t (*getJobs)(struct UA_ServerNetworkLayer *nl, UA_Job **jobs, UA_UInt16 timeout);

    /**
     * Closes the network connection and returns all the jobs that need to be finished before the
     * network layer can be safely deleted.
     *
     * @param nl The network layer
     * @param jobs When the returned integer is >0, jobs points to an array of UA_Job of the
     * returned size.
     * @return The size of the jobs array. If the result is negative, an error has occurred.
     */
    size_t (*stop)(struct UA_ServerNetworkLayer *nl, UA_Job **jobs);

    /** Deletes the network layer. Call only after a successful shutdown. */
    void (*deleteMembers)(struct UA_ServerNetworkLayer *nl);
} UA_ServerNetworkLayer;

/**
 * Adds a network layer to the server. The network layer is destroyed together
 * with the server. Do not use it after adding it as it might be moved around on
 * the heap.
 */
void UA_EXPORT UA_Server_addNetworkLayer(UA_Server *server, UA_ServerNetworkLayer *networkLayer);

/** @} */

#ifndef __cplusplus /* the external nodestore does not work with c++ so far */

/**
 * @ingroup nodestore
 *
 * @defgroup external_nodestore External Nodestore
 *
 * @brief An external application that manages its own data and data model
 *
 * To plug in outside data sources, one can use
 *
 * - VariableNodes with a data source (functions that are called for read and write access)
 * - An external nodestore that is mapped to specific namespaces
 *
 * If no external nodestore is defined for a nodeid, it is always looked up in
 * the "local" nodestore of open62541. Namespace Zero is always in the local
 * nodestore.
 *
 * @{
 */

typedef UA_Int32 (*UA_ExternalNodeStore_addNodes)
(void *ensHandle, const UA_RequestHeader *requestHeader, UA_AddNodesItem *nodesToAdd, UA_UInt32 *indices,
 UA_UInt32 indicesSize, UA_AddNodesResult* addNodesResults, UA_DiagnosticInfo *diagnosticInfos);

typedef UA_Int32 (*UA_ExternalNodeStore_addReferences)
(void *ensHandle, const UA_RequestHeader *requestHeader, UA_AddReferencesItem* referencesToAdd,
 UA_UInt32 *indices,UA_UInt32 indicesSize, UA_StatusCode *addReferencesResults,
 UA_DiagnosticInfo *diagnosticInfos);

typedef UA_Int32 (*UA_ExternalNodeStore_deleteNodes)
(void *ensHandle, const UA_RequestHeader *requestHeader, UA_DeleteNodesItem *nodesToDelete, UA_UInt32 *indices,
 UA_UInt32 indicesSize, UA_StatusCode *deleteNodesResults, UA_DiagnosticInfo *diagnosticInfos);

typedef UA_Int32 (*UA_ExternalNodeStore_deleteReferences)
(void *ensHandle, const UA_RequestHeader *requestHeader, UA_DeleteReferencesItem *referenceToDelete,
 UA_UInt32 *indices, UA_UInt32 indicesSize, UA_StatusCode deleteReferencesresults,
 UA_DiagnosticInfo *diagnosticInfos);

typedef UA_Int32 (*UA_ExternalNodeStore_readNodes)
(void *ensHandle, const UA_RequestHeader *requestHeader, UA_ReadValueId *readValueIds, UA_UInt32 *indices,
 UA_UInt32 indicesSize,UA_DataValue *readNodesResults, UA_Boolean timeStampToReturn,
 UA_DiagnosticInfo *diagnosticInfos);

typedef UA_Int32 (*UA_ExternalNodeStore_writeNodes)
(void *ensHandle, const UA_RequestHeader *requestHeader, UA_WriteValue *writeValues, UA_UInt32 *indices,
 UA_UInt32 indicesSize, UA_StatusCode *writeNodesResults, UA_DiagnosticInfo *diagnosticInfo);

typedef UA_Int32 (*UA_ExternalNodeStore_browseNodes)
(void *ensHandle, const UA_RequestHeader *requestHeader, UA_BrowseDescription *browseDescriptions,
 UA_UInt32 *indices, UA_UInt32 indicesSize, UA_UInt32 requestedMaxReferencesPerNode,
 UA_BrowseResult *browseResults, UA_DiagnosticInfo *diagnosticInfos);

typedef UA_Int32 (*UA_ExternalNodeStore_translateBrowsePathsToNodeIds)
(void *ensHandle, const UA_RequestHeader *requestHeader, UA_BrowsePath *browsePath, UA_UInt32 *indices,
 UA_UInt32 indicesSize, UA_BrowsePathResult *browsePathResults, UA_DiagnosticInfo *diagnosticInfos);

typedef UA_Int32 (*UA_ExternalNodeStore_delete)(void *ensHandle);

typedef struct UA_ExternalNodeStore {
    void *ensHandle;
	UA_ExternalNodeStore_addNodes addNodes;
	UA_ExternalNodeStore_deleteNodes deleteNodes;
	UA_ExternalNodeStore_writeNodes writeNodes;
	UA_ExternalNodeStore_readNodes readNodes;
	UA_ExternalNodeStore_browseNodes browseNodes;
	UA_ExternalNodeStore_translateBrowsePathsToNodeIds translateBrowsePathsToNodeIds;
	UA_ExternalNodeStore_addReferences addReferences;
	UA_ExternalNodeStore_deleteReferences deleteReferences;
	UA_ExternalNodeStore_delete destroy;
} UA_ExternalNodeStore;

#ifdef UA_EXTERNAL_NAMESPACES
UA_StatusCode UA_EXPORT
UA_Server_addExternalNamespace(UA_Server *server, UA_UInt16 namespaceIndex, const UA_String *url, UA_ExternalNodeStore *nodeStore);
#endif /* UA_EXTERNAL_NAMESPACES*/
/** @} */

#endif /* external nodestore */

#ifndef _HAVE_UA_INSTANTIONCALLBACK_D
#define _HAVE_UA_INSTANTIONCALLBACK_D
typedef UA_StatusCode (*UA_InstantiationCallback)(UA_NodeId objectId, UA_NodeId definitionId, void *handle);
#endif

typedef struct arrayOfNodeIds_s {
  UA_Int32  size;
  UA_NodeId *ids;
} arrayOfNodeIds;

UA_StatusCode UA_EXPORT
UA_Server_appendInstanceOfSupertype(UA_Server *server, UA_NodeId nodeId, UA_NodeId appendToNodeId, 
                                    arrayOfNodeIds *subtypeRefs, arrayOfNodeIds *componentRefs, 
                                    UA_InstantiationCallback callback, arrayOfNodeIds *instantiatedTypes, 
                                    void *handle);

void UA_EXPORT
UA_Server_addInstanceOf_instatiateChildNode(UA_Server *server, 
                                                 arrayOfNodeIds *subtypeRefs, arrayOfNodeIds *componentRefs, arrayOfNodeIds *typedefRefs,
                                                 UA_NodeId objectRoot, UA_InstantiationCallback callback, void *typeDefNode,
                                                 UA_Boolean instantiateObjects, arrayOfNodeIds *instantiatedTypes, void *handle);
                                                 
UA_StatusCode UA_EXPORT
UA_Server_addInstanceOf(UA_Server *server, UA_NodeId nodeId, const UA_QualifiedName browseName,
                        UA_LocalizedText displayName, UA_LocalizedText description, const UA_NodeId parentNodeId, 
                        const UA_NodeId referenceTypeId, UA_UInt32 userWriteMask, UA_UInt32 writeMask, 
                        const UA_ExpandedNodeId typeDefinition, UA_InstantiationCallback callback, void *handle, 
                        UA_NodeId *createdNodeId);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_SERVER_H_ */
