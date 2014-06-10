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

#endif /* UA_STACK_SESSION_H_ */


typedef struct UA_SessionType *UA_Session;


UA_Boolean UA_Session_compare(UA_Session session1, UA_Session session2);
UA_Int32 UA_Session_getId(UA_Session session, UA_NodeId *sessionId);
UA_Int32 UA_Session_getChannel(UA_Session session, SL_secureChannel *channel);

UA_Int32 UA_Session_new(UA_Session *newSession);
