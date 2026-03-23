/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 * Tutorial: OPC UA FileSystem Driver Overview
 *
 * This tutorial demonstrates the complete FileSystem driver setup:
 * 1. Creating a FileServerDriver
 * 2. Mounting a host filesystem directory into the OPC UA address space
 * 3. Attaching a filesystem to a custom object (e.g., a Pump)
 * 4. Using the public API (makeFile, makeDirectory) to create content
 * 5. Running the server so clients can interact with files
 *
 * Public Server API (for server-side code):
 *   - UA_Server_makeDirectory()   : Create a directory (on disk + address space)
 *   - UA_Server_makeFile()        : Create a file (on disk + address space)
 *   - UA_FileServerDriver_addFileDirectory() : Mount a filesystem directory (driver-level)
 *
 * Client API (via OPC UA method calls on FileType/FileDirectoryType nodes):
 *   - CreateDirectory, CreateFile, DeleteFileSystemObject, MoveOrCopy
 *   - Open, Close, Read, Write, GetPosition, SetPosition
 *
 * See also:
 *   - tutorial_server_filesystem_directory.c  (Directory operations)
 *   - tutorial_server_filesystem_file.c       (File operations)
 */

#include <open62541/server_config_default.h>
#include <open62541/server.h>
#include <filesystem/ua_fileserver_driver.h>
#include <open62541/plugin/log_stdout.h>

#if defined(UA_FILESYSTEM)

/* Helper: Create a "Pump" object and attach a FileSystem to it.
 * This shows how any OPC UA object can have an associated filesystem. */
static void
manuallyDefinePump(UA_Server *server, UA_FileServerDriver *driver) {
    UA_NodeId pumpId;
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Pump (Manual)");

    UA_Server_addObjectNode(server, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                            UA_NS0ID(ORGANIZES), UA_QUALIFIEDNAME(1, "Pump (Manual)"),
                            UA_NS0ID(BASEOBJECTTYPE),
                            oAttr, NULL, &pumpId);

    UA_VariableAttributes mnAttr = UA_VariableAttributes_default;
    UA_String manufacturerName = UA_STRING("Pump King Ltd.");
    UA_Variant_setScalar(&mnAttr.value, &manufacturerName, &UA_TYPES[UA_TYPES_STRING]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "ManufacturerName");
    UA_Server_addVariableNode(server, UA_NODEID_NULL, pumpId, UA_NS0ID(HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "ManufacturerName"),
                              UA_NS0ID(BASEDATAVARIABLETYPE), mnAttr, NULL, NULL);

    UA_VariableAttributes statusAttr = UA_VariableAttributes_default;
    UA_Boolean status = true;
    UA_Variant_setScalar(&statusAttr.value, &status, &UA_TYPES[UA_TYPES_BOOLEAN]);
    statusAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Status");
    UA_Server_addVariableNode(server, UA_NODEID_NULL, pumpId, UA_NS0ID(HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Status"), UA_NS0ID(BASEDATAVARIABLETYPE),
                              statusAttr, NULL, NULL);

    /* Attach a FileSystem to the Pump.
     * Clients can now browse to Pump -> FileSystem and access files. */
    UA_NodeId filesystemId;
    UA_FileServerDriver_addFileDirectory(driver, server, &pumpId,
                                      ".", &filesystemId, ".");

    /* Use the public API to create directories and files under the pump’s filesystem */
    UA_NodeId logDirId;
    UA_Server_makeDirectory(server, NULL, NULL, &filesystemId, "PumpLogs", &logDirId);

    UA_Int32 handle;
    UA_Server_makeFile(server, NULL, NULL, &logDirId, "startup.log", false, &handle);
}

int main(void) {
    /* Step 1: Create server */
    UA_Server *server = UA_Server_new();

    /* Step 2: Create FileServerDriver (local filesystem type) */
    UA_FileServerDriver *fsDriver =
        UA_FileServerDriver_new("MainFileServer", server, FILE_DRIVER_TYPE_LOCAL);

    /* Step 3: Initialize the driver */
    UA_DriverContext ctx;
    ctx.server = server;
    ctx.config = NULL;
    fsDriver->base.lifecycle.init(server, (UA_Driver*)fsDriver, &ctx);

    /* Step 4: Create a Pump object with attached FileSystem */
    manuallyDefinePump(server, fsDriver);

    /* Step 5: Mount a standalone filesystem under Objects folder
     * UA_FileServerDriver_addFileDirectory() is the driver-level function to
     * mount a host directory into the OPC UA address space. */
    UA_NodeId objectsFolder = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId fsNodeId;
    UA_FileServerDriver_addFileDirectory(fsDriver, server, &objectsFolder, ".", &fsNodeId, ".");

    /* Step 6: Start driver and run server */
    fsDriver->base.lifecycle.start((UA_Driver*)fsDriver);
    fsDriver->base.lifecycle.updateModel(server, (UA_Driver*)fsDriver);

    UA_StatusCode retval = UA_Server_runUntilInterrupt(server);

    /* Cleanup */
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
