/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef UA_SECURECHANNEL_H_
#define UA_SECURECHANNEL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "queue.h"
#include "ua_types.h"
#include "ua_transport_generated.h"
#include "ua_connection_internal.h"
#include "ua_securitycontext.h"
#include "ua_securitypolicy.h"

struct UA_Session;
typedef struct UA_Session UA_Session;

struct SessionEntry {
    LIST_ENTRY(SessionEntry) pointers;
    UA_Session *session; // Just a pointer. The session is held in the session manager or the client
};

/* For chunked requests */
struct ChunkEntry {
    LIST_ENTRY(ChunkEntry) pointers;
    UA_UInt32 requestId;
    UA_ByteString bytes;
};

/* For chunked responses */
typedef struct {
    UA_SecureChannel *channel;
    UA_UInt32 requestId;
    UA_UInt32 messageType;
    UA_UInt16 chunksSoFar;
    size_t messageSizeSoFar;
    UA_Boolean final;
    UA_StatusCode errorCode;
} UA_ChunkInfo;

struct UA_SecureChannel {
    UA_Boolean temporary; // this flag is set to false if the channel is fully opened.
    UA_MessageSecurityMode  securityMode;
    UA_ChannelSecurityToken securityToken; // the channelId is contained in the securityToken
    UA_ChannelSecurityToken nextSecurityToken; // the channelId is contained in the securityToken
    UA_AsymmetricAlgorithmSecurityHeader clientAsymAlgSettings;
    UA_AsymmetricAlgorithmSecurityHeader serverAsymAlgSettings;

    /** The active security policy and context of the channel */
    const UA_SecurityPolicy* securityPolicy;
    UA_Channel_SecurityContext* securityContext;

    /** Stores all available security policies that may be used when establishing a connection. */
    UA_SecurityPolicies availableSecurityPolicies;

    UA_ByteString  clientNonce;
    UA_ByteString  serverNonce;
    UA_UInt32      receiveSequenceNumber;
    UA_UInt32      sendSequenceNumber;
    UA_Connection *connection;

    UA_Logger logger;

    LIST_HEAD(session_pointerlist, SessionEntry) sessions;
    LIST_HEAD(chunk_pointerlist, ChunkEntry) chunks;
};

/**
 * \brief Initializes the secure channel.
 *
 * \param channel the channel to initialize.
 * \param securityPolicies the securityPolicies struct that contains all available policies
 *                         the channel may choose from when a channel is being established.
 * \param logger the logger the securechannel may use to log messages.
 */
void UA_SecureChannel_init(UA_SecureChannel *channel, UA_SecurityPolicies securityPolicies, UA_Logger logger);
void UA_SecureChannel_deleteMembersCleanup(UA_SecureChannel *channel);

/**
 * Generates a nonce.
 *
 * Uses the random generator of the supplied security policy
 *
 * \param nonce will contain the nonce after being successfully called.
 * \param securityPolicy the SecurityPolicy to use.
 */
UA_StatusCode UA_SecureChannel_generateNonce(UA_ByteString* const nonce, const UA_SecurityPolicy* const securityPolicy);

/**
 * Generates new keys and sets them in the channel context
 *
 * \param channel the channel to generate new keys for
 */
UA_StatusCode UA_SecureChannel_generateNewKeys(UA_SecureChannel* const channel);

void UA_SecureChannel_attachSession(UA_SecureChannel *channel, UA_Session *session);
void UA_SecureChannel_detachSession(UA_SecureChannel *channel, UA_Session *session);
UA_Session * UA_SecureChannel_getSession(UA_SecureChannel *channel, UA_NodeId *token);

UA_StatusCode UA_SecureChannel_sendBinaryMessage(UA_SecureChannel *channel, UA_UInt32 requestId,
                                                  const void *content, const UA_DataType *contentType);

void UA_SecureChannel_revolveTokens(UA_SecureChannel *channel);

/**
 * Chunking
 * -------- */
typedef void
(UA_ProcessMessageCallback)(void *application, UA_SecureChannel *channel,
                             UA_MessageType messageType, UA_UInt32 requestId,
                             const UA_ByteString *message);

UA_StatusCode
UA_SecureChannel_processChunks(UA_SecureChannel *channel, const UA_ByteString *chunks,
                               UA_ProcessMessageCallback callback, void *application);

/**
 * Log Helper
 * ---------- */
#define UA_LOG_TRACE_CHANNEL(LOGGER, CHANNEL, MSG, ...)                 \
    UA_LOG_TRACE(LOGGER, UA_LOGCATEGORY_SECURECHANNEL, "Connection %i | SecureChannel %i | " MSG, \
                 ((CHANNEL)->connection ? CHANNEL->connection->sockfd : 0), \
                 (CHANNEL)->securityToken.channelId, ##__VA_ARGS__);

#define UA_LOG_DEBUG_CHANNEL(LOGGER, CHANNEL, MSG, ...)                 \
    UA_LOG_DEBUG(LOGGER, UA_LOGCATEGORY_SECURECHANNEL, "Connection %i | SecureChannel %i | " MSG, \
                 ((CHANNEL)->connection ? (CHANNEL)->connection->sockfd : 0), \
                 (CHANNEL)->securityToken.channelId, ##__VA_ARGS__);

#define UA_LOG_INFO_CHANNEL(LOGGER, CHANNEL, MSG, ...)                   \
    UA_LOG_INFO(LOGGER, UA_LOGCATEGORY_SECURECHANNEL, "Connection %i | SecureChannel %i | " MSG, \
                ((CHANNEL)->connection ? (CHANNEL)->connection->sockfd : 0), \
                (CHANNEL)->securityToken.channelId, ##__VA_ARGS__);

#define UA_LOG_WARNING_CHANNEL(LOGGER, CHANNEL, MSG, ...)               \
    UA_LOG_WARNING(LOGGER, UA_LOGCATEGORY_SECURECHANNEL, "Connection %i | SecureChannel %i | " MSG, \
                   ((CHANNEL)->connection ? (CHANNEL)->connection->sockfd : 0), \
                   (CHANNEL)->securityToken.channelId, ##__VA_ARGS__);

#define UA_LOG_ERROR_CHANNEL(LOGGER, CHANNEL, MSG, ...)                 \
    UA_LOG_ERROR(LOGGER, UA_LOGCATEGORY_SECURECHANNEL, "Connection %i | SecureChannel %i | " MSG, \
                 ((CHANNEL)->connection ? (CHANNEL)->connection->sockfd : 0), \
                 (CHANNEL)->securityToken.channelId, ##__VA_ARGS__);

#define UA_LOG_FATAL_CHANNEL(LOGGER, CHANNEL, MSG, ...)                 \
    UA_LOG_FATAL(LOGGER, UA_LOGCATEGORY_SECURECHANNEL, "Connection %i | SecureChannel %i | " MSG, \
                 ((CHANNEL)->connection ? (CHANNEL)->connection->sockfd : 0), \
                 (CHANNEL)->securityToken.channelId, ##__VA_ARGS__);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_SECURECHANNEL_H_ */
