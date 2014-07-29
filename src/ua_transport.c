#include "ua_transport.h"
#include "ua_types_encoding_binary.h"
#include "stdio.h"

UA_TYPE_DEFAULT(UA_MessageType)
UA_Int32 UA_MessageType_calcSize(UA_MessageType const * ptr){
	if(ptr==UA_NULL){return sizeof(UA_MessageType);}
	return 0
	 + 3 * sizeof(UA_Byte);
}

UA_Int32 UA_MessageType_encodeBinary(UA_MessageType const * src, UA_ByteString* dst, UA_UInt32 *offset){
	UA_Int32 retval = UA_SUCCESS;
	UA_Byte tmpBuf[3];
	tmpBuf[0] = (UA_Byte)((((UA_Int32)*src) >> 16) );
	tmpBuf[1] = (UA_Byte)((((UA_Int32)*src) >> 8));
	tmpBuf[2] = (UA_Byte)(((UA_Int32)*src));

	retval |= UA_Byte_encodeBinary(&(tmpBuf[0]),dst,offset);
	retval |= UA_Byte_encodeBinary(&(tmpBuf[1]),dst,offset);
	retval |= UA_Byte_encodeBinary(&(tmpBuf[2]),dst,offset);
	return retval;
}

UA_Int32 UA_MessageType_decodeBinary(UA_ByteString const * src, UA_UInt32 *offset, UA_MessageType* dst){
	UA_Int32 retval = UA_SUCCESS;
	UA_Byte tmpBuf[3];
	retval |= UA_Byte_decodeBinary(src,offset,&(tmpBuf[0])); //messageType to Byte representation
	retval |= UA_Byte_decodeBinary(src,offset,&(tmpBuf[1]));
	retval |= UA_Byte_decodeBinary(src,offset,&(tmpBuf[2]));
	*dst = (UA_MessageType)((UA_Int32)(tmpBuf[0]<<16) + (UA_Int32)(tmpBuf[1]<<8) + (UA_Int32)(tmpBuf[2]));
	return retval;
}

void UA_MessageType_printf(char *label, UA_MessageType* p) {
	UA_Byte* b = (UA_Byte*) p;
	printf("%s{%c%c%c}\n", label, b[2],b[1],b[0]);
}

UA_Int32 UA_OPCUATcpMessageHeader_calcSizeBinary(UA_OPCUATcpMessageHeader const * ptr) {
    	if(ptr==UA_NULL) return sizeof(UA_OPCUATcpMessageHeader);
    	return 0
	 + 3 // messageType
	 + sizeof(UA_Byte) // isFinal
	 + sizeof(UA_UInt32) // messageSize
	;
}

UA_Int32 UA_OPCUATcpMessageHeader_encodeBinary(UA_OPCUATcpMessageHeader const * src, UA_ByteString* dst, UA_UInt32 *offset) {
    	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_MessageType_encodeBinary(&src->messageType,dst,offset);
	retval |= UA_Byte_encodeBinary(&src->isFinal,dst,offset);
	retval |= UA_UInt32_encodeBinary(&src->messageSize,dst,offset);
	return retval;
}

UA_Int32 UA_OPCUATcpMessageHeader_decodeBinary(UA_ByteString const * src, UA_UInt32 *offset, UA_OPCUATcpMessageHeader * dst) {
    	UA_Int32 retval = UA_SUCCESS;
	UA_OPCUATcpMessageHeader_init(dst);
	CHECKED_DECODE(UA_MessageType_decodeBinary(src,offset,&dst->messageType), UA_OPCUATcpMessageHeader_deleteMembers(dst));
	CHECKED_DECODE(UA_Byte_decodeBinary(src,offset,&dst->isFinal), UA_OPCUATcpMessageHeader_deleteMembers(dst));
	CHECKED_DECODE(UA_UInt32_decodeBinary(src,offset,&dst->messageSize), UA_OPCUATcpMessageHeader_deleteMembers(dst));
	return retval;
}

UA_Int32 UA_OPCUATcpMessageHeader_delete(UA_OPCUATcpMessageHeader *p) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_OPCUATcpMessageHeader_deleteMembers(p);
	retval |= UA_free(p);
	return retval;
}

UA_Int32 UA_OPCUATcpMessageHeader_deleteMembers(UA_OPCUATcpMessageHeader *p) {
    	UA_Int32 retval = UA_SUCCESS;
	return retval;
}

UA_Int32 UA_OPCUATcpMessageHeader_init(UA_OPCUATcpMessageHeader *p) {
    	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_Byte_init(&p->isFinal);
	retval |= UA_UInt32_init(&p->messageSize);
	return retval;
}

UA_TYPE_NEW_DEFAULT(UA_OPCUATcpMessageHeader)
UA_Int32 UA_OPCUATcpMessageHeader_copy(const UA_OPCUATcpMessageHeader *src,UA_OPCUATcpMessageHeader *dst) {
    	if(src == UA_NULL || dst == UA_NULL) return UA_ERROR;
    	UA_Int32 retval = UA_SUCCESS;
	memcpy(dst, src, sizeof(UA_OPCUATcpMessageHeader));
	return retval;
}

UA_Int32 UA_OPCUATcpHelloMessage_calcSizeBinary(UA_OPCUATcpHelloMessage const * ptr) {
    	if(ptr==UA_NULL) return sizeof(UA_OPCUATcpHelloMessage);
    	return 0
	 + sizeof(UA_UInt32) // protocolVersion
	 + sizeof(UA_UInt32) // receiveBufferSize
	 + sizeof(UA_UInt32) // sendBufferSize
	 + sizeof(UA_UInt32) // maxMessageSize
	 + sizeof(UA_UInt32) // maxChunkCount
	 + UA_String_calcSizeBinary(&ptr->endpointUrl)
	;
}

UA_Int32 UA_OPCUATcpHelloMessage_encodeBinary(UA_OPCUATcpHelloMessage const * src, UA_ByteString* dst, UA_UInt32 *offset) {
    	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt32_encodeBinary(&src->protocolVersion,dst,offset);
	retval |= UA_UInt32_encodeBinary(&src->receiveBufferSize,dst,offset);
	retval |= UA_UInt32_encodeBinary(&src->sendBufferSize,dst,offset);
	retval |= UA_UInt32_encodeBinary(&src->maxMessageSize,dst,offset);
	retval |= UA_UInt32_encodeBinary(&src->maxChunkCount,dst,offset);
	retval |= UA_String_encodeBinary(&src->endpointUrl,dst,offset);
	return retval;
}

UA_Int32 UA_OPCUATcpHelloMessage_decodeBinary(UA_ByteString const * src, UA_UInt32 *offset, UA_OPCUATcpHelloMessage * dst) {
    	UA_Int32 retval = UA_SUCCESS;
	UA_OPCUATcpHelloMessage_init(dst);
	CHECKED_DECODE(UA_UInt32_decodeBinary(src,offset,&dst->protocolVersion), UA_OPCUATcpHelloMessage_deleteMembers(dst));
	CHECKED_DECODE(UA_UInt32_decodeBinary(src,offset,&dst->receiveBufferSize), UA_OPCUATcpHelloMessage_deleteMembers(dst));
	CHECKED_DECODE(UA_UInt32_decodeBinary(src,offset,&dst->sendBufferSize), UA_OPCUATcpHelloMessage_deleteMembers(dst));
	CHECKED_DECODE(UA_UInt32_decodeBinary(src,offset,&dst->maxMessageSize), UA_OPCUATcpHelloMessage_deleteMembers(dst));
	CHECKED_DECODE(UA_UInt32_decodeBinary(src,offset,&dst->maxChunkCount), UA_OPCUATcpHelloMessage_deleteMembers(dst));
	CHECKED_DECODE(UA_String_decodeBinary(src,offset,&dst->endpointUrl), UA_OPCUATcpHelloMessage_deleteMembers(dst));
	return retval;
}

UA_Int32 UA_OPCUATcpHelloMessage_delete(UA_OPCUATcpHelloMessage *p) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_OPCUATcpHelloMessage_deleteMembers(p);
	retval |= UA_free(p);
	return retval;
}

UA_Int32 UA_OPCUATcpHelloMessage_deleteMembers(UA_OPCUATcpHelloMessage *p) {
    	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_String_deleteMembers(&p->endpointUrl);
	return retval;
}

UA_Int32 UA_OPCUATcpHelloMessage_init(UA_OPCUATcpHelloMessage *p) {
    	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt32_init(&p->protocolVersion);
	retval |= UA_UInt32_init(&p->receiveBufferSize);
	retval |= UA_UInt32_init(&p->sendBufferSize);
	retval |= UA_UInt32_init(&p->maxMessageSize);
	retval |= UA_UInt32_init(&p->maxChunkCount);
	retval |= UA_String_init(&p->endpointUrl);
	return retval;
}

UA_TYPE_NEW_DEFAULT(UA_OPCUATcpHelloMessage)
UA_Int32 UA_OPCUATcpHelloMessage_copy(const UA_OPCUATcpHelloMessage *src,UA_OPCUATcpHelloMessage *dst) {
    	if(src == UA_NULL || dst == UA_NULL) return UA_ERROR;
    	UA_Int32 retval = UA_SUCCESS;
	memcpy(dst, src, sizeof(UA_OPCUATcpHelloMessage));
	retval |= UA_String_copy(&src->endpointUrl,&dst->endpointUrl);
	return retval;
}

UA_Int32 UA_OPCUATcpAcknowledgeMessage_calcSizeBinary(UA_OPCUATcpAcknowledgeMessage const * ptr) {
    	if(ptr==UA_NULL) return sizeof(UA_OPCUATcpAcknowledgeMessage);
    	return 0
	 + sizeof(UA_UInt32) // protocolVersion
	 + sizeof(UA_UInt32) // receiveBufferSize
	 + sizeof(UA_UInt32) // sendBufferSize
	 + sizeof(UA_UInt32) // maxMessageSize
	 + sizeof(UA_UInt32) // maxChunkCount
	;
}

UA_Int32 UA_OPCUATcpAcknowledgeMessage_encodeBinary(UA_OPCUATcpAcknowledgeMessage const * src, UA_ByteString* dst, UA_UInt32 *offset) {
    	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt32_encodeBinary(&src->protocolVersion,dst,offset);
	retval |= UA_UInt32_encodeBinary(&src->receiveBufferSize,dst,offset);
	retval |= UA_UInt32_encodeBinary(&src->sendBufferSize,dst,offset);
	retval |= UA_UInt32_encodeBinary(&src->maxMessageSize,dst,offset);
	retval |= UA_UInt32_encodeBinary(&src->maxChunkCount,dst,offset);
	return retval;
}

UA_Int32 UA_OPCUATcpAcknowledgeMessage_decodeBinary(UA_ByteString const * src, UA_UInt32 *offset, UA_OPCUATcpAcknowledgeMessage * dst) {
    	UA_Int32 retval = UA_SUCCESS;
	UA_OPCUATcpAcknowledgeMessage_init(dst);
	CHECKED_DECODE(UA_UInt32_decodeBinary(src,offset,&dst->protocolVersion), UA_OPCUATcpAcknowledgeMessage_deleteMembers(dst));
	CHECKED_DECODE(UA_UInt32_decodeBinary(src,offset,&dst->receiveBufferSize), UA_OPCUATcpAcknowledgeMessage_deleteMembers(dst));
	CHECKED_DECODE(UA_UInt32_decodeBinary(src,offset,&dst->sendBufferSize), UA_OPCUATcpAcknowledgeMessage_deleteMembers(dst));
	CHECKED_DECODE(UA_UInt32_decodeBinary(src,offset,&dst->maxMessageSize), UA_OPCUATcpAcknowledgeMessage_deleteMembers(dst));
	CHECKED_DECODE(UA_UInt32_decodeBinary(src,offset,&dst->maxChunkCount), UA_OPCUATcpAcknowledgeMessage_deleteMembers(dst));
	return retval;
}

UA_Int32 UA_OPCUATcpAcknowledgeMessage_delete(UA_OPCUATcpAcknowledgeMessage *p) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_OPCUATcpAcknowledgeMessage_deleteMembers(p);
	retval |= UA_free(p);
	return retval;
}

UA_Int32 UA_OPCUATcpAcknowledgeMessage_deleteMembers(UA_OPCUATcpAcknowledgeMessage *p) {
    	UA_Int32 retval = UA_SUCCESS;
	return retval;
}

UA_Int32 UA_OPCUATcpAcknowledgeMessage_init(UA_OPCUATcpAcknowledgeMessage *p) {
    	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt32_init(&p->protocolVersion);
	retval |= UA_UInt32_init(&p->receiveBufferSize);
	retval |= UA_UInt32_init(&p->sendBufferSize);
	retval |= UA_UInt32_init(&p->maxMessageSize);
	retval |= UA_UInt32_init(&p->maxChunkCount);
	return retval;
}

UA_TYPE_NEW_DEFAULT(UA_OPCUATcpAcknowledgeMessage)
UA_Int32 UA_OPCUATcpAcknowledgeMessage_copy(const UA_OPCUATcpAcknowledgeMessage *src,UA_OPCUATcpAcknowledgeMessage *dst) {
    	if(src == UA_NULL || dst == UA_NULL) return UA_ERROR;
    	UA_Int32 retval = UA_SUCCESS;
	memcpy(dst, src, sizeof(UA_OPCUATcpAcknowledgeMessage));
	return retval;
}

UA_Int32 UA_SecureConversationMessageHeader_calcSizeBinary(UA_SecureConversationMessageHeader const * ptr) {
    	if(ptr==UA_NULL) return sizeof(UA_SecureConversationMessageHeader);
    	return 0
	 + sizeof(UA_OPCUATcpMessageHeader) // messageHeader CHECK: delete this or use in binary_secure.c
	 + sizeof(UA_UInt32) // secureChannelId
	;
}

UA_Int32 UA_SecureConversationMessageHeader_encodeBinary(UA_SecureConversationMessageHeader const * src, UA_ByteString* dst, UA_UInt32 *offset) {
    	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_OPCUATcpMessageHeader_encodeBinary(&src->messageHeader,dst,offset);
	retval |= UA_UInt32_encodeBinary(&src->secureChannelId,dst,offset);
	return retval;
}

UA_Int32 UA_SecureConversationMessageHeader_decodeBinary(UA_ByteString const * src, UA_UInt32 *offset, UA_SecureConversationMessageHeader * dst) {
    	UA_Int32 retval = UA_SUCCESS;
	UA_SecureConversationMessageHeader_init(dst);
	CHECKED_DECODE(UA_OPCUATcpMessageHeader_decodeBinary(src,offset,&dst->messageHeader), UA_SecureConversationMessageHeader_deleteMembers(dst));
	CHECKED_DECODE(UA_UInt32_decodeBinary(src,offset,&dst->secureChannelId), UA_SecureConversationMessageHeader_deleteMembers(dst));
	return retval;
}

UA_Int32 UA_SecureConversationMessageHeader_delete(UA_SecureConversationMessageHeader *p) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_SecureConversationMessageHeader_deleteMembers(p);
	retval |= UA_free(p);
	return retval;
}

UA_Int32 UA_SecureConversationMessageHeader_deleteMembers(UA_SecureConversationMessageHeader *p) {
    	UA_Int32 retval = UA_SUCCESS;
	return retval;
}

UA_Int32 UA_SecureConversationMessageHeader_init(UA_SecureConversationMessageHeader *p) {
    	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_OPCUATcpMessageHeader_init(&p->messageHeader);
	retval |= UA_UInt32_init(&p->secureChannelId);
	return retval;
}

UA_TYPE_NEW_DEFAULT(UA_SecureConversationMessageHeader)
UA_Int32 UA_SecureConversationMessageHeader_copy(const UA_SecureConversationMessageHeader *src,UA_SecureConversationMessageHeader *dst) {
    	if(src == UA_NULL || dst == UA_NULL) return UA_ERROR;
    	UA_Int32 retval = UA_SUCCESS;
	memcpy(dst, src, sizeof(UA_SecureConversationMessageHeader));
	return retval;
}

UA_Int32 UA_AsymmetricAlgorithmSecurityHeader_calcSizeBinary(UA_AsymmetricAlgorithmSecurityHeader const * ptr) {
    	if(ptr==UA_NULL) return sizeof(UA_AsymmetricAlgorithmSecurityHeader);
    	return 0
	 + UA_ByteString_calcSizeBinary(&ptr->securityPolicyUri)
	 + UA_ByteString_calcSizeBinary(&ptr->senderCertificate)
	 + UA_ByteString_calcSizeBinary(&ptr->receiverCertificateThumbprint)
			//	 + sizeof(UA_UInt32) // requestId
	;
}

UA_Int32 UA_AsymmetricAlgorithmSecurityHeader_encodeBinary(UA_AsymmetricAlgorithmSecurityHeader const * src, UA_ByteString* dst, UA_UInt32 *offset) {
    	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_ByteString_encodeBinary(&src->securityPolicyUri,dst,offset);
	retval |= UA_ByteString_encodeBinary(&src->senderCertificate,dst,offset);
	retval |= UA_ByteString_encodeBinary(&src->receiverCertificateThumbprint,dst,offset);
	//	retval |= UA_UInt32_encodeBinary(&src->requestId,dst,offset);
	return retval;
}

UA_Int32 UA_AsymmetricAlgorithmSecurityHeader_decodeBinary(UA_ByteString const * src, UA_UInt32 *offset, UA_AsymmetricAlgorithmSecurityHeader * dst) {
    	UA_Int32 retval = UA_SUCCESS;
	UA_AsymmetricAlgorithmSecurityHeader_init(dst);
	CHECKED_DECODE(UA_ByteString_decodeBinary(src,offset,&dst->securityPolicyUri), UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(dst));
	CHECKED_DECODE(UA_ByteString_decodeBinary(src,offset,&dst->senderCertificate), UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(dst));
	CHECKED_DECODE(UA_ByteString_decodeBinary(src,offset,&dst->receiverCertificateThumbprint), UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(dst));
	//	CHECKED_DECODE(UA_UInt32_decodeBinary(src,offset,&dst->requestId), UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(dst));
	return retval;
}

UA_Int32 UA_AsymmetricAlgorithmSecurityHeader_delete(UA_AsymmetricAlgorithmSecurityHeader *p) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(p);
	retval |= UA_free(p);
	return retval;
}

UA_Int32 UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(UA_AsymmetricAlgorithmSecurityHeader *p) {
    	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_ByteString_deleteMembers(&p->securityPolicyUri);
	retval |= UA_ByteString_deleteMembers(&p->senderCertificate);
	retval |= UA_ByteString_deleteMembers(&p->receiverCertificateThumbprint);
	return retval;
}

UA_Int32 UA_AsymmetricAlgorithmSecurityHeader_init(UA_AsymmetricAlgorithmSecurityHeader *p) {
    	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_ByteString_init(&p->securityPolicyUri);
	retval |= UA_ByteString_init(&p->senderCertificate);
	retval |= UA_ByteString_init(&p->receiverCertificateThumbprint);
	//retval |= UA_UInt32_init(&p->requestId);
	return retval;
}

UA_TYPE_NEW_DEFAULT(UA_AsymmetricAlgorithmSecurityHeader)
UA_Int32 UA_AsymmetricAlgorithmSecurityHeader_copy(const UA_AsymmetricAlgorithmSecurityHeader *src,UA_AsymmetricAlgorithmSecurityHeader *dst) {
    	if(src == UA_NULL || dst == UA_NULL) return UA_ERROR;
    	UA_Int32 retval = UA_SUCCESS;
	memcpy(dst, src, sizeof(UA_AsymmetricAlgorithmSecurityHeader));
	retval |= UA_ByteString_copy(&src->securityPolicyUri,&dst->securityPolicyUri);
	retval |= UA_ByteString_copy(&src->senderCertificate,&dst->senderCertificate);
	retval |= UA_ByteString_copy(&src->receiverCertificateThumbprint,&dst->receiverCertificateThumbprint);
	return retval;
}

UA_Int32 UA_SymmetricAlgorithmSecurityHeader_calcSizeBinary(UA_SymmetricAlgorithmSecurityHeader const * ptr) {
    	if(ptr==UA_NULL) return sizeof(UA_SymmetricAlgorithmSecurityHeader);
    	return 0
	 + sizeof(UA_UInt32) // tokenId
	;
}

UA_Int32 UA_SymmetricAlgorithmSecurityHeader_encodeBinary(UA_SymmetricAlgorithmSecurityHeader const * src, UA_ByteString* dst, UA_UInt32 *offset) {
    	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt32_encodeBinary(&src->tokenId,dst,offset);
	return retval;
}

UA_Int32 UA_SymmetricAlgorithmSecurityHeader_decodeBinary(UA_ByteString const * src, UA_UInt32 *offset, UA_SymmetricAlgorithmSecurityHeader * dst) {
    	UA_Int32 retval = UA_SUCCESS;
	UA_SymmetricAlgorithmSecurityHeader_init(dst);
	CHECKED_DECODE(UA_UInt32_decodeBinary(src,offset,&dst->tokenId), UA_SymmetricAlgorithmSecurityHeader_deleteMembers(dst));
	return retval;
}

UA_Int32 UA_SymmetricAlgorithmSecurityHeader_delete(UA_SymmetricAlgorithmSecurityHeader *p) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_SymmetricAlgorithmSecurityHeader_deleteMembers(p);
	retval |= UA_free(p);
	return retval;
}

UA_Int32 UA_SymmetricAlgorithmSecurityHeader_deleteMembers(UA_SymmetricAlgorithmSecurityHeader *p) {
    	UA_Int32 retval = UA_SUCCESS;
	return retval;
}

UA_Int32 UA_SymmetricAlgorithmSecurityHeader_init(UA_SymmetricAlgorithmSecurityHeader *p) {
    	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt32_init(&p->tokenId);
	return retval;
}

UA_TYPE_NEW_DEFAULT(UA_SymmetricAlgorithmSecurityHeader)
UA_Int32 UA_SymmetricAlgorithmSecurityHeader_copy(const UA_SymmetricAlgorithmSecurityHeader *src,UA_SymmetricAlgorithmSecurityHeader *dst) {
    	if(src == UA_NULL || dst == UA_NULL) return UA_ERROR;
    	UA_Int32 retval = UA_SUCCESS;
	memcpy(dst, src, sizeof(UA_SymmetricAlgorithmSecurityHeader));
	return retval;
}

UA_Int32 UA_SequenceHeader_calcSizeBinary(UA_SequenceHeader const * ptr) {
    	if(ptr==UA_NULL) return sizeof(UA_SequenceHeader);
    	return 0
	 + sizeof(UA_UInt32) // sequenceNumber
	 + sizeof(UA_UInt32) // requestId
	;
}

UA_Int32 UA_SequenceHeader_encodeBinary(UA_SequenceHeader const * src, UA_ByteString* dst, UA_UInt32 *offset) {
    	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt32_encodeBinary(&src->sequenceNumber,dst,offset);
	retval |= UA_UInt32_encodeBinary(&src->requestId,dst,offset);
	return retval;
}

UA_Int32 UA_SequenceHeader_decodeBinary(UA_ByteString const * src, UA_UInt32 *offset, UA_SequenceHeader * dst) {
    	UA_Int32 retval = UA_SUCCESS;
	UA_SequenceHeader_init(dst);
	CHECKED_DECODE(UA_UInt32_decodeBinary(src,offset,&dst->sequenceNumber), UA_SequenceHeader_deleteMembers(dst));
	CHECKED_DECODE(UA_UInt32_decodeBinary(src,offset,&dst->requestId), UA_SequenceHeader_deleteMembers(dst));
	return retval;
}

UA_Int32 UA_SequenceHeader_delete(UA_SequenceHeader *p) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_SequenceHeader_deleteMembers(p);
	retval |= UA_free(p);
	return retval;
}

UA_Int32 UA_SequenceHeader_deleteMembers(UA_SequenceHeader *p) {
    	UA_Int32 retval = UA_SUCCESS;
	return retval;
}

UA_Int32 UA_SequenceHeader_init(UA_SequenceHeader *p) {
    	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt32_init(&p->sequenceNumber);
	retval |= UA_UInt32_init(&p->requestId);
	return retval;
}

UA_TYPE_NEW_DEFAULT(UA_SequenceHeader)
UA_Int32 UA_SequenceHeader_copy(const UA_SequenceHeader *src,UA_SequenceHeader *dst) {
    	if(src == UA_NULL || dst == UA_NULL) return UA_ERROR;
    	UA_Int32 retval = UA_SUCCESS;
	memcpy(dst, src, sizeof(UA_SequenceHeader));
	return retval;
}

UA_Int32 UA_SecureConversationMessageFooter_calcSizeBinary(UA_SecureConversationMessageFooter const * ptr) {
    	if(ptr==UA_NULL) return sizeof(UA_SecureConversationMessageFooter);
    	return 0
		+ UA_Array_calcSizeBinary(ptr->paddingSize,&UA_.types[UA_BYTE],ptr->padding)
	 + sizeof(UA_Byte) // signature
	;
}

UA_Int32 UA_SecureConversationMessageFooter_encodeBinary(UA_SecureConversationMessageFooter const * src, UA_ByteString* dst, UA_UInt32 *offset) {
    	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_Array_encodeBinary(&src->padding,src->paddingSize,&UA_.types[UA_BYTE],dst,offset);
	retval |= UA_Byte_encodeBinary(&src->signature,dst,offset);
	return retval;
}

UA_Int32 UA_SecureConversationMessageFooter_decodeBinary(UA_ByteString const * src, UA_UInt32 *offset, UA_SecureConversationMessageFooter * dst) {
    	UA_Int32 retval = UA_SUCCESS;
	UA_SecureConversationMessageFooter_init(dst);
	CHECKED_DECODE(UA_Int32_decodeBinary(src,offset,&dst->paddingSize),UA_SecureConversationMessageFooter_deleteMembers(dst));
	CHECKED_DECODE(UA_Array_decodeBinary(src,offset,dst->paddingSize,&UA_.types[UA_BYTE],(void**)&dst->padding), UA_SecureConversationMessageFooter_deleteMembers(dst));
	CHECKED_DECODE(UA_Byte_decodeBinary(src,offset,&dst->signature), UA_SecureConversationMessageFooter_deleteMembers(dst));
	return retval;
}

UA_Int32 UA_SecureConversationMessageFooter_delete(UA_SecureConversationMessageFooter *p) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_SecureConversationMessageFooter_deleteMembers(p);
	retval |= UA_free(p);
	return retval;
}

UA_Int32 UA_SecureConversationMessageFooter_deleteMembers(UA_SecureConversationMessageFooter *p) {
    	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_Array_delete((void*)p->padding,p->paddingSize,&UA_.types[UA_BYTE]);
	return retval;
}

UA_Int32 UA_SecureConversationMessageFooter_init(UA_SecureConversationMessageFooter *p) {
    	UA_Int32 retval = UA_SUCCESS;
	p->paddingSize = 0;
	p->padding = UA_NULL;
	retval |= UA_Byte_init(&p->signature);
	return retval;
}

UA_TYPE_NEW_DEFAULT(UA_SecureConversationMessageFooter)
UA_Int32 UA_SecureConversationMessageFooter_copy(const UA_SecureConversationMessageFooter *src,UA_SecureConversationMessageFooter *dst) {
    	if(src == UA_NULL || dst == UA_NULL) return UA_ERROR;
    	UA_Int32 retval = UA_SUCCESS;
	memcpy(dst, src, sizeof(UA_SecureConversationMessageFooter));

	dst->padding = src->padding;
	retval |= UA_Array_copy(&src->padding, src->paddingSize,&UA_.types[UA_BYTE],(void**)&dst->padding);
	return retval;
}

UA_Int32 UA_SecureConversationMessageAbortBody_calcSizeBinary(UA_SecureConversationMessageAbortBody const * ptr) {
    	if(ptr==UA_NULL) return sizeof(UA_SecureConversationMessageAbortBody);
    	return 0
	 + sizeof(UA_UInt32) // error
	 + UA_String_calcSizeBinary(&ptr->reason)
	;
}

UA_Int32 UA_SecureConversationMessageAbortBody_encodeBinary(UA_SecureConversationMessageAbortBody const * src, UA_ByteString* dst, UA_UInt32 *offset) {
    	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt32_encodeBinary(&src->error,dst,offset);
	retval |= UA_String_encodeBinary(&src->reason,dst,offset);
	return retval;
}

UA_Int32 UA_SecureConversationMessageAbortBody_decodeBinary(UA_ByteString const * src, UA_UInt32 *offset, UA_SecureConversationMessageAbortBody * dst) {
    	UA_Int32 retval = UA_SUCCESS;
	UA_SecureConversationMessageAbortBody_init(dst);
	CHECKED_DECODE(UA_UInt32_decodeBinary(src,offset,&dst->error), UA_SecureConversationMessageAbortBody_deleteMembers(dst));
	CHECKED_DECODE(UA_String_decodeBinary(src,offset,&dst->reason), UA_SecureConversationMessageAbortBody_deleteMembers(dst));
	return retval;
}

UA_Int32 UA_SecureConversationMessageAbortBody_delete(UA_SecureConversationMessageAbortBody *p) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_SecureConversationMessageAbortBody_deleteMembers(p);
	retval |= UA_free(p);
	return retval;
}

UA_Int32 UA_SecureConversationMessageAbortBody_deleteMembers(UA_SecureConversationMessageAbortBody *p) {
    	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_String_deleteMembers(&p->reason);
	return retval;
}

UA_Int32 UA_SecureConversationMessageAbortBody_init(UA_SecureConversationMessageAbortBody *p) {
    	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt32_init(&p->error);
	retval |= UA_String_init(&p->reason);
	return retval;
}

UA_TYPE_NEW_DEFAULT(UA_SecureConversationMessageAbortBody)
UA_Int32 UA_SecureConversationMessageAbortBody_copy(const UA_SecureConversationMessageAbortBody *src,UA_SecureConversationMessageAbortBody *dst) {
    	if(src == UA_NULL || dst == UA_NULL) return UA_ERROR;
    	UA_Int32 retval = UA_SUCCESS;
	memcpy(dst, src, sizeof(UA_SecureConversationMessageAbortBody));
	retval |= UA_String_copy(&src->reason,&dst->reason);
	return retval;
}
