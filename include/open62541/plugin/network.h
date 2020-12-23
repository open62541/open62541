/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef UA_PLUGIN_NETWORK_H_
#define UA_PLUGIN_NETWORK_H_

#include <open62541/plugin/log.h>
#include <open62541/server.h>

_UA_BEGIN_DECLS

/* Forward declarations */
struct UA_Connection;
typedef struct UA_Connection UA_Connection;

struct UA_SecureChannel;
typedef struct UA_SecureChannel UA_SecureChannel;

struct UA_ServerNetworkLayer;
typedef struct UA_ServerNetworkLayer UA_ServerNetworkLayer;

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
    UA_UInt32 maxMessageSize; /* Indicated by the remote side (0 = unbounded) */
    UA_UInt32 maxChunkCount;  /* Indicated by the remote side (0 = unbounded) */
} UA_ConnectionConfig;

typedef enum {
    UA_CONNECTION_CLOSED,     /* The socket has been closed and the connection
                               * will be deleted */
    UA_CONNECTION_OPENING,    /* The socket is open, but the HEL/ACK handshake
                               * is not done */
    UA_CONNECTION_ESTABLISHED /* The socket is open and the connection
                               * configured */

} UA_ConnectionState;

struct UA_Connection {
    UA_ConnectionState state;
    UA_ConnectionConfig config;
    UA_SecureChannel *channel;     /* The securechannel that is attached to
                                    * this connection */
    UA_SOCKET sockfd;              /* Most connectivity solutions run on
                                    * sockets. Having the socket id here
                                    * simplifies the design. */
    UA_DateTime openingDate;       /* The date the connection was created */
    void *handle;                  /* A pointer to internal data */
    UA_ByteString incompleteChunk; /* A half-received chunk (TCP is a
                                    * streaming protocol) is stored here */
    UA_UInt64 connectCallbackID;   /* Callback Id, for the connect-loop */
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
     * @param response The response string. It is allocated by the connection
     *        and needs to be freed with connection->releaseBuffer
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

/* Cleans up half-received messages, and so on. Called from connection->free. */
void UA_EXPORT
UA_Connection_deleteMembers(UA_Connection *connection);

/**
 * Server Network Layer
 * --------------------
 * The server exposes two functions to interact with remote clients:
 * `processBinaryMessage` and `removeConnection`. These functions are called by
 * the server network layer.
 *
 * It is the job of the server network layer to listen on a TCP socket, to
 * accept new connections, to call the server with received messages and to
 * signal closed connections to the server.
 *
 * The network layer is part of the server config. So users can provide a custom
 * implementation if the provided example does not fit their architecture. The
 * network layer is invoked only from the server's main loop. So the network
 * layer does not need to be thread-safe. If the networklayer receives a
 * positive duration for blocking listening, the server's main loop will block
 * until a message is received or the duration times out. */

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

struct UA_ServerNetworkLayer {
    void *handle; /* Internal data */

    UA_String discoveryUrl;

    UA_ConnectionConfig localConnectionConfig;

    /* Start listening on the networklayer.
     *
     * @param nl The network layer
     * @return Returns UA_STATUSCODE_GOOD or an error code. */
    UA_StatusCode (*start)(UA_ServerNetworkLayer *nl, const UA_String *customHostname);

    /* Listen for new and closed connections and arriving packets. Calls
     * UA_Server_processBinaryMessage for the arriving packets. Closed
     * connections are picked up here and forwarded to
     * UA_Server_removeConnection where they are cleaned up and freed.
     *
     * @param nl The network layer
     * @param server The server for processing the incoming packets and for
     *               closing connections.
     * @param timeout The timeout during which an event must arrive in
     *                milliseconds
     * @return A statuscode for the status of the network layer. */
    UA_StatusCode (*listen)(UA_ServerNetworkLayer *nl, UA_Server *server,
                            UA_UInt16 timeout);

    /* Close the network socket and all open connections. Afterwards, the
     * network layer can be safely deleted.
     *
     * @param nl The network layer
     * @param server The server that processes the incoming packets and for
     *               closing connections before deleting them.
     * @return A statuscode for the status of the closing operation. */
    void (*stop)(UA_ServerNetworkLayer *nl, UA_Server *server);

    /* Deletes the network layer context. Call only after stopping. */
    void (*deleteMembers)(UA_ServerNetworkLayer *nl);
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
                              UA_UInt32 timeout, UA_Logger *logger);

_UA_END_DECLS

#endif /* UA_PLUGIN_NETWORK_H_ */
