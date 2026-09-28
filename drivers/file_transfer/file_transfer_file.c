/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include "file_transfer_internal.h"

static enum ZIP_CMP
ftHandleOrder(const UA_UInt32 *a, const UA_UInt32 *b) {
    return *a < *b ? ZIP_CMP_LESS : (*a > *b ? ZIP_CMP_MORE : ZIP_CMP_EQ);
}

ZIP_FUNCTIONS(FTHandlesById, FTHandle, idTreeEntry, UA_UInt32, handle, ftHandleOrder)
ZIP_FUNCTIONS(FTHandlesBySession, FTHandle, sessionTreeEntry,
              UA_NodeId, sessionId, ftNodeIdOrder)

/* A NULL Session requests general permissions only. The callbacks operate on
 * the Object, not on its Properties. Unknown permission bits are discarded. */
UA_StatusCode
getFTAccessRights(UA_Server *server, const FTEntry *node,
                const UA_NodeId *sessionId, void *sessionContext,
                UA_FileAccessRights *outRights) {
    *outRights = 0;
    UA_FileTransferFileInfo info;
    UA_FileTransferFileBackend *b = &node->driver->backend.file;
    UA_StatusCode res = backendGetInfo(b, node->path, &info);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_FileAccessRights rights = info.accessRights &
        (UA_FILEACCESS_READ | UA_FILEACCESS_WRITE | UA_FILEACCESS_TRAVERSE);
    const FTConfig *opts = &node->driver->config;
    if(opts->readOnly)
        rights &= (UA_FileAccessRights)~UA_FILEACCESS_WRITE;
    if(b->getAccessRights) {
        UA_FileAccessRights general = 0;
        res = b->getAccessRights(b, server, &node->nodeId, &general);
        if(res != UA_STATUSCODE_GOOD)
            return res;
        rights &= general;
    }
    if(sessionId && b->getUserAccessRights) {
        UA_FileAccessRights user = 0;
        res = b->getUserAccessRights(b, server, sessionId, sessionContext,
                                     &node->nodeId, &user);
        if(res != UA_STATUSCODE_GOOD)
            return res;
        rights &= user;
    }
    *outRights = rights;
    return UA_STATUSCODE_GOOD;
}

/* Check the Object and traverse permission on its managed ancestor directories.
 * A standalone file has no managed ancestors; its backend resolves the path. */
UA_StatusCode
checkFTAccess(UA_Server *server, const FTEntry *node,
              const UA_NodeId *sessionId, void *sessionContext,
              UA_FileAccessRights required,
              UA_StatusCode deniedStatus) {
    UA_FileAccessRights rights = 0;
    UA_StatusCode res =
        getFTAccessRights(server, node, sessionId, sessionContext, &rights);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if((rights & required) != required)
        return deniedStatus;
    for(const FTEntry *parent = node->parent; parent; parent = parent->parent) {
        res = getFTAccessRights(server, parent, sessionId, sessionContext, &rights);
        if(res != UA_STATUSCODE_GOOD)
            return res;
        if(!(rights & UA_FILEACCESS_TRAVERSE))
            return UA_STATUSCODE_BADUSERACCESSDENIED;
    }
    return UA_STATUSCODE_GOOD;
}


FTHandle *
findFTHandle(FileTransferDriver *ftd, const UA_NodeId *sessionId,
             UA_UInt32 handle) {
    FTHandle *h = ZIP_FIND(FTHandlesById, &ftd->handlesById, &handle);
    return h && UA_NodeId_equal(&h->sessionId, sessionId) ? h : NULL;
}

static void *
countSessionHandle(void *context, FTHandle *handle) {
    (*(size_t*)context)++;
    return NULL;
}

static size_t
countSessionHandles(FileTransferDriver *ftd, const UA_NodeId *sessionId) {
    size_t count = 0;
    ZIP_ITER_KEY(FTHandlesBySession, &ftd->handlesBySession,
                 sessionId, countSessionHandle, &count);
    return count;
}

/* Part 20 requires Session-unique handles. Each driver owns its counter and
 * handles; check the other instances without a shared allocator or registry. */
static UA_UInt32
newHandleId(FileTransferDriver *ftd) {
    UA_Boolean inUse;
    do {
        ftd->nextHandle++;
        if(ftd->nextHandle == 0)
            ftd->nextHandle = 1;
        inUse = false;
        for(UA_Driver *drv = UA_Server_getDrivers(ftd->driver.server);
            drv && !inUse; drv = drv->next) {
            if(!isFileTransferDriver(drv))
                continue;
            inUse = ZIP_FIND(FTHandlesById, &((FileTransferDriver*)drv)->handlesById,
                             &ftd->nextHandle) != NULL;
        }
    } while(inUse);
    return ftd->nextHandle;
}

/* Close the backend handle and release the handle. Removes zombie nodes once
 * their last handle is closed. */
UA_StatusCode
closeFTHandle(UA_Server *server, FTHandle *h) {
    FTEntry *node = h->file;
    UA_FileTransferFileBackend *b = &node->driver->backend.file;
    UA_StatusCode res = b->close(b, h->backendHandle);

    ZIP_REMOVE(FTHandlesById, &node->driver->handlesById, h);
    ZIP_REMOVE(FTHandlesBySession, &node->driver->handlesBySession, h);
    UA_NodeId_clear(&h->sessionId);
    if(h->mode & UA_OPENFILEMODE_WRITE)
        node->openForWrite = false;
    for(FTEntry *entry = node; entry; entry = entry->parent) {
        UA_assert(entry->subtreeHandleCount > 0);
        entry->subtreeHandleCount--;
    }
    UA_free(h);

    if(node->zombie && node->subtreeHandleCount == 0)
        removeSubtree(server, node);
    return res;
}

/* The backend may leave fields of the FileInfo unset */
UA_StatusCode
backendGetInfo(UA_FileTransferFileBackend *b, const UA_String path,
               UA_FileTransferFileInfo *info) {
    memset(info, 0, sizeof(UA_FileTransferFileInfo));
    return b->getInfo(b, path, info);
}

/* Close only this instance's handles. */
void
closeDriverHandles(UA_Server *server, FileTransferDriver *ftd) {
    FTHandle *h;
    while((h = ZIP_ROOT(&ftd->handlesById)))
        closeFTHandle(server, h);
}

void
closeSessionHandles(UA_Server *server, FileTransferDriver *ftd,
                     const UA_NodeId *sessionId) {
    FTHandle *h;
    /* Removal rebalances both indexes; find again instead of mutating a tree
     * during ZIP_ITER_KEY. Only this Session's handles are visited. */
    while((h = ZIP_FIND(FTHandlesBySession, &ftd->handlesBySession, sessionId)))
        closeFTHandle(server, h);
}

/**************************************
 * FileType Method Callbacks
 **************************************/

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
openFileHandle(UA_Server *server, FTEntry *node,
               const UA_NodeId *sessionId, void *sessionContext, UA_Byte mode,
               UA_UInt32 *outHandle) {
    FileTransferDriver *ftd = node->driver;
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
    if(writeBit && node->subtreeHandleCount > 0)
        return UA_STATUSCODE_BADNOTWRITABLE;
    if((mode & UA_OPENFILEMODE_READ) && node->openForWrite)
        return UA_STATUSCODE_BADNOTREADABLE;

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(writeBit)
        res = checkFTAccess(server, node, sessionId, sessionContext,
                            UA_FILEACCESS_WRITE,
                            UA_STATUSCODE_BADNOTWRITABLE);
    if(res == UA_STATUSCODE_GOOD && (mode & UA_OPENFILEMODE_READ))
        res = checkFTAccess(server, node, sessionId, sessionContext,
                            UA_FILEACCESS_READ,
                            UA_STATUSCODE_BADNOTREADABLE);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Resource limits */
    if(node->subtreeHandleCount >= ftd->config.maxHandlesPerFile ||
       countSessionHandles(ftd, sessionId) >= ftd->config.maxHandlesPerSession)
        return UA_STATUSCODE_BADRESOURCEUNAVAILABLE;

    UA_FileTransferFileBackend *b = &node->driver->backend.file;
    UA_UInt32 backendHandle = 0;
    res = b->open(b, node->path, mode, &backendHandle);
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
    ZIP_INSERT(FTHandlesById, &ftd->handlesById, h);
    ZIP_INSERT(FTHandlesBySession, &ftd->handlesBySession, h);

    for(FTEntry *entry = node; entry; entry = entry->parent)
        entry->subtreeHandleCount++;
    if(writeBit)
        node->openForWrite = true;

    *outHandle = h->handle;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
openMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                   void *sessionContext, const UA_NodeId *methodId,
                   void *methodContext, const UA_NodeId *objectId,
                   void *objectContext, size_t inputSize, const UA_Variant *input,
                   size_t outputSize, UA_Variant *output) {
    FTEntry *node = resolveFTEntry(server, objectId, objectContext);
    if(!node)
        return UA_STATUSCODE_BADNOTSUPPORTED;
    FileTransferDriver *ftd = node->driver;

    if(node->isDirectory || node->zombie)
        return UA_STATUSCODE_BADNOTFOUND;
    UA_StatusCode res;

    if(inputSize < 1 || outputSize < 1 ||
       !UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_BYTE]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    /* Open takes a Byte bit mask, not the generated 32-bit UA_OpenFileMode. */
    UA_Byte mode = *(UA_Byte*)input[0].data;

    UA_UInt32 handle = 0;
    res = openFileHandle(server, node, sessionId, sessionContext, mode, &handle);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* The client cannot close a handle it does not receive */
    res = UA_Variant_setScalarCopy(&output[0], &handle,
                                   &UA_TYPES[UA_TYPES_UINT32]);
    if(res != UA_STATUSCODE_GOOD) {
        FTHandle *h = findFTHandle(ftd, sessionId, handle);
        if(h)
            closeFTHandle(server, h);
    }
    return res;
}

static UA_StatusCode
closeMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                    void *sessionContext, const UA_NodeId *methodId,
                    void *methodContext, const UA_NodeId *objectId,
                    void *objectContext, size_t inputSize, const UA_Variant *input,
                    size_t outputSize, UA_Variant *output) {
    FTEntry *node = resolveFTEntry(server, objectId, objectContext);
    if(!node)
        return UA_STATUSCODE_BADNOTSUPPORTED;
    FileTransferDriver *ftd = node->driver;

    if(inputSize < 1)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    FTHandle *h = NULL;
    UA_StatusCode res = resolveHandle(ftd, sessionId, objectId, &input[0], &h);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    return closeFTHandle(server, h);
}

static UA_StatusCode
readMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                   void *sessionContext, const UA_NodeId *methodId,
                   void *methodContext, const UA_NodeId *objectId,
                   void *objectContext, size_t inputSize, const UA_Variant *input,
                   size_t outputSize, UA_Variant *output) {
    FTEntry *node = resolveFTEntry(server, objectId, objectContext);
    if(!node)
        return UA_STATUSCODE_BADNOTSUPPORTED;
    FileTransferDriver *ftd = node->driver;

    if(inputSize < 2 || outputSize < 1)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    FTHandle *h = NULL;
    UA_StatusCode res = resolveHandle(ftd, sessionId, objectId, &input[0], &h);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(!(h->mode & UA_OPENFILEMODE_READ))
        return UA_STATUSCODE_BADINVALIDSTATE;

    res = checkFTAccess(server, h->file, sessionId, sessionContext,
                        UA_FILEACCESS_READ,
                        UA_STATUSCODE_BADNOTREADABLE);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    if(!UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_INT32]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_Int32 length = *(UA_Int32*)input[1].data;
    if(length <= 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* The Server is allowed to return less data than the requested length */
    if((UA_UInt32)length > ftd->config.maxReadLength)
        length = (UA_Int32)ftd->config.maxReadLength;

    UA_ByteString *data = UA_ByteString_new();
    if(!data)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_FileTransferFileBackend *b = &h->file->driver->backend.file;
    res = b->read(b, h->backendHandle, length, data);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ByteString_delete(data);
        return res;
    }

    UA_Variant_setScalar(&output[0], data, &UA_TYPES[UA_TYPES_BYTESTRING]);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                    void *sessionContext, const UA_NodeId *methodId,
                    void *methodContext, const UA_NodeId *objectId,
                    void *objectContext, size_t inputSize, const UA_Variant *input,
                    size_t outputSize, UA_Variant *output) {
    FTEntry *node = resolveFTEntry(server, objectId, objectContext);
    if(!node)
        return UA_STATUSCODE_BADNOTSUPPORTED;
    FileTransferDriver *ftd = node->driver;

    if(inputSize < 2)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    FTHandle *h = NULL;
    UA_StatusCode res = resolveHandle(ftd, sessionId, objectId, &input[0], &h);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(!(h->mode & UA_OPENFILEMODE_WRITE))
        return UA_STATUSCODE_BADINVALIDSTATE;

    res = checkFTAccess(server, h->file, sessionId, sessionContext,
                        UA_FILEACCESS_WRITE,
                        UA_STATUSCODE_BADNOTWRITABLE);
    if(res != UA_STATUSCODE_GOOD)
        return res;

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
    if(data.length > ftd->config.maxReadLength)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_FileTransferFileBackend *b = &h->file->driver->backend.file;
    return b->write(b, h->backendHandle, data);
}

static UA_StatusCode
getPositionMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                          void *sessionContext, const UA_NodeId *methodId,
                          void *methodContext, const UA_NodeId *objectId,
                          void *objectContext, size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output) {
    FTEntry *node = resolveFTEntry(server, objectId, objectContext);
    if(!node)
        return UA_STATUSCODE_BADNOTSUPPORTED;
    FileTransferDriver *ftd = node->driver;

    if(inputSize < 1 || outputSize < 1)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    FTHandle *h = NULL;
    UA_StatusCode res = resolveHandle(ftd, sessionId, objectId, &input[0], &h);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_FileTransferFileBackend *b = &h->file->driver->backend.file;
    UA_UInt64 position = 0;
    res = b->getPosition(b, h->backendHandle, &position);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    return UA_Variant_setScalarCopy(&output[0], &position,
                                    &UA_TYPES[UA_TYPES_UINT64]);
}

static UA_StatusCode
setPositionMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                          void *sessionContext, const UA_NodeId *methodId,
                          void *methodContext, const UA_NodeId *objectId,
                          void *objectContext, size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output) {
    FTEntry *node = resolveFTEntry(server, objectId, objectContext);
    if(!node)
        return UA_STATUSCODE_BADNOTSUPPORTED;
    FileTransferDriver *ftd = node->driver;

    if(inputSize < 2)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    FTHandle *h = NULL;
    UA_StatusCode res = resolveHandle(ftd, sessionId, objectId, &input[0], &h);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    if(!UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_UINT64]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_UInt64 position = *(UA_UInt64*)input[1].data;

    UA_FileTransferFileBackend *b = &h->file->driver->backend.file;
    return b->setPosition(b, h->backendHandle, position);
}

const FTMethod fileTypeMethods[] = {
    {UA_NS0ID_FILETYPE_OPEN, "Open", openMethodCallback},
    {UA_NS0ID_FILETYPE_CLOSE, "Close", closeMethodCallback},
    {UA_NS0ID_FILETYPE_READ, "Read", readMethodCallback},
    {UA_NS0ID_FILETYPE_WRITE, "Write", writeMethodCallback},
    {UA_NS0ID_FILETYPE_GETPOSITION, "GetPosition", getPositionMethodCallback},
    {UA_NS0ID_FILETYPE_SETPOSITION, "SetPosition", setPositionMethodCallback}
};
