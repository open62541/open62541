/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2022 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014-2016 (c) Sten Grüner
 *    Copyright 2014-2015, 2017 (c) Florian Palm
 *    Copyright 2015-2016 (c) Chris Iatrou
 *    Copyright 2015-2016 (c) Oleksiy Vasylyev
 *    Copyright 2016 (c) Joakim L. Gilje
 *    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2016 (c) TorbenD
 *    Copyright 2017 (c) frax2222
 *    Copyright 2017 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2019 (c) Kalycito Infotech Private Limited
 */

#include <open62541/transport_generated.h>
#include <open62541/transport_generated_handling.h>
#include <open62541/types_generated_handling.h>
#include "open62541/plugin/network.h"

#include "ua_server_internal.h"
#include "ua_types_encoding_binary.h"
#include "ua_services.h"

#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
// store the authentication token and session ID so we can help fuzzing by setting
// these values in the next request automatically
UA_NodeId unsafe_fuzz_authenticationToken = {0, UA_NODEIDTYPE_NUMERIC, {0}};
#endif

#ifdef UA_DEBUG_DUMP_PKGS_FILE
void UA_debug_dumpCompleteChunk(UA_Server *const server, UA_Connection *const connection,
                                UA_ByteString *messageBuffer);
#endif

/********************/
/* Helper Functions */
/********************/

UA_StatusCode
sendServiceFault(UA_SecureChannel *channel, UA_UInt32 requestId,
                 UA_UInt32 requestHandle, UA_StatusCode statusCode) {
    UA_ServiceFault response;
    UA_ServiceFault_init(&response);
    UA_ResponseHeader *responseHeader = &response.responseHeader;
    responseHeader->requestHandle = requestHandle;
    responseHeader->timestamp = UA_DateTime_now();
    responseHeader->serviceResult = statusCode;

    UA_LOG_DEBUG(channel->securityPolicy->logger, UA_LOGCATEGORY_SERVER,
                 "Sending response for RequestId %u with ServiceResult %s",
                 (unsigned)requestId, UA_StatusCode_name(statusCode));

    /* Send error message. Message type is MSG and not ERR, since we are on a
     * SecureChannel! */
    return UA_SecureChannel_sendSymmetricMessage(channel, requestId,
                                                 UA_MESSAGETYPE_MSG, &response,
                                                 &UA_TYPES[UA_TYPES_SERVICEFAULT]);
}

/* This is not an ERR message, the connection is not closed afterwards */
static UA_StatusCode
decodeHeaderSendServiceFault(UA_SecureChannel *channel, const UA_ByteString *msg,
                             size_t offset, const UA_DataType *responseType,
                             UA_UInt32 requestId, UA_StatusCode error) {
    UA_RequestHeader requestHeader;
    UA_StatusCode retval =
        UA_decodeBinaryInternal(msg, &offset, &requestHeader,
                                &UA_TYPES[UA_TYPES_REQUESTHEADER], NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    retval = sendServiceFault(channel,  requestId, requestHeader.requestHandle, error);
    UA_RequestHeader_clear(&requestHeader);
    return retval;
}

/* The counterOffset is the offset of the UA_ServiceCounterDataType for the
 * service in the UA_ SessionDiagnosticsDataType. */
#ifdef UA_ENABLE_DIAGNOSTICS
#define UA_SERVICECOUNTER_OFFSET(X)                             \
    *counterOffset = offsetof(UA_SessionDiagnosticsDataType, X)
#else
#define UA_SERVICECOUNTER_OFFSET(X)
#endif

static void
getServicePointers(UA_UInt32 requestTypeId, const UA_DataType **requestType,
                   const UA_DataType **responseType, UA_Service *service,
                   UA_Boolean *requiresSession, size_t *counterOffset) {
    switch(requestTypeId) {
    case UA_NS0ID_GETENDPOINTSREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_GetEndpoints;
        *requestType = &UA_TYPES[UA_TYPES_GETENDPOINTSREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_GETENDPOINTSRESPONSE];
        *requiresSession = false;
        break;
    case UA_NS0ID_FINDSERVERSREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_FindServers;
        *requestType = &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_FINDSERVERSRESPONSE];
        *requiresSession = false;
        break;
#ifdef UA_ENABLE_DISCOVERY
# ifdef UA_ENABLE_DISCOVERY_MULTICAST
    case UA_NS0ID_FINDSERVERSONNETWORKREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_FindServersOnNetwork;
        *requestType = &UA_TYPES[UA_TYPES_FINDSERVERSONNETWORKREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_FINDSERVERSONNETWORKRESPONSE];
        *requiresSession = false;
        break;
# endif
    case UA_NS0ID_REGISTERSERVERREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_RegisterServer;
        *requestType = &UA_TYPES[UA_TYPES_REGISTERSERVERREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_REGISTERSERVERRESPONSE];
        *requiresSession = false;
        break;
    case UA_NS0ID_REGISTERSERVER2REQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_RegisterServer2;
        *requestType = &UA_TYPES[UA_TYPES_REGISTERSERVER2REQUEST];
        *responseType = &UA_TYPES[UA_TYPES_REGISTERSERVER2RESPONSE];
        *requiresSession = false;
        break;
#endif
    case UA_NS0ID_CREATESESSIONREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_CreateSession;
        *requestType = &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE];
        *requiresSession = false;
        break;
    case UA_NS0ID_ACTIVATESESSIONREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_ActivateSession;
        *requestType = &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE];
        break;
    case UA_NS0ID_CLOSESESSIONREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_CloseSession;
        *requestType = &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_CLOSESESSIONRESPONSE];
        break;
    case UA_NS0ID_READREQUEST_ENCODING_DEFAULTBINARY:
        *service = NULL;
        *service = (UA_Service)Service_Read;
        *requestType = &UA_TYPES[UA_TYPES_READREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_READRESPONSE];
        UA_SERVICECOUNTER_OFFSET(readCount);
        break;
    case UA_NS0ID_WRITEREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_Write;
        *requestType = &UA_TYPES[UA_TYPES_WRITEREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_WRITERESPONSE];
        UA_SERVICECOUNTER_OFFSET(writeCount);
        break;
    case UA_NS0ID_BROWSEREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_Browse;
        *requestType = &UA_TYPES[UA_TYPES_BROWSEREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_BROWSERESPONSE];
        UA_SERVICECOUNTER_OFFSET(browseCount);
        break;
    case UA_NS0ID_BROWSENEXTREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_BrowseNext;
        *requestType = &UA_TYPES[UA_TYPES_BROWSENEXTREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_BROWSENEXTRESPONSE];
        UA_SERVICECOUNTER_OFFSET(browseNextCount);
        break;
    case UA_NS0ID_REGISTERNODESREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_RegisterNodes;
        *requestType = &UA_TYPES[UA_TYPES_REGISTERNODESREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_REGISTERNODESRESPONSE];
        UA_SERVICECOUNTER_OFFSET(registerNodesCount);
        break;
    case UA_NS0ID_UNREGISTERNODESREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_UnregisterNodes;
        *requestType = &UA_TYPES[UA_TYPES_UNREGISTERNODESREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_UNREGISTERNODESRESPONSE];
        UA_SERVICECOUNTER_OFFSET(unregisterNodesCount);
        break;
    case UA_NS0ID_TRANSLATEBROWSEPATHSTONODEIDSREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_TranslateBrowsePathsToNodeIds;
        *requestType = &UA_TYPES[UA_TYPES_TRANSLATEBROWSEPATHSTONODEIDSREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_TRANSLATEBROWSEPATHSTONODEIDSRESPONSE];
        UA_SERVICECOUNTER_OFFSET(translateBrowsePathsToNodeIdsCount);
        break;

#ifdef UA_ENABLE_SUBSCRIPTIONS
    case UA_NS0ID_CREATESUBSCRIPTIONREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_CreateSubscription;
        *requestType = &UA_TYPES[UA_TYPES_CREATESUBSCRIPTIONREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_CREATESUBSCRIPTIONRESPONSE];
        UA_SERVICECOUNTER_OFFSET(createSubscriptionCount);
        break;
    case UA_NS0ID_PUBLISHREQUEST_ENCODING_DEFAULTBINARY:
        *requestType = &UA_TYPES[UA_TYPES_PUBLISHREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_PUBLISHRESPONSE];
        UA_SERVICECOUNTER_OFFSET(publishCount);
        break;
    case UA_NS0ID_REPUBLISHREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_Republish;
        *requestType = &UA_TYPES[UA_TYPES_REPUBLISHREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_REPUBLISHRESPONSE];
        UA_SERVICECOUNTER_OFFSET(republishCount);
        break;
    case UA_NS0ID_MODIFYSUBSCRIPTIONREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_ModifySubscription;
        *requestType = &UA_TYPES[UA_TYPES_MODIFYSUBSCRIPTIONREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_MODIFYSUBSCRIPTIONRESPONSE];
        UA_SERVICECOUNTER_OFFSET(modifySubscriptionCount);
        break;
    case UA_NS0ID_SETPUBLISHINGMODEREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_SetPublishingMode;
        *requestType = &UA_TYPES[UA_TYPES_SETPUBLISHINGMODEREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_SETPUBLISHINGMODERESPONSE];
        UA_SERVICECOUNTER_OFFSET(setPublishingModeCount);
        break;
    case UA_NS0ID_DELETESUBSCRIPTIONSREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_DeleteSubscriptions;
        *requestType = &UA_TYPES[UA_TYPES_DELETESUBSCRIPTIONSREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_DELETESUBSCRIPTIONSRESPONSE];
        UA_SERVICECOUNTER_OFFSET(deleteSubscriptionsCount);
        break;
    case UA_NS0ID_TRANSFERSUBSCRIPTIONSREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_TransferSubscriptions;
        *requestType = &UA_TYPES[UA_TYPES_TRANSFERSUBSCRIPTIONSREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_TRANSFERSUBSCRIPTIONSRESPONSE];
        UA_SERVICECOUNTER_OFFSET(transferSubscriptionsCount);
        break;
    case UA_NS0ID_CREATEMONITOREDITEMSREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_CreateMonitoredItems;
        *requestType = &UA_TYPES[UA_TYPES_CREATEMONITOREDITEMSREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_CREATEMONITOREDITEMSRESPONSE];
        UA_SERVICECOUNTER_OFFSET(createMonitoredItemsCount);
        break;
    case UA_NS0ID_DELETEMONITOREDITEMSREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_DeleteMonitoredItems;
        *requestType = &UA_TYPES[UA_TYPES_DELETEMONITOREDITEMSREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_DELETEMONITOREDITEMSRESPONSE];
        UA_SERVICECOUNTER_OFFSET(deleteMonitoredItemsCount);
        break;
    case UA_NS0ID_MODIFYMONITOREDITEMSREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_ModifyMonitoredItems;
        *requestType = &UA_TYPES[UA_TYPES_MODIFYMONITOREDITEMSREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_MODIFYMONITOREDITEMSRESPONSE];
        UA_SERVICECOUNTER_OFFSET(modifyMonitoredItemsCount);
        break;
    case UA_NS0ID_SETMONITORINGMODEREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_SetMonitoringMode;
        *requestType = &UA_TYPES[UA_TYPES_SETMONITORINGMODEREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_SETMONITORINGMODERESPONSE];
        UA_SERVICECOUNTER_OFFSET(setMonitoringModeCount);
        break;
    case UA_NS0ID_SETTRIGGERINGREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_SetTriggering;
        *requestType = &UA_TYPES[UA_TYPES_SETTRIGGERINGREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_SETTRIGGERINGRESPONSE];
        UA_SERVICECOUNTER_OFFSET(setTriggeringCount);
        break;
#endif
#ifdef UA_ENABLE_HISTORIZING
        /* For History read */
    case UA_NS0ID_HISTORYREADREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_HistoryRead;
        *requestType = &UA_TYPES[UA_TYPES_HISTORYREADREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_HISTORYREADRESPONSE];
        UA_SERVICECOUNTER_OFFSET(historyReadCount);
        break;
        /* For History update */
    case UA_NS0ID_HISTORYUPDATEREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_HistoryUpdate;
        *requestType = &UA_TYPES[UA_TYPES_HISTORYUPDATEREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_HISTORYUPDATERESPONSE];
        UA_SERVICECOUNTER_OFFSET(historyUpdateCount);
        break;
#endif

#ifdef UA_ENABLE_METHODCALLS
    case UA_NS0ID_CALLREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_Call;
        *requestType = &UA_TYPES[UA_TYPES_CALLREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_CALLRESPONSE];
        UA_SERVICECOUNTER_OFFSET(callCount);
        break;
#endif

#ifdef UA_ENABLE_NODEMANAGEMENT
    case UA_NS0ID_ADDNODESREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_AddNodes;
        *requestType = &UA_TYPES[UA_TYPES_ADDNODESREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_ADDNODESRESPONSE];
        UA_SERVICECOUNTER_OFFSET(addNodesCount);
        break;
    case UA_NS0ID_ADDREFERENCESREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_AddReferences;
        *requestType = &UA_TYPES[UA_TYPES_ADDREFERENCESREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_ADDREFERENCESRESPONSE];
        UA_SERVICECOUNTER_OFFSET(addReferencesCount);
        break;
    case UA_NS0ID_DELETENODESREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_DeleteNodes;
        *requestType = &UA_TYPES[UA_TYPES_DELETENODESREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_DELETENODESRESPONSE];
        UA_SERVICECOUNTER_OFFSET(deleteNodesCount);
        break;
    case UA_NS0ID_DELETEREFERENCESREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_DeleteReferences;
        *requestType = &UA_TYPES[UA_TYPES_DELETEREFERENCESREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_DELETEREFERENCESRESPONSE];
        UA_SERVICECOUNTER_OFFSET(deleteReferencesCount);
        break;
#endif

    default:
        break;
    }
}

/*************************/
/* Process Message Types */
/*************************/

/* HEL -> Open up the connection */
static UA_StatusCode
processHEL(UA_Server *server, UA_SecureChannel *channel, const UA_ByteString *msg) {
    if(channel->state != UA_SECURECHANNELSTATE_FRESH)
        return UA_STATUSCODE_BADINTERNALERROR;
    size_t offset = 0; /* Go to the beginning of the TcpHelloMessage */
    UA_TcpHelloMessage helloMessage;
    UA_StatusCode retval =
        UA_decodeBinaryInternal(msg, &offset, &helloMessage,
                                &UA_TRANSPORT[UA_TRANSPORT_TCPHELLOMESSAGE], NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Currently not checked */
    UA_String_clear(&helloMessage.endpointUrl);

    /* Parameterize the connection. The TcpHelloMessage casts to a
     * TcpAcknowledgeMessage. */
    retval = UA_SecureChannel_processHELACK(channel,
                                            (UA_TcpAcknowledgeMessage*)&helloMessage);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_NETWORK,
                    "Connection %i | Error during the HEL/ACK handshake",
                    (int)(channel->connection->sockfd));
        return retval;
    }

    /* Get the send buffer from the network layer */
    UA_Connection *connection = channel->connection;
    UA_ByteString ack_msg;
    UA_ByteString_init(&ack_msg);
    retval = connection->getSendBuffer(connection, channel->config.sendBufferSize, &ack_msg);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Build acknowledge response */
    UA_TcpAcknowledgeMessage ackMessage;
    ackMessage.protocolVersion = 0;
    ackMessage.receiveBufferSize = channel->config.recvBufferSize;
    ackMessage.sendBufferSize = channel->config.sendBufferSize;
    ackMessage.maxMessageSize = channel->config.localMaxMessageSize;
    ackMessage.maxChunkCount = channel->config.localMaxChunkCount;

    UA_TcpMessageHeader ackHeader;
    ackHeader.messageTypeAndChunkType = UA_MESSAGETYPE_ACK + UA_CHUNKTYPE_FINAL;
    ackHeader.messageSize = 8 + 20; /* ackHeader + ackMessage */

    /* Encode and send the response */
    UA_Byte *bufPos = ack_msg.data;
    const UA_Byte *bufEnd = &ack_msg.data[ack_msg.length];
    retval |= UA_encodeBinaryInternal(&ackHeader,
                                      &UA_TRANSPORT[UA_TRANSPORT_TCPMESSAGEHEADER],
                                      &bufPos, &bufEnd, NULL, NULL);
    retval |= UA_encodeBinaryInternal(&ackMessage,
                                      &UA_TRANSPORT[UA_TRANSPORT_TCPACKNOWLEDGEMESSAGE],
                                      &bufPos, &bufEnd, NULL, NULL);
    if(retval != UA_STATUSCODE_GOOD) {
        connection->releaseSendBuffer(connection, &ack_msg);
        return retval;
    }

    ack_msg.length = ackHeader.messageSize;
    retval = connection->send(connection, &ack_msg);
    if(retval == UA_STATUSCODE_GOOD)
        channel->state = UA_SECURECHANNELSTATE_ACK_SENT;
    return retval;
}

/* OPN -> Open up/renew the securechannel */
static UA_StatusCode
processOPN(UA_Server *server, UA_SecureChannel *channel,
           const UA_UInt32 requestId, const UA_ByteString *msg) {
    if(channel->state != UA_SECURECHANNELSTATE_ACK_SENT &&
       channel->state != UA_SECURECHANNELSTATE_OPEN)
        return UA_STATUSCODE_BADINTERNALERROR;
    /* Decode the request */
    UA_NodeId requestType;
    UA_OpenSecureChannelRequest openSecureChannelRequest;
    size_t offset = 0;
    UA_StatusCode retval = UA_NodeId_decodeBinary(msg, &offset, &requestType);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_NodeId_clear(&requestType);
        UA_LOG_WARNING_CHANNEL(&server->config.logger, channel,
                               "Could not decode the NodeId. Closing the connection");
        UA_Server_closeSecureChannel(server, channel, UA_DIAGNOSTICEVENT_REJECT);
        return retval;
    }
    retval = UA_decodeBinaryInternal(msg, &offset, &openSecureChannelRequest,
                                     &UA_TYPES[UA_TYPES_OPENSECURECHANNELREQUEST], NULL);

    /* Error occurred */
    if(retval != UA_STATUSCODE_GOOD ||
       !UA_NodeId_equal(&requestType, &UA_TYPES[UA_TYPES_OPENSECURECHANNELREQUEST].binaryEncodingId)) {
        UA_NodeId_clear(&requestType);
        UA_OpenSecureChannelRequest_clear(&openSecureChannelRequest);
        UA_LOG_WARNING_CHANNEL(&server->config.logger, channel,
                               "Could not decode the OPN message. Closing the connection.");
        UA_Server_closeSecureChannel(server, channel, UA_DIAGNOSTICEVENT_REJECT);
        return retval;
    }
    UA_NodeId_clear(&requestType);

    /* Call the service */
    UA_OpenSecureChannelResponse openScResponse;
    UA_OpenSecureChannelResponse_init(&openScResponse);
    Service_OpenSecureChannel(server, channel, &openSecureChannelRequest, &openScResponse);
    UA_OpenSecureChannelRequest_clear(&openSecureChannelRequest);
    if(openScResponse.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_CHANNEL(&server->config.logger, channel, "Could not open a SecureChannel. "
                               "Closing the connection.");
        UA_Server_closeSecureChannel(server, channel, UA_DIAGNOSTICEVENT_REJECT);
        return openScResponse.responseHeader.serviceResult;
    }

    /* Send the response */
    retval = UA_SecureChannel_sendAsymmetricOPNMessage(channel, requestId, &openScResponse,
                                                       &UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE]);
    UA_OpenSecureChannelResponse_clear(&openScResponse);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_CHANNEL(&server->config.logger, channel,
                               "Could not send the OPN answer with error code %s",
                               UA_StatusCode_name(retval));
        UA_Server_closeSecureChannel(server, channel, UA_DIAGNOSTICEVENT_REJECT);
    }

    return retval;
}

/* The responseHeader must have the requestHandle already set */
UA_StatusCode
sendResponse(UA_Server *server, UA_Session *session, UA_SecureChannel *channel,
             UA_UInt32 requestId, UA_Response *response, const UA_DataType *responseType) {
    if(!channel)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* If the overall service call failed, answer with a ServiceFault */
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        return sendServiceFault(channel, requestId, response->responseHeader.requestHandle,
                                response->responseHeader.serviceResult);

    /* Prepare the ResponseHeader */
    response->responseHeader.timestamp = UA_DateTime_now();

    if(session) {
#ifdef UA_ENABLE_TYPEDESCRIPTION
        UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                             "Sending response for RequestId %u of type %s",
                             (unsigned)requestId, responseType->typeName);
#else
        UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                             "Sending reponse for RequestId %u of type %" PRIu32,
                             (unsigned)requestId, responseType->binaryEncodingId.identifier.numeric);
#endif
    } else {
#ifdef UA_ENABLE_TYPEDESCRIPTION
        UA_LOG_DEBUG_CHANNEL(&server->config.logger, channel,
                             "Sending response for RequestId %u of type %s",
                             (unsigned)requestId, responseType->typeName);
#else
        UA_LOG_DEBUG_CHANNEL(&server->config.logger, channel,
                             "Sending reponse for RequestId %u of type %" PRIu32,
                             (unsigned)requestId, responseType->binaryEncodingId.identifier.numeric);
#endif
    }

    /* Start the message context */
    UA_MessageContext mc;
    UA_StatusCode retval = UA_MessageContext_begin(&mc, channel, requestId, UA_MESSAGETYPE_MSG);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Assert's required for clang-analyzer */
    UA_assert(mc.buf_pos == &mc.messageBuffer.data[UA_SECURECHANNEL_SYMMETRIC_HEADER_TOTALLENGTH]);
    UA_assert(mc.buf_end <= &mc.messageBuffer.data[mc.messageBuffer.length]);

    /* Encode the response type */
    retval = UA_MessageContext_encode(&mc, &responseType->binaryEncodingId,
                                      &UA_TYPES[UA_TYPES_NODEID]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Encode the response */
    retval = UA_MessageContext_encode(&mc, response, responseType);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Finish / send out */
    return UA_MessageContext_finish(&mc);
}

/* A Session is "bound" to a SecureChannel if it was created by the
 * SecureChannel or if it was activated on it. A Session can only be bound to
 * one SecureChannel. A Session can only be closed from the SecureChannel to
 * which it is bound.
 *
 * Returns Good if the AuthenticationToken exists nowhere (for CTT). */
UA_StatusCode
getBoundSession(UA_Server *server, const UA_SecureChannel *channel,
                const UA_NodeId *token, UA_Session **session) {
    UA_DateTime now = UA_DateTime_nowMonotonic();
    UA_SessionHeader *sh;
    SLIST_FOREACH(sh, &channel->sessions, next) {
        if(!UA_NodeId_equal(token, &sh->authenticationToken))
            continue;
        UA_Session *current = (UA_Session*)sh;
        /* Has the session timed out? */
        if(current->validTill < now) {
            server->serverDiagnosticsSummary.rejectedSessionCount++;
            return UA_STATUSCODE_BADSESSIONCLOSED;
        }
        *session = current;
        return UA_STATUSCODE_GOOD;
    }

    server->serverDiagnosticsSummary.rejectedSessionCount++;

    /* Session exists on another SecureChannel. The CTT expect this error. */
    UA_Session *tmpSession = getSessionByToken(server, token);
    if(tmpSession) {
#ifdef UA_ENABLE_DIAGNOSTICS
        tmpSession->diagnostics.unauthorizedRequestCount++;
#endif
        return UA_STATUSCODE_BADSECURECHANNELIDINVALID;
    }

    return UA_STATUSCODE_GOOD;
}

static const UA_String securityPolicyNone =
    UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#None");

/* Returns a status of the SecureChannel. The detailed service status (usually
 * part of the response) is set in the serviceResult argument. */
static UA_StatusCode
processMSGDecoded(UA_Server *server, UA_SecureChannel *channel, UA_UInt32 requestId,
                  UA_Service service, const UA_Request *request,
                  const UA_DataType *requestType, UA_Response *response,
                  const UA_DataType *responseType, UA_Boolean sessionRequired,
                  size_t counterOffset) {
    UA_Session *session = NULL;
    UA_StatusCode channelRes = UA_STATUSCODE_GOOD;
    UA_StatusCode serviceRes = UA_STATUSCODE_GOOD;
    const UA_RequestHeader *requestHeader = &request->requestHeader;

    /* If it is an unencrypted (#None) channel, only allow the discovery services */
    if(server->config.securityPolicyNoneDiscoveryOnly &&
       UA_String_equal(&channel->securityPolicy->policyUri, &securityPolicyNone ) &&
       requestType != &UA_TYPES[UA_TYPES_GETENDPOINTSREQUEST] &&
       requestType != &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST]
#if defined(UA_ENABLE_DISCOVERY) && defined(UA_ENABLE_DISCOVERY_MULTICAST)
       && requestType != &UA_TYPES[UA_TYPES_FINDSERVERSONNETWORKREQUEST]
#endif
       ) {
        serviceRes = UA_STATUSCODE_BADSECURITYPOLICYREJECTED;
        channelRes = sendServiceFault(channel, requestId, requestHeader->requestHandle,
                                      UA_STATUSCODE_BADSECURITYPOLICYREJECTED);
        goto update_statistics;
    }

    /* Session lifecycle services. */
    if(requestType == &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST] ||
       requestType == &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST] ||
       requestType == &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST]) {
        UA_LOCK(&server->serviceMutex);
        ((UA_ChannelService)service)(server, channel, request, response);
        UA_UNLOCK(&server->serviceMutex);
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
        /* Store the authentication token so we can help fuzzing by setting
         * these values in the next request automatically */
        if(requestType == &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST]) {
            UA_CreateSessionResponse *res = &response->createSessionResponse;
            UA_NodeId_copy(&res->authenticationToken, &unsafe_fuzz_authenticationToken);
        }
#endif
        serviceRes = response->responseHeader.serviceResult;
        channelRes = sendResponse(server, NULL, channel, requestId, response, responseType);
        goto update_statistics;
    }

    /* Get the Session bound to the SecureChannel (not necessarily activated) */
    if(!UA_NodeId_isNull(&requestHeader->authenticationToken)) {
        UA_LOCK(&server->serviceMutex);
        UA_StatusCode retval =
            getBoundSession(server, channel,
                            &requestHeader->authenticationToken, &session);
        UA_UNLOCK(&server->serviceMutex);
        if(retval != UA_STATUSCODE_GOOD) {
            serviceRes = response->responseHeader.serviceResult;
            channelRes = sendServiceFault(channel, requestId,
                                          requestHeader->requestHandle, retval);
            goto update_statistics;
        }
    }

    /* Set an anonymous, inactive session for services that need no session */
    UA_Session anonymousSession;
    if(!session) {
        if(sessionRequired) {
#ifdef UA_ENABLE_TYPEDESCRIPTION
            UA_LOG_WARNING_CHANNEL(&server->config.logger, channel,
                                   "%s refused without a valid session",
                                   requestType->typeName);
#else
            UA_LOG_WARNING_CHANNEL(&server->config.logger, channel,
                                   "Service %" PRIu32 " refused without a valid session",
                                   requestType->binaryEncodingId.identifier.numeric);
#endif
            serviceRes = UA_STATUSCODE_BADSESSIONIDINVALID;
            channelRes = sendServiceFault(channel, requestId, requestHeader->requestHandle,
                                          UA_STATUSCODE_BADSESSIONIDINVALID);
            goto update_statistics;
        }

        UA_Session_init(&anonymousSession);
        anonymousSession.sessionId = UA_NODEID_GUID(0, UA_GUID_NULL);
        anonymousSession.header.channel = channel;
        session = &anonymousSession;
    }

    UA_assert(session != NULL);

    /* Trying to use a non-activated session? */
    if(sessionRequired && !session->activated) {
#ifdef UA_ENABLE_TYPEDESCRIPTION
        UA_LOG_WARNING_SESSION(&server->config.logger, session,
                               "%s refused on a non-activated session",
                               requestType->typeName);
#else
        UA_LOG_WARNING_SESSION(&server->config.logger, session,
                               "Service %" PRIu32 " refused on a non-activated session",
                               requestType->binaryEncodingId.identifier.numeric);
#endif
        if(session != &anonymousSession) {
            UA_LOCK(&server->serviceMutex);
            UA_Server_removeSessionByToken(server, &session->header.authenticationToken,
                                           UA_DIAGNOSTICEVENT_ABORT);
            UA_UNLOCK(&server->serviceMutex);
        }
        serviceRes = UA_STATUSCODE_BADSESSIONNOTACTIVATED;
        channelRes = sendServiceFault(channel, requestId, requestHeader->requestHandle,
                                      UA_STATUSCODE_BADSESSIONNOTACTIVATED);
        goto update_statistics;
    }

    /* Update the session lifetime */
    UA_Session_updateLifetime(session);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* The publish request is not answered immediately */
    if(requestType == &UA_TYPES[UA_TYPES_PUBLISHREQUEST]) {
        UA_LOCK(&server->serviceMutex);
        serviceRes = Service_Publish(server, session, &request->publishRequest, requestId);
        /* No channelRes due to the async response */
        UA_UNLOCK(&server->serviceMutex);
        goto update_statistics;
    }
#endif

#if UA_MULTITHREADING >= 100
    /* The call request might not be answered immediately */
    if(requestType == &UA_TYPES[UA_TYPES_CALLREQUEST]) {
        UA_Boolean finished = true;
        UA_LOCK(&server->serviceMutex);
        Service_CallAsync(server, session, requestId, &request->callRequest,
                          &response->callResponse, &finished);
        UA_UNLOCK(&server->serviceMutex);

        /* Async method calls remain. Don't send a response now. In case we have
         * an async call, count as a "good" request for the diagnostics
         * statistic. */
        if(UA_LIKELY(finished)) {
            serviceRes = response->responseHeader.serviceResult;
            channelRes = sendResponse(server, session, channel,
                                      requestId, response, responseType);
        }
        goto update_statistics;
    }
#endif

    /* Execute the synchronous service call */
    UA_LOCK(&server->serviceMutex);
    service(server, session, request, response);
    UA_UNLOCK(&server->serviceMutex);

    /* Send the response */
    serviceRes = response->responseHeader.serviceResult;
    channelRes = sendResponse(server, session, channel, requestId, response, responseType);

    /* Update the diagnostics statistics */
 update_statistics:
#ifdef UA_ENABLE_DIAGNOSTICS
    if(session && session != &server->adminSession) {
        session->diagnostics.totalRequestCount.totalCount++;
        if(serviceRes != UA_STATUSCODE_GOOD)
            session->diagnostics.totalRequestCount.errorCount++;
        if(counterOffset != 0) {
            UA_ServiceCounterDataType *serviceCounter = (UA_ServiceCounterDataType*)
                (((uintptr_t)&session->diagnostics) + counterOffset);
            serviceCounter->totalCount++;
            if(serviceRes != UA_STATUSCODE_GOOD)
                serviceCounter->errorCount++;
        }
    }
#else
    (void)serviceRes; /* Pacify compiler warnings */
#endif

    return channelRes;
}

static UA_StatusCode
processMSG(UA_Server *server, UA_SecureChannel *channel,
           UA_UInt32 requestId, const UA_ByteString *msg) {
    if(channel->state != UA_SECURECHANNELSTATE_OPEN)
        return UA_STATUSCODE_BADINTERNALERROR;
    /* Decode the nodeid */
    size_t offset = 0;
    UA_NodeId requestTypeId;
    UA_StatusCode retval = UA_NodeId_decodeBinary(msg, &offset, &requestTypeId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    if(requestTypeId.namespaceIndex != 0 ||
       requestTypeId.identifierType != UA_NODEIDTYPE_NUMERIC)
        UA_NodeId_clear(&requestTypeId); /* leads to badserviceunsupported */

    size_t requestPos = offset; /* Store the offset (for sendServiceFault) */

    /* Get the service pointers */
    UA_Service service = NULL;
    UA_Boolean sessionRequired = true;
    const UA_DataType *requestType = NULL;
    const UA_DataType *responseType = NULL;
    size_t counterOffset = 0;
    getServicePointers(requestTypeId.identifier.numeric, &requestType,
                       &responseType, &service, &sessionRequired, &counterOffset);
    if(!requestType) {
        if(requestTypeId.identifier.numeric ==
           UA_NS0ID_CREATESUBSCRIPTIONREQUEST_ENCODING_DEFAULTBINARY) {
            UA_LOG_INFO_CHANNEL(&server->config.logger, channel,
                                "Client requested a subscription, "
                                "but those are not enabled in the build");
        } else {
            UA_LOG_INFO_CHANNEL(&server->config.logger, channel,
                                "Unknown request with type identifier %" PRIi32,
                                requestTypeId.identifier.numeric);
        }
        return decodeHeaderSendServiceFault(channel, msg, requestPos,
                                            &UA_TYPES[UA_TYPES_SERVICEFAULT],
                                            requestId, UA_STATUSCODE_BADSERVICEUNSUPPORTED);
    }
    UA_assert(responseType);

    /* Decode the request */
    UA_Request request;
    retval = UA_decodeBinaryInternal(msg, &offset, &request,
                                     requestType, server->config.customDataTypes);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG_CHANNEL(&server->config.logger, channel,
                             "Could not decode the request with StatusCode %s",
                             UA_StatusCode_name(retval));
        return decodeHeaderSendServiceFault(channel, msg, requestPos,
                                            responseType, requestId, retval);
    }

    /* Check timestamp in the request header */
    UA_RequestHeader *requestHeader = &request.requestHeader;
    if(requestHeader->timestamp == 0) {
        if(server->config.verifyRequestTimestamp <= UA_RULEHANDLING_WARN) {
            UA_LOG_WARNING_CHANNEL(&server->config.logger, channel,
                                   "The server sends no timestamp in the request header. "
                                   "See the 'verifyRequestTimestamp' setting.");
            if(server->config.verifyRequestTimestamp <= UA_RULEHANDLING_ABORT) {
                retval = sendServiceFault(channel, requestId, requestHeader->requestHandle,
                                          UA_STATUSCODE_BADINVALIDTIMESTAMP);
                UA_clear(&request, requestType);
                return retval;
            }
        }
    }

#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    /* Set the authenticationToken from the create session request to help
     * fuzzing cover more lines */
    if(!UA_NodeId_isNull(&unsafe_fuzz_authenticationToken) &&
       !UA_NodeId_isNull(&requestHeader->authenticationToken)) {
        UA_NodeId_clear(&requestHeader->authenticationToken);
        UA_NodeId_copy(&unsafe_fuzz_authenticationToken, &requestHeader->authenticationToken);
    }
#endif

    /* Prepare the respone and process the request */
    UA_Response response;
    UA_init(&response, responseType);
    response.responseHeader.requestHandle = requestHeader->requestHandle;
    retval = processMSGDecoded(server, channel, requestId, service, &request, requestType,
                               &response, responseType, sessionRequired, counterOffset);

    /* Clean up */
    UA_clear(&request, requestType);
    UA_clear(&response, responseType);
    return retval;
}

/* Takes decoded messages starting at the nodeid of the content type. */
static UA_StatusCode
processSecureChannelMessage(void *application, UA_SecureChannel *channel,
                            UA_MessageType messagetype, UA_UInt32 requestId,
                            UA_ByteString *message) {
    UA_Server *server = (UA_Server*)application;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    switch(messagetype) {
    case UA_MESSAGETYPE_HEL:
        UA_LOG_TRACE_CHANNEL(&server->config.logger, channel, "Process a HEL message");
        retval = processHEL(server, channel, message);
        break;
    case UA_MESSAGETYPE_OPN:
        UA_LOG_TRACE_CHANNEL(&server->config.logger, channel, "Process an OPN message");
        retval = processOPN(server, channel, requestId, message);
        break;
    case UA_MESSAGETYPE_MSG:
        UA_LOG_TRACE_CHANNEL(&server->config.logger, channel, "Process a MSG");
        retval = processMSG(server, channel, requestId, message);
        break;
    case UA_MESSAGETYPE_CLO:
        UA_LOG_TRACE_CHANNEL(&server->config.logger, channel, "Process a CLO");
        Service_CloseSecureChannel(server, channel); /* Regular close */
        break;
    default:
        UA_LOG_TRACE_CHANNEL(&server->config.logger, channel, "Invalid message type");
        retval = UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
        break;
    }
    if(retval != UA_STATUSCODE_GOOD) {
        if(!channel->connection) {
            UA_LOG_INFO_CHANNEL(&server->config.logger, channel,
                                "Processing the message failed. Channel already closed "
                                "with StatusCode %s. ", UA_StatusCode_name(retval));
            return retval;
        }

        UA_LOG_INFO_CHANNEL(&server->config.logger, channel,
                            "Processing the message failed with StatusCode %s. "
                            "Closing the channel.", UA_StatusCode_name(retval));
        UA_TcpErrorMessage errMsg;
        UA_TcpErrorMessage_init(&errMsg);
        errMsg.error = retval;
        UA_Connection_sendError(channel->connection, &errMsg);
        switch(retval) {
        case UA_STATUSCODE_BADSECURITYMODEREJECTED:
        case UA_STATUSCODE_BADSECURITYCHECKSFAILED:
        case UA_STATUSCODE_BADSECURECHANNELIDINVALID:
        case UA_STATUSCODE_BADSECURECHANNELTOKENUNKNOWN:
        case UA_STATUSCODE_BADSECURITYPOLICYREJECTED:
        case UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED:
            UA_Server_closeSecureChannel(server, channel, UA_DIAGNOSTICEVENT_SECURITYREJECT);
            break;
        default:
            UA_Server_closeSecureChannel(server, channel, UA_DIAGNOSTICEVENT_CLOSE);
            break;
        }
    }

    return retval;
}

void
UA_Server_processBinaryMessage(UA_Server *server, UA_Connection *connection,
                               UA_ByteString *message) {
    UA_LOG_TRACE(&server->config.logger, UA_LOGCATEGORY_NETWORK,
                 "Connection %i | Received a packet.", (int)(connection->sockfd));

    UA_TcpErrorMessage error;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_SecureChannel *channel = connection->channel;

    /* Add a SecureChannel to a new connection */
    if(!channel) {
        retval = UA_Server_createSecureChannel(server, connection);
        if(retval != UA_STATUSCODE_GOOD)
            goto error;
        channel = connection->channel;
        UA_assert(channel);
    }

#ifdef UA_DEBUG_DUMP_PKGS
    UA_dump_hex_pkg(message->data, message->length);
#endif
#ifdef UA_DEBUG_DUMP_PKGS_FILE
    UA_debug_dumpCompleteChunk(server, channel->connection, message);
#endif

    retval = UA_SecureChannel_processBuffer(channel, server, processSecureChannelMessage, message);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_NETWORK,
                    "Connection %i | Processing the message failed with error %s",
                    (int)(connection->sockfd), UA_StatusCode_name(retval));
        goto error;
    }

    return;

 error:
    /* Send an ERR message and close the connection */
    error.error = retval;
    error.reason = UA_STRING_NULL;
    UA_Connection_sendError(connection, &error);
    connection->close(connection);
}
