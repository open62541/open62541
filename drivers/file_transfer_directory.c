/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include "file_transfer_internal.h"

#ifdef UA_ENABLE_DRIVER_FILE_TRANSFER

/**************************************
 * Directory Tree Mirroring
 **************************************/

/* Collected directory listing. Processing the entries after the listing
 * avoids reentrant calls into the backend. */
typedef struct ScanEntry {
    struct ScanEntry *next;
    UA_String name;
    UA_Boolean isDir;
    UA_Boolean matched; /* Used during refresh reconciliation */
} ScanEntry;

typedef struct {
    ScanEntry *entries;
    UA_StatusCode res; /* A failed allocation in the collector */
} ScanList;

static void
scanCollector(void *listContext, const UA_String name, UA_Boolean isDirectory) {
    ScanList *list = (ScanList*)listContext;
    if(list->res != UA_STATUSCODE_GOOD)
        return;
    ScanEntry *e = (ScanEntry*)UA_calloc(1, sizeof(ScanEntry));
    if(!e || UA_String_copy(&name, &e->name) != UA_STATUSCODE_GOOD) {
        UA_free(e);
        list->res = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    e->isDir = isDirectory;
    e->next = list->entries;
    list->entries = e;
}

static void
freeScanEntries(ScanEntry *head) {
    while(head) {
        ScanEntry *next = head->next;
        UA_String_clear(&head->name);
        UA_free(head);
        head = next;
    }
}

/* List a directory completely or not at all. The callers remove, skip or
 * delete what is not listed, so a partial listing must not reach them. */
static UA_StatusCode
listEntries(UA_FileTransferBackend *b, const UA_String path,
            ScanEntry **outEntries) {
    ScanList list = {NULL, UA_STATUSCODE_GOOD};
    UA_StatusCode res = b->listDirectory(b, path, scanCollector, &list);
    if(res == UA_STATUSCODE_GOOD)
        res = list.res;
    if(res != UA_STATUSCODE_GOOD) {
        freeScanEntries(list.entries);
        return res;
    }
    *outEntries = list.entries;
    return UA_STATUSCODE_GOOD;
}

/* Entry names must not contain path separators or navigate the hierarchy */
UA_Boolean
validEntryName(const UA_String name) {
    if(name.length == 0)
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
static UA_StatusCode
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

static UA_String
pathLastSegment(const UA_String path) {
    for(size_t i = path.length; i > 0; i--) {
        if(path.data[i - 1] == '/') {
            UA_String segment = {path.length - i, path.data + i};
            return segment;
        }
    }
    return path;
}

UA_UInt32
pathDepth(const UA_String path) {
    if(path.length == 0)
        return 0;
    UA_UInt32 depth = 1;
    for(size_t i = 0; i < path.length; i++) {
        if(path.data[i] == '/')
            depth++;
    }
    return depth;
}

static UA_Boolean
pathWithinSubtree(const UA_String path, const UA_String subtreeRoot) {
    if(subtreeRoot.length == 0)
        return true; /* The mount root contains everything */
    if(path.length < subtreeRoot.length ||
       memcmp(path.data, subtreeRoot.data, subtreeRoot.length) != 0)
        return false;
    return path.length == subtreeRoot.length ||
        path.data[subtreeRoot.length] == '/';
}

static UA_Boolean
isDirectChildPath(const UA_String parent, const UA_String child) {
    size_t offset = 0;
    if(parent.length > 0) {
        if(child.length <= parent.length + 1 ||
           memcmp(child.data, parent.data, parent.length) != 0 ||
           child.data[parent.length] != '/')
            return false;
        offset = parent.length + 1;
    }
    if(child.length == offset)
        return false;
    for(size_t i = offset; i < child.length; i++) {
        if(child.data[i] == '/')
            return false;
    }
    return true;
}

static FTNode *
findChildByPath(FileTransferDriver *ftd, FTMount *mount, const UA_String path) {
    FTNode *node;
    LIST_FOREACH(node, &ftd->nodes, listEntry) {
        if(node->mount == mount && UA_String_equal(&node->path, &path))
            return node;
    }
    return NULL;
}

/* Is the node below the root of a subtree? A file has no subtree. An entry
 * with the path of the root is not below it either: it replaced the vanished
 * root (a directory for a file or the other way round). */
static UA_Boolean
isBelow(const FTNode *node, const FTNode *root) {
    return root->isDirectory && node->mount == root->mount &&
        node->path.length > root->path.length &&
        pathWithinSubtree(node->path, root->path);
}

static UA_Boolean
subtreeHasOpenHandles(FileTransferDriver *ftd, FTNode *root) {
    FTHandle *h;
    LIST_FOREACH(h, &ftd->handles, listEntry) {
        if(h->file == root || isBelow(h->file, root))
            return true;
    }
    return false;
}

UA_UInt32
countMountNodes(FileTransferDriver *ftd, FTMount *mount) {
    UA_UInt32 count = 0;
    FTNode *node;
    LIST_FOREACH(node, &ftd->nodes, listEntry) {
        if(node->mount == mount)
            count++;
    }
    return count;
}

/* Create a FileType Object (with info) or a FileDirectoryType Object (info
 * NULL) with its FTNode below a directory node */
static UA_StatusCode
mirrorObject(UA_Server *server, FileTransferDriver *ftd, FTNode *dirNode,
             const UA_String name, const UA_FileTransferFileInfo *info,
             FTNode **outNode) {
    UA_String path = UA_STRING_NULL;
    UA_StatusCode res = joinPath(dirNode->path, name, &path);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* The generated NodeId (and those of the instantiated children) are in
     * the namespace of the mount as well */
    UA_UInt16 nsIndex = dirNode->mount->options.namespaceIndex;
    UA_QualifiedName browseName = {nsIndex, name};
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName.text = name;
    UA_NodeId newNodeId = UA_NODEID_NULL;
    res = UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(nsIndex, 0),
                                  dirNode->nodeId,
                                  UA_NS0ID(ORGANIZES), browseName,
                                  (info) ? UA_NS0ID(FILETYPE) :
                                  UA_NS0ID(FILEDIRECTORYTYPE),
                                  attr, NULL, &newNodeId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_String_clear(&path);
        return res;
    }

    FTNode *node = newFTNode(ftd, dirNode->mount, newNodeId, path, !info);
    if(!node)
        res = UA_STATUSCODE_BADOUTOFMEMORY;
    else if(info)
        res = setupFileNode(server, ftd, node, info);
    if(res == UA_STATUSCODE_GOOD)
        res = bindObjectMethods(server, node);
    if(res != UA_STATUSCODE_GOOD) {
        if(node)
            removeSubtree(server, ftd, node);
        else
            UA_Server_deleteNode(server, newNodeId, true);
    } else if(outNode) {
        *outNode = node;
    }
    UA_NodeId_clear(&newNodeId);
    UA_String_clear(&path);
    return res;
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

/* Mirror one directory entry below dirNode. Returns the StatusCode of the
 * operation; the caller decides whether to skip the entry or abort the scan.
 * dirPath is passed explicitly so the caller can hand over a snapshot of
 * dirNode->path where it holds one. */
static UA_StatusCode
mirrorEntry(UA_Server *server, FileTransferDriver *ftd, FTNode *dirNode,
            const UA_String dirPath, const ScanEntry *e, UA_UInt32 depth,
            UA_UInt32 *nodeBudget) {
    if(e->isDir) {
        FTNode *childNode = NULL;
        UA_StatusCode res =
            mirrorObject(server, ftd, dirNode, e->name, NULL, &childNode);
        if(res != UA_STATUSCODE_GOOD)
            return res;
        (*nodeBudget)--;
        /* The Object itself exists from here on. A subdirectory whose content
         * cannot be listed stays as an empty Object, so report that instead of
         * letting the caller log it as a skipped entry. */
        res = fileTransferMirrorTree(server, ftd, childNode, depth + 1,
                                     nodeBudget);
        if(res == UA_STATUSCODE_GOOD || fatalScanError(res))
            return res;
        UA_LOG_WARNING(ftd->logging, UA_LOGCATEGORY_SERVER,
                       "FileTransfer: The directory \"%S\" is mirrored empty, "
                       "its content cannot be listed: %s",
                       childNode->path, UA_StatusCode_name(res));
        return UA_STATUSCODE_GOOD;
    }

    UA_String childPath = UA_STRING_NULL;
    UA_StatusCode res = joinPath(dirPath, e->name, &childPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_FileTransferBackend *b = &dirNode->mount->backend;
    UA_FileTransferFileInfo info;
    res = backendGetInfo(&b->file, childPath, &info);
    UA_String_clear(&childPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = mirrorObject(server, ftd, dirNode, e->name, &info, NULL);
    if(res == UA_STATUSCODE_GOOD)
        (*nodeBudget)--;
    return res;
}

/* Walk the collected listing and mirror every entry, skipping the ones that
 * cannot be represented. Shared by the initial scan and the refresh. */
static UA_StatusCode
mirrorEntries(UA_Server *server, FileTransferDriver *ftd, FTNode *dirNode,
              const UA_String dirPath, ScanEntry *entries, UA_Boolean skipMatched,
              UA_UInt32 depth, UA_UInt32 *nodeBudget) {
    for(ScanEntry *e = entries; e; e = e->next) {
        if(skipMatched && e->matched)
            continue;
        if(!validEntryName(e->name)) {
            UA_LOG_WARNING(ftd->logging, UA_LOGCATEGORY_SERVER,
                           "FileTransfer: Skipping the entry \"%S\" with an "
                           "invalid name", e->name);
            continue;
        }
        if(*nodeBudget == 0) {
            UA_LOG_WARNING(ftd->logging, UA_LOGCATEGORY_SERVER,
                           "FileTransfer: The maxNodes limit is reached. "
                           "Entries are not mirrored into the address space");
            break;
        }

        UA_StatusCode res =
            mirrorEntry(server, ftd, dirNode, dirPath, e, depth, nodeBudget);
        if(res == UA_STATUSCODE_GOOD)
            continue;
        if(fatalScanError(res))
            return res;
        UA_LOG_WARNING(ftd->logging, UA_LOGCATEGORY_SERVER,
                       "FileTransfer: Skipping the entry \"%S\": %s",
                       e->name, UA_StatusCode_name(res));
    }
    return UA_STATUSCODE_GOOD;
}

/* Recursively mirror the backend content below a directory node */
UA_StatusCode
fileTransferMirrorTree(UA_Server *server, FileTransferDriver *ftd, FTNode *dirNode,
                       UA_UInt32 depth, UA_UInt32 *nodeBudget) {
    const UA_FileTransferMountOptions *opts = &dirNode->mount->options;
    if(opts->maxScanDepth > 0 && depth > opts->maxScanDepth)
        return UA_STATUSCODE_GOOD;

    UA_FileTransferBackend *b = &dirNode->mount->backend;
    ScanEntry *entries = NULL;
    /* Failing to list this directory is reported to the caller. For the mount
     * root that fails addFileSystem; for a subdirectory the recursion site
     * turns it into a skipped entry. */
    UA_StatusCode res = listEntries(b, dirNode->path, &entries);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    res = mirrorEntries(server, ftd, dirNode, dirNode->path, entries, false,
                        depth, nodeBudget);
    freeScanEntries(entries);
    return res;
}

/* Remove the Objects and registry entries of a subtree, the given root
 * included. The caller must ensure that no open handles refer to the
 * subtree.
 *
 * Deleting a node keeps a child that has another hierarchical parent. So every
 * Object is deleted explicitly, and the value sources that point to the FTNodes
 * are released before anything is deleted: a Property can survive in the same
 * way. The registry is ordered newest first, so children go before their
 * parent. */
void
removeSubtree(UA_Server *server, FileTransferDriver *ftd, FTNode *subtreeRoot) {
    FTNode *node, *tmp;
    LIST_FOREACH(node, &ftd->nodes, listEntry) {
        if(!node->isDirectory && isBelow(node, subtreeRoot))
            releaseFileNode(server, node);
    }
    if(!subtreeRoot->isDirectory)
        releaseFileNode(server, subtreeRoot);

    LIST_FOREACH_SAFE(node, &ftd->nodes, listEntry, tmp) {
        if(!isBelow(node, subtreeRoot))
            continue;
        UA_Server_deleteNode(server, node->nodeId, true);
        removeFTNode(ftd, node);
    }
    /* An attached Object belongs to the application and is kept */
    if(subtreeRoot->mount->attached)
        setObjectMethodCallbacks(server, ftd, subtreeRoot, false);
    else
        UA_Server_deleteNode(server, subtreeRoot->nodeId, true);
    removeFTNode(ftd, subtreeRoot);
}

/* A subtree with open handles cannot be removed right away when its backend
 * entries vanished. The nodes stay in the address space so that the open
 * handles remain usable (Close in particular). Zombie file nodes are removed
 * when their last handle is closed; the remaining zombie nodes are collected
 * on the next refresh. */
static void
markSubtreeZombie(FileTransferDriver *ftd, FTNode *subtreeRoot) {
    subtreeRoot->zombie = true;
    FTNode *node;
    LIST_FOREACH(node, &ftd->nodes, listEntry) {
        if(isBelow(node, subtreeRoot))
            node->zombie = true;
    }
}

/* Recursively delete a backend subtree (bottom-up) */
static UA_StatusCode
deleteBackendTree(UA_FileTransferBackend *b, const UA_String path,
                  UA_Boolean isDir) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(isDir) {
        ScanEntry *entries = NULL;
        res = listEntries(b, path, &entries);
        if(res != UA_STATUSCODE_GOOD)
            return res;
        for(ScanEntry *e = entries; e && res == UA_STATUSCODE_GOOD; e = e->next) {
            UA_String childPath = UA_STRING_NULL;
            res = joinPath(path, e->name, &childPath);
            if(res == UA_STATUSCODE_GOOD)
                res = deleteBackendTree(b, childPath, e->isDir);
            UA_String_clear(&childPath);
        }
        freeScanEntries(entries);
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
    res = dstB->createFile(dstB, toPath);
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
                UA_Boolean isDir) {
    if(!isDir)
        return copyBackendFile(srcB, fromPath, dstB, toPath);

    UA_StatusCode res = dstB->createDirectory(dstB, toPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    ScanEntry *entries = NULL;
    res = listEntries(srcB, fromPath, &entries);
    for(ScanEntry *e = entries; e && res == UA_STATUSCODE_GOOD; e = e->next) {
        UA_String childFrom = UA_STRING_NULL;
        UA_String childTo = UA_STRING_NULL;
        res = joinPath(fromPath, e->name, &childFrom);
        if(res == UA_STATUSCODE_GOOD)
            res = joinPath(toPath, e->name, &childTo);
        if(res == UA_STATUSCODE_GOOD)
            res = copyBackendTree(srcB, childFrom, dstB, childTo, e->isDir);
        UA_String_clear(&childFrom);
        UA_String_clear(&childTo);
    }
    freeScanEntries(entries);

    /* The directory was created by the copy. Remove a partial copy so that a
     * failed copy does not leave an entry behind that is not mirrored. */
    if(res != UA_STATUSCODE_GOOD)
        deleteBackendTree(dstB, toPath, true);
    return res;
}

/* Reconcile a mirrored directory with the backend content */
UA_StatusCode
fileTransferSyncTree(UA_Server *server, FileTransferDriver *ftd, FTNode *dirNode,
                     UA_UInt32 depth, UA_UInt32 *nodeBudget) {
    const UA_FileTransferMountOptions *opts = &dirNode->mount->options;
    if(opts->maxScanDepth > 0 && depth > opts->maxScanDepth)
        return UA_STATUSCODE_GOOD;

    UA_FileTransferBackend *b = &dirNode->mount->backend;
    ScanEntry *entries = NULL;
    UA_StatusCode res = listEntries(b, dirNode->path, &entries);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Snapshot the fields read inside the loop. removeSubtree(child) frees
     * child->path/mount; although it never frees dirNode, the analyzer cannot
     * prove that, so read dirNode only once before iterating. */
    FTMount *dirMount = dirNode->mount;
    UA_String dirPath;
    if(UA_String_copy(&dirNode->path, &dirPath) != UA_STATUSCODE_GOOD) {
        freeScanEntries(entries);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Match the registry children against the backend listing. Vanished
     * entries are removed, or marked as zombies while open handles refer to
     * them. */
    FTNode *child, *childTmp;
    LIST_FOREACH_SAFE(child, &ftd->nodes, listEntry, childTmp) {
        if(child->mount != dirMount ||
           !isDirectChildPath(dirPath, child->path))
            continue;
        UA_String name = pathLastSegment(child->path);
        ScanEntry *match = NULL;
        for(ScanEntry *e = entries; e; e = e->next) {
            if(!e->matched && e->isDir == child->isDirectory &&
               UA_String_equal(&e->name, &name)) {
                match = e;
                break;
            }
        }
        if(match) {
            match->matched = true;
            child->zombie = false; /* The backend entry (re)appeared */
        } else if(subtreeHasOpenHandles(ftd, child)) {
            markSubtreeZombie(ftd, child);
        } else {
            removeSubtree(server, ftd, child);
        }
    }

    /* Create nodes for new backend entries. Entries that cannot be mirrored
     * are skipped with a warning rather than aborting the reconciliation.
     * Without a budget, only the vanished entries are removed. */
    if(nodeBudget)
        res = mirrorEntries(server, ftd, dirNode, dirPath, entries, true,
                            depth, nodeBudget);
    freeScanEntries(entries);
    if(res != UA_STATUSCODE_GOOD) {
        UA_String_clear(&dirPath);
        return res;
    }

    /* Recurse into the (kept) subdirectories. A subdirectory that cannot be
     * listed any more keeps its nodes and does not stop the walk. */
    LIST_FOREACH_SAFE(child, &ftd->nodes, listEntry, childTmp) {
        if(child->mount != dirMount || !child->isDirectory ||
           child->zombie || !isDirectChildPath(dirPath, child->path))
            continue;
        UA_StatusCode childRes =
            fileTransferSyncTree(server, ftd, child, depth + 1, nodeBudget);
        if(childRes == UA_STATUSCODE_GOOD)
            continue;
        if(fatalScanError(childRes)) {
            res = childRes;
            break;
        }
        UA_LOG_WARNING(ftd->logging, UA_LOGCATEGORY_SERVER,
                       "FileTransfer: Skipping the refresh of \"%S\": %s",
                       child->path, UA_StatusCode_name(childRes));
    }
    UA_String_clear(&dirPath);
    return res;
}

/**************************************
 * FileDirectoryType Method Callbacks
 **************************************/

/* The maxNodes ceiling has to apply to Objects created through the Methods as
 * well, not only to the scan -- otherwise a client can grow the address space
 * past the configured limit one CreateFile call at a time. */
static UA_Boolean
mountNodeBudgetExhausted(FileTransferDriver *ftd, FTMount *mount) {
    return mount->options.maxNodes > 0 &&
        countMountNodes(ftd, mount) >= mount->options.maxNodes;
}

/* Resolve the FTNode of a directory Object addressed by a Method call */
static UA_StatusCode
resolveDirectoryNode(FileTransferDriver *ftd, const UA_NodeId *objectId,
                     FTNode **outNode) {
    FTNode *node = findFTNode(ftd, objectId);
    if(!node || !node->isDirectory || node->zombie)
        return UA_STATUSCODE_BADNOTFOUND;
    *outNode = node;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
createDirectoryMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                                void *sessionContext, const UA_NodeId *methodId,
                                void *methodContext, const UA_NodeId *objectId,
                                void *objectContext, size_t inputSize, const UA_Variant *input,
                                size_t outputSize, UA_Variant *output) {
    FileTransferDriver *ftd = findFileTransferDriver(server);
    if(!ftd)
        return UA_STATUSCODE_BADNOTSUPPORTED;

    FTNode *dirNode = NULL;
    UA_StatusCode res = resolveDirectoryNode(ftd, objectId, &dirNode);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(!userCanWrite(server, dirNode, sessionId))
        return UA_STATUSCODE_BADUSERACCESSDENIED;

    if(inputSize < 1 || outputSize < 1 ||
       !UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_STRING]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_String name = *(UA_String*)input[0].data;
    if(!validEntryName(name))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(mountNodeBudgetExhausted(ftd, dirNode->mount))
        return UA_STATUSCODE_BADRESOURCEUNAVAILABLE;

    UA_String newPath = UA_STRING_NULL;
    res = joinPath(dirNode->path, name, &newPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(findChildByPath(ftd, dirNode->mount, newPath)) {
        UA_String_clear(&newPath);
        return UA_STATUSCODE_BADBROWSENAMEDUPLICATED;
    }

    /* A directory that cannot be mirrored is removed again. Otherwise a retry
     * would fail on an entry the client cannot see. */
    UA_FileTransferBackend *b = &dirNode->mount->backend;
    res = b->createDirectory(b, newPath);
    FTNode *newNode = NULL;
    if(res == UA_STATUSCODE_GOOD) {
        res = mirrorObject(server, ftd, dirNode, name, NULL, &newNode);
        if(res != UA_STATUSCODE_GOOD)
            b->remove(b, newPath);
    }
    UA_String_clear(&newPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    return UA_Variant_setScalarCopy(&output[0], &newNode->nodeId,
                                    &UA_TYPES[UA_TYPES_NODEID]);
}

UA_StatusCode
createFileMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                          void *sessionContext, const UA_NodeId *methodId,
                          void *methodContext, const UA_NodeId *objectId,
                          void *objectContext, size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output) {
    FileTransferDriver *ftd = findFileTransferDriver(server);
    if(!ftd)
        return UA_STATUSCODE_BADNOTSUPPORTED;

    FTNode *dirNode = NULL;
    UA_StatusCode res = resolveDirectoryNode(ftd, objectId, &dirNode);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(!userCanWrite(server, dirNode, sessionId))
        return UA_STATUSCODE_BADUSERACCESSDENIED;

    if(inputSize < 2 || outputSize < 2 ||
       !UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_STRING]) ||
       !UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_BOOLEAN]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_String name = *(UA_String*)input[0].data;
    UA_Boolean requestFileOpen = *(UA_Boolean*)input[1].data;
    if(!validEntryName(name))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(mountNodeBudgetExhausted(ftd, dirNode->mount))
        return UA_STATUSCODE_BADRESOURCEUNAVAILABLE;

    UA_String newPath = UA_STRING_NULL;
    res = joinPath(dirNode->path, name, &newPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(findChildByPath(ftd, dirNode->mount, newPath)) {
        UA_String_clear(&newPath);
        return UA_STATUSCODE_BADBROWSENAMEDUPLICATED;
    }

    /* A file that cannot be mirrored is removed again. Otherwise a retry
     * would fail on an entry the client cannot see. */
    UA_FileTransferBackend *b = &dirNode->mount->backend;
    res = b->createFile(b, newPath);
    if(res != UA_STATUSCODE_GOOD) {
        UA_String_clear(&newPath);
        return res;
    }
    UA_FileTransferFileInfo info;
    FTNode *newNode = NULL;
    res = backendGetInfo(&b->file, newPath, &info);
    if(res == UA_STATUSCODE_GOOD)
        res = mirrorObject(server, ftd, dirNode, name, &info, &newNode);
    if(res != UA_STATUSCODE_GOOD)
        b->remove(b, newPath);
    UA_String_clear(&newPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Optionally open the new file for reading and writing. A failed open
     * removes the file again. */
    UA_UInt32 handle = 0;
    if(requestFileOpen) {
        res = openFileHandle(server, ftd, newNode, sessionId,
                             UA_OPENFILEMODE_READ | UA_OPENFILEMODE_WRITE,
                             &handle);
        if(res != UA_STATUSCODE_GOOD) {
            b->remove(b, newNode->path);
            removeSubtree(server, ftd, newNode);
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
            closeFTHandle(server, ftd, h);
    }
    return res;
}

UA_StatusCode
deleteMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                     void *sessionContext, const UA_NodeId *methodId,
                     void *methodContext, const UA_NodeId *objectId,
                     void *objectContext, size_t inputSize, const UA_Variant *input,
                     size_t outputSize, UA_Variant *output) {
    FileTransferDriver *ftd = findFileTransferDriver(server);
    if(!ftd)
        return UA_STATUSCODE_BADNOTSUPPORTED;

    FTNode *dirNode = NULL;
    UA_StatusCode res = resolveDirectoryNode(ftd, objectId, &dirNode);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(!userCanWrite(server, dirNode, sessionId))
        return UA_STATUSCODE_BADUSERACCESSDENIED;

    if(inputSize < 1 || !UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_NODEID]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_NodeId *objectToDelete = (UA_NodeId*)input[0].data;

    /* The object must be organized by this directory */
    FTNode *target = findFTNode(ftd, objectToDelete);
    if(!target || target->zombie || target->mount != dirNode->mount ||
       !isDirectChildPath(dirNode->path, target->path))
        return UA_STATUSCODE_BADNOTFOUND;

    /* Open files cannot be deleted */
    if(subtreeHasOpenHandles(ftd, target))
        return UA_STATUSCODE_BADINVALIDSTATE;

    /* A directory can be deleted partially. Then the Objects of the deleted
     * entries are removed (without a node budget, nothing is added). */
    UA_FileTransferBackend *b = &dirNode->mount->backend;
    res = deleteBackendTree(b, target->path, target->isDirectory);
    if(res != UA_STATUSCODE_GOOD) {
        if(target->isDirectory)
            fileTransferSyncTree(server, ftd, dirNode,
                                 pathDepth(dirNode->path) + 1, NULL);
        return res;
    }

    removeSubtree(server, ftd, target);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
moveOrCopyMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                          void *sessionContext, const UA_NodeId *methodId,
                          void *methodContext, const UA_NodeId *objectId,
                          void *objectContext, size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output) {
    FileTransferDriver *ftd = findFileTransferDriver(server);
    if(!ftd)
        return UA_STATUSCODE_BADNOTSUPPORTED;

    FTNode *dirNode = NULL;
    UA_StatusCode res = resolveDirectoryNode(ftd, objectId, &dirNode);
    if(res != UA_STATUSCODE_GOOD)
        return res;

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

    /* The object must be organized by this directory */
    FTNode *source = findFTNode(ftd, objectToMoveOrCopy);
    if(!source || source->zombie || source->mount != dirNode->mount ||
       !isDirectChildPath(dirNode->path, source->path))
        return UA_STATUSCODE_BADNOTFOUND;

    FTNode *targetDir = NULL;
    res = resolveDirectoryNode(ftd, targetDirectory, &targetDir);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* The target directory must be writable. For a move the source parent must
     * be writable too, since the source entry is deleted. A copy only reads
     * the source, so a copy from a read-only mount is allowed. */
    if(!userCanWrite(server, targetDir, sessionId))
        return UA_STATUSCODE_BADUSERACCESSDENIED;
    if(!createCopy && !userCanWrite(server, dirNode, sessionId))
        return UA_STATUSCODE_BADUSERACCESSDENIED;

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
    if(res != UA_STATUSCODE_GOOD) {
        UA_String_clear(&name);
        return res;
    }

    UA_Boolean sameMount = (source->mount == targetDir->mount);

    /* Moving to the identical location is a no-op (same mount only) */
    if(!createCopy && sameMount && UA_String_equal(&destPath, &source->path)) {
        UA_String_clear(&name);
        UA_String_clear(&destPath);
        return UA_Variant_setScalarCopy(&output[0], &source->nodeId,
                                        &UA_TYPES[UA_TYPES_NODEID]);
    }

    if(findChildByPath(ftd, targetDir->mount, destPath)) {
        UA_String_clear(&name);
        UA_String_clear(&destPath);
        return UA_STATUSCODE_BADBROWSENAMEDUPLICATED;
    }

    /* Objects with open file handles are locked */
    if(subtreeHasOpenHandles(ftd, source)) {
        UA_String_clear(&name);
        UA_String_clear(&destPath);
        return UA_STATUSCODE_BADINVALIDSTATE;
    }

    /* Copying or moving a directory into itself or its own subtree would
     * recurse without bound: copyBackendTree creates the destination inside the
     * source and then lists the source, which now contains the fresh copy. */
    if(source->isDirectory && sameMount &&
       pathWithinSubtree(destPath, source->path)) {
        UA_String_clear(&name);
        UA_String_clear(&destPath);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    /* A copy (also for a move to another mount) creates Objects in the target
     * mount */
    if((createCopy || !sameMount) &&
       mountNodeBudgetExhausted(ftd, targetDir->mount)) {
        UA_String_clear(&name);
        UA_String_clear(&destPath);
        return UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
    }

    UA_Boolean isDir = source->isDirectory;
    UA_FileTransferBackend *srcB = &source->mount->backend;
    UA_FileTransferBackend *dstB = &targetDir->mount->backend;
    UA_StatusCode deleteRes = UA_STATUSCODE_GOOD;
    if(!createCopy && sameMount) {
        /* Within one backend a move is an atomic rename */
        res = dstB->rename(dstB, source->path, destPath);
        if(res == UA_STATUSCODE_GOOD)
            removeSubtree(server, ftd, source); /* Invalidates source */
    } else {
        /* A copy, or a move to another mount: copy to the target backend and
         * delete from the source backend. A file that cannot be deleted is
         * unchanged and its copy is removed again. A directory can be deleted
         * partially. Then the copy is kept, so that no data is lost, and the
         * source directory is reconciled with its backend. */
        res = copyBackendTree(srcB, source->path, dstB, destPath, isDir);
        if(res == UA_STATUSCODE_GOOD && !createCopy) {
            deleteRes = deleteBackendTree(srcB, source->path, isDir);
            if(deleteRes == UA_STATUSCODE_GOOD) {
                removeSubtree(server, ftd, source); /* Invalidates source */
            } else if(!isDir) {
                dstB->remove(dstB, destPath);
                res = deleteRes;
            }
        }
    }

    /* Mirror the entry at the new location */
    FTNode *newNode = NULL;
    if(res == UA_STATUSCODE_GOOD) {
        if(isDir) {
            res = mirrorObject(server, ftd, targetDir, name, NULL, &newNode);
            if(res == UA_STATUSCODE_GOOD) {
                UA_UInt32 nodeBudget = (UA_UInt32)0xffffffffu;
                const UA_FileTransferMountOptions *opts = &targetDir->mount->options;
                if(opts->maxNodes > 0) {
                    UA_UInt32 current = countMountNodes(ftd, targetDir->mount);
                    nodeBudget = (opts->maxNodes > current) ?
                        opts->maxNodes - current : 0;
                }
                res = fileTransferMirrorTree(server, ftd, newNode,
                                             pathDepth(destPath) + 1, &nodeBudget);
                /* The entry was moved or copied. As in the scan, a directory
                 * whose content cannot be listed stays empty. */
                if(res != UA_STATUSCODE_GOOD && !fatalScanError(res)) {
                    UA_LOG_WARNING(ftd->logging, UA_LOGCATEGORY_SERVER,
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
                res = mirrorObject(server, ftd, targetDir, name, &info, &newNode);
        }
    }
    UA_String_clear(&name);
    UA_String_clear(&destPath);

    /* The Objects of the entries deleted by an incomplete move are removed
     * (without a node budget, nothing is added) */
    if(deleteRes != UA_STATUSCODE_GOOD) {
        fileTransferSyncTree(server, ftd, dirNode, pathDepth(dirNode->path) + 1,
                             NULL);
        return (res != UA_STATUSCODE_GOOD) ? res : deleteRes;
    }
    if(res != UA_STATUSCODE_GOOD)
        return res;
    return UA_Variant_setScalarCopy(&output[0], &newNode->nodeId,
                                    &UA_TYPES[UA_TYPES_NODEID]);
}

#endif /* UA_ENABLE_DRIVER_FILE_TRANSFER */
