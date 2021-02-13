/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2018 (c) Thomas Stalder, Blue Time Concept SA
 *    Copyright 2019 (c) HMS Industrial Networks AB (Author: Jonas Green)
 */

#include "ua_session.h"
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
    UA_LOCK_ASSERT(server->serviceMutex, 1);

    /* Remove all Subscriptions. This may send out remaining publish
     * responses. */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_Subscription *sub, *tempsub;
    TAILQ_FOREACH_SAFE(sub, &session->subscriptions, sessionListEntry, tempsub) {
        UA_Subscription_delete(server, sub);
    }
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
}

void
UA_Session_attachToSecureChannel(UA_Session *session, UA_SecureChannel *channel) {
    UA_Session_detachFromSecureChannel(session);
    session->header.channel = channel;
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
        generateNonce(channel->securityPolicy, &session->serverNonce);
}

void UA_Session_updateLifetime(UA_Session *session) {
    session->validTill = UA_DateTime_nowMonotonic() +
        (UA_DateTime)(session->timeout * UA_DATETIME_MSEC);
}

#ifdef UA_ENABLE_SUBSCRIPTIONS

void
UA_Session_attachSubscription(UA_Session *session, UA_Subscription *sub) {
    /* Attach to the session */
    sub->session = session;
    TAILQ_INSERT_TAIL(&session->subscriptions, sub, sessionListEntry);

    /* Increase the count */
    session->subscriptionsSize++;

    /* Increase the number of outstanding retransmissions */
    session->totalRetransmissionQueueSize += sub->retransmissionQueueSize;
}

void
UA_Session_detachSubscription(UA_Server *server, UA_Session *session,
                              UA_Subscription *sub) {
    /* Detach from the session */
    sub->session = NULL;
    TAILQ_REMOVE(&session->subscriptions, sub, sessionListEntry);

    /* Reduce the count */
    UA_assert(session->subscriptionsSize > 0);
    session->subscriptionsSize--;

    /* Reduce the number of outstanding retransmissions */
    session->totalRetransmissionQueueSize -= sub->retransmissionQueueSize;
    
    /* Send remaining publish responses if the last subscription was removed */
    if(!TAILQ_EMPTY(&session->subscriptions))
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
        session->numPublishReq--;
    }
    return entry;
}

void
UA_Session_queuePublishReq(UA_Session *session, UA_PublishResponseEntry* entry, UA_Boolean head) {
    if(!head)
        SIMPLEQ_INSERT_TAIL(&session->responseQueue, entry, listEntry);
    else
        SIMPLEQ_INSERT_HEAD(&session->responseQueue, entry, listEntry);
    session->numPublishReq++;
}

#endif
