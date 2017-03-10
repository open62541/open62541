/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_securechannel_manager.h"
#include "ua_session.h"
#include "ua_server_internal.h"
#include "ua_transport_generated_handling.h"
#include "ua_types_generated_handling.h"

#define STARTCHANNELID 1
#define STARTTOKENID 1

UA_StatusCode
UA_SecureChannelManager_init(UA_SecureChannelManager* cm, UA_Server* server)
{
    LIST_INIT(&cm->channels);
    cm->lastChannelId = STARTCHANNELID;
    cm->lastTokenId = STARTTOKENID;
    cm->currentChannelCount = 0;
    cm->server = server;
    return UA_STATUSCODE_GOOD;
}

void UA_SecureChannelManager_deleteMembers(UA_SecureChannelManager* cm)
{
    channel_list_entry *entry, *temp;
    LIST_FOREACH_SAFE(entry, &cm->channels, pointers, temp)
    {
        LIST_REMOVE(entry, pointers);
        UA_SecureChannel_deleteMembersCleanup(&entry->channel);
        UA_free(entry);
    }
}

static void removeSecureChannel(UA_SecureChannelManager* cm, channel_list_entry* entry)
{
    LIST_REMOVE(entry, pointers);
    UA_atomic_add(&cm->currentChannelCount, (UA_UInt32)-1);
    UA_SecureChannel_deleteMembersCleanup(&entry->channel);
#ifndef UA_ENABLE_MULTITHREADING
    UA_free(entry);
#else
    UA_Server_delayedFree(cm->server, entry);
#endif
}

/* remove channels that were not renewed or who have no connection attached */
void UA_SecureChannelManager_cleanupTimedOut(UA_SecureChannelManager* cm, UA_DateTime nowMonotonic)
{
    channel_list_entry *entry, *temp;
    LIST_FOREACH_SAFE(entry, &cm->channels, pointers, temp)
    {
        UA_DateTime timeout = entry->channel.securityToken.createdAt +
            (UA_DateTime)(entry->channel.securityToken.revisedLifetime * UA_MSEC_TO_DATETIME);
        if (timeout < nowMonotonic || !entry->channel.connection)
        {
            UA_LOG_DEBUG_CHANNEL(cm->server->config.logger, &entry->channel, "SecureChannel has timed out");
            removeSecureChannel(cm, entry);
        }
        else if (entry->channel.nextSecurityToken.tokenId > 0)
        {
            UA_SecureChannel_revolveTokens(&entry->channel);
        }
    }
}

/* remove the first channel that has no session attached */
static UA_Boolean purgeFirstChannelWithoutSession(UA_SecureChannelManager* cm)
{
    channel_list_entry* entry;
    LIST_FOREACH(entry, &cm->channels, pointers)
    {
        if (LIST_EMPTY(&(entry->channel.sessions)) && !entry->channel.temporary)
        {
            UA_LOG_DEBUG_CHANNEL(cm->server->config.logger, &entry->channel,
                "Channel was purged since maxSecureChannels was reached and channel had no session attached");
            removeSecureChannel(cm, entry);
            UA_assert(entry != LIST_FIRST(&cm->channels));
            return true;
        }
    }
    return false;
}

UA_StatusCode
UA_SecureChannelManager_open_temporary(UA_SecureChannelManager* const cm,
                                       UA_SecureChannel** const pp_channel,
                                       UA_Connection* const connection)
{
    if (pp_channel == NULL)
    {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    // connection already has a channel attached. No need for a new temporary.
    if (connection->channel != NULL)
    {
        *pp_channel = connection->channel;
        return UA_STATUSCODE_GOOD;
    }

    //check if there exists a free SC, otherwise try to purge one SC without a session
    //the purge has been introduced to pass CTT, it is not clear what strategy is expected here
    if (cm->currentChannelCount >= cm->server->config.maxSecureChannels && !purgeFirstChannelWithoutSession(cm))
    {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_LOG_INFO(cm->server->config.logger, UA_LOGCATEGORY_SECURECHANNEL, "Creating a new temporary channel");

    channel_list_entry* entry = (channel_list_entry*)UA_malloc(sizeof(channel_list_entry));

    if (entry == NULL)
    {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_SecureChannel_init(&entry->channel, cm->server->config.securityPolicies, cm->server->config.logger);
    entry->channel.temporary = UA_TRUE;
    entry->channel.securityToken.channelId = cm->lastChannelId++;
    entry->channel.securityToken.tokenId = cm->lastTokenId++;
    entry->channel.securityToken.createdAt = UA_DateTime_now();
    entry->channel.securityToken.revisedLifetime = cm->server->config.maxSecurityTokenLifetime;

    UA_Connection_attachSecureChannel(connection, &entry->channel);
    LIST_INSERT_HEAD(&cm->channels, entry, pointers);
    UA_atomic_add(&cm->currentChannelCount, 1);

    (*pp_channel) = &entry->channel;

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_SecureChannelManager_close_temporary(UA_SecureChannelManager* const cm,
                                        UA_SecureChannel* const channel)
{
    if (cm == NULL || channel == NULL)
    {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_LOG_INFO(cm->server->config.logger, UA_LOGCATEGORY_SECURECHANNEL, "Closing temporary channel %i", channel->securityToken.channelId);

    return UA_SecureChannelManager_close(cm, channel->securityToken.channelId);
}

UA_StatusCode
UA_SecureChannelManager_open(UA_SecureChannelManager* cm, UA_Connection* conn,
                             const UA_OpenSecureChannelRequest* request,
                             UA_OpenSecureChannelResponse* response,
                             UA_SecureChannel* tmpChannel)
{
    //if(request->securityMode != UA_MESSAGESECURITYMODE_NONE)
    //    return UA_STATUSCODE_BADSECURITYMODEREJECTED;

    if (!tmpChannel->temporary)
    {
        UA_LOG_ERROR(cm->server->config.logger, UA_LOGCATEGORY_SECURECHANNEL, "Trying to open a channel with an already opened channel.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    //check if there exists a free SC, otherwise try to purge one SC without a session
    //the purge has been introduced to pass CTT, it is not clear what strategy is expected here
    if (cm->currentChannelCount >= cm->server->config.maxSecureChannels && !purgeFirstChannelWithoutSession(cm))
    {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Set up the channel */
    channel_list_entry* entry = (channel_list_entry*)UA_malloc(sizeof(channel_list_entry));
    if (!entry)
    {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    // move stuff from temporary channel to actual channel
    UA_SecureChannel_init(&entry->channel, cm->server->config.securityPolicies, cm->server->config.logger);
    entry->channel = *tmpChannel;
    UA_AsymmetricAlgorithmSecurityHeader_init(&entry->channel.clientAsymAlgSettings);
    UA_AsymmetricAlgorithmSecurityHeader_copy(&tmpChannel->clientAsymAlgSettings, &entry->channel.clientAsymAlgSettings);
    tmpChannel->securityContext = NULL; // We don't want to clean up the securityContext
    UA_SecureChannelManager_close_temporary(cm, tmpChannel);
    entry->channel.connection = NULL;
    entry->channel.temporary = UA_FALSE;

    // reinitialize lists because we copied the old ones over.
    LIST_INIT(&entry->channel.sessions);
    LIST_INIT(&entry->channel.chunks);

    entry->channel.securityToken.channelId = cm->lastChannelId++;
    entry->channel.securityToken.tokenId = cm->lastTokenId++;
    entry->channel.securityToken.createdAt = UA_DateTime_now();
    entry->channel.securityToken.revisedLifetime =
        (request->requestedLifetime > cm->server->config.maxSecurityTokenLifetime) ?
            cm->server->config.maxSecurityTokenLifetime : request->requestedLifetime;
    if (entry->channel.securityToken.revisedLifetime == 0) /* lifetime 0 -> set the maximum possible */
        entry->channel.securityToken.revisedLifetime = cm->server->config.maxSecurityTokenLifetime;
    UA_ByteString_copy(&request->clientNonce, &entry->channel.clientNonce);
    entry->channel.securityMode = request->securityMode;
    UA_SecureChannel_generateNonce(&entry->channel.serverNonce, entry->channel.securityPolicy);

    // Generate initial set of keys
    UA_SecureChannel_generateNewKeys(&entry->channel);

    /* Set the response */
    UA_ByteString_copy(&entry->channel.serverNonce, &response->serverNonce);
    UA_ChannelSecurityToken_copy(&entry->channel.securityToken, &response->securityToken);
    response->responseHeader.timestamp = UA_DateTime_now();

    /* Now overwrite the creation date with the internal monotonic clock */
    entry->channel.securityToken.createdAt = UA_DateTime_nowMonotonic();

    /* Set all the pointers internally */
    UA_Connection_attachSecureChannel(conn, &entry->channel);
    LIST_INSERT_HEAD(&cm->channels, entry, pointers);
    UA_atomic_add(&cm->currentChannelCount, 1);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_SecureChannelManager_renew(UA_SecureChannelManager* cm, UA_Connection* conn,
                              const UA_OpenSecureChannelRequest* request,
                              UA_OpenSecureChannelResponse* response)
{
    UA_SecureChannel* channel = conn->channel;
    if (!channel)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* if no security token is already issued */
    if (channel->nextSecurityToken.tokenId == 0)
    {
        channel->nextSecurityToken.channelId = channel->securityToken.channelId;
        channel->nextSecurityToken.tokenId = cm->lastTokenId++;
        channel->nextSecurityToken.createdAt = UA_DateTime_now();
        channel->nextSecurityToken.revisedLifetime =
            (request->requestedLifetime > cm->server->config.maxSecurityTokenLifetime) ?
                cm->server->config.maxSecurityTokenLifetime : request->requestedLifetime;
        if (channel->nextSecurityToken.revisedLifetime == 0) /* lifetime 0 -> return the max lifetime */
            channel->nextSecurityToken.revisedLifetime = cm->server->config.maxSecurityTokenLifetime;
    }

    /* invalidate the old nonce */
    if (channel->clientNonce.data)
    {
        UA_ByteString_deleteMembers(&channel->clientNonce);
    }
    if (channel->serverNonce.data)
    {
        UA_ByteString_deleteMembers(&channel->serverNonce);
    }

    /* set the response */
    UA_ByteString_copy(&request->clientNonce, &channel->clientNonce);
    UA_SecureChannel_generateNonce(&channel->serverNonce, channel->securityPolicy);
    UA_ByteString_copy(&channel->serverNonce, &response->serverNonce);
    UA_ChannelSecurityToken_copy(&channel->nextSecurityToken, &response->securityToken);

    /* reset the creation date to the monotonic clock */
    channel->nextSecurityToken.createdAt = UA_DateTime_nowMonotonic();

    return UA_STATUSCODE_GOOD;
}

UA_SecureChannel*
UA_SecureChannelManager_get(UA_SecureChannelManager* cm, UA_UInt32 channelId)
{
    channel_list_entry* entry;
    LIST_FOREACH(entry, &cm->channels, pointers)
    {
        if (entry->channel.securityToken.channelId == channelId)
            return &entry->channel;
    }
    return NULL;
}

UA_StatusCode
UA_SecureChannelManager_close(UA_SecureChannelManager* cm, UA_UInt32 channelId)
{
    channel_list_entry* entry;
    LIST_FOREACH(entry, &cm->channels, pointers)
    {
        if (entry->channel.securityToken.channelId == channelId)
            break;
    }
    if (!entry)
        return UA_STATUSCODE_BADINTERNALERROR;
    removeSecureChannel(cm, entry);
    return UA_STATUSCODE_GOOD;
}
