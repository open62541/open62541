#include "opcua_namespace.h"
#include <string.h>
#include <stdio.h>

/* The basic data structure is a hash-map of UA_Node objects.

   Entry lookup via Algorithm D from Knuth's TAOCP (no linked lists here).
   Table of primes and mod-functions are from libiberty (licensed under LGPL) */

struct prime_ent {
	hash_t prime;
	hash_t inv;
	hash_t inv_m2;	/* inverse of prime-2 */
	hash_t shift;
};

static struct prime_ent const prime_tab[] = {
  {          7, 0x24924925, 0x9999999b, 2 },
  {         13, 0x3b13b13c, 0x745d1747, 3 },
  {         31, 0x08421085, 0x1a7b9612, 4 },
  {         61, 0x0c9714fc, 0x15b1e5f8, 5 },
  {        127, 0x02040811, 0x0624dd30, 6 },
  {        251, 0x05197f7e, 0x073260a5, 7 },
  {        509, 0x01824366, 0x02864fc8, 8 },
  {       1021, 0x00c0906d, 0x014191f7, 9 },
  {       2039, 0x0121456f, 0x0161e69e, 10 },
  {       4093, 0x00300902, 0x00501908, 11 },
  {       8191, 0x00080041, 0x00180241, 12 },
  {      16381, 0x000c0091, 0x00140191, 13 },
  {      32749, 0x002605a5, 0x002a06e6, 14 },
  {      65521, 0x000f00e2, 0x00110122, 15 },
  {     131071, 0x00008001, 0x00018003, 16 },
  {     262139, 0x00014002, 0x0001c004, 17 },
  {     524287, 0x00002001, 0x00006001, 18 },
  {    1048573, 0x00003001, 0x00005001, 19 },
  {    2097143, 0x00004801, 0x00005801, 20 },
  {    4194301, 0x00000c01, 0x00001401, 21 },
  {    8388593, 0x00001e01, 0x00002201, 22 },
  {   16777213, 0x00000301, 0x00000501, 23 },
  {   33554393, 0x00001381, 0x00001481, 24 },
  {   67108859, 0x00000141, 0x000001c1, 25 },
  {  134217689, 0x000004e1, 0x00000521, 26 },
  {  268435399, 0x00000391, 0x000003b1, 27 },
  {  536870909, 0x00000019, 0x00000029, 28 },
  { 1073741789, 0x0000008d, 0x00000095, 29 },
  { 2147483647, 0x00000003, 0x00000007, 30 },
  /* Avoid "decimal constant so large it is unsigned" for 4294967291.  */
  { 0xfffffffb, 0x00000006, 0x00000008, 31 }
};

UA_Int32 create_ns(namespace **result, uint32_t size) {
	namespace *ns = UA_NULL;
	uint32_t sizePrimeIndex = higher_prime_index(size);
	size = prime_tab[sizePrimeIndex].prime;

	if (UA_alloc((void **)&ns, sizeof(namespace)) != UA_SUCCESS) return UA_ERR_NO_MEMORY;
  
	if (UA_alloc((void **)&(ns->entries), sizeof(void *)*size) != UA_SUCCESS) {
		UA_free(ns);
		return UA_ERR_NO_MEMORY;
	}
	
	for (uint32_t i=0; i<size;i++)
		ns->entries[i] = UA_NULL;

	ns->size = size;
	ns->count = 0;
	ns->sizePrimeIndex = sizePrimeIndex;
	*result = ns;
	return UA_SUCCESS;
}

void empty_ns(namespace *ns) {
	uint32_t size = ns->size;
	UA_Node **entries = ns->entries;

	for (uint32_t i = 0; i < size; i++) {
		if (entries[i] != UA_NULL) {
			switch(entries[i]->nodeClass) {
			case UA_NODECLASS_OBJECT:
				// UA_ObjectNode_delete((UA_ObjectNode *) entries[i]);
				break;
			case UA_NODECLASS_VARIABLE:
				// UA_VariableNode_delete((UA_VariableNode *) entries[i]);
				break;
			case UA_NODECLASS_METHOD:
				// UA_MethodNode_delete((UA_MethodNode *) entries[i]);
				break;
			case UA_NODECLASS_OBJECTTYPE:
				// UA_ObjectTypeNode_delete((UA_ObjectTypeNode *) entries[i]);
				break;
			case UA_NODECLASS_VARIABLETYPE:
				// UA_VariableTypeNode_delete((UA_VariableTypeNode *) entries[i]);
				break;
			case UA_NODECLASS_REFERENCETYPE:
				// UA_ReferenceTypeNode_delete((UA_ReferenceTypeNode *) entries[i]);
				break;
			case UA_NODECLASS_DATATYPE:
				// UA_DataTypeNode_delete((UA_DataTypeNode *) entries[i]);
				break;
			case UA_NODECLASS_VIEW:
				// UA_ViewNode_delete((UA_ViewNode *) entries[i]);
				break;
			default:
				break; // Unspecified nodes are not permitted.
			}
		}
	}	

	/* Downsize the table.  */
	if (size > 1024*1024 / sizeof (void *)) {
		int nindex = higher_prime_index (1024 / sizeof (void *));
		int nsize = prime_tab[nindex].prime;

		UA_free(ns->entries);
		UA_alloc((void **)&(ns->entries), sizeof(void *)*nsize); //FIXME: Check if memory was available
		ns->size = nsize;
		ns->sizePrimeIndex = nindex;
		ns->count = 0;
	}
	else {
		memset(*(ns->entries), 0, size * sizeof(void *));
		ns->count = 0;
	}
}

/* This function frees all memory allocated for a namespace */
void delete_ns (namespace *ns) {
	uint32_t size = ns->size;
	UA_Node **entries = ns->entries;

	for (uint32_t i = 0; i < size; i++) {
		if (entries[i] != UA_NULL) {
			switch(entries[i]->nodeClass) {
			case UA_NODECLASS_OBJECT:
				// UA_ObjectNode_delete((UA_ObjectNode *) entries[i]);
				break;
			case UA_NODECLASS_VARIABLE:
				// UA_VariableNode_delete((UA_VariableNode *) entries[i]);
				break;
			case UA_NODECLASS_METHOD:
				// UA_MethodNode_delete((UA_MethodNode *) entries[i]);
				break;
			case UA_NODECLASS_OBJECTTYPE:
				// UA_ObjectTypeNode_delete((UA_ObjectTypeNode *) entries[i]);
				break;
			case UA_NODECLASS_VARIABLETYPE:
				// UA_VariableTypeNode_delete((UA_VariableTypeNode *) entries[i]);
				break;
			case UA_NODECLASS_REFERENCETYPE:
				// UA_ReferenceTypeNode_delete((UA_ReferenceTypeNode *) entries[i]);
				break;
			case UA_NODECLASS_DATATYPE:
				// UA_DataTypeNode_delete((UA_DataTypeNode *) entries[i]);
				break;
			case UA_NODECLASS_VIEW:
				// UA_ViewNode_delete((UA_ViewNode *) entries[i]);
				break;
			default:
				break; // Unspecified nodes are not permitted.
			}
		}
	}	

	UA_free(entries);
	UA_free(ns);
}

UA_Int32 insert(namespace *ns, UA_Node *node) {
	if((float)ns->count/(float)ns->size < 0.75) {
		if(expand(ns) != UA_SUCCESS)
			return UA_ERROR;
	}
		
	hash_t h = hash(&(node->nodeId));
	UA_Node **slot = find_empty_slot(ns, h);
	*slot = node;
	return UA_SUCCESS;
}

UA_Int32 find(namespace *ns, UA_Node **result, UA_NodeId *nodeid) {
	UA_Node **slot = UA_NULL;
	if(find_slot(ns, slot, nodeid) == UA_SUCCESS) {
		*result = *slot;
		return UA_SUCCESS;
	}
	return UA_ERROR;
}

void delete(namespace *ns, UA_NodeId *nodeid) {
	UA_Node **slot = UA_NULL;
	if (find_slot(ns, slot, nodeid) != UA_SUCCESS)
		return;

	UA_Node *node = *slot;
	*slot = UA_NULL;

	switch(node->nodeClass) {
	case UA_NODECLASS_OBJECT:
		// UA_ObjectNode_delete((UA_ObjectNode *) node);
		break;
	case UA_NODECLASS_VARIABLE:
		// UA_VariableNode_delete((UA_VariableNode *) node);
		break;
	case UA_NODECLASS_METHOD:
		// UA_MethodNode_delete((UA_MethodNode *) node);
		break;
	case UA_NODECLASS_OBJECTTYPE:
		// UA_ObjectTypeNode_delete((UA_ObjectTypeNode *) node);
		break;
	case UA_NODECLASS_VARIABLETYPE:
		// UA_VariableTypeNode_delete((UA_VariableTypeNode *) node);
		break;
	case UA_NODECLASS_REFERENCETYPE:
		// UA_ReferenceTypeNode_delete((UA_ReferenceTypeNode *) node);
		break;
	case UA_NODECLASS_DATATYPE:
		// UA_DataTypeNode_delete((UA_DataTypeNode *) node);
		break;
	case UA_NODECLASS_VIEW:
		// UA_ViewNode_delete((UA_ViewNode *) node);
		break;
	default:
		break; // Unspecified nodes are not permitted.
	}
}

/* Hashing inspired by code from from http://www.azillionmonkeys.com/qed/hash.html, licensed under the LGPL 2.1 */
#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8) +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif

static hash_t hash_string (const UA_Byte * data, UA_Int32 len) {
	hash_t hash = len;
	hash_t tmp;
	int rem;

    if (len <= 0 || data == UA_NULL) return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--) {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
	case 3: hash += get16bits (data);
		hash ^= hash << 16;
		hash ^= ((signed char)data[sizeof (uint16_t)]) << 18;
		hash += hash >> 11;
		break;
	case 2: hash += get16bits (data);
		hash ^= hash << 11;
		hash += hash >> 17;
		break;
	case 1: hash += (signed char)*data;
		hash ^= hash << 10;
		hash += hash >> 1;
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

static hash_t hash(const UA_NodeId *n) {
	switch (n->encodingByte) {
	case UA_NODEIDTYPE_TWOBYTE:
	case UA_NODEIDTYPE_FOURBYTE:
	case UA_NODEIDTYPE_NUMERIC:
		return n->identifier.numeric;
	case UA_NODEIDTYPE_STRING:
		return hash_string(n->identifier.string.data, n->identifier.string.length);
	case UA_NODEIDTYPE_GUID:
		return hash_string((UA_Byte*) &(n->identifier.guid), sizeof(UA_Guid));
	case UA_NODEIDTYPE_BYTESTRING:
		return hash_string((UA_Byte*) n->identifier.byteString.data, n->identifier.byteString.length);
	default:
		// Not permitted
		return 0;
	}
}

/* The following function returns an index into the above table of the nearest prime number which is greater than N, and near a power of two. */
static unsigned int higher_prime_index (unsigned long n) {
	unsigned int low = 0;
	unsigned int high = sizeof(prime_tab) / sizeof(prime_tab[0]);

	while (low != high) {
		unsigned int mid = low + (high - low) / 2;
		if (n > prime_tab[mid].prime)
			low = mid + 1;
		else
			high = mid;
	}

	// Fixme: If we've run out of primes, abort.
	/* if (n > prime_tab[low].prime) */
	/* 	abort ();  */
  
	return low;
}

/* Return X % Y. */
static inline hash_t mod_1 (hash_t x, hash_t y, hash_t inv, int shift) {
	/* The multiplicative inverses computed above are for 32-bit types, and requires that we are able to compute a highpart multiply.  */
#ifdef UNSIGNED_64BIT_TYPE
	__extension__ typedef UNSIGNED_64BIT_TYPE ull;
	if (sizeof (hash_t) * CHAR_BIT <= 32) {
		hash_t t1, t2, t3, t4, q, r;
		t1 = ((ull)x * inv) >> 32;
		t2 = x - t1;
		t3 = t2 >> 1;
		t4 = t1 + t3;
		q  = t4 >> shift;
		r  = x - (q * y);
		return r;
	}
#endif

	return x % y; /* Otherwise just use the native division routines.  */
}

static inline hash_t mod (hash_t h, const namespace *ns) {
	const struct prime_ent *p = &prime_tab[ns->sizePrimeIndex];
	return mod_1 (h, p->prime, p->inv, p->shift);
}

static inline hash_t mod_m2 (hash_t h, const namespace *ns) {
	const struct prime_ent *p = &prime_tab[ns->sizePrimeIndex];
	return 1 + mod_1 (h, p->prime - 2, p->inv_m2, p->shift);
}

static UA_Int32 find_slot (const namespace *ns, UA_Node **slot, UA_NodeId *nodeid) {
	hash_t h = hash(nodeid);
	hash_t index, hash2;
	uint32_t size;
	UA_Node *entry;

	size = ns->size;
	index = mod(h, ns);

	entry = ns->entries[index];
	if (entry == UA_NULL)
		return UA_ERROR;
	if (UA_NodeId_compare(&(entry->nodeId), nodeid)) {
		*slot = ns->entries[index];
		return UA_SUCCESS;
	}

	hash2 = mod_m2(h, ns);
	for (;;) {
		index += hash2;
		if (index >= size)
			index -= size;

		entry = ns->entries[index];
		if (entry == UA_NULL)
			return UA_ERROR;
		if (UA_NodeId_compare(&(entry->nodeId), nodeid)) {
			slot = &(ns->entries[index]);
			return UA_SUCCESS;
		}
    }
}

/* Always returns an empty slot. This is inevitable if the entries are not
   completely full. */
static UA_Node ** find_empty_slot(const namespace *ns, hash_t h) {
	hash_t index = mod(h, ns);
	uint32_t size = ns->size;
	UA_Node **slot = ns->entries + index;

	if (*slot == UA_NULL)
		return slot;

	hash_t hash2 = mod_m2 (h, ns);
	for (;;) {
		index += hash2;
		if (index >= size)
			index -= size;

		slot = ns->entries + index;
		if (*slot == UA_NULL)
			return slot;
	}
}

/* The following function changes size of memory allocated for the
   entries and repeatedly inserts the table elements.  The occupancy
   of the table after the call will be about 50%.  Naturally the hash
   table must already exist.  Remember also that the place of the
   table entries is changed.  If memory allocation failures are allowed,
   this function will return zero, indicating that the table could not be
   expanded.  If all goes well, it will return a non-zero value.  */
static UA_Int32 expand (namespace *ns) {
	UA_Node **p;
	UA_Node **nentries;
	int32_t nsize;
	uint32_t nindex;

	UA_Node **oentries = ns->entries;
	int32_t osize = ns->size;
	UA_Node **olimit = oentries + osize;
	int32_t count = ns->count;

	/* Resize only when table after removal of unused elements is either too full or too empty.  */
	if (count * 2 > osize || (count * 8 < osize && osize > 32)) {
		nindex = higher_prime_index (count * 2);
		nsize = prime_tab[nindex].prime;
	}
	return UA_SUCCESS;

	if (UA_alloc((void **)nentries, sizeof(void *)*nsize) != UA_SUCCESS)
		return UA_ERR_NO_MEMORY;
	ns->entries = nentries;
	ns->size = nsize;
	ns->sizePrimeIndex = nindex;

	p = oentries;
	do {
		UA_Node *x = *p;

		if (x != UA_NULL) {
			UA_Node **q = find_empty_slot(ns, hash(&(x->nodeId)));
			*q = x;
		}
		p++;
	}
	while (p < olimit);
	
	UA_free(oentries);
	return UA_SUCCESS;
}
