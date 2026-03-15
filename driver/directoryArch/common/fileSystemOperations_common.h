#ifndef UA_FILESYSTEMOPERATIONS_COMMON_H_
#define UA_FILESYSTEMOPERATIONS_COMMON_H_

#if defined(UA_FILESYSTEM)

#include <open62541/types.h>
#include <open62541/server.h>
#include <open62541/plugin/log_stdout.h>
#include <driver.h>
#include <filesystem/ua_fileserver_driver.h>
#include <filesystem/ua_filetypes.h>
#include <stdio.h>

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

inline static UA_StatusCode
getFullPath(UA_Server *server,
            const UA_NodeId *startNode,
            char *buffer,
            size_t bufferSize)
{
    buffer[0] = '\0';

    UA_NodeId current = *startNode;

    while(true) {
        /* Get the context of the current node */
        FileDirectoryContext *ctx = NULL;
        UA_Server_getNodeContext(server, current, (void**)&ctx);

        if(!ctx || !ctx->path) {
            /* No context means we reached the root */
            break;
        }

        /* Prepend this path segment */
        char temp[2048];
        snprintf(temp, sizeof(temp), "%s/%s", ctx->path, buffer);
        strncpy(buffer, temp, bufferSize - 1);
        buffer[bufferSize - 1] = '\0';

        /* Browse inverse to find the parent */
        UA_BrowseResult br = UA_Server_browse(server, 10,
            &(UA_BrowseDescription){
                .nodeId = current,
                .browseDirection = UA_BROWSEDIRECTION_INVERSE,
                .includeSubtypes = true,
                .referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                .nodeClassMask = UA_NODECLASS_OBJECT
            });

        if(br.referencesSize == 0) {
            UA_BrowseResult_clear(&br);
            break;
        }

        /* Move to parent */
        current = br.references[0].nodeId.nodeId;

        UA_BrowseResult_clear(&br);
    }

    return UA_STATUSCODE_GOOD;
}

bool 
isDirectory(const char *path);

/* Directory operations */
UA_StatusCode
makeDirectory(const char *path);

UA_StatusCode
makeFile(const char *path, bool fileHandleBool, UA_Int32* output);

UA_StatusCode
deleteDirOrFile(const char *path);

UA_StatusCode
moveOrCopyItem(const char *sourcePath, const char *destinationPath, bool copy);

UA_StatusCode
directoryExists(const char *path, UA_Boolean *exists);

/* File operations - for FileType methods */
UA_StatusCode openFile(const char *path, UA_Byte openMode, UA_Int32 **handle);
UA_StatusCode closeFile(UA_Int32 *handle);
UA_StatusCode readFile(UA_Int32 *handle, UA_Int32 length, UA_ByteString *data);
UA_StatusCode writeFile(UA_Int32 *handle, const UA_ByteString *data);
UA_StatusCode seekFile(UA_Int32 *handle, UA_UInt64 position);
UA_StatusCode getFilePosition(UA_Int32 *handle, UA_UInt64 *position);
UA_StatusCode getFileSize(const char *path, UA_UInt64 *size);

typedef UA_StatusCode (*AddDirType)(UA_Driver *, UA_Server *, const UA_NodeId *, const char *, UA_NodeId *, const char *);
typedef UA_StatusCode (*AddFileType)(UA_Server *, const UA_NodeId *, const char *, UA_NodeId *);

UA_StatusCode scanDirectoryRecursive(
    UA_Server *server, 
    const UA_NodeId *parentNode, 
    const char *path, 
    AddDirType addDirFunc, 
    AddFileType addFileFunc);

inline static UA_StatusCode
fillLocalFileDriverContext(UA_FileDriverContext *ctx, FileDriverType driverType) {
    switch (driverType) {
        case FILE_DRIVER_TYPE_LOCAL:
            ctx->openFile = &openFile;
            ctx->closeFile = &closeFile;
            ctx->readFile = &readFile;
            ctx->writeFile = &writeFile;
            ctx->seekFile = &seekFile;
            ctx->getFilePosition = &getFilePosition;
            ctx->getFileSize = &getFileSize;
            ctx->deleteDirOrFile = &deleteDirOrFile;
            ctx->moveOrCopyItem = &moveOrCopyItem;
            ctx->makeDirectory = &makeDirectory;
            ctx->makeFile = &makeFile;
            ctx->isDirectory = &isDirectory;
            break;
        default:
            return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

#endif /* UA_FILESYSTEMOPERATIONS_COMMON_H_ */
#endif /* UA_FILESYSTEM */
