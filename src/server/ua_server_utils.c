/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2016-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2016 (c) Lorenz Haas
 *    Copyright 2017 (c) frax2222
 *    Copyright 2017 (c) Florian Palm
 *    Copyright 2017-2018 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Julian Grothoff
 */

#include "ua_server_internal.h"

const UA_DataType *
UA_Server_findDataType(UA_Server *server, const UA_NodeId *typeId) {
    return UA_findDataTypeWithCustom(typeId, server->config.customDataTypes);
}

/********************************/
/* Information Model Operations */
/********************************/

static void *
returnFirstType(void *context, UA_ReferenceTarget *t) {
    UA_Server *server = (UA_Server*)context;
    /* Don't release the node that is returned.
     * Continues to iterate if NULL is returned. */
    return (void*)(uintptr_t)UA_NODESTORE_GETFROMREF(server, t->targetId);
}

const UA_Node *
getNodeType(UA_Server *server, const UA_NodeHead *head) {
    /* The reference to the parent is different for variable and variabletype */
    UA_Byte parentRefIndex;
    UA_Boolean inverse;
    switch(head->nodeClass) {
    case UA_NODECLASS_OBJECT:
    case UA_NODECLASS_VARIABLE:
        parentRefIndex = UA_REFERENCETYPEINDEX_HASTYPEDEFINITION;
        inverse = false;
        break;
    case UA_NODECLASS_OBJECTTYPE:
    case UA_NODECLASS_VARIABLETYPE:
    case UA_NODECLASS_REFERENCETYPE:
    case UA_NODECLASS_DATATYPE:
        parentRefIndex = UA_REFERENCETYPEINDEX_HASSUBTYPE;
        inverse = true;
        break;
    default:
        return NULL;
    }

    /* Return the first matching candidate */
    for(size_t i = 0; i < head->referencesSize; ++i) {
        UA_NodeReferenceKind *rk = &head->references[i];
        if(rk->isInverse != inverse)
            continue;
        if(rk->referenceTypeIndex != parentRefIndex)
            continue;
        const UA_Node *type = (const UA_Node*)
            UA_NodeReferenceKind_iterate(rk, returnFirstType, server);
        if(type)
            return type;
    }

    return NULL;
}

UA_Boolean
UA_Node_hasSubTypeOrInstances(const UA_NodeHead *head) {
    for(size_t i = 0; i < head->referencesSize; ++i) {
        if(head->references[i].isInverse == false &&
           head->references[i].referenceTypeIndex == UA_REFERENCETYPEINDEX_HASSUBTYPE)
            return true;
        if(head->references[i].isInverse == true &&
           head->references[i].referenceTypeIndex == UA_REFERENCETYPEINDEX_HASTYPEDEFINITION)
            return true;
    }
    return false;
}

UA_StatusCode
getTypeAndInterfaceHierarchy(UA_Server *server, const UA_NodeId *leafNode,
                             UA_Boolean includeLeaf, UA_NodeId **typeHierarchy,
                             size_t *typeHierarchySize) {
    UA_ReferenceTypeSet hastype = UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASTYPEDEFINITION);
    UA_ReferenceTypeSet hassubtype = UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASSUBTYPE);
    UA_ReferenceTypeSet hasinterface = UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASINTERFACE);

    /* Initialize the tree and add the leaf */
    RefTree rt;
    UA_StatusCode res = RefTree_init(&rt);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = RefTree_addNodeId(&rt, leafNode, NULL);
    if(res != UA_STATUSCODE_GOOD)
        goto errout;

    /* Get all types */
    res = browseRecursiveRefTree(server, &rt, UA_BROWSEDIRECTION_FORWARD, &hastype,
                                 UA_NODECLASS_OBJECTTYPE | UA_NODECLASS_VARIABLETYPE);
    if(res != UA_STATUSCODE_GOOD)
        goto errout;

    /* Get all super types */
    res = browseRecursiveRefTree(server, &rt, UA_BROWSEDIRECTION_INVERSE, &hassubtype,
                                 UA_NODECLASS_OBJECTTYPE | UA_NODECLASS_VARIABLETYPE);
    if(res != UA_STATUSCODE_GOOD)
        goto errout;

    /* Get all interfaces */
    res = browseRecursiveRefTree(server, &rt, UA_BROWSEDIRECTION_FORWARD, &hasinterface,
                                 UA_NODECLASS_OBJECTTYPE | UA_NODECLASS_VARIABLETYPE);

 errout:
    if(res != UA_STATUSCODE_GOOD || rt.size == 0) {
        RefTree_clear(&rt);
        return res;
    }

    /* Make the array of ExpandedNodeId into an array of NodeId */
    UA_NodeId *outArray = (UA_NodeId*)rt.targets;
    size_t pos = 0;
    for(size_t i = 0; i < rt.size; i++) {
        UA_NodeId *n = &outArray[pos];
        UA_ExpandedNodeId *e = &rt.targets[i];
        if(!UA_ExpandedNodeId_isLocal(e)) {
            UA_ExpandedNodeId_clear(e);
            continue;
        }
        *n = e->nodeId;
        UA_String_clear(&e->namespaceUri);
        pos++;
    }

    *typeHierarchySize = pos;
    *typeHierarchy = outArray;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
getAllInterfaces(UA_Server *server, const UA_NodeId *objectNode,
                 UA_NodeId **interfaceNodes, size_t *interfaceNodesSize) {
    UA_ReferenceTypeSet hastype = UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASTYPEDEFINITION);
    UA_ReferenceTypeSet hassubtype = UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASSUBTYPE);
    UA_ReferenceTypeSet hasinterface = UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASINTERFACE);

    /* Initialize the tree and add the leaf */
    size_t beforeInterfaces = 0;
    RefTree rt;
    UA_StatusCode res = RefTree_init(&rt);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = RefTree_addNodeId(&rt, objectNode, NULL);
    if(res != UA_STATUSCODE_GOOD)
        goto errout;

    /* Get all types */
    res = browseRecursiveRefTree(server, &rt, UA_BROWSEDIRECTION_FORWARD, &hastype,
                                 UA_NODECLASS_OBJECTTYPE | UA_NODECLASS_VARIABLETYPE);
    if(res != UA_STATUSCODE_GOOD)
        goto errout;

    /* Get all super types */
    res = browseRecursiveRefTree(server, &rt, UA_BROWSEDIRECTION_INVERSE, &hassubtype,
                                 UA_NODECLASS_OBJECTTYPE | UA_NODECLASS_VARIABLETYPE);
    if(res != UA_STATUSCODE_GOOD)
        goto errout;

    /* Get all interfaces */
    beforeInterfaces = rt.size; /* Return only the interfaces */
    res = browseRecursiveRefTree(server, &rt, UA_BROWSEDIRECTION_FORWARD, &hasinterface,
                                 UA_NODECLASS_OBJECTTYPE | UA_NODECLASS_VARIABLETYPE);

 errout:
    if(res != UA_STATUSCODE_GOOD || rt.size == 0) {
        RefTree_clear(&rt);
        return res;
    }

    /* Make the array of ExpandedNodeId into an array of NodeId */
    UA_NodeId *outArray = (UA_NodeId*)rt.targets;
    size_t pos = 0;
    for(size_t i = 0; i < rt.size; i++) {
        UA_NodeId *n = &outArray[pos];
        UA_ExpandedNodeId *e = &rt.targets[i];
        if(i < beforeInterfaces || !UA_ExpandedNodeId_isLocal(e)) {
            UA_ExpandedNodeId_clear(e);
            continue;
        }
        *n = e->nodeId;
        UA_String_clear(&e->namespaceUri);
        pos++;
    }

    /* No interfaces found */
    if(pos == 0) {
        RefTree_clear(&rt);
        outArray = NULL;
    }

    *interfaceNodesSize = pos;
    *interfaceNodes = outArray;
    return UA_STATUSCODE_GOOD;
}

/* Get the node, make the changes and release */
UA_StatusCode
editNode(UA_Server *server, UA_Session *session, const UA_NodeId *nodeId,
         UA_UInt32 attributeMask, UA_ReferenceTypeSet references,
         UA_BrowseDirection referenceDirections,
         UA_EditNodeCallback callback, void *data) {
    UA_Node *node =
        UA_NODESTORE_GET_EDIT_SELECTIVE(server, nodeId, attributeMask,
                                        references, referenceDirections);
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    UA_StatusCode retval = callback(server, session, node, data);
    UA_NODESTORE_RELEASE(server, node);
    return retval;
}

/*********************************/
/* Default attribute definitions */
/*********************************/

const UA_ObjectAttributes UA_ObjectAttributes_default = {
    0,                      /* specifiedAttributes */
    {{0, NULL}, {0, NULL}}, /* displayName */
    {{0, NULL}, {0, NULL}}, /* description */
    0, 0,                   /* writeMask (userWriteMask) */
    0                       /* eventNotifier */
};

const UA_VariableAttributes UA_VariableAttributes_default = {
    0,                           /* specifiedAttributes */
    {{0, NULL}, {0, NULL}},      /* displayName */
    {{0, NULL}, {0, NULL}},      /* description */
    0, 0,                        /* writeMask (userWriteMask) */
    {NULL, UA_VARIANT_DATA,
     0, NULL, 0, NULL},          /* value */
    {0, UA_NODEIDTYPE_NUMERIC,
     {UA_NS0ID_BASEDATATYPE}},   /* dataType */
    UA_VALUERANK_ANY,            /* valueRank */
    0, NULL,                     /* arrayDimensions */
    UA_ACCESSLEVELMASK_READ |    /* accessLevel */
    UA_ACCESSLEVELMASK_STATUSWRITE |
    UA_ACCESSLEVELMASK_TIMESTAMPWRITE,
    0,                           /* userAccessLevel */
    0.0,                         /* minimumSamplingInterval */
    false                        /* historizing */
};

const UA_MethodAttributes UA_MethodAttributes_default = {
    0,                      /* specifiedAttributes */
    {{0, NULL}, {0, NULL}}, /* displayName */
    {{0, NULL}, {0, NULL}}, /* description */
    0, 0,                   /* writeMask (userWriteMask) */
    true, true              /* executable (userExecutable) */
};

const UA_ObjectTypeAttributes UA_ObjectTypeAttributes_default = {
    0,                      /* specifiedAttributes */
    {{0, NULL}, {0, NULL}}, /* displayName */
    {{0, NULL}, {0, NULL}}, /* description */
    0, 0,                   /* writeMask (userWriteMask) */
    false                   /* isAbstract */
};

const UA_VariableTypeAttributes UA_VariableTypeAttributes_default = {
    0,                           /* specifiedAttributes */
    {{0, NULL}, {0, NULL}},      /* displayName */
    {{0, NULL}, {0, NULL}},      /* description */
    0, 0,                        /* writeMask (userWriteMask) */
    {NULL, UA_VARIANT_DATA,
     0, NULL, 0, NULL},          /* value */
    {0, UA_NODEIDTYPE_NUMERIC,
     {UA_NS0ID_BASEDATATYPE}},   /* dataType */
    UA_VALUERANK_ANY,            /* valueRank */
    0, NULL,                     /* arrayDimensions */
    false                        /* isAbstract */
};

const UA_ReferenceTypeAttributes UA_ReferenceTypeAttributes_default = {
    0,                      /* specifiedAttributes */
    {{0, NULL}, {0, NULL}}, /* displayName */
    {{0, NULL}, {0, NULL}}, /* description */
    0, 0,                   /* writeMask (userWriteMask) */
    false,                  /* isAbstract */
    false,                  /* symmetric */
    {{0, NULL}, {0, NULL}}  /* inverseName */
};

const UA_DataTypeAttributes UA_DataTypeAttributes_default = {
    0,                      /* specifiedAttributes */
    {{0, NULL}, {0, NULL}}, /* displayName */
    {{0, NULL}, {0, NULL}}, /* description */
    0, 0,                   /* writeMask (userWriteMask) */
    false                   /* isAbstract */
};

const UA_ViewAttributes UA_ViewAttributes_default = {
    0,                      /* specifiedAttributes */
    {{0, NULL}, {0, NULL}}, /* displayName */
    {{0, NULL}, {0, NULL}}, /* description */
    0, 0,                   /* writeMask (userWriteMask) */
    false,                  /* containsNoLoops */
    0                       /* eventNotifier */
};

