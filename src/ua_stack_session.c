/*
 * ua_stack_session.c
 *
 *  Created on: 05.06.2014
 *      Author: root
 */


#include "ua_stack_session.h"
typedef struct UA_SessionType
{
	UA_NodeId authenticationToken;
	UA_NodeId sessionId;
	void *applicationPayload;
	Application *application;
}UA_SessionType;


UA_Boolean UA_Session_compare(UA_Session session1, UA_Session session2)
{
	if(session1 && session2)
	{
		return (UA_Guid_compare(((UA_SessionType*)session1)->sessionId.identifier,((UA_SessionType*)session2)->sessionId.identifier) == 0);
	}
	return UA_FALSE;
}
