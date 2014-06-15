/*
 * ua_stack_session_manager.c
 *
 *  Created on: 05.06.2014
 *      Author: root
 */

#include "ua_stack_session_manager.h"


typedef struct UA_SessionManagerType
{
	UA_list_List *sessions;


	UA_UInt32 maxSessionCount;
	UA_Int32 lastSessionId;
	UA_UInt32 currentSessionCount;
	UA_DateTime maxSessionLifeTime;

	UA_DateTime sessionLifetime;
}UA_SessionManagerType;

static UA_SessionManagerType *sessionManager;
UA_Int32 UA_SessionManager_init(UA_UInt32 maxSessionCount,UA_UInt32 sessionLifetime, UA_UInt32 startSessionId)
{
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_alloc((void**)&sessionManager,sizeof(UA_SessionManagerType));

	retval |= UA_list_init(sessionManager->sessions);

	sessionManager->maxSessionCount = maxSessionCount;
	sessionManager->lastSessionId = startSessionId;
	sessionManager->sessionLifetime = sessionLifetime;
	return retval;
}

UA_Boolean UA_SessionManager_sessionExists(UA_Session *session)
{
	if(UA_list_search(sessionManager->sessions,(UA_list_PayloadComparer)UA_Session_compare,(void*)session))
	{
		return UA_TRUE;
	}
	return UA_FALSE;
}

UA_Int32 UA_SessionManager_getSessionById(UA_NodeId *sessionId, UA_Session *session)
{
 	UA_list_Element* current = sessionManager->sessions->first;
	while (current)
	{
		if (current->payload)
		{
			UA_list_Element* elem = (UA_list_Element*) current;
			*session = *((UA_Session*) (elem->payload));
		 	if(UA_Session_compareById(*session,sessionId))
		 	{
		 		return UA_SUCCESS;
		 	}
		}
	}
	*session = UA_NULL;
	return UA_ERROR;
}

UA_Int32 UA_SessionManager_getSessionByToken(UA_NodeId *token, UA_Session *session)
{
	if(sessionManager->sessions)
	{
		UA_list_Element* current = sessionManager->sessions->first;
		while (current)
		{
			if (current->payload)
			{
				UA_list_Element* elem = (UA_list_Element*) current;
				*session = *((UA_Session*) (elem->payload));
				if(UA_Session_compareByToken(*session,token))
				{
					return UA_SUCCESS;
				}
			}
		}
	}
	*session = UA_NULL;
	return UA_ERROR;
}

UA_Int32 UA_SessionManager_addSession(UA_Session *session)
{
	UA_Int32 retval = UA_SUCCESS;
	if(!UA_SessionManager_sessionExists(session))
	{
		retval |= UA_list_addElementToBack(sessionManager->sessions,(void*)session);
		return retval;
	}
	else
	{
		printf("UA_SessionManager_addSession - session already in list");
		return UA_ERROR;
	}
}


/*UA_Int32 UA_SessionManager_removeSession(UA_Int32 channelId);

UA_Int32 UA_SessionManager_getSession(UA_UInt32 sessionId, UA_Session *session);

UA_Int32 UA_SessionManager_updateSessions();

UA_Int32 UA_SessionManager_getSessionLifeTime(UA_DateTime *lifeTime);

UA_Int32 SL_UA_SessionManager_generateToken(SL_secureChannel channel, UA_Int32 requestedLifeTime, SecurityTokenRequestType requestType, UA_ChannelSecurityToken* newToken);

UA_Int32 SL_UA_SessionManager_generateSessionId(UA_UInt32 *newChannelId);
*/
