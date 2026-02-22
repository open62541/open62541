#include <filesystem/ua_fileserver_driver.h>

#if defined(UA_FILESYSTEM)

UA_StatusCode
UA_Server_addFileSystem(UA_FileServerDriver *driver, UA_Server *server,
                      const UA_NodeId parentNode,
                      const char *mountPath) {
    // Adding filesystem to existing driver
    if (driver) {
        UA_NodeId newNodeId;
        UA_StatusCode res = UA_FileServerDriver_addFileDirectory(driver, server, &parentNode, mountPath, &newNodeId, mountPath);
        return res;
    }
    return UA_STATUSCODE_BADINVALIDARGUMENT;
}

UA_StatusCode
UA_Server_openFile(UA_Server *server, UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *fileNodeId, UA_Byte openMode, UA_Int32 **handle) {
    return __fileTypeOperation(server, sessionId, sessionContext, NULL, NULL, fileNodeId, NULL, sizeof(UA_Byte), (const UA_Variant*)&openMode, sizeof(UA_Int32*), (UA_Variant*)handle, FILE_OP_OPEN);
}

UA_StatusCode
UA_Server_closeFile(UA_Server *server, UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *fileNodeId, UA_Int32 *handle) {
    return __fileTypeOperation(server, sessionId, sessionContext, NULL, NULL, fileNodeId, NULL, sizeof(UA_Int32*), (const UA_Variant*)&handle, 0, NULL, FILE_OP_CLOSE);
}

UA_StatusCode
UA_Server_readFile(UA_Server *server, UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *fileNodeId, UA_Int32 *handle, UA_Int32 length, UA_ByteString *data) {
    return __fileTypeOperation(server, sessionId, sessionContext, NULL, NULL, fileNodeId, NULL, sizeof(UA_Int32), (const UA_Variant*)&length, sizeof(UA_ByteString), (UA_Variant*)data, FILE_OP_READ);
}

UA_StatusCode
UA_Server_writeFile(UA_Server *server, UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *fileNodeId, UA_Int32 *handle, const UA_ByteString *data) {
    return __fileTypeOperation(server, sessionId, sessionContext, NULL, NULL, fileNodeId, NULL, sizeof(UA_ByteString), (const UA_Variant*)data, 0, NULL, FILE_OP_WRITE);
}

UA_StatusCode
UA_Server_setFilePosition(UA_Server *server, UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *fileNodeId, UA_Int32 *handle, UA_UInt64 position) {
    return __fileTypeOperation(server, sessionId, sessionContext, NULL, NULL, fileNodeId, NULL, sizeof(UA_UInt64), (const UA_Variant*)&position, 0, NULL, FILE_OP_SETPOSITION);
}

UA_StatusCode
UA_Server_getFilePosition(UA_Server *server, UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *fileNodeId, UA_Int32 *handle, UA_UInt64 *position) {
    return __fileTypeOperation(server, sessionId, sessionContext, NULL, NULL, fileNodeId, NULL, 0, NULL, sizeof(UA_UInt64), (UA_Variant*)position, FILE_OP_GETPOSITION);
}

UA_StatusCode
UA_Server_deleteDirOrFile(UA_Server *server, UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *nodeId) {
    return __directoryOperation(server, sessionId, sessionContext, NULL, NULL, nodeId, NULL, 0, NULL, 0, NULL, DIR_OP_DELETE);
}

UA_StatusCode
UA_Server_moveOrCopyItem(UA_Server *server, UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *sourceNodeId, const UA_NodeId *destinationNodeId, bool copy) {
    return __directoryOperation(server, sessionId, sessionContext, NULL, NULL, sourceNodeId, NULL, sizeof(UA_NodeId), (const UA_Variant*)destinationNodeId, 0, NULL, DIR_OP_MOVEORCOPY);
}

UA_StatusCode
UA_Server_makeDirectory(UA_Server *server, UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *parentNode, const char *dirName, UA_NodeId *newNodeId) {
    return __directoryOperation(server, sessionId, sessionContext, NULL, NULL, parentNode, NULL, sizeof(UA_String), (const UA_Variant*)&(UA_String){strlen(dirName), (const UA_Byte*)dirName}, sizeof(UA_NodeId), (UA_Variant*)newNodeId, DIR_OP_MKDIR);
}

UA_StatusCode
UA_Server_makeFile(UA_Server *server, UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *parentNode, const char *fileName, bool fileHandleBool, UA_Int32* output) {
    return __directoryOperation(server, sessionId, sessionContext, NULL, NULL, parentNode, NULL, sizeof(UA_String), (const UA_Variant*)&(UA_String){strlen(fileName), (const UA_Byte*)fileName}, sizeof(UA_Int32), (UA_Variant*)output, DIR_OP_MKFILE);
}

#endif /* UA_FILESYSTEM */
