/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include "file_transfer_internal.h"

/* Flat in-memory store for the files of temporary transfers. Files and open
 * handles live in tables that grow on demand. A handle is its index + 1 in the
 * handle table. Removing a file invalidates the handles that still use it. */

typedef struct {
    UA_String name;
    UA_ByteString content;
    UA_DateTime lastModified;
} MemStoreFile;

typedef struct {
    MemStoreFile *file; /* NULL for a free slot */
    size_t position;
} MemStoreHandle;

typedef struct {
    MemStoreFile **files; /* NULL for a free slot */
    size_t filesSize;
    MemStoreHandle *handles;
    size_t handlesSize;
} MemStore;

static UA_StatusCode
memStoreGrow(void **table, size_t *size, size_t elementSize) {
    size_t newSize = (*size > 0) ? *size * 2 : 8;
    if(newSize > UA_UINT32_MAX || newSize > SIZE_MAX / elementSize)
        return UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
    void *grown = UA_realloc(*table, newSize * elementSize);
    if(!grown)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memset((char*)grown + (*size * elementSize), 0, (newSize - *size) * elementSize);
    *table = grown;
    *size = newSize;
    return UA_STATUSCODE_GOOD;
}

static MemStoreFile *
memStoreFind(MemStore *store, const UA_String name, size_t *index) {
    for(size_t i = 0; i < store->filesSize; i++) {
        if(store->files[i] && UA_String_equal(&store->files[i]->name, &name)) {
            if(index)
                *index = i;
            return store->files[i];
        }
    }
    return NULL;
}

static MemStoreHandle *
memStoreHandle(UA_FileTransferFileBackend *b, UA_UInt32 handle) {
    MemStore *store = (MemStore*)b->context;
    if(handle == 0 || handle > store->handlesSize || !store->handles[handle - 1].file)
        return NULL;
    return &store->handles[handle - 1];
}

static UA_StatusCode
memStoreOpen(UA_FileTransferFileBackend *b, const UA_String path, UA_Byte mode,
             const UA_FileTransferOpenContext *context, UA_UInt32 *handle) {
    MemStore *store = (MemStore*)b->context;
    MemStoreFile *file = memStoreFind(store, path, NULL);
    if(!file)
        return UA_STATUSCODE_BADNOTFOUND;
    size_t i = 0;
    while(i < store->handlesSize && store->handles[i].file)
        i++;
    if(i == store->handlesSize) {
        UA_StatusCode res = memStoreGrow((void**)&store->handles, &store->handlesSize,
                                         sizeof(MemStoreHandle));
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }
    if(mode & UA_OPENFILEMODE_ERASEEXISTING) {
        UA_ByteString_clear(&file->content);
        file->lastModified = UA_DateTime_now();
    }
    store->handles[i].file = file;
    store->handles[i].position =
        (mode & UA_OPENFILEMODE_APPEND) ? file->content.length : 0;
    *handle = (UA_UInt32)(i + 1);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memStoreClose(UA_FileTransferFileBackend *b, UA_UInt32 handle) {
    MemStoreHandle *h = memStoreHandle(b, handle);
    if(!h)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    h->file = NULL;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memStoreRead(UA_FileTransferFileBackend *b, UA_UInt32 handle, UA_Int32 length,
             UA_ByteString *out) {
    MemStoreHandle *h = memStoreHandle(b, handle);
    if(!h || length <= 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    size_t remaining = (h->position < h->file->content.length) ?
        h->file->content.length - h->position : 0;
    size_t toRead = ((size_t)length < remaining) ? (size_t)length : remaining;
    UA_ByteString_init(out);
    if(toRead == 0)
        return UA_STATUSCODE_GOOD; /* The end of the file */
    UA_StatusCode res = UA_ByteString_allocBuffer(out, toRead);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    memcpy(out->data, h->file->content.data + h->position, toRead);
    h->position += toRead;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memStoreWrite(UA_FileTransferFileBackend *b, UA_UInt32 handle,
              const UA_ByteString data) {
    MemStoreHandle *h = memStoreHandle(b, handle);
    if(!h)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(data.length == 0)
        return UA_STATUSCODE_GOOD;
    if(data.length > SIZE_MAX - h->position)
        return UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
    MemStoreFile *file = h->file;
    size_t end = h->position + data.length;
    if(end > file->content.length) {
        UA_Byte *grown = (UA_Byte*)UA_realloc(file->content.data, end);
        if(!grown)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        memset(grown + file->content.length, 0, end - file->content.length);
        file->content.data = grown;
        file->content.length = end;
    }
    memcpy(file->content.data + h->position, data.data, data.length);
    h->position = end;
    file->lastModified = UA_DateTime_now();
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memStoreGetPosition(UA_FileTransferFileBackend *b, UA_UInt32 handle,
                    UA_UInt64 *outPosition) {
    MemStoreHandle *h = memStoreHandle(b, handle);
    if(!h)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    *outPosition = h->position;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memStoreSetPosition(UA_FileTransferFileBackend *b, UA_UInt32 handle,
                    UA_UInt64 position) {
    MemStoreHandle *h = memStoreHandle(b, handle);
    if(!h)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    h->position = (position < h->file->content.length) ?
        (size_t)position : h->file->content.length;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memStoreGetInfo(UA_FileTransferFileBackend *b, const UA_String path,
                UA_FileTransferFileInfo *outInfo) {
    if(path.length == 0) { /* The store root */
        outInfo->isDirectory = true;
        outInfo->accessRights = UA_FILEACCESS_READ | UA_FILEACCESS_WRITE |
                                UA_FILEACCESS_TRAVERSE;
        return UA_STATUSCODE_GOOD;
    }
    MemStoreFile *file = memStoreFind((MemStore*)b->context, path, NULL);
    if(!file)
        return UA_STATUSCODE_BADNOTFOUND;
    memcpy(outInfo->name, file->name.data, file->name.length);
    outInfo->name[file->name.length] = 0;
    outInfo->size = file->content.length;
    outInfo->lastModified = file->lastModified;
    outInfo->accessRights = UA_FILEACCESS_READ | UA_FILEACCESS_WRITE;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memStoreCreate(UA_FileTransferBackend *b, const UA_String path,
               const UA_FileTransferFileInfo *info) {
    if(info->isDirectory)
        return UA_STATUSCODE_BADNOTSUPPORTED; /* Flat store */
    if(!validEntryName(path))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    MemStore *store = (MemStore*)b->file.context;
    if(memStoreFind(store, path, NULL))
        return UA_STATUSCODE_BADBROWSENAMEDUPLICATED;
    size_t i = 0;
    while(i < store->filesSize && store->files[i])
        i++;
    if(i == store->filesSize) {
        UA_StatusCode res = memStoreGrow((void**)&store->files, &store->filesSize,
                                         sizeof(MemStoreFile*));
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }
    MemStoreFile *file = (MemStoreFile*)UA_calloc(1, sizeof(MemStoreFile));
    if(!file)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_StatusCode res = UA_String_copy(&path, &file->name);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(file);
        return res;
    }
    file->lastModified = UA_DateTime_now();
    store->files[i] = file;
    return UA_STATUSCODE_GOOD;
}

static void
memStoreDelete(MemStore *store, size_t index) {
    MemStoreFile *file = store->files[index];
    for(size_t i = 0; i < store->handlesSize; i++) {
        if(store->handles[i].file == file)
            store->handles[i].file = NULL;
    }
    UA_String_clear(&file->name);
    UA_ByteString_clear(&file->content);
    UA_free(file);
    store->files[index] = NULL;
}

static UA_StatusCode
memStoreRemove(UA_FileTransferBackend *b, const UA_String path) {
    MemStore *store = (MemStore*)b->file.context;
    size_t index;
    if(!memStoreFind(store, path, &index))
        return UA_STATUSCODE_BADNOTFOUND;
    memStoreDelete(store, index);
    return UA_STATUSCODE_GOOD;
}

static void
memStoreClear(UA_FileTransferFileBackend *b) {
    MemStore *store = (MemStore*)b->context;
    if(!store)
        return;
    for(size_t i = 0; i < store->filesSize; i++) {
        if(store->files[i])
            memStoreDelete(store, i);
    }
    UA_free(store->files);
    UA_free(store->handles);
    UA_free(store);
    b->context = NULL;
}

UA_StatusCode
memStoreInit(UA_FileTransferBackend *out) {
    MemStore *store = (MemStore*)UA_calloc(1, sizeof(MemStore));
    if(!store)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memset(out, 0, sizeof(UA_FileTransferBackend));
    out->file.context = store;
    out->file.open = memStoreOpen;
    out->file.close = memStoreClose;
    out->file.read = memStoreRead;
    out->file.write = memStoreWrite;
    out->file.getPosition = memStoreGetPosition;
    out->file.setPosition = memStoreSetPosition;
    out->file.getInfo = memStoreGetInfo;
    out->file.clear = memStoreClear;
    out->create = memStoreCreate;
    out->remove = memStoreRemove;
    return UA_STATUSCODE_GOOD;
}
