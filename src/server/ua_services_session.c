/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_services.h"
#include "ua_server_internal.h"
#include "ua_session_manager.h"
#include "ua_types_generated_handling.h"

/* Create a signed nonce */
static UA_StatusCode
nonceAndSignCreateSessionResponse(UA_Server *server, UA_SecureChannel *channel,
                                  UA_Session *session,
                                  const UA_CreateSessionRequest *request,
                                  UA_CreateSessionResponse *response) {
    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGN &&
       channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return UA_STATUSCODE_GOOD;

    const UA_SecurityPolicy *const securityPolicy = channel->securityPolicy;
    UA_SignatureData *signatureData = &response->serverSignature;

    const UA_NodeId *authenticationToken = &session->header.authenticationToken;

    /* Generate Nonce
     * FIXME: remove magic number??? */
    UA_StatusCode retval = UA_SecureChannel_generateNonce(channel, 32, &response->serverNonce);
    UA_ByteString_deleteMembers(&session->serverNonce);
    retval |= UA_ByteString_copy(&response->serverNonce, &session->serverNonce);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_SessionManager_removeSession(&server->sessionManager, authenticationToken);
        return retval;
    }

    size_t signatureSize = securityPolicy->asymmetricModule.cryptoModule.
        getLocalSignatureSize(securityPolicy, channel->channelContext);

    retval |= UA_ByteString_allocBuffer(&signatureData->signature, signatureSize);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_SessionManager_removeSession(&server->sessionManager, authenticationToken);
        return retval;
    }

    UA_ByteString dataToSign;
    retval |= UA_ByteString_allocBuffer(&dataToSign,
                                        request->clientCertificate.length +
                                        request->clientNonce.length);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_SignatureData_deleteMembers(signatureData);
        UA_SessionManager_removeSession(&server->sessionManager, authenticationToken);
        return retval;
    }

    memcpy(dataToSign.data, request->clientCertificate.data, request->clientCertificate.length);
    memcpy(dataToSign.data + request->clientCertificate.length,
           request->clientNonce.data, request->clientNonce.length);

    retval |= UA_String_copy(&securityPolicy->asymmetricModule.cryptoModule.
                             signatureAlgorithmUri, &signatureData->algorithm);
    retval |= securityPolicy->asymmetricModule.cryptoModule.
        sign(securityPolicy, channel->channelContext, &dataToSign, &signatureData->signature);

    UA_ByteString_deleteMembers(&dataToSign);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_SignatureData_deleteMembers(signatureData);
        UA_SessionManager_removeSession(&server->sessionManager, authenticationToken);
    }
    return retval;
}

void Service_CreateSession(UA_Server *server, UA_SecureChannel *channel,
                           const UA_CreateSessionRequest *request,
                           UA_CreateSessionResponse *response) {
    if(channel == NULL) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADINTERNALERROR;
        return;
    }

    if(channel->connection == NULL) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADINTERNALERROR;
        return;
    }

    UA_LOG_DEBUG_CHANNEL(server->config.logger, channel, "Trying to create session");

    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        if(!UA_ByteString_equal(&request->clientCertificate,
                                &channel->remoteCertificate)) {
            response->responseHeader.serviceResult = UA_STATUSCODE_BADCERTIFICATEINVALID;
            return;
        }
    }
    if(channel->securityToken.channelId == 0) {
        response->responseHeader.serviceResult =
            UA_STATUSCODE_BADSECURECHANNELIDINVALID;
        return;
    }

    if(!UA_ByteString_equal(&channel->securityPolicy->policyUri,
                            &UA_SECURITY_POLICY_NONE_URI) &&
       request->clientNonce.length < 32) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNONCEINVALID;
        return;
    }

    ////////////////////// TODO: Compare application URI with certificate uri (decode certificate)

    /* Allocate the response */
    response->serverEndpoints = (UA_EndpointDescription*)
        UA_Array_new(server->config.endpointsSize,
                     &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    if(!response->serverEndpoints) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    response->serverEndpointsSize = server->config.endpointsSize;

    /* Copy the server's endpointdescriptions into the response */
    for(size_t i = 0; i < server->config.endpointsSize; ++i)
        response->responseHeader.serviceResult |=
            UA_EndpointDescription_copy(&server->config.endpoints[i].endpointDescription,
                                        &response->serverEndpoints[i]);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        return;

    /* Mirror back the endpointUrl */
    for(size_t i = 0; i < response->serverEndpointsSize; ++i) {
        UA_String_deleteMembers(&response->serverEndpoints[i].endpointUrl);
        UA_String_copy(&request->endpointUrl,
                       &response->serverEndpoints[i].endpointUrl);
    }

    UA_Session *newSession;
    response->responseHeader.serviceResult =
        UA_SessionManager_createSession(&server->sessionManager,
                                        channel, request, &newSession);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG_CHANNEL(server->config.logger, channel,
                             "Processing CreateSessionRequest failed");
        return;
    }

    /* Fill the session with more information */
    newSession->maxResponseMessageSize = request->maxResponseMessageSize;
    newSession->maxRequestMessageSize =
        channel->connection->localConf.maxMessageSize;
    response->responseHeader.serviceResult |=
        UA_ApplicationDescription_copy(&request->clientDescription,
                                       &newSession->clientDescription);

    /* Prepare the response */
    response->sessionId = newSession->sessionId;
    response->revisedSessionTimeout = (UA_Double)newSession->timeout;
    response->authenticationToken = newSession->header.authenticationToken;
    response->responseHeader.serviceResult =
        UA_String_copy(&request->sessionName, &newSession->sessionName);

    if(server->config.endpointsSize > 0)
         response->responseHeader.serviceResult |=
         UA_ByteString_copy(&channel->securityPolicy->localCertificate,
                            &response->serverCertificate);

    /* Create a signed nonce */
    response->responseHeader.serviceResult =
        nonceAndSignCreateSessionResponse(server, channel, newSession, request, response);

    /* Failure -> remove the session */
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_SessionManager_removeSession(&server->sessionManager,
                                        &newSession->header.authenticationToken);
        return;
    }

    UA_LOG_DEBUG_CHANNEL(server->config.logger, channel,
           "Session " UA_PRINTF_GUID_FORMAT " created",
           UA_PRINTF_GUID_DATA(newSession->sessionId.identifier.guid));
}

static void
checkSignature(const UA_Server *server,
               const UA_SecureChannel *channel,
               UA_Session *session,
               const UA_ActivateSessionRequest *request,
               UA_ActivateSessionResponse *response) {
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        const UA_SecurityPolicy *const securityPolicy = channel->securityPolicy;
        const UA_ByteString *const localCertificate = &securityPolicy->localCertificate;

        UA_ByteString dataToVerify;
        UA_StatusCode retval = UA_ByteString_allocBuffer(&dataToVerify,
                                                         localCertificate->length + session->serverNonce.length);

        if(retval != UA_STATUSCODE_GOOD) {
            response->responseHeader.serviceResult = retval;
            UA_LOG_DEBUG_SESSION(server->config.logger, session,
                                 "Failed to allocate buffer for signature verification! %#10x", retval);
            return;
        }

        memcpy(dataToVerify.data, localCertificate->data, localCertificate->length);
        memcpy(dataToVerify.data + localCertificate->length,
               session->serverNonce.data, session->serverNonce.length);

        retval = securityPolicy->asymmetricModule.cryptoModule.
            verify(securityPolicy, channel->channelContext, &dataToVerify,
                   &request->clientSignature.signature);
        if(retval != UA_STATUSCODE_GOOD) {
            response->responseHeader.serviceResult = retval;
            UA_LOG_DEBUG_SESSION(server->config.logger, session,
                                 "Failed to verify the client signature! %#10x", retval);
            UA_ByteString_deleteMembers(&dataToVerify);
            return;
        }

        retval  = UA_SecureChannel_generateNonce(channel, 32, &response->serverNonce);
        UA_ByteString_deleteMembers(&session->serverNonce);
        retval |= UA_ByteString_copy(&response->serverNonce, &session->serverNonce);
        if(retval != UA_STATUSCODE_GOOD) {
            response->responseHeader.serviceResult = retval;
            UA_LOG_DEBUG_SESSION(server->config.logger, session,
                                 "Failed to generate a new nonce! %#10x", retval);
            UA_ByteString_deleteMembers(&dataToVerify);
            return;
        }

        UA_ByteString_deleteMembers(&dataToVerify);
    }
}

void
Service_ActivateSession(UA_Server *server, UA_SecureChannel *channel,
                        UA_Session *session, const UA_ActivateSessionRequest *request,
                        UA_ActivateSessionResponse *response) {
    if(session->validTill < UA_DateTime_nowMonotonic()) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "ActivateSession: SecureChannel %i wants "
                            "to activate, but the session has timed out",
                            channel->securityToken.channelId);
        response->responseHeader.serviceResult =
            UA_STATUSCODE_BADSESSIONIDINVALID;
        return;
    }

    checkSignature(server, channel, session, request, response);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        return;

    /* Callback into userland access control */
    response->responseHeader.serviceResult =
        server->config.accessControl.activateSession(&session->sessionId,
                                                     &request->userIdentityToken,
                                                     &session->sessionHandle);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        return;

    /* Detach the old SecureChannel */
    if(session->header.channel && session->header.channel != channel) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "ActivateSession: Detach from old channel");
        UA_Session_detachFromSecureChannel(session);
    }

    /* Attach to the SecureChannel and activate */
    UA_Session_attachToSecureChannel(session, channel);
    session->activated = true;
    UA_Session_updateLifetime(session);
    UA_LOG_INFO_SESSION(server->config.logger, session,
                        "ActivateSession: Session activated");
}

void
Service_CloseSession(UA_Server *server, UA_Session *session,
                     const UA_CloseSessionRequest *request,
                     UA_CloseSessionResponse *response) {
    UA_LOG_INFO_SESSION(server->config.logger, session, "CloseSession");

    /* Callback into userland access control */
    server->config.accessControl.closeSession(&session->sessionId,
                                              session->sessionHandle);
    response->responseHeader.serviceResult =
        UA_SessionManager_removeSession(&server->sessionManager,
                                        &session->header.authenticationToken);
}
