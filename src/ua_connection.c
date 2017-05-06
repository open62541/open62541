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
prependIncomplete(UA_Connection *connection, UA_ByteString * UA_RESTRICT message,
                  UA_Boolean * UA_RESTRICT realloced) {
    UA_assert(connection->incompleteMessage.length > 0);

    /* Allocate the new message buffer */
    size_t length = connection->incompleteMessage.length + message->length;
    UA_Byte *data = (UA_Byte*)UA_realloc(connection->incompleteMessage.data, length);
    if(!data) {
        UA_ByteString_deleteMembers(&connection->incompleteMessage);
        connection->releaseRecvBuffer(connection, message);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Copy / release the current message buffer */
    memcpy(&data[connection->incompleteMessage.length], message->data, message->length);
    connection->releaseRecvBuffer(connection, message);
    message->length = length;
    message->data = data;
    connection->incompleteMessage = UA_BYTESTRING_NULL;
    *realloced = true;
    return UA_STATUSCODE_GOOD;
}

static size_t
completeChunksUntil(UA_Connection *connection, UA_ByteString * UA_RESTRICT message,
                    UA_Boolean *garbage_end) {
    size_t complete_until = 0;

    /* At least 8 byte needed for the header... */
    while(message->length - complete_until >= 8) {
        /* Check the message type */
        UA_UInt32 msgtype = (UA_UInt32)message->data[complete_until] +
                           ((UA_UInt32)message->data[complete_until+1] << 8) +
                           ((UA_UInt32)message->data[complete_until+2] << 16);
        if(msgtype != ('M' + ('S' << 8) + ('G' << 16)) &&
           msgtype != ('E' + ('R' << 8) + ('R' << 16)) &&
           msgtype != ('O' + ('P' << 8) + ('N' << 16)) &&
           msgtype != ('H' + ('E' << 8) + ('L' << 16)) &&
           msgtype != ('A' + ('C' << 8) + ('K' << 16)) &&
           msgtype != ('C' + ('L' << 8) + ('O' << 16))) {
            *garbage_end = true; /* the message type is not recognized */
            break;
        }

        /* Decoding failed or the message size is not allowed. The remaining
         * message is garbage. */
        UA_UInt32 chunk_length = 0;
        size_t length_pos = complete_until + 4;
        if(UA_UInt32_decodeBinary(message, &length_pos, &chunk_length) != UA_STATUSCODE_GOOD ||
           chunk_length < 16 || chunk_length > connection->localConf.recvBufferSize) {
            *garbage_end = true;
            break;
        }

        /* The chunk is okay but incomplete. Store the end. */
        if(chunk_length + complete_until > message->length)
            break;

        /* Continue to the next chunk */
        complete_until += chunk_length;
    }

    UA_assert(complete_until <= message->length);
    return complete_until;
}

static UA_StatusCode
separateIncompleteChunk(UA_Connection *connection, UA_ByteString * UA_RESTRICT message,
                        size_t complete_until, UA_Boolean garbage_end, UA_Boolean *realloced) {
    UA_assert(complete_until < message->length);

    /* Garbage after the last good chunk. No need to keep an incomplete message. */
    if(garbage_end) {
        if(complete_until == 0) /* All garbage */
            return UA_STATUSCODE_BADDECODINGERROR;
        message->length = complete_until;
        return UA_STATUSCODE_GOOD;
    }

    /* No good chunk, buffer the entire message */
    if(complete_until == 0) {
        if(!*realloced) {
            UA_StatusCode retval =
                UA_ByteString_allocBuffer(&connection->incompleteMessage, message->length);
            if(retval != UA_STATUSCODE_GOOD)
                return retval;
            memcpy(connection->incompleteMessage.data, message->data, message->length);
            connection->releaseRecvBuffer(connection, message);
            *realloced = true;
        } else {
            connection->incompleteMessage = *message;
            *message = UA_BYTESTRING_NULL;
        }
        return UA_STATUSCODE_GOOD;
    }

    /* At least one good chunk and an incomplete one */
    size_t incomplete_length = message->length - complete_until;
    UA_StatusCode retval =
        UA_ByteString_allocBuffer(&connection->incompleteMessage, incomplete_length);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    memcpy(connection->incompleteMessage.data, &message->data[complete_until], incomplete_length);
    message->length = complete_until;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Connection_completeMessages(UA_Connection *connection, UA_ByteString * UA_RESTRICT message,
                              UA_Boolean * UA_RESTRICT realloced) {
    /* If we have a stored an incomplete chunk, prefix to the received message.
     * After this block, connection->incompleteMessage is always empty. The
     * message and the buffer is released if allocating the memory fails. */
    if(connection->incompleteMessage.length > 0) {
        UA_StatusCode retval = prependIncomplete(connection, message, realloced);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    /* Find the end of the last complete chunk */
    UA_Boolean garbage_end = false; /* garbage after the last complete message */
    size_t complete_until = completeChunksUntil(connection, message, &garbage_end);

    /* Buffer incomplete chunk (at the end of the message) in the connection and
     * adjust the size of the message. */
    if(complete_until < message->length) {
        UA_StatusCode retval = separateIncompleteChunk(connection, message, complete_until,
                                                       garbage_end, realloced);
        if(retval != UA_STATUSCODE_GOOD) {
            if(*realloced)
                UA_ByteString_deleteMembers(message);
            else
                connection->releaseRecvBuffer(connection, message);
            return retval;
        }
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Connection_receiveChunksBlocking(UA_Connection *connection, UA_ByteString *chunks,
                                    UA_Boolean *realloced, UA_UInt32 timeout) {
    UA_DateTime now = UA_DateTime_nowMonotonic();
    UA_DateTime maxDate = now + (timeout * UA_MSEC_TO_DATETIME);
    *realloced = false;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    while(true) {
        /* Listen for messages to arrive */
        retval = connection->recv(connection, chunks, timeout);

        /* Get complete chunks and return */
        retval |= UA_Connection_completeMessages(connection, chunks, realloced);
        if(retval != UA_STATUSCODE_GOOD || chunks->length > 0)
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
