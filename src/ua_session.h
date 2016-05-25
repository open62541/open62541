#ifndef UA_SESSION_H_
#define UA_SESSION_H_

#include "queue.h"
#include "ua_types.h"
#include "ua_securechannel.h"
#include "ua_server.h"

#define MAXCONTINUATIONPOINTS 5

struct ContinuationPointEntry {
    LIST_ENTRY(ContinuationPointEntry) pointers;
    UA_ByteString        identifier;
    UA_BrowseDescription browseDescription;
    UA_UInt32            continuationIndex;
    UA_UInt32            maxReferences;
};

struct UA_Subscription;
typedef struct UA_Subscription UA_Subscription;

typedef struct UA_PublishResponseEntry {
    SIMPLEQ_ENTRY(UA_PublishResponseEntry) listEntry;
    UA_UInt32 requestId;
    UA_PublishResponse response;
} UA_PublishResponseEntry;

struct UA_Session {
    UA_ApplicationDescription clientDescription;
    UA_Boolean        activated;
    UA_String         sessionName;
    UA_NodeId         authenticationToken;
    UA_NodeId         sessionId;
    UA_UInt32         maxRequestMessageSize;
    UA_UInt32         maxResponseMessageSize;
    UA_Double         timeout; // [ms]
    UA_DateTime       validTill;
    UA_SecureChannel *channel;
    UA_UInt16 availableContinuationPoints;
    LIST_HEAD(ContinuationPointList, ContinuationPointEntry) continuationPoints;
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_UInt32 lastSubscriptionID;
    LIST_HEAD(UA_ListOfUASubscriptions, UA_Subscription) serverSubscriptions;
    SIMPLEQ_HEAD(UA_ListOfQueuedPublishResponses, UA_PublishResponseEntry) responseQueue;
#endif
};

/* Local access to the services (for startup and maintenance) uses this Session
 * with all possible access rights (Session ID: 1) */
extern UA_Session adminSession;

void UA_Session_init(UA_Session *session);
void UA_Session_deleteMembersCleanup(UA_Session *session, UA_Server *server);

/* If any activity on a session happens, the timeout is extended */
void UA_Session_updateLifetime(UA_Session *session);

#ifdef UA_ENABLE_SUBSCRIPTIONS
void UA_Session_addSubscription(UA_Session *session, UA_Subscription *newSubscription);

UA_Subscription *
UA_Session_getSubscriptionByID(UA_Session *session, UA_UInt32 subscriptionID);

UA_StatusCode
UA_Session_deleteSubscription(UA_Server *server, UA_Session *session,
                              UA_UInt32 subscriptionID);

UA_UInt32
UA_Session_getUniqueSubscriptionID(UA_Session *session);
#endif

/**
 * Log Helper
 * ---------- */
#define UA_LOG_TRACE_SESSION(LOGGER, SESSION, MSG, ...)                 \
    UA_LOG_TRACE(LOGGER, UA_LOGCATEGORY_SESSION, "Connection %i | SecureChannel %i | Session %i | " MSG, \
                 (SESSION->channel ? (SESSION->channel->connection ? SESSION->channel->connection->sockfd : 0) : 0), \
                 (SESSION->channel ? SESSION->channel->securityToken.channelId : 0), \
                 SESSION->sessionId.identifier.numeric, ##__VA_ARGS__);

#define UA_LOG_DEBUG_SESSION(LOGGER, SESSION, MSG, ...)                 \
    UA_LOG_DEBUG(LOGGER, UA_LOGCATEGORY_SESSION, "Connection %i | SecureChannel %i | Session %i | " MSG, \
                 (SESSION->channel ? (SESSION->channel->connection ? SESSION->channel->connection->sockfd : 0) : 0), \
                 (SESSION->channel ? SESSION->channel->securityToken.channelId : 0), \
                 SESSION->sessionId.identifier.numeric, ##__VA_ARGS__);

#define UA_LOG_INFO_SESSION(LOGGER, SESSION, MSG, ...)                  \
    UA_LOG_INFO(LOGGER, UA_LOGCATEGORY_SESSION, "Connection %i | SecureChannel %i | Session %i | " MSG, \
                 (SESSION->channel ? (SESSION->channel->connection ? SESSION->channel->connection->sockfd : 0) : 0), \
                 (SESSION->channel ? SESSION->channel->securityToken.channelId : 0), \
                 SESSION->sessionId.identifier.numeric, ##__VA_ARGS__);

#define UA_LOG_WARNING_SESSION(LOGGER, SESSION, MSG, ...)               \
    UA_LOG_WARNING(LOGGER, UA_LOGCATEGORY_SESSION, "Connection %i | SecureChannel %i | Session %i | " MSG, \
                   (SESSION->channel ? (SESSION->channel->connection ? SESSION->channel->connection->sockfd : 0) : 0), \
                   (SESSION->channel ? SESSION->channel->securityToken.channelId : 0), \
                   SESSION->sessionId.identifier.numeric, ##__VA_ARGS__);

#define UA_LOG_ERROR_SESSION(LOGGER, SESSION, MSG, ...)                 \
    UA_LOG_ERROR(LOGGER, UA_LOGCATEGORY_SESSION, "Connection %i | SecureChannel %i | Session %i | " MSG, \
                 (SESSION->channel ? (SESSION->channel->connection ? SESSION->channel->connection->sockfd : 0) : 0), \
                 (SESSION->channel ? SESSION->channel->securityToken.channelId : 0), \
                 SESSION->sessionId.identifier.numeric, ##__VA_ARGS__);

#define UA_LOG_FATAL_SESSION(LOGGER, SESSION, MSG, ...)                 \
    UA_LOG_FATAL(LOGGER, UA_LOGCATEGORY_SESSION, "Connection %i | SecureChannel %i | Session %i | " MSG, \
                 (SESSION->channel ? (SESSION->channel->connection ? SESSION->channel->connection->sockfd : 0) : 0), \
                 (SESSION->channel ? SESSION->channel->securityToken.channelId : 0), \
                 SESSION->sessionId.identifier.numeric, ##__VA_ARGS__);


#endif /* UA_SESSION_H_ */
