#include "ua_securechannel_manager.h"
#include "ua_session.h"
#include "ua_statuscodes.h"

UA_StatusCode UA_SecureChannelManager_init(UA_SecureChannelManager *cm, UA_UInt32 maxChannelCount,
                                           UA_UInt32 tokenLifetime, UA_UInt32 startChannelId,
                                           UA_UInt32 startTokenId) {
    LIST_INIT(&cm->channels);
    cm->lastChannelId      = startChannelId;
    cm->lastTokenId        = startTokenId;
    cm->maxChannelLifetime = tokenLifetime;
    cm->maxChannelCount    = maxChannelCount;
    return UA_STATUSCODE_GOOD;
}

void UA_SecureChannelManager_deleteMembers(UA_SecureChannelManager *cm) {
    channel_list_entry *current = LIST_FIRST(&cm->channels);
    while(current) {
        LIST_REMOVE(current, pointers);
        UA_SecureChannel_deleteMembersCleanup(&current->channel);
        UA_free(current);
        current = LIST_FIRST(&cm->channels);
    }
}

void UA_SecureChannelManager_cleanupTimedOut(UA_SecureChannelManager *cm, UA_DateTime now) {
    channel_list_entry *entry = LIST_FIRST(&cm->channels);
    /* remove channels that were not renewed or who have no connection attached */
    while(entry) {
        if(entry->channel.securityToken.createdAt +
            (UA_DateTime)entry->channel.securityToken.revisedLifetime*10000 > now &&
            entry->channel.connection) {
            entry = LIST_NEXT(entry, pointers);
        }else if(entry->channel.nextSecurityToken.tokenId > 0 &&
                 entry->channel.nextSecurityToken.createdAt +
                (UA_DateTime)entry->channel.nextSecurityToken.revisedLifetime*10000 > now &&
                entry->channel.connection){
            UA_SecureChannel_revolveTokens(&entry->channel);
            entry = LIST_NEXT(entry, pointers);
        }
        else {
            channel_list_entry *next = LIST_NEXT(entry, pointers);
            LIST_REMOVE(entry, pointers);
            UA_SecureChannel_deleteMembersCleanup(&entry->channel);
            UA_free(entry);
            entry = next;
        }
    }
}

UA_StatusCode UA_SecureChannelManager_open(UA_SecureChannelManager *cm, UA_Connection *conn,
                                           const UA_OpenSecureChannelRequest *request,
                                           UA_OpenSecureChannelResponse *response) {
    switch(request->securityMode) {
    case UA_MESSAGESECURITYMODE_NONE:
        break;
    case UA_MESSAGESECURITYMODE_INVALID:
    case UA_MESSAGESECURITYMODE_SIGN:
    case UA_MESSAGESECURITYMODE_SIGNANDENCRYPT:
    default:
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSECURITYMODEREJECTED;
        return response->responseHeader.serviceResult;
    }

    channel_list_entry *entry = UA_malloc(sizeof(channel_list_entry));
    if(!entry)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_SecureChannel_init(&entry->channel);
    response->responseHeader.stringTableSize = 0;
    response->responseHeader.timestamp       = UA_DateTime_now();
    response->serverProtocolVersion = 0;

    entry->channel.securityToken.channelId       = cm->lastChannelId++;
    entry->channel.securityToken.tokenId         = cm->lastTokenId++;
    entry->channel.securityToken.createdAt       = UA_DateTime_now();
    entry->channel.securityToken.revisedLifetime = (request->requestedLifetime > cm->maxChannelLifetime) ?
                                                   cm->maxChannelLifetime : request->requestedLifetime;
    //FIXME: pragmatic workaround to get clients requesting lifetime of 0 working
    if(entry->channel.securityToken.revisedLifetime == 0) {
        entry->channel.securityToken.revisedLifetime = cm->maxChannelLifetime;
        //FIXME: I'd log it, but there is no pointer to the logger
        // printf("Warning: client requests token lifetime of 0 in OpenSecureChannelRequest setting it to %llu\n", cm->maxChannelLifetime);
    }

    UA_ByteString_copy(&request->clientNonce, &entry->channel.clientNonce);
    entry->channel.serverAsymAlgSettings.securityPolicyUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#None");

    UA_SecureChannel_generateNonce(&entry->channel.serverNonce);
    UA_ByteString_copy(&entry->channel.serverNonce, &response->serverNonce);
    UA_ChannelSecurityToken_copy(&entry->channel.securityToken, &response->securityToken);

    UA_Connection_attachSecureChannel(conn, &entry->channel);
    LIST_INSERT_HEAD(&cm->channels, entry, pointers);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_SecureChannelManager_renew(UA_SecureChannelManager *cm, UA_Connection *conn,
                                            const UA_OpenSecureChannelRequest *request,
                                            UA_OpenSecureChannelResponse *response)
{
    UA_SecureChannel *channel = conn->channel;
    if(channel == UA_NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    //if no new security token is already issued
    if(channel->nextSecurityToken.tokenId == 0){
        channel->nextSecurityToken.channelId       = channel->securityToken.channelId;
        //FIXME: UaExpert seems not to use new the new tokenid
        channel->nextSecurityToken.tokenId         = cm->lastTokenId++;
        //channel->nextSecurityToken.tokenId         = channel->securityToken.tokenId;
        channel->nextSecurityToken.createdAt       = UA_DateTime_now();
        channel->nextSecurityToken.revisedLifetime = (request->requestedLifetime > cm->maxChannelLifetime) ?
                                                 cm->maxChannelLifetime : request->requestedLifetime;

        //FIXME: pragmatic workaround to get clients requesting lifetime of 0 working
        if(channel->nextSecurityToken.revisedLifetime == 0){
            channel->nextSecurityToken.revisedLifetime = cm->maxChannelLifetime;
            //FIXME: I'd log it, but there is no pointer to the logger
            // printf("Warning: client requests token lifetime of 0 in renewing SecureChannel setting it to %llu\n", cm->maxChannelLifetime);
        }

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
    return UA_NULL;
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
    UA_free(entry);
    return UA_STATUSCODE_GOOD;
}
