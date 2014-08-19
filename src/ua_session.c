#include <time.h>
#include <stdlib.h>

#include "ua_session.h"

struct UA_Session {
	UA_NodeId authenticationToken;
	UA_NodeId sessionId;
	UA_String name;
	Application *application;
//	UA_list_List pendingRequests;
	SL_Channel *channel;
	UA_UInt32 maxRequestMessageSize;
	UA_UInt32 maxResponseMessageSize;
	UA_Int64 timeout;
	UA_DateTime validTill;
};

/* mock up function to generate tokens for authentication */
UA_Int32 UA_Session_generateToken(UA_NodeId *newToken) {
	//Random token generation
	UA_Int32 retval = UA_SUCCESS;
	srand(time(NULL));

	UA_Int32 i = 0;
	UA_Int32 r = 0;
	//retval |= UA_NodeId_new(newToken);

	newToken->namespaceIndex = 0; // where else?
	newToken->identifierType = UA_NODEIDTYPE_GUID;
	newToken->identifier.guid.data1 = rand();
	r = rand();
	newToken->identifier.guid.data2 = (UA_UInt16)((r>>16) );
	r = rand();
	UA_Int32 r1 = (r>>16);
	UA_Int32 r2 = r1 & 0xFFFF;
	r2 = r2 * 1;
	newToken->identifier.guid.data3 = (UA_UInt16)((r>>16) );
	for(i=0;i<8;i++) {
		r = rand();
		newToken->identifier.guid.data4[i] = (UA_Byte)((r>>28) );
	}
	return retval;
}

UA_Int32 UA_Session_bind(UA_Session *session, SL_Channel *channel) {
	if(!channel || !session)
		return UA_ERROR;

	session->channel = channel;
	return UA_SUCCESS;
}

UA_Int32 UA_Session_new(UA_Session **newSession) {
	UA_Int32 retval = UA_SUCCESS;
	UA_Session *session;

	retval |= UA_alloc((void**)&session,sizeof(UA_Session));
	retval |= UA_alloc((void**)session,sizeof(UA_Session));
	*newSession = session;
	**newSession = *session;
	//get memory for request list
	return retval;
}

UA_Int32 UA_Session_deleteMembers(UA_Session *session) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_NodeId_deleteMembers(&session->authenticationToken);
	retval |= UA_String_deleteMembers(&session->name);
	retval |= UA_NodeId_deleteMembers(&session->sessionId);
	return retval;
}

UA_Int32 UA_Session_delete(UA_Session *session) {
	UA_Int32 retval = UA_SUCCESS;
	UA_Session_deleteMembers(session);
	retval |= UA_free(session);
	return retval;
}

UA_Int32 UA_Session_init(UA_Session *session, UA_String *sessionName, UA_Double requestedSessionTimeout,
		UA_UInt32 maxRequestMessageSize,
		UA_UInt32 maxResponseMessageSize,
		UA_Session_idProvider idProvider,
		UA_Int64 timeout){
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_String_copy(sessionName, &session->name);
	session->maxRequestMessageSize = maxRequestMessageSize;
	session->maxResponseMessageSize = maxResponseMessageSize;

	UA_Session_generateToken(&session->authenticationToken);

	idProvider(&session->sessionId);
	session->timeout = requestedSessionTimeout > timeout ? timeout : requestedSessionTimeout;

	UA_Session_updateLifetime(session);
	return retval;
}

UA_Boolean UA_Session_compare(UA_Session *session1, UA_Session *session2) {
	if(session1 && session2) {
		if (UA_NodeId_equal(&session1->sessionId, &session2->sessionId) == UA_EQUAL) {
			return UA_TRUE;
		}
	}
	return UA_FALSE;
}

UA_Boolean UA_Session_compareByToken(UA_Session *session, UA_NodeId *token) {
	if(session && token) {
		return UA_NodeId_equal(&session->authenticationToken, token);
	}
	return UA_NOT_EQUAL;
}

UA_Boolean UA_Session_compareById(UA_Session *session, UA_NodeId *sessionId) {
	if(session && sessionId) {
		return UA_NodeId_equal(&session->sessionId, sessionId);
	}
	return UA_NOT_EQUAL;
}

UA_Int32 UA_Session_getId(UA_Session *session, UA_NodeId *sessionId) {
	if(!session)
		return UA_ERROR;
	return UA_NodeId_copy(&session->sessionId, sessionId);
}

UA_Int32 UA_Session_getToken(UA_Session *session, UA_NodeId *authenticationToken) {
	if(!session)
		return UA_ERROR;
	return UA_NodeId_copy(&session->authenticationToken, authenticationToken);
}

UA_Int32 UA_Session_updateLifetime(UA_Session *session) {
	if(!session)
		return UA_ERROR;

	session->validTill = UA_DateTime_now() + session->timeout * 100000; //timeout in ms
	return UA_SUCCESS;
}

UA_Int32 UA_Session_getChannel(UA_Session *session, SL_Channel **channel) {
	if(!session)
		return UA_ERROR;

	*channel = session->channel;
	return UA_SUCCESS;
}

UA_Int32 UA_Session_getPendingLifetime(UA_Session *session, UA_Double *pendingLifetime_ms) {
	if(!session)
		return UA_ERROR;

	*pendingLifetime_ms = (session->validTill- UA_DateTime_now() ) / 10000000; //difference in ms
	return UA_SUCCESS;
}

UA_Boolean UA_Session_verifyChannel(UA_Session *session, SL_Channel *channel) {
	if(!session || !channel)
		return UA_FALSE;

	return SL_Channel_compare(session->channel, channel);
}

UA_Int32 UA_Session_getApplicationPointer(UA_Session *session, Application** application) {
	if(!session) {
		*application = UA_NULL;
		return UA_ERROR;
	}

	*application = session->application;
	return UA_SUCCESS;
}

UA_Int32 UA_Session_setApplicationPointer(UA_Session *session, Application* application) {
	if(!session)
		return UA_ERROR;

	session->application = application;
	return UA_SUCCESS;
}
