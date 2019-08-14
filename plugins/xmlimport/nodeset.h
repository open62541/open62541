/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#ifndef NODESET_H
#define NODESET_H
#include <open62541/plugin/nodesetLoader.h>
#include <open62541/plugin/nodestore.h>

#include <stdbool.h>
#include <stddef.h>

/*
typedef enum {
    NODECLASS_OBJECT = 0,
    NODECLASS_OBJECTTYPE = 1,
    NODECLASS_VARIABLE = 2,
    NODECLASS_DATATYPE = 3,
    NODECLASS_METHOD = 4,
    NODECLASS_REFERENCETYPE = 5,
    NODECLASS_VARIABLETYPE = 6
} TNodeClass;
*/

struct Nodeset;
typedef struct Nodeset Nodeset;

struct TNamespace;
typedef struct TNamespace TNamespace;

struct Alias;
typedef struct Alias Alias;

Nodeset *
Nodeset_new(UA_Server *server);
void
Nodeset_setNewNamespaceCallback(Nodeset *nodeset, addNamespaceCb nsCallback);
void
Nodeset_cleanup(Nodeset *nodeset);
UA_Node *
Nodeset_newNode(Nodeset *nodeset, UA_NodeClass nodeClass, int attributeSize,
                const char **attributes);
void
Nodeset_newNodeFinish(Nodeset *nodeset, UA_Node *node);
UA_NodeReferenceKind *
Nodeset_newReference(Nodeset *nodeset, UA_Node *node, int attributeSize,
                     const char **attributes);
void
Nodeset_newReferenceFinish(Nodeset *nodeset, UA_NodeReferenceKind *ref, char *target);
Alias *
Nodeset_newAlias(Nodeset *nodeset, int attributeSize, const char **attribute);
void
Nodeset_newAliasFinish(const Nodeset *nodeset, Alias* alias, char *idString);
TNamespace *
Nodeset_newNamespace(Nodeset *nodeset);
void
Nodeset_newNamespaceFinish(Nodeset *nodeset, void *userContext, char *namespaceUri);
void
Nodeset_addRefCountedChar(Nodeset *nodeset, char *newChar);
void
Nodeset_linkReferences(Nodeset *nodeset);
void Nodeset_setDisplayname(UA_Node *node, char *s);
#endif
