/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_securechannel_manager.h"
#include "ua_session.h"
#include "ua_server_internal.h"
#include "ua_transport_generated_handling.h"

#define STARTCHANNELID 1
#define STARTTOKENID 1

UA_StatusCode
UA_SecureChannelManager_init(UA_SecureChannelManager* cm, UA_Server* server) {
    LIST_INIT(&cm->channels);
    // TODO: use an ID that is likely to be unique after a restart
    cm->lastChannelId = STARTCHANNELID;
    cm->lastTokenId = STARTTOKENID;
    cm->currentChannelCount = 0;
    cm->server = server;
    return UA_STATUSCODE_GOOD;
}

void UA_SecureChannelManager_deleteMembers(UA_SecureChannelManager* cm) {
    channel_list_entry *entry, *temp;
    LIST_FOREACH_SAFE(entry, &cm->channels, pointers, temp) {
        LIST_REMOVE(entry, pointers);
        UA_SecureChannel_deleteMembersCleanup(&entry->channel);
        UA_free(entry);
    }
}

static void
removeSecureChannelCallback(UA_Server *server, void *entry) {
    channel_list_entry *centry = (channel_list_entry*)entry;
    UA_SecureChannel_deleteMembersCleanup(&centry->channel);
    UA_free(entry);
}

static UA_StatusCode
removeSecureChannel(UA_SecureChannelManager *cm, channel_list_entry *entry) {
    /* Add a delayed callback to remove the channel when the currently
     * scheduled jobs have completed */
    UA_StatusCode retval = UA_Server_delayedCallback(cm->server, removeSecureChannelCallback, entry);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(cm->server->config.logger, UA_LOGCATEGORY_SESSION,
                       "Could not remove the secure channel with error code %s",
                       UA_StatusCode_name(retval));
        return retval; /* Try again next time */
    }

    /* Detach the channel and make the capacity available */
    LIST_REMOVE(entry, pointers);
    UA_atomic_add(&cm->currentChannelCount, (UA_UInt32)-1);
    return UA_STATUSCODE_GOOD;
}

/* remove channels that were not renewed or who have no connection attached */
void
UA_SecureChannelManager_cleanupTimedOut(UA_SecureChannelManager *cm, UA_DateTime nowMonotonic) {
    channel_list_entry *entry, *temp;
    LIST_FOREACH_SAFE(entry, &cm->channels, pointers, temp) {
        UA_DateTime timeout = entry->channel.securityToken.createdAt +
            (UA_DateTime)(entry->channel.securityToken.revisedLifetime * UA_MSEC_TO_DATETIME);
        if(timeout < nowMonotonic || !entry->channel.connection) {
            UA_LOG_INFO_CHANNEL(cm->server->config.logger, &entry->channel,
                                "SecureChannel has timed out");
            removeSecureChannel(cm, entry);
        }
        else if(entry->channel.nextSecurityToken.tokenId > 0) {
            UA_SecureChannel_revolveTokens(&entry->channel);
        }
    }
}

/* remove the first channel that has no session attached */
static UA_Boolean purgeFirstChannelWithoutSession(UA_SecureChannelManager* cm) {
    channel_list_entry* entry;
    LIST_FOREACH(entry, &cm->channels, pointers) {
        if(LIST_EMPTY(&(entry->channel.sessions))) {
            UA_LOG_DEBUG_CHANNEL(cm->server->config.logger, &entry->channel,
                                 "Channel was purged since maxSecureChannels was "
                                 "reached and channel had no session attached");
            removeSecureChannel(cm, entry);
            UA_assert(entry != LIST_FIRST(&cm->channels));
            return true;
        }
    }
    return false;
}

UA_StatusCode
UA_SecureChannelManager_create(UA_SecureChannelManager *const cm, UA_Connection *const connection,
                               const UA_SecurityPolicy *const securityPolicy,
                               const UA_AsymmetricAlgorithmSecurityHeader *const asymHeader) {
    /* connection already has a channel attached. */
    if(connection->channel != NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Check if there exists a free SC, otherwise try to purge one SC without a
     * session the purge has been introduced to pass CTT, it is not clear what
     * strategy is expected here */
    if(cm->currentChannelCount >= cm->server->config.maxSecureChannels &&
       !purgeFirstChannelWithoutSession(cm))
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_LOG_INFO(cm->server->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                "Creating a new SecureChannel");

    channel_list_entry* entry = (channel_list_entry*)UA_malloc(sizeof(channel_list_entry));
    if(!entry)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Create the channel context and parse the sender (remote) certificate used for the
     * secureChannel. */
    UA_StatusCode retval = UA_SecureChannel_init(&entry->channel, securityPolicy,
                                                 &asymHeader->senderCertificate);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(entry);
        return retval;
    }

    /* Channel state is fresh (0) */
    entry->channel.securityToken.channelId = 0;
    entry->channel.securityToken.tokenId = cm->lastTokenId++;
    entry->channel.securityToken.createdAt = UA_DateTime_now();
    entry->channel.securityToken.revisedLifetime = cm->server->config.maxSecurityTokenLifetime;

    LIST_INSERT_HEAD(&cm->channels, entry, pointers);
    UA_atomic_add(&cm->currentChannelCount, 1);
    UA_Connection_attachSecureChannel(connection, &entry->channel);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_SecureChannelManager_open(UA_SecureChannelManager* cm, UA_SecureChannel *channel,
                             const UA_OpenSecureChannelRequest* request,
                             UA_OpenSecureChannelResponse* response) {
    if(channel->state != UA_SECURECHANNELSTATE_FRESH) {
        UA_LOG_ERROR_CHANNEL(cm->server->config.logger, channel,
                             "Called open on already open or closed channel");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    if(request->securityMode != UA_MESSAGESECURITYMODE_NONE &&
       UA_ByteString_equal(&channel->securityPolicy->policyUri, &UA_SECURITY_POLICY_NONE_URI)) {
        return UA_STATUSCODE_BADSECURITYMODEREJECTED;
    }

    channel->securityToken.channelId = cm->lastChannelId++;
    channel->securityToken.createdAt = UA_DateTime_now();
    channel->securityToken.revisedLifetime =
        (request->requestedLifetime > cm->server->config.maxSecurityTokenLifetime) ?
        cm->server->config.maxSecurityTokenLifetime : request->requestedLifetime;
    if(channel->securityToken.revisedLifetime == 0) // lifetime 0 -> set the maximum possible
        channel->securityToken.revisedLifetime = cm->server->config.maxSecurityTokenLifetime;
    UA_ByteString_copy(&request->clientNonce, &channel->remoteNonce);
    channel->securityMode = request->securityMode;
    const size_t keyLength = channel->securityPolicy->symmetricModule.cryptoModule.
        getLocalEncryptionKeyLength(channel->securityPolicy, channel->channelContext);
    UA_SecureChannel_generateNonce(channel,
                                   keyLength,
                                   &channel->localNonce);

    UA_SecureChannel_generateNewKeys(channel);

    // Set the response
    UA_ByteString_copy(&channel->localNonce, &response->serverNonce);
    UA_ChannelSecurityToken_copy(&channel->securityToken, &response->securityToken);
    response->responseHeader.timestamp = UA_DateTime_now();

    // Now overwrite the creation date with the internal monotonic clock
    channel->securityToken.createdAt = UA_DateTime_nowMonotonic();

    channel->state = UA_SECURECHANNELSTATE_OPEN;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_SecureChannelManager_renew(UA_SecureChannelManager* cm, UA_SecureChannel *channel,
                              const UA_OpenSecureChannelRequest* request,
                              UA_OpenSecureChannelResponse* response) {
    /* If no security token is already issued */
    if(channel->nextSecurityToken.tokenId == 0) {
        channel->nextSecurityToken.channelId = channel->securityToken.channelId;
        channel->nextSecurityToken.tokenId = cm->lastTokenId++;
        channel->nextSecurityToken.createdAt = UA_DateTime_now();
        channel->nextSecurityToken.revisedLifetime =
            (request->requestedLifetime > cm->server->config.maxSecurityTokenLifetime) ?
            cm->server->config.maxSecurityTokenLifetime : request->requestedLifetime;
        if(channel->nextSecurityToken.revisedLifetime == 0) /* lifetime 0 -> return the max lifetime */
            channel->nextSecurityToken.revisedLifetime = cm->server->config.maxSecurityTokenLifetime;
    }

    /* invalidate the old nonce */
    if(channel->remoteNonce.data)
        UA_ByteString_deleteMembers(&channel->remoteNonce);
    if(channel->localNonce.data)
        UA_ByteString_deleteMembers(&channel->localNonce);

    /* set the response */
    UA_ByteString_copy(&request->clientNonce, &channel->remoteNonce);
    const size_t keyLength = channel->securityPolicy->symmetricModule.cryptoModule.
        getLocalEncryptionKeyLength(channel->securityPolicy, channel->channelContext);
    UA_SecureChannel_generateNonce(channel,
                                   keyLength,
                                   &channel->localNonce);
    UA_ByteString_copy(&channel->localNonce, &response->serverNonce);
    UA_ChannelSecurityToken_copy(&channel->nextSecurityToken, &response->securityToken);

    /* reset the creation date to the monotonic clock */
    channel->nextSecurityToken.createdAt = UA_DateTime_nowMonotonic();
    return UA_STATUSCODE_GOOD;
}

UA_SecureChannel*
UA_SecureChannelManager_get(UA_SecureChannelManager* cm, UA_UInt32 channelId) {
    channel_list_entry* entry;
    LIST_FOREACH(entry, &cm->channels, pointers) {
        if(entry->channel.securityToken.channelId == channelId)
            return &entry->channel;
    }
    return NULL;
}

UA_StatusCode
UA_SecureChannelManager_close(UA_SecureChannelManager* cm, UA_UInt32 channelId) {
    channel_list_entry* entry;
    LIST_FOREACH(entry, &cm->channels, pointers) {
        if(entry->channel.securityToken.channelId == channelId)
            break;
    }
    if(!entry)
        return UA_STATUSCODE_BADINTERNALERROR;
    return removeSecureChannel(cm, entry);
}
