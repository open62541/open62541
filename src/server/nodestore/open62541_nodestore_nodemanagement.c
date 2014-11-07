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
	open62541NodeStore_release(targetnode);

	return retval;
}

#define COPY_STANDARDATTRIBUTES do { \
if(attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_DISPLAYNAME) { \
vnode->displayName = attr.displayName; \
UA_LocalizedText_init(&attr.displayName); \
} \
if(attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_DESCRIPTION) { \
vnode->description = attr.description; \
UA_LocalizedText_init(&attr.description); \
} \
if(attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_WRITEMASK) \
vnode->writeMask = attr.writeMask; \
if(attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_USERWRITEMASK) \
vnode->userWriteMask = attr.userWriteMask; \
} while(0)

static UA_StatusCode parseVariableNode(UA_ExtensionObject *attributes,
		UA_Node **new_node, const UA_VTable_Entry **vt) {
	if (attributes->typeId.identifier.numeric != 357) // VariableAttributes_Encoding_DefaultBinary,357,Object
		return UA_STATUSCODE_BADNODEATTRIBUTESINVALID;
	UA_VariableAttributes attr;
	UA_UInt32 pos = 0;
// todo return more informative error codes from decodeBinary
	if (UA_VariableAttributes_decodeBinary(&attributes->body, &pos, &attr)
			!= UA_STATUSCODE_GOOD)
		return UA_STATUSCODE_BADNODEATTRIBUTESINVALID;
	UA_VariableNode *vnode = UA_VariableNode_new();
	if (!vnode) {
		UA_VariableAttributes_deleteMembers(&attr);
		return UA_STATUSCODE_BADOUTOFMEMORY;
	}
// now copy all the attributes. This potentially removes them from the decoded attributes.
	COPY_STANDARDATTRIBUTES;
	if (attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_ACCESSLEVEL)
		vnode->accessLevel = attr.accessLevel;
	if (attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_USERACCESSLEVEL)
		vnode->userAccessLevel = attr.userAccessLevel;
	if (attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_HISTORIZING)
		vnode->historizing = attr.historizing;
	if (attr.specifiedAttributes
			& UA_NODEATTRIBUTESMASK_MINIMUMSAMPLINGINTERVAL)
		vnode->minimumSamplingInterval = attr.minimumSamplingInterval;
	if (attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_VALUERANK)
		vnode->valueRank = attr.valueRank;
	if (attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_ARRAYDIMENSIONS) {
		vnode->arrayDimensionsSize = attr.arrayDimensionsSize;
		vnode->arrayDimensions = attr.arrayDimensions;
		attr.arrayDimensionsSize = -1;
		attr.arrayDimensions = UA_NULL;
	}
	if ((attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_DATATYPE)
			|| (attr.specifiedAttributes
					& UA_NODEATTRIBUTESMASK_OBJECTTYPEORDATATYPE)) {
		vnode->dataType = attr.dataType;
		UA_NodeId_init(&attr.dataType);
	}
	if (attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_VALUE) {
		vnode->value = attr.value;
		UA_Variant_init(&attr.value);
	}
	UA_VariableAttributes_deleteMembers(&attr);
	*new_node = (UA_Node*) vnode;
	*vt = &UA_TYPES[UA_VARIABLENODE];
	return UA_STATUSCODE_GOOD;
}

static UA_StatusCode parseObjectNode(UA_ExtensionObject *attributes,
		UA_Node **new_node, const UA_VTable_Entry **vt) {
	if (attributes->typeId.identifier.numeric != 354) // VariableAttributes_Encoding_DefaultBinary,357,Object
		return UA_STATUSCODE_BADNODEATTRIBUTESINVALID;
	UA_ObjectAttributes attr;
	UA_UInt32 pos = 0;
// todo return more informative error codes from decodeBinary
	if (UA_ObjectAttributes_decodeBinary(&attributes->body, &pos, &attr)
			!= UA_STATUSCODE_GOOD)
		return UA_STATUSCODE_BADNODEATTRIBUTESINVALID;
	UA_ObjectNode *vnode = UA_ObjectNode_new();
	if (!vnode) {
		UA_ObjectAttributes_deleteMembers(&attr);
		return UA_STATUSCODE_BADOUTOFMEMORY;
	}
// now copy all the attributes. This potentially removes them from the decoded attributes.
	COPY_STANDARDATTRIBUTES;
	if (attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_EVENTNOTIFIER) {
		vnode->eventNotifier = attr.eventNotifier;
	}
	UA_ObjectAttributes_deleteMembers(&attr);
	*new_node = (UA_Node*) vnode;
	*vt = &UA_TYPES[UA_OBJECTNODE];
	return UA_STATUSCODE_GOOD;
}

static UA_StatusCode parseReferenceTypeNode(UA_ExtensionObject *attributes,
		UA_Node **new_node, const UA_VTable_Entry **vt) {

	UA_ReferenceTypeAttributes attr;
	UA_UInt32 pos = 0;
// todo return more informative error codes from decodeBinary
	if (UA_ReferenceTypeAttributes_decodeBinary(&attributes->body, &pos, &attr)
			!= UA_STATUSCODE_GOOD)
		return UA_STATUSCODE_BADNODEATTRIBUTESINVALID;
	UA_ReferenceTypeNode *vnode = UA_ReferenceTypeNode_new();
	if (!vnode) {
		UA_ReferenceTypeAttributes_deleteMembers(&attr);
		return UA_STATUSCODE_BADOUTOFMEMORY;
	}
// now copy all the attributes. This potentially removes them from the decoded attributes.
	COPY_STANDARDATTRIBUTES;

	if (attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_ISABSTRACT) {
		vnode->isAbstract = attr.isAbstract;
	}
	if (attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_SYMMETRIC) {
		vnode->symmetric = attr.symmetric;
	}
	if (attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_INVERSENAME) {
		vnode->inverseName = attr.inverseName;

		attr.inverseName.text.length = -1;
		attr.inverseName.text.data = UA_NULL;

		attr.inverseName.locale.length = -1;
		attr.inverseName.locale.data = UA_NULL;
	}

	UA_ReferenceTypeAttributes_deleteMembers(&attr);
	*new_node = (UA_Node*) vnode;
	*vt = &UA_TYPES[UA_REFERENCETYPENODE];
	return UA_STATUSCODE_GOOD;
}

static UA_StatusCode parseObjectTypeNode(UA_ExtensionObject *attributes,
		UA_Node **new_node, const UA_VTable_Entry **vt) {

	UA_ObjectTypeAttributes attr;
	UA_UInt32 pos = 0;
// todo return more informative error codes from decodeBinary
	if (UA_ObjectTypeAttributes_decodeBinary(&attributes->body, &pos, &attr)
			!= UA_STATUSCODE_GOOD)
		return UA_STATUSCODE_BADNODEATTRIBUTESINVALID;
	UA_ObjectTypeNode *vnode = UA_ObjectTypeNode_new();
	if (!vnode) {
		UA_ObjectTypeAttributes_deleteMembers(&attr);
		return UA_STATUSCODE_BADOUTOFMEMORY;
	}
// now copy all the attributes. This potentially removes them from the decoded attributes.
	COPY_STANDARDATTRIBUTES;

	if (attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_ISABSTRACT) {
		vnode->isAbstract = attr.isAbstract;
	}
	UA_ObjectTypeAttributes_deleteMembers(&attr);
	*new_node = (UA_Node*) vnode;
	*vt = &UA_TYPES[UA_OBJECTTYPENODE];
	return UA_STATUSCODE_GOOD;
}

static UA_StatusCode parseViewNode(UA_ExtensionObject *attributes,
		UA_Node **new_node, const UA_VTable_Entry **vt) {

	UA_ViewAttributes attr;
	UA_UInt32 pos = 0;
// todo return more informative error codes from decodeBinary
	if (UA_ViewAttributes_decodeBinary(&attributes->body, &pos, &attr)
			!= UA_STATUSCODE_GOOD)
		return UA_STATUSCODE_BADNODEATTRIBUTESINVALID;
	UA_ViewNode *vnode = UA_ViewNode_new();
	if (!vnode) {
		UA_ViewAttributes_deleteMembers(&attr);
		return UA_STATUSCODE_BADOUTOFMEMORY;
	}
// now copy all the attributes. This potentially removes them from the decoded attributes.
	COPY_STANDARDATTRIBUTES;

	if (attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_CONTAINSNOLOOPS) {
		vnode->containsNoLoops = attr.containsNoLoops;
	}
	if (attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_EVENTNOTIFIER) {
		vnode->eventNotifier = attr.eventNotifier;
	}
	UA_ViewAttributes_deleteMembers(&attr);
	*new_node = (UA_Node*) vnode;
	*vt = &UA_TYPES[UA_VIEWNODE];
	return UA_STATUSCODE_GOOD;
}

void open62541Nodestore_getNewNodeId(UA_ExpandedNodeId *requestedNodeId) {
	//check nodeId here
	return;
}

UA_Int32 open62541NodeStore_AddReferences(const UA_RequestHeader *requestHeader,
		UA_AddReferencesItem* referencesToAdd, UA_UInt32 *indices,
		UA_UInt32 indicesSize, UA_StatusCode *addReferencesResults,
		UA_DiagnosticInfo *diagnosticInfos) {
	for (UA_UInt32 i = 0; i < indicesSize; i++) {
		UA_Node *node = UA_NULL;
		open62541NodeStore *ns = open62541NodeStore_getNodeStore();
		open62541NodeStore_get((const open62541NodeStore*) ns,
				(const UA_NodeId*) &referencesToAdd[indices[i]].sourceNodeId,
				(const UA_Node**) &node);
		if (node == UA_NULL) {
			addReferencesResults[indices[i]] =
					UA_STATUSCODE_BADSOURCENODEIDINVALID;
			continue;
		}
		// TODO: Check if reference already exists
		UA_Int32 count = node->referencesSize;
		UA_ReferenceNode *old_refs = node->references;
		UA_ReferenceNode *new_refs;
		if (count < 0)
			count = 0;

		if (!(new_refs = UA_alloc(sizeof(UA_ReferenceNode) * (count + 1))))
			return UA_STATUSCODE_BADOUTOFMEMORY;

		UA_memcpy(new_refs, old_refs, sizeof(UA_ReferenceNode) * count);

		//UA_ReferenceNode *reference = UA_ReferenceNode_new();
		UA_ReferenceNode reference;
		UA_ReferenceNode_init(&reference);
		reference.isInverse = !referencesToAdd[indices[i]].isForward;
		UA_NodeId_copy(&referencesToAdd[indices[i]].referenceTypeId,
				&reference.referenceTypeId);
		UA_ExpandedNodeId_copy(&referencesToAdd[indices[i]].targetNodeId,
				&reference.targetId);

		addReferencesResults[indices[i]] = AddReference(ns, node, &reference);
		//TODO fill diagnostic info if needed
		UA_free(new_refs);
	}

	return UA_STATUSCODE_GOOD;
}
UA_Boolean isRootNode(UA_NodeId *nodeId) {
	return nodeId->identifierType == UA_NODEIDTYPE_NUMERIC
			&& nodeId->namespaceIndex == 0 && nodeId->identifier.numeric == 84;
}
UA_Int32 open62541NodeStore_AddNodes(const UA_RequestHeader *requestHeader,
		UA_AddNodesItem *nodesToAdd, UA_UInt32 *indices, UA_UInt32 indicesSize,
		UA_AddNodesResult* addNodesResults, UA_DiagnosticInfo *diagnosticInfos) {

	UA_Node *node = UA_NULL;
	for (UA_UInt32 i = 0; i < indicesSize; i++) {

		const UA_Node *parent = UA_NULL;
		//todo what if node is in another namespace, readrequest to test, if it exists?
		open62541NodeStore *ns = open62541NodeStore_getNodeStore();
		if (open62541NodeStore_get(ns, &nodesToAdd->parentNodeId.nodeId,
				&parent) != UA_STATUSCODE_GOOD
				&& !isRootNode(&nodesToAdd->requestedNewNodeId.nodeId)) {
			addNodesResults[indices[i]].statusCode =
					UA_STATUSCODE_BADPARENTNODEIDINVALID;
			continue;
		}

		open62541NodeStore_get((const open62541NodeStore*) ns,
				(const UA_NodeId*) &nodesToAdd[indices[i]].requestedNewNodeId.nodeId,
				(const UA_Node**) &node);

		if (node != UA_NULL) {
			//todo or overwrite existing node?
			continue;
		}
		UA_Node *newNode = UA_NULL;
		const UA_VTable_Entry *newNodeVT = UA_NULL;

		switch (nodesToAdd[indices[i]].nodeClass) {
		case UA_NODECLASS_DATATYPE: {
			addNodesResults[indices[i]].statusCode =
					UA_STATUSCODE_BADNOTIMPLEMENTED;
			continue;
			break;
		}
		case UA_NODECLASS_METHOD: {
			addNodesResults[indices[i]].statusCode =
					UA_STATUSCODE_BADNOTIMPLEMENTED;
			continue;
			break;
		}
		case UA_NODECLASS_OBJECT: {
			parseObjectNode(&nodesToAdd[indices[i]].nodeAttributes, &newNode,
					&newNodeVT);
			newNode->nodeClass = UA_NODECLASS_OBJECT;
			break;
		}
		case UA_NODECLASS_OBJECTTYPE: {
			parseObjectTypeNode(&nodesToAdd[indices[i]].nodeAttributes,
					&newNode, &newNodeVT);
			newNode->nodeClass = UA_NODECLASS_OBJECTTYPE;
			break;
		}
		case UA_NODECLASS_REFERENCETYPE: {
			parseReferenceTypeNode(&nodesToAdd[indices[i]].nodeAttributes,
					&newNode, &newNodeVT);
			newNode->nodeClass = UA_NODECLASS_REFERENCETYPE;
			break;
		}
		case UA_NODECLASS_VARIABLE: {
			parseVariableNode(&nodesToAdd[indices[i]].nodeAttributes, &newNode,
					&newNodeVT);
			newNode->nodeClass = UA_NODECLASS_VARIABLE;
			break;
		}
		case UA_NODECLASS_VARIABLETYPE: {
			addNodesResults[indices[i]].statusCode =
					UA_STATUSCODE_BADNOTIMPLEMENTED;
			continue;
			break;
		}
		default: {
			addNodesResults[indices[i]].statusCode =
					UA_STATUSCODE_BADNOTIMPLEMENTED;
			continue;
			break;
		}

		}
		open62541Nodestore_getNewNodeId(
				&nodesToAdd[indices[i]].requestedNewNodeId);
		newNode->nodeId = nodesToAdd[indices[i]].requestedNewNodeId.nodeId;
		UA_QualifiedName_copy(&nodesToAdd[indices[i]].browseName,
				&newNode->browseName);

		UA_AddReferencesItem addRefItem;
		UA_AddReferencesItem_init(&addRefItem);
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
			open62541NodeStore_AddReferences(requestHeader, &addRefItem, &ind,
					indSize, &result, &diagnosticInfo);
		}
		UA_AddReferencesItem_deleteMembers(&addRefItem);
	}
	return UA_STATUSCODE_GOOD;
}
