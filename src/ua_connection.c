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

#include <open62541/transport_generated_encoding_binary.h>
#include <open62541/types_generated_encoding_binary.h>
#include <open62541/types_generated_handling.h>

#include "ua_connection_internal.h"
#include "ua_securechannel.h"
#include "ua_types_encoding_binary.h"
#include "ua_util_internal.h"

UA_StatusCode
UA_Connection_processHELACK(UA_Connection *connection,
                            const UA_ConnectionConfig *localConfig,
                            const UA_ConnectionConfig *remoteConfig) {
    connection->config = *remoteConfig;

    /* The lowest common version is used by both sides */
    if(connection->config.protocolVersion > localConfig->protocolVersion)
        connection->config.protocolVersion = localConfig->protocolVersion;

    /* Can we receive the max send size? */
    if(connection->config.sendBufferSize > localConfig->recvBufferSize)
        connection->config.sendBufferSize = localConfig->recvBufferSize;

    /* Can we send the max receive size? */
    if(connection->config.recvBufferSize > localConfig->sendBufferSize)
        connection->config.recvBufferSize = localConfig->sendBufferSize;

    /* Chunks of at least 8192 bytes must be permissible.
     * See Part 6, Clause 6.7.1 */
    if(connection->config.recvBufferSize < 8192 ||
       connection->config.sendBufferSize < 8192 ||
       (connection->config.maxMessageSize != 0 &&
        connection->config.maxMessageSize < 8192))
        return UA_STATUSCODE_BADINTERNALERROR;

    connection->state = UA_CONNECTION_ESTABLISHED;

    return UA_STATUSCODE_GOOD;
}

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
    UA_TcpMessageHeader_encodeBinary(&header, &bufPos, bufEnd);
    UA_TcpErrorMessage_encodeBinary(error, &bufPos, bufEnd);
    msg.length = header.messageSize;
    connection->send(connection, &msg);
}

void UA_Connection_detachSecureChannel(UA_Connection *connection) {
    UA_SecureChannel *channel = connection->channel;
    if(channel)
        /* only replace when the channel points to this connection */
        UA_atomic_cmpxchg((void**)&channel->connection, connection, NULL);
    UA_atomic_xchg((void**)&connection->channel, NULL);
}

// TODO: Return an error code
void
UA_Connection_attachSecureChannel(UA_Connection *connection, UA_SecureChannel *channel) {
    if(UA_atomic_cmpxchg((void**)&channel->connection, NULL, connection) == NULL)
        UA_atomic_xchg((void**)&connection->channel, (void*)channel);
}
