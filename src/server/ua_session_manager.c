#include "ua_session_manager.h"
#include "ua_server_internal.h"

UA_StatusCode
UA_SessionManager_init(UA_SessionManager *sm, UA_Server *server) {
    LIST_INIT(&sm->sessions);
    sm->currentSessionCount = 0;
    sm->server = server;
    return UA_STATUSCODE_GOOD;
}

void UA_SessionManager_deleteMembers(UA_SessionManager *sm) {
    session_list_entry *current, *temp;
    LIST_FOREACH_SAFE(current, &sm->sessions, pointers, temp) {
        LIST_REMOVE(current, pointers);
        UA_Session_deleteMembersCleanup(&current->session, sm->server);
        UA_free(current);
    }
}

void UA_SessionManager_cleanupTimedOut(UA_SessionManager *sm, UA_DateTime nowMonotonic) {
    session_list_entry *sentry, *temp;
    LIST_FOREACH_SAFE(sentry, &sm->sessions, pointers, temp) {
        if(sentry->session.validTill < nowMonotonic) {
            UA_LOG_DEBUG(sm->server->config.logger, UA_LOGCATEGORY_SESSION,
                         "Session with token %i has timed out and is removed",
                         sentry->session.sessionId.identifier.numeric);
            LIST_REMOVE(sentry, pointers);
            UA_Session_deleteMembersCleanup(&sentry->session, sm->server);
#ifndef UA_ENABLE_MULTITHREADING
            sm->currentSessionCount--;
            UA_free(sentry);
#else
            sm->currentSessionCount = uatomic_add_return(&sm->currentSessionCount, -1);
            UA_Server_delayedFree(sm->server, sentry);
#endif
        }
    }
}

UA_Session *
UA_SessionManager_getSession(UA_SessionManager *sm, const UA_NodeId *token) {
    session_list_entry *current = NULL;
    LIST_FOREACH(current, &sm->sessions, pointers) {
        if(UA_NodeId_equal(&current->session.authenticationToken, token)) {
            if(UA_DateTime_nowMonotonic() > current->session.validTill) {
                UA_LOG_DEBUG(sm->server->config.logger, UA_LOGCATEGORY_SESSION,
                             "Try to use Session with token " UA_PRINTF_GUID_FORMAT ", but has timed out",
                             UA_PRINTF_GUID_DATA(token->identifier.guid));
                return NULL;
            }
            return &current->session;
        }
    }
    UA_LOG_DEBUG(sm->server->config.logger, UA_LOGCATEGORY_SESSION,
                 "Try to use Session with token " UA_PRINTF_GUID_FORMAT " but is not found",
                 UA_PRINTF_GUID_DATA(token->identifier.guid));
    return NULL;
}

/** Creates and adds a session. But it is not yet attached to a secure channel. */
UA_StatusCode
UA_SessionManager_createSession(UA_SessionManager *sm, UA_SecureChannel *channel,
                                const UA_CreateSessionRequest *request, UA_Session **session) {
    if(sm->currentSessionCount >= sm->server->config.maxSessions)
        return UA_STATUSCODE_BADTOOMANYSESSIONS;

    session_list_entry *newentry = UA_malloc(sizeof(session_list_entry));
    if(!newentry)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    sm->currentSessionCount++;
    UA_Session_init(&newentry->session);
    newentry->session.sessionId = UA_NODEID_GUID(1, UA_Guid_random());
    newentry->session.authenticationToken = UA_NODEID_GUID(1, UA_Guid_random());

    if(request->requestedSessionTimeout <= sm->server->config.maxSessionTimeout &&
       request->requestedSessionTimeout > 0)
        newentry->session.timeout = request->requestedSessionTimeout;
    else
        newentry->session.timeout = sm->server->config.maxSessionTimeout;

    UA_Session_updateLifetime(&newentry->session);
    LIST_INSERT_HEAD(&sm->sessions, newentry, pointers);
    *session = &newentry->session;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_SessionManager_removeSession(UA_SessionManager *sm, const UA_NodeId *token) {
    session_list_entry *current;
    LIST_FOREACH(current, &sm->sessions, pointers) {
        if(UA_NodeId_equal(&current->session.authenticationToken, token))
            break;
    }

    if(!current)
        return UA_STATUSCODE_BADSESSIONIDINVALID;

    LIST_REMOVE(current, pointers);
    UA_Session_deleteMembersCleanup(&current->session, sm->server);
#ifndef UA_ENABLE_MULTITHREADING
    sm->currentSessionCount--;
    UA_free(current);
#else
    sm->currentSessionCount = uatomic_add_return(&sm->currentSessionCount, -1);
    UA_Server_delayedFree(sm->server, current);
#endif
    return UA_STATUSCODE_GOOD;
}
