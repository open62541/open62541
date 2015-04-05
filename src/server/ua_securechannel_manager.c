#include "ua_securechannel_manager.h"
#include "ua_session.h"
#include "ua_statuscodes.h"

struct channel_list_entry {
    UA_SecureChannel channel;
    LIST_ENTRY(channel_list_entry) pointers;
};

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
    struct channel_list_entry *current;
    struct channel_list_entry *next = LIST_FIRST(&cm->channels);
    while(next) {
        current = next;
        next = LIST_NEXT(current, pointers);
        LIST_REMOVE(current, pointers);
        if(current->channel.session)
            current->channel.session->channel = UA_NULL;
        if(current->channel.connection)
            current->channel.connection->channel = UA_NULL;
        UA_SecureChannel_deleteMembers(&current->channel);
        UA_free(current);
    }
}

UA_StatusCode UA_SecureChannelManager_open(UA_SecureChannelManager           *cm,
                                           UA_Connection                     *conn,
                                           const UA_OpenSecureChannelRequest *request,
                                           UA_OpenSecureChannelResponse      *response) {
    switch(request->securityMode) {
    case UA_MESSAGESECURITYMODE_INVALID:
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSECURITYMODEREJECTED;
        return response->responseHeader.serviceResult;

        // fall through and handle afterwards
    /* case UA_MESSAGESECURITYMODE_NONE: */
    /*     UA_ByteString_copy(&request->clientNonce, &entry->channel.clientNonce); */
    /*     break; */

    case UA_MESSAGESECURITYMODE_SIGN:
    case UA_MESSAGESECURITYMODE_SIGNANDENCRYPT:
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSECURITYMODEREJECTED;
        return response->responseHeader.serviceResult;

    default:
        // do nothing
        break;
    }

    struct channel_list_entry *entry = UA_malloc(sizeof(struct channel_list_entry));
    if(!entry) return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_SecureChannel_init(&entry->channel);

    response->responseHeader.stringTableSize = 0;
    response->responseHeader.timestamp       = UA_DateTime_now();

    entry->channel.connection = conn;
    conn->channel = &entry->channel;
    entry->channel.securityToken.channelId       = cm->lastChannelId++;
    entry->channel.securityToken.tokenId         = cm->lastTokenId++;
    entry->channel.securityToken.createdAt       = UA_DateTime_now();
    entry->channel.securityToken.revisedLifetime =
        request->requestedLifetime > cm->maxChannelLifetime ?
        cm->maxChannelLifetime : request->requestedLifetime;

    UA_ByteString_copy(&request->clientNonce, &entry->channel.clientNonce);
    entry->channel.serverAsymAlgSettings.securityPolicyUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#None");
    LIST_INSERT_HEAD(&cm->channels, entry, pointers);

    response->serverProtocolVersion = 0;
    UA_SecureChannel_generateNonce(&entry->channel.serverNonce);
    UA_ByteString_copy(&entry->channel.serverNonce, &response->serverNonce);
    UA_ChannelSecurityToken_copy(&entry->channel.securityToken, &response->securityToken);
    conn->channel = &entry->channel;

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_SecureChannelManager_renew(UA_SecureChannelManager           *cm,
                                            UA_Connection                     *conn,
                                            const UA_OpenSecureChannelRequest *request,
                                            UA_OpenSecureChannelResponse      *response) {
    UA_SecureChannel *channel = conn->channel;
    if(channel == UA_NULL) return UA_STATUSCODE_BADINTERNALERROR;

    channel->securityToken.tokenId         = cm->lastTokenId++;
    channel->securityToken.createdAt       = UA_DateTime_now(); // todo: is wanted?
    channel->securityToken.revisedLifetime = request->requestedLifetime > cm->maxChannelLifetime ?
                                             cm->maxChannelLifetime : request->requestedLifetime;

    if(channel->serverNonce.data != UA_NULL)
        UA_ByteString_deleteMembers(&channel->serverNonce);
    UA_SecureChannel_generateNonce(&channel->serverNonce);
    UA_ByteString_copy(&channel->serverNonce, &response->serverNonce);
    UA_ChannelSecurityToken_copy(&channel->securityToken, &response->securityToken);

    return UA_STATUSCODE_GOOD;
}

UA_SecureChannel * UA_SecureChannelManager_get(UA_SecureChannelManager *cm, UA_UInt32 channelId) {
    struct channel_list_entry *entry;
    LIST_FOREACH(entry, &cm->channels, pointers) {
        if(entry->channel.securityToken.channelId == channelId)
            return &entry->channel;
    }
    return UA_NULL;
}

UA_StatusCode UA_SecureChannelManager_close(UA_SecureChannelManager *cm, UA_UInt32 channelId) {
    //TODO lock access
    // TODO: close the binaryconnection if it is still open. So we dö not have stray pointers..
    struct channel_list_entry *entry;
    LIST_FOREACH(entry, &cm->channels, pointers) {
        if(entry->channel.securityToken.channelId == channelId) {
            if(entry->channel.connection)
                entry->channel.connection->channel = UA_NULL; // remove pointer back to the channel
            if(entry->channel.session)
                entry->channel.session->channel = UA_NULL; // remove ponter back to the channel
            UA_SecureChannel_deleteMembers(&entry->channel);
            LIST_REMOVE(entry, pointers);
            UA_free(entry);
            return UA_STATUSCODE_GOOD;
        }
    }
    //TODO notify server application that secureChannel has been closed part 6 - §7.1.4
    return UA_STATUSCODE_BADINTERNALERROR;
}
