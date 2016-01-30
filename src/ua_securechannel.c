#include "ua_util.h"
#include "ua_securechannel.h"
#include "ua_session.h"
#include "ua_types_encoding_binary.h"
#include "ua_types_generated_encoding_binary.h"
#include "ua_transport_generated_encoding_binary.h"

void UA_SecureChannel_init(UA_SecureChannel *channel) {
    UA_MessageSecurityMode_init(&channel->securityMode);
    UA_ChannelSecurityToken_init(&channel->securityToken);
    UA_ChannelSecurityToken_init(&channel->nextSecurityToken);
    UA_AsymmetricAlgorithmSecurityHeader_init(&channel->clientAsymAlgSettings);
    UA_AsymmetricAlgorithmSecurityHeader_init(&channel->serverAsymAlgSettings);
    UA_ByteString_init(&channel->clientNonce);
    UA_ByteString_init(&channel->serverNonce);
    channel->sequenceNumber = 0;
    channel->connection = NULL;
    LIST_INIT(&channel->sessions);
}

void UA_SecureChannel_deleteMembersCleanup(UA_SecureChannel *channel) {
    UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&channel->serverAsymAlgSettings);
    UA_ByteString_deleteMembers(&channel->serverNonce);
    UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&channel->clientAsymAlgSettings);
    UA_ByteString_deleteMembers(&channel->clientNonce);
    UA_ChannelSecurityToken_deleteMembers(&channel->securityToken);
    UA_ChannelSecurityToken_deleteMembers(&channel->nextSecurityToken);
    UA_Connection *c = channel->connection;
    if(c) {
        UA_Connection_detachSecureChannel(c);
        if(c->close)
            c->close(c);
    }
    /* just remove the pointers and free the linked list (not the sessions) */
    struct SessionEntry *se, *temp;
    LIST_FOREACH_SAFE(se, &channel->sessions, pointers, temp) {
        if(se->session)
            se->session->channel = NULL;
        LIST_REMOVE(se, pointers);
        UA_free(se);
    }
}

//TODO implement real nonce generator - DUMMY function
UA_StatusCode UA_SecureChannel_generateNonce(UA_ByteString *nonce) {
    if(!(nonce->data = UA_malloc(1)))
        return UA_STATUSCODE_BADOUTOFMEMORY;
    nonce->length  = 1;
    nonce->data[0] = 'a';
    return UA_STATUSCODE_GOOD;
}

#if (__GNUC__ <= 4 && __GNUC_MINOR__ <= 6)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wunused-value"
#endif

void UA_SecureChannel_attachSession(UA_SecureChannel *channel, UA_Session *session) {
    struct SessionEntry *se = UA_malloc(sizeof(struct SessionEntry));
    if(!se)
        return;
    se->session = session;
#ifdef UA_ENABLE_MULTITHREADING
    if(uatomic_cmpxchg(&session->channel, NULL, channel) != NULL) {
        UA_free(se);
        return;
    }
#else
    if(session->channel != NULL) {
        UA_free(se);
        return;
    }
    session->channel = channel;
#endif
    LIST_INSERT_HEAD(&channel->sessions, se, pointers);
}

#if (__GNUC__ <= 4 && __GNUC_MINOR__ <= 6)
#pragma GCC diagnostic pop
#endif

void UA_SecureChannel_detachSession(UA_SecureChannel *channel, UA_Session *session) {
    if(session)
        session->channel = NULL;
    struct SessionEntry *se, *temp;
    LIST_FOREACH_SAFE(se, &channel->sessions, pointers, temp) {
        if(se->session != session)
            continue;
        LIST_REMOVE(se, pointers);
        UA_free(se);
        break;
    }
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

void UA_SecureChannel_revolveTokens(UA_SecureChannel *channel){
    if(channel->nextSecurityToken.tokenId==0) //no security token issued
        return;

    //FIXME: not thread-safe
    //swap tokens
    memcpy(&channel->securityToken, &channel->nextSecurityToken, sizeof(UA_ChannelSecurityToken));
    UA_ChannelSecurityToken_init(&channel->nextSecurityToken);
}

/* Chunking
   We expect that the OPN/CLO messages fit into a single chunk.

 */

typedef struct {
	UA_SecureChannel *channel;
	UA_UInt32 requestId;
    UA_UInt32 messageType;
    UA_UInt16 chunksSoFar;
    size_t messageSizeSoFar;
    UA_Boolean final;
} chunkInfo;

static UA_StatusCode
UA_SecureChannel_sendChunk(chunkInfo *ci, UA_ByteString *dst, size_t offset) {
    UA_SecureChannel *channel = ci->channel;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Connection *connection = channel->connection;
    if(!connection)
       return UA_STATUSCODE_BADINTERNALERROR;

    /* adjust the buffer where the header was hidden */
    dst->data = &dst->data[-24];
    dst->length += 24;
    offset += 24;
    ci->messageSizeSoFar += offset;

    UA_Boolean abort = (++ci->chunksSoFar >= connection->remoteConf.maxChunkCount ||
                        ci->messageSizeSoFar > connection->remoteConf.maxMessageSize);

	UA_SecureConversationMessageHeader respHeader;
	respHeader.messageHeader.messageTypeAndChunkType = ci->messageType;
    if(!abort) {
        if(ci->final)
            respHeader.messageHeader.messageTypeAndChunkType += UA_CHUNKTYPE_FINAL;
        else
            respHeader.messageHeader.messageTypeAndChunkType += UA_CHUNKTYPE_INTERMEDIATE;
    } else {
        // todo: abort
    }

	respHeader.secureChannelId = channel->securityToken.channelId;
	respHeader.messageHeader.messageSize = (UA_UInt32)offset;
	UA_SymmetricAlgorithmSecurityHeader symSecHeader;
	symSecHeader.tokenId = channel->securityToken.tokenId;

	UA_SequenceHeader seqHeader;
	seqHeader.requestId = ci->requestId;
#ifndef UA_ENABLE_MULTITHREADING
    seqHeader.sequenceNumber = ++channel->sequenceNumber;
#else
    seqHeader.sequenceNumber = uatomic_add_return(&channel->sequenceNumber, 1);
#endif

    offset = 0;
    UA_SecureConversationMessageHeader_encodeBinary(&respHeader, dst, &offset);
    UA_SymmetricAlgorithmSecurityHeader_encodeBinary(&symSecHeader, dst, &offset);
    UA_SequenceHeader_encodeBinary(&seqHeader, dst, &offset);
    dst->length = respHeader.messageHeader.messageSize;

    /* the buffer is freed internally */
    connection->send(channel->connection, dst);

    // get new buffer for the next chunk
    if(!ci->final) {
        retval = connection->getSendBuffer(connection, connection->localConf.sendBufferSize, dst);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        /* hide the header of the buffer */
        dst->data = &dst->data[24];
        dst->length = connection->localConf.sendBufferSize - 24;
    }

    return UA_STATUSCODE_GOOD;
}


UA_StatusCode
UA_SecureChannel_sendBinaryMessage(UA_SecureChannel *channel, UA_UInt32 requestId, const void *content,
                                   const UA_DataType *contentType) {
    UA_Connection *connection = channel->connection;
    if(!connection)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_NodeId typeId = contentType->typeId;
    if(typeId.identifierType != UA_NODEIDTYPE_NUMERIC)
        return UA_STATUSCODE_BADINTERNALERROR;
    typeId.identifier.numeric += UA_ENCODINGOFFSET_BINARY;

    UA_ByteString message;
    UA_ByteString_init(&message);
    UA_StatusCode retval = connection->getSendBuffer(connection, connection->localConf.sendBufferSize,
                                                     &message);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* hide the message beginning where the header will be encoded */
    message.data = &message.data[24];
    message.data -= 24;

    chunkInfo ci;
    ci.channel = channel;
    ci.requestId = requestId;
    ci.chunksSoFar = 0;
    ci.messageSizeSoFar = 0;
    ci.final = UA_FALSE;
    ci.messageType = UA_MESSAGETYPE_MSG;

    if(typeId.identifier.numeric == 446 || typeId.identifier.numeric == 449)
        ci.messageType = UA_MESSAGETYPE_OPN;
    else if(typeId.identifier.numeric == 452 || typeId.identifier.numeric == 455)
        ci.messageType = UA_MESSAGETYPE_CLO;

    size_t messagePos = 0;
    retval |= UA_NodeId_encodeBinary(&typeId, &message, &messagePos);
    if(ci.messageType != UA_MESSAGETYPE_MSG)
        /* jumps into sendMSGChunk to send the current chunk and continues encoding afterwards */
        retval |= UA_encodeBinary(content, contentType, (UA_exchangeEncodeBuffer)UA_SecureChannel_sendChunk,
                                  (void*)&ci, &message, &messagePos);
    else
        /* no chunking */
        retval |= UA_encodeBinary(content, contentType, NULL, NULL, &message, &messagePos);
    
    if(retval != UA_STATUSCODE_GOOD) {
        message.data = &message.data[-24];
        message.length += 24;
        connection->releaseSendBuffer(connection, &message);
        return retval;
    }
    
    /* content is encoded, send the final chunk */
    ci.final = UA_TRUE;
    retval = UA_SecureChannel_sendChunk(&ci, &message, messagePos);
    return retval;
}
