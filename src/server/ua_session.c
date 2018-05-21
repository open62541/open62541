/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2018 (c) Thomas Stalder, Blue Time Concept SA
 */

#include "ua_session.h"
#ifdef UA_ENABLE_SUBSCRIPTIONS
#include "ua_server_internal.h"
#include "ua_subscription.h"
#endif

#define UA_SESSION_NONCELENTH 32

UA_Session adminSession = {
    {{NULL, NULL}, /* .pointers */
     {0,UA_NODEIDTYPE_NUMERIC,{1}}, /* .authenticationToken */
     NULL,}, /* .channel */
    {{0, NULL},{0, NULL},
     {{0, NULL},{0, NULL}},
     UA_APPLICATIONTYPE_CLIENT,
     {0, NULL},{0, NULL},
     0, NULL}, /* .clientDescription */
    {sizeof("Administrator Session")-1, (UA_Byte*)"Administrator Session"}, /* .sessionName */
    false, /* .activated */
    NULL, /* .sessionHandle */
    {0,UA_NODEIDTYPE_NUMERIC,{1}}, /* .sessionId */
    UA_UINT32_MAX, /* .maxRequestMessageSize */
    UA_UINT32_MAX, /* .maxResponseMessageSize */
    (UA_Double)UA_INT64_MAX, /* .timeout */
    UA_INT64_MAX, /* .validTill */
    {0, NULL},
    UA_MAXCONTINUATIONPOINTS, /* .availableContinuationPoints */
    {NULL}, /* .continuationPoints */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    0, /* .lastSubscriptionId */
    0, /* .lastSeenSubscriptionId */
    {NULL}, /* .serverSubscriptions */
    {NULL, NULL}, /* .responseQueue */
    0, /* numSubscriptions */
    0  /* numPublishReq */
#endif
};

void UA_Session_init(UA_Session *session) {
    memset(session, 0, sizeof(UA_Session));
    session->availableContinuationPoints = UA_MAXCONTINUATIONPOINTS;
#ifdef UA_ENABLE_SUBSCRIPTIONS
    SIMPLEQ_INIT(&session->responseQueue);
#endif
}

void UA_Session_deleteMembersCleanup(UA_Session *session, UA_Server* server) {
    UA_Session_detachFromSecureChannel(session);
    UA_ApplicationDescription_deleteMembers(&session->clientDescription);
    UA_NodeId_deleteMembers(&session->header.authenticationToken);
    UA_NodeId_deleteMembers(&session->sessionId);
    UA_String_deleteMembers(&session->sessionName);
    UA_ByteString_deleteMembers(&session->serverNonce);
    struct ContinuationPointEntry *cp, *temp;
    LIST_FOREACH_SAFE(cp, &session->continuationPoints, pointers, temp) {
        LIST_REMOVE(cp, pointers);
        UA_ByteString_deleteMembers(&cp->identifier);
        UA_BrowseDescription_deleteMembers(&cp->browseDescription);
        UA_free(cp);
    }

#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_Subscription *sub, *tempsub;
    LIST_FOREACH_SAFE(sub, &session->serverSubscriptions, listEntry, tempsub) {
        UA_Session_deleteSubscription(server, session, sub->subscriptionId);
    }

    UA_PublishResponseEntry *entry;
    while((entry = UA_Session_dequeuePublishReq(session))) {
        UA_PublishResponse_deleteMembers(&entry->response);
        UA_free(entry);
    }
#endif
}

void UA_Session_attachToSecureChannel(UA_Session *session, UA_SecureChannel *channel) {
    LIST_INSERT_HEAD(&channel->sessions, &session->header, pointers);
    session->header.channel = channel;
}

void UA_Session_detachFromSecureChannel(UA_Session *session) {
    if(!session->header.channel)
        return;
    session->header.channel = NULL;
    LIST_REMOVE(&session->header, pointers);
}

UA_StatusCode
UA_Session_generateNonce(UA_Session *session) {
    UA_SecureChannel *channel = session->header.channel;
    if(!channel || !channel->securityPolicy)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Is the length of the previous nonce correct? */
    if(session->serverNonce.length != UA_SESSION_NONCELENTH) {
        UA_ByteString_deleteMembers(&session->serverNonce);
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

void UA_Session_addSubscription(UA_Session *session, UA_Subscription *newSubscription) {
    newSubscription->subscriptionId = ++session->lastSubscriptionId;

    LIST_INSERT_HEAD(&session->serverSubscriptions, newSubscription, listEntry);
    session->numSubscriptions++;
}

UA_StatusCode
UA_Session_deleteSubscription(UA_Server *server, UA_Session *session,
                              UA_UInt32 subscriptionId) {
    UA_Subscription *sub = UA_Session_getSubscriptionById(session, subscriptionId);
    if(!sub)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;

    UA_Subscription_deleteMembers(server, sub);

    /* Add a delayed callback to remove the subscription when the currently
     * scheduled jobs have completed */
    UA_StatusCode retval = UA_Server_delayedFree(server, sub);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_SESSION(server->config.logger, session,
                       "Could not remove subscription with error code %s",
                       UA_StatusCode_name(retval));
        return retval; /* Try again next time */
    }

    /* Remove from the session */
    LIST_REMOVE(sub, listEntry);
    UA_assert(session->numSubscriptions > 0);
    session->numSubscriptions--;
    return UA_STATUSCODE_GOOD;
}

UA_Subscription *
UA_Session_getSubscriptionById(UA_Session *session, UA_UInt32 subscriptionId) {
    UA_Subscription *sub;
    LIST_FOREACH(sub, &session->serverSubscriptions, listEntry) {
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
