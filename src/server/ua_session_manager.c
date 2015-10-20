#include "ua_session_manager.h"
#include "ua_statuscodes.h"
#include "ua_util.h"

UA_StatusCode
UA_SessionManager_init(UA_SessionManager *sessionManager, UA_UInt32 maxSessionCount,
                       UA_UInt32 maxSessionLifeTime, UA_UInt32 startSessionId) {
    LIST_INIT(&sessionManager->sessions);
    sessionManager->maxSessionCount = maxSessionCount;
    sessionManager->lastSessionId   = startSessionId;
    sessionManager->maxSessionLifeTime  = maxSessionLifeTime;
    sessionManager->currentSessionCount = 0;
    return UA_STATUSCODE_GOOD;
}

void UA_SessionManager_deleteMembers(UA_SessionManager *sessionManager, UA_Server *server) {
    session_list_entry *current, *temp;
    LIST_FOREACH_SAFE(current, &sessionManager->sessions, pointers, temp) {
        LIST_REMOVE(current, pointers);
        UA_Session_deleteMembersCleanup(&current->session, server);
        UA_free(current);
    }
}

void UA_SessionManager_cleanupTimedOut(UA_SessionManager *sessionManager,
                                       UA_Server* server, UA_DateTime now) {
    session_list_entry *sentry, *temp;
    LIST_FOREACH_SAFE(sentry, &sessionManager->sessions, pointers, temp) {
        if(sentry->session.validTill < now) {
            UA_Session_deleteMembersCleanup(&sentry->session, server);
            UA_free(sentry);
            sessionManager->currentSessionCount--;
        }
    }
}

UA_Session *
UA_SessionManager_getSession(UA_SessionManager *sessionManager, const UA_NodeId *token) {
    session_list_entry *current = UA_NULL;
    LIST_FOREACH(current, &sessionManager->sessions, pointers) {
        if(UA_NodeId_equal(&current->session.authenticationToken, token)) {
            if(UA_DateTime_now() > current->session.validTill)
                return UA_NULL;
            return &current->session;
        }
    }
    return UA_NULL;
}

/** Creates and adds a session. But it is not yet attached to a secure channel. */
UA_StatusCode
UA_SessionManager_createSession(UA_SessionManager *sessionManager, UA_SecureChannel *channel,
                                const UA_CreateSessionRequest *request, UA_Session **session) {
    if(sessionManager->currentSessionCount >= sessionManager->maxSessionCount)
        return UA_STATUSCODE_BADTOOMANYSESSIONS;

    session_list_entry *newentry = UA_malloc(sizeof(session_list_entry));
    if(!newentry)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    sessionManager->currentSessionCount++;
    UA_Session_init(&newentry->session);
    newentry->session.sessionId = UA_NODEID_NUMERIC(1, sessionManager->lastSessionId++);
    UA_UInt32 randSeed = (UA_UInt32)(sessionManager->lastSessionId + UA_DateTime_now());
    newentry->session.authenticationToken = UA_NODEID_GUID(1, UA_Guid_random(&randSeed));

    if(request->requestedSessionTimeout <= sessionManager->maxSessionLifeTime &&
       request->requestedSessionTimeout > 0)
        newentry->session.timeout = (UA_Int64)request->requestedSessionTimeout;
    else
        newentry->session.timeout = sessionManager->maxSessionLifeTime; // todo: remove when the CTT is fixed

    UA_Session_updateLifetime(&newentry->session);
    LIST_INSERT_HEAD(&sessionManager->sessions, newentry, pointers);
    *session = &newentry->session;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_SessionManager_removeSession(UA_SessionManager *sessionManager,
                                UA_Server* server, const UA_NodeId *token) {
    session_list_entry *current;
    LIST_FOREACH(current, &sessionManager->sessions, pointers) {
        if(UA_NodeId_equal(&current->session.authenticationToken, token))
            break;
    }

    if(!current)
        return UA_STATUSCODE_BADSESSIONIDINVALID;

    LIST_REMOVE(current, pointers);
    UA_Session_deleteMembersCleanup(&current->session, server);
    UA_free(current);
    sessionManager->currentSessionCount--;
    return UA_STATUSCODE_GOOD;
}
