#ifndef UA_SECURECHANNEL_H_
#define UA_SECURECHANNEL_H_

#include "queue.h"
#include "ua_types.h"
#include "ua_transport_generated.h"
#include "ua_connection_internal.h"

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
    UA_MessageSecurityMode  securityMode;
    UA_ChannelSecurityToken securityToken; // the channelId is contained in the securityToken
    UA_ChannelSecurityToken nextSecurityToken; // the channelId is contained in the securityToken
    UA_AsymmetricAlgorithmSecurityHeader clientAsymAlgSettings;
    UA_AsymmetricAlgorithmSecurityHeader serverAsymAlgSettings;
    UA_ByteString  clientNonce;
    UA_ByteString  serverNonce;
    UA_UInt32      receiveSequenceNumber;
    UA_UInt32      sendSequenceNumber;
    UA_Connection *connection;
    LIST_HEAD(session_pointerlist, SessionEntry) sessions;
    LIST_HEAD(chunk_pointerlist, ChunkEntry) chunks;
};

void UA_SecureChannel_init(UA_SecureChannel *channel);
void UA_SecureChannel_deleteMembersCleanup(UA_SecureChannel *channel);

UA_StatusCode UA_SecureChannel_generateNonce(UA_ByteString *nonce);

void UA_SecureChannel_attachSession(UA_SecureChannel *channel, UA_Session *session);
void UA_SecureChannel_detachSession(UA_SecureChannel *channel, UA_Session *session);
UA_Session * UA_SecureChannel_getSession(UA_SecureChannel *channel, UA_NodeId *token);

UA_StatusCode UA_SecureChannel_sendBinaryMessage(UA_SecureChannel *channel, UA_UInt32 requestId,
                                                  const void *content, const UA_DataType *contentType);

void UA_SecureChannel_revolveTokens(UA_SecureChannel *channel);

/**
 * Chunking
 * -------- */
/* Offset is initially set to the beginning of the chunk content. chunklength is
   the length of the decoded chunk content (minus header, padding, etc.) */
void UA_SecureChannel_appendChunk(UA_SecureChannel *channel, UA_UInt32 requestId,
                                  const UA_ByteString *msg, size_t offset, size_t chunklength);

/* deleteChunk indicates if the returned bytestring was copied off the network
   buffer (and needs to be freed) or points into the msg */
UA_ByteString UA_SecureChannel_finalizeChunk(UA_SecureChannel *channel, UA_UInt32 requestId,
                                             const UA_ByteString *msg, size_t offset, size_t chunklength,
                                             UA_Boolean *deleteChunk);

void UA_SecureChannel_removeChunk(UA_SecureChannel *channel, UA_UInt32 requestId);

UA_StatusCode UA_SecureChannel_processSequenceNumber (UA_UInt32 SequenceNumber, UA_SecureChannel *channel);
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

#endif /* UA_SECURECHANNEL_H_ */
