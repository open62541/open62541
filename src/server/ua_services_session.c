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
    if(channel->securityToken.channelId == 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSECURECHANNELIDINVALID;
        return;
    }

    /* Copy the server's endpoint into the response */
    response->responseHeader.serviceResult =
        UA_Array_copy(server->endpointDescriptions, server->endpointDescriptionsSize,
                      (void**)&response->serverEndpoints, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        return;
    response->serverEndpointsSize = server->endpointDescriptionsSize;

    /* Mirror back the endpointUrl */
    for(size_t i = 0; i < response->serverEndpointsSize; ++i)
        UA_String_copy(&request->endpointUrl, &response->serverEndpoints[i].endpointUrl);

    UA_Session *newSession;
    response->responseHeader.serviceResult =
        UA_SessionManager_createSession(&server->sessionManager, channel, request, &newSession);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG_CHANNEL(server->config.logger, channel, "Processing CreateSessionRequest failed");
        return;
    }

    /* Fill the session with more information */
    newSession->maxResponseMessageSize = request->maxResponseMessageSize;
    newSession->maxRequestMessageSize = channel->connection->localConf.maxMessageSize;
    response->responseHeader.serviceResult |=
        UA_ApplicationDescription_copy(&request->clientDescription, &newSession->clientDescription);

    /* Prepare the response */
    response->sessionId = newSession->sessionId;
    response->revisedSessionTimeout = (UA_Double)newSession->timeout;
    response->authenticationToken = newSession->authenticationToken;
    response->responseHeader.serviceResult = UA_String_copy(&request->sessionName, &newSession->sessionName);
    if(server->endpointDescriptionsSize > 0)
        response->responseHeader.serviceResult |=
            UA_ByteString_copy(&server->endpointDescriptions->serverCertificate,
                               &response->serverCertificate);

    /* Failure -> remove the session */
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_SessionManager_removeSession(&server->sessionManager, &newSession->authenticationToken);
         return;
    }

    UA_LOG_DEBUG_CHANNEL(server->config.logger, channel, "Session " UA_PRINTF_GUID_FORMAT " created",
                         UA_PRINTF_GUID_DATA(newSession->sessionId.identifier.guid));
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

    // TODO: Verify client signature and generate nonce??

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
