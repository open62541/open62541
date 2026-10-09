/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include "file_transfer_internal.h"

/**************************************
 * Driver Lookup and Notifications
 **************************************/

static UA_StatusCode FileTransferDriver_start(UA_Driver *drv);

/* Identify our private implementation before casting a generic driver.
 * Names are configurable and do not identify the concrete implementation. */
UA_Boolean
isFileTransferDriver(const UA_Driver *driver) {
    return driver->start == FileTransferDriver_start;
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

    closeSessionHandles(drv->server, ftd, sessionId);
}

/**************************************
 * Backend and Object Helpers
 **************************************/

/* Validate that a backend implements the operations the mount can actually
 * reach. Requiring all of them unconditionally would force a read-only or
 * single-file backend -- content generated on the fly, a file in flash -- to
 * supply half a dozen stubs that only ever return Bad_NotSupported. */
static UA_Boolean
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
    if(!readOnly && (!b->create || !b->remove || !b->rename))
        return false;
    return true;
}

/* A namespaceIndex for the mirrored BrowseNames has to resolve in the server's
 * namespace array. An index that does not would put the Objects in a namespace
 * no client can interpret, and the mismatch would only show up on a browse. */
static UA_StatusCode
checkMountNamespace(UA_Server *server,
                    const FTConfig *options) {
    if(options->namespaceIndex == 0)
        return UA_STATUSCODE_GOOD;
    UA_String uri = UA_STRING_NULL;
    UA_StatusCode res =
        UA_Server_getNamespaceByIndex(server, options->namespaceIndex, &uri);
    UA_String_clear(&uri);
    return (res == UA_STATUSCODE_GOOD) ?
        UA_STATUSCODE_GOOD : UA_STATUSCODE_BADINVALIDARGUMENT;
}

/**************************************
 * Lifecycle and Constructor
 **************************************/

static void FileTransferDriver_stop(UA_Driver *drv);

/* A failing refresh is reported when its result changes, not on every pass */
static void
refreshCallback(UA_Server *server, void *context) {
    FileTransferDriver *ftd = (FileTransferDriver*)context;
    UA_StatusCode res = fileTransferRefresh(&ftd->driver, ftd->root->nodeId);
    if(res != ftd->refreshResult) {
        const UA_Logger *logger = UA_Server_getConfig(server)->logging;
        if(res != UA_STATUSCODE_GOOD)
            UA_LOG_WARNING(logger, UA_LOGCATEGORY_SERVER,
                           "FileTransfer: Refresh of %N failed with %s",
                           ftd->root->nodeId, UA_StatusCode_name(res));
        else
            UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER,
                        "FileTransfer: Refresh of %N succeeds again",
                        ftd->root->nodeId);
    }
    ftd->refreshResult = res;
}

static UA_StatusCode
configureDriver(FileTransferDriver *ftd) {
    const UA_KeyValueMap *params = &ftd->driver.params;
    memset(&ftd->config, 0, sizeof(ftd->config));
    ftd->config.maxHandlesPerSession = UA_FILETRANSFER_MAXHANDLESPERSESSION_DEFAULT;
    const UA_UInt16 *maxPerSession = (const UA_UInt16*)
        UA_KeyValueMap_getScalar(params,
                                 UA_QUALIFIEDNAME(0, "max-open-handles-per-session"),
                                 &UA_TYPES[UA_TYPES_UINT16]);
    if(maxPerSession)
        ftd->config.maxHandlesPerSession = *maxPerSession;

    ftd->config.maxHandlesPerFile = UA_FILETRANSFER_MAXHANDLESPERFILE_DEFAULT;
    const UA_UInt16 *maxPerFile = (const UA_UInt16*)
        UA_KeyValueMap_getScalar(params,
                                 UA_QUALIFIEDNAME(0, "max-open-handles-per-file"),
                                 &UA_TYPES[UA_TYPES_UINT16]);
    if(maxPerFile)
        ftd->config.maxHandlesPerFile = *maxPerFile;

    ftd->config.maxReadLength = UA_FILETRANSFER_MAXREADLENGTH_DEFAULT;
    const UA_UInt32 *maxReadLength = (const UA_UInt32*)
        UA_KeyValueMap_getScalar(params,
                                 UA_QUALIFIEDNAME(0, "max-read-length"),
                                 &UA_TYPES[UA_TYPES_UINT32]);
    if(maxReadLength && *maxReadLength > 0)
        ftd->config.maxReadLength = *maxReadLength;

    const UA_Boolean *readOnly = (const UA_Boolean*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "read-only"), &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(readOnly)
        ftd->config.readOnly = *readOnly;
    const UA_UInt32 *maxDepth = (const UA_UInt32*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "max-scan-depth"), &UA_TYPES[UA_TYPES_UINT32]);
    if(maxDepth)
        ftd->config.maxScanDepth = *maxDepth;
    const UA_UInt32 *maxNodes = (const UA_UInt32*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "max-nodes"), &UA_TYPES[UA_TYPES_UINT32]);
    if(maxNodes)
        ftd->config.maxNodes = *maxNodes;
    const UA_UInt16 *ns = (const UA_UInt16*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "namespace-index"), &UA_TYPES[UA_TYPES_UINT16]);
    if(ns)
        ftd->config.namespaceIndex = *ns;
    ftd->config.refreshInterval = 1000;
    const UA_Double *interval = (const UA_Double*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "refresh-interval"), &UA_TYPES[UA_TYPES_DOUBLE]);
    if(interval)
        ftd->config.refreshInterval = *interval;
    /* Zero disables the periodic refresh. Otherwise the timer needs at least
     * one DateTime tick and an Int64 tick count (rejects NaN and infinity). */
    UA_Double ri = ftd->config.refreshInterval;
    if(ri != 0.0 && !(ri * UA_DATETIME_MSEC >= 1.0 &&
                      ri <= (UA_Double)UA_INT64_MAX / UA_DATETIME_MSEC))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(!backendComplete(&ftd->backend, !ftd->root->isDirectory, ftd->config.readOnly))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    return checkMountNamespace(ftd->driver.server, &ftd->config);
}

static void *
prepareEntry(void *context, FTEntry *node) {
    UA_Server *server = node->driver->driver.server;
    UA_StatusCode res = bindObjectContext(server, node);
    if(res == UA_STATUSCODE_GOOD && !node->isDirectory && !node->binding) {
        UA_FileTransferFileInfo info;
        res = backendGetInfo(&node->driver->backend.file, node->path, &info);
        if(res == UA_STATUSCODE_GOOD)
            res = setupFileNode(server, node, &info);
    }
    *(UA_StatusCode*)context = res;
    return res == UA_STATUSCODE_GOOD ? NULL : context;
}

static void *
bindEntryMethods(void *context, FTEntry *node) {
    UA_StatusCode res = bindObjectMethods(node->driver->driver.server, node);
    *(UA_StatusCode*)context = res;
    return res == UA_STATUSCODE_GOOD ? NULL : context;
}

static void *
unbindEntryMethods(void *context, FTEntry *node) {
    unbindObjectMethods(node->driver->driver.server, node);
    return NULL;
}

static UA_StatusCode
FileTransferDriver_start(UA_Driver *drv) {
    if(drv->state != UA_LIFECYCLESTATE_STOPPED)
        return UA_STATUSCODE_BADINVALIDSTATE;
    FileTransferDriver *ftd = (FileTransferDriver*)drv;
    /* Constructors can run before registration. Check again now, so two
     * unregistered drivers cannot later claim the same existing Object. */
    for(UA_Driver *other = UA_Server_getDrivers(drv->server); other; other = other->next) {
        if(other != drv && isFileTransferDriver(other) &&
           findFTEntry((FileTransferDriver*)other, &ftd->root->nodeId))
            return UA_STATUSCODE_BADNODEIDEXISTS;
    }
    UA_StatusCode res = configureDriver(ftd);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Objects survive stop/start. Synchronize the tree using the parameters
     * configured after construction, before accepting Method calls. */
    if(ftd->root->isDirectory) {
        ftd->scanReported = false;
        res = fileTransferRefresh(&ftd->driver, ftd->root->nodeId);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }
    ftd->refreshResult = UA_STATUSCODE_GOOD;
    ZIP_ITER(FTEntriesById, &ftd->entriesByNodeId, prepareEntry, &res);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    res = registerFileTransferMethodCallbacks(drv->server);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    drv->state = UA_LIFECYCLESTATE_STARTED;
    ZIP_ITER(FTEntriesById, &ftd->entriesByNodeId, bindEntryMethods, &res);
    if(res == UA_STATUSCODE_GOOD && ftd->root->isDirectory &&
       ftd->config.refreshInterval > 0)
        res = UA_Server_addRepeatedCallback(drv->server, refreshCallback, ftd,
                                            ftd->config.refreshInterval, &ftd->refreshCallbackId);
    if(res != UA_STATUSCODE_GOOD)
        FileTransferDriver_stop(drv);
    return res;
}

static void
FileTransferDriver_stop(UA_Driver *drv) {
    FileTransferDriver *ftd = (FileTransferDriver*)drv;

    /* Only a driver that actually started owns the Method nodes. A driver
     * whose start failed -- without the full Namespace Zero, for instance --
     * is still STOPPED, and releasing the callbacks here would strip them from
     * the owner of the Method nodes. */
    UA_Boolean wasStarted = (drv->state == UA_LIFECYCLESTATE_STARTED);

    if(ftd->refreshCallbackId) {
        UA_Server_removeCallback(drv->server, ftd->refreshCallbackId);
        ftd->refreshCallbackId = 0;
    }
    if(wasStarted)
        ZIP_ITER(FTEntriesById, &ftd->entriesByNodeId, unbindEntryMethods, NULL);

    /* Close this instance's handles; retain its root and nodes for restart. */
    closeDriverHandles(drv->server, ftd);

    /* Release the shared Namespace Zero Method nodes again. A stopped driver
     * must not keep answering calls on FileType/FileDirectoryType Objects that
     * belong to the application or to another driver. */
    if(wasStarted) {
        UA_Boolean otherStarted = false;
        for(UA_Driver *other = UA_Server_getDrivers(drv->server); other; other = other->next) {
            if(other != drv && isFileTransferDriver(other) &&
               other->state == UA_LIFECYCLESTATE_STARTED) {
                otherStarted = true;
                break;
            }
        }
        if(!otherStarted)
            unregisterFileTransferMethodCallbacks(drv->server);
    }

    drv->state = UA_LIFECYCLESTATE_STOPPED;
}

static UA_StatusCode
FileTransferDriver_free(UA_Driver *drv) {
    if(drv->state != UA_LIFECYCLESTATE_STOPPED)
        return UA_STATUSCODE_BADINTERNALERROR;

    FileTransferDriver *ftd = (FileTransferDriver*)drv;

    UA_EventLoop *el = UA_Server_getConfig(drv->server)->eventLoop;
    el->lock(el);

    /* All handles are closed during stop */
    UA_assert(!ZIP_ROOT(&ftd->handlesById));
    UA_assert(!ZIP_ROOT(&ftd->handlesBySession));

    /* Delete the root and its Objects before freeing the FTEntry records. The value
     * sources of their Properties carry the FTEntry as node context; leaving
     * the nodes live after free would leave the callbacks pointing at freed
     * memory (use-after-free on a later Read when the driver is removed from a
     * running server). */
    FTEntry *rootNode = ftd->root;
    if(rootNode)
        removeSubtree(drv->server, rootNode);
    if(ftd->backend.file.clear)
        ftd->backend.file.clear(&ftd->backend.file);
    UA_assert(!ZIP_ROOT(&ftd->entriesByNodeId));
    UA_assert(ftd->entryCount == 0);

    UA_KeyValueMap_clear(&drv->params);
    el->unlock(el);
    UA_free(ftd);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
newDriver(UA_Server *server, const UA_FileTransferBackend *backend,
          UA_Boolean standaloneFile, const UA_String path,
          const UA_FileTransferNodeDescription *description, UA_NodeId *outNodeId,
          UA_Driver **outDriver) {
    if(outDriver)
        *outDriver = NULL;
    if(!server || !backend || !outDriver)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    /* Mutating callbacks may be omitted when read-only is configured before
     * start. The callbacks needed for initialization must already be present. */
    if(!backendComplete(backend, standaloneFile, true))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_EventLoop *el = UA_Server_getConfig(server)->eventLoop;
    el->lock(el);
    UA_FileTransferNodeDescription desc;
    memset(&desc, 0, sizeof(desc));
    if(description)
        desc = *description;
    UA_NodeClass cls;
    UA_StatusCode res = UA_STATUSCODE_BADNODEIDUNKNOWN;
    UA_Boolean created = true;
    if(!UA_NodeId_isNull(&desc.nodeId)) {
        res = UA_Server_readNodeClass(server, desc.nodeId, &cls);
        if(res == UA_STATUSCODE_GOOD) {
            created = false;
            res = checkFileTransferObject(server, desc.nodeId, !standaloneFile);
            if(res == UA_STATUSCODE_GOOD && findEntryOwner(server, &desc.nodeId))
                res = UA_STATUSCODE_BADNODEIDEXISTS;
        }
        if(res != UA_STATUSCODE_GOOD && !(created && res == UA_STATUSCODE_BADNODEIDUNKNOWN)) {
            el->unlock(el);
            return res;
        }
    }
    if(created) {
        if(UA_NodeId_isNull(&desc.nodeId))
            desc.nodeId = UA_NODEID_NUMERIC(1, 0);
        if(UA_NodeId_isNull(&desc.parentNodeId))
            desc.parentNodeId = UA_NS0ID(OBJECTSFOLDER);
        if(UA_NodeId_isNull(&desc.referenceTypeId))
            desc.referenceTypeId = UA_NS0ID(HASCOMPONENT);
        if(UA_NodeId_isNull(&desc.typeDefinition))
            desc.typeDefinition = standaloneFile ? UA_NS0ID(FILETYPE) : UA_NS0ID(FILEDIRECTORYTYPE);
        res = checkFileTransferType(server, desc.typeDefinition, !standaloneFile);
        if(res != UA_STATUSCODE_GOOD) {
            el->unlock(el);
            return res;
        }
    }
    UA_FileTransferFileInfo info;
    UA_FileTransferFileBackend fileBackend = backend->file;
    res = backendGetInfo(&fileBackend, path, &info);
    if(res == UA_STATUSCODE_GOOD && info.isDirectory == standaloneFile)
        res = UA_STATUSCODE_BADINVALIDARGUMENT;
    if(res != UA_STATUSCODE_GOOD) {
        el->unlock(el);
        return res;
    }
    FileTransferDriver *ftd = (FileTransferDriver*)UA_calloc(1, sizeof(FileTransferDriver));
    if(!ftd) {
        el->unlock(el);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    ftd->backend = *backend;
    ftd->config.maxReadLength = UA_FILETRANSFER_MAXREADLENGTH_DEFAULT;
    ZIP_INIT(&ftd->entriesByNodeId);
    ZIP_INIT(&ftd->handlesById);
    ZIP_INIT(&ftd->handlesBySession);
    UA_Driver *base = &ftd->driver;
    base->server = server;
    base->driverType = UA_DRIVERTYPE_GENERIC;
    base->name = UA_STRING(UA_DRIVER_FILE_TRANSFER_NAME);
    base->notificationCallback = FileTransferDriver_notification;
    base->notificationFilter = UA_APPLICATIONNOTIFICATIONTYPE_SESSION;
    base->start = FileTransferDriver_start;
    base->stop = FileTransferDriver_stop;
    base->free = FileTransferDriver_free;

    UA_NodeId rootId = UA_NODEID_NULL;
    if(created) {
        if(desc.browseName.name.length == 0) {
            if(standaloneFile) {
                UA_String name = pathLastSegment(path);
                if(name.length == 0) {
                    const char *end = (const char*)memchr(info.name, 0, sizeof(info.name));
                    if(end)
                        name = (UA_String){(size_t)(end - info.name), (UA_Byte*)info.name};
                }
                if(name.length == 0)
                    name = UA_STRING("File");
                desc.browseName = (UA_QualifiedName){1, name};
            } else {
                desc.browseName = UA_QUALIFIEDNAME(0, "FileSystem");
            }
        }
        if(desc.attributes.displayName.text.length == 0)
            desc.attributes.displayName.text = desc.browseName.name;
        res = UA_Server_addObjectNode(server, desc.nodeId, desc.parentNodeId,
                                      desc.referenceTypeId, desc.browseName, desc.typeDefinition,
                                      desc.attributes, &ftd->driver, &rootId);
    } else {
        res = UA_NodeId_copy(&desc.nodeId, &rootId);
    }
    FTEntry *root = NULL;
    if(res == UA_STATUSCODE_GOOD) {
        root = newFTEntry(ftd, NULL, rootId, path, !standaloneFile);
        if(!root) {
            res = UA_STATUSCODE_BADOUTOFMEMORY;
        } else {
            root->created = created;
            if(created)
                res = bindObjectContext(server, root);
            if(res == UA_STATUSCODE_GOOD && standaloneFile && created)
                res = setupFileNode(server, root, &info);
        }
        if(res == UA_STATUSCODE_GOOD)
            ftd->root = root;
        if(res != UA_STATUSCODE_GOOD) {
            if(root)
                removeSubtree(server, root);
            else if(created)
                UA_Server_deleteNode(server, rootId, true);
        }
    }
    if(res == UA_STATUSCODE_GOOD && outNodeId)
        *outNodeId = rootId;
    else
        UA_NodeId_clear(&rootId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(ftd); /* Backend ownership stays with the caller. */
    } else {
        *outDriver = &ftd->driver;
    }
    el->unlock(el);
    return res;
}

UA_StatusCode
UA_FileTransferDriver_newFile(UA_Server *server,
                              const UA_FileTransferFileBackend *backend,
                              const UA_String path,
                              const UA_FileTransferNodeDescription *description,
                              UA_NodeId *outNodeId, UA_Driver **outDriver) {
    UA_FileTransferBackend fullBackend;
    memset(&fullBackend, 0, sizeof(fullBackend));
    if(backend)
        fullBackend.file = *backend;
    return newDriver(server, backend ? &fullBackend : NULL, true, path,
                     description, outNodeId, outDriver);
}

UA_StatusCode
UA_FileTransferDriver_refresh(UA_Driver *driver) {
    if(!driver || !driver->server || !isFileTransferDriver(driver))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_EventLoop *el = UA_Server_getConfig(driver->server)->eventLoop;
    el->lock(el);
    FileTransferDriver *ftd = (FileTransferDriver*)driver;
    UA_StatusCode res = UA_STATUSCODE_BADINVALIDSTATE;
    if(driver->state == UA_LIFECYCLESTATE_STARTED)
        res = ftd->root->isDirectory ?
            fileTransferRefresh(driver, ftd->root->nodeId) :
            UA_STATUSCODE_BADNOTSUPPORTED;
    el->unlock(el);
    return res;
}

UA_StatusCode
UA_FileTransferDriver_newDirectory(UA_Server *server,
                                   const UA_FileTransferBackend *backend,
                                   const UA_FileTransferNodeDescription *description,
                                   UA_NodeId *outNodeId, UA_Driver **outDriver) {
    return newDriver(server, backend, false, UA_STRING_NULL,
                     description, outNodeId, outDriver);
}
