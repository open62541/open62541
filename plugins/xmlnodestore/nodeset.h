/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#ifndef NODESET_H
#define NODESET_H
#include "nodesetLoader.h"
#include <stdbool.h>
#include <stddef.h>

#define MAX_OBJECTTYPES 1000
#define MAX_OBJECTS 100000
#define MAX_METHODS 1000
#define MAX_DATATYPES 1000
#define MAX_VARIABLES 1000000
#define MAX_REFERENCETYPES 1000
#define MAX_VARIABLETYPES 1000
#define MAX_HIERACHICAL_REFS 50
#define MAX_ALIAS 100
#define MAX_REFCOUNTEDCHARS 10000000
#define MAX_REFCOUNTEDREFS 1000000

#define OBJECT "UAObject"
#define METHOD "UAMethod"
#define OBJECTTYPE "UAObjectType"
#define VARIABLE "UAVariable"
#define VARIABLETYPE "UAVariableType"
#define DATATYPE "UADataType"
#define REFERENCETYPE "UAReferenceType"
#define DISPLAYNAME "DisplayName"
#define REFERENCES "References"
#define REFERENCE "Reference"
#define DESCRIPTION "Description"
#define ALIAS "Alias"
#define NAMESPACEURIS "NamespaceUris"
#define NAMESPACEURI "Uri"

typedef struct {
    const char *name;
    char *defaultValue;
    bool optional;
} NodeAttribute;

extern NodeAttribute attrNodeId;
extern NodeAttribute attrBrowseName;
extern NodeAttribute attrParentNodeId;
extern NodeAttribute attrEventNotifier;
extern NodeAttribute attrDataType;
extern NodeAttribute attrValueRank;
extern NodeAttribute attrArrayDimensions;
extern NodeAttribute attrIsAbstract;
extern NodeAttribute attrIsForward;
extern NodeAttribute attrReferenceType;
extern NodeAttribute attrAlias;

struct Nodeset;
typedef struct Nodeset Nodeset;

typedef struct {
    char *name;
    UA_NodeId id;
} Alias;

typedef struct {
    size_t cnt;
    UA_Node *nodes;
} NodeContainer;

struct TParserCtx;
typedef struct TParserCtx TParserCtx;

struct TNamespace;
typedef struct TNamespace TNamespace;

struct TNamespace {
    size_t idx;
    char *name;
};

typedef struct {
    size_t size;
    TNamespace *ns;
    addNamespaceCb cb;
} TNamespaceTable;

struct Nodeset {
    const char **countedChars;
    Alias **aliasArray;
    NodeContainer *nodes[NODECLASS_COUNT];
    size_t aliasSize;
    size_t charsSize;
    size_t refsSize;
    TNamespaceTable *namespaceTable;
    size_t hierachicalRefsSize;
    const char **hierachicalRefs;
};

UA_NodeId extractNodedId(const TNamespace *namespaces, char *s);
UA_NodeId translateNodeId(const TNamespace *namespaces, UA_NodeId id);
UA_NodeId alias2Id(const char *alias);
void Nodeset_new(addNamespaceCb nsCallback);
void Nodeset_cleanup(void);
UA_Node *Nodeset_newNode(TNodeClass nodeClass, int attributeSize, const char **attributes);
void Nodeset_newNodeFinish(UA_Node *node);
void Nodeset_newReference(UA_Node *node, int attributeSize, const char **attributes);
void Nodeset_newReferenceFinish(UA_Node *node, char* target);
Alias *Nodeset_newAlias(int attributeSize, const char **attribute);
void Nodeset_newAliasFinish(char* idString);
TNamespace *Nodeset_newNamespace(void);
void Nodeset_newNamespaceFinish(void* userContext, char* namespaceUri);
void Nodeset_addRefCountedChar(char *newChar);

#endif
