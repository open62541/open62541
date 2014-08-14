/*
 * ua_stack_channel.c
 *
 *  Created on: 09.05.2014
 *      Author: root
 */

#include "ua_stack_channel.h"
#include <time.h>
#include <stdlib.h>

struct SL_Channel {
	SL_channelState state;
	UA_UInt32 channelId;
	//TL_Connection* tlConnection;

	UA_TL_Connection *connection;
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
};

UA_Int32 SL_Channel_setRemoteSecuritySettings(SL_Channel *channel,
		UA_AsymmetricAlgorithmSecurityHeader *asymSecHeader,
		UA_SequenceHeader *sequenceHeader) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_AsymmetricAlgorithmSecurityHeader_copy(asymSecHeader,
			&channel->remoteAsymAlgSettings);
	//set starting sequence number from remote partner
	channel->sequenceNumber =
			sequenceHeader->sequenceNumber;
	//set starting request id from remote partner
	channel->requestId = sequenceHeader->requestId;
	return retval;
}
/*
UA_Int32 SL_Channel_initLocalSecuritySettings(SL_Channel *channel)
{
	UA_Int32 retval = UA_SUCCESS;
	channel->localAsymAlgSettings.receiverCertificateThumbprint.data = UA_NULL;
	channel->localAsymAlgSettings.receiverCertificateThumbprint.length = 0;

	retval |= UA_String_copycstring("http://opcfoundation.org/UA/SecurityPolicy#None",(UA_String*)&channel->localAsymAlgSettings.securityPolicyUri);

	channel->localAsymAlgSettings.senderCertificate.data = UA_NULL;
	channel->localAsymAlgSettings.senderCertificate.length = 0;
	return retval;
}
*/
UA_Int32 SL_Channel_new(SL_Channel **channel) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_alloc((void** )channel, sizeof(SL_Channel));
	SL_Channel *thisChannel = UA_NULL;
	retval |= UA_alloc((void** )&thisChannel, sizeof(SL_Channel));
	*channel = thisChannel;
	return retval;
}

//TODO implement real nonce generator - DUMMY function
UA_Int32 SL_Channel_generateNonce(UA_ByteString *nonce) {
	//UA_ByteString_new(&nonce);
	UA_alloc((void** )&(nonce->data), 1);
	nonce->length = 1;
	nonce->data[0] = 'a';
	return UA_SUCCESS;
}

UA_Int32 SL_Channel_init(SL_Channel *channel, UA_TL_Connection *connection,
		SL_ChannelIdProvider channelIdProvider,
		SL_ChannelSecurityTokenProvider tokenProvider) {

	UA_Int32 retval = UA_SUCCESS;

	channel->channelIdProvider = channelIdProvider;
	channel->tokenProvider = tokenProvider;

	channel->connection = connection;
	//generate secure channel id
	channel->channelIdProvider(
			&channel->channelId);

	//generate local nonce
	SL_Channel_generateNonce(&channel->localNonce);
	//TODO get this from the local configuration file MOCK UP ---start
	channel->localAsymAlgSettings.receiverCertificateThumbprint.data = UA_NULL;
	channel->localAsymAlgSettings.receiverCertificateThumbprint.length = 0;

	retval |= UA_String_copycstring("http://opcfoundation.org/UA/SecurityPolicy#None",(UA_String*)&channel->localAsymAlgSettings.securityPolicyUri);

	channel->localAsymAlgSettings.senderCertificate.data = UA_NULL;
	channel->localAsymAlgSettings.senderCertificate.length = 0;
	// MOCK UP ---end


	channel->state = UA_SL_CHANNEL_CLOSED;
	return retval;
}

UA_Int32 SL_Channel_registerTokenProvider(SL_Channel *channel,
		SL_ChannelSecurityTokenProvider provider) {
	channel->tokenProvider = provider;
	return UA_SUCCESS;
}

UA_Int32 SL_Channel_getRemainingLifetime(SL_Channel *channel, UA_Int32 *lifetime) {
	if (channel) {
		UA_Int64 diffInMS =
				(channel->securityToken.createdAt
						- UA_DateTime_now()) / 1e7;
		if (diffInMS > UA_INT32_MAX) {
			*lifetime = UA_INT32_MAX;
		} else {
			*lifetime = (UA_Int32) diffInMS;
		}
		return UA_SUCCESS;
	} else {
		printf(
				"SL_Channel_getRemainingLifetime - no valid channel object, null pointer");
		return UA_ERROR;
	}
}

UA_Int32 SL_Channel_getChannelId(SL_Channel *channel, UA_UInt32 *channelId) {
	if (channel) {
		*channelId = channel->channelId;
		return UA_SUCCESS;
	}
	return UA_ERROR;
}

UA_Int32 SL_Channel_getTokenId(SL_Channel *channel, UA_UInt32 *tokenId) {
	if (channel) {
		*tokenId = channel->securityToken.tokenId;
		return UA_SUCCESS;
	}
	return UA_ERROR;
}

UA_Int32 SL_Channel_getLocalAsymAlgSettings(SL_Channel *channel,
		UA_AsymmetricAlgorithmSecurityHeader **asymAlgSettings) {
	UA_Int32 retval = 0;

	retval |= UA_alloc((void** )asymAlgSettings,
			UA_AsymmetricAlgorithmSecurityHeader_calcSizeBinary(UA_NULL));

	retval |=
			UA_ByteString_copy(
					&(channel->localAsymAlgSettings.receiverCertificateThumbprint),
					&(*asymAlgSettings)->receiverCertificateThumbprint);
	retval |=
			UA_ByteString_copy(
					&(channel->localAsymAlgSettings.securityPolicyUri),
					&(*asymAlgSettings)->securityPolicyUri);
	retval |=
			UA_ByteString_copy(
					&(channel->localAsymAlgSettings.senderCertificate),
					&(*asymAlgSettings)->senderCertificate);

	return UA_SUCCESS;
}

UA_Int32 SL_Channel_getConnection(SL_Channel *channel,
		UA_TL_Connection **connection) {
	if (channel) {
		*connection = channel->connection;
		return UA_SUCCESS;
	}
	return UA_ERROR;
}
UA_Int32 SL_Channel_getRequestId(SL_Channel *channel, UA_UInt32 *requestId) {
	if (channel) {
		*requestId = channel->requestId;
		return UA_SUCCESS;
	}
	return UA_ERROR;
}
UA_Int32 SL_Channel_getSequenceNumber(SL_Channel *channel,
		UA_UInt32 *sequenceNumber) {
	if (channel) {
		*sequenceNumber = channel->sequenceNumber;
		return UA_SUCCESS;
	}
	return UA_ERROR;
}
UA_Int32 SL_Channel_getState(SL_Channel *channel, SL_channelState *channelState) {
	if (channel) {
		*channelState = channel->state;
		return UA_SUCCESS;
	}
	return UA_ERROR;
}

UA_Int32 SL_Channel_getRevisedLifetime(SL_Channel *channel,
		UA_UInt32 *revisedLifetime) {
	if (channel) {
		*revisedLifetime =
				channel->securityToken.revisedLifetime;
		return UA_SUCCESS;
	}
	return UA_ERROR;
}

//setters
UA_Int32 SL_Channel_setId(SL_Channel *channel, UA_UInt32 id) {
	channel->channelId = id;
	return UA_SUCCESS;
}

//private function
UA_Int32 SL_Channel_setState(SL_Channel *channel, SL_channelState channelState) {
	channel->state = channelState;
	return UA_SUCCESS;
}



UA_Boolean SL_Channel_compare(SL_Channel *channel1, SL_Channel *channel2) {
	return (((SL_Channel*) channel1)->channelId
			== ((SL_Channel*) channel2)->channelId) ?
	UA_TRUE :
															UA_FALSE;
}

UA_Int32 SL_Channel_bind(SL_Channel *channel, UA_TL_Connection *connection) {
	if (channel && connection) {
		channel->connection = connection;
		return UA_SUCCESS;
	}
	return UA_ERROR;
}

UA_Int32 SL_Channel_deleteMembers(SL_Channel *channel) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(
			&channel->localAsymAlgSettings);
	retval |= UA_ByteString_deleteMembers(
			&channel->localNonce);
	retval |= UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(
			&channel->remoteAsymAlgSettings);
	retval |= UA_ByteString_deleteMembers(
			&channel->remoteNonce);
	retval |= UA_ChannelSecurityToken_deleteMembers(
			&channel->securityToken);
	return retval;
}
UA_Int32 SL_Channel_delete(SL_Channel *channel) {
	UA_Int32 retval = UA_SUCCESS;
	retval = SL_Channel_deleteMembers(channel);
	retval = UA_free(channel);
	return retval;
}

UA_Int32 SL_Channel_processTokenRequest(SL_Channel *channel,
		UA_UInt32 requestedLifetime, UA_SecurityTokenRequestType requestType) {
	if (channel->tokenProvider) {
		return channel->tokenProvider(channel,
				requestedLifetime, requestType,
				&channel->securityToken);

	}
	printf("SL_Channel_processTokenRequest - no Token provider registered");
	return UA_ERROR;
}
UA_Int32 SL_Channel_renewToken(SL_Channel *channel, UA_UInt32 tokenId,
		UA_DateTime revisedLifetime, UA_DateTime createdAt) {
	channel->securityToken.tokenId = tokenId;
	channel->securityToken.createdAt = createdAt;
	channel->securityToken.revisedLifetime =
			revisedLifetime;
	return UA_SUCCESS;
}

UA_Int32 SL_Channel_checkSequenceNumber(SL_Channel *channel,
		UA_UInt32 sequenceNumber) {
	//TODO review checking of sequence
	if (channel->sequenceNumber+1  == sequenceNumber) {
		channel->sequenceNumber++;

		return UA_SUCCESS;
	}
	printf(
			"SL_Channel_checkSequenceNumber - ERROR, wrong SequenceNumber expected: %i, received: %i",
			channel->sequenceNumber + 1, sequenceNumber);
	return UA_ERROR;

}

UA_Int32 SL_Channel_checkRequestId(SL_Channel *channel, UA_UInt32 requestId) {
	//TODO review checking of request id
	if (channel->requestId+1  == requestId) {
		channel->requestId++;

		return UA_SUCCESS;
	}
	printf(
			"SL_Channel_requestId - ERROR, wrong requestId expected: %i, received: %i",
			channel->requestId + 1, requestId);
	return UA_ERROR;

}

UA_Int32 SL_Channel_processOpenRequest(SL_Channel *channel,
		const UA_OpenSecureChannelRequest* request,
		UA_OpenSecureChannelResponse* response) {
	UA_UInt32 protocolVersion;
	SL_Channel* thisChannel = channel;
	UA_Int32 retval = UA_SUCCESS;

	UA_TL_Connection_getProtocolVersion(thisChannel->connection,
			&protocolVersion);


	if (request->clientProtocolVersion != protocolVersion) {
		printf("SL_Channel_processOpenRequest - error protocol version \n");
		//TODO ERROR_Bad_ProtocolVersionUnsupported
	}

	switch (request->requestType) {
	case UA_SECURITYTOKEN_ISSUE:
		if (thisChannel->state == UA_SL_CHANNEL_OPEN) {
			printf(
					"SL_Channel_processOpenRequest - multiple security token request");
			//TODO return ERROR
			retval = UA_ERROR;
			break;
		}
		printf(
				"SL_Channel_processOpenRequest - creating new token for a new SecureChannel\n");
		SL_Channel_processTokenRequest(channel, request->requestedLifetime,
				request->requestType);
		//	SL_createNewToken(connection);
		break;
	case UA_SECURITYTOKEN_RENEW:
		if (thisChannel->state == UA_SL_CHANNEL_CLOSED) {
			printf(
					"SL_Channel_processOpenRequest - renew token request received, but no secureChannel was established before");
			//TODO return ERROR
			retval = UA_ERROR;
			break;
		} else {
			//generate new SecurityToken
			retval = SL_Channel_processTokenRequest(channel,
					request->requestedLifetime, request->requestType);
			if (retval != UA_SUCCESS) {
				printf(
						"SL_Channel_processOpenRequest - cannot create new token for an existing SecureChannel\n");

			} else {
				printf(
						"SL_Channel_processOpenRequest - creating new token for an existing SecureChannel\n");
			}

			break;
		}
	}
	switch (request->securityMode) {
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
		printf(
				"SL_Channel_processOpenRequest - client demands signed & encrypted \n");
		//TODO check if senderCertificate and ReceiverCertificateThumbprint are present
		break;
	}
	UA_ByteString_copy(&request->clientNonce, &thisChannel->remoteNonce);
	thisChannel->state = UA_SL_CHANNEL_OPEN;

	if (request->requestHeader.returnDiagnostics != 0) {
		printf("SL_openSecureChannel - diagnostics demanded by the client\n");
		printf(
				"SL_openSecureChannel - retrieving diagnostics not implemented!\n");
		//TODO fill with demanded information part 4, 7.8 - Table 123
		response->responseHeader.serviceDiagnostics.encodingMask = 0;
	} else {
		response->responseHeader.serviceDiagnostics.encodingMask = 0;
	}

	response->serverProtocolVersion = protocolVersion;

	UA_ChannelSecurityToken_copy(&channel->securityToken,
			&(response->securityToken));

	UA_ByteString_copy(&thisChannel->localNonce, &response->serverNonce);

	return retval;
}

UA_Int32 SL_Channel_processCloseRequest(SL_Channel *channel,
		const UA_CloseSecureChannelRequest* request) {
	SL_Channel_setState(channel, UA_SL_CHANNEL_CLOSED);
	return UA_SUCCESS;
}

