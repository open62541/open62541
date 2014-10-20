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

/** @defgroup connection Connection */

/** Used for zero-copy communication. The array of bytestrings is sent over the
   network as a single buffer. */
typedef struct UA_ByteStringArray {
    UA_UInt32      stringsSize;
    UA_ByteString *strings;
} UA_ByteStringArray;

typedef enum UA_ConnectionState {
    UA_CONNECTION_OPENING,
    UA_CONNECTION_CLOSING,
    UA_CONNECTION_ESTABLISHED
} UA_ConnectionState;

typedef struct UA_ConnectionConfig {
    UA_UInt32 protocolVersion;
    UA_UInt32 sendBufferSize;
    UA_UInt32 recvBufferSize;
    UA_UInt32 maxMessageSize;
    UA_UInt32 maxChunkCount;
} UA_ConnectionConfig;

extern UA_EXPORT UA_ConnectionConfig UA_ConnectionConfig_standard;

/* Forward declaration */
struct UA_SecureChannel;
typedef struct UA_SecureChannel UA_SecureChannel;

typedef void (*UA_Connection_writeCallback)(void *handle, const UA_ByteStringArray buf);
typedef void (*UA_Connection_closeCallback)(void *handle);

typedef struct UA_Connection {
    UA_ConnectionState  state;
    UA_ConnectionConfig localConf;
    UA_ConnectionConfig remoteConf;
    UA_SecureChannel   *channel;
    void *callbackHandle;
    UA_Connection_writeCallback write;
    UA_Connection_closeCallback close;
} UA_Connection;

UA_StatusCode UA_EXPORT UA_Connection_init(UA_Connection *connection, UA_ConnectionConfig localConf, void *callbackHandle,
                                         UA_Connection_closeCallback close, UA_Connection_writeCallback write);
void UA_EXPORT UA_Connection_deleteMembers(UA_Connection *connection);

// todo: closing a binaryconnection that was closed on the network level

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_CONNECTION_H_ */
