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

#include <check.h>
#include <stdlib.h>

#if defined(UA_ENABLE_METHODCALLS) && defined(UA_GENERATED_NAMESPACE_ZERO_FULL)
# define UA_TEST_ENABLE_FILETRANSFER
#endif

UA_Server *server_ft;
static UA_FileTransferDriver *ftDriver;

static void setup(void) {
    server_ft = UA_Server_newForUnitTest();
#ifdef UA_TEST_ENABLE_FILETRANSFER
    ftDriver = UA_FileTransferDriver_new(UA_KEYVALUEMAP_NULL);
    ck_assert_ptr_nonnull(ftDriver);
    ck_assert_uint_eq(UA_Server_addDriver(server_ft, &ftDriver->drv),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ftDriver->drv.start(&ftDriver->drv),
                      UA_STATUSCODE_GOOD);
#endif
}

static void teardown(void) {
#ifdef UA_TEST_ENABLE_FILETRANSFER
    ftDriver->drv.stop(&ftDriver->drv);
    ck_assert_uint_eq(UA_Server_removeDriver(server_ft, &ftDriver->drv),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ftDriver->drv.free(&ftDriver->drv),
                      UA_STATUSCODE_GOOD);
#endif
    UA_Server_delete(server_ft);
}

#ifdef UA_TEST_ENABLE_FILETRANSFER

/**************************************
 * In-Memory Test Backend
 *
 * A minimal UA_FileTransferBackend implementation that stores the file
 * content on the heap. Used to test the backend contract and the driver
 * independently of the local filesystem.
 **************************************/

#define MEM_MAXENTRIES 64
#define MEM_MAXPATH 256

typedef struct {
    UA_Boolean used;
    UA_Boolean isDir;
    char path[MEM_MAXPATH];
    UA_ByteString content;
    UA_DateTime mtime;
} MemEntry;

typedef struct {
    MemEntry entries[MEM_MAXENTRIES];
} MemBackendContext;

typedef struct {
    MemEntry *entry;
    size_t pos;
} MemOpenFile;

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

static UA_StatusCode
memOpenFile(UA_FileTransferBackend *b, const UA_String path,
            UA_Byte mode, void **fileContext) {
    MemBackendContext *ctx = (MemBackendContext*)b->context;
    MemEntry *e = memFind(ctx, path);
    if(!e || e->isDir)
        return UA_STATUSCODE_BADNOTFOUND;
    if(!(mode & (UA_OPENFILEMODE_READ | UA_OPENFILEMODE_WRITE)))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(mode & UA_OPENFILEMODE_ERASEEXISTING) {
        UA_ByteString_clear(&e->content);
        e->mtime = UA_DateTime_now();
    }
    MemOpenFile *of = (MemOpenFile*)UA_calloc(1, sizeof(MemOpenFile));
    if(!of)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    of->entry = e;
    of->pos = (mode & UA_OPENFILEMODE_APPEND) ? e->content.length : 0;
    *fileContext = of;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memCloseFile(UA_FileTransferBackend *b, void *fileContext) {
    UA_free(fileContext);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memRead(UA_FileTransferBackend *b, void *fileContext,
        UA_Int32 length, UA_ByteString *out) {
    if(length <= 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    MemOpenFile *of = (MemOpenFile*)fileContext;
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
memWrite(UA_FileTransferBackend *b, void *fileContext,
         const UA_ByteString data) {
    if(data.length == 0)
        return UA_STATUSCODE_GOOD;
    MemOpenFile *of = (MemOpenFile*)fileContext;
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
memGetPosition(UA_FileTransferBackend *b, void *fileContext,
               UA_UInt64 *outPosition) {
    MemOpenFile *of = (MemOpenFile*)fileContext;
    *outPosition = of->pos;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memSetPosition(UA_FileTransferBackend *b, void *fileContext,
               UA_UInt64 position) {
    MemOpenFile *of = (MemOpenFile*)fileContext;
    of->pos = (position < of->entry->content.length) ?
        (size_t)position : of->entry->content.length;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memGetAttributes(UA_FileTransferBackend *b, const UA_String path,
                 UA_FileTransferFileInfo *outInfo) {
    MemBackendContext *ctx = (MemBackendContext*)b->context;
    MemEntry *e = memFind(ctx, path);
    if(!e) {
        /* The root directory always exists */
        if(path.length == 0) {
            memset(outInfo, 0, sizeof(UA_FileTransferFileInfo));
            outInfo->isDirectory = true;
            outInfo->writable = true;
            return UA_STATUSCODE_GOOD;
        }
        return UA_STATUSCODE_BADNOTFOUND;
    }
    memset(outInfo, 0, sizeof(UA_FileTransferFileInfo));
    outInfo->size = e->content.length;
    outInfo->lastModified = e->mtime;
    outInfo->isDirectory = e->isDir;
    outInfo->writable = true;
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
    MemBackendContext *ctx = (MemBackendContext*)b->context;
    if(path.length > 0) {
        MemEntry *dir = memFind(ctx, path);
        if(!dir || !dir->isDir)
            return UA_STATUSCODE_BADNOTFOUND;
    }
    for(size_t i = 0; i < MEM_MAXENTRIES; i++) {
        MemEntry *e = &ctx->entries[i];
        if(!e->used || !memIsDirectChild(e->path, path))
            continue;
        const char *name = e->path;
        if(path.length > 0)
            name += path.length + 1;
        cb(listContext, UA_STRING((char*)(uintptr_t)name), e->isDir);
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memCreateEntry(UA_FileTransferBackend *b, const UA_String path,
               UA_Boolean isDir) {
    MemBackendContext *ctx = (MemBackendContext*)b->context;
    if(memFind(ctx, path))
        return UA_STATUSCODE_BADBROWSENAMEDUPLICATED;
    if(!memAdd(ctx, path, isDir))
        return UA_STATUSCODE_BADOUTOFMEMORY;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memCreateFile(UA_FileTransferBackend *b, const UA_String path) {
    return memCreateEntry(b, path, false);
}

static UA_StatusCode
memCreateDirectory(UA_FileTransferBackend *b, const UA_String path) {
    return memCreateEntry(b, path, true);
}

static UA_StatusCode
memRemove(UA_FileTransferBackend *b, const UA_String path) {
    MemBackendContext *ctx = (MemBackendContext*)b->context;
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
    MemBackendContext *ctx = (MemBackendContext*)b->context;
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
memClear(UA_FileTransferBackend *b) {
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
    out->context = ctx;
    out->openFile = memOpenFile;
    out->closeFile = memCloseFile;
    out->read = memRead;
    out->write = memWrite;
    out->getPosition = memGetPosition;
    out->setPosition = memSetPosition;
    out->getAttributes = memGetAttributes;
    out->listDirectory = memListDirectory;
    out->createFile = memCreateFile;
    out->createDirectory = memCreateDirectory;
    out->remove = memRemove;
    out->rename = memRename;
    out->copy = NULL;
    out->clear = memClear;
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
} ListResult;

static void
listCollector(void *listContext, const UA_String name, UA_Boolean isDirectory) {
    ListResult *lr = (ListResult*)listContext;
    if(lr->count >= 16 || name.length >= 64)
        return;
    memcpy(lr->names[lr->count], name.data, name.length);
    lr->names[lr->count][name.length] = 0;
    lr->isDir[lr->count] = isDirectory;
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
    UA_StatusCode res;
    void *fc = NULL;
    UA_ByteString out;
    UA_UInt64 pos = 0;
    UA_FileTransferFileInfo info;

    /* Create a directory and a file */
    ck_assert_uint_eq(b->createDirectory(b, UA_STRING("sub")), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b->createFile(b, UA_STRING("hello.txt")), UA_STATUSCODE_GOOD);

    /* Duplicates are rejected */
    ck_assert_uint_eq(b->createFile(b, UA_STRING("hello.txt")),
                      UA_STATUSCODE_BADBROWSENAMEDUPLICATED);
    ck_assert_uint_eq(b->createDirectory(b, UA_STRING("sub")),
                      UA_STATUSCODE_BADBROWSENAMEDUPLICATED);

    /* Write content */
    res = b->openFile(b, UA_STRING("hello.txt"), UA_OPENFILEMODE_WRITE, &fc);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b->write(b, fc, UA_BYTESTRING("Hello World")),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b->getPosition(b, fc, &pos), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pos, 11);
    ck_assert_uint_eq(b->closeFile(b, fc), UA_STATUSCODE_GOOD);

    /* Attributes */
    ck_assert_uint_eq(b->getAttributes(b, UA_STRING("hello.txt"), &info),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(info.size, 11);
    ck_assert(!info.isDirectory);
    ck_assert_uint_eq(b->getAttributes(b, UA_STRING("sub"), &info),
                      UA_STATUSCODE_GOOD);
    ck_assert(info.isDirectory);
    ck_assert_uint_eq(b->getAttributes(b, UA_STRING("missing"), &info),
                      UA_STATUSCODE_BADNOTFOUND);

    /* Read in chunks until the end of the file */
    res = b->openFile(b, UA_STRING("hello.txt"), UA_OPENFILEMODE_READ, &fc);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b->read(b, fc, 5, &out), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.length, 5);
    ck_assert_int_eq(memcmp(out.data, "Hello", 5), 0);
    UA_ByteString_clear(&out);
    ck_assert_uint_eq(b->getPosition(b, fc, &pos), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pos, 5);
    ck_assert_uint_eq(b->read(b, fc, 100, &out), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.length, 6);
    ck_assert_int_eq(memcmp(out.data, " World", 6), 0);
    UA_ByteString_clear(&out);
    ck_assert_uint_eq(b->read(b, fc, 10, &out), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.length, 0); /* EOF */
    UA_ByteString_clear(&out);

    /* Positioning with clamping */
    ck_assert_uint_eq(b->setPosition(b, fc, 6), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b->read(b, fc, 5, &out), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.length, 5);
    ck_assert_int_eq(memcmp(out.data, "World", 5), 0);
    UA_ByteString_clear(&out);
    ck_assert_uint_eq(b->setPosition(b, fc, 1000), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b->getPosition(b, fc, &pos), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pos, 11);
    ck_assert_uint_eq(b->closeFile(b, fc), UA_STATUSCODE_GOOD);

    /* EraseExisting truncates */
    res = b->openFile(b, UA_STRING("hello.txt"),
                      UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING, &fc);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b->write(b, fc, UA_BYTESTRING("Hi")), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b->closeFile(b, fc), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b->getAttributes(b, UA_STRING("hello.txt"), &info),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(info.size, 2);

    /* Append positions at the end */
    res = b->openFile(b, UA_STRING("hello.txt"),
                      UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_APPEND, &fc);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b->getPosition(b, fc, &pos), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pos, 2);
    ck_assert_uint_eq(b->write(b, fc, UA_BYTESTRING("!")), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b->closeFile(b, fc), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b->getAttributes(b, UA_STRING("hello.txt"), &info),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(info.size, 3);

    /* Empty write is a no-op */
    res = b->openFile(b, UA_STRING("hello.txt"), UA_OPENFILEMODE_WRITE, &fc);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b->write(b, fc, UA_BYTESTRING_NULL), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b->closeFile(b, fc), UA_STATUSCODE_GOOD);

    /* Invalid open modes and missing files */
    ck_assert_uint_eq(b->openFile(b, UA_STRING("hello.txt"), 0, &fc),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(b->openFile(b, UA_STRING("missing"),
                                  UA_OPENFILEMODE_READ, &fc),
                      UA_STATUSCODE_BADNOTFOUND);
    ck_assert_uint_eq(b->openFile(b, UA_STRING("missing"),
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

    /* Rename/move */
    ck_assert_uint_eq(b->rename(b, UA_STRING("hello.txt"),
                                UA_STRING("sub/hello2.txt")),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b->getAttributes(b, UA_STRING("hello.txt"), &info),
                      UA_STATUSCODE_BADNOTFOUND);
    ck_assert_uint_eq(b->getAttributes(b, UA_STRING("sub/hello2.txt"), &info),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(info.size, 3);

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
    b.clear(&b);
} END_TEST

#ifndef _WIN32

#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

static char scratchDir[64];

static void
makeScratchDir(void) {
    strcpy(scratchDir, "/tmp/check_filetransfer_XXXXXX");
    ck_assert_ptr_nonnull(mkdtemp(scratchDir));
}

static void
removeTree(const char *path) {
    DIR *dir = opendir(path);
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

START_TEST(localFilesystemBackendContract) {
    makeScratchDir();
    UA_FileTransferBackend b;
    ck_assert_uint_eq(UA_FileTransferBackend_localFilesystem(
                          UA_STRING(scratchDir), &b),
                      UA_STATUSCODE_GOOD);
    runBackendContract(&b);
    b.clear(&b);
    removeTree(scratchDir);
} END_TEST

START_TEST(localFilesystemBackendSandbox) {
    makeScratchDir();
    UA_FileTransferBackend b;
    ck_assert_uint_eq(UA_FileTransferBackend_localFilesystem(
                          UA_STRING(scratchDir), &b),
                      UA_STATUSCODE_GOOD);

    /* Paths that could escape the root or are malformed are rejected */
    const char *badPaths[8] = {"..", "../escape", "a/../../b", "/absolute",
                               "a//b", "a/./b", "trailing/", "back\\slash"};
    void *fc = NULL;
    for(size_t i = 0; i < 8; i++) {
        UA_String p = UA_STRING((char*)(uintptr_t)badPaths[i]);
        ck_assert_uint_eq(b.openFile(&b, p, UA_OPENFILEMODE_READ, &fc),
                          UA_STATUSCODE_BADINVALIDARGUMENT);
        ck_assert_uint_eq(b.createFile(&b, p), UA_STATUSCODE_BADINVALIDARGUMENT);
        ck_assert_uint_eq(b.createDirectory(&b, p),
                          UA_STATUSCODE_BADINVALIDARGUMENT);
        ck_assert_uint_eq(b.remove(&b, p), UA_STATUSCODE_BADINVALIDARGUMENT);
    }

    /* The backend requires an existing root directory */
    UA_FileTransferBackend b2;
    ck_assert_uint_eq(UA_FileTransferBackend_localFilesystem(
                          UA_STRING("/nonexistent-filetransfer-root"), &b2),
                      UA_STATUSCODE_BADNOTFOUND);

    b.clear(&b);
    removeTree(scratchDir);
} END_TEST

#endif /* !_WIN32 */

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

START_TEST(addDriver_rejectsDuplicateFileTransfer) {
    UA_FileTransferDriver *second = UA_FileTransferDriver_new(UA_KEYVALUEMAP_NULL);
    ck_assert_ptr_nonnull(second);

    ck_assert_uint_eq(UA_Server_addDriver(server_ft, &second->drv),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(second->drv.start(&second->drv),
                      UA_STATUSCODE_BADALREADYEXISTS);
    ck_assert_uint_eq(UA_Server_removeDriver(server_ft, &second->drv),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(second->drv.free(&second->drv),
                      UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(restartDriver) {
    ftDriver->drv.stop(&ftDriver->drv);
    ck_assert_uint_eq(ftDriver->drv.start(&ftDriver->drv),
                      UA_STATUSCODE_GOOD);
} END_TEST

/* A FileType instance must reference the method nodes of the FileType
 * ObjectType itself (methods are not copied during instantiation). The
 * method callbacks registered on the type method nodes then dispatch on the
 * objectId. The whole driver design relies on this. */
START_TEST(instanceSharesTypeMethodNodes) {
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

    UA_CallMethodResult result = UA_Server_call(server_ft, &callMethodRequest);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_BADNOTFOUND);
    UA_CallMethodResult_clear(&result);

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
    ck_assert_uint_eq(b.createFile(&b, path), UA_STATUSCODE_GOOD);
    if(content && strlen(content) > 0) {
        void *fc = NULL;
        ck_assert_uint_eq(b.openFile(&b, path, UA_OPENFILEMODE_WRITE, &fc),
                          UA_STATUSCODE_GOOD);
        UA_ByteString data = UA_BYTESTRING((char*)(uintptr_t)content);
        ck_assert_uint_eq(b.write(&b, fc, data), UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(b.closeFile(&b, fc), UA_STATUSCODE_GOOD);
    }
    return b;
}

/* Helper: add a standalone file backed by a fresh memory backend */
static UA_NodeId
addTestFile(const char *browseName, const char *content,
            const UA_FileTransferMountOptions *options) {
    UA_FileTransferBackend b = memBackendWithFile("f.bin", content);
    UA_NodeId fileNodeId = UA_NODEID_NULL;
    UA_StatusCode res = ftDriver->addFile(
        ftDriver, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
        UA_QUALIFIEDNAME(0, (char*)(uintptr_t)browseName), b,
        UA_STRING("f.bin"), options, &fileNodeId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    return fileNodeId;
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

    ck_assert_uint_eq(ftDriver->removeFile(ftDriver, fileId), UA_STATUSCODE_GOOD);
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
    ck_assert_uint_eq(ftDriver->removeFile(ftDriver, fileId), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileId);

    /* A driver configured with a custom max-read-length reflects that value */
    UA_Server *server = UA_Server_newForUnitTest();
    UA_UInt32 maxRead = 4096;
    UA_KeyValueMap params = UA_KEYVALUEMAP_NULL;
    UA_KeyValueMap_setScalar(&params, UA_QUALIFIEDNAME(0, "max-read-length"),
                             &maxRead, &UA_TYPES[UA_TYPES_UINT32]);
    UA_FileTransferDriver *driver = UA_FileTransferDriver_new(params);
    UA_KeyValueMap_clear(&params);
    ck_assert_ptr_nonnull(driver);
    ck_assert_uint_eq(UA_Server_addDriver(server, &driver->drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(driver->drv.start(&driver->drv), UA_STATUSCODE_GOOD);

    UA_NodeId customFile = UA_NODEID_NULL;
    UA_FileTransferBackend b = memBackendWithFile("f.bin", "x");
    ck_assert_uint_eq(driver->addFile(driver, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                                      UA_QUALIFIEDNAME(0, "CustomFile"), b,
                                      UA_STRING("f.bin"), NULL, &customFile),
                      UA_STATUSCODE_GOOD);
    UA_QualifiedName qn = UA_QUALIFIEDNAME(0, "MaxByteStringLength");
    UA_BrowsePathResult bpr =
        UA_Server_browseSimplifiedBrowsePath(server, customFile, 1, &qn);
    ck_assert_uint_eq(bpr.statusCode, UA_STATUSCODE_GOOD);
    UA_Variant custom;
    ck_assert_uint_eq(UA_Server_readValue(server, bpr.targets[0].targetId.nodeId,
                                          &custom), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(*(UA_UInt32*)custom.data, 4096);
    UA_Variant_clear(&custom);
    UA_BrowsePathResult_clear(&bpr);

    driver->drv.stop(&driver->drv);
    ck_assert_uint_eq(UA_Server_removeDriver(server, &driver->drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(driver->drv.free(&driver->drv), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&customFile);
    UA_Server_delete(server);
} END_TEST

#ifndef _WIN32
/* MimeType is inferred from the extension by the local filesystem backend */
START_TEST(fileMimeType) {
    makeScratchDir();

    UA_FileTransferBackend pre;
    ck_assert_uint_eq(UA_FileTransferBackend_localFilesystem(
                          UA_STRING(scratchDir), &pre), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pre.createFile(&pre, UA_STRING("a.txt")), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pre.createFile(&pre, UA_STRING("b.json")), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pre.createFile(&pre, UA_STRING("c")), UA_STATUSCODE_GOOD);
    pre.clear(&pre);

    UA_FileTransferBackend b;
    ck_assert_uint_eq(UA_FileTransferBackend_localFilesystem(
                          UA_STRING(scratchDir), &b), UA_STATUSCODE_GOOD);
    UA_NodeId fsId = UA_NODEID_NULL;
    ck_assert_uint_eq(ftDriver->addFileSystem(
                          ftDriver, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                          UA_QUALIFIEDNAME(0, "FileSystem"), b, NULL, &fsId),
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

    ck_assert_uint_eq(ftDriver->removeFileSystem(ftDriver, fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&txtId);
    UA_NodeId_clear(&jsonId);
    UA_NodeId_clear(&cId);
    UA_NodeId_clear(&fsId);
    removeTree(scratchDir);
} END_TEST
#endif

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

    ck_assert_uint_eq(ftDriver->removeFile(ftDriver, fileId), UA_STATUSCODE_GOOD);
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

    ck_assert_uint_eq(ftDriver->removeFile(ftDriver, fileId), UA_STATUSCODE_GOOD);
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

    ck_assert_uint_eq(ftDriver->removeFile(ftDriver, fileId), UA_STATUSCODE_GOOD);
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

    ck_assert_uint_eq(ftDriver->removeFile(ftDriver, fileA), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ftDriver->removeFile(ftDriver, fileB), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileA);
    UA_NodeId_clear(&fileB);
} END_TEST

START_TEST(fileReadOnlyMount) {
    UA_FileTransferMountOptions options = {true, 0, 0, NULL, NULL};
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

    ck_assert_uint_eq(ftDriver->removeFile(ftDriver, fileId), UA_STATUSCODE_GOOD);
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

    UA_FileTransferDriver *driver = UA_FileTransferDriver_new(params);
    UA_KeyValueMap_clear(&params);
    ck_assert_ptr_nonnull(driver);
    ck_assert_uint_eq(UA_Server_addDriver(server, &driver->drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(driver->drv.start(&driver->drv), UA_STATUSCODE_GOOD);

    UA_NodeId fileA = UA_NODEID_NULL;
    UA_NodeId fileB = UA_NODEID_NULL;
    UA_FileTransferBackend bA = memBackendWithFile("a.bin", "aaa");
    UA_FileTransferBackend bB = memBackendWithFile("b.bin", "bbb");
    ck_assert_uint_eq(driver->addFile(driver, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                                      UA_QUALIFIEDNAME(0, "LimitFileA"), bA,
                                      UA_STRING("a.bin"), NULL, &fileA),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(driver->addFile(driver, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                                      UA_QUALIFIEDNAME(0, "LimitFileB"), bB,
                                      UA_STRING("b.bin"), NULL, &fileB),
                      UA_STATUSCODE_GOOD);

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
    driver->drv.stop(&driver->drv);
    ck_assert_uint_eq(UA_Server_removeDriver(server, &driver->drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(driver->drv.free(&driver->drv), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileA);
    UA_NodeId_clear(&fileB);
    UA_Server_delete(server);
} END_TEST

START_TEST(removeFileClosesHandles) {
    UA_NodeId fileId = addTestFile("RemoveFile", "content", NULL);

    UA_UInt32 h = callOpen(fileId, UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    (void)h;
    ck_assert_uint_eq(ftDriver->removeFile(ftDriver, fileId), UA_STATUSCODE_GOOD);

    /* The object is gone from the address space */
    UA_QualifiedName browseName;
    ck_assert_uint_ne(UA_Server_readBrowseName(server_ft, fileId, &browseName),
                      UA_STATUSCODE_GOOD);

    /* Removing again fails */
    ck_assert_uint_eq(ftDriver->removeFile(ftDriver, fileId),
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
    ck_assert_uint_eq(b.createFile(&b, UA_STRING("readme.txt")), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b.createDirectory(&b, UA_STRING("docs")), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b.createFile(&b, UA_STRING("docs/a.txt")), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b.createDirectory(&b, UA_STRING("docs/sub")), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b.createFile(&b, UA_STRING("docs/sub/b.txt")), UA_STATUSCODE_GOOD);
    void *fc = NULL;
    ck_assert_uint_eq(b.openFile(&b, UA_STRING("docs/a.txt"),
                                 UA_OPENFILEMODE_WRITE, &fc), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b.write(&b, fc, UA_BYTESTRING("content-a")), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b.closeFile(&b, fc), UA_STATUSCODE_GOOD);
    return b;
}

static UA_NodeId
mountTree(const UA_FileTransferMountOptions *options) {
    UA_NodeId fsId = UA_NODEID_NULL;
    UA_StatusCode res = ftDriver->addFileSystem(
        ftDriver, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
        UA_QUALIFIEDNAME(0, "FileSystem"), memBackendWithTree(), options, &fsId);
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

    ck_assert_uint_eq(ftDriver->removeFileSystem(ftDriver, fsId),
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
    UA_FileTransferMountOptions options = {false, 1, 0, NULL, NULL};
    UA_NodeId fsId = mountTree(&options);

    /* Only the first level is mirrored */
    UA_NodeId docsId;
    ck_assert(tryResolveChild(server_ft, fsId, "readme.txt", NULL));
    ck_assert(tryResolveChild(server_ft, fsId, "docs", &docsId));
    ck_assert(!tryResolveChild(server_ft, docsId, "a.txt", NULL));

    ck_assert_uint_eq(ftDriver->removeFileSystem(ftDriver, fsId),
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

    ck_assert_uint_eq(ftDriver->removeFileSystem(ftDriver, fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&newDirId);
    UA_NodeId_clear(&newFileId);
    UA_NodeId_clear(&openFileId);
    UA_NodeId_clear(&fsId);
} END_TEST

START_TEST(dirReadOnlyMount) {
    UA_FileTransferMountOptions options = {true, 0, 0, NULL, NULL};
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

    ck_assert_uint_eq(ftDriver->removeFileSystem(ftDriver, fsId),
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

    ck_assert_uint_eq(ftDriver->removeFileSystem(ftDriver, fsId),
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

    ck_assert_uint_eq(ftDriver->removeFileSystem(ftDriver, fsId),
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
    ck_assert_uint_eq(b->createFile(b, path), UA_STATUSCODE_GOOD);
    void *fc = NULL;
    ck_assert_uint_eq(b->openFile(b, path, UA_OPENFILEMODE_WRITE, &fc),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b->write(b, fc, UA_BYTESTRING((char*)(uintptr_t)content)),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b->closeFile(b, fc), UA_STATUSCODE_GOOD);
}

static UA_NodeId
mountNamedMem(UA_FileTransferBackend backend, const char *browseName,
              const UA_FileTransferMountOptions *options) {
    UA_NodeId fsId = UA_NODEID_NULL;
    ck_assert_uint_eq(
        ftDriver->addFileSystem(ftDriver, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                                UA_QUALIFIEDNAME(0, (char*)(uintptr_t)browseName),
                                backend, options, &fsId),
        UA_STATUSCODE_GOOD);
    return fsId;
}

/* Moving and copying between two mounts backed by different backends */
START_TEST(crossMountMoveCopy) {
    UA_FileTransferBackend bA;
    ck_assert_uint_eq(memBackend(&bA), UA_STATUSCODE_GOOD);
    writeMemFile(&bA, "doc.txt", "hello");
    writeMemFile(&bA, "move.txt", "world");
    ck_assert_uint_eq(bA.createDirectory(&bA, UA_STRING("d")), UA_STATUSCODE_GOOD);
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

    ck_assert_uint_eq(ftDriver->removeFileSystem(ftDriver, fsA), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ftDriver->removeFileSystem(ftDriver, fsB), UA_STATUSCODE_GOOD);
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

    UA_FileTransferMountOptions ro = {true, 0, 0, NULL, NULL};
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

    ck_assert_uint_eq(ftDriver->removeFileSystem(ftDriver, fsA), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ftDriver->removeFileSystem(ftDriver, fsB), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&aRo);
    UA_NodeId_clear(&copyId);
    UA_NodeId_clear(&fsA);
    UA_NodeId_clear(&fsB);
} END_TEST

START_TEST(dirRefresh) {
    /* Keep a second reference to the backend to make out-of-band changes.
     * The context is shared with the copy held by the mount. */
    UA_FileTransferBackend b = memBackendWithTree();
    UA_NodeId fsId = UA_NODEID_NULL;
    ck_assert_uint_eq(ftDriver->addFileSystem(
                          ftDriver, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                          UA_QUALIFIEDNAME(0, "FileSystem"), b, NULL, &fsId),
                      UA_STATUSCODE_GOOD);

    /* A new backend entry appears after refresh */
    ck_assert_uint_eq(b.createFile(&b, UA_STRING("new.txt")), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b.createDirectory(&b, UA_STRING("docs/newdir")),
                      UA_STATUSCODE_GOOD);
    ck_assert(!tryResolveChild(server_ft, fsId, "new.txt", NULL));
    ck_assert_uint_eq(ftDriver->refresh(ftDriver, fsId), UA_STATUSCODE_GOOD);
    ck_assert(tryResolveChild(server_ft, fsId, "new.txt", NULL));
    UA_NodeId docsId;
    ck_assert(tryResolveChild(server_ft, fsId, "docs", &docsId));
    ck_assert(tryResolveChild(server_ft, docsId, "newdir", NULL));

    /* A vanished backend entry disappears after refresh */
    ck_assert_uint_eq(b.remove(&b, UA_STRING("readme.txt")), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ftDriver->refresh(ftDriver, fsId), UA_STATUSCODE_GOOD);
    ck_assert(!tryResolveChild(server_ft, fsId, "readme.txt", NULL));

    /* A vanished file with an open handle: the node stays in the address
     * space (as a zombie) so the handle remains usable. The node is removed
     * when the last handle is closed. */
    UA_NodeId aId;
    ck_assert(tryResolveChild(server_ft, docsId, "a.txt", &aId));
    UA_UInt32 h = callOpen(aId, UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b.remove(&b, UA_STRING("docs/a.txt")),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ftDriver->refresh(ftDriver, fsId), UA_STATUSCODE_GOOD);
    ck_assert(tryResolveChild(server_ft, docsId, "a.txt", NULL));
    /* Zombie files cannot be opened again */
    callOpen(aId, UA_OPENFILEMODE_READ, UA_STATUSCODE_BADNOTFOUND);
    callClose(aId, h, UA_STATUSCODE_GOOD);
    ck_assert(!tryResolveChild(server_ft, docsId, "a.txt", NULL));

    /* Refresh is stable afterwards */
    ck_assert_uint_eq(ftDriver->refresh(ftDriver, fsId), UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(ftDriver->removeFileSystem(ftDriver, fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&docsId);
    UA_NodeId_clear(&aId);
    UA_NodeId_clear(&fsId);
} END_TEST

START_TEST(removeFileSystemWithOpenHandles) {
    UA_NodeId fsId = mountTree(NULL);

    UA_NodeId docsId, aId;
    ck_assert(tryResolveChild(server_ft, fsId, "docs", &docsId));
    ck_assert(tryResolveChild(server_ft, docsId, "a.txt", &aId));
    callOpen(aId, UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);

    /* The mount removal closes the handle and deletes the subtree */
    ck_assert_uint_eq(ftDriver->removeFileSystem(ftDriver, fsId),
                      UA_STATUSCODE_GOOD);
    UA_QualifiedName bn;
    ck_assert_uint_ne(UA_Server_readBrowseName(server_ft, aId, &bn),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ftDriver->removeFileSystem(ftDriver, fsId),
                      UA_STATUSCODE_BADNOTFOUND);

    UA_NodeId_clear(&docsId);
    UA_NodeId_clear(&aId);
    UA_NodeId_clear(&fsId);
} END_TEST

#ifndef _WIN32
/* The full directory workflow on the local filesystem backend */
START_TEST(localFilesystemMount) {
    makeScratchDir();

    /* Pre-create a small tree */
    UA_FileTransferBackend pre;
    ck_assert_uint_eq(UA_FileTransferBackend_localFilesystem(
                          UA_STRING(scratchDir), &pre), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pre.createDirectory(&pre, UA_STRING("logs")),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pre.createFile(&pre, UA_STRING("logs/log1.txt")),
                      UA_STATUSCODE_GOOD);
    pre.clear(&pre);

    UA_FileTransferBackend b;
    ck_assert_uint_eq(UA_FileTransferBackend_localFilesystem(
                          UA_STRING(scratchDir), &b), UA_STATUSCODE_GOOD);
    UA_NodeId fsId = UA_NODEID_NULL;
    ck_assert_uint_eq(ftDriver->addFileSystem(
                          ftDriver, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                          UA_QUALIFIEDNAME(0, "FileSystem"), b, NULL, &fsId),
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

    ck_assert_uint_eq(ftDriver->removeFileSystem(ftDriver, fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&logsId);
    UA_NodeId_clear(&fileId);
    UA_NodeId_clear(&fsId);
    removeTree(scratchDir);
} END_TEST
#endif

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

#endif /* UA_TEST_ENABLE_FILETRANSFER */

int main(void) {
    Suite *s = suite_create("server_filetransfer");

    TCase *tc_lifecycle = tcase_create("Driver Lifecycle");
#ifdef UA_TEST_ENABLE_FILETRANSFER
    tcase_add_test(tc_lifecycle, addDriver_rejectsDuplicateFileTransfer);
    tcase_add_test(tc_lifecycle, restartDriver);
    tcase_add_test(tc_lifecycle, instanceSharesTypeMethodNodes);
    tcase_add_test(tc_lifecycle, instanceHasMandatoryProperties);
#endif
    tcase_add_checked_fixture(tc_lifecycle, setup, teardown);
    suite_add_tcase(s, tc_lifecycle);

    TCase *tc_file = tcase_create("FileType Methods");
#ifdef UA_TEST_ENABLE_FILETRANSFER
    tcase_add_test(tc_file, fileProperties);
    tcase_add_test(tc_file, fileOpenModes);
    tcase_add_test(tc_file, fileLocking);
    tcase_add_test(tc_file, fileReadWrite);
    tcase_add_test(tc_file, fileBadHandles);
    tcase_add_test(tc_file, fileReadOnlyMount);
    tcase_add_test(tc_file, fileHandleLimits);
    tcase_add_test(tc_file, removeFileClosesHandles);
    tcase_add_test(tc_file, fileMaxByteStringLength);
# ifndef _WIN32
    tcase_add_test(tc_file, fileMimeType);
# endif
#endif
    tcase_add_checked_fixture(tc_file, setup, teardown);
    suite_add_tcase(s, tc_file);

    TCase *tc_dir = tcase_create("FileDirectoryType Methods");
#ifdef UA_TEST_ENABLE_FILETRANSFER
    tcase_add_test(tc_dir, mountScanMirrorsTree);
    tcase_add_test(tc_dir, mountScanDepthLimit);
    tcase_add_test(tc_dir, dirCreateMethods);
    tcase_add_test(tc_dir, dirReadOnlyMount);
    tcase_add_test(tc_dir, dirDelete);
    tcase_add_test(tc_dir, dirMoveOrCopy);
    tcase_add_test(tc_dir, crossMountMoveCopy);
    tcase_add_test(tc_dir, crossMountReadOnlySource);
    tcase_add_test(tc_dir, dirRefresh);
    tcase_add_test(tc_dir, removeFileSystemWithOpenHandles);
# ifndef _WIN32
    tcase_add_test(tc_dir, localFilesystemMount);
# endif
#endif
    tcase_add_checked_fixture(tc_dir, setup, teardown);
    suite_add_tcase(s, tc_dir);

    TCase *tc_backend = tcase_create("Storage Backends");
#ifdef UA_TEST_ENABLE_FILETRANSFER
    tcase_add_test(tc_backend, memoryBackendContract);
# ifndef _WIN32
    tcase_add_test(tc_backend, localFilesystemBackendContract);
    tcase_add_test(tc_backend, localFilesystemBackendSandbox);
# endif
#endif
    suite_add_tcase(s, tc_backend);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
