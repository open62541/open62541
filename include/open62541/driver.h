#ifndef DRIVER_H
#define DRIVER_H

#include <open62541/types.h>
#include <open62541/server.h>

typedef struct UA_Driver UA_Driver;

typedef enum {
    UA_DRIVER_STATE_UNINITIALIZED   = 0,
    UA_DRIVER_STATE_INITIALIZED     = 1,
    UA_DRIVER_STATE_RUNNING         = 2,
    UA_DRIVER_STATE_STOPPED         = 3,
    UA_DRIVER_STATE_ERROR           = 100
} UA_DriverState;

typedef void (*UA_DriverUserDataCleanup)(void *userData);

typedef struct UA_DriverConfig {
    UA_String name;
    void *userData;
    UA_DriverUserDataCleanup userDataCleanup;
} UA_DriverConfig;

typedef UA_StatusCode (*UA_DriverInitFunc)(UA_Driver *driver, const UA_DriverConfig *config);
typedef UA_StatusCode (*UA_DriverStartFunc)(UA_Driver *driver);
typedef UA_StatusCode (*UA_DriverStopFunc)(UA_Driver *driver);
typedef void          (*UA_DriverCleanupFunc)(UA_Driver *driver);

struct UA_Driver {
    UA_String *name;
    UA_DriverState state;
    UA_NodeId stateVariableNodeId;
    void *userData;

    // callbacks
    UA_DriverInitFunc init;
    UA_DriverStartFunc start;
    UA_DriverStopFunc stop;
    UA_DriverCleanupFunc cleanup;

    UA_NodeId driverNodeId; // Ein Knoten für den Treiber
};

UA_StatusCode UA_DriverManager_registerDriver(UA_Server *server, UA_Driver *driver, const UA_DriverConfig *config);
UA_StatusCode UA_DriverManager_startDriver(UA_Server *server, const char *driverName);
UA_StatusCode UA_DriverManager_stopDriver(UA_Server *server, const char *driverName);
UA_StatusCode UA_DriverManager_unregisterDriver(UA_Server *server, const char *driverName);

UA_StatusCode UA_Server_addFileNode(UA_Server *server, const UA_NodeId *parent, const char *browseName, const char *filesystemPath, UA_NodeId *outFileNodeId);
// TODO: Implement other server functions

#endif