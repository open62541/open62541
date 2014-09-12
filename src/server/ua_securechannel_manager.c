#include "ua_securechannel_manager.h"
#include "util/ua_util.h"

struct channel_list_entry {
    UA_SecureChannel channel;
    LIST_ENTRY(channel_list_entry) pointers;
};

struct UA_SecureChannelManager {
    UA_Int32    maxChannelCount;
    UA_DateTime maxChannelLifetime;
    LIST_HEAD(channel_list, channel_list_entry) channels;
    UA_MessageSecurityMode securityMode;
    UA_String   endpointUrl;
    UA_DateTime channelLifeTime;
    UA_Int32    lastChannelId;
    UA_UInt32   lastTokenId;
};

UA_Int32 UA_SecureChannelManager_new(UA_SecureChannelManager **cm, UA_UInt32 maxChannelCount,
                                     UA_UInt32 tokenLifetime, UA_UInt32 startChannelId,
                                     UA_UInt32 startTokenId, UA_String *endpointUrl) {
    UA_alloc((void **)cm, sizeof(UA_SecureChannelManager));
    UA_SecureChannelManager *channelManager = *cm;
    LIST_INIT(&channelManager->channels);
    channelManager->lastChannelId      = startChannelId;
    channelManager->lastTokenId        = startTokenId;
    UA_String_copy(endpointUrl, &channelManager->endpointUrl);
    channelManager->maxChannelLifetime = tokenLifetime;
    channelManager->maxChannelCount    = maxChannelCount;
    return UA_SUCCESS;
}

UA_Int32 UA_SecureChannelManager_delete(UA_SecureChannelManager *cm) {
    struct channel_list_entry *entry;
    LIST_FOREACH(entry, &cm->channels, pointers) {
        // deleting a securechannel means closing the connection
        // delete the binaryconnction beforehand. so there is no pointer
        // todo: unbind entry->channel.connection;
        LIST_REMOVE(entry, pointers);
        UA_SecureChannel_deleteMembers(&entry->channel);
        UA_free(entry);
    }
    UA_String_deleteMembers(&cm->endpointUrl);
    UA_free(cm);
    return UA_SUCCESS;
}

UA_Int32 UA_SecureChannelManager_open(UA_SecureChannelManager           *cm,
                                      UA_Connection                     *conn,
                                      const UA_OpenSecureChannelRequest *request,
                                      UA_OpenSecureChannelResponse      *response) {
    struct channel_list_entry *entry;
    UA_alloc((void **)&entry, sizeof(struct channel_list_entry));
    UA_SecureChannel_init(&entry->channel);

    entry->channel.connection = conn;
    entry->channel.securityToken.channelId       = cm->lastChannelId++;
    entry->channel.securityToken.tokenId         = cm->lastTokenId++;
    entry->channel.securityToken.createdAt       = UA_DateTime_now();
    entry->channel.securityToken.revisedLifetime =
        request->requestedLifetime > cm->maxChannelLifetime ?
        cm->maxChannelLifetime : request->requestedLifetime;

    switch(request->securityMode) {
    case UA_SECURITYMODE_INVALID:
        printf("UA_SecureChannel_processOpenRequest - client demands invalid \n");
        break;

    case UA_SECURITYMODE_NONE:
        UA_ByteString_copy(&request->clientNonce, &entry->channel.clientNonce);
        break;

    case UA_SECURITYMODE_SIGNANDENCRYPT:
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

    return UA_SUCCESS;
}

UA_Int32 UA_SecureChannelManager_renew(UA_SecureChannelManager           *cm,
                                       UA_Connection                     *conn,
                                       const UA_OpenSecureChannelRequest *request,
                                       UA_OpenSecureChannelResponse      *response) {

    UA_SecureChannel *channel = conn->channel;
    if(channel == UA_NULL)
        return UA_ERROR;

    channel->securityToken.tokenId         = cm->lastTokenId++;
    channel->securityToken.createdAt       = UA_DateTime_now(); // todo: is wanted?
    channel->securityToken.revisedLifetime = request->requestedLifetime > cm->maxChannelLifetime ?
                                             cm->maxChannelLifetime : request->requestedLifetime;

    UA_SecureChannel_generateNonce(&channel->serverNonce);
    UA_ByteString_copy(&channel->serverNonce, &response->serverNonce);
    UA_ChannelSecurityToken_copy(&channel->securityToken, &response->securityToken);

    return UA_SUCCESS;
}

UA_Int32 UA_SecureChannelManager_get(UA_SecureChannelManager *cm, UA_UInt32 channelId,
                                     UA_SecureChannel **channel) {
    struct channel_list_entry *entry;
    LIST_FOREACH(entry, &cm->channels, pointers) {
        if(entry->channel.securityToken.channelId == channelId) {
            *channel = &entry->channel;
            return UA_SUCCESS;
        }
    }
    *channel = UA_NULL;
    return UA_ERROR;
}

UA_Int32 UA_SecureChannelManager_close(UA_SecureChannelManager *cm, UA_UInt32 channelId) {
    //TODO lock access
    // TODO: close the binaryconnection if it is still open. So we dö not have stray pointers..
    struct channel_list_entry *entry;
    LIST_FOREACH(entry, &cm->channels, pointers) {
        if(entry->channel.securityToken.channelId == channelId) {
            UA_SecureChannel_deleteMembers(&entry->channel);
            LIST_REMOVE(entry, pointers);
            UA_free(entry);
            return UA_SUCCESS;
        }
    }
    //TODO notify server application that secureChannel has been closed part 6 - §7.1.4
    return UA_ERROR;
}
