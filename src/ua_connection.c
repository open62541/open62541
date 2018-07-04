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
 */

#include "ua_util_internal.h"
#include "ua_connection_internal.h"
#include "ua_types_encoding_binary.h"
#include "ua_types_generated_encoding_binary.h"
#include "ua_types_generated_handling.h"
#include "ua_transport_generated_encoding_binary.h"
#include "ua_securechannel.h"

void UA_Connection_deleteMembers(UA_Connection *connection) {
    UA_ByteString_deleteMembers(&connection->incompleteMessage);
}

/* Hides somme errors before sending them to a client according to the
 * standard. */
static void
hideErrors(UA_TcpErrorMessage *const error) {
    switch(error->error) {
    case UA_STATUSCODE_BADCERTIFICATEUNTRUSTED:
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

static UA_StatusCode
prependIncompleteChunk(UA_Connection *connection, UA_ByteString *message) {
    /* Allocate the new message buffer */
    size_t length = connection->incompleteMessage.length + message->length;
    UA_Byte *data = (UA_Byte*)UA_realloc(connection->incompleteMessage.data, length);
    if(!data) {
        UA_ByteString_deleteMembers(&connection->incompleteMessage);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Copy / release the current message buffer */
    memcpy(&data[connection->incompleteMessage.length], message->data, message->length);
    message->length = length;
    message->data = data;
    connection->incompleteMessage = UA_BYTESTRING_NULL;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
bufferIncompleteChunk(UA_Connection *connection, const UA_Byte *pos, size_t length) {
    UA_assert(length > 0);
    UA_StatusCode retval = UA_ByteString_allocBuffer(&connection->incompleteMessage, length);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    memcpy(connection->incompleteMessage.data, pos, length);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
processChunk(UA_Connection *connection, void *application,
             UA_Connection_processChunk processCallback,
             const UA_Byte **posp, const UA_Byte *end, UA_Boolean *done) {
    const UA_Byte *pos = *posp;
    const size_t length = (uintptr_t)end - (uintptr_t)pos;

    /* At least 8 byte needed for the header. Wait for the next chunk. */
    if(length < 8) {
        if(length > 0)
            bufferIncompleteChunk(connection, pos, length);
        *done = true;
        return UA_STATUSCODE_GOOD;
    }

    /* Check the message type */
    UA_MessageType msgtype = (UA_MessageType)
        ((UA_UInt32)pos[0] + ((UA_UInt32)pos[1] << 8) + ((UA_UInt32)pos[2] << 16));
    if(msgtype != UA_MESSAGETYPE_MSG && msgtype != UA_MESSAGETYPE_ERR &&
       msgtype != UA_MESSAGETYPE_OPN && msgtype != UA_MESSAGETYPE_HEL &&
       msgtype != UA_MESSAGETYPE_ACK && msgtype != UA_MESSAGETYPE_CLO) {
        /* The message type is not recognized */
        return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
    }

    UA_Byte isFinal = pos[3];
    if(isFinal != 'C' && isFinal != 'F' && isFinal != 'A') {
        /* The message type is not recognized */
        return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
    }

    UA_UInt32 chunk_length = 0;
    UA_ByteString temp = { 8, (UA_Byte*)(uintptr_t)pos }; /* At least 8 byte left */
    size_t temp_offset = 4;
    /* Decoding the UInt32 cannot fail */
    UA_UInt32_decodeBinary(&temp, &temp_offset, &chunk_length);

    /* The message size is not allowed */
    if(chunk_length < 16 || chunk_length > connection->localConf.recvBufferSize)
        return UA_STATUSCODE_BADTCPMESSAGETOOLARGE;

    /* Wait for the next packet to process the complete chunk */
    if(chunk_length > length) {
        bufferIncompleteChunk(connection, pos, length);
        *done = true;
        return UA_STATUSCODE_GOOD;
    }

    /* Process the chunk; forward the position pointer */
    temp.length = chunk_length;
    *posp += chunk_length;
    *done = false;
    return processCallback(application, connection, &temp);
}

UA_StatusCode
UA_Connection_processChunks(UA_Connection *connection, void *application,
                            UA_Connection_processChunk processCallback,
                            const UA_ByteString *packet) {
    /* If we have stored an incomplete chunk, prefix to the received message.
     * After this block, connection->incompleteMessage is always empty. The
     * message and the buffer is released if allocating the memory fails. */
    UA_Boolean realloced = false;
    UA_ByteString message = *packet;
    UA_StatusCode retval;
    if(connection->incompleteMessage.length > 0) {
        retval = prependIncompleteChunk(connection, &message);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        realloced = true;
    }

    /* Loop over the received chunks. pos is increased with each chunk. */
    const UA_Byte *pos = message.data;
    const UA_Byte *end = &message.data[message.length];
    UA_Boolean done = true;
    do {
        retval = processChunk(connection, application, processCallback,
                              &pos, end, &done);
    } while(!done && retval == UA_STATUSCODE_GOOD);

    if(realloced)
        UA_ByteString_deleteMembers(&message);
    return retval;
}

/* In order to know whether a chunk was processed, we insert an redirection into
 * the callback. */
struct completeChunkTrampolineData {
    UA_Boolean called;
    void *application;
    UA_Connection_processChunk processCallback;
};

static UA_StatusCode
completeChunkTrampoline(void *application, UA_Connection *connection,
                        UA_ByteString *chunk) {
    struct completeChunkTrampolineData *data =
        (struct completeChunkTrampolineData*)application;
    data->called = true;
    return data->processCallback(data->application, connection, chunk);
}

UA_StatusCode
UA_Connection_receiveChunksBlocking(UA_Connection *connection, void *application,
                                    UA_Connection_processChunk processCallback,
                                    UA_UInt32 timeout) {
    UA_DateTime now = UA_DateTime_nowMonotonic();
    UA_DateTime maxDate = now + (timeout * UA_DATETIME_MSEC);

    struct completeChunkTrampolineData data;
    data.called = false;
    data.application = application;
    data.processCallback = processCallback;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    while(true) {
        /* Listen for messages to arrive */
        UA_ByteString packet = UA_BYTESTRING_NULL;
        retval = connection->recv(connection, &packet, timeout);
        if(retval != UA_STATUSCODE_GOOD)
            break;

        /* Try to process one complete chunk */
        retval = UA_Connection_processChunks(connection, &data,
                                             completeChunkTrampoline, &packet);
        connection->releaseRecvBuffer(connection, &packet);
        if(data.called)
            break;

        /* We received a message. But the chunk is incomplete. Compute the
         * remaining timeout. */
        now = UA_DateTime_nowMonotonic();

        /* >= avoid timeout to be set to 0 */
        if(now >= maxDate)
            return UA_STATUSCODE_GOODNONCRITICALTIMEOUT;

        /* round always to upper value to avoid timeout to be set to 0
         * if(maxDate - now) < (UA_DATETIME_MSEC/2) */
        timeout = (UA_UInt32)(((maxDate - now) + (UA_DATETIME_MSEC - 1)) / UA_DATETIME_MSEC);
    }
    return retval;
}

UA_StatusCode
UA_Connection_receiveChunksNonBlocking(UA_Connection *connection, void *application,
                                    UA_Connection_processChunk processCallback) {
    struct completeChunkTrampolineData data;
    data.called = false;
    data.application = application;
    data.processCallback = processCallback;

    /* Listen for messages to arrive */
    UA_ByteString packet = UA_BYTESTRING_NULL;
    UA_StatusCode retval = connection->recv(connection, &packet, 1);

    if((retval != UA_STATUSCODE_GOOD) && (retval != UA_STATUSCODE_GOODNONCRITICALTIMEOUT))
        return retval;

    /* Try to process one complete chunk */
    retval = UA_Connection_processChunks(connection, &data, completeChunkTrampoline, &packet);
    connection->releaseRecvBuffer(connection, &packet);

    return retval;
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
