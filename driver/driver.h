#ifndef UA_DRIVER_H
#define UA_DRIVER_H

#include <open62541/server.h>
#include <open62541/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Driver lifecycle states
 *
 * This enumeration defines the possible states of a driver during its
 * lifecycle. It is used to track whether a driver is stopped, initialized,
 * running, or in a special watching state (e.g., monitoring for changes).
 */
enum DriverState {
    UA_DRIVER_STATE_UNINITIALIZED = 0,
    UA_DRIVER_STATE_STOPPED = 1,
    UA_DRIVER_STATE_RUNNING = 2,
    UA_DRIVER_STATE_WATCHING = 3
};
/* Forward declarations
 *
 * These are lightweight type declarations for the driver structures.
 * They allow us to reference UA_Driver and UA_DriverContext before
 * their full definitions appear later in the file.
 */
typedef struct UA_Driver UA_Driver;
typedef struct UA_DriverContext UA_DriverContext;

/* Lifecycle interface for drivers
 *
 * Each driver implements a set of lifecycle functions that define
 * how it is initialized, started, stopped, and synchronized with
 * the OPC UA information model.
 *
 * - init:        Called once when the driver is created. Used to set up
 *                internal data structures and prepare for operation.
 * - start:       Called when the driver is activated. This is where
 *                resources such as threads, file handles, or connections
 *                can be opened.
 * - stop:        Called when the driver is deactivated. Responsible for
 *                cleaning up resources and resetting state.
 * - updateModel: Called to synchronize the driver’s internal state with
 *                the OPC UA server’s address space. For example, updating
 *                properties or attaching methods to nodes.
 */
typedef struct {
    UA_StatusCode (*init)(UA_Server *server, UA_Driver *driver, UA_DriverContext *ctx);
    UA_StatusCode (*cleanup)(UA_Server *server, UA_Driver *driver);
    UA_StatusCode (*start)(UA_Driver *driver);
    UA_StatusCode (*stop)(UA_Driver *driver);
    void (*updateModel)(UA_Server *server, UA_Driver *DriverCallback);
} UA_DriverLifecycle;

/* Generic driver structure
 *
 * This is the base type for all drivers. It contains:
 * - name:       A human-readable identifier for the driver.
 * - userData:   A pointer to driver-specific data. This allows each
 *               driver to store its own context without modifying
 *               the generic interface.
 * - state:      The current lifecycle state of the driver.
 * - lifecycle:  A set of function pointers implementing the driver’s
 *               lifecycle behavior (init, start, stop, updateModel).
 *
 * Specific drivers (e.g., FileServerDriver) extend this structure
 * by embedding UA_Driver as their first member, ensuring compatibility
 * with the generic driver manager.
 */
struct UA_Driver{
    UA_NodeId nodeId;           /* Entry point in the OPC UA information model */
    UA_UInt64 driverWatcherId;
    const char *name;
    void *userData;                  /* Driver-specific context or private data */
    enum DriverState state;
    UA_DriverContext *context;    /* Access to server and configuration */
    UA_DriverLifecycle lifecycle;    /* Function pointers for lifecycle management */
};

/* Context structure for driver management
 *
 * This structure provides drivers with access to the OPC UA server
 * and configuration data. It is passed into lifecycle functions so
 * that drivers can interact with the server and read their settings.
 *
 * - server: Pointer to the UA_Server instance. Allows drivers to add
 *           nodes, methods, and properties to the information model.
 * - config: Pointer to driver-specific configuration data. This can
 *           be used to store initialization parameters or runtime options.
 */
struct UA_DriverContext {
    UA_Server *server;               /* Access to the UA server instance */
    void *config;                    /* Driver configuration data */
};

/* DriverManager API
 *
 * These functions provide a generic interface for registering and
 * managing drivers. They allow the application to dynamically add,
 * remove, start, and stop drivers without hardcoding their lifecycle.
 *
 * - register:   Registers a new driver with the manager.
 * - unregister: Removes a driver by name.
 * - start:      Activates a driver by name, calling its start function.
 * - stop:       Deactivates a driver by name, calling its stop function.
 *
 * This API makes it possible to manage multiple drivers in a modular
 * way, supporting extensibility and clean separation of concerns.
 */
UA_StatusCode UA_DriverManager_register(UA_Driver *driver);
UA_StatusCode UA_DriverManager_unregister(const char *driverName);
UA_StatusCode UA_DriverManager_start(const char *driverName);
UA_StatusCode UA_DriverManager_stop(const char *driverName);

#ifdef __cplusplus
}
#endif

#endif /* UA_DRIVER_H */