/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#ifndef UA_DRIVER_FILE_TRANSFER_H_
#define UA_DRIVER_FILE_TRANSFER_H_

#include <open62541/server.h>

/**
 * File Transfer Driver
 * --------------------
 *
 * OPC UA Part 20 defines an information model for file transfer. Files are
 * represented as Objects of FileType with Methods to open, close, read and
 * write the file content in chunks. Directories are represented as Objects of
 * FileDirectoryType with Methods to create, delete, move and copy files and
 * directories. The root of an exposed directory tree is an Object with the
 * BrowseName "FileSystem".
 *
 * This driver provides the Part 20 semantics on top of a pluggable storage
 * backend (``UA_FileTransferBackend``). Backends can store the file content in
 * memory, in flash, in a database, or generate it on the fly.
 *
 * File handles returned by the Open Method are bound to the Session that
 * created them. They are closed automatically when the Session closes. The
 * life-cycle of the storage entry and the Object representing it in the
 * address space are decoupled: files may disappear from the backend while the
 * Object still exists (Method calls then return an error and the refresh
 * function reconciles the address space).
 *
 * Build open62541 with ``UA_ENABLE_DRIVER_FILE_TRANSFER`` enabled. The driver
 * needs Method calls and the reduced or full Namespace Zero; with the reduced
 * Namespace Zero, the FileType and FileDirectoryType definitions are added to
 * it.
 *
 * Create the driver with UA_FileTransferDriver_new() and attach its ``drv``
 * member to a server with UA_Server_addDriver(). Only one file transfer driver
 * instance can be attached to a server. The mounts can be added once the
 * driver is attached, also before the server is started. The functions of the
 * driver can be called from any thread, they take the server lock.
 *
 * While started, the driver owns the FileType and FileDirectoryType Method
 * nodes of Namespace Zero. Object instances reference those shared nodes
 * instead of copying them, so the Part 20 Methods of *every*
 * FileType/FileDirectoryType Object in the server are answered by the driver,
 * which rejects Objects it does not manage. An application that implements
 * FileType Objects itself must not run this driver at the same time. Stopping
 * the driver releases the Method nodes again. With ``copyMethodsOnInstances``,
 * the Objects created by the driver get their own Method copies, which the
 * driver serves as well. */

#ifdef UA_ENABLE_DRIVER_FILE_TRANSFER

_UA_BEGIN_DECLS

/**
 * Storage Backend
 * ~~~~~~~~~~~~~~~
 *
 * The backend maps abstract file operations to the actual storage. A single
 * file is served with a ``UA_FileTransferFileBackend``. A directory tree is
 * served with a ``UA_FileTransferBackend``, which extends the file backend
 * (its first member) with the directory operations.
 *
 * One backend instance is bound to one mount (a FileSystem Object created with
 * UA_FileTransferDriver_addFileSystem() or a file Object created with
 * UA_FileTransferDriver_addFile()). The backend struct is copied and the
 * driver takes ownership of it in all cases: the clear callback is invoked
 * automatically exactly once, either when the mount is removed, when the
 * driver is freed, or immediately when adding the mount fails.
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
 * Every successful open call returns a handle with an independent position.
 * The handles are chosen by the backend and only need to be unique within the
 * backend instance. The driver hands out its own FileHandles to the clients
 * (unique per Session, Part 20, 4.2.2) and maps them to the backend handles.
 * Backends return OPC UA StatusCodes directly (e.g. map ENOENT to
 * UA_STATUSCODE_BADNOTFOUND and EACCES to UA_STATUSCODE_BADUSERACCESSDENIED).
 *
 * A backend only has to implement the operations its mount can reach. open,
 * close, read, getPosition, setPosition and getInfo are always required. write
 * is required unless the mount is read-only. listDirectory and -- for a
 * writable mount -- createFile, createDirectory, remove and rename are
 * required for a directory mount. Adding a mount whose backend is missing an
 * operation it would need returns Bad_InvalidArgument.
 *
 * When the backend reports an error for a single entry during the initial scan
 * or a refresh -- an unreadable subdirectory, a file that vanished mid-scan --
 * the entry is skipped with a warning instead of failing the whole mount.
 *
 * Backend calls are made with the server lock held, from the server's main
 * loop or from the thread that calls the driver API. They must not block for
 * extended periods of time (network filesystems, remote storage). Read and
 * write sizes are bounded by the max-read-length parameter of the driver. A
 * scan, a refresh and the copy or deletion of a directory tree run within one
 * call, so their duration grows with the size of the tree. */

typedef struct {
    UA_UInt64 size;             /* Size in bytes (files only) */
    UA_DateTime lastModified;   /* Last modification time */
    UA_Boolean isDirectory;
    UA_Boolean writable;        /* Storage-level write permission. User-level
                                 * access rights are handled by the driver. */
    char mimeType[255];         /* Media type "type/subtype" (RFC 6838, files
                                 * only), padded with trailing zeros. It is not
                                 * zero-terminated when all 255 characters are
                                 * used. Empty means unknown; the optional
                                 * MimeType Property is then not added (an
                                 * existing one is left as it is). */
} UA_FileTransferFileInfo;

typedef struct UA_FileTransferFileBackend UA_FileTransferFileBackend;
struct UA_FileTransferFileBackend {
    /* Backend-private state, e.g. the root path of the served directory and
     * the table of the open handles */
    void *context;

    /* Open the file at path and return a handle with an independent position.
     * The position is at the end of the file if the Append bit is set,
     * otherwise at the beginning. The EraseExisting bit truncates the file.
     * Directories are not opened, they are read with listDirectory. */
    UA_StatusCode (*open)(UA_FileTransferFileBackend *b, const UA_String path,
                          UA_Byte mode, UA_UInt32 *handle);

    /* Close the handle. Buffered content is written to the storage, a failed
     * write is reported here. A copy made by the driver fails when closing its
     * destination fails. */
    UA_StatusCode (*close)(UA_FileTransferFileBackend *b, UA_UInt32 handle);

    /* Read up to length bytes from the current position. The position is
     * advanced by the number of bytes read. out is allocated by the backend
     * and freed by the caller. An empty out ByteString indicates the end of
     * the file. */
    UA_StatusCode (*read)(UA_FileTransferFileBackend *b, UA_UInt32 handle,
                          UA_Int32 length, UA_ByteString *out);

    /* Write the data at the current position. The position is advanced by the
     * number of bytes written. The data buffer remains owned by the caller. */
    UA_StatusCode (*write)(UA_FileTransferFileBackend *b, UA_UInt32 handle,
                           const UA_ByteString data);

    /* Get the current position of the handle */
    UA_StatusCode (*getPosition)(UA_FileTransferFileBackend *b, UA_UInt32 handle,
                                 UA_UInt64 *outPosition);

    /* Set the current position of the handle. Positions beyond the end of the
     * file are clamped to the file size (Part 20, 4.2.7). */
    UA_StatusCode (*setPosition)(UA_FileTransferFileBackend *b, UA_UInt32 handle,
                                 UA_UInt64 position);

    /* Get the metadata of the file or directory at path. outInfo is
     * zero-initialized by the driver. */
    UA_StatusCode (*getInfo)(UA_FileTransferFileBackend *b, const UA_String path,
                             UA_FileTransferFileInfo *outInfo);

    /* Release the backend context. Called automatically by the driver exactly
     * once: when the mount is removed, when the driver is freed, or right away
     * when adding the mount fails. Can be NULL. */
    void (*clear)(UA_FileTransferFileBackend *b);
};

/* Called by the backend for every entry when listing a directory */
typedef void
(*UA_FileTransferListCallback)(void *listContext, const UA_String name,
                               UA_Boolean isDirectory);

typedef struct UA_FileTransferBackend UA_FileTransferBackend;
struct UA_FileTransferBackend {
    /* The file operations. Must be the first member: the driver passes a
     * pointer to it to the file operations of a directory backend. */
    UA_FileTransferFileBackend file;

    /* Call cb for every entry in the directory at path */
    UA_StatusCode (*listDirectory)(UA_FileTransferBackend *b, const UA_String path,
                                   UA_FileTransferListCallback cb, void *listContext);

    /* Create an empty file at path. Fails if the entry already exists. */
    UA_StatusCode (*createFile)(UA_FileTransferBackend *b, const UA_String path);

    /* Create a directory at path. Fails if the entry already exists. */
    UA_StatusCode (*createDirectory)(UA_FileTransferBackend *b, const UA_String path);

    /* Remove the file or the empty directory at path. The backend never has
     * to remove a non-empty directory: the driver deletes a directory tree
     * with one remove call per entry, the deepest entries first. */
    UA_StatusCode (*remove)(UA_FileTransferBackend *b, const UA_String path);

    /* Move the file or directory at fromPath to toPath within this backend.
     * The driver checks that no Object exists for toPath. An entry at toPath
     * that the driver does not know (created out-of-band, below maxScanDepth)
     * must not be overwritten or merged: fail with Bad_BrowseNameDuplicated.
     * Entries with open file handles are not moved (Bad_InvalidState). */
    UA_StatusCode (*rename)(UA_FileTransferBackend *b, const UA_String fromPath,
                            const UA_String toPath);

    /* Optional fast path to copy the file at fromPath to toPath within this
     * backend. It is only called for files, the driver copies a directory
     * entry by entry (createDirectory and one copy per file). As for rename,
     * an existing entry at toPath is not overwritten and entries with open
     * file handles are not copied. A failed copy leaves no partial file at
     * toPath. If NULL, the driver copies with open/read/write/close. */
    UA_StatusCode (*copy)(UA_FileTransferBackend *b, const UA_String fromPath,
                          const UA_String toPath);
};

/**
 * Mount Options
 * ~~~~~~~~~~~~~ */

typedef struct {
    /* Expose the mount read-only: the Writable/UserWritable Properties are
     * false, opening a file for writing returns Bad_NotWritable and the
     * mutating directory Methods return Bad_UserAccessDenied. */
    UA_Boolean readOnly;

    /* Maximum recursion depth of the initial scan and refresh. Files and
     * directories below the limit are not represented in the address space.
     * 0 means unlimited. */
    UA_UInt32 maxScanDepth;

    /* Maximum number of file/directory Objects created for this mount, the
     * FileSystem root included. Entries beyond the limit are skipped with a
     * warning, and the CreateFile/CreateDirectory Methods return
     * Bad_ResourceUnavailable. 0 means unlimited. */
    UA_UInt32 maxNodes;

    /* NamespaceIndex used for the BrowseNames and NodeIds of the mirrored file
     * and directory Objects. Part 20 does not constrain the namespace of the
     * <FileName>/<FileDirectoryName> placeholders, and the default 0 keeps
     * the names next to the Part 20 Properties and Methods. Point this at the
     * server's own namespace to keep storage-defined names out of the OPC UA
     * namespace. The Properties and Methods defined by Part 20 always stay in
     * namespace 0, as does the "FileSystem" root. */
    UA_UInt16 namespaceIndex;

    /* Per-user write permission hook. For a file Object it defines the
     * UserWritable Property and whether the file can be opened for writing. It
     * restricts the Writable Property, which is the storage writability of the
     * file within the mount configuration (Part 20, 4.2.1). For a directory
     * Object it defines whether CreateFile, CreateDirectory, Delete and
     * MoveOrCopy can change the directory content (like the write permission
     * of a POSIX directory: deleting a file checks its directory, not the
     * file). If NULL, UserWritable mirrors Writable. */
    UA_Boolean (*getUserWritable)(UA_Server *server, const UA_NodeId *sessionId,
                                  const UA_NodeId *nodeId, void *mountContext);

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
 *    Maximum number of bytes transferred by a single Read or Write Method
 *    call (default: 1 MByte). Published as the MaxByteStringLength Property
 *    of every file Object, which Part 20 defines for both directions.
 *    Longer read requests are truncated and clients continue reading at the
 *    advanced position; a longer Write is rejected with Bad_InvalidArgument,
 *    because truncating it would silently discard client data. */

typedef struct UA_FileTransferDriver {
    UA_Driver drv; /* Must be the first member */
} UA_FileTransferDriver;

/* Create a file transfer driver. The returned driver is heap-allocated and
 * must either be passed to UA_Server_addDriver() or released with its ``free``
 * callback. Returns NULL if the allocation fails. */
UA_EXPORT UA_FileTransferDriver *
UA_FileTransferDriver_new(const UA_KeyValueMap params);

/* Create a FileDirectoryType Object under parentNodeId (referenced with
 * HasComponent) and mirror the backend content below it. Subdirectories
 * become FileDirectoryType Objects and files become FileType Objects, both
 * referenced with Organizes (Part 20, 4.3). The driver must be attached to a
 * server.
 *
 * @param driver The file transfer driver
 * @param requestedNodeId The requested NodeId for the FileSystem Object.
 *        Passing UA_NODEID_NULL selects an unused NodeId in namespace 0.
 * @param parentNodeId The parent node of the FileSystem Object
 * @param browseName The BrowseName of the FileSystem Object. Part 20 mandates
 *        the name 0:"FileSystem" for the root of an exposed directory
 *        structure; passing an empty BrowseName selects it. Another name is
 *        accepted -- two mounts below the same parent need distinct names --
 *        but is logged as a warning, because it puts the address space outside
 *        the Part 20 conformance unit.
 * @param backend The storage backend for this mount. The struct is copied and
 *        the driver takes ownership of it. Its clear callback is called
 *        automatically when the mount is removed, when the driver is freed,
 *        or right away when adding the mount fails.
 * @param options Mount options. NULL selects the defaults (writable,
 *        unlimited scan).
 * @param outFileSystemNodeId The created FileSystem Object (can be NULL)
 * @return The StatusCode of the operation */
UA_EXPORT UA_StatusCode
UA_FileTransferDriver_addFileSystem(UA_FileTransferDriver *driver,
                                    const UA_NodeId requestedNodeId,
                                    const UA_NodeId parentNodeId,
                                    const UA_QualifiedName browseName,
                                    const UA_FileTransferBackend *backend,
                                    const UA_FileTransferMountOptions *options,
                                    UA_NodeId *outFileSystemNodeId);

/* Create a new FileType Object for a single backend file. Useful to expose an
 * individual file (configuration, firmware, log) without exposing a
 * directory.
 *
 * @param driver The file transfer driver
 * @param requestedNodeId The requested NodeId for the file Object. Passing
 *        UA_NODEID_NULL selects an unused NodeId in namespace 0.
 * @param parentNodeId The parent node of the file Object (referenced with
 *        HasComponent)
 * @param browseName The BrowseName of the file Object
 * @param backend The storage backend for this file. Copied and owned by the
 *        driver as for UA_FileTransferDriver_addFileSystem().
 * @param path The backend path of the file. Must exist.
 * @param options Mount options. NULL selects the defaults.
 * @param outFileNodeId The created file Object (can be NULL)
 * @return The StatusCode of the operation */
UA_EXPORT UA_StatusCode
UA_FileTransferDriver_addFile(UA_FileTransferDriver *driver,
                              const UA_NodeId requestedNodeId,
                              const UA_NodeId parentNodeId,
                              const UA_QualifiedName browseName,
                              const UA_FileTransferFileBackend *backend,
                              const UA_String path,
                              const UA_FileTransferMountOptions *options,
                              UA_NodeId *outFileNodeId);

/* Remove a mount created with UA_FileTransferDriver_addFileSystem() or
 * UA_FileTransferDriver_addFile(). Open handles are closed and the Objects are
 * removed from the address space. The backend storage content is not touched.
 *
 * @param driver The file transfer driver
 * @param nodeId The FileSystem Object or the file Object of the mount
 * @return The StatusCode of the operation. Bad_NotFound for other nodes. */
UA_EXPORT UA_StatusCode
UA_FileTransferDriver_remove(UA_FileTransferDriver *driver,
                             const UA_NodeId nodeId);

/* Reconcile a mounted directory (or the entire mount when called with the
 * FileSystem Object) with the backend content: Objects are created for new
 * entries and removed for vanished entries. Vanished files with open handles
 * are removed from the address space once the last handle is closed.
 *
 * @param driver The file transfer driver
 * @param directoryNodeId A directory Object of a mount
 * @return The StatusCode of the operation */
UA_EXPORT UA_StatusCode
UA_FileTransferDriver_refresh(UA_FileTransferDriver *driver,
                              const UA_NodeId directoryNodeId);

_UA_END_DECLS

#endif /* UA_ENABLE_DRIVER_FILE_TRANSFER */

#endif /* UA_DRIVER_FILE_TRANSFER_H_ */
