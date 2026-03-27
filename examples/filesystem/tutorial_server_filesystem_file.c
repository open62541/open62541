/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 * Tutorial: Working with Files in the OPC UA FileSystem Driver
 *
 * This tutorial demonstrates how to:
 * 1. Create files using UA_Server_makeFile()
 * 2. Understand how OPC UA clients interact with FileType nodes
 *    (Open, Read, Write, Close, GetPosition, SetPosition)
 *
 * Important: File operations like Open, Read, Write, Close are NOT part of
 * the public server API. They are accessed by OPC UA clients through method
 * calls on FileType nodes. The server only provides makeFile() and
 * makeDirectory() as its public API.
 *
 * Architecture:
 *   Server API:  UA_Server_makeFile()  -> creates file on disk + FileType node
 *   Client API:  UA_Server_call() with FileType methods (Open, Read, Write, Close)
 *                These are called automatically when a client invokes the methods.
 */

#include <open62541/server_config_default.h>
#include <open62541/server.h>
#include <open62541/plugin/log_stdout.h>
#include <filesystem/ua_fileserver_driver.h>

#if defined(UA_FILESYSTEM)

int main(void) {
    /* Create server and driver */
    UA_Server *server = UA_Server_new();

    UA_FileServerDriver *fsDriver =
        UA_FileServerDriver_new("FileTutorialDriver", server, FILE_DRIVER_TYPE_LOCAL);
    if (!fsDriver) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Failed to create FileServerDriver");
        UA_Server_delete(server);
        return 1;
    }

    /* Initialize the driver */
    UA_DriverContext ctx;
    ctx.server = server;
    ctx.config = NULL;
    fsDriver->base.lifecycle.init(server, (UA_Driver*)fsDriver, &ctx);

    /* Mount a filesystem using the driver-level function */
    UA_NodeId objectsFolder = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId fsNodeId;
    UA_FileServerDriver_addFileDirectory(fsDriver, server, &objectsFolder, ".", &fsNodeId, ".");

    UA_NodeId fsRootId = fsDriver->fsNodes[0];

    /* Create a directory first to organize our files */
    UA_NodeId dataDirId;
    UA_Server_makeDirectory(fsDriver, server, &fsRootId, "Data", &dataDirId);

    /* ----------------------------------------------------------------
     * Create files using UA_Server_makeFile()
     *
     * Parameters:
     *  - server:         The OPC UA server
     *  - sessionId:      NULL when calling from server side
     *  - sessionContext:  NULL when calling from server side
     *  - parentNode:     The directory node where the file should be created
     *  - fileName:       Name of the file to create
     *  - fileHandleBool: Whether to return a file handle (for immediate use)
     *  - output:         Output parameter for the file handle (if requested)
     * ---------------------------------------------------------------- */

    /* Create a simple data file */
    UA_NodeId fileNode1;
    UA_StatusCode res = UA_Server_makeFile(fsDriver, server,
                                            &dataDirId, "sensor_log.csv",
                                            &fileNode1);
    if (res == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Created 'Data/sensor_log.csv' successfully");
    }

    /* Create a config file */
    UA_NodeId fileNode2;
    res = UA_Server_makeFile(fsDriver, server,
                              &dataDirId, "config.json",
                              &fileNode2);
    if (res == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Created 'Data/config.json' successfully");
    }

    /* ----------------------------------------------------------------
     * How OPC UA Clients interact with files:
     *
     * Once the server is running, clients can browse to the FileType nodes
     * and call the following methods:
     *
     *   Open(Mode)           -> Returns a file handle
     *   Write(Data)          -> Writes a ByteString to the file
     *   Read(Length)          -> Reads Length bytes, returns ByteString
     *   SetPosition(Position) -> Seeks to a byte position
     *   GetPosition()         -> Returns current byte position
     *   Close(Handle)         -> Closes the file handle
     *
     * Open mode flags:
     *   0x01 = Read
     *   0x02 = Write
     *   0x03 = Read + Write
     *   0x04 = EraseExisting
     *   0x08 = Append
     *
     * Each client session gets its own file handle, so multiple clients
     * can work with the same file independently (multi-session support).
     * ---------------------------------------------------------------- */

    /* Start driver and run server */
    fsDriver->base.lifecycle.start((UA_Driver*)fsDriver);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Server running. Clients can now Open/Read/Write/Close files.");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Browse to the FileType nodes and call their methods.");
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
