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

    size_t namespacesSize;
    UA_String *namespaces;

#ifdef UA_ENABLE_EXTERNAL_NAMESPACES
    size_t externalNamespacesSize;
    UA_ExternalNamespace *externalNamespaces;
#endif

    /* Jobs with a repetition interval */
    LIST_HEAD(RepeatedJobsList, RepeatedJob) repeatedJobs;

#ifdef UA_ENABLE_MULTITHREADING
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

    /* Disable some consistency checks when adding nodes */
    UA_Boolean bootstrapInformationModel;
};

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

UA_StatusCode
getTypeHierarchy(UA_NodeStore *ns, const UA_NodeId *root,
                 UA_NodeId **reftypes, size_t *reftypes_count);

UA_StatusCode
isNodeInTree(UA_NodeStore *ns, const UA_NodeId *rootNode,
             const UA_NodeId *nodeToFind, const UA_NodeId *referenceTypeIds,
             size_t referenceTypeIdsSize, UA_Boolean *found);

/***************************************/
/* Check Information Model Consistency */
/***************************************/

UA_StatusCode
UA_VariableNode_setArrayDimensions(UA_Server *server, UA_VariableNode *node,
                                   const UA_VariableTypeNode *vt,
                                   size_t arrayDimensionsSize, UA_UInt32 *arrayDimensions);

UA_StatusCode
UA_VariableNode_setValueRank(UA_Server *server, UA_VariableNode *node,
                             const UA_VariableTypeNode *vt,
                             const UA_Int32 valueRank);

UA_StatusCode
UA_VariableNode_setDataType(UA_Server *server, UA_VariableNode *node,
                            const UA_VariableTypeNode *vt,
                            const UA_NodeId *dataType);

UA_StatusCode
UA_VariableNode_setValue(UA_Server *server, UA_VariableNode *node,
                         const UA_DataValue *value, const UA_String *indexRange);

UA_StatusCode
UA_Variant_matchVariableDefinition(UA_Server *server, const UA_NodeId *variableDataTypeId,
                                   UA_Int32 variableValueRank, size_t variableArrayDimensionsSize,
                                   const UA_UInt32 *variableArrayDimensions, const UA_Variant *value,
                                   const UA_NumericRange *range, UA_Variant *equivalent);

#endif /* UA_SERVER_INTERNAL_H_ */
