/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#ifndef UA_SERVER_DRIVER_H_
#define UA_SERVER_DRIVER_H_

#include <open62541/util.h>
#include <open62541/common.h>

_UA_BEGIN_DECLS

struct UA_Driver;
typedef struct UA_Driver UA_Driver;

/**
 * Server Drivers
 * ==============
 * Drivers are different from other "plugins" in that they have an explicit
 * stateful lifecycle and can be started/stopped at runtime. Their lifecycle is
 * however dependent on the server into which the drivers are embedded. When the
 * server shuts down, the drivers are also stopped.
 *
 * Drivers can use the server's public API to the full extend. For example
 * add/remove nodes, or register connections and timers in the server's
 * EventLoop.
 *
 * Some drivers define core functionality and are added internally in the server
 * implementation. */

/* Type-tag for some well-known extensions of the general Driver
 * structure. See below. */
typedef enum {
    UA_DRIVERTYPE_GENERIC = 0
} UA_DriverType;

struct UA_Driver {
    UA_Driver *next; /* linked-list */
    UA_DriverType driverType;

    UA_String name;
    UA_LifecycleState state;

    /* Backpointer to the server. Must be set before _start is called. If NULL
     * this is set by the server during registering. Generally the server must
     * not be switched out once the driver has been started. */
    UA_Server *server;

    /* Start the Driver. It will typically register timers/connections in the
     * EventLoop and may add nodes in the server's information model. Starting
     * can fail if the server is not already started also.
     *
     * During startup, the server calls start on all registered drivers. */
    UA_StatusCode (*start)(UA_Driver *sc);

    /* Stopping is asynchronous and might need a few iterations of the eventloop
     * to succeed. All Drivers are stopped during the shutdown of the server.
     * Once fully stopped, the Driver must no longer rely on the server
     * backpointer. So it can be detached and _free'd at runtime of the
     * server. */
    void (*stop)(UA_Driver *sc);

    /* Clean up and delete the Driver. Can fail if it is not fully stopped. When
     * successfully removed, the Driver must no longer be accessed from the
     * server.
     *
     * Drivers are all free'd when the server is deleted. If a Driver is
     * manually removed before, then it needs to be unlinked from the server's
     * internal linked-list before. */
    UA_StatusCode (*free)(UA_Driver *sc);
};

/* Adds the Driver to the server. Starts the driver if the server is already
 * started. */
UA_StatusCode
UA_Server_addDriver(UA_Server *server, UA_Driver *drv);

/* Remove the Driver from the server. This will fail if the driver is not fully
 * stopped. */
UA_StatusCode
UA_Server_removeDriver(UA_Server *server, UA_Driver *drv);

_UA_END_DECLS

#endif /* UA_SERVERCOMPONENT_H_ */
