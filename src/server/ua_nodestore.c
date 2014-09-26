#include "ua_nodestore.h"
#include "ua_util.h"

struct UA_NodeStore {
    const UA_Node **entries;
    UA_UInt32       size;
    UA_UInt32       count;
    UA_UInt32       sizePrimeIndex;
};

typedef UA_UInt32 hash_t;
/* The size of the hash-map is always a prime number. They are chosen to be
   close to the next power of 2. So the size ca. doubles with each prime. */
static hash_t const primes[] = {
    7,         13,         31,         61,         127,         251,
    509,       1021,       2039,       4093,       8191,        16381,
    32749,     65521,      131071,     262139,     524287,      1048573,
    2097143,   4194301,    8388593,    16777213,   33554393,    67108859,
    134217689, 268435399,  536870909,  1073741789, 2147483647,  4294967291
};

static INLINE hash_t mod(hash_t h, hash_t size) {
    return h % size;
}
static INLINE hash_t mod2(hash_t h, hash_t size) {
    return 1 + (h % (size - 2));
}

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

/* Based on Murmur-Hash 3 by Austin Appleby (public domain, freely usable) */
static INLINE hash_t hash_array(const UA_Byte *data, UA_UInt32 len, UA_UInt32 seed) {
    const int32_t   nblocks = len / 4;
    const uint32_t *blocks;

    static const uint32_t c1 = 0xcc9e2d51;
    static const uint32_t c2 = 0x1b873593;
    static const uint32_t r1 = 15;
    static const uint32_t r2 = 13;
    static const uint32_t m  = 5;
    static const uint32_t n  = 0xe6546b64;
    hash_t hash = seed;

    if(data == UA_NULL)
        return 0;

    blocks = (const uint32_t *)data;
    for(int32_t i = 0;i < nblocks;i++) {
        uint32_t k = blocks[i];
        k    *= c1;
        k     = (k << r1) | (k >> (32 - r1));
        k    *= c2;
        hash ^= k;
        hash  = ((hash << r2) | (hash >> (32 - r2))) * m + n;
    }

    const uint8_t *tail = (const uint8_t *)(data + nblocks * 4);
    uint32_t       k1   = 0;

    switch(len & 3) {
    case 3:
        k1 ^= tail[2] << 16;

    case 2:
        k1 ^= tail[1] << 8;

    case 1:
        k1   ^= tail[0];
        k1   *= c1;
        k1    = (k1 << r1) | (k1 >> (32 - r1));
        k1   *= c2;
        hash ^= k1;
    }

    hash ^= len;
    hash ^= (hash >> 16);
    hash *= 0x85ebca6b;
    hash ^= (hash >> 13);
    hash *= 0xc2b2ae35;
    hash ^= (hash >> 16);

    return hash;
}

static INLINE hash_t hash(const UA_NodeId *n) {
    switch(n->identifierType) {
    case UA_NODEIDTYPE_NUMERIC:
        /*  Knuth's multiplicative hashing */
        return (n->identifier.numeric + n->namespaceIndex) * 2654435761;   // mod(2^32) is implicit

    case UA_NODEIDTYPE_STRING:
        return hash_array(n->identifier.string.data, n->identifier.string.length, n->namespaceIndex);

    case UA_NODEIDTYPE_GUID:
        return hash_array((UA_Byte *)&(n->identifier.guid), sizeof(UA_Guid), n->namespaceIndex);

    case UA_NODEIDTYPE_BYTESTRING:
        return hash_array((UA_Byte *)n->identifier.byteString.data, n->identifier.byteString.length, n->namespaceIndex);

    default:
        UA_assert(UA_FALSE);
        return 0;
    }
}

static INLINE void clear_entry(UA_NodeStore *ns, const UA_Node **entry) {
    const UA_Node *node;
    if(entry == UA_NULL || *entry == UA_NULL)
        return;

    node = *entry;
    switch(node->nodeClass) {
    case UA_NODECLASS_OBJECT:
        UA_ObjectNode_delete((UA_ObjectNode *)node);
        break;

    case UA_NODECLASS_VARIABLE:
        UA_VariableNode_delete((UA_VariableNode *)node);
        break;

    case UA_NODECLASS_METHOD:
        UA_MethodNode_delete((UA_MethodNode *)node);
        break;

    case UA_NODECLASS_OBJECTTYPE:
        UA_ObjectTypeNode_delete((UA_ObjectTypeNode *)node);
        break;

    case UA_NODECLASS_VARIABLETYPE:
        UA_VariableTypeNode_delete((UA_VariableTypeNode *)node);
        break;

    case UA_NODECLASS_REFERENCETYPE:
        UA_ReferenceTypeNode_delete((UA_ReferenceTypeNode *)node);
        break;

    case UA_NODECLASS_DATATYPE:
        UA_DataTypeNode_delete((UA_DataTypeNode *)node);
        break;

    case UA_NODECLASS_VIEW:
        UA_ViewNode_delete((UA_ViewNode *)node);
        break;

    default:
        UA_assert(UA_FALSE);
        break;
    }
    entry = UA_NULL;
    ns->count--;
}

/* Returns UA_SUCCESS if an entry was found. Otherwise, UA_ERROR is returned and the "entry"
   argument points to the first free entry under the NodeId. */
static INLINE UA_Int32 find_entry(const UA_NodeStore *ns, const UA_NodeId *nodeid, const UA_Node ***entry) {
    hash_t          h     = hash(nodeid);
    UA_UInt32       size  = ns->size;
    hash_t          index = mod(h, size);
    const UA_Node **e     = &ns->entries[index];

    if(*e == UA_NULL) {
        *entry = e;
        return UA_ERROR;
    }

    if(UA_NodeId_equal(&(*e)->nodeId, nodeid) == UA_EQUAL) {
        *entry = e;
        return UA_SUCCESS;
    }

    hash_t hash2 = mod2(h, size);
    for(;;) {
        index += hash2;
        if(index >= size)
            index -= size;

        e = &ns->entries[index];

        if(*e == UA_NULL) {
            *entry = e;
            return UA_ERROR;
        }

        if(UA_NodeId_equal(&(*e)->nodeId, nodeid) == UA_EQUAL) {
            *entry = e;
            return UA_SUCCESS;
        }
    }

    /* NOTREACHED */
    return UA_SUCCESS;
}

/* The following function changes size of memory allocated for the entries and
   repeatedly inserts the table elements. The occupancy of the table after the
   call will be about 50%. If memory allocation failures occur, this function
   will return UA_ERROR. */
static UA_Int32 expand(UA_NodeStore *ns) {
    const UA_Node **nentries;
    int32_t nsize;
    UA_UInt32       nindex;

    const UA_Node **oentries = ns->entries;
    int32_t osize = ns->size;
    const UA_Node **olimit   = &oentries[osize];
    int32_t count = ns->count;

    /* Resize only when table after removal of unused elements is either too full or too empty.  */
    if(count * 2 < osize && (count * 8 > osize || osize <= 32))
        return UA_SUCCESS;

    nindex = higher_prime_index(count * 2);
    nsize  = primes[nindex];

    if(UA_alloc((void **)&nentries, sizeof(UA_Node *) * nsize) != UA_SUCCESS)
        return UA_ERR_NO_MEMORY;

    memset(nentries, 0, nsize * sizeof(UA_Node *));
    ns->entries = nentries;
    ns->size    = nsize;
    ns->sizePrimeIndex = nindex;

    const UA_Node **p = oentries;
    do {
        if(*p != UA_NULL) {
            const UA_Node **e;
            find_entry(ns, &(*p)->nodeId, &e);  /* We know this returns an empty entry here */
            *e = *p;
        }
        p++;
    } while(p < olimit);

    UA_free(oentries);
    return UA_SUCCESS;
}

/**********************/
/* Exported functions */
/**********************/

UA_Int32 UA_NodeStore_new(UA_NodeStore **result) {
    UA_NodeStore *ns;
    UA_UInt32     sizePrimeIndex, size;
    if(UA_alloc((void **)&ns, sizeof(UA_NodeStore)) != UA_SUCCESS)
        return UA_ERR_NO_MEMORY;

    sizePrimeIndex = higher_prime_index(32);
    size = primes[sizePrimeIndex];
    if(UA_alloc((void **)&ns->entries, sizeof(UA_Node *) * size) != UA_SUCCESS) {
        UA_free(ns);
        return UA_ERR_NO_MEMORY;
    }

    /* set entries to zero */
    memset(ns->entries, 0, size * sizeof(UA_Node *));

    *ns     = (UA_NodeStore) {ns->entries, size, 0, sizePrimeIndex };
    *result = ns;
    return UA_SUCCESS;
}

UA_Int32 UA_NodeStore_delete(UA_NodeStore *ns) {
    UA_UInt32       size    = ns->size;
    const UA_Node **entries = ns->entries;

    for(UA_UInt32 i = 0;i < size;i++)
        clear_entry(ns, &entries[i]);

    UA_free(ns->entries);
    UA_free(ns);
    return UA_SUCCESS;
}

UA_Int32 UA_NodeStore_insert(UA_NodeStore *ns, UA_Node **node, UA_Byte flags) {
    if(ns == UA_NULL || node == UA_NULL || *node == UA_NULL)
        return UA_ERROR;

    if(ns->size * 3 <= ns->count * 4) {
        if(expand(ns) != UA_SUCCESS)
            return UA_ERROR;
    }

    const UA_Node **entry;
    UA_Int32 found = find_entry(ns, &(*node)->nodeId, &entry);

    if(flags & UA_NODESTORE_INSERT_UNIQUE) {
        if(found == UA_SUCCESS)
            return UA_ERROR;    /* There is already an entry for that nodeid */
        else
            *entry = *node;
    } else {
        if(found == UA_SUCCESS)
            clear_entry(ns, entry);
        *entry = *node;
    }

    if(!(flags & UA_NODESTORE_INSERT_GETMANAGED))
        *node = UA_NULL;

    ns->count++;
    return UA_SUCCESS;
}

UA_Int32 UA_NodeStore_get(const UA_NodeStore *ns, const UA_NodeId *nodeid, const UA_Node **managedNode) {
    const UA_Node **entry;
    if(ns == UA_NULL || nodeid == UA_NULL || managedNode == UA_NULL)
        return UA_ERROR;

    if(find_entry(ns, nodeid, &entry) != UA_SUCCESS)
        return UA_ERROR;

    *managedNode = *entry;
    return UA_SUCCESS;
}

UA_Int32 UA_NodeStore_remove(UA_NodeStore *ns, const UA_NodeId *nodeid) {
    const UA_Node **entry;
    if(find_entry(ns, nodeid, &entry) != UA_SUCCESS)
        return UA_ERROR;

    // Check before if deleting the node makes the UA_NodeStore inconsistent.
    clear_entry(ns, entry);

    /* Downsize the hashmap if it is very empty */
    if(ns->count * 8 < ns->size && ns->size > 32)
        expand(ns);

    return UA_SUCCESS;
}

UA_Int32 UA_NodeStore_iterate(const UA_NodeStore *ns, UA_NodeStore_nodeVisitor visitor) {
    if(ns == UA_NULL || visitor == UA_NULL)
        return UA_ERROR;

    for(UA_UInt32 i = 0;i < ns->size;i++) {
        const UA_Node *node = ns->entries[i];
        if(node != UA_NULL)
            visitor(node);
    }
    return UA_SUCCESS;
}

void UA_NodeStore_releaseManagedNode(const UA_Node *managed) {
    ;
}
