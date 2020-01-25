/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2020 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014, 2016-2017 (c) Florian Palm
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2015 (c) Oleksiy Vasylyev
 *    Copyright 2016 (c) TorbenD
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017-2018 (c) Mark Giraud, Fraunhofer IOSB
 */

#include <open62541/transport_generated_encoding_binary.h>
#include <open62541/transport_generated_handling.h>
#include <open62541/types_generated_encoding_binary.h>
#include <open62541/types_generated_handling.h>

#include "ua_securechannel.h"
#include "ua_util_internal.h"

#define UA_BITMASK_MESSAGETYPE 0x00ffffffu
#define UA_BITMASK_CHUNKTYPE 0xff000000u

const UA_ByteString UA_SECURITY_POLICY_NONE_URI =
    {47, (UA_Byte *)"http://opcfoundation.org/UA/SecurityPolicy#None"};

#ifdef UA_ENABLE_UNIT_TEST_FAILURE_HOOKS
UA_StatusCode decrypt_verifySignatureFailure;
UA_StatusCode sendAsym_sendFailure;
UA_StatusCode processSym_seqNumberFailure;
#endif

void UA_SecureChannel_init(UA_SecureChannel *channel,
                           const UA_ConnectionConfig *config) {
    /* Linked lists are also initialized by zeroing out */
    memset(channel, 0, sizeof(UA_SecureChannel));
    channel->state = UA_SECURECHANNELSTATE_FRESH;
    TAILQ_INIT(&channel->messages);
    channel->config = *config;
}

UA_StatusCode
UA_SecureChannel_setSecurityPolicy(UA_SecureChannel *channel,
                                   const UA_SecurityPolicy *securityPolicy,
                                   const UA_ByteString *remoteCertificate) {
    /* Is a policy already configured? */
    if(channel->securityPolicy) {
        UA_LOG_ERROR(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Security policy already configured");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode retval = securityPolicy->channelModule.
        newContext(securityPolicy, remoteCertificate, &channel->channelContext);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                       "Could not set up the SecureChannel context");
        return retval;
    }

    retval = UA_ByteString_copy(remoteCertificate, &channel->remoteCertificate);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_ByteString remoteCertificateThumbprint = {20, channel->remoteCertificateThumbprint};
    retval = securityPolicy->asymmetricModule.
        makeCertificateThumbprint(securityPolicy, &channel->remoteCertificate,
                                  &remoteCertificateThumbprint);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                       "Could not create the certificate thumbprint");
        return retval;
    }

    channel->securityPolicy = securityPolicy;
    return UA_STATUSCODE_GOOD;
}

static void
deleteMessage(UA_Message *me) {
    UA_ChunkPayload *cp;
    while((cp = SIMPLEQ_FIRST(&me->chunkPayloads))) {
        if(cp->copied)
            UA_ByteString_deleteMembers(&cp->bytes);
        SIMPLEQ_REMOVE_HEAD(&me->chunkPayloads, pointers);
        UA_free(cp);
    }
    UA_free(me);
}

static void
deleteLatestMessage(UA_SecureChannel *channel, UA_UInt32 requestId) {
    UA_Message *me = TAILQ_LAST(&channel->messages, UA_MessageQueue);
    if(!me)
        return;
    if(me->requestId != requestId)
        return;

    TAILQ_REMOVE(&channel->messages, me, pointers);
    deleteMessage(me);
}

void
UA_SecureChannel_deleteMessages(UA_SecureChannel *channel) {
    UA_Message *me, *me_tmp;
    TAILQ_FOREACH_SAFE(me, &channel->messages, pointers, me_tmp) {
        TAILQ_REMOVE(&channel->messages, me, pointers);
        deleteMessage(me);
    }
}

void
UA_SecureChannel_deleteMembers(UA_SecureChannel *channel) {
    /* Delete members */
    UA_ByteString_deleteMembers(&channel->remoteCertificate);
    UA_ByteString_deleteMembers(&channel->localNonce);
    UA_ByteString_deleteMembers(&channel->remoteNonce);
    UA_ChannelSecurityToken_deleteMembers(&channel->securityToken);
    UA_ChannelSecurityToken_deleteMembers(&channel->nextSecurityToken);

    /* Delete the channel context for the security policy */
    if(channel->securityPolicy) {
        channel->securityPolicy->channelModule.deleteContext(channel->channelContext);
        channel->securityPolicy = NULL;
    }

    /* Remove the buffered messages */
    UA_SecureChannel_deleteMessages(channel);
    UA_ByteString_clear(&channel->incompleteChunk);
    UA_ConnectionConfig oldConfig = channel->config;
    UA_SecureChannel_init(channel, &oldConfig);
}

void
UA_SecureChannel_close(UA_SecureChannel *channel) {
    /* Set the status to closed */
    channel->state = UA_SECURECHANNELSTATE_CLOSED;

    /* Detach from the connection and close the connection */
    if(channel->connection) {
        if(channel->connection->state != UA_CONNECTION_CLOSED)
            channel->connection->close(channel->connection);
        UA_Connection_detachSecureChannel(channel->connection);
    }

    /* Remove session pointers (not the sessions) and NULL the pointers back to
     * the SecureChannel in the Session */
    if(channel->session) {
        channel->session->channel = NULL;
        channel->session = NULL;
    }
}

UA_StatusCode
UA_SecureChannel_processHELACK(UA_SecureChannel *channel,
                               const UA_TcpAcknowledgeMessage *remoteConfig) {
    /* The lowest common version is used by both sides */
    if(channel->config.protocolVersion > remoteConfig->protocolVersion)
        channel->config.protocolVersion = remoteConfig->protocolVersion;

    /* Can we receive the max send size? */
    if(channel->config.sendBufferSize > remoteConfig->receiveBufferSize)
        channel->config.sendBufferSize = remoteConfig->receiveBufferSize;

    /* Can we send the max receive size? */
    if(channel->config.recvBufferSize > remoteConfig->sendBufferSize)
        channel->config.recvBufferSize = remoteConfig->sendBufferSize;

    channel->config.remoteMaxMessageSize = remoteConfig->maxMessageSize;
    channel->config.remoteMaxChunkCount = remoteConfig->maxChunkCount;

    /* Chunks of at least 8192 bytes must be permissible.
     * See Part 6, Clause 6.7.1 */
    if(channel->config.recvBufferSize < 8192 ||
       channel->config.sendBufferSize < 8192 ||
       (channel->config.remoteMaxMessageSize != 0 &&
        channel->config.remoteMaxMessageSize < 8192))
        return UA_STATUSCODE_BADINTERNALERROR;

    channel->connection->state = UA_CONNECTION_ESTABLISHED;

    return UA_STATUSCODE_GOOD;
}

/* Sends an OPN message using asymmetric encryption if defined */
UA_StatusCode
UA_SecureChannel_sendAsymmetricOPNMessage(UA_SecureChannel *channel,
                                          UA_UInt32 requestId, const void *content,
                                          const UA_DataType *contentType) {
    if(channel->securityMode == UA_MESSAGESECURITYMODE_INVALID)
        return UA_STATUSCODE_BADSECURITYMODEREJECTED;

    const UA_SecurityPolicy *sp = channel->securityPolicy;
    if(!sp)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_Connection *connection = channel->connection;
    if(!connection)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Allocate the message buffer */
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode retval =
        connection->getSendBuffer(connection, channel->config.sendBufferSize, &buf);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Restrict buffer to the available space for the payload */
    UA_Byte *buf_pos = buf.data;
    const UA_Byte *buf_end = &buf.data[buf.length];
    hideBytesAsym(channel, &buf_pos, &buf_end);

    /* Encode the message type and content */
    UA_NodeId typeId = UA_NODEID_NUMERIC(0, contentType->binaryEncodingId);
    retval |= UA_encodeBinary(&typeId, &UA_TYPES[UA_TYPES_NODEID], &buf_pos, &buf_end, NULL, NULL);
    retval |= UA_encodeBinary(content, contentType, &buf_pos, &buf_end, NULL, NULL);
    if(retval != UA_STATUSCODE_GOOD) {
        connection->releaseSendBuffer(connection, &buf);
        return retval;
    }

    const size_t securityHeaderLength = calculateAsymAlgSecurityHeaderLength(channel);

    /* Add padding to the chunk */
#ifdef UA_ENABLE_ENCRYPTION
    padChunkAsym(channel, &buf, securityHeaderLength, &buf_pos);
#endif

    /* The total message length */
    size_t pre_sig_length = (uintptr_t)buf_pos - (uintptr_t)buf.data;
    size_t total_length = pre_sig_length;
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        total_length += sp->asymmetricModule.cryptoModule.signatureAlgorithm.
            getLocalSignatureSize(sp, channel->channelContext);

    /* The total message length is known here which is why we encode the headers
     * at this step and not earlier. */
    size_t finalLength = 0;
    retval = prependHeadersAsym(channel, buf.data, buf_end, total_length,
                                securityHeaderLength, requestId, &finalLength);
    if(retval != UA_STATUSCODE_GOOD) {
        connection->releaseSendBuffer(connection, &buf);
        return retval;
    }

#ifdef UA_ENABLE_ENCRYPTION
    retval = signAndEncryptAsym(channel, pre_sig_length, &buf, securityHeaderLength, total_length);
    if(retval != UA_STATUSCODE_GOOD) {
        connection->releaseSendBuffer(connection, &buf);
        return retval;
    }
#endif

    /* Send the message, the buffer is freed in the network layer */
    buf.length = finalLength;
    retval = connection->send(connection, &buf);
#ifdef UA_ENABLE_UNIT_TEST_FAILURE_HOOKS
    retval |= sendAsym_sendFailure;
#endif
    return retval;
}

/* Will this chunk surpass the capacity of the SecureChannel for the message? */
static UA_StatusCode
checkLimitsSym(UA_MessageContext *const mc, size_t *const bodyLength) {
    UA_Byte *buf_body_start = mc->messageBuffer.data + UA_SECURE_MESSAGE_HEADER_LENGTH;
    const UA_Byte *buf_body_end = mc->buf_pos;
    *bodyLength = (uintptr_t)buf_body_end - (uintptr_t)buf_body_start;
    mc->messageSizeSoFar += *bodyLength;
    mc->chunksSoFar++;

    UA_SecureChannel *channel = mc->channel;
    if(mc->messageSizeSoFar > channel->config.localMaxMessageSize &&
       channel->config.localMaxMessageSize != 0)
        return UA_STATUSCODE_BADRESPONSETOOLARGE;

    if(mc->chunksSoFar > channel->config.localMaxChunkCount &&
       channel->config.localMaxChunkCount != 0)
        return UA_STATUSCODE_BADRESPONSETOOLARGE;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
encodeHeadersSym(UA_MessageContext *const messageContext, size_t totalLength) {
    UA_SecureChannel *channel = messageContext->channel;
    UA_Byte *header_pos = messageContext->messageBuffer.data;

    UA_TcpMessageHeader header;
    header.messageTypeAndChunkType = messageContext->messageType;
    header.messageSize = (UA_UInt32)totalLength;
    if(messageContext->final)
        header.messageTypeAndChunkType += UA_CHUNKTYPE_FINAL;
    else
        header.messageTypeAndChunkType += UA_CHUNKTYPE_INTERMEDIATE;

    UA_UInt32 tokenId = channel->securityToken.tokenId;
    /* This is a server SecureChannel and we have sent out the OPN response but
     * not gotten a request with the new token. So send with nextSecurityToken
     * and still allow to receive with the old one. */
    if(channel->nextSecurityToken.tokenId != 0)
        tokenId = channel->nextSecurityToken.tokenId;

    UA_SequenceHeader seqHeader;
    seqHeader.requestId = messageContext->requestId;
    seqHeader.sequenceNumber = UA_atomic_addUInt32(&channel->sendSequenceNumber, 1);

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    res |= UA_encodeBinary(&header, &UA_TRANSPORT[UA_TRANSPORT_TCPMESSAGEHEADER],
                           &header_pos, &messageContext->buf_end, NULL, NULL);
    res |= UA_encodeBinary(&channel->securityToken.channelId, &UA_TYPES[UA_TYPES_UINT32],
                           &header_pos, &messageContext->buf_end, NULL, NULL);
    res |= UA_encodeBinary(&tokenId, &UA_TYPES[UA_TYPES_UINT32],
                           &header_pos, &messageContext->buf_end, NULL, NULL);
    res |= UA_encodeBinary(&seqHeader, &UA_TRANSPORT[UA_TRANSPORT_SEQUENCEHEADER],
                           &header_pos, &messageContext->buf_end, NULL, NULL);
    return res;
}

static UA_StatusCode
sendSymmetricChunk(UA_MessageContext *messageContext) {
    UA_SecureChannel *const channel = messageContext->channel;
    const UA_SecurityPolicy *securityPolicy = channel->securityPolicy;
    UA_Connection *const connection = channel->connection;
    if(!connection)
        return UA_STATUSCODE_BADINTERNALERROR;

    size_t bodyLength = 0;
    UA_StatusCode res = checkLimitsSym(messageContext, &bodyLength);
    if(res != UA_STATUSCODE_GOOD)
        goto error;

    /* Add padding */
#ifdef UA_ENABLE_ENCRYPTION
    padChunkSym(messageContext, bodyLength);
#endif

    /* The total message length */
    size_t pre_sig_length = (uintptr_t)(messageContext->buf_pos) -
        (uintptr_t)messageContext->messageBuffer.data;
    size_t total_length = pre_sig_length;
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        total_length += securityPolicy->symmetricModule.cryptoModule.signatureAlgorithm.
            getLocalSignatureSize(securityPolicy, channel->channelContext);
    /* Space for the padding and the signature have been reserved in setBufPos() */
    UA_assert(total_length <= channel->config.sendBufferSize);

    /* For giving the buffer to the network layer */
    messageContext->messageBuffer.length = total_length;

    UA_assert(res == UA_STATUSCODE_GOOD);
    res = encodeHeadersSym(messageContext, total_length);
    if(res != UA_STATUSCODE_GOOD)
        goto error;

#ifdef UA_ENABLE_ENCRYPTION
    res = signChunkSym(messageContext, pre_sig_length);
    if(res != UA_STATUSCODE_GOOD)
        goto error;

    res = encryptChunkSym(messageContext, total_length);
    if(res != UA_STATUSCODE_GOOD)
        goto error;
#endif

    /* Send the chunk, the buffer is freed in the network layer */
    return connection->send(channel->connection, &messageContext->messageBuffer);

error:
    connection->releaseSendBuffer(channel->connection, &messageContext->messageBuffer);
    return res;
}

/* Callback from the encoding layer. Send the chunk and replace the buffer. */
static UA_StatusCode
sendSymmetricEncodingCallback(void *data, UA_Byte **buf_pos, const UA_Byte **buf_end) {
    /* Set buf values from encoding in the messagecontext */
    UA_MessageContext *mc = (UA_MessageContext *)data;
    mc->buf_pos = *buf_pos;
    mc->buf_end = *buf_end;

    /* Send out */
    UA_StatusCode retval = sendSymmetricChunk(mc);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Set a new buffer for the next chunk */
    UA_Connection *connection = mc->channel->connection;
    if(!connection)
        return UA_STATUSCODE_BADINTERNALERROR;

    retval = connection->getSendBuffer(connection, mc->channel->config.sendBufferSize,
                                       &mc->messageBuffer);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Hide bytes for header, padding and signature */
    setBufPos(mc);
    *buf_pos = mc->buf_pos;
    *buf_end = mc->buf_end;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_MessageContext_begin(UA_MessageContext *mc, UA_SecureChannel *channel,
                        UA_UInt32 requestId, UA_MessageType messageType) {
    UA_Connection *connection = channel->connection;
    if(!connection)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(messageType != UA_MESSAGETYPE_MSG && messageType != UA_MESSAGETYPE_CLO)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Create the chunking info structure */
    mc->channel = channel;
    mc->requestId = requestId;
    mc->chunksSoFar = 0;
    mc->messageSizeSoFar = 0;
    mc->final = false;
    mc->messageBuffer = UA_BYTESTRING_NULL;
    mc->messageType = messageType;

    /* Allocate the message buffer */
    UA_StatusCode retval =
        connection->getSendBuffer(connection, channel->config.sendBufferSize,
                                  &mc->messageBuffer);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Hide bytes for header, padding and signature */
    setBufPos(mc);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_MessageContext_encode(UA_MessageContext *mc, const void *content,
                         const UA_DataType *contentType) {
    UA_StatusCode retval = UA_encodeBinary(content, contentType, &mc->buf_pos, &mc->buf_end,
                                           sendSymmetricEncodingCallback, mc);
    if(retval != UA_STATUSCODE_GOOD && mc->messageBuffer.length > 0)
        UA_MessageContext_abort(mc);
    return retval;
}

UA_StatusCode
UA_MessageContext_finish(UA_MessageContext *mc) {
    mc->final = true;
    return sendSymmetricChunk(mc);
}

void
UA_MessageContext_abort(UA_MessageContext *mc) {
    UA_Connection *connection = mc->channel->connection;
    connection->releaseSendBuffer(connection, &mc->messageBuffer);
}

UA_StatusCode
UA_SecureChannel_sendSymmetricMessage(UA_SecureChannel *channel, UA_UInt32 requestId,
                                      UA_MessageType messageType, void *payload,
                                      const UA_DataType *payloadType) {
    if(!channel || !channel->connection || !payload || !payloadType)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(channel->connection->state == UA_CONNECTION_CLOSED)
        return UA_STATUSCODE_BADCONNECTIONCLOSED;

    UA_MessageContext mc;
    UA_StatusCode retval = UA_MessageContext_begin(&mc, channel, requestId, messageType);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Assert's required for clang-analyzer */
    UA_assert(mc.buf_pos == &mc.messageBuffer.data[UA_SECURE_MESSAGE_HEADER_LENGTH]);
    UA_assert(mc.buf_end <= &mc.messageBuffer.data[mc.messageBuffer.length]);

    UA_NodeId typeId = UA_NODEID_NUMERIC(0, payloadType->binaryEncodingId);
    retval = UA_MessageContext_encode(&mc, &typeId, &UA_TYPES[UA_TYPES_NODEID]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = UA_MessageContext_encode(&mc, payload, payloadType);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    return UA_MessageContext_finish(&mc);
}

/*****************************/
/* Assemble Complete Message */
/*****************************/

static UA_StatusCode
addChunkPayload(UA_SecureChannel *channel, UA_UInt32 requestId,
                UA_MessageType messageType, UA_ByteString *chunkPayload,
                UA_Boolean final) {
    UA_Message *latest = TAILQ_LAST(&channel->messages, UA_MessageQueue);
    if(latest) {
        if(latest->requestId != requestId) {
            /* Start of a new message */
            if(!latest->final)
                return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
            latest = NULL;
        } else {
            if(latest->messageType != messageType) /* MessageType mismatch */
                return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
            if(latest->final) /* Correct message, but already finalized */
                return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
        }
    }

    /* Create a new message entry */
    if(!latest) {
        latest = (UA_Message *)UA_malloc(sizeof(UA_Message));
        if(!latest)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        memset(latest, 0, sizeof(UA_Message));
        latest->requestId = requestId;
        latest->messageType = messageType;
        SIMPLEQ_INIT(&latest->chunkPayloads);
        TAILQ_INSERT_TAIL(&channel->messages, latest, pointers);
    }

    /* Test against the connection settings */
    const UA_ConnectionConfig *config = &channel->config;
    if(config->localMaxChunkCount > 0 &&
       config->localMaxChunkCount <= latest->chunkPayloadsSize)
        return UA_STATUSCODE_BADRESPONSETOOLARGE;

    if(config->localMaxMessageSize > 0 &&
       config->localMaxMessageSize < latest->messageSize + chunkPayload->length)
        return UA_STATUSCODE_BADRESPONSETOOLARGE;

    /* Create a new chunk entry */
    UA_ChunkPayload *cp = (UA_ChunkPayload *)UA_malloc(sizeof(UA_ChunkPayload));
    if(!cp)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    cp->bytes = *chunkPayload;
    cp->copied = false;

    /* Add the chunk */
    SIMPLEQ_INSERT_TAIL(&latest->chunkPayloads, cp, pointers);
    latest->chunkPayloadsSize += 1;
    latest->messageSize += chunkPayload->length;
    latest->final = final;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
processMessage(UA_SecureChannel *channel, const UA_Message *message,
               void *application, UA_ProcessMessageCallback callback) {
    if(message->chunkPayloadsSize == 1) {
        /* No need to combine chunks */
        UA_ChunkPayload *cp = SIMPLEQ_FIRST(&message->chunkPayloads);
        callback(application, channel, message->messageType, message->requestId, &cp->bytes);
    } else {
        /* Allocate memory */
        UA_ByteString bytes;
        bytes.data = (UA_Byte *)UA_malloc(message->messageSize);
        if(!bytes.data)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        bytes.length = message->messageSize;

        /* Assemble the full message */
        size_t curPos = 0;
        UA_ChunkPayload *cp;
        SIMPLEQ_FOREACH(cp, &message->chunkPayloads, pointers) {
            memcpy(&bytes.data[curPos], cp->bytes.data, cp->bytes.length);
            curPos += cp->bytes.length;
        }

        /* Process the message */
        callback(application, channel, message->messageType, message->requestId, &bytes);
        UA_ByteString_deleteMembers(&bytes);
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
processCompleteMessages(UA_SecureChannel *channel, void *application,
                        UA_ProcessMessageCallback callback) {
    UA_Message *message, *tmp_message;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    TAILQ_FOREACH_SAFE(message, &channel->messages, pointers, tmp_message) {
        /* Stop at the first incomplete message */
        if(!message->final)
            break;

        /* Has the channel been closed (during the last message)? */
        if(channel->state == UA_SECURECHANNELSTATE_CLOSED)
            break;

        /* Remove the current message before processing */
        TAILQ_REMOVE(&channel->messages, message, pointers);

        /* Process */
        retval = processMessage(channel, message, application, callback);
        if(retval != UA_STATUSCODE_GOOD)
            break;

        /* Clean up the message */
        UA_ChunkPayload *payload;
        while((payload = SIMPLEQ_FIRST(&message->chunkPayloads))) {
            if(payload->copied)
                UA_ByteString_deleteMembers(&payload->bytes);
            SIMPLEQ_REMOVE_HEAD(&message->chunkPayloads, pointers);
            UA_free(payload);
        }
        UA_free(message);
    }
    return retval;
}

#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
static UA_StatusCode
processSequenceNumberSym(UA_SecureChannel *channel, UA_UInt32 sequenceNumber) {
    /* Failure mode hook for unit tests */
#ifdef UA_ENABLE_UNIT_TEST_FAILURE_HOOKS
    if(processSym_seqNumberFailure != UA_STATUSCODE_GOOD)
        return processSym_seqNumberFailure;
#endif

    /* Does the sequence number match? */
    if(sequenceNumber != channel->receiveSequenceNumber + 1) {
        /* FIXME: Remove magic numbers :( */
        if(channel->receiveSequenceNumber + 1 > 4294966271 && sequenceNumber < 1024)
            channel->receiveSequenceNumber = sequenceNumber - 1; /* Roll over */
        else
            return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }
    ++channel->receiveSequenceNumber;
    return UA_STATUSCODE_GOOD;
}
#endif

/* The chunk body begins after the SecureConversationMessageHeader */
static UA_StatusCode
decryptAddChunk(UA_SecureChannel *channel, UA_MessageType messageType,
                UA_ChunkType chunkType, UA_ByteString *chunk,
                UA_Boolean allowPreviousToken) {
    /* Has the SecureChannel timed out? */
    if(channel->state == UA_SECURECHANNELSTATE_CLOSED)
        return UA_STATUSCODE_BADSECURECHANNELCLOSED;

    /* Is the SecureChannel configured? */
    if(!channel->connection)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Connection-level messages. These are forwarded entirely. OPN message are
     * also forwarded undecrypted */
    if(messageType == UA_MESSAGETYPE_HEL || messageType == UA_MESSAGETYPE_ACK ||
       messageType == UA_MESSAGETYPE_ERR || messageType == UA_MESSAGETYPE_OPN) {
        if(chunkType != UA_CHUNKTYPE_FINAL)
            return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
        return addChunkPayload(channel, 0, messageType, chunk, true);
    }

    /* Only messages on SecureChannel-level with symmetric encryption afterwards */
    if(messageType != UA_MESSAGETYPE_MSG &&
       messageType != UA_MESSAGETYPE_CLO)
        return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;

    /* Check the chunk type before decrypting */
    if(chunkType != UA_CHUNKTYPE_FINAL &&
       chunkType != UA_CHUNKTYPE_INTERMEDIATE &&
       chunkType != UA_CHUNKTYPE_ABORT)
        return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;

    const UA_SecurityPolicy *sp = channel->securityPolicy;
    if(!sp)
        return UA_STATUSCODE_BADINTERNALERROR;

    size_t offset = 8; /* Skip the message header */
    UA_UInt32 secureChannelId;
    UA_UInt32 tokenId;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    res |= UA_UInt32_decodeBinary(chunk, &offset, &secureChannelId);
    res |= UA_UInt32_decodeBinary(chunk, &offset, &tokenId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

#if !defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
    /* Check the ChannelId. Non-opened channels have the id zero. */
    if(secureChannelId != channel->securityToken.channelId)
        return UA_STATUSCODE_BADSECURECHANNELIDINVALID;
#endif

    /* Check (and revolve) the SecurityToken */
    res = checkSymHeader(channel, tokenId, allowPreviousToken);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_UInt32 requestId = 0;
    UA_UInt32 sequenceNumber = 0;
    res = decryptAndVerifyChunk(channel, &sp->symmetricModule.cryptoModule, messageType,
                                chunk, offset, &requestId, &sequenceNumber);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Check the sequence number. Skip sequence number checking for fuzzer to
     * improve coverage */
#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    res = processSequenceNumberSym(channel, sequenceNumber);
    if(res != UA_STATUSCODE_GOOD)
        return res;
#endif

    /* A message consisting of serveral chunks is aborted */
    if(chunkType == UA_CHUNKTYPE_ABORT) {
        deleteLatestMessage(channel, requestId);
        return UA_STATUSCODE_GOOD;
    }

    return addChunkPayload(channel, requestId, messageType,
                           chunk, chunkType == UA_CHUNKTYPE_FINAL);
}

static UA_StatusCode
persistIncompleteMessages(UA_SecureChannel *channel) {
    UA_Message *me;
    TAILQ_FOREACH(me, &channel->messages, pointers) {
        UA_ChunkPayload *cp;
        SIMPLEQ_FOREACH(cp, &me->chunkPayloads, pointers) {
            if(cp->copied)
                continue;
            UA_ByteString copy;
            UA_StatusCode retval = UA_ByteString_copy(&cp->bytes, &copy);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_SecureChannel_close(channel);
                return retval;
            }
            cp->bytes = copy;
            cp->copied = true;
        }
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
bufferIncompleteChunk(UA_SecureChannel *channel, const UA_ByteString *packet,
                      size_t offset) {
    UA_assert(channel->incompleteChunk.length == 0);
    UA_assert(offset < packet->length);
    size_t length = packet->length - offset;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&channel->incompleteChunk, length);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    memcpy(channel->incompleteChunk.data, &packet->data[offset], length);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
processChunk(UA_SecureChannel *channel, const UA_ByteString *packet,
             size_t *offset, UA_Boolean *done) {
    /* At least 8 byte needed for the header. Wait for the next chunk. */
    size_t initial_offset = *offset;
    size_t remaining = packet->length - initial_offset;
    if(remaining < 8) {
        *done = true;
        return UA_STATUSCODE_GOOD;
    }

    /* Decoding cannot fail */
    UA_TcpMessageHeader hdr;
    UA_TcpMessageHeader_decodeBinary(packet, &initial_offset, &hdr);
    UA_MessageType msgType = (UA_MessageType)
        (hdr.messageTypeAndChunkType & UA_BITMASK_MESSAGETYPE);
    UA_ChunkType chunkType = (UA_ChunkType)
        (hdr.messageTypeAndChunkType & UA_BITMASK_CHUNKTYPE);

    /* The message size is not allowed */
    if(hdr.messageSize < 16)
        return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
    if(hdr.messageSize > channel->config.recvBufferSize)
        return UA_STATUSCODE_BADTCPMESSAGETOOLARGE;

    /* Incomplete chunk */
    if(hdr.messageSize > remaining) {
        *done = true;
        return UA_STATUSCODE_GOOD;
    }

    /* ByteString with only this chunk. Don't forward the original packet
     * ByteString into decryptAddChunk. The data pointer is modified internally
     * to point to payload beyond the header. */
    UA_ByteString chunk;
    chunk.data = &packet->data[*offset];
    chunk.length = hdr.messageSize;

    /* Stop processing after a non-MSG message */
    *done = (msgType != UA_MESSAGETYPE_MSG);

    /* Process the chunk; forward the offset */
    *offset += hdr.messageSize;
    return decryptAddChunk(channel, msgType, chunkType, &chunk, true);
}

UA_StatusCode
UA_SecureChannel_processPacket(UA_SecureChannel *channel, void *application,
                               UA_ProcessMessageCallback callback,
                               const UA_ByteString *packet) {
    UA_ByteString appended = channel->incompleteChunk;

    /* Prepend the incomplete last chunk. This is usually done in the
     * networklayer. But we test for a buffered incomplete chunk here again to
     * work around "lazy" network layers. */
    if(appended.length > 0) {
        channel->incompleteChunk = UA_BYTESTRING_NULL;
        UA_Byte *t = (UA_Byte*)UA_realloc(appended.data, appended.length + packet->length);
        if(!t) {
            UA_ByteString_deleteMembers(&appended);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        memcpy(&t[appended.length], packet->data, packet->length);
        appended.data = t;
        appended.length += packet->length;
        packet = &appended;
    }

    UA_assert(channel->incompleteChunk.length == 0);

    /* Loop over the received chunks */
    size_t offset = 0;
    UA_Boolean done = false;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    while(!done) {
        res = processChunk(channel, packet, &offset, &done);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
    }

    /* Process whatever we can */
    res = processCompleteMessages(channel, application, callback);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Persist full chunks that still point to the packet */
    res = persistIncompleteMessages(channel);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Buffer half-received chunk */
    if(offset < packet->length)
        res = bufferIncompleteChunk(channel, packet, offset);

 cleanup:
    UA_ByteString_deleteMembers(&appended);
    return res;
}

UA_StatusCode
UA_SecureChannel_receiveChunksBlocking(UA_SecureChannel *channel, void *application,
                                       UA_ProcessMessageCallback callback,
                                       UA_UInt32 timeout) {
    UA_Connection *connection = channel->connection;
    if(!connection)
        return UA_STATUSCODE_BADINTERNALERROR;
    
    /* Listen for messages to arrive */
    UA_ByteString packet = UA_BYTESTRING_NULL;
    UA_StatusCode retval = connection->recv(connection, &packet, timeout);
    if(retval == UA_STATUSCODE_GOODNONCRITICALTIMEOUT)
        return UA_STATUSCODE_GOOD;
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Try to process one complete chunk */
    retval = UA_SecureChannel_processPacket(channel, application, callback, &packet);
    connection->releaseRecvBuffer(connection, &packet);
    return retval;
}

UA_StatusCode
UA_SecureChannel_receiveChunksNonBlocking(UA_SecureChannel *channel, void *application,
                                          UA_ProcessMessageCallback callback) {
    return UA_SecureChannel_receiveChunksBlocking(channel, application, callback, 0);
}
