/*
 * ua_stack_session_manager.h
 *
 *  Created on: 05.06.2014
 *      Author: root
 */

#ifndef UA_STACK_SESSION_MANAGER_H_
#define UA_STACK_SESSION_MANAGER_H_



#include "ua_stack_session.h"



//hide internal data of channelManager
struct UA_SessionManager;
typedef struct UA_SessionManager UA_SessionManager;

//typedef struct UA_SessionManagerType *UA_SessionManager;

/**
 * @brief initializes the session manager
 * @param maxSessionCount maximum amount of sessions which should be allowed to be created
 * @param sessionLifetime lifetime of a session, after this time the session must be renewed
 * @param startSessionId the start id of the session identifiers, newer sessions get higher ids
 * @return error code
 */
UA_Int32 UA_SessionManager_init(UA_UInt32 maxSessionCount,UA_UInt32 sessionLifetime,
		UA_UInt32 startSessionId);

/**
 * @brief adds a session to the manager list
 * @param session session object which should be added to the manager list
 * @return error code
 */
UA_Int32 UA_SessionManager_addSession(UA_Session *session);

/**
 * @brief removes a session from the manager list
 * @param sessionId id which assign to a session
 * @return error code
 */
UA_Int32 UA_SessionManager_removeSession(UA_NodeId *sessionId);

/**
 * @brief finds the session which is identified by the sessionId
 * @param sessionId the session id is used to identify the unknown session
 * @param session the session object is returned if no error occurs
 * @return error code
 */
UA_Int32 UA_SessionManager_getSessionById(UA_NodeId *sessionId, UA_Session *session);

/**
 * @brief
 * @param token authentication token which is used to get the session object
 * @param session output, session object which is identified by the authentication token
 * @return error code
 */
UA_Int32 UA_SessionManager_getSessionByToken(UA_NodeId *token, UA_Session *session);


/**
 * @brief gets the session timeout value which should be assigned to
 * all sessions handled by the manager
 * @param timeout_ms timeout in milliseconds
 * @return error code
 */
UA_Int32 UA_SessionManager_getSessionTimeout(UA_Int64 *timeout_ms);


//UA_Int32 UA_SessionManager_updateSessions();


//UA_Int32 UA_SessionManager_generateToken(UA_Session session, UA_Int32 requestedLifeTime, SecurityTokenRequestType requestType, UA_ChannelSecurityToken* newToken);

UA_Int32 UA_SessionManager_generateSessionId(UA_NodeId *newSessionId);

#endif /* UA_STACK_SESSION_MANAGER_H_ */
