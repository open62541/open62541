/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#include "nodeset.h"
//#include "sort.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define free_const(x) free((void *)(long)(x))

static Nodeset *nodeset = NULL;

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

const char *hierachicalReferences[MAX_HIERACHICAL_REFS] = {
    "Organizes",  "HasEventSource", "HasNotifier", "Aggregates",
    "HasSubtype", "HasComponent",   "HasProperty"};

TNodeId translateNodeId(const TNamespace *namespaces, TNodeId id) {
    if(id.nsIdx > 0) {
        id.nsIdx = (int)namespaces[id.nsIdx].idx;
        return id;
    }
    return id;
}

TNodeId extractNodedId(const TNamespace *namespaces, char *s) {
    if(s == NULL) {
        TNodeId id;
        id.id = 0;
        id.nsIdx = 0;
        id.idString = "null";
        return id;
    }
    TNodeId id;
    id.nsIdx = 0;
    id.idString = s;
    char *idxSemi = strchr(s, ';');
    if(idxSemi == NULL) {
        id.id = s;
        return id;
    } else {
        id.nsIdx = atoi(&s[3]);
        id.id = idxSemi + 1;
    }
    return translateNodeId(namespaces, id);
}

TNodeId alias2Id(const char *alias) {
    for(size_t cnt = 0; cnt < nodeset->aliasSize; cnt++) {
        if(strEqual(alias, nodeset->aliasArray[cnt]->name)) {
            return nodeset->aliasArray[cnt]->id;
        }
    }
    TNodeId id;
    id.id = 0;
    return id;
}

void Nodeset_new(addNamespaceCb nsCallback) {
    nodeset = (Nodeset *)malloc(sizeof(Nodeset));
    nodeset->aliasArray = (Alias **)malloc(sizeof(Alias *) * MAX_ALIAS);
    nodeset->aliasSize = 0;
    nodeset->countedRefs =
        (const Reference **)malloc(sizeof(Reference *) * MAX_REFCOUNTEDREFS);
    nodeset->refsSize = 0;
    nodeset->countedChars = (const char **)malloc(sizeof(char *) * MAX_REFCOUNTEDCHARS);
    nodeset->charsSize = 0;
    // objects
    nodeset->nodes[NODECLASS_OBJECT] = (NodeContainer *)malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_OBJECT]->nodes =
        (TNode **)malloc(sizeof(TNode *) * MAX_OBJECTS);
    nodeset->nodes[NODECLASS_OBJECT]->cnt = 0;
    // variables
    nodeset->nodes[NODECLASS_VARIABLE] = (NodeContainer *)malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_VARIABLE]->nodes =
        (TNode **)malloc(sizeof(TNode *) * MAX_VARIABLES);
    nodeset->nodes[NODECLASS_VARIABLE]->cnt = 0;
    // methods
    nodeset->nodes[NODECLASS_METHOD] = (NodeContainer *)malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_METHOD]->nodes =
        (TNode **)malloc(sizeof(TNode *) * MAX_METHODS);
    nodeset->nodes[NODECLASS_METHOD]->cnt = 0;
    // objecttypes
    nodeset->nodes[NODECLASS_OBJECTTYPE] = (NodeContainer *)malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_OBJECTTYPE]->nodes =
        (TNode **)malloc(sizeof(TNode *) * MAX_DATATYPES);
    nodeset->nodes[NODECLASS_OBJECTTYPE]->cnt = 0;
    // datatypes
    nodeset->nodes[NODECLASS_DATATYPE] = (NodeContainer *)malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_DATATYPE]->nodes =
        (TNode **)malloc(sizeof(TNode *) * MAX_DATATYPES);
    nodeset->nodes[NODECLASS_DATATYPE]->cnt = 0;
    // referencetypes
    nodeset->nodes[NODECLASS_REFERENCETYPE] =
        (NodeContainer *)malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_REFERENCETYPE]->nodes =
        (TNode **)malloc(sizeof(TNode *) * MAX_REFERENCETYPES);
    nodeset->nodes[NODECLASS_REFERENCETYPE]->cnt = 0;
    // variabletypes
    nodeset->nodes[NODECLASS_VARIABLETYPE] =
        (NodeContainer *)malloc(sizeof(NodeContainer));
    nodeset->nodes[NODECLASS_VARIABLETYPE]->nodes =
        (TNode **)malloc(sizeof(TNode *) * MAX_VARIABLETYPES);
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
    // init sorting
    //init();
}

//void Nodeset_addNodeToSort(const TNode *node) { addNodeToSort(node); }


/*
bool Nodeset_getSortedNodes(void *userContext, addNodeCb callback) {

#ifdef XMLIMPORT_TRACE
    printf("--- namespace table ---\n");
    printf("FileIdx ServerIdx URI\n");
    for(size_t fileIndex = 0; fileIndex < nodeset->namespaceTable->size; fileIndex++) {
        printf("%zu\t%zu\t%s\n", fileIndex, nodeset->namespaceTable->ns[fileIndex].idx,
               nodeset->namespaceTable->ns[fileIndex].name);
    }
#endif

    if(!sort(Nodeset_addNode)) {
        return false;
    }

    for(size_t cnt = 0; cnt < nodeset->nodes[NODECLASS_REFERENCETYPE]->cnt; cnt++) {
        callback(userContext, nodeset->nodes[NODECLASS_REFERENCETYPE]->nodes[cnt]);
    }

    for(size_t cnt = 0; cnt < nodeset->nodes[NODECLASS_OBJECTTYPE]->cnt; cnt++) {
        callback(userContext, nodeset->nodes[NODECLASS_OBJECTTYPE]->nodes[cnt]);
    }

    for(size_t cnt = 0; cnt < nodeset->nodes[NODECLASS_OBJECT]->cnt; cnt++) {
        callback(userContext, nodeset->nodes[NODECLASS_OBJECT]->nodes[cnt]);
    }

    for(size_t cnt = 0; cnt < nodeset->nodes[NODECLASS_METHOD]->cnt; cnt++) {
        callback(userContext, nodeset->nodes[NODECLASS_METHOD]->nodes[cnt]);
    }

    for(size_t cnt = 0; cnt < nodeset->nodes[NODECLASS_DATATYPE]->cnt; cnt++) {
        callback(userContext, nodeset->nodes[NODECLASS_DATATYPE]->nodes[cnt]);
    }

    for(size_t cnt = 0; cnt < nodeset->nodes[NODECLASS_VARIABLETYPE]->cnt; cnt++) {
        callback(userContext, nodeset->nodes[NODECLASS_VARIABLETYPE]->nodes[cnt]);
    }

    for(size_t cnt = 0; cnt < nodeset->nodes[NODECLASS_VARIABLE]->cnt; cnt++) {
        callback(userContext, nodeset->nodes[NODECLASS_VARIABLE]->nodes[cnt]);
    }
    return true;
}
*/

void Nodeset_cleanup() {
    Nodeset *n = nodeset;
    // free chars
    for(size_t cnt = 0; cnt < n->charsSize; cnt++) {
        free_const(n->countedChars[cnt]);
    }
    free(n->countedChars);

    // free refs
    for(size_t cnt = 0; cnt < n->refsSize; cnt++) {
        free_const(n->countedRefs[cnt]);
    }
    free(n->countedRefs);

    // free alias
    for(size_t cnt = 0; cnt < n->aliasSize; cnt++) {
        free(n->aliasArray[cnt]);
    }
    free(n->aliasArray);

    for(size_t cnt = 0; cnt < NODECLASS_COUNT; cnt++) {
        size_t storedNodes = n->nodes[cnt]->cnt;
        for(size_t nodeCnt = 0; nodeCnt < storedNodes; nodeCnt++) {
            free_const(n->nodes[cnt]->nodes[nodeCnt]);
        }
        free((void *)n->nodes[cnt]->nodes);
        free((void *)n->nodes[cnt]);
    }

    free(n->namespaceTable->ns);
    free(n->namespaceTable);
    free(n);
}

bool isHierachicalReference(const Reference *ref) {
    for(size_t i = 0; i < nodeset->hierachicalRefsSize; i++) {
        if(!strcmp(ref->refType.idString, nodeset->hierachicalRefs[i])) {
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
        nodeset->countedChars[nodeset->charsSize++] = value;
        memcpy(value, value_start, size);
        value[size] = '\0';
        return value;
    }
    if(attr->defaultValue != NULL || attr->optional) {
        return attr->defaultValue;
    }
    // todo: remove this assertation

    printf("attribute: %s\n", attr->name);
    assertf(false, "attribute not found, no default value set in getAttributeValue\n");
}

static void extractAttributes(const TNamespace *namespaces, TNode *node,
                              int attributeSize, const char **attributes) {
    node->id = extractNodedId(namespaces,
                              getAttributeValue(&attrNodeId, attributes, attributeSize));
    node->browseName = getAttributeValue(&attrBrowseName, attributes, attributeSize);
    switch(node->nodeClass) {
        case NODECLASS_OBJECTTYPE: {
            ((TObjectTypeNode *)node)->isAbstract =
                getAttributeValue(&attrIsAbstract, attributes, attributeSize);
            break;
        }
        case NODECLASS_OBJECT: {
            ((TObjectNode *)node)->parentNodeId =
                extractNodedId(namespaces, getAttributeValue(&attrParentNodeId,
                                                             attributes, attributeSize));
            ((TObjectNode *)node)->eventNotifier =
                getAttributeValue(&attrEventNotifier, attributes, attributeSize);
            break;
        }
        case NODECLASS_VARIABLE: {

            ((TVariableNode *)node)->parentNodeId =
                extractNodedId(namespaces, getAttributeValue(&attrParentNodeId,
                                                             attributes, attributeSize));
            char *datatype = getAttributeValue(&attrDataType, attributes, attributeSize);
            TNodeId aliasId = alias2Id(datatype);
            if(aliasId.id != 0) {
                ((TVariableNode *)node)->datatype = aliasId;
            } else {
                ((TVariableNode *)node)->datatype = extractNodedId(namespaces, datatype);
            }
            ((TVariableNode *)node)->valueRank =
                getAttributeValue(&attrValueRank, attributes, attributeSize);
            ((TVariableNode *)node)->arrayDimensions =
                getAttributeValue(&attrArrayDimensions, attributes, attributeSize);

            break;
        }
        case NODECLASS_VARIABLETYPE: {

            ((TVariableTypeNode *)node)->valueRank =
                getAttributeValue(&attrValueRank, attributes, attributeSize);
            char *datatype = getAttributeValue(&attrDataType, attributes, attributeSize);
            TNodeId aliasId = alias2Id(datatype);
            if(aliasId.id != 0) {
                ((TVariableTypeNode *)node)->datatype = aliasId;
            } else {
                ((TVariableTypeNode *)node)->datatype =
                    extractNodedId(namespaces, datatype);
            }
            ((TVariableTypeNode *)node)->arrayDimensions =
                getAttributeValue(&attrArrayDimensions, attributes, attributeSize);
            ((TVariableTypeNode *)node)->isAbstract =
                getAttributeValue(&attrIsAbstract, attributes, attributeSize);
            break;
        }
        case NODECLASS_DATATYPE:;
            break;
        case NODECLASS_METHOD:
            ((TMethodNode *)node)->parentNodeId =
                extractNodedId(namespaces, getAttributeValue(&attrParentNodeId,
                                                             attributes, attributeSize));
            break;
        case NODECLASS_REFERENCETYPE:;
            break;
        default:;
    }
}

static void initNode(TNamespace *namespaces, TNodeClass nodeClass, TNode *node,
                     int nb_attributes, const char **attributes) {
    node->nodeClass = nodeClass;
    node->hierachicalRefs = NULL;
    node->nonHierachicalRefs = NULL;
    node->browseName = NULL;
    node->displayName = NULL;
    node->description = NULL;
    node->writeMask = NULL;
    extractAttributes(namespaces, node, nb_attributes, attributes);
}

TNode *Nodeset_newNode(TNodeClass nodeClass, int nb_attributes, const char **attributes) {
    //add link to hashtable here

    size_t cnt = nodeset->nodes[nodeClass]->cnt;
    TNode *newNode = nodeset->nodes[nodeClass]->nodes[cnt];    
    nodeset->nodes[nodeClass]->cnt++;
    initNode(nodeset->namespaceTable->ns, nodeClass, newNode, nb_attributes, attributes);
    return newNode;    
}

Reference *Nodeset_newReference(TNode *node, int attributeSize, const char **attributes) {
    Reference *newRef = (Reference *)malloc(sizeof(Reference));
    newRef->target.idString = NULL;
    newRef->target.id = NULL;
    newRef->refType.idString = NULL;
    newRef->refType.id = NULL;
    nodeset->countedRefs[nodeset->refsSize++] = newRef;
    newRef->next = NULL;
    if(strEqual("true", getAttributeValue(&attrIsForward, attributes, attributeSize))) {
        newRef->isForward = true;
    } else {
        newRef->isForward = false;
    }
    newRef->refType =
        extractNodedId(nodeset->namespaceTable->ns,
                       getAttributeValue(&attrReferenceType, attributes, attributeSize));
    if(isHierachicalReference(newRef)) {
        Reference **lastRef = &node->hierachicalRefs;
        while(*lastRef) {
            lastRef = &(*lastRef)->next;
        }
        *lastRef = newRef;
    } else {
        Reference **lastRef = &node->nonHierachicalRefs;
        while(*lastRef) {
            lastRef = &(*lastRef)->next;
        }
        *lastRef = newRef;
    }
    return newRef;
}

Alias *Nodeset_newAlias(int attributeSize, const char **attributes) {
    nodeset->aliasArray[nodeset->aliasSize] = (Alias *)malloc(sizeof(Alias));
    nodeset->aliasArray[nodeset->aliasSize]->id.idString = NULL;
    nodeset->aliasArray[nodeset->aliasSize]->name =
        getAttributeValue(&attrAlias, attributes, attributeSize);
    return nodeset->aliasArray[nodeset->aliasSize];
}

void Nodeset_newAliasFinish() {
    nodeset->aliasArray[nodeset->aliasSize]->id =
        extractNodedId(nodeset->namespaceTable->ns,
                       nodeset->aliasArray[nodeset->aliasSize]->id.idString);
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

void Nodeset_newNamespaceFinish(void* userContext)
{
    int globalIdx = nodeset->namespaceTable->cb(
        userContext,
        nodeset->namespaceTable->ns[nodeset->namespaceTable->size - 1].name);

    nodeset->namespaceTable->ns[nodeset->namespaceTable->size - 1].idx =
        (size_t)globalIdx;
}

void Nodeset_newNodeFinish(TNode* node)
{
    //Nodeset_addNodeToSort(node);
    if(node->nodeClass == NODECLASS_REFERENCETYPE) {
        Reference *ref = node->hierachicalRefs;
        while(ref) {
            if(!ref->isForward) {
                nodeset->hierachicalRefs[nodeset->hierachicalRefsSize++] =
                    node->id.idString;
                break;
            }
            ref = ref->next;
        }
    }
}

void Nodeset_newReferenceFinish(TNode* node)
{
    Reference *ref = node->hierachicalRefs;
    while(ref) {
        ref->target = extractNodedId(nodeset->namespaceTable->ns, ref->target.idString);
        ref = ref->next;
    }
    ref = node->nonHierachicalRefs;
    while(ref) {
        ref->target = extractNodedId(nodeset->namespaceTable->ns, ref->target.idString);
        ref = ref->next;
    }
}

void Nodeset_addRefCountedChar(char *newChar)
{
    nodeset->countedChars[nodeset->charsSize++] = newChar;
}
