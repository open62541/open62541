/*
 * ua_stack_session.c
 *
 *  Created on: 05.06.2014
 *      Author: root
 */
#include <time.h>
#include <stdlib.h>

#include "ua_stack_session.h"
typedef struct UA_SessionStruct
{
	UA_NodeId authenticationToken;
	UA_NodeId sessionId;
	UA_String name;
	Application *application;
//	UA_list_List pendingRequests;
	SL_Channel channel;
	UA_UInt32 maxRequestMessageSize;
	UA_UInt32 maxResponseMessageSize;
	UA_Int64 timeout;
	UA_DateTime validTill;

}UA_SessionType;

/* mock up function to generate tokens for authentication */
UA_Int32 UA_Session_generateToken(UA_NodeId *newToken)
{
	//Random token generation
	UA_Int32 retval = UA_SUCCESS;
	srand(time(NULL));

	UA_Int32 i = 0;
	UA_Int32 r = 0;
	//retval |= UA_NodeId_new(newToken);

	newToken->encodingByte = 0x04; //GUID
	newToken->namespace = 0; // where else?
	newToken->identifier.guid.data1 = rand();
	r = rand();
	newToken->identifier.guid.data2 = (UA_UInt16)((r>>16) );
	r = rand();
	UA_Int32 r1 = (r>>16);
	UA_Int32 r2 = r1 && 0xFFFF;
	r2 = r2 * 1;
	newToken->identifier.guid.data3 = (UA_UInt16)((r>>16) );
	for(i=0;i<8;i++)
	{
		r = rand();
		newToken->identifier.guid.data4[i] = (UA_Byte)((r>>28) );
	}


	return retval;
}

UA_Int32 UA_Session_bind(UA_Session session, SL_Channel channel)
{

	if(channel && session)
	{
		((UA_SessionType*)session)->channel = channel;
		return UA_SUCCESS;
	}
	return UA_ERROR;
}

UA_Int32 UA_Session_new(UA_Session **newSession)
{
	UA_Int32 retval = UA_SUCCESS;
	UA_Session *session;

	retval |= UA_alloc((void**)&session,sizeof(UA_Session));

	retval |= UA_alloc((void**)session,sizeof(UA_SessionType));
	*newSession = session;
	**newSession = *session;
	//get memory for request list
	return retval;
}
UA_Int32 UA_Session_deleteMembers(UA_Session session)
{
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_NodeId_deleteMembers(&((UA_SessionType*)session)->authenticationToken);
	retval |= UA_String_deleteMembers(&((UA_SessionType*)session)->name);
	retval |= UA_NodeId_deleteMembers(&((UA_SessionType*)session)->sessionId);
	return retval;
}
UA_Int32 UA_Session_delete(UA_Session *session)
{
	UA_Int32 retval = UA_SUCCESS;
	UA_Session_deleteMembers(*session);
	retval |= UA_free((UA_SessionType*)(*session));
	return retval;
}
UA_Int32 UA_Session_init(UA_Session session, UA_String *sessionName, UA_Double requestedSessionTimeout,
		UA_UInt32 maxRequestMessageSize,
		UA_UInt32 maxResponseMessageSize,
		UA_Session_idProvider idProvider,
		UA_Int64 timeout){
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_String_copy(sessionName, &((UA_SessionType*)session)->name);
	((UA_SessionType*)session)->maxRequestMessageSize = maxRequestMessageSize;
	((UA_SessionType*)session)->maxResponseMessageSize = maxResponseMessageSize;

	UA_Session_generateToken(&((UA_SessionType*)session)->authenticationToken);

	idProvider(&((UA_SessionType*)session)->sessionId);
	((UA_SessionType*)session)->timeout = requestedSessionTimeout > timeout ? timeout : requestedSessionTimeout;

	UA_Session_updateLifetime(session);
	return retval;
}

UA_Boolean UA_Session_compare(UA_Session session1, UA_Session session2)
{
	if(session1 && session2){

		if (UA_NodeId_equal(&((UA_SessionType*)session1)->sessionId,
				&((UA_SessionType*)session2)->sessionId) == UA_EQUAL){
			return UA_TRUE;
		}
	}
	return UA_FALSE;
}

UA_Boolean UA_Session_compareByToken(UA_Session session, UA_NodeId *token)
{
	if(session && token){
		return UA_NodeId_equal(&((UA_SessionType*)session)->authenticationToken, token);
	}
	return UA_NOT_EQUAL;
}

UA_Boolean UA_Session_compareById(UA_Session session, UA_NodeId *sessionId)
{
	if(session && sessionId){
		return UA_NodeId_equal(&((UA_SessionType*)session)->sessionId, sessionId);
	}
	return UA_NOT_EQUAL;
}

UA_Int32 UA_Session_getId(UA_Session session, UA_NodeId *sessionId)
{
	if(session)
	{
		return UA_NodeId_copy(&((UA_SessionType*)session)->sessionId, sessionId);
	}
	return UA_ERROR;
}

UA_Int32 UA_Session_getToken(UA_Session session, UA_NodeId *authenticationToken)
{
	if(session)
	{
		return UA_NodeId_copy(&((UA_SessionType*)session)->authenticationToken, authenticationToken);
	}
	return UA_ERROR;
}
UA_Int32 UA_Session_updateLifetime(UA_Session session)
{
	if(session)
	{
		((UA_SessionType*)session)->validTill = UA_DateTime_now() +
				((UA_SessionType*)session)->timeout * 100000; //timeout in ms
		return UA_SUCCESS;
	}
	return UA_ERROR;
}
UA_Int32 UA_Session_getChannel(UA_Session session, SL_Channel *channel)
{
	if(session)
	{
		*channel = ((UA_SessionType*)session)->channel;
		return UA_SUCCESS;
	}
	return UA_ERROR;
}
UA_Int32 UA_Session_getPendingLifetime(UA_Session session, UA_Double *pendingLifetime_ms)
{
	if(session)
	{
		*pendingLifetime_ms = (((UA_SessionType*)session)->validTill- UA_DateTime_now() ) / 10000000; //difference in ms
		return UA_SUCCESS;
	}
	return UA_ERROR;
}

UA_Boolean UA_Session_verifyChannel(UA_Session session, SL_Channel channel)
{
	if(session && channel)
	{
		if(SL_Channel_compare(((UA_SessionType*)session)->channel, channel) == UA_TRUE) {
				return UA_TRUE;
		}
	}
	return UA_FALSE;
}
UA_Int32 UA_Session_getApplicationPointer(UA_Session session, Application** application)
{
	if(session)
	{
		*application = ((UA_SessionType*)session)->application;
		return UA_SUCCESS;
	}
	*application = UA_NULL;
	return UA_ERROR;
}

UA_Int32 UA_Session_setApplicationPointer(UA_Session session, Application* application)
{
	if(session)
	{
		((UA_SessionType*)session)->application = application;
		return UA_SUCCESS;
	}
	return UA_ERROR;
}


