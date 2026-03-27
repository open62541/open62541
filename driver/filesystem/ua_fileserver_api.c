#if defined(UA_FILESYSTEM)
#include <filesystem/ua_fileserver_driver.h>

UA_StatusCode
UA_Server_makeDirectory(UA_FileServerDriver *driver, UA_Server *server, const UA_NodeId *parentNode, const char *dirName, UA_NodeId *newNodeId) {

    UA_StatusCode ret = UA_FileServerDriver_addFileDirectory(
        driver, 
        server, 
        parentNode, 
        dirName, 
        newNodeId,
        true);

    return ret;
}

UA_StatusCode
UA_Server_makeFile(UA_FileServerDriver *driver, UA_Server *server, const UA_NodeId *parentNode, const char *fileName, UA_NodeId *newNodeId) {
    UA_StatusCode ret = UA_FileServerDriver_addFile(
        driver, 
        server, 
        parentNode, 
        fileName, 
        newNodeId);
    return ret;
}

#endif /* UA_FILESYSTEM */
