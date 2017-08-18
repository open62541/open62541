/* This Source Code Form is subject to the terms of the Mozilla Public
*  License, v. 2.0. If a copy of the MPL was not distributed with this 
*  file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef UA_SERVER_H_
#define UA_SERVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_config.h"
#include "ua_types.h"
#include "ua_types_generated.h"
#include "ua_types_generated_handling.h"
#include "ua_nodeids.h"
#include "ua_log.h"
#include "ua_job.h"
#include "ua_connection.h"

/**
 * .. _server:
 *
 * Server
 * ======
 *
 * Network Layer
 * -------------
 * Interface to the binary network layers. The functions in the network layer
 * are never called in parallel but only sequentially from the server's main
 * loop. So the network layer does not need to be thread-safe. */
struct UA_ServerNetworkLayer;
typedef struct UA_ServerNetworkLayer UA_ServerNetworkLayer;

struct UA_ServerNetworkLayer {
    void *handle; // pointer to internal data
    UA_String discoveryUrl;

    /* Starts listening on the the networklayer.
     *
     * @param nl The network layer
     * @param logger The logger
     * @return Returns UA_STATUSCODE_GOOD or an error code. */
    UA_StatusCode (*start)(UA_ServerNetworkLayer *nl, UA_Logger logger);

    /* Gets called from the main server loop and returns the jobs (accumulated
     * messages and close events) for dispatch.
     *
     * @param nl The network layer
     * @param jobs When the returned integer is >0, *jobs points to an array of
     *        UA_Job of the returned size.
     * @param timeout The timeout during which an event must arrive in
     *        microseconds
     * @return The size of the jobs array. If the result is negative,
     *         an error has occurred. */
    size_t (*getJobs)(UA_ServerNetworkLayer *nl, UA_Job **jobs, UA_UInt16 timeout);

    /* Closes the network connection and returns all the jobs that need to be
     * finished before the network layer can be safely deleted.
     *
     * @param nl The network layer
     * @param jobs When the returned integer is >0, jobs points to an array of
     *        UA_Job of the returned size.
     * @return The size of the jobs array. If the result is negative,
     *         an error has occurred. */
    size_t (*stop)(UA_ServerNetworkLayer *nl, UA_Job **jobs);

    /** Deletes the network content. Call only after stopping. */
    void (*deleteMembers)(UA_ServerNetworkLayer *nl);
};

/**
 * Server Configuration
 * --------------------
 * The following structure is passed to a new server for configuration. */
typedef struct {
    UA_String username;
    UA_String password;
} UA_UsernamePasswordLogin;

typedef struct {
    UA_UInt32 min;
    UA_UInt32 max;
} UA_UInt32Range;

typedef struct {
    UA_Double min;
    UA_Double max;
} UA_DoubleRange;

typedef struct {
    UA_UInt16 nThreads; /* only if multithreading is enabled */
    UA_Logger logger;

    /* Server Description */
    UA_BuildInfo buildInfo;
    UA_ApplicationDescription applicationDescription;
    UA_ByteString serverCertificate;

    /* Networking */
    size_t networkLayersSize;
    UA_ServerNetworkLayer *networkLayers;

    /* Login */
    UA_Boolean enableAnonymousLogin;
    UA_Boolean enableUsernamePasswordLogin;
    size_t usernamePasswordLoginsSize;
    UA_UsernamePasswordLogin* usernamePasswordLogins;

    /* Limits for SecureChannels */
    UA_UInt16 maxSecureChannels;
    UA_UInt32 maxSecurityTokenLifetime; /* in ms */

    /* Limits for Sessions */
    UA_UInt16 maxSessions;
    UA_Double maxSessionTimeout; /* in ms */

    /* Limits for Subscriptions */
    UA_DoubleRange publishingIntervalLimits;
    UA_UInt32Range lifeTimeCountLimits;
    UA_UInt32Range keepAliveCountLimits;
    UA_UInt32 maxNotificationsPerPublish;
    UA_UInt32 maxRetransmissionQueueSize; /* 0 -> unlimited size */

    /* Limits for MonitoredItems */
    UA_DoubleRange samplingIntervalLimits;
    UA_UInt32Range queueSizeLimits; /* Negotiated with the client */
} UA_ServerConfig;

/* Add a new namespace to the server. Returns the index of the new namespace */
UA_UInt16 UA_EXPORT UA_Server_addNamespace(UA_Server *server, const char* name);

/**
 * .. _server-lifecycle:
 *
 * Server Lifecycle
 * ---------------- */
UA_Server UA_EXPORT * UA_Server_new(const UA_ServerConfig config);
void UA_EXPORT UA_Server_delete(UA_Server *server);

/* Runs the main loop of the server. In each iteration, this calls into the
 * networklayers to see if jobs have arrived and checks if repeated jobs need to
 * be triggered.
 *
 * @param server The server object.
 * @param running The loop is run as long as *running is true.
 *        Otherwise, the server shuts down.
 * @return Returns the statuscode of the UA_Server_run_shutdown method */
UA_StatusCode UA_EXPORT
UA_Server_run(UA_Server *server, volatile UA_Boolean *running);

/* The prologue part of UA_Server_run (no need to use if you call
 * UA_Server_run) */
UA_StatusCode UA_EXPORT UA_Server_run_startup(UA_Server *server);

/* Executes a single iteration of the server's main loop.
 *
 * @param server The server object.
 * @param waitInternal Should we wait for messages in the networklayer?
 *        Otherwise, the timouts for the networklayers are set to zero.
 *        The default max wait time is 50millisec.
 * @return Returns how long we can wait until the next scheduled
 *         job (in millisec) */
UA_UInt16 UA_EXPORT
UA_Server_run_iterate(UA_Server *server, UA_Boolean waitInternal);

/* The epilogue part of UA_Server_run (no need to use if you call
 * UA_Server_run) */
UA_StatusCode UA_EXPORT UA_Server_run_shutdown(UA_Server *server);

/**
 * Repeated jobs
 * ------------- */
/* Add a job for cyclic repetition to the server.
 *
 * @param server The server object.
 * @param job The job that shall be added.
 * @param interval The job shall be repeatedly executed with the given interval
 *        (in ms). The interval must be larger than 5ms. The first execution
 *        occurs at now() + interval at the latest.
 * @param jobId Set to the guid of the repeated job. This can be used to cancel
 *        the job later on. If the pointer is null, the guid is not set.
 * @return Upon success, UA_STATUSCODE_GOOD is returned.
 *         An error code otherwise. */
UA_StatusCode UA_EXPORT
UA_Server_addRepeatedJob(UA_Server *server, UA_Job job,
                         UA_UInt32 interval, UA_Guid *jobId);

/* Remove repeated job.
 *
 * @param server The server object.
 * @param jobId The id of the job that shall be removed.
 * @return Upon sucess, UA_STATUSCODE_GOOD is returned.
 *         An error code otherwise. */
UA_StatusCode UA_EXPORT
UA_Server_removeRepeatedJob(UA_Server *server, UA_Guid jobId);

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
UA_DataValue UA_EXPORT
UA_Server_read(UA_Server *server, const UA_ReadValueId *item,
               UA_TimestampsToReturn timestamps);
    
/* Don't use this function. There are typed versions for every supported
 * attribute. */
UA_StatusCode UA_EXPORT
__UA_Server_read(UA_Server *server, const UA_NodeId *nodeId,
                 UA_AttributeId attributeId, void *v);

static UA_INLINE UA_StatusCode
UA_Server_readNodeId(UA_Server *server, const UA_NodeId nodeId,
                     UA_NodeId *outNodeId) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_NODEID, outNodeId);
}

static UA_INLINE UA_StatusCode
UA_Server_readNodeClass(UA_Server *server, const UA_NodeId nodeId,
                        UA_NodeClass *outNodeClass) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_NODECLASS,
                            outNodeClass);
}

static UA_INLINE UA_StatusCode
UA_Server_readBrowseName(UA_Server *server, const UA_NodeId nodeId,
                         UA_QualifiedName *outBrowseName) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_BROWSENAME,
                            outBrowseName);
}

static UA_INLINE UA_StatusCode
UA_Server_readDisplayName(UA_Server *server, const UA_NodeId nodeId,
                          UA_LocalizedText *outDisplayName) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_DISPLAYNAME,
                            outDisplayName);
}

static UA_INLINE UA_StatusCode
UA_Server_readDescription(UA_Server *server, const UA_NodeId nodeId,
                          UA_LocalizedText *outDescription) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_DESCRIPTION,
                            outDescription);
}

static UA_INLINE UA_StatusCode
UA_Server_readWriteMask(UA_Server *server, const UA_NodeId nodeId,
                        UA_UInt32 *outWriteMask) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_WRITEMASK,
                            outWriteMask);
}

static UA_INLINE UA_StatusCode
UA_Server_readIsAbstract(UA_Server *server, const UA_NodeId nodeId,
                         UA_Boolean *outIsAbstract) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_ISABSTRACT,
                            outIsAbstract);
}

static UA_INLINE UA_StatusCode
UA_Server_readSymmetric(UA_Server *server, const UA_NodeId nodeId,
                        UA_Boolean *outSymmetric) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_SYMMETRIC,
                            outSymmetric);
}

static UA_INLINE UA_StatusCode
UA_Server_readInverseName(UA_Server *server, const UA_NodeId nodeId,
                          UA_LocalizedText *outInverseName) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_INVERSENAME,
                            outInverseName);
}

static UA_INLINE UA_StatusCode
UA_Server_readContainsNoLoop(UA_Server *server, const UA_NodeId nodeId,
                             UA_Boolean *outContainsNoLoops) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_CONTAINSNOLOOPS,
                            outContainsNoLoops);
}

static UA_INLINE UA_StatusCode
UA_Server_readEventNotifier(UA_Server *server, const UA_NodeId nodeId,
                            UA_Byte *outEventNotifier) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_EVENTNOTIFIER,
                            outEventNotifier);
}

static UA_INLINE UA_StatusCode
UA_Server_readValue(UA_Server *server, const UA_NodeId nodeId,
                    UA_Variant *outValue) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_VALUE, outValue);
}

static UA_INLINE UA_StatusCode
UA_Server_readDataType(UA_Server *server, const UA_NodeId nodeId,
                       UA_NodeId *outDataType) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_DATATYPE,
                            outDataType);
}

static UA_INLINE UA_StatusCode
UA_Server_readValueRank(UA_Server *server, const UA_NodeId nodeId,
                        UA_Int32 *outValueRank) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_VALUERANK,
                            outValueRank);
}

/* Returns a variant with an int32 array */
static UA_INLINE UA_StatusCode
UA_Server_readArrayDimensions(UA_Server *server, const UA_NodeId nodeId,
                              UA_Variant *outArrayDimensions) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_ARRAYDIMENSIONS,
                            outArrayDimensions);
}

static UA_INLINE UA_StatusCode
UA_Server_readAccessLevel(UA_Server *server, const UA_NodeId nodeId,
                          UA_Byte *outAccessLevel) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_ACCESSLEVEL,
                            outAccessLevel);
}

static UA_INLINE UA_StatusCode
UA_Server_readMinimumSamplingInterval(UA_Server *server, const UA_NodeId nodeId,
                                      UA_Double *outMinimumSamplingInterval) {
    return __UA_Server_read(server, &nodeId,
                            UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL,
                            outMinimumSamplingInterval);
}

static UA_INLINE UA_StatusCode
UA_Server_readHistorizing(UA_Server *server, const UA_NodeId nodeId,
                          UA_Boolean *outHistorizing) {
    return __UA_Server_read(server, &nodeId, UA_ATTRIBUTEID_HISTORIZING,
                            outHistorizing);
}

static UA_INLINE UA_StatusCode
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
 * - ContainsNoLoop
 *
 * The following attributes cannot be written from the server, as they are
 * specific to the different users:
 *
 * - UserWriteMask
 * - UserAccessLevel
 * - UserExecutable
 *
 * Historizing is currently unsupported */
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
UA_StatusCode UA_EXPORT
UA_Server_write(UA_Server *server, const UA_WriteValue *value);

/* Don't use this function. There are typed versions with no additional
 * overhead. */
UA_StatusCode UA_EXPORT
__UA_Server_write(UA_Server *server, const UA_NodeId *nodeId,
                  const UA_AttributeId attributeId,
                  const UA_DataType *attr_type, const void *attr);

static UA_INLINE UA_StatusCode
UA_Server_writeBrowseName(UA_Server *server, const UA_NodeId nodeId,
                          const UA_QualifiedName browseName) {
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_BROWSENAME,
                             &UA_TYPES[UA_TYPES_QUALIFIEDNAME], &browseName);
}

static UA_INLINE UA_StatusCode
UA_Server_writeDisplayName(UA_Server *server, const UA_NodeId nodeId,
                           const UA_LocalizedText displayName) {
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_DISPLAYNAME,
                             &UA_TYPES[UA_TYPES_LOCALIZEDTEXT], &displayName);
}

static UA_INLINE UA_StatusCode
UA_Server_writeDescription(UA_Server *server, const UA_NodeId nodeId,
                           const UA_LocalizedText description) {
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_DESCRIPTION,
                             &UA_TYPES[UA_TYPES_LOCALIZEDTEXT], &description);
}

static UA_INLINE UA_StatusCode
UA_Server_writeWriteMask(UA_Server *server, const UA_NodeId nodeId,
                         const UA_UInt32 writeMask) {
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_WRITEMASK,
                             &UA_TYPES[UA_TYPES_UINT32], &writeMask);
}

static UA_INLINE UA_StatusCode
UA_Server_writeIsAbstract(UA_Server *server, const UA_NodeId nodeId,
                          const UA_Boolean isAbstract) {
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_ISABSTRACT,
                             &UA_TYPES[UA_TYPES_BOOLEAN], &isAbstract);
}

static UA_INLINE UA_StatusCode
UA_Server_writeInverseName(UA_Server *server, const UA_NodeId nodeId,
                           const UA_LocalizedText inverseName) {
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_INVERSENAME,
                             &UA_TYPES[UA_TYPES_LOCALIZEDTEXT], &inverseName);
}

static UA_INLINE UA_StatusCode
UA_Server_writeEventNotifier(UA_Server *server, const UA_NodeId nodeId,
                             const UA_Byte eventNotifier) {
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_EVENTNOTIFIER,
                             &UA_TYPES[UA_TYPES_BYTE], &eventNotifier);
}

static UA_INLINE UA_StatusCode
UA_Server_writeValue(UA_Server *server, const UA_NodeId nodeId,
                     const UA_Variant value) {
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_VALUE,
                             &UA_TYPES[UA_TYPES_VARIANT], &value);
}

static UA_INLINE UA_StatusCode
UA_Server_writeDataType(UA_Server *server, const UA_NodeId nodeId,
                        const UA_NodeId dataType) {
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_DATATYPE,
                             &UA_TYPES[UA_TYPES_NODEID], &dataType);
}

static UA_INLINE UA_StatusCode
UA_Server_writeValueRank(UA_Server *server, const UA_NodeId nodeId,
                         const UA_Int32 valueRank) {
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_VALUERANK,
                             &UA_TYPES[UA_TYPES_INT32], &valueRank);
}

static UA_INLINE UA_StatusCode
UA_Server_writeArrayDimensions(UA_Server *server, const UA_NodeId nodeId,
                               const UA_Variant arrayDimensions) {
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_VALUE,
                             &UA_TYPES[UA_TYPES_VARIANT], &arrayDimensions);
}

static UA_INLINE UA_StatusCode
UA_Server_writeAccessLevel(UA_Server *server, const UA_NodeId nodeId,
                           const UA_Byte accessLevel) {
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_ACCESSLEVEL,
                             &UA_TYPES[UA_TYPES_BYTE], &accessLevel);
}

static UA_INLINE UA_StatusCode
UA_Server_writeMinimumSamplingInterval(UA_Server *server, const UA_NodeId nodeId,
                                       const UA_Double miniumSamplingInterval) {
    return __UA_Server_write(server, &nodeId,
                             UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL,
                             &UA_TYPES[UA_TYPES_DOUBLE],
                             &miniumSamplingInterval);
}

static UA_INLINE UA_StatusCode
UA_Server_writeExecutable(UA_Server *server, const UA_NodeId nodeId,
                          const UA_Boolean executable) {
    return __UA_Server_write(server, &nodeId, UA_ATTRIBUTEID_EXECUTABLE,
                             &UA_TYPES[UA_TYPES_BOOLEAN], &executable); }

/**
 * Browsing
 * -------- */
UA_BrowseResult UA_EXPORT
UA_Server_browse(UA_Server *server, UA_UInt32 maxrefs,
                 const UA_BrowseDescription *descr);

UA_BrowseResult UA_EXPORT
UA_Server_browseNext(UA_Server *server, UA_Boolean releaseContinuationPoint,
                     const UA_ByteString *continuationPoint);

UA_BrowsePathResult UA_EXPORT
UA_Server_translateBrowsePathToNodeIds(UA_Server *server,
                                       const UA_BrowsePath *browsePath);

#ifndef HAVE_NODEITER_CALLBACK
#define HAVE_NODEITER_CALLBACK
/* Iterate over all nodes referenced by parentNodeId by calling the callback
 * function for each child node (in ifdef because GCC/CLANG handle include order
 * differently) */
typedef UA_StatusCode
(*UA_NodeIteratorCallback)(UA_NodeId childId, UA_Boolean isInverse,
                           UA_NodeId referenceTypeId, void *handle);
#endif

UA_StatusCode UA_EXPORT
UA_Server_forEachChildNodeCall(UA_Server *server, UA_NodeId parentNodeId,
                               UA_NodeIteratorCallback callback, void *handle);

/**
 * Method Call
 * ----------- */
#ifdef UA_ENABLE_METHODCALLS
UA_CallMethodResult UA_EXPORT
UA_Server_call(UA_Server *server, const UA_CallMethodRequest *request);
#endif

/**
 * Node Management
 * ---------------
 *
 * Callback Mechanisms
 * ^^^^^^^^^^^^^^^^^^^
 * There are four mechanisms for callbacks from the node-based information model
 * into userspace:
 *
 * - Datasources for variable nodes, where the variable content is managed
 *   externally
 * - Value-callbacks for variable nodes, where userspace is notified when a
 *   read/write occurs
 * - Object lifecycle management, where a user-defined constructor and
 *   destructor is added to an object type
 * - Method callbacks, where a user-defined method is exposed in the information
 *   model
 *
 * .. _datasource:
 *
 * Data Source Callback
 * ~~~~~~~~~~~~~~~~~~~~
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
    void *handle; /* A custom pointer to reuse the same datasource functions for
                     multiple sources */
    /* Copies the data from the source into the provided value.
     *
     * @param handle An optional pointer to user-defined data for the
     *        specific data source
     * @param nodeid Id of the read node
     * @param includeSourceTimeStamp If true, then the datasource is expected to
     *        set the source timestamp in the returned value
     * @param range If not null, then the datasource shall return only a
     *        selection of the (nonscalar) data. Set
     *        UA_STATUSCODE_BADINDEXRANGEINVALID in the value if this does not
     *        apply.
     * @param value The (non-null) DataValue that is returned to the client. The
     *        data source sets the read data, the result status and optionally a
     *        sourcetimestamp.
     * @return Returns a status code for logging. Error codes intended for the
     *         original caller are set in the value. If an error is returned,
     *         then no releasing of the value is done. */
    UA_StatusCode (*read)(void *handle, const UA_NodeId nodeid,
                          UA_Boolean includeSourceTimeStamp,
                          const UA_NumericRange *range, UA_DataValue *value);

    /* Write into a data source. The write member of UA_DataSource can be empty
     * if the operation is unsupported.
     *
     * @param handle An optional pointer to user-defined data for the
     *        specific data source
     * @param nodeid Id of the node being written to
     * @param data The data to be written into the data source
     * @param range An optional data range. If the data source is scalar or does
     *        not support writing of ranges, then an error code is returned.
     * @return Returns a status code that is returned to the user
     */
    UA_StatusCode (*write)(void *handle, const UA_NodeId nodeid,
                           const UA_Variant *data, const UA_NumericRange *range);
} UA_DataSource;

UA_StatusCode UA_EXPORT
UA_Server_setVariableNode_dataSource(UA_Server *server, const UA_NodeId nodeId,
                                     const UA_DataSource dataSource);

/**
 * .. _value-callback:
 *
 * Value Callback
 * ~~~~~~~~~~~~~~
 * Value Callbacks can be attached to variable and variable type nodes. If
 * not-null, they are called before reading and after writing respectively. */
typedef struct {
    /* Pointer to user-provided data for the callback */
    void *handle;

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
    void (*onRead)(void *handle, const UA_NodeId nodeid,
                   const UA_Variant *data, const UA_NumericRange *range);

    /* Called after writing the value attribute. The node is re-opened after
     * writing so that the new value is visible in the callback.
     *
     * @param handle Points to user-provided data for the callback.
     * @param nodeid The identifier of the node.
     * @param data Points to the current node value (after writing).
     * @param range Points to the numeric range the client wants to write to (or
     *        NULL). */
    void (*onWrite)(void *handle, const UA_NodeId nodeid,
                    const UA_Variant *data, const UA_NumericRange *range);
} UA_ValueCallback;

UA_StatusCode UA_EXPORT
UA_Server_setVariableNode_valueCallback(UA_Server *server, const UA_NodeId nodeId,
                                        const UA_ValueCallback callback);

/**
 * .. _object-lifecycle:
 *
 * Object Lifecycle Management Callbacks
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Lifecycle management adds constructor and destructor callbacks to
 * object types. */
typedef struct {
    /* Returns the instance handle that is then attached to the node */
    void * (*constructor)(const UA_NodeId instance);
    void (*destructor)(const UA_NodeId instance, void *instanceHandle);
} UA_ObjectLifecycleManagement;

UA_StatusCode UA_EXPORT
UA_Server_setObjectTypeNode_lifecycleManagement(UA_Server *server,
                                                UA_NodeId nodeId,
                                                UA_ObjectLifecycleManagement olm);

/**
 * Method Callbacks
 * ~~~~~~~~~~~~~~~~ */
typedef UA_StatusCode
(*UA_MethodCallback)(void *methodHandle, const UA_NodeId objectId,
                     size_t inputSize, const UA_Variant *input,
                     size_t outputSize, UA_Variant *output);

#ifdef UA_ENABLE_METHODCALLS
UA_StatusCode UA_EXPORT
UA_Server_setMethodNode_callback(UA_Server *server, const UA_NodeId methodNodeId,
                                 UA_MethodCallback method, void *handle);
#endif

/**
 * .. _addnodes:
 *
 * Node Addition and Deletion
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^
 *
 * When creating dynamic node instances at runtime, chances are that you will
 * not care about the specific NodeId of the new node, as long as you can
 * reference it later. When passing numeric NodeIds with a numeric identifier 0,
 * the stack evaluates this as "select a random unassigned numeric NodeId in
 * that namespace". To find out which NodeId was actually assigned to the new
 * node, you may pass a pointer `outNewNodeId`, which will (after a successfull
 * node insertion) contain the nodeId of the new node. You may also pass NULL
 * pointer if this result is not relevant. The namespace index for nodes you
 * create should never be 0, as that index is reserved for OPC UA's
 * self-description (namespace * 0).
 *
 * The methods for node addition and deletion take mostly const arguments that
 * are not modified. When creating a node, a deep copy of the node identifier,
 * node attributes, etc. is created. Therefore, it is possible to call for
 * example `UA_Server_addVariablenode` with a value attribute (a :ref:`variant`)
 * pointing to a memory location on the stack. If you need changes to a variable
 * value to manifest at a specific memory location, please use a
 * :ref:`datasource` or a :ref:`value-callback`. */
/* The instantiation callback is used to track the addition of new nodes. It is
 * also called for all sub-nodes contained in an object or variable type node
 * that is instantiated. */
typedef struct {
  UA_StatusCode (*method)(const UA_NodeId objectId,
                          const UA_NodeId typeDefinitionId, void *handle);
  void *handle;
} UA_InstantiationCallback;

/* Don't use this function. There are typed versions as inline functions. */
UA_StatusCode UA_EXPORT
__UA_Server_addNode(UA_Server *server, const UA_NodeClass nodeClass,
                    const UA_NodeId requestedNewNodeId,
                    const UA_NodeId parentNodeId,
                    const UA_NodeId referenceTypeId,
                    const UA_QualifiedName browseName,
                    const UA_NodeId typeDefinition,
                    const UA_NodeAttributes *attr,
                    const UA_DataType *attributeType,
                    UA_InstantiationCallback *instantiationCallback,
                    UA_NodeId *outNewNodeId);

static UA_INLINE UA_StatusCode
UA_Server_addVariableNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                          const UA_NodeId parentNodeId,
                          const UA_NodeId referenceTypeId,
                          const UA_QualifiedName browseName,
                          const UA_NodeId typeDefinition,
                          const UA_VariableAttributes attr,
                          UA_InstantiationCallback *instantiationCallback,
                          UA_NodeId *outNewNodeId) {
    return __UA_Server_addNode(server, UA_NODECLASS_VARIABLE, requestedNewNodeId,
                               parentNodeId, referenceTypeId, browseName,
                               typeDefinition, (const UA_NodeAttributes*)&attr,
                               &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
                               instantiationCallback, outNewNodeId);
}

static UA_INLINE UA_StatusCode
UA_Server_addVariableTypeNode(UA_Server *server,
                              const UA_NodeId requestedNewNodeId,
                              const UA_NodeId parentNodeId,
                              const UA_NodeId referenceTypeId,
                              const UA_QualifiedName browseName,
                              const UA_NodeId typeDefinition,
                              const UA_VariableTypeAttributes attr,
                              UA_InstantiationCallback *instantiationCallback,
                              UA_NodeId *outNewNodeId) {
    return __UA_Server_addNode(server, UA_NODECLASS_VARIABLETYPE,
                               requestedNewNodeId, parentNodeId, referenceTypeId,
                               browseName, typeDefinition,
                               (const UA_NodeAttributes*)&attr,
                               &UA_TYPES[UA_TYPES_VARIABLETYPEATTRIBUTES],
                               instantiationCallback, outNewNodeId);
}

static UA_INLINE UA_StatusCode
UA_Server_addObjectNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                        const UA_NodeId parentNodeId,
                        const UA_NodeId referenceTypeId,
                        const UA_QualifiedName browseName,
                        const UA_NodeId typeDefinition,
                        const UA_ObjectAttributes attr,
                        UA_InstantiationCallback *instantiationCallback,
                        UA_NodeId *outNewNodeId) {
    return __UA_Server_addNode(server, UA_NODECLASS_OBJECT, requestedNewNodeId,
                               parentNodeId, referenceTypeId, browseName,
                               typeDefinition, (const UA_NodeAttributes*)&attr,
                               &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                               instantiationCallback, outNewNodeId);
}

static UA_INLINE UA_StatusCode
UA_Server_addObjectTypeNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                            const UA_NodeId parentNodeId,
                            const UA_NodeId referenceTypeId,
                            const UA_QualifiedName browseName,
                            const UA_ObjectTypeAttributes attr,
                            UA_InstantiationCallback *instantiationCallback,
                            UA_NodeId *outNewNodeId) {
    return __UA_Server_addNode(server, UA_NODECLASS_OBJECTTYPE, requestedNewNodeId,
                               parentNodeId, referenceTypeId, browseName,
                               UA_NODEID_NULL, (const UA_NodeAttributes*)&attr,
                               &UA_TYPES[UA_TYPES_OBJECTTYPEATTRIBUTES],
                               instantiationCallback, outNewNodeId);
}

static UA_INLINE UA_StatusCode
UA_Server_addViewNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                      const UA_NodeId parentNodeId,
                      const UA_NodeId referenceTypeId,
                      const UA_QualifiedName browseName,
                      const UA_ViewAttributes attr,
                      UA_InstantiationCallback *instantiationCallback,
                      UA_NodeId *outNewNodeId) {
    return __UA_Server_addNode(server, UA_NODECLASS_VIEW, requestedNewNodeId,
                               parentNodeId, referenceTypeId, browseName,
                               UA_NODEID_NULL, (const UA_NodeAttributes*)&attr,
                               &UA_TYPES[UA_TYPES_VIEWATTRIBUTES],
                               instantiationCallback, outNewNodeId);
}

static UA_INLINE UA_StatusCode
UA_Server_addReferenceTypeNode(UA_Server *server,
                               const UA_NodeId requestedNewNodeId,
                               const UA_NodeId parentNodeId,
                               const UA_NodeId referenceTypeId,
                               const UA_QualifiedName browseName,
                               const UA_ReferenceTypeAttributes attr,
                               UA_InstantiationCallback *instantiationCallback,
                               UA_NodeId *outNewNodeId) {
    return __UA_Server_addNode(server, UA_NODECLASS_REFERENCETYPE,
                               requestedNewNodeId, parentNodeId, referenceTypeId,
                               browseName, UA_NODEID_NULL,
                               (const UA_NodeAttributes*)&attr,
                               &UA_TYPES[UA_TYPES_REFERENCETYPEATTRIBUTES],
                               instantiationCallback, outNewNodeId);
}

static UA_INLINE UA_StatusCode
UA_Server_addDataTypeNode(UA_Server *server,
                          const UA_NodeId requestedNewNodeId,
                          const UA_NodeId parentNodeId,
                          const UA_NodeId referenceTypeId,
                          const UA_QualifiedName browseName,
                          const UA_DataTypeAttributes attr,
                          UA_InstantiationCallback *instantiationCallback,
                          UA_NodeId *outNewNodeId) {
    return __UA_Server_addNode(server, UA_NODECLASS_DATATYPE, requestedNewNodeId,
                               parentNodeId, referenceTypeId, browseName,
                               UA_NODEID_NULL, (const UA_NodeAttributes*)&attr,
                               &UA_TYPES[UA_TYPES_DATATYPEATTRIBUTES],
                               instantiationCallback, outNewNodeId);
}

UA_StatusCode UA_EXPORT
UA_Server_addDataSourceVariableNode(UA_Server *server,
                                    const UA_NodeId requestedNewNodeId,
                                    const UA_NodeId parentNodeId,
                                    const UA_NodeId referenceTypeId,
                                    const UA_QualifiedName browseName,
                                    const UA_NodeId typeDefinition,
                                    const UA_VariableAttributes attr,
                                    const UA_DataSource dataSource,
                                    UA_NodeId *outNewNodeId);

#ifdef UA_ENABLE_METHODCALLS
UA_StatusCode UA_EXPORT
UA_Server_addMethodNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                        const UA_NodeId parentNodeId,
                        const UA_NodeId referenceTypeId,
                        const UA_QualifiedName browseName,
                        const UA_MethodAttributes attr,
                        UA_MethodCallback method, void *handle,
                        size_t inputArgumentsSize,
                        const UA_Argument* inputArguments, 
                        size_t outputArgumentsSize,
                        const UA_Argument* outputArguments,
                        UA_NodeId *outNewNodeId);
#endif

UA_StatusCode UA_EXPORT
UA_Server_deleteNode(UA_Server *server, const UA_NodeId nodeId,
                     UA_Boolean deleteReferences);

/**
 * Reference Management
 * -------------------- */
UA_StatusCode UA_EXPORT
UA_Server_addReference(UA_Server *server, const UA_NodeId sourceId,
                       const UA_NodeId refTypeId,
                       const UA_ExpandedNodeId targetId, UA_Boolean isForward);

UA_StatusCode UA_EXPORT
UA_Server_deleteReference(UA_Server *server, const UA_NodeId sourceNodeId,
                          const UA_NodeId referenceTypeId, UA_Boolean isForward,
                          const UA_ExpandedNodeId targetNodeId,
                          UA_Boolean deleteBidirectional);

#ifdef __cplusplus
}
#endif

#endif /* UA_SERVER_H_ */
