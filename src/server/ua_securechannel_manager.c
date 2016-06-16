#include "ua_securechannel_manager.h"
#include "ua_session.h"
#include "ua_server_internal.h"

#define STARTCHANNELID 1
#define STARTTOKENID 1

UA_StatusCode
UA_SecureChannelManager_init(UA_SecureChannelManager *cm, UA_Server *server) {
    LIST_INIT(&cm->channels);
    cm->lastChannelId = STARTCHANNELID;
    cm->lastTokenId = STARTTOKENID;
    cm->currentChannelCount = 0;
    cm->server = server;
    return UA_STATUSCODE_GOOD;
}

void UA_SecureChannelManager_deleteMembers(UA_SecureChannelManager *cm) {
    channel_list_entry *entry, *temp;
    LIST_FOREACH_SAFE(entry, &cm->channels, pointers, temp) {
        LIST_REMOVE(entry, pointers);
        UA_SecureChannel_deleteMembersCleanup(&entry->channel);
        UA_free(entry);
    }
}

static void removeSecureChannel(UA_SecureChannelManager *cm, channel_list_entry *entry){
    LIST_REMOVE(entry, pointers);
    UA_SecureChannel_deleteMembersCleanup(&entry->channel);
#ifndef UA_ENABLE_MULTITHREADING
    cm->currentChannelCount--;
    UA_free(entry);
#else
    cm->currentChannelCount = uatomic_add_return(&cm->currentChannelCount, -1);
    UA_Server_delayedFree(cm->server, entry);
#endif
}
/* remove channels that were not renewed or who have no connection attached */
void UA_SecureChannelManager_cleanupTimedOut(UA_SecureChannelManager *cm, UA_DateTime now) {
    channel_list_entry *entry, *temp;
    LIST_FOREACH_SAFE(entry, &cm->channels, pointers, temp) {
        UA_DateTime timeout =
            entry->channel.securityToken.createdAt +
            (UA_DateTime)(entry->channel.securityToken.revisedLifetime * UA_MSEC_TO_DATETIME);
        if(timeout < now || !entry->channel.connection) {
            UA_LOG_DEBUG_CHANNEL(cm->server->config.logger, (&entry->channel), "SecureChannel has timed out");
            removeSecureChannel(cm, entry);
        } else if(entry->channel.nextSecurityToken.tokenId > 0) {
            UA_SecureChannel_revolveTokens(&entry->channel);
        }
    }
}

/* remove the first channel that has no session attached */
static UA_Boolean purgeFirstChannelWithoutSession(UA_SecureChannelManager *cm) {
    channel_list_entry *entry;
    LIST_FOREACH(entry, &cm->channels, pointers) {
        if(LIST_EMPTY(&(entry->channel.sessions))){
            UA_LOG_DEBUG_CHANNEL(cm->server->config.logger, (&entry->channel), "Channel was purged since maxSecureChannels was reached and channel had no session attached");
            removeSecureChannel(cm, entry);
            UA_assert(entry != LIST_FIRST(&cm->channels));
            return true;
        }
    }
    return false;
}

UA_StatusCode
UA_SecureChannelManager_open(UA_SecureChannelManager *cm, UA_Connection *conn,
                             const UA_OpenSecureChannelRequest *request,
                             UA_OpenSecureChannelResponse *response) {
    if(request->securityMode != UA_MESSAGESECURITYMODE_NONE)
        return UA_STATUSCODE_BADSECURITYMODEREJECTED;
    //check if there exists a free SC, otherwise try to purge one SC without a session
    //the purge has been introduced to pass CTT, it is not clear what strategy is expected here
    if(cm->currentChannelCount >= cm->server->config.maxSecureChannels && !purgeFirstChannelWithoutSession(cm)){
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    channel_list_entry *entry = UA_malloc(sizeof(channel_list_entry));
    if(!entry)
        return UA_STATUSCODE_BADOUTOFMEMORY;
#ifndef UA_ENABLE_MULTITHREADING
    cm->currentChannelCount++;
#else
    cm->currentChannelCount = uatomic_add_return(&cm->currentChannelCount, 1);
#endif

    UA_SecureChannel_init(&entry->channel);
    response->responseHeader.stringTableSize = 0;
    response->responseHeader.timestamp = UA_DateTime_now();
    response->serverProtocolVersion = 0;

    entry->channel.securityToken.channelId = cm->lastChannelId++;
    entry->channel.securityToken.tokenId = cm->lastTokenId++;
    entry->channel.securityToken.createdAt = UA_DateTime_now();
    entry->channel.securityToken.revisedLifetime =
        (request->requestedLifetime > cm->server->config.maxSecurityTokenLifetime) ?
        cm->server->config.maxSecurityTokenLifetime : request->requestedLifetime;
    /* lifetime 0 -> set the maximum possible */
    if(entry->channel.securityToken.revisedLifetime == 0)
        entry->channel.securityToken.revisedLifetime = cm->server->config.maxSecurityTokenLifetime;

    UA_ByteString_copy(&request->clientNonce, &entry->channel.clientNonce);
    entry->channel.serverAsymAlgSettings.securityPolicyUri = UA_STRING_ALLOC(
            "http://opcfoundation.org/UA/SecurityPolicy#None");

    UA_SecureChannel_generateNonce(&entry->channel.serverNonce);
    UA_ByteString_copy(&entry->channel.serverNonce, &response->serverNonce);
    UA_ChannelSecurityToken_copy(&entry->channel.securityToken,
            &response->securityToken);

    UA_Connection_attachSecureChannel(conn, &entry->channel);
    LIST_INSERT_HEAD(&cm->channels, entry, pointers);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_SecureChannelManager_renew(UA_SecureChannelManager *cm, UA_Connection *conn,
                              const UA_OpenSecureChannelRequest *request,
                              UA_OpenSecureChannelResponse *response) {
    UA_SecureChannel *channel = conn->channel;
    if(!channel)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* if no security token is already issued */
    if(channel->nextSecurityToken.tokenId == 0) {
        channel->nextSecurityToken.channelId = channel->securityToken.channelId;
        channel->nextSecurityToken.tokenId = cm->lastTokenId++;
        channel->nextSecurityToken.createdAt = UA_DateTime_now();
        channel->nextSecurityToken.revisedLifetime =
            (request->requestedLifetime > cm->server->config.maxSecurityTokenLifetime) ?
            cm->server->config.maxSecurityTokenLifetime : request->requestedLifetime;
        /* lifetime 0 -> return the max lifetime */
        if(channel->nextSecurityToken.revisedLifetime == 0)
            channel->nextSecurityToken.revisedLifetime = cm->server->config.maxSecurityTokenLifetime;
    }

    if(channel->clientNonce.data)
        UA_ByteString_deleteMembers(&channel->clientNonce);

    UA_ByteString_copy(&request->clientNonce, &channel->clientNonce);
    UA_ByteString_copy(&channel->serverNonce, &response->serverNonce);
    UA_ChannelSecurityToken_copy(&channel->nextSecurityToken, &response->securityToken);
    return UA_STATUSCODE_GOOD;
}

UA_SecureChannel * UA_SecureChannelManager_get(UA_SecureChannelManager *cm, UA_UInt32 channelId) {
    channel_list_entry *entry;
    LIST_FOREACH(entry, &cm->channels, pointers) {
        if(entry->channel.securityToken.channelId == channelId)
            return &entry->channel;
    }
    return NULL;
}

UA_StatusCode UA_SecureChannelManager_close(UA_SecureChannelManager *cm, UA_UInt32 channelId) {
    channel_list_entry *entry;
    LIST_FOREACH(entry, &cm->channels, pointers) {
        if(entry->channel.securityToken.channelId == channelId)
            break;
    }
    if(!entry)
        return UA_STATUSCODE_BADINTERNALERROR;

    LIST_REMOVE(entry, pointers);
    UA_SecureChannel_deleteMembersCleanup(&entry->channel);
#ifndef UA_ENABLE_MULTITHREADING
    cm->currentChannelCount--;
    UA_free(entry);
#else
    cm->currentChannelCount = uatomic_add_return(&cm->currentChannelCount, -1);
    UA_Server_delayedFree(cm->server, entry);
#endif
    return UA_STATUSCODE_GOOD;
}
