#include <filesystem/ua_fileserver_driver.h>

#if defined(UA_FILESYSTEM)

UA_StatusCode
UA_Server_addFileSystem(UA_FileServerDriver *driver, UA_Server *server,
                      const UA_NodeId parentNode,
                      const char *mountPath) {
    if (driver) {
        UA_NodeId newNodeId;
        UA_StatusCode res = UA_FileServerDriver_addFileDirectory(driver, server, &parentNode, mountPath, &newNodeId, mountPath);
        return res;
    }
    return UA_STATUSCODE_BADINVALIDARGUMENT;
}

UA_StatusCode
UA_Server_makeDirectory(UA_Server *server, UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *parentNode, const char *dirName, UA_NodeId *newNodeId) {
    return __directoryOperation(server, sessionId, sessionContext, NULL, NULL, parentNode, NULL, sizeof(UA_String), (const UA_Variant*)&(UA_String){strlen(dirName), (UA_Byte*)dirName}, sizeof(UA_NodeId), (UA_Variant*)newNodeId, DIR_OP_MKDIR);
}

UA_StatusCode
UA_Server_makeFile(UA_Server *server, UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *parentNode, const char *fileName, bool fileHandleBool, UA_Int32* output) {
    return __directoryOperation(server, sessionId, sessionContext, NULL, NULL, parentNode, NULL, sizeof(UA_String), (const UA_Variant*)&(UA_String){strlen(fileName), (UA_Byte*)fileName}, sizeof(UA_Int32), (UA_Variant*)output, DIR_OP_MKFILE);
}

#endif /* UA_FILESYSTEM */
