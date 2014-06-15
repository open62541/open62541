/*
 * ua_stack_session.h
 *
 *  Created on: 05.06.2014
 *      Author: root
 */

#ifndef UA_STACK_SESSION_H_
#define UA_STACK_SESSION_H_

#include "../include/opcua.h"
#include "ua_stack_channel.h"




typedef struct UA_SessionType *UA_Session;

UA_Int32 UA_Session_new(UA_Session **newSession);
UA_Int32 UA_Session_init(UA_Session session, UA_String *sessionName, UA_Double requestedSessionTimeout, UA_UInt32 maxRequestMessageSize, UA_UInt32 maxResponseMessageSize);


UA_Boolean UA_Session_compare(UA_Session session1, UA_Session session2);
UA_Boolean UA_Session_compareByToken(UA_Session session, UA_NodeId *token);
UA_Boolean UA_Session_compareById(UA_Session session, UA_NodeId *sessionId);

UA_Int32 UA_Session_getId(UA_Session session, UA_NodeId *sessionId);
UA_Int32 UA_Session_getChannel(UA_Session session, SL_secureChannel *channel);

#endif /* UA_STACK_SESSION_H_ */
