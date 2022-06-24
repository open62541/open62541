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
#include <open62541/plugin/eventloop.h>
#include <open62541/transport_generated.h>

_UA_BEGIN_DECLS



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

/********************************/
/* Server Eventloop integration */
/********************************/


/* In this example, we integrate the server into an external "mainloop". This
   can be for example the event-loop used in GUI toolkits, such as Qt or GTK. */

UA_StatusCode UA_Server_Connection_getSendBuffer(UA_Connection *connection, size_t length,
                                                        UA_ByteString *buf);
UA_StatusCode UA_Server_Connection_send(UA_Connection *connection, UA_ByteString *buf);
void UA_Server_Connection_releaseBuffer (UA_Connection *connection, UA_ByteString *buf);
void UA_Server_Connection_close(UA_Connection *connection);

/* Process a binary message (TCP packet). The message can contain partial
 * chunks. (TCP is a streaming protocol and packets may be split/merge during
 * transport.) After processing, the message is freed with
 * connection->releaseRecvBuffer. */
void UA_Server_processBinaryMessage(UA_Server *server, UA_Connection *connection,
                                    UA_ByteString *message);

_UA_END_DECLS

#endif /* UA_CONNECTION_INTERNAL_H_ */
