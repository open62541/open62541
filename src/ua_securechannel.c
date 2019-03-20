/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014, 2016-2017 (c) Florian Palm
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2015 (c) Oleksiy Vasylyev
 *    Copyright 2016 (c) TorbenD
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017-2018 (c) Mark Giraud, Fraunhofer IOSB
 */

#include "ua_securechannel.h"

#include <open62541/plugin/securitypolicy.h>
#include <open62541/transport_generated_encoding_binary.h>
#include <open62541/transport_generated_handling.h>
#include <open62541/types_generated_encoding_binary.h>
#include <open62541/types_generated_handling.h>

#include "ua_types_encoding_binary.h"
#include "ua_util_internal.h"

#define UA_BITMASK_MESSAGETYPE 0x00ffffffu
#define UA_BITMASK_CHUNKTYPE 0xff000000u
#define UA_ASYMMETRIC_ALG_SECURITY_HEADER_FIXED_LENGTH 12
#define UA_SYMMETRIC_ALG_SECURITY_HEADER_LENGTH 4
#define UA_SEQUENCE_HEADER_LENGTH 8
#define UA_SECUREMH_AND_SYMALGH_LENGTH              \
    (UA_SECURE_CONVERSATION_MESSAGE_HEADER_LENGTH + \
    UA_SYMMETRIC_ALG_SECURITY_HEADER_LENGTH)

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

        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    } else {
        UA_LOG_WARNING(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                       "No PKI plugin set. Accepting all certificates");
    }

    retval = securityPolicy->channelModule.
        newContext(securityPolicy, remoteCertificate, &channel->channelContext);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = UA_ByteString_copy(remoteCertificate, &channel->remoteCertificate);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_ByteString remoteCertificateThumbprint = {20, channel->remoteCertificateThumbprint};
    retval = securityPolicy->asymmetricModule.
        makeCertificateThumbprint(securityPolicy, &channel->remoteCertificate,
                                  &remoteCertificateThumbprint);

    if(retval == UA_STATUSCODE_GOOD)
        channel->securityPolicy = securityPolicy;

    return retval;
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
UA_SecureChannel_generateLocalKeys(const UA_SecureChannel *const channel,
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
UA_SecureChannel_generateRemoteKeys(const UA_SecureChannel *const channel,
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
    UA_StatusCode retval =
        UA_SecureChannel_generateLocalKeys(channel, channel->securityPolicy);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(channel->securityPolicy->logger, UA_LOGCATEGORY_SECURECHANNEL,
            "Could not generate a local key");
        return retval;
    }

    retval = UA_SecureChannel_generateRemoteKeys(channel, channel->securityPolicy);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(channel->securityPolicy->logger, UA_LOGCATEGORY_SECURECHANNEL,
            "Could not generate a remote key");
        return retval;
    }

    return retval;
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
    return UA_SecureChannel_generateLocalKeys(channel, channel->securityPolicy);
}
/***************************/
/* Send Asymmetric Message */
/***************************/

static size_t
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

static UA_StatusCode
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

static void
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

static void
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

static UA_StatusCode
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

#endif /* UA_ENABLE_ENCRYPTION */

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

/**************************/
/* Send Symmetric Message */
/**************************/

#ifdef UA_ENABLE_ENCRYPTION

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

static void
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

static UA_StatusCode
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

static UA_StatusCode
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

static void
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
    if(retval != UA_STATUSCODE_GOOD) {
        /* TODO: Send the abort message */
        if(mc->messageBuffer.length > 0) {
            UA_Connection *connection = mc->channel->connection;
            connection->releaseSendBuffer(connection, &mc->messageBuffer);
        }
    }
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

/****************************/
/* Process a received Chunk */
/****************************/

static UA_StatusCode
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

static UA_UInt16
decodeChunkPaddingSize(const UA_SecureChannel *const channel,
                       const UA_SecurityPolicyCryptoModule *const cryptoModule,
                       UA_MessageType const messageType, const UA_ByteString *const chunk,
                       size_t const chunkSizeAfterDecryption, size_t sigsize) {
    /* Is padding used? */
    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT &&
       !(messageType == UA_MESSAGETYPE_OPN && channel->securityMode > UA_MESSAGESECURITYMODE_NONE))
        return 0;

    size_t paddingSize = chunk->data[chunkSizeAfterDecryption - sigsize - 1];

    /* Extra padding size */
    size_t keyLength = cryptoModule->encryptionAlgorithm.
        getRemoteKeyLength(channel->securityPolicy, channel->channelContext);
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

static UA_StatusCode
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

/* Sets the payload to a pointer inside the chunk buffer. Returns the requestId
 * and the sequenceNumber */
static UA_StatusCode
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
processSequenceNumberAsym(UA_SecureChannel *channel, UA_UInt32 sequenceNumber) {
    UA_LOG_TRACE_CHANNEL(channel->securityPolicy->logger, channel,
                         "Sequence Number processed: %i", sequenceNumber);
    channel->receiveSequenceNumber = sequenceNumber;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
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

static UA_StatusCode
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

static UA_StatusCode
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
            retval = UA_SecureChannel_generateRemoteKeys(channel, channel->securityPolicy);
            UA_ChannelSecurityToken_deleteMembers(&channel->previousSecurityToken);
            return retval;
        }
    }

    /* It is possible that the sent messages already use the new token, but
     * the received messages still use the old token. If we receive a message
     * with the new token, we will need to generate the keys and discard the
     * old token now*/
    if(channel->previousSecurityToken.tokenId != 0) {
        UA_StatusCode retval =
            UA_SecureChannel_generateRemoteKeys(channel, channel->securityPolicy);
        UA_ChannelSecurityToken_deleteMembers(&channel->previousSecurityToken);
        return retval;
    }

    return UA_STATUSCODE_GOOD;
}

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
    UA_UInt32 requestId = 0;
    UA_UInt32 sequenceNumber = 0;
    UA_ByteString chunkPayload;
    const UA_SecurityPolicyCryptoModule *cryptoModule = NULL;
    UA_SequenceNumberCallback sequenceNumberCallback = NULL;

    switch(messageType) {
        /* ERR message (not encrypted) */
    case UA_MESSAGETYPE_ERR:
        if(chunkType != UA_CHUNKTYPE_FINAL)
            return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
        chunkPayload.length = chunk->length - offset;
        chunkPayload.data = chunk->data + offset;
        return putPayload(channel, requestId, messageType, chunkType, &chunkPayload);

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
        if(retval != UA_STATUSCODE_GOOD)
            return retval;

        cryptoModule = &channel->securityPolicy->symmetricModule.cryptoModule;
        sequenceNumberCallback = processSequenceNumberSym;
        break;
    }

        /* OPN: Asymmetric encryption */
    case UA_MESSAGETYPE_OPN: {
        /* Chunking not allowed for OPN */
        if(chunkType != UA_CHUNKTYPE_FINAL)
            return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;

        /* Decode the asymmetric algorithm security header and call the callback
         * to perform checks. */
        UA_AsymmetricAlgorithmSecurityHeader asymHeader;
        UA_AsymmetricAlgorithmSecurityHeader_init(&asymHeader);
        offset = UA_SECURE_CONVERSATION_MESSAGE_HEADER_LENGTH;
        retval = UA_AsymmetricAlgorithmSecurityHeader_decodeBinary(chunk, &offset, &asymHeader);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;

        retval = checkAsymHeader(channel, &asymHeader);
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;

        cryptoModule = &channel->securityPolicy->asymmetricModule.cryptoModule;
        sequenceNumberCallback = processSequenceNumberAsym;
        break;
    }

        /* Invalid message type */
    default:return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
    }

    UA_assert(cryptoModule != NULL);
    retval = decryptAndVerifyChunk(channel, cryptoModule, messageType, chunk, offset,
                                   &requestId, &sequenceNumber, &chunkPayload);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Check the sequence number. Skip sequence number checking for fuzzer to
     * improve coverage */
    if(sequenceNumberCallback == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
#if defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
    retval = UA_STATUSCODE_GOOD;
#else
    retval = sequenceNumberCallback(channel, sequenceNumber);
#endif
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    return putPayload(channel, requestId, messageType, chunkType, &chunkPayload);
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
