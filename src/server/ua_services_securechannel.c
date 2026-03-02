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
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <open62541/types.h>
#include "ua_server_internal.h"
#include "ua_services.h"

/* The OpenSecureChannel Service in the server is split as follows:
 *
 * - Decode the OPN Asymmetric Header (SecureChannel)
 * - Process the OPN Asymmetric Header (here, via channel->processOPNHeader callback)
 *   - Verify the remote certificate and configure the SecureChannel
 * - Verify the OPN message signature and decrypt (SecureChannel)
 * - Process the OpenSecureChannelRequest (here, via standard service call logic)
 */

UA_StatusCode
processOPN_AsymHeader(void *application, UA_SecureChannel *channel,
                      const UA_AsymmetricAlgorithmSecurityHeader *asymHeader) {
    if(channel->securityPolicy)
        return UA_STATUSCODE_GOOD;

    /* Iterate over available endpoints and choose the correct one */
    UA_Server *server = (UA_Server *)application;
    UA_ServerConfig *sc = &server->config;
    UA_SecurityPolicy *securityPolicy = NULL;
    for(size_t i = 0; i < sc->securityPoliciesSize; ++i) {
        UA_SecurityPolicy *policy = &sc->securityPolicies[i];
        if(!UA_String_equal(&asymHeader->securityPolicyUri, &policy->policyUri))
            continue;

        UA_StatusCode res = policy->
            compareCertThumbprint(policy, &asymHeader->receiverCertificateThumbprint);
        if(res != UA_STATUSCODE_GOOD)
            continue;

        /* We found the correct policy. The endpoint is selected later during
         * CreateSession. There the channel's SecurityMode also gets checked
         * against the endpoint definition. */
        securityPolicy = policy;
        break;
    }

    if(!securityPolicy)
        return UA_STATUSCODE_BADSECURITYPOLICYREJECTED;

    /* Verify the client certificate (chain).
     * Here we don't have the ApplicationDescription.
     * This check follows in the CreateSession service. */
    if(asymHeader->senderCertificate.length > 0) {
        UA_StatusCode res =
            validateCertificate(server, &sc->secureChannelPKI, channel, NULL,
                                "OpenSecureChannel", NULL, asymHeader->senderCertificate);
        UA_CHECK_STATUS(res, return res);
    }

    /* If the sender provides a chain of certificates then we shall extract the
     * ApplicationInstanceCertificate and ignore the extra bytes. See also: OPC
     * UA Part 6, V1.04, 6.7.2.3 Security Header, Table 42 - Asymmetric
     * algorithm Security header */
    UA_ByteString appInstCert = getLeafCertificate(asymHeader->senderCertificate);

    /* Create the channel context and parse the sender (remote) certificate used
     * for the secureChannel. This sets a "temporary SecurityMode" so that we
     * can properly decrypt the remaining OPN message. The final SecurityMode is
     * set in the OpenSecureChannel service. */
    return UA_SecureChannel_setSecurityPolicy(channel, securityPolicy, &appInstCert);
}

static void
Service_OpenSecureChannel_inner(UA_Server *server, UA_SecureChannel *channel,
                                UA_OpenSecureChannelRequest *request,
                                UA_OpenSecureChannelResponse *response) {
    UA_ServerConfig *sc = &server->config;
    UA_EventLoop *el = server->config.eventLoop;
    const UA_SecurityPolicy *sp = channel->securityPolicy;

    switch(request->requestType) {
    /* Open the channel */
    case UA_SECURITYTOKENREQUESTTYPE_ISSUE: {
        /* We must expect an OPN handshake */
        if(channel->state != UA_SECURECHANNELSTATE_ACK_SENT) {
            UA_LOG_ERROR_CHANNEL(sc->logging, channel,
                                 "OpenSecureChannel: Cannot open "
                                 "already open or closed channel");
            response->responseHeader.serviceResult = UA_STATUSCODE_BADINTERNALERROR;
            return;
        }

        /* Ensure the SecurityMode does not cause a wrong array access during
         * logging */
        if(request->securityMode > UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
            request->securityMode = UA_MESSAGESECURITYMODE_INVALID;

        /* Set the SecurityMode. This overwrites the "temporary SecurityMode"
         * that has been set set in UA_SecureChannel_setSecurityPolicy.*/
        response->responseHeader.serviceResult =
            UA_SecureChannel_setSecurityMode(channel, request->securityMode);
        if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR_CHANNEL(sc->logging, channel,
                                 "OpenSecureChannel: Client tries mismatching "
                                 "SecurityMode %s for SecurityPolicy %S",
                                 securityModeNames[request->securityMode],
                                 sp->policyUri);
            return;
        }
        break;
    }

    /* Renew the channel */
    case UA_SECURITYTOKENREQUESTTYPE_RENEW:
        /* The channel must be open to be renewed */
        if(channel->state != UA_SECURECHANNELSTATE_OPEN) {
            UA_LOG_ERROR_CHANNEL(sc->logging, channel,
                                 "OpenSecureChannel: The client called renew on "
                                 "channel which is not open");
            response->responseHeader.serviceResult = UA_STATUSCODE_BADINTERNALERROR;
            return;
        }

        /* Check whether the nonce was reused */
        if(channel->securityMode != UA_MESSAGESECURITYMODE_NONE &&
           UA_ByteString_equal(&channel->remoteNonce, &request->clientNonce)) {
            UA_LOG_ERROR_CHANNEL(sc->logging, channel,
                                 "OpenSecureChannel: The client called renew "
                                 "reusing the previous nonce");
            response->responseHeader.serviceResult =
                UA_STATUSCODE_BADSECURITYCHECKSFAILED;
            return;
        }

        break;

    /* Unknown request type */
    default:
        UA_LOG_ERROR_CHANNEL(sc->logging, channel,
                             "OpenSecureChannel: Unknown request type");
        response->responseHeader.serviceResult = UA_STATUSCODE_BADINTERNALERROR;
        return;
    }

    /* Create a new SecurityToken. It will be switched over when the first
     * message is received. The ChannelId is left unchanged. */
    channel->altSecurityToken.channelId = channel->securityToken.channelId;
    channel->altSecurityToken.tokenId = server->lastTokenId++;
    channel->altSecurityToken.createdAt = el->dateTime_nowMonotonic(el);
    channel->altSecurityToken.revisedLifetime =
        (request->requestedLifetime > sc->maxSecurityTokenLifetime) ?
        sc->maxSecurityTokenLifetime : request->requestedLifetime;
    if(channel->altSecurityToken.revisedLifetime == 0)
        channel->altSecurityToken.revisedLifetime =
            sc->maxSecurityTokenLifetime;

    /* Set the nonces. The remote nonce will be "rotated in" when it is first used. */
    UA_ByteString_clear(&channel->remoteNonce);
    channel->remoteNonce = request->clientNonce;
    UA_ByteString_init(&request->clientNonce);

    response->responseHeader.serviceResult = UA_SecureChannel_generateLocalNonce(channel);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_CHANNEL(sc->logging, channel,
                             "OpenSecureChannel: Cannot generate the local nonce");
        return;
    }

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
    UA_CHECK_STATUS(response->responseHeader.serviceResult, return);

    /* Success */
    if(request->requestType == UA_SECURITYTOKENREQUESTTYPE_ISSUE) {
        UA_LOG_INFO_CHANNEL(sc->logging, channel,
                            "OpenSecureChannel: Channel opened with SecurityMode %s for "
                            "SecurityPolicy %S and a revised lifetime of %.2fs",
                            securityModeNames[channel->securityMode],
                            channel->securityPolicy->policyUri,
                            (UA_Float)response->securityToken.revisedLifetime / 1000);

        /* Notify the application about the open SecureChannel */
        notifySecureChannel(server, channel,
                            UA_APPLICATIONNOTIFICATIONTYPE_SECURECHANNEL_OPENED);
    } else {
        UA_LOG_INFO_CHANNEL(sc->logging, channel,
                            "OpenSecureChannel: Channel renewed with a revised "
                            "lifetime of %.2fs",
                            (UA_Float)response->securityToken.revisedLifetime / 1000);
    }
}

#ifdef UA_ENABLE_AUDITING
static UA_THREAD_LOCAL UA_KeyValuePair channelAuditPayload[14] = {
    {{0, UA_STRING_STATIC("/ActionTimeStamp")}, {0}},             /* 0 */
    {{0, UA_STRING_STATIC("/Status")}, {0}},                      /* 1 */
    {{0, UA_STRING_STATIC("/ServerId")}, {0}},                    /* 2 */
    {{0, UA_STRING_STATIC("/ClientAuditEntryId")}, {0}},          /* 3 */
    {{0, UA_STRING_STATIC("/ClientUserId")}, {0}},                /* 4 */
    {{0, UA_STRING_STATIC("/StatusCodeId")}, {0}},                /* 5 */
    {{0, UA_STRING_STATIC("/SecureChannelId")}, {0}},             /* 6 */
    {{0, UA_STRING_STATIC("/SourceName")}, {0}},                  /* 7 */
    {{0, UA_STRING_STATIC("/ClientCertificate")}, {0}},           /* 8 */
    {{0, UA_STRING_STATIC("/ClientCertificateThumbprint")}, {0}}, /* 9 */
    {{0, UA_STRING_STATIC("/RequestType")}, {0}},                 /* 10 */
    {{0, UA_STRING_STATIC("/SecurityPolicyUri")}, {0}},           /* 11 */
    {{0, UA_STRING_STATIC("/SecurityMode")}, {0}},                /* 12 */
    {{0, UA_STRING_STATIC("/RequestedLifetime")}, {0}},           /* 13 */
};
#endif

void
Service_OpenSecureChannel(UA_Server *server, UA_SecureChannel *channel,
                          UA_OpenSecureChannelRequest *request,
                          UA_OpenSecureChannelResponse *response) {
    /* Call the main OpenSecureChannel implementation */
    Service_OpenSecureChannel_inner(server, channel, request, response);

#ifdef UA_ENABLE_AUDITING
    /* Create the audit event for CreateSecureChannel - also on failure */
    const UA_SecurityPolicy *sp = channel->securityPolicy;
    UA_KeyValueMap payloadMap = {14, channelAuditPayload};
    UA_Boolean status = (response->responseHeader.serviceResult != UA_STATUSCODE_GOOD);
    UA_ByteString certThumbprint = {20, channel->remoteCertificateThumbprint};
    UA_Variant_setScalar(&channelAuditPayload[8].value, &channel->remoteCertificate,
                         &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Variant_setScalar(&channelAuditPayload[9].value, &certThumbprint,
                         &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Variant_setScalar(&channelAuditPayload[10].value, &request->requestType,
                         &UA_TYPES[UA_TYPES_SECURITYTOKENREQUESTTYPE]);
    UA_Variant_setScalar(&channelAuditPayload[11].value, (void*)(uintptr_t) &sp->policyUri,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&channelAuditPayload[12].value, &request->securityMode,
                         &UA_TYPES[UA_TYPES_MESSAGESECURITYMODE]);
    UA_Variant_setScalar(&channelAuditPayload[13].value, &request->requestedLifetime,
                         &UA_TYPES[UA_TYPES_UINT32]);
    auditChannelEvent(server, UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CHANNEL_OPEN,
                      channel, NULL, "OpenSecureChannel", status,
                      response->responseHeader.serviceResult, payloadMap);
#endif
}

/* The server does not send a CloseSecureChannel response */
void
Service_CloseSecureChannel(UA_Server *server, UA_SecureChannel *channel) {
    if(UA_SecureChannel_isConnected(channel)) {
        /* Shutdown - takes effect in the next EventLoop iteration */
        UA_SecureChannel_shutdown(channel, UA_SHUTDOWNREASON_CLOSE);

#ifdef UA_ENABLE_AUDITING
        /* Create the audit event */
        UA_KeyValueMap payloadMap = {8, channelAuditPayload}; /* Not all fields */
        auditChannelEvent(server, UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CHANNEL,
                          channel, NULL, "CloseSecureChannel", true,
                          UA_STATUSCODE_GOOD, payloadMap);
#endif
    }
}

void
notifySecureChannel(UA_Server *server, UA_SecureChannel *channel,
                    UA_ApplicationNotificationType type) {
    UA_ServerConfig *sc = &server->config;

    /* Nothing to do? */
    if(!sc->globalNotificationCallback && !sc->secureChannelNotificationCallback)
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
    if(sc->secureChannelNotificationCallback)
        sc->secureChannelNotificationCallback(server, type, notifySCMap);
    if(sc->globalNotificationCallback)
        sc->globalNotificationCallback(server, type, notifySCMap);
}
