#include "ua_services.h"
#include "ua_securechannel_manager.h"

void Service_OpenSecureChannel(UA_Server *server, UA_Connection *connection,
                               const UA_OpenSecureChannelRequest *request,
                               UA_OpenSecureChannelResponse *response) {
    // todo: if(request->clientProtocolVersion != protocolVersion)
    if(request->requestType == UA_SECURITYTOKEN_ISSUE)
        UA_SecureChannelManager_open(server->secureChannelManager, connection, request, response);
    else
        UA_SecureChannelManager_renew(server->secureChannelManager, connection, request, response);
}

void Service_CloseSecureChannel(UA_Server *server, UA_Int32 channelId) {
	//Sten: this service is a bit assymmetric to OpenSecureChannel since CloseSecureChannelRequest does not contain any indormation
    UA_SecureChannelManager_close(server->secureChannelManager, channelId);
    // 62451 Part 6 Chapter 7.1.4 - The server does not send a CloseSecureChannel response
}
