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
    UA_PublishResponse response;
} UA_PublishResponseEntry;
#endif

typedef struct {
    UA_SessionHeader  header;
    UA_ApplicationDescription clientDescription;
    UA_String         sessionName;
    UA_Boolean        activated;
    void             *sessionHandle; /* pointer assigned in userland-callback */
    UA_NodeId         sessionId;
    UA_UInt32         maxRequestMessageSize;
    UA_UInt32         maxResponseMessageSize;
    UA_Double         timeout; /* in ms */
    UA_DateTime       validTill;
    UA_ByteString     serverNonce;
    UA_UInt16         availableContinuationPoints;
    ContinuationPoint *continuationPoints;
#ifdef UA_ENABLE_SUBSCRIPTIONS
    size_t subscriptionsSize;
    TAILQ_HEAD(, UA_Subscription) subscriptions; /* Late subscriptions that do eventually
                                                  * publish are moved to the tail. So that
                                                  * other late subscriptions are not
                                                  * starved. */
    SIMPLEQ_HEAD(, UA_PublishResponseEntry) responseQueue;
    UA_UInt32 numPublishReq;
    size_t totalRetransmissionQueueSize; /* Retransmissions of all subscriptions */
#endif
} UA_Session;

/**
 * Session Lifecycle
 * ----------------- */

void UA_Session_init(UA_Session *session);
void UA_Session_clear(UA_Session *session, UA_Server *server);
void UA_Session_attachToSecureChannel(UA_Session *session, UA_SecureChannel *channel);
void UA_Session_detachFromSecureChannel(UA_Session *session);
UA_StatusCode UA_Session_generateNonce(UA_Session *session);

/* If any activity on a session happens, the timeout is extended */
void UA_Session_updateLifetime(UA_Session *session);

/**
 * Subscription handling
 * --------------------- */

#ifdef UA_ENABLE_SUBSCRIPTIONS

void
UA_Session_attachSubscription(UA_Session *session, UA_Subscription *sub);

void
UA_Session_detachSubscription(UA_Server *server, UA_Session *session,
                              UA_Subscription *sub);

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
    do {                                                                \
        UA_String idString = UA_STRING_NULL;                            \
        UA_UInt32 channelId = 0;                                        \
        if(SESSION) {                                                   \
            UA_NodeId_print(&(SESSION)->sessionId, &idString);          \
            channelId = (SESSION)->header.channel ?                     \
                (SESSION)->header.channel->securityToken.channelId : 0; \
        }                                                               \
        UA_LOG_##LEVEL(LOGGER, UA_LOGCATEGORY_SESSION,                  \
                       "SecureChannel %i | Session %.*s | " MSG "%.0s", \
                       channelId, (int)idString.length, idString.data, __VA_ARGS__); \
        UA_String_clear(&idString);                                     \
    } while(0)

#if UA_LOGLEVEL <= 100
# define UA_LOG_TRACE_SESSION(LOGGER, SESSION, ...)                     \
    UA_MACRO_EXPAND(UA_LOG_SESSION_INTERNAL(LOGGER, TRACE, SESSION, __VA_ARGS__, ""))
#else
# define UA_LOG_TRACE_SESSION(LOGGER, SESSION, ...)
#endif

#if UA_LOGLEVEL <= 200
# define UA_LOG_DEBUG_SESSION(LOGGER, SESSION, ...)                     \
    UA_MACRO_EXPAND(UA_LOG_SESSION_INTERNAL(LOGGER, DEBUG, SESSION, __VA_ARGS__, ""))
#else
# define UA_LOG_DEBUG_SESSION(LOGGER, SESSION, ...)
#endif

#if UA_LOGLEVEL <= 300
# define UA_LOG_INFO_SESSION(LOGGER, SESSION, ...)                      \
    UA_MACRO_EXPAND(UA_LOG_SESSION_INTERNAL(LOGGER, INFO, SESSION, __VA_ARGS__, ""))
#else
# define UA_LOG_INFO_SESSION(LOGGER, SESSION, ...)
#endif

#if UA_LOGLEVEL <= 400
# define UA_LOG_WARNING_SESSION(LOGGER, SESSION, ...)                    \
    UA_MACRO_EXPAND(UA_LOG_SESSION_INTERNAL(LOGGER, WARNING, SESSION, __VA_ARGS__, ""))
#else
# define UA_LOG_WARNING_SESSION(LOGGER, SESSION, ...)
#endif

#if UA_LOGLEVEL <= 500
# define UA_LOG_ERROR_SESSION(LOGGER, SESSION, ...)                      \
    UA_MACRO_EXPAND(UA_LOG_SESSION_INTERNAL(LOGGER, ERROR, SESSION, __VA_ARGS__, ""))
#else
# define UA_LOG_ERROR_SESSION(LOGGER, SESSION, ...)
#endif

#if UA_LOGLEVEL <= 600
# define UA_LOG_FATAL_SESSION(LOGGER, SESSION, ...)                      \
    UA_MACRO_EXPAND(UA_LOG_SESSION_INTERNAL(LOGGER, FATAL, SESSION, __VA_ARGS__, ""))
#else
# define UA_LOG_FATAL_SESSION(LOGGER, SESSION, ...)
#endif

_UA_END_DECLS

#endif /* UA_SESSION_H_ */
