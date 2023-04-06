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

#ifndef container_of
#define container_of(ptr, type, member) \
    (type *)((uintptr_t)ptr - offsetof(type,member))
#endif

void
deleteServerSecureChannel(UA_Server *server, UA_SecureChannel *channel) {
    UA_LOG_INFO_CHANNEL(&server->config.logger, channel, "SecureChannel closed");

    /* Clean up the SecureChannel. This is the only place where
     * UA_SecureChannel_clear must be called within the server code-base. */
    UA_SecureChannel_clear(channel);

    /* Detach the channel from the server list */
    struct channel_entry *entry = container_of(channel, channel_entry, channel);
    TAILQ_REMOVE(&server->channels, entry, pointers);

    /* Update the statistics */
    UA_SecureChannelStatistics *scs = &server->secureChannelStatistics;
    scs->currentChannelCount--;
    switch(entry->closeEvent) {
    case UA_DIAGNOSTICEVENT_CLOSE:
        break;
    case UA_DIAGNOSTICEVENT_TIMEOUT:
        scs->channelTimeoutCount++;
        break;
    case UA_DIAGNOSTICEVENT_PURGE:
        scs->channelPurgeCount++;
        break;
    case UA_DIAGNOSTICEVENT_REJECT:
    case UA_DIAGNOSTICEVENT_SECURITYREJECT:
        scs->rejectedChannelCount++;
        break;
    case UA_DIAGNOSTICEVENT_ABORT:
        scs->channelAbortCount++;
        break;
    default:
        UA_assert(false);
        break;
    }

    UA_free(entry);
}

/* Trigger the closing of the SecureChannel. This needs one iteration of the
 * eventloop to take effect. */
void
shutdownServerSecureChannel(UA_Server *server, UA_SecureChannel *channel,
                            UA_DiagnosticEvent event) {
    /* Does the channel have an open socket? */
    if(!UA_SecureChannel_isConnected(channel))
        return;

    UA_LOG_INFO_CHANNEL(&server->config.logger, channel, "Closing the channel");

    /* Set the event for diagnostics. The shutdown event is used in the
     * deleteServerSecureChannel callback. */
    struct channel_entry *entry = container_of(channel, channel_entry, channel);
    entry->closeEvent = event;

    /* Close the connection in the event-loop, which in the next iteration of
     * the eventloop triggers the final deletion. This also sets the state to
     * "closing". */
    UA_SecureChannel_shutdown(channel);
}

void
UA_Server_deleteSecureChannels(UA_Server *server) {
    channel_entry *entry;
    TAILQ_FOREACH(entry, &server->channels, pointers) {
        shutdownServerSecureChannel(server, &entry->channel, UA_DIAGNOSTICEVENT_CLOSE);
    }
}

/* Remove channels that were not renewed in time */
void
UA_Server_cleanupTimedOutSecureChannels(UA_Server *server,
                                        UA_DateTime nowMonotonic) {
    channel_entry *entry;
    TAILQ_FOREACH(entry, &server->channels, pointers) {
        /* Compute the timeout date of the SecurityToken */
        UA_DateTime timeout =
            entry->channel.securityToken.createdAt +
            (UA_DateTime)(entry->channel.securityToken.revisedLifetime * UA_DATETIME_MSEC);

        /* The token has timed out. Try to do the token revolving now instead of
         * shutting the channel down.
         *
         * Part 4, 5.5.2 says: Servers shall use the existing SecurityToken to
         * secure outgoing Messages until the SecurityToken expires or the
         * Server receives a Message secured with a new SecurityToken.*/
        if(timeout < nowMonotonic &&
           entry->channel.renewState == UA_SECURECHANNELRENEWSTATE_NEWTOKEN_SERVER) {
            /* Revolve the token manually. This is otherwise done in checkSymHeader. */
            entry->channel.renewState = UA_SECURECHANNELRENEWSTATE_NORMAL;
            entry->channel.securityToken = entry->channel.altSecurityToken;
            UA_ChannelSecurityToken_init(&entry->channel.altSecurityToken);
            UA_SecureChannel_generateLocalKeys(&entry->channel);
            generateRemoteKeys(&entry->channel);

            /* Use the timeout of the new SecurityToken */
            timeout = entry->channel.securityToken.createdAt +
                (UA_DateTime)(entry->channel.securityToken.revisedLifetime * UA_DATETIME_MSEC);
        }

        if(timeout < nowMonotonic) {
            UA_LOG_INFO_CHANNEL(&server->config.logger, &entry->channel,
                                "SecureChannel has timed out");
            shutdownServerSecureChannel(server, &entry->channel, UA_DIAGNOSTICEVENT_TIMEOUT);
        }
    }
}

/* remove the first channel that has no session attached */
static UA_Boolean
purgeFirstChannelWithoutSession(UA_Server *server) {
    channel_entry *entry;
    TAILQ_FOREACH(entry, &server->channels, pointers) {
        if(SLIST_FIRST(&entry->channel.sessions))
            continue;
        UA_LOG_INFO_CHANNEL(&server->config.logger, &entry->channel,
                            "Channel was purged since maxSecureChannels was "
                            "reached and channel had no session attached");
        shutdownServerSecureChannel(server, &entry->channel, UA_DIAGNOSTICEVENT_PURGE);
        return true;
    }
    return false;
}

UA_StatusCode
createServerSecureChannel(UA_Server *server, UA_ConnectionManager *cm,
                          uintptr_t connectionId, UA_SecureChannel **outChannel) {
    UA_ServerConfig *config = &server->config;

    /* Check if we have space for another SC, otherwise try to find an SC
     * without a session and purge it */
    UA_SecureChannelStatistics *scs = &server->secureChannelStatistics;
    if(scs->currentChannelCount >= config->maxSecureChannels &&
       !purgeFirstChannelWithoutSession(server))
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Allocate memory for the SecureChannel */
    channel_entry *entry = (channel_entry *)UA_malloc(sizeof(channel_entry));
    if(!entry)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Set up the initial connection config */
    UA_ConnectionConfig connConfig;
    connConfig.protocolVersion = 0;
    connConfig.recvBufferSize = config->tcpBufSize;
    connConfig.sendBufferSize = config->tcpBufSize;
    connConfig.localMaxMessageSize = config->tcpMaxMsgSize;
    connConfig.remoteMaxMessageSize = config->tcpMaxMsgSize;
    connConfig.localMaxChunkCount = config->tcpMaxChunks;
    connConfig.remoteMaxChunkCount = config->tcpMaxChunks;

    if(connConfig.recvBufferSize == 0)
        connConfig.recvBufferSize = 1 << 16; /* 64kB */
    if(connConfig.sendBufferSize == 0)
        connConfig.sendBufferSize = 1 << 16; /* 64kB */

    /* Set up the new SecureChannel */
    UA_SecureChannel_init(&entry->channel);
    entry->channel.config = connConfig;
    entry->channel.certificateVerification = &config->certificateVerification;
    entry->channel.processOPNHeader = configServerSecureChannel;
    entry->channel.connectionManager = cm;
    entry->channel.connectionId = connectionId;
    entry->closeEvent = UA_DIAGNOSTICEVENT_CLOSE; /* Used if the eventloop closes */

    /* Set the SecureChannel identifier already here. So we get the right
     * identifier for logging right away. The rest of the SecurityToken is set
     * in UA_SecureChannelManager_open. Set the ChannelId also in the
     * alternative security token, we don't touch this value during the token
     * rollover. */
    entry->channel.securityToken.channelId = server->lastChannelId++;
    entry->channel.altSecurityToken.channelId = entry->channel.securityToken.channelId;

    /* Set an initial timeout before the negotiation handshake. So the channel
     * is caught in UA_Server_cleanupTimedOutSecureChannels if the client is
     * unresponsive.
     *
     * TODO: Make this a configuration option */
    entry->channel.securityToken.createdAt = UA_DateTime_nowMonotonic();
    entry->channel.securityToken.revisedLifetime = 10000; /* 10s should be enough */

    /* Add to the server's list */
    TAILQ_INSERT_TAIL(&server->channels, entry, pointers);

    /* Update the statistics */
    server->secureChannelStatistics.currentChannelCount++;
    server->secureChannelStatistics.cumulatedChannelCount++;

    *outChannel = &entry->channel;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
configServerSecureChannel(void *application, UA_SecureChannel *channel,
                          const UA_AsymmetricAlgorithmSecurityHeader *asymHeader) {
    /* Iterate over available endpoints and choose the correct one */
    UA_SecurityPolicy *securityPolicy = NULL;
    UA_Server *const server = (UA_Server *const) application;
    for(size_t i = 0; i < server->config.securityPoliciesSize; ++i) {
        UA_SecurityPolicy *policy = &server->config.securityPolicies[i];
        if(!UA_ByteString_equal(&asymHeader->securityPolicyUri, &policy->policyUri))
            continue;

        UA_StatusCode res = policy->asymmetricModule.
            compareCertificateThumbprint(policy, &asymHeader->receiverCertificateThumbprint);
        if(res != UA_STATUSCODE_GOOD)
            continue;

        /* We found the correct policy (except for security mode). The endpoint
         * needs to be selected by the client / server to match the security
         * mode in the endpoint for the session. */
        securityPolicy = policy;
        break;
    }

    if(!securityPolicy)
        return UA_STATUSCODE_BADSECURITYPOLICYREJECTED;

    /* Create the channel context and parse the sender (remote) certificate used
     * for the secureChannel. */
    return UA_SecureChannel_setSecurityPolicy(channel, securityPolicy,
                                              &asymHeader->senderCertificate);
}

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
    channel->altSecurityToken.tokenId = server->lastTokenId++;
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
    channel->altSecurityToken.tokenId = server->lastTokenId++;
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
    shutdownServerSecureChannel(server, channel, UA_DIAGNOSTICEVENT_CLOSE);
}
