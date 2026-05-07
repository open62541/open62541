/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include "ua_server_internal.h"

#ifdef UA_ENABLE_AUDITING

/* The channel must be non-NULL. The session can be NULL.
 *
 * The first five entries of the payload-maps must be /ActionTimeStamp, /Status,
 * /ServerId, /ClientAuditEntryId and /ClientUserId. These field values are set
 * internally. The channel and session pointer can be NULL if none is defined
 * for the current context. */
static void
auditEvent(UA_Server *server, UA_ApplicationNotificationType type,
           UA_SecureChannel *channel, UA_Session *session,
           const UA_NodeId *sourceNode, const char *serviceName,
           UA_Boolean status, const UA_KeyValueMap payload) {
    UA_ServerConfig *config = &server->config;

    /* Check if auditing is disabled */
    if(!config->auditingEnabled)
        return;

    /* Set the values for AuditEventType fields:
     * /ActionTimeStamp    -> 0
     * /Status             -> 1
     * /ServerId           -> 2
     * /ClientAuditEntryId -> 3
     * /ClientUserId       -> 4 */

    UA_UInt32 channelId = (channel) ? channel->securityToken.channelId : 0;
    UA_NodeId sessionId = (session) ? session->sessionId : UA_NODEID_NULL;
    UA_Byte entryIdBuf[521];
    UA_String auditEntryId = {512, entryIdBuf};
    UA_String_format(&auditEntryId, "%u:%N:%s", channelId, sessionId, serviceName);
    UA_String clientUserId = (session) ?
        session->clientUserIdOfSession : UA_STRING_NULL;
    UA_DateTime actionTimestamp =
        config->eventLoop->dateTime_now(config->eventLoop);

    UA_Variant_setScalar(&payload.map[0].value, &actionTimestamp,
                         &UA_TYPES[UA_TYPES_DATETIME]);
    UA_Variant_setScalar(&payload.map[1].value, &status,
                         &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Variant_setScalar(&payload.map[2].value,
                         &config->applicationDescription.applicationUri,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&payload.map[3].value, &auditEntryId,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&payload.map[4].value, &clientUserId,
                         &UA_TYPES[UA_TYPES_STRING]);

    /* Call the server notification callback */
    if(config->auditNotificationCallback)
        config->auditNotificationCallback(server, type, payload);
    if(config->globalNotificationCallback)
        config->globalNotificationCallback(server, type, payload);

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    /* Create the Event in the information model */
    UA_EventDescription ed;
    memset(&ed, 0, sizeof(UA_EventDescription));
    ed.sourceNode = (sourceNode) ? *sourceNode : UA_NS0ID(SERVER);
    ed.eventFields = &payload;
    switch(type) {
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT:
        ed.eventType = UA_NS0ID(AUDITEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY:
        ed.eventType = UA_NS0ID(AUDITSECURITYEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CHANNEL:
        ed.eventType = UA_NS0ID(AUDITCHANNELEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CHANNEL_OPEN:
        ed.eventType = UA_NS0ID(AUDITOPENSECURECHANNELEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_SESSION:
        ed.eventType = UA_NS0ID(AUDITSESSIONEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_SESSION_CREATE:
        ed.eventType = UA_NS0ID(AUDITCREATESESSIONEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_SESSION_ACTIVATE:
        ed.eventType = UA_NS0ID(AUDITACTIVATESESSIONEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_SESSION_CANCEL:
        ed.eventType = UA_NS0ID(AUDITCANCELEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CERTIFICATE:
        ed.eventType = UA_NS0ID(AUDITCERTIFICATEEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CERTIFICATE_DATAMISMATCH:
        ed.eventType = UA_NS0ID(AUDITCERTIFICATEDATAMISMATCHEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CERTIFICATE_EXPIRED:
        ed.eventType = UA_NS0ID(AUDITCERTIFICATEEXPIREDEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CERTIFICATE_INVALID:
        ed.eventType = UA_NS0ID(AUDITCERTIFICATEINVALIDEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CERTIFICATE_UNTRUSTED:
        ed.eventType = UA_NS0ID(AUDITCERTIFICATEUNTRUSTEDEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CERTIFICATE_REVOKED:
        ed.eventType = UA_NS0ID(AUDITCERTIFICATEREVOKEDEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CERTIFICATE_MISMATCH:
        ed.eventType = UA_NS0ID(AUDITCERTIFICATEMISMATCHEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_NODE:
        ed.eventType = UA_NS0ID(AUDITNODEMANAGEMENTEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_NODE_ADD:
        ed.eventType = UA_NS0ID(AUDITADDNODESEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_NODE_DELETE:
        ed.eventType = UA_NS0ID(AUDITDELETENODESEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_NODE_ADDREFERENCES:
        ed.eventType = UA_NS0ID(AUDITADDREFERENCESEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_NODE_DELETEREFERENCES:
        ed.eventType = UA_NS0ID(AUDITDELETEREFERENCESEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_UPDATE:
        ed.eventType = UA_NS0ID(AUDITUPDATEEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_UPDATE_WRITE:
        ed.eventType = UA_NS0ID(AUDITWRITEUPDATEEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_UPDATE_HISTORY:
        ed.eventType = UA_NS0ID(AUDITHISTORYUPDATEEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_UPDATE_METHOD:
        ed.eventType = UA_NS0ID(AUDITUPDATEMETHODEVENTTYPE); break;
    default:
        /* TODO:
         * UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_CLIENT                            = 0x1800,
         * UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_CLIENT_UPDATEMETHOD               = 0x1810
        */
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "Unsupported audit log type requested: %u", type);
        return;
    }
    createEvent(server, &ed, NULL);
 #endif
}

/* In addition to auditEvent, the sixth entry of the payload-map must be /StatusCodeId */
static void
auditSecurityEvent(UA_Server *server, UA_ApplicationNotificationType type,
                   UA_SecureChannel *channel, UA_Session *session,
                   const char *serviceName, UA_Boolean status, UA_StatusCode statusCodeId,
                   const UA_KeyValueMap payload) {
    /* /StatusCodeId */
    UA_Variant_setScalar(&payload.map[5].value, &statusCodeId,
                         &UA_TYPES[UA_TYPES_STATUSCODE]);

    auditEvent(server, type, channel, session, NULL, serviceName, status, payload);
}

/* In addition to auditSecurityEvent, the seventh entry of the payload-map must be
 * /SecureChannelId and the eighth entry must be /SourceName. */
static void
auditChannelEvent(UA_Server *server, UA_ApplicationNotificationType type,
                  UA_SecureChannel *channel, UA_Session *session, const char *serviceName,
                  UA_Boolean status, UA_StatusCode statusCodeId,
                  const UA_KeyValueMap payload) {
    /* /SecureChannelId */
    UA_Byte secureChannelNameBuf[32];
    UA_String secureChannelName = {32, secureChannelNameBuf};
    UA_String_format(&secureChannelName, "%lu",
                     (long unsigned)channel->securityToken.channelId);
    UA_Variant_setScalar(&payload.map[6].value, &secureChannelName,
                         &UA_TYPES[UA_TYPES_STRING]);

    /* SourceName */
    UA_Byte sourceNameBuf[128];
    UA_String sourceName = {128, sourceNameBuf};
    UA_String_format(&sourceName, "SecureChannel/%s", serviceName);
    UA_Variant_setScalar(&payload.map[7].value, &sourceName,
                         &UA_TYPES[UA_TYPES_STRING]);

    auditSecurityEvent(server, type, channel, session, serviceName, status,
                       statusCodeId, payload);
}

void
auditOpenSecureChannelEvent(UA_Server *server, UA_SecureChannel *channel,
                            const UA_OpenSecureChannelRequest *req,
                            const UA_OpenSecureChannelResponse *resp) {
    static UA_THREAD_LOCAL UA_KeyValuePair channelAuditPayload[14] = {
        {{0, UA_STRING_STATIC("/ActionTimeStamp")}, {0}},             /* 0 */
        {{0, UA_STRING_STATIC("/Status")}, {0}},                      /* 1 */
        {{0, UA_STRING_STATIC("/ServerId")}, {0}},                    /* 2 */
        {{0, UA_STRING_STATIC("/ClientAuditEntryId")}, {0}},          /* 3 */
        {{0, UA_STRING_STATIC("/ClientUserId")}, {0}},                /* 4 */
        {{0, UA_STRING_STATIC("/StatusCodeId")}, {0}},                /* 5 */
        {{0, UA_STRING_STATIC("/SecureChannelId")}, {0}},             /* 6 */
        {{0, UA_STRING_STATIC("/SourceName")}, {0}},                  /* 7 */
        {{0, UA_STRING_STATIC("/ClientCertificate")}, {0}},           /* 8 */
        {{0, UA_STRING_STATIC("/ClientCertificateThumbprint")}, {0}}, /* 9 */
        {{0, UA_STRING_STATIC("/RequestType")}, {0}},                 /* 10 */
        {{0, UA_STRING_STATIC("/SecurityPolicyUri")}, {0}},           /* 11 */
        {{0, UA_STRING_STATIC("/SecurityMode")}, {0}},                /* 12 */
        {{0, UA_STRING_STATIC("/RequestedLifetime")}, {0}},           /* 13 */
    };

    const UA_SecurityPolicy *sp = channel->securityPolicy;
    UA_Boolean status = (resp->responseHeader.serviceResult == UA_STATUSCODE_GOOD);
    UA_ByteString certThumbprint = {20, channel->remoteCertificateThumbprint};
    UA_Variant_setScalar(&channelAuditPayload[8].value, &channel->remoteCertificate,
                         &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Variant_setScalar(&channelAuditPayload[9].value, &certThumbprint,
                         &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Variant_setScalar(&channelAuditPayload[10].value, (void*)(uintptr_t)&req->requestType,
                         &UA_TYPES[UA_TYPES_SECURITYTOKENREQUESTTYPE]);
    UA_Variant_setScalar(&channelAuditPayload[11].value, (void*)(uintptr_t)&sp->policyUri,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&channelAuditPayload[12].value, (void*)(uintptr_t)&req->securityMode,
                         &UA_TYPES[UA_TYPES_MESSAGESECURITYMODE]);
    UA_Variant_setScalar(&channelAuditPayload[13].value, (void*)(uintptr_t)&req->requestedLifetime,
                         &UA_TYPES[UA_TYPES_UINT32]);

    UA_KeyValueMap payload = {14, channelAuditPayload};
    auditChannelEvent(server, UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CHANNEL_OPEN,
                      channel, NULL, "OpenSecureChannel", status,
                      resp->responseHeader.serviceResult, payload);
}

void
auditCloseSecureChannelEvent(UA_Server *server, UA_SecureChannel *channel) {
    static UA_THREAD_LOCAL UA_KeyValuePair closeAuditPayload[8] = {
        {{0, UA_STRING_STATIC("/ActionTimeStamp")}, {0}},             /* 0 */
        {{0, UA_STRING_STATIC("/Status")}, {0}},                      /* 1 */
        {{0, UA_STRING_STATIC("/ServerId")}, {0}},                    /* 2 */
        {{0, UA_STRING_STATIC("/ClientAuditEntryId")}, {0}},          /* 3 */
        {{0, UA_STRING_STATIC("/ClientUserId")}, {0}},                /* 4 */
        {{0, UA_STRING_STATIC("/StatusCodeId")}, {0}},                /* 5 */
        {{0, UA_STRING_STATIC("/SecureChannelId")}, {0}},             /* 6 */
        {{0, UA_STRING_STATIC("/SourceName")}, {0}},                  /* 7 */
    };

    UA_KeyValueMap payload = {14, closeAuditPayload};
    auditChannelEvent(server, UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CHANNEL,
                      channel, NULL, "CloseSecureChannel", true,
                      UA_STATUSCODE_GOOD, payload);
}

/* In addition to auditSecurityEvent, the seventh and eighth entry of the
 * payload-map must be /SessionId and /SourceName */
static void
auditSessionEvent(UA_Server *server, UA_ApplicationNotificationType type,
                  UA_SecureChannel *channel, UA_Session *session,
                  const char *serviceName, UA_Boolean status,
                  UA_StatusCode statusCodeId, const UA_KeyValueMap payload) {
    /* /SessionId */
    if(session)
        UA_Variant_setScalar(&payload.map[6].value, &session->sessionId,
                             &UA_TYPES[UA_TYPES_NODEID]);
    else
        UA_Variant_init(&payload.map[6].value);

    /* /SourceName */
    UA_Byte sourceNameBuf[128];
    UA_String sourceName = {128, sourceNameBuf};
    UA_String_format(&sourceName, "Session/%s", serviceName);
    UA_Variant_setScalar(&payload.map[7].value, &sourceName,
                         &UA_TYPES[UA_TYPES_STRING]);

    auditSecurityEvent(server, type, channel, session, serviceName, status,
                       statusCodeId, payload);
}

void
auditCreateSessionEvent(UA_Server *server, UA_SecureChannel *channel, UA_Session *session,
                        const UA_CreateSessionRequest *req, const UA_CreateSessionResponse *resp) {
    static UA_THREAD_LOCAL UA_KeyValuePair sessionCreateAuditPayload[12] = {
        {{0, UA_STRING_STATIC("/ActionTimeStamp")}, {0}},             /* 0 */
        {{0, UA_STRING_STATIC("/Status")}, {0}},                      /* 1 */
        {{0, UA_STRING_STATIC("/ServerId")}, {0}},                    /* 2 */
        {{0, UA_STRING_STATIC("/ClientAuditEntryId")}, {0}},          /* 3 */
        {{0, UA_STRING_STATIC("/ClientUserId")}, {0}},                /* 4 */
        {{0, UA_STRING_STATIC("/StatusCodeId")}, {0}},                /* 5 */
        {{0, UA_STRING_STATIC("/SessionId")}, {0}},                   /* 6 */
        {{0, UA_STRING_STATIC("/SourceName")}, {0}},                  /* 7 */
        {{0, UA_STRING_STATIC("/SecureChannelId")}, {0}},             /* 8 */
        {{0, UA_STRING_STATIC("/ClientCertificate")}, {0}},           /* 9 */
        {{0, UA_STRING_STATIC("/ClientCertificateThumbprint")}, {0}}, /* 10 */
        {{0, UA_STRING_STATIC("/RevisedSessionTimeout")}, {0}}        /* 11 */
    };

    /* /SecureChannelId */
    UA_Byte secureChannelNameBuf[32];
    UA_String secureChannelName = {32, secureChannelNameBuf};
    UA_String_format(&secureChannelName, "%lu",
                     (long unsigned)channel->securityToken.channelId);
    UA_Variant_setScalar(&sessionCreateAuditPayload[8].value, &secureChannelName,
                         &UA_TYPES[UA_TYPES_STRING]);

    /* /ClientCertificate */
    UA_Variant_setScalar(&sessionCreateAuditPayload[9].value,
                         (void*)(uintptr_t)&req->clientCertificate, &UA_TYPES[UA_TYPES_BYTESTRING]);

    UA_SecurityPolicy *sp = channel->securityPolicy;
    UA_assert(sp);

    /* /ClientCertificateThumbprint
     *
     * Upon success we can take the certificate thumbprint from the
     * SecureChannel. Because we know it must be the same as the client
     * certificate from the CreateSession request. And this is checked in
     * _CreateSession. */
    UA_ByteString certThumbprint;
    UA_Boolean status = (resp->responseHeader.serviceResult == UA_STATUSCODE_GOOD);
    if(status == true) {
        certThumbprint.data = channel->remoteCertificateThumbprint;
        certThumbprint.length = 20;
    } else {
        UA_ByteString_init(&certThumbprint);
        sp->makeCertThumbprint(sp, &req->clientCertificate, &certThumbprint); /* Ignore error */
    }
    UA_Variant_setScalar(&sessionCreateAuditPayload[10].value, &certThumbprint,
                         &UA_TYPES[UA_TYPES_BYTESTRING]);

    /* /ReviseSessionTimeout */
    if(session)
        UA_Variant_setScalar(&sessionCreateAuditPayload[11].value, &session->timeout,
                             &UA_TYPES[UA_TYPES_DOUBLE]);
    else
        UA_Variant_init(&sessionCreateAuditPayload[11].value);

    UA_KeyValueMap payload = {12, sessionCreateAuditPayload};
    auditSessionEvent(server,
                      UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_SESSION_CREATE,
                      channel, session, "CreateSession",
                      status, resp->responseHeader.serviceResult, payload);
    if(status != true)
        UA_ByteString_clear(&certThumbprint);
}

void
auditActivateSessionEvent(UA_Server *server,
                          UA_SecureChannel *channel, UA_Session *session,
                          const UA_ActivateSessionRequest *req,
                          const UA_ActivateSessionResponse *resp) {
    static UA_THREAD_LOCAL UA_KeyValuePair sessionActivateAuditPayload[11] = {
        {{0, UA_STRING_STATIC("/ActionTimeStamp")}, {0}},             /* 0 */
        {{0, UA_STRING_STATIC("/Status")}, {0}},                      /* 1 */
        {{0, UA_STRING_STATIC("/ServerId")}, {0}},                    /* 2 */
        {{0, UA_STRING_STATIC("/ClientAuditEntryId")}, {0}},          /* 3 */
        {{0, UA_STRING_STATIC("/ClientUserId")}, {0}},                /* 4 */
        {{0, UA_STRING_STATIC("/StatusCodeId")}, {0}},                /* 5 */
        {{0, UA_STRING_STATIC("/SessionId")}, {0}},                   /* 6 */
        {{0, UA_STRING_STATIC("/SourceName")}, {0}},                  /* 7 */
        {{0, UA_STRING_STATIC("/ClientSoftwareCertificates")}, {0}},  /* 8 */
        {{0, UA_STRING_STATIC("/UserIdentityToken")}, {0}},           /* 9 */
        {{0, UA_STRING_STATIC("/SecureChannelId")}, {0}}              /* 10 */
    };

    /* /ClientSoftwareCertificates */
    UA_Variant_setArray(&sessionActivateAuditPayload[8].value,
                        req->clientSoftwareCertificates,
                        req->clientSoftwareCertificatesSize,
                        &UA_TYPES[UA_TYPES_SIGNEDSOFTWARECERTIFICATE]);

    /* /UserIdentityToken */
    if(req->userIdentityToken.encoding == UA_EXTENSIONOBJECT_DECODED ||
       req->userIdentityToken.encoding == UA_EXTENSIONOBJECT_DECODED_NODELETE) {
        const UA_ExtensionObject *uit = &req->userIdentityToken;
        UA_Variant_setScalar(&sessionActivateAuditPayload[9].value, uit->content.decoded.data,
                             uit->content.decoded.type);
    } else {
        UA_Variant_init(&sessionActivateAuditPayload[9].value);
    }

    /* /SecureChannelId */
    UA_Byte secureChannelNameBuf[32];
    UA_String secureChannelName = {32, secureChannelNameBuf};
    UA_String_format(&secureChannelName, "%lu",
                     (long unsigned)channel->securityToken.channelId);
    UA_Variant_setScalar(&sessionActivateAuditPayload[10].value, &secureChannelName,
                         &UA_TYPES[UA_TYPES_STRING]);

    UA_KeyValueMap payload = {11, sessionActivateAuditPayload};
    auditSessionEvent(server, UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_SESSION_ACTIVATE,
                      channel, session, "ActivateSession",
                      (resp->responseHeader.serviceResult == UA_STATUSCODE_GOOD),
                      resp->responseHeader.serviceResult, payload);
}

void
auditCloseSessionEvent(UA_Server *server, UA_Session *session) {
    static UA_THREAD_LOCAL UA_KeyValuePair sessionCloseAuditPayload[8] = {
        {{0, UA_STRING_STATIC("/ActionTimeStamp")}, {0}},             /* 0 */
        {{0, UA_STRING_STATIC("/Status")}, {0}},                      /* 1 */
        {{0, UA_STRING_STATIC("/ServerId")}, {0}},                    /* 2 */
        {{0, UA_STRING_STATIC("/ClientAuditEntryId")}, {0}},          /* 3 */
        {{0, UA_STRING_STATIC("/ClientUserId")}, {0}},                /* 4 */
        {{0, UA_STRING_STATIC("/StatusCodeId")}, {0}},                /* 5 */
        {{0, UA_STRING_STATIC("/SessionId")}, {0}},                   /* 6 */
        {{0, UA_STRING_STATIC("/SourceName")}, {0}},                  /* 7 */
    };

    UA_KeyValueMap payload = {8, sessionCloseAuditPayload};
    auditSessionEvent(server, UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_SESSION,
                      session->channel, session, "CloseSession", true, UA_STATUSCODE_GOOD, payload);
}

void
auditCancelEvent(UA_Server *server, UA_SecureChannel *channel, UA_Session *session,
                 UA_Boolean status, UA_StatusCode statusCodeId, UA_UInt32 requestHandle) {
    static UA_THREAD_LOCAL UA_KeyValuePair sessionCancelAuditPayload[9] = {
        {{0, UA_STRING_STATIC("/ActionTimeStamp")}, {0}},             /* 0 */
        {{0, UA_STRING_STATIC("/Status")}, {0}},                      /* 1 */
        {{0, UA_STRING_STATIC("/ServerId")}, {0}},                    /* 2 */
        {{0, UA_STRING_STATIC("/ClientAuditEntryId")}, {0}},          /* 3 */
        {{0, UA_STRING_STATIC("/ClientUserId")}, {0}},                /* 4 */
        {{0, UA_STRING_STATIC("/StatusCodeId")}, {0}},                /* 5 */
        {{0, UA_STRING_STATIC("/SessionId")}, {0}},                   /* 6 */
        {{0, UA_STRING_STATIC("/SourceName")}, {0}},                  /* 7 */
        {{0, UA_STRING_STATIC("/RequestHandle")}, {0}}                /* 8 */
    };

    /* /RequestHandle */
    UA_Variant_setScalar(&sessionCancelAuditPayload[8].value, &requestHandle,
                         &UA_TYPES[UA_TYPES_UINT32]);

    UA_KeyValueMap payloadMap = {9, sessionCancelAuditPayload};
    auditSessionEvent(server, UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_SESSION_CANCEL,
                      channel, session, "Cancel", status, statusCodeId, payloadMap);
}

static void
auditCertificateEvent_internal(UA_Server *server, UA_ApplicationNotificationType type,
                               UA_SecureChannel *channel, UA_Session *session,
                               const char *serviceName, UA_Boolean status,
                               UA_StatusCode statusCodeId, UA_ByteString certificate,
                               const UA_KeyValueMap payload) {
    /* /Certificate */
    UA_Variant_setScalar(&payload.map[6].value, &certificate,
                         &UA_TYPES[UA_TYPES_BYTESTRING]);
    /* /SourceName */
    UA_String sourceName = UA_STRING("Security/Certificate");
    UA_Variant_setScalar(&payload.map[7].value, &sourceName,
                         &UA_TYPES[UA_TYPES_STRING]);

    auditSecurityEvent(server, type, channel, session, serviceName, status,
                       statusCodeId, payload);
}

void
auditCertificateDataMismatchEvent(UA_Server *server,
                                  UA_SecureChannel *channel, UA_Session *session,
                                  const char *serviceName, UA_StatusCode statusCodeId,
                                  UA_ByteString certificate, UA_String invalidUri) {
    static UA_THREAD_LOCAL UA_KeyValuePair mismatchAuditPayload[9] = {
        {{0, UA_STRING_STATIC("/ActionTimeStamp")}, {0}},             /* 0 */
        {{0, UA_STRING_STATIC("/Status")}, {0}},                      /* 1 */
        {{0, UA_STRING_STATIC("/ServerId")}, {0}},                    /* 2 */
        {{0, UA_STRING_STATIC("/ClientAuditEntryId")}, {0}},          /* 3 */
        {{0, UA_STRING_STATIC("/ClientUserId")}, {0}},                /* 4 */
        {{0, UA_STRING_STATIC("/StatusCodeId")}, {0}},                /* 5 */
        {{0, UA_STRING_STATIC("/Certificate")}, {0}},                 /* 6 */
        {{0, UA_STRING_STATIC("/SourceName")}, {0}},                  /* 7 */
        {{0, UA_STRING_STATIC("/InvalidUri")}, {0}},                  /* 8*/
    };

    /* /InvalidUri */
    UA_Variant_setScalar(&mismatchAuditPayload[8].value, &invalidUri,
                         &UA_TYPES[UA_TYPES_STRING]);

    UA_KeyValueMap payload = {9, mismatchAuditPayload};
    auditCertificateEvent_internal(server,
                                   UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CERTIFICATE_DATAMISMATCH,
                                   channel, session, serviceName, (statusCodeId == UA_STATUSCODE_GOOD),
                                   statusCodeId, certificate, payload);
}

void
auditCertificateEvent(UA_Server *server, UA_ApplicationNotificationType type,
                      UA_SecureChannel *channel, UA_Session *session,
                      const char *serviceName, UA_StatusCode statusCodeId,
                      UA_ByteString certificate, UA_String message) {
    static UA_THREAD_LOCAL UA_KeyValuePair certAuditPayload[9] = {
        {{0, UA_STRING_STATIC("/ActionTimeStamp")}, {0}},             /* 0 */
        {{0, UA_STRING_STATIC("/Status")}, {0}},                      /* 1 */
        {{0, UA_STRING_STATIC("/ServerId")}, {0}},                    /* 2 */
        {{0, UA_STRING_STATIC("/ClientAuditEntryId")}, {0}},          /* 3 */
        {{0, UA_STRING_STATIC("/ClientUserId")}, {0}},                /* 4 */
        {{0, UA_STRING_STATIC("/StatusCodeId")}, {0}},                /* 5 */
        {{0, UA_STRING_STATIC("/Certificate")}, {0}},                 /* 6 */
        {{0, UA_STRING_STATIC("/SourceName")}, {0}},                  /* 7 */
        {{0, UA_STRING_STATIC("/Message")}, {0}},                     /* 8 */
    };

    /* /Message */
    UA_Variant_setScalar(&certAuditPayload[8].value, &message, &UA_TYPES[UA_TYPES_STRING]);

    UA_KeyValueMap payload = {9, certAuditPayload};
    auditCertificateEvent_internal(server, type, channel, session, serviceName,
                                   (statusCodeId == UA_STATUSCODE_GOOD),
                                   statusCodeId, certificate, payload);
}

static void
auditNodeManagementEvent(UA_Server *server, UA_ApplicationNotificationType type,
                         UA_SecureChannel *channel, UA_Session *session,
                         const char *serviceName, UA_Boolean status,
                         const UA_KeyValueMap payload) {
    /* /SourceName */
    UA_Byte sourceNameBuf[128];
    UA_String sourceName = {128, sourceNameBuf};
    UA_String_format(&sourceName, "NodeManagement/%s", serviceName);
    UA_Variant_setScalar(&payload.map[5].value, &sourceName,
                         &UA_TYPES[UA_TYPES_STRING]);

    auditEvent(server, type, channel, session, NULL, serviceName, status, payload);
}

void
auditAddNodesEvent(UA_Server *server, UA_SecureChannel *channel, UA_Session *session,
                   UA_Boolean status, size_t itemsSize, UA_AddNodesItem *items) {
    static UA_THREAD_LOCAL UA_KeyValuePair addNodesPayload[7] = {
        {{0, UA_STRING_STATIC("/ActionTimeStamp")}, {0}},             /* 0 */
        {{0, UA_STRING_STATIC("/Status")}, {0}},                      /* 1 */
        {{0, UA_STRING_STATIC("/ServerId")}, {0}},                    /* 2 */
        {{0, UA_STRING_STATIC("/ClientAuditEntryId")}, {0}},          /* 3 */
        {{0, UA_STRING_STATIC("/ClientUserId")}, {0}},                /* 4 */
        {{0, UA_STRING_STATIC("/SourceName")}, {0}},                  /* 5 */
        {{0, UA_STRING_STATIC("/NodesToAdd")}, {0}}                   /* 6 */
    };

    /* /NodesToAdd */
    UA_Variant_setArray(&addNodesPayload[6].value, items, itemsSize,
                        &UA_TYPES[UA_TYPES_ADDNODESITEM]);

    UA_KeyValueMap payload = {7, addNodesPayload};
    auditNodeManagementEvent(server, UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_NODE_ADD,
                             channel, session, "AddNodes", status, payload);
}

void
auditDeleteNodesEvent(UA_Server *server, UA_SecureChannel *channel, UA_Session *session,
                      UA_Boolean status, size_t itemsSize, UA_DeleteNodesItem *items) {
    static UA_THREAD_LOCAL UA_KeyValuePair deleteNodesPayload[7] = {
        {{0, UA_STRING_STATIC("/ActionTimeStamp")}, {0}},             /* 0 */
        {{0, UA_STRING_STATIC("/Status")}, {0}},                      /* 1 */
        {{0, UA_STRING_STATIC("/ServerId")}, {0}},                    /* 2 */
        {{0, UA_STRING_STATIC("/ClientAuditEntryId")}, {0}},          /* 3 */
        {{0, UA_STRING_STATIC("/ClientUserId")}, {0}},                /* 4 */
        {{0, UA_STRING_STATIC("/SourceName")}, {0}},                  /* 5 */
        {{0, UA_STRING_STATIC("/NodesToDelete")}, {0}}                /* 6 */
    };

    /* /NodesToDelete */
    UA_Variant_setArray(&deleteNodesPayload[6].value, items, itemsSize,
                        &UA_TYPES[UA_TYPES_DELETENODESITEM]);

    UA_KeyValueMap payload = {7, deleteNodesPayload};
    auditNodeManagementEvent(server, UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_NODE_DELETE,
                             channel, session, "DeleteNodes", status, payload);
}

void
auditAddReferencesEvent(UA_Server *server, UA_SecureChannel *channel, UA_Session *session,
                        UA_Boolean status, size_t itemsSize, UA_AddReferencesItem *items) {
    static UA_THREAD_LOCAL UA_KeyValuePair addReferencesPayload[7] = {
        {{0, UA_STRING_STATIC("/ActionTimeStamp")}, {0}},             /* 0 */
        {{0, UA_STRING_STATIC("/Status")}, {0}},                      /* 1 */
        {{0, UA_STRING_STATIC("/ServerId")}, {0}},                    /* 2 */
        {{0, UA_STRING_STATIC("/ClientAuditEntryId")}, {0}},          /* 3 */
        {{0, UA_STRING_STATIC("/ClientUserId")}, {0}},                /* 4 */
        {{0, UA_STRING_STATIC("/SourceName")}, {0}},                  /* 5 */
        {{0, UA_STRING_STATIC("/ReferencesToAdd")}, {0}}              /* 6 */
    };

    /* /ReferencesToAdd */
    UA_Variant_setArray(&addReferencesPayload[6].value, items, itemsSize,
                        &UA_TYPES[UA_TYPES_ADDREFERENCESITEM]);

    UA_KeyValueMap payload = {7, addReferencesPayload};
    auditNodeManagementEvent(server, UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_NODE_ADDREFERENCES,
                             channel, session, "AddReferences", status, payload);
}

void
auditDeleteReferencesEvent(UA_Server *server, UA_SecureChannel *channel, UA_Session *session,
                           UA_Boolean status, size_t itemsSize, UA_DeleteReferencesItem *items) {
    static UA_THREAD_LOCAL UA_KeyValuePair deleteReferencesPayload[7] = {
        {{0, UA_STRING_STATIC("/ActionTimeStamp")}, {0}},             /* 0 */
        {{0, UA_STRING_STATIC("/Status")}, {0}},                      /* 1 */
        {{0, UA_STRING_STATIC("/ServerId")}, {0}},                    /* 2 */
        {{0, UA_STRING_STATIC("/ClientAuditEntryId")}, {0}},          /* 3 */
        {{0, UA_STRING_STATIC("/ClientUserId")}, {0}},                /* 4 */
        {{0, UA_STRING_STATIC("/SourceName")}, {0}},                  /* 5 */
        {{0, UA_STRING_STATIC("/ReferencesToDelete")}, {0}}           /* 6 */
    };

    /* /ReferencesToDelete */
    UA_Variant_setArray(&deleteReferencesPayload[6].value, items, itemsSize,
                        &UA_TYPES[UA_TYPES_DELETEREFERENCESITEM]);

    UA_KeyValueMap payload = {7, deleteReferencesPayload};
    auditNodeManagementEvent(server, UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_NODE_DELETEREFERENCES,
                             channel, session, "DeleteReferences", status, payload);
}

static void
auditUpdateEvent(UA_Server *server, UA_ApplicationNotificationType type,
                 UA_SecureChannel *channel, UA_Session *session,
                 const UA_NodeId *sourceNode, const char *serviceName,
                 UA_Boolean status, const UA_KeyValueMap payload) {
    /* SourceName */
    UA_Byte sourceNameBuf[128];
    UA_String sourceName = {128, sourceNameBuf};
    UA_String_format(&sourceName, "Attribute/%s", serviceName);
    UA_Variant_setScalar(&payload.map[5].value, &sourceName,
                         &UA_TYPES[UA_TYPES_STRING]);

    auditEvent(server, type, channel, session, sourceNode,
               serviceName, status, payload);
}

void
auditWriteUpdateEvent(UA_Server *server, UA_SecureChannel *channel, UA_Session *session,
                      UA_Boolean status, const UA_NodeId *sourceNode,
                      UA_UInt32 attributeId, const UA_String indexRange,
                      const UA_Variant *newValue, const UA_Variant *oldValue) {
    static UA_THREAD_LOCAL UA_KeyValuePair writeUpdatePayload[10] = {
        {{0, UA_STRING_STATIC("/ActionTimeStamp")}, {0}},             /* 0 */
        {{0, UA_STRING_STATIC("/Status")}, {0}},                      /* 1 */
        {{0, UA_STRING_STATIC("/ServerId")}, {0}},                    /* 2 */
        {{0, UA_STRING_STATIC("/ClientAuditEntryId")}, {0}},          /* 3 */
        {{0, UA_STRING_STATIC("/ClientUserId")}, {0}},                /* 4 */
        {{0, UA_STRING_STATIC("/SourceName")}, {0}},                  /* 5 */
        {{0, UA_STRING_STATIC("/AttributeId")}, {0}},                 /* 6 */
        {{0, UA_STRING_STATIC("/IndexRange")}, {0}},                  /* 7 */
        {{0, UA_STRING_STATIC("/NewValue")}, {0}},                    /* 8 */
        {{0, UA_STRING_STATIC("/OldValue")}, {0}}                     /* 9 */
    };

    /* /AttributeId */
    UA_Variant_setScalar(&writeUpdatePayload[6].value, &attributeId,
                         &UA_TYPES[UA_TYPES_UINT32]);

    /* /IndexRange */
    UA_Variant_setScalar(&writeUpdatePayload[7].value,
                         (void*)(uintptr_t)&indexRange,
                         &UA_TYPES[UA_TYPES_STRING]);

    /* /NewValue */
    UA_Variant_setScalar(&writeUpdatePayload[8].value,
                         (UA_Variant*)(uintptr_t)newValue,
                         &UA_TYPES[UA_TYPES_VARIANT]);

    /* /OldValue */
    UA_Variant_setScalar(&writeUpdatePayload[9].value,
                         (UA_Variant*)(uintptr_t)oldValue,
                         &UA_TYPES[UA_TYPES_VARIANT]);

    UA_KeyValueMap payload = {10, writeUpdatePayload};
    auditUpdateEvent(server, UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_UPDATE_WRITE,
                     channel, session, sourceNode, "Write", status, payload);
}

void
auditMethodUpdateEvent(UA_Server *server, UA_SecureChannel *channel, UA_Session *session,
                       UA_Boolean status, const UA_NodeId *sourceNode,
                       const UA_NodeId *methodNode, UA_StatusCode statusCodeId,
                       size_t inputsSize, UA_Variant *inputs,
                       size_t outputsSize, UA_Variant *outputs) {
    static UA_THREAD_LOCAL UA_KeyValuePair methodUpdatePayload[10] = {
        {{0, UA_STRING_STATIC("/ActionTimeStamp")}, {0}},             /* 0 */
        {{0, UA_STRING_STATIC("/Status")}, {0}},                      /* 1 */
        {{0, UA_STRING_STATIC("/ServerId")}, {0}},                    /* 2 */
        {{0, UA_STRING_STATIC("/ClientAuditEntryId")}, {0}},          /* 3 */
        {{0, UA_STRING_STATIC("/ClientUserId")}, {0}},                /* 4 */
        {{0, UA_STRING_STATIC("/SourceName")}, {0}},                  /* 5 */
        {{0, UA_STRING_STATIC("/MethodId")}, {0}},                    /* 6 */
        {{0, UA_STRING_STATIC("/StatusCodeId")}, {0}},                /* 7 */
        {{0, UA_STRING_STATIC("/InputArguments")}, {0}},              /* 8 */
        {{0, UA_STRING_STATIC("/OutputArguments")}, {0}}              /* 9 */
    };

    /* /MethodId */
    UA_Variant_setScalar(&methodUpdatePayload[6].value, (void*)(uintptr_t)methodNode,
                         &UA_TYPES[UA_TYPES_NODEID]);

    /* /StatusCodeId */
    UA_Variant_setScalar(&methodUpdatePayload[7].value, &statusCodeId,
                         &UA_TYPES[UA_TYPES_STATUSCODE]);

    /* /InputArguments */
    UA_Variant_setArray(&methodUpdatePayload[8].value, inputs, inputsSize,
                         &UA_TYPES[UA_TYPES_VARIANT]);

    /* /OutputArguments */
    UA_Variant_setArray(&methodUpdatePayload[9].value, outputs, outputsSize,
                         &UA_TYPES[UA_TYPES_VARIANT]);

    UA_KeyValueMap payload = {10, methodUpdatePayload};
    auditUpdateEvent(server, UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_UPDATE_METHOD,
                     channel, session, sourceNode, "Call", status, payload);
}

#endif /* UA_ENABLE_AUDITING */
