/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include "file_transfer_internal.h"

#ifdef UA_ENABLE_DRIVER_FILE_TRANSFER

/**************************************
 * Property Value Sources
 **************************************/

UA_StatusCode
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

typedef enum {
    FT_FILEINFO_SIZE,
    FT_FILEINFO_WRITABLE,
    FT_FILEINFO_LASTMODIFIED
} FTFileInfoField;

/* The Size, Writable and LastModifiedTime Properties are computed from the
 * backend on demand. They stay correct when the file changes behind the
 * server. Writable is the storage writability within the mount configuration
 * (Part 20, 4.2.1). */
static UA_StatusCode
readFileInfo(void *nodeContext, UA_Boolean includeSourceTimeStamp,
             FTFileInfoField field, UA_DataValue *value) {
    FTNode *node = (FTNode*)nodeContext;
    UA_FileTransferFileBackend *b = &node->mount->backend.file;
    UA_FileTransferFileInfo info;
    UA_StatusCode res = backendGetInfo(b, node->path, &info);
    if(res != UA_STATUSCODE_GOOD) {
        value->hasStatus = true;
        value->status = res;
        return UA_STATUSCODE_GOOD;
    }
    UA_Boolean writable = !node->mount->options.readOnly && info.writable;
    if(field == FT_FILEINFO_SIZE)
        res = UA_Variant_setScalarCopy(&value->value, &info.size,
                                       &UA_TYPES[UA_TYPES_UINT64]);
    else if(field == FT_FILEINFO_WRITABLE)
        res = UA_Variant_setScalarCopy(&value->value, &writable,
                                       &UA_TYPES[UA_TYPES_BOOLEAN]);
    else
        res = UA_Variant_setScalarCopy(&value->value, &info.lastModified,
                                       &UA_TYPES[UA_TYPES_DATETIME]);
    value->hasValue = (res == UA_STATUSCODE_GOOD);
    if(includeSourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
    return UA_STATUSCODE_GOOD;
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
readWritableCallback(UA_Server *server, const UA_NodeId *sessionId,
                     void *sessionContext, const UA_NodeId *nodeId,
                     void *nodeContext, UA_Boolean includeSourceTimeStamp,
                     const UA_NumericRange *range, UA_DataValue *value) {
    return readFileInfo(nodeContext, includeSourceTimeStamp,
                        FT_FILEINFO_WRITABLE, value);
}

static UA_StatusCode
readLastModifiedCallback(UA_Server *server, const UA_NodeId *sessionId,
                         void *sessionContext, const UA_NodeId *nodeId,
                         void *nodeContext, UA_Boolean includeSourceTimeStamp,
                         const UA_NumericRange *range, UA_DataValue *value) {
    return readFileInfo(nodeContext, includeSourceTimeStamp,
                        FT_FILEINFO_LASTMODIFIED, value);
}

/* The value of the Writable Property */
static UA_Boolean
fileWritable(FTNode *node) {
    UA_FileTransferFileBackend *b = &node->mount->backend.file;
    UA_FileTransferFileInfo info;
    return !node->mount->options.readOnly &&
        backendGetInfo(b, node->path, &info) == UA_STATUSCODE_GOOD &&
        info.writable;
}

UA_Boolean
userCanWrite(UA_Server *server, FTNode *node, const UA_NodeId *sessionId) {
    const UA_FileTransferMountOptions *opts = &node->mount->options;
    if(opts->readOnly)
        return false;
    if(opts->getUserWritable)
        return opts->getUserWritable(server, sessionId, &node->nodeId,
                                     opts->mountContext);
    return true;
}

/* UserWritable takes the user access rights of the Session into account. They
 * restrict Writable, they do not replace it (Part 20, 4.2.1). */
static UA_StatusCode
readUserWritableCallback(UA_Server *server, const UA_NodeId *sessionId,
                         void *sessionContext, const UA_NodeId *nodeId,
                         void *nodeContext, UA_Boolean includeSourceTimeStamp,
                         const UA_NumericRange *range, UA_DataValue *value) {
    FTNode *node = (FTNode*)nodeContext;
    UA_Boolean userWritable =
        fileWritable(node) && userCanWrite(server, node, sessionId);
    value->hasValue = (UA_Variant_setScalarCopy(
        &value->value, &userWritable, &UA_TYPES[UA_TYPES_BOOLEAN]) ==
        UA_STATUSCODE_GOOD);
    if(includeSourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
    return UA_STATUSCODE_GOOD;
}

/* Wire up the Properties of an instantiated FileType Object */
UA_StatusCode
setupFileNode(UA_Server *server, FileTransferDriver *ftd, FTNode *node,
              const UA_FileTransferFileInfo *info) {
    /* Size is read from the backend on demand */
    UA_NodeId sizeId = UA_NODEID_NULL;
    UA_StatusCode res = getChildId(server, node->nodeId, "Size", &sizeId);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_CallbackValueSource sizeSource;
    memset(&sizeSource, 0, sizeof(UA_CallbackValueSource));
    sizeSource.read = readSizeCallback;
    res = UA_Server_setNodeContext(server, sizeId, node);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_Server_setVariableNode_callbackValueSource(server, sizeId,
                                                             sizeSource);
    UA_NodeId_clear(&sizeId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Writable reflects the mount configuration and the storage */
    UA_NodeId writableId = UA_NODEID_NULL;
    res = getChildId(server, node->nodeId, "Writable", &writableId);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_CallbackValueSource writableSource;
    memset(&writableSource, 0, sizeof(UA_CallbackValueSource));
    writableSource.read = readWritableCallback;
    res = UA_Server_setNodeContext(server, writableId, node);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_Server_setVariableNode_callbackValueSource(server, writableId,
                                                             writableSource);
    UA_NodeId_clear(&writableId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* UserWritable is computed per Session */
    UA_NodeId userWritableId = UA_NODEID_NULL;
    res = getChildId(server, node->nodeId, "UserWritable", &userWritableId);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_CallbackValueSource userWritableSource;
    memset(&userWritableSource, 0, sizeof(UA_CallbackValueSource));
    userWritableSource.read = readUserWritableCallback;
    res = UA_Server_setNodeContext(server, userWritableId, node);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_Server_setVariableNode_callbackValueSource(server, userWritableId,
                                                             userWritableSource);
    UA_NodeId_clear(&userWritableId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* OpenCount is updated by the driver on every Open/Close */
    res = getChildId(server, node->nodeId, "OpenCount", &node->openCountId);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    updateOpenCount(server, node);

    /* The three optional Properties below are normally not instantiated with
     * the Object: open62541 skips optional children unless the application
     * provides a nodeLifecycle->createOptionalChild callback that asks for
     * them. Where it does, the Property already exists and must be reused --
     * adding a second one leaves the Object with a duplicate BrowseName, which
     * makes TranslateBrowsePathsToNodeIds return two targets for one Property
     * and leaves the value source on only one of them. */

    /* LastModifiedTime is computed from the backend on demand */
    UA_CallbackValueSource lmSource;
    memset(&lmSource, 0, sizeof(UA_CallbackValueSource));
    lmSource.read = readLastModifiedCallback;
    UA_NodeId lastModifiedId = UA_NODEID_NULL;
    if(getChildId(server, node->nodeId, "LastModifiedTime",
                  &lastModifiedId) == UA_STATUSCODE_GOOD) {
        res = UA_Server_setNodeContext(server, lastModifiedId, node);
        if(res == UA_STATUSCODE_GOOD)
            res = UA_Server_setVariableNode_callbackValueSource(
                server, lastModifiedId, lmSource);
        UA_NodeId_clear(&lastModifiedId);
    } else {
        UA_VariableAttributes lmAttr = UA_VariableAttributes_default;
        lmAttr.displayName = UA_LOCALIZEDTEXT("", "LastModifiedTime");
        lmAttr.dataType = UA_TYPES[UA_TYPES_DATETIME].typeId;
        lmAttr.valueRank = UA_VALUERANK_SCALAR;
        res = UA_Server_addCallbackValueSourceVariableNode(
            server, UA_NODEID_NULL, node->nodeId, UA_NS0ID(HASPROPERTY),
            UA_QUALIFIEDNAME(0, "LastModifiedTime"), UA_NS0ID(PROPERTYTYPE),
            lmAttr, lmSource, node, NULL);
    }
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* MaxByteStringLength advertises the maximum number of bytes accepted or
     * returned by a single Read/Write (the driver's max-read-length) */
    UA_Variant mbslValue;
    UA_Variant_setScalar(&mbslValue, &ftd->maxReadLength,
                         &UA_TYPES[UA_TYPES_UINT32]);
    UA_NodeId mbslId = UA_NODEID_NULL;
    if(getChildId(server, node->nodeId, "MaxByteStringLength",
                  &mbslId) == UA_STATUSCODE_GOOD) {
        res = UA_Server_writeValue(server, mbslId, mbslValue);
        UA_NodeId_clear(&mbslId);
    } else {
        UA_VariableAttributes mbslAttr = UA_VariableAttributes_default;
        mbslAttr.displayName = UA_LOCALIZEDTEXT("", "MaxByteStringLength");
        mbslAttr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
        mbslAttr.valueRank = UA_VALUERANK_SCALAR;
        mbslAttr.value = mbslValue;
        res = UA_Server_addVariableNode(
            server, UA_NODEID_NULL, node->nodeId, UA_NS0ID(HASPROPERTY),
            UA_QUALIFIEDNAME(0, "MaxByteStringLength"), UA_NS0ID(PROPERTYTYPE),
            mbslAttr, NULL, NULL);
    }
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* MimeType is only meaningful when the backend reports a media type. An
     * existing Property is filled in; it is never added for an unknown type. */
    UA_NodeId mimeTypeId = UA_NODEID_NULL;
    UA_Boolean mimeTypeExists =
        (getChildId(server, node->nodeId, "MimeType",
                    &mimeTypeId) == UA_STATUSCODE_GOOD);
    const char *mtEnd = (const char*)
        memchr(info->mimeType, 0, sizeof(info->mimeType));
    UA_String mimeType;
    mimeType.data = (UA_Byte*)(uintptr_t)info->mimeType;
    mimeType.length = (mtEnd) ? (size_t)(mtEnd - info->mimeType) :
        sizeof(info->mimeType);
    if(mimeType.length > 0) {
        UA_Variant mtValue;
        UA_Variant_setScalar(&mtValue, &mimeType, &UA_TYPES[UA_TYPES_STRING]);
        if(mimeTypeExists) {
            res = UA_Server_writeValue(server, mimeTypeId, mtValue);
        } else {
            UA_VariableAttributes mtAttr = UA_VariableAttributes_default;
            mtAttr.displayName = UA_LOCALIZEDTEXT("", "MimeType");
            mtAttr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
            mtAttr.valueRank = UA_VALUERANK_SCALAR;
            mtAttr.value = mtValue;
            res = UA_Server_addVariableNode(
                server, UA_NODEID_NULL, node->nodeId, UA_NS0ID(HASPROPERTY),
                UA_QUALIFIEDNAME(0, "MimeType"), UA_NS0ID(PROPERTYTYPE),
                mtAttr, NULL, NULL);
        }
    }
    UA_NodeId_clear(&mimeTypeId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    return UA_STATUSCODE_GOOD;
}

void
releaseFileNode(UA_Server *server, FTNode *node) {
    UA_UInt64 size = 0;
    UA_Boolean writable = false;
    UA_DateTime lastModified = 0;
    const struct {
        const char *name;
        void *value;
        const UA_DataType *type;
    } sources[4] = {
        {"Size", &size, &UA_TYPES[UA_TYPES_UINT64]},
        {"Writable", &writable, &UA_TYPES[UA_TYPES_BOOLEAN]},
        {"UserWritable", &writable, &UA_TYPES[UA_TYPES_BOOLEAN]},
        {"LastModifiedTime", &lastModified, &UA_TYPES[UA_TYPES_DATETIME]}
    };
    for(size_t i = 0; i < 4; i++) {
        UA_NodeId propertyId;
        if(getChildId(server, node->nodeId, sources[i].name,
                      &propertyId) != UA_STATUSCODE_GOOD)
            continue;
        UA_DataValue dv;
        UA_DataValue_init(&dv);
        UA_Variant_setScalar(&dv.value, sources[i].value, sources[i].type);
        dv.hasValue = true;
        UA_Server_setVariableNode_internalValueSource(server, propertyId, &dv, NULL);
        UA_Server_setNodeContext(server, propertyId, NULL);
        UA_NodeId_clear(&propertyId);
    }
}

/**************************************
 * FileType Method Callbacks
 **************************************/

/* Resolve the FTNode of a file Object addressed by a Method call */
static UA_StatusCode
resolveFileNode(FileTransferDriver *ftd, const UA_NodeId *objectId,
                FTNode **outNode) {
    FTNode *node = findFTNode(ftd, objectId);
    if(!node || node->isDirectory || node->zombie)
        return UA_STATUSCODE_BADNOTFOUND;
    *outNode = node;
    return UA_STATUSCODE_GOOD;
}

/* Resolve the FTHandle from the fileHandle Method argument. Handles are only
 * valid within the Session that opened them and for the file Object the
 * Method is called on. */
static UA_StatusCode
resolveHandle(FileTransferDriver *ftd, const UA_NodeId *sessionId,
              const UA_NodeId *objectId, const UA_Variant *arg,
              FTHandle **outHandle) {
    if(!UA_Variant_hasScalarType(arg, &UA_TYPES[UA_TYPES_UINT32]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_UInt32 handleId = *(UA_UInt32*)arg->data;
    FTHandle *h = findFTHandle(ftd, sessionId, handleId);
    if(!h || !UA_NodeId_equal(&h->file->nodeId, objectId))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    *outHandle = h;
    return UA_STATUSCODE_GOOD;
}

/* Open a file and register the handle. Shared between the Open Method and
 * CreateFile with requestFileOpen. */
UA_StatusCode
openFileHandle(UA_Server *server, FileTransferDriver *ftd, FTNode *node,
               const UA_NodeId *sessionId, UA_Byte mode,
               UA_UInt32 *outHandle) {
    /* Validate the mode bit combination */
    if((mode & ~UA_FILETRANSFER_OPENMODE_ALLBITS) ||
       !(mode & (UA_OPENFILEMODE_READ | UA_OPENFILEMODE_WRITE)) ||
       ((mode & UA_OPENFILEMODE_ERASEEXISTING) &&
        !(mode & UA_OPENFILEMODE_WRITE)))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Locking semantics (Part 20, 4.2.2): a file that is open cannot be
     * opened for writing; a file that is open for writing cannot be opened
     * for reading */
    UA_Boolean writeBit = (mode & UA_OPENFILEMODE_WRITE) != 0;
    if(writeBit && (node->openCount > 0 || !fileWritable(node) ||
                    !userCanWrite(server, node, sessionId)))
        return UA_STATUSCODE_BADNOTWRITABLE;
    if((mode & UA_OPENFILEMODE_READ) && node->openForWrite)
        return UA_STATUSCODE_BADNOTREADABLE;

    /* Resource limits */
    if(node->openCount >= ftd->maxHandlesPerFile ||
       countSessionHandles(ftd, sessionId) >= ftd->maxHandlesPerSession)
        return UA_STATUSCODE_BADRESOURCEUNAVAILABLE;

    UA_FileTransferFileBackend *b = &node->mount->backend.file;
    UA_UInt32 backendHandle = 0;
    UA_StatusCode res = b->open(b, node->path, mode, &backendHandle);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    FTHandle *h = (FTHandle*)UA_calloc(1, sizeof(FTHandle));
    if(!h) {
        b->close(b, backendHandle);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    res = UA_NodeId_copy(sessionId, &h->sessionId);
    if(res != UA_STATUSCODE_GOOD) {
        b->close(b, backendHandle);
        UA_free(h);
        return res;
    }

    h->handle = newHandleId(ftd);
    h->file = node;
    h->mode = mode;
    h->backendHandle = backendHandle;
    LIST_INSERT_HEAD(&ftd->handles, h, listEntry);

    node->openCount++;
    if(writeBit)
        node->openForWrite = true;
    updateOpenCount(server, node);

    *outHandle = h->handle;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
openMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                   void *sessionContext, const UA_NodeId *methodId,
                   void *methodContext, const UA_NodeId *objectId,
                   void *objectContext, size_t inputSize, const UA_Variant *input,
                   size_t outputSize, UA_Variant *output) {
    FileTransferDriver *ftd = findFileTransferDriver(server);
    if(!ftd)
        return UA_STATUSCODE_BADNOTSUPPORTED;

    FTNode *node = NULL;
    UA_StatusCode res = resolveFileNode(ftd, objectId, &node);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    if(inputSize < 1 || outputSize < 1 ||
       !UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_BYTE]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_Byte mode = *(UA_Byte*)input[0].data;

    UA_UInt32 handle = 0;
    res = openFileHandle(server, ftd, node, sessionId, mode, &handle);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* The client cannot close a handle it does not receive */
    res = UA_Variant_setScalarCopy(&output[0], &handle,
                                   &UA_TYPES[UA_TYPES_UINT32]);
    if(res != UA_STATUSCODE_GOOD) {
        FTHandle *h = findFTHandle(ftd, sessionId, handle);
        if(h)
            closeFTHandle(server, ftd, h);
    }
    return res;
}

UA_StatusCode
closeMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                    void *sessionContext, const UA_NodeId *methodId,
                    void *methodContext, const UA_NodeId *objectId,
                    void *objectContext, size_t inputSize, const UA_Variant *input,
                    size_t outputSize, UA_Variant *output) {
    FileTransferDriver *ftd = findFileTransferDriver(server);
    if(!ftd)
        return UA_STATUSCODE_BADNOTSUPPORTED;

    if(inputSize < 1)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    FTHandle *h = NULL;
    UA_StatusCode res = resolveHandle(ftd, sessionId, objectId, &input[0], &h);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    return closeFTHandle(server, ftd, h);
}

UA_StatusCode
readMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                   void *sessionContext, const UA_NodeId *methodId,
                   void *methodContext, const UA_NodeId *objectId,
                   void *objectContext, size_t inputSize, const UA_Variant *input,
                   size_t outputSize, UA_Variant *output) {
    FileTransferDriver *ftd = findFileTransferDriver(server);
    if(!ftd)
        return UA_STATUSCODE_BADNOTSUPPORTED;

    if(inputSize < 2 || outputSize < 1)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    FTHandle *h = NULL;
    UA_StatusCode res = resolveHandle(ftd, sessionId, objectId, &input[0], &h);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(!(h->mode & UA_OPENFILEMODE_READ))
        return UA_STATUSCODE_BADINVALIDSTATE;

    if(!UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_INT32]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_Int32 length = *(UA_Int32*)input[1].data;
    if(length <= 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* The Server is allowed to return less data than the requested length */
    if((UA_UInt32)length > ftd->maxReadLength)
        length = (UA_Int32)ftd->maxReadLength;

    UA_ByteString *data = UA_ByteString_new();
    if(!data)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_FileTransferFileBackend *b = &h->file->mount->backend.file;
    res = b->read(b, h->backendHandle, length, data);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ByteString_delete(data);
        return res;
    }

    UA_Variant_setScalar(&output[0], data, &UA_TYPES[UA_TYPES_BYTESTRING]);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
writeMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                    void *sessionContext, const UA_NodeId *methodId,
                    void *methodContext, const UA_NodeId *objectId,
                    void *objectContext, size_t inputSize, const UA_Variant *input,
                    size_t outputSize, UA_Variant *output) {
    FileTransferDriver *ftd = findFileTransferDriver(server);
    if(!ftd)
        return UA_STATUSCODE_BADNOTSUPPORTED;

    if(inputSize < 2)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    FTHandle *h = NULL;
    UA_StatusCode res = resolveHandle(ftd, sessionId, objectId, &input[0], &h);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(!(h->mode & UA_OPENFILEMODE_WRITE))
        return UA_STATUSCODE_BADINVALIDSTATE;

    /* Writing an empty or null ByteString is a no-op with a Good result */
    if(UA_Variant_isEmpty(&input[1]))
        return UA_STATUSCODE_GOOD;
    if(!UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_BYTESTRING]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_ByteString data = *(UA_ByteString*)input[1].data;
    if(data.length == 0)
        return UA_STATUSCODE_GOOD;

    /* The MaxByteStringLength Property announces this limit for Read and Write
     * alike (Part 20, 4.2.1). A Read may return less than requested, but
     * truncating a Write would silently discard client data, so an oversized
     * chunk is rejected instead. */
    if(data.length > ftd->maxReadLength)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_FileTransferFileBackend *b = &h->file->mount->backend.file;
    return b->write(b, h->backendHandle, data);
}

UA_StatusCode
getPositionMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                          void *sessionContext, const UA_NodeId *methodId,
                          void *methodContext, const UA_NodeId *objectId,
                          void *objectContext, size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output) {
    FileTransferDriver *ftd = findFileTransferDriver(server);
    if(!ftd)
        return UA_STATUSCODE_BADNOTSUPPORTED;

    if(inputSize < 1 || outputSize < 1)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    FTHandle *h = NULL;
    UA_StatusCode res = resolveHandle(ftd, sessionId, objectId, &input[0], &h);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_FileTransferFileBackend *b = &h->file->mount->backend.file;
    UA_UInt64 position = 0;
    res = b->getPosition(b, h->backendHandle, &position);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    return UA_Variant_setScalarCopy(&output[0], &position,
                                    &UA_TYPES[UA_TYPES_UINT64]);
}

UA_StatusCode
setPositionMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                          void *sessionContext, const UA_NodeId *methodId,
                          void *methodContext, const UA_NodeId *objectId,
                          void *objectContext, size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output) {
    FileTransferDriver *ftd = findFileTransferDriver(server);
    if(!ftd)
        return UA_STATUSCODE_BADNOTSUPPORTED;

    if(inputSize < 2)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    FTHandle *h = NULL;
    UA_StatusCode res = resolveHandle(ftd, sessionId, objectId, &input[0], &h);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    if(!UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_UINT64]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_UInt64 position = *(UA_UInt64*)input[1].data;

    UA_FileTransferFileBackend *b = &h->file->mount->backend.file;
    return b->setPosition(b, h->backendHandle, position);
}

#endif /* UA_ENABLE_DRIVER_FILE_TRANSFER */
