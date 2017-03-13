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
#include "ua_server_external_ns.h"
#include "ua_connection_internal.h"
#include "ua_session_manager.h"
#include "ua_securechannel_manager.h"
#include "ua_nodestore.h"

#ifdef UA_ENABLE_DISCOVERY_MULTICAST
#include "mdnsd/libmdnsd/mdnsd.h"
#endif

#define ANONYMOUS_POLICY "open62541-anonymous-policy"
#define USERNAME_POLICY "open62541-username-policy"

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
#   define UA_ASSERT_RCU_LOCKED() assert(rcu_locked)
#   define UA_ASSERT_RCU_UNLOCKED() assert(!rcu_locked)
#   define UA_RCU_LOCK() do {                     \
        UA_ASSERT_RCU_UNLOCKED();                 \
        rcu_locked = true;                        \
        rcu_read_lock(); } while(0)
#   define UA_RCU_UNLOCK() do {                   \
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

#ifdef UA_ENABLE_EXTERNAL_NAMESPACES
/* Mapping of namespace-id and url to an external nodestore. For namespaces that
 * have no mapping defined, the internal nodestore is used by default. */
typedef struct UA_ExternalNamespace {
    UA_UInt16 index;
    UA_String url;
    UA_ExternalNodeStore externalNodeStore;
} UA_ExternalNamespace;
#endif

#ifdef UA_ENABLE_MULTITHREADING
typedef struct {
    UA_Server *server;
    pthread_t thr;
    UA_UInt32 counter;
    volatile UA_Boolean running;
    char padding[64 - sizeof(void*) - sizeof(pthread_t) -
                 sizeof(UA_UInt32) - sizeof(UA_Boolean)]; // separate cache lines
} UA_Worker;

struct MainLoopJob {
    struct cds_lfs_node node;
    UA_Job job;
};

void
UA_Server_dispatchJob(UA_Server *server, const UA_Job *job);

#endif

void
UA_Server_processJob(UA_Server *server, UA_Job *job);

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
#endif

#endif

struct UA_Server {
    /* Meta */
    UA_DateTime startTime;
    size_t endpointDescriptionsSize;
    UA_EndpointDescription *endpointDescriptions;

    /* Security */
    UA_SecureChannelManager secureChannelManager;
    UA_SessionManager sessionManager;

    /* Address Space */
    UA_NodeStore *nodestore;

#ifdef UA_ENABLE_DISCOVERY
    /* Discovery */
    LIST_HEAD(registeredServer_list, registeredServer_list_entry) registeredServers; // doubly-linked list of registered servers
    size_t registeredServersSize;
    struct PeriodicServerRegisterJob *periodicServerRegisterJob;
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

#ifdef UA_ENABLE_EXTERNAL_NAMESPACES
    size_t externalNamespacesSize;
    UA_ExternalNamespace *externalNamespaces;
#endif

    /* Jobs with a repetition interval */
    UA_RepeatedJobsList repeatedJobs;

#ifndef UA_ENABLE_MULTITHREADING
    SLIST_HEAD(DelayedJobsList, UA_DelayedJob) delayedCallbacks;
#else
    /* Dispatch queue head for the worker threads (the tail should not be in the same cache line) */
    struct cds_wfcq_head dispatchQueue_head;
    UA_Worker *workers; /* there are nThread workers in a running server */
    struct cds_lfs_stack mainLoopJobs; /* Work that shall be executed only in the main loop and not
                                          by worker threads */
    struct DelayedJobs *delayedJobs;
    pthread_cond_t dispatchQueue_condition; /* so the workers don't spin if the queue is empty */
    pthread_mutex_t dispatchQueue_mutex; /* mutex for access to condition variable */
    struct cds_wfcq_tail dispatchQueue_tail; /* Dispatch queue tail for the worker threads */
#endif

    /* Config is the last element so that MSVC allows the usernamePasswordLogins
       field with zero-sized array */
    UA_ServerConfig config;
};

/*****************/
/* Node Handling */
/*****************/

void UA_Node_deleteMembersAnyNodeClass(UA_Node *node);
UA_StatusCode UA_Node_copyAnyNodeClass(const UA_Node *src, UA_Node *dst);

typedef UA_StatusCode (*UA_EditNodeCallback)(UA_Server*, UA_Session*, UA_Node*, const void*);

/* Calls callback on the node. In the multithreaded case, the node is copied before and replaced in
   the nodestore. */
UA_StatusCode UA_Server_editNode(UA_Server *server, UA_Session *session, const UA_NodeId *nodeId,
                                 UA_EditNodeCallback callback, const void *data);

void UA_Server_processBinaryMessage(UA_Server *server, UA_Connection *connection,
                                    const UA_ByteString *message);

UA_StatusCode UA_Server_delayedCallback(UA_Server *server, UA_ServerCallback callback, void *data);
UA_StatusCode UA_Server_delayedFree(UA_Server *server, void *data);

/*********************/
/* Utility Functions */
/*********************/

UA_StatusCode
parse_numericrange(const UA_String *str, UA_NumericRange *range);

UA_UInt16 addNamespace(UA_Server *server, const UA_String name);

UA_Boolean
UA_Node_hasSubTypeOrInstances(const UA_Node *node);

const UA_VariableTypeNode *
getVariableNodeType(UA_Server *server, const UA_VariableNode *node);

const UA_ObjectTypeNode *
getObjectNodeType(UA_Server *server, const UA_ObjectNode *node);

/* Returns an array with all subtype nodeids (including the root). Subtypes need
 * to have the same nodeClass as root and are (recursively) related with a
 * hasSubType reference. Since multi-inheritance is possible, we test for
 * duplicates and return evey nodeid at most once. */
UA_StatusCode
getTypeHierarchy(UA_NodeStore *ns, const UA_Node *rootRef, UA_Boolean inverse,
                 UA_NodeId **typeHierarchy, size_t *typeHierarchySize);

/* Recursively searches "upwards" in the tree following specific reference types */
UA_Boolean
isNodeInTree(UA_NodeStore *ns, const UA_NodeId *leafNode,
             const UA_NodeId *nodeToFind, const UA_NodeId *referenceTypeIds,
             size_t referenceTypeIdsSize);

void
getNodeType(UA_Server *server, const UA_Node *node, UA_NodeId *typeId);

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
UA_Boolean
compatibleDataType(UA_Server *server, const UA_NodeId *dataType,
                   const UA_NodeId *constraintDataType);

UA_StatusCode
compatibleValueRankArrayDimensions(UA_Int32 valueRank, size_t arrayDimensionsSize);

UA_Boolean
compatibleDataType(UA_Server *server, const UA_NodeId *dataType,
                   const UA_NodeId *constraintDataType);

UA_StatusCode
compatibleValueRankArrayDimensions(UA_Int32 valueRank, size_t arrayDimensionsSize);

UA_StatusCode
writeValueRankAttribute(UA_Server *server, UA_VariableNode *node,
                        UA_Int32 valueRank, UA_Int32 constraintValueRank);

UA_StatusCode
compatibleValueRanks(UA_Int32 valueRank, UA_Int32 constraintValueRank);

UA_StatusCode
compatibleValueRanks(UA_Int32 valueRank, UA_Int32 constraintValueRank);

/*******************/
/* Single-Services */
/*******************/

/* Some services take an array of "independent" requests. The single-services
 * are stored here to keep ua_services.h clean for documentation purposes. */

UA_StatusCode
Service_AddReferences_single(UA_Server *server, UA_Session *session,
                             const UA_AddReferencesItem *item);

UA_StatusCode
Service_DeleteNodes_single(UA_Server *server, UA_Session *session,
                           const UA_NodeId *nodeId, UA_Boolean deleteReferences);

UA_StatusCode
Service_DeleteReferences_single(UA_Server *server, UA_Session *session,
                                const UA_DeleteReferencesItem *item);

void Service_Browse_single(UA_Server *server, UA_Session *session,
                           struct ContinuationPointEntry *cp,
                           const UA_BrowseDescription *descr,
                           UA_UInt32 maxrefs, UA_BrowseResult *result);

void
Service_TranslateBrowsePathsToNodeIds_single(UA_Server *server, UA_Session *session,
                                             const UA_BrowsePath *path,
                                             UA_BrowsePathResult *result);

void Service_Read_single(UA_Server *server, UA_Session *session,
                         UA_TimestampsToReturn timestamps,
                         const UA_ReadValueId *id, UA_DataValue *v);

void Service_Call_single(UA_Server *server, UA_Session *session,
                         const UA_CallMethodRequest *request,
                         UA_CallMethodResult *result);

/* Periodic task to clean up the discovery registry */
void UA_Discovery_cleanupTimedOut(UA_Server *server, UA_DateTime nowMonotonic);

# ifdef UA_ENABLE_DISCOVERY_MULTICAST

UA_StatusCode UA_Discovery_multicastInit(UA_Server* server);
void UA_Discovery_multicastDestroy(UA_Server* server);

typedef enum {
    UA_DISCOVERY_TCP,     /* OPC UA TCP mapping */
    UA_DISCOVERY_TLS     /* OPC UA HTTPS mapping */
} UA_DiscoveryProtocol;

UA_StatusCode
UA_Discovery_multicastQuery(UA_Server* server);

UA_StatusCode
UA_Discovery_addRecord(UA_Server* server, const char* servername, const char* hostname,
                       unsigned short port, const char* path,
                       const UA_DiscoveryProtocol protocol, UA_Boolean createTxt,
                       const UA_String* capabilites, const size_t *capabilitiesSize);
UA_StatusCode
UA_Discovery_removeRecord(UA_Server* server, const char* servername, const char* hostname,
                          unsigned short port, UA_Boolean removeTxt);

#  ifdef UA_ENABLE_MULTITHREADING
UA_StatusCode UA_Discovery_multicastListenStart(UA_Server* server);
UA_StatusCode UA_Discovery_multicastListenStop(UA_Server* server);
#  endif
UA_StatusCode UA_Discovery_multicastIterate(UA_Server* server, UA_DateTime *nextRepeat, UA_Boolean processIn);

# endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_SERVER_INTERNAL_H_ */
