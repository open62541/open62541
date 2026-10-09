/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include "file_transfer_internal.h"

/**************************************
 * Directory Tree Mirroring
 **************************************/

/* A complete, sorted snapshot. Inline FileInfo names remain valid when the
 * array grows; no pointers into an earlier allocation are retained. */
typedef struct {
    UA_FileTransferFileInfo *entries;
    size_t size;
    size_t capacity;
    UA_StatusCode res;
} ScanList;

static void
scanCollector(void *listContext, const UA_FileTransferFileInfo *info) {
    ScanList *list = (ScanList*)listContext;
    if(list->res != UA_STATUSCODE_GOOD)
        return;
    if(!memchr(info->name, 0, sizeof(info->name))) {
        list->res = UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
        return;
    }
    if(list->size == list->capacity) {
        size_t capacity = list->capacity ? list->capacity * 2 : 16;
        if(capacity < list->capacity || capacity > SIZE_MAX / sizeof(*list->entries)) {
            list->res = UA_STATUSCODE_BADOUTOFMEMORY;
            return;
        }
        void *entries = UA_realloc(list->entries, capacity * sizeof(*list->entries));
        if(!entries) {
            list->res = UA_STATUSCODE_BADOUTOFMEMORY;
            return;
        }
        list->entries = (UA_FileTransferFileInfo*)entries;
        list->capacity = capacity;
    }
    list->entries[list->size++] = *info;
}

static void
clearScanList(ScanList *list) {
    UA_free(list->entries);
    memset(list, 0, sizeof(*list));
}

static int
scanNameOrder(const void *a, const void *b) {
    return strcmp(((const UA_FileTransferFileInfo*)a)->name,
                  ((const UA_FileTransferFileInfo*)b)->name);
}

/* A partial or ambiguous listing must not delete existing model entries. */
static UA_StatusCode
listEntries(UA_FileTransferBackend *b, const UA_String path, ScanList *list) {
    memset(list, 0, sizeof(*list));
    UA_StatusCode res = b->listDirectory(b, path, scanCollector, list);
    if(res == UA_STATUSCODE_GOOD)
        res = list->res;
    if(res == UA_STATUSCODE_GOOD && list->size > 1) {
        qsort(list->entries, list->size, sizeof(*list->entries), scanNameOrder);
        for(size_t i = 1; i < list->size; i++) {
            if(strcmp(list->entries[i - 1].name, list->entries[i].name) == 0) {
                res = UA_STATUSCODE_BADBROWSENAMEDUPLICATED;
                break;
            }
        }
    }
    if(res != UA_STATUSCODE_GOOD)
        clearScanList(list);
    return res;
}

/* Create an empty entry with its basename and type; metadata uses backend
 * defaults. The target path may have changed during a copy or move. */
static UA_StatusCode
createBackendEntry(UA_FileTransferBackend *b, const UA_String path,
                    UA_Boolean isDirectory) {
    UA_String name = pathLastSegment(path);
    if(!validEntryName(name))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_FileTransferFileInfo info;
    memset(&info, 0, sizeof(info));
    memcpy(info.name, name.data, name.length);
    info.isDirectory = isDirectory;
    return b->create(b, path, &info);
}

static UA_UInt32
entryDepth(const FTEntry *node) {
    UA_UInt32 depth = 0;
    for(const FTEntry *parent = node->parent; parent; parent = parent->parent)
        depth++;
    return depth;
}

static void *
liveChild(void *context, FTEntry *node) {
    return node->zombie ? NULL : node;
}

static FTEntry *
findEntryByPath(FileTransferDriver *ftd, const UA_String path) {
    FTEntry *node = ftd->root;
    if(UA_String_equal(&path, &node->path))
        return node->zombie ? NULL : node;
    if(!node->isDirectory || !pathWithinSubtree(path, node->path))
        return NULL;
    size_t offset = node->path.length ? node->path.length + 1 : 0;
    while(offset < path.length) {
        size_t end = offset;
        while(end < path.length && path.data[end] != '/')
            end++;
        UA_String name = {end - offset, path.data + offset};
        node = (FTEntry*)ZIP_ITER_KEY(FTChildrenByName, &node->children,
                                      &name, liveChild, NULL);
        if(!node)
            return NULL;
        offset = end + 1;
    }
    return node;
}

/* A backend error for a single entry -- an unreadable subdirectory, a file that
 * vanished mid-scan, a permission-denied stat -- must not abort the whole scan.
 * Such an entry is skipped with a warning, exactly like an entry with an
 * invalid name, so that one restricted directory does not make the entire
 * mount unavailable. Only a failed allocation is fatal: the resulting tree
 * would be arbitrarily incomplete for a reason the caller cannot act on. */
static UA_Boolean
fatalScanError(UA_StatusCode res) {
    return res == UA_STATUSCODE_BADOUTOFMEMORY;
}

/* A subtree with open handles cannot be removed right away when its backend
 * entries vanished. The nodes stay in the address space so that the open
 * handles remain usable (Close in particular). Zombie file nodes are removed
 * when their last handle is closed; the remaining zombie nodes are collected
 * on the next refresh. */
static void *
markSubtreeZombie(void *context, FTEntry *node) {
    node->zombie = true;
    ZIP_ITER(FTChildrenByName, &node->children, markSubtreeZombie, NULL);
    return NULL;
}

/* Context for per-entry authorization in recursive storage operations. NULL
 * is used only when rolling back a destination created by this operation. */
typedef struct {
    UA_Server *server;
    FileTransferDriver *driver;
    const UA_NodeId *sessionId;
    void *sessionContext;
    UA_Boolean copying;
    UA_Boolean deleting;
} FTTreeAccess;

/* Copy reads each file. Deletion changes its containing directory, not the
 * file. Directories need read/traverse and, for deletion, write. An unmirrored
 * entry cannot be authorized by a per-Object callback, so deny that case. */
static UA_StatusCode
checkTreeEntryAccess(const FTTreeAccess *access, const UA_String path,
                     UA_Boolean isDirectory) {
    if(!access)
        return UA_STATUSCODE_GOOD;
    if(!isDirectory && !access->copying)
        return UA_STATUSCODE_GOOD;
    UA_FileAccessRights required = UA_FILEACCESS_READ;
    if(isDirectory) {
        required |= UA_FILEACCESS_TRAVERSE;
        if(access->deleting)
            required |= UA_FILEACCESS_WRITE;
    }
    FTEntry *node = findEntryByPath(access->driver, path);
    UA_StatusCode res;
    if(node) {
        res = checkFTAccess(access->server, node, access->sessionId,
                            access->sessionContext, required, UA_STATUSCODE_BADUSERACCESSDENIED);
    } else {
        if(access->driver->backend.file.getAccessRights ||
           access->driver->backend.file.getUserAccessRights)
            return UA_STATUSCODE_BADUSERACCESSDENIED;
        UA_FileTransferFileInfo info;
        res = backendGetInfo(&access->driver->backend.file, path, &info);
        if(res == UA_STATUSCODE_GOOD && (info.accessRights & required) != required)
            res = UA_STATUSCODE_BADUSERACCESSDENIED;
    }
    return res;
}

/* Preflight avoids partial changes for a denied descendant. The actual
 * traversal checks again, because a backend listing can change in between. */
static UA_StatusCode
checkTreeAccess(const FTTreeAccess *access, const UA_String path,
                UA_Boolean isDirectory) {
    UA_StatusCode res = checkTreeEntryAccess(access, path, isDirectory);
    if(res != UA_STATUSCODE_GOOD || !isDirectory)
        return res;
    ScanList entries;
    res = listEntries(&access->driver->backend, path, &entries);
    for(size_t i = 0; i < entries.size && res == UA_STATUSCODE_GOOD; i++) {
        UA_FileTransferFileInfo *info = &entries.entries[i];
        UA_String name = UA_STRING(info->name);
        UA_String childPath = UA_STRING_NULL;
        res = joinPath(path, name, &childPath);
        if(res == UA_STATUSCODE_GOOD)
            res = checkTreeAccess(access, childPath, info->isDirectory);
        UA_String_clear(&childPath);
    }
    clearScanList(&entries);
    return res;
}

/* Recursively delete a backend subtree (bottom-up) */
static UA_StatusCode
deleteBackendTree(UA_FileTransferBackend *b, const UA_String path,
                  UA_Boolean isDir, const FTTreeAccess *access) {
    UA_StatusCode res = checkTreeEntryAccess(access, path, isDir);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(isDir) {
        ScanList entries;
        res = listEntries(b, path, &entries);
        if(res != UA_STATUSCODE_GOOD)
            return res;
        for(size_t i = 0; i < entries.size && res == UA_STATUSCODE_GOOD; i++) {
            UA_FileTransferFileInfo *info = &entries.entries[i];
            UA_String name = UA_STRING(info->name);
            UA_String childPath = UA_STRING_NULL;
            res = joinPath(path, name, &childPath);
            if(res == UA_STATUSCODE_GOOD)
                res = deleteBackendTree(b, childPath, info->isDirectory, access);
            UA_String_clear(&childPath);
        }
        clearScanList(&entries);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }
    return b->remove(b, path);
}

/* Copy a single file. Source and target may live in different backends. Uses
 * the backend fast-path only within one backend, otherwise streams the content
 * through read/write loops. */
static UA_StatusCode
copyBackendFile(UA_FileTransferBackend *srcB, const UA_String fromPath,
                UA_FileTransferBackend *dstB, const UA_String toPath) {
    if(srcB == dstB && dstB->copy)
        return dstB->copy(dstB, fromPath, toPath);

    UA_FileTransferFileBackend *srcF = &srcB->file;
    UA_FileTransferFileBackend *dstF = &dstB->file;
    UA_UInt32 src = 0;
    UA_StatusCode res = srcF->open(srcF, fromPath, UA_OPENFILEMODE_READ, &src);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = createBackendEntry(dstB, toPath, false);
    if(res != UA_STATUSCODE_GOOD) {
        srcF->close(srcF, src);
        return res;
    }
    UA_UInt32 dst = 0;
    res = dstF->open(dstF, toPath, UA_OPENFILEMODE_WRITE, &dst);
    if(res != UA_STATUSCODE_GOOD) {
        /* Remove the empty file just created so a failed open does not leave
         * a stale entry behind in the destination backend. */
        dstB->remove(dstB, toPath);
        srcF->close(srcF, src);
        return res;
    }

    while(res == UA_STATUSCODE_GOOD) {
        UA_ByteString chunk = UA_BYTESTRING_NULL;
        res = srcF->read(srcF, src, UA_FILETRANSFER_COPYCHUNKSIZE, &chunk);
        if(res != UA_STATUSCODE_GOOD || chunk.length == 0) {
            UA_ByteString_clear(&chunk);
            break;
        }
        res = dstF->write(dstF, dst, chunk);
        UA_ByteString_clear(&chunk);
    }

    /* Closing flushes the content to the storage, so the copy is complete only
     * when both closes succeed. The first error is kept. A partially written
     * destination is removed, so a failed copy does not leave a truncated file
     * behind. An EOF read leaves res Good. */
    UA_StatusCode closeRes = srcF->close(srcF, src);
    UA_StatusCode dstCloseRes = dstF->close(dstF, dst);
    if(closeRes == UA_STATUSCODE_GOOD)
        closeRes = dstCloseRes;
    if(res == UA_STATUSCODE_GOOD)
        res = closeRes;
    if(res != UA_STATUSCODE_GOOD)
        dstB->remove(dstB, toPath);
    return res;
}

static UA_StatusCode
copyBackendTree(UA_FileTransferBackend *srcB, const UA_String fromPath,
                UA_FileTransferBackend *dstB, const UA_String toPath,
                UA_Boolean isDir, const FTTreeAccess *access) {
    UA_StatusCode res = checkTreeEntryAccess(access, fromPath, isDir);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(!isDir)
        return copyBackendFile(srcB, fromPath, dstB, toPath);

    res = createBackendEntry(dstB, toPath, true);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    ScanList entries;
    res = listEntries(srcB, fromPath, &entries);
    for(size_t i = 0; i < entries.size && res == UA_STATUSCODE_GOOD; i++) {
        UA_FileTransferFileInfo *info = &entries.entries[i];
        UA_String name = UA_STRING(info->name);
        UA_String childFrom = UA_STRING_NULL;
        UA_String childTo = UA_STRING_NULL;
        res = joinPath(fromPath, name, &childFrom);
        if(res == UA_STATUSCODE_GOOD)
            res = joinPath(toPath, name, &childTo);
        if(res == UA_STATUSCODE_GOOD)
            res = copyBackendTree(srcB, childFrom, dstB, childTo,
                                   info->isDirectory, access);
        UA_String_clear(&childFrom);
        UA_String_clear(&childTo);
    }
    clearScanList(&entries);

    /* The directory was created by the copy. Remove a partial copy so that a
     * failed copy does not leave an entry behind that is not mirrored. */
    if(res != UA_STATUSCODE_GOOD)
        deleteBackendTree(dstB, toPath, true, NULL);
    return res;
}

typedef enum {
    FT_RECONCILE_FULL,
    FT_RECONCILE_REMOVE_ONLY
} FTReconcileMode;

typedef struct {
    FTReconcileMode mode;
    UA_UInt32 remaining; /* Shared node budget for this tree walk */
    UA_Boolean quiet;    /* Log skipped entries at debug level only */
    FTScanSummary summary;
} FTReconcile;

/* Repeated refreshes meet the same skipped entries on every pass. Quiet passes
 * log them at debug level only. */
#define FT_LOG(server, quiet, LOGFN, ...) do {                              \
        const UA_Logger *logger_ = UA_Server_getConfig(server)->logging;    \
        if(quiet)                                                           \
            UA_LOG_DEBUG(logger_, UA_LOGCATEGORY_SERVER, __VA_ARGS__);      \
        else                                                                \
            LOGFN(logger_, UA_LOGCATEGORY_SERVER, __VA_ARGS__);             \
    } while(0)
#define FT_LOG_SKIP(server, scan, ...) do {                                 \
        (scan)->summary.skipped++;                                          \
        FT_LOG(server, (scan)->quiet, UA_LOG_WARNING, __VA_ARGS__);         \
    } while(0)

static void *
resetScanMark(void *context, FTEntry *node) {
    node->scanSeen = false;
    return NULL;
}

static void *
matchingLiveChild(void *context, FTEntry *node) {
    const UA_FileTransferFileInfo *info = (const UA_FileTransferFileInfo*)context;
    return !node->zombie && node->isDirectory == info->isDirectory ? node : NULL;
}

static void *
matchingRetainedChild(void *context, FTEntry *node) {
    const UA_FileTransferFileInfo *info = (const UA_FileTransferFileInfo*)context;
    return node->zombie && node->isDirectory == info->isDirectory ? node : NULL;
}

static FTEntry *
findScanChild(FTEntry *directory, const UA_FileTransferFileInfo *info) {
    UA_String name = UA_STRING((char*)(uintptr_t)info->name);
    FTEntry *child = (FTEntry*)ZIP_ITER_KEY(FTChildrenByName, &directory->children,
        &name, matchingLiveChild, (void*)(uintptr_t)info);
    if(!child)
        child = (FTEntry*)ZIP_ITER_KEY(FTChildrenByName, &directory->children,
            &name, matchingRetainedChild, (void*)(uintptr_t)info);
    return child;
}

/* Visit the name-index branches before removing their root. Removing entries
 * below a zip-tree node changes only that branch, so the other branch and the
 * current node remain valid. ZIP_ITER itself must not rebalance its tree. */
static void
retireUnseenChildren(UA_Server *server, FTEntry *node) {
    if(!node)
        return;
    retireUnseenChildren(server, ZIP_LEFT(node, nameTreeEntry));
    retireUnseenChildren(server, ZIP_RIGHT(node, nameTreeEntry));
    if(node->scanSeen)
        return;
    if(node->subtreeHandleCount > 0)
        markSubtreeZombie(NULL, node);
    else
        removeSubtree(server, node);
}

/* Resolve names through the child index. Each complete listing is collected
 * once per pass; matching no longer compares every child to every scan entry. */
static UA_StatusCode
reconcileTree(UA_Server *server, FTEntry *dirNode, UA_UInt32 depth,
               FTReconcile *scan) {
    FileTransferDriver *ftd = dirNode->driver;
    if(ftd->config.maxScanDepth > 0 && depth > ftd->config.maxScanDepth) {
        if(scan->mode == FT_RECONCILE_FULL)
            scan->summary.depthCut++;
        return UA_STATUSCODE_GOOD;
    }
    ScanList entries;
    UA_StatusCode res = listEntries(&ftd->backend, dirNode->path, &entries);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    ZIP_ITER(FTChildrenByName, &dirNode->children, resetScanMark, NULL);
    for(size_t i = 0; i < entries.size; i++) {
        FTEntry *child = findScanChild(dirNode, &entries.entries[i]);
        if(child) {
            child->scanSeen = true;
            child->zombie = false;
        }
    }
    retireUnseenChildren(server, ZIP_ROOT(&dirNode->children));

    for(size_t i = 0; i < entries.size; i++) {
        UA_FileTransferFileInfo *info = &entries.entries[i];
        UA_String name = UA_STRING(info->name);
        FTEntry *child = findScanChild(dirNode, info);
        if(!child) {
            if(scan->mode == FT_RECONCILE_REMOVE_ONLY)
                continue;
            if(!validEntryName(name)) {
                FT_LOG_SKIP(server, scan,
                            "FileTransfer: Skipping the entry \"%S\" with an invalid name",
                            name);
                continue;
            }
            if(scan->remaining == 0) {
                scan->summary.nodesCut++;
                continue;
            }
            res = mirrorObject(server, dirNode, name,
                               info->isDirectory ? NULL : info, &child);
            if(res != UA_STATUSCODE_GOOD) {
                if(fatalScanError(res))
                    break;
                FT_LOG_SKIP(server, scan, "FileTransfer: Skipping the entry \"%S\": %s",
                            name, UA_StatusCode_name(res));
                res = UA_STATUSCODE_GOOD;
                continue;
            }
            scan->remaining--;
        }
        if(!child->isDirectory)
            continue;
        res = reconcileTree(server, child, depth + 1, scan);
        if(res == UA_STATUSCODE_GOOD)
            continue;
        if(fatalScanError(res))
            break;
        FT_LOG_SKIP(server, scan, "FileTransfer: Cannot list the directory \"%S\": %s",
                    child->path, UA_StatusCode_name(res));
        res = UA_STATUSCODE_GOOD;
    }
    clearScanList(&entries);
    return res;
}

/* Info for the depth limit, which can be intended. Warnings for missing entries. */
static void
logScanSummary(UA_Server *server, const FTEntry *directory,
               const FTScanSummary *summary, UA_Boolean quiet) {
    const FTConfig *config = &directory->driver->config;
    if(summary->skipped > 0)
        FT_LOG(server, quiet, UA_LOG_WARNING,
               "FileTransfer: %N skipped %u entries that cannot be mirrored or listed",
               directory->nodeId, (unsigned)summary->skipped);
    if(summary->depthCut > 0)
        FT_LOG(server, quiet, UA_LOG_INFO,
               "FileTransfer: %N is mirrored up to max-scan-depth %u "
               "(directories not listed: %u)", directory->nodeId,
               (unsigned)config->maxScanDepth, (unsigned)summary->depthCut);
    if(summary->nodesCut > 0)
        FT_LOG(server, quiet, UA_LOG_WARNING,
               "FileTransfer: %N reached max-nodes %u (listed entries not served: %u)",
               directory->nodeId, (unsigned)config->maxNodes,
               (unsigned)summary->nodesCut);
}

/* Without outSummary, the summary is logged right away */
static UA_StatusCode
reconcileDirectory(UA_Server *server, FTEntry *directory, FTReconcileMode mode,
                   UA_Boolean quiet, FTScanSummary *outSummary) {
    FileTransferDriver *ftd = directory->driver;
    FTReconcile scan;
    memset(&scan, 0, sizeof(scan));
    scan.mode = mode;
    scan.remaining = (UA_UInt32)0xffffffffu;
    scan.quiet = quiet;
    UA_UInt32 depth = entryDepth(directory) + 1;
    /* Free the whole subtree's budget before adding anything. Traversal order
     * must not prevent one directory from using space freed in another. */
    if(mode == FT_RECONCILE_FULL && ftd->config.maxNodes > 0) {
        scan.mode = FT_RECONCILE_REMOVE_ONLY;
        UA_StatusCode res = reconcileTree(server, directory, depth, &scan);
        if(res != UA_STATUSCODE_GOOD)
            return res;
        UA_UInt32 current = ftd->entryCount;
        scan.remaining = ftd->config.maxNodes > current ? ftd->config.maxNodes - current : 0;
        scan.mode = FT_RECONCILE_FULL;
    }
    UA_StatusCode res = reconcileTree(server, directory, depth, &scan);
    if(outSummary)
        *outSummary = scan.summary;
    else
        logScanSummary(server, directory, &scan.summary, quiet);
    return res;
}

/* The timer, the start and manual refreshes report the same problems on every
 * pass. They are logged on the first pass after start and on changes. */
UA_StatusCode
fileTransferRefresh(UA_Driver *drv, const UA_NodeId directoryNodeId) {
    FileTransferDriver *ftd = (FileTransferDriver*)drv;
    FTEntry *directory = findFTEntry(ftd, &directoryNodeId);
    if(!directory || !directory->isDirectory || directory->zombie)
        return UA_STATUSCODE_BADNOTFOUND;
    FTScanSummary summary; /* Stays empty if the removal pass fails */
    memset(&summary, 0, sizeof(summary));
    UA_StatusCode res = reconcileDirectory(drv->server, directory, FT_RECONCILE_FULL,
                                           ftd->scanReported, &summary);
    UA_Boolean changed = !ftd->scanReported ||
        summary.skipped != ftd->lastScan.skipped ||
        summary.depthCut != ftd->lastScan.depthCut ||
        summary.nodesCut != ftd->lastScan.nodesCut;
    logScanSummary(drv->server, directory, &summary, !changed);
    ftd->lastScan = summary;
    ftd->scanReported = true;
    return res;
}

/**************************************
 * FileDirectoryType Method Callbacks
 **************************************/

/* The maxNodes ceiling has to apply to Objects created through the Methods as
 * well, not only to the scan -- otherwise a client can grow the address space
 * past the configured limit one CreateFile call at a time. */
static UA_Boolean
nodeBudgetExhausted(FileTransferDriver *ftd) {
    return ftd->config.maxNodes > 0 &&
        ftd->entryCount >= ftd->config.maxNodes;
}

/* The scan does not list entries below max-scan-depth. The Methods must not
 * create them either, the refresh would never see them again. */
static UA_Boolean
childBeyondScanDepth(const FTEntry *directory) {
    UA_UInt32 maxDepth = directory->driver->config.maxScanDepth;
    return maxDepth > 0 && entryDepth(directory) + 1 > maxDepth;
}

/* Create the backend entry and its Object together. Roll back the backend
 * entry if reading metadata or binding the Object fails. */
static UA_StatusCode
createChild(UA_Server *server, FTEntry *directory, const UA_String name,
             UA_Boolean isDirectory, FTEntry **outNode) {
    FileTransferDriver *ftd = directory->driver;
    if(!validEntryName(name))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(nodeBudgetExhausted(ftd) || childBeyondScanDepth(directory))
        return UA_STATUSCODE_BADRESOURCEUNAVAILABLE;

    UA_String path = UA_STRING_NULL;
    UA_StatusCode res = joinPath(directory->path, name, &path);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(findEntryByPath(ftd, path)) {
        res = UA_STATUSCODE_BADBROWSENAMEDUPLICATED;
        goto cleanup;
    }

    UA_FileTransferBackend *backend = &ftd->backend;
    res = createBackendEntry(backend, path, isDirectory);
    if(res == UA_STATUSCODE_GOOD) {
        UA_FileTransferFileInfo info;
        if(!isDirectory)
            res = backendGetInfo(&backend->file, path, &info);
        if(res == UA_STATUSCODE_GOOD)
            res = mirrorObject(server, directory, name, isDirectory ? NULL : &info, outNode);
        if(res != UA_STATUSCODE_GOOD)
            backend->remove(backend, path);
    }
 cleanup:
    UA_String_clear(&path);
    return res;
}

static UA_StatusCode
resolveWritableDirectory(UA_Server *server, const UA_NodeId *sessionId,
                          void *sessionContext, const UA_NodeId *objectId,
                          void *objectContext, FTEntry **outNode) {
    FTEntry *node = resolveFTEntry(server, objectId, objectContext);
    if(!node)
        return UA_STATUSCODE_BADNOTSUPPORTED;
    if(!node->isDirectory || node->zombie)
        return UA_STATUSCODE_BADNOTFOUND;
    *outNode = node;
    return checkFTAccess(server, node, sessionId, sessionContext,
                         UA_FILEACCESS_WRITE | UA_FILEACCESS_TRAVERSE,
                         UA_STATUSCODE_BADUSERACCESSDENIED);
}

static UA_StatusCode
createDirectoryMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                                void *sessionContext, const UA_NodeId *methodId,
                                void *methodContext, const UA_NodeId *objectId,
                                void *objectContext, size_t inputSize, const UA_Variant *input,
                                size_t outputSize, UA_Variant *output) {
    FTEntry *dirNode;
    UA_StatusCode res = resolveWritableDirectory(server, sessionId, sessionContext,
                                                 objectId, objectContext, &dirNode);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    if(inputSize < 1 || outputSize < 1 ||
       !UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_STRING]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_String name = *(UA_String*)input[0].data;
    FTEntry *newNode = NULL;
    res = createChild(server, dirNode, name, true, &newNode);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    return UA_Variant_setScalarCopy(&output[0], &newNode->nodeId,
                                    &UA_TYPES[UA_TYPES_NODEID]);
}

static UA_StatusCode
createFileMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                          void *sessionContext, const UA_NodeId *methodId,
                          void *methodContext, const UA_NodeId *objectId,
                          void *objectContext, size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output) {
    FTEntry *dirNode;
    UA_StatusCode res = resolveWritableDirectory(server, sessionId, sessionContext,
                                                 objectId, objectContext, &dirNode);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    FileTransferDriver *ftd = dirNode->driver;

    if(inputSize < 2 || outputSize < 2 ||
       !UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_STRING]) ||
       !UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_BOOLEAN]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_String name = *(UA_String*)input[0].data;
    UA_Boolean requestFileOpen = *(UA_Boolean*)input[1].data;
    FTEntry *newNode = NULL;
    res = createChild(server, dirNode, name, false, &newNode);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Optionally open the new file for reading and writing. A failed open
     * removes the file again. */
    UA_UInt32 handle = 0;
    if(requestFileOpen) {
        res = openFileHandle(server, newNode, sessionId, sessionContext,
                             UA_OPENFILEMODE_READ | UA_OPENFILEMODE_WRITE,
                             &handle);
        if(res != UA_STATUSCODE_GOOD) {
            ftd->backend.remove(&ftd->backend, newNode->path);
            removeSubtree(server, newNode);
            return res;
        }
    }

    /* The client cannot close a handle it does not receive */
    res = UA_Variant_setScalarCopy(&output[0], &newNode->nodeId,
                                   &UA_TYPES[UA_TYPES_NODEID]);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_Variant_setScalarCopy(&output[1], &handle,
                                       &UA_TYPES[UA_TYPES_UINT32]);
    if(res != UA_STATUSCODE_GOOD && requestFileOpen) {
        FTHandle *h = findFTHandle(ftd, sessionId, handle);
        if(h)
            closeFTHandle(server, h);
    }
    return res;
}

static UA_StatusCode
deleteMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                     void *sessionContext, const UA_NodeId *methodId,
                     void *methodContext, const UA_NodeId *objectId,
                     void *objectContext, size_t inputSize, const UA_Variant *input,
                     size_t outputSize, UA_Variant *output) {
    FTEntry *dirNode;
    UA_StatusCode res = resolveWritableDirectory(server, sessionId, sessionContext,
                                                 objectId, objectContext, &dirNode);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    FileTransferDriver *ftd = dirNode->driver;

    if(inputSize < 1 || !UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_NODEID]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_NodeId *objectToDelete = (UA_NodeId*)input[0].data;

    /* The Object must be a direct child in the managed storage tree. */
    FTEntry *target = findFTEntry(ftd, objectToDelete);
    if(!target || target->zombie || target->parent != dirNode)
        return UA_STATUSCODE_BADNOTFOUND;

    /* Open files cannot be deleted */
    if(target->subtreeHandleCount > 0)
        return UA_STATUSCODE_BADINVALIDSTATE;

    FTTreeAccess access = {server, ftd, sessionId,
                           sessionContext, false, true};
    res = checkTreeAccess(&access, target->path, target->isDirectory);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* A directory can be deleted partially. Then the Objects of the deleted
     * entries are removed (without adding entries). */
    UA_FileTransferBackend *b = &dirNode->driver->backend;
    res = deleteBackendTree(b, target->path, target->isDirectory, &access);
    if(res != UA_STATUSCODE_GOOD) {
        if(target->isDirectory)
            reconcileDirectory(server, dirNode, FT_RECONCILE_REMOVE_ONLY, false, NULL);
        return res;
    }

    removeSubtree(server, target);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
moveOrCopyMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                          void *sessionContext, const UA_NodeId *methodId,
                          void *methodContext, const UA_NodeId *objectId,
                          void *objectContext, size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output) {
    FTEntry *dirNode = resolveFTEntry(server, objectId, objectContext);
    if(!dirNode)
        return UA_STATUSCODE_BADNOTSUPPORTED;
    if(!dirNode->isDirectory || dirNode->zombie)
        return UA_STATUSCODE_BADNOTFOUND;
    FileTransferDriver *ftd = dirNode->driver;
    UA_StatusCode res;

    if(inputSize < 4 || outputSize < 1 ||
       !UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_NODEID]) ||
       !UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_NODEID]) ||
       !UA_Variant_hasScalarType(&input[2], &UA_TYPES[UA_TYPES_BOOLEAN]) ||
       !UA_Variant_hasScalarType(&input[3], &UA_TYPES[UA_TYPES_STRING]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_NodeId *objectToMoveOrCopy = (UA_NodeId*)input[0].data;
    UA_NodeId *targetDirectory = (UA_NodeId*)input[1].data;
    UA_Boolean createCopy = *(UA_Boolean*)input[2].data;
    UA_String newName = *(UA_String*)input[3].data;

    /* The Object must be a direct child in the managed storage tree. */
    FTEntry *source = findFTEntry(ftd, objectToMoveOrCopy);
    if(!source || source->zombie || source->parent != dirNode)
        return UA_STATUSCODE_BADNOTFOUND;

    FTEntry *targetDir = resolveFTEntryById(server, targetDirectory);
    if(!targetDir)
        return UA_STATUSCODE_BADNOTFOUND;
    if(!targetDir->isDirectory || targetDir->zombie)
        return UA_STATUSCODE_BADNOTFOUND;
    FileTransferDriver *dstDriver = targetDir->driver;

    /* The target directory must be writable. For a move the source parent must
     * be writable too, since the source entry is deleted. A copy only reads
     * the source, so a copy from a read-only mount is allowed. */
    res = checkFTAccess(server, targetDir, sessionId, sessionContext,
                        UA_FILEACCESS_WRITE | UA_FILEACCESS_TRAVERSE,
                        UA_STATUSCODE_BADUSERACCESSDENIED);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_FileAccessRights sourceRights = UA_FILEACCESS_TRAVERSE;
    if(!createCopy)
        sourceRights |= UA_FILEACCESS_WRITE;
    res = checkFTAccess(server, dirNode, sessionId, sessionContext,
                        sourceRights, UA_STATUSCODE_BADUSERACCESSDENIED);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* An empty name keeps the current name. Copy the name: it may point
     * into the source registry entry that is removed during a move. */
    UA_String nameRef = (newName.length > 0) ?
        newName : pathLastSegment(source->path);
    if(!validEntryName(nameRef))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_String name = UA_STRING_NULL;
    res = UA_String_copy(&nameRef, &name);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_String destPath = UA_STRING_NULL;
    res = joinPath(targetDir->path, name, &destPath);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    UA_Boolean sameDriver = (source->driver == targetDir->driver);

    /* A rename only changes directory entries. Copies and cross-mount moves
     * read the source tree; the latter also delete its entries. */
    FTTreeAccess access = {server, ftd, sessionId,
                           sessionContext, true, !createCopy};
    if(createCopy || !sameDriver) {
        res = checkTreeAccess(&access, source->path, source->isDirectory);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
    }

    /* Moving to the identical location is a no-op (same mount only) */
    if(!createCopy && sameDriver && UA_String_equal(&destPath, &source->path)) {
        res = UA_Variant_setScalarCopy(&output[0], &source->nodeId,
                                        &UA_TYPES[UA_TYPES_NODEID]);
        goto cleanup;
    }

    if(findEntryByPath(dstDriver, destPath)) {
        res = UA_STATUSCODE_BADBROWSENAMEDUPLICATED;
        goto cleanup;
    }

    /* Objects with open file handles are locked */
    if(source->subtreeHandleCount > 0) {
        res = UA_STATUSCODE_BADINVALIDSTATE;
        goto cleanup;
    }

    /* Copying or moving a directory into itself or its own subtree would
     * recurse without bound: copyBackendTree creates the destination inside the
     * source and then lists the source, which now contains the fresh copy. */
    if(source->isDirectory && sameDriver &&
       pathWithinSubtree(destPath, source->path)) {
        res = UA_STATUSCODE_BADINVALIDARGUMENT;
        goto cleanup;
    }

    /* A copy (also for a move to another mount) creates Objects in the target
     * mount. Every target has to be within the target's scan depth. */
    if(((createCopy || !sameDriver) && nodeBudgetExhausted(dstDriver)) ||
       childBeyondScanDepth(targetDir)) {
        res = UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
        goto cleanup;
    }

    UA_Boolean isDir = source->isDirectory;
    UA_FileTransferBackend *srcB = &source->driver->backend;
    UA_FileTransferBackend *dstB = &targetDir->driver->backend;
    UA_StatusCode deleteRes = UA_STATUSCODE_GOOD;
    if(!createCopy && sameDriver) {
        /* Within one backend a move is an atomic rename */
        res = dstB->rename(dstB, source->path, destPath);
        if(res == UA_STATUSCODE_GOOD)
            removeSubtree(server, source); /* Invalidates source */
    } else {
        /* A copy, or a move to another mount: copy to the target backend and
         * delete from the source backend. A file that cannot be deleted is
         * unchanged and its copy is removed again. A directory can be deleted
         * partially. Then the copy is kept, so that no data is lost, and the
         * source directory is reconciled with its backend. */
        res = copyBackendTree(srcB, source->path, dstB, destPath, isDir, &access);
        if(res == UA_STATUSCODE_GOOD && !createCopy) {
            deleteRes = deleteBackendTree(srcB, source->path, isDir, &access);
            if(deleteRes == UA_STATUSCODE_GOOD) {
                removeSubtree(server, source); /* Invalidates source */
            } else if(!isDir) {
                dstB->remove(dstB, destPath);
                res = deleteRes;
            }
        }
    }

    /* Mirror the entry at the new location */
    FTEntry *newNode = NULL;
    if(res == UA_STATUSCODE_GOOD) {
        if(isDir) {
            res = mirrorObject(server, targetDir, name, NULL, &newNode);
            if(res == UA_STATUSCODE_GOOD) {
                res = reconcileDirectory(server, newNode, FT_RECONCILE_FULL, false, NULL);
                /* The entry was moved or copied. As in the scan, a directory
                 * whose content cannot be listed stays empty. */
                if(res != UA_STATUSCODE_GOOD && !fatalScanError(res)) {
                    UA_LOG_WARNING(UA_Server_getConfig(server)->logging, UA_LOGCATEGORY_SERVER,
                                   "FileTransfer: The directory \"%S\" is "
                                   "mirrored empty, its content cannot be "
                                   "listed: %s", newNode->path,
                                   UA_StatusCode_name(res));
                    res = UA_STATUSCODE_GOOD;
                }
            }
        } else {
            UA_FileTransferFileInfo info;
            res = backendGetInfo(&dstB->file, destPath, &info);
            if(res == UA_STATUSCODE_GOOD)
                res = mirrorObject(server, targetDir, name, &info, &newNode);
        }
    }
    /* The Objects of the entries deleted by an incomplete move are removed
     * (without adding entries) */
    if(deleteRes != UA_STATUSCODE_GOOD) {
        reconcileDirectory(server, dirNode, FT_RECONCILE_REMOVE_ONLY, false, NULL);
        if(res == UA_STATUSCODE_GOOD)
            res = deleteRes;
    }
    if(res == UA_STATUSCODE_GOOD)
        res = UA_Variant_setScalarCopy(&output[0], &newNode->nodeId,
                                        &UA_TYPES[UA_TYPES_NODEID]);
 cleanup:
    UA_String_clear(&name);
    UA_String_clear(&destPath);
    return res;
}

const FTMethod fileDirectoryTypeMethods[] = {
    {UA_NS0ID_FILEDIRECTORYTYPE_CREATEDIRECTORY, "CreateDirectory",
     createDirectoryMethodCallback},
    {UA_NS0ID_FILEDIRECTORYTYPE_CREATEFILE, "CreateFile", createFileMethodCallback},
    {UA_NS0ID_FILEDIRECTORYTYPE_DELETEFILESYSTEMOBJECT, "Delete",
     deleteMethodCallback},
    {UA_NS0ID_FILEDIRECTORYTYPE_MOVEORCOPY, "MoveOrCopy", moveOrCopyMethodCallback}
};
