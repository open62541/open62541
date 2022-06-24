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

static void
removeSecureChannelCallback(void *_, channel_entry *entry) {
    UA_SecureChannel_close(&entry->channel);
}

/* Half-closes the channel. Will be completely closed / deleted in a deferred
 * callback. Deferring is necessary so we don't remove lists that are still
 * processed upwards the call stack. */
static void
removeSecureChannel(UA_Server *server, channel_entry *entry,
                    UA_DiagnosticEvent event) {
    if(entry->channel.state == UA_SECURECHANNELSTATE_CLOSING)
        return;
    entry->channel.state = UA_SECURECHANNELSTATE_CLOSING;

    /* Detach from the connection and close the connection */
    if(entry->channel.connection) {
        UA_Connection_detachSecureChannel(entry->channel.connection);
    }

    /* Detach the channel */
    TAILQ_REMOVE(&server->channels, entry, pointers);

    /* Update the statistics */
    UA_SecureChannelStatistics *scs = &server->secureChannelStatistics;
    scs->currentChannelCount--;
    switch(event) {
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

    /* Add a delayed callback to remove the channel when the currently
     * scheduled jobs have completed */
    entry->cleanupCallback.callback = (UA_Callback)removeSecureChannelCallback;
    entry->cleanupCallback.application = NULL;
    entry->cleanupCallback.context = entry;
    server->config.eventLoop->
        addDelayedCallback(server->config.eventLoop, &entry->cleanupCallback);
}

void
UA_Server_deleteSecureChannels(UA_Server *server) {
    channel_entry *entry, *temp;
    TAILQ_FOREACH_SAFE(entry, &server->channels, pointers, temp)
        removeSecureChannel(server, entry, UA_DIAGNOSTICEVENT_CLOSE);
}

/* remove channels that were not renewed or who have no connection attached */
void
UA_Server_cleanupTimedOutSecureChannels(UA_Server *server,
                                        UA_DateTime nowMonotonic) {
    channel_entry *entry, *temp;
    TAILQ_FOREACH_SAFE(entry, &server->channels, pointers, temp) {
        /* The channel was closed internally */
        if(entry->channel.state == UA_SECURECHANNELSTATE_CLOSED ||
           !entry->channel.connection) {
            removeSecureChannel(server, entry, UA_DIAGNOSTICEVENT_CLOSE);
            continue;
        }

        /* Is the SecurityToken already created? */
        if(entry->channel.securityToken.createdAt == 0) {
        	/* No -> channel is still in progress of being opened, do not remove */
        	continue;
        }

        /* Has the SecurityToken timed out? */
        UA_DateTime timeout =
            entry->channel.securityToken.createdAt +
            (UA_DateTime)(entry->channel.securityToken.revisedLifetime * UA_DATETIME_MSEC);

        /* There is a new token. But it has not been used by the client so far.
         * Do the rollover now instead of shutting the channel down.
         *
         * Part 4, 5.5.2 says: Servers shall use the existing SecurityToken to
         * secure outgoing Messages until the SecurityToken expires or the
         * Server receives a Message secured with a new SecurityToken.*/
        if(timeout < nowMonotonic &&
           entry->channel.renewState == UA_SECURECHANNELRENEWSTATE_NEWTOKEN_SERVER) {
            /* Compare with the rollover in checkSymHeader */
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
            removeSecureChannel(server, entry, UA_DIAGNOSTICEVENT_TIMEOUT);
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
        removeSecureChannel(server, entry, UA_DIAGNOSTICEVENT_PURGE);
        return true;
    }
    return false;
}

UA_StatusCode
UA_Server_createSecureChannel(UA_Server *server, UA_Connection *connection) {
    /* connection already has a channel attached. */
    if(connection->channel != NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ServerConfig *config = &server->config;

    /* Check if there exists a free SC, otherwise try to purge one SC without a
     * session the purge has been introduced to pass CTT, it is not clear what
     * strategy is expected here */
    if((server->secureChannelStatistics.currentChannelCount >=
        config->maxSecureChannels) &&
       !purgeFirstChannelWithoutSession(server))
        return UA_STATUSCODE_BADOUTOFMEMORY;

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

    /* Channel state is closed (0) */
    UA_SecureChannel_init(&entry->channel, &connConfig);
    entry->channel.certificateVerification = &config->certificateVerification;
    entry->channel.processOPNHeader = UA_Server_configSecureChannel;

    TAILQ_INSERT_TAIL(&server->channels, entry, pointers);
    UA_Connection_attachSecureChannel(connection, &entry->channel);
    server->secureChannelStatistics.currentChannelCount++;
    server->secureChannelStatistics.cumulatedChannelCount++;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_configSecureChannel(void *application, UA_SecureChannel *channel,
                              const UA_AsymmetricAlgorithmSecurityHeader *asymHeader) {
    /* Iterate over available endpoints and choose the correct one */
    UA_SecurityPolicy *securityPolicy = NULL;
    UA_Server *const server = (UA_Server *const) application;
    for(size_t i = 0; i < server->config.securityPoliciesSize; ++i) {
        UA_SecurityPolicy *policy = &server->config.securityPolicies[i];
        if(!UA_ByteString_equal(&asymHeader->securityPolicyUri, &policy->policyUri))
            continue;

        UA_StatusCode retval = policy->asymmetricModule.
            compareCertificateThumbprint(policy, &asymHeader->receiverCertificateThumbprint);
        if(retval != UA_STATUSCODE_GOOD)
            continue;

        /* We found the correct policy (except for security mode). The endpoint
         * needs to be selected by the client / server to match the security
         * mode in the endpoint for the session. */
        securityPolicy = policy;
        break;
    }

    if(!securityPolicy)
        return UA_STATUSCODE_BADSECURITYPOLICYREJECTED;

    /* Create the channel context and parse the sender (remote) certificate used for the
     * secureChannel. */
    UA_StatusCode retval =
        UA_SecureChannel_setSecurityPolicy(channel, securityPolicy,
                                           &asymHeader->senderCertificate);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    channel->securityToken.tokenId = server->lastTokenId++;
    return UA_STATUSCODE_GOOD;
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

    if(request->securityMode != UA_MESSAGESECURITYMODE_NONE &&
       UA_ByteString_equal(&channel->securityPolicy->policyUri, &UA_SECURITY_POLICY_NONE_URI)) {
        return UA_STATUSCODE_BADSECURITYMODEREJECTED;
    }

    channel->securityMode = request->securityMode;
    channel->securityToken.channelId = server->lastChannelId++;
    channel->securityToken.createdAt = UA_DateTime_nowMonotonic();

    /* Set the lifetime. Lifetime 0 -> set the maximum possible */
    channel->securityToken.revisedLifetime =
        (request->requestedLifetime > server->config.maxSecurityTokenLifetime) ?
        server->config.maxSecurityTokenLifetime : request->requestedLifetime;
    if(channel->securityToken.revisedLifetime == 0)
        channel->securityToken.revisedLifetime = server->config.maxSecurityTokenLifetime;

    UA_StatusCode retval = UA_ByteString_copy(&request->clientNonce, &channel->remoteNonce);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Generate the nonce. The syymmetric encryption keys are generated when the
     * first symmetric message is received */
    retval = UA_SecureChannel_generateLocalNonce(channel);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Set the response. The token will be revolved to the new token when the
     * first symmetric messages is received. */
    response->securityToken = channel->securityToken;
    response->securityToken.createdAt = UA_DateTime_now(); /* Only for sending */
    response->responseHeader.timestamp = response->securityToken.createdAt;
    response->responseHeader.requestHandle = request->requestHeader.requestHandle;
    retval = UA_ByteString_copy(&channel->localNonce, &response->serverNonce);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* The channel is open */
    channel->state = UA_SECURECHANNELSTATE_OPEN;

    /* For the first revolve */
    channel->renewState = UA_SECURECHANNELRENEWSTATE_NEWTOKEN_SERVER;
    channel->altSecurityToken = channel->securityToken;
    channel->securityToken.tokenId = 0;
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
     * is received. */
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
    response->responseHeader.requestHandle = request->requestHeader.requestHandle;
    retval = UA_ByteString_copy(&channel->localNonce, &response->serverNonce);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    channel->renewState = UA_SECURECHANNELRENEWSTATE_NEWTOKEN_SERVER;
    return UA_STATUSCODE_GOOD;
}

void
UA_Server_closeSecureChannel(UA_Server *server, UA_SecureChannel *channel,
                             UA_DiagnosticEvent event) {
    removeSecureChannel(server, container_of(channel, channel_entry, channel), event);
}

void
Service_OpenSecureChannel(UA_Server *server, UA_SecureChannel *channel,
                          const UA_OpenSecureChannelRequest *request,
                          UA_OpenSecureChannelResponse *response) {
    if(request->requestType == UA_SECURITYTOKENREQUESTTYPE_RENEW) {
        /* Renew the channel */
        response->responseHeader.serviceResult =
            UA_SecureChannelManager_renew(server, channel, request, response);

        /* Logging */
        if(response->responseHeader.serviceResult == UA_STATUSCODE_GOOD) {
            UA_Float lifetime = (UA_Float)response->securityToken.revisedLifetime / 1000;
            UA_LOG_INFO_CHANNEL(&server->config.logger, channel, "SecureChannel renewed "
                                "with a revised lifetime of %.2fs", lifetime);
        } else {
            UA_LOG_DEBUG_CHANNEL(&server->config.logger, channel,
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
        UA_SecureChannelManager_open(server, channel, request, response);

    /* Logging */
    if(response->responseHeader.serviceResult == UA_STATUSCODE_GOOD) {
        UA_Float lifetime = (UA_Float)response->securityToken.revisedLifetime / 1000;
        UA_LOG_INFO_CHANNEL(&server->config.logger, channel,
                            "SecureChannel opened with SecurityPolicy %.*s "
                            "and a revised lifetime of %.2fs",
                            (int)channel->securityPolicy->policyUri.length,
                            channel->securityPolicy->policyUri.data, lifetime);
    } else {
        UA_LOG_INFO_CHANNEL(&server->config.logger, channel,
                            "Opening a SecureChannel failed");
    }
}

/* The server does not send a CloseSecureChannel response */
void
Service_CloseSecureChannel(UA_Server *server, UA_SecureChannel *channel) {
    UA_LOG_INFO_CHANNEL(&server->config.logger, channel, "CloseSecureChannel");
    removeSecureChannel(server, container_of(channel, channel_entry, channel),
                        UA_DIAGNOSTICEVENT_CLOSE);
}
