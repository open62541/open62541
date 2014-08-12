/*
 * ua_stack_channel_manager.h
 *
 *  Created on: 09.05.2014
 *      Author: root
 */

#ifndef UA_STACK_CHANNEL_MANAGER_H_
#define UA_STACK_CHANNEL_MANAGER_H_

#include "ua_stack_channel.h"







UA_Int32 SL_ChannelManager_init(UA_UInt32 maxChannelCount,UA_UInt32 tokenLifetime, UA_UInt32 startChannelId, UA_UInt32 startTokenId, UA_String *endpointUrl);
UA_Int32 SL_ChannelManager_addChannel(SL_Channel *channel);
//UA_Int32 SL_ChannelManager_renewChannelToken(UA_Int32 channelId, UA_DateTime requestedLifeTime);
//UA_Int32 SL_ChannelManager_createChannelToken(SL_secureChannel channel);
//UA_Int32 SL_ChannelManager_renewChannelToken(SL_secureChannel channel);

//UA_Int32 SL_ChannelManager_bindChannel(UA_Int32 channelId, TL_Connection *connection);
UA_Int32 SL_ChannelManager_removeChannel(UA_Int32 channelId);
UA_Int32 SL_ChannelManager_getChannel(UA_UInt32 channelId, SL_Channel *channel);



UA_Int32 SL_ChannelManager_getChannelLifeTime(UA_DateTime *lifeTime);

UA_Int32 SL_ChannelManager_generateToken(SL_Channel channel, UA_Int32 requestedLifeTime, SecurityTokenRequestType requestType, UA_ChannelSecurityToken* newToken);

UA_Int32 SL_ChannelManager_generateChannelId(UA_UInt32 *newChannelId);


#endif /* UA_STACK_CHANNEL_MANAGER_H_ */
