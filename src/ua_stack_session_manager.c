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
	sessionManager->maxSessionCount = maxSessionCount;
	sessionManager->lastSessionId = startSessionId;
	sessionManager->sessionLifetime = sessionLifetime;
	return retval;
}
UA_Int32 UA_SessionManager_addSession(UA_Session *session)
{


	if(UA_SessionManager_getSession())
	UA_list_addElementToBack(sessionManager->sessions,session);
}
UA_Int32 UA_SessionManager_removeSession(UA_Int32 channelId);

UA_Int32 UA_SessionManager_getSession(UA_UInt32 sessionId, UA_Session *session);

UA_Int32 UA_SessionManager_updateSessions();

UA_Int32 UA_SessionManager_getSessionLifeTime(UA_DateTime *lifeTime);

UA_Int32 SL_UA_SessionManager_generateToken(SL_secureChannel channel, UA_Int32 requestedLifeTime, SecurityTokenRequestType requestType, UA_ChannelSecurityToken* newToken);

UA_Int32 SL_UA_SessionManager_generateSessionId(UA_UInt32 *newChannelId);
