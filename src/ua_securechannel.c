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
 *    Copyright 2018-2019 (c) HMS Industrial Networks AB (Author: Jonas Green)
 */

#include <open62541/transport_generated_encoding_binary.h>
#include <open62541/transport_generated_handling.h>
#include <open62541/types_generated_encoding_binary.h>
#include <open62541/types_generated_handling.h>

#include "ua_securechannel.h"
#include "ua_util_internal.h"

#define UA_BITMASK_MESSAGETYPE 0x00ffffffu
#define UA_BITMASK_CHUNKTYPE 0xff000000u
#define UA_CONNECTION_PROTOCOL_MESSAGE_HEADER_SIZE 8

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
    SIMPLEQ_INIT(&channel->completeChunks);
    SIMPLEQ_INIT(&channel->decryptedChunks);
    SLIST_INIT(&channel->sessions);
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
UA_Chunk_delete(UA_Chunk *chunk) {
    if(chunk->copied)
        UA_ByteString_clear(&chunk->bytes);
    UA_free(chunk);
}

static void
deleteChunks(UA_ChunkQueue *queue) {
    UA_Chunk *chunk;
    while((chunk = SIMPLEQ_FIRST(queue))) {
        SIMPLEQ_REMOVE_HEAD(queue, pointers);
        UA_Chunk_delete(chunk);
    }
}

void
UA_SecureChannel_deleteBuffered(UA_SecureChannel *channel) {
    deleteChunks(&channel->completeChunks);
    deleteChunks(&channel->decryptedChunks);
    UA_ByteString_clear(&channel->incompleteChunk);
}

void
UA_SecureChannel_close(UA_SecureChannel *channel) {
    /* Set the status to closed */
    channel->state = UA_SECURECHANNELSTATE_CLOSED;

    /* Detach from the connection and close the connection */
    if(channel->connection) {
        if(channel->connection->state != UA_CONNECTIONSTATE_CLOSED)
            channel->connection->close(channel->connection);
        UA_Connection_detachSecureChannel(channel->connection);
    }

    /* Remove session pointers (not the sessions) and NULL the pointers back to
     * the SecureChannel in the Session */
    UA_SessionHeader *sh;
    while((sh = SLIST_FIRST(&channel->sessions))) {
        sh->channel = NULL;
        SLIST_REMOVE_HEAD(&channel->sessions, next);
    }

    /* Delete the channel context for the security policy */
    if(channel->securityPolicy) {
        channel->securityPolicy->channelModule.deleteContext(channel->channelContext);
        channel->securityPolicy = NULL;
        channel->channelContext = NULL;
    }

    /* Delete members */
    UA_ByteString_clear(&channel->remoteCertificate);
    UA_ByteString_clear(&channel->localNonce);
    UA_ByteString_clear(&channel->remoteNonce);
    UA_ChannelSecurityToken_clear(&channel->securityToken);
    UA_ChannelSecurityToken_clear(&channel->altSecurityToken);
    UA_SecureChannel_deleteBuffered(channel);
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

    channel->connection->state = UA_CONNECTIONSTATE_ESTABLISHED;

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
    retval |= UA_encodeBinary(&contentType->binaryEncodingId, &UA_TYPES[UA_TYPES_NODEID],
                              &buf_pos, &buf_end, NULL, NULL);
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

    UA_SequenceHeader seqHeader;
    seqHeader.requestId = messageContext->requestId;
    seqHeader.sequenceNumber = UA_atomic_addUInt32(&channel->sendSequenceNumber, 1);

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    res |= UA_encodeBinary(&header, &UA_TRANSPORT[UA_TRANSPORT_TCPMESSAGEHEADER],
                           &header_pos, &messageContext->buf_end, NULL, NULL);
    res |= UA_encodeBinary(&channel->securityToken.channelId, &UA_TYPES[UA_TYPES_UINT32],
                           &header_pos, &messageContext->buf_end, NULL, NULL);
    res |= UA_encodeBinary(&channel->securityToken.tokenId, &UA_TYPES[UA_TYPES_UINT32],
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
    size_t total_length = 0;
    size_t pre_sig_length = 0;
    if(res != UA_STATUSCODE_GOOD)
        goto error;

    /* Add padding */
#ifdef UA_ENABLE_ENCRYPTION
    padChunkSym(messageContext, bodyLength);
#endif

    /* The total message length */
    pre_sig_length = (uintptr_t)(messageContext->buf_pos) -
        (uintptr_t)messageContext->messageBuffer.data;
    total_length = pre_sig_length;
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

    if(channel->state != UA_SECURECHANNELSTATE_OPEN)
        return UA_STATUSCODE_BADCONNECTIONCLOSED;

    if(channel->connection->state != UA_CONNECTIONSTATE_ESTABLISHED)
        return UA_STATUSCODE_BADCONNECTIONCLOSED;

    UA_MessageContext mc;
    UA_StatusCode retval = UA_MessageContext_begin(&mc, channel, requestId, messageType);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Assert's required for clang-analyzer */
    UA_assert(mc.buf_pos == &mc.messageBuffer.data[UA_SECURE_MESSAGE_HEADER_LENGTH]);
    UA_assert(mc.buf_end <= &mc.messageBuffer.data[mc.messageBuffer.length]);

    retval = UA_MessageContext_encode(&mc, &payloadType->binaryEncodingId,
                                      &UA_TYPES[UA_TYPES_NODEID]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = UA_MessageContext_encode(&mc, payload, payloadType);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    return UA_MessageContext_finish(&mc);
}

/********************************/
/* Receive and Process Messages */
/********************************/

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

#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
static UA_StatusCode
processSequenceNumberNop(UA_SecureChannel *channel, UA_UInt32 sequenceNumber) {
    return UA_STATUSCODE_GOOD;
}
#endif

static UA_StatusCode
decryptMessageChunk(UA_SecureChannel *channel, UA_Chunk *chunk, void *application) {
    size_t offset = UA_CONNECTION_PROTOCOL_MESSAGE_HEADER_SIZE; /* Skip the message header */
    UA_UInt32 secureChannelId;
    UA_StatusCode res = UA_UInt32_decodeBinary(&chunk->bytes, &offset, &secureChannelId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_AsymmetricAlgorithmSecurityHeader asymHeader;
    UA_SymmetricAlgorithmSecurityHeader symHeader;
    UA_StatusCode (*processSequenceNumber)(UA_SecureChannel *, UA_UInt32);
    UA_StatusCode (*checkHeader)(const UA_SecureChannel *, void *);
    void (*clearHeader)(void *);
    void *securityHeader;
    const UA_SecurityPolicyCryptoModule *cryptoModule;
    UA_SequenceHeader sequenceHeader;

    if(chunk->messageType == UA_MESSAGETYPE_OPN) {
        if(channel->state != UA_SECURECHANNELSTATE_OPEN &&
           channel->state != UA_SECURECHANNELSTATE_OPN_SENT &&
           channel->state != UA_SECURECHANNELSTATE_ACK_SENT)
            return UA_STATUSCODE_BADINVALIDSTATE;
        processSequenceNumber = processSequenceNumberAsym;
        checkHeader = (UA_StatusCode (*)(const UA_SecureChannel *, void *)) checkAsymHeader;
        securityHeader = &asymHeader;
        clearHeader = (void (*)(void *)) UA_AsymmetricAlgorithmSecurityHeader_clear;
        res = UA_AsymmetricAlgorithmSecurityHeader_decodeBinary(&chunk->bytes, &offset, &asymHeader);
        if(res != UA_STATUSCODE_GOOD)
            goto error;

        if(asymHeader.senderCertificate.length > 0) {
            if(channel->certificateVerification == NULL) {
                res = UA_STATUSCODE_BADINTERNALERROR;
                goto error;
            }
            res = channel->certificateVerification->
                verifyCertificate(channel->certificateVerification->context,
                                  &asymHeader.senderCertificate);
            if(res != UA_STATUSCODE_GOOD)
                goto error;
        }

        if(channel->processOPNHeader != NULL && channel->securityPolicy == NULL) {
            res = channel->processOPNHeader(application, channel, &asymHeader);
            if(res != UA_STATUSCODE_GOOD)
                goto error;
        }

        if(secureChannelId != 0 && channel->securityToken.channelId == 0)
            channel->securityToken.channelId = secureChannelId;

        if(!channel->securityPolicy) {
            res = UA_STATUSCODE_BADINTERNALERROR;
            goto error;
        }
        cryptoModule = &channel->securityPolicy->asymmetricModule.cryptoModule;
    } else {
        if(channel->state == UA_SECURECHANNELSTATE_CLOSED)
            return UA_STATUSCODE_BADSECURECHANNELCLOSED;
#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
        processSequenceNumber = processSequenceNumberSym;
#else
        processSequenceNumber = processSequenceNumberNop;
#endif
        checkHeader = (UA_StatusCode (*)(const UA_SecureChannel *, void *)) checkSymHeader;
        securityHeader = &symHeader;
        clearHeader = (void (*)(void *)) UA_SymmetricAlgorithmSecurityHeader_clear;
        res = UA_SymmetricAlgorithmSecurityHeader_decodeBinary(&chunk->bytes, &offset, &symHeader);
        if(res != UA_STATUSCODE_GOOD)
            goto error;

        if(!channel->securityPolicy) {
            res = UA_STATUSCODE_BADINTERNALERROR;
            goto error;
        }
        cryptoModule = &channel->securityPolicy->symmetricModule.cryptoModule;
    }

#if !defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
    /* Check the ChannelId. Non-opened channels have the id zero. */
    if(secureChannelId != channel->securityToken.channelId) {
        res = UA_STATUSCODE_BADSECURECHANNELIDINVALID;
        goto error;
    }
#endif

    /* Check (and revolve) the SecurityToken */
    res = checkHeader(channel, securityHeader);
    clearHeader(securityHeader);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Decrypt the chunk payload */
    res = decryptAndVerifyChunk(channel, cryptoModule, chunk->messageType,
                                &chunk->bytes, offset);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Check the sequence number. Skip sequence number checking for fuzzer to
     * improve coverage */
    res = UA_SequenceHeader_decodeBinary(&chunk->bytes, &offset, &sequenceHeader);
#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    res |= processSequenceNumber(channel, sequenceHeader.sequenceNumber);
#endif
    if(res != UA_STATUSCODE_GOOD)
        return res;

    chunk->requestId = sequenceHeader.requestId;
    chunk->bytes.data += offset;
    chunk->bytes.length -= offset;
    return res;

error:
    if(securityHeader != NULL)
        clearHeader(securityHeader);
    return res;
}

static UA_StatusCode
assembleProcessMessage(UA_SecureChannel *channel, void *application,
                       UA_ProcessMessageCallback callback) {
    UA_Chunk *chunk = SIMPLEQ_FIRST(&channel->decryptedChunks);
    UA_assert(chunk != NULL);

    if(chunk->chunkType == UA_CHUNKTYPE_FINAL) {
        SIMPLEQ_REMOVE_HEAD(&channel->decryptedChunks, pointers);
        UA_assert(chunk->chunkType == UA_CHUNKTYPE_FINAL);
        UA_StatusCode retval = callback(application, channel, chunk->messageType, chunk->requestId, &chunk->bytes);
        UA_Chunk_delete(chunk);
        return retval;
    }

    UA_UInt32 requestId = chunk->requestId;
    UA_MessageType messageType = chunk->messageType;
    UA_ChunkType chunkType = chunk->chunkType;
    UA_assert(chunkType == UA_CHUNKTYPE_INTERMEDIATE);

    size_t messageSize = 0;
    SIMPLEQ_FOREACH(chunk, &channel->decryptedChunks, pointers) {
        /* Consistency check */
        if(requestId != chunk->requestId)
            return UA_STATUSCODE_BADINTERNALERROR;
        if(chunkType != chunk->chunkType && chunk->chunkType != UA_CHUNKTYPE_FINAL)
            return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
        if(chunk->messageType != messageType)
            return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;

        /* Sum up the lengths */
        messageSize += chunk->bytes.length;
        if(chunk->chunkType == UA_CHUNKTYPE_FINAL)
            break;
    }

    /* Allocate memory for the full message */
    UA_ByteString payload;
    UA_StatusCode res = UA_ByteString_allocBuffer(&payload, messageSize);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    
    /* Assemble the full message */
    size_t offset = 0;
    while(true) {
        chunk = SIMPLEQ_FIRST(&channel->decryptedChunks);
        memcpy(&payload.data[offset], chunk->bytes.data, chunk->bytes.length);
        offset += chunk->bytes.length;
        SIMPLEQ_REMOVE_HEAD(&channel->decryptedChunks, pointers);
        UA_ChunkType ct = chunk->chunkType;
        UA_Chunk_delete(chunk);
        if(ct == UA_CHUNKTYPE_FINAL)
            break;
    }
    
    /* Process the assembled message */
    UA_StatusCode retval = callback(application, channel, messageType, requestId, &payload);
    UA_ByteString_clear(&payload);
    return retval;
}

static UA_StatusCode
persistCompleteChunks(UA_ChunkQueue *queue) {
    UA_Chunk *chunk;
    SIMPLEQ_FOREACH(chunk, queue, pointers) {
        if(chunk->copied)
            continue;
        UA_ByteString copy;
        UA_StatusCode retval = UA_ByteString_copy(&chunk->bytes, &copy);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        chunk->bytes = copy;
        chunk->copied = true;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
persistIncompleteChunk(UA_SecureChannel *channel, const UA_ByteString *buffer,
                       size_t offset) {
    UA_assert(channel->incompleteChunk.length == 0);
    UA_assert(offset < buffer->length);
    size_t length = buffer->length - offset;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&channel->incompleteChunk, length);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    memcpy(channel->incompleteChunk.data, &buffer->data[offset], length);
    return UA_STATUSCODE_GOOD;
}

/* Processes chunks and puts them into the payloads queue. Once a final chunk is
 * put into the queue, the message is assembled and the callback is called. The
 * queue will be cleared for the next message. */
static UA_StatusCode
processChunks(UA_SecureChannel *channel, void *application,
              UA_ProcessMessageCallback callback) {
    UA_Chunk *chunk;
    UA_StatusCode retval;
    while((chunk = SIMPLEQ_FIRST(&channel->completeChunks))) {
        /* Decrypt and add to the decrypted queue */
        SIMPLEQ_REMOVE_HEAD(&channel->completeChunks, pointers);
        if(chunk->messageType == UA_MESSAGETYPE_OPN ||
           chunk->messageType == UA_MESSAGETYPE_MSG ||
           chunk->messageType == UA_MESSAGETYPE_CLO) {
            retval = decryptMessageChunk(channel, chunk, application);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_Chunk_delete(chunk);
                return retval;
            }
        } else {
            chunk->bytes.data += UA_CONNECTION_PROTOCOL_MESSAGE_HEADER_SIZE;
            chunk->bytes.length -= UA_CONNECTION_PROTOCOL_MESSAGE_HEADER_SIZE;
        }
        SIMPLEQ_INSERT_TAIL(&channel->decryptedChunks, chunk, pointers);

        /* Check the ressource limits */
        channel->decryptedChunksCount++;
        channel->decryptedChunksLength += chunk->bytes.length;
        if((channel->config.localMaxChunkCount != 0 &&
            channel->decryptedChunksCount > channel->config.localMaxChunkCount) ||
           (channel->config.localMaxMessageSize != 0 &&
            channel->decryptedChunksLength > channel->config.localMaxMessageSize)) {
            return UA_STATUSCODE_BADTCPMESSAGETOOLARGE;
        }

        /* Continue */
        if(chunk->chunkType != UA_CHUNKTYPE_FINAL)
            continue;

        /* The decrypted queue contains a full message. Process it. */
        retval = assembleProcessMessage(channel, application, callback);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;

        /* Reset the counters */
        channel->decryptedChunksCount = 0;
        channel->decryptedChunksLength = 0;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
extractCompleteChunk(UA_SecureChannel *channel, const UA_ByteString *buffer,
                     size_t *offset, UA_Boolean *done) {
    /* At least 8 byte needed for the header. Wait for the next chunk. */
    size_t initial_offset = *offset;
    size_t remaining = buffer->length - initial_offset;
    if(remaining < UA_CONNECTION_PROTOCOL_MESSAGE_HEADER_SIZE) {
        *done = true;
        return UA_STATUSCODE_GOOD;
    }

    /* Decoding cannot fail */
    UA_TcpMessageHeader hdr;
    UA_TcpMessageHeader_decodeBinary(buffer, &initial_offset, &hdr);
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

    /* ByteString with only this chunk. */
    UA_ByteString chunkPayload;
    chunkPayload.data = &buffer->data[*offset];
    chunkPayload.length = hdr.messageSize;

    if(msgType == UA_MESSAGETYPE_HEL || msgType == UA_MESSAGETYPE_ACK ||
       msgType == UA_MESSAGETYPE_ERR || msgType == UA_MESSAGETYPE_OPN) {
        if(chunkType != UA_CHUNKTYPE_FINAL)
            return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
    } else {
        /* Only messages on SecureChannel-level with symmetric encryption afterwards */
        if(msgType != UA_MESSAGETYPE_MSG &&
           msgType != UA_MESSAGETYPE_CLO)
            return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;

        /* Check the chunk type before decrypting */
        if(chunkType != UA_CHUNKTYPE_FINAL &&
           chunkType != UA_CHUNKTYPE_INTERMEDIATE &&
           chunkType != UA_CHUNKTYPE_ABORT)
            return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
    }

    /* Add the chunk; forward the offset */
    *offset += hdr.messageSize;
    UA_Chunk *chunk = (UA_Chunk *) UA_malloc(sizeof(UA_Chunk));
    if(!chunk)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    chunk->bytes = chunkPayload;
    chunk->messageType = msgType;
    chunk->chunkType = chunkType;
    chunk->requestId = 0;
    chunk->copied = false;

    SIMPLEQ_INSERT_TAIL(&channel->completeChunks, chunk, pointers);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_SecureChannel_processBuffer(UA_SecureChannel *channel, void *application,
                               UA_ProcessMessageCallback callback,
                               const UA_ByteString *buffer) {
    /* Prepend the incomplete last chunk. This is usually done in the
     * networklayer. But we test for a buffered incomplete chunk here again to
     * work around "lazy" network layers. */
    UA_ByteString appended = channel->incompleteChunk;
    if(appended.length > 0) {
        channel->incompleteChunk = UA_BYTESTRING_NULL;
        UA_Byte *t = (UA_Byte*)UA_realloc(appended.data, appended.length + buffer->length);
        if(!t) {
            UA_ByteString_clear(&appended);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        memcpy(&t[appended.length], buffer->data, buffer->length);
        appended.data = t;
        appended.length += buffer->length;
        buffer = &appended;
    }

    /* Loop over the received chunks */
    size_t offset = 0;
    UA_Boolean done = false;
    UA_StatusCode res;
    while(!done) {
        res = extractCompleteChunk(channel, buffer, &offset, &done);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
    }

    /* Buffer half-received chunk. Before processing the messages so that
     * processing is reentrant. */
    if(offset < buffer->length) {
        res = persistIncompleteChunk(channel, buffer, offset);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
    }

    /* Process whatever we can. Chunks of completed and processed messages are
     * removed. */
    res = processChunks(channel, application, callback);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Persist full chunks that still point to the buffer. Can only return
     * UA_STATUSCODE_BADOUTOFMEMORY as an error code. So merging res works. */
    res |= persistCompleteChunks(&channel->completeChunks);
    res |= persistCompleteChunks(&channel->decryptedChunks);

 cleanup:
    UA_ByteString_clear(&appended);
    return res;
}

UA_StatusCode
UA_SecureChannel_receive(UA_SecureChannel *channel, void *application,
                         UA_ProcessMessageCallback callback, UA_UInt32 timeout) {
    UA_Connection *connection = channel->connection;
    if(!connection)
        return UA_STATUSCODE_BADINTERNALERROR;
    
    /* Listen for messages to arrive */
    UA_ByteString buffer = UA_BYTESTRING_NULL;
    UA_StatusCode retval = connection->recv(connection, &buffer, timeout);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Try to process one complete chunk */
    retval = UA_SecureChannel_processBuffer(channel, application, callback, &buffer);
    connection->releaseRecvBuffer(connection, &buffer);
    return retval;
}
