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
#include "ua_plugin_securitypolicy.h"

#define UA_BITMASK_MESSAGETYPE 0x00ffffff
#define UA_BITMASK_CHUNKTYPE 0xff000000
#define UA_SECURE_MESSAGE_HEADER_LENGTH 24
#define UA_ASYMMETRIC_ALG_SECURITY_HEADER_FIXED_LENGTH 12
#define UA_SYMMETRIC_ALG_SECURITY_HEADER_LENGTH 4
#define UA_SEQUENCE_HEADER_LENGTH 8
#define UA_SECUREMH_AND_SYMALGH_LENGTH              \
    (UA_SECURE_CONVERSATION_MESSAGE_HEADER_LENGTH + \
    UA_SYMMETRIC_ALG_SECURITY_HEADER_LENGTH)

const UA_ByteString
UA_SECURITY_POLICY_NONE_URI = { 47, (UA_Byte*)"http://opcfoundation.org/UA/SecurityPolicy#None" };

UA_StatusCode
UA_SecureChannel_init(UA_SecureChannel *channel,
                      const UA_SecurityPolicy *securityPolicy,
                      const UA_ByteString *remoteCertificate) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    memset(channel, 0, sizeof(UA_SecureChannel));
    channel->state = UA_SECURECHANNELSTATE_FRESH;
    channel->securityPolicy = securityPolicy;

    retval = securityPolicy->channelModule.newContext(securityPolicy, remoteCertificate,
                                                      &channel->channelContext);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = UA_ByteString_copy(remoteCertificate, &channel->remoteCertificate);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_ByteString remoteCertificateThumbprint = { 20, channel->remoteCertificateThumbprint };
    retval = securityPolicy->asymmetricModule.
        makeCertificateThumbprint(securityPolicy, &channel->remoteCertificate,
                                  &remoteCertificateThumbprint);

    return retval;
    /* Linked lists are also initialized by zeroing out */
    /* LIST_INIT(&channel->sessions); */
    /* LIST_INIT(&channel->chunks); */
}

void UA_SecureChannel_deleteMembersCleanup(UA_SecureChannel* channel) {
    /* Delete members */
    UA_ByteString_deleteMembers(&channel->remoteCertificate);
    UA_ByteString_deleteMembers(&channel->localNonce);
    UA_ByteString_deleteMembers(&channel->remoteNonce);
    UA_ChannelSecurityToken_deleteMembers(&channel->securityToken);
    UA_ChannelSecurityToken_deleteMembers(&channel->nextSecurityToken);

    /* Delete the channel context for the security policy */
    if(channel->securityPolicy)
        channel->securityPolicy->channelModule.deleteContext(channel->channelContext);

    /* Detach from the connection */
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

UA_StatusCode UA_SecureChannel_generateNonce(const UA_SecureChannel *const channel,
                                             const size_t nonceLength,
                                             UA_ByteString *const nonce) {
    UA_ByteString_allocBuffer(nonce, nonceLength);
    if(!nonce->data)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    return channel->securityPolicy->symmetricModule.generateNonce(channel->securityPolicy,
                                                                  nonce);
}

UA_StatusCode
UA_SecureChannel_generateNewKeys(UA_SecureChannel *const channel) {
    const UA_SecurityPolicy *const securityPolicy = channel->securityPolicy;
    const UA_SecurityPolicyChannelModule *channelModule =
        &securityPolicy->channelModule;
    const UA_SecurityPolicySymmetricModule *symmetricModule =
        &securityPolicy->symmetricModule;

    /* Symmetric key length */
    size_t encryptionKeyLength = symmetricModule->cryptoModule.
        getLocalEncryptionKeyLength(securityPolicy, channel->channelContext);
    const size_t buffSize = symmetricModule->encryptionBlockSize +
        symmetricModule->signingKeyLength + encryptionKeyLength;
    UA_ByteString buffer = { buffSize, (UA_Byte*)UA_alloca(buffSize) };

    /* Remote keys */
    UA_StatusCode retval = symmetricModule->generateKey(securityPolicy, &channel->localNonce,
                                                        &channel->remoteNonce, &buffer);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    const UA_ByteString remoteSigningKey = { symmetricModule->signingKeyLength, buffer.data };
    const UA_ByteString remoteEncryptingKey =
    { encryptionKeyLength, buffer.data + symmetricModule->signingKeyLength };
    const UA_ByteString remoteIv = { symmetricModule->encryptionBlockSize,
                                    buffer.data + symmetricModule->signingKeyLength +
                                    encryptionKeyLength };
    retval  = channelModule->setRemoteSymSigningKey(channel->channelContext, &remoteSigningKey);
    retval |= channelModule->setRemoteSymEncryptingKey(channel->channelContext, &remoteEncryptingKey);
    retval |= channelModule->setRemoteSymIv(channel->channelContext, &remoteIv);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Local keys */
    retval = symmetricModule->generateKey(securityPolicy, &channel->remoteNonce,
                                          &channel->localNonce, &buffer);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    const UA_ByteString localSigningKey = { symmetricModule->signingKeyLength, buffer.data };
    const UA_ByteString localEncryptingKey =
    { encryptionKeyLength, buffer.data + symmetricModule->signingKeyLength };
    const UA_ByteString localIv = { symmetricModule->encryptionBlockSize,
                                    buffer.data + symmetricModule->signingKeyLength +
                                    encryptionKeyLength };
    retval  = channelModule->setLocalSymSigningKey(channel->channelContext, &localSigningKey);
    retval |= channelModule->setLocalSymEncryptingKey(channel->channelContext, &localEncryptingKey);
    retval |= channelModule->setLocalSymIv(channel->channelContext, &localIv);
    return retval;
}

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

UA_Session *
UA_SecureChannel_getSession(UA_SecureChannel* channel, UA_NodeId* token) {
    struct SessionEntry* se;
    LIST_FOREACH(se, &channel->sessions, pointers) {
        if(UA_NodeId_equal(&se->session->authenticationToken, token))
            break;
    }
    if(!se)
        return NULL;
    return se->session;
}

UA_StatusCode UA_SecureChannel_revolveTokens(UA_SecureChannel* channel) {
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
    size_t plainTextBlockSize = securityPolicy->channelModule.
        getRemoteAsymPlainTextBlockSize(channelContext);
    size_t signatureSize = securityPolicy->asymmetricModule.cryptoModule.
        getLocalSignatureSize(securityPolicy, channelContext);
    size_t padding = (plainTextBlockSize - ((bytesToWrite + signatureSize + 1) % plainTextBlockSize));
    *paddingSize = (UA_Byte)padding;
    *extraPaddingSize = (UA_Byte)(padding >> 8);
    return (UA_UInt16)padding;
}

static size_t
calculateAsymAlgSecurityHeaderLength(const UA_SecureChannel *channel) {
    size_t asymHeaderLength = UA_ASYMMETRIC_ALG_SECURITY_HEADER_FIXED_LENGTH +
        channel->securityPolicy->policyUri.length;
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        asymHeaderLength += channel->securityPolicy->localCertificate.length;
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        asymHeaderLength += 20; /* Thumbprints are always 20 byte long */
    return asymHeaderLength;
}

static void
hideBytesAsym(UA_SecureChannel *const channel, UA_Byte **const buf_start,
              const UA_Byte **const buf_end) {
    const UA_SecurityPolicy *const securityPolicy = channel->securityPolicy;
    *buf_start += UA_SECURE_CONVERSATION_MESSAGE_HEADER_LENGTH + UA_SEQUENCE_HEADER_LENGTH;

    /* Add the SecurityHeaderLength */
    *buf_start += calculateAsymAlgSecurityHeaderLength(channel);
    size_t potentialEncryptionMaxSize = (size_t)(*buf_end - *buf_start) + UA_SEQUENCE_HEADER_LENGTH;

    /* Hide bytes for signature and padding */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        *buf_end -= securityPolicy->asymmetricModule.cryptoModule.
            getLocalSignatureSize(securityPolicy, channel->channelContext);
        *buf_end -= 2; // padding byte and extraPadding byte

        /* Add some overhead length due to RSA implementations adding a signature themselves */
        *buf_end -= securityPolicy->channelModule
            .getRemoteAsymEncryptionBufferLengthOverhead(channel->channelContext,
                                                         potentialEncryptionMaxSize);
    }
}

/* Sends an OPN message using asymmetric encryption if defined */
UA_StatusCode
UA_SecureChannel_sendAsymmetricOPNMessage(UA_SecureChannel *channel, UA_UInt32 requestId,
                                          const void *content, const UA_DataType *contentType) {
    const UA_SecurityPolicy *const securityPolicy = channel->securityPolicy;
    UA_Connection* connection = channel->connection;
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
    retval = UA_encodeBinary(&typeId, &UA_TYPES[UA_TYPES_NODEID],
                             &buf_pos, &buf_end, NULL, NULL);
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
        for(UA_UInt16 i = 0; i < totalPaddingSize; ++i) {
            *buf_pos = paddingSize;
            ++buf_pos;
        }
        if(extraPaddingSize > 0) {
            *buf_pos = extraPaddingSize;
            ++buf_pos;
        }
    }

    /* The total message length */
    size_t pre_sig_length = (uintptr_t)buf_pos - (uintptr_t)buf.data;
    size_t total_length = pre_sig_length;
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        total_length += securityPolicy->asymmetricModule.cryptoModule.
        getLocalSignatureSize(securityPolicy, channel->channelContext);

    /* Encode the headers at the beginning of the message */
    UA_Byte *header_pos = buf.data;
    size_t dataToEncryptLength = total_length -
        (UA_SECURE_CONVERSATION_MESSAGE_HEADER_LENGTH + securityHeaderLength);
    UA_SecureConversationMessageHeader respHeader;
    respHeader.messageHeader.messageTypeAndChunkType = UA_MESSAGETYPE_OPN + UA_CHUNKTYPE_FINAL;
    respHeader.messageHeader.messageSize = (UA_UInt32)
        (total_length + securityPolicy->channelModule.
         getRemoteAsymEncryptionBufferLengthOverhead(channel->channelContext, dataToEncryptLength));
    respHeader.secureChannelId = channel->securityToken.channelId;
    retval = UA_encodeBinary(&respHeader, &UA_TRANSPORT[UA_TRANSPORT_SECURECONVERSATIONMESSAGEHEADER],
                             &header_pos, &buf_end, NULL, NULL);

    UA_AsymmetricAlgorithmSecurityHeader asymHeader;
    UA_AsymmetricAlgorithmSecurityHeader_init(&asymHeader);
    asymHeader.securityPolicyUri = channel->securityPolicy->policyUri;
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        asymHeader.senderCertificate = channel->securityPolicy->localCertificate;
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        asymHeader.receiverCertificateThumbprint.length = 20;
        asymHeader.receiverCertificateThumbprint.data = channel->remoteCertificateThumbprint;
    }
    retval |= UA_encodeBinary(&asymHeader, &UA_TRANSPORT[UA_TRANSPORT_ASYMMETRICALGORITHMSECURITYHEADER],
                              &header_pos, &buf_end, NULL, NULL);

    UA_SequenceHeader seqHeader;
    seqHeader.requestId = requestId;
    seqHeader.sequenceNumber = UA_atomic_add(&channel->sendSequenceNumber, 1);
    retval |= UA_encodeBinary(&seqHeader, &UA_TRANSPORT[UA_TRANSPORT_SEQUENCEHEADER],
                              &header_pos, &buf_end, NULL, NULL);

    /* Did encoding the header succeed? */
    if(retval != UA_STATUSCODE_GOOD) {
        connection->releaseSendBuffer(connection, &buf);
        return retval;
    }

    /* Sign message */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        const UA_ByteString dataToSign = { pre_sig_length, buf.data };
        size_t sigsize = securityPolicy->asymmetricModule.cryptoModule.
            getLocalSignatureSize(securityPolicy, channel->channelContext);
        UA_ByteString signature = { sigsize, buf.data + pre_sig_length };
        retval = securityPolicy->asymmetricModule.cryptoModule.
            sign(securityPolicy, channel->channelContext, &dataToSign, &signature);
        if(retval != UA_STATUSCODE_GOOD) {
            connection->releaseSendBuffer(connection, &buf);
            return retval;
        }
    }

    /* Encrypt message if mode not none */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        size_t unencrypted_length =
            UA_SECURE_CONVERSATION_MESSAGE_HEADER_LENGTH + securityHeaderLength;
        UA_ByteString dataToEncrypt = { total_length - unencrypted_length,
                                       &buf.data[unencrypted_length] };
        retval = securityPolicy->asymmetricModule.cryptoModule.
            encrypt(securityPolicy, channel->channelContext, &dataToEncrypt);
        if(retval != UA_STATUSCODE_GOOD) {
            connection->releaseSendBuffer(connection, &buf);
            return retval;
        }
    }

    /* Send the message, the buffer is freed in the network layer */
    buf.length = respHeader.messageHeader.messageSize;
    return connection->send(connection, &buf);
}

/**************************/
/* Send Symmetric Message */
/**************************/

static UA_UInt16
calculatePaddingSym(const UA_SecurityPolicy *securityPolicy, const void *channelContext,
                    size_t bytesToWrite, UA_Byte *paddingSize, UA_Byte *extraPaddingSize) {
    UA_UInt16 padding = (UA_UInt16)(securityPolicy->symmetricModule.encryptionBlockSize -
        ((bytesToWrite + securityPolicy->symmetricModule.cryptoModule.
          getLocalSignatureSize(securityPolicy, channelContext) + 1) %
         securityPolicy->symmetricModule.encryptionBlockSize));
    *paddingSize = (UA_Byte)padding;
    *extraPaddingSize = (UA_Byte)(padding >> 8);
    return padding;
}

/* Sends a message using symmetric encryption if defined
 *
 * @param ci the chunk information that is used to send the chunk.
 * @param buf_pos the position in the send buffer after the body was encoded.
 *                Should be less than or equal to buf_end.
 * @param buf_end the maximum position of the body. */
static UA_StatusCode
sendChunkSymmetric(UA_ChunkInfo* ci, UA_Byte **buf_pos, const UA_Byte **buf_end) {
    UA_SecureChannel* const channel = ci->channel;
    const UA_SecurityPolicy *securityPolicy = channel->securityPolicy;
    UA_Connection* const connection = channel->connection;
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
    if(ci->errorCode != UA_STATUSCODE_GOOD) {
        connection->releaseSendBuffer(channel->connection, &ci->messageBuffer);
        return ci->errorCode;
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
        for(UA_UInt16 i = 0; i < totalPaddingSize; ++i) {
            **buf_pos = paddingSize;
            ++(*buf_pos);
        }
        if(extraPaddingSize > 0) {
            **buf_pos = extraPaddingSize;
            ++(*buf_pos);
        }
    }

    /* The total message length */
    size_t pre_sig_length = (uintptr_t)(*buf_pos) - (uintptr_t)ci->messageBuffer.data;
    size_t total_length = pre_sig_length;
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        total_length += securityPolicy->symmetricModule.cryptoModule.
        getLocalSignatureSize(securityPolicy, channel->channelContext);

    /* Encode the chunk headers at the beginning of the buffer */
    UA_Byte *header_pos = ci->messageBuffer.data;
    UA_SecureConversationMessageHeader respHeader;
    respHeader.secureChannelId = channel->securityToken.channelId;
    respHeader.messageHeader.messageTypeAndChunkType = ci->messageType;
    respHeader.messageHeader.messageSize = (UA_UInt32)total_length;
    if(ci->errorCode == UA_STATUSCODE_GOOD) {
        if(ci->final)
            respHeader.messageHeader.messageTypeAndChunkType += UA_CHUNKTYPE_FINAL;
        else
            respHeader.messageHeader.messageTypeAndChunkType += UA_CHUNKTYPE_INTERMEDIATE;
    } else {
        respHeader.messageHeader.messageTypeAndChunkType += UA_CHUNKTYPE_ABORT;
    }
    ci->errorCode |= UA_encodeBinary(&respHeader,
                                     &UA_TRANSPORT[UA_TRANSPORT_SECURECONVERSATIONMESSAGEHEADER],
                                     &header_pos, buf_end, NULL, NULL);

    UA_SymmetricAlgorithmSecurityHeader symSecHeader;
    symSecHeader.tokenId = channel->securityToken.tokenId;
    ci->errorCode |= UA_encodeBinary(&symSecHeader.tokenId,
                                     &UA_TRANSPORT[UA_TRANSPORT_SYMMETRICALGORITHMSECURITYHEADER],
                                     &header_pos, buf_end, NULL, NULL);

    UA_SequenceHeader seqHeader;
    seqHeader.requestId = ci->requestId;
    seqHeader.sequenceNumber = UA_atomic_add(&channel->sendSequenceNumber, 1);
    ci->errorCode |= UA_encodeBinary(&seqHeader, &UA_TRANSPORT[UA_TRANSPORT_SEQUENCEHEADER],
                                     &header_pos, buf_end, NULL, NULL);

    /* Sign message */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        UA_ByteString dataToSign = ci->messageBuffer;
        dataToSign.length = pre_sig_length;
        UA_ByteString signature;
        signature.length = securityPolicy->symmetricModule.cryptoModule.
            getLocalSignatureSize(securityPolicy, channel->channelContext);
        signature.data = *buf_pos;
        ci->errorCode = securityPolicy->symmetricModule.cryptoModule.
            sign(securityPolicy, channel->channelContext, &dataToSign, &signature);
        if(ci->errorCode != UA_STATUSCODE_GOOD) {
            connection->releaseSendBuffer(channel->connection, &ci->messageBuffer);
            return ci->errorCode;
        }
    }

    /* Encrypt message */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        UA_ByteString dataToEncrypt;
        dataToEncrypt.data = ci->messageBuffer.data + UA_SECUREMH_AND_SYMALGH_LENGTH;
        dataToEncrypt.length = total_length - UA_SECUREMH_AND_SYMALGH_LENGTH;
        ci->errorCode = securityPolicy->symmetricModule.cryptoModule.
            encrypt(securityPolicy, channel->channelContext, &dataToEncrypt);
        if(ci->errorCode != UA_STATUSCODE_GOOD) {
            connection->releaseSendBuffer(channel->connection, &ci->messageBuffer);
            return ci->errorCode;
        }
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

        /* Forward the data pointer so that the payload is encoded after the
         * message header */
        *buf_pos = &ci->messageBuffer.data[UA_SECURE_MESSAGE_HEADER_LENGTH];
        *buf_end = &ci->messageBuffer.data[ci->messageBuffer.length];

        if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
           channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
            *buf_end -= securityPolicy->symmetricModule.cryptoModule.
            getLocalSignatureSize(securityPolicy, channel->channelContext);

        /* Hide a byte needed for padding */
        if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
            *buf_end -= 2;
    }
    return ci->errorCode;
}

UA_StatusCode
UA_SecureChannel_sendSymmetricMessage(UA_SecureChannel* channel, UA_UInt32 requestId,
                                      UA_MessageType messageType, const void *content,
                                      const UA_DataType *contentType) {
    const UA_SecurityPolicy *const securityPolicy = channel->securityPolicy;
    UA_Connection* connection = channel->connection;
    if(!connection)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Minimum required size */
    if(connection->localConf.sendBufferSize <= UA_SECURE_MESSAGE_HEADER_LENGTH)
        return UA_STATUSCODE_BADRESPONSETOOLARGE;

    /* Create the chunking info structure */
    UA_ChunkInfo ci;
    ci.channel = channel;
    ci.requestId = requestId;
    ci.chunksSoFar = 0;
    ci.messageSizeSoFar = 0;
    ci.final = false;
    ci.errorCode = UA_STATUSCODE_GOOD;
    ci.messageBuffer = UA_BYTESTRING_NULL;
    ci.messageType = messageType;

    /* Allocate the message buffer */
    UA_StatusCode retval =
        connection->getSendBuffer(connection, connection->localConf.sendBufferSize,
                                  &ci.messageBuffer);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Hide the message beginning where the header will be encoded */
    UA_Byte *buf_start = &ci.messageBuffer.data[UA_SECURE_MESSAGE_HEADER_LENGTH];
    const UA_Byte *buf_end = &ci.messageBuffer.data[ci.messageBuffer.length];

    /* Hide bytes for signature */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        buf_end -= securityPolicy->symmetricModule.cryptoModule.
        getLocalSignatureSize(securityPolicy, channel->channelContext);

    /* Hide one byte for padding */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        buf_end -= 2;

    /* Encode the message type */
    UA_NodeId typeId = UA_NODEID_NUMERIC(0, contentType->binaryEncodingId);
    retval = UA_encodeBinary(&typeId, &UA_TYPES[UA_TYPES_NODEID],
                             &buf_start, &buf_end, NULL, NULL);

    /* Encode with the chunking callback */
    retval |= UA_encodeBinary(content, contentType, &buf_start, &buf_end,
        (UA_exchangeEncodeBuffer)sendChunkSymmetric, &ci);

    /* Encoding failed, release the message */
    if(retval != UA_STATUSCODE_GOOD) {
        if(!ci.final) {
            /* the abort message was not sent */
            ci.errorCode = retval;
            sendChunkSymmetric(&ci, &buf_start, &buf_end);
        }
        return retval;
    }

    /* Encoding finished, send the final chunk */
    ci.final = UA_TRUE;
    return sendChunkSymmetric(&ci, &buf_start, &buf_end);
}

/*****************************/
/* Assemble Complete Message */
/*****************************/

static void
UA_SecureChannel_removeChunks(UA_SecureChannel *channel, UA_UInt32 requestId) {
    struct ChunkEntry *ch;
    LIST_FOREACH(ch, &channel->chunks, pointers) {
        if(ch->requestId == requestId) {
            UA_ByteString_deleteMembers(&ch->bytes);
            LIST_REMOVE(ch, pointers);
            UA_free(ch);
            return;
        }
    }
}

static UA_StatusCode
appendChunk(struct ChunkEntry* const chunkEntry, const UA_ByteString* const chunkBody) {
    UA_Byte* new_bytes = (UA_Byte*)
        UA_realloc(chunkEntry->bytes.data, chunkEntry->bytes.length + chunkBody->length);
    if(!new_bytes) {
        UA_ByteString_deleteMembers(&chunkEntry->bytes);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    chunkEntry->bytes.data = new_bytes;
    memcpy(&chunkEntry->bytes.data[chunkEntry->bytes.length], chunkBody->data, chunkBody->length);
    chunkEntry->bytes.length += chunkBody->length;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_SecureChannel_appendChunk(UA_SecureChannel* channel, UA_UInt32 requestId,
                             const UA_ByteString* chunkBody) {
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
UA_SecureChannel_finalizeChunk(UA_SecureChannel *channel, UA_UInt32 requestId,
                               const UA_ByteString *const chunkBody, UA_MessageType messageType,
                               UA_ProcessMessageCallback callback, void *application) {
    struct ChunkEntry* chunkEntry;
    LIST_FOREACH(chunkEntry, &channel->chunks, pointers) {
        if(chunkEntry->requestId == requestId)
            break;
    }

    UA_ByteString bytes;
    if(!chunkEntry) {
        bytes = *chunkBody;
    } else {
        UA_StatusCode retval = appendChunk(chunkEntry, chunkBody);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        bytes = chunkEntry->bytes;
        LIST_REMOVE(chunkEntry, pointers);
        UA_free(chunkEntry);
    }

    UA_StatusCode retval = callback(application, channel, messageType, requestId, &bytes);
    if(chunkEntry)
        UA_ByteString_deleteMembers(&bytes);
    return retval;
}

/****************************/
/* Process a received Chunk */
/****************************/

static UA_StatusCode
decryptChunk(UA_SecureChannel *channel, const UA_SecurityPolicyCryptoModule *cryptoModule,
             UA_ByteString *chunk, size_t offset, UA_UInt32 *requestId,
             UA_UInt32 *sequenceNumber, UA_ByteString *payload,
             UA_MessageType messageType) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    const UA_SecurityPolicy *securityPolicy = channel->securityPolicy;

    if(cryptoModule == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Decrypt the chunk. Always decrypt opn messages if mode not none */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT ||
       messageType == UA_MESSAGETYPE_OPN) {
        UA_ByteString cipherText = { chunk->length - offset, chunk->data + offset };
        retval = cryptoModule->decrypt(securityPolicy, channel->channelContext, &cipherText);
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
        sigsize = cryptoModule->getRemoteSignatureSize(securityPolicy, channel->channelContext);

        if(channel->securityMode != UA_MESSAGESECURITYMODE_NONE)
            paddingSize = chunk->data[chunk->length - sigsize - 1];

        size_t keyLength =
            cryptoModule->getRemoteEncryptionKeyLength(securityPolicy, channel->channelContext);
        if(keyLength > 2048) {
            paddingSize <<= 8; /* Extra padding size */
            paddingSize += chunk->data[chunk->length - sigsize - 2];
        }
        if(offset + paddingSize + sigsize >= chunk->length)
            return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

        /* Verify the signature */
        const UA_ByteString chunkDataToVerify = { chunk->length - sigsize, chunk->data };
        const UA_ByteString signature = { sigsize, chunk->data + chunk->length - sigsize };
        retval = securityPolicy->asymmetricModule.cryptoModule.
            verify(securityPolicy, channel->channelContext, &chunkDataToVerify, &signature);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    /* Decode the sequence header */
    UA_SequenceHeader sequenceHeader;
    retval = UA_SequenceHeader_decodeBinary(chunk, &offset, &sequenceHeader);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    *requestId = sequenceHeader.requestId;
    *sequenceNumber = sequenceHeader.sequenceNumber;
    payload->data = chunk->data + offset;
    payload->length = chunk->length - offset - sigsize - paddingSize;
    return UA_STATUSCODE_GOOD;
}

typedef UA_StatusCode(*UA_SequenceNumberCallback)(UA_SecureChannel *channel,
                                                  UA_UInt32 sequenceNumber);

static UA_StatusCode
processSequenceNumberAsym(UA_SecureChannel *const channel, UA_UInt32 sequenceNumber) {
    channel->receiveSequenceNumber = sequenceNumber;

    return UA_STATUSCODE_GOOD;
}

// TODO: We somehow need to make sure that a sequence number is never reused for the same tokenId
static UA_StatusCode
processSequenceNumberSym(UA_SecureChannel *const channel, UA_UInt32 sequenceNumber) {
    /* Does the sequence number match? */
    if(sequenceNumber != channel->receiveSequenceNumber + 1) {
        if(channel->receiveSequenceNumber + 1 > 4294966271 && sequenceNumber < 1024) // FIXME: Remove magic numbers :(
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
    #ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    if(tokenId != channel->securityToken.tokenId) {
        if(tokenId != channel->nextSecurityToken.tokenId)
            return UA_STATUSCODE_BADSECURECHANNELTOKENUNKNOWN;
        return UA_SecureChannel_revolveTokens(channel);
    }
    #endif

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
    case UA_MESSAGETYPE_ERR:
    {
        if(chunkType != UA_CHUNKTYPE_FINAL)
            return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
        chunkPayload.length = chunk->length - offset;
        chunkPayload.data = chunk->data + offset;
        return callback(application, channel, messageType, requestId, &chunkPayload);
    }

    case UA_MESSAGETYPE_MSG:
    case UA_MESSAGETYPE_CLO:
    {
        /* Decode and check the symmetric security header (tokenId) */
        UA_SymmetricAlgorithmSecurityHeader symmetricSecurityHeader;
        UA_SymmetricAlgorithmSecurityHeader_init(&symmetricSecurityHeader);
        retval = UA_SymmetricAlgorithmSecurityHeader_decodeBinary(chunk, &offset,
                                                                  &symmetricSecurityHeader);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;

        retval = checkSymHeader(channel, symmetricSecurityHeader.tokenId);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;

        cryptoModule = &channel->securityPolicy->symmetricModule.cryptoModule;
        sequenceNumberCallback = processSequenceNumberSym;

        break;
    }
    case UA_MESSAGETYPE_OPN:
    {
        /* Chunking not allowed for OPN */
        if(chunkType != UA_CHUNKTYPE_FINAL)
            return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;

        // Decode the asymmetric algorithm security header and
        // call the callback to perform checks.
        UA_AsymmetricAlgorithmSecurityHeader asymHeader;
        UA_AsymmetricAlgorithmSecurityHeader_init(&asymHeader);
        offset = UA_SECURE_CONVERSATION_MESSAGE_HEADER_LENGTH;
        retval = UA_AsymmetricAlgorithmSecurityHeader_decodeBinary(chunk,
                                                                   &offset,
                                                                   &asymHeader);
        if(retval != UA_STATUSCODE_GOOD)
            break;

        retval = checkAsymHeader(channel, &asymHeader);
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
        if(retval != UA_STATUSCODE_GOOD)
            break;

        cryptoModule = &channel->securityPolicy->asymmetricModule.cryptoModule;
        sequenceNumberCallback = processSequenceNumberAsym;

        break;
    }
    default:
        return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
    }

    /* Decrypt message */
    retval = decryptChunk(channel, cryptoModule, chunk, offset, &requestId,
                          &sequenceNumber, &chunkPayload, messageType);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Check the sequence number */
    if(sequenceNumberCallback == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    retval = sequenceNumberCallback(channel, sequenceNumber);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

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
