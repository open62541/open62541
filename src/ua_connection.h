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
struct UA_Connection;
typedef struct UA_Connection UA_Connection;

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

struct UA_Connection {
    UA_ConnectionState state;
    UA_ConnectionConfig config;
    UA_SecureChannel *channel;       /* The securechannel that is attached to
                                      * this connection */
    UA_Socket *sock;
    UA_DateTime openingDate;         /* The date the connection was created */
    void *handle;                    /* A pointer to internal data */
    UA_ByteString chunkBuffer;   /* A half-received chunk (TCP is a
                                      * streaming protocol) is stored here */
    UA_UInt64 connectCallbackID;     /* Callback Id, for the connect-loop */

    /* Close the connection. The network layer closes the socket. This is picked
     * up during the next 'listen' and the connection is freed in the network
     * layer. */
    void (*close)(UA_Connection *connection);

    /* To be called only from within the server (and not the network layer).
     * Frees up the connection's memory. */
    void (*free)(UA_Connection *connection);
};

UA_StatusCode
UA_Connection_detachSecureChannel(UA_Connection *connection);

UA_StatusCode
UA_Connection_attachSecureChannel(UA_Connection *connection,
                                  UA_SecureChannel *channel);

UA_StatusCode
UA_Connection_adjustParameters(UA_Connection *connection, const UA_ConnectionConfig *remoteConfig);

void UA_EXPORT
UA_Connection_deleteMembers(UA_Connection *connection);


// TODO: needed?
/* Process a binary message (TCP packet). The message can contain partial
 * chunks. (TCP is a streaming protocol and packets may be split/merge during
 * transport.) After processing, the message is freed with
 * connection->releaseRecvBuffer. */
void UA_EXPORT
UA_Server_processBinaryMessage(UA_Server *server, UA_Connection *connection,
                               UA_ByteString *message);

/* The server internally cleans up the connection and then calls
 * connection->free. */
void UA_EXPORT
UA_Server_removeConnection(UA_Server *server, UA_Connection *connection);

_UA_END_DECLS

#endif //OPEN62541_UA_CONNECTION_H
