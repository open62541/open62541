#ifndef UA_SESSION_MANAGER_H_
#define UA_SESSION_MANAGER_H_

#include "ua_server.h"
#include "ua_util.h"
#include "ua_session.h"

typedef struct UA_SessionManager {
    LIST_HEAD(session_list, session_list_entry) sessions; // doubly-linked list of sessions
    UA_UInt32    maxSessionCount;
    UA_Int32     lastSessionId;
    UA_UInt32    currentSessionCount;
    UA_DateTime  maxSessionLifeTime;
    UA_DateTime  sessionTimeout;
} UA_SessionManager;

UA_StatusCode UA_SessionManager_init(UA_SessionManager *sessionManager, UA_UInt32 maxSessionCount,
                                    UA_UInt32 sessionLifetime, UA_UInt32 startSessionId);

void UA_SessionManager_deleteMembers(UA_SessionManager *sessionManager);

UA_StatusCode UA_SessionManager_createSession(UA_SessionManager *sessionManager,
                                              UA_SecureChannel *channel, UA_Session **session);

UA_StatusCode UA_SessionManager_removeSession(UA_SessionManager *sessionManager,
                                              UA_NodeId         *sessionId);

/** Finds the session which is identified by the sessionId */
UA_StatusCode UA_SessionManager_getSessionById(UA_SessionManager *sessionManager,
                                               UA_NodeId *sessionId, UA_Session **session);

UA_StatusCode UA_SessionManager_getSessionByToken(UA_SessionManager *sessionManager,
                                                  UA_NodeId *token, UA_Session **session);

//UA_Int32 UA_SessionManager_updateSessions();
//UA_Int32 UA_SessionManager_generateToken(UA_Session session, UA_Int32 requestedLifeTime, SecurityTokenRequestType requestType, UA_ChannelSecurityToken* newToken);

#endif /* UA_SESSION_MANAGER_H_ */
