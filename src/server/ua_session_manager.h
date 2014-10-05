#ifndef UA_SESSION_MANAGER_H_
#define UA_SESSION_MANAGER_H_

#include "ua_server.h"
#include "ua_session.h"

UA_Int32 UA_SessionManager_new(UA_SessionManager **sessionManager, UA_UInt32 maxSessionCount,
                               UA_UInt32 sessionLifetime, UA_UInt32 startSessionId);

UA_Int32 UA_SessionManager_delete(UA_SessionManager *sessionManager);

UA_StatusCode UA_SessionManager_createSession(UA_SessionManager *sessionManager,
                                              UA_SecureChannel *channel, UA_Session **session);

UA_Int32 UA_SessionManager_removeSession(UA_SessionManager *sessionManager,
                                         UA_NodeId         *sessionId);

/**
 * @brief finds the session which is identified by the sessionId
 * @param sessionId the session id is used to identify the unknown session
 * @param session the session object is returned if no error occurs
 * @return error code
 */
UA_Int32 UA_SessionManager_getSessionById(UA_SessionManager *sessionManager,
                                          UA_NodeId *sessionId, UA_Session **session);

/**
 * @param token authentication token which is used to get the session object
 * @param session output, session object which is identified by the authentication token
 * @return error code
 */
UA_Int32 UA_SessionManager_getSessionByToken(UA_SessionManager *sessionManager,
                                             UA_NodeId *token, UA_Session **session);

//UA_Int32 UA_SessionManager_updateSessions();
//UA_Int32 UA_SessionManager_generateToken(UA_Session session, UA_Int32 requestedLifeTime, SecurityTokenRequestType requestType, UA_ChannelSecurityToken* newToken);

#endif /* UA_SESSION_MANAGER_H_ */
