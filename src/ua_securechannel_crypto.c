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
#include <open62541/transport_generated_encoding_binary.h>
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
    const UA_SecurityPolicy *sp = channel->securityPolicy;
    if(!sp)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_LOG_DEBUG_CHANNEL(sp->logger, channel, "Generating new local nonce");

    /* Is the length of the previous nonce correct? */
    size_t nonceLength = sp->symmetricModule.secureChannelNonceLength;
    if(channel->localNonce.length != nonceLength) {
        UA_ByteString_deleteMembers(&channel->localNonce);
        UA_StatusCode retval = UA_ByteString_allocBuffer(&channel->localNonce, nonceLength);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    return sp->symmetricModule.generateNonce(sp, &channel->localNonce);
}

static UA_StatusCode
generateLocalKeys(const UA_SecureChannel *channel,
                  const UA_SecurityPolicy *sp) {
    UA_LOG_TRACE_CHANNEL(sp->logger, channel, "Generating new local keys");
    const UA_SecurityPolicyChannelModule *cm = &sp->channelModule;
    const UA_SecurityPolicySymmetricModule *sm = &sp->symmetricModule;
    const UA_SecurityPolicyCryptoModule *crm = &sm->cryptoModule;
    void *cc = channel->channelContext;

    /* Symmetric key length */
    size_t encrKL = crm->encryptionAlgorithm.getLocalKeyLength(sp, cc);
    size_t encrBS = crm->encryptionAlgorithm.getLocalBlockSize(sp, cc);
    size_t signKL = crm->signatureAlgorithm.getLocalKeyLength(sp, cc);

    const size_t bufSize = encrBS + signKL + encrKL;
    UA_STACKARRAY(UA_Byte, bufBytes, bufSize);
    UA_ByteString buf = {bufSize, bufBytes};

    UA_assert(channel->localNonce.length == sp->symmetricModule.secureChannelNonceLength);
    UA_StatusCode retval = sm->generateKey(sp, &channel->remoteNonce,
                                           &channel->localNonce, &buf);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    const UA_ByteString localSigningKey = {signKL, buf.data};
    retval = cm->setLocalSymSigningKey(cc, &localSigningKey);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    const UA_ByteString localEncryptingKey = {encrKL, &buf.data[signKL]};
    retval = cm->setLocalSymEncryptingKey(cc, &localEncryptingKey);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    const UA_ByteString localIv = {encrBS, &buf.data[signKL + encrKL]};
    return cm->setLocalSymIv(cc, &localIv);
}

static UA_StatusCode
generateRemoteKeys(const UA_SecureChannel *channel,
                   const UA_SecurityPolicy *sp) {
    UA_LOG_TRACE_CHANNEL(sp->logger, channel, "Generating new remote keys");
    const UA_SecurityPolicyChannelModule *cm = &sp->channelModule;
    const UA_SecurityPolicySymmetricModule *sm = &sp->symmetricModule;
    const UA_SecurityPolicyCryptoModule *crm = &sm->cryptoModule;
    void *cc = channel->channelContext;

    /* Symmetric key length */
    size_t encrKL = crm->encryptionAlgorithm.getRemoteKeyLength(sp, cc);
    size_t encrBS = crm->encryptionAlgorithm.getRemoteBlockSize(sp, cc);
    size_t signKL = crm->signatureAlgorithm.getRemoteKeyLength(sp, cc);

    const size_t bufSize = encrBS + signKL + encrKL;
    UA_STACKARRAY(UA_Byte, bufBytes, bufSize);
    UA_ByteString buf = {bufSize, bufBytes};

    /* Remote keys */
    UA_StatusCode retval = sm->generateKey(sp, &channel->localNonce,
                                           &channel->remoteNonce, &buf);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    const UA_ByteString remoteSigningKey = {signKL, buf.data};
    retval = cm->setRemoteSymSigningKey(cc, &remoteSigningKey);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    const UA_ByteString remoteEncryptingKey = {encrKL, &buf.data[signKL]};
    retval = cm->setRemoteSymEncryptingKey(cc, &remoteEncryptingKey);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    const UA_ByteString remoteIv = {encrBS, &buf.data[signKL + encrKL]};
    return cm->setRemoteSymIv(cc, &remoteIv);
}

UA_StatusCode
UA_SecureChannel_generateNewKeys(UA_SecureChannel *channel) {
    const UA_SecurityPolicy *sp = channel->securityPolicy;
    if(!sp)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_LOG_DEBUG_CHANNEL(sp->logger, channel, "Generating SecureChannel keys");

    UA_StatusCode retval = generateLocalKeys(channel, sp);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(sp->logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Could not generate a local key");
        return retval;
    }

    retval = generateRemoteKeys(channel, sp);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(sp->logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Could not generate a remote key");
        return retval;
    }

    return retval;
}

UA_StatusCode
UA_SecureChannel_revolveTokens(UA_SecureChannel *channel) {
    const UA_SecurityPolicy *sp = channel->securityPolicy;
    if(!sp)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(channel->nextSecurityToken.tokenId == 0) /* no next security token issued */
        return UA_STATUSCODE_BADSECURECHANNELTOKENUNKNOWN;

    UA_ChannelSecurityToken_clear(&channel->securityToken);
    channel->securityToken = channel->nextSecurityToken;
    UA_ChannelSecurityToken_init(&channel->nextSecurityToken);

    /* remote keys are generated later on */
    return generateLocalKeys(channel, sp);
}

/***************************/
/* Send Asymmetric Message */
/***************************/

size_t
calculateAsymAlgSecurityHeaderLength(const UA_SecureChannel *channel) {
    const UA_SecurityPolicy *sp = channel->securityPolicy;
    if(!sp)
        return UA_STATUSCODE_BADINTERNALERROR;

    size_t asymHeaderLength = UA_ASYMMETRIC_ALG_SECURITY_HEADER_FIXED_LENGTH +
                              sp->policyUri.length;
    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGN &&
       channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return asymHeaderLength;

    /* OPN is always encrypted even if the mode is sign only */
    asymHeaderLength += 20; /* Thumbprints are always 20 byte long */
    asymHeaderLength += sp->localCertificate.length;
    return asymHeaderLength;
}

UA_StatusCode
prependHeadersAsym(UA_SecureChannel *const channel, UA_Byte *header_pos,
                   const UA_Byte *buf_end, size_t totalLength,
                   size_t securityHeaderLength, UA_UInt32 requestId,
                   size_t *const finalLength) {
    const UA_SecurityPolicy *sp = channel->securityPolicy;
    if(!sp)
        return UA_STATUSCODE_BADINTERNALERROR;

    size_t dataToEncryptLength =
        totalLength - (UA_SECURE_CONVERSATION_MESSAGE_HEADER_LENGTH + securityHeaderLength);

    UA_TcpMessageHeader messageHeader;
    messageHeader.messageTypeAndChunkType = UA_MESSAGETYPE_OPN + UA_CHUNKTYPE_FINAL;
    messageHeader.messageSize = (UA_UInt32)
        (totalLength +
         UA_SecurityPolicy_getRemoteAsymEncryptionBufferLengthOverhead(sp, channel->channelContext,
                                                                       dataToEncryptLength));
    UA_UInt32 secureChannelId = channel->securityToken.channelId;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_encodeBinary(&messageHeader, &UA_TRANSPORT[UA_TRANSPORT_TCPMESSAGEHEADER],
                              &header_pos, &buf_end, NULL, NULL);
    retval |= UA_encodeBinary(&secureChannelId, &UA_TYPES[UA_TYPES_UINT32],
                              &header_pos, &buf_end, NULL, NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_AsymmetricAlgorithmSecurityHeader asymHeader;
    UA_AsymmetricAlgorithmSecurityHeader_init(&asymHeader);
    asymHeader.securityPolicyUri = sp->policyUri;
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        asymHeader.senderCertificate = sp->localCertificate;
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

    *finalLength = messageHeader.messageSize;

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

    const UA_SecurityPolicy *sp = channel->securityPolicy;

    /* Hide bytes for signature and padding */
    size_t potentialEncryptMaxSize = (size_t)(*buf_end - *buf_start) + UA_SEQUENCE_HEADER_LENGTH;
    *buf_end -= sp->asymmetricModule.cryptoModule.signatureAlgorithm.
        getLocalSignatureSize(sp, channel->channelContext);
    *buf_end -= 2; /* padding byte and extraPadding byte */

    /* Add some overhead length due to RSA implementations adding a signature themselves */
    *buf_end -= UA_SecurityPolicy_getRemoteAsymEncryptionBufferLengthOverhead(sp,
                                                                              channel->channelContext,
                                                                              potentialEncryptMaxSize);
#endif
}

#ifdef UA_ENABLE_ENCRYPTION

void
padChunkAsym(UA_SecureChannel *channel, const UA_ByteString *buf,
             size_t securityHeaderLength, UA_Byte **buf_pos) {
    const UA_SecurityPolicy *sp = channel->securityPolicy;

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
    size_t plainTextBlockSize = sp->asymmetricModule.cryptoModule.encryptionAlgorithm.
        getRemotePlainTextBlockSize(sp, channel->channelContext);
    size_t signatureSize = sp->asymmetricModule.cryptoModule.signatureAlgorithm.
        getLocalSignatureSize(sp, channel->channelContext);
    size_t paddingBytes = 1;
    if(sp->asymmetricModule.cryptoModule.encryptionAlgorithm.
        getRemoteKeyLength(sp, channel->channelContext) > 2048)
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
    if(sp->asymmetricModule.cryptoModule.encryptionAlgorithm.
       getRemoteKeyLength(sp, channel->channelContext) > 2048) {
        UA_Byte extraPaddingSize = (UA_Byte)(totalPaddingSize >> 8u);
        **buf_pos = extraPaddingSize;
        ++*buf_pos;
    }
}

UA_StatusCode
signAndEncryptAsym(UA_SecureChannel *channel, size_t preSignLength,
                   UA_ByteString *buf, size_t securityHeaderLength,
                   size_t totalLength) {
    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGN &&
       channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return UA_STATUSCODE_GOOD;

    const UA_SecurityPolicy *sp = channel->securityPolicy;
    
    /* Sign message */
    const UA_ByteString dataToSign = {preSignLength, buf->data};
    size_t sigsize = sp->asymmetricModule.cryptoModule.signatureAlgorithm.
        getLocalSignatureSize(sp, channel->channelContext);
    UA_ByteString signature = {sigsize, buf->data + preSignLength};
    UA_StatusCode retval = sp->asymmetricModule.cryptoModule.signatureAlgorithm.
        sign(sp, channel->channelContext, &dataToSign, &signature);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Specification part 6, 6.7.4: The OpenSecureChannel Messages are
     * signed and encrypted if the SecurityMode is not None (even if the
     * SecurityMode is SignOnly). */
    size_t unencrypted_length =
        UA_SECURE_CONVERSATION_MESSAGE_HEADER_LENGTH + securityHeaderLength;
    UA_ByteString dataToEncrypt = {totalLength - unencrypted_length,
                                   &buf->data[unencrypted_length]};
    return sp->asymmetricModule.cryptoModule.encryptionAlgorithm.
        encrypt(sp, channel->channelContext, &dataToEncrypt);
}

/**************************/
/* Send Symmetric Message */
/**************************/

static UA_UInt16
calculatePaddingSym(const UA_SecurityPolicy *sp, const void *channelContext,
                    size_t bytesToWrite, UA_Byte *paddingSize, UA_Byte *extraPaddingSize) {
    size_t encryptionBlockSize = sp->symmetricModule.cryptoModule.
        encryptionAlgorithm.getLocalBlockSize(sp, channelContext);
    size_t signatureSize = sp->symmetricModule.cryptoModule.signatureAlgorithm.
        getLocalSignatureSize(sp, channelContext);

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

    const UA_SecurityPolicy *sp = channel->securityPolicy;
    UA_ByteString dataToSign = messageContext->messageBuffer;
    dataToSign.length = preSigLength;
    UA_ByteString signature;
    signature.length = sp->symmetricModule.cryptoModule.signatureAlgorithm.
        getLocalSignatureSize(sp, channel->channelContext);
    signature.data = messageContext->buf_pos;

    return sp->symmetricModule.cryptoModule.signatureAlgorithm.
        sign(sp, channel->channelContext, &dataToSign, &signature);
}

UA_StatusCode
encryptChunkSym(UA_MessageContext *const messageContext, size_t totalLength) {
    const UA_SecureChannel *channel = messageContext->channel;
    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return UA_STATUSCODE_GOOD;
        
    UA_ByteString dataToEncrypt;
    dataToEncrypt.data = messageContext->messageBuffer.data + UA_SECUREMH_AND_SYMALGH_LENGTH;
    dataToEncrypt.length = totalLength - UA_SECUREMH_AND_SYMALGH_LENGTH;

    const UA_SecurityPolicy *sp = channel->securityPolicy;
    return sp->symmetricModule.cryptoModule.encryptionAlgorithm.
        encrypt(sp, channel->channelContext, &dataToEncrypt);
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
    const UA_SecurityPolicy *sp = channel->securityPolicy;

    /* Reserve space for the message footer at the end of the chunk if the chunk
     * is signed and/or encrypted. The footer includes the fields PaddingSize,
     * Padding, ExtraPadding and Signature. The padding fields are only present
     * if the chunk is encrypted. */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        mc->buf_end -= sp->symmetricModule.cryptoModule.signatureAlgorithm.
            getLocalSignatureSize(sp, channel->channelContext);

    /* The size of the padding depends on the amount of data that shall be sent
     * and is unknown at this point. Reserve space for the PaddingSize byte,
     * the maximum amount of Padding which equals the block size of the
     * symmetric encryption algorithm and last 1 byte for the ExtraPaddingSize
     * field that is present if the encryption key is larger than 2048 bits.
     * The actual padding size is later calculated by the function
     * calculatePaddingSym(). */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        /* PaddingSize and ExtraPaddingSize fields */
        size_t encryptionBlockSize = sp->symmetricModule.cryptoModule.
            encryptionAlgorithm.getLocalBlockSize(sp, channel->channelContext);
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

static UA_StatusCode
decryptChunk(const UA_SecureChannel *channel,
             const UA_SecurityPolicyCryptoModule *cryptoModule,
             UA_MessageType messageType, UA_ByteString *chunk,
             size_t offset) {
    const UA_SecurityPolicy *sp = channel->securityPolicy;
    UA_LOG_TRACE_CHANNEL(sp->logger, channel, "Decrypting chunk");

    /* Always decrypt opn messages if mode not none */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT ||
       messageType == UA_MESSAGETYPE_OPN) {
        UA_ByteString cipherText = {chunk->length - offset, chunk->data + offset};
        UA_StatusCode retval = cryptoModule->encryptionAlgorithm.
            decrypt(sp, channel->channelContext, &cipherText);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        UA_LOG_TRACE_CHANNEL(sp->logger, channel,
                             "Chunk size before and after decryption: %lu, %lu",
                             (long unsigned int)chunk->length,
                             (long unsigned int)(cipherText.length + offset));
        chunk->length = cipherText.length + offset;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_UInt16
decodeChunkPaddingSize(const UA_SecureChannel *channel,
                       const UA_SecurityPolicyCryptoModule *cryptoModule,
                       UA_MessageType messageType, const UA_ByteString *chunk,
                       size_t sigsize) {
    /* Is padding used? */
    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT &&
       !(messageType == UA_MESSAGETYPE_OPN &&
         !UA_String_equal(&cryptoModule->encryptionAlgorithm.uri, &UA_STRING_NULL)))
        return 0;

    size_t paddingSize = chunk->data[chunk->length - sigsize - 1];

    /* Extra padding size */
    size_t keyLength = cryptoModule->encryptionAlgorithm.
        getLocalKeyLength(channel->securityPolicy, channel->channelContext);
    if(keyLength > 2048) {
        paddingSize <<= 8u;
        paddingSize += 1;
        paddingSize += chunk->data[chunk->length - sigsize - 2];
    }

    /* We need to add one to the padding size since the paddingSize byte itself
     * need to be removed as well. */
    paddingSize += 1;

    UA_LOG_TRACE_CHANNEL(channel->securityPolicy->logger, channel,
                         "Calculated padding size to be %lu",
                         (long unsigned int)paddingSize);
    return (UA_UInt16)paddingSize;
}

static UA_StatusCode
verifySignature(const UA_SecureChannel *channel,
                const UA_SecurityPolicyCryptoModule *cryptoModule,
                const UA_ByteString *chunk, size_t sigsize) {
    UA_LOG_TRACE_CHANNEL(channel->securityPolicy->logger, channel,
                         "Verifying chunk signature");
    if(sigsize >= chunk->length)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    const UA_ByteString content = {chunk->length - sigsize, chunk->data};
    const UA_ByteString sig = {sigsize, chunk->data + chunk->length - sigsize};
    UA_StatusCode retval = cryptoModule->signatureAlgorithm.
        verify(channel->securityPolicy, channel->channelContext, &content, &sig);
#ifdef UA_ENABLE_UNIT_TEST_FAILURE_HOOKS
    retval |= decrypt_verifySignatureFailure;
#endif
    return retval;
}

/* Sets the payload to a pointer inside the chunk buffer. Returns the requestId
 * and the sequenceNumber */
UA_StatusCode
decryptAndVerifyChunk(const UA_SecureChannel *channel,
                      const UA_SecurityPolicyCryptoModule *cryptoModule,
                      UA_MessageType messageType, UA_ByteString *chunk,
                      size_t offset, UA_UInt32 *requestId,
                      UA_UInt32 *sequenceNumber) {
    /* Decrypt the chunk */
    UA_StatusCode retval = decryptChunk(channel, cryptoModule, messageType, chunk, offset);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Verify the chunk signature */
    size_t sigsize = 0;
    size_t paddingSize = 0;
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT ||
       messageType == UA_MESSAGETYPE_OPN) {
        sigsize = cryptoModule->signatureAlgorithm.
            getRemoteSignatureSize(channel->securityPolicy, channel->channelContext);
        retval = verifySignature(channel, cryptoModule, chunk, sigsize);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        paddingSize = decodeChunkPaddingSize(channel, cryptoModule, messageType, chunk, sigsize);
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
    chunk->data += offset;
    chunk->length -= (offset + sigsize + paddingSize);
    UA_LOG_TRACE_CHANNEL(channel->securityPolicy->logger, channel,
                         "Decrypted and verified chunk with request id %u and "
                         "sequence number %u", *requestId, *sequenceNumber);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
processSequenceNumberAsym(UA_SecureChannel *channel, UA_UInt32 sequenceNumber) {
    channel->receiveSequenceNumber = sequenceNumber;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
checkAsymHeader(UA_SecureChannel *channel,
                const UA_AsymmetricAlgorithmSecurityHeader *asymHeader) {
    const UA_SecurityPolicy *sp = channel->securityPolicy;

    if(!UA_ByteString_equal(&sp->policyUri, &asymHeader->securityPolicyUri))
        return UA_STATUSCODE_BADSECURITYPOLICYREJECTED;

    // TODO: Verify certificate using certificate plugin. This will come with a new PR
    /* Something like this
    retval = certificateManager->verify(certificateStore??, &asymHeader->senderCertificate);
    if(retval != UA_STATUSCODE_GOOD)
    return retval;
    */
    return sp->asymmetricModule.
        compareCertificateThumbprint(sp, &asymHeader->receiverCertificateThumbprint);
}

UA_StatusCode
checkSymHeader(UA_SecureChannel *channel, UA_UInt32 tokenId,
               UA_Boolean allowPreviousToken) {
    /* If the message uses a different token, check if it is the next token. */
    if(tokenId != channel->securityToken.tokenId) {
        if(tokenId != channel->nextSecurityToken.tokenId) {
            UA_LOG_WARNING_CHANNEL(channel->securityPolicy->logger, channel,
                                 "Received an unknown SecurityToken");
            return UA_STATUSCODE_BADSECURECHANNELTOKENUNKNOWN;
        }

        UA_LOG_DEBUG_CHANNEL(channel->securityPolicy->logger, channel,
                             "Revolving to the next SecurityToken");

        /* If the token is indeed the next token, revolve the tokens */
        UA_StatusCode retval = UA_SecureChannel_revolveTokens(channel);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING_CHANNEL(channel->securityPolicy->logger, channel,
                                   "Revolving to the next SecurityToken failed");
            return retval;
        }

        /* If the message now uses the currently active token also generate
         * new remote keys to correctly decrypt. */
        retval = generateRemoteKeys(channel, channel->securityPolicy);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING_CHANNEL(channel->securityPolicy->logger, channel,
                                   "Could not generate new remote keys");
            return retval;
        }
    }

    UA_DateTime timeout = channel->securityToken.createdAt +
        (channel->securityToken.revisedLifetime * UA_DATETIME_MSEC);
    if(channel->state == UA_SECURECHANNELSTATE_OPEN &&
       timeout < UA_DateTime_nowMonotonic()) {
        UA_LOG_WARNING_CHANNEL(channel->securityPolicy->logger, channel,
                               "SecurityToken timed out");
        UA_SecureChannel_close(channel);
        return UA_STATUSCODE_BADSECURECHANNELCLOSED;
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
