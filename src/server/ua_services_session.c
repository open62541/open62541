/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2020 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014-2017 (c) Florian Palm
 *    Copyright 2014-2016 (c) Sten Grüner
 *    Copyright 2015 (c) Chris Iatrou
 *    Copyright 2015 (c) Oleksiy Vasylyev
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017-2018 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2019 (c) Kalycito Infotech Private Limited
 *    Copyright 2018-2020 (c) HMS Industrial Networks AB (Author: Jonas Green)
 *    Copyright 2025 (c) Siemens AG (Author: Tin Raic)
 */

#include "ua_server_internal.h"
#include "ua_services.h"

void
notifySession(UA_Server *server, UA_Session *session,
              UA_ApplicationNotificationType type) {
    /* Nothing to do */
    if(!server->config.globalNotificationCallback &&
       !server->config.sessionNotificationCallback)
        return;

    /* Set up the payload */
    size_t payloadSize = 6 + session->attributes.mapSize;
    UA_STACKARRAY(UA_KeyValuePair, payloadData, payloadSize);
    UA_KeyValueMap payloadMap = {payloadSize, payloadData};
    payloadData[0].key = UA_QUALIFIEDNAME(0, "session-id");
    UA_Variant_setScalar(&payloadData[0].value, &session->sessionId,
                         &UA_TYPES[UA_TYPES_NODEID]);
    payloadData[1].key = UA_QUALIFIEDNAME(0, "securechannel-id");
    UA_UInt32 secureChannelId = 0;
    if(session->channel)
        secureChannelId = session->channel->securityToken.channelId;
    UA_Variant_setScalar(&payloadData[1].value, &secureChannelId,
                         &UA_TYPES[UA_TYPES_UINT32]);
    payloadData[2].key = UA_QUALIFIEDNAME(0, "session-name");
    UA_Variant_setScalar(&payloadData[2].value, &session->sessionName,
                         &UA_TYPES[UA_TYPES_STRING]);
    payloadData[3].key = UA_QUALIFIEDNAME(0, "client-description");
    UA_Variant_setScalar(&payloadData[3].value, &session->clientDescription,
                         &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);
    payloadData[4].key = UA_QUALIFIEDNAME(0, "client-user-id");
    UA_Variant_setScalar(&payloadData[4].value, &session->clientUserIdOfSession,
                         &UA_TYPES[UA_TYPES_STRING]);
    payloadData[5].key = UA_QUALIFIEDNAME(0, "locale-ids");
    UA_Variant_setArray(&payloadData[5].value, session->localeIds,
                        session->localeIdsSize, &UA_TYPES[UA_TYPES_STRING]);

    memcpy(&payloadData[6], session->attributes.map,
           sizeof(UA_KeyValuePair) * session->attributes.mapSize);

    /* Call the notification callback */
    if(server->config.sessionNotificationCallback)
        server->config.sessionNotificationCallback(server, type, payloadMap);
    if(server->config.globalNotificationCallback)
        server->config.globalNotificationCallback(server, type, payloadMap);
}

/* Delayed callback to free the session memory */
static void
removeSessionCallback(UA_Server *server, session_list_entry *entry) {
    lockServer(server);
    UA_Session_clear(&entry->session, server);
    unlockServer(server);
    UA_free(entry);
}

void
UA_Session_remove(UA_Server *server, UA_Session *session,
                  UA_ShutdownReason shutdownReason) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Remove the Subscriptions */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_Subscription *sub, *tempsub;
    TAILQ_FOREACH_SAFE(sub, &session->subscriptions, sessionListEntry, tempsub) {
        UA_Subscription_delete(server, sub);
    }

    UA_PublishResponseEntry *entry;
    while((entry = UA_Session_dequeuePublishReq(session))) {
        UA_PublishResponse_clear(&entry->response);
        UA_free(entry);
    }
#endif

    /* Callback into userland access control */
    if(server->config.accessControl.closeSession) {
        server->config.accessControl.
            closeSession(server, &server->config.accessControl,
                         &session->sessionId, session->context);
    }

    /* Detach the Session from the SecureChannel */
    UA_Session_detachFromSecureChannel(server, session);

    /* Deactivate the session */
    if(session->activated) {
        session->activated = false;
        server->activeSessionCount--;
    }

    /* Detach the session from the session manager and make the capacity
     * available */
    session_list_entry *sentry = container_of(session, session_list_entry, session);
    LIST_REMOVE(sentry, pointers);
    server->sessionCount--;

    switch(shutdownReason) {
    case UA_SHUTDOWNREASON_CLOSE:
    case UA_SHUTDOWNREASON_PURGE:
        break;
    case UA_SHUTDOWNREASON_TIMEOUT:
        server->serverDiagnosticsSummary.sessionTimeoutCount++;
        break;
    case UA_SHUTDOWNREASON_REJECT:
        server->serverDiagnosticsSummary.rejectedSessionCount++;
        break;
    case UA_SHUTDOWNREASON_SECURITYREJECT:
        server->serverDiagnosticsSummary.securityRejectedSessionCount++;
        break;
    case UA_SHUTDOWNREASON_ABORT:
        server->serverDiagnosticsSummary.sessionAbortCount++;
        break;
    default:
        UA_assert(false);
        break;
    }

    /* Notify the application */
    notifySession(server, session, UA_APPLICATIONNOTIFICATIONTYPE_SESSION_CLOSED);

    /* Add a delayed callback to remove the session when the currently
     * scheduled jobs have completed */
    sentry->cleanupCallback.callback = (UA_Callback)removeSessionCallback;
    sentry->cleanupCallback.application = server;
    sentry->cleanupCallback.context = sentry;
    UA_EventLoop *el = server->config.eventLoop;
    el->addDelayedCallback(el, &sentry->cleanupCallback);
}

void
cleanupSessions(UA_Server *server, UA_DateTime nowMonotonic) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    session_list_entry *sentry, *temp;
    LIST_FOREACH_SAFE(sentry, &server->sessions, pointers, temp) {
        /* Session has timed out? */
        if(sentry->session.validTill >= nowMonotonic)
            continue;
        UA_LOG_INFO_SESSION(server->config.logging, &sentry->session,
                            "Session has timed out");
        UA_Session_remove(server, &sentry->session, UA_SHUTDOWNREASON_TIMEOUT);
    }
}

/************/
/* Services */
/************/

UA_Session *
getSessionByToken(UA_Server *server, const UA_NodeId *token) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    session_list_entry *current = NULL;
    LIST_FOREACH(current, &server->sessions, pointers) {
        /* Token does not match */
        if(!UA_NodeId_equal(&current->session.authenticationToken, token))
            continue;

        /* Session has timed out */
        UA_EventLoop *el = server->config.eventLoop;
        UA_DateTime now = el->dateTime_nowMonotonic(el);
        if(now > current->session.validTill) {
            UA_LOG_INFO_SESSION(server->config.logging, &current->session,
                                "Client tries to use a session that has timed out");
            return NULL;
        }

        return &current->session;
    }

    return NULL;
}

UA_Session *
getSessionById(UA_Server *server, const UA_NodeId *sessionId) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    session_list_entry *current = NULL;
    LIST_FOREACH(current, &server->sessions, pointers) {
        /* Token does not match */
        if(!UA_NodeId_equal(&current->session.sessionId, sessionId))
            continue;

        /* Session has timed out */
        UA_EventLoop *el = server->config.eventLoop;
        UA_DateTime now = el->dateTime_nowMonotonic(el);
        if(now > current->session.validTill) {
            UA_LOG_INFO_SESSION(server->config.logging, &current->session,
                                "Client tries to use a session that has timed out");
            return NULL;
        }

        return &current->session;
    }

    if(UA_NodeId_equal(sessionId, &server->adminSession.sessionId))
        return &server->adminSession;

    return NULL;
}

static UA_StatusCode
signCreateSessionResponse(UA_Server *server, UA_SecureChannel *channel,
                          const UA_CreateSessionRequest *request,
                          UA_CreateSessionResponse *response) {
    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGN &&
       channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return UA_STATUSCODE_GOOD;

    const UA_SecurityPolicy *sp = channel->securityPolicy;
    void *cc = channel->channelContext;
    UA_SignatureData *signatureData = &response->serverSignature;

    /* Prepare the signature */
    const UA_SecurityPolicySignatureAlgorithm *signAlg = &sp->asymSignatureAlgorithm;
    size_t signatureSize = signAlg->getLocalSignatureSize(sp, cc);
    UA_StatusCode retval = UA_String_copy(&signAlg->uri, &signatureData->algorithm);
    retval |= UA_ByteString_allocBuffer(&signatureData->signature, signatureSize);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Allocate a temp buffer */
    size_t dataToSignSize =
        request->clientCertificate.length + request->clientNonce.length;
    UA_ByteString dataToSign;
    retval = UA_ByteString_allocBuffer(&dataToSign, dataToSignSize);
    if(retval != UA_STATUSCODE_GOOD)
        return retval; /* signatureData->signature is cleaned up with the response */

    /* Sign the signature */
    memcpy(dataToSign.data, request->clientCertificate.data,
           request->clientCertificate.length);
    memcpy(dataToSign.data + request->clientCertificate.length,
           request->clientNonce.data, request->clientNonce.length);
    retval = signAlg->sign(sp, cc, &dataToSign, &signatureData->signature);

    /* Clean up */
    UA_ByteString_clear(&dataToSign);
    return retval;
}

static UA_StatusCode
createCheckSessionAuthSecurityPolicyContext(UA_Server *server, UA_Session *session,
                                            UA_SecurityPolicy *sp,
                                            const UA_ByteString remoteCertificate) {
    /* The session is already "taken" by a different SecurityPolicy */
    if(session->authSp && session->authSp != sp) {
        UA_LOG_ERROR_SESSION(server->config.logging, session,
                             "Cannot instantiate SecurityPolicyContext %S for the "
                             "Session. A different SecurityPolicy %S is "
                             "already in place",
                             sp->policyUri, session->authSp->policyUri);
        return UA_STATUSCODE_BADSECURITYPOLICYREJECTEDl;
    }
    session->authSp = sp;

    /* Existing SecurityPolicy context, check for the identical remote
     * certificate */
    UA_StatusCode res;
    if(session->authSpContext) {
        res = sp->compareCertificate(sp, session->authSpContext, &remoteCertificate);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR_SESSION(server->config.logging, session,
                                 "The client tries to use a different certificate "
                                 "for authentication");
        }
        return res;
    }

    /* Instantiate a new context */
    res = sp->newChannelContext(sp, &remoteCertificate, &session->authSpContext);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_SESSION(server->config.logging, session,
                             "Could not use the supplied certificate "
                             "to instantiate the SecurityPolicy %S for the "
                             "validation of the UserIdentityToken",
                             sp->policyUri);
    }
    return res;
}

static UA_StatusCode
addEphemeralKeyAdditionalHeader(UA_Server *server, UA_Session *session,
                                UA_ExtensionObject *ah) {
    UA_assert(session->authSp && session->authSpContext);
    UA_SecurityPolicy *sp = session->authSp;
    void *spContext = session->authSpContext;

    /* Allocate additional parameters */
    UA_AdditionalParametersType *ap = UA_AdditionalParametersType_new();
    if(!ap)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Set the additional parameters in the additional header. They also get
     * cleaned up from there in the error case. */
    UA_ExtensionObject_setValue(ah, ap, &UA_TYPES[UA_TYPES_ADDITIONALPARAMETERSTYPE]);

    /* UA_KeyValueMap has the identical layout. And better helper methods. */
    UA_KeyValueMap *map = (UA_KeyValueMap*)ap;

    /* Add the PolicyUri to the map */
    UA_StatusCode res =
        UA_KeyValueMap_setScalar(map, UA_QUALIFIEDNAME(0, "ECDHPolicyUri"),
                                 &sp->policyUri, &UA_TYPES[UA_TYPES_STRING]);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Allocate the EphemeralKey structure */
    UA_EphemeralKeyType *ephKey = UA_EphemeralKeyType_new();
    if(!ephKey)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Add the EphemeralKeyto the map */
    res = UA_KeyValueMap_setScalarShallow(map, UA_QUALIFIEDNAME(0, "ECDHKey"),
                                          ephKey, &UA_TYPES[UA_TYPES_EPHEMERALKEYTYPE]);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Allocate the ephemeral key buffer to the exact size of the ephemeral key
     * for the used ECC policy so that the nonce generation function knows that
     * it needs to generate an ephemeral key and not some other random byte
     * string.
     *
     * TODO: There should be a more stable way to signal the generation of an
     * ephemeral key */
    res = UA_ByteString_allocBuffer(&ephKey->publicKey, sp->nonceLength);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Allocate the signature buffer */
    size_t signatureSize =
        sp->asymSignatureAlgorithm.getLocalSignatureSize(sp, spContext);
    res = UA_ByteString_allocBuffer(&ephKey->signature, signatureSize);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Generate the ephemeral key and signature */
    res |= sp->generateNonce(sp, spContext, &ephKey->publicKey);
    res |= sp->asymSignatureAlgorithm.sign(sp, spContext, &ephKey->publicKey,
                                           &ephKey->signature);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_SESSION(server->config.logging, session,
                             "Could not prepare the ephemeral key (%s)",
                             UA_StatusCode_name(res));
    }
    return res;
}

/* Creates and adds a session. But it is not yet attached to a secure channel. */
UA_StatusCode
UA_Session_create(UA_Server *server, UA_SecureChannel *channel,
                  const UA_CreateSessionRequest *request, UA_Session **session) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    if(server->sessionCount >= server->config.maxSessions) {
        UA_LOG_WARNING_CHANNEL(server->config.logging, channel,
                               "Could not create a Session - Server limits reached");
        return UA_STATUSCODE_BADTOOMANYSESSIONS;
    }

    session_list_entry *newentry = (session_list_entry*)
        UA_malloc(sizeof(session_list_entry));
    if(!newentry)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Initialize the Session */
    UA_Session_init(&newentry->session);
    newentry->session.sessionId = UA_NODEID_GUID(1, UA_Guid_random());
    newentry->session.authenticationToken = UA_NODEID_GUID(1, UA_Guid_random());

    newentry->session.timeout = server->config.maxSessionTimeout;
    if(request->requestedSessionTimeout <= server->config.maxSessionTimeout &&
       request->requestedSessionTimeout > 0)
        newentry->session.timeout = request->requestedSessionTimeout;

    /* Attach the session to the channel. But don't activate for now. */
    if(channel)
        UA_Session_attachToSecureChannel(server, &newentry->session, channel);

    UA_EventLoop *el = server->config.eventLoop;
    UA_DateTime now = el->dateTime_now(el);
    UA_DateTime nowMonotonic = el->dateTime_nowMonotonic(el);
    UA_Session_updateLifetime(&newentry->session, now, nowMonotonic);

    /* Add to the server */
    LIST_INSERT_HEAD(&server->sessions, newentry, pointers);
    server->sessionCount++;

    /* Notify the application */
    notifySession(server, &newentry->session,
                  UA_APPLICATIONNOTIFICATIONTYPE_SESSION_CREATED);

    /* Return */
    *session = &newentry->session;
    return UA_STATUSCODE_GOOD;
}

void
Service_CreateSession(UA_Server *server, UA_SecureChannel *channel,
                      const UA_CreateSessionRequest *request,
                      UA_CreateSessionResponse *response) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_LOG_DEBUG_CHANNEL(server->config.logging, channel, "Trying to create session");

    void *cc = channel->channelContext;
    UA_SecurityPolicy *sp = channel->securityPolicy;
    UA_ResponseHeader *rh = &response->responseHeader;
    UA_assert(sp != NULL);

    /* Part 4: In many cases, the Certificates used to establish the
     * SecureChannel will be the ApplicationInstanceCertificates. However, some
     * Communication Stacks might not support Certificates that are specific to
     * a single application. Instead, they expect all communication to be
     * secured with a Certificate specific to a user or the entire machine. For
     * this reason, OPC UA Applications will need to exchange their
     * ApplicationInstanceCertificates when creating a Session.
     *
     * However Part 6 requires that the asymmetric algorithm security header
     * contains the "X.509 v3 Certificate assigned to the sending application
     * Instance." So we assume that the SecureChannel was created with the same
     * ApplicationInstanceCertificate. This may change in the future for
     * additional transport types. */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {

        /* Compare the clientCertificate with the remoteCertificate of the
         * channel.
         *
         * Both the clientCertificate of this request and the remoteCertificate
         * of the channel may contain a partial or a complete certificate chain.
         * The compareCertificate function will compare the first certificate of
         * each chain. The end certificate shall be located first in the chain
         * according to the OPC UA specification Part 6 (1.04), chapter 6.2.3.*/
        UA_StatusCode res = sp->compareCertificate(sp, cc, &request->clientCertificate);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR_CHANNEL(server->config.logging, channel,
                                 "The client uses a different certificate for "
                                 "SecureChannel and Session");
            server->serverDiagnosticsSummary.securityRejectedSessionCount++;
            server->serverDiagnosticsSummary.rejectedSessionCount++;
            rh->serviceResult = UA_STATUSCODE_BADCERTIFICATEINVALID;
            return;
        }
    }

    /* Check the client certificate and ApplicationDescription. It was already
     * checked for the SecureChannel. But here we use the Session
     * CertificateGroup and we now also have the ApplicationDescription to check
     * the ApplicationUri. */
    if(request->clientCertificate.length > 0) {
        rh->serviceResult =
            validateCertificate(server, &server->config.secureChannelPKI,
                                channel, NULL, &request->clientDescription,
                                request->clientCertificate);
        if(rh->serviceResult != UA_STATUSCODE_GOOD) {
            server->serverDiagnosticsSummary.securityRejectedSessionCount++;
            server->serverDiagnosticsSummary.rejectedSessionCount++;
            return;
        }
    }

    /* The ClientNonce needs a length between 32 and 128 bytes inclusive */
    if(channel->securityPolicy->policyType != UA_SECURITYPOLICYTYPE_NONE &&
       (request->clientNonce.length < 32 || request->clientNonce.length > 128)) {
        UA_LOG_ERROR_CHANNEL(server->config.logging, channel,
                             "The nonce provided by the client has the wrong length");
        server->serverDiagnosticsSummary.securityRejectedSessionCount++;
        server->serverDiagnosticsSummary.rejectedSessionCount++;
        rh->serviceResult = UA_STATUSCODE_BADNONCEINVALID;
        return;
    }

    /* Create the Session */
    UA_Session *newSession = NULL;
    rh->serviceResult = UA_Session_create(server, channel, request, &newSession);
    if(rh->serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_CHANNEL(server->config.logging, channel,
                               "Processing CreateSessionRequest failed");
        server->serverDiagnosticsSummary.rejectedSessionCount++;
        return;
    }

    /* Configure the Session */

    /* If the session name is empty, use the generated SessionId */
    rh->serviceResult |= UA_String_copy(&request->sessionName,
                                        &newSession->sessionName);
    if(newSession->sessionName.length == 0)
        rh->serviceResult |= UA_NodeId_print(&newSession->sessionId,
                                             &newSession->sessionName);

    rh->serviceResult |= UA_Session_generateNonce(newSession);
    newSession->maxResponseMessageSize = request->maxResponseMessageSize;
    newSession->maxRequestMessageSize = channel->config.localMaxMessageSize;
    rh->serviceResult |= UA_ApplicationDescription_copy(&request->clientDescription,
                                                        &newSession->clientDescription);

    /* The SecureChannel certificate must correspond to the client's
     * ApplicationCertificate. But for unencrypted SecureChannels we store the
     * certificate directly in the Session. We need it later when an encrypted
     * UserIdentityToken is sent over an unencrypted SecureChannel. */
    if(channel->remoteCertificate.length == 0)
        rh->serviceResult |= UA_ByteString_copy(&request->clientCertificate,
                                                &newSession->clientCertificate);

#ifdef UA_ENABLE_DIAGNOSTICS
    rh->serviceResult |= UA_String_copy(&request->serverUri,
                                        &newSession->diagnostics.serverUri);
    rh->serviceResult |= UA_String_copy(&request->endpointUrl,
                                        &newSession->diagnostics.endpointUrl);
#endif

    if(rh->serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_CHANNEL(server->config.logging, channel,
                             "Could not create the new session (%s)",
                             UA_StatusCode_name(rh->serviceResult));
        UA_Session_remove(server, newSession, UA_SHUTDOWNREASON_REJECT);
        return;
    }

    /* Prepare the response */
    response->sessionId = newSession->sessionId;
    response->revisedSessionTimeout = (UA_Double)newSession->timeout;
    response->authenticationToken = newSession->authenticationToken;
    rh->serviceResult |= UA_ByteString_copy(&newSession->serverNonce,
                                            &response->serverNonce);

    /* Copy the server's endpointdescriptions into the response */
    rh->serviceResult |= setCurrentEndPointsArray(server, request->endpointUrl,
                                                  NULL, 0,
                                                  &response->serverEndpoints,
                                                  &response->serverEndpointsSize);

    /* Get the authentication SecurityPolicy matching the SecureChannel. The
     * same selection is done fr the generated endpoints. Don't mix RSA/ECC
     * between channel and authentication. Return the server certificate used
     * for usertoken encryption */
    UA_SecurityPolicy *authSp =
        getDefaultEncryptedSecurityPolicy(server, sp->policyType);
    if(authSp)
        rh->serviceResult |= UA_ByteString_copy(&authSp->localCertificate,
                                                &response->serverCertificate);

    if(rh->serviceResult != UA_STATUSCODE_GOOD) {
        UA_Session_remove(server, newSession, UA_SHUTDOWNREASON_REJECT);
        UA_LOG_ERROR_CHANNEL(server->config.logging, channel,
                             "Could not prepare the CreateSessionResponse (%s)",
                             UA_StatusCode_name(rh->serviceResult));
        return;
    }

    /* If ECC policy, instantiate an auth SecurityPolicy context and create an
     * ephemeral key to be returned in the response. The private part of the
     * ephemeral key is persisted in the SecurityPolicy context. Until it gets
     * overridden during ActivateSession. */
    if(authSp && authSp->policyType == UA_SECURITYPOLICYTYPE_ECC) {
        rh->serviceResult =
            createCheckSessionAuthSecurityPolicyContext(server, newSession, authSp,
                                                        request->clientCertificate);
        if(rh->serviceResult != UA_STATUSCODE_GOOD) {
            UA_Session_remove(server, newSession, UA_SHUTDOWNREASON_REJECT);
            return;
        }

        rh->serviceResult = addEphemeralKeyAdditionalHeader(server, newSession,
                                                            &rh->additionalHeader);
        if(rh->serviceResult != UA_STATUSCODE_GOOD) {
            UA_Session_remove(server, newSession, UA_SHUTDOWNREASON_REJECT);
            return;
        }
    }

    /* Sign the signature */
    rh->serviceResult |= signCreateSessionResponse(server, channel,
                                                   request, response);
    if(rh->serviceResult != UA_STATUSCODE_GOOD) {
        UA_Session_remove(server, newSession, UA_SHUTDOWNREASON_REJECT);
        UA_LOG_ERROR_CHANNEL(server->config.logging, channel,
                             "Could not sign the CreateSessionResponse (%s)",
                             UA_StatusCode_name(rh->serviceResult));
        return;
    }

#ifdef UA_ENABLE_DIAGNOSTICS
    UA_EventLoop *el = server->config.eventLoop;
    newSession->diagnostics.clientConnectionTime = el->dateTime_now(el);
    newSession->diagnostics.clientLastContactTime =
        newSession->diagnostics.clientConnectionTime;

    /* Create the object in the information model */
    createSessionObject(server, newSession);
#endif

    UA_LOG_INFO_SESSION(server->config.logging, newSession, "Session created");
}

static UA_StatusCode
checkCertificateSignature(const UA_Server *server, const UA_SecurityPolicy *sp,
                          void *channelContext, const UA_ByteString *serverNonce,
                          const UA_SignatureData *signature,
                          const bool isUserTokenSignature) {
    /* Check for zero signature length */
    if(signature->signature.length == 0) {
        if(isUserTokenSignature)
            return UA_STATUSCODE_BADUSERSIGNATUREINVALID;
        return UA_STATUSCODE_BADAPPLICATIONSIGNATUREINVALID;
    }

    if(!sp)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Data to verify is calculated by appending the serverNonce to the local
     * certificate */
    const UA_ByteString *localCertificate = &sp->localCertificate;
    UA_ByteString dataToVerify;
    size_t dataToVerifySize = localCertificate->length + serverNonce->length;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&dataToVerify, dataToVerifySize);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    memcpy(dataToVerify.data, localCertificate->data, localCertificate->length);
    memcpy(dataToVerify.data + localCertificate->length,
           serverNonce->data, serverNonce->length);
    retval = sp->asymSignatureAlgorithm.
        verify(sp, channelContext, &dataToVerify, &signature->signature);
    UA_ByteString_clear(&dataToVerify);
    if(retval != UA_STATUSCODE_GOOD) {
        if(isUserTokenSignature)
            retval = UA_STATUSCODE_BADUSERSIGNATUREINVALID;
        else
            retval = UA_STATUSCODE_BADAPPLICATIONSIGNATUREINVALID;
    }
    return retval;
}

/* Always sets tokenSp (default: SecurityPolicy of the channel) */
static const UA_UserTokenPolicy *
selectTokenPolicy(UA_Server *server, UA_SecureChannel *channel,
                  UA_Session *session, const UA_ExtensionObject *identityToken,
                  const UA_EndpointDescription *ed,
                  UA_SecurityPolicy **tokenSp) {
    /* If no UserTokenPolicies are configured in the endpoint, then use
     * those configured in the AccessControl plugin. */
    size_t identPoliciesSize = ed->userIdentityTokensSize;
    const UA_UserTokenPolicy *identPolicies = ed->userIdentityTokens;
    if(identPoliciesSize == 0) {
        identPoliciesSize = server->config.accessControl.userTokenPoliciesSize;
        identPolicies = server->config.accessControl.userTokenPolicies;
    }

    /* Match the UserTokenType */
    const UA_DataType *tokenDataType = identityToken->content.decoded.type;
    for(size_t j = 0; j < identPoliciesSize; j++) {
        const UA_UserTokenPolicy *pol = &identPolicies[j];

        /* Part 4, Section 5.6.3.2, Table 17: A NULL or empty
         * UserIdentityToken should be treated as Anonymous */
        if(identityToken->encoding == UA_EXTENSIONOBJECT_ENCODED_NOBODY &&
           pol->tokenType == UA_USERTOKENTYPE_ANONYMOUS) {
            *tokenSp = channel->securityPolicy;
            return pol;
        }

        /* Expect decoded content if not anonymous */
        if(!tokenDataType)
            continue;

        /* Match the DataType of the provided token with the policy */
        switch(pol->tokenType) {
        case UA_USERTOKENTYPE_ANONYMOUS:
            if(tokenDataType != &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN])
                continue;
            break;
        case UA_USERTOKENTYPE_USERNAME:
            if(tokenDataType != &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN])
                continue;
            break;
        case UA_USERTOKENTYPE_CERTIFICATE:
            if(tokenDataType != &UA_TYPES[UA_TYPES_X509IDENTITYTOKEN])
                continue;
            break;
        case UA_USERTOKENTYPE_ISSUEDTOKEN:
            if(tokenDataType != &UA_TYPES[UA_TYPES_ISSUEDIDENTITYTOKEN])
                continue;
            break;
        default:
            continue;
        }

        /* All valid token data types start with a string policyId. Casting
         * to anonymous hence works for all of them. */
        UA_AnonymousIdentityToken *token = (UA_AnonymousIdentityToken*)
            identityToken->content.decoded.data;

        /* In setCurrentEndPointsArray we prepend the PolicyId with the
         * SecurityMode of the endpoint and the postfix of the
         * SecurityPolicyUri to make it unique. Check the PolicyId. */
        if(pol->policyId.length > token->policyId.length)
            continue;
        UA_String policyPrefix = token->policyId;
        policyPrefix.length = pol->policyId.length;
        if(!UA_String_equal(&policyPrefix, &pol->policyId))
            continue;

        /* Get the SecurityPolicy for the endpoint from the postfix */
        UA_String utPolPostfix = securityPolicyUriPostfix(token->policyId);
        UA_SecurityPolicy *candidateSp =
            getSecurityPolicyByPostfix(server, utPolPostfix);
        if(!candidateSp) {
            UA_LOG_WARNING_SESSION(server->config.logging, session,
                                   "ActivateSession: The UserTokenPolicy of "
                                   "the endpoint defines an unknown "
                                   "SecurityPolicy %S",
                                   pol->securityPolicyUri);
            continue;
        }

        /* A non-anonymous authentication token is transmitted over an
         * unencrypted SecureChannel */
        if(pol->tokenType != UA_USERTOKENTYPE_ANONYMOUS &&
           channel->securityPolicy->policyType == UA_SECURITYPOLICYTYPE_NONE &&
           candidateSp->policyType == UA_SECURITYPOLICYTYPE_NONE) {
            /* Check if the allowNonePolicyPassword option is set.
             * But this exception only works for Username/Password. */
            if(!server->config.allowNonePolicyPassword ||
               pol->tokenType != UA_USERTOKENTYPE_USERNAME)
                continue;
        }

        /* Found a match policy */
        *tokenSp = candidateSp;
        return pol;
    }

    return NULL;
}

static void
selectEndpointAndTokenPolicy(UA_Server *server, UA_SecureChannel *channel,
                             UA_Session *session,
                             const UA_ExtensionObject *identityToken,
                             const UA_EndpointDescription **ed,
                             const UA_UserTokenPolicy **utp,
                             UA_SecurityPolicy **tokenSp) {
    UA_ServerConfig *sc = &server->config;
    for(size_t i = 0; i < sc->endpointsSize; ++i) {
        const UA_EndpointDescription *desc = &sc->endpoints[i];

        /* Match the Security Mode */
        if(desc->securityMode != channel->securityMode)
            continue;

        /* Match the SecurityPolicy of the endpoint with the current channel */
        if(!UA_String_equal(&desc->securityPolicyUri,
                            &channel->securityPolicy->policyUri))
            continue;

        /* Select the UserTokenPolicy from the Endpoint */
        *utp = selectTokenPolicy(server, channel, session,
                                 identityToken, desc, tokenSp);
        if(*utp) {
            /* Match found */
            *ed = desc;
            return;
        }
    }
}

static UA_StatusCode
checkActivateSessionX509(UA_Server *server, UA_Session *session,
                         const UA_SecurityPolicy *sp, UA_X509IdentityToken* token,
                         const UA_SignatureData *tokenSignature) {
    /* The SecurityPolicy must not be None for the signature */
    if(sp->policyType == UA_SECURITYPOLICYTYPE_NONE)
        return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

    /* We need a channel context with the user certificate in order to reuse
     * the signature checking code. */
    void *tempChannelContext;
    UA_StatusCode res = sp->newChannelContext(sp, &token->certificateData,
                                              &tempChannelContext);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_SESSION(server->config.logging, session,
                               "ActivateSession: Failed to create a context "
                               "for the SecurityPolicy %S", sp->policyUri);
        return res;
    }

    /* Check the user token signature */
    res = checkCertificateSignature(server, sp, tempChannelContext,
                                    &session->serverNonce, tokenSignature, true);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_SESSION(server->config.logging, session,
                               "ActivateSession: User token signature check "
                               "failed with StatusCode %s", UA_StatusCode_name(res));
        goto out;
    }

    /* Validate the certificate against the SessionPKI */
    res = validateCertificate(server, &server->config.sessionPKI,
                              session->channel, session, NULL,
                              token->certificateData);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_SESSION(server->config.logging, session,
                               "ActivateSession: User token x509 certificate "
                               "did not validate");
    }

 out:
    /* Delete the temporary channel context */
    sp->deleteChannelContext(sp, tempChannelContext);
    return res;
}

/* TODO: Check all of the following: The Server shall verify that the
 * Certificate the Client used to create the new SecureChannel is the same as
 * the Certificate used to create the original SecureChannel. In addition, the
 * Server shall verify that the Client supplied a UserIdentityToken that is
 * identical to the token currently associated with the Session. Once the Server
 * accepts the new SecureChannel it shall reject requests sent via the old
 * SecureChannel. */

#define UA_SESSION_REJECT                                               \
    do {                                                                \
        server->serverDiagnosticsSummary.rejectedSessionCount++;        \
        return;                                                         \
    } while(0)

#define UA_SECURITY_REJECT                                              \
    do {                                                                \
        server->serverDiagnosticsSummary.securityRejectedSessionCount++; \
        server->serverDiagnosticsSummary.rejectedSessionCount++;        \
        return;                                                         \
    } while(0)

void
Service_ActivateSession(UA_Server *server, UA_SecureChannel *channel,
                        const UA_ActivateSessionRequest *req,
                        UA_ActivateSessionResponse *resp) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_ResponseHeader *rh = &resp->responseHeader;

    /* Get the session */
    UA_Session *session =
        getSessionByToken(server, &req->requestHeader.authenticationToken);
    if(!session) {
        UA_LOG_WARNING_CHANNEL(server->config.logging, channel,
                               "ActivateSession: Session not found");
        rh->serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;
        UA_SESSION_REJECT;
    }

    /* Part 4, §5.6.3: When the ActivateSession Service is called for the
     * first time then the Server shall reject the request if the
     * SecureChannel is not same as the one associated with the
     * CreateSession request. Subsequent calls to ActivateSession may be
     * associated with different SecureChannels. */
    if(!session->activated && session->channel != channel) {
        UA_LOG_WARNING_CHANNEL(server->config.logging, channel,
                               "ActivateSession: The Session has to be initially "
                               "activated on the SecureChannel that created it");
        rh->serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;
        UA_SESSION_REJECT;
    }

    /* Has the session timed out? */
    UA_EventLoop *el = server->config.eventLoop;
    UA_DateTime nowMonotonic = el->dateTime_nowMonotonic(el);
    if(session->validTill < nowMonotonic) {
        UA_LOG_WARNING_SESSION(server->config.logging, session,
                               "ActivateSession: The Session has timed out");
        rh->serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;
        UA_SESSION_REJECT;
    }

    /* Check the client signature */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        rh->serviceResult =
            checkCertificateSignature(server, channel->securityPolicy,
                                      channel->channelContext,
                                      &session->serverNonce,
                                      &req->clientSignature, false);
        if(rh->serviceResult != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING_SESSION(server->config.logging, session,
                                   "ActivateSession: Client signature check failed "
                                   "with StatusCode %s",
                                   UA_StatusCode_name(rh->serviceResult));
            UA_SECURITY_REJECT;
        }
    }

    /* Find the matching Endpoint with UserTokenPolicy.
     * Also sets the SecurityPolicy used to encrypt the token. */
    UA_ByteString applicationCert;
    const UA_EndpointDescription *ed = NULL;
    const UA_UserTokenPolicy *utp = NULL;
    UA_SecurityPolicy *tokenSp = NULL;
    selectEndpointAndTokenPolicy(server, channel, session,
                                 &req->userIdentityToken,
                                 &ed, &utp, &tokenSp);
    if(!ed || !tokenSp) {
        UA_LOG_WARNING_SESSION(server->config.logging, session,
                               "ActivateSession: Requested Endpoint/UserTokenPolicy "
                               "not available");
        rh->serviceResult = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
        UA_SESSION_REJECT;
    }

    /* Nothing to decrypt */
    if(utp->tokenType == UA_USERTOKENTYPE_ANONYMOUS)
        goto activate_session;

    /* Not encrypted */
    if(tokenSp->policyType == UA_SECURITYPOLICYTYPE_NONE) {
        if(channel->securityMode == UA_MESSAGESECURITYMODE_NONE) {
            UA_LOG_WARNING_SESSION(server->config.logging, session,
                                   "ActivateSession: Processing an unencrypted "
                                   "UserToken. This is dangerous for the server "
                                   "to allow.");
        }
        goto activate_session;
    }

    /* Ensure the session has an instance of the authentication SecurityPolicy
     * to decrypt the UserIdentityToken */
    applicationCert = (channel->remoteCertificate.length > 0) ?
        channel->remoteCertificate : session->clientCertificate;
    rh->serviceResult =
        createCheckSessionAuthSecurityPolicyContext(server, session, tokenSp,
                                                    applicationCert);
    if(rh->serviceResult != UA_STATUSCODE_GOOD)
        UA_SESSION_REJECT;

    /* SecurityPolicies with encryption always set a context */
    UA_assert(session->authSpContext);

    /* Decrypt (or validate the signature) of the UserToken. The DataType of the
     * UserToken was already checked in selectEndpointAndTokenPolicy. This
     * replaces the content of the token in-situ. */
    UA_ByteString *token = NULL;
    switch(utp->tokenType) {
    case UA_USERTOKENTYPE_USERNAME: {
        UA_UserNameIdentityToken *userToken = (UA_UserNameIdentityToken *)
           req->userIdentityToken.content.decoded.data;

        /* Test if the correct encryption algorithm is used */
        if(!UA_String_equal(&tokenSp->asymEncryptionAlgorithm.uri,
                            &userToken->encryptionAlgorithm)) {
            rh->serviceResult = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
            UA_SESSION_REJECT;
        }

        token = &userToken->password;
        break; }
    case UA_USERTOKENTYPE_CERTIFICATE: {
        /* If it is a X509IdentityToken, check the userTokenSignature. Note this
         * only validates that the user has the corresponding private key for
         * the given user certificate. Checking whether the user certificate is
         * trusted has to be implemented in the access control plugin. The
         * entire token is forwarded in the call to ActivateSession. */
        UA_X509IdentityToken* x509token = (UA_X509IdentityToken*)
            req->userIdentityToken.content.decoded.data;
        rh->serviceResult =
            checkActivateSessionX509(server, session, tokenSp,
                                     x509token, &req->userTokenSignature);
        goto activate_session; }
    case UA_USERTOKENTYPE_ISSUEDTOKEN: {
        UA_IssuedIdentityToken *issuedToken = (UA_IssuedIdentityToken*)
            req->userIdentityToken.content.decoded.data;

        /* Test if the correct encryption algorithm is used */
        if(!UA_String_equal(&tokenSp->asymEncryptionAlgorithm.uri,
                            &issuedToken->encryptionAlgorithm)) {
            rh->serviceResult = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
            UA_SESSION_REJECT;
        }

        token = &issuedToken->tokenData;
        break; }
    default:
        rh->serviceResult = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
        goto activate_session;
    }

    /* Decrypt the token. Differentiate between secret encryptions (ECC,
     * legacy).
     * TODO: Implement "modern" RSA* secret encryption */
    if(tokenSp->policyType == UA_SECURITYPOLICYTYPE_ECC) {
        rh->serviceResult =
            decryptUserTokenEcc(server->config.logging, channel,
                                session->authSp, session->authSpContext,
                                session->serverNonce, token);
    } else {
        rh->serviceResult =
            decryptSecretLegacy(session->authSp, session->authSpContext,
                                session->serverNonce, token);
    }

 activate_session:
    if(rh->serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_SESSION(server->config.logging, session,
                               "ActivateSession: Could not decrypt/"
                               "cryptographically validate the UserIdentityToken "
                               "with the StatusCode %s",
                               UA_StatusCode_name(rh->serviceResult));
        UA_SECURITY_REJECT;
    }

    /* Callback into userland access control */
    rh->serviceResult = server->config.accessControl.
        activateSession(server, &server->config.accessControl, ed,
                        &channel->remoteCertificate, &session->sessionId,
                        &req->userIdentityToken, &session->context);
    if(rh->serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_SESSION(server->config.logging, session,
                               "ActivateSession: The AccessControl "
                               "plugin denied the activation with the StatusCode %s",
                               UA_StatusCode_name(rh->serviceResult));
        UA_SECURITY_REJECT;
    }

    /* Attach the session to the currently used channel if the session isn't
     * attached to a channel or if the session is activated on a different
     * channel than it is attached to. */
    if(!session->channel || session->channel != channel) {
        /* Attach the new SecureChannel, the old channel will be detached if present */
        UA_Session_attachToSecureChannel(server, session, channel);
        UA_LOG_INFO_SESSION(server->config.logging, session,
                            "ActivateSession: Session attached to new channel");
    }

    /* Generate a new session nonce for the next time ActivateSession is called */
    rh->serviceResult = UA_Session_generateNonce(session);
    rh->serviceResult |= UA_ByteString_copy(&session->serverNonce,
                                            &resp->serverNonce);
    if(rh->serviceResult != UA_STATUSCODE_GOOD) {
        UA_Session_detachFromSecureChannel(server, session);
        UA_LOG_WARNING_SESSION(server->config.logging, session,
                               "ActivateSession: Could not generate the server nonce");
        UA_SESSION_REJECT;
    }

    /* Set the Locale */
    if(req->localeIdsSize > 0) {
        /* Part 4, §5.6.3.2: This parameter only needs to be specified during
         * the first call to ActivateSession during a single application
         * Session. If it is not specified the Server shall keep using the
         * current localeIds for the Session. */
        UA_String *tmpLocaleIds;
        rh->serviceResult |= UA_Array_copy(req->localeIds, req->localeIdsSize,
                                           (void**)&tmpLocaleIds,
                                           &UA_TYPES[UA_TYPES_STRING]);
        if(rh->serviceResult != UA_STATUSCODE_GOOD) {
            UA_Session_detachFromSecureChannel(server, session);
            UA_LOG_WARNING_SESSION(server->config.logging, session,
                                   "ActivateSession: Could not store the "
                                   "Session LocaleIds");
            UA_SESSION_REJECT;
        }
        UA_Array_delete(session->localeIds, session->localeIdsSize,
                        &UA_TYPES[UA_TYPES_STRING]);
        session->localeIds = tmpLocaleIds;
        session->localeIdsSize = req->localeIdsSize;
    }

    /* Update the Session lifetime */
    UA_DateTime now = el->dateTime_now(el);
    nowMonotonic = el->dateTime_nowMonotonic(el);
    UA_Session_updateLifetime(session, now, nowMonotonic);

    /* If ECC policy, create the new ephemeral key to be returned in the
     * ActivateSession response */
    const UA_SecurityPolicy *authSp = session->authSp;
    if(authSp && session->authSpContext &&
       authSp->policyType == UA_SECURITYPOLICYTYPE_ECC) {
        rh->serviceResult = addEphemeralKeyAdditionalHeader(server, session,
                                                            &rh->additionalHeader);
        if(rh->serviceResult != UA_STATUSCODE_GOOD)
            UA_SECURITY_REJECT;
    }

    /* Activate the session */
    if(!session->activated) {
        session->activated = true;
        server->activeSessionCount++;
        server->serverDiagnosticsSummary.cumulatedSessionCount++;
    }

    /* Store the ClientUserId. tokenType can be NULL for the anonymous user. */
    UA_String_clear(&session->clientUserIdOfSession);
    const UA_DataType *tokenType = req->userIdentityToken.content.decoded.type;
    if(tokenType == &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN]) {
        const UA_UserNameIdentityToken *userToken = (UA_UserNameIdentityToken*)
            req->userIdentityToken.content.decoded.data;
        UA_String_copy(&userToken->userName, &session->clientUserIdOfSession);
    } else if(tokenType == &UA_TYPES[UA_TYPES_X509IDENTITYTOKEN]) {
        UA_X509IdentityToken* userCertToken = (UA_X509IdentityToken*)
            req->userIdentityToken.content.decoded.data;
        UA_CertificateUtils_getSubjectName(&session->clientUserIdOfSession,
                                           &userCertToken->certificateData);
    } else {
        /* TODO: Handle issued token */
    }

#ifdef UA_ENABLE_DIAGNOSTICS
    /* Add the ClientUserId to the diagnostics history. Ignoring errors in _appendCopy. */
    UA_SessionSecurityDiagnosticsDataType *ssd = &session->securityDiagnostics;
    UA_Array_appendCopy((void**)&ssd->clientUserIdHistory,
                        &ssd->clientUserIdHistorySize,
                        &ssd->clientUserIdOfSession,
                        &UA_TYPES[UA_TYPES_STRING]);

    /* Store the auth mechanism */
    UA_String_clear(&ssd->authenticationMechanism);
    switch(utp->tokenType) {
    case UA_USERTOKENTYPE_ANONYMOUS:
        ssd->authenticationMechanism = UA_STRING_ALLOC("Anonymous"); break;
    case UA_USERTOKENTYPE_USERNAME:
        ssd->authenticationMechanism = UA_STRING_ALLOC("UserName"); break;
    case UA_USERTOKENTYPE_CERTIFICATE:
        ssd->authenticationMechanism = UA_STRING_ALLOC("Certificate"); break;
    case UA_USERTOKENTYPE_ISSUEDTOKEN:
        ssd->authenticationMechanism = UA_STRING_ALLOC("IssuedToken"); break;
    default: break;
    }
#endif

    /* Notify the application */
    notifySession(server, session, UA_APPLICATIONNOTIFICATIONTYPE_SESSION_ACTIVATED);

    /* Log the user for which the Session was activated */
    UA_LOG_INFO_SESSION(server->config.logging, session,
                        "ActivateSession: Session activated with ClientUserId \"%S\"",
                        session->clientUserIdOfSession);
}

void
Service_CloseSession(UA_Server *server, UA_SecureChannel *channel,
                     const UA_CloseSessionRequest *request,
                     UA_CloseSessionResponse *response) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_ResponseHeader *rh = &response->responseHeader;

    /* Part 4, 5.6.4: When the CloseSession Service is called before the Session
     * is successfully activated, the Server shall reject the request if the
     * SecureChannel is not the same as the one associated with the
     * CreateSession request.
     *
     * A non-activated Session is already bound to the SecureChannel that
     * created the Session. */
    UA_Session *session = NULL;
    const UA_NodeId *authToken = &request->requestHeader.authenticationToken;
    rh->serviceResult = getBoundSession(server, channel, authToken, &session);
    if(!session && rh->serviceResult == UA_STATUSCODE_GOOD)
        rh->serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;
    if(rh->serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_CHANNEL(server->config.logging, channel,
                               "CloseSession: No Session activated to the SecureChannel");
        return;
    }

    UA_assert(session); /* Assured by the previous section */
    UA_LOG_INFO_SESSION(server->config.logging, session, "Closing the Session");

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* If Subscriptions are not deleted, detach them from the Session */
    if(!request->deleteSubscriptions) {
        UA_Subscription *sub, *sub_tmp;
        TAILQ_FOREACH_SAFE(sub, &session->subscriptions, sessionListEntry, sub_tmp) {
            UA_LOG_INFO_SUBSCRIPTION(server->config.logging, sub,
                                     "Detaching the Subscription from the Session");
            UA_Session_detachSubscription(server, session, sub, true);
        }
    }
#endif

    /* Remove the sesison */
    UA_Session_remove(server, session, UA_SHUTDOWNREASON_CLOSE);
}

UA_Boolean
Service_Cancel(UA_Server *server, UA_Session *session,
               const UA_CancelRequest *request, UA_CancelResponse *response) {
    /* If multithreading is disabled, then there are no async services. If all
     * services are answered "right away", then there are no services that can
     * be cancelled. */
    response->cancelCount = UA_AsyncManager_cancel(server, session,
                                                   request->requestHandle);

    /* Publish requests for Subscriptions are stored separately */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_PublishResponseEntry *pre, *pre_tmp;
    UA_PublishResponseEntry *prev = NULL;
    SIMPLEQ_FOREACH_SAFE(pre, &session->responseQueue, listEntry, pre_tmp) {
        /* Skip entry and set as the previous entry that is kept in the list */
        if(pre->response.responseHeader.requestHandle != request->requestHandle) {
            prev = pre;
            continue;
        }

        /* Dequeue */
        if(prev)
            SIMPLEQ_REMOVE_AFTER(&session->responseQueue, prev, listEntry);
        else
            SIMPLEQ_REMOVE_HEAD(&session->responseQueue, listEntry);
        session->responseQueueSize--;

        /* Send response and clean up */
        response->responseHeader.serviceResult = UA_STATUSCODE_BADREQUESTCANCELLEDBYCLIENT;
        sendResponse(server, session->channel, pre->requestId, (UA_Response *)response,
                     &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]);
        UA_PublishResponse_clear(&pre->response);
        UA_free(pre);

        /* Increase the CancelCount */
        response->cancelCount++;
    }
#endif

    return true;
}
