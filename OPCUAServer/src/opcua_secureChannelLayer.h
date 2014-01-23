/*
 * opcua_secureChannelLayer.h
 *
 *  Created on: Dec 19, 2013
 *      Author: opcua
 */

#ifndef OPCUA_SECURECHANNELLAYER_H_
#define OPCUA_SECURECHANNELLAYER_H_

static const Int32 SL_HEADER_LENGTH = 0;
typedef struct _SL_SecurityToken
{
	UInt32 ChannelId;
	UInt32 TokenId;
	UA_DateTime CreatedAt;
	Int32 RevisedLifeTime;
}SL_SecurityToken;
typedef struct _SL_OpenSecureChannelResponse
{
	UInt32 ServerProtocolVersion;
	SL_SecurityToken ChannelSecurityToken;
	UA_String ServerNonce;
}SL_Response;
typedef struct _SL_SecureConversationMessageHeader
{
	UInt32 MessageType;
	Byte   IsFinal;
	UInt32 MessageSize;
	UInt32 SecureChannelId;
}SL_SecureConversationMessageHeader;
typedef struct _SL_AsymmetricAlgorithmSecurityHeader
{
	UA_String SecurityPolicyUri;
	UA_String SenderCertificate;
	UA_String ReceiverThumbprint;
}SL_AsymmetricAlgorithmSecurityHeader;

typedef struct _SL_SequenceHeader
{
	UInt32 SequenceNumber;
	UInt32 RequestId;
}SL_SequenceHeader;

/*
 * optional, only if there is encryption present
 */
typedef struct _SL_AsymmetricAlgorithmSecurityFooter
{
	Byte PaddingSize;
	Byte *Padding;

	UInt32 SignatureSize;
	Byte *Signature;
}SL_AsymmetricAlgorithmSecurityFooter;





#endif /* OPCUA_SECURECHANNELLAYER_H_ */
