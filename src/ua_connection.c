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
#include "ua_connection.h"

#define UA_BITMASK_MESSAGETYPE 0x00ffffff
#define UA_BITMASK_CHUNKTYPE 0xff000000

typedef struct {
    UA_Socket *sock;
} UA_Connection_internalData;

void UA_Connection_old_deleteMembers(UA_Connection_old *connection) {
    UA_ByteString_deleteMembers(&connection->incompleteChunk);
}

UA_StatusCode
UA_Connection_old_processHELACK(UA_Connection_old *connection,
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
UA_Connection_old_sendError(UA_Connection_old *connection, UA_TcpErrorMessage *error) {
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

UA_StatusCode
UA_Connection_sendError(UA_Connection *connection, UA_TcpErrorMessage *error) {
    UA_Socket *const sock = UA_Connection_getSocket(connection);
    if(sock == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    hideErrors(error);

    UA_TcpMessageHeader header;
    header.messageTypeAndChunkType = UA_MESSAGETYPE_ERR + UA_CHUNKTYPE_FINAL;
    // Header + ErrorMessage (error + reasonLength_field + length)
    header.messageSize = 8 + (4 + 4 + (UA_UInt32)error->reason.length);

    /* Get the send buffer from the network layer */
    UA_ByteString *sendBuffer = NULL;
    UA_StatusCode retval = sock->getSendBuffer(sock, header.messageSize, &sendBuffer);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Encode and send the response */
    UA_Byte *bufPos = sendBuffer->data;
    const UA_Byte *bufEnd = &sendBuffer->data[sendBuffer->length];
    // TODO: error handling
    UA_TcpMessageHeader_encodeBinary(&header, &bufPos, bufEnd);
    UA_TcpErrorMessage_encodeBinary(error, &bufPos, bufEnd);
    sendBuffer->length = header.messageSize;
    return sock->send(sock);
}

static UA_StatusCode
bufferIncompleteChunk(UA_Connection_old *connection, const UA_Byte *pos,
                      const UA_Byte *end) {
    UA_assert(pos < end);
    size_t length = (uintptr_t)end - (uintptr_t)pos;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&connection->incompleteChunk, length);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    memcpy(connection->incompleteChunk.data, pos, length);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
processChunk(UA_Connection_old *connection, void *application,
             UA_Connection_old_processChunk processCallback,
             const UA_Byte **posp, const UA_Byte *end, UA_Boolean *done) {
    const UA_Byte *pos = *posp;
    const size_t remaining = (uintptr_t)end - (uintptr_t)pos;

    /* At least 8 byte needed for the header. Wait for the next chunk. */
    if(remaining < 8) {
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
    if(chunk_length < 16 || chunk_length > connection->config.recvBufferSize)
        return UA_STATUSCODE_BADTCPMESSAGETOOLARGE;

    /* Have an the complete chunk */
    if(chunk_length > remaining) {
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
UA_Connection_old_processChunks(UA_Connection_old *connection, void *application,
                                UA_Connection_old_processChunk processCallback,
                                const UA_ByteString *packet) {
    /* The connection has already prepended any incomplete chunk during recv */
    UA_assert(connection->incompleteChunk.length == 0);

    /* Loop over the received chunks. pos is increased with each chunk. */
    const UA_Byte *pos = packet->data;
    const UA_Byte *end = &packet->data[packet->length];
    UA_Boolean done = false;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    while(!done) {
        retval = processChunk(connection, application, processCallback, &pos, end, &done);
        /* If an irrecoverable error happens: do not buffer incomplete chunk */
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    if(end > pos)
        retval = bufferIncompleteChunk(connection, pos, end);

    return retval;
}

/* In order to know whether a chunk was processed, we insert an redirection into
 * the callback. */
struct completeChunkTrampolineData {
    UA_Boolean called;
    void *application;
    UA_Connection_old_processChunk processCallback;
};

static UA_StatusCode
completeChunkTrampoline(void *application, UA_Connection_old *connection,
                        UA_ByteString *chunk) {
    struct completeChunkTrampolineData *data =
        (struct completeChunkTrampolineData*)application;
    data->called = true;
    return data->processCallback(data->application, connection, chunk);
}

UA_StatusCode
UA_Connection_old_receiveChunksBlocking(UA_Connection_old *connection, void *application,
                                        UA_Connection_old_processChunk processCallback,
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
        // TODO: Do we still need the receiveChunksBlocking function?
        // TODO: We now handle data via callbacks...
        // retval = connection->recv(connection, &packet, timeout);
        if(retval != UA_STATUSCODE_GOOD)
            break;

        /* Try to process one complete chunk */
        retval = UA_Connection_old_processChunks(connection, &data,
                                                 completeChunkTrampoline, &packet);
        // TODO: fix, see above
        // connection->releaseRecvBuffer(connection, &packet);
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
UA_Connection_old_receiveChunksNonBlocking(UA_Connection_old *connection, void *application,
                                           UA_Connection_old_processChunk processCallback) {
    struct completeChunkTrampolineData data;
    data.called = false;
    data.application = application;
    data.processCallback = processCallback;

    /* Listen for messages to arrive */
    UA_ByteString packet = UA_BYTESTRING_NULL;
    // TODO: Do we still need receiveChunksNonBlocking now? We use callbacks now...
    UA_StatusCode retval = connection->recv(connection, &packet, 1);

    if((retval != UA_STATUSCODE_GOOD) && (retval != UA_STATUSCODE_GOODNONCRITICALTIMEOUT))
        return retval;

    /* Try to process one complete chunk */
    retval = UA_Connection_old_processChunks(connection, &data, completeChunkTrampoline, &packet);
    // TODO: see above
    // connection->releaseRecvBuffer(connection, &packet);

    return retval;
}

void
UA_Connection_old_detachSecureChannel(UA_Connection_old *connection) {
    UA_SecureChannel *channel = connection->channel;
    if(channel)
        /* only replace when the channel points to this connection */
        UA_atomic_cmpxchg((void**)&channel->connection, connection, NULL);
    UA_atomic_xchg((void**)&connection->channel, NULL);
}

void
UA_Connection_old_attachSecureChannel(UA_Connection_old *connection, UA_SecureChannel *channel) {
    if(UA_atomic_cmpxchg((void**)&channel->connection, NULL, connection) == NULL)
        UA_atomic_xchg((void**)&connection->channel, (void*)channel);
}

UA_StatusCode
UA_Connection_detachSecureChannel(UA_Connection *connection) {
    UA_SecureChannel *channel = connection->channel;
    if(channel)
        /* only replace when the channel points to this connection */
        UA_atomic_cmpxchg((void **)&channel->connection, connection, NULL);
    UA_atomic_xchg((void **)&connection->channel, NULL);

    return UA_STATUSCODE_GOOD;
}

// TODO: Return an error code
UA_StatusCode
UA_Connection_attachSecureChannel(UA_Connection *connection, UA_SecureChannel *channel) {
    if(UA_atomic_cmpxchg((void **)&channel->connection, NULL, connection) == NULL)
        UA_atomic_xchg((void **)&connection->channel, (void *)channel);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Connection_adjustParameters(UA_Connection *connection, const UA_ConnectionConfig *remoteConfig) {

    /* The lowest common version is used by both sides */
    if(connection->config.protocolVersion > remoteConfig->protocolVersion)
        connection->config.protocolVersion = remoteConfig->protocolVersion;

    // connection->config = *remoteConfig;

    /* Clamp the buffer if we can receive more than the remote can send */
    if(connection->config.recvBufferSize > remoteConfig->sendBufferSize)
        connection->config.recvBufferSize = remoteConfig->sendBufferSize;

    /* Clamp the send buffer to the maximum recv buffer size of the remote to avoid abortion of chunks. */
    if(connection->config.sendBufferSize > remoteConfig->recvBufferSize)
        connection->config.sendBufferSize = remoteConfig->recvBufferSize;

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

UA_Socket *
UA_Connection_getSocket(UA_Connection *connection) {
    if(connection == NULL)
        return NULL;
    return ((UA_Connection_internalData *)connection->internalData)->sock;
}

UA_StatusCode
UA_ConnectionManager_init(UA_ConnectionManager *connectionManager, UA_Logger *logger) {
    memset(connectionManager, 0, sizeof(UA_ConnectionManager));

    connectionManager->logger = logger;
    connectionManager->currentConnectionCount = 0;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_ConnectionManager_add(UA_ConnectionManager *connectionManager, UA_Connection *connection) {
    if(connection == NULL || connectionManager == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_ConnectionEntry *connectionEntry = (UA_ConnectionEntry *)UA_malloc(sizeof(UA_ConnectionEntry));
    connectionEntry->connection = connection;
    TAILQ_INSERT_TAIL(&connectionManager->connections, connectionEntry, pointers);
    UA_atomic_addUInt32(&connectionManager->currentConnectionCount, 1);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_ConnectionManager_remove(UA_ConnectionManager *connectionManager, UA_Connection *connection) {
    UA_ConnectionEntry *connectionEntry;
    TAILQ_FOREACH(connectionEntry, &connectionManager->connections, pointers) {
        if(connectionEntry->connection == connection)
            break;
    }
    if(!connectionEntry)
        return UA_STATUSCODE_GOOD;

    TAILQ_REMOVE(&connectionManager->connections, connectionEntry, pointers);
    UA_free(connectionEntry);
    UA_atomic_subUInt32(&connectionManager->currentConnectionCount, 1);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_ConnectionManager_deleteMembers(UA_ConnectionManager *connectionManager) {
    UA_ConnectionEntry *connectionEntry;
    UA_ConnectionEntry *tmp;
    TAILQ_FOREACH_SAFE(connectionEntry, &connectionManager->connections, pointers, tmp) {
        TAILQ_REMOVE(&connectionManager->connections, connectionEntry, pointers);
        UA_free(connectionEntry);
        UA_atomic_subUInt32(&connectionManager->currentConnectionCount, 1);
    }
    return UA_STATUSCODE_GOOD;
}

#define NOHELLOTIMEOUT 120000 /* Timeout in ms before the connection is closed,
                               * if server does not receive a HEL message. */

UA_StatusCode
UA_ConnectionManager_cleanupTimedOut(UA_ConnectionManager *connectionManager, UA_DateTime nowMonotonic) {

    UA_ConnectionEntry *connectionEntry;
    UA_ConnectionEntry *tmp;
    TAILQ_FOREACH_SAFE(connectionEntry, &connectionManager->connections, pointers, tmp) {
        int sockid = (int)(UA_Connection_getSocket(connectionEntry->connection)->id);
        if((connectionEntry->connection->state == UA_CONNECTION_OPENING) &&
           (nowMonotonic > (connectionEntry->connection->creationDate + (NOHELLOTIMEOUT * UA_DATETIME_MSEC)))) {
            UA_LOG_INFO(connectionEntry->connection->logger, UA_LOGCATEGORY_NETWORK,
                        "Freeing connection with associated socket %i (no Hello Message).", sockid);
            TAILQ_REMOVE(&connectionManager->connections, connectionEntry, pointers);
            UA_Connection_free(connectionEntry->connection);
            UA_free(connectionEntry);
        }
        if(connectionEntry->connection->state == UA_CONNECTION_CLOSED) {
            UA_LOG_DEBUG(connectionEntry->connection->logger, UA_LOGCATEGORY_NETWORK,
                         "Freeing connection with associated socket %i (closed).", sockid);
            TAILQ_REMOVE(&connectionManager->connections, connectionEntry, pointers);
            UA_Connection_free(connectionEntry->connection);
            UA_free(connectionEntry);
        }
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Connection_new(UA_ConnectionConfig config, UA_Socket *sock,
                  UA_ConnectionManager *connectionManager,
                  UA_Connection **p_connection) {
    if(sock == NULL || p_connection == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_Connection *const connection = (UA_Connection *const)UA_malloc(sizeof(UA_Connection));
    if(connection == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memset(connection, 0, sizeof(UA_Connection));

    connection->config = config;
    connection->state = UA_CONNECTION_OPENING;
    connection->creationDate = UA_DateTime_nowMonotonic();
    connection->logger = sock->logger;

    connection->internalData = UA_malloc(sizeof(UA_Connection_internalData));
    UA_Connection_internalData *const internalData = (UA_Connection_internalData *const)connection->internalData;
    if(internalData == NULL) {
        UA_free(connection);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    connection->connectionManager = connectionManager;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(connection->config.recvBufferSize < 8) {
        retval = UA_STATUSCODE_BADCONFIGURATIONERROR;
        UA_LOG_ERROR(sock->logger, UA_LOGCATEGORY_NETWORK,
                     "Receive buffer has to be at least 8 bytes large.");
        goto error;
    }
    retval = UA_ByteString_allocBuffer(&connection->chunkBuffer, connection->config.recvBufferSize);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;
    connection->chunkBuffer.length = 0;

    internalData->sock = sock;

    if(connectionManager != NULL)
        UA_ConnectionManager_add(connectionManager, connection);

    *p_connection = connection;

    return UA_STATUSCODE_GOOD;

error:
    UA_ByteString_deleteMembers(&connection->chunkBuffer);
    UA_free(internalData);
    UA_free(connection);
    return retval;
}

#define UA_MESSAGE_HEADER_SIZE 8

static UA_StatusCode
UA_Connection_resetChunkBuffer(UA_Connection *connection) {
    connection->currentChunkSize = 0;
    connection->chunkBuffer.length = 0;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_Connection_validateChunkHeader(UA_Connection *connection, UA_TcpMessageHeader *messageHeader) {
    if(messageHeader->messageSize > connection->config.recvBufferSize) {
        UA_LOG_ERROR(connection->logger, UA_LOGCATEGORY_NETWORK,
                     "The size of the received chunk is too large to be processed.");
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    }
    if(messageHeader->messageSize < 16) {
        UA_LOG_ERROR(connection->logger, UA_LOGCATEGORY_NETWORK,
                     "The size of the received chunk is too small to be processed.");
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    }

    UA_MessageType messageType = (UA_MessageType)
        (messageHeader->messageTypeAndChunkType & UA_BITMASK_MESSAGETYPE);
    UA_ChunkType chunkType = (UA_ChunkType)
        (messageHeader->messageTypeAndChunkType & UA_BITMASK_CHUNKTYPE);

    if(messageType != UA_MESSAGETYPE_MSG && messageType != UA_MESSAGETYPE_ERR &&
       messageType != UA_MESSAGETYPE_OPN && messageType != UA_MESSAGETYPE_HEL &&
       messageType != UA_MESSAGETYPE_ACK && messageType != UA_MESSAGETYPE_CLO) {
        return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
    }

    if(chunkType != UA_CHUNKTYPE_ABORT &&
       chunkType != UA_CHUNKTYPE_FINAL &&
       chunkType != UA_CHUNKTYPE_INTERMEDIATE) {
        return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Connection_assembleChunk(UA_Connection *connection, UA_ByteString *buffer, UA_Socket *sock) {
    size_t alreadyCopied = 0;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    while(alreadyCopied < buffer->length) {
        if(connection->chunkBuffer.length < UA_MESSAGE_HEADER_SIZE) {
            size_t bytesToCopy = UA_MESSAGE_HEADER_SIZE - connection->chunkBuffer.length;
            if(buffer->length < bytesToCopy)
                bytesToCopy = buffer->length;
            memcpy(connection->chunkBuffer.data + connection->chunkBuffer.length,
                   buffer->data + alreadyCopied, bytesToCopy);
            connection->chunkBuffer.length += bytesToCopy;
            alreadyCopied += bytesToCopy;
        }

        if(connection->chunkBuffer.length < UA_MESSAGE_HEADER_SIZE) {
            // not enough data to assemble chunk. Skip until next receive
            return UA_STATUSCODE_GOOD;
        }

        if(connection->currentChunkSize == 0) {
            UA_TcpMessageHeader messageHeader;
            size_t offset = 0;
            retval = UA_TcpMessageHeader_decodeBinary(&connection->chunkBuffer, &offset, &messageHeader);
            if(retval != UA_STATUSCODE_GOOD)
                goto error;
            retval = UA_Connection_validateChunkHeader(connection, &messageHeader);
            if(retval != UA_STATUSCODE_GOOD)
                goto error;
            connection->currentChunkSize = messageHeader.messageSize;
        }

        size_t bytesToCopy = connection->currentChunkSize - connection->chunkBuffer.length;
        if((buffer->length - alreadyCopied) < bytesToCopy)
            bytesToCopy = buffer->length - alreadyCopied;
        memcpy(connection->chunkBuffer.data + connection->chunkBuffer.length,
               buffer->data + alreadyCopied, bytesToCopy);
        connection->chunkBuffer.length += bytesToCopy;
        alreadyCopied += bytesToCopy;

        if(connection->chunkBuffer.length == connection->currentChunkSize) {
            if(connection->chunkCallback.function != NULL) {
                // TODO: check retvals.
                retval = connection->chunkCallback.function(connection->chunkCallback.callbackContext,
                                                            connection, &connection->chunkBuffer);
                if(retval != UA_STATUSCODE_GOOD)
                    goto error;
            } else {
                UA_LOG_WARNING(connection->logger, UA_LOGCATEGORY_NETWORK,
                               "Discarding chunk, because no chunk callback is set");
            }

            UA_Connection_resetChunkBuffer(connection);
        }
    }
    return retval;

error:
    UA_Connection_resetChunkBuffer(connection);
    UA_LOG_ERROR(connection->logger, UA_LOGCATEGORY_NETWORK,
                 "Socket %i | Processing the message failed with "
                 "error %s", (int)(sock->id), UA_StatusCode_name(retval));
    /* Send an ERR message and close the connection */
    UA_TcpErrorMessage error;
    error.error = retval;
    error.reason = UA_STRING_NULL;
    UA_Connection_sendError(connection, &error);
    UA_Connection_close(connection);
    return retval;
}

UA_StatusCode
UA_Connection_close(UA_Connection *connection) {
    if(connection->state == UA_CONNECTION_CLOSED)
        return UA_STATUSCODE_GOOD;
    connection->state = UA_CONNECTION_CLOSED;
    if(connection->channel != NULL)
        UA_SecureChannel_close(connection->channel);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Connection_free(UA_Connection *connection) {
    if(connection == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_LOG_DEBUG(connection->logger, UA_LOGCATEGORY_NETWORK,
                 "Freeing connection %p", (void *)connection);
    if(connection->connectionManager != NULL)
        UA_ConnectionManager_remove(connection->connectionManager, connection);
    if(connection->channel != NULL)
        UA_SecureChannel_close(connection->channel);
    UA_ByteString_deleteMembers(&connection->chunkBuffer);
    UA_free(connection->internalData);
    UA_free(connection);
    return UA_STATUSCODE_GOOD;
}
