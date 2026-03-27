/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 * Tutorial: Working with Directories in the OPC UA FileSystem Driver
 *
 * This tutorial demonstrates how to:
 * 1. Create a FileServerDriver and mount a filesystem
 * 2. Use UA_Server_makeDirectory() to create directories via the server API
 * 3. Browse directory nodes in the OPC UA address space
 *
 * Directories are represented as FileDirectoryType nodes in OPC UA.
 * When a client connects, it can also call CreateDirectory/DeleteFileSystemObject
 * methods directly on these nodes.
 */

#include <open62541/server_config_default.h>
#include <open62541/server.h>
#include <open62541/plugin/log_stdout.h>
#include <filesystem/ua_fileserver_driver.h>

#if defined(UA_FILESYSTEM)

int main(void) {
    /* Step 1: Create the OPC UA server */
    UA_Server *server = UA_Server_new();

    /* Step 2: Create a FileServerDriver
     * The driver manages file system operations and maps them to OPC UA nodes.
     * FILE_DRIVER_TYPE_LOCAL means it uses the host's native file system. */
    UA_FileServerDriver *fsDriver =
        UA_FileServerDriver_new("DirectoryTutorialDriver", server, FILE_DRIVER_TYPE_LOCAL);
    if (!fsDriver) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Failed to create FileServerDriver");
        UA_Server_delete(server);
        return 1;
    }

    /* Step 3: Initialize the driver
     * The driver context links the driver to the server. */
    UA_DriverContext ctx;
    ctx.server = server;
    ctx.config = NULL;
    fsDriver->base.lifecycle.init(server, (UA_Driver*)fsDriver, &ctx);

    /* Step 4: Mount a filesystem directory
     * UA_FileServerDriver_addFileDirectory() creates a FileDirectoryType node
     * under the Objects folder that represents the given path on the host filesystem. */
    UA_NodeId objectsFolder = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId fsNodeId;
    UA_FileServerDriver_addFileDirectory(fsDriver, server, &objectsFolder, ".", &fsNodeId, true);

    /* Step 5: Create subdirectories using the public API
     * UA_Server_makeDirectory() creates a new directory both on disk
     * and as a FileDirectoryType node in the address space.
     *
     * Note: sessionId and sessionContext are NULL here because we're calling
     * from the server side. When an OPC UA client calls CreateDirectory,
     * the session context is provided automatically. */
    UA_NodeId fsRootId = fsDriver->fsNodes[0]; /* The mounted filesystem root node */
    UA_NodeId newDirId;

    UA_StatusCode res = UA_Server_makeDirectory(fsDriver, server, &fsRootId, "Logs", &newDirId);
    if (res == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Created 'Logs' directory successfully");
    }

    /* Create a nested directory inside Logs */
    UA_NodeId nestedDirId;
    res = UA_Server_makeDirectory(fsDriver, server, &newDirId, "2024", &nestedDirId);
    if (res == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Created 'Logs/2024' directory successfully");
    }

    /* Step 6: Start the driver and run the server
     * Once running, OPC UA clients can browse the directory structure
     * and call methods like CreateDirectory and DeleteFileSystemObject. */
    fsDriver->base.lifecycle.start((UA_Driver*)fsDriver);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Server running. Connect with an OPC UA client to browse directories.");
    UA_StatusCode retval = UA_Server_runUntilInterrupt(server);

    /* Cleanup */
    fsDriver->base.lifecycle.stop((UA_Driver*)fsDriver);
    fsDriver->base.lifecycle.cleanup(server, (UA_Driver*)fsDriver);
    UA_Server_delete(server);
    free(fsDriver);

    return (int)retval;
}

#else
int main(void) {
    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                 "Filesystem support is disabled. Enable UA_FILESYSTEM in CMake.");
    return 1;
}
#endif /* UA_FILESYSTEM */
