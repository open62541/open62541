#ifndef UA_CONNECTION_INTERNAL_H_
#define UA_CONNECTION_INTERNAL_H_

#include "ua_connection.h"

/**
 * The network layer may receive chopped up messages since TCP is a streaming
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
 *         and the current buffer in the connection are freed.
 */
UA_StatusCode
UA_Connection_completeMessages(UA_Connection *connection, UA_ByteString * UA_RESTRICT message,
                               UA_Boolean * UA_RESTRICT realloced);

void UA_EXPORT UA_Connection_detachSecureChannel(UA_Connection *connection);
void UA_EXPORT UA_Connection_attachSecureChannel(UA_Connection *connection, UA_SecureChannel *channel);

#endif /* UA_CONNECTION_INTERNAL_H_ */
