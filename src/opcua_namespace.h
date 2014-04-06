#ifndef __NAMESPACE_H__
#define __NAMESPACE_H__

/* Defines needed for pthread_rwlock_t */
#define _XOPEN_SOURCE 500

#include "opcua_basictypes.h"
#include "opcua.h"
#include <pthread.h>

typedef uint32_t hash_t;
typedef struct pthread_rwlock_t ns_lock;

typedef struct ns_entry_t {
	ns_lock *lock; /* locks are heap-allocated, so we can resize the entry-array online */
	UA_Node *node;
} ns_entry;

typedef struct namespace_t {
	UA_Int32 namespaceId;
	UA_String namespaceUri;
	ns_entry *entries;
	uint32_t size;
	uint32_t count;
	uint32_t sizePrimeIndex; /* Current size, as an index into the table of primes.  */
} namespace;

UA_Int32 create_ns(namespace **result, uint32_t size);
void empty_ns(namespace *ns);
void delete_ns(namespace *ns);
UA_Int32 insert_node(namespace *ns, UA_Node *node);
UA_Int32 get_node(namespace *ns, UA_NodeId *nodeid, UA_Node ** const result, ns_lock ** lock);
UA_Int32 get_writable_node(namespace *ns, UA_NodeId *nodeid, UA_Node **result, ns_lock ** lock);
inline void unlock_node(ns_lock *lock);
void delete_node(namespace *ns, UA_NodeId *nodeid);

/* Internal */
static hash_t hash_string(const UA_Byte * data, UA_Int32 len);
static hash_t hash(const UA_NodeId *n);

static unsigned int higher_prime_index (unsigned long n);
static inline hash_t mod_1(hash_t x, hash_t y, hash_t inv, int shift);
static inline hash_t mod(hash_t hash, const namespace *ns);
static inline hash_t htab_mod_m2(hash_t hash, const namespace *ns);
static inline void clear_slot(namespace *ns, ns_entry *slot);
static void clear_ns(namespace *ns);
static UA_Int32 find_slot(const namespace *ns, ns_entry **slot, UA_NodeId *nodeid);
static ns_entry * find_empty_slot(const namespace *ns, hash_t hash);
static UA_Int32 expand(namespace *ns);

#endif /* __NAMESPACE_H */
