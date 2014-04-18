#ifndef __NAMESPACE_H__
#define __NAMESPACE_H__

#include "ua_basictypes.h"
#include "opcua.h"
#include "ua_list.h"

#ifdef MULTITHREADING
#define _XOPEN_SOURCE 500
#define __USE_UNIX98
#include <pthread.h>
typedef struct pthread_rwlock_t ns_lock;
#else
typedef void ns_lock;
#endif

/* Poor-man's transactions: If we need multiple locks and at least one of them
   is a writelock ("transaction"), a deadlock can be introduced in conjunction
   with a second thread.

   Convention: All nodes in a transaction (read and write) must be locked before
   the first write. If one write-lock cannot be acquired immediately, bail out
   and restart the transaction.

   A transaction_context is currently only a linked list of the acquired locks.

   More advanced transaction mechanisms will be established once the runtime
   behavior can be observed. */
typedef UA_list_List transaction_context;
UA_Int32 init_tc(transaction_context * tc);

/* Each namespace is a hash-map of NodeIds to Nodes. Every entry in the hashmap
   consists of a pointer to a read-write lock and a pointer to the Node. */
typedef struct ns_entry_t {
#ifdef MULTITHREADING
	ns_lock *lock; /* locks are heap-allocated, so we can resize the entry-array online */
#endif
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
UA_Int32 get_node(namespace const *ns, const UA_NodeId *nodeid, UA_Node const ** result, ns_lock ** lock);
UA_Int32 get_writable_node(namespace const *ns, const UA_NodeId *nodeid, UA_Node **result, ns_lock ** lock); // use only for _single_ writes.
UA_Int32 get_tc_node(namespace *ns, transaction_context *tc, const UA_NodeId *nodeid, UA_Node ** const result, ns_lock ** lock);
UA_Int32 get_tc_writable_node(namespace *ns, transaction_context *tc, const UA_NodeId *nodeid, UA_Node **result, ns_lock ** lock); // use only for _single_ writes.

// inline void release_node(ns_lock *lock);
// portable solution, see http://www.greenend.org.uk/rjk/tech/inline.html
static inline void release_node(ns_lock *lock) {
#ifdef MULTITHREADING
	pthread_rwlock_unlock((pthread_rwlock_t *)lock);
#endif
}
void delete_node(namespace *ns, UA_NodeId *nodeid);

/* Internal */
typedef uint32_t hash_t;
static hash_t hash_string(const UA_Byte * data, UA_Int32 len);
static hash_t hash(const UA_NodeId *n);

static unsigned int higher_prime_index (unsigned long n);
static inline hash_t mod_1(hash_t x, hash_t y, hash_t inv, int shift);
static inline hash_t mod(hash_t hash, const namespace *ns);
static inline hash_t htab_mod_m2(hash_t hash, const namespace *ns);
static inline void clear_slot(namespace *ns, ns_entry *slot);
static void clear_ns(namespace *ns);
static UA_Int32 find_slot(const namespace *ns, ns_entry **slot, const UA_NodeId *nodeid);
static ns_entry * find_empty_slot(const namespace *ns, hash_t hash);
static UA_Int32 expand(namespace *ns);

/* We store UA_MethodNode_Callback instead of UA_MethodNode in the namespace.
   Pointer casting to UA_MethodNode is possible since pointers point to the
   first element anyway. */
typedef struct UA_MethodNode_Callback_T {
	UA_MethodNode *method_node;
	UA_Int32 (*method_callback)(UA_list_List *input_args, UA_list_List *output_args);
} UA_MethodNode_Callback;

#endif /* __NAMESPACE_H */
