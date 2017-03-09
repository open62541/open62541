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
#include <stdio.h>

#define UA_SECURE_MESSAGE_HEADER_LENGTH 24
#define UA_BITMASK_MESSAGETYPE 0x00ffffff
#define UA_BITMASK_CHUNKTYPE 0xff000000

void UA_SecureChannel_init(UA_SecureChannel *channel, UA_SecurityPolicies securityPolicies, UA_Logger logger) {
    memset(channel, 0, sizeof(UA_SecureChannel));
    channel->availableSecurityPolicies = securityPolicies;
    channel->logger = logger;
    /* Linked lists are also initialized by zeroing out */
    /* LIST_INIT(&channel->sessions); */
    /* LIST_INIT(&channel->chunks); */
}

void UA_SecureChannel_deleteMembersCleanup(UA_SecureChannel *channel) {
    /* Delete members */
    UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&channel->serverAsymAlgSettings);
    UA_ByteString_deleteMembers(&channel->serverNonce);
    UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&channel->clientAsymAlgSettings);
    UA_ByteString_deleteMembers(&channel->clientNonce);
    UA_ChannelSecurityToken_deleteMembers(&channel->securityToken);
    UA_ChannelSecurityToken_deleteMembers(&channel->nextSecurityToken);

    if (channel->securityContext != NULL)
    {
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

//TODO implement real nonce generator - DUMMY function
UA_StatusCode UA_SecureChannel_generateNonce(UA_ByteString *nonce) {
    if(!(nonce->data = (UA_Byte *)UA_malloc(1)))
        return UA_STATUSCODE_BADOUTOFMEMORY;
    nonce->length  = 1;
    nonce->data[0] = 'a';
    return UA_STATUSCODE_GOOD;
}

#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wunused-value"
#endif

void UA_SecureChannel_attachSession(UA_SecureChannel *channel, UA_Session *session) {
    struct SessionEntry *se = (struct SessionEntry *)UA_malloc(sizeof(struct SessionEntry));
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

void UA_SecureChannel_detachSession(UA_SecureChannel *channel, UA_Session *session) {
    if(session)
        session->channel = NULL;
    struct SessionEntry *se;
    LIST_FOREACH(se, &channel->sessions, pointers) {
        if(se->session == session)
            break;
    }
    if(!se)
        return;
    LIST_REMOVE(se, pointers);
    UA_free(se);
}

UA_Session * UA_SecureChannel_getSession(UA_SecureChannel *channel, UA_NodeId *token) {
    struct SessionEntry *se;
    LIST_FOREACH(se, &channel->sessions, pointers) {
        if(UA_NodeId_equal(&se->session->authenticationToken, token))
            break;
    }
    if(!se)
        return NULL;
    return se->session;
}

void UA_SecureChannel_revolveTokens(UA_SecureChannel *channel) {
    if(channel->nextSecurityToken.tokenId == 0) //no security token issued
        return;

    //FIXME: not thread-safe
    memcpy(&channel->securityToken, &channel->nextSecurityToken,
           sizeof(UA_ChannelSecurityToken));
    UA_ChannelSecurityToken_init(&channel->nextSecurityToken);

    // TODO: Generate new keys
}

/***********************/
/* Send Binary Message */
/***********************/

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////// TODO: The chunking procedure when sending needs to be modified to allow asymmetric messages.               //////
/////////// TODO: Currently the length of the message header is hardcoded and hidden / unhidden. This cannot           //////
/////////// TODO: be done when dealing with asymmetric messages since the asymmetric header does not have fixed length //////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static UA_StatusCode
UA_SecureChannel_sendChunk(UA_ChunkInfo *ci, UA_ByteString *dst, size_t offset) {
    UA_SecureChannel *channel = ci->channel;
    UA_Connection *connection = channel->connection;
    if(!connection)
       return UA_STATUSCODE_BADINTERNALERROR;

    /* adjust the buffer where the header was hidden */
    dst->data = &dst->data[-UA_SECURE_MESSAGE_HEADER_LENGTH];
    dst->length += UA_SECURE_MESSAGE_HEADER_LENGTH;
    offset += UA_SECURE_MESSAGE_HEADER_LENGTH;

    if(ci->messageSizeSoFar + offset > connection->remoteConf.maxMessageSize &&
       connection->remoteConf.maxMessageSize > 0)
        ci->errorCode = UA_STATUSCODE_BADRESPONSETOOLARGE;
    if(++ci->chunksSoFar > connection->remoteConf.maxChunkCount &&
       connection->remoteConf.maxChunkCount > 0)
        ci->errorCode = UA_STATUSCODE_BADRESPONSETOOLARGE;

    /* Prepare the chunk headers */
    UA_SecureConversationMessageHeader respHeader;
    respHeader.secureChannelId = channel->securityToken.channelId;
    respHeader.messageHeader.messageTypeAndChunkType = ci->messageType;
    if(ci->errorCode == UA_STATUSCODE_GOOD) {
        if(ci->final)
            respHeader.messageHeader.messageTypeAndChunkType += UA_CHUNKTYPE_FINAL;
        else
            respHeader.messageHeader.messageTypeAndChunkType += UA_CHUNKTYPE_INTERMEDIATE;
    } else {
        /* abort message */
        ci->final = true; /* mark as finished */
        respHeader.messageHeader.messageTypeAndChunkType += UA_CHUNKTYPE_ABORT;
        UA_String errorMsg;
        UA_String_init(&errorMsg);
        offset = UA_SECURE_MESSAGE_HEADER_LENGTH;
        UA_UInt32_encodeBinary(&ci->errorCode, dst, &offset);
        UA_String_encodeBinary(&errorMsg, dst, &offset);
    }
    respHeader.messageHeader.messageSize = (UA_UInt32)offset;
    ci->messageSizeSoFar += offset;

    /* Encode the header at the beginning of the buffer */
    UA_SymmetricAlgorithmSecurityHeader symSecHeader;
    symSecHeader.tokenId = channel->securityToken.tokenId;
    UA_SequenceHeader seqHeader;
    seqHeader.requestId = ci->requestId;
    seqHeader.sequenceNumber = UA_atomic_add(&channel->sendSequenceNumber, 1);
    size_t offset_header = 0;
    UA_SecureConversationMessageHeader_encodeBinary(&respHeader, dst, &offset_header);
    UA_SymmetricAlgorithmSecurityHeader_encodeBinary(&symSecHeader, dst, &offset_header);
    UA_SequenceHeader_encodeBinary(&seqHeader, dst, &offset_header);

    /* Send the chunk, the buffer is freed in the network layer */
    dst->length = offset; /* set the buffer length to the content length */
    connection->send(channel->connection, dst);

    /* Replace with the buffer for the next chunk */
    if(!ci->final) {
        UA_StatusCode retval =
            connection->getSendBuffer(connection, connection->localConf.sendBufferSize, dst);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        /* Forward the data pointer so that the payload is encoded after the message header.
         * TODO: This works but is a bit too clever. Instead, we could return an offset to the
         * binary encoding exchangeBuffer function. */
        dst->data = &dst->data[UA_SECURE_MESSAGE_HEADER_LENGTH];
        dst->length = connection->localConf.sendBufferSize - UA_SECURE_MESSAGE_HEADER_LENGTH;
    }
    return ci->errorCode;
}

UA_StatusCode
UA_SecureChannel_sendBinaryMessage(UA_SecureChannel *channel, UA_UInt32 requestId,
                                   const void *content, const UA_DataType *contentType) {
    UA_Connection *connection = channel->connection;
    if(!connection)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Allocate the message buffer */
    UA_ByteString message;
    UA_StatusCode retval =
        connection->getSendBuffer(connection, connection->localConf.sendBufferSize, &message);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Hide the message beginning where the header will be encoded */
    message.data = &message.data[UA_SECURE_MESSAGE_HEADER_LENGTH];
    message.length -= UA_SECURE_MESSAGE_HEADER_LENGTH;

    /* Encode the message type */
    size_t messagePos = 0;
    UA_NodeId typeId = contentType->typeId; /* always numeric */
    typeId.identifier.numeric = contentType->binaryEncodingId;
    UA_NodeId_encodeBinary(&typeId, &message, &messagePos);

    /* Encode with the chunking callback */
    UA_ChunkInfo ci;
    ci.channel = channel;
    ci.requestId = requestId;
    ci.chunksSoFar = 0;
    ci.messageSizeSoFar = 0;
    ci.final = false;
    ci.messageType = UA_MESSAGETYPE_MSG;
    ci.errorCode = UA_STATUSCODE_GOOD;
    if(typeId.identifier.numeric == UA_TYPES[UA_TYPES_OPENSECURECHANNELREQUEST].binaryEncodingId ||
       typeId.identifier.numeric == UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE].binaryEncodingId)
        ci.messageType = UA_MESSAGETYPE_OPN;
    else if(typeId.identifier.numeric == UA_TYPES[UA_TYPES_CLOSESECURECHANNELREQUEST].binaryEncodingId ||
            typeId.identifier.numeric == UA_TYPES[UA_TYPES_CLOSESECURECHANNELRESPONSE].binaryEncodingId)
        ci.messageType = UA_MESSAGETYPE_CLO;
    retval = UA_encodeBinary(content, contentType,
                             (UA_exchangeEncodeBuffer)UA_SecureChannel_sendChunk,
                             &ci, &message, &messagePos);

    /* Encoding failed, release the message */
    if(retval != UA_STATUSCODE_GOOD) {
        if(!ci.final) {
            /* the abort message was not sent */
            ci.errorCode = retval;
            UA_SecureChannel_sendChunk(&ci, &message, messagePos);
        }
        return retval;
    }

    /* Encoding finished, send the final chunk */
    ci.final = UA_TRUE;
    return UA_SecureChannel_sendChunk(&ci, &message, messagePos);
}

/***************************/
/* Process Received Chunks */
/***************************/

static void
UA_SecureChannel_removeChunk(UA_SecureChannel *channel, UA_UInt32 requestId) {
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
UA_SecureChannel_appendChunk(UA_SecureChannel *channel,
                             UA_UInt32 requestId,
                             const UA_ByteString *chunkBody) {
    /*
    // Check if the chunk fits into the message
    if(chunk->length - offset < chunklength) {
        // can't process all chunks for that request
        UA_SecureChannel_removeChunk(channel, requestId);
        return;
    }
    */

    /* Get the chunkentry */
    struct ChunkEntry *ch;
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

static UA_ByteString
UA_SecureChannel_finalizeChunk(UA_SecureChannel *channel,
                               UA_UInt32 requestId,
                               const UA_ByteString *chunkBody,
                               UA_Boolean *deleteChunk) {
    /*
    if(chunk->length - offset < chunklength) {
        // can't process all chunks for that request
        UA_SecureChannel_removeChunk(channel, requestId);
        return UA_BYTESTRING_NULL;
    }
    */

    struct ChunkEntry *chunkEntry;
    LIST_FOREACH(chunkEntry, &channel->chunks, pointers) {
        if(chunkEntry->requestId == requestId)
            break;
    }

    UA_ByteString bytes;
    if(!chunkEntry) {
        *deleteChunk = false;
        bytes.length = chunkBody->length;
        bytes.data = chunkBody->data;
    } else {
        *deleteChunk = true;
        appendChunk(chunkEntry, chunkBody); // TODO: pass error value on?
        bytes = chunkEntry->bytes;
        LIST_REMOVE(chunkEntry, pointers);
        UA_free(chunkEntry);
    }
    return bytes;
}

static UA_StatusCode
UA_SecureChannel_processSequenceNumber(UA_SecureChannel *channel, UA_UInt32 SequenceNumber) {
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
                                       size_t* const bodySize)
{
    size_t chunkSize = messageHeader->messageHeader.messageSize;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    /* Check the symmetric security header */
    UA_SymmetricAlgorithmSecurityHeader symmetricSecurityHeader;
    retval |= UA_SymmetricAlgorithmSecurityHeader_decodeBinary(chunk, processedBytes, &symmetricSecurityHeader);

    size_t messageAndSecurityHeaderOffset = *processedBytes;

    /* Does the token match? */
    if (symmetricSecurityHeader.tokenId != channel->securityToken.tokenId) {
        if (symmetricSecurityHeader.tokenId != channel->nextSecurityToken.tokenId)
            return UA_STATUSCODE_BADSECURECHANNELTOKENUNKNOWN;
        UA_SecureChannel_revolveTokens(channel);
    }

    // Decrypt message
    if (channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
    {
        const UA_ByteString cipherText = {
            .data = chunk->data + messageAndSecurityHeaderOffset,
            .length = chunkSize - messageAndSecurityHeaderOffset
        };

        UA_ByteString decrypted;
        retval |= UA_ByteString_allocBuffer(&decrypted, cipherText.length);

        if (retval != UA_STATUSCODE_GOOD)
        {
            return retval;
        }

        retval |= channel->securityPolicy->symmetricModule.decrypt(&cipherText, channel->securityContext, &decrypted);

        if (retval != UA_STATUSCODE_GOOD)
        {
            UA_ByteString_deleteMembers(&decrypted);
            return retval;
        }

        // Write back decrypted message for further processing
        memcpy(chunk->data + messageAndSecurityHeaderOffset, decrypted.data, decrypted.length);

        UA_ByteString_deleteMembers(&decrypted);
    }

    // Verify signature
    if (channel->securityMode == UA_MESSAGESECURITYMODE_SIGN || channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
    {
        // signature is made over everything except the signature itself.
        const UA_ByteString chunkDataToVerify = {
            .data = chunk->data,
            .length = chunkSize - channel->securityPolicy->symmetricModule.signingModule.signatureSize
        };
        const UA_ByteString signature = {
            .data = chunk->data + chunkDataToVerify.length, // Signature starts after the signed data
            .length = channel->securityPolicy->symmetricModule.signingModule.signatureSize
        };

        retval |= channel->securityPolicy->symmetricModule.signingModule.verify(&chunkDataToVerify,
            &signature,
            channel->securityContext);

        if (retval != UA_STATUSCODE_GOOD)
        {
            return retval;
        }
    }

    UA_SequenceHeader sequenceHeader;
    UA_SequenceHeader_init(&sequenceHeader);
    retval |= UA_SequenceHeader_decodeBinary(chunk, processedBytes, &sequenceHeader);
    if (retval != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;

    /* Does the sequence number match? */
    retval = UA_SecureChannel_processSequenceNumber(channel, sequenceHeader.sequenceNumber);
    if (retval != UA_STATUSCODE_GOOD)
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
                                           size_t* const bodySize)
{
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    size_t chunkSize = messageHeader->messageHeader.messageSize;

	UA_AsymmetricAlgorithmSecurityHeader clientAsymHeader;
    UA_AsymmetricAlgorithmSecurityHeader_init(&clientAsymHeader);
	retval |= UA_AsymmetricAlgorithmSecurityHeader_decodeBinary(chunk, processedBytes, &clientAsymHeader);
    size_t messageAndSecurityHeaderOffset = *processedBytes;
    if (retval != UA_STATUSCODE_GOOD)
    {
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&clientAsymHeader);
        return retval;
    }

    retval |= UA_AsymmetricAlgorithmSecurityHeader_copy(&clientAsymHeader, &channel->clientAsymAlgSettings);
    if (retval != UA_STATUSCODE_GOOD)
    {
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&clientAsymHeader);
        return retval;
    }


    if (channel->securityPolicy == NULL)
    {
        // iterate available security policies and choose the correct one
        const UA_SecurityPolicy* securityPolicy = NULL;
        UA_LOG_DEBUG(channel->logger,
                     UA_LOGCATEGORY_SECURECHANNEL,
                     "Trying to open connection with policy %.*s",
                     clientAsymHeader.securityPolicyUri.length,
                     clientAsymHeader.securityPolicyUri.data);
        for (size_t i = 0; i < channel->availableSecurityPolicies.count; ++i)
        {
            if (UA_ByteString_equal(&clientAsymHeader.securityPolicyUri,
                                    &channel->availableSecurityPolicies.policies[i].policyUri))
            {
                UA_LOG_DEBUG(channel->logger,
                             UA_LOGCATEGORY_SECURECHANNEL,
                             "Using security policy %s",
                            channel->availableSecurityPolicies.policies[i].policyUri.data);

                securityPolicy = &channel->availableSecurityPolicies.policies[i];
                break;
            }
        }

        if (securityPolicy == NULL)
        {
            // TODO: Abort connection?
            UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&clientAsymHeader);
            return UA_STATUSCODE_BADSECURITYPOLICYREJECTED;
        }

        channel->securityPolicy = securityPolicy;
    }
    else
    {
        // Policy not the same as when channel was originally opened
        if (!UA_ByteString_equal(&clientAsymHeader.securityPolicyUri,
                                 &channel->securityPolicy->policyUri))
        {
            UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&clientAsymHeader);
            return UA_STATUSCODE_BADSECURITYPOLICYREJECTED;
        }
    }

    // Create the channel context and parse the sender certificate used for the secureChannel.
    if (channel->securityContext == NULL)
    {
        UA_Channel_SecurityContext* channelContext = NULL;
        retval |= channel->securityPolicy->makeChannelContext(channel->securityPolicy, &channelContext);
        if (retval != UA_STATUSCODE_GOOD)
        {
            UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&clientAsymHeader);
            return retval;
        }
        
        retval |= channelContext->init(channelContext, channel->logger);
        if (retval != UA_STATUSCODE_GOOD)
        {
            channelContext->deleteMembers(channelContext);
            UA_free(channelContext);
            UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&clientAsymHeader);
            return retval;
        }

        // TODO: Do we need to parse the cert only once or for each chunk? (Assuming chunking is even possible??)
        retval |= channelContext->parseClientCertificate(channelContext, &clientAsymHeader.senderCertificate);
        if (retval != UA_STATUSCODE_GOOD)
        {
            channelContext->deleteMembers(channelContext);
            UA_free(channelContext);
            UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&clientAsymHeader);
            return retval;
        }

        channel->securityContext = channelContext;
    }

    // Verify the vertificate
    retval |= channel->securityPolicy->verifyCertificate(&clientAsymHeader.senderCertificate, &channel->securityPolicy->context);
    if (retval != UA_STATUSCODE_GOOD)
    {
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&clientAsymHeader);
        return retval;
    }
    
    // Decrypt message
    {
        const UA_ByteString cipherText = {
            .data = chunk->data + messageAndSecurityHeaderOffset,
            .length = chunkSize - messageAndSecurityHeaderOffset
        };

        UA_ByteString decrypted;
        retval |= UA_ByteString_allocBuffer(&decrypted, cipherText.length);

        if (retval != UA_STATUSCODE_GOOD)
        {
            UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&clientAsymHeader);
            return retval;
        }

        retval |= channel->securityPolicy->asymmetricModule.decrypt(&cipherText, &channel->securityPolicy->context, &decrypted);

        if (retval != UA_STATUSCODE_GOOD)
        {
            UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&clientAsymHeader);
            UA_ByteString_deleteMembers(&decrypted);
            return retval;
        }

        // Write back decrypted message for further processing
        memcpy(chunk->data + messageAndSecurityHeaderOffset, decrypted.data, decrypted.length);

        UA_ByteString_deleteMembers(&decrypted);
    }

    // Verify signature
    {
        // signature is made over everything except the signature itself.
        const UA_ByteString chunkDataToVerify = {
            .data = chunk->data,
            .length = chunkSize - channel->securityPolicy->asymmetricModule.signingModule.signatureSize
        };
        const UA_ByteString signature = {
            .data = chunk->data + chunkDataToVerify.length, // Signature starts after the signed data
            .length = channel->securityPolicy->asymmetricModule.signingModule.signatureSize
        };

        retval |= channel->securityPolicy->asymmetricModule.signingModule.verify(&chunkDataToVerify,
                                                                                 &signature,
                                                                                 channel->securityContext);

        if (retval != UA_STATUSCODE_GOOD)
        {
            UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&clientAsymHeader);
            return retval;
        }
    }

    UA_SequenceHeader sequenceHeader;
    UA_SequenceHeader_init(&sequenceHeader);
    retval |= UA_SequenceHeader_decodeBinary(chunk, processedBytes, &sequenceHeader);
    if (retval != UA_STATUSCODE_GOOD)
    {
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&clientAsymHeader);
        UA_SequenceHeader_deleteMembers(&sequenceHeader);
        return retval;
    }

    *requestId = sequenceHeader.requestId;

    // Set the starting sequence number
    channel->receiveSequenceNumber = sequenceHeader.sequenceNumber;

    // calculate body size
    size_t signatureSize = channel->securityPolicy->asymmetricModule.signingModule.signatureSize;
    // TODO: padding field is not present, when dealing with securitymode none messages
    // TODO: How to get securitymode? or is securitymode none only possible when security policy is none?
    //UA_Byte paddingSize = chunk->data[chunkSize - signatureSize - 1]; // TODO: Need to differentiate if extra padding byte was used
    UA_Byte paddingSize = 0;
    *bodySize = chunkSize - *processedBytes - signatureSize - paddingSize;
    if (*bodySize > chunkSize)
    {
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&clientAsymHeader);
        UA_SequenceHeader_deleteMembers(&sequenceHeader);
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }

    // Cleanup
    UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&clientAsymHeader);
    UA_SequenceHeader_deleteMembers(&sequenceHeader);

	return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_SecureChannel_processChunks(UA_SecureChannel *channel, const UA_ByteString *chunks,
                               UA_ProcessMessageCallback callback, void *application) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    // The offset in the chunks memory region. Always points to the start of a chunk
    size_t offset= 0;
    do {

        if (chunks->length > 3 && chunks->data[offset] == 'E' &&
                chunks->data[offset+1] == 'R' && chunks->data[offset+2] == 'R') {
            UA_TcpMessageHeader header;
            retval = UA_TcpMessageHeader_decodeBinary(chunks, &offset, &header);
            if(retval != UA_STATUSCODE_GOOD)
                break;

            UA_TcpErrorMessage errorMessage;
            retval = UA_TcpErrorMessage_decodeBinary(chunks, &offset, &errorMessage);
            if(retval != UA_STATUSCODE_GOOD)
                break;

            // dirty cast to pass errorMessage
            UA_UInt32 val = 0;
            callback(application, (UA_SecureChannel *)channel, (UA_MessageType)UA_MESSAGETYPE_ERR,
                     val, (const UA_ByteString*)&errorMessage);
            continue;
        }

        // The chunk that is being processed. The length exceeds the actual length of the chunk, since it is not yet known.
        UA_ByteString chunk = {.data = chunks->data + offset, .length = chunks->length - offset};
        size_t processedChunkBytes = 0;

        /* Decode message header */
        UA_SecureConversationMessageHeader messageHeader;
        retval = UA_SecureConversationMessageHeader_decodeBinary(&chunk, &processedChunkBytes, &messageHeader);
        if(retval != UA_STATUSCODE_GOOD)
            break;

        if (messageHeader.messageHeader.messageSize > chunk.length)
        {
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
        switch (messageHeader.messageHeader.messageTypeAndChunkType & UA_BITMASK_MESSAGETYPE)
        {
        case UA_MESSAGETYPE_OPN:
        {
			retval |= UA_SecureChannel_processAsymmetricOPNChunk(&chunk, channel, &messageHeader, &processedChunkBytes, &requestId, &bodySize);

			if(retval != UA_STATUSCODE_GOOD)
			{
				return retval;
			}
            break;
        }
        default:
            retval |= UA_SecureChannel_processSymmetricChunk(&chunk, channel, &messageHeader, &processedChunkBytes, &requestId, &bodySize);

            if (retval != UA_STATUSCODE_GOOD)
            {
                return retval;
            }
            break;
        }

        // Append the chunk and if it is final, call the message handling callback with the complete message
        size_t processed_header = processedChunkBytes;

        const UA_ByteString chunkBody = {
            .data = chunk.data + processed_header,
            .length = bodySize
        };

        switch(messageHeader.messageHeader.messageTypeAndChunkType & UA_BITMASK_CHUNKTYPE) {
        case UA_CHUNKTYPE_INTERMEDIATE:
            UA_SecureChannel_appendChunk(channel,
                                         requestId,
                                         &chunkBody);
            break;
        case UA_CHUNKTYPE_FINAL: {
            UA_Boolean realloced = false;
            UA_ByteString message =
                UA_SecureChannel_finalizeChunk(channel,
                                               requestId,
                                               &chunkBody,
                                               &realloced);
            if(message.length > 0) {
                callback(application,
                         (UA_SecureChannel *)channel,
                         (UA_MessageType)(messageHeader.messageHeader.messageTypeAndChunkType & UA_BITMASK_MESSAGETYPE),
                         requestId,
                         &message);
                if(realloced)
                    UA_ByteString_deleteMembers(&message);
            }
            break; }
        case UA_CHUNKTYPE_ABORT:
            UA_SecureChannel_removeChunk(channel, requestId);
            break;
        default:
            return UA_STATUSCODE_BADDECODINGERROR;
        }

        /* Jump to the end of the chunk */
        offset += messageHeader.messageHeader.messageSize;
    } while(chunks->length > offset);

    return retval;
}
