/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014, 2017 (c) Florian Palm
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2015 (c) Chris Iatrou
 *    Copyright 2015-2016 (c) Oleksiy Vasylyev
 *    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Julian Grothoff
 */

#ifndef UA_DISCOVERY_MANAGER_H_
#define UA_DISCOVERY_MANAGER_H_

#include "ua_server_internal.h"

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_DISCOVERY

#ifdef UA_ENABLE_DISCOVERY
struct UA_DiscoveryManager;
typedef struct UA_DiscoveryManager UA_DiscoveryManager;
#endif

typedef struct registeredServer {
    LIST_ENTRY(registeredServer) pointers;
    UA_RegisteredServer registeredServer;
    UA_DateTime lastSeen;
} registeredServer;

/* Store async register service calls. So we can cancel outstanding requests
 * during shutdown. */
typedef struct {
    UA_DelayedCallback cleanupCallback; /* delayed cleanup */
    UA_Server *server;
    UA_DiscoveryManager *dm;
    UA_Client *client;
    UA_String semaphoreFilePath;
    UA_Boolean unregister;

    UA_Boolean register2;
    UA_Boolean shutdown;
    UA_Boolean connectSuccess;
} asyncRegisterRequest;

#define UA_MAXREGISTERREQUESTS 4

struct UA_DiscoveryManager {
    UA_ServerComponent sc;
    UA_UInt64 discoveryCallbackId;

    /* Outstanding requests. So they can be cancelled during shutdown. */
    asyncRegisterRequest registerRequests[UA_MAXREGISTERREQUESTS];

    LIST_HEAD(, registeredServer) registeredServers;
    size_t registeredServersSize;
};

void
UA_DiscoveryManager_setState(UA_DiscoveryManager *dm,
                             UA_LifecycleState state);

#endif /* UA_ENABLE_DISCOVERY */

_UA_END_DECLS

#endif /* UA_DISCOVERY_MANAGER_H_ */
