#include "ua_server_internal.h"
#include "ua_services.h"
#include "ua_securechannel_manager.h"

void Service_OpenSecureChannel(UA_Server *server, UA_Connection *connection,
                               const UA_OpenSecureChannelRequest *request,
                               UA_OpenSecureChannelResponse *response) {
    // todo: if(request->clientProtocolVersion != protocolVersion)
    if(request->requestType == UA_SECURITYTOKENREQUESTTYPE_ISSUE) {
        response->responseHeader.serviceResult =
            UA_SecureChannelManager_open(&server->secureChannelManager, connection, request, response);

        if(response->responseHeader.serviceResult == UA_STATUSCODE_GOOD)
            UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                         "Connection %i | SecureChannel %i | OpenSecureChannel: Opened SecureChannel",
                         connection->sockfd, response->securityToken.channelId);
        else
            UA_LOG_DEBUG(server->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                         "Connection %i | OpenSecureChannel: Opening a SecureChannel failed",
                         connection->sockfd);
    } else {
        response->responseHeader.serviceResult =
            UA_SecureChannelManager_renew(&server->secureChannelManager, connection, request, response);

        if(response->responseHeader.serviceResult == UA_STATUSCODE_GOOD)
            UA_LOG_DEBUG(server->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                         "Connection %i | SecureChannel %i | OpenSecureChannel: SecureChannel renewed",
                         connection->sockfd, response->securityToken.channelId);
        else
            UA_LOG_DEBUG(server->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                         "Connection %i | OpenSecureChannel: Renewing SecureChannel failed",
                         connection->sockfd);
    }
}

/* The server does not send a CloseSecureChannel response */
void Service_CloseSecureChannel(UA_Server *server, UA_SecureChannel *channel) {
    UA_LOG_INFO_CHANNEL(server->config.logger, channel, "CloseSecureChannel");
    UA_SecureChannelManager_close(&server->secureChannelManager, channel->securityToken.channelId);
}
