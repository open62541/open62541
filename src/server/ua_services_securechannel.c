/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_server_internal.h"
#include "ua_services.h"
#include "ua_securechannel_manager.h"

void
Service_OpenSecureChannel(UA_Server *server, UA_SecureChannel *channel,
                          const UA_OpenSecureChannelRequest *request,
                          UA_OpenSecureChannelResponse *response) {
    if(request->requestType == UA_SECURITYTOKENREQUESTTYPE_RENEW) {
        /* Renew the channel */
        response->responseHeader.serviceResult =
            UA_SecureChannelManager_renew(&server->secureChannelManager,
                                          channel, request, response);

        /* Logging */
        if(response->responseHeader.serviceResult == UA_STATUSCODE_GOOD) {
            UA_LOG_DEBUG_CHANNEL(server->config.logger, channel,
                                 "SecureChannel renewed");
        } else {
            UA_LOG_DEBUG_CHANNEL(server->config.logger, channel,
                                 "Renewing SecureChannel failed");
        }
        return;
    }

    /* Must be ISSUE or RENEW */
    if(request->requestType != UA_SECURITYTOKENREQUESTTYPE_ISSUE) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADINTERNALERROR;
        return;
    }

    /* Open the channel */
    response->responseHeader.serviceResult =
        UA_SecureChannelManager_open(&server->secureChannelManager, channel,
                                     request, response);

    /* Logging */
    if(response->responseHeader.serviceResult == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_CHANNEL(server->config.logger, channel,
                            "Opened SecureChannel");
    } else {
        UA_LOG_INFO_CHANNEL(server->config.logger, channel,
                            "Opening a SecureChannel failed");
    }
}

/* The server does not send a CloseSecureChannel response */
void
Service_CloseSecureChannel(UA_Server *server, UA_SecureChannel *channel) {
    UA_LOG_INFO_CHANNEL(server->config.logger, channel, "CloseSecureChannel");
    UA_SecureChannelManager_close(&server->secureChannelManager,
                                  channel->securityToken.channelId);
}
