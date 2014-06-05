/*
 * ua_stack_channel_manager.c
 *
 *  Created on: 09.05.2014
 *      Author: root
 */


#include "ua_stack_channel_manager.h"

typedef struct SL_ChannelManager {
	UA_UInt32 maxChannelCount;
	UA_Int32 lastChannelId;
	UA_UInt32 currentChannelCount;
	UA_DateTime maxChannelLifeTime;
	UA_list_List channels;
	UA_MessageSecurityMode securityMode;
	UA_String endpointUrl;
	UA_DateTime channelLifeTime;
	UA_UInt32 lastTokenId;
}SL_ChannelManager;
static SL_ChannelManager *channelManager;
/*
UA_Int32 SL_ChannelManager_newChannelByRequest(UA_Int32 connectionId, const UA_ByteString* msg,
		UA_Int32* pos, SL_secureChannel *newChannel)
{
	UA_UInt32 channelId = 0;
	SL_Channel_newChannelByRequest(connectionId, msg, pos, newChannel);

	SL_ChannelManager_addChannel(newChannel, &channelId);
	SL_Channel_setId(newChannel, channelId);
	return UA_SUCCESS;
}
*/
UA_Int32 SL_ChannelManager_init(UA_UInt32 maxChannelCount,UA_UInt32 tokenLifetime, UA_UInt32 startChannelId, UA_UInt32 startTokenId, UA_String *endpointUrl)
{
	UA_alloc((void**)&channelManager,sizeof(SL_ChannelManager));
	UA_list_init(&(channelManager->channels));
	channelManager->lastChannelId = startChannelId;
	channelManager->lastTokenId = startTokenId;
	UA_String_copy(endpointUrl,&channelManager->endpointUrl);
	channelManager->maxChannelLifeTime = tokenLifetime;
	channelManager->maxChannelCount = maxChannelCount;

	return UA_SUCCESS;
}

UA_Int32 SL_ChannelManager_addChannel(SL_secureChannel *channel)
{
	if (channelManager->maxChannelCount > channelManager->currentChannelCount)
	{

//TODO lock access (mulitthreading)------------
		UA_list_Element *element;
		UA_alloc((void**)&element, sizeof(UA_list_Element));
		UA_list_initElement(element);
		element->payload =(void*) channel;

		UA_list_addElementToBack(&channelManager->channels,element);
		return UA_SUCCESS;
		//set id in channel object which was added
//TODO lock access------------
	}
	return UA_ERROR;

}

UA_Int32 generateNewTokenId()
{
	//TODO lock accesss
	return channelManager->lastTokenId++;
}

UA_Int32 SL_ChannelManager_generateChannelId(UA_UInt32 *newChannelId)
{
	if(channelManager)
	{
		*newChannelId = channelManager->lastChannelId++;
		return UA_SUCCESS;
	}
	return UA_ERROR;
}

UA_UInt32 SL_ChannelManager_generateNewTokenId()
{
	return channelManager->lastTokenId++;
}

UA_Int32 SL_ChannelManager_generateToken(SL_secureChannel channel, UA_Int32 requestedLifeTime,
		SecurityTokenRequestType requestType,
		UA_ChannelSecurityToken* newToken)
{
	UA_UInt32 tokenId;
	if(channel){
		SL_Channel_getTokenId(channel,&tokenId);
		if(requestType==UA_SECURITYTOKEN_ISSUE)
		{
			SL_Channel_getChannelId(channel,&newToken->channelId);
			newToken->createdAt = UA_DateTime_now();
			newToken->revisedLifetime = requestedLifeTime > channelManager->maxChannelLifeTime ? channelManager->maxChannelLifeTime : requestedLifeTime;
			newToken->tokenId = SL_ChannelManager_generateNewTokenId();
			return UA_SUCCESS;
		}
		else if(requestType==UA_SECURITYTOKEN_RENEW)
		{
			SL_Channel_getChannelId(channel,&newToken->channelId);
			newToken->createdAt = UA_DateTime_now();
			newToken->revisedLifetime = requestedLifeTime > channelManager->maxChannelLifeTime ? channelManager->maxChannelLifeTime : requestedLifeTime;
			return UA_SUCCESS;
		}
		else
		{
			return UA_ERROR;
		}
	}
	return UA_ERROR;
}

UA_Int32 SL_ChannelManager_processOpenRequest(SL_secureChannel channel,
		const UA_OpenSecureChannelRequest* request, UA_OpenSecureChannelResponse* response)
{


	return UA_SUCCESS;
}

UA_Int32 SL_ChannelManager_updateChannels()
{
	//TODO lock access
	UA_Int32 channelLifetime;
	UA_UInt32 channelId;
	UA_list_Element* current = channelManager->channels.first;
	while (current)
	{
		if (current->payload)
		{
			UA_indexedList_Element* elem =
					(UA_indexedList_Element*) current->payload;
			SL_secureChannel *channel = (SL_secureChannel*) elem->payload;
			//remove channels with expired lifetime, close linked list
			if (channel)
			{
				UA_list_addPayloadToBack(&channelManager->channels,(void*)channel);
				SL_Channel_getRemainingLifetime(*channel,&channelLifetime);
				if(channelLifetime <= 0)
				{
					//removeChannel
				}
			}
			else
			{
				SL_Channel_getChannelId(*channel, &channelId);
				//not possible to remove channel
				printf(
						"UA_SL_ChannelManager_updateChannels - could not remove channel with id: %i \n",
						channelId);
				return UA_SUCCESS;
			}
		}
	}
	return UA_SUCCESS;
}

UA_Int32 SL_ChannelManager_removeChannel(UA_Int32 channelId)
{
	//TODO lock access
	SL_secureChannel channel;
	SL_ChannelManager_getChannel(channelId, &channel);
	UA_list_Element *element =  UA_list_search(&channelManager->channels, SL_Channel_equal, channel);

	if (element)
	{
		element->next->prev = element->prev;
		element->prev->next = element->next;

		SL_Channel_delete((SL_secureChannel)element->payload);
		//TODO notify server application that secureChannel has been closed part 6 - §7.1.4
	}
	//channel not found
	return UA_ERROR;
}
UA_Int32 SL_ChannelManager_getChannelLifeTime(UA_DateTime *lifeTime)
{
	*lifeTime = channelManager->channelLifeTime;
	return UA_SUCCESS;
}

/*UA_Int32 SL_ChannelManager_getChannelsByConnectionId(UA_Int32 connectionId,
		SL_secureChannel **channels, UA_Int32 *noOfChannels)
{
	return UA_SUCCESS;UA_list_Element
}
*/
UA_Int32 SL_ChannelManager_getChannel(UA_UInt32 channelId, SL_secureChannel *channel)
{
	UA_UInt32 tmpChannelId;
 	UA_list_Element* current = channelManager->channels.first;
	while (current)
	{
		if (current->payload)
		{
			UA_list_Element* elem = (UA_list_Element*) current;
			*channel = *((SL_secureChannel*) (elem->payload));
			SL_Channel_getChannelId(*channel, &tmpChannelId);
		 	if(tmpChannelId == channelId)
		 	{
		 		return UA_SUCCESS;
		 	}
		}
	}
	*channel = UA_NULL;
	return UA_ERROR;
}

