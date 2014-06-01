/*
 * ua_stack_channel.c
 *
 *  Created on: 09.05.2014
 *      Author: root
 */

#include "ua_stack_channel.h"


typedef struct SL_Channel1 {
	SL_channelState state;
	UA_UInt32 channelId;
	//TL_Connection* tlConnection;

	UA_TL_Connection1 connection;
	UA_UInt32 requestId;
	//UA_UInt32 lastRequestId;

	UA_UInt32 sequenceNumber;
	//UA_UInt32 lastSequenceNumber;

	UA_AsymmetricAlgorithmSecurityHeader remoteAsymAlgSettings;
	UA_AsymmetricAlgorithmSecurityHeader localAsymAlgSettings;


	UA_ChannelSecurityToken securityToken;

	UA_MessageSecurityMode securityMode;
	UA_ByteString remoteNonce;
	UA_ByteString localNonce;
	SL_ChannelSecurityTokenProvider tokenProvider;
	SL_ChannelIdProvider channelIdProvider;
}SL_Channel1;

UA_Int32 SL_Channel_registerTokenProvider(SL_secureChannel channel, SL_ChannelSecurityTokenProvider provider)
{
	((SL_Channel1*)channel)->tokenProvider = provider;
	return UA_SUCCESS;
}

UA_Int32 SL_Channel_getRemainingLifetime(SL_secureChannel channel, UA_Int32 *lifetime)
{
	if(channel)
	{
		*lifetime = UA_DateTime_difference_ms(((SL_Channel1*)channel)->securityToken.createdAt,UA_DateTime_now());
	return UA_SUCCESS;
	}
	else
	{
		printf("SL_Channel_getRemainingLifetime - no valid channel object, null pointer");
		return UA_ERROR;
	}
}

UA_Int32 SL_Channel_getChannelId(SL_secureChannel channel, UA_UInt32 *channelId)
{
	if(channel)
	{
		*channelId = ((SL_Channel1*)channel)->channelId;
		return UA_SUCCESS;
	}
	return UA_ERROR;
}

UA_Int32 SL_Channel_getTokenId(SL_secureChannel channel, UA_UInt32 *tokenId)
{
	if(channel)
	{
		*tokenId = ((SL_Channel1*)channel)->securityToken.tokenId;
		return UA_SUCCESS;
	}
	return UA_ERROR;
}

UA_Int32 SL_Channel_getLocalAsymAlgSettings(SL_secureChannel channel, UA_AsymmetricAlgorithmSecurityHeader **asymAlgSettings)
{
	UA_Int32 retval = 0;

	retval |= UA_alloc((void**)asymAlgSettings,UA_AsymmetricAlgorithmSecurityHeader_calcSize(UA_NULL));

	retval |= UA_ByteString_copy(&(channel->localAsymAlgSettings.receiverCertificateThumbprint),
			&(*asymAlgSettings)->receiverCertificateThumbprint);
	retval |= UA_ByteString_copy(&(channel->localAsymAlgSettings.securityPolicyUri),
				&(*asymAlgSettings)->securityPolicyUri);
	retval |= UA_ByteString_copy(&(channel->localAsymAlgSettings.senderCertificate),
					&(*asymAlgSettings)->senderCertificate);

	return UA_SUCCESS;
}
UA_Int32 SL_Channel_getConnection(SL_secureChannel channel, UA_TL_Connection1 *connection)
{
	if(channel)
	{
		*connection = ((SL_Channel1*)channel)->connection;
		return UA_SUCCESS;
	}
	return UA_ERROR;
}
UA_Int32 SL_Channel_getRequestId(SL_secureChannel channel, UA_UInt32 *requestId)
{
	if(channel)
	{
		*requestId = ((SL_Channel1*)channel)->requestId;
		return UA_SUCCESS;
	}
	return UA_ERROR;
}
UA_Int32 SL_Channel_getSequenceNumber(SL_secureChannel channel, UA_UInt32 *sequenceNumber)
{
	if(channel)
	{
		*sequenceNumber = ((SL_Channel1*)channel)->sequenceNumber;
		return UA_SUCCESS;
	}
	return UA_ERROR;
}
UA_Int32 SL_Channel_getState(SL_secureChannel channel,SL_channelState *channelState)
{
	if(channel)
	{
		*channelState = ((SL_Channel1*)channel)->state;
		return UA_SUCCESS;
	}
	return UA_ERROR;
}

UA_Int32 SL_Channel_getRevisedLifetime(SL_secureChannel channel, UA_UInt32 *revisedLifetime)
{
	if(channel)
	{
		*revisedLifetime = ((SL_Channel1*)channel)->securityToken.revisedLifetime;
		return UA_SUCCESS;
	}
	return UA_ERROR;
}

//setters
UA_Int32 SL_Channel_setId(SL_secureChannel channel, UA_UInt32 id)
{
	((SL_Channel1*)channel)->channelId = id;
	return UA_SUCCESS;
}

//private function
UA_Int32 SL_Channel_setState(SL_secureChannel channel,SL_channelState channelState)
{
	((SL_Channel1*)channel)->state = channelState;
	return UA_SUCCESS;
}

UA_Int32 SL_Channel_init(SL_secureChannel channel,
		UA_ByteString *receiverCertificateThumbprint,
		UA_ByteString *securityPolicyUri,
		UA_ByteString *senderCertificate,
		UA_MessageSecurityMode securityMode)
{
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_ByteString_copy(receiverCertificateThumbprint,
			&((SL_Channel1*)channel)->localAsymAlgSettings.receiverCertificateThumbprint);

	retval |= UA_ByteString_copy(securityPolicyUri,
			&((SL_Channel1*)channel)->localAsymAlgSettings.securityPolicyUri);

	retval |= UA_ByteString_copy(senderCertificate,
			&((SL_Channel1*)channel)->localAsymAlgSettings.senderCertificate);



	((SL_Channel1*)channel)->state = UA_SL_CHANNEL_CLOSED;
	return retval;
}
//TODO implement real nonce generator - DUMMY function
UA_Int32 SL_Channel_generateNonce(UA_ByteString *nonce)
{
	UA_ByteString_new(&nonce);
	UA_alloc((void**)&(nonce->data),1);
	nonce->length = 1;
	nonce->data[0] = 'a';
	return UA_SUCCESS;
}

_Bool SL_Channel_equal(void* channel1, void* channel2)
{
	return (((SL_Channel1*)channel1)->channelId == ((SL_Channel1*)channel2)->channelId);
}

UA_Int32 SL_Channel_new(SL_secureChannel **channel,
		SL_ChannelIdProvider channelIdProvider,
		SL_ChannelSecurityTokenProvider tokenProvider,
		UA_ByteString *receiverCertificateThumbprint,
		UA_ByteString *securityPolicyUri,
		UA_ByteString *senderCertificate,
		UA_MessageSecurityMode securityMode)

{
	UA_Int32 retval = UA_SUCCESS;

	retval |= UA_alloc((void**)channel,sizeof(SL_secureChannel));

	SL_Channel1 *thisChannel = UA_NULL;
	retval |= UA_alloc((void**)&thisChannel,sizeof(SL_Channel1));

	thisChannel->channelIdProvider = channelIdProvider;
	thisChannel->tokenProvider = tokenProvider;

	retval |= UA_ByteString_copy(receiverCertificateThumbprint,
			&thisChannel->localAsymAlgSettings.receiverCertificateThumbprint);

	retval |= UA_ByteString_copy(securityPolicyUri,
			&thisChannel->localAsymAlgSettings.securityPolicyUri);

	retval |= UA_ByteString_copy(senderCertificate,
			&thisChannel->localAsymAlgSettings.senderCertificate);



	thisChannel->state = UA_SL_CHANNEL_CLOSED;

	**channel = (SL_secureChannel)thisChannel;

	return UA_SUCCESS;
}
UA_Int32 SL_Channel_initByRequest(SL_secureChannel channel,
		UA_TL_Connection1 connection,
		const UA_ByteString* msg,
		UA_Int32* pos)
{

	UA_SequenceHeader *sequenceHeader;

	UA_SequenceHeader_new(&sequenceHeader);
	UA_SequenceHeader_init(sequenceHeader);


	((SL_Channel1*)channel)->channelIdProvider(&((SL_Channel1*)channel)->channelId);

	((SL_Channel1*)channel)->connection = connection;

//	channel->createdAt = UA_DateTime_now();

//	channel->localAsymAlgSettings.receiverCertificateThumbprint


	SL_Channel_generateNonce(&channel->localNonce);



	*pos += 4; // skip the securechannelid
	UA_AsymmetricAlgorithmSecurityHeader_decodeBinary(msg, pos,
			&channel->remoteAsymAlgSettings);

	UA_SequenceHeader_decodeBinary(msg, pos,
			sequenceHeader);


	//init last requestId and sequenceNumber

	//channel->lastRequestId = sequenceHeader->requestId;
	//channel->lastSequenceNumber = sequenceHeader->sequenceNumber;

	channel->requestId = sequenceHeader->requestId;//channel->lastRequestId;
	channel->sequenceNumber = sequenceHeader->sequenceNumber;//channel->lastSequenceNumber;

	channel->state = UA_SL_CHANNEL_CLOSED;

	return UA_SUCCESS;
}

UA_Int32 SL_Channel_delete(SL_secureChannel channel)
{
	//TODO implement me!
	return UA_SUCCESS;
}

UA_Int32 SL_Channel_deleteMembers(SL_secureChannel channel)
{
	//TODO implement me!
	return UA_SUCCESS;
}
UA_Int32 SL_Channel_processTokenRequest(SL_secureChannel channel,UA_UInt32 requestedLifetime, UA_SecurityTokenRequestType requestType)
{
	if(((SL_Channel1*)channel)->tokenProvider)
	{
		return ((SL_Channel1*)channel)->tokenProvider(channel,requestedLifetime,requestType, &((SL_Channel1*)channel)->securityToken);
	}
		printf("SL_Channel_processTokenRequest - no Token provider registered");
		return UA_ERROR;
}
UA_Int32 SL_Channel_renewToken(SL_secureChannel channel, UA_UInt32 tokenId,UA_DateTime revisedLifetime, UA_DateTime createdAt)
{
	((SL_Channel1*)channel)->securityToken.tokenId = tokenId;
	((SL_Channel1*)channel)->securityToken.createdAt = createdAt;
	((SL_Channel1*)channel)->securityToken.revisedLifetime = revisedLifetime;
	return UA_SUCCESS;
}


UA_Int32 SL_Channel_checkSequenceNumber(SL_secureChannel channel, UA_UInt32 sequenceNumber)
{
	if(((SL_Channel1*)channel)->sequenceNumber+1 == sequenceNumber){
		((SL_Channel1*)channel)->sequenceNumber++;

		return UA_SUCCESS;
	}
	printf("SL_Channel_checkSequenceNumber - ERROR, wrong SequenceNumber expected: %i, received: %i",
			((SL_Channel1*)channel)->sequenceNumber+1,sequenceNumber);
	return UA_ERROR;

}

UA_Int32 SL_Channel_checkRequestId(SL_secureChannel channel, UA_UInt32 requestId)
{
	if(((SL_Channel1*)channel)->requestId+1 == requestId){
		((SL_Channel1*)channel)->requestId++;

		return UA_SUCCESS;
	}
	printf("SL_Channel_requestId - ERROR, wrong requestId expected: %i, received: %i",
			((SL_Channel1*)channel)->requestId+1,requestId);
	return UA_ERROR;

}

UA_Int32 SL_Channel_processOpenRequest(SL_secureChannel channel,
		const UA_OpenSecureChannelRequest* request, UA_OpenSecureChannelResponse* response)
{
	UA_UInt32 protocolVersion;
	SL_Channel1 *thisChannel = (SL_Channel1*)channel;
	UA_Int32 retval = UA_SUCCESS;

	UA_TL_Connection_getProtocolVersion(thisChannel->connection, &protocolVersion);

	if (request->clientProtocolVersion
			!= protocolVersion)
	{
		printf("SL_Channel_processOpenRequest - error protocol version \n");
		//TODO ERROR_Bad_ProtocolVersionUnsupported
	}

	switch (request->requestType)
	{
	case UA_SECURITYTOKEN_ISSUE:
		if (thisChannel->state == UA_SL_CHANNEL_OPEN)
		{
			printf("SL_Channel_processOpenRequest - multiple security token request");
			//TODO return ERROR
			retval = UA_ERROR;
			break;
		}
		printf(
				"SL_Channel_processOpenRequest - creating new token for a new SecureChannel\n");


		SL_Channel_processTokenRequest(channel,request->requestedLifetime, request->requestType);
		//	SL_createNewToken(connection);
		break;
	case UA_SECURITYTOKEN_RENEW:
		if (thisChannel->state == UA_SL_CHANNEL_CLOSED)
		{
			printf(
					"SL_Channel_processOpenRequest - renew token request received, but no secureChannel was established before");
			//TODO return ERROR
			retval = UA_ERROR;
			break;
		}
		else
		{
			//generate new SecurityToken
			retval = SL_Channel_processTokenRequest(channel,request->requestedLifetime, request->requestType);
			if (retval != UA_SUCCESS)
			{
				printf(
						"SL_Channel_processOpenRequest - creating new token for an existing SecureChannel\n");
			}
			else
			{
				printf(
						"SL_Channel_processOpenRequest - cannot create new token for an existing SecureChannel\n");
			}

			break;
		}
	}
	switch (request->securityMode)
	{
	case UA_SECURITYMODE_INVALID:
		thisChannel->remoteNonce.data = UA_NULL;
		thisChannel->remoteNonce.length = -1;
		printf("SL_Channel_processOpenRequest - client demands no security \n");
		break;

	case UA_SECURITYMODE_SIGN:
		printf("SL_Channel_processOpenRequest - client demands signed \n");
		//TODO check if senderCertificate and ReceiverCertificateThumbprint are present
		break;

	case UA_SECURITYMODE_SIGNANDENCRYPT:
		printf("SL_Channel_processOpenRequest - client demands signed & encrypted \n");
		//TODO check if senderCertificate and ReceiverCertificateThumbprint are present
		break;
	}

	thisChannel->state = CONNECTIONSTATE_ESTABLISHED;

	if (request->requestHeader.returnDiagnostics != 0)
	{
		printf("SL_openSecureChannel - diagnostics demanded by the client\n");
		printf(
				"SL_openSecureChannel - retrieving diagnostics not implemented!\n");
		//TODO fill with demanded information part 4, 7.8 - Table 123
		response->responseHeader.serviceDiagnostics.encodingMask = 0;
	}
	else
	{
		response->responseHeader.serviceDiagnostics.encodingMask = 0;
	}

	response->serverProtocolVersion = protocolVersion;

	UA_ChannelSecurityToken_copy(&((SL_Channel1*)(channel))->securityToken, &(response->securityToken));

	UA_ByteString_copy(&thisChannel->localNonce, &response->serverNonce);


	return retval;
}

UA_Int32 SL_Channel_processCloseRequest(SL_secureChannel channel,
		const UA_CloseSecureChannelRequest* request)
{
	SL_Channel_setState(channel,UA_SL_CHANNEL_CLOSED);
	return UA_SUCCESS;
}


