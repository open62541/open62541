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
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include "ua_server_internal.h"

/**************************/
/* Discovery URL Handling */
/**************************/

UA_Boolean
addServerDiscoveryUrl(UA_Server *server, const UA_String *url) {
    UA_ApplicationDescription *ad = &server->config.applicationDescription;
    for(size_t i = 0; i < ad->discoveryUrlsSize; i++) {
        if(UA_String_equal(url, &ad->discoveryUrls[i]))
            return false;
    }

    UA_StatusCode res =
        UA_Array_appendCopy((void **)&ad->discoveryUrls, &ad->discoveryUrlsSize,
                            url, &UA_TYPES[UA_TYPES_STRING]);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "Could not register DiscoveryUrl -- out of memory");
        return false;
    }
    UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_SERVER,
                "New DiscoveryUrl added: %S", *url);
    return true;
}

void
removeServerDiscoveryUrl(UA_Server *server, const UA_String *url) {
    UA_ApplicationDescription *ad = &server->config.applicationDescription;
    for(size_t i = 0; i < ad->discoveryUrlsSize; i++) {
        if(!UA_String_equal(url, &ad->discoveryUrls[i]))
            continue;
        UA_String_clear(&ad->discoveryUrls[i]);
        ad->discoveryUrlsSize--;
        if(i < ad->discoveryUrlsSize)
            memmove(&ad->discoveryUrls[i], &ad->discoveryUrls[i + 1],
                    (ad->discoveryUrlsSize - i) * sizeof(UA_String));
        if(ad->discoveryUrlsSize == 0) {
            UA_free(ad->discoveryUrls);
            ad->discoveryUrls = NULL;
        }
        return;
    }
}

UA_Boolean
getHttpUrlSecurity(const UA_String *url, UA_Boolean *secure) {
    static const UA_String schemes[2] = {
        UA_STRING_STATIC("opc.http://"), UA_STRING_STATIC("opc.https://")};
    for(size_t i = 0; i < 2; i++) {
        if(url->length < schemes[i].length ||
           memcmp(url->data, schemes[i].data, schemes[i].length) != 0)
            continue;
        if(secure)
            *secure = (i == 1);
        return true;
    }
    return false;
}

UA_ConnectionManager *
findConnectionManager(UA_EventLoop *eventLoop, const UA_String *protocol) {
    if(!eventLoop)
        return NULL;
    for(UA_EventSource *es = eventLoop->eventSources; es; es = es->next) {
        if(es->eventSourceType != UA_EVENTSOURCETYPE_CONNECTIONMANAGER)
            continue;
        UA_ConnectionManager *cm = (UA_ConnectionManager *)es;
        if(UA_String_equal(&cm->protocol, protocol))
            return cm;
    }
    return NULL;
}

/****************************/
/* Custom DataType Handling */
/****************************/

const UA_DataTypeArray *
serverCustomTypes(UA_Server *server) {
    if(server->customTypes_internalSize == 0)
        return server->config.customDataTypes;
    server->customTypes_internal[server->customTypes_internalSize-1].next = server->config.customDataTypes;
    return server->customTypes_internal;
}

const UA_DataTypeArray *
UA_Server_getDataTypes(UA_Server *server) {
    lockServer(server);
    const UA_DataTypeArray * out = serverCustomTypes(server);
    unlockServer(server);
    return out;
}

const UA_DataType *
UA_Server_findDataType(UA_Server *server, const UA_NodeId *typeId) {
    return UA_findDataTypeWithCustom(typeId, serverCustomTypes(server));
}

/* DataTypes need a stable pointer. So we allocate an array of 64 datatypes.
 * When that is full we move the server->customTypes_internal into a
 * heap-structure that gets cleaned up with the server lifecycle. */
static UA_StatusCode
addDataType(UA_Server *server, UA_DataType *dt) {
    /* Allocate the space for the new DataType */
#define TYPES_LIST_SIZE 64
    UA_DataTypeArray *current = NULL;
    if(server->customTypes_internalSize > 0)
        current = &server->customTypes_internal[server->customTypes_internalSize-1];
    if(!current || current->typesSize == TYPES_LIST_SIZE) {
        /* Increase the list-of-lists size */
        UA_DataTypeArray *lol = (UA_DataTypeArray*)
            UA_realloc(server->customTypes_internal,
                       sizeof(UA_DataTypeArray) * (server->customTypes_internalSize+1));
        if(!lol)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        memset(&lol[server->customTypes_internalSize], 0, sizeof(UA_DataTypeArray));
        server->customTypes_internal = lol;
        server->customTypes_internalSize++;

        /* Update the next-pointers for the internal DataTypeArray */
        for(size_t i = 0; i < server->customTypes_internalSize-1; i++)
            lol[i].next = &lol[i+1];

        /* Add a new types list. With the space for the datatypes already appended */
        current = &server->customTypes_internal[server->customTypes_internalSize-1];
        current->types = (UA_DataType*)UA_calloc(TYPES_LIST_SIZE, sizeof(UA_DataType));
        current->typesSize = 0;
        if(!current->types) {
            server->customTypes_internalSize--;
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
    }

    /* Move the datatype into the stable location in the server. Repair
     * self-referential members because their source pointer is about to go
     * out of scope. */
    UA_DataType *target = (UA_DataType*)(uintptr_t)&current->types[current->typesSize];
    *target = *dt;
    UA_DataTypeMember *members = (UA_DataTypeMember*)(uintptr_t)target->members;
    for(size_t i = 0; i < target->membersSize; i++) {
        if(members[i].memberType == dt)
            members[i].memberType = target;
    }
    current->typesSize++;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_addDataType(UA_Server *server, const UA_NodeId parentNodeId,
                      const UA_DataType *type) {
    /* Check that the type does not already exist. We do not allow changes to
     * DataTypes once they are set. */
    if(UA_Server_findDataType(server, &type->typeId))
        return UA_STATUSCODE_BADNODEIDEXISTS;

    /* Make a copy of the UA_DataType */
    UA_DataType dt2;
    UA_StatusCode res = UA_DataType_copy(type, &dt2);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Add the UA_DataType to the server */
    res = addDataType(server, &dt2);
    if(res != UA_STATUSCODE_GOOD)
        UA_DataType_clear(&dt2);
    return res;
}

/* Keep exact lookups distinct from resolving a simple subtype's storage.
 * In particular, an unregistered structure must never inherit its parent's
 * layout implicitly: it may introduce additional fields or encodings. */
const UA_DataType *
resolveDataTypeRepresentation(UA_Server *server, const UA_NodeId *typeId) {
    lockServer(server);
    const UA_DataType *type = UA_Server_findDataType(server, typeId);
    if(type) {
        unlockServer(server);
        return type;
    }

    RefTree visited;
    if(RefTree_init(&visited) != UA_STATUSCODE_GOOD) {
        unlockServer(server);
        return NULL;
    }

    const UA_Node *node = UA_NODESTORE_GET_SELECTIVE(
        server, typeId, UA_NODEATTRIBUTESMASK_NONE,
        UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASSUBTYPE),
        UA_BROWSEDIRECTION_INVERSE);
    while(node) {
        UA_Boolean duplicate = false;
        UA_StatusCode res = RefTree_addNodeId(&visited, &node->head.nodeId, &duplicate);
        if(res != UA_STATUSCODE_GOOD || duplicate ||
           node->head.nodeClass != UA_NODECLASS_DATATYPE) {
            UA_NODESTORE_RELEASE(server, node);
            break;
        }

        const UA_DataType *candidate =
            UA_Server_findDataType(server, &node->head.nodeId);
        if(candidate) {
            if(candidate->typeKind <= UA_DATATYPEKIND_DIAGNOSTICINFO &&
               candidate->typeKind != UA_DATATYPEKIND_EXTENSIONOBJECT)
                type = candidate;
            UA_NODESTORE_RELEASE(server, node);
            break;
        }
        const UA_Node *parent =
            getNodeType(server, &node->head, UA_NODEATTRIBUTESMASK_NODEID,
                        UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASSUBTYPE),
                        UA_BROWSEDIRECTION_INVERSE);
        UA_NODESTORE_RELEASE(server, node);
        node = parent;
    }
    RefTree_clear(&visited);
    unlockServer(server);
    return type;
}

#ifdef UA_TYPES_STRUCTUREDESCRIPTION
/* Replace an alias NodeId with the concrete datatype that defines its layout. */
static UA_StatusCode
normalizeDataTypeId(UA_Server *server, UA_NodeId *typeId) {
    if(UA_NodeId_isNull(typeId))
        return UA_STATUSCODE_GOOD;
    if(UA_Server_findDataType(server, typeId))
        return UA_STATUSCODE_GOOD;
    const UA_DataType *representation =
        resolveDataTypeRepresentation(server, typeId);
    if(!representation)
        return UA_STATUSCODE_BADNOTFOUND;
    UA_NodeId_clear(typeId);
    return UA_NodeId_copy(&representation->typeId, typeId);
}

#endif

UA_StatusCode
UA_Server_addDataTypeFromDescription(UA_Server *server,
                                     const UA_ExtensionObject *description) {
#ifdef UA_TYPES_STRUCTUREDESCRIPTION
    /* Normalize simple aliases in a temporary copy for memory-layout
     * construction. */
    UA_ExtensionObject layoutDescription;
    UA_StatusCode res = UA_ExtensionObject_copy(description, &layoutDescription);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(UA_ExtensionObject_hasDecodedType(
           &layoutDescription, &UA_TYPES[UA_TYPES_STRUCTUREDESCRIPTION])) {
        UA_StructureDescription *structureDescription =
            (UA_StructureDescription*)layoutDescription.content.decoded.data;
        UA_StructureDefinition *definition =
            &structureDescription->structureDefinition;
        for(size_t i = 0; res == UA_STATUSCODE_GOOD &&
                        i < definition->fieldsSize; i++) {
            if(UA_NodeId_equal(&definition->fields[i].dataType,
                               &structureDescription->dataTypeId))
                continue;
            res = normalizeDataTypeId(server, &definition->fields[i].dataType);
        }
    }
    if(res != UA_STATUSCODE_GOOD) {
        UA_ExtensionObject_clear(&layoutDescription);
        return res;
    }

    /* Translate into a new UA_DataType */
    UA_DataType dt;
    res = UA_DataType_fromDescription(&dt, &layoutDescription,
                                      serverCustomTypes(server));
    UA_ExtensionObject_clear(&layoutDescription);
    if(res != UA_STATUSCODE_GOOD)
        return res;

#else
    UA_DataType dt;
    UA_StatusCode res = UA_DataType_fromDescription(&dt, description,
                                                    serverCustomTypes(server));
    if(res != UA_STATUSCODE_GOOD)
        return res;

#endif

    /* Check that the type does not already exist. We do not allow changes to
     * DataTypes once they are set. */
    if(UA_Server_findDataType(server, &dt.typeId)) {
        UA_DataType_clear(&dt);
        return UA_STATUSCODE_BADNODEIDEXISTS;
    }

    /* Add the UA_DataType to the server */
    res = addDataType(server, &dt);
    if(res != UA_STATUSCODE_GOOD)
        UA_DataType_clear(&dt);
    return res;
}

/* Generated arrays use immutable identities, independently of in-place loads. */
static UA_StatusCode
generatedNodeId(UA_Server *server, const UA_DataTypeArraySource *source,
                const UA_NodeId *original, UA_NodeId *mapped) {
    if(UA_NodeId_isNull(original))
        return UA_NodeId_copy(original, mapped);
    if(original->namespaceIndex >= source->namespaceUrisSize)
        return UA_STATUSCODE_BADNODEIDINVALID;
    const UA_String *uri = &source->namespaceUris[original->namespaceIndex];
    for(size_t i = 0; i < server->namespacesSize; i++) {
        if(!UA_String_equal(uri, &server->namespaces[i]))
            continue;
        UA_StatusCode res = UA_NodeId_copy(original, mapped);
        if(res == UA_STATUSCODE_GOOD)
            mapped->namespaceIndex = (UA_UInt16)i;
        return res;
    }
    return UA_STATUSCODE_BADNOTFOUND;
}

static UA_Boolean
generatedTypeIndex(const UA_DataTypeArraySource *source, const UA_DataType *type,
                   size_t *index) {
    uintptr_t ptr = (uintptr_t)type;
    uintptr_t bases[2] = {(uintptr_t)source->staticTypes,
                          (uintptr_t)source->originTypes};
    for(size_t j = 0; j < 2; j++) {
        if(ptr < bases[j])
            continue;
        uintptr_t offset = ptr - bases[j];
        if(offset % sizeof(UA_DataType) == 0 &&
           offset / sizeof(UA_DataType) < source->typesSize) {
            *index = (size_t)(offset / sizeof(UA_DataType));
            return true;
        }
    }
    return false;
}

static const UA_DataTypeArraySource *
generatedTypeOrigin(const UA_DataTypeArraySource *source, size_t dependenciesSize,
                    UA_DataTypeArraySource *const *dependencies,
                    const UA_DataType *type, size_t *index) {
    if(generatedTypeIndex(source, type, index))
        return source;
    for(size_t i = 0; i < dependenciesSize; i++) {
        if(generatedTypeIndex(dependencies[i], type, index))
            return dependencies[i];
    }
    return NULL;
}

static UA_Boolean
generatedStructure(const UA_DataType *type) {
    return type->typeKind == UA_DATATYPEKIND_STRUCTURE ||
        type->typeKind == UA_DATATYPEKIND_OPTSTRUCT ||
        type->typeKind == UA_DATATYPEKIND_UNION;
}

/* Check the dependency's ABI and field identities, including dependencies loaded
 * from XML at runtime. Matching NodeIds alone do not establish compatibility. */
static UA_StatusCode
checkGeneratedDependency(UA_Server *server, const UA_DataTypeArraySource *source,
                         size_t dependenciesSize,
                         UA_DataTypeArraySource *const *dependencies,
                         const UA_DataTypeArraySource *owner,
                         const UA_DataType *expected, const UA_DataType *actual) {
    if(expected->typeKind != actual->typeKind || expected->memSize != actual->memSize ||
       expected->membersSize != actual->membersSize)
        return UA_STATUSCODE_BADTYPEMISMATCH;
    UA_NodeId binary = UA_NODEID_NULL;
    UA_NodeId xml = UA_NODEID_NULL;
    UA_StatusCode res = generatedNodeId(server, owner, &expected->binaryEncodingId, &binary);
    if(res == UA_STATUSCODE_GOOD)
        res = generatedNodeId(server, owner, &expected->xmlEncodingId, &xml);
    if(res == UA_STATUSCODE_GOOD &&
       (!UA_NodeId_equal(&binary, &actual->binaryEncodingId) ||
        !UA_NodeId_equal(&xml, &actual->xmlEncodingId)))
        res = UA_STATUSCODE_BADTYPEMISMATCH;
    UA_NodeId_clear(&binary);
    UA_NodeId_clear(&xml);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    for(size_t j = 0; j < expected->membersSize; j++) {
        const UA_DataTypeMember *em = &expected->members[j];
        const UA_DataTypeMember *am = &actual->members[j];
        if(expected->typeKind == UA_DATATYPEKIND_ENUM) {
            if(em->memberType != am->memberType)
                return UA_STATUSCODE_BADTYPEMISMATCH;
            continue;
        }
        if(!generatedStructure(expected))
            continue;
        if(em->padding != am->padding || em->isArray != am->isArray ||
           em->isOptional != am->isOptional)
            return UA_STATUSCODE_BADTYPEMISMATCH;
        size_t index;
        const UA_DataTypeArraySource *memberOwner = generatedTypeOrigin(
            source, dependenciesSize, dependencies, em->memberType, &index);
        UA_NodeId id = UA_NODEID_NULL;
        if(memberOwner) {
            res = generatedNodeId(server, memberOwner,
                                   &memberOwner->originTypes[index].typeId, &id);
        } else {
            uintptr_t offset = (uintptr_t)em->memberType - (uintptr_t)UA_TYPES;
            if((uintptr_t)em->memberType < (uintptr_t)UA_TYPES ||
               offset % sizeof(UA_DataType) != 0 ||
               offset / sizeof(UA_DataType) >= UA_TYPES_COUNT)
                return UA_STATUSCODE_BADNOTFOUND;
            res = UA_NodeId_copy(&em->memberType->typeId, &id);
        }
        if(res == UA_STATUSCODE_GOOD &&
           (!am->memberType || !UA_NodeId_equal(&id, &am->memberType->typeId)))
            res = UA_STATUSCODE_BADTYPEMISMATCH;
        UA_NodeId_clear(&id);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }
    return UA_STATUSCODE_GOOD;
}

static void
generatedTypesClear(UA_DataType *types, size_t size) {
    if(!types)
        return;
    for(size_t i = 0; i < size; i++)
        UA_DataType_clear(&types[i]);
    UA_free(types);
}

#ifdef UA_TYPES_STRUCTUREDESCRIPTION
static UA_StatusCode
copyGeneratedType(UA_Server *server, const UA_DataTypeArraySource *source,
                  size_t dependenciesSize, UA_DataTypeArraySource *const *dependencies,
                  size_t index, const UA_DataTypeArray *staged, UA_DataType *result) {
    const UA_DataType *original = &source->originTypes[index];
    UA_DataType view = *original;
    UA_DataTypeMember *members = NULL;
    if(generatedStructure(original) && original->membersSize) {
        members = (UA_DataTypeMember*)UA_malloc(
            original->membersSize * sizeof(UA_DataTypeMember));
        if(!members)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        memcpy(members, original->members,
               original->membersSize * sizeof(UA_DataTypeMember));
        view.members = members;
        /* Do not dereference mutable public descriptors during conversion. */
        for(size_t j = 0; j < view.membersSize; j++) {
            size_t memberIndex;
            const UA_DataTypeArraySource *owner = generatedTypeOrigin(
                source, dependenciesSize, dependencies, members[j].memberType, &memberIndex);
            if(owner)
                members[j].memberType = &owner->originTypes[memberIndex];
        }
    }
    UA_ExtensionObject description;
    UA_StatusCode res = UA_DataType_toDescription(&view, &description);
    UA_free(members);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_DataTypeDescription *base =
        (UA_DataTypeDescription*)description.content.decoded.data;
    UA_NodeId_clear(&base->dataTypeId);
    res = generatedNodeId(server, source, &original->typeId, &base->dataTypeId);
    if(res == UA_STATUSCODE_GOOD && generatedStructure(original)) {
        UA_StructureDescription *sd = (UA_StructureDescription*)base;
        UA_NodeId_clear(&sd->structureDefinition.defaultEncodingId);
        res = generatedNodeId(server, source, &original->binaryEncodingId,
                              &sd->structureDefinition.defaultEncodingId);
        for(size_t j = 0; res == UA_STATUSCODE_GOOD && j < original->membersSize; j++) {
            const UA_DataType *member = original->members[j].memberType;
            size_t memberIndex;
            const UA_DataTypeArraySource *owner = generatedTypeOrigin(
                source, dependenciesSize, dependencies, member, &memberIndex);
            if(owner) {
                UA_NodeId *id = &sd->structureDefinition.fields[j].dataType;
                UA_NodeId_clear(id);
                res = generatedNodeId(server, owner,
                                      &owner->originTypes[memberIndex].typeId, id);
            }
        }
    }
    if(res == UA_STATUSCODE_GOOD)
        res = UA_DataType_fromDescription(result, &description, staged);
    UA_ExtensionObject_clear(&description);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    /* Simple/enum descriptions and StructureDefinition do not carry both IDs. */
    UA_NodeId_clear(&result->binaryEncodingId);
    res = generatedNodeId(server, source, &original->binaryEncodingId,
                          &result->binaryEncodingId);
    if(res == UA_STATUSCODE_GOOD)
        res = generatedNodeId(server, source, &original->xmlEncodingId,
                              &result->xmlEncodingId);
    if(res != UA_STATUSCODE_GOOD)
        UA_DataType_clear(result);
    return res;
}
#endif

UA_StatusCode
UA_Server_addGeneratedDataTypes(UA_Server *server, UA_DataTypeArraySource *source,
                               size_t dependenciesSize,
                               UA_DataTypeArraySource *const *dependencies,
                               UA_Boolean copyTypes) {
    if(!server || !source || (source->typesSize &&
       (!source->originTypes || !source->staticTypes || !source->namespaceUris)) ||
       (dependenciesSize && !dependencies))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    for(size_t i = 0; i < dependenciesSize; i++) {
        if(!dependencies[i] || !dependencies[i]->originTypes ||
           !dependencies[i]->staticTypes || !dependencies[i]->namespaceUris)
            return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    if(source->typesSize == 0)
        return UA_STATUSCODE_GOOD;
    /* Every custom member must have immutable identity metadata. Do not
     * dereference descriptors that another server could bind in place. */
    for(size_t i = 0; i < source->typesSize; i++) {
        const UA_DataType *type = &source->originTypes[i];
        if(!generatedStructure(type))
            continue;
        if(type->membersSize && !type->members)
            return UA_STATUSCODE_BADINVALIDARGUMENT;
        for(size_t j = 0; j < type->membersSize; j++) {
            const UA_DataType *member = type->members[j].memberType;
            size_t index;
            if(generatedTypeOrigin(source, dependenciesSize, dependencies, member, &index))
                continue;
            uintptr_t offset = (uintptr_t)member - (uintptr_t)UA_TYPES;
            if((uintptr_t)member < (uintptr_t)UA_TYPES ||
               offset % sizeof(UA_DataType) != 0 ||
               offset / sizeof(UA_DataType) >= UA_TYPES_COUNT)
                return UA_STATUSCODE_BADNOTFOUND;
        }
    }
#ifndef UA_TYPES_STRUCTUREDESCRIPTION
    if(copyTypes)
        return UA_STATUSCODE_BADNOTSUPPORTED;
#endif
    lockServer(server);
    for(UA_GeneratedTypeBinding *b = server->generatedTypes; b; b = b->next) {
        if(b->source == source) {
            UA_StatusCode res = b->copied == copyTypes ?
                UA_STATUSCODE_GOOD : UA_STATUSCODE_BADINVALIDSTATE;
            unlockServer(server);
            return res;
        }
    }
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    size_t count = source->typesSize;
    UA_DataType *types = (UA_DataType*)UA_calloc(count, sizeof(UA_DataType));
    UA_Boolean *ready = (UA_Boolean*)UA_calloc(count, sizeof(UA_Boolean));
    UA_GeneratedTypeBinding *binding =
        (UA_GeneratedTypeBinding*)UA_calloc(1, sizeof(UA_GeneratedTypeBinding));
    UA_DataTypeArray staged = {NULL, count, types, false};
    size_t remaining = count;
    if(!types || !ready || !binding) {
        res = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }
    for(size_t i = 0; i < count; i++) {
        res = generatedNodeId(server, source, &source->originTypes[i].typeId, &types[i].typeId);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
        if(UA_findDataTypeWithCustom(&types[i].typeId, serverCustomTypes(server))) {
            res = UA_STATUSCODE_BADNODEIDEXISTS;
            goto cleanup;
        }
        for(size_t j = 0; j < i; j++) {
            if(UA_NodeId_equal(&types[i].typeId, &types[j].typeId)) {
                res = UA_STATUSCODE_BADNODEIDEXISTS;
                goto cleanup;
            }
        }
        /* Stable shells permit indirect cycles; inline dependencies wait. */
        types[i].typeKind = source->originTypes[i].typeKind;
        types[i].memSize = source->originTypes[i].memSize;
        types[i].pointerFree = source->originTypes[i].pointerFree;
        types[i].overlayable = source->originTypes[i].overlayable;
    }
    /* Validate registered dependency layouts before their pointers are used.
     * Inspect each present type once, so recursive graphs need no recursion. */
    for(size_t d = 0; d < dependenciesSize; d++) {
        const UA_DataTypeArraySource *dependency = dependencies[d];
        if(dependency == source)
            continue;
        for(size_t i = 0; i < dependency->typesSize; i++) {
            UA_NodeId id = UA_NODEID_NULL;
            res = generatedNodeId(server, dependency, &dependency->originTypes[i].typeId, &id);
            if(res != UA_STATUSCODE_GOOD)
                goto cleanup;
            const UA_DataType *actual = UA_findDataTypeWithCustom(&id, serverCustomTypes(server));
            UA_NodeId_clear(&id);
            if(!actual)
                continue;
            res = checkGeneratedDependency(server, source, dependenciesSize, dependencies,
                                            dependency, &dependency->originTypes[i], actual);
            if(res != UA_STATUSCODE_GOOD)
                goto cleanup;
        }
    }
    staged.next = (UA_DataTypeArray*)(uintptr_t)serverCustomTypes(server);
    while(remaining) {
        size_t before = remaining;
        for(size_t i = 0; i < count; i++) {
            if(ready[i])
                continue;
            const UA_DataType *original = &source->originTypes[i];
            UA_Boolean wait = false;
            if(generatedStructure(original)) {
                for(size_t j = 0; j < original->membersSize; j++) {
                    const UA_DataTypeMember *member = &original->members[j];
                    size_t target;
                    if(!member->isArray && !member->isOptional &&
                       generatedTypeIndex(source, member->memberType, &target) &&
                       !ready[target])
                        wait = true;
                }
            }
            if(wait)
                continue;
            UA_DataType built;
            memset(&built, 0, sizeof(built));
            if(copyTypes) {
#ifdef UA_TYPES_STRUCTUREDESCRIPTION
                res = copyGeneratedType(server, source, dependenciesSize, dependencies,
                                        i, &staged, &built);
#endif
            } else {
                res = UA_DataType_copy(original, &built);
                if(res == UA_STATUSCODE_GOOD) {
                    UA_NodeId_clear(&built.typeId);
                    UA_NodeId_clear(&built.binaryEncodingId);
                    UA_NodeId_clear(&built.xmlEncodingId);
                    res = generatedNodeId(server, source, &original->typeId, &built.typeId);
                    res |= generatedNodeId(server, source, &original->binaryEncodingId,
                                           &built.binaryEncodingId);
                    res |= generatedNodeId(server, source, &original->xmlEncodingId,
                                           &built.xmlEncodingId);
                }
                if(res == UA_STATUSCODE_GOOD && generatedStructure(original)) {
                    UA_DataTypeMember *members = (UA_DataTypeMember*)(uintptr_t)built.members;
                    for(size_t j = 0; j < original->membersSize; j++) {
                        size_t target;
                        const UA_DataTypeArraySource *owner = generatedTypeOrigin(
                            source, dependenciesSize, dependencies,
                            original->members[j].memberType, &target);
                        if(!owner)
                            continue; /* Builtin type. */
                        UA_NodeId id = UA_NODEID_NULL;
                        res = generatedNodeId(server, owner, &owner->originTypes[target].typeId, &id);
                        if(res == UA_STATUSCODE_GOOD)
                            members[j].memberType = UA_findDataTypeWithCustom(&id, &staged);
                        UA_NodeId_clear(&id);
                        if(res != UA_STATUSCODE_GOOD || !members[j].memberType) {
                            res = UA_STATUSCODE_BADNOTFOUND;
                            break;
                        }
                    }
                }
            }
            if(res != UA_STATUSCODE_GOOD) {
                UA_DataType_clear(&built);
                goto cleanup;
            }
            if(built.memSize != original->memSize || built.typeKind != original->typeKind ||
               built.membersSize != original->membersSize) {
                UA_DataType_clear(&built);
                res = UA_STATUSCODE_BADTYPEMISMATCH;
                goto cleanup;
            }
            if(generatedStructure(original)) {
                UA_DataTypeMember *members = (UA_DataTypeMember*)(uintptr_t)built.members;
                for(size_t j = 0; j < built.membersSize; j++) {
                    if(members[j].padding != original->members[j].padding ||
                       members[j].isArray != original->members[j].isArray ||
                       members[j].isOptional != original->members[j].isOptional) {
                        res = UA_STATUSCODE_BADTYPEMISMATCH;
                        break;
                    }
                    if(members[j].memberType == &built)
                        members[j].memberType = &types[i];
                }
            }
            if(res != UA_STATUSCODE_GOOD) {
                UA_DataType_clear(&built);
                goto cleanup;
            }
            UA_NodeId_clear(&types[i].typeId);
            types[i] = built;
            ready[i] = true;
            remaining--;
        }
        if(before == remaining) {
            res = UA_STATUSCODE_BADNOTSUPPORTED;
            goto cleanup;
        }
    }
    if(!copyTypes) {
        for(size_t i = 0; i < count; i++) {
            const UA_DataType *target = &source->staticTypes[i];
            if(source->staticTypesAreConst &&
               (!UA_NodeId_equal(&target->typeId, &types[i].typeId) ||
                !UA_NodeId_equal(&target->binaryEncodingId, &types[i].binaryEncodingId) ||
                !UA_NodeId_equal(&target->xmlEncodingId, &types[i].xmlEncodingId))) {
                res = UA_STATUSCODE_BADCONFIGURATIONERROR;
                goto cleanup;
            }
            if(!generatedStructure(target))
                continue;
            for(size_t j = 0; j < target->membersSize; j++) {
                const UA_DataType *member = types[i].members[j].memberType;
                size_t targetIndex;
                if(generatedTypeIndex(source, member, &targetIndex))
                    member = &source->staticTypes[targetIndex];
                else {
                    uintptr_t offset = (uintptr_t)member - (uintptr_t)types;
                    if((uintptr_t)member >= (uintptr_t)types &&
                       offset % sizeof(UA_DataType) == 0 && offset / sizeof(UA_DataType) < count)
                        member = &source->staticTypes[offset / sizeof(UA_DataType)];
                }
                if(target->members[j].memberType != member) {
                    res = UA_STATUSCODE_BADCONFIGURATIONERROR;
                    goto cleanup;
                }
            }
        }
        if(!source->staticTypesAreConst) {
            uintptr_t expected = 0;
            UA_atomic_cmpxchg(&source->inUse, &expected, (uintptr_t)1);
            if(expected != 0) {
                res = UA_STATUSCODE_BADINVALIDSTATE;
                goto cleanup;
            }
            for(size_t i = 0; i < count; i++) {
                UA_DataType *target = (UA_DataType*)(uintptr_t)&source->staticTypes[i];
                target->typeId.namespaceIndex = types[i].typeId.namespaceIndex;
                target->binaryEncodingId.namespaceIndex = types[i].binaryEncodingId.namespaceIndex;
                target->xmlEncodingId.namespaceIndex = types[i].xmlEncodingId.namespaceIndex;
            }
        }
    }
    binding->source = source;
    binding->copied = copyTypes;
    binding->array.typesSize = count;
    binding->array.types = copyTypes ? types : source->staticTypes;
    binding->array.next = server->config.customDataTypes;
    server->config.customDataTypes = &binding->array;
    binding->next = server->generatedTypes;
    server->generatedTypes = binding;
    binding = NULL;
    if(copyTypes)
        types = NULL;
cleanup:
    generatedTypesClear(types, count);
    UA_free(ready);
    UA_free(binding);
    unlockServer(server);
    return res;
}

const UA_DataType *
UA_Server_getGeneratedDataType(UA_Server *server, const UA_DataTypeArraySource *source,
                               size_t index) {
    if(!server || !source || index >= source->typesSize)
        return NULL;
    lockServer(server);
    const UA_DataType *result = NULL;
    for(UA_GeneratedTypeBinding *b = server->generatedTypes; b; b = b->next) {
        if(b->source == source) {
            result = &b->array.types[index];
            break;
        }
    }
    unlockServer(server);
    return result;
}

/********************************/
/* Information Model Operations */
/********************************/

struct ReturnTypeContext {
    UA_Server *server;
    UA_UInt32 attributeMask;
    UA_ReferenceTypeSet references;
    UA_BrowseDirection referenceDirections;
};

static void *
returnFirstType(void *context, UA_ReferenceTarget *t) {
    struct ReturnTypeContext *ctx = (struct ReturnTypeContext*)context;
    /* Don't release the node that is returned.
     * Continues to iterate if NULL is returned. */
    return (void *)(uintptr_t)UA_NODESTORE_GETFROMREF_SELECTIVE(
        ctx->server, t->targetId, ctx->attributeMask, ctx->references,
        ctx->referenceDirections);
}

const UA_Node *
getNodeType(UA_Server *server, const UA_NodeHead *head,
            UA_UInt32 attributeMask, UA_ReferenceTypeSet references,
            UA_BrowseDirection referenceDirections) {
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

    struct ReturnTypeContext ctx;
    ctx.server = server;
    ctx.attributeMask = attributeMask;
    ctx.references = references;
    ctx.referenceDirections = referenceDirections;

    /* Return the first matching candidate */
    for(size_t i = 0; i < head->referencesSize; ++i) {
        UA_NodeReferenceKind *rk = &head->references[i];
        if(rk->isInverse != inverse)
            continue;
        if(rk->referenceTypeIndex != parentRefIndex)
            continue;
        const UA_Node *type = (const UA_Node*)
            UA_NodeReferenceKind_iterate(rk, returnFirstType, &ctx);
        if(type)
            return type;
    }

    return NULL;
}

struct CopyTypeContext {
    UA_NodeId *out;
    UA_StatusCode res;
};

static void *
copyFirstType(void *context, UA_ReferenceTarget *t) {
    struct CopyTypeContext *ctx = (struct CopyTypeContext*)context;
    UA_NodeId id = UA_NodePointer_toNodeId(t->targetId);
    ctx->res = UA_NodeId_copy(&id, ctx->out);
    return (void*)0x01; /* stop early */
}

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Server_getNodeType(UA_Server *server, const UA_NodeId nodeId,
                      UA_NodeId *outTypeId) {
    lockServer(server);

    UA_ReferenceTypeSet refs;
    UA_ReferenceTypeSet_init(&refs);
    UA_ReferenceTypeSet_add(&refs, UA_REFERENCETYPEINDEX_HASTYPEDEFINITION);
    UA_ReferenceTypeSet_add(&refs, UA_REFERENCETYPEINDEX_HASSUBTYPE);

    const UA_Node *node =
        UA_NODESTORE_GET_SELECTIVE(server, &nodeId,
                                          UA_NODEATTRIBUTESMASK_NONE, refs,
                                          UA_BROWSEDIRECTION_BOTH);
    if(!node) {
        unlockServer(server);
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    UA_Byte parentRefIndex;
    UA_Boolean inverse;
    switch(node->head.nodeClass) {
    default:
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
    }

    /* Return the first matching candidate */
    struct CopyTypeContext ctx;
    ctx.out = outTypeId;
    ctx.res = UA_STATUSCODE_BADNOTFOUND;
    for(size_t i = 0; i < node->head.referencesSize; ++i) {
        UA_NodeReferenceKind *rk = &node->head.references[i];
        if(rk->isInverse != inverse)
            continue;
        if(rk->referenceTypeIndex != parentRefIndex)
            continue;
        UA_NodeReferenceKind_iterate(rk, copyFirstType, &ctx);
        break;
    }

    UA_NODESTORE_RELEASE(server, node);
    unlockServer(server);
    return ctx.res;
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

struct NodeVersionPropertyContext {
    UA_Server *server;
    UA_NodeId *outPropertyId;
    UA_UInt32 browseNameHash;
    UA_StatusCode res;
};

static void *
findNodeVersionProperty(void *context, UA_ReferenceTarget *target) {
    struct NodeVersionPropertyContext *ctx =
        (struct NodeVersionPropertyContext*)context;
    if(target->targetNameHash != ctx->browseNameHash ||
       !UA_NodePointer_isLocal(target->targetId))
        return NULL;

    const UA_Node *property =
        UA_NODESTORE_GETFROMREF_SELECTIVE(ctx->server, target->targetId,
                                         UA_NODEATTRIBUTESMASK_NODECLASS |
                                         UA_NODEATTRIBUTESMASK_BROWSENAME |
                                         UA_NODEATTRIBUTESMASK_DATATYPE |
                                         UA_NODEATTRIBUTESMASK_VALUERANK,
                                         UA_REFERENCETYPESET_NONE,
                                         UA_BROWSEDIRECTION_INVALID);
    if(!property)
        return NULL;

    const UA_QualifiedName nodeVersionName = UA_QUALIFIEDNAME(0, "NodeVersion");
    UA_Boolean matches =
        property->head.nodeClass == UA_NODECLASS_VARIABLE &&
        UA_QualifiedName_equal(&property->head.browseName, &nodeVersionName) &&
        UA_NodeId_equal(&property->variableNode.dataType,
                        &UA_TYPES[UA_TYPES_STRING].typeId) &&
        property->variableNode.valueRank == UA_VALUERANK_SCALAR;
    if(matches) {
        UA_NodeId id = UA_NodePointer_toNodeId(target->targetId);
        ctx->res = UA_NodeId_copy(&id, ctx->outPropertyId);
    }
    UA_NODESTORE_RELEASE(ctx->server, property);
    return matches ? (void*)0x01 : NULL;
}

UA_StatusCode
getNodeVersionProperty(UA_Server *server, const UA_NodeHead *head,
                       UA_NodeId *outPropertyId) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_NodeId_init(outPropertyId);

    UA_ReferenceTypeSet propertyRefs;
    const UA_NodeId hasProperty = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    UA_StatusCode res =
        referenceTypeIndices(server, &hasProperty, &propertyRefs, true);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    const UA_QualifiedName nodeVersionName = UA_QUALIFIEDNAME(0, "NodeVersion");
    struct NodeVersionPropertyContext ctx = {
        server, outPropertyId, UA_QualifiedName_hash(&nodeVersionName),
        UA_STATUSCODE_BADNOTFOUND};
    for(size_t i = 0; i < head->referencesSize; ++i) {
        UA_NodeReferenceKind *rk = &head->references[i];
        if(rk->isInverse ||
           !UA_ReferenceTypeSet_contains(&propertyRefs, rk->referenceTypeIndex))
            continue;
        if(UA_NodeReferenceKind_iterate(rk, findNodeVersionProperty, &ctx))
            break;
    }
    return ctx.res;
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
        /* memmove: source and destination can overlap on 32-bit targets,
         * where sizeof(UA_ExpandedNodeId) < 2 * sizeof(UA_NodeId) */
        memmove(n, &e->nodeId, sizeof(UA_NodeId));
        UA_String_clear(&e->namespaceUri);
        pos++;
    }

    *typeHierarchySize = pos;
    *typeHierarchy = outArray;
    return UA_STATUSCODE_GOOD;
}

static UA_SecureChannel *
findSecureChannel(UA_Server *server, UA_UInt32 channelId) {
    UA_SecureChannel *channel;
    TAILQ_FOREACH(channel, &server->channels, serverEntry) {
        if(channel->securityToken.channelId == channelId)
            return channel;
    }
    return NULL;
}

void
shutdownSecureChannel(UA_Server *server, UA_SecureChannel *channel,
                      UA_ShutdownReason reason) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    switch(channel->transport) {
    case UA_SECURECHANNEL_TRANSPORT_UACP:
        UA_SecureChannel_shutdown(channel, reason);
        break;
    case UA_SECURECHANNEL_TRANSPORT_HTTP:
        shutdownHttpSecureChannel(server, channel, reason);
        break;
    default:
        UA_assert(false);
    }
}

UA_StatusCode
UA_Server_closeSecureChannel(UA_Server *server, UA_UInt32 channelId,
                             UA_ShutdownReason reason) {
    lockServer(server);
    UA_SecureChannel *channel = findSecureChannel(server, channelId);
    if(channel) {
        shutdownSecureChannel(server, channel, reason);
        unlockServer(server);
        return UA_STATUSCODE_GOOD;
    }
    unlockServer(server);
    return UA_STATUSCODE_BADNOTFOUND;
}

/* The one SecureChannel attribute key interpreted -- and the only ns0 key
 * writable -- by the server itself. See the doc comment on
 * UA_Server_setSecureChannelAttribute in server.h. */
static const UA_QualifiedName maxMessageSizeAttributeKey =
    {0, UA_STRING_STATIC("maxMessageSize")};

/* Namespace 0 is reserved for the server-defined SecureChannel attributes:
 * UA_SecureChannel_builtinAttributeKeys (read-only, computed on access from
 * the live channel state -- see UA_SecureChannel_getBuiltinAttribute) and
 * maxMessageSizeAttributeKey (the one writable key, stored in
 * channel->attributes). Any other ns0 key is rejected on both read and
 * write; applications get their own free-form key/value space by using a
 * non-zero namespace. */
static UA_Boolean
isWritableSecureChannelAttribute(const UA_QualifiedName *key) {
    return UA_QualifiedName_equal(key, &maxMessageSizeAttributeKey);
}

UA_StatusCode
UA_Server_getSecureChannelAttribute(UA_Server *server, UA_UInt32 channelId,
                                    const UA_QualifiedName key,
                                    UA_Variant *outValue) {
    if(!outValue)
        return UA_STATUSCODE_BADINTERNALERROR;
    lockServer(server);
    UA_SecureChannel *channel = findSecureChannel(server, channelId);
    if(!channel) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    if(UA_SecureChannel_getBuiltinAttribute(channel, &key, outValue)) {
        outValue->storageType = UA_VARIANT_DATA_NODELETE;
        unlockServer(server);
        return UA_STATUSCODE_GOOD;
    }
    if(key.namespaceIndex == 0 && !isWritableSecureChannelAttribute(&key)) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    const UA_Variant *attr = UA_KeyValueMap_get(&channel->attributes, key);
    if(!attr) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    *outValue = *attr;
    outValue->storageType = UA_VARIANT_DATA_NODELETE;
    unlockServer(server);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_getSecureChannelAttributeCopy(UA_Server *server, UA_UInt32 channelId,
                                        const UA_QualifiedName key,
                                        UA_Variant *outValue) {
    if(!outValue)
        return UA_STATUSCODE_BADINTERNALERROR;
    lockServer(server);
    UA_SecureChannel *channel = findSecureChannel(server, channelId);
    if(!channel) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_Variant builtin;
    if(UA_SecureChannel_getBuiltinAttribute(channel, &key, &builtin)) {
        UA_StatusCode res = UA_Variant_copy(&builtin, outValue);
        unlockServer(server);
        return res;
    }
    if(key.namespaceIndex == 0 && !isWritableSecureChannelAttribute(&key)) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    const UA_Variant *attr = UA_KeyValueMap_get(&channel->attributes, key);
    UA_StatusCode res = attr ?
        UA_Variant_copy(attr, outValue) : UA_STATUSCODE_BADNOTFOUND;
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_getSecureChannelAttribute_scalar(UA_Server *server,
                                           UA_UInt32 channelId,
                                           const UA_QualifiedName key,
                                           const UA_DataType *type,
                                           void *outValue) {
    lockServer(server);
    UA_SecureChannel *channel = findSecureChannel(server, channelId);
    if(!channel) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_Variant builtin;
    if(UA_SecureChannel_getBuiltinAttribute(channel, &key, &builtin)) {
        if(!UA_Variant_hasScalarType(&builtin, type)) {
            unlockServer(server);
            return UA_STATUSCODE_BADNOTFOUND;
        }
        memcpy(outValue, builtin.data, type->memSize);
        unlockServer(server);
        return UA_STATUSCODE_GOOD;
    }
    if(key.namespaceIndex == 0 && !isWritableSecureChannelAttribute(&key)) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    const UA_Variant *attr = UA_KeyValueMap_get(&channel->attributes, key);
    if(!attr || !UA_Variant_hasScalarType(attr, type)) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    memcpy(outValue, attr->data, type->memSize);
    unlockServer(server);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_setSecureChannelAttribute(UA_Server *server, UA_UInt32 channelId,
                                    const UA_QualifiedName key,
                                    const UA_Variant *value) {
    lockServer(server);
    UA_SecureChannel *channel = findSecureChannel(server, channelId);
    if(!channel) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_Boolean isMaxMessageSize = isWritableSecureChannelAttribute(&key);
    if(key.namespaceIndex == 0 && !isMaxMessageSize) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTWRITABLE;
    }
    if(isMaxMessageSize &&
       (!value || !UA_Variant_hasScalarType(value, &UA_TYPES[UA_TYPES_UINT32]))) {
        unlockServer(server);
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }
    UA_StatusCode res = UA_KeyValueMap_set(&channel->attributes, key, value);
    if(res == UA_STATUSCODE_GOOD && isMaxMessageSize)
        channel->maxMessageSizeOverride = *(const UA_UInt32*)value->data;
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_deleteSecureChannelAttribute(UA_Server *server, UA_UInt32 channelId,
                                       const UA_QualifiedName key) {
    lockServer(server);
    UA_SecureChannel *channel = findSecureChannel(server, channelId);
    if(!channel) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_Boolean isMaxMessageSize = isWritableSecureChannelAttribute(&key);
    if(key.namespaceIndex == 0 && !isMaxMessageSize) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTWRITABLE;
    }
    UA_StatusCode res = UA_KeyValueMap_remove(&channel->attributes, key);
    if(res == UA_STATUSCODE_GOOD && isMaxMessageSize)
        channel->maxMessageSizeOverride = 0;
    unlockServer(server);
    return res;
}

UA_StatusCode
registerSecureChannel(UA_Server *server, UA_SecureChannel *channel) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_assert(channel->securityToken.channelId == 0);

    /* ChannelIds share one namespace across all transports. Skip zero and
     * identifiers that remain active after the counter wraps around. */
    UA_UInt32 start = server->nextChannelId;
    do {
        UA_UInt32 channelId = server->nextChannelId++;
        if(channelId != 0 && !findSecureChannel(server, channelId)) {
            channel->securityToken.channelId = channelId;
            TAILQ_INSERT_TAIL(&server->channels, channel, serverEntry);
            return UA_STATUSCODE_GOOD;
        }
    } while(server->nextChannelId != start);
    return UA_STATUSCODE_BADOUTOFMEMORY;
}

void
unregisterSecureChannel(UA_Server *server, UA_SecureChannel *channel) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    TAILQ_REMOVE(&server->channels, channel, serverEntry);
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
        /* memmove, see getTypeAndInterfaceHierarchy */
        memmove(n, &e->nodeId, sizeof(UA_NodeId));
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

void
notifyApplication(UA_Server *server, UA_ApplicationNotificationType type,
                  const UA_KeyValueMap payload) {
    /* Notify the drivers */
    UA_UInt64 filter = type & 0xFFFFFFFF00000000ULL; /* Only the high 32bit */
    for(UA_Driver *drv = server->drivers; drv; drv = drv->next) {
        if((drv->notificationFilter & filter) != 0 && drv->notificationCallback)
            drv->notificationCallback(drv, type, payload);
    }

    /* Specialized application notification callbacks */
    UA_ServerConfig *config = &server->config;
    if((type & UA_APPLICATIONNOTIFICATIONTYPE_LIFECYCLE) != 0 &&
       config->lifecycleNotificationCallback)
        config->lifecycleNotificationCallback(server, type, payload);
    if((type & UA_APPLICATIONNOTIFICATIONTYPE_SECURECHANNEL) != 0 &&
       config->secureChannelNotificationCallback)
        config->secureChannelNotificationCallback(server, type, payload);
    if((type & UA_APPLICATIONNOTIFICATIONTYPE_SESSION) != 0 &&
       config->sessionNotificationCallback)
        config->sessionNotificationCallback(server, type, payload);
    if((type & UA_APPLICATIONNOTIFICATIONTYPE_SERVICE) != 0 &&
       config->serviceNotificationCallback)
        config->serviceNotificationCallback(server, type, payload);
    if((type & UA_APPLICATIONNOTIFICATIONTYPE_SUBSCRIPTION) != 0 &&
       config->subscriptionNotificationCallback)
        config->subscriptionNotificationCallback(server, type, payload);
#ifdef UA_ENABLE_AUDITING
    if((type & UA_APPLICATIONNOTIFICATIONTYPE_AUDIT) != 0 &&
       config->auditNotificationCallback)
        config->auditNotificationCallback(server, type, payload);
#endif

    /* Global application notification */
    if(config->globalNotificationCallback)
        config->globalNotificationCallback(server, type, payload);
}

/**************************/
/* Certificate Validation */
/**************************/

#ifdef UA_ENABLE_AUDITING
/* Map the status code of the certificate validation to the audit event type. */
static UA_ApplicationNotificationType
certificateAuditType(UA_StatusCode res) {
    switch(res) {
    case UA_STATUSCODE_BADCERTIFICATEINVALID:
    case UA_STATUSCODE_BADCERTIFICATECHAININCOMPLETE:
    case UA_STATUSCODE_BADCERTIFICATEPOLICYCHECKFAILED:
        return UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CERTIFICATE_INVALID;
    case UA_STATUSCODE_BADCERTIFICATEUNTRUSTED:
        return UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CERTIFICATE_UNTRUSTED;
    case UA_STATUSCODE_BADCERTIFICATETIMEINVALID:
    case UA_STATUSCODE_BADCERTIFICATEISSUERTIMEINVALID:
        return UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CERTIFICATE_EXPIRED;
    case UA_STATUSCODE_BADCERTIFICATEHOSTNAMEINVALID:
    case UA_STATUSCODE_BADCERTIFICATEURIINVALID:
        return UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CERTIFICATE_DATAMISMATCH;
    case UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED:
    case UA_STATUSCODE_BADCERTIFICATEISSUERUSENOTALLOWED:
        return UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CERTIFICATE_MISMATCH;
    case UA_STATUSCODE_BADCERTIFICATEREVOCATIONUNKNOWN:
    case UA_STATUSCODE_BADCERTIFICATEISSUERREVOCATIONUNKNOWN:
    case UA_STATUSCODE_BADCERTIFICATEREVOKED:
    case UA_STATUSCODE_BADCERTIFICATEISSUERREVOKED:
        return UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CERTIFICATE_REVOKED;
    default:
        return UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CERTIFICATE;
    }
}
#endif

UA_StatusCode
validateCertificate(UA_Server *server, UA_CertificateGroup *cg,
                    const UA_SecurityPolicy *securityPolicy,
                    UA_SecureChannel *channel, UA_Session *session,
                    const char *logPrefix,
                    const UA_ApplicationDescription *ad,
                    const UA_ByteString certificate) {
    /* Verify the ApplicationUri */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(ad) {
        res = UA_CertificateUtils_verifyApplicationUri(&certificate, &ad->applicationUri);
        if(res != UA_STATUSCODE_GOOD) {
            if(server->config.allowAllCertificateUris <= UA_RULEHANDLING_WARN) {
                if(session) {
                    UA_LOG_ERROR_SESSION(server->config.logging, session,
                                         "%s: The client's ApplicationUri "
                                         "could not be verified against the "
                                         "ApplicationUri %S from the client's "
                                         "ApplicationDescription", logPrefix,
                                         ad->applicationUri);
                } else if(channel) {
                    UA_LOG_ERROR_CHANNEL(server->config.logging, channel,
                                         "%s: The client certificate's ApplicationUri "
                                         "could not be verified against the "
                                         "ApplicationUri %S from the client's "
                                         "ApplicationDescription", logPrefix,
                                         ad->applicationUri);
                } else {
                    UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                                 "%s: The server certificate's ApplicationUri "
                                 "could not be verified against the "
                                 "ApplicationUri %S from its "
                                 "ApplicationDescription", logPrefix,
                                 ad->applicationUri);
                }
            }

            /* Throw an audit event and abort */
            if(server->config.allowAllCertificateUris < UA_RULEHANDLING_WARN) {
#ifdef UA_ENABLE_AUDITING
                auditCertificateDataMismatchEvent(server, channel, session, logPrefix,
                                                  res, certificate, ad->applicationUri);
#endif
                return (res == UA_STATUSCODE_BADCERTIFICATEURIINVALID) ?
                    UA_STATUSCODE_BADCERTIFICATEURIINVALID :
                    UA_STATUSCODE_BADCERTIFICATEINVALID;
            }

            /* Ignore the bad result depending on the server configuration.
             * The res variable gets overwritten in all cases. */
        }
    }

    /* The server receives client application and user certificates. RSA
     * application certificates require clientAuth. For ECC application and user
     * certificates the extension is optional, but restrictive values apply. */
    if(securityPolicy && securityPolicy->policyType != UA_SECURITYPOLICYTYPE_NONE) {
        UA_Boolean applicationCertificate =
            (cg == &server->config.secureChannelPKI);
        UA_Boolean ekuRequired = applicationCertificate &&
            securityPolicy->policyType == UA_SECURITYPOLICYTYPE_RSA;
        res = UA_CertificateUtils_checkExtendedKeyUsage(
            &certificate, UA_CERTIFICATEEKU_CLIENTAUTH, ekuRequired);
        if(res == UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED) {
            if(server->config.certificateEkuRule <= UA_RULEHANDLING_WARN) {
                UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SECURITYPOLICY,
                               "%s: The client certificate does not permit clientAuth",
                               logPrefix);
            }
            if(server->config.certificateEkuRule == UA_RULEHANDLING_ABORT)
                goto errout;
        } else if(res != UA_STATUSCODE_GOOD) {
            goto errout;
        }
    }

    if(!cg->verifyCertificate) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "%s: Could not validate the certificate "
                     "as the CertificateGroup is not configured", logPrefix);
        res = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    /* Validate in the CertificateGroup */
    res = cg->verifyCertificate(cg, &certificate);
    if(res != UA_STATUSCODE_GOOD) {
        const char *descr = UA_StatusCode_name(res);
        if(session) {
            UA_LOG_ERROR_SESSION(server->config.logging, session,
                                 "%s: The client certificate failed the verification "
                                 "in the CertificateGroup with StatusCode %s",
                                 logPrefix, descr);
        } else if(channel) {
            UA_LOG_ERROR_CHANNEL(server->config.logging, channel,
                                 "%s: The client certificate failed the verification "
                                 "in the CertificateGroup with StatusCode %s",
                                 logPrefix, descr);
        } else {
            UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                         "%s: The client certificate failed the verification "
                         "in the CertificateGroup with StatusCode %s",
                         logPrefix, descr);
        }
    }

 errout:
#ifdef UA_ENABLE_AUDITING
    /* Create the audit event */
    auditCertificateEvent(server, certificateAuditType(res),
                          channel, session, logPrefix, res, certificate, UA_STRING(""));
#endif

    return res;
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
