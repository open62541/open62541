/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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

/* Delayed callback to free the session memory */
static void
removeSessionCallback(UA_Server *server, void *entry) {
    session_list_entry *sentry = (session_list_entry*)entry;
    UA_Session_deleteMembersCleanup(&sentry->session, server);
    UA_free(sentry);
}

static UA_StatusCode
removeSession(UA_SessionManager *sm, session_list_entry *sentry) {
    /* Detach the Session from the SecureChannel */
    UA_Session_detachFromSecureChannel(&sentry->session);

    /* Deactivate the session */
    sentry->session.activated = false;

    /* Add a delayed callback to remove the session when the currently
     * scheduled jobs have completed */
    UA_StatusCode retval = UA_Server_delayedCallback(sm->server, removeSessionCallback, sentry);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_SESSION(sm->server->config.logger, &sentry->session,
                       "Could not remove session with error code %s",
                       UA_StatusCode_name(retval));
        return retval; /* Try again next time */
    }

    /* Detach the session from the session manager and make the capacity
     * available */
    LIST_REMOVE(sentry, pointers);
    UA_atomic_add(&sm->currentSessionCount, (UA_UInt32)-1);
    return UA_STATUSCODE_GOOD;
}

void
UA_SessionManager_cleanupTimedOut(UA_SessionManager *sm,
                                  UA_DateTime nowMonotonic) {
    session_list_entry *sentry, *temp;
    LIST_FOREACH_SAFE(sentry, &sm->sessions, pointers, temp) {
        /* Session has timed out? */
        if(sentry->session.validTill >= nowMonotonic)
            continue;
        UA_LOG_INFO_SESSION(sm->server->config.logger, &sentry->session,
                            "Session has timed out");
        sm->server->config.accessControl.closeSession(&sentry->session.sessionId,
                                                      sentry->session.sessionHandle);
        removeSession(sm, sentry);
    }
}

UA_Session *
UA_SessionManager_getSessionByToken(UA_SessionManager *sm, const UA_NodeId *token) {
    session_list_entry *current = NULL;
    LIST_FOREACH(current, &sm->sessions, pointers) {
        /* Token does not match */
        if(!UA_NodeId_equal(&current->session.header.authenticationToken, token))
            continue;

        /* Session has timed out */
        if(UA_DateTime_nowMonotonic() > current->session.validTill) {
            UA_LOG_INFO_SESSION(sm->server->config.logger, &current->session,
                                "Client tries to use a session that has timed out");
            return NULL;
        }

        /* Ok, return */
        return &current->session;
    }

    /* Session not found */
    UA_LOG_INFO(sm->server->config.logger, UA_LOGCATEGORY_SESSION,
                "Try to use Session with token " UA_PRINTF_GUID_FORMAT " but is not found",
                UA_PRINTF_GUID_DATA(token->identifier.guid));
    return NULL;
}

UA_Session *
UA_SessionManager_getSessionById(UA_SessionManager *sm, const UA_NodeId *sessionId) {
    session_list_entry *current = NULL;
    LIST_FOREACH(current, &sm->sessions, pointers) {
        /* Token does not match */
        if(!UA_NodeId_equal(&current->session.sessionId, sessionId))
            continue;

        /* Session has timed out */
        if(UA_DateTime_nowMonotonic() > current->session.validTill) {
            UA_LOG_INFO_SESSION(sm->server->config.logger, &current->session,
                                "Client tries to use a session that has timed out");
            return NULL;
        }

        /* Ok, return */
        return &current->session;
    }

    /* Session not found */
    UA_LOG_INFO(sm->server->config.logger, UA_LOGCATEGORY_SESSION,
                "Try to use Session with identifier " UA_PRINTF_GUID_FORMAT " but is not found",
                UA_PRINTF_GUID_DATA(sessionId->identifier.guid));
    return NULL;
}

/* Creates and adds a session. But it is not yet attached to a secure channel. */
UA_StatusCode
UA_SessionManager_createSession(UA_SessionManager *sm, UA_SecureChannel *channel,
                                const UA_CreateSessionRequest *request, UA_Session **session) {
    if(sm->currentSessionCount >= sm->server->config.maxSessions)
        return UA_STATUSCODE_BADTOOMANYSESSIONS;

    session_list_entry *newentry = (session_list_entry *)UA_malloc(sizeof(session_list_entry));
    if(!newentry)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_atomic_add(&sm->currentSessionCount, 1);
    UA_Session_init(&newentry->session);
    newentry->session.sessionId = UA_NODEID_GUID(1, UA_Guid_random());
    newentry->session.header.authenticationToken = UA_NODEID_GUID(1, UA_Guid_random());

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
        if(UA_NodeId_equal(&current->session.header.authenticationToken, token))
            break;
    }
    if(!current)
        return UA_STATUSCODE_BADSESSIONIDINVALID;
    return removeSession(sm, current);
}
