#include "ua_namespace.h"
#include "ua_list.h"

/**************************/
/* Internal Functionality */
/**************************/

enum NAMESPACE_TRANSACTION_ACTIONTYPE_enum {
	NAMESPACE_TRANSACTION_ACTIONTYPE_INSERT = 0x00,
	NAMESPACE_TRANSACTION_ACTIONTYPE_INSERTREPLACE = 0x01,
	NAMESPACE_TRANSACTION_ACTIONTYPE_INSERTUNIQUE = 0x02,
	NAMESPACE_TRANSACTION_ACTIONTYPE_REMOVE = 0x03,
};

typedef struct Namespace_Transaction_Action {
	UA_SLIST_ENTRY(Namespace_Transaction_Action) next;	// next entry in the list
	union {
		UA_Node *insert;
		const UA_NodeId *remove;
	} obj;
	UA_Byte action;	// from NAMESPACE_TRANSACTION_ACTIONTYPE_enum
} Namespace_Transaction_Action;

struct Namespace_Transaction {
	UA_SLIST_HEAD(Namespace_Transaction_Actions, Namespace_Transaction_Action) actions;
	Namespace *ns;
};

/**********************/
/* Exported Functions */
/**********************/

UA_Int32 Namespace_Transaction_new(Namespace * ns, Namespace_Transaction ** result) {
	Namespace_Transaction *new_ts;
	if(UA_alloc((void **)&new_ts, sizeof(Namespace_Transaction)) != UA_SUCCESS)
		return UA_ERROR;

	new_ts->ns = ns;
	UA_SLIST_INIT(&new_ts->actions);
	*result = new_ts;
	return UA_SUCCESS;
}

UA_Int32 Namespace_Transaction_enqueueInsert(Namespace_Transaction * t, const UA_Node * node) {
	Namespace_Transaction_Action *new_action;
	UA_alloc((void **)&new_action, sizeof(Namespace_Transaction_Action));
	new_action->obj.insert = (UA_Node *)node;
	new_action->action = NAMESPACE_TRANSACTION_ACTIONTYPE_INSERT;
	UA_SLIST_INSERT_HEAD(&t->actions, new_action, next);
	return UA_SUCCESS;
}

UA_Int32 Namespace_Transaction_enqueueInsertOrReplace(Namespace_Transaction * t, const UA_Node * node) {
	Namespace_Transaction_Action *new_action;
	UA_alloc((void **)&new_action, sizeof(Namespace_Transaction_Action));
	new_action->obj.insert = (UA_Node *)node;
	new_action->action = NAMESPACE_TRANSACTION_ACTIONTYPE_INSERTREPLACE;
	UA_SLIST_INSERT_HEAD(&t->actions, new_action, next);
	return UA_SUCCESS;
}

UA_Int32 Namespace_Transaction_enqueueInsertUnique(Namespace_Transaction * t, UA_Node * node) {
	Namespace_Transaction_Action *new_action;
	UA_alloc((void **)&new_action, sizeof(Namespace_Transaction_Action));
	new_action->obj.insert = node;
	new_action->action = NAMESPACE_TRANSACTION_ACTIONTYPE_INSERTUNIQUE;
	UA_SLIST_INSERT_HEAD(&t->actions, new_action, next);
	return UA_SUCCESS;
}

UA_Int32 Namespace_Transaction_enqueueRemove(Namespace_Transaction * t, const UA_NodeId * nodeid) {
	Namespace_Transaction_Action *new_action;
	UA_alloc((void **)&new_action, sizeof(Namespace_Transaction_Action));
	new_action->obj.remove = nodeid;
	new_action->action = NAMESPACE_TRANSACTION_ACTIONTYPE_REMOVE;
	UA_SLIST_INSERT_HEAD(&t->actions, new_action, next);
	return UA_SUCCESS;
}

UA_Int32 Namespace_Transaction_commit(Namespace_Transaction * t) {
	return UA_ERROR; // TODO not yet implemented
}

UA_Int32 Namespace_Transaction_delete(Namespace_Transaction * t) {
	return UA_SUCCESS; // TODO
}
