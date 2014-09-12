#include "ua_services.h"
#include "ua_securechannel_manager.h"

UA_Int32 Service_OpenSecureChannel(UA_Server *server, UA_Connection *connection,
                                   const UA_OpenSecureChannelRequest *request,
                                   UA_OpenSecureChannelResponse *response) {
    UA_Int32 retval = UA_SUCCESS;
    // todo: if(request->clientProtocolVersion != protocolVersion)
    if(request->requestType == UA_SECURITYTOKEN_ISSUE)
        retval |= UA_SecureChannelManager_open(server->secureChannelManager, connection, request, response);
    else
        retval |= UA_SecureChannelManager_renew(server->secureChannelManager, connection, request, response);
    return retval;
}

UA_Int32 Service_CloseSecureChannel(UA_Server *server, UA_SecureChannel *channel,
                                    const UA_CloseSecureChannelRequest *request,
                                    UA_CloseSecureChannelResponse *response) {
    return UA_SecureChannelManager_close(server->secureChannelManager, channel->securityToken.channelId);
    // 62451 Part 6 Chapter 7.1.4 - The server does not send a CloseSecureChannel response
}
