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
#include "ua_server_internal.h"

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef UA_ENABLE_DRIVER_FILE_TRANSFER
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
    MemBackendContext *ctx = (MemBackendContext*)b->file.context;
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
    MemBackendContext *ctx = (MemBackendContext*)b->file.context;
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
    out->createFile = memCreateFile;
    out->createDirectory = memCreateDirectory;
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
    UA_FileTransferFileBackend *f = &b->file;
    UA_StatusCode res;
    UA_UInt32 fc = 0;
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

    /* Rename/move */
    ck_assert_uint_eq(b->rename(b, UA_STRING("hello.txt"),
                                UA_STRING("sub/hello2.txt")),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(f->getInfo(f, UA_STRING("hello.txt"), &info),
                      UA_STATUSCODE_BADNOTFOUND);
    ck_assert_uint_eq(f->getInfo(f, UA_STRING("sub/hello2.txt"), &info),
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

START_TEST(localFilesystemBackendContract) {
    makeScratchDir();
    UA_FileTransferBackend b;
    ck_assert_uint_eq(UA_FileTransferBackend_localFilesystem(
                          UA_STRING(scratchDir), &b),
                      UA_STATUSCODE_GOOD);
    runBackendContract(&b);
    b.file.clear(&b.file);
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
    UA_UInt32 fc = 0;
    for(size_t i = 0; i < 8; i++) {
        UA_String p = UA_STRING((char*)(uintptr_t)badPaths[i]);
        ck_assert_uint_eq(b.file.open(&b.file, p, UA_OPENFILEMODE_READ, &fc),
                          UA_STATUSCODE_BADINVALIDARGUMENT);
        ck_assert_uint_eq(b.createFile(&b, p), UA_STATUSCODE_BADINVALIDARGUMENT);
        ck_assert_uint_eq(b.createDirectory(&b, p),
                          UA_STATUSCODE_BADINVALIDARGUMENT);
        ck_assert_uint_eq(b.remove(&b, p), UA_STATUSCODE_BADINVALIDARGUMENT);
    }

#ifdef _WIN32
    /* Names Windows cannot store as given */
    const char *windowsNames[9] = {"CON", "nul.txt", "com1", "LPT9.log", "a:b",
                                   "x?", "trail.", "trail ", "sub/AUX"};
    for(size_t i = 0; i < 9; i++) {
        UA_String p = UA_STRING((char*)(uintptr_t)windowsNames[i]);
        ck_assert_uint_eq(b.createFile(&b, p), UA_STATUSCODE_BADINVALIDARGUMENT);
    }
    ck_assert_uint_eq(b.createFile(&b, UA_STRING("CONSOLE.txt")),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b.createFile(&b, UA_STRING("COM10")), UA_STATUSCODE_GOOD);
#endif

    /* The backend requires an existing root directory */
    UA_FileTransferBackend b2;
    ck_assert_uint_eq(UA_FileTransferBackend_localFilesystem(
                          UA_STRING("/nonexistent-filetransfer-root"), &b2),
                      UA_STATUSCODE_BADNOTFOUND);

    b.file.clear(&b.file);
    removeTree(scratchDir);
} END_TEST

/* The names are UTF-8. On Windows, the backend stores them as UTF-16, so every
 * name is represented independent of the active code page. */
START_TEST(localFilesystemUtf8Names) {
    makeScratchDir();
    const char *name = "Gr\xc3\xb6\xc3\x9f" "e-\xe6\x97\xa5\xe6\x9c\xac.txt";
    UA_String path = UA_STRING((char*)(uintptr_t)name);
    UA_FileTransferBackend b;
    ck_assert_uint_eq(UA_FileTransferBackend_localFilesystem(
                          UA_STRING(scratchDir), &b),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b.createFile(&b, path), UA_STATUSCODE_GOOD);
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
        UA_UInt32 fc = 0;
        ck_assert_uint_eq(b.file.open(&b.file, path, UA_OPENFILEMODE_WRITE, &fc),
                          UA_STATUSCODE_GOOD);
        UA_ByteString data = UA_BYTESTRING((char*)(uintptr_t)content);
        ck_assert_uint_eq(b.file.write(&b.file, fc, data), UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(b.file.close(&b.file, fc), UA_STATUSCODE_GOOD);
    }
    return b;
}

/* Helper: add a standalone file backed by a fresh memory backend */
static UA_NodeId
addTestFile(const char *browseName, const char *content,
            const UA_FileTransferMountOptions *options) {
    UA_FileTransferBackend b = memBackendWithFile("f.bin", content);
    UA_NodeId fileNodeId = UA_NODEID_NULL;
    UA_StatusCode res = UA_FileTransferDriver_addFile(
        ftDriver, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
        UA_QUALIFIEDNAME(0, (char *)(uintptr_t)browseName), &b.file,
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

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fileId),
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
    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fileId),
                      UA_STATUSCODE_GOOD);
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
    ck_assert_uint_eq(UA_FileTransferDriver_addFile(
                          driver, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                          UA_QUALIFIEDNAME(0, "CustomFile"), &b.file,
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

/* MimeType is inferred from the extension by the local filesystem backend */
START_TEST(fileMimeType) {
    makeScratchDir();

    UA_FileTransferBackend pre;
    ck_assert_uint_eq(UA_FileTransferBackend_localFilesystem(
                          UA_STRING(scratchDir), &pre), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pre.createFile(&pre, UA_STRING("a.txt")), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pre.createFile(&pre, UA_STRING("b.json")), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pre.createFile(&pre, UA_STRING("c")), UA_STATUSCODE_GOOD);
    pre.file.clear(&pre.file);

    UA_FileTransferBackend b;
    ck_assert_uint_eq(UA_FileTransferBackend_localFilesystem(
                          UA_STRING(scratchDir), &b), UA_STATUSCODE_GOOD);
    UA_NodeId fsId = UA_NODEID_NULL;
    ck_assert_uint_eq(UA_FileTransferDriver_addFileSystem(
                          ftDriver, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
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

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsId),
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

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fileId),
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

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fileId),
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

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fileId),
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

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fileA),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fileB),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileA);
    UA_NodeId_clear(&fileB);
} END_TEST

START_TEST(fileReadOnlyMount) {
    UA_FileTransferMountOptions options;
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

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fileId),
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

    UA_FileTransferDriver *driver = UA_FileTransferDriver_new(params);
    UA_KeyValueMap_clear(&params);
    ck_assert_ptr_nonnull(driver);
    ck_assert_uint_eq(UA_Server_addDriver(server, &driver->drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(driver->drv.start(&driver->drv), UA_STATUSCODE_GOOD);

    UA_NodeId fileA = UA_NODEID_NULL;
    UA_NodeId fileB = UA_NODEID_NULL;
    UA_FileTransferBackend bA = memBackendWithFile("a.bin", "aaa");
    UA_FileTransferBackend bB = memBackendWithFile("b.bin", "bbb");
    ck_assert_uint_eq(UA_FileTransferDriver_addFile(
                          driver, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                          UA_QUALIFIEDNAME(0, "LimitFileA"), &bA.file,
                          UA_STRING("a.bin"), NULL, &fileA),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_FileTransferDriver_addFile(
                          driver, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                          UA_QUALIFIEDNAME(0, "LimitFileB"), &bB.file,
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
    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fileId),
                      UA_STATUSCODE_GOOD);

    /* The object is gone from the address space */
    UA_QualifiedName browseName;
    ck_assert_uint_ne(UA_Server_readBrowseName(server_ft, fileId, &browseName),
                      UA_STATUSCODE_GOOD);

    /* Removing again fails */
    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fileId),
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
mountTree(const UA_FileTransferMountOptions *options) {
    UA_NodeId fsId = UA_NODEID_NULL;
    UA_StatusCode res = UA_FileTransferDriver_addFileSystem(
        ftDriver, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
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

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsId),
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
    UA_FileTransferMountOptions options;
    memset(&options, 0, sizeof(options));
    options.maxScanDepth = 1;
    UA_NodeId fsId = mountTree(&options);

    /* Only the first level is mirrored */
    UA_NodeId docsId;
    ck_assert(tryResolveChild(server_ft, fsId, "readme.txt", NULL));
    ck_assert(tryResolveChild(server_ft, fsId, "docs", &docsId));
    ck_assert(!tryResolveChild(server_ft, docsId, "a.txt", NULL));

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsId),
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

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&newDirId);
    UA_NodeId_clear(&newFileId);
    UA_NodeId_clear(&openFileId);
    UA_NodeId_clear(&fsId);
} END_TEST

START_TEST(dirReadOnlyMount) {
    UA_FileTransferMountOptions options;
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

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsId),
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

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsId),
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

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsId),
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
              const UA_FileTransferMountOptions *options) {
    UA_NodeId fsId = UA_NODEID_NULL;
    ck_assert_uint_eq(UA_FileTransferDriver_addFileSystem(
                          ftDriver, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
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

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsA),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsB),
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

    UA_FileTransferMountOptions ro;
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

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsA),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsB),
                      UA_STATUSCODE_GOOD);
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
    ck_assert_uint_eq(UA_FileTransferDriver_addFileSystem(
                          ftDriver, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                          UA_QUALIFIEDNAME(0, "FileSystem"), &b, NULL, &fsId),
                      UA_STATUSCODE_GOOD);

    /* A new backend entry appears after refresh */
    ck_assert_uint_eq(b.createFile(&b, UA_STRING("new.txt")), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b.createDirectory(&b, UA_STRING("docs/newdir")),
                      UA_STATUSCODE_GOOD);
    ck_assert(!tryResolveChild(server_ft, fsId, "new.txt", NULL));
    ck_assert_uint_eq(UA_FileTransferDriver_refresh(ftDriver, fsId),
                      UA_STATUSCODE_GOOD);
    ck_assert(tryResolveChild(server_ft, fsId, "new.txt", NULL));
    UA_NodeId docsId;
    ck_assert(tryResolveChild(server_ft, fsId, "docs", &docsId));
    ck_assert(tryResolveChild(server_ft, docsId, "newdir", NULL));

    /* A vanished backend entry disappears after refresh */
    ck_assert_uint_eq(b.remove(&b, UA_STRING("readme.txt")), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_FileTransferDriver_refresh(ftDriver, fsId),
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
    ck_assert_uint_eq(UA_FileTransferDriver_refresh(ftDriver, fsId),
                      UA_STATUSCODE_GOOD);
    ck_assert(tryResolveChild(server_ft, docsId, "a.txt", NULL));
    /* Zombie files cannot be opened again */
    callOpen(aId, UA_OPENFILEMODE_READ, UA_STATUSCODE_BADNOTFOUND);
    callClose(aId, h, UA_STATUSCODE_GOOD);
    ck_assert(!tryResolveChild(server_ft, docsId, "a.txt", NULL));

    /* Refresh is stable afterwards */
    ck_assert_uint_eq(UA_FileTransferDriver_refresh(ftDriver, fsId),
                      UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsId),
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
    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsId),
                      UA_STATUSCODE_GOOD);
    UA_QualifiedName bn;
    ck_assert_uint_ne(UA_Server_readBrowseName(server_ft, aId, &bn),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsId),
                      UA_STATUSCODE_BADNOTFOUND);

    UA_NodeId_clear(&docsId);
    UA_NodeId_clear(&aId);
    UA_NodeId_clear(&fsId);
} END_TEST

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
    pre.file.clear(&pre.file);

    UA_FileTransferBackend b;
    ck_assert_uint_eq(UA_FileTransferBackend_localFilesystem(
                          UA_STRING(scratchDir), &b), UA_STATUSCODE_GOOD);
    UA_NodeId fsId = UA_NODEID_NULL;
    ck_assert_uint_eq(UA_FileTransferDriver_addFileSystem(
                          ftDriver, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
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

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&logsId);
    UA_NodeId_clear(&fileId);
    UA_NodeId_clear(&fsId);
    removeTree(scratchDir);
} END_TEST

/* Sizes and positions beyond 2 GiB. The file is sparse, it takes no space. */
START_TEST(localFilesystemLargeFile) {
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
    ck_assert_uint_eq(UA_FileTransferBackend_localFilesystem(
                          UA_STRING(scratchDir), &b), UA_STATUSCODE_GOOD);
    UA_NodeId fsId = UA_NODEID_NULL;
    ck_assert_uint_eq(UA_FileTransferDriver_addFileSystem(
                          ftDriver, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
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

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsId),
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
    UA_MethodCallback cb = NULL;
    ck_assert_uint_eq(UA_Server_getMethodNodeCallback(
                          server_ft, UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_OPEN),
                          &cb), UA_STATUSCODE_GOOD);
    ck_assert(cb != NULL);

    ftDriver->drv.stop(&ftDriver->drv);
    cb = NULL;
    ck_assert_uint_eq(UA_Server_getMethodNodeCallback(
                          server_ft, UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_OPEN),
                          &cb), UA_STATUSCODE_GOOD);
    ck_assert(cb == NULL);

    /* Restarting claims them again, so the fixture teardown is unaffected */
    ck_assert_uint_eq(ftDriver->drv.start(&ftDriver->drv), UA_STATUSCODE_GOOD);
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

    UA_FileTransferDriver *drv = UA_FileTransferDriver_new(UA_KEYVALUEMAP_NULL);
    ck_assert_ptr_nonnull(drv);
    ck_assert_uint_eq(UA_Server_addDriver(s, &drv->drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(drv->drv.start(&drv->drv), UA_STATUSCODE_GOOD);

    UA_NodeId fileId = UA_NODEID_NULL;
    UA_FileTransferBackend b = memBackendWithFile("f.bin", "data");
    ck_assert_uint_eq(
        UA_FileTransferDriver_addFile(drv, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                                      UA_QUALIFIEDNAME(0, "OptFile"), &b.file,
                                      UA_STRING("f.bin"), NULL, &fileId),
        UA_STATUSCODE_GOOD);

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

    drv->drv.stop(&drv->drv);
    ck_assert_uint_eq(UA_Server_removeDriver(s, &drv->drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(drv->drv.free(&drv->drv), UA_STATUSCODE_GOOD);
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
    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fileId),
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
    outInfo->size = sizeof(minimalContent) - 1;
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

/* A read-only standalone file never reaches write, listDirectory, createFile,
 * createDirectory, remove or rename, so the backend need not supply stubs for
 * them. A writable mount still has to provide the mutating operations. */
START_TEST(readOnlyBackendNeedsNoWriteCallbacks) {
    UA_FileTransferMountOptions options;
    memset(&options, 0, sizeof(options));
    options.readOnly = true;

    UA_NodeId fileId = UA_NODEID_NULL;
    ck_assert_uint_eq(UA_FileTransferDriver_addFile(
                          ftDriver, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
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
    ck_assert_uint_eq(UA_FileTransferDriver_addFile(
                          ftDriver, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                          UA_QUALIFIEDNAME(0, "WritableFile"), minimalBackend(),
                          UA_STRING("static"), NULL, &rejected),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert(UA_NodeId_isNull(&rejected));

    /* ... and for a read-only directory mount, which still needs a listing */
    UA_FileTransferBackend dirBackend;
    memset(&dirBackend, 0, sizeof(dirBackend));
    dirBackend.file = *minimalBackend();
    UA_NodeId rejectedFs = UA_NODEID_NULL;
    ck_assert_uint_eq(UA_FileTransferDriver_addFileSystem(
                          ftDriver, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                          UA_QUALIFIEDNAME(0, "FileSystem"), &dirBackend, &options,
                          &rejectedFs),
                      UA_STATUSCODE_BADINVALIDARGUMENT);

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fileId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileId);
} END_TEST

/* The maxNodes ceiling has to hold for Objects created through the Methods as
 * well, otherwise a client grows the address space past the configured limit
 * one CreateFile call at a time. */
START_TEST(dirCreateRespectsMaxNodes) {
    UA_FileTransferMountOptions options;
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

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsId),
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

    UA_FileTransferMountOptions options;
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

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsId),
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
    ck_assert_uint_eq(b.createDirectory(&b, UA_STRING("readable")),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b.createFile(&b, UA_STRING("readable/visible.txt")),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b.createDirectory(&b, UA_STRING("locked")),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b.createFile(&b, UA_STRING("locked/hidden.txt")),
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
    ck_assert_uint_eq(UA_FileTransferDriver_addFileSystem(
                          ftDriver, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
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
    ck_assert_uint_eq(UA_FileTransferDriver_refresh(ftDriver, fsId),
                      UA_STATUSCODE_GOOD);
    ck_assert(tryResolveChild(server_ft, fsId, "locked", NULL));
    ck_assert(tryResolveChild(server_ft, readableId, "visible.txt", NULL));

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&readableId);
    UA_NodeId_clear(&lockedId);
    UA_NodeId_clear(&fsId);
} END_TEST

/* rename(2) replaces an existing target silently. An entry created behind the
 * driver's back must not be destroyed by a MoveOrCopy onto its name. */
START_TEST(moveOrCopyKeepsUnmirroredTarget) {
    makeScratchDir();

    UA_FileTransferBackend pre;
    ck_assert_uint_eq(UA_FileTransferBackend_localFilesystem(
                          UA_STRING(scratchDir), &pre), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pre.createFile(&pre, UA_STRING("source.txt")),
                      UA_STATUSCODE_GOOD);
    pre.file.clear(&pre.file);

    UA_FileTransferBackend b;
    ck_assert_uint_eq(UA_FileTransferBackend_localFilesystem(
                          UA_STRING(scratchDir), &b), UA_STATUSCODE_GOOD);
    UA_NodeId fsId = UA_NODEID_NULL;
    ck_assert_uint_eq(UA_FileTransferDriver_addFileSystem(
                          ftDriver, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
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

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&sourceId);
    UA_NodeId_clear(&fsId);
    removeTree(scratchDir);
} END_TEST

/* Failure injection for one backend path */
static char failingPath[MEM_MAXPATH];
static UA_StatusCode (*memRemoveFn)(UA_FileTransferBackend *b, const UA_String path);
static UA_StatusCode (*memCreateFileFn)(UA_FileTransferBackend *b, const UA_String path);
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
failingCreateFile(UA_FileTransferBackend *b, const UA_String path) {
    return isFailingPath(path) ? UA_STATUSCODE_BADUSERACCESSDENIED :
        memCreateFileFn(b, path);
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
    ck_assert_uint_eq(bA.createDirectory(&bA, UA_STRING("d")), UA_STATUSCODE_GOOD);
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

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsA),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsB),
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
    ck_assert_uint_eq(bA.createDirectory(&bA, UA_STRING("d")), UA_STATUSCODE_GOOD);
    writeMemFile(&bA, "d/one.txt", "1");
    writeMemFile(&bA, "d/two.txt", "2");
    UA_FileTransferBackend bB;
    ck_assert_uint_eq(memBackend(&bB), UA_STATUSCODE_GOOD);
    memCreateFileFn = bB.createFile;
    bB.createFile = failingCreateFile;
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

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsA),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsB),
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

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileId);
    UA_NodeId_clear(&fsId);
} END_TEST

/* A Delete that fails partway removes the Objects of the entries that were
 * deleted from the backend. The remaining entries keep their Objects. */
START_TEST(partialDeleteRemovesDeletedObjects) {
    UA_FileTransferBackend b;
    ck_assert_uint_eq(memBackend(&b), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b.createDirectory(&b, UA_STRING("d")), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b.createDirectory(&b, UA_STRING("d/sub")), UA_STATUSCODE_GOOD);
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

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsId),
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
    ck_assert_uint_eq(b.createDirectory(&b, UA_STRING("d")), UA_STATUSCODE_GOOD);
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

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsId),
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

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileId);
    UA_NodeId_clear(&fsId);
} END_TEST

#ifndef _WIN32
/* The local filesystem backend serves a symbolic link to a file as the file.
 * Links to directories and special files are not served. Deleting never
 * descends into a link target: a directory with an entry that is not served is
 * not empty for the driver, a served link is removed itself. */
START_TEST(localFilesystemSkipsLinksAndSpecialFiles) {
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
    ck_assert_uint_eq(UA_FileTransferBackend_localFilesystem(UA_STRING(root), &b),
                      UA_STATUSCODE_GOOD);
    UA_NodeId fsId = UA_NODEID_NULL;
    ck_assert_uint_eq(UA_FileTransferDriver_addFileSystem(
                          ftDriver, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
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

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsId),
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
    ftDriver->drv.stop(&ftDriver->drv);
    UA_NodeId fileId = addTestFile("EarlyFile", "early", NULL);
    ck_assert_uint_eq(ftDriver->drv.start(&ftDriver->drv), UA_STATUSCODE_GOOD);
    UA_ByteString data = readFileContent(fileId);
    ck_assert_uint_eq(data.length, strlen("early"));
    UA_ByteString_clear(&data);
    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fileId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileId);
} END_TEST

/* A server has only one file transfer driver. Stopping and freeing a rejected
 * driver leaves the Namespace Zero Method nodes with the running driver. */
START_TEST(addDriverRejectsSecondFileTransfer) {
    UA_FileTransferDriver *second = UA_FileTransferDriver_new(UA_KEYVALUEMAP_NULL);
    ck_assert_ptr_nonnull(second);
    ck_assert_uint_eq(UA_Server_addDriver(server_ft, &second->drv),
                      UA_STATUSCODE_BADALREADYEXISTS);
    second->drv.stop(&second->drv);
    ck_assert_uint_eq(second->drv.free(&second->drv), UA_STATUSCODE_GOOD);

    /* The running driver still serves its files */
    UA_MethodCallback cb = NULL;
    ck_assert_uint_eq(UA_Server_getMethodNodeCallback(
                          server_ft, UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_OPEN),
                          &cb), UA_STATUSCODE_GOOD);
    ck_assert(cb != NULL);
    UA_NodeId fileId = addTestFile("StillWorks", "content", NULL);
    UA_ByteString data = readFileContent(fileId);
    ck_assert_uint_eq(data.length, strlen("content"));
    UA_ByteString_clear(&data);
    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fileId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileId);
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

    UA_FileTransferMountOptions options;
    memset(&options, 0, sizeof(options));
    options.namespaceIndex = (UA_UInt16)nsSize; /* one past the last index */

    UA_NodeId fsId = UA_NODEID_NULL;
    ck_assert_uint_eq(UA_FileTransferDriver_addFileSystem(
                          ftDriver, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
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
    UA_NodeId sizeId = resolveChild(server_ft, aId, "Size");

    /* A second parent for a file and for a Property */
    UA_ExpandedNodeId target = UA_EXPANDEDNODEID_NULL;
    target.nodeId = readmeId;
    ck_assert_uint_eq(UA_Server_addReference(server_ft, UA_NS0ID(OBJECTSFOLDER),
                                             UA_NS0ID(ORGANIZES), target, true),
                      UA_STATUSCODE_GOOD);
    target.nodeId = sizeId;
    ck_assert_uint_eq(UA_Server_addReference(server_ft, UA_NS0ID(OBJECTSFOLDER),
                                             UA_NS0ID(ORGANIZES), target, true),
                      UA_STATUSCODE_GOOD);

    /* Deleting docs removes a.txt. Its Size Property keeps a value. */
    callDelete(fsId, docsId, UA_STATUSCODE_GOOD);
    UA_NodeClass nodeClass;
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ft, aId, &nodeClass),
                      UA_STATUSCODE_BADNODEIDUNKNOWN);
    UA_Variant value;
    ck_assert_uint_eq(UA_Server_readValue(server_ft, sizeId, &value),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT64]));
    UA_Variant_clear(&value);

    /* Removing the mount deletes the file with the second parent */
    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsId),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ft, readmeId, &nodeClass),
                      UA_STATUSCODE_BADNODEIDUNKNOWN);

    UA_NodeId_clear(&readmeId);
    UA_NodeId_clear(&docsId);
    UA_NodeId_clear(&aId);
    UA_NodeId_clear(&sizeId);
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
    ck_assert_uint_eq(bA.createDirectory(&bA, UA_STRING("d")), UA_STATUSCODE_GOOD);
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

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsA),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsB),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&aFile);
    UA_NodeId_clear(&aDir);
    UA_NodeId_clear(&fsA);
    UA_NodeId_clear(&fsB);
} END_TEST

/* With copyMethodsOnInstances the Objects get their own Method copies. The
 * copies made while the driver is stopped are served once it is started. */
START_TEST(copiedMethodsServedAfterStart) {
    ftDriver->drv.stop(&ftDriver->drv);
    UA_ServerConfig *config = UA_Server_getConfig(server_ft);
    config->copyMethodsOnInstances = true;
    UA_NodeId fileId = addTestFile("CopiedFile", "copied", NULL);
    UA_NodeId fsId = mountTree(NULL);
    config->copyMethodsOnInstances = false;
    ck_assert_uint_eq(ftDriver->drv.start(&ftDriver->drv), UA_STATUSCODE_GOOD);

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

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fileId),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsId),
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
        outInfo->writable = false;
    return res;
}

static UA_Boolean
denyUserWrite(UA_Server *server, const UA_NodeId *sessionId,
              const UA_NodeId *fileNodeId, void *mountContext) {
    return false;
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
    ck_assert_uint_eq(UA_FileTransferDriver_addFile(
                          ftDriver, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
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
    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fileId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileId);

    UA_FileTransferMountOptions options;
    memset(&options, 0, sizeof(options));
    options.getUserWritable = denyUserWrite;
    fileId = addTestFile("UserFile", "data", &options);
    ck_assert(readBooleanProperty(fileId, "Writable"));
    ck_assert(!readBooleanProperty(fileId, "UserWritable"));
    callOpen(fileId, UA_OPENFILEMODE_WRITE, UA_STATUSCODE_BADNOTWRITABLE);
    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fileId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileId);
} END_TEST

/* maxNodes counts the FileSystem root */
START_TEST(maxNodesCountsRoot) {
    UA_FileTransferMountOptions options;
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

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsId),
                      UA_STATUSCODE_GOOD);
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
    ck_assert_uint_eq(b.createFile(&b, UA_STRING("new.txt")), UA_STATUSCODE_GOOD);

    failListing = true;
    ck_assert_uint_eq(UA_FileTransferDriver_refresh(ftDriver, fsId),
                      UA_STATUSCODE_BADUNEXPECTEDERROR);
    ck_assert(tryResolveChild(server_ft, fsId, "readme.txt", NULL));
    ck_assert(!tryResolveChild(server_ft, fsId, "new.txt", NULL));
    UA_NodeId docsId = resolveChild(server_ft, fsId, "docs");
    callDelete(fsId, docsId, UA_STATUSCODE_BADUNEXPECTEDERROR);
    ck_assert(tryResolveChild(server_ft, fsId, "docs", NULL));
    failListing = false;

    ck_assert_uint_eq(UA_FileTransferDriver_refresh(ftDriver, fsId),
                      UA_STATUSCODE_GOOD);
    ck_assert(!tryResolveChild(server_ft, fsId, "readme.txt", NULL));
    ck_assert(tryResolveChild(server_ft, fsId, "new.txt", NULL));

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsId),
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
    ck_assert_uint_eq(b.createDirectory(&b, UA_STRING("a")), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b.createDirectory(&b, UA_STRING("b")), UA_STATUSCODE_GOOD);
    writeMemFile(&b, "a/one", "1");
    writeMemFile(&b, "b/two", "2");
    UA_FileTransferMountOptions options;
    memset(&options, 0, sizeof(options));
    options.maxNodes = 5; /* The root, two directories and two files */
    UA_NodeId fsId = mountNamedMem(b, "FileSystem", &options);
    UA_NodeId aId = resolveChild(server_ft, fsId, "a");
    UA_NodeId bId = resolveChild(server_ft, fsId, "b");

    ck_assert_uint_eq(b.remove(&b, UA_STRING("b/two")), UA_STATUSCODE_GOOD);
    writeMemFile(&b, "a/two", "2");
    ck_assert_uint_eq(UA_FileTransferDriver_refresh(ftDriver, fsId),
                      UA_STATUSCODE_GOOD);
    ck_assert(tryResolveChild(server_ft, aId, "two", NULL));
    ck_assert(!tryResolveChild(server_ft, bId, "two", NULL));

    ck_assert_uint_eq(b.remove(&b, UA_STRING("a/two")), UA_STATUSCODE_GOOD);
    writeMemFile(&b, "b/two", "2");
    ck_assert_uint_eq(UA_FileTransferDriver_refresh(ftDriver, fsId),
                      UA_STATUSCODE_GOOD);
    ck_assert(tryResolveChild(server_ft, bId, "two", NULL));
    ck_assert(!tryResolveChild(server_ft, aId, "two", NULL));

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsId),
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
    ck_assert_uint_eq(b.createDirectory(&b, UA_STRING("d")), UA_STATUSCODE_GOOD);
    writeMemFile(&b, "d/f", "open");
    UA_NodeId fsId = mountNamedMem(b, "FileSystem", NULL);
    UA_NodeId dirId = resolveChild(server_ft, fsId, "d");
    UA_NodeId fId = resolveChild(server_ft, dirId, "f");
    UA_UInt32 h = callOpen(fId, UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(b.remove(&b, UA_STRING("d/f")), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(b.remove(&b, UA_STRING("d")), UA_STATUSCODE_GOOD);
    writeMemFile(&b, "d", "file");
    ck_assert_uint_eq(UA_FileTransferDriver_refresh(ftDriver, fsId),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_FileTransferDriver_refresh(ftDriver, fsId),
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
    ck_assert_uint_eq(UA_FileTransferDriver_refresh(ftDriver, fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeClass nodeClass;
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ft, dirId, &nodeClass),
                      UA_STATUSCODE_BADNODEIDUNKNOWN);

    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fsId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&dirId);
    UA_NodeId_clear(&fId);
    UA_NodeId_clear(&fsId);
} END_TEST

/* An existing Object with its own instances of the FileType Methods is served
 * by the driver. The Methods work on the handles of the driver and the Object
 * is kept when it is detached. */
START_TEST(attachExistingFile) {
    UA_ServerConfig *config = UA_Server_getConfig(server_ft);
    config->copyMethodsOnInstances = true;
    UA_NodeId fileId = addFileTypeInstance(server_ft, "AttachedFile");
    config->copyMethodsOnInstances = false;
    UA_NodeId openId = resolveChild(server_ft, fileId, "Open");
    UA_NodeId typeOpenId = UA_NS0ID(FILETYPE_OPEN);
    ck_assert(!UA_NodeId_equal(&openId, &typeOpenId));
    UA_NodeId_clear(&openId);

    ck_assert_uint_eq(
        UA_FileTransferDriver_attachFile(
            ftDriver, fileId,
            &backendArg(memBackendWithFile("f.bin", "attached"))->file,
            UA_STRING("f.bin"), NULL),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_FileTransferDriver_attachFile(
                          ftDriver, fileId,
                          &backendArg(memBackendWithFile("f.bin", NULL))->file,
                          UA_STRING("f.bin"), NULL),
                      UA_STATUSCODE_BADNODEIDEXISTS);
    ck_assert_uint_eq(UA_FileTransferDriver_remove(ftDriver, fileId),
                      UA_STATUSCODE_BADNOTFOUND);

    /* Open and read with the Methods of the Object */
    UA_Byte mode = UA_OPENFILEMODE_READ | UA_OPENFILEMODE_WRITE;
    UA_Variant input[2];
    UA_Variant_setScalar(&input[0], &mode, &UA_TYPES[UA_TYPES_BYTE]);
    UA_CallMethodResult result = callObjectMethod(fileId, "Open", 1, input);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_GOOD);
    UA_UInt32 handle = *(UA_UInt32*)result.outputArguments[0].data;
    UA_CallMethodResult_clear(&result);
    ck_assert_uint_eq(readOpenCount(fileId), 1);

    UA_Int32 length = 100;
    UA_Variant_setScalar(&input[0], &handle, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&input[1], &length, &UA_TYPES[UA_TYPES_INT32]);
    result = callObjectMethod(fileId, "Read", 2, input);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_GOOD);
    UA_ByteString expected = UA_BYTESTRING("attached");
    ck_assert(UA_ByteString_equal((UA_ByteString*)result.outputArguments[0].data,
                                  &expected));
    UA_CallMethodResult_clear(&result);

    /* The handle is available to the application and can be closed */
    const UA_NodeId *sessionId = &server_ft->adminSession.sessionId;
    UA_Byte handleMode = 0;
    UA_UInt32 backendHandle = 0;
    ck_assert_uint_eq(
        UA_FileTransferDriver_getHandleInfo(ftDriver, fileId, sessionId, handle,
                                            &handleMode, &backendHandle),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(handleMode, mode);
    ck_assert_uint_ne(backendHandle, 0);
    ck_assert_uint_eq(
        UA_FileTransferDriver_getHandleInfo(ftDriver, UA_NS0ID(OBJECTSFOLDER),
                                            sessionId, handle, NULL, NULL),
        UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(UA_FileTransferDriver_closeHandle(ftDriver, sessionId, handle),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(readOpenCount(fileId), 0);
    ck_assert_uint_eq(UA_FileTransferDriver_getHandleInfo(
                          ftDriver, fileId, sessionId, handle, NULL, NULL),
                      UA_STATUSCODE_BADINVALIDARGUMENT);

    /* Detaching closes the handles and keeps the Object */
    UA_Variant_setScalar(&input[0], &mode, &UA_TYPES[UA_TYPES_BYTE]);
    result = callObjectMethod(fileId, "Open", 1, input);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&result);
    ck_assert_uint_eq(UA_FileTransferDriver_detachFile(ftDriver, fileId),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_FileTransferDriver_detachFile(ftDriver, fileId),
                      UA_STATUSCODE_BADNOTFOUND);
    UA_NodeClass nodeClass;
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ft, fileId, &nodeClass),
                      UA_STATUSCODE_GOOD);
    UA_Variant value;
    readProperty(fileId, "Size", &value);
    UA_Variant_clear(&value);
    result = callObjectMethod(fileId, "Open", 1, input);
    ck_assert_uint_ne(result.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&result);

    /* Attach again, the driver releases the Object when it is freed */
    ck_assert_uint_eq(UA_FileTransferDriver_attachFile(
                          ftDriver, fileId,
                          &backendArg(memBackendWithFile("f.bin", "again"))->file,
                          UA_STRING("f.bin"), NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_FileTransferDriver_attachFile(
                          ftDriver, UA_NODEID_NUMERIC(1, 999999),
                          &backendArg(memBackendWithFile("f.bin", NULL))->file,
                          UA_STRING("f.bin"), NULL),
                      UA_STATUSCODE_BADNODEIDUNKNOWN);
    UA_NodeId_clear(&fileId);
} END_TEST

/* The instance declarations of a FileType subtype are shared by its
 * instances. Detaching one instance keeps them for the others. */
START_TEST(detachKeepsSharedSubtypeMethods) {
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
    const char *names[2] = {"SubtypeA", "SubtypeB"};
    for(size_t i = 0; i < 2; i++) {
        ck_assert_uint_eq(UA_Server_addObjectNode(server_ft, UA_NODEID_NULL,
                              UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(ORGANIZES),
                              UA_QUALIFIEDNAME(1, (char*)(uintptr_t)names[i]),
                              typeId, UA_ObjectAttributes_default, NULL,
                              &files[i]), UA_STATUSCODE_GOOD);
        UA_NodeId instanceOpenId = resolveChild(server_ft, files[i], "Open");
        ck_assert(UA_NodeId_equal(&instanceOpenId, &openId));
        UA_NodeId_clear(&instanceOpenId);
        ck_assert_uint_eq(
            UA_FileTransferDriver_attachFile(
                ftDriver, files[i],
                &backendArg(memBackendWithFile("f.bin", "data"))->file,
                UA_STRING("f.bin"), NULL),
            UA_STATUSCODE_GOOD);
    }

    UA_Byte mode = UA_OPENFILEMODE_READ;
    UA_Variant input;
    UA_Variant_setScalar(&input, &mode, &UA_TYPES[UA_TYPES_BYTE]);
    ck_assert_uint_eq(UA_FileTransferDriver_detachFile(ftDriver, files[0]),
                      UA_STATUSCODE_GOOD);
    UA_CallMethodResult result = callObjectMethod(files[1], "Open", 1, &input);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&result);

    /* The last user releases the Method */
    ck_assert_uint_eq(UA_FileTransferDriver_detachFile(ftDriver, files[1]),
                      UA_STATUSCODE_GOOD);
    result = callObjectMethod(files[1], "Open", 1, &input);
    ck_assert_uint_ne(result.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&result);
    UA_NodeId_clear(&files[0]);
    UA_NodeId_clear(&files[1]);
} END_TEST

#endif /* UA_TEST_ENABLE_FILETRANSFER */

int main(void) {
    Suite *s = suite_create("server_filetransfer");

    TCase *tc_lifecycle = tcase_create("Driver Lifecycle");
#ifdef UA_TEST_ENABLE_FILETRANSFER
    tcase_add_test(tc_lifecycle, addDriverRejectsSecondFileTransfer);
    tcase_add_test(tc_lifecycle, restartDriver);
    tcase_add_test(tc_lifecycle, mountBeforeStart);
    tcase_add_test(tc_lifecycle, instanceSharesTypeMethodNodes);
    tcase_add_test(tc_lifecycle, instanceHasMandatoryProperties);
    tcase_add_test(tc_lifecycle, stopReleasesTypeMethodCallbacks);
    tcase_add_test(tc_lifecycle, optionalPropertiesAreNotDuplicated);
    tcase_add_test(tc_lifecycle, copiedMethodsServedAfterStart);
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
    tcase_add_test(tc_file, fileWriteRespectsMaxByteStringLength);
    tcase_add_test(tc_file, readOnlyBackendNeedsNoWriteCallbacks);
    tcase_add_test(tc_file, userWritableFollowsStorage);
    tcase_add_test(tc_file, attachExistingFile);
    tcase_add_test(tc_file, detachKeepsSharedSubtypeMethods);
    tcase_add_test(tc_file, fileMimeType);
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
    tcase_add_test(tc_dir, crossMountMoveKeepsDataOnFailedDelete);
    tcase_add_test(tc_dir, failedDirectoryCopyLeavesNothing);
    tcase_add_test(tc_dir, createFileRemovedOnFailedOpen);
    tcase_add_test(tc_dir, createFileRemovedOnFailedMirror);
    tcase_add_test(tc_dir, partialDeleteRemovesDeletedObjects);
    tcase_add_test(tc_dir, moveOfUnlistableDirectorySucceeds);
    tcase_add_test(tc_dir, dirRefresh);
    tcase_add_test(tc_dir, removeFileSystemWithOpenHandles);
    tcase_add_test(tc_dir, dirCreateRespectsMaxNodes);
    tcase_add_test(tc_dir, mirroredNamesUseMountNamespace);
    tcase_add_test(tc_dir, mountRejectsUnknownNamespace);
    tcase_add_test(tc_dir, mountSkipsUnreadableEntries);
    tcase_add_test(tc_dir, removalReleasesValueSources);
    tcase_add_test(tc_dir, copyFailsOnDestinationClose);
    tcase_add_test(tc_dir, maxNodesCountsRoot);
    tcase_add_test(tc_dir, failedListingKeepsTree);
    tcase_add_test(tc_dir, maxNodesReusedAcrossDirectories);
    tcase_add_test(tc_dir, replacingEntryIsNotZombie);
    tcase_add_test(tc_dir, localFilesystemMount);
    tcase_add_test(tc_dir, localFilesystemLargeFile);
    tcase_add_test(tc_dir, moveOrCopyKeepsUnmirroredTarget);
# ifndef _WIN32
    tcase_add_test(tc_dir, localFilesystemSkipsLinksAndSpecialFiles);
# endif
#endif
    tcase_add_checked_fixture(tc_dir, setup, teardown);
    suite_add_tcase(s, tc_dir);

    TCase *tc_backend = tcase_create("Storage Backends");
#ifdef UA_TEST_ENABLE_FILETRANSFER
    tcase_add_test(tc_backend, memoryBackendContract);
    tcase_add_test(tc_backend, localFilesystemBackendContract);
    tcase_add_test(tc_backend, localFilesystemBackendSandbox);
    tcase_add_test(tc_backend, localFilesystemUtf8Names);
#endif
    suite_add_tcase(s, tc_backend);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
