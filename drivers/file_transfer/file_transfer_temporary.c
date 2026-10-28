/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include "file_transfer_internal.h"
#include "mp_printf.h"

/**************************************
 * Transfer Files
 **************************************/

static UA_Boolean
isFileTypeMethod(const UA_NodeId *nodeId) {
    for(size_t i = 0; i < sizeof(fileTypeMethods) / sizeof(fileTypeMethods[0]); i++) {
        UA_NodeId methodId = UA_NODEID_NUMERIC(0, fileTypeMethods[i].methodId);
        if(UA_NodeId_equal(nodeId, &methodId))
            return true;
    }
    return false;
}

/* Only the generating Session reaches a temporary file, through the returned
 * NodeId and handle (Part 20, 4.4.1). Its Object has no parent, and the inverse
 * references that the instantiation added to shared nodes (the types and the
 * FileType Method declarations) are removed, so browsing a type does not
 * reveal it. The children of the file are handled the same way. */
static UA_StatusCode
hideTransferNode(UA_Server *server, const UA_NodeId nodeId, size_t depth) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = nodeId;
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.includeSubtypes = true;
    bd.resultMask = UA_BROWSERESULTMASK_REFERENCETYPEID;
    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
    UA_NodeId hasProperty = UA_NS0ID(HASPROPERTY);
    UA_NodeId hasComponent = UA_NS0ID(HASCOMPONENT);
    UA_StatusCode res = br.statusCode;
    while(res == UA_STATUSCODE_GOOD) {
        for(size_t i = 0; i < br.referencesSize && res == UA_STATUSCODE_GOOD; i++) {
            UA_ReferenceDescription *ref = &br.references[i];
            if(!UA_ExpandedNodeId_isLocal(&ref->nodeId))
                continue;
            const UA_NodeId *target = &ref->nodeId.nodeId;
            UA_Boolean child = (UA_NodeId_equal(&ref->referenceTypeId, &hasProperty) ||
                                UA_NodeId_equal(&ref->referenceTypeId, &hasComponent)) &&
                !isFileTypeMethod(target);
            if(child) {
                res = (depth < 4) ? hideTransferNode(server, *target, depth + 1) :
                    UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
                continue;
            }
            res = UA_Server_deleteReference(server, *target, ref->referenceTypeId, false,
                                             UA_EXPANDEDNODEID_NODEID(nodeId), false);
            /* An inverse reference already absent needs no further hiding. */
            if(res == UA_STATUSCODE_UNCERTAINREFERENCENOTDELETED)
                res = UA_STATUSCODE_GOOD;
        }
        if(res != UA_STATUSCODE_GOOD || br.continuationPoint.length == 0)
            break;
        /* maxReferences=0 still observes maxReferencesPerNode. Only the
         * targets' inverse references change, so this forward browse remains
         * valid while all continuation pages are consumed. */
        UA_BrowseResult next = UA_Server_browseNext(server, false, &br.continuationPoint);
        res = next.statusCode;
        if(res != UA_STATUSCODE_GOOD) {
            /* BrowseNext can fail without returning or releasing its point. */
            UA_BrowseResult_clear(&next);
            break;
        }
        UA_BrowseResult_clear(&br);
        br = next;
    }
    if(br.continuationPoint.length > 0) {
        UA_BrowseResult released = UA_Server_browseNext(server, true, &br.continuationPoint);
        UA_BrowseResult_clear(&released);
    }
    UA_BrowseResult_clear(&br);
    return res;
}

/* Create the store file. The name is also the BrowseName of the Object. */
static UA_StatusCode
createStoreFile(FileTransferDriver *ftd, UA_Boolean forWrite, char *name,
                size_t nameSize, UA_String *path) {
    UA_StatusCode res = UA_STATUSCODE_BADBROWSENAMEDUPLICATED;
    /* An application store can hold files of an earlier server run */
    for(size_t attempt = 0; attempt < 16 && res == UA_STATUSCODE_BADBROWSENAMEDUPLICATED;
        attempt++) {
        ftd->nextTransferId++;
        int length = mp_snprintf(name, nameSize, "%s-%u", forWrite ? "write" : "read",
                                 (unsigned)ftd->nextTransferId);
        if(length <= 0 || (size_t)length >= nameSize)
            return UA_STATUSCODE_BADINTERNALERROR;
        path->length = (size_t)length;
        path->data = (UA_Byte*)name;
        res = createBackendEntry(&ftd->backend, *path, false);
    }
    return res;
}

static UA_StatusCode
createTransferFile(UA_Server *server, FTEntry *root, const UA_NodeId *sessionId,
                   void *sessionContext, const UA_Variant *generateOptions,
                   UA_Boolean forWrite, FTEntry **outFile) {
    FileTransferDriver *ftd = root->driver;
    char name[32];
    UA_String path = UA_STRING_NULL;
    UA_StatusCode res = createStoreFile(ftd, forWrite, name, sizeof(name), &path);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_FileTransferTemporaryFile transfer = {sessionId, sessionContext, generateOptions,
                                             &ftd->backend.file, path, forWrite, NULL};
    UA_FileTransferTemporaryCallback generate = forWrite ?
        ftd->temporary.generateFileForWrite : ftd->temporary.generateFileForRead;
    if(generate)
        res = generate(server, ftd->temporary.context, &transfer);
    UA_Boolean generated = (generate && res == UA_STATUSCODE_GOOD);
    UA_FileTransferFileInfo info;
    if(res == UA_STATUSCODE_GOOD)
        res = backendGetInfo(&ftd->backend.file, path, &info);
    if(res == UA_STATUSCODE_GOOD && ftd->config.maxTransferSize > 0 &&
       info.size > ftd->config.maxTransferSize)
        res = UA_STATUSCODE_BADRESOURCEUNAVAILABLE;

    UA_NodeId nodeId = UA_NODEID_NULL;
    if(res == UA_STATUSCODE_GOOD) {
        UA_UInt16 nsIndex = ftd->config.namespaceIndex;
        UA_ObjectAttributes attr = UA_ObjectAttributes_default;
        attr.displayName.text = path;
        UA_QualifiedName browseName = {nsIndex, path};
        res = UA_Server_addObjectNode(server, UA_NODEID_GUID(nsIndex, UA_Guid_random()),
                                      UA_NODEID_NULL, UA_NODEID_NULL, browseName,
                                      UA_NS0ID(FILETYPE), attr, NULL, &nodeId);
    }
    FTEntry *file = NULL;
    if(res == UA_STATUSCODE_GOOD) {
        file = newFTEntry(ftd, NULL, nodeId, path, false);
        if(!file)
            res = UA_STATUSCODE_BADOUTOFMEMORY;
    }
    if(file) {
        file->kind = FT_ENTRY_TRANSFERFILE;
        file->created = true;
        file->transfer = (FTTransfer*)UA_calloc(1, sizeof(FTTransfer));
        if(!file->transfer)
            res = UA_STATUSCODE_BADOUTOFMEMORY;
    }
    if(res == UA_STATUSCODE_GOOD) {
        file->transfer->forWrite = forWrite;
        file->transfer->transferContext = transfer.transferContext;
        file->transfer->endStatus = forWrite ?
            UA_STATUSCODE_BADREQUESTCANCELLEDBYCLIENT : UA_STATUSCODE_GOOD;
        res = UA_NodeId_copy(sessionId, &file->transfer->sessionId);
        if(res == UA_STATUSCODE_GOOD && generateOptions)
            res = UA_Variant_copy(generateOptions, &file->transfer->generateOptions);
    }
    if(res == UA_STATUSCODE_GOOD)
        res = bindObjectContext(server, file);
    if(res == UA_STATUSCODE_GOOD)
        res = setupFileNode(server, file, &info);
    if(res == UA_STATUSCODE_GOOD)
        res = bindObjectMethods(server, file);
    if(res == UA_STATUSCODE_GOOD)
        res = hideTransferNode(server, nodeId, 0);

    if(res != UA_STATUSCODE_GOOD) {
        if(generated && ftd->temporary.endTransfer) {
            transfer.sessionContext = NULL;
            ftd->temporary.endTransfer(server, ftd->temporary.context, &transfer, res);
        }
        if(file)
            removeSubtree(server, file);
        else if(!UA_NodeId_isNull(&nodeId))
            UA_Server_deleteNode(server, nodeId, true);
        ftd->backend.remove(&ftd->backend, path);
        UA_NodeId_clear(&nodeId);
        return res;
    }
    UA_NodeId_clear(&nodeId);
    if(forWrite)
        ftd->activeWrite = true;
    else
        ftd->activeReads++;
    *outFile = file;
    return UA_STATUSCODE_GOOD;
}

void
removeTransferFile(UA_Server *server, FTEntry *file) {
    FileTransferDriver *ftd = file->driver;
    FTTransfer *t = file->transfer;
    if(ftd->temporary.endTransfer) {
        UA_FileTransferTemporaryFile transfer = {&t->sessionId, NULL, &t->generateOptions,
                                                 &ftd->backend.file, file->path,
                                                 t->forWrite, t->transferContext};
        ftd->temporary.endTransfer(server, ftd->temporary.context, &transfer,
                                   t->endStatus);
    }
    /* closeAndCommit may have moved the file to keep it */
    UA_StatusCode res = ftd->backend.remove(&ftd->backend, file->path);
    if(res != UA_STATUSCODE_GOOD && res != UA_STATUSCODE_BADNOTFOUND)
        UA_LOG_WARNING(UA_Server_getConfig(server)->logging, UA_LOGCATEGORY_SERVER,
                       "FileTransfer: Removing the transfer file \"%S\" failed with %s",
                       file->path, UA_StatusCode_name(res));
    if(t->forWrite)
        ftd->activeWrite = false;
    else if(ftd->activeReads > 0)
        ftd->activeReads--;
    removeSubtree(server, file);
}

/**************************************
 * Content of Temporary Files
 **************************************/

UA_StatusCode
UA_FileTransferTemporaryFile_setContent(UA_FileTransferTemporaryFile *file,
                                        const UA_ByteString content) {
    if(!file)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_FileTransferFileBackend *store = file->store;
    UA_UInt32 handle = 0;
    UA_StatusCode res = store->open(store, file->path,
                                    UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING,
                                    NULL, &handle);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(content.length > 0)
        res = store->write(store, handle, content);
    UA_StatusCode closeRes = store->close(store, handle);
    return (res != UA_STATUSCODE_GOOD) ? res : closeRes;
}

UA_StatusCode
UA_FileTransferTemporaryFile_getContent(UA_FileTransferTemporaryFile *file,
                                        UA_ByteString *content) {
    if(!file || !content)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_ByteString_init(content);
    UA_FileTransferFileBackend *store = file->store;
    UA_UInt32 handle = 0;
    UA_StatusCode res = store->open(store, file->path, UA_OPENFILEMODE_READ, NULL, &handle);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    /* An empty chunk is the end of the file */
    UA_ByteString chunk = UA_BYTESTRING_NULL;
    do {
        UA_ByteString_clear(&chunk);
        res = store->read(store, handle, UA_FILETRANSFER_COPYCHUNKSIZE, &chunk);
        if(res == UA_STATUSCODE_GOOD)
            res = UA_String_append(content, chunk);
    } while(res == UA_STATUSCODE_GOOD && chunk.length > 0);
    UA_ByteString_clear(&chunk);
    UA_StatusCode closeRes = store->close(store, handle);
    if(res == UA_STATUSCODE_GOOD)
        res = closeRes;
    if(res != UA_STATUSCODE_GOOD)
        UA_ByteString_clear(content);
    return res;
}

UA_StatusCode
UA_FileTransferTemporaryFile_copyFrom(UA_FileTransferTemporaryFile *file,
                                      const UA_String path) {
    if(!file || UA_String_equal(&path, &file->path))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    /* The store is a directory backend. The copy creates its target, so the
     * empty transfer file is removed first, once the source exists. */
    UA_FileTransferBackend *store = (UA_FileTransferBackend*)file->store;
    UA_FileTransferFileInfo info;
    UA_StatusCode res = backendGetInfo(&store->file, path, &info);
    if(res == UA_STATUSCODE_GOOD && info.isDirectory)
        res = UA_STATUSCODE_BADINVALIDARGUMENT;
    if(res == UA_STATUSCODE_GOOD)
        res = store->remove(store, file->path);
    if(res == UA_STATUSCODE_GOOD)
        res = copyStoreFile(store, path, file->path);
    return res;
}

/**************************************
 * TemporaryFileTransferType Methods
 **************************************/

/* Each transfer holds one handle. Before the generate callbacks run,
 * max-open-handles-per-file limits the transfers of the Object and
 * max-open-handles-per-session those of the Session. */
static UA_Boolean
transferLimitReached(FileTransferDriver *ftd, const UA_NodeId *sessionId) {
    UA_UInt32 transfers = ftd->activeReads + (ftd->activeWrite ? 1u : 0u);
    return transfers >= ftd->config.maxHandlesPerFile ||
        sessionHandleLimitReached(ftd, sessionId);
}

static FTEntry *
resolveTemporaryRoot(UA_Server *server, const UA_NodeId *objectId,
                     void *objectContext) {
    FTEntry *root = resolveFTEntry(server, objectId, objectContext);
    return (root && root->kind == FT_ENTRY_TEMPORARY) ? root : NULL;
}

/* Start a transfer and open its handle for the Session. A write transfer runs
 * alone (Part 20, 4.4.1). */
UA_StatusCode
generateTransfer(UA_Server *server, FTEntry *root, const UA_NodeId *sessionId,
                 void *sessionContext, const UA_Variant *generateOptions,
                 UA_Boolean forWrite, FTHandle **outHandle) {
    FileTransferDriver *ftd = root->driver;
    if(forWrite && (!ftd->temporary.closeAndCommit || ftd->config.readOnly ||
                    ftd->activeWrite || ftd->activeReads > 0))
        return UA_STATUSCODE_BADNOTWRITABLE;
    if(!forWrite && (!ftd->temporary.generateFileForRead || ftd->activeWrite))
        return UA_STATUSCODE_BADNOTREADABLE;
    if(transferLimitReached(ftd, sessionId))
        return UA_STATUSCODE_BADRESOURCEUNAVAILABLE;

    FTEntry *file = NULL;
    UA_StatusCode res = createTransferFile(server, root, sessionId, sessionContext,
                                           generateOptions, forWrite, &file);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = openFileHandle(server, file, sessionId, sessionContext,
                         forWrite ? UA_OPENFILEMODE_WRITE : UA_OPENFILEMODE_READ,
                         NULL, outHandle);
    if(res != UA_STATUSCODE_GOOD) {
        file->transfer->endStatus = res;
        removeTransferFile(server, file);
    }
    return res;
}

/* GenerateFileForRead and GenerateFileForWrite return fileNodeId and
 * fileHandle, GenerateFileForRead also a null completionStateMachine. */
static UA_StatusCode
generateFileMethod(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                   const UA_NodeId *objectId, void *objectContext, size_t inputSize,
                   const UA_Variant *input, size_t outputSize, UA_Variant *output,
                   UA_Boolean forWrite) {
    FTEntry *root = resolveTemporaryRoot(server, objectId, objectContext);
    if(!root)
        return UA_STATUSCODE_BADNOTSUPPORTED;
    if(inputSize < 1 || outputSize < (forWrite ? 2u : 3u))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    FTHandle *h = NULL;
    UA_StatusCode res = generateTransfer(server, root, sessionId, sessionContext,
                                         &input[0], forWrite, &h);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = UA_Variant_setScalarCopy(&output[0], &h->file->nodeId, &UA_TYPES[UA_TYPES_NODEID]);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_Variant_setScalarCopy(&output[1], &h->handle, &UA_TYPES[UA_TYPES_UINT32]);
    if(res == UA_STATUSCODE_GOOD && !forWrite) {
        UA_NodeId none = UA_NODEID_NULL;
        res = UA_Variant_setScalarCopy(&output[2], &none, &UA_TYPES[UA_TYPES_NODEID]);
    }
    /* The client cannot end a transfer whose handle it does not receive */
    if(res != UA_STATUSCODE_GOOD) {
        h->file->transfer->endStatus = res;
        closeFTHandle(server, h);
    }
    return res;
}

static UA_StatusCode
generateFileForReadMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                                  void *sessionContext, const UA_NodeId *methodId,
                                  void *methodContext, const UA_NodeId *objectId,
                                  void *objectContext, size_t inputSize,
                                  const UA_Variant *input, size_t outputSize,
                                  UA_Variant *output) {
    return generateFileMethod(server, sessionId, sessionContext, objectId,
                              objectContext, inputSize, input, outputSize, output, false);
}

static UA_StatusCode
generateFileForWriteMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                                   void *sessionContext, const UA_NodeId *methodId,
                                   void *methodContext, const UA_NodeId *objectId,
                                   void *objectContext, size_t inputSize,
                                   const UA_Variant *input, size_t outputSize,
                                   UA_Variant *output) {
    return generateFileMethod(server, sessionId, sessionContext, objectId,
                              objectContext, inputSize, input, outputSize, output, true);
}

static UA_StatusCode
closeAndCommitMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                             void *sessionContext, const UA_NodeId *methodId,
                             void *methodContext, const UA_NodeId *objectId,
                             void *objectContext, size_t inputSize,
                             const UA_Variant *input, size_t outputSize,
                             UA_Variant *output) {
    FTEntry *root = resolveTemporaryRoot(server, objectId, objectContext);
    if(!root)
        return UA_STATUSCODE_BADNOTSUPPORTED;
    if(inputSize < 1 || outputSize < 1 ||
       !UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_UINT32]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    FileTransferDriver *ftd = root->driver;
    FTHandle *h = findFTHandle(ftd, sessionId, *(UA_UInt32*)input[0].data);
    if(!h || !h->file->transfer || !h->file->transfer->forWrite)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* The file outlives its handle until closeAndCommit has run */
    FTEntry *file = h->file;
    file->transfer->committing = true;
    UA_StatusCode res = closeFTHandle(server, h);
    UA_FileTransferTemporaryFile transfer = {sessionId, sessionContext,
                                             &file->transfer->generateOptions,
                                             &ftd->backend.file, file->path, true,
                                             file->transfer->transferContext};
    if(res == UA_STATUSCODE_GOOD)
        res = ftd->temporary.closeAndCommit(server, ftd->temporary.context, &transfer);
    file->transfer->transferContext = transfer.transferContext;
    file->transfer->endStatus = res;
    removeTransferFile(server, file);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_NodeId none = UA_NODEID_NULL;
    return UA_Variant_setScalarCopy(&output[0], &none, &UA_TYPES[UA_TYPES_NODEID]);
}

const FTMethod temporaryFileTransferTypeMethods[] = {
    {UA_NS0ID_TEMPORARYFILETRANSFERTYPE_GENERATEFILEFORREAD, "GenerateFileForRead",
     generateFileForReadMethodCallback},
    {UA_NS0ID_TEMPORARYFILETRANSFERTYPE_GENERATEFILEFORWRITE, "GenerateFileForWrite",
     generateFileForWriteMethodCallback},
    {UA_NS0ID_TEMPORARYFILETRANSFERTYPE_CLOSEANDCOMMIT, "CloseAndCommit",
     closeAndCommitMethodCallback}
};
