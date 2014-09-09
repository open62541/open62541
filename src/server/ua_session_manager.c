#include "ua_session_manager.h"
#include "util/ua_list.h"

struct UA_SessionManager {
    UA_list_List sessions;
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
    retval |= UA_list_init(&(*sessionManager)->sessions);
    (*sessionManager)->maxSessionCount = maxSessionCount;
    (*sessionManager)->lastSessionId   = startSessionId;
    (*sessionManager)->sessionTimeout  = sessionTimeout;
    return retval;
}

UA_Int32 UA_SessionManager_delete(UA_SessionManager *sessionManager) {
    // todo
    return UA_SUCCESS;
}


UA_Int32 UA_SessionManager_generateSessionId(UA_SessionManager *sessionManager,
                                             UA_NodeId         *sessionId) {
    sessionId->namespaceIndex     = 0;
    sessionId->identifierType     = UA_NODEIDTYPE_NUMERIC;
    sessionId->identifier.numeric = sessionManager->lastSessionId++;
    return UA_SUCCESS;
}

UA_Boolean UA_SessionManager_sessionExists(UA_SessionManager *sessionManager,
                                           UA_Session        *session) {
    if(sessionManager == UA_NULL)
        return UA_FALSE;

    if(UA_list_search(&sessionManager->sessions,
                      (UA_list_PayloadComparer)UA_Session_compare, (void *)session)) {
        UA_Double pendingLifetime;
        UA_Session_getPendingLifetime(session, &pendingLifetime);

        if(pendingLifetime > 0)
            return UA_TRUE;

        //timeout of session reached so remove it
        UA_NodeId *sessionId = &session->sessionId;
        UA_SessionManager_removeSession(sessionManager, sessionId);
    }
    return UA_FALSE;
}

UA_Int32 UA_SessionManager_getSessionById(UA_SessionManager *sessionManager,
                                          UA_NodeId *sessionId, UA_Session **session) {
    if(sessionManager == UA_NULL) {
        *session = UA_NULL;
        return UA_ERROR;
    }

    UA_list_Element *current = sessionManager->sessions.first;
    while(current) {
        if(current->payload) {
            UA_list_Element *elem = (UA_list_Element *)current;
            *session = ((UA_Session *)(elem->payload));
            if(UA_NodeId_equal(&(*session)->sessionId, sessionId) == UA_EQUAL) {
                UA_Double pendingLifetime;
                UA_Session_getPendingLifetime(*session, &pendingLifetime);

                if(pendingLifetime > 0)
                    return UA_SUCCESS;

                //session not valid anymore -> remove it
                UA_list_removeElement(elem, (UA_list_PayloadVisitor)UA_Session_delete);
                *session = UA_NULL;
                return UA_ERROR;
            }
        }
        current = current->next;
    }
    *session = UA_NULL;
    return UA_ERROR;
}

UA_Int32 UA_SessionManager_getSessionByToken(UA_SessionManager *sessionManager,
                                             UA_NodeId *token, UA_Session **session) {
    if(sessionManager == UA_NULL) {
        *session = UA_NULL;
        return UA_ERROR;
    }

    UA_list_Element *current = sessionManager->sessions.first;
    while(current) {
        if(current->payload) {
            UA_list_Element *elem = (UA_list_Element *)current;
            *session = ((UA_Session *)(elem->payload));

            if(UA_NodeId_equal(&(*session)->authenticationToken, token) == UA_EQUAL) {
                UA_Double pendingLifetime;
                UA_Session_getPendingLifetime(*session, &pendingLifetime);

                if(pendingLifetime > 0)
                    return UA_SUCCESS;

                //session not valid anymore -> remove it
                UA_list_removeElement(elem, (UA_list_PayloadVisitor)UA_Session_delete);
                *session = UA_NULL;
                return UA_ERROR;
            }
        }
        current = current->next;
    }
    *session = UA_NULL;
    return UA_ERROR;
}

/** Creates and adds a session. */
UA_Int32 UA_SessionManager_addSession(UA_SessionManager *sessionManager,
                                      UA_SecureChannel *channel, UA_Session **session) {
    UA_Int32 retval = UA_SUCCESS;
    if(sessionManager->currentSessionCount >= sessionManager->maxSessionCount)
        return UA_ERROR;
    UA_Session_new(session);
    (*session)->sessionId = (UA_NodeId) {.namespaceIndex     = 1, .identifierType = UA_NODEIDTYPE_NUMERIC,
                                         .identifier.numeric = sessionManager->lastSessionId++ };
    (*session)->channel   = channel;
    channel->session      = *session;

    sessionManager->currentSessionCount++;
    return retval;
}

UA_Int32 UA_SessionManager_removeSession(UA_SessionManager *sessionManager,
                                         UA_NodeId         *sessionId) {
    UA_Int32 retval = UA_SUCCESS;
    UA_list_Element *element =
        UA_list_search(&sessionManager->sessions, (UA_list_PayloadComparer)UA_Session_compare,
                       sessionId);
    if(element) {
        UA_Session *session = element->payload;
        session->channel->session = UA_NULL;
        retval |= UA_list_removeElement(element, (UA_list_PayloadVisitor)UA_Session_delete);
        printf("UA_SessionManager_removeSession - session removed, current count: %i \n",
               sessionManager->sessions.size);
    }
    return retval;
}

UA_Int32 UA_SessionManager_getSessionTimeout(UA_SessionManager *sessionManager,
                                             UA_Int64          *timeout_ms) {
    if(sessionManager) {
        *timeout_ms = sessionManager->sessionTimeout;
        return UA_SUCCESS;
    }
    *timeout_ms = 0;
    return UA_ERROR;
}
