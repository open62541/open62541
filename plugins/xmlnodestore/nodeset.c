/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#include "nodeset.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uthash.h"
#include "memory.h"
#include <open62541/util.h>

struct nodeEntry
{
    UT_hash_handle hh;
    char *cKey;
    int iKey;
    UA_Node *node;
};

struct nodeEntry *nodeHead = NULL;

// UANode
#define ATTRIBUTE_NODEID "NodeId"
#define ATTRIBUTE_BROWSENAME "BrowseName"
// UAInstance
#define ATTRIBUTE_PARENTNODEID "ParentNodeId"
// UAVariable
#define ATTRIBUTE_DATATYPE "DataType"
#define ATTRIBUTE_VALUERANK "ValueRank"
#define ATTRIBUTE_ARRAYDIMENSIONS "ArrayDimensions"
// UAObject
#define ATTRIBUTE_EVENTNOTIFIER "EventNotifier"
// UAObjectType
#define ATTRIBUTE_ISABSTRACT "IsAbstract"
// Reference
#define ATTRIBUTE_REFERENCETYPE "ReferenceType"
#define ATTRIBUTE_ISFORWARD "IsForward"
#define ATTRIBUTE_SYMMETRIC "Symmetric"
#define ATTRIBUTE_ALIAS "Alias"

NodeAttribute attrNodeId = {ATTRIBUTE_NODEID, NULL, false};
NodeAttribute attrBrowseName = {ATTRIBUTE_BROWSENAME, NULL, false};
NodeAttribute attrParentNodeId = {ATTRIBUTE_PARENTNODEID, NULL, true};
NodeAttribute attrEventNotifier = {ATTRIBUTE_EVENTNOTIFIER, NULL, true};
NodeAttribute attrDataType = {ATTRIBUTE_DATATYPE, "i=24", false};
NodeAttribute attrValueRank = {ATTRIBUTE_VALUERANK, "-1", false};
NodeAttribute attrArrayDimensions = {ATTRIBUTE_ARRAYDIMENSIONS, "", false};
NodeAttribute attrIsAbstract = {ATTRIBUTE_ARRAYDIMENSIONS, "false", false};
NodeAttribute attrIsForward = {ATTRIBUTE_ISFORWARD, "true", false};
NodeAttribute attrReferenceType = {ATTRIBUTE_REFERENCETYPE, NULL, true};
NodeAttribute attrAlias = {ATTRIBUTE_ALIAS, NULL, false};

const UA_NodeClass UA_NODECLASSES[NODECLASS_COUNT] = {
    UA_NODECLASS_OBJECT,      UA_NODECLASS_OBJECTTYPE, UA_NODECLASS_VARIABLE,
    UA_NODECLASS_DATATYPE,    UA_NODECLASS_METHOD,     UA_NODECLASS_REFERENCETYPE,
    UA_NODECLASS_VARIABLETYPE};


//hierachical references
UA_NodeId hierachicalRefs[MAX_HIERACHICAL_REFS] = {
    {.namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric = UA_NS0ID_ORGANIZES},
    {.namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric = UA_NS0ID_HASEVENTSOURCE},
    {.namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric = UA_NS0ID_HASNOTIFIER},
    {.namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric = UA_NS0ID_AGGREGATES},
    {.namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric = UA_NS0ID_HASSUBTYPE},
    {.namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric = UA_NS0ID_HASCOMPONENT},
    {.namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric = UA_NS0ID_HASPROPERTY}
    };

UA_NodeId translateNodeId(const TNamespace *namespaces, UA_NodeId id) {
    if(id.namespaceIndex > 0) {
        id.namespaceIndex = namespaces[id.namespaceIndex].idx;
        return id;
    }
    return id;
}

UA_NodeId extractNodedId(const TNamespace *namespaces, char *s) {
    if(s == NULL) {
        return UA_NODEID_NULL;
    }
    UA_NodeId id;
    id.namespaceIndex = 0;
    char *idxSemi = strchr(s, ';');


    //namespaceindex zero?
    if(idxSemi == NULL) {
        id.namespaceIndex = 0;
         switch(s[0]) {
        // integer
        case 'i': {
            UA_UInt32 nodeId = (UA_UInt32)atoi(&s[2]);
            return UA_NODEID_NUMERIC(0, nodeId);
            break;
        }
        case 's':
        {
            return UA_NODEID_STRING_ALLOC(0, &s[2]);
            break;
        }
    }
    } else {
        UA_UInt16 nsIdx= (UA_UInt16) atoi(&s[3]);
         switch(idxSemi[1]) {
        // integer
        case 'i': {
            UA_UInt32 nodeId = (UA_UInt32)atoi(&idxSemi[3]);
            id.namespaceIndex = nsIdx;
            id.identifierType = UA_NODEIDTYPE_NUMERIC;
            id.identifier.numeric = nodeId;
            break;
        }
        case 's':
        {
            UA_String sid = UA_STRING_ALLOC(&idxSemi[3]);
            id.namespaceIndex = nsIdx;
            id.identifierType = UA_NODEIDTYPE_STRING;
            id.identifier.string = sid;
            break;
        }
    }
    }    
    return translateNodeId(namespaces, id);
}

static bool isNodeId(const char* s)
{
    if(!strncmp(s, "ns=",3) || !strncmp(s, "i=", 2) || !strncmp(s, "s=", 2))
    {
        return true;
    }
    return false;
}

static UA_NodeId alias2Id(Nodeset* nodeset, const char *alias) {
    for(size_t cnt = 0; cnt < nodeset->aliasSize; cnt++) {
        if(!strcmp(alias, nodeset->aliasArray[cnt]->name)) {
            return nodeset->aliasArray[cnt]->id;
        }
    }
    return UA_NODEID_NULL;
}

Nodeset* Nodeset_new(addNamespaceCb nsCallback) {
    Nodeset* nodeset = (Nodeset *)malloc(sizeof(Nodeset));
    nodeset->aliasArray = (Alias **)malloc(sizeof(Alias *) * MAX_ALIAS);
    nodeset->aliasSize = 0;
    nodeset->refsSize = 0;
    nodeset->countedChars = (char **)malloc(sizeof(char *) * MAX_REFCOUNTEDCHARS);
    nodeset->charsSize = 0;
    // mem pools
    nodeset->nodes = (NodeContainer *)malloc(sizeof(NodeContainer)*NODECLASS_COUNT);
    nodeset->nodes[NODECLASS_OBJECT].nodePool =
    MemoryPool_init(sizeof(UA_ObjectNode), 1000);
    nodeset->nodes[NODECLASS_VARIABLE].nodePool = MemoryPool_init(sizeof(UA_VariableNode),
    1000);   
    nodeset->nodes[NODECLASS_METHOD].nodePool = MemoryPool_init(sizeof(UA_MethodNode),
    1000);   
    nodeset->nodes[NODECLASS_OBJECTTYPE].nodePool = MemoryPool_init(sizeof(UA_ObjectTypeNode),
    1000);
    nodeset->nodes[NODECLASS_VARIABLETYPE].nodePool = MemoryPool_init(sizeof(UA_VariableTypeNode),
    1000);
    nodeset->nodes[NODECLASS_REFERENCETYPE].nodePool = MemoryPool_init(sizeof(UA_ReferenceTypeNode),
    1000);
    nodeset->nodes[NODECLASS_DATATYPE].nodePool = MemoryPool_init(sizeof(UA_DataTypeNode),
    1000);

    // known hierachical refs
    nodeset->hierachicalRefs = hierachicalRefs;
    nodeset->hierachicalRefsSize = 7;
    //refs
    nodeset->refs = (TRef*) calloc(MAX_REFCOUNTEDREFS, sizeof(TRef));
    nodeset->refsSize =0;

    TNamespaceTable *table = (TNamespaceTable *)malloc(sizeof(TNamespaceTable));
    table->cb = nsCallback;
    table->size = 1;
    table->ns = (TNamespace *)malloc((sizeof(TNamespace)));
    table->ns[0].idx = 0;
    table->ns[0].name = "http://opcfoundation.org/UA/";
    nodeset->namespaceTable = table;
    return nodeset;
}

void Nodeset_cleanup(Nodeset* nodeset) {
    free(nodeset->countedChars);
    free(nodeset->refs);
}


static bool isHierachicalReference(Nodeset* nodeset, const UA_NodeId *refId) {
    for(size_t i = 0; i < nodeset->hierachicalRefsSize; i++) {
        if(UA_NodeId_equal(&nodeset->hierachicalRefs[i], refId))
        {
            return true;
        }
    }
    return false;
}


static char *getAttributeValue(NodeAttribute *attr, const char **attributes,
                               int nb_attributes) {
    const int fields = 5;
    for(int i = 0; i < nb_attributes; i++) {
        const char *localname = attributes[i * fields + 0];
        if(strcmp((const char *)localname, attr->name))
            continue;
        const char *value_start = attributes[i * fields + 3];
        const char *value_end = attributes[i * fields + 4];
        size_t size = (size_t)(value_end - value_start);
        char *value = (char *)malloc(sizeof(char) * size + 1);
        //todo: nodeset, refcount char
        //nodeset->countedChars[nodeset->charsSize++] = value;
        memcpy(value, value_start, size);
        value[size] = '\0';
        return value;
    }
    if(attr->defaultValue != NULL || attr->optional) {
        return attr->defaultValue;
    }
    else
    {
        printf("error attribute lookup\n");
        return NULL;
    }
}

static UA_DataSource noDataSource = {.read=NULL, .write=NULL};

static void extractAttributes(Nodeset* nodeset, const TNamespace *namespaces, UA_Node *node,
                              int attributeSize, const char **attributes) {
    node->nodeId = extractNodedId(namespaces,
                              getAttributeValue(&attrNodeId, attributes, attributeSize));
    //todo: split on : , e.g. 1:Test
    node->browseName = UA_QUALIFIEDNAME(2, getAttributeValue(&attrBrowseName, attributes, attributeSize));
    switch(node->nodeClass) {
        case UA_NODECLASS_OBJECTTYPE: 
            //((UA_ObjectTypeNode *)node)->isAbstract =
            //    getAttributeValue(&attrIsAbstract, attributes, attributeSize);
            break;
        case UA_NODECLASS_OBJECT:
            //todo
            //((UA_ObjectNode *)node)->eventNotifier =
            //    getAttributeValue(&attrEventNotifier, attributes, attributeSize);
            break;
        case UA_NODECLASS_VARIABLE:
        {
            
            char *datatype = getAttributeValue(&attrDataType, attributes, attributeSize);
            UA_NodeId aliasId = alias2Id(nodeset, datatype);
            if(!UA_NodeId_equal(&aliasId, &UA_NODEID_NULL)) {
                ((UA_VariableNode *)node)->dataType= aliasId;
            } else {
                ((UA_VariableNode *)node)->dataType = extractNodedId(namespaces, datatype);
            }
            ((UA_VariableNode *)node)->valueSource = UA_VALUESOURCE_DATASOURCE;
            ((UA_VariableNode *)node)->value.dataSource = noDataSource;
            ((UA_VariableNode *)node)->valueRank = -1;
            ((UA_VariableNode *)node)->writeMask = 0;
            ((UA_VariableNode *)node)->arrayDimensionsSize = 0;


            // todo: fix this
            //((UA_VariableNode *)node)->valueRank =
            //    getAttributeValue(&attrValueRank, attributes, attributeSize);
            //((UA_VariableNode *)node)->arrayDimensions =
            //    getAttributeValue(&attrArrayDimensions, attributes, attributeSize);

            break;
        }
        case UA_NODECLASS_VARIABLETYPE: {

            //todo
            //((UA_VariableTypeNode *)node)->valueRank =
            //    getAttributeValue(&attrValueRank, attributes, attributeSize);
            char *datatype = getAttributeValue(&attrDataType, attributes, attributeSize);
            UA_NodeId aliasId = alias2Id(nodeset, datatype);
            if(!UA_NodeId_equal(&aliasId, &UA_NODEID_NULL)) {
                ((UA_VariableTypeNode *)node)->dataType= aliasId;
            } else {
                ((UA_VariableTypeNode *)node)->dataType = extractNodedId(namespaces, datatype);
            }
            //((UA_VariableTypeNode *)node)->arrayDimensions =
            //    getAttributeValue(&attrArrayDimensions, attributes, attributeSize);
            //((UA_VariableTypeNode *)node)->isAbstract =
            //    getAttributeValue(&attrIsAbstract, attributes, attributeSize);
            break;
        }
        case UA_NODECLASS_DATATYPE:;
            break;
        case UA_NODECLASS_METHOD:
            //((UA_MethodNode *)node)->executable =
            //    extractNodedId(namespaces, getAttributeValue(&attrParentNodeId,
            //                                                 attributes, attributeSize));
            break;
        case UA_NODECLASS_REFERENCETYPE:
            ((UA_ReferenceTypeNode *)node)->inverseName =
                UA_LOCALIZEDTEXT_ALLOC("de", "inverse");
            break;
        default:;
    }
}

static void initNode(Nodeset* nodeset, TNamespace *namespaces, UA_Node *node,
                     int nb_attributes, const char **attributes) {
    extractAttributes(nodeset, namespaces, node, nb_attributes, attributes);
}

UA_Node *Nodeset_newNode(Nodeset* nodeset, TNodeClass nodeClass, int nb_attributes, const char **attributes) {
    

    UA_Node *newNode = NULL;
    switch(nodeClass) {
        case NODECLASS_OBJECTTYPE: 
            newNode = (UA_Node *)((UA_ObjectTypeNode*)MemoryPool_getMemoryForElement(nodeset->nodes[nodeClass].nodePool));
            break;
        case NODECLASS_OBJECT:
            newNode = (UA_Node *)((UA_ObjectNode*)MemoryPool_getMemoryForElement(nodeset->nodes[nodeClass].nodePool));
            break;
        case NODECLASS_VARIABLE:
            newNode = (UA_Node *)((UA_VariableNode*)MemoryPool_getMemoryForElement(nodeset->nodes[nodeClass].nodePool));
            break;
        case NODECLASS_VARIABLETYPE:
            newNode = (UA_Node *)((UA_VariableTypeNode*)MemoryPool_getMemoryForElement(nodeset->nodes[nodeClass].nodePool));
            break;
        case NODECLASS_DATATYPE:
            newNode = (UA_Node *)((UA_DataTypeNode*)MemoryPool_getMemoryForElement(nodeset->nodes[nodeClass].nodePool));
            break;
        case NODECLASS_METHOD:
            newNode = (UA_Node *)((UA_MethodNode*)MemoryPool_getMemoryForElement(nodeset->nodes[nodeClass].nodePool));
            break;
        case NODECLASS_REFERENCETYPE:
            newNode = (UA_Node *)((UA_ReferenceTypeNode*)MemoryPool_getMemoryForElement(nodeset->nodes[nodeClass].nodePool));
            break;
        default:
            newNode = NULL;
    }
    newNode->nodeClass = UA_NODECLASSES[nodeClass];
    initNode(nodeset, nodeset->namespaceTable->ns, newNode, nb_attributes, attributes);

    //works currently only for nodes with string nodeIds and numeric ones
    struct nodeEntry* n = (struct nodeEntry*)malloc(sizeof(struct nodeEntry));
    n->node = newNode;
    switch(newNode->nodeId.identifierType)
    {
        case UA_NODEIDTYPE_STRING:
            n->cKey = (char*) newNode->nodeId.identifier.string.data;
            HASH_ADD_KEYPTR(hh, nodeHead, n->cKey, newNode->nodeId.identifier.string.length, n);
            break;
        case UA_NODEIDTYPE_NUMERIC:
            n->iKey = (int)newNode->nodeId.identifier.numeric;
            HASH_ADD_INT(nodeHead, iKey, n);
            break;
        default:
            break;
    }
    return newNode;
}

UA_Node * Nodeset_getNode(const UA_NodeId *nodeId)
{
    struct nodeEntry *entry = NULL;
    switch(nodeId->identifierType)
    {
        case UA_NODEIDTYPE_STRING:
            HASH_FIND(hh, nodeHead, nodeId->identifier.string.data,
              nodeId->identifier.string.length, entry);
            break;
        case UA_NODEIDTYPE_NUMERIC:
            HASH_FIND_INT(nodeHead, &nodeId->identifier.numeric, entry);
            break;
        default:
            break;
    }

    if(entry)
    {
#ifdef XMLSTORE_TRACE
        UA_String s = UA_STRING_NULL;
        UA_NodeId_toString(&entry->node->nodeId,&s);
        printf("nodeId: %.*s\n", (int)s.length, s.data);

        for(size_t i=0; i <entry->node->referencesSize;i++)
        {
            UA_NodeReferenceKind refKind = entry->node->references[i];
            for(size_t j=0; j < refKind.targetIdsSize; j++)
            {
                UA_ExpandedNodeId id = refKind.targetIds[j];
                s = UA_STRING_NULL;
                UA_NodeId_toString(&id.nodeId,&s);
                printf("\ttargetId: %.*s\n", (int)s.length, s.data);
            }
        }
        printf("------\n");
#endif
        return entry->node;
    }        
    return NULL;
}


UA_NodeReferenceKind* Nodeset_newReference(Nodeset* nodeset, UA_Node *node, int attributeSize, const char **attributes) {
    UA_Boolean isForward = false;
    if(!strcmp("true", getAttributeValue(&attrIsForward, attributes, attributeSize))) {
        isForward = true;
    }

    char* s = getAttributeValue(&attrReferenceType, attributes, attributeSize);
    UA_NodeId refTypeId = UA_NODEID_NULL;
    if(!isNodeId(s))
    {
        //try it with alias
        refTypeId = translateNodeId(nodeset->namespaceTable->ns,alias2Id(nodeset, s));
    }
    else
    {
        refTypeId = extractNodedId(nodeset->namespaceTable->ns, s);
    }

    UA_NodeReferenceKind *existingRefs = NULL;
    for(size_t i = 0; i < node->referencesSize; ++i) {
        UA_NodeReferenceKind *refs = &node->references[i];
        if(refs->isInverse != isForward
                && UA_NodeId_equal(&refs->referenceTypeId, &refTypeId)) {
            existingRefs = refs;
            break;
        }
    }
    //if we have an existing referenceKind, use this one
    if(existingRefs != NULL) {
        return existingRefs;
    }

    node->referencesSize++;
    UA_NodeReferenceKind *refs =
        (UA_NodeReferenceKind *)realloc(node->references, sizeof(UA_NodeReferenceKind)*node->referencesSize);
    node->references = refs;
    UA_NodeReferenceKind *newRef = refs + node->referencesSize - 1;
    newRef->referenceTypeId = refTypeId;
    newRef->isInverse = !isForward;
    newRef->targetIdsSize = 0;
    newRef->targetIds = NULL;
    return newRef;
}

void Nodeset_linkReferences(Nodeset* nodeset, UA_Server* server)
{
    //iterate over all references, if it's an hierachical ref, insert the inverse ref
    // from UA Spec part 3, References:
    // It might not always be possible for Servers to instantiate both forward and inverse References
    // for non-symmetric ReferenceTypes as shown in Figure 9. When they do, the References are
    // referred to as bidirectional. Although not required, it is recommended that all hierarchical
    // References be instantiated as bidirectional to ensure browse connectivity. A bidirectional
    // Reference is modelled as two separate References

    for(size_t i=0; i<nodeset->refsSize; i++)
    {
        if(isHierachicalReference(nodeset, &nodeset->refs[i].ref->referenceTypeId))
        {
            for(size_t cnt=0; cnt < nodeset->refs[i].ref->targetIdsSize; cnt++)
            {
                //try it with server
                UA_ExpandedNodeId eId;
                eId.namespaceUri = UA_STRING_NULL;
                eId.nodeId = *nodeset->refs[i].src;
                eId.serverIndex = 0;
                UA_Server_addReference(server, nodeset->refs[i].ref->targetIds[cnt].nodeId, nodeset->refs[i].ref->referenceTypeId, eId, nodeset->refs[i].ref->isInverse);
            }
        }
    }
}

Alias *Nodeset_newAlias(Nodeset* nodeset, int attributeSize, const char **attributes) {
    nodeset->aliasArray[nodeset->aliasSize] = (Alias *)malloc(sizeof(Alias));
    nodeset->aliasArray[nodeset->aliasSize]->name =
        getAttributeValue(&attrAlias, attributes, attributeSize);
    return nodeset->aliasArray[nodeset->aliasSize];
}

void Nodeset_newAliasFinish(Nodeset* nodeset, char* idString) {
    nodeset->aliasArray[nodeset->aliasSize]->id =
        extractNodedId(nodeset->namespaceTable->ns,
                       idString);
    nodeset->aliasSize++;
}

TNamespace* Nodeset_newNamespace(Nodeset* nodeset)
{
    nodeset->namespaceTable->size++;
    TNamespace *ns =
        (TNamespace *)realloc(nodeset->namespaceTable->ns,
                              sizeof(TNamespace) * (nodeset->namespaceTable->size));
    nodeset->namespaceTable->ns = ns;
    ns[nodeset->namespaceTable->size - 1].name = NULL;
    return &ns[nodeset->namespaceTable->size - 1];
}

void Nodeset_newNamespaceFinish(Nodeset* nodeset, void* userContext, char* namespaceUri)
{
    nodeset->namespaceTable->ns[nodeset->namespaceTable->size - 1].name = namespaceUri;
    int globalIdx = nodeset->namespaceTable->cb(
        (UA_Server*)userContext, nodeset->namespaceTable->ns[nodeset->namespaceTable->size - 1].name);

    nodeset->namespaceTable->ns[nodeset->namespaceTable->size - 1].idx =
        (UA_UInt16)globalIdx;
}

void Nodeset_setDisplayname(UA_Node* node, char* s)
{
    node->displayName = UA_LOCALIZEDTEXT_ALLOC("de", s);
}

void Nodeset_newNodeFinish(Nodeset* nodeset, UA_Node* node)
{
    //add it to the known hierachical refs
    if(node->nodeClass == UA_NODECLASS_REFERENCETYPE) {
        for(size_t i = 0; i < node->referencesSize; i++)
        {
            if(node->references[i].isInverse)
            {
                nodeset->hierachicalRefs[nodeset->hierachicalRefsSize++] = node->nodeId;
                break;
            }
        }
    }

    //store all references
    for(size_t cnt = 0; cnt < node->referencesSize; cnt++)
    {
        nodeset->refs[nodeset->refsSize].ref = &node->references[cnt];
        nodeset->refs[nodeset->refsSize].src = &node->nodeId;
        nodeset->refsSize++;
    }
}

void Nodeset_newReferenceFinish(Nodeset* nodeset, UA_NodeReferenceKind* ref, char* targetId)
{
    //add target for every reference    
    UA_ExpandedNodeId *targets =
        (UA_ExpandedNodeId*) UA_realloc(ref->targetIds,
                                        sizeof(UA_ExpandedNodeId) * (ref->targetIdsSize+1));

    ref->targetIds = targets;
    ref->targetIds[ref->targetIdsSize].nodeId = extractNodedId(nodeset->namespaceTable->ns, targetId);
    ref->targetIds[ref->targetIdsSize].namespaceUri = UA_STRING_NULL;
    ref->targetIds[ref->targetIdsSize].serverIndex = 0;
    ref->targetIdsSize++;
}

void Nodeset_addRefCountedChar(Nodeset* nodeset, char *newChar)
{
    nodeset->countedChars[nodeset->charsSize++] = newChar;
}
