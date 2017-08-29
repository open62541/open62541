/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_util.h"
#include "ua_server_internal.h"
#include "ua_services.h"
#include "ua_securechannel_manager.h"
#include "ua_session_manager.h"
#include "ua_types_generated_encoding_binary.h"
#include "ua_transport_generated.h"
#include "ua_transport_generated_handling.h"
#include "ua_transport_generated_encoding_binary.h"

/********************/
/* Helper Functions */
/********************/

static void
sendError(UA_SecureChannel *channel, const UA_ByteString *msg,
          size_t offset, const UA_DataType *responseType,
          UA_UInt32 requestId, UA_StatusCode error) {
    UA_RequestHeader requestHeader;
    UA_StatusCode retval = UA_RequestHeader_decodeBinary(msg, &offset, &requestHeader);
    if(retval != UA_STATUSCODE_GOOD)
        return;
    void *response = UA_alloca(responseType->memSize);
    UA_init(response, responseType);
    UA_ResponseHeader *responseHeader = (UA_ResponseHeader*)response;
    responseHeader->requestHandle = requestHeader.requestHandle;
    responseHeader->timestamp = UA_DateTime_now();
    responseHeader->serviceResult = error;
    UA_SecureChannel_sendBinaryMessage(channel, requestId, response, responseType);
    UA_RequestHeader_deleteMembers(&requestHeader);
    UA_ResponseHeader_deleteMembers(responseHeader);
}

static void
getServicePointers(UA_UInt32 requestTypeId, const UA_DataType **requestType,
                   const UA_DataType **responseType, UA_Service *service,
                   UA_Boolean *requiresSession) {
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
        *service = (UA_Service)Service_Read;
        *requestType = &UA_TYPES[UA_TYPES_READREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_READRESPONSE];
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
static void
processHEL(UA_Server *server, UA_Connection *connection, const UA_ByteString *msg, size_t *offset) {
    UA_TcpHelloMessage helloMessage;
    if(UA_TcpHelloMessage_decodeBinary(msg, offset, &helloMessage) != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_NETWORK,
                    "Connection %i | Could not decode the HEL message. Closing the connection.",
                    connection->sockfd);
        connection->close(connection);
        return;
    }

    /* Parameterize the connection */
    connection->remoteConf.maxChunkCount = helloMessage.maxChunkCount; /* zero -> unlimited */
    connection->remoteConf.maxMessageSize = helloMessage.maxMessageSize; /* zero -> unlimited */
    connection->remoteConf.protocolVersion = helloMessage.protocolVersion;
    connection->remoteConf.recvBufferSize = helloMessage.receiveBufferSize;
    if(connection->localConf.sendBufferSize > helloMessage.receiveBufferSize)
        connection->localConf.sendBufferSize = helloMessage.receiveBufferSize;
    connection->remoteConf.sendBufferSize = helloMessage.sendBufferSize;
    if(connection->localConf.recvBufferSize > helloMessage.sendBufferSize)
        connection->localConf.recvBufferSize = helloMessage.sendBufferSize;
    connection->state = UA_CONNECTION_ESTABLISHED;
    UA_TcpHelloMessage_deleteMembers(&helloMessage);

    if (connection->remoteConf.recvBufferSize == 0) {
        UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_NETWORK,
                    "Connection %i | Remote end indicated a receive buffer size of 0. Not able to send any messages.",
                    connection->sockfd);
        connection->close(connection);
        return;
    }

    /* Build acknowledge response */
    UA_TcpAcknowledgeMessage ackMessage;
    ackMessage.protocolVersion = connection->localConf.protocolVersion;
    ackMessage.receiveBufferSize = connection->localConf.recvBufferSize;
    ackMessage.sendBufferSize = connection->localConf.sendBufferSize;
    ackMessage.maxMessageSize = connection->localConf.maxMessageSize;
    ackMessage.maxChunkCount = connection->localConf.maxChunkCount;

    UA_TcpMessageHeader ackHeader;
    ackHeader.messageTypeAndChunkType = UA_MESSAGETYPE_ACK + UA_CHUNKTYPE_FINAL;
    ackHeader.messageSize = 8 + 20; /* ackHeader + ackMessage */

    /* Get the send buffer from the network layer */
    UA_ByteString ack_msg;
    UA_ByteString_init(&ack_msg);
    UA_StatusCode retval =
        connection->getSendBuffer(connection, connection->localConf.sendBufferSize, &ack_msg);
    if(retval != UA_STATUSCODE_GOOD)
        return;

    /* Encode and send the response */
    UA_Byte *bufPos = ack_msg.data;
    const UA_Byte *bufEnd = &ack_msg.data[ack_msg.length];
    UA_TcpMessageHeader_encodeBinary(&ackHeader, &bufPos, &bufEnd);
    UA_TcpAcknowledgeMessage_encodeBinary(&ackMessage, &bufPos, &bufEnd);
    ack_msg.length = ackHeader.messageSize;
    connection->send(connection, &ack_msg);
}

/* OPN -> Open up/renew the securechannel */
static void
processOPN(UA_Server *server, UA_Connection *connection,
           UA_UInt32 channelId, const UA_ByteString *msg) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    /* Called before HEL */
    if(connection->state != UA_CONNECTION_ESTABLISHED)
        retval = UA_STATUSCODE_BADCOMMUNICATIONERROR;
    /* Opening up a channel with a channelid already set */
    if(!connection->channel && channelId != 0)
        retval = UA_STATUSCODE_BADCOMMUNICATIONERROR;
    /* Renew a channel with the wrong channelid */
    if(connection->channel && channelId != connection->channel->securityToken.channelId)
        retval = UA_STATUSCODE_BADCOMMUNICATIONERROR;

    /* Decode the request */
    UA_AsymmetricAlgorithmSecurityHeader asymHeader;
    UA_SequenceHeader seqHeader;
    UA_NodeId requestType;
    UA_OpenSecureChannelRequest r;
    size_t offset = 0;
    retval |= UA_AsymmetricAlgorithmSecurityHeader_decodeBinary(msg, &offset, &asymHeader);
    retval |= UA_SequenceHeader_decodeBinary(msg, &offset, &seqHeader);
    retval |= UA_NodeId_decodeBinary(msg, &offset, &requestType);
    retval |= UA_OpenSecureChannelRequest_decodeBinary(msg, &offset, &r);

    /* Error occured */
    if(retval != UA_STATUSCODE_GOOD || requestType.identifier.numeric != 446) {
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
        UA_NodeId_deleteMembers(&requestType);
        UA_OpenSecureChannelRequest_deleteMembers(&r);
        UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_NETWORK,
                    "Connection %i | Could not decode the OPN message. Closing the connection.",
                    connection->sockfd);
        connection->close(connection);
        return;
    }

    /* Call the service */
    UA_OpenSecureChannelResponse p;
    UA_OpenSecureChannelResponse_init(&p);
    Service_OpenSecureChannel(server, connection, &r, &p);
    UA_OpenSecureChannelRequest_deleteMembers(&r);

    /* Opening the channel failed */
    UA_SecureChannel *channel = connection->channel;
    if(!channel) {
        UA_OpenSecureChannelResponse_deleteMembers(&p);
        UA_NodeId_deleteMembers(&requestType);
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
        UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_NETWORK,
                    "Connection %i | Could not open a SecureChannel. "
                    "Closing the connection.", connection->sockfd);
        connection->close(connection);
        return;
    }

    // every message contains secureconversationmessageheader. Thus its size has to be
    // the minimum buffer size
    if (connection->localConf.sendBufferSize <= 12) {
        UA_OpenSecureChannelResponse_deleteMembers(&p);
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
        UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_NETWORK,
                    "Connection %i | Response too large for client configuration. %s", connection->sockfd, UA_StatusCode_name(UA_STATUSCODE_BADRESPONSETOOLARGE));
        connection->close(connection);
        return;
    }

    /* Set the starting sequence number */
    channel->receiveSequenceNumber = seqHeader.sequenceNumber;

    /* Allocate the return message */
    UA_ByteString resp_msg;
    UA_ByteString_init(&resp_msg);
    retval = connection->getSendBuffer(connection, connection->localConf.sendBufferSize, &resp_msg);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_OpenSecureChannelResponse_deleteMembers(&p);
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
        UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_NETWORK,
                    "Connection %i | Could not obtain a buffer to answer the OPN message. "
                    "Closing the connection.", connection->sockfd);
        connection->close(connection);
        return;
    }

    /* Encode the message after the secureconversationmessageheader */
    UA_Byte *bufPos = &resp_msg.data[12]; /* skip the header */
    const UA_Byte *bufEnd = &resp_msg.data[resp_msg.length];
    seqHeader.sequenceNumber = UA_atomic_add(&channel->sendSequenceNumber, 1);
    retval |= UA_AsymmetricAlgorithmSecurityHeader_encodeBinary(&asymHeader, &bufPos, &bufEnd); // just mirror back
    retval |= UA_SequenceHeader_encodeBinary(&seqHeader, &bufPos, &bufEnd);
    UA_NodeId responseType = UA_NODEID_NUMERIC(0, UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE].binaryEncodingId);
    retval |= UA_NodeId_encodeBinary(&responseType, &bufPos, &bufEnd);
    retval |= UA_OpenSecureChannelResponse_encodeBinary(&p, &bufPos, &bufEnd);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_NETWORK,
                    "Connection %i | Could not encode the OPN message. "
                    "Closing the connection. %s", connection->sockfd, UA_StatusCode_name(retval));
        connection->releaseSendBuffer(connection, &resp_msg);
        UA_OpenSecureChannelResponse_deleteMembers(&p);
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
        connection->close(connection);
        return;
    }

    /* Encode the secureconversationmessageheader (cannot fail) and send */
    UA_SecureConversationMessageHeader respHeader;
    respHeader.messageHeader.messageTypeAndChunkType = UA_MESSAGETYPE_OPN + UA_CHUNKTYPE_FINAL;
    respHeader.messageHeader.messageSize = (u32)((uintptr_t)bufPos - (uintptr_t)resp_msg.data);
    respHeader.secureChannelId = p.securityToken.channelId;
    bufPos = resp_msg.data;
    UA_SecureConversationMessageHeader_encodeBinary(&respHeader, &bufPos, &bufEnd);
    resp_msg.length = respHeader.messageHeader.messageSize;
    connection->send(connection, &resp_msg);

    /* Clean up */
    UA_OpenSecureChannelResponse_deleteMembers(&p);
    UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
}

static void
processMSG(UA_Server *server, UA_SecureChannel *channel,
           UA_UInt32 requestId, const UA_ByteString *msg) {
    /* At 0, the nodeid starts... */
    size_t ppos = 0;
    size_t *offset = &ppos;

    /* Decode the nodeid */
    UA_NodeId requestTypeId;
    UA_StatusCode retval = UA_NodeId_decodeBinary(msg, offset, &requestTypeId);
    if(retval != UA_STATUSCODE_GOOD)
        return;
    if(requestTypeId.identifierType != UA_NODEIDTYPE_NUMERIC)
        UA_NodeId_deleteMembers(&requestTypeId); /* leads to badserviceunsupported */

    /* Store the start-position of the request */
    size_t requestPos = *offset;

    /* Get the service pointers */
    UA_Service service = NULL;
    const UA_DataType *requestType = NULL;
    const UA_DataType *responseType = NULL;
    UA_Boolean sessionRequired = true;
    getServicePointers(requestTypeId.identifier.numeric, &requestType,
                       &responseType, &service, &sessionRequired);
    if(!requestType) {
        if(requestTypeId.identifier.numeric == 787) {
            UA_LOG_INFO_CHANNEL(server->config.logger, channel,
                                "Client requested a subscription, " \
                                "but those are not enabled in the build");
        } else {
            UA_LOG_INFO_CHANNEL(server->config.logger, channel,
                                "Unknown request with type identifier %i",
                                requestTypeId.identifier.numeric);
        }
        sendError(channel, msg, requestPos, &UA_TYPES[UA_TYPES_SERVICEFAULT],
                  requestId, UA_STATUSCODE_BADSERVICEUNSUPPORTED);
        return;
    }
    UA_assert(responseType);

    /* Decode the request */
    void *request = UA_alloca(requestType->memSize);
    UA_RequestHeader *requestHeader = (UA_RequestHeader*)request;
    retval = UA_decodeBinary(msg, offset, request, requestType,
                             server->config.customDataTypesSize,
                             server->config.customDataTypes);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG_CHANNEL(server->config.logger, channel,
                             "Could not decode the request");
        sendError(channel, msg, requestPos, responseType, requestId, retval);
        return;
    }

    /* Prepare the respone */
    void *response = UA_alloca(responseType->memSize);
    UA_init(response, responseType);
    UA_Session *session = NULL; /* must be initialized before goto send_response */

    /* CreateSession doesn't need a session */
    if(requestType == &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST]) {
        Service_CreateSession(server, channel,
                              (const UA_CreateSessionRequest *)request,
                              (UA_CreateSessionResponse *)response);
        goto send_response;
    }

    /* Find the matching session */
    session = UA_SecureChannel_getSession(channel, &requestHeader->authenticationToken);
    if(!session)
        session = UA_SessionManager_getSessionByToken(&server->sessionManager,
                                                      &requestHeader->authenticationToken);

    if(requestType == &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST]) {
        if(!session) {
            UA_LOG_DEBUG_CHANNEL(server->config.logger, channel,
                                 "Trying to activate a session that is " \
                                 "not known in the server");
            sendError(channel, msg, requestPos, responseType,
                      requestId, UA_STATUSCODE_BADSESSIONIDINVALID);
            UA_deleteMembers(request, requestType);
            return;
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
            UA_LOG_INFO_CHANNEL(server->config.logger, channel,
                                "Service request %i without a valid session",
                                requestType->binaryEncodingId);
            sendError(channel, msg, requestPos, responseType,
                      requestId, UA_STATUSCODE_BADSESSIONIDINVALID);
            UA_deleteMembers(request, requestType);
            return;
        }
        UA_Session_init(&anonymousSession);
        anonymousSession.sessionId = UA_NODEID_GUID(0, UA_GUID_NULL);
        anonymousSession.channel = channel;
        session = &anonymousSession;
    }

    /* Trying to use a non-activated session? */
    if(sessionRequired && !session->activated) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "Calling service %i on a non-activated session",
                            requestType->binaryEncodingId);
        sendError(channel, msg, requestPos, responseType,
                  requestId, UA_STATUSCODE_BADSESSIONNOTACTIVATED);
        UA_SessionManager_removeSession(&server->sessionManager,
                                        &session->authenticationToken);
        UA_deleteMembers(request, requestType);
        return;
    }

    /* The session is bound to another channel */
    if(session->channel != channel) {
        UA_LOG_DEBUG_CHANNEL(server->config.logger, channel,
                             "Client tries to use an obsolete securechannel");
        sendError(channel, msg, requestPos, responseType,
                  requestId, UA_STATUSCODE_BADSECURECHANNELIDINVALID);
        UA_deleteMembers(request, requestType);
        return;
    }

    /* Update the session lifetime */
    UA_Session_updateLifetime(session);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* The publish request is not answered immediately */
    if(requestType == &UA_TYPES[UA_TYPES_PUBLISHREQUEST]) {
        Service_Publish(server, session,
                        (const UA_PublishRequest*)request, requestId);
        UA_deleteMembers(request, requestType);
        return;
    }
#endif

    /* Call the service */
    UA_assert(service); /* For all services besides publish, the service pointer is non-NULL*/
    service(server, session, request, response);

 send_response:
    /* Send the response */
    ((UA_ResponseHeader*)response)->requestHandle = requestHeader->requestHandle;
    ((UA_ResponseHeader*)response)->timestamp = UA_DateTime_now();
    retval = UA_SecureChannel_sendBinaryMessage(channel, requestId, response, responseType);

    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_INFO_CHANNEL(server->config.logger, channel,
                            "Could not send the message over the SecureChannel "
                            "with StatusCode %s", UA_StatusCode_name(retval));

    /* Clean up */
    UA_deleteMembers(request, requestType);
    UA_deleteMembers(response, responseType);
}

/* ERR -> Error from the remote connection */
static void processERR(UA_Server *server, UA_Connection *connection, const UA_ByteString *msg, size_t *offset) {
    UA_TcpErrorMessage errorMessage;
    if (UA_TcpErrorMessage_decodeBinary(msg, offset, &errorMessage) != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_NETWORK,
                    "Connection %i | Could not decide the ERR message. "
                    "Closing the connection.", connection->sockfd);
        connection->close(connection);
        return;
    }

    UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_NETWORK,
                 "Connection %i | Client replied with an error message: %s %.*s",
                 connection->sockfd, UA_StatusCode_name(errorMessage.error),
                 (int)errorMessage.reason.length, errorMessage.reason.data);

    UA_TcpErrorMessage_deleteMembers(&errorMessage);
}

/* Takes decoded messages starting at the nodeid of the content type. Only OPN
 * messages start at the asymmetricalgorithmsecurityheader and are not
 * decoded. */
static void
UA_Server_processSecureChannelMessage(UA_Server *server, UA_SecureChannel *channel,
                                      UA_MessageType messagetype, UA_UInt32 requestId,
                                      const UA_ByteString *message) {
    UA_assert(channel);
    UA_assert(channel->connection);
    switch(messagetype) {
    case UA_MESSAGETYPE_ERR: {
        const UA_TcpErrorMessage *msg = (const UA_TcpErrorMessage *) message;
        UA_LOG_ERROR_CHANNEL(server->config.logger, channel,
                             "Client replied with an error message: %s %.*s",
                             UA_StatusCode_name(msg->error), (int)msg->reason.length,
                             msg->reason.data);
        break;
    }
    case UA_MESSAGETYPE_HEL:
        UA_LOG_TRACE_CHANNEL(server->config.logger, channel,
                             "Cannot process a HEL on an open channel");
        break;
    case UA_MESSAGETYPE_OPN:
        UA_LOG_TRACE_CHANNEL(server->config.logger, channel,
                             "Process an OPN on an open channel");
        processOPN(server, channel->connection, channel->securityToken.channelId, message);
        break;
    case UA_MESSAGETYPE_MSG:
        UA_LOG_TRACE_CHANNEL(server->config.logger, channel, "Process a MSG");
        processMSG(server, channel, requestId, message);
        break;
    case UA_MESSAGETYPE_CLO:
        UA_LOG_TRACE_CHANNEL(server->config.logger, channel, "Process a CLO");
        Service_CloseSecureChannel(server, channel);
        break;
    default:
        UA_LOG_TRACE_CHANNEL(server->config.logger, channel,
                             "Unknown message type");
    }
}

static void
processCompleteChunk(UA_Server *server, UA_Connection *connection,
                     UA_ByteString *message) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_SecureChannel *channel = connection->channel; 
    if(!channel) {
        /* Process chunk without a channel; must be OPN */
        UA_LOG_TRACE(server->config.logger, UA_LOGCATEGORY_NETWORK,
                     "Connection %i | No channel attached to the connection. "
                     "Process the chunk directly", connection->sockfd);
        size_t offset = 0;
        UA_TcpMessageHeader tcpMessageHeader;
        retval = UA_TcpMessageHeader_decodeBinary(message, &offset, &tcpMessageHeader);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_NETWORK,
                        "Connection %i | Could not decode the TCP message header. "
                        "Closing the connection.", connection->sockfd);
            connection->close(connection);
            return;
        }

        /* Dispatch according to the message type */
        switch(tcpMessageHeader.messageTypeAndChunkType & 0x00ffffff) {
        case UA_MESSAGETYPE_ERR:
            UA_LOG_TRACE(server->config.logger, UA_LOGCATEGORY_NETWORK,
                         "Connection %i | Process ERR message", connection->sockfd);
            processERR(server, connection, message, &offset);
            break;
        case UA_MESSAGETYPE_HEL:
            UA_LOG_TRACE(server->config.logger, UA_LOGCATEGORY_NETWORK,
                         "Connection %i | Process HEL message", connection->sockfd);
            processHEL(server, connection, message, &offset);
            break;
        case UA_MESSAGETYPE_OPN: {
            UA_LOG_TRACE(server->config.logger, UA_LOGCATEGORY_NETWORK,
                         "Connection %i | Process OPN message", connection->sockfd);
            UA_UInt32 channelId = 0;
            retval = UA_UInt32_decodeBinary(message, &offset, &channelId);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_NETWORK,
                            "Connection %i | Could not decode the channel ID for an OPN call. "
                            "Closing the connection.", connection->sockfd);
                connection->close(connection);
                break;
            }
            UA_ByteString offsetMessage;
            offsetMessage.data = message->data + 12;
            offsetMessage.length = message->length - 12;
            processOPN(server, connection, channelId, &offsetMessage);
            break; }
        case UA_MESSAGETYPE_MSG:
            UA_LOG_TRACE(server->config.logger, UA_LOGCATEGORY_NETWORK,
                         "Connection %i | Processing a MSG message not possible "
                         "without a SecureChannel", connection->sockfd);
            connection->close(connection);
            break;
        case UA_MESSAGETYPE_CLO:
            UA_LOG_TRACE(server->config.logger, UA_LOGCATEGORY_NETWORK,
                         "Connection %i | Processing a CLO message not possible "
                         "without a SecureChannel", connection->sockfd);
            connection->close(connection);
            break;
        default:
            UA_LOG_TRACE(server->config.logger, UA_LOGCATEGORY_NETWORK,
                         "Connection %i | Unknown message type", connection->sockfd);
            connection->close(connection);
        }
        return;
    }

    /* Process (and decode) chunk in the securechannel and process complete messages */
    retval = UA_SecureChannel_processChunk(channel, message, (UA_ProcessMessageCallback*)
                                           UA_Server_processSecureChannelMessage, server);
    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_TRACE_CHANNEL(server->config.logger, channel, "Procesing chunks "
                             "resulted in error code %s", UA_StatusCode_name(retval));
}

/* Takes the raw message from the network layer */
static void
processBinaryMessage(UA_Server *server, UA_Connection *connection,
                     UA_ByteString *message) {
    UA_LOG_TRACE(server->config.logger, UA_LOGCATEGORY_NETWORK,
                 "Connection %i | Received a packet.", connection->sockfd);

#ifdef UA_DEBUG_DUMP_PKGS
    UA_dump_hex_pkg(message->data, message->length);
#endif

    UA_StatusCode retval =
        UA_Connection_processChunks(connection, server,
                                    (UA_Connection_processChunk)processCompleteChunk,
                                    message);

    /* Failed to complete a chunk */
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_NETWORK,
                    "Connection %i | Failed to complete a chunk. "
                    "Closing the connection.", connection->sockfd);
        connection->close(connection);
    }
}

#ifndef UA_ENABLE_MULTITHREADING

void
UA_Server_processBinaryMessage(UA_Server *server, UA_Connection *connection,
                               UA_ByteString *message) {
    processBinaryMessage(server, connection, message);
}

#else

typedef struct {
    UA_Connection *connection;
    UA_ByteString message;
} ConnectionMessage;

static void
workerProcessBinaryMessage(UA_Server *server, ConnectionMessage *cm) {
    processBinaryMessage(server, cm->connection, &cm->message);
    UA_free(cm);
}

void
UA_Server_processBinaryMessage(UA_Server *server, UA_Connection *connection,
                               UA_ByteString *message) {
    /* Allocate the memory for the callback data */
    ConnectionMessage *cm = (ConnectionMessage*)UA_malloc(sizeof(ConnectionMessage));

    /* If malloc failed, execute immediately */
    if(!cm) {
        processBinaryMessage(server, connection, message);
        return;
    }

    /* Dispatch to the workers */
    cm->connection = connection;
    cm->message = *message;
    UA_Server_workerCallback(server, (UA_ServerCallback)workerProcessBinaryMessage, cm);
}

#endif

#ifdef UA_ENABLE_MULTITHREADING
static void
deleteConnectionTrampoline(UA_Server *server, void *data) {
    UA_Connection *connection = (UA_Connection*)data;
    connection->free(connection);
}
#endif

void
UA_Server_removeConnection(UA_Server *server, UA_Connection *connection) {
    UA_Connection_detachSecureChannel(connection);
#ifndef UA_ENABLE_MULTITHREADING
    connection->free(connection);
#else
    UA_Server_delayedCallback(server, deleteConnectionTrampoline, connection);
#endif
}
