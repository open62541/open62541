#include "ua_util.h"
#include "ua_securechannel.h"
#include "ua_session.h"
#include "ua_types_encoding_binary.h"
#include "ua_types_generated_encoding_binary.h"
#include "ua_transport_generated_encoding_binary.h"

#define UA_SECURE_MESSAGE_HEADER_LENGTH 24

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
    LIST_INIT(&channel->chunks);
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

    struct ChunkEntry *ch, *temp_ch;
    LIST_FOREACH_SAFE(ch, &channel->chunks, pointers, temp_ch) {
        UA_ByteString_deleteMembers(&ch->bytes);
        LIST_REMOVE(ch, pointers);
        UA_free(ch);
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

#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
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

#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
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

void UA_SecureChannel_revolveTokens(UA_SecureChannel *channel) {
    if(channel->nextSecurityToken.tokenId == 0) //no security token issued
        return;

    //FIXME: not thread-safe
    memcpy(&channel->securityToken, &channel->nextSecurityToken, sizeof(UA_ChannelSecurityToken));
    UA_ChannelSecurityToken_init(&channel->nextSecurityToken);
}

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
    ci->messageSizeSoFar += offset;

    UA_Boolean chunkedMsg = (ci->chunksSoFar > 0 || ci->final == false);
    UA_Boolean abortMsg = ((++ci->chunksSoFar >= connection->remoteConf.maxChunkCount ||
                            ci->messageSizeSoFar > connection->remoteConf.maxMessageSize)) && chunkedMsg;

    /* Prepare the chunk headers */
    UA_SecureConversationMessageHeader respHeader;
    respHeader.secureChannelId = channel->securityToken.channelId;
    respHeader.messageHeader.messageTypeAndChunkType = ci->messageType;
    if(!abortMsg) {
        if(ci->final)
            respHeader.messageHeader.messageTypeAndChunkType += UA_CHUNKTYPE_FINAL;
        else
            respHeader.messageHeader.messageTypeAndChunkType += UA_CHUNKTYPE_INTERMEDIATE;
    } else {
        respHeader.messageHeader.messageTypeAndChunkType += UA_CHUNKTYPE_ABORT;
        ci->abort = true;
        UA_StatusCode retval = UA_STATUSCODE_BADTCPMESSAGETOOLARGE;
        UA_String errorMsg = UA_STRING("Encoded message too long");
        offset = UA_SECURE_MESSAGE_HEADER_LENGTH;
        UA_UInt32_encodeBinary(&retval,dst,&offset);
        UA_String_encodeBinary(&errorMsg,dst,&offset);
    }
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

    /* Encode the header at the beginning of the buffer */
    size_t offset_header = 0;
    UA_SecureConversationMessageHeader_encodeBinary(&respHeader, dst, &offset_header);
    UA_SymmetricAlgorithmSecurityHeader_encodeBinary(&symSecHeader, dst, &offset_header);
    UA_SequenceHeader_encodeBinary(&seqHeader, dst, &offset_header);

    /* Send the chunk, the buffer is freed in the network layer */
    dst->length = offset; /* set the buffer length to the content length */
    connection->send(channel->connection, dst);

    /* Replace with the buffer for the next chunk */
    if(!ci->final && !ci->abort) {
        UA_StatusCode retval = connection->getSendBuffer(connection, connection->localConf.sendBufferSize, dst);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        /* Hide the header of the buffer, so that the ensuing encoding does not overwrite anything */
        dst->data = &dst->data[UA_SECURE_MESSAGE_HEADER_LENGTH];
        dst->length = connection->localConf.sendBufferSize - UA_SECURE_MESSAGE_HEADER_LENGTH;
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_SecureChannel_sendBinaryMessage(UA_SecureChannel *channel, UA_UInt32 requestId, const void *content,
                                   const UA_DataType *contentType) {
    UA_Connection *connection = channel->connection;
    if(!connection)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Allocate the message buffer */
    UA_ByteString message;
    UA_StatusCode retval = connection->getSendBuffer(connection, connection->localConf.sendBufferSize, &message);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Hide the message beginning where the header will be encoded */
    message.data = &message.data[UA_SECURE_MESSAGE_HEADER_LENGTH];
    message.length -= UA_SECURE_MESSAGE_HEADER_LENGTH;

    /* Encode the message type */
    size_t messagePos = 0;
    UA_NodeId typeId = contentType->typeId; /* always numeric */
    typeId.identifier.numeric += UA_ENCODINGOFFSET_BINARY;
    UA_NodeId_encodeBinary(&typeId, &message, &messagePos);

    /* Encode with the chunking callback */
    UA_ChunkInfo ci;
    ci.channel = channel;
    ci.requestId = requestId;
    ci.chunksSoFar = 0;
    ci.messageSizeSoFar = 0;
    ci.final = false;
    ci.messageType = UA_MESSAGETYPE_MSG;
    ci.abort = false;
    if(typeId.identifier.numeric == 446 || typeId.identifier.numeric == 449)
        ci.messageType = UA_MESSAGETYPE_OPN;
    else if(typeId.identifier.numeric == 452 || typeId.identifier.numeric == 455)
        ci.messageType = UA_MESSAGETYPE_CLO;
    retval = UA_encodeBinary(content, contentType, (UA_exchangeEncodeBuffer)UA_SecureChannel_sendChunk,
                             &ci, &message, &messagePos);

    /* Abort message was sent, the buffer is already freed */
    if(ci.abort)
        return retval;

    /* Encoding failed, release the message */
    if(retval != UA_STATUSCODE_GOOD) {
        /* Unhide the beginning of the buffer (header) */
        message.data = &message.data[-UA_SECURE_MESSAGE_HEADER_LENGTH];
        connection->releaseSendBuffer(connection, &message);
        return retval;
    }

    /* Encoding finished, send the final chunk */
    ci.final = UA_TRUE;
    return UA_SecureChannel_sendChunk(&ci, &message, messagePos);
}
