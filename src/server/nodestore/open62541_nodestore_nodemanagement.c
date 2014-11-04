/*
 * open62541_nodestore_nodemanagement.c
 *
 *  Created on: Oct 27, 2014
 *      Author: opcua
 */

#include "../ua_services.h"
#include "open62541_nodestore.h"
#include "ua_namespace_0.h"
#include "ua_util.h"


static UA_Int32 AddSingleReference(UA_Node *node, UA_ReferenceNode *reference) {
	// TODO: Check if reference already exists
	UA_Int32 count = node->referencesSize;
	UA_ReferenceNode *old_refs = node->references;
	UA_ReferenceNode *new_refs;

	if (count < 0)
		count = 0;

	if (!(new_refs = UA_alloc(sizeof(UA_ReferenceNode) * (count + 1))))
		return UA_STATUSCODE_BADOUTOFMEMORY;

	UA_memcpy(new_refs, old_refs, sizeof(UA_ReferenceNode) * count);
	if (UA_ReferenceNode_copy(reference, &new_refs[count])
			!= UA_STATUSCODE_GOOD) {
		UA_free(new_refs);
		return UA_STATUSCODE_BADOUTOFMEMORY;
	}

	node->references = new_refs;
	node->referencesSize = count + 1;
	UA_free(old_refs);
	return UA_STATUSCODE_GOOD;
}

static UA_Int32 AddReference(open62541NodeStore *nodestore, UA_Node *node,
		UA_ReferenceNode *reference) {
	UA_Int32 retval = AddSingleReference(node, reference);
	UA_Node *targetnode;
	UA_ReferenceNode inversereference;
	if (retval != UA_STATUSCODE_GOOD || nodestore == UA_NULL)
		return retval;

	// Do a copy every time?
	if (open62541NodeStore_get(nodestore, &reference->targetId.nodeId,
			(const UA_Node **) &targetnode) != UA_STATUSCODE_GOOD)
		return UA_STATUSCODE_BADINTERNALERROR;

	inversereference.referenceTypeId = reference->referenceTypeId;
	inversereference.isInverse = !reference->isInverse;
	inversereference.targetId.nodeId = node->nodeId;
	inversereference.targetId.namespaceUri = UA_STRING_NULL;
	inversereference.targetId.serverIndex = 0;
	retval = AddSingleReference(targetnode, &inversereference);
	open62541NodeStore_releaseManagedNode(targetnode);

	return retval;
}

//TODO export to types, maybe?
void UA_String_setToNULL(UA_String* string){
	string->data = NULL;
	string->length = -1;
}

void UA_Node_setAttributes(UA_NodeAttributes *nodeAttributes, UA_Node *node){

	if(nodeAttributes->specifiedAttributes & UA_NODEATTRIBUTESMASK_DISPLAYNAME){
		node->displayName =  nodeAttributes->displayName;
		UA_String_setToNULL(&nodeAttributes->displayName.locale);
		UA_String_setToNULL(&nodeAttributes->displayName.text);
	}
	if(nodeAttributes->specifiedAttributes & UA_NODEATTRIBUTESMASK_DESCRIPTION){
		node->description =  nodeAttributes->description;
		UA_String_setToNULL(&nodeAttributes->description.locale);
		UA_String_setToNULL(&nodeAttributes->description.text);
	}
	if(nodeAttributes->specifiedAttributes & UA_NODEATTRIBUTESMASK_WRITEMASK){
			node->writeMask = nodeAttributes->writeMask;
	}
	if(nodeAttributes->specifiedAttributes & UA_NODEATTRIBUTESMASK_USERWRITEMASK){
		node->userWriteMask = nodeAttributes->userWriteMask;
	}
}
void UA_ObjectNode_setAttributes(UA_ObjectAttributes *objectAttributes, UA_ObjectNode *node){
	UA_Node_setAttributes((UA_NodeAttributes*)objectAttributes,(UA_Node*)node);

	if(objectAttributes->specifiedAttributes & UA_NODEATTRIBUTESMASK_EVENTNOTIFIER){
			node->eventNotifier = objectAttributes->eventNotifier;
	}
}

void UA_ReferenceTypeNode_setAttributes(UA_ReferenceTypeAttributes *referenceTypeAttributes, UA_ReferenceTypeNode *node){
	UA_Node_setAttributes((UA_NodeAttributes*)referenceTypeAttributes,(UA_Node*)node);

	if(referenceTypeAttributes->specifiedAttributes & UA_NODEATTRIBUTESMASK_ISABSTRACT){
			node->isAbstract = referenceTypeAttributes->isAbstract;
	}
	if(referenceTypeAttributes->specifiedAttributes & UA_NODEATTRIBUTESMASK_SYMMETRIC){
				node->symmetric = referenceTypeAttributes->symmetric;
	}
	if(referenceTypeAttributes->specifiedAttributes & UA_NODEATTRIBUTESMASK_INVERSENAME){
		node->inverseName = referenceTypeAttributes->inverseName;
		UA_String_setToNULL(&referenceTypeAttributes->inverseName.locale);
		UA_String_setToNULL(&referenceTypeAttributes->inverseName.text);
	}
}
void UA_ObjectTypeNode_setAttributes(UA_ObjectTypeAttributes *objectTypeAttributes, UA_ObjectTypeNode *node){
	UA_Node_setAttributes((UA_NodeAttributes*)objectTypeAttributes,(UA_Node*)node);

	if(objectTypeAttributes->specifiedAttributes & UA_NODEATTRIBUTESMASK_ISABSTRACT){
			node->isAbstract = objectTypeAttributes->isAbstract;
	}
}

void UA_VariableNode_setAttributes(UA_VariableAttributes *variableAttributes, UA_VariableNode *node){
	UA_Node_setAttributes((UA_NodeAttributes*)variableAttributes,(UA_Node*)node);

	if(variableAttributes->specifiedAttributes & UA_NODEATTRIBUTESMASK_VALUE){
			UA_Variant_copy(&variableAttributes->value,&node->value);
	}
	if(variableAttributes->specifiedAttributes & UA_NODEATTRIBUTESMASK_DATATYPE){
				UA_NodeId_copy(&variableAttributes->dataType,&node->dataType);
	}
	if(variableAttributes->specifiedAttributes & UA_NODEATTRIBUTESMASK_VALUERANK){
		node->valueRank = variableAttributes->valueRank;
	}
	if(variableAttributes->specifiedAttributes & UA_NODEATTRIBUTESMASK_ARRAYDIMENSIONS){
		node->arrayDimensions = variableAttributes->arrayDimensions;
		variableAttributes->arrayDimensions = NULL;
	}
	if(variableAttributes->specifiedAttributes & UA_NODEATTRIBUTESMASK_ACCESSLEVEL){
		node->accessLevel = variableAttributes->accessLevel;
	}
	if(variableAttributes->specifiedAttributes & UA_NODEATTRIBUTESMASK_USERACCESSLEVEL){
		node->userAccessLevel = variableAttributes->userAccessLevel;
	}
	if(variableAttributes->specifiedAttributes & UA_NODEATTRIBUTESMASK_MINIMUMSAMPLINGINTERVAL){
		node->minimumSamplingInterval = variableAttributes->minimumSamplingInterval;
	}
	if(variableAttributes->specifiedAttributes & UA_NODEATTRIBUTESMASK_HISTORIZING){
		node->historizing = variableAttributes->historizing;
	}
}
void UA_ViewNode_setAttributes(UA_ViewAttributes *viewAttributes, UA_ViewNode *node){
	UA_Node_setAttributes((UA_NodeAttributes*)viewAttributes,(UA_Node*)node);
	if(viewAttributes->specifiedAttributes & UA_NODEATTRIBUTESMASK_CONTAINSNOLOOPS){
			node->containsNoLoops = viewAttributes->containsNoLoops;
	}
	if(viewAttributes->specifiedAttributes & UA_NODEATTRIBUTESMASK_EVENTNOTIFIER){
				node->eventNotifier = viewAttributes->eventNotifier;
	}
}
void open62541Nodestore_getNewNodeId(UA_ExpandedNodeId *requestedNodeId){
	//check nodeId here
	return;
}

UA_Int32 open62541NodeStore_AddReferences(const UA_RequestHeader *requestHeader, UA_AddReferencesItem* referencesToAdd,
		UA_UInt32 *indices,UA_UInt32 indicesSize, UA_StatusCode *addReferencesResults,
		UA_DiagnosticInfo *diagnosticInfos)
{
	for(UA_UInt32 i = 0;i<indicesSize;i++){
		UA_Node *node = UA_NULL;
		open62541NodeStore *ns = open62541NodeStore_getNodeStore();
		open62541NodeStore_get((const open62541NodeStore*)ns,(const UA_NodeId*)&referencesToAdd[indices[i]].sourceNodeId, (const UA_Node**)&node);
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

	    UA_ReferenceNode *reference = UA_ReferenceNode_new();

	    reference->isInverse = !referencesToAdd[indices[i]].isForward;
	    UA_NodeId_copy(&referencesToAdd[indices[i]].referenceTypeId,&reference->referenceTypeId);
	    UA_ExpandedNodeId_copy(&referencesToAdd[indices[i]].targetNodeId,&reference->targetId);

	    addReferencesResults[indices[i]] =  AddReference(ns,node,reference);
	    UA_ReferenceNode_delete(reference); //FIXME to be removed
	    //TODO fill diagnostic info if needed
	}

	return UA_STATUSCODE_GOOD;
}
UA_Boolean isRootNode(UA_NodeId *nodeId){
	return nodeId->identifierType == UA_NODEIDTYPE_NUMERIC && nodeId->namespaceIndex == 0 && nodeId->identifier.numeric == 84;
}
UA_Int32 open62541NodeStore_AddNodes(const UA_RequestHeader *requestHeader,UA_AddNodesItem *nodesToAdd,UA_UInt32 *indices,
		UA_UInt32 indicesSize, UA_AddNodesResult* addNodesResults,
		UA_DiagnosticInfo *diagnosticInfos){

	UA_Node *node = UA_NULL;
	for(UA_UInt32 i=0;i<indicesSize;i++){

		const UA_Node *parent;
		//todo what if node is in another namespace, readrequest to test, if it exists?
		open62541NodeStore *ns = open62541NodeStore_getNodeStore();
		if (open62541NodeStore_get(ns, &nodesToAdd->parentNodeId.nodeId,
				&parent) != UA_STATUSCODE_GOOD && !isRootNode(&nodesToAdd->requestedNewNodeId.nodeId)) {
			addNodesResults[indices[i]].statusCode = UA_STATUSCODE_BADPARENTNODEIDINVALID;
			continue;
		}


		open62541NodeStore_get((const open62541NodeStore*)ns, (const UA_NodeId*)&nodesToAdd[indices[i]].requestedNewNodeId.nodeId , (const UA_Node**)&node);


		if(node!=UA_NULL){
			//todo or overwrite existing node?
			continue;
		}
		UA_Node *newNode = UA_NULL;
		UA_UInt32 offset = 0;
		switch(nodesToAdd[indices[i]].nodeClass){
			case UA_NODECLASS_DATATYPE:
			{
				addNodesResults[indices[i]].statusCode = UA_STATUSCODE_BADNOTIMPLEMENTED;
				continue;
				break;
			}
			case UA_NODECLASS_METHOD:
			{
				addNodesResults[indices[i]].statusCode = UA_STATUSCODE_BADNOTIMPLEMENTED;
				continue;
				break;
			}
			case UA_NODECLASS_OBJECT:
			{
				UA_ObjectAttributes attributes;

				newNode = (UA_Node*)UA_ObjectNode_new();
				newNode->nodeClass = UA_NODECLASS_OBJECT;
				UA_ObjectAttributes_decodeBinary(&nodesToAdd[indices[i]].nodeAttributes.body,&offset,&attributes);
				UA_ObjectNode_setAttributes((UA_ObjectAttributes*)&attributes, (UA_ObjectNode*)newNode);
				break;
			}
			case UA_NODECLASS_OBJECTTYPE:
			{
				addNodesResults[indices[i]].statusCode = UA_STATUSCODE_BADNOTIMPLEMENTED;
				continue;
				break;
			}
			case UA_NODECLASS_REFERENCETYPE:
			{
				UA_ReferenceTypeAttributes attributes;
				newNode = (UA_Node*)UA_ReferenceTypeNode_new();
				newNode->nodeClass = UA_NODECLASS_REFERENCETYPE;
				UA_ReferenceTypeAttributes_decodeBinary(&nodesToAdd[indices[i]].nodeAttributes.body,&offset,&attributes);
				UA_ReferenceTypeNode_setAttributes((UA_ReferenceTypeAttributes*)&attributes,(UA_ReferenceTypeNode*)newNode);
				break;
			}
			case UA_NODECLASS_VARIABLE:
			{
				UA_VariableAttributes attributes;
				newNode = (UA_Node*)UA_VariableNode_new();
				newNode->nodeClass = UA_NODECLASS_VARIABLE;
				UA_VariableAttributes_decodeBinary(&nodesToAdd[indices[i]].nodeAttributes.body,&offset,&attributes);
				UA_VariableNode_setAttributes((UA_VariableAttributes*)&attributes,(UA_VariableNode*)newNode);
				break;
			}
			case UA_NODECLASS_VARIABLETYPE:
			{
				addNodesResults[indices[i]].statusCode = UA_STATUSCODE_BADNOTIMPLEMENTED;
				continue;
				break;
			}
			default:
			{
				addNodesResults[indices[i]].statusCode = UA_STATUSCODE_BADNOTIMPLEMENTED;
				continue;
				break;
			}

		}
		open62541Nodestore_getNewNodeId(&nodesToAdd[indices[i]].requestedNewNodeId);
					newNode->nodeId    = nodesToAdd[indices[i]].requestedNewNodeId.nodeId;
		UA_QualifiedName_copy(&nodesToAdd[indices[i]].browseName,
				&newNode->browseName);

		UA_AddReferencesItem addRefItem;
		addRefItem.isForward = UA_TRUE;
		addRefItem.referenceTypeId = nodesToAdd[indices[i]].referenceTypeId;
		addRefItem.sourceNodeId = nodesToAdd[indices[i]].parentNodeId.nodeId;
		addRefItem.targetNodeId.nodeId = newNode->nodeId;
		addRefItem.targetNodeId.namespaceUri.length = 0;
		addRefItem.targetServerUri.length = 0;
		addRefItem.targetNodeClass = newNode->nodeClass;


		open62541NodeStore_insert(ns, (UA_Node**) &newNode,
				UA_NODESTORE_INSERT_UNIQUE);
		if (!isRootNode(&nodesToAdd[indices[i]].requestedNewNodeId.nodeId)) {
			UA_UInt32 ind = 0;
			UA_UInt32 indSize = 1;
			UA_StatusCode result;
			UA_DiagnosticInfo diagnosticInfo;
			open62541NodeStore_AddReferences(requestHeader, &addRefItem, &ind, indSize,
					&result, &diagnosticInfo);
		}

	}
	return UA_STATUSCODE_GOOD;
}
