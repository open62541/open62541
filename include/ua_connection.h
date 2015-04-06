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

/** Used for zero-copy communication. The array of bytestrings is sent over the
   network as a single buffer. */
typedef struct UA_ByteStringArray {
    UA_UInt32      stringsSize;
    UA_ByteString *strings;
} UA_ByteStringArray;

typedef enum UA_ConnectionState {
    UA_CONNECTION_OPENING, ///< The port is open, but we haven't received a HEL
    UA_CONNECTION_ESTABLISHED, ///< The port is open and the connection configured
    UA_CONNECTION_CLOSING, ///< The port has been closed and the connection will be deleted
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

typedef struct UA_Connection {
    UA_ConnectionState  state;
    UA_ConnectionConfig localConf;
    UA_ConnectionConfig remoteConf;
    UA_SecureChannel   *channel;
    void (*write)(void *connection, UA_ByteStringArray buf);
    void (*close)(void *connection);
    UA_ByteString incompleteMessage;
} UA_Connection;

void UA_EXPORT UA_Connection_detachSecureChannel(UA_Connection *connection);
// void UA_Connection_attachSecureChannel(UA_Connection *connection);

/** Returns a string of complete message (the length entry is decoded for that).
    If the received message is incomplete, it is retained in the connection. */
UA_ByteString UA_EXPORT UA_Connection_completeMessages(UA_Connection *connection, UA_ByteString received);

/** @} */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_CONNECTION_H_ */
