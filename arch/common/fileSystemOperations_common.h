#ifndef UA_FILESYSTEMOPERATIONS_COMMON_H_
#define UA_FILESYSTEMOPERATIONS_COMMON_H_

#include <open62541/types.h>
#include <open62541/server.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/driver/ua_fileserver_driver.h>
#include <filesystem/ua_filetypes.h>
#include <stdio.h>

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

static UA_StatusCode
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
        strncpy(buffer, temp, bufferSize);

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
makeFile(const char *path);

UA_StatusCode
deleteDirOrFile(const char *path);

UA_StatusCode
moveOrCopyItem(const char *sourcePath, const char *destinationPath, bool copy);

UA_StatusCode
directoryExists(const char *path, UA_Boolean *exists);

/* File operations - for FileType methods */
UA_StatusCode openFile(const char *path, UA_Byte openMode, FILE **handle);
UA_StatusCode closeFile(FILE *handle);
UA_StatusCode readFile(FILE *handle, UA_Int32 length, UA_ByteString *data);
UA_StatusCode writeFile(FILE *handle, const UA_ByteString *data);
UA_StatusCode seekFile(FILE *handle, UA_UInt64 position);
UA_StatusCode getFilePosition(FILE *handle, UA_UInt64 *position);
UA_StatusCode getFileSize(const char *path, UA_UInt64 *size);

UA_StatusCode scanDirectoryRecursive(UA_Server *server, const UA_NodeId *parentNode, const char *path, void* addDirFunc, void* addFileFunc);

#endif /* UA_FILESYSTEMOPERATIONS_COMMON_H_ */