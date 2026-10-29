/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "companion_test.h"
#include <open62541/plugin/log_stdout.h>
#include <open62541/server_config_default.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static UA_Server *newServer(unsigned shift) {
    UA_ServerConfig config;
    memset(&config, 0, sizeof(config));
    config.logging = UA_Log_Stdout_new(UA_LOGLEVEL_ERROR);
    if(UA_ServerConfig_setMinimal(&config, 0, NULL) != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clear(&config);
        return NULL;
    }
    UA_Server *server = UA_Server_newWithConfig(&config);
    if(!server)
        return NULL;
    for(unsigned i = 0; i < shift; i++) {
        char uri[64];
        snprintf(uri, sizeof(uri), "urn:companion-test:unrelated:%u", i);
        UA_Server_addNamespace(server, uri);
    }
    return server;
}
static const CompanionModule *findModule(size_t index) {
    for(size_t i = 0; i < companionModulesSize; i++)
        if(companionModules[i]->index == index)
            return companionModules[i];
    return NULL;
}
static UA_StatusCode loadXml(UA_Server *server, const char *path) {
    char full[2048];
    snprintf(full, sizeof(full), "%s%s", COMPANION_NODESET_DIR, path);
    FILE *f = fopen(full, "rb");
    if(!f)
        return UA_STATUSCODE_BADNOTFOUND;
    if(fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    long len = ftell(f);
    if(len < 0 || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_ByteString xml = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_ByteString_allocBuffer(&xml, (size_t)len);
    if(!res && fread(xml.data, 1, xml.length, f) != xml.length)
        res = UA_STATUSCODE_BADINTERNALERROR;
    fclose(f);
    if(!res)
        res = UA_Server_loadNodeset(server, xml);
    UA_ByteString_clear(&xml);
    return res;
}
static UA_StatusCode loadModule(const CompanionModule *module, UA_Server *server,
                                UA_Boolean compiled, UA_Boolean copy, UA_Boolean *loaded) {
    if(loaded[module->index])
        return UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < module->dependenciesSize; i++) {
        const CompanionModule *dep = findModule(module->dependencies[i]);
        if(!dep)
            return UA_STATUSCODE_BADNOTFOUND;
        UA_StatusCode res = loadModule(dep, server, compiled, copy, loaded);
        if(res)
            return res;
    }
    UA_StatusCode res = compiled ? module->load(server, copy) : loadXml(server, module->path);
    if(res)
        fprintf(stderr, "%s %s: %s\n", compiled ? "COMPILED" : "XML", module->path,
                UA_StatusCode_name(res));
    else
        loaded[module->index] = true;
    return res;
}
static UA_StatusCode nodeId(UA_Server *server, const CompanionNode *node, UA_NodeId *id) {
    UA_StatusCode res = UA_NodeId_parse(id, UA_STRING((char *)(uintptr_t)node->identifier));
    size_t ns = 0;
    if(!res)
        res = UA_Server_getNamespaceByName(server, UA_STRING((char *)(uintptr_t)node->uri), &ns);
    if(!res)
        id->namespaceIndex = (UA_UInt16)ns;
    return res;
}
static UA_StatusCode mappings(UA_Server *a, UA_Server *b, UA_NamespaceMapping *ma,
                              UA_NamespaceMapping *mb) {
    memset(ma, 0, sizeof(*ma));
    memset(mb, 0, sizeof(*mb));
    UA_String *uris = NULL;
    size_t count = 0;
    for(unsigned which = 0; which < 2; which++) {
        UA_Server *server = which ? b : a;
        UA_NamespaceMapping *map = which ? mb : ma;
        for(size_t ns = 0;; ns++) {
            UA_String uri = UA_STRING_NULL;
            if(UA_Server_getNamespaceByIndex(server, ns, &uri) != UA_STATUSCODE_GOOD)
                break;
            size_t index = 0;
            while(index < count && !UA_String_equal(&uri, &uris[index]))
                index++;
            if(index == count) {
                UA_String *next = (UA_String *)realloc(uris, (count + 1) * sizeof(UA_String));
                if(!next) {
                    UA_String_clear(&uri);
                    goto error;
                }
                uris = next;
                uris[count++] = uri;
            } else
                UA_String_clear(&uri);
            UA_UInt16 *next = (UA_UInt16 *)realloc(map->local2remote, (ns + 1) * sizeof(UA_UInt16));
            if(!next)
                goto error;
            map->local2remote = next;
            map->local2remote[ns] = (UA_UInt16)index;
            map->local2remoteSize = ns + 1;
        }
    }
    ma->namespaceUris = uris;
    ma->namespaceUrisSize = count;
    mb->namespaceUris = uris;
    mb->namespaceUrisSize = count;
    return UA_STATUSCODE_GOOD;
error:
    for(size_t i = 0; i < count; i++)
        UA_String_clear(&uris[i]);
    free(uris);
    free(ma->local2remote);
    free(mb->local2remote);
    memset(ma, 0, sizeof(*ma));
    memset(mb, 0, sizeof(*mb));
    return UA_STATUSCODE_BADOUTOFMEMORY;
}
static UA_StatusCode referencePresence(UA_Server *server, UA_NodeId id, const CompanionNode *node,
                                       UA_Boolean *present) {
    UA_BrowseDescription description;
    UA_BrowseDescription_init(&description);
    description.nodeId = id;
    description.browseDirection = UA_BROWSEDIRECTION_BOTH;
    description.resultMask = UA_BROWSERESULTMASK_REFERENCETYPEID | UA_BROWSERESULTMASK_ISFORWARD;
    UA_BrowseResult result = UA_Server_browse(server, 0, &description);
    UA_StatusCode status = UA_STATUSCODE_GOOD;
    for(;;) {
        if(result.statusCode != UA_STATUSCODE_GOOD) {
            status = result.statusCode;
            break;
        }
        for(size_t r = 0; r < node->referencesSize; r++) {
            const CompanionReference *expected = &node->references[r];
            CompanionNode type = {expected->typeUri, expected->typeIdentifier, 0, NULL, 0};
            CompanionNode target = {expected->targetUri, expected->targetIdentifier, 0, NULL, 0};
            UA_NodeId typeId = UA_NODEID_NULL, targetId = UA_NODEID_NULL;
            status = nodeId(server, &type, &typeId);
            if(!status)
                status = nodeId(server, &target, &targetId);
            for(size_t j = 0; !status && j < result.referencesSize; j++) {
                const UA_ReferenceDescription *actual = &result.references[j];
                if(actual->isForward == expected->forward &&
                   UA_NodeId_equal(&actual->referenceTypeId, &typeId) &&
                   UA_NodeId_equal(&actual->nodeId.nodeId, &targetId) &&
                   actual->nodeId.serverIndex == 0 && actual->nodeId.namespaceUri.length == 0)
                    present[r] = true;
            }
            UA_NodeId_clear(&typeId);
            UA_NodeId_clear(&targetId);
            if(status)
                break;
        }
        if(status || result.continuationPoint.length == 0)
            break;
        UA_BrowseResult next = UA_Server_browseNext(server, false, &result.continuationPoint);
        UA_BrowseResult_clear(&result);
        result = next;
    }
    UA_BrowseResult_clear(&result);
    return status;
}

static int compareReferences(UA_Server *a, UA_Server *b, UA_NodeId ia, UA_NodeId ib,
                             const CompanionNode *node, const char *path, size_t *absent) {
    if(node->referencesSize == 0)
        return 0;
    UA_Boolean *pa = (UA_Boolean *)calloc(node->referencesSize, sizeof(UA_Boolean));
    UA_Boolean *pb = (UA_Boolean *)calloc(node->referencesSize, sizeof(UA_Boolean));
    int failed = 1;
    if(!pa || !pb || referencePresence(a, ia, node, pa) || referencePresence(b, ib, node, pb))
        goto cleanup;
    failed = 0;
    for(size_t r = 0; r < node->referencesSize; r++) {
        if(pa[r] != pb[r]) {
            fprintf(stderr, "Reference mismatch: %s %s -> %s (%u/%u)\n", path, node->identifier,
                    node->references[r].targetIdentifier, (unsigned)pa[r], (unsigned)pb[r]);
            failed = 1;
            break;
        }
        if(!pa[r])
            (*absent)++;
    }
cleanup:
    free(pa);
    free(pb);
    return failed;
}

static int compareLayout(UA_Server *a, UA_Server *b, UA_NodeId ia, UA_NodeId ib, const char *path,
                         const char *identifier) {
    const UA_DataType *ta = UA_Server_findDataType(a, &ia);
    const UA_DataType *tb = UA_Server_findDataType(b, &ib);
    if(!ta && !tb)
        return 0;
    if(!ta || !tb || ta->typeKind != tb->typeKind || ta->memSize != tb->memSize ||
       ta->membersSize != tb->membersSize || ta->pointerFree != tb->pointerFree)
        goto failed;
    if(ia.namespaceIndex != 0 && ta == tb)
        goto failed; /* Custom layouts belong to distinct live servers. */
#ifdef UA_ENABLE_TYPEDESCRIPTION
    if(!ta->typeName || !tb->typeName || strcmp(ta->typeName, tb->typeName))
        goto failed;
#endif
    if(ta->typeKind == UA_DATATYPEKIND_ENUM)
        return 0; /* The Definition attribute compares enum names and values. */
    for(size_t m = 0; m < ta->membersSize; m++) {
        if(ta->members[m].padding != tb->members[m].padding ||
           ta->members[m].isArray != tb->members[m].isArray ||
           ta->members[m].isOptional != tb->members[m].isOptional)
            goto failed;
    }
    return 0;
failed:
    fprintf(stderr, "Native layout mismatch: %s %s\n", path, identifier);
    return 1;
}

static int compareRegisteredTypes(UA_Server *a, UA_Server *b, const CompanionModule *module) {
    for(size_t t = 0; t < module->typesSize; t++) {
        const UA_DataType *type = UA_Server_getGeneratedDataType(a, module->staticType(t));
        if(!type) {
            fprintf(stderr, "Generated type %zu was not registered: %s\n", t, module->path);
            return 1;
        }
        UA_NodeId otherId = UA_NODEID_NULL;
        UA_String uri = UA_STRING_NULL;
        size_t ns = 0;
        UA_StatusCode status = UA_NodeId_copy(&type->typeId, &otherId);
        if(!status)
            status = UA_Server_getNamespaceByIndex(a, type->typeId.namespaceIndex, &uri);
        if(!status)
            status = UA_Server_getNamespaceByName(b, uri, &ns);
        otherId.namespaceIndex = (UA_UInt16)ns;
        const char *name = "generated descriptor";
#ifdef UA_ENABLE_TYPEDESCRIPTION
        name = type->typeName;
#endif
        int failed = status != UA_STATUSCODE_GOOD ||
                     compareLayout(a, b, type->typeId, otherId, module->path, name);
        UA_NodeId_clear(&otherId);
        UA_String_clear(&uri);
        if(failed)
            return 1;
    }
    return 0;
}

static int compare(UA_Server *a, UA_Server *b, const CompanionModule *module,
                   UA_NamespaceMapping *ma, UA_NamespaceMapping *mb) {
    if(compareRegisteredTypes(a, b, module))
        return 1;
    size_t absent = 0, absentReferences = 0;
    for(size_t n = 0; n < module->nodesSize; n++) {
        const CompanionNode *node = &module->nodes[n];
        UA_NodeId ia = UA_NODEID_NULL, ib = UA_NODEID_NULL;
        UA_StatusCode ra = nodeId(a, node, &ia), rb = nodeId(b, node, &ib);
        if(ra != rb || ra) {
            UA_NodeId_clear(&ia);
            UA_NodeId_clear(&ib);
            fprintf(stderr, "Namespace missing: %s %s\n", module->path, node->uri);
            return 1;
        }
        UA_NodeClass ca, cb;
        ra = UA_Server_readNodeClass(a, ia, &ca);
        rb = UA_Server_readNodeClass(b, ib, &cb);
        if(ra != rb) {
            fprintf(stderr, "Node availability: %s %s (%s / %s)\n", module->path, node->identifier,
                    UA_StatusCode_name(ra), UA_StatusCode_name(rb));
            goto failed;
        }
        if(ra) {
            absent++;
            UA_NodeId_clear(&ia);
            UA_NodeId_clear(&ib);
            continue;
        }
        if(ca != cb)
            goto failed;
        if(compareReferences(a, b, ia, ib, node, module->path, &absentReferences) ||
           (ca == UA_NODECLASS_DATATYPE && (node->attributes & (1u << 23)) &&
            compareLayout(a, b, ia, ib, module->path, node->identifier)))
            goto failed;
        for(UA_UInt32 attr = 1; attr < 32; attr++) {
            if(!(node->attributes & (1u << attr)))
                continue;
            UA_ReadValueId read;
            UA_ReadValueId_init(&read);
            read.attributeId = attr;
            read.nodeId = ia;
            UA_DataValue va = UA_Server_read(a, &read, UA_TIMESTAMPSTORETURN_NEITHER);
            read.nodeId = ib;
            UA_DataValue vb = UA_Server_read(b, &read, UA_TIMESTAMPSTORETURN_NEITHER);
            UA_ByteString ba = UA_BYTESTRING_NULL, bb = UA_BYTESTRING_NULL;
            UA_EncodeBinaryOptions oa, ob;
            memset(&oa, 0, sizeof(oa));
            memset(&ob, 0, sizeof(ob));
            oa.namespaceMapping = ma;
            ob.namespaceMapping = mb;
            ra = UA_encodeBinary(&va.value, &UA_TYPES[UA_TYPES_VARIANT], &ba, &oa);
            rb = UA_encodeBinary(&vb.value, &UA_TYPES[UA_TYPES_VARIANT], &bb, &ob);
            int different =
                va.status != vb.status || ra || rb || (!ra && !UA_ByteString_equal(&ba, &bb));
            if(different) {
                fprintf(stderr, "Attribute mismatch: %s %s attr %u (%s/%s, %zu/%zu bytes)\n",
                        module->path, node->identifier, (unsigned)attr,
                        UA_StatusCode_name(va.status), UA_StatusCode_name(vb.status), ba.length,
                        bb.length);
                UA_String pa = UA_STRING_NULL, pb = UA_STRING_NULL;
                (void)UA_print(&va.value, &UA_TYPES[UA_TYPES_VARIANT], &pa);
                (void)UA_print(&vb.value, &UA_TYPES[UA_TYPES_VARIANT], &pb);
                fprintf(stderr, "COMPILED: %.*s\nREFERENCE: %.*s\n",
                        (int)(pa.length < 500 ? pa.length : 500), (char *)pa.data,
                        (int)(pb.length < 500 ? pb.length : 500), (char *)pb.data);
                UA_String_clear(&pa);
                UA_String_clear(&pb);
            }
            UA_ByteString_clear(&ba);
            UA_ByteString_clear(&bb);
            UA_DataValue_clear(&va);
            UA_DataValue_clear(&vb);
            if(different)
                goto failed;
        }
        UA_NodeId_clear(&ia);
        UA_NodeId_clear(&ib);
        continue;
    failed:
        UA_NodeId_clear(&ia);
        UA_NodeId_clear(&ib);
        return 1;
    }
    fprintf(stderr,
            "CHECK %s: %zu source nodes; %zu nodes and %zu references absent in both loaders\n",
            module->path, module->nodesSize, absent, absentReferences);
    return 0;
}
int companionRun(size_t selected, UA_Boolean copyTypes) {
    if(selected != (size_t)-1 && !findModule(selected)) {
        fprintf(stderr, "Unknown companion case: %zu\n", selected);
        return 1;
    }
    size_t capacity = 0;
    for(size_t i = 0; i < companionModulesSize; i++)
        if(companionModules[i]->index >= capacity)
            capacity = companionModules[i]->index + 1;
    UA_Boolean *loaded = (UA_Boolean *)calloc(capacity, sizeof(UA_Boolean));
    UA_Boolean *referenceLoaded = (UA_Boolean *)calloc(capacity, sizeof(UA_Boolean));
    UA_Boolean *secondLoaded = (UA_Boolean *)calloc(capacity, sizeof(UA_Boolean));
    UA_Server *server = newServer(0), *reference = newServer(0),
              *second = copyTypes ? newServer(3) : NULL;
    int failed = 1;
    UA_NamespaceMapping ma, mb;
    memset(&ma, 0, sizeof(ma));
    memset(&mb, 0, sizeof(mb));
    if(!loaded || !referenceLoaded || !secondLoaded || !server || !reference ||
       (copyTypes && !second))
        goto cleanup;
    for(size_t i = 0; i < companionModulesSize; i++) {
        const CompanionModule *m = companionModules[i];
        if(selected != (size_t)-1 ? m->index != selected : !m->preferred)
            continue;
        if(loadModule(m, server, true, copyTypes, loaded) ||
           loadModule(m, reference, false, true, referenceLoaded))
            goto cleanup;
    }
    if(mappings(server, reference, &ma, &mb))
        goto cleanup;
    for(size_t i = 0; i < companionModulesSize; i++)
        if(loaded[companionModules[i]->index] &&
           compare(server, reference, companionModules[i], &ma, &mb))
            goto cleanup;
    for(size_t i = 0; i < ma.namespaceUrisSize; i++)
        UA_String_clear(&ma.namespaceUris[i]);
    free(ma.namespaceUris);
    free(ma.local2remote);
    free(mb.local2remote);
    memset(&ma, 0, sizeof(ma));
    memset(&mb, 0, sizeof(mb));
    if(copyTypes) {
        for(size_t i = companionModulesSize; i > 0; i--) {
            const CompanionModule *m = companionModules[i - 1];
            if(loaded[m->index] && loadModule(m, second, true, true, secondLoaded))
                goto cleanup;
        }
        if(mappings(server, second, &ma, &mb))
            goto cleanup;
        for(size_t i = 0; i < companionModulesSize; i++)
            if(loaded[companionModules[i]->index] &&
               compare(server, second, companionModules[i], &ma, &mb))
                goto cleanup;
    }
    failed = 0;
cleanup:
    for(size_t i = 0; i < ma.namespaceUrisSize; i++)
        UA_String_clear(&ma.namespaceUris[i]);
    free(ma.namespaceUris);
    free(ma.local2remote);
    free(mb.local2remote);
    UA_Server_delete(server);
    UA_Server_delete(reference);
    UA_Server_delete(second);
    free(loaded);
    free(referenceLoaded);
    free(secondLoaded);
    return failed;
}
