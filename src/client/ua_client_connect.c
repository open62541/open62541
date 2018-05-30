/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2017-2018 (c) Thomas Stalder, Blue Time Concept SA
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2018 (c) Kalycito Infotech Private Limited
 */

#include "ua_client.h"
#include "ua_client_internal.h"
#include "ua_transport_generated.h"
#include "ua_transport_generated_handling.h"
#include "ua_transport_generated_encoding_binary.h"
#include "ua_types_encoding_binary.h"
#include "ua_types_generated_encoding_binary.h"

/* Size are refered in bytes */
#define UA_MINMESSAGESIZE                8192
#define UA_SESSION_LOCALNONCELENGTH      32
#define MAX_DATA_SIZE                    4096

 /********************/
 /* Set client state */
 /********************/
void
setClientState(UA_Client *client, UA_ClientState state) {
    if(client->state != state) {
        client->state = state;
        if(client->config.stateCallback)
            client->config.stateCallback(client, client->state);
    }
}

/***********************/
/* Open the Connection */
/***********************/

#define UA_BITMASK_MESSAGETYPE 0x00ffffff
#define UA_BITMASK_CHUNKTYPE 0xff000000

static UA_StatusCode
processACKResponse(void *application, UA_Connection *connection, UA_ByteString *chunk) {
    UA_Client *client = (UA_Client*)application;

    /* Decode the message */
    size_t offset = 0;
    UA_StatusCode retval;
    UA_TcpMessageHeader messageHeader;
    UA_TcpAcknowledgeMessage ackMessage;
    retval = UA_TcpMessageHeader_decodeBinary(chunk, &offset, &messageHeader);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_NETWORK,
                    "Decoding ACK message failed");
        return retval;
    }

    // check if we got an error response from the server
    UA_MessageType messageType = (UA_MessageType)
        (messageHeader.messageTypeAndChunkType & UA_BITMASK_MESSAGETYPE);
    UA_ChunkType chunkType = (UA_ChunkType)
        (messageHeader.messageTypeAndChunkType & UA_BITMASK_CHUNKTYPE);
    if (messageType == UA_MESSAGETYPE_ERR) {
        // Header + ErrorMessage (error + reasonLength_field + length)
        UA_StatusCode error = *(UA_StatusCode*)(&chunk->data[offset]);
        UA_UInt32 len = *((UA_UInt32*)&chunk->data[offset + 4]);
        UA_Byte *data = (UA_Byte*)&chunk->data[offset + 4+4];
        UA_LOG_ERROR(client->config.logger, UA_LOGCATEGORY_NETWORK,
                    "Received ERR response. %s - %.*s", UA_StatusCode_name(error), len, data);
        return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
    }
    if (chunkType != UA_CHUNKTYPE_FINAL) {
        return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
    }

    retval |= UA_TcpAcknowledgeMessage_decodeBinary(chunk, &offset, &ackMessage);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_NETWORK,
                    "Decoding ACK message failed");
        return retval;
    }

    /* Store remote connection settings and adjust local configuration to not
     * exceed the limits */
    UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_NETWORK, "Received ACK message");
    connection->remoteConf.maxChunkCount = ackMessage.maxChunkCount; /* may be zero -> unlimited */
    connection->remoteConf.maxMessageSize = ackMessage.maxMessageSize; /* may be zero -> unlimited */
    connection->remoteConf.protocolVersion = ackMessage.protocolVersion;
    connection->remoteConf.sendBufferSize = ackMessage.sendBufferSize;
    connection->remoteConf.recvBufferSize = ackMessage.receiveBufferSize;
    if(connection->remoteConf.recvBufferSize < connection->localConf.sendBufferSize)
        connection->localConf.sendBufferSize = connection->remoteConf.recvBufferSize;
    if(connection->remoteConf.sendBufferSize < connection->localConf.recvBufferSize)
        connection->localConf.recvBufferSize = connection->remoteConf.sendBufferSize;
    connection->state = UA_CONNECTION_ESTABLISHED;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
HelAckHandshake(UA_Client *client) {
    /* Get a buffer */
    UA_ByteString message;
    UA_Connection *conn = &client->connection;
    UA_StatusCode retval = conn->getSendBuffer(conn, UA_MINMESSAGESIZE, &message);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Prepare the HEL message and encode at offset 8 */
    UA_TcpHelloMessage hello;
    UA_String_copy(&client->endpointUrl, &hello.endpointUrl); /* must be less than 4096 bytes */
    hello.maxChunkCount = conn->localConf.maxChunkCount;
    hello.maxMessageSize = conn->localConf.maxMessageSize;
    hello.protocolVersion = conn->localConf.protocolVersion;
    hello.receiveBufferSize = conn->localConf.recvBufferSize;
    hello.sendBufferSize = conn->localConf.sendBufferSize;

    UA_Byte *bufPos = &message.data[8]; /* skip the header */
    const UA_Byte *bufEnd = &message.data[message.length];
    retval = UA_TcpHelloMessage_encodeBinary(&hello, &bufPos, bufEnd);
    UA_TcpHelloMessage_deleteMembers(&hello);

    /* Encode the message header at offset 0 */
    UA_TcpMessageHeader messageHeader;
    messageHeader.messageTypeAndChunkType = UA_CHUNKTYPE_FINAL + UA_MESSAGETYPE_HEL;
    messageHeader.messageSize = (UA_UInt32)((uintptr_t)bufPos - (uintptr_t)message.data);
    bufPos = message.data;
    retval |= UA_TcpMessageHeader_encodeBinary(&messageHeader, &bufPos, bufEnd);
    if(retval != UA_STATUSCODE_GOOD) {
        conn->releaseSendBuffer(conn, &message);
        return retval;
    }

    /* Send the HEL message */
    message.length = messageHeader.messageSize;
    retval = conn->send(conn, &message);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_NETWORK,
                    "Sending HEL failed");
        return retval;
    }
    UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_NETWORK,
                 "Sent HEL message");

    /* Loop until we have a complete chunk */
    retval = UA_Connection_receiveChunksBlocking(conn, client, processACKResponse,
                                                 client->config.timeout);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_NETWORK,
                    "Receiving ACK message failed");
        if(retval == UA_STATUSCODE_BADCONNECTIONCLOSED)
            client->state = UA_CLIENTSTATE_DISCONNECTED;
        UA_Client_close(client);
    }
    return retval;
}

static void
processDecodedOPNResponse(UA_Client *client, UA_OpenSecureChannelResponse *response, UA_Boolean renew) {
    /* Replace the token */
    if (renew)
        client->channel.nextSecurityToken = response->securityToken; // Set the next token
    else
        client->channel.securityToken = response->securityToken; // Set initial token

    /* Replace the nonce */
    UA_ByteString_deleteMembers(&client->channel.remoteNonce);
    client->channel.remoteNonce = response->serverNonce;
    UA_ByteString_init(&response->serverNonce);

    if(client->channel.state == UA_SECURECHANNELSTATE_OPEN)
        UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "SecureChannel in the server renewed");
    else
        UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Opened SecureChannel acknowledged by the server");

    /* Response.securityToken.revisedLifetime is UInt32 we need to cast it to
     * DateTime=Int64 we take 75% of lifetime to start renewing as described in
     * standard */
    client->channel.state = UA_SECURECHANNELSTATE_OPEN;
    client->nextChannelRenewal = UA_DateTime_nowMonotonic() + (UA_DateTime)
        (client->channel.securityToken.revisedLifetime * (UA_Double)UA_DATETIME_MSEC * 0.75);
}

static UA_StatusCode
openSecureChannel(UA_Client *client, UA_Boolean renew) {
    /* Check if sc is still valid */
    if(renew && client->nextChannelRenewal > UA_DateTime_nowMonotonic())
        return UA_STATUSCODE_GOOD;

    UA_Connection *conn = &client->connection;
    if(conn->state != UA_CONNECTION_ESTABLISHED)
        return UA_STATUSCODE_BADSERVERNOTCONNECTED;

    /* Prepare the OpenSecureChannelRequest */
    UA_OpenSecureChannelRequest opnSecRq;
    UA_OpenSecureChannelRequest_init(&opnSecRq);
    opnSecRq.requestHeader.timestamp = UA_DateTime_now();
    opnSecRq.requestHeader.authenticationToken = client->authenticationToken;
    if(renew) {
        opnSecRq.requestType = UA_SECURITYTOKENREQUESTTYPE_RENEW;
        UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Requesting to renew the SecureChannel");
    } else {
        opnSecRq.requestType = UA_SECURITYTOKENREQUESTTYPE_ISSUE;
        UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Requesting to open a SecureChannel");
    }

    /* Set the securityMode to input securityMode from client data */
    opnSecRq.securityMode = client->channel.securityMode;

    opnSecRq.clientNonce = client->channel.localNonce;
    opnSecRq.requestedLifetime = client->config.secureChannelLifeTime;

    /* Send the OPN message */
    UA_UInt32 requestId = ++client->requestId;
    UA_StatusCode retval =
        UA_SecureChannel_sendAsymmetricOPNMessage(&client->channel, requestId, &opnSecRq,
                                                  &UA_TYPES[UA_TYPES_OPENSECURECHANNELREQUEST]);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Sending OPN message failed with error %s", UA_StatusCode_name(retval));
        UA_Client_close(client);
        return retval;
    }

    UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL, "OPN message sent");

    /* Increase nextChannelRenewal to avoid that we re-start renewal when
     * publish responses are received before the OPN response arrives. */
    client->nextChannelRenewal = UA_DateTime_nowMonotonic() +
        (2 * ((UA_DateTime)client->config.timeout * UA_DATETIME_MSEC));

    /* Receive / decrypt / decode the OPN response. Process async services in
     * the background until the OPN response arrives. */
    UA_OpenSecureChannelResponse response;
    retval = receiveServiceResponse(client, &response,
                                    &UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE],
                                    UA_DateTime_nowMonotonic() +
                                    ((UA_DateTime)client->config.timeout * UA_DATETIME_MSEC),
                                    &requestId);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_close(client);
        return retval;
    }

    processDecodedOPNResponse(client, &response, renew);
    UA_OpenSecureChannelResponse_deleteMembers(&response);
    return retval;
}

/* Function to verify the signature corresponds to ClientNonce
 * using the local certificate.
 *
 * @param  channel      current channel in which the client runs
 * @param  response     create session response from the server
 * @return Returns an error code or UA_STATUSCODE_GOOD. */
UA_StatusCode
checkClientSignature(const UA_SecureChannel *channel, const UA_CreateSessionResponse *response) {
    if(channel == NULL || response == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGN &&
       channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return UA_STATUSCODE_GOOD;

    if(!channel->securityPolicy)
        return UA_STATUSCODE_BADINTERNALERROR;

    const UA_SecurityPolicy* securityPolicy   = channel->securityPolicy;
    const UA_ByteString*     localCertificate = &securityPolicy->localCertificate;

    size_t dataToVerifySize = localCertificate->length + channel->localNonce.length;
    UA_ByteString dataToVerify = UA_BYTESTRING_NULL;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&dataToVerify, dataToVerifySize);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    memcpy(dataToVerify.data, localCertificate->data, localCertificate->length);
    memcpy(dataToVerify.data + localCertificate->length,
           channel->localNonce.data, channel->localNonce.length);

    retval = securityPolicy->
             certificateSigningAlgorithm.verify(securityPolicy,
                                                channel->channelContext,
                                                &dataToVerify,
                                                &response->serverSignature.signature);
    UA_ByteString_deleteMembers(&dataToVerify);
    return retval;
}

/* Function to create a signature using remote certificate and nonce
 *
 * @param  channel      current channel in which the client runs
 * @param  request      activate session request message to server
 * @return Returns an error or UA_STATUSCODE_GOOD */
UA_StatusCode
signActivateSessionRequest(UA_SecureChannel *channel,
                           UA_ActivateSessionRequest *request) {
    if(channel == NULL || request == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGN &&
       channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return UA_STATUSCODE_GOOD;

    const UA_SecurityPolicy *const securityPolicy = channel->securityPolicy;
    UA_SignatureData *signatureData = &request->clientSignature;

    /* Prepare the signature */
    size_t signatureSize = securityPolicy->certificateSigningAlgorithm.
                           getLocalSignatureSize(securityPolicy, channel->channelContext);
    UA_StatusCode retval = UA_String_copy(&securityPolicy->certificateSigningAlgorithm.uri,
                                          &signatureData->algorithm);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = UA_ByteString_allocBuffer(&signatureData->signature, signatureSize);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Allocate a temporary buffer */
    size_t dataToSignSize = channel->remoteCertificate.length + channel->remoteNonce.length;
    /* Prevent stack-smashing. TODO: Compute MaxSenderCertificateSize */
    if(dataToSignSize > MAX_DATA_SIZE)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString dataToSign;
    retval = UA_ByteString_allocBuffer(&dataToSign, dataToSignSize);
    if(retval != UA_STATUSCODE_GOOD)
        return retval; /* signatureData->signature is cleaned up with the response */

    /* Sign the signature */
    memcpy(dataToSign.data, channel->remoteCertificate.data, channel->remoteCertificate.length);
    memcpy(dataToSign.data + channel->remoteCertificate.length,
           channel->remoteNonce.data, channel->remoteNonce.length);
    retval = securityPolicy->certificateSigningAlgorithm.
             sign(securityPolicy, channel->channelContext, &dataToSign,
                  &signatureData->signature);

    /* Clean up */
    UA_ByteString_deleteMembers(&dataToSign);
    return retval;
}

static UA_StatusCode
activateSession(UA_Client *client) {
    UA_ActivateSessionRequest request;
    UA_ActivateSessionRequest_init(&request);
    request.requestHeader.requestHandle = ++client->requestHandle;
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 600000;

    //manual ExtensionObject encoding of the identityToken
    if(client->authenticationMethod == UA_CLIENTAUTHENTICATION_NONE) {
        UA_AnonymousIdentityToken* identityToken = UA_AnonymousIdentityToken_new();
        UA_AnonymousIdentityToken_init(identityToken);
        UA_String_copy(&client->token.policyId, &identityToken->policyId);
        request.userIdentityToken.encoding = UA_EXTENSIONOBJECT_DECODED;
        request.userIdentityToken.content.decoded.type = &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN];
        request.userIdentityToken.content.decoded.data = identityToken;
    } else {
        UA_UserNameIdentityToken* identityToken = UA_UserNameIdentityToken_new();
        UA_UserNameIdentityToken_init(identityToken);
        UA_String_copy(&client->token.policyId, &identityToken->policyId);
        UA_String_copy(&client->username, &identityToken->userName);
        UA_String_copy(&client->password, &identityToken->password);
        request.userIdentityToken.encoding = UA_EXTENSIONOBJECT_DECODED;
        request.userIdentityToken.content.decoded.type = &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN];
        request.userIdentityToken.content.decoded.data = identityToken;
    }

    /* This function call is to prepare a client signature */
    if(client->channel.securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       client->channel.securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        signActivateSessionRequest(&client->channel, &request);
    }

    UA_ActivateSessionResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST],
                        &response, &UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE]);

    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "ActivateSession failed with error code %s",
                     UA_StatusCode_name(response.responseHeader.serviceResult));
    }

    UA_StatusCode retval = response.responseHeader.serviceResult;
    UA_ActivateSessionRequest_deleteMembers(&request);
    UA_ActivateSessionResponse_deleteMembers(&response);
    return retval;
}

/* Gets a list of endpoints. Memory is allocated for endpointDescription array */
UA_StatusCode
UA_Client_getEndpointsInternal(UA_Client *client, size_t* endpointDescriptionsSize,
                               UA_EndpointDescription** endpointDescriptions) {
    UA_GetEndpointsRequest request;
    UA_GetEndpointsRequest_init(&request);
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    // assume the endpointurl outlives the service call
    request.endpointUrl = client->endpointUrl;

    UA_GetEndpointsResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_GETENDPOINTSREQUEST],
                        &response, &UA_TYPES[UA_TYPES_GETENDPOINTSRESPONSE]);

    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_StatusCode retval = response.responseHeader.serviceResult;
        UA_LOG_ERROR(client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "GetEndpointRequest failed with error code %s",
                     UA_StatusCode_name(retval));
        UA_GetEndpointsResponse_deleteMembers(&response);
        return retval;
    }
    *endpointDescriptions = response.endpoints;
    *endpointDescriptionsSize = response.endpointsSize;
    response.endpoints = NULL;
    response.endpointsSize = 0;
    UA_GetEndpointsResponse_deleteMembers(&response);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
getEndpoints(UA_Client *client) {
    UA_EndpointDescription* endpointArray = NULL;
    size_t endpointArraySize = 0;
    UA_StatusCode retval =
        UA_Client_getEndpointsInternal(client, &endpointArraySize, &endpointArray);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_Boolean endpointFound = false;
    UA_Boolean tokenFound = false;
    UA_String securityNone = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None");
    UA_String binaryTransport = UA_STRING("http://opcfoundation.org/UA-Profile/"
                                          "Transport/uatcp-uasc-uabinary");

    // TODO: compare endpoint information with client->endpointUri
    for(size_t i = 0; i < endpointArraySize; ++i) {
        UA_EndpointDescription* endpoint = &endpointArray[i];
        /* look out for binary transport endpoints */
        /* Note: Siemens returns empty ProfileUrl, we will accept it as binary */
        if(endpoint->transportProfileUri.length != 0 &&
           !UA_String_equal(&endpoint->transportProfileUri, &binaryTransport))
            continue;

        /* look for an endpoint corresponding to the client security policy */
        if(!UA_String_equal(&endpoint->securityPolicyUri, &client->securityPolicy.policyUri))
            continue;

        endpointFound = true;

        /* look for a user token policy with an anonymous token */
        for(size_t j = 0; j < endpoint->userIdentityTokensSize; ++j) {
            UA_UserTokenPolicy* userToken = &endpoint->userIdentityTokens[j];

            /* Usertokens also have a security policy... */
            if(userToken->securityPolicyUri.length > 0 &&
               !UA_String_equal(&userToken->securityPolicyUri, &securityNone))
                continue;

            /* UA_CLIENTAUTHENTICATION_NONE == UA_USERTOKENTYPE_ANONYMOUS
             * UA_CLIENTAUTHENTICATION_USERNAME == UA_USERTOKENTYPE_USERNAME
             * TODO: Check equivalence for other types when adding the support */
            if((int)client->authenticationMethod != (int)userToken->tokenType)
                continue;

            /* Endpoint with matching usertokenpolicy found */
            tokenFound = true;
            UA_UserTokenPolicy_deleteMembers(&client->token);
            UA_UserTokenPolicy_copy(userToken, &client->token);
            break;
        }
    }

    UA_Array_delete(endpointArray, endpointArraySize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);

    if(!endpointFound) {
        UA_LOG_ERROR(client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "No suitable endpoint found");
        retval = UA_STATUSCODE_BADINTERNALERROR;
    } else if(!tokenFound) {
        UA_LOG_ERROR(client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "No suitable UserTokenPolicy found for the possible endpoints");
        retval = UA_STATUSCODE_BADINTERNALERROR;
    }
    return retval;
}

static UA_StatusCode
createSession(UA_Client *client) {
    UA_CreateSessionRequest request;
    UA_CreateSessionRequest_init(&request);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    if(client->channel.securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       client->channel.securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        if(client->channel.localNonce.length != UA_SESSION_LOCALNONCELENGTH) {
           UA_ByteString_deleteMembers(&client->channel.localNonce);
            retval = UA_ByteString_allocBuffer(&client->channel.localNonce,
                                               UA_SESSION_LOCALNONCELENGTH);
            if(retval != UA_STATUSCODE_GOOD)
               return retval;
        }

        retval = client->channel.securityPolicy->symmetricModule.
                 generateNonce(client->channel.securityPolicy, &client->channel.localNonce);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    UA_ByteString_copy(&client->channel.localNonce, &request.clientNonce);
    request.requestedSessionTimeout = 1200000;
    request.maxResponseMessageSize = UA_INT32_MAX;
    UA_String_copy(&client->endpointUrl, &request.endpointUrl);

    if(client->channel.securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       client->channel.securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        UA_ByteString_copy(&client->channel.securityPolicy->localCertificate,
                           &request.clientCertificate);
    }

    UA_CreateSessionResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
                        &response, &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE]);

    if(response.responseHeader.serviceResult == UA_STATUSCODE_GOOD &&
        (client->channel.securityMode == UA_MESSAGESECURITYMODE_SIGN ||
         client->channel.securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)) {
        UA_ByteString_deleteMembers(&client->channel.remoteNonce);
        UA_ByteString_copy(&response.serverNonce, &client->channel.remoteNonce);

        if(!UA_ByteString_equal(&response.serverCertificate,
                                &client->channel.remoteCertificate)) {
            return UA_STATUSCODE_BADCERTIFICATEINVALID;
        }

        /* Verify the client signature */
        retval = checkClientSignature(&client->channel, &response);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    UA_NodeId_copy(&response.authenticationToken, &client->authenticationToken);

    retval = response.responseHeader.serviceResult;
    UA_CreateSessionRequest_deleteMembers(&request);
    UA_CreateSessionResponse_deleteMembers(&response);
    return retval;
}

UA_StatusCode
UA_Client_connectInternal(UA_Client *client, const char *endpointUrl,
                          UA_Boolean endpointsHandshake, UA_Boolean createNewSession) {
    if(client->state >= UA_CLIENTSTATE_CONNECTED)
        return UA_STATUSCODE_GOOD;
    UA_ChannelSecurityToken_init(&client->channel.securityToken);
    client->channel.state = UA_SECURECHANNELSTATE_FRESH;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    client->connection =
        client->config.connectionFunc(client->config.localConnectionConfig,
                                      endpointUrl, client->config.timeout,
                                      client->config.logger);
    if(client->connection.state != UA_CONNECTION_OPENING) {
        retval = UA_STATUSCODE_BADCONNECTIONCLOSED;
        goto cleanup;
    }

    UA_String_deleteMembers(&client->endpointUrl);
    client->endpointUrl = UA_STRING_ALLOC(endpointUrl);
    if(!client->endpointUrl.data) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }

    /* Local nonce set and generate sym keys */
    retval |= UA_SecureChannel_generateLocalNonce(&client->channel);

    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Open a TCP connection */
    client->connection.localConf = client->config.localConnectionConfig;
    retval = HelAckHandshake(client);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;
    setClientState(client, UA_CLIENTSTATE_CONNECTED);

    /* Open a SecureChannel. TODO: Select with endpoint  */
    client->channel.connection = &client->connection;
    retval = openSecureChannel(client, false);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;
    setClientState(client, UA_CLIENTSTATE_SECURECHANNEL);

    /* Delete async service. TODO: Move this from connect to the disconnect/cleanup phase */
    UA_Client_AsyncService_removeAll(client, UA_STATUSCODE_BADSHUTDOWN);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    client->currentlyOutStandingPublishRequests = 0;
#endif

// TODO: actually, reactivate an existing session is working, but currently republish is not implemented
// This option is disabled until we have a good implementation of the subscription recovery.

#ifdef UA_SESSION_RECOVERY
    /* Try to activate an existing Session for this SecureChannel */
    if((!UA_NodeId_equal(&client->authenticationToken, &UA_NODEID_NULL)) && (createNewSession)) {
        retval = activateSession(client);
        if(retval == UA_STATUSCODE_BADSESSIONIDINVALID) {
            /* Could not recover an old session. Remove authenticationToken */
            UA_NodeId_deleteMembers(&client->authenticationToken);
        } else {
            if(retval != UA_STATUSCODE_GOOD)
                goto cleanup;
            setClientState(client, UA_CLIENTSTATE_SESSION_RENEWED);
            return retval;
        }
    } else {
        UA_NodeId_deleteMembers(&client->authenticationToken);
    }
#else
    UA_NodeId_deleteMembers(&client->authenticationToken);
#endif /* UA_SESSION_RECOVERY */

    /* Generate new local and remote key */
    retval |= UA_SecureChannel_generateNewKeys(&client->channel);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Get Endpoints */
    if(endpointsHandshake) {
        retval = getEndpoints(client);
        if(retval != UA_STATUSCODE_GOOD)
            goto cleanup;
    }

    /* Create the Session for this SecureChannel */
    if(createNewSession) {
        retval = createSession(client);
        if(retval != UA_STATUSCODE_GOOD)
            goto cleanup;
#ifdef UA_ENABLE_SUBSCRIPTIONS
        /* A new session has been created. We need to clean up the subscriptions */
        UA_Client_Subscriptions_clean(client);
#endif
        retval = activateSession(client);
        if(retval != UA_STATUSCODE_GOOD)
            goto cleanup;
        setClientState(client, UA_CLIENTSTATE_SESSION);
    }

    return retval;

cleanup:
    UA_Client_close(client);
    return retval;
}

UA_StatusCode
UA_Client_connect(UA_Client *client, const char *endpointUrl) {
    return UA_Client_connectInternal(client, endpointUrl, UA_TRUE, UA_TRUE);
}

UA_StatusCode
UA_Client_connect_noSession(UA_Client *client, const char *endpointUrl) {
    return UA_Client_connectInternal(client, endpointUrl, UA_TRUE, UA_FALSE);
}

UA_StatusCode
UA_Client_connect_username(UA_Client *client, const char *endpointUrl,
                           const char *username, const char *password) {
    client->authenticationMethod = UA_CLIENTAUTHENTICATION_USERNAME;
    client->username = UA_STRING_ALLOC(username);
    client->password = UA_STRING_ALLOC(password);
    return UA_Client_connect(client, endpointUrl);
}

UA_StatusCode
UA_Client_manuallyRenewSecureChannel(UA_Client *client) {
    UA_StatusCode retval = openSecureChannel(client, true);
    if(retval != UA_STATUSCODE_GOOD)
        UA_Client_close(client);

    return retval;
}

/************************/
/* Close the Connection */
/************************/

static void
sendCloseSession(UA_Client *client) {
    UA_CloseSessionRequest request;
    UA_CloseSessionRequest_init(&request);

    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    request.deleteSubscriptions = true;
    UA_CloseSessionResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST],
                        &response, &UA_TYPES[UA_TYPES_CLOSESESSIONRESPONSE]);
    UA_CloseSessionRequest_deleteMembers(&request);
    UA_CloseSessionResponse_deleteMembers(&response);
}

static void
sendCloseSecureChannel(UA_Client *client) {
    UA_SecureChannel *channel = &client->channel;
    UA_CloseSecureChannelRequest request;
    UA_CloseSecureChannelRequest_init(&request);
    request.requestHeader.requestHandle = ++client->requestHandle;
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    request.requestHeader.authenticationToken = client->authenticationToken;
    UA_SecureChannel_sendSymmetricMessage(channel, ++client->requestId,
                                          UA_MESSAGETYPE_CLO, &request,
                                          &UA_TYPES[UA_TYPES_CLOSESECURECHANNELREQUEST]);
    UA_CloseSecureChannelRequest_deleteMembers(&request);
    UA_SecureChannel_deleteMembersCleanup(&client->channel);
}

UA_StatusCode
UA_Client_disconnect(UA_Client *client) {
    /* Is a session established? */
    if(client->state >= UA_CLIENTSTATE_SESSION) {
        client->state = UA_CLIENTSTATE_SECURECHANNEL;
        sendCloseSession(client);
    }
    UA_NodeId_deleteMembers(&client->authenticationToken);
    client->requestHandle = 0;

    /* Is a secure channel established? */
    if(client->state >= UA_CLIENTSTATE_SECURECHANNEL) {
        client->state = UA_CLIENTSTATE_CONNECTED;
        sendCloseSecureChannel(client);
    }

    /* Close the TCP connection */
    if(client->connection.state != UA_CONNECTION_CLOSED
            && client->connection.state != UA_CONNECTION_OPENING)
        /*UA_ClientConnectionTCP_init sets initial state to opening */
        if(client->connection.close != NULL)
            client->connection.close(&client->connection);


#ifdef UA_ENABLE_SUBSCRIPTIONS
// TODO REMOVE WHEN UA_SESSION_RECOVERY IS READY
    /* We need to clean up the subscriptions */
    UA_Client_Subscriptions_clean(client);
#endif

    setClientState(client, UA_CLIENTSTATE_DISCONNECTED);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Client_close(UA_Client *client) {
    client->requestHandle = 0;

    if(client->state >= UA_CLIENTSTATE_SECURECHANNEL)
        UA_SecureChannel_deleteMembersCleanup(&client->channel);

    /* Close the TCP connection */
    if(client->connection.state != UA_CONNECTION_CLOSED)
        client->connection.close(&client->connection);

#ifdef UA_ENABLE_SUBSCRIPTIONS
// TODO REMOVE WHEN UA_SESSION_RECOVERY IS READY
        /* We need to clean up the subscriptions */
        UA_Client_Subscriptions_clean(client);
#endif

    setClientState(client, UA_CLIENTSTATE_DISCONNECTED);
    return UA_STATUSCODE_GOOD;
}
