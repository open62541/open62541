#if defined(UA_FILESYSTEM)
#include <filesystem/ua_fileserver_driver.h>

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
    UA_String dirNameVariant = UA_STRING_ALLOC(dirName);

    UA_StatusCode ret = __directoryOperation(
        server, sessionId, sessionContext,
        NULL, NULL, parentNode, NULL,
        sizeof(UA_String), (const UA_Variant*)&dirNameVariant,
        sizeof(UA_NodeId), (UA_Variant*)newNodeId,
        DIR_OP_MKDIR
    );

    UA_String_clear(&dirNameVariant);
    return ret;
}

UA_StatusCode
UA_Server_makeFile(UA_Server *server, UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *parentNode, const char *fileName, bool fileHandleBool, UA_Int32* output) {
    UA_String fileNameVariant = UA_STRING_ALLOC(fileName);

    UA_StatusCode ret = __directoryOperation(
        server, sessionId, sessionContext,
        NULL, NULL, parentNode, NULL,
        sizeof(UA_String), (const UA_Variant*)&fileNameVariant,
        sizeof(UA_Int32), (UA_Variant*)output,
        DIR_OP_MKFILE
    );

    UA_String_clear(&fileNameVariant);
    return ret;
}

#endif /* UA_FILESYSTEM */
