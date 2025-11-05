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

#include <open62541/types.h>
#include "ua_server_internal.h"
#include "ua_services.h"

void
notifySecureChannel(UA_Server *server, UA_SecureChannel *channel,
                    UA_ApplicationNotificationType type) {
    /* Nothing to do? */
    if(!server->config.globalNotificationCallback &&
       !server->config.secureChannelNotificationCallback)
        return;

    /* Prepare the payload */
    static UA_THREAD_LOCAL UA_KeyValuePair notifySCData[15] = {
        {{0, UA_STRING_STATIC("securechannel-id")}, {0}},
        {{0, UA_STRING_STATIC("connection-manager-name")}, {0}},
        {{0, UA_STRING_STATIC("connection-id")}, {0}},
        {{0, UA_STRING_STATIC("remote-address")}, {0}},
        {{0, UA_STRING_STATIC("protocol-version")}, {0}},
        {{0, UA_STRING_STATIC("recv-buffer-size")}, {0}},
        {{0, UA_STRING_STATIC("recv-max-message-size")}, {0}},
        {{0, UA_STRING_STATIC("recv-max-chunk-count")}, {0}},
        {{0, UA_STRING_STATIC("send-buffer-size")}, {0}},
        {{0, UA_STRING_STATIC("send-max-message-size")}, {0}},
        {{0, UA_STRING_STATIC("send-max-chunk-count")}, {0}},
        {{0, UA_STRING_STATIC("endpoint-url")}, {0}},
        {{0, UA_STRING_STATIC("security-mode")}, {0}},
        {{0, UA_STRING_STATIC("security-policy-url")}, {0}},
        {{0, UA_STRING_STATIC("remote-certificate")}, {0}}
    };
    UA_KeyValueMap notifySCMap = {15, notifySCData};

    UA_Variant_setScalar(&notifySCData[0].value, &channel->securityToken.channelId,
                         &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&notifySCData[1].value,
                         &channel->connectionManager->eventSource.name,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_UInt64 connectionId = channel->connectionId;
    UA_Variant_setScalar(&notifySCData[2].value, &connectionId,
                         &UA_TYPES[UA_TYPES_UINT64]);
    UA_Variant_setScalar(&notifySCData[3].value, &channel->remoteAddress,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&notifySCData[4].value, &channel->config.protocolVersion,
                         &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&notifySCData[5].value, &channel->config.recvBufferSize,
                         &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&notifySCData[6].value, &channel->config.localMaxMessageSize,
                         &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&notifySCData[7].value, &channel->config.localMaxChunkCount,
                         &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&notifySCData[8].value, &channel->config.sendBufferSize,
                         &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&notifySCData[9].value, &channel->config.remoteMaxMessageSize,
                         &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&notifySCData[10].value, &channel->config.remoteMaxChunkCount,
                         &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&notifySCData[11].value, &channel->endpointUrl,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&notifySCData[12].value, &channel->securityMode,
                         &UA_TYPES[UA_TYPES_MESSAGESECURITYMODE]);
    UA_String securityPolicyUri = UA_STRING_NULL;
    if(channel->securityPolicy)
        securityPolicyUri = channel->securityPolicy->policyUri;
    UA_Variant_setScalar(&notifySCData[13].value, &securityPolicyUri,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&notifySCData[14].value, &channel->remoteCertificate,
                         &UA_TYPES[UA_TYPES_BYTESTRING]);

    /* Notify the application */
    if(server->config.secureChannelNotificationCallback)
        server->config.secureChannelNotificationCallback(server, type, notifySCMap);
    if(server->config.globalNotificationCallback)
        server->config.globalNotificationCallback(server, type, notifySCMap);
}

void
Service_OpenSecureChannel(UA_Server *server, UA_SecureChannel *channel,
                          UA_OpenSecureChannelRequest *request,
                          UA_OpenSecureChannelResponse *response) {
    UA_EventLoop *el = server->config.eventLoop;
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
           UA_String_equal(&sp->policyUri, &UA_SECURITY_POLICY_NONE_URI)) {
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
    channel->altSecurityToken.channelId = channel->securityToken.channelId;
    channel->altSecurityToken.tokenId = server->lastTokenId++;
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
                            "SecureChannel opened with SecurityPolicy %S "
                            "and a revised lifetime of %.2fs",
                            channel->securityPolicy->policyUri,
                            (UA_Float)response->securityToken.revisedLifetime / 1000);

        /* Notify the application about the open SecureChannel */
        notifySecureChannel(server, channel,
                            UA_APPLICATIONNOTIFICATIONTYPE_SECURECHANNEL_OPENED);
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
