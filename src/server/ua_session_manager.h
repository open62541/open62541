#ifndef UA_SESSION_MANAGER_H_
#define UA_SESSION_MANAGER_H_

#include "queue.h"
#include "ua_server.h"
#include "ua_util.h"
#include "ua_session.h"

typedef struct session_list_entry {
    LIST_ENTRY(session_list_entry) pointers;
    UA_Session session;
} session_list_entry;

typedef struct UA_SessionManager {
    LIST_HEAD(session_list, session_list_entry) sessions; // doubly-linked list of sessions
    UA_UInt32 currentSessionCount;
    UA_Server *server;
} UA_SessionManager;

UA_StatusCode
UA_SessionManager_init(UA_SessionManager *sm, UA_Server *server);

void UA_SessionManager_deleteMembers(UA_SessionManager *sessionManager);

void UA_SessionManager_cleanupTimedOut(UA_SessionManager *sessionManager, UA_DateTime nowMonotonic);

UA_StatusCode
UA_SessionManager_createSession(UA_SessionManager *sessionManager, UA_SecureChannel *channel,
                                const UA_CreateSessionRequest *request, UA_Session **session);

UA_StatusCode
UA_SessionManager_removeSession(UA_SessionManager *sessionManager, const UA_NodeId *token);

UA_Session *
UA_SessionManager_getSession(UA_SessionManager *sessionManager, const UA_NodeId *token);

#endif /* UA_SESSION_MANAGER_H_ */
