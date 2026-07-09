/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include <open62541/driver/file_transfer.h>

#include "open62541_queue.h"

#if defined(UA_ENABLE_METHODCALLS) && defined(UA_GENERATED_NAMESPACE_ZERO_FULL)

#if defined(UA_ARCHITECTURE_POSIX) || defined(UA_ARCHITECTURE_WIN32) || defined(__APPLE__)
# define UA_FILETRANSFER_LOCALBACKEND
# include "../arch/posix/eventloop_posix.h"
# include "mp_printf.h"
# include <errno.h>
# ifdef UA_ARCHITECTURE_WIN32
#  include <io.h>
#  define ft_rmdir _rmdir
#  define ft_access _access
#  define FT_ISDIR(mode) (((mode) & _S_IFDIR) != 0)
# else
#  define ft_rmdir rmdir
#  define ft_access access
#  define FT_ISDIR(mode) S_ISDIR(mode)
# endif
#endif

/**************************************
 * Types and Constants
 **************************************/

#define UA_DRIVER_FILE_TRANSFER_NAME "file-transfer"

typedef struct FTMount FTMount;
typedef struct FTNode FTNode;

/* One entry per fileHandle returned by the Open Method. Handles are bound to
 * the Session that created them. */
typedef struct FTHandle {
    LIST_ENTRY(FTHandle) listEntry;
    UA_UInt32 handle; /* Globally unique, never 0 */
    UA_NodeId sessionId;
    FTNode *file;
    UA_Byte mode;
    void *backendFileContext;
} FTHandle;

/* One entry per driver-managed FileType/FileDirectoryType Object. The
 * registry is driver-owned so that the node context of the Objects remains
 * available to the application. */
struct FTNode {
    LIST_ENTRY(FTNode) listEntry;
    UA_NodeId nodeId;
    FTMount *mount;
    UA_String path; /* Relative to the mount root */
    UA_Boolean isDirectory;
    UA_Boolean zombie; /* Backend entry vanished; the node is removed when
                        * the last open handle is closed */
    UA_UInt16 openCount;
    UA_Boolean openForWrite;
    UA_NodeId openCountId; /* The OpenCount Property of a file node */
};

struct FTMount {
    LIST_ENTRY(FTMount) listEntry;
    UA_NodeId rootNodeId;
    UA_FileTransferBackend backend;
    UA_FileTransferMountOptions options;
    UA_Boolean standaloneFile; /* Created via addFile (no directory tree) */
};

typedef struct FileTransferDriver {
    UA_FileTransferDriver driver;
    UA_Logger *logging;
    LIST_HEAD(, FTMount) mounts;
    LIST_HEAD(, FTNode) nodes;
    LIST_HEAD(, FTHandle) handles;
    UA_UInt32 nextHandle;
    UA_UInt16 maxHandlesPerSession;
    UA_UInt16 maxHandlesPerFile;
    UA_UInt32 maxReadLength;
} FileTransferDriver;

#define UA_FILETRANSFER_MAXHANDLESPERSESSION_DEFAULT 64
#define UA_FILETRANSFER_MAXHANDLESPERFILE_DEFAULT 16
#define UA_FILETRANSFER_MAXREADLENGTH_DEFAULT (1 << 20) /* 1 MByte */

#define UA_FILETRANSFER_OPENMODE_ALLBITS                        \
    (UA_OPENFILEMODE_READ | UA_OPENFILEMODE_WRITE |             \
     UA_OPENFILEMODE_ERASEEXISTING | UA_OPENFILEMODE_APPEND)

/**************************************
 * Local Filesystem Backend
 **************************************/

#ifdef UA_FILETRANSFER_LOCALBACKEND

typedef struct {
    UA_String rootPath;
} LocalFileSystemContext;

static UA_StatusCode
errnoToStatusCode(int err) {
    switch(err) {
    case ENOENT: return UA_STATUSCODE_BADNOTFOUND;
    case EACCES:
    case EPERM:  return UA_STATUSCODE_BADUSERACCESSDENIED;
    case EEXIST: return UA_STATUSCODE_BADBROWSENAMEDUPLICATED;
    case ENOMEM: return UA_STATUSCODE_BADOUTOFMEMORY;
#ifdef ENOTEMPTY
    case ENOTEMPTY: return UA_STATUSCODE_BADINVALIDSTATE;
#endif
    default: return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
}

/* Only well-formed relative paths reach the storage: no leading, trailing or
 * double slashes, no "." or ".." segments, no backslashes or NUL bytes. This
 * guarantees that the resulting path cannot escape the root directory. */
static UA_StatusCode
checkRelativePath(const UA_String path) {
    if(path.length == 0)
        return UA_STATUSCODE_GOOD; /* The mount root */

    for(size_t i = 0; i < path.length; i++) {
        if(path.data[i] == '\\' || path.data[i] == 0)
            return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    size_t segStart = 0;
    for(size_t i = 0; i <= path.length; i++) {
        if(i < path.length && path.data[i] != '/')
            continue;
        size_t segLen = i - segStart;
        if(segLen == 0)
            return UA_STATUSCODE_BADINVALIDARGUMENT;
        if(segLen == 1 && path.data[segStart] == '.')
            return UA_STATUSCODE_BADINVALIDARGUMENT;
        if(segLen == 2 && path.data[segStart] == '.' &&
           path.data[segStart + 1] == '.')
            return UA_STATUSCODE_BADINVALIDARGUMENT;
        segStart = i + 1;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
buildLocalPath(const LocalFileSystemContext *ctx, const UA_String relPath,
               char *out) {
    UA_StatusCode res = checkRelativePath(relPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    int len;
    if(relPath.length == 0)
        len = mp_snprintf(out, UA_PATH_MAX, "%.*s",
                          (int)ctx->rootPath.length, (char*)ctx->rootPath.data);
    else
        len = mp_snprintf(out, UA_PATH_MAX, "%.*s/%.*s",
                          (int)ctx->rootPath.length, (char*)ctx->rootPath.data,
                          (int)relPath.length, (char*)relPath.data);
    if(len < 0 || len >= UA_PATH_MAX)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
localFsOpenFile(UA_FileTransferBackend *b, const UA_String path,
                UA_Byte mode, void **fileContext) {
    char localPath[UA_PATH_MAX];
    UA_StatusCode res = buildLocalPath((LocalFileSystemContext*)b->context,
                                       path, localPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_Boolean readBit = (mode & UA_OPENFILEMODE_READ) != 0;
    UA_Boolean writeBit = (mode & UA_OPENFILEMODE_WRITE) != 0;
    UA_Boolean eraseBit = (mode & UA_OPENFILEMODE_ERASEEXISTING) != 0;
    UA_Boolean appendBit = (mode & UA_OPENFILEMODE_APPEND) != 0;

    /* "r+b" opens an existing file for writing without truncation. "wb" /
     * "w+b" truncate for the EraseExisting semantics. The driver already
     * validates the mode bit combinations. */
    const char *fmode;
    if(writeBit)
        fmode = eraseBit ? (readBit ? "w+b" : "wb") : "r+b";
    else if(readBit)
        fmode = "rb";
    else
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    errno = 0;
    UA_FILE *fp = UA_fopen(localPath, fmode);
    if(!fp)
        return errnoToStatusCode(errno);

    if(appendBit && UA_fseek(fp, 0, UA_SEEK_END) != 0) {
        UA_fclose(fp);
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

    *fileContext = fp;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
localFsCloseFile(UA_FileTransferBackend *b, void *fileContext) {
    if(UA_fclose((UA_FILE*)fileContext) != 0)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
localFsRead(UA_FileTransferBackend *b, void *fileContext,
            UA_Int32 length, UA_ByteString *out) {
    if(length <= 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode res = UA_ByteString_allocBuffer(out, (size_t)length);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Update streams require a positioning call between switching from
     * writing to reading (C99 7.19.5.3) */
    UA_FILE *fp = (UA_FILE*)fileContext;
    UA_fseek(fp, 0, SEEK_CUR);
    size_t bytesRead = UA_fread(out->data, 1, (size_t)length, fp);
    if(bytesRead < (size_t)length && ferror(fp)) {
        UA_ByteString_clear(out);
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

    if(bytesRead == 0) {
        UA_ByteString_clear(out); /* The end of the file is reached */
        return UA_STATUSCODE_GOOD;
    }

    out->length = bytesRead;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
localFsWrite(UA_FileTransferBackend *b, void *fileContext,
             const UA_ByteString data) {
    if(data.length == 0)
        return UA_STATUSCODE_GOOD;

    /* Update streams require a positioning call between switching from
     * reading to writing (C99 7.19.5.3) */
    UA_FILE *fp = (UA_FILE*)fileContext;
    UA_fseek(fp, 0, SEEK_CUR);
    size_t written = UA_fwrite(data.data, 1, data.length, fp);
    if(written != data.length)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
localFsGetPosition(UA_FileTransferBackend *b, void *fileContext,
                   UA_UInt64 *outPosition) {
    long pos = UA_ftell((UA_FILE*)fileContext);
    if(pos < 0)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    *outPosition = (UA_UInt64)pos;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
localFsSetPosition(UA_FileTransferBackend *b, void *fileContext,
                   UA_UInt64 position) {
    /* Positions beyond the end of the file are clamped to the file size.
     * File sizes are limited to LONG_MAX by the underlying long-based
     * stdio positioning. */
    UA_FILE *fp = (UA_FILE*)fileContext;
    if(UA_fseek(fp, 0, UA_SEEK_END) != 0)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    long size = UA_ftell(fp);
    if(size < 0)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;

    long target = (position < (UA_UInt64)size) ? (long)position : size;
    if(UA_fseek(fp, target, UA_SEEK_SET) != 0)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    return UA_STATUSCODE_GOOD;
}

/* Guess the RFC 2046 media type from the filename extension. Returns a static
 * string (safe to borrow) or the empty string for unknown extensions. */
static UA_String
localFsMimeType(const UA_String path) {
    static const struct {
        const char *ext;
        const char *mime;
    } table[] = {
        {"txt", "text/plain"},        {"xml", "application/xml"},
        {"json", "application/json"}, {"html", "text/html"},
        {"htm", "text/html"},         {"csv", "text/csv"},
        {"pdf", "application/pdf"},    {"png", "image/png"},
        {"jpg", "image/jpeg"},        {"jpeg", "image/jpeg"},
        {"svg", "image/svg+xml"}
    };

    /* Find the extension (the segment after the last '.' in the last path
     * segment) */
    size_t dot = path.length;
    for(size_t i = path.length; i > 0; i--) {
        if(path.data[i - 1] == '/')
            break;
        if(path.data[i - 1] == '.') {
            dot = i; /* First byte of the extension */
            break;
        }
    }
    if(dot >= path.length)
        return UA_STRING_NULL;
    size_t extLen = path.length - dot;

    for(size_t i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
        if(strlen(table[i].ext) != extLen)
            continue;
        UA_Boolean match = true;
        for(size_t j = 0; j < extLen; j++) {
            char c = (char)path.data[dot + j];
            if(c >= 'A' && c <= 'Z')
                c = (char)(c - 'A' + 'a'); /* Case-insensitive */
            if(c != table[i].ext[j]) {
                match = false;
                break;
            }
        }
        if(match)
            return UA_STRING((char*)(uintptr_t)table[i].mime);
    }
    return UA_STRING_NULL;
}

static UA_StatusCode
localFsGetAttributes(UA_FileTransferBackend *b, const UA_String path,
                     UA_FileTransferFileInfo *outInfo) {
    char localPath[UA_PATH_MAX];
    UA_StatusCode res = buildLocalPath((LocalFileSystemContext*)b->context,
                                       path, localPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    errno = 0;
    struct UA_STAT st;
    if(UA_stat(localPath, &st) != 0)
        return errnoToStatusCode(errno);

    memset(outInfo, 0, sizeof(UA_FileTransferFileInfo));
    outInfo->size = (UA_UInt64)st.st_size;
    outInfo->lastModified = UA_DATETIME_UNIX_EPOCH +
        (UA_DateTime)st.st_mtime * UA_DATETIME_SEC;
    outInfo->isDirectory = FT_ISDIR(st.st_mode);
    outInfo->writable = (ft_access(localPath, 2 /* W_OK */) == 0);
    if(!outInfo->isDirectory)
        outInfo->mimeType = localFsMimeType(path);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
localFsListDirectory(UA_FileTransferBackend *b, const UA_String path,
                     UA_FileTransferListCallback cb, void *listContext) {
    char localPath[UA_PATH_MAX];
    UA_StatusCode res = buildLocalPath((LocalFileSystemContext*)b->context,
                                       path, localPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    errno = 0;
    UA_DIR *dir = UA_opendir(localPath);
    if(!dir)
        return errnoToStatusCode(errno);

    struct UA_DIRENT *entry;
    while((entry = UA_readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        /* Not all filesystems report the entry type in the dirent */
        char entryPath[UA_PATH_MAX];
        int len = mp_snprintf(entryPath, UA_PATH_MAX, "%s/%s",
                              localPath, entry->d_name);
        if(len < 0 || len >= UA_PATH_MAX)
            continue;
        struct UA_STAT st;
        if(UA_stat(entryPath, &st) != 0)
            continue;

        UA_String name = UA_STRING(entry->d_name);
        cb(listContext, name, FT_ISDIR(st.st_mode));
    }

    UA_closedir(dir);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
localFsCreateFile(UA_FileTransferBackend *b, const UA_String path) {
    char localPath[UA_PATH_MAX];
    UA_StatusCode res = buildLocalPath((LocalFileSystemContext*)b->context,
                                       path, localPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    struct UA_STAT st;
    if(UA_stat(localPath, &st) == 0)
        return UA_STATUSCODE_BADBROWSENAMEDUPLICATED;

    errno = 0;
    UA_FILE *fp = UA_fopen(localPath, "wb");
    if(!fp)
        return errnoToStatusCode(errno);
    UA_fclose(fp);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
localFsCreateDirectory(UA_FileTransferBackend *b, const UA_String path) {
    char localPath[UA_PATH_MAX];
    UA_StatusCode res = buildLocalPath((LocalFileSystemContext*)b->context,
                                       path, localPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    errno = 0;
    if(UA_mkdir(localPath, 0755) != 0)
        return errnoToStatusCode(errno);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
localFsRemove(UA_FileTransferBackend *b, const UA_String path) {
    char localPath[UA_PATH_MAX];
    UA_StatusCode res = buildLocalPath((LocalFileSystemContext*)b->context,
                                       path, localPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    errno = 0;
    struct UA_STAT st;
    if(UA_stat(localPath, &st) != 0)
        return errnoToStatusCode(errno);

    int ret = FT_ISDIR(st.st_mode) ? ft_rmdir(localPath) : UA_remove(localPath);
    if(ret != 0)
        return errnoToStatusCode(errno);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
localFsRename(UA_FileTransferBackend *b, const UA_String fromPath,
              const UA_String toPath) {
    char localFrom[UA_PATH_MAX];
    char localTo[UA_PATH_MAX];
    LocalFileSystemContext *ctx = (LocalFileSystemContext*)b->context;
    UA_StatusCode res = buildLocalPath(ctx, fromPath, localFrom);
    res |= buildLocalPath(ctx, toPath, localTo);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    errno = 0;
    if(rename(localFrom, localTo) != 0)
        return errnoToStatusCode(errno);
    return UA_STATUSCODE_GOOD;
}

static void
localFsClear(UA_FileTransferBackend *b) {
    LocalFileSystemContext *ctx = (LocalFileSystemContext*)b->context;
    if(!ctx)
        return;
    UA_String_clear(&ctx->rootPath);
    UA_free(ctx);
    b->context = NULL;
}

UA_StatusCode
UA_FileTransferBackend_localFilesystem(const UA_String rootPath,
                                       UA_FileTransferBackend *out) {
    if(!out || rootPath.length == 0 || rootPath.length >= UA_PATH_MAX)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* The root directory must exist */
    char localPath[UA_PATH_MAX];
    mp_snprintf(localPath, UA_PATH_MAX, "%.*s",
                (int)rootPath.length, (char*)rootPath.data);
    errno = 0;
    struct UA_STAT st;
    if(UA_stat(localPath, &st) != 0)
        return errnoToStatusCode(errno);
    if(!FT_ISDIR(st.st_mode))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    LocalFileSystemContext *ctx = (LocalFileSystemContext*)
        UA_calloc(1, sizeof(LocalFileSystemContext));
    if(!ctx)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Copy the root path without trailing slashes */
    UA_String root = rootPath;
    while(root.length > 1 &&
          (root.data[root.length - 1] == '/' ||
           root.data[root.length - 1] == '\\'))
        root.length--;
    UA_StatusCode res = UA_String_copy(&root, &ctx->rootPath);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(ctx);
        return res;
    }

    memset(out, 0, sizeof(UA_FileTransferBackend));
    out->context = ctx;
    out->openFile = localFsOpenFile;
    out->closeFile = localFsCloseFile;
    out->read = localFsRead;
    out->write = localFsWrite;
    out->getPosition = localFsGetPosition;
    out->setPosition = localFsSetPosition;
    out->getAttributes = localFsGetAttributes;
    out->listDirectory = localFsListDirectory;
    out->createFile = localFsCreateFile;
    out->createDirectory = localFsCreateDirectory;
    out->remove = localFsRemove;
    out->rename = localFsRename;
    out->copy = NULL; /* Emulated by the driver */
    out->clear = localFsClear;
    return UA_STATUSCODE_GOOD;
}

#else /* UA_FILETRANSFER_LOCALBACKEND */

UA_StatusCode
UA_FileTransferBackend_localFilesystem(const UA_String rootPath,
                                       UA_FileTransferBackend *out) {
    return UA_STATUSCODE_BADNOTSUPPORTED;
}

#endif /* UA_FILETRANSFER_LOCALBACKEND */

/**************************************
 * Driver Lookup and Notifications
 **************************************/

static UA_StatusCode FileTransferDriver_start(UA_Driver *drv);

static UA_Boolean
isFileTransferDriver(const UA_Driver *drv) {
    return drv && drv->start == FileTransferDriver_start;
}

static FileTransferDriver *
findFileTransferDriver(UA_Server *server) {
    for(UA_Driver *drv = UA_Server_getDrivers(server); drv; drv = drv->next) {
        if(isFileTransferDriver(drv) &&
           drv->state == UA_LIFECYCLESTATE_STARTED)
            return (FileTransferDriver*)drv;
    }
    return NULL;
}

/**************************************
 * Registry and Handle Management
 **************************************/

static FTNode *
findFTNode(FileTransferDriver *ftd, const UA_NodeId *nodeId) {
    FTNode *node;
    LIST_FOREACH(node, &ftd->nodes, listEntry) {
        if(UA_NodeId_equal(&node->nodeId, nodeId))
            return node;
    }
    return NULL;
}

static FTHandle *
findFTHandle(FileTransferDriver *ftd, const UA_NodeId *sessionId,
             UA_UInt32 handle) {
    FTHandle *h;
    LIST_FOREACH(h, &ftd->handles, listEntry) {
        if(h->handle == handle && UA_NodeId_equal(&h->sessionId, sessionId))
            return h;
    }
    return NULL;
}

static size_t
countSessionHandles(FileTransferDriver *ftd, const UA_NodeId *sessionId) {
    size_t count = 0;
    FTHandle *h;
    LIST_FOREACH(h, &ftd->handles, listEntry) {
        if(UA_NodeId_equal(&h->sessionId, sessionId))
            count++;
    }
    return count;
}

static UA_UInt32
newHandleId(FileTransferDriver *ftd) {
    UA_Boolean inUse;
    do {
        ftd->nextHandle++;
        if(ftd->nextHandle == 0)
            ftd->nextHandle = 1;
        inUse = false;
        FTHandle *h;
        LIST_FOREACH(h, &ftd->handles, listEntry) {
            if(h->handle == ftd->nextHandle) {
                inUse = true;
                break;
            }
        }
    } while(inUse);
    return ftd->nextHandle;
}

static void
updateOpenCount(UA_Server *server, FTNode *node) {
    if(UA_NodeId_isNull(&node->openCountId))
        return;
    UA_Variant value;
    UA_Variant_setScalar(&value, &node->openCount, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_writeValue(server, node->openCountId, value);
}

static void
removeFTNode(FileTransferDriver *ftd, FTNode *node) {
    LIST_REMOVE(node, listEntry);
    UA_NodeId_clear(&node->nodeId);
    UA_NodeId_clear(&node->openCountId);
    UA_String_clear(&node->path);
    UA_free(node);
}

/* Close the backend file context and release the handle. Removes zombie
 * nodes once their last handle is closed. */
static UA_StatusCode
closeFTHandle(UA_Server *server, FileTransferDriver *ftd, FTHandle *h) {
    FTNode *node = h->file;
    UA_FileTransferBackend *b = &node->mount->backend;
    UA_StatusCode res = b->closeFile(b, h->backendFileContext);

    LIST_REMOVE(h, listEntry);
    UA_NodeId_clear(&h->sessionId);
    if(h->mode & UA_OPENFILEMODE_WRITE)
        node->openForWrite = false;
    if(node->openCount > 0)
        node->openCount--;
    UA_free(h);

    if(node->zombie && node->openCount == 0) {
        UA_Server_deleteNode(server, node->nodeId, true);
        removeFTNode(ftd, node);
    } else {
        updateOpenCount(server, node);
    }
    return res;
}

/**************************************
 * Property Value Sources
 **************************************/

static UA_StatusCode
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

static UA_Boolean
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
static UA_StatusCode
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
    res |= UA_Server_setNodeContext(server, sizeId, node);
    res |= UA_Server_setVariableNode_callbackValueSource(server, sizeId,
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
    res |= UA_Server_setNodeContext(server, userWritableId, node);
    res |= UA_Server_setVariableNode_callbackValueSource(server, userWritableId,
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
 * Notifications (Session Cleanup)
 **************************************/

static void
FileTransferDriver_notification(UA_Driver *drv,
                                UA_ApplicationNotificationType type,
                                const UA_KeyValueMap payload) {
    if(type != UA_APPLICATIONNOTIFICATIONTYPE_SESSION_CLOSED)
        return;

    FileTransferDriver *ftd = (FileTransferDriver*)drv;
    const UA_NodeId *sessionId = (const UA_NodeId*)
        UA_KeyValueMap_getScalar(&payload, UA_QUALIFIEDNAME(0, "session-id"),
                                 &UA_TYPES[UA_TYPES_NODEID]);
    if(!sessionId)
        return;

    /* Close all handles of the closed Session */
    FTHandle *h, *tmp;
    LIST_FOREACH_SAFE(h, &ftd->handles, listEntry, tmp) {
        if(UA_NodeId_equal(&h->sessionId, sessionId))
            closeFTHandle(drv->server, ftd, h);
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
static UA_StatusCode
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

static UA_StatusCode
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

static UA_StatusCode
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

static UA_StatusCode
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

static UA_StatusCode
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

static UA_StatusCode
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

static UA_StatusCode
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

/**************************************
 * Directory Tree Mirroring
 **************************************/

static FTNode *
newFTNode(FileTransferDriver *ftd, FTMount *mount, const UA_NodeId nodeId,
          const UA_String path, UA_Boolean isDirectory);

/* Collected directory listing. Processing the entries after the listing
 * avoids reentrant calls into the backend. */
typedef struct ScanEntry {
    struct ScanEntry *next;
    UA_String name;
    UA_Boolean isDir;
    UA_Boolean matched; /* Used during refresh reconciliation */
} ScanEntry;

static void
scanCollector(void *listContext, const UA_String name, UA_Boolean isDirectory) {
    ScanEntry **head = (ScanEntry**)listContext;
    ScanEntry *e = (ScanEntry*)UA_calloc(1, sizeof(ScanEntry));
    if(!e)
        return;
    if(UA_String_copy(&name, &e->name) != UA_STATUSCODE_GOOD) {
        UA_free(e);
        return;
    }
    e->isDir = isDirectory;
    e->next = *head;
    *head = e;
}

static void
freeScanEntries(ScanEntry *head) {
    while(head) {
        ScanEntry *next = head->next;
        UA_String_clear(&head->name);
        UA_free(head);
        head = next;
    }
}

/* Entry names must not contain path separators or navigate the hierarchy */
static UA_Boolean
validEntryName(const UA_String name) {
    if(name.length == 0)
        return false;
    if(name.length == 1 && name.data[0] == '.')
        return false;
    if(name.length == 2 && name.data[0] == '.' && name.data[1] == '.')
        return false;
    for(size_t i = 0; i < name.length; i++) {
        if(name.data[i] == '/' || name.data[i] == '\\' || name.data[i] == 0)
            return false;
    }
    return true;
}

/* "parent/name" with the empty string as the mount root */
static UA_StatusCode
joinPath(const UA_String parent, const UA_String name, UA_String *out) {
    if(parent.length == 0)
        return UA_String_copy(&name, out);
    UA_StatusCode res = UA_ByteString_allocBuffer(
        out, parent.length + 1 + name.length);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    memcpy(out->data, parent.data, parent.length);
    out->data[parent.length] = '/';
    memcpy(out->data + parent.length + 1, name.data, name.length);
    return UA_STATUSCODE_GOOD;
}

static UA_String
pathLastSegment(const UA_String path) {
    for(size_t i = path.length; i > 0; i--) {
        if(path.data[i - 1] == '/') {
            UA_String segment = {path.length - i, path.data + i};
            return segment;
        }
    }
    return path;
}

static UA_UInt32
pathDepth(const UA_String path) {
    if(path.length == 0)
        return 0;
    UA_UInt32 depth = 1;
    for(size_t i = 0; i < path.length; i++) {
        if(path.data[i] == '/')
            depth++;
    }
    return depth;
}

static UA_Boolean
pathWithinSubtree(const UA_String path, const UA_String subtreeRoot) {
    if(subtreeRoot.length == 0)
        return true; /* The mount root contains everything */
    if(path.length < subtreeRoot.length ||
       memcmp(path.data, subtreeRoot.data, subtreeRoot.length) != 0)
        return false;
    return path.length == subtreeRoot.length ||
        path.data[subtreeRoot.length] == '/';
}

static UA_Boolean
isDirectChildPath(const UA_String parent, const UA_String child) {
    size_t offset = 0;
    if(parent.length > 0) {
        if(child.length <= parent.length + 1 ||
           memcmp(child.data, parent.data, parent.length) != 0 ||
           child.data[parent.length] != '/')
            return false;
        offset = parent.length + 1;
    }
    if(child.length == offset)
        return false;
    for(size_t i = offset; i < child.length; i++) {
        if(child.data[i] == '/')
            return false;
    }
    return true;
}

static FTNode *
findChildByPath(FileTransferDriver *ftd, FTMount *mount, const UA_String path) {
    FTNode *node;
    LIST_FOREACH(node, &ftd->nodes, listEntry) {
        if(node->mount == mount && UA_String_equal(&node->path, &path))
            return node;
    }
    return NULL;
}

static UA_Boolean
subtreeHasOpenHandles(FileTransferDriver *ftd, FTNode *root) {
    FTHandle *h;
    LIST_FOREACH(h, &ftd->handles, listEntry) {
        if(h->file->mount == root->mount &&
           pathWithinSubtree(h->file->path, root->path))
            return true;
    }
    return false;
}

static UA_UInt32
countMountNodes(FileTransferDriver *ftd, FTMount *mount) {
    UA_UInt32 count = 0;
    FTNode *node;
    LIST_FOREACH(node, &ftd->nodes, listEntry) {
        if(node->mount == mount)
            count++;
    }
    return count;
}

/* Create a FileType Object with its FTNode below a directory node */
static UA_StatusCode
mirrorFile(UA_Server *server, FileTransferDriver *ftd, FTNode *dirNode,
           const UA_String name, const UA_FileTransferFileInfo *info,
           FTNode **outNode) {
    UA_String path = UA_STRING_NULL;
    UA_StatusCode res = joinPath(dirNode->path, name, &path);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_QualifiedName browseName = {0, name};
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName.text = name;
    UA_NodeId newNodeId = UA_NODEID_NULL;
    res = UA_Server_addObjectNode(server, UA_NODEID_NULL, dirNode->nodeId,
                                  UA_NS0ID(ORGANIZES), browseName,
                                  UA_NS0ID(FILETYPE), attr, NULL, &newNodeId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_String_clear(&path);
        return res;
    }

    FTNode *node = newFTNode(ftd, dirNode->mount, newNodeId, path, false);
    if(node)
        res = setupFileNode(server, ftd, node, info);
    else
        res = UA_STATUSCODE_BADOUTOFMEMORY;
    if(res != UA_STATUSCODE_GOOD) {
        UA_Server_deleteNode(server, newNodeId, true);
        if(node)
            removeFTNode(ftd, node);
    } else if(outNode) {
        *outNode = node;
    }
    UA_NodeId_clear(&newNodeId);
    UA_String_clear(&path);
    return res;
}

/* Create a FileDirectoryType Object with its FTNode below a directory node */
static UA_StatusCode
mirrorDirectory(UA_Server *server, FileTransferDriver *ftd, FTNode *dirNode,
                const UA_String name, FTNode **outNode) {
    UA_String path = UA_STRING_NULL;
    UA_StatusCode res = joinPath(dirNode->path, name, &path);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_QualifiedName browseName = {0, name};
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName.text = name;
    UA_NodeId newNodeId = UA_NODEID_NULL;
    res = UA_Server_addObjectNode(server, UA_NODEID_NULL, dirNode->nodeId,
                                  UA_NS0ID(ORGANIZES), browseName,
                                  UA_NS0ID(FILEDIRECTORYTYPE), attr, NULL,
                                  &newNodeId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_String_clear(&path);
        return res;
    }

    FTNode *node = newFTNode(ftd, dirNode->mount, newNodeId, path, true);
    if(!node) {
        UA_Server_deleteNode(server, newNodeId, true);
        res = UA_STATUSCODE_BADOUTOFMEMORY;
    } else if(outNode) {
        *outNode = node;
    }
    UA_NodeId_clear(&newNodeId);
    UA_String_clear(&path);
    return res;
}

/* Recursively mirror the backend content below a directory node */
static UA_StatusCode
mirrorTree(UA_Server *server, FileTransferDriver *ftd, FTNode *dirNode,
           UA_UInt32 depth, UA_UInt32 *nodeBudget) {
    const UA_FileTransferMountOptions *opts = &dirNode->mount->options;
    if(opts->maxScanDepth > 0 && depth > opts->maxScanDepth)
        return UA_STATUSCODE_GOOD;

    UA_FileTransferBackend *b = &dirNode->mount->backend;
    ScanEntry *entries = NULL;
    UA_StatusCode res = b->listDirectory(b, dirNode->path, scanCollector,
                                         &entries);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    for(ScanEntry *e = entries; e && res == UA_STATUSCODE_GOOD; e = e->next) {
        if(!validEntryName(e->name)) {
            UA_LOG_WARNING(ftd->logging, UA_LOGCATEGORY_SERVER,
                           "FileTransfer: Skipping the entry \"%S\" with an "
                           "invalid name", e->name);
            continue;
        }
        if(*nodeBudget == 0) {
            UA_LOG_WARNING(ftd->logging, UA_LOGCATEGORY_SERVER,
                           "FileTransfer: The maxNodes limit is reached. "
                           "Entries are not mirrored into the address space");
            break;
        }

        if(e->isDir) {
            FTNode *childNode = NULL;
            res = mirrorDirectory(server, ftd, dirNode, e->name, &childNode);
            if(res == UA_STATUSCODE_GOOD) {
                (*nodeBudget)--;
                res = mirrorTree(server, ftd, childNode, depth + 1, nodeBudget);
            }
        } else {
            UA_String childPath = UA_STRING_NULL;
            res = joinPath(dirNode->path, e->name, &childPath);
            if(res != UA_STATUSCODE_GOOD)
                break;
            UA_FileTransferFileInfo info;
            res = b->getAttributes(b, childPath, &info);
            UA_String_clear(&childPath);
            if(res != UA_STATUSCODE_GOOD)
                break;
            res = mirrorFile(server, ftd, dirNode, e->name, &info, NULL);
            if(res == UA_STATUSCODE_GOOD)
                (*nodeBudget)--;
        }
    }

    freeScanEntries(entries);
    return res;
}

/* Remove the address-space nodes and registry entries below (and including)
 * the given subtree root. The caller must ensure that no open handles refer
 * to the subtree. */
static void
removeSubtree(UA_Server *server, FileTransferDriver *ftd, FTNode *subtreeRoot) {
    /* Deleting the root node removes the mirrored children recursively */
    UA_Server_deleteNode(server, subtreeRoot->nodeId, true);

    FTMount *mount = subtreeRoot->mount;
    UA_String rootPath = subtreeRoot->path;
    FTNode *node, *tmp;
    LIST_FOREACH_SAFE(node, &ftd->nodes, listEntry, tmp) {
        if(node->mount == mount && pathWithinSubtree(node->path, rootPath))
            removeFTNode(ftd, node);
    }
}

/* A subtree with open handles cannot be removed right away when its backend
 * entries vanished. The nodes stay in the address space so that the open
 * handles remain usable (Close in particular). Zombie file nodes are removed
 * when their last handle is closed; the remaining zombie nodes are collected
 * on the next refresh. */
static void
markSubtreeZombie(FileTransferDriver *ftd, FTNode *subtreeRoot) {
    FTNode *node;
    LIST_FOREACH(node, &ftd->nodes, listEntry) {
        if(node->mount == subtreeRoot->mount &&
           pathWithinSubtree(node->path, subtreeRoot->path))
            node->zombie = true;
    }
}

/* Recursively delete a backend subtree (bottom-up) */
static UA_StatusCode
deleteBackendTree(UA_FileTransferBackend *b, const UA_String path,
                  UA_Boolean isDir) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(isDir) {
        ScanEntry *entries = NULL;
        res = b->listDirectory(b, path, scanCollector, &entries);
        if(res != UA_STATUSCODE_GOOD)
            return res;
        for(ScanEntry *e = entries; e && res == UA_STATUSCODE_GOOD; e = e->next) {
            UA_String childPath = UA_STRING_NULL;
            res = joinPath(path, e->name, &childPath);
            if(res == UA_STATUSCODE_GOOD)
                res = deleteBackendTree(b, childPath, e->isDir);
            UA_String_clear(&childPath);
        }
        freeScanEntries(entries);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }
    return b->remove(b, path);
}

#define UA_FILETRANSFER_COPYCHUNKSIZE 65536

/* Copy a single file within the same backend. Uses the backend fast-path if
 * available, otherwise emulates the copy with read/write loops. */
static UA_StatusCode
copyBackendFile(UA_FileTransferBackend *b, const UA_String fromPath,
                const UA_String toPath) {
    if(b->copy)
        return b->copy(b, fromPath, toPath);

    void *src = NULL;
    UA_StatusCode res = b->openFile(b, fromPath, UA_OPENFILEMODE_READ, &src);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = b->createFile(b, toPath);
    if(res != UA_STATUSCODE_GOOD) {
        b->closeFile(b, src);
        return res;
    }
    void *dst = NULL;
    res = b->openFile(b, toPath, UA_OPENFILEMODE_WRITE, &dst);
    if(res != UA_STATUSCODE_GOOD) {
        b->closeFile(b, src);
        return res;
    }

    while(res == UA_STATUSCODE_GOOD) {
        UA_ByteString chunk = UA_BYTESTRING_NULL;
        res = b->read(b, src, UA_FILETRANSFER_COPYCHUNKSIZE, &chunk);
        if(res != UA_STATUSCODE_GOOD || chunk.length == 0) {
            UA_ByteString_clear(&chunk);
            break;
        }
        res = b->write(b, dst, chunk);
        UA_ByteString_clear(&chunk);
    }

    b->closeFile(b, src);
    b->closeFile(b, dst);
    return res;
}

static UA_StatusCode
copyBackendTree(UA_FileTransferBackend *b, const UA_String fromPath,
                const UA_String toPath, UA_Boolean isDir) {
    if(!isDir)
        return copyBackendFile(b, fromPath, toPath);

    UA_StatusCode res = b->createDirectory(b, toPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    ScanEntry *entries = NULL;
    res = b->listDirectory(b, fromPath, scanCollector, &entries);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    for(ScanEntry *e = entries; e && res == UA_STATUSCODE_GOOD; e = e->next) {
        UA_String childFrom = UA_STRING_NULL;
        UA_String childTo = UA_STRING_NULL;
        res = joinPath(fromPath, e->name, &childFrom);
        if(res == UA_STATUSCODE_GOOD)
            res = joinPath(toPath, e->name, &childTo);
        if(res == UA_STATUSCODE_GOOD)
            res = copyBackendTree(b, childFrom, childTo, e->isDir);
        UA_String_clear(&childFrom);
        UA_String_clear(&childTo);
    }
    freeScanEntries(entries);
    return res;
}

/* Reconcile a mirrored directory with the backend content */
static UA_StatusCode
syncTree(UA_Server *server, FileTransferDriver *ftd, FTNode *dirNode,
         UA_UInt32 depth, UA_UInt32 *nodeBudget) {
    const UA_FileTransferMountOptions *opts = &dirNode->mount->options;
    if(opts->maxScanDepth > 0 && depth > opts->maxScanDepth)
        return UA_STATUSCODE_GOOD;

    UA_FileTransferBackend *b = &dirNode->mount->backend;
    ScanEntry *entries = NULL;
    UA_StatusCode res = b->listDirectory(b, dirNode->path, scanCollector,
                                         &entries);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Match the registry children against the backend listing. Vanished
     * entries are removed, or marked as zombies while open handles refer to
     * them. */
    FTNode *child, *childTmp;
    LIST_FOREACH_SAFE(child, &ftd->nodes, listEntry, childTmp) {
        if(child->mount != dirNode->mount ||
           !isDirectChildPath(dirNode->path, child->path))
            continue;
        UA_String name = pathLastSegment(child->path);
        ScanEntry *match = NULL;
        for(ScanEntry *e = entries; e; e = e->next) {
            if(!e->matched && e->isDir == child->isDirectory &&
               UA_String_equal(&e->name, &name)) {
                match = e;
                break;
            }
        }
        if(match) {
            match->matched = true;
            child->zombie = false; /* The backend entry (re)appeared */
        } else if(subtreeHasOpenHandles(ftd, child)) {
            markSubtreeZombie(ftd, child);
        } else {
            removeSubtree(server, ftd, child);
        }
    }

    /* Create nodes for new backend entries */
    for(ScanEntry *e = entries; e && res == UA_STATUSCODE_GOOD; e = e->next) {
        if(e->matched || !validEntryName(e->name))
            continue;
        if(*nodeBudget == 0)
            break;
        if(e->isDir) {
            FTNode *newDir = NULL;
            res = mirrorDirectory(server, ftd, dirNode, e->name, &newDir);
            if(res == UA_STATUSCODE_GOOD) {
                (*nodeBudget)--;
                res = mirrorTree(server, ftd, newDir, depth + 1, nodeBudget);
            }
        } else {
            UA_String childPath = UA_STRING_NULL;
            res = joinPath(dirNode->path, e->name, &childPath);
            if(res != UA_STATUSCODE_GOOD)
                break;
            UA_FileTransferFileInfo info;
            res = b->getAttributes(b, childPath, &info);
            UA_String_clear(&childPath);
            if(res != UA_STATUSCODE_GOOD)
                break;
            res = mirrorFile(server, ftd, dirNode, e->name, &info, NULL);
            if(res == UA_STATUSCODE_GOOD)
                (*nodeBudget)--;
        }
    }
    freeScanEntries(entries);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Recurse into the (kept) subdirectories */
    LIST_FOREACH_SAFE(child, &ftd->nodes, listEntry, childTmp) {
        if(child->mount != dirNode->mount || !child->isDirectory ||
           child->zombie || !isDirectChildPath(dirNode->path, child->path))
            continue;
        res = syncTree(server, ftd, child, depth + 1, nodeBudget);
        if(res != UA_STATUSCODE_GOOD)
            break;
    }
    return res;
}

/**************************************
 * FileDirectoryType Method Callbacks
 **************************************/

/* Resolve the FTNode of a directory Object addressed by a Method call */
static UA_StatusCode
resolveDirectoryNode(FileTransferDriver *ftd, const UA_NodeId *objectId,
                     FTNode **outNode) {
    FTNode *node = findFTNode(ftd, objectId);
    if(!node || !node->isDirectory || node->zombie)
        return UA_STATUSCODE_BADNOTFOUND;
    *outNode = node;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
createDirectoryMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                              void *sessionContext, const UA_NodeId *methodId,
                              void *methodContext, const UA_NodeId *objectId,
                              void *objectContext, size_t inputSize, const UA_Variant *input,
                              size_t outputSize, UA_Variant *output) {
    FileTransferDriver *ftd = findFileTransferDriver(server);
    if(!ftd)
        return UA_STATUSCODE_BADNOTSUPPORTED;

    FTNode *dirNode = NULL;
    UA_StatusCode res = resolveDirectoryNode(ftd, objectId, &dirNode);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(!userCanWrite(server, dirNode, sessionId))
        return UA_STATUSCODE_BADUSERACCESSDENIED;

    if(inputSize < 1 || !UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_STRING]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_String name = *(UA_String*)input[0].data;
    if(!validEntryName(name))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_String newPath = UA_STRING_NULL;
    res = joinPath(dirNode->path, name, &newPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(findChildByPath(ftd, dirNode->mount, newPath)) {
        UA_String_clear(&newPath);
        return UA_STATUSCODE_BADBROWSENAMEDUPLICATED;
    }

    UA_FileTransferBackend *b = &dirNode->mount->backend;
    res = b->createDirectory(b, newPath);
    UA_String_clear(&newPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    FTNode *newNode = NULL;
    res = mirrorDirectory(server, ftd, dirNode, name, &newNode);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    return UA_Variant_setScalarCopy(&output[0], &newNode->nodeId,
                                    &UA_TYPES[UA_TYPES_NODEID]);
}

static UA_StatusCode
createFileMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                         void *sessionContext, const UA_NodeId *methodId,
                         void *methodContext, const UA_NodeId *objectId,
                         void *objectContext, size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output) {
    FileTransferDriver *ftd = findFileTransferDriver(server);
    if(!ftd)
        return UA_STATUSCODE_BADNOTSUPPORTED;

    FTNode *dirNode = NULL;
    UA_StatusCode res = resolveDirectoryNode(ftd, objectId, &dirNode);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(!userCanWrite(server, dirNode, sessionId))
        return UA_STATUSCODE_BADUSERACCESSDENIED;

    if(inputSize < 2 ||
       !UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_STRING]) ||
       !UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_BOOLEAN]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_String name = *(UA_String*)input[0].data;
    UA_Boolean requestFileOpen = *(UA_Boolean*)input[1].data;
    if(!validEntryName(name))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_String newPath = UA_STRING_NULL;
    res = joinPath(dirNode->path, name, &newPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(findChildByPath(ftd, dirNode->mount, newPath)) {
        UA_String_clear(&newPath);
        return UA_STATUSCODE_BADBROWSENAMEDUPLICATED;
    }

    UA_FileTransferBackend *b = &dirNode->mount->backend;
    res = b->createFile(b, newPath);
    if(res != UA_STATUSCODE_GOOD) {
        UA_String_clear(&newPath);
        return res;
    }

    UA_FileTransferFileInfo info;
    res = b->getAttributes(b, newPath, &info);
    UA_String_clear(&newPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    FTNode *newNode = NULL;
    res = mirrorFile(server, ftd, dirNode, name, &info, &newNode);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Optionally open the new file for reading and writing */
    UA_UInt32 handle = 0;
    if(requestFileOpen) {
        res = openFileHandle(server, ftd, newNode, sessionId,
                             UA_OPENFILEMODE_READ | UA_OPENFILEMODE_WRITE,
                             &handle);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }

    res = UA_Variant_setScalarCopy(&output[0], &newNode->nodeId,
                                   &UA_TYPES[UA_TYPES_NODEID]);
    res |= UA_Variant_setScalarCopy(&output[1], &handle,
                                    &UA_TYPES[UA_TYPES_UINT32]);
    return res;
}

static UA_StatusCode
deleteMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                     void *sessionContext, const UA_NodeId *methodId,
                     void *methodContext, const UA_NodeId *objectId,
                     void *objectContext, size_t inputSize, const UA_Variant *input,
                     size_t outputSize, UA_Variant *output) {
    FileTransferDriver *ftd = findFileTransferDriver(server);
    if(!ftd)
        return UA_STATUSCODE_BADNOTSUPPORTED;

    FTNode *dirNode = NULL;
    UA_StatusCode res = resolveDirectoryNode(ftd, objectId, &dirNode);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(!userCanWrite(server, dirNode, sessionId))
        return UA_STATUSCODE_BADUSERACCESSDENIED;

    if(inputSize < 1 || !UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_NODEID]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_NodeId *objectToDelete = (UA_NodeId*)input[0].data;

    /* The object must be organized by this directory */
    FTNode *target = findFTNode(ftd, objectToDelete);
    if(!target || target->zombie || target->mount != dirNode->mount ||
       !isDirectChildPath(dirNode->path, target->path))
        return UA_STATUSCODE_BADNOTFOUND;

    /* Open files cannot be deleted */
    if(subtreeHasOpenHandles(ftd, target))
        return UA_STATUSCODE_BADINVALIDSTATE;

    UA_FileTransferBackend *b = &dirNode->mount->backend;
    res = deleteBackendTree(b, target->path, target->isDirectory);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    removeSubtree(server, ftd, target);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
moveOrCopyMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                         void *sessionContext, const UA_NodeId *methodId,
                         void *methodContext, const UA_NodeId *objectId,
                         void *objectContext, size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output) {
    FileTransferDriver *ftd = findFileTransferDriver(server);
    if(!ftd)
        return UA_STATUSCODE_BADNOTSUPPORTED;

    FTNode *dirNode = NULL;
    UA_StatusCode res = resolveDirectoryNode(ftd, objectId, &dirNode);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    if(inputSize < 4 ||
       !UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_NODEID]) ||
       !UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_NODEID]) ||
       !UA_Variant_hasScalarType(&input[2], &UA_TYPES[UA_TYPES_BOOLEAN]) ||
       !UA_Variant_hasScalarType(&input[3], &UA_TYPES[UA_TYPES_STRING]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_NodeId *objectToMoveOrCopy = (UA_NodeId*)input[0].data;
    UA_NodeId *targetDirectory = (UA_NodeId*)input[1].data;
    UA_Boolean createCopy = *(UA_Boolean*)input[2].data;
    UA_String newName = *(UA_String*)input[3].data;

    /* The object must be organized by this directory */
    FTNode *source = findFTNode(ftd, objectToMoveOrCopy);
    if(!source || source->zombie || source->mount != dirNode->mount ||
       !isDirectChildPath(dirNode->path, source->path))
        return UA_STATUSCODE_BADNOTFOUND;

    FTNode *targetDir = NULL;
    res = resolveDirectoryNode(ftd, targetDirectory, &targetDir);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Moving and copying between different mounts (and thus different
     * backends) is not supported */
    if(targetDir->mount != source->mount)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    if(!userCanWrite(server, dirNode, sessionId) ||
       !userCanWrite(server, targetDir, sessionId))
        return UA_STATUSCODE_BADUSERACCESSDENIED;

    /* An empty name keeps the current name. Copy the name: it may point
     * into the source registry entry that is removed during a move. */
    UA_String nameRef = (newName.length > 0) ?
        newName : pathLastSegment(source->path);
    if(!validEntryName(nameRef))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_String name = UA_STRING_NULL;
    res = UA_String_copy(&nameRef, &name);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_String destPath = UA_STRING_NULL;
    res = joinPath(targetDir->path, name, &destPath);
    if(res != UA_STATUSCODE_GOOD) {
        UA_String_clear(&name);
        return res;
    }

    /* Moving to the identical location is a no-op */
    if(!createCopy && UA_String_equal(&destPath, &source->path)) {
        UA_String_clear(&name);
        UA_String_clear(&destPath);
        return UA_Variant_setScalarCopy(&output[0], &source->nodeId,
                                        &UA_TYPES[UA_TYPES_NODEID]);
    }

    if(findChildByPath(ftd, dirNode->mount, destPath)) {
        UA_String_clear(&name);
        UA_String_clear(&destPath);
        return UA_STATUSCODE_BADBROWSENAMEDUPLICATED;
    }

    /* Objects with open file handles are locked */
    if(subtreeHasOpenHandles(ftd, source)) {
        UA_String_clear(&name);
        UA_String_clear(&destPath);
        return UA_STATUSCODE_BADINVALIDSTATE;
    }

    UA_Boolean isDir = source->isDirectory;
    UA_FileTransferBackend *b = &dirNode->mount->backend;
    if(createCopy) {
        res = copyBackendTree(b, source->path, destPath, isDir);
    } else {
        res = b->rename(b, source->path, destPath);
        if(res == UA_STATUSCODE_GOOD)
            removeSubtree(server, ftd, source); /* Invalidates source */
    }

    /* Mirror the entry at the new location */
    FTNode *newNode = NULL;
    if(res == UA_STATUSCODE_GOOD) {
        if(isDir) {
            res = mirrorDirectory(server, ftd, targetDir, name, &newNode);
            if(res == UA_STATUSCODE_GOOD) {
                UA_UInt32 nodeBudget = (UA_UInt32)0xffffffffu;
                const UA_FileTransferMountOptions *opts = &targetDir->mount->options;
                if(opts->maxNodes > 0) {
                    UA_UInt32 current = countMountNodes(ftd, targetDir->mount);
                    nodeBudget = (opts->maxNodes > current) ?
                        opts->maxNodes - current : 0;
                }
                res = mirrorTree(server, ftd, newNode, pathDepth(destPath) + 1,
                                 &nodeBudget);
            }
        } else {
            UA_FileTransferFileInfo info;
            res = b->getAttributes(b, destPath, &info);
            if(res == UA_STATUSCODE_GOOD)
                res = mirrorFile(server, ftd, targetDir, name, &info, &newNode);
        }
    }
    UA_String_clear(&name);
    UA_String_clear(&destPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    return UA_Variant_setScalarCopy(&output[0], &newNode->nodeId,
                                    &UA_TYPES[UA_TYPES_NODEID]);
}

static UA_StatusCode
registerFileTransferMethodCallbacks(UA_Server *server) {
    const struct {
        UA_UInt32 methodId;
        UA_MethodCallback callback;
    } methods[] = {
        {UA_NS0ID_FILETYPE_OPEN, openMethodCallback},
        {UA_NS0ID_FILETYPE_CLOSE, closeMethodCallback},
        {UA_NS0ID_FILETYPE_READ, readMethodCallback},
        {UA_NS0ID_FILETYPE_WRITE, writeMethodCallback},
        {UA_NS0ID_FILETYPE_GETPOSITION, getPositionMethodCallback},
        {UA_NS0ID_FILETYPE_SETPOSITION, setPositionMethodCallback},
        {UA_NS0ID_FILEDIRECTORYTYPE_CREATEDIRECTORY, createDirectoryMethodCallback},
        {UA_NS0ID_FILEDIRECTORYTYPE_CREATEFILE, createFileMethodCallback},
        {UA_NS0ID_FILEDIRECTORYTYPE_DELETEFILESYSTEMOBJECT, deleteMethodCallback},
        {UA_NS0ID_FILEDIRECTORYTYPE_MOVEORCOPY, moveOrCopyMethodCallback}
    };

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); i++) {
        res |= UA_Server_setMethodNodeCallback(
            server, UA_NODEID_NUMERIC(0, methods[i].methodId), methods[i].callback);
    }
    return res;
}

/**************************************
 * Public Driver API
 **************************************/

static UA_Boolean
backendComplete(const UA_FileTransferBackend *b);

static FTMount *
newMount(FileTransferDriver *ftd, UA_FileTransferBackend backend,
         const UA_FileTransferMountOptions *options, UA_Boolean standaloneFile);

static void
removeMount(FileTransferDriver *ftd, FTMount *mount);

static void
closeMountHandles(UA_Server *server, FileTransferDriver *ftd, FTMount *mount);

/* Remove all registry entries of a mount */
static void
removeMountNodes(FileTransferDriver *ftd, FTMount *mount) {
    FTNode *node, *tmp;
    LIST_FOREACH_SAFE(node, &ftd->nodes, listEntry, tmp) {
        if(node->mount == mount)
            removeFTNode(ftd, node);
    }
}

static UA_StatusCode
addFileSystem(UA_FileTransferDriver *driver, const UA_NodeId requestedNodeId,
              const UA_NodeId parentNodeId, const UA_QualifiedName browseName,
              UA_FileTransferBackend backend,
              const UA_FileTransferMountOptions *options,
              UA_NodeId *outFileSystemNodeId) {
    FileTransferDriver *ftd = (FileTransferDriver*)driver;
    UA_Driver *drv = &driver->drv;

    /* The driver takes ownership of the backend. On failure the backend is
     * cleared before returning. */
    if(!backendComplete(&backend)) {
        if(backend.clear)
            backend.clear(&backend);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    if(drv->state != UA_LIFECYCLESTATE_STARTED || !drv->server) {
        if(backend.clear)
            backend.clear(&backend);
        return UA_STATUSCODE_BADINVALIDSTATE;
    }

    /* The backend root must be a directory */
    UA_FileTransferFileInfo info;
    UA_StatusCode res = backend.getAttributes(&backend, UA_STRING_NULL, &info);
    if(res == UA_STATUSCODE_GOOD && !info.isDirectory)
        res = UA_STATUSCODE_BADINVALIDARGUMENT;
    if(res != UA_STATUSCODE_GOOD) {
        if(backend.clear)
            backend.clear(&backend);
        return res;
    }

    FTMount *mount = newMount(ftd, backend, options, false);
    if(!mount) {
        if(backend.clear)
            backend.clear(&backend);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Part 20 mandates the BrowseName "FileSystem" for the root Object of an
     * exposed directory structure */
    UA_QualifiedName rootName = browseName;
    if(rootName.name.length == 0)
        rootName = UA_QUALIFIEDNAME(0, "FileSystem");

    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName.text = rootName.name;
    UA_NodeId rootNodeId = UA_NODEID_NULL;
    res = UA_Server_addObjectNode(drv->server, requestedNodeId, parentNodeId,
                                  UA_NS0ID(HASCOMPONENT), rootName,
                                  UA_NS0ID(FILEDIRECTORYTYPE), attr, NULL,
                                  &rootNodeId);
    if(res != UA_STATUSCODE_GOOD) {
        removeMount(ftd, mount);
        return res;
    }

    /* Mirror the backend content (eager scan) */
    FTNode *rootNode = newFTNode(ftd, mount, rootNodeId, UA_STRING_NULL, true);
    if(rootNode) {
        UA_UInt32 nodeBudget = (mount->options.maxNodes > 0) ?
            mount->options.maxNodes : (UA_UInt32)0xffffffffu;
        res = mirrorTree(drv->server, ftd, rootNode, 1, &nodeBudget);
    } else {
        res = UA_STATUSCODE_BADOUTOFMEMORY;
    }
    if(res == UA_STATUSCODE_GOOD)
        res = UA_NodeId_copy(&rootNodeId, &mount->rootNodeId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_Server_deleteNode(drv->server, rootNodeId, true);
        removeMountNodes(ftd, mount);
        removeMount(ftd, mount);
        UA_NodeId_clear(&rootNodeId);
        return res;
    }

    if(outFileSystemNodeId)
        *outFileSystemNodeId = rootNodeId;
    else
        UA_NodeId_clear(&rootNodeId);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
removeFileSystem(UA_FileTransferDriver *driver, const UA_NodeId fileSystemNodeId) {
    FileTransferDriver *ftd = (FileTransferDriver*)driver;
    UA_Driver *drv = &driver->drv;
    if(drv->state != UA_LIFECYCLESTATE_STARTED || !drv->server)
        return UA_STATUSCODE_BADINVALIDSTATE;

    FTMount *mount = NULL;
    LIST_FOREACH(mount, &ftd->mounts, listEntry) {
        if(!mount->standaloneFile &&
           UA_NodeId_equal(&mount->rootNodeId, &fileSystemNodeId))
            break;
    }
    if(!mount)
        return UA_STATUSCODE_BADNOTFOUND;

    closeMountHandles(drv->server, ftd, mount);
    /* Deleting the root removes the mirrored subtree recursively */
    UA_Server_deleteNode(drv->server, mount->rootNodeId, true);
    removeMountNodes(ftd, mount);
    removeMount(ftd, mount);
    return UA_STATUSCODE_GOOD;
}

static const UA_FileTransferMountOptions defaultMountOptions =
    {false, 0, 0, NULL, NULL};

/* Validate that a backend implements the mandatory operations */
static UA_Boolean
backendComplete(const UA_FileTransferBackend *b) {
    return b->openFile && b->closeFile && b->read && b->write &&
        b->getPosition && b->setPosition && b->getAttributes &&
        b->listDirectory && b->createFile && b->createDirectory &&
        b->remove && b->rename;
}

static FTMount *
newMount(FileTransferDriver *ftd, UA_FileTransferBackend backend,
         const UA_FileTransferMountOptions *options, UA_Boolean standaloneFile) {
    FTMount *mount = (FTMount*)UA_calloc(1, sizeof(FTMount));
    if(!mount)
        return NULL;
    mount->backend = backend;
    mount->options = options ? *options : defaultMountOptions;
    mount->standaloneFile = standaloneFile;
    LIST_INSERT_HEAD(&ftd->mounts, mount, listEntry);
    return mount;
}

static void
removeMount(FileTransferDriver *ftd, FTMount *mount) {
    if(mount->backend.clear)
        mount->backend.clear(&mount->backend);
    UA_NodeId_clear(&mount->rootNodeId);
    LIST_REMOVE(mount, listEntry);
    UA_free(mount);
}

static FTNode *
newFTNode(FileTransferDriver *ftd, FTMount *mount, const UA_NodeId nodeId,
          const UA_String path, UA_Boolean isDirectory) {
    FTNode *node = (FTNode*)UA_calloc(1, sizeof(FTNode));
    if(!node)
        return NULL;
    if(UA_NodeId_copy(&nodeId, &node->nodeId) != UA_STATUSCODE_GOOD ||
       UA_String_copy(&path, &node->path) != UA_STATUSCODE_GOOD) {
        UA_NodeId_clear(&node->nodeId);
        UA_String_clear(&node->path);
        UA_free(node);
        return NULL;
    }
    node->mount = mount;
    node->isDirectory = isDirectory;
    LIST_INSERT_HEAD(&ftd->nodes, node, listEntry);
    return node;
}

/* Close all handles that refer to files below the given mount */
static void
closeMountHandles(UA_Server *server, FileTransferDriver *ftd, FTMount *mount) {
    FTHandle *h, *tmp;
    LIST_FOREACH_SAFE(h, &ftd->handles, listEntry, tmp) {
        if(h->file->mount == mount)
            closeFTHandle(server, ftd, h);
    }
}

static UA_StatusCode
addFile(UA_FileTransferDriver *driver, const UA_NodeId requestedNodeId,
        const UA_NodeId parentNodeId, const UA_QualifiedName browseName,
        UA_FileTransferBackend backend, const UA_String path,
        const UA_FileTransferMountOptions *options, UA_NodeId *outFileNodeId) {
    FileTransferDriver *ftd = (FileTransferDriver*)driver;
    UA_Driver *drv = &driver->drv;

    /* The driver takes ownership of the backend. On failure the backend is
     * cleared before returning. */
    if(!backendComplete(&backend)) {
        if(backend.clear)
            backend.clear(&backend);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    if(drv->state != UA_LIFECYCLESTATE_STARTED || !drv->server) {
        if(backend.clear)
            backend.clear(&backend);
        return UA_STATUSCODE_BADINVALIDSTATE;
    }

    /* The backend file must exist */
    UA_FileTransferFileInfo info;
    UA_StatusCode res = backend.getAttributes(&backend, path, &info);
    if(res == UA_STATUSCODE_GOOD && info.isDirectory)
        res = UA_STATUSCODE_BADINVALIDARGUMENT;
    if(res != UA_STATUSCODE_GOOD) {
        if(backend.clear)
            backend.clear(&backend);
        return res;
    }

    FTMount *mount = newMount(ftd, backend, options, true);
    if(!mount) {
        if(backend.clear)
            backend.clear(&backend);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Create the FileType Object */
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName.text = browseName.name;
    UA_NodeId fileNodeId = UA_NODEID_NULL;
    res = UA_Server_addObjectNode(drv->server, requestedNodeId, parentNodeId,
                                  UA_NS0ID(HASCOMPONENT), browseName,
                                  UA_NS0ID(FILETYPE), attr, NULL, &fileNodeId);
    if(res != UA_STATUSCODE_GOOD) {
        removeMount(ftd, mount);
        return res;
    }

    FTNode *node = newFTNode(ftd, mount, fileNodeId, path, false);
    if(node)
        res = setupFileNode(drv->server, ftd, node, &info);
    else
        res = UA_STATUSCODE_BADOUTOFMEMORY;
    if(res == UA_STATUSCODE_GOOD)
        res = UA_NodeId_copy(&fileNodeId, &mount->rootNodeId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_Server_deleteNode(drv->server, fileNodeId, true);
        if(node)
            removeFTNode(ftd, node);
        removeMount(ftd, mount);
        UA_NodeId_clear(&fileNodeId);
        return res;
    }

    if(outFileNodeId)
        *outFileNodeId = fileNodeId;
    else
        UA_NodeId_clear(&fileNodeId);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
removeFile(UA_FileTransferDriver *driver, const UA_NodeId fileNodeId) {
    FileTransferDriver *ftd = (FileTransferDriver*)driver;
    UA_Driver *drv = &driver->drv;
    if(drv->state != UA_LIFECYCLESTATE_STARTED || !drv->server)
        return UA_STATUSCODE_BADINVALIDSTATE;

    FTNode *node = findFTNode(ftd, &fileNodeId);
    if(!node || !node->mount->standaloneFile)
        return UA_STATUSCODE_BADNOTFOUND;

    FTMount *mount = node->mount;
    closeMountHandles(drv->server, ftd, mount);
    /* Closing the last handle removes zombie nodes already */
    node = findFTNode(ftd, &fileNodeId);
    if(node) {
        UA_Server_deleteNode(drv->server, node->nodeId, true);
        removeFTNode(ftd, node);
    }
    removeMount(ftd, mount);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
refresh(UA_FileTransferDriver *driver, const UA_NodeId directoryNodeId) {
    FileTransferDriver *ftd = (FileTransferDriver*)driver;
    UA_Driver *drv = &driver->drv;
    if(drv->state != UA_LIFECYCLESTATE_STARTED || !drv->server)
        return UA_STATUSCODE_BADINVALIDSTATE;

    FTNode *dirNode = findFTNode(ftd, &directoryNodeId);
    if(!dirNode || !dirNode->isDirectory || dirNode->zombie)
        return UA_STATUSCODE_BADNOTFOUND;

    UA_UInt32 nodeBudget = (UA_UInt32)0xffffffffu;
    const UA_FileTransferMountOptions *opts = &dirNode->mount->options;
    if(opts->maxNodes > 0) {
        UA_UInt32 current = countMountNodes(ftd, dirNode->mount);
        nodeBudget = (opts->maxNodes > current) ? opts->maxNodes - current : 0;
    }
    return syncTree(drv->server, ftd, dirNode, pathDepth(dirNode->path) + 1,
                    &nodeBudget);
}

/**************************************
 * Lifecycle and Constructor
 **************************************/

static UA_StatusCode
FileTransferDriver_start(UA_Driver *drv) {
    if(!drv->server)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(drv->state != UA_LIFECYCLESTATE_STOPPED)
        return UA_STATUSCODE_BADINTERNALERROR;

    FileTransferDriver *ftd = (FileTransferDriver*)drv;
    if(!ftd->logging)
        ftd->logging = UA_Server_getConfig(drv->server)->logging;

    for(UA_Driver *existing = UA_Server_getDrivers(drv->server);
        existing; existing = existing->next) {
        if(existing == drv ||
           existing->server != drv->server ||
           existing->state != UA_LIFECYCLESTATE_STARTED ||
           !isFileTransferDriver(existing))
            continue;
        UA_LOG_ERROR(ftd->logging, UA_LOGCATEGORY_SERVER,
                     "Cannot add the driver \"%S\". The file transfer "
                     "driver is already loaded", drv->name);
        return UA_STATUSCODE_BADALREADYEXISTS;
    }

    /* Verify that the FileType node is present. It is only part of the full
     * Namespace Zero. */
    UA_QualifiedName fileTypeName;
    UA_StatusCode res = UA_Server_readBrowseName(
        drv->server, UA_NS0ID(FILETYPE), &fileTypeName);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(ftd->logging, UA_LOGCATEGORY_SERVER,
                     "Cannot start the driver \"%S\". The FileType node is "
                     "not present in the address space. The file transfer "
                     "driver requires the full Namespace Zero", drv->name);
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }
    UA_QualifiedName_clear(&fileTypeName);

    res = registerFileTransferMethodCallbacks(drv->server);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(ftd->logging, UA_LOGCATEGORY_SERVER,
                     "Cannot start the driver \"%S\". Registering the "
                     "FileType/FileDirectoryType method callbacks failed "
                     "with %s", drv->name, UA_StatusCode_name(res));
        return res;
    }

    drv->state = UA_LIFECYCLESTATE_STARTED;
    return UA_STATUSCODE_GOOD;
}

static void
FileTransferDriver_stop(UA_Driver *drv) {
    FileTransferDriver *ftd = (FileTransferDriver*)drv;

    /* Close all open file handles. The mounts and the mirrored nodes are
     * kept so the driver can be restarted. */
    FTHandle *h, *tmp;
    LIST_FOREACH_SAFE(h, &ftd->handles, listEntry, tmp) {
        closeFTHandle(drv->server, ftd, h);
    }

    drv->state = UA_LIFECYCLESTATE_STOPPED;
}

static UA_StatusCode
FileTransferDriver_free(UA_Driver *drv) {
    if(drv->state != UA_LIFECYCLESTATE_STOPPED)
        return UA_STATUSCODE_BADINTERNALERROR;

    FileTransferDriver *ftd = (FileTransferDriver*)drv;

    /* All handles are closed during stop */
    UA_assert(LIST_EMPTY(&ftd->handles));

    FTNode *node, *nodeTmp;
    LIST_FOREACH_SAFE(node, &ftd->nodes, listEntry, nodeTmp) {
        removeFTNode(ftd, node);
    }

    FTMount *mount, *mountTmp;
    LIST_FOREACH_SAFE(mount, &ftd->mounts, listEntry, mountTmp) {
        removeMount(ftd, mount);
    }

    UA_KeyValueMap_clear(&drv->params);
    UA_free(ftd);
    return UA_STATUSCODE_GOOD;
}

UA_FileTransferDriver *
UA_FileTransferDriver_new(const UA_KeyValueMap params) {
    FileTransferDriver *ftd =
        (FileTransferDriver*)UA_calloc(1, sizeof(FileTransferDriver));
    if(!ftd)
        return NULL;

    UA_FileTransferDriver *driver = &ftd->driver;
    UA_Driver *base = &driver->drv;

    UA_StatusCode res = UA_KeyValueMap_copy(&params, &base->params);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(ftd);
        return NULL;
    }

    ftd->maxHandlesPerSession = UA_FILETRANSFER_MAXHANDLESPERSESSION_DEFAULT;
    const UA_UInt16 *maxPerSession = (const UA_UInt16*)
        UA_KeyValueMap_getScalar(&params,
                                 UA_QUALIFIEDNAME(0, "max-open-handles-per-session"),
                                 &UA_TYPES[UA_TYPES_UINT16]);
    if(maxPerSession)
        ftd->maxHandlesPerSession = *maxPerSession;

    ftd->maxHandlesPerFile = UA_FILETRANSFER_MAXHANDLESPERFILE_DEFAULT;
    const UA_UInt16 *maxPerFile = (const UA_UInt16*)
        UA_KeyValueMap_getScalar(&params,
                                 UA_QUALIFIEDNAME(0, "max-open-handles-per-file"),
                                 &UA_TYPES[UA_TYPES_UINT16]);
    if(maxPerFile)
        ftd->maxHandlesPerFile = *maxPerFile;

    ftd->maxReadLength = UA_FILETRANSFER_MAXREADLENGTH_DEFAULT;
    const UA_UInt32 *maxReadLength = (const UA_UInt32*)
        UA_KeyValueMap_getScalar(&params,
                                 UA_QUALIFIEDNAME(0, "max-read-length"),
                                 &UA_TYPES[UA_TYPES_UINT32]);
    if(maxReadLength && *maxReadLength > 0)
        ftd->maxReadLength = *maxReadLength;

    LIST_INIT(&ftd->mounts);
    LIST_INIT(&ftd->nodes);
    LIST_INIT(&ftd->handles);

    base->name = UA_STRING(UA_DRIVER_FILE_TRANSFER_NAME);
    base->notificationCallback = FileTransferDriver_notification;
    base->notificationFilter = UA_APPLICATIONNOTIFICATIONTYPE_SESSION;
    base->start = FileTransferDriver_start;
    base->stop = FileTransferDriver_stop;
    base->free = FileTransferDriver_free;

    driver->addFileSystem = addFileSystem;
    driver->removeFileSystem = removeFileSystem;
    driver->addFile = addFile;
    driver->removeFile = removeFile;
    driver->refresh = refresh;
    return driver;
}

#endif /* UA_ENABLE_METHODCALLS && UA_GENERATED_NAMESPACE_ZERO_FULL */
