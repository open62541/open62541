/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
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
#include <open62541/transport_generated_encoding_binary.h>
#include <open62541/transport_generated_handling.h>
#include <open62541/types_generated_encoding_binary.h>
#include <open62541/types_generated_handling.h>

#include "ua_securechannel_manager.h"
#include "ua_server_internal.h"
#include "ua_services.h"
#include "ua_session_manager.h"

#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
// store the authentication token and session ID so we can help fuzzing by setting
// these values in the next request automatically
UA_NodeId unsafe_fuzz_authenticationToken = {
        0, UA_NODEIDTYPE_NUMERIC, {0}
};
#endif

#ifdef UA_DEBUG_DUMP_PKGS_FILE
void UA_debug_dumpCompleteChunk(UA_Server *const server, UA_Connection *const connection, UA_ByteString *messageBuffer);
#endif

/********************/
/* Helper Functions */
/********************/

 /* This is not an ERR message, the connection is not closed afterwards */
static UA_StatusCode
sendServiceFault(UA_SecureChannel *channel, const UA_ByteString *msg,
                 size_t offset, const UA_DataType *responseType,
                 UA_UInt32 requestId, UA_StatusCode error) {
    UA_RequestHeader requestHeader;
    UA_StatusCode retval = UA_RequestHeader_decodeBinary(msg, &offset, &requestHeader);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    UA_STACKARRAY(UA_Byte, response, responseType->memSize);
    UA_init(response, responseType);
    UA_ResponseHeader *responseHeader = (UA_ResponseHeader*)response;
    responseHeader->requestHandle = requestHeader.requestHandle;
    responseHeader->timestamp = UA_DateTime_now();
    responseHeader->serviceResult = error;

    // Send error message. Message type is MSG and not ERR, since we are on a securechannel!
    retval = UA_SecureChannel_sendSymmetricMessage(channel, requestId, UA_MESSAGETYPE_MSG,
                                                   response, responseType);

    UA_RequestHeader_deleteMembers(&requestHeader);
    UA_LOG_DEBUG(channel->securityPolicy->logger, UA_LOGCATEGORY_SERVER,
                 "Sent ServiceFault with error code %s", UA_StatusCode_name(error));
    return retval;
}

typedef enum {
    UA_SERVICETYPE_NORMAL,
    UA_SERVICETYPE_INSITU,
    UA_SERVICETYPE_CUSTOM
} UA_ServiceType;

static void
getServicePointers(UA_UInt32 requestTypeId, const UA_DataType **requestType,
                   const UA_DataType **responseType, UA_Service *service,
                   UA_InSituService *serviceInsitu,
                   UA_Boolean *requiresSession, UA_ServiceType *serviceType) {
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
        *service = NULL; //(UA_Service)Service_CreateSession;
        *requestType = &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE];
        *requiresSession = false;
        *serviceType = UA_SERVICETYPE_CUSTOM;
        break;
    case UA_NS0ID_ACTIVATESESSIONREQUEST_ENCODING_DEFAULTBINARY:
        *service = NULL; //(UA_Service)Service_ActivateSession;
        *requestType = &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE];
        *serviceType = UA_SERVICETYPE_CUSTOM;
        break;
    case UA_NS0ID_CLOSESESSIONREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_CloseSession;
        *requestType = &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_CLOSESESSIONRESPONSE];
        break;
    case UA_NS0ID_READREQUEST_ENCODING_DEFAULTBINARY:
        *service = NULL;
        *serviceInsitu = (UA_InSituService)Service_Read;
        *requestType = &UA_TYPES[UA_TYPES_READREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_READRESPONSE];
        *serviceType = UA_SERVICETYPE_INSITU;
        break;
    case UA_NS0ID_WRITEREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_Write;
        *requestType = &UA_TYPES[UA_TYPES_WRITEREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_WRITERESPONSE];
        break;
    case UA_NS0ID_BROWSEREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_Browse;
        *requestType = &UA_TYPES[UA_TYPES_BROWSEREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_BROWSERESPONSE];
        break;
    case UA_NS0ID_BROWSENEXTREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_BrowseNext;
        *requestType = &UA_TYPES[UA_TYPES_BROWSENEXTREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_BROWSENEXTRESPONSE];
        break;
    case UA_NS0ID_REGISTERNODESREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_RegisterNodes;
        *requestType = &UA_TYPES[UA_TYPES_REGISTERNODESREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_REGISTERNODESRESPONSE];
        break;
    case UA_NS0ID_UNREGISTERNODESREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_UnregisterNodes;
        *requestType = &UA_TYPES[UA_TYPES_UNREGISTERNODESREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_UNREGISTERNODESRESPONSE];
        break;
    case UA_NS0ID_TRANSLATEBROWSEPATHSTONODEIDSREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_TranslateBrowsePathsToNodeIds;
        *requestType = &UA_TYPES[UA_TYPES_TRANSLATEBROWSEPATHSTONODEIDSREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_TRANSLATEBROWSEPATHSTONODEIDSRESPONSE];
        break;

#ifdef UA_ENABLE_SUBSCRIPTIONS
    case UA_NS0ID_CREATESUBSCRIPTIONREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_CreateSubscription;
        *requestType = &UA_TYPES[UA_TYPES_CREATESUBSCRIPTIONREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_CREATESUBSCRIPTIONRESPONSE];
        break;
    case UA_NS0ID_PUBLISHREQUEST_ENCODING_DEFAULTBINARY:
        *requestType = &UA_TYPES[UA_TYPES_PUBLISHREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_PUBLISHRESPONSE];
        break;
    case UA_NS0ID_REPUBLISHREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_Republish;
        *requestType = &UA_TYPES[UA_TYPES_REPUBLISHREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_REPUBLISHRESPONSE];
        break;
    case UA_NS0ID_MODIFYSUBSCRIPTIONREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_ModifySubscription;
        *requestType = &UA_TYPES[UA_TYPES_MODIFYSUBSCRIPTIONREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_MODIFYSUBSCRIPTIONRESPONSE];
        break;
    case UA_NS0ID_SETPUBLISHINGMODEREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_SetPublishingMode;
        *requestType = &UA_TYPES[UA_TYPES_SETPUBLISHINGMODEREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_SETPUBLISHINGMODERESPONSE];
        break;
    case UA_NS0ID_DELETESUBSCRIPTIONSREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_DeleteSubscriptions;
        *requestType = &UA_TYPES[UA_TYPES_DELETESUBSCRIPTIONSREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_DELETESUBSCRIPTIONSRESPONSE];
        break;
    case UA_NS0ID_CREATEMONITOREDITEMSREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_CreateMonitoredItems;
        *requestType = &UA_TYPES[UA_TYPES_CREATEMONITOREDITEMSREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_CREATEMONITOREDITEMSRESPONSE];
        break;
    case UA_NS0ID_DELETEMONITOREDITEMSREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_DeleteMonitoredItems;
        *requestType = &UA_TYPES[UA_TYPES_DELETEMONITOREDITEMSREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_DELETEMONITOREDITEMSRESPONSE];
        break;
    case UA_NS0ID_MODIFYMONITOREDITEMSREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_ModifyMonitoredItems;
        *requestType = &UA_TYPES[UA_TYPES_MODIFYMONITOREDITEMSREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_MODIFYMONITOREDITEMSRESPONSE];
        break;
    case UA_NS0ID_SETMONITORINGMODEREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_SetMonitoringMode;
        *requestType = &UA_TYPES[UA_TYPES_SETMONITORINGMODEREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_SETMONITORINGMODERESPONSE];
        break;
#endif
#ifdef UA_ENABLE_HISTORIZING
        /* For History read */
    case UA_NS0ID_HISTORYREADREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_HistoryRead;
        *requestType = &UA_TYPES[UA_TYPES_HISTORYREADREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_HISTORYREADRESPONSE];
        break;
        /* For History update */
    case UA_NS0ID_HISTORYUPDATEREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_HistoryUpdate;
        *requestType = &UA_TYPES[UA_TYPES_HISTORYUPDATEREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_HISTORYUPDATERESPONSE];
        break;
#endif

#ifdef UA_ENABLE_METHODCALLS
    case UA_NS0ID_CALLREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_Call;
        *requestType = &UA_TYPES[UA_TYPES_CALLREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_CALLRESPONSE];
        break;
#endif

#ifdef UA_ENABLE_NODEMANAGEMENT
    case UA_NS0ID_ADDNODESREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_AddNodes;
        *requestType = &UA_TYPES[UA_TYPES_ADDNODESREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_ADDNODESRESPONSE];
        break;
    case UA_NS0ID_ADDREFERENCESREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_AddReferences;
        *requestType = &UA_TYPES[UA_TYPES_ADDREFERENCESREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_ADDREFERENCESRESPONSE];
        break;
    case UA_NS0ID_DELETENODESREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_DeleteNodes;
        *requestType = &UA_TYPES[UA_TYPES_DELETENODESREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_DELETENODESRESPONSE];
        break;
    case UA_NS0ID_DELETEREFERENCESREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_DeleteReferences;
        *requestType = &UA_TYPES[UA_TYPES_DELETEREFERENCESREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_DELETEREFERENCESRESPONSE];
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
processHEL(UA_Server *server, UA_Connection *connection,
           const UA_ByteString *msg, size_t *offset) {
    UA_TcpHelloMessage helloMessage;
    UA_StatusCode retval = UA_TcpHelloMessage_decodeBinary(msg, offset, &helloMessage);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Currently not checked */
    UA_String_deleteMembers(&helloMessage.endpointUrl);

    /* TODO: Use the config of the exact NetworkLayer */
    if(server->config.networkLayersSize == 0)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    const UA_ConnectionConfig *localConfig = &server->config.networkLayers[0].localConnectionConfig;

    /* Parameterize the connection */
    UA_ConnectionConfig remoteConfig;
    remoteConfig.protocolVersion = helloMessage.protocolVersion;
    remoteConfig.sendBufferSize = helloMessage.sendBufferSize;
    remoteConfig.recvBufferSize = helloMessage.receiveBufferSize;
    remoteConfig.maxMessageSize = helloMessage.maxMessageSize;
    remoteConfig.maxChunkCount = helloMessage.maxChunkCount;
    retval = UA_Connection_processHELACK(connection, localConfig, &remoteConfig);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_NETWORK,
                    "Connection %i | Error during the HEL/ACK handshake",
                    (int)(connection->sockfd));
        return retval;
    }

    /* Build acknowledge response */
    UA_TcpAcknowledgeMessage ackMessage;
    memcpy(&ackMessage, localConfig, sizeof(UA_TcpAcknowledgeMessage)); /* Same struct layout.. */
    UA_TcpMessageHeader ackHeader;
    ackHeader.messageTypeAndChunkType = UA_MESSAGETYPE_ACK + UA_CHUNKTYPE_FINAL;
    ackHeader.messageSize = 8 + 20; /* ackHeader + ackMessage */

    /* Get the send buffer from the network layer */
    UA_ByteString ack_msg;
    UA_ByteString_init(&ack_msg);
    retval = connection->getSendBuffer(connection, connection->config.sendBufferSize, &ack_msg);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Encode and send the response */
    UA_Byte *bufPos = ack_msg.data;
    const UA_Byte *bufEnd = &ack_msg.data[ack_msg.length];

    retval = UA_TcpMessageHeader_encodeBinary(&ackHeader, &bufPos, bufEnd);
    if(retval != UA_STATUSCODE_GOOD) {
        connection->releaseSendBuffer(connection, &ack_msg);
        return retval;
    }

    retval = UA_TcpAcknowledgeMessage_encodeBinary(&ackMessage, &bufPos, bufEnd);
    if(retval != UA_STATUSCODE_GOOD) {
        connection->releaseSendBuffer(connection, &ack_msg);
        return retval;
    }
    ack_msg.length = ackHeader.messageSize;
    return connection->send(connection, &ack_msg);
}

/* OPN -> Open up/renew the securechannel */
static UA_StatusCode
processOPN(UA_Server *server, UA_SecureChannel *channel,
           const UA_UInt32 requestId, const UA_ByteString *msg) {
    /* Decode the request */
    size_t offset = 0;
    UA_NodeId requestType;
    UA_OpenSecureChannelRequest openSecureChannelRequest;
    UA_StatusCode retval = UA_NodeId_decodeBinary(msg, &offset, &requestType);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_NodeId_deleteMembers(&requestType);
        UA_LOG_INFO_CHANNEL(&server->config.logger, channel,
                            "Could not decode the NodeId. Closing the connection");
        UA_SecureChannelManager_close(&server->secureChannelManager, channel->securityToken.channelId);
        return retval;
    }
    retval = UA_OpenSecureChannelRequest_decodeBinary(msg, &offset, &openSecureChannelRequest);

    /* Error occurred */
    if(retval != UA_STATUSCODE_GOOD ||
       requestType.identifier.numeric != UA_TYPES[UA_TYPES_OPENSECURECHANNELREQUEST].binaryEncodingId) {
        UA_NodeId_deleteMembers(&requestType);
        UA_OpenSecureChannelRequest_deleteMembers(&openSecureChannelRequest);
        UA_LOG_INFO_CHANNEL(&server->config.logger, channel,
                            "Could not decode the OPN message. Closing the connection.");
        UA_SecureChannelManager_close(&server->secureChannelManager, channel->securityToken.channelId);
        return retval;
    }
    UA_NodeId_deleteMembers(&requestType);

    /* Call the service */
    UA_OpenSecureChannelResponse openScResponse;
    UA_OpenSecureChannelResponse_init(&openScResponse);
    Service_OpenSecureChannel(server, channel, &openSecureChannelRequest, &openScResponse);
    UA_OpenSecureChannelRequest_deleteMembers(&openSecureChannelRequest);
    if(openScResponse.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_CHANNEL(&server->config.logger, channel, "Could not open a SecureChannel. "
                            "Closing the connection.");
        UA_SecureChannelManager_close(&server->secureChannelManager,
                                      channel->securityToken.channelId);
        return openScResponse.responseHeader.serviceResult;
    }

    /* Send the response */
    retval = UA_SecureChannel_sendAsymmetricOPNMessage(channel, requestId, &openScResponse,
                                                       &UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE]);
    UA_OpenSecureChannelResponse_deleteMembers(&openScResponse);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_CHANNEL(&server->config.logger, channel,
                            "Could not send the OPN answer with error code %s",
                            UA_StatusCode_name(retval));
        UA_SecureChannelManager_close(&server->secureChannelManager,
                                      channel->securityToken.channelId);
        return retval;
    }

    return retval;
}

static UA_StatusCode
processMSG(UA_Server *server, UA_SecureChannel *channel,
           UA_UInt32 requestId, const UA_ByteString *msg) {
    /* At 0, the nodeid starts... */
    size_t offset = 0;

    /* Decode the nodeid */
    UA_NodeId requestTypeId;
    UA_StatusCode retval = UA_NodeId_decodeBinary(msg, &offset, &requestTypeId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    if(requestTypeId.namespaceIndex != 0 ||
       requestTypeId.identifierType != UA_NODEIDTYPE_NUMERIC)
        UA_NodeId_deleteMembers(&requestTypeId); /* leads to badserviceunsupported */

    /* Store the start-position of the request */
    size_t requestPos = offset;

    /* Get the service pointers */
    UA_Service service = NULL;
    UA_InSituService serviceInsitu = NULL;
    const UA_DataType *requestType = NULL;
    const UA_DataType *responseType = NULL;
    UA_Boolean sessionRequired = true;
    UA_ServiceType serviceType = UA_SERVICETYPE_NORMAL;
    getServicePointers(requestTypeId.identifier.numeric, &requestType,
                       &responseType, &service, &serviceInsitu, &sessionRequired, &serviceType);
    if(!requestType) {
        if(requestTypeId.identifier.numeric == 787) {
            UA_LOG_INFO_CHANNEL(&server->config.logger, channel,
                                "Client requested a subscription, " \
                                "but those are not enabled in the build");
        } else {
            UA_LOG_INFO_CHANNEL(&server->config.logger, channel,
                                "Unknown request with type identifier %i",
                                requestTypeId.identifier.numeric);
        }
        return sendServiceFault(channel, msg, requestPos, &UA_TYPES[UA_TYPES_SERVICEFAULT],
                                requestId, UA_STATUSCODE_BADSERVICEUNSUPPORTED);
    }
    UA_assert(responseType);

    /* Decode the request */
    UA_STACKARRAY(UA_Byte, request, requestType->memSize);
    UA_RequestHeader *requestHeader = (UA_RequestHeader*)request;
    retval = UA_decodeBinary(msg, &offset, request, requestType, server->config.customDataTypes);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG_CHANNEL(&server->config.logger, channel,
                             "Could not decode the request");
        return sendServiceFault(channel, msg, requestPos, responseType, requestId, retval);
    }

    /* Prepare the respone */
    UA_STACKARRAY(UA_Byte, responseBuf, responseType->memSize);
    void *response = (void*)(uintptr_t)&responseBuf[0]; /* Get around aliasing rules */
    UA_init(response, responseType);
    UA_Session *session = NULL; /* must be initialized before goto send_response */

    /* CreateSession doesn't need a session */
    if(requestType == &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST]) {
        Service_CreateSession(server, channel,
            (const UA_CreateSessionRequest *)request,
                              (UA_CreateSessionResponse *)response);
        #ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
        // store the authentication token and session ID so we can help fuzzing by setting
        // these values in the next request automatically
        UA_CreateSessionResponse *res = (UA_CreateSessionResponse *)response;
        UA_NodeId_copy(&res->authenticationToken, &unsafe_fuzz_authenticationToken);
        #endif
        goto send_response;
    }

    #ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    // set the authenticationToken from the create session request to help fuzzing cover more lines
    UA_NodeId_deleteMembers(&requestHeader->authenticationToken);
    if(!UA_NodeId_isNull(&unsafe_fuzz_authenticationToken))
        UA_NodeId_copy(&unsafe_fuzz_authenticationToken, &requestHeader->authenticationToken);
    #endif

    /* Find the matching session */
    session = (UA_Session*)UA_SecureChannel_getSession(channel, &requestHeader->authenticationToken);
    if(!session && !UA_NodeId_isNull(&requestHeader->authenticationToken))
        session = UA_SessionManager_getSessionByToken(&server->sessionManager,
                                                      &requestHeader->authenticationToken);

    if(requestType == &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST]) {
        if(!session) {
            UA_LOG_DEBUG_CHANNEL(&server->config.logger, channel,
                                 "Trying to activate a session that is " \
                                 "not known in the server");
            UA_deleteMembers(request, requestType);
            return sendServiceFault(channel, msg, requestPos, responseType,
                                    requestId, UA_STATUSCODE_BADSESSIONIDINVALID);
        }
        Service_ActivateSession(server, channel, session,
            (const UA_ActivateSessionRequest*)request,
                                (UA_ActivateSessionResponse*)response);
        goto send_response;
    }

    /* Set an anonymous, inactive session for services that need no session */
    UA_Session anonymousSession;
    if(!session) {
        if(sessionRequired) {
            UA_LOG_WARNING_CHANNEL(&server->config.logger, channel,
                                   "Service request %i without a valid session",
                                   requestType->binaryEncodingId);
            UA_deleteMembers(request, requestType);
            return sendServiceFault(channel, msg, requestPos, responseType,
                                    requestId, UA_STATUSCODE_BADSESSIONIDINVALID);
        }

        UA_Session_init(&anonymousSession);
        anonymousSession.sessionId = UA_NODEID_GUID(0, UA_GUID_NULL);
        anonymousSession.header.channel = channel;
        session = &anonymousSession;
    }

    /* Trying to use a non-activated session?
     * Do not allow if request is of type CloseSessionRequest */
    if(sessionRequired && !session->activated && requestType != &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST]) {
        UA_LOG_WARNING_SESSION(&server->config.logger, session,
                               "Calling service %i on a non-activated session",
                               requestType->binaryEncodingId);
        UA_SessionManager_removeSession(&server->sessionManager,
                                        &session->header.authenticationToken);
        UA_deleteMembers(request, requestType);
        return sendServiceFault(channel, msg, requestPos, responseType,
                                requestId, UA_STATUSCODE_BADSESSIONNOTACTIVATED);
    }

    /* The session is bound to another channel */
    if(session != &anonymousSession && session->header.channel != channel) {
        UA_LOG_WARNING_CHANNEL(&server->config.logger, channel,
                               "Client tries to use a Session that is not "
                               "bound to this SecureChannel");
        UA_deleteMembers(request, requestType);
        return sendServiceFault(channel, msg, requestPos, responseType,
                                requestId, UA_STATUSCODE_BADSECURECHANNELIDINVALID);
    }

    /* Update the session lifetime */
    UA_Session_updateLifetime(session);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* The publish request is not answered immediately */
    if(requestType == &UA_TYPES[UA_TYPES_PUBLISHREQUEST]) {
        Service_Publish(server, session,
            (const UA_PublishRequest*)request, requestId);
        UA_deleteMembers(request, requestType);
        return UA_STATUSCODE_GOOD;
    }
#endif

    send_response:

    /* Prepare the ResponseHeader */
    ((UA_ResponseHeader*)response)->requestHandle = requestHeader->requestHandle;
    ((UA_ResponseHeader*)response)->timestamp = UA_DateTime_now();

    /* Check timestamp in the request header */
    if(requestHeader->timestamp == 0) {
        if(server->config.verifyRequestTimestamp <= UA_RULEHANDLING_WARN) {
            UA_LOG_WARNING_CHANNEL(&server->config.logger, channel,
                                   "The server sends no timestamp in the request header. "
                                   "See the 'verifyRequestTimestamp' setting.");
            if(server->config.verifyRequestTimestamp <= UA_RULEHANDLING_ABORT)
                return sendServiceFault(channel, msg, requestPos, responseType,
                                        requestId, UA_STATUSCODE_BADINVALIDTIMESTAMP);
        }
    }

    /* Process normal services before initializing the message context.
     * Some services may initialize new message contexts and to support network
     * layers only providing one send buffer, only one message context can be
     * initialized concurrently. */
    if(serviceType == UA_SERVICETYPE_NORMAL)
        service(server, session, request, response);

    /* Start the message */
    UA_NodeId typeId = UA_NODEID_NUMERIC(0, responseType->binaryEncodingId);
    UA_MessageContext mc;
    retval = UA_MessageContext_begin(&mc, channel, requestId, UA_MESSAGETYPE_MSG);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Assert's required for clang-analyzer */
    UA_assert(mc.buf_pos == &mc.messageBuffer.data[UA_SECURE_MESSAGE_HEADER_LENGTH]);
    UA_assert(mc.buf_end <= &mc.messageBuffer.data[mc.messageBuffer.length]);

    retval = UA_MessageContext_encode(&mc, &typeId, &UA_TYPES[UA_TYPES_NODEID]);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    switch(serviceType) {
    case UA_SERVICETYPE_CUSTOM:
        /* Was processed before...*/
        retval = UA_MessageContext_encode(&mc, response, responseType);
        break;
    case UA_SERVICETYPE_INSITU:
        retval = serviceInsitu
            (server, session, &mc, request, (UA_ResponseHeader*)response);
        break;
    case UA_SERVICETYPE_NORMAL:
    default:
        retval = UA_MessageContext_encode(&mc, response, responseType);
        break;
    }

    /* Finish sending the message */
    if(retval != UA_STATUSCODE_GOOD) {
        UA_MessageContext_abort(&mc);
        goto cleanup;
    }

    retval = UA_MessageContext_finish(&mc);

 cleanup:
    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_INFO_CHANNEL(&server->config.logger, channel,
                            "Could not send the message over the SecureChannel "
                            "with StatusCode %s", UA_StatusCode_name(retval));
    /* Clean up */
    UA_deleteMembers(request, requestType);
    UA_deleteMembers(response, responseType);
    return retval;
}

/* Takes decoded messages starting at the nodeid of the content type. */
static void
processSecureChannelMessage(void *application, UA_SecureChannel *channel,
                            UA_MessageType messagetype, UA_UInt32 requestId,
                            const UA_ByteString *message) {
    UA_Server *server = (UA_Server*)application;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    switch(messagetype) {
    case UA_MESSAGETYPE_OPN:
        UA_LOG_TRACE_CHANNEL(&server->config.logger, channel,
                             "Process an OPN on an open channel");
        retval = processOPN(server, channel, requestId, message);
        break;
    case UA_MESSAGETYPE_MSG:
        UA_LOG_TRACE_CHANNEL(&server->config.logger, channel, "Process a MSG");
        retval = processMSG(server, channel, requestId, message);
        break;
    case UA_MESSAGETYPE_CLO:
        UA_LOG_TRACE_CHANNEL(&server->config.logger, channel, "Process a CLO");
        Service_CloseSecureChannel(server, channel);
        break;
    default:
        UA_LOG_TRACE_CHANNEL(&server->config.logger, channel, "Invalid message type");
        retval = UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
        break;
    }
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_CHANNEL(&server->config.logger, channel,
                            "Processing the message failed with StatusCode %s. "
                            "Closing the channel.", UA_StatusCode_name(retval));
        Service_CloseSecureChannel(server, channel);
    }
}

static UA_StatusCode
createSecureChannel(void *application, UA_Connection *connection,
                    UA_AsymmetricAlgorithmSecurityHeader *asymHeader) {
    UA_Server *server = (UA_Server*)application;

    /* Iterate over available endpoints and choose the correct one */
    UA_EndpointDescription *endpoint = NULL;
    UA_SecurityPolicy *securityPolicy = NULL;
    for(size_t i = 0; i < server->config.endpointsSize; ++i) {
        UA_EndpointDescription *endpointCandidate = &server->config.endpoints[i];
        if(!UA_ByteString_equal(&asymHeader->securityPolicyUri,
                                &endpointCandidate->securityPolicyUri))
            continue;
        securityPolicy = UA_SecurityPolicy_getSecurityPolicyByUri(server,
                                                                  (UA_ByteString*)&endpointCandidate->securityPolicyUri);
        if(!securityPolicy)
            return UA_STATUSCODE_BADINTERNALERROR;

        UA_StatusCode retval = securityPolicy->asymmetricModule.
            compareCertificateThumbprint(securityPolicy,
                                         &asymHeader->receiverCertificateThumbprint);
        if(retval != UA_STATUSCODE_GOOD)
            continue;

        /* We found the correct endpoint (except for security mode) The endpoint
         * needs to be changed by the client / server to match the security
         * mode. The server does this in the securechannel manager */
        endpoint = endpointCandidate;
        break;
    }

    if(!endpoint)
        return UA_STATUSCODE_BADSECURITYPOLICYREJECTED;

    /* Create a new channel */
    return UA_SecureChannelManager_create(&server->secureChannelManager, connection,
                                          securityPolicy, asymHeader);
}

static UA_StatusCode
processCompleteChunkWithoutChannel(UA_Server *server, UA_Connection *connection,
                                   UA_ByteString *message) {
    /* Process chunk without a channel; must be OPN */
    UA_LOG_TRACE(&server->config.logger, UA_LOGCATEGORY_NETWORK,
                 "Connection %i | No channel attached to the connection. "
                 "Process the chunk directly", (int)(connection->sockfd));
    size_t offset = 0;
    UA_TcpMessageHeader tcpMessageHeader;
    UA_StatusCode retval =
        UA_TcpMessageHeader_decodeBinary(message, &offset, &tcpMessageHeader);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    // Only HEL and OPN messages possible without a channel (on the server side)
    switch(tcpMessageHeader.messageTypeAndChunkType & 0x00ffffffu) {
    case UA_MESSAGETYPE_HEL:
        retval = processHEL(server, connection, message, &offset);
        break;
    case UA_MESSAGETYPE_OPN:
    {
        UA_LOG_TRACE(&server->config.logger, UA_LOGCATEGORY_NETWORK,
                     "Connection %i | Process OPN message", (int)(connection->sockfd));

        /* Called before HEL */
        if(connection->state != UA_CONNECTION_ESTABLISHED) {
            retval = UA_STATUSCODE_BADCOMMUNICATIONERROR;
            break;
        }

        // Decode the asymmetric algorithm security header since it is not encrypted and
        // needed to decide what security policy to use.
        UA_AsymmetricAlgorithmSecurityHeader asymHeader;
        UA_AsymmetricAlgorithmSecurityHeader_init(&asymHeader);
        size_t messageHeaderOffset = UA_SECURE_CONVERSATION_MESSAGE_HEADER_LENGTH;
        retval = UA_AsymmetricAlgorithmSecurityHeader_decodeBinary(message,
                                                                   &messageHeaderOffset,
                                                                   &asymHeader);
        if(retval != UA_STATUSCODE_GOOD)
            break;

        retval = createSecureChannel(server, connection, &asymHeader);
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
        if(retval != UA_STATUSCODE_GOOD)
            break;

        retval = UA_SecureChannel_decryptAddChunk(connection->channel, message, false);
        if(retval != UA_STATUSCODE_GOOD)
            break;

        UA_SecureChannel_processCompleteMessages(connection->channel, server,
                                                 processSecureChannelMessage);
        break;
    }
    default:
        UA_LOG_TRACE(&server->config.logger, UA_LOGCATEGORY_NETWORK,
                     "Connection %i | Expected OPN or HEL message on a connection "
                     "without a SecureChannel", (int)(connection->sockfd));
        retval = UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
        break;
    }
    return retval;
}

static UA_StatusCode
processCompleteChunk(void *const application, UA_Connection *connection,
                     UA_ByteString *chunk) {
    UA_Server *server = (UA_Server*)application;
#ifdef UA_DEBUG_DUMP_PKGS_FILE
    UA_debug_dumpCompleteChunk(server, connection, chunk);
#endif
    if(!connection->channel)
        return processCompleteChunkWithoutChannel(server, connection, chunk);
    return UA_SecureChannel_decryptAddChunk(connection->channel, chunk, false);
}

void
UA_Server_processBinaryMessage(UA_Server *server, UA_Connection *connection,
                               UA_ByteString *message) {
    UA_LOG_TRACE(&server->config.logger, UA_LOGCATEGORY_NETWORK,
                 "Connection %i | Received a packet.", (int)(connection->sockfd));
#ifdef UA_DEBUG_DUMP_PKGS
    UA_dump_hex_pkg(message->data, message->length);
#endif

    UA_StatusCode retval = UA_Connection_processChunks(connection, server,
                                                       processCompleteChunk, message);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_NETWORK,
                    "Connection %i | Processing the message failed with "
                    "error %s", (int)(connection->sockfd), UA_StatusCode_name(retval));
        /* Send an ERR message and close the connection */
        UA_TcpErrorMessage error;
        error.error = retval;
        error.reason = UA_STRING_NULL;
        UA_Connection_sendError(connection, &error);
        connection->close(connection);
        return;
    }

    UA_SecureChannel *channel = connection->channel;
    if(!channel)
        return;

    /* Process complete messages */
    UA_SecureChannel_processCompleteMessages(channel, server, processSecureChannelMessage);

    /* Is the channel still open? */
    if(channel->state == UA_SECURECHANNELSTATE_CLOSED)
        return;

    /* Store unused decoded chunks internally in the SecureChannel */
    UA_SecureChannel_persistIncompleteMessages(connection->channel);
}

#ifdef UA_ENABLE_MULTITHREADING
static void
deleteConnection(UA_Server *server, UA_Connection *connection) {
    connection->free(connection);
}
#endif

void
UA_Server_removeConnection(UA_Server *server, UA_Connection *connection) {
    UA_Connection_detachSecureChannel(connection);
#ifndef UA_ENABLE_MULTITHREADING
    connection->free(connection);
#else
    UA_DelayedCallback *dc = (UA_DelayedCallback*)UA_malloc(sizeof(UA_DelayedCallback));
    if(!dc)
        return; /* Malloc cannot fail on OS's that support multithreading. They
                 * rather kill the process. */
    dc->callback = (UA_ApplicationCallback)deleteConnection;
    dc->application = server;
    dc->data = connection;
    UA_WorkQueue_enqueueDelayed(&server->workQueue, dc);
#endif
}
