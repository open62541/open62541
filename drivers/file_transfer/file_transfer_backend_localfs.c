/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include "file_transfer_internal.h"

/* The backend accesses the files through a small OS layer that takes UTF-8
 * paths. On Windows, the wide-character API of the C runtime is used, so that
 * every name can be represented independent of the active code page, and the
 * positions are 64 bit. On POSIX, the paths are passed as they are. The POSIX
 * layer is only used where the POSIX EventLoop is compiled in: the
 * "posix-lwip" architecture sets both UA_ARCHITECTURE_POSIX and
 * UA_ARCHITECTURE_LWIP. Architectures with neither (lwip, freertos-lwip,
 * zephyr) fall through to the UA_STATUSCODE_BADNOTSUPPORTED stub at the
 * bottom of this file. */
#if defined(UA_ARCHITECTURE_WIN32)
# define UA_FILETRANSFER_LOCALBACKEND
# include <direct.h>
# include <errno.h>
# include <fcntl.h>
# include <io.h>
# include <stdio.h>
# include <sys/stat.h>
# include "tr_dirent.h"
# include "mp_printf.h"

/* A UTF-16 code unit takes at most three UTF-8 bytes */
# define FT_PATH_MAX (MAX_PATH * 3)
# define FT_MODE(m) L##m
# define FT_ISDIR(mode) (((mode) & _S_IFMT) == _S_IFDIR)
# define FT_ISREG(mode) (((mode) & _S_IFMT) == _S_IFREG)
# define ftSeek _fseeki64
# define ftTell _ftelli64
typedef __int64 FtOffset;
typedef struct _stat64 FtStat;
typedef wchar_t FtChar;

/* Convert the UTF-8 path to UTF-16. A path that cannot be converted is
 * reported as too long. */
static UA_Boolean
ftWiden(const char *path, wchar_t *out) {
    if(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1,
                           out, MAX_PATH) > 0)
        return true;
    errno = ENAMETOOLONG;
    return false;
}

static int
ftStat(const char *path, FtStat *st) {
    wchar_t wpath[MAX_PATH];
    return ftWiden(path, wpath) ? _wstat64(wpath, st) : -1;
}

/* The C runtime follows symbolic links and junctions. Removing one with
 * _wrmdir/_wremove removes the link itself. */
static UA_Boolean
ftIsLink(const char *path) {
    wchar_t wpath[MAX_PATH];
    if(!ftWiden(path, wpath))
        return false;
    DWORD attr = GetFileAttributesW(wpath);
    return attr != INVALID_FILE_ATTRIBUTES &&
        (attr & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}

static FILE *
ftOpen(const char *path, const FtChar *mode) {
    wchar_t wpath[MAX_PATH];
    return ftWiden(path, wpath) ? _wfopen(wpath, mode) : NULL;
}

static int
ftCreateExclusive(const char *path) {
    wchar_t wpath[MAX_PATH];
    if(!ftWiden(path, wpath))
        return -1;
    int fd = _wopen(wpath, _O_WRONLY | _O_CREAT | _O_EXCL | _O_BINARY,
                    _S_IREAD | _S_IWRITE);
    if(fd < 0)
        return -1;
    _close(fd);
    return 0;
}

static int
ftMkdir(const char *path) {
    wchar_t wpath[MAX_PATH];
    return ftWiden(path, wpath) ? _wmkdir(wpath) : -1;
}

/* _wremove removes a file, _wrmdir a directory or a junction */
static int
ftRemoveEntry(const char *path) {
    wchar_t wpath[MAX_PATH];
    if(!ftWiden(path, wpath))
        return -1;
    if(_wremove(wpath) == 0)
        return 0;
    int err = errno;
    if(_wrmdir(wpath) == 0)
        return 0;
    if(errno == ENOTDIR)
        errno = err; /* Not a directory: the error of _wremove */
    return -1;
}

/* Without MOVEFILE_REPLACE_EXISTING, an existing entry is never replaced */
static int
ftRenameNoReplace(const char *from, const char *to) {
    wchar_t wfrom[MAX_PATH];
    wchar_t wto[MAX_PATH];
    if(!ftWiden(from, wfrom) || !ftWiden(to, wto))
        return -1;
    if(MoveFileExW(wfrom, wto, 0))
        return 0;
    DWORD err = GetLastError();
    if(err == ERROR_ALREADY_EXISTS || err == ERROR_FILE_EXISTS)
        errno = EEXIST;
    else if(err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND)
        errno = ENOENT;
    else if(err == ERROR_ACCESS_DENIED || err == ERROR_SHARING_VIOLATION)
        errno = EACCES;
    else
        errno = EINVAL;
    return -1;
}

static UA_FileAccessRights
ftAccessRights(const char *path, UA_Boolean isDirectory) {
    wchar_t wpath[MAX_PATH];
    if(!ftWiden(path, wpath))
        return 0;
    UA_FileAccessRights rights = 0;
    if(_waccess(wpath, 4 /* R_OK */) == 0)
        rights |= UA_FILEACCESS_READ;
    if(_waccess(wpath, 2 /* W_OK */) == 0)
        rights |= UA_FILEACCESS_WRITE;
    /* The Windows CRT has no X_OK. Directory traversal is checked by the
     * actual filesystem operations and can be restricted by the callbacks. */
    if(isDirectory)
        rights |= UA_FILEACCESS_TRAVERSE;
    return rights;
}

/* Windows cannot store every name as given: the device names (CON, PRN, AUX,
 * NUL, COM1-9, LPT1-9, also with an extension), the characters <>:"|?* (':'
 * would name an alternate data stream) and a trailing dot or space, which is
 * stripped ("a." would name "a"). */
static UA_Boolean
ftValidName(const UA_String name) {
    for(size_t i = 0; i < name.length; i++) {
        UA_Byte c = name.data[i];
        if(c < 32 || c == '<' || c == '>' || c == ':' || c == '"' ||
           c == '|' || c == '?' || c == '*')
            return false;
    }
    UA_Byte last = name.data[name.length - 1];
    if(last == '.' || last == ' ')
        return false;

    /* The device names are matched case-insensitive before the first dot,
     * trailing spaces ignored */
    size_t baseLen = 0;
    while(baseLen < name.length && name.data[baseLen] != '.')
        baseLen++;
    while(baseLen > 0 && name.data[baseLen - 1] == ' ')
        baseLen--;
    if(baseLen != 3 && baseLen != 4)
        return true;
    char base[5];
    for(size_t i = 0; i < baseLen; i++) {
        char c = (char)name.data[i];
        base[i] = (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c;
    }
    base[baseLen] = 0;
    if(baseLen == 3)
        return strcmp(base, "CON") != 0 && strcmp(base, "PRN") != 0 &&
            strcmp(base, "AUX") != 0 && strcmp(base, "NUL") != 0;
    if(base[3] < '1' || base[3] > '9')
        return true;
    base[3] = 0;
    return strcmp(base, "COM") != 0 && strcmp(base, "LPT") != 0;
}

#elif defined(UA_ARCHITECTURE_POSIX) && !defined(UA_ARCHITECTURE_LWIP)
# define UA_FILETRANSFER_LOCALBACKEND
# include <dirent.h>
# include <errno.h>
# include <fcntl.h>
# include <limits.h>
# include <stdio.h>
# include <sys/stat.h>
# include <unistd.h>
# include "mp_printf.h"

# ifdef PATH_MAX
#  define FT_PATH_MAX PATH_MAX
# else
#  define FT_PATH_MAX 4096
# endif
# define FT_MODE(m) m
# define FT_ISDIR(mode) S_ISDIR(mode)
# define FT_ISREG(mode) S_ISREG(mode)
/* Positions are limited by long: 64 bit on the 64-bit systems */
# define ftSeek fseek
# define ftTell ftell
# define ftStat stat
# define ftOpen fopen
# define ftMkdir(path) mkdir(path, 0755)
/* remove() takes a file, an empty directory or a symbolic link itself */
# define ftRemoveEntry remove
static UA_FileAccessRights
ftAccessRights(const char *path, UA_Boolean isDirectory) {
    UA_FileAccessRights rights = 0;
    if(access(path, R_OK) == 0)
        rights |= UA_FILEACCESS_READ;
    if(access(path, W_OK) == 0)
        rights |= UA_FILEACCESS_WRITE;
    if(isDirectory && access(path, X_OK) == 0)
        rights |= UA_FILEACCESS_TRAVERSE;
    return rights;
}

# define ftValidName(name) true
typedef long FtOffset;
typedef struct stat FtStat;
typedef char FtChar;

static int
ftCreateExclusive(const char *path) {
    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if(fd < 0)
        return -1;
    close(fd);
    return 0;
}

static UA_Boolean
ftIsLink(const char *path) {
    struct stat st;
    return lstat(path, &st) == 0 && S_ISLNK(st.st_mode);
}

/* Move without replacing an existing entry. Linux and macOS rename atomically
 * without replacing. Otherwise (other systems, filesystems without the flag)
 * the target name is reserved with an exclusively created placeholder, which
 * the rename then replaces. A file placeholder is tried first; renaming a
 * directory onto it fails with ENOTDIR, then the placeholder is a directory. */
static int
ftRenameNoReplace(const char *from, const char *to) {
#if defined(__linux__) && defined(RENAME_NOREPLACE)
    if(renameat2(AT_FDCWD, from, AT_FDCWD, to, RENAME_NOREPLACE) == 0)
        return 0;
    if(errno != EINVAL && errno != ENOSYS)
        return -1;
#elif defined(__APPLE__) && defined(RENAME_EXCL)
    if(renamex_np(from, to, RENAME_EXCL) == 0)
        return 0;
    if(errno != ENOTSUP)
        return -1;
#endif
    if(ftCreateExclusive(to) != 0)
        return -1;
    if(rename(from, to) == 0)
        return 0;
    int err = errno;
    unlink(to);
    if(err != ENOTDIR) {
        errno = err;
        return -1;
    }
    if(mkdir(to, 0700) != 0)
        return -1;
    if(rename(from, to) == 0)
        return 0;
    err = errno;
    rmdir(to);
    errno = err;
    return -1;
}
#endif

/**************************************
 * Local Filesystem Backend
 **************************************/

#ifdef UA_FILETRANSFER_LOCALBACKEND

/* The open files are kept in a table. A handle is the index + 1. */
typedef struct {
    UA_String rootPath;
    UA_Boolean singleFile; /* Only the empty backend path is accepted */
    FILE **files;
    size_t filesSize;
} LocalFileSystemContext;

static UA_StatusCode
errnoToStatusCode(int err) {
    switch(err) {
    case ENOENT: return UA_STATUSCODE_BADNOTFOUND;
    case EACCES:
    case EPERM:  return UA_STATUSCODE_BADUSERACCESSDENIED;
    case EEXIST: return UA_STATUSCODE_BADBROWSENAMEDUPLICATED;
    case ENOMEM: return UA_STATUSCODE_BADOUTOFMEMORY;
    case ENOTDIR:
#ifdef ELOOP
    case ELOOP:
#endif
        return UA_STATUSCODE_BADNOTFOUND;
    case ENAMETOOLONG: return UA_STATUSCODE_BADINVALIDARGUMENT;
    case ENOSPC:
#ifdef EDQUOT
    case EDQUOT:
#endif
        return UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
    case EROFS: return UA_STATUSCODE_BADNOTWRITABLE;
#ifdef ENOTEMPTY
    case ENOTEMPTY: return UA_STATUSCODE_BADINVALIDSTATE;
#endif
    default: return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
}

static FILE *
lookupFile(UA_FileTransferFileBackend *b, UA_UInt32 handle) {
    LocalFileSystemContext *ctx = (LocalFileSystemContext*)b->context;
    if(handle == 0 || handle > ctx->filesSize)
        return NULL;
    return ctx->files[handle - 1];
}

static UA_StatusCode
storeFile(LocalFileSystemContext *ctx, FILE *fp, UA_UInt32 *handle) {
    size_t i = 0;
    while(i < ctx->filesSize && ctx->files[i])
        i++;
    if(i == ctx->filesSize) {
        size_t newSize = (ctx->filesSize > 0) ? ctx->filesSize * 2 : 8;
        if(newSize > UA_UINT32_MAX)
            return UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
        FILE **files = (FILE**)UA_realloc(ctx->files, newSize * sizeof(FILE*));
        if(!files)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        for(size_t j = ctx->filesSize; j < newSize; j++)
            files[j] = NULL;
        ctx->files = files;
        ctx->filesSize = newSize;
    }
    ctx->files[i] = fp;
    *handle = (UA_UInt32)(i + 1);
    return UA_STATUSCODE_GOOD;
}

/* Only well-formed relative paths reach the storage: no leading, trailing or
 * double slashes, no "." or ".." segments, no backslashes or NUL bytes. This
 * blocks path traversal (naming a target outside rootPath via "..") but does
 * not sandbox against symlinks: a symlink to a file inside rootPath may
 * resolve to a target outside it, and the OS follows it. Each '/'-separated
 * segment is validated with validEntryName so the traversal-safety policy
 * lives in a single place. On Windows, the names the filesystem cannot store
 * as given are rejected as well. */
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
        if(segment.length > UA_FILETRANSFER_FILENAME_MAX)
            return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
        if(!validEntryName(segment) || !ftValidName(segment))
            return UA_STATUSCODE_BADINVALIDARGUMENT;
        segStart = i + 1;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
buildLocalPath(const LocalFileSystemContext *ctx, const UA_String relPath,
               char *out) {
    if(ctx->singleFile && relPath.length != 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_StatusCode res = checkRelativePath(relPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* A root that ends with a separator ("/", "C:\") gets no second one */
    const UA_String *root = &ctx->rootPath;
    UA_Byte last = root->data[root->length - 1];
    const char *sep = (last == '/' || last == '\\') ? "" : "/";
    int len;
    if(relPath.length == 0)
        len = mp_snprintf(out, FT_PATH_MAX, "%.*s",
                          (int)root->length, (char*)root->data);
    else
        len = mp_snprintf(out, FT_PATH_MAX, "%.*s%s%.*s",
                          (int)root->length, (char*)root->data, sep,
                          (int)relPath.length, (char*)relPath.data);
    if(len < 0 || len >= FT_PATH_MAX)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
localFsOpen(UA_FileTransferFileBackend *b, const UA_String path,
            UA_Byte mode, UA_UInt32 *handle) {
    LocalFileSystemContext *ctx = (LocalFileSystemContext*)b->context;
    char localPath[FT_PATH_MAX];
    UA_StatusCode res = buildLocalPath(ctx, path, localPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_Boolean readBit = (mode & UA_OPENFILEMODE_READ) != 0;
    UA_Boolean writeBit = (mode & UA_OPENFILEMODE_WRITE) != 0;
    UA_Boolean eraseBit = (mode & UA_OPENFILEMODE_ERASEEXISTING) != 0;
    UA_Boolean appendBit = (mode & UA_OPENFILEMODE_APPEND) != 0;

    /* "r+b" opens an existing file for writing without truncation. "wb" /
     * "w+b" truncate for the EraseExisting semantics. The driver already
     * validates the mode bit combinations. */
    const FtChar *fmode;
    if(writeBit)
        fmode = eraseBit ? (readBit ? FT_MODE("w+b") : FT_MODE("wb")) :
            FT_MODE("r+b");
    else if(readBit)
        fmode = FT_MODE("rb");
    else
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    errno = 0;
    FILE *fp = ftOpen(localPath, fmode);
    if(!fp)
        return errnoToStatusCode(errno);

    if(appendBit && ftSeek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

    res = storeFile(ctx, fp, handle);
    if(res != UA_STATUSCODE_GOOD)
        fclose(fp);
    return res;
}

static UA_StatusCode
localFsClose(UA_FileTransferFileBackend *b, UA_UInt32 handle) {
    FILE *fp = lookupFile(b, handle);
    if(!fp)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    LocalFileSystemContext *ctx = (LocalFileSystemContext*)b->context;
    ctx->files[handle - 1] = NULL;
    if(fclose(fp) != 0)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
localFsRead(UA_FileTransferFileBackend *b, UA_UInt32 handle,
            UA_Int32 length, UA_ByteString *out) {
    FILE *fp = lookupFile(b, handle);
    if(!fp || length <= 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode res = UA_ByteString_allocBuffer(out, (size_t)length);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Update streams require a positioning call between switching from
     * writing to reading (C99 7.19.5.3) */
    ftSeek(fp, 0, SEEK_CUR);
    size_t bytesRead = fread(out->data, 1, (size_t)length, fp);
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
localFsWrite(UA_FileTransferFileBackend *b, UA_UInt32 handle,
             const UA_ByteString data) {
    FILE *fp = lookupFile(b, handle);
    if(!fp)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(data.length == 0)
        return UA_STATUSCODE_GOOD;

    /* Update streams require a positioning call between switching from
     * reading to writing (C99 7.19.5.3) */
    ftSeek(fp, 0, SEEK_CUR);

    /* Flushed right away, so that a full disk fails this Write and not only
     * the later Close */
    errno = 0;
    size_t written = fwrite(data.data, 1, data.length, fp);
    if(written != data.length || fflush(fp) != 0)
        return (errno != 0) ? errnoToStatusCode(errno) :
            UA_STATUSCODE_BADUNEXPECTEDERROR;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
localFsGetPosition(UA_FileTransferFileBackend *b, UA_UInt32 handle,
                   UA_UInt64 *outPosition) {
    FILE *fp = lookupFile(b, handle);
    if(!fp)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    FtOffset pos = ftTell(fp);
    if(pos < 0)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    *outPosition = (UA_UInt64)pos;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
localFsSetPosition(UA_FileTransferFileBackend *b, UA_UInt32 handle,
                   UA_UInt64 position) {
    FILE *fp = lookupFile(b, handle);
    if(!fp)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Positions beyond the end of the file are clamped to the file size */
    if(ftSeek(fp, 0, SEEK_END) != 0)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    FtOffset size = ftTell(fp);
    if(size < 0)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    FtOffset target = (position < (UA_UInt64)size) ? (FtOffset)position : size;
    if(ftSeek(fp, target, SEEK_SET) != 0)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    return UA_STATUSCODE_GOOD;
}

/* Guess the media type from the filename extension. Returns NULL for unknown
 * extensions. */
static const char *
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
        return NULL;
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
            return table[i].mime;
    }
    return NULL;
}

/* Shared by getInfo and directory listing, which already obtained stat. */
static UA_StatusCode
localFsFileInfo(const char *localPath, const UA_String name, const FtStat *st,
                UA_FileTransferFileInfo *outInfo) {
    if(name.length > UA_FILETRANSFER_FILENAME_MAX)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    memset(outInfo, 0, sizeof(*outInfo));
    if(name.length > 0)
        memcpy(outInfo->name, name.data, name.length);
    outInfo->size = (UA_UInt64)st->st_size;
    outInfo->lastModified = UA_DATETIME_UNIX_EPOCH +
        (UA_DateTime)st->st_mtime * UA_DATETIME_SEC;
    outInfo->isDirectory = FT_ISDIR(st->st_mode);
    outInfo->accessRights = ftAccessRights(localPath, outInfo->isDirectory);
    const char *mime = (outInfo->isDirectory) ? NULL : localFsMimeType(name);
    if(mime)
        memcpy(outInfo->mimeType, mime, strlen(mime)); /* Zero-padded */
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
localFsGetInfo(UA_FileTransferFileBackend *b, const UA_String path,
               UA_FileTransferFileInfo *outInfo) {
    LocalFileSystemContext *ctx = (LocalFileSystemContext*)b->context;
    char localPath[FT_PATH_MAX];
    UA_StatusCode res = buildLocalPath(ctx, path, localPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_String name = ctx->singleFile ? ctx->rootPath : path;
    for(size_t i = name.length; i > 0; i--) {
        if(name.data[i - 1] == '/'
#ifdef UA_ARCHITECTURE_WIN32
           || name.data[i - 1] == '\\' || (i == 2 && name.data[1] == ':')
#endif
        ) {
            name.data += i;
            name.length -= i;
            break;
        }
    }
    errno = 0;
    FtStat st;
    if(ftStat(localPath, &st) != 0)
        return errnoToStatusCode(errno);
    if(ctx->singleFile && !FT_ISREG(st.st_mode))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    return localFsFileInfo(localPath, name, &st, outInfo);
}

/* Report one directory entry. A symbolic link to a file is served as the
 * file. A link to a directory is not served: deleting or copying the tree
 * would descend into the link target, and link cycles would make the scan
 * recurse without bound. Special files (FIFOs, sockets, devices) are not
 * served either, opening them can block the server. */
static UA_StatusCode
listEntry(const char *dirPath, const char *name,
          UA_FileTransferListCallback cb, void *listContext) {
    if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
        return UA_STATUSCODE_GOOD;
    if(strlen(name) > UA_FILETRANSFER_FILENAME_MAX)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;

    /* Not all filesystems report the entry type in the dirent */
    char entryPath[FT_PATH_MAX];
    int len = mp_snprintf(entryPath, FT_PATH_MAX, "%s/%s", dirPath, name);
    if(len < 0 || len >= FT_PATH_MAX)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    FtStat st;
    if(ftStat(entryPath, &st) != 0)
        return UA_STATUSCODE_GOOD;
    if(FT_ISDIR(st.st_mode) && ftIsLink(entryPath))
        return UA_STATUSCODE_GOOD;
    if(!FT_ISDIR(st.st_mode) && !FT_ISREG(st.st_mode))
        return UA_STATUSCODE_GOOD;
    UA_FileTransferFileInfo info;
    UA_StatusCode res = localFsFileInfo(entryPath, UA_STRING((char*)(uintptr_t)name),
                                       &st, &info);
    if(res == UA_STATUSCODE_GOOD)
        cb(listContext, &info);
    return res;
}

static UA_StatusCode
localFsListDirectory(UA_FileTransferBackend *b, const UA_String path,
                     UA_FileTransferListCallback cb, void *listContext) {
    char localPath[FT_PATH_MAX];
    UA_StatusCode res = buildLocalPath((LocalFileSystemContext*)b->file.context,
                                       path, localPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;

#if defined(UA_ARCHITECTURE_WIN32)
    /* The names are converted back to UTF-8 */
    wchar_t wpath[MAX_PATH];
    if(!ftWiden(localPath, wpath))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    errno = 0;
    _WDIR *dir = _wopendir(wpath);
    if(!dir)
        return errnoToStatusCode(errno);
    struct _wdirent *entry;
    while((entry = _wreaddir(dir)) != NULL) {
        char name[FT_PATH_MAX];
        if(WideCharToMultiByte(CP_UTF8, 0, entry->d_name, -1, name,
                               (int)sizeof(name), NULL, NULL) > 0)
            res = listEntry(localPath, name, cb, listContext);
        else
            res = UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
        if(res != UA_STATUSCODE_GOOD)
            break;
    }
    _wclosedir(dir);
#else
    errno = 0;
    DIR *dir = opendir(localPath);
    if(!dir)
        return errnoToStatusCode(errno);
    struct dirent *entry;
    while((entry = readdir(dir)) != NULL) {
        res = listEntry(localPath, entry->d_name, cb, listContext);
        if(res != UA_STATUSCODE_GOOD)
            break;
    }
    closedir(dir);
#endif
    return res;
}

static UA_StatusCode
localFsCreate(UA_FileTransferBackend *b, const UA_String path,
               const UA_FileTransferFileInfo *info) {
    char localPath[FT_PATH_MAX];
    UA_StatusCode res = buildLocalPath((LocalFileSystemContext*)b->file.context,
                                       path, localPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Both operations fail if an entry already exists. */
    errno = 0;
    int ret = info->isDirectory ? ftMkdir(localPath) : ftCreateExclusive(localPath);
    return (ret == 0) ? UA_STATUSCODE_GOOD : errnoToStatusCode(errno);
}

static UA_StatusCode
localFsRemove(UA_FileTransferBackend *b, const UA_String path) {
    char localPath[FT_PATH_MAX];
    UA_StatusCode res = buildLocalPath((LocalFileSystemContext*)b->file.context,
                                       path, localPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* A symbolic link is removed itself and not its target. Checking the kind
     * of the entry first would race with a concurrent change. */
    errno = 0;
    if(ftRemoveEntry(localPath) != 0)
        return errnoToStatusCode(errno);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
localFsRename(UA_FileTransferBackend *b, const UA_String fromPath,
              const UA_String toPath) {
    char localFrom[FT_PATH_MAX];
    char localTo[FT_PATH_MAX];
    LocalFileSystemContext *ctx = (LocalFileSystemContext*)b->file.context;
    UA_StatusCode res = buildLocalPath(ctx, fromPath, localFrom);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = buildLocalPath(ctx, toPath, localTo);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* rename(2) replaces an existing target without warning. The driver only
     * knows about entries it mirrored, so an entry created out-of-band or
     * below maxScanDepth would be destroyed silently. EEXIST maps to
     * Bad_BrowseNameDuplicated. */
    errno = 0;
    if(ftRenameNoReplace(localFrom, localTo) != 0)
        return errnoToStatusCode(errno);
    return UA_STATUSCODE_GOOD;
}

static void
localFsClear(UA_FileTransferFileBackend *b) {
    LocalFileSystemContext *ctx = (LocalFileSystemContext*)b->context;
    if(!ctx)
        return;
    for(size_t i = 0; i < ctx->filesSize; i++) {
        if(ctx->files[i])
            fclose(ctx->files[i]);
    }
    UA_free(ctx->files);
    UA_String_clear(&ctx->rootPath);
    UA_free(ctx);
    b->context = NULL;
}

static UA_StatusCode
localFsBackend(const UA_String rootPath, UA_Boolean singleFile,
                UA_FileTransferFileBackend *out) {
    if(!out || rootPath.length == 0 || rootPath.length >= FT_PATH_MAX ||
       !rootPath.data || memchr(rootPath.data, 0, rootPath.length))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* The served file or directory must exist. */
    char localPath[FT_PATH_MAX];
    mp_snprintf(localPath, FT_PATH_MAX, "%.*s",
                (int)rootPath.length, (char*)rootPath.data);
    errno = 0;
    FtStat st;
    if(ftStat(localPath, &st) != 0)
        return errnoToStatusCode(errno);
    if(singleFile ? !FT_ISREG(st.st_mode) : !FT_ISDIR(st.st_mode))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    LocalFileSystemContext *ctx = (LocalFileSystemContext*)
        UA_calloc(1, sizeof(LocalFileSystemContext));
    if(!ctx)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Copy the root path without trailing slashes. The separator of a
     * Windows drive root ("C:\") is kept, "C:" names the current directory
     * of the drive. */
    UA_String root = rootPath;
    while(!singleFile && root.length > 1 &&
          (root.data[root.length - 1] == '/' ||
           root.data[root.length - 1] == '\\') &&
          !(root.length == 3 && root.data[1] == ':'))
        root.length--;
    UA_StatusCode res = UA_String_copy(&root, &ctx->rootPath);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(ctx);
        return res;
    }

    ctx->singleFile = singleFile;
    memset(out, 0, sizeof(*out));
    out->context = ctx;
    out->open = localFsOpen;
    out->close = localFsClose;
    out->read = localFsRead;
    out->write = localFsWrite;
    out->getPosition = localFsGetPosition;
    out->setPosition = localFsSetPosition;
    out->getInfo = localFsGetInfo;
    out->clear = localFsClear;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_FileTransferBackend_localDirectory(const UA_String rootPath,
                                      UA_FileTransferBackend *out) {
    if(!out)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_FileTransferFileBackend file;
    UA_StatusCode res = localFsBackend(rootPath, false, &file);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    memset(out, 0, sizeof(*out));
    out->file = file;
    out->listDirectory = localFsListDirectory;
    out->create = localFsCreate;
    out->remove = localFsRemove;
    out->rename = localFsRename;
    out->copy = NULL; /* Emulated by the driver */
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_FileTransferFileBackend_localFile(const UA_String filePath,
                                     UA_FileTransferFileBackend *out) {
    return localFsBackend(filePath, true, out);
}

#else /* UA_FILETRANSFER_LOCALBACKEND */

UA_StatusCode
UA_FileTransferBackend_localDirectory(const UA_String rootPath,
                                      UA_FileTransferBackend *out) {
    return UA_STATUSCODE_BADNOTSUPPORTED;
}

UA_StatusCode
UA_FileTransferFileBackend_localFile(const UA_String filePath,
                                     UA_FileTransferFileBackend *out) {
    return UA_STATUSCODE_BADNOTSUPPORTED;
}

#endif /* UA_FILETRANSFER_LOCALBACKEND */
