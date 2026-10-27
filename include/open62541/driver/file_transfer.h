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
 * Serves a file or a directory tree with the FileType and FileDirectoryType
 * Methods of OPC UA Part 20. Each driver owns one storage backend; the built-in
 * backends serve local files and directories.
 *
 * The driver needs ``UA_ENABLE_DRIVER_FILE_TRANSFER``. The reduced and the full
 * Namespace Zero contain the required types; with MINIMAL or NONE, load them
 * before constructing a driver. :doc:`tutorial_server_filetransfer` shows the
 * setup.
 *
 * Storage Backend
 * ~~~~~~~~~~~~~~~
 *
 * ``UA_FileTransferFileBackend`` serves one file, ``UA_FileTransferBackend`` a
 * directory tree. Each driver needs a backend context of its own. The
 * callbacks return OPC UA StatusCodes. Paths are UTF-8 and relative to the
 * backend root, with '/' as separator; the empty path is the root.
 *
 * Every backend needs open, close, read, getPosition, setPosition and getInfo,
 * a directory backend also listDirectory. Unless ``read-only`` is set, write
 * is needed as well, and for directories create, remove and rename. A missing
 * callback fails the construction or the start with Bad_InvalidArgument.
 *
 * Backend handles must be unique within the backend, each with a position of
 * its own. Client handles belong to their Session and close with it.
 *
 * The callbacks run synchronously with the server lock held and must not block
 * for long. Scans and recursive copies or deletes run in one call, so they take
 * longer for larger trees. */

/* Access rights of one Object as a bit mask. For files, read and write give
 * access to the content. For directories, read lists the entries, write
 * changes them and traverse gives access to the children. Every managed
 * ancestor needs traverse. Changing a directory needs write and traverse; a
 * recursive copy needs read and traverse on the directories and read on the
 * files. Traverse has no meaning for files.
 *
 * An operation needs the rights of the storage, of the read-only configuration
 * and of the optional access callbacks together. They apply to the file
 * operations and to Writable/UserWritable; browsing stays with
 * accessControl.allowBrowseNode. Owner, group and ACL checks are up to the
 * backend. */
typedef UA_Byte UA_FileAccessRights;

#define UA_FILEACCESS_TRAVERSE ((UA_FileAccessRights)0x01)
#define UA_FILEACCESS_WRITE    ((UA_FileAccessRights)0x02)
#define UA_FILEACCESS_READ     ((UA_FileAccessRights)0x04)

/* Maximum length of one file name in UTF-8 bytes, without the terminator. It
 * is a limit of the API, not of the filesystem. Never truncate a name. */
#define UA_FILETRANSFER_FILENAME_MAX 1023

typedef struct {
    /* NUL-terminated UTF-8 base name; empty for a directory root */
    char name[UA_FILETRANSFER_FILENAME_MAX + 1];
    UA_UInt64 size;             /* Size in bytes (files only) */
    UA_Boolean sizeUnknown;     /* The Size Property then reports
                                 * Bad_NotSupported (Part 20, 4.2.1) */
    UA_DateTime lastModified;   /* Time of the last change */
    UA_Boolean isDirectory;
    UA_FileAccessRights accessRights; /* Rights of the storage. The access
                                      * callbacks can restrict them further. */
    char mimeType[255];         /* Media type, padded with zeros; it can use all
                                * bytes. Empty adds no MimeType Property and
                                * keeps an existing one. */
} UA_FileTransferFileInfo;

/* Who opens a file. For Open, CreateFile, GenerateFileForRead/-Write, the
 * copies of MoveOrCopy and UA_FileTransferDriver_openHandle(), the driver
 * passes the Session of the Method call. Calls of the application pass
 * NULL. */
typedef struct {
    UA_Server *server;
    const UA_NodeId *sessionId;
    void *sessionContext;
    void *openContext; /* Of UA_FileTransferDriver_openHandle(), else NULL */
} UA_FileTransferOpenContext;

typedef struct UA_FileTransferFileBackend UA_FileTransferFileBackend;
struct UA_FileTransferFileBackend {
    /* State of the backend, e.g. its storage and handles */
    void *context;

    /* Open a file and return a new handle with its own position. mode is a bit
     * mask of UA_OPENFILEMODE_* flags that the driver has validated. Append
     * starts at the end of the file, otherwise the position is zero;
     * EraseExisting empties the file. context is the caller or NULL. For a
     * Method call, the driver has already checked the mode, the locks (Part
     * 20, 4.2.2), the access rights and the handle limits. The result goes
     * back to the client, e.g. Bad_NotSupported for a mode that the file does
     * not offer. */
    UA_StatusCode (*open)(UA_FileTransferFileBackend *b, const UA_String path,
                          UA_Byte mode, const UA_FileTransferOpenContext *context,
                          UA_UInt32 *handle);

    /* Close the handle and flush buffered writes. Report flush errors here; at
     * the target of a copy, such an error fails the copy. */
    UA_StatusCode (*close)(UA_FileTransferFileBackend *b, UA_UInt32 handle);

    /* Read up to length bytes and advance the position. Allocate out; the
     * caller frees it with UA_ByteString_clear(). An empty out marks the end of
     * the file. */
    UA_StatusCode (*read)(UA_FileTransferFileBackend *b, UA_UInt32 handle,
                          UA_Int32 length, UA_ByteString *out);

    /* Write all data and advance the position. The buffer stays with the
     * caller. */
    UA_StatusCode (*write)(UA_FileTransferFileBackend *b, UA_UInt32 handle,
                           const UA_ByteString data);

    /* Get the position of the handle */
    UA_StatusCode (*getPosition)(UA_FileTransferFileBackend *b, UA_UInt32 handle,
                                 UA_UInt64 *outPosition);

    /* Set the position of the handle. A position beyond the end of the file
     * becomes the file size (Part 20, 4.2.7). */
    UA_StatusCode (*setPosition)(UA_FileTransferFileBackend *b, UA_UInt32 handle,
                                 UA_UInt64 position);

    /* Get the metadata, including the base name (empty for a directory root).
     * outInfo is zeroed. Set accessRights explicitly, zero denies access. A
     * name that is too long returns Bad_EncodingLimitsExceeded. */
    UA_StatusCode (*getInfo)(UA_FileTransferFileBackend *b, const UA_String path,
                             UA_FileTransferFileInfo *outInfo);

    /* Optional restriction that does not depend on the client. outRights
     * starts at zero. An error denies access and goes back to the caller. NULL
     * adds no restriction. b->context can hold the policy. */
    UA_StatusCode (*getAccessRights)(UA_FileTransferFileBackend *b,
                                     UA_Server *server, const UA_NodeId *nodeId,
                                     UA_FileAccessRights *outRights);

    /* Optional restriction per Session, with the Session and its context from
     * the Method or Property call. The rules of getAccessRights apply. Open
     * and every Read and Write check the rights again; Close still works after
     * they were revoked. With either access callback, recursive operations
     * reject entries without an Object, because they cannot be checked. */
    UA_StatusCode (*getUserAccessRights)(UA_FileTransferFileBackend *b,
                                         UA_Server *server,
                                         const UA_NodeId *sessionId,
                                         void *sessionContext,
                                         const UA_NodeId *nodeId,
                                         UA_FileAccessRights *outRights);

    /* Release the context when the driver is freed. Optional, called once. */
    void (*clear)(UA_FileTransferFileBackend *b);
};

/* Report one entry with complete metadata and a non-empty, NUL-terminated base
 * name. info is only valid during the callback; copy the struct to keep it. */
typedef void
(*UA_FileTransferListCallback)(void *listContext,
                               const UA_FileTransferFileInfo *info);

typedef struct UA_FileTransferBackend UA_FileTransferBackend;
struct UA_FileTransferBackend {
    /* The file operations, first so that b can be cast back to this struct */
    UA_FileTransferFileBackend file;

    /* List the entries at path. Return an error if listing or reading an entry
     * fails, also when an entry disappears meanwhile. An error discards the
     * whole listing, because an incomplete one could lose data during a move.
     * Never truncate names that are too long; return
     * Bad_EncodingLimitsExceeded. */
    UA_StatusCode (*listDirectory)(UA_FileTransferBackend *b, const UA_String path,
                                   UA_FileTransferListCallback cb, void *listContext);

    /* Create an empty entry at path and fail if it exists. info is only valid
     * during the call: name is the base name of path and isDirectory selects
     * the kind. The other fields are zero and ignored; the backend picks the
     * metadata. */
    UA_StatusCode (*create)(UA_FileTransferBackend *b, const UA_String path,
                            const UA_FileTransferFileInfo *info);

    /* Remove a file or an empty directory. The driver removes trees from the
     * bottom up. */
    UA_StatusCode (*remove)(UA_FileTransferBackend *b, const UA_String path);

    /* Move within this backend. Never overwrite or merge an existing target,
     * even one without an Object; return Bad_BrowseNameDuplicated. The driver
     * does not move entries with open handles. */
    UA_StatusCode (*rename)(UA_FileTransferBackend *b, const UA_String fromPath,
                            const UA_String toPath);

    /* Optional copy of a file within this backend; without it, the driver
     * copies with open, read, write and close. Trees are copied entry by
     * entry. As for rename, never overwrite a target or copy an entry with
     * open handles, and remove a partial target on failure. */
    UA_StatusCode (*copy)(UA_FileTransferBackend *b, const UA_String fromPath,
                          const UA_String toPath);
};

/**
 * Construction and Parameters
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * Set ``driver->params`` before UA_Server_addDriver(). The driver reads them at
 * every start; all keys are in namespace 0. A known key with another type than
 * listed fails the start with Bad_InvalidArgument. Intervals are milliseconds.
 *
 * 0:read-only [Boolean]
 *    Refuse writes and changes to directories (default: false).
 * 0:max-scan-depth [UInt32]
 *    How deep the tree is mirrored; the entries of the root have depth 1, and
 *    directories at the limit look empty. 0 means no limit (default).
 *    CreateFile, CreateDirectory and MoveOrCopy return Bad_ResourceUnavailable
 *    for targets beyond the limit. A lower limit keeps deeper Objects, but no
 *    longer refreshes them.
 * 0:max-nodes [UInt32]
 *    Maximum number of mirrored Objects, the root included; 0 means no limit
 *    (default). A lower limit does not remove existing Objects.
 * 0:namespace-index [UInt16]
 *    Namespace of the BrowseNames and NodeIds of new entries (default: the
 *    namespace of the root Object, 1 for a root in Namespace Zero). Standard
 *    Properties and Methods stay in namespace 0.
 * 0:refresh-interval [Double]
 *    How often directories are compared with the backend (default: 1000). 0
 *    turns the periodic refresh off; then call UA_FileTransferDriver_refresh().
 *    A file that disappears while it is open stays until it is closed.
 * 0:max-open-handles-per-session [UInt16]
 *    Open handles per Session in this driver (default: 64).
 * 0:max-open-handles-per-file [UInt16]
 *    Open handles per file (default: 16). When either limit is reached, Open
 *    returns Bad_ResourceUnavailable. For temporary drivers, each transfer
 *    counts as one handle, and a write transfer always runs alone.
 * 0:activity-timeout [Double]
 *    Close a handle like Close when it sees no Method call for this long, e.g.
 *    for the ActivityTimeout of a TrustList (Part 12, 7.8.2.1). The FileType
 *    Methods, openHandle and getHandleInfo count as calls. A handle closes at
 *    the latest after twice the timeout. 0 turns it off (default). Temporary
 *    drivers publish it as ClientProcessingTimeout (default: 60000) and cancel
 *    idle transfers.
 * 0:max-transfer-size [UInt64]
 *    Temporary drivers: largest temporary file in bytes (default: 16777216, 0
 *    means no limit). Larger files fail with Bad_ResourceUnavailable.
 * 0:max-read-length [UInt32]
 *    Maximum bytes per Read or Write (default, also for 0: 1048576), published
 *    as MaxByteStringLength. Longer reads return this many bytes; longer
 *    writes fail with Bad_InvalidArgument.
 *
 * Each refresh lists every mirrored directory with the server lock held and
 * applies max-nodes in two passes, removals first. For large or deep trees,
 * set max-scan-depth and max-nodes and use a longer refresh-interval, or 0 and
 * UA_FileTransferDriver_refresh(). Skipped entries and trees cut off by the
 * limits are logged at the start and when their numbers change; unchanged
 * results only at debug level. */

/* Description of the root Object. Fields left zero select the defaults. The
 * description is only used during the construction. */
typedef struct {
    UA_NodeId nodeId;          /* Existing Object to reuse, or NodeId to create.
                               * Null creates a NodeId in namespace 1. */
    UA_NodeId parentNodeId;    /* Default: Objects folder */
    UA_NodeId referenceTypeId; /* Default: HasComponent */
    UA_QualifiedName browseName; /* Default: base name (file) or 0:FileSystem */
    UA_NodeId typeDefinition;  /* Default: FileType or FileDirectoryType */
    UA_ObjectAttributes attributes; /* Default Object attributes; an empty
                                    * DisplayName uses the BrowseName */
} UA_FileTransferNodeDescription;

/* Construct a stopped driver for an existing file or directory of a backend.
 * newDirectory serves the empty path. description can be NULL for the
 * defaults. The default BrowseName of a file is the base name of the path in
 * namespace 1; for the empty path, it is the name from getInfo or "File".
 *
 * An existing description->nodeId reuses that FileType or FileDirectoryType
 * Object (or a subtype); the other fields of the description are ignored.
 * Children are matched by the text of their BrowseName in any namespace.
 * Ambiguous or incompatible entries are rejected; unrelated nodes stay
 * untouched.
 *
 * The bound Objects get the driver as context, also while it is stopped. A
 * reused root is bound at the first start. Freeing the driver removes the
 * nodes it created and restores the contexts of reused Objects and the values,
 * sources and contexts of their Properties. Reused Objects stay when their
 * storage entry disappears, but Delete and successful moves delete them like
 * created ones. Before an Object is deleted, the context from its construction
 * or reuse is restored for the lifecycle destructors of the application.
 *
 * While started, the drivers serve the standard Methods: the shared Method
 * nodes of Namespace Zero and the copies that subtypes inherit. Other Methods
 * of subtypes stay with the application, see
 * UA_FileTransferDriver_getHandleInfo(). The drivers own these Method nodes:
 * the first start replaces callbacks of the application, the last stop
 * removes them. FileType and TemporaryFileTransferType Objects that the
 * application serves without a driver need Method nodes of their own.
 *
 * On success, the backend struct is copied and the driver owns its context. On
 * failure, the created nodes are removed, *outDriver is NULL and the backend
 * stays with the caller. outDriver is required; outNodeId is optional and
 * receives an owned NodeId only on success.
 *
 * Registering is a separate step. Before UA_Server_addDriver() succeeded, the
 * caller frees the driver before deleting the server; afterwards, the server
 * owns it. To free it earlier, stop and remove it first. The start mirrors the
 * directory entries, and the periodic refresh (unless refresh-interval is 0)
 * keeps them up to date. Stop closes the handles but keeps the nodes and the
 * backend for a restart. */
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

/* Compare the directory tree of a started directory driver with its backend
 * now, e.g. when the periodic refresh is off. Returns Bad_InvalidState for a
 * stopped driver and Bad_NotSupported for other drivers. */
UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_FileTransferDriver_refresh(UA_Driver *driver);

/* FileType subtypes can define Methods on an open file, e.g. CloseAndUpdate of
 * the PubSubConfiguration (Part 14). These Methods stay with the application,
 * which also implements the backend of the file. getHandleInfo turns the
 * FileHandle of the client into the open mode and the backend handle;
 * closeHandle closes it like the Close Method and returns the result of the
 * backend close. A handle only works for the Session and the file Object that
 * opened it; other handles return Bad_InvalidArgument. Both can be called from
 * Method callbacks. */
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

/* Subtype Methods that open the file with their own arguments, e.g.
 * OpenWithMasks of a TrustList (Part 12), create the FileHandle with
 * openHandle; openContext reaches the backend open, e.g. with the masks.
 * Methods that take file NodeIds as arguments, e.g. ValidateFiles of DI (OPC
 * 10000-100), open the files the same way and read them through the backend.
 * The checks and results of the Open Method apply; a stopped driver returns
 * Bad_InvalidState and a node that is not a file of the driver
 * Bad_InvalidArgument. The handle belongs to the Session like one from Open:
 * it counts in OpenCount and the handle limits and closes with the Session.
 * Return fileHandle to the client, or close it with closeHandle if the Method
 * fails afterwards. backendHandle is optional. */
UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_FileTransferDriver_openHandle(UA_Driver *driver,
                                 const UA_NodeId fileNodeId,
                                 const UA_NodeId *sessionId,
                                 void *sessionContext, UA_Byte mode,
                                 void *openContext, UA_UInt32 *fileHandle,
                                 UA_UInt32 *backendHandle);

/**
 * Local Backends
 * ~~~~~~~~~~~~~~ */

/* Serve an existing local directory; rootPath is UTF-8 and copied. out is only
 * written on success. The rights follow the filesystem permissions of the
 * server process, limited further by the optional access callbacks. On
 * Windows, the filesystem operations check the traversal.
 *
 * Paths with ".." segments are rejected, but links to regular files are
 * followed, also outside rootPath: this is no sandbox, not even with
 * read-only. Links to directories and special files are skipped; directories
 * that contain them cannot be deleted through the driver (Bad_InvalidState).
 * Windows rejects names it cannot represent. On 32-bit POSIX, positions are
 * limited to 2 GiB. */
UA_EXPORT UA_StatusCode
UA_FileTransferBackend_localDirectory(const UA_String rootPath,
                                      UA_FileTransferBackend *out);

/* Serve one existing regular file with the same local I/O and permissions.
 * filePath is UTF-8 and copied; out is only written on success. Links to
 * regular files are followed. Only the empty backend path is accepted; getInfo
 * reports the base name and the media type. Pass UA_STRING_NULL as path to
 * newFile(). */
UA_EXPORT UA_StatusCode
UA_FileTransferFileBackend_localFile(const UA_String filePath,
                                     UA_FileTransferFileBackend *out);

/**
 * Temporary File Transfer
 * ~~~~~~~~~~~~~~~~~~~~~~~
 * A TemporaryFileTransferType Object (Part 20, 4.4) creates a file for one
 * transfer. For GenerateFileForRead, the application fills the file and the
 * client reads it. For GenerateFileForWrite, the client writes the file and
 * CloseAndCommit hands it to the application; Close cancels. Only the Session
 * that generated a file can use it.
 *
 * The driver cannot know the content, so the application provides and takes
 * it in callbacks. They run with the server lock held and must not stop or
 * remove the driver.
 *
 * - generateFileForRead fills the new file.
 * - generateFileForWrite can refuse a write, e.g. for unknown generateOptions.
 * - closeAndCommit takes the written file and returns the result of
 *   CloseAndCommit. The driver removes the file afterwards, so copy or move
 *   what you keep.
 * - endTransfer runs once when a transfer ends, also when its creation fails
 *   after a successful generate callback; free transferContext there. The
 *   status says how the transfer ended, e.g. Good, the result of
 *   closeAndCommit, Bad_RequestCancelledByClient, Bad_Timeout or
 *   Bad_SessionClosed.
 *
 * See ``examples/filetransfer/server_filetransfer_temporary.c``. */

/* The temporary file of one transfer, only valid during the callback */
typedef struct {
    const UA_NodeId *sessionId;
    void *sessionContext;              /* NULL in endTransfer */
    const UA_Variant *generateOptions; /* Can be an empty Variant */
    UA_FileTransferFileBackend *store;
    UA_String path;                    /* Of the file in the store */
    UA_Boolean forWrite;
    void *transferContext;             /* Of the application, initially NULL */
} UA_FileTransferTemporaryFile;

typedef UA_StatusCode
(*UA_FileTransferTemporaryCallback)(UA_Server *server, void *context,
                                    UA_FileTransferTemporaryFile *file);

typedef void
(*UA_FileTransferTemporaryEndCallback)(UA_Server *server, void *context,
                                       UA_FileTransferTemporaryFile *file,
                                       UA_StatusCode status);

/* Set the content of the temporary file, get a copy of it, or copy another
 * file of the store into it. Only valid during a callback. */
UA_EXPORT UA_StatusCode
UA_FileTransferTemporaryFile_setContent(UA_FileTransferTemporaryFile *file,
                                        const UA_ByteString content);

UA_EXPORT UA_StatusCode
UA_FileTransferTemporaryFile_getContent(UA_FileTransferTemporaryFile *file,
                                        UA_ByteString *content);

UA_EXPORT UA_StatusCode
UA_FileTransferTemporaryFile_copyFrom(UA_FileTransferTemporaryFile *file,
                                      const UA_String path);

typedef struct {
    UA_FileTransferTemporaryCallback generateFileForRead;  /* NULL: Bad_NotReadable */
    UA_FileTransferTemporaryCallback generateFileForWrite; /* Optional */
    UA_FileTransferTemporaryCallback closeAndCommit;       /* NULL: Bad_NotWritable */
    UA_FileTransferTemporaryEndCallback endTransfer;       /* Optional */
    void *context;                                         /* Passed to the callbacks */
    /* Where the temporary files are kept; NULL keeps them in memory. It needs
     * the file operations, create and remove. To rename or copy, e.g. to keep
     * a written file, cast file->store back to UA_FileTransferBackend. The
     * access callbacks only see the random NodeIds of the temporary files, so
     * authorize the generate Methods in the access control plugin. */
    const UA_FileTransferBackend *store;
} UA_FileTransferTemporaryOptions;

/* Construct a stopped driver for a TemporaryFileTransferType Object (default
 * BrowseName 1:TemporaryFileTransfer). generateFileForRead or closeAndCommit is
 * required. The rules of UA_FileTransferDriver_newFile() apply to the
 * description and the store. read-only refuses GenerateFileForWrite;
 * max-scan-depth, max-nodes and refresh-interval do not apply. */
UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_FileTransferDriver_newTemporary(UA_Server *server,
                                   const UA_FileTransferTemporaryOptions *options,
                                   const UA_FileTransferNodeDescription *description,
                                   UA_NodeId *outNodeId, UA_Driver **outDriver);

/* For subtype Methods that take the FileHandle of a running transfer, e.g.
 * SendProductionDatasetInformation of EUROMAP 83: get the direction and the
 * transferContext of the transfer (both optional), or replace the context.
 * Unknown handles and those of other Sessions return Bad_InvalidArgument. Both
 * count as calls for the activity-timeout and can be called from Method
 * callbacks. */
UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_FileTransferDriver_getTransfer(UA_Driver *driver, const UA_NodeId *sessionId,
                                  UA_UInt32 fileHandle, UA_Boolean *forWrite,
                                  void **transferContext);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_FileTransferDriver_setTransferContext(UA_Driver *driver, const UA_NodeId *sessionId,
                                         UA_UInt32 fileHandle, void *transferContext);

/* Start a transfer like GenerateFileForRead or GenerateFileForWrite, e.g. in a
 * Method that returns a temporary file. The callbacks and limits apply;
 * generateOptions can be NULL. The client ends the transfer with Close, or
 * commits a write with CloseAndCommit of the Object. fileNodeId receives an
 * owned copy. Can be called from Method callbacks. */
UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_FileTransferDriver_generateFile(UA_Driver *driver, const UA_NodeId *sessionId,
                                   void *sessionContext, UA_Boolean forWrite,
                                   const UA_Variant *generateOptions,
                                   UA_NodeId *fileNodeId, UA_UInt32 *fileHandle);

_UA_END_DECLS

#endif /* UA_DRIVER_FILE_TRANSFER_H_ */
