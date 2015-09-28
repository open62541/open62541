/*
 * Copyright (C) 2014 the contributors as stated in the AUTHORS file
 *
 * This file is part of open62541. open62541 is free software: you can
 * redistribute it and/or modify it under the terms of the GNU Lesser General
 * Public License, version 3 (as published by the Free Software Foundation) with
 * a static linking exception as stated in the LICENSE file provided with
 * open62541.
 *
 * open62541 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 */

#ifndef UA_CONNECTION_H_
#define UA_CONNECTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_types.h"
#include "ua_job.h"

typedef enum UA_ConnectionState {
    UA_CONNECTION_OPENING, ///< The socket is open, but the HEL/ACK handshake is not done
    UA_CONNECTION_ESTABLISHED, ///< The socket is open and the connection configured
    UA_CONNECTION_CLOSED, ///< The socket has been closed and the connection will be deleted
} UA_ConnectionState;

typedef struct UA_ConnectionConfig {
    UA_UInt32 protocolVersion;
    UA_UInt32 sendBufferSize;
    UA_UInt32 recvBufferSize;
    UA_UInt32 maxMessageSize;
    UA_UInt32 maxChunkCount;
} UA_ConnectionConfig;

extern const UA_EXPORT UA_ConnectionConfig UA_ConnectionConfig_standard;

/* Forward declaration */
struct UA_SecureChannel;
typedef struct UA_SecureChannel UA_SecureChannel;

/**
 * The connection to a single client (or server). The connection is defined independent of the
 * underlying network layer implementation. This allows a plugging-in custom implementations (e.g.
 * an embedded TCP stack)
 */
struct UA_Connection {
    UA_ConnectionState state;
    UA_ConnectionConfig localConf;
    UA_ConnectionConfig remoteConf;
    UA_SecureChannel *channel; ///< The securechannel that is attached to this connection (or null)
    UA_Int32 sockfd; ///< Most connectivity solutions run on sockets. Having the socket id here
                     ///  simplifies the design.
    void *handle; ///< A pointer to the networklayer
    UA_ByteString incompleteMessage; ///< A half-received message (TCP is a streaming protocol) is stored here

    /** Get a buffer for sending */
    UA_StatusCode (*getSendBuffer)(UA_Connection *connection, UA_Int32 length, UA_ByteString *buf);

    /** Release the send buffer manually */
    void (*releaseSendBuffer)(UA_Connection *connection, UA_ByteString *buf);

    /**
     * Sends a message over the connection.
     * @param connection The connection
     * @param buf The message buffer is always released (freed) internally
     * @return Returns an error code or UA_STATUSCODE_GOOD.
     */
    UA_StatusCode (*send)(UA_Connection *connection, UA_ByteString *buf);

   /**
     * Receive a message from the remote connection
	 * @param connection The connection
	 * @param response The response string. It is allocated by the connection and needs to be freed
              with connection->releaseBuffer
     * @param timeout Timeout of the recv operation in milliseconds
     * @return Returns UA_STATUSCODE_BADCOMMUNICATIONERROR if the recv operation can be repeated,
     *         UA_STATUSCODE_GOOD if it succeeded and UA_STATUSCODE_BADCONNECTIONCLOSED if the
     *         connection was closed.
	 */
    UA_StatusCode (*recv)(UA_Connection *connection, UA_ByteString *response, UA_UInt32 timeout);

    /** Release the buffer of a received message */
    void (*releaseRecvBuffer)(UA_Connection *connection, UA_ByteString *buf);

    /** Close the connection */
    void (*close)(UA_Connection *connection);
};

void UA_EXPORT UA_Connection_init(UA_Connection *connection);
void UA_EXPORT UA_Connection_deleteMembers(UA_Connection *connection);

void UA_EXPORT UA_Connection_detachSecureChannel(UA_Connection *connection);
void UA_EXPORT UA_Connection_attachSecureChannel(UA_Connection *connection, UA_SecureChannel *channel);

/** Returns a job that contains either a message-bytestring managed by the network layer or a
    message-bytestring that was newly allocated (or a nothing-job). Half-received messages are
    attached to the connection. The next completion tries to create a complete message with the next
    buffer the connection receives. */
UA_Job UA_EXPORT UA_Connection_completeMessages(UA_Connection *connection, UA_ByteString received);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_CONNECTION_H_ */
