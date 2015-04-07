#ifndef UA_SERVER_INTERNAL_H_
#define UA_SERVER_INTERNAL_H_

#include "ua_util.h"
#include "ua_server.h"
#include "ua_session_manager.h"
#include "ua_securechannel_manager.h"
#include "ua_nodestore.h"

/** Mapping of namespace-id and url to an external nodestore. For namespaces
    that have no mapping defined, the internal nodestore is used by default. */
typedef struct UA_ExternalNamespace {
	UA_UInt16 index;
	UA_String url;
	UA_ExternalNodeStore externalNodeStore;
} UA_ExternalNamespace;

// forward declarations
struct UA_TimedWork;
typedef struct UA_TimedWork UA_TimedWork;

struct UA_DelayedWork;
typedef struct UA_DelayedWork UA_DelayedWork;

struct UA_Server {
    UA_ApplicationDescription description;
    UA_Int32 endpointDescriptionsSize;
    UA_EndpointDescription *endpointDescriptions;

    UA_ByteString serverCertificate;
    UA_SecureChannelManager secureChannelManager;
    UA_SessionManager sessionManager;
    UA_Logger logger;

    UA_NodeStore *nodestore;
    UA_Int32 externalNamespacesSize;
    UA_ExternalNamespace *externalNamespaces;

    UA_Int32 nlsSize;
    UA_ServerNetworkLayer *nls;

    UA_UInt32 random_seed;

#ifdef UA_MULTITHREADING
    UA_Boolean *running;
    UA_UInt16 nThreads;
    UA_UInt32 **workerCounters;
    UA_DelayedWork *delayedWork;

    // worker threads wait on the queue
	struct cds_wfcq_head dispatchQueue_head;
	struct cds_wfcq_tail dispatchQueue_tail;
    pthread_cond_t dispatchQueue_condition; // so the workers don't spin if the queue is empty
#endif

    LIST_HEAD(UA_TimedWorkList, UA_TimedWork) timedWork;

    UA_DateTime startTime;
    UA_DateTime buildDate;
};

void UA_Server_processBinaryMessage(UA_Server *server, UA_Connection *connection, UA_ByteString *msg);

UA_AddNodesResult UA_Server_addNodeWithSession(UA_Server *server, UA_Session *session, UA_Node *node,
                                               const UA_ExpandedNodeId *parentNodeId,
                                               const UA_NodeId *referenceTypeId);

UA_AddNodesResult UA_Server_addNode(UA_Server *server, UA_Node *node, const UA_ExpandedNodeId *parentNodeId,
                                    const UA_NodeId *referenceTypeId);

UA_StatusCode UA_Server_addReferenceWithSession(UA_Server *server, UA_Session *session, const UA_AddReferencesItem *item);

void UA_Server_deleteTimedWork(UA_Server *server);

#define ADDREFERENCE(NODEID, REFTYPE_NODEID, TARGET_EXPNODEID) do {     \
        UA_AddReferencesItem item;                                      \
        UA_AddReferencesItem_init(&item);                               \
        item.sourceNodeId = NODEID;                                     \
        item.referenceTypeId = REFTYPE_NODEID;                          \
        item.isForward = UA_TRUE;                                       \
        item.targetNodeId = TARGET_EXPNODEID;                           \
        UA_Server_addReference(server, &item);                          \
    } while(0)

#endif /* UA_SERVER_INTERNAL_H_ */
