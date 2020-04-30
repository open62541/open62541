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
    UA_NodeId authenticationToken;
    UA_SecureChannel *channel; /* The pointer back to the SecureChannel in the session. */
} UA_SessionHeader;

/* For chunked requests */
typedef struct UA_Chunk {
    SIMPLEQ_ENTRY(UA_Chunk) pointers;
    UA_ByteString bytes;
    UA_MessageType messageType;
    UA_ChunkType chunkType;
    UA_Boolean copied; /* Do the bytes point to a buffer from the network or was
                        * memory allocated for the chunk separately */
    UA_Boolean decrypted; /* The chunk has been decrypted */
} UA_Chunk;

typedef SIMPLEQ_HEAD(UA_ChunkQueue, UA_Chunk) UA_ChunkQueue;

struct UA_SecureChannel {
    UA_SecureChannelState   state;
    UA_MessageSecurityMode  securityMode;
    UA_ConnectionConfig     config;

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

    /* The standard does not forbid a SecureChannel to carry several Sessions.
     * But this is not supported here. So clients need one SecureChannel for
     * every Session. */
    UA_SessionHeader *session;

    /* If a buffer is received, first all chunks are put into the completeChunks
     * queue. Then they are processed in order. This ensures that processing
     * buffers is reentrant with the correct processing order. (This has lead to
     * problems in the client in the past.) */
    UA_ChunkQueue completeChunks; /* Received full chunks that have not been
                                   * processed so far */
    UA_ByteString incompleteChunk; /* A half-received chunk (TCP is a
                                    * streaming protocol) is stored here */
};

void UA_SecureChannel_init(UA_SecureChannel *channel,
                           const UA_ConnectionConfig *config);

void UA_SecureChannel_close(UA_SecureChannel *channel);

/* Process the remote configuration in the HEL/ACK handshake. The connection
 * config is initialized with the local settings. */
UA_StatusCode
UA_SecureChannel_processHELACK(UA_SecureChannel *channel,
                               const UA_TcpAcknowledgeMessage *remoteConfig);

UA_StatusCode
UA_SecureChannel_setSecurityPolicy(UA_SecureChannel *channel,
                                   const UA_SecurityPolicy *securityPolicy,
                                   const UA_ByteString *remoteCertificate);

/* Remove (partially) received unprocessed chunks */
void
UA_SecureChannel_deleteBuffered(UA_SecureChannel *channel);

void UA_SecureChannel_deleteMembers(UA_SecureChannel *channel);

/* Generates new keys and sets them in the channel context */
UA_StatusCode
UA_SecureChannel_generateNewKeys(UA_SecureChannel* channel);

/* Wrapper function for generating a local nonce for the supplied channel. Uses
 * the random generator of the channels security policy to allocate and generate
 * a nonce with the specified length. */
UA_StatusCode
UA_SecureChannel_generateLocalNonce(UA_SecureChannel *channel);

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

typedef void
(UA_ProcessMessageCallback)(void *application, UA_SecureChannel *channel,
                            UA_MessageType messageType, UA_UInt32 requestId,
                            UA_ByteString *message);

/* Process a received buffer. The callback function is called with the message
 * body if the message is complete. The message is removed afterwards. Returns
 * if an irrecoverable error occured.
 *
 * Note that only MSG and CLO messages are decrypted. HEL/ACK/OPN/... are
 * forwarded verbatim to the application. */
UA_StatusCode
UA_SecureChannel_processBuffer(UA_SecureChannel *channel, void *application,
                               UA_ProcessMessageCallback callback,
                               const UA_ByteString *buffer);

/* Try to receive at least one complete chunk on the connection. This blocks the
 * current thread up to the given timeout. It will return once the first buffer
 * has been received (and possibly processed when the message is complete).
 *
 * @param channel The SecureChannel
 * @param application The client or server application
 * @param callback The function pointer for processing complete messages
 * @param timeout The timeout (in milliseconds) the method will block at most.
 * @return Returns UA_STATUSCODE_GOOD or an error code. A timeout does not
 *         create an error. */
UA_StatusCode
UA_SecureChannel_receive(UA_SecureChannel *channel, void *application,
                         UA_ProcessMessageCallback callback, UA_UInt32 timeout);

/* Internal methods in ua_securechannel_crypto.h */

void
hideBytesAsym(const UA_SecureChannel *channel, UA_Byte **buf_start,
              const UA_Byte **buf_end);

/* Decrypt and verify via the signature. The chunk buffer is reused to hold the
 * decrypted data after the MessageHeader and SecurityHeader. The chunk length
 * is reduced by the signature, padding and encryption overhead.
 *
 * The offset argument points to the start of the encrypted content (beginning
 * with the SequenceHeader).*/
UA_StatusCode
decryptAndVerifyChunk(const UA_SecureChannel *channel,
                      const UA_SecurityPolicyCryptoModule *cryptoModule,
                      UA_MessageType messageType, UA_ByteString *chunk,
                      size_t offset);

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
checkSymHeader(UA_SecureChannel *channel, UA_UInt32 tokenId);

UA_StatusCode
processSequenceNumberAsym(UA_SecureChannel *channel, UA_UInt32 sequenceNumber);

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
                 "Connection %i | SecureChannel %" PRIi32 " | " MSG "%.0s",     \
                 ((CHANNEL)->connection ? (int)((CHANNEL)->connection->sockfd) : 0), \
                 (CHANNEL)->securityToken.channelId, __VA_ARGS__)

#define UA_LOG_TRACE_CHANNEL(LOGGER, CHANNEL, ...)        \
    UA_MACRO_EXPAND(UA_LOG_TRACE_CHANNEL_INTERNAL(LOGGER, CHANNEL, __VA_ARGS__, ""))

#define UA_LOG_DEBUG_CHANNEL_INTERNAL(LOGGER, CHANNEL, MSG, ...)              \
    UA_LOG_DEBUG(LOGGER, UA_LOGCATEGORY_SECURECHANNEL,                        \
                 "Connection %i | SecureChannel %" PRIi32 " | " MSG "%.0s",     \
                 ((CHANNEL)->connection ? (int)((CHANNEL)->connection->sockfd) : 0), \
                 (CHANNEL)->securityToken.channelId, __VA_ARGS__)

#define UA_LOG_DEBUG_CHANNEL(LOGGER, CHANNEL, ...)        \
    UA_MACRO_EXPAND(UA_LOG_DEBUG_CHANNEL_INTERNAL(LOGGER, CHANNEL, __VA_ARGS__, ""))

#define UA_LOG_INFO_CHANNEL_INTERNAL(LOGGER, CHANNEL, MSG, ...)               \
    UA_LOG_INFO(LOGGER, UA_LOGCATEGORY_SECURECHANNEL,                         \
                 "Connection %i | SecureChannel %" PRIi32 " | " MSG "%.0s",     \
                 ((CHANNEL)->connection ? (int)((CHANNEL)->connection->sockfd) : 0), \
                 (CHANNEL)->securityToken.channelId, __VA_ARGS__)

#define UA_LOG_INFO_CHANNEL(LOGGER, CHANNEL, ...)        \
    UA_MACRO_EXPAND(UA_LOG_INFO_CHANNEL_INTERNAL(LOGGER, CHANNEL, __VA_ARGS__, ""))

#define UA_LOG_WARNING_CHANNEL_INTERNAL(LOGGER, CHANNEL, MSG, ...)            \
    UA_LOG_WARNING(LOGGER, UA_LOGCATEGORY_SECURECHANNEL,                      \
                 "Connection %i | SecureChannel %" PRIi32 " | " MSG "%.0s",     \
                 ((CHANNEL)->connection ? (int)((CHANNEL)->connection->sockfd) : 0), \
                 (CHANNEL)->securityToken.channelId, __VA_ARGS__)

#define UA_LOG_WARNING_CHANNEL(LOGGER, CHANNEL, ...)        \
    UA_MACRO_EXPAND(UA_LOG_WARNING_CHANNEL_INTERNAL(LOGGER, CHANNEL, __VA_ARGS__, ""))

#define UA_LOG_ERROR_CHANNEL_INTERNAL(LOGGER, CHANNEL, MSG, ...)              \
    UA_LOG_ERROR(LOGGER, UA_LOGCATEGORY_SECURECHANNEL,                        \
                 "Connection %i | SecureChannel %" PRIi32 " | " MSG "%.0s",     \
                 ((CHANNEL)->connection ? (int)((CHANNEL)->connection->sockfd) : 0), \
                 (CHANNEL)->securityToken.channelId, __VA_ARGS__)

#define UA_LOG_ERROR_CHANNEL(LOGGER, CHANNEL, ...)        \
    UA_MACRO_EXPAND(UA_LOG_ERROR_CHANNEL_INTERNAL(LOGGER, CHANNEL, __VA_ARGS__, ""))

#define UA_LOG_FATAL_CHANNEL_INTERNAL(LOGGER, CHANNEL, MSG, ...)              \
    UA_LOG_FATAL(LOGGER, UA_LOGCATEGORY_SECURECHANNEL,                        \
                 "Connection %i | SecureChannel %" PRIi32 " | " MSG "%.0s",     \
                 ((CHANNEL)->connection ? (CHANNEL)->connection->sockfd : 0), \
                 (CHANNEL)->securityToken.channelId, __VA_ARGS__)

#define UA_LOG_FATAL_CHANNEL(LOGGER, CHANNEL, ...)        \
    UA_MACRO_EXPAND(UA_LOG_FATAL_CHANNEL_INTERNAL(LOGGER, CHANNEL, __VA_ARGS__, ""))

_UA_END_DECLS

#endif /* UA_SECURECHANNEL_H_ */
