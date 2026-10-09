/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include <open62541/server.h>
#include <open62541/driver/file_transfer.h>
#include <open62541/server_config_default.h>
#include "test_helpers.h"
#include "testing_clock.h"
#include "ua_server_internal.h"
#include "../../drivers/file_transfer/file_transfer_internal.h"

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

UA_Server *server_ft;
static UA_Driver *ftDriver;
static void clearTestDrivers(void);

static void setup(void) {
    server_ft = UA_Server_newForUnitTest();
    ftDriver = NULL;
}

static void teardown(void) {
    clearTestDrivers();
    UA_Server_delete(server_ft);
}

/* Fixture bindings use one independently owned driver per backend. Keep the
 * root-to-driver association for assertions after a root has been removed. */
static struct {
    UA_NodeId root;
    UA_Driver *driver;
} testDrivers[128];
static size_t testDriversSize;
static UA_Boolean startTestDrivers = true;

static void
clearTestDrivers(void) {
    UA_Driver *drv = UA_Server_getDrivers(server_ft);
    while(drv) {
        UA_Driver *next = drv->next;
        if(!isFileTransferDriver(drv)) {
            drv = next;
            continue;
        }
        drv->stop(drv);
        ck_assert_uint_eq(UA_Server_removeDriver(server_ft, drv), UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(drv->free(drv), UA_STATUSCODE_GOOD);
        drv = next;
    }
    for(size_t i = 0; i < testDriversSize; i++)
        UA_NodeId_clear(&testDrivers[i].root);
    testDriversSize = 0;
    ftDriver = NULL;
    startTestDrivers = true;
}

static UA_Driver *
driverForRoot(UA_NodeId root) {
    for(size_t i = testDriversSize; i > 0; i--) {
        if(UA_NodeId_equal(&root, &testDrivers[i - 1].root))
            return testDrivers[i - 1].driver;
    }
    return ftDriver;
}

static void
registerTestDriver(UA_Driver *driver) {
    ck_assert_ptr_nonnull(driver);
    ck_assert_uint_eq(UA_Server_addDriver(server_ft, driver), UA_STATUSCODE_GOOD);
    if(startTestDrivers)
        ck_assert_uint_eq(driver->start(driver), UA_STATUSCODE_GOOD);
}

static void
setTestOptions(UA_Driver *driver, const FTConfig *options) {
    if(!options)
        return;
    UA_KeyValueMap *params = &driver->params;
    ck_assert_uint_eq(UA_KeyValueMap_setScalar(params, UA_QUALIFIEDNAME(0, "read-only"),
                          &options->readOnly, &UA_TYPES[UA_TYPES_BOOLEAN]), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_KeyValueMap_setScalar(params, UA_QUALIFIEDNAME(0, "max-nodes"),
                          &options->maxNodes, &UA_TYPES[UA_TYPES_UINT32]), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_KeyValueMap_setScalar(params, UA_QUALIFIEDNAME(0, "max-scan-depth"),
                          &options->maxScanDepth, &UA_TYPES[UA_TYPES_UINT32]), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_KeyValueMap_setScalar(params, UA_QUALIFIEDNAME(0, "namespace-index"),
                          &options->namespaceIndex, &UA_TYPES[UA_TYPES_UINT16]), UA_STATUSCODE_GOOD);
}

static UA_StatusCode
recordTestBinding(UA_Driver *driver, UA_NodeId root, UA_StatusCode res) {
    if(res != UA_STATUSCODE_GOOD) {
        driver->stop(driver);
        ck_assert_uint_eq(UA_Server_removeDriver(server_ft, driver), UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(driver->free(driver), UA_STATUSCODE_GOOD);
        return res;
    }
    ck_assert_uint_lt(testDriversSize, 128);
    ck_assert_uint_eq(UA_NodeId_copy(&root, &testDrivers[testDriversSize].root),
                      UA_STATUSCODE_GOOD);
    testDrivers[testDriversSize++].driver = driver;
    ftDriver = driver;
    return res;
}

static UA_StatusCode
testRemove(UA_Driver *driver, UA_NodeId root) {
    if(!driver || !UA_NodeId_equal(&((FileTransferDriver*)driver)->root->nodeId, &root))
        return UA_STATUSCODE_BADNOTFOUND;
    driver->stop(driver);
    UA_StatusCode res = UA_Server_removeDriver(driver->server, driver);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    for(size_t i = 0; i < testDriversSize; i++) {
        if(testDrivers[i].driver == driver)
            testDrivers[i].driver = NULL;
    }
    if(ftDriver == driver)
        ftDriver = NULL;
    return driver->free(driver);
}

/* Keep tests focused on storage behavior; construction and start are separate. */
static UA_StatusCode
testAddDirectory(UA_NodeId requested, UA_NodeId parent, UA_QualifiedName name,
                  const UA_FileTransferBackend *backend,
                  const FTConfig *options, UA_NodeId *out) {
    UA_Driver *driver = NULL;
    UA_NodeId root = UA_NODEID_NULL;
    if(name.name.length == 0)
        name = UA_QUALIFIEDNAME(0, "FileSystem");
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName.text = name.name;
    UA_StatusCode res = UA_FileTransferDriver_newDirectory(server_ft, backend,
        &(UA_FileTransferNodeDescription){
            .nodeId = requested,
            .parentNodeId = parent,
            .referenceTypeId = UA_NS0ID(HASCOMPONENT),
            .browseName = name,
            .typeDefinition = UA_NS0ID(FILEDIRECTORYTYPE),
            .attributes = attr}, &root, &driver);
    if(res != UA_STATUSCODE_GOOD) {
        if(backend->file.clear) {
            UA_FileTransferFileBackend copy = backend->file;
            copy.clear(&copy);
        }
        return res;
    }
    setTestOptions(driver, options);
    ck_assert_uint_eq(UA_Server_addDriver(server_ft, driver), UA_STATUSCODE_GOOD);
    if(startTestDrivers)
        res = driver->start(driver);
    recordTestBinding(driver, root, res);
    if(res != UA_STATUSCODE_GOOD || !out)
        UA_NodeId_clear(&root);
    else
        *out = root;
    return res;
}

static UA_StatusCode
testAddFile(UA_NodeId requested, UA_NodeId parent, UA_QualifiedName name,
            const UA_FileTransferFileBackend *backend, UA_String path,
            const FTConfig *options, UA_NodeId *out) {
    UA_Driver *driver = NULL;
    UA_NodeId root = UA_NODEID_NULL;
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName.text = name.name;
    UA_StatusCode res = UA_FileTransferDriver_newFile(server_ft, backend, path,
        &(UA_FileTransferNodeDescription){
            .nodeId = requested,
            .parentNodeId = parent,
            .referenceTypeId = UA_NS0ID(HASCOMPONENT),
            .browseName = name,
            .typeDefinition = UA_NS0ID(FILETYPE),
            .attributes = attr}, &root, &driver);
    if(res != UA_STATUSCODE_GOOD) {
        if(backend->clear) {
            UA_FileTransferFileBackend copy = *backend;
            copy.clear(&copy);
        }
        return res;
    }
    setTestOptions(driver, options);
    ck_assert_uint_eq(UA_Server_addDriver(server_ft, driver), UA_STATUSCODE_GOOD);
    if(startTestDrivers)
        res = driver->start(driver);
    recordTestBinding(driver, root, res);
    if(res != UA_STATUSCODE_GOOD || !out)
        UA_NodeId_clear(&root);
    else
        *out = root;
    return res;
}

static UA_StatusCode
testRefresh(UA_Driver *driver, UA_NodeId root) {
    lockServer(driver->server);
    UA_StatusCode res = fileTransferRefresh(driver, root);
    unlockServer(driver->server);
    return res;
}

static UA_Driver *
newTestFile(UA_Server *server, const UA_FileTransferFileBackend *backend,
            const char *name, UA_NodeId *out) {
    UA_Driver *driver = NULL;
    ck_assert_uint_eq(UA_FileTransferDriver_newFile(server, backend, UA_STRING("f.bin"),
        &(UA_FileTransferNodeDescription){
            .nodeId = UA_NODEID_NULL,
            .parentNodeId = UA_NS0ID(OBJECTSFOLDER),
            .referenceTypeId = UA_NS0ID(HASCOMPONENT),
            .browseName = UA_QUALIFIEDNAME(0, (char*)(uintptr_t)name),
            .typeDefinition = UA_NS0ID(FILETYPE),
            .attributes = UA_ObjectAttributes_default}, out, &driver), UA_STATUSCODE_GOOD);
    return driver;
}

static UA_Driver *
newTestDirectory(UA_Server *server, const UA_FileTransferBackend *backend,
                  UA_NodeId *out) {
    UA_Driver *driver = NULL;
    ck_assert_uint_eq(UA_FileTransferDriver_newDirectory(server, backend,
        &(UA_FileTransferNodeDescription){
            .nodeId = UA_NODEID_NULL,
            .parentNodeId = UA_NS0ID(OBJECTSFOLDER),
            .referenceTypeId = UA_NS0ID(HASCOMPONENT),
            .browseName = UA_QUALIFIEDNAME(0, "FileSystem"),
            .typeDefinition = UA_NS0ID(FILEDIRECTORYTYPE),
            .attributes = UA_ObjectAttributes_default}, out, &driver), UA_STATUSCODE_GOOD);
    return driver;
}

/* The driver copies the backend struct right away. Tests that build a backend
 * inline pass it through this (single-threaded) temporary. */
static UA_FileTransferBackend backendArgStorage;

static const UA_FileTransferBackend *
backendArg(UA_FileTransferBackend b) {
    backendArgStorage = b;
    return &backendArgStorage;
}

/**************************************
 * In-Memory Test Backend
 *
 * A minimal UA_FileTransferBackend implementation that stores the file
 * content on the heap. Used to test the backend contract and the driver
 * independently of the local filesystem.
 **************************************/

#define MEM_MAXENTRIES 64
#define MEM_MAXPATH 256
#define MEM_MAXOPEN 64

typedef struct {
    UA_Boolean used;
    UA_Boolean isDir;
    char path[MEM_MAXPATH];
    UA_ByteString content;
    UA_DateTime mtime;
} MemEntry;

/* An open file. The handle is the index + 1. */
typedef struct {
    MemEntry *entry;
    size_t pos;
} MemOpenFile;

typedef struct {
    MemEntry entries[MEM_MAXENTRIES];
    MemOpenFile open[MEM_MAXOPEN];
    void *accessPolicy;
    size_t listCalls;
} MemBackendContext;

static MemEntry *
memFind(MemBackendContext *ctx, const UA_String path) {
    for(size_t i = 0; i < MEM_MAXENTRIES; i++) {
        MemEntry *e = &ctx->entries[i];
        if(e->used && strlen(e->path) == path.length &&
           memcmp(e->path, path.data, path.length) == 0)
            return e;
    }
    return NULL;
}

static MemEntry *
memAdd(MemBackendContext *ctx, const UA_String path, UA_Boolean isDir) {
    if(path.length >= MEM_MAXPATH)
        return NULL;
    for(size_t i = 0; i < MEM_MAXENTRIES; i++) {
        MemEntry *e = &ctx->entries[i];
        if(e->used)
            continue;
        memset(e, 0, sizeof(MemEntry));
        e->used = true;
        e->isDir = isDir;
        memcpy(e->path, path.data, path.length);
        e->path[path.length] = 0;
        e->mtime = UA_DateTime_now();
        return e;
    }
    return NULL;
}

static MemOpenFile *
memHandle(UA_FileTransferFileBackend *b, UA_UInt32 handle) {
    MemBackendContext *ctx = (MemBackendContext*)b->context;
    if(handle == 0 || handle > MEM_MAXOPEN || !ctx->open[handle - 1].entry)
        return NULL;
    return &ctx->open[handle - 1];
}

static UA_StatusCode
memOpen(UA_FileTransferFileBackend *b, const UA_String path,
        UA_Byte mode, UA_UInt32 *handle) {
    MemBackendContext *ctx = (MemBackendContext*)b->context;
    MemEntry *e = memFind(ctx, path);
    if(!e || e->isDir)
        return UA_STATUSCODE_BADNOTFOUND;
    if(!(mode & (UA_OPENFILEMODE_READ | UA_OPENFILEMODE_WRITE)))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    size_t i = 0;
    while(i < MEM_MAXOPEN && ctx->open[i].entry)
        i++;
    if(i == MEM_MAXOPEN)
        return UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
    if(mode & UA_OPENFILEMODE_ERASEEXISTING) {
        UA_ByteString_clear(&e->content);
        e->mtime = UA_DateTime_now();
    }
    ctx->open[i].entry = e;
    ctx->open[i].pos = (mode & UA_OPENFILEMODE_APPEND) ? e->content.length : 0;
    *handle = (UA_UInt32)(i + 1);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memClose(UA_FileTransferFileBackend *b, UA_UInt32 handle) {
    MemOpenFile *of = memHandle(b, handle);
    if(!of)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    of->entry = NULL;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memRead(UA_FileTransferFileBackend *b, UA_UInt32 handle,
        UA_Int32 length, UA_ByteString *out) {
    MemOpenFile *of = memHandle(b, handle);
    if(!of || length <= 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    size_t remaining = (of->pos < of->entry->content.length) ?
        of->entry->content.length - of->pos : 0;
    size_t toRead = ((size_t)length < remaining) ? (size_t)length : remaining;
    if(toRead == 0) {
        UA_ByteString_init(out);
        return UA_STATUSCODE_GOOD;
    }
    UA_StatusCode res = UA_ByteString_allocBuffer(out, toRead);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    memcpy(out->data, of->entry->content.data + of->pos, toRead);
    of->pos += toRead;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memWrite(UA_FileTransferFileBackend *b, UA_UInt32 handle,
         const UA_ByteString data) {
    MemOpenFile *of = memHandle(b, handle);
    if(!of)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(data.length == 0)
        return UA_STATUSCODE_GOOD;
    MemEntry *e = of->entry;
    size_t newLength = of->pos + data.length;
    if(newLength > e->content.length) {
        UA_Byte *grown = (UA_Byte*)UA_realloc(e->content.data, newLength);
        if(!grown)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        if(of->pos > e->content.length)
            memset(grown + e->content.length, 0, of->pos - e->content.length);
        e->content.data = grown;
        e->content.length = newLength;
    }
    memcpy(e->content.data + of->pos, data.data, data.length);
    of->pos += data.length;
    e->mtime = UA_DateTime_now();
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memGetPosition(UA_FileTransferFileBackend *b, UA_UInt32 handle,
               UA_UInt64 *outPosition) {
    MemOpenFile *of = memHandle(b, handle);
    if(!of)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    *outPosition = of->pos;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memSetPosition(UA_FileTransferFileBackend *b, UA_UInt32 handle,
               UA_UInt64 position) {
    MemOpenFile *of = memHandle(b, handle);
    if(!of)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    of->pos = (position < of->entry->content.length) ?
        (size_t)position : of->entry->content.length;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memGetInfo(UA_FileTransferFileBackend *b, const UA_String path,
           UA_FileTransferFileInfo *outInfo) {
    MemBackendContext *ctx = (MemBackendContext*)b->context;
    MemEntry *e = memFind(ctx, path);
    if(!e) {
        /* The root directory always exists */
        if(path.length == 0) {
            memset(outInfo, 0, sizeof(UA_FileTransferFileInfo));
            outInfo->isDirectory = true;
            outInfo->accessRights = UA_FILEACCESS_READ | UA_FILEACCESS_WRITE |
                                    UA_FILEACCESS_TRAVERSE;
            return UA_STATUSCODE_GOOD;
        }
        return UA_STATUSCODE_BADNOTFOUND;
    }
    memset(outInfo, 0, sizeof(UA_FileTransferFileInfo));
    const char *name = strrchr(e->path, '/');
    strcpy(outInfo->name, name ? name + 1 : e->path);
    outInfo->size = e->content.length;
    outInfo->lastModified = e->mtime;
    outInfo->isDirectory = e->isDir;
    outInfo->accessRights = UA_FILEACCESS_READ | UA_FILEACCESS_WRITE;
    if(e->isDir)
        outInfo->accessRights |= UA_FILEACCESS_TRAVERSE;
    return UA_STATUSCODE_GOOD;
}

/* Is entryPath a direct child of dirPath? */
static UA_Boolean
memIsDirectChild(const char *entryPath, const UA_String dirPath) {
    size_t entryLen = strlen(entryPath);
    if(dirPath.length > 0) {
        if(entryLen <= dirPath.length + 1 ||
           memcmp(entryPath, dirPath.data, dirPath.length) != 0 ||
           entryPath[dirPath.length] != '/')
            return false;
        entryPath += dirPath.length + 1;
    }
    return strchr(entryPath, '/') == NULL;
}

static UA_StatusCode
memListDirectory(UA_FileTransferBackend *b, const UA_String path,
                 UA_FileTransferListCallback cb, void *listContext) {
    MemBackendContext *ctx = (MemBackendContext*)b->file.context;
    ctx->listCalls++;
    if(path.length > 0) {
        MemEntry *dir = memFind(ctx, path);
        if(!dir || !dir->isDir)
            return UA_STATUSCODE_BADNOTFOUND;
    }
    for(size_t i = 0; i < MEM_MAXENTRIES; i++) {
        MemEntry *e = &ctx->entries[i];
        if(!e->used || !memIsDirectChild(e->path, path))
            continue;
        UA_FileTransferFileInfo info;
        UA_StatusCode res = b->file.getInfo(&b->file, UA_STRING(e->path), &info);
        if(res != UA_STATUSCODE_GOOD)
            continue;
        cb(listContext, &info);
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memCreate(UA_FileTransferBackend *b, const UA_String path,
           const UA_FileTransferFileInfo *info) {
    UA_String name = path;
    for(size_t i = 0; i < path.length; i++) {
        if(path.data[i] == '/') {
            name.data = path.data + i + 1;
            name.length = path.length - i - 1;
        }
    }
    ck_assert_uint_eq(strlen(info->name), name.length);
    ck_assert_int_eq(memcmp(info->name, name.data, name.length), 0);
    ck_assert_uint_eq(info->size, 0);
    ck_assert_int_eq(info->lastModified, 0);
    ck_assert_uint_eq(info->accessRights, 0);
    ck_assert_str_eq(info->mimeType, "");
    MemBackendContext *ctx = (MemBackendContext*)b->file.context;
    if(memFind(ctx, path))
        return UA_STATUSCODE_BADBROWSENAMEDUPLICATED;
    if(!memAdd(ctx, path, info->isDirectory))
        return UA_STATUSCODE_BADOUTOFMEMORY;
    return UA_STATUSCODE_GOOD;
}

/* Construct the creation request for direct backend calls in the tests. */
static UA_StatusCode
createEntry(UA_FileTransferBackend *b, const UA_String path,
             UA_Boolean isDirectory) {
    UA_String name = path;
    for(size_t i = 0; i < path.length; i++) {
        if(path.data[i] == '/') {
            name.data = path.data + i + 1;
            name.length = path.length - i - 1;
        }
    }
    UA_FileTransferFileInfo info;
    memset(&info, 0, sizeof(info));
    ck_assert_uint_le(name.length, UA_FILETRANSFER_FILENAME_MAX);
    memcpy(info.name, name.data, name.length);
    info.isDirectory = isDirectory;
    return b->create(b, path, &info);
}

static UA_StatusCode
memRemove(UA_FileTransferBackend *b, const UA_String path) {
    MemBackendContext *ctx = (MemBackendContext*)b->file.context;
    MemEntry *e = memFind(ctx, path);
    if(!e)
        return UA_STATUSCODE_BADNOTFOUND;
    if(e->isDir) {
        /* Only empty directories can be removed */
        for(size_t i = 0; i < MEM_MAXENTRIES; i++) {
            MemEntry *child = &ctx->entries[i];
            if(child->used && child != e &&
               strncmp(child->path, e->path, strlen(e->path)) == 0 &&
               child->path[strlen(e->path)] == '/')
                return UA_STATUSCODE_BADINVALIDSTATE;
        }
    }
    UA_ByteString_clear(&e->content);
    e->used = false;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memRename(UA_FileTransferBackend *b, const UA_String fromPath,
          const UA_String toPath) {
    MemBackendContext *ctx = (MemBackendContext*)b->file.context;
    MemEntry *e = memFind(ctx, fromPath);
    if(!e)
        return UA_STATUSCODE_BADNOTFOUND;
    if(memFind(ctx, toPath))
        return UA_STATUSCODE_BADBROWSENAMEDUPLICATED;

    /* Rewrite the path prefix of the entry and all its descendants */
    for(size_t i = 0; i < MEM_MAXENTRIES; i++) {
        MemEntry *c = &ctx->entries[i];
        if(!c->used)
            continue;
        size_t cLen = strlen(c->path);
        UA_Boolean isSelf = (cLen == fromPath.length &&
            memcmp(c->path, fromPath.data, fromPath.length) == 0);
        UA_Boolean isChild = (cLen > fromPath.length + 1 &&
            memcmp(c->path, fromPath.data, fromPath.length) == 0 &&
            c->path[fromPath.length] == '/');
        if(!isSelf && !isChild)
            continue;
        char newPath[MEM_MAXPATH];
        int len = snprintf(newPath, MEM_MAXPATH, "%.*s%s",
                           (int)toPath.length, (char*)toPath.data,
                           c->path + fromPath.length);
        if(len < 0 || len >= MEM_MAXPATH)
            return UA_STATUSCODE_BADINVALIDARGUMENT;
        memcpy(c->path, newPath, (size_t)len + 1);
    }
    return UA_STATUSCODE_GOOD;
}

static void
memClear(UA_FileTransferFileBackend *b) {
    MemBackendContext *ctx = (MemBackendContext*)b->context;
    if(!ctx)
        return;
    for(size_t i = 0; i < MEM_MAXENTRIES; i++) {
        if(ctx->entries[i].used)
            UA_ByteString_clear(&ctx->entries[i].content);
    }
    UA_free(ctx);
    b->context = NULL;
}

static UA_StatusCode
memBackend(UA_FileTransferBackend *out) {
    MemBackendContext *ctx = (MemBackendContext*)
        UA_calloc(1, sizeof(MemBackendContext));
    if(!ctx)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memset(out, 0, sizeof(UA_FileTransferBackend));
    out->file.context = ctx;
    out->file.open = memOpen;
    out->file.close = memClose;
    out->file.read = memRead;
    out->file.write = memWrite;
    out->file.getPosition = memGetPosition;
    out->file.setPosition = memSetPosition;
    out->file.getInfo = memGetInfo;
    out->file.clear = memClear;
    out->listDirectory = memListDirectory;
    out->create = memCreate;
    out->remove = memRemove;
    out->rename = memRename;
    out->copy = NULL;
    return UA_STATUSCODE_GOOD;
}

/**************************************
 * Backend Contract Tests
 *
 * The same test body runs against every backend implementation.
 **************************************/

typedef struct {
    size_t count;
    char names[16][64];
    UA_Boolean isDir[16];
    UA_FileTransferFileInfo info[16];
} ListResult;

static void
listCollector(void *listContext, const UA_FileTransferFileInfo *info) {
    UA_String name = UA_STRING((char*)(uintptr_t)info->name);
    ListResult *lr = (ListResult*)listContext;
    if(lr->count >= 16 || name.length >= 64)
        return;
    memcpy(lr->names[lr->count], name.data, name.length);
    lr->names[lr->count][name.length] = 0;
    lr->isDir[lr->count] = info->isDirectory;
    lr->info[lr->count] = *info;
    lr->count++;
}

static UA_Boolean
listContains(const ListResult *lr, const char *name, UA_Boolean isDir) {
    for(size_t i = 0; i < lr->count; i++) {
        if(strcmp(lr->names[i], name) == 0 && lr->isDir[i] == isDir)
            return true;
    }
    return false;
}

static void
runBackendContract(UA_FileTransferBackend *b) {
    UA_FileTransferFileBackend *f = &b->file;
    UA_StatusCode res;
    UA_UInt32 fc = 0;
    UA_ByteString out;
    UA_UInt64 pos = 0;
    UA_FileTransferFileInfo info;

    /* Create a directory and a file */
    ck_assert_uint_eq(createEntry(b, UA_STRING("sub"), true), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(b, UA_STRING("hello.txt"), false), UA_STATUSCODE_GOOD);

    /* Duplicates are rejected */
    ck_assert_uint_eq(createEntry(b, UA_STRING("hello.txt"), false),
                      UA_STATUSCODE_BADBROWSENAMEDUPLICATED);
    ck_assert_uint_eq(createEntry(b, UA_STRING("sub"), true),
                      UA_STATUSCODE_BADBROWSENAMEDUPLICATED);

    /* Write content */
    res = f->open(f, UA_STRING("hello.txt"), UA_OPENFILEMODE_WRITE, &fc);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(f->write(f, fc, UA_BYTESTRING("Hello World")),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(f->getPosition(f, fc, &pos), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pos, 11);
    ck_assert_uint_eq(f->close(f, fc), UA_STATUSCODE_GOOD);

    /* Attributes */
    ck_assert_uint_eq(f->getInfo(f, UA_STRING("hello.txt"), &info),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(info.size, 11);
    ck_assert_str_eq(info.name, "hello.txt");
    ck_assert(!info.isDirectory);
    ck_assert_uint_eq(f->getInfo(f, UA_STRING("sub"), &info),
                      UA_STATUSCODE_GOOD);
    ck_assert(info.isDirectory);
    ck_assert_uint_eq(f->getInfo(f, UA_STRING("missing"), &info),
                      UA_STATUSCODE_BADNOTFOUND);

    /* Read in chunks until the end of the file */
    res = f->open(f, UA_STRING("hello.txt"), UA_OPENFILEMODE_READ, &fc);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(f->read(f, fc, 5, &out), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.length, 5);
    ck_assert_int_eq(memcmp(out.data, "Hello", 5), 0);
    UA_ByteString_clear(&out);
    ck_assert_uint_eq(f->getPosition(f, fc, &pos), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pos, 5);
    ck_assert_uint_eq(f->read(f, fc, 100, &out), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.length, 6);
    ck_assert_int_eq(memcmp(out.data, " World", 6), 0);
    UA_ByteString_clear(&out);
    ck_assert_uint_eq(f->read(f, fc, 10, &out), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.length, 0); /* EOF */
    UA_ByteString_clear(&out);

    /* Positioning with clamping */
    ck_assert_uint_eq(f->setPosition(f, fc, 6), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(f->read(f, fc, 5, &out), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.length, 5);
    ck_assert_int_eq(memcmp(out.data, "World", 5), 0);
    UA_ByteString_clear(&out);
    ck_assert_uint_eq(f->setPosition(f, fc, 1000), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(f->getPosition(f, fc, &pos), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pos, 11);
    ck_assert_uint_eq(f->close(f, fc), UA_STATUSCODE_GOOD);

    /* EraseExisting truncates */
    res = f->open(f, UA_STRING("hello.txt"),
                      UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING, &fc);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(f->write(f, fc, UA_BYTESTRING("Hi")), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(f->close(f, fc), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(f->getInfo(f, UA_STRING("hello.txt"), &info),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(info.size, 2);

    /* Append positions at the end */
    res = f->open(f, UA_STRING("hello.txt"),
                      UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_APPEND, &fc);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(f->getPosition(f, fc, &pos), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pos, 2);
    ck_assert_uint_eq(f->write(f, fc, UA_BYTESTRING("!")), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(f->close(f, fc), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(f->getInfo(f, UA_STRING("hello.txt"), &info),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(info.size, 3);

    /* Empty write is a no-op */
    res = f->open(f, UA_STRING("hello.txt"), UA_OPENFILEMODE_WRITE, &fc);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(f->write(f, fc, UA_BYTESTRING_NULL), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(f->close(f, fc), UA_STATUSCODE_GOOD);

    /* Invalid open modes and missing files */
    ck_assert_uint_eq(f->open(f, UA_STRING("hello.txt"), 0, &fc),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(f->open(f, UA_STRING("missing"),
                                  UA_OPENFILEMODE_READ, &fc),
                      UA_STATUSCODE_BADNOTFOUND);
    ck_assert_uint_eq(f->open(f, UA_STRING("missing"),
                                  UA_OPENFILEMODE_WRITE, &fc),
                      UA_STATUSCODE_BADNOTFOUND);

    /* Listing */
    ListResult lr;
    memset(&lr, 0, sizeof(lr));
    ck_assert_uint_eq(b->listDirectory(b, UA_STRING(""), listCollector, &lr),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(lr.count, 2);
    ck_assert(listContains(&lr, "sub", true));
    ck_assert(listContains(&lr, "hello.txt", false));
    for(size_t i = 0; i < lr.count; i++) {
        ck_assert_uint_eq(f->getInfo(f, UA_STRING(lr.names[i]), &info),
                          UA_STATUSCODE_GOOD);
        ck_assert_str_eq(lr.info[i].name, info.name);
        ck_assert_uint_eq(lr.info[i].size, info.size);
        ck_assert_int_eq(lr.info[i].lastModified, info.lastModified);
        ck_assert_uint_eq(lr.info[i].accessRights, info.accessRights);
        ck_assert_int_eq(memcmp(lr.info[i].mimeType, info.mimeType,
                                sizeof(info.mimeType)), 0);
    }

    /* Rename/move */
    ck_assert_uint_eq(b->rename(b, UA_STRING("hello.txt"),
                                UA_STRING("sub/hello2.txt")),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(f->getInfo(f, UA_STRING("hello.txt"), &info),
                      UA_STATUSCODE_BADNOTFOUND);
    ck_assert_uint_eq(f->getInfo(f, UA_STRING("sub/hello2.txt"), &info),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(info.size, 3);
    ck_assert_str_eq(info.name, "hello2.txt");

    /* Remove */
    ck_assert_uint_eq(b->remove(b, UA_STRING("sub/hello2.txt")),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b->remove(b, UA_STRING("sub")), UA_STATUSCODE_GOOD);
    memset(&lr, 0, sizeof(lr));
    ck_assert_uint_eq(b->listDirectory(b, UA_STRING(""), listCollector, &lr),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(lr.count, 0);
    ck_assert_uint_eq(b->remove(b, UA_STRING("missing")),
                      UA_STATUSCODE_BADNOTFOUND);
}

START_TEST(memoryBackendContract) {
    UA_FileTransferBackend b;
    ck_assert_uint_eq(memBackend(&b), UA_STATUSCODE_GOOD);
    runBackendContract(&b);
    b.file.clear(&b.file);
} END_TEST

/**************************************
 * Local Filesystem Test Helpers
 **************************************/

#include <sys/stat.h>
#ifdef _WIN32
# include <direct.h>
# include <io.h>
# include <wchar.h>
# include <windows.h>
# include <winioctl.h>
#else
# include <dirent.h>
# include <unistd.h>
#endif

static char scratchDir[128];

#ifdef _WIN32
/* The scratch directories are created below the working directory. The
 * content is handled with the wide-character API, like in the backend. */
static void
makeScratchDir(void) {
    static unsigned counter = 0;
    snprintf(scratchDir, sizeof(scratchDir), "check_filetransfer_%lu_%u",
             (unsigned long)GetCurrentProcessId(), counter++);
    ck_assert_int_eq(_mkdir(scratchDir), 0);
}

static void
widePath(const char *path, wchar_t *out) {
    ck_assert_int_gt(MultiByteToWideChar(CP_UTF8, 0, path, -1, out, MAX_PATH), 0);
}

static void
removeTreeW(const wchar_t *path) {
    wchar_t pattern[MAX_PATH];
    wcscpy(pattern, path);
    wcscat(pattern, L"\\*");
    struct _wfinddata_t fd;
    intptr_t h = _wfindfirst(pattern, &fd);
    if(h != -1) {
        do {
            if(wcscmp(fd.name, L".") == 0 || wcscmp(fd.name, L"..") == 0)
                continue;
            wchar_t child[MAX_PATH];
            wcscpy(child, path);
            wcscat(child, L"\\");
            wcscat(child, fd.name);
            if(fd.attrib & _A_SUBDIR)
                removeTreeW(child);
            else
                _wremove(child);
        } while(_wfindnext(h, &fd) == 0);
        _findclose(h);
    }
    _wrmdir(path);
}

static void
removeTree(const char *path) {
    wchar_t wpath[MAX_PATH];
    widePath(path, wpath);
    removeTreeW(wpath);
}
#else
static void
makeScratchDir(void) {
    strcpy(scratchDir, "/tmp/check_filetransfer_XXXXXX");
    ck_assert_ptr_nonnull(mkdtemp(scratchDir));
}

static void
removeTree(const char *path) {
    /* A symbolic link is removed itself, its target is not descended into */
    struct stat st;
    DIR *dir = (lstat(path, &st) == 0 && S_ISDIR(st.st_mode)) ?
        opendir(path) : NULL;
    if(dir) {
        struct dirent *entry;
        while((entry = readdir(dir)) != NULL) {
            if(strcmp(entry->d_name, ".") == 0 ||
               strcmp(entry->d_name, "..") == 0)
                continue;
            char child[512];
            snprintf(child, sizeof(child), "%s/%s", path, entry->d_name);
            removeTree(child);
        }
        closedir(dir);
        rmdir(path);
    } else {
        unlink(path);
    }
}

#endif

START_TEST(localDirectoryBackendContract) {
    makeScratchDir();
    UA_FileTransferBackend b;
    ck_assert_uint_eq(UA_FileTransferBackend_localDirectory(
                          UA_STRING(scratchDir), &b),
                      UA_STATUSCODE_GOOD);
    runBackendContract(&b);
    b.file.clear(&b.file);
    removeTree(scratchDir);
} END_TEST

START_TEST(localDirectoryBackendSandbox) {
    makeScratchDir();
    UA_FileTransferBackend b;
    ck_assert_uint_eq(UA_FileTransferBackend_localDirectory(
                          UA_STRING(scratchDir), &b),
                      UA_STATUSCODE_GOOD);

    /* Paths that could escape the root or are malformed are rejected */
    const char *badPaths[8] = {"..", "../escape", "a/../../b", "/absolute",
                               "a//b", "a/./b", "trailing/", "back\\slash"};
    UA_UInt32 fc = 0;
    for(size_t i = 0; i < 8; i++) {
        UA_String p = UA_STRING((char*)(uintptr_t)badPaths[i]);
        ck_assert_uint_eq(b.file.open(&b.file, p, UA_OPENFILEMODE_READ, &fc),
                          UA_STATUSCODE_BADINVALIDARGUMENT);
        ck_assert_uint_eq(createEntry(&b, p, false), UA_STATUSCODE_BADINVALIDARGUMENT);
        ck_assert_uint_eq(createEntry(&b, p, true),
                          UA_STATUSCODE_BADINVALIDARGUMENT);
        ck_assert_uint_eq(b.remove(&b, p), UA_STATUSCODE_BADINVALIDARGUMENT);
    }

#ifdef _WIN32
    /* Names Windows cannot store as given */
    const char *windowsNames[9] = {"CON", "nul.txt", "com1", "LPT9.log", "a:b",
                                   "x?", "trail.", "trail ", "sub/AUX"};
    for(size_t i = 0; i < 9; i++) {
        UA_String p = UA_STRING((char*)(uintptr_t)windowsNames[i]);
        ck_assert_uint_eq(createEntry(&b, p, false), UA_STATUSCODE_BADINVALIDARGUMENT);
    }
    ck_assert_uint_eq(createEntry(&b, UA_STRING("CONSOLE.txt"), false),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&b, UA_STRING("COM10"), false), UA_STATUSCODE_GOOD);
#endif

    /* The backend requires an existing root directory */
    UA_FileTransferBackend b2;
    ck_assert_uint_eq(UA_FileTransferBackend_localDirectory(
                          UA_STRING("/nonexistent-filetransfer-root"), &b2),
                      UA_STATUSCODE_BADNOTFOUND);

    b.file.clear(&b.file);
    removeTree(scratchDir);
} END_TEST

/* The names are UTF-8. On Windows, the backend stores them as UTF-16, so every
 * name is represented independent of the active code page. */
START_TEST(localDirectoryUtf8Names) {
    makeScratchDir();
    const char *name = "Gr\xc3\xb6\xc3\x9f" "e-\xe6\x97\xa5\xe6\x9c\xac.txt";
    UA_String path = UA_STRING((char*)(uintptr_t)name);
    UA_FileTransferBackend b;
    ck_assert_uint_eq(UA_FileTransferBackend_localDirectory(
                          UA_STRING(scratchDir), &b),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&b, path, false), UA_STATUSCODE_GOOD);
    UA_UInt32 fc = 0;
    ck_assert_uint_eq(b.file.open(&b.file, path, UA_OPENFILEMODE_WRITE, &fc),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b.file.write(&b.file, fc, UA_BYTESTRING("utf8")),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b.file.close(&b.file, fc), UA_STATUSCODE_GOOD);

    /* The listing returns the same name */
    ListResult lr;
    memset(&lr, 0, sizeof(lr));
    ck_assert_uint_eq(b.listDirectory(&b, UA_STRING(""), listCollector, &lr),
                      UA_STATUSCODE_GOOD);
    ck_assert(listContains(&lr, name, false));

    /* The name on the disk is the Unicode name */
#ifdef _WIN32
    wchar_t diskPath[MAX_PATH];
    widePath(scratchDir, diskPath);
    wcscat(diskPath, L"\\Gr\u00f6\u00dfe-\u65e5\u672c.txt");
    struct _stat64 st;
    ck_assert_int_eq(_wstat64(diskPath, &st), 0);
#else
    char diskPath[256];
    snprintf(diskPath, sizeof(diskPath), "%s/%s", scratchDir, name);
    struct stat st;
    ck_assert_int_eq(stat(diskPath, &st), 0);
#endif
    ck_assert_uint_eq((size_t)st.st_size, 4);

    /* The single-file backend accepts the same UTF-8 filename. */
    char filePath[512];
    snprintf(filePath, sizeof(filePath), "%s/%s", scratchDir, name);
#ifdef _WIN32
    for(char *p = filePath; *p; p++) {
        if(*p == '/')
            *p = '\\';
    }
#endif
    UA_FileTransferFileBackend file;
    ck_assert_uint_eq(UA_FileTransferFileBackend_localFile(UA_STRING(filePath), &file),
                      UA_STATUSCODE_GOOD);
    UA_FileTransferFileInfo info;
    ck_assert_uint_eq(file.getInfo(&file, UA_STRING_NULL, &info), UA_STATUSCODE_GOOD);
    ck_assert_str_eq(info.name, name);
    ck_assert_str_eq(info.mimeType, "text/plain");
    ck_assert_uint_eq(info.size, 4);
    ck_assert_uint_eq(file.open(&file, UA_STRING_NULL, UA_OPENFILEMODE_READ, &fc),
                      UA_STATUSCODE_GOOD);
    UA_ByteString content;
    ck_assert_uint_eq(file.read(&file, fc, 10, &content), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(content.length, 4);
    ck_assert_int_eq(memcmp(content.data, "utf8", 4), 0);
    UA_ByteString_clear(&content);
    ck_assert_uint_eq(file.close(&file, fc), UA_STATUSCODE_GOOD);
    file.clear(&file);

    b.file.clear(&b.file);
    removeTree(scratchDir);
} END_TEST

/* Helper: add an object instance of FileType without going through the
 * driver API */
static UA_NodeId
addFileTypeInstance(UA_Server *s, const char *name) {
    UA_NodeId fileNodeId = UA_NODEID_NULL;
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", (char*)(uintptr_t)name);
    UA_StatusCode retval = UA_Server_addObjectNode(
        s, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(HASCOMPONENT),
        UA_QUALIFIEDNAME(0, (char*)(uintptr_t)name), UA_NS0ID(FILETYPE),
        attr, NULL, &fileNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    return fileNodeId;
}

/* Non-asserting child resolution (defined further below) */
static UA_Boolean
tryResolveChild(UA_Server *s, const UA_NodeId parent, const char *name,
                UA_NodeId *out);

/* Helper: resolve a child of a node by BrowseName */
static UA_NodeId
resolveChild(UA_Server *s, const UA_NodeId parent, const char *name) {
    UA_QualifiedName qn = UA_QUALIFIEDNAME(0, (char*)(uintptr_t)name);
    UA_BrowsePathResult bpr =
        UA_Server_browseSimplifiedBrowsePath(s, parent, 1, &qn);
    ck_assert_uint_eq(bpr.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_ge(bpr.targetsSize, 1);
    UA_NodeId result;
    UA_NodeId_copy(&bpr.targets[0].targetId.nodeId, &result);
    UA_BrowsePathResult_clear(&bpr);
    return result;
}

START_TEST(restartDriver) {
    UA_FileTransferBackend backend;
    ck_assert_uint_eq(memBackend(&backend), UA_STATUSCODE_GOOD);
    ftDriver = newTestDirectory(server_ft, &backend, NULL);
    registerTestDriver(ftDriver);
    ftDriver->stop(ftDriver);
    ck_assert_uint_eq(ftDriver->start(ftDriver),
                      UA_STATUSCODE_GOOD);
} END_TEST

/* A FileType instance must reference the method nodes of the FileType
 * ObjectType itself (methods are not copied during instantiation). The
 * method callbacks registered on the type method nodes then dispatch on the
 * objectId. The whole driver design relies on this. */
START_TEST(instanceSharesTypeMethodNodes) {
    UA_FileTransferBackend backend;
    ck_assert_uint_eq(memBackend(&backend), UA_STATUSCODE_GOOD);
    registerTestDriver(newTestDirectory(server_ft, &backend, NULL));
    UA_NodeId fileNodeId = addFileTypeInstance(server_ft, "TestFile");

    UA_NodeId openMethodId = resolveChild(server_ft, fileNodeId, "Open");
    UA_NodeId typeOpenId = UA_NS0ID(FILETYPE_OPEN);
    ck_assert(UA_NodeId_equal(&openMethodId, &typeOpenId));

    /* Calling Open on the instance reaches the registered driver callback.
     * The instance was created without the driver API, so the driver
     * rejects it as unmanaged. */
    UA_Byte mode = 0x01; /* Read */
    UA_Variant inputArgument;
    UA_Variant_setScalar(&inputArgument, &mode, &UA_TYPES[UA_TYPES_BYTE]);

    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.methodId = openMethodId;
    callMethodRequest.objectId = fileNodeId;
    callMethodRequest.inputArgumentsSize = 1;
    callMethodRequest.inputArguments = &inputArgument;

    /* Shared Methods must not dereference arbitrary application contexts, or
     * accept an unmanaged Object merely because it points to a valid driver. */
    UA_Driver unrelated;
    memset(&unrelated, 0, sizeof(unrelated));
    unrelated.state = UA_LIFECYCLESTATE_STARTED;
    ck_assert_uint_eq(UA_Server_addDriver(server_ft, &unrelated), UA_STATUSCODE_GOOD);
    void *contexts[] = {NULL, (void*)(uintptr_t)1, &unrelated, ftDriver};
    for(size_t i = 0; i < sizeof(contexts) / sizeof(contexts[0]); i++) {
        ck_assert_uint_eq(UA_Server_setNodeContext(server_ft, fileNodeId, contexts[i]),
                          UA_STATUSCODE_GOOD);
        UA_CallMethodResult result = UA_Server_call(server_ft, &callMethodRequest);
        ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_BADNOTSUPPORTED);
        UA_CallMethodResult_clear(&result);
    }
    ck_assert_uint_eq(UA_Server_setNodeContext(server_ft, fileNodeId, NULL), UA_STATUSCODE_GOOD);
    unrelated.state = UA_LIFECYCLESTATE_STOPPED;
    ck_assert_uint_eq(UA_Server_removeDriver(server_ft, &unrelated), UA_STATUSCODE_GOOD);

    UA_NodeId_clear(&openMethodId);
    UA_NodeId_clear(&fileNodeId);
} END_TEST

/**************************************
 * FileType Method Tests
 **************************************/

/* Helper: create a memory backend containing one file with content */
static UA_FileTransferBackend
memBackendWithFile(const char *name, const char *content) {
    UA_FileTransferBackend b;
    ck_assert_uint_eq(memBackend(&b), UA_STATUSCODE_GOOD);
    UA_String path = UA_STRING((char*)(uintptr_t)name);
    ck_assert_uint_eq(createEntry(&b, path, false), UA_STATUSCODE_GOOD);
    if(content && strlen(content) > 0) {
        UA_UInt32 fc = 0;
        ck_assert_uint_eq(b.file.open(&b.file, path, UA_OPENFILEMODE_WRITE, &fc),
                          UA_STATUSCODE_GOOD);
        UA_ByteString data = UA_BYTESTRING((char*)(uintptr_t)content);
        ck_assert_uint_eq(b.file.write(&b.file, fc, data), UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(b.file.close(&b.file, fc), UA_STATUSCODE_GOOD);
    }
    return b;
}

/* Helper: add a standalone f.bin from a supplied backend */
static UA_NodeId
addTestFileBackend(UA_FileTransferBackend b, const char *browseName,
                   const FTConfig *options) {
    UA_NodeId fileNodeId = UA_NODEID_NULL;
    UA_StatusCode res = testAddFile(UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
        UA_QUALIFIEDNAME(0, (char *)(uintptr_t)browseName), &b.file,
        UA_STRING("f.bin"), options, &fileNodeId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    return fileNodeId;
}

/* Helper: add a standalone file backed by a fresh memory backend */
static UA_NodeId
addTestFile(const char *browseName, const char *content,
            const FTConfig *options) {
    return addTestFileBackend(memBackendWithFile("f.bin", content), browseName, options);
}

/* Helper: call a FileType/FileDirectoryType Method on an object */
static UA_CallMethodResult
callMethod(const UA_NodeId objectId, UA_UInt32 methodNs0Id,
           size_t inputSize, UA_Variant *input) {
    UA_CallMethodRequest request;
    UA_CallMethodRequest_init(&request);
    request.objectId = objectId;
    request.methodId = UA_NODEID_NUMERIC(0, methodNs0Id);
    request.inputArgumentsSize = inputSize;
    request.inputArguments = input;
    return UA_Server_call(server_ft, &request);
}

static UA_UInt32
callOpen(const UA_NodeId fileId, UA_Byte mode, UA_StatusCode expected) {
    UA_Variant input;
    UA_Variant_setScalar(&input, &mode, &UA_TYPES[UA_TYPES_BYTE]);
    UA_CallMethodResult result = callMethod(fileId, UA_NS0ID_FILETYPE_OPEN, 1, &input);
    ck_assert_uint_eq(result.statusCode, expected);
    UA_UInt32 handle = 0;
    if(expected == UA_STATUSCODE_GOOD) {
        ck_assert_uint_eq(result.outputArgumentsSize, 1);
        handle = *(UA_UInt32*)result.outputArguments[0].data;
        ck_assert_uint_ne(handle, 0);
    }
    UA_CallMethodResult_clear(&result);
    return handle;
}

static void
callClose(const UA_NodeId fileId, UA_UInt32 handle, UA_StatusCode expected) {
    UA_Variant input;
    UA_Variant_setScalar(&input, &handle, &UA_TYPES[UA_TYPES_UINT32]);
    UA_CallMethodResult result = callMethod(fileId, UA_NS0ID_FILETYPE_CLOSE, 1, &input);
    ck_assert_uint_eq(result.statusCode, expected);
    UA_CallMethodResult_clear(&result);
}

/* Returns the read data on success. The caller clears the ByteString. */
static UA_ByteString
callRead(const UA_NodeId fileId, UA_UInt32 handle, UA_Int32 length,
         UA_StatusCode expected) {
    UA_Variant input[2];
    UA_Variant_setScalar(&input[0], &handle, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&input[1], &length, &UA_TYPES[UA_TYPES_INT32]);
    UA_CallMethodResult result = callMethod(fileId, UA_NS0ID_FILETYPE_READ, 2, input);
    ck_assert_uint_eq(result.statusCode, expected);
    UA_ByteString data = UA_BYTESTRING_NULL;
    if(expected == UA_STATUSCODE_GOOD) {
        ck_assert_uint_eq(result.outputArgumentsSize, 1);
        UA_ByteString_copy((UA_ByteString*)result.outputArguments[0].data, &data);
    }
    UA_CallMethodResult_clear(&result);
    return data;
}

static void
callWrite(const UA_NodeId fileId, UA_UInt32 handle, const char *content,
          UA_StatusCode expected) {
    UA_ByteString data = UA_BYTESTRING((char*)(uintptr_t)content);
    UA_Variant input[2];
    UA_Variant_setScalar(&input[0], &handle, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&input[1], &data, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_CallMethodResult result = callMethod(fileId, UA_NS0ID_FILETYPE_WRITE, 2, input);
    ck_assert_uint_eq(result.statusCode, expected);
    UA_CallMethodResult_clear(&result);
}

static UA_UInt64
callGetPosition(const UA_NodeId fileId, UA_UInt32 handle) {
    UA_Variant input;
    UA_Variant_setScalar(&input, &handle, &UA_TYPES[UA_TYPES_UINT32]);
    UA_CallMethodResult result =
        callMethod(fileId, UA_NS0ID_FILETYPE_GETPOSITION, 1, &input);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_GOOD);
    UA_UInt64 position = *(UA_UInt64*)result.outputArguments[0].data;
    UA_CallMethodResult_clear(&result);
    return position;
}

static void
callSetPosition(const UA_NodeId fileId, UA_UInt32 handle, UA_UInt64 position) {
    UA_Variant input[2];
    UA_Variant_setScalar(&input[0], &handle, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&input[1], &position, &UA_TYPES[UA_TYPES_UINT64]);
    UA_CallMethodResult result =
        callMethod(fileId, UA_NS0ID_FILETYPE_SETPOSITION, 2, input);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&result);
}

/* Helper: read a Property value of a file object */
static void
readProperty(const UA_NodeId fileId, const char *name, UA_Variant *out) {
    UA_NodeId propId = resolveChild(server_ft, fileId, name);
    ck_assert_uint_eq(UA_Server_readValue(server_ft, propId, out),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&propId);
}

static UA_UInt16
readOpenCount(const UA_NodeId fileId) {
    UA_Variant value;
    readProperty(fileId, "OpenCount", &value);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT16]));
    UA_UInt16 openCount = *(UA_UInt16*)value.data;
    UA_Variant_clear(&value);
    return openCount;
}

START_TEST(fileProperties) {
    UA_NodeId fileId = addTestFile("PropFile", "0123456789", NULL);

    UA_Variant value;
    readProperty(fileId, "Size", &value);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT64]));
    ck_assert_uint_eq(*(UA_UInt64*)value.data, 10);
    UA_Variant_clear(&value);

    readProperty(fileId, "Writable", &value);
    ck_assert(*(UA_Boolean*)value.data);
    UA_Variant_clear(&value);

    readProperty(fileId, "UserWritable", &value);
    ck_assert(*(UA_Boolean*)value.data);
    UA_Variant_clear(&value);

    ck_assert_uint_eq(readOpenCount(fileId), 0);

    /* Derived scalar Properties retain Read timestamp and index-range behavior. */
    const char *derived[] = {"OpenCount", "MaxByteStringLength"};
    for(size_t i = 0; i < 2; i++) {
        UA_ReadValueId item;
        UA_ReadValueId_init(&item);
        item.nodeId = resolveChild(server_ft, fileId, derived[i]);
        item.attributeId = UA_ATTRIBUTEID_VALUE;
        UA_DataValue data = UA_Server_read(server_ft, &item, UA_TIMESTAMPSTORETURN_SOURCE);
        ck_assert_uint_eq(data.status, UA_STATUSCODE_GOOD);
        ck_assert(data.hasValue && data.hasSourceTimestamp);
        UA_DataValue_clear(&data);
        item.indexRange = UA_STRING("0");
        data = UA_Server_read(server_ft, &item, UA_TIMESTAMPSTORETURN_NEITHER);
        ck_assert_uint_eq(data.status, UA_STATUSCODE_BADINDEXRANGENODATA);
        UA_DataValue_clear(&data);
        UA_NodeId_clear(&item.nodeId);
    }

    readProperty(fileId, "LastModifiedTime", &value);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATETIME]));
    UA_Variant_clear(&value);

    /* The Size Property reflects changes made through the Methods */
    UA_UInt32 handle = callOpen(fileId, UA_OPENFILEMODE_WRITE |
                                UA_OPENFILEMODE_APPEND, UA_STATUSCODE_GOOD);
    callWrite(fileId, handle, "more!", UA_STATUSCODE_GOOD);
    callClose(fileId, handle, UA_STATUSCODE_GOOD);
    readProperty(fileId, "Size", &value);
    ck_assert_uint_eq(*(UA_UInt64*)value.data, 15);
    UA_Variant_clear(&value);

    ck_assert_uint_eq(testRemove(driverForRoot(fileId), fileId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileId);
} END_TEST

START_TEST(fileMaxByteStringLength) {
    /* The default driver exposes the default max-read-length (1 MByte) */
    UA_NodeId fileId = addTestFile("MbslFile", "data", NULL);
    UA_Variant value;
    readProperty(fileId, "MaxByteStringLength", &value);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT32]));
    ck_assert_uint_eq(*(UA_UInt32*)value.data, 1u << 20);
    UA_Variant_clear(&value);
    /* Mem-backed files report no MimeType, so the Property is omitted */
    ck_assert(!tryResolveChild(server_ft, fileId, "MimeType", NULL));
    ck_assert_uint_eq(testRemove(driverForRoot(fileId), fileId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileId);

    /* A driver configured with a custom max-read-length reflects that value */
    UA_Server *server = UA_Server_newForUnitTest();
    UA_UInt32 maxRead = 4096;
    UA_KeyValueMap params = UA_KEYVALUEMAP_NULL;
    UA_KeyValueMap_setScalar(&params, UA_QUALIFIEDNAME(0, "max-read-length"),
                             &maxRead, &UA_TYPES[UA_TYPES_UINT32]);
    UA_FileTransferBackend b = memBackendWithFile("f.bin", "x");
    UA_NodeId customFile = UA_NODEID_NULL;
    UA_Driver *driver = newTestFile(server, &b.file, "CustomFile", &customFile);
    driver->params = params;
    ck_assert_ptr_nonnull(driver);
    ck_assert_uint_eq(UA_Server_addDriver(server, driver), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(driver->start(driver), UA_STATUSCODE_GOOD);

    UA_QualifiedName qn = UA_QUALIFIEDNAME(0, "MaxByteStringLength");
    UA_BrowsePathResult bpr =
        UA_Server_browseSimplifiedBrowsePath(server, customFile, 1, &qn);
    ck_assert_uint_eq(bpr.statusCode, UA_STATUSCODE_GOOD);
    UA_Variant custom;
    ck_assert_uint_eq(UA_Server_readValue(server, bpr.targets[0].targetId.nodeId,
                                          &custom), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(*(UA_UInt32*)custom.data, 4096);
    UA_Variant_clear(&custom);

    /* Restart changes the configuration behind the existing Property. */
    driver->stop(driver);
    maxRead = 8192;
    ck_assert_uint_eq(UA_KeyValueMap_setScalar(&driver->params,
        UA_QUALIFIEDNAME(0, "max-read-length"), &maxRead, &UA_TYPES[UA_TYPES_UINT32]),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(driver->start(driver), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_readValue(server, bpr.targets[0].targetId.nodeId,
                                          &custom), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(*(UA_UInt32*)custom.data, 8192);
    UA_Variant_clear(&custom);
    UA_BrowsePathResult_clear(&bpr);

    driver->stop(driver);
    ck_assert_uint_eq(UA_Server_removeDriver(server, driver), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(driver->free(driver), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&customFile);
    UA_Server_delete(server);
} END_TEST

/* MimeType is inferred from the extension by the local filesystem backend */
START_TEST(fileMimeType) {
    makeScratchDir();

    UA_FileTransferBackend pre;
    ck_assert_uint_eq(UA_FileTransferBackend_localDirectory(
                          UA_STRING(scratchDir), &pre), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&pre, UA_STRING("a.txt"), false), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&pre, UA_STRING("b.json"), false), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&pre, UA_STRING("c"), false), UA_STATUSCODE_GOOD);
    pre.file.clear(&pre.file);

    UA_FileTransferBackend b;
    ck_assert_uint_eq(UA_FileTransferBackend_localDirectory(
                          UA_STRING(scratchDir), &b), UA_STATUSCODE_GOOD);
    UA_NodeId fsId = UA_NODEID_NULL;
    ck_assert_uint_eq(testAddDirectory(UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                          UA_QUALIFIEDNAME(0, "FileSystem"), &b, NULL, &fsId),
                      UA_STATUSCODE_GOOD);

    UA_NodeId txtId, jsonId, cId;
    ck_assert(tryResolveChild(server_ft, fsId, "a.txt", &txtId));
    ck_assert(tryResolveChild(server_ft, fsId, "b.json", &jsonId));
    ck_assert(tryResolveChild(server_ft, fsId, "c", &cId));

    UA_Variant value;
    readProperty(txtId, "MimeType", &value);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_STRING]));
    UA_String expectedTxt = UA_STRING("text/plain");
    ck_assert(UA_String_equal((UA_String*)value.data, &expectedTxt));
    UA_Variant_clear(&value);

    readProperty(jsonId, "MimeType", &value);
    UA_String expectedJson = UA_STRING("application/json");
    ck_assert(UA_String_equal((UA_String*)value.data, &expectedJson));
    UA_Variant_clear(&value);

    /* A file without a known extension has no MimeType Property */
    ck_assert(!tryResolveChild(server_ft, cId, "MimeType", NULL));
    ck_assert(!tryResolveChild(server_ft, fsId, "MimeType", NULL)); /* not the dir */

    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&txtId);
    UA_NodeId_clear(&jsonId);
    UA_NodeId_clear(&cId);
    UA_NodeId_clear(&fsId);
    removeTree(scratchDir);
} END_TEST

START_TEST(fileOpenModes) {
    UA_NodeId fileId = addTestFile("ModesFile", "content", NULL);

    /* Invalid modes */
    callOpen(fileId, 0, UA_STATUSCODE_BADINVALIDARGUMENT);
    callOpen(fileId, 0x10, UA_STATUSCODE_BADINVALIDARGUMENT); /* Reserved bit */
    callOpen(fileId, UA_OPENFILEMODE_READ | UA_OPENFILEMODE_ERASEEXISTING,
             UA_STATUSCODE_BADINVALIDARGUMENT); /* Erase requires write */

    /* Multiple parallel read handles are allowed */
    UA_UInt32 h1 = callOpen(fileId, UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    UA_UInt32 h2 = callOpen(fileId, UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(h1, h2);
    ck_assert_uint_eq(readOpenCount(fileId), 2);
    callClose(fileId, h1, UA_STATUSCODE_GOOD);
    callClose(fileId, h2, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(readOpenCount(fileId), 0);

    /* EraseExisting truncates the file */
    UA_UInt32 h3 = callOpen(fileId, UA_OPENFILEMODE_WRITE |
                            UA_OPENFILEMODE_ERASEEXISTING, UA_STATUSCODE_GOOD);
    callClose(fileId, h3, UA_STATUSCODE_GOOD);
    UA_Variant value;
    readProperty(fileId, "Size", &value);
    ck_assert_uint_eq(*(UA_UInt64*)value.data, 0);
    UA_Variant_clear(&value);

    ck_assert_uint_eq(testRemove(driverForRoot(fileId), fileId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileId);
} END_TEST

START_TEST(fileLocking) {
    UA_NodeId fileId = addTestFile("LockFile", "content", NULL);

    /* A file that is open cannot be opened for writing */
    UA_UInt32 hRead = callOpen(fileId, UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    callOpen(fileId, UA_OPENFILEMODE_WRITE, UA_STATUSCODE_BADNOTWRITABLE);
    callClose(fileId, hRead, UA_STATUSCODE_GOOD);

    /* A file that is open for writing cannot be opened at all */
    UA_UInt32 hWrite = callOpen(fileId, UA_OPENFILEMODE_WRITE, UA_STATUSCODE_GOOD);
    callOpen(fileId, UA_OPENFILEMODE_READ, UA_STATUSCODE_BADNOTREADABLE);
    callOpen(fileId, UA_OPENFILEMODE_WRITE, UA_STATUSCODE_BADNOTWRITABLE);
    callClose(fileId, hWrite, UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(testRemove(driverForRoot(fileId), fileId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileId);
} END_TEST

START_TEST(fileReadWrite) {
    UA_NodeId fileId = addTestFile("RwFile", "0123456789", NULL);

    UA_UInt32 h = callOpen(fileId, UA_OPENFILEMODE_READ | UA_OPENFILEMODE_WRITE,
                           UA_STATUSCODE_GOOD);

    /* Read advances the position */
    UA_ByteString data = callRead(fileId, h, 4, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(data.length, 4);
    ck_assert_int_eq(memcmp(data.data, "0123", 4), 0);
    UA_ByteString_clear(&data);
    ck_assert_uint_eq(callGetPosition(fileId, h), 4);

    /* Write at the current position */
    callWrite(fileId, h, "AB", UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(callGetPosition(fileId, h), 6);
    callSetPosition(fileId, h, 0);
    data = callRead(fileId, h, 100, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(data.length, 10);
    ck_assert_int_eq(memcmp(data.data, "0123AB6789", 10), 0);
    UA_ByteString_clear(&data);

    /* Reading at the end of the file returns an empty ByteString */
    data = callRead(fileId, h, 10, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(data.length, 0);
    UA_ByteString_clear(&data);

    /* Only positive read lengths are allowed */
    callRead(fileId, h, 0, UA_STATUSCODE_BADINVALIDARGUMENT);
    callRead(fileId, h, -5, UA_STATUSCODE_BADINVALIDARGUMENT);

    /* Writing an empty ByteString is a no-op with a Good result */
    callWrite(fileId, h, "", UA_STATUSCODE_GOOD);

    callClose(fileId, h, UA_STATUSCODE_GOOD);

    /* Mode restrictions on the handle */
    UA_UInt32 hRead = callOpen(fileId, UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    callWrite(fileId, hRead, "x", UA_STATUSCODE_BADINVALIDSTATE);
    callClose(fileId, hRead, UA_STATUSCODE_GOOD);
    UA_UInt32 hWrite = callOpen(fileId, UA_OPENFILEMODE_WRITE, UA_STATUSCODE_GOOD);
    callRead(fileId, hWrite, 1, UA_STATUSCODE_BADINVALIDSTATE);
    callClose(fileId, hWrite, UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(testRemove(driverForRoot(fileId), fileId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileId);
} END_TEST

START_TEST(fileBadHandles) {
    UA_NodeId fileA = addTestFile("HandleFileA", "aaa", NULL);
    UA_NodeId fileB = addTestFile("HandleFileB", "bbb", NULL);

    /* Unknown handle */
    callClose(fileA, 12345, UA_STATUSCODE_BADINVALIDARGUMENT);

    /* A handle is only valid for the file object it was created on */
    UA_UInt32 h = callOpen(fileA, UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    callRead(fileB, h, 1, UA_STATUSCODE_BADINVALIDARGUMENT);
    callClose(fileB, h, UA_STATUSCODE_BADINVALIDARGUMENT);
    callClose(fileA, h, UA_STATUSCODE_GOOD);

    /* A closed handle is invalid */
    callClose(fileA, h, UA_STATUSCODE_BADINVALIDARGUMENT);

    ck_assert_uint_eq(testRemove(driverForRoot(fileA), fileA),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(testRemove(driverForRoot(fileB), fileB),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileA);
    UA_NodeId_clear(&fileB);
} END_TEST

START_TEST(fileReadOnlyMount) {
    FTConfig options;
    memset(&options, 0, sizeof(options));
    options.readOnly = true;
    UA_NodeId fileId = addTestFile("RoFile", "content", &options);

    UA_Variant value;
    readProperty(fileId, "Writable", &value);
    ck_assert(!*(UA_Boolean*)value.data);
    UA_Variant_clear(&value);
    readProperty(fileId, "UserWritable", &value);
    ck_assert(!*(UA_Boolean*)value.data);
    UA_Variant_clear(&value);

    callOpen(fileId, UA_OPENFILEMODE_WRITE, UA_STATUSCODE_BADNOTWRITABLE);
    UA_UInt32 h = callOpen(fileId, UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    callClose(fileId, h, UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(testRemove(driverForRoot(fileId), fileId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileId);
} END_TEST

START_TEST(fileHandleLimits) {
    /* A driver with tight limits on a separate server */
    UA_Server *server = UA_Server_newForUnitTest();

    UA_UInt16 maxPerFile = 2;
    UA_UInt16 maxPerSession = 3;
    UA_KeyValueMap params = UA_KEYVALUEMAP_NULL;
    UA_KeyValueMap_setScalar(&params,
                             UA_QUALIFIEDNAME(0, "max-open-handles-per-file"),
                             &maxPerFile, &UA_TYPES[UA_TYPES_UINT16]);
    UA_KeyValueMap_setScalar(&params,
                             UA_QUALIFIEDNAME(0, "max-open-handles-per-session"),
                             &maxPerSession, &UA_TYPES[UA_TYPES_UINT16]);

    UA_FileTransferBackend backend;
    ck_assert_uint_eq(memBackend(&backend), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&backend, UA_STRING("a.bin"), false), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&backend, UA_STRING("b.bin"), false), UA_STATUSCODE_GOOD);
    UA_NodeId root = UA_NODEID_NULL;
    UA_Driver *driver = newTestDirectory(server, &backend, &root);
    driver->params = params;
    ck_assert_ptr_nonnull(driver);
    ck_assert_uint_eq(UA_Server_addDriver(server, driver), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(driver->start(driver), UA_STATUSCODE_GOOD);
    UA_NodeId fileA = resolveChild(server, root, "a.bin");
    UA_NodeId fileB = resolveChild(server, root, "b.bin");
    UA_NodeId_clear(&root);

    UA_Byte readMode = UA_OPENFILEMODE_READ;
    UA_Variant input;
    UA_Variant_setScalar(&input, &readMode, &UA_TYPES[UA_TYPES_BYTE]);

    UA_CallMethodRequest request;
    UA_CallMethodRequest_init(&request);
    request.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_OPEN);
    request.inputArgumentsSize = 1;
    request.inputArguments = &input;

    /* Per-file limit: the third open on the same file fails */
    request.objectId = fileA;
    UA_CallMethodResult r1 = UA_Server_call(server, &request);
    UA_CallMethodResult r2 = UA_Server_call(server, &request);
    UA_CallMethodResult r3 = UA_Server_call(server, &request);
    ck_assert_uint_eq(r1.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(r2.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(r3.statusCode, UA_STATUSCODE_BADRESOURCEUNAVAILABLE);

    /* Per-session limit: the fourth open in the session fails */
    request.objectId = fileB;
    UA_CallMethodResult r4 = UA_Server_call(server, &request);
    UA_CallMethodResult r5 = UA_Server_call(server, &request);
    ck_assert_uint_eq(r4.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(r5.statusCode, UA_STATUSCODE_BADRESOURCEUNAVAILABLE);

    UA_CallMethodResult_clear(&r1);
    UA_CallMethodResult_clear(&r2);
    UA_CallMethodResult_clear(&r3);
    UA_CallMethodResult_clear(&r4);
    UA_CallMethodResult_clear(&r5);

    /* The driver cleans up the open handles on stop/free */
    driver->stop(driver);
    ck_assert_uint_eq(UA_Server_removeDriver(server, driver), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(driver->free(driver), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileA);
    UA_NodeId_clear(&fileB);
    UA_Server_delete(server);
} END_TEST

START_TEST(removeFileClosesHandles) {
    UA_NodeId fileId = addTestFile("RemoveFile", "content", NULL);

    UA_UInt32 h = callOpen(fileId, UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    (void)h;
    ck_assert_uint_eq(testRemove(driverForRoot(fileId), fileId),
                      UA_STATUSCODE_GOOD);

    /* The object is gone from the address space */
    UA_QualifiedName browseName;
    ck_assert_uint_ne(UA_Server_readBrowseName(server_ft, fileId, &browseName),
                      UA_STATUSCODE_GOOD);

    /* Removing again fails */
    ck_assert_uint_eq(testRemove(driverForRoot(fileId), fileId),
                      UA_STATUSCODE_BADNOTFOUND);
    UA_NodeId_clear(&fileId);
} END_TEST

/**************************************
 * FileDirectoryType Method Tests
 **************************************/

/* Non-asserting child resolution */
static UA_Boolean
tryResolveChild(UA_Server *s, const UA_NodeId parent, const char *name,
                UA_NodeId *out) {
    UA_QualifiedName qn = UA_QUALIFIEDNAME(0, (char*)(uintptr_t)name);
    UA_BrowsePathResult bpr =
        UA_Server_browseSimplifiedBrowsePath(s, parent, 1, &qn);
    UA_Boolean found = (bpr.statusCode == UA_STATUSCODE_GOOD &&
                        bpr.targetsSize >= 1);
    if(found && out)
        UA_NodeId_copy(&bpr.targets[0].targetId.nodeId, out);
    UA_BrowsePathResult_clear(&bpr);
    return found;
}

/* Helper: memory backend with a pre-created directory tree:
 *   readme.txt, docs/, docs/a.txt, docs/sub/, docs/sub/b.txt */
static UA_FileTransferBackend
memBackendWithTree(void) {
    UA_FileTransferBackend b;
    ck_assert_uint_eq(memBackend(&b), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&b, UA_STRING("readme.txt"), false), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&b, UA_STRING("docs"), true), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&b, UA_STRING("docs/a.txt"), false), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&b, UA_STRING("docs/sub"), true), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&b, UA_STRING("docs/sub/b.txt"), false), UA_STATUSCODE_GOOD);
    UA_UInt32 fc = 0;
    ck_assert_uint_eq(b.file.open(&b.file, UA_STRING("docs/a.txt"),
                                  UA_OPENFILEMODE_WRITE, &fc),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b.file.write(&b.file, fc, UA_BYTESTRING("content-a")),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b.file.close(&b.file, fc), UA_STATUSCODE_GOOD);
    return b;
}

static UA_NodeId
mountTree(const FTConfig *options) {
    UA_NodeId fsId = UA_NODEID_NULL;
    UA_StatusCode res = testAddDirectory(UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
        UA_QUALIFIEDNAME(0, "FileSystem"), backendArg(memBackendWithTree()),
        options, &fsId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    return fsId;
}

static UA_NodeId
callCreateDirectory(const UA_NodeId dirId, const char *name,
                    UA_StatusCode expected) {
    UA_String dirName = UA_STRING((char*)(uintptr_t)name);
    UA_Variant input;
    UA_Variant_setScalar(&input, &dirName, &UA_TYPES[UA_TYPES_STRING]);
    UA_CallMethodResult result =
        callMethod(dirId, UA_NS0ID_FILEDIRECTORYTYPE_CREATEDIRECTORY, 1, &input);
    ck_assert_uint_eq(result.statusCode, expected);
    UA_NodeId newNodeId = UA_NODEID_NULL;
    if(expected == UA_STATUSCODE_GOOD) {
        ck_assert_uint_eq(result.outputArgumentsSize, 1);
        UA_NodeId_copy((UA_NodeId*)result.outputArguments[0].data, &newNodeId);
    }
    UA_CallMethodResult_clear(&result);
    return newNodeId;
}

static UA_NodeId
callCreateFile(const UA_NodeId dirId, const char *name, UA_Boolean requestOpen,
               UA_UInt32 *outHandle, UA_StatusCode expected) {
    UA_String fileName = UA_STRING((char*)(uintptr_t)name);
    UA_Variant input[2];
    UA_Variant_setScalar(&input[0], &fileName, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&input[1], &requestOpen, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_CallMethodResult result =
        callMethod(dirId, UA_NS0ID_FILEDIRECTORYTYPE_CREATEFILE, 2, input);
    ck_assert_uint_eq(result.statusCode, expected);
    UA_NodeId newNodeId = UA_NODEID_NULL;
    if(expected == UA_STATUSCODE_GOOD) {
        ck_assert_uint_eq(result.outputArgumentsSize, 2);
        UA_NodeId_copy((UA_NodeId*)result.outputArguments[0].data, &newNodeId);
        if(outHandle)
            *outHandle = *(UA_UInt32*)result.outputArguments[1].data;
    }
    UA_CallMethodResult_clear(&result);
    return newNodeId;
}

static void
callDelete(const UA_NodeId dirId, const UA_NodeId objectToDelete,
           UA_StatusCode expected) {
    UA_Variant input;
    UA_Variant_setScalar(&input, (void*)(uintptr_t)&objectToDelete,
                         &UA_TYPES[UA_TYPES_NODEID]);
    UA_CallMethodResult result = callMethod(
        dirId, UA_NS0ID_FILEDIRECTORYTYPE_DELETEFILESYSTEMOBJECT, 1, &input);
    ck_assert_uint_eq(result.statusCode, expected);
    UA_CallMethodResult_clear(&result);
}

static UA_NodeId
callMoveOrCopy(const UA_NodeId dirId, const UA_NodeId object,
               const UA_NodeId targetDir, UA_Boolean createCopy,
               const char *newName, UA_StatusCode expected) {
    UA_String name = UA_STRING((char*)(uintptr_t)newName);
    UA_Variant input[4];
    UA_Variant_setScalar(&input[0], (void*)(uintptr_t)&object,
                         &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalar(&input[1], (void*)(uintptr_t)&targetDir,
                         &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalar(&input[2], &createCopy, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Variant_setScalar(&input[3], &name, &UA_TYPES[UA_TYPES_STRING]);
    UA_CallMethodResult result =
        callMethod(dirId, UA_NS0ID_FILEDIRECTORYTYPE_MOVEORCOPY, 4, input);
    ck_assert_uint_eq(result.statusCode, expected);
    UA_NodeId newNodeId = UA_NODEID_NULL;
    if(expected == UA_STATUSCODE_GOOD) {
        ck_assert_uint_eq(result.outputArgumentsSize, 1);
        UA_NodeId_copy((UA_NodeId*)result.outputArguments[0].data, &newNodeId);
    }
    UA_CallMethodResult_clear(&result);
    return newNodeId;
}

/* Read the whole file content of a file object via Open/Read/Close */
static UA_ByteString
readFileContent(const UA_NodeId fileId) {
    UA_UInt32 h = callOpen(fileId, UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    UA_ByteString data = callRead(fileId, h, 1024, UA_STATUSCODE_GOOD);
    callClose(fileId, h, UA_STATUSCODE_GOOD);
    return data;
}

START_TEST(mountScanMirrorsTree) {
    UA_NodeId fsId = mountTree(NULL);

    /* The whole tree is mirrored */
    UA_NodeId readmeId, docsId, subId, aId, bId;
    ck_assert(tryResolveChild(server_ft, fsId, "readme.txt", &readmeId));
    ck_assert(tryResolveChild(server_ft, fsId, "docs", &docsId));
    ck_assert(tryResolveChild(server_ft, docsId, "a.txt", &aId));
    ck_assert(tryResolveChild(server_ft, docsId, "sub", &subId));
    ck_assert(tryResolveChild(server_ft, subId, "b.txt", &bId));

    /* Mirrored files are functional FileType objects */
    UA_ByteString content = readFileContent(aId);
    ck_assert_uint_eq(content.length, 9);
    ck_assert_int_eq(memcmp(content.data, "content-a", 9), 0);
    UA_ByteString_clear(&content);

    /* FileType methods are no components of directory objects. The call
     * service rejects them before the driver is reached. */
    callOpen(docsId, UA_OPENFILEMODE_READ, UA_STATUSCODE_BADMETHODINVALID);

    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    ck_assert(!tryResolveChild(server_ft, UA_NS0ID(OBJECTSFOLDER),
                               "FileSystem", NULL));
    UA_NodeId_clear(&readmeId);
    UA_NodeId_clear(&docsId);
    UA_NodeId_clear(&subId);
    UA_NodeId_clear(&aId);
    UA_NodeId_clear(&bId);
    UA_NodeId_clear(&fsId);
} END_TEST

START_TEST(mountScanDepthLimit) {
    FTConfig options;
    memset(&options, 0, sizeof(options));
    options.maxScanDepth = 1;
    UA_NodeId fsId = mountTree(&options);

    /* Only the first level is mirrored */
    UA_NodeId docsId;
    ck_assert(tryResolveChild(server_ft, fsId, "readme.txt", NULL));
    ck_assert(tryResolveChild(server_ft, fsId, "docs", &docsId));
    ck_assert(!tryResolveChild(server_ft, docsId, "a.txt", NULL));

    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&docsId);
    UA_NodeId_clear(&fsId);
} END_TEST

START_TEST(dirCreateMethods) {
    UA_NodeId fsId = mountTree(NULL);

    /* CreateDirectory */
    UA_NodeId newDirId = callCreateDirectory(fsId, "upload", UA_STATUSCODE_GOOD);
    ck_assert(!UA_NodeId_isNull(&newDirId));
    ck_assert(tryResolveChild(server_ft, fsId, "upload", NULL));

    /* CreateFile without opening */
    UA_UInt32 handle = 1234;
    UA_NodeId newFileId = callCreateFile(newDirId, "data.bin", false, &handle,
                                         UA_STATUSCODE_GOOD);
    ck_assert(!UA_NodeId_isNull(&newFileId));
    ck_assert_uint_eq(handle, 0); /* Shall be 0 if requestFileOpen is false */

    /* CreateFile with requestFileOpen: the returned handle is usable */
    UA_NodeId openFileId = callCreateFile(newDirId, "open.bin", true, &handle,
                                          UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(handle, 0);
    callWrite(openFileId, handle, "written", UA_STATUSCODE_GOOD);
    callClose(openFileId, handle, UA_STATUSCODE_GOOD);
    UA_ByteString content = readFileContent(openFileId);
    ck_assert_uint_eq(content.length, 7);
    UA_ByteString_clear(&content);

    /* Duplicate names */
    UA_NodeId dup = callCreateDirectory(fsId, "upload",
                                        UA_STATUSCODE_BADBROWSENAMEDUPLICATED);
    ck_assert(UA_NodeId_isNull(&dup));
    dup = callCreateFile(newDirId, "data.bin", false, NULL,
                         UA_STATUSCODE_BADBROWSENAMEDUPLICATED);
    ck_assert(UA_NodeId_isNull(&dup));
    /* A directory cannot be shadowed by a file and vice versa */
    dup = callCreateFile(fsId, "upload", false, NULL,
                         UA_STATUSCODE_BADBROWSENAMEDUPLICATED);
    ck_assert(UA_NodeId_isNull(&dup));

    /* Invalid names */
    callCreateDirectory(fsId, "a/b", UA_STATUSCODE_BADINVALIDARGUMENT);
    callCreateDirectory(fsId, "..", UA_STATUSCODE_BADINVALIDARGUMENT);
    callCreateDirectory(fsId, "", UA_STATUSCODE_BADINVALIDARGUMENT);
    callCreateFile(fsId, "a\\b", false, NULL, UA_STATUSCODE_BADINVALIDARGUMENT);

    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&newDirId);
    UA_NodeId_clear(&newFileId);
    UA_NodeId_clear(&openFileId);
    UA_NodeId_clear(&fsId);
} END_TEST

START_TEST(dirReadOnlyMount) {
    FTConfig options;
    memset(&options, 0, sizeof(options));
    options.readOnly = true;
    UA_NodeId fsId = mountTree(&options);

    callCreateDirectory(fsId, "nope", UA_STATUSCODE_BADUSERACCESSDENIED);
    callCreateFile(fsId, "nope.txt", false, NULL,
                   UA_STATUSCODE_BADUSERACCESSDENIED);

    UA_NodeId readmeId;
    ck_assert(tryResolveChild(server_ft, fsId, "readme.txt", &readmeId));
    UA_Variant input;
    UA_Variant_setScalar(&input, &readmeId, &UA_TYPES[UA_TYPES_NODEID]);
    UA_CallMethodResult result = callMethod(
        fsId, UA_NS0ID_FILEDIRECTORYTYPE_DELETEFILESYSTEMOBJECT, 1, &input);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_BADUSERACCESSDENIED);
    UA_CallMethodResult_clear(&result);

    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&readmeId);
    UA_NodeId_clear(&fsId);
} END_TEST

START_TEST(dirDelete) {
    UA_NodeId fsId = mountTree(NULL);

    UA_NodeId readmeId, docsId, subId, bId;
    ck_assert(tryResolveChild(server_ft, fsId, "readme.txt", &readmeId));
    ck_assert(tryResolveChild(server_ft, fsId, "docs", &docsId));
    ck_assert(tryResolveChild(server_ft, docsId, "sub", &subId));
    ck_assert(tryResolveChild(server_ft, subId, "b.txt", &bId));

    /* Delete requires the direct parent directory */
    callDelete(fsId, bId, UA_STATUSCODE_BADNOTFOUND);
    callDelete(fsId, fsId, UA_STATUSCODE_BADNOTFOUND);

    /* Open files (also nested ones) block the deletion */
    UA_UInt32 h = callOpen(bId, UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    callDelete(subId, bId, UA_STATUSCODE_BADINVALIDSTATE);
    callDelete(fsId, docsId, UA_STATUSCODE_BADINVALIDSTATE);
    callClose(bId, h, UA_STATUSCODE_GOOD);

    /* Delete a single file */
    callDelete(fsId, readmeId, UA_STATUSCODE_GOOD);
    ck_assert(!tryResolveChild(server_ft, fsId, "readme.txt", NULL));
    /* The backend entry is gone: the name can be reused */
    UA_NodeId again = callCreateFile(fsId, "readme.txt", false, NULL,
                                     UA_STATUSCODE_GOOD);
    ck_assert(!UA_NodeId_isNull(&again));

    /* Recursive directory deletion */
    callDelete(fsId, docsId, UA_STATUSCODE_GOOD);
    ck_assert(!tryResolveChild(server_ft, fsId, "docs", NULL));
    UA_QualifiedName bn;
    ck_assert_uint_ne(UA_Server_readBrowseName(server_ft, bId, &bn),
                      UA_STATUSCODE_GOOD); /* Nested nodes are gone */

    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&readmeId);
    UA_NodeId_clear(&docsId);
    UA_NodeId_clear(&subId);
    UA_NodeId_clear(&bId);
    UA_NodeId_clear(&again);
    UA_NodeId_clear(&fsId);
} END_TEST

START_TEST(dirMoveOrCopy) {
    UA_NodeId fsId = mountTree(NULL);

    UA_NodeId readmeId, docsId;
    ck_assert(tryResolveChild(server_ft, fsId, "readme.txt", &readmeId));
    ck_assert(tryResolveChild(server_ft, fsId, "docs", &docsId));

    /* Rename in place */
    UA_NodeId renamedId = callMoveOrCopy(fsId, readmeId, fsId, false,
                                         "manual.txt", UA_STATUSCODE_GOOD);
    ck_assert(!tryResolveChild(server_ft, fsId, "readme.txt", NULL));
    ck_assert(tryResolveChild(server_ft, fsId, "manual.txt", NULL));

    /* Move into a subdirectory, keeping the name */
    UA_NodeId movedId = callMoveOrCopy(fsId, renamedId, docsId, false, "",
                                       UA_STATUSCODE_GOOD);
    ck_assert(!tryResolveChild(server_ft, fsId, "manual.txt", NULL));
    ck_assert(tryResolveChild(server_ft, docsId, "manual.txt", NULL));

    /* Copy a file: both exist with the same content */
    UA_NodeId aId;
    ck_assert(tryResolveChild(server_ft, docsId, "a.txt", &aId));
    UA_NodeId copyId = callMoveOrCopy(docsId, aId, fsId, true, "a-copy.txt",
                                      UA_STATUSCODE_GOOD);
    UA_ByteString orig = readFileContent(aId);
    UA_ByteString copy = readFileContent(copyId);
    ck_assert(UA_ByteString_equal(&orig, &copy));
    ck_assert_uint_eq(copy.length, 9);
    UA_ByteString_clear(&orig);
    UA_ByteString_clear(&copy);

    /* Copy a whole directory recursively */
    UA_NodeId dirCopyId = callMoveOrCopy(fsId, docsId, fsId, true, "docs2",
                                         UA_STATUSCODE_GOOD);
    UA_NodeId subCopyId;
    ck_assert(tryResolveChild(server_ft, dirCopyId, "sub", &subCopyId));
    ck_assert(tryResolveChild(server_ft, subCopyId, "b.txt", NULL));

    /* Duplicate target names are rejected */
    callMoveOrCopy(docsId, aId, fsId, true, "a-copy.txt",
                   UA_STATUSCODE_BADBROWSENAMEDUPLICATED);

    /* The object must be organized by the called directory */
    callMoveOrCopy(fsId, aId, fsId, false, "elsewhere.txt",
                   UA_STATUSCODE_BADNOTFOUND);

    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&readmeId);
    UA_NodeId_clear(&docsId);
    UA_NodeId_clear(&renamedId);
    UA_NodeId_clear(&movedId);
    UA_NodeId_clear(&aId);
    UA_NodeId_clear(&copyId);
    UA_NodeId_clear(&dirCopyId);
    UA_NodeId_clear(&subCopyId);
    UA_NodeId_clear(&fsId);
} END_TEST

/* Write a file with content into a memory backend before it is mounted */
static void
writeMemFile(UA_FileTransferBackend *b, const char *name, const char *content) {
    UA_String path = UA_STRING((char*)(uintptr_t)name);
    ck_assert_uint_eq(createEntry(b, path, false), UA_STATUSCODE_GOOD);
    UA_UInt32 fc = 0;
    ck_assert_uint_eq(b->file.open(&b->file, path, UA_OPENFILEMODE_WRITE, &fc),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b->file.write(&b->file, fc,
                                    UA_BYTESTRING((char*)(uintptr_t)content)),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b->file.close(&b->file, fc), UA_STATUSCODE_GOOD);
}

static UA_NodeId
mountNamedMem(UA_FileTransferBackend backend, const char *browseName,
              const FTConfig *options) {
    UA_NodeId fsId = UA_NODEID_NULL;
    ck_assert_uint_eq(testAddDirectory(UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                          UA_QUALIFIEDNAME(0, (char *)(uintptr_t)browseName),
                          &backend, options, &fsId),
                      UA_STATUSCODE_GOOD);
    return fsId;
}

/* Moving and copying between two mounts backed by different backends */
START_TEST(crossMountMoveCopy) {
    UA_FileTransferBackend bA;
    ck_assert_uint_eq(memBackend(&bA), UA_STATUSCODE_GOOD);
    writeMemFile(&bA, "doc.txt", "hello");
    writeMemFile(&bA, "move.txt", "world");
    ck_assert_uint_eq(createEntry(&bA, UA_STRING("d"), true), UA_STATUSCODE_GOOD);
    writeMemFile(&bA, "d/nested.txt", "deep");

    UA_FileTransferBackend bB;
    ck_assert_uint_eq(memBackend(&bB), UA_STATUSCODE_GOOD);

    UA_NodeId fsA = mountNamedMem(bA, "FsA", NULL);
    UA_NodeId fsB = mountNamedMem(bB, "FsB", NULL);

    /* Copy a file A -> B: present in both, content preserved */
    UA_NodeId aDoc, bDoc;
    ck_assert(tryResolveChild(server_ft, fsA, "doc.txt", &aDoc));
    UA_NodeId copyId = callMoveOrCopy(fsA, aDoc, fsB, true, "doc.txt",
                                      UA_STATUSCODE_GOOD);
    ck_assert(tryResolveChild(server_ft, fsA, "doc.txt", NULL));
    ck_assert(tryResolveChild(server_ft, fsB, "doc.txt", &bDoc));
    UA_ByteString bContent = readFileContent(bDoc);
    ck_assert_uint_eq(bContent.length, 5);
    ck_assert_int_eq(memcmp(bContent.data, "hello", 5), 0);
    UA_ByteString_clear(&bContent);

    /* Move a file A -> B: gone from A, present in B */
    UA_NodeId aMove;
    ck_assert(tryResolveChild(server_ft, fsA, "move.txt", &aMove));
    UA_NodeId movedId = callMoveOrCopy(fsA, aMove, fsB, false, "",
                                       UA_STATUSCODE_GOOD);
    ck_assert(!tryResolveChild(server_ft, fsA, "move.txt", NULL));
    UA_NodeId bMove;
    ck_assert(tryResolveChild(server_ft, fsB, "move.txt", &bMove));
    UA_ByteString mContent = readFileContent(bMove);
    ck_assert_uint_eq(mContent.length, 5);
    ck_assert_int_eq(memcmp(mContent.data, "world", 5), 0);
    UA_ByteString_clear(&mContent);

    /* Copy a directory tree A -> B recursively */
    UA_NodeId aDir;
    ck_assert(tryResolveChild(server_ft, fsA, "d", &aDir));
    UA_NodeId dirCopyId = callMoveOrCopy(fsA, aDir, fsB, true, "d",
                                         UA_STATUSCODE_GOOD);
    ck_assert(tryResolveChild(server_ft, dirCopyId, "nested.txt", NULL));

    /* A duplicate target name is rejected across mounts too */
    callMoveOrCopy(fsA, aDoc, fsB, true, "doc.txt",
                   UA_STATUSCODE_BADBROWSENAMEDUPLICATED);

    ck_assert_uint_eq(testRemove(driverForRoot(fsA), fsA),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(testRemove(driverForRoot(fsB), fsB),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&aDoc);
    UA_NodeId_clear(&bDoc);
    UA_NodeId_clear(&copyId);
    UA_NodeId_clear(&aMove);
    UA_NodeId_clear(&movedId);
    UA_NodeId_clear(&bMove);
    UA_NodeId_clear(&aDir);
    UA_NodeId_clear(&dirCopyId);
    UA_NodeId_clear(&fsA);
    UA_NodeId_clear(&fsB);
} END_TEST

/* A copy may read from a read-only source mount; a move may not delete it */
START_TEST(crossMountReadOnlySource) {
    UA_FileTransferBackend bA;
    ck_assert_uint_eq(memBackend(&bA), UA_STATUSCODE_GOOD);
    writeMemFile(&bA, "ro.txt", "data");
    UA_FileTransferBackend bB;
    ck_assert_uint_eq(memBackend(&bB), UA_STATUSCODE_GOOD);

    FTConfig ro;
    memset(&ro, 0, sizeof(ro));
    ro.readOnly = true;
    UA_NodeId fsA = mountNamedMem(bA, "FsRo", &ro);
    UA_NodeId fsB = mountNamedMem(bB, "FsRW", NULL);

    UA_NodeId aRo;
    ck_assert(tryResolveChild(server_ft, fsA, "ro.txt", &aRo));

    /* Copy from the read-only mount to the writable mount succeeds */
    UA_NodeId copyId = callMoveOrCopy(fsA, aRo, fsB, true, "ro.txt",
                                      UA_STATUSCODE_GOOD);
    ck_assert(tryResolveChild(server_ft, fsB, "ro.txt", NULL));

    /* Moving out of the read-only mount is denied (source cannot be deleted) */
    callMoveOrCopy(fsA, aRo, fsB, false, "moved.txt",
                   UA_STATUSCODE_BADUSERACCESSDENIED);
    ck_assert(tryResolveChild(server_ft, fsA, "ro.txt", NULL));

    ck_assert_uint_eq(testRemove(driverForRoot(fsA), fsA),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(testRemove(driverForRoot(fsB), fsB),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&aRo);
    UA_NodeId_clear(&copyId);
    UA_NodeId_clear(&fsA);
    UA_NodeId_clear(&fsB);
} END_TEST

/* The Methods do not create entries below max-scan-depth, which the refresh
 * would never list. The storage stays unchanged. */
START_TEST(depthLimitAppliesToMethods) {
    FTConfig options;
    memset(&options, 0, sizeof(options));
    options.maxScanDepth = 1;
    UA_FileTransferBackend b = memBackendWithTree();
    MemBackendContext *ctx = (MemBackendContext*)b.file.context;
    UA_NodeId fsId = mountNamedMem(b, "FileSystem", &options);
    UA_NodeId docs, readme;
    ck_assert(tryResolveChild(server_ft, fsId, "docs", &docs));
    ck_assert(tryResolveChild(server_ft, fsId, "readme.txt", &readme));

    /* docs has depth 1, its entries would have depth 2 */
    callCreateDirectory(docs, "deep", UA_STATUSCODE_BADRESOURCEUNAVAILABLE);
    callCreateFile(docs, "x.txt", false, NULL, UA_STATUSCODE_BADRESOURCEUNAVAILABLE);
    callCreateFile(docs, "y.txt", true, NULL, UA_STATUSCODE_BADRESOURCEUNAVAILABLE);
    callMoveOrCopy(fsId, readme, docs, false, "", UA_STATUSCODE_BADRESOURCEUNAVAILABLE);
    callMoveOrCopy(fsId, readme, docs, true, "", UA_STATUSCODE_BADRESOURCEUNAVAILABLE);
    ck_assert_ptr_null(memFind(ctx, UA_STRING("docs/deep")));
    ck_assert_ptr_null(memFind(ctx, UA_STRING("docs/x.txt")));
    ck_assert_ptr_null(memFind(ctx, UA_STRING("docs/y.txt")));
    ck_assert_ptr_null(memFind(ctx, UA_STRING("docs/readme.txt")));
    ck_assert_ptr_nonnull(memFind(ctx, UA_STRING("readme.txt")));
    ck_assert(tryResolveChild(server_ft, fsId, "readme.txt", NULL));

    /* Entries of the root are within the limit */
    UA_NodeId dirId = callCreateDirectory(fsId, "top", UA_STATUSCODE_GOOD);
    UA_NodeId fileId = callCreateFile(fsId, "z.txt", false, NULL, UA_STATUSCODE_GOOD);
    UA_NodeId copyId = callMoveOrCopy(fsId, readme, fsId, true, "copy.txt",
                                      UA_STATUSCODE_GOOD);
    ck_assert_ptr_nonnull(memFind(ctx, UA_STRING("top")));
    ck_assert_ptr_nonnull(memFind(ctx, UA_STRING("z.txt")));
    ck_assert_ptr_nonnull(memFind(ctx, UA_STRING("copy.txt")));

    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&docs);
    UA_NodeId_clear(&readme);
    UA_NodeId_clear(&dirId);
    UA_NodeId_clear(&fileId);
    UA_NodeId_clear(&copyId);
    UA_NodeId_clear(&fsId);
} END_TEST

/* MoveOrCopy into another mount applies the depth limit of the target */
START_TEST(depthLimitAppliesToCrossMountTargets) {
    UA_FileTransferBackend bA;
    ck_assert_uint_eq(memBackend(&bA), UA_STATUSCODE_GOOD);
    writeMemFile(&bA, "doc.txt", "hello");
    MemBackendContext *ctxA = (MemBackendContext*)bA.file.context;
    UA_FileTransferBackend bB = memBackendWithTree();
    MemBackendContext *ctxB = (MemBackendContext*)bB.file.context;
    FTConfig options;
    memset(&options, 0, sizeof(options));
    options.maxScanDepth = 1;
    UA_NodeId fsA = mountNamedMem(bA, "FsA", NULL);
    UA_NodeId fsB = mountNamedMem(bB, "FsB", &options);
    UA_NodeId aDoc, bDocs;
    ck_assert(tryResolveChild(server_ft, fsA, "doc.txt", &aDoc));
    ck_assert(tryResolveChild(server_ft, fsB, "docs", &bDocs));

    callMoveOrCopy(fsA, aDoc, bDocs, false, "", UA_STATUSCODE_BADRESOURCEUNAVAILABLE);
    callMoveOrCopy(fsA, aDoc, bDocs, true, "", UA_STATUSCODE_BADRESOURCEUNAVAILABLE);
    ck_assert_ptr_null(memFind(ctxB, UA_STRING("docs/doc.txt")));
    ck_assert_ptr_nonnull(memFind(ctxA, UA_STRING("doc.txt")));
    ck_assert(tryResolveChild(server_ft, fsA, "doc.txt", NULL));

    /* The root of the target is within the limit */
    UA_NodeId movedId = callMoveOrCopy(fsA, aDoc, fsB, false, "", UA_STATUSCODE_GOOD);
    ck_assert(tryResolveChild(server_ft, fsB, "doc.txt", NULL));
    ck_assert_ptr_null(memFind(ctxA, UA_STRING("doc.txt")));

    ck_assert_uint_eq(testRemove(driverForRoot(fsA), fsA), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(testRemove(driverForRoot(fsB), fsB), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&aDoc);
    UA_NodeId_clear(&bDocs);
    UA_NodeId_clear(&movedId);
    UA_NodeId_clear(&fsA);
    UA_NodeId_clear(&fsB);
} END_TEST

START_TEST(dirRefresh) {
    /* Keep a second reference to the backend to make out-of-band changes.
     * The context is shared with the copy held by the mount. */
    UA_FileTransferBackend b = memBackendWithTree();
    UA_NodeId fsId = UA_NODEID_NULL;
    ck_assert_uint_eq(testAddDirectory(UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                          UA_QUALIFIEDNAME(0, "FileSystem"), &b, NULL, &fsId),
                      UA_STATUSCODE_GOOD);

    /* A new backend entry appears after refresh */
    ck_assert_uint_eq(createEntry(&b, UA_STRING("new.txt"), false), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&b, UA_STRING("docs/newdir"), true),
                      UA_STATUSCODE_GOOD);
    ck_assert(!tryResolveChild(server_ft, fsId, "new.txt", NULL));
    ck_assert_uint_eq(testRefresh(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    ck_assert(tryResolveChild(server_ft, fsId, "new.txt", NULL));
    UA_NodeId docsId;
    ck_assert(tryResolveChild(server_ft, fsId, "docs", &docsId));
    ck_assert(tryResolveChild(server_ft, docsId, "newdir", NULL));

    /* A vanished backend entry disappears after refresh */
    ck_assert_uint_eq(b.remove(&b, UA_STRING("readme.txt")), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(testRefresh(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    ck_assert(!tryResolveChild(server_ft, fsId, "readme.txt", NULL));

    /* A vanished file with an open handle: the node stays in the address
     * space (as a zombie) so the handle remains usable. The node is removed
     * when the last handle is closed. */
    UA_NodeId aId;
    ck_assert(tryResolveChild(server_ft, docsId, "a.txt", &aId));
    UA_UInt32 h = callOpen(aId, UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b.remove(&b, UA_STRING("docs/a.txt")),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(testRefresh(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    ck_assert(tryResolveChild(server_ft, docsId, "a.txt", NULL));
    /* Zombie files cannot be opened again */
    callOpen(aId, UA_OPENFILEMODE_READ, UA_STATUSCODE_BADNOTFOUND);
    callClose(aId, h, UA_STATUSCODE_GOOD);
    ck_assert(!tryResolveChild(server_ft, docsId, "a.txt", NULL));

    /* Refresh is stable afterwards */
    ck_assert_uint_eq(testRefresh(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&docsId);
    UA_NodeId_clear(&aId);
    UA_NodeId_clear(&fsId);
} END_TEST

START_TEST(removeDirectoryWithOpenHandles) {
    UA_NodeId fsId = mountTree(NULL);

    UA_NodeId docsId, aId;
    ck_assert(tryResolveChild(server_ft, fsId, "docs", &docsId));
    ck_assert(tryResolveChild(server_ft, docsId, "a.txt", &aId));
    callOpen(aId, UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);

    /* The mount removal closes the handle and deletes the subtree */
    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    UA_QualifiedName bn;
    ck_assert_uint_ne(UA_Server_readBrowseName(server_ft, aId, &bn),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_BADNOTFOUND);

    UA_NodeId_clear(&docsId);
    UA_NodeId_clear(&aId);
    UA_NodeId_clear(&fsId);
} END_TEST

/* The full directory workflow on the local filesystem backend */
START_TEST(localDirectoryMount) {
    makeScratchDir();

    /* Pre-create a small tree */
    UA_FileTransferBackend pre;
    ck_assert_uint_eq(UA_FileTransferBackend_localDirectory(
                          UA_STRING(scratchDir), &pre), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&pre, UA_STRING("logs"), true),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&pre, UA_STRING("logs/log1.txt"), false),
                      UA_STATUSCODE_GOOD);
    pre.file.clear(&pre.file);

    UA_FileTransferBackend b;
    ck_assert_uint_eq(UA_FileTransferBackend_localDirectory(
                          UA_STRING(scratchDir), &b), UA_STATUSCODE_GOOD);
    UA_NodeId fsId = UA_NODEID_NULL;
    ck_assert_uint_eq(testAddDirectory(UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                          UA_QUALIFIEDNAME(0, "FileSystem"), &b, NULL, &fsId),
                      UA_STATUSCODE_GOOD);

    UA_NodeId logsId;
    ck_assert(tryResolveChild(server_ft, fsId, "logs", &logsId));
    ck_assert(tryResolveChild(server_ft, logsId, "log1.txt", NULL));

    /* Create, write, read back and delete a file */
    UA_UInt32 h = 0;
    UA_NodeId fileId = callCreateFile(logsId, "log2.txt", true, &h,
                                      UA_STATUSCODE_GOOD);
    callWrite(fileId, h, "local backend", UA_STATUSCODE_GOOD);
    callClose(fileId, h, UA_STATUSCODE_GOOD);
    UA_ByteString content = readFileContent(fileId);
    ck_assert_uint_eq(content.length, 13);
    UA_ByteString_clear(&content);
    callDelete(logsId, fileId, UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&logsId);
    UA_NodeId_clear(&fileId);
    UA_NodeId_clear(&fsId);
    removeTree(scratchDir);
} END_TEST

START_TEST(localFileMount) {
    makeScratchDir();
    UA_FileTransferBackend directory;
    ck_assert_uint_eq(UA_FileTransferBackend_localDirectory(
        UA_STRING(scratchDir), &directory), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&directory, UA_STRING("single.txt"), false), UA_STATUSCODE_GOOD);
    directory.file.clear(&directory.file);

    UA_FileTransferFileBackend backend;
    memset(&backend, 0, sizeof(backend));
    ck_assert_uint_eq(UA_FileTransferFileBackend_localFile(UA_STRING_NULL, &backend),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(UA_FileTransferFileBackend_localFile(UA_STRING(scratchDir), &backend),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    char filePath[512];
    snprintf(filePath, sizeof(filePath), "%s/missing.txt", scratchDir);
    ck_assert_uint_eq(UA_FileTransferFileBackend_localFile(UA_STRING(filePath), &backend),
                      UA_STATUSCODE_BADNOTFOUND);
    ck_assert_ptr_null(backend.context);
    snprintf(filePath, sizeof(filePath), "%s/single.txt", scratchDir);
    ck_assert_uint_eq(UA_FileTransferFileBackend_localFile(UA_STRING(filePath), NULL),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(UA_FileTransferFileBackend_localFile(UA_STRING(filePath), &backend),
                      UA_STATUSCODE_GOOD);

    /* Only the empty backend path addresses the configured file. */
    const char *paths[] = {"single.txt", "sibling.txt", "..", "../other", filePath};
    UA_UInt32 handle = 0;
    UA_FileTransferFileInfo info;
    for(size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); i++) {
        UA_String path = UA_STRING((char*)(uintptr_t)paths[i]);
        ck_assert_uint_eq(backend.open(&backend, path, UA_OPENFILEMODE_READ, &handle),
                          UA_STATUSCODE_BADINVALIDARGUMENT);
        ck_assert_uint_eq(backend.getInfo(&backend, path, &info), UA_STATUSCODE_BADINVALIDARGUMENT);
    }
    filePath[0] = '!'; /* The factory copied the path. */
    UA_Driver *driver = NULL;
    UA_NodeId nodeId;
    ck_assert_uint_eq(UA_FileTransferDriver_newFile(server_ft, &backend, UA_STRING_NULL,
        NULL, &nodeId, &driver), UA_STATUSCODE_GOOD);
    UA_QualifiedName name;
    ck_assert_uint_eq(UA_Server_readBrowseName(server_ft, nodeId, &name), UA_STATUSCODE_GOOD);
    UA_String expectedName = UA_STRING("single.txt");
    ck_assert(UA_String_equal(&name.name, &expectedName));
    UA_QualifiedName_clear(&name);
    registerTestDriver(driver);
    handle = callOpen(nodeId, UA_OPENFILEMODE_WRITE, UA_STATUSCODE_GOOD);
    callWrite(nodeId, handle, "single file", UA_STATUSCODE_GOOD);
    callClose(nodeId, handle, UA_STATUSCODE_GOOD);
    UA_ByteString content = readFileContent(nodeId);
    UA_ByteString expected = UA_BYTESTRING("single file");
    ck_assert(UA_ByteString_equal(&content, &expected));
    UA_ByteString_clear(&content);
    ck_assert_uint_eq(testRemove(driver, nodeId), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&nodeId);
    removeTree(scratchDir);
} END_TEST

/* Sizes and positions beyond 2 GiB. The file is sparse, it takes no space. */
START_TEST(localDirectoryLargeFile) {
#ifndef _WIN32
    if(sizeof(long) < 8)
        return; /* The positions are long on POSIX */
#endif
    makeScratchDir();
    const UA_UInt64 size = (UA_UInt64)3 << 30;
    char path[256];
    snprintf(path, sizeof(path), "%s/big.bin", scratchDir);
#ifdef _WIN32
    wchar_t wpath[MAX_PATH];
    widePath(path, wpath);
    HANDLE h = CreateFileW(wpath, GENERIC_WRITE, 0, NULL, CREATE_NEW,
                           FILE_ATTRIBUTE_NORMAL, NULL);
    ck_assert(h != INVALID_HANDLE_VALUE);
    DWORD returned = 0;
    DeviceIoControl(h, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &returned, NULL);
    LARGE_INTEGER end;
    end.QuadPart = (LONGLONG)size;
    ck_assert(SetFilePointerEx(h, end, NULL, FILE_BEGIN));
    ck_assert(SetEndOfFile(h));
    CloseHandle(h);
#else
    FILE *fp = fopen(path, "wb");
    ck_assert_ptr_nonnull(fp);
    fclose(fp);
    ck_assert_int_eq(truncate(path, (off_t)size), 0);
#endif

    UA_FileTransferBackend b;
    ck_assert_uint_eq(UA_FileTransferBackend_localDirectory(
                          UA_STRING(scratchDir), &b), UA_STATUSCODE_GOOD);
    UA_NodeId fsId = UA_NODEID_NULL;
    ck_assert_uint_eq(testAddDirectory(UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                          UA_QUALIFIEDNAME(0, "FileSystem"), &b, NULL, &fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId fileId;
    ck_assert(tryResolveChild(server_ft, fsId, "big.bin", &fileId));

    UA_Variant value;
    readProperty(fileId, "Size", &value);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT64]));
    ck_assert_uint_eq(*(UA_UInt64*)value.data, size);
    UA_Variant_clear(&value);

    /* Read the last bytes. Positions beyond the end are clamped. */
    UA_UInt32 handle = callOpen(fileId, UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    callSetPosition(fileId, handle, size - 4);
    ck_assert_uint_eq(callGetPosition(fileId, handle), size - 4);
    UA_ByteString data = callRead(fileId, handle, 10, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(data.length, 4);
    UA_ByteString_clear(&data);
    callSetPosition(fileId, handle, size + 100);
    ck_assert_uint_eq(callGetPosition(fileId, handle), size);
    callClose(fileId, handle, UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileId);
    UA_NodeId_clear(&fsId);
    removeTree(scratchDir);
} END_TEST

/* The mandatory FileType properties are instantiated with the object */
START_TEST(instanceHasMandatoryProperties) {
    UA_NodeId fileNodeId = addFileTypeInstance(server_ft, "TestFileProps");

    const char *props[4] = {"Size", "Writable", "UserWritable", "OpenCount"};
    for(size_t i = 0; i < 4; i++) {
        UA_NodeId propId = resolveChild(server_ft, fileNodeId, props[i]);
        ck_assert(!UA_NodeId_isNull(&propId));
        UA_NodeId_clear(&propId);
    }

    UA_NodeId_clear(&fileNodeId);
} END_TEST

/**************************************
 * Regression Tests
 **************************************/

/* The driver hands the shared Namespace Zero Method nodes back when it stops,
 * so a stopped driver no longer answers calls on FileType Objects that belong
 * to the application or to another driver. */
START_TEST(stopReleasesTypeMethodCallbacks) {
    UA_NodeId file = addTestFile("File", "data", NULL);
    UA_NodeId_clear(&file);
    UA_MethodCallback cb = NULL;
    ck_assert_uint_eq(UA_Server_getMethodNodeCallback(
                          server_ft, UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_OPEN),
                          &cb), UA_STATUSCODE_GOOD);
    ck_assert(cb != NULL);

    ftDriver->stop(ftDriver);
    cb = NULL;
    ck_assert_uint_eq(UA_Server_getMethodNodeCallback(
                          server_ft, UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_OPEN),
                          &cb), UA_STATUSCODE_GOOD);
    ck_assert(cb == NULL);

    /* Restarting claims them again, so the fixture teardown is unaffected */
    ck_assert_uint_eq(ftDriver->start(ftDriver), UA_STATUSCODE_GOOD);
    cb = NULL;
    ck_assert_uint_eq(UA_Server_getMethodNodeCallback(
                          server_ft, UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_OPEN),
                          &cb), UA_STATUSCODE_GOOD);
    ck_assert(cb != NULL);
} END_TEST

static UA_Boolean
createEveryOptionalChild(UA_Server *s, const UA_NodeId *sessionId,
                         void *sessionContext, const UA_NodeId *sourceNodeId,
                         const UA_NodeId *targetParentNodeId,
                         const UA_NodeId *referenceTypeId) {
    return true;
}

/* An application may ask for every optional child to be instantiated. The
 * optional FileType Properties then already exist when the driver wires up the
 * Object and have to be reused: adding a second one would leave the Object with
 * a duplicate BrowseName, so TranslateBrowsePathsToNodeIds returns two targets
 * for one Property and only one of them carries the value source. */
START_TEST(optionalPropertiesAreNotDuplicated) {
    UA_Server *s = UA_Server_newForUnitTest();
    UA_GlobalNodeLifecycle lifecycle;
    memset(&lifecycle, 0, sizeof(lifecycle));
    lifecycle.createOptionalChild = createEveryOptionalChild;
    UA_Server_getConfig(s)->nodeLifecycle = &lifecycle;

    UA_FileTransferBackend b = memBackendWithFile("f.bin", "data");
    UA_NodeId fileId = UA_NODEID_NULL;
    UA_Driver *drv = newTestFile(s, &b.file, "OptFile", &fileId);
    ck_assert_ptr_nonnull(drv);
    ck_assert_uint_eq(UA_Server_addDriver(s, drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(drv->start(drv), UA_STATUSCODE_GOOD);

    /* Every Property of the FileType resolves to exactly one node */
    const char *properties[] = {"Size", "Writable", "UserWritable", "OpenCount",
                                "LastModifiedTime", "MaxByteStringLength",
                                "MimeType"};
    for(size_t i = 0; i < sizeof(properties) / sizeof(properties[0]); i++) {
        UA_QualifiedName qn = UA_QUALIFIEDNAME(0, (char*)(uintptr_t)properties[i]);
        UA_BrowsePathResult bpr =
            UA_Server_browseSimplifiedBrowsePath(s, fileId, 1, &qn);
        ck_assert_uint_eq(bpr.statusCode, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(bpr.targetsSize, 1);
        UA_BrowsePathResult_clear(&bpr);
    }

    /* The reused LastModifiedTime node carries the driver's value source */
    UA_QualifiedName lmName = UA_QUALIFIEDNAME(0, "LastModifiedTime");
    UA_BrowsePathResult bpr =
        UA_Server_browseSimplifiedBrowsePath(s, fileId, 1, &lmName);
    ck_assert_uint_eq(bpr.statusCode, UA_STATUSCODE_GOOD);
    UA_Variant value;
    UA_Variant_init(&value);
    ck_assert_uint_eq(UA_Server_readValue(s, bpr.targets[0].targetId.nodeId,
                                          &value), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATETIME]));
    ck_assert(*(UA_DateTime*)value.data > 0);
    UA_Variant_clear(&value);
    UA_BrowsePathResult_clear(&bpr);

    drv->stop(drv);
    ck_assert_uint_eq(UA_Server_removeDriver(s, drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(drv->free(drv), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileId);
    UA_Server_delete(s);
} END_TEST

/* MaxByteStringLength bounds Read and Write alike (Part 20, 4.2.1). A Read may
 * return less than requested, but silently truncating a Write would discard
 * client data, so an oversized chunk is rejected. */
START_TEST(fileWriteRespectsMaxByteStringLength) {
    UA_NodeId fileId = addTestFile("BigWriteFile", "", NULL);
    UA_UInt32 h = callOpen(fileId, UA_OPENFILEMODE_WRITE, UA_STATUSCODE_GOOD);

    UA_Variant value;
    readProperty(fileId, "MaxByteStringLength", &value);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT32]));
    UA_UInt32 maxLen = *(UA_UInt32*)value.data;
    UA_Variant_clear(&value);

    UA_ByteString chunk;
    ck_assert_uint_eq(UA_ByteString_allocBuffer(&chunk, (size_t)maxLen + 1),
                      UA_STATUSCODE_GOOD);
    memset(chunk.data, 'x', chunk.length);

    UA_Variant input[2];
    UA_Variant_setScalar(&input[0], &h, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&input[1], &chunk, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_CallMethodResult result =
        callMethod(fileId, UA_NS0ID_FILETYPE_WRITE, 2, input);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_BADINVALIDARGUMENT);
    UA_CallMethodResult_clear(&result);

    /* A chunk exactly at the announced limit is accepted */
    chunk.length = maxLen;
    UA_Variant_setScalar(&input[1], &chunk, &UA_TYPES[UA_TYPES_BYTESTRING]);
    result = callMethod(fileId, UA_NS0ID_FILETYPE_WRITE, 2, input);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&result);

    UA_ByteString_clear(&chunk);
    callClose(fileId, h, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(testRemove(driverForRoot(fileId), fileId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileId);
} END_TEST

/**************************************
 * Minimal Read-Only Test Backend
 *
 * Implements only what a read-only standalone file can reach. Used to check
 * that the driver does not demand stubs for operations it never calls.
 **************************************/

static const char minimalContent[] = "read-only";

/* The positions of the open handles. The handle is the index + 1. */
#define MINIMAL_MAXOPEN 8
static struct {
    UA_Boolean used;
    size_t pos;
} minimalFiles[MINIMAL_MAXOPEN];

static UA_StatusCode
minimalOpen(UA_FileTransferFileBackend *b, const UA_String path,
            UA_Byte mode, UA_UInt32 *handle) {
    if(mode & UA_OPENFILEMODE_WRITE)
        return UA_STATUSCODE_BADNOTWRITABLE;
    for(size_t i = 0; i < MINIMAL_MAXOPEN; i++) {
        if(minimalFiles[i].used)
            continue;
        minimalFiles[i].used = true;
        minimalFiles[i].pos = 0;
        *handle = (UA_UInt32)(i + 1);
        return UA_STATUSCODE_GOOD;
    }
    return UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
}

static size_t *
minimalPosition(UA_UInt32 handle) {
    if(handle == 0 || handle > MINIMAL_MAXOPEN || !minimalFiles[handle - 1].used)
        return NULL;
    return &minimalFiles[handle - 1].pos;
}

static UA_StatusCode
minimalClose(UA_FileTransferFileBackend *b, UA_UInt32 handle) {
    if(!minimalPosition(handle))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    minimalFiles[handle - 1].used = false;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
minimalRead(UA_FileTransferFileBackend *b, UA_UInt32 handle, UA_Int32 length,
            UA_ByteString *out) {
    size_t *pos = minimalPosition(handle);
    if(!pos)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    size_t total = sizeof(minimalContent) - 1;
    size_t remaining = (*pos < total) ? total - *pos : 0;
    size_t toRead = ((size_t)length < remaining) ? (size_t)length : remaining;
    if(toRead == 0) {
        UA_ByteString_init(out);
        return UA_STATUSCODE_GOOD;
    }
    UA_StatusCode res = UA_ByteString_allocBuffer(out, toRead);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    memcpy(out->data, minimalContent + *pos, toRead);
    *pos += toRead;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
minimalGetPosition(UA_FileTransferFileBackend *b, UA_UInt32 handle,
                   UA_UInt64 *outPosition) {
    size_t *pos = minimalPosition(handle);
    if(!pos)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    *outPosition = *pos;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
minimalSetPosition(UA_FileTransferFileBackend *b, UA_UInt32 handle,
                   UA_UInt64 position) {
    size_t *pos = minimalPosition(handle);
    if(!pos)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    size_t total = sizeof(minimalContent) - 1;
    *pos = (position < total) ? (size_t)position : total;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
minimalGetInfo(UA_FileTransferFileBackend *b, const UA_String path,
               UA_FileTransferFileInfo *outInfo) {
    strcpy(outInfo->name, "static");
    outInfo->size = sizeof(minimalContent) - 1;
    outInfo->accessRights = UA_FILEACCESS_READ;
    outInfo->lastModified = UA_DateTime_now();
    return UA_STATUSCODE_GOOD;
}

/* A file backend with only the operations to read */
static const UA_FileTransferFileBackend *
minimalBackend(void) {
    static UA_FileTransferFileBackend b;
    memset(&b, 0, sizeof(b));
    b.open = minimalOpen;
    b.close = minimalClose;
    b.read = minimalRead;
    b.getPosition = minimalGetPosition;
    b.setPosition = minimalSetPosition;
    b.getInfo = minimalGetInfo;
    return &b;
}

/* A read-only standalone file never reaches write, listDirectory, create,
 * remove or rename, so the backend need not supply stubs for
 * them. A writable mount still has to provide the mutating operations. */
START_TEST(readOnlyBackendNeedsNoWriteCallbacks) {
    FTConfig options;
    memset(&options, 0, sizeof(options));
    options.readOnly = true;

    UA_NodeId fileId = UA_NODEID_NULL;
    ck_assert_uint_eq(testAddFile(UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                          UA_QUALIFIEDNAME(0, "MinimalFile"), minimalBackend(),
                          UA_STRING("static"), &options, &fileId),
                      UA_STATUSCODE_GOOD);

    UA_ByteString content = readFileContent(fileId);
    ck_assert_uint_eq(content.length, strlen(minimalContent));
    ck_assert(memcmp(content.data, minimalContent, content.length) == 0);
    UA_ByteString_clear(&content);

    /* Writing is refused by the mount before the backend is consulted */
    callOpen(fileId, UA_OPENFILEMODE_WRITE, UA_STATUSCODE_BADNOTWRITABLE);

    /* The same backend is rejected for a writable mount */
    UA_NodeId rejected = UA_NODEID_NULL;
    ck_assert_uint_eq(testAddFile(UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                          UA_QUALIFIEDNAME(0, "WritableFile"), minimalBackend(),
                          UA_STRING("static"), NULL, &rejected),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert(UA_NodeId_isNull(&rejected));

    /* ... and for a read-only directory mount, which still needs a listing */
    UA_FileTransferBackend dirBackend;
    memset(&dirBackend, 0, sizeof(dirBackend));
    dirBackend.file = *minimalBackend();
    UA_NodeId rejectedFs = UA_NODEID_NULL;
    ck_assert_uint_eq(testAddDirectory(UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                          UA_QUALIFIEDNAME(0, "FileSystem"), &dirBackend, &options,
                          &rejectedFs),
                      UA_STATUSCODE_BADINVALIDARGUMENT);

    ck_assert_uint_eq(testRemove(driverForRoot(fileId), fileId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileId);
} END_TEST

/* The maxNodes ceiling has to hold for Objects created through the Methods as
 * well, otherwise a client grows the address space past the configured limit
 * one CreateFile call at a time. */
START_TEST(dirCreateRespectsMaxNodes) {
    FTConfig options;
    memset(&options, 0, sizeof(options));
    /* The mirrored tree is the root plus five entries */
    options.maxNodes = 7;
    UA_NodeId fsId = mountTree(&options);

    /* One more Object fits */
    UA_NodeId firstId = callCreateFile(fsId, "first.txt", false, NULL,
                                       UA_STATUSCODE_GOOD);
    ck_assert(!UA_NodeId_isNull(&firstId));

    /* The next one is refused, and nothing is created in the backend */
    callCreateFile(fsId, "second.txt", false, NULL,
                   UA_STATUSCODE_BADRESOURCEUNAVAILABLE);
    ck_assert(!tryResolveChild(server_ft, fsId, "second.txt", NULL));
    callCreateDirectory(fsId, "seconddir", UA_STATUSCODE_BADRESOURCEUNAVAILABLE);
    ck_assert(!tryResolveChild(server_ft, fsId, "seconddir", NULL));
    callMoveOrCopy(fsId, firstId, fsId, true, "copy.txt",
                   UA_STATUSCODE_BADRESOURCEUNAVAILABLE);
    ck_assert(!tryResolveChild(server_ft, fsId, "copy.txt", NULL));

    /* Deleting one frees the budget again */
    callDelete(fsId, firstId, UA_STATUSCODE_GOOD);
    UA_NodeId thirdId = callCreateFile(fsId, "third.txt", false, NULL,
                                       UA_STATUSCODE_GOOD);
    ck_assert(!UA_NodeId_isNull(&thirdId));

    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&firstId);
    UA_NodeId_clear(&thirdId);
    UA_NodeId_clear(&fsId);
} END_TEST

/* Part 20 does not constrain the namespace of the <FileName> placeholder. The
 * mount can place the storage-defined names in the server's own namespace
 * instead of the OPC UA namespace; the Part 20 Properties stay in namespace 0. */
START_TEST(mirroredNamesUseMountNamespace) {
    UA_UInt16 nsIdx = UA_Server_addNamespace(server_ft, "http://example.org/files");
    ck_assert_uint_gt(nsIdx, 0);

    FTConfig options;
    memset(&options, 0, sizeof(options));
    options.namespaceIndex = nsIdx;
    UA_NodeId fsId = mountTree(&options);

    /* The mirrored names live in the configured namespace */
    UA_QualifiedName readme = {nsIdx, UA_STRING_STATIC("readme.txt")};
    UA_BrowsePathResult bpr =
        UA_Server_browseSimplifiedBrowsePath(server_ft, fsId, 1, &readme);
    ck_assert_uint_eq(bpr.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(bpr.targetsSize, 1);
    UA_NodeId readmeId;
    UA_NodeId_copy(&bpr.targets[0].targetId.nodeId, &readmeId);
    UA_BrowsePathResult_clear(&bpr);

    /* ... and not in namespace 0. The generated NodeId is in the namespace of
     * the mount as well. */
    ck_assert(!tryResolveChild(server_ft, fsId, "readme.txt", NULL));
    ck_assert_uint_eq(readmeId.namespaceIndex, nsIdx);

    /* The Part 20 Properties of the mirrored file stay in namespace 0 */
    UA_NodeId sizeId = resolveChild(server_ft, readmeId, "Size");
    UA_NodeId_clear(&sizeId);

    /* Objects created through the Methods follow the same namespace */
    UA_NodeId createdId = callCreateFile(fsId, "created.txt", false, NULL,
                                         UA_STATUSCODE_GOOD);
    UA_QualifiedName created = {nsIdx, UA_STRING_STATIC("created.txt")};
    bpr = UA_Server_browseSimplifiedBrowsePath(server_ft, fsId, 1, &created);
    ck_assert_uint_eq(bpr.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(bpr.targetsSize, 1);
    UA_BrowsePathResult_clear(&bpr);

    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&createdId);
    UA_NodeId_clear(&readmeId);
    UA_NodeId_clear(&fsId);
} END_TEST

/* A backend whose listing of one subdirectory fails, to exercise the scan's
 * skip-and-warn path deterministically. Filesystem permissions cannot be used
 * for this: CI runs as root in a container, where chmod 000 does not stop
 * opendir, so the entry would be listed after all. */
static char failingListPath[MEM_MAXPATH];

static UA_StatusCode
failingListDirectory(UA_FileTransferBackend *b, const UA_String path,
                     UA_FileTransferListCallback cb, void *listContext) {
    if(strlen(failingListPath) == path.length &&
       memcmp(failingListPath, path.data, path.length) == 0)
        return UA_STATUSCODE_BADUSERACCESSDENIED;
    return memListDirectory(b, path, cb, listContext);
}

static UA_FileTransferBackend
memBackendWithUnlistableSubdir(const char *unlistable) {
    UA_FileTransferBackend b;
    ck_assert_uint_eq(memBackend(&b), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&b, UA_STRING("readable"), true),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&b, UA_STRING("readable/visible.txt"), false),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&b, UA_STRING("locked"), true),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&b, UA_STRING("locked/hidden.txt"), false),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_lt(strlen(unlistable), MEM_MAXPATH);
    strcpy(failingListPath, unlistable);
    b.listDirectory = failingListDirectory;
    return b;
}

/* One entry the server cannot list must not make the whole mount unavailable:
 * it is skipped with a warning, like an entry with an invalid name. */
START_TEST(mountSkipsUnreadableEntries) {
    UA_NodeId fsId = UA_NODEID_NULL;
    ck_assert_uint_eq(testAddDirectory(UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                          UA_QUALIFIEDNAME(0, "FileSystem"),
                          backendArg(memBackendWithUnlistableSubdir("locked")),
                          NULL, &fsId),
                      UA_STATUSCODE_GOOD);

    /* The readable part of the tree is mirrored */
    UA_NodeId readableId;
    ck_assert(tryResolveChild(server_ft, fsId, "readable", &readableId));
    ck_assert(tryResolveChild(server_ft, readableId, "visible.txt", NULL));

    /* The unlistable directory itself is represented but stays empty */
    UA_NodeId lockedId;
    ck_assert(tryResolveChild(server_ft, fsId, "locked", &lockedId));
    ck_assert(!tryResolveChild(server_ft, lockedId, "hidden.txt", NULL));

    /* A refresh does not fail over it either, and does not drop what is there */
    ck_assert_uint_eq(testRefresh(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    ck_assert(tryResolveChild(server_ft, fsId, "locked", NULL));
    ck_assert(tryResolveChild(server_ft, readableId, "visible.txt", NULL));

    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&readableId);
    UA_NodeId_clear(&lockedId);
    UA_NodeId_clear(&fsId);
} END_TEST

/* Counts the log messages above debug level whose format contains a marker */
static const char *logMarker;
static size_t logMarkerCount;

static void
countMarkerLog(void *context, UA_LogLevel level, UA_LogCategory category,
               const char *msg, va_list args) {
    if(level > UA_LOGLEVEL_DEBUG && logMarker && strstr(msg, logMarker))
        logMarkerCount++;
}

static UA_Logger markerLogger = {countMarkerLog, NULL, NULL};

/* Skipped entries and the limits are logged on the first refresh after start
 * and when they change. Unchanged repeats stay at debug level. */
START_TEST(scanSummaryLoggedOnChange) {
    UA_ServerConfig *config = UA_Server_getConfig(server_ft);
    UA_Logger *serverLogger = config->logging;
    config->logging = &markerLogger;

    logMarker = "cannot be mirrored or listed";
    logMarkerCount = 0;
    UA_NodeId fsId = UA_NODEID_NULL;
    ck_assert_uint_eq(testAddDirectory(UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                          UA_QUALIFIEDNAME(0, "FileSystem"),
                          backendArg(memBackendWithUnlistableSubdir("locked")),
                          NULL, &fsId), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(logMarkerCount, 1);
    UA_Driver *driver = driverForRoot(fsId);
    ck_assert_uint_eq(testRefresh(driver, fsId), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_FileTransferDriver_refresh(driver), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(logMarkerCount, 1);
    driver->stop(driver);
    ck_assert_uint_eq(driver->start(driver), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(logMarkerCount, 2);
    ck_assert_uint_eq(testRemove(driver, fsId), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fsId);

    logMarker = "max-scan-depth";
    logMarkerCount = 0;
    FTConfig options;
    memset(&options, 0, sizeof(options));
    options.maxScanDepth = 1;
    UA_FileTransferBackend b = memBackendWithTree();
    fsId = mountNamedMem(b, "Limited", &options);
    driver = driverForRoot(fsId);
    ck_assert_uint_eq(logMarkerCount, 1);
    ck_assert_uint_eq(testRefresh(driver, fsId), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(logMarkerCount, 1);
    ck_assert_uint_eq(createEntry(&b, UA_STRING("more"), true), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(testRefresh(driver, fsId), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(logMarkerCount, 2);
    ck_assert_uint_eq(testRefresh(driver, fsId), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(logMarkerCount, 2);
    ck_assert_uint_eq(testRemove(driver, fsId), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fsId);

    logMarker = NULL;
    config->logging = serverLogger;
} END_TEST

/* rename(2) replaces an existing target silently. An entry created behind the
 * driver's back must not be destroyed by a MoveOrCopy onto its name. */
START_TEST(moveOrCopyKeepsUnmirroredTarget) {
    makeScratchDir();

    UA_FileTransferBackend pre;
    ck_assert_uint_eq(UA_FileTransferBackend_localDirectory(
                          UA_STRING(scratchDir), &pre), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&pre, UA_STRING("source.txt"), false),
                      UA_STATUSCODE_GOOD);
    pre.file.clear(&pre.file);

    UA_FileTransferBackend b;
    ck_assert_uint_eq(UA_FileTransferBackend_localDirectory(
                          UA_STRING(scratchDir), &b), UA_STATUSCODE_GOOD);
    UA_NodeId fsId = UA_NODEID_NULL;
    ck_assert_uint_eq(testAddDirectory(UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                          UA_QUALIFIEDNAME(0, "FileSystem"), &b, NULL, &fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId sourceId;
    ck_assert(tryResolveChild(server_ft, fsId, "source.txt", &sourceId));

    /* Create a file the driver never mirrored */
    char victimPath[256];
    strcpy(victimPath, scratchDir);
    strcat(victimPath, "/victim.txt");
    FILE *fp = fopen(victimPath, "wb");
    ck_assert_ptr_nonnull(fp);
    ck_assert_uint_eq(fwrite("keep me", 1, 7, fp), 7);
    fclose(fp);

    /* Renaming onto it is refused instead of overwriting it */
    callMoveOrCopy(fsId, sourceId, fsId, false, "victim.txt",
                   UA_STATUSCODE_BADBROWSENAMEDUPLICATED);

    /* The file behind the driver's back is untouched */
    struct stat st;
    ck_assert_int_eq(stat(victimPath, &st), 0);
    ck_assert_uint_eq((size_t)st.st_size, 7);
    /* ... and the source still exists */
    ck_assert(tryResolveChild(server_ft, fsId, "source.txt", NULL));

    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&sourceId);
    UA_NodeId_clear(&fsId);
    removeTree(scratchDir);
} END_TEST

/* Failure injection for one backend path */
static char failingPath[MEM_MAXPATH];
static UA_StatusCode (*memRemoveFn)(UA_FileTransferBackend *b, const UA_String path);
static UA_StatusCode (*memCreateFn)(UA_FileTransferBackend *b, const UA_String path,
                                      const UA_FileTransferFileInfo *info);
static UA_StatusCode (*memOpenFn)(UA_FileTransferFileBackend *b,
                                  const UA_String path, UA_Byte mode,
                                  UA_UInt32 *handle);

static UA_Boolean
isFailingPath(const UA_String path) {
    return failingPath[0] != 0 && strlen(failingPath) == path.length &&
        memcmp(failingPath, path.data, path.length) == 0;
}

static UA_StatusCode
failingRemove(UA_FileTransferBackend *b, const UA_String path) {
    return isFailingPath(path) ? UA_STATUSCODE_BADUSERACCESSDENIED :
        memRemoveFn(b, path);
}

static UA_StatusCode
failingCreate(UA_FileTransferBackend *b, const UA_String path,
               const UA_FileTransferFileInfo *info) {
    return isFailingPath(path) ? UA_STATUSCODE_BADUSERACCESSDENIED :
        memCreateFn(b, path, info);
}

static UA_StatusCode
failingOpen(UA_FileTransferFileBackend *b, const UA_String path, UA_Byte mode,
            UA_UInt32 *handle) {
    return isFailingPath(path) ? UA_STATUSCODE_BADUSERACCESSDENIED :
        memOpenFn(b, path, mode, handle);
}

/* A move to another mount that cannot delete the source keeps the data. A
 * file is unchanged and its copy is removed again. A directory that is deleted
 * partially keeps the complete copy. Both mounts show their backend content. */
START_TEST(crossMountMoveKeepsDataOnFailedDelete) {
    UA_FileTransferBackend bA;
    ck_assert_uint_eq(memBackend(&bA), UA_STATUSCODE_GOOD);
    writeMemFile(&bA, "f.txt", "file");
    ck_assert_uint_eq(createEntry(&bA, UA_STRING("d"), true), UA_STATUSCODE_GOOD);
    writeMemFile(&bA, "d/one.txt", "1");
    writeMemFile(&bA, "d/two.txt", "2");
    memRemoveFn = bA.remove;
    bA.remove = failingRemove;
    MemBackendContext *ctxA = (MemBackendContext*)bA.file.context;
    UA_FileTransferBackend bB;
    ck_assert_uint_eq(memBackend(&bB), UA_STATUSCODE_GOOD);
    MemBackendContext *ctxB = (MemBackendContext*)bB.file.context;
    UA_NodeId fsA = mountNamedMem(bA, "FsA", NULL);
    UA_NodeId fsB = mountNamedMem(bB, "FsB", NULL);

    UA_NodeId aFile, aDir, bDir;
    ck_assert(tryResolveChild(server_ft, fsA, "f.txt", &aFile));
    strcpy(failingPath, "f.txt");
    callMoveOrCopy(fsA, aFile, fsB, false, "", UA_STATUSCODE_BADUSERACCESSDENIED);
    ck_assert_ptr_nonnull(memFind(ctxA, UA_STRING("f.txt")));
    ck_assert_ptr_null(memFind(ctxB, UA_STRING("f.txt")));
    ck_assert(tryResolveChild(server_ft, fsA, "f.txt", NULL));
    ck_assert(!tryResolveChild(server_ft, fsB, "f.txt", NULL));

    ck_assert(tryResolveChild(server_ft, fsA, "d", &aDir));
    strcpy(failingPath, "d");
    callMoveOrCopy(fsA, aDir, fsB, false, "", UA_STATUSCODE_BADUSERACCESSDENIED);
    ck_assert_ptr_nonnull(memFind(ctxB, UA_STRING("d/one.txt")));
    ck_assert_ptr_nonnull(memFind(ctxB, UA_STRING("d/two.txt")));
    ck_assert(tryResolveChild(server_ft, fsB, "d", &bDir));
    ck_assert(tryResolveChild(server_ft, bDir, "one.txt", NULL));
    ck_assert_ptr_null(memFind(ctxA, UA_STRING("d/one.txt")));
    ck_assert(tryResolveChild(server_ft, fsA, "d", NULL));
    ck_assert(!tryResolveChild(server_ft, aDir, "one.txt", NULL));
    failingPath[0] = 0;

    ck_assert_uint_eq(testRemove(driverForRoot(fsA), fsA),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(testRemove(driverForRoot(fsB), fsB),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&aFile);
    UA_NodeId_clear(&aDir);
    UA_NodeId_clear(&bDir);
    UA_NodeId_clear(&fsA);
    UA_NodeId_clear(&fsB);
} END_TEST

/* A failed copy of a directory removes the partial copy. A retry succeeds. */
START_TEST(failedDirectoryCopyLeavesNothing) {
    UA_FileTransferBackend bA;
    ck_assert_uint_eq(memBackend(&bA), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&bA, UA_STRING("d"), true), UA_STATUSCODE_GOOD);
    writeMemFile(&bA, "d/one.txt", "1");
    writeMemFile(&bA, "d/two.txt", "2");
    UA_FileTransferBackend bB;
    ck_assert_uint_eq(memBackend(&bB), UA_STATUSCODE_GOOD);
    memCreateFn = bB.create;
    bB.create = failingCreate;
    MemBackendContext *ctxB = (MemBackendContext*)bB.file.context;
    UA_NodeId fsA = mountNamedMem(bA, "FsA", NULL);
    UA_NodeId fsB = mountNamedMem(bB, "FsB", NULL);

    UA_NodeId aDir;
    ck_assert(tryResolveChild(server_ft, fsA, "d", &aDir));
    strcpy(failingPath, "d/two.txt");
    callMoveOrCopy(fsA, aDir, fsB, true, "", UA_STATUSCODE_BADUSERACCESSDENIED);
    ck_assert_ptr_null(memFind(ctxB, UA_STRING("d")));
    ck_assert_ptr_null(memFind(ctxB, UA_STRING("d/one.txt")));
    ck_assert(!tryResolveChild(server_ft, fsB, "d", NULL));

    failingPath[0] = 0;
    UA_NodeId copyId = callMoveOrCopy(fsA, aDir, fsB, true, "", UA_STATUSCODE_GOOD);
    ck_assert(tryResolveChild(server_ft, copyId, "two.txt", NULL));

    ck_assert_uint_eq(testRemove(driverForRoot(fsA), fsA),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(testRemove(driverForRoot(fsB), fsB),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&aDir);
    UA_NodeId_clear(&copyId);
    UA_NodeId_clear(&fsA);
    UA_NodeId_clear(&fsB);
} END_TEST

/* CreateFile removes the new file again when it cannot be opened as
 * requested */
START_TEST(createFileRemovedOnFailedOpen) {
    UA_FileTransferBackend b;
    ck_assert_uint_eq(memBackend(&b), UA_STATUSCODE_GOOD);
    memOpenFn = b.file.open;
    b.file.open = failingOpen;
    MemBackendContext *ctx = (MemBackendContext*)b.file.context;
    UA_NodeId fsId = mountNamedMem(b, "FileSystem", NULL);

    strcpy(failingPath, "new.txt");
    UA_UInt32 handle = 0;
    callCreateFile(fsId, "new.txt", true, &handle, UA_STATUSCODE_BADUSERACCESSDENIED);
    ck_assert_ptr_null(memFind(ctx, UA_STRING("new.txt")));
    ck_assert(!tryResolveChild(server_ft, fsId, "new.txt", NULL));

    /* Without the open request the file is created */
    UA_NodeId fileId = callCreateFile(fsId, "new.txt", false, NULL,
                                      UA_STATUSCODE_GOOD);
    ck_assert(!UA_NodeId_isNull(&fileId));
    failingPath[0] = 0;

    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileId);
    UA_NodeId_clear(&fsId);
} END_TEST

/* A Delete that fails partway removes the Objects of the entries that were
 * deleted from the backend. The remaining entries keep their Objects. */
START_TEST(partialDeleteRemovesDeletedObjects) {
    UA_FileTransferBackend b;
    ck_assert_uint_eq(memBackend(&b), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&b, UA_STRING("d"), true), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&b, UA_STRING("d/sub"), true), UA_STATUSCODE_GOOD);
    writeMemFile(&b, "d/one.txt", "1");
    writeMemFile(&b, "d/sub/two.txt", "2");
    memRemoveFn = b.remove;
    b.remove = failingRemove;
    MemBackendContext *ctx = (MemBackendContext*)b.file.context;
    UA_NodeId fsId = mountNamedMem(b, "FileSystem", NULL);

    UA_NodeId dId, subId;
    ck_assert(tryResolveChild(server_ft, fsId, "d", &dId));
    ck_assert(tryResolveChild(server_ft, dId, "sub", &subId));
    strcpy(failingPath, "d/sub");
    callDelete(fsId, dId, UA_STATUSCODE_BADUSERACCESSDENIED);
    failingPath[0] = 0;

    /* d/sub/two.txt is deleted before d/sub fails */
    ck_assert_ptr_null(memFind(ctx, UA_STRING("d/sub/two.txt")));
    ck_assert(!tryResolveChild(server_ft, subId, "two.txt", NULL));
    ck_assert(tryResolveChild(server_ft, dId, "sub", NULL));
    ck_assert(tryResolveChild(server_ft, fsId, "d", NULL));
    /* d/one.txt has an Object exactly if it still exists */
    ck_assert(tryResolveChild(server_ft, dId, "one.txt", NULL) ==
              (memFind(ctx, UA_STRING("d/one.txt")) != NULL));

    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&dId);
    UA_NodeId_clear(&subId);
    UA_NodeId_clear(&fsId);
} END_TEST

/* A directory is moved even when its content cannot be listed at the new
 * location. It is mirrored empty, as in the scan. */
START_TEST(moveOfUnlistableDirectorySucceeds) {
    UA_FileTransferBackend b;
    ck_assert_uint_eq(memBackend(&b), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&b, UA_STRING("d"), true), UA_STATUSCODE_GOOD);
    writeMemFile(&b, "d/x.txt", "x");
    b.listDirectory = failingListDirectory;
    strcpy(failingListPath, "e");
    UA_NodeId fsId = mountNamedMem(b, "FileSystem", NULL);

    UA_NodeId dId;
    ck_assert(tryResolveChild(server_ft, fsId, "d", &dId));
    UA_NodeId eId = callMoveOrCopy(fsId, dId, fsId, false, "e", UA_STATUSCODE_GOOD);
    ck_assert(!UA_NodeId_isNull(&eId));
    ck_assert(tryResolveChild(server_ft, fsId, "e", NULL));
    ck_assert(!tryResolveChild(server_ft, fsId, "d", NULL));
    failingListPath[0] = 0;

    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&dId);
    UA_NodeId_clear(&eId);
    UA_NodeId_clear(&fsId);
} END_TEST

static UA_StatusCode (*memGetInfoFn)(UA_FileTransferFileBackend *b,
                                     const UA_String path,
                                     UA_FileTransferFileInfo *outInfo);

static UA_StatusCode
failingGetInfo(UA_FileTransferFileBackend *b, const UA_String path,
               UA_FileTransferFileInfo *outInfo) {
    return isFailingPath(path) ? UA_STATUSCODE_BADUSERACCESSDENIED :
        memGetInfoFn(b, path, outInfo);
}

/* The storage metadata of the failing path is gone, as for a file deleted
 * behind the server's back. Its backend handles still work. */
static UA_StatusCode
vanishedGetInfo(UA_FileTransferFileBackend *b, const UA_String path,
                UA_FileTransferFileInfo *outInfo) {
    return isFailingPath(path) ? UA_STATUSCODE_BADNOTFOUND :
        memGetInfoFn(b, path, outInfo);
}

/* An open handle keeps working when its file vanishes from the storage. The
 * file cannot be opened again. */
START_TEST(openHandleSurvivesVanishedFile) {
    UA_FileTransferBackend b = memBackendWithFile("f.bin", "content");
    memGetInfoFn = b.file.getInfo;
    b.file.getInfo = vanishedGetInfo;
    UA_NodeId fileId = addTestFileBackend(b, "VanishingFile", NULL);
    UA_UInt32 h = callOpen(fileId, UA_OPENFILEMODE_READ | UA_OPENFILEMODE_WRITE,
                           UA_STATUSCODE_GOOD);
    strcpy(failingPath, "f.bin");
    callWrite(fileId, h, "more", UA_STATUSCODE_GOOD);
    callSetPosition(fileId, h, 0);
    UA_ByteString data = callRead(fileId, h, 100, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(data.length, 7);
    ck_assert_int_eq(memcmp(data.data, "moreent", 7), 0);
    UA_ByteString_clear(&data);
    callClose(fileId, h, UA_STATUSCODE_GOOD);
    callOpen(fileId, UA_OPENFILEMODE_READ, UA_STATUSCODE_BADNOTFOUND);
    failingPath[0] = 0;
    UA_NodeId_clear(&fileId);
} END_TEST

/* Replacing a parent directory with a file of the same name keeps the handles
 * below it usable, before and after the refresh turns them into zombies */
START_TEST(openHandleSurvivesReplacedParent) {
    UA_FileTransferBackend b;
    ck_assert_uint_eq(memBackend(&b), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&b, UA_STRING("d"), true), UA_STATUSCODE_GOOD);
    writeMemFile(&b, "d/f", "open");
    UA_NodeId fsId = mountNamedMem(b, "FileSystem", NULL);
    UA_NodeId dirId = resolveChild(server_ft, fsId, "d");
    UA_NodeId fId = resolveChild(server_ft, dirId, "f");
    UA_UInt32 h = callOpen(fId, UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b.remove(&b, UA_STRING("d/f")), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b.remove(&b, UA_STRING("d")), UA_STATUSCODE_GOOD);
    writeMemFile(&b, "d", "file");

    UA_ByteString data = callRead(fId, h, 10, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&data);
    ck_assert_uint_eq(testRefresh(driverForRoot(fsId), fsId), UA_STATUSCODE_GOOD);
    data = callRead(fId, h, 10, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&data);
    callClose(fId, h, UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&dirId);
    UA_NodeId_clear(&fId);
    UA_NodeId_clear(&fsId);
} END_TEST

/* Read at the end of the file returns an empty ByteString, not a null one
 * (Part 20, 4.2.4) */
START_TEST(readAtEndReturnsEmptyByteString) {
    UA_NodeId fileId = addTestFile("EofFile", "abc", NULL);
    UA_UInt32 h = callOpen(fileId, UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    UA_ByteString data = callRead(fileId, h, 10, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(data.length, 3);
    UA_ByteString_clear(&data);
    data = callRead(fileId, h, 10, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(data.length, 0);
    ck_assert_ptr_nonnull(data.data);
    UA_ByteString_clear(&data);
    callClose(fileId, h, UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileId);
} END_TEST

/* CreateFile removes the new file again when it cannot be mirrored. A retry
 * then does not fail on an entry the client cannot see. */
START_TEST(createFileRemovedOnFailedMirror) {
    UA_FileTransferBackend b;
    ck_assert_uint_eq(memBackend(&b), UA_STATUSCODE_GOOD);
    memGetInfoFn = b.file.getInfo;
    b.file.getInfo = failingGetInfo;
    MemBackendContext *ctx = (MemBackendContext*)b.file.context;
    UA_NodeId fsId = mountNamedMem(b, "FileSystem", NULL);

    strcpy(failingPath, "new.txt");
    callCreateFile(fsId, "new.txt", false, NULL, UA_STATUSCODE_BADUSERACCESSDENIED);
    ck_assert_ptr_null(memFind(ctx, UA_STRING("new.txt")));
    ck_assert(!tryResolveChild(server_ft, fsId, "new.txt", NULL));
    failingPath[0] = 0;

    UA_NodeId fileId = callCreateFile(fsId, "new.txt", false, NULL,
                                      UA_STATUSCODE_GOOD);
    ck_assert(!UA_NodeId_isNull(&fileId));

    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileId);
    UA_NodeId_clear(&fsId);
} END_TEST

#ifndef _WIN32
/* The local filesystem backend serves a symbolic link to a file as the file.
 * Links to directories and special files are not served. Deleting never
 * descends into a link target: a directory with an entry that is not served is
 * not empty for the driver, a served link is removed itself. */
START_TEST(localDirectorySkipsLinksAndSpecialFiles) {
    makeScratchDir();
    char root[256], path[320], target[320];
    snprintf(root, sizeof(root), "%s/root", scratchDir);
    ck_assert_int_eq(mkdir(root, 0755), 0);
    snprintf(path, sizeof(path), "%s/outside", scratchDir);
    ck_assert_int_eq(mkdir(path, 0755), 0);
    snprintf(path, sizeof(path), "%s/outside/secret.txt", scratchDir);
    FILE *fp = fopen(path, "wb");
    ck_assert_ptr_nonnull(fp);
    ck_assert_uint_eq(fwrite("secret", 1, 6, fp), 6);
    fclose(fp);

    snprintf(target, sizeof(target), "%s/outside", scratchDir);
    snprintf(path, sizeof(path), "%s/dirlink", root);
    ck_assert_int_eq(symlink(target, path), 0);
    snprintf(path, sizeof(path), "%s/loop", root);
    ck_assert_int_eq(symlink(".", path), 0);
    snprintf(target, sizeof(target), "%s/outside/secret.txt", scratchDir);
    snprintf(path, sizeof(path), "%s/filelink.txt", root);
    ck_assert_int_eq(symlink(target, path), 0);
    snprintf(path, sizeof(path), "%s/pipe", root);
    ck_assert_int_eq(mkfifo(path, 0644), 0);
    snprintf(path, sizeof(path), "%s/d", root);
    ck_assert_int_eq(mkdir(path, 0755), 0);
    snprintf(target, sizeof(target), "%s/outside", scratchDir);
    snprintf(path, sizeof(path), "%s/d/inner", root);
    ck_assert_int_eq(symlink(target, path), 0);

    UA_FileTransferBackend b;
    ck_assert_uint_eq(UA_FileTransferBackend_localDirectory(UA_STRING(root), &b),
                      UA_STATUSCODE_GOOD);
    UA_NodeId fsId = UA_NODEID_NULL;
    ck_assert_uint_eq(testAddDirectory(UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                          UA_QUALIFIEDNAME(0, "FileSystem"), &b, NULL, &fsId),
                      UA_STATUSCODE_GOOD);

    ck_assert(!tryResolveChild(server_ft, fsId, "dirlink", NULL));
    ck_assert(!tryResolveChild(server_ft, fsId, "loop", NULL));
    ck_assert(!tryResolveChild(server_ft, fsId, "pipe", NULL));
    UA_NodeId fileLinkId, dId;
    ck_assert(tryResolveChild(server_ft, fsId, "filelink.txt", &fileLinkId));
    UA_ByteString content = readFileContent(fileLinkId);
    ck_assert_uint_eq(content.length, 6);
    UA_ByteString_clear(&content);
    ck_assert(tryResolveChild(server_ft, fsId, "d", &dId));
    ck_assert(!tryResolveChild(server_ft, dId, "inner", NULL));

    /* The link targets are kept */
    callDelete(fsId, dId, UA_STATUSCODE_BADINVALIDSTATE);
    callDelete(fsId, fileLinkId, UA_STATUSCODE_GOOD);
    struct stat st;
    snprintf(path, sizeof(path), "%s/d/inner", root);
    ck_assert_int_eq(lstat(path, &st), 0);
    ck_assert(tryResolveChild(server_ft, fsId, "d", NULL));
    snprintf(path, sizeof(path), "%s/filelink.txt", root);
    ck_assert_int_ne(lstat(path, &st), 0);
    snprintf(path, sizeof(path), "%s/outside/secret.txt", scratchDir);
    ck_assert_int_eq(stat(path, &st), 0);
    ck_assert_uint_eq((size_t)st.st_size, 6);

    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileLinkId);
    UA_NodeId_clear(&dId);
    UA_NodeId_clear(&fsId);
    removeTree(scratchDir);
} END_TEST
#endif /* !_WIN32 */

/* Mounts can be added before the driver is started. The Methods are served
 * once it is started. */
START_TEST(mountBeforeStart) {
    startTestDrivers = false;
    UA_NodeId fileId = addTestFile("EarlyFile", "early", NULL);
    ck_assert_uint_eq(ftDriver->start(ftDriver), UA_STATUSCODE_GOOD);
    UA_ByteString data = readFileContent(fileId);
    ck_assert_uint_eq(data.length, strlen("early"));
    UA_ByteString_clear(&data);
    ck_assert_uint_eq(testRemove(driverForRoot(fileId), fileId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileId);
} END_TEST

/* Each backend has its own driver. Stopping one leaves the other operational. */
START_TEST(multipleDriversHaveIndependentLifecycle) {
    /* An unrelated generic driver may even have the same display name. */
    UA_Driver unrelated;
    memset(&unrelated, 0, sizeof(unrelated));
    unrelated.driverType = UA_DRIVERTYPE_GENERIC;
    unrelated.name = UA_STRING("file-transfer");
    unrelated.state = UA_LIFECYCLESTATE_STARTED;
    ck_assert_uint_eq(UA_Server_addDriver(server_ft, &unrelated), UA_STATUSCODE_GOOD);
    UA_NodeId first = addTestFile("First", "one", NULL);
    UA_Driver *firstDriver = ftDriver;
    UA_NodeId second = addTestFile("Second", "two", NULL);
    UA_Driver *secondDriver = ftDriver;
    ck_assert_uint_eq(firstDriver->driverType, UA_DRIVERTYPE_GENERIC);
    ck_assert_uint_eq(secondDriver->driverType, UA_DRIVERTYPE_GENERIC);
    firstDriver->name = UA_STRING("Custom name");
    /* Standard Methods need no per-Object NodeId allocation. Rebinding is
     * still idempotent even though there is no allocation to mark it. */
    FTEntry *firstEntry = ((FileTransferDriver*)firstDriver)->root;
    ck_assert(firstEntry->methodsBound);
    ck_assert_ptr_null(firstEntry->methods);
    ck_assert_uint_eq(bindObjectMethods(server_ft, firstEntry), UA_STATUSCODE_GOOD);
    ck_assert_ptr_null(firstEntry->methods);
    UA_UInt32 firstHandle = callOpen(first, UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    /* Wrapping skips zero and a handle already used by another driver. */
    ((FileTransferDriver*)secondDriver)->nextHandle = UA_UINT32_MAX;
    UA_UInt32 secondHandle = callOpen(second, UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(secondHandle, 0);
    ck_assert_uint_ne(firstHandle, secondHandle);
    firstDriver->stop(firstDriver);
    ck_assert(!firstEntry->methodsBound);
    ck_assert_uint_eq(readOpenCount(first), 0);
    ck_assert_uint_eq(readOpenCount(second), 1);
    UA_ByteString data = callRead(second, secondHandle, 8, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(data.length, 3);
    ck_assert_int_eq(memcmp(data.data, "two", 3), 0);
    UA_ByteString_clear(&data);
    callOpen(first, UA_OPENFILEMODE_READ, UA_STATUSCODE_BADNOTSUPPORTED);
    ck_assert_uint_eq(firstDriver->start(firstDriver), UA_STATUSCODE_GOOD);
    ck_assert(firstEntry->methodsBound);
    ck_assert_ptr_null(firstEntry->methods);
    data = readFileContent(first);
    ck_assert_uint_eq(data.length, 3);
    UA_ByteString_clear(&data);
    secondDriver->stop(secondDriver);
    data = readFileContent(first);
    UA_ByteString_clear(&data);
    firstDriver->stop(firstDriver);
    UA_MethodCallback callback = NULL;
    ck_assert_uint_eq(UA_Server_getMethodNodeCallback(server_ft, UA_NS0ID(FILETYPE_OPEN),
                                                     &callback), UA_STATUSCODE_GOOD);
    ck_assert(callback == NULL); /* An unrelated started driver keeps no bindings. */
    unrelated.state = UA_LIFECYCLESTATE_STOPPED;
    ck_assert_uint_eq(UA_Server_removeDriver(server_ft, &unrelated), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&first);
    UA_NodeId_clear(&second);
} END_TEST

/* A namespaceIndex that does not resolve in the server's namespace array is
 * rejected instead of producing Objects in a namespace no client can read. */
START_TEST(mountRejectsUnknownNamespace) {
    size_t nsSize = 0;
    UA_String uri = UA_STRING_NULL;
    while(UA_Server_getNamespaceByIndex(server_ft, nsSize, &uri) ==
          UA_STATUSCODE_GOOD) {
        UA_String_clear(&uri);
        nsSize++;
    }
    ck_assert_uint_gt(nsSize, 0);

    FTConfig options;
    memset(&options, 0, sizeof(options));
    options.namespaceIndex = (UA_UInt16)nsSize; /* one past the last index */

    UA_NodeId fsId = UA_NODEID_NULL;
    ck_assert_uint_eq(testAddDirectory(UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                          UA_QUALIFIEDNAME(0, "FileSystem"),
                          backendArg(memBackendWithTree()), &options, &fsId),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert(UA_NodeId_isNull(&fsId));
} END_TEST

/* Call a Method node of the object by its BrowseName */
static UA_CallMethodResult
callObjectMethod(const UA_NodeId objectId, const char *name,
                 size_t inputSize, UA_Variant *input) {
    UA_CallMethodRequest request;
    UA_CallMethodRequest_init(&request);
    request.objectId = objectId;
    request.methodId = resolveChild(server_ft, objectId, name);
    request.inputArgumentsSize = inputSize;
    request.inputArguments = input;
    UA_CallMethodResult result = UA_Server_call(server_ft, &request);
    UA_NodeId_clear(&request.methodId);
    return result;
}

/* Deleting a node keeps a child that has another hierarchical parent. The
 * driver deletes every Object explicitly and releases the value sources of the
 * Properties, which point to its registry entries. */
START_TEST(removalReleasesValueSources) {
    UA_NodeId fsId = mountTree(NULL);
    UA_NodeId readmeId = resolveChild(server_ft, fsId, "readme.txt");
    UA_NodeId docsId = resolveChild(server_ft, fsId, "docs");
    UA_NodeId aId = resolveChild(server_ft, docsId, "a.txt");
    const char *properties[] = {"Size", "OpenCount", "MaxByteStringLength"};
    const size_t types[] = {UA_TYPES_UINT64, UA_TYPES_UINT16, UA_TYPES_UINT32};
    UA_NodeId propertyIds[3];
    for(size_t i = 0; i < 3; i++)
        propertyIds[i] = resolveChild(server_ft, aId, properties[i]);

    /* A second parent for a file and for a Property */
    UA_ExpandedNodeId target = UA_EXPANDEDNODEID_NULL;
    target.nodeId = readmeId;
    ck_assert_uint_eq(UA_Server_addReference(server_ft, UA_NS0ID(OBJECTSFOLDER),
                                             UA_NS0ID(ORGANIZES), target, true),
                      UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 3; i++) {
        target.nodeId = propertyIds[i];
        ck_assert_uint_eq(UA_Server_addReference(server_ft, UA_NS0ID(OBJECTSFOLDER),
            UA_NS0ID(ORGANIZES), target, true), UA_STATUSCODE_GOOD);
    }

    /* Surviving Properties keep typed values and no driver context. */
    callDelete(fsId, docsId, UA_STATUSCODE_GOOD);
    UA_NodeClass nodeClass;
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ft, aId, &nodeClass),
                      UA_STATUSCODE_BADNODEIDUNKNOWN);
    for(size_t i = 0; i < 3; i++) {
        UA_Variant value;
        ck_assert_uint_eq(UA_Server_readValue(server_ft, propertyIds[i], &value),
                          UA_STATUSCODE_GOOD);
        ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[types[i]]));
        UA_Variant_clear(&value);
        void *context;
        ck_assert_uint_eq(UA_Server_getNodeContext(server_ft, propertyIds[i], &context),
                          UA_STATUSCODE_GOOD);
        ck_assert_ptr_null(context);
        UA_NodeId_clear(&propertyIds[i]);
    }

    /* Removing the mount deletes the file with the second parent */
    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ft, readmeId, &nodeClass),
                      UA_STATUSCODE_BADNODEIDUNKNOWN);

    UA_NodeId_clear(&readmeId);
    UA_NodeId_clear(&docsId);
    UA_NodeId_clear(&aId);
    UA_NodeId_clear(&fsId);
} END_TEST

/* A backend that commits the content at close, and the commit fails */
static UA_StatusCode
failingClose(UA_FileTransferFileBackend *b, UA_UInt32 handle) {
    MemOpenFile *of = memHandle(b, handle);
    UA_Boolean fail = isFailingPath(UA_STRING(of->entry->path));
    if(fail)
        UA_ByteString_clear(&of->entry->content);
    memClose(b, handle);
    return fail ? UA_STATUSCODE_BADUNEXPECTEDERROR : UA_STATUSCODE_GOOD;
}

/* Closing is the flush boundary of a backend. A copy whose destination cannot
 * be closed fails, removes the destination and keeps the source of a move. */
START_TEST(copyFailsOnDestinationClose) {
    UA_FileTransferBackend bA;
    ck_assert_uint_eq(memBackend(&bA), UA_STATUSCODE_GOOD);
    writeMemFile(&bA, "f.txt", "file");
    ck_assert_uint_eq(createEntry(&bA, UA_STRING("d"), true), UA_STATUSCODE_GOOD);
    writeMemFile(&bA, "d/one.txt", "1");
    MemBackendContext *ctxA = (MemBackendContext*)bA.file.context;
    UA_FileTransferBackend bB;
    ck_assert_uint_eq(memBackend(&bB), UA_STATUSCODE_GOOD);
    bB.file.close = failingClose;
    MemBackendContext *ctxB = (MemBackendContext*)bB.file.context;
    UA_NodeId fsA = mountNamedMem(bA, "FsA", NULL);
    UA_NodeId fsB = mountNamedMem(bB, "FsB", NULL);

    UA_NodeId aFile, aDir;
    ck_assert(tryResolveChild(server_ft, fsA, "f.txt", &aFile));
    ck_assert(tryResolveChild(server_ft, fsA, "d", &aDir));
    strcpy(failingPath, "f.txt");
    callMoveOrCopy(fsA, aFile, fsB, true, "", UA_STATUSCODE_BADUNEXPECTEDERROR);
    callMoveOrCopy(fsA, aFile, fsB, false, "", UA_STATUSCODE_BADUNEXPECTEDERROR);
    ck_assert_ptr_null(memFind(ctxB, UA_STRING("f.txt")));
    ck_assert(!tryResolveChild(server_ft, fsB, "f.txt", NULL));
    MemEntry *source = memFind(ctxA, UA_STRING("f.txt"));
    ck_assert_ptr_nonnull(source);
    ck_assert_uint_eq(source->content.length, 4);
    ck_assert(tryResolveChild(server_ft, fsA, "f.txt", NULL));

    strcpy(failingPath, "d/one.txt");
    callMoveOrCopy(fsA, aDir, fsB, false, "", UA_STATUSCODE_BADUNEXPECTEDERROR);
    ck_assert_ptr_null(memFind(ctxB, UA_STRING("d")));
    ck_assert_ptr_nonnull(memFind(ctxA, UA_STRING("d/one.txt")));
    ck_assert(tryResolveChild(server_ft, fsA, "d", NULL));
    failingPath[0] = 0;

    ck_assert_uint_eq(testRemove(driverForRoot(fsA), fsA),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(testRemove(driverForRoot(fsB), fsB),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&aFile);
    UA_NodeId_clear(&aDir);
    UA_NodeId_clear(&fsA);
    UA_NodeId_clear(&fsB);
} END_TEST

/* With copyMethodsOnInstances the Objects get their own Method copies. The
 * copies made while the driver is stopped are served once it is started. */
START_TEST(copiedMethodsServedAfterStart) {
    startTestDrivers = false;
    UA_ServerConfig *config = UA_Server_getConfig(server_ft);
    config->copyMethodsOnInstances = true;
    UA_NodeId fileId = addTestFile("CopiedFile", "copied", NULL);
    UA_NodeId fsId = mountTree(NULL);
    config->copyMethodsOnInstances = false;
    for(UA_Driver *drv = UA_Server_getDrivers(server_ft); drv; drv = drv->next) {
        if(isFileTransferDriver(drv))
            ck_assert_uint_eq(drv->start(drv), UA_STATUSCODE_GOOD);
    }

    UA_NodeId openId = resolveChild(server_ft, fileId, "Open");
    UA_NodeId typeOpenId = UA_NS0ID(FILETYPE_OPEN);
    ck_assert(!UA_NodeId_equal(&openId, &typeOpenId));
    UA_NodeId_clear(&openId);

    /* The Methods of a standalone file, a mirrored file and a directory */
    UA_Byte mode = UA_OPENFILEMODE_READ;
    UA_Variant input[2];
    UA_Variant_setScalar(&input[0], &mode, &UA_TYPES[UA_TYPES_BYTE]);
    UA_CallMethodResult result = callObjectMethod(fileId, "Open", 1, input);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&result);
    UA_NodeId readmeId = resolveChild(server_ft, fsId, "readme.txt");
    result = callObjectMethod(readmeId, "Open", 1, input);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&result);
    UA_String name = UA_STRING("created.txt");
    UA_Boolean requestOpen = false;
    UA_Variant_setScalar(&input[0], &name, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&input[1], &requestOpen, &UA_TYPES[UA_TYPES_BOOLEAN]);
    result = callObjectMethod(fsId, "CreateFile", 2, input);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&result);

    ck_assert_uint_eq(testRemove(driverForRoot(fileId), fileId),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&readmeId);
    UA_NodeId_clear(&fileId);
    UA_NodeId_clear(&fsId);
} END_TEST

/* The failing path is not writable in the storage */
static UA_StatusCode
unwritableGetInfo(UA_FileTransferFileBackend *b, const UA_String path,
                  UA_FileTransferFileInfo *outInfo) {
    UA_StatusCode res = memGetInfo(b, path, outInfo);
    if(isFailingPath(path))
        outInfo->accessRights &= (UA_FileAccessRights)~UA_FILEACCESS_WRITE;
    return res;
}

static UA_StatusCode
denyUserWrite(UA_FileTransferFileBackend *b, UA_Server *server,
              const UA_NodeId *sessionId, void *sessionContext,
              const UA_NodeId *fileNodeId,
              UA_FileAccessRights *outRights) {
    *outRights = UA_FILEACCESS_READ | UA_FILEACCESS_TRAVERSE;
    return UA_STATUSCODE_GOOD;
}

static UA_Boolean
readBooleanProperty(const UA_NodeId fileId, const char *name) {
    UA_Variant value;
    readProperty(fileId, name, &value);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_BOOLEAN]));
    UA_Boolean b = *(UA_Boolean*)value.data;
    UA_Variant_clear(&value);
    return b;
}

/* Writable is the storage writability within the mount configuration.
 * UserWritable restricts it for the user and never grants more (Part 20,
 * 4.2.1). Both follow the storage without a refresh. */
START_TEST(userWritableFollowsStorage) {
    UA_FileTransferBackend b = memBackendWithFile("f.bin", "data");
    b.file.getInfo = unwritableGetInfo;
    UA_NodeId fileId = UA_NODEID_NULL;
    ck_assert_uint_eq(testAddFile(UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                          UA_QUALIFIEDNAME(0, "StorageFile"), &b.file,
                          UA_STRING("f.bin"), NULL, &fileId),
                      UA_STATUSCODE_GOOD);
    strcpy(failingPath, "f.bin");
    ck_assert(!readBooleanProperty(fileId, "Writable"));
    ck_assert(!readBooleanProperty(fileId, "UserWritable"));
    callOpen(fileId, UA_OPENFILEMODE_WRITE, UA_STATUSCODE_BADNOTWRITABLE);
    UA_UInt32 h = callOpen(fileId, UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    callClose(fileId, h, UA_STATUSCODE_GOOD);
    failingPath[0] = 0;
    ck_assert(readBooleanProperty(fileId, "Writable"));
    ck_assert(readBooleanProperty(fileId, "UserWritable"));
    h = callOpen(fileId, UA_OPENFILEMODE_WRITE, UA_STATUSCODE_GOOD);
    strcpy(failingPath, "f.bin");
    callWrite(fileId, h, "blocked", UA_STATUSCODE_BADNOTWRITABLE);
    callClose(fileId, h, UA_STATUSCODE_GOOD);
    failingPath[0] = 0;
    ck_assert_uint_eq(testRemove(driverForRoot(fileId), fileId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileId);

    b = memBackendWithFile("f.bin", "data");
    b.file.getUserAccessRights = denyUserWrite;
    fileId = addTestFileBackend(b, "UserFile", NULL);
    ck_assert(readBooleanProperty(fileId, "Writable"));
    ck_assert(!readBooleanProperty(fileId, "UserWritable"));
    callOpen(fileId, UA_OPENFILEMODE_WRITE, UA_STATUSCODE_BADNOTWRITABLE);
    ck_assert_uint_eq(testRemove(driverForRoot(fileId), fileId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileId);
} END_TEST

/* The callbacks select one Object, independently of the other Objects of the
 * mount. A null target applies the policy to all Objects. */
typedef struct {
    UA_NodeId target;
    UA_FileAccessRights general;
    UA_FileAccessRights user;
    UA_StatusCode generalStatus;
    UA_StatusCode userStatus;
} AccessPolicy;

static UA_StatusCode
objectAccessRights(UA_FileTransferFileBackend *b, UA_Server *server,
                    const UA_NodeId *nodeId,
                    UA_FileAccessRights *outRights) {
    MemBackendContext *ctx = (MemBackendContext*)b->context;
    AccessPolicy *policy = (AccessPolicy*)ctx->accessPolicy;
    ck_assert_uint_eq(*outRights, 0);
    *outRights = UA_FILEACCESS_READ | UA_FILEACCESS_WRITE | UA_FILEACCESS_TRAVERSE;
    if(!UA_NodeId_isNull(&policy->target) &&
       !UA_NodeId_equal(nodeId, &policy->target))
        return UA_STATUSCODE_GOOD;
    *outRights = policy->general;
    return policy->generalStatus;
}

static UA_StatusCode
objectUserAccessRights(UA_FileTransferFileBackend *b, UA_Server *server,
                        const UA_NodeId *sessionId, void *sessionContext,
                        const UA_NodeId *nodeId,
                        UA_FileAccessRights *outRights) {
    MemBackendContext *ctx = (MemBackendContext*)b->context;
    AccessPolicy *policy = (AccessPolicy*)ctx->accessPolicy;
    ck_assert_ptr_eq(server, server_ft);
    ck_assert_ptr_nonnull(sessionId);
    ck_assert(UA_NodeId_equal(sessionId, &server_ft->adminSession.sessionId));
    ck_assert_ptr_nonnull(sessionContext);
    ck_assert_ptr_eq(sessionContext, server_ft->adminSession.context);
    ck_assert_uint_eq(*outRights, 0);
    *outRights = UA_FILEACCESS_READ | UA_FILEACCESS_WRITE | UA_FILEACCESS_TRAVERSE;
    if(!UA_NodeId_isNull(&policy->target) &&
       !UA_NodeId_equal(nodeId, &policy->target))
        return UA_STATUSCODE_GOOD;
    *outRights = policy->user;
    return policy->userStatus;
}

static int accessSessionContext;

static UA_FileTransferBackend
accessBackend(UA_FileTransferBackend backend, AccessPolicy *policy) {
    memset(policy, 0, sizeof(*policy));
    policy->general = policy->user =
        UA_FILEACCESS_READ | UA_FILEACCESS_WRITE | UA_FILEACCESS_TRAVERSE;
    backend.file.getAccessRights = objectAccessRights;
    backend.file.getUserAccessRights = objectUserAccessRights;
    ((MemBackendContext*)backend.file.context)->accessPolicy = policy;
    server_ft->adminSession.context = &accessSessionContext;
    return backend;
}

START_TEST(fileAccessRightsEnforced) {
    AccessPolicy policy;
    UA_FileTransferBackend backend =
        accessBackend(memBackendWithFile("f.bin", "original"), &policy);
    UA_NodeId fileId = addTestFileBackend(backend, "Access", NULL);
    policy.target = fileId;

    policy.general = UA_FILEACCESS_READ;
    ck_assert(!readBooleanProperty(fileId, "Writable"));
    ck_assert(!readBooleanProperty(fileId, "UserWritable"));
    callOpen(fileId, UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING,
             UA_STATUSCODE_BADNOTWRITABLE);
    policy.general |= UA_FILEACCESS_WRITE;
    policy.user = UA_FILEACCESS_READ;
    ck_assert(readBooleanProperty(fileId, "Writable"));
    ck_assert(!readBooleanProperty(fileId, "UserWritable"));
    callOpen(fileId, UA_OPENFILEMODE_WRITE, UA_STATUSCODE_BADNOTWRITABLE);
    policy.user = UA_FILEACCESS_WRITE;
    callOpen(fileId, UA_OPENFILEMODE_READ, UA_STATUSCODE_BADNOTREADABLE);
    policy.user |= UA_FILEACCESS_READ;

    UA_UInt32 h = callOpen(fileId, UA_OPENFILEMODE_READ | UA_OPENFILEMODE_WRITE,
                           UA_STATUSCODE_GOOD);
    /* A changed Session context must reach checks on existing handles. */
    server_ft->adminSession.context = &policy;
    /* Revoking either mask affects existing handles, including empty writes. */
    policy.user = UA_FILEACCESS_READ;
    callWrite(fileId, h, "replaced", UA_STATUSCODE_BADNOTWRITABLE);
    callWrite(fileId, h, "", UA_STATUSCODE_BADNOTWRITABLE);
    policy.user = UA_FILEACCESS_READ | UA_FILEACCESS_WRITE;
    policy.general = UA_FILEACCESS_READ;
    callWrite(fileId, h, "replaced", UA_STATUSCODE_BADNOTWRITABLE);
    policy.general = UA_FILEACCESS_WRITE;
    UA_ByteString data = callRead(fileId, h, 8, UA_STATUSCODE_BADNOTREADABLE);
    UA_ByteString_clear(&data);
    policy.general |= UA_FILEACCESS_READ;
    policy.user = UA_FILEACCESS_WRITE;
    data = callRead(fileId, h, 8, UA_STATUSCODE_BADNOTREADABLE);
    UA_ByteString_clear(&data);
    policy.user |= UA_FILEACCESS_READ;

    /* A callback error cannot be hidden by an otherwise permissive mask. */
    policy.generalStatus = UA_STATUSCODE_BADINTERNALERROR;
    callWrite(fileId, h, "replaced", UA_STATUSCODE_BADINTERNALERROR);
    UA_NodeId writableId = resolveChild(server_ft, fileId, "Writable");
    UA_Variant value;
    UA_Variant_init(&value);
    ck_assert_uint_eq(UA_Server_readValue(server_ft, writableId, &value),
                      UA_STATUSCODE_BADINTERNALERROR);
    UA_Variant_clear(&value);
    UA_NodeId_clear(&writableId);
    policy.generalStatus = UA_STATUSCODE_GOOD;
    policy.userStatus = UA_STATUSCODE_BADINTERNALERROR;
    callWrite(fileId, h, "replaced", UA_STATUSCODE_BADINTERNALERROR);
    policy.userStatus = UA_STATUSCODE_GOOD;
    data = callRead(fileId, h, 8, UA_STATUSCODE_GOOD);
    UA_ByteString expected = UA_BYTESTRING("original");
    ck_assert(UA_ByteString_equal(&data, &expected));
    UA_ByteString_clear(&data);
    policy.general = policy.user = 0;
    callClose(fileId, h, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(testRemove(driverForRoot(fileId), fileId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileId);
} END_TEST

START_TEST(directoryAccessRightsEnforced) {
    AccessPolicy policy;
    UA_FileTransferBackend backend = accessBackend(memBackendWithTree(), &policy);
    UA_NodeId fsId = mountNamedMem(backend, "FileSystem", NULL);
    UA_NodeId docs = resolveChild(server_ft, fsId, "docs");
    UA_NodeId a = resolveChild(server_ft, docs, "a.txt");
    UA_NodeId sub = resolveChild(server_ft, docs, "sub");
    UA_NodeId b = resolveChild(server_ft, sub, "b.txt");
    UA_NodeId readme = resolveChild(server_ft, fsId, "readme.txt");

    /* Traverse alone on an ancestor permits accessing a known child. */
    policy.target = fsId;
    policy.user = UA_FILEACCESS_TRAVERSE;
    UA_UInt32 h = callOpen(a, UA_OPENFILEMODE_READ | UA_OPENFILEMODE_WRITE,
                           UA_STATUSCODE_GOOD);
    /* Read/write on the ancestor cannot replace traverse, even with an
     * already open handle or a known child NodeId. */
    policy.user = UA_FILEACCESS_READ | UA_FILEACCESS_WRITE;
    callOpen(readme, UA_OPENFILEMODE_READ, UA_STATUSCODE_BADUSERACCESSDENIED);
    callWrite(a, h, "forbidden", UA_STATUSCODE_BADUSERACCESSDENIED);
    UA_ByteString data = callRead(a, h, 4, UA_STATUSCODE_BADUSERACCESSDENIED);
    UA_ByteString_clear(&data);
    callClose(a, h, UA_STATUSCODE_GOOD);
    callCreateFile(fsId, "denied", false, NULL, UA_STATUSCODE_BADUSERACCESSDENIED);
    callCreateDirectory(fsId, "denied", UA_STATUSCODE_BADUSERACCESSDENIED);
    callDelete(fsId, readme, UA_STATUSCODE_BADUSERACCESSDENIED);
    callMoveOrCopy(fsId, readme, docs, false, "denied",
                   UA_STATUSCODE_BADUSERACCESSDENIED);

    /* General directory permissions cannot be overridden by the user. */
    policy.user |= UA_FILEACCESS_TRAVERSE;
    policy.general = UA_FILEACCESS_READ | UA_FILEACCESS_TRAVERSE;
    callCreateFile(fsId, "denied", false, NULL, UA_STATUSCODE_BADUSERACCESSDENIED);
    callCreateDirectory(fsId, "denied", UA_STATUSCODE_BADUSERACCESSDENIED);
    callDelete(fsId, readme, UA_STATUSCODE_BADUSERACCESSDENIED);
    policy.general |= UA_FILEACCESS_WRITE;

    /* Copy must authorize the source file, including descendants of a tree. */
    policy.target = b;
    policy.user = 0;
    callMoveOrCopy(fsId, docs, fsId, true, "denied",
                   UA_STATUSCODE_BADUSERACCESSDENIED);
    ck_assert(!tryResolveChild(server_ft, fsId, "denied", NULL));
    policy.target = a;
    callMoveOrCopy(docs, a, fsId, true, "denied",
                   UA_STATUSCODE_BADUSERACCESSDENIED);
    policy.target = docs;
    policy.user = UA_FILEACCESS_WRITE | UA_FILEACCESS_TRAVERSE;
    callMoveOrCopy(fsId, docs, fsId, true, "denied",
                   UA_STATUSCODE_BADUSERACCESSDENIED);

    /* Recursive deletion also checks each directory, before removing files. */
    policy.target = sub;
    policy.user = UA_FILEACCESS_READ | UA_FILEACCESS_TRAVERSE;
    callDelete(fsId, docs, UA_STATUSCODE_BADUSERACCESSDENIED);
    ck_assert(tryResolveChild(server_ft, docs, "a.txt", NULL));
    ck_assert(tryResolveChild(server_ft, sub, "b.txt", NULL));

    /* Target directory rights apply to copy/move as well. */
    policy.target = docs;
    callMoveOrCopy(fsId, readme, docs, true, "denied",
                   UA_STATUSCODE_BADUSERACCESSDENIED);
    callMoveOrCopy(fsId, readme, docs, false, "denied",
                   UA_STATUSCODE_BADUSERACCESSDENIED);

    /* CreateFile with requestFileOpen also checks the newly created Object.
     * A failed open rolls creation back. */
    policy.target = UA_NODEID_NULL;
    policy.user = UA_FILEACCESS_WRITE | UA_FILEACCESS_TRAVERSE;
    callCreateFile(fsId, "denied", true, NULL, UA_STATUSCODE_BADNOTREADABLE);
    ck_assert(!tryResolveChild(server_ft, fsId, "denied", NULL));

    /* Removing a file requires rights on its parent, not on the file. */
    policy.target = readme;
    policy.user = 0;
    callDelete(fsId, readme, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&readme);
    UA_NodeId_clear(&b);
    UA_NodeId_clear(&sub);
    UA_NodeId_clear(&a);
    UA_NodeId_clear(&docs);
    UA_NodeId_clear(&fsId);
} END_TEST

#ifndef _WIN32
START_TEST(localDirectoryAccessRightsFollowPermissions) {
    makeScratchDir();
    UA_FileTransferBackend backend;
    ck_assert_uint_eq(UA_FileTransferBackend_localDirectory(
                          UA_STRING(scratchDir), &backend), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&backend, UA_STRING("access.txt"), false),
                      UA_STATUSCODE_GOOD);
    char path[1024];
    snprintf(path, sizeof(path), "%s/access.txt", scratchDir);
    FILE *permissionsFile = fopen(path, "rb");
    ck_assert_ptr_nonnull(permissionsFile);
    ck_assert_int_eq(fchmod(fileno(permissionsFile), 0700), 0);
    UA_FileTransferFileInfo info;
    ck_assert_uint_eq(backend.file.getInfo(&backend.file, UA_STRING("access.txt"),
                                          &info), UA_STATUSCODE_GOOD);
    /* An executable file does not acquire a directory-only permission. */
    ck_assert_uint_eq(info.accessRights, UA_FILEACCESS_READ | UA_FILEACCESS_WRITE);
    ck_assert_uint_eq(backend.file.getInfo(&backend.file, UA_STRING_NULL, &info),
                      UA_STATUSCODE_GOOD);
    ck_assert(info.accessRights & UA_FILEACCESS_TRAVERSE);

    UA_NodeId fileId = UA_NODEID_NULL;
    ck_assert_uint_eq(testAddFile(UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                          UA_QUALIFIEDNAME(0, "Permissions"), &backend.file,
                          UA_STRING("access.txt"), NULL, &fileId),
                      UA_STATUSCODE_GOOD);
    UA_UInt32 h = callOpen(fileId, UA_OPENFILEMODE_WRITE, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(fchmod(fileno(permissionsFile), 0400), 0);
    ck_assert_uint_eq(backend.file.getInfo(&backend.file, UA_STRING("access.txt"),
                                          &info), UA_STATUSCODE_GOOD);
    ck_assert(!(info.accessRights & UA_FILEACCESS_TRAVERSE));
    /* CI may run as root, which retains write permission after chmod. */
    if(access(path, W_OK) != 0) {
        ck_assert_uint_eq(info.accessRights, UA_FILEACCESS_READ);
        ck_assert(!readBooleanProperty(fileId, "Writable"));
        ck_assert(!readBooleanProperty(fileId, "UserWritable"));
        callWrite(fileId, h, "blocked", UA_STATUSCODE_BADNOTWRITABLE);
    }
    callClose(fileId, h, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(testRemove(driverForRoot(fileId), fileId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileId);
    ck_assert_int_eq(fclose(permissionsFile), 0);
    removeTree(scratchDir);
} END_TEST
#endif

static UA_Boolean hideNextListing;

static UA_StatusCode
changingAccessListing(UA_FileTransferBackend *b, const UA_String path,
                       UA_FileTransferListCallback cb, void *listContext) {
    if(hideNextListing) {
        hideNextListing = false;
        return UA_STATUSCODE_GOOD;
    }
    return memListDirectory(b, path, cb, listContext);
}

START_TEST(changedListingDoesNotBypassAccessCallbacks) {
    AccessPolicy policy;
    UA_FileTransferBackend backend = accessBackend(memBackendWithTree(), &policy);
    backend.listDirectory = changingAccessListing;
    UA_NodeId fsId = UA_NODEID_NULL;
    ck_assert_uint_eq(testAddDirectory(UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                          UA_QUALIFIEDNAME(0, "FileSystem"), &backend, NULL,
                          &fsId), UA_STATUSCODE_GOOD);
    UA_NodeId docs = resolveChild(server_ft, fsId, "docs");
    UA_NodeId a = resolveChild(server_ft, docs, "a.txt");
    policy.target = a;
    policy.user = 0;
    /* The preflight sees an empty directory. The actual copy must still
     * authorize entries that appear afterwards, and remove its partial copy. */
    hideNextListing = true;
    callMoveOrCopy(fsId, docs, fsId, true, "denied",
                   UA_STATUSCODE_BADUSERACCESSDENIED);
    UA_FileTransferFileInfo info;
    ck_assert_uint_eq(backend.file.getInfo(&backend.file, UA_STRING("denied"),
                                          &info), UA_STATUSCODE_BADNOTFOUND);
    ck_assert(!tryResolveChild(server_ft, fsId, "denied", NULL));
    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&a);
    UA_NodeId_clear(&docs);
    UA_NodeId_clear(&fsId);
} END_TEST

START_TEST(unmirroredEntriesDoNotBypassAccessCallbacks) {
    AccessPolicy policy;
    UA_FileTransferBackend backend = accessBackend(memBackendWithTree(), &policy);
    FTConfig options;
    memset(&options, 0, sizeof(options));
    options.maxScanDepth = 1;
    UA_NodeId fsId = mountNamedMem(backend, "FileSystem", &options);
    UA_NodeId docs = resolveChild(server_ft, fsId, "docs");
    callMoveOrCopy(fsId, docs, fsId, true, "denied",
                   UA_STATUSCODE_BADUSERACCESSDENIED);
    callDelete(fsId, docs, UA_STATUSCODE_BADUSERACCESSDENIED);
    ck_assert(tryResolveChild(server_ft, fsId, "docs", NULL));
    ck_assert(!tryResolveChild(server_ft, fsId, "denied", NULL));
    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&docs);
    UA_NodeId_clear(&fsId);
} END_TEST

/* maxNodes counts the FileSystem root */
START_TEST(maxNodesCountsRoot) {
    FTConfig options;
    memset(&options, 0, sizeof(options));
    options.maxNodes = 2;

    /* The root and one of the two files */
    UA_FileTransferBackend b;
    ck_assert_uint_eq(memBackend(&b), UA_STATUSCODE_GOOD);
    writeMemFile(&b, "a", "1");
    writeMemFile(&b, "b", "2");
    UA_NodeId fsId = mountNamedMem(b, "FileSystem", &options);
    ck_assert(tryResolveChild(server_ft, fsId, "a", NULL) !=
              tryResolveChild(server_ft, fsId, "b", NULL));

    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fsId);
} END_TEST

static UA_Boolean malformedSnapshotName;

static UA_StatusCode
snapshotGetInfo(UA_FileTransferFileBackend *b, const UA_String path,
                 UA_FileTransferFileInfo *info) {
    if(path.length == 0)
        return memGetInfo(b, path, info);
    memset(info, 0, sizeof(*info));
    memcpy(info->name, path.data, path.length);
    info->accessRights = UA_FILEACCESS_READ;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
snapshotListDirectory(UA_FileTransferBackend *b, const UA_String path,
                       UA_FileTransferListCallback cb, void *listContext) {
    UA_FileTransferFileInfo info;
    memset(&info, 0, sizeof(info));
    if(malformedSnapshotName) {
        memset(info.name, 'x', sizeof(info.name)); /* Missing terminator */
        cb(listContext, &info);
        return UA_STATUSCODE_GOOD;
    }
    strcpy(info.name, "listed.txt");
    info.size = 42;
    info.lastModified = UA_DATETIME_UNIX_EPOCH;
    info.accessRights = UA_FILEACCESS_READ;
    strcpy(info.mimeType, "application/x-listing");
    cb(listContext, &info);

    /* Reuse the buffer, as a backend may do for every listed entry. */
    memset(info.name, 'm', sizeof(info.name) - 1);
    info.name[sizeof(info.name) - 1] = 0;
    cb(listContext, &info);
    memset(&info, 0, sizeof(info));
    /* 150 non-ASCII characters need 300 UTF-8 bytes. */
    for(size_t i = 0; i < 150; i++) {
        info.name[2 * i] = (char)0xc3;
        info.name[2 * i + 1] = (char)0xa9;
    }
    info.accessRights = UA_FILEACCESS_READ;
    cb(listContext, &info);
    memset(&info, 0xff, sizeof(info));
    return UA_STATUSCODE_GOOD;
}

START_TEST(listingCarriesFullInfoAndInlineNames) {
    UA_FileTransferBackend backend;
    ck_assert_uint_eq(memBackend(&backend), UA_STATUSCODE_GOOD);
    backend.file.getInfo = snapshotGetInfo;
    backend.listDirectory = snapshotListDirectory;
    malformedSnapshotName = false;
    UA_NodeId fsId = mountNamedMem(backend, "FileSystem", NULL);
    /* Only the listing reports MimeType. Node creation must use that snapshot,
     * while live properties continue to use getInfo. */
    UA_NodeId listed = resolveChild(server_ft, fsId, "listed.txt");
    UA_Variant mime;
    readProperty(listed, "MimeType", &mime);
    UA_String expected = UA_STRING("application/x-listing");
    ck_assert(UA_String_equal((UA_String*)mime.data, &expected));
    UA_Variant_clear(&mime);

    char maximum[UA_FILETRANSFER_FILENAME_MAX + 2];
    memset(maximum, 'm', sizeof(maximum));
    maximum[UA_FILETRANSFER_FILENAME_MAX] = 0;
    ck_assert(tryResolveChild(server_ft, fsId, maximum, NULL));
    maximum[UA_FILETRANSFER_FILENAME_MAX] = 'm';
    maximum[UA_FILETRANSFER_FILENAME_MAX + 1] = 0;
    callCreateFile(fsId, maximum, false, NULL, UA_STATUSCODE_BADINVALIDARGUMENT);
    char utf8[301];
    for(size_t i = 0; i < 150; i++) {
        utf8[2 * i] = (char)0xc3;
        utf8[2 * i + 1] = (char)0xa9;
    }
    utf8[300] = 0;
    ck_assert(tryResolveChild(server_ft, fsId, utf8, NULL));

    /* A malformed name aborts the listing, preserving the existing tree. */
    malformedSnapshotName = true;
    ck_assert_uint_eq(testRefresh(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
    ck_assert(tryResolveChild(server_ft, fsId, "listed.txt", NULL));
    ck_assert(tryResolveChild(server_ft, fsId, utf8, NULL));
    malformedSnapshotName = false;
    ck_assert_uint_eq(testRefresh(driverForRoot(fsId), fsId), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&listed);
    UA_NodeId_clear(&fsId);
} END_TEST

/* A listing that reports its entries and then fails */
static UA_Boolean failListing;

static UA_StatusCode
partialListDirectory(UA_FileTransferBackend *b, const UA_String path,
                     UA_FileTransferListCallback cb, void *listContext) {
    UA_StatusCode res = memListDirectory(b, path, cb, listContext);
    return failListing ? UA_STATUSCODE_BADUNEXPECTEDERROR : res;
}

/* An incomplete listing is dropped as a whole. The refresh keeps the mirrored
 * tree and the Delete Method keeps the directory. */
START_TEST(failedListingKeepsTree) {
    UA_FileTransferBackend b = memBackendWithTree();
    b.listDirectory = partialListDirectory;
    UA_NodeId fsId = mountNamedMem(b, "FileSystem", NULL);
    ck_assert_uint_eq(b.remove(&b, UA_STRING("readme.txt")), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&b, UA_STRING("new.txt"), false), UA_STATUSCODE_GOOD);

    failListing = true;
    ck_assert_uint_eq(testRefresh(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_BADUNEXPECTEDERROR);
    ck_assert(tryResolveChild(server_ft, fsId, "readme.txt", NULL));
    ck_assert(!tryResolveChild(server_ft, fsId, "new.txt", NULL));
    UA_NodeId docsId = resolveChild(server_ft, fsId, "docs");
    callDelete(fsId, docsId, UA_STATUSCODE_BADUNEXPECTEDERROR);
    ck_assert(tryResolveChild(server_ft, fsId, "docs", NULL));
    failListing = false;

    ck_assert_uint_eq(testRefresh(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    ck_assert(!tryResolveChild(server_ft, fsId, "readme.txt", NULL));
    ck_assert(tryResolveChild(server_ft, fsId, "new.txt", NULL));

    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&docsId);
    UA_NodeId_clear(&fsId);
} END_TEST

/* A refresh removes the vanished entries of all directories before it mirrors
 * new ones, so the room they free is available in every directory. A file
 * moves from b to a and then from a to b, so one of the two refreshes needs
 * room that is freed in a directory the walk reaches later. */
START_TEST(maxNodesReusedAcrossDirectories) {
    UA_FileTransferBackend b;
    ck_assert_uint_eq(memBackend(&b), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&b, UA_STRING("a"), true), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&b, UA_STRING("b"), true), UA_STATUSCODE_GOOD);
    writeMemFile(&b, "a/one", "1");
    writeMemFile(&b, "b/two", "2");
    FTConfig options;
    memset(&options, 0, sizeof(options));
    options.maxNodes = 5; /* The root, two directories and two files */
    UA_NodeId fsId = mountNamedMem(b, "FileSystem", &options);
    UA_NodeId aId = resolveChild(server_ft, fsId, "a");
    UA_NodeId bId = resolveChild(server_ft, fsId, "b");

    ck_assert_uint_eq(b.remove(&b, UA_STRING("b/two")), UA_STATUSCODE_GOOD);
    writeMemFile(&b, "a/two", "2");
    ck_assert_uint_eq(testRefresh(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    ck_assert(tryResolveChild(server_ft, aId, "two", NULL));
    ck_assert(!tryResolveChild(server_ft, bId, "two", NULL));

    ck_assert_uint_eq(b.remove(&b, UA_STRING("a/two")), UA_STATUSCODE_GOOD);
    writeMemFile(&b, "b/two", "2");
    ck_assert_uint_eq(testRefresh(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    ck_assert(tryResolveChild(server_ft, bId, "two", NULL));
    ck_assert(!tryResolveChild(server_ft, aId, "two", NULL));

    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&aId);
    UA_NodeId_clear(&bId);
    UA_NodeId_clear(&fsId);
} END_TEST

/* A directory that vanished while a file in it is open stays as a zombie. A
 * file that replaces it under the same name is a different entry and stays
 * usable. */
START_TEST(replacingEntryIsNotZombie) {
    UA_FileTransferBackend b;
    ck_assert_uint_eq(memBackend(&b), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&b, UA_STRING("d"), true), UA_STATUSCODE_GOOD);
    writeMemFile(&b, "d/f", "open");
    UA_NodeId fsId = mountNamedMem(b, "FileSystem", NULL);
    UA_NodeId dirId = resolveChild(server_ft, fsId, "d");
    UA_NodeId fId = resolveChild(server_ft, dirId, "f");
    UA_UInt32 h = callOpen(fId, UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(b.remove(&b, UA_STRING("d/f")), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b.remove(&b, UA_STRING("d")), UA_STATUSCODE_GOOD);
    writeMemFile(&b, "d", "file");
    ck_assert_uint_eq(testRefresh(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(testRefresh(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);

    /* The new file d is served while the old directory d is a zombie */
    UA_QualifiedName dName = UA_QUALIFIEDNAME(0, "d");
    UA_BrowsePathResult bpr =
        UA_Server_browseSimplifiedBrowsePath(server_ft, fsId, 1, &dName);
    ck_assert_uint_eq(bpr.targetsSize, 2);
    UA_NodeId newFileId = bpr.targets[0].targetId.nodeId;
    if(UA_NodeId_equal(&newFileId, &dirId))
        newFileId = bpr.targets[1].targetId.nodeId;
    UA_ByteString data = readFileContent(newFileId);
    ck_assert_uint_eq(data.length, 4);
    UA_ByteString_clear(&data);
    UA_BrowsePathResult_clear(&bpr);

    /* Closing the last handle removes the zombies */
    callClose(fId, h, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(testRefresh(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeClass nodeClass;
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ft, dirId, &nodeClass),
                      UA_STATUSCODE_BADNODEIDUNKNOWN);

    ck_assert_uint_eq(testRemove(driverForRoot(fsId), fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&dirId);
    UA_NodeId_clear(&fId);
    UA_NodeId_clear(&fsId);
} END_TEST

/* Constructor failure retains the backend; successful drivers own it until free. */
static size_t backendClearCount;

static void
countedBackendClear(UA_FileTransferFileBackend *backend) {
    backendClearCount++;
    memClear(backend);
}

START_TEST(backendLivesUntilDriverFree) {
    backendClearCount = 0;
    UA_FileTransferBackend backend = memBackendWithFile("f.bin", "retained");
    backend.file.clear = countedBackendClear;
    UA_Driver *driver = NULL;
    UA_NodeId root = UA_NODEID_NUMERIC(1, 54321);
    ck_assert_uint_eq(UA_FileTransferDriver_newFile(server_ft, &backend.file, UA_STRING("missing"),
        &(UA_FileTransferNodeDescription){
            .nodeId = root,
            .parentNodeId = UA_NS0ID(OBJECTSFOLDER),
            .referenceTypeId = UA_NS0ID(ORGANIZES),
            .browseName = UA_QUALIFIEDNAME(1, "File"),
            .typeDefinition = UA_NS0ID(FILETYPE),
            .attributes = UA_ObjectAttributes_default}, NULL, &driver), UA_STATUSCODE_BADNOTFOUND);
    ck_assert_ptr_null(driver);
    ck_assert_uint_eq(backendClearCount, 0);
    UA_NodeClass cls;
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ft, root, &cls), UA_STATUSCODE_BADNODEIDUNKNOWN);

    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en", "My file");
    ck_assert_uint_eq(UA_FileTransferDriver_newFile(server_ft, &backend.file, UA_STRING("f.bin"),
        &(UA_FileTransferNodeDescription){
            .nodeId = root,
            .parentNodeId = UA_NS0ID(OBJECTSFOLDER),
            .referenceTypeId = UA_NS0ID(ORGANIZES),
            .browseName = UA_QUALIFIEDNAME(1, "File"),
            .typeDefinition = UA_NS0ID(FILETYPE),
            .attributes = attr}, NULL, &driver), UA_STATUSCODE_GOOD);
    /* The Object already exists before the driver is registered. */
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ft, root, &cls), UA_STATUSCODE_GOOD);
    void *actualContext = NULL;
    ck_assert_uint_eq(UA_Server_getNodeContext(server_ft, root, &actualContext), UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(actualContext, driver);
    UA_LocalizedText actual;
    ck_assert_uint_eq(UA_Server_readDisplayName(server_ft, root, &actual), UA_STATUSCODE_GOOD);
    ck_assert(UA_LocalizedText_equal(&actual, &attr.displayName));
    UA_LocalizedText_clear(&actual);
    registerTestDriver(driver);
    UA_UInt32 handle = callOpen(root, UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    driver->stop(driver);
    ck_assert_uint_eq(readOpenCount(root), 0);
    ck_assert_uint_eq(backendClearCount, 0);
    ck_assert_uint_eq(UA_Server_getNodeContext(server_ft, root, &actualContext), UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(actualContext, driver);
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ft, root, &cls), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(driver->start(driver), UA_STATUSCODE_GOOD);
    callClose(root, handle, UA_STATUSCODE_BADINVALIDARGUMENT);
    UA_ByteString data = readFileContent(root);
    ck_assert_uint_eq(data.length, 8);
    UA_ByteString_clear(&data);
    ck_assert_uint_eq(testRemove(driver, root), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(backendClearCount, 1);
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ft, root, &cls), UA_STATUSCODE_BADNODEIDUNKNOWN);

    backend = memBackendWithFile("f.bin", "unregistered");
    backend.file.clear = countedBackendClear;
    driver = newTestFile(server_ft, &backend.file, "Unregistered", &root);
    ck_assert_uint_eq(driver->free(driver), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(backendClearCount, 2);
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ft, root, &cls), UA_STATUSCODE_BADNODEIDUNKNOWN);
    UA_NodeId_clear(&root);
} END_TEST

START_TEST(constructorServesSharedSubtypeMethods) {
    UA_NodeId typeId = UA_NODEID_NUMERIC(1, 50000);
    UA_ObjectTypeAttributes typeAttr = UA_ObjectTypeAttributes_default;
    typeAttr.displayName = UA_LOCALIZEDTEXT("", "SubtypeFile");
    ck_assert_uint_eq(UA_Server_addObjectTypeNode(server_ft, typeId,
                                                  UA_NS0ID(FILETYPE),
                                                  UA_NS0ID(HASSUBTYPE),
                                                  UA_QUALIFIEDNAME(1, "SubtypeFile"),
                                                  typeAttr, NULL, NULL),
                      UA_STATUSCODE_GOOD);
    UA_NodeId openId = UA_NODEID_NUMERIC(1, 50001);
    UA_MethodAttributes methodAttr = UA_MethodAttributes_default;
    methodAttr.displayName = UA_LOCALIZEDTEXT("", "Open");
    methodAttr.executable = true;
    methodAttr.userExecutable = true;
    UA_Argument inArg, outArg;
    UA_Argument_init(&inArg);
    UA_Argument_init(&outArg);
    inArg.name = UA_STRING("Mode");
    inArg.dataType = UA_TYPES[UA_TYPES_BYTE].typeId;
    inArg.valueRank = UA_VALUERANK_SCALAR;
    outArg.name = UA_STRING("FileHandle");
    outArg.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    outArg.valueRank = UA_VALUERANK_SCALAR;
    ck_assert_uint_eq(UA_Server_addMethodNode(server_ft, openId, typeId,
                                              UA_NS0ID(HASCOMPONENT),
                                              UA_QUALIFIEDNAME(0, "Open"),
                                              methodAttr, NULL, 1, &inArg, 1,
                                              &outArg, NULL, NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_addReference(server_ft, openId,
                                             UA_NS0ID(HASMODELLINGRULE),
                                             UA_NS0EXID(MODELLINGRULE_MANDATORY),
                                             true), UA_STATUSCODE_GOOD);

    UA_NodeId files[2];
    UA_Driver *drivers[2];
    const char *names[2] = {"SubtypeA", "SubtypeB"};
    for(size_t i = 0; i < 2; i++) {
        UA_FileTransferBackend backend = memBackendWithFile("f.bin", "data");
        ck_assert_uint_eq(UA_FileTransferDriver_newFile(server_ft, &backend.file, UA_STRING("f.bin"),
        &(UA_FileTransferNodeDescription){
            .nodeId = UA_NODEID_NULL,
            .parentNodeId = UA_NS0ID(OBJECTSFOLDER),
            .referenceTypeId = UA_NS0ID(ORGANIZES),
            .browseName = UA_QUALIFIEDNAME(1, (char*)(uintptr_t)names[i]),
            .typeDefinition = typeId,
            .attributes = UA_ObjectAttributes_default}, &files[i], &drivers[i]), UA_STATUSCODE_GOOD);
        registerTestDriver(drivers[i]);
        UA_NodeId instanceOpenId = resolveChild(server_ft, files[i], "Open");
        ck_assert(UA_NodeId_equal(&instanceOpenId, &openId));
        UA_NodeId_clear(&instanceOpenId);
    }
    UA_Byte mode = UA_OPENFILEMODE_READ;
    UA_Variant input;
    UA_Variant_setScalar(&input, &mode, &UA_TYPES[UA_TYPES_BYTE]);
    drivers[0]->stop(drivers[0]);
    UA_CallMethodResult result = callObjectMethod(files[0], "Open", 1, &input);
    ck_assert_uint_ne(result.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&result);
    /* Stopping/freeing one subtype instance preserves the other's callbacks. */
    ck_assert_uint_eq(testRemove(drivers[0], files[0]), UA_STATUSCODE_GOOD);
    result = callObjectMethod(files[1], "Open", 1, &input);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&result);
    drivers[1]->stop(drivers[1]);
    UA_MethodCallback cb = NULL;
    ck_assert_uint_eq(UA_Server_getMethodNodeCallback(server_ft, openId, &cb), UA_STATUSCODE_GOOD);
    ck_assert(cb == NULL);
    ck_assert_uint_eq(drivers[1]->start(drivers[1]), UA_STATUSCODE_GOOD);
    result = callObjectMethod(files[1], "Open", 1, &input);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&result);
    ck_assert_uint_eq(testRemove(drivers[1], files[1]), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&files[0]);
    UA_NodeId_clear(&files[1]);
} END_TEST

START_TEST(constructorFailureLeavesBackendAndOutputs) {
    backendClearCount = 0;
    UA_FileTransferBackend backend = memBackendWithFile("f.bin", "data");
    backend.file.clear = countedBackendClear;
    UA_Driver *driver = NULL;
    UA_NodeId root = UA_NODEID_NUMERIC(1, 54322);
    UA_NodeId out = UA_NODEID_NUMERIC(1, 54323);
    UA_StatusCode res = UA_FileTransferDriver_newFile(server_ft, &backend.file, UA_STRING("f.bin"),
        &(UA_FileTransferNodeDescription){
            .nodeId = root,
            .parentNodeId = UA_NS0ID(OBJECTSFOLDER),
            .referenceTypeId = UA_NS0ID(HASCOMPONENT),
            .browseName = UA_QUALIFIEDNAME(1, "WrongType"),
            .typeDefinition = UA_NS0ID(FOLDERTYPE),
            .attributes = UA_ObjectAttributes_default}, &out, &driver);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADTYPEDEFINITIONINVALID);
    ck_assert_ptr_null(driver);
    ck_assert_uint_eq(out.identifier.numeric, 54323);
    ck_assert_uint_eq(backendClearCount, 0);
    UA_NodeClass cls;
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ft, root, &cls), UA_STATUSCODE_BADNODEIDUNKNOWN);

    /* An invalid parent fails Object creation after backend validation. */
    res = UA_FileTransferDriver_newFile(server_ft, &backend.file, UA_STRING("f.bin"),
        &(UA_FileTransferNodeDescription){
            .nodeId = root,
            .parentNodeId = UA_NODEID_NUMERIC(1, 54324),
            .referenceTypeId = UA_NS0ID(HASCOMPONENT),
            .browseName = UA_QUALIFIEDNAME(1, "BadParent"),
            .typeDefinition = UA_NS0ID(FILETYPE),
            .attributes = UA_ObjectAttributes_default}, &out, &driver);
    ck_assert_uint_ne(res, UA_STATUSCODE_GOOD);
    ck_assert_ptr_null(driver);
    ck_assert_uint_eq(out.identifier.numeric, 54323);
    ck_assert_uint_eq(backendClearCount, 0);
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ft, root, &cls), UA_STATUSCODE_BADNODEIDUNKNOWN);
    driver = newTestFile(server_ft, &backend.file, "Retry", NULL);
    ck_assert_uint_eq(driver->free(driver), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(backendClearCount, 1);
} END_TEST

START_TEST(failedRegistrationRetainsObjects) {
    UA_Server_getConfig(server_ft)->tcpEnabled = false;
    ck_assert_uint_eq(UA_Server_run_startup(server_ft), UA_STATUSCODE_GOOD);
    backendClearCount = 0;
    UA_FileTransferBackend backend = memBackendWithFile("f.bin", "read-only");
    backend.file.write = NULL;
    backend.file.clear = countedBackendClear;
    UA_NodeId root;
    UA_Driver *driver = newTestFile(server_ft, &backend.file, "Retry", &root);
    UA_Driver *previous = UA_Server_getDrivers(server_ft);
    /* Default writable configuration cannot start this backend. */
    ck_assert_uint_eq(UA_Server_addDriver(server_ft, driver), UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_ptr_eq(UA_Server_getDrivers(server_ft), previous);
    ck_assert_uint_eq(driver->state, UA_LIFECYCLESTATE_STOPPED);
    ck_assert_uint_eq(backendClearCount, 0);
    UA_NodeClass cls;
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ft, root, &cls), UA_STATUSCODE_GOOD);
    /* Configure the constructed driver and retry without recreating its Object. */
    UA_Boolean readOnly = true;
    ck_assert_uint_eq(UA_KeyValueMap_setScalar(&driver->params,
        UA_QUALIFIEDNAME(0, "read-only"), &readOnly, &UA_TYPES[UA_TYPES_BOOLEAN]), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_addDriver(server_ft, driver), UA_STATUSCODE_GOOD);
    UA_ByteString data = readFileContent(root);
    ck_assert_uint_eq(data.length, 9);
    UA_ByteString_clear(&data);
    callOpen(root, UA_OPENFILEMODE_WRITE, UA_STATUSCODE_BADNOTWRITABLE);
    ck_assert_uint_eq(testRemove(driver, root), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(backendClearCount, 1);
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ft, root, &cls), UA_STATUSCODE_BADNODEIDUNKNOWN);
    ck_assert_uint_eq(UA_Server_run_shutdown(server_ft), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&root);
} END_TEST

START_TEST(directoryLifetimeAndAutomaticRefresh) {
    UA_Server_getConfig(server_ft)->tcpEnabled = false;
    ck_assert_uint_eq(UA_Server_run_startup(server_ft), UA_STATUSCODE_GOOD);
    UA_FileTransferBackend backend = memBackendWithTree();
    UA_NodeId root;
    UA_Driver *driver = newTestDirectory(server_ft, &backend, &root);
    UA_Double interval = 10;
    ck_assert_uint_eq(UA_KeyValueMap_setScalar(&driver->params,
        UA_QUALIFIEDNAME(0, "refresh-interval"), &interval, &UA_TYPES[UA_TYPES_DOUBLE]), UA_STATUSCODE_GOOD);
    UA_NodeClass cls;
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ft, root, &cls), UA_STATUSCODE_GOOD);
    ck_assert(!tryResolveChild(server_ft, root, "readme.txt", NULL));
    /* Adding to a running server performs the first scan. */
    ck_assert_uint_eq(UA_Server_addDriver(server_ft, driver), UA_STATUSCODE_GOOD);
    UA_NodeId file = resolveChild(server_ft, root, "readme.txt");
    UA_UInt32 handle = callOpen(file, UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    driver->stop(driver);
    ck_assert_uint_eq(readOpenCount(file), 0);
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ft, file, &cls), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&backend, UA_STRING("new.txt"), false), UA_STATUSCODE_GOOD);
    UA_fakeSleep(20);
    UA_Server_run_iterate(server_ft, false);
    ck_assert(!tryResolveChild(server_ft, root, "new.txt", NULL));
    ck_assert_uint_eq(driver->start(driver), UA_STATUSCODE_GOOD);
    UA_NodeId sameFile = resolveChild(server_ft, root, "readme.txt");
    ck_assert(UA_NodeId_equal(&file, &sameFile));
    UA_NodeId_clear(&sameFile);
    callClose(file, handle, UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert(tryResolveChild(server_ft, root, "new.txt", NULL));
    ck_assert_uint_eq(createEntry(&backend, UA_STRING("automatic.txt"), false), UA_STATUSCODE_GOOD);
    UA_fakeSleep(20);
    UA_Server_run_iterate(server_ft, false);
    ck_assert(tryResolveChild(server_ft, root, "automatic.txt", NULL));
    ck_assert_uint_eq(testRemove(driver, root), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ft, root, &cls), UA_STATUSCODE_BADNODEIDUNKNOWN);
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ft, file, &cls), UA_STATUSCODE_BADNODEIDUNKNOWN);
    /* The refresh timer was removed before freeing the driver. */
    UA_fakeSleep(20);
    UA_Server_run_iterate(server_ft, false);
    ck_assert_uint_eq(UA_Server_run_shutdown(server_ft), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&root);
    UA_NodeId_clear(&file);
} END_TEST

/* A negative refresh interval is rejected. 0 disables the periodic refresh;
 * the application reconciles on demand. */
START_TEST(disabledPeriodicRefresh) {
    UA_Server_getConfig(server_ft)->tcpEnabled = false;
    ck_assert_uint_eq(UA_Server_run_startup(server_ft), UA_STATUSCODE_GOOD);
    UA_FileTransferBackend backend = memBackendWithTree();
    UA_NodeId root;
    UA_Driver *driver = newTestDirectory(server_ft, &backend, &root);
    /* Negative, NaN, infinite and sub-tick intervals are rejected */
    UA_Double zero = 0.0;
    UA_Double invalid[4] = {-1, zero / zero, 1.0 / zero, 0.00001};
    UA_Double interval;
    for(size_t i = 0; i < 4; i++) {
        ck_assert_uint_eq(UA_KeyValueMap_setScalar(&driver->params,
            UA_QUALIFIEDNAME(0, "refresh-interval"), &invalid[i], &UA_TYPES[UA_TYPES_DOUBLE]),
            UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(UA_Server_addDriver(server_ft, driver), UA_STATUSCODE_BADINVALIDARGUMENT);
    }

    interval = 0;
    ck_assert_uint_eq(UA_KeyValueMap_setScalar(&driver->params,
        UA_QUALIFIEDNAME(0, "refresh-interval"), &interval, &UA_TYPES[UA_TYPES_DOUBLE]), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_addDriver(server_ft, driver), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&backend, UA_STRING("new.txt"), false), UA_STATUSCODE_GOOD);
    UA_fakeSleep(5000);
    UA_Server_run_iterate(server_ft, false);
    ck_assert(!tryResolveChild(server_ft, root, "new.txt", NULL));
    ck_assert_uint_eq(UA_FileTransferDriver_refresh(driver), UA_STATUSCODE_GOOD);
    ck_assert(tryResolveChild(server_ft, root, "new.txt", NULL));

    driver->stop(driver);
    ck_assert_uint_eq(UA_FileTransferDriver_refresh(driver), UA_STATUSCODE_BADINVALIDSTATE);
    ck_assert_uint_eq(UA_FileTransferDriver_refresh(NULL), UA_STATUSCODE_BADINVALIDARGUMENT);

    /* A file driver has no tree to refresh */
    UA_FileTransferBackend fileBackend = memBackendWithFile("f.bin", "data");
    UA_Driver *fileDriver = newTestFile(server_ft, &fileBackend.file, "RefreshFile", NULL);
    ck_assert_uint_eq(UA_Server_addDriver(server_ft, fileDriver), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_FileTransferDriver_refresh(fileDriver), UA_STATUSCODE_BADNOTSUPPORTED);
    ck_assert_uint_eq(UA_Server_run_shutdown(server_ft), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&root);
} END_TEST

/* The header allows a directory backend to cast its file backend back to the
 * full struct, also when the driver queries the root at construction */
static UA_StatusCode
castingGetInfo(UA_FileTransferFileBackend *b, const UA_String path,
               UA_FileTransferFileInfo *outInfo) {
    UA_FileTransferBackend *full = (UA_FileTransferBackend*)b;
    if(full->listDirectory != memListDirectory)
        return UA_STATUSCODE_BADINTERNALERROR;
    return memGetInfo(b, path, outInfo);
}

START_TEST(directoryBackendCastsFileBackend) {
    UA_FileTransferBackend backend = memBackendWithTree();
    backend.file.getInfo = castingGetInfo;
    UA_NodeId root;
    UA_Driver *driver = newTestDirectory(server_ft, &backend, &root);
    registerTestDriver(driver);
    ck_assert(tryResolveChild(server_ft, root, "readme.txt", NULL));
    UA_NodeId_clear(&root);
} END_TEST

START_TEST(defaultNodeDescriptions) {
    UA_FileTransferBackend backend = memBackendWithTree();
    UA_Driver *driver = NULL;
    UA_NodeId file;
    ck_assert_uint_eq(UA_FileTransferDriver_newFile(server_ft, &backend.file,
        UA_STRING("docs/a.txt"), NULL, &file, &driver), UA_STATUSCODE_GOOD);
    UA_QualifiedName name;
    ck_assert_uint_eq(UA_Server_readBrowseName(server_ft, file, &name), UA_STATUSCODE_GOOD);
    UA_QualifiedName expected = UA_QUALIFIEDNAME(1, "a.txt");
    ck_assert(UA_QualifiedName_equal(&name, &expected));
    UA_QualifiedName_clear(&name);
    ck_assert_uint_eq(file.namespaceIndex, 1);
    UA_LocalizedText display;
    ck_assert_uint_eq(UA_Server_readDisplayName(server_ft, file, &display), UA_STATUSCODE_GOOD);
    ck_assert(UA_String_equal(&display.text, &expected.name));
    UA_LocalizedText_clear(&display);
    registerTestDriver(driver);
    UA_ByteString data = readFileContent(file);
    ck_assert_uint_eq(data.length, 9);
    UA_ByteString_clear(&data);
    ck_assert_uint_eq(testRemove(driver, file), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&file);

    backend = memBackendWithTree();
    UA_NodeId root;
    ck_assert_uint_eq(UA_FileTransferDriver_newDirectory(server_ft, &backend,
        NULL, &root, &driver), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_readBrowseName(server_ft, root, &name), UA_STATUSCODE_GOOD);
    expected = UA_QUALIFIEDNAME(0, "FileSystem");
    ck_assert(UA_QualifiedName_equal(&name, &expected));
    UA_QualifiedName_clear(&name);
    ck_assert_uint_eq(driver->free(driver), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&root);
} END_TEST

static UA_StatusCode
originalSizeSource(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                   const UA_NodeId *nodeId, void *nodeContext,
                   UA_Boolean includeSourceTimeStamp, const UA_NumericRange *range,
                   UA_DataValue *value) {
    ck_assert_ptr_nonnull(nodeContext);
    value->hasValue = true;
    return UA_Variant_setScalarCopy(&value->value, nodeContext, &UA_TYPES[UA_TYPES_UINT64]);
}

START_TEST(reuseFileRestoresProperties) {
    UA_NodeId file = addFileTypeInstance(server_ft, "Existing");
    UA_NodeId size = resolveChild(server_ft, file, "Size");
    int objectContext = 42;
    UA_UInt64 originalSize = 99;
    ck_assert_uint_eq(UA_Server_setNodeContext(server_ft, file, &objectContext), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_setNodeContext(server_ft, size, &originalSize), UA_STATUSCODE_GOOD);
    UA_CallbackValueSource source;
    memset(&source, 0, sizeof(source));
    source.read = originalSizeSource;
    ck_assert_uint_eq(UA_Server_setVariableNode_callbackValueSource(server_ft, size, source), UA_STATUSCODE_GOOD);
    UA_NodeId writable = resolveChild(server_ft, file, "Writable");
    UA_Boolean originalWritable = false;
    UA_DataValue writableValue;
    UA_DataValue_init(&writableValue);
    writableValue.hasValue = true;
    UA_Variant_setScalar(&writableValue.value, &originalWritable, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_DataValue *externalValue = &writableValue;
    ck_assert_uint_eq(UA_Server_setVariableNode_externalValueSource(server_ft, writable,
        &externalValue, NULL), UA_STATUSCODE_GOOD);
    UA_NodeId countId = resolveChild(server_ft, file, "OpenCount");
    UA_UInt16 originalCount = 7;
    UA_Variant countValue;
    UA_Variant_setScalar(&countValue, &originalCount, &UA_TYPES[UA_TYPES_UINT16]);
    ck_assert_uint_eq(UA_Server_writeValue(server_ft, countId, countValue), UA_STATUSCODE_GOOD);
    ck_assert(!tryResolveChild(server_ft, file, "MaxByteStringLength", NULL));
    UA_FileTransferBackend backend = memBackendWithFile("f.bin", "data");
    UA_FileTransferNodeDescription description;
    memset(&description, 0, sizeof(description));
    description.nodeId = file;
    /* Creation-only attributes cannot overwrite the reused Object. */
    description.browseName = UA_QUALIFIEDNAME(1, "Ignored");
    description.typeDefinition = UA_NS0ID(FOLDERTYPE);
    UA_Driver *driver = NULL;
    UA_NodeId out;
    ck_assert_uint_eq(UA_FileTransferDriver_newFile(server_ft, &backend.file,
        UA_STRING("f.bin"), &description, &out, &driver), UA_STATUSCODE_GOOD);
    ck_assert(UA_NodeId_equal(&out, &file));
    UA_NodeId_clear(&out);
    UA_Variant value;
    readProperty(file, "Size", &value);
    ck_assert_uint_eq(*(UA_UInt64*)value.data, 99); /* Unregistered: still original */
    UA_Variant_clear(&value);
    void *context = NULL;
    ck_assert_uint_eq(UA_Server_getNodeContext(server_ft, file, &context), UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(context, &objectContext);
    registerTestDriver(driver);
    ck_assert_uint_eq(UA_Server_getNodeContext(server_ft, file, &context), UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(context, driver);
    readProperty(file, "Size", &value);
    ck_assert_uint_eq(*(UA_UInt64*)value.data, 4);
    UA_Variant_clear(&value);
    UA_ByteString data = readFileContent(file);
    ck_assert_uint_eq(data.length, 4);
    UA_ByteString_clear(&data);
    ck_assert(tryResolveChild(server_ft, file, "MaxByteStringLength", NULL));
    ck_assert_uint_eq(testRemove(driver, file), UA_STATUSCODE_GOOD);
    UA_NodeClass cls;
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ft, file, &cls), UA_STATUSCODE_GOOD);
    ck_assert(!tryResolveChild(server_ft, file, "MaxByteStringLength", NULL));
    ck_assert(!tryResolveChild(server_ft, file, "LastModifiedTime", NULL));
    readProperty(file, "Size", &value);
    ck_assert_uint_eq(*(UA_UInt64*)value.data, 99);
    UA_Variant_clear(&value);
    ck_assert_uint_eq(UA_Server_getNodeContext(server_ft, file, &context), UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(context, &objectContext);
    ck_assert_uint_eq(UA_Server_getNodeContext(server_ft, size, &context), UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(context, &originalSize);
    ck_assert(!readBooleanProperty(file, "Writable"));
    originalWritable = true; /* The restored source is still the external pointer. */
    ck_assert(readBooleanProperty(file, "Writable"));
    ck_assert_uint_eq(readOpenCount(file), 7);

    /* A setup error after installing the value sources restores them, too. */
    UA_String originalLimit = UA_STRING("unchanged");
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    attr.valueRank = UA_VALUERANK_SCALAR;
    UA_Variant_setScalar(&attr.value, &originalLimit, &UA_TYPES[UA_TYPES_STRING]);
    UA_NodeId invalidLimit;
    ck_assert_uint_eq(UA_Server_addVariableNode(server_ft, UA_NODEID_NULL, file,
        UA_NS0ID(HASPROPERTY), UA_QUALIFIEDNAME(0, "MaxByteStringLength"),
        UA_NS0ID(PROPERTYTYPE), attr, NULL, &invalidLimit), UA_STATUSCODE_GOOD);
    backend = memBackendWithFile("f.bin", "data");
    ck_assert_uint_eq(UA_FileTransferDriver_newFile(server_ft, &backend.file,
        UA_STRING("f.bin"), &description, NULL, &driver), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_addDriver(server_ft, driver), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(driver->start(driver), UA_STATUSCODE_BADTYPEMISMATCH);
    readProperty(file, "Size", &value);
    ck_assert_uint_eq(*(UA_UInt64*)value.data, 99);
    UA_Variant_clear(&value);
    readProperty(file, "MaxByteStringLength", &value);
    ck_assert(UA_String_equal((UA_String*)value.data, &originalLimit));
    UA_Variant_clear(&value);
    ck_assert(!tryResolveChild(server_ft, file, "LastModifiedTime", NULL));
    ck_assert_uint_eq(testRemove(driver, file), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_deleteNode(server_ft, file, true), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&writable);
    UA_NodeId_clear(&countId);
    UA_NodeId_clear(&invalidLimit);
    UA_NodeId_clear(&file);
    UA_NodeId_clear(&size);
} END_TEST

static UA_NodeId
modelObject(UA_NodeId parent, UA_NodeId type, UA_UInt16 ns, char *name) {
    UA_NodeId out;
    ck_assert_uint_eq(UA_Server_addObjectNode(server_ft, UA_NODEID_NUMERIC(ns, 0),
        parent, UA_NS0ID(HASCOMPONENT), UA_QUALIFIEDNAME(ns, name), type,
        UA_ObjectAttributes_default, NULL, &out), UA_STATUSCODE_GOOD);
    return out;
}

START_TEST(reuseDerivedPropertyDataTypes) {
    UA_NodeId alias;
    ck_assert_uint_eq(UA_Server_addDataTypeNode(server_ft, UA_NODEID_NUMERIC(1, 0),
        UA_NS0ID(UINT32), UA_NS0ID(HASSUBTYPE), UA_QUALIFIEDNAME(1, "Length"),
        UA_DataTypeAttributes_default, NULL, &alias), UA_STATUSCODE_GOOD);
    UA_NodeId types[2] = {alias, UA_NS0ID(UINTEGER)};
    for(size_t i = 0; i < 2; i++) {
        UA_NodeId file = addFileTypeInstance(server_ft, "Existing");
        UA_UInt32 original = 123;
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.dataType = types[i];
        attr.valueRank = UA_VALUERANK_SCALAR;
        UA_Variant_setScalar(&attr.value, &original, &UA_TYPES[UA_TYPES_UINT32]);
        UA_NodeId limit;
        ck_assert_uint_eq(UA_Server_addVariableNode(server_ft, UA_NODEID_NULL, file,
            UA_NS0ID(HASPROPERTY), UA_QUALIFIEDNAME(0, "MaxByteStringLength"),
            UA_NS0ID(PROPERTYTYPE), attr, NULL, &limit), UA_STATUSCODE_GOOD);

        UA_FileTransferBackend backend = memBackendWithFile("f.bin", "data");
        UA_FileTransferNodeDescription description;
        memset(&description, 0, sizeof(description));
        description.nodeId = file;
        UA_Driver *driver;
        ck_assert_uint_eq(UA_FileTransferDriver_newFile(server_ft, &backend.file,
            UA_STRING("f.bin"), &description, NULL, &driver), UA_STATUSCODE_GOOD);
        registerTestDriver(driver);
        UA_Variant value;
        readProperty(file, "MaxByteStringLength", &value);
        ck_assert_uint_eq(*(UA_UInt32*)value.data, 1u << 20);
        UA_Variant_clear(&value);
        ck_assert_uint_eq(testRemove(driver, file), UA_STATUSCODE_GOOD);
        readProperty(file, "MaxByteStringLength", &value);
        ck_assert_uint_eq(*(UA_UInt32*)value.data, original);
        UA_Variant_clear(&value);
        ck_assert_uint_eq(UA_Server_deleteNode(server_ft, file, true), UA_STATUSCODE_GOOD);
        UA_NodeId_clear(&limit);
        UA_NodeId_clear(&file);
    }
    UA_NodeId_clear(&alias);
} END_TEST

START_TEST(reuseNestedDirectoryStructure) {
    UA_UInt16 ns = UA_Server_addNamespace(server_ft, "urn:test:existing-files");
    UA_NodeId root = modelObject(UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(FILEDIRECTORYTYPE), ns, "ExistingFS");
    UA_NodeId docs = modelObject(root, UA_NS0ID(FILEDIRECTORYTYPE), ns, "docs");
    UA_NodeId file = modelObject(docs, UA_NS0ID(FILETYPE), ns, "a.txt");
    UA_NodeId unrelated = modelObject(root, UA_NS0ID(FOLDERTYPE), ns, "Unrelated");
    UA_FileTransferBackend backend = memBackendWithTree();
    UA_FileTransferNodeDescription description;
    memset(&description, 0, sizeof(description));
    description.nodeId = root;
    UA_Driver *driver = NULL;
    ck_assert_uint_eq(UA_FileTransferDriver_newDirectory(server_ft, &backend,
        &description, NULL, &driver), UA_STATUSCODE_GOOD);
    registerTestDriver(driver);
    /* Namespace 0 is used for new entries; existing entries use another ns. */
    ck_assert(!tryResolveChild(server_ft, root, "docs", NULL));
    ck_assert(!tryResolveChild(server_ft, docs, "a.txt", NULL));
    UA_ByteString data = readFileContent(file);
    ck_assert_uint_eq(data.length, 9);
    UA_ByteString_clear(&data);
    UA_NodeId created = resolveChild(server_ft, docs, "sub");
    UA_NodeId createdFile = resolveChild(server_ft, created, "b.txt");
    UA_NodeId objects[5] = {root, docs, file, created, createdFile};
    void *context = NULL;
    for(size_t i = 0; i < sizeof(objects) / sizeof(objects[0]); i++) {
        ck_assert_uint_eq(UA_Server_getNodeContext(server_ft, objects[i], &context), UA_STATUSCODE_GOOD);
        ck_assert_ptr_eq(context, driver);
    }
    ck_assert_uint_eq(testRefresh(driver, root), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(testRemove(driver, root), UA_STATUSCODE_GOOD);
    UA_NodeClass cls;
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ft, root, &cls), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ft, docs, &cls), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ft, file, &cls), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ft, unrelated, &cls), UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 3; i++) {
        ck_assert_uint_eq(UA_Server_getNodeContext(server_ft, objects[i], &context), UA_STATUSCODE_GOOD);
        ck_assert_ptr_null(context);
    }
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ft, created, &cls), UA_STATUSCODE_BADNODEIDUNKNOWN);
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ft, createdFile, &cls), UA_STATUSCODE_BADNODEIDUNKNOWN);
    UA_NodeId_clear(&root);
    UA_NodeId_clear(&docs);
    UA_NodeId_clear(&file);
    UA_NodeId_clear(&unrelated);
    UA_NodeId_clear(&created);
    UA_NodeId_clear(&createdFile);

    /* Application-owned children also survive a driver-created parent. */
    backend = memBackendWithTree();
    ck_assert_uint_eq(UA_FileTransferDriver_newDirectory(server_ft, &backend,
        NULL, &root, &driver), UA_STATUSCODE_GOOD);
    docs = modelObject(root, UA_NS0ID(FILEDIRECTORYTYPE), ns, "docs");
    file = modelObject(docs, UA_NS0ID(FILETYPE), ns, "a.txt");
    registerTestDriver(driver);
    data = readFileContent(file);
    ck_assert_uint_eq(data.length, 9);
    UA_ByteString_clear(&data);
    ck_assert_uint_eq(testRemove(driver, root), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ft, root, &cls), UA_STATUSCODE_BADNODEIDUNKNOWN);
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ft, docs, &cls), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ft, file, &cls), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&root);
    UA_NodeId_clear(&docs);
    UA_NodeId_clear(&file);
} END_TEST

START_TEST(reuseRejectsConflictingDriversAndTypes) {
    UA_Server_getConfig(server_ft)->tcpEnabled = false;
    ck_assert_uint_eq(UA_Server_run_startup(server_ft), UA_STATUSCODE_GOOD);
    UA_NodeId root = addFileTypeInstance(server_ft, "Shared");
    UA_FileTransferNodeDescription description;
    memset(&description, 0, sizeof(description));
    description.nodeId = root;
    UA_FileTransferBackend a = memBackendWithFile("f.bin", "one");
    UA_FileTransferBackend b = memBackendWithFile("f.bin", "two");
    UA_Driver *first = NULL, *second = NULL;
    /* Both constructors precede registration: neither replaces Properties yet. */
    ck_assert_uint_eq(UA_FileTransferDriver_newFile(server_ft, &a.file, UA_STRING("f.bin"),
        &description, NULL, &first), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_FileTransferDriver_newFile(server_ft, &b.file, UA_STRING("f.bin"),
        &description, NULL, &second), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_addDriver(server_ft, first), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_addDriver(server_ft, second), UA_STATUSCODE_BADNODEIDEXISTS);
    ck_assert_uint_eq(second->free(second), UA_STATUSCODE_GOOD);
    /* Distinct Objects cannot bind the same Property to different drivers. */
    UA_NodeId otherRoot = addFileTypeInstance(server_ft, "Other");
    UA_NodeId otherSize = resolveChild(server_ft, otherRoot, "Size");
    ck_assert_uint_eq(UA_Server_deleteNode(server_ft, otherSize, true), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&otherSize);
    UA_NodeId sharedSize = resolveChild(server_ft, root, "Size");
    UA_ExpandedNodeId sharedTarget;
    UA_ExpandedNodeId_init(&sharedTarget);
    sharedTarget.nodeId = sharedSize;
    ck_assert_uint_eq(UA_Server_addReference(server_ft, otherRoot, UA_NS0ID(HASPROPERTY),
        sharedTarget, true), UA_STATUSCODE_GOOD);
    b = memBackendWithFile("f.bin", "two");
    description.nodeId = otherRoot;
    ck_assert_uint_eq(UA_FileTransferDriver_newFile(server_ft, &b.file, UA_STRING("f.bin"),
        &description, NULL, &second), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_addDriver(server_ft, second), UA_STATUSCODE_BADNODEIDEXISTS);
    ck_assert_uint_eq(second->free(second), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&otherRoot);
    UA_NodeId_clear(&sharedSize);
    UA_ByteString data = readFileContent(root);
    UA_ByteString expected = UA_BYTESTRING("one");
    ck_assert(UA_ByteString_equal(&data, &expected));
    UA_ByteString_clear(&data);
    ck_assert_uint_eq(testRemove(first, root), UA_STATUSCODE_GOOD);

    a = memBackendWithFile("f.bin", "retry");
    description.nodeId = UA_NS0ID(OBJECTSFOLDER); /* Existing, but not FileType */
    ck_assert_uint_eq(UA_FileTransferDriver_newFile(server_ft, &a.file, UA_STRING("f.bin"),
        &description, NULL, &first), UA_STATUSCODE_BADTYPEDEFINITIONINVALID);
    ck_assert_ptr_null(first);
    a.file.clear(&a.file);
    ck_assert_uint_eq(UA_Server_run_shutdown(server_ft), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&root);
} END_TEST

START_TEST(driverLimitsAndSessionCleanupAreIndependent) {
    UA_Driver *drivers[2];
    UA_NodeId roots[2];
    UA_UInt16 limit = 1;
    UA_KeyValueMap params = UA_KEYVALUEMAP_NULL;
    ck_assert_uint_eq(UA_KeyValueMap_setScalar(&params,
                          UA_QUALIFIEDNAME(0, "max-open-handles-per-session"),
                          &limit, &UA_TYPES[UA_TYPES_UINT16]), UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 2; i++) {
        UA_UInt32 maxRead = (i == 0) ? 4 : 8;
        ck_assert_uint_eq(UA_KeyValueMap_setScalar(&params,
                              UA_QUALIFIEDNAME(0, "max-read-length"), &maxRead,
                              &UA_TYPES[UA_TYPES_UINT32]), UA_STATUSCODE_GOOD);
        UA_FileTransferBackend backend = memBackendWithFile("f.bin", "abcdefgh");
        drivers[i] = newTestFile(server_ft, &backend.file, i == 0 ? "First" : "Second", &roots[i]);
        ck_assert_uint_eq(UA_KeyValueMap_copy(&params, &drivers[i]->params), UA_STATUSCODE_GOOD);
        registerTestDriver(drivers[i]);
        UA_UInt32 handle = callOpen(roots[i], UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
        callOpen(roots[i], UA_OPENFILEMODE_READ, UA_STATUSCODE_BADRESOURCEUNAVAILABLE);
        UA_ByteString data = callRead(roots[i], handle, 8, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(data.length, maxRead);
        UA_ByteString_clear(&data);
    }
    UA_KeyValueMap_clear(&params);
    /* The server distributes a Session-close notification to every driver. */
    UA_KeyValueMap payload = UA_KEYVALUEMAP_NULL;
    ck_assert_uint_eq(UA_KeyValueMap_setScalar(&payload, UA_QUALIFIEDNAME(0, "session-id"),
                          &server_ft->adminSession.sessionId, &UA_TYPES[UA_TYPES_NODEID]),
                      UA_STATUSCODE_GOOD);
    lockServer(server_ft);
    notifyApplication(server_ft, UA_APPLICATIONNOTIFICATIONTYPE_SESSION_CLOSED, payload);
    unlockServer(server_ft);
    UA_KeyValueMap_clear(&payload);
    for(size_t i = 0; i < 2; i++) {
        ck_assert_uint_eq(readOpenCount(roots[i]), 0);
        UA_UInt32 handle = callOpen(roots[i], UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
        callClose(roots[i], handle, UA_STATUSCODE_GOOD);
        UA_NodeId_clear(&roots[i]);
    }
} END_TEST

START_TEST(crossDriverCopyUsesDestinationPolicyAndBudget) {
    AccessPolicy srcPolicy, dstPolicy;
    UA_FileTransferBackend src = accessBackend(memBackendWithTree(), &srcPolicy);
    UA_FileTransferBackend dst;
    ck_assert_uint_eq(memBackend(&dst), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&dst, UA_STRING("target"), true), UA_STATUSCODE_GOOD);
    dst = accessBackend(dst, &dstPolicy);
    UA_NodeId srcRoot = mountNamedMem(src, "Source", NULL);
    FTConfig options;
    memset(&options, 0, sizeof(options));
    options.maxNodes = 3; /* root + target + one copied file */
    UA_NodeId dstRoot = mountNamedMem(dst, "Destination", &options);
    UA_NodeId source = resolveChild(server_ft, srcRoot, "readme.txt");
    UA_NodeId target = resolveChild(server_ft, dstRoot, "target");
    dstPolicy.target = dstRoot;
    dstPolicy.user = UA_FILEACCESS_READ | UA_FILEACCESS_WRITE;
    callMoveOrCopy(srcRoot, source, target, true, "copied", UA_STATUSCODE_BADUSERACCESSDENIED);
    dstPolicy.user |= UA_FILEACCESS_TRAVERSE;
    UA_NodeId copy = callMoveOrCopy(srcRoot, source, target, true, "copied", UA_STATUSCODE_GOOD);
    callMoveOrCopy(srcRoot, source, target, true, "overflow", UA_STATUSCODE_BADRESOURCEUNAVAILABLE);
    ck_assert(tryResolveChild(server_ft, target, "copied", NULL));
    ck_assert(!tryResolveChild(server_ft, target, "overflow", NULL));
    ck_assert_uint_eq(testRemove(driverForRoot(srcRoot), srcRoot), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(testRemove(driverForRoot(dstRoot), dstRoot), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&copy);
    UA_NodeId_clear(&target);
    UA_NodeId_clear(&source);
    UA_NodeId_clear(&dstRoot);
    UA_NodeId_clear(&srcRoot);
} END_TEST

/* New and existing subdirectories use one reconciliation pass. In particular,
 * a freshly mirrored subtree must not be listed again on the same refresh. */
START_TEST(reconciliationListsDirectoriesOnce) {
    UA_FileTransferBackend backend = memBackendWithTree();
    MemBackendContext *ctx = (MemBackendContext*)backend.file.context;
    UA_NodeId root = mountNamedMem(backend, "Tree", NULL);
    ck_assert_uint_eq(ctx->listCalls, 3); /* root, docs, docs/sub */
    ctx->listCalls = 0;
    ck_assert_uint_eq(testRefresh(driverForRoot(root), root), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ctx->listCalls, 3);

    ck_assert_uint_eq(createEntry(&backend, UA_STRING("new"), true), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&backend, UA_STRING("new/deep"), true), UA_STATUSCODE_GOOD);
    writeMemFile(&backend, "new/deep/file", "data");
    ctx->listCalls = 0;
    ck_assert_uint_eq(testRefresh(driverForRoot(root), root), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ctx->listCalls, 5);
    UA_NodeId added = resolveChild(server_ft, root, "new");
    UA_NodeId deep = resolveChild(server_ft, added, "deep");
    ck_assert(tryResolveChild(server_ft, deep, "file", NULL));

    UA_Driver *driver = driverForRoot(root);
    driver->stop(driver);
    ctx->listCalls = 0;
    ck_assert_uint_eq(driver->start(driver), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ctx->listCalls, 5);
    ck_assert_uint_eq(testRemove(driver, root), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&deep);
    UA_NodeId_clear(&added);
    UA_NodeId_clear(&root);
} END_TEST

/* An extra OPC UA parent does not change the driver's storage parent, nor
 * confer the ability to delete or move that parent's referenced files. */
START_TEST(storageParentsIgnoreExtraModelReferences) {
    UA_NodeId root = mountTree(NULL);
    UA_NodeId file = resolveChild(server_ft, root, "readme.txt");
    UA_NodeId docs = resolveChild(server_ft, root, "docs");
    UA_NodeId sub = resolveChild(server_ft, docs, "sub");
    UA_ExpandedNodeId target = UA_EXPANDEDNODEID_NULL;
    target.nodeId = file;
    ck_assert_uint_eq(UA_Server_addReference(server_ft, sub, UA_NS0ID(ORGANIZES), target, true),
                      UA_STATUSCODE_GOOD);
    callDelete(sub, file, UA_STATUSCODE_BADNOTFOUND);
    callMoveOrCopy(sub, file, root, true, "copy", UA_STATUSCODE_BADNOTFOUND);
    callDelete(root, docs, UA_STATUSCODE_GOOD);
    UA_UInt32 handle = callOpen(file, UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    callClose(file, handle, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(testRefresh(driverForRoot(root), root), UA_STATUSCODE_GOOD);
    ck_assert(tryResolveChild(server_ft, root, "readme.txt", NULL));
    ck_assert_uint_eq(testRemove(driverForRoot(root), root), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&sub);
    UA_NodeId_clear(&docs);
    UA_NodeId_clear(&file);
    UA_NodeId_clear(&root);
} END_TEST

START_TEST(failedStartKeepsOtherDriverCallbacks) {
    UA_NodeId root = addTestFile("First", "data", NULL);
    UA_MethodCallback original = NULL;
    ck_assert_uint_eq(UA_Server_getMethodNodeCallback(server_ft, UA_NS0ID(FILETYPE_OPEN),
                                                      &original), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_deleteNode(server_ft, UA_NS0ID(FILEDIRECTORYTYPE_MOVEORCOPY),
                                           true), UA_STATUSCODE_GOOD);
    UA_FileTransferBackend backend = memBackendWithFile("f.bin", "other");
    UA_Driver *second = newTestFile(server_ft, &backend.file, "Other", NULL);
    ck_assert_uint_eq(UA_Server_addDriver(server_ft, second), UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(second->start(second), UA_STATUSCODE_GOOD);
    second->stop(second);
    ck_assert_uint_eq(UA_Server_removeDriver(server_ft, second), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(second->free(second), UA_STATUSCODE_GOOD);
    UA_MethodCallback current = NULL;
    ck_assert_uint_eq(UA_Server_getMethodNodeCallback(server_ft, UA_NS0ID(FILETYPE_OPEN),
                                                      &current), UA_STATUSCODE_GOOD);
    ck_assert(current == original);
    UA_ByteString data = readFileContent(root);
    ck_assert_uint_eq(data.length, 4);
    UA_ByteString_clear(&data);
    UA_NodeId_clear(&root);
} END_TEST

/* Grow the listing beyond its initial allocation, remove entries throughout
 * the name index, and retain an open file until both Sessions have closed. */
START_TEST(indexedDirectoryAndSessionCleanup) {
    UA_FileTransferBackend backend;
    ck_assert_uint_eq(memBackend(&backend), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createEntry(&backend, UA_STRING("dir"), true), UA_STATUSCODE_GOOD);
    for(unsigned i = 0; i < 40; i++) {
        char path[32];
        snprintf(path, sizeof(path), "dir/file%02u", i);
        writeMemFile(&backend, path, "data");
    }
    UA_NodeId root;
    UA_Driver *driver = newTestDirectory(server_ft, &backend, &root);
    UA_UInt16 limit = 8;
    ck_assert_uint_eq(UA_KeyValueMap_setScalar(&driver->params,
        UA_QUALIFIEDNAME(0, "max-open-handles-per-session"), &limit,
        &UA_TYPES[UA_TYPES_UINT16]), UA_STATUSCODE_GOOD);
    registerTestDriver(driver);
    FileTransferDriver *ftd = (FileTransferDriver*)driver;
    UA_NodeId directory = resolveChild(server_ft, root, "dir");
    UA_NodeId files[40];
    for(unsigned i = 0; i < 40; i++) {
        char name[16];
        snprintf(name, sizeof(name), "file%02u", i);
        files[i] = resolveChild(server_ft, directory, name);
        ck_assert_ptr_nonnull(findFTEntry(ftd, &files[i]));
    }
    ck_assert_uint_eq(ftd->entryCount, 42);

    UA_NodeId sessions[2] = {UA_NODEID_STRING(1, "reader"), UA_NODEID_NUMERIC(1, 123)};
    UA_UInt32 handles[2][8];
    FTEntry *file = findFTEntry(ftd, &files[0]);
    lockServer(server_ft);
    for(size_t s = 0; s < 2; s++) {
        for(size_t i = 0; i < 8; i++) {
            ck_assert_uint_eq(openFileHandle(server_ft, file, &sessions[s], NULL,
                UA_OPENFILEMODE_READ, &handles[s][i]), UA_STATUSCODE_GOOD);
            ck_assert_ptr_null(findFTHandle(ftd, &sessions[1 - s], handles[s][i]));
        }
        UA_UInt32 rejected;
        ck_assert_uint_eq(openFileHandle(server_ft, file, &sessions[s], NULL,
            UA_OPENFILEMODE_READ, &rejected), UA_STATUSCODE_BADRESOURCEUNAVAILABLE);
    }
    unlockServer(server_ft);
    ck_assert_uint_eq(readOpenCount(files[0]), 16);
    ck_assert_uint_eq(ftd->root->subtreeHandleCount, 16);
    ck_assert_uint_eq(findFTEntry(ftd, &directory)->subtreeHandleCount, 16);

    for(unsigned i = 0; i < 40; i += 2) {
        char path[32];
        snprintf(path, sizeof(path), "dir/file%02u", i);
        ck_assert_uint_eq(backend.remove(&backend, UA_STRING(path)), UA_STATUSCODE_GOOD);
    }
    ck_assert_uint_eq(testRefresh(driver, root), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ftd->entryCount, 23); /* Root, dir, 20 files, retained file0 */
    ck_assert(file->zombie);
    for(unsigned i = 1; i < 40; i++)
        ck_assert_int_eq(findFTEntry(ftd, &files[i]) != NULL, i % 2 != 0);

    for(size_t s = 0; s < 2; s++) {
        UA_KeyValueMap payload = UA_KEYVALUEMAP_NULL;
        ck_assert_uint_eq(UA_KeyValueMap_setScalar(&payload,
            UA_QUALIFIEDNAME(0, "session-id"), &sessions[s], &UA_TYPES[UA_TYPES_NODEID]),
            UA_STATUSCODE_GOOD);
        lockServer(server_ft);
        notifyApplication(server_ft, UA_APPLICATIONNOTIFICATIONTYPE_SESSION_CLOSED, payload);
        unlockServer(server_ft);
        UA_KeyValueMap_clear(&payload);
        for(size_t i = 0; i < 8; i++) {
            ck_assert_ptr_null(findFTHandle(ftd, &sessions[s], handles[s][i]));
            if(s == 0)
                ck_assert_ptr_nonnull(findFTHandle(ftd, &sessions[1], handles[1][i]));
        }
        ck_assert_uint_eq(ftd->root->subtreeHandleCount, s == 0 ? 8 : 0);
    }
    ck_assert_ptr_null(findFTEntry(ftd, &files[0]));
    ck_assert_uint_eq(ftd->entryCount, 22);
    callDelete(root, directory, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ftd->entryCount, 1);
    for(size_t i = 0; i < 40; i++) {
        ck_assert_ptr_null(findFTEntry(ftd, &files[i]));
        UA_NodeId_clear(&files[i]);
    }
    UA_NodeId_clear(&directory);
    UA_NodeId_clear(&root);
} END_TEST

static UA_Boolean duplicateListing;

static UA_StatusCode
duplicateListDirectory(UA_FileTransferBackend *backend, const UA_String path,
                       UA_FileTransferListCallback cb, void *context) {
    UA_StatusCode res = memListDirectory(backend, path, cb, context);
    if(res == UA_STATUSCODE_GOOD && duplicateListing)
        res = memListDirectory(backend, path, cb, context);
    return res;
}

START_TEST(duplicateListingKeepsTree) {
    UA_FileTransferBackend backend = memBackendWithFile("old.txt", "data");
    backend.listDirectory = duplicateListDirectory;
    duplicateListing = false;
    UA_NodeId root = mountNamedMem(backend, "Tree", NULL);
    UA_NodeId original = resolveChild(server_ft, root, "old.txt");
    ck_assert_uint_eq(backend.remove(&backend, UA_STRING("old.txt")), UA_STATUSCODE_GOOD);
    writeMemFile(&backend, "new.txt", "data");
    duplicateListing = true;
    UA_Driver *driver = driverForRoot(root);
    ck_assert_uint_eq(testRefresh(driver, root), UA_STATUSCODE_BADBROWSENAMEDUPLICATED);
    ck_assert_ptr_nonnull(findFTEntry((FileTransferDriver*)driver, &original));
    ck_assert(!tryResolveChild(server_ft, root, "new.txt", NULL));
    duplicateListing = false;
    ck_assert_uint_eq(testRefresh(driver, root), UA_STATUSCODE_GOOD);
    /* The server may recycle the removed Object's numeric NodeId. */
    ck_assert(!tryResolveChild(server_ft, root, "old.txt", NULL));
    ck_assert(tryResolveChild(server_ft, root, "new.txt", NULL));
    UA_NodeId_clear(&original);
    UA_NodeId_clear(&root);
} END_TEST

int main(void) {
    Suite *s = suite_create("server_filetransfer");

    TCase *tc_lifecycle = tcase_create("Driver Lifecycle");
    tcase_add_test(tc_lifecycle, multipleDriversHaveIndependentLifecycle);
    tcase_add_test(tc_lifecycle, backendLivesUntilDriverFree);
    tcase_add_test(tc_lifecycle, constructorServesSharedSubtypeMethods);
    tcase_add_test(tc_lifecycle, constructorFailureLeavesBackendAndOutputs);
    tcase_add_test(tc_lifecycle, failedRegistrationRetainsObjects);
    tcase_add_test(tc_lifecycle, directoryLifetimeAndAutomaticRefresh);
    tcase_add_test(tc_lifecycle, disabledPeriodicRefresh);
    tcase_add_test(tc_lifecycle, directoryBackendCastsFileBackend);
    tcase_add_test(tc_lifecycle, defaultNodeDescriptions);
    tcase_add_test(tc_lifecycle, reuseFileRestoresProperties);
    tcase_add_test(tc_lifecycle, reuseDerivedPropertyDataTypes);
    tcase_add_test(tc_lifecycle, reuseNestedDirectoryStructure);
    tcase_add_test(tc_lifecycle, reuseRejectsConflictingDriversAndTypes);
    tcase_add_test(tc_lifecycle, driverLimitsAndSessionCleanupAreIndependent);
    tcase_add_test(tc_lifecycle, crossDriverCopyUsesDestinationPolicyAndBudget);
    tcase_add_test(tc_lifecycle, failedStartKeepsOtherDriverCallbacks);
    tcase_add_test(tc_lifecycle, restartDriver);
    tcase_add_test(tc_lifecycle, mountBeforeStart);
    tcase_add_test(tc_lifecycle, instanceSharesTypeMethodNodes);
    tcase_add_test(tc_lifecycle, instanceHasMandatoryProperties);
    tcase_add_test(tc_lifecycle, stopReleasesTypeMethodCallbacks);
    tcase_add_test(tc_lifecycle, optionalPropertiesAreNotDuplicated);
    tcase_add_test(tc_lifecycle, copiedMethodsServedAfterStart);
    tcase_add_checked_fixture(tc_lifecycle, setup, teardown);
    suite_add_tcase(s, tc_lifecycle);

    TCase *tc_file = tcase_create("FileType Methods");
    tcase_add_test(tc_file, fileProperties);
    tcase_add_test(tc_file, fileOpenModes);
    tcase_add_test(tc_file, fileLocking);
    tcase_add_test(tc_file, fileReadWrite);
    tcase_add_test(tc_file, openHandleSurvivesVanishedFile);
    tcase_add_test(tc_file, openHandleSurvivesReplacedParent);
    tcase_add_test(tc_file, readAtEndReturnsEmptyByteString);
    tcase_add_test(tc_file, fileBadHandles);
    tcase_add_test(tc_file, fileReadOnlyMount);
    tcase_add_test(tc_file, fileHandleLimits);
    tcase_add_test(tc_file, removeFileClosesHandles);
    tcase_add_test(tc_file, fileMaxByteStringLength);
    tcase_add_test(tc_file, fileWriteRespectsMaxByteStringLength);
    tcase_add_test(tc_file, readOnlyBackendNeedsNoWriteCallbacks);
    tcase_add_test(tc_file, userWritableFollowsStorage);
    tcase_add_test(tc_file, fileAccessRightsEnforced);
#ifndef _WIN32
    tcase_add_test(tc_file, localDirectoryAccessRightsFollowPermissions);
#endif
    tcase_add_test(tc_file, fileMimeType);
    tcase_add_checked_fixture(tc_file, setup, teardown);
    suite_add_tcase(s, tc_file);

    TCase *tc_dir = tcase_create("FileDirectoryType Methods");
    tcase_add_test(tc_dir, mountScanMirrorsTree);
    tcase_add_test(tc_dir, listingCarriesFullInfoAndInlineNames);
    tcase_add_test(tc_dir, mountScanDepthLimit);
    tcase_add_test(tc_dir, depthLimitAppliesToMethods);
    tcase_add_test(tc_dir, depthLimitAppliesToCrossMountTargets);
    tcase_add_test(tc_dir, dirCreateMethods);
    tcase_add_test(tc_dir, dirReadOnlyMount);
    tcase_add_test(tc_dir, directoryAccessRightsEnforced);
    tcase_add_test(tc_dir, changedListingDoesNotBypassAccessCallbacks);
    tcase_add_test(tc_dir, unmirroredEntriesDoNotBypassAccessCallbacks);
    tcase_add_test(tc_dir, dirDelete);
    tcase_add_test(tc_dir, dirMoveOrCopy);
    tcase_add_test(tc_dir, crossMountMoveCopy);
    tcase_add_test(tc_dir, crossMountReadOnlySource);
    tcase_add_test(tc_dir, crossMountMoveKeepsDataOnFailedDelete);
    tcase_add_test(tc_dir, failedDirectoryCopyLeavesNothing);
    tcase_add_test(tc_dir, createFileRemovedOnFailedOpen);
    tcase_add_test(tc_dir, createFileRemovedOnFailedMirror);
    tcase_add_test(tc_dir, partialDeleteRemovesDeletedObjects);
    tcase_add_test(tc_dir, moveOfUnlistableDirectorySucceeds);
    tcase_add_test(tc_dir, dirRefresh);
    tcase_add_test(tc_dir, reconciliationListsDirectoriesOnce);
    tcase_add_test(tc_dir, storageParentsIgnoreExtraModelReferences);
    tcase_add_test(tc_dir, removeDirectoryWithOpenHandles);
    tcase_add_test(tc_dir, dirCreateRespectsMaxNodes);
    tcase_add_test(tc_dir, mirroredNamesUseMountNamespace);
    tcase_add_test(tc_dir, mountRejectsUnknownNamespace);
    tcase_add_test(tc_dir, mountSkipsUnreadableEntries);
    tcase_add_test(tc_dir, scanSummaryLoggedOnChange);
    tcase_add_test(tc_dir, removalReleasesValueSources);
    tcase_add_test(tc_dir, copyFailsOnDestinationClose);
    tcase_add_test(tc_dir, maxNodesCountsRoot);
    tcase_add_test(tc_dir, failedListingKeepsTree);
    tcase_add_test(tc_dir, duplicateListingKeepsTree);
    tcase_add_test(tc_dir, indexedDirectoryAndSessionCleanup);
    tcase_add_test(tc_dir, maxNodesReusedAcrossDirectories);
    tcase_add_test(tc_dir, replacingEntryIsNotZombie);
    tcase_add_test(tc_dir, localDirectoryMount);
    tcase_add_test(tc_dir, localFileMount);
    tcase_add_test(tc_dir, localDirectoryLargeFile);
    tcase_add_test(tc_dir, moveOrCopyKeepsUnmirroredTarget);
# ifndef _WIN32
    tcase_add_test(tc_dir, localDirectorySkipsLinksAndSpecialFiles);
# endif
    tcase_add_checked_fixture(tc_dir, setup, teardown);
    suite_add_tcase(s, tc_dir);

    TCase *tc_backend = tcase_create("Storage Backends");
    tcase_add_test(tc_backend, memoryBackendContract);
    tcase_add_test(tc_backend, localDirectoryBackendContract);
    tcase_add_test(tc_backend, localDirectoryBackendSandbox);
    tcase_add_test(tc_backend, localDirectoryUtf8Names);
    suite_add_tcase(s, tc_backend);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
