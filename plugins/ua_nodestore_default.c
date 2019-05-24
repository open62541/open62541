/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. 
 *
 *    Copyright 2014-2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Julian Grothoff
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#include <open62541/plugin/nodestore.h>
#include "ziptree.h"

#ifndef UA_ENABLE_CUSTOM_NODESTORE

#ifdef UA_ENABLE_MULTITHREADING
#include <pthread.h>
#define BEGIN_CRITSECT(NODEMAP) pthread_mutex_lock(&(NODEMAP)->mutex)
#define END_CRITSECT(NODEMAP) pthread_mutex_unlock(&(NODEMAP)->mutex)
#else
#define BEGIN_CRITSECT(NODEMAP) do {} while(0)
#define END_CRITSECT(NODEMAP) do {} while(0)
#endif

/* container_of */
#define container_of(ptr, type, member) \
    (type *)((uintptr_t)ptr - offsetof(type,member))

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

    if(UA_NodeId_equal(&aa->nodeId, &bb->nodeId))
        return ZIP_CMP_EQ;

    /* Compare namespaceIndex */
    if(aa->nodeId.namespaceIndex < bb->nodeId.namespaceIndex)
        return ZIP_CMP_LESS;
    if(aa->nodeId.namespaceIndex > bb->nodeId.namespaceIndex)
        return ZIP_CMP_MORE;

    /* Compare identifierType */
    if(aa->nodeId.identifierType < bb->nodeId.identifierType)
        return ZIP_CMP_LESS;
    if(aa->nodeId.identifierType > bb->nodeId.identifierType)
        return ZIP_CMP_MORE;

    /* Compare the identifier */
    switch(aa->nodeId.identifierType) {
    case UA_NODEIDTYPE_NUMERIC:
        if(aa->nodeId.identifier.numeric < bb->nodeId.identifier.numeric)
            return ZIP_CMP_LESS;
        if(aa->nodeId.identifier.numeric > bb->nodeId.identifier.numeric)
            return ZIP_CMP_MORE;
        break;
    case UA_NODEIDTYPE_GUID:
        if(aa->nodeId.identifier.guid.data1 < bb->nodeId.identifier.guid.data1 ||
           aa->nodeId.identifier.guid.data2 < bb->nodeId.identifier.guid.data2 ||
           aa->nodeId.identifier.guid.data3 < bb->nodeId.identifier.guid.data3 ||
           strncmp((const char*)aa->nodeId.identifier.guid.data4,
                   (const char*)bb->nodeId.identifier.guid.data4, 8) < 0)
            return ZIP_CMP_LESS;
        if(aa->nodeId.identifier.guid.data1 > bb->nodeId.identifier.guid.data1 ||
           aa->nodeId.identifier.guid.data2 > bb->nodeId.identifier.guid.data2 ||
           aa->nodeId.identifier.guid.data3 > bb->nodeId.identifier.guid.data3 ||
           strncmp((const char*)aa->nodeId.identifier.guid.data4,
                   (const char*)bb->nodeId.identifier.guid.data4, 8) > 0)
            return ZIP_CMP_MORE;
        break;
    case UA_NODEIDTYPE_STRING:
    case UA_NODEIDTYPE_BYTESTRING: {
        if(aa->nodeId.identifier.string.length < bb->nodeId.identifier.string.length)
            return ZIP_CMP_LESS;
        if(aa->nodeId.identifier.string.length > bb->nodeId.identifier.string.length)
            return ZIP_CMP_MORE;
        int cmp = strncmp((const char*)aa->nodeId.identifier.string.data,
                          (const char*)bb->nodeId.identifier.string.data,
                          aa->nodeId.identifier.string.length);
        if(cmp < 0)
            return ZIP_CMP_LESS;
        if(cmp > 0)
            return ZIP_CMP_MORE;
        break;
    }
    default:
        break;
    }

    return ZIP_CMP_EQ;
}

ZIP_HEAD(NodeTree, NodeEntry);
typedef struct NodeTree NodeTree;

typedef struct {
    NodeTree root;
#ifdef UA_ENABLE_MULTITHREADING
    pthread_mutex_t mutex; /* Protect access */
#endif
} NodeMap;

ZIP_PROTTYPE(NodeTree, NodeEntry, NodeEntry)
ZIP_IMPL(NodeTree, NodeEntry, zipfields, NodeEntry, zipfields, cmpNodeId)

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
    node->nodeClass = nodeClass;
    return entry;
}

static void
deleteEntry(NodeEntry *entry) {
    UA_Node_deleteMembers((UA_Node*)&entry->nodeId);
    UA_free(entry);
}

static void
cleanupEntry(NodeEntry *entry) {
    if(entry->deleted && entry->refCount == 0)
        deleteEntry(entry);
}

/***********************/
/* Interface functions */
/***********************/

/* Not yet inserted into the NodeMap */
UA_Node *
UA_Nodestore_newNode(void *nsCtx, UA_NodeClass nodeClass) {
    NodeEntry *entry = newEntry(nodeClass);
    if(!entry)
        return NULL;
    return (UA_Node*)&entry->nodeId;
}

/* Not yet inserted into the NodeMap */
void
UA_Nodestore_deleteNode(void *nsCtx, UA_Node *node) {
    deleteEntry(container_of(node, NodeEntry, nodeId));
}

const UA_Node *
UA_Nodestore_getNode(void *nsCtx, const UA_NodeId *nodeId) {
    NodeMap *ns = (NodeMap*)nsCtx;
    BEGIN_CRITSECT(ns);
    NodeEntry dummy;
    dummy.nodeIdHash = UA_NodeId_hash(nodeId);
    dummy.nodeId = *nodeId;
    NodeEntry *entry = ZIP_FIND(NodeTree, &ns->root, &dummy);
    if(!entry) {
        END_CRITSECT(ns);
        return NULL;
    }
    ++entry->refCount;
    END_CRITSECT(ns);
    return (const UA_Node*)&entry->nodeId;
}

void
UA_Nodestore_releaseNode(void *nsCtx, const UA_Node *node) {
    if(!node)
        return;
#ifdef UA_ENABLE_MULTITHREADING
    NodeMap *ns = (NodeMap*)nsCtx;
#endif
    BEGIN_CRITSECT(ns);
    NodeEntry *entry = container_of(node, NodeEntry, nodeId);
    UA_assert(entry->refCount > 0);
    --entry->refCount;
    cleanupEntry(entry);
    END_CRITSECT(ns);
}

UA_StatusCode
UA_Nodestore_getNodeCopy(void *nsCtx, const UA_NodeId *nodeId,
                         UA_Node **outNode) {
    /* Find the node */
    const UA_Node *node = UA_Nodestore_getNode(nsCtx, nodeId);
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    /* Create the new entry */
    NodeEntry *ne = newEntry(node->nodeClass);
    if(!ne) {
        UA_Nodestore_releaseNode(nsCtx, node);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Copy the node content */
    UA_Node *nnode = (UA_Node*)&ne->nodeId;
    UA_StatusCode retval = UA_Node_copy(node, nnode);
    UA_Nodestore_releaseNode(nsCtx, node);
    if(retval != UA_STATUSCODE_GOOD) {
        deleteEntry(ne);
        return retval;
    }

    ne->orig = container_of(node, NodeEntry, nodeId);
    *outNode = nnode;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Nodestore_insertNode(void *nsCtx, UA_Node *node, UA_NodeId *addedNodeId) {
    NodeEntry *entry = container_of(node, NodeEntry, nodeId);
    NodeMap *ns = (NodeMap*)nsCtx;
    BEGIN_CRITSECT(ns);

    /* Ensure that the NodeId is unique */
    NodeEntry dummy;
    dummy.nodeId = node->nodeId;
    if(node->nodeId.identifierType == UA_NODEIDTYPE_NUMERIC &&
       node->nodeId.identifier.numeric == 0) {
        do { /* Create a random nodeid until we find an unoccupied id */
            node->nodeId.identifier.numeric = UA_UInt32_random();
            dummy.nodeId.identifier.numeric = node->nodeId.identifier.numeric;
            dummy.nodeIdHash = UA_NodeId_hash(&node->nodeId);
        } while(ZIP_FIND(NodeTree, &ns->root, &dummy));
    } else {
        dummy.nodeIdHash = UA_NodeId_hash(&node->nodeId);
        if(ZIP_FIND(NodeTree, &ns->root, &dummy)) { /* The nodeid exists */
            deleteEntry(entry);
            END_CRITSECT(ns);
            return UA_STATUSCODE_BADNODEIDEXISTS;
        }
    }

    /* Copy the NodeId */
    if(addedNodeId) {
        UA_StatusCode retval = UA_NodeId_copy(&node->nodeId, addedNodeId);
        if(retval != UA_STATUSCODE_GOOD) {
            deleteEntry(entry);
            END_CRITSECT(ns);
            return retval;
        }
    }

    /* Insert the node */
    entry->nodeIdHash = dummy.nodeIdHash;
    ZIP_INSERT(NodeTree, &ns->root, entry, ZIP_FFS32(UA_UInt32_random()));
    END_CRITSECT(ns);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Nodestore_replaceNode(void *nsCtx, UA_Node *node) {
    /* Find the node */
    const UA_Node *oldNode = UA_Nodestore_getNode(nsCtx, &node->nodeId);
    if(!oldNode)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    /* Test if the copy is current */
    NodeEntry *entry = container_of(node, NodeEntry, nodeId);
    NodeEntry *oldEntry = container_of(oldNode, NodeEntry, nodeId);
    if(oldEntry != entry->orig) {
        /* The node was already updated since the copy was made */
        deleteEntry(entry);
        UA_Nodestore_releaseNode(nsCtx, oldNode);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Replace */
    NodeMap *ns = (NodeMap*)nsCtx;
    BEGIN_CRITSECT(ns);
    ZIP_REMOVE(NodeTree, &ns->root, oldEntry);
    entry->nodeIdHash = oldEntry->nodeIdHash;
    ZIP_INSERT(NodeTree, &ns->root, entry, ZIP_RANK(entry, zipfields));
    oldEntry->deleted = true;
    END_CRITSECT(ns);

    UA_Nodestore_releaseNode(nsCtx, oldNode);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Nodestore_removeNode(void *nsCtx, const UA_NodeId *nodeId) {
    NodeMap *ns = (NodeMap*)nsCtx;
    BEGIN_CRITSECT(ns);
    NodeEntry dummy;
    dummy.nodeIdHash = UA_NodeId_hash(nodeId);
    dummy.nodeId = *nodeId;
    NodeEntry *entry = ZIP_FIND(NodeTree, &ns->root, &dummy);
    if(!entry) {
        END_CRITSECT(ns);
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }
    ZIP_REMOVE(NodeTree, &ns->root, entry);
    entry->deleted = true;
    cleanupEntry(entry);
    END_CRITSECT(ns);
    return UA_STATUSCODE_GOOD;
}

struct VisitorData {
    UA_NodestoreVisitor visitor;
    void *visitorContext;
};

static void
nodeVisitor(NodeEntry *entry, void *data) {
    struct VisitorData *d = (struct VisitorData*)data;
    d->visitor(d->visitorContext, (UA_Node*)&entry->nodeId);
}

void
UA_Nodestore_iterate(void *nsCtx, UA_NodestoreVisitor visitor,
                     void *visitorCtx) {
    struct VisitorData d;
    d.visitor = visitor;
    d.visitorContext = visitorCtx;
    NodeMap *ns = (NodeMap*)nsCtx;
    BEGIN_CRITSECT(ns);
    ZIP_ITER(NodeTree, &ns->root, nodeVisitor, &d);
    END_CRITSECT(ns);
}

static void
deleteNodeVisitor(NodeEntry *entry, void *data) {
    deleteEntry(entry);
}

/***********************/
/* Nodestore Lifecycle */
/***********************/

const UA_Boolean inPlaceEditAllowed = true;

UA_StatusCode
UA_Nodestore_new(void **nsCtx) {
    /* Allocate and initialize the nodemap */
    NodeMap *nodemap = (NodeMap*)UA_malloc(sizeof(NodeMap));
    if(!nodemap)
        return UA_STATUSCODE_BADOUTOFMEMORY;
#ifdef UA_ENABLE_MULTITHREADING
    pthread_mutex_init(&nodemap->mutex, NULL);
#endif

    ZIP_INIT(&nodemap->root);

    /* Populate the nodestore */
    *nsCtx = (void*)nodemap;
    return UA_STATUSCODE_GOOD;
}

void
UA_Nodestore_delete(void *nsCtx) {
    if (!nsCtx)
        return;

    NodeMap *ns = (NodeMap*)nsCtx;
#ifdef UA_ENABLE_MULTITHREADING
    pthread_mutex_destroy(&ns->mutex);
#endif
    ZIP_ITER(NodeTree, &ns->root, deleteNodeVisitor, NULL);
    UA_free(ns);
}

#endif /* UA_ENABLE_CUSTOM_NODESTORE */
