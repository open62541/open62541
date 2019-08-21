/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2019 (c) Julian Grothoff, Torben Deppe, Florian Palm
 */
#include <open62541/plugin/nodestore_switch.h>

/**********************************************************
 * Copy of default nodestore (plugins/ua_nodestore_default.c),
 * as it is unlinked with UA_ENABLE_CUSTOM_NODESTORE
 * (See https://github.com/open62541/open62541/pull/2748#issuecomment-496834686)
 **********************************************************/
#include "ziptree.h"

#if UA_MULTITHREADING >= 100
#define BEGIN_CRITSECT(NODEMAP) UA_LOCK(NODEMAP->lock)
#define END_CRITSECT(NODEMAP) UA_UNLOCK(NODEMAP->lock)
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

    /* Compore nodes in detail */
    return (enum ZIP_CMP)UA_NodeId_order(&aa->nodeId, &bb->nodeId);
}

ZIP_HEAD(NodeTree, NodeEntry);
typedef struct NodeTree NodeTree;

typedef struct {
    NodeTree root;
#if UA_MULTITHREADING >= 100
    UA_LOCK_TYPE(lock) /* Protect access */
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
UA_Nodestore_Default_newNode(void *nsCtx, UA_NodeClass nodeClass) {
    NodeEntry *entry = newEntry(nodeClass);
    if(!entry)
        return NULL;
    return (UA_Node*)&entry->nodeId;
}

/* Not yet inserted into the NodeMap */
void
UA_Nodestore_Default_deleteNode(void *nsCtx, UA_Node *node) {
    deleteEntry(container_of(node, NodeEntry, nodeId));
}

const UA_Node *
UA_Nodestore_Default_getNode(void *nsCtx, const UA_NodeId *nodeId) {
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
UA_Nodestore_Default_releaseNode(void *nsCtx, const UA_Node *node) {
    if(!node)
        return;
#if UA_MULTITHREADING >= 100
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
UA_Nodestore_Default_getNodeCopy(void *nsCtx, const UA_NodeId *nodeId,
                         UA_Node **outNode) {
    /* Find the node */
    const UA_Node *node = UA_Nodestore_Default_getNode(nsCtx, nodeId);
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    /* Create the new entry */
    NodeEntry *ne = newEntry(node->nodeClass);
    if(!ne) {
        UA_Nodestore_Default_releaseNode(nsCtx, node);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Copy the node content */
    UA_Node *nnode = (UA_Node*)&ne->nodeId;
    UA_StatusCode retval = UA_Node_copy(node, nnode);
    UA_Nodestore_Default_releaseNode(nsCtx, node);
    if(retval != UA_STATUSCODE_GOOD) {
        deleteEntry(ne);
        return retval;
    }

    ne->orig = container_of(node, NodeEntry, nodeId);
    *outNode = nnode;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Nodestore_Default_insertNode(void *nsCtx, UA_Node *node, UA_NodeId *addedNodeId) {
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
UA_Nodestore_Default_replaceNode(void *nsCtx, UA_Node *node) {
    /* Find the node */
    const UA_Node *oldNode = UA_Nodestore_Default_getNode(nsCtx, &node->nodeId);
    if(!oldNode){
        deleteEntry(container_of(node, NodeEntry, nodeId));
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    /* Test if the copy is current */
    NodeEntry *entry = container_of(node, NodeEntry, nodeId);
    NodeEntry *oldEntry = container_of(oldNode, NodeEntry, nodeId);
    if(oldEntry != entry->orig) {
        /* The node was already updated since the copy was made */
        deleteEntry(entry);
        UA_Nodestore_Default_releaseNode(nsCtx, oldNode);
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

    UA_Nodestore_Default_releaseNode(nsCtx, oldNode);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Nodestore_Default_removeNode(void *nsCtx, const UA_NodeId *nodeId) {
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
UA_Nodestore_Default_iterate(void *nsCtx, UA_NodestoreVisitor visitor,
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
#ifndef UA_ENABLE_CUSTOM_NODESTORE
const UA_Boolean inPlaceEditAllowed = true;
#endif
UA_StatusCode
UA_Nodestore_Default_new(void **nsCtx) {
    /* Allocate and initialize the nodemap */
    NodeMap *nodemap = (NodeMap*)UA_malloc(sizeof(NodeMap));
    if(!nodemap)
        return UA_STATUSCODE_BADOUTOFMEMORY;
#if UA_MULTITHREADING >= 100
    UA_LOCK_INIT(nodemap->lock)
#endif

    ZIP_INIT(&nodemap->root);

    /* Populate the nodestore */
    *nsCtx = (void*)nodemap;
    return UA_STATUSCODE_GOOD;
}

void
UA_Nodestore_Default_delete(void *nsCtx) {
    if (!nsCtx)
        return;

    NodeMap *ns = (NodeMap*)nsCtx;
#if UA_MULTITHREADING >= 100
    UA_LOCK_RELEASE(ns->lock);
#endif
    ZIP_ITER(NodeTree, &ns->root, deleteNodeVisitor, NULL);
    UA_free(ns);
}


/**********************************************************
 *                Start of nodestore switch               *
 **********************************************************/

//TODO Make multithreading save
//TODO rename Switch

/*
 * Definition of a switch based on the namespace id
 */
struct UA_Nodestore_Switch {
	UA_UInt16 size;
	UA_NodestoreInterface *defaultStore; // Nodestore for all namespaces, that have a NULL nodestore interface (and for i>=size)
	UA_NodestoreInterface **stores; 		// Array of all nodestore interfaces
	UA_NodestoreInterface *nsi; // Interface to nodestore switch itself //TODO make const?
};

/*
 * Switch API: linking and unlinking of namespace index to store
 */
UA_NodestoreInterface* UA_Nodestore_Switch_getNodestore(
		UA_Nodestore_Switch* storeSwitch, UA_UInt16 index,
		UA_Boolean useDefault) {
	if (index < storeSwitch->size && storeSwitch->stores[index] != NULL) {
		return storeSwitch->stores[index];
	}
	if (useDefault && storeSwitch->defaultStore != NULL)
		return storeSwitch->defaultStore;
	return NULL;
}

UA_StatusCode UA_Nodestore_Switch_setNodestore(UA_Nodestore_Switch* storeSwitch,
		UA_UInt16 index, UA_NodestoreInterface* store) {
	UA_UInt16 newSize = (UA_UInt16) (index + 1);
	// calculate new size if last nodestore is unlinked
	if (store == NULL && index == (storeSwitch->size - 1)) {
		// check all interface links before last nodestore
		for (newSize = (UA_UInt16) (storeSwitch->size - 1); newSize > 0;
				--newSize) {
			if (storeSwitch->stores[newSize - 1] != NULL)
				break;
		}
	}

	// resize array if neccessary
	if (newSize != storeSwitch->size) {
		UA_NodestoreInterface **newNsArray =
				(UA_NodestoreInterface **) UA_realloc(storeSwitch->stores,
						newSize * sizeof(UA_NodestoreInterface*));
		if (!newNsArray && newSize != 0)
			return UA_STATUSCODE_BADOUTOFMEMORY;
		storeSwitch->stores = newNsArray;
		// fill new array entries with NULL to use the default nodestore
		for (UA_UInt16 i = storeSwitch->size; i < newSize ; ++i) {
			storeSwitch->stores[i] = NULL;
		}
		storeSwitch->size = newSize;
	}

	// set nodestore
	if (index < storeSwitch->size)
		storeSwitch->stores[index] = store;
	return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Nodestore_Switch_setNodestoreDefault(
		UA_Nodestore_Switch* storeSwitch, UA_NodestoreInterface* store) {
	storeSwitch->defaultStore = store;
	return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Nodestore_Switch_getIndices(UA_Nodestore_Switch* storeSwitch,
		UA_NodestoreInterface* store, UA_UInt16* count, UA_UInt16** indices) {
	// Count occurances
	UA_UInt16 found = 0;
	for (UA_UInt16 i = 0; i < storeSwitch->size; i++) {
		if (storeSwitch->stores[i] == store)
			++found;
	}
	// Create an array of for indices
	if (indices != NULL) {
		*indices = (UA_UInt16*) UA_malloc(sizeof(UA_Int16) * found);
		if (*indices != NULL) {
			found = 0;
			for (UA_UInt16 i = 0; i < storeSwitch->size; i++) {
				if (storeSwitch->stores[i] == store)
					*indices[found++] = i;
			}
		} else
			return UA_STATUSCODE_BADOUTOFMEMORY;
	}
	if (count)
		*count = found;
	return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Nodestore_Switch_changeNodestore(
		UA_Nodestore_Switch* storeSwitch, UA_NodestoreInterface *storeOld,
		UA_NodestoreInterface *storeNew) {
	if (storeOld == NULL)
		return UA_STATUSCODE_GOOD;
	for (UA_UInt16 i = 0; i < storeSwitch->size - 1; i++) {
		if (storeSwitch->stores[i] == storeOld)
			storeSwitch->stores[i] = storeNew;
	}
	// resize stores array if last store will be unlinked
	if (storeSwitch->stores[storeSwitch->size - 1] == storeOld) {
		UA_Nodestore_Switch_setNodestore(storeSwitch,
				(UA_UInt16) (storeSwitch->size - 1), storeNew);
	}
	return UA_STATUSCODE_GOOD;
}

/*
 * Switch API: Get an interface of the default nodestore or a switch
 */

UA_StatusCode UA_Nodestore_Default_Interface_new(UA_NodestoreInterface** store) {
	UA_NodestoreInterface* defaultStore = (UA_NodestoreInterface*) UA_malloc(
			sizeof(UA_NodestoreInterface));
	if (defaultStore == NULL) {
		return UA_STATUSCODE_BADOUTOFMEMORY;
	}

	UA_StatusCode result = UA_Nodestore_Default_new(
			(void**) &defaultStore->context);
	if (result != UA_STATUSCODE_GOOD) {
		UA_free(defaultStore);
		return result;
	}
	defaultStore->deleteNodestore = UA_Nodestore_Default_delete;
	defaultStore->newNode = UA_Nodestore_Default_newNode;
	defaultStore->deleteNode = UA_Nodestore_Default_deleteNode;
	defaultStore->getNode = UA_Nodestore_Default_getNode;
	defaultStore->releaseNode = UA_Nodestore_Default_releaseNode;
	defaultStore->getNodeCopy = UA_Nodestore_Default_getNodeCopy;
	defaultStore->insertNode = UA_Nodestore_Default_insertNode;
	defaultStore->replaceNode = UA_Nodestore_Default_replaceNode;
	defaultStore->iterate = UA_Nodestore_Default_iterate;
	defaultStore->removeNode = UA_Nodestore_Default_removeNode;
	*store = defaultStore;
	return UA_STATUSCODE_GOOD;
}

static UA_NodestoreInterface* UA_Nodestore_Switch_Interface_new(
		UA_Nodestore_Switch *storeSwitch) {
	UA_NodestoreInterface* ns = (UA_NodestoreInterface*) UA_malloc(
			sizeof(UA_NodestoreInterface));
	if (ns == NULL) {
		return NULL;
	}
	ns->context = storeSwitch;
	ns->deleteNodestore = UA_Nodestore_delete;
	ns->newNode = UA_Nodestore_newNode;
	ns->deleteNode = UA_Nodestore_deleteNode;
	ns->getNode = UA_Nodestore_getNode;
	ns->releaseNode = UA_Nodestore_releaseNode;
	ns->getNodeCopy = UA_Nodestore_getNodeCopy;
	ns->insertNode = UA_Nodestore_insertNode;
	ns->replaceNode = UA_Nodestore_replaceNode;
	ns->iterate = UA_Nodestore_iterate;
	ns->removeNode = UA_Nodestore_removeNode;
	return ns;
}

UA_NodestoreInterface*
UA_Nodestore_Switch_Interface_get(UA_Nodestore_Switch *storeSwitch) {
	return storeSwitch->nsi;
}

/*
 * Nodestore switch life cycle
 */

UA_StatusCode UA_Nodestore_Switch_newEmpty(void **nsCtx) {
	UA_Nodestore_Switch *storeSwitch = (UA_Nodestore_Switch *) UA_malloc(
			sizeof(UA_Nodestore_Switch));
	if (!storeSwitch)
		return UA_STATUSCODE_BADOUTOFMEMORY;
	storeSwitch->nsi = UA_Nodestore_Switch_Interface_new(storeSwitch);
	if (!storeSwitch->nsi) {
		UA_free(storeSwitch);
		return UA_STATUSCODE_BADOUTOFMEMORY;
	}
	storeSwitch->defaultStore = NULL;
	storeSwitch->stores = NULL;
	storeSwitch->size = 0;
	*nsCtx = (void*) storeSwitch;
	return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Nodestore_new(void **nsCtx) {
	UA_StatusCode result = UA_Nodestore_Switch_newEmpty(nsCtx);
	if (result != UA_STATUSCODE_GOOD)
		return result;
	result = UA_Nodestore_Default_Interface_new(
			&((UA_Nodestore_Switch*)*nsCtx)->defaultStore);
	if (result != UA_STATUSCODE_GOOD) {
		UA_free(*nsCtx);
	}
	return result;
}

void UA_Nodestore_delete(void *nsCtx) {
	UA_Nodestore_Switch *storeSwitch = (UA_Nodestore_Switch *) nsCtx;
	// delete custom nodestores
	for (size_t i = 0; i < storeSwitch->size; i++) {
		// if namespace i has custom nodestore
		if (storeSwitch->stores[i]) {
			//check that custom nodestore is not equal to default nodestore
			if (storeSwitch->stores[i] == storeSwitch->defaultStore) {
				storeSwitch->stores[i] = NULL;
				continue;
			}
			// search forward for other occurances of nodestore and set to null
			for (size_t j = i + 1; j < storeSwitch->size; j++) {
				// check that nodestore is equal by comparision of interfaces
				if (storeSwitch->stores[j] == storeSwitch->stores[i])
					storeSwitch->stores[j] = NULL;
			}
			// delete the nodestore
			storeSwitch->stores[i]->deleteNodestore(
					storeSwitch->stores[i]->context);
			UA_free(storeSwitch->stores[i]);
			storeSwitch->stores[i] = NULL;
		}
	}
	// delete default nodestore
	if (storeSwitch->defaultStore) {
		storeSwitch->defaultStore->deleteNodestore(
				storeSwitch->defaultStore->context);
		UA_free(storeSwitch->defaultStore);
		storeSwitch->defaultStore = NULL;
	}

	// delete nodestore switch itself
	UA_free(storeSwitch->stores);
	storeSwitch->stores = NULL;
	UA_free(storeSwitch->nsi);
	UA_free(storeSwitch);
}

/*
 * Functions of nodestore interface that are plugged into the server and do the actual switch
 */

UA_Node *UA_Nodestore_newNode(void *nsCtx, UA_NodeClass nodeClass) {
	//TODO: Not clear in which nodestore the memory has to be allocated. (storeSwitch or defaultNodestore)
	UA_Nodestore_Switch *storeSwitch = (UA_Nodestore_Switch *) nsCtx;
	return storeSwitch->defaultStore->newNode(
			storeSwitch->defaultStore->context, nodeClass);
}

void UA_Nodestore_deleteNode(void *nsCtx, UA_Node *node) {
	UA_NodestoreInterface * nsi = UA_Nodestore_Switch_getNodestore((UA_Nodestore_Switch *) nsCtx,
			node->nodeId.namespaceIndex, UA_TRUE);
	if (nsi != NULL) {
		nsi->deleteNode(nsi->context, node);
		return;
	}
}

const UA_Node *UA_Nodestore_getNode(void *nsCtx, const UA_NodeId *nodeId) {
	UA_NodestoreInterface * nsi = UA_Nodestore_Switch_getNodestore((UA_Nodestore_Switch *) nsCtx,
			nodeId->namespaceIndex, UA_TRUE);
	if (nsi != NULL) {
		return (nsi->getNode(nsi->context, nodeId));
	}
	return NULL;
}

void UA_Nodestore_releaseNode(void *nsCtx, const UA_Node *node) {
	if (node == NULL) //TODO check wether this should lead to an error?
		return;
	UA_NodestoreInterface * nsi = UA_Nodestore_Switch_getNodestore((UA_Nodestore_Switch *) nsCtx,
			node->nodeId.namespaceIndex, UA_TRUE);
	if (nsi != NULL) {
		nsi->releaseNode(nsi->context, node);
		return;
	}
}

UA_StatusCode UA_Nodestore_getNodeCopy(void *nsCtx,
		const UA_NodeId *nodeId, UA_Node **outNode) {
	UA_NodestoreInterface * nsi = UA_Nodestore_Switch_getNodestore((UA_Nodestore_Switch *) nsCtx,
			nodeId->namespaceIndex, UA_TRUE);
	if (nsi != NULL)
		return nsi->getNodeCopy(nsi->context, nodeId, outNode);
	return UA_STATUSCODE_BADNODEIDUNKNOWN;
}

UA_StatusCode UA_Nodestore_insertNode(void *nsCtx, UA_Node *node,
		UA_NodeId *addedNodeId) {
	UA_NodestoreInterface * nsi = UA_Nodestore_Switch_getNodestore((UA_Nodestore_Switch *) nsCtx,
			node->nodeId.namespaceIndex, UA_TRUE);
	if (nsi != NULL)
		return (nsi->insertNode(nsi->context, node, addedNodeId));
	return UA_STATUSCODE_BADNODEIDUNKNOWN;
}

UA_StatusCode UA_Nodestore_replaceNode(void *nsCtx, UA_Node *node) {
	UA_NodestoreInterface * nsi = UA_Nodestore_Switch_getNodestore((UA_Nodestore_Switch *) nsCtx,
			node->nodeId.namespaceIndex, UA_TRUE);
	if (nsi != NULL)
		return nsi->replaceNode(nsi->context, node);
	return UA_STATUSCODE_BADNODEIDUNKNOWN; //TODO check if BADNODEIDUNKNOWN
}

UA_StatusCode UA_Nodestore_removeNode(void *nsCtx,
		const UA_NodeId *nodeId) {
	UA_NodestoreInterface * nsi = UA_Nodestore_Switch_getNodestore((UA_Nodestore_Switch *) nsCtx,
			nodeId->namespaceIndex, UA_TRUE);
	if (nsi != NULL)
		return nsi->removeNode(nsi->context, nodeId);
	return UA_STATUSCODE_BADNODEIDUNKNOWN;
}

void UA_Nodestore_iterate(void *nsCtx, UA_NodestoreVisitor visitor,
		void *visitorCtx) {
	UA_Nodestore_Switch *storeSwitch = (UA_Nodestore_Switch *) nsCtx;
	UA_NodestoreInterface *tempArray[storeSwitch->size];
	for (size_t i = 0; i < storeSwitch->size; i++) {
		for (size_t j = 0; j <= i; j++) {
			if (tempArray[j] == storeSwitch->stores[i])
				break;
			else if (j == i) {
				tempArray[i] = storeSwitch->stores[i];
				storeSwitch->stores[i]->iterate(storeSwitch, visitor,
						visitorCtx);
			}
		}
	}
}
