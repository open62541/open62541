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
    UA_NodeId_clear(&session->header.authenticationToken);
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

    UA_Array_delete(session->params, session->paramsSize,
                    &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
    session->params = NULL;
    session->paramsSize = 0;

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
    UA_Session_detachFromSecureChannel(session);
    session->header.channel = channel;
    session->header.serverSession = true;
    SLIST_INSERT_HEAD(&channel->sessions, &session->header, next);
}

void
UA_Session_detachFromSecureChannel(UA_Session *session) {
    UA_SecureChannel *channel = session->header.channel;
    if(!channel)
        return;
    session->header.channel = NULL;
    UA_SessionHeader *sh;
    SLIST_FOREACH(sh, &channel->sessions, next) {
        if((UA_Session*)sh != session)
            continue;
        SLIST_REMOVE(&channel->sessions, sh, UA_SessionHeader, next);
        break;
    }

    /* Clean up the response queue. Their RequestId is bound to the
     * SecureChannel so they cannot be reused. */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_PublishResponseEntry *pre;
    while((pre = UA_Session_dequeuePublishReq(session))) {
        UA_PublishResponse_clear(&pre->response);
        UA_free(pre);
    }
#endif
}

UA_StatusCode
UA_Session_generateNonce(UA_Session *session) {
    UA_SecureChannel *channel = session->header.channel;
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

void UA_Session_updateLifetime(UA_Session *session) {
    session->validTill = UA_DateTime_nowMonotonic() +
        (UA_DateTime)(session->timeout * UA_DATETIME_MSEC);
#ifdef UA_ENABLE_DIAGNOSTICS
    session->diagnostics.clientLastContactTime = UA_DateTime_now();
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
        response->responseHeader.timestamp = UA_DateTime_now();
        sendResponse(server, session, session->header.channel, pre->requestId,
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
UA_Server_getSubscriptionById(UA_Server *server, UA_UInt32 subscriptionId) {
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
    UA_PublishResponseEntry* entry = SIMPLEQ_FIRST(&session->responseQueue);
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
            UA_Server_removeSession(server, entry, UA_DIAGNOSTICEVENT_CLOSE);
            res = UA_STATUSCODE_GOOD;
            break;
        }
    }
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_setSessionParameter(UA_Server *server, const UA_NodeId *sessionId,
                              const UA_QualifiedName key, const UA_Variant *value) {
    UA_LOCK(&server->serviceMutex);
    UA_Session *session = UA_Server_getSessionById(server, sessionId);
    UA_StatusCode res = UA_STATUSCODE_BADSESSIONIDINVALID;
    if(session)
        res = UA_KeyValueMap_set(&session->params, &session->paramsSize,
                                 key, value);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

void
UA_Server_deleteSessionParameter(UA_Server *server, const UA_NodeId *sessionId,
                                 const UA_QualifiedName key) {
    UA_LOCK(&server->serviceMutex);
    UA_Session *session = UA_Server_getSessionById(server, sessionId);
    if(session)
        UA_KeyValueMap_delete(&session->params, &session->paramsSize, key);
    UA_UNLOCK(&server->serviceMutex);
}

UA_StatusCode
UA_Server_getSessionParameter(UA_Server *server, const UA_NodeId *sessionId,
                              const UA_QualifiedName key,
                              UA_Variant *outParameter) {
    UA_LOCK(&server->serviceMutex);
    if(!outParameter) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_Session *session = UA_Server_getSessionById(server, sessionId);
    if(!session) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADSESSIONIDINVALID;
    }

    const UA_Variant *param =
        UA_KeyValueMap_get(session->params, session->paramsSize, key);
    if(!param) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    UA_StatusCode res = UA_Variant_copy(param, outParameter);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_getSessionParameter_scalar(UA_Server *server, const UA_NodeId *sessionId,
                                     const UA_QualifiedName key,
                                     const UA_DataType *type,
                                     void *outParameter) {
    UA_LOCK(&server->serviceMutex);
    if(!outParameter) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_Session *session = UA_Server_getSessionById(server, sessionId);
    if(!session) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADSESSIONIDINVALID;
    }

    const UA_Variant *param =
        UA_KeyValueMap_get(session->params, session->paramsSize, key);
    if(!param || !UA_Variant_hasScalarType(param, type)) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    UA_StatusCode res = UA_copy(param->data, outParameter, type);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}
