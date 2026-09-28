/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include "file_transfer_internal.h"

#ifdef UA_ENABLE_DRIVER_FILE_TRANSFER

/**************************************
 * Driver Lookup and Notifications
 **************************************/

FileTransferDriver *
findFileTransferDriver(UA_Server *server) {
    for(UA_Driver *drv = UA_Server_getDrivers(server); drv; drv = drv->next) {
        if(drv->driverType == UA_DRIVERTYPE_FILE_TRANSFER &&
           drv->state == UA_LIFECYCLESTATE_STARTED)
            return (FileTransferDriver*)drv;
    }
    return NULL;
}

/**************************************
 * Registry and Handle Management
 **************************************/

FTNode *
findFTNode(FileTransferDriver *ftd, const UA_NodeId *nodeId) {
    FTNode *node;
    LIST_FOREACH(node, &ftd->nodes, listEntry) {
        if(UA_NodeId_equal(&node->nodeId, nodeId))
            return node;
    }
    return NULL;
}

FTHandle *
findFTHandle(FileTransferDriver *ftd, const UA_NodeId *sessionId,
             UA_UInt32 handle) {
    FTHandle *h;
    LIST_FOREACH(h, &ftd->handles, listEntry) {
        if(h->handle == handle && UA_NodeId_equal(&h->sessionId, sessionId))
            return h;
    }
    return NULL;
}

size_t
countSessionHandles(FileTransferDriver *ftd, const UA_NodeId *sessionId) {
    size_t count = 0;
    FTHandle *h;
    LIST_FOREACH(h, &ftd->handles, listEntry) {
        if(UA_NodeId_equal(&h->sessionId, sessionId))
            count++;
    }
    return count;
}

UA_UInt32
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

void
updateOpenCount(UA_Server *server, FTNode *node) {
    if(UA_NodeId_isNull(&node->openCountId))
        return;
    UA_Variant value;
    UA_Variant_setScalar(&value, &node->openCount, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_writeValue(server, node->openCountId, value);
}

void
removeFTNode(FileTransferDriver *ftd, FTNode *node) {
    LIST_REMOVE(node, listEntry);
    UA_NodeId_clear(&node->nodeId);
    UA_NodeId_clear(&node->openCountId);
    UA_String_clear(&node->path);
    UA_free(node);
}

/* Close the backend handle and release the handle. Removes zombie nodes once
 * their last handle is closed. */
UA_StatusCode
closeFTHandle(UA_Server *server, FileTransferDriver *ftd, FTHandle *h) {
    FTNode *node = h->file;
    UA_FileTransferFileBackend *b = &node->mount->backend.file;
    UA_StatusCode res = b->close(b, h->backendHandle);

    LIST_REMOVE(h, listEntry);
    UA_NodeId_clear(&h->sessionId);
    if(h->mode & UA_OPENFILEMODE_WRITE)
        node->openForWrite = false;
    if(node->openCount > 0)
        node->openCount--;
    UA_free(h);

    if(node->zombie && node->openCount == 0)
        removeSubtree(server, ftd, node);
    else
        updateOpenCount(server, node);
    return res;
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
 * Method Registration
 **************************************/

/* The FileType and FileDirectoryType Methods with their BrowseName */
typedef struct {
    UA_UInt32 methodId;
    const char *name;
    UA_MethodCallback callback;
} FTMethod;

static const FTMethod fileTypeMethods[] = {
    {UA_NS0ID_FILETYPE_OPEN, "Open", openMethodCallback},
    {UA_NS0ID_FILETYPE_CLOSE, "Close", closeMethodCallback},
    {UA_NS0ID_FILETYPE_READ, "Read", readMethodCallback},
    {UA_NS0ID_FILETYPE_WRITE, "Write", writeMethodCallback},
    {UA_NS0ID_FILETYPE_GETPOSITION, "GetPosition", getPositionMethodCallback},
    {UA_NS0ID_FILETYPE_SETPOSITION, "SetPosition", setPositionMethodCallback}
};

static const FTMethod fileDirectoryTypeMethods[] = {
    {UA_NS0ID_FILEDIRECTORYTYPE_CREATEDIRECTORY, "CreateDirectory",
     createDirectoryMethodCallback},
    {UA_NS0ID_FILEDIRECTORYTYPE_CREATEFILE, "CreateFile", createFileMethodCallback},
    {UA_NS0ID_FILEDIRECTORYTYPE_DELETEFILESYSTEMOBJECT, "Delete",
     deleteMethodCallback},
    {UA_NS0ID_FILEDIRECTORYTYPE_MOVEORCOPY, "MoveOrCopy", moveOrCopyMethodCallback}
};

#define UA_FTMETHODS_SIZE(methods) (sizeof(methods) / sizeof(methods[0]))

/* The callbacks are attached to the Namespace Zero type declarations. With the
 * default configuration (copyMethodsOnInstances false) an Object instance
 * references the type's Method nodes instead of copying them, so one
 * registration serves every FileType/FileDirectoryType instance. The flip side
 * is that this is server-global state: it claims the Part 20 Methods for the
 * driver, and it has to be released again when the driver stops. */
static UA_StatusCode
setFileTransferMethodCallbacks(UA_Server *server, UA_Boolean install) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < UA_FTMETHODS_SIZE(fileTypeMethods); i++) {
        res = UA_Server_setMethodNodeCallback(
            server, UA_NODEID_NUMERIC(0, fileTypeMethods[i].methodId),
            install ? fileTypeMethods[i].callback : NULL);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }
    for(size_t i = 0; i < UA_FTMETHODS_SIZE(fileDirectoryTypeMethods); i++) {
        res = UA_Server_setMethodNodeCallback(
            server, UA_NODEID_NUMERIC(0, fileDirectoryTypeMethods[i].methodId),
            install ? fileDirectoryTypeMethods[i].callback : NULL);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }
    return res;
}

UA_StatusCode
registerFileTransferMethodCallbacks(UA_Server *server) {
    return setFileTransferMethodCallbacks(server, true);
}

void
unregisterFileTransferMethodCallbacks(UA_Server *server) {
    setFileTransferMethodCallbacks(server, false);
}

UA_StatusCode
bindObjectMethods(UA_Server *server, const FTNode *node) {
    if(!UA_Server_getConfig(server)->copyMethodsOnInstances)
        return UA_STATUSCODE_GOOD;
    const FTMethod *methods = fileTypeMethods;
    size_t methodsSize = UA_FTMETHODS_SIZE(fileTypeMethods);
    if(node->isDirectory) {
        methods = fileDirectoryTypeMethods;
        methodsSize = UA_FTMETHODS_SIZE(fileDirectoryTypeMethods);
    }
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < methodsSize; i++) {
        UA_NodeId methodId;
        if(getChildId(server, node->nodeId, methods[i].name,
                      &methodId) != UA_STATUSCODE_GOOD)
            continue;
        UA_NodeId typeMethodId = UA_NODEID_NUMERIC(0, methods[i].methodId);
        UA_StatusCode methodRes = UA_STATUSCODE_GOOD;
        if(!UA_NodeId_equal(&methodId, &typeMethodId))
            methodRes = UA_Server_setMethodNodeCallback(server, methodId,
                                                        methods[i].callback);
        UA_NodeId_clear(&methodId);
        if(res == UA_STATUSCODE_GOOD)
            res = methodRes; /* The first failure */
    }
    return res;
}

/**************************************
 * Public Driver API
 **************************************/

static const UA_FileTransferMountOptions defaultMountOptions =
    {false, 0, 0, 0, NULL, NULL};

/* The mounts can be managed once the driver is added to a server. The Methods
 * are served while the driver is started. */
static UA_StatusCode
checkDriverAdded(FileTransferDriver *ftd) {
    UA_Server *server = ftd->driver.drv.server;
    if(!server)
        return UA_STATUSCODE_BADINVALIDSTATE;
    if(!ftd->logging)
        ftd->logging = UA_Server_getConfig(server)->logging;
    return UA_STATUSCODE_GOOD;
}

/* The driver takes ownership of the backend. It is released when adding a
 * mount fails. */
static UA_StatusCode
releaseBackend(UA_FileTransferBackend *backend, UA_StatusCode res) {
    if(backend->file.clear)
        backend->file.clear(&backend->file);
    return res;
}

/* The backend may leave fields of the FileInfo unset */
UA_StatusCode
backendGetInfo(UA_FileTransferFileBackend *b, const UA_String path,
               UA_FileTransferFileInfo *info) {
    memset(info, 0, sizeof(UA_FileTransferFileInfo));
    return b->getInfo(b, path, info);
}

/* Validate that a backend implements the operations the mount can actually
 * reach. Requiring all of them unconditionally would force a read-only or
 * single-file backend -- content generated on the fly, a file in flash -- to
 * supply half a dozen stubs that only ever return Bad_NotSupported. */
UA_Boolean
backendComplete(const UA_FileTransferBackend *b, UA_Boolean standaloneFile,
                UA_Boolean readOnly) {
    /* Reading a file and moving within it is always possible */
    const UA_FileTransferFileBackend *fb = &b->file;
    if(!fb->open || !fb->close || !fb->read ||
       !fb->getPosition || !fb->setPosition || !fb->getInfo)
        return false;
    if(!readOnly && !fb->write)
        return false;
    /* A standalone file has no directory tree: no listing, and none of the
     * mutating directory operations can name it */
    if(standaloneFile)
        return true;
    if(!b->listDirectory)
        return false;
    /* CreateFile/CreateDirectory/Delete/MoveOrCopy are rejected with
     * Bad_UserAccessDenied on a read-only mount before reaching the backend */
    if(!readOnly && (!b->createFile || !b->createDirectory ||
                     !b->remove || !b->rename))
        return false;
    return true;
}

/* A namespaceIndex for the mirrored BrowseNames has to resolve in the server's
 * namespace array. An index that does not would put the Objects in a namespace
 * no client can interpret, and the mismatch would only show up on a browse. */
static UA_StatusCode
checkMountNamespace(UA_Server *server,
                    const UA_FileTransferMountOptions *options) {
    if(!options || options->namespaceIndex == 0)
        return UA_STATUSCODE_GOOD;
    UA_String uri = UA_STRING_NULL;
    UA_StatusCode res =
        UA_Server_getNamespaceByIndex(server, options->namespaceIndex, &uri);
    UA_String_clear(&uri);
    return (res == UA_STATUSCODE_GOOD) ?
        UA_STATUSCODE_GOOD : UA_STATUSCODE_BADINVALIDARGUMENT;
}

FTMount *
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

void
removeMount(FileTransferDriver *ftd, FTMount *mount) {
    if(mount->backend.file.clear)
        mount->backend.file.clear(&mount->backend.file);
    UA_NodeId_clear(&mount->rootNodeId);
    LIST_REMOVE(mount, listEntry);
    UA_free(mount);
}

FTNode *
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
void
closeMountHandles(UA_Server *server, FileTransferDriver *ftd, FTMount *mount) {
    FTHandle *h, *tmp;
    LIST_FOREACH_SAFE(h, &ftd->handles, listEntry, tmp) {
        if(h->file->mount == mount)
            closeFTHandle(server, ftd, h);
    }
}

static UA_StatusCode
addFileSystemLocked(UA_FileTransferDriver *driver,
                    const UA_NodeId requestedNodeId,
                    const UA_NodeId parentNodeId,
                    const UA_QualifiedName browseName,
                    UA_FileTransferBackend backend,
                    const UA_FileTransferMountOptions *options,
                    UA_NodeId *outFileSystemNodeId) {
    FileTransferDriver *ftd = (FileTransferDriver*)driver;
    UA_Driver *drv = &driver->drv;
    if(!backendComplete(&backend, false, options && options->readOnly))
        return releaseBackend(&backend, UA_STATUSCODE_BADINVALIDARGUMENT);
    UA_StatusCode res = checkDriverAdded(ftd);
    if(res == UA_STATUSCODE_GOOD)
        res = checkMountNamespace(drv->server, options);

    /* The backend root must be a directory */
    UA_FileTransferFileInfo info;
    if(res == UA_STATUSCODE_GOOD)
        res = backendGetInfo(&backend.file, UA_STRING_NULL, &info);
    if(res == UA_STATUSCODE_GOOD && !info.isDirectory)
        res = UA_STATUSCODE_BADINVALIDARGUMENT;
    if(res != UA_STATUSCODE_GOOD)
        return releaseBackend(&backend, res);

    FTMount *mount = newMount(ftd, backend, options, false);
    if(!mount)
        return releaseBackend(&backend, UA_STATUSCODE_BADOUTOFMEMORY);

    /* Part 20, 4.3.2: "The Object representing the root of a file directory
     * structure shall have the BrowseName FileSystem." An empty BrowseName
     * selects it. A different name is still accepted -- two mounts below the
     * same parent need distinct names -- but it puts the address space outside
     * the "Base Info FileDirectoryType Base" conformance unit, so say so
     * instead of deviating silently. */
    UA_QualifiedName rootName = browseName;
    UA_QualifiedName fileSystemName = UA_QUALIFIEDNAME(0, "FileSystem");
    if(rootName.name.length == 0) {
        rootName = fileSystemName;
    } else if(rootName.namespaceIndex != fileSystemName.namespaceIndex ||
              !UA_String_equal(&rootName.name, &fileSystemName.name)) {
        UA_LOG_WARNING(ftd->logging, UA_LOGCATEGORY_SERVER,
                       "FileTransfer: Creating the FileSystem root with the "
                       "BrowseName %u:\"%S\". Part 20 requires 0:\"FileSystem\" "
                       "for the root of an exposed directory structure",
                       (unsigned)rootName.namespaceIndex, rootName.name);
    }

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

    /* Mirror the backend content (eager scan). The root is one of the maxNodes
     * Objects of the mount. */
    FTNode *rootNode = newFTNode(ftd, mount, rootNodeId, UA_STRING_NULL, true);
    res = (rootNode) ? bindObjectMethods(drv->server, rootNode) :
        UA_STATUSCODE_BADOUTOFMEMORY;
    if(res == UA_STATUSCODE_GOOD) {
        UA_UInt32 nodeBudget = (mount->options.maxNodes > 0) ?
            mount->options.maxNodes - 1 : (UA_UInt32)0xffffffffu;
        res = fileTransferMirrorTree(drv->server, ftd, rootNode, 1, &nodeBudget);
    }
    if(res == UA_STATUSCODE_GOOD)
        res = UA_NodeId_copy(&rootNodeId, &mount->rootNodeId);
    if(res != UA_STATUSCODE_GOOD) {
        if(rootNode)
            removeSubtree(drv->server, ftd, rootNode);
        else
            UA_Server_deleteNode(drv->server, rootNodeId, true);
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

/* Remove a mount (a FileSystem root or a file Object) */
static UA_StatusCode
removeLocked(UA_FileTransferDriver *driver, const UA_NodeId nodeId) {
    FileTransferDriver *ftd = (FileTransferDriver*)driver;
    UA_Driver *drv = &driver->drv;
    UA_StatusCode res = checkDriverAdded(ftd);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    FTMount *mount = NULL;
    LIST_FOREACH(mount, &ftd->mounts, listEntry) {
        if(UA_NodeId_equal(&mount->rootNodeId, &nodeId))
            break;
    }
    if(!mount)
        return UA_STATUSCODE_BADNOTFOUND;

    /* Closing the last handle removes zombie nodes already */
    closeMountHandles(drv->server, ftd, mount);
    FTNode *rootNode = findFTNode(ftd, &mount->rootNodeId);
    if(rootNode)
        removeSubtree(drv->server, ftd, rootNode);
    removeMount(ftd, mount);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
addFileLocked(UA_FileTransferDriver *driver, const UA_NodeId requestedNodeId,
              const UA_NodeId parentNodeId, const UA_QualifiedName browseName,
              UA_FileTransferBackend backend, const UA_String path,
              const UA_FileTransferMountOptions *options,
              UA_NodeId *outFileNodeId) {
    FileTransferDriver *ftd = (FileTransferDriver*)driver;
    UA_Driver *drv = &driver->drv;
    if(!backendComplete(&backend, true, options && options->readOnly))
        return releaseBackend(&backend, UA_STATUSCODE_BADINVALIDARGUMENT);
    UA_StatusCode res = checkDriverAdded(ftd);

    /* The backend file must exist */
    UA_FileTransferFileInfo info;
    if(res == UA_STATUSCODE_GOOD)
        res = backendGetInfo(&backend.file, path, &info);
    if(res == UA_STATUSCODE_GOOD && info.isDirectory)
        res = UA_STATUSCODE_BADINVALIDARGUMENT;
    if(res != UA_STATUSCODE_GOOD)
        return releaseBackend(&backend, res);

    FTMount *mount = newMount(ftd, backend, options, true);
    if(!mount)
        return releaseBackend(&backend, UA_STATUSCODE_BADOUTOFMEMORY);

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
        res = bindObjectMethods(drv->server, node);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_NodeId_copy(&fileNodeId, &mount->rootNodeId);
    if(res != UA_STATUSCODE_GOOD) {
        if(node)
            removeSubtree(drv->server, ftd, node);
        else
            UA_Server_deleteNode(drv->server, fileNodeId, true);
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
refreshLocked(UA_FileTransferDriver *driver, const UA_NodeId directoryNodeId) {
    FileTransferDriver *ftd = (FileTransferDriver*)driver;
    UA_Driver *drv = &driver->drv;
    UA_StatusCode res = checkDriverAdded(ftd);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    FTNode *dirNode = findFTNode(ftd, &directoryNodeId);
    if(!dirNode || !dirNode->isDirectory || dirNode->zombie)
        return UA_STATUSCODE_BADNOTFOUND;

    /* With a maxNodes limit, the vanished entries are removed from the whole
     * subtree first. A single walk can reach a directory with new entries
     * before the one whose removed entries make room for them. */
    UA_UInt32 depth = pathDepth(dirNode->path) + 1;
    UA_UInt32 nodeBudget = (UA_UInt32)0xffffffffu;
    const UA_FileTransferMountOptions *opts = &dirNode->mount->options;
    if(opts->maxNodes > 0) {
        res = fileTransferSyncTree(drv->server, ftd, dirNode, depth, NULL);
        if(res != UA_STATUSCODE_GOOD)
            return res;
        UA_UInt32 current = countMountNodes(ftd, dirNode->mount);
        nodeBudget = (opts->maxNodes > current) ? opts->maxNodes - current : 0;
    }
    return fileTransferSyncTree(drv->server, ftd, dirNode, depth, &nodeBudget);
}

/**************************************
 * Public API
 *
 * The API can be called from another thread than the server's main loop, so
 * it takes the lock of the EventLoop. The lock is recursive: the Method
 * callbacks, which run with the lock held, can call the API as well.
 **************************************/

static UA_EventLoop *
lockDriver(UA_FileTransferDriver *driver) {
    UA_Server *server = driver->drv.server;
    UA_EventLoop *el = (server) ? UA_Server_getConfig(server)->eventLoop : NULL;
    if(el)
        el->lock(el);
    return el;
}

static void
unlockDriver(UA_EventLoop *el) {
    if(el)
        el->unlock(el);
}

UA_StatusCode
UA_FileTransferDriver_addFileSystem(UA_FileTransferDriver *driver,
                                    const UA_NodeId requestedNodeId,
                                    const UA_NodeId parentNodeId,
                                    const UA_QualifiedName browseName,
                                    const UA_FileTransferBackend *backend,
                                    const UA_FileTransferMountOptions *options,
                                    UA_NodeId *outFileSystemNodeId) {
    if(!driver || !backend)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_EventLoop *el = lockDriver(driver);
    UA_StatusCode res =
        addFileSystemLocked(driver, requestedNodeId, parentNodeId, browseName,
                            *backend, options, outFileSystemNodeId);
    unlockDriver(el);
    return res;
}

/* A file mount stores the file backend in a backend without directory
 * operations */
static UA_FileTransferBackend
fileMountBackend(const UA_FileTransferFileBackend *fileBackend) {
    UA_FileTransferBackend backend;
    memset(&backend, 0, sizeof(UA_FileTransferBackend));
    backend.file = *fileBackend;
    return backend;
}

UA_StatusCode
UA_FileTransferDriver_addFile(UA_FileTransferDriver *driver,
                              const UA_NodeId requestedNodeId,
                              const UA_NodeId parentNodeId,
                              const UA_QualifiedName browseName,
                              const UA_FileTransferFileBackend *backend,
                              const UA_String path,
                              const UA_FileTransferMountOptions *options,
                              UA_NodeId *outFileNodeId) {
    if(!driver || !backend)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_EventLoop *el = lockDriver(driver);
    UA_StatusCode res =
        addFileLocked(driver, requestedNodeId, parentNodeId, browseName,
                      fileMountBackend(backend), path, options, outFileNodeId);
    unlockDriver(el);
    return res;
}

UA_StatusCode
UA_FileTransferDriver_remove(UA_FileTransferDriver *driver,
                             const UA_NodeId nodeId) {
    if(!driver)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_EventLoop *el = lockDriver(driver);
    UA_StatusCode res = removeLocked(driver, nodeId);
    unlockDriver(el);
    return res;
}

UA_StatusCode
UA_FileTransferDriver_refresh(UA_FileTransferDriver *driver,
                              const UA_NodeId directoryNodeId) {
    if(!driver)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_EventLoop *el = lockDriver(driver);
    UA_StatusCode res = refreshLocked(driver, directoryNodeId);
    unlockDriver(el);
    return res;
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

    /* Only a driver that actually started owns the Method nodes. A driver
     * whose start failed -- without the full Namespace Zero, for instance --
     * is still STOPPED, and releasing the callbacks here would strip them from
     * the owner of the Method nodes. */
    UA_Boolean wasStarted = (drv->state == UA_LIFECYCLESTATE_STARTED);

    /* Close all open file handles. The mounts and the mirrored nodes are
     * kept so the driver can be restarted. */
    FTHandle *h, *tmp;
    LIST_FOREACH_SAFE(h, &ftd->handles, listEntry, tmp) {
        closeFTHandle(drv->server, ftd, h);
    }

    /* Release the shared Namespace Zero Method nodes again. A stopped driver
     * must not keep answering calls on FileType/FileDirectoryType Objects that
     * belong to the application or to another driver. */
    if(wasStarted && drv->server)
        unregisterFileTransferMethodCallbacks(drv->server);

    drv->state = UA_LIFECYCLESTATE_STOPPED;
}

static UA_StatusCode
FileTransferDriver_free(UA_Driver *drv) {
    if(drv->state != UA_LIFECYCLESTATE_STOPPED)
        return UA_STATUSCODE_BADINTERNALERROR;

    FileTransferDriver *ftd = (FileTransferDriver*)drv;

    /* All handles are closed during stop */
    UA_assert(LIST_EMPTY(&ftd->handles));

    /* Delete the Objects of the mounts before freeing the FTNodes. The value
     * sources of their Properties carry the FTNode as node context; leaving
     * the nodes live after free would leave the callbacks pointing at freed
     * memory (use-after-free on a later Read when the driver is removed from a
     * running server). */
    FTMount *mount, *mountTmp;
    LIST_FOREACH_SAFE(mount, &ftd->mounts, listEntry, mountTmp) {
        FTNode *rootNode = findFTNode(ftd, &mount->rootNodeId);
        if(rootNode && drv->server)
            removeSubtree(drv->server, ftd, rootNode);
        removeMount(ftd, mount);
    }
    UA_assert(LIST_EMPTY(&ftd->nodes));

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

    base->driverType = UA_DRIVERTYPE_FILE_TRANSFER;
    base->name = UA_STRING(UA_DRIVER_FILE_TRANSFER_NAME);
    base->notificationCallback = FileTransferDriver_notification;
    base->notificationFilter = UA_APPLICATIONNOTIFICATIONTYPE_SESSION;
    base->start = FileTransferDriver_start;
    base->stop = FileTransferDriver_stop;
    base->free = FileTransferDriver_free;
    return driver;
}

#endif /* UA_ENABLE_DRIVER_FILE_TRANSFER */
