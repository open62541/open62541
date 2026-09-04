/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 *    Copyright 2025 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include "ua_nodeset_loader_internal.h"

#include "parse_num.h"

#include <stdint.h>
#include <string.h>

typedef struct Alias {
    char *name;
    UA_NodeId id;
    struct Alias *next;
} Alias;

static enum ZIP_CMP
compareNodeId(const void *a, const void *b) {
    return (enum ZIP_CMP)UA_NodeId_order((const UA_NodeId *)a, (const UA_NodeId *)b);
}

ZIP_FUNCTIONS(NL_NodeTree, NL_Node, treeEntry, UA_NodeId, id, compareNodeId)

static NL_Node *
nodeNew(UA_NodeClass nodeClass) {
    size_t size;
    switch(nodeClass) {
    case UA_NODECLASS_OBJECT:
        size = sizeof(NL_ObjectNode);
        break;
    case UA_NODECLASS_OBJECTTYPE:
        size = sizeof(NL_ObjectTypeNode);
        break;
    case UA_NODECLASS_VARIABLE:
        size = sizeof(NL_VariableNode);
        break;
    case UA_NODECLASS_DATATYPE:
        size = sizeof(NL_DataTypeNode);
        break;
    case UA_NODECLASS_METHOD:
        size = sizeof(NL_MethodNode);
        break;
    case UA_NODECLASS_REFERENCETYPE:
        size = sizeof(NL_ReferenceTypeNode);
        break;
    case UA_NODECLASS_VARIABLETYPE:
        size = sizeof(NL_VariableTypeNode);
        break;
    case UA_NODECLASS_VIEW:
        size = sizeof(NL_ViewNode);
        break;
    default:
        return NULL;
    }
    return (NL_Node *)UA_calloc(1, size);
}

static void
nodeDelete(NL_Node *node) {
    if(!node)
        return;
    UA_NodeId_clear(&node->id);
    UA_NodeId_clear(&node->parentId);
    UA_QualifiedName_clear(&node->browseName);

    NL_Reference *ref = node->refs;
    while(ref) {
        NL_Reference *next = ref->next;
        UA_NodeId_clear(&ref->target);
        UA_NodeId_clear(&ref->refType);
        UA_free(ref);
        ref = next;
    }

    if(node->nodeClass == UA_NODECLASS_VARIABLE) {
        NL_VariableNode *varNode = (NL_VariableNode *)node;
        UA_String_clear(&varNode->value);
        UA_NodeId_clear(&varNode->datatype);
    } else if(node->nodeClass == UA_NODECLASS_VARIABLETYPE) {
        NL_VariableTypeNode *varTypeNode = (NL_VariableTypeNode *)node;
        UA_NodeId_clear(&varTypeNode->datatype);
    } else if(node->nodeClass == UA_NODECLASS_DATATYPE) {
        NL_DataTypeNode *dtNode = (NL_DataTypeNode *)node;
        if(dtNode->definition) {
            for(size_t i = 0; i < dtNode->definition->fieldsSize; i++)
                UA_NodeId_clear(&dtNode->definition->fields[i].dataType);
            UA_free(dtNode->definition->fields);
            UA_free(dtNode->definition);
        }
    }
    UA_free(node);
}

static const UA_NodeId *
getAliasNodeId(const NodeSet *nodeset, const char *name) {
    if(!name)
        return NULL;
    for(const Alias *alias = nodeset->aliases; alias; alias = alias->next) {
        if(!strcmp(name, alias->name))
            return &alias->id;
    }
    return NULL;
}

static bool
parseNodeId(const NodeSet *nodeset, char *s, UA_NodeId *out) {
    if(!s)
        return false;
    return UA_NodeId_parseEx(out, UA_STRING(s), &nodeset->namespaceMapping) == UA_STATUSCODE_GOOD;
}

static bool
parseQualifiedName(const NodeSet *nodeset, char *s, UA_QualifiedName *out) {
    if(!s)
        return false;
    return UA_QualifiedName_parseEx(out, UA_STRING(s), &nodeset->namespaceMapping) ==
           UA_STATUSCODE_GOOD;
}

static const UA_NodeId *
builtinTypeId(const char *name) {
    static const struct {
        const char *name;
        size_t typeIndex;
    } builtinTypes[] = {{"Boolean", UA_TYPES_BOOLEAN},
                        {"SByte", UA_TYPES_SBYTE},
                        {"Byte", UA_TYPES_BYTE},
                        {"Int16", UA_TYPES_INT16},
                        {"UInt16", UA_TYPES_UINT16},
                        {"Int32", UA_TYPES_INT32},
                        {"UInt32", UA_TYPES_UINT32},
                        {"Int64", UA_TYPES_INT64},
                        {"UInt64", UA_TYPES_UINT64},
                        {"Float", UA_TYPES_FLOAT},
                        {"Double", UA_TYPES_DOUBLE},
                        {"String", UA_TYPES_STRING},
                        {"DateTime", UA_TYPES_DATETIME},
                        {"Guid", UA_TYPES_GUID},
                        {"ByteString", UA_TYPES_BYTESTRING},
                        {"XmlElement", UA_TYPES_XMLELEMENT},
                        {"NodeId", UA_TYPES_NODEID},
                        {"ExpandedNodeId", UA_TYPES_EXPANDEDNODEID},
                        {"StatusCode", UA_TYPES_STATUSCODE},
                        {"QualifiedName", UA_TYPES_QUALIFIEDNAME},
                        {"LocalizedText", UA_TYPES_LOCALIZEDTEXT},
                        {"ExtensionObject", UA_TYPES_EXTENSIONOBJECT},
                        {"DataValue", UA_TYPES_DATAVALUE},
                        {"Variant", UA_TYPES_VARIANT},
                        {"DiagnosticInfo", UA_TYPES_DIAGNOSTICINFO}};

    if(!name)
        return NULL;
    for(size_t i = 0; i < sizeof(builtinTypes) / sizeof(builtinTypes[0]); i++) {
        if(!strcmp(name, builtinTypes[i].name))
            return &UA_TYPES[builtinTypes[i].typeIndex].typeId;
    }
    return NULL;
}

static bool
alias2Id(const NodeSet *nodeset, char *name, UA_NodeId *out) {
    const UA_NodeId *alias = getAliasNodeId(nodeset, name);
    if(!alias)
        alias = builtinTypeId(name);
    if(alias)
        return UA_NodeId_copy(alias, out) == UA_STATUSCODE_GOOD;
    if(parseNodeId(nodeset, name, out))
        return true;

    /* Keep accepting symbolic identifiers that are not declared in Aliases,
     * as the previous loader did. They remain unresolved and are handled by
     * the server when the node is added. */
    UA_NodeId_clear(out);
    return true;
}

static UA_StatusCode
updateNamespaceUris(NodeSet *nodeset) {
    for(;;) {
        UA_String nsUri = UA_STRING_NULL;
        UA_StatusCode res = UA_Server_getNamespaceByIndex(
            nodeset->server, nodeset->namespaceMapping.namespaceUrisSize, &nsUri);
        if(res == UA_STATUSCODE_BADNOTFOUND)
            return UA_STATUSCODE_GOOD;
        if(res != UA_STATUSCODE_GOOD)
            return res;
        res = UA_Array_appendCopy((void **)&nodeset->namespaceMapping.namespaceUris,
                                  &nodeset->namespaceMapping.namespaceUrisSize, &nsUri,
                                  &UA_TYPES[UA_TYPES_STRING]);
        UA_String_clear(&nsUri);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }
}

UA_StatusCode
UA_NodeSet_addNamespace(NodeSet *nodeset, const UA_String nsUri) {
    size_t serverIdx = 0;
    UA_StatusCode res = UA_Server_getNamespaceByName(nodeset->server, nsUri, &serverIdx);
    if(res != UA_STATUSCODE_GOOD && res != UA_STATUSCODE_BADNOTFOUND)
        return res;

    if(res == UA_STATUSCODE_BADNOTFOUND) {
        if(nsUri.length == SIZE_MAX)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        char *name = (char *)UA_malloc(nsUri.length + 1);
        if(!name)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        memcpy(name, nsUri.data, nsUri.length);
        name[nsUri.length] = 0;
        (void)UA_Server_addNamespace(nodeset->server, name);
        UA_free(name);

        res = UA_Server_getNamespaceByName(nodeset->server, nsUri, &serverIdx);
    }
    if(res != UA_STATUSCODE_GOOD || serverIdx > UA_UINT16_MAX)
        return res != UA_STATUSCODE_GOOD ? res : UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    UA_UInt16 localIdx = (UA_UInt16)serverIdx;

    res = updateNamespaceUris(nodeset);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Namespace zero can also appear explicitly as the document's first URI. */
    if(localIdx == 0 && nodeset->namespaceMapping.remote2localSize > 0)
        return UA_STATUSCODE_GOOD;

    return UA_Array_appendCopy((void **)&nodeset->namespaceMapping.remote2local,
                               &nodeset->namespaceMapping.remote2localSize, &localIdx,
                               &UA_TYPES[UA_TYPES_UINT16]);
}

UA_StatusCode
UA_NodeSet_updateParentRefTypes(NodeSet *nodeset) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.referenceTypeId = UA_NS0ID(HASSUBTYPE);
    bd.nodeId = UA_NS0ID(HIERARCHICALREFERENCES);

    size_t parentRefTypesSize = 0;
    UA_ExpandedNodeId *parentRefTypes = NULL;
    UA_StatusCode res =
        UA_Server_browseRecursive(nodeset->server, &bd, &parentRefTypesSize, &parentRefTypes);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_ExpandedNodeId hierarchicalReferences =
        UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES);
    res = UA_Array_append((void **)&parentRefTypes, &parentRefTypesSize, &hierarchicalReferences,
                          &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
    if(res != UA_STATUSCODE_GOOD) {
        UA_Array_delete(parentRefTypes, parentRefTypesSize, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
        return res;
    }

    UA_Array_delete(nodeset->parentRefTypes, nodeset->parentRefTypesSize,
                    &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
    nodeset->parentRefTypes = parentRefTypes;
    nodeset->parentRefTypesSize = parentRefTypesSize;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_NodeSet_init(NodeSet *nodeset, UA_Server *server) {
    if(!nodeset || !server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    memset(nodeset, 0, sizeof(*nodeset));
    ZIP_INIT(&nodeset->nodeTree);
    nodeset->server = server;
    nodeset->logger = UA_Server_getConfig(server)->logging;
    UA_StatusCode res = updateNamespaceUris(nodeset);
    if(res == UA_STATUSCODE_GOOD) {
        UA_UInt16 ns0 = 0;
        res = UA_Array_appendCopy((void **)&nodeset->namespaceMapping.remote2local,
                                  &nodeset->namespaceMapping.remote2localSize, &ns0,
                                  &UA_TYPES[UA_TYPES_UINT16]);
    }
    if(res == UA_STATUSCODE_GOOD)
        res = UA_NodeSet_updateParentRefTypes(nodeset);
    if(res == UA_STATUSCODE_GOOD)
        return res;

    UA_NodeSet_clear(nodeset);
    return res;
}

NL_Node *
UA_NodeSet_findNode(NodeSet *nodeset, const UA_NodeId *key) {
    return ZIP_FIND(NL_NodeTree, &nodeset->nodeTree, key);
}

static bool
isHierarchicalReference(const NodeSet *nodeset, const UA_NodeId *refType) {
    for(size_t i = 0; i < nodeset->parentRefTypesSize; i++) {
        if(UA_NodeId_equal(refType, &nodeset->parentRefTypes[i].nodeId))
            return true;
    }
    return false;
}

static void
setInsertionParent(NL_Node *node, const UA_NodeId *parentId, NL_Reference *parentRef,
                   NL_Node *parent) {
    bool hasExplicitParent = !UA_NodeId_isNull(&node->parentId);
    bool matchesExplicitParent = hasExplicitParent && UA_NodeId_equal(parentId, &node->parentId);
    const UA_NodeId *currentParentId = NULL;
    if(node->insertionParent)
        currentParentId = &node->insertionParent->id;
    else if(node->insertionParentRef)
        currentParentId = &node->insertionParentRef->target;
    bool currentMatchesExplicitParent =
        hasExplicitParent && currentParentId && UA_NodeId_equal(currentParentId, &node->parentId);
    if(currentParentId && (!matchesExplicitParent || currentMatchesExplicitParent))
        return;

    node->insertionParentRef = parentRef;
    node->insertionParent = parent;
}

void
UA_NodeSet_resolveReferences(NodeSet *nodeset) {
    if(!nodeset)
        return;
    /* Resolve the complete reference graph before selecting insertion parents.
     * Parent references can be declared in either direction and ParentNodeId is
     * only optional design-tool metadata. */
    for(NL_Node *node = nodeset->nodes; node; node = node->next) {
        node->typeDefinitionRef = NULL;
        node->insertionParentRef = NULL;
        node->insertionParent = NULL;
    }

    static const UA_NodeId hasTypeDefinition = {
        0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_HASTYPEDEFINITION}};
    for(NL_Node *node = nodeset->nodes; node; node = node->next) {
        for(NL_Reference *ref = node->refs; ref; ref = ref->next) {
            ref->targetPtr = UA_NodeSet_findNode(nodeset, &ref->target);
            if(ref->isForward && UA_NodeId_equal(&ref->refType, &hasTypeDefinition))
                node->typeDefinitionRef = ref;
            if(!isHierarchicalReference(nodeset, &ref->refType))
                continue;
            if(ref->isForward) {
                if(ref->targetPtr)
                    setInsertionParent(ref->targetPtr, &node->id, ref, node);
            } else {
                setInsertionParent(node, &ref->target, ref, ref->targetPtr);
            }
        }
    }
}

void
UA_NodeSet_clear(NodeSet *nodeset) {
    if(!nodeset)
        return;
    Alias *alias = nodeset->aliases;
    while(alias) {
        Alias *next = alias->next;
        UA_NodeId_clear(&alias->id);
        UA_free(alias);
        alias = next;
    }
    NL_Node *node = nodeset->nodes;
    while(node) {
        NL_Node *next = node->next;
        nodeDelete(node);
        node = next;
    }
    UA_free(nodeset->text);
    UA_NamespaceMapping_clear(&nodeset->namespaceMapping);
    UA_Array_delete(nodeset->parentRefTypes, nodeset->parentRefTypesSize,
                    &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
    memset(nodeset, 0, sizeof(*nodeset));
}

static char *
getAttributeValue(const XmlAttributes *attributes, const char *attributeName) {
    for(size_t i = 0; i < attributes->size; i++) {
        const XmlToken *token = &attributes->tokens[i];
        if(strcmp(token->name, attributeName))
            continue;
        return token->content;
    }
    return NULL;
}

static UA_Boolean
getBooleanAttribute(const XmlAttributes *attributes, const char *name, UA_Boolean defaultValue) {
    const char *value = getAttributeValue(attributes, name);
    if(!value)
        return defaultValue;
    if(!strcmp(value, "true") || !strcmp(value, "1"))
        return true;
    if(!strcmp(value, "false") || !strcmp(value, "0"))
        return false;
    return defaultValue;
}

static UA_Int64
getIntegerAttribute(const XmlAttributes *attributes, const char *name, UA_Int64 defaultValue) {
    const char *value = getAttributeValue(attributes, name);
    if(!value)
        return defaultValue;

    UA_Int64 parsed = 0;
    size_t length = strlen(value);
    if(length == 0 || parseInt64(value, length, &parsed) != length)
        return defaultValue;
    return parsed;
}

static UA_Double
getDoubleAttribute(const XmlAttributes *attributes, const char *name, UA_Double defaultValue) {
    const char *value = getAttributeValue(attributes, name);
    if(!value)
        return defaultValue;

    UA_Double parsed = 0.0;
    size_t length = strlen(value);
    if(length == 0 || parseDouble(value, length, &parsed) != length)
        return defaultValue;
    return parsed;
}

static bool
extractAttributes(NodeSet *nodeset, NL_Node *node, const XmlAttributes *attributes) {
    if(!parseNodeId(nodeset, getAttributeValue(attributes, "NodeId"), &node->id) ||
       !parseQualifiedName(nodeset, getAttributeValue(attributes, "BrowseName"), &node->browseName))
        return false;
    char *parentNodeId = getAttributeValue(attributes, "ParentNodeId");
    if(parentNodeId && !alias2Id(nodeset, parentNodeId, &node->parentId))
        return false;
    switch(node->nodeClass) {
    case UA_NODECLASS_OBJECTTYPE:
        ((NL_ObjectTypeNode *)node)->isAbstract =
            getBooleanAttribute(attributes, "IsAbstract", false);
        break;

    case UA_NODECLASS_OBJECT:
        ((NL_ObjectNode *)node)->eventNotifier =
            (UA_Byte)getIntegerAttribute(attributes, "EventNotifier", 0);
        break;

    case UA_NODECLASS_VARIABLE: {
        NL_VariableNode *variable = (NL_VariableNode *)node;
        char *datatype = getAttributeValue(attributes, "DataType");
        if(datatype && !alias2Id(nodeset, datatype, &variable->datatype))
            return false;
        variable->valueRankDefined = getAttributeValue(attributes, "ValueRank") != NULL;
        variable->valueRank = (UA_Int32)getIntegerAttribute(attributes, "ValueRank", -1);
        variable->minimumSamplingInterval =
            getDoubleAttribute(attributes, "MinimumSamplingInterval", -1.0);
        variable->arrayDimensions = getAttributeValue(attributes, "ArrayDimensions");
        variable->accessLevel = (UA_Byte)getIntegerAttribute(attributes, "AccessLevel", 1);
        variable->userAccessLevel = (UA_Byte)getIntegerAttribute(attributes, "UserAccessLevel", 1);
        variable->historizing = getBooleanAttribute(attributes, "Historizing", false);
        break;
    }

    case UA_NODECLASS_VARIABLETYPE: {
        NL_VariableTypeNode *variableType = (NL_VariableTypeNode *)node;
        variableType->valueRankDefined = getAttributeValue(attributes, "ValueRank") != NULL;
        variableType->valueRank = (UA_Int32)getIntegerAttribute(attributes, "ValueRank", -1);
        char *datatype = getAttributeValue(attributes, "DataType");
        if(datatype && !alias2Id(nodeset, datatype, &variableType->datatype))
            return false;
        variableType->arrayDimensions = getAttributeValue(attributes, "ArrayDimensions");
        variableType->isAbstract = getBooleanAttribute(attributes, "IsAbstract", false);
        break;
    }

    case UA_NODECLASS_DATATYPE:
        ((NL_DataTypeNode *)node)->isAbstract =
            getBooleanAttribute(attributes, "IsAbstract", false);
        break;

    case UA_NODECLASS_METHOD:
        ((NL_MethodNode *)node)->executable = getBooleanAttribute(attributes, "Executable", true);
        ((NL_MethodNode *)node)->userExecutable =
            getBooleanAttribute(attributes, "UserExecutable", true);
        break;

    case UA_NODECLASS_REFERENCETYPE:
        ((NL_ReferenceTypeNode *)node)->symmetric =
            getBooleanAttribute(attributes, "Symmetric", false);
        break;

    case UA_NODECLASS_VIEW:
        ((NL_ViewNode *)node)->containsNoLoops =
            getBooleanAttribute(attributes, "ContainsNoLoops", false);
        ((NL_ViewNode *)node)->eventNotifier =
            (UA_Byte)getIntegerAttribute(attributes, "EventNotifier", 0);
        break;

    default:
        break;
    }
    return true;
}

NL_Node *
UA_NodeSet_newNode(NodeSet *nodeset, UA_NodeClass nodeClass, const XmlAttributes *attributes) {
    NL_Node *node = nodeNew(nodeClass);
    if(!node)
        return NULL;
    node->nodeClass = nodeClass;
    if(!extractAttributes(nodeset, node, attributes)) {
        nodeDelete(node);
        return NULL;
    }

    if(nodeset->nodesTail)
        nodeset->nodesTail->next = node;
    else
        nodeset->nodes = node;
    nodeset->nodesTail = node;
    ZIP_INSERT(NL_NodeTree, &nodeset->nodeTree, node);
    return node;
}

bool
UA_NodeSet_addReference(NodeSet *nodeset, NL_Node *node, const XmlAttributes *attributes,
                        char *idString) {
    NL_Reference *newRef = (NL_Reference *)UA_calloc(1, sizeof(NL_Reference));
    if(!newRef)
        return false;

    newRef->isForward = getBooleanAttribute(attributes, "IsForward", true);

    char *aliasIdString = getAttributeValue(attributes, "ReferenceType");
    if(!alias2Id(nodeset, aliasIdString, &newRef->refType) ||
       !alias2Id(nodeset, idString, &newRef->target)) {
        UA_NodeId_clear(&newRef->refType);
        UA_NodeId_clear(&newRef->target);
        UA_free(newRef);
        return false;
    }

    newRef->next = node->refs;
    node->refs = newRef;
    return true;
}

static NL_DataTypeDefinitionField *
dataTypeNodeAddDefinitionField(NL_DataTypeDefinition *def) {
    if(def->fieldsSize >= SIZE_MAX / sizeof(NL_DataTypeDefinitionField))
        return NULL;
    size_t newCount = def->fieldsSize + 1;
    NL_DataTypeDefinitionField *fields = (NL_DataTypeDefinitionField *)UA_realloc(
        def->fields, newCount * sizeof(NL_DataTypeDefinitionField));
    if(!fields)
        return NULL;
    def->fields = fields;
    def->fieldsSize = newCount;
    return &fields[newCount - 1];
}

bool
UA_NodeSet_addDataTypeDefinition(NL_Node *node, const XmlAttributes *attributes) {
    NL_DataTypeNode *dataTypeNode = (NL_DataTypeNode *)node;
    if(dataTypeNode->definition)
        return false;
    dataTypeNode->definition = (NL_DataTypeDefinition *)UA_calloc(1, sizeof(NL_DataTypeDefinition));
    if(!dataTypeNode->definition)
        return false;
    dataTypeNode->definition->isUnion = getBooleanAttribute(attributes, "IsUnion", false);
    dataTypeNode->definition->isOptionSet = getBooleanAttribute(attributes, "IsOptionSet", false);
    return true;
}

bool
UA_NodeSet_addDataTypeField(NodeSet *nodeset, NL_Node *node, const XmlAttributes *attributes) {
    NL_DataTypeNode *dataTypeNode = (NL_DataTypeNode *)node;
    if(!dataTypeNode->definition)
        return false;

    NL_DataTypeDefinitionField *newField = dataTypeNodeAddDefinitionField(dataTypeNode->definition);
    if(!newField)
        return false;
    memset(newField, 0, sizeof(NL_DataTypeDefinitionField));

    newField->name = getAttributeValue(attributes, "Name");
    if(!newField->name) {
        dataTypeNode->definition->fieldsSize--;
        return false;
    }

    char *value = getAttributeValue(attributes, "Value");
    if(value) {
        newField->value = getIntegerAttribute(attributes, "Value", 0);
        dataTypeNode->definition->isEnum = !dataTypeNode->definition->isOptionSet;
    } else {
        char *dataType = getAttributeValue(attributes, "DataType");
        if(!dataType)
            dataType = "i=24";
        if(!alias2Id(nodeset, dataType, &newField->dataType)) {
            dataTypeNode->definition->fieldsSize--;
            return false;
        }
        newField->valueRank = (UA_Int32)getIntegerAttribute(attributes, "ValueRank", -1);
        newField->isOptional = getBooleanAttribute(attributes, "IsOptional", false);
    }
    return true;
}

bool
UA_NodeSet_addAlias(NodeSet *nodeset, const XmlAttributes *attributes, char *idString) {
    char *name = getAttributeValue(attributes, "Alias");
    if(!name)
        return false;

    Alias *alias = (Alias *)UA_calloc(1, sizeof(Alias));
    if(!alias)
        return false;
    if(!parseNodeId(nodeset, idString, &alias->id)) {
        UA_NodeId_clear(&alias->id);
        UA_free(alias);
        return false;
    }

    alias->name = name;
    alias->next = nodeset->aliases;
    nodeset->aliases = alias;
    return true;
}

void
UA_NodeSet_setLocalizedText(UA_LocalizedText *target, const XmlAttributes *attributes, char *text) {
    target->locale = UA_STRING(getAttributeValue(attributes, "Locale"));
    target->text = UA_STRING(text);
}
