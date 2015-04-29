#ifndef UA_CHANNEL_MANAGER_H_
#define UA_CHANNEL_MANAGER_H_

#include "ua_util.h"
#include "ua_server.h"
#include "ua_securechannel.h"
#include "queue.h"

typedef struct channel_list_entry {
    UA_SecureChannel channel;
    LIST_ENTRY(channel_list_entry) pointers;
} channel_list_entry;

typedef struct UA_SecureChannelManager {
    LIST_HEAD(channel_list, channel_list_entry) channels; // doubly-linked list of channels
    UA_Int32    maxChannelCount;
    UA_DateTime maxChannelLifetime;
    UA_MessageSecurityMode securityMode;
    UA_DateTime channelLifeTime;
    UA_Int32    lastChannelId;
    UA_UInt32   lastTokenId;
} UA_SecureChannelManager;

UA_StatusCode UA_SecureChannelManager_init(UA_SecureChannelManager *cm, UA_UInt32 maxChannelCount,
                                           UA_UInt32 tokenLifetime, UA_UInt32 startChannelId,
                                           UA_UInt32 startTokenId);
void UA_SecureChannelManager_deleteMembers(UA_SecureChannelManager *cm);
void UA_SecureChannelManager_cleanupTimedOut(UA_SecureChannelManager *cm, UA_DateTime now);
UA_StatusCode UA_SecureChannelManager_open(UA_SecureChannelManager *cm, UA_Connection *conn,
                                           const UA_OpenSecureChannelRequest *request,
                                           UA_OpenSecureChannelResponse *response);
UA_StatusCode UA_SecureChannelManager_renew(UA_SecureChannelManager *cm, UA_Connection *conn,
                                            const UA_OpenSecureChannelRequest *request,
                                            UA_OpenSecureChannelResponse *response);
UA_SecureChannel * UA_SecureChannelManager_get(UA_SecureChannelManager *cm, UA_UInt32 channelId);
UA_StatusCode UA_SecureChannelManager_close(UA_SecureChannelManager *cm, UA_UInt32 channelId);

#endif /* UA_CHANNEL_MANAGER_H_ */
