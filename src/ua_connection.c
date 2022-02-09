/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014, 2016-2017 (c) Florian Palm
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2015 (c) Oleksiy Vasylyev
 *    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2019 (c) Kalycito Infotech Private Limited
 */

#include <open62541/types_generated_handling.h>

#include "ua_connection_internal.h"
#include "ua_securechannel.h"
#include "ua_types_encoding_binary.h"
#include "ua_util_internal.h"

#include <open62541/plugin/eventloop.h>

/* Hides some errors before sending them to a client according to the
 * standard. */
static void
hideErrors(UA_TcpErrorMessage *const error) {
    switch(error->error) {
    case UA_STATUSCODE_BADCERTIFICATEUNTRUSTED:
    case UA_STATUSCODE_BADCERTIFICATEREVOKED:
        error->error = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        error->reason = UA_STRING_NULL;
        break;
        // TODO: Check if these are all cases that need to be covered.
    default:
        break;
    }
}

void
UA_Connection_sendError(UA_Connection *connection, UA_TcpErrorMessage *error) {
    hideErrors(error);

    UA_TcpMessageHeader header;
    header.messageTypeAndChunkType = UA_MESSAGETYPE_ERR + UA_CHUNKTYPE_FINAL;
    // Header + ErrorMessage (error + reasonLength_field + length)
    header.messageSize = 8 + (4 + 4 + (UA_UInt32)error->reason.length);

    /* Get the send buffer from the network layer */
    UA_ByteString msg = UA_BYTESTRING_NULL;
    UA_StatusCode retval = connection->getSendBuffer(connection, header.messageSize, &msg);
    if(retval != UA_STATUSCODE_GOOD)
        return;

    /* Encode and send the response */
    UA_Byte *bufPos = msg.data;
    const UA_Byte *bufEnd = &msg.data[msg.length];
    retval |= UA_encodeBinaryInternal(&header,
                                      &UA_TRANSPORT[UA_TRANSPORT_TCPMESSAGEHEADER],
                                      &bufPos, &bufEnd, NULL, NULL);
    retval |= UA_encodeBinaryInternal(error,
                                      &UA_TRANSPORT[UA_TRANSPORT_TCPERRORMESSAGE],
                                      &bufPos, &bufEnd, NULL, NULL);
    (void)retval; /* Encoding of these cannot fail */
    msg.length = header.messageSize;
    connection->send(connection, &msg);
}

void UA_Connection_detachSecureChannel(UA_Connection *connection) {
    UA_SecureChannel *channel = connection->channel;
    if(channel)
        /* only replace when the channel points to this connection */
        UA_atomic_cmpxchg((void**)&channel->connection, connection, NULL);
    UA_atomic_xchg((void**)&connection->channel, NULL);
}

// TODO: Return an error code
void
UA_Connection_attachSecureChannel(UA_Connection *connection, UA_SecureChannel *channel) {
    if(UA_atomic_cmpxchg((void**)&channel->connection, NULL, connection) == NULL)
        UA_atomic_xchg((void**)&connection->channel, (void*)channel);
}

UA_StatusCode UA_Server_Connection_getSendBuffer(UA_Connection *connection, size_t length,
                                                        UA_ByteString *buf) {
    UA_Server_ConnectionContext *ctx = (UA_Server_ConnectionContext *) connection->handle;
    UA_ConnectionManager *cm = ((UA_Server_BasicConnectionContext *)ctx)->cm;
    return cm->allocNetworkBuffer(cm, ctx->connectionId, buf, length);
}

UA_StatusCode UA_Server_Connection_send(UA_Connection *connection, UA_ByteString *buf) {
    UA_Server_ConnectionContext *ctx = (UA_Server_ConnectionContext *) connection->handle;
    UA_ConnectionManager *cm = ((UA_Server_BasicConnectionContext *)ctx)->cm;

    return cm->sendWithConnection(cm, ctx->connectionId, 0, NULL, buf);
}

void UA_Server_Connection_releaseBuffer (UA_Connection *connection, UA_ByteString *buf) {
    UA_Server_ConnectionContext *ctx = (UA_Server_ConnectionContext *) connection->handle;
    UA_ConnectionManager *cm = ((UA_Server_BasicConnectionContext *)ctx)->cm;
    cm->freeNetworkBuffer(cm, ctx->connectionId, buf);
}

void UA_Server_Connection_close(UA_Connection *connection) {
    UA_Server_ConnectionContext *ctx = (UA_Server_ConnectionContext *) connection->handle;
    UA_ConnectionManager *cm = ((UA_Server_BasicConnectionContext *)ctx)->cm;
    cm->closeConnection(cm, ctx->connectionId);
}

void
UA_Server_connectionCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                             void **connectionContext, UA_StatusCode stat,
                             size_t paramsSize, const UA_KeyValuePair *params,
                             UA_ByteString msg) {

    UA_LOG_DEBUG(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_SERVER,
                 "connection callback for id: %lu", (unsigned long) connectionId);

    UA_Server_BasicConnectionContext *ctx = (UA_Server_BasicConnectionContext *) *connectionContext;

    if (stat != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_SERVER, "closing connection");

        if (!ctx->isInitial) {

            UA_Server_ConnectionContext *serverctx = (UA_Server_ConnectionContext *) ctx;
            /* Instead of removing the complete connection just detach the secure channel */
            /* TODO: check if UA_Server_removeConnection is public api */
            /* UA_Server_removeConnection(ctx->server, &serverctx->connection); */
            UA_Connection_detachSecureChannel(&serverctx->connection);
            free(*connectionContext);
        }
        return;
    }

    if (ctx->isInitial) {
        UA_Server_ConnectionContext *newCtx = (UA_Server_ConnectionContext*) calloc(1, sizeof(UA_Server_ConnectionContext));
        newCtx->base.isInitial = false;
        newCtx->base.cm = ctx->cm;
        newCtx->base.server = ctx->server;
        newCtx->connectionId = connectionId;
        newCtx->connection.close = UA_Server_Connection_close;
        newCtx->connection.free = NULL;
        newCtx->connection.getSendBuffer = UA_Server_Connection_getSendBuffer;
        newCtx->connection.recv = NULL;
        newCtx->connection.releaseRecvBuffer = UA_Server_Connection_releaseBuffer;
        newCtx->connection.releaseSendBuffer = UA_Server_Connection_releaseBuffer;
        newCtx->connection.send = UA_Server_Connection_send;
        newCtx->connection.state = UA_CONNECTIONSTATE_CLOSED;

        newCtx->connection.handle = newCtx;

        *connectionContext = newCtx;
    }

    UA_Server_ConnectionContext *conCtx = (UA_Server_ConnectionContext *) *connectionContext;

    if (msg.length > 0) {
        UA_Server_processBinaryMessage(ctx->server, &conCtx->connection, &msg);
    }
}
