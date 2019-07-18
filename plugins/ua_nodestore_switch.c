/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2019 (c) Julian Grothoff, Torben Deppe, Florian Palm
 */

#include <open62541/plugin/nodestore_switch.h>

const UA_Boolean inPlaceEditAllowed = UA_FALSE;

//TODO make multithreading save --> Lock/Unlock or CRTISECTION

/*
 * Definition of a switch based on the namespace id
 */
struct UA_Nodestore_Switch {
	UA_UInt16 size;
	UA_NodestoreInterface *defaultStore; // Nodestore for all namespaces, that have a NULL nodestore interface (and for i>=size)
	UA_NodestoreInterface **stores; 		// Array of all nodestore interfaces
	UA_NodestoreInterface *nsi; // Interface to nodestore switch itself //TODO make const?
};

/*
 * Switch API: linking and unlinking of namespace index to store
 */
UA_NodestoreInterface* UA_Nodestore_Switch_getNodestore(
		UA_Nodestore_Switch* storeSwitch, UA_UInt16 index,
		UA_Boolean useDefault) {
	if (index < storeSwitch->size && storeSwitch->stores[index] != NULL) {
		return storeSwitch->stores[index];
	}
	if (useDefault && storeSwitch->defaultStore != NULL)
		return storeSwitch->defaultStore;
	return NULL;
}

UA_StatusCode UA_Nodestore_Switch_setNodestore(UA_Nodestore_Switch* storeSwitch,
		UA_UInt16 index, UA_NodestoreInterface* store) {
	UA_UInt16 newSize = (UA_UInt16) (index + 1);
	// calculate new size if last nodestore is unlinked
	if (store == NULL && index == (storeSwitch->size - 1)) {
		// check all interface links before last nodestore
		for (newSize = (UA_UInt16) (storeSwitch->size - 1); newSize > 0;
				--newSize) {
			if (storeSwitch->stores[newSize - 1] != NULL)
				break;
		}
	}

	// resize array if neccessary
	if (newSize != storeSwitch->size) {
		UA_NodestoreInterface **newNsArray =
				(UA_NodestoreInterface **) UA_realloc(storeSwitch->stores,
						newSize * sizeof(UA_NodestoreInterface*));
		if (!newNsArray && newSize != 0)
			return UA_STATUSCODE_BADOUTOFMEMORY;
		storeSwitch->stores = newNsArray;
		// fill new array entries with NULL to use the default nodestore
		for (UA_UInt16 i = storeSwitch->size; i < newSize ; ++i) {
			storeSwitch->stores[i] = NULL;
		}
		storeSwitch->size = newSize;
	}

	// set nodestore
	if (index < storeSwitch->size)
		storeSwitch->stores[index] = store;
	return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Nodestore_Switch_setNodestoreDefault(
		UA_Nodestore_Switch* storeSwitch, UA_NodestoreInterface* store) {
	storeSwitch->defaultStore = store;
	return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Nodestore_Switch_getIndices(UA_Nodestore_Switch* storeSwitch,
		UA_NodestoreInterface* store, UA_UInt16* count, UA_UInt16** indices) {
	// Count occurances
	UA_UInt16 found = 0;
	for (UA_UInt16 i = 0; i < storeSwitch->size; i++) {
		if (storeSwitch->stores[i] == store)
			++found;
	}
	// Create an array of for indices
	if (indices != NULL) {
		*indices = (UA_UInt16*) UA_malloc(sizeof(UA_Int16) * found);
		if (*indices != NULL) {
			found = 0;
			for (UA_UInt16 i = 0; i < storeSwitch->size; i++) {
				if (storeSwitch->stores[i] == store)
					*indices[found++] = i;
			}
		} else
			return UA_STATUSCODE_BADOUTOFMEMORY;
	}
	if (count)
		*count = found;
	return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Nodestore_Switch_changeNodestore(
		UA_Nodestore_Switch* storeSwitch, UA_NodestoreInterface *storeOld,
		UA_NodestoreInterface *storeNew) {
	if (storeOld == NULL)
		return UA_STATUSCODE_GOOD;
	for (UA_UInt16 i = 0; i < storeSwitch->size - 1; i++) {
		if (storeSwitch->stores[i] == storeOld)
			storeSwitch->stores[i] = storeNew;
	}
	// resize stores array if last store will be unlinked
	if (storeSwitch->stores[storeSwitch->size - 1] == storeOld) {
		UA_Nodestore_Switch_setNodestore(storeSwitch,
				(UA_UInt16) (storeSwitch->size - 1), storeNew);
	}
	return UA_STATUSCODE_GOOD;
}

/*
 * Switch API: Get an interface of the default nodestore or a switch
 */

UA_StatusCode UA_Nodestore_Default_Interface_new(UA_NodestoreInterface** store) {
	UA_NodestoreInterface* defaultStore = (UA_NodestoreInterface*) UA_malloc(
			sizeof(UA_NodestoreInterface));
	if (defaultStore == NULL) {
		return UA_STATUSCODE_BADOUTOFMEMORY;
	}

	UA_StatusCode result = UA_Nodestore_Default_new(
			(void**) &defaultStore->context);
	if (result != UA_STATUSCODE_GOOD) {
		UA_free(defaultStore);
		return result;
	}
	defaultStore->deleteNodestore = UA_Nodestore_Default_delete;
	defaultStore->newNode = UA_Nodestore_Default_newNode;
	defaultStore->deleteNode = UA_Nodestore_Default_deleteNode;
	defaultStore->getNode = UA_Nodestore_Default_getNode;
	defaultStore->releaseNode = UA_Nodestore_Default_releaseNode;
	defaultStore->getNodeCopy = UA_Nodestore_Default_getNodeCopy;
	defaultStore->insertNode = UA_Nodestore_Default_insertNode;
	defaultStore->replaceNode = UA_Nodestore_Default_replaceNode;
	defaultStore->iterate = UA_Nodestore_Default_iterate;
	defaultStore->removeNode = UA_Nodestore_Default_removeNode;
	*store = defaultStore;
	return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Nodestore_Xml_Interface_new(UA_NodestoreInterface** store, const FileHandler* fileHandler) {
	UA_NodestoreInterface* xmlStore = (UA_NodestoreInterface*) UA_malloc(
			sizeof(UA_NodestoreInterface));
	if (xmlStore == NULL) {
		return UA_STATUSCODE_BADOUTOFMEMORY;
	}

	UA_StatusCode result = UA_Nodestore_Xml_new(
			(void**) &xmlStore->context, fileHandler);
	if (result != UA_STATUSCODE_GOOD) {
		UA_free(xmlStore);
		return result;
	}
	xmlStore->deleteNodestore = UA_Nodestore_Xml_delete;
	xmlStore->newNode = UA_Nodestore_Xml_newNode;
	xmlStore->deleteNode = UA_Nodestore_Xml_deleteNode;
	xmlStore->getNode = UA_Nodestore_Xml_getNode;
	xmlStore->releaseNode = UA_Nodestore_Xml_releaseNode;
	xmlStore->getNodeCopy = UA_Nodestore_Xml_getNodeCopy;
	xmlStore->insertNode = UA_Nodestore_Xml_insertNode;
	xmlStore->replaceNode = UA_Nodestore_Xml_replaceNode;
	xmlStore->iterate = UA_Nodestore_Xml_iterate;
	xmlStore->removeNode = UA_Nodestore_Xml_removeNode;
	*store = xmlStore;
	return UA_STATUSCODE_GOOD;
}

static UA_NodestoreInterface* UA_Nodestore_Switch_Interface_new(
		UA_Nodestore_Switch *storeSwitch) {
	UA_NodestoreInterface* ns = (UA_NodestoreInterface*) UA_malloc(
			sizeof(UA_NodestoreInterface));
	if (ns == NULL) {
		return NULL;
	}
	ns->context = storeSwitch;
	ns->deleteNodestore = UA_Nodestore_Switch_delete;
	ns->newNode = UA_Nodestore_Switch_newNode;
	ns->deleteNode = UA_Nodestore_Switch_deleteNode;
	ns->getNode = UA_Nodestore_Switch_getNode;
	ns->releaseNode = UA_Nodestore_Switch_releaseNode;
	ns->getNodeCopy = UA_Nodestore_Switch_getNodeCopy;
	ns->insertNode = UA_Nodestore_Switch_insertNode;
	ns->replaceNode = UA_Nodestore_Switch_replaceNode;
	ns->iterate = UA_Nodestore_Switch_iterate;
	ns->removeNode = UA_Nodestore_Switch_removeNode;
	return ns;
}

UA_NodestoreInterface*
UA_Nodestore_Switch_Interface_get(UA_Nodestore_Switch *storeSwitch) {
	return storeSwitch->nsi;
}

/*
 * Nodestore switch life cycle
 */

UA_StatusCode UA_Nodestore_Switch_newEmpty(void **nsCtx) {
	UA_Nodestore_Switch *storeSwitch = (UA_Nodestore_Switch *) UA_malloc(
			sizeof(UA_Nodestore_Switch));
	if (!storeSwitch)
		return UA_STATUSCODE_BADOUTOFMEMORY;
	storeSwitch->nsi = UA_Nodestore_Switch_Interface_new(storeSwitch);
	if (!storeSwitch->nsi) {
		UA_free(storeSwitch);
		return UA_STATUSCODE_BADOUTOFMEMORY;
	}
	storeSwitch->defaultStore = NULL;
	storeSwitch->stores = NULL;
	storeSwitch->size = 0;
	*nsCtx = (void*) storeSwitch;
	return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Nodestore_Switch_new(void **nsCtx) {
	UA_StatusCode result = UA_Nodestore_Switch_newEmpty(nsCtx);
	if (result != UA_STATUSCODE_GOOD)
		return result;
	result = UA_Nodestore_Default_Interface_new(
			&((UA_Nodestore_Switch*)*nsCtx)->defaultStore);
	if (result != UA_STATUSCODE_GOOD) {
		UA_free(*nsCtx);
	}
	return result;
}

void UA_Nodestore_Switch_delete(void *nsCtx) {
	UA_Nodestore_Switch *storeSwitch = (UA_Nodestore_Switch *) nsCtx;
	// delete custom nodestores
	for (size_t i = 0; i < storeSwitch->size; i++) {
		// if namespace i has custom nodestore
		if (storeSwitch->stores[i]) {
			//check that custom nodestore is not equal to default nodestore
			if (storeSwitch->stores[i] == storeSwitch->defaultStore) {
				storeSwitch->stores[i] = NULL;
				continue;
			}
			// search forward for other occurances of nodestore and set to null
			for (size_t j = i + 1; j < storeSwitch->size; j++) {
				// check that nodestore is equal by comparision of interfaces
				if (storeSwitch->stores[j] == storeSwitch->stores[i])
					storeSwitch->stores[j] = NULL;
			}
			// delete the nodestore
			storeSwitch->stores[i]->deleteNodestore(
					storeSwitch->stores[i]->context);
			UA_free(storeSwitch->stores[i]);
			storeSwitch->stores[i] = NULL;
		}
	}
	// delete default nodestore
	if (storeSwitch->defaultStore) {
		storeSwitch->defaultStore->deleteNodestore(
				storeSwitch->defaultStore->context);
		UA_free(storeSwitch->defaultStore);
		storeSwitch->defaultStore = NULL;
	}

	// delete nodestore switch itself
	UA_free(storeSwitch->stores);
	storeSwitch->stores = NULL;
	UA_free(storeSwitch->nsi);
	UA_free(storeSwitch);
}

/*
 * Functions of nodestore interface that are plugged into the server and do the actual switch
 */

UA_Node *UA_Nodestore_Switch_newNode(void *nsCtx, UA_NodeClass nodeClass) {
	//TODO: Not clear in which nodestore the memory has to be allocated. (storeSwitch or defaultNodestore)
	UA_Nodestore_Switch *storeSwitch = (UA_Nodestore_Switch *) nsCtx;
	return storeSwitch->defaultStore->newNode(
			storeSwitch->defaultStore->context, nodeClass);
}

void UA_Nodestore_Switch_deleteNode(void *nsCtx, UA_Node *node) {
	UA_NodestoreInterface * nsi = UA_Nodestore_Switch_getNodestore((UA_Nodestore_Switch *) nsCtx,
			node->nodeId.namespaceIndex, UA_TRUE);
	if (nsi != NULL) {
		nsi->deleteNode(nsi->context, node);
		return;
	}
}

const UA_Node *UA_Nodestore_Switch_getNode(void *nsCtx, const UA_NodeId *nodeId) {
	UA_NodestoreInterface * nsi = UA_Nodestore_Switch_getNodestore((UA_Nodestore_Switch *) nsCtx,
			nodeId->namespaceIndex, UA_TRUE);
	if (nsi != NULL) {
		return (nsi->getNode(nsi->context, nodeId));
	}
	return NULL;
}

void UA_Nodestore_Switch_releaseNode(void *nsCtx, const UA_Node *node) {
	if (node == NULL) //TODO check wether this should lead to an error?
		return;
	UA_NodestoreInterface * nsi = UA_Nodestore_Switch_getNodestore((UA_Nodestore_Switch *) nsCtx,
			node->nodeId.namespaceIndex, UA_TRUE);
	if (nsi != NULL) {
		nsi->releaseNode(nsi->context, node);
		return;
	}
}

UA_StatusCode UA_Nodestore_Switch_getNodeCopy(void *nsCtx,
		const UA_NodeId *nodeId, UA_Node **outNode) {
	UA_NodestoreInterface * nsi = UA_Nodestore_Switch_getNodestore((UA_Nodestore_Switch *) nsCtx,
			nodeId->namespaceIndex, UA_TRUE);
	if (nsi != NULL)
		return nsi->getNodeCopy(nsi->context, nodeId, outNode);
	return UA_STATUSCODE_BADNODEIDUNKNOWN;
}

UA_StatusCode UA_Nodestore_Switch_insertNode(void *nsCtx, UA_Node *node,
		UA_NodeId *addedNodeId) {
	UA_NodestoreInterface * nsi = UA_Nodestore_Switch_getNodestore((UA_Nodestore_Switch *) nsCtx,
			node->nodeId.namespaceIndex, UA_TRUE);
	if (nsi != NULL)
		return (nsi->insertNode(nsi->context, node, addedNodeId));
	return UA_STATUSCODE_BADNODEIDUNKNOWN;
}

UA_StatusCode UA_Nodestore_Switch_replaceNode(void *nsCtx, UA_Node *node) {
	UA_NodestoreInterface * nsi = UA_Nodestore_Switch_getNodestore((UA_Nodestore_Switch *) nsCtx,
			node->nodeId.namespaceIndex, UA_TRUE);
	if (nsi != NULL)
		return nsi->replaceNode(nsi->context, node);
	return UA_STATUSCODE_BADNODEIDUNKNOWN; //TODO check if BADNODEIDUNKNOWN
}

UA_StatusCode UA_Nodestore_Switch_removeNode(void *nsCtx,
		const UA_NodeId *nodeId) {
	UA_NodestoreInterface * nsi = UA_Nodestore_Switch_getNodestore((UA_Nodestore_Switch *) nsCtx,
			nodeId->namespaceIndex, UA_TRUE);
	if (nsi != NULL)
		return nsi->removeNode(nsi->context, nodeId);
	return UA_STATUSCODE_BADNODEIDUNKNOWN;
}

void UA_Nodestore_Switch_iterate(void *nsCtx, UA_NodestoreVisitor visitor,
		void *visitorCtx) {
	UA_Nodestore_Switch *storeSwitch = (UA_Nodestore_Switch *) nsCtx;
	UA_NodestoreInterface *tempArray[storeSwitch->size];
	for (size_t i = 0; i < storeSwitch->size; i++) {
		for (size_t j = 0; j <= i; j++) {
			if (tempArray[j] == storeSwitch->stores[i])
				break;
			else if (j == i) {
				tempArray[i] = storeSwitch->stores[i];
				storeSwitch->stores[i]->iterate(storeSwitch, visitor,
						visitorCtx);
			}
		}
	}
}
