#include "ua_nodestore.h"
#include "ua_util.h"
#include "ua_statuscodes.h"
#include "ua_server_internal.h"
#include <stdio.h>

#define UA_NODESTORE_MINSIZE 64

typedef struct UA_NodeStoreEntry {
    struct UA_NodeStoreEntry *orig; // the version this is a copy from (or NULL)
    UA_Node node;
} UA_NodeStoreEntry;

struct UA_NodeStore {
    UA_NodeStoreEntry **entries;
    UA_UInt32 size;
    UA_UInt32 count;
    UA_UInt32 sizePrimeIndex;
	UA_Server* server;
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

static UA_UInt16 higher_prime_index(hash_t n) {
    UA_UInt16 low  = 0;
    UA_UInt16 high = (UA_UInt16)(sizeof(primes) / sizeof(hash_t));
    while(low != high) {
        UA_UInt16 mid = (UA_UInt16)(low + ((high - low) / 2));
        if(n > primes[mid])
            low = (UA_UInt16)(mid + 1);
        else
            high = mid;
    }
    return low;
}

static UA_NodeStoreEntry * instantiateEntry(UA_NodeClass nodeClass) {
    size_t size = sizeof(UA_NodeStoreEntry) - sizeof(UA_Node);
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
    UA_NodeStoreEntry *entry = UA_calloc(1, size);
    if(!entry)
        return NULL;
    entry->node.nodeClass = nodeClass;
    return entry;
}

static void deleteEntry(UA_NodeStoreEntry *entry) {
    UA_Node_deleteMembersAnyNodeClass(&entry->node);
    UA_free(entry);
}

/* Returns true if an entry was found under the nodeid. Otherwise, returns
   false and sets slot to a pointer to the next free slot. */
static UA_Boolean
containsNodeId(const UA_NodeStore *ns, const UA_NodeId *nodeid, UA_NodeStoreEntry ***entry) {
    hash_t h = hash(nodeid);
    UA_UInt32 size = ns->size;
    hash_t idx = mod(h, size);
    UA_NodeStoreEntry *e = ns->entries[idx];

    if(!e) {
        *entry = &ns->entries[idx];
        return false;
    }

    if(UA_NodeId_equal(&e->node.nodeId, nodeid)) {
        *entry = &ns->entries[idx];
        return true;
    }

    hash_t hash2 = mod2(h, size);
    for(;;) {
        idx += hash2;
        if(idx >= size)
            idx -= size;
        e = ns->entries[idx];
        if(!e) {
            *entry = &ns->entries[idx];
            return false;
        }
        if(UA_NodeId_equal(&e->node.nodeId, nodeid)) {
            *entry = &ns->entries[idx];
            return true;
        }
    }

    /* NOTREACHED */
    return true;
}

/* The occupancy of the table after the call will be about 50% */
static UA_StatusCode expand(UA_NodeStore *ns) {
    UA_UInt32 osize = ns->size;
    UA_UInt32 count = ns->count;
    /* Resize only when table after removal of unused elements is either too full or too empty  */
    if(count * 2 < osize && (count * 8 > osize || osize <= UA_NODESTORE_MINSIZE))
        return UA_STATUSCODE_GOOD;

    UA_NodeStoreEntry **oentries = ns->entries;
    UA_UInt32 nindex = higher_prime_index(count * 2);
    UA_UInt32 nsize = primes[nindex];
    UA_NodeStoreEntry **nentries;
    if(!(nentries = UA_calloc(nsize, sizeof(UA_NodeStoreEntry*))))
        return UA_STATUSCODE_BADOUTOFMEMORY;

    ns->entries = nentries;
    ns->size = nsize;
    ns->sizePrimeIndex = nindex;

    /* recompute the position of every entry and insert the pointer */
    for(size_t i = 0, j = 0; i < osize && j < count; i++) {
        if(!oentries[i])
            continue;
        UA_NodeStoreEntry **e;
        containsNodeId(ns, &oentries[i]->node.nodeId, &e);  /* We know this returns an empty entry here */
        *e = oentries[i];
        j++;
    }

    UA_free(oentries);
    return UA_STATUSCODE_GOOD;
}

/**********************/
/* Exported functions */
/**********************/

UA_NodeStore * UA_NodeStore_new(UA_Server* server) {
    UA_NodeStore *ns;
    if(!(ns = UA_malloc(sizeof(UA_NodeStore))))
        return NULL;
	ns->server = server;
    ns->sizePrimeIndex = higher_prime_index(UA_NODESTORE_MINSIZE);
    ns->size = primes[ns->sizePrimeIndex];
    ns->count = 0;
    if(!(ns->entries = UA_calloc(ns->size, sizeof(UA_NodeStoreEntry*)))) {
        UA_free(ns);
        return NULL;
    }
    return ns;
}

void UA_NodeStore_delete(UA_NodeStore *ns) {
    UA_UInt32 size = ns->size;
    UA_NodeStoreEntry **entries = ns->entries;
    for(UA_UInt32 i = 0; i < size; i++) {
        if(entries[i])
            deleteEntry(entries[i]);
    }
    UA_free(ns->entries);
    UA_free(ns);
}

UA_Node * UA_NodeStore_newNode(UA_NodeClass class) {
    UA_NodeStoreEntry *entry = instantiateEntry(class);
    if(!entry)
        return NULL;
    return (UA_Node*)&entry->node;
}

void UA_NodeStore_deleteNode(UA_Node *node) {
    deleteEntry(container_of(node, UA_NodeStoreEntry, node));
}

UA_StatusCode UA_NodeStore_insert(UA_NodeStore *ns, UA_Node *node) {
    if(ns->size * 3 <= ns->count * 4) {
        if(expand(ns) != UA_STATUSCODE_GOOD)
            return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_NodeId tempNodeid;
    tempNodeid = node->nodeId;
    tempNodeid.namespaceIndex = 0;
    UA_NodeStoreEntry **entry;
    if(UA_NodeId_isNull(&tempNodeid)) {
        if(node->nodeId.namespaceIndex == 0)
            node->nodeId.namespaceIndex = 1;
        /* find a free nodeid */
        UA_UInt32 identifier = ns->count+1; // start value
        UA_UInt32 size = ns->size;
        hash_t increase = mod2(identifier, size);
        while(true) {
            node->nodeId.identifier.numeric = identifier;
            if(!containsNodeId(ns, &node->nodeId, &entry))
                break;
            identifier += increase;
            if(identifier >= size)
                identifier -= size;
        }
    } else {
        if(containsNodeId(ns, &node->nodeId, &entry)) {
            deleteEntry(container_of(node, UA_NodeStoreEntry, node));
            return UA_STATUSCODE_BADNODEIDEXISTS;
        }
    }

    *entry = container_of(node, UA_NodeStoreEntry, node);
    ns->count++;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_NodeStore_replace(UA_NodeStore *ns, UA_Node *node) {
    UA_NodeStoreEntry **entry;
    if(!containsNodeId(ns, &node->nodeId, &entry))
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    UA_NodeStoreEntry *newEntry = container_of(node, UA_NodeStoreEntry, node);
    if(*entry != newEntry->orig) {
        deleteEntry(newEntry);
        return UA_STATUSCODE_BADINTERNALERROR; // the node was replaced since the copy was made
    }
    deleteEntry(*entry);
    *entry = newEntry;
    return UA_STATUSCODE_GOOD;
}

const UA_Node * UA_NodeStore_get(UA_NodeStore *ns, const UA_NodeId *nodeid) {
	void* nsHandle = ns->server->nodestores[nodeid->namespaceIndex].handle;
	return ns->server->nodestores[nodeid->namespaceIndex].get(nsHandle, nodeid);
}
const UA_Node * UA_NodeStore_get_internal(UA_NodeStore *ns, const UA_NodeId *nodeid) {
	UA_NodeStoreEntry **entry;
	if (!containsNodeId(ns, nodeid, &entry))
	return NULL;
	return (const UA_Node*)&(*entry)->node;
}

UA_Node * UA_NodeStore_getCopy(UA_NodeStore *ns, const UA_NodeId *nodeid) {
    UA_NodeStoreEntry **slot;
    if(!containsNodeId(ns, nodeid, &slot))
        return NULL;
    UA_NodeStoreEntry *entry = *slot;
    UA_NodeStoreEntry *new = instantiateEntry(entry->node.nodeClass);
    if(!new)
        return NULL;
    if(UA_Node_copyAnyNodeClass(&entry->node, &new->node) != UA_STATUSCODE_GOOD) {
        deleteEntry(new);
        return NULL;
    }
    new->orig = entry;
    return &new->node;
}

UA_StatusCode UA_NodeStore_remove(UA_NodeStore *ns, const UA_NodeId *nodeid) {
    UA_NodeStoreEntry **slot;
    if(!containsNodeId(ns, nodeid, &slot))
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    deleteEntry(*slot);
    *slot = NULL;
    ns->count--;
    /* Downsize the hashmap if it is very empty */
    if(ns->count * 8 < ns->size && ns->size > 32)
        expand(ns); // this can fail. we just continue with the bigger hashmap.
    return UA_STATUSCODE_GOOD;
}

void UA_NodeStore_iterate(UA_NodeStore *ns, UA_NodeStore_nodeVisitor visitor) {
    for(UA_UInt32 i = 0; i < ns->size; i++) {
        if(ns->entries[i])
            visitor((UA_Node*)&ns->entries[i]->node);
    }
}
