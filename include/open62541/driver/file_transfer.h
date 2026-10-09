/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#ifndef UA_DRIVER_FILE_TRANSFER_H_
#define UA_DRIVER_FILE_TRANSFER_H_

#include <open62541/server.h>

_UA_BEGIN_DECLS

/**
 * File Transfer Driver
 * --------------------
 *
 * Expose a file or directory tree through the OPC UA Part 20 FileType and
 * FileDirectoryType Methods. Each driver owns one storage backend; built-in
 * backends serve local files and directories.
 *
 * Declarations are always available; linking requires ``UA_ENABLE_METHODCALLS``.
 * The reduced and full Namespace Zero include the required types; with MINIMAL
 * or NONE, load them before constructing a driver. See
 * :doc:`tutorial_server_filetransfer` for setup.
 *
 * Storage Backend
 * ~~~~~~~~~~~~~~~
 *
 * Use ``UA_FileTransferFileBackend`` for one file or ``UA_FileTransferBackend``
 * for a directory tree. Each driver needs an independently owned context.
 * Callbacks return OPC UA StatusCodes. Paths are UTF-8, relative to the backend
 * root, with '/' separators; the empty path denotes the root.
 *
 * open, close, read, getPosition, setPosition and getInfo are required.
 * Directory backends also require listDirectory. Unless ``read-only`` is set,
 * write is required, plus create, remove and rename for directories. Missing
 * callbacks cause Bad_InvalidArgument during construction or start.
 *
 * Backend handles must be unique within the instance and have independent
 * positions. Client handles belong to their Session and close with it.
 *
 * Callbacks run synchronously with the server lock held and must not block
 * for extended periods. Scans and recursive copy/delete run in a single call;
 * their duration grows with the tree size. */

/* Permission bit mask for one Object. For files, read/write access content.
 * For directories, read lists entries, write changes entries, and traverse
 * accesses children. Managed ancestors require traverse; directory mutations
 * require write/traverse, recursive copy requires read/traverse on directories
 * and read on files. Traverse has no meaning for files.
 *
 * Storage rights, read-only configuration and the optional access callbacks
 * are intersected. They govern file operations and Writable/UserWritable;
 * OPC UA Browse remains controlled by accessControl.allowBrowseNode.
 * Owner/group/other selection and ACL evaluation belong to the backend. */
typedef UA_Byte UA_FileAccessRights;

#define UA_FILEACCESS_TRAVERSE ((UA_FileAccessRights)0x01)
#define UA_FILEACCESS_WRITE    ((UA_FileAccessRights)0x02)
#define UA_FILEACCESS_READ     ((UA_FileAccessRights)0x04)

/* Maximum UTF-8 byte length of one filename, excluding the terminator.
 * This is an API limit, not a filesystem limit. Never truncate a name. */
#define UA_FILETRANSFER_FILENAME_MAX 1023

typedef struct {
    /* NUL-terminated UTF-8 basename; empty for a directory root */
    char name[UA_FILETRANSFER_FILENAME_MAX + 1];
    UA_UInt64 size;             /* Size in bytes (files only) */
    UA_DateTime lastModified;   /* Last modification time */
    UA_Boolean isDirectory;
    UA_FileAccessRights accessRights; /* Storage-level permissions. Backend
                                      * callbacks can further restrict them. */
    char mimeType[255];         /* Media type, zero-padded; all bytes may be used.
                                * Empty omits a new MimeType Property and
                                * leaves an existing one unchanged. */
} UA_FileTransferFileInfo;

typedef struct UA_FileTransferFileBackend UA_FileTransferFileBackend;
struct UA_FileTransferFileBackend {
    /* Backend-owned storage and handle state */
    void *context;

    /* Open a file. mode is a UA_Byte bit mask of UA_OPENFILEMODE_* flags,
     * validated by the driver. Append starts at EOF; otherwise start at zero.
     * EraseExisting truncates the file. Return an independent handle. */
    UA_StatusCode (*open)(UA_FileTransferFileBackend *b, const UA_String path,
                          UA_Byte mode, UA_UInt32 *handle);

    /* Close the handle and flush buffered writes. Report flush errors here;
     * a destination close error also fails a driver-managed copy. */
    UA_StatusCode (*close)(UA_FileTransferFileBackend *b, UA_UInt32 handle);

    /* Read up to length bytes and advance the position. Allocate out for the
     * caller to free with UA_ByteString_clear(). Empty out means EOF. */
    UA_StatusCode (*read)(UA_FileTransferFileBackend *b, UA_UInt32 handle,
                          UA_Int32 length, UA_ByteString *out);

    /* Write all data and advance the position. The caller retains the buffer. */
    UA_StatusCode (*write)(UA_FileTransferFileBackend *b, UA_UInt32 handle,
                           const UA_ByteString data);

    /* Get the current position of the handle */
    UA_StatusCode (*getPosition)(UA_FileTransferFileBackend *b, UA_UInt32 handle,
                                 UA_UInt64 *outPosition);

    /* Set the current position of the handle. Positions beyond the end of the
     * file are clamped to the file size (Part 20, 4.2.7). */
    UA_StatusCode (*setPosition)(UA_FileTransferFileBackend *b, UA_UInt32 handle,
                                 UA_UInt64 position);

    /* Get metadata, including the basename (empty for a directory root).
     * outInfo is zero-initialized. Set accessRights explicitly: zero denies
     * access. An overlong name returns Bad_EncodingLimitsExceeded. */
    UA_StatusCode (*getInfo)(UA_FileTransferFileBackend *b, const UA_String path,
                             UA_FileTransferFileInfo *outInfo);

    /* Optional restrictions independent of client identity. outRights starts
     * at zero; errors deny access and propagate to the caller. NULL adds no
     * restriction. Policy state is available in b->context. */
    UA_StatusCode (*getAccessRights)(UA_FileTransferFileBackend *b,
                                     UA_Server *server, const UA_NodeId *nodeId,
                                     UA_FileAccessRights *outRights);

    /* Optional Session-specific restrictions, with the Session identity and
     * context from the Method or Property callback. Same output/error rules
     * as getAccessRights. Open and every Read/Write recheck permissions;
     * Close remains possible after revocation. With either access callback,
     * recursive operations reject unmirrored entries needing an Object check. */
    UA_StatusCode (*getUserAccessRights)(UA_FileTransferFileBackend *b,
                                         UA_Server *server,
                                         const UA_NodeId *sessionId,
                                         void *sessionContext,
                                         const UA_NodeId *nodeId,
                                         UA_FileAccessRights *outRights);

    /* Release context when the driver is freed. Optional; called once if set. */
    void (*clear)(UA_FileTransferFileBackend *b);
};

/* Report one entry with fully initialized metadata and a non-empty,
 * NUL-terminated basename. info is borrowed for the duration of the callback;
 * copy the struct to retain it. */
typedef void
(*UA_FileTransferListCallback)(void *listContext,
                               const UA_FileTransferFileInfo *info);

typedef struct UA_FileTransferBackend UA_FileTransferBackend;
struct UA_FileTransferBackend {
    /* File operations; first member to allow casting b back to this struct */
    UA_FileTransferFileBackend file;

    /* Enumerate entries at path. Unreadable metadata may be skipped. An
     * enumeration error discards the listing. Never truncate overlong names;
     * return Bad_EncodingLimitsExceeded instead. */
    UA_StatusCode (*listDirectory)(UA_FileTransferBackend *b, const UA_String path,
                                   UA_FileTransferListCallback cb, void *listContext);

    /* Create an empty entry at path; fail if it exists. info is borrowed;
     * name matches the path basename, isDirectory selects the type. Other
     * fields are zeroed and ignored; use backend defaults for metadata. */
    UA_StatusCode (*create)(UA_FileTransferBackend *b, const UA_String path,
                            const UA_FileTransferFileInfo *info);

    /* Remove a file or empty directory. The driver removes trees bottom-up. */
    UA_StatusCode (*remove)(UA_FileTransferBackend *b, const UA_String path);

    /* Move within this backend. Never overwrite or merge an existing target,
     * even if it is not mirrored: return Bad_BrowseNameDuplicated. The driver
     * rejects moves of entries with open handles. */
    UA_StatusCode (*rename)(UA_FileTransferBackend *b, const UA_String fromPath,
                            const UA_String toPath);

    /* Optional file copy within this backend; otherwise the driver uses
     * open/read/write/close. Trees are copied entry by entry. As for rename,
     * do not overwrite targets or copy entries with open handles. Remove any
     * partial destination on failure. */
    UA_StatusCode (*copy)(UA_FileTransferBackend *b, const UA_String fromPath,
                          const UA_String toPath);
};

/**
 * Construction and Parameters
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * Set ``driver->params`` before UA_Server_addDriver(). Parameters are read on
 * each start; all keys use namespace 0.
 *
 * 0:read-only [Boolean]
 *    Disable writes and directory mutations (default: false).
 * 0:max-scan-depth [UInt32]
 *    Maximum depth of mirrored entries; the root's entries have depth 1 and
 *    directories at the limit appear empty. 0 means unlimited (default).
 *    CreateFile, CreateDirectory and MoveOrCopy return Bad_ResourceUnavailable
 *    for targets beyond the limit. Lowering the limit keeps deeper Objects,
 *    but they are no longer refreshed.
 * 0:max-nodes [UInt32]
 *    Maximum mirrored Objects, including the root; 0 means unlimited
 *    (default). Lowering the limit does not remove existing Objects.
 * 0:namespace-index [UInt16]
 *    Namespace for new entry BrowseNames and NodeIds (default: 0).
 *    Standard Properties and Methods remain in namespace 0.
 * 0:refresh-interval [Double]
 *    Directory refresh interval in milliseconds (default: 1000), at least
 *    0.0001. 0 disables the periodic refresh, see
 *    UA_FileTransferDriver_refresh(). Vanished files with open handles remain
 *    until closed.
 * 0:max-open-handles-per-session [UInt16]
 *    Open-handle limit per Session in this driver (default: 64).
 * 0:max-open-handles-per-file [UInt16]
 *    Open-handle limit per file (default: 16). Either handle limit makes
 *    Open return Bad_ResourceUnavailable when reached.
 * 0:max-read-length [UInt32]
 *    Maximum bytes per Read or Write (default, also when zero: 1048576).
 *    Published as MaxByteStringLength. Reads are capped at this length;
 *    larger writes fail with Bad_InvalidArgument.
 *
 * Each refresh lists every mirrored directory with the server lock held, with
 * max-nodes twice (removals first). For large or deep trees, set
 * max-scan-depth and max-nodes and consider a longer refresh-interval, or 0
 * with UA_FileTransferDriver_refresh(). Skipped entries and trees cut off by
 * these limits are logged at start and when their numbers change; unchanged
 * results only at debug level. */

/* Description of the root Object. Zero-initialized fields select defaults.
 * The description and its contents are borrowed only during construction. */
typedef struct {
    UA_NodeId nodeId;          /* Existing Object to reuse, or ID to create.
                               * Null generates a NodeId in namespace 1. */
    UA_NodeId parentNodeId;    /* Default: Objects folder */
    UA_NodeId referenceTypeId; /* Default: HasComponent */
    UA_QualifiedName browseName; /* Default: path basename (file), or 0:FileSystem */
    UA_NodeId typeDefinition;  /* Default: FileType or FileDirectoryType */
    UA_ObjectAttributes attributes; /* Default Object attributes; an empty
                                    * DisplayName uses the BrowseName */
} UA_FileTransferNodeDescription;

/* Construct a stopped driver for an existing backend file or directory.
 * newDirectory serves the empty path. description may be NULL for defaults.
 * A default file BrowseName uses namespace 1 and the path basename; for an
 * empty path, use getInfo's name or "File" if unnamed.
 *
 * An existing description->nodeId reuses a FileType/FileDirectoryType Object
 * (or subtype); other description fields are ignored. Children are matched
 * by BrowseName text regardless of namespace. Ambiguous or incompatible
 * entries are rejected; unrelated existing nodes are untouched.
 *
 * Bound Object contexts point to the driver, including while stopped. Reused
 * roots are bound on first start. Freeing removes driver-created nodes and
 * restores reused Objects' contexts and Properties' values, sources and contexts.
 * Reused Objects also survive disappearance of their storage entry.
 * While started, drivers supply the standard Methods, including shared
 * Namespace Zero Method nodes and copies inherited by subtypes. Other
 * Methods of subtypes stay with the application, see
 * UA_FileTransferDriver_getHandleInfo().
 *
 * On success, the backend struct is copied and its context becomes owned by
 * the driver. On failure, created nodes are removed, *outDriver is NULL and
 * the caller retains the backend. outDriver is required; outNodeId is optional
 * and receives an owned NodeId only on success.
 *
 * Registration is separate. Before successful UA_Server_addDriver(), the
 * caller must free the driver before deleting the server; afterwards the
 * server owns it. To free explicitly, stop and remove the driver first.
 * Start mirrors directory entries and the periodic refresh (unless
 * refresh-interval is 0) keeps them in sync.
 * Stop closes handles but preserves nodes and backend for restart. */
UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_FileTransferDriver_newFile(UA_Server *server,
                              const UA_FileTransferFileBackend *backend,
                              const UA_String path,
                              const UA_FileTransferNodeDescription *description,
                              UA_NodeId *outNodeId, UA_Driver **outDriver);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_FileTransferDriver_newDirectory(UA_Server *server,
                                   const UA_FileTransferBackend *backend,
                                   const UA_FileTransferNodeDescription *description,
                                   UA_NodeId *outNodeId, UA_Driver **outDriver);

/* Reconcile the directory tree of a started directory driver with its backend
 * now, e.g. with the periodic refresh disabled. Returns Bad_InvalidState for a
 * stopped driver and Bad_NotSupported for a file driver. */
UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_FileTransferDriver_refresh(UA_Driver *driver);

/* FileType subtypes can define Methods on an open file, e.g. CloseAndUpdate of
 * the PubSubConfiguration (Part 14). These Methods stay with the application,
 * which also implements the backend of the file. Their callbacks resolve the
 * client's FileHandle to the open mode and the backend handle, and may then
 * close it like the Close Method. Handles are bound to the Session and the file
 * Object that opened them; others return Bad_InvalidArgument. closeHandle
 * returns the result of the backend close. Both can be called from Method
 * callbacks. */
UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_FileTransferDriver_getHandleInfo(UA_Driver *driver,
                                    const UA_NodeId fileNodeId,
                                    const UA_NodeId *sessionId,
                                    UA_UInt32 fileHandle, UA_Byte *mode,
                                    UA_UInt32 *backendHandle);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_FileTransferDriver_closeHandle(UA_Driver *driver,
                                  const UA_NodeId fileNodeId,
                                  const UA_NodeId *sessionId,
                                  UA_UInt32 fileHandle);

/**
 * Local Backends
 * ~~~~~~~~~~~~~~ */

/* Serve an existing local directory (UTF-8 path, copied on success).
 * out is written only on success. Reported rights follow the server process's
 * filesystem permissions, further restricted by the optional access callbacks.
 * Windows traversal checks are left to the filesystem operations.
 *
 * Paths with ".." segments are rejected, but links to regular files are followed
 * even outside rootPath: this is not a filesystem sandbox, including in
 * read-only mode. Directory symlinks and special files are skipped; directories
 * containing them cannot be deleted through the driver (Bad_InvalidState).
 * Windows rejects unrepresentable names. 32-bit POSIX positions are limited
 * to 2 GiB. */
UA_EXPORT UA_StatusCode
UA_FileTransferBackend_localDirectory(const UA_String rootPath,
                                      UA_FileTransferBackend *out);

/* Serve one existing regular file using the same local I/O and permissions.
 * filePath is UTF-8 and copied; out is written only on success. Links to regular
 * files are followed. Only the empty backend path is accepted; getInfo reports
 * the basename and media type. Pass UA_STRING_NULL to newFile(). */
UA_EXPORT UA_StatusCode
UA_FileTransferFileBackend_localFile(const UA_String filePath,
                                     UA_FileTransferFileBackend *out);

_UA_END_DECLS

#endif /* UA_DRIVER_FILE_TRANSFER_H_ */
