/*
 * opcua_secureChannelLayer.h
 *
 *  Created on: Dec 19, 2013
 *      Author: opcua
 */

#ifndef OPCUA_SECURECHANNELLAYER_H_
#define OPCUA_SECURECHANNELLAYER_H_

#include "opcua_advancedDatatypes.h"
#include "opcua_encodingLayer.h"
#include "opcua_connectionHelper.h"

static const Int32 SL_HEADER_LENGTH = 0;



typedef struct _SL_OpenSecureChannelResponse
{
	UInt32 ServerProtocolVersion;
	SL_ChannelSecurityToken SecurityToken;
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

/*
typedef struct _SL_ResponseHeader
{
	UA_DateTime timestamp;
    IntegerId requestHandle;
    UA_StatusCode serviceResult;
    UA_DiagnosticInfo serviceDiagnostics;
    UA_String *stringTable;
    UInt32 stringTableLength;
    UA_ExtensionObject additionalHeader;
}SL_ResponseHeader;
*/

#endif /* OPCUA_SECURECHANNELLAYER_H_ */
