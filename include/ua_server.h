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
 * Runs the main loop of the server. In each iteration, this calls into the
 * networklayers to see if work have arrived and checks if timed events need to
 * be triggered.
 *
 * @param server The server object
 * @param nThreads The number of worker threads. Is ignored if MULTITHREADING is
 * not activated.
 * @param running Points to a booloean value on the heap. When running is set to
 * false, the worker threads and the main loop close and the server is shut
 * down.
 * @return Indicates whether the server shut down cleanly
 *
 */
UA_StatusCode UA_EXPORT UA_Server_run(UA_Server *server, UA_UInt16 nThreads, UA_Boolean *running);

/**
 * Datasources are the interface to local data providers. Implementors of datasources need to
 * provide functions for the callbacks in this structure. After every read, the handle needs to be
 * released to indicate that the pointer is no longer accessed. As a rule, datasources are never
 * copied, but only their content. The only way to write into a datasource is via the write-service.
 *
 * It is expected that the read and release callbacks are implemented. The write
 * callback can be set to null.
 */
typedef struct {
    void *handle; ///> A custom pointer to reuse the same datasource functions for multiple sources

    /**
     * Read current data from the data source
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
    UA_StatusCode (*read)(void *handle, UA_Boolean includeSourceTimeStamp, const UA_NumericRange *range,
                          UA_DataValue *value);

    /**
     * Release data that was allocated during a read (and/or release locks in the data source)
     *
     * @param handle An optional pointer to user-defined data for the specific data source
     * @param value The DataValue that was used for a successful read.
     */
    void (*release)(void *handle, UA_DataValue *value);

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
UA_StatusCode UA_EXPORT UA_Server_addReference(UA_Server *server, const UA_AddReferencesItem *item);

UA_StatusCode UA_EXPORT
UA_Server_addVariableNode(UA_Server *server, UA_Variant *value, const UA_QualifiedName browseName, 
                          UA_NodeId nodeId, const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId);

UA_StatusCode UA_EXPORT
UA_Server_addObjectNode(UA_Server *server, const UA_QualifiedName browseName,
                        UA_NodeId nodeId, const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                        const UA_NodeId typeDefinition);

UA_StatusCode UA_EXPORT
UA_Server_addDataSourceVariableNode(UA_Server *server, UA_DataSource dataSource,
                                    const UA_QualifiedName browseName, UA_NodeId nodeId,
                                    const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId);

/** Work that is run in the main loop (singlethreaded) or dispatched to a worker thread */
typedef struct UA_WorkItem {
    enum {
        UA_WORKITEMTYPE_NOTHING,
        UA_WORKITEMTYPE_CLOSECONNECTION,
        UA_WORKITEMTYPE_BINARYMESSAGE,
        UA_WORKITEMTYPE_METHODCALL,
        UA_WORKITEMTYPE_DELAYEDMETHODCALL,
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
    } work;
} UA_WorkItem;

/**
 * @param server The server object.
 *
 * @param work Pointer to the WorkItem that shall be added. The pointer is not
 *        freed but copied to an internal representation.
 *
 * @param executionTime The time when the work shall be executed. If the time lies in the
 *        past, the work will be executed in the next iteration of the server's
 *        main loop
 *
 * @param resultWorkGuid Upon success, the pointed value is set to the guid of
 *        the workitem in the server queue. This can be used to cancel the work
 *        later on. If the pointer is null, the guid is not set.
 *
 * @return Upon sucess, UA_STATUSCODE_GOOD is returned. An error code otherwise.
 */
UA_StatusCode UA_EXPORT
UA_Server_addTimedWorkItem(UA_Server *server, const UA_WorkItem *work,
                           UA_DateTime executionTime, UA_Guid *resultWorkGuid);

/**
 * @param server The server object.
 *
 * @param work Pointer to the WorkItem that shall be added. The pointer is not
 *        freed but copied to an internal representation.
 *
 * @param interval The work that is executed repeatedly with the given interval
 *        (in ms). If work with the same repetition interval already exists,
 *        the first execution might occur sooner.
 *
 * @param resultWorkGuid Upon success, the pointed value is set to the guid of
 *        the workitem in the server queue. This can be used to cancel the work
 *        later on. If the pointer is null, the guid is not set.
 *
 * @return Upon sucess, UA_STATUSCODE_GOOD is returned. An error code otherwise.
 */
UA_StatusCode UA_EXPORT
UA_Server_addRepeatedWorkItem(UA_Server *server, const UA_WorkItem *work,
                              UA_UInt32 interval, UA_Guid *resultWorkGuid);

/** Remove timed or repeated work */
/* UA_Boolean UA_EXPORT UA_Server_removeWorkItem(UA_Server *server, UA_Guid workId); */

/**
 * Interface to the binary network layers. This structure is returned from the
 * function that initializes the network layer. The layer is already bound to a
 * specific port and listening. The functions in the structure are never called
 * in parallel but only sequentially from the server's main loop. So the network
 * layer does not need to be thread-safe.
 */
typedef struct {
    void *nlHandle;

    /**
     * Starts listening on the the networklayer.
     *
     * @return Returns UA_STATUSCODE_GOOD or an error code.
     */
    UA_StatusCode (*start)(void *nlHandle, UA_Logger *logger);
    
    /**
     * Gets called from the main server loop and returns the work that
     * accumulated (messages and close events) for dispatch. The networklayer
     * does not wait on connections but returns immediately the work that
     * accumulated.
     *
     * @param workItems When the returned integer is positive, *workItems points
     * to an array of WorkItems of the returned size.
     * @param timeout The timeout during which an event must arrive in microseconds
     * @return The size of the returned workItems array. If the result is
     * negative, an error has occured.
     */
    UA_Int32 (*getWork)(void *nlhandle, UA_WorkItem **workItems, UA_UInt16 timeout);

    /**
     * Closes the network connection and returns all the work that needs to
     * be finished before the network layer can be safely deleted.
     *
     * @param workItems When the returned integer is positive, *workItems points
     * to an array of WorkItems of the returned size.
     * @return The size of the returned workItems array. If the result is
     * negative, an error has occured.
     */
    UA_Int32 (*stop)(void *nlhandle, UA_WorkItem **workItems);

    /** Deletes the network layer. Call only after a successful shutdown. */
    void (*free)(void *nlhandle);

    /**
     * String containing the discovery URL that will be add to the server's list
     * contains the protocol the host and the port of the layer
     */
    UA_String* discoveryUrl;
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

/* UA_StatusCode UA_EXPORT */
/* UA_Server_addExternalNamespace(UA_Server *server, UA_UInt16 namespaceIndex, const UA_String *url, UA_ExternalNodeStore *nodeStore); */

/** @} */

#endif /* external nodestore */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_SERVER_H_ */
