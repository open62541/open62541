/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#ifndef UA_SERVERCOMPONENT_H_
#define UA_SERVERCOMPONENT_H_

#include <open62541/util.h>
#include <open62541/common.h>

_UA_BEGIN_DECLS

struct UA_ServerComponent;
typedef struct UA_ServerComponent UA_ServerComponent;

/* ServerComponents are different from other "plugins" in that they have an
 * explicit stateful lifecycle and can be started/stopped at runtime. They
 * typically register connections and timers in the server's EventLoop and can
 * also register nodes in the server's information model.
 *
 * ServerComponents can be registered by the application using the public C API.
 * Some ServerComponents define core functionality and are added internally in
 * the server implementation. */

/* Type-tag for some well-known extensions of the general ServerComponent
 * structure. See below. */
typedef enum {
    UA_SERVERCOMPONENTTYPE_NORMAL = 0
} UA_ServerComponentType;

struct UA_ServerComponent {
    UA_ServerComponent *next; /* linked-list */
    UA_ServerComponentType serverComponentType;

    UA_String name;
    UA_LifecycleState state;

    /* Backpointer to the server. Needs to be set before the ServerComponent is
     * started. */
    UA_Server *server;

    /* Start the ServerComponent. It will typically register timers/connections
     * in the EventLoop and may add nodes in the server's information model.
     * Starting can fail if the server is not already started also.
     *
     * During startup, the server calls start on all registered
     * ServerComponents. */
    UA_StatusCode (*start)(UA_ServerComponent *sc);

    /* Stopping is asynchronous and might need a few iterations of the eventloop
     * to succeed. All ServerComponents are stopped during the shutdown of the
     * server. Once fully stopped, the ServerComponent must no longer rely on
     * the server backpointer. So it can be detached and _free'd at runtime of
     * the server. */
    void (*stop)(UA_ServerComponent *sc);

    /* Clean up and delete the ServerComponent. Can fail if it is not fully
     * stopped. When successfully removed, the ServerComponent must no longer be
     * accessed from the server.
     *
     * ServerComponents are all free'd when the server is deleted. If a
     * ServerComponent is manually removed before, then it needs to be unlinked
     * from the server's internal linked-list before. */
    UA_StatusCode (*free)(UA_ServerComponent *sc);
};

/* Adds the ServerComponent to the server.
 * Starts the component if the server is started. */
UA_StatusCode
UA_Server_addServerComponent(UA_Server *server, UA_ServerComponent *sc);

/* Remove the ServerComponent from the server.
 * This will fail if the ServerComponent is not already fully stopped. */
UA_StatusCode
UA_Server_removeServerComponent(UA_Server *server, UA_ServerComponent *sc);

_UA_END_DECLS

#endif /* UA_SERVERCOMPONENT_H_ */
