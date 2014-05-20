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

struct Namespace;
typedef struct Namespace Namespace;

struct Namespace_Entry_Lock;
typedef struct Namespace_Entry_Lock Namespace_Entry_Lock;
void Namespace_Entry_Lock_release(Namespace_Entry_Lock * lock);

struct Namespace_Transaction;
typedef struct Namespace_Transaction Namespace_Transaction;

/*************/
/* Namespace */
/*************/

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

typedef void (*Namespace_nodeVisitor) (UA_Node const *node);

/** @brief Iterate over all nodes in a namespace */
UA_Int32 Namespace_iterate(const Namespace * ns, Namespace_nodeVisitor visitor);

/****************/
/* Transactions */
/****************/

/** @brief Create a transaction that operates on a single namespace */
UA_Int32 Namespace_Transaction_new(Namespace * ns, Namespace_Transaction ** result);

/** @brief Insert a new node into the namespace as part of a transaction */
UA_Int32 Namespace_Transaction_enqueueInsert(Namespace_Transaction * t, const UA_Node * node);

/** @brief Insert a new node or replace an existing node as part of a transaction */
UA_Int32 Namespace_Transaction_enqueueInsertOrReplace(Namespace_Transaction * t, const UA_Node * node);

/** @brief Find an unused (numeric) NodeId in the namespace and insert the node as
	part of a transaction */
UA_Int32 Namespace_Transaction_enqueueInsertUnique(Namespace_Transaction * t, UA_Node * node);

/** @brief Remove a node from the namespace as part of a transaction */
UA_Int32 Namespace_Transaction_enqueueRemove(Namespace_Transaction * t, const UA_NodeId * nodeid);

/** @brief Executes a transaction and returns the status */
UA_Int32 Namespace_Transaction_commit(Namespace_Transaction * t);

/** @brief Frees the transaction and deletes all the member objects */
UA_Int32 Namespace_Transaction_delete(Namespace_Transaction * t);

#endif /* __NAMESPACE_H */
