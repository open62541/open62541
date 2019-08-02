/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Mark Giraud, Fraunhofer IOSB
 */

#include <open62541/types_generated_encoding_binary.h>
#include <open62541/transport_generated_encoding_binary.h>
#include "ua_client_internal.h"

static UA_StatusCode
UA_Client_processTcpErrorMessage(UA_Client *client, UA_ByteString *message,
                                 size_t *offset) {
    UA_TcpErrorMessage tcpErrorMessage;
    UA_TcpErrorMessage_decodeBinary(message, offset, &tcpErrorMessage);

    UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_NETWORK,
                 "Received ERR response. %s - %.*s",
                 UA_StatusCode_name(tcpErrorMessage.error),
                 (int)tcpErrorMessage.reason.length,
                 tcpErrorMessage.reason.data);
    return tcpErrorMessage.error;
}

static UA_StatusCode
client_processCompleteChunkWithoutChannel(UA_Client *client, UA_Connection *connection,
                                          UA_ByteString *message) {
    UA_Socket *const sock = UA_Connection_getSocket(connection);
    if(sock == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_LOG_TRACE(&client->config.logger, UA_LOGCATEGORY_NETWORK,
                 "Socket %i | No channel attached to the connection. "
                 "Process the chunk directly", (int)(sock->id));
    size_t offset = 0;
    UA_TcpMessageHeader tcpMessageHeader;
    UA_StatusCode retval = UA_TcpMessageHeader_decodeBinary(message, &offset, &tcpMessageHeader);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_MessageType messageType = (UA_MessageType)
        (tcpMessageHeader.messageTypeAndChunkType & UA_BITMASK_MESSAGETYPE);
    UA_ChunkType chunkType = (UA_ChunkType)
        (tcpMessageHeader.messageTypeAndChunkType & UA_BITMASK_CHUNKTYPE);
    if(chunkType != UA_CHUNKTYPE_FINAL) {
        // Chunks can only be final if we don't have a secure channel.
        return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
    }

    // Only HEL and OPN messages possible without a channel (on the client side)
    switch(messageType) {
    case UA_MESSAGETYPE_ACK: {
        retval = processACKResponse(client, connection, message, &offset);
        break;
    }
    case UA_MESSAGETYPE_ERR: {
        retval = UA_Client_processTcpErrorMessage(client, message, &offset);
        break;
    }
    default:
        UA_LOG_TRACE(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "Socket %i | Expected ACK or ERR message on a connection "
                     "without a SecureChannel", (int)(sock->id));
        retval = UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
        break;
    }
    return retval;
}

/* For synchronous service calls. Execute async responses with a callback. When
 * the response with the correct requestId turns up, return it via the
 * SyncResponseDescription pointer. */
typedef struct {
    UA_Client *client;
    UA_Boolean received;
    UA_UInt32 requestId;
    void *response;
    const UA_DataType *responseType;
} SyncResponseDescription;

static const UA_NodeId
    serviceFaultId = {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_SERVICEFAULT_ENCODING_DEFAULTBINARY}};

/* Look for the async callback in the linked list, execute and delete it */
static UA_StatusCode
processAsyncResponse(UA_Client *client, UA_UInt32 requestId, const UA_NodeId *responseTypeId,
                     const UA_ByteString *responseMessage, size_t *offset) {
    /* Find the callback */
    AsyncServiceCall *ac;
    LIST_FOREACH(ac, &client->asyncServiceCalls, pointers) {
        if(ac->requestId == requestId)
            break;
    }
    if(!ac)
        return UA_STATUSCODE_BADREQUESTHEADERINVALID;

    /* Allocate the response */
    UA_STACKARRAY(UA_Byte, responseBuf, ac->responseType->memSize);
    void *response = (void *)(uintptr_t)&responseBuf[0]; /* workaround aliasing rules */

    /* Verify the type of the response */
    const UA_DataType *responseType = ac->responseType;
    const UA_NodeId expectedNodeId = UA_NODEID_NUMERIC(0, ac->responseType->binaryEncodingId);
    UA_StatusCode retval;
    if(!UA_NodeId_equal(responseTypeId, &expectedNodeId)) {
        UA_init(response, ac->responseType);
        if(UA_NodeId_equal(responseTypeId, &serviceFaultId)) {
            /* Decode as a ServiceFault, i.e. only the response header */
            UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                        "Received a ServiceFault response");
            responseType = &UA_TYPES[UA_TYPES_SERVICEFAULT];
        } else {
            /* Close the connection */
            UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                         "Reply contains the wrong service response");
            retval = UA_STATUSCODE_BADCOMMUNICATIONERROR;
            goto process;
        }
    }

    /* Decode the response */
    retval = UA_decodeBinary(responseMessage, offset, response, responseType, NULL);

process:
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "Could not decode the response with id %u due to %s",
                    requestId, UA_StatusCode_name(retval));
        ((UA_ResponseHeader *)response)->serviceResult = retval;
    } else if(((UA_ResponseHeader *)response)->serviceResult != UA_STATUSCODE_GOOD) {
        /* Decode as a ServiceFault, i.e. only the response header */
        UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "The ServiceResult has the StatusCode %s",
                    UA_StatusCode_name(((UA_ResponseHeader *)response)->serviceResult));
    }

    /* Call the callback */
    if(ac->callback)
        ac->callback(client, ac->userdata, requestId, response);
    UA_deleteMembers(response, ac->responseType);

    /* Remove the callback */
    LIST_REMOVE(ac, pointers);
    UA_free(ac);
    return retval;
}

/* Processes the received service response. Either with an async callback or by
 * decoding the message and returning it "upwards" in the
 * SyncResponseDescription. */
static void
processServiceResponse(void *application, UA_SecureChannel *channel,
                       UA_MessageType messageType, UA_UInt32 requestId,
                       const UA_ByteString *message) {
    SyncResponseDescription *rd = (SyncResponseDescription *)application;

    /* Must be OPN or MSG */
    if(messageType != UA_MESSAGETYPE_OPN &&
       messageType != UA_MESSAGETYPE_MSG) {
        UA_LOG_TRACE_CHANNEL(&rd->client->config.logger, channel,
                             "Invalid message type");
        return;
    }

    /* Decode the data type identifier of the response */
    size_t offset = 0;
    UA_NodeId responseId;
    UA_StatusCode retval = UA_NodeId_decodeBinary(message, &offset, &responseId);
    /* warning C4533: initialization of 'expectedNodeId' is skipped by 'goto finish'
     * (because of windows (c++ compiler))*/
    UA_NodeId expectedNodeId = UA_NODEID_NULL;
    if(retval != UA_STATUSCODE_GOOD)
        goto finish;

    /* Got an asynchronous response. Don't expected a synchronous response
     * (responseType NULL) or the id does not match. */
    if(!rd->responseType || requestId != rd->requestId) {
        retval = processAsyncResponse(rd->client, requestId, &responseId, message, &offset);
        goto finish;
    }

    /* Got the synchronous response */
    rd->received = true;

    /* Check that the response type matches */
    expectedNodeId = UA_NODEID_NUMERIC(0, rd->responseType->binaryEncodingId);
    if(!UA_NodeId_equal(&responseId, &expectedNodeId)) {
        if(UA_NodeId_equal(&responseId, &serviceFaultId)) {
            UA_LOG_INFO(&rd->client->config.logger, UA_LOGCATEGORY_CLIENT,
                        "Received a ServiceFault response");
            UA_init(rd->response, rd->responseType);
            retval = UA_decodeBinary(message, &offset, rd->response,
                                     &UA_TYPES[UA_TYPES_SERVICEFAULT], NULL);
            if(retval != UA_STATUSCODE_GOOD)
                ((UA_ResponseHeader *)rd->response)->serviceResult = retval;
            UA_LOG_INFO(&rd->client->config.logger, UA_LOGCATEGORY_CLIENT,
                        "Received a ServiceFault response with StatusCode %s",
                        UA_StatusCode_name(((UA_ResponseHeader *)rd->response)->serviceResult));
        } else {
            /* Close the connection */
            UA_LOG_ERROR(&rd->client->config.logger, UA_LOGCATEGORY_CLIENT,
                         "Reply contains the wrong service response");
            retval = UA_STATUSCODE_BADCOMMUNICATIONERROR;
        }
        goto finish;
    }

#ifdef UA_ENABLE_TYPEDESCRIPTION
    UA_LOG_DEBUG(&rd->client->config.logger, UA_LOGCATEGORY_CLIENT,
                 "Decode a message of type %s", rd->responseType->typeName);
#else
    UA_LOG_DEBUG(&rd->client->config.logger, UA_LOGCATEGORY_CLIENT,
                 "Decode a message of type %u", responseId.identifier.numeric);
#endif

    /* Decode the response */
    retval = UA_decodeBinary(message, &offset, rd->response, rd->responseType,
                             rd->client->config.customDataTypes);

finish:
    UA_NodeId_deleteMembers(&responseId);
    if(retval != UA_STATUSCODE_GOOD) {
        if(retval == UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED)
            retval = UA_STATUSCODE_BADRESPONSETOOLARGE;
        UA_LOG_INFO(&rd->client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "Error receiving the response with status code %s",
                    UA_StatusCode_name(retval));

        if(rd->response) {
            UA_ResponseHeader *respHeader = (UA_ResponseHeader *)rd->response;
            respHeader->serviceResult = retval;
        }
    }
}

/* Receive and process messages until a synchronous message arrives or the
 * timout finishes */
UA_StatusCode
receiveServiceResponse(UA_Client *client, void *response,
                       const UA_DataType *responseType, UA_DateTime maxDate,
                       const UA_UInt32 *synchronousRequestId) {
    /* Prepare the response and the structure we give into processServiceResponse */
    SyncResponseDescription rd = {client, false, 0, response, responseType};

    /* Return upon receiving the synchronized response. All other responses are
     * processed with a callback "in the background". */
    if(synchronousRequestId)
        rd.requestId = *synchronousRequestId;

    UA_StatusCode retval;
    do {
        UA_DateTime now = UA_DateTime_nowMonotonic();

        /* >= avoid timeout to be set to 0 */
        if(now >= maxDate)
            return UA_STATUSCODE_GOODNONCRITICALTIMEOUT;

        /* round always to upper value to avoid timeout to be set to 0
         * if(maxDate - now) < (UA_DATETIME_MSEC/2) */
        UA_UInt32 timeout = (UA_UInt32)(((maxDate - now) + (UA_DATETIME_MSEC - 1)) / UA_DATETIME_MSEC);

        if(rd.requestId != 0) {
            do {
                if(client->config.networkManager == NULL) {
                    UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                                 "No NetworkManager configured");
                    return UA_STATUSCODE_BADCONFIGURATIONERROR;
                }
                retval = client->config.networkManager->processSocket(client->config.networkManager,
                                                                      timeout,
                                                                      UA_Connection_getSocket(client->connection));
                if(retval != UA_STATUSCODE_GOOD) {
                    UA_Client_disconnect(client);
                    return retval;
                }
            } while(!UA_SecureChannel_isMessageComplete(&client->channel, rd.requestId));

            retval = UA_SecureChannel_processMessage(&client->channel, &rd,
                                                     processServiceResponse, rd.requestId);
        } else {
            retval = UA_SecureChannel_processCompleteMessages(&client->channel, &rd,
                                                              processServiceResponse);
            rd.received = true;
        }

        if(retval != UA_STATUSCODE_GOOD && retval != UA_STATUSCODE_GOODNONCRITICALTIMEOUT) {
            if(retval == UA_STATUSCODE_BADCONNECTIONCLOSED)
                setClientState(client, UA_CLIENTSTATE_DISCONNECTED);
            UA_Client_disconnect(client);
            break;
        }
    } while(!rd.received);
    return retval;
}

UA_StatusCode
receiveServiceResponseAsync(UA_Client *client, void *response,
                            const UA_DataType *responseType) {
    SyncResponseDescription rd = {client, false, 0, response, responseType};

    UA_StatusCode retval = UA_SecureChannel_processCompleteMessages(&client->channel, &rd, processServiceResponse);
    /*let client run when non critical timeout*/
    if(retval != UA_STATUSCODE_GOOD
       && retval != UA_STATUSCODE_GOODNONCRITICALTIMEOUT) {
        if(retval == UA_STATUSCODE_BADCONNECTIONCLOSED) {
            setClientState(client, UA_CLIENTSTATE_DISCONNECTED);
        }
        UA_Client_disconnect(client);
    }
    return retval;
}

UA_StatusCode
UA_Client_processChunk(UA_Client *client, UA_Connection *connection, UA_ByteString *chunk) {

    UA_StatusCode retval;
    if(!connection->channel)
        return client_processCompleteChunkWithoutChannel(client, connection, chunk);
    else
        retval = UA_SecureChannel_decryptAddChunk(connection->channel, chunk, false);

    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    return UA_SecureChannel_persistIncompleteMessages(connection->channel);
}
