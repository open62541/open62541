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
#include "ua_connection.h"
#include "ua_log.h"
  
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

struct UA_Server;
typedef struct UA_Server UA_Server;

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
     * @param includeSourceTimeStamp If true, then the datasource is expected to set the source
     *        timestamp in the returned value
     * @param range If not null, then the datasource shall return only a selection of the (nonscalar)
     *        data. Set UA_STATUSCODE_BADINDEXRANGEINVALID in the value if this does not apply.
     * @param value The (non-null) DataValue that is returned to the client. The data source sets the
     *        read data, the result status and optionally a sourcetimestamp.
     * @return Returns a status code for logging. Error codes intended for the original caller are set
     *         in the value. If an error is returned, then no releasing of the value is done.
     */
    UA_StatusCode (*read)(void *handle, UA_Boolean includeSourceTimeStamp, const UA_NumericRange *range, UA_DataValue *value);

    /**
     * Write into a data source. The write member of UA_DataSource can be empty if the operation
     * is unsupported.
     *
     * @param handle An optional pointer to user-defined data for the specific data source
     * @param data The data to be written into the data source
     * @param range An optional data range. If the data source is scalar or does not support writing
     *        of ranges, then an error code is returned.
     * @return Returns a status code that is returned to the user
     */
    UA_StatusCode (*write)(void *handle, const UA_Variant *data, const UA_NumericRange *range);
} UA_DataSource;

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
UA_StatusCode UA_EXPORT
UA_Server_deleteNode(UA_Server *server, UA_NodeId nodeId);

#define UA_SERVER_DELETENODEALIAS_DECL(TYPE) \
UA_StatusCode UA_EXPORT UA_Server_delete##TYPE##Node(UA_Server *server, UA_NodeId nodeId);

/** Deletes an ObjectNode from the nodestore. This is a high-level alias for UA_Server_deleteNode()
 *
 * @param server The server object
 * @param nodeId ID of the node to be deleted
 * 
 * @return Return UA_STATUSCODE_GOOD if the node was deleted or an appropriate errorcode if the node was not found
 *         or cannot be deleted.
 */
UA_SERVER_DELETENODEALIAS_DECL(Object)

/** Deletes a VariableNode from the nodestore. This is a high-level alias for UA_Server_deleteNode()
 *
 * @param server The server object
 * @param nodeId ID of the node to be deleted
 * 
 * @return Return UA_STATUSCODE_GOOD if the node was deleted or an appropriate errorcode if the node was not found
 *         or cannot be deleted.
 */
UA_SERVER_DELETENODEALIAS_DECL(Variable)

/** Deletes a ReferenceType node from the nodestore. This is a high-level alias for UA_Server_deleteNode()
 * 
 * @param server The server object
 * @param nodeId ID of the node to be deleted
 * 
 * @return Return UA_STATUSCODE_GOOD if the node was deleted or an appropriate errorcode if the node was not found
 *         or cannot be deleted.
 */
UA_SERVER_DELETENODEALIAS_DECL(ReferenceType)

/** Deletes a View Node from the nodestore. This is a high-level alias for UA_Server_deleteNode()
 * 
 * @param server The server object
 * @param nodeId ID of the node to be deleted
 * 
 * @return Return UA_STATUSCODE_GOOD if the node was deleted or an appropriate errorcode if the node was not found
 *         or cannot be deleted.
 */
UA_SERVER_DELETENODEALIAS_DECL(View)

/** Deletes a VariableType node from the nodestore. This is a high-level alias for UA_Server_deleteNode()
 * 
 * @param server The server object
 * @param nodeId ID of the node to be deleted
 * 
 * @return Return UA_STATUSCODE_GOOD if the node was deleted or an appropriate errorcode if the node was not found
 *         or cannot be deleted.
 */
UA_SERVER_DELETENODEALIAS_DECL(VariableType)

/** Deletes a DataType Node from the nodestore. This is a high-level alias for UA_Server_deleteNode()
 * 
 * @param server The server object
 * @param nodeId ID of the node to be deleted
 * 
 * @return Return UA_STATUSCODE_GOOD if the node was deleted or an appropriate errorcode if the node was not found
 *         or cannot be deleted.
 */
UA_SERVER_DELETENODEALIAS_DECL(DataType)

#ifdef ENABLE_METHODCALLS
/** Deletes an MethodNode from the nodestore. This is a high-level alias for UA_Server_deleteNode()
 *
 * @param server The server object
 * @param nodeId ID of the node to be deleted
 * 
 * @return Return UA_STATUSCODE_GOOD if the node was deleted or an appropriate errorcode if the node was not found
 *         or cannot be deleted.
 */
UA_SERVER_DELETENODEALIAS_DECL(Method)
#endif

/** Deletes a copied instance of a node by deallocating it and all its attributes. This assumes that the node was
 * priorly copied using getNodeCopy. To delete nodes that are located in the nodestore, use UA_Server_deleteNode()
 * instead.
 *
 * @param server The server object
 * @param nodeId ID of the node copy to be deleted
 * 
 * @return Return UA_STATUSCODE_GOOD if the node was deleted or an appropriate errorcode if the node was not found
 *         or cannot be deleted.
 */
UA_StatusCode UA_EXPORT 
UA_Server_deleteNodeCopy(UA_Server *server, void **node);

/** Creates a deep copy of a node located in the nodestore and returns it to the userspace. Note that any manipulation
 * of this copied node is not reflected by the server, but otherwise not accessible attributes of the node's struct
 * can be examined in bulk. node->nodeClass can be used to cast the node to a specific node type. Use 
 * UA_Server_deleteNodeCopy() to deallocate this node.
 *
 * @param server The server object
 * @param nodeId ID of the node copy to be copied
 * @param copyInto Pointer to a NULL pointer that will hold the copy of the node on a successfull return.
 * 
 * @return Return UA_STATUSCODE_GOOD if the node was copied or an appropriate errorcode if the node was not found
 *         or cannot be copied.
 */
UA_StatusCode UA_EXPORT 
UA_Server_getNodeCopy(UA_Server *server, UA_NodeId nodeId, void **copyInto);

/** A new variable Node with a value passed in variant.
 *
 * @param server The server object
 * @param nodeId        The requested nodeId of the new node. Use the numeric id with i=0 to get a new ID from the server.
 * @param browseName    The qualified name of this node
 * @param displayName   The localized text shown when displaying the node
 * @param description   The localized human readable description
 * @param parentNodeId  The node under which this node exists ("parent")
 * @param referenceTypeId Reference type used by the parent to reference this node
 * @param userWriteMask Bitmask defining the user write permissions
 * @param writeMask     Bitmask defining the write permissions
 * @param value         A variant containing the value to be assigned to this node.
 * @param copyInto Pointer to a NULL pointer that will hold the copy of the node on a successfull return.
 * 
 * @return Return UA_STATUSCODE_GOOD if the node was created or an appropriate error code if not.
 */
UA_StatusCode UA_EXPORT
UA_Server_addVariableNode(UA_Server *server, UA_NodeId nodeId, const UA_QualifiedName browseName, 
                          UA_LocalizedText displayName, UA_LocalizedText description, const UA_NodeId parentNodeId, 
                          const UA_NodeId referenceTypeId, UA_UInt32 userWriteMask, UA_UInt32 writeMask, 
                          UA_Variant *value, UA_NodeId *createdNodeId);

UA_StatusCode UA_EXPORT
UA_Server_addObjectNode(UA_Server *server, UA_NodeId nodeId, const UA_QualifiedName browseName, 
                        UA_LocalizedText displayName, UA_LocalizedText description, const UA_NodeId parentNodeId, 
                        const UA_NodeId referenceTypeId, UA_UInt32 userWriteMask, UA_UInt32 writeMask, 
                        const UA_ExpandedNodeId typeDefinition, UA_NodeId *createdNodeId);

UA_StatusCode UA_EXPORT 
UA_Server_addReferenceTypeNode(UA_Server *server, UA_NodeId nodeId, UA_QualifiedName browseName, 
                               UA_LocalizedText displayName, UA_LocalizedText description, UA_NodeId parentNodeId, 
                               const UA_NodeId referenceTypeId, UA_UInt32 userWriteMask, UA_UInt32 writeMask, 
                               const UA_ExpandedNodeId typeDefinition, UA_LocalizedText inverseName, UA_NodeId *createdNodeId );

UA_StatusCode UA_EXPORT
UA_Server_addObjectTypeNode(UA_Server *server, UA_NodeId nodeId, UA_QualifiedName browseName, 
                            UA_LocalizedText displayName, UA_LocalizedText description, UA_NodeId parentNodeId, 
                            UA_NodeId referenceTypeId, UA_UInt32 userWriteMask, UA_UInt32 writeMask, 
                            UA_ExpandedNodeId typeDefinition, UA_Boolean isAbstract,  
                            UA_NodeId *createdNodeId );

UA_StatusCode UA_EXPORT 
UA_Server_addVariableTypeNode(UA_Server *server, UA_NodeId nodeId, const UA_QualifiedName browseName, 
                              UA_LocalizedText displayName, UA_LocalizedText description, const UA_NodeId parentNodeId, 
                              const UA_NodeId referenceTypeId, UA_UInt32 userWriteMask, UA_UInt32 writeMask, 
                              UA_Variant *value,  UA_Int32 valueRank, UA_Boolean isAbstract, UA_NodeId *createdNodeId);

UA_StatusCode UA_EXPORT
UA_Server_addDataTypeNode(UA_Server *server, UA_NodeId nodeId, const UA_QualifiedName browseName, 
                          UA_LocalizedText displayName, UA_LocalizedText description, const UA_NodeId parentNodeId, 
                          const UA_NodeId referenceTypeId, UA_UInt32 userWriteMask, UA_UInt32 writeMask, 
                          UA_ExpandedNodeId typeDefinition, UA_Boolean isAbstract, UA_NodeId *createdNodeId);


UA_StatusCode UA_EXPORT
UA_Server_addViewNode(UA_Server *server, UA_NodeId nodeId, const UA_QualifiedName browseName, 
                      UA_LocalizedText displayName, UA_LocalizedText description, const UA_NodeId parentNodeId, 
                      const UA_NodeId referenceTypeId, UA_UInt32 userWriteMask, UA_UInt32 writeMask, 
                      UA_ExpandedNodeId typeDefinition, UA_NodeId *createdNodeId);

UA_StatusCode UA_EXPORT
UA_Server_addDataSourceVariableNode(UA_Server *server, UA_DataSource dataSource,
                                    const UA_QualifiedName browseName, UA_NodeId nodeId,
                                    const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                                    UA_NodeId *createdNodeId);

UA_StatusCode UA_EXPORT
UA_Server_AddMonodirectionalReference(UA_Server *server, UA_NodeId sourceNodeId,
                                      UA_ExpandedNodeId targetNodeId, UA_NodeId referenceTypeId,
                                      UA_Boolean isforward);

#ifdef ENABLE_METHODCALLS
typedef UA_StatusCode (*UA_MethodCallback)(const UA_NodeId objectId, const UA_Variant *input,
                                           UA_Variant *output);
/** Creates a serverside method including input- and output variable descriptions
 * 
 * @param server The server object.
 * 
 * @param browseName BrowseName to be used for the new method.
 * 
 * @param nodeId Requested NodeId for the new method. If a numeric ID with i=0 is used, the server will assign a random unused id.
 * 
 * @param parentNodeId Parent node containing this method. Note that an ObjectNode needs to reference the method with hasProperty in order for the method to be callable.
 * 
 * @param referenceTypeId Reference type ID to be used by the parent to reference the new method.
 * 
 * @param method Userspace Method/Function of type UA_MethodCallback to be called when a client invokes the method using the Call Service Set.
 * 
 * @param inputArgumentsSize Number of input arguments expected to be passed by a calling client.
 * 
 * @param inputArguments Description of input arguments expected to be passed by a calling client.
 * 
 * @param outputArgumentsSize Description of output arguments expected to be passed by a calling client.
 * 
 * @param outputArguments Description of output arguments expected to be passed by a calling client.
 * 
 * @param createdNodeId Actual nodeId of the new method node if UA_StatusCode indicates success. Can be used to determine the random unique ID assigned by the server if i=0 was passed as a nodeId.
 * 
 */
UA_StatusCode UA_EXPORT
UA_Server_addMethodNode(UA_Server *server, const UA_QualifiedName browseName, UA_NodeId nodeId,
                        const UA_ExpandedNodeId parentNodeId, const UA_NodeId referenceTypeId,
                        UA_MethodCallback method, UA_Int32 inputArgumentsSize,
                        const UA_Argument *inputArguments, UA_Int32 outputArgumentsSize,
                        const UA_Argument *outputArguments,
                        UA_NodeId *createdNodeId);
#endif

typedef UA_StatusCode (*UA_NodeIteratorCallback)(UA_NodeId childId, UA_Boolean isInverse, UA_NodeId referenceTypeId);

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
UA_StatusCode UA_EXPORT UA_Server_forEachChildNodeCall(UA_Server *server, UA_NodeId parentNodeId, UA_NodeIteratorCallback callback);

/** Jobs describe work that is executed once or repeatedly. */
typedef struct {
    enum {
        UA_JOBTYPE_NOTHING,
        UA_JOBTYPE_DETACHCONNECTION,
        UA_JOBTYPE_BINARYMESSAGE,
        UA_JOBTYPE_METHODCALL,
        UA_JOBTYPE_DELAYEDMETHODCALL,
    } type;
    union {
        UA_Connection *closeConnection;
        struct {
            UA_Connection *connection;
            UA_ByteString message;
        } binaryMessage;
        struct {
            void *data;
            void (*method)(UA_Server *server, void *data);
        } methodCall;
    } job;
} UA_Job;

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
 * Remove repeated job. The entry will be removed asynchronously during the
 * next iteration of the server main loop.
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
    void *handle;
    UA_String discoveryUrl;

    /**
     * Starts listening on the the networklayer.
     *
     * @param nl The network layer
     * @param logger The logger
     * @return Returns UA_STATUSCODE_GOOD or an error code.
     */
    UA_StatusCode (*start)(struct UA_ServerNetworkLayer *nl, UA_Logger *logger);
    
    /**
     * Gets called from the main server loop and returns the jobs (accumulated messages and close
     * events) for dispatch.
     *
     * @param nl The network layer
     * @param jobs When the returned integer is positive, *jobs points to an array of UA_Job of the
     * returned size.
     * @param timeout The timeout during which an event must arrive in microseconds
     * @return The size of the jobs array. If the result is negative, an error has occurred.
     */
    UA_Int32 (*getJobs)(struct UA_ServerNetworkLayer *nl, UA_Job **jobs, UA_UInt16 timeout);

    /**
     * Closes the network connection and returns all the jobs that need to be finished before the
     * network layer can be safely deleted.
     *
     * @param nl The network layer
     * @param jobs When the returned integer is positive, jobs points to an array of UA_Job of the
     * returned size.
     * @return The size of the jobs array. If the result is negative, an error has occurred.
     */
    UA_Int32 (*stop)(struct UA_ServerNetworkLayer *nl, UA_Job **jobs);

    /** Deletes the network layer. Call only after a successful shutdown. */
    void (*deleteMembers)(struct UA_ServerNetworkLayer *nl);
} UA_ServerNetworkLayer;

/**
 * Adds a network layer to the server. The network layer is destroyed together
 * with the server. Do not use it after adding it as it might be moved around on
 * the heap.
 */
void UA_EXPORT UA_Server_addNetworkLayer(UA_Server *server, UA_ServerNetworkLayer networkLayer);

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
(void *ensHandle, const UA_RequestHeader *requestHeader, UA_BrowsePath *browsePath,
 UA_UInt32 *indices, UA_UInt32 indicesSize, UA_BrowsePathResult *browsePathResults, UA_DiagnosticInfo *diagnosticInfos);

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


UA_StatusCode UA_EXPORT
UA_Server_addExternalNamespace(UA_Server *server, UA_UInt16 namespaceIndex, const UA_String *url, UA_ExternalNodeStore *nodeStore);
/** @} */

#endif /* external nodestore */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_SERVER_H_ */
