/*
 * opcua_stackInternalTypes.c
 *
 *  Created on: Mar 24, 2014
 *      Author: opcua
 */


#include "UA_stackInternalTypes.h"
UA_Int32 UA_MessageType_calcSize(UA_MessageType const * ptr){
	if(ptr==UA_NULL){return sizeof(UA_MessageType);}
	return 0
	 + 3 * sizeof(UA_Byte);
}

UA_Int32 UA_MessageType_encode(UA_MessageType const * src, UA_Int32* pos, char* dst){
	UA_Int32 retval = UA_SUCCESS;
	UA_Byte tmpBuf[3];
	tmpBuf[0] = (UA_Byte)((((UA_Int32)*src) >> 16) );
	tmpBuf[1] = (UA_Byte)((((UA_Int32)*src) >> 8));
	tmpBuf[2] = (UA_Byte)(((UA_Int32)*src));

	retval |= UA_Byte_encode(&(tmpBuf[0]),pos,dst);
	retval |= UA_Byte_encode(&(tmpBuf[1]),pos,dst);
	retval |= UA_Byte_encode(&(tmpBuf[2]),pos,dst);
	return retval;
}

UA_Int32 UA_MessageType_decode(char const * src, UA_Int32* pos, UA_MessageType* dst){
	UA_Int32 retval = UA_SUCCESS;
	UA_Byte tmpBuf[3];
	retval |= UA_Byte_decode(src,pos,&(tmpBuf[0]));//messageType to Byte representation
	retval |= UA_Byte_decode(src,pos,&(tmpBuf[1]));
	retval |= UA_Byte_decode(src,pos,&(tmpBuf[2]));
	*dst = (UA_MessageType)((UA_Int32)(tmpBuf[0]<<16) + (UA_Int32)(tmpBuf[1]<<8) + (UA_Int32)(tmpBuf[2]));
	return retval;
}
UA_TYPE_METHOD_DELETE_FREE(UA_MessageType)
UA_TYPE_METHOD_DELETEMEMBERS_NOACTION(UA_MessageType)

UA_Int32 UA_OPCUATcpMessageHeader_calcSize(UA_OPCUATcpMessageHeader const * ptr) {
	if(ptr==UA_NULL){return sizeof(UA_OPCUATcpMessageHeader);}
	return 0
	 + UA_MessageType_calcSize(&(ptr->messageType))
	 + sizeof(UA_Byte) // isFinal
	 + sizeof(UA_UInt32) // messageSize
	;
}

UA_Int32 UA_OPCUATcpMessageHeader_encode(UA_OPCUATcpMessageHeader const * src, UA_Int32* pos, char* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_MessageType_encode(&(src->messageType),pos,dst);
	retval |= UA_Byte_encode(&(src->isFinal),pos,dst);
	retval |= UA_UInt32_encode(&(src->messageSize),pos,dst);
	return retval;
}

UA_Int32 UA_OPCUATcpMessageHeader_decode(char const * src, UA_Int32* pos, UA_OPCUATcpMessageHeader* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_MessageType_decode(src,pos,&(dst->messageType));
	retval |= UA_Byte_decode(src,pos,&(dst->isFinal));
	retval |= UA_UInt32_decode(src,pos,&(dst->messageSize));
	return retval;
}

UA_Int32 UA_OPCUATcpMessageHeader_delete(UA_OPCUATcpMessageHeader* p) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_OPCUATcpMessageHeader_deleteMembers(p);
	retval |= UA_free(p);
	return retval;
    }
UA_Int32 UA_OPCUATcpMessageHeader_deleteMembers(UA_OPCUATcpMessageHeader* p) {
	UA_Int32 retval = UA_SUCCESS;
	return retval;
}

UA_Int32 UA_OPCUATcpHelloMessage_calcSize(UA_OPCUATcpHelloMessage const * ptr) {
	if(ptr==UA_NULL){return sizeof(UA_OPCUATcpHelloMessage);}
	return 0
	 + sizeof(UA_UInt32) // protocolVersion
	 + sizeof(UA_UInt32) // receiveBufferSize
	 + sizeof(UA_UInt32) // sendBufferSize
	 + sizeof(UA_UInt32) // maxMessageSize
	 + sizeof(UA_UInt32) // maxChunkCount
	 + UA_String_calcSize(&(ptr->endpointUrl))
	;
}

UA_Int32 UA_OPCUATcpHelloMessage_encode(UA_OPCUATcpHelloMessage const * src, UA_Int32* pos, char* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt32_encode(&(src->protocolVersion),pos,dst);
	retval |= UA_UInt32_encode(&(src->receiveBufferSize),pos,dst);
	retval |= UA_UInt32_encode(&(src->sendBufferSize),pos,dst);
	retval |= UA_UInt32_encode(&(src->maxMessageSize),pos,dst);
	retval |= UA_UInt32_encode(&(src->maxChunkCount),pos,dst);
	retval |= UA_String_encode(&(src->endpointUrl),pos,dst);
	return retval;
}

UA_Int32 UA_OPCUATcpHelloMessage_decode(char const * src, UA_Int32* pos, UA_OPCUATcpHelloMessage* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt32_decode(src,pos,&(dst->protocolVersion));
	retval |= UA_UInt32_decode(src,pos,&(dst->receiveBufferSize));
	retval |= UA_UInt32_decode(src,pos,&(dst->sendBufferSize));
	retval |= UA_UInt32_decode(src,pos,&(dst->maxMessageSize));
	retval |= UA_UInt32_decode(src,pos,&(dst->maxChunkCount));
	retval |= UA_String_decode(src,pos,&(dst->endpointUrl));
	return retval;
}

UA_Int32 UA_OPCUATcpHelloMessage_delete(UA_OPCUATcpHelloMessage* p) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_OPCUATcpHelloMessage_deleteMembers(p);
	retval |= UA_free(p);
	return retval;
    }
UA_Int32 UA_OPCUATcpHelloMessage_deleteMembers(UA_OPCUATcpHelloMessage* p) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_String_deleteMembers(&(p->endpointUrl));
	return retval;
}

UA_Int32 UA_OPCUATcpAcknowledgeMessage_calcSize(UA_OPCUATcpAcknowledgeMessage const * ptr) {
	if(ptr==UA_NULL){return sizeof(UA_OPCUATcpAcknowledgeMessage);}
	return 0
	 + sizeof(UA_UInt32) // protocolVersion
	 + sizeof(UA_UInt32) // receiveBufferSize
	 + sizeof(UA_UInt32) // sendBufferSize
	 + sizeof(UA_UInt32) // maxMessageSize
	 + sizeof(UA_UInt32) // maxChunkCount
	;
}

UA_Int32 UA_OPCUATcpAcknowledgeMessage_encode(UA_OPCUATcpAcknowledgeMessage const * src, UA_Int32* pos, char* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt32_encode(&(src->protocolVersion),pos,dst);
	retval |= UA_UInt32_encode(&(src->receiveBufferSize),pos,dst);
	retval |= UA_UInt32_encode(&(src->sendBufferSize),pos,dst);
	retval |= UA_UInt32_encode(&(src->maxMessageSize),pos,dst);
	retval |= UA_UInt32_encode(&(src->maxChunkCount),pos,dst);
	return retval;
}

UA_Int32 UA_OPCUATcpAcknowledgeMessage_decode(char const * src, UA_Int32* pos, UA_OPCUATcpAcknowledgeMessage* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt32_decode(src,pos,&(dst->protocolVersion));
	retval |= UA_UInt32_decode(src,pos,&(dst->receiveBufferSize));
	retval |= UA_UInt32_decode(src,pos,&(dst->sendBufferSize));
	retval |= UA_UInt32_decode(src,pos,&(dst->maxMessageSize));
	retval |= UA_UInt32_decode(src,pos,&(dst->maxChunkCount));
	return retval;
}

UA_Int32 UA_OPCUATcpAcknowledgeMessage_delete(UA_OPCUATcpAcknowledgeMessage* p) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_OPCUATcpAcknowledgeMessage_deleteMembers(p);
	retval |= UA_free(p);
	return retval;
    }
UA_Int32 UA_OPCUATcpAcknowledgeMessage_deleteMembers(UA_OPCUATcpAcknowledgeMessage* p) {
	UA_Int32 retval = UA_SUCCESS;
	return retval;
}

UA_Int32 UA_SecureConversationMessageHeader_calcSize(UA_SecureConversationMessageHeader const * ptr) {
	if(ptr==UA_NULL){return sizeof(UA_SecureConversationMessageHeader);}
	return 0
	 + UA_OPCUATcpMessageHeader_calcSize(ptr->tcpMessageHeader)
	 + sizeof(UA_UInt32) // secureChannelId
	;
}

UA_Int32 UA_SecureConversationMessageHeader_encode(UA_SecureConversationMessageHeader const * src, UA_Int32* pos, char* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_OPCUATcpMessageHeader_encode(src->tcpMessageHeader,pos,dst);
	retval |= UA_UInt32_encode(&(src->secureChannelId),pos,dst);
	return retval;
}

UA_Int32 UA_SecureConversationMessageHeader_decode(char const * src, UA_Int32* pos, UA_SecureConversationMessageHeader* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_alloc((void**)&(dst->tcpMessageHeader),UA_OPCUATcpMessageHeader_calcSize(UA_NULL));
	retval |= UA_OPCUATcpMessageHeader_decode(src,pos,dst->tcpMessageHeader);
	retval |= UA_UInt32_decode(src,pos,&(dst->secureChannelId));
	return retval;
}

UA_Int32 UA_SecureConversationMessageHeader_delete(UA_SecureConversationMessageHeader* p) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_SecureConversationMessageHeader_deleteMembers(p);
	retval |= UA_free(p);
	return retval;
    }
UA_Int32 UA_SecureConversationMessageHeader_deleteMembers(UA_SecureConversationMessageHeader* p) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_OPCUATcpMessageHeader_delete(p->tcpMessageHeader);
	return retval;
}

UA_Int32 UA_AsymmetricAlgorithmSecurityHeader_calcSize(UA_AsymmetricAlgorithmSecurityHeader const * ptr) {
	if(ptr==UA_NULL){return sizeof(UA_AsymmetricAlgorithmSecurityHeader);}
	return 0
	 + UA_ByteString_calcSize(&(ptr->securityPolicyUri))
	 + UA_ByteString_calcSize(&(ptr->senderCertificate))
	 + UA_ByteString_calcSize(&(ptr->receiverCertificateThumbprint))
	 + sizeof(UA_UInt32) // requestId
	;
}

UA_Int32 UA_AsymmetricAlgorithmSecurityHeader_encode(UA_AsymmetricAlgorithmSecurityHeader const * src, UA_Int32* pos, char* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_ByteString_encode(&(src->securityPolicyUri),pos,dst);
	retval |= UA_ByteString_encode(&(src->senderCertificate),pos,dst);
	retval |= UA_ByteString_encode(&(src->receiverCertificateThumbprint),pos,dst);
	retval |= UA_UInt32_encode(&(src->requestId),pos,dst);
	return retval;
}

UA_Int32 UA_AsymmetricAlgorithmSecurityHeader_decode(char const * src, UA_Int32* pos, UA_AsymmetricAlgorithmSecurityHeader* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_ByteString_decode(src,pos,&(dst->securityPolicyUri));
	retval |= UA_ByteString_decode(src,pos,&(dst->senderCertificate));
	retval |= UA_ByteString_decode(src,pos,&(dst->receiverCertificateThumbprint));
	retval |= UA_UInt32_decode(src,pos,&(dst->requestId));
	return retval;
}

UA_Int32 UA_AsymmetricAlgorithmSecurityHeader_delete(UA_AsymmetricAlgorithmSecurityHeader* p) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(p);
	retval |= UA_free(p);
	return retval;
    }
UA_Int32 UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(UA_AsymmetricAlgorithmSecurityHeader* p) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_ByteString_deleteMembers(&(p->securityPolicyUri));
	retval |= UA_ByteString_deleteMembers(&(p->senderCertificate));
	retval |= UA_ByteString_deleteMembers(&(p->receiverCertificateThumbprint));
	return retval;
}

UA_Int32 UA_SymmetricAlgorithmSecurityHeader_calcSize(UA_SymmetricAlgorithmSecurityHeader const * ptr) {
	if(ptr==UA_NULL){return sizeof(UA_SymmetricAlgorithmSecurityHeader);}
	return 0
	 + sizeof(UA_UInt32) // tokenId
	;
}

UA_Int32 UA_SymmetricAlgorithmSecurityHeader_encode(UA_SymmetricAlgorithmSecurityHeader const * src, UA_Int32* pos, char* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt32_encode(&(src->tokenId),pos,dst);
	return retval;
}

UA_Int32 UA_SymmetricAlgorithmSecurityHeader_decode(char const * src, UA_Int32* pos, UA_SymmetricAlgorithmSecurityHeader* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt32_decode(src,pos,&(dst->tokenId));
	return retval;
}

UA_Int32 UA_SymmetricAlgorithmSecurityHeader_delete(UA_SymmetricAlgorithmSecurityHeader* p) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_SymmetricAlgorithmSecurityHeader_deleteMembers(p);
	retval |= UA_free(p);
	return retval;
    }
UA_Int32 UA_SymmetricAlgorithmSecurityHeader_deleteMembers(UA_SymmetricAlgorithmSecurityHeader* p) {
	UA_Int32 retval = UA_SUCCESS;
	return retval;
}

UA_Int32 UA_SequenceHeader_calcSize(UA_SequenceHeader const * ptr) {
	if(ptr==UA_NULL){return sizeof(UA_SequenceHeader);}
	return 0
	 + sizeof(UA_UInt32) // sequenceNumber
	 + sizeof(UA_UInt32) // requestId
	;
}

UA_Int32 UA_SequenceHeader_encode(UA_SequenceHeader const * src, UA_Int32* pos, char* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt32_encode(&(src->sequenceNumber),pos,dst);
	retval |= UA_UInt32_encode(&(src->requestId),pos,dst);
	return retval;
}

UA_Int32 UA_SequenceHeader_decode(char const * src, UA_Int32* pos, UA_SequenceHeader* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt32_decode(src,pos,&(dst->sequenceNumber));
	retval |= UA_UInt32_decode(src,pos,&(dst->requestId));
	return retval;
}

UA_Int32 UA_SequenceHeader_delete(UA_SequenceHeader* p) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_SequenceHeader_deleteMembers(p);
	retval |= UA_free(p);
	return retval;
    }
UA_Int32 UA_SequenceHeader_deleteMembers(UA_SequenceHeader* p) {
	UA_Int32 retval = UA_SUCCESS;
	return retval;
}

UA_Int32 UA_SecureConversationMessageFooter_calcSize(UA_SecureConversationMessageFooter const * ptr) {
	if(ptr==UA_NULL){return sizeof(UA_SecureConversationMessageFooter);}
	return 0
	 + 0 //paddingSize is included in UA_Array_calcSize
	 + UA_Array_calcSize(ptr->paddingSize, UA_BYTE, (void const**) ptr->padding)
	 + sizeof(UA_Byte) // signature
	;
}

UA_Int32 UA_SecureConversationMessageFooter_encode(UA_SecureConversationMessageFooter const * src, UA_Int32* pos, char* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_Int32_encode(&(src->paddingSize),pos,dst); // encode size
	retval |= UA_Array_encode((void const**) (src->padding),src->paddingSize, UA_BYTE,pos,dst);
	retval |= UA_Byte_encode(&(src->signature),pos,dst);
	return retval;
}

UA_Int32 UA_SecureConversationMessageFooter_decode(char const * src, UA_Int32* pos, UA_SecureConversationMessageFooter* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_Int32_decode(src,pos,&(dst->paddingSize)); // decode size
	retval |= UA_alloc((void**)&(dst->padding),dst->paddingSize*sizeof(void*));
	retval |= UA_Array_decode(src,dst->paddingSize, UA_BYTE,pos,(void const**) (dst->padding));
	retval |= UA_Byte_decode(src,pos,&(dst->signature));
	return retval;
}

UA_Int32 UA_SecureConversationMessageFooter_delete(UA_SecureConversationMessageFooter* p) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_SecureConversationMessageFooter_deleteMembers(p);
	retval |= UA_free(p);
	return retval;
    }
UA_Int32 UA_SecureConversationMessageFooter_deleteMembers(UA_SecureConversationMessageFooter* p) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_Array_delete(p->padding,p->paddingSize);
	return retval;
}

UA_Int32 UA_SecureConversationMessageAbortBody_calcSize(UA_SecureConversationMessageAbortBody const * ptr) {
	if(ptr==UA_NULL){return sizeof(UA_SecureConversationMessageAbortBody);}
	return 0
	 + sizeof(UA_UInt32) // error
	 + UA_String_calcSize(&(ptr->reason))
	;
}

UA_Int32 UA_SecureConversationMessageAbortBody_encode(UA_SecureConversationMessageAbortBody const * src, UA_Int32* pos, char* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt32_encode(&(src->error),pos,dst);
	retval |= UA_String_encode(&(src->reason),pos,dst);
	return retval;
}

UA_Int32 UA_SecureConversationMessageAbortBody_decode(char const * src, UA_Int32* pos, UA_SecureConversationMessageAbortBody* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt32_decode(src,pos,&(dst->error));
	retval |= UA_String_decode(src,pos,&(dst->reason));
	return retval;
}

UA_Int32 UA_SecureConversationMessageAbortBody_delete(UA_SecureConversationMessageAbortBody* p) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_SecureConversationMessageAbortBody_deleteMembers(p);
	retval |= UA_free(p);
	return retval;
    }
UA_Int32 UA_SecureConversationMessageAbortBody_deleteMembers(UA_SecureConversationMessageAbortBody* p) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_String_deleteMembers(&(p->reason));
	return retval;
}
