/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef UA_PLUGIN_NETWORK_H_
#define UA_PLUGIN_NETWORK_H_

#include <open62541/util.h>
#include <open62541/plugin/log.h>

_UA_BEGIN_DECLS

/* Forward declarations */
struct UA_Connection;
typedef struct UA_Connection UA_Connection;

struct UA_SecureChannel;
typedef struct UA_SecureChannel UA_SecureChannel;

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

typedef struct {
    UA_UInt32 protocolVersion;
    UA_UInt32 recvBufferSize;
    UA_UInt32 sendBufferSize;
    UA_UInt32 localMaxMessageSize;  /* (0 = unbounded) */
    UA_UInt32 remoteMaxMessageSize; /* (0 = unbounded) */
    UA_UInt32 localMaxChunkCount;   /* (0 = unbounded) */
    UA_UInt32 remoteMaxChunkCount;  /* (0 = unbounded) */
} UA_ConnectionConfig;

typedef enum {
    UA_CONNECTIONSTATE_CLOSED,     /* The socket has been closed and the connection
                                    * will be deleted */
    UA_CONNECTIONSTATE_OPENING,    /* The socket is open, but the HEL/ACK handshake
                                    * is not done */
    UA_CONNECTIONSTATE_ESTABLISHED /* The socket is open and the connection
                                    * configured */
} UA_ConnectionState;

struct UA_Connection {
    UA_ConnectionState state;
    UA_SecureChannel *channel;     /* The securechannel that is attached to
                                    * this connection */
    UA_SOCKET sockfd;              /* Most connectivity solutions run on
                                    * sockets. Having the socket id here
                                    * simplifies the design. */
    void *handle;                  /* A pointer to internal data */

    /* Get a buffer for sending */
    UA_StatusCode (*getSendBuffer)(UA_Connection *connection, size_t length,
                                   UA_ByteString *buf);

    /* Release the send buffer manually */
    void (*releaseSendBuffer)(UA_Connection *connection, UA_ByteString *buf);

    /* Sends a message over the connection. The message buffer is always freed,
     * even if sending fails.
     *
     * @param connection The connection
     * @param buf The message buffer
     * @return Returns an error code or UA_STATUSCODE_GOOD. */
    UA_StatusCode (*send)(UA_Connection *connection, UA_ByteString *buf);

    /* Receive a message from the remote connection
     *
     * @param connection The connection

     * @param response The response string. If this is empty, it will be
     *        allocated by the connection and needs to be freed with
     *        connection->releaseBuffer. If the response string is non-empty, it
     *        will be used as the receive buffer. If bytes are received, the
     *        length of the buffer is adjusted to match the length of the
     *        received bytes.
     * @param timeout Timeout of the recv operation in milliseconds
     * @return Returns UA_STATUSCODE_BADCOMMUNICATIONERROR if the recv operation
     *         can be repeated, UA_STATUSCODE_GOOD if it succeeded and
     *         UA_STATUSCODE_BADCONNECTIONCLOSED if the connection was
     *         closed. */
    UA_StatusCode (*recv)(UA_Connection *connection, UA_ByteString *response,
                          UA_UInt32 timeout);

    /* Release the buffer of a received message */
    void (*releaseRecvBuffer)(UA_Connection *connection, UA_ByteString *buf);

    /* Close the connection. The network layer closes the socket. This is picked
     * up during the next 'listen' and the connection is freed in the network
     * layer. */
    void (*close)(UA_Connection *connection);

    /* To be called only from within the server (and not the network layer).
     * Frees up the connection's memory. */
    void (*free)(UA_Connection *connection);
};

/**
 * Client Network Layer
 * --------------------
 * The client has only a single connection used for sending and receiving binary
 * messages. */

/* @param config the connection config for this client
 * @param endpointUrl to where to connect
 * @param timeout in ms until the connection try times out if remote not reachable
 * @param logger the logger to use */
typedef UA_Connection
(*UA_ConnectClientConnection)(UA_ConnectionConfig config, UA_String endpointUrl,
                              UA_UInt32 timeout, const UA_Logger *logger);

_UA_END_DECLS

#endif /* UA_PLUGIN_NETWORK_H_ */
