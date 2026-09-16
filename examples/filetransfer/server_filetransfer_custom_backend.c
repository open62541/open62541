/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/* This example implements a custom UA_FileTransferBackend that stores the
 * file content in memory. Use this pattern for devices without a filesystem
 * (content in flash or generated on the fly) or to stage the content before
 * committing it to the actual storage.
 *
 * The backend serves a flat set of files below the mount root. The mandatory
 * operations of the interface are implemented; directory creation is
 * rejected to keep the example small. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/driver/file_transfer.h>
#include <open62541/server.h>

#include <stdlib.h>
#include <string.h>

#define MEMFS_MAXFILES 16

typedef struct {
    UA_Boolean used;
    UA_String name;
    UA_ByteString content;
    UA_DateTime lastModified;
} MemFile;

typedef struct {
    MemFile files[MEMFS_MAXFILES];
} MemFs;

/* Every open call gets its own context with an independent position */
typedef struct {
    MemFile *file;
    size_t position;
} MemFsOpenFile;

static MemFile *
memFsFind(MemFs *fs, const UA_String name) {
    for(size_t i = 0; i < MEMFS_MAXFILES; i++) {
        if(fs->files[i].used && UA_String_equal(&fs->files[i].name, &name))
            return &fs->files[i];
    }
    return NULL;
}

static UA_StatusCode
memFsOpen(UA_FileTransferBackend *b, const UA_String path, UA_Byte mode,
          void **fileContext) {
    MemFile *file = memFsFind((MemFs*)b->context, path);
    if(!file)
        return UA_STATUSCODE_BADNOTFOUND;
    if(mode & UA_OPENFILEMODE_ERASEEXISTING) {
        UA_ByteString_clear(&file->content);
        file->lastModified = UA_DateTime_now();
    }
    MemFsOpenFile *of = (MemFsOpenFile*)UA_calloc(1, sizeof(MemFsOpenFile));
    if(!of)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    of->file = file;
    of->position = (mode & UA_OPENFILEMODE_APPEND) ? file->content.length : 0;
    *fileContext = of;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memFsClose(UA_FileTransferBackend *b, void *fileContext) {
    UA_free(fileContext);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memFsRead(UA_FileTransferBackend *b, void *fileContext, UA_Int32 length,
          UA_ByteString *out) {
    MemFsOpenFile *of = (MemFsOpenFile*)fileContext;
    size_t remaining = (of->position < of->file->content.length) ?
        of->file->content.length - of->position : 0;
    size_t toRead = ((size_t)length < remaining) ? (size_t)length : remaining;
    if(toRead == 0) {
        UA_ByteString_init(out); /* Empty: the end of the file is reached */
        return UA_STATUSCODE_GOOD;
    }
    UA_StatusCode res = UA_ByteString_allocBuffer(out, toRead);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    memcpy(out->data, of->file->content.data + of->position, toRead);
    of->position += toRead;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memFsWrite(UA_FileTransferBackend *b, void *fileContext,
           const UA_ByteString data) {
    MemFsOpenFile *of = (MemFsOpenFile*)fileContext;
    MemFile *file = of->file;
    size_t newLength = of->position + data.length;
    if(newLength > file->content.length) {
        UA_Byte *grown = (UA_Byte*)UA_realloc(file->content.data, newLength);
        if(!grown)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        file->content.data = grown;
        file->content.length = newLength;
    }
    memcpy(file->content.data + of->position, data.data, data.length);
    of->position += data.length;
    file->lastModified = UA_DateTime_now();
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memFsGetPosition(UA_FileTransferBackend *b, void *fileContext,
                 UA_UInt64 *outPosition) {
    *outPosition = ((MemFsOpenFile*)fileContext)->position;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memFsSetPosition(UA_FileTransferBackend *b, void *fileContext,
                 UA_UInt64 position) {
    MemFsOpenFile *of = (MemFsOpenFile*)fileContext;
    of->position = (position < of->file->content.length) ?
        (size_t)position : of->file->content.length;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memFsGetAttributes(UA_FileTransferBackend *b, const UA_String path,
                   UA_FileTransferFileInfo *outInfo) {
    memset(outInfo, 0, sizeof(UA_FileTransferFileInfo));
    if(path.length == 0) { /* The mount root */
        outInfo->isDirectory = true;
        outInfo->writable = true;
        return UA_STATUSCODE_GOOD;
    }
    MemFile *file = memFsFind((MemFs*)b->context, path);
    if(!file)
        return UA_STATUSCODE_BADNOTFOUND;
    outInfo->size = file->content.length;
    outInfo->lastModified = file->lastModified;
    outInfo->writable = true;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memFsListDirectory(UA_FileTransferBackend *b, const UA_String path,
                   UA_FileTransferListCallback cb, void *listContext) {
    if(path.length > 0)
        return UA_STATUSCODE_BADNOTFOUND; /* Flat hierarchy */
    MemFs *fs = (MemFs*)b->context;
    for(size_t i = 0; i < MEMFS_MAXFILES; i++) {
        if(fs->files[i].used)
            cb(listContext, fs->files[i].name, false);
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memFsCreateFile(UA_FileTransferBackend *b, const UA_String path) {
    MemFs *fs = (MemFs*)b->context;
    if(memFsFind(fs, path))
        return UA_STATUSCODE_BADBROWSENAMEDUPLICATED;
    for(size_t i = 0; i < MEMFS_MAXFILES; i++) {
        MemFile *file = &fs->files[i];
        if(file->used)
            continue;
        if(UA_String_copy(&path, &file->name) != UA_STATUSCODE_GOOD)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        file->used = true;
        file->content = UA_BYTESTRING_NULL;
        file->lastModified = UA_DateTime_now();
        return UA_STATUSCODE_GOOD;
    }
    return UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
}

static UA_StatusCode
memFsCreateDirectory(UA_FileTransferBackend *b, const UA_String path) {
    return UA_STATUSCODE_BADNOTSUPPORTED; /* Flat hierarchy */
}

static UA_StatusCode
memFsRemove(UA_FileTransferBackend *b, const UA_String path) {
    MemFile *file = memFsFind((MemFs*)b->context, path);
    if(!file)
        return UA_STATUSCODE_BADNOTFOUND;
    UA_String_clear(&file->name);
    UA_ByteString_clear(&file->content);
    file->used = false;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memFsRename(UA_FileTransferBackend *b, const UA_String fromPath,
            const UA_String toPath) {
    MemFs *fs = (MemFs*)b->context;
    MemFile *file = memFsFind(fs, fromPath);
    if(!file)
        return UA_STATUSCODE_BADNOTFOUND;
    if(memFsFind(fs, toPath))
        return UA_STATUSCODE_BADBROWSENAMEDUPLICATED;
    UA_String newName = UA_STRING_NULL;
    if(UA_String_copy(&toPath, &newName) != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_String_clear(&file->name);
    file->name = newName;
    return UA_STATUSCODE_GOOD;
}

static void
memFsClear(UA_FileTransferBackend *b) {
    MemFs *fs = (MemFs*)b->context;
    if(!fs)
        return;
    for(size_t i = 0; i < MEMFS_MAXFILES; i++) {
        UA_String_clear(&fs->files[i].name);
        UA_ByteString_clear(&fs->files[i].content);
    }
    UA_free(fs);
    b->context = NULL;
}

static UA_StatusCode
memFsBackend(UA_FileTransferBackend *out) {
    MemFs *fs = (MemFs*)UA_calloc(1, sizeof(MemFs));
    if(!fs)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memset(out, 0, sizeof(UA_FileTransferBackend));
    out->context = fs;
    out->openFile = memFsOpen;
    out->closeFile = memFsClose;
    out->read = memFsRead;
    out->write = memFsWrite;
    out->getPosition = memFsGetPosition;
    out->setPosition = memFsSetPosition;
    out->getAttributes = memFsGetAttributes;
    out->listDirectory = memFsListDirectory;
    out->createFile = memFsCreateFile;
    out->createDirectory = memFsCreateDirectory;
    out->remove = memFsRemove;
    out->rename = memFsRename;
    out->copy = NULL; /* The driver emulates copying with read/write loops */
    out->clear = memFsClear;
    return UA_STATUSCODE_GOOD;
}

int main(void) {
    UA_Server *server = UA_Server_new();

    UA_FileTransferDriver *ftd = UA_FileTransferDriver_new(UA_KEYVALUEMAP_NULL);
    if(!ftd) {
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }
    UA_Server_addDriver(server, &ftd->drv);
    ftd->drv.start(&ftd->drv);

    /* Prepare the in-memory content */
    UA_FileTransferBackend backend;
    UA_StatusCode res = memFsBackend(&backend);
    if(res != UA_STATUSCODE_GOOD) {
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }
    backend.createFile(&backend, UA_STRING("device-report.txt"));

    /* Mount the backend as a FileSystem Object */
    UA_NodeId fileSystemId;
    res = ftd->addFileSystem(ftd, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                             UA_QUALIFIEDNAME(0, "FileSystem"), backend,
                             NULL, &fileSystemId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Mounting the in-memory backend failed");
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }
    UA_NodeId_clear(&fileSystemId);

    UA_Server_runUntilInterrupt(server);
    UA_Server_delete(server);
    return EXIT_SUCCESS;
}
