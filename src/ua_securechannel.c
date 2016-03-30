#include "ua_util.h"
#include "ua_securechannel.h"
#include "ua_session.h"
#include "ua_types_encoding_binary.h"
#include "ua_types_generated_encoding_binary.h"
#include "ua_transport_generated_encoding_binary.h"
#include "stdio.h"
#define SECURE_MESSAGE_HEADER 24
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

void UA_SecureChannel_revolveTokens(UA_SecureChannel *channel){
    if(channel->nextSecurityToken.tokenId==0) //no security token issued
        return;

    //FIXME: not thread-safe
    //swap tokens
    memcpy(&channel->securityToken, &channel->nextSecurityToken, sizeof(UA_ChannelSecurityToken));
    UA_ChannelSecurityToken_init(&channel->nextSecurityToken);
}


static UA_StatusCode
UA_SecureChannel_sendChunk(UA_ChunkInfo *ci, UA_ByteString *dst, size_t offset) {
    UA_SecureChannel *channel = ci->channel;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Connection *connection = channel->connection;
    if(!connection)
       return UA_STATUSCODE_BADINTERNALERROR;
    printf("UA_SecureChannel_sendChunk - entered \n");
    /* adjust the buffer where the header was hidden */
    dst->data = &dst->data[-SECURE_MESSAGE_HEADER];
    dst->length += SECURE_MESSAGE_HEADER;
    offset += SECURE_MESSAGE_HEADER;
    ci->messageSizeSoFar += offset;

    UA_Boolean chunkedMsg = (ci->chunksSoFar > 0 || ci->final == false);
    UA_Boolean abortMsg = ((++ci->chunksSoFar >= connection->remoteConf.maxChunkCount ||
                        ci->messageSizeSoFar > connection->remoteConf.maxMessageSize)) && chunkedMsg;

    UA_SecureConversationMessageHeader respHeader;
    respHeader.messageHeader.messageTypeAndChunkType = ci->messageType;



    if(abortMsg){
        retval = UA_STATUSCODE_BADTCPMESSAGETOOLARGE;
        UA_String errorMsg = UA_STRING_ALLOC("Encoded message too long");
        offset = SECURE_MESSAGE_HEADER;
		//set new message size and encode error code / error message
        dst->length = SECURE_MESSAGE_HEADER + sizeof(UA_UInt32) + UA_calcSizeBinary(&errorMsg, &UA_TYPES[UA_TYPES_STRING]);
        UA_UInt32_encodeBinary(&retval,dst,&offset);
        UA_String_encodeBinary(&errorMsg,dst,&offset);
        respHeader.messageHeader.messageTypeAndChunkType += UA_CHUNKTYPE_ABORT;
        UA_String_deleteMembers(&errorMsg);
        ci->abort = true;
        printf("aborting message \n");
	}else{
	    printf("offset = %i \n",offset);
	    dst->length = offset;
        if(ci->final)
            respHeader.messageHeader.messageTypeAndChunkType += UA_CHUNKTYPE_FINAL;
        else
            respHeader.messageHeader.messageTypeAndChunkType += UA_CHUNKTYPE_INTERMEDIATE;
    }

	respHeader.secureChannelId = channel->securityToken.channelId;
	respHeader.messageHeader.messageSize = (UA_UInt32)dst->length;
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
    //respHeader.messageHeader.messageSize = dst->length;
    UA_SecureConversationMessageHeader_encodeBinary(&respHeader, dst, &offset);
    UA_SymmetricAlgorithmSecurityHeader_encodeBinary(&symSecHeader, dst, &offset);
    UA_SequenceHeader_encodeBinary(&seqHeader, dst, &offset);

    printf("sending msg with buf at: %p, %i \n",dst->data,dst->length);
    connection->send(channel->connection, dst);

    // get new buffer for the next chunk
    if(!(ci->final || ci->abort)) {
        retval = connection->getSendBuffer(connection, connection->localConf.sendBufferSize, dst);
        printf("alloced mem for chk buf at: %p \n",dst->data);
        if(retval != UA_STATUSCODE_GOOD){
            printf("UA_SecureChannel_sendChunk - exited \n");
            return retval;
        }
        /* hide the header of the buffer */
        dst->data = &dst->data[SECURE_MESSAGE_HEADER];
        dst->length = connection->localConf.sendBufferSize - SECURE_MESSAGE_HEADER;
    }
    printf("UA_SecureChannel_sendChunk - exited \n");
    return retval;
}

UA_StatusCode
UA_SecureChannel_sendBinaryMessage(UA_SecureChannel *channel, UA_UInt32 requestId, const void *content,
                                   const UA_DataType *contentType) {
    UA_Connection *connection = channel->connection;
    printf("UA_SecureChannel_sendBinaryMessage - entered \n");
    if(!connection)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_NodeId typeId = contentType->typeId;
    if(typeId.identifierType != UA_NODEIDTYPE_NUMERIC)
        return UA_STATUSCODE_BADINTERNALERROR;
    typeId.identifier.numeric += UA_ENCODINGOFFSET_BINARY;

    UA_ByteString message;
    UA_StatusCode retval = connection->getSendBuffer(connection, connection->localConf.sendBufferSize,
                                                     &message);
    printf("alloced mem for msg buf at: %p \n",message.data);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* hide the message beginning where the header will be encoded */
    message.data = &message.data[SECURE_MESSAGE_HEADER];
    message.length -= SECURE_MESSAGE_HEADER;

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

    size_t messagePos = 0;
    retval |= UA_NodeId_encodeBinary(&typeId, &message, &messagePos);
    if(ci.messageType == UA_MESSAGETYPE_MSG){
        /* jumps into sendMSGChunk to send the current chunk and continues encoding afterwards */
        retval |= UA_encodeBinary(content, contentType, (UA_exchangeEncodeBuffer)UA_SecureChannel_sendChunk,
                                  (void*)&ci, &message, &messagePos);
        if(ci.abort){ //abort message sent, buffer is freed
            return UA_STATUSCODE_GOOD;
        }
    }
    else{
        /* no chunking */
        retval |= UA_encodeBinary(content, contentType, NULL, NULL, &message, &messagePos);
    }
    
    if(retval != UA_STATUSCODE_GOOD) {
        message.data = &message.data[-SECURE_MESSAGE_HEADER];
        //TODO why? The message is freed message.length += SECURE_MESSAGE_HEADER;
        connection->releaseSendBuffer(connection, &message);
        return retval;
    }
    
    /* content is encoded, send the final chunk */
    ci.final = UA_TRUE;
    retval = UA_SecureChannel_sendChunk(&ci, &message, messagePos);

    return retval;
}
