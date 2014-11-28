#include "ua_nodestore.h"
#include "ua_util.h"
#include "ua_statuscodes.h"

/* It could happen that we want to delete a node even though a function higher
   in the call-chain still has a reference. So we count references and delete
   once the count falls to zero. That means we copy every node to a new place
   where it is right behind the refcount integer.

   Since we copy nodes on the heap, we make the alloc for the nodeEntry bigger
   to accommodate for the different nodeclasses (the nodeEntry may have an
   overlength "tail"). */
#define ALIVE_BIT (1 << 15) /* Alive bit in the refcount */
struct nodeEntry {
    UA_UInt16 refcount;
    const UA_Node node;
};

struct UA_NodeStore {
    struct nodeEntry **entries;
    UA_UInt32          size;
    UA_UInt32          count;
    UA_UInt32          sizePrimeIndex;
};

#include "ua_nodestore_hash.inc"

/* The size of the hash-map is always a prime number. They are chosen to be
   close to the next power of 2. So the size ca. doubles with each prime. */
static hash_t const primes[] = {
    7,         13,         31,         61,         127,         251,
    509,       1021,       2039,       4093,       8191,        16381,
    32749,     65521,      131071,     262139,     524287,      1048573,
    2097143,   4194301,    8388593,    16777213,   33554393,    67108859,
    134217689, 268435399,  536870909,  1073741789, 2147483647,  4294967291
};

static INLINE UA_Int16 higher_prime_index(hash_t n) {
    UA_UInt16 low  = 0;
    UA_UInt16 high = sizeof(primes) / sizeof(hash_t);
    while(low != high) {
        UA_UInt16 mid = low + (high - low) / 2;
        if(n > primes[mid])
            low = mid + 1;
        else
            high = mid;
    }
    return low;
}

/* Returns UA_TRUE if an entry was found under the nodeid. Otherwise, returns
   false and sets slot to a pointer to the next free slot. */
static UA_Boolean __containsNodeId(const UA_NodeStore *ns, const UA_NodeId *nodeid, struct nodeEntry ***entry) {
    hash_t         h     = hash(nodeid);
    UA_UInt32      size  = ns->size;
    hash_t         index = mod(h, size);
    struct nodeEntry **e = &ns->entries[index];

    if(*e == UA_NULL) {
        *entry = e;
        return UA_FALSE;
    }

    if(UA_NodeId_equal(&(*e)->node.nodeId, nodeid)) {
        *entry = e;
        return UA_TRUE;
    }

    hash_t hash2 = mod2(h, size);
    for(;;) {
        index += hash2;
        if(index >= size)
            index -= size;

        e = &ns->entries[index];

        if(*e == UA_NULL) {
            *entry = e;
            return UA_FALSE;
        }

        if(UA_NodeId_equal(&(*e)->node.nodeId, nodeid)) {
            *entry = e;
            return UA_TRUE;
        }
    }

    /* NOTREACHED */
    return UA_TRUE;
}

/* The following function changes size of memory allocated for the entries and
   repeatedly inserts the table elements. The occupancy of the table after the
   call will be about 50%. */
static UA_StatusCode __expand(UA_NodeStore *ns) {
    UA_Int32 osize = ns->size;
    UA_Int32 count = ns->count;
    /* Resize only when table after removal of unused elements is either too full or too empty.  */
    if(count * 2 < osize && (count * 8 > osize || osize <= 32))
        return UA_STATUSCODE_GOOD;


    UA_UInt32 nindex = higher_prime_index(count * 2);
    UA_Int32 nsize = primes[nindex];
    struct nodeEntry **nentries;
    if(!(nentries = UA_alloc(sizeof(struct nodeEntry *) * nsize)))
        return UA_STATUSCODE_BADOUTOFMEMORY;

    memset(nentries, 0, nsize * sizeof(struct nodeEntry *));
    struct nodeEntry **oentries = ns->entries;
    ns->entries = nentries;
    ns->size    = nsize;
    ns->sizePrimeIndex = nindex;

    // recompute the position of every entry and insert the pointer
    for(UA_Int32 i=0, j=0;i<osize && j<count;i++) {
        if(!oentries[i])
            continue;
        struct nodeEntry **e;
        __containsNodeId(ns, &(*oentries[i]).node.nodeId, &e);  /* We know this returns an empty entry here */
        *e = oentries[i];
        j++;
    }

    UA_free(oentries);
    return UA_STATUSCODE_GOOD;
}

/* Marks the entry dead and deletes if necessary. */
static void __deleteEntry(struct nodeEntry *entry) {
    if(entry->refcount > 0)
        return;
    const UA_Node *node = &entry->node;
    switch(node->nodeClass) {
    case UA_NODECLASS_OBJECT:
        UA_ObjectNode_deleteMembers((UA_ObjectNode *)node);
        break;
    case UA_NODECLASS_VARIABLE:
        UA_VariableNode_deleteMembers((UA_VariableNode *)node);
        break;
    case UA_NODECLASS_METHOD:
        UA_MethodNode_deleteMembers((UA_MethodNode *)node);
        break;
    case UA_NODECLASS_OBJECTTYPE:
        UA_ObjectTypeNode_deleteMembers((UA_ObjectTypeNode *)node);
        break;
    case UA_NODECLASS_VARIABLETYPE:
        UA_VariableTypeNode_deleteMembers((UA_VariableTypeNode *)node);
        break;
    case UA_NODECLASS_REFERENCETYPE:
        UA_ReferenceTypeNode_deleteMembers((UA_ReferenceTypeNode *)node);
        break;
    case UA_NODECLASS_DATATYPE:
        UA_DataTypeNode_deleteMembers((UA_DataTypeNode *)node);
        break;
    case UA_NODECLASS_VIEW:
        UA_ViewNode_deleteMembers((UA_ViewNode *)node);
        break;
    default:
        UA_assert(UA_FALSE);
        break;
    }
    UA_free(entry);
}

static INLINE struct nodeEntry * __nodeEntryFromNode(const UA_Node *node) {
    UA_UInt32 nodesize = 0;
    /* Copy the node into the entry. Then reset the original node. It shall no longer be used. */
    switch(node->nodeClass) {
    case UA_NODECLASS_OBJECT:
        nodesize = sizeof(UA_ObjectNode);
        break;
    case UA_NODECLASS_VARIABLE:
        nodesize = sizeof(UA_VariableNode);
        break;
    case UA_NODECLASS_METHOD:
        nodesize = sizeof(UA_MethodNode);
        break;
    case UA_NODECLASS_OBJECTTYPE:
        nodesize = sizeof(UA_ObjectTypeNode);
        break;
    case UA_NODECLASS_VARIABLETYPE:
        nodesize = sizeof(UA_VariableTypeNode);
        break;
    case UA_NODECLASS_REFERENCETYPE:
        nodesize = sizeof(UA_ReferenceTypeNode);
        break;
    case UA_NODECLASS_DATATYPE:
        nodesize = sizeof(UA_DataTypeNode);
        break;
    case UA_NODECLASS_VIEW:
        nodesize = sizeof(UA_ViewNode);
        break;
    default:
        UA_assert(UA_FALSE);
    }

    struct nodeEntry *entry;
    if(!(entry = UA_alloc(sizeof(struct nodeEntry) - sizeof(UA_Node) + nodesize)))
        return UA_NULL;
    memcpy((void *)&entry->node, node, nodesize);
    UA_free((void*)node);
    return entry;
}

/**********************/
/* Exported functions */
/**********************/

UA_NodeStore * UA_NodeStore_new() {
    UA_NodeStore *ns;
    if(!(ns = UA_alloc(sizeof(UA_NodeStore))))
        return UA_NULL;

    ns->sizePrimeIndex = higher_prime_index(32);
    ns->size = primes[ns->sizePrimeIndex];
    ns->count = 0;
    if(!(ns->entries = UA_alloc(sizeof(struct nodeEntry *) * ns->size))) {
        UA_free(ns);
        return UA_NULL;
    }
    memset(ns->entries, 0, ns->size * sizeof(struct nodeEntry *));
    return ns;
}

void UA_NodeStore_delete(UA_NodeStore *ns) {
    UA_UInt32 size = ns->size;
    struct nodeEntry **entries = ns->entries;
    for(UA_UInt32 i = 0;i < size;i++) {
        if(entries[i] != UA_NULL) {
            entries[i]->refcount &= ~ALIVE_BIT; // mark dead
            __deleteEntry(entries[i]);
            entries[i] = UA_NULL;
            ns->count--;
        }
    }

    UA_free(ns->entries);
    UA_free(ns);
}

UA_StatusCode UA_NodeStore_insert(UA_NodeStore *ns, const UA_Node **node, UA_Boolean getManaged) {
    if(ns->size * 3 <= ns->count * 4) {
        if(__expand(ns) != UA_STATUSCODE_GOOD)
            return UA_STATUSCODE_BADINTERNALERROR;
    }

    // get a free slot
    struct nodeEntry **slot;
    UA_NodeId *nodeId = (UA_NodeId *)&(*node)->nodeId;
    if(UA_NodeId_isNull(nodeId)) {
        // find a unique nodeid that is not taken
        nodeId->identifierType = UA_NODEIDTYPE_NUMERIC;
        nodeId->namespaceIndex = 1; // namespace 1 is always in the local nodestore
        UA_Int32 identifier = ns->count+1; // start value
        UA_Int32 size = ns->size;
        hash_t increase = mod2(identifier, size);
        while(UA_TRUE) {
            nodeId->identifier.numeric = identifier;
            if(!__containsNodeId(ns, nodeId, &slot))
                break;
            identifier += increase;
            if(identifier >= size)
                identifier -= size;
        }
    } else {
        if(__containsNodeId(ns, nodeId, &slot))
            return UA_STATUSCODE_BADNODEIDEXISTS;
    }
    
    struct nodeEntry *entry = __nodeEntryFromNode(*node);
    if(!entry)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    *slot = entry;
    ns->count++;

    if(getManaged) {
        entry->refcount = ALIVE_BIT + 1;
        *node = &entry->node;
    } else {
        entry->refcount = ALIVE_BIT;
        *node = UA_NULL;
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_NodeStore_replace(UA_NodeStore *ns, const UA_Node **node, UA_Boolean getManaged) {
    struct nodeEntry **slot;
    const UA_NodeId *nodeId = &(*node)->nodeId;
    if(!__containsNodeId(ns, nodeId, &slot))
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    struct nodeEntry *entry = __nodeEntryFromNode(*node);
    if(!entry)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    (*slot)->refcount &= ~ALIVE_BIT; // mark dead
    __deleteEntry(*slot);
    *slot = entry;

    if(getManaged) {
        entry->refcount = ALIVE_BIT + 1;
        *node = &entry->node;
    } else {
        entry->refcount = ALIVE_BIT;
        *node = UA_NULL;
    }
    return UA_STATUSCODE_GOOD;
}

const UA_Node * UA_NodeStore_get(const UA_NodeStore *ns, const UA_NodeId *nodeid) {
    struct nodeEntry **slot;
    if(!__containsNodeId(ns, nodeid, &slot))
        return UA_NULL;
    (*slot)->refcount++;
    return &(*slot)->node;
}

UA_StatusCode UA_NodeStore_remove(UA_NodeStore *ns, const UA_NodeId *nodeid) {
    struct nodeEntry **slot;
    if(!__containsNodeId(ns, nodeid, &slot))
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    // Check before if deleting the node makes the UA_NodeStore inconsistent.
    (*slot)->refcount &= ~ALIVE_BIT; // mark dead
    __deleteEntry(*slot);
    *slot = UA_NULL;
    ns->count--;

    /* Downsize the hashmap if it is very empty */
    if(ns->count * 8 < ns->size && ns->size > 32)
        __expand(ns); // this can fail. we just continue with the bigger hashmap.

    return UA_STATUSCODE_GOOD;
}

void UA_NodeStore_iterate(const UA_NodeStore *ns, UA_NodeStore_nodeVisitor visitor) {
    for(UA_UInt32 i = 0;i < ns->size;i++) {
        if(ns->entries[i] != UA_NULL)
            visitor(&ns->entries[i]->node);
    }
}

void UA_NodeStore_release(const UA_Node *managed) {
    struct nodeEntry *entry = (struct nodeEntry *) ((char*)managed - offsetof(struct nodeEntry, node));
    entry->refcount--;
    __deleteEntry(entry);    
}
