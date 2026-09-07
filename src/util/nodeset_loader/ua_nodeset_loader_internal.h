/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#ifndef UA_NODESET_LOADER_INTERNAL_H_
#define UA_NODESET_LOADER_INTERNAL_H_

#include "ziptree.h"

#include <open62541/server.h>

#include <stdbool.h>
#include <stddef.h>

typedef struct NL_Node NL_Node;

typedef enum {
    NL_NODE_NOT_ADDED,
    NL_NODE_BEGUN,
    NL_NODE_FINISHED,
    NL_NODE_REJECTED
} NL_NodeAddState;

typedef enum {
    NL_DATATYPE_NOT_REGISTERED,
    NL_DATATYPE_REGISTERED,
    NL_DATATYPE_REGISTRATION_REJECTED
} NL_DataTypeRegistrationState;

typedef struct NL_Reference {
    bool isForward;
    bool addedWithNode;
    UA_NodeId refType;
    UA_NodeId target;
    NL_Node *targetPtr;
    struct NL_Reference *next;
} NL_Reference;

#define NL_NODE_ATTRIBUTES                                                                         \
    UA_NodeClass nodeClass;                                                                        \
    UA_NodeId id;                                                                                  \
    UA_NodeId parentId;                                                                            \
    UA_QualifiedName browseName;                                                                   \
    UA_LocalizedText displayName;                                                                  \
    UA_LocalizedText description;                                                                  \
    NL_Reference *refs;                                                                            \
    NL_Reference *typeDefinitionRef;                                                               \
    NL_Reference *insertionParentRef;                                                              \
    NL_Node *insertionParent;                                                                      \
    NL_NodeAddState addState;                                                                      \
    struct {                                                                                       \
        NL_Node *left;                                                                             \
        NL_Node *right;                                                                            \
    } treeEntry;                                                                                   \
    NL_Node *next;

struct NL_Node {
    NL_NODE_ATTRIBUTES
};

typedef struct {
    NL_NODE_ATTRIBUTES UA_Byte eventNotifier;
} NL_ObjectNode;
typedef struct {
    NL_NODE_ATTRIBUTES UA_Boolean isAbstract;
} NL_ObjectTypeNode;
typedef struct {
    NL_NODE_ATTRIBUTES
    UA_Boolean isAbstract;
    UA_NodeId datatype;
    char *arrayDimensions;
    UA_Int32 valueRank;
    UA_Boolean valueRankDefined;
} NL_VariableTypeNode;
typedef struct {
    NL_NODE_ATTRIBUTES
    UA_NodeId datatype;
    char *arrayDimensions;
    UA_Int32 valueRank;
    UA_Boolean valueRankDefined;
    UA_Byte accessLevel;
    UA_Byte userAccessLevel;
    UA_Boolean historizing;
    UA_Double minimumSamplingInterval;
    UA_String value;
} NL_VariableNode;
typedef struct {
    char *name;
    UA_NodeId dataType;
    UA_Int32 valueRank;
    UA_Int64 value;
    UA_Boolean isOptional;
    UA_Boolean allowSubTypes;
} NL_DataTypeDefinitionField;
typedef struct {
    NL_DataTypeDefinitionField *fields;
    size_t fieldsSize;
    bool isEnum;
    bool isUnion;
    bool isOptionSet;
} NL_DataTypeDefinition;
typedef struct {
    NL_NODE_ATTRIBUTES
    NL_DataTypeDefinition *definition;
    UA_Boolean isAbstract;
    NL_DataTypeRegistrationState registrationState;
} NL_DataTypeNode;
typedef struct {
    NL_NODE_ATTRIBUTES
    UA_Boolean executable;
    UA_Boolean userExecutable;
} NL_MethodNode;
typedef struct {
    NL_NODE_ATTRIBUTES
    UA_LocalizedText inverseName;
    UA_Boolean symmetric;
} NL_ReferenceTypeNode;
typedef struct {
    NL_NODE_ATTRIBUTES
    UA_Boolean containsNoLoops;
    UA_Byte eventNotifier;
} NL_ViewNode;

typedef struct {
    char *name;
    char *content;
    size_t attributes;
    size_t subtreeEnd;
    size_t start;
    size_t end;
} XmlToken;

typedef struct {
    const XmlToken *tokens;
    size_t size;
} XmlAttributes;

typedef ZIP_HEAD(NL_NodeTree, NL_Node) NL_NodeTree;

typedef struct {
    char *text;
    struct Alias *aliases;

    NL_NodeTree nodeTree;
    NL_Node *nodes;
    NL_Node *nodesTail;

    UA_Server *server;
    UA_NamespaceMapping namespaceMapping;
    UA_Logger *logger;
    size_t parentRefTypesSize;
    UA_ExpandedNodeId *parentRefTypes;
} NodeSet;

UA_StatusCode UA_NodeSet_init(NodeSet *nodeset, UA_Server *server);
void UA_NodeSet_clear(NodeSet *nodeset);
void UA_NodeSet_resolveReferences(NodeSet *nodeset);
UA_StatusCode UA_NodeSet_updateParentRefTypes(NodeSet *nodeset);
NL_Node *UA_NodeSet_findNode(NodeSet *nodeset, const UA_NodeId *nodeId);
UA_StatusCode UA_NodeSet_import(NodeSet *nodeset, const UA_XmlElement *xml);
UA_StatusCode UA_NodeSet_apply(NodeSet *nodeset);
NL_Node *UA_NodeSet_newNode(NodeSet *nodeset, UA_NodeClass nodeClass,
                            const XmlAttributes *attributes);
bool UA_NodeSet_addReference(NodeSet *nodeset, NL_Node *node, const XmlAttributes *attributes,
                             char *idString);
bool UA_NodeSet_addAlias(NodeSet *nodeset, const XmlAttributes *attributes, char *idString);
UA_StatusCode UA_NodeSet_addNamespace(NodeSet *nodeset, const UA_String namespaceUri);
bool UA_NodeSet_addDataTypeDefinition(NL_Node *node, const XmlAttributes *attributes);
bool UA_NodeSet_addDataTypeField(NodeSet *nodeset, NL_Node *node, const XmlAttributes *attributes);
void UA_NodeSet_setLocalizedText(UA_LocalizedText *target, const XmlAttributes *attributes,
                                 char *text);

#endif /* UA_NODESET_LOADER_INTERNAL_H_ */
