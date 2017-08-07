/* This Source Code Form is subject to the terms of the Mozilla Public
*  License, v. 2.0. If a copy of the MPL was not distributed with this
*  file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_namespace.h"
#include "ua_types_generated_handling.h"

void UA_Namespace_init(UA_Namespace * namespacePtr, const UA_String * namespaceUri){
    namespacePtr->dataTypesSize = 0;
    namespacePtr->dataTypes = NULL;
    namespacePtr->nodestore = NULL;
    namespacePtr->index = UA_NAMESPACE_UNDEFINED;
    UA_String_copy(namespaceUri, &namespacePtr->uri);
}

UA_Namespace* UA_Namespace_new(const UA_String * namespaceUri){
    UA_Namespace* ns = (UA_Namespace*)UA_malloc(sizeof(UA_Namespace));
    UA_Namespace_init(ns,namespaceUri);
    return ns;
}

UA_Namespace* UA_Namespace_newFromChar(const char * namespaceUri){
    // Override const attribute to get string (dirty hack) /
    UA_String nameString;
    nameString.length = (size_t) strlen(namespaceUri);
    nameString.data = (UA_Byte*)(uintptr_t)namespaceUri;
    return UA_Namespace_new(&nameString);
}


void UA_Namespace_deleteMembers(UA_Namespace* namespacePtr){
    if(namespacePtr->nodestore){
        namespacePtr->nodestore->deleteNodestore(
                namespacePtr->nodestore->handle, namespacePtr->index);
    }
    UA_String_deleteMembers(&namespacePtr->uri);
}

void UA_Namespace_updateDataTypes(UA_Namespace * namespaceToUpdate,
                                  UA_Namespace * namespaceNewDataTypes, UA_UInt16 newNamespaceIndex){
    if(namespaceNewDataTypes && namespaceNewDataTypes->dataTypes){
        //replace data types
        namespaceToUpdate->dataTypesSize = 0;
        namespaceToUpdate->dataTypes = namespaceNewDataTypes->dataTypes;
        namespaceToUpdate->dataTypesSize = namespaceNewDataTypes->dataTypesSize;
    }
    // update indices in dataTypes
    for(size_t j = 0; j < namespaceToUpdate->dataTypesSize; j++){
        namespaceToUpdate->dataTypes[j].typeId.namespaceIndex = newNamespaceIndex;
        //TODO Only update namespaceIndex if equal to old index (namespaceToUpdate->index)?
        //TODO maybe better a pointer for data types namespaceIndex -> Only one pointer update needed.
    }
}

static UA_UInt16 checkUpdateNodeNeccessary(UA_UInt16 nodeIdx, size_t* newNsIndices, size_t newNsIndicesSize) {
    if(nodeIdx < (UA_UInt16) newNsIndicesSize)
        return (UA_UInt16)newNsIndices[(size_t)nodeIdx];
    else
        return UA_NAMESPACE_UNDEFINED;
}

static void updateNodeNamespaceIndices(UA_Node * node, UA_UInt16 newNodeIdx, size_t* newNsIndices, size_t newNsIndicesSize) {
    /* Update NodeId */
	node->nodeId.namespaceIndex = newNodeIdx;

    /* Update Qualified name */
    UA_UInt16* nodeIdx = &node->browseName.namespaceIndex;
    if(*nodeIdx < (UA_UInt16) newNsIndicesSize)
        *nodeIdx =(UA_UInt16) newNsIndices[(size_t)*nodeIdx];
    else
        *nodeIdx = UA_NAMESPACE_UNDEFINED;

    /* Update References */
    for(size_t i = 0 ; i < node->referencesSize ; ++i) {
    	UA_NodeReferenceKind *refs = &node->references[i];
    	//Update namespace index of reference type ids
    	nodeIdx = &refs->referenceTypeId.namespaceIndex;
    	if(*nodeIdx < (UA_UInt16) newNsIndicesSize)
    		*nodeIdx = (UA_UInt16) newNsIndices[(size_t)*nodeIdx];
    	//update namespace index of target ids
    	for(size_t j = 0; j < refs->targetIdsSize; ++j) {
    		//TODO Support expanded NodeIds. check if expanded nodeId in own server ?
    		/* Currently no expandednodeids are allowed */
    		if(refs->targetIds[j].namespaceUri.length == 0){
    			nodeIdx = &refs->targetIds[j].nodeId.namespaceIndex;
    			if(*nodeIdx < (UA_UInt16) newNsIndicesSize)
    				*nodeIdx = (UA_UInt16) newNsIndices[(size_t)*nodeIdx];
    			else
    				*nodeIdx = UA_NAMESPACE_UNDEFINED;
    		}
    	}
    }
}

struct nodeListEntry{
    struct nodeListEntry* next;
    const UA_Node* node;
};

struct updateNodeNamespaceIndexHandle{
    UA_NodestoreInterface* nsi;
    struct nodeListEntry*  nextNode;
    UA_StatusCode result;
} ;

static void updateNodeNamespaceIndexCallback(struct updateNodeNamespaceIndexHandle * handle, const UA_Node * node){
    //printf("getnode: ns=%i, i=%i, name=%.*s\n",node->nodeId.namespaceIndex, node->nodeId.identifier.numeric,
    //        (int)node->browseName.name.length, node->browseName.name.data);
    if(handle->result != UA_STATUSCODE_GOOD){
        return;
    }
    handle->nextNode->node = handle->nsi->getNode(handle->nsi->handle, &node->nodeId);

    //set pointer to node in listentry and set list pointer one step further
    handle->nextNode->next = (struct nodeListEntry*)UA_malloc(sizeof(struct nodeListEntry));
    if(handle->nextNode->next){
        handle->nextNode->next->node = NULL;
        handle->nextNode->next->next = NULL;
        handle->nextNode = handle->nextNode->next;
    }else{
        handle->result = UA_STATUSCODE_BADOUTOFMEMORY;
    }
}

static void updateNodestoreNamespaceIndex(UA_NodestoreInterface* nodestore,
                                                size_t* newNsIndices, size_t newNsIndicesSize) {
    struct nodeListEntry* nodeEntry = (struct nodeListEntry*)UA_malloc(sizeof(struct nodeListEntry));
    nodeEntry->node = NULL;
    nodeEntry->next = NULL;

    struct updateNodeNamespaceIndexHandle handle;
    handle.nsi = nodestore;
    handle.nextNode = nodeEntry;
    handle.result = UA_STATUSCODE_GOOD;

    nodestore->iterate(nodestore->handle, &handle,
                       (UA_NodestoreInterface_nodeVisitor)updateNodeNamespaceIndexCallback);
    while(nodeEntry) {
        if(nodeEntry->node){
        	UA_NodeId nodeId;
        	UA_NodeId_copy(&nodeEntry->node->nodeId, &nodeId);
        	nodestore->releaseNode(nodestore->handle, nodeEntry->node);

        	UA_UInt16 newNsIdxNode = checkUpdateNodeNeccessary(nodeId.namespaceIndex, newNsIndices, newNsIndicesSize);
        	if(newNsIdxNode != UA_NAMESPACE_UNDEFINED){
        		UA_Node* copy = nodestore->getNodeCopy(nodestore->handle, &nodeId);
        		updateNodeNamespaceIndices(copy ,newNsIdxNode, newNsIndices, newNsIndicesSize);

        		/* Check if updating or remove and insert*/
        		if(UA_NodeId_equal(&nodeId, &copy->nodeId)){
        			if(nodestore->replaceNode(nodestore->handle, copy) != UA_STATUSCODE_GOOD){
        				nodestore->deleteNode(copy);
            			nodestore->removeNode(nodestore->handle, &nodeId);
        			}
        		}else{
        			nodestore->removeNode(nodestore->handle, &nodeId);
        			if(nodestore->insertNode(nodestore->handle, copy, &nodeId) == UA_STATUSCODE_GOOD){
        				if(UA_NodeId_equal(&nodeId, &copy->nodeId)){
        					//TODO Error? Update all references ? --> Define a strategy to handle this
        				}
        			}else{
        				nodestore->deleteNode(copy);
        			}
        		}
        	}else{
    			nodestore->removeNode(nodestore->handle, &nodeId);
        	}
    		UA_NodeId_deleteMembers(&nodeId);
        }
        struct nodeListEntry * next = nodeEntry->next;
        UA_free(nodeEntry);
        nodeEntry = next;
    }
}

void UA_Namespace_changeNodestore(UA_Namespace * namespacesToUpdate,
                                  UA_Namespace * namespaceNewNodestore,
                                  UA_NodestoreInterface * defaultNodestore,
                                  UA_UInt16 newIdx){
    /* Delete existing nodestore and set new nodestore if needed*/
    if(namespaceNewNodestore && namespaceNewNodestore->nodestore){
    	/* Is there already a nodestore in this namespace ?*/
    	if(namespacesToUpdate->nodestore){
    		/* Is the nodestore in this namespace not the same as the one we are trying to assign?*/
    		if (namespacesToUpdate->nodestore != namespaceNewNodestore->nodestore) {
    			namespacesToUpdate->nodestore->deleteNodestore(
    					namespacesToUpdate->nodestore->handle, namespacesToUpdate->index);
    			namespacesToUpdate->nodestore = namespaceNewNodestore->nodestore;
    			namespacesToUpdate->nodestore->linkNamespace(namespacesToUpdate->nodestore->handle, newIdx);
    		}
    	}else{
    		namespacesToUpdate->nodestore = namespaceNewNodestore->nodestore;
    		namespacesToUpdate->nodestore->linkNamespace(namespacesToUpdate->nodestore->handle, newIdx);
    	}
    }
    if(!namespacesToUpdate->nodestore){
        //Set default nodestore if no nodestore is present in the namespace
        namespacesToUpdate->nodestore = defaultNodestore;
        namespacesToUpdate->nodestore->linkNamespace(namespacesToUpdate->nodestore->handle, newIdx);
    }

}


void UA_Namespace_updateNodestores(UA_Namespace * namespacesToUpdate, size_t namespacesToUpdateSize,
                                   size_t* oldNsIdxToNewNsIdx, size_t oldNsIdxToNewNsIdxSize) {
    /* Update namespace indices in all relevant nodestores */
    for(size_t i = 0; i < namespacesToUpdateSize ; ++i){
        UA_NodestoreInterface * ns = namespacesToUpdate[i].nodestore;
        if(ns){
            //TODO Add a flag to nodestore if nodes should be automatically updated on namespace changes
            //check if same nodestore is used for different namespaces and update only once
            UA_Boolean nodestoreAlreadyUpdated = UA_FALSE;
            for(size_t j = 0; j < i ; ++j){
                if(namespacesToUpdate[j].nodestore == ns){
                    nodestoreAlreadyUpdated = UA_TRUE;
                    break;
                }
            }
            if(nodestoreAlreadyUpdated == UA_FALSE){
                updateNodestoreNamespaceIndex(ns,
                    oldNsIdxToNewNsIdx,oldNsIdxToNewNsIdxSize);
            }
            //If Index changed link and unlink nodestores
            if((UA_UInt16)oldNsIdxToNewNsIdx[(size_t)namespacesToUpdate->index] != namespacesToUpdate->index){
                ns->linkNamespace(ns->handle, (UA_UInt16)oldNsIdxToNewNsIdx[(size_t)namespacesToUpdate->index]);
                ns->unlinkNamespace(ns->handle, namespacesToUpdate->index);
            }
        }
    }
}
