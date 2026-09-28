/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#ifndef UA_DRIVER_FILE_TRANSFER_INTERNAL_H_
#define UA_DRIVER_FILE_TRANSFER_INTERNAL_H_

#include <open62541/driver/file_transfer.h>

#include "ziptree.h"

_UA_BEGIN_DECLS

#define UA_DRIVER_FILE_TRANSFER_NAME "file-transfer"

#define UA_FILETRANSFER_MAXHANDLESPERSESSION_DEFAULT 64
#define UA_FILETRANSFER_MAXHANDLESPERFILE_DEFAULT 16
#define UA_FILETRANSFER_MAXREADLENGTH_DEFAULT (1 << 20) /* 1 MByte */

#define UA_FILETRANSFER_OPENMODE_ALLBITS                        \
    (UA_OPENFILEMODE_READ | UA_OPENFILEMODE_WRITE |             \
     UA_OPENFILEMODE_ERASEEXISTING | UA_OPENFILEMODE_APPEND)

#define UA_FILETRANSFER_COPYCHUNKSIZE 65536

typedef struct FileTransferDriver FileTransferDriver;
typedef struct FTEntry FTEntry;

/* One entry per fileHandle returned by the Open Method. Handles are bound to
 * the Session that created them. */
typedef struct FTHandle {
    ZIP_ENTRY(FTHandle) idTreeEntry;
    ZIP_ENTRY(FTHandle) sessionTreeEntry;
    UA_UInt32 handle; /* Globally unique, never 0 */
    UA_NodeId sessionId;
    FTEntry *file;
    UA_Byte mode; /* UA_OPENFILEMODE_* bit mask, matching the Open Method Byte */
    UA_UInt32 backendHandle;
} FTHandle;

ZIP_HEAD(FTEntriesById, FTEntry);
ZIP_HEAD(FTChildrenByName, FTEntry);
ZIP_HEAD(FTHandlesById, FTHandle);
ZIP_HEAD(FTHandlesBySession, FTHandle);

/* One entry per driver-managed FileType/FileDirectoryType Object.
 * The Object context points to the owning driver. */
struct FTEntry {
    ZIP_ENTRY(FTEntry) idTreeEntry;
    ZIP_ENTRY(FTEntry) nameTreeEntry;
    FTEntry *parent; /* Storage parent, independent of OPC UA references */
    struct FTChildrenByName children;
    UA_String name; /* Basename view into path; permits retained equal names */
    UA_NodeId nodeId;
    FileTransferDriver *driver;
    UA_String path; /* Relative to the mount root */
    UA_Boolean isDirectory;
    UA_Boolean created;         /* Delete only Objects created by this driver */
    UA_Boolean contextBound;
    void *savedObjectContext;  /* Restore when releasing a reused Object */
    UA_Boolean zombie; /* Retained until the last open handle is closed */
    UA_Boolean methodsBound; /* Also set when all Methods use standard declarations */
    UA_NodeId *methods; /* Nonstandard Method ids; standard slots remain null */
    UA_UInt32 subtreeHandleCount; /* OpenCount for files; sum below directories */
    UA_Boolean openForWrite; /* Files only */
    UA_Boolean scanSeen; /* Temporary mark during directory reconciliation */
    struct FTFileBinding *binding; /* Files only; owned by the model layer */
};

/* Configuration read from drv.params on start. */
typedef struct {
    UA_Boolean readOnly;
    UA_UInt32 maxScanDepth;
    UA_UInt32 maxNodes;
    UA_UInt16 namespaceIndex;
    UA_Double refreshInterval;
    UA_UInt16 maxHandlesPerSession;
    UA_UInt16 maxHandlesPerFile;
    UA_UInt32 maxReadLength;
} FTConfig;

/* One driver owns one backend and one root Object for its entire lifetime. */
struct FileTransferDriver {
    UA_Driver driver; /* Must be the first member */
    UA_FileTransferBackend backend;
    FTConfig config;
    FTEntry *root;
    UA_UInt64 refreshCallbackId;
    struct FTEntriesById entriesByNodeId;
    struct FTHandlesById handlesById;
    struct FTHandlesBySession handlesBySession;
    UA_UInt32 entryCount; /* Includes retained zombie entries */
    UA_UInt32 nextHandle;
};

static UA_INLINE enum ZIP_CMP
ftNodeIdOrder(const UA_NodeId *a, const UA_NodeId *b) {
    return (enum ZIP_CMP)UA_NodeId_order(a, b);
}

static UA_INLINE enum ZIP_CMP
ftNameOrder(const UA_String *a, const UA_String *b) {
    size_t length = a->length < b->length ? a->length : b->length;
    int cmp = length ? memcmp(a->data, b->data, length) : 0;
    if(cmp != 0)
        return cmp < 0 ? ZIP_CMP_LESS : ZIP_CMP_MORE;
    return a->length < b->length ? ZIP_CMP_LESS :
        (a->length > b->length ? ZIP_CMP_MORE : ZIP_CMP_EQ);
}

ZIP_FUNCTIONS(FTEntriesById, FTEntry, idTreeEntry, UA_NodeId, nodeId, ftNodeIdOrder)
ZIP_FUNCTIONS(FTChildrenByName, FTEntry, nameTreeEntry, UA_String, name, ftNameOrder)

/* driver.c -- implementation identity and lifecycle */
UA_Boolean isFileTransferDriver(const UA_Driver *driver);

/* model.c -- registry and information-model ownership */
FileTransferDriver *findEntryOwner(UA_Server *server, const UA_NodeId *objectId);
FTEntry *findFTEntry(FileTransferDriver *ftd, const UA_NodeId *nodeId);
FTEntry *newFTEntry(FileTransferDriver *ftd, FTEntry *parent,
                    const UA_NodeId nodeId, const UA_String path,
                    UA_Boolean isDirectory);
UA_Boolean isBelow(const FTEntry *node, const FTEntry *root);
/* Release bindings and remove driver-created Objects, including the root.
 * Reused Objects survive; callers must close all subtree handles first. */
void removeSubtree(UA_Server *server, FTEntry *subtreeRoot);

/* Validate an untrusted context against registered, started drivers before
 * resolving the Object. NodeId-only lookup is for cross-driver destinations. */
FTEntry *resolveFTEntry(UA_Server *server, const UA_NodeId *objectId,
                        void *objectContext);
FTEntry *resolveFTEntryById(UA_Server *server, const UA_NodeId *objectId);

UA_StatusCode checkFileTransferType(UA_Server *server,
                                    const UA_NodeId typeDefinition,
                                    UA_Boolean isDirectory);
UA_StatusCode checkFileTransferObject(UA_Server *server, const UA_NodeId nodeId,
                                      UA_Boolean isDirectory);
UA_StatusCode mirrorObject(UA_Server *server, FTEntry *directory,
                           const UA_String name,
                           const UA_FileTransferFileInfo *info, FTEntry **outNode);
UA_StatusCode bindObjectContext(UA_Server *server, FTEntry *node);
UA_StatusCode setupFileNode(UA_Server *server, FTEntry *node,
                            const UA_FileTransferFileInfo *info);
/* Idempotent rollback/release: Property callbacks must not outlive the entry. */
void releaseFileNode(UA_Server *server, FTEntry *node);

/* Shared type Methods and copied/subtype Methods have separate bindings. */
UA_StatusCode registerFileTransferMethodCallbacks(UA_Server *server);
void unregisterFileTransferMethodCallbacks(UA_Server *server);
UA_StatusCode bindObjectMethods(UA_Server *server, FTEntry *node);
void unbindObjectMethods(UA_Server *server, FTEntry *node);

/* Callbacks are private to file.c / directory.c; only descriptions are shared. */
typedef struct {
    UA_UInt32 methodId;
    const char *name;
    UA_MethodCallback callback;
} FTMethod;
extern const FTMethod fileTypeMethods[6];
extern const FTMethod fileDirectoryTypeMethods[4];

/* file.c -- permissions and Session handles */
/* Query the backend with a zero-initialized FileInfo. */
UA_StatusCode backendGetInfo(UA_FileTransferFileBackend *backend,
                             const UA_String path, UA_FileTransferFileInfo *info);
UA_StatusCode getFTAccessRights(UA_Server *server, const FTEntry *node,
                                const UA_NodeId *sessionId, void *sessionContext,
                                UA_FileAccessRights *outRights);
UA_StatusCode checkFTAccess(UA_Server *server, const FTEntry *node,
                            const UA_NodeId *sessionId, void *sessionContext,
                            UA_FileAccessRights required, UA_StatusCode deniedStatus);
FTHandle *findFTHandle(FileTransferDriver *ftd, const UA_NodeId *sessionId,
                       UA_UInt32 handle);
/* mode combines UA_OPENFILEMODE_* flags in the protocol's Byte representation. */
UA_StatusCode openFileHandle(UA_Server *server, FTEntry *node,
                             const UA_NodeId *sessionId, void *sessionContext,
                             UA_Byte mode, UA_UInt32 *outHandle);
UA_StatusCode closeFTHandle(UA_Server *server, FTHandle *handle);
void closeDriverHandles(UA_Server *server, FileTransferDriver *ftd);
void closeSessionHandles(UA_Server *server, FileTransferDriver *ftd,
                          const UA_NodeId *sessionId);

/* directory.c -- reconcile a managed directory with the server lock held. */
UA_StatusCode fileTransferRefresh(UA_Driver *driver,
                                  const UA_NodeId directoryNodeId);

/* Shared helpers independent of driver state. */
/* Entry names must not contain path separators or navigate the hierarchy */
static UA_INLINE UA_Boolean
validEntryName(const UA_String name) {
    if(name.length == 0 || name.length > UA_FILETRANSFER_FILENAME_MAX)
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
static UA_INLINE UA_StatusCode
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

static UA_INLINE UA_String
pathLastSegment(const UA_String path) {
    for(size_t i = path.length; i > 0; i--) {
        if(path.data[i - 1] == '/') {
            UA_String segment = {path.length - i, path.data + i};
            return segment;
        }
    }
    return path;
}

static UA_INLINE UA_Boolean
pathWithinSubtree(const UA_String path, const UA_String subtreeRoot) {
    if(subtreeRoot.length == 0)
        return true; /* The mount root contains everything */
    if(path.length < subtreeRoot.length ||
       memcmp(path.data, subtreeRoot.data, subtreeRoot.length) != 0)
        return false;
    return path.length == subtreeRoot.length ||
        path.data[subtreeRoot.length] == '/';
}

_UA_END_DECLS

#endif /* UA_DRIVER_FILE_TRANSFER_INTERNAL_H_ */
