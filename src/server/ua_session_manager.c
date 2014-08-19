#include "ua_session_manager.h"

struct UA_SessionManager {
	UA_list_List sessions;
	UA_UInt32 maxSessionCount;
	UA_Int32 lastSessionId;
	UA_UInt32 currentSessionCount;
	UA_DateTime maxSessionLifeTime;
	UA_DateTime sessionTimeout;
};

static UA_SessionManager *sessionManager;

UA_Int32 UA_SessionManager_generateSessionId(UA_NodeId *sessionId) {
	sessionId->namespaceIndex = 0;
	sessionId->identifierType = UA_NODEIDTYPE_NUMERIC;
	sessionId->identifier.numeric = sessionManager->lastSessionId++;
	return UA_SUCCESS;
}

UA_Int32 UA_SessionManager_init(UA_UInt32 maxSessionCount,UA_UInt32 sessionTimeout, UA_UInt32 startSessionId) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_alloc((void**)&sessionManager,sizeof(UA_SessionManager));
	retval |= UA_list_init(&sessionManager->sessions);
	sessionManager->maxSessionCount = maxSessionCount;
	sessionManager->lastSessionId = startSessionId;
	sessionManager->sessionTimeout = sessionTimeout;
	return retval;
}

UA_Boolean UA_SessionManager_sessionExists(UA_Session *session) {
	if(sessionManager == UA_NULL)
		return UA_FALSE;

	if(UA_list_search(&sessionManager->sessions,(UA_list_PayloadComparer)UA_Session_compare,(void*)session)) {
		UA_Double pendingLifetime;
		UA_Session_getPendingLifetime(session,&pendingLifetime);

		if(pendingLifetime>0)
			return UA_TRUE;

		//timeout of session reached so remove it
		UA_NodeId sessionId;
		UA_Session_getId(session,&sessionId);
		UA_SessionManager_removeSession(&sessionId);
	}
	return UA_FALSE;
}

UA_Int32 UA_SessionManager_getSessionById(UA_NodeId *sessionId, UA_Session **session) {
	if(sessionManager == UA_NULL) {
		*session = UA_NULL;
		return UA_ERROR;
	}

	UA_list_Element* current = sessionManager->sessions.first;
	while (current) {
		if (current->payload) {
			UA_list_Element* elem = (UA_list_Element*) current;
			*session = ((UA_Session*) (elem->payload));
			if(UA_Session_compareById(*session,sessionId) == UA_EQUAL){
				UA_Double pendingLifetime;
				UA_Session_getPendingLifetime(*session, &pendingLifetime);

				if(pendingLifetime > 0)
					return UA_SUCCESS;

				//session not valid anymore -> remove it
				UA_list_removeElement(elem, (UA_list_PayloadVisitor)UA_Session_delete);
				*session = UA_NULL;
				return UA_ERROR;
			}
		}
		current = current->next;
	}
	*session = UA_NULL;
	return UA_ERROR;
}

UA_Int32 UA_SessionManager_getSessionByToken(UA_NodeId *token, UA_Session **session) {
	if(sessionManager == UA_NULL) {
		*session = UA_NULL;
		return UA_ERROR;
	}

	UA_list_Element* current = sessionManager->sessions.first;
	while (current) {
		if (current->payload) {
			UA_list_Element* elem = (UA_list_Element*) current;
			*session = ((UA_Session*) (elem->payload));

			if(UA_Session_compareByToken(*session,token) == UA_EQUAL) {
				UA_Double pendingLifetime;
				UA_Session_getPendingLifetime(*session, &pendingLifetime);

				if(pendingLifetime > 0)
					return UA_SUCCESS;

				//session not valid anymore -> remove it
				UA_list_removeElement(elem, (UA_list_PayloadVisitor)UA_Session_delete);
				*session = UA_NULL;
				return UA_ERROR;
			}
		}
		current = current->next;
	}
	*session = UA_NULL;
	return UA_ERROR;
}

UA_Int32 UA_SessionManager_addSession(UA_Session *session) {
	UA_Int32 retval = UA_SUCCESS;
	UA_NodeId sessionId;
	if(!UA_SessionManager_sessionExists(session)) {
		retval |= UA_list_addPayloadToBack(&sessionManager->sessions,(void*)session);
		UA_Session_getId(session, &sessionId);
		printf("UA_SessionManager_addSession - added session with id = %d \n",sessionId.identifier.numeric);
		printf("UA_SessionManager_addSession - current session count: %i \n",sessionManager->sessions.size);

		return retval;
	}
	printf("UA_SessionManager_addSession - session already in list");
	return UA_ERROR;
}

UA_Int32 UA_SessionManager_removeSession(UA_NodeId *sessionId) {
	UA_Int32 retval = UA_SUCCESS;
	UA_list_Element *element = UA_list_search(&sessionManager->sessions,(UA_list_PayloadComparer)UA_Session_compare,sessionId);
	if(element) {
		retval |= UA_list_removeElement(element,(UA_list_PayloadVisitor)UA_Session_delete);
		printf("UA_SessionManager_removeSession - session removed, current count: %i \n",sessionManager->sessions.size);
	}
	return retval;
}
/*
UA_Int32 UA_SessionManager_updateSessions()
{
	if(sessionManager == UA_NULL)
	{
		return UA_ERROR;
	}
	UA_list_Element* current = sessionManager->sessions.first;
	while (current)
	{
		if (current->payload)
		{
			UA_list_Element* elem = (UA_list_Element*) current;
			UA_Session *session = ((UA_Session*) (elem->payload));
			UA_Double pendingLifetime;
			UA_Session_getPendingLifetime(session, &pendingLifetime);

			if(pendingLifetime <= 0){
				UA_NodeId sessionId;
				UA_Session_getId(session,&sessionId);
				UA_SessionManager_removeSession(&sessionId);
			}
		}
		current = current->next;
	}
	return UA_SUCCESS;
}
*/

UA_Int32 UA_SessionManager_getSessionTimeout(UA_Int64 *timeout_ms) {
	if(sessionManager) {
		*timeout_ms = sessionManager->sessionTimeout;
		return UA_SUCCESS;
	}
	*timeout_ms = 0;
	return UA_ERROR;
}
