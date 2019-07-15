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

static Nodeset *nodeset = NULL;

struct nodeEntry
{
    UT_hash_handle hh;
    char *key;
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

const char *hierachicalReferences[MAX_HIERACHICAL_REFS] = {
    "Organizes",  "HasEventSource", "HasNotifier", "Aggregates",
    "HasSubtype", "HasComponent",   "HasProperty"};

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
            return UA_NODEID_NUMERIC(nsIdx, nodeId);
            break;
        }
        case 's':
        {
            return UA_NODEID_STRING_ALLOC(nsIdx, &idxSemi[3]);
            break;
        }
    }
    }
    return translateNodeId(namespaces, id);
}

/*
static UA_NodeId
    getReferenceTypeId(char *ref) {
    if(ref==NULL)
    {
        return UA_NODEID_NULL;
    }
    if(!strcmp(ref, "HasProperty"))
    {
        return UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    }
    else if (!strcmp(ref, "HasComponent"))
    {
        return UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    }
    else if (!strcmp(ref, "Organizes"))
    {
        return UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    }
    else if (!strcmp(ref, "HasTypeDefinition"))
    {
        return UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
    }
    else if(!strcmp(ref, "HasSubtype")) 
    {
        return UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
    } 
    else if(!strcmp(ref, "HasEncoding")) 
    {
        return UA_NODEID_NUMERIC(0, UA_NS0ID_HASENCODING);
    } 
    else {
        return extractNodedId(nodeset->namespaceTable->ns, ref);
    }
    return UA_NODEID_NULL;
}

*/

UA_NodeId alias2Id(const char *alias) {
    for(size_t cnt = 0; cnt < nodeset->aliasSize; cnt++) {
        if(!strcmp(alias, nodeset->aliasArray[cnt]->name)) {
            return nodeset->aliasArray[cnt]->id;
        }
    }
    return UA_NODEID_NULL;
}

void Nodeset_new(addNamespaceCb nsCallback) {
    nodeset = (Nodeset *)malloc(sizeof(Nodeset));
    nodeset->aliasArray = (Alias **)malloc(sizeof(Alias *) * MAX_ALIAS);
    nodeset->aliasSize = 0;
    nodeset->refsSize = 0;
    nodeset->countedChars = (const char **)malloc(sizeof(char *) * MAX_REFCOUNTEDCHARS);
    nodeset->charsSize = 0;
    // objects
    nodeset->nodes[NODECLASS_OBJECT] = (NodeContainer *)malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_OBJECT]->nodes =
        (UA_Node *)malloc(sizeof(UA_ObjectNode *) * MAX_OBJECTS);
    nodeset->nodes[NODECLASS_OBJECT]->cnt = 0;
    // variables
    nodeset->nodes[NODECLASS_VARIABLE] = (NodeContainer *)malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_VARIABLE]->nodes =
        (UA_Node *)malloc(sizeof(UA_VariableNode *) * MAX_VARIABLES);
    nodeset->nodes[NODECLASS_VARIABLE]->cnt = 0;
    // methods
    nodeset->nodes[NODECLASS_METHOD] = (NodeContainer *)malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_METHOD]->nodes =
        (UA_Node *)malloc(sizeof(UA_MethodNode*) * MAX_METHODS);
    nodeset->nodes[NODECLASS_METHOD]->cnt = 0;
    // objecttypes
    nodeset->nodes[NODECLASS_OBJECTTYPE] = (NodeContainer *)malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_OBJECTTYPE]->nodes =
        (UA_Node *)malloc(sizeof(UA_ObjectTypeNode *) * MAX_OBJECTTYPES);
    nodeset->nodes[NODECLASS_OBJECTTYPE]->cnt = 0;
    // datatypes
    nodeset->nodes[NODECLASS_DATATYPE] = (NodeContainer *)malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_DATATYPE]->nodes =
        (UA_Node *)malloc(sizeof(UA_DataTypeNode *) * MAX_DATATYPES);
    nodeset->nodes[NODECLASS_DATATYPE]->cnt = 0;
    // referencetypes
    nodeset->nodes[NODECLASS_REFERENCETYPE] =
        (NodeContainer *)malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_REFERENCETYPE]->nodes =
        (UA_Node *)malloc(sizeof(UA_ReferenceTypeNode *) * MAX_REFERENCETYPES);
    nodeset->nodes[NODECLASS_REFERENCETYPE]->cnt = 0;
    // variabletypes
    nodeset->nodes[NODECLASS_VARIABLETYPE] =
        (NodeContainer *)malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_VARIABLETYPE]->nodes =
        (UA_Node *)malloc(sizeof(UA_VariableTypeNode *) * MAX_VARIABLETYPES);
    nodeset->nodes[NODECLASS_VARIABLETYPE]->cnt = 0;
    // known hierachical refs
    nodeset->hierachicalRefs = hierachicalReferences;
    nodeset->hierachicalRefsSize = 7;

    TNamespaceTable *table = (TNamespaceTable *)malloc(sizeof(TNamespaceTable));
    table->cb = nsCallback;
    table->size = 1;
    table->ns = (TNamespace *)malloc((sizeof(TNamespace)));
    table->ns[0].idx = 0;
    table->ns[0].name = "http://opcfoundation.org/UA/";
    nodeset->namespaceTable = table;
}

void Nodeset_cleanup() {
    //Nodeset *n = nodeset;
    // free chars
    //for(size_t cnt = 0; cnt < n->charsSize; cnt++) {
    //    free_const(n->countedChars[cnt]);
    //}
    //free(n->countedChars);

    // free alias
    //for(size_t cnt = 0; cnt < n->aliasSize; cnt++) {
    //    free(n->aliasArray[cnt]);
    //}
    //free(n->aliasArray);

    //for(size_t cnt = 0; cnt < NODECLASS_COUNT; cnt++) {
    //    free((void *)n->nodes[cnt]->nodes);
    //    free((void *)n->nodes[cnt]);
    //}

    //free(n->namespaceTable->ns);
    //free(n->namespaceTable);
    //free(n);
}

/*
bool isHierachicalReference(const Reference *ref) {
    for(size_t i = 0; i < nodeset->hierachicalRefsSize; i++) {
        if(!strcmp(ref->refType.idString, nodeset->hierachicalRefs[i])) {
            return true;
        }
    }
    return false;
}
*/

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
        nodeset->countedChars[nodeset->charsSize++] = value;
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

static void extractAttributes(const TNamespace *namespaces, UA_Node *node,
                              int attributeSize, const char **attributes) {
    node->nodeId = extractNodedId(namespaces,
                              getAttributeValue(&attrNodeId, attributes, attributeSize));
    //todo: getBrowseName
    //node->browseName = getAttributeValue(&attrBrowseName, attributes, attributeSize);
    switch(node->nodeClass) {
        case UA_NODECLASS_OBJECTTYPE: 
            ((UA_ObjectTypeNode *)node)->isAbstract =
                getAttributeValue(&attrIsAbstract, attributes, attributeSize);
            break;
        case UA_NODECLASS_OBJECT:
            //todo
            //((UA_ObjectNode *)node)->eventNotifier =
            //    getAttributeValue(&attrEventNotifier, attributes, attributeSize);
            break;
        case UA_NODECLASS_VARIABLE:
        {
            
            char *datatype = getAttributeValue(&attrDataType, attributes, attributeSize);
            UA_NodeId aliasId = alias2Id(datatype);
            if(!UA_NodeId_equal(&aliasId, &UA_NODEID_NULL)) {
                ((UA_VariableNode *)node)->dataType= aliasId;
            } else {
                ((UA_VariableNode *)node)->dataType = extractNodedId(namespaces, datatype);
            }
            //todo: fix this
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
            UA_NodeId aliasId = alias2Id(datatype);
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
        case UA_NODECLASS_REFERENCETYPE:;
            break;
        default:;
    }
}

static void initNode(TNamespace *namespaces, UA_Node *node,
                     int nb_attributes, const char **attributes) {
    extractAttributes(namespaces, node, nb_attributes, attributes);
}

UA_Node *Nodeset_newNode(TNodeClass nodeClass, int nb_attributes, const char **attributes) {
    
    size_t cnt = nodeset->nodes[nodeClass]->cnt;
    UA_Node *newNode = nodeset->nodes[nodeClass]->nodes + cnt;
    newNode->nodeClass = UA_NODECLASSES[nodeClass];
    nodeset->nodes[nodeClass]->cnt++;
    initNode(nodeset->namespaceTable->ns, newNode, nb_attributes, attributes);

    //works currently only for nodes with string nodeIds
    struct nodeEntry* n = (struct nodeEntry*)malloc(sizeof(struct nodeEntry));
    n->key = (char*) newNode->nodeId.identifier.string.data;
    n->node = newNode;
    HASH_ADD_KEYPTR(hh, nodeHead, n->key, newNode->nodeId.identifier.string.length, n);

    printf("nodeCnt: %d\n", HASH_COUNT(nodeHead));

    return newNode;    
}

const UA_Node * Nodeset_getNode(const UA_NodeId *nodeId)
{
    struct nodeEntry *entry = NULL;
    HASH_FIND(hh, nodeHead, nodeId->identifier.string.data,
              nodeId->identifier.string.length, entry);
    if(entry)
        return entry->node;
    return NULL;
}

void Nodeset_newReference(UA_Node *node, int attributeSize, const char **attributes) {

    //node->referencesSize++;
    //UA_NodeReferenceKind *refs =
    //    (UA_NodeReferenceKind *)realloc(node->references, sizeof(UA_NodeReferenceKind)*node->referencesSize);
    //node->references = refs;
    //UA_NodeReferenceKind *newRef = refs + node->referencesSize - 1;

    //if(!strcmp("true", getAttributeValue(&attrIsForward, attributes, attributeSize))) {
    //    newRef->isInverse = false;
    //} else {
    //    newRef->isInverse = true;
    //}
    //newRef->referenceTypeId = 
    //    extractNodedId(nodeset->namespaceTable->ns,
    //                   getAttributeValue(&attrReferenceType, attributes, attributeSize));    
}

Alias *Nodeset_newAlias(int attributeSize, const char **attributes) {
    nodeset->aliasArray[nodeset->aliasSize] = (Alias *)malloc(sizeof(Alias));
    nodeset->aliasArray[nodeset->aliasSize]->name =
        getAttributeValue(&attrAlias, attributes, attributeSize);
    return nodeset->aliasArray[nodeset->aliasSize];
}

void Nodeset_newAliasFinish(char* idString) {
    nodeset->aliasArray[nodeset->aliasSize]->id =
        extractNodedId(nodeset->namespaceTable->ns,
                       idString);
    nodeset->aliasSize++;
}

TNamespace* Nodeset_newNamespace()
{
    nodeset->namespaceTable->size++;
    TNamespace *ns =
        (TNamespace *)realloc(nodeset->namespaceTable->ns,
                              sizeof(TNamespace) * (nodeset->namespaceTable->size));
    nodeset->namespaceTable->ns = ns;
    ns[nodeset->namespaceTable->size - 1].name = NULL;
    return &ns[nodeset->namespaceTable->size - 1];
}

void Nodeset_newNamespaceFinish(void* userContext, char* namespaceUri)
{
    nodeset->namespaceTable->ns[nodeset->namespaceTable->size - 1].name = namespaceUri;
    int globalIdx = nodeset->namespaceTable->cb(
        userContext, nodeset->namespaceTable->ns[nodeset->namespaceTable->size - 1].name);

    nodeset->namespaceTable->ns[nodeset->namespaceTable->size - 1].idx =
        (UA_UInt16)globalIdx;
}

void Nodeset_newNodeFinish(UA_Node* node)
{
}

void Nodeset_newReferenceFinish(UA_Node* node, char* targetId)
{

    //UA_NodeReferenceKind *ref = &node->references[node->referencesSize - 1];
    //ref->targetIdsSize = 1;
    //ref->targetIds = (UA_ExpandedNodeId*)malloc(sizeof(UA_ExpandedNodeId));

    //todo: namespaceuri, serverindex missing
    //ref->targetIds->nodeId = extractNodedId(nodeset->namespaceTable->ns, targetId);
}

void Nodeset_addRefCountedChar(char *newChar)
{
    nodeset->countedChars[nodeset->charsSize++] = newChar;
}
