#ifndef __NAMESPACE_H__
#define __NAMESPACE_H__

#include "ua_basictypes.h"
#include "opcua.h"
#include "ua_list.h"

#ifdef MULTITHREADING
#define _XOPEN_SOURCE 500
#define __USE_UNIX98
#include <pthread.h>
typedef struct pthread_rwlock_t Namespace_Lock;
#else
typedef void Namespace_Lock;
#endif

static inline void Namespace_Lock_release(Namespace_Lock * lock) {
#ifdef MULTITHREADING
	pthread_rwlock_unlock((pthread_rwlock_t *) lock);
#endif
}

/* Poor-man's transactions: If we need multiple locks and at least one of them is a writelock
   ("transaction"), a deadlock can be introduced in conjunction with a second thread.

   Convention: All nodes in a transaction (read and write) must be locked before the first write.
   If one write-lock cannot be acquired immediately, bail out and restart the transaction. A
   Namespace_TransactionContext is currently only a linked list of the acquired locks. More advanced
   transaction mechanisms will be established once the runtime behavior can be observed. */
typedef UA_list_List Namespace_TransactionContext;
UA_Int32 Namespace_TransactionContext_init(Namespace_TransactionContext * tc);

/* Each namespace is a hash-map of NodeIds to Nodes. Every entry in the hashmap consists of a
   pointer to a read-write lock and a pointer to the Node. */
typedef struct Namespace_Entry_T {
#ifdef MULTITHREADING
	Namespace_Lock *lock;	/* locks are heap-allocated */
#endif
	UA_Node *node;
} Namespace_Entry;

typedef struct Namespace_T {
	UA_Int32 namespaceId;
	UA_String namespaceUri;
	Namespace_Entry *entries;
	UA_UInt32 size;
	UA_UInt32 count;
	UA_UInt32 sizePrimeIndex;	/* Current size, as an index into the table of primes.  */
} Namespace;

/** @brief Create a new namespace */
UA_Int32 Namespace_create(Namespace ** result, UA_UInt32 size);

/** @brief Delete all nodes in the namespace */
void Namespace_empty(Namespace * ns);

/** @brief Delete the namespace and all nodes in it */
void Namespace_delete(Namespace * ns);

/** @brief Insert a new node into the namespace */
UA_Int32 Namespace_insert(Namespace * ns, UA_Node * node);

/** @brief Remove a node from the namespace */
void Namespace_remove(Namespace * ns, UA_NodeId * nodeid);

/** @brief Retrieve a node (read-only) from the namespace. Nodes are identified
	by their NodeId. After the Node is no longer used, the lock needs to be
	released. */
UA_Int32 Namespace_get(Namespace const *ns, const UA_NodeId * nodeid, UA_Node const **result, Namespace_Lock ** lock);

/** @brief Retrieve a node (read and write) from the namespace. Nodes are
	identified by their NodeId. After the Node is no longer used, the lock needs
	to be released. */
UA_Int32 Namespace_getWritable(Namespace const *ns, const UA_NodeId * nodeid, UA_Node ** result, Namespace_Lock ** lock);

/** @brief Retrieve a node (read-only) as part of a transaction. If multiples
	nodes are to be retrieved as part of a transaction, the transaction context
	needs to be specified. */
UA_Int32 Namespace_transactionGet(Namespace * ns, Namespace_TransactionContext * tc, const UA_NodeId * nodeid, UA_Node ** const result, Namespace_Lock ** lock);

/** @brief Retrieve a node (read and write) as part of a transaction. If
	multiples nodes are to be retrieved as part of a transaction, the
	transaction context needs to be specified. */
UA_Int32 Namespace_transactionGetWritable(Namespace * ns, Namespace_TransactionContext * tc, const UA_NodeId * nodeid, UA_Node ** result, Namespace_Lock ** lock);

typedef void (*Namespace_nodeVisitor) (UA_Node const *node);

/** @brief Iterate over all nodes in a namespace */
UA_Int32 Namespace_iterate(const Namespace * ns, Namespace_nodeVisitor visitor);

#endif /* __NAMESPACE_H */
