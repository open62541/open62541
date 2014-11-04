#include "ua_securechannel_manager.h"
#include "ua_session.h"
#include "ua_statuscodes.h"

struct channel_list_entry {
    UA_SecureChannel channel;
    LIST_ENTRY(channel_list_entry) pointers;
};

UA_StatusCode UA_SecureChannelManager_init(UA_SecureChannelManager *cm, UA_UInt32 maxChannelCount,
                                           UA_UInt32 tokenLifetime, UA_UInt32 startChannelId,
                                           UA_UInt32 startTokenId, UA_String *endpointUrl) {
    LIST_INIT(&cm->channels);
    cm->lastChannelId      = startChannelId;
    cm->lastTokenId        = startTokenId;
    UA_String_copy(endpointUrl, &cm->endpointUrl);
    cm->maxChannelLifetime = tokenLifetime;
    cm->maxChannelCount    = maxChannelCount;
    return UA_STATUSCODE_GOOD;
}

void UA_SecureChannelManager_deleteMembers(UA_SecureChannelManager *cm) {
    struct channel_list_entry *entry = LIST_FIRST(&cm->channels);
    while(entry) {
        LIST_REMOVE(entry, pointers);
        if(entry->channel.session)
            entry->channel.session->channel = NULL;
        if(entry->channel.connection)
            entry->channel.connection->channel = NULL;
        UA_SecureChannel_deleteMembers(&entry->channel);
        UA_free(entry);
        entry = LIST_FIRST(&cm->channels);
    }
    UA_String_deleteMembers(&cm->endpointUrl);
}

UA_StatusCode UA_SecureChannelManager_open(UA_SecureChannelManager           *cm,
                                           UA_Connection                     *conn,
                                           const UA_OpenSecureChannelRequest *request,
                                           UA_OpenSecureChannelResponse      *response) {
    struct channel_list_entry *entry = UA_alloc(sizeof(struct channel_list_entry));
    if(!entry)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_SecureChannel_init(&entry->channel);

    entry->channel.connection = conn;
    entry->channel.securityToken.channelId       = cm->lastChannelId++;
    entry->channel.securityToken.tokenId         = cm->lastTokenId++;
    entry->channel.securityToken.createdAt       = UA_DateTime_now();
    entry->channel.securityToken.revisedLifetime =
        request->requestedLifetime > cm->maxChannelLifetime ?
        cm->maxChannelLifetime : request->requestedLifetime;

    switch(request->securityMode) {
    case UA_MESSAGESECURITYMODE_INVALID:
        printf("UA_SecureChannel_processOpenRequest - client demands invalid \n");
        break;

    case UA_MESSAGESECURITYMODE_NONE:
        UA_ByteString_copy(&request->clientNonce, &entry->channel.clientNonce);
        break;

    case UA_MESSAGESECURITYMODE_SIGN:
    case UA_MESSAGESECURITYMODE_SIGNANDENCRYPT:
        printf("UA_SecureChannel_processOpenRequest - client demands signed & encrypted \n");
        //TODO check if senderCertificate and ReceiverCertificateThumbprint are present
        break;
    }

    UA_String_copycstring("http://opcfoundation.org/UA/SecurityPolicy#None",
                          (UA_String *)&entry->channel.serverAsymAlgSettings.securityPolicyUri);
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
    if(channel == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    channel->securityToken.tokenId         = cm->lastTokenId++;
    channel->securityToken.createdAt       = UA_DateTime_now(); // todo: is wanted?
    channel->securityToken.revisedLifetime = request->requestedLifetime > cm->maxChannelLifetime ?
                                             cm->maxChannelLifetime : request->requestedLifetime;

    UA_SecureChannel_generateNonce(&channel->serverNonce);
    UA_ByteString_copy(&channel->serverNonce, &response->serverNonce);
    UA_ChannelSecurityToken_copy(&channel->securityToken, &response->securityToken);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_SecureChannelManager_get(UA_SecureChannelManager *cm, UA_UInt32 channelId,
                                          UA_SecureChannel **channel) {
    struct channel_list_entry *entry;
    LIST_FOREACH(entry, &cm->channels, pointers) {
        if(entry->channel.securityToken.channelId == channelId) {
            *channel = &entry->channel;
            return UA_STATUSCODE_GOOD;
        }
    }
    *channel = NULL;
    return UA_STATUSCODE_BADINTERNALERROR;
}

UA_StatusCode UA_SecureChannelManager_close(UA_SecureChannelManager *cm, UA_UInt32 channelId) {
    //TODO lock access
    // TODO: close the binaryconnection if it is still open. So we dö not have stray pointers..
    struct channel_list_entry *entry;
    LIST_FOREACH(entry, &cm->channels, pointers) {
        if(entry->channel.securityToken.channelId == channelId) {
            if(entry->channel.connection)
                entry->channel.connection->channel = NULL; // remove pointer back to the channel
            if(entry->channel.session)
                entry->channel.session->channel = NULL; // remove ponter back to the channel
            UA_SecureChannel_deleteMembers(&entry->channel);
            LIST_REMOVE(entry, pointers);
            UA_free(entry);
            return UA_STATUSCODE_GOOD;
        }
    }
    //TODO notify server application that secureChannel has been closed part 6 - §7.1.4
    return UA_STATUSCODE_BADINTERNALERROR;
}
