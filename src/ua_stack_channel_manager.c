#include "ua_stack_channel_manager.h"

struct SL_ChannelManager {
	UA_Int32 maxChannelCount;
	UA_Int32 lastChannelId;
	UA_DateTime maxChannelLifeTime;
	UA_list_List channels;
	UA_MessageSecurityMode securityMode;
	UA_String endpointUrl;
	UA_DateTime channelLifeTime;
	UA_UInt32 lastTokenId;
};

static SL_ChannelManager *channelManager;

UA_Int32 SL_ChannelManager_init(UA_UInt32 maxChannelCount, UA_UInt32 tokenLifetime, UA_UInt32 startChannelId,
								UA_UInt32 startTokenId, UA_String *endpointUrl) {
	UA_alloc((void**)&channelManager,sizeof(SL_ChannelManager));
	UA_list_init(&(channelManager->channels));
	channelManager->lastChannelId = startChannelId;
	channelManager->lastTokenId = startTokenId;
	UA_String_copy(endpointUrl,&channelManager->endpointUrl);
	channelManager->maxChannelLifeTime = tokenLifetime;
	channelManager->maxChannelCount = maxChannelCount;

	return UA_SUCCESS;
}

UA_Int32 SL_ChannelManager_addChannel(SL_Channel *channel) {
	if (!channelManager || (channelManager->maxChannelCount <= channelManager->channels.size))
		return UA_ERROR;

	//TODO lock access (mulitthreading)------------
	UA_list_addPayloadToBack(&channelManager->channels,(void*)channel);
	return UA_SUCCESS;
	//TODO lock access------------
}

UA_Int32 generateNewTokenId() {
	//TODO lock accesss
	return channelManager->lastTokenId++;
}

UA_Int32 SL_ChannelManager_generateChannelId(UA_UInt32 *newChannelId) {
	if(channelManager) {
		*newChannelId = channelManager->lastChannelId++;
		return UA_SUCCESS;
	}
	return UA_ERROR;
}

UA_UInt32 SL_ChannelManager_generateNewTokenId() {
	return channelManager->lastTokenId++;
}

UA_Int32 SL_ChannelManager_generateToken(SL_Channel *channel, UA_Int32 requestedLifeTime,
		SecurityTokenRequestType requestType,
		UA_ChannelSecurityToken* newToken) {
	UA_UInt32 tokenId;
	if(channel){
		SL_Channel_getTokenId(channel,&tokenId);
		if(requestType==UA_SECURITYTOKEN_ISSUE) {
			SL_Channel_getChannelId(channel,&newToken->channelId);
			newToken->createdAt = UA_DateTime_now();
			newToken->revisedLifetime = requestedLifeTime > channelManager->maxChannelLifeTime ? channelManager->maxChannelLifeTime : requestedLifeTime;
			newToken->tokenId = SL_ChannelManager_generateNewTokenId();
			return UA_SUCCESS;
		} else if(requestType==UA_SECURITYTOKEN_RENEW) {
			SL_Channel_getChannelId(channel,&newToken->channelId);
			newToken->createdAt = UA_DateTime_now();
			newToken->revisedLifetime = requestedLifeTime > channelManager->maxChannelLifeTime ? channelManager->maxChannelLifeTime : requestedLifeTime;
			return UA_SUCCESS;
		}
	}
	return UA_ERROR;
}

UA_Int32 SL_ChannelManager_removeChannel(UA_Int32 channelId) {
	//TODO lock access
	SL_Channel *channel;
	UA_Int32 retval = UA_SUCCESS;
	SL_ChannelManager_getChannel(channelId, &channel);

	UA_list_Element *element =  UA_list_search(&channelManager->channels, (UA_list_PayloadComparer)SL_Channel_compare, &channel);
	if(element) {
		retval |= UA_list_removeElement(element,(UA_list_PayloadVisitor)SL_Channel_delete);
		return retval;
	}
		//TODO notify server application that secureChannel has been closed part 6 - §7.1.4

	return UA_ERROR;
}

UA_Int32 SL_ChannelManager_getChannelLifeTime(UA_DateTime *lifeTime) {
	*lifeTime = channelManager->channelLifeTime;
	return UA_SUCCESS;
}

/*UA_Int32 SL_ChannelManager_getChannelsByConnectionId(UA_Int32 connectionId,
		SL_secureChannel **channels, UA_Int32 *noOfChannels)
{
	return UA_SUCCESS;UA_list_Element
}
*/

UA_Int32 SL_ChannelManager_getChannel(UA_UInt32 channelId, SL_Channel **channel) {
	UA_UInt32 tmpChannelId;
	if(channelManager==UA_NULL){
		*channel = UA_NULL;
		return UA_ERROR;
	}

 	UA_list_Element* current = channelManager->channels.first;
	while (current) {
		if (current->payload) {
			UA_list_Element* elem = (UA_list_Element*) current;
			*channel = ((SL_Channel*) (elem->payload));
			SL_Channel_getChannelId(*channel, &tmpChannelId);
		 	if(tmpChannelId == channelId)
		 		return UA_SUCCESS;
		}
		current = current->next;
	}
#ifdef DEBUG

//	SL_Channel c1 = *(SL_Channel*)(channelManager->channels.first->payload);
//	SL_Channel c2 = *(SL_Channel*)(channelManager->channels.last->payload);
//	UA_UInt32 id1,id2;

//	SL_Channel_getChannelId(c1,&id1);
//	SL_Channel_getChannelId(c2,&id2);
//
//	printf("SL_ChannelManager_getChannel c1: %i \n",id1);
//	printf("SL_ChannelManager_getChannel c2: %i \n",id2);
#endif
	*channel = UA_NULL;
	return UA_ERROR;
}

