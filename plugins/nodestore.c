#include "nodestore.h"
#include "../src/server/ua_server_internal.h"
#include "ua_util.h"

#ifndef UA_ENABLE_MULTITHREADING /* conditional compilation */

#define NODESTORE_MINSIZE 64

typedef struct NodeStoreEntry {
    struct NodeStoreEntry *orig; // the version this is a copy from (or NULL)
    UA_Node node;
} NodeStoreEntry;

#define NODESTORE_TOMBSTONE ((NodeStoreEntry*)0x01)

struct NodeStore {
    NodeStoreEntry **entries;
    UA_UInt32 size;
    UA_UInt32 count;
    UA_UInt32 sizePrimeIndex;
};

#include "nodestore_hash.inc"

/* The size of the hash-map is always a prime number. They are chosen to be
 * close to the next power of 2. So the size ca. doubles with each prime. */
static hash_t const primes[] = {
    7,         13,         31,         61,         127,         251,
    509,       1021,       2039,       4093,       8191,        16381,
    32749,     65521,      131071,     262139,     524287,      1048573,
    2097143,   4194301,    8388593,    16777213,   33554393,    67108859,
    134217689, 268435399,  536870909,  1073741789, 2147483647,  4294967291
};

static UA_UInt16
higher_prime_index(hash_t n) {
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

static NodeStoreEntry *
instantiateEntry(UA_NodeClass nodeClass) {
    size_t size = sizeof(NodeStoreEntry) - sizeof(UA_Node);
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
    NodeStoreEntry *entry = UA_calloc(1, size);
    if(!entry)
        return NULL;
    entry->node.nodeClass = nodeClass;
    return entry;
}

static void
deleteEntry(NodeStoreEntry *entry) {
    UA_Node_deleteMembersAnyNodeClass(&entry->node);
    UA_free(entry);
}

/* returns slot of a valid node or null */
static NodeStoreEntry **
findNode(const NodeStore *ns, const UA_NodeId *nodeid) {
    hash_t h = hash(nodeid);
    UA_UInt32 size = ns->size;
    hash_t idx = mod(h, size);
    hash_t hash2 = mod2(h, size);

    while(true) {
        NodeStoreEntry *e = ns->entries[idx];
        if(!e)
            return NULL;
        if(e > NODESTORE_TOMBSTONE &&
           UA_NodeId_equal(&e->node.nodeId, nodeid))
            return &ns->entries[idx];
        idx += hash2;
        if(idx >= size)
            idx -= size;
    }

    /* NOTREACHED */
    return NULL;
}

/* returns an empty slot or null if the nodeid exists */
static NodeStoreEntry **
findSlot(const NodeStore *ns, const UA_NodeId *nodeid) {
    hash_t h = hash(nodeid);
    UA_UInt32 size = ns->size;
    hash_t idx = mod(h, size);
    hash_t hash2 = mod2(h, size);

    while(true) {
        NodeStoreEntry *e = ns->entries[idx];
        if(e > NODESTORE_TOMBSTONE &&
           UA_NodeId_equal(&e->node.nodeId, nodeid))
            return NULL;
        if(ns->entries[idx] <= NODESTORE_TOMBSTONE)
            return &ns->entries[idx];
        idx += hash2;
        if(idx >= size)
            idx -= size;
    }

    /* NOTREACHED */
    return NULL;
}

/* The occupancy of the table after the call will be about 50% */
static UA_StatusCode
expand(NodeStore *ns) {
    UA_UInt32 osize = ns->size;
    UA_UInt32 count = ns->count;
    /* Resize only when table after removal of unused elements is either too
       full or too empty */
    if(count * 2 < osize && (count * 8 > osize || osize <= NODESTORE_MINSIZE))
        return UA_STATUSCODE_GOOD;

    NodeStoreEntry **oentries = ns->entries;
    UA_UInt32 nindex = higher_prime_index(count * 2);
    UA_UInt32 nsize = primes[nindex];
    NodeStoreEntry **nentries = UA_calloc(nsize, sizeof(NodeStoreEntry*));
    if(!nentries)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    ns->entries = nentries;
    ns->size = nsize;
    ns->sizePrimeIndex = nindex;

    /* recompute the position of every entry and insert the pointer */
    for(size_t i = 0, j = 0; i < osize && j < count; i++) {
        if(oentries[i] <= NODESTORE_TOMBSTONE)
            continue;
        NodeStoreEntry **e = findSlot(ns, &oentries[i]->node.nodeId);
        *e = oentries[i];
        j++;
    }

    UA_free(oentries);
    return UA_STATUSCODE_GOOD;
}

/**********************/
/* Exported functions */
/**********************/

NodeStore *
NodeStore_new(void) {
    NodeStore *ns = UA_malloc(sizeof(NodeStore));
    if(!ns)
        return NULL;
    ns->sizePrimeIndex = higher_prime_index(NODESTORE_MINSIZE);
    ns->size = primes[ns->sizePrimeIndex];
    ns->count = 0;
    ns->entries = UA_calloc(ns->size, sizeof(NodeStoreEntry*));
    if(!ns->entries) {
        UA_free(ns);
        return NULL;
    }
    return ns;
}

UA_NodeStoreInterface
UA_NodeStoreInterface_standard() {
	UA_NodeStoreInterface nsi;
	nsi.handle =		NodeStore_new();
	nsi.delete = 		(UA_NodeStoreInterface_delete)		NodeStore_delete;
	nsi.newNode = 		(UA_NodeStoreInterface_newNode)		NodeStore_newNode;
	nsi.deleteNode = 	(UA_NodeStoreInterface_deleteNode)	NodeStore_deleteNode;
	nsi.insert = 		(UA_NodeStoreInterface_insert)		NodeStore_insert;
	nsi.get = 			(UA_NodeStoreInterface_get)			NodeStore_get;
	nsi.getCopy = 		(UA_NodeStoreInterface_getCopy)		NodeStore_getCopy;
	nsi.replace = 		(UA_NodeStoreInterface_replace)		NodeStore_replace;
	nsi.remove = 		(UA_NodeStoreInterface_remove)		NodeStore_remove,
	nsi.iterate = 		(UA_NodeStoreInterface_iterate)		NodeStore_iterate;
	return nsi;
}

void
NodeStore_delete(NodeStore *ns) {
    UA_UInt32 size = ns->size;
    NodeStoreEntry **entries = ns->entries;
    for(UA_UInt32 i = 0; i < size; i++) {
        if(entries[i] > NODESTORE_TOMBSTONE)
            deleteEntry(entries[i]);
    }
    UA_free(ns->entries);
    UA_free(ns);
}

UA_Node *
NodeStore_newNode(UA_NodeClass class) {
    NodeStoreEntry *entry = instantiateEntry(class);
    if(!entry)
        return NULL;
    return &entry->node;
}

void
NodeStore_deleteNode(UA_Node *node) {
    NodeStoreEntry *entry = container_of(node, NodeStoreEntry, node);
    UA_assert(&entry->node == node);
    deleteEntry(entry);
}

UA_StatusCode
NodeStore_insert(NodeStore *ns, UA_Node *node) {
    if(ns->size * 3 <= ns->count * 4) {
        if(expand(ns) != UA_STATUSCODE_GOOD)
            return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_NodeId tempNodeid;
    tempNodeid = node->nodeId;
    tempNodeid.namespaceIndex = 0;
    NodeStoreEntry **entry;
    if(UA_NodeId_isNull(&tempNodeid)) {
        /* create a random nodeid */
        if(node->nodeId.namespaceIndex == 0)
            node->nodeId.namespaceIndex = 1;
        UA_UInt32 identifier = ns->count+1; // start value
        UA_UInt32 size = ns->size;
        hash_t increase = mod2(identifier, size);
        while(true) {
            node->nodeId.identifier.numeric = identifier;
            entry = findSlot(ns, &node->nodeId);
            if(entry)
                break;
            identifier += increase;
            if(identifier >= size)
                identifier -= size;
        }
    } else {
        entry = findSlot(ns, &node->nodeId);
        if(!entry) {
            deleteEntry(container_of(node, NodeStoreEntry, node));
            return UA_STATUSCODE_BADNODEIDEXISTS;
        }
    }

    *entry = container_of(node, NodeStoreEntry, node);
    ns->count++;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
NodeStore_replace(NodeStore *ns, UA_Node *node) {
    NodeStoreEntry **entry = findNode(ns, &node->nodeId);
    if(!entry)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    NodeStoreEntry *newEntry = container_of(node, NodeStoreEntry, node);
    if(*entry != newEntry->orig) {
        // the node was replaced since the copy was made
        deleteEntry(newEntry);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    deleteEntry(*entry);
    *entry = newEntry;
    return UA_STATUSCODE_GOOD;
}

const UA_Node *
NodeStore_get(NodeStore *ns, const UA_NodeId *nodeid) {
    NodeStoreEntry **entry = findNode(ns, nodeid);
    if(!entry)
        return NULL;
    return (const UA_Node*)&(*entry)->node;
}

UA_Node *
NodeStore_getCopy(NodeStore *ns, const UA_NodeId *nodeid) {
    NodeStoreEntry **slot = findNode(ns, nodeid);
    if(!slot)
        return NULL;
    NodeStoreEntry *entry = *slot;
    NodeStoreEntry *new = instantiateEntry(entry->node.nodeClass);
    if(!new)
        return NULL;
    if(UA_Node_copyAnyNodeClass(&entry->node, &new->node) != UA_STATUSCODE_GOOD) {
        deleteEntry(new);
        return NULL;
    }
    new->orig = entry; // store the pointer to the original
    return &new->node;
}

UA_StatusCode
NodeStore_remove(NodeStore *ns, const UA_NodeId *nodeid) {
    NodeStoreEntry **slot = findNode(ns, nodeid);
    if(!slot)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    deleteEntry(*slot);
    *slot = NODESTORE_TOMBSTONE;
    ns->count--;
    /* Downsize the hashmap if it is very empty */
    if(ns->count * 8 < ns->size && ns->size > 32)
        expand(ns); // this can fail. we just continue with the bigger hashmap.
    return UA_STATUSCODE_GOOD;
}

void
NodeStore_iterate(NodeStore *ns, NodeStore_nodeVisitor visitor) {
    for(UA_UInt32 i = 0; i < ns->size; i++) {
        if(ns->entries[i] > NODESTORE_TOMBSTONE)
            visitor((UA_Node*)&ns->entries[i]->node);
    }
}

#endif /* UA_ENABLE_MULTITHREADING */
