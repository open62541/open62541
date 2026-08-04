/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include "file_transfer_internal.h"

#if defined(UA_ENABLE_METHODCALLS) && defined(UA_GENERATED_NAMESPACE_ZERO_FULL)

/**************************************
 * Driver Lookup and Notifications
 **************************************/

static UA_StatusCode FileTransferDriver_start(UA_Driver *drv);

static UA_Boolean
isFileTransferDriver(const UA_Driver *drv) {
    return drv && drv->start == FileTransferDriver_start;
}

FileTransferDriver *
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

/* Close the backend file context and release the handle. Removes zombie
 * nodes once their last handle is closed. */
UA_StatusCode
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

UA_StatusCode
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
        res = UA_Server_setMethodNodeCallback(
            server, UA_NODEID_NUMERIC(0, methods[i].methodId), methods[i].callback);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }
    return res;
}

/**************************************
 * Public Driver API
 **************************************/

static const UA_FileTransferMountOptions defaultMountOptions =
    {false, 0, 0, NULL, NULL};

/* Validate that a backend implements the mandatory operations */
UA_Boolean
backendComplete(const UA_FileTransferBackend *b) {
    return b->openFile && b->closeFile && b->read && b->write &&
        b->getPosition && b->setPosition && b->getAttributes &&
        b->listDirectory && b->createFile && b->createDirectory &&
        b->remove && b->rename;
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
    if(mount->backend.clear)
        mount->backend.clear(&mount->backend);
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

/* Remove all registry entries of a mount */
void
removeMountNodes(FileTransferDriver *ftd, FTMount *mount) {
    FTNode *node, *tmp;
    LIST_FOREACH_SAFE(node, &ftd->nodes, listEntry, tmp) {
        if(node->mount == mount)
            removeFTNode(ftd, node);
    }
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
        res = fileTransferMirrorTree(drv->server, ftd, rootNode, 1, &nodeBudget);
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
    return fileTransferSyncTree(drv->server, ftd, dirNode, pathDepth(dirNode->path) + 1,
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

    /* Delete the mirrored Objects from the address space before freeing the
     * FTNodes. Their Size/UserWritable/LastModifiedTime value sources carry the
     * FTNode as node context; leaving the nodes live after free would leave the
     * callbacks pointing at freed memory (use-after-free on a later Read when
     * the driver is removed from a running server). Deleting a directory root
     * removes its children recursively; the later per-child delete then no-ops. */
    FTNode *node, *nodeTmp;
    LIST_FOREACH_SAFE(node, &ftd->nodes, listEntry, nodeTmp) {
        UA_Server_deleteNode(drv->server, node->nodeId, true);
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