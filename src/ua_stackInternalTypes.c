#include "ua_stackInternalTypes.h"
#include "stdio.h"

UA_Int32 UA_MessageType_calcSize(UA_MessageType const * ptr){
	if(ptr==UA_NULL){return sizeof(UA_MessageType);}
	return 0
	 + 3 * sizeof(UA_Byte);
}

UA_Int32 UA_MessageType_encodeBinary(UA_MessageType const * src, UA_Int32* pos, UA_ByteString* dst){
	UA_Int32 retval = UA_SUCCESS;
	UA_Byte tmpBuf[3];
	tmpBuf[0] = (UA_Byte)((((UA_Int32)*src) >> 16) );
	tmpBuf[1] = (UA_Byte)((((UA_Int32)*src) >> 8));
	tmpBuf[2] = (UA_Byte)(((UA_Int32)*src));

	retval |= UA_Byte_encodeBinary(&(tmpBuf[0]),pos,dst);
	retval |= UA_Byte_encodeBinary(&(tmpBuf[1]),pos,dst);
	retval |= UA_Byte_encodeBinary(&(tmpBuf[2]),pos,dst);
	return retval;
}

UA_Int32 UA_MessageType_decodeBinary(UA_ByteString const * src, UA_Int32* pos, UA_MessageType* dst){
	UA_Int32 retval = UA_SUCCESS;
	UA_Byte tmpBuf[3];
	retval |= UA_Byte_decodeBinary(src,pos,&(tmpBuf[0]));//messageType to Byte representation
	retval |= UA_Byte_decodeBinary(src,pos,&(tmpBuf[1]));
	retval |= UA_Byte_decodeBinary(src,pos,&(tmpBuf[2]));
	*dst = (UA_MessageType)((UA_Int32)(tmpBuf[0]<<16) + (UA_Int32)(tmpBuf[1]<<8) + (UA_Int32)(tmpBuf[2]));
	return retval;
}
UA_TYPE_METHOD_DELETE_FREE(UA_MessageType)
UA_TYPE_METHOD_DELETEMEMBERS_NOACTION(UA_MessageType)
void UA_MessageType_printf(char *label, UA_MessageType* p) {
	UA_Byte* b = (UA_Byte*) p;
	printf("%s{%c%c%c}\n", label, b[2],b[1],b[0]);
}

UA_Int32 UA_OPCUATcpMessageHeader_calcSize(UA_OPCUATcpMessageHeader const * ptr) {
	if(ptr==UA_NULL){return sizeof(UA_OPCUATcpMessageHeader);}
	return 0
	 + UA_MessageType_calcSize(&(ptr->messageType))
	 + sizeof(UA_Byte) // isFinal
	 + sizeof(UA_UInt32) // messageSize
	;
}

UA_Int32 UA_OPCUATcpMessageHeader_encodeBinary(UA_OPCUATcpMessageHeader const * src, UA_Int32* pos, UA_ByteString* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_MessageType_encodeBinary(&(src->messageType),pos,dst);
	retval |= UA_Byte_encodeBinary(&(src->isFinal),pos,dst);
	retval |= UA_UInt32_encodeBinary(&(src->messageSize),pos,dst);
	return retval;
}

UA_Int32 UA_OPCUATcpMessageHeader_decodeBinary(UA_ByteString const * src, UA_Int32* pos, UA_OPCUATcpMessageHeader* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_MessageType_decodeBinary(src,pos,&(dst->messageType));
	retval |= UA_Byte_decodeBinary(src,pos,&(dst->isFinal));
	retval |= UA_UInt32_decodeBinary(src,pos,&(dst->messageSize));
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

UA_Int32 UA_OPCUATcpHelloMessage_encodeBinary(UA_OPCUATcpHelloMessage const * src, UA_Int32* pos, UA_ByteString* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt32_encodeBinary(&(src->protocolVersion),pos,dst);
	retval |= UA_UInt32_encodeBinary(&(src->receiveBufferSize),pos,dst);
	retval |= UA_UInt32_encodeBinary(&(src->sendBufferSize),pos,dst);
	retval |= UA_UInt32_encodeBinary(&(src->maxMessageSize),pos,dst);
	retval |= UA_UInt32_encodeBinary(&(src->maxChunkCount),pos,dst);
	retval |= UA_String_encodeBinary(&(src->endpointUrl),pos,dst);
	return retval;
}

UA_Int32 UA_OPCUATcpHelloMessage_decodeBinary(UA_ByteString const * src, UA_Int32* pos, UA_OPCUATcpHelloMessage* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt32_decodeBinary(src,pos,&(dst->protocolVersion));
	retval |= UA_UInt32_decodeBinary(src,pos,&(dst->receiveBufferSize));
	retval |= UA_UInt32_decodeBinary(src,pos,&(dst->sendBufferSize));
	retval |= UA_UInt32_decodeBinary(src,pos,&(dst->maxMessageSize));
	retval |= UA_UInt32_decodeBinary(src,pos,&(dst->maxChunkCount));
	retval |= UA_String_decodeBinary(src,pos,&(dst->endpointUrl));
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

UA_Int32 UA_OPCUATcpAcknowledgeMessage_encodeBinary(UA_OPCUATcpAcknowledgeMessage const * src, UA_Int32* pos, UA_ByteString* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt32_encodeBinary(&(src->protocolVersion),pos,dst);
	retval |= UA_UInt32_encodeBinary(&(src->receiveBufferSize),pos,dst);
	retval |= UA_UInt32_encodeBinary(&(src->sendBufferSize),pos,dst);
	retval |= UA_UInt32_encodeBinary(&(src->maxMessageSize),pos,dst);
	retval |= UA_UInt32_encodeBinary(&(src->maxChunkCount),pos,dst);
	return retval;
}

UA_Int32 UA_OPCUATcpAcknowledgeMessage_decodeBinary(UA_ByteString const * src, UA_Int32* pos, UA_OPCUATcpAcknowledgeMessage* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt32_decodeBinary(src,pos,&(dst->protocolVersion));
	retval |= UA_UInt32_decodeBinary(src,pos,&(dst->receiveBufferSize));
	retval |= UA_UInt32_decodeBinary(src,pos,&(dst->sendBufferSize));
	retval |= UA_UInt32_decodeBinary(src,pos,&(dst->maxMessageSize));
	retval |= UA_UInt32_decodeBinary(src,pos,&(dst->maxChunkCount));
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
	 // + UA_OPCUATcpMessageHeader_calcSize(ptr->tcpMessageHeader)
	 + sizeof(UA_UInt32) // secureChannelId
	;
}

UA_Int32 UA_SecureConversationMessageHeader_encodeBinary(UA_SecureConversationMessageHeader const * src, UA_Int32* pos, UA_ByteString* dst) {
	UA_Int32 retval = UA_SUCCESS;
	// retval |= UA_OPCUATcpMessageHeader_encode(src->tcpMessageHeader,pos,dst);
	retval |= UA_UInt32_encodeBinary(&(src->secureChannelId),pos,dst);
	return retval;
}

UA_Int32 UA_SecureConversationMessageHeader_decodeBinary(UA_ByteString const * src, UA_Int32* pos, UA_SecureConversationMessageHeader* dst) {
	UA_Int32 retval = UA_SUCCESS;
	//retval |= UA_alloc((void**)&(dst->tcpMessageHeader),UA_OPCUATcpMessageHeader_calcSize(UA_NULL));
	//retval |= UA_OPCUATcpMessageHeader_decode(src,pos,dst->tcpMessageHeader);
	retval |= UA_UInt32_decodeBinary(src,pos,&(dst->secureChannelId));
	return retval;
}

UA_Int32 UA_SecureConversationMessageHeader_delete(UA_SecureConversationMessageHeader* p) {
	UA_Int32 retval = UA_SUCCESS;
	// retval |= UA_SecureConversationMessageHeader_deleteMembers(p);
	retval |= UA_free(p);
	return retval;
    }
UA_Int32 UA_SecureConversationMessageHeader_deleteMembers(UA_SecureConversationMessageHeader* p) {
	UA_Int32 retval = UA_SUCCESS;
	// retval |= UA_OPCUATcpMessageHeader_delete(p->tcpMessageHeader);
	return retval;
}

UA_Int32 UA_AsymmetricAlgorithmSecurityHeader_calcSize(UA_AsymmetricAlgorithmSecurityHeader const * ptr) {
	if(ptr==UA_NULL){return sizeof(UA_AsymmetricAlgorithmSecurityHeader);}
	return 0
	 + UA_ByteString_calcSize(&(ptr->securityPolicyUri))
	 + UA_ByteString_calcSize(&(ptr->senderCertificate))
	 + UA_ByteString_calcSize(&(ptr->receiverCertificateThumbprint))
	;
}

UA_Int32 UA_AsymmetricAlgorithmSecurityHeader_encodeBinary(UA_AsymmetricAlgorithmSecurityHeader const * src, UA_Int32* pos, UA_ByteString* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_ByteString_encodeBinary(&(src->securityPolicyUri),pos,dst);
	retval |= UA_ByteString_encodeBinary(&(src->senderCertificate),pos,dst);
	retval |= UA_ByteString_encodeBinary(&(src->receiverCertificateThumbprint),pos,dst);
	return retval;
}

UA_Int32 UA_AsymmetricAlgorithmSecurityHeader_decodeBinary(UA_ByteString const * src, UA_Int32* pos, UA_AsymmetricAlgorithmSecurityHeader* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_ByteString_decodeBinary(src,pos,&(dst->securityPolicyUri));
	retval |= UA_ByteString_decodeBinary(src,pos,&(dst->senderCertificate));
	retval |= UA_ByteString_decodeBinary(src,pos,&(dst->receiverCertificateThumbprint));
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
UA_Int32 UA_AsymmetricAlgorithmSecurityHeader_init(UA_AsymmetricAlgorithmSecurityHeader* p) {
	UA_Int32 retval = UA_SUCCESS;
	if(p==UA_NULL) return UA_ERROR;
	retval |= UA_ByteString_init(&(p->securityPolicyUri));
	retval |= UA_ByteString_init(&(p->senderCertificate));
	retval |= UA_ByteString_init(&(p->receiverCertificateThumbprint));
	return retval;
}

UA_TYPE_METHOD_DECODEBINARY_AS(UA_SymmetricAlgorithmSecurityHeader, UA_UInt32)
UA_TYPE_METHOD_ENCODEBINARY_AS(UA_SymmetricAlgorithmSecurityHeader, UA_UInt32)
UA_TYPE_METHOD_DELETE_AS(UA_SymmetricAlgorithmSecurityHeader, UA_UInt32)
UA_TYPE_METHOD_DELETEMEMBERS_AS(UA_SymmetricAlgorithmSecurityHeader, UA_UInt32)
UA_TYPE_METHOD_CALCSIZE_AS(UA_SymmetricAlgorithmSecurityHeader, UA_UInt32)

UA_Int32 UA_SequenceHeader_calcSize(UA_SequenceHeader const * ptr) {
	if(ptr==UA_NULL){return sizeof(UA_SequenceHeader);}
	return 0
	 + sizeof(UA_UInt32) // sequenceNumber
	 + sizeof(UA_UInt32) // requestId
	;
}

UA_Int32 UA_SequenceHeader_encodeBinary(UA_SequenceHeader const * src, UA_Int32* pos, UA_ByteString* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt32_encodeBinary(&(src->sequenceNumber),pos,dst);
	retval |= UA_UInt32_encodeBinary(&(src->requestId),pos,dst);
	return retval;
}

UA_Int32 UA_SequenceHeader_decodeBinary(UA_ByteString const * src, UA_Int32* pos, UA_SequenceHeader* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt32_decodeBinary(src,pos,&(dst->sequenceNumber));
	retval |= UA_UInt32_decodeBinary(src,pos,&(dst->requestId));
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

UA_Int32 UA_SecureConversationMessageFooter_encodeBinary(UA_SecureConversationMessageFooter const * src, UA_Int32* pos, UA_ByteString* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_Int32_encodeBinary(&(src->paddingSize),pos,dst); // encode size
	retval |= UA_Array_encodeBinary((void const**) (src->padding),src->paddingSize, UA_BYTE,pos,dst);
	retval |= UA_Byte_encodeBinary(&(src->signature),pos,dst);
	return retval;
}

UA_Int32 UA_SecureConversationMessageFooter_decodeBinary(UA_ByteString const * src, UA_Int32* pos, UA_SecureConversationMessageFooter* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_Int32_decodeBinary(src,pos,&(dst->paddingSize)); // decode size
	retval |= UA_Array_new((void**)&(dst->padding),dst->paddingSize, UA_BYTE);
	retval |= UA_Array_decodeBinary(src,dst->paddingSize, UA_BYTE,pos,(void ** const) (dst->padding));
	retval |= UA_Byte_decodeBinary(src,pos,&(dst->signature));
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
	retval |= UA_Array_delete((void**)p->padding,p->paddingSize,UA_BYTE);
	return retval;
}

UA_Int32 UA_SecureConversationMessageAbortBody_calcSize(UA_SecureConversationMessageAbortBody const * ptr) {
	if(ptr==UA_NULL){return sizeof(UA_SecureConversationMessageAbortBody);}
	return 0
	 + sizeof(UA_UInt32) // error
	 + UA_String_calcSize(&(ptr->reason))
	;
}

UA_Int32 UA_SecureConversationMessageAbortBody_encodeBinary(UA_SecureConversationMessageAbortBody const * src, UA_Int32* pos, UA_ByteString* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt32_encodeBinary(&(src->error),pos,dst);
	retval |= UA_String_encodeBinary(&(src->reason),pos,dst);
	return retval;
}

UA_Int32 UA_SecureConversationMessageAbortBody_decodeBinary(UA_ByteString const * src, UA_Int32* pos, UA_SecureConversationMessageAbortBody* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt32_decodeBinary(src,pos,&(dst->error));
	retval |= UA_String_decodeBinary(src,pos,&(dst->reason));
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
