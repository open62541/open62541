/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2017-2018 (c) Thomas Stalder, Blue Time Concept SA
 *    Copyright 2017-2019 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2018 (c) Kalycito Infotech Private Limited
 */

#include <open62541/transport_generated.h>
#include <open62541/transport_generated_encoding_binary.h>
#include <open62541/transport_generated_handling.h>
#include <open62541/types_generated_encoding_binary.h>

#include "ua_client_internal.h"

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

#define UA_BITMASK_MESSAGETYPE 0x00ffffffu
#define UA_BITMASK_CHUNKTYPE 0xff000000u

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
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_NETWORK,
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
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_NETWORK,
                    "Received ERR response. %s - %.*s", UA_StatusCode_name(error), len, data);
        return error;
    }
    if (chunkType != UA_CHUNKTYPE_FINAL) {
        return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
    }

    /* Decode the ACK message */
    retval = UA_TcpAcknowledgeMessage_decodeBinary(chunk, &offset, &ackMessage);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_NETWORK,
                     "Decoding ACK message failed");
        return retval;
    }
    UA_LOG_DEBUG(&client->config.logger, UA_LOGCATEGORY_NETWORK, "Received ACK message");

    /* Process the ACK message */
    return UA_Connection_processHELACK(connection, &client->config.localConnectionConfig,
                                       (const UA_ConnectionConfig*)&ackMessage);
}

static UA_StatusCode
HelAckHandshake(UA_Client *client, const UA_String endpointUrl) {
    /* Get a buffer */
    UA_ByteString message;
    UA_Connection *conn = &client->connection;
    UA_StatusCode retval = conn->getSendBuffer(conn, UA_MINMESSAGESIZE, &message);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Prepare the HEL message and encode at offset 8 */
    UA_TcpHelloMessage hello;
    /* just reference to avoid copy */
    hello.endpointUrl = endpointUrl;
    memcpy(&hello, &client->config.localConnectionConfig,
           sizeof(UA_ConnectionConfig)); /* same struct layout */

    UA_Byte *bufPos = &message.data[8]; /* skip the header */
    const UA_Byte *bufEnd = &message.data[message.length];
    retval = UA_TcpHelloMessage_encodeBinary(&hello, &bufPos, bufEnd);
    /* avoid deleting reference */
    hello.endpointUrl = UA_STRING_NULL;
    UA_TcpHelloMessage_deleteMembers(&hello);
    if(retval != UA_STATUSCODE_GOOD) {
        conn->releaseSendBuffer(conn, &message);
        return retval;
    }

    /* Encode the message header at offset 0 */
    UA_TcpMessageHeader messageHeader;
    messageHeader.messageTypeAndChunkType = UA_CHUNKTYPE_FINAL + UA_MESSAGETYPE_HEL;
    messageHeader.messageSize = (UA_UInt32)((uintptr_t)bufPos - (uintptr_t)message.data);
    bufPos = message.data;
    retval = UA_TcpMessageHeader_encodeBinary(&messageHeader, &bufPos, bufEnd);
    if(retval != UA_STATUSCODE_GOOD) {
        conn->releaseSendBuffer(conn, &message);
        return retval;
    }

    /* Send the HEL message */
    message.length = messageHeader.messageSize;
    retval = conn->send(conn, &message);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_NETWORK,
                     "Sending HEL failed");
        return retval;
    }
    UA_LOG_DEBUG(&client->config.logger, UA_LOGCATEGORY_NETWORK,
                 "Sent HEL message");

    /* Loop until we have a complete chunk */
    retval = UA_Connection_receiveChunksBlocking(conn, client, processACKResponse,
                                                 client->config.timeout);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_NETWORK,
                     "Receiving ACK message failed with %s", UA_StatusCode_name(retval));
        if(retval == UA_STATUSCODE_BADCONNECTIONCLOSED)
            client->state = UA_CLIENTSTATE_DISCONNECTED;
        UA_Client_disconnect(client);
    }
    return retval;
}

UA_SecurityPolicy *
getSecurityPolicy(UA_Client *client, UA_String policyUri) {
    for(size_t i = 0; i < client->config.securityPoliciesSize; i++) {
        if(UA_String_equal(&policyUri, &client->config.securityPolicies[i].policyUri))
            return &client->config.securityPolicies[i];
    }
    return NULL;
}

static void
processDecodedOPNResponse(UA_Client *client, UA_OpenSecureChannelResponse *response,
                          UA_Boolean renew) {
    /* Replace the token */
    if(renew)
        client->channel.nextSecurityToken = response->securityToken;
    else
        client->channel.securityToken = response->securityToken;

    /* Replace the nonce */
    UA_ByteString_deleteMembers(&client->channel.remoteNonce);
    client->channel.remoteNonce = response->serverNonce;
    UA_ByteString_init(&response->serverNonce);

    if(client->channel.state == UA_SECURECHANNELSTATE_OPEN)
        UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "SecureChannel renewed");
    else
        UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "Opened SecureChannel with SecurityPolicy %.*s",
                    (int)client->channel.securityPolicy->policyUri.length,
                    client->channel.securityPolicy->policyUri.data);

    /* Response.securityToken.revisedLifetime is UInt32 we need to cast it to
     * DateTime=Int64 we take 75% of lifetime to start renewing as described in
     * standard */
    client->channel.state = UA_SECURECHANNELSTATE_OPEN;
    client->nextChannelRenewal = UA_DateTime_nowMonotonic() + (UA_DateTime)
        (client->channel.securityToken.revisedLifetime * (UA_Double)UA_DATETIME_MSEC * 0.75);
}

UA_StatusCode
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
        UA_LOG_DEBUG(&client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Requesting to renew the SecureChannel");
    } else {
        opnSecRq.requestType = UA_SECURITYTOKENREQUESTTYPE_ISSUE;
        UA_LOG_DEBUG(&client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
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
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Sending OPN message failed with error %s", UA_StatusCode_name(retval));
        UA_Client_disconnect(client);
        return retval;
    }

    UA_LOG_DEBUG(&client->config.logger, UA_LOGCATEGORY_SECURECHANNEL, "OPN message sent");

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
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Receiving service response failed with error %s", UA_StatusCode_name(retval));
        UA_Client_disconnect(client);
        return retval;
    }

    processDecodedOPNResponse(client, &response, renew);
    UA_OpenSecureChannelResponse_deleteMembers(&response);
    return retval;
}

/* Function to verify the signature corresponds to ClientNonce
 * using the local certificate */
static UA_StatusCode
checkClientSignature(const UA_SecureChannel *channel,
                     const UA_CreateSessionResponse *response) {
    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGN &&
       channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return UA_STATUSCODE_GOOD;

    if(!channel->securityPolicy)
        return UA_STATUSCODE_BADINTERNALERROR;

    const UA_SecurityPolicy *sp = channel->securityPolicy;
    const UA_ByteString *lc = &sp->localCertificate;

    size_t dataToVerifySize = lc->length + channel->localNonce.length;
    UA_ByteString dataToVerify = UA_BYTESTRING_NULL;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&dataToVerify, dataToVerifySize);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    memcpy(dataToVerify.data, lc->data, lc->length);
    memcpy(dataToVerify.data + lc->length,
           channel->localNonce.data, channel->localNonce.length);

    retval = sp->certificateSigningAlgorithm.
        verify(sp, channel->channelContext, &dataToVerify,
               &response->serverSignature.signature);
    UA_ByteString_deleteMembers(&dataToVerify);
    return retval;
}

/* Function to create a signature using remote certificate and nonce */
#ifdef UA_ENABLE_ENCRYPTION
UA_StatusCode
signActivateSessionRequest(UA_SecureChannel *channel,
                           UA_ActivateSessionRequest *request) {
    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGN &&
       channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return UA_STATUSCODE_GOOD;

    const UA_SecurityPolicy *sp = channel->securityPolicy;
    UA_SignatureData *sd = &request->clientSignature;

    /* Prepare the signature */
    size_t signatureSize = sp->certificateSigningAlgorithm.
        getLocalSignatureSize(sp, channel->channelContext);
    UA_StatusCode retval = UA_String_copy(&sp->certificateSigningAlgorithm.uri,
                                          &sd->algorithm);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = UA_ByteString_allocBuffer(&sd->signature, signatureSize);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Allocate a temporary buffer */
    size_t dataToSignSize = channel->remoteCertificate.length + channel->remoteNonce.length;
    if(dataToSignSize > MAX_DATA_SIZE)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString dataToSign;
    retval = UA_ByteString_allocBuffer(&dataToSign, dataToSignSize);
    if(retval != UA_STATUSCODE_GOOD)
        return retval; /* sd->signature is cleaned up with the response */

    /* Sign the signature */
    memcpy(dataToSign.data, channel->remoteCertificate.data,
           channel->remoteCertificate.length);
    memcpy(dataToSign.data + channel->remoteCertificate.length,
           channel->remoteNonce.data, channel->remoteNonce.length);
    retval = sp->certificateSigningAlgorithm.sign(sp, channel->channelContext,
                                                  &dataToSign, &sd->signature);

    /* Clean up */
    UA_ByteString_deleteMembers(&dataToSign);
    return retval;
}

UA_StatusCode
encryptUserIdentityToken(UA_Client *client, const UA_String *userTokenSecurityPolicy,
                         UA_ExtensionObject *userIdentityToken) {
    UA_IssuedIdentityToken *iit = NULL;
    UA_UserNameIdentityToken *unit = NULL;
    UA_ByteString *tokenData;
    if(userIdentityToken->content.decoded.type == &UA_TYPES[UA_TYPES_ISSUEDIDENTITYTOKEN]) {
        iit = (UA_IssuedIdentityToken*)userIdentityToken->content.decoded.data;
        tokenData = &iit->tokenData;
    } else if(userIdentityToken->content.decoded.type == &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN]) {
        unit = (UA_UserNameIdentityToken*)userIdentityToken->content.decoded.data;
        tokenData = &unit->password;
    } else {
        return UA_STATUSCODE_GOOD;
    }

    /* No encryption */
    const UA_String none = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None");
    if(userTokenSecurityPolicy->length == 0 ||
       UA_String_equal(userTokenSecurityPolicy, &none)) {
        return UA_STATUSCODE_GOOD;
    }

    UA_SecurityPolicy *sp = getSecurityPolicy(client, *userTokenSecurityPolicy);
    if(!sp) {
        UA_LOG_WARNING(&client->config.logger, UA_LOGCATEGORY_NETWORK,
                       "Could not find the required SecurityPolicy for the UserToken");
        return UA_STATUSCODE_BADSECURITYPOLICYREJECTED;
    }

    /* Create a temp channel context */

    void *channelContext;
    UA_StatusCode retval = sp->channelModule.
        newContext(sp, &client->config.endpoint.serverCertificate, &channelContext);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(&client->config.logger, UA_LOGCATEGORY_NETWORK,
                       "Could not instantiate the SecurityPolicy for the UserToken");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    
    /* Compute the encrypted length (at least one byte padding) */
    size_t plainTextBlockSize = sp->asymmetricModule.cryptoModule.
        encryptionAlgorithm.getRemotePlainTextBlockSize(sp, channelContext);
    UA_UInt32 length = (UA_UInt32)(tokenData->length + client->channel.remoteNonce.length);
    UA_UInt32 totalLength = length + 4; /* Including the length field */
    size_t blocks = totalLength / plainTextBlockSize;
    if(totalLength  % plainTextBlockSize != 0)
        blocks++;
    size_t overHead =
        UA_SecurityPolicy_getRemoteAsymEncryptionBufferLengthOverhead(sp, channelContext,
                                                                      blocks * plainTextBlockSize);

    /* Allocate memory for encryption overhead */
    UA_ByteString encrypted;
    retval = UA_ByteString_allocBuffer(&encrypted, (blocks * plainTextBlockSize) + overHead);
    if(retval != UA_STATUSCODE_GOOD) {
        sp->channelModule.deleteContext(channelContext);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_Byte *pos = encrypted.data;
    const UA_Byte *end = &encrypted.data[encrypted.length];
    UA_UInt32_encodeBinary(&length, &pos, end);
    memcpy(pos, tokenData->data, tokenData->length);
    memcpy(&pos[tokenData->length], client->channel.remoteNonce.data,
           client->channel.remoteNonce.length);

    /* Add padding
     *
     * 7.36.2.2 Legacy Encrypted Token Secret Format: A Client should not add any
     * padding after the secret. If a Client adds padding then all bytes shall
     * be zero. A Server shall check for padding added by Clients and ensure
     * that all padding bytes are zeros. */
    size_t paddedLength = plainTextBlockSize * blocks;
    for(size_t i = totalLength; i < paddedLength; i++)
        encrypted.data[i] = 0;
    encrypted.length = paddedLength;

    retval = sp->asymmetricModule.cryptoModule.encryptionAlgorithm.encrypt(sp, channelContext,
                                                                           &encrypted);
    encrypted.length = (blocks * plainTextBlockSize) + overHead;

    if(iit) {
        retval |= UA_String_copy(&sp->asymmetricModule.cryptoModule.encryptionAlgorithm.uri,
                                 &iit->encryptionAlgorithm);
    } else {
        retval |= UA_String_copy(&sp->asymmetricModule.cryptoModule.encryptionAlgorithm.uri,
                                 &unit->encryptionAlgorithm);
    }

    UA_ByteString_deleteMembers(tokenData);
    *tokenData = encrypted;

    /* Delete the temp channel context */
    sp->channelModule.deleteContext(channelContext);

    return retval;
}
#endif

static UA_StatusCode
activateSession(UA_Client *client) {
    UA_ActivateSessionRequest request;
    UA_ActivateSessionRequest_init(&request);
    request.requestHeader.requestHandle = ++client->requestHandle;
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 600000;
    UA_StatusCode retval =
        UA_ExtensionObject_copy(&client->config.userIdentityToken, &request.userIdentityToken);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* If not token is set, use anonymous */
    if(request.userIdentityToken.encoding == UA_EXTENSIONOBJECT_ENCODED_NOBODY) {
        UA_AnonymousIdentityToken *t = UA_AnonymousIdentityToken_new();
        if(!t) {
            UA_ActivateSessionRequest_deleteMembers(&request);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        request.userIdentityToken.content.decoded.data = t;
        request.userIdentityToken.content.decoded.type = &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN];
        request.userIdentityToken.encoding = UA_EXTENSIONOBJECT_DECODED;
    }

    /* Set the policy-Id from the endpoint. Every IdentityToken starts with a
     * string. */
    retval = UA_String_copy(&client->config.userTokenPolicy.policyId,
                            (UA_String*)request.userIdentityToken.content.decoded.data);

#ifdef UA_ENABLE_ENCRYPTION
    /* Encrypt the UserIdentityToken */
    const UA_String *userTokenPolicy = &client->channel.securityPolicy->policyUri;
    if(client->config.userTokenPolicy.securityPolicyUri.length > 0)
        userTokenPolicy = &client->config.userTokenPolicy.securityPolicyUri;
    retval |= encryptUserIdentityToken(client, userTokenPolicy, &request.userIdentityToken);

    /* This function call is to prepare a client signature */
    retval |= signActivateSessionRequest(&client->channel, &request);
#endif

    if(retval != UA_STATUSCODE_GOOD) {
        UA_ActivateSessionRequest_deleteMembers(&request);
        return retval;
    }

    UA_ActivateSessionResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST],
                        &response, &UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE]);

    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "ActivateSession failed with error code %s",
                     UA_StatusCode_name(response.responseHeader.serviceResult));
    }

    retval = response.responseHeader.serviceResult;
    UA_ActivateSessionRequest_deleteMembers(&request);
    UA_ActivateSessionResponse_deleteMembers(&response);
    return retval;
}

/* Gets a list of endpoints. Memory is allocated for endpointDescription array */
UA_StatusCode
UA_Client_getEndpointsInternal(UA_Client *client, const UA_String endpointUrl,
                               size_t *endpointDescriptionsSize,
                               UA_EndpointDescription **endpointDescriptions) {
    UA_GetEndpointsRequest request;
    UA_GetEndpointsRequest_init(&request);
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    // assume the endpointurl outlives the service call
    request.endpointUrl = endpointUrl;

    UA_GetEndpointsResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_GETENDPOINTSREQUEST],
                        &response, &UA_TYPES[UA_TYPES_GETENDPOINTSRESPONSE]);

    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_StatusCode retval = response.responseHeader.serviceResult;
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
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
selectEndpoint(UA_Client *client, const UA_String endpointUrl) {
    UA_EndpointDescription* endpointArray = NULL;
    size_t endpointArraySize = 0;
    UA_StatusCode retval =
        UA_Client_getEndpointsInternal(client, endpointUrl,
                                       &endpointArraySize, &endpointArray);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_Boolean endpointFound = false;
    UA_Boolean tokenFound = false;
    UA_String binaryTransport = UA_STRING("http://opcfoundation.org/UA-Profile/"
                                          "Transport/uatcp-uasc-uabinary");

    UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT, "Found %lu endpoints", (long unsigned)endpointArraySize);
    for(size_t i = 0; i < endpointArraySize; ++i) {
        UA_EndpointDescription* endpoint = &endpointArray[i];
        /* Match Binary TransportProfile?
         * Note: Siemens returns empty ProfileUrl, we will accept it as binary */
        if(endpoint->transportProfileUri.length != 0 &&
           !UA_String_equal(&endpoint->transportProfileUri, &binaryTransport))
            continue;

        /* Valid SecurityMode? */
        if(endpoint->securityMode < 1 || endpoint->securityMode > 3) {
            UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT, "Rejecting endpoint %lu: invalid security mode", (long unsigned)i);
            continue;
        }

        /* Selected SecurityMode? */
        if(client->config.securityMode > 0 &&
           client->config.securityMode != endpoint->securityMode) {
            UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT, "Rejecting endpoint %lu: security mode doesn't match", (long unsigned)i);
            continue;
        }

        /* Matching SecurityPolicy? */
        if(client->config.securityPolicyUri.length > 0 &&
           !UA_String_equal(&client->config.securityPolicyUri,
                            &endpoint->securityPolicyUri)) {
            UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT, "Rejecting endpoint %lu: security policy doesn't match", (long unsigned)i);
            continue;
        }

        /* SecurityPolicy available? */
        if(!getSecurityPolicy(client, endpoint->securityPolicyUri)) {
            UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT, "Rejecting endpoint %lu: security policy not available", (long unsigned)i);
            continue;
        }

        endpointFound = true;

        /* Select a matching UserTokenPolicy inside the endpoint */
        UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT, "Endpoint %lu has %lu user token policies", (long unsigned)i, (long unsigned)endpoint->userIdentityTokensSize);
        for(size_t j = 0; j < endpoint->userIdentityTokensSize; ++j) {
            UA_UserTokenPolicy* userToken = &endpoint->userIdentityTokens[j];

            /* Usertokens also have a security policy... */
            if (userToken->securityPolicyUri.length > 0 &&
                !getSecurityPolicy(client, userToken->securityPolicyUri)) {
                UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT, "Rejecting UserTokenPolicy %lu in endpoint %lu: security policy '%.*s' not available",
                (long unsigned)j, (long unsigned)i,
                (int)userToken->securityPolicyUri.length, userToken->securityPolicyUri.data);
                continue;
            }

            if(userToken->tokenType > 3) {
                UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT, "Rejecting UserTokenPolicy %lu in endpoint %lu: invalid token type", (long unsigned)j, (long unsigned)i);
                continue;
            }

            /* Does the token type match the client configuration? */
            if (userToken->tokenType == UA_USERTOKENTYPE_ANONYMOUS &&
                client->config.userIdentityToken.content.decoded.type != &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN] &&
                client->config.userIdentityToken.content.decoded.type != NULL) {
                UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT, "Rejecting UserTokenPolicy %lu (anonymous) in endpoint %lu: configuration doesn't match", (long unsigned)j, (long unsigned)i);
                continue;
            }
            if (userToken->tokenType == UA_USERTOKENTYPE_USERNAME &&
                client->config.userIdentityToken.content.decoded.type != &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN]) {
                UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT, "Rejecting UserTokenPolicy %lu (username) in endpoint %lu: configuration doesn't match", (long unsigned)j, (long unsigned)i);
                continue;
            }
            if (userToken->tokenType == UA_USERTOKENTYPE_CERTIFICATE &&
                client->config.userIdentityToken.content.decoded.type != &UA_TYPES[UA_TYPES_X509IDENTITYTOKEN]) {
                UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT, "Rejecting UserTokenPolicy %lu (certificate) in endpoint %lu: configuration doesn't match", (long unsigned)j, (long unsigned)i);
                continue;
            }
            if (userToken->tokenType == UA_USERTOKENTYPE_ISSUEDTOKEN &&
                client->config.userIdentityToken.content.decoded.type != &UA_TYPES[UA_TYPES_ISSUEDIDENTITYTOKEN]) {
                UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT, "Rejecting UserTokenPolicy %lu (token) in endpoint %lu: configuration doesn't match", (long unsigned)j, (long unsigned)i);
                continue;
            }

            /* Endpoint with matching UserTokenPolicy found. Copy to the configuration. */
            tokenFound = true;
            UA_EndpointDescription_deleteMembers(&client->config.endpoint);
            UA_EndpointDescription temp = *endpoint;
            temp.userIdentityTokensSize = 0;
            temp.userIdentityTokens = NULL;
            UA_UserTokenPolicy_deleteMembers(&client->config.userTokenPolicy);

            retval = UA_EndpointDescription_copy(&temp, &client->config.endpoint);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "Copying endpoint description failed with error code %s",
                    UA_StatusCode_name(retval));
                break;
            }

            retval = UA_UserTokenPolicy_copy(userToken, &client->config.userTokenPolicy);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "Copying user token policy failed with error code %s",
                    UA_StatusCode_name(retval));
                break;
            }

#if UA_LOGLEVEL <= 300
            const char *securityModeNames[3] = {"None", "Sign", "SignAndEncrypt"};
            const char *userTokenTypeNames[4] = {"Anonymous", "UserName",
                                                 "Certificate", "IssuedToken"};
            UA_String *securityPolicyUri = &userToken->securityPolicyUri;
            if(securityPolicyUri->length == 0)
                securityPolicyUri = &endpoint->securityPolicyUri;

            /* Log the selected endpoint */
            UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                        "Selected Endpoint %.*s with SecurityMode %s and SecurityPolicy %.*s",
                        (int)endpoint->endpointUrl.length, endpoint->endpointUrl.data,
                        securityModeNames[endpoint->securityMode - 1],
                        (int)endpoint->securityPolicyUri.length,
                        endpoint->securityPolicyUri.data);

            /* Log the selected UserTokenPolicy */
            UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                        "Selected UserTokenPolicy %.*s with UserTokenType %s and SecurityPolicy %.*s",
                        (int)userToken->policyId.length, userToken->policyId.data,
                        userTokenTypeNames[userToken->tokenType],
                        (int)securityPolicyUri->length, securityPolicyUri->data);
#endif
            break;
        }

        if(tokenFound)
            break;
    }

    UA_Array_delete(endpointArray, endpointArraySize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);

    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    if(!endpointFound) {
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "No suitable endpoint found");
        retval = UA_STATUSCODE_BADINTERNALERROR;
    } else if(!tokenFound) {
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
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
    request.requestedSessionTimeout = client->config.requestedSessionTimeout;
    request.maxResponseMessageSize = UA_INT32_MAX;
    UA_String_copy(&client->config.endpoint.endpointUrl, &request.endpointUrl);

    UA_ApplicationDescription_copy(&client->config.clientDescription,
                                   &request.clientDescription);

    if(client->channel.securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       client->channel.securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        UA_ByteString_copy(&client->channel.securityPolicy->localCertificate,
                           &request.clientCertificate);
    }

    UA_CreateSessionResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
                        &response, &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE]);

    if(response.responseHeader.serviceResult == UA_STATUSCODE_GOOD) {
        /* Verify the encrypted response */
        if(client->channel.securityMode == UA_MESSAGESECURITYMODE_SIGN ||
           client->channel.securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {

            if(!UA_ByteString_equal(&response.serverCertificate,
                                    &client->channel.remoteCertificate)) {
                retval = UA_STATUSCODE_BADCERTIFICATEINVALID;
                goto cleanup;
            }

            /* Verify the client signature */
            retval = checkClientSignature(&client->channel, &response);
            if(retval != UA_STATUSCODE_GOOD)
                goto cleanup;
        }

        /* Copy nonce and and authenticationtoken */
        UA_ByteString_deleteMembers(&client->channel.remoteNonce);
        retval |= UA_ByteString_copy(&response.serverNonce, &client->channel.remoteNonce);

        UA_NodeId_deleteMembers(&client->authenticationToken);
        retval |= UA_NodeId_copy(&response.authenticationToken, &client->authenticationToken);
    }

    retval |= response.responseHeader.serviceResult;

 cleanup:
    UA_CreateSessionRequest_deleteMembers(&request);
    UA_CreateSessionResponse_deleteMembers(&response);
    return retval;
}

UA_StatusCode
UA_Client_connectTCPSecureChannel(UA_Client *client, const UA_String endpointUrl) {
    if(client->state >= UA_CLIENTSTATE_CONNECTED)
        return UA_STATUSCODE_GOOD;

    UA_ChannelSecurityToken_init(&client->channel.securityToken);
    client->channel.state = UA_SECURECHANNELSTATE_FRESH;
    client->channel.sendSequenceNumber = 0;
    client->requestId = 0;

    /* Set the channel SecurityMode */
    client->channel.securityMode = client->config.endpoint.securityMode;
    if(client->channel.securityMode == UA_MESSAGESECURITYMODE_INVALID)
        client->channel.securityMode = UA_MESSAGESECURITYMODE_NONE;

    /* Initialized the SecureChannel */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_LOG_DEBUG(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                 "Initialize the SecurityPolicy context");
    if(!client->channel.securityPolicy) {
        /* Set the channel SecurityPolicy to #None if no endpoint is selected */
        UA_String sps = client->config.endpoint.securityPolicyUri;
        if(sps.length == 0) {
            UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                        "SecurityPolicy not specified -> use default #None");
            sps = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None");
        }

        UA_SecurityPolicy *sp = getSecurityPolicy(client, sps);
        if(!sp) {
            UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                         "Failed to find the required security policy");
            retval = UA_STATUSCODE_BADINTERNALERROR;
            goto cleanup;
        }
        
        
        retval = UA_SecureChannel_setSecurityPolicy(&client->channel, sp,
                                                    &client->config.endpoint.serverCertificate);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                        "Failed to set the security policy");
            goto cleanup;
        }
    }

    /* Open a TCP connection */
    client->connection = client->config.connectionFunc(client->config.localConnectionConfig,
                                                       endpointUrl, client->config.timeout,
                                                       &client->config.logger);
    if(client->connection.state != UA_CONNECTION_OPENING) {
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "Opening the TCP socket failed");
        retval = UA_STATUSCODE_BADCONNECTIONCLOSED;
        goto cleanup;
    }

    UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                "TCP connection established");

    /* Perform the HEL/ACK handshake */
    client->connection.config = client->config.localConnectionConfig;
    retval = HelAckHandshake(client, endpointUrl);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "HEL/ACK handshake failed");
        goto cleanup;
    }
    setClientState(client, UA_CLIENTSTATE_CONNECTED);

    /* Open a SecureChannel. */
    retval = UA_SecureChannel_generateLocalNonce(&client->channel);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "Generating a local nonce failed");
        goto cleanup;
    }
    client->channel.connection = &client->connection;
    retval = openSecureChannel(client, false);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "Opening a secure channel failed");
        goto cleanup;
    }
    retval = UA_SecureChannel_generateNewKeys(&client->channel);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "Generating new keys failed");
        return retval;
    }
    setClientState(client, UA_CLIENTSTATE_SECURECHANNEL);

    return retval;

cleanup:
    UA_Client_disconnect(client);
    return retval;
}

UA_StatusCode
UA_Client_connectSession(UA_Client *client) {
    if(client->state < UA_CLIENTSTATE_SECURECHANNEL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Delete async service. TODO: Move this from connect to the disconnect/cleanup phase */
    UA_Client_AsyncService_removeAll(client, UA_STATUSCODE_BADSHUTDOWN);

    // TODO: actually, reactivate an existing session is working, but currently
    // republish is not implemented This option is disabled until we have a good
    // implementation of the subscription recovery.

#ifdef UA_SESSION_RECOVERY
    /* Try to activate an existing Session for this SecureChannel */
    if((!UA_NodeId_equal(&client->authenticationToken, &UA_NODEID_NULL)) && (createNewSession)) {
        UA_StatusCode res = activateSession(client);
        if(res != UA_STATUSCODE_BADSESSIONIDINVALID) {
            if(res == UA_STATUSCODE_GOOD) {
                setClientState(client, UA_CLIENTSTATE_SESSION_RENEWED);
            } else {
                UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                             "Could not activate the Session with StatusCode %s",
                             UA_StatusCode_name(retval));
                UA_Client_disconnect(client);
            }
            return res;
        }
    }
#endif /* UA_SESSION_RECOVERY */

    /* Could not recover an old session. Remove authenticationToken */
    UA_NodeId_deleteMembers(&client->authenticationToken);

    /* Create a session */
    UA_LOG_DEBUG(&client->config.logger, UA_LOGCATEGORY_CLIENT, "Create a new session");
    UA_StatusCode retval = createSession(client);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "Could not open a Session with StatusCode %s",
                     UA_StatusCode_name(retval));
        UA_Client_disconnect(client);
        return retval;
    }
    
    /* A new session has been created. We need to clean up the subscriptions */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_Client_Subscriptions_clean(client);
    client->currentlyOutStandingPublishRequests = 0;
#endif

    /* Activate the session */
    retval = activateSession(client);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "Could not activate the Session with StatusCode %s",
                     UA_StatusCode_name(retval));
        UA_Client_disconnect(client);
        return retval;
    }
    setClientState(client, UA_CLIENTSTATE_SESSION);
    return retval;
}

#ifdef UA_ENABLE_ENCRYPTION
/* The local ApplicationURI has to match the certificates of the
 * SecurityPolicies */
static void
verifyClientApplicationURI(const UA_Client *client) {
#if UA_LOGLEVEL <= 400
    for(size_t i = 0; i < client->config.securityPoliciesSize; i++) {
        UA_SecurityPolicy *sp = &client->config.securityPolicies[i];
        if(!sp->certificateVerification)
            continue;
        UA_StatusCode retval =
            sp->certificateVerification->
            verifyApplicationURI(sp->certificateVerification->context,
                                 &sp->localCertificate,
                                 &client->config.clientDescription.applicationUri);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                           "The configured ApplicationURI does not match the URI "
                           "specified in the certificate for the SecurityPolicy %.*s",
                           (int)sp->policyUri.length, sp->policyUri.data);
        }
    }
#endif
}
#endif

UA_StatusCode
UA_Client_connectInternal(UA_Client *client, const UA_String endpointUrl) {
    if(client->state >= UA_CLIENTSTATE_CONNECTED)
        return UA_STATUSCODE_GOOD;

    UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                "Connecting to endpoint %.*s", (int)endpointUrl.length,
                endpointUrl.data);

#ifdef UA_ENABLE_ENCRYPTION
    verifyClientApplicationURI(client);
#endif

    /* Get endpoints only if the description has not been touched (memset to zero) */
    UA_Byte test = 0;
    UA_Byte *pos = (UA_Byte*)&client->config.endpoint;
    for(size_t i = 0; i < sizeof(UA_EndpointDescription); i++)
        test = test | pos[i];
    pos = (UA_Byte*)&client->config.userTokenPolicy;
    for(size_t i = 0; i < sizeof(UA_UserTokenPolicy); i++)
        test = test | pos[i];
    UA_Boolean getEndpoints = (test == 0);

    /* Connect up to the SecureChannel */
    UA_StatusCode retval = UA_Client_connectTCPSecureChannel(client, endpointUrl);
    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "Couldn't connect the client to a TCP secure channel");
        goto cleanup;
    }
    
    /* Get and select endpoints if required */
    if(getEndpoints) {
        UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "Endpoint and UserTokenPolicy unconfigured, perform GetEndpoints");
        retval = selectEndpoint(client, endpointUrl);
        if(retval != UA_STATUSCODE_GOOD)
            goto cleanup;

        /* Reconnect with a new SecureChannel if the current one does not match
         * the selected endpoint */
        if(!UA_String_equal(&client->config.endpoint.securityPolicyUri,
                            &client->channel.securityPolicy->policyUri)) {
            UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                        "Disconnect to switch to a different SecurityPolicy");
            UA_Client_disconnect(client);
            return UA_Client_connectInternal(client, endpointUrl);
        }
    }

    retval = UA_Client_connectSession(client);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    return retval;

cleanup:
    UA_Client_disconnect(client);
    return retval;
}

UA_StatusCode
UA_Client_connect(UA_Client *client, const char *endpointUrl) {
    return UA_Client_connectInternal(client, UA_STRING((char*)(uintptr_t)endpointUrl));
}

UA_StatusCode
UA_Client_connect_noSession(UA_Client *client, const char *endpointUrl) {
    return UA_Client_connectTCPSecureChannel(client, UA_STRING((char*)(uintptr_t)endpointUrl));
}

UA_StatusCode
UA_Client_connect_username(UA_Client *client, const char *endpointUrl,
                           const char *username, const char *password) {
    UA_UserNameIdentityToken* identityToken = UA_UserNameIdentityToken_new();
    if(!identityToken)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    identityToken->userName = UA_STRING_ALLOC(username);
    identityToken->password = UA_STRING_ALLOC(password);
    UA_ExtensionObject_deleteMembers(&client->config.userIdentityToken);
    client->config.userIdentityToken.encoding = UA_EXTENSIONOBJECT_DECODED;
    client->config.userIdentityToken.content.decoded.type = &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN];
    client->config.userIdentityToken.content.decoded.data = identityToken;
    return UA_Client_connect(client, endpointUrl);
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
    UA_SecureChannel_close(&client->channel);
    UA_SecureChannel_deleteMembers(&client->channel);
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
        /* UA_ClientConnectionTCP_init sets initial state to opening */
        if(client->connection.close != NULL)
            client->connection.close(&client->connection);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    // TODO REMOVE WHEN UA_SESSION_RECOVERY IS READY
    /* We need to clean up the subscriptions */
    UA_Client_Subscriptions_clean(client);
#endif

    UA_SecureChannel_deleteMembers(&client->channel);

    setClientState(client, UA_CLIENTSTATE_DISCONNECTED);
    return UA_STATUSCODE_GOOD;
}
