#include <open62541/server_config_default.h>
#include <open62541/server.h>
#include <filesystem/ua_fileserver_driver.h>
#include <open62541/plugin/log_stdout.h>

/* Helper function: Manually define a "Pump" object in the OPC UA information model.
 *
 * This function demonstrates how to create a custom object node (representing a pump)
 * and attach several variables to it (manufacturer name, model name, status, RPM).
 * It also shows how to attach a FileSystem node to the pump, so that logs or
 * configuration files can be accessed via OPC UA FileType methods.
 */

#if defined(UA_FILESYSTEM)

static void
manuallyDefinePump(UA_Server *server, UA_FileServerDriver *driver) {
    UA_NodeId pumpId; /* NodeId assigned by the server for the new pump object */
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Pump (Manual)");

    /* Create the Pump object under the Objects folder */
    UA_Server_addObjectNode(server, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                            UA_NS0ID(ORGANIZES), UA_QUALIFIEDNAME(1, "Pump (Manual)"),
                            UA_NS0ID(BASEOBJECTTYPE),
                            oAttr, NULL, &pumpId);

    /* Add a ManufacturerName variable to the Pump object */
    UA_VariableAttributes mnAttr = UA_VariableAttributes_default;
    UA_String manufacturerName = UA_STRING("Pump King Ltd.");
    UA_Variant_setScalar(&mnAttr.value, &manufacturerName, &UA_TYPES[UA_TYPES_STRING]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "ManufacturerName");
    UA_Server_addVariableNode(server, UA_NODEID_NULL, pumpId, UA_NS0ID(HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "ManufacturerName"),
                              UA_NS0ID(BASEDATAVARIABLETYPE), mnAttr, NULL, NULL);

    /* Add a ModelName variable to the Pump object */
    UA_VariableAttributes modelAttr = UA_VariableAttributes_default;
    UA_String modelName = UA_STRING("Mega Pump 3000");
    UA_Variant_setScalar(&modelAttr.value, &modelName, &UA_TYPES[UA_TYPES_STRING]);
    modelAttr.displayName = UA_LOCALIZEDTEXT("en-US", "ModelName");
    UA_Server_addVariableNode(server, UA_NODEID_NULL, pumpId, UA_NS0ID(HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "ModelName"),
                              UA_NS0ID(BASEDATAVARIABLETYPE), modelAttr, NULL, NULL);

    /* Add a Status variable (boolean) to indicate if the pump is running */
    UA_VariableAttributes statusAttr = UA_VariableAttributes_default;
    UA_Boolean status = true;
    UA_Variant_setScalar(&statusAttr.value, &status, &UA_TYPES[UA_TYPES_BOOLEAN]);
    statusAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Status");
    UA_Server_addVariableNode(server, UA_NODEID_NULL, pumpId, UA_NS0ID(HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Status"), UA_NS0ID(BASEDATAVARIABLETYPE),
                              statusAttr, NULL, NULL);

    /* Add a MotorRPM variable (double) to represent the pump’s motor speed */
    UA_VariableAttributes rpmAttr = UA_VariableAttributes_default;
    UA_Double rpm = 50.0;
    UA_Variant_setScalar(&rpmAttr.value, &rpm, &UA_TYPES[UA_TYPES_DOUBLE]);
    rpmAttr.displayName = UA_LOCALIZEDTEXT("en-US", "MotorRPM");
    UA_Server_addVariableNode(server, UA_NODEID_NULL, pumpId, UA_NS0ID(HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "MotorRPMs"),
                              UA_NS0ID(BASEDATAVARIABLETYPE),
                              rpmAttr, NULL, NULL);

    /* Attach a FileSystem node to the Pump object.
     * This allows clients to access files (e.g., logs) associated with the pump.
     */
    UA_NodeId filesystemId; /* NodeId assigned by the server for the FileSystem */
    UA_FileServerDriver_addFileDirectory(driver, server, &pumpId,
                                      ".", &filesystemId, ".");
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

    /* Manually define a Pump object with variables and a FileSystem */
    manuallyDefinePump(server, fsDriver);

    /* Mount two additional FileSystems directly under the Objects folder */
    UA_NodeId nodeId1 = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId newFsNodeId1;
    UA_FileServerDriver_addFileDirectory(fsDriver, server, &nodeId1,
                                      "D:/UnifiedAutomation", &newFsNodeId1, "D:/UnifiedAutomation");

    /* Start the driver (could open resources or spawn threads here) */
    fsDriver->base.lifecycle.start((UA_Driver*)fsDriver);

    /* Update the information model (attach methods, update properties, etc.) */
    fsDriver->base.lifecycle.updateModel(server, (UA_Driver*)fsDriver);

    /* Run the server until interrupted (e.g., Ctrl+C) */
    UA_StatusCode retval = UA_Server_runUntilInterrupt(server);

    /* Stop the driver and clean up resources */
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
