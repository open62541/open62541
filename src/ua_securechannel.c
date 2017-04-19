/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_util.h"
#include "ua_securechannel.h"
#include "ua_session.h"
#include "ua_types_encoding_binary.h"
#include "ua_types_generated_encoding_binary.h"
#include "ua_transport_generated_encoding_binary.h"
#include "ua_types_generated_handling.h"
#include "ua_transport_generated_handling.h"
#include "ua_securitycontext.h"
#include "ua_securitypolicy.h"

const UA_ByteString
UA_SECURITY_POLICY_NONE_URI = {47, (UA_Byte*)"http://opcfoundation.org/UA/SecurityPolicy#None"};

#define UA_SECURE_MESSAGE_HEADER_LENGTH 24
#define UA_ASYMMETRIC_ALG_SECURITY_HEADER_FIXED_LENGTH 12
#define UA_SYMMETRIC_ALG_SECURITY_HEADER_LENGTH 4
#define UA_MESSAGE_HEADER_LENGTH 8
#define UA_SEQUENCE_HEADER_LENGTH 8
#define UA_SECURE_CONVERSATION_MESSAGE_HEADER_LENGTH 12
#define UA_SECUREMH_AND_SYMALGH_LENGTH \
    (UA_SECURE_CONVERSATION_MESSAGE_HEADER_LENGTH + \
    UA_SYMMETRIC_ALG_SECURITY_HEADER_LENGTH)
#define UA_BITMASK_MESSAGETYPE 0x00ffffff
#define UA_BITMASK_CHUNKTYPE 0xff000000

/* For chunked responses */
typedef struct {
    UA_SecureChannel *channel;
    UA_UInt32 requestId;
    UA_UInt32 messageType;

    UA_UInt16 chunksSoFar;
    size_t messageSizeSoFar;

    UA_ByteString messageBuffer;
    UA_StatusCode errorCode;
    UA_Boolean final;
} UA_ChunkInfo;

void UA_SecureChannel_init(UA_SecureChannel* channel, UA_SecurityPolicies securityPolicies, UA_Logger logger) {
    memset(channel, 0, sizeof(UA_SecureChannel));
    channel->availableSecurityPolicies = securityPolicies;
    channel->logger = logger;
    /* Linked lists are also initialized by zeroing out */
    /* LIST_INIT(&channel->sessions); */
    /* LIST_INIT(&channel->chunks); */
}

void UA_SecureChannel_deleteMembersCleanup(UA_SecureChannel* channel) {
    /* Delete members */
    UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&channel->localAsymAlgSettings);
    UA_ByteString_deleteMembers(&channel->serverNonce);
    UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&channel->remoteAsymAlgSettings);
    UA_ByteString_deleteMembers(&channel->clientNonce);
    UA_ChannelSecurityToken_deleteMembers(&channel->securityToken);
    UA_ChannelSecurityToken_deleteMembers(&channel->nextSecurityToken);

    if(channel->securityContext != NULL) {
        channel->securityContext->deleteMembers(channel->securityContext);
        UA_free(channel->securityContext);
    }

    /* Detach from the channel */
    if(channel->connection)
        UA_Connection_detachSecureChannel(channel->connection);

    /* Remove session pointers (not the sessions) */
    struct SessionEntry *se, *temp;
    LIST_FOREACH_SAFE(se, &channel->sessions, pointers, temp) {
        if(se->session)
            se->session->channel = NULL;
        LIST_REMOVE(se, pointers);
        UA_free(se);
    }

    /* Remove the buffered chunks */
    struct ChunkEntry *ch, *temp_ch;
    LIST_FOREACH_SAFE(ch, &channel->chunks, pointers, temp_ch) {
        UA_ByteString_deleteMembers(&ch->bytes);
        LIST_REMOVE(ch, pointers);
        UA_free(ch);
    }
}

/**
 * Generates a nonce.
 *
 * Uses the random generator of the supplied security policy
 *
 * \param nonce will contain the nonce after being successfully called.
 * \param securityPolicy the SecurityPolicy to use.
 */
UA_StatusCode UA_SecureChannel_generateNonce(UA_ByteString* const nonce, const UA_SecurityPolicy* const securityPolicy) {
    nonce->data = (UA_Byte *)UA_malloc(securityPolicy->symmetricModule.encryptingKeyLength);
    if(!nonce->data)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    return securityPolicy->symmetricModule.generateNonce(securityPolicy, nonce);
}

/**
 * Generates new keys and sets them in the channel context
 *
 * \param channel the channel to generate new keys for
 */
UA_StatusCode UA_SecureChannel_generateNewKeys(UA_SecureChannel* const channel) {
    const UA_SecurityPolicySymmetricModule* const symmetricModule = &channel->securityPolicy->symmetricModule;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    UA_ByteString buffer;
    const size_t buffSize = symmetricModule->encryptingBlockSize +
        symmetricModule->signingKeyLength +
        symmetricModule->encryptingKeyLength;
    retval = UA_ByteString_allocBuffer(&buffer, buffSize);
    if(retval != UA_STATUSCODE_GOOD) {
        return retval;
    }

    // Client keys
    retval = symmetricModule->generateKey(&channel->serverNonce,
                                          &channel->clientNonce,
                                          &buffer);
    if(retval != UA_STATUSCODE_GOOD) {
        return retval;
    }

    const UA_ByteString clientSigningKey = {
        symmetricModule->signingKeyLength,
        buffer.data,
    };
    retval |= channel->securityContext->setRemoteSigningKey(channel->securityContext, &clientSigningKey);

    const UA_ByteString clientEncryptingKey = {
        symmetricModule->encryptingKeyLength,
        buffer.data + symmetricModule->signingKeyLength
    };
    retval |= channel->securityContext->setRemoteEncryptingKey(channel->securityContext, &clientEncryptingKey);

    const UA_ByteString clientIv = {
        symmetricModule->encryptingBlockSize,
        buffer.data + symmetricModule->signingKeyLength + symmetricModule->encryptingKeyLength
    };
    retval |= channel->securityContext->setRemoteIv(channel->securityContext, &clientIv);

    if(retval != UA_STATUSCODE_GOOD) {
        return retval;
    }

    // Server keys
    symmetricModule->generateKey(&channel->clientNonce,
                                 &channel->serverNonce,
                                 &buffer);
    if(retval != UA_STATUSCODE_GOOD) {
        return retval;
    }

    const UA_ByteString serverSigningKey = {
        symmetricModule->signingKeyLength,
        buffer.data
    };
    retval |= channel->securityContext->setLocalSigningKey(channel->securityContext, &serverSigningKey);

    const UA_ByteString serverEncryptingKey = {
        symmetricModule->encryptingKeyLength,
        buffer.data + symmetricModule->signingKeyLength
    };
    retval |= channel->securityContext->setLocalEncryptingKey(channel->securityContext, &serverEncryptingKey);

    const UA_ByteString serverIv = {
        symmetricModule->encryptingBlockSize,
        buffer.data + symmetricModule->signingKeyLength + symmetricModule->encryptingKeyLength
    };
    retval |= channel->securityContext->setLocalIv(channel->securityContext, &serverIv);

    return retval;
}

#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wunused-value"
#endif

void UA_SecureChannel_attachSession(UA_SecureChannel* channel, UA_Session* session) {
    struct SessionEntry* se = (struct SessionEntry *)UA_malloc(sizeof(struct SessionEntry));
    if(!se)
        return;
    se->session = session;
    if(UA_atomic_cmpxchg((void**)&session->channel, NULL, channel) != NULL) {
        UA_free(se);
        return;
    }
    LIST_INSERT_HEAD(&channel->sessions, se, pointers);
}

#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif

void UA_SecureChannel_detachSession(UA_SecureChannel* channel, UA_Session* session) {
    if(session)
        session->channel = NULL;
    struct SessionEntry* se;
    LIST_FOREACH(se, &channel->sessions, pointers) {
        if(se->session == session)
            break;
    }
    if(!se)
        return;
    LIST_REMOVE(se, pointers);
    UA_free(se);
}

UA_Session* UA_SecureChannel_getSession(UA_SecureChannel* channel, UA_NodeId* token) {
    struct SessionEntry* se;
    LIST_FOREACH(se, &channel->sessions, pointers) {
        if(UA_NodeId_equal(&se->session->authenticationToken, token))
            break;
    }
    if(!se)
        return NULL;
    return se->session;
}

void UA_SecureChannel_revolveTokens(UA_SecureChannel* channel) {
    if(channel->nextSecurityToken.tokenId == 0) //no security token issued
        return;

    //FIXME: not thread-safe
    memcpy(&channel->securityToken, &channel->nextSecurityToken,
           sizeof(UA_ChannelSecurityToken));
    UA_ChannelSecurityToken_init(&channel->nextSecurityToken);

    UA_SecureChannel_generateNewKeys(channel);
}

/***********************/
/* Send Binary Message */
/***********************/

static UA_StatusCode
UA_SecureChannel_sendChunk(UA_ChunkInfo* ci, UA_Byte **buf_pos, UA_Byte **buf_end) {
    UA_SecureChannel* const channel = ci->channel;
    UA_Connection* const connection = channel->connection;
    const UA_SecurityPolicy* const securityPolicy = channel->securityPolicy;

    if(!connection)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Will this chunk surpass the capacity of the SecureChannel for the message? */
    UA_Byte *buf_body_start = ci->messageBuffer.data + UA_SECURE_MESSAGE_HEADER_LENGTH;
    UA_Byte *buf_body_end = *buf_pos;
    size_t bodyLength = (uintptr_t)buf_body_end - (uintptr_t)buf_body_start;
    ci->messageSizeSoFar += bodyLength;
    ci->chunksSoFar++;
    if(ci->messageSizeSoFar > connection->remoteConf.maxMessageSize &&
       connection->remoteConf.maxMessageSize != 0)
        ci->errorCode = UA_STATUSCODE_BADRESPONSETOOLARGE;
    if(ci->chunksSoFar > connection->remoteConf.maxChunkCount &&
       connection->remoteConf.maxChunkCount != 0)
        ci->errorCode = UA_STATUSCODE_BADRESPONSETOOLARGE;

    /* An error occurred. Overwrite the current chunk with an abort message. */
    if(ci->errorCode != UA_STATUSCODE_GOOD) {
        *buf_pos = buf_body_start;
        UA_String errorMsg = UA_STRING_NULL;
        UA_encodeBinary(&ci->errorCode, &UA_TYPES[UA_TYPES_UINT32],
                        buf_pos, buf_end, NULL, NULL);
        UA_encodeBinary(&errorMsg, &UA_TYPES[UA_TYPES_STRING],
                        buf_pos, buf_end, NULL, NULL);
        ci->final = true; /* Mark chunk as finished */
    }

    /* Pad the message. The bytes for the padding and signature were removed
     * from buf_end before encoding the payload. So we don't check here. */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        size_t bytesToWrite = bodyLength + UA_SEQUENCE_HEADER_LENGTH;
        UA_Byte paddingSize = 0;
        UA_Byte extraPaddingSize = 0;
        securityPolicy->symmetricModule.calculatePadding(securityPolicy, bytesToWrite,
                                                         &paddingSize, &extraPaddingSize);
        UA_UInt16 totalPaddingSize = (UA_UInt16)((extraPaddingSize << 8) | paddingSize);

        // This is <= because the paddingSize byte also has to be written.
        for(UA_UInt16 i = 0; i <= totalPaddingSize; ++i) {
            **buf_pos = paddingSize;
            ++(*buf_pos);
        }

        if(extraPaddingSize > 0) {
            **buf_pos = extraPaddingSize;
            ++(*buf_pos);
        }
    }

    /* The total message length */
    UA_UInt32 pre_sig_length = (UA_UInt32)((uintptr_t)(*buf_pos) - (uintptr_t)ci->messageBuffer.data);
    UA_UInt32 total_length = pre_sig_length;
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        total_length +=
            securityPolicy->symmetricModule.signingModule.signatureSize;

    /* Encode the chunk headers at the beginning of the buffer */
    UA_Byte *header_pos = ci->messageBuffer.data;
    UA_SecureConversationMessageHeader respHeader;
    respHeader.secureChannelId = channel->securityToken.channelId;
    respHeader.messageHeader.messageTypeAndChunkType = ci->messageType;
    respHeader.messageHeader.messageSize = total_length;
    if(ci->errorCode == UA_STATUSCODE_GOOD) {
        if(ci->final)
            respHeader.messageHeader.messageTypeAndChunkType += UA_CHUNKTYPE_FINAL;
        else
            respHeader.messageHeader.messageTypeAndChunkType += UA_CHUNKTYPE_INTERMEDIATE;
    } else {
        respHeader.messageHeader.messageTypeAndChunkType += UA_CHUNKTYPE_ABORT;
    }
    UA_encodeBinary(&respHeader, &UA_TRANSPORT[UA_TRANSPORT_SECURECONVERSATIONMESSAGEHEADER],
                    &header_pos, buf_end, NULL, NULL);

    UA_SymmetricAlgorithmSecurityHeader symSecHeader;
    symSecHeader.tokenId = channel->securityToken.tokenId;
    UA_encodeBinary(&symSecHeader.tokenId, &UA_TRANSPORT[UA_TRANSPORT_SYMMETRICALGORITHMSECURITYHEADER],
                    &header_pos, buf_end, NULL, NULL);

    UA_SequenceHeader seqHeader;
    seqHeader.requestId = ci->requestId;
    seqHeader.sequenceNumber = UA_atomic_add(&channel->sendSequenceNumber, 1);
    UA_encodeBinary(&seqHeader, &UA_TRANSPORT[UA_TRANSPORT_SEQUENCEHEADER],
                    &header_pos, buf_end, NULL, NULL);

    /* Sign message */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        UA_ByteString dataToSign = ci->messageBuffer;
        dataToSign.length = pre_sig_length;

        UA_ByteString signature;
        signature.length = securityPolicy->symmetricModule.signingModule.signatureSize;
        signature.data = *buf_pos;

        securityPolicy->symmetricModule.signingModule.sign(&dataToSign, channel->securityContext,
                                                           &signature);
    }

    /* Encrypt message */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        UA_ByteString dataToEncrypt;
        dataToEncrypt.data = ci->messageBuffer.data + UA_SECUREMH_AND_SYMALGH_LENGTH;
        dataToEncrypt.length = total_length - UA_SECUREMH_AND_SYMALGH_LENGTH;

        securityPolicy->symmetricModule.encrypt(channel->securityContext, &dataToEncrypt);
    }

    /* Send the chunk, the buffer is freed in the network layer */
    ci->messageBuffer.length = respHeader.messageHeader.messageSize;
    connection->send(channel->connection, &ci->messageBuffer);

    /* Replace with the buffer for the next chunk */
    if(!ci->final && ci->errorCode == UA_STATUSCODE_GOOD) {
        UA_StatusCode retval =
            connection->getSendBuffer(connection, connection->localConf.sendBufferSize,
                                      &ci->messageBuffer);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;

        /* Forward the data pointer so that the payload is encoded after the message header.
         * TODO: This works but is a bit too clever. Instead, we could return an offset to the
         * binary encoding exchangeBuffer function. */
        *buf_pos = ci->messageBuffer.data + UA_SECURE_MESSAGE_HEADER_LENGTH;
        *buf_end = &ci->messageBuffer.data[ci->messageBuffer.length];

        if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
           channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
            *buf_end -= securityPolicy->symmetricModule.signingModule.signatureSize;

        /* Hide bytes for paddingByte and potential extra padding byte */
        if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
            *buf_end -= 2;
    }
    return ci->errorCode;
}

static size_t
UA_SecureChannel_calculateAsymAlgSecurityHeaderLength(
    const UA_AsymmetricAlgorithmSecurityHeader* const asymHeader) {
    return UA_ASYMMETRIC_ALG_SECURITY_HEADER_FIXED_LENGTH +
        asymHeader->securityPolicyUri.length +
        asymHeader->senderCertificate.length +
        asymHeader->receiverCertificateThumbprint.length;
}

/**
 * \brief Sends an OPN chunk using asymmetric encryption.
 *
 * \param ci the chunk information that is used to send the chunk.
 * \param buf_pos the position in the send buffer after the body was encoded.
 *                Should be less than or equal to buf_end.
 * \param buf_end the maximum position of the body.
 */
static UA_StatusCode UA_SecureChannel_sendOPNChunkAsymmetric(UA_ChunkInfo* const ci,
                                                             UA_Byte **buf_pos,
                                                             UA_Byte **buf_end) {
    UA_SecureChannel* const channel = ci->channel;
    UA_Connection* const connection = channel->connection;
    const UA_SecurityPolicy* const securityPolicy = channel->securityPolicy;
    
    if(!connection)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Restore dst and offset that were present in the old interface */
    /*
    UA_ByteString _dst;
    _dst.data = ci->messageBuffer.data + UA_SECURE_CONVERSATION_MESSAGE_HEADER_LENGTH + UA_SEQUENCE_HEADER_LENGTH;
    if(UA_ByteString_equal(&UA_SECURITY_POLICY_NONE_URI, &channel->securityPolicy->policyUri))
        _dst.data +=
            UA_SecureChannel_calculateAsymAlgSecurityHeaderLength(&channel->remoteAsymAlgSettings);
    else
        _dst.data +=
            UA_SecureChannel_calculateAsymAlgSecurityHeaderLength(&channel->localAsymAlgSettings);
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        _dst.data -= channel->securityPolicy->asymmetricModule.signingModule.signatureSize;
        _dst.data -= 2;
    }
    _dst.length = ci->messageBuffer.length - ((uintptr_t)_dst.data - (uintptr_t)ci->messageBuffer.data);
    UA_ByteString *dst = &_dst;
    size_t offset = (uintptr_t)*buf_pos - (uintptr_t)_dst.data;
    */

    /* Will this chunk surpass the capacity of the SecureChannel for the message? */
    
    const UA_AsymmetricAlgorithmSecurityHeader* asymHeader = NULL;
    if(UA_ByteString_equal(&UA_SECURITY_POLICY_NONE_URI, &securityPolicy->policyUri)) {
        asymHeader = &channel->remoteAsymAlgSettings;
    }
    else {
        asymHeader = &channel->localAsymAlgSettings;
    }

    const size_t securityHeaderLength = UA_SecureChannel_calculateAsymAlgSecurityHeaderLength(asymHeader);
    const size_t headerLength = 
        UA_SECURE_CONVERSATION_MESSAGE_HEADER_LENGTH +
        UA_SEQUENCE_HEADER_LENGTH +
        securityHeaderLength;

    UA_Byte *buf_body_start = ci->messageBuffer.data + headerLength;
    UA_Byte *buf_body_end = *buf_pos;
    size_t bodyLength = (uintptr_t)buf_body_end - (uintptr_t)buf_body_start;
    ci->messageSizeSoFar += bodyLength;
    ci->chunksSoFar++;
    if(ci->messageSizeSoFar > connection->remoteConf.maxMessageSize &&
       connection->remoteConf.maxMessageSize != 0)
        ci->errorCode = UA_STATUSCODE_BADRESPONSETOOLARGE;
    if(ci->chunksSoFar > connection->remoteConf.maxChunkCount &&
       connection->remoteConf.maxChunkCount != 0)
        ci->errorCode = UA_STATUSCODE_BADRESPONSETOOLARGE;
    
    if(ci->errorCode != UA_STATUSCODE_GOOD) {
        // abort message
        // TODO: This does not seem to be specified in the standard since chunking isnt specified as well.
        // TODO: Should we just close the connection here?
        connection->close(connection);
        return ci->errorCode;
    }

    const size_t bytesToWrite = bodyLength + UA_SEQUENCE_HEADER_LENGTH;

    // TODO: Potentially introduce error handling to the encryption methods???

    // Pad the message. Also if securitymode is only sign, since we are using
    // asymmetric communication to exchange keys and thus need to encrypt.
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        UA_Byte paddingSize = 0;
        UA_Byte extraPaddingSize = 0;
        securityPolicy->asymmetricModule.calculatePadding(securityPolicy, bytesToWrite,
                                                          &paddingSize, &extraPaddingSize);
        UA_UInt16 totalPaddingSize = (UA_UInt16)((extraPaddingSize << 8) | paddingSize);

        // This is <= because the paddingSize byte also has to be written.
        for(UA_UInt16 i = 0; i <= totalPaddingSize; ++i) {
            **buf_pos = paddingSize;
            ++(*buf_pos);
        }

        if(extraPaddingSize > 0) {
            **buf_pos = extraPaddingSize;
            ++(*buf_pos);
        }
    }

    // The total message length
    UA_UInt32 pre_sig_length = (UA_UInt32)((uintptr_t)(*buf_pos) - (uintptr_t)ci->messageBuffer.data);
    UA_UInt32 total_length = pre_sig_length;
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        total_length += securityPolicy->asymmetricModule.signingModule.signatureSize;

    // Encode the chunk headers at the beginning of the buffer
    UA_Byte *header_pos = ci->messageBuffer.data;
    UA_SecureConversationMessageHeader respHeader;
    respHeader.secureChannelId = channel->securityToken.channelId;
    respHeader.messageHeader.messageTypeAndChunkType = ci->messageType;
    respHeader.messageHeader.messageSize = total_length;
    if(ci->errorCode == UA_STATUSCODE_GOOD) {
        if(ci->final)
            respHeader.messageHeader.messageTypeAndChunkType += UA_CHUNKTYPE_FINAL;
        else
            respHeader.messageHeader.messageTypeAndChunkType += UA_CHUNKTYPE_INTERMEDIATE;
    } else {
        /* abort message */
        // TODO: This does not seem to be specified in the standard since chunking isnt specified as well.
        // TODO: Should we just close the connection here?
        connection->close(connection);
        return ci->errorCode;
    }

    UA_encodeBinary(&respHeader, &UA_TRANSPORT[UA_TRANSPORT_SECURECONVERSATIONMESSAGEHEADER],
                    &header_pos, buf_end, NULL, NULL);

    if(UA_ByteString_equal(&UA_SECURITY_POLICY_NONE_URI, &securityPolicy->policyUri)) {
        UA_encodeBinary(&channel->remoteAsymAlgSettings,
                        &UA_TRANSPORT[UA_TRANSPORT_ASYMMETRICALGORITHMSECURITYHEADER],
                        &header_pos, buf_end, NULL, NULL);
    } else {
        UA_encodeBinary(&channel->localAsymAlgSettings,
                        &UA_TRANSPORT[UA_TRANSPORT_ASYMMETRICALGORITHMSECURITYHEADER],
                        &header_pos, buf_end, NULL, NULL);
    }

    UA_SequenceHeader seqHeader;
    seqHeader.requestId = ci->requestId;
    seqHeader.sequenceNumber = UA_atomic_add(&channel->sendSequenceNumber, 1);
    UA_encodeBinary(&seqHeader, &UA_TRANSPORT[UA_TRANSPORT_SEQUENCEHEADER],
                    &header_pos, buf_end, NULL, NULL);

    // Sign message
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        const UA_ByteString dataToSign = {
            pre_sig_length,
            ci->messageBuffer.data
        };

        UA_ByteString signature = {
            securityPolicy->asymmetricModule.signingModule.signatureSize,
            ci->messageBuffer.data + pre_sig_length
        };

        securityPolicy->asymmetricModule.signingModule.sign(&dataToSign, &securityPolicy->context, &signature);
    }

    // Always encrypt message if mode not none, since we are exchanging keys.
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        const UA_ByteString dataToEncrypt = {
            total_length - (UA_SECURE_CONVERSATION_MESSAGE_HEADER_LENGTH + securityHeaderLength),
            ci->messageBuffer.data + UA_SECURE_CONVERSATION_MESSAGE_HEADER_LENGTH + securityHeaderLength
        };

        securityPolicy->asymmetricModule.encrypt(&securityPolicy->context, &dataToEncrypt);
    }

    // Send the chunk, the buffer is freed in the network layer
    ci->messageBuffer.length = respHeader.messageHeader.messageSize; // set the buffer length to the content length
    connection->send(channel->connection, &ci->messageBuffer);

    /* Replace with the buffer for the next chunk */
    if(!ci->final && ci->errorCode == UA_STATUSCODE_GOOD) {
        UA_StatusCode retval =
            connection->getSendBuffer(connection, connection->localConf.sendBufferSize, &ci->messageBuffer);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;

        *buf_pos = ci->messageBuffer.data + headerLength;
        *buf_end = &ci->messageBuffer.data[ci->messageBuffer.length];

        if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
           channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
            *buf_end -= securityPolicy->asymmetricModule.signingModule.signatureSize;

            // Hide byte for paddingByte and potential extra padding byte
            *buf_end -= 2;
        }
    }
    return ci->errorCode;
}

UA_StatusCode
UA_SecureChannel_sendBinaryMessage(UA_SecureChannel* channel, UA_UInt32 requestId,
                                   const void* content, const UA_DataType* contentType) {
    UA_Connection* connection = channel->connection;
    if(!connection)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ChunkInfo ci;
    ci.channel = channel;
    ci.requestId = requestId;
    ci.chunksSoFar = 0;
    ci.messageSizeSoFar = 0;
    ci.final = false;
    ci.messageType = UA_MESSAGETYPE_MSG;
    ci.errorCode = UA_STATUSCODE_GOOD;
    ci.messageBuffer = UA_BYTESTRING_NULL;
    if(contentType->binaryEncodingId == UA_TYPES[UA_TYPES_OPENSECURECHANNELREQUEST].binaryEncodingId ||
       contentType->binaryEncodingId == UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE].binaryEncodingId)
        ci.messageType = UA_MESSAGETYPE_OPN;
    else if(contentType->binaryEncodingId == UA_TYPES[UA_TYPES_CLOSESECURECHANNELREQUEST].binaryEncodingId ||
            contentType->binaryEncodingId == UA_TYPES[UA_TYPES_CLOSESECURECHANNELRESPONSE].binaryEncodingId)
        ci.messageType = UA_MESSAGETYPE_CLO;

    /* Allocate the message buffer */
    UA_StatusCode retval =
        connection->getSendBuffer(connection, connection->localConf.sendBufferSize, &ci.messageBuffer);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_ExchangeEncodeBuffer sendChunk = NULL;
    UA_Byte *buf_start = NULL;
    UA_Byte *buf_end = &ci.messageBuffer.data[ci.messageBuffer.length];

    switch(ci.messageType) {
    case UA_MESSAGETYPE_MSG:
        /* Hide the message beginning where the header will be encoded */
        buf_start = &ci.messageBuffer.data[UA_SECURE_MESSAGE_HEADER_LENGTH];

        /* Hide bytes for signature */
        if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
           channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
            buf_end -= channel->securityPolicy->symmetricModule.signingModule.signatureSize;

        /* Hide bytes for padding */
        if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
            buf_end -= 2;

        /* Set the callback */
        sendChunk = (UA_ExchangeEncodeBuffer)UA_SecureChannel_sendChunk;
        break;

    case UA_MESSAGETYPE_OPN:
        buf_start = &ci.messageBuffer.data[UA_SECURE_CONVERSATION_MESSAGE_HEADER_LENGTH + UA_SEQUENCE_HEADER_LENGTH];
        
        /* Add the SecurityHeaderLength */
        if(UA_ByteString_equal(&UA_SECURITY_POLICY_NONE_URI, &channel->securityPolicy->policyUri))
            buf_start +=
                UA_SecureChannel_calculateAsymAlgSecurityHeaderLength(&channel->remoteAsymAlgSettings);
        else
            buf_start +=
                UA_SecureChannel_calculateAsymAlgSecurityHeaderLength(&channel->localAsymAlgSettings);

        /* Hide bytes for signature and padding */
        if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
           channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
            buf_end -= channel->securityPolicy->asymmetricModule.signingModule.signatureSize;
            buf_end -= 2;
        }

        sendChunk = (UA_ExchangeEncodeBuffer)UA_SecureChannel_sendOPNChunkAsymmetric;
        break;

    default:
        UA_LOG_FATAL(channel->logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Called sendBinaryMessage with an invalid message type");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_assert(buf_start != NULL);
    UA_assert(sendChunk != NULL);

    /* Encode the message type */
    UA_NodeId typeId = contentType->typeId; /* always numeric */
    typeId.identifier.numeric = contentType->binaryEncodingId;
    UA_encodeBinary(&typeId, &UA_TYPES[UA_TYPES_NODEID], &buf_start, &buf_end, NULL, NULL);

    /* Encode with the chunking callback */
    retval = UA_encodeBinary(content, contentType, &buf_start, &buf_end, sendChunk, &ci);

    /* Encoding failed, release the message */
    if(retval != UA_STATUSCODE_GOOD) {
        if(!ci.final) {
            /* the abort message was not sent */
            ci.errorCode = retval;
            sendChunk(&ci, &buf_start, &buf_end);
        }
        return retval;
    }

    /* Encoding finished, send the final chunk */
    ci.final = UA_TRUE;
    return sendChunk(&ci, &buf_start, &buf_end);
}

/***************************/
/* Process Received Chunks */
/***************************/

static void
UA_SecureChannel_removeChunk(UA_SecureChannel* channel, UA_UInt32 requestId) {
    struct ChunkEntry* ch;
    LIST_FOREACH(ch, &channel->chunks, pointers) {
        if(ch->requestId == requestId) {
            UA_ByteString_deleteMembers(&ch->bytes);
            LIST_REMOVE(ch, pointers);
            UA_free(ch);
            return;
        }
    }
}

/* assume that chunklength fits */
// TODO: Write documentation
static UA_StatusCode
appendChunk(struct ChunkEntry* const chunkEntry, const UA_ByteString* const chunkBody) {
    UA_Byte* new_bytes = (UA_Byte *)UA_realloc(chunkEntry->bytes.data, chunkEntry->bytes.length + chunkBody->length);
    if(!new_bytes) {
        UA_ByteString_deleteMembers(&chunkEntry->bytes);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    chunkEntry->bytes.data = new_bytes;
    memcpy(&chunkEntry->bytes.data[chunkEntry->bytes.length], chunkBody->data, chunkBody->length);
    chunkEntry->bytes.length += chunkBody->length;

    return UA_STATUSCODE_GOOD;
}

/**
 * \brief Appends a decrypted chunk to the already processed chunks.
 *
 * \param channel the UA_SecureChannel to search for existing chunks.
 * \param requestId the request id of the message.
 * \param chunkBody the body of the chunk to append to the request.
 */
static UA_StatusCode
UA_SecureChannel_appendChunk(UA_SecureChannel* channel,
                             UA_UInt32 requestId,
                             const UA_ByteString* chunkBody) {
    /*
    // Check if the chunk fits into the message
    if(chunk->length - offset < chunklength) {
        // can't process all chunks for that request
        UA_SecureChannel_removeChunk(channel, requestId);
        return;
    }
    */

    /* Get the chunkentry */
    struct ChunkEntry* ch;
    LIST_FOREACH(ch, &channel->chunks, pointers) {
        if(ch->requestId == requestId)
            break;
    }

    /* No chunkentry on the channel, create one */
    if(!ch) {
        ch = (struct ChunkEntry *)UA_malloc(sizeof(struct ChunkEntry));
        if(!ch)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        ch->requestId = requestId;
        UA_ByteString_init(&ch->bytes);
        LIST_INSERT_HEAD(&channel->chunks, ch, pointers);
    }

    return appendChunk(ch, chunkBody);
}

static UA_StatusCode
UA_SecureChannel_finalizeChunk(UA_SecureChannel *const channel,
                               const UA_UInt32 requestId,
                               const UA_ByteString *const chunkBody,
                               UA_Boolean *const deleteChunk,
                               UA_ByteString *const finalMessage) {
    /*
    if(chunk->length - offset < chunklength) {
        // can't process all chunks for that request
        UA_SecureChannel_removeChunk(channel, requestId);
        return UA_BYTESTRING_NULL;
    }
    */

    UA_StatusCode status = UA_STATUSCODE_GOOD;

    struct ChunkEntry* chunkEntry;
    LIST_FOREACH(chunkEntry, &channel->chunks, pointers) {
        if(chunkEntry->requestId == requestId)
            break;
    }

    UA_ByteString bytes;
    if(!chunkEntry) {
        *deleteChunk = false;
        bytes.length = chunkBody->length;
        bytes.data = chunkBody->data;
    }
    else {
        *deleteChunk = true;
        status |= appendChunk(chunkEntry, chunkBody);
        bytes = chunkEntry->bytes;
        LIST_REMOVE(chunkEntry, pointers);
        UA_free(chunkEntry);
    }

    *finalMessage = bytes;

    return status;
}

static UA_StatusCode
UA_SecureChannel_processSequenceNumber(UA_SecureChannel* channel, UA_UInt32 SequenceNumber) {
    /* Does the sequence number match? */
    if(SequenceNumber != channel->receiveSequenceNumber + 1) {
        if(channel->receiveSequenceNumber + 1 > 4294966271 && SequenceNumber < 1024) // FIXME: Remove magic numbers :(
            channel->receiveSequenceNumber = SequenceNumber - 1; /* Roll over */
        else
            return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }
    ++channel->receiveSequenceNumber;
    return UA_STATUSCODE_GOOD;
}

/**
 * \brief Processes a symmetric chunk, decoding and decrypting it.
 *
 * \param chunk the chunk to process. The data in the chunk will be modified in place.
                That is, it will be decoded and decrypted in place.
 * \param channel the UA_SecureChannel to work on.
 * \param messageHeader the message header of the chunk that was already decoded.
 * \param processedBytes the already processed bytes. After this function
 *                       finishes, the processedBytes offset will point to
 *                       the beginning of the message body
 * \param requestId the requestId of the chunk. Will be filled by the function.
 * \param bodySize the size of the chunk body will be written here.
 */
static UA_StatusCode
UA_SecureChannel_processSymmetricChunk(UA_ByteString* const chunk,
                                       UA_SecureChannel* const channel,
                                       UA_SecureConversationMessageHeader* const messageHeader,
                                       size_t* const processedBytes,
                                       UA_UInt32* const requestId,
                                       size_t* const bodySize) {
    const UA_SecurityPolicy* const securityPolicy = channel->securityPolicy;

    size_t chunkSize = messageHeader->messageHeader.messageSize;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    /* Check the symmetric security header */
    UA_SymmetricAlgorithmSecurityHeader symmetricSecurityHeader;
    retval |= UA_SymmetricAlgorithmSecurityHeader_decodeBinary(chunk, processedBytes, &symmetricSecurityHeader);

    size_t messageAndSecurityHeaderOffset = *processedBytes;

    /* Does the token match? */
    if(symmetricSecurityHeader.tokenId != channel->securityToken.tokenId) {
        if(symmetricSecurityHeader.tokenId != channel->nextSecurityToken.tokenId)
            return UA_STATUSCODE_BADSECURECHANNELTOKENUNKNOWN;
        UA_SecureChannel_revolveTokens(channel);
    }

    // Decrypt message
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        UA_ByteString cipherText = {
            chunkSize - messageAndSecurityHeaderOffset,
            chunk->data + messageAndSecurityHeaderOffset
        };

        retval |= securityPolicy->symmetricModule.decrypt(channel->securityContext, &cipherText);

        if(retval != UA_STATUSCODE_GOOD) {
            return retval;
        }
    }

    // Verify signature
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN || channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        // signature is made over everything except the signature itself.
        const UA_ByteString chunkDataToVerify = {
            chunkSize - securityPolicy->symmetricModule.signingModule.signatureSize,
            chunk->data
        };
        const UA_ByteString signature = {
            securityPolicy->symmetricModule.signingModule.signatureSize,
            chunk->data + chunkDataToVerify.length // Signature starts after the signed data
        };

        retval |= securityPolicy->symmetricModule.signingModule.verify(&chunkDataToVerify,
                                                                       &signature,
                                                                       channel->securityContext);

        if(retval != UA_STATUSCODE_GOOD) {
            return retval;
        }
    }

    UA_SequenceHeader sequenceHeader;
    UA_SequenceHeader_init(&sequenceHeader);
    retval |= UA_SequenceHeader_decodeBinary(chunk, processedBytes, &sequenceHeader);
    if(retval != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;

    /* Does the sequence number match? */
    retval = UA_SecureChannel_processSequenceNumber(channel, sequenceHeader.sequenceNumber);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    *requestId = sequenceHeader.requestId;

    *bodySize = messageHeader->messageHeader.messageSize - *processedBytes;

    return UA_STATUSCODE_GOOD;
}

/**
 * \brief Processes an asymmetric chunk, decoding and decrypting it.
 *
 * \param chunk the chunk to process. The data in the chunk will be modified in place.
                That is, it will be decoded and decrypted in place.
 * \param channel the UA_SecureChannel to work on.
 * \param messageHeader the message header of the chunk that was already decoded.
 * \param processedBytes the already processed bytes. The offset must be relative to the start of the chunk.
 *                       After this function finishes, the processedBytes offset will point to
 *                       the beginning of the message body
 * \param requestId the requestId of the chunk. Will be filled by the function.
 * \param bodySize the size of the body segment of the chunk.
 */
static UA_StatusCode
UA_SecureChannel_processAsymmetricOPNChunk(const UA_ByteString* const chunk,
                                           UA_SecureChannel* const channel,
                                           UA_SecureConversationMessageHeader* const messageHeader,
                                           size_t* const processedBytes,
                                           UA_UInt32* const requestId,
                                           size_t* const bodySize) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    size_t chunkSize = messageHeader->messageHeader.messageSize;

    UA_AsymmetricAlgorithmSecurityHeader clientAsymHeader;
    UA_AsymmetricAlgorithmSecurityHeader_init(&clientAsymHeader);
    retval |= UA_AsymmetricAlgorithmSecurityHeader_decodeBinary(chunk, processedBytes, &clientAsymHeader);
    size_t messageAndSecurityHeaderOffset = *processedBytes;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&clientAsymHeader);
        return retval;
    }

    retval |= UA_AsymmetricAlgorithmSecurityHeader_copy(&clientAsymHeader, &channel->remoteAsymAlgSettings);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&clientAsymHeader);
        return retval;
    }


    if(channel->securityPolicy == NULL) {
        // iterate available security policies and choose the correct one
        const UA_SecurityPolicy* securityPolicy = NULL;
        UA_LOG_DEBUG(channel->logger,
                     UA_LOGCATEGORY_SECURECHANNEL,
                     "Trying to open connection with policy %.*s",
                     clientAsymHeader.securityPolicyUri.length,
                     clientAsymHeader.securityPolicyUri.data);
        for(size_t i = 0; i < channel->availableSecurityPolicies.count; ++i) {
            if(UA_ByteString_equal(&clientAsymHeader.securityPolicyUri,
                                   &channel->availableSecurityPolicies.policies[i].policyUri)) {
                UA_LOG_DEBUG(channel->logger,
                             UA_LOGCATEGORY_SECURECHANNEL,
                             "Using security policy %s",
                             channel->availableSecurityPolicies.policies[i].policyUri.data);

                securityPolicy = &channel->availableSecurityPolicies.policies[i];
                break;
            }
        }

        if(securityPolicy == NULL) {
            // TODO: Abort connection?
            UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&clientAsymHeader);
            return UA_STATUSCODE_BADSECURITYPOLICYREJECTED;
        }

        channel->securityPolicy = securityPolicy;
    }
    else {
        // Policy not the same as when channel was originally opened
        if(!UA_ByteString_equal(&clientAsymHeader.securityPolicyUri,
                                &channel->securityPolicy->policyUri)) {
            UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&clientAsymHeader);
            return UA_STATUSCODE_BADSECURITYPOLICYREJECTED;
        }
    }

    const UA_SecurityPolicy* const securityPolicy = channel->securityPolicy;

    // Create the channel context and parse the sender certificate used for the secureChannel.
    if(channel->securityContext == NULL) {
        UA_Channel_SecurityContext* channelContext = NULL;
        retval |= securityPolicy->makeChannelContext(securityPolicy, &channelContext);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&clientAsymHeader);
            return retval;
        }

        retval |= channelContext->init(channelContext, securityPolicy, channel->logger);
        if(retval != UA_STATUSCODE_GOOD) {
            channelContext->deleteMembers(channelContext);
            UA_free(channelContext);
            UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&clientAsymHeader);
            return retval;
        }

        // TODO: Do we need to parse the cert only once or for each chunk? (Assuming chunking is even possible??)
        retval |= channelContext->parseClientCertificate(channelContext, &clientAsymHeader.senderCertificate);
        if(retval != UA_STATUSCODE_GOOD) {
            channelContext->deleteMembers(channelContext);
            UA_free(channelContext);
            UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&clientAsymHeader);
            return retval;
        }

        channel->securityContext = channelContext;
    }

    // Verify the vertificate
    retval |= securityPolicy->verifyCertificate(&clientAsymHeader.senderCertificate, &securityPolicy->context);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&clientAsymHeader);
        return retval;
    }

    // Decrypt message
    {
        UA_ByteString cipherText = {
            chunkSize - messageAndSecurityHeaderOffset,
            chunk->data + messageAndSecurityHeaderOffset
        };

        retval |= securityPolicy->asymmetricModule.decrypt(&securityPolicy->context, &cipherText);

        if(retval != UA_STATUSCODE_GOOD) {
            return retval;
        }
    }

    // Verify signature
    {
        // signature is made over everything except the signature itself.
        const UA_ByteString chunkDataToVerify = {
            chunkSize - securityPolicy->asymmetricModule.signingModule.signatureSize,
            chunk->data
        };
        const UA_ByteString signature = {
            securityPolicy->asymmetricModule.signingModule.signatureSize,
            chunk->data + chunkDataToVerify.length // Signature starts after the signed data
        };

        retval |= securityPolicy->asymmetricModule.signingModule.verify(&chunkDataToVerify,
                                                                        &signature,
                                                                        channel->securityContext);

        if(retval != UA_STATUSCODE_GOOD) {
            UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&clientAsymHeader);
            return retval;
        }
    }

    UA_SequenceHeader sequenceHeader;
    UA_SequenceHeader_init(&sequenceHeader);
    retval |= UA_SequenceHeader_decodeBinary(chunk, processedBytes, &sequenceHeader);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&clientAsymHeader);
        UA_SequenceHeader_deleteMembers(&sequenceHeader);
        return retval;
    }

    *requestId = sequenceHeader.requestId;

    // Set the starting sequence number
    channel->receiveSequenceNumber = sequenceHeader.sequenceNumber;

    // calculate body size
    size_t signatureSize = securityPolicy->asymmetricModule.signingModule.signatureSize;
    // TODO: padding field is not present, when dealing with securitymode none messages
    // TODO: How to get securitymode? or is securitymode none only possible when security policy is none?
    //UA_Byte paddingSize = chunk->data[chunkSize - signatureSize - 1]; // TODO: Need to differentiate if extra padding byte was used
    UA_Byte paddingSize = 0;
    *bodySize = chunkSize - *processedBytes - signatureSize - paddingSize;
    if(*bodySize > chunkSize) {
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&clientAsymHeader);
        UA_SequenceHeader_deleteMembers(&sequenceHeader);
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }

    // Cleanup
    UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&clientAsymHeader);
    UA_SequenceHeader_deleteMembers(&sequenceHeader);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_SecureChannel_processERRChunk(UA_SecureChannel* const channel,
                                 void* const application,
                                 const UA_ByteString* const chunks,
                                 UA_ProcessMessageCallback callback,
                                 size_t* const offset) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    // TODO: Can error messages be encrypted? If so we need to handle it here.
    UA_TcpMessageHeader header;
    retval = UA_TcpMessageHeader_decodeBinary(chunks, offset, &header);
    if(retval != UA_STATUSCODE_GOOD) {
        return retval;
    }

    UA_TcpErrorMessage errorMessage;
    retval = UA_TcpErrorMessage_decodeBinary(chunks, offset, &errorMessage);
    if(retval != UA_STATUSCODE_GOOD) {
        return retval;
    }

    // dirty cast to pass errorMessage
    UA_UInt32 val = 0;
    callback(application, (UA_SecureChannel *)channel, (UA_MessageType)UA_MESSAGETYPE_ERR,
             val, (const UA_ByteString*)&errorMessage);

    return retval;
}

/**
 * \brief Processes a single chunk.
 *
 * If the chunk is final, the callback is called.
 * Otherwise the chunk body is appended to the previously processed ones.
 *
 * \param channel the channel the chunk was recieved on.
 * \param application pointer to application data that gets passed on to the callback.
 * \param callback the callback that is called, when a final chunk is processed.
 * \param chunk the chunk data to be processed. The length will be set to the actual chunk length
 *              after processing the chunk.
 */
static UA_StatusCode
UA_SecureChannel_processChunk(UA_SecureChannel* const channel,
                              void* const application,
                              UA_ProcessMessageCallback callback,
                              UA_ByteString* const chunk) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    size_t processedChunkBytes = 0;

    /* Decode message header */
    UA_SecureConversationMessageHeader messageHeader;
    retval = UA_SecureConversationMessageHeader_decodeBinary(chunk, &processedChunkBytes, &messageHeader);
    if(retval != UA_STATUSCODE_GOOD) {
        return retval;
    }

    if(messageHeader.messageHeader.messageSize > chunk->length) {
        // TODO: Kill channel?
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }

    /* Is the channel attached to connection and not temporary? */
    if(messageHeader.secureChannelId != channel->securityToken.channelId && !channel->temporary) {
        //Service_CloseSecureChannel(server, channel);
        //connection->close(connection);
        return UA_STATUSCODE_BADSECURECHANNELIDINVALID;
    }

    UA_UInt32 requestId = 0;

    size_t bodySize = 0;

    // Process the chunk.
    switch(messageHeader.messageHeader.messageTypeAndChunkType & UA_BITMASK_MESSAGETYPE) {
    case UA_MESSAGETYPE_OPN:
    {
        retval |= UA_SecureChannel_processAsymmetricOPNChunk(chunk, channel, &messageHeader, &processedChunkBytes, &requestId, &bodySize);
        break;
    }
    default:
        retval |= UA_SecureChannel_processSymmetricChunk(chunk, channel, &messageHeader, &processedChunkBytes, &requestId, &bodySize);
        break;
    }

    if(retval != UA_STATUSCODE_GOOD) {
        return retval;
    }

    // Append the chunk and if it is final, call the message handling callback with the complete message
    size_t processed_header = processedChunkBytes;

    const UA_ByteString chunkBody = {
        bodySize,
        chunk->data + processed_header
    };

    switch(messageHeader.messageHeader.messageTypeAndChunkType & UA_BITMASK_CHUNKTYPE) {
    case UA_CHUNKTYPE_INTERMEDIATE:
        UA_SecureChannel_appendChunk(channel,
                                     requestId,
                                     &chunkBody);
        break;
    case UA_CHUNKTYPE_FINAL:
    {
        UA_Boolean realloced = false;
        UA_ByteString message;
        UA_SecureChannel_finalizeChunk(channel,
                                       requestId,
                                       &chunkBody,
                                       &realloced,
                                       &message);
        if(message.length > 0) {
            callback(application,
                (UA_SecureChannel *)channel,
                     (UA_MessageType)(messageHeader.messageHeader.messageTypeAndChunkType & UA_BITMASK_MESSAGETYPE),
                     requestId,
                     &message);
            if(realloced)
                UA_ByteString_deleteMembers(&message);
        }
        break;
    }
    case UA_CHUNKTYPE_ABORT:
        UA_SecureChannel_removeChunk(channel, requestId);
        break;
    default:
        return UA_STATUSCODE_BADDECODINGERROR;
    }

    chunk->length = messageHeader.messageHeader.messageSize;

    return retval;
}

UA_StatusCode
UA_SecureChannel_processChunks(UA_SecureChannel* channel, const UA_ByteString* chunks,
                               UA_ProcessMessageCallback callback, void* application) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    // The offset in the chunks memory region. Always points to the start of a chunk
    size_t offset = 0;
    do {
        if(chunks->length > 3 && chunks->data[offset] == 'E' &&
           chunks->data[offset + 1] == 'R' && chunks->data[offset + 2] == 'R') {
            retval |= UA_SecureChannel_processERRChunk(channel, application, chunks, callback, &offset);
            if(retval != UA_STATUSCODE_GOOD) {
                return retval;
            }
        }
        else {
            // The chunk that is being processed. The length exceeds the actual length of the chunk, since it is not yet known.
            UA_ByteString chunk = {
                chunks->length - offset,
                chunks->data + offset
            };

            UA_SecureChannel_processChunk(channel, application, callback, &chunk);

            // Jump to the end of the chunk. Length of chunk is now known.
            offset += chunk.length;
        }
    } while(chunks->length > offset);

    return retval;
}
