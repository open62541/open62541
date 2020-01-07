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

void
UA_SecureChannel_init(UA_SecureChannel *channel) {
    /* Linked lists are also initialized by zeroing out */
    memset(channel, 0, sizeof(UA_SecureChannel));
    channel->state = UA_SECURECHANNELSTATE_FRESH;
    TAILQ_INIT(&channel->messages);
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

    UA_StatusCode retval;
    if(securityPolicy->certificateVerification != NULL) {
        retval = securityPolicy->certificateVerification->
            verifyCertificate(securityPolicy->certificateVerification->context,
                              remoteCertificate);

        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                           "Could not verify the remote certificate");
            return retval;
        }
    } else {
        UA_LOG_WARNING(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                       "Security policy None is used to create SecureChannel. Accepting all certificates");
    }

    retval = securityPolicy->channelModule.
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

    UA_SecureChannel_init(channel);
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
    UA_SessionHeader *sh, *temp;
    LIST_FOREACH_SAFE(sh, &channel->sessions, pointers, temp) {
        sh->channel = NULL;
        LIST_REMOVE(sh, pointers);
    }
}

UA_SessionHeader *
UA_SecureChannel_getSession(UA_SecureChannel *channel,
                            const UA_NodeId *authenticationToken) {
    UA_SessionHeader *sh;
    LIST_FOREACH(sh, &channel->sessions, pointers) {
        if(UA_NodeId_equal(&sh->authenticationToken, authenticationToken))
            break;
    }
    return sh;
}

/* Sends an OPN message using asymmetric encryption if defined */
UA_StatusCode
UA_SecureChannel_sendAsymmetricOPNMessage(UA_SecureChannel *channel,
                                          UA_UInt32 requestId, const void *content,
                                          const UA_DataType *contentType) {
    if(channel->securityMode == UA_MESSAGESECURITYMODE_INVALID)
        return UA_STATUSCODE_BADSECURITYMODEREJECTED;

    const UA_SecurityPolicy *const securityPolicy = channel->securityPolicy;
    UA_Connection *connection = channel->connection;
    if(!connection)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Allocate the message buffer */
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode retval =
        connection->getSendBuffer(connection, connection->config.sendBufferSize, &buf);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Restrict buffer to the available space for the payload */
    UA_Byte *buf_pos = buf.data;
    const UA_Byte *buf_end = &buf.data[buf.length];
    hideBytesAsym(channel, &buf_pos, &buf_end);

    /* Encode the message type and content */
    UA_NodeId typeId = UA_NODEID_NUMERIC(0, contentType->binaryEncodingId);
    retval |= UA_encodeBinary(&typeId, &UA_TYPES[UA_TYPES_NODEID],
                              &buf_pos, &buf_end, NULL, NULL);
    retval |= UA_encodeBinary(content, contentType,
                              &buf_pos, &buf_end, NULL, NULL);
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
        total_length += securityPolicy->asymmetricModule.cryptoModule.signatureAlgorithm.
            getLocalSignatureSize(securityPolicy, channel->channelContext);

    /* The total message length is known here which is why we encode the headers
     * at this step and not earlier. */
    size_t finalLength = 0;
    retval = prependHeadersAsym(channel, buf.data, buf_end, total_length,
                                securityHeaderLength, requestId, &finalLength);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;

#ifdef UA_ENABLE_ENCRYPTION
    retval = signAndEncryptAsym(channel, pre_sig_length, &buf, securityHeaderLength, total_length);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;
#endif

    /* Send the message, the buffer is freed in the network layer */
    buf.length = finalLength;
    retval = connection->send(connection, &buf);
#ifdef UA_ENABLE_UNIT_TEST_FAILURE_HOOKS
    retval |= sendAsym_sendFailure;
#endif
    return retval;

error:
    connection->releaseSendBuffer(connection, &buf);
    return retval;
}

static UA_StatusCode
checkLimitsSym(UA_MessageContext *const messageContext, size_t *const bodyLength) {
    /* Will this chunk surpass the capacity of the SecureChannel for the message? */
    UA_Connection *const connection = messageContext->channel->connection;
    if(!connection)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_Byte *buf_body_start = messageContext->messageBuffer.data + UA_SECURE_MESSAGE_HEADER_LENGTH;
    const UA_Byte *buf_body_end = messageContext->buf_pos;
    *bodyLength = (uintptr_t)buf_body_end - (uintptr_t)buf_body_start;
    messageContext->messageSizeSoFar += *bodyLength;
    messageContext->chunksSoFar++;

    if(messageContext->messageSizeSoFar > connection->config.maxMessageSize &&
       connection->config.maxMessageSize != 0)
        return UA_STATUSCODE_BADRESPONSETOOLARGE;

    if(messageContext->chunksSoFar > connection->config.maxChunkCount &&
       connection->config.maxChunkCount != 0)
        return UA_STATUSCODE_BADRESPONSETOOLARGE;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
encodeHeadersSym(UA_MessageContext *const messageContext, size_t totalLength) {
    UA_SecureChannel *channel = messageContext->channel;
    UA_Byte *header_pos = messageContext->messageBuffer.data;

    UA_SecureConversationMessageHeader respHeader;
    respHeader.secureChannelId = channel->securityToken.channelId;
    respHeader.messageHeader.messageTypeAndChunkType = messageContext->messageType;
    respHeader.messageHeader.messageSize = (UA_UInt32)totalLength;
    if(messageContext->final)
        respHeader.messageHeader.messageTypeAndChunkType += UA_CHUNKTYPE_FINAL;
    else
        respHeader.messageHeader.messageTypeAndChunkType += UA_CHUNKTYPE_INTERMEDIATE;

    UA_StatusCode res =
        UA_encodeBinary(&respHeader, &UA_TRANSPORT[UA_TRANSPORT_SECURECONVERSATIONMESSAGEHEADER],
                        &header_pos, &messageContext->buf_end, NULL, NULL);

    UA_SymmetricAlgorithmSecurityHeader symSecHeader;
    symSecHeader.tokenId = channel->securityToken.tokenId;
    /* This is a server SecureChannel and we have sent out the OPN response but
     * not gotten a request with the new token. So we want to send with
     * nextSecurityToken and still allow to receive with the old one. */
    if(channel->nextSecurityToken.tokenId != 0)
        symSecHeader.tokenId = channel->nextSecurityToken.tokenId;

    res |= UA_encodeBinary(&symSecHeader.tokenId,
                           &UA_TRANSPORT[UA_TRANSPORT_SYMMETRICALGORITHMSECURITYHEADER],
                           &header_pos, &messageContext->buf_end, NULL, NULL);

    UA_SequenceHeader seqHeader;
    seqHeader.requestId = messageContext->requestId;
    seqHeader.sequenceNumber = UA_atomic_addUInt32(&channel->sendSequenceNumber, 1);
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
    UA_assert(total_length <= connection->config.sendBufferSize);

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

    retval = connection->getSendBuffer(connection, connection->config.sendBufferSize,
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
        connection->getSendBuffer(connection, connection->config.sendBufferSize,
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
    const UA_ConnectionConfig *config = &channel->connection->config;
    UA_assert(config != NULL); /* clang-analyzer false positive */

    if(config->maxChunkCount > 0 &&
       config->maxChunkCount <= latest->chunkPayloadsSize)
        return UA_STATUSCODE_BADRESPONSETOOLARGE;

    if(config->maxMessageSize > 0 &&
       config->maxMessageSize < latest->messageSize + chunkPayload->length)
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
        if(!bytes.data) {
            UA_LOG_ERROR(channel->securityPolicy->logger, UA_LOGCATEGORY_SECURECHANNEL,
                         "Could not allocate the memory to assemble the message");
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
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

UA_StatusCode
UA_SecureChannel_processCompleteMessages(UA_SecureChannel *channel, void *application,
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

/* Sets the payload to a pointer inside the chunk buffer. Returns the requestId
 * and the sequenceNumber */
UA_StatusCode
decryptAndVerifyChunk(const UA_SecureChannel *channel,
                      const UA_SecurityPolicyCryptoModule *cryptoModule,
                      UA_MessageType messageType, const UA_ByteString *chunk,
                      size_t offset, UA_UInt32 *requestId,
                      UA_UInt32 *sequenceNumber, UA_ByteString *payload) {
    size_t chunkSizeAfterDecryption = chunk->length;
    UA_StatusCode retval = decryptChunk(channel, cryptoModule, messageType,
                                        chunk, offset, &chunkSizeAfterDecryption);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Verify the chunk signature */
    size_t sigsize = 0;
    size_t paddingSize = 0;
    const UA_SecurityPolicy *securityPolicy = channel->securityPolicy;
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT ||
       messageType == UA_MESSAGETYPE_OPN) {
        sigsize = cryptoModule->signatureAlgorithm.
            getRemoteSignatureSize(securityPolicy, channel->channelContext);
        paddingSize = decodeChunkPaddingSize(channel, cryptoModule, messageType, chunk,
                                             chunkSizeAfterDecryption, sigsize);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;

        if(offset + paddingSize + sigsize >= chunkSizeAfterDecryption)
            return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

        retval = verifyChunk(channel, cryptoModule, chunk, chunkSizeAfterDecryption, sigsize);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    /* Decode the sequence header */
    UA_SequenceHeader sequenceHeader;
    retval = UA_SequenceHeader_decodeBinary(chunk, &offset, &sequenceHeader);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    if(offset + paddingSize + sigsize >= chunk->length)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    *requestId = sequenceHeader.requestId;
    *sequenceNumber = sequenceHeader.sequenceNumber;
    payload->data = chunk->data + offset;
    payload->length = chunkSizeAfterDecryption - offset - sigsize - paddingSize;
    UA_LOG_TRACE_CHANNEL(channel->securityPolicy->logger, channel,
                         "Decrypted and verified chunk with request id %u and "
                         "sequence number %u", *requestId, *sequenceNumber);
    return UA_STATUSCODE_GOOD;
}

typedef UA_StatusCode
(*UA_SequenceNumberCallback)(UA_SecureChannel *channel, UA_UInt32 sequenceNumber);

static UA_StatusCode
putPayload(UA_SecureChannel *const channel, UA_UInt32 const requestId,
           UA_MessageType const messageType, UA_ChunkType const chunkType,
           UA_ByteString *chunkPayload) {
    switch(chunkType) {
    case UA_CHUNKTYPE_INTERMEDIATE:
    case UA_CHUNKTYPE_FINAL:
        return addChunkPayload(channel, requestId, messageType,
                               chunkPayload, chunkType == UA_CHUNKTYPE_FINAL);
    case UA_CHUNKTYPE_ABORT:
        deleteLatestMessage(channel, requestId);
        return UA_STATUSCODE_GOOD;
    default:
        return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
    }
}

/* The chunk body begins after the SecureConversationMessageHeader */
static UA_StatusCode
decryptAddChunk(UA_SecureChannel *channel, const UA_ByteString *chunk,
                UA_Boolean allowPreviousToken) {
    const UA_SecurityPolicy *sp = channel->securityPolicy;

    /* Decode the MessageHeader */
    size_t offset = 0;
    UA_SecureConversationMessageHeader messageHeader;
    UA_StatusCode retval =
        UA_SecureConversationMessageHeader_decodeBinary(chunk, &offset, &messageHeader);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

#if !defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
    /* The wrong ChannelId. Non-opened channels have the id zero. */
    if(messageHeader.secureChannelId != channel->securityToken.channelId &&
       channel->state != UA_SECURECHANNELSTATE_FRESH)
        return UA_STATUSCODE_BADSECURECHANNELIDINVALID;
#endif

    UA_MessageType messageType = (UA_MessageType)
        (messageHeader.messageHeader.messageTypeAndChunkType & UA_BITMASK_MESSAGETYPE);
    UA_ChunkType chunkType = (UA_ChunkType)
        (messageHeader.messageHeader.messageTypeAndChunkType & UA_BITMASK_CHUNKTYPE);
    UA_ByteString chunkPayload;

    switch(messageType) {
        /* ERR message (not encrypted) */
    case UA_MESSAGETYPE_ERR:
        if(chunkType != UA_CHUNKTYPE_FINAL)
            return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
        chunkPayload.length = chunk->length - offset;
        chunkPayload.data = chunk->data + offset;
        return putPayload(channel, 0, messageType, chunkType, &chunkPayload);

        /* MSG and CLO: Symmetric encryption */
    case UA_MESSAGETYPE_MSG:
    case UA_MESSAGETYPE_CLO: {
        /* Decode and check the symmetric security header (tokenId) */
        UA_SymmetricAlgorithmSecurityHeader symmetricSecurityHeader;
        UA_SymmetricAlgorithmSecurityHeader_init(&symmetricSecurityHeader);
        retval = UA_SymmetricAlgorithmSecurityHeader_decodeBinary(chunk, &offset,
                                                                  &symmetricSecurityHeader);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;

#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
        /* Help fuzzing by always setting the correct tokenId */
        symmetricSecurityHeader.tokenId = channel->securityToken.tokenId;
#endif

        retval = checkSymHeader(channel, symmetricSecurityHeader.tokenId, allowPreviousToken);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING_CHANNEL(sp->logger, channel, "Could not validate the chunk header");
            return retval;
        }

        UA_UInt32 requestId = 0;
        UA_UInt32 sequenceNumber = 0;
        retval = decryptAndVerifyChunk(channel, &sp->symmetricModule.cryptoModule, messageType,
                                       chunk, offset, &requestId, &sequenceNumber, &chunkPayload);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING_CHANNEL(sp->logger, channel, "Could not decrypt and verify the chunk payload");
            return retval;
        }

        /* Check the sequence number. Skip sequence number checking for fuzzer to
         * improve coverage */
#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
        retval = processSequenceNumberSym(channel, sequenceNumber);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING_CHANNEL(sp->logger, channel,
                                   "Processing the sequence number failed");
            return retval;
        }
#endif

        return putPayload(channel, requestId, messageType, chunkType, &chunkPayload);
    }

        /* OPN: Asymmetric encryption */
    case UA_MESSAGETYPE_OPN: {
        /* Chunking not allowed for OPN */
        if(chunkType != UA_CHUNKTYPE_FINAL)
            return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;

        return putPayload(channel, 0, messageType, chunkType, (UA_ByteString*)(uintptr_t)chunk);
    }

        /* Invalid message type */
    default:
        break;
    }

    return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
}

UA_StatusCode
UA_SecureChannel_decryptAddChunk(UA_SecureChannel *channel, const UA_ByteString *chunk,
                                 UA_Boolean allowPreviousToken) {
    /* Has the SecureChannel timed out? */
    if(channel->state == UA_SECURECHANNELSTATE_CLOSED)
        return UA_STATUSCODE_BADSECURECHANNELCLOSED;

    /* Is the SecureChannel configured? */
    if(!channel->connection)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode retval = decryptAddChunk(channel, chunk, allowPreviousToken);
    if(retval != UA_STATUSCODE_GOOD)
        UA_SecureChannel_close(channel);

    return retval;
}

UA_StatusCode
UA_SecureChannel_persistIncompleteMessages(UA_SecureChannel *channel) {
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

/* Functionality used by both the SecureChannel and the SecurityPolicy */

size_t
UA_SecurityPolicy_getRemoteAsymEncryptionBufferLengthOverhead(const UA_SecurityPolicy *securityPolicy,
                                                              const void *channelContext,
                                                              size_t maxEncryptionLength) {
    if(maxEncryptionLength == 0)
        return 0;

    size_t plainTextBlockSize = securityPolicy->asymmetricModule.cryptoModule.
        encryptionAlgorithm.getRemotePlainTextBlockSize(securityPolicy, channelContext);
    size_t encryptedBlockSize = securityPolicy->asymmetricModule.cryptoModule.
        encryptionAlgorithm.getRemoteBlockSize(securityPolicy, channelContext);
    if(plainTextBlockSize == 0)
        return 0;

    size_t maxNumberOfBlocks = maxEncryptionLength / plainTextBlockSize;
    return maxNumberOfBlocks * (encryptedBlockSize - plainTextBlockSize);
}
