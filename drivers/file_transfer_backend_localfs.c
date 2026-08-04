/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include "file_transfer_internal.h"

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
 * blocks path traversal (naming a target outside rootPath via "..") but does
 * not sandbox against symlinks: a symlink inside rootPath may resolve to a
 * target outside it, and the OS follows it. Each '/'-separated segment is
 * validated with validEntryName so the traversal-safety policy lives in a
 * single place. */
static UA_StatusCode
checkRelativePath(const UA_String path) {
    if(path.length == 0)
        return UA_STATUSCODE_GOOD; /* The mount root */

    size_t segStart = 0;
    for(size_t i = 0; i <= path.length; i++) {
        if(i < path.length && path.data[i] != '/')
            continue;
        UA_String segment;
        segment.length = i - segStart;
        segment.data = path.data + segStart;
        if(!validEntryName(segment))
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

    /* Reset errno right before the mutating call; the stat above may have
     * set it and the remove/rmdir result must be diagnosed from its own
     * errno. */
    errno = 0;
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
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = buildLocalPath(ctx, toPath, localTo);
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

#endif /* UA_ENABLE_METHODCALLS && UA_GENERATED_NAMESPACE_ZERO_FULL */