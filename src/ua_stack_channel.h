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
#include "opcua.h"
#include "ua_transport_connection.h"

typedef enum ChannelState {
	UA_SL_CHANNEL_CLOSING,
	UA_SL_CHANNEL_CLOSED,
	UA_SL_CHANNEL_OPENING,
	UA_SL_CHANNEL_OPEN
}SL_channelState;
//hide object behind typedef
typedef struct SL_Channel1 *SL_secureChannel;
typedef UA_Int32 (*SL_ChannelSecurityTokenProvider)(SL_secureChannel ,UA_Int32 , SecurityTokenRequestType, UA_ChannelSecurityToken*);
typedef UA_Int32 (*SL_ChannelIdProvider)(UA_UInt32*);

UA_Int32 SL_Channel_new(SL_secureChannel **channel,
		SL_ChannelIdProvider channelIdProvider,
		SL_ChannelSecurityTokenProvider tokenProvider,
		UA_ByteString *receiverCertificateThumbprint,
		UA_ByteString *securityPolicyUri,
		UA_ByteString *senderCertificate,
		UA_MessageSecurityMode securityMode);

UA_Int32 SL_Channel_init(SL_secureChannel channel,
		UA_ByteString *receiverCertificateThumbprint,
		UA_ByteString *securityPolicyUri,
		UA_ByteString *senderCertificate, UA_MessageSecurityMode securityMode);

UA_Int32 SL_Channel_initByRequest(SL_secureChannel channel, UA_TL_Connection1 connection, const UA_ByteString* msg,
		UA_Int32* pos);

UA_Int32 SL_Channel_initMembers(SL_secureChannel channel,
		UA_TL_Connection1 *connection,
		UA_UInt32 connectionId,
		UA_UInt32 sequenceNumber,
		UA_UInt32 requestId,
		UA_MessageSecurityMode securityMode,
		UA_ByteString *remoteNonce,
		UA_ByteString *localNonce,
		UA_ByteString *receiverCertificateThumbprint,
		UA_ByteString *securityPolicyUri,
		UA_ByteString *senderCertificate);

UA_Int32 SL_Channel_delete(SL_secureChannel channel);
UA_Int32 SL_Channel_deleteMembers(SL_secureChannel channel);
UA_Int32 SL_Channel_renewToken(SL_secureChannel channel, UA_UInt32 tokenId, UA_DateTime revisedLifetime, UA_DateTime createdAt);
UA_Int32 SL_Channel_processOpenRequest(SL_secureChannel channel,
		const UA_OpenSecureChannelRequest* request, UA_OpenSecureChannelResponse* response);
UA_Int32 SL_Channel_processCloseRequest(SL_secureChannel channel,
		const UA_CloseSecureChannelRequest* request);
UA_Int32 SL_Channel_registerTokenProvider(SL_secureChannel channel,SL_ChannelSecurityTokenProvider provider);
UA_Int32 SL_Channel_registerChannelIdProvider(SL_ChannelIdProvider provider);
UA_Int32 SL_Channel_checkRequestId(SL_secureChannel channel, UA_UInt32 requestId);

UA_Int32 SL_Channel_checkSequenceNumber(SL_secureChannel channel, UA_UInt32 sequenceNumber);
UA_Boolean SL_Channel_equal(void* channel1, void* channel2);
//getters
UA_Int32 SL_Channel_getChannelId(SL_secureChannel channel, UA_UInt32 *channelId);
UA_Int32 SL_Channel_getTokenId(SL_secureChannel channel, UA_UInt32 *tokenlId);
UA_Int32 SL_Channel_getSequenceNumber(SL_secureChannel channel, UA_UInt32 *sequenceNumber);
UA_Int32 SL_Channel_getRequestId(SL_secureChannel channel, UA_UInt32 *requestId);
UA_Int32 SL_Channel_getConnectionId(SL_secureChannel channel, UA_UInt32 *connectionId);
UA_Int32 SL_Channel_getConnection(SL_secureChannel channel, UA_TL_Connection1 *connection);
UA_Int32 SL_Channel_getState(SL_secureChannel channel, SL_channelState *channelState);
UA_Int32 SL_Channel_getLocalAsymAlgSettings(SL_secureChannel channel, UA_AsymmetricAlgorithmSecurityHeader **asymAlgSettings);
UA_Int32 SL_Channel_getRemainingLifetime(SL_secureChannel channel, UA_Int32 *lifetime);

UA_Int32 SL_Channel_getRevisedLifetime(SL_secureChannel channel, UA_UInt32 *revisedLifetime);


//setters
UA_Int32 SL_Channel_setId(SL_secureChannel channel, UA_UInt32 id);



/*
typedef struct SL_ChannelManager {
	UA_UInt32 maxChannelCount;
	UA_Int32 lastChannelId;
	UA_UInt32 currentChannelCount;
	UA_DateTime maxChannelLifeTime;
	UA_indexedList_List channels;
	UA_MessageSecurityMode securityMode;
	UA_String *endpointUrl;
}SL_ChannelManager;

UA_Int32 SL_ChannelManager_init(SL_ChannelManager *channelManager, UA_UInt32 maxChannelCount, UA_Int32 startChannelId);
UA_Int32 SL_ChannelManager_addChannel(SL_secureChannel channel);
UA_Int32 SL_ChannelManager_renewChannelToken(UA_Int32 channelId, UA_DateTime requestedLifeTime);
UA_Int32 SL_ChannelManager_bindChannel(UA_Int32 channelId, UA_TL_Connection1 connection);
UA_Int32 SL_ChannelManager_removeChannel(UA_Int32 channelId);
UA_Int32 SL_ChannelManager_getChannel(UA_Int32 channelId, SL_secureChannel *channel);
UA_Int32 SL_ChannelManager_updateChannels();

*/



#endif /* UA_STACK_CHANNEL_H_ */
