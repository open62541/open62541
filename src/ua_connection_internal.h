/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2016-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Florian Palm
 *    Copyright 2017 (c) Mark Giraud, Fraunhofer IOSB
 */

#ifndef UA_CONNECTION_INTERNAL_H_
#define UA_CONNECTION_INTERNAL_H_

#include "ua_plugin_network.h"
#include "ua_transport_generated.h"
#include "ua_connection.h"

_UA_BEGIN_DECLS

/* Process the remote configuration in the HEL/ACK handshake. The connection
 * config is initialized with the local settings. */
UA_StatusCode
UA_Connection_old_processHELACK(UA_Connection_old *connection,
                                const UA_ConnectionConfig *localConfig,
                                const UA_ConnectionConfig *remoteConfig);

/* The application can be the client or the server */
typedef UA_StatusCode (*UA_Connection_old_processChunk)(void *application,
                                                        UA_Connection_old *connection,
                                                        UA_ByteString *chunk);

typedef UA_StatusCode (*UA_Connection_processChunkFunction)(void *application,
                                                            UA_Connection *connection,
                                                            UA_ByteString *chunk);

/* The network layer may receive chopped up messages since TCP is a streaming
 * protocol. This method calls the processChunk callback on all full chunks that
 * were received. Dangling half-complete chunks are buffered in the connection
 * and considered for the next received packet.
 *
 * If an entire chunk is received, it is forwarded directly. But the memory
 * needs to be freed with the networklayer-specific mechanism. If a half message
 * is received, we copy it into a local buffer. Then, the stack-specific free
 * needs to be used.
 *
 * @param connection The connection
 * @param application The client or server application
 * @param processCallback The function pointer for processing each chunk
 * @param packet The received packet.
 * @return Returns UA_STATUSCODE_GOOD or an error code. When an error occurs,
 *         the ingoing message and the current buffer in the connection are
 *         freed. */
UA_StatusCode
UA_Connection_old_processChunks(UA_Connection_old *connection, void *application,
                                UA_Connection_old_processChunk processCallback,
                                const UA_ByteString *packet);

UA_StatusCode
UA_Connection_processChunks(UA_Connection *connection, void *application,
                            UA_Connection_processChunkFunction processCallback,
                            const UA_ByteString *packet);

/* Try to receive at least one complete chunk on the connection. This blocks the
 * current thread up to the given timeout.
 *
 * @param connection The connection
 * @param application The client or server application
 * @param processCallback The function pointer for processing each chunk
 * @param timeout The timeout (in milliseconds) the method will block at most.
 * @return Returns UA_STATUSCODE_GOOD or an error code. When an timeout occurs,
 *         UA_STATUSCODE_GOODNONCRITICALTIMEOUT is returned. */
UA_StatusCode
UA_Connection_old_receiveChunksBlocking(UA_Connection_old *connection, void *application,
                                        UA_Connection_old_processChunk processCallback,
                                        UA_UInt32 timeout);

UA_StatusCode
UA_Connection_old_receiveChunksNonBlocking(UA_Connection_old *connection, void *application,
                                           UA_Connection_old_processChunk processCallback);

/* When a fatal error occurs the Server shall send an Error Message to the
 * Client and close the socket. When a Client encounters one of these errors, it
 * shall also close the socket but does not send an Error Message. After the
 * socket is closed a Client shall try to reconnect automatically using the
 * mechanisms described in [...]. */
void
UA_Connection_old_sendError(UA_Connection_old *connection,
                            UA_TcpErrorMessage *error);

void
UA_Connection_sendError(UA_Connection *connection,
                        UA_TcpErrorMessage *error);

void
UA_Connection_old_detachSecureChannel(UA_Connection_old *connection);

void
UA_Connection_old_attachSecureChannel(UA_Connection_old *connection,
                                       UA_SecureChannel *channel);

_UA_END_DECLS

#endif /* UA_CONNECTION_INTERNAL_H_ */
