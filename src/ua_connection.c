/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014, 2016-2017 (c) Florian Palm
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2015 (c) Oleksiy Vasylyev
 *    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2019 (c) Kalycito Infotech Private Limited
 */

#include <open62541/types_generated_handling.h>

#include "ua_connection_internal.h"
#include "ua_securechannel.h"
#include "ua_types_encoding_binary.h"
#include "ua_util_internal.h"

#include <open62541/plugin/eventloop.h>

/* Hides some errors before sending them to a client according to the
 * standard. */
static void
hideErrors(UA_TcpErrorMessage *const error) {
    switch(error->error) {
    case UA_STATUSCODE_BADCERTIFICATEUNTRUSTED:
    case UA_STATUSCODE_BADCERTIFICATEREVOKED:
        error->error = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        error->reason = UA_STRING_NULL;
        break;
        // TODO: Check if these are all cases that need to be covered.
    default:
        break;
    }
}

void
UA_Connection_sendError(UA_Connection *connection, UA_TcpErrorMessage *error) {
    hideErrors(error);

    UA_TcpMessageHeader header;
    header.messageTypeAndChunkType = UA_MESSAGETYPE_ERR + UA_CHUNKTYPE_FINAL;
    // Header + ErrorMessage (error + reasonLength_field + length)
    header.messageSize = 8 + (4 + 4 + (UA_UInt32)error->reason.length);

    /* Get the send buffer from the network layer */
    UA_ByteString msg = UA_BYTESTRING_NULL;
    UA_StatusCode retval = connection->getSendBuffer(connection, header.messageSize, &msg);
    if(retval != UA_STATUSCODE_GOOD)
        return;

    /* Encode and send the response */
    UA_Byte *bufPos = msg.data;
    const UA_Byte *bufEnd = &msg.data[msg.length];
    retval |= UA_encodeBinaryInternal(&header,
                                      &UA_TRANSPORT[UA_TRANSPORT_TCPMESSAGEHEADER],
                                      &bufPos, &bufEnd, NULL, NULL);
    retval |= UA_encodeBinaryInternal(error,
                                      &UA_TRANSPORT[UA_TRANSPORT_TCPERRORMESSAGE],
                                      &bufPos, &bufEnd, NULL, NULL);
    (void)retval; /* Encoding of these cannot fail */
    msg.length = header.messageSize;
    connection->send(connection, &msg);
}

void UA_Connection_detachSecureChannel(UA_Connection *connection) {
    UA_SecureChannel *channel = connection->channel;
    /* only replace when the channel points to this connection */
    if(channel && channel->connection == connection)
        channel->connection = NULL;
    connection->channel = NULL;
}

// TODO: Return an error code
void
UA_Connection_attachSecureChannel(UA_Connection *connection, UA_SecureChannel *channel) {
    if(channel->connection == NULL) {
        channel->connection = connection;
        connection->channel = channel;
    }
}

UA_StatusCode
UA_Server_Connection_getSendBuffer(UA_Connection *connection, size_t length,
                                   UA_ByteString *buf) {
    UA_ConnectionManager *cm = (UA_ConnectionManager*)connection->handle;
    return cm->allocNetworkBuffer(cm, (uintptr_t)connection->sockfd, buf, length);
}

UA_StatusCode
UA_Server_Connection_send(UA_Connection *connection, UA_ByteString *buf) {
    UA_ConnectionManager *cm = (UA_ConnectionManager*)connection->handle;
    return cm->sendWithConnection(cm, (uintptr_t)connection->sockfd, 0, NULL, buf);
}

void
UA_Server_Connection_releaseBuffer (UA_Connection *connection, UA_ByteString *buf) {
    UA_ConnectionManager *cm = (UA_ConnectionManager*)connection->handle;
    cm->freeNetworkBuffer(cm, (uintptr_t)connection->sockfd, buf);
}

void UA_Server_Connection_close(UA_Connection *connection) {
    UA_ConnectionManager *cm = (UA_ConnectionManager*)connection->handle;
    cm->closeConnection(cm, (uintptr_t)connection->sockfd);
}
