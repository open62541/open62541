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

#include "ua_util_internal.h"
#include "ua_securechannel.h"
#include "ua_types_encoding_binary.h"
#include "ua_types_generated_encoding_binary.h"
#include "ua_transport_generated_encoding_binary.h"
#include "ua_types_generated_handling.h"
#include "ua_transport_generated_handling.h"
#include "ua_plugin_securitypolicy.h"

#define UA_BITMASK_MESSAGETYPE 0x00ffffff
#define UA_BITMASK_CHUNKTYPE 0xff000000
#define UA_ASYMMETRIC_ALG_SECURITY_HEADER_FIXED_LENGTH 12
#define UA_SYMMETRIC_ALG_SECURITY_HEADER_LENGTH 4
#define UA_SEQUENCE_HEADER_LENGTH 8
#define UA_SECUREMH_AND_SYMALGH_LENGTH              \
    (UA_SECURE_CONVERSATION_MESSAGE_HEADER_LENGTH + \
    UA_SYMMETRIC_ALG_SECURITY_HEADER_LENGTH)

const UA_ByteString
    UA_SECURITY_POLICY_NONE_URI = {47, (UA_Byte *)"http://opcfoundation.org/UA/SecurityPolicy#None"};

#ifdef UA_ENABLE_UNIT_TEST_FAILURE_HOOKS
UA_StatusCode decrypt_verifySignatureFailure;
UA_StatusCode sendAsym_sendFailure;
UA_StatusCode processSym_seqNumberFailure;
#endif

UA_StatusCode
UA_SecureChannel_init(UA_SecureChannel *channel,
                      const UA_SecurityPolicy *securityPolicy,
                      const UA_ByteString *remoteCertificate) {
    if(channel == NULL || securityPolicy == NULL || remoteCertificate == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Linked lists are also initialized by zeroing out */
    memset(channel, 0, sizeof(UA_SecureChannel));
    channel->state = UA_SECURECHANNELSTATE_FRESH;
    channel->securityPolicy = securityPolicy;

    UA_StatusCode retval;
    if(channel->securityPolicy->certificateVerification != NULL) {
        retval = channel->securityPolicy->certificateVerification->
            verifyCertificate(channel->securityPolicy->certificateVerification->context, remoteCertificate);

        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    } else {
        UA_LOG_WARNING(channel->securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY, "No PKI plugin set. "
            "Accepting all certificates");
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

    return retval;
}

void
UA_SecureChannel_deleteMembersCleanup(UA_SecureChannel *channel) {
    /* Delete members */
    UA_ByteString_deleteMembers(&channel->remoteCertificate);
    UA_ByteString_deleteMembers(&channel->localNonce);
    UA_ByteString_deleteMembers(&channel->remoteNonce);
    UA_ChannelSecurityToken_deleteMembers(&channel->securityToken);
    UA_ChannelSecurityToken_deleteMembers(&channel->nextSecurityToken);

    /* Delete the channel context for the security policy */
    if(channel->securityPolicy)
        channel->securityPolicy->channelModule.deleteContext(channel->channelContext);

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

    /* Remove the buffered chunks */
    struct MessageEntry *me, *temp_me;
    LIST_FOREACH_SAFE(me, &channel->chunks, pointers, temp_me) {
        struct ChunkPayload *cp, *temp_cp;
        SIMPLEQ_FOREACH_SAFE(cp, &me->chunkPayload, pointers, temp_cp) {
            UA_ByteString_deleteMembers(&cp->bytes);
            UA_free(cp);
        }
        LIST_REMOVE(me, pointers);
        UA_free(me);
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
    const UA_SecurityPolicyChannelModule *channelModule = &securityPolicy->channelModule;
    const UA_SecurityPolicySymmetricModule *symmetricModule = &securityPolicy->symmetricModule;
    const UA_SecurityPolicyCryptoModule *const cryptoModule = &securityPolicy->symmetricModule.cryptoModule;
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
    retval |= channelModule->setLocalSymEncryptingKey(channel->channelContext, &localEncryptingKey);
    retval |= channelModule->setLocalSymIv(channel->channelContext, &localIv);
    return retval;
}

static UA_StatusCode
UA_SecureChannel_generateRemoteKeys(const UA_SecureChannel *const channel,
                                    const UA_SecurityPolicy *const securityPolicy) {
    const UA_SecurityPolicyChannelModule *channelModule = &securityPolicy->channelModule;
    const UA_SecurityPolicySymmetricModule *symmetricModule = &securityPolicy->symmetricModule;
    const UA_SecurityPolicyCryptoModule *const cryptoModule = &securityPolicy->symmetricModule.cryptoModule;
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
    retval |= channelModule->setRemoteSymEncryptingKey(channel->channelContext, &remoteEncryptingKey);
    retval |= channelModule->setRemoteSymIv(channel->channelContext, &remoteIv);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    return retval;
}

UA_StatusCode
UA_SecureChannel_generateNewKeys(UA_SecureChannel *channel) {
    UA_StatusCode retval = UA_SecureChannel_generateLocalKeys(channel, channel->securityPolicy);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = UA_SecureChannel_generateRemoteKeys(channel, channel->securityPolicy);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    return retval;
}

UA_SessionHeader *
UA_SecureChannel_getSession(UA_SecureChannel *channel,
                            const UA_NodeId *authenticationToken) {
    struct UA_SessionHeader *sh;
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

    //FIXME: not thread-safe
    memcpy(&channel->securityToken, &channel->nextSecurityToken,
           sizeof(UA_ChannelSecurityToken));
    UA_ChannelSecurityToken_init(&channel->nextSecurityToken);
    return UA_SecureChannel_generateNewKeys(channel);
}
/***************************/
/* Send Asymmetric Message */
/***************************/

static UA_UInt16
calculatePaddingAsym(const UA_SecurityPolicy *securityPolicy, const void *channelContext,
                     size_t bytesToWrite, UA_Byte *paddingSize, UA_Byte *extraPaddingSize) {
    size_t plainTextBlockSize = securityPolicy->asymmetricModule.cryptoModule.encryptionAlgorithm.
        getRemotePlainTextBlockSize(securityPolicy, channelContext);
    size_t signatureSize = securityPolicy->asymmetricModule.cryptoModule.signatureAlgorithm.
        getLocalSignatureSize(securityPolicy, channelContext);
    size_t paddingBytes = 1;
    if(securityPolicy->asymmetricModule.cryptoModule.encryptionAlgorithm.
        getRemoteKeyLength(securityPolicy, channelContext) > 2048)
        ++paddingBytes;
    size_t padding = (plainTextBlockSize - ((bytesToWrite + signatureSize + paddingBytes) %
                                            plainTextBlockSize));
    *paddingSize = (UA_Byte)(padding & 0xff);
    *extraPaddingSize = (UA_Byte)(padding >> 8);
    return (UA_UInt16)padding;
}

static size_t
calculateAsymAlgSecurityHeaderLength(const UA_SecureChannel *channel) {
    size_t asymHeaderLength = UA_ASYMMETRIC_ALG_SECURITY_HEADER_FIXED_LENGTH +
                              channel->securityPolicy->policyUri.length;
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        /* OPN is always encrypted even if mode sign only */
        asymHeaderLength += 20; /* Thumbprints are always 20 byte long */
        asymHeaderLength += channel->securityPolicy->localCertificate.length;
    }
    return asymHeaderLength;
}

static void
hideBytesAsym(const UA_SecureChannel *channel, UA_Byte **buf_start, const UA_Byte **buf_end) {
    const UA_SecurityPolicy *securityPolicy = channel->securityPolicy;
    *buf_start += UA_SECURE_CONVERSATION_MESSAGE_HEADER_LENGTH + UA_SEQUENCE_HEADER_LENGTH;

    /* Add the SecurityHeaderLength */
    *buf_start += calculateAsymAlgSecurityHeaderLength(channel);
    size_t potentialEncryptionMaxSize = (size_t)(*buf_end - *buf_start) + UA_SEQUENCE_HEADER_LENGTH;

    /* Hide bytes for signature and padding */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        *buf_end -= securityPolicy->asymmetricModule.cryptoModule.signatureAlgorithm.
            getLocalSignatureSize(securityPolicy, channel->channelContext);
        *buf_end -= 2; /* padding byte and extraPadding byte */

        /* Add some overhead length due to RSA implementations adding a signature themselves */
        *buf_end -= UA_SecurityPolicy_getRemoteAsymEncryptionBufferLengthOverhead(securityPolicy,
                                                                                  channel->channelContext,
                                                                                  potentialEncryptionMaxSize);
    }
}

/* Sends an OPN message using asymmetric encryption if defined */
UA_StatusCode
UA_SecureChannel_sendAsymmetricOPNMessage(UA_SecureChannel *channel, UA_UInt32 requestId,
                                          const void *content, const UA_DataType *contentType) {
    if(channel->securityMode == UA_MESSAGESECURITYMODE_INVALID)
        return UA_STATUSCODE_BADSECURITYMODEREJECTED;

    const UA_SecurityPolicy *const securityPolicy = channel->securityPolicy;
    UA_Connection *connection = channel->connection;
    if(!connection)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Allocate the message buffer */
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode retval =
        connection->getSendBuffer(connection, connection->localConf.sendBufferSize, &buf);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Restrict buffer to the available space for the payload */
    UA_Byte *buf_pos = buf.data;
    const UA_Byte *buf_end = &buf.data[buf.length];
    hideBytesAsym(channel, &buf_pos, &buf_end);

    /* Encode the message type and content */
    UA_NodeId typeId = UA_NODEID_NUMERIC(0, contentType->binaryEncodingId);
    retval = UA_encodeBinary(&typeId, &UA_TYPES[UA_TYPES_NODEID], &buf_pos, &buf_end, NULL, NULL);
    retval |= UA_encodeBinary(content, contentType, &buf_pos, &buf_end, NULL, NULL);
    if(retval != UA_STATUSCODE_GOOD) {
        connection->releaseSendBuffer(connection, &buf);
        return retval;
    }

    /* Compute the length of the asym header */
    const size_t securityHeaderLength = calculateAsymAlgSecurityHeaderLength(channel);

    /* Pad the message. Also if securitymode is only sign, since we are using
     * asymmetric communication to exchange keys and thus need to encrypt. */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        const UA_Byte *buf_body_start =
            &buf.data[UA_SECURE_CONVERSATION_MESSAGE_HEADER_LENGTH +
                      UA_SEQUENCE_HEADER_LENGTH + securityHeaderLength];
        const size_t bytesToWrite =
            (uintptr_t)buf_pos - (uintptr_t)buf_body_start + UA_SEQUENCE_HEADER_LENGTH;
        UA_Byte paddingSize = 0;
        UA_Byte extraPaddingSize = 0;
        UA_UInt16 totalPaddingSize =
            calculatePaddingAsym(securityPolicy, channel->channelContext,
                                 bytesToWrite, &paddingSize, &extraPaddingSize);

        // This is <= because the paddingSize byte also has to be written.
        for(UA_UInt16 i = 0; i <= totalPaddingSize; ++i) {
            *buf_pos = paddingSize;
            ++buf_pos;
        }
        if(securityPolicy->asymmetricModule.cryptoModule.encryptionAlgorithm.
            getRemoteKeyLength(securityPolicy, channel->channelContext) > 2048) {
            *buf_pos = extraPaddingSize;
            ++buf_pos;
        }
    }

    /* The total message length */
    size_t pre_sig_length = (uintptr_t)buf_pos - (uintptr_t)buf.data;
    size_t total_length = pre_sig_length;
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        total_length += securityPolicy->asymmetricModule.cryptoModule.signatureAlgorithm.
            getLocalSignatureSize(securityPolicy, channel->channelContext);

    /* Encode the headers at the beginning of the message */
    UA_Byte *header_pos = buf.data;
    size_t dataToEncryptLength =
        total_length - (UA_SECURE_CONVERSATION_MESSAGE_HEADER_LENGTH + securityHeaderLength);
    UA_SecureConversationMessageHeader respHeader;
    respHeader.messageHeader.messageTypeAndChunkType = UA_MESSAGETYPE_OPN + UA_CHUNKTYPE_FINAL;
    respHeader.messageHeader.messageSize = (UA_UInt32)
        (total_length + UA_SecurityPolicy_getRemoteAsymEncryptionBufferLengthOverhead(securityPolicy,
                                                                                      channel->channelContext,
                                                                                      dataToEncryptLength));
    respHeader.secureChannelId = channel->securityToken.channelId;
    retval = UA_encodeBinary(&respHeader, &UA_TRANSPORT[UA_TRANSPORT_SECURECONVERSATIONMESSAGEHEADER],
                             &header_pos, &buf_end, NULL, NULL);

    UA_AsymmetricAlgorithmSecurityHeader asymHeader;
    UA_AsymmetricAlgorithmSecurityHeader_init(&asymHeader);
    asymHeader.securityPolicyUri = channel->securityPolicy->policyUri;
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        asymHeader.senderCertificate = channel->securityPolicy->localCertificate;
        asymHeader.receiverCertificateThumbprint.length = 20;
        asymHeader.receiverCertificateThumbprint.data = channel->remoteCertificateThumbprint;
    }
    retval |= UA_encodeBinary(&asymHeader, &UA_TRANSPORT[UA_TRANSPORT_ASYMMETRICALGORITHMSECURITYHEADER],
                              &header_pos, &buf_end, NULL, NULL);

    UA_SequenceHeader seqHeader;
    seqHeader.requestId = requestId;
    seqHeader.sequenceNumber = UA_atomic_addUInt32(&channel->sendSequenceNumber, 1);
    retval |= UA_encodeBinary(&seqHeader, &UA_TRANSPORT[UA_TRANSPORT_SEQUENCEHEADER],
                              &header_pos, &buf_end, NULL, NULL);

    /* Did encoding the header succeed? */
    if(retval != UA_STATUSCODE_GOOD) {
        connection->releaseSendBuffer(connection, &buf);
        return retval;
    }

    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        /* Sign message */
        const UA_ByteString dataToSign = {pre_sig_length, buf.data};
        size_t sigsize = securityPolicy->asymmetricModule.cryptoModule.signatureAlgorithm.
            getLocalSignatureSize(securityPolicy, channel->channelContext);
        UA_ByteString signature = {sigsize, buf.data + pre_sig_length};
        retval = securityPolicy->asymmetricModule.cryptoModule.signatureAlgorithm.
            sign(securityPolicy, channel->channelContext, &dataToSign, &signature);
        if(retval != UA_STATUSCODE_GOOD) {
            connection->releaseSendBuffer(connection, &buf);
            return retval;
        }

        /* Specification part 6, 6.7.4: The OpenSecureChannel Messages are
         * signed and encrypted if the SecurityMode is not None (even if the
         * SecurityMode is SignOnly). */
        size_t unencrypted_length =
            UA_SECURE_CONVERSATION_MESSAGE_HEADER_LENGTH + securityHeaderLength;
        UA_ByteString dataToEncrypt = {total_length - unencrypted_length,
                                       &buf.data[unencrypted_length]};
        retval = securityPolicy->asymmetricModule.cryptoModule.encryptionAlgorithm.
            encrypt(securityPolicy, channel->channelContext, &dataToEncrypt);
        if(retval != UA_STATUSCODE_GOOD) {
            connection->releaseSendBuffer(connection, &buf);
            return retval;
        }
    }

    /* Send the message, the buffer is freed in the network layer */
    buf.length = respHeader.messageHeader.messageSize;
    retval = connection->send(connection, &buf);
#ifdef UA_ENABLE_UNIT_TEST_FAILURE_HOOKS
    retval |= sendAsym_sendFailure
#endif
    return retval;
}

/**************************/
/* Send Symmetric Message */
/**************************/

static UA_UInt16
calculatePaddingSym(const UA_SecurityPolicy *securityPolicy, const void *channelContext,
                    size_t bytesToWrite, UA_Byte *paddingSize, UA_Byte *extraPaddingSize) {

    size_t encryptionBlockSize = securityPolicy->symmetricModule.cryptoModule.encryptionAlgorithm.
        getLocalBlockSize(securityPolicy, channelContext);
    size_t signatureSize = securityPolicy->symmetricModule.cryptoModule.signatureAlgorithm.
        getLocalSignatureSize(securityPolicy, channelContext);

    UA_UInt16 padding = (UA_UInt16)(encryptionBlockSize - ((bytesToWrite + signatureSize + 1) % encryptionBlockSize));
    *paddingSize = (UA_Byte)padding;
    *extraPaddingSize = (UA_Byte)(padding >> 8);
    return padding;
}

static void
setBufPos(UA_MessageContext *mc) {
    const UA_SecureChannel *channel = mc->channel;
    const UA_SecurityPolicy *securityPolicy = channel->securityPolicy;

    /* Forward the data pointer so that the payload is encoded after the
     * message header */
    mc->buf_pos = &mc->messageBuffer.data[UA_SECURE_MESSAGE_HEADER_LENGTH];
    mc->buf_end = &mc->messageBuffer.data[mc->messageBuffer.length];

    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        mc->buf_end -= securityPolicy->symmetricModule.cryptoModule.signatureAlgorithm.
            getLocalSignatureSize(securityPolicy, channel->channelContext);

    /* Hide a byte needed for padding */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        mc->buf_end -= 2;
}

static UA_StatusCode
sendSymmetricChunk(UA_MessageContext *mc) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_SecureChannel *const channel = mc->channel;
    const UA_SecurityPolicy *securityPolicy = channel->securityPolicy;
    UA_Connection *const connection = channel->connection;
    if(!connection)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Will this chunk surpass the capacity of the SecureChannel for the message? */
    UA_Byte *buf_body_start = mc->messageBuffer.data + UA_SECURE_MESSAGE_HEADER_LENGTH;
    const UA_Byte *buf_body_end = mc->buf_pos;
    size_t bodyLength = (uintptr_t)buf_body_end - (uintptr_t)buf_body_start;
    mc->messageSizeSoFar += bodyLength;
    mc->chunksSoFar++;
    if(mc->messageSizeSoFar > connection->remoteConf.maxMessageSize &&
       connection->remoteConf.maxMessageSize != 0)
        res = UA_STATUSCODE_BADRESPONSETOOLARGE;
    if(mc->chunksSoFar > connection->remoteConf.maxChunkCount &&
       connection->remoteConf.maxChunkCount != 0)
        res = UA_STATUSCODE_BADRESPONSETOOLARGE;
    if(res != UA_STATUSCODE_GOOD) {
        connection->releaseSendBuffer(channel->connection, &mc->messageBuffer);
        return res;
    }

    /* Pad the message. The bytes for the padding and signature were removed
     * from buf_end before encoding the payload. So we don't check here. */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        size_t bytesToWrite = bodyLength + UA_SEQUENCE_HEADER_LENGTH;
        UA_Byte paddingSize = 0;
        UA_Byte extraPaddingSize = 0;
        UA_UInt16 totalPaddingSize =
            calculatePaddingSym(securityPolicy, channel->channelContext,
                                bytesToWrite, &paddingSize, &extraPaddingSize);

        // This is <= because the paddingSize byte also has to be written.
        for(UA_UInt16 i = 0; i <= totalPaddingSize; ++i) {
            *mc->buf_pos = paddingSize;
            ++(mc->buf_pos);
        }
        if(extraPaddingSize > 0) {
            *mc->buf_pos = extraPaddingSize;
            ++(mc->buf_pos);
        }
    }

    /* The total message length */
    size_t pre_sig_length = (uintptr_t)(mc->buf_pos) - (uintptr_t)mc->messageBuffer.data;
    size_t total_length = pre_sig_length;
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        total_length += securityPolicy->symmetricModule.cryptoModule.signatureAlgorithm.
            getLocalSignatureSize(securityPolicy, channel->channelContext);
    mc->messageBuffer.length = total_length; /* For giving the buffer to the network layer */

    /* Encode the chunk headers at the beginning of the buffer */
    UA_assert(res == UA_STATUSCODE_GOOD);
    UA_Byte *header_pos = mc->messageBuffer.data;
    UA_SecureConversationMessageHeader respHeader;
    respHeader.secureChannelId = channel->securityToken.channelId;
    respHeader.messageHeader.messageTypeAndChunkType = mc->messageType;
    respHeader.messageHeader.messageSize = (UA_UInt32)total_length;
    if(mc->final)
        respHeader.messageHeader.messageTypeAndChunkType += UA_CHUNKTYPE_FINAL;
    else
        respHeader.messageHeader.messageTypeAndChunkType += UA_CHUNKTYPE_INTERMEDIATE;
    res = UA_encodeBinary(&respHeader, &UA_TRANSPORT[UA_TRANSPORT_SECURECONVERSATIONMESSAGEHEADER],
                          &header_pos, &mc->buf_end, NULL, NULL);

    UA_SymmetricAlgorithmSecurityHeader symSecHeader;
    symSecHeader.tokenId = channel->securityToken.tokenId;
    res |= UA_encodeBinary(&symSecHeader.tokenId,
                           &UA_TRANSPORT[UA_TRANSPORT_SYMMETRICALGORITHMSECURITYHEADER],
                           &header_pos, &mc->buf_end, NULL, NULL);

    UA_SequenceHeader seqHeader;
    seqHeader.requestId = mc->requestId;
    seqHeader.sequenceNumber = UA_atomic_addUInt32(&channel->sendSequenceNumber, 1);
    res |= UA_encodeBinary(&seqHeader, &UA_TRANSPORT[UA_TRANSPORT_SEQUENCEHEADER],
                           &header_pos, &mc->buf_end, NULL, NULL);

    /* Sign message */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        UA_ByteString dataToSign = mc->messageBuffer;
        dataToSign.length = pre_sig_length;
        UA_ByteString signature;
        signature.length = securityPolicy->symmetricModule.cryptoModule.signatureAlgorithm.
            getLocalSignatureSize(securityPolicy, channel->channelContext);
        signature.data = mc->buf_pos;
        res |= securityPolicy->symmetricModule.cryptoModule.signatureAlgorithm.
            sign(securityPolicy, channel->channelContext, &dataToSign, &signature);
    }

    /* Encrypt message */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        UA_ByteString dataToEncrypt;
        dataToEncrypt.data = mc->messageBuffer.data + UA_SECUREMH_AND_SYMALGH_LENGTH;
        dataToEncrypt.length = total_length - UA_SECUREMH_AND_SYMALGH_LENGTH;
        res |= securityPolicy->symmetricModule.cryptoModule.encryptionAlgorithm.
            encrypt(securityPolicy, channel->channelContext, &dataToEncrypt);
    }

    if(res != UA_STATUSCODE_GOOD) {
        connection->releaseSendBuffer(channel->connection, &mc->messageBuffer);
        return res;
    }

    /* Send the chunk, the buffer is freed in the network layer */
    return connection->send(channel->connection, &mc->messageBuffer);
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
    retval = connection->getSendBuffer(connection, connection->localConf.sendBufferSize,
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

    /* Minimum required size */
    if(connection->localConf.sendBufferSize <= UA_SECURE_MESSAGE_HEADER_LENGTH)
        return UA_STATUSCODE_BADRESPONSETOOLARGE;

    /* Allocate the message buffer */
    UA_StatusCode retval =
        connection->getSendBuffer(connection, connection->localConf.sendBufferSize,
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
    if(!channel || !payload || !payloadType)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(channel->connection) {
        if(channel->connection->state == UA_CONNECTION_CLOSED)
            return UA_STATUSCODE_BADCONNECTIONCLOSED;
    }

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

static void
UA_SecureChannel_removeChunks(UA_SecureChannel *channel, UA_UInt32 requestId) {
    struct MessageEntry *me;
    LIST_FOREACH(me, &channel->chunks, pointers) {
        if(me->requestId == requestId) {
            struct ChunkPayload *cp, *temp_cp;
            SIMPLEQ_FOREACH_SAFE(cp, &me->chunkPayload, pointers, temp_cp) {
                UA_ByteString_deleteMembers(&cp->bytes);
                UA_free(cp);
            }
            LIST_REMOVE(me, pointers);
            UA_free(me);
            return;
        }
    }
}

static UA_StatusCode
appendChunk(struct MessageEntry *messageEntry, const UA_ByteString *chunkBody) {
    
    struct ChunkPayload* cp = (struct ChunkPayload*)UA_malloc(sizeof(struct ChunkPayload));
    UA_StatusCode retval = UA_ByteString_copy(chunkBody, &cp->bytes);
    if (retval != UA_STATUSCODE_GOOD)
        return retval;
    
    SIMPLEQ_INSERT_TAIL(&messageEntry->chunkPayload, cp, pointers);
    messageEntry->chunkPayloadSize += chunkBody->length;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_SecureChannel_appendChunk(UA_SecureChannel *channel, UA_UInt32 requestId,
                             const UA_ByteString *chunkBody) {
    struct MessageEntry *me;
    LIST_FOREACH(me, &channel->chunks, pointers) {
        if(me->requestId == requestId)
            break;
    }

    /* No chunkentry on the channel, create one */
    if(!me) {
        me = (struct MessageEntry *)UA_malloc(sizeof(struct MessageEntry));
        if(!me)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        memset(me, 0, sizeof(struct MessageEntry));
        me->requestId = requestId;
        SIMPLEQ_INIT(&me->chunkPayload);
        LIST_INSERT_HEAD(&channel->chunks, me, pointers);
    }

    return appendChunk(me, chunkBody);
}

static UA_StatusCode
UA_SecureChannel_finalizeChunk(UA_SecureChannel *channel, UA_UInt32 requestId,
                               const UA_ByteString *chunkBody, UA_MessageType messageType,
                               UA_ProcessMessageCallback callback, void *application) {
    struct MessageEntry *messageEntry;
    LIST_FOREACH(messageEntry, &channel->chunks, pointers) {
        if(messageEntry->requestId == requestId)
            break;
    }

    UA_ByteString bytes;
    if(!messageEntry) {
        bytes = *chunkBody;
    } else {
        UA_StatusCode retval = appendChunk(messageEntry, chunkBody);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        
        UA_ByteString_init(&bytes);

        bytes.data = (UA_Byte*) UA_malloc(messageEntry->chunkPayloadSize);
        if (!bytes.data)
            return UA_STATUSCODE_BADOUTOFMEMORY;

        struct ChunkPayload *cp, *temp_cp;
        size_t curPos = 0;
        SIMPLEQ_FOREACH_SAFE(cp, &messageEntry->chunkPayload, pointers, temp_cp) {
            memcpy(&bytes.data[curPos], cp->bytes.data, cp->bytes.length);
            curPos += cp->bytes.length;
            UA_ByteString_deleteMembers(&cp->bytes);
            UA_free(cp);
        }

        bytes.length = messageEntry->chunkPayloadSize;

        LIST_REMOVE(messageEntry, pointers);
        UA_free(messageEntry);
    }

    UA_StatusCode retval = callback(application, channel, messageType, requestId, &bytes);
    if(messageEntry)
        UA_ByteString_deleteMembers(&bytes);
    return retval;
}

/****************************/
/* Process a received Chunk */
/****************************/

static UA_StatusCode
decryptChunk(UA_SecureChannel *channel, const UA_SecurityPolicyCryptoModule *cryptoModule,
             UA_ByteString *chunk, size_t offset, UA_UInt32 *requestId, UA_UInt32 *sequenceNumber,
             UA_ByteString *payload, UA_MessageType messageType) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    const UA_SecurityPolicy *securityPolicy = channel->securityPolicy;
    size_t chunkSizeAfterDecryption = chunk->length;

    /* Decrypt the chunk. Always decrypt opn messages if mode not none */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT ||
       messageType == UA_MESSAGETYPE_OPN) {
        UA_ByteString cipherText = {chunk->length - offset, chunk->data + offset};
        size_t sizeBeforeDecryption = cipherText.length;
        retval = cryptoModule->encryptionAlgorithm.decrypt(securityPolicy, channel->channelContext, &cipherText);
        chunkSizeAfterDecryption -= (sizeBeforeDecryption - cipherText.length);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    /* Verify the chunk signature */
    size_t sigsize = 0;
    size_t paddingSize = 0;
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT ||
       messageType == UA_MESSAGETYPE_OPN) {
        /* Compute the padding size */
        sigsize = cryptoModule->signatureAlgorithm.getRemoteSignatureSize(securityPolicy, channel->channelContext);

        if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT ||
           (messageType == UA_MESSAGETYPE_OPN &&
            channel->securityMode > UA_MESSAGESECURITYMODE_NONE)) {
            paddingSize = (size_t)chunk->data[chunkSizeAfterDecryption - sigsize - 1];

            size_t keyLength =
                cryptoModule->encryptionAlgorithm.getRemoteKeyLength(securityPolicy, channel->channelContext);
            if(keyLength > 2048) {
                paddingSize <<= 8; /* Extra padding size */
                paddingSize += chunk->data[chunkSizeAfterDecryption - sigsize - 2];
                // see comment below but for extraPaddingSize
                paddingSize += 1;
            }

            // we need to add one to the padding size since the paddingSize byte itself need to be removed as well.
            // TODO: write unit test for correct padding calculation
            paddingSize += 1;
        }
        if(offset + paddingSize + sigsize >= chunkSizeAfterDecryption)
            return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

        /* Verify the signature */
        const UA_ByteString chunkDataToVerify = {chunkSizeAfterDecryption - sigsize, chunk->data};
        const UA_ByteString signature = {sigsize, chunk->data + chunkSizeAfterDecryption - sigsize};
        retval = cryptoModule->signatureAlgorithm.verify(securityPolicy, channel->channelContext,
                                                         &chunkDataToVerify, &signature);
#ifdef UA_ENABLE_UNIT_TEST_FAILURE_HOOKS
        retval |= decrypt_verifySignatureFailure;
#endif
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
    return UA_STATUSCODE_GOOD;
}

typedef UA_StatusCode(*UA_SequenceNumberCallback)(UA_SecureChannel *channel,
                                                  UA_UInt32 sequenceNumber);

static UA_StatusCode
processSequenceNumberAsym(UA_SecureChannel *const channel, UA_UInt32 sequenceNumber) {
    channel->receiveSequenceNumber = sequenceNumber;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
processSequenceNumberSym(UA_SecureChannel *const channel, UA_UInt32 sequenceNumber) {
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

static UA_StatusCode
checkAsymHeader(UA_SecureChannel *const channel,
                UA_AsymmetricAlgorithmSecurityHeader *const asymHeader) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    const UA_SecurityPolicy *const securityPolicy = channel->securityPolicy;

    if(!UA_ByteString_equal(&securityPolicy->policyUri, &asymHeader->securityPolicyUri)) {
        return UA_STATUSCODE_BADSECURITYPOLICYREJECTED;
    }

    // TODO: Verify certificate using certificate plugin. This will come with a new PR
    /* Something like this
    retval = certificateManager->verify(certificateStore??, &asymHeader->senderCertificate);
    if(retval != UA_STATUSCODE_GOOD)
    return retval;
    */
    retval = securityPolicy->asymmetricModule.
        compareCertificateThumbprint(securityPolicy, &asymHeader->receiverCertificateThumbprint);
    if(retval != UA_STATUSCODE_GOOD) {
        return retval;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
checkSymHeader(UA_SecureChannel *const channel,
               const UA_UInt32 tokenId) {
    if(tokenId != channel->securityToken.tokenId) {
        if(tokenId != channel->nextSecurityToken.tokenId)
            return UA_STATUSCODE_BADSECURECHANNELTOKENUNKNOWN;
        return UA_SecureChannel_revolveTokens(channel);
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_SecureChannel_processChunk(UA_SecureChannel *channel, UA_ByteString *chunk,
                              UA_ProcessMessageCallback callback,
                              void *application) {
    /* Decode message header */
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

    /* ERR message (not encrypted) */
    UA_UInt32 requestId = 0;
    UA_UInt32 sequenceNumber = 0;
    UA_ByteString chunkPayload;
    const UA_SecurityPolicyCryptoModule *cryptoModule = NULL;
    UA_SequenceNumberCallback sequenceNumberCallback = NULL;

    switch(messageType) {
    case UA_MESSAGETYPE_ERR: {
        if(chunkType != UA_CHUNKTYPE_FINAL)
            return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
        chunkPayload.length = chunk->length - offset;
        chunkPayload.data = chunk->data + offset;
        return callback(application, channel, messageType, requestId, &chunkPayload);
    }

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

        retval = checkSymHeader(channel, symmetricSecurityHeader.tokenId);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;

        cryptoModule = &channel->securityPolicy->symmetricModule.cryptoModule;
        sequenceNumberCallback = processSequenceNumberSym;
        break;
    }
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
    default:return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
    }

    /* Decrypt message */
    UA_assert(cryptoModule != NULL);
    retval = decryptChunk(channel, cryptoModule, chunk, offset, &requestId,
                          &sequenceNumber, &chunkPayload, messageType);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Check the sequence number */
    if(sequenceNumberCallback == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    retval = sequenceNumberCallback(channel, sequenceNumber);

    /* Skip sequence number checking for fuzzer to improve coverage */
    if(retval != UA_STATUSCODE_GOOD) {
#if !defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
        return retval;
#else
        retval = UA_STATUSCODE_GOOD;
#endif
    }

    /* Process the payload */
    if(chunkType == UA_CHUNKTYPE_FINAL) {
        retval = UA_SecureChannel_finalizeChunk(channel, requestId, &chunkPayload,
                                                messageType, callback, application);
    } else if(chunkType == UA_CHUNKTYPE_INTERMEDIATE) {
        retval = UA_SecureChannel_appendChunk(channel, requestId, &chunkPayload);
    } else if(chunkType == UA_CHUNKTYPE_ABORT) {
        UA_SecureChannel_removeChunks(channel, requestId);
    } else {
        retval = UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
    }
    return retval;
}

/* Functionality used by both the SecureChannel and the SecurityPolicy */

size_t
UA_SecurityPolicy_getRemoteAsymEncryptionBufferLengthOverhead(const UA_SecurityPolicy *securityPolicy,
                                                              const void *channelContext,
                                                              size_t maxEncryptionLength) {
    if(maxEncryptionLength == 0)
        return 0;

    size_t plainTextBlockSize = securityPolicy->asymmetricModule.cryptoModule.encryptionAlgorithm.
        getRemotePlainTextBlockSize(securityPolicy, channelContext);
    size_t encryptedBlockSize = securityPolicy->asymmetricModule.cryptoModule.encryptionAlgorithm.
        getRemoteBlockSize(securityPolicy, channelContext);
    if(plainTextBlockSize == 0)
        return 0;

    size_t maxNumberOfBlocks = maxEncryptionLength / plainTextBlockSize;
    return maxNumberOfBlocks * (encryptedBlockSize - plainTextBlockSize);
}
