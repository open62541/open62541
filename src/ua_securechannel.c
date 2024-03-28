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

#include <open62541/types_generated_handling.h>
#include <open62541/transport_generated_handling.h>

#include "ua_securechannel.h"
#include "ua_types_encoding_binary.h"

#define UA_BITMASK_MESSAGETYPE 0x00ffffffu
#define UA_BITMASK_CHUNKTYPE 0xff000000u

const UA_String UA_SECURITY_POLICY_NONE_URI =
    {47, (UA_Byte *)"http://opcfoundation.org/UA/SecurityPolicy#None"};

void
UA_SecureChannel_init(UA_SecureChannel *channel) {
    /* Normal linked lists are initialized by zeroing out */
    memset(channel, 0, sizeof(UA_SecureChannel));
    SIMPLEQ_INIT(&channel->completeChunks);
    SIMPLEQ_INIT(&channel->decryptedChunks);
}

UA_StatusCode
UA_SecureChannel_setSecurityPolicy(UA_SecureChannel *channel,
                                   const UA_SecurityPolicy *securityPolicy,
                                   const UA_ByteString *remoteCertificate) {
    /* Is a policy already configured? */
    UA_CHECK_ERROR(!channel->securityPolicy, return UA_STATUSCODE_BADINTERNALERROR,
                   securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                   "Security policy already configured");

    /* Create the context */
    UA_StatusCode res = securityPolicy->channelModule.
        newContext(securityPolicy, remoteCertificate, &channel->channelContext);
    res |= UA_ByteString_copy(remoteCertificate, &channel->remoteCertificate);
    UA_CHECK_STATUS_WARN(res, return res, securityPolicy->logger,
                         UA_LOGCATEGORY_SECURITYPOLICY,
                         "Could not set up the SecureChannel context");

    /* Compute the certificate thumbprint */
    UA_ByteString remoteCertificateThumbprint =
        {20, channel->remoteCertificateThumbprint};
    res = securityPolicy->asymmetricModule.
        makeCertificateThumbprint(securityPolicy, &channel->remoteCertificate,
                                  &remoteCertificateThumbprint);
    UA_CHECK_STATUS_WARN(res, return res, securityPolicy->logger,
                         UA_LOGCATEGORY_SECURITYPOLICY,
                         "Could not create the certificate thumbprint");

    /* Set the policy */
    channel->securityPolicy = securityPolicy;
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

UA_Boolean
UA_SecureChannel_isConnected(UA_SecureChannel *channel) {
    return (channel->state > UA_SECURECHANNELSTATE_CLOSED &&
            channel->state < UA_SECURECHANNELSTATE_CLOSING);
}

void
UA_SecureChannel_sendError(UA_SecureChannel *channel, UA_TcpErrorMessage *error) {
    if(!UA_SecureChannel_isConnected(channel))
        return;

    hideErrors(error);

    UA_TcpMessageHeader header;
    header.messageTypeAndChunkType = UA_MESSAGETYPE_ERR + UA_CHUNKTYPE_FINAL;
    /* Header + ErrorMessage (error + reasonLength_field + length) */
    header.messageSize = 8 + (4 + 4 + (UA_UInt32)error->reason.length);

    /* Get the send buffer from the network layer */
    UA_ConnectionManager *cm = channel->connectionManager;
    UA_ByteString msg = UA_BYTESTRING_NULL;
    UA_StatusCode retval = cm->allocNetworkBuffer(cm, channel->connectionId,
                                                  &msg, header.messageSize);
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
    cm->sendWithConnection(cm, channel->connectionId, &UA_KEYVALUEMAP_NULL, &msg);
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
UA_SecureChannel_shutdown(UA_SecureChannel *channel,
                          UA_ShutdownReason shutdownReason) {
    /* No open socket or already closing -> nothing to do */
    if(!UA_SecureChannel_isConnected(channel))
        return;

    /* Set the shutdown event for diagnostics */
    channel->shutdownReason= shutdownReason;

    /* Trigger the async closing of the connection */
    UA_ConnectionManager *cm = channel->connectionManager;
    cm->closeConnection(cm, channel->connectionId);
    channel->state = UA_SECURECHANNELSTATE_CLOSING;
}

void
UA_SecureChannel_clear(UA_SecureChannel *channel) {
    /* No sessions must be attached to this any longer */
    UA_assert(channel->sessions == NULL);

    /* Delete the channel context for the security policy */
    if(channel->securityPolicy) {
        channel->securityPolicy->channelModule.deleteContext(channel->channelContext);
        channel->securityPolicy = NULL;
        channel->channelContext = NULL;
    }

    /* The EventLoop connection is no longer valid */
    channel->connectionId = 0;
    channel->connectionManager = NULL;

    /* Clean up the SecurityToken */
    UA_ChannelSecurityToken_clear(&channel->securityToken);
    UA_ChannelSecurityToken_clear(&channel->altSecurityToken);

    /* Clean up certificate and nonces */
    UA_ByteString_clear(&channel->remoteCertificate);
    UA_ByteString_clear(&channel->localNonce);
    UA_ByteString_clear(&channel->remoteNonce);

    /* Clean up endpointUrl */
    UA_String_clear(&channel->endpointUrl);

    /* Delete remaining chunks */
    UA_SecureChannel_deleteBuffered(channel);

    /* Reset the SecureChannel for reuse (in the client) */
    channel->securityMode = UA_MESSAGESECURITYMODE_INVALID;
    channel->shutdownReason = UA_SHUTDOWNREASON_CLOSE;
    memset(&channel->config, 0, sizeof(UA_ConnectionConfig));
    channel->receiveSequenceNumber = 0;
    channel->sendSequenceNumber = 0;

    /* Set the state to closed */
    channel->state = UA_SECURECHANNELSTATE_CLOSED;
    channel->renewState = UA_SECURECHANNELRENEWSTATE_NORMAL;
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

    return UA_STATUSCODE_GOOD;
}

/* Sends an OPN message using asymmetric encryption if defined */
UA_StatusCode
UA_SecureChannel_sendAsymmetricOPNMessage(UA_SecureChannel *channel,
                                          UA_UInt32 requestId, const void *content,
                                          const UA_DataType *contentType) {
    UA_CHECK(channel->securityMode != UA_MESSAGESECURITYMODE_INVALID,
             return UA_STATUSCODE_BADSECURITYMODEREJECTED);

    /* Can we use the connection manager? */
    UA_ConnectionManager *cm = channel->connectionManager;
    if(!UA_SecureChannel_isConnected(channel))
        return UA_STATUSCODE_BADCONNECTIONCLOSED;

    const UA_SecurityPolicy *sp = channel->securityPolicy;
    UA_CHECK_MEM(sp, return UA_STATUSCODE_BADINTERNALERROR);

    /* Allocate the message buffer */
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = cm->allocNetworkBuffer(cm, channel->connectionId, &buf,
                                               channel->config.sendBufferSize);
    UA_CHECK_STATUS(res, return res);

    /* Restrict buffer to the available space for the payload */
    UA_Byte *buf_pos = buf.data;
    const UA_Byte *buf_end = &buf.data[buf.length];
    hideBytesAsym(channel, &buf_pos, &buf_end);

    /* Define variables here to pacify some compilers wrt goto */
    size_t securityHeaderLength, pre_sig_length, total_length, encryptedLength;

    /* Encode the message type and content */
    res |= UA_NodeId_encodeBinary(&contentType->binaryEncodingId, &buf_pos, buf_end);
    res |= UA_encodeBinaryInternal(content, contentType, &buf_pos, &buf_end, NULL, NULL);
    UA_CHECK_STATUS(res, goto error);

    /* Compute the header length */
    securityHeaderLength = calculateAsymAlgSecurityHeaderLength(channel);

    /* Add padding to the chunk. Also pad if the securityMode is SIGN_ONLY,
     * since we are using asymmetric communication to exchange keys and thus
     * need to encrypt. */
    if(channel->securityMode != UA_MESSAGESECURITYMODE_NONE)
        padChunk(channel, &channel->securityPolicy->asymmetricModule.cryptoModule,
                 &buf.data[UA_SECURECHANNEL_CHANNELHEADER_LENGTH + securityHeaderLength],
                 &buf_pos);

    /* The total message length */
    pre_sig_length = (uintptr_t)buf_pos - (uintptr_t)buf.data;
    total_length = pre_sig_length;
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        total_length += sp->asymmetricModule.cryptoModule.signatureAlgorithm.
            getLocalSignatureSize(channel->channelContext);

    /* The total message length is known here which is why we encode the headers
     * at this step and not earlier. */
    res = prependHeadersAsym(channel, buf.data, buf_end, total_length,
                             securityHeaderLength, requestId, &encryptedLength);
    UA_CHECK_STATUS(res, goto error);

    res = signAndEncryptAsym(channel, pre_sig_length, &buf,
                             securityHeaderLength, total_length);
    UA_CHECK_STATUS(res, goto error);

    /* Send the message, the buffer is freed in the network layer */
    buf.length = encryptedLength;
    return cm->sendWithConnection(cm, channel->connectionId, &UA_KEYVALUEMAP_NULL, &buf);

 error:
    cm->freeNetworkBuffer(cm, channel->connectionId, &buf);
    return res;
}

/* Will this chunk surpass the capacity of the SecureChannel for the message? */
static UA_StatusCode
adjustCheckMessageLimitsSym(UA_MessageContext *mc, size_t bodyLength) {
    mc->messageSizeSoFar += bodyLength;
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
encodeHeadersSym(UA_MessageContext *mc, size_t totalLength) {
    UA_SecureChannel *channel = mc->channel;
    UA_Byte *header_pos = mc->messageBuffer.data;

    UA_TcpMessageHeader header;
    header.messageTypeAndChunkType = mc->messageType;
    header.messageSize = (UA_UInt32)totalLength;
    if(mc->final)
        header.messageTypeAndChunkType += UA_CHUNKTYPE_FINAL;
    else
        header.messageTypeAndChunkType += UA_CHUNKTYPE_INTERMEDIATE;

    /* Increase the sequence number in the channel */
    channel->sendSequenceNumber++;

    UA_SequenceHeader seqHeader;
    seqHeader.requestId = mc->requestId;
    seqHeader.sequenceNumber = channel->sendSequenceNumber;

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    res |= UA_encodeBinaryInternal(&header, &UA_TRANSPORT[UA_TRANSPORT_TCPMESSAGEHEADER],
                                   &header_pos, &mc->buf_end, NULL, NULL);
    res |= UA_UInt32_encodeBinary(&channel->securityToken.channelId,
                                  &header_pos, mc->buf_end);
    res |= UA_UInt32_encodeBinary(&channel->securityToken.tokenId,
                                  &header_pos, mc->buf_end);
    res |= UA_encodeBinaryInternal(&seqHeader, &UA_TRANSPORT[UA_TRANSPORT_SEQUENCEHEADER],
                                   &header_pos, &mc->buf_end, NULL, NULL);
    return res;
}

static UA_StatusCode
sendSymmetricChunk(UA_MessageContext *mc) {
    UA_SecureChannel *channel = mc->channel;
    const UA_SecurityPolicy *sp = channel->securityPolicy;
    UA_ConnectionManager *cm = channel->connectionManager;
    if(!UA_SecureChannel_isConnected(channel))
        return UA_STATUSCODE_BADCONNECTIONCLOSED;

    /* The size of the message payload */
    size_t bodyLength = (uintptr_t)mc->buf_pos -
        (uintptr_t)&mc->messageBuffer.data[UA_SECURECHANNEL_SYMMETRIC_HEADER_TOTALLENGTH];

    /* Early-declare variables so we can use a goto in the error case */
    size_t total_length = 0;
    size_t pre_sig_length = 0;

    /* Check if chunk exceeds the limits for the overall message */
    UA_StatusCode res = adjustCheckMessageLimitsSym(mc, bodyLength);
    UA_CHECK_STATUS(res, goto error);

    UA_LOG_TRACE_CHANNEL(sp->logger, channel,
                         "Send from a symmetric message buffer of length %lu "
                         "a message of header+payload length of %lu",
                         (long unsigned int)mc->messageBuffer.length,
                         (long unsigned int)
                         ((uintptr_t)mc->buf_pos - (uintptr_t)mc->messageBuffer.data));

    /* Add padding if the message is encrypted */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        padChunk(channel, &sp->symmetricModule.cryptoModule,
                 &mc->messageBuffer.data[UA_SECURECHANNEL_SYMMETRIC_HEADER_UNENCRYPTEDLENGTH],
                 &mc->buf_pos);

    /* Compute the total message length */
    pre_sig_length = (uintptr_t)mc->buf_pos - (uintptr_t)mc->messageBuffer.data;
    total_length = pre_sig_length;
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        total_length += sp->symmetricModule.cryptoModule.signatureAlgorithm.
            getLocalSignatureSize(channel->channelContext);

    UA_LOG_TRACE_CHANNEL(sp->logger, channel,
                         "Send from a symmetric message buffer of length %lu "
                         "a message of length %lu",
                         (long unsigned int)mc->messageBuffer.length,
                         (long unsigned int)total_length);

    /* Space for the padding and the signature have been reserved in setBufPos() */
    UA_assert(total_length <= channel->config.sendBufferSize);

    /* Adjust the buffer size of the network layer */
    mc->messageBuffer.length = total_length;

    /* Generate and encode the header for symmetric messages */
    res = encodeHeadersSym(mc, total_length);
    UA_CHECK_STATUS(res, goto error);

    /* Sign and encrypt the messge */
    res = signAndEncryptSym(mc, pre_sig_length, total_length);
    UA_CHECK_STATUS(res, goto error);

    /* Send the chunk. The buffer is freed in the network layer. If sending goes
     * wrong, the connection is removed in the next iteration of the
     * SecureChannel. Set the SecureChannel to closing already. */
    res = cm->sendWithConnection(cm, channel->connectionId,
                                 &UA_KEYVALUEMAP_NULL, &mc->messageBuffer);
    if(res != UA_STATUSCODE_GOOD && UA_SecureChannel_isConnected(channel))
        channel->state = UA_SECURECHANNELSTATE_CLOSING;

 error:
    /* Free the unused message buffer */
    cm->freeNetworkBuffer(cm, channel->connectionId, &mc->messageBuffer);
    return res;
}

/* Callback from the encoding layer. Send the chunk and replace the buffer. */
static UA_StatusCode
sendSymmetricEncodingCallback(void *data, UA_Byte **buf_pos,
                              const UA_Byte **buf_end) {
    /* Set buf values from encoding in the messagecontext */
    UA_MessageContext *mc = (UA_MessageContext *)data;
    mc->buf_pos = *buf_pos;
    mc->buf_end = *buf_end;

    /* Send out */
    UA_StatusCode res = sendSymmetricChunk(mc);
    UA_CHECK_STATUS(res, return res);

    /* Set a new buffer for the next chunk */
    UA_ConnectionManager *cm = mc->channel->connectionManager;
    if(!UA_SecureChannel_isConnected(mc->channel))
        return UA_STATUSCODE_BADCONNECTIONCLOSED;

    res = cm->allocNetworkBuffer(cm, mc->channel->connectionId,
                                 &mc->messageBuffer,
                                 mc->channel->config.sendBufferSize);
    UA_CHECK_STATUS(res, return res);

    /* Hide bytes for header, padding and signature */
    setBufPos(mc);
    *buf_pos = mc->buf_pos;
    *buf_end = mc->buf_end;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_MessageContext_begin(UA_MessageContext *mc, UA_SecureChannel *channel,
                        UA_UInt32 requestId, UA_MessageType messageType) {
    UA_CHECK(messageType == UA_MESSAGETYPE_MSG || messageType == UA_MESSAGETYPE_CLO,
             return UA_STATUSCODE_BADINTERNALERROR);

    UA_ConnectionManager *cm = channel->connectionManager;
    if(!UA_SecureChannel_isConnected(channel))
        return UA_STATUSCODE_BADCONNECTIONCLOSED;

    /* Create the chunking info structure */
    mc->channel = channel;
    mc->requestId = requestId;
    mc->chunksSoFar = 0;
    mc->messageSizeSoFar = 0;
    mc->final = false;
    mc->messageBuffer = UA_BYTESTRING_NULL;
    mc->messageType = messageType;

    /* Allocate the message buffer */
    UA_StatusCode res =
        cm->allocNetworkBuffer(cm, channel->connectionId,
                               &mc->messageBuffer,
                               channel->config.sendBufferSize);
    UA_CHECK_STATUS(res, return res);

    /* Hide bytes for header, padding and signature */
    setBufPos(mc);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_MessageContext_encode(UA_MessageContext *mc, const void *content,
                         const UA_DataType *contentType) {
    UA_StatusCode res =
        UA_encodeBinaryInternal(content, contentType, &mc->buf_pos, &mc->buf_end,
                                sendSymmetricEncodingCallback, mc);
    if(res != UA_STATUSCODE_GOOD && mc->messageBuffer.length > 0)
        UA_MessageContext_abort(mc);
    return res;
}

UA_StatusCode
UA_MessageContext_finish(UA_MessageContext *mc) {
    mc->final = true;
    return sendSymmetricChunk(mc);
}

void
UA_MessageContext_abort(UA_MessageContext *mc) {
    UA_ConnectionManager *cm = mc->channel->connectionManager;
    if(!UA_SecureChannel_isConnected(mc->channel))
        return;
    cm->freeNetworkBuffer(cm, mc->channel->connectionId, &mc->messageBuffer);
}

UA_StatusCode
UA_SecureChannel_sendSymmetricMessage(UA_SecureChannel *channel, UA_UInt32 requestId,
                                      UA_MessageType messageType, void *payload,
                                      const UA_DataType *payloadType) {
    if(!channel || !payload || !payloadType)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(channel->state != UA_SECURECHANNELSTATE_OPEN)
        return UA_STATUSCODE_BADCONNECTIONCLOSED;

    UA_MessageContext mc;
    UA_StatusCode res = UA_MessageContext_begin(&mc, channel, requestId, messageType);
    UA_CHECK_STATUS(res, return res);

    /* Assert's required for clang-analyzer */
    UA_assert(mc.buf_pos ==
              &mc.messageBuffer.data[UA_SECURECHANNEL_SYMMETRIC_HEADER_TOTALLENGTH]);
    UA_assert(mc.buf_end <= &mc.messageBuffer.data[mc.messageBuffer.length]);

    res = UA_MessageContext_encode(&mc, &payloadType->binaryEncodingId,
                                   &UA_TYPES[UA_TYPES_NODEID]);
    UA_CHECK_STATUS(res, return res);

    res = UA_MessageContext_encode(&mc, payload, payloadType);
    UA_CHECK_STATUS(res, return res);

    return UA_MessageContext_finish(&mc);
}

/********************************/
/* Receive and Process Messages */
/********************************/

/* Does the sequence number match? Otherwise try to rollover. See Part 6,
 * Section 6.7.2.4 of the standard. */
#define UA_SEQUENCENUMBER_ROLLOVER 4294966271

#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
static UA_StatusCode
processSequenceNumberSym(UA_SecureChannel *channel, UA_UInt32 sequenceNumber) {
    if(sequenceNumber != channel->receiveSequenceNumber + 1) {
        if(channel->receiveSequenceNumber + 1 <= UA_SEQUENCENUMBER_ROLLOVER ||
           sequenceNumber >= 1024)
            return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        channel->receiveSequenceNumber = sequenceNumber - 1; /* Roll over */
    }
    ++channel->receiveSequenceNumber;
    return UA_STATUSCODE_GOOD;
}
#endif

static UA_StatusCode
unpackPayloadOPN(UA_SecureChannel *channel, UA_Chunk *chunk, void *application) {
    UA_assert(chunk->bytes.length >= UA_SECURECHANNEL_MESSAGE_MIN_LENGTH);
    size_t offset = UA_SECURECHANNEL_MESSAGEHEADER_LENGTH; /* Skip the message header */
    UA_UInt32 secureChannelId;
    UA_StatusCode res = UA_UInt32_decodeBinary(&chunk->bytes, &offset, &secureChannelId);
    UA_assert(res == UA_STATUSCODE_GOOD);

    UA_AsymmetricAlgorithmSecurityHeader asymHeader;
    res = UA_decodeBinaryInternal(&chunk->bytes, &offset, &asymHeader,
             &UA_TRANSPORT[UA_TRANSPORT_ASYMMETRICALGORITHMSECURITYHEADER], NULL);
    UA_CHECK_STATUS(res, return res);

    if(asymHeader.senderCertificate.length > 0) {
        if(channel->certificateVerification)
            res = channel->certificateVerification->
                verifyCertificate(channel->certificateVerification,
                                  &asymHeader.senderCertificate);
        else
            res = UA_STATUSCODE_BADINTERNALERROR;
        UA_CHECK_STATUS(res, goto error);
    }

    /* New channel, create a security policy context and attach */
    if(!channel->securityPolicy) {
        if(channel->processOPNHeader)
            res = channel->processOPNHeader(application, channel, &asymHeader);
        if(!channel->securityPolicy)
            res = UA_STATUSCODE_BADINTERNALERROR;
        UA_CHECK_STATUS(res, goto error);
    }

    /* On the client side, take the SecureChannelId from the first response */
    if(secureChannelId != 0 && channel->securityToken.channelId == 0)
        channel->securityToken.channelId = secureChannelId;

    /* Check the ChannelId */
#if !defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
    if(secureChannelId != channel->securityToken.channelId) {
        /* Allow the channel id to be different if the sent channel id is zero
         * and the SecurityToken is not initialized. This only happens on the
         * server side before we had a chance to tell the client which ChannelId
         * to use. */
        if(secureChannelId != 0 || channel->securityToken.tokenId != 0) {
            res = UA_STATUSCODE_BADSECURECHANNELIDINVALID;
            goto error;
        }
    }
#endif

    /* Check the header for the channel's security policy */
    res = checkAsymHeader(channel, &asymHeader);
    UA_AsymmetricAlgorithmSecurityHeader_clear(&asymHeader);
    UA_CHECK_STATUS(res, return res);

    /* Decrypt the chunk payload */
    res = decryptAndVerifyChunk(channel,
                                &channel->securityPolicy->asymmetricModule.cryptoModule,
                                chunk->messageType, &chunk->bytes, offset);
    UA_CHECK_STATUS(res, return res);

    /* Decode the SequenceHeader */
    UA_SequenceHeader sequenceHeader;
    res = UA_decodeBinaryInternal(&chunk->bytes, &offset, &sequenceHeader,
                                  &UA_TRANSPORT[UA_TRANSPORT_SEQUENCEHEADER], NULL);
    UA_CHECK_STATUS(res, return res);

    /* Set the sequence number for the channel from which to count up */
    channel->receiveSequenceNumber = sequenceHeader.sequenceNumber;
    chunk->requestId = sequenceHeader.requestId; /* Set the RequestId of the chunk */

    /* Use only the payload */
    chunk->bytes.data += offset;
    chunk->bytes.length -= offset;
    return UA_STATUSCODE_GOOD;

error:
    UA_AsymmetricAlgorithmSecurityHeader_clear(&asymHeader);
    return res;
}

static UA_StatusCode
unpackPayloadMSG(UA_SecureChannel *channel, UA_Chunk *chunk,
                 UA_DateTime nowMonotonic) {
    UA_CHECK_MEM(channel->securityPolicy, return UA_STATUSCODE_BADINTERNALERROR);

    UA_assert(chunk->bytes.length >= UA_SECURECHANNEL_MESSAGE_MIN_LENGTH);
    size_t offset = UA_SECURECHANNEL_MESSAGEHEADER_LENGTH; /* Skip the message header */
    UA_UInt32 secureChannelId;
    UA_UInt32 tokenId; /* SymmetricAlgorithmSecurityHeader */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    res |= UA_UInt32_decodeBinary(&chunk->bytes, &offset, &secureChannelId);
    res |= UA_UInt32_decodeBinary(&chunk->bytes, &offset, &tokenId);
    UA_assert(offset == UA_SECURECHANNEL_MESSAGE_MIN_LENGTH);
    UA_assert(res == UA_STATUSCODE_GOOD);

#if !defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
    /* Check the ChannelId. Non-opened channels have the id zero. */
    if(secureChannelId != channel->securityToken.channelId)
        return UA_STATUSCODE_BADSECURECHANNELIDINVALID;
#endif

    /* Check (and revolve) the SecurityToken */
    res = checkSymHeader(channel, tokenId, nowMonotonic);
    UA_CHECK_STATUS(res, return res);

    /* Decrypt the chunk payload */
    res = decryptAndVerifyChunk(channel,
                                &channel->securityPolicy->symmetricModule.cryptoModule,
                                chunk->messageType, &chunk->bytes, offset);
    UA_CHECK_STATUS(res, return res);

    /* Check the sequence number. Skip sequence number checking for fuzzer to
     * improve coverage */
    UA_SequenceHeader sequenceHeader;
    res = UA_decodeBinaryInternal(&chunk->bytes, &offset, &sequenceHeader,
                                  &UA_TRANSPORT[UA_TRANSPORT_SEQUENCEHEADER], NULL);
#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    res |= processSequenceNumberSym(channel, sequenceHeader.sequenceNumber);
#endif
    UA_CHECK_STATUS(res, return res);

    chunk->requestId = sequenceHeader.requestId; /* Set the RequestId of the chunk */

    /* Use only the payload */
    chunk->bytes.data += offset;
    chunk->bytes.length -= offset;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
assembleProcessMessage(UA_SecureChannel *channel, void *application,
                       UA_ProcessMessageCallback callback) {
    UA_Chunk *chunk = SIMPLEQ_FIRST(&channel->decryptedChunks);
    UA_assert(chunk != NULL);

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(chunk->chunkType == UA_CHUNKTYPE_FINAL) {
        SIMPLEQ_REMOVE_HEAD(&channel->decryptedChunks, pointers);
        UA_assert(chunk->chunkType == UA_CHUNKTYPE_FINAL);
        res = callback(application, channel, chunk->messageType,
                       chunk->requestId, &chunk->bytes);
        UA_Chunk_delete(chunk);
        return res;
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
    res = UA_ByteString_allocBuffer(&payload, messageSize);
    UA_CHECK_STATUS(res, return res);

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
    res = callback(application, channel, messageType, requestId, &payload);
    UA_ByteString_clear(&payload);
    return res;
}

static UA_StatusCode
persistCompleteChunks(UA_ChunkQueue *queue) {
    UA_Chunk *chunk;
    SIMPLEQ_FOREACH(chunk, queue, pointers) {
        if(chunk->copied)
            continue;
        UA_ByteString copy;
        UA_StatusCode res = UA_ByteString_copy(&chunk->bytes, &copy);
        UA_CHECK_STATUS(res, return res);
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
    UA_StatusCode res = UA_ByteString_allocBuffer(&channel->incompleteChunk, length);
    UA_CHECK_STATUS(res, return res);
    memcpy(channel->incompleteChunk.data, &buffer->data[offset], length);
    return UA_STATUSCODE_GOOD;
}

/* Processes chunks and puts them into the payloads queue. Once a final chunk is
 * put into the queue, the message is assembled and the callback is called. The
 * queue will be cleared for the next message. */
static UA_StatusCode
processChunks(UA_SecureChannel *channel, void *application,
              UA_ProcessMessageCallback callback,
              UA_DateTime nowMonotonic) {
    UA_Chunk *chunk;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    while((chunk = SIMPLEQ_FIRST(&channel->completeChunks))) {
        /* Remove from the complete-chunk queue */
        SIMPLEQ_REMOVE_HEAD(&channel->completeChunks, pointers);

        /* Check, decrypt and unpack the payload */
        if(chunk->messageType == UA_MESSAGETYPE_OPN) {
            if(channel->state != UA_SECURECHANNELSTATE_OPEN &&
               channel->state != UA_SECURECHANNELSTATE_OPN_SENT &&
               channel->state != UA_SECURECHANNELSTATE_ACK_SENT)
                res = UA_STATUSCODE_BADINVALIDSTATE;
            else
                res = unpackPayloadOPN(channel, chunk, application);
        } else if(chunk->messageType == UA_MESSAGETYPE_MSG ||
                  chunk->messageType == UA_MESSAGETYPE_CLO) {
            if(channel->state == UA_SECURECHANNELSTATE_CLOSED)
                res = UA_STATUSCODE_BADSECURECHANNELCLOSED;
            else
                res = unpackPayloadMSG(channel, chunk, nowMonotonic);
        } else {
            chunk->bytes.data += UA_SECURECHANNEL_MESSAGEHEADER_LENGTH;
            chunk->bytes.length -= UA_SECURECHANNEL_MESSAGEHEADER_LENGTH;
        }

        if(res != UA_STATUSCODE_GOOD) {
            UA_Chunk_delete(chunk);
            return res;
        }

        /* Add to the decrypted-chunk queue */
        SIMPLEQ_INSERT_TAIL(&channel->decryptedChunks, chunk, pointers);

        /* Check the resource limits */
        channel->decryptedChunksCount++;
        channel->decryptedChunksLength += chunk->bytes.length;
        if((channel->config.localMaxChunkCount != 0 &&
            channel->decryptedChunksCount > channel->config.localMaxChunkCount) ||
           (channel->config.localMaxMessageSize != 0 &&
            channel->decryptedChunksLength > channel->config.localMaxMessageSize)) {
            return UA_STATUSCODE_BADTCPMESSAGETOOLARGE;
        }

        /* Waiting for additional chunks */
        if(chunk->chunkType == UA_CHUNKTYPE_INTERMEDIATE)
            continue;

        /* Final chunk or abort. Reset the counters. */
        channel->decryptedChunksCount = 0;
        channel->decryptedChunksLength = 0;

        /* Abort the message, remove all decrypted chunks
         * TODO: Log a warning with the error code */
        if(chunk->chunkType == UA_CHUNKTYPE_ABORT) {
            while((chunk = SIMPLEQ_FIRST(&channel->decryptedChunks))) {
                SIMPLEQ_REMOVE_HEAD(&channel->decryptedChunks, pointers);
                UA_Chunk_delete(chunk);
            }
            continue;
        }

        /* The decrypted queue contains a full message. Process it. */
        UA_assert(chunk->chunkType == UA_CHUNKTYPE_FINAL);
        res = assembleProcessMessage(channel, application, callback);
        UA_CHECK_STATUS(res, return res);
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
extractCompleteChunk(UA_SecureChannel *channel, const UA_ByteString *buffer,
                     size_t *offset, UA_Boolean *done) {
    /* At least 8 byte needed for the header. Wait for the next chunk. */
    size_t initial_offset = *offset;
    size_t remaining = buffer->length - initial_offset;
    if(remaining < UA_SECURECHANNEL_MESSAGEHEADER_LENGTH) {
        *done = true;
        return UA_STATUSCODE_GOOD;
    }

    /* Decoding cannot fail */
    UA_TcpMessageHeader hdr;
    UA_StatusCode res =
        UA_decodeBinaryInternal(buffer, &initial_offset, &hdr,
                                &UA_TRANSPORT[UA_TRANSPORT_TCPMESSAGEHEADER], NULL);
    UA_assert(res == UA_STATUSCODE_GOOD);
    (void)res; /* pacify compilers if assert is ignored */
    UA_MessageType msgType = (UA_MessageType)
        (hdr.messageTypeAndChunkType & UA_BITMASK_MESSAGETYPE);
    UA_ChunkType chunkType = (UA_ChunkType)
        (hdr.messageTypeAndChunkType & UA_BITMASK_CHUNKTYPE);

    /* The message size is not allowed */
    if(hdr.messageSize < UA_SECURECHANNEL_MESSAGE_MIN_LENGTH)
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

    if(msgType == UA_MESSAGETYPE_RHE || msgType == UA_MESSAGETYPE_HEL || msgType == UA_MESSAGETYPE_ACK ||
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
    UA_Chunk *chunk = (UA_Chunk*)UA_malloc(sizeof(UA_Chunk));
    UA_CHECK_MEM(chunk, return UA_STATUSCODE_BADOUTOFMEMORY);

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
                               const UA_ByteString *buffer,
                               UA_DateTime nowMonotonic) {
    /* Prepend the incomplete last chunk. This is usually done in the
     * networklayer. But we test for a buffered incomplete chunk here again to
     * work around "lazy" network layers. */
    UA_ByteString appended = channel->incompleteChunk;
    if(appended.length > 0) {
        channel->incompleteChunk = UA_BYTESTRING_NULL;
        UA_Byte *t = (UA_Byte*)UA_realloc(appended.data, appended.length + buffer->length);
        UA_CHECK_MEM(t, UA_ByteString_clear(&appended);
                     return UA_STATUSCODE_BADOUTOFMEMORY);
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
        UA_CHECK_STATUS(res, goto cleanup);
    }

    /* Buffer half-received chunk. Before processing the messages so that
     * processing is reentrant. */
    if(offset < buffer->length) {
        res = persistIncompleteChunk(channel, buffer, offset);
        UA_CHECK_STATUS(res, goto cleanup);
    }

    /* Process whatever we can. Chunks of completed and processed messages are
     * removed. */
    res = processChunks(channel, application, callback, nowMonotonic);
    UA_CHECK_STATUS(res, goto cleanup);

    /* Persist full chunks that still point to the buffer. Can only return
     * UA_STATUSCODE_BADOUTOFMEMORY as an error code. So merging res works. */
    res |= persistCompleteChunks(&channel->completeChunks);
    res |= persistCompleteChunks(&channel->decryptedChunks);

 cleanup:
    UA_ByteString_clear(&appended);
    return res;
}
