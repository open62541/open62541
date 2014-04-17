#include "ua_services.h"
#include "ua_transport_binary_secure.h"

UA_Int32 Service_OpenSecureChannel(SL_Channel *channel, const UA_OpenSecureChannelRequest* request, UA_OpenSecureChannelResponse* response) {
	
	if (request->clientProtocolVersion != channel->tlConnection->remoteConf.protocolVersion) {
		printf("SL_processMessage - error protocol version \n");
		//TODO ERROR_Bad_ProtocolVersionUnsupported
	}

	UA_UInt32 retval = UA_SUCCESS;
	switch (request->requestType) {
	case UA_SECURITYTOKEN_ISSUE:
		if (channel->connectionState == CONNECTIONSTATE_ESTABLISHED) {
			printf("SL_processMessage - multiple security token request");
			//TODO return ERROR
			retval = UA_ERROR;
			break;
		}
		printf("SL_processMessage - TODO: create new token for a new SecureChannel\n");
		//	SL_createNewToken(connection);
		break;
	case UA_SECURITYTOKEN_RENEW:
		if (channel->connectionState == CONNECTIONSTATE_CLOSED) {
			printf("SL_processMessage - renew token request received, but no secureChannel was established before");
			//TODO return ERROR
			retval = UA_ERROR;
			break;
		}
		printf("TODO: create new token for an existing SecureChannel\n");
		break;
	}

	switch (request->securityMode) {
	case UA_SECURITYMODE_INVALID:
		channel->remoteNonce.data = UA_NULL;
		channel->remoteNonce.length = -1;
		printf("SL_processMessage - client demands no security \n");
		break;

	case UA_SECURITYMODE_SIGN:
		printf("SL_processMessage - client demands signed \n");
		//TODO check if senderCertificate and ReceiverCertificateThumbprint are present
		break;

	case UA_SECURITYMODE_SIGNANDENCRYPT:
		printf("SL_processMessage - client demands signed & encrypted \n");
		//TODO check if senderCertificate and ReceiverCertificateThumbprint are present
		break;
	}

	channel->connectionState = CONNECTIONSTATE_ESTABLISHED;

	if (request->requestHeader.returnDiagnostics != 0) {
		printf("SL_openSecureChannel - diagnostics demanded by the client\n");
		printf("SL_openSecureChannel - retrieving diagnostics not implemented!\n");
		//TODO fill with demanded information part 4, 7.8 - Table 123
		response->responseHeader.serviceDiagnostics.encodingMask = 0;
	} else {
		response->responseHeader.serviceDiagnostics.encodingMask = 0;
	}

	response->serverProtocolVersion = channel->tlConnection->localConf.protocolVersion;
	response->securityToken.channelId = channel->securityToken.secureChannelId;
	response->securityToken.tokenId = channel->securityToken.tokenId;
	response->securityToken.revisedLifetime = channel->securityToken.revisedLifetime;
	UA_ByteString_copy(&channel->localNonce, &response->serverNonce);
	return retval;
}

UA_Int32 Service_CloseSecureChannel(SL_Channel *channel, const UA_CloseSecureChannelRequest *request, UA_CloseSecureChannelResponse *response) {
	// 62451 Part 6 Chapter 7.1.4 - The server does not send a CloseSecureChannel response
	channel->connectionState = CONNECTIONSTATE_CLOSE;
	return UA_SUCCESS;
}
