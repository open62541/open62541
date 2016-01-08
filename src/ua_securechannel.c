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
    UA_ChannelSecurityToken_deleteMembers(&channel->securityToken); //FIXME: not really needed
    UA_ChannelSecurityToken_deleteMembers(&channel->nextSecurityToken); //FIXME: not really needed
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

static UA_StatusCode UA_SecureChannel_sendChunkAndRenewBuffer(void *handle, UA_ByteString **dst, size_t *UA_RESTRICT offset,UA_Byte chunkType){
    UA_Request* request = (UA_Request*)handle;

    UA_SecureChannel * channel = request->channel;


    UA_Connection *connection = channel->connection;
    if(!connection)
       return UA_STATUSCODE_BADINTERNALERROR;

	UA_NodeId typeId = request->messageType.typeId;
	if(typeId.identifierType != UA_NODEIDTYPE_NUMERIC)
		return UA_STATUSCODE_BADINTERNALERROR;
	typeId.identifier.numeric += UA_ENCODINGOFFSET_BINARY;

	UA_SecureConversationMessageHeader respHeader;
	//TODO add proper message type
	respHeader.messageHeader.messageTypeAndFinal =  UA_MESSAGETYPEANDFINAL_MSGF;
	respHeader.messageHeader.messageSize = 0;
	respHeader.secureChannelId = channel->securityToken.channelId;
	respHeader.messageHeader.messageSize = *offset;
	UA_SymmetricAlgorithmSecurityHeader symSecHeader;
	symSecHeader.tokenId = channel->securityToken.tokenId;

	UA_SequenceHeader seqHeader;
	seqHeader.requestId = request->requestId;
#ifndef UA_ENABLE_MULTITHREADING
    seqHeader.sequenceNumber = ++channel->sequenceNumber;
#else
    seqHeader.sequenceNumber = uatomic_add_return(&channel->sequenceNumber, 1);
#endif
    *offset = 0;
    UA_SecureConversationMessageHeader_encodeBinary(&respHeader,NULL,NULL,  dst, offset);
    UA_SymmetricAlgorithmSecurityHeader_encodeBinary(&symSecHeader,NULL,NULL,  dst, offset);
    UA_SequenceHeader_encodeBinary(&seqHeader,NULL,NULL,  dst, offset);
    (*dst)->length = respHeader.messageHeader.messageSize;

    connection->send(channel->connection, *dst);
    connection->releaseRecvBuffer(channel->connection,*dst);

    UA_StatusCode retval = connection->getSendBuffer(connection, connection->remoteConf.recvBufferSize,
                                                         *dst);
    *offset = 0;
    return retval;

}
UA_StatusCode UA_SecureChannel_sendBinaryMessage(UA_SecureChannel *channel, UA_UInt32 requestId,
                                                  const void *content,
                                                  const UA_DataType *contentType) {
    UA_Connection *connection = channel->connection;
    if(!connection)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_NodeId typeId = contentType->typeId;
    if(typeId.identifierType != UA_NODEIDTYPE_NUMERIC)
        return UA_STATUSCODE_BADINTERNALERROR;
    typeId.identifier.numeric += UA_ENCODINGOFFSET_BINARY;

    UA_ByteString *message = UA_ByteString_new();
    UA_StatusCode retval = connection->getSendBuffer(connection, connection->remoteConf.recvBufferSize,
                                                     message);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    size_t messagePos = 24; // after the headers
    retval |= UA_NodeId_encodeBinary(&typeId,NULL,NULL, &message,&messagePos);
    retval |= UA_encodeBinary(content, contentType,NULL,NULL, &message, &messagePos);

    if(retval != UA_STATUSCODE_GOOD) {
        connection->releaseSendBuffer(connection, message);
        return retval;
    }

    UA_SecureConversationMessageHeader respHeader;
    respHeader.messageHeader.messageTypeAndFinal = UA_MESSAGETYPEANDFINAL_MSGF;
    respHeader.messageHeader.messageSize = 0;
    respHeader.secureChannelId = channel->securityToken.channelId;

    UA_SymmetricAlgorithmSecurityHeader symSecHeader;
    symSecHeader.tokenId = channel->securityToken.tokenId;

    UA_SequenceHeader seqHeader;
    seqHeader.requestId = requestId;
    /* now write the header with the size */
    respHeader.messageHeader.messageSize = messagePos;
#ifndef UA_ENABLE_MULTITHREADING
    seqHeader.sequenceNumber = ++channel->sequenceNumber;
#else
    seqHeader.sequenceNumber = uatomic_add_return(&channel->sequenceNumber, 1);
#endif

    messagePos = 0;
    UA_SecureConversationMessageHeader_encodeBinary(&respHeader,NULL,NULL,  &message, &messagePos);
    UA_SymmetricAlgorithmSecurityHeader_encodeBinary(&symSecHeader,NULL,NULL,  &message, &messagePos);
    UA_SequenceHeader_encodeBinary(&seqHeader,NULL,NULL,  &message, &messagePos);
    message->length = respHeader.messageHeader.messageSize;

    retval = connection->send(connection, message);
    UA_free(message);
    return retval;
}
