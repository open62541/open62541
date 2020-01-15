/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2014-2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Florian Palm
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Mark Giraud, Fraunhofer IOSB
 */

#ifndef UA_SECURECHANNEL_H_
#define UA_SECURECHANNEL_H_

#include <open62541/plugin/log.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/transport_generated.h>
#include <open62541/types.h>

#include "open62541_queue.h"
#include "ua_connection_internal.h"

_UA_BEGIN_DECLS

#define UA_SECURE_CONVERSATION_MESSAGE_HEADER_LENGTH 12
#define UA_SECURE_MESSAGE_HEADER_LENGTH 24

/* Thread-local variables to force failure modes during testing */
#ifdef UA_ENABLE_UNIT_TEST_FAILURE_HOOKS
extern UA_StatusCode decrypt_verifySignatureFailure;
extern UA_StatusCode sendAsym_sendFailure;
extern UA_StatusCode processSym_seqNumberFailure;
#endif

/* The Session implementation differs between client and server. Still, it is
 * expected that the Session structure begins with the SessionHeader. This is
 * the interface that will be used by the SecureChannel. The lifecycle of
 * Sessions is independent of the underlying SecureChannel. But every Session
 * can be attached to only one SecureChannel. */
typedef struct UA_SessionHeader {
    LIST_ENTRY(UA_SessionHeader) pointers;
    UA_NodeId authenticationToken;
    UA_SecureChannel *channel; /* The pointer back to the SecureChannel in the session. */
} UA_SessionHeader;

/* For chunked requests */
typedef struct UA_ChunkPayload {
    SIMPLEQ_ENTRY(UA_ChunkPayload) pointers;
    UA_ByteString bytes;
    UA_Boolean copied; /* Do the bytes point to a buffer from the network or was
                          memory allocated for the chunk separately */
} UA_ChunkPayload;

/* Receieved messages. Process them only in order. The Chunk payload has all
 * headers and the padding stripped out. The payload begins at the
 * ExtensionObject prefix.*/
typedef struct UA_Message {
    TAILQ_ENTRY(UA_Message) pointers;
    UA_UInt32 requestId;
    UA_MessageType messageType;
    SIMPLEQ_HEAD(pp, UA_ChunkPayload) chunkPayloads;
    size_t chunkPayloadsSize; /* No of chunks received so far */
    size_t messageSize; /* Total length of the chunks received so far */
    UA_Boolean final; /* All chunks for the message have been received */
} UA_Message;

typedef enum {
    UA_SECURECHANNELSTATE_FRESH,
    UA_SECURECHANNELSTATE_OPEN,
    UA_SECURECHANNELSTATE_CLOSED
} UA_SecureChannelState;

typedef TAILQ_HEAD(UA_MessageQueue, UA_Message) UA_MessageQueue;

struct UA_SecureChannel {
    UA_SecureChannelState   state;
    UA_MessageSecurityMode  securityMode;

    /* Rules for revolving the token with a renew OPN request: The client is
     * allowed to accept messages with the old token until the OPN response has
     * arrived. The server accepts the old token until one message secured with
     * the new token has arrived.
     *
     * We recognize whether nextSecurityToken contains a valid next token if the
     * ChannelId is not 0. */
    UA_ChannelSecurityToken securityToken;     /* Also contains the channelId */
    UA_ChannelSecurityToken nextSecurityToken; /* Only used by the server. The next token
                                                * is put here when sending the OPN
                                                * response. */

    /* The endpoint and context of the channel */
    const UA_SecurityPolicy *securityPolicy;
    void *channelContext; /* For interaction with the security policy */
    UA_Connection *connection;

    /* Asymmetric encryption info */
    UA_ByteString remoteCertificate;
    UA_Byte remoteCertificateThumbprint[20]; /* The thumbprint of the remote certificate */

    /* Symmetric encryption info */
    UA_ByteString remoteNonce;
    UA_ByteString localNonce;

    UA_UInt32 receiveSequenceNumber;
    UA_UInt32 sendSequenceNumber;

    LIST_HEAD(, UA_SessionHeader) sessions;
    UA_MessageQueue messages;
};

void UA_SecureChannel_init(UA_SecureChannel *channel);

void UA_SecureChannel_close(UA_SecureChannel *channel);

UA_StatusCode
UA_SecureChannel_setSecurityPolicy(UA_SecureChannel *channel,
                                   const UA_SecurityPolicy *securityPolicy,
                                   const UA_ByteString *remoteCertificate);

/* Remove (partially) received unprocessed messages */
void UA_SecureChannel_deleteMessages(UA_SecureChannel *channel);

void UA_SecureChannel_deleteMembers(UA_SecureChannel *channel);

/* Generates new keys and sets them in the channel context */
UA_StatusCode
UA_SecureChannel_generateNewKeys(UA_SecureChannel* channel);

/* Wrapper function for generating a local nonce for the supplied channel. Uses
 * the random generator of the channels security policy to allocate and generate
 * a nonce with the specified length. */
UA_StatusCode
UA_SecureChannel_generateLocalNonce(UA_SecureChannel *channel);

UA_SessionHeader *
UA_SecureChannel_getSession(UA_SecureChannel *channel,
                            const UA_NodeId *authenticationToken);

UA_StatusCode
UA_SecureChannel_revolveTokens(UA_SecureChannel *channel);

/**
 * Sending Messages
 * ---------------- */

UA_StatusCode
UA_SecureChannel_sendAsymmetricOPNMessage(UA_SecureChannel *channel, UA_UInt32 requestId,
                                          const void *content, const UA_DataType *contentType);

UA_StatusCode
UA_SecureChannel_sendSymmetricMessage(UA_SecureChannel *channel, UA_UInt32 requestId,
                                      UA_MessageType messageType, void *payload,
                                      const UA_DataType *payloadType);

/* The MessageContext is forwarded into the encoding layer so that we can send
 * chunks before continuing to encode. This lets us reuse a fixed chunk-sized
 * messages buffer. */
typedef struct {
    UA_SecureChannel *channel;
    UA_UInt32 requestId;
    UA_UInt32 messageType;

    UA_UInt16 chunksSoFar;
    size_t messageSizeSoFar;

    UA_ByteString messageBuffer;
    UA_Byte *buf_pos;
    const UA_Byte *buf_end;

    UA_Boolean final;
} UA_MessageContext;

/* Start the context of a new symmetric message. */
UA_StatusCode
UA_MessageContext_begin(UA_MessageContext *mc, UA_SecureChannel *channel,
                        UA_UInt32 requestId, UA_MessageType messageType);

/* Encode the content and send out full chunks. If the return code is good, then
 * the ChunkInfo contains encoded content that has not been sent. If the return
 * code is bad, then the ChunkInfo has been cleaned up internally. */
UA_StatusCode
UA_MessageContext_encode(UA_MessageContext *mc, const void *content,
                         const UA_DataType *contentType);

/* Sends a symmetric message already encoded in the context. The context is
 * cleaned up, also in case of errors. */
UA_StatusCode
UA_MessageContext_finish(UA_MessageContext *mc);

/* To be used when a failure occures when a MessageContext is open. Note that
 * the _encode and _finish methods will clean up internally. _abort can be run
 * on a MessageContext that has already been cleaned up before. */
void
UA_MessageContext_abort(UA_MessageContext *mc);

/**
 * Receive Message
 * --------------- */

/* Decrypt a chunk and add it to the message. Create a new message if necessary. */
UA_StatusCode
UA_SecureChannel_decryptAddChunk(UA_SecureChannel *channel, const UA_ByteString *chunk,
                                 UA_Boolean allowPreviousToken);

/* The network buffer is about to be cleared. Copy all chunks that point into
 * the network buffer into dedicated memory. */
UA_StatusCode
UA_SecureChannel_persistIncompleteMessages(UA_SecureChannel *channel);

typedef void
(UA_ProcessMessageCallback)(void *application, UA_SecureChannel *channel,
                            UA_MessageType messageType, UA_UInt32 requestId,
                            const UA_ByteString *message);

/* Process received complete messages in-order. The callback function is called
 * with the complete message body if the message is complete. The message is
 * removed afterwards.
 *
 * Symmetric callback is ERR, MSG, CLO only
 * Asymmetric callback is OPN only
 *
 * @param channel the channel the chunks were received on.
 * @param application data pointer to application specific data that gets passed
 *                    on to the callback function.
 * @param callback the callback function that gets called with the complete
 *                 message body, once a final chunk is processed.
 * @return Returns if an irrecoverable error occured. Maybe close the channel. */
UA_StatusCode
UA_SecureChannel_processCompleteMessages(UA_SecureChannel *channel, void *application,
                                         UA_ProcessMessageCallback callback);

/* Internal methods in ua_securechannel_crypto.h */

void
hideBytesAsym(const UA_SecureChannel *channel, UA_Byte **buf_start,
              const UA_Byte **buf_end);

/* Sets the payload to a pointer inside the chunk buffer. Returns the requestId
 * and the sequenceNumber */
UA_StatusCode
decryptAndVerifyChunk(const UA_SecureChannel *channel,
                      const UA_SecurityPolicyCryptoModule *cryptoModule,
                      UA_MessageType messageType, const UA_ByteString *chunk,
                      size_t offset, UA_UInt32 *requestId,
                      UA_UInt32 *sequenceNumber, UA_ByteString *payload);

size_t
calculateAsymAlgSecurityHeaderLength(const UA_SecureChannel *channel);

UA_StatusCode
prependHeadersAsym(UA_SecureChannel *const channel, UA_Byte *header_pos,
                   const UA_Byte *buf_end, size_t totalLength,
                   size_t securityHeaderLength, UA_UInt32 requestId,
                   size_t *const finalLength);

void
setBufPos(UA_MessageContext *mc);

UA_StatusCode
decryptChunk(const UA_SecureChannel *const channel,
             const UA_SecurityPolicyCryptoModule *const cryptoModule,
             UA_MessageType const messageType, const UA_ByteString *const chunk,
             size_t const offset, size_t *const chunkSizeAfterDecryption);

UA_UInt16
decodeChunkPaddingSize(const UA_SecureChannel *channel,
                       const UA_SecurityPolicyCryptoModule *cryptoModule,
                       UA_MessageType messageType, const UA_ByteString *chunk,
                       size_t chunkSizeAfterDecryption, size_t sigsize);

UA_StatusCode
verifyChunk(const UA_SecureChannel *channel,
            const UA_SecurityPolicyCryptoModule *cryptoModule,
            const UA_ByteString *chunk,
            size_t chunkSizeAfterDecryption, size_t sigsize);

UA_StatusCode
checkSymHeader(UA_SecureChannel *channel, UA_UInt32 tokenId,
               UA_Boolean allowPreviousToken);

UA_StatusCode
processSequenceNumberAsym(UA_SecureChannel *channel, UA_UInt32 sequenceNumber);

UA_StatusCode
processSequenceNumberSym(UA_SecureChannel *channel, UA_UInt32 sequenceNumber);

UA_StatusCode
checkAsymHeader(UA_SecureChannel *channel,
                const UA_AsymmetricAlgorithmSecurityHeader *asymHeader);

void
padChunkAsym(UA_SecureChannel *channel, const UA_ByteString *buf,
             size_t securityHeaderLength, UA_Byte **buf_pos);

UA_StatusCode
signAndEncryptAsym(UA_SecureChannel *channel, size_t preSignLength,
                   UA_ByteString *buf, size_t securityHeaderLength,
                   size_t totalLength);

void
padChunkSym(UA_MessageContext *messageContext, size_t bodyLength);

UA_StatusCode
signChunkSym(UA_MessageContext *const messageContext, size_t preSigLength);

UA_StatusCode
encryptChunkSym(UA_MessageContext *const messageContext, size_t totalLength);

/**
 * Log Helper
 * ----------
 * C99 requires at least one element for the variadic argument. If the log
 * statement has no variable arguments, supply an additional NULL. It will be
 * ignored by printf.
 *
 * We have to jump through some hoops to enable the use of format strings
 * without arguments since (pedantic) C99 does not allow variadic macros with
 * zero arguments. So we add a dummy argument that is not printed (%.0s is
 * string of length zero). */

#define UA_LOG_TRACE_CHANNEL_INTERNAL(LOGGER, CHANNEL, MSG, ...)              \
    UA_LOG_TRACE(LOGGER, UA_LOGCATEGORY_SECURECHANNEL,                        \
                 "Connection %i | SecureChannel %i | " MSG "%.0s",            \
                 ((CHANNEL)->connection ? (int)((CHANNEL)->connection->sockfd) : 0), \
                 (CHANNEL)->securityToken.channelId, __VA_ARGS__)

#define UA_LOG_TRACE_CHANNEL(LOGGER, CHANNEL, ...)        \
    UA_MACRO_EXPAND(UA_LOG_TRACE_CHANNEL_INTERNAL(LOGGER, CHANNEL, __VA_ARGS__, ""))

#define UA_LOG_DEBUG_CHANNEL_INTERNAL(LOGGER, CHANNEL, MSG, ...)              \
    UA_LOG_DEBUG(LOGGER, UA_LOGCATEGORY_SECURECHANNEL,                        \
                 "Connection %i | SecureChannel %i | " MSG "%.0s",            \
                 ((CHANNEL)->connection ? (int)((CHANNEL)->connection->sockfd) : 0), \
                 (CHANNEL)->securityToken.channelId, __VA_ARGS__)

#define UA_LOG_DEBUG_CHANNEL(LOGGER, CHANNEL, ...)        \
    UA_MACRO_EXPAND(UA_LOG_DEBUG_CHANNEL_INTERNAL(LOGGER, CHANNEL, __VA_ARGS__, ""))

#define UA_LOG_INFO_CHANNEL_INTERNAL(LOGGER, CHANNEL, MSG, ...)               \
    UA_LOG_INFO(LOGGER, UA_LOGCATEGORY_SECURECHANNEL,                         \
                 "Connection %i | SecureChannel %i | " MSG "%.0s",            \
                 ((CHANNEL)->connection ? (int)((CHANNEL)->connection->sockfd) : 0), \
                 (CHANNEL)->securityToken.channelId, __VA_ARGS__)

#define UA_LOG_INFO_CHANNEL(LOGGER, CHANNEL, ...)        \
    UA_MACRO_EXPAND(UA_LOG_INFO_CHANNEL_INTERNAL(LOGGER, CHANNEL, __VA_ARGS__, ""))

#define UA_LOG_WARNING_CHANNEL_INTERNAL(LOGGER, CHANNEL, MSG, ...)            \
    UA_LOG_WARNING(LOGGER, UA_LOGCATEGORY_SECURECHANNEL,                      \
                 "Connection %i | SecureChannel %i | " MSG "%.0s",            \
                 ((CHANNEL)->connection ? (int)((CHANNEL)->connection->sockfd) : 0), \
                 (CHANNEL)->securityToken.channelId, __VA_ARGS__)

#define UA_LOG_WARNING_CHANNEL(LOGGER, CHANNEL, ...)        \
    UA_MACRO_EXPAND(UA_LOG_WARNING_CHANNEL_INTERNAL(LOGGER, CHANNEL, __VA_ARGS__, ""))

#define UA_LOG_ERROR_CHANNEL_INTERNAL(LOGGER, CHANNEL, MSG, ...)              \
    UA_LOG_ERROR(LOGGER, UA_LOGCATEGORY_SECURECHANNEL,                        \
                 "Connection %i | SecureChannel %i | " MSG "%.0s",            \
                 ((CHANNEL)->connection ? (int)((CHANNEL)->connection->sockfd) : 0), \
                 (CHANNEL)->securityToken.channelId, __VA_ARGS__)

#define UA_LOG_ERROR_CHANNEL(LOGGER, CHANNEL, ...)        \
    UA_MACRO_EXPAND(UA_LOG_ERROR_CHANNEL_INTERNAL(LOGGER, CHANNEL, __VA_ARGS__, ""))

#define UA_LOG_FATAL_CHANNEL_INTERNAL(LOGGER, CHANNEL, MSG, ...)              \
    UA_LOG_FATAL(LOGGER, UA_LOGCATEGORY_SECURECHANNEL,                        \
                 "Connection %i | SecureChannel %i | " MSG "%.0s",            \
                 ((CHANNEL)->connection ? (CHANNEL)->connection->sockfd : 0), \
                 (CHANNEL)->securityToken.channelId, __VA_ARGS__)

#define UA_LOG_FATAL_CHANNEL(LOGGER, CHANNEL, ...)        \
    UA_MACRO_EXPAND(UA_LOG_FATAL_CHANNEL_INTERNAL(LOGGER, CHANNEL, __VA_ARGS__, ""))

_UA_END_DECLS

#endif /* UA_SECURECHANNEL_H_ */
