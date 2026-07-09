/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef UA_DRIVER_FILE_TRANSFER_H_
#define UA_DRIVER_FILE_TRANSFER_H_

#include <open62541/server.h>

/**
 * File Transfer Driver (Experimental)
 * -----------------------------------
 *
 * OPC UA Part 20 defines an information model for file transfer. Files are
 * represented as Objects of FileType with Methods to open, close, read and
 * write the file content in chunks. Directories are represented as Objects of
 * FileDirectoryType with Methods to create, delete, move and copy files and
 * directories. The root of an exposed directory tree is an Object with the
 * BrowseName "FileSystem".
 *
 * This driver provides the Part 20 semantics on top of a pluggable storage
 * backend (``UA_FileTransferBackend``). A built-in backend serves a directory
 * of the local filesystem. Custom backends can store the file content in
 * memory, in flash, in a database, or generate it on the fly.
 *
 * File handles returned by the Open Method are bound to the Session that
 * created them. They are closed automatically when the Session closes. The
 * life-cycle of the storage entry and the Object representing it in the
 * address space are decoupled: files may disappear from the backend while the
 * Object still exists (Method calls then return an error and the refresh
 * function reconciles the address space).
 *
 * Create the driver with UA_FileTransferDriver_new() and attach it to a
 * server with UA_Server_addDriver(). Only one file transfer driver instance
 * can be attached to a server. The driver requires the full Namespace Zero
 * (``UA_NAMESPACE_ZERO=FULL``) and Method calls enabled. */

#if defined(UA_ENABLE_METHODCALLS) && defined(UA_GENERATED_NAMESPACE_ZERO_FULL)

_UA_BEGIN_DECLS

/**
 * Storage Backend
 * ~~~~~~~~~~~~~~~
 *
 * The backend maps abstract file operations to the actual storage. One
 * backend instance is bound to one mount (a FileSystem Object created with
 * addFileSystem or a standalone file created with addFile). The backend
 * struct is copied and the driver takes ownership in all cases: the clear
 * callback is invoked exactly once, either when the mount is removed, when
 * the driver is freed, or immediately when adding the mount fails.
 *
 * Paths passed to the backend are UTF-8 encoded, use '/' as separator and are
 * relative to the mount root (the empty string denotes the root itself).
 * Backends never see OPC UA NodeIds.
 *
 * The open mode is the Part 20 bit mask (see the generated UA_OpenFileMode:
 * UA_OPENFILEMODE_READ = 1, UA_OPENFILEMODE_WRITE = 2,
 * UA_OPENFILEMODE_ERASEEXISTING = 4, UA_OPENFILEMODE_APPEND = 8). The driver
 * validates the mode before calling into the backend.
 *
 * Every successful openFile call creates an independent file context with its
 * own position. Backends return OPC UA StatusCodes directly (e.g. map ENOENT
 * to UA_STATUSCODE_BADNOTFOUND and EACCES to
 * UA_STATUSCODE_BADUSERACCESSDENIED).
 *
 * Backend calls are executed in the server's main loop. They must not block
 * for extended periods of time (network filesystems, remote storage). Read
 * and write sizes are bounded by the maxByteStringLength limit configured for
 * the server. */

typedef struct {
    UA_UInt64 size;             /* Size in bytes (files only) */
    UA_DateTime lastModified;   /* Last modification time */
    UA_Boolean isDirectory;
    UA_Boolean writable;        /* Storage-level write permission. User-level
                                 * access rights are handled by the driver. */
} UA_FileTransferFileInfo;

/* Called by the backend for every entry when listing a directory */
typedef void
(*UA_FileTransferListCallback)(void *listContext, const UA_String name,
                               UA_Boolean isDirectory);

typedef struct UA_FileTransferBackend UA_FileTransferBackend;
struct UA_FileTransferBackend {
    /* Backend-private state, e.g. the root path of the served directory */
    void *context;

    /* Open the file at path. Returns an opaque per-open file context with an
     * independent position. The position is at the end of the file if the
     * Append bit is set, otherwise at the beginning. The EraseExisting bit
     * truncates the file. */
    UA_StatusCode (*openFile)(UA_FileTransferBackend *b, const UA_String path,
                              UA_Byte mode, void **fileContext);

    /* Close the file context. Pending content is flushed to the storage. */
    UA_StatusCode (*closeFile)(UA_FileTransferBackend *b, void *fileContext);

    /* Read up to length bytes from the current position. The position is
     * advanced by the number of bytes read. out is allocated by the backend
     * and freed by the caller. An empty out ByteString indicates the end of
     * the file. */
    UA_StatusCode (*read)(UA_FileTransferBackend *b, void *fileContext,
                          UA_Int32 length, UA_ByteString *out);

    /* Write the data at the current position. The position is advanced by the
     * number of bytes written. The data buffer remains owned by the caller. */
    UA_StatusCode (*write)(UA_FileTransferBackend *b, void *fileContext,
                           const UA_ByteString data);

    /* Get the current position of the file context */
    UA_StatusCode (*getPosition)(UA_FileTransferBackend *b, void *fileContext,
                                 UA_UInt64 *outPosition);

    /* Set the current position of the file context. Positions beyond the end
     * of the file are clamped to the file size (Part 20, 4.2.7). */
    UA_StatusCode (*setPosition)(UA_FileTransferBackend *b, void *fileContext,
                                 UA_UInt64 position);

    /* Query metadata for the entry at path */
    UA_StatusCode (*getAttributes)(UA_FileTransferBackend *b, const UA_String path,
                                   UA_FileTransferFileInfo *outInfo);

    /* Call cb for every entry in the directory at path */
    UA_StatusCode (*listDirectory)(UA_FileTransferBackend *b, const UA_String path,
                                   UA_FileTransferListCallback cb, void *listContext);

    /* Create an empty file at path. Fails if the entry already exists. */
    UA_StatusCode (*createFile)(UA_FileTransferBackend *b, const UA_String path);

    /* Create a directory at path. Fails if the entry already exists. */
    UA_StatusCode (*createDirectory)(UA_FileTransferBackend *b, const UA_String path);

    /* Remove the file or empty directory at path. The driver removes
     * directory trees recursively bottom-up with individual calls. */
    UA_StatusCode (*remove)(UA_FileTransferBackend *b, const UA_String path);

    /* Rename/move an entry within the same backend */
    UA_StatusCode (*rename)(UA_FileTransferBackend *b, const UA_String fromPath,
                            const UA_String toPath);

    /* Optional fast-path to copy a file within the same backend. If NULL, the
     * driver emulates copying with openFile/read/write loops. */
    UA_StatusCode (*copy)(UA_FileTransferBackend *b, const UA_String fromPath,
                          const UA_String toPath);

    /* Release the backend context. Called exactly once when the mount is
     * removed or the driver is freed. Can be NULL. */
    void (*clear)(UA_FileTransferBackend *b);
};

/* Built-in backend that serves rootPath of the local filesystem. The backend
 * enforces that no path can escape rootPath (path segments like ".." are
 * rejected). Symbolic links inside rootPath are followed.
 *
 * @param rootPath The served directory. Must exist.
 * @param out The backend to initialize
 * @return The StatusCode of the operation */
UA_EXPORT UA_StatusCode
UA_FileTransferBackend_localFilesystem(const UA_String rootPath,
                                       UA_FileTransferBackend *out);

/**
 * Mount Options
 * ~~~~~~~~~~~~~ */

typedef struct {
    /* Expose the mount read-only: the Writable/UserWritable Properties are
     * false, opening files for writing and all mutating directory Methods
     * return Bad_UserAccessDenied. */
    UA_Boolean readOnly;

    /* Maximum recursion depth of the initial scan and refresh. Files and
     * directories below the limit are not represented in the address space.
     * 0 means unlimited. */
    UA_UInt32 maxScanDepth;

    /* Maximum number of file/directory Objects created for this mount.
     * Entries beyond the limit are skipped with a warning. 0 means
     * unlimited. */
    UA_UInt32 maxNodes;

    /* Per-user write permission hook for the UserWritable Property and write
     * access checks. If NULL, UserWritable mirrors the Writable Property. */
    UA_Boolean (*getUserWritable)(UA_Server *server, const UA_NodeId *sessionId,
                                  const UA_NodeId *fileNodeId, void *mountContext);

    /* Passed to the getUserWritable hook */
    void *mountContext;
} UA_FileTransferMountOptions;

/**
 * File Transfer Driver
 * ~~~~~~~~~~~~~~~~~~~~
 *
 * The driver accepts the following configuration parameters (UA_KeyValueMap):
 *
 * 0:max-open-handles-per-session [UInt16]
 *    Maximum number of open file handles per Session (default: 64). Open
 *    returns Bad_ResourceUnavailable when the limit is reached.
 * 0:max-open-handles-per-file [UInt16]
 *    Maximum number of open file handles per file (default: 16).
 * 0:max-read-length [UInt32]
 *    Maximum number of bytes returned by a single Read Method call
 *    (default: 1 MByte). Longer read requests are truncated; clients
 *    continue reading at the advanced position. */

typedef struct UA_FileTransferDriver UA_FileTransferDriver;
struct UA_FileTransferDriver {
    UA_Driver drv; /* Must be the first member */

    /* Create a FileDirectoryType Object under parentNodeId (referenced with
     * HasComponent) and mirror the backend content below it. Subdirectories
     * become FileDirectoryType Objects and files become FileType Objects,
     * both referenced with Organizes (Part 20, 4.3).
     *
     * The driver must be started. The backend struct is copied; its clear
     * callback is invoked when the mount is removed.
     *
     * @param driver The file transfer driver
     * @param requestedNodeId The requested NodeId for the FileSystem Object.
     *        Passing UA_NODEID_NULL selects an unused NodeId in namespace 0.
     * @param parentNodeId The parent node of the FileSystem Object
     * @param browseName The BrowseName of the FileSystem Object. Part 20
     *        mandates the name 0:"FileSystem" for the root of an exposed
     *        directory structure.
     * @param backend The storage backend for this mount
     * @param options Mount options. NULL selects the defaults (writable,
     *        unlimited scan).
     * @param outFileSystemNodeId The created FileSystem Object (can be NULL)
     * @return The StatusCode of the operation */
    UA_StatusCode (*addFileSystem)(UA_FileTransferDriver *driver,
                                   const UA_NodeId requestedNodeId,
                                   const UA_NodeId parentNodeId,
                                   const UA_QualifiedName browseName,
                                   UA_FileTransferBackend backend,
                                   const UA_FileTransferMountOptions *options,
                                   UA_NodeId *outFileSystemNodeId);

    /* Remove a mount created with addFileSystem. Open handles below the mount
     * are closed and the Objects are removed from the address space. The
     * backend storage content is not touched.
     *
     * @param driver The file transfer driver
     * @param fileSystemNodeId The FileSystem Object of the mount
     * @return The StatusCode of the operation */
    UA_StatusCode (*removeFileSystem)(UA_FileTransferDriver *driver,
                                      const UA_NodeId fileSystemNodeId);

    /* Create a standalone FileType Object bound to a single backend path.
     * Useful to expose an individual file (configuration, firmware, log)
     * without exposing a directory.
     *
     * @param driver The file transfer driver
     * @param requestedNodeId The requested NodeId for the file Object.
     *        Passing UA_NODEID_NULL selects an unused NodeId in namespace 0.
     * @param parentNodeId The parent node of the file Object (referenced with
     *        HasComponent)
     * @param browseName The BrowseName of the file Object
     * @param backend The storage backend for this file
     * @param path The backend path of the file. Must exist.
     * @param options Mount options. NULL selects the defaults.
     * @param outFileNodeId The created file Object (can be NULL)
     * @return The StatusCode of the operation */
    UA_StatusCode (*addFile)(UA_FileTransferDriver *driver,
                             const UA_NodeId requestedNodeId,
                             const UA_NodeId parentNodeId,
                             const UA_QualifiedName browseName,
                             UA_FileTransferBackend backend,
                             const UA_String path,
                             const UA_FileTransferMountOptions *options,
                             UA_NodeId *outFileNodeId);

    /* Remove a standalone file Object created with addFile. Open handles are
     * closed. The backend file is not touched.
     *
     * @param driver The file transfer driver
     * @param fileNodeId The file Object
     * @return The StatusCode of the operation */
    UA_StatusCode (*removeFile)(UA_FileTransferDriver *driver,
                                const UA_NodeId fileNodeId);

    /* Reconcile a mounted directory (or the entire mount when called with the
     * FileSystem Object) with the backend content: Objects are created for
     * new entries and removed for vanished entries. Vanished files with open
     * handles are removed from the address space once the last handle is
     * closed.
     *
     * @param driver The file transfer driver
     * @param directoryNodeId A directory Object of a mount
     * @return The StatusCode of the operation */
    UA_StatusCode (*refresh)(UA_FileTransferDriver *driver,
                             const UA_NodeId directoryNodeId);
};

UA_EXPORT UA_FileTransferDriver *
UA_FileTransferDriver_new(const UA_KeyValueMap params);

_UA_END_DECLS

#endif /* UA_ENABLE_METHODCALLS && UA_GENERATED_NAMESPACE_ZERO_FULL */

#endif /* UA_DRIVER_FILE_TRANSFER_H_ */
