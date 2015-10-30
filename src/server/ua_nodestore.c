#include "ua_nodestore.h"
#include "ua_util.h"
#include "ua_statuscodes.h"

#define UA_NODESTORE_MINSIZE 128

typedef struct {
    UA_Boolean taken;
    union {
        UA_Node node;
        UA_ObjectNode objectNode;
        UA_ObjectTypeNode objectTypeNode;
        UA_VariableNode variableNode;
        UA_VariableTypeNode variableTypeNode;
        UA_ReferenceTypeNode referenceTypeNode;
        UA_MethodNode methodeNode;
        UA_ViewNode viewNode;
        UA_DataTypeNode dataTypeNode;
    } node;
} UA_NodeStoreEntry;

struct UA_NodeStore {
    UA_NodeStoreEntry *entries;
    UA_UInt32 size;
    UA_UInt32 count;
    UA_UInt32 sizePrimeIndex;
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

static UA_Int16 higher_prime_index(hash_t n) {
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
static UA_Boolean
containsNodeId(const UA_NodeStore *ns, const UA_NodeId *nodeid, UA_NodeStoreEntry **entry) {
    hash_t         h     = hash(nodeid);
    UA_UInt32      size  = ns->size;
    hash_t         index = mod(h, size);
    UA_NodeStoreEntry *e = &ns->entries[index];

    if(!e->taken) {
        *entry = e;
        return UA_FALSE;
    }

    if(UA_NodeId_equal(&e->node.node.nodeId, nodeid)) {
        *entry = e;
        return UA_TRUE;
    }

    hash_t hash2 = mod2(h, size);
    for(;;) {
        index += hash2;
        if(index >= size)
            index -= size;
        e = &ns->entries[index];
        if(!e->taken) {
            *entry = e;
            return UA_FALSE;
        }
        if(UA_NodeId_equal(&e->node.node.nodeId, nodeid)) {
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
static UA_StatusCode expand(UA_NodeStore *ns) {
    UA_UInt32 osize = ns->size;
    UA_UInt32 count = ns->count;
    /* Resize only when table after removal of unused elements is either too full or too empty.  */
    if(count * 2 < osize && (count * 8 > osize || osize <= UA_NODESTORE_MINSIZE))
        return UA_STATUSCODE_GOOD;

    UA_UInt32 nindex = higher_prime_index(count * 2);
    UA_Int32 nsize = primes[nindex];
    UA_NodeStoreEntry *nentries;
    if(!(nentries = UA_calloc(nsize, sizeof(UA_NodeStoreEntry))))
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_NodeStoreEntry *oentries = ns->entries;
    ns->entries = nentries;
    ns->size    = nsize;
    ns->sizePrimeIndex = nindex;

    // recompute the position of every entry and insert the pointer
    for(size_t i = 0, j = 0; i < osize && j < count; i++) {
        if(!oentries[i].taken)
            continue;
        UA_NodeStoreEntry *e;
        containsNodeId(ns, &oentries[i].node.node.nodeId, &e);  /* We know this returns an empty entry here */
        *e = oentries[i];
        j++;
    }

    UA_free(oentries);
    return UA_STATUSCODE_GOOD;
}

/* Marks the entry dead and deletes if necessary. */
static UA_INLINE void
deleteEntry(UA_NodeStoreEntry *entry) {
    UA_Node_deleteMembersAnyNodeClass(&entry->node.node);
    entry->taken = UA_FALSE;
}

/** Copies the node into the entry. Then free the original node (but not its content). */
static void fillEntry(UA_NodeStoreEntry *entry, UA_Node *node) {
    size_t nodesize = 0;
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
    memcpy(&entry->node, node, nodesize);
    UA_free(node);
    entry->taken = UA_TRUE;
}

/**********************/
/* Exported functions */
/**********************/

UA_NodeStore * UA_NodeStore_new(void) {
    UA_NodeStore *ns;
    if(!(ns = UA_malloc(sizeof(UA_NodeStore))))
        return NULL;
    ns->sizePrimeIndex = higher_prime_index(UA_NODESTORE_MINSIZE);
    ns->size = primes[ns->sizePrimeIndex];
    ns->count = 0;
    if(!(ns->entries = UA_calloc(ns->size, sizeof(UA_NodeStoreEntry)))) {
        UA_free(ns);
        return NULL;
    }
    return ns;
}

void UA_NodeStore_delete(UA_NodeStore *ns) {
    UA_UInt32 size = ns->size;
    UA_NodeStoreEntry *entries = ns->entries;
    for(UA_UInt32 i = 0;i < size;i++) {
        if(entries[i].taken)
            deleteEntry(&entries[i]);
    }
    UA_free(ns->entries);
    UA_free(ns);
}

UA_StatusCode UA_NodeStore_insert(UA_NodeStore *ns, UA_Node *node, const UA_Node **inserted) {
    if(ns->size * 3 <= ns->count * 4) {
        if(expand(ns) != UA_STATUSCODE_GOOD)
            return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_NodeStoreEntry *entry;
    UA_NodeId tempNodeid;
    tempNodeid = node->nodeId;
    tempNodeid.namespaceIndex = 0;
    if(UA_NodeId_isNull(&tempNodeid)) {
        /* find a free nodeid */
        if(node->nodeId.namespaceIndex == 0) //original request for ns=0 should yield ns=1
            node->nodeId.namespaceIndex = 1;
        UA_Int32 identifier = ns->count+1; // start value
        UA_Int32 size = ns->size;
        hash_t increase = mod2(identifier, size);
        while(UA_TRUE) {
            node->nodeId.identifier.numeric = identifier;
            if(!containsNodeId(ns, &node->nodeId, &entry))
                break;
            identifier += increase;
            if(identifier >= size)
                identifier -= size;
        }
    } else {
        if(containsNodeId(ns, &node->nodeId, &entry))
            return UA_STATUSCODE_BADNODEIDEXISTS;
    }

    fillEntry(entry, node);
    ns->count++;
    if(inserted)
        *inserted = &entry->node.node;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_NodeStore_replace(UA_NodeStore *ns, const UA_Node *oldNode, UA_Node *node, const UA_Node **inserted) {
    UA_NodeStoreEntry *slot;
    if(!containsNodeId(ns, &node->nodeId, &slot))
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    /* that is not the node you are looking for */
    if(&slot->node.node != oldNode)
        return UA_STATUSCODE_BADINTERNALERROR;
    deleteEntry(slot);
    fillEntry(slot, node);
    if(inserted)
        *inserted = &slot->node.node;
    return UA_STATUSCODE_GOOD;
}

const UA_Node * UA_NodeStore_get(const UA_NodeStore *ns, const UA_NodeId *nodeid) {
    UA_NodeStoreEntry *slot;
    if(!containsNodeId(ns, nodeid, &slot))
        return NULL;
    return &slot->node.node;
}

UA_StatusCode UA_NodeStore_remove(UA_NodeStore *ns, const UA_NodeId *nodeid) {
    UA_NodeStoreEntry *slot;
    if(!containsNodeId(ns, nodeid, &slot))
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    deleteEntry(slot);
    ns->count--;
    /* Downsize the hashmap if it is very empty */
    if(ns->count * 8 < ns->size && ns->size > 32)
        expand(ns); // this can fail. we just continue with the bigger hashmap.
    return UA_STATUSCODE_GOOD;
}

void UA_NodeStore_iterate(const UA_NodeStore *ns, UA_NodeStore_nodeVisitor visitor) {
    for(UA_UInt32 i = 0;i < ns->size;i++) {
        if(ns->entries[i].taken)
            visitor(&ns->entries[i].node.node);
    }
}

void UA_NodeStore_release(const UA_Node *managed) {
}
