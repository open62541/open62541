/*
 * open62541_nodestore_nodemanagement.c
 *
 *  Created on: Oct 27, 2014
 *      Author: opcua
 */
#include "ua_nodestoreExample.h"
#include "../ua_services.h"
#include "open62541_nodestore.h"
#include "ua_namespace_0.h"
#include "ua_util.h"
/*

    // ReferenceType Ids
    UA_ExpandedNodeId RefTypeId_References; NS0EXPANDEDNODEID(RefTypeId_References, 31);
    UA_ExpandedNodeId RefTypeId_NonHierarchicalReferences; NS0EXPANDEDNODEID(RefTypeId_NonHierarchicalReferences, 32);
    UA_ExpandedNodeId RefTypeId_HierarchicalReferences; NS0EXPANDEDNODEID(RefTypeId_HierarchicalReferences, 33);
    UA_ExpandedNodeId RefTypeId_HasChild; NS0EXPANDEDNODEID(RefTypeId_HasChild, 34);
    UA_ExpandedNodeId RefTypeId_Organizes; NS0EXPANDEDNODEID(RefTypeId_Organizes, 35);
    UA_ExpandedNodeId RefTypeId_HasEventSource; NS0EXPANDEDNODEID(RefTypeId_HasEventSource, 36);
    UA_ExpandedNodeId RefTypeId_HasModellingRule; NS0EXPANDEDNODEID(RefTypeId_HasModellingRule, 37);
    UA_ExpandedNodeId RefTypeId_HasEncoding; NS0EXPANDEDNODEID(RefTypeId_HasEncoding, 38);
    UA_ExpandedNodeId RefTypeId_HasDescription; NS0EXPANDEDNODEID(RefTypeId_HasDescription, 39);
    UA_ExpandedNodeId RefTypeId_HasTypeDefinition; NS0EXPANDEDNODEID(RefTypeId_HasTypeDefinition, 40);
    UA_ExpandedNodeId RefTypeId_GeneratesEvent; NS0EXPANDEDNODEID(RefTypeId_GeneratesEvent, 41);
    UA_ExpandedNodeId RefTypeId_Aggregates; NS0EXPANDEDNODEID(RefTypeId_Aggregates, 44);
    UA_ExpandedNodeId RefTypeId_HasSubtype; NS0EXPANDEDNODEID(RefTypeId_HasSubtype, 45);
    UA_ExpandedNodeId RefTypeId_HasProperty; NS0EXPANDEDNODEID(RefTypeId_HasProperty, 46);
    UA_ExpandedNodeId RefTypeId_HasComponent; NS0EXPANDEDNODEID(RefTypeId_HasComponent, 47);
    UA_ExpandedNodeId RefTypeId_HasNotifier; NS0EXPANDEDNODEID(RefTypeId_HasNotifier, 48);
    UA_ExpandedNodeId RefTypeId_HasOrderedComponent; NS0EXPANDEDNODEID(RefTypeId_HasOrderedComponent, 49);
    UA_ExpandedNodeId RefTypeId_HasModelParent; NS0EXPANDEDNODEID(RefTypeId_HasModelParent, 50);
    UA_ExpandedNodeId RefTypeId_FromState; NS0EXPANDEDNODEID(RefTypeId_FromState, 51);
    UA_ExpandedNodeId RefTypeId_ToState; NS0EXPANDEDNODEID(RefTypeId_ToState, 52);
    UA_ExpandedNodeId RefTypeId_HasCause; NS0EXPANDEDNODEID(RefTypeId_HasCause, 53);
    UA_ExpandedNodeId RefTypeId_HasEffect; NS0EXPANDEDNODEID(RefTypeId_HasEffect, 54);
    UA_ExpandedNodeId RefTypeId_HasHistoricalConfiguration; NS0EXPANDEDNODEID(RefTypeId_HasHistoricalConfiguration, 56);

*/
UA_Int32 open62541NodeStore_addReferences(UA_AddReferencesItem* referencesToAdd,
		UA_UInt32 *indices,UA_UInt32 indicesSize, UA_StatusCode *addReferencesResults,
		UA_DiagnosticInfo *diagnosticInfos)
{

	for(UA_UInt32 i = 0;i<indicesSize;i++){
		UA_Node *node = UA_NULL;
		UA_NodeStoreExample *ns = Nodestore_get();
		UA_NodeStoreExample_get((const UA_NodeStoreExample*)ns,(const UA_NodeId*)&referencesToAdd[indices[i]].sourceNodeId, (const UA_Node**)&node);
		if(node == UA_NULL){
			addReferencesResults[indices[i]] = UA_STATUSCODE_BADSOURCENODEIDINVALID;
			continue;
		}
	    // TODO: Check if reference already exists
	    UA_Int32 count = node->referencesSize;
	    UA_ReferenceNode *old_refs = node->references;
	    UA_ReferenceNode *new_refs;
	    if(count < 0) count = 0;

	    if(!(new_refs = UA_alloc(sizeof(UA_ReferenceNode)*(count+1))))
	        return UA_STATUSCODE_BADOUTOFMEMORY;

	    UA_memcpy(new_refs, old_refs, sizeof(UA_ReferenceNode)*count);

	    UA_ReferenceNode *reference;
	    UA_ReferenceNode_new(&reference);

	    reference->isInverse = !referencesToAdd[indices[i]].isForward;
	    UA_NodeId_copy(&referencesToAdd[indices[i]].referenceTypeId,&reference->referenceTypeId);
	    UA_ExpandedNodeId_copy(&referencesToAdd[indices[i]].targetNodeId,&reference->targetId);

	    if(UA_ReferenceNode_copy(reference, &new_refs[count]) != UA_STATUSCODE_GOOD) {
	        UA_free(new_refs);
	        addReferencesResults[indices[i]] = UA_STATUSCODE_BADOUTOFMEMORY;
	    }
	    node->references     = new_refs;
	    node->referencesSize = count+1;
	    UA_free(old_refs);
	    addReferencesResults[indices[i]] =  UA_STATUSCODE_GOOD;
	    UA_ReferenceNode_delete(reference); //FIXME to be removed
	    //TODO fill diagnostic info if needed
	}

	return UA_STATUSCODE_GOOD;
}

UA_Int32 open62541Nodestore_addNodes(UA_AddNodesItem *nodesToAdd,UA_UInt32 *indices,
		UA_UInt32 indicesSize, UA_AddNodesResult* addNodesResults,
		UA_DiagnosticInfo *diagnosticInfos){

	UA_Node *node = UA_NULL;
	for(UA_UInt32 i=0;i<indicesSize;i++){
		UA_NodeStoreExample *ns = Nodestore_get();
		UA_NodeStoreExample_get((const UA_NodeStoreExample*)ns, (const UA_NodeId*)&nodesToAdd[indices[i]].requestedNewNodeId.nodeId , (const UA_Node**)&node);
		if(node==UA_NULL){
			switch(nodesToAdd[indices[i]].nodeClass){
				case UA_NODECLASS_DATATYPE:
				{
					break;
				}
				case UA_NODECLASS_METHOD:
				{

					break;
				}
				case UA_NODECLASS_OBJECT:
				{
					UA_ObjectNode *newNode;
					UA_ObjectNode_new(&newNode);
					newNode->nodeId    = nodesToAdd[indices[i]].requestedNewNodeId.nodeId;
					newNode->nodeClass = nodesToAdd[indices[i]].nodeClass;
					UA_QualifiedName_copy(&nodesToAdd[indices[i]].browseName, &newNode->browseName);

					UA_UInt32 offset = 0;
					UA_ObjectAttributes objType;

					UA_ObjectAttributes_decodeBinary(&nodesToAdd[indices[i]].nodeAttributes.body,&offset,&objType);
					if(objType.specifiedAttributes & UA_ATTRIBUTEID_DISPLAYNAME){
						UA_LocalizedText_copy(&objType.displayName, &newNode->displayName);
					}

					if(objType.specifiedAttributes & UA_ATTRIBUTEID_DESCRIPTION){
						UA_LocalizedText_copy(&objType.description, &newNode->description);
					}
					if(objType.specifiedAttributes & UA_ATTRIBUTEID_EVENTNOTIFIER){
						newNode->eventNotifier =  objType.eventNotifier;
					}
					if(objType.specifiedAttributes & UA_ATTRIBUTEID_WRITEMASK){
						newNode->writeMask = objType.writeMask;
					}

					UA_AddReferencesItem addRefItem;
					addRefItem.isForward = UA_TRUE;

					addRefItem.referenceTypeId = nodesToAdd[indices[i]].referenceTypeId;
					addRefItem.sourceNodeId =  nodesToAdd[indices[i]].parentNodeId.nodeId;
					addRefItem.targetNodeId.nodeId = newNode->nodeId;
					addRefItem.targetNodeId.namespaceUri.length = 0;
					addRefItem.targetServerUri.length = 0;
					addRefItem.targetNodeClass = newNode->nodeClass;

					UA_UInt32 ind = 0;
					UA_UInt32 indSize = 1;
					UA_StatusCode result;
					UA_DiagnosticInfo diagnosticInfo;
					UA_NodeStoreExample_insert(ns, (UA_Node**)&newNode, UA_NODESTORE_INSERT_UNIQUE);
					if(!(
							nodesToAdd[indices[i]].requestedNewNodeId.nodeId.identifier.numeric == 84 &&
							nodesToAdd[indices[i]].requestedNewNodeId.nodeId.namespaceIndex ==  0)){
						open62541NodeStore_addReferences(&addRefItem, &ind, indSize, &result, &diagnosticInfo);
					}
					break;
				}
				case UA_NODECLASS_OBJECTTYPE:
				{

					break;
				}
				case UA_NODECLASS_REFERENCETYPE:
				{
					UA_ReferenceTypeNode *newNode;
					UA_ReferenceTypeNode_new(&newNode);
					newNode->nodeId    = nodesToAdd[indices[i]].requestedNewNodeId.nodeId;
					newNode->nodeClass = nodesToAdd[indices[i]].nodeClass;
					UA_QualifiedName_copy(&nodesToAdd[indices[i]].browseName, &newNode->browseName);
					UA_UInt32 offset = 0;
					UA_ReferenceTypeAttributes refType;
					UA_ReferenceTypeAttributes_decodeBinary(&nodesToAdd[indices[i]].nodeAttributes.body,&offset,&refType);
					if(refType.specifiedAttributes & UA_ATTRIBUTEID_DISPLAYNAME){
						UA_LocalizedText_copy(&refType.displayName, &newNode->displayName);
					}

					if(refType.specifiedAttributes & UA_ATTRIBUTEID_DESCRIPTION){
						UA_LocalizedText_copy(&refType.description, &newNode->description);
					}

					if(refType.specifiedAttributes & UA_ATTRIBUTEID_ISABSTRACT){
						newNode->isAbstract = refType.isAbstract;
					}

					if(refType.specifiedAttributes & UA_ATTRIBUTEID_SYMMETRIC){
						newNode->symmetric = refType.symmetric;
					}
					//ADDREFERENCE(haschild, RefTypeId_HasSubtype, UA_TRUE, RefTypeId_HierarchicalReferences);

					UA_AddReferencesItem addRefItem;
					addRefItem.isForward = UA_TRUE;

					addRefItem.referenceTypeId = nodesToAdd[indices[i]].referenceTypeId;
					addRefItem.sourceNodeId =  nodesToAdd[indices[i]].parentNodeId.nodeId;
					addRefItem.targetNodeId.nodeId = newNode->nodeId;
					addRefItem.targetNodeId.namespaceUri.length = 0;
					addRefItem.targetServerUri.length = 0;
					addRefItem.targetNodeClass = newNode->nodeClass;

					UA_UInt32 ind = 0;
					UA_UInt32 indSize = 1;
					UA_StatusCode result;
					UA_DiagnosticInfo diagnosticInfo;
					UA_NodeStoreExample_insert(ns, (UA_Node**)&newNode, UA_NODESTORE_INSERT_UNIQUE);
					open62541NodeStore_addReferences(&addRefItem, &ind, indSize, &result, &diagnosticInfo);


					break;
				}
				case UA_NODECLASS_VARIABLE:
				{

					break;
				}
				case UA_NODECLASS_VARIABLETYPE:
				{

					break;
				}
				default:
				{
					break;
				}
			}
		}
	}
	return UA_STATUSCODE_GOOD;
}
