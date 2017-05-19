/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_services.h"
#include "ua_server_internal.h"
#include "ua_session_manager.h"
#include "ua_types_generated_handling.h"

void Service_CreateSession(UA_Server *server, UA_SecureChannel *channel,
                           const UA_CreateSessionRequest *request,
                           UA_CreateSessionResponse *response) {
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        if(!UA_ByteString_equal(&request->clientCertificate,
                                &channel->remoteAsymAlgSettings.senderCertificate)) {
            response->responseHeader.serviceResult = UA_STATUSCODE_BADCERTIFICATEINVALID;
            return;
        }
    }
    if(channel->securityToken.channelId == 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSECURECHANNELIDINVALID;
        return;
    }

    /* Copy the server's endpoints into the response */
    response->serverEndpoints = UA_Array_new(server->endpoints.count,
                                             &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);

    if(response->serverEndpoints == NULL) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    for(size_t i = 0; i < server->endpoints.count; ++i) {
        response->responseHeader.serviceResult |= UA_EndpointDescription_copy(
            &server->endpoints.endpoints[i].endpointDescription,
            &response->serverEndpoints[i]);
    }

    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        return;
    response->serverEndpointsSize = server->endpoints.count;

    /* Mirror back the endpointUrl */
    for(size_t i = 0; i < response->serverEndpointsSize; ++i)
        UA_String_copy(&request->endpointUrl, &response->serverEndpoints[i].endpointUrl);

    UA_Session *newSession;
    response->responseHeader.serviceResult =
        UA_SessionManager_createSession(&server->sessionManager, channel, request, &newSession);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG_CHANNEL(server->config.logger, channel,
                             "Processing CreateSessionRequest failed");
        return;
    }

    /* Fill the session with more information */
    newSession->maxResponseMessageSize = request->maxResponseMessageSize;
    newSession->maxRequestMessageSize = channel->connection->localConf.maxMessageSize;
    response->responseHeader.serviceResult |=
        UA_ApplicationDescription_copy(&request->clientDescription,
                                       &newSession->clientDescription);

    /* Prepare the response */
    response->sessionId = newSession->sessionId;
    response->revisedSessionTimeout = (UA_Double)newSession->timeout;
    response->authenticationToken = newSession->authenticationToken;
    response->responseHeader.serviceResult =
        UA_String_copy(&request->sessionName, &newSession->sessionName);
    if(server->endpoints.count > 0)
        response->responseHeader.serviceResult |=
        UA_ByteString_copy(&channel->endpoint->endpointDescription.serverCertificate,
                           &response->serverCertificate);

    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        UA_StatusCode retval = UA_STATUSCODE_GOOD;

        const UA_SecurityPolicy *const securityPolicy = channel->endpoint->securityPolicy;

        retval |=
            UA_SecureChannel_generateNonce(channel, 32, &response->serverNonce); // FIXME: remove magic number???
        retval |= UA_ByteString_copy(&response->serverNonce, &newSession->serverNonce);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_SessionManager_removeSession(&server->sessionManager, &newSession->authenticationToken);
            return;
        }

        size_t signatureSize =
            securityPolicy->endpointContext.getLocalAsymSignatureSize(securityPolicy,
                                                                      channel->endpoint->securityContext);

        UA_SignatureData signatureData;
        UA_SignatureData_init(&signatureData);
        retval |= UA_ByteString_allocBuffer(&signatureData.signature, signatureSize);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_SessionManager_removeSession(&server->sessionManager, &newSession->authenticationToken);
            return;
        }

        UA_ByteString dataToSign;
        retval |=
            UA_ByteString_allocBuffer(&dataToSign,
                                      request->clientCertificate.length + request->clientNonce.length);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_SignatureData_deleteMembers(&signatureData);
            UA_SessionManager_removeSession(&server->sessionManager, &newSession->authenticationToken);
            return;
        }

        memcpy(dataToSign.data,
               request->clientCertificate.data,
               request->clientCertificate.length);
        memcpy(dataToSign.data + request->clientCertificate.length,
               request->clientNonce.data,
               request->clientNonce.length);

        retval |=
            UA_String_copy(&securityPolicy->asymmetricModule.signingModule.signatureAlgorithmUri,
                           &signatureData.algorithm);

        retval |=
            securityPolicy->asymmetricModule.signingModule.sign(securityPolicy,
                                                                channel->endpoint->securityContext,
                                                                &dataToSign,
                                                                &signatureData.signature);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_ByteString_deleteMembers(&dataToSign);
            UA_SignatureData_deleteMembers(&signatureData);
            UA_SessionManager_removeSession(&server->sessionManager, &newSession->authenticationToken);
            return;
        }

        retval |= UA_SignatureData_copy(&signatureData, &response->serverSignature);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_ByteString_deleteMembers(&dataToSign);
            UA_SignatureData_deleteMembers(&signatureData);
            UA_SessionManager_removeSession(&server->sessionManager, &newSession->authenticationToken);
            return;
        }

        UA_ByteString_deleteMembers(&dataToSign);
        UA_SignatureData_deleteMembers(&signatureData);
    }

    /* Failure -> remove the session */
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_SessionManager_removeSession(&server->sessionManager, &newSession->authenticationToken);
        return;
    }

    UA_LOG_DEBUG_CHANNEL(server->config.logger, channel, "Session " UA_PRINTF_GUID_FORMAT " created",
                         UA_PRINTF_GUID_DATA(newSession->sessionId.identifier.guid));
}

static void
Service_ActivateSession_checkSignature(const UA_Server *const server,
                                       const UA_SecureChannel *const channel,
                                       const UA_Session *const session,
                                       const UA_ActivateSessionRequest *const request,
                                       UA_ActivateSessionResponse *const response) {
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        UA_StatusCode retval = UA_STATUSCODE_GOOD;

        const UA_SecurityPolicy *const securityPolicy = channel->endpoint->securityPolicy;

        const UA_ByteString *const localCertificate =
            securityPolicy->endpointContext.getServerCertificate(securityPolicy,
                                                                channel->endpoint->securityContext);

        UA_ByteString dataToVerify;
        retval |= UA_ByteString_allocBuffer(&dataToVerify, localCertificate->length + session->serverNonce.length);
        if(retval != UA_STATUSCODE_GOOD) {
            response->responseHeader.serviceResult = retval;
            return;
        }

        memcpy(dataToVerify.data, localCertificate->data, localCertificate->length);
        memcpy(dataToVerify.data + localCertificate->length, session->serverNonce.data, session->serverNonce.length);

        retval |= securityPolicy->asymmetricModule.signingModule.verify(securityPolicy,
                                                                        channel->securityContext,
                                                                        &dataToVerify,
                                                                        &request->clientSignature.signature);
        if(retval != UA_STATUSCODE_GOOD) {
            response->responseHeader.serviceResult = retval;
            UA_ByteString_deleteMembers(&dataToVerify);
            return;
        }

        retval |= UA_SecureChannel_generateNonce(channel, 32, &response->serverNonce);
        if(retval != UA_STATUSCODE_GOOD) {
            response->responseHeader.serviceResult = retval;
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
        UA_LOG_INFO_SESSION(server->config.logger, session, "ActivateSession: SecureChannel %i wants "
                            "to activate, but the session has timed out", channel->securityToken.channelId);
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;
        return;
    }

    Service_ActivateSession_checkSignature(server,
                                           channel,
                                           session,
                                           request,
                                           response);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        return;

    /* Callback into userland access control */
    response->responseHeader.serviceResult =
        server->config.accessControl.activateSession(&session->sessionId, &request->userIdentityToken,
                                                     &session->sessionHandle);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        return;

    /* Detach the old SecureChannel */
    if(session->channel && session->channel != channel) {
        UA_LOG_INFO_SESSION(server->config.logger, session, "ActivateSession: Detach from old channel");
        UA_SecureChannel_detachSession(session->channel, session);
    }

    /* Attach to the SecureChannel and activate */
    UA_SecureChannel_attachSession(channel, session);
    session->activated = true;
    UA_Session_updateLifetime(session);
    UA_LOG_INFO_SESSION(server->config.logger, session, "ActivateSession: Session activated");
}

void
Service_CloseSession(UA_Server *server, UA_Session *session, const UA_CloseSessionRequest *request,
                     UA_CloseSessionResponse *response) {
    UA_LOG_INFO_SESSION(server->config.logger, session, "CloseSession");
    /* Callback into userland access control */
    server->config.accessControl.closeSession(&session->sessionId, session->sessionHandle);
    response->responseHeader.serviceResult =
        UA_SessionManager_removeSession(&server->sessionManager, &session->authenticationToken);
}
