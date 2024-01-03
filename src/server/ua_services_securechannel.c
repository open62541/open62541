/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014, 2017 (c) Florian Palm
 *    Copyright 2015 (c) Oleksiy Vasylyev
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2023 (c) Hilscher Gesellschaft für Systemautomation mbH (Author: Phuong Nguyen)
 */

#include "ua_server_internal.h"
#include "ua_services.h"

/* This contains the SecureChannel Services to be called after validation and
 * decoding of the message. The main SecureChannel logic is handled in
 * /src/ua_securechannel.* and /src/server/ua_server_binary.c. */

void
Service_OpenSecureChannel(UA_Server *server, UA_SecureChannel *channel,
                          UA_OpenSecureChannelRequest *request,
                          UA_OpenSecureChannelResponse *response) {
    const UA_SecurityPolicy *sp = channel->securityPolicy;

    switch(request->requestType) {
    /* Open the channel */
    case UA_SECURITYTOKENREQUESTTYPE_ISSUE:
        /* We must expect an OPN handshake */
        if(channel->state != UA_SECURECHANNELSTATE_ACK_SENT) {
            UA_LOG_ERROR_CHANNEL(server->config.logging, channel,
                                 "Called open on already open or closed channel");
            response->responseHeader.serviceResult = UA_STATUSCODE_BADINTERNALERROR;
            goto error;
        }

        /* Set the SecurityMode */
        if(request->securityMode != UA_MESSAGESECURITYMODE_NONE &&
           UA_ByteString_equal(&sp->policyUri, &UA_SECURITY_POLICY_NONE_URI)) {
            response->responseHeader.serviceResult = UA_STATUSCODE_BADSECURITYMODEREJECTED;
            goto error;
        }
        channel->securityMode = request->securityMode;
        break;

    /* Renew the channel */
    case UA_SECURITYTOKENREQUESTTYPE_RENEW:
        /* The channel must be open to be renewed */
        if(channel->state != UA_SECURECHANNELSTATE_OPEN) {
            UA_LOG_ERROR_CHANNEL(server->config.logging, channel,
                                 "Called renew on channel which is not open");
            response->responseHeader.serviceResult = UA_STATUSCODE_BADINTERNALERROR;
            goto error;
        }

        /* Check whether the nonce was reused */
        if(channel->securityMode != UA_MESSAGESECURITYMODE_NONE &&
           UA_ByteString_equal(&channel->remoteNonce, &request->clientNonce)) {
            UA_LOG_ERROR_CHANNEL(server->config.logging, channel,
                                 "The client reused the last nonce");
            response->responseHeader.serviceResult = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
            goto error;
        }

        break;

    /* Unknown request type */
    default:
        response->responseHeader.serviceResult = UA_STATUSCODE_BADINTERNALERROR;
        return;
    }

    /* Create a new SecurityToken. It will be switched over when the first
     * message is received. The ChannelId is left unchanged. */
    UA_EventLoop *el = server->config.eventLoop;
    channel->altSecurityToken.channelId = channel->securityToken.channelId;
    channel->altSecurityToken.tokenId = generateSecureChannelTokenId(server);
    channel->altSecurityToken.createdAt = el->dateTime_nowMonotonic(el);
    channel->altSecurityToken.revisedLifetime =
        (request->requestedLifetime > server->config.maxSecurityTokenLifetime) ?
        server->config.maxSecurityTokenLifetime : request->requestedLifetime;
    if(channel->altSecurityToken.revisedLifetime == 0)
        channel->altSecurityToken.revisedLifetime = server->config.maxSecurityTokenLifetime;

    /* Set the nonces. The remote nonce will be "rotated in" when it is first used. */
    UA_ByteString_clear(&channel->remoteNonce);
    channel->remoteNonce = request->clientNonce;
    UA_ByteString_init(&request->clientNonce);

    response->responseHeader.serviceResult = UA_SecureChannel_generateLocalNonce(channel);
    UA_CHECK_STATUS(response->responseHeader.serviceResult, goto error);

    /* Update the channel state */
    channel->renewState = UA_SECURECHANNELRENEWSTATE_NEWTOKEN_SERVER;
    channel->state = UA_SECURECHANNELSTATE_OPEN;

    /* Set the response */
    response->securityToken = channel->altSecurityToken;
    response->securityToken.createdAt = el->dateTime_now(el); /* only for sending */
    response->responseHeader.timestamp = response->securityToken.createdAt;
    response->responseHeader.requestHandle = request->requestHeader.requestHandle;
    response->responseHeader.serviceResult =
        UA_ByteString_copy(&channel->localNonce, &response->serverNonce);
    UA_CHECK_STATUS(response->responseHeader.serviceResult, goto error);

    /* Success */
    if(request->requestType == UA_SECURITYTOKENREQUESTTYPE_ISSUE) {
        UA_LOG_INFO_CHANNEL(server->config.logging, channel,
                            "SecureChannel opened with SecurityPolicy %.*s "
                            "and a revised lifetime of %.2fs",
                            (int)channel->securityPolicy->policyUri.length,
                            channel->securityPolicy->policyUri.data,
                            (UA_Float)response->securityToken.revisedLifetime / 1000);
    } else {
        UA_LOG_INFO_CHANNEL(server->config.logging, channel, "SecureChannel renewed "
                            "with a revised lifetime of %.2fs",
                            (UA_Float)response->securityToken.revisedLifetime / 1000);
    }

    return;

 error:
    if(request->requestType == UA_SECURITYTOKENREQUESTTYPE_ISSUE) {
        UA_LOG_INFO_CHANNEL(server->config.logging, channel,
                            "Opening a SecureChannel failed");
    } else {
        UA_LOG_DEBUG_CHANNEL(server->config.logging, channel,
                             "Renewing SecureChannel failed");
    }
}

/* The server does not send a CloseSecureChannel response */
void
Service_CloseSecureChannel(UA_Server *server, UA_SecureChannel *channel) {
    UA_SecureChannel_shutdown(channel, UA_SHUTDOWNREASON_CLOSE);
}
