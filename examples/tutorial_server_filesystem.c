#include <open62541/server.h>
#include <open62541/driver.h>
#include <src/driver/ua_driver_filesystem.c>

int main(void) {
    UA_Server *server = UA_Server_new();

    UA_FsAccessLayer fsal;
    fsal.open = NULL;
    fsal.close = NULL;
    fsal.read = NULL;
    fsal.write = NULL;
    fsal.list = NULL;

    UA_Driver *fsDriver = UA_FileSystemDriver_new(&fsal, NULL);

    UA_DriverConfig cfg;
    cfg.name = UA_STRING_ALLOC("DemoFileSystemDriver");
    cfg.userData = fsDriver->userData;
    cfg.userDataCleanup = NULL;

    UA_DriverManager_registerDriver(server, fsDriver, &cfg);

    UA_DriverManager_startDriver(server, "DemoFileSystemDriver");

    UA_NodeId fsNode;
    UA_NodeId objectsFolder = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_FileSystemDriver_mount(server, fsDriver, &objectsFolder, "RootFS", "/home/user", &fsNode);

    UA_Server_runUntilInterrupt(server);
    UA_Server_delete(server);
    return 0;
}