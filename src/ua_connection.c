/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_util.h"
#include "ua_connection_internal.h"
#include "ua_types_encoding_binary.h"
#include "ua_types_generated_encoding_binary.h"
#include "ua_types_generated_handling.h"
#include "ua_securechannel.h"

void UA_Connection_deleteMembers(UA_Connection *connection) {
    UA_ByteString_deleteMembers(&connection->incompleteMessage);
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
bufferIncompleteChunk(UA_Connection *connection, const UA_Byte *pos, const UA_Byte *end) {
    size_t length = (uintptr_t)end - (uintptr_t)pos;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&connection->incompleteMessage, length);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    memcpy(connection->incompleteMessage.data, pos, length);
    return UA_STATUSCODE_GOOD;
}

static UA_Boolean
processChunk(UA_Connection *connection, void *application,
             UA_Connection_processChunk processCallback,
             const UA_Byte **posp, const UA_Byte *end) {
    const UA_Byte *pos = *posp;
    size_t length = (uintptr_t)end - (uintptr_t)pos;

    /* At least 8 byte needed for the header. Wait for the next chunk. */
    if(length < 8) {
        bufferIncompleteChunk(connection, pos, end);
        return true;
    }

    /* Check the message type */
    UA_UInt32 msgtype = (UA_UInt32)pos[0] +
                       ((UA_UInt32)pos[1] << 8) +
                       ((UA_UInt32)pos[2] << 16);
    if(msgtype != ('M' + ('S' << 8) + ('G' << 16)) &&
       msgtype != ('E' + ('R' << 8) + ('R' << 16)) &&
       msgtype != ('O' + ('P' << 8) + ('N' << 16)) &&
       msgtype != ('H' + ('E' << 8) + ('L' << 16)) &&
       msgtype != ('A' + ('C' << 8) + ('K' << 16)) &&
       msgtype != ('C' + ('L' << 8) + ('O' << 16))) {
        /* The message type is not recognized */
        return true;
    }

    UA_Byte isFinal = pos[3];
    if(isFinal != 'C' && isFinal != 'F' && isFinal != 'A') {
        /* The message type is not recognized */
        return true;
    }

    UA_UInt32 chunk_length = 0;
    UA_ByteString temp = {8, (UA_Byte*)(uintptr_t)pos}; /* At least 8 byte left */
    size_t temp_offset = 4;
    /* Decoding the UInt32 cannot fail */
    UA_UInt32_decodeBinary(&temp, &temp_offset, &chunk_length);

    /* The message size is not allowed */
    if(chunk_length < 16 || chunk_length > connection->localConf.recvBufferSize)
        return true;

    /* Wait for the next packet to process the complete chunk */
    if(chunk_length > length) {
        bufferIncompleteChunk(connection, pos, end);
        return true;
    }

    /* Process the chunk */
    temp.length = chunk_length;
    processCallback(application, connection, &temp);

    /* Continue to the next chunk */
    *posp += chunk_length;
    return false;
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
    if(connection->incompleteMessage.length > 0) {
        UA_StatusCode retval = prependIncompleteChunk(connection, &message);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        realloced = true;
    }

    /* Loop over the received chunks. pos is increased with each chunk. */
    const UA_Byte *pos = message.data;
    const UA_Byte *end = &message.data[message.length];
    UA_Boolean done;
    do {
        done = processChunk(connection, application, processCallback, &pos, end);
    } while(!done);

    if(realloced)
        UA_ByteString_deleteMembers(&message);
    return UA_STATUSCODE_GOOD;
}

/* In order to know whether a chunk was processed, we insert an indirection into
 * the callback. */
struct completeChunkTrampolineData {
    UA_Boolean called;
    void *application;
    UA_Connection_processChunk processCallback;
};

static void
completeChunkTrampoline(void *application, UA_Connection *connection,
                        UA_ByteString *chunk) {
    struct completeChunkTrampolineData *data =
        (struct completeChunkTrampolineData*)application;
    data->called = true;
    data->processCallback(data->application, connection, chunk);
}

UA_StatusCode
UA_Connection_receiveChunksBlocking(UA_Connection *connection, void *application,
                                    UA_Connection_processChunk processCallback,
                                    UA_UInt32 timeout) {
    UA_DateTime now = UA_DateTime_nowMonotonic();
    UA_DateTime maxDate = now + (timeout * UA_MSEC_TO_DATETIME);

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
        if(now > maxDate)
            return UA_STATUSCODE_GOODNONCRITICALTIMEOUT;
        timeout = (UA_UInt32)((maxDate - now) / UA_MSEC_TO_DATETIME);
    }
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
