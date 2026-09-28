/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/* An in-memory directory backend. Each open returns an index into its private
 * handle table. The directory contains only files; subdirectory creation is
 * rejected to keep the example small. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/driver/file_transfer.h>
#include <open62541/server.h>

#include <stdlib.h>
#include <string.h>

#define MEMFS_MAXFILES 16
#define MEMFS_MAXOPEN 16

typedef struct {
    UA_Boolean used;
    UA_String name;
    UA_ByteString content;
    UA_DateTime lastModified;
} MemFile;

/* Every open call gets its own entry with an independent position. The
 * backend returns the index + 1 as the handle. */
typedef struct {
    MemFile *file;
    size_t position;
} MemFsOpenFile;

typedef struct {
    MemFile files[MEMFS_MAXFILES];
    MemFsOpenFile open[MEMFS_MAXOPEN];
} MemFs;

static MemFile *
memFsFind(MemFs *fs, const UA_String name) {
    for(size_t i = 0; i < MEMFS_MAXFILES; i++) {
        if(fs->files[i].used && UA_String_equal(&fs->files[i].name, &name))
            return &fs->files[i];
    }
    return NULL;
}

static MemFsOpenFile *
memFsHandle(UA_FileTransferFileBackend *b, UA_UInt32 handle) {
    MemFs *fs = (MemFs*)b->context;
    if(handle == 0 || handle > MEMFS_MAXOPEN || !fs->open[handle - 1].file)
        return NULL;
    return &fs->open[handle - 1];
}

static UA_StatusCode
memFsOpen(UA_FileTransferFileBackend *b, const UA_String path, UA_Byte mode,
          UA_UInt32 *handle) {
    MemFs *fs = (MemFs*)b->context;
    MemFile *file = memFsFind(fs, path);
    if(!file)
        return UA_STATUSCODE_BADNOTFOUND;
    size_t i = 0;
    while(i < MEMFS_MAXOPEN && fs->open[i].file)
        i++;
    if(i == MEMFS_MAXOPEN)
        return UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
    if(mode & UA_OPENFILEMODE_ERASEEXISTING) {
        UA_ByteString_clear(&file->content);
        file->lastModified = UA_DateTime_now();
    }
    fs->open[i].file = file;
    fs->open[i].position =
        (mode & UA_OPENFILEMODE_APPEND) ? file->content.length : 0;
    *handle = (UA_UInt32)(i + 1);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memFsClose(UA_FileTransferFileBackend *b, UA_UInt32 handle) {
    MemFsOpenFile *of = memFsHandle(b, handle);
    if(!of)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    of->file = NULL;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memFsRead(UA_FileTransferFileBackend *b, UA_UInt32 handle, UA_Int32 length,
          UA_ByteString *out) {
    MemFsOpenFile *of = memFsHandle(b, handle);
    if(!of)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
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
memFsWrite(UA_FileTransferFileBackend *b, UA_UInt32 handle,
           const UA_ByteString data) {
    MemFsOpenFile *of = memFsHandle(b, handle);
    if(!of)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
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
memFsGetPosition(UA_FileTransferFileBackend *b, UA_UInt32 handle,
                 UA_UInt64 *outPosition) {
    MemFsOpenFile *of = memFsHandle(b, handle);
    if(!of)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    *outPosition = of->position;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memFsSetPosition(UA_FileTransferFileBackend *b, UA_UInt32 handle,
                 UA_UInt64 position) {
    MemFsOpenFile *of = memFsHandle(b, handle);
    if(!of)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    of->position = (position < of->file->content.length) ?
        (size_t)position : of->file->content.length;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memFsGetInfo(UA_FileTransferFileBackend *b, const UA_String path,
             UA_FileTransferFileInfo *outInfo) {
    if(path.length == 0) { /* The directory root */
        outInfo->isDirectory = true;
        outInfo->accessRights = UA_FILEACCESS_READ | UA_FILEACCESS_WRITE |
                                UA_FILEACCESS_TRAVERSE;
        return UA_STATUSCODE_GOOD;
    }
    MemFile *file = memFsFind((MemFs*)b->context, path);
    if(!file)
        return UA_STATUSCODE_BADNOTFOUND;
    if(file->name.length > UA_FILETRANSFER_FILENAME_MAX)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    memcpy(outInfo->name, file->name.data, file->name.length);
    outInfo->name[file->name.length] = 0;
    outInfo->size = file->content.length;
    outInfo->lastModified = file->lastModified;
    outInfo->accessRights = UA_FILEACCESS_READ | UA_FILEACCESS_WRITE;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memFsListDirectory(UA_FileTransferBackend *b, const UA_String path,
                   UA_FileTransferListCallback cb, void *listContext) {
    if(path.length > 0)
        return UA_STATUSCODE_BADNOTFOUND; /* Flat hierarchy */
    MemFs *fs = (MemFs*)b->file.context;
    for(size_t i = 0; i < MEMFS_MAXFILES; i++) {
        if(!fs->files[i].used)
            continue;
        UA_FileTransferFileInfo info;
        memset(&info, 0, sizeof(info));
        UA_StatusCode res = memFsGetInfo(&b->file, fs->files[i].name, &info);
        if(res != UA_STATUSCODE_GOOD)
            return res;
        cb(listContext, &info);
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
memFsCreate(UA_FileTransferBackend *b, const UA_String path,
             const UA_FileTransferFileInfo *info) {
    if(info->isDirectory)
        return UA_STATUSCODE_BADNOTSUPPORTED; /* Flat hierarchy */
    if(path.length > UA_FILETRANSFER_FILENAME_MAX)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    MemFs *fs = (MemFs*)b->file.context;
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
memFsRemove(UA_FileTransferBackend *b, const UA_String path) {
    MemFile *file = memFsFind((MemFs*)b->file.context, path);
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
    MemFs *fs = (MemFs*)b->file.context;
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
memFsClear(UA_FileTransferFileBackend *b) {
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
    out->file.context = fs;
    out->file.open = memFsOpen;
    out->file.close = memFsClose;
    out->file.read = memFsRead;
    out->file.write = memFsWrite;
    out->file.getPosition = memFsGetPosition;
    out->file.setPosition = memFsSetPosition;
    out->file.getInfo = memFsGetInfo;
    out->file.clear = memFsClear;
    out->listDirectory = memFsListDirectory;
    out->create = memFsCreate;
    out->remove = memFsRemove;
    out->rename = memFsRename;
    out->copy = NULL; /* The driver emulates copying with read/write loops */
    return UA_STATUSCODE_GOOD;
}

int main(void) {
    UA_Server *server = UA_Server_new();

    /* Prepare the in-memory content */
    UA_FileTransferBackend backend;
    UA_StatusCode res = memFsBackend(&backend);
    if(res != UA_STATUSCODE_GOOD) {
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }
    UA_FileTransferFileInfo info;
    memset(&info, 0, sizeof(info));
    strcpy(info.name, "device-report.txt");
    res = backend.create(&backend, UA_STRING(info.name), &info);
    if(res != UA_STATUSCODE_GOOD) {
        backend.file.clear(&backend.file);
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    UA_Driver *ftd = NULL;
    res = UA_FileTransferDriver_newDirectory(server, &backend, NULL, NULL, &ftd);
    if(res != UA_STATUSCODE_GOOD) {
        backend.file.clear(&backend.file);
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }
    /* The root already exists. Registration transfers ownership to the server. */
    res = UA_Server_addDriver(server, ftd);
    if(res != UA_STATUSCODE_GOOD) {
        ftd->free(ftd);
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    UA_Server_runUntilInterrupt(server);
    UA_Server_delete(server);
    return EXIT_SUCCESS;
}
