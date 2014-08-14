/*
 * ua_stack_channel.h
 *
 *  Created on: 09.05.2014
 *      Author: root
 */

#ifndef UA_STACK_CHANNEL_H_
#define UA_STACK_CHANNEL_H_

#include <stdio.h>
#include <memory.h> // memcpy

#include "ua_transport_connection.h"

typedef enum ChannelState {
	UA_SL_CHANNEL_CLOSING,
	UA_SL_CHANNEL_CLOSED,
	UA_SL_CHANNEL_OPENING,
	UA_SL_CHANNEL_OPEN
} SL_channelState;

//hide object behind typedef
struct SL_Channel;
typedef struct SL_Channel SL_Channel;


typedef UA_Int32 (*SL_ChannelSecurityTokenProvider)(SL_Channel, UA_Int32,
		SecurityTokenRequestType, UA_ChannelSecurityToken*);
typedef UA_Int32 (*SL_ChannelIdProvider)(UA_UInt32*);
UA_Int32 SL_Channel_new(SL_Channel **channel);

UA_Int32 SL_Channel_init(SL_Channel channel, UA_TL_Connection connection,
		SL_ChannelIdProvider channelIdProvider,
		SL_ChannelSecurityTokenProvider tokenProvider);

UA_Int32 SL_Channel_bind(SL_Channel channel, UA_TL_Connection connection);
UA_Int32 SL_Channel_setRemoteSecuritySettings(SL_Channel channel,
		UA_AsymmetricAlgorithmSecurityHeader *asymSecHeader,
		UA_SequenceHeader *sequenceHeader);
UA_Int32 SL_Channel_initLocalSecuritySettings(SL_Channel channel);

UA_Int32 SL_Channel_delete(SL_Channel *channel);
UA_Int32 SL_Channel_deleteMembers(SL_Channel channel);
UA_Int32 SL_Channel_renewToken(SL_Channel channel, UA_UInt32 tokenId,
		UA_DateTime revisedLifetime, UA_DateTime createdAt);
UA_Int32 SL_Channel_processOpenRequest(SL_Channel channel,
		const UA_OpenSecureChannelRequest* request,
		UA_OpenSecureChannelResponse* response);
UA_Int32 SL_Channel_processCloseRequest(SL_Channel channel,
		const UA_CloseSecureChannelRequest* request);
UA_Int32 SL_Channel_registerTokenProvider(SL_Channel channel,
		SL_ChannelSecurityTokenProvider provider);
UA_Int32 SL_Channel_registerChannelIdProvider(SL_ChannelIdProvider provider);
UA_Int32 SL_Channel_checkRequestId(SL_Channel channel, UA_UInt32 requestId);

UA_Int32 SL_Channel_checkSequenceNumber(SL_Channel channel,
		UA_UInt32 sequenceNumber);
UA_Boolean SL_Channel_compare(SL_Channel channel1, SL_Channel channel2);
//getters
UA_Int32 SL_Channel_getChannelId(SL_Channel channel, UA_UInt32 *channelId);
UA_Int32 SL_Channel_getTokenId(SL_Channel channel, UA_UInt32 *tokenlId);
UA_Int32 SL_Channel_getSequenceNumber(SL_Channel channel,
		UA_UInt32 *sequenceNumber);
UA_Int32 SL_Channel_getRequestId(SL_Channel channel, UA_UInt32 *requestId);
UA_Int32 SL_Channel_getConnectionId(SL_Channel channel,
		UA_UInt32 *connectionId);
UA_Int32 SL_Channel_getConnection(SL_Channel channel,
		UA_TL_Connection *connection);
UA_Int32 SL_Channel_getState(SL_Channel channel, SL_channelState *channelState);
UA_Int32 SL_Channel_getLocalAsymAlgSettings(SL_Channel channel,
		UA_AsymmetricAlgorithmSecurityHeader **asymAlgSettings);
UA_Int32 SL_Channel_getRemainingLifetime(SL_Channel channel,
		UA_Int32 *lifetime);

UA_Int32 SL_Channel_getRevisedLifetime(SL_Channel channel,
		UA_UInt32 *revisedLifetime);

//setters
UA_Int32 SL_Channel_setId(SL_Channel channel, UA_UInt32 id);

#endif /* UA_STACK_CHANNEL_H_ */
