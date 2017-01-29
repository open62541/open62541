#include "ua_namespace.h"
#include "ua_types_generated_handling.h"

void UA_Namespace_init(UA_Namespace * namespace, const UA_String * namespaceUri){
    namespace->dataTypesSize = 0;
    namespace->dataTypes = NULL;
    namespace->nodestore = NULL;
    namespace->index = UA_NAMESPACE_UNDEFINED;
    UA_String_copy(namespaceUri, &namespace->uri);
}

UA_Namespace* UA_Namespace_new(const UA_String * namespaceUri){
    UA_Namespace* ns = UA_malloc(sizeof(UA_Namespace));
    UA_Namespace_init(ns,namespaceUri);
    return ns;
}

UA_Namespace* UA_Namespace_newFromChar(const char * namespaceUri){
    // Override const attribute to get string (dirty hack) /
    const UA_String nameString = {.length = strlen(namespaceUri),
                                  .data = (UA_Byte*)(uintptr_t)namespaceUri};
    return UA_Namespace_new(&nameString);
}


void UA_Namespace_deleteMembers(UA_Namespace* namespace){
    if(namespace->nodestore){
        namespace->nodestore->deleteNodestore(
                namespace->nodestore->handle, namespace->index);
    }
    UA_String_deleteMembers(&namespace->uri);
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

static UA_Boolean updateNodeNamespaceIndices(UA_Node * node, size_t* newNsIndices, size_t newNsIndicesSize) {
    /* Update NodeId */
    UA_UInt16* nodeIdx = &node->nodeId.namespaceIndex;
    //if(*nodeIdx == 0 && &node->nodeId.identifierType == UA_NODEIDTYPE_NUMERIC && &node->nodeId.identifier.numeric == 2255)
    //return UA_FALSE;
    if(*nodeIdx < (UA_UInt16) newNsIndicesSize){
        UA_UInt16 newNodeIdx = (UA_UInt16)newNsIndices[(size_t)*nodeIdx];
        if(newNodeIdx == UA_NAMESPACE_UNDEFINED)
            return UA_FALSE;
        *nodeIdx = newNodeIdx;
    }
    else
        return UA_FALSE;

    /* Update Qualified name */
    nodeIdx = &node->browseName.namespaceIndex;
    if(*nodeIdx < (UA_UInt16) newNsIndicesSize)
        *nodeIdx =(UA_UInt16) newNsIndices[(size_t)*nodeIdx];
    else
        *nodeIdx = UA_NAMESPACE_UNDEFINED;

    /* Update References */
    for(size_t i = 0 ; i < node->referencesSize ; ++i){
        nodeIdx = &node->references[i].referenceTypeId.namespaceIndex;
        if(*nodeIdx < (UA_UInt16) newNsIndicesSize)
            *nodeIdx = (UA_UInt16) newNsIndices[(size_t)*nodeIdx];
        //TODO Support expanded NodeIds. check if expanded nodeId in own server ?
        /* Currently no expandednodeids are allowed */
        if(node->references[i].targetId.namespaceUri.length == 0){
            nodeIdx = &node->references[i].targetId.nodeId.namespaceIndex;
            if(*nodeIdx < (UA_UInt16) newNsIndicesSize)
                *nodeIdx = (UA_UInt16) newNsIndices[(size_t)*nodeIdx];
            else
                *nodeIdx = UA_NAMESPACE_UNDEFINED;
        }
    }
    return UA_TRUE;
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
    handle->nextNode->next = UA_malloc(sizeof(struct nodeListEntry));
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
    struct nodeListEntry* nodeEntry = UA_malloc(sizeof(struct nodeListEntry));
    nodeEntry->node = NULL;
    nodeEntry->next = NULL;

    struct updateNodeNamespaceIndexHandle handle = {
        .nsi =  nodestore,
        .nextNode = nodeEntry,
        .result = UA_STATUSCODE_GOOD
    };
    nodestore->iterate(nodestore->handle, &handle,
                       (UA_NodestoreInterface_nodeVisitor)updateNodeNamespaceIndexCallback);
    while(nodeEntry) {
        if(nodeEntry->node){
            UA_Node* copy = nodestore->getNodeCopy(nodestore->handle, &nodeEntry->node->nodeId);
            nodestore->removeNode(nodestore->handle, &nodeEntry->node->nodeId);
            nodestore->releaseNode(nodestore->handle, nodeEntry->node);
            if(copy){
                if(updateNodeNamespaceIndices(copy, newNsIndices, newNsIndicesSize) == UA_TRUE){
                    //printf("insertnode: ns=%i, i=%i, name=%.*s\n",copy->nodeId.namespaceIndex, copy->nodeId.identifier.numeric,
                    //        (int)copy->browseName.name.length, copy->browseName.name.data);
                    if(nodestore->insertNode(nodestore->handle, copy, NULL) != UA_STATUSCODE_GOOD){
                        nodestore->deleteNode(copy);
                    }
                }else{
                    nodestore->deleteNode(copy);
                }
            }
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
    /* Delete existing nodestore and set new nodestore */
    if(namespaceNewNodestore && namespaceNewNodestore->nodestore){
       if(namespacesToUpdate->nodestore){
           namespacesToUpdate->nodestore->deleteNodestore(
                namespacesToUpdate->nodestore->handle, namespacesToUpdate->index);
       }
       namespacesToUpdate->nodestore = namespaceNewNodestore->nodestore;
    }
    if(!namespacesToUpdate->nodestore){
        //Set default nodestore if not already set
        namespacesToUpdate->nodestore = defaultNodestore;

    }
    namespacesToUpdate->nodestore->linkNamespace(namespacesToUpdate->nodestore->handle, newIdx);
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
            //If Index changed link and unlink
            if((UA_UInt16)oldNsIdxToNewNsIdx[(size_t)namespacesToUpdate->index] != namespacesToUpdate->index){
                ns->linkNamespace(ns->handle, (UA_UInt16)oldNsIdxToNewNsIdx[(size_t)namespacesToUpdate->index]);
                ns->unlinkNamespace(ns->handle, namespacesToUpdate->index);
            }
        }
    }
}
