/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014, 2017 (c) Florian Palm
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2015 (c) Chris Iatrou
 *    Copyright 2015-2016 (c) Oleksiy Vasylyev
 *    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Julian Grothoff
 */

#ifndef UA_SERVER_INTERNAL_H_
#define UA_SERVER_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_util_internal.h"
#include "ua_server.h"
#include "ua_server_config.h"
#include "ua_timer.h"
#include "ua_connection_internal.h"
#include "ua_session_manager.h"
#include "ua_securechannel_manager.h"

#ifdef UA_ENABLE_PUBSUB
#include "ua_pubsub_manager.h"
#endif

#ifdef UA_ENABLE_SUBSCRIPTIONS
#include "ua_subscription.h"

typedef struct {
    UA_MonitoredItem monitoredItem;
    void *context;
    union {
        UA_Server_DataChangeNotificationCallback dataChangeCallback;
        /* UA_Server_EventNotificationCallback eventCallback; */
    } callback;
} UA_LocalMonitoredItem;

#endif

#ifdef UA_ENABLE_MULTITHREADING

#include <pthread.h>

struct UA_Worker;
typedef struct UA_Worker UA_Worker;

struct UA_WorkerCallback;
typedef struct UA_WorkerCallback UA_WorkerCallback;

SIMPLEQ_HEAD(UA_DispatchQueue, UA_WorkerCallback);
typedef struct UA_DispatchQueue UA_DispatchQueue;

#endif /* UA_ENABLE_MULTITHREADING */

#ifdef UA_ENABLE_DISCOVERY

typedef struct registeredServer_list_entry {
    LIST_ENTRY(registeredServer_list_entry) pointers;
    UA_RegisteredServer registeredServer;
    UA_DateTime lastSeen;
} registeredServer_list_entry;

typedef struct periodicServerRegisterCallback_entry {
    LIST_ENTRY(periodicServerRegisterCallback_entry) pointers;
    struct PeriodicServerRegisterCallback *callback;
} periodicServerRegisterCallback_entry;

#ifdef UA_ENABLE_DISCOVERY_MULTICAST

#include "mdnsd/libmdnsd/mdnsd.h"

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

#ifdef UA_ENABLE_DISCOVERY
    /* Discovery */
    LIST_HEAD(registeredServer_list, registeredServer_list_entry) registeredServers; // doubly-linked list of registered servers
    size_t registeredServersSize;
    LIST_HEAD(periodicServerRegisterCallback_list, periodicServerRegisterCallback_entry) periodicServerRegisterCallbacks; // doubly-linked list of current register callbacks
    UA_Server_registerServerCallback registerServerCallback;
    void* registerServerCallbackData;
# ifdef UA_ENABLE_DISCOVERY_MULTICAST
    mdns_daemon_t *mdnsDaemon;
    UA_SOCKET mdnsSocket;
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

    /* Namespaces */
    size_t namespacesSize;
    UA_String *namespaces;

    /* Callbacks with a repetition interval */
    UA_Timer timer;

    /* Delayed callbacks */
    SLIST_HEAD(DelayedCallbacksList, UA_DelayedCallback) delayedCallbacks;

    /* Worker threads */
#ifdef UA_ENABLE_MULTITHREADING
    UA_Worker *workers; /* there are nThread workers in a running server */
    UA_DispatchQueue dispatchQueue; /* Dispatch queue for the worker threads */
    pthread_mutex_t dispatchQueue_accessMutex; /* mutex for access to queue */
    pthread_cond_t dispatchQueue_condition; /* so the workers don't spin if the queue is empty */
    pthread_mutex_t dispatchQueue_conditionMutex; /* mutex for access to condition variable */
#endif

    /* For bootstrapping, omit some consistency checks, creating a reference to
     * the parent and member instantiation */
    UA_Boolean bootstrapNS0;

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* To be cast to UA_LocalMonitoredItem to get the callback and context */
    LIST_HEAD(LocalMonitoredItems, UA_MonitoredItem) localMonitoredItems;
    UA_UInt32 lastLocalMonitoredItemId;
#endif

#ifdef UA_ENABLE_PUBSUB
    /* Publish/Subscribe toplevel container */
    UA_PubSubManager pubSubManager;
#endif

    /* Config */
    UA_ServerConfig config;
};

/*****************/
/* Node Handling */
/*****************/

#define UA_Nodestore_get(SERVER, NODEID)                                \
    (SERVER)->config.nodestore.getNode((SERVER)->config.nodestore.context, NODEID)

#define UA_Nodestore_release(SERVER, NODEID)                            \
    (SERVER)->config.nodestore.releaseNode((SERVER)->config.nodestore.context, NODEID)

#define UA_Nodestore_new(SERVER, NODECLASS)                               \
    (SERVER)->config.nodestore.newNode((SERVER)->config.nodestore.context, NODECLASS)

#define UA_Nodestore_getCopy(SERVER, NODEID, OUTNODE)                   \
    (SERVER)->config.nodestore.getNodeCopy((SERVER)->config.nodestore.context, NODEID, OUTNODE)

#define UA_Nodestore_insert(SERVER, NODE, OUTNODEID)                    \
    (SERVER)->config.nodestore.insertNode((SERVER)->config.nodestore.context, NODE, OUTNODEID)

#define UA_Nodestore_delete(SERVER, NODE)                               \
    (SERVER)->config.nodestore.deleteNode((SERVER)->config.nodestore.context, NODE)

#define UA_Nodestore_remove(SERVER, NODEID)                             \
    (SERVER)->config.nodestore.removeNode((SERVER)->config.nodestore.context, NODEID)

/* Calls the callback with the node retrieved from the nodestore on top of the
 * stack. Either a copy or the original node for in-situ editing. Depends on
 * multithreading and the nodestore.*/
typedef UA_StatusCode (*UA_EditNodeCallback)(UA_Server*, UA_Session*,
                                             UA_Node *node, void*);
UA_StatusCode UA_Server_editNode(UA_Server *server, UA_Session *session,
                                 const UA_NodeId *nodeId,
                                 UA_EditNodeCallback callback,
                                 void *data);

/*************/
/* Callbacks */
/*************/

/* Delayed callbacks are executed when all previously dispatched callbacks are
 * finished */
UA_StatusCode
UA_Server_delayedCallback(UA_Server *server, UA_ServerCallback callback, void *data);

UA_StatusCode
UA_Server_delayedFree(UA_Server *server, void *data);

#ifndef UA_ENABLE_MULTITHREADING
/* Execute all delayed callbacks regardless of whether the worker threads have
 * finished previous work */
void UA_Server_cleanupDelayedCallbacks(UA_Server *server);
#else
void UA_Server_cleanupDispatchQueue(UA_Server *server);
#endif

/* Callback is executed in the same thread or, if possible, dispatched to one of
 * the worker threads. */
void
UA_Server_workerCallback(UA_Server *server, UA_ServerCallback callback, void *data);

/*********************/
/* Utility Functions */
/*********************/

/* A few global NodeId definitions */
extern const UA_NodeId subtypeId;
extern const UA_NodeId hierarchicalReferences;

UA_UInt16 addNamespace(UA_Server *server, const UA_String name);

UA_Boolean
UA_Node_hasSubTypeOrInstances(const UA_Node *node);

/* Recursively searches "upwards" in the tree following specific reference types */
UA_Boolean
isNodeInTree(UA_Nodestore *ns, const UA_NodeId *leafNode,
             const UA_NodeId *nodeToFind, const UA_NodeId *referenceTypeIds,
             size_t referenceTypeIdsSize);

/* Returns an array with the hierarchy of type nodes. The returned array starts
 * at the leaf and continues "upwards" in the hierarchy based on the
 * ``hasSubType`` references. Since multiple-inheritance is possible in general,
 * duplicate entries are removed. */
UA_StatusCode
getTypeHierarchy(UA_Nodestore *ns, const UA_NodeId *leafType,
                 UA_NodeId **typeHierarchy, size_t *typeHierarchySize);

/* Returns the type node from the node on the stack top. The type node is pushed
 * on the stack and returned. */
const UA_Node * getNodeType(UA_Server *server, const UA_Node *node);

/* Write a node attribute with a defined session */
UA_StatusCode
UA_Server_writeWithSession(UA_Server *server, UA_Session *session,
                           const UA_WriteValue *value);


/* Many services come as an array of operations. This function generalizes the
 * processing of the operations. */
typedef void (*UA_ServiceOperation)(UA_Server *server, UA_Session *session,
                                    void *context,
                                    const void *requestOperation,
                                    void *responseOperation);

UA_StatusCode
UA_Server_processServiceOperations(UA_Server *server, UA_Session *session,
                                   UA_ServiceOperation operationCallback,
                                   void *context,
                                   const size_t *requestOperations,
                                   const UA_DataType *requestOperationsType,
                                   size_t *responseOperations,
                                   const UA_DataType *responseOperationsType)
    UA_FUNC_ATTR_WARN_UNUSED_RESULT;

/***************************************/
/* Check Information Model Consistency */
/***************************************/

UA_StatusCode
readValueAttribute(UA_Server *server, UA_Session *session,
                   const UA_VariableNode *vn, UA_DataValue *v);

/* Test whether the value matches a variable definition given by
 * - datatype
 * - valueranke
 * - array dimensions.
 * Sometimes it can be necessary to transform the content of the value, e.g.
 * byte array to bytestring or uint32 to some enum. If editableValue is non-NULL,
 * we try to create a matching variant that points to the original data. */
UA_Boolean
compatibleValue(UA_Server *server, UA_Session *session, const UA_NodeId *targetDataTypeId,
                UA_Int32 targetValueRank, size_t targetArrayDimensionsSize,
                const UA_UInt32 *targetArrayDimensions, const UA_Variant *value,
                const UA_NumericRange *range);

UA_Boolean
compatibleArrayDimensions(size_t constraintArrayDimensionsSize,
                          const UA_UInt32 *constraintArrayDimensions,
                          size_t testArrayDimensionsSize,
                          const UA_UInt32 *testArrayDimensions);

UA_Boolean
compatibleValueArrayDimensions(const UA_Variant *value, size_t targetArrayDimensionsSize,
                               const UA_UInt32 *targetArrayDimensions);

UA_Boolean
compatibleValueRankArrayDimensions(UA_Server *server, UA_Session *session,
                                   UA_Int32 valueRank, size_t arrayDimensionsSize);

UA_Boolean
compatibleDataType(UA_Server *server, const UA_NodeId *dataType,
                   const UA_NodeId *constraintDataType, UA_Boolean isValue);

UA_Boolean
compatibleValueRanks(UA_Int32 valueRank, UA_Int32 constraintValueRank);

void
Operation_Browse(UA_Server *server, UA_Session *session, UA_UInt32 *maxrefs,
                 const UA_BrowseDescription *descr, UA_BrowseResult *result);

UA_DataValue
UA_Server_readWithSession(UA_Server *server, UA_Session *session,
                          const UA_ReadValueId *item,
                          UA_TimestampsToReturn timestampsToReturn);

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

/* Creates a new node in the nodestore. */
UA_StatusCode
AddNode_raw(UA_Server *server, UA_Session *session, void *nodeContext,
            const UA_AddNodesItem *item, UA_NodeId *outNewNodeId);

/* Check the reference to the parent node; Add references. */
UA_StatusCode
AddNode_addRefs(UA_Server *server, UA_Session *session, const UA_NodeId *nodeId,
                const UA_NodeId *parentNodeId, const UA_NodeId *referenceTypeId,
                const UA_NodeId *typeDefinitionId);

/* Type-check type-definition; Run the constructors */
UA_StatusCode
AddNode_finish(UA_Server *server, UA_Session *session, const UA_NodeId *nodeId);

/**********************/
/* Create Namespace 0 */
/**********************/

UA_StatusCode UA_Server_initNS0(UA_Server *server);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_SERVER_INTERNAL_H_ */
