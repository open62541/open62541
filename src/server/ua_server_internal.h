#ifndef UA_SERVER_INTERNAL_H_
#define UA_SERVER_INTERNAL_H_

#include "ua_server.h"
#include "ua_nodestore.h"
#include "ua_session_manager.h"
#include "ua_securechannel_manager.h"

/** Mapping of namespace-id and url to an external nodestore. For namespaces
    that have no mapping defined, the internal nodestore is used by default. */
typedef struct UA_ExternalNamespace {
	UA_UInt16 index;
	UA_String url;
	UA_ExternalNodeStore externalNodeStore;
} UA_ExternalNamespace;

void UA_ExternalNamespace_init(UA_ExternalNamespace *ens);
void UA_ExternalNamespace_deleteMembers(UA_ExternalNamespace *ens);

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
};

UA_AddNodesResult
UA_Server_addNodeWithSession(UA_Server *server, UA_Session *session, const UA_Node **node,
                             const UA_ExpandedNodeId *parentNodeId, const UA_NodeId *referenceTypeId);

UA_StatusCode
UA_Server_addReferenceWithSession(UA_Server *server, UA_Session *session, const UA_AddReferencesItem *item);

#endif /* UA_SERVER_INTERNAL_H_ */
