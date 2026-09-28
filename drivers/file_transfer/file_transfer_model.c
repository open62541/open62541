/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include "file_transfer_internal.h"
#include <open62541/plugin/nodestore.h>

static UA_StatusCode
getChildId(UA_Server *server, const UA_NodeId parent, const char *name,
            UA_NodeId *out) {
    UA_QualifiedName qn = UA_QUALIFIEDNAME(0, (char*)(uintptr_t)name);
    UA_BrowsePathResult bpr =
        UA_Server_browseSimplifiedBrowsePath(server, parent, 1, &qn);
    if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
        UA_BrowsePathResult_clear(&bpr);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_StatusCode res = UA_NodeId_copy(&bpr.targets[0].targetId.nodeId, out);
    UA_BrowsePathResult_clear(&bpr);
    return res;
}

FileTransferDriver *
findEntryOwner(UA_Server *server, const UA_NodeId *objectId) {
    for(UA_Driver *drv = UA_Server_getDrivers(server); drv; drv = drv->next) {
        if(isFileTransferDriver(drv) &&
           findFTEntry((FileTransferDriver*)drv, objectId))
            return (FileTransferDriver*)drv;
    }
    return NULL;
}

FTEntry *
resolveFTEntry(UA_Server *server, const UA_NodeId *objectId, void *objectContext) {
    /* Shared type Methods can be called on unmanaged Objects with arbitrary
     * contexts. Compare pointers before inspecting any context memory. */
    for(UA_Driver *drv = UA_Server_getDrivers(server); drv; drv = drv->next) {
        if(drv == objectContext && isFileTransferDriver(drv) &&
           drv->state == UA_LIFECYCLESTATE_STARTED)
            return findFTEntry((FileTransferDriver*)drv, objectId);
    }
    return NULL;
}

FTEntry *
resolveFTEntryById(UA_Server *server, const UA_NodeId *objectId) {
    void *context = NULL;
    if(UA_Server_getNodeContext(server, *objectId, &context) != UA_STATUSCODE_GOOD)
        return NULL;
    return resolveFTEntry(server, objectId, context);
}

FTEntry *
findFTEntry(FileTransferDriver *ftd, const UA_NodeId *nodeId) {
    return ZIP_FIND(FTEntriesById, &ftd->entriesByNodeId, nodeId);
}

static void
removeFTEntry(FileTransferDriver *ftd, FTEntry *node) {
    UA_assert(!ZIP_ROOT(&node->children));
    UA_assert(node->subtreeHandleCount == 0);
    if(node->parent)
        ZIP_REMOVE(FTChildrenByName, &node->parent->children, node);
    ZIP_REMOVE(FTEntriesById, &ftd->entriesByNodeId, node);
    ftd->entryCount--;
    if(node->contextBound) {
        void *context = NULL;
        UA_Server *server = ftd->driver.server;
        if(UA_Server_getNodeContext(server, node->nodeId, &context) == UA_STATUSCODE_GOOD &&
           context == &ftd->driver)
            UA_Server_setNodeContext(server, node->nodeId, node->savedObjectContext);
    }
    releaseFileNode(ftd->driver.server, node);
    if(ftd->root == node)
        ftd->root = NULL;
    UA_NodeId_clear(&node->nodeId);
    UA_String_clear(&node->path);
    UA_free(node);
}

#define UA_FTMETHODS_SIZE(methods) (sizeof(methods) / sizeof(methods[0]))

/* The callbacks are attached to the Namespace Zero type declarations. With the
 * default configuration (copyMethodsOnInstances false) an Object instance
 * references the type's Method nodes instead of copying them, so one
 * registration serves every FileType/FileDirectoryType instance. The flip side
 * is that this is server-global state: it claims the Part 20 Methods for the
 * drivers, and is released when the last active instance stops. */
static const FTMethod *
fileTransferMethod(size_t index) {
    if(index < UA_FTMETHODS_SIZE(fileTypeMethods))
        return &fileTypeMethods[index];
    return &fileDirectoryTypeMethods[index - UA_FTMETHODS_SIZE(fileTypeMethods)];
}

static UA_StatusCode
setFileTransferMethodCallbacks(UA_Server *server, UA_Boolean install) {
    const size_t count = UA_FTMETHODS_SIZE(fileTypeMethods) +
        UA_FTMETHODS_SIZE(fileDirectoryTypeMethods);
    UA_MethodCallback previous[UA_FTMETHODS_SIZE(fileTypeMethods) +
                               UA_FTMETHODS_SIZE(fileDirectoryTypeMethods)];
    size_t changed = 0;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < count; i++) {
        const FTMethod *method = fileTransferMethod(i);
        UA_NodeId id = UA_NODEID_NUMERIC(0, method->methodId);
        UA_MethodCallback current = NULL;
        res = UA_Server_getMethodNodeCallback(server, id, &current);
        if(res != UA_STATUSCODE_GOOD)
            break;
        previous[i] = current;
        /* Never remove a callback replaced by the application. */
        if(install || current == method->callback)
            res = UA_Server_setMethodNodeCallback(server, id,
                                                    install ? method->callback : NULL);
        if(res != UA_STATUSCODE_GOOD)
            break;
        changed++;
    }
    if(install && res != UA_STATUSCODE_GOOD) {
        for(size_t i = 0; i < changed; i++)
            UA_Server_setMethodNodeCallback(server,
                UA_NODEID_NUMERIC(0, fileTransferMethod(i)->methodId), previous[i]);
    }
    return res;
}

UA_StatusCode
registerFileTransferMethodCallbacks(UA_Server *server) {
    return setFileTransferMethodCallbacks(server, true);
}

void
unregisterFileTransferMethodCallbacks(UA_Server *server) {
    setFileTransferMethodCallbacks(server, false);
}

static const FTMethod *
entryMethods(const FTEntry *node, size_t *size) {
    if(node->isDirectory) {
        *size = UA_FTMETHODS_SIZE(fileDirectoryTypeMethods);
        return fileDirectoryTypeMethods;
    }
    *size = UA_FTMETHODS_SIZE(fileTypeMethods);
    return fileTypeMethods;
}

/* Subtype Method declarations can be shared across Objects and drivers.
 * Bound ids are cached, so releasing a binding needs no repeated browsing. */
static void *
entryUsesMethod(void *context, FTEntry *node) {
    if(!node->methods)
        return NULL;
    const UA_NodeId *methodId = (const UA_NodeId*)context;
    size_t count;
    entryMethods(node, &count);
    for(size_t i = 0; i < count; i++) {
        if(UA_NodeId_equal(&node->methods[i], methodId))
            return node;
    }
    return NULL;
}

static UA_Boolean
methodInUse(UA_Server *server, const UA_NodeId *methodId) {
    for(UA_Driver *drv = UA_Server_getDrivers(server); drv; drv = drv->next) {
        if(isFileTransferDriver(drv) &&
           ZIP_ITER(FTEntriesById, &((FileTransferDriver*)drv)->entriesByNodeId,
                    entryUsesMethod, (void*)(uintptr_t)methodId))
            return true;
    }
    return false;
}

UA_StatusCode
bindObjectContext(UA_Server *server, FTEntry *node) {
    if(node->contextBound)
        return UA_STATUSCODE_GOOD;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(!node->created)
        res = UA_Server_getNodeContext(server, node->nodeId, &node->savedObjectContext);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_Server_setNodeContext(server, node->nodeId, &node->driver->driver);
    if(res == UA_STATUSCODE_GOOD)
        node->contextBound = true;
    return res;
}

UA_StatusCode
bindObjectMethods(UA_Server *server, FTEntry *node) {
    if(node->driver->driver.state != UA_LIFECYCLESTATE_STARTED || node->methodsBound)
        return UA_STATUSCODE_GOOD;
    node->methodsBound = true;
    size_t count;
    const FTMethod *methods = entryMethods(node, &count);
    for(size_t i = 0; i < count; i++) {
        UA_NodeId id = UA_NODEID_NULL;
        UA_StatusCode res = getChildId(server, node->nodeId, methods[i].name, &id);
        if(res == UA_STATUSCODE_BADNOTFOUND)
            continue;
        UA_NodeId typeId = UA_NODEID_NUMERIC(0, methods[i].methodId);
        if(res == UA_STATUSCODE_GOOD && !UA_NodeId_equal(&id, &typeId)) {
            /* Standard declarations are bound once for all drivers. Allocate
             * per-Object storage only for copied or subtype Methods. */
            if(!node->methods)
                node->methods = (UA_NodeId*)UA_calloc(count, sizeof(UA_NodeId));
            if(!node->methods) {
                res = UA_STATUSCODE_BADOUTOFMEMORY;
            } else {
                node->methods[i] = id;
                id = UA_NODEID_NULL;
                res = UA_Server_setMethodNodeCallback(server, node->methods[i],
                                                       methods[i].callback);
            }
        }
        UA_NodeId_clear(&id);
        if(res != UA_STATUSCODE_GOOD) {
            unbindObjectMethods(server, node);
            return res;
        }
    }
    return UA_STATUSCODE_GOOD;
}

void
unbindObjectMethods(UA_Server *server, FTEntry *node) {
    node->methodsBound = false;
    if(!node->methods)
        return;
    UA_NodeId *ids = node->methods;
    node->methods = NULL;
    size_t count;
    const FTMethod *methods = entryMethods(node, &count);
    for(size_t i = 0; i < count; i++) {
        if(!UA_NodeId_isNull(&ids[i]) && !methodInUse(server, &ids[i])) {
            UA_MethodCallback current = NULL;
            if(UA_Server_getMethodNodeCallback(server, ids[i], &current) == UA_STATUSCODE_GOOD &&
               current == methods[i].callback)
                UA_Server_setMethodNodeCallback(server, ids[i], NULL);
        }
        UA_NodeId_clear(&ids[i]);
    }
    UA_free(ids);
}

FTEntry *
newFTEntry(FileTransferDriver *ftd, FTEntry *parent, const UA_NodeId nodeId,
          const UA_String path, UA_Boolean isDirectory) {
    FTEntry *node = (FTEntry*)UA_calloc(1, sizeof(FTEntry));
    if(!node)
        return NULL;
    if(UA_NodeId_copy(&nodeId, &node->nodeId) != UA_STATUSCODE_GOOD ||
       UA_String_copy(&path, &node->path) != UA_STATUSCODE_GOOD) {
        UA_NodeId_clear(&node->nodeId);
        UA_String_clear(&node->path);
        UA_free(node);
        return NULL;
    }
    node->driver = ftd;
    node->isDirectory = isDirectory;
    node->parent = parent;
    node->name = pathLastSegment(node->path);
    ZIP_INIT(&node->children);
    if(parent)
        ZIP_INSERT(FTChildrenByName, &parent->children, node);
    ZIP_INSERT(FTEntriesById, &ftd->entriesByNodeId, node);
    ftd->entryCount++;
    return node;
}

/* Both ObjectType and DataType validation follow inverse HasSubtype links. */
static UA_StatusCode
hasSupertype(UA_Server *server, const UA_NodeId type, const UA_NodeId base,
             UA_NodeClass nodeClass, UA_Boolean *found) {
    *found = UA_NodeId_equal(&type, &base);
    if(*found)
        return UA_STATUSCODE_GOOD;
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = type;
    bd.browseDirection = UA_BROWSEDIRECTION_INVERSE;
    bd.referenceTypeId = UA_NS0ID(HASSUBTYPE);
    bd.nodeClassMask = nodeClass;
    size_t count = 0;
    UA_ExpandedNodeId *types = NULL;
    UA_StatusCode res = UA_Server_browseRecursive(server, &bd, &count, &types);
    for(size_t i = 0; i < count; i++) {
        if(UA_ExpandedNodeId_isLocal(&types[i]) &&
           UA_NodeId_equal(&types[i].nodeId, &base)) {
            *found = true;
            break;
        }
    }
    UA_Array_delete(types, count, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
    return res;
}

UA_StatusCode
checkFileTransferType(UA_Server *server, const UA_NodeId typeDefinition,
                      UA_Boolean isDirectory) {
    UA_NodeId base = isDirectory ? UA_NS0ID(FILEDIRECTORYTYPE) : UA_NS0ID(FILETYPE);
    UA_Boolean found;
    UA_StatusCode res = hasSupertype(server, typeDefinition, base,
                                      UA_NODECLASS_OBJECTTYPE, &found);
    return (res == UA_STATUSCODE_GOOD && !found) ?
        UA_STATUSCODE_BADTYPEDEFINITIONINVALID : res;
}

UA_StatusCode
checkFileTransferObject(UA_Server *server, const UA_NodeId nodeId,
                        UA_Boolean isDirectory) {
    UA_NodeClass cls;
    UA_StatusCode res = UA_Server_readNodeClass(server, nodeId, &cls);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(cls != UA_NODECLASS_OBJECT)
        return UA_STATUSCODE_BADNODECLASSINVALID;
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = nodeId;
    bd.referenceTypeId = UA_NS0ID(HASTYPEDEFINITION);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
    res = br.statusCode;
    if(res == UA_STATUSCODE_GOOD) {
        res = UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
        if(br.referencesSize == 1 && UA_ExpandedNodeId_isLocal(&br.references[0].nodeId))
            res = checkFileTransferType(server, br.references[0].nodeId.nodeId, isDirectory);
    }
    UA_BrowseResult_clear(&br);
    return res;
}

static UA_StatusCode
readScalar(const void *scalar, const UA_DataType *type,
           UA_Boolean includeSourceTimeStamp, UA_DataValue *value) {
    UA_StatusCode res = UA_Variant_setScalarCopy(&value->value, scalar, type);
    value->hasValue = (res == UA_STATUSCODE_GOOD);
    if(includeSourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
    return res;
}

typedef enum {
    FT_FILEINFO_SIZE,
    FT_FILEINFO_LASTMODIFIED
} FTFileInfoField;

/* The Size, Writable and LastModifiedTime Properties are computed from the
 * backend on demand. They stay correct when the file changes behind the
 * server. Writable is the storage writability within the mount configuration
 * (Part 20, 4.2.1). */
static UA_StatusCode
readFileInfo(void *nodeContext, UA_Boolean includeSourceTimeStamp,
             FTFileInfoField field, UA_DataValue *value) {
    FTEntry *node = (FTEntry*)nodeContext;
    UA_FileTransferFileBackend *b = &node->driver->backend.file;
    UA_FileTransferFileInfo info;
    UA_StatusCode res = backendGetInfo(b, node->path, &info);
    if(res != UA_STATUSCODE_GOOD) {
        value->hasStatus = true;
        value->status = res;
        return UA_STATUSCODE_GOOD;
    }
    if(field == FT_FILEINFO_SIZE)
        return readScalar(&info.size, &UA_TYPES[UA_TYPES_UINT64], includeSourceTimeStamp, value);
    return readScalar(&info.lastModified, &UA_TYPES[UA_TYPES_DATETIME],
                       includeSourceTimeStamp, value);
}

static UA_StatusCode
readSizeCallback(UA_Server *server, const UA_NodeId *sessionId,
                 void *sessionContext, const UA_NodeId *nodeId,
                 void *nodeContext, UA_Boolean includeSourceTimeStamp,
                 const UA_NumericRange *range, UA_DataValue *value) {
    return readFileInfo(nodeContext, includeSourceTimeStamp, FT_FILEINFO_SIZE,
                        value);
}

static UA_StatusCode
readLastModifiedCallback(UA_Server *server, const UA_NodeId *sessionId,
                         void *sessionContext, const UA_NodeId *nodeId,
                         void *nodeContext, UA_Boolean includeSourceTimeStamp,
                         const UA_NumericRange *range, UA_DataValue *value) {
    return readFileInfo(nodeContext, includeSourceTimeStamp,
                        FT_FILEINFO_LASTMODIFIED, value);
}

/* These Properties read the authoritative state, including while stopped.
 * Open/Close and configuration changes need no separate Property writes. */
static UA_StatusCode
readOpenCountCallback(UA_Server *server, const UA_NodeId *sessionId,
                      void *sessionContext, const UA_NodeId *nodeId,
                      void *nodeContext, UA_Boolean includeSourceTimeStamp,
                      const UA_NumericRange *range, UA_DataValue *value) {
    if(range)
        return UA_STATUSCODE_BADINDEXRANGENODATA;
    FTEntry *node = (FTEntry*)nodeContext;
    UA_UInt16 count = (UA_UInt16)node->subtreeHandleCount;
    return readScalar(&count, &UA_TYPES[UA_TYPES_UINT16], includeSourceTimeStamp, value);
}

static UA_StatusCode
readMaxLengthCallback(UA_Server *server, const UA_NodeId *sessionId,
                      void *sessionContext, const UA_NodeId *nodeId,
                      void *nodeContext, UA_Boolean includeSourceTimeStamp,
                      const UA_NumericRange *range, UA_DataValue *value) {
    if(range)
        return UA_STATUSCODE_BADINDEXRANGENODATA;
    FTEntry *node = (FTEntry*)nodeContext;
    return readScalar(&node->driver->config.maxReadLength, &UA_TYPES[UA_TYPES_UINT32],
                       includeSourceTimeStamp, value);
}

static UA_StatusCode
readWritable(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
              FTEntry *node, UA_Boolean includeSourceTimeStamp, UA_DataValue *value) {
    UA_FileAccessRights rights = 0;
    UA_StatusCode res =
        getFTAccessRights(server, node, sessionId, sessionContext, &rights);
    if(res != UA_STATUSCODE_GOOD) {
        value->hasStatus = true;
        value->status = res;
        return UA_STATUSCODE_GOOD;
    }
    UA_Boolean writable = (rights & UA_FILEACCESS_WRITE) != 0;
    return readScalar(&writable, &UA_TYPES[UA_TYPES_BOOLEAN], includeSourceTimeStamp, value);
}

static UA_StatusCode
readWritableCallback(UA_Server *server, const UA_NodeId *sessionId,
                     void *sessionContext, const UA_NodeId *nodeId,
                     void *nodeContext, UA_Boolean includeSourceTimeStamp,
                     const UA_NumericRange *range, UA_DataValue *value) {
    return readWritable(server, NULL, NULL, (FTEntry*)nodeContext,
                         includeSourceTimeStamp, value);
}

static UA_StatusCode
readUserWritableCallback(UA_Server *server, const UA_NodeId *sessionId,
                         void *sessionContext, const UA_NodeId *nodeId,
                         void *nodeContext, UA_Boolean includeSourceTimeStamp,
                         const UA_NumericRange *range, UA_DataValue *value) {
    return readWritable(server, sessionId, sessionContext, (FTEntry*)nodeContext,
                         includeSourceTimeStamp, value);
}

typedef enum {
    FT_PROPERTY_SIZE,
    FT_PROPERTY_WRITABLE,
    FT_PROPERTY_USERWRITABLE,
    FT_PROPERTY_OPENCOUNT,
    FT_PROPERTY_LASTMODIFIED,
    FT_PROPERTY_MAXLENGTH,
    FT_PROPERTY_MIMETYPE,
    FT_PROPERTIES_SIZE
} FTProperty;

/* Only the binding is saved, not an artificial VariableNode. Internal values
 * are copied; external values and callbacks remain application-owned. */
typedef struct {
    UA_Boolean saved;
    void *context;
    UA_ValueSourceType sourceType;
    UA_ValueSourceNotifications notifications;
    union {
        UA_DataValue internal;
        UA_atomic(UA_DataValue*) *external;
        UA_CallbackValueSource callback;
    } value;
} FTPropertySnapshot;

typedef struct {
    UA_NodeId nodeId;
    UA_Boolean created;
} FTPropertyBinding;

typedef struct FTFileBinding {
    UA_Boolean bound; /* All snapshots completed before the first mutation */
    FTPropertyBinding properties[FT_PROPERTIES_SIZE];
    FTPropertySnapshot *saved; /* Allocated only for reused Objects */
} FTFileBinding;

static const struct {
    const char *name;
    UA_UInt16 typeIndex;
    UA_CallbackValueSource source;
} fileProperties[FT_PROPERTIES_SIZE] = {
    {"Size", UA_TYPES_UINT64, {readSizeCallback, NULL}},
    {"Writable", UA_TYPES_BOOLEAN, {readWritableCallback, NULL}},
    {"UserWritable", UA_TYPES_BOOLEAN, {readUserWritableCallback, NULL}},
    {"OpenCount", UA_TYPES_UINT16, {readOpenCountCallback, NULL}},
    {"LastModifiedTime", UA_TYPES_DATETIME, {readLastModifiedCallback, NULL}},
    {"MaxByteStringLength", UA_TYPES_UINT32, {readMaxLengthCallback, NULL}},
    {"MimeType", UA_TYPES_STRING, {NULL, NULL}}
};

static UA_StatusCode
savePropertyBinding(UA_Server *server, const UA_NodeId nodeId,
                     FTPropertySnapshot *binding) {
    UA_Nodestore *ns = UA_Server_getConfig(server)->nodestore;
    const UA_Node *original = ns->getNode(ns, &nodeId,
        UA_NODEATTRIBUTESMASK_VALUE, UA_REFERENCETYPESET_NONE,
        UA_BROWSEDIRECTION_INVALID);
    if(!original)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(original->head.nodeClass != UA_NODECLASS_VARIABLE) {
        res = UA_STATUSCODE_BADNODECLASSINVALID;
    } else {
        const UA_VariableNode *v = &original->variableNode;
        binding->context = original->head.context;
        binding->sourceType = v->valueSourceType;
        switch(v->valueSourceType) {
        case UA_VALUESOURCETYPE_INTERNAL:
            binding->notifications = v->valueSource.internal.notifications;
            res = UA_DataValue_copy(&v->valueSource.internal.value,
                                     &binding->value.internal);
            break;
        case UA_VALUESOURCETYPE_EXTERNAL:
            binding->notifications = v->valueSource.external.notifications;
            binding->value.external = v->valueSource.external.value;
            break;
        case UA_VALUESOURCETYPE_CALLBACK:
            binding->value.callback = v->valueSource.callback;
            /* A snapshot must not retain another driver's entry context. */
            for(size_t i = 0; i < FT_PROPERTIES_SIZE; i++) {
                if(fileProperties[i].source.read &&
                   v->valueSource.callback.read == fileProperties[i].source.read) {
                    res = UA_STATUSCODE_BADNODEIDEXISTS;
                    break;
                }
            }
            break;
        }
    }
    ns->releaseNode(ns, original);
    binding->saved = (res == UA_STATUSCODE_GOOD);
    return res;
}

static void
restorePropertyBinding(UA_Server *server, const UA_NodeId nodeId,
                        const FTPropertySnapshot *binding) {
    UA_Server_setNodeContext(server, nodeId, binding->context);
    switch(binding->sourceType) {
    case UA_VALUESOURCETYPE_INTERNAL:
        UA_Server_setVariableNode_internalValueSource(server, nodeId,
            &binding->value.internal, &binding->notifications);
        break;
    case UA_VALUESOURCETYPE_EXTERNAL:
        /* The public API omits the atomic qualification of the value slot. */
        UA_Server_setVariableNode_externalValueSource(server, nodeId,
            (UA_DataValue**)(uintptr_t)binding->value.external,
            &binding->notifications);
        break;
    case UA_VALUESOURCETYPE_CALLBACK:
        UA_Server_setVariableNode_callbackValueSource(server, nodeId,
            binding->value.callback);
        break;
    }
}

/* Installing a callback does not validate its result against the Variable's
 * attributes. Retain the validation previously performed by Property writes,
 * without invoking an application's old value source. */
static UA_StatusCode
checkScalarProperty(UA_Server *server, const UA_NodeId *nodeId,
                     const UA_DataType *type) {
    UA_Nodestore *ns = UA_Server_getConfig(server)->nodestore;
    const UA_Node *node = ns->getNode(ns, nodeId,
        UA_NODEATTRIBUTESMASK_DATATYPE | UA_NODEATTRIBUTESMASK_VALUERANK |
        UA_NODEATTRIBUTESMASK_ARRAYDIMENSIONS, UA_REFERENCETYPESET_NONE,
        UA_BROWSEDIRECTION_INVALID);
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    UA_StatusCode res = UA_STATUSCODE_BADNODECLASSINVALID;
    if(node->head.nodeClass != UA_NODECLASS_VARIABLE)
        goto cleanup;
    const UA_VariableNode *v = &node->variableNode;
    res = UA_STATUSCODE_BADTYPEMISMATCH;
    if(v->valueRank < UA_VALUERANK_SCALAR_OR_ONE_DIMENSION ||
       v->valueRank > UA_VALUERANK_SCALAR || v->arrayDimensionsSize != 0)
        goto cleanup;
    if(UA_NodeId_equal(&v->dataType, &type->typeId) || UA_NodeId_isNull(&v->dataType)) {
        res = UA_STATUSCODE_GOOD;
        goto cleanup;
    }

    /* Accept a supertype constraint or a subtype represented by this scalar
     * type, just as a normal Value write does. */
    UA_Boolean found;
    res = hasSupertype(server, type->typeId, v->dataType, UA_NODECLASS_DATATYPE, &found);
    if(res == UA_STATUSCODE_GOOD && !found)
        res = hasSupertype(server, v->dataType, type->typeId, UA_NODECLASS_DATATYPE, &found);
    if(res == UA_STATUSCODE_GOOD && !found)
        res = UA_STATUSCODE_BADTYPEMISMATCH;
 cleanup:
    ns->releaseNode(ns, node);
    return res;
}

/* Cache all Property ids and save reused bindings before changing any of them.
 * The same release path rolls back a partial setup and frees a complete one. */
UA_StatusCode
setupFileNode(UA_Server *server, FTEntry *node,
              const UA_FileTransferFileInfo *info) {
    if(node->binding)
        return UA_STATUSCODE_GOOD;
    FTFileBinding *binding = (FTFileBinding*)UA_calloc(1, sizeof(FTFileBinding));
    if(!binding)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    if(!node->created) {
        binding->saved = (FTPropertySnapshot*)UA_calloc(FT_PROPERTIES_SIZE,
                                                       sizeof(FTPropertySnapshot));
        if(!binding->saved) {
            UA_free(binding);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
    }
    node->binding = binding;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < FT_PROPERTIES_SIZE; i++) {
        FTPropertyBinding *property = &binding->properties[i];
        res = getChildId(server, node->nodeId, fileProperties[i].name, &property->nodeId);
        if(res == UA_STATUSCODE_BADNOTFOUND && i >= FT_PROPERTY_LASTMODIFIED) {
            res = UA_STATUSCODE_GOOD;
            continue;
        }
        if(res == UA_STATUSCODE_GOOD && !node->created)
            res = savePropertyBinding(server, property->nodeId, &binding->saved[i]);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
    }
    binding->bound = true;

    const char *mtEnd = (const char*)memchr(info->mimeType, 0, sizeof(info->mimeType));
    UA_String mimeType = {mtEnd ? (size_t)(mtEnd - info->mimeType) : sizeof(info->mimeType),
                          (UA_Byte*)(uintptr_t)info->mimeType};
    for(size_t i = 0; i < FT_PROPERTIES_SIZE; i++) {
        FTPropertyBinding *property = &binding->properties[i];
        if(i == FT_PROPERTY_MIMETYPE && mimeType.length == 0)
            continue;
        UA_Variant value;
        UA_Variant_init(&value);
        if(i == FT_PROPERTY_MIMETYPE)
            UA_Variant_setScalar(&value, &mimeType, &UA_TYPES[UA_TYPES_STRING]);
        if(UA_NodeId_isNull(&property->nodeId)) {
            UA_VariableAttributes attr = UA_VariableAttributes_default;
            attr.displayName = UA_LOCALIZEDTEXT("", (char*)(uintptr_t)fileProperties[i].name);
            attr.dataType = UA_TYPES[fileProperties[i].typeIndex].typeId;
            attr.valueRank = UA_VALUERANK_SCALAR;
            attr.value = value;
            UA_UInt64 zero = 0;
            if(fileProperties[i].source.read)
                UA_Variant_setScalar(&attr.value, &zero, &UA_TYPES[fileProperties[i].typeIndex]);
            res = UA_Server_addVariableNode(server, UA_NODEID_NULL, node->nodeId,
                UA_NS0ID(HASPROPERTY), UA_QUALIFIEDNAME(0, (char*)(uintptr_t)fileProperties[i].name),
                UA_NS0ID(PROPERTYTYPE), attr, NULL, &property->nodeId);
            property->created = (res == UA_STATUSCODE_GOOD);
        } else if(value.type) {
            res = UA_Server_writeValue(server, property->nodeId, value);
        }
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
        if(i == FT_PROPERTY_OPENCOUNT || i == FT_PROPERTY_MAXLENGTH) {
            res = checkScalarProperty(server, &property->nodeId,
                                        &UA_TYPES[fileProperties[i].typeIndex]);
            if(res != UA_STATUSCODE_GOOD)
                goto cleanup;
        }
        if(fileProperties[i].source.read) {
            res = UA_Server_setNodeContext(server, property->nodeId, node);
            if(res == UA_STATUSCODE_GOOD)
                res = UA_Server_setVariableNode_callbackValueSource(
                    server, property->nodeId, fileProperties[i].source);
        }
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
    }
    return UA_STATUSCODE_GOOD;

 cleanup:
    releaseFileNode(server, node);
    return res;
}

void
releaseFileNode(UA_Server *server, FTEntry *node) {
    FTFileBinding *binding = node->binding;
    if(!binding)
        return;
    for(size_t i = 0; i < FT_PROPERTIES_SIZE; i++) {
        FTPropertyBinding *property = &binding->properties[i];
        FTPropertySnapshot *saved = binding->saved ? &binding->saved[i] : NULL;
        if(binding->bound && saved && saved->saved) {
            restorePropertyBinding(server, property->nodeId, saved);
        } else if(binding->bound && node->created && fileProperties[i].source.read &&
                  !UA_NodeId_isNull(&property->nodeId)) {
            /* A Property may survive its created Object via another parent.
             * Replace callbacks carrying our context with a typed zero value. */
            UA_UInt64 zero = 0;
            UA_DataValue value;
            UA_DataValue_init(&value);
            UA_Variant_setScalar(&value.value, &zero, &UA_TYPES[fileProperties[i].typeIndex]);
            value.hasValue = true;
            UA_Server_setVariableNode_internalValueSource(server, property->nodeId, &value, NULL);
            UA_Server_setNodeContext(server, property->nodeId, NULL);
        }
        if(!node->created && property->created)
            UA_Server_deleteNode(server, property->nodeId, true);
        if(saved && saved->sourceType == UA_VALUESOURCETYPE_INTERNAL)
            UA_DataValue_clear(&saved->value.internal);
        UA_NodeId_clear(&property->nodeId);
    }
    UA_free(binding->saved);
    UA_free(binding);
    node->binding = NULL;
}

UA_Boolean
isBelow(const FTEntry *node, const FTEntry *root) {
    for(const FTEntry *parent = node->parent; parent; parent = parent->parent) {
        if(parent == root)
            return true;
    }
    return false;
}

/* Match storage names to an existing information-model structure. Namespace
 * indices need not match the namespace used for newly mirrored Objects. */
static UA_StatusCode
findExistingChild(UA_Server *server, FileTransferDriver *ftd,
                   const UA_NodeId parent, const UA_String name,
                   UA_NodeId *outNodeId) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = parent;
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.referenceTypeId = UA_NS0ID(HIERARCHICALREFERENCES);
    bd.includeSubtypes = true;
    bd.nodeClassMask = UA_NODECLASS_OBJECT;
    bd.resultMask = UA_BROWSERESULTMASK_BROWSENAME;
    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_Boolean found = false;
    while(true) {
        if(br.statusCode != UA_STATUSCODE_GOOD) {
            res = br.statusCode;
            break;
        }
        for(size_t i = 0; i < br.referencesSize; i++) {
            UA_ReferenceDescription *ref = &br.references[i];
            if(!UA_String_equal(&ref->browseName.name, &name) ||
               !UA_ExpandedNodeId_isLocal(&ref->nodeId))
                continue;
            FTEntry *managed = findFTEntry(ftd, &ref->nodeId.nodeId);
            if(managed && managed->zombie)
                continue;
            if(found && UA_NodeId_equal(outNodeId, &ref->nodeId.nodeId))
                continue; /* The same Object can have multiple references. */
            if(found) {
                res = UA_STATUSCODE_BADBROWSENAMEDUPLICATED;
                break;
            }
            res = UA_NodeId_copy(&ref->nodeId.nodeId, outNodeId);
            if(res != UA_STATUSCODE_GOOD)
                break;
            found = true;
        }
        if(res != UA_STATUSCODE_GOOD || br.continuationPoint.length == 0)
            break;
        UA_BrowseResult next = UA_Server_browseNext(server, false, &br.continuationPoint);
        UA_BrowseResult_clear(&br);
        br = next;
    }
    if(br.continuationPoint.length > 0) {
        UA_BrowseResult released = UA_Server_browseNext(server, true, &br.continuationPoint);
        UA_BrowseResult_clear(&released);
    }
    UA_BrowseResult_clear(&br);
    if(res != UA_STATUSCODE_GOOD) {
        UA_NodeId_clear(outNodeId);
        return res;
    }
    return found ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADNOTFOUND;
}

/* Create a FileType Object (with info) or a FileDirectoryType Object (info
 * NULL) with its FTEntry below a directory node */
UA_StatusCode
mirrorObject(UA_Server *server, FTEntry *dirNode,
             const UA_String name, const UA_FileTransferFileInfo *info,
             FTEntry **outNode) {
    FileTransferDriver *ftd = dirNode->driver;
    UA_String path = UA_STRING_NULL;
    UA_StatusCode res = joinPath(dirNode->path, name, &path);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* The generated NodeId (and those of the instantiated children) are in
     * the namespace of the mount as well */
    UA_UInt16 nsIndex = dirNode->driver->config.namespaceIndex;
    UA_QualifiedName browseName = {nsIndex, name};
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName.text = name;
    UA_NodeId newNodeId = UA_NODEID_NULL;
    res = findExistingChild(server, ftd, dirNode->nodeId, name, &newNodeId);
    UA_Boolean created = (res == UA_STATUSCODE_BADNOTFOUND);
    if(created) {
        res = UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(nsIndex, 0),
                                      dirNode->nodeId, UA_NS0ID(ORGANIZES), browseName,
                                      info ? UA_NS0ID(FILETYPE) : UA_NS0ID(FILEDIRECTORYTYPE),
                                      attr, &ftd->driver, &newNodeId);
    } else if(res == UA_STATUSCODE_GOOD) {
        if(findEntryOwner(server, &newNodeId) || findFTEntry(ftd, &newNodeId))
            res = UA_STATUSCODE_BADNODEIDEXISTS;
        else
            res = checkFileTransferObject(server, newNodeId, !info);
    }
    if(res != UA_STATUSCODE_GOOD) {
        UA_NodeId_clear(&newNodeId);
        UA_String_clear(&path);
        return res;
    }

    FTEntry *node = newFTEntry(ftd, dirNode, newNodeId, path, !info);
    if(!node) {
        res = UA_STATUSCODE_BADOUTOFMEMORY;
    } else {
        node->created = created;
        res = bindObjectContext(server, node);
        if(res == UA_STATUSCODE_GOOD && info)
            res = setupFileNode(server, node, info);
    }
    if(res == UA_STATUSCODE_GOOD)
        res = bindObjectMethods(server, node);
    if(res != UA_STATUSCODE_GOOD) {
        if(node)
            removeSubtree(server, node);
        else if(created)
            UA_Server_deleteNode(server, newNodeId, true);
    } else if(outNode) {
        *outNode = node;
    }
    UA_NodeId_clear(&newNodeId);
    UA_String_clear(&path);
    return res;
}

/* Detach reused Objects before deleting their driver-created parents, since
 * deleting an Object also deletes children with no other hierarchical parent. */
static void
detachReusedObject(UA_Server *server, FileTransferDriver *ftd,
                   FTEntry *node, FTEntry *subtreeRoot) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = node->nodeId;
    bd.browseDirection = UA_BROWSEDIRECTION_INVERSE;
    bd.referenceTypeId = UA_NS0ID(HIERARCHICALREFERENCES);
    bd.includeSubtypes = true;
    bd.resultMask = UA_BROWSERESULTMASK_REFERENCETYPEID;
    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
    while(br.statusCode == UA_STATUSCODE_GOOD) {
        UA_Boolean removed = false;
        for(size_t i = 0; i < br.referencesSize; i++) {
            UA_ReferenceDescription *ref = &br.references[i];
            if(!UA_ExpandedNodeId_isLocal(&ref->nodeId))
                continue;
            FTEntry *parent = findFTEntry(ftd, &ref->nodeId.nodeId);
            if(!parent || !parent->created ||
               (parent != subtreeRoot && !isBelow(parent, subtreeRoot)))
                continue;
            if(UA_Server_deleteReference(server, node->nodeId, ref->referenceTypeId,
                false, ref->nodeId, true) == UA_STATUSCODE_GOOD) {
                removed = true;
                break;
            }
        }
        if(!removed && br.continuationPoint.length == 0)
            break;
        UA_BrowseResult next;
        if(removed) {
            /* Restart after a mutation so continuation offsets cannot skip
             * another reference to a parent that is being removed. */
            if(br.continuationPoint.length > 0) {
                UA_BrowseResult released = UA_Server_browseNext(server, true, &br.continuationPoint);
                UA_BrowseResult_clear(&released);
            }
            next = UA_Server_browse(server, 0, &bd);
        } else {
            next = UA_Server_browseNext(server, false, &br.continuationPoint);
        }
        UA_BrowseResult_clear(&br);
        br = next;
    }
    UA_BrowseResult_clear(&br);
}

/* Detach reused Objects and release all Property callbacks before deleting
 * any Object: both Objects and Properties may have extra model parents. */
static void *
prepareSubtreeRelease(void *context, FTEntry *node) {
    FTEntry *root = (FTEntry*)context;
    UA_Server *server = node->driver->driver.server;
    if(!node->created)
        detachReusedObject(server, node->driver, node, root);
    releaseFileNode(server, node);
    ZIP_ITER(FTChildrenByName, &node->children, prepareSubtreeRelease, root);
    return NULL;
}

static void
releaseSubtree(UA_Server *server, FTEntry *node) {
    FTEntry *child;
    while((child = ZIP_ROOT(&node->children)))
        releaseSubtree(server, child);
    unbindObjectMethods(server, node);
    if(node->created)
        UA_Server_deleteNode(server, node->nodeId, true);
    removeFTEntry(node->driver, node);
}

/* Handles must be closed before releasing their entries. Only driver-created
 * Objects are deleted; reused Objects recover their application bindings. */
void
removeSubtree(UA_Server *server, FTEntry *subtreeRoot) {
    prepareSubtreeRelease(subtreeRoot, subtreeRoot);
    releaseSubtree(server, subtreeRoot);
}
