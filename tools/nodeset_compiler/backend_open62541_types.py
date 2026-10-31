# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
"""Private support emitted into generated implementation headers."""
SUPPORT = r'''#ifndef UA_GENERATED_TYPES_SUPPORT_H_
#define UA_GENERATED_TYPES_SUPPORT_H_
#include <open62541/server.h>
#include <open62541/util.h>

/* All descriptors are immutable. namespaceUris describes their baked indexes,
 * which are either a pinned server map or a compiler-local source map. */
typedef struct {
    const UA_DataType *types;
    size_t typesSize;
    const UA_String *namespaceUris;
    size_t namespaceUrisSize;
    UA_Boolean pinned;
} UA_GeneratedTypeSource;

static UA_INLINE UA_Boolean
UA_GeneratedTypes_structure(const UA_DataType *t) {
    return t->typeKind == UA_DATATYPEKIND_STRUCTURE ||
        t->typeKind == UA_DATATYPEKIND_OPTSTRUCT || t->typeKind == UA_DATATYPEKIND_UNION;
}
static UA_INLINE UA_Boolean
UA_GeneratedTypes_index(const UA_GeneratedTypeSource *s, const UA_DataType *t, size_t *i) {
    uintptr_t p = (uintptr_t)t, base = (uintptr_t)s->types;
    if(p < base || (p-base) % sizeof(UA_DataType) ||
       (p-base) / sizeof(UA_DataType) >= s->typesSize)
        return false;
    *i = (size_t)((p-base) / sizeof(UA_DataType));
    return true;
}
static UA_INLINE UA_Boolean
UA_GeneratedTypes_builtin(const UA_DataType *t) {
    uintptr_t p = (uintptr_t)t, base = (uintptr_t)UA_TYPES;
    return p >= base && (p-base) % sizeof(UA_DataType) == 0 &&
        (p-base) / sizeof(UA_DataType) < UA_TYPES_COUNT;
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
UA_GeneratedTypes_view(UA_Server *server, const UA_GeneratedTypeSource *s,
                       const UA_NodeId *id, UA_NodeId *out) {
    *out = *id; /* Borrow immutable identifier storage for lookup/validation. */
    if(UA_NodeId_isNull(id)) return UA_STATUSCODE_GOOD;
    if(id->namespaceIndex >= s->namespaceUrisSize)
        return UA_STATUSCODE_BADNODEIDINVALID;
    size_t ns = 0;
    UA_StatusCode r = UA_Server_getNamespaceByName(server, s->namespaceUris[id->namespaceIndex], &ns);
    if(!r) out->namespaceIndex = (UA_UInt16)ns;
    return r;
}
static UA_INLINE UA_StatusCode
UA_GeneratedTypes_map(UA_Server *server, const UA_GeneratedTypeSource *s,
                      const UA_NodeId *id, UA_NodeId *out) {
    UA_NodeId view;
    UA_StatusCode r = UA_GeneratedTypes_view(server, s, id, &view);
    return r ? r : UA_NodeId_copy(&view, out);
}
static UA_INLINE const UA_DataType *
UA_GeneratedTypes_find(UA_Server *server, const UA_GeneratedTypeSource *s, size_t i) {
    UA_NodeId id = UA_NODEID_NULL;
    UA_StatusCode r = UA_GeneratedTypes_view(server, s, &s->types[i].typeId, &id);
    const UA_DataType *t = r ? NULL : UA_Server_findDataType(server, &id);
    return t;
}
/* Compare the ABI and field identities, including already registered XML types.
 * Every dependency is checked separately, so recursive graphs need no recursion. */
static UA_INLINE UA_StatusCode
UA_GeneratedTypes_match(UA_Server *server, const UA_GeneratedTypeSource *s,
                        size_t size, const UA_GeneratedTypeSource *const *deps,
                        const UA_GeneratedTypeSource *owner,
                        const UA_DataType *expected, const UA_DataType *actual) {
    if(expected->memSize != actual->memSize || expected->typeKind != actual->typeKind ||
       expected->membersSize != actual->membersSize)
        return UA_STATUSCODE_BADTYPEMISMATCH;
    const UA_NodeId *ids[] = {&expected->typeId, &expected->binaryEncodingId, &expected->xmlEncodingId};
    const UA_NodeId *other[] = {&actual->typeId, &actual->binaryEncodingId, &actual->xmlEncodingId};
    for(size_t i = 0; i < 3; i++) {
        UA_NodeId id = UA_NODEID_NULL;
        UA_StatusCode r = UA_GeneratedTypes_view(server, owner, ids[i], &id);
        if(!r && !UA_NodeId_equal(&id, other[i])) r = UA_STATUSCODE_BADTYPEMISMATCH;
        if(r) return r;
    }
    for(size_t j = 0; j < expected->membersSize; j++) {
        const UA_DataTypeMember *a = &expected->members[j], *b = &actual->members[j];
        if(expected->typeKind == UA_DATATYPEKIND_ENUM) {
            if(a->memberType != b->memberType) return UA_STATUSCODE_BADTYPEMISMATCH;
            continue;
        }
        if(!UA_GeneratedTypes_structure(expected)) continue;
        if(a->padding != b->padding || a->isArray != b->isArray || a->isOptional != b->isOptional)
            return UA_STATUSCODE_BADTYPEMISMATCH;
        size_t k;
        const UA_GeneratedTypeSource *o = UA_GeneratedTypes_owner(s, size, deps, a->memberType, &k);
        if(!o && !UA_GeneratedTypes_builtin(a->memberType)) return UA_STATUSCODE_BADNOTFOUND;
        UA_NodeId id = UA_NODEID_NULL;
        UA_StatusCode r = UA_STATUSCODE_GOOD;
        if(o) r = UA_GeneratedTypes_view(server, o, &o->types[k].typeId, &id);
        else id = a->memberType->typeId;
        if(!r && (!b->memberType || !UA_NodeId_equal(&id, &b->memberType->typeId)))
            r = UA_STATUSCODE_BADTYPEMISMATCH;
        if(r) return r;
    }
    return UA_STATUSCODE_GOOD;
}
#ifdef UA_TYPES_STRUCTUREDESCRIPTION
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
    if(!r && UA_GeneratedTypes_structure(original)) {
        UA_StructureDefinition *sd = &((UA_StructureDescription*)base)->structureDefinition;
        UA_NodeId_clear(&sd->defaultEncodingId);
        r = UA_GeneratedTypes_map(server, s, &original->binaryEncodingId, &sd->defaultEncodingId);
        for(size_t m = 0; !r && m < original->membersSize; m++) {
            size_t k;
            const UA_GeneratedTypeSource *o = UA_GeneratedTypes_owner(
                s, size, deps, original->members[m].memberType, &k);
            if(!o) {
                if(!UA_GeneratedTypes_builtin(original->members[m].memberType)) r = UA_STATUSCODE_BADNOTFOUND;
                continue;
            }
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
    if(!s->typesSize) return UA_STATUSCODE_GOOD;
    if(!copy && UA_GeneratedTypes_checkPins(server, s)) return UA_STATUSCODE_BADCONFIGURATIONERROR;
    UA_StatusCode r;
    size_t present = 0;
    for(size_t i = 0; i < s->typesSize; i++) {
        for(size_t j = 0; j < i; j++)
            if(UA_NodeId_equal(&s->types[i].typeId, &s->types[j].typeId))
                return UA_STATUSCODE_BADNODEIDEXISTS;
        if(!copy && UA_GeneratedTypes_match(server, s, size, deps, s, &s->types[i], &s->types[i]))
            return UA_STATUSCODE_BADCONFIGURATIONERROR;
        const UA_DataType *actual = UA_GeneratedTypes_find(server, s, i);
        if(!actual) continue;
        r = UA_GeneratedTypes_match(server, s, size, deps, s, &s->types[i], actual);
        if(r) return r;
        if((actual == &s->types[i]) == copy) return UA_STATUSCODE_BADINVALIDSTATE;
        present++;
    }
    if(present) return present == s->typesSize ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADNODEIDEXISTS;
    for(size_t d = 0; d < size; d++) {
        if(!copy && UA_GeneratedTypes_checkPins(server, deps[d])) return UA_STATUSCODE_BADCONFIGURATIONERROR;
        for(size_t i = 0; i < deps[d]->typesSize; i++) {
            const UA_DataType *actual = UA_GeneratedTypes_find(server, deps[d], i);
            if(!actual) continue;
            r = UA_GeneratedTypes_match(server, s, size, deps, deps[d], &deps[d]->types[i], actual);
            if(r) return r;
            if(!copy && actual != &deps[d]->types[i]) return UA_STATUSCODE_BADCONFIGURATIONERROR;
        }
    }
    UA_ServerConfig *config = UA_Server_getConfig(server);
    if(!copy) {
        /* A borrowed entry is followed by an empty owning entry. The latter
         * frees their common allocation after the borrowed entry was visited;
         * the read-only datatype array itself is never freed. */
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
#ifndef UA_TYPES_STRUCTUREDESCRIPTION
    return UA_STATUSCODE_BADNOTSUPPORTED;
#else
    UA_DataTypeArray *array = (UA_DataTypeArray*)UA_calloc(1, sizeof(UA_DataTypeArray));
    UA_DataType *types = (UA_DataType*)UA_calloc(s->typesSize, sizeof(UA_DataType));
    UA_Boolean *ready = (UA_Boolean*)UA_calloc(s->typesSize, sizeof(UA_Boolean));
    r = UA_STATUSCODE_BADOUTOFMEMORY;
    if(!array || !types || !ready) goto cleanup;
    array->types = types;
    array->typesSize = s->typesSize;
    array->cleanup = true;
    array->next = (UA_DataTypeArray*)(uintptr_t)UA_Server_getDataTypes(server);
    for(size_t i = 0; i < s->typesSize; i++) {
        r = UA_GeneratedTypes_map(server, s, &s->types[i].typeId, &types[i].typeId);
        if(r) goto cleanup;
        types[i].typeKind = s->types[i].typeKind;
        types[i].memSize = s->types[i].memSize;
        types[i].pointerFree = s->types[i].pointerFree;
    }
    /* Stable slots support self references and indirect recursion. Inline
     * members wait for their complete layout. Nothing is published on error. */
    for(size_t remaining = s->typesSize; remaining;) {
        size_t before = remaining;
        for(size_t i = 0; i < s->typesSize; i++) {
            if(ready[i]) continue;
            UA_Boolean wait = false;
            if(UA_GeneratedTypes_structure(&s->types[i])) {
                for(size_t j = 0; j < s->types[i].membersSize; j++) {
                    const UA_DataTypeMember *m = &s->types[i].members[j];
                    size_t k;
                    if(!m->isArray && !m->isOptional &&
                       UA_GeneratedTypes_index(s, m->memberType, &k) && !ready[k]) wait = true;
                }
            }
            if(wait) continue;
            UA_NodeId_clear(&types[i].typeId);
            r = UA_GeneratedTypes_copy(server, s, size, deps, i, array, &types[i]);
            if(!r) r = UA_GeneratedTypes_match(server, s, size, deps, s, &s->types[i], &types[i]);
            if(r) goto cleanup;
            ready[i] = true;
            remaining--;
        }
        if(remaining == before) { r = UA_STATUSCODE_BADNOTSUPPORTED; goto cleanup; }
    }
    array->next = config->customDataTypes;
    config->customDataTypes = array;
    UA_free(ready);
    return UA_STATUSCODE_GOOD;
cleanup:
    if(types) {
        for(size_t i = 0; i < s->typesSize; i++) UA_DataType_clear(&types[i]);
        UA_free(types);
    }
    UA_free(array);
    UA_free(ready);
    return r;
#endif
}
#endif /* UA_GENERATED_TYPES_SUPPORT_H_ */
'''
