/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014, 2017 (c) Florian Palm
 *    Copyright 2015 (c) Sten Grüner
 *    Copyright 2015 (c) Oleksiy Vasylyev
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#include "ua_session_manager.h"
#include "ua_server_internal.h"
#include "ua_subscription.h"

UA_StatusCode
UA_SessionManager_init(UA_SessionManager *sm, UA_Server *server) {
    LIST_INIT(&sm->sessions);
    sm->currentSessionCount = 0;
    sm->server = server;
    return UA_STATUSCODE_GOOD;
}

/* Delayed callback to free the session memory */
static void
removeSessionCallback(UA_Server *server, session_list_entry *entry) {
    UA_Session_deleteMembersCleanup(&entry->session, server);
}

static void
removeSession(UA_SessionManager *sm, session_list_entry *sentry) {
    /* Remove the Subscriptions */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_Subscription *sub, *tempsub;
    LIST_FOREACH_SAFE(sub, &sentry->session.serverSubscriptions, listEntry, tempsub) {
        UA_Session_deleteSubscription(sm->server, &sentry->session, sub->subscriptionId);
    }

    UA_PublishResponseEntry *entry;
    while((entry = UA_Session_dequeuePublishReq(&sentry->session))) {
        UA_PublishResponse_deleteMembers(&entry->response);
        UA_free(entry);
    }
#endif

    /* Detach the Session from the SecureChannel */
    UA_Session_detachFromSecureChannel(&sentry->session);

    /* Deactivate the session */
    sentry->session.activated = false;

    /* Detach the session from the session manager and make the capacity
     * available */
    LIST_REMOVE(sentry, pointers);
    UA_atomic_subUInt32(&sm->currentSessionCount, 1);

    /* Add a delayed callback to remove the session when the currently
     * scheduled jobs have completed */
    sentry->cleanupCallback.callback = (UA_ApplicationCallback)removeSessionCallback;
    sentry->cleanupCallback.application = sm->server;
    sentry->cleanupCallback.data = sentry;
    UA_WorkQueue_enqueueDelayed(&sm->server->workQueue, &sentry->cleanupCallback);
}

void UA_SessionManager_deleteMembers(UA_SessionManager *sm) {
    session_list_entry *current, *temp;
    LIST_FOREACH_SAFE(current, &sm->sessions, pointers, temp) {
        removeSession(sm, current);
    }
}

void
UA_SessionManager_cleanupTimedOut(UA_SessionManager *sm,
                                  UA_DateTime nowMonotonic) {
    session_list_entry *sentry, *temp;
    LIST_FOREACH_SAFE(sentry, &sm->sessions, pointers, temp) {
        /* Session has timed out? */
        if(sentry->session.validTill >= nowMonotonic)
            continue;
        UA_LOG_INFO_SESSION(&sm->server->config.logger, &sentry->session,
                            "Session has timed out");
        sm->server->config.accessControl.closeSession(sm->server,
                                                      &sm->server->config.accessControl,
                                                      &sentry->session.sessionId,
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
            UA_LOG_INFO_SESSION(&sm->server->config.logger, &current->session,
                                "Client tries to use a session that has timed out");
            return NULL;
        }

        /* Ok, return */
        return &current->session;
    }

    /* Session not found */
    UA_String nodeIdStr = UA_STRING_NULL;
    UA_NodeId_toString(token, &nodeIdStr);
    UA_LOG_INFO(&sm->server->config.logger, UA_LOGCATEGORY_SESSION,
                "Try to use Session with token %.*s but is not found",
                (int)nodeIdStr.length, nodeIdStr.data);
    UA_String_deleteMembers(&nodeIdStr);
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
            UA_LOG_INFO_SESSION(&sm->server->config.logger, &current->session,
                                "Client tries to use a session that has timed out");
            return NULL;
        }

        /* Ok, return */
        return &current->session;
    }

    /* Session not found */
    UA_String sessionIdStr = UA_STRING_NULL;
    UA_NodeId_toString(sessionId, &sessionIdStr);
    UA_LOG_INFO(&sm->server->config.logger, UA_LOGCATEGORY_SESSION,
                "Try to use Session with identifier %.*s but is not found",
                (int)sessionIdStr.length, sessionIdStr.data);
    UA_String_deleteMembers(&sessionIdStr);
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

    UA_atomic_addUInt32(&sm->currentSessionCount, 1);
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

    removeSession(sm, current);
    return UA_STATUSCODE_GOOD;
}
