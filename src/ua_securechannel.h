/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2020 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Florian Palm
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Mark Giraud, Fraunhofer IOSB
 */

#ifndef UA_SECURECHANNEL_H_
#define UA_SECURECHANNEL_H_

#include <open62541/util.h>
#include <open62541/types.h>
#include <open62541/plugin/log.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/plugin/eventloop.h>
#include <open62541/transport_generated.h>

#include "open62541_queue.h"
#include "util/ua_util_internal.h"

_UA_BEGIN_DECLS

struct UA_SecureChannel;
typedef struct UA_SecureChannel UA_SecureChannel;

/* Forward-Declaration so the SecureChannel can point to a singly-linked list of
 * Sessions. This is only used in the server, not in the client. */
struct UA_Session;
typedef struct UA_Session UA_Session;

/* The message header of the OPC UA binary protocol is structured as follows:
 *
 * - MessageType (3 Byte)
 * - IsFinal (1 Byte)
 * - MessageSize (4 Byte)
 * *** UA_SECURECHANNEL_MESSAGEHEADER_LENGTH ***
 * - SecureChannelId (4 Byte)
 * *** UA_SECURECHANNEL_CHANNELHEADER_LENGTH ***
 * - SecurityHeader (4 Byte TokenId for symmetric, otherwise dynamic length)
 * - SequenceHeader (8 Byte)
 *   - SequenceNumber
 *   - RequestId
 */

#define UA_SECURECHANNEL_MESSAGEHEADER_LENGTH 8
#define UA_SECURECHANNEL_CHANNELHEADER_LENGTH 12
#define UA_SECURECHANNEL_SYMMETRIC_SECURITYHEADER_LENGTH 4
#define UA_SECURECHANNEL_SEQUENCEHEADER_LENGTH 8
#define UA_SECURECHANNEL_SYMMETRIC_HEADER_UNENCRYPTEDLENGTH \
    (UA_SECURECHANNEL_CHANNELHEADER_LENGTH +                \
     UA_SECURECHANNEL_SYMMETRIC_SECURITYHEADER_LENGTH)
#define UA_SECURECHANNEL_SYMMETRIC_HEADER_TOTALLENGTH   \
    (UA_SECURECHANNEL_CHANNELHEADER_LENGTH +            \
    UA_SECURECHANNEL_SYMMETRIC_SECURITYHEADER_LENGTH +  \
     UA_SECURECHANNEL_SEQUENCEHEADER_LENGTH)

/* Minimum length of a valid message (ERR message with an empty reason) */
#define UA_SECURECHANNEL_MESSAGE_MIN_LENGTH 16

/* For chunked requests */
typedef struct UA_Chunk {
    SIMPLEQ_ENTRY(UA_Chunk) pointers;
    UA_ByteString bytes;
    UA_MessageType messageType;
    UA_ChunkType chunkType;
    UA_UInt32 requestId;
    UA_Boolean copied; /* Do the bytes point to a buffer from the network or was
                        * memory allocated for the chunk separately */
} UA_Chunk;

typedef SIMPLEQ_HEAD(UA_ChunkQueue, UA_Chunk) UA_ChunkQueue;

typedef enum {
    UA_SECURECHANNELRENEWSTATE_NORMAL,

    /* Client has sent an OPN, but not received a response so far. */
    UA_SECURECHANNELRENEWSTATE_SENT,

    /* The server waits for the first request with the new token for the rollover.
     * The new token is stored in the altSecurityToken. The configured local and
     * remote symmetric encryption keys are the old ones. */
    UA_SECURECHANNELRENEWSTATE_NEWTOKEN_SERVER,

    /* The client already uses the new token. But he waits for the server to respond
     * with the new token to complete the rollover. The old token is stored in
     * altSecurityToken. The local symmetric encryption key is new. The remote
     * encryption key is the old one. */
    UA_SECURECHANNELRENEWSTATE_NEWTOKEN_CLIENT
} UA_SecureChannelRenewState;

struct UA_SecureChannel {
    UA_SecureChannelState state;
    UA_SecureChannelRenewState renewState;
    UA_MessageSecurityMode securityMode;
    UA_ShutdownReason shutdownReason;
    UA_ConnectionConfig config;

    UA_String endpointUrl;

    /* Connection handling in the EventLoop */
    UA_ConnectionManager *connectionManager;
    uintptr_t connectionId;

    /* Rules for revolving the token with a renew OPN request: The client is
     * allowed to accept messages with the old token until the OPN response has
     * arrived. The server accepts the old token until one message secured with
     * the new token has arrived.
     *
     * We recognize whether nextSecurityToken contains a valid next token if the
     * ChannelId is not 0. */
    UA_ChannelSecurityToken securityToken;    /* Also contains the channelId */
    UA_ChannelSecurityToken altSecurityToken; /* Alternative token for the rollover.
                                               * See the renewState. */

    /* The endpoint and context of the channel */
    const UA_SecurityPolicy *securityPolicy;
    void *channelContext; /* For interaction with the security policy */

    /* Asymmetric encryption info */
    UA_ByteString remoteCertificate;
    UA_Byte remoteCertificateThumbprint[20]; /* The thumbprint of the remote certificate */

    /* Symmetric encryption nonces. These are used to generate the key material
     * and must not be reused once the keys are in place.
     *
     * Nonces are also used during the CreateSession / ActivateSession
     * handshake. These are not handled here, as the Session handling can
     * overlap with a RenewSecureChannel. */
    UA_ByteString remoteNonce;
    UA_ByteString localNonce;

    UA_UInt32 receiveSequenceNumber;
    UA_UInt32 sendSequenceNumber;

    /* Sessions that are bound to the SecureChannel (singly-linked list, only
     * used in the server) */
    UA_Session *sessions;

    /* If a buffer is received, first all chunks are put into the completeChunks
     * queue. Then they are processed in order. This ensures that processing
     * buffers is reentrant with the correct processing order. (This has lead to
     * problems in the client in the past.) */
    UA_ChunkQueue completeChunks; /* Received full chunks that have not been
                                   * decrypted so far */
    UA_ChunkQueue decryptedChunks; /* Received chunks that were decrypted but
                                    * not processed */
    size_t decryptedChunksCount;
    size_t decryptedChunksLength;
    UA_ByteString incompleteChunk; /* A half-received chunk (TCP is a
                                    * streaming protocol) is stored here */

    UA_CertificateGroup *certificateVerification;
    UA_StatusCode (*processOPNHeader)(void *application, UA_SecureChannel *channel,
                                      const UA_AsymmetricAlgorithmSecurityHeader *asymHeader);
};

void UA_SecureChannel_init(UA_SecureChannel *channel);

/* Trigger the shutdown */
void UA_SecureChannel_shutdown(UA_SecureChannel *channel,
                               UA_ShutdownReason shutdownReason);

/* Eventual cleanup after the channel has closed. It is possible to call _init
 * on the channel afterwards to reset it to the fresh status. */
void UA_SecureChannel_clear(UA_SecureChannel *channel);

/* Process the remote configuration in the HEL/ACK handshake. The connection
 * config is initialized with the local settings. */
UA_StatusCode
UA_SecureChannel_processHELACK(UA_SecureChannel *channel,
                               const UA_TcpAcknowledgeMessage *remoteConfig);

UA_StatusCode
UA_SecureChannel_setSecurityPolicy(UA_SecureChannel *channel,
                                   const UA_SecurityPolicy *securityPolicy,
                                   const UA_ByteString *remoteCertificate);

UA_Boolean
UA_SecureChannel_isConnected(UA_SecureChannel *channel);

/* Returns true if the channel has timed out. Performs the SecurityToken
 * rollover if required and possible. */
UA_Boolean
UA_SecureChannel_checkTimeout(UA_SecureChannel *channel,
                              UA_DateTime nowMonotonic);

/* When a fatal error occurs the Server shall send an Error Message to the
 * Client and close the socket. When a Client encounters one of these errors, it
 * shall also close the socket but does not send an Error Message. After the
 * socket is closed a Client shall try to reconnect automatically using the
 * mechanisms described in [...]. */
void
UA_SecureChannel_sendError(UA_SecureChannel *channel, UA_TcpErrorMessage *error);

/* Remove (partially) received unprocessed chunks */
void
UA_SecureChannel_deleteBuffered(UA_SecureChannel *channel);

/* Wrapper function for generating a local nonce for the supplied channel. Uses
 * the random generator of the channels security policy to allocate and generate
 * a nonce with the specified length. */
UA_StatusCode
UA_SecureChannel_generateLocalNonce(UA_SecureChannel *channel);

UA_StatusCode
UA_SecureChannel_generateLocalKeys(const UA_SecureChannel *channel);

UA_StatusCode
generateRemoteKeys(const UA_SecureChannel *channel);

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

typedef UA_StatusCode
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
                               const UA_ByteString *buffer,
                               UA_DateTime nowMonotonic);

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
checkSymHeader(UA_SecureChannel *channel, const UA_UInt32 tokenId,
               UA_DateTime nowMonotonic);

UA_StatusCode
checkAsymHeader(UA_SecureChannel *channel,
                const UA_AsymmetricAlgorithmSecurityHeader *asymHeader);

void
padChunk(UA_SecureChannel *channel, const UA_SecurityPolicyCryptoModule *cm,
         const UA_Byte *start, UA_Byte **pos);

UA_StatusCode
signAndEncryptAsym(UA_SecureChannel *channel, size_t preSignLength,
                   UA_ByteString *buf, size_t securityHeaderLength,
                   size_t totalLength);

UA_StatusCode
signAndEncryptSym(UA_MessageContext *messageContext,
                  size_t preSigLength, size_t totalLength);

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

#define UA_LOG_CHANNEL_INTERNAL(LOGGER, LEVEL, CHANNEL, MSG, ...)       \
    if(UA_LOGLEVEL <= UA_LOGLEVEL_##LEVEL) {                            \
        UA_LOG_##LEVEL(LOGGER, UA_LOGCATEGORY_SECURECHANNEL,            \
                       "TCP %lu\t| SC %" PRIu32 "\t| " MSG "%.0s", \
                       (long unsigned)(CHANNEL)->connectionId,          \
                       (CHANNEL)->securityToken.channelId, __VA_ARGS__); \
    }

#define UA_LOG_TRACE_CHANNEL(LOGGER, CHANNEL, ...)                      \
    UA_MACRO_EXPAND(UA_LOG_CHANNEL_INTERNAL(LOGGER, TRACE, CHANNEL, __VA_ARGS__, ""))
#define UA_LOG_DEBUG_CHANNEL(LOGGER, CHANNEL, ...)                      \
    UA_MACRO_EXPAND(UA_LOG_CHANNEL_INTERNAL(LOGGER, DEBUG, CHANNEL, __VA_ARGS__, ""))
#define UA_LOG_INFO_CHANNEL(LOGGER, CHANNEL, ...)                       \
    UA_MACRO_EXPAND(UA_LOG_CHANNEL_INTERNAL(LOGGER, INFO, CHANNEL, __VA_ARGS__, ""))
#define UA_LOG_WARNING_CHANNEL(LOGGER, CHANNEL, ...)                    \
    UA_MACRO_EXPAND(UA_LOG_CHANNEL_INTERNAL(LOGGER, WARNING, CHANNEL, __VA_ARGS__, ""))
#define UA_LOG_ERROR_CHANNEL(LOGGER, CHANNEL, ...)                      \
    UA_MACRO_EXPAND(UA_LOG_CHANNEL_INTERNAL(LOGGER, ERROR, CHANNEL, __VA_ARGS__, ""))
#define UA_LOG_FATAL_CHANNEL(LOGGER, CHANNEL, ...)                      \
    UA_MACRO_EXPAND(UA_LOG_CHANNEL_INTERNAL(LOGGER, FATAL, CHANNEL, __VA_ARGS__, ""))

_UA_END_DECLS

#endif /* UA_SECURECHANNEL_H_ */
