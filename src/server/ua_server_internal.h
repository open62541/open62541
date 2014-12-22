#ifndef UA_SERVER_INTERNAL_H_
#define UA_SERVER_INTERNAL_H_

#include "ua_config.h"

#ifdef UA_MULTITHREADING
#define _LGPL_SOURCE
#include <urcu.h>
#include <urcu/wfcqueue.h>
#endif

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
    UA_NetworkLayer *nls;

    UA_UInt32 random_seed;

#ifdef UA_MULTITHREADING
    UA_Boolean *running;
    UA_UInt16 nThreads;
    UA_UInt32 **workerCounters;
    UA_DelayedWork *delayedWork;

    // worker threads wait on the queue
	struct cds_wfcq_head dispatchQueue_head;
	struct cds_wfcq_tail dispatchQueue_tail;
#endif

    LIST_HEAD(UA_TimedWorkList, UA_TimedWork) timedWork;
};

void UA_Server_processBinaryMessage(UA_Server *server, UA_Connection *connection, const UA_ByteString *msg);

UA_AddNodesResult UA_Server_addNodeWithSession(UA_Server *server, UA_Session *session, const UA_Node **node,
                                               const UA_ExpandedNodeId *parentNodeId, const UA_NodeId *referenceTypeId);

UA_StatusCode UA_Server_addReferenceWithSession(UA_Server *server, UA_Session *session, const UA_AddReferencesItem *item);

void UA_Server_deleteTimedWork(UA_Server *server);

/** The (nodes) AttributeIds are defined in part 6, table A1 of the standard */
typedef enum {
    UA_ATTRIBUTEID_NODEID                  = 1,
    UA_ATTRIBUTEID_NODECLASS               = 2,
    UA_ATTRIBUTEID_BROWSENAME              = 3,
    UA_ATTRIBUTEID_DISPLAYNAME             = 4,
    UA_ATTRIBUTEID_DESCRIPTION             = 5,
    UA_ATTRIBUTEID_WRITEMASK               = 6,
    UA_ATTRIBUTEID_USERWRITEMASK           = 7,
    UA_ATTRIBUTEID_ISABSTRACT              = 8,
    UA_ATTRIBUTEID_SYMMETRIC               = 9,
    UA_ATTRIBUTEID_INVERSENAME             = 10,
    UA_ATTRIBUTEID_CONTAINSNOLOOPS         = 11,
    UA_ATTRIBUTEID_EVENTNOTIFIER           = 12,
    UA_ATTRIBUTEID_VALUE                   = 13,
    UA_ATTRIBUTEID_DATATYPE                = 14,
    UA_ATTRIBUTEID_VALUERANK               = 15,
    UA_ATTRIBUTEID_ARRAYDIMENSIONS         = 16,
    UA_ATTRIBUTEID_ACCESSLEVEL             = 17,
    UA_ATTRIBUTEID_USERACCESSLEVEL         = 18,
    UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL = 19,
    UA_ATTRIBUTEID_HISTORIZING             = 20,
    UA_ATTRIBUTEID_EXECUTABLE              = 21,
    UA_ATTRIBUTEID_USEREXECUTABLE          = 22
} UA_AttributeId;

#endif /* UA_SERVER_INTERNAL_H_ */
