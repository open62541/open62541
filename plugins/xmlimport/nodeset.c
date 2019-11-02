/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */


#include <open62541/server.h>

#include "memory.h"
#include "conversion.h"
#include "nodeset.h"

#define MAX_BIDIRECTIONAL_REFS 50
#define BIDIRECTIONAL_REF_COUNT 8
#define MAX_REFCOUNTEDCHARS 10000000

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

typedef struct {
    const char *name;
    char *defaultValue;
    bool optional;
} NodeAttribute;

const NodeAttribute attrNodeId = {ATTRIBUTE_NODEID, NULL, false};
const NodeAttribute attrBrowseName = {ATTRIBUTE_BROWSENAME, NULL, false};
const NodeAttribute attrParentNodeId = {ATTRIBUTE_PARENTNODEID, NULL, true};
const NodeAttribute attrEventNotifier = {ATTRIBUTE_EVENTNOTIFIER, NULL, true};
const NodeAttribute attrDataType = {ATTRIBUTE_DATATYPE, "i=24", false};
const NodeAttribute attrValueRank = {ATTRIBUTE_VALUERANK, "-1", false};
const NodeAttribute attrArrayDimensions = {ATTRIBUTE_ARRAYDIMENSIONS, "", false};
const NodeAttribute attrIsAbstract = {ATTRIBUTE_ISABSTRACT, "false", false};
const NodeAttribute attrIsForward = {ATTRIBUTE_ISFORWARD, "true", false};
const NodeAttribute attrReferenceType = {ATTRIBUTE_REFERENCETYPE, NULL, true};
const NodeAttribute attrAlias = {ATTRIBUTE_ALIAS, NULL, false};
const NodeAttribute attrExecutable = {"Executable", "true", false};

struct Alias {
    char *name;
    UA_NodeId id;
};

struct TNamespace {
    UA_UInt16 idx;
    char *uri;
};

typedef struct {
    size_t size;
    TNamespace *ns;
    addNamespaceCb cb;
} TNamespaceTable;

typedef struct {
    UA_NodeId *src;
    UA_NodeReferenceKind *ref;
} TRef;

struct Nodeset {
    char **countedChars;
    struct MemoryPool *aliasPool;
    size_t charsSize;
    TNamespaceTable *namespaceTable;
    size_t hierachicalRefsSize;
    UA_NodeId * hierachicalRefs;
    struct MemoryPool *refPool;
    UA_Server *server;
};

/* start with hierachical and hasEncoding refs */
UA_NodeId bidirectionalRefs[MAX_BIDIRECTIONAL_REFS] = {
    {.namespaceIndex = 0,
     .identifierType = UA_NODEIDTYPE_NUMERIC,
     .identifier.numeric = UA_NS0ID_ORGANIZES},
    {.namespaceIndex = 0,
     .identifierType = UA_NODEIDTYPE_NUMERIC,
     .identifier.numeric = UA_NS0ID_HASEVENTSOURCE},
    {.namespaceIndex = 0,
     .identifierType = UA_NODEIDTYPE_NUMERIC,
     .identifier.numeric = UA_NS0ID_HASNOTIFIER},
    {.namespaceIndex = 0,
     .identifierType = UA_NODEIDTYPE_NUMERIC,
     .identifier.numeric = UA_NS0ID_AGGREGATES},
    {.namespaceIndex = 0,
     .identifierType = UA_NODEIDTYPE_NUMERIC,
     .identifier.numeric = UA_NS0ID_HASSUBTYPE},
    {.namespaceIndex = 0,
     .identifierType = UA_NODEIDTYPE_NUMERIC,
     .identifier.numeric = UA_NS0ID_HASCOMPONENT},
    {.namespaceIndex = 0,
     .identifierType = UA_NODEIDTYPE_NUMERIC,
     .identifier.numeric = UA_NS0ID_HASPROPERTY},
    {.namespaceIndex = 0,
     .identifierType = UA_NODEIDTYPE_NUMERIC,
     .identifier.numeric = UA_NS0ID_HASENCODING}};

static UA_NodeId
translateNodeId(const TNamespace *namespaces, UA_NodeId id) {
    if(id.namespaceIndex > 0) {
        id.namespaceIndex = namespaces[id.namespaceIndex].idx;
        return id;
    }
    return id;
}

static bool compareAlias(const void* element, const void* data)
{
    const Alias *a = (const Alias *)element;
    const char *s = (const char *)data;
    if(!strcmp(a->name, s))
    {
        return true;
    }
    return false;
}

static UA_NodeId
alias2Id(Nodeset *nodeset, const char *alias) {

    Alias* found = (Alias*)MemoryPool_find(nodeset->aliasPool, compareAlias, alias);
    if(found)
    {
        return found->id;
    }
    return UA_NODEID_NULL;
}

Nodeset *
Nodeset_new(UA_Server *server) {
    Nodeset *nodeset = (Nodeset *)UA_calloc(1, sizeof(Nodeset));
    if(!nodeset)
    {
        return NULL;
    }
    nodeset->aliasPool = MemoryPool_init(sizeof(Alias), 100);
    nodeset->countedChars = (char **)UA_calloc(MAX_REFCOUNTEDCHARS, sizeof(char *));
    if(!nodeset->countedChars)
    {
        return NULL;
    }
    nodeset->charsSize = 0;

    nodeset->hierachicalRefs = bidirectionalRefs;
    nodeset->hierachicalRefsSize = BIDIRECTIONAL_REF_COUNT;
    nodeset->refPool = MemoryPool_init(sizeof(TRef), 1000);
    nodeset->server = server;

    TNamespaceTable *table = (TNamespaceTable *)UA_calloc(1, sizeof(TNamespaceTable));
    if(!table) {
        return NULL;
    }

    table->cb = NULL;
    table->size = 1;
    table->ns = (TNamespace *)UA_calloc(1, (sizeof(TNamespace)));
    if(!table->ns) {
        return NULL;
    }
    table->ns[0].idx = 0;
    table->ns[0].uri = "http://opcfoundation.org/UA/";
    nodeset->namespaceTable = table;
    return nodeset;
}

void
Nodeset_setNewNamespaceCallback(Nodeset *nodeset, addNamespaceCb nsCallback) {
    nodeset->namespaceTable->cb = nsCallback;
}

void
Nodeset_cleanup(Nodeset *nodeset) {
    for(size_t cnt = 0; cnt < nodeset->charsSize; cnt++) {
        UA_free(nodeset->countedChars[cnt]);
    }
    UA_free(nodeset->countedChars);
    MemoryPool_cleanup(nodeset->aliasPool);
    UA_free(nodeset->namespaceTable->ns);
    UA_free(nodeset->namespaceTable);
    MemoryPool_cleanup(nodeset->refPool);
    UA_free(nodeset);
}

static char *
getAttributeValue(Nodeset *nodeset, const NodeAttribute *attr, const char **attributes,
                  int nb_attributes) {
    const int fields = 5;
    for(int i = 0; i < nb_attributes; i++) {
        const char *localname = attributes[i * fields + 0];
        if(strcmp((const char *)localname, attr->name))
            continue;
        const char *value_start = attributes[i * fields + 3];
        const char *value_end = attributes[i * fields + 4];
        size_t size = (size_t)(value_end - value_start);
        char *value = (char *)UA_malloc(sizeof(char) * size + 1);
        if(!value) {
            return NULL;
        }
        // todo: nodeset, refcount char
        nodeset->countedChars[nodeset->charsSize++] = value;
        memcpy(value, value_start, size);
        value[size] = '\0';
        return value;
    }
    if(attr->defaultValue != NULL || attr->optional) {
        return attr->defaultValue;
    }
    printf("attribute lookup error: %s\n", attr->name);
    return NULL;
}

static UA_QualifiedName
extractBrowseName(char *s) {
    char *idxSemi = strchr(s, ':');
    if(!idxSemi) {
        return UA_QUALIFIEDNAME_ALLOC((UA_UInt16)0, s);
    }
    return UA_QUALIFIEDNAME_ALLOC((UA_UInt16)atoi(s), idxSemi + 1);
}

static void
extractAttributes(Nodeset *nodeset, const TNamespace *namespaces, UA_Node *node,
                  int attributeSize, const char **attributes) {
    node->nodeId = translateNodeId(
        namespaces, extractNodeId(getAttributeValue(nodeset, &attrNodeId, attributes, attributeSize)));

    node->browseName = extractBrowseName(
        getAttributeValue(nodeset, &attrBrowseName, attributes, attributeSize));
    switch(node->nodeClass) {
        case UA_NODECLASS_OBJECTTYPE:
            ((UA_ObjectTypeNode *)node)->isAbstract =
                isTrue(getAttributeValue(nodeset, &attrIsAbstract, attributes, attributeSize));
            break;
        case UA_NODECLASS_OBJECT:
            ((UA_ObjectNode *)node)->eventNotifier = isTrue(getAttributeValue(
                nodeset, &attrEventNotifier, attributes, attributeSize));
            break;
        case UA_NODECLASS_VARIABLE: {

            char *datatype =
                getAttributeValue(nodeset, &attrDataType, attributes, attributeSize);
            UA_NodeId aliasId = alias2Id(nodeset, datatype);
            if(!UA_NodeId_equal(&aliasId, &UA_NODEID_NULL)) {
                ((UA_VariableNode *)node)->dataType = aliasId;
            } else {
                ((UA_VariableNode *)node)->dataType =
                    translateNodeId(namespaces, extractNodeId(datatype));
            }
            ((UA_VariableNode *)node)->valueRank = atoi(
                getAttributeValue(nodeset, &attrValueRank, attributes, attributeSize));
            ((UA_VariableNode *)node)->writeMask = 0;
            ((UA_VariableNode *)node)->arrayDimensionsSize = 0;
            ((UA_VariableNode *)node)->accessLevel = UA_ACCESSLEVELMASK_READ;

            //((UA_VariableNode *)node)->arrayDimensions =
            //    getAttributeValue(&attrArrayDimensions, attributes, attributeSize);

            break;
        }
        case UA_NODECLASS_VARIABLETYPE: {

            ((UA_VariableTypeNode *)node)->valueRank = atoi(
                getAttributeValue(nodeset, &attrValueRank, attributes, attributeSize));
            char *datatype =
                getAttributeValue(nodeset, &attrDataType, attributes, attributeSize);
            UA_NodeId aliasId = alias2Id(nodeset, datatype);
            if(!UA_NodeId_equal(&aliasId, &UA_NODEID_NULL)) {
                ((UA_VariableTypeNode *)node)->dataType = aliasId;
            } else {
                ((UA_VariableTypeNode *)node)->dataType =
                    translateNodeId(namespaces, extractNodeId(datatype));
            }
            //((UA_VariableTypeNode *)node)->arrayDimensions =
            //    getAttributeValue(&attrArrayDimensions, attributes, attributeSize);
            ((UA_VariableTypeNode *)node)->isAbstract = isTrue(
                getAttributeValue(nodeset, &attrIsAbstract, attributes, attributeSize));
            break;
        }
        case UA_NODECLASS_DATATYPE:
            ((UA_DataTypeNode *)node)->isAbstract = isTrue(
                getAttributeValue(nodeset, &attrIsAbstract, attributes, attributeSize));
            break;
        case UA_NODECLASS_METHOD:
            ((UA_MethodNode *)node)->executable = isTrue(
                getAttributeValue(nodeset, &attrExecutable, attributes, attributeSize));
            break;
        case UA_NODECLASS_REFERENCETYPE:
            break;
        case UA_NODECLASS_VIEW:
            break;
        default:;
    }
}

static void
initNode(Nodeset *nodeset, TNamespace *namespaces, UA_Node *node, int nb_attributes,
         const char **attributes) {
    extractAttributes(nodeset, namespaces, node, nb_attributes, attributes);
}

UA_Node *
Nodeset_newNode(Nodeset *nodeset, UA_NodeClass nodeClass, int nb_attributes,
                const char **attributes) {
    UA_ServerConfig *config = UA_Server_getConfig(nodeset->server);
    UA_Node *newNode = config->nodestore.newNode(config->nodestore.context, nodeClass);
    initNode(nodeset, nodeset->namespaceTable->ns, newNode, nb_attributes, attributes);
    return newNode;
}

UA_NodeReferenceKind *
Nodeset_newReference(Nodeset *nodeset, UA_Node *node, int attributeSize,
                     const char **attributes) {
    UA_Boolean isForward = false;
    if(!strcmp("true",
               getAttributeValue(nodeset, &attrIsForward, attributes, attributeSize))) {
        isForward = true;
    }

    char *s = getAttributeValue(nodeset, &attrReferenceType, attributes, attributeSize);
    UA_NodeId refTypeId = UA_NODEID_NULL;
    if(!isNodeId(s)) {
        // try it with alias
        refTypeId = translateNodeId(nodeset->namespaceTable->ns, alias2Id(nodeset, s));
    } else {
        refTypeId = translateNodeId(nodeset->namespaceTable->ns, extractNodeId(s));
    }

    UA_NodeReferenceKind *existingRefs = NULL;
    for(size_t i = 0; i < node->referencesSize; ++i) {
        UA_NodeReferenceKind *refs = &node->references[i];
        if(refs->isInverse != isForward &&
           UA_NodeId_equal(&refs->referenceTypeId, &refTypeId)) {
            existingRefs = refs;
            break;
        }
    }
    // if we have an existing referenceKind, use this one
    if(existingRefs != NULL) {
        return existingRefs;
    }

    node->referencesSize++;
    UA_NodeReferenceKind *refs = (UA_NodeReferenceKind *)realloc(
        node->references, sizeof(UA_NodeReferenceKind) * node->referencesSize);
    node->references = refs;
    UA_NodeReferenceKind *newRef = refs + node->referencesSize - 1;
    newRef->referenceTypeId = refTypeId;
    newRef->isInverse = !isForward;
    newRef->refTargetsSize = 0;
    newRef->refTargets = NULL;
    ZIP_INIT(&newRef->refTargetsTree);
    return newRef;
}

static bool
isBidirectionalReference(Nodeset *nodeset, const UA_NodeId *refId) {
    for(size_t i = 0; i < nodeset->hierachicalRefsSize; i++) {
        if(UA_NodeId_equal(&nodeset->hierachicalRefs[i], refId)) {
            return true;
        }
    }
    return false;
}

static void
addReference(void *ref, void *userData) {
    TRef *tref = (TRef *)ref;
    Nodeset *ns = (Nodeset *)userData;

    /*
        Inverse references are only inserted auomatically if its in the bidirectional
       array (hierachical and hasEncoding).
        If the non-hierachical inverse reference is needed, it must be manually added to
       the nodeset
    */
    if(isBidirectionalReference(ns, &tref->ref->referenceTypeId)) {
        for(size_t cnt = 0; cnt < tref->ref->refTargetsSize; cnt++) {
            UA_ExpandedNodeId eId;
            eId.namespaceUri = UA_STRING_NULL;
            eId.nodeId = *tref->src;
            eId.serverIndex = 0;
            UA_Server_addReference(ns->server, tref->ref->refTargets[cnt].target.nodeId,
                                   tref->ref->referenceTypeId, eId, tref->ref->isInverse);
        }
    }
}

static void
cleanupRefs(void *ref, void *userData) {
    TRef *tref = (TRef *)ref;
    UA_NodeId_delete(tref->src);
    UA_free(tref->ref->refTargets);
    UA_free(tref->ref);
}

void
Nodeset_linkReferences(Nodeset *nodeset) {

    /*
        iterate over all references, if it's an hierachical ref or hasEncoding ref, insert
       the inverse ref
        From UA Spec part 3, References:
        It might not always be possible for Servers to instantiate both forward and
        inverse References for non-symmetric ReferenceTypes as shown in Figure 9. When
       they
        do, the References are referred to as bidirectional. Although not required, it is
        recommended that all hierarchical References be instantiated as
        bidirectional to ensure browse connectivity. A bidirectional Reference is modelled
       as two
        separate References
    */

    MemoryPool_forEach(nodeset->refPool, addReference, nodeset);
    MemoryPool_forEach(nodeset->refPool, cleanupRefs, NULL);
}

Alias *
Nodeset_newAlias(Nodeset *nodeset, int attributeSize, const char **attributes) {
    Alias *alias = (Alias*)MemoryPool_getMemoryForElement(nodeset->aliasPool);    
    alias->name =
        getAttributeValue(nodeset, &attrAlias, attributes, attributeSize);
    return alias;
}

void
Nodeset_newAliasFinish(const Nodeset *nodeset, Alias* alias, char *idString) {
    alias->id =
        translateNodeId(nodeset->namespaceTable->ns, extractNodeId(idString));
}

TNamespace *
Nodeset_newNamespace(Nodeset *nodeset) {
    nodeset->namespaceTable->size++;
    TNamespace *ns =
        (TNamespace *)UA_realloc(nodeset->namespaceTable->ns,
                                 sizeof(TNamespace) * (nodeset->namespaceTable->size));
    nodeset->namespaceTable->ns = ns;
    ns[nodeset->namespaceTable->size - 1].uri = NULL;
    return &ns[nodeset->namespaceTable->size - 1];
}

void
Nodeset_newNamespaceFinish(Nodeset *nodeset, void *userContext, char *namespaceUri) {
    nodeset->namespaceTable->ns[nodeset->namespaceTable->size - 1].uri = namespaceUri;
    int globalIdx = nodeset->namespaceTable->cb(
        nodeset->server,
        nodeset->namespaceTable->ns[nodeset->namespaceTable->size - 1].uri);

    nodeset->namespaceTable->ns[nodeset->namespaceTable->size - 1].idx =
        (UA_UInt16)globalIdx;
}

void
Nodeset_setDisplayname(UA_Node *node, char *s) {
    // todo
    node->displayName = UA_LOCALIZEDTEXT_ALLOC("de", s);
}

static void
addToBidirectionalRefsArray(Nodeset *nodeset, const UA_Node *node) {
    for(size_t i = 0; i < node->referencesSize; i++) {
        if(node->references[i].isInverse) {
            nodeset->hierachicalRefs[nodeset->hierachicalRefsSize++] = node->nodeId;
            break;
        }
    }
}

void
Nodeset_newNodeFinish(Nodeset *nodeset, UA_Node *node) {
    if(node->nodeClass == UA_NODECLASS_REFERENCETYPE) {
        addToBidirectionalRefsArray(nodeset, node);
    }

    // store all references
    for(size_t cnt = 0; cnt < node->referencesSize; cnt++) {
        TRef *ref = (TRef *)MemoryPool_getMemoryForElement(nodeset->refPool);
        UA_NodeReferenceKind *copyRef =
            (UA_NodeReferenceKind *)UA_malloc(sizeof(UA_NodeReferenceKind));
        memcpy(copyRef, &node->references[cnt], sizeof(UA_NodeReferenceKind));
        copyRef->refTargets = (UA_ReferenceTarget *)UA_malloc(
            sizeof(UA_ReferenceTarget) * node->references[cnt].refTargetsSize);
        memcpy(copyRef->refTargets, node->references[cnt].refTargets,
               sizeof(UA_ReferenceTarget) * node->references[cnt].refTargetsSize);
        UA_NodeId_copy(&node->references[cnt].referenceTypeId, &copyRef->referenceTypeId);
        ref->ref = copyRef;
        ref->src = UA_NodeId_new();
        UA_NodeId_copy(&node->nodeId, ref->src);
    }
    UA_ServerConfig* config = UA_Server_getConfig(nodeset->server);
    config->nodestore.insertNode(config->nodestore.context, node, NULL);
}

//copied from ua_nodes
static UA_StatusCode
addReferenceTarget(UA_NodeReferenceKind *refs, const UA_ExpandedNodeId *target,
                   UA_UInt32 targetHash) {
    UA_ReferenceTarget *targets = (UA_ReferenceTarget *)UA_realloc(
        refs->refTargets, (refs->refTargetsSize + 1) * sizeof(UA_ReferenceTarget));
    if(!targets)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Repair the pointers in the tree for the realloced array */
    uintptr_t arraydiff = (uintptr_t)targets - (uintptr_t)refs->refTargets;
    if(arraydiff != 0) {
        for(size_t i = 0; i < refs->refTargetsSize; i++) {
            if(targets[i].zipfields.zip_left)
                *(uintptr_t *)&targets[i].zipfields.zip_left += arraydiff;
            if(targets[i].zipfields.zip_right)
                *(uintptr_t *)&targets[i].zipfields.zip_right += arraydiff;
        }
    }

    if(refs->refTargetsTree.zip_root)
        *(uintptr_t *)&refs->refTargetsTree.zip_root += arraydiff;
    refs->refTargets = targets;

    UA_ReferenceTarget *entry = &refs->refTargets[refs->refTargetsSize];
    UA_StatusCode retval = UA_ExpandedNodeId_copy(target, &entry->target);
    if(retval != UA_STATUSCODE_GOOD) {
        if(refs->refTargetsSize == 0) {
            /* We had zero references before (realloc was a malloc) */
            UA_free(refs->refTargets);
            refs->refTargets = NULL;
        }
        return retval;
    }

    entry->targetHash = targetHash;
    ZIP_INSERT(UA_ReferenceTargetHead, &refs->refTargetsTree, entry,
               ZIP_FFS32(UA_UInt32_random()));
    refs->refTargetsSize++;
    return UA_STATUSCODE_GOOD;
}

void
Nodeset_newReferenceFinish(Nodeset *nodeset, UA_NodeReferenceKind *ref, char *targetId) {

    UA_ExpandedNodeId eid;
    eid.nodeId = translateNodeId(nodeset->namespaceTable->ns, extractNodeId(targetId));
    eid.serverIndex = 0;
    eid.namespaceUri = UA_STRING_NULL;
    UA_UInt32 targetHash = UA_ExpandedNodeId_hash(&eid);
    addReferenceTarget(ref, &eid, targetHash);
}

void
Nodeset_addRefCountedChar(Nodeset *nodeset, char *newChar) {
    nodeset->countedChars[nodeset->charsSize++] = newChar;
}
