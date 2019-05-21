/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2019 (c) Julian Grothoff, Torben Deppe, Florian Palm
 */

#include <open62541/plugin/nodestore_switch.h>

const UA_Boolean inPlaceEditAllowed = false;
//TODO checks for defaultNS != NULL --> Allow defaultNS to be NULL and return error (or NULL)

void UA_Nodestore_copy(const UA_NodestoreInterface* src, UA_NodestoreInterface* dst){
	dst->context = src->context;
	dst->deleteNode = src->deleteNode;
	dst->deleteNodestore = src->deleteNodestore;
	dst->getNode = src->getNode;
	dst->getNodeCopy = src->getNodeCopy;
	dst->insertNode = src->insertNode;
	dst->iterate = src->iterate;
	dst->newNode = src->newNode;
	dst->releaseNode = src->releaseNode;
	dst->removeNode = src->removeNode;
    dst->replaceNode = src->replaceNode;
}

static size_t findNSHandle(UA_Nodestore_Switch *storeSwitch, void *nsHandle)
{
	size_t i;
	for(i=0; i<storeSwitch->size; i++)
	{
		if(storeSwitch->nodestoreArray[i]->context == nsHandle)
		{
			return i;
		}
	}
	return i;
}

UA_StatusCode UA_Nodestore_Default_Interface_new(UA_NodestoreInterface** nsInterface){
	UA_NodestoreInterface* uaDefaultStore = (UA_NodestoreInterface*)UA_malloc(sizeof(UA_NodestoreInterface));
	if(uaDefaultStore == NULL){
		return UA_STATUSCODE_BADOUTOFMEMORY;
	}

	UA_StatusCode result = UA_Nodestore_Default_new((void**)&uaDefaultStore->context);
	if(result != UA_STATUSCODE_GOOD){
		UA_free(uaDefaultStore);
		return result;
	}
	uaDefaultStore->deleteNodestore = UA_Nodestore_Default_delete;
	uaDefaultStore->newNode = UA_Nodestore_Default_newNode;
	uaDefaultStore->deleteNode = UA_Nodestore_Default_deleteNode;
	uaDefaultStore->getNode = UA_Nodestore_Default_getNode;
	uaDefaultStore->releaseNode = UA_Nodestore_Default_releaseNode;
	uaDefaultStore->getNodeCopy = UA_Nodestore_Default_getNodeCopy;
	uaDefaultStore->insertNode = UA_Nodestore_Default_insertNode;
	uaDefaultStore->replaceNode = UA_Nodestore_Default_replaceNode;
	uaDefaultStore->iterate = UA_Nodestore_Default_iterate;
	uaDefaultStore->removeNode = UA_Nodestore_Default_removeNode;
	*nsInterface = uaDefaultStore;
	return UA_STATUSCODE_GOOD;
}


UA_StatusCode UA_Nodestore_Switch_new(void **nsCtx)
{
	UA_Nodestore_Switch *storeSwitch = (UA_Nodestore_Switch *)UA_malloc(sizeof(UA_Nodestore_Switch));
	if(!storeSwitch)
		return UA_STATUSCODE_BADOUTOFMEMORY;

	UA_StatusCode result = UA_Nodestore_Default_Interface_new(&storeSwitch->defaultNodestore);
	if(result != UA_STATUSCODE_GOOD){
		UA_free(storeSwitch);
		return result;
	}

	storeSwitch->nodestoreArray = NULL;
	storeSwitch->size = 0;
	*nsCtx = (void*)storeSwitch;
	return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Nodestore_Switch_newEmpty(void **nsCtx)
{
	UA_Nodestore_Switch *storeSwitch = (UA_Nodestore_Switch *)UA_malloc(sizeof(UA_Nodestore_Switch));
	if(!storeSwitch)
		return UA_STATUSCODE_BADOUTOFMEMORY;
	storeSwitch->defaultNodestore = NULL;
	storeSwitch->nodestoreArray = NULL;
	storeSwitch->size = 0;
	*nsCtx = (void*)storeSwitch;
	return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Nodestore_Switch_linkDefaultNodestore(UA_Nodestore_Switch *storeSwitch, UA_NodestoreInterface *store)
{
	if(store == NULL)
		return UA_STATUSCODE_BADNOTFOUND;
	storeSwitch->defaultNodestore = store;
	return UA_STATUSCODE_GOOD;
}

void UA_Nodestore_Switch_linkSwitchToStore(UA_Nodestore_Switch *storeSwitch ,UA_NodestoreInterface *store)
{
	store->context = storeSwitch;
	store->deleteNodestore = UA_Nodestore_Switch_deleteNodestores;
	store->newNode = UA_Nodestore_Switch_newNode;
	store->deleteNode = UA_Nodestore_Switch_deleteNode;
	store->getNode = UA_Nodestore_Switch_getNode;
	store->releaseNode = UA_Nodestore_Switch_releaseNode;
	store->getNodeCopy = UA_Nodestore_Switch_getNodeCopy;
	store->insertNode = UA_Nodestore_Switch_insertNode;
	store->replaceNode = UA_Nodestore_Switch_replaceNode;
	store->iterate = UA_Nodestore_Switch_iterate;
	store->removeNode = UA_Nodestore_Switch_removeNode;
}


UA_StatusCode UA_Nodestore_Switch_linkNodestoreToNamespace(UA_Nodestore_Switch *storeSwitch,
		UA_NodestoreInterface *ns, UA_UInt16 namespaceindex)
{
	//TODO make multithreading save --> Lock/Unlock or CRTISECTION
	if(storeSwitch->size <= namespaceindex)
	{
		UA_NodestoreInterface **tmpPointer = (UA_NodestoreInterface **) UA_realloc(
				storeSwitch->nodestoreArray , (size_t)(namespaceindex + 1) * sizeof (UA_NodestoreInterface*));
		if(!tmpPointer)
			return UA_STATUSCODE_BADOUTOFMEMORY;
		storeSwitch->nodestoreArray = tmpPointer;
		for (UA_UInt16 i = storeSwitch->size; i < namespaceindex + 1; i++){
			storeSwitch->nodestoreArray[i] = NULL;
		}
		storeSwitch->size = (UA_UInt16) (namespaceindex + 1);
	}

	storeSwitch->nodestoreArray[namespaceindex] = ns;
	return UA_STATUSCODE_GOOD;

}


UA_StatusCode UA_Nodestore_Switch_unlinkNodestoreFromNamespace(UA_Nodestore_Switch *storeSwitch, UA_NodestoreInterface *ns)
{
	//TODO make multithreading save --> Lock/Unlock or CRTISECTION
	size_t flag = 0;
	if(ns == NULL)
		return UA_STATUSCODE_BADNOTFOUND;
	for(size_t i=0; i< storeSwitch->size; i++)
	{
		if(storeSwitch->nodestoreArray[i] == ns)
		{
			if(flag == 0)
			{
				storeSwitch->nodestoreArray[i]->deleteNodestore(storeSwitch->nodestoreArray[i]->context);
				flag = 1;
			}
			storeSwitch->nodestoreArray[i] = NULL;//UA_free(&storeSwitch->nodestoreArray[i]);
			//TODO resize ns array if ns is last in array?
		}

	}
	return (flag == 1) ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADNOTFOUND;
}

//TODO check and change to replace all ns --> comfortfunction --> change to getNodestore(nsIndex)
//TODO  add getNSIndexes(nsHandle) --> change findNSHandle for all matches
UA_StatusCode UA_Nodestore_Switch_changeNodestore(UA_Nodestore_Switch *storeSwitch, void *nsHandleOut, UA_NodestoreInterface *nsIn) {
	size_t i= findNSHandle(storeSwitch, nsHandleOut);
	if(i == storeSwitch->size)
		return UA_STATUSCODE_BADNOTFOUND;
	if(nsIn == NULL)
		return UA_STATUSCODE_BADINTERNALERROR;
	storeSwitch->nodestoreArray[i] = nsIn;
	return UA_STATUSCODE_GOOD;
}

void UA_Nodestore_Switch_delete(void *storeSwitch)
{
	UA_Nodestore_Switch_deleteNodestores(storeSwitch);
	UA_free(storeSwitch);
}


 /* Functions of nodestore interface to plug in to server */
void UA_Nodestore_Switch_deleteNode(void *storeSwitchHandle, UA_Node *node)
{
	UA_Nodestore_Switch *storeSwitch = (UA_Nodestore_Switch *)storeSwitchHandle;
	if (node->nodeId.namespaceIndex < storeSwitch->size){
		if (storeSwitch->nodestoreArray[node->nodeId.namespaceIndex] != NULL){
			storeSwitch->nodestoreArray[node->nodeId.namespaceIndex]->deleteNode(storeSwitch->nodestoreArray[node->nodeId.namespaceIndex]->context, node);
			return;
		}
	}
	if(storeSwitch->defaultNodestore != NULL){
		storeSwitch->defaultNodestore->deleteNode(storeSwitch->defaultNodestore->context, node);
		return;
	}
}

UA_StatusCode UA_Nodestore_Switch_insertNode(void *storeSwitchHandle, UA_Node *node, UA_NodeId *addedNodeId)
{
	UA_Nodestore_Switch *storeSwitch = (UA_Nodestore_Switch *)storeSwitchHandle;
	if (node->nodeId.namespaceIndex < storeSwitch->size){
		if (storeSwitch->nodestoreArray[node->nodeId.namespaceIndex] != NULL){
			return (storeSwitch->nodestoreArray[node->nodeId.namespaceIndex]->insertNode(storeSwitch->nodestoreArray[node->nodeId.namespaceIndex]->context, node, addedNodeId));
		}
	}
	if(storeSwitch->defaultNodestore != NULL){
		return storeSwitch->defaultNodestore->insertNode(storeSwitch->defaultNodestore->context, node, addedNodeId);
	}
	return UA_STATUSCODE_BADNODEIDINVALID;
}

const UA_Node *UA_Nodestore_Switch_getNode(void *storeSwitchHandle, const UA_NodeId *nodeId)
{
	UA_Nodestore_Switch *storeSwitch = (UA_Nodestore_Switch *)storeSwitchHandle;
	if (nodeId->namespaceIndex < storeSwitch->size){
		if (storeSwitch->nodestoreArray[nodeId->namespaceIndex] != NULL){
			return (storeSwitch->nodestoreArray[nodeId->namespaceIndex]->getNode(storeSwitch->nodestoreArray[nodeId->namespaceIndex]->context,nodeId));
		}
	}

	if(storeSwitch->defaultNodestore != NULL){
		return (const UA_Node*) storeSwitch->defaultNodestore->getNode(storeSwitch->defaultNodestore->context, nodeId);
	}
	return NULL;
}

UA_StatusCode UA_Nodestore_Switch_getNodeCopy(void *storeSwitchHandle, const UA_NodeId *nodeId, UA_Node **outNode)
{
	UA_Nodestore_Switch *storeSwitch = (UA_Nodestore_Switch *)storeSwitchHandle;
	if (nodeId->namespaceIndex < storeSwitch->size){
		if (storeSwitch->nodestoreArray[nodeId->namespaceIndex] != NULL){
			return storeSwitch->nodestoreArray[nodeId->namespaceIndex]->getNodeCopy(storeSwitch->nodestoreArray[nodeId->namespaceIndex]->context, nodeId, outNode);
		}
	}
	if(storeSwitch->defaultNodestore != NULL){
		return storeSwitch->defaultNodestore->getNodeCopy(storeSwitch->defaultNodestore->context, nodeId, outNode);
	}
	return UA_STATUSCODE_BADNODEIDINVALID;
}

UA_StatusCode UA_Nodestore_Switch_replaceNode(void *storeSwitchHandle, UA_Node *node)
{
	UA_Nodestore_Switch *storeSwitch = (UA_Nodestore_Switch *)storeSwitchHandle;
	if (node->nodeId.namespaceIndex < storeSwitch->size){
		if (storeSwitch->nodestoreArray[node->nodeId.namespaceIndex] != NULL){
			return storeSwitch->nodestoreArray[node->nodeId.namespaceIndex]->replaceNode(storeSwitch->nodestoreArray[node->nodeId.namespaceIndex]->context, node);
		}
	}
	if(storeSwitch->defaultNodestore != NULL){
		return storeSwitch->defaultNodestore->replaceNode(storeSwitch->defaultNodestore->context, node);
	}
	return UA_STATUSCODE_BADNODEIDINVALID;
}

UA_StatusCode UA_Nodestore_Switch_removeNode(void *storeSwitchHandle, const UA_NodeId *nodeId)
{
	UA_Nodestore_Switch *storeSwitch = (UA_Nodestore_Switch *)storeSwitchHandle;
	if (nodeId->namespaceIndex < storeSwitch->size){
		if (storeSwitch->nodestoreArray[nodeId->namespaceIndex] != NULL){
			return storeSwitch->nodestoreArray[nodeId->namespaceIndex]->removeNode(storeSwitch->nodestoreArray[nodeId->namespaceIndex]->context, nodeId);
		}
	}
	if(storeSwitch->defaultNodestore != NULL){
		return storeSwitch->defaultNodestore->removeNode(storeSwitch->defaultNodestore->context, nodeId);
	}
	return UA_STATUSCODE_BADNODEIDINVALID;
}

void UA_Nodestore_Switch_releaseNode(void *storeSwitchHandle, const UA_Node *node)
{
	if(node == NULL) return;
	UA_Nodestore_Switch *storeSwitch = (UA_Nodestore_Switch *)storeSwitchHandle;
	if (node->nodeId.namespaceIndex < storeSwitch->size){
		if (storeSwitch->nodestoreArray[node->nodeId.namespaceIndex] != NULL){
			storeSwitch->nodestoreArray[node->nodeId.namespaceIndex]->releaseNode(storeSwitch->nodestoreArray[node->nodeId.namespaceIndex]->context, node);
			return;
		}
	}
	if(storeSwitch->defaultNodestore != NULL){
		storeSwitch->defaultNodestore->releaseNode(storeSwitch->defaultNodestore->context, node);
		return;
	}
}

void UA_Nodestore_Switch_deleteNodestores(void *storeSwitchHandle)
 {
	UA_Nodestore_Switch *storeSwitch = (UA_Nodestore_Switch *)storeSwitchHandle;

	//Delete default nodestore
	if(storeSwitch->defaultNodestore){
		//Check for explicit link to default nodestore and set to NULL
		for(size_t i=0 ; i < storeSwitch->size ; i++){
			//check that nodestore is equal by comparision of interfaces
			if(storeSwitch->nodestoreArray[i] == storeSwitch->defaultNodestore)
				storeSwitch->nodestoreArray[i]= NULL;
		}
		storeSwitch->defaultNodestore->deleteNodestore(storeSwitch->defaultNodestore->context);
		storeSwitch->defaultNodestore = NULL;
	}

 	for(size_t i=0; i< storeSwitch->size; i++)
 	{
 		//if namespace i has custom nodestore
 		if(storeSwitch->nodestoreArray[i]){
 			// search forward for other occurances of nodestore and set to null
 			for(size_t j=i+1 ; j < storeSwitch->size ; j++){
 				//check that nodestore is equal by comparision of interfaces
 				if(storeSwitch->nodestoreArray[j] == storeSwitch->nodestoreArray[i])
 					storeSwitch->nodestoreArray[j]= NULL;
 			}
 			//delete the nodestore
 			storeSwitch->nodestoreArray[i]->deleteNodestore(storeSwitch->nodestoreArray[i]->context);
 			storeSwitch->nodestoreArray[i] = NULL;
 		}
 	}
 }


  UA_Node *UA_Nodestore_Switch_newNode(void *storeSwitchHandle, UA_NodeClass nodeClass)
  {
	//todo: NOT IMPLEMENTED Cause of, not clear in which nodestore the memory has to be allocated. (storeSwitch or defaultNodestore)
 	UA_Nodestore_Switch *storeSwitch = (UA_Nodestore_Switch *)storeSwitchHandle;
 	return storeSwitch->defaultNodestore->newNode(storeSwitch->defaultNodestore->context, nodeClass);
	//  return NULL;
  }

  void UA_Nodestore_Switch_iterate(void *nsCtx, UA_NodestoreVisitor visitor,
          void *visitorCtx)
  {
  	UA_Nodestore_Switch *storeSwitch = (UA_Nodestore_Switch *)nsCtx;
  	UA_NodestoreInterface *tempArray[storeSwitch->size];
  	for(size_t i=0; i<storeSwitch->size; i++)
  	{
  		for(size_t j=0; j<=i; j++)
  		{
  			if(tempArray[j] == storeSwitch->nodestoreArray[i])
  				break;
  			else if(j==i)
  			{
  				tempArray[i] = storeSwitch->nodestoreArray[i];
  				storeSwitch->nodestoreArray[i]->iterate(storeSwitch, visitor, visitorCtx);
  			}
  		}
  	}

  }
