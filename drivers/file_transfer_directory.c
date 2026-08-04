/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include "file_transfer_internal.h"

#if defined(UA_ENABLE_METHODCALLS) && defined(UA_GENERATED_NAMESPACE_ZERO_FULL)

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

static void
scanCollector(void *listContext, const UA_String name, UA_Boolean isDirectory) {
    ScanEntry **head = (ScanEntry**)listContext;
    ScanEntry *e = (ScanEntry*)UA_calloc(1, sizeof(ScanEntry));
    if(!e)
        return;
    if(UA_String_copy(&name, &e->name) != UA_STATUSCODE_GOOD) {
        UA_free(e);
        return;
    }
    e->isDir = isDirectory;
    e->next = *head;
    *head = e;
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

static UA_Boolean
subtreeHasOpenHandles(FileTransferDriver *ftd, FTNode *root) {
    FTHandle *h;
    LIST_FOREACH(h, &ftd->handles, listEntry) {
        if(h->file->mount == root->mount &&
           pathWithinSubtree(h->file->path, root->path))
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

/* Create a FileType Object with its FTNode below a directory node */
static UA_StatusCode
mirrorFile(UA_Server *server, FileTransferDriver *ftd, FTNode *dirNode,
           const UA_String name, const UA_FileTransferFileInfo *info,
           FTNode **outNode) {
    UA_String path = UA_STRING_NULL;
    UA_StatusCode res = joinPath(dirNode->path, name, &path);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_QualifiedName browseName = {0, name};
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName.text = name;
    UA_NodeId newNodeId = UA_NODEID_NULL;
    res = UA_Server_addObjectNode(server, UA_NODEID_NULL, dirNode->nodeId,
                                  UA_NS0ID(ORGANIZES), browseName,
                                  UA_NS0ID(FILETYPE), attr, NULL, &newNodeId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_String_clear(&path);
        return res;
    }

    FTNode *node = newFTNode(ftd, dirNode->mount, newNodeId, path, false);
    if(node)
        res = setupFileNode(server, ftd, node, info);
    else
        res = UA_STATUSCODE_BADOUTOFMEMORY;
    if(res != UA_STATUSCODE_GOOD) {
        UA_Server_deleteNode(server, newNodeId, true);
        if(node)
            removeFTNode(ftd, node);
    } else if(outNode) {
        *outNode = node;
    }
    UA_NodeId_clear(&newNodeId);
    UA_String_clear(&path);
    return res;
}

/* Create a FileDirectoryType Object with its FTNode below a directory node */
static UA_StatusCode
mirrorDirectory(UA_Server *server, FileTransferDriver *ftd, FTNode *dirNode,
                const UA_String name, FTNode **outNode) {
    UA_String path = UA_STRING_NULL;
    UA_StatusCode res = joinPath(dirNode->path, name, &path);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_QualifiedName browseName = {0, name};
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName.text = name;
    UA_NodeId newNodeId = UA_NODEID_NULL;
    res = UA_Server_addObjectNode(server, UA_NODEID_NULL, dirNode->nodeId,
                                  UA_NS0ID(ORGANIZES), browseName,
                                  UA_NS0ID(FILEDIRECTORYTYPE), attr, NULL,
                                  &newNodeId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_String_clear(&path);
        return res;
    }

    FTNode *node = newFTNode(ftd, dirNode->mount, newNodeId, path, true);
    if(!node) {
        UA_Server_deleteNode(server, newNodeId, true);
        res = UA_STATUSCODE_BADOUTOFMEMORY;
    } else if(outNode) {
        *outNode = node;
    }
    UA_NodeId_clear(&newNodeId);
    UA_String_clear(&path);
    return res;
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
    UA_StatusCode res = b->listDirectory(b, dirNode->path, scanCollector,
                                         &entries);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    for(ScanEntry *e = entries; e && res == UA_STATUSCODE_GOOD; e = e->next) {
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

        if(e->isDir) {
            FTNode *childNode = NULL;
            res = mirrorDirectory(server, ftd, dirNode, e->name, &childNode);
            if(res == UA_STATUSCODE_GOOD) {
                (*nodeBudget)--;
                res = fileTransferMirrorTree(server, ftd, childNode, depth + 1, nodeBudget);
            }
        } else {
            UA_String childPath = UA_STRING_NULL;
            res = joinPath(dirNode->path, e->name, &childPath);
            if(res != UA_STATUSCODE_GOOD)
                break;
            UA_FileTransferFileInfo info;
            res = b->getAttributes(b, childPath, &info);
            UA_String_clear(&childPath);
            if(res != UA_STATUSCODE_GOOD)
                break;
            res = mirrorFile(server, ftd, dirNode, e->name, &info, NULL);
            if(res == UA_STATUSCODE_GOOD)
                (*nodeBudget)--;
        }
    }

    freeScanEntries(entries);
    return res;
}

/* Remove the address-space nodes and registry entries below (and including)
 * the given subtree root. The caller must ensure that no open handles refer
 * to the subtree. */
static void
removeSubtree(UA_Server *server, FileTransferDriver *ftd, FTNode *subtreeRoot) {
    /* Deleting the root node removes the mirrored children recursively */
    UA_Server_deleteNode(server, subtreeRoot->nodeId, true);

    /* Copy the root path: removeFTNode frees node->path, and the subtree root
     * is one of the nodes removed below, so borrowing its path across the loop
     * would be a use-after-free. */
    FTMount *mount = subtreeRoot->mount;
    UA_String rootPath;
    if(UA_String_copy(&subtreeRoot->path, &rootPath) != UA_STATUSCODE_GOOD)
        rootPath = UA_STRING_NULL;
    FTNode *node, *tmp;
    LIST_FOREACH_SAFE(node, &ftd->nodes, listEntry, tmp) {
        if(node->mount == mount && pathWithinSubtree(node->path, rootPath))
            removeFTNode(ftd, node);
    }
    UA_String_clear(&rootPath);
}

/* A subtree with open handles cannot be removed right away when its backend
 * entries vanished. The nodes stay in the address space so that the open
 * handles remain usable (Close in particular). Zombie file nodes are removed
 * when their last handle is closed; the remaining zombie nodes are collected
 * on the next refresh. */
static void
markSubtreeZombie(FileTransferDriver *ftd, FTNode *subtreeRoot) {
    FTNode *node;
    LIST_FOREACH(node, &ftd->nodes, listEntry) {
        if(node->mount == subtreeRoot->mount &&
           pathWithinSubtree(node->path, subtreeRoot->path))
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
        res = b->listDirectory(b, path, scanCollector, &entries);
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

    void *src = NULL;
    UA_StatusCode res = srcB->openFile(srcB, fromPath, UA_OPENFILEMODE_READ, &src);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = dstB->createFile(dstB, toPath);
    if(res != UA_STATUSCODE_GOOD) {
        srcB->closeFile(srcB, src);
        return res;
    }
    void *dst = NULL;
    res = dstB->openFile(dstB, toPath, UA_OPENFILEMODE_WRITE, &dst);
    if(res != UA_STATUSCODE_GOOD) {
        /* Remove the empty file just created so a failed open does not leave
         * a stale entry behind in the destination backend. */
        dstB->remove(dstB, toPath);
        srcB->closeFile(srcB, src);
        return res;
    }

    while(res == UA_STATUSCODE_GOOD) {
        UA_ByteString chunk = UA_BYTESTRING_NULL;
        res = srcB->read(srcB, src, UA_FILETRANSFER_COPYCHUNKSIZE, &chunk);
        if(res != UA_STATUSCODE_GOOD || chunk.length == 0) {
            UA_ByteString_clear(&chunk);
            break;
        }
        res = dstB->write(dstB, dst, chunk);
        UA_ByteString_clear(&chunk);
    }

    srcB->closeFile(srcB, src);
    dstB->closeFile(dstB, dst);
    /* Remove a partially written destination so a failed copy does not leave
     * a truncated file behind. An EOF read leaves res Good, so the complete
     * destination is kept. */
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
    res = srcB->listDirectory(srcB, fromPath, scanCollector, &entries);
    if(res != UA_STATUSCODE_GOOD)
        return res;
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
    UA_StatusCode res = b->listDirectory(b, dirNode->path, scanCollector,
                                         &entries);
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

    /* Create nodes for new backend entries */
    for(ScanEntry *e = entries; e && res == UA_STATUSCODE_GOOD; e = e->next) {
        if(e->matched || !validEntryName(e->name))
            continue;
        if(*nodeBudget == 0)
            break;
        if(e->isDir) {
            FTNode *newDir = NULL;
            res = mirrorDirectory(server, ftd, dirNode, e->name, &newDir);
            if(res == UA_STATUSCODE_GOOD) {
                (*nodeBudget)--;
                res = fileTransferMirrorTree(server, ftd, newDir, depth + 1, nodeBudget);
            }
        } else {
            UA_String childPath = UA_STRING_NULL;
            res = joinPath(dirPath, e->name, &childPath);
            if(res != UA_STATUSCODE_GOOD)
                break;
            UA_FileTransferFileInfo info;
            res = b->getAttributes(b, childPath, &info);
            UA_String_clear(&childPath);
            if(res != UA_STATUSCODE_GOOD)
                break;
            res = mirrorFile(server, ftd, dirNode, e->name, &info, NULL);
            if(res == UA_STATUSCODE_GOOD)
                (*nodeBudget)--;
        }
    }
    freeScanEntries(entries);
    if(res != UA_STATUSCODE_GOOD) {
        UA_String_clear(&dirPath);
        return res;
    }

    /* Recurse into the (kept) subdirectories */
    LIST_FOREACH_SAFE(child, &ftd->nodes, listEntry, childTmp) {
        if(child->mount != dirMount || !child->isDirectory ||
           child->zombie || !isDirectChildPath(dirPath, child->path))
            continue;
        res = fileTransferSyncTree(server, ftd, child, depth + 1, nodeBudget);
        if(res != UA_STATUSCODE_GOOD)
            break;
    }
    UA_String_clear(&dirPath);
    return res;
}

/**************************************
 * FileDirectoryType Method Callbacks
 **************************************/

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

    if(inputSize < 1 || !UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_STRING]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_String name = *(UA_String*)input[0].data;
    if(!validEntryName(name))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_String newPath = UA_STRING_NULL;
    res = joinPath(dirNode->path, name, &newPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(findChildByPath(ftd, dirNode->mount, newPath)) {
        UA_String_clear(&newPath);
        return UA_STATUSCODE_BADBROWSENAMEDUPLICATED;
    }

    UA_FileTransferBackend *b = &dirNode->mount->backend;
    res = b->createDirectory(b, newPath);
    UA_String_clear(&newPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    FTNode *newNode = NULL;
    res = mirrorDirectory(server, ftd, dirNode, name, &newNode);
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

    if(inputSize < 2 ||
       !UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_STRING]) ||
       !UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_BOOLEAN]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_String name = *(UA_String*)input[0].data;
    UA_Boolean requestFileOpen = *(UA_Boolean*)input[1].data;
    if(!validEntryName(name))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_String newPath = UA_STRING_NULL;
    res = joinPath(dirNode->path, name, &newPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(findChildByPath(ftd, dirNode->mount, newPath)) {
        UA_String_clear(&newPath);
        return UA_STATUSCODE_BADBROWSENAMEDUPLICATED;
    }

    UA_FileTransferBackend *b = &dirNode->mount->backend;
    res = b->createFile(b, newPath);
    if(res != UA_STATUSCODE_GOOD) {
        UA_String_clear(&newPath);
        return res;
    }

    UA_FileTransferFileInfo info;
    res = b->getAttributes(b, newPath, &info);
    UA_String_clear(&newPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    FTNode *newNode = NULL;
    res = mirrorFile(server, ftd, dirNode, name, &info, &newNode);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Optionally open the new file for reading and writing */
    UA_UInt32 handle = 0;
    if(requestFileOpen) {
        res = openFileHandle(server, ftd, newNode, sessionId,
                             UA_OPENFILEMODE_READ | UA_OPENFILEMODE_WRITE,
                             &handle);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }

    res = UA_Variant_setScalarCopy(&output[0], &newNode->nodeId,
                                   &UA_TYPES[UA_TYPES_NODEID]);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_Variant_setScalarCopy(&output[1], &handle,
                                       &UA_TYPES[UA_TYPES_UINT32]);
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

    UA_FileTransferBackend *b = &dirNode->mount->backend;
    res = deleteBackendTree(b, target->path, target->isDirectory);
    if(res != UA_STATUSCODE_GOOD)
        return res;

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

    if(inputSize < 4 ||
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

    UA_Boolean isDir = source->isDirectory;
    UA_FileTransferBackend *srcB = &source->mount->backend;
    UA_FileTransferBackend *dstB = &targetDir->mount->backend;
    if(!createCopy && sameMount) {
        /* Within one backend a move is an atomic rename */
        res = dstB->rename(dstB, source->path, destPath);
        if(res == UA_STATUSCODE_GOOD)
            removeSubtree(server, ftd, source); /* Invalidates source */
    } else if(!createCopy) {
        /* Cross-mount move: copy to the target backend, then delete from the
         * source backend. This is not atomic. If the source delete fails, the
         * target copy is rolled back on a best-effort basis. */
        res = copyBackendTree(srcB, source->path, dstB, destPath, isDir);
        if(res == UA_STATUSCODE_GOOD) {
            UA_StatusCode delRes = deleteBackendTree(srcB, source->path, isDir);
            if(delRes == UA_STATUSCODE_GOOD) {
                removeSubtree(server, ftd, source); /* Invalidates source */
            } else {
                deleteBackendTree(dstB, destPath, isDir); /* Roll back */
                res = delRes;
            }
        }
    } else {
        /* Copy (same or cross mount) */
        res = copyBackendTree(srcB, source->path, dstB, destPath, isDir);
    }

    /* Mirror the entry at the new location */
    FTNode *newNode = NULL;
    if(res == UA_STATUSCODE_GOOD) {
        if(isDir) {
            res = mirrorDirectory(server, ftd, targetDir, name, &newNode);
            if(res == UA_STATUSCODE_GOOD) {
                UA_UInt32 nodeBudget = (UA_UInt32)0xffffffffu;
                const UA_FileTransferMountOptions *opts = &targetDir->mount->options;
                if(opts->maxNodes > 0) {
                    UA_UInt32 current = countMountNodes(ftd, targetDir->mount);
                    nodeBudget = (opts->maxNodes > current) ?
                        opts->maxNodes - current : 0;
                }
                res = fileTransferMirrorTree(server, ftd, newNode, pathDepth(destPath) + 1,
                                 &nodeBudget);
            }
        } else {
            UA_FileTransferFileInfo info;
            res = dstB->getAttributes(dstB, destPath, &info);
            if(res == UA_STATUSCODE_GOOD)
                res = mirrorFile(server, ftd, targetDir, name, &info, &newNode);
        }
    }
    UA_String_clear(&name);
    UA_String_clear(&destPath);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    return UA_Variant_setScalarCopy(&output[0], &newNode->nodeId,
                                    &UA_TYPES[UA_TYPES_NODEID]);
}

#endif /* UA_ENABLE_METHODCALLS && UA_GENERATED_NAMESPACE_ZERO_FULL */