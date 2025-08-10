/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2014-2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Julian Grothoff
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#include <open62541/types.h>
#include <open62541/plugin/nodestore_default.h>
#include "ziptree.h"
#include "pcg_basic.h"

#ifndef container_of
#define container_of(ptr, type, member) \
    (type *)((uintptr_t)ptr - offsetof(type,member))
#endif

struct NodeEntry;
typedef struct NodeEntry NodeEntry;

struct NodeEntry {
    ZIP_ENTRY(NodeEntry) zipfields;
    UA_UInt32 nodeIdHash;
    UA_UInt16 refCount; /* How many consumers have a reference to the node? */
    UA_Boolean deleted; /* Node was marked as deleted and can be deleted when refCount == 0 */
    NodeEntry *orig;    /* If a copy is made to replace a node, track that we
                         * replace only the node from which the copy was made.
                         * Important for concurrent operations. */
    UA_NodeId nodeId; /* This is actually a UA_Node that also starts with a NodeId */
};

/* Absolute ordering for NodeIds */
static enum ZIP_CMP
cmpNodeId(const void *a, const void *b) {
    const NodeEntry *aa = (const NodeEntry*)a;
    const NodeEntry *bb = (const NodeEntry*)b;

    /* Compare hash */
    if(aa->nodeIdHash < bb->nodeIdHash)
        return ZIP_CMP_LESS;
    if(aa->nodeIdHash > bb->nodeIdHash)
        return ZIP_CMP_MORE;

    /* Compore nodes in detail */
    return (enum ZIP_CMP)UA_NodeId_order(&aa->nodeId, &bb->nodeId);
}

ZIP_HEAD(NodeTree, NodeEntry);
typedef struct NodeTree NodeTree;

typedef struct {
    UA_Nodestore ns;

    NodeTree root;
    size_t size;

    /* Maps ReferenceTypeIndex to the NodeId of the ReferenceType */
    UA_NodeId referenceTypeIds[UA_REFERENCETYPESET_MAX];
    UA_Byte referenceTypeCounter;
} ZipNodestore;

ZIP_FUNCTIONS(NodeTree, NodeEntry, zipfields, NodeEntry, zipfields, cmpNodeId)

static NodeEntry *
newEntry(UA_NodeClass nodeClass) {
    size_t size = sizeof(NodeEntry) - sizeof(UA_NodeId);
    switch(nodeClass) {
    case UA_NODECLASS_OBJECT:
        size += sizeof(UA_ObjectNode);
        break;
    case UA_NODECLASS_VARIABLE:
        size += sizeof(UA_VariableNode);
        break;
    case UA_NODECLASS_METHOD:
        size += sizeof(UA_MethodNode);
        break;
    case UA_NODECLASS_OBJECTTYPE:
        size += sizeof(UA_ObjectTypeNode);
        break;
    case UA_NODECLASS_VARIABLETYPE:
        size += sizeof(UA_VariableTypeNode);
        break;
    case UA_NODECLASS_REFERENCETYPE:
        size += sizeof(UA_ReferenceTypeNode);
        break;
    case UA_NODECLASS_DATATYPE:
        size += sizeof(UA_DataTypeNode);
        break;
    case UA_NODECLASS_VIEW:
        size += sizeof(UA_ViewNode);
        break;
    default:
        return NULL;
    }
    NodeEntry *entry = (NodeEntry*)UA_calloc(1, size);
    if(!entry)
        return NULL;
    UA_Node *node = (UA_Node*)&entry->nodeId;
    node->head.nodeClass = nodeClass;
    return entry;
}

static void
deleteEntry(NodeEntry *entry) {
    UA_Node_clear((UA_Node*)&entry->nodeId);
    UA_free(entry);
}

static void
cleanupEntry(NodeEntry *entry) {
    if(entry->refCount > 0)
        return;
    if(entry->deleted) {
        deleteEntry(entry);
        return;
    }
    UA_NodeHead *head = (UA_NodeHead*)&entry->nodeId;
    for(size_t i = 0; i < head->referencesSize; i++) {
        UA_NodeReferenceKind *rk = &head->references[i];
        if(rk->targetsSize > 16 && !rk->hasRefTree)
            UA_NodeReferenceKind_switch(rk);
    }
}

/***********************/
/* Interface functions */
/***********************/

/* Not yet inserted into the ZipContext */
static UA_Node *
zipNsNewNode(UA_Nodestore *_, UA_NodeClass nodeClass) {
    NodeEntry *entry = newEntry(nodeClass);
    if(!entry)
        return NULL;
    return (UA_Node*)&entry->nodeId;
}

/* Not yet inserted into the ZipContext */
static void
zipNsDeleteNode(UA_Nodestore *_, UA_Node *node) {
    deleteEntry(container_of(node, NodeEntry, nodeId));
}

static const UA_Node *
zipNsGetNode(UA_Nodestore *ns, const UA_NodeId *nodeId,
             UA_UInt32 attributeMask,
             UA_ReferenceTypeSet references,
             UA_BrowseDirection referenceDirections) {
    NodeEntry dummy;
    dummy.nodeIdHash = UA_NodeId_hash(nodeId);
    dummy.nodeId = *nodeId;
    ZipNodestore *zns = (ZipNodestore*)ns;
    NodeEntry *entry = ZIP_FIND(NodeTree, &zns->root, &dummy);
    if(!entry)
        return NULL;
    ++entry->refCount;
    return (const UA_Node*)&entry->nodeId;
}

static const UA_Node *
zipNsGetNodeFromPtr(UA_Nodestore *ns, UA_NodePointer ptr,
                    UA_UInt32 attributeMask,
                    UA_ReferenceTypeSet references,
                    UA_BrowseDirection referenceDirections) {
    if(!UA_NodePointer_isLocal(ptr))
        return NULL;
    UA_NodeId id = UA_NodePointer_toNodeId(ptr);
    return zipNsGetNode(ns, &id, attributeMask,
                        references, referenceDirections);
}

static void
zipNsReleaseNode(UA_Nodestore *_, const UA_Node *node) {
    if(!node)
        return;
    NodeEntry *entry = container_of(node, NodeEntry, nodeId);
    UA_assert(entry->refCount > 0);
    --entry->refCount;
    cleanupEntry(entry);
}

static UA_StatusCode
zipNsGetNodeCopy(UA_Nodestore *ns, const UA_NodeId *nodeId,
                 UA_Node **outNode) {
    /* Get the node (with all attributes and references, the mask and refs are
       currently noy evaluated within the plugin.) */
    const UA_Node *node =
        zipNsGetNode(ns, nodeId, UA_NODEATTRIBUTESMASK_ALL,
                     UA_REFERENCETYPESET_ALL, UA_BROWSEDIRECTION_BOTH);
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    /* Create the new entry */
    NodeEntry *ne = newEntry(node->head.nodeClass);
    if(!ne) {
        zipNsReleaseNode(ns, node);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Copy the node content */
    UA_Node *nnode = (UA_Node*)&ne->nodeId;
    UA_StatusCode retval = UA_Node_copy(node, nnode);
    zipNsReleaseNode(NULL, node);
    if(retval != UA_STATUSCODE_GOOD) {
        deleteEntry(ne);
        return retval;
    }

    ne->orig = container_of(node, NodeEntry, nodeId);
    *outNode = nnode;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
zipNsInsertNode(UA_Nodestore *ns, UA_Node *node, UA_NodeId *addedNodeId) {
    NodeEntry *entry = container_of(node, NodeEntry, nodeId);
    ZipNodestore *zns = (ZipNodestore*)ns;

    /* Ensure that the NodeId is unique by testing their presence. If the NodeId
     * is ns=xx;i=0, then the numeric identifier is replaced with a random
     * unused int32. It is ensured that the created identifiers are stable after
     * a server restart (assuming that Nodes are created in the same order and
     * with the same BrowseName). */
    NodeEntry dummy;
    memset(&dummy, 0, sizeof(NodeEntry));
    dummy.nodeId = node->head.nodeId;
    if(node->head.nodeId.identifierType == UA_NODEIDTYPE_NUMERIC &&
       node->head.nodeId.identifier.numeric == 0) {
        NodeEntry *found;
        UA_UInt32 mask = 0x2F;
        pcg32_random_t rng;
        pcg32_srandom_r(&rng, zns->size, 0);
        do {
            /* Generate a random NodeId. Favor "easy" NodeIds.
             * Always above 50000. */
            UA_UInt32 numId = (pcg32_random_r(&rng) & mask) + 50000;

#if SIZE_MAX <= UA_UINT32_MAX
            /* The compressed "immediate" representation of nodes does not
             * support the full range on 32bit systems. Generate smaller
             * identifiers as they can be stored more compactly. */
            if(numId >= (0x01 << 24))
                numId = numId % (0x01 << 24);
#endif
            node->head.nodeId.identifier.numeric = numId;

            /* Look up the current NodeId */
            dummy.nodeId.identifier.numeric = numId;
            dummy.nodeIdHash = UA_NodeId_hash(&node->head.nodeId);
            found = ZIP_FIND(NodeTree, &zns->root, &dummy);

            if(found) {
                /* Reseed the rng using the browseName of the existing node.
                 * This ensures that different information models end up with
                 * different NodeId sequences, but still stable after a
                 * restart. */
                UA_NodeHead *nh = (UA_NodeHead*)&found->nodeId;
                pcg32_srandom_r(&rng, rng.state, UA_QualifiedName_hash(&nh->browseName));

                /* Make the mask less strict when the NodeId already exists */
                mask = (mask << 1) | 0x01;
            }
        } while(found);
    } else {
        dummy.nodeIdHash = UA_NodeId_hash(&node->head.nodeId);
        if(ZIP_FIND(NodeTree, &zns->root, &dummy)) { /* The nodeid exists */
            deleteEntry(entry);
            return UA_STATUSCODE_BADNODEIDEXISTS;
        }
    }

    /* Copy the NodeId */
    if(addedNodeId) {
        UA_StatusCode retval = UA_NodeId_copy(&node->head.nodeId, addedNodeId);
        if(retval != UA_STATUSCODE_GOOD) {
            deleteEntry(entry);
            return retval;
        }
    }

    /* For new ReferencetypeNodes add to the index map */
    if(node->head.nodeClass == UA_NODECLASS_REFERENCETYPE) {
        UA_ReferenceTypeNode *refNode = &node->referenceTypeNode;
        if(zns->referenceTypeCounter >= UA_REFERENCETYPESET_MAX) {
            deleteEntry(entry);
            return UA_STATUSCODE_BADINTERNALERROR;
        }

        UA_StatusCode retval =
            UA_NodeId_copy(&node->head.nodeId,
                           &zns->referenceTypeIds[zns->referenceTypeCounter]);
        if(retval != UA_STATUSCODE_GOOD) {
            deleteEntry(entry);
            return UA_STATUSCODE_BADINTERNALERROR;
        }

        /* Assign the ReferenceTypeIndex to the new ReferenceTypeNode */
        refNode->referenceTypeIndex = zns->referenceTypeCounter;
        refNode->subTypes = UA_REFTYPESET(zns->referenceTypeCounter);
        zns->referenceTypeCounter++;
    }

    /* Insert the node */
    entry->nodeIdHash = dummy.nodeIdHash;
    ZIP_INSERT(NodeTree, &zns->root, entry);
    zns->size++;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
zipNsReplaceNode(UA_Nodestore *ns, UA_Node *node) {
    /* Find the node (the mask and refs are not evaluated yet by the plugin)*/
    const UA_Node *oldNode =
        zipNsGetNode(ns, &node->head.nodeId, UA_NODEATTRIBUTESMASK_ALL,
                     UA_REFERENCETYPESET_ALL, UA_BROWSEDIRECTION_BOTH);
    if(!oldNode) {
        deleteEntry(container_of(node, NodeEntry, nodeId));
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    /* Test if the copy is current */
    NodeEntry *entry = container_of(node, NodeEntry, nodeId);
    NodeEntry *oldEntry = container_of(oldNode, NodeEntry, nodeId);
    if(oldEntry != entry->orig) {
        /* The node was already updated since the copy was made */
        deleteEntry(entry);
        zipNsReleaseNode(NULL, oldNode);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Replace */
    ZipNodestore *zns = (ZipNodestore*)ns;
    ZIP_REMOVE(NodeTree, &zns->root, oldEntry);
    entry->nodeIdHash = oldEntry->nodeIdHash;
    ZIP_INSERT(NodeTree, &zns->root, entry);
    oldEntry->deleted = true;

    zipNsReleaseNode(NULL, oldNode);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
zipNsRemoveNode(UA_Nodestore *ns, const UA_NodeId *nodeId) {
    ZipNodestore *zns = (ZipNodestore*)ns;
    NodeEntry dummy;
    dummy.nodeIdHash = UA_NodeId_hash(nodeId);
    dummy.nodeId = *nodeId;
    NodeEntry *entry = ZIP_FIND(NodeTree, &zns->root, &dummy);
    if(!entry)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    ZIP_REMOVE(NodeTree, &zns->root, entry);
    zns->size--;
    entry->deleted = true;
    cleanupEntry(entry);
    return UA_STATUSCODE_GOOD;
}

static const UA_NodeId *
zipNsGetReferenceTypeId(UA_Nodestore *ns, UA_Byte refTypeIndex) {
    ZipNodestore *zns = (ZipNodestore*)ns;
    if(refTypeIndex >= zns->referenceTypeCounter)
        return NULL;
    return &zns->referenceTypeIds[refTypeIndex];
}

struct VisitorData {
    UA_NodestoreVisitor visitor;
    void *visitorContext;
};

static void *
nodeVisitor(void *data, NodeEntry *entry) {
    struct VisitorData *d = (struct VisitorData*)data;
    d->visitor(d->visitorContext, (UA_Node*)&entry->nodeId);
    return NULL;
}

static void
zipNsIterate(UA_Nodestore *ns, UA_NodestoreVisitor visitor,
             void *visitorCtx) {
    struct VisitorData d;
    d.visitor = visitor;
    d.visitorContext = visitorCtx;
    ZipNodestore *zns = (ZipNodestore*)ns;
    ZIP_ITER(NodeTree, &zns->root, nodeVisitor, &d);
}

static void *
deleteNodeVisitor(void *data, NodeEntry *entry) {
    deleteEntry(entry);
    return NULL;
}

/***********************/
/* Nodestore Lifecycle */
/***********************/

static void
zipNsFree(UA_Nodestore *ns) {
    ZipNodestore *zns = (ZipNodestore*)ns;
    ZIP_ITER(NodeTree, &zns->root, deleteNodeVisitor, NULL);

    /* Clean up the ReferenceTypes index array */
    for(size_t i = 0; i < zns->referenceTypeCounter; i++)
        UA_NodeId_clear(&zns->referenceTypeIds[i]);

    UA_free(zns);
}

UA_Nodestore *
UA_Nodestore_ZipTree(void) {
    /* Allocate and initialize the context */
    ZipNodestore *zns = (ZipNodestore*)UA_calloc(1, sizeof(ZipNodestore));
    if(!zns)
        return NULL;

    ZIP_INIT(&zns->root);
    zns->referenceTypeCounter = 0;

    /* Populate the nodestore */
    zns->ns.free = zipNsFree;
    zns->ns.newNode = zipNsNewNode;
    zns->ns.deleteNode = zipNsDeleteNode;
    zns->ns.getNode = zipNsGetNode;
    zns->ns.getNodeFromPtr = zipNsGetNodeFromPtr;
    zns->ns.releaseNode = zipNsReleaseNode;
    zns->ns.getNodeCopy = zipNsGetNodeCopy;
    zns->ns.insertNode = zipNsInsertNode;
    zns->ns.replaceNode = zipNsReplaceNode;
    zns->ns.removeNode = zipNsRemoveNode;
    zns->ns.getReferenceTypeId = zipNsGetReferenceTypeId;
    zns->ns.iterate = zipNsIterate;

    /* All nodes are stored in RAM. Changes are made in-situ. GetEditNode is
     * identical to GetNode -- but the Node pointer is non-const. */
    zns->ns.getEditNode =
        (UA_Node * (*)(UA_Nodestore *ns, const UA_NodeId *nodeId,
                       UA_UInt32 attributeMask,
                       UA_ReferenceTypeSet references,
                       UA_BrowseDirection referenceDirections))zipNsGetNode;
    zns->ns.getEditNodeFromPtr =
        (UA_Node * (*)(UA_Nodestore *ns, UA_NodePointer ptr,
                       UA_UInt32 attributeMask,
                       UA_ReferenceTypeSet references,
                       UA_BrowseDirection referenceDirections))zipNsGetNodeFromPtr;

    return &zns->ns;
}
