/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef UA_SERVER_INTERNAL_H_
#define UA_SERVER_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_util.h"
#include "ua_server.h"
#include "ua_timer.h"
#include "ua_connection_internal.h"
#include "ua_session_manager.h"
#include "ua_securechannel_manager.h"
#include "ua_nodestore.h"

#ifdef UA_ENABLE_DISCOVERY_MULTICAST
#include "mdnsd/libmdnsd/mdnsd.h"
#endif

/* The general idea of RCU is to delay freeing nodes (or any callback invoked
 * with call_rcu) until all threads have left their critical section. Thus we
 * can delete nodes safely in concurrent operations. The macros UA_RCU_LOCK and
 * UA_RCU_UNLOCK are used to test during debugging that we do not nest read-side
 * critical sections (although this is generally allowed). */
#ifdef UA_ENABLE_MULTITHREADING
# define _LGPL_SOURCE
# include <urcu.h>
# include <urcu/lfstack.h>
# ifdef NDEBUG
#  define UA_RCU_LOCK() rcu_read_lock()
#  define UA_RCU_UNLOCK() rcu_read_unlock()
#  define UA_ASSERT_RCU_LOCKED()
#  define UA_ASSERT_RCU_UNLOCKED()
# else
   extern UA_THREAD_LOCAL bool rcu_locked;
#  define UA_ASSERT_RCU_LOCKED() assert(rcu_locked)
#  define UA_ASSERT_RCU_UNLOCKED() assert(!rcu_locked)
#  define UA_RCU_LOCK() do {                      \
        UA_ASSERT_RCU_UNLOCKED();                 \
        rcu_locked = true;                        \
        rcu_read_lock(); } while(0)
#  define UA_RCU_UNLOCK() do {                    \
        UA_ASSERT_RCU_LOCKED();                   \
        rcu_locked = false;                       \
        rcu_read_unlock(); } while(0)
# endif
#else
# define UA_RCU_LOCK()
# define UA_RCU_UNLOCK()
# define UA_ASSERT_RCU_LOCKED()
# define UA_ASSERT_RCU_UNLOCKED()
#endif

#ifdef UA_ENABLE_MULTITHREADING
struct UA_Worker;
typedef struct UA_Worker UA_Worker;
#endif

#if defined(UA_ENABLE_METHODCALLS) && defined(UA_ENABLE_SUBSCRIPTIONS)
/* Internally used context to a session 'context' of the current mehtod call */
extern UA_THREAD_LOCAL UA_Session* methodCallSession;
#endif

#ifdef UA_ENABLE_DISCOVERY

typedef struct registeredServer_list_entry {
    LIST_ENTRY(registeredServer_list_entry) pointers;
    UA_RegisteredServer registeredServer;
    UA_DateTime lastSeen;
} registeredServer_list_entry;

# ifdef UA_ENABLE_DISCOVERY_MULTICAST
typedef struct serverOnNetwork_list_entry {
    LIST_ENTRY(serverOnNetwork_list_entry) pointers;
    UA_ServerOnNetwork serverOnNetwork;
    UA_DateTime created;
    UA_DateTime lastSeen;
    UA_Boolean txtSet;
    UA_Boolean srvSet;
    char* pathTmp;
} serverOnNetwork_list_entry;

#define SERVER_ON_NETWORK_HASH_PRIME 1009
typedef struct serverOnNetwork_hash_entry {
    serverOnNetwork_list_entry* entry;
    struct serverOnNetwork_hash_entry* next;
} serverOnNetwork_hash_entry;

#endif /* UA_ENABLE_DISCOVERY_MULTICAST */

#endif /* UA_ENABLE_DISCOVERY */

struct UA_Server {
    /* Meta */
    UA_DateTime startTime;

    /* Security */
    UA_SecureChannelManager secureChannelManager;
    UA_SessionManager sessionManager;

    /* Address Space */
    UA_NodeStore *nodestore;

#ifdef UA_ENABLE_DISCOVERY
    /* Discovery */
    LIST_HEAD(registeredServer_list, registeredServer_list_entry) registeredServers; // doubly-linked list of registered servers
    size_t registeredServersSize;
    struct PeriodicServerRegisterCallback *periodicServerRegisterCallback;
    UA_Server_registerServerCallback registerServerCallback;
    void* registerServerCallbackData;
# ifdef UA_ENABLE_DISCOVERY_MULTICAST
    mdns_daemon_t *mdnsDaemon;
    int mdnsSocket;
    UA_Boolean mdnsMainSrvAdded;
#  ifdef UA_ENABLE_MULTITHREADING
    pthread_t mdnsThread;
    UA_Boolean mdnsRunning;
#  endif

    LIST_HEAD(serverOnNetwork_list, serverOnNetwork_list_entry) serverOnNetwork; // doubly-linked list of servers on the network (from mDNS)
    size_t serverOnNetworkSize;
    UA_UInt32 serverOnNetworkRecordIdCounter;
    UA_DateTime serverOnNetworkRecordIdLastReset;
    // hash mapping domain name to serverOnNetwork list entry
    struct serverOnNetwork_hash_entry* serverOnNetworkHash[SERVER_ON_NETWORK_HASH_PRIME];

    UA_Server_serverOnNetworkCallback serverOnNetworkCallback;
    void* serverOnNetworkCallbackData;

# endif
#endif

    size_t namespacesSize;
    UA_String *namespaces;

    /* Callbacks with a repetition interval */
    UA_Timer timer;

    /* Delayed callbacks */
    SLIST_HEAD(DelayedCallbacksList, UA_DelayedCallback) delayedCallbacks;

    /* Worker threads */
#ifdef UA_ENABLE_MULTITHREADING
    /* Dispatch queue head for the worker threads (the tail should not be in the same cache line) */
    struct cds_wfcq_head dispatchQueue_head;
    UA_Worker *workers; /* there are nThread workers in a running server */
    pthread_cond_t dispatchQueue_condition; /* so the workers don't spin if the queue is empty */
    pthread_mutex_t dispatchQueue_mutex; /* mutex for access to condition variable */
    struct cds_wfcq_tail dispatchQueue_tail; /* Dispatch queue tail for the worker threads */
#endif

    /* Config is the last element so that MSVC allows the usernamePasswordLogins
     * field with zero-sized array */
    UA_ServerConfig config;
};

/*****************/
/* Node Handling */
/*****************/

void UA_Node_deleteMembersAnyNodeClass(UA_Node *node);
void UA_Node_deleteReferences(UA_Node *node);
UA_StatusCode UA_Node_copyAnyNodeClass(const UA_Node *src, UA_Node *dst);

/* Calls callback on the node. In the multithreaded case, the node is copied before and replaced in
   the nodestore. */
typedef UA_StatusCode (*UA_EditNodeCallback)(UA_Server*, UA_Session*, UA_Node*, const void*);
UA_StatusCode UA_Server_editNode(UA_Server *server, UA_Session *session, const UA_NodeId *nodeId,
                                 UA_EditNodeCallback callback, const void *data);

/*************/
/* Callbacks */
/*************/

/* Delayed callbacks are executed when all previously dispatched callbacks are
 * finished */
UA_StatusCode
UA_Server_delayedCallback(UA_Server *server, UA_ServerCallback callback, void *data);

void
UA_Server_workerCallback(UA_Server *server, UA_ServerCallback callback, void *data);

/*********************/
/* Utility Functions */
/*********************/

UA_StatusCode
UA_NumericRange_parseFromString(UA_NumericRange *range, const UA_String *str);

UA_UInt16 addNamespace(UA_Server *server, const UA_String name);

UA_Boolean
UA_Node_hasSubTypeOrInstances(const UA_Node *node);

const UA_VariableTypeNode *
getVariableNodeType(UA_Server *server, const UA_VariableNode *node);

const UA_ObjectTypeNode *
getObjectNodeType(UA_Server *server, const UA_ObjectNode *node);

/* Recursively searches "upwards" in the tree following specific reference types */
UA_Boolean
isNodeInTree(UA_NodeStore *ns, const UA_NodeId *leafNode,
             const UA_NodeId *nodeToFind, const UA_NodeId *referenceTypeIds,
             size_t referenceTypeIdsSize);

/* Returns an array with the hierarchy of type nodes. The returned array starts
 * at the leaf and continues "upwards" in the hierarchy based on the
 * ``hasSubType`` references. Since multiple-inheritance is possible in general,
 * duplicate entries are avoided. */
UA_StatusCode
getTypeHierarchy(UA_NodeStore *ns, const UA_NodeId *leafType,
                 UA_NodeId **typeHierarchy, size_t *typeHierarchySize);

/* Returns a pointer to the nodeid of the node type in the node's references. If
 * no type is defined, a pointer to UA_NODEID_NULL is returned */
const UA_NodeId * getNodeType(UA_Server *server, const UA_Node *node);

typedef void (*UA_ServiceOperation)(UA_Server *server, UA_Session *session,
                                    const void *requestOperation, void *responseOperation);

UA_StatusCode
UA_Server_processServiceOperations(UA_Server *server, UA_Session *session,
                                   UA_ServiceOperation operationCallback,
                                   const size_t *requestOperations,
                                   const UA_DataType *requestOperationsType,
                                   size_t *responseOperations,
                                   const UA_DataType *responseOperationsType);

/***************************************/
/* Check Information Model Consistency */
/***************************************/

UA_StatusCode
readValueAttribute(UA_Server *server, const UA_VariableNode *vn, UA_DataValue *v);

UA_StatusCode
typeCheckValue(UA_Server *server, const UA_NodeId *targetDataTypeId,
               UA_Int32 targetValueRank, size_t targetArrayDimensionsSize,
               const UA_UInt32 *targetArrayDimensions, const UA_Variant *value,
               const UA_NumericRange *range, UA_Variant *editableValue);

UA_StatusCode
compatibleArrayDimensions(size_t constraintArrayDimensionsSize,
                          const UA_UInt32 *constraintArrayDimensions,
                          size_t testArrayDimensionsSize,
                          const UA_UInt32 *testArrayDimensions);

UA_StatusCode
compatibleValueRankArrayDimensions(UA_Int32 valueRank, size_t arrayDimensionsSize);

UA_Boolean
compatibleDataType(UA_Server *server, const UA_NodeId *dataType,
                   const UA_NodeId *constraintDataType);

UA_StatusCode
writeValueRankAttribute(UA_Server *server, UA_VariableNode *node,
                        UA_Int32 valueRank, UA_Int32 constraintValueRank);

UA_StatusCode
compatibleValueRanks(UA_Int32 valueRank, UA_Int32 constraintValueRank);

/*******************/
/* Single-Services */
/*******************/

/* Some services take an array of "independent" requests. The single-services
 * are stored here to keep ua_services.h clean for documentation purposes. */

void Service_Browse_single(UA_Server *server, UA_Session *session,
                           struct ContinuationPointEntry *cp,
                           const UA_BrowseDescription *descr,
                           UA_UInt32 maxrefs, UA_BrowseResult *result);

UA_DataValue
UA_Server_readWithSession(UA_Server *server, UA_Session *session,
                          const UA_ReadValueId *item,
                          UA_TimestampsToReturn timestamps);

/* Checks if a registration timed out and removes that registration.
 * Should be called periodically in main loop */
void UA_Discovery_cleanupTimedOut(UA_Server *server, UA_DateTime nowMonotonic);

# ifdef UA_ENABLE_DISCOVERY_MULTICAST

UA_StatusCode
initMulticastDiscoveryServer(UA_Server* server);

void startMulticastDiscoveryServer(UA_Server *server);

void stopMulticastDiscoveryServer(UA_Server *server);

UA_StatusCode
iterateMulticastDiscoveryServer(UA_Server* server, UA_DateTime *nextRepeat,
                                UA_Boolean processIn);

void destroyMulticastDiscoveryServer(UA_Server* server);

typedef enum {
    UA_DISCOVERY_TCP,     /* OPC UA TCP mapping */
    UA_DISCOVERY_TLS     /* OPC UA HTTPS mapping */
} UA_DiscoveryProtocol;

/* Send a multicast probe to find any other OPC UA server on the network through mDNS. */
UA_StatusCode
UA_Discovery_multicastQuery(UA_Server* server);

UA_StatusCode
UA_Discovery_addRecord(UA_Server *server, const UA_String *servername,
                       const UA_String *hostname, UA_UInt16 port,
                       const UA_String *path, const UA_DiscoveryProtocol protocol,
                       UA_Boolean createTxt, const UA_String* capabilites,
                       size_t *capabilitiesSize);
UA_StatusCode
UA_Discovery_removeRecord(UA_Server *server, const UA_String *servername,
                          const UA_String *hostname, UA_UInt16 port,
                          UA_Boolean removeTxt);

# endif

/*****************************/
/* AddNodes Begin and Finish */
/*****************************/

/* Don't use this function. There are typed versions as inline functions. */
UA_StatusCode UA_EXPORT
__UA_Server_addNode_begin(UA_Server *server, const UA_NodeClass nodeClass,
                          const UA_NodeId *requestedNewNodeId,
                          const UA_QualifiedName *browseName,
                          const UA_NodeAttributes *attr,
                          const UA_DataType *attributeType,
                          UA_NodeId *outNewNodeId);

/* The inline function UA_Server_addNode_finish might be more convenient to
 * pass NodeIds in-situ (e.g. UA_NODEID_NUMERIC(0, 5)) */
UA_StatusCode UA_EXPORT
UA_Server_addNode_finish(UA_Server *server, const UA_NodeId nodeId,
                         const UA_NodeId parentNodeId,
                         const UA_NodeId referenceTypeId,
                         const UA_NodeId typeDefinition,
                         UA_InstantiationCallback *instantiationCallback);

static UA_INLINE UA_StatusCode
UA_Server_addReferenceTypeNode_begin(UA_Server *server, const UA_NodeId requestedNewNodeId,
                                     const UA_QualifiedName browseName,
                                     const UA_ReferenceTypeAttributes attr,
                                     UA_NodeId *outNewNodeId) {
    return __UA_Server_addNode_begin(server, UA_NODECLASS_REFERENCETYPE,
                                     &requestedNewNodeId,
                                     &browseName, (const UA_NodeAttributes*)&attr,
                                     &UA_TYPES[UA_TYPES_REFERENCETYPEATTRIBUTES],
                                     outNewNodeId);
}

static UA_INLINE UA_StatusCode
UA_Server_addDataTypeNode_begin(UA_Server *server,
                                const UA_NodeId requestedNewNodeId,
                                const UA_QualifiedName browseName,
                                const UA_DataTypeAttributes attr,
                                UA_NodeId *outNewNodeId) {
    return __UA_Server_addNode_begin(server, UA_NODECLASS_DATATYPE,
                                     &requestedNewNodeId,
                                     &browseName, (const UA_NodeAttributes*)&attr,
                                     &UA_TYPES[UA_TYPES_DATATYPEATTRIBUTES],
                                     outNewNodeId);
}

static UA_INLINE UA_StatusCode
UA_Server_addVariableNode_begin(UA_Server *server, const UA_NodeId requestedNewNodeId,
                                const UA_QualifiedName browseName,
                                const UA_VariableAttributes attr,
                                UA_NodeId *outNewNodeId) {
    return __UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE,
                                     &requestedNewNodeId, &browseName,
                                     (const UA_NodeAttributes*)&attr,
                                     &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
                                     outNewNodeId);
}

static UA_INLINE UA_StatusCode
UA_Server_addVariableTypeNode_begin(UA_Server *server, const UA_NodeId requestedNewNodeId,
                                    const UA_QualifiedName browseName,
                                    const UA_VariableTypeAttributes attr,
                                    UA_NodeId *outNewNodeId) {
    return __UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLETYPE,
                                     &requestedNewNodeId, &browseName,
                                     (const UA_NodeAttributes*)&attr,
                                     &UA_TYPES[UA_TYPES_VARIABLETYPEATTRIBUTES],
                                     outNewNodeId);
}

static UA_INLINE UA_StatusCode
UA_Server_addObjectNode_begin(UA_Server *server, const UA_NodeId requestedNewNodeId,
                              const UA_QualifiedName browseName,
                              const UA_ObjectAttributes attr,
                              UA_NodeId *outNewNodeId) {
    return __UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT,
                                     &requestedNewNodeId,
                                     &browseName, (const UA_NodeAttributes*)&attr,
                                     &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                                     outNewNodeId);
}

static UA_INLINE UA_StatusCode
UA_Server_addObjectTypeNode_begin(UA_Server *server,
                                  const UA_NodeId requestedNewNodeId,
                                  const UA_QualifiedName browseName,
                                  const UA_ObjectTypeAttributes attr,
                                  UA_NodeId *outNewNodeId) {
    return __UA_Server_addNode_begin(server, UA_NODECLASS_OBJECTTYPE,
                                     &requestedNewNodeId,
                                     &browseName, (const UA_NodeAttributes*)&attr,
                                     &UA_TYPES[UA_TYPES_OBJECTTYPEATTRIBUTES],
                                     outNewNodeId);
}

#ifdef UA_ENABLE_METHODCALLS
UA_StatusCode
UA_Server_addMethodNode_begin(UA_Server *server, const UA_NodeId requestedNewNodeId,
                              const UA_QualifiedName browseName,
                              const UA_MethodAttributes attr, UA_MethodCallback method,
                              void *handle, UA_NodeId *outNewNodeId);

UA_StatusCode
UA_Server_addMethodNode_finish(UA_Server *server, const UA_NodeId nodeId,
                               const UA_NodeId parentNodeId,
                               const UA_NodeId referenceTypeId,
                               size_t inputArgumentsSize,
                               const UA_Argument* inputArguments,
                               size_t outputArgumentsSize,
                               const UA_Argument* outputArguments);
#endif

/**********************/
/* Create Namespace 0 */
/**********************/

#ifndef UA_ENABLE_GENERATE_NAMESPACE0
void UA_Server_createNS0(UA_Server *server);
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_SERVER_INTERNAL_H_ */
