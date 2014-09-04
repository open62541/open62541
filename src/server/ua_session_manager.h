#ifndef UA_SESSION_MANAGER_H_
#define UA_SESSION_MANAGER_H_

#include "ua_session.h"

struct UA_SessionManager;
typedef struct UA_SessionManager UA_SessionManager;

UA_Int32 UA_SessionManager_new(UA_SessionManager **sessionManager, UA_UInt32 maxSessionCount,
								UA_UInt32 sessionLifetime, UA_UInt32 startSessionId);

UA_Int32 UA_SessionManager_delete(UA_SessionManager *sessionManager);

UA_Int32 UA_SessionManager_addSession(UA_SessionManager *sessionManager,
									  UA_SecureChannel *channel, UA_Session **session);

/**
 * @brief removes a session from the manager list
 * @param sessionId id which assign to a session
 * @return error code
 */
UA_Int32 UA_SessionManager_removeSession(UA_SessionManager *sessionManager,
										 UA_NodeId *sessionId);

/**
 * @brief finds the session which is identified by the sessionId
 * @param sessionId the session id is used to identify the unknown session
 * @param session the session object is returned if no error occurs
 * @return error code
 */
UA_Int32 UA_SessionManager_getSessionById(UA_SessionManager *sessionManager,
										  UA_NodeId *sessionId, UA_Session **session);

/**
 * @brief
 * @param token authentication token which is used to get the session object
 * @param session output, session object which is identified by the authentication token
 * @return error code
 */
UA_Int32 UA_SessionManager_getSessionByToken(UA_SessionManager *sessionManager,
											 UA_NodeId *token, UA_Session **session);

/**
 * @brief gets the session timeout value which should be assigned to
 * all sessions handled by the manager
 * @param timeout_ms timeout in milliseconds
 * @return error code
 */
UA_Int32 UA_SessionManager_getSessionTimeout(UA_SessionManager *sessionManager,
											 UA_Int64 *timeout_ms);

//UA_Int32 UA_SessionManager_updateSessions();
//UA_Int32 UA_SessionManager_generateToken(UA_Session session, UA_Int32 requestedLifeTime, SecurityTokenRequestType requestType, UA_ChannelSecurityToken* newToken);
UA_Int32 UA_SessionManager_generateSessionId(UA_SessionManager *sessionManager,
											 UA_NodeId *newSessionId);

#endif /* UA_SESSION_MANAGER_H_ */
