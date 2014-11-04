/*
 * service_view_implementation.c
 *
 *  Created on: Oct 27, 2014
 *      Author: opcua
 */
#include "open62541_nodestore.h"
#include "../ua_services.h"
#include "ua_namespace_0.h"
#include "ua_util.h"



/* Releases the current node, even if it was supplied as an argument. */
static UA_StatusCode fillReferenceDescription(open62541NodeStore *ns, const UA_Node *currentNode, UA_ReferenceNode *reference,
                                              UA_UInt32 resultMask, UA_ReferenceDescription *referenceDescription) {
    UA_ReferenceDescription_init(referenceDescription);
    if(!currentNode && resultMask != 0) {
        if(open62541NodeStore_get(ns, &reference->targetId.nodeId, &currentNode) != UA_STATUSCODE_GOOD)
            return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_NodeId_copy(&currentNode->nodeId, &referenceDescription->nodeId.nodeId);
    //TODO: ExpandedNodeId is mocked up
    referenceDescription->nodeId.serverIndex = 0;
    referenceDescription->nodeId.namespaceUri.length = -1;

    if(resultMask & UA_BROWSERESULTMASK_REFERENCETYPEID)
        retval |= UA_NodeId_copy(&reference->referenceTypeId, &referenceDescription->referenceTypeId);
    if(resultMask & UA_BROWSERESULTMASK_ISFORWARD)
        referenceDescription->isForward = !reference->isInverse;
    if(resultMask & UA_BROWSERESULTMASK_NODECLASS)
        retval |= UA_NodeClass_copy(&currentNode->nodeClass, &referenceDescription->nodeClass);
    if(resultMask & UA_BROWSERESULTMASK_BROWSENAME)
        retval |= UA_QualifiedName_copy(&currentNode->browseName, &referenceDescription->browseName);
    if(resultMask & UA_BROWSERESULTMASK_DISPLAYNAME)
        retval |= UA_LocalizedText_copy(&currentNode->displayName, &referenceDescription->displayName);
    if(resultMask & UA_BROWSERESULTMASK_TYPEDEFINITION && currentNode->nodeClass != UA_NODECLASS_OBJECT &&
       currentNode->nodeClass != UA_NODECLASS_VARIABLE) {
        for(UA_Int32 i = 0;i < currentNode->referencesSize;i++) {
            UA_ReferenceNode *ref = &currentNode->references[i];
            if(ref->referenceTypeId.identifier.numeric == 40 /* hastypedefinition */) {
                retval |= UA_ExpandedNodeId_copy(&ref->targetId, &referenceDescription->typeDefinition);
                break;
            }
        }
    }

    if(currentNode)
    	open62541NodeStore_release(currentNode);
    if(retval)
        UA_ReferenceDescription_deleteMembers(referenceDescription);
    return retval;
}

/* Tests if the node is relevant an shall be returned. If the targetNode needs
   to be retrieved from the nodestore to determine this, the targetNode is
   returned if the node is relevant. */
static UA_Boolean isRelevantTargetNode(open62541NodeStore *ns, const UA_BrowseDescription *browseDescription, UA_Boolean returnAll,
                                       UA_ReferenceNode *reference, const UA_Node **currentNode,
                                       UA_NodeId *relevantRefTypes, UA_UInt32 relevantRefTypesCount) {
    // 1) Test Browse direction
    if(reference->isInverse == UA_TRUE && browseDescription->browseDirection == UA_BROWSEDIRECTION_FORWARD)
        return UA_FALSE;

    else if(reference->isInverse == UA_FALSE && browseDescription->browseDirection == UA_BROWSEDIRECTION_INVERSE)
        return UA_FALSE;

    // 2) Test if the reference type is relevant
    UA_Boolean isRelevant = returnAll;
    if(!isRelevant) {
        for(UA_UInt32 i = 0;i < relevantRefTypesCount;i++) {
            if(UA_NodeId_equal(&reference->referenceTypeId, &relevantRefTypes[i]))
                isRelevant = UA_TRUE;
        }
        if(!isRelevant)
            return UA_FALSE;
    }

    // 3) Test if the target nodeClass is relevant
    if(browseDescription->nodeClassMask == 0)
        return UA_TRUE; // the node is relevant, but we didn't need to get it from the nodestore yet.

    if(open62541NodeStore_get(ns, &reference->targetId.nodeId, currentNode) != UA_STATUSCODE_GOOD)
        return UA_FALSE;

    if(((*currentNode)->nodeClass & browseDescription->nodeClassMask) == 0) {
    	open62541NodeStore_release(*currentNode);
        return UA_FALSE;
    }

    // the node is relevant and was retrieved from the nodestore, do not release it.
    return UA_TRUE;
}

/* We do not search across namespaces so far. The id of the root-referencetype
   is returned in the array also. */
static UA_StatusCode findRelevantReferenceTypes(open62541NodeStore *ns, const UA_NodeId *rootReferenceType,
                                                UA_NodeId **referenceTypes, UA_UInt32 *referenceTypesSize) {
    /* The references form a tree. We walk the tree by adding new nodes to the end of the array. */
    UA_UInt32 currentIndex = 0;
    UA_UInt32 currentLastIndex = 0;
    UA_UInt32 currentArraySize = 20; // should be more than enough. if not, increase the array size.
    UA_NodeId *typeArray = UA_alloc(sizeof(UA_NodeId) * currentArraySize);
    if(!typeArray)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_NodeId_copy(rootReferenceType, &typeArray[0]);
    if(retval) {
        UA_free(typeArray);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    const UA_ReferenceTypeNode *node;
    do {
        retval |= open62541NodeStore_get(ns, &typeArray[currentIndex], (const UA_Node **)&node);
        if(retval)
            break;
        if(node->nodeClass != UA_NODECLASS_REFERENCETYPE) // subtypes of referencestypes are always referencestypes?
            continue;

        // Find subtypes of the current referencetype
        for(UA_Int32 i = 0; i < node->referencesSize && retval == UA_STATUSCODE_GOOD; i++) {
            if(node->references[i].referenceTypeId.identifier.numeric != 45 /* HasSubtype */ ||
               node->references[i].isInverse == UA_TRUE)
                continue;

            if(currentLastIndex + 1 >= currentArraySize) {
                // we need to resize the array
                UA_NodeId *newArray = UA_alloc(sizeof(UA_NodeId) * currentArraySize * 2);
                if(newArray) {
                    memcpy(newArray, typeArray, sizeof(UA_NodeId) * currentArraySize);
                    currentArraySize *= 2;
                    UA_free(typeArray);
                    typeArray = newArray;
                } else {
                    retval = UA_STATUSCODE_BADOUTOFMEMORY;
                    break;
                }
            }

            // ok, we have space to add the new referencetype.
            retval |= UA_NodeId_copy(&node->references[i].targetId.nodeId, &typeArray[++currentLastIndex]);
            if(retval)
                currentLastIndex--; // undo if we need to delete the typeArray
        }
        open62541NodeStore_release((UA_Node*)node);
    } while(++currentIndex <= currentLastIndex && retval == UA_STATUSCODE_GOOD);

    if(retval)
        UA_Array_delete(typeArray, currentLastIndex, &UA_TYPES[UA_NODEID]);
    else {
        *referenceTypes = typeArray;
        *referenceTypesSize = currentLastIndex + 1;
    }

    return retval;
}

/* Results for a single browsedescription. */
static void getBrowseResult(open62541NodeStore *ns, const UA_BrowseDescription *browseDescription,
                            UA_UInt32 maxReferences, UA_BrowseResult *browseResult) {
    UA_UInt32  relevantReferenceTypesSize = 0;
    UA_NodeId *relevantReferenceTypes = UA_NULL;

    // if the referencetype is null, all referencetypes are returned
    UA_Boolean returnAll = UA_NodeId_isNull(&browseDescription->referenceTypeId);
    if(!returnAll) {
        if(browseDescription->includeSubtypes) {
            browseResult->statusCode = findRelevantReferenceTypes(ns, &browseDescription->referenceTypeId,
                                                                  &relevantReferenceTypes, &relevantReferenceTypesSize);
            if(browseResult->statusCode != UA_STATUSCODE_GOOD)
                return;
        } else {
            relevantReferenceTypes = (UA_NodeId*)&browseDescription->referenceTypeId; // is const
            relevantReferenceTypesSize = 1;
        }
    }

    const UA_Node *parentNode;
    if(open62541NodeStore_get(ns, &browseDescription->nodeId, &parentNode) != UA_STATUSCODE_GOOD) {
        browseResult->statusCode = UA_STATUSCODE_BADNODEIDUNKNOWN;
        if(!returnAll && browseDescription->includeSubtypes)
            UA_Array_delete(relevantReferenceTypes, relevantReferenceTypesSize, &UA_TYPES[UA_NODEID]);
        return;
    }

    // 0 => unlimited references
    if(maxReferences == 0 || maxReferences > UA_INT32_MAX || (UA_Int32)maxReferences > parentNode->referencesSize)
        maxReferences = parentNode->referencesSize;

    /* We allocate an array that is probably too big. But since most systems
       have more than enough memory, this has zero impact on speed and
       performance. Call Array_delete with the actual content size! */
    browseResult->references = UA_alloc(sizeof(UA_ReferenceDescription) * maxReferences);
    if(!browseResult->references) {
        browseResult->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
    } else {
        UA_UInt32 currentRefs = 0;
        for(UA_Int32 i = 0;i < parentNode->referencesSize && currentRefs < maxReferences;i++) {
            // 1) Is the node relevant? This might retrieve the node from the nodestore
            const UA_Node *currentNode = UA_NULL;
            if(!isRelevantTargetNode(ns, browseDescription, returnAll, &parentNode->references[i], &currentNode,
                                     relevantReferenceTypes, relevantReferenceTypesSize))
                continue;

            // 2) Fill the reference description. This also releases the current node.
            if(fillReferenceDescription(ns, currentNode, &parentNode->references[i], browseDescription->resultMask,
                                        &browseResult->references[currentRefs]) != UA_STATUSCODE_GOOD) {
                UA_Array_delete(browseResult->references, currentRefs, &UA_TYPES[UA_REFERENCEDESCRIPTION]);
                currentRefs = 0;
                browseResult->references = UA_NULL;
                browseResult->statusCode = UA_STATUSCODE_UNCERTAINNOTALLNODESAVAILABLE;
                break;
            }
            currentRefs++;
        }
        browseResult->referencesSize = currentRefs;
    }

    open62541NodeStore_release(parentNode);
    if(!returnAll && browseDescription->includeSubtypes)
        UA_Array_delete(relevantReferenceTypes, relevantReferenceTypesSize, &UA_TYPES[UA_NODEID]);
}

UA_Int32 open62541NodeStore_BrowseNodes(const UA_RequestHeader *requestHeader,UA_BrowseDescription *browseDescriptions,UA_UInt32 *indices,UA_UInt32 indicesSize, UA_UInt32 requestedMaxReferencesPerNode,
		UA_BrowseResult *browseResults,
		UA_DiagnosticInfo *diagnosticInfos){


	for(UA_UInt32 i = 0; i < indicesSize; i++){
		getBrowseResult(open62541NodeStore_getNodeStore(),&browseDescriptions[indices[i]],requestedMaxReferencesPerNode, &browseResults[indices[i]]);
	}
	return UA_STATUSCODE_GOOD;
}
