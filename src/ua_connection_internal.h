/* This Source Code Form is subject to the terms of the Mozilla Public
*  License, v. 2.0. If a copy of the MPL was not distributed with this 
*  file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef UA_CONNECTION_INTERNAL_H_
#define UA_CONNECTION_INTERNAL_H_

#include "ua_connection.h"

/* The network layer may receive chopped up messages since TCP is a streaming
 * protocol. Furthermore, the networklayer may operate on ringbuffers or
 * statically assigned memory.
 *
 * If an entire message is received, it is forwarded directly. But the memory
 * needs to be freed with the networklayer-specific mechanism. If a half message
 * is received, we copy it into a local buffer. Then, the stack-specific free
 * needs to be used.
 *
 * @param connection The connection
 * @param message The received message. The content may be overwritten when a
 *        previsouly received buffer is completed.
 * @param realloced The Boolean value is set to true if the outgoing message has
 *        been reallocated from the network layer.
 * @return Returns UA_STATUSCODE_GOOD or an error code. When an error occurs, the ingoing message
 *         and the current buffer in the connection are freed. */
UA_StatusCode
UA_Connection_completeMessages(UA_Connection *connection, UA_ByteString *message,
                               UA_Boolean *realloced);

/* Try to receive at least one complete chunk on the connection. This blocks the
 * current thread up to the given timeout.
 *
 * @param connection The connection
 * @param chunk The received chunk. The memory is allocated either by the
 *        networklayer or internally.
 * @param realloced The Boolean value is set to true if the chunk has been
 *        reallocated from the network layer.
 * @param timeout The timeout (in milliseconds) the method will block at most.
 * @return Returns UA_STATUSCODE_GOOD or an error code. When an error occurs,
 *         the chunk buffer is returned empty. Upon a timeout,
 *         UA_STATUSCODE_GOODNONCRITICALTIMEOUT is returned.
 */
UA_StatusCode
UA_Connection_receiveChunksBlocking(UA_Connection *connection, UA_ByteString *chunks,
                                    UA_Boolean *realloced, UA_UInt32 timeout);

void UA_Connection_detachSecureChannel(UA_Connection *connection);
void UA_Connection_attachSecureChannel(UA_Connection *connection, UA_SecureChannel *channel);

/* Split the given endpoint url into hostname and port. Some of the chunks are
 * returned as pointer.
 * @param endpointUrl The endpoint URL to split up
 * @param hostname the target array for hostname. Has to be at least 256 size.
 * @param port if url contains port, it will point to the beginning of port.
 *        NULL otherwise. It may also include the path part, thus stop at
 *        position of path pointer, if it is not NULL.
 * @param path points to the first occurance of '/' after the port or NULL if no
 *        path in url
 * @return UA_STATUSCODE_BADOUTOFRANGE if url too long,
 *         UA_STATUSCODE_BADATTRIBUTEIDINVALID if url not starting with
 *         'opc.tcp://', UA_STATUSCODE_GOOD on success */
UA_StatusCode
UA_EndpointUrl_split_ptr(const char *endpointUrl, char *hostname,
                         const char ** port, const char ** path);

#endif /* UA_CONNECTION_INTERNAL_H_ */
