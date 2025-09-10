#include <open62541/plugin/nodestore.h>
#include <open62541/plugin/nodestore_default.h>
#include <open62541/server.h>

#include "adhoc_nodestore.h"

typedef struct {
    UA_Nodestore ns;
    UA_Nodestore *fallback;
} AdHocNodestore;

/* Always create new nodes from the fallback. The backend nodes are already there. */
static UA_Node *
AdHocNodestore_newNode(UA_Nodestore *orig_ns, UA_NodeClass nodeClass) {
    AdHocNodestore *ns = (AdHocNodestore*)orig_ns;
    return ns->fallback->newNode(ns->fallback, nodeClass);
}

static void
AdHocNodestore_deleteNode(UA_Nodestore *orig_ns, UA_Node *node) {
    AdHocNodestore *ns = (AdHocNodestore*)orig_ns;
    if(!isBackendNode(&node->head.nodeId))
        ns->fallback->deleteNode(ns->fallback, node);
}

static const UA_Node *
AdHocNodestore_getNode(UA_Nodestore *orig_ns, const UA_NodeId *nodeid,
                       UA_UInt32 attributeMask, UA_ReferenceTypeSet references,
                       UA_BrowseDirection referenceDirections) {
    AdHocNodestore *ns = (AdHocNodestore*)orig_ns;
    if(!isBackendNode(nodeid))
        return ns->fallback->getNode(ns->fallback, nodeid, attributeMask,
                                     references, referenceDirections);

    /* Create ad-hoc node */
    UA_Node *node = (UA_Node*)UA_calloc(1, sizeof(UA_Node));
    if(!node)
        return NULL;

    UA_NodeId_copy(nodeid, &node->head.nodeId);

    collectNodeAttributes(node);
    collectParentInfo(node);
    collectChildsInfo(node);

    return node;
}

/***********************/
/* Interface functions */
/***********************/

static const UA_Node *
AdHocNodestore_getNodeFromPtr(UA_Nodestore *ns, UA_NodePointer ptr,
                              UA_UInt32 attributeMask,
                              UA_ReferenceTypeSet references,
                              UA_BrowseDirection referenceDirections) {
    if(!UA_NodePointer_isLocal(ptr))
        return NULL;
    UA_NodeId id = UA_NodePointer_toNodeId(ptr);
    return AdHocNodestore_getNode(ns, &id, attributeMask, references, referenceDirections);
}

static void
AdHocNodestore_releaseNode(UA_Nodestore *orig_ns, const UA_Node *node) {
    if(!node)
        return;

    AdHocNodestore *ns = (AdHocNodestore*)orig_ns;
    if(!isBackendNode(&node->head.nodeId)) {
        ns->fallback->releaseNode(ns->fallback, node);
        return;
    }

    UA_Node *mutNode = (UA_Node*)(uintptr_t)node;
    UA_Node_clear(mutNode);
    UA_free(mutNode);
}

static UA_StatusCode
AdHocNodestore_getNodeCopy(UA_Nodestore *orig_ns, const UA_NodeId *nodeId,
                           UA_Node **outNode) {
    if(isBackendNode(nodeId))
        return UA_STATUSCODE_BADNOTWRITABLE;
    AdHocNodestore *ns = (AdHocNodestore*)orig_ns;
    return ns->fallback->getNodeCopy(ns->fallback, nodeId, outNode);
}

static UA_StatusCode
AdHocNodestore_removeNode(UA_Nodestore *orig_ns, const UA_NodeId *nodeid) {
    AdHocNodestore *ns = (AdHocNodestore*)orig_ns;
    if(!isBackendNode(nodeid))
        return ns->fallback->removeNode(ns->fallback, nodeid);
    return UA_STATUSCODE_GOOD;
}

/*
 * If this function fails in any way, the node parameter is deleted here,
 * so the caller function does not need to take care of it anymore
 */
static UA_StatusCode
AdHocNodestore_insertNode(UA_Nodestore *orig_ns, UA_Node *node, UA_NodeId *addedNodeId) {
    /* The node was originally created from the fallback nodestore */
    AdHocNodestore *ns = (AdHocNodestore*)orig_ns;
    if(!isBackendNode(&node->head.nodeId))
        return ns->fallback->insertNode(ns->fallback, node, addedNodeId);
    UA_Node_clear(node);
    UA_free(node);
    return UA_STATUSCODE_BADALREADYEXISTS;
}

static UA_StatusCode
AdHocNodestore_replaceNode(UA_Nodestore *orig_ns, UA_Node *node) {
    AdHocNodestore *ns = (AdHocNodestore*)orig_ns;
    if(!isBackendNode(&node->head.nodeId))
        return ns->fallback->replaceNode(ns->fallback, node);
    UA_Node_clear(node);
    UA_free(node);
    return UA_STATUSCODE_BADINTERNALERROR;
}

static const UA_NodeId *
AdHocNodestore_getReferenceTypeId(UA_Nodestore *orig_ns, UA_Byte refTypeIndex) {
    AdHocNodestore *ns = (AdHocNodestore*)orig_ns;
    return ns->fallback->getReferenceTypeId(ns->fallback, refTypeIndex);
}

static void
AdHocNodestore_iterate(UA_Nodestore *orig_ns, UA_NodestoreVisitor visitor,
                       void *visitorContext) {
    AdHocNodestore *ns = (AdHocNodestore*)orig_ns;
    ns->fallback->iterate(ns->fallback, visitor, visitorContext);
}

static void
AdHocNodestore_free(UA_Nodestore *ns) {
    AdHocNodestore *ans = (AdHocNodestore*)ns;
    ans->fallback->free(ans->fallback);
    UA_free(ans);
}

UA_Nodestore *
UA_Nodestore_PilzAdHoc(void) {
    /* Allocate and initialize the nodemap */
    AdHocNodestore *ns = (AdHocNodestore*)UA_malloc(sizeof(AdHocNodestore));
    if(!ns)
        return NULL;

    ns->fallback = UA_Nodestore_ZipTree();
    if(!ns->fallback) {
        UA_free(ns);
        return NULL;
    }

    /* Populate the nodestore */
    ns->ns.free = AdHocNodestore_free;
    ns->ns.newNode = AdHocNodestore_newNode;
    ns->ns.deleteNode = AdHocNodestore_deleteNode;
    ns->ns.getNode = AdHocNodestore_getNode;
    ns->ns.getNodeFromPtr = AdHocNodestore_getNodeFromPtr;
    ns->ns.releaseNode = AdHocNodestore_releaseNode;
    ns->ns.getNodeCopy = AdHocNodestore_getNodeCopy;
    ns->ns.insertNode = AdHocNodestore_insertNode;
    ns->ns.replaceNode = AdHocNodestore_replaceNode;
    ns->ns.removeNode = AdHocNodestore_removeNode;
    ns->ns.getReferenceTypeId = AdHocNodestore_getReferenceTypeId;
    ns->ns.iterate = AdHocNodestore_iterate;

    /* All nodes are stored in RAM. Changes are made in-situ. GetEditNode is
     * identical to GetNode -- but the Node pointer is non-const. */
    ns->ns.getEditNode =
        (UA_Node * (*)(UA_Nodestore *ns, const UA_NodeId *nodeId,
                       UA_UInt32 attributeMask,
                       UA_ReferenceTypeSet references,
                       UA_BrowseDirection referenceDirections))AdHocNodestore_getNode;
    ns->ns.getEditNodeFromPtr =
        (UA_Node * (*)(UA_Nodestore *ns, UA_NodePointer ptr,
                       UA_UInt32 attributeMask,
                       UA_ReferenceTypeSet references,
                       UA_BrowseDirection referenceDirections))AdHocNodestore_getNodeFromPtr;

    return &ns->ns;
}
