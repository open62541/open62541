/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014, 2017 (c) Florian Palm
 *    Copyright 2015 (c) Oleksiy Vasylyev
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Mark Giraud, Fraunhofer IOSB
 */

#include "ua_server_internal.h"
#include "ua_services.h"

/* This contains the SecureChannel Services to be called after validation and
 * decoding of the message. The main SecureChannel logic is handled in
 * /src/ua_securechannel.* and /src/server/ua_server_binary.c. */

static UA_StatusCode
UA_SecureChannelManager_open(UA_Server *server, UA_SecureChannel *channel,
                             const UA_OpenSecureChannelRequest *request,
                             UA_OpenSecureChannelResponse *response) {
    if(channel->state != UA_SECURECHANNELSTATE_ACK_SENT) {
        UA_LOG_ERROR_CHANNEL(&server->config.logger, channel,
                             "Called open on already open or closed channel");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Set the SecurityMode */
    const UA_SecurityPolicy *sp = channel->securityPolicy;
    if(request->securityMode != UA_MESSAGESECURITYMODE_NONE &&
       UA_ByteString_equal(&sp->policyUri, &UA_SECURITY_POLICY_NONE_URI))
        return UA_STATUSCODE_BADSECURITYMODEREJECTED;
    channel->securityMode = request->securityMode;

    /* Set the initial SecurityToken. Set the alternative token that is moved to
     * the primary token when the first symmetric message triggers a token
     * revolve. Lifetime 0 -> set the maximum possible lifetime */
    channel->renewState = UA_SECURECHANNELRENEWSTATE_NEWTOKEN_SERVER;
    channel->altSecurityToken.tokenId = generateSecureChannelTokenId(server);
    channel->altSecurityToken.createdAt = UA_DateTime_nowMonotonic();
    channel->altSecurityToken.revisedLifetime =
        (request->requestedLifetime > server->config.maxSecurityTokenLifetime) ?
        server->config.maxSecurityTokenLifetime : request->requestedLifetime;
    if(channel->altSecurityToken.revisedLifetime == 0)
        channel->altSecurityToken.revisedLifetime = server->config.maxSecurityTokenLifetime;

    /* Store the received client nonce and generate the server nonce. The
     * syymmetric encryption keys are generated from the nonces when the first
     * symmetric message is received that triggers revolving of the token. */
    UA_StatusCode retval = UA_ByteString_copy(&request->clientNonce, &channel->remoteNonce);
    UA_CHECK_STATUS(retval, return retval);

    retval = UA_SecureChannel_generateLocalNonce(channel);
    UA_CHECK_STATUS(retval, return retval);

    /* Set the response. The token will be revolved to the new token when the
     * first symmetric messages is received. */
    response->securityToken = channel->altSecurityToken;
    response->securityToken.createdAt = UA_DateTime_now(); /* Only for sending */
    response->responseHeader.timestamp = response->securityToken.createdAt;
    response->responseHeader.requestHandle = request->requestHeader.requestHandle;
    retval = UA_ByteString_copy(&channel->localNonce, &response->serverNonce);
    UA_CHECK_STATUS(retval, return retval);

    /* The channel is open */
    channel->state = UA_SECURECHANNELSTATE_OPEN;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_SecureChannelManager_renew(UA_Server *server, UA_SecureChannel *channel,
                              const UA_OpenSecureChannelRequest *request,
                              UA_OpenSecureChannelResponse *response) {
    if(channel->state != UA_SECURECHANNELSTATE_OPEN) {
        UA_LOG_ERROR_CHANNEL(&server->config.logger, channel,
                             "Called renew on channel which is not open");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Check whether the nonce was reused */
    if(channel->securityMode != UA_MESSAGESECURITYMODE_NONE &&
       UA_ByteString_equal(&channel->remoteNonce, &request->clientNonce)) {
        UA_LOG_ERROR_CHANNEL(&server->config.logger, channel,
                             "The client reused the last nonce");
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }

    /* Create a new SecurityToken. Will be switched over when the first message
     * is received. The ChannelId is left unchanged. */
    channel->altSecurityToken = channel->securityToken;
    channel->altSecurityToken.tokenId = generateSecureChannelTokenId(server);
    channel->altSecurityToken.createdAt = UA_DateTime_nowMonotonic();
    channel->altSecurityToken.revisedLifetime =
        (request->requestedLifetime > server->config.maxSecurityTokenLifetime) ?
        server->config.maxSecurityTokenLifetime : request->requestedLifetime;
    if(channel->altSecurityToken.revisedLifetime == 0) /* lifetime 0 -> return the max lifetime */
        channel->altSecurityToken.revisedLifetime = server->config.maxSecurityTokenLifetime;

    /* Replace the nonces */
    UA_ByteString_clear(&channel->remoteNonce);
    UA_StatusCode retval = UA_ByteString_copy(&request->clientNonce, &channel->remoteNonce);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = UA_SecureChannel_generateLocalNonce(channel);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Set the response */
    response->securityToken = channel->altSecurityToken;
    response->securityToken.createdAt = UA_DateTime_now(); /* Only for sending */
    response->responseHeader.timestamp = response->securityToken.createdAt;
    response->responseHeader.requestHandle = request->requestHeader.requestHandle;
    retval = UA_ByteString_copy(&channel->localNonce, &response->serverNonce);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    channel->renewState = UA_SECURECHANNELRENEWSTATE_NEWTOKEN_SERVER;
    return UA_STATUSCODE_GOOD;
}

void
Service_OpenSecureChannel(UA_Server *server, UA_SecureChannel *channel,
                          const UA_OpenSecureChannelRequest *request,
                          UA_OpenSecureChannelResponse *response) {
    /* Renew the channel */
    if(request->requestType == UA_SECURITYTOKENREQUESTTYPE_RENEW) {
        response->responseHeader.serviceResult =
            UA_SecureChannelManager_renew(server, channel, request, response);
        if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
            UA_LOG_DEBUG_CHANNEL(&server->config.logger, channel,
                                 "Renewing SecureChannel failed");
            return;
        }

        /* Log the renewal and the lifetime */
        UA_Float lifetime = (UA_Float)response->securityToken.revisedLifetime / 1000;
        UA_LOG_INFO_CHANNEL(&server->config.logger, channel, "SecureChannel renewed "
                            "with a revised lifetime of %.2fs", lifetime);
        return;
    }

    /* Must be ISSUE or RENEW */
    if(request->requestType != UA_SECURITYTOKENREQUESTTYPE_ISSUE) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADINTERNALERROR;
        return;
    }

    /* Open the channel */
    response->responseHeader.serviceResult =
        UA_SecureChannelManager_open(server, channel, request, response);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_CHANNEL(&server->config.logger, channel,
                            "Opening a SecureChannel failed");
        return;
    }

    /* Log the lifetime */
    UA_Float lifetime = (UA_Float)response->securityToken.revisedLifetime / 1000;
    UA_LOG_INFO_CHANNEL(&server->config.logger, channel,
                        "SecureChannel opened with SecurityPolicy %.*s "
                        "and a revised lifetime of %.2fs",
                        (int)channel->securityPolicy->policyUri.length,
                        channel->securityPolicy->policyUri.data, lifetime);
}

/* The server does not send a CloseSecureChannel response */
void
Service_CloseSecureChannel(UA_Server *server, UA_SecureChannel *channel) {
    UA_SecureChannel_shutdown(channel, UA_SHUTDOWNREASON_CLOSE);
}
