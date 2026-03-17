#include <open62541/server_config_default.h>
#include <open62541/server.h>
#include <filesystem/ua_fileserver_driver.h>
#include <open62541/plugin/log_stdout.h>

#if defined(UA_FILESYSTEM)

UA_StatusCode manuallyDefineFileSystem(UA_Server *server, UA_FileServerDriver *driver, const UA_NodeId parentNode);

/*
 * Helper function: Manually define a FileSystem node in the OPC UA information model.
 * This function demonstrates how to create a custom FileSystem node and attach it to
 * a specified parent node in the server’s address space. The FileSystem node will be
 * associated with the provided FileServerDriver.
 */
UA_StatusCode
manuallyDefineFileSystem(UA_Server *server, UA_FileServerDriver *driver, const UA_NodeId parentNode) {
    /* Create a FileSystem node under the specified parent node */
    UA_NodeId fsNodeId;
    UA_StatusCode res = UA_FileServerDriver_addFileDirectory(driver, server, &parentNode,
                                                            ".", &fsNodeId, true);
    if (res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                     "Failed to add FileSystem: %s", UA_StatusCode_name(res));
        return res;
    }

    return UA_STATUSCODE_GOOD;
}

int main(void) {
    /* Create a new UA Server instance */
    UA_Server *server = UA_Server_new();

    /* Create a new FileServerDriver instance */
    UA_FileServerDriver *fsDriver = UA_FileServerDriver_new("MainFileServer", server, FILE_DRIVER_TYPE_LOCAL);

    /* Initialize the driver with a context that provides access to the server */
    UA_DriverContext ctx;
    ctx.server = server;
    ctx.config = NULL;
    fsDriver->base.lifecycle.init(server, (UA_Driver*)fsDriver, &ctx);

    /* Manually define a FileSystem node in the Objects folder */
    manuallyDefineFileSystem(server, fsDriver, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER));

    /* Create an example object under the Objects folder and add a FileSystem to it */
    UA_NodeId exampleNodeId = UA_NODEID_NULL;
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US", "Example Object");
    UA_Server_addObjectNode(server, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                            UA_NS0ID(ORGANIZES), UA_QUALIFIEDNAME(1, "Example Object"),
                            UA_NS0ID(BASEOBJECTTYPE),
                            oAttr, NULL, &exampleNodeId);

    UA_NodeId newFsNodeId1;
    UA_FileServerDriver_addFileDirectory(fsDriver, server, &exampleNodeId,
                                      ".", &newFsNodeId1, true);

    fsDriver->base.lifecycle.start((UA_Driver*)fsDriver);

    fsDriver->base.lifecycle.updateModel(server, (UA_Driver*)fsDriver);

    UA_StatusCode retval = UA_Server_runUntilInterrupt(server);

    // cleanup
    fsDriver->base.lifecycle.cleanup(server, (UA_Driver*)fsDriver);
    UA_Server_delete(server);
    free(fsDriver);

    return (int)retval;
}
#else
int main(void) {
    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                 "Filesystem support is disabled. Please enable UA_ENABLE_FILESYSTEM in CMake configuration to run this example.");
    return 1;
}
#endif // UA_FILESYSTEM
