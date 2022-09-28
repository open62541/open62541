/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2014-2019 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Julian Grothoff
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#include <open62541/util.h>
#include <open62541/plugin/nodestore_default.h>

#ifndef container_of
#define container_of(ptr, type, member) \
    (type *)((uintptr_t)ptr - offsetof(type,member))
#endif

/* The default Nodestore is simply a hash-map from NodeIds to Nodes. To find an
 * entry, iterate over candidate positions according to the NodeId hash.
 *
 * - Tombstone or non-matching NodeId: continue searching
 * - Matching NodeId: Return the entry
 * - NULL: Abort the search */

typedef struct UA_NodeMapEntry {
    struct UA_NodeMapEntry *orig; /* the version this is a copy from (or NULL) */
    UA_UInt16 refCount; /* How many consumers have a reference to the node? */
    UA_Boolean deleted; /* Node was marked as deleted and can be deleted when refCount == 0 */
    UA_Node node;
} UA_NodeMapEntry;

#define UA_NODEMAP_MINSIZE 64
#define UA_NODEMAP_TOMBSTONE ((UA_NodeMapEntry*)0x01)

typedef struct {
    UA_NodeMapEntry *entry;
    UA_UInt32 nodeIdHash;
} UA_NodeMapSlot;

typedef struct {
    UA_NodeMapSlot *slots;
    UA_UInt32 size;
    UA_UInt32 count;
    UA_UInt32 sizePrimeIndex;

    /* Maps ReferenceTypeIndex to the NodeId of the ReferenceType */
    UA_NodeId referenceTypeIds[UA_REFERENCETYPESET_MAX];
    UA_Byte referenceTypeCounter;
} UA_NodeMap;

/*********************/
/* HashMap Utilities */
/*********************/

/* The size of the hash-map is always a prime number. They are chosen to be
 * close to the next power of 2. So the size ca. doubles with each prime. */
static UA_UInt32 const primes[] = {
    7,         13,         31,         61,         127,         251,
    509,       1021,       2039,       4093,       8191,        16381,
    32749,     65521,      131071,     262139,     524287,      1048573,
    2097143,   4194301,    8388593,    16777213,   33554393,    67108859,
    134217689, 268435399,  536870909,  1073741789, 2147483647,  4294967291
};

static UA_UInt32 mod(UA_UInt32 h, UA_UInt32 size) { return h % size; }
static UA_UInt32 mod2(UA_UInt32 h, UA_UInt32 size) { return 1 + (h % (size - 2)); }

static UA_UInt16
higher_prime_index(UA_UInt32 n) {
    UA_UInt16 low  = 0;
    UA_UInt16 high = (UA_UInt16)(sizeof(primes) / sizeof(UA_UInt32));
    while(low != high) {
        UA_UInt16 mid = (UA_UInt16)(low + ((high - low) / 2));
        if(n > primes[mid])
            low = (UA_UInt16)(mid + 1);
        else
            high = mid;
    }
    return low;
}

/* Returns an empty slot or null if the nodeid exists or if no empty slot is found. */
static UA_NodeMapSlot *
findFreeSlot(const UA_NodeMap *ns, const UA_NodeId *nodeid) {
    UA_UInt32 h = UA_NodeId_hash(nodeid);
    UA_UInt32 size = ns->size;
    UA_UInt64 idx = mod(h, size); /* Use 64bit container to avoid overflow  */
    UA_UInt32 startIdx = (UA_UInt32)idx;
    UA_UInt32 hash2 = mod2(h, size);

    UA_NodeMapSlot *candidate = NULL;
    do {
        UA_NodeMapSlot *slot = &ns->slots[(UA_UInt32)idx];

        if(slot->entry > UA_NODEMAP_TOMBSTONE) {
            /* A Node with the NodeId does already exist */
            if(slot->nodeIdHash == h &&
               UA_NodeId_equal(&slot->entry->node.head.nodeId, nodeid))
                return NULL;
        } else {
            /* Found a candidate node */
            if(!candidate)
                candidate = slot;
            /* No matching node can come afterwards */
            if(slot->entry == NULL)
                return candidate;
        }

        idx += hash2;
        if(idx >= size)
            idx -= size;
    } while((UA_UInt32)idx != startIdx);

    return candidate;
}

/* The occupancy of the table after the call will be about 50% */
static UA_StatusCode
expand(UA_NodeMap *ns) {
    UA_UInt32 osize = ns->size;
    UA_UInt32 count = ns->count;
    /* Resize only when table after removal of unused elements is either too
       full or too empty */
    if(count * 2 < osize && (count * 8 > osize || osize <= UA_NODEMAP_MINSIZE))
        return UA_STATUSCODE_GOOD;

    UA_NodeMapSlot *oslots = ns->slots;
    UA_UInt32 nindex = higher_prime_index(count * 2);
    UA_UInt32 nsize = primes[nindex];
    UA_NodeMapSlot *nslots= (UA_NodeMapSlot*)UA_calloc(nsize, sizeof(UA_NodeMapSlot));
    if(!nslots)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    ns->slots = nslots;
    ns->size = nsize;
    ns->sizePrimeIndex = nindex;

    /* recompute the position of every entry and insert the pointer */
    for(size_t i = 0, j = 0; i < osize && j < count; ++i) {
        if(oslots[i].entry <= UA_NODEMAP_TOMBSTONE)
            continue;
        UA_NodeMapSlot *s = findFreeSlot(ns, &oslots[i].entry->node.head.nodeId);
        UA_assert(s);
        *s = oslots[i];
        ++j;
    }

    UA_free(oslots);
    return UA_STATUSCODE_GOOD;
}

static UA_NodeMapEntry *
createEntry(UA_NodeClass nodeClass) {
    size_t size = sizeof(UA_NodeMapEntry) - sizeof(UA_Node);
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
    UA_NodeMapEntry *entry = (UA_NodeMapEntry*)UA_calloc(1, size);
    if(!entry)
        return NULL;
    entry->node.head.nodeClass = nodeClass;
    return entry;
}

static void
deleteNodeMapEntry(UA_NodeMapEntry *entry) {
    UA_Node_clear(&entry->node);
    UA_free(entry);
}

static void
cleanupNodeMapEntry(UA_NodeMapEntry *entry) {
    if(entry->refCount > 0)
        return;
    if(entry->deleted) {
        deleteNodeMapEntry(entry);
        return;
    }
    for(size_t i = 0; i < entry->node.head.referencesSize; i++) {
        UA_NodeReferenceKind *rk = &entry->node.head.references[i];
        if(rk->targetsSize > 16 && !rk->hasRefTree)
            UA_NodeReferenceKind_switch(rk);
    }
}

static UA_NodeMapSlot *
findOccupiedSlot(const UA_NodeMap *ns, const UA_NodeId *nodeid) {
    UA_UInt32 h = UA_NodeId_hash(nodeid);
    UA_UInt32 size = ns->size;
    UA_UInt64 idx = mod(h, size); /* Use 64bit container to avoid overflow */
    UA_UInt32 hash2 = mod2(h, size);
    UA_UInt32 startIdx = (UA_UInt32)idx;

    do {
        UA_NodeMapSlot *slot= &ns->slots[(UA_UInt32)idx];
        if(slot->entry > UA_NODEMAP_TOMBSTONE) {
            if(slot->nodeIdHash == h &&
               UA_NodeId_equal(&slot->entry->node.head.nodeId, nodeid))
                return slot;
        } else {
            if(slot->entry == NULL)
                return NULL; /* No further entry possible */
        }

        idx += hash2;
        if(idx >= size)
            idx -= size;
    } while((UA_UInt32)idx != startIdx);

    return NULL;
}

/***********************/
/* Interface functions */
/***********************/

static UA_Node *
UA_NodeMap_newNode(void *context, UA_NodeClass nodeClass) {
    UA_NodeMapEntry *entry = createEntry(nodeClass);
    if(!entry)
        return NULL;
    return &entry->node;
}

static void
UA_NodeMap_deleteNode(void *context, UA_Node *node) {
    UA_NodeMapEntry *entry = container_of(node, UA_NodeMapEntry, node);
    UA_assert(&entry->node == node);
    deleteNodeMapEntry(entry);
}

static const UA_Node *
UA_NodeMap_getNode(void *context, const UA_NodeId *nodeid,
                   UA_UInt32 attributeMask,
                   UA_ReferenceTypeSet references,
                   UA_BrowseDirection referenceDirections) {
    UA_NodeMap *ns = (UA_NodeMap*)context;
    UA_NodeMapSlot *slot = findOccupiedSlot(ns, nodeid);
    if(!slot)
        return NULL;
    ++slot->entry->refCount;
    return &slot->entry->node;
}

static const UA_Node *
UA_NodeMap_getNodeFromPtr(void *context, UA_NodePointer ptr,
                          UA_UInt32 attributeMask,
                          UA_ReferenceTypeSet references,
                          UA_BrowseDirection referenceDirections) {
    if(!UA_NodePointer_isLocal(ptr))
        return NULL;
    UA_NodeId id = UA_NodePointer_toNodeId(ptr);
    return UA_NodeMap_getNode(context, &id, attributeMask, references, referenceDirections);
}

static void
UA_NodeMap_releaseNode(void *context, const UA_Node *node) {
    if (!node)
        return;
    UA_NodeMapEntry *entry = container_of(node, UA_NodeMapEntry, node);
    UA_assert(&entry->node == node);
    UA_assert(entry->refCount > 0);
    --entry->refCount;
    cleanupNodeMapEntry(entry);
}

static UA_StatusCode
UA_NodeMap_getNodeCopy(void *context, const UA_NodeId *nodeid,
                       UA_Node **outNode) {
    UA_NodeMap *ns = (UA_NodeMap*)context;
    UA_NodeMapSlot *slot = findOccupiedSlot(ns, nodeid);
    if(!slot)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    UA_NodeMapEntry *entry = slot->entry;
    UA_NodeMapEntry *newItem = createEntry(entry->node.head.nodeClass);
    if(!newItem)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_StatusCode retval = UA_Node_copy(&entry->node, &newItem->node);
    if(retval == UA_STATUSCODE_GOOD) {
        newItem->orig = entry; /* Store the pointer to the original */
        *outNode = &newItem->node;
    } else {
        deleteNodeMapEntry(newItem);
    }
    return retval;
}

static UA_StatusCode
UA_NodeMap_removeNode(void *context, const UA_NodeId *nodeid) {
    UA_NodeMap *ns = (UA_NodeMap*)context;
    UA_NodeMapSlot *slot = findOccupiedSlot(ns, nodeid);
    if(!slot)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    UA_NodeMapEntry *entry = slot->entry;
    slot->entry = UA_NODEMAP_TOMBSTONE;
    entry->deleted = true;
    cleanupNodeMapEntry(entry);
    --ns->count;
    /* Downsize the hashmap if it is very empty */
    if(ns->count * 8 < ns->size && ns->size > UA_NODEMAP_MINSIZE)
        expand(ns); /* Can fail. Just continue with the bigger hashmap. */
    return UA_STATUSCODE_GOOD;
}

/*
 * If this function fails in any way, the node parameter is deleted here,
 * so the caller function does not need to take care of it anymore
 */
static UA_StatusCode
UA_NodeMap_insertNode(void *context, UA_Node *node,
                      UA_NodeId *addedNodeId) {
    UA_NodeMap *ns = (UA_NodeMap*)context;
    if(ns->size * 3 <= ns->count * 4) {
        if(expand(ns) != UA_STATUSCODE_GOOD){
            deleteNodeMapEntry(container_of(node, UA_NodeMapEntry, node));
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    }

    UA_NodeMapSlot *slot;
    if(node->head.nodeId.identifierType == UA_NODEIDTYPE_NUMERIC &&
       node->head.nodeId.identifier.numeric == 0) {
        /* Create a random nodeid: Start at least with 50,000 to make sure we
         * don not conflict with nodes from the spec. If we find a conflict, we
         * just try another identifier until we have tried all possible
         * identifiers. Since the size is prime and we don't change the increase
         * val, we will reach the starting id again. E.g. adding a nodeset will
         * create children while there are still other nodes which need to be
         * created. Thus the node ids may collide. */
        UA_UInt32 size = ns->size;
        UA_UInt64 identifier = mod(50000 + size+1, UA_UINT32_MAX); /* Use 64bit to
                                                                    * avoid overflow */
        UA_UInt32 increase = mod2(ns->count+1, size);
        UA_UInt32 startId = (UA_UInt32)identifier; /* mod ensures us that the id
                                                    * is a valid 32 bit integer */

        do {
            node->head.nodeId.identifier.numeric = (UA_UInt32)identifier;
            slot = findFreeSlot(ns, &node->head.nodeId);
            if(slot)
                break;
            identifier += increase;
            if(identifier >= size)
                identifier -= size;
#if SIZE_MAX <= UA_UINT32_MAX
            /* The compressed "immediate" representation of nodes does not
             * support the full range on 32bit systems. Generate smaller
             * identifiers as they can be stored more compactly. */
            if(identifier >= (0x01 << 24))
                identifier = identifier % (0x01 << 24);
#endif
        } while((UA_UInt32)identifier != startId);
    } else {
        slot = findFreeSlot(ns, &node->head.nodeId);
    }

    if(!slot) {
        deleteNodeMapEntry(container_of(node, UA_NodeMapEntry, node));
        return UA_STATUSCODE_BADNODEIDEXISTS;
    }

    /* Copy the NodeId */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(addedNodeId) {
        retval = UA_NodeId_copy(&node->head.nodeId, addedNodeId);
        if(retval != UA_STATUSCODE_GOOD) {
            deleteNodeMapEntry(container_of(node, UA_NodeMapEntry, node));
            return retval;
        }
    }

    /* For new ReferencetypeNodes add to the index map */
    if(node->head.nodeClass == UA_NODECLASS_REFERENCETYPE) {
        UA_ReferenceTypeNode *refNode = &node->referenceTypeNode;
        if(ns->referenceTypeCounter >= UA_REFERENCETYPESET_MAX) {
            deleteNodeMapEntry(container_of(node, UA_NodeMapEntry, node));
            return UA_STATUSCODE_BADINTERNALERROR;
        }

        retval = UA_NodeId_copy(&node->head.nodeId, &ns->referenceTypeIds[ns->referenceTypeCounter]);
        if(retval != UA_STATUSCODE_GOOD) {
            deleteNodeMapEntry(container_of(node, UA_NodeMapEntry, node));
            return UA_STATUSCODE_BADINTERNALERROR;
        }

        /* Assign the ReferenceTypeIndex to the new ReferenceTypeNode */
        refNode->referenceTypeIndex = ns->referenceTypeCounter;
        refNode->subTypes = UA_REFTYPESET(ns->referenceTypeCounter);

        ns->referenceTypeCounter++;
    }

    /* Insert the node */
    UA_NodeMapEntry *newEntry = container_of(node, UA_NodeMapEntry, node);
    slot->nodeIdHash = UA_NodeId_hash(&node->head.nodeId);
    slot->entry = newEntry;
    ++ns->count;
    return retval;
}

static UA_StatusCode
UA_NodeMap_replaceNode(void *context, UA_Node *node) {
    UA_NodeMap *ns = (UA_NodeMap*)context;
    UA_NodeMapEntry *newEntry = container_of(node, UA_NodeMapEntry, node);

    /* Find the node */
    UA_NodeMapSlot *slot = findOccupiedSlot(ns, &node->head.nodeId);
    if(!slot) {
        deleteNodeMapEntry(newEntry);
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    /* The node was already updated since the copy was made? */
    UA_NodeMapEntry *oldEntry = slot->entry;
    if(oldEntry != newEntry->orig) {
        deleteNodeMapEntry(newEntry);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Replace the entry */
    slot->entry = newEntry;
    oldEntry->deleted = true;
    cleanupNodeMapEntry(oldEntry);
    return UA_STATUSCODE_GOOD;
}

static const UA_NodeId *
UA_NodeMap_getReferenceTypeId(void *nsCtx, UA_Byte refTypeIndex) {
    UA_NodeMap *ns = (UA_NodeMap*)nsCtx;
    if(refTypeIndex >= ns->referenceTypeCounter)
        return NULL;
    return &ns->referenceTypeIds[refTypeIndex];
}

static void
UA_NodeMap_iterate(void *context, UA_NodestoreVisitor visitor,
                   void *visitorContext) {
    UA_NodeMap *ns = (UA_NodeMap*)context;
    for(UA_UInt32 i = 0; i < ns->size; ++i) {
        UA_NodeMapSlot *slot = &ns->slots[i];
        if(slot->entry > UA_NODEMAP_TOMBSTONE) {
            /* The visitor can delete the node. So refcount here. */
            slot->entry->refCount++;
            visitor(visitorContext, &slot->entry->node);
            slot->entry->refCount--;
            cleanupNodeMapEntry(slot->entry);
        }
    }
}

static void
UA_NodeMap_delete(void *context) {
    /* Already cleaned up? */
    if(!context)
        return;

    UA_NodeMap *ns = (UA_NodeMap*)context;
    UA_UInt32 size = ns->size;
    UA_NodeMapSlot *slots = ns->slots;
    for(UA_UInt32 i = 0; i < size; ++i) {
        if(slots[i].entry > UA_NODEMAP_TOMBSTONE) {
            /* On debugging builds, check that all nodes were release */
            UA_assert(slots[i].entry->refCount == 0);
            /* Delete the node */
            deleteNodeMapEntry(slots[i].entry);
        }
    }
    UA_free(ns->slots);

    /* Clean up the ReferenceTypes index array */
    for(size_t i = 0; i < ns->referenceTypeCounter; i++)
        UA_NodeId_clear(&ns->referenceTypeIds[i]);

    UA_free(ns);
}

UA_StatusCode
UA_Nodestore_HashMap(UA_Nodestore *ns) {
    /* Allocate and initialize the nodemap */
    UA_NodeMap *nodemap = (UA_NodeMap*)UA_malloc(sizeof(UA_NodeMap));
    if(!nodemap)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    nodemap->sizePrimeIndex = higher_prime_index(UA_NODEMAP_MINSIZE);
    nodemap->size = primes[nodemap->sizePrimeIndex];
    nodemap->count = 0;
    nodemap->slots = (UA_NodeMapSlot*)
        UA_calloc(nodemap->size, sizeof(UA_NodeMapSlot));
    if(!nodemap->slots) {
        UA_free(nodemap);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    nodemap->referenceTypeCounter = 0;

    /* Populate the nodestore */
    ns->context = nodemap;
    ns->clear = UA_NodeMap_delete;
    ns->newNode = UA_NodeMap_newNode;
    ns->deleteNode = UA_NodeMap_deleteNode;
    ns->getNode = UA_NodeMap_getNode;
    ns->getNodeFromPtr = UA_NodeMap_getNodeFromPtr;
    ns->releaseNode = UA_NodeMap_releaseNode;
    ns->getNodeCopy = UA_NodeMap_getNodeCopy;
    ns->insertNode = UA_NodeMap_insertNode;
    ns->replaceNode = UA_NodeMap_replaceNode;
    ns->removeNode = UA_NodeMap_removeNode;
    ns->getReferenceTypeId = UA_NodeMap_getReferenceTypeId;
    ns->iterate = UA_NodeMap_iterate;
    return UA_STATUSCODE_GOOD;
}
