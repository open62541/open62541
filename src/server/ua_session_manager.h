#ifndef UA_SESSION_MANAGER_H_
#define UA_SESSION_MANAGER_H_

#include "../deps/queue.h"
#include "ua_server.h"
#include "ua_util.h"
#include "ua_session.h"

typedef struct session_list_entry {
    LIST_ENTRY(session_list_entry) pointers;
    UA_Session session;
} session_list_entry;

typedef struct UA_SessionManager {
    LIST_HEAD(session_list, session_list_entry) sessions; // doubly-linked list of sessions
    UA_UInt32    maxSessionCount;
    UA_Int32     lastSessionId;
    UA_UInt32    currentSessionCount;
    UA_DateTime  maxSessionLifeTime;
} UA_SessionManager;

UA_StatusCode UA_SessionManager_init(UA_SessionManager *sessionManager, UA_UInt32 maxSessionCount,
                                    UA_UInt32 maxSessionLifeTime, UA_UInt32 startSessionId);

void UA_SessionManager_deleteMembers(UA_SessionManager *sessionManager, UA_Server *server);

void UA_SessionManager_cleanupTimedOut(UA_SessionManager *sessionManager, UA_Server *server, UA_DateTime now);

UA_StatusCode UA_SessionManager_createSession(UA_SessionManager *sessionManager,
                                              UA_SecureChannel *channel, const UA_CreateSessionRequest *request,
                                              UA_Session **session);

UA_StatusCode UA_SessionManager_removeSession(UA_SessionManager *sessionManager, UA_Server *server, const UA_NodeId *token);

/** Finds the session which is identified by the authentication token */
UA_Session * UA_SessionManager_getSession(UA_SessionManager *sessionManager, const UA_NodeId *token);

#endif /* UA_SESSION_MANAGER_H_ */
