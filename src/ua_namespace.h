#ifndef __NAMESPACE_H__
#define __NAMESPACE_H__

#include "ua_basictypes.h"
#include "opcua.h"
#include "ua_list.h"

#ifdef MULTITHREADING
#define _XOPEN_SOURCE 500
#define __USE_UNIX98
#include <pthread.h>
#endif

/** @brief Namespace entries point to an UA_Node. But the actual data structure
	is opaque outside of ua_namespace.c */
struct Namespace_Entry;
typedef struct Namespace_Entry Namespace_Entry;

/** @brief Namespace datastructure. It mainly serves as a hashmap to UA_Nodes. */
typedef struct Namespace {
	UA_UInt32 namespaceId;
	Namespace_Entry *entries;
	UA_UInt32 size;
	UA_UInt32 count;
	UA_UInt32 sizePrimeIndex;	/* Current size, as an index into the table of primes.  */
} Namespace;

/** Namespace locks indicate that a thread currently operates on an entry. */
struct Namespace_Entry_Lock;
typedef struct Namespace_Entry_Lock Namespace_Entry_Lock;

/** @brief Release a lock on a namespace entry. */
void Namespace_Entry_Lock_release(Namespace_Entry_Lock * lock);

/** @brief Create a new namespace */
UA_Int32 Namespace_new(Namespace ** result, UA_UInt32 size, UA_UInt32 namespaceId);

/** @brief Delete all nodes in the namespace */
void Namespace_empty(Namespace * ns);

/** @brief Delete the namespace and all nodes in it */
void Namespace_delete(Namespace * ns);

/** @brief Insert a new node into the namespace. Abort an entry with the same
	NodeId is already present */
UA_Int32 Namespace_insert(Namespace * ns, const UA_Node * node);

/** @brief Insert a new node or replace an existing node if an entry has the same NodeId. */
UA_Int32 Namespace_insertOrReplace(Namespace * ns, const UA_Node * node);

/** @brief Find an unused (numeric) NodeId in the namespace and insert the node.
	The node is modified to contain the new nodeid after insertion. */
UA_Int32 Namespace_insertUnique(Namespace * ns, UA_Node * node);

/** @brief Remove a node from the namespace */
UA_Int32 Namespace_remove(Namespace * ns, const UA_NodeId * nodeid);

/** @brief Tests whether the namespace contains an entry for a given NodeId */
UA_Int32 Namespace_contains(const Namespace * ns, const UA_NodeId * nodeid);

/** @brief Retrieve a node (read-only) from the namespace. Nodes are identified
	by their NodeId. After the Node is no longer used, the lock needs to be
	released. */
UA_Int32 Namespace_get(Namespace const *ns, const UA_NodeId * nodeid, UA_Node const **result,
					   Namespace_Entry_Lock ** lock);

/** @brief A function that can be evaluated on all entries in a namespace via Namespace_iterate */
typedef void (*Namespace_nodeVisitor) (UA_Node const *node);

/** @brief Iterate over all nodes in a namespace */
UA_Int32 Namespace_iterate(const Namespace * ns, Namespace_nodeVisitor visitor);

#endif /* __NAMESPACE_H */
