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

/**
 * @defgroup communication Communication
 *
 * @{
 */

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

struct UA_Connection;
typedef struct UA_Connection UA_Connection;

/**
 * The connection to a single client (or server). The connection is defined independent of the
 * underlying network layer implementation. This allows a plugging-in custom implementations (e.g.
 * an embedded TCP stack)
 */
struct UA_Connection {
    UA_ConnectionState state;
    UA_ConnectionConfig localConf;
    UA_ConnectionConfig remoteConf;
    UA_SecureChannel *channel; ///> The securechannel that is attached to this connection (or null)
    UA_Int32 sockfd; ///> Most connectivity solutions run on sockets. Having the socket id here simplifies the design.
    void *handle; ///> A pointer to the networklayer
    UA_ByteString incompleteMessage; ///> Half-received messages (tcp is a streaming protocol) get stored here
    UA_StatusCode (*getBuffer)(UA_Connection *connection, UA_ByteString *buf, size_t minSize); ///> Attach the data array to the buffer. Fails if minSize is larger than remoteConf allows
    void (*releaseBuffer)(UA_Connection *connection, UA_ByteString *buf); ///> Release the buffer
    UA_StatusCode (*write)(UA_Connection *connection, const UA_ByteString *buf); ///> The bytestrings cannot be reused after sending!
    UA_StatusCode (*recv)(UA_Connection *connection, UA_ByteString *response, UA_UInt32 timeout); // timeout in milliseconds
    void (*close)(UA_Connection *connection);
};

void UA_EXPORT UA_Connection_init(UA_Connection *connection);
void UA_EXPORT UA_Connection_deleteMembers(UA_Connection *connection);

void UA_EXPORT UA_Connection_detachSecureChannel(UA_Connection *connection);
void UA_EXPORT UA_Connection_attachSecureChannel(UA_Connection *connection, UA_SecureChannel *channel);

/** Returns a string of complete message (the length entry is decoded for that).
    If the received message is incomplete, it is retained in the connection. */
UA_ByteString UA_EXPORT UA_Connection_completeMessages(UA_Connection *connection, UA_ByteString received);

/** @} */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_CONNECTION_H_ */
