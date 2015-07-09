#ifndef UA_SERVER_INTERNAL_H_
#define UA_SERVER_INTERNAL_H_

#include "ua_util.h"
#include "ua_server.h"
#include "ua_session_manager.h"
#include "ua_securechannel_manager.h"
#include "ua_nodestore.h"

#ifdef ENABLE_SUBSCRIPTIONS
#include "ua_subscription_manager.h"
#endif

#define PRODUCT_URI "http://open62541.org"
#define ANONYMOUS_POLICY "open62541-anonymous-policy"
#define USERNAME_POLICY "open62541-username-policy"

/** Mapping of namespace-id and url to an external nodestore. For namespaces
    that have no mapping defined, the internal nodestore is used by default. */
typedef struct UA_ExternalNamespace {
	UA_UInt16 index;
	UA_String url;
	UA_ExternalNodeStore externalNodeStore;
} UA_ExternalNamespace;

struct UA_Server {
    /* Config */
    UA_ServerConfig config;
    UA_Logger logger;
    UA_UInt32 random_seed;

    /* Meta */
    UA_DateTime startTime;
    UA_DateTime buildDate;
    UA_ApplicationDescription description;
    UA_Int32 endpointDescriptionsSize;
    UA_EndpointDescription *endpointDescriptions;

    /* Communication */
    size_t networkLayersSize;
    UA_ServerNetworkLayer *networkLayers;

    /* Security */
    UA_ByteString serverCertificate;
    UA_SecureChannelManager secureChannelManager;
    UA_SessionManager sessionManager;

    /* Address Space */
    UA_NodeStore *nodestore;
    size_t namespacesSize;
    UA_String *namespaces;
    size_t externalNamespacesSize;
    UA_ExternalNamespace *externalNamespaces;
     
    /* Jobs with a repetition interval */
    LIST_HEAD(RepeatedJobsList, RepeatedJobs) repeatedJobs;
    
#ifdef UA_MULTITHREADING
    /* Dispatch queue head for the worker threads (the tail should not be in the same cache line) */
	struct cds_wfcq_head dispatchQueue_head;

    UA_Boolean *running;
    UA_UInt16 nThreads;
    UA_UInt32 **workerCounters;
    pthread_t *thr;

    struct cds_lfs_stack mainLoopJobs; /* Work that shall be executed only in the main loop and not
                                          by worker threads */
    struct DelayedJobs *delayedJobs;

    pthread_cond_t dispatchQueue_condition; /* so the workers don't spin if the queue is empty */
    /* Dispatch queue tail for the worker threads */
	struct cds_wfcq_tail dispatchQueue_tail;
#endif
};

void UA_Server_processBinaryMessage(UA_Server *server, UA_Connection *connection, UA_ByteString *msg);

UA_AddNodesResult UA_Server_addNodeWithSession(UA_Server *server, UA_Session *session, UA_Node *node,
                                               const UA_ExpandedNodeId parentNodeId,
                                               const UA_NodeId referenceTypeId);

UA_AddNodesResult UA_Server_addNode(UA_Server *server, UA_Node *node, const UA_ExpandedNodeId parentNodeId,
                                    const UA_NodeId referenceTypeId);

UA_StatusCode UA_Server_addReferenceWithSession(UA_Server *server, UA_Session *session,
                                                const UA_AddReferencesItem *item);

UA_StatusCode addOneWayReferenceWithSession(UA_Server *server, UA_Session *session, 
                                                   const UA_AddReferencesItem *item);

UA_StatusCode UA_Server_addDelayedJob(UA_Server *server, UA_Job job);

void UA_Server_deleteAllRepeatedJobs(UA_Server *server);

#endif /* UA_SERVER_INTERNAL_H_ */
