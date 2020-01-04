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

#include <open62541/types_generated_encoding_binary.h>
#include <open62541/transport_generated_handling.h>

#include "ua_securechannel.h"
#include "ua_util_internal.h"

#define UA_ASYMMETRIC_ALG_SECURITY_HEADER_FIXED_LENGTH 12
#define UA_SEQUENCE_HEADER_LENGTH 8
#define UA_SYMMETRIC_ALG_SECURITY_HEADER_LENGTH 4
#define UA_SECUREMH_AND_SYMALGH_LENGTH              \
    (UA_SECURE_CONVERSATION_MESSAGE_HEADER_LENGTH + \
    UA_SYMMETRIC_ALG_SECURITY_HEADER_LENGTH)

UA_StatusCode
UA_SecureChannel_generateLocalNonce(UA_SecureChannel *channel) {
    if(!channel->securityPolicy)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Is the length of the previous nonce correct? */
    size_t nonceLength = channel->securityPolicy->symmetricModule.secureChannelNonceLength;
    if(channel->localNonce.length != nonceLength) {
        UA_ByteString_deleteMembers(&channel->localNonce);
        UA_StatusCode retval = UA_ByteString_allocBuffer(&channel->localNonce, nonceLength);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    return channel->securityPolicy->symmetricModule.
        generateNonce(channel->securityPolicy, &channel->localNonce);
}

static UA_StatusCode
generateLocalKeys(const UA_SecureChannel *const channel,
                  const UA_SecurityPolicy *const securityPolicy) {
    UA_LOG_TRACE_CHANNEL(securityPolicy->logger, channel, "Generating new local keys");
    const UA_SecurityPolicyChannelModule *channelModule = &securityPolicy->channelModule;
    const UA_SecurityPolicySymmetricModule *symmetricModule = &securityPolicy->symmetricModule;
    const UA_SecurityPolicyCryptoModule *const cryptoModule =
        &securityPolicy->symmetricModule.cryptoModule;

    /* Symmetric key length */
    size_t encryptionKeyLength =
        cryptoModule->encryptionAlgorithm.getLocalKeyLength(securityPolicy, channel->channelContext);
    size_t encryptionBlockSize =
        cryptoModule->encryptionAlgorithm.getLocalBlockSize(securityPolicy, channel->channelContext);
    size_t signingKeyLength =
        cryptoModule->signatureAlgorithm.getLocalKeyLength(securityPolicy, channel->channelContext);
    const size_t bufSize = encryptionBlockSize + signingKeyLength + encryptionKeyLength;
    UA_STACKARRAY(UA_Byte, bufBytes, bufSize);
    UA_ByteString buffer = {bufSize, bufBytes};

    /* Local keys */
    UA_StatusCode retval = symmetricModule->generateKey(securityPolicy, &channel->remoteNonce,
                                                        &channel->localNonce, &buffer);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    const UA_ByteString localSigningKey = {signingKeyLength, buffer.data};
    const UA_ByteString localEncryptingKey = {encryptionKeyLength,
                                              buffer.data + signingKeyLength};
    const UA_ByteString localIv = {encryptionBlockSize,
                                   buffer.data + signingKeyLength +
                                   encryptionKeyLength};

    retval = channelModule->setLocalSymSigningKey(channel->channelContext, &localSigningKey);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = channelModule->setLocalSymEncryptingKey(channel->channelContext, &localEncryptingKey);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = channelModule->setLocalSymIv(channel->channelContext, &localIv);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    return retval;
}

static UA_StatusCode
generateRemoteKeys(const UA_SecureChannel *const channel,
                   const UA_SecurityPolicy *const securityPolicy) {
    UA_LOG_TRACE_CHANNEL(securityPolicy->logger, channel, "Generating new remote keys");
    const UA_SecurityPolicyChannelModule *channelModule = &securityPolicy->channelModule;
    const UA_SecurityPolicySymmetricModule *symmetricModule = &securityPolicy->symmetricModule;
    const UA_SecurityPolicyCryptoModule *const cryptoModule =
        &securityPolicy->symmetricModule.cryptoModule;

    /* Symmetric key length */
    size_t encryptionKeyLength =
        cryptoModule->encryptionAlgorithm.getRemoteKeyLength(securityPolicy, channel->channelContext);
    size_t encryptionBlockSize =
        cryptoModule->encryptionAlgorithm.getRemoteBlockSize(securityPolicy, channel->channelContext);
    size_t signingKeyLength =
        cryptoModule->signatureAlgorithm.getRemoteKeyLength(securityPolicy, channel->channelContext);
    const size_t bufSize = encryptionBlockSize + signingKeyLength + encryptionKeyLength;
    UA_STACKARRAY(UA_Byte, bufBytes, bufSize);
    UA_ByteString buffer = {bufSize, bufBytes};

    /* Remote keys */
    UA_StatusCode retval = symmetricModule->generateKey(securityPolicy, &channel->localNonce,
                                                        &channel->remoteNonce, &buffer);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    const UA_ByteString remoteSigningKey = {signingKeyLength, buffer.data};
    const UA_ByteString remoteEncryptingKey = {encryptionKeyLength,
                                               buffer.data + signingKeyLength};
    const UA_ByteString remoteIv = {encryptionBlockSize,
                                    buffer.data + signingKeyLength +
                                    encryptionKeyLength};

    retval = channelModule->setRemoteSymSigningKey(channel->channelContext, &remoteSigningKey);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = channelModule->setRemoteSymEncryptingKey(channel->channelContext, &remoteEncryptingKey);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = channelModule->setRemoteSymIv(channel->channelContext, &remoteIv);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    return retval;
}

UA_StatusCode
UA_SecureChannel_generateNewKeys(UA_SecureChannel *channel) {
    UA_StatusCode retval = generateLocalKeys(channel, channel->securityPolicy);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(channel->securityPolicy->logger, UA_LOGCATEGORY_SECURECHANNEL,
            "Could not generate a local key");
        return retval;
    }

    retval = generateRemoteKeys(channel, channel->securityPolicy);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(channel->securityPolicy->logger, UA_LOGCATEGORY_SECURECHANNEL,
            "Could not generate a remote key");
        return retval;
    }

    return retval;
}

UA_StatusCode
UA_SecureChannel_revolveTokens(UA_SecureChannel *channel) {
    if(channel->nextSecurityToken.tokenId == 0) // no security token issued
        return UA_STATUSCODE_BADSECURECHANNELTOKENUNKNOWN;

    //FIXME: not thread-safe ???? Why is this not thread safe?
    UA_ChannelSecurityToken_deleteMembers(&channel->previousSecurityToken);
    UA_ChannelSecurityToken_copy(&channel->securityToken, &channel->previousSecurityToken);

    UA_ChannelSecurityToken_deleteMembers(&channel->securityToken);
    UA_ChannelSecurityToken_copy(&channel->nextSecurityToken, &channel->securityToken);

    UA_ChannelSecurityToken_deleteMembers(&channel->nextSecurityToken);
    UA_ChannelSecurityToken_init(&channel->nextSecurityToken);

    /* remote keys are generated later on */
    return generateLocalKeys(channel, channel->securityPolicy);
}

/***************************/
/* Send Asymmetric Message */
/***************************/

size_t
calculateAsymAlgSecurityHeaderLength(const UA_SecureChannel *channel) {
    size_t asymHeaderLength = UA_ASYMMETRIC_ALG_SECURITY_HEADER_FIXED_LENGTH +
                              channel->securityPolicy->policyUri.length;
    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGN &&
       channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return asymHeaderLength;

    /* OPN is always encrypted even if the mode is sign only */
    asymHeaderLength += 20; /* Thumbprints are always 20 byte long */
    asymHeaderLength += channel->securityPolicy->localCertificate.length;
    return asymHeaderLength;
}

UA_StatusCode
prependHeadersAsym(UA_SecureChannel *const channel, UA_Byte *header_pos,
                   const UA_Byte *buf_end, size_t totalLength,
                   size_t securityHeaderLength, UA_UInt32 requestId,
                   size_t *const finalLength) {
    UA_StatusCode retval;
    size_t dataToEncryptLength =
        totalLength - (UA_SECURE_CONVERSATION_MESSAGE_HEADER_LENGTH + securityHeaderLength);

    UA_SecureConversationMessageHeader respHeader;
    respHeader.messageHeader.messageTypeAndChunkType = UA_MESSAGETYPE_OPN + UA_CHUNKTYPE_FINAL;
    respHeader.messageHeader.messageSize = (UA_UInt32)
        (totalLength +
         UA_SecurityPolicy_getRemoteAsymEncryptionBufferLengthOverhead(channel->securityPolicy,
                                                                       channel->channelContext,
                                                                       dataToEncryptLength));
    respHeader.secureChannelId = channel->securityToken.channelId;
    retval = UA_encodeBinary(&respHeader,
                             &UA_TRANSPORT[UA_TRANSPORT_SECURECONVERSATIONMESSAGEHEADER],
                             &header_pos, &buf_end, NULL, NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_AsymmetricAlgorithmSecurityHeader asymHeader;
    UA_AsymmetricAlgorithmSecurityHeader_init(&asymHeader);
    asymHeader.securityPolicyUri = channel->securityPolicy->policyUri;
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        asymHeader.senderCertificate = channel->securityPolicy->localCertificate;
        asymHeader.receiverCertificateThumbprint.length = 20;
        asymHeader.receiverCertificateThumbprint.data = channel->remoteCertificateThumbprint;
    }
    retval = UA_encodeBinary(&asymHeader,
                             &UA_TRANSPORT[UA_TRANSPORT_ASYMMETRICALGORITHMSECURITYHEADER],
                             &header_pos, &buf_end, NULL, NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_SequenceHeader seqHeader;
    seqHeader.requestId = requestId;
    seqHeader.sequenceNumber = UA_atomic_addUInt32(&channel->sendSequenceNumber, 1);
    retval = UA_encodeBinary(&seqHeader, &UA_TRANSPORT[UA_TRANSPORT_SEQUENCEHEADER],
                             &header_pos, &buf_end, NULL, NULL);

    *finalLength = respHeader.messageHeader.messageSize;

    return retval;
}

void
hideBytesAsym(const UA_SecureChannel *channel, UA_Byte **buf_start,
              const UA_Byte **buf_end) {
    *buf_start += UA_SECURE_CONVERSATION_MESSAGE_HEADER_LENGTH;
    *buf_start += calculateAsymAlgSecurityHeaderLength(channel);
    *buf_start += UA_SEQUENCE_HEADER_LENGTH;

#ifdef UA_ENABLE_ENCRYPTION
    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGN &&
       channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return;

    const UA_SecurityPolicy *securityPolicy = channel->securityPolicy;

    /* Hide bytes for signature and padding */
    size_t potentialEncryptMaxSize = (size_t)(*buf_end - *buf_start) + UA_SEQUENCE_HEADER_LENGTH;
    *buf_end -= securityPolicy->asymmetricModule.cryptoModule.signatureAlgorithm.
        getLocalSignatureSize(securityPolicy, channel->channelContext);
    *buf_end -= 2; /* padding byte and extraPadding byte */

    /* Add some overhead length due to RSA implementations adding a signature themselves */
    *buf_end -= UA_SecurityPolicy_getRemoteAsymEncryptionBufferLengthOverhead(securityPolicy,
                                                                              channel->channelContext,
                                                                              potentialEncryptMaxSize);
#endif
}

#ifdef UA_ENABLE_ENCRYPTION

void
padChunkAsym(UA_SecureChannel *channel, const UA_ByteString *const buf,
             size_t securityHeaderLength, UA_Byte **buf_pos) {
    const UA_SecurityPolicy *const securityPolicy = channel->securityPolicy;

    /* Also pad if the securityMode is SIGN_ONLY, since we are using
     * asymmetric communication to exchange keys and thus need to encrypt. */
    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGN &&
       channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return;

    const UA_Byte *buf_body_start =
        &buf->data[UA_SECURE_CONVERSATION_MESSAGE_HEADER_LENGTH +
                   UA_SEQUENCE_HEADER_LENGTH + securityHeaderLength];
    const size_t bytesToWrite =
        (uintptr_t)*buf_pos - (uintptr_t)buf_body_start + UA_SEQUENCE_HEADER_LENGTH;

    /* Compute the padding length */
    size_t plainTextBlockSize = securityPolicy->asymmetricModule.cryptoModule.encryptionAlgorithm.
        getRemotePlainTextBlockSize(securityPolicy, channel->channelContext);
    size_t signatureSize = securityPolicy->asymmetricModule.cryptoModule.signatureAlgorithm.
        getLocalSignatureSize(securityPolicy, channel->channelContext);
    size_t paddingBytes = 1;
    if(securityPolicy->asymmetricModule.cryptoModule.encryptionAlgorithm.
        getRemoteKeyLength(securityPolicy, channel->channelContext) > 2048)
        ++paddingBytes; /* extra padding */
    size_t totalPaddingSize =
        (plainTextBlockSize - ((bytesToWrite + signatureSize + paddingBytes) % plainTextBlockSize));

    /* Write the padding. This is <= because the paddingSize byte also has to be written */
    UA_Byte paddingSize = (UA_Byte)(totalPaddingSize & 0xffu);
    for(UA_UInt16 i = 0; i <= totalPaddingSize; ++i) {
        **buf_pos = paddingSize;
        ++*buf_pos;
    }

    /* Write the extra padding byte if required */
    if(securityPolicy->asymmetricModule.cryptoModule.encryptionAlgorithm.
       getRemoteKeyLength(securityPolicy, channel->channelContext) > 2048) {
        UA_Byte extraPaddingSize = (UA_Byte)(totalPaddingSize >> 8u);
        **buf_pos = extraPaddingSize;
        ++*buf_pos;
    }
}

UA_StatusCode
signAndEncryptAsym(UA_SecureChannel *const channel, size_t preSignLength,
                   UA_ByteString *buf, size_t securityHeaderLength,
                   size_t totalLength) {
    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGN &&
       channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return UA_STATUSCODE_GOOD;

    const UA_SecurityPolicy *const securityPolicy = channel->securityPolicy;
    
    /* Sign message */
    const UA_ByteString dataToSign = {preSignLength, buf->data};
    size_t sigsize = securityPolicy->asymmetricModule.cryptoModule.signatureAlgorithm.
        getLocalSignatureSize(securityPolicy, channel->channelContext);
    UA_ByteString signature = {sigsize, buf->data + preSignLength};
    UA_StatusCode retval = securityPolicy->asymmetricModule.cryptoModule.signatureAlgorithm.
        sign(securityPolicy, channel->channelContext, &dataToSign, &signature);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Specification part 6, 6.7.4: The OpenSecureChannel Messages are
     * signed and encrypted if the SecurityMode is not None (even if the
     * SecurityMode is SignOnly). */
    size_t unencrypted_length =
        UA_SECURE_CONVERSATION_MESSAGE_HEADER_LENGTH + securityHeaderLength;
    UA_ByteString dataToEncrypt = {totalLength - unencrypted_length,
                                   &buf->data[unencrypted_length]};
    return securityPolicy->asymmetricModule.cryptoModule.encryptionAlgorithm.
        encrypt(securityPolicy, channel->channelContext, &dataToEncrypt);
}

/**************************/
/* Send Symmetric Message */
/**************************/

static UA_UInt16
calculatePaddingSym(const UA_SecurityPolicy *securityPolicy, const void *channelContext,
                    size_t bytesToWrite, UA_Byte *paddingSize, UA_Byte *extraPaddingSize) {
    size_t encryptionBlockSize = securityPolicy->symmetricModule.cryptoModule.
        encryptionAlgorithm.getLocalBlockSize(securityPolicy, channelContext);
    size_t signatureSize = securityPolicy->symmetricModule.cryptoModule.signatureAlgorithm.
        getLocalSignatureSize(securityPolicy, channelContext);

    size_t padding = (encryptionBlockSize -
                      ((bytesToWrite + signatureSize + 1) % encryptionBlockSize));
    *paddingSize = (UA_Byte)padding;
    *extraPaddingSize = (UA_Byte)(padding >> 8u);
    return (UA_UInt16)padding;
}

void
padChunkSym(UA_MessageContext *messageContext, size_t bodyLength) {
    if(messageContext->channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return;

    /* The bytes for the padding and signature were removed from buf_end before
     * encoding the payload. So we don't have to check if there is enough
     * space. */

    size_t bytesToWrite = bodyLength + UA_SEQUENCE_HEADER_LENGTH;
    UA_Byte paddingSize = 0;
    UA_Byte extraPaddingSize = 0;
    UA_UInt16 totalPaddingSize =
        calculatePaddingSym(messageContext->channel->securityPolicy,
                            messageContext->channel->channelContext,
                            bytesToWrite, &paddingSize, &extraPaddingSize);

    /* This is <= because the paddingSize byte also has to be written. */
    for(UA_UInt16 i = 0; i <= totalPaddingSize; ++i) {
        *messageContext->buf_pos = paddingSize;
        ++(messageContext->buf_pos);
    }
    if(extraPaddingSize > 0) {
        *messageContext->buf_pos = extraPaddingSize;
        ++(messageContext->buf_pos);
    }
}

UA_StatusCode
signChunkSym(UA_MessageContext *const messageContext, size_t preSigLength) {
    const UA_SecureChannel *channel = messageContext->channel;
    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGN &&
       channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return UA_STATUSCODE_GOOD;

    const UA_SecurityPolicy *securityPolicy = channel->securityPolicy;
    UA_ByteString dataToSign = messageContext->messageBuffer;
    dataToSign.length = preSigLength;
    UA_ByteString signature;
    signature.length = securityPolicy->symmetricModule.cryptoModule.signatureAlgorithm.
        getLocalSignatureSize(securityPolicy, channel->channelContext);
    signature.data = messageContext->buf_pos;

    return securityPolicy->symmetricModule.cryptoModule.signatureAlgorithm.
        sign(securityPolicy, channel->channelContext, &dataToSign, &signature);
}

UA_StatusCode
encryptChunkSym(UA_MessageContext *const messageContext, size_t totalLength) {
    const UA_SecureChannel *channel = messageContext->channel;
    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return UA_STATUSCODE_GOOD;
        
    UA_ByteString dataToEncrypt;
    dataToEncrypt.data = messageContext->messageBuffer.data + UA_SECUREMH_AND_SYMALGH_LENGTH;
    dataToEncrypt.length = totalLength - UA_SECUREMH_AND_SYMALGH_LENGTH;

    const UA_SecurityPolicy *securityPolicy = channel->securityPolicy;
    return securityPolicy->symmetricModule.cryptoModule.encryptionAlgorithm.
        encrypt(securityPolicy, channel->channelContext, &dataToEncrypt);
}

#endif /* UA_ENABLE_ENCRYPTION */

void
setBufPos(UA_MessageContext *mc) {
    /* Forward the data pointer so that the payload is encoded after the
     * message header */
    mc->buf_pos = &mc->messageBuffer.data[UA_SECURE_MESSAGE_HEADER_LENGTH];
    mc->buf_end = &mc->messageBuffer.data[mc->messageBuffer.length];

#ifdef UA_ENABLE_ENCRYPTION
    const UA_SecureChannel *channel = mc->channel;
    const UA_SecurityPolicy *securityPolicy = channel->securityPolicy;

    /* Reserve space for the message footer at the end of the chunk if the chunk
     * is signed and/or encrypted. The footer includes the fields PaddingSize,
     * Padding, ExtraPadding and Signature. The padding fields are only present
     * if the chunk is encrypted. */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        mc->buf_end -= securityPolicy->symmetricModule.cryptoModule.signatureAlgorithm.
            getLocalSignatureSize(securityPolicy, channel->channelContext);

    /* The size of the padding depends on the amount of data that shall be sent
     * and is unknown at this point. Reserve space for the PaddingSize byte,
     * the maximum amount of Padding which equals the block size of the
     * symmetric encryption algorithm and last 1 byte for the ExtraPaddingSize
     * field that is present if the encryption key is larger than 2048 bits.
     * The actual padding size is later calculated by the function
     * calculatePaddingSym(). */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        /* PaddingSize and ExtraPaddingSize fields */
        size_t encryptionBlockSize = securityPolicy->symmetricModule.cryptoModule.
            encryptionAlgorithm.getLocalBlockSize(securityPolicy, channel->channelContext);
        mc->buf_end -= 1 + ((encryptionBlockSize >> 8u) ? 1 : 0);

        /* Reduce the message body size with the remainder of the operation
         * maxEncryptedDataSize modulo EncryptionBlockSize to get a whole
         * number of blocks to encrypt later. Also reserve one byte for
         * padding (1 <= paddingSize <= encryptionBlockSize). */
        size_t maxEncryptDataSize = mc->messageBuffer.length -
            UA_SECURE_CONVERSATION_MESSAGE_HEADER_LENGTH -
            UA_SYMMETRIC_ALG_SECURITY_HEADER_LENGTH;
        mc->buf_end -= (maxEncryptDataSize % encryptionBlockSize) + 1;
    }
#endif
}

/****************************/
/* Process a received Chunk */
/****************************/

UA_StatusCode
decryptChunk(const UA_SecureChannel *const channel,
             const UA_SecurityPolicyCryptoModule *const cryptoModule,
             UA_MessageType const messageType, const UA_ByteString *const chunk,
             size_t const offset, size_t *const chunkSizeAfterDecryption) {
    UA_LOG_TRACE_CHANNEL(channel->securityPolicy->logger, channel, "Decrypting chunk");

    UA_ByteString cipherText = {chunk->length - offset, chunk->data + offset};
    size_t sizeBeforeDecryption = cipherText.length;
    size_t chunkSizeBeforeDecryption = *chunkSizeAfterDecryption;

    /* Always decrypt opn messages if mode not none */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT ||
       messageType == UA_MESSAGETYPE_OPN) {
        UA_StatusCode retval = cryptoModule->encryptionAlgorithm.
            decrypt(channel->securityPolicy, channel->channelContext, &cipherText);
        *chunkSizeAfterDecryption -= (sizeBeforeDecryption - cipherText.length);
        if(retval != UA_STATUSCODE_GOOD) {
            return retval;
        }
    }

    UA_LOG_TRACE_CHANNEL(channel->securityPolicy->logger, channel,
                         "Chunk size before and after decryption: %lu, %lu",
                         (long unsigned int)chunkSizeBeforeDecryption,
                         (long unsigned int)*chunkSizeAfterDecryption);

    return UA_STATUSCODE_GOOD;
}

UA_UInt16
decodeChunkPaddingSize(const UA_SecureChannel *const channel,
                       const UA_SecurityPolicyCryptoModule *const cryptoModule,
                       UA_MessageType const messageType, const UA_ByteString *const chunk,
                       size_t const chunkSizeAfterDecryption, size_t sigsize) {
    /* Is padding used? */
    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT &&
       !(messageType == UA_MESSAGETYPE_OPN &&
         !UA_String_equal(&cryptoModule->encryptionAlgorithm.uri, &UA_STRING_NULL)))
        return 0;

    size_t paddingSize = chunk->data[chunkSizeAfterDecryption - sigsize - 1];

    /* Extra padding size */
    size_t keyLength = cryptoModule->encryptionAlgorithm.
        getLocalKeyLength(channel->securityPolicy, channel->channelContext);
    if(keyLength > 2048) {
        paddingSize <<= 8u;
        paddingSize += 1;
        paddingSize += chunk->data[chunkSizeAfterDecryption - sigsize - 2];
    }

    /* We need to add one to the padding size since the paddingSize byte itself
     * need to be removed as well. */
    paddingSize += 1;

    UA_LOG_TRACE_CHANNEL(channel->securityPolicy->logger, channel,
                         "Calculated padding size to be %lu",
                         (long unsigned int)paddingSize);
    return (UA_UInt16)paddingSize;
}

UA_StatusCode
verifyChunk(const UA_SecureChannel *const channel,
            const UA_SecurityPolicyCryptoModule *const cryptoModule,
            const UA_ByteString *const chunk,
            size_t const chunkSizeAfterDecryption, size_t sigsize) {
    UA_LOG_TRACE_CHANNEL(channel->securityPolicy->logger, channel,
                         "Verifying chunk signature");

    /* Verify the signature */
    const UA_ByteString chunkDataToVerify = {chunkSizeAfterDecryption - sigsize, chunk->data};
    const UA_ByteString signature = {sigsize, chunk->data + chunkSizeAfterDecryption - sigsize};
    UA_StatusCode retval = cryptoModule->signatureAlgorithm.
        verify(channel->securityPolicy, channel->channelContext, &chunkDataToVerify, &signature);
#ifdef UA_ENABLE_UNIT_TEST_FAILURE_HOOKS
    retval |= decrypt_verifySignatureFailure;
#endif

    return retval;
}

UA_StatusCode
processSequenceNumberAsym(UA_SecureChannel *channel, UA_UInt32 sequenceNumber) {
    UA_LOG_TRACE_CHANNEL(channel->securityPolicy->logger, channel,
                         "Sequence Number processed: %i", sequenceNumber);
    channel->receiveSequenceNumber = sequenceNumber;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
processSequenceNumberSym(UA_SecureChannel *channel, UA_UInt32 sequenceNumber) {
    /* Failure mode hook for unit tests */
#ifdef UA_ENABLE_UNIT_TEST_FAILURE_HOOKS
    if(processSym_seqNumberFailure != UA_STATUSCODE_GOOD)
        return processSym_seqNumberFailure;
#endif

    UA_LOG_TRACE_CHANNEL(channel->securityPolicy->logger, channel,
                         "Sequence Number processed: %i", sequenceNumber);
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

UA_StatusCode
checkAsymHeader(UA_SecureChannel *const channel,
                UA_AsymmetricAlgorithmSecurityHeader *const asymHeader) {
    const UA_SecurityPolicy *const securityPolicy = channel->securityPolicy;

    if(!UA_ByteString_equal(&securityPolicy->policyUri,
                            &asymHeader->securityPolicyUri)) {
        return UA_STATUSCODE_BADSECURITYPOLICYREJECTED;
    }

    // TODO: Verify certificate using certificate plugin. This will come with a new PR
    /* Something like this
    retval = certificateManager->verify(certificateStore??, &asymHeader->senderCertificate);
    if(retval != UA_STATUSCODE_GOOD)
    return retval;
    */
    UA_StatusCode retval = securityPolicy->asymmetricModule.
        compareCertificateThumbprint(securityPolicy,
                                     &asymHeader->receiverCertificateThumbprint);
    if(retval != UA_STATUSCODE_GOOD) {
        return retval;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
checkPreviousToken(UA_SecureChannel *const channel, const UA_UInt32 tokenId) {
    if(tokenId != channel->previousSecurityToken.tokenId)
        return UA_STATUSCODE_BADSECURECHANNELTOKENUNKNOWN;

    UA_DateTime timeout = channel->previousSecurityToken.createdAt +
        (UA_DateTime)((UA_Double)channel->previousSecurityToken.revisedLifetime *
                      (UA_Double)UA_DATETIME_MSEC * 1.25);

    if(timeout < UA_DateTime_nowMonotonic())
        return UA_STATUSCODE_BADSECURECHANNELTOKENUNKNOWN;

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
checkSymHeader(UA_SecureChannel *const channel,
               const UA_UInt32 tokenId, UA_Boolean allowPreviousToken) {

    /* If the message uses the currently active token, check if it is still valid */
    if(tokenId == channel->securityToken.tokenId) {
        if(channel->state == UA_SECURECHANNELSTATE_OPEN &&
           (channel->securityToken.createdAt +
            (channel->securityToken.revisedLifetime * UA_DATETIME_MSEC))
           < UA_DateTime_nowMonotonic()) {
            UA_SecureChannel_close(channel);
            return UA_STATUSCODE_BADSECURECHANNELCLOSED;
        }
    }

    /* If the message uses a different token, check if it is the next token. */
    if(tokenId != channel->securityToken.tokenId) {
        /* If it isn't the next token, we might be dealing with a message, that
         * still uses the old token, so check if the old one is still valid.*/
        if(tokenId != channel->nextSecurityToken.tokenId) {
            if(allowPreviousToken)
                return checkPreviousToken(channel, tokenId);

            return UA_STATUSCODE_BADSECURECHANNELTOKENUNKNOWN;
        }
        /* If the token is indeed the next token, revolve the tokens */
        UA_StatusCode retval = UA_SecureChannel_revolveTokens(channel);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;

        /* If the message now uses the currently active token also generate
         * new remote keys to correctly decrypt. */
        if(channel->securityToken.tokenId == tokenId) {
            retval = generateRemoteKeys(channel, channel->securityPolicy);
            UA_ChannelSecurityToken_deleteMembers(&channel->previousSecurityToken);
            UA_ChannelSecurityToken_init(&channel->previousSecurityToken);
            return retval;
        }
    }

    /* It is possible that the sent messages already use the new token, but
     * the received messages still use the old token. If we receive a message
     * with the new token, we will need to generate the keys and discard the
     * old token now*/
    if(channel->previousSecurityToken.tokenId != 0) {
        UA_StatusCode retval = generateRemoteKeys(channel, channel->securityPolicy);
        UA_ChannelSecurityToken_deleteMembers(&channel->previousSecurityToken);
        UA_ChannelSecurityToken_init(&channel->previousSecurityToken);
        return retval;
    }

    return UA_STATUSCODE_GOOD;
}
