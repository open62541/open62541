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
#include <open62541/plugin/networkmanager.h>
#include "ua_securechannel.h"
#include "ua_util_internal.h"
#include <open62541/connection.h>

typedef struct {
    UA_Socket *sock;
} UA_Connection_internalData;

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

static UA_StatusCode
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
    UA_StatusCode retval = sock->acquireSendBuffer(sock, header.messageSize, &sendBuffer);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Encode and send the response */
    UA_Byte *bufPos = sendBuffer->data;
    const UA_Byte *bufEnd = &sendBuffer->data[sendBuffer->length];
    // TODO: error handling
    UA_TcpMessageHeader_encodeBinary(&header, &bufPos, bufEnd);
    UA_TcpErrorMessage_encodeBinary(error, &bufPos, bufEnd);
    sendBuffer->length = header.messageSize;
    return sock->send(sock, sendBuffer);
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
        } else if(connectionEntry->connection->state == UA_CONNECTION_CLOSED) {
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
UA_Connection_new(UA_ConnectionConfig config, UA_Socket *sock, UA_ConnectionManager *connectionManager,
                  const UA_Logger *logger, UA_Connection **p_connection) {
    if(sock == NULL || logger == NULL || p_connection == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_Connection *const connection = (UA_Connection *const)UA_malloc(sizeof(UA_Connection));
    if(connection == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memset(connection, 0, sizeof(UA_Connection));

    connection->config = config;
    connection->state = UA_CONNECTION_OPENING;
    connection->creationDate = UA_DateTime_nowMonotonic();
    connection->logger = logger;

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
        UA_LOG_ERROR(sock->networkManager->logger, UA_LOGCATEGORY_NETWORK,
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

static UA_StatusCode
UA_Connection_parseChunkSize(UA_Connection *connection) {
    UA_TcpMessageHeader messageHeader;
    size_t offset = 0;
    UA_StatusCode retval = UA_TcpMessageHeader_decodeBinary(&connection->chunkBuffer, &offset, &messageHeader);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    retval = UA_Connection_validateChunkHeader(connection, &messageHeader);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    connection->currentChunkSize = messageHeader.messageSize;

    return retval;
}

static void
UA_Connection_copyChunkPart(UA_Connection *connection, UA_ByteString *buffer,
                            size_t *offset, size_t bytesToCopy) {
    if((buffer->length - *offset) < bytesToCopy)
        bytesToCopy = buffer->length - *offset;
    memcpy(connection->chunkBuffer.data + connection->chunkBuffer.length,
           buffer->data + *offset, bytesToCopy);
    connection->chunkBuffer.length += bytesToCopy;
    *offset += bytesToCopy;
}

UA_StatusCode
UA_Connection_assembleChunks(UA_ByteString *buffer, UA_Socket *sock) {
#ifdef UA_DEBUG_DUMP_PKGS
    UA_dump_hex_pkg(buffer->data, buffer->length);
#endif
    UA_Connection *const connection = (UA_Connection *const)sock->context;
    size_t offset = 0;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    while(offset < buffer->length) {
        if(connection->chunkBuffer.length < UA_MESSAGE_HEADER_SIZE) {
            UA_Connection_copyChunkPart(connection, buffer, &offset,
                                        UA_MESSAGE_HEADER_SIZE - connection->chunkBuffer.length);
        }

        if(connection->chunkBuffer.length < UA_MESSAGE_HEADER_SIZE) {
            // not enough data to assemble chunk. Skip until next receive
            return UA_STATUSCODE_GOOD;
        }

        if(connection->currentChunkSize == 0) {
            retval = UA_Connection_parseChunkSize(connection);
            if(retval != UA_STATUSCODE_GOOD)
                goto error;
        }

        UA_Connection_copyChunkPart(connection, buffer, &offset,
                                    connection->currentChunkSize - connection->chunkBuffer.length);

        if(connection->chunkBuffer.length == connection->currentChunkSize) {
            if(connection->chunkCallback.function != NULL) {
                retval = connection->chunkCallback.function(connection->chunkCallback.callbackContext,
                                                            connection, &connection->chunkBuffer);
                if(retval != UA_STATUSCODE_GOOD)
                    goto error;
            } else {
                UA_LOG_WARNING(connection->logger, UA_LOGCATEGORY_NETWORK,
                               "Discarding chunk, because no chunk callback is set");
            }

            UA_Connection_resetChunkBuffer(connection);
        } else if(connection->chunkBuffer.length > connection->currentChunkSize) {
            retval = UA_STATUSCODE_BADINTERNALERROR;
            goto error;
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
    UA_Socket *sock = ((UA_Connection_internalData *)connection->internalData)->sock;
    if(sock != NULL) {
        sock->close(sock);
    }
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
