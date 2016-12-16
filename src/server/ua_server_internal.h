#ifndef UA_SERVER_INTERNAL_H_
#define UA_SERVER_INTERNAL_H_

#include "ua_util.h"
#include "ua_server.h"
#include "ua_server_external_ns.h"
#include "ua_connection_internal.h"
#include "ua_session_manager.h"
#include "ua_securechannel_manager.h"
#include "ua_nodestore.h"

#define ANONYMOUS_POLICY "open62541-anonymous-policy"
#define USERNAME_POLICY "open62541-username-policy"

/* liburcu includes */
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
        rcu_read_lock(); } while(0)
# endif
#else
# define UA_RCU_LOCK()
# define UA_RCU_UNLOCK()
# define UA_ASSERT_RCU_LOCKED()
# define UA_ASSERT_RCU_UNLOCKED()
#endif

#ifdef UA_ENABLE_EXTERNAL_NAMESPACES
/** Mapping of namespace-id and url to an external nodestore. For namespaces
    that have no mapping defined, the internal nodestore is used by default. */
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
#endif

    size_t namespacesSize;
    UA_String *namespaces;

#ifdef UA_ENABLE_EXTERNAL_NAMESPACES
    size_t externalNamespacesSize;
    UA_ExternalNamespace *externalNamespaces;
#endif

    /* Jobs with a repetition interval */
    LIST_HEAD(RepeatedJobsList, RepeatedJob) repeatedJobs;

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

void UA_Server_processBinaryMessage(UA_Server *server, UA_Connection *connection, const UA_ByteString *msg);

UA_StatusCode UA_Server_delayedCallback(UA_Server *server, UA_ServerCallback callback, void *data);
UA_StatusCode UA_Server_delayedFree(UA_Server *server, void *data);
void UA_Server_deleteAllRepeatedJobs(UA_Server *server);

/* Add an existing node. The node is assumed to be "finished", i.e. no
 * instantiation from inheritance is necessary. Instantiationcallback and
 * addedNodeId may be NULL. */
UA_StatusCode
Service_AddNodes_existing(UA_Server *server, UA_Session *session, UA_Node *node,
                          const UA_NodeId *parentNodeId,
                          const UA_NodeId *referenceTypeId,
                          const UA_NodeId *typeDefinition,
                          UA_InstantiationCallback *instantiationCallback,
                          UA_NodeId *addedNodeId);

/*********************/
/* Utility Functions */
/*********************/

UA_StatusCode
parse_numericrange(const UA_String *str, UA_NumericRange *range);

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

UA_Boolean
isNodeInTree(UA_NodeStore *ns, const UA_NodeId *rootNode,
             const UA_NodeId *nodeToFind, const UA_NodeId *referenceTypeIds,
             size_t referenceTypeIdsSize);

const UA_Node *
getNodeType(UA_Server *server, const UA_Node *node);

/***************************************/
/* Check Information Model Consistency */
/***************************************/

UA_StatusCode
readValueAttribute(UA_Server *server, const UA_VariableNode *vn, UA_DataValue *v);

UA_StatusCode
typeCheckValue(UA_Server *server, const UA_NodeId *variableDataTypeId,
               UA_Int32 variableValueRank, size_t variableArrayDimensionsSize,
               const UA_UInt32 *variableArrayDimensions, const UA_Variant *value,
               const UA_NumericRange *range, UA_Variant *equivalent);

UA_StatusCode
writeDataTypeAttribute(UA_Server *server, UA_VariableNode *node,
                       const UA_NodeId *dataType, const UA_NodeId *constraintDataType);

UA_StatusCode
compatibleArrayDimensions(size_t constraintArrayDimensionsSize,
                          const UA_UInt32 *constraintArrayDimensions,
                          size_t testArrayDimensionsSize,
                          const UA_UInt32 *testArrayDimensions);

UA_StatusCode
writeValueRankAttribute(UA_Server *server, UA_VariableNode *node, UA_Int32 valueRank,
                        UA_Int32 constraintValueRank);

UA_StatusCode
writeValueAttribute(UA_Server *server, UA_VariableNode *node,
                    const UA_DataValue *value, const UA_String *indexRange);

/*******************/
/* Single-Services */
/*******************/

/* Some services take an array of "independent" requests. The single-services
   are stored here to keep ua_services.h clean for documentation purposes. */

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
void UA_Discovery_cleanupTimedOut(UA_Server *server, UA_DateTime now);

#endif /* UA_SERVER_INTERNAL_H_ */
