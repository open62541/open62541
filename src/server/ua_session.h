/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2019 (c) HMS Industrial Networks AB (Author: Jonas Green)
 */

#ifndef UA_SESSION_H_
#define UA_SESSION_H_

#include <open62541/util.h>

#include "ua_securechannel.h"

_UA_BEGIN_DECLS

#define UA_MAXCONTINUATIONPOINTS 5

struct ContinuationPoint;
typedef struct ContinuationPoint ContinuationPoint;

/* Returns the next entry in the linked list */
ContinuationPoint *
ContinuationPoint_clear(ContinuationPoint *cp);

struct UA_Subscription;
typedef struct UA_Subscription UA_Subscription;

#ifdef UA_ENABLE_SUBSCRIPTIONS
typedef struct UA_PublishResponseEntry {
    SIMPLEQ_ENTRY(UA_PublishResponseEntry) listEntry;
    UA_UInt32 requestId;
    UA_DateTime maxTime; /* Based on the TimeoutHint of the request */
    UA_PublishResponse response;
} UA_PublishResponseEntry;
#endif

struct UA_Session {
    UA_Session *next; /* singly-linked list */
    UA_SecureChannel *channel; /* The pointer back to the SecureChannel in the session. */

    UA_NodeId sessionId;
    UA_NodeId authenticationToken;
    UA_String sessionName;
    UA_Boolean activated;

    void *context; /* Pointer assigned by the user in the
                    * accessControl->activateSession context */

    UA_ByteString serverNonce;

    UA_ApplicationDescription clientDescription;
    UA_String clientUserIdOfSession;
    UA_Double timeout; /* in ms */
    UA_DateTime validTill;

    UA_KeyValueMap *attributes;

    /* TODO: Currently unused */
    UA_UInt32 maxRequestMessageSize;
    UA_UInt32 maxResponseMessageSize;

    UA_UInt16         availableContinuationPoints;
    ContinuationPoint *continuationPoints;

    /* Localization information */
    size_t localeIdsSize;
    UA_String *localeIds;

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* The queue is ordered according to the priority byte (higher bytes come
     * first). When a late subscription finally publishes, then it is pushed to
     * the back within the sub-set of subscriptions that has the same priority
     * (round-robin scheduling). */
    size_t subscriptionsSize;
    TAILQ_HEAD(, UA_Subscription) subscriptions;

    size_t responseQueueSize;
    SIMPLEQ_HEAD(, UA_PublishResponseEntry) responseQueue;

    size_t totalRetransmissionQueueSize; /* Retransmissions of all subscriptions */
#endif

#ifdef UA_ENABLE_DIAGNOSTICS
    UA_SessionSecurityDiagnosticsDataType securityDiagnostics;
    UA_SessionDiagnosticsDataType diagnostics;
#endif
};

/**
 * Session Lifecycle
 * ----------------- */

void UA_Session_init(UA_Session *session);
void UA_Session_clear(UA_Session *session, UA_Server *server);
void UA_Session_attachToSecureChannel(UA_Session *session, UA_SecureChannel *channel);
void UA_Session_detachFromSecureChannel(UA_Session *session);
UA_StatusCode UA_Session_generateNonce(UA_Session *session);

/* If any activity on a session happens, the timeout is extended */
void UA_Session_updateLifetime(UA_Session *session, UA_DateTime now,
                               UA_DateTime nowMonotonic);

/**
 * Subscription handling
 * --------------------- */

#ifdef UA_ENABLE_SUBSCRIPTIONS

void
UA_Session_attachSubscription(UA_Session *session, UA_Subscription *sub);

/* If releasePublishResponses is true and the last subscription is removed, all
 * outstanding PublishResponse are sent with a StatusCode. But we don't do that
 * if a Subscription is only detached for modification. */
void
UA_Session_detachSubscription(UA_Server *server, UA_Session *session,
                              UA_Subscription *sub, UA_Boolean releasePublishResponses);

UA_Subscription *
UA_Session_getSubscriptionById(UA_Session *session,
                               UA_UInt32 subscriptionId);


void
UA_Session_queuePublishReq(UA_Session *session,
                           UA_PublishResponseEntry* entry,
                           UA_Boolean head);

UA_PublishResponseEntry *
UA_Session_dequeuePublishReq(UA_Session *session);

#endif

/**
 * Log Helper
 * ----------
 * We have to jump through some hoops to enable the use of format strings
 * without arguments since (pedantic) C99 does not allow variadic macros with
 * zero arguments. So we add a dummy argument that is not printed (%.0s is
 * string of length zero). */

#define UA_LOG_SESSION_INTERNAL(LOGGER, LEVEL, SESSION, MSG, ...)       \
    if(UA_LOGLEVEL <= UA_LOGLEVEL_##LEVEL) {                           \
        int nameLen = (SESSION) ? (int)(SESSION)->sessionName.length : 0; \
        const char *nameStr = (SESSION) ?                               \
            (const char*)(SESSION)->sessionName.data : "";              \
        unsigned long sockId = ((SESSION) && (SESSION)->channel) ? \
            (unsigned long)(SESSION)->channel->connectionId : 0; \
        UA_UInt32 chanId = ((SESSION) && (SESSION)->channel) ?   \
            (SESSION)->channel->securityToken.channelId : 0;     \
        UA_LOG_##LEVEL(LOGGER, UA_LOGCATEGORY_SESSION,                  \
                       "TCP %lu\t| SC %" PRIu32 "\t| Session \"%.*s\"\t| " MSG "%.0s", \
                       sockId, chanId, nameLen, nameStr, __VA_ARGS__);   \
    }

#define UA_LOG_TRACE_SESSION(LOGGER, SESSION, ...)                      \
    UA_MACRO_EXPAND(UA_LOG_SESSION_INTERNAL(LOGGER, TRACE, SESSION, __VA_ARGS__, ""))
#define UA_LOG_DEBUG_SESSION(LOGGER, SESSION, ...)                      \
    UA_MACRO_EXPAND(UA_LOG_SESSION_INTERNAL(LOGGER, DEBUG, SESSION, __VA_ARGS__, ""))
#define UA_LOG_INFO_SESSION(LOGGER, SESSION, ...)                       \
    UA_MACRO_EXPAND(UA_LOG_SESSION_INTERNAL(LOGGER, INFO, SESSION, __VA_ARGS__, ""))
#define UA_LOG_WARNING_SESSION(LOGGER, SESSION, ...)                    \
    UA_MACRO_EXPAND(UA_LOG_SESSION_INTERNAL(LOGGER, WARNING, SESSION, __VA_ARGS__, ""))
#define UA_LOG_ERROR_SESSION(LOGGER, SESSION, ...)                      \
    UA_MACRO_EXPAND(UA_LOG_SESSION_INTERNAL(LOGGER, ERROR, SESSION, __VA_ARGS__, ""))
#define UA_LOG_FATAL_SESSION(LOGGER, SESSION, ...)                      \
    UA_MACRO_EXPAND(UA_LOG_SESSION_INTERNAL(LOGGER, FATAL, SESSION, __VA_ARGS__, ""))

_UA_END_DECLS

#endif /* UA_SESSION_H_ */
