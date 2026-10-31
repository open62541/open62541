# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
"""Private support emitted into generated namespace C files."""
SUPPORT = r'''#include <open62541/server.h>
#include <open62541/util.h>

/* All descriptors are immutable. namespaceUris describes their baked indexes,
 * which are either a pinned server map or a compiler-local source map. */
typedef struct {
    const UA_DataType *types;
    size_t typesSize;
    const UA_String *namespaceUris;
    size_t namespaceUrisSize;
    UA_Boolean pinned;
    const size_t *order;
} UA_GeneratedTypeSource;

static UA_INLINE UA_Boolean
UA_GeneratedTypes_index(const UA_GeneratedTypeSource *s, const UA_DataType *t, size_t *i) {
    uintptr_t p = (uintptr_t)t, base = (uintptr_t)s->types;
    if(p < base || (p-base) % sizeof(UA_DataType) ||
       (p-base) / sizeof(UA_DataType) >= s->typesSize)
        return false;
    *i = (size_t)((p-base) / sizeof(UA_DataType));
    return true;
}
static UA_INLINE UA_StatusCode
UA_GeneratedTypes_checkPins(UA_Server *server, const UA_GeneratedTypeSource *s) {
    if(!s->pinned) return UA_STATUSCODE_BADCONFIGURATIONERROR;
    for(size_t i = 0; i < s->namespaceUrisSize; i++) {
        if(!s->namespaceUris[i].length) continue;
        size_t actual;
        if(UA_Server_getNamespaceByName(server, s->namespaceUris[i], &actual) == UA_STATUSCODE_GOOD &&
           actual != i) return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }
    return UA_STATUSCODE_GOOD;
}
static UA_INLINE UA_StatusCode
UA_GeneratedTypes_view(UA_Server *server, const UA_GeneratedTypeSource *s,
                       const UA_NodeId *id, UA_NodeId *out) {
    *out = *id; /* Borrow immutable identifier storage for lookup/validation. */
    if(UA_NodeId_isNull(id)) return UA_STATUSCODE_GOOD;
    size_t ns = 0;
    UA_StatusCode r = UA_Server_getNamespaceByName(server, s->namespaceUris[id->namespaceIndex], &ns);
    if(!r) out->namespaceIndex = (UA_UInt16)ns;
    return r;
}
static UA_INLINE const UA_DataType *
UA_GeneratedTypes_find(UA_Server *server, const UA_GeneratedTypeSource *s, size_t i) {
    UA_NodeId id = UA_NODEID_NULL;
    UA_StatusCode r = UA_GeneratedTypes_view(server, s, &s->types[i].typeId, &id);
    return r ? NULL : UA_Server_findDataType(server, &id);
}
#ifdef UA_TYPES_STRUCTUREDESCRIPTION
static UA_INLINE const UA_GeneratedTypeSource *
UA_GeneratedTypes_owner(const UA_GeneratedTypeSource *s, size_t size,
                        const UA_GeneratedTypeSource *const *deps,
                        const UA_DataType *t, size_t *i) {
    if(UA_GeneratedTypes_index(s, t, i)) return s;
    for(size_t d = 0; d < size; d++)
        if(UA_GeneratedTypes_index(deps[d], t, i)) return deps[d];
    return NULL;
}
static UA_INLINE UA_StatusCode
UA_GeneratedTypes_map(UA_Server *server, const UA_GeneratedTypeSource *s,
                      const UA_NodeId *id, UA_NodeId *out) {
    UA_NodeId view;
    UA_StatusCode r = UA_GeneratedTypes_view(server, s, id, &view);
    return r ? r : UA_NodeId_copy(&view, out);
}
static UA_INLINE UA_StatusCode
UA_GeneratedTypes_copy(UA_Server *server, const UA_GeneratedTypeSource *s,
                       size_t size, const UA_GeneratedTypeSource *const *deps,
                       size_t i, const UA_DataTypeArray *available, UA_DataType *result) {
    const UA_DataType *original = &s->types[i];
    UA_ExtensionObject description;
    UA_StatusCode r = UA_DataType_toDescription(original, &description);
    if(r) return r;
    UA_DataTypeDescription *base = (UA_DataTypeDescription*)description.content.decoded.data;
    UA_NodeId_clear(&base->dataTypeId);
    r = UA_GeneratedTypes_map(server, s, &original->typeId, &base->dataTypeId);
    if(!r && UA_ExtensionObject_hasDecodedType(
           &description, &UA_TYPES[UA_TYPES_STRUCTUREDESCRIPTION])) {
        UA_StructureDefinition *sd = &((UA_StructureDescription*)base)->structureDefinition;
        UA_NodeId_clear(&sd->defaultEncodingId); /* Restored after conversion. */
        for(size_t m = 0; !r && m < original->membersSize; m++) {
            size_t k;
            const UA_GeneratedTypeSource *o = UA_GeneratedTypes_owner(
                s, size, deps, original->members[m].memberType, &k);
            if(!o) continue; /* Standard SDK members already use namespace zero. */
            UA_NodeId_clear(&sd->fields[m].dataType);
            r = UA_GeneratedTypes_map(server, o, &o->types[k].typeId, &sd->fields[m].dataType);
        }
    }
    if(!r) r = UA_DataType_fromDescription(result, &description, available);
    UA_ExtensionObject_clear(&description);
    /* Descriptions do not carry both encoding IDs. */
    if(!r) {
        UA_NodeId_clear(&result->binaryEncodingId);
        r = UA_GeneratedTypes_map(server, s, &original->binaryEncodingId, &result->binaryEncodingId);
    }
    if(!r) r = UA_GeneratedTypes_map(server, s, &original->xmlEncodingId, &result->xmlEncodingId);
    return r;
}
#endif

static UA_INLINE UA_StatusCode
UA_GeneratedTypes_register(UA_Server *server, const UA_GeneratedTypeSource *s,
                           size_t size, const UA_GeneratedTypeSource *const *deps,
                           UA_Boolean copy) {
    if(copy) {
#ifndef UA_TYPES_STRUCTUREDESCRIPTION
        return UA_STATUSCODE_BADNOTSUPPORTED;
#else
        for(size_t j = 0; j < s->typesSize; j++) {
            size_t i = s->order[j];
            const UA_DataType *actual = UA_GeneratedTypes_find(server, s, i);
            if(actual) {
                if(actual == &s->types[i]) return UA_STATUSCODE_BADINVALIDSTATE;
                continue;
            }
            UA_DataType type;
            memset(&type, 0, sizeof(type));
            UA_StatusCode r = UA_GeneratedTypes_copy(server, s, size, deps, i,
                                                     UA_Server_getDataTypes(server), &type);
            if(!r) r = UA_Server_addDataType(server, UA_NODEID_NULL, &type);
            UA_DataType_clear(&type);
            if(r) return r;
        }
        return UA_STATUSCODE_GOOD;
#endif
    }
    if(UA_GeneratedTypes_checkPins(server, s)) return UA_STATUSCODE_BADCONFIGURATIONERROR;
    size_t present = 0;
    for(size_t i = 0; i < s->typesSize; i++) {
        const UA_DataType *actual = UA_GeneratedTypes_find(server, s, i);
        if(!actual) continue;
        if(actual != &s->types[i]) return UA_STATUSCODE_BADINVALIDSTATE;
        present++;
    }
    if(present) return present == s->typesSize ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADNODEIDEXISTS;
    /* Static members must retain their registered dependency addresses. */
    for(size_t d = 0; d < size; d++) {
        if(deps[d]->types == s->types) continue;
        for(size_t i = 0; i < deps[d]->typesSize; i++) {
            const UA_DataType *actual = UA_GeneratedTypes_find(server, deps[d], i);
            if(actual && actual != &deps[d]->types[i]) return UA_STATUSCODE_BADCONFIGURATIONERROR;
        }
    }
    /* A borrowed entry is followed by an empty owning entry. The latter
     * frees their common allocation after the borrowed entry was visited;
     * the read-only datatype array itself is never freed. */
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_DataTypeArray *entries = (UA_DataTypeArray*)UA_calloc(2, sizeof(UA_DataTypeArray));
    if(!entries) return UA_STATUSCODE_BADOUTOFMEMORY;
    entries[0].cleanup = true;
    entries[0].next = config->customDataTypes;
    entries[1].types = s->types;
    entries[1].typesSize = s->typesSize;
    entries[1].next = &entries[0];
    config->customDataTypes = &entries[1];
    return UA_STATUSCODE_GOOD;
}
'''
