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

#include <open62541/plugin/network.h>
#include <open62541/transport_generated.h>

_UA_BEGIN_DECLS

/* Process the remote configuration in the HEL/ACK handshake. The connection
 * config is initialized with the local settings. */
UA_StatusCode
UA_Connection_processHELACK(UA_Connection *connection,
                            const UA_ConnectionConfig *localConfig,
                            const UA_ConnectionConfig *remoteConfig);

/* The application can be the client or the server */
typedef UA_StatusCode (*UA_Connection_processChunk)(void *application,
                                                    UA_Connection *connection,
                                                    UA_ByteString *chunk);

/* The network layer may receive several chunks in one packet since TCP is a
 * streaming protocol. The last chunk in the packet may be only partial. This
 * method calls the processChunk callback on all full chunks that were received.
 * The last incomplete chunk is buffered in the connection for the next
 * iteration.
 *
 * The packet itself is not edited in this method. But possibly in the callback
 * that is executed on complete chunks.
 *
 * @param connection The connection
 * @param application The client or server application
 * @param processCallback The function pointer for processing each chunk
 * @param packet The received packet.
 * @return Returns UA_STATUSCODE_GOOD or an error code. When an error occurs,
 *         the current buffer in the connection are
 *         freed. */
UA_StatusCode
UA_Connection_processChunks(UA_Connection *connection, void *application,
                            UA_Connection_processChunk processCallback,
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
UA_Connection_receiveChunksBlocking(UA_Connection *connection, void *application,
                                    UA_Connection_processChunk processCallback,
                                    UA_UInt32 timeout);

UA_StatusCode
UA_Connection_receiveChunksNonBlocking(UA_Connection *connection, void *application,
                                       UA_Connection_processChunk processCallback);

/* When a fatal error occurs the Server shall send an Error Message to the
 * Client and close the socket. When a Client encounters one of these errors, it
 * shall also close the socket but does not send an Error Message. After the
 * socket is closed a Client shall try to reconnect automatically using the
 * mechanisms described in [...]. */
void
UA_Connection_sendError(UA_Connection *connection,
                        UA_TcpErrorMessage *error);

void UA_Connection_detachSecureChannel(UA_Connection *connection);
void UA_Connection_attachSecureChannel(UA_Connection *connection,
                                       UA_SecureChannel *channel);

_UA_END_DECLS

#endif /* UA_CONNECTION_INTERNAL_H_ */
