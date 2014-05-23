#include "ua_namespace.h"
#include <string.h>
#include <stdio.h>

/*****************************************/
/* Internal (not exported) functionality */
/*****************************************/

struct Namespace_Entry {
	UA_UInt64 status;	/* 2 bits status | 14 bits checkout count | 48 bits timestamp */
	const UA_Node *node;	/* Nodes are immutable. It is not recommended to change nodes in place */
};

struct Namespace_Entry_Lock {
	Namespace_Entry *entry;
};

/* The tombstone (entry.node == 0x01) indicates that an entry was deleted at the position in the
   hash-map. This is information is used to decide whether the entire table shall be rehashed so
   that entries are found faster. */
#define ENTRY_EMPTY UA_NULL
#define ENTRY_TOMBSTONE 0x01

/* The central data structure is a hash-map of UA_Node objects. Entry lookup via Algorithm D from
   Knuth's TAOCP (no linked lists here). Table of primes and mod-functions are from libiberty
   (licensed under LGPL) */ typedef UA_UInt32 hash_t;
struct prime_ent {
	hash_t prime;
	hash_t inv;
	hash_t inv_m2;	/* inverse of prime-2 */
	hash_t shift;
};

static struct prime_ent const prime_tab[] = {
	{7, 0x24924925, 0x9999999b, 2},
	{13, 0x3b13b13c, 0x745d1747, 3},
	{31, 0x08421085, 0x1a7b9612, 4},
	{61, 0x0c9714fc, 0x15b1e5f8, 5},
	{127, 0x02040811, 0x0624dd30, 6},
	{251, 0x05197f7e, 0x073260a5, 7},
	{509, 0x01824366, 0x02864fc8, 8},
	{1021, 0x00c0906d, 0x014191f7, 9},
	{2039, 0x0121456f, 0x0161e69e, 10},
	{4093, 0x00300902, 0x00501908, 11},
	{8191, 0x00080041, 0x00180241, 12},
	{16381, 0x000c0091, 0x00140191, 13},
	{32749, 0x002605a5, 0x002a06e6, 14},
	{65521, 0x000f00e2, 0x00110122, 15},
	{131071, 0x00008001, 0x00018003, 16},
	{262139, 0x00014002, 0x0001c004, 17},
	{524287, 0x00002001, 0x00006001, 18},
	{1048573, 0x00003001, 0x00005001, 19},
	{2097143, 0x00004801, 0x00005801, 20},
	{4194301, 0x00000c01, 0x00001401, 21},
	{8388593, 0x00001e01, 0x00002201, 22},
	{16777213, 0x00000301, 0x00000501, 23},
	{33554393, 0x00001381, 0x00001481, 24},
	{67108859, 0x00000141, 0x000001c1, 25},
	{134217689, 0x000004e1, 0x00000521, 26},
	{268435399, 0x00000391, 0x000003b1, 27},
	{536870909, 0x00000019, 0x00000029, 28},
	{1073741789, 0x0000008d, 0x00000095, 29},
	{2147483647, 0x00000003, 0x00000007, 30},
	/* Avoid "decimal constant so large it is unsigned" for 4294967291.  */
	{0xfffffffb, 0x00000006, 0x00000008, 31}
};

/* Hashing inspired by code from from http://www.azillionmonkeys.com/qed/hash.html, licensed under
   the LGPL 2.1 */
#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) || \
	defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((UA_UInt32)(((const uint8_t *)(d))[1])) << 8) + \
					  (UA_UInt32)(((const uint8_t *)(d))[0]) )
#endif

static inline hash_t hash_string(const UA_Byte * data, UA_Int32 len) {
	hash_t hash = len;
	hash_t tmp;
	int rem;

	if(len <= 0 || data == UA_NULL)
		return 0;

	rem = len & 3;
	len >>= 2;

	/* Main loop */
	for(; len > 0; len--) {
		hash += get16bits(data);
		tmp = (get16bits(data + 2) << 11) ^ hash;
		hash = (hash << 16) ^ tmp;
		data += 2 * sizeof(uint16_t);
		hash += hash >> 11;
	}

	/* Handle end cases */
	switch (rem) {
	case 3:
		hash += get16bits(data);
		hash ^= hash << 16;
		hash ^= ((signed char)data[sizeof(uint16_t)]) << 18;
		hash += hash >> 11;
		break;
	case 2:
		hash += get16bits(data);
		hash ^= hash << 11;
		hash += hash >> 17;
		break;
	case 1:
		hash += (signed char)*data;
		hash ^= hash << 10;
		hash += hash >> 1;
		break;
	}

	/* Force "avalanching" of final 127 bits */
	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;

	return hash;
}

static inline hash_t hash(const UA_NodeId * n) {
	switch (n->encodingByte) {
	case UA_NODEIDTYPE_TWOBYTE:
	case UA_NODEIDTYPE_FOURBYTE:
	case UA_NODEIDTYPE_NUMERIC:
		return (n->identifier.numeric * 2654435761) % 2 ^ 32;	// Knuth's multiplicative hashing
	case UA_NODEIDTYPE_STRING:
		return hash_string(n->identifier.string.data, n->identifier.string.length);
	case UA_NODEIDTYPE_GUID:
		return hash_string((UA_Byte *) & (n->identifier.guid), sizeof(UA_Guid));
	case UA_NODEIDTYPE_BYTESTRING:
		return hash_string((UA_Byte *) n->identifier.byteString.data, n->identifier.byteString.length);
	default:
		return 0;
	}
}

/* The following function returns an index into the above table of the nearest prime number which
   is greater than N, and near a power of two. */
static inline unsigned int higher_prime_index(unsigned long n) {
	unsigned int low = 0;
	unsigned int high = sizeof(prime_tab) / sizeof(prime_tab[0]);

	while(low != high) {
		unsigned int mid = low + (high - low) / 2;
		if(n > prime_tab[mid].prime)
			low = mid + 1;
		else
			high = mid;
	}

	return low;
}

static inline hash_t mod_1(hash_t x, hash_t y, hash_t inv, int shift) {
	/* The multiplicative inverses computed above are for 32-bit types, and requires that we are
	   able to compute a highpart multiply.  */
#ifdef UNSIGNED_64BIT_TYPE
	__extension__ typedef UNSIGNED_64BIT_TYPE ull;
	if(sizeof(hash_t) * CHAR_BIT <= 32) {
		hash_t t1, t2, t3, t4, q, r;
		t1 = ((ull) x * inv) >> 32;
		t2 = x - t1;
		t3 = t2 >> 1;
		t4 = t1 + t3;
		q = t4 >> shift;
		r = x - (q * y);
		return r;
	}
#endif
	return x % y;
}

static inline hash_t mod(hash_t h, const Namespace * ns) {
	const struct prime_ent *p = &prime_tab[ns->sizePrimeIndex];
	return mod_1(h, p->prime, p->inv, p->shift);
}

static inline hash_t mod_m2(hash_t h, const Namespace * ns) {
	const struct prime_ent *p = &prime_tab[ns->sizePrimeIndex];
	return 1 + mod_1(h, p->prime - 2, p->inv_m2, p->shift);
}

static inline void clear_entry(Namespace * ns, Namespace_Entry * entry) {
	if(entry->node == UA_NULL)
		return;

	switch (entry->node->nodeClass) {
	case UA_NODECLASS_OBJECT:
		UA_ObjectNode_delete((UA_ObjectNode *) entry->node);
		break;
	case UA_NODECLASS_VARIABLE:
		UA_VariableNode_delete((UA_VariableNode *) entry->node);
		break;
	case UA_NODECLASS_METHOD:
		UA_MethodNode_delete((UA_MethodNode *) entry->node);
		break;
	case UA_NODECLASS_OBJECTTYPE:
		UA_ObjectTypeNode_delete((UA_ObjectTypeNode *) entry->node);
		break;
	case UA_NODECLASS_VARIABLETYPE:
		UA_VariableTypeNode_delete((UA_VariableTypeNode *) entry->node);
		break;
	case UA_NODECLASS_REFERENCETYPE:
		UA_ReferenceTypeNode_delete((UA_ReferenceTypeNode *) entry->node);
		break;
	case UA_NODECLASS_DATATYPE:
		UA_DataTypeNode_delete((UA_DataTypeNode *) entry->node);
		break;
	case UA_NODECLASS_VIEW:
		UA_ViewNode_delete((UA_ViewNode *) entry->node);
		break;
	default:
		break;
	}
	entry->node = UA_NULL;
}

/* Returns UA_SUCCESS if an entry was found. Otherwise, UA_ERROR is returned and the "entry"
   argument points to the first free entry under the NodeId. */
static inline UA_Int32 find_entry(const Namespace * ns, const UA_NodeId * nodeid, Namespace_Entry ** entry) {
	hash_t h = hash(nodeid);
	hash_t index = mod(h, ns);
	UA_UInt32 size = ns->size;
	Namespace_Entry *e = &ns->entries[index];

	if(e->node == UA_NULL) {
		*entry = e;
		return UA_ERROR;
	}

	if(UA_NodeId_compare(&e->node->nodeId, nodeid) == UA_EQUAL) {
		*entry = e;
		return UA_SUCCESS;
	}

	hash_t hash2 = mod_m2(h, ns);
	for(;;) {
		index += hash2;
		if(index >= size)
			index -= size;

		e = &ns->entries[index];

		if(e->node == UA_NULL) {
			*entry = e;
			return UA_ERROR;
		}

		if(UA_NodeId_compare(&e->node->nodeId, nodeid) == UA_EQUAL) {
			*entry = e;
			return UA_SUCCESS;
		}
	}

	/* NOTREACHED */
	return UA_SUCCESS;
}

/* The following function changes size of memory allocated for the entries and repeatedly inserts
   the table elements. The occupancy of the table after the call will be about 50%. Naturally the
   hash table must already exist. Remember also that the place of the table entries is changed. If
   memory allocation failures are allowed, this function will return zero, indicating that the
   table could not be expanded. If all goes well, it will return a non-zero value. */
static UA_Int32 expand(Namespace * ns) {
	Namespace_Entry *nentries;
	int32_t nsize;
	UA_UInt32 nindex;

	Namespace_Entry *oentries = ns->entries;
	int32_t osize = ns->size;
	Namespace_Entry *olimit = &oentries[osize];
	int32_t count = ns->count;

	/* Resize only when table after removal of unused elements is either too full or too empty.  */
	if(count * 2 < osize && (count * 8 > osize || osize <= 32)) {
		return UA_SUCCESS;
	}

	nindex = higher_prime_index(count * 2);
	nsize = prime_tab[nindex].prime;

	if(UA_alloc((void **)&nentries, sizeof(Namespace_Entry) * nsize) != UA_SUCCESS)
		return UA_ERR_NO_MEMORY;
	ns->entries = nentries;
	ns->size = nsize;
	ns->sizePrimeIndex = nindex;

	Namespace_Entry *p = oentries;
	do {
		if(p->node != UA_NULL) {
			Namespace_Entry *e;
			find_entry(ns, &p->node->nodeId, &e);	/* We know this returns an empty entry here */
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

UA_Int32 Namespace_new(Namespace ** result, UA_UInt32 size, UA_UInt32 namespaceId) {
	Namespace *ns;
	if(UA_alloc((void **)&ns, sizeof(Namespace)) != UA_SUCCESS)
		return UA_ERR_NO_MEMORY;

	UA_UInt32 sizePrimeIndex = higher_prime_index(size);
	size = prime_tab[sizePrimeIndex].prime;
	if(UA_alloc((void **)&ns->entries, sizeof(Namespace_Entry) * size) != UA_SUCCESS) {
		UA_free(ns);
		return UA_ERR_NO_MEMORY;
	}

	/* set entries to zero */
	memset(ns->entries, 0, size * sizeof(Namespace_Entry));

	*ns = (Namespace) {
	namespaceId, ns->entries, size, 0, sizePrimeIndex};
	*result = ns;
	return UA_SUCCESS;
}

static void Namespace_clear(Namespace * ns) {
	UA_UInt32 size = ns->size;
	Namespace_Entry *entries = ns->entries;
	for(UA_UInt32 i = 0; i < size; i++)
		clear_entry(ns, &entries[i]);
	ns->count = 0;
}

void Namespace_empty(Namespace * ns) {
	Namespace_clear(ns);

	/* Downsize the table.  */
	if(ns->size > 1024 * 1024 / sizeof(Namespace_Entry)) {
		int nindex = higher_prime_index(1024 / sizeof(Namespace_Entry));
		int nsize = prime_tab[nindex].prime;
		UA_free(ns->entries);
		UA_alloc((void **)&ns->entries, sizeof(Namespace_Entry) * nsize);	// FIXME: Check return
		// value
		ns->size = nsize;
		ns->sizePrimeIndex = nindex;
	}
}

void Namespace_delete(Namespace * ns) {
	Namespace_clear(ns);
	UA_free(ns->entries);
	UA_free(ns);
}

UA_Int32 Namespace_insert(Namespace * ns, const UA_Node * node) {
	if(ns->size * 3 <= ns->count * 4) {
		if(expand(ns) != UA_SUCCESS)
			return UA_ERROR;
	}

	Namespace_Entry *entry;
	UA_Int32 found = find_entry(ns, &node->nodeId, &entry);

	if(found == UA_SUCCESS)
		return UA_ERROR;	/* There is already an entry for that nodeid */

	entry->node = node;
	ns->count++;
	return UA_SUCCESS;
}

UA_Int32 Namespace_insertUnique(Namespace * ns, UA_Node * node) {
	if(ns->size * 3 <= ns->count * 4) {
		if(expand(ns) != UA_SUCCESS)
			return UA_ERROR;
	}
	// find unoccupied numeric nodeid
	node->nodeId.namespace = ns->namespaceId;
	node->nodeId.encodingByte = UA_NODEIDTYPE_NUMERIC;
	node->nodeId.identifier.numeric = ns->count;

	hash_t h = hash(&node->nodeId);
	hash_t hash2 = mod_m2(h, ns);
	UA_UInt32 size = ns->size;

	// advance integer (hash) until a free entry is found
	Namespace_Entry *entry = UA_NULL;
	while(1) {
		if(find_entry(ns, &node->nodeId, &entry) != UA_SUCCESS)
			break;
		node->nodeId.identifier.numeric += hash2;
		if(node->nodeId.identifier.numeric >= size)
			node->nodeId.identifier.numeric -= size;
	}

	entry->node = node;
	ns->count++;
	return UA_SUCCESS;
}

UA_Int32 Namespace_contains(const Namespace * ns, const UA_NodeId * nodeid) {
	Namespace_Entry *entry;
	return find_entry(ns, nodeid, &entry);
}

UA_Int32 Namespace_get(Namespace const *ns, const UA_NodeId * nodeid, const UA_Node **result,
					   Namespace_Entry_Lock ** lock) {
	Namespace_Entry *entry;
	if(find_entry(ns, nodeid, &entry) != UA_SUCCESS)
		return UA_ERROR;

	*result = entry->node;
	return UA_SUCCESS;
}

UA_Int32 Namespace_remove(Namespace * ns, const UA_NodeId * nodeid) {
	Namespace_Entry *entry;
	if(find_entry(ns, nodeid, &entry) != UA_SUCCESS)
		return UA_ERROR;

	// TODO: Check if deleting the node makes the Namespace inconsistent.
	clear_entry(ns, entry);

	/* Downsize the hashmap if it is very empty */
	if(ns->count * 8 < ns->size && ns->size > 32)
		expand(ns);

	return UA_SUCCESS;
}

UA_Int32 Namespace_iterate(const Namespace * ns, Namespace_nodeVisitor visitor) {
	UA_UInt32 i;
	for(i = 0; i < ns->size; i++) {
		Namespace_Entry *entry = &ns->entries[i];
		if(entry != UA_NULL && visitor != UA_NULL)
			visitor(entry->node);
	}
	return UA_SUCCESS;
}

void Namespace_Entry_Lock_release(Namespace_Entry_Lock * lock) {
	;
}
