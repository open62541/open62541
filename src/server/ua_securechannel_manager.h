#ifndef UA_CHANNEL_MANAGER_H_
#define UA_CHANNEL_MANAGER_H_

#include "ua_types_generated.h"
#include "ua_securechannel.h"

struct UA_SecureChannelManager;
typedef struct UA_SecureChannelManager UA_SecureChannelManager;

UA_Int32 UA_SecureChannelManager_new(UA_SecureChannelManager **cm, UA_UInt32 maxChannelCount,
									 UA_UInt32 tokenLifetime, UA_UInt32 startChannelId,
									 UA_UInt32 startTokenId, UA_String *endpointUrl);
UA_Int32 UA_SecureChannelManager_delete(UA_SecureChannelManager *cm);
UA_Int32 UA_SecureChannelManager_open(UA_SecureChannelManager *cm, UA_Connection *conn,
										const UA_OpenSecureChannelRequest* request,
										UA_OpenSecureChannelResponse* response);
UA_Int32 UA_SecureChannelManager_renew(UA_SecureChannelManager *cm, UA_Connection *conn,
									   const UA_OpenSecureChannelRequest* request,
									   UA_OpenSecureChannelResponse* response);
UA_Int32 UA_SecureChannelManager_get(UA_SecureChannelManager *cm, UA_UInt32 channelId,
									   UA_SecureChannel **channel);
UA_Int32 UA_SecureChannelManager_close(UA_SecureChannelManager *cm, UA_UInt32 channelId);

#endif /* UA_CHANNEL_MANAGER_H_ */
