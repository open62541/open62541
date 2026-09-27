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
            validateCertificate(server, &sc->secureChannelPKI, securityPolicy,
                                channel, NULL,
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

void
Service_OpenSecureChannel(UA_Server *server, UA_SecureChannel *channel,
                          void *request_, void *response_) {
    UA_OpenSecureChannelRequest *request = (UA_OpenSecureChannelRequest*)request_;
    UA_OpenSecureChannelResponse *response = (UA_OpenSecureChannelResponse*)response_;
    /* Call the main OpenSecureChannel implementation */
    Service_OpenSecureChannel_inner(server, channel, request, response);

#ifdef UA_ENABLE_AUDITING
    auditOpenSecureChannelEvent(server, channel, request, response);
#endif
}

/* The server does not send a CloseSecureChannel response */
void
Service_CloseSecureChannel(UA_Server *server, UA_SecureChannel *channel) {
    if(UA_SecureChannel_isConnected(channel)) {
        UA_assert(channel->transport == UA_SECURECHANNEL_TRANSPORT_UACP);

#ifdef UA_ENABLE_AUDITING
        auditCloseSecureChannelEvent(server, channel);
#endif

        UA_SecureChannel_shutdown(channel, UA_SHUTDOWNREASON_CLOSE);
    }
}

/* The built-in, read-only ns0 SecureChannel attribute keys. Also the
 * payload keys of the SECURECHANNEL notification below. See the doc
 * comment on UA_Server_setSecureChannelAttribute in server.h and
 * UA_SecureChannel_getBuiltinAttribute below. */
const UA_QualifiedName UA_SecureChannel_builtinAttributeKeys[UA_SECURECHANNEL_BUILTIN_ATTRIBUTES_SIZE] = {
    {0, UA_STRING_STATIC("securechannel-id")},
    {0, UA_STRING_STATIC("connection-manager-name")},
    {0, UA_STRING_STATIC("connection-id")},
    {0, UA_STRING_STATIC("remote-address")},
    {0, UA_STRING_STATIC("protocol-version")},
    {0, UA_STRING_STATIC("recv-buffer-size")},
    {0, UA_STRING_STATIC("recv-max-message-size")},
    {0, UA_STRING_STATIC("recv-max-chunk-count")},
    {0, UA_STRING_STATIC("send-buffer-size")},
    {0, UA_STRING_STATIC("send-max-message-size")},
    {0, UA_STRING_STATIC("send-max-chunk-count")},
    {0, UA_STRING_STATIC("endpoint-url")},
    {0, UA_STRING_STATIC("security-mode")},
    {0, UA_STRING_STATIC("security-policy-url")},
    {0, UA_STRING_STATIC("certificate-type-id")},
    {0, UA_STRING_STATIC("remote-certificate")}
};

UA_Boolean
UA_SecureChannel_getBuiltinAttribute(UA_SecureChannel *channel,
                                     const UA_QualifiedName *key,
                                     UA_Variant *out) {
    /* Backing storage for the handful of values that are not stable,
     * addressable members of *channel and must be computed or defaulted on
     * the fly -- see the doc comment on channel->builtinAttributeScratch.
     * It lives on the channel itself, not a local of this function, so
     * *out stays valid for as long as the channel does, no matter when (or
     * whether) the caller consumes it. */
    const UA_QualifiedName *k = UA_SecureChannel_builtinAttributeKeys;
    if(UA_QualifiedName_equal(key, &k[0])) {
        UA_Variant_setScalar(out, &channel->securityToken.channelId,
                             &UA_TYPES[UA_TYPES_UINT32]);
    } else if(UA_QualifiedName_equal(key, &k[1])) {
        channel->builtinAttributeScratch.connectionManagerName = channel->connectionManager ?
            channel->connectionManager->eventSource.name : UA_STRING_NULL;
        UA_Variant_setScalar(out, &channel->builtinAttributeScratch.connectionManagerName,
                             &UA_TYPES[UA_TYPES_STRING]);
    } else if(UA_QualifiedName_equal(key, &k[2])) {
        channel->builtinAttributeScratch.connectionId = channel->connectionId;
        UA_Variant_setScalar(out, &channel->builtinAttributeScratch.connectionId,
                             &UA_TYPES[UA_TYPES_UINT64]);
    } else if(UA_QualifiedName_equal(key, &k[3])) {
        UA_Variant_setScalar(out, &channel->remoteAddress,
                             &UA_TYPES[UA_TYPES_STRING]);
    } else if(UA_QualifiedName_equal(key, &k[4])) {
        UA_Variant_setScalar(out, &channel->config.protocolVersion,
                             &UA_TYPES[UA_TYPES_UINT32]);
    } else if(UA_QualifiedName_equal(key, &k[5])) {
        UA_Variant_setScalar(out, &channel->config.recvBufferSize,
                             &UA_TYPES[UA_TYPES_UINT32]);
    } else if(UA_QualifiedName_equal(key, &k[6])) {
        UA_Variant_setScalar(out, &channel->config.localMaxMessageSize,
                             &UA_TYPES[UA_TYPES_UINT32]);
    } else if(UA_QualifiedName_equal(key, &k[7])) {
        UA_Variant_setScalar(out, &channel->config.localMaxChunkCount,
                             &UA_TYPES[UA_TYPES_UINT32]);
    } else if(UA_QualifiedName_equal(key, &k[8])) {
        UA_Variant_setScalar(out, &channel->config.sendBufferSize,
                             &UA_TYPES[UA_TYPES_UINT32]);
    } else if(UA_QualifiedName_equal(key, &k[9])) {
        UA_Variant_setScalar(out, &channel->config.remoteMaxMessageSize,
                             &UA_TYPES[UA_TYPES_UINT32]);
    } else if(UA_QualifiedName_equal(key, &k[10])) {
        UA_Variant_setScalar(out, &channel->config.remoteMaxChunkCount,
                             &UA_TYPES[UA_TYPES_UINT32]);
    } else if(UA_QualifiedName_equal(key, &k[11])) {
        UA_Variant_setScalar(out, &channel->endpointUrl,
                             &UA_TYPES[UA_TYPES_STRING]);
    } else if(UA_QualifiedName_equal(key, &k[12])) {
        UA_Variant_setScalar(out, &channel->securityMode,
                             &UA_TYPES[UA_TYPES_MESSAGESECURITYMODE]);
    } else if(UA_QualifiedName_equal(key, &k[13])) {
        channel->builtinAttributeScratch.securityPolicyUri = channel->securityPolicy ?
            channel->securityPolicy->policyUri : UA_STRING_NULL;
        UA_Variant_setScalar(out, &channel->builtinAttributeScratch.securityPolicyUri,
                             &UA_TYPES[UA_TYPES_STRING]);
    } else if(UA_QualifiedName_equal(key, &k[14])) {
        channel->builtinAttributeScratch.certificateTypeId = channel->securityPolicy ?
            channel->securityPolicy->certificateTypeId : UA_NODEID_NULL;
        UA_Variant_setScalar(out, &channel->builtinAttributeScratch.certificateTypeId,
                             &UA_TYPES[UA_TYPES_NODEID]);
    } else if(UA_QualifiedName_equal(key, &k[15])) {
        UA_Variant_setScalar(out, &channel->remoteCertificate,
                             &UA_TYPES[UA_TYPES_BYTESTRING]);
    } else {
        return false;
    }
    return true;
}

void
notifySecureChannel(UA_Server *server, UA_SecureChannel *channel,
                    UA_ApplicationNotificationType type) {
    /* Prepare the payload -- the same built-in attributes exposed via
     * UA_Server_getSecureChannelAttribute and friends. */
    UA_STATIC_THREAD_LOCAL UA_KeyValuePair
        notifySCData[UA_SECURECHANNEL_BUILTIN_ATTRIBUTES_SIZE];
    for(size_t i = 0; i < UA_SECURECHANNEL_BUILTIN_ATTRIBUTES_SIZE; i++) {
        notifySCData[i].key = UA_SecureChannel_builtinAttributeKeys[i];
        UA_SecureChannel_getBuiltinAttribute(
            channel, &UA_SecureChannel_builtinAttributeKeys[i], &notifySCData[i].value);
    }
    UA_KeyValueMap notifySCMap =
        {UA_SECURECHANNEL_BUILTIN_ATTRIBUTES_SIZE, notifySCData};

    /* Notify the application */
    notifyApplication(server, type, notifySCMap);
}
