#ifndef __NAMESPACE_H__
#define __NAMESPACE_H__

#include "opcua_basictypes.h"
#include "opcua.h"

typedef uint32_t hash_t;

typedef struct namespace_t {
	UA_Int32 namespaceId;
	UA_String namespaceUri;
	UA_Node **entries;
	uint32_t size;
	uint32_t count;
	uint32_t sizePrimeIndex; /* Current size, as an index into the table of primes.  */
} namespace;

UA_Int32 create_ns(namespace **result, uint32_t size);
void empty_ns(namespace *ns);
void delete_ns(namespace *ns);
UA_Int32 insert(namespace *ns, UA_Node *node);
UA_Int32 find(namespace *ns, UA_Node **result, UA_NodeId *nodeid);
void delete(namespace *ns, UA_NodeId *nodeid);

/* Internal */
static hash_t hash_string(const UA_Byte * data, UA_Int32 len);
static hash_t hash(const UA_NodeId *n);

static unsigned int higher_prime_index (unsigned long n);
static inline hash_t mod_1(hash_t x, hash_t y, hash_t inv, int shift);
static inline hash_t mod(hash_t hash, const namespace *ns);
static inline hash_t htab_mod_m2(hash_t hash, const namespace *ns);
static UA_Int32 find_slot(const namespace *ns, UA_Node **slot, UA_NodeId *nodeid);
static UA_Node ** find_empty_slot(const namespace *ns, hash_t hash);
static UA_Int32 expand(namespace *ns);

#endif /* __NAMESPACE_H */
