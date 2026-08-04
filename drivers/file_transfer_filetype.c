/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include "file_transfer_internal.h"

#if defined(UA_ENABLE_METHODCALLS) && defined(UA_GENERATED_NAMESPACE_ZERO_FULL)

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

/* The Size and LastModifiedTime Properties are computed from the backend on
 * demand. They stay correct when the file changes behind the server. */
static UA_StatusCode
readSizeCallback(UA_Server *server, const UA_NodeId *sessionId,
                 void *sessionContext, const UA_NodeId *nodeId,
                 void *nodeContext, UA_Boolean includeSourceTimeStamp,
                 const UA_NumericRange *range, UA_DataValue *value) {
    FTNode *node = (FTNode*)nodeContext;
    UA_FileTransferBackend *b = &node->mount->backend;
    UA_FileTransferFileInfo info;
    UA_StatusCode res = b->getAttributes(b, node->path, &info);
    if(res != UA_STATUSCODE_GOOD) {
        value->hasStatus = true;
        value->status = res;
        return UA_STATUSCODE_GOOD;
    }
    value->hasValue = (UA_Variant_setScalarCopy(
        &value->value, &info.size, &UA_TYPES[UA_TYPES_UINT64]) ==
        UA_STATUSCODE_GOOD);
    if(includeSourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readLastModifiedCallback(UA_Server *server, const UA_NodeId *sessionId,
                         void *sessionContext, const UA_NodeId *nodeId,
                         void *nodeContext, UA_Boolean includeSourceTimeStamp,
                         const UA_NumericRange *range, UA_DataValue *value) {
    FTNode *node = (FTNode*)nodeContext;
    UA_FileTransferBackend *b = &node->mount->backend;
    UA_FileTransferFileInfo info;
    UA_StatusCode res = b->getAttributes(b, node->path, &info);
    if(res != UA_STATUSCODE_GOOD) {
        value->hasStatus = true;
        value->status = res;
        return UA_STATUSCODE_GOOD;
    }
    value->hasValue = (UA_Variant_setScalarCopy(
        &value->value, &info.lastModified, &UA_TYPES[UA_TYPES_DATETIME]) ==
        UA_STATUSCODE_GOOD);
    if(includeSourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
    return UA_STATUSCODE_GOOD;
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

/* UserWritable takes the user access rights of the Session into account */
static UA_StatusCode
readUserWritableCallback(UA_Server *server, const UA_NodeId *sessionId,
                         void *sessionContext, const UA_NodeId *nodeId,
                         void *nodeContext, UA_Boolean includeSourceTimeStamp,
                         const UA_NumericRange *range, UA_DataValue *value) {
    FTNode *node = (FTNode*)nodeContext;
    UA_Boolean userWritable = userCanWrite(server, node, sessionId);
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
    UA_Boolean writable = !node->mount->options.readOnly && info->writable;
    UA_Variant value;
    UA_Variant_setScalar(&value, &writable, &UA_TYPES[UA_TYPES_BOOLEAN]);
    res = UA_Server_writeValue(server, writableId, value);
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

    /* The optional LastModifiedTime Property is added explicitly */
    UA_VariableAttributes lmAttr = UA_VariableAttributes_default;
    lmAttr.displayName = UA_LOCALIZEDTEXT("", "LastModifiedTime");
    lmAttr.dataType = UA_TYPES[UA_TYPES_DATETIME].typeId;
    lmAttr.valueRank = UA_VALUERANK_SCALAR;
    UA_CallbackValueSource lmSource;
    memset(&lmSource, 0, sizeof(UA_CallbackValueSource));
    lmSource.read = readLastModifiedCallback;
    res = UA_Server_addCallbackValueSourceVariableNode(
        server, UA_NODEID_NULL, node->nodeId, UA_NS0ID(HASPROPERTY),
        UA_QUALIFIEDNAME(0, "LastModifiedTime"), UA_NS0ID(PROPERTYTYPE),
        lmAttr, lmSource, node, NULL);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* The optional MaxByteStringLength Property advertises the maximum number
     * of bytes returned by a single Read (the driver's max-read-length) */
    UA_VariableAttributes mbslAttr = UA_VariableAttributes_default;
    mbslAttr.displayName = UA_LOCALIZEDTEXT("", "MaxByteStringLength");
    mbslAttr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    mbslAttr.valueRank = UA_VALUERANK_SCALAR;
    UA_Variant_setScalar(&mbslAttr.value, &ftd->maxReadLength,
                         &UA_TYPES[UA_TYPES_UINT32]);
    res = UA_Server_addVariableNode(
        server, UA_NODEID_NULL, node->nodeId, UA_NS0ID(HASPROPERTY),
        UA_QUALIFIEDNAME(0, "MaxByteStringLength"), UA_NS0ID(PROPERTYTYPE),
        mbslAttr, NULL, NULL);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* The optional MimeType Property is added only when the backend reports a
     * media type for the file */
    if(info->mimeType.length > 0) {
        UA_VariableAttributes mtAttr = UA_VariableAttributes_default;
        mtAttr.displayName = UA_LOCALIZEDTEXT("", "MimeType");
        mtAttr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
        mtAttr.valueRank = UA_VALUERANK_SCALAR;
        UA_String mimeType = info->mimeType;
        UA_Variant_setScalar(&mtAttr.value, &mimeType, &UA_TYPES[UA_TYPES_STRING]);
        res = UA_Server_addVariableNode(
            server, UA_NODEID_NULL, node->nodeId, UA_NS0ID(HASPROPERTY),
            UA_QUALIFIEDNAME(0, "MimeType"), UA_NS0ID(PROPERTYTYPE),
            mtAttr, NULL, NULL);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }

    return UA_STATUSCODE_GOOD;
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
    if(writeBit && (node->openCount > 0 || !userCanWrite(server, node, sessionId)))
        return UA_STATUSCODE_BADNOTWRITABLE;
    if((mode & UA_OPENFILEMODE_READ) && node->openForWrite)
        return UA_STATUSCODE_BADNOTREADABLE;

    /* Resource limits */
    if(node->openCount >= ftd->maxHandlesPerFile ||
       countSessionHandles(ftd, sessionId) >= ftd->maxHandlesPerSession)
        return UA_STATUSCODE_BADRESOURCEUNAVAILABLE;

    UA_FileTransferBackend *b = &node->mount->backend;
    void *fileContext = NULL;
    UA_StatusCode res = b->openFile(b, node->path, mode, &fileContext);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    FTHandle *h = (FTHandle*)UA_calloc(1, sizeof(FTHandle));
    if(!h) {
        b->closeFile(b, fileContext);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    res = UA_NodeId_copy(sessionId, &h->sessionId);
    if(res != UA_STATUSCODE_GOOD) {
        b->closeFile(b, fileContext);
        UA_free(h);
        return res;
    }

    h->handle = newHandleId(ftd);
    h->file = node;
    h->mode = mode;
    h->backendFileContext = fileContext;
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

    if(inputSize < 1 || !UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_BYTE]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_Byte mode = *(UA_Byte*)input[0].data;

    UA_UInt32 handle = 0;
    res = openFileHandle(server, ftd, node, sessionId, mode, &handle);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    return UA_Variant_setScalarCopy(&output[0], &handle,
                                    &UA_TYPES[UA_TYPES_UINT32]);
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

    UA_FileTransferBackend *b = &h->file->mount->backend;
    res = b->read(b, h->backendFileContext, length, data);
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

    UA_FileTransferBackend *b = &h->file->mount->backend;
    return b->write(b, h->backendFileContext, data);
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

    FTHandle *h = NULL;
    UA_StatusCode res = resolveHandle(ftd, sessionId, objectId, &input[0], &h);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_FileTransferBackend *b = &h->file->mount->backend;
    UA_UInt64 position = 0;
    res = b->getPosition(b, h->backendFileContext, &position);
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

    FTHandle *h = NULL;
    UA_StatusCode res = resolveHandle(ftd, sessionId, objectId, &input[0], &h);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    if(!UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_UINT64]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_UInt64 position = *(UA_UInt64*)input[1].data;

    UA_FileTransferBackend *b = &h->file->mount->backend;
    return b->setPosition(b, h->backendFileContext, position);
}

#endif /* UA_ENABLE_METHODCALLS && UA_GENERATED_NAMESPACE_ZERO_FULL */