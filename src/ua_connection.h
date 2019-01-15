/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2019 (c) Mark Giraud, Fraunhofer IOSB
 */

#ifndef OPEN62541_UA_CONNECTION_H
#define OPEN62541_UA_CONNECTION_H

#include "ua_server.h"
#include "ua_plugin_log.h"
#include "ua_plugin_network.h"

_UA_BEGIN_DECLS

/* Forward declarations */
typedef struct UA_ConnectionManager UA_ConnectionManager;

//struct UA_SecureChannel;
//typedef struct UA_SecureChannel UA_SecureChannel;

// TODO: Rework documentation?
/**
 * .. _networking:
 *
 * Networking Plugin API
 * =====================
 *
 * Connection
 * ----------
 * Client-server connections are represented by a `UA_Connection`. The
 * connection is stateful and stores partially received messages, and so on. In
 * addition, the connection contains function pointers to the underlying
 * networking implementation. An example for this is the `send` function. So the
 * connection encapsulates all the required networking functionality. This lets
 * users on embedded (or otherwise exotic) systems implement their own
 * networking plugins with a clear interface to the main open62541 library. */

typedef UA_StatusCode (*UA_ProcessChunkCallbackFunction)(void *userData, UA_Connection *connection,
                                                         UA_ByteString *chunk);

typedef struct {
    void *callbackContext;
    UA_ProcessChunkCallbackFunction function;
} UA_ProcessChunkCallback;

struct UA_Connection {
    UA_ConnectionState state;
    UA_ConnectionConfig config;
    UA_Logger *logger;
    UA_ProcessChunkCallback chunkCallback;

    /**
     * If a connection manager is used, this points to the used connection manager.
     * The connection will remove itself when it is freed.
     */
    UA_ConnectionManager *connectionManager;
    UA_SecureChannel *channel;       /* The securechannel that is attached to
                                      * this connection */
    UA_DateTime creationDate;         /* The date the connection was created */
    void *internalData;              /* A pointer to internal data */
    UA_ByteString chunkBuffer;       /* A half-received chunk (TCP is a
                                      * streaming protocol) is stored here */
    size_t currentChunkSize;


};

UA_StatusCode
UA_Connection_new(UA_ConnectionConfig config, UA_Socket *sock, UA_ConnectionManager *connectionManager,
                  UA_Connection **p_connection);

UA_Socket *
UA_Connection_getSocket(UA_Connection *connection);

UA_StatusCode
UA_Connection_assembleChunks(UA_Connection *connection, UA_ByteString *buffer, UA_Socket *sock);

UA_StatusCode
UA_Connection_detachSecureChannel(UA_Connection *connection);

UA_StatusCode
UA_Connection_attachSecureChannel(UA_Connection *connection,
                                  UA_SecureChannel *channel);

UA_StatusCode
UA_Connection_adjustParameters(UA_Connection *connection, const UA_ConnectionConfig *remoteConfig);

UA_StatusCode
UA_Connection_close(UA_Connection *connection);

UA_StatusCode
UA_Connection_free(UA_Connection *connection);

/**
 * Connection Manager
 *
 */

typedef struct UA_ConnectionEntry {
    UA_Connection *connection;
    TAILQ_ENTRY(UA_ConnectionEntry) pointers;
} UA_ConnectionEntry;

struct UA_ConnectionManager {
    TAILQ_HEAD(, UA_ConnectionEntry) connections; // doubly-linked list of connections
    UA_UInt32 currentConnectionCount;
    UA_Logger *logger;
};

UA_StatusCode
UA_ConnectionManager_init(UA_ConnectionManager *connectionManager, UA_Logger *logger);

UA_StatusCode
UA_ConnectionManager_deleteMembers(UA_ConnectionManager *connectionManager);

UA_StatusCode
UA_ConnectionManager_cleanupTimedOut(UA_ConnectionManager *connectionManager,
                                     UA_DateTime nowMonotonic);

UA_StatusCode
UA_ConnectionManager_add(UA_ConnectionManager *connectionManager, UA_Connection *connection);

UA_StatusCode
UA_ConnectionManager_remove(UA_ConnectionManager *connectionManager, UA_Connection *connection);

_UA_END_DECLS

#endif //OPEN62541_UA_CONNECTION_H
