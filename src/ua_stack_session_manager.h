/*
 * ua_stack_session_manager.h
 *
 *  Created on: 05.06.2014
 *      Author: root
 */

#ifndef UA_STACK_SESSION_MANAGER_H_
#define UA_STACK_SESSION_MANAGER_H_

#include "include/opcua.h"
#include "include/ua_list.h"
#include "ua_stack_session.h"



//hide internal data of channelManager
typedef struct UA_SessionManagerType *UA_SessionManager;


UA_Int32 UA_SessionManager_init(UA_UInt32 maxSessionCount,UA_UInt32 tokenLifetime, UA_UInt32 startChannelId, UA_UInt32 startTokenId, UA_String *endpointUrl);
UA_Int32 UA_SessionManager_addSession(SL_secureChannel *channel);
UA_Int32 UA_SessionManager_removeSession(UA_Int32 channelId);

UA_Int32 UA_SessionManager_getSession(UA_UInt32 sessionId, UA_Session *session);
UA_Int32 UA_SessionManager_updateSessions();

UA_Int32 UA_SessionManager_getSessionLifeTime(UA_DateTime *lifeTime);

UA_Int32 UA_SessionManager_generateToken(SL_secureChannel channel, UA_Int32 requestedLifeTime, SecurityTokenRequestType requestType, UA_ChannelSecurityToken* newToken);

UA_Int32 UA_SessionManager_generateSessionId(UA_UInt32 *newChannelId);

#endif /* UA_STACK_SESSION_MANAGER_H_ */
