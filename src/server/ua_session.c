/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2018 (c) Thomas Stalder, Blue Time Concept SA
 *    Copyright 2019 (c) HMS Industrial Networks AB (Author: Jonas Green)
 */

#include "ua_session.h"
#include "open62541/types.h"
#include "ua_server_internal.h"
#ifdef UA_ENABLE_SUBSCRIPTIONS
#include "ua_subscription.h"
#endif

#define UA_SESSION_NONCELENTH 32

void UA_Session_init(UA_Session *session) {
    memset(session, 0, sizeof(UA_Session));
    session->availableContinuationPoints = UA_MAXCONTINUATIONPOINTS;
#ifdef UA_ENABLE_SUBSCRIPTIONS
    SIMPLEQ_INIT(&session->responseQueue);
    TAILQ_INIT(&session->subscriptions);
#endif
}

void UA_Session_clear(UA_Session *session, UA_Server* server) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Remove all Subscriptions. This may send out remaining publish
     * responses. */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_Subscription *sub, *tempsub;
    TAILQ_FOREACH_SAFE(sub, &session->subscriptions, sessionListEntry, tempsub) {
        UA_Subscription_delete(server, sub);
    }
#endif

#ifdef UA_ENABLE_DIAGNOSTICS
    deleteNode(server, session->sessionId, true);
#endif

    UA_Session_detachFromSecureChannel(session);
    UA_ApplicationDescription_clear(&session->clientDescription);
    UA_NodeId_clear(&session->authenticationToken);
    UA_String_clear(&session->clientUserIdOfSession);
    UA_NodeId_clear(&session->sessionId);
    UA_String_clear(&session->sessionName);
    UA_ByteString_clear(&session->serverNonce);
    struct ContinuationPoint *cp, *next = session->continuationPoints;
    while((cp = next)) {
        next = ContinuationPoint_clear(cp);
        UA_free(cp);
    }
    session->continuationPoints = NULL;
    session->availableContinuationPoints = UA_MAXCONTINUATIONPOINTS;

    UA_KeyValueMap_delete(session->attributes);
    session->attributes = NULL;

    UA_Array_delete(session->localeIds, session->localeIdsSize,
                    &UA_TYPES[UA_TYPES_STRING]);
    session->localeIds = NULL;
    session->localeIdsSize = 0;

#ifdef UA_ENABLE_DIAGNOSTICS
    UA_SessionDiagnosticsDataType_clear(&session->diagnostics);
    UA_SessionSecurityDiagnosticsDataType_clear(&session->securityDiagnostics);
#endif
}

void
UA_Session_attachToSecureChannel(UA_Session *session, UA_SecureChannel *channel) {
    /* Ensure the Session is not attached to another SecureChannel */
    UA_Session_detachFromSecureChannel(session);

    /* Add to singly-linked list */
    session->next = channel->sessions;
    channel->sessions = session;

    /* Add backpointer */
    session->channel = channel;
}

void
UA_Session_detachFromSecureChannel(UA_Session *session) {
    /* Clean up the response queue. Their RequestId is bound to the
     * SecureChannel so they cannot be reused. */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_PublishResponseEntry *pre;
    while((pre = UA_Session_dequeuePublishReq(session))) {
        UA_PublishResponse_clear(&pre->response);
        UA_free(pre);
    }
#endif

    /* Remove from singly-linked list */
    UA_SecureChannel *channel = session->channel;
    if(!channel)
        return;

    if(channel->sessions == session) {
        channel->sessions = session->next;
    } else {
        UA_Session *elm =  channel->sessions;
        while(elm->next != session)
            elm = elm->next;
        elm->next = session->next;
    }

    /* Reset the backpointer */
    session->channel = NULL;
}

UA_StatusCode
UA_Session_generateNonce(UA_Session *session) {
    UA_SecureChannel *channel = session->channel;
    if(!channel || !channel->securityPolicy)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Is the length of the previous nonce correct? */
    if(session->serverNonce.length != UA_SESSION_NONCELENTH) {
        UA_ByteString_clear(&session->serverNonce);
        UA_StatusCode retval =
            UA_ByteString_allocBuffer(&session->serverNonce, UA_SESSION_NONCELENTH);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    return channel->securityPolicy->symmetricModule.
        generateNonce(channel->securityPolicy->policyContext, &session->serverNonce);
}

void
UA_Session_updateLifetime(UA_Session *session, UA_DateTime now,
                          UA_DateTime nowMonotonic) {
    session->validTill = nowMonotonic +
        (UA_DateTime)(session->timeout * UA_DATETIME_MSEC);
#ifdef UA_ENABLE_DIAGNOSTICS
    session->diagnostics.clientLastContactTime = now;
#endif
}

#ifdef UA_ENABLE_SUBSCRIPTIONS

void
UA_Session_attachSubscription(UA_Session *session, UA_Subscription *sub) {
    /* Attach to the session */
    sub->session = session;

    /* Increase the count */
    session->subscriptionsSize++;

    /* Increase the number of outstanding retransmissions */
    session->totalRetransmissionQueueSize += sub->retransmissionQueueSize;

    /* Insert at the end of the subscriptions of the same priority / just before
     * the subscriptions with the next lower priority. */
    UA_Subscription *after = NULL;
    TAILQ_FOREACH(after, &session->subscriptions, sessionListEntry) {
        if(after->priority < sub->priority) {
            TAILQ_INSERT_BEFORE(after, sub, sessionListEntry);
            return;
        }
    }
    TAILQ_INSERT_TAIL(&session->subscriptions, sub, sessionListEntry);
}

void
UA_Session_detachSubscription(UA_Server *server, UA_Session *session,
                              UA_Subscription *sub, UA_Boolean releasePublishResponses) {
    /* Detach from the session */
    sub->session = NULL;
    TAILQ_REMOVE(&session->subscriptions, sub, sessionListEntry);

    /* Reduce the count */
    UA_assert(session->subscriptionsSize > 0);
    session->subscriptionsSize--;

    /* Reduce the number of outstanding retransmissions */
    session->totalRetransmissionQueueSize -= sub->retransmissionQueueSize;

    /* Send remaining publish responses if the last subscription was removed */
    if(!releasePublishResponses || !TAILQ_EMPTY(&session->subscriptions))
        return;
    UA_PublishResponseEntry *pre;
    while((pre = UA_Session_dequeuePublishReq(session))) {
        UA_PublishResponse *response = &pre->response;
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOSUBSCRIPTION;
        sendResponse(server, session->channel, pre->requestId,
                     (UA_Response*)response, &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]);
        UA_PublishResponse_clear(response);
        UA_free(pre);
    }
}

UA_Subscription *
UA_Session_getSubscriptionById(UA_Session *session, UA_UInt32 subscriptionId) {
    UA_Subscription *sub;
    TAILQ_FOREACH(sub, &session->subscriptions, sessionListEntry) {
        /* Prevent lookup of subscriptions that are to be deleted with a statuschange */
        if(sub->statusChange != UA_STATUSCODE_GOOD)
            continue;
        if(sub->subscriptionId == subscriptionId)
            break;
    }
    return sub;
}

UA_Subscription *
getSubscriptionById(UA_Server *server, UA_UInt32 subscriptionId) {
    UA_Subscription *sub;
    LIST_FOREACH(sub, &server->subscriptions, serverListEntry) {
        /* Prevent lookup of subscriptions that are to be deleted with a statuschange */
        if(sub->statusChange != UA_STATUSCODE_GOOD)
            continue;
        if(sub->subscriptionId == subscriptionId)
            break;
    }
    return sub;
}

UA_PublishResponseEntry*
UA_Session_dequeuePublishReq(UA_Session *session) {
    UA_PublishResponseEntry *entry = SIMPLEQ_FIRST(&session->responseQueue);
    if(entry) {
        SIMPLEQ_REMOVE_HEAD(&session->responseQueue, listEntry);
        session->responseQueueSize--;
    }
    return entry;
}

void
UA_Session_queuePublishReq(UA_Session *session, UA_PublishResponseEntry* entry,
                           UA_Boolean head) {
    if(!head)
        SIMPLEQ_INSERT_TAIL(&session->responseQueue, entry, listEntry);
    else
        SIMPLEQ_INSERT_HEAD(&session->responseQueue, entry, listEntry);
    session->responseQueueSize++;
}

#endif

/* Session Handling */

UA_StatusCode
UA_Server_closeSession(UA_Server *server, const UA_NodeId *sessionId) {
    UA_LOCK(&server->serviceMutex);
    session_list_entry *entry;
    UA_StatusCode res = UA_STATUSCODE_BADSESSIONIDINVALID;
    LIST_FOREACH(entry, &server->sessions, pointers) {
        if(UA_NodeId_equal(&entry->session.sessionId, sessionId)) {
            UA_Server_removeSession(server, entry, UA_SHUTDOWNREASON_CLOSE);
            res = UA_STATUSCODE_GOOD;
            break;
        }
    }
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

/* Session Attributes */

#define UA_PROTECTEDATTRIBUTESSIZE 4
static const UA_QualifiedName protectedAttributes[UA_PROTECTEDATTRIBUTESSIZE] = {
    {0, UA_STRING_STATIC("localeIds")},
    {0, UA_STRING_STATIC("clientDescription")},
    {0, UA_STRING_STATIC("sessionName")},
    {0, UA_STRING_STATIC("clientUserId")}
};

static UA_Boolean
protectedAttribute(const UA_QualifiedName key) {
    for(size_t i = 0; i < UA_PROTECTEDATTRIBUTESSIZE; i++) {
        if(UA_QualifiedName_equal(&key, &protectedAttributes[i]))
            return true;
    }
    return false;
}

UA_StatusCode
UA_Server_setSessionAttribute(UA_Server *server, const UA_NodeId *sessionId,
                              const UA_QualifiedName key, const UA_Variant *value) {
    if(protectedAttribute(key))
        return UA_STATUSCODE_BADNOTWRITABLE;
    UA_LOCK(&server->serviceMutex);
    UA_Session *session = getSessionById(server, sessionId);
    UA_StatusCode res = UA_STATUSCODE_BADSESSIONIDINVALID;
    if(session)
        res = UA_KeyValueMap_set(session->attributes,
                                 key, value);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_deleteSessionAttribute(UA_Server *server, const UA_NodeId *sessionId,
                                 const UA_QualifiedName key) {
    if(protectedAttribute(key))
        return UA_STATUSCODE_BADNOTWRITABLE;
    UA_LOCK(&server->serviceMutex);
    UA_Session *session = getSessionById(server, sessionId);
    if(!session) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADSESSIONIDINVALID;
    }
    UA_StatusCode res =
        UA_KeyValueMap_remove(session->attributes, key);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

static UA_StatusCode
getSessionAttribute(UA_Server *server, const UA_NodeId *sessionId,
                    const UA_QualifiedName key, UA_Variant *outValue,
                    UA_Boolean copy) {
    if(!outValue)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_Session *session = getSessionById(server, sessionId);
    if(!session)
        return UA_STATUSCODE_BADSESSIONIDINVALID;

    const UA_Variant *attr;
    UA_Variant localAttr;

    if(UA_QualifiedName_equal(&key, &protectedAttributes[0])) {
        /* Return LocaleIds */
        UA_Variant_setArray(&localAttr, session->localeIds,
                            session->localeIdsSize, &UA_TYPES[UA_TYPES_STRING]);
        attr = &localAttr;
    } else if(UA_QualifiedName_equal(&key, &protectedAttributes[1])) {
        /* Return client description */
        UA_Variant_setScalar(&localAttr, &session->clientDescription,
                             &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);
        attr = &localAttr;
    } else if(UA_QualifiedName_equal(&key, &protectedAttributes[2])) {
        /* Return session name */
        UA_Variant_setScalar(&localAttr, &session->sessionName,
                             &UA_TYPES[UA_TYPES_STRING]);
        attr = &localAttr;
    } else if(UA_QualifiedName_equal(&key, &protectedAttributes[3])) {
        /* Return client user id */
        UA_Variant_setScalar(&localAttr, &session->clientUserIdOfSession,
                             &UA_TYPES[UA_TYPES_STRING]);
        attr = &localAttr;
    } else {
        /* Get from the actual key-value list */
        attr = UA_KeyValueMap_get(session->attributes, key);
        if(!attr)
            return UA_STATUSCODE_BADNOTFOUND;
    }

    if(copy)
        return UA_Variant_copy(attr, outValue);

    *outValue = *attr;
    outValue->storageType = UA_VARIANT_DATA_NODELETE;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_getSessionAttribute(UA_Server *server, const UA_NodeId *sessionId,
                              const UA_QualifiedName key, UA_Variant *outValue) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = getSessionAttribute(server, sessionId, key, outValue, false);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_getSessionAttributeCopy(UA_Server *server, const UA_NodeId *sessionId,
                                  const UA_QualifiedName key, UA_Variant *outValue) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = getSessionAttribute(server, sessionId, key, outValue, true);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_getSessionAttribute_scalar(UA_Server *server,
                                     const UA_NodeId *sessionId,
                                     const UA_QualifiedName key,
                                     const UA_DataType *type,
                                     void *outValue) {
    UA_LOCK(&server->serviceMutex);

    UA_Variant attr;
    UA_StatusCode res = getSessionAttribute(server, sessionId, key, &attr, false);
    if(res != UA_STATUSCODE_GOOD) {
        UA_UNLOCK(&server->serviceMutex);
        return res;
    }

    if(!UA_Variant_hasScalarType(&attr, type)) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    memcpy(outValue, attr.data, type->memSize);

    UA_UNLOCK(&server->serviceMutex);
    return UA_STATUSCODE_GOOD;
}
