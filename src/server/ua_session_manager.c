#include "ua_session_manager.h"
#include "ua_util.h"

/**
 The functions in this file are not thread-safe. For multi-threaded access, a
 second implementation should be provided. See for example, how a nodestore
 implementation is choosen based on whether multithreading is enabled or not.
 */

struct session_list_entry {
    UA_Session session;
    LIST_ENTRY(session_list_entry) pointers;
};

struct UA_SessionManager {
    LIST_HEAD(session_list, session_list_entry) sessions;
    UA_UInt32    maxSessionCount;
    UA_Int32     lastSessionId;
    UA_UInt32    currentSessionCount;
    UA_DateTime  maxSessionLifeTime;
    UA_DateTime  sessionTimeout;
};

UA_Int32 UA_SessionManager_new(UA_SessionManager **sessionManager, UA_UInt32 maxSessionCount,
                               UA_UInt32 sessionTimeout, UA_UInt32 startSessionId) {
    UA_Int32 retval = UA_SUCCESS;
    retval |= UA_alloc((void **)sessionManager, sizeof(UA_SessionManager));
    if(retval != UA_SUCCESS)
        return UA_ERROR;
    LIST_INIT(&(*sessionManager)->sessions);
    (*sessionManager)->maxSessionCount = maxSessionCount;
    (*sessionManager)->lastSessionId   = startSessionId;
    (*sessionManager)->sessionTimeout  = sessionTimeout;
    return retval;
}

UA_Int32 UA_SessionManager_delete(UA_SessionManager *sessionManager) {
    struct session_list_entry *current = LIST_FIRST(&sessionManager->sessions);
    while(current) {
        LIST_REMOVE(current, pointers);
        if(current->session.channel)
            current->session.channel->session = UA_NULL; // the channel is no longer attached to a session
        UA_Session_deleteMembers(&current->session);
        UA_free(current);
        current = LIST_FIRST(&sessionManager->sessions);
    }
    UA_free(sessionManager);
    return UA_SUCCESS;
}

UA_Int32 UA_SessionManager_getSessionById(UA_SessionManager *sessionManager, UA_NodeId *sessionId, UA_Session **session) {
    if(sessionManager == UA_NULL) {
        *session = UA_NULL;
        return UA_ERROR;
    }

    struct session_list_entry *current = UA_NULL;
    LIST_FOREACH(current, &sessionManager->sessions, pointers) {
        if(UA_NodeId_equal(&current->session.sessionId, sessionId) == UA_EQUAL)
            break;
    }

    if(!current) {
        *session = UA_NULL;
        return UA_ERROR;
    }

    // Lifetime handling is not done here, but in a regular cleanup by the
    // server. If the session still exists, then it is valid.
    *session = &current->session;
    return UA_SUCCESS;
}

UA_Int32 UA_SessionManager_getSessionByToken(UA_SessionManager *sessionManager, UA_NodeId *token, UA_Session **session) {
    if(sessionManager == UA_NULL) {
        *session = UA_NULL;
        return UA_ERROR;
    }

    struct session_list_entry *current = UA_NULL;
    LIST_FOREACH(current, &sessionManager->sessions, pointers) {
        if(UA_NodeId_equal(&current->session.authenticationToken, token) == UA_EQUAL)
            break;
    }

    if(!current) {
        *session = UA_NULL;
        return UA_ERROR;
    }

    // Lifetime handling is not done here, but in a regular cleanup by the
    // server. If the session still exists, then it is valid.
    *session = &current->session;
    return UA_SUCCESS;
}

/** Creates and adds a session. */
UA_Int32 UA_SessionManager_createSession(UA_SessionManager *sessionManager, UA_SecureChannel *channel, UA_Session **session) {
    if(sessionManager->currentSessionCount >= sessionManager->maxSessionCount)
        return UA_ERROR;

    struct session_list_entry *newentry;
    if(UA_alloc((void **)&newentry, sizeof(struct session_list_entry)) != UA_SUCCESS)
        return UA_ERROR;

    UA_Session_init(&newentry->session);
    newentry->session.sessionId = (UA_NodeId) {.namespaceIndex = 1, .identifierType = UA_NODEIDTYPE_NUMERIC,
                                               .identifier.numeric = sessionManager->lastSessionId++ };
    newentry->session.authenticationToken = (UA_NodeId) {.namespaceIndex = 1, .identifierType = UA_NODEIDTYPE_NUMERIC,
                                                         .identifier.numeric = sessionManager->lastSessionId };
    newentry->session.channel = channel;
    newentry->session.timeout = 3600 * 1000; // 1h
    UA_Session_setExpirationDate(&newentry->session);

    sessionManager->currentSessionCount++;
    LIST_INSERT_HEAD(&sessionManager->sessions, newentry, pointers);
    *session = &newentry->session;
    return UA_SUCCESS;
}

UA_Int32 UA_SessionManager_removeSession(UA_SessionManager *sessionManager, UA_NodeId  *sessionId) {
    struct session_list_entry *current = UA_NULL;
    LIST_FOREACH(current, &sessionManager->sessions, pointers) {
        if(UA_NodeId_equal(&current->session.sessionId, sessionId) == UA_EQUAL)
            break;
    }

    if(!current)
        return UA_ERROR;

    LIST_REMOVE(current, pointers);
    if(current->session.channel)
        current->session.channel->session = UA_NULL; // the channel is no longer attached to a session
    UA_Session_deleteMembers(&current->session);
    UA_free(current);
    return UA_SUCCESS;
}
