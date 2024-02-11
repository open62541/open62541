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

#include <open62541/transport_generated_handling.h>

#include "ua_securechannel.h"
#include "ua_types_encoding_binary.h"

UA_StatusCode
UA_SecureChannel_generateLocalNonce(UA_SecureChannel *channel) {
    const UA_SecurityPolicy *sp = channel->securityPolicy;
    UA_CHECK_MEM(sp, return UA_STATUSCODE_BADINTERNALERROR);
    UA_LOG_DEBUG_CHANNEL(sp->logger, channel, "Generating new local nonce");

    /* Is the length of the previous nonce correct? */
    size_t nonceLength = sp->symmetricModule.secureChannelNonceLength;
    if(channel->localNonce.length != nonceLength) {
        UA_ByteString_clear(&channel->localNonce);
        UA_StatusCode res = UA_ByteString_allocBuffer(&channel->localNonce, nonceLength);
        UA_CHECK_STATUS(res, return res);
    }

    /* Generate the nonce */
    return sp->symmetricModule.generateNonce(sp->policyContext, &channel->localNonce);
}

UA_StatusCode
UA_SecureChannel_generateLocalKeys(const UA_SecureChannel *channel) {
    const UA_SecurityPolicy *sp = channel->securityPolicy;
    UA_CHECK_MEM(sp, return UA_STATUSCODE_BADINTERNALERROR);
    UA_LOG_TRACE_CHANNEL(sp->logger, channel, "Generating new local keys");

    void *cc = channel->channelContext;
    const UA_SecurityPolicyChannelModule *cm = &sp->channelModule;
    const UA_SecurityPolicySymmetricModule *sm = &sp->symmetricModule;
    const UA_SecurityPolicyCryptoModule *crm = &sm->cryptoModule;

    /* Generate symmetric key buffer of the required length. The block size is
     * identical for local/remote. */
    UA_ByteString buf;
    size_t encrKL = crm->encryptionAlgorithm.getLocalKeyLength(cc);
    size_t encrBS = crm->encryptionAlgorithm.getRemoteBlockSize(cc);
    size_t signKL = crm->signatureAlgorithm.getLocalKeyLength(cc);
    if(encrBS + signKL + encrKL == 0)
        return UA_STATUSCODE_GOOD; /* No keys to generate */

    UA_StatusCode retval = UA_ByteString_allocBuffer(&buf, encrBS + signKL + encrKL);
    UA_CHECK_STATUS(retval, return retval);
    UA_ByteString localSigningKey = {signKL, buf.data};
    UA_ByteString localEncryptingKey = {encrKL, &buf.data[signKL]};
    UA_ByteString localIv = {encrBS, &buf.data[signKL + encrKL]};

    /* Generate key */
    retval = sm->generateKey(sp->policyContext, &channel->remoteNonce,
                             &channel->localNonce, &buf);
    UA_CHECK_STATUS(retval, goto error);

    /* Set the channel context */
    retval |= cm->setLocalSymSigningKey(cc, &localSigningKey);
    retval |= cm->setLocalSymEncryptingKey(cc, &localEncryptingKey);
    retval |= cm->setLocalSymIv(cc, &localIv);

 error:
    UA_CHECK_STATUS(retval, UA_LOG_WARNING_CHANNEL(sp->logger, channel,
                            "Could not generate local keys (statuscode: %s)",
                            UA_StatusCode_name(retval)));
    UA_ByteString_clear(&buf);
    return retval;
}

UA_StatusCode
generateRemoteKeys(const UA_SecureChannel *channel) {
    const UA_SecurityPolicy *sp = channel->securityPolicy;
    UA_CHECK_MEM(sp, return UA_STATUSCODE_BADINTERNALERROR);
    UA_LOG_TRACE_CHANNEL(sp->logger, channel, "Generating new remote keys");

    void *cc = channel->channelContext;
    const UA_SecurityPolicyChannelModule *cm = &sp->channelModule;
    const UA_SecurityPolicySymmetricModule *sm = &sp->symmetricModule;
    const UA_SecurityPolicyCryptoModule *crm = &sm->cryptoModule;

    /* Generate symmetric key buffer of the required length */
    UA_ByteString buf;
    size_t encrKL = crm->encryptionAlgorithm.getRemoteKeyLength(cc);
    size_t encrBS = crm->encryptionAlgorithm.getRemoteBlockSize(cc);
    size_t signKL = crm->signatureAlgorithm.getRemoteKeyLength(cc);
    if(encrBS + signKL + encrKL == 0)
        return UA_STATUSCODE_GOOD; /* No keys to generate */

    UA_StatusCode retval = UA_ByteString_allocBuffer(&buf, encrBS + signKL + encrKL);
    UA_CHECK_STATUS(retval, return retval);
    UA_ByteString remoteSigningKey = {signKL, buf.data};
    UA_ByteString remoteEncryptingKey = {encrKL, &buf.data[signKL]};
    UA_ByteString remoteIv = {encrBS, &buf.data[signKL + encrKL]};

    /* Generate key */
    retval = sm->generateKey(sp->policyContext, &channel->localNonce,
                             &channel->remoteNonce, &buf);
    UA_CHECK_STATUS(retval, goto error);

    /* Set the channel context */
    retval |= cm->setRemoteSymSigningKey(cc, &remoteSigningKey);
    retval |= cm->setRemoteSymEncryptingKey(cc, &remoteEncryptingKey);
    retval |= cm->setRemoteSymIv(cc, &remoteIv);

 error:
    UA_CHECK_STATUS(retval, UA_LOG_WARNING_CHANNEL(sp->logger, channel,
                            "Could not generate remote keys (statuscode: %s)",
                            UA_StatusCode_name(retval)));
    UA_ByteString_clear(&buf);
    return retval;
}

/***************************/
/* Send Asymmetric Message */
/***************************/

/* The length of the static header content */
#define UA_SECURECHANNEL_ASYMMETRIC_SECURITYHEADER_FIXED_LENGTH 12

size_t
calculateAsymAlgSecurityHeaderLength(const UA_SecureChannel *channel) {
    const UA_SecurityPolicy *sp = channel->securityPolicy;
    UA_CHECK_MEM(sp, return UA_STATUSCODE_BADINTERNALERROR);

    size_t asymHeaderLength = UA_SECURECHANNEL_ASYMMETRIC_SECURITYHEADER_FIXED_LENGTH +
                              sp->policyUri.length;
    if(channel->securityMode == UA_MESSAGESECURITYMODE_NONE)
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
                   size_t *const encryptedLength) {
    const UA_SecurityPolicy *sp = channel->securityPolicy;
    UA_CHECK_MEM(sp, return UA_STATUSCODE_BADINTERNALERROR);

    if(channel->securityMode == UA_MESSAGESECURITYMODE_NONE) {
        *encryptedLength = totalLength;
    } else {
        size_t dataToEncryptLength = totalLength -
            (UA_SECURECHANNEL_CHANNELHEADER_LENGTH + securityHeaderLength);
        size_t plainTextBlockSize = sp->asymmetricModule.cryptoModule.
            encryptionAlgorithm.getRemotePlainTextBlockSize(channel->channelContext);
        size_t encryptedBlockSize = sp->asymmetricModule.cryptoModule.
            encryptionAlgorithm.getRemoteBlockSize(channel->channelContext);

        /* Padding always fills up the last block */
        UA_assert(dataToEncryptLength % plainTextBlockSize == 0);
        size_t blocks = dataToEncryptLength / plainTextBlockSize;
        *encryptedLength = totalLength + blocks * (encryptedBlockSize - plainTextBlockSize);
    }

    UA_TcpMessageHeader messageHeader;
    messageHeader.messageTypeAndChunkType = UA_MESSAGETYPE_OPN + UA_CHUNKTYPE_FINAL;
    messageHeader.messageSize = (UA_UInt32)*encryptedLength;
    UA_UInt32 secureChannelId = channel->securityToken.channelId;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_encodeBinaryInternal(&messageHeader,
                                      &UA_TRANSPORT[UA_TRANSPORT_TCPMESSAGEHEADER],
                                      &header_pos, &buf_end, NULL, NULL);
    retval |= UA_UInt32_encodeBinary(&secureChannelId, &header_pos, buf_end);
    UA_CHECK_STATUS(retval, return retval);

    UA_AsymmetricAlgorithmSecurityHeader asymHeader;
    UA_AsymmetricAlgorithmSecurityHeader_init(&asymHeader);
    asymHeader.securityPolicyUri = sp->policyUri;
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        asymHeader.senderCertificate = sp->localCertificate;
        asymHeader.receiverCertificateThumbprint.length = 20;
        asymHeader.receiverCertificateThumbprint.data = channel->remoteCertificateThumbprint;
    }
    retval = UA_encodeBinaryInternal(&asymHeader,
                &UA_TRANSPORT[UA_TRANSPORT_ASYMMETRICALGORITHMSECURITYHEADER],
                &header_pos, &buf_end, NULL, NULL);
    UA_CHECK_STATUS(retval, return retval);

    /* Increase the sequence number in the channel */
    channel->sendSequenceNumber++;

    UA_SequenceHeader seqHeader;
    seqHeader.requestId = requestId;
    seqHeader.sequenceNumber = channel->sendSequenceNumber;
    retval = UA_encodeBinaryInternal(&seqHeader, &UA_TRANSPORT[UA_TRANSPORT_SEQUENCEHEADER],
                                     &header_pos, &buf_end, NULL, NULL);
    return retval;
}

void
hideBytesAsym(const UA_SecureChannel *channel, UA_Byte **buf_start,
              const UA_Byte **buf_end) {
    /* Set buf_start to the beginning of the payload body */
    *buf_start += UA_SECURECHANNEL_CHANNELHEADER_LENGTH;
    *buf_start += calculateAsymAlgSecurityHeaderLength(channel);
    *buf_start += UA_SECURECHANNEL_SEQUENCEHEADER_LENGTH;

    if(channel->securityMode == UA_MESSAGESECURITYMODE_NONE)
        return;

    /* Make space for the certificate */
    const UA_SecurityPolicy *sp = channel->securityPolicy;
    *buf_end -= sp->asymmetricModule.cryptoModule.signatureAlgorithm.
        getLocalSignatureSize(channel->channelContext);

    /* Block sizes depend on the remote key (certificate) */
    size_t plainTextBlockSize = sp->asymmetricModule.cryptoModule.
        encryptionAlgorithm.getRemotePlainTextBlockSize(channel->channelContext);
    size_t encryptedBlockSize = sp->asymmetricModule.cryptoModule.
        encryptionAlgorithm.getRemoteBlockSize(channel->channelContext);
    UA_Boolean extraPadding = (sp->asymmetricModule.cryptoModule.encryptionAlgorithm.
                               getRemoteKeyLength(channel->channelContext) > 2048);

    /* Compute the maximum number of encrypted blocks that can fit entirely
     * before the signature. From that compute the maximum usable plaintext
     * size. */
    size_t maxEncrypted = (size_t)(*buf_end - *buf_start) +
        UA_SECURECHANNEL_SEQUENCEHEADER_LENGTH;
    size_t max_blocks = maxEncrypted / encryptedBlockSize;
    size_t paddingBytes = (UA_LIKELY(!extraPadding)) ? 1u : 2u;
    *buf_end = *buf_start + (max_blocks * plainTextBlockSize) -
        UA_SECURECHANNEL_SEQUENCEHEADER_LENGTH - paddingBytes;
}

/* Assumes that pos can be advanced to the end of the current block */
void
padChunk(UA_SecureChannel *channel, const UA_SecurityPolicyCryptoModule *cm,
         const UA_Byte *start, UA_Byte **pos) {
    const size_t bytesToWrite = (uintptr_t)*pos - (uintptr_t)start;
    size_t signatureSize = cm->signatureAlgorithm.
        getLocalSignatureSize(channel->channelContext);
    size_t plainTextBlockSize = cm->encryptionAlgorithm.
        getRemotePlainTextBlockSize(channel->channelContext);
    UA_Boolean extraPadding = (cm->encryptionAlgorithm.
        getRemoteKeyLength(channel->channelContext) > 2048);
    size_t paddingBytes = (UA_LIKELY(!extraPadding)) ? 1u : 2u;

    size_t lastBlock = ((bytesToWrite + signatureSize + paddingBytes) % plainTextBlockSize);
    size_t paddingLength = (lastBlock != 0) ? plainTextBlockSize - lastBlock : 0;

    UA_LOG_TRACE_CHANNEL(channel->securityPolicy->logger, channel,
                         "Add %lu bytes of padding plus %lu padding size bytes",
                         (long unsigned int)paddingLength,
                         (long unsigned int)paddingBytes);

    /* Write the padding. This is <= because the paddingSize byte also has to be
     * written */
    UA_Byte paddingByte = (UA_Byte)paddingLength;
    for(UA_UInt16 i = 0; i <= paddingLength; ++i) {
        **pos = paddingByte;
        ++*pos;
    }

    /* Write the extra padding byte if required */
    if(extraPadding) {
        **pos = (UA_Byte)(paddingLength >> 8u);
        ++*pos;
    }
}

UA_StatusCode
signAndEncryptAsym(UA_SecureChannel *channel, size_t preSignLength,
                   UA_ByteString *buf, size_t securityHeaderLength,
                   size_t totalLength) {
    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGN &&
       channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return UA_STATUSCODE_GOOD;

    /* Sign message */
    const UA_SecurityPolicy *sp = channel->securityPolicy;
    const UA_ByteString dataToSign = {preSignLength, buf->data};
    size_t sigsize = sp->asymmetricModule.cryptoModule.signatureAlgorithm.
        getLocalSignatureSize(channel->channelContext);
    UA_ByteString signature = {sigsize, buf->data + preSignLength};
    UA_StatusCode retval = sp->asymmetricModule.cryptoModule.signatureAlgorithm.
        sign(channel->channelContext, &dataToSign, &signature);
    UA_CHECK_STATUS(retval, return retval);

    /* Specification part 6, 6.7.4: The OpenSecureChannel Messages are
     * signed and encrypted if the SecurityMode is not None (even if the
     * SecurityMode is SignOnly). */
    size_t unencrypted_length =
        UA_SECURECHANNEL_CHANNELHEADER_LENGTH + securityHeaderLength;
    UA_ByteString dataToEncrypt = {totalLength - unencrypted_length,
                                   &buf->data[unencrypted_length]};
    return sp->asymmetricModule.cryptoModule.encryptionAlgorithm.
        encrypt(channel->channelContext, &dataToEncrypt);
}

/**************************/
/* Send Symmetric Message */
/**************************/

UA_StatusCode
signAndEncryptSym(UA_MessageContext *messageContext,
                  size_t preSigLength, size_t totalLength) {
    const UA_SecureChannel *channel = messageContext->channel;
    if(channel->securityMode == UA_MESSAGESECURITYMODE_NONE)
        return UA_STATUSCODE_GOOD;

    /* Sign */
    const UA_SecurityPolicy *sp = channel->securityPolicy;
    UA_ByteString dataToSign = messageContext->messageBuffer;
    dataToSign.length = preSigLength;
    UA_ByteString signature;
    signature.length = sp->symmetricModule.cryptoModule.signatureAlgorithm.
        getLocalSignatureSize(channel->channelContext);
    signature.data = messageContext->buf_pos;
    UA_StatusCode res = sp->symmetricModule.cryptoModule.signatureAlgorithm.
        sign(channel->channelContext, &dataToSign, &signature);
    UA_CHECK_STATUS(res, return res);

    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return UA_STATUSCODE_GOOD;

    /* Encrypt */
    UA_ByteString dataToEncrypt;
    dataToEncrypt.data = messageContext->messageBuffer.data +
        UA_SECURECHANNEL_CHANNELHEADER_LENGTH +
        UA_SECURECHANNEL_SYMMETRIC_SECURITYHEADER_LENGTH;
    dataToEncrypt.length = totalLength -
        (UA_SECURECHANNEL_CHANNELHEADER_LENGTH +
         UA_SECURECHANNEL_SYMMETRIC_SECURITYHEADER_LENGTH);
    return sp->symmetricModule.cryptoModule.encryptionAlgorithm.
        encrypt(channel->channelContext, &dataToEncrypt);
}

void
setBufPos(UA_MessageContext *mc) {
    /* Forward the data pointer so that the payload is encoded after the message
     * header. This has to be a symmetric message because OPN (with asymmetric
     * encryption) does not support chunking. */
    mc->buf_pos = &mc->messageBuffer.data[UA_SECURECHANNEL_SYMMETRIC_HEADER_TOTALLENGTH];
    mc->buf_end = &mc->messageBuffer.data[mc->messageBuffer.length];

    if(mc->channel->securityMode == UA_MESSAGESECURITYMODE_NONE)
        return;

    const UA_SecureChannel *channel = mc->channel;
    const UA_SecurityPolicy *sp = channel->securityPolicy;
    size_t sigsize = sp->symmetricModule.cryptoModule.signatureAlgorithm.
        getLocalSignatureSize(channel->channelContext);
    size_t plainBlockSize = sp->symmetricModule.cryptoModule.
        encryptionAlgorithm.getRemotePlainTextBlockSize(channel->channelContext);

    /* Assuming that for symmetric encryption the plainTextBlockSize ==
     * cypherTextBlockSize. For symmetric encryption the remote/local block
     * sizes are identical. */
    UA_assert(sp->symmetricModule.cryptoModule.encryptionAlgorithm.
              getRemoteBlockSize(channel->channelContext) == plainBlockSize);

    /* Leave enough space for the signature and padding */
    mc->buf_end -= sigsize;
    mc->buf_end -= mc->messageBuffer.length % plainBlockSize;

    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        /* Reserve space for the padding bytes */
        UA_Boolean extraPadding =
            (sp->symmetricModule.cryptoModule.encryptionAlgorithm.
             getRemoteKeyLength(channel->channelContext) > 2048);
        mc->buf_end -= (UA_LIKELY(!extraPadding)) ? 1 : 2;
    }

    UA_LOG_TRACE_CHANNEL(sp->logger, channel,
                         "Prepare a symmetric message buffer of length %lu "
                         "with a usable maximum payload length of %lu",
                         (long unsigned)mc->messageBuffer.length,
                         (long unsigned)((uintptr_t)mc->buf_end -
                                         (uintptr_t)mc->messageBuffer.data));
}

/****************************/
/* Process a received Chunk */
/****************************/

static size_t
decodePadding(const UA_SecureChannel *channel,
              const UA_SecurityPolicyCryptoModule *cryptoModule,
              const UA_ByteString *chunk, size_t sigsize) {
    /* Read the byte with the padding size */
    size_t paddingSize = chunk->data[chunk->length - sigsize - 1];

    /* Extra padding size */
    if(cryptoModule->encryptionAlgorithm.
       getLocalKeyLength(channel->channelContext) > 2048) {
        paddingSize <<= 8u;
        paddingSize += chunk->data[chunk->length - sigsize - 2];
        paddingSize += 1; /* Extra padding byte itself */
    }

    /* Add one since the paddingSize byte itself needs to be removed as well */
    return paddingSize + 1;
}

static UA_StatusCode
verifySignature(const UA_SecureChannel *channel,
                const UA_SecurityPolicyCryptoModule *cryptoModule,
                const UA_ByteString *chunk, size_t sigsize) {
    UA_LOG_TRACE_CHANNEL(channel->securityPolicy->logger, channel,
                         "Verifying chunk signature");
    UA_CHECK(sigsize < chunk->length, return UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    const UA_ByteString content = {chunk->length - sigsize, chunk->data};
    const UA_ByteString sig = {sigsize, chunk->data + chunk->length - sigsize};
    UA_StatusCode retval = cryptoModule->signatureAlgorithm.
        verify(channel->channelContext, &content, &sig);
    return retval;
}

/* Sets the payload to a pointer inside the chunk buffer. Returns the requestId
 * and the sequenceNumber */
UA_StatusCode
decryptAndVerifyChunk(const UA_SecureChannel *channel,
                      const UA_SecurityPolicyCryptoModule *cryptoModule,
                      UA_MessageType messageType, UA_ByteString *chunk,
                      size_t offset) {
    /* Decrypt the chunk */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT ||
       messageType == UA_MESSAGETYPE_OPN) {
        UA_ByteString cipher = {chunk->length - offset, chunk->data + offset};
        res = cryptoModule->encryptionAlgorithm.decrypt(channel->channelContext, &cipher);
        UA_CHECK_STATUS(res, return res);
        chunk->length = cipher.length + offset;
    }

    /* Does the message have a signature? */
    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGN &&
       channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT &&
       messageType != UA_MESSAGETYPE_OPN)
        return UA_STATUSCODE_GOOD;

    /* Verify the chunk signature */
    size_t sigsize = cryptoModule->signatureAlgorithm.
        getRemoteSignatureSize(channel->channelContext);
    res = verifySignature(channel, cryptoModule, chunk, sigsize);
    UA_CHECK_STATUS(res,
       UA_LOG_WARNING_CHANNEL(channel->securityPolicy->logger, channel,
                              "Could not verify the signature"); return res);

    /* Compute the padding if the payload as encrypted */
    size_t padSize = 0;
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT ||
       (messageType == UA_MESSAGETYPE_OPN &&
        cryptoModule->encryptionAlgorithm.uri.length > 0)) {
        padSize = decodePadding(channel, cryptoModule, chunk, sigsize);
        UA_LOG_TRACE_CHANNEL(channel->securityPolicy->logger, channel,
                             "Calculated padding size to be %lu",
                             (long unsigned)padSize);
    }

    /* Verify the content length. The encrypted payload has to be at least 9
     * bytes long: 8 byte for the SequenceHeader and one byte for the actual
     * message */
    UA_CHECK(offset + padSize + sigsize + 9 < chunk->length,
             UA_LOG_WARNING_CHANNEL(channel->securityPolicy->logger, channel,
                                    "Impossible padding value");
             return UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    /* Hide the signature and padding */
    chunk->length -= (sigsize + padSize);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
checkAsymHeader(UA_SecureChannel *channel,
                const UA_AsymmetricAlgorithmSecurityHeader *asymHeader) {
    const UA_SecurityPolicy *sp = channel->securityPolicy;
    if(!UA_ByteString_equal(&sp->policyUri, &asymHeader->securityPolicyUri))
        return UA_STATUSCODE_BADSECURITYPOLICYREJECTED;

    return sp->asymmetricModule.
        compareCertificateThumbprint(sp, &asymHeader->receiverCertificateThumbprint);

    /* The certificate in the header is verified via the configured PKI plugin
     * as certificateVerification.verifyCertificate(...). We cannot do it here
     * because the client/server context is needed. */
}

UA_StatusCode
checkSymHeader(UA_SecureChannel *channel, const UA_UInt32 tokenId,
               UA_DateTime nowMonotonic) {
    /* If no match, try to revolve to the next token after a
     * RenewSecureChannel */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ChannelSecurityToken *token = &channel->securityToken;
    switch(channel->renewState) {
    case UA_SECURECHANNELRENEWSTATE_NORMAL:
    case UA_SECURECHANNELRENEWSTATE_SENT:
    default:
        break;

    case UA_SECURECHANNELRENEWSTATE_NEWTOKEN_SERVER:
        /* Old token still in use */
        if(tokenId == channel->securityToken.tokenId)
            break;

        /* Not the new token */
        UA_CHECK(tokenId == channel->altSecurityToken.tokenId,
                 UA_LOG_WARNING_CHANNEL(channel->securityPolicy->logger, channel,
                                        "Unknown SecurityToken");
                 return UA_STATUSCODE_BADSECURECHANNELTOKENUNKNOWN);

        /* Roll over to the new token, generate new local and remote keys */
        channel->renewState = UA_SECURECHANNELRENEWSTATE_NORMAL;
        channel->securityToken = channel->altSecurityToken;
        UA_ChannelSecurityToken_init(&channel->altSecurityToken);
        retval |= UA_SecureChannel_generateLocalKeys(channel);
        retval |= generateRemoteKeys(channel);
        UA_CHECK_STATUS(retval, return retval);
        break;

    case UA_SECURECHANNELRENEWSTATE_NEWTOKEN_CLIENT:
        /* The server is still using the old token. That's okay. */
        if(tokenId == channel->altSecurityToken.tokenId) {
            token = &channel->altSecurityToken;
            break;
        }

        /* Not the new token */
        UA_CHECK(tokenId == channel->securityToken.tokenId,
                 UA_LOG_WARNING_CHANNEL(channel->securityPolicy->logger, channel,
                                        "Unknown SecurityToken");
                 return UA_STATUSCODE_BADSECURECHANNELTOKENUNKNOWN);

        /* The remote server uses the new token for the first time. Delete the
         * old token and roll the remote key over. The local key already uses
         * the nonce pair from the last OPN exchange. */
        channel->renewState = UA_SECURECHANNELRENEWSTATE_NORMAL;
        UA_ChannelSecurityToken_init(&channel->altSecurityToken);
        retval = generateRemoteKeys(channel);
        UA_CHECK_STATUS(retval, return retval);
    }

    UA_DateTime timeout = token->createdAt + (token->revisedLifetime * UA_DATETIME_MSEC);
    if(channel->state == UA_SECURECHANNELSTATE_OPEN &&
       timeout < nowMonotonic) {
        UA_LOG_WARNING_CHANNEL(channel->securityPolicy->logger, channel,
                               "SecurityToken timed out");
        UA_SecureChannel_shutdown(channel, UA_SHUTDOWNREASON_TIMEOUT);
        return UA_STATUSCODE_BADSECURECHANNELCLOSED;
    }

    return UA_STATUSCODE_GOOD;
}

UA_Boolean
UA_SecureChannel_checkTimeout(UA_SecureChannel *channel, UA_DateTime nowMonotonic) {
    /* Compute the timeout date of the SecurityToken */
    UA_DateTime timeout = channel->securityToken.createdAt +
        (UA_DateTime)(channel->securityToken.revisedLifetime * UA_DATETIME_MSEC);

    /* The token has timed out. Try to do the token revolving now instead of
     * shutting the channel down.
     *
     * Part 4, 5.5.2 says: Servers shall use the existing SecurityToken to
     * secure outgoing Messages until the SecurityToken expires or the
     * Server receives a Message secured with a new SecurityToken.*/
    if(timeout < nowMonotonic && channel->renewState == UA_SECURECHANNELRENEWSTATE_NEWTOKEN_SERVER) {
        /* Revolve the token manually. This is otherwise done in checkSymHeader. */
        channel->renewState = UA_SECURECHANNELRENEWSTATE_NORMAL;
        channel->securityToken = channel->altSecurityToken;
        UA_ChannelSecurityToken_init(&channel->altSecurityToken);
        UA_SecureChannel_generateLocalKeys(channel);
        generateRemoteKeys(channel);

        /* Use the timeout of the new SecurityToken */
        timeout = channel->securityToken.createdAt +
            (UA_DateTime)(channel->securityToken.revisedLifetime * UA_DATETIME_MSEC);
    }

    return (timeout < nowMonotonic);
}
