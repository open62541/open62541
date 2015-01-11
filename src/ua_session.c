#include "ua_session.h"
#include "ua_util.h"
#include "ua_statuscodes.h"

#ifdef UA_MULTITHREADING
#include <urcu/uatomic.h>
#endif

UA_Session anonymousSession = {
    .clientDescription =  {.applicationUri = {-1, UA_NULL},
                           .productUri = {-1, UA_NULL},
                           .applicationName = {.locale = {-1, UA_NULL}, .text = {-1, UA_NULL}},
                           .applicationType = UA_APPLICATIONTYPE_CLIENT,
                           .gatewayServerUri = {-1, UA_NULL},
                           .discoveryProfileUri = {-1, UA_NULL},
                           .discoveryUrlsSize = -1,
                           .discoveryUrls = UA_NULL},
    .sessionName = {sizeof("Anonymous Session")-1, (UA_Byte*)"Anonymous Session"},
    .authenticationToken = {.namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric = 0}, // is never used, as this session is not stored in the sessionmanager
    .sessionId = {.namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric = 0},
    .maxRequestMessageSize = UA_UINT32_MAX,
    .maxResponseMessageSize = UA_UINT32_MAX,
    .timeout = UA_INT64_MAX,
    .validTill = UA_INT64_MAX,
    .channel = UA_NULL};

UA_Session adminSession = {
    .clientDescription =  {.applicationUri = {-1, UA_NULL},
                           .productUri = {-1, UA_NULL},
                           .applicationName = {.locale = {-1, UA_NULL}, .text = {-1, UA_NULL}},
                           .applicationType = UA_APPLICATIONTYPE_CLIENT,
                           .gatewayServerUri = {-1, UA_NULL},
                           .discoveryProfileUri = {-1, UA_NULL},
                           .discoveryUrlsSize = -1,
                           .discoveryUrls = UA_NULL},
    .sessionName = {sizeof("Administrator Session")-1, (UA_Byte*)"Administrator Session"},
    .authenticationToken = {.namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric = 1}, // is never used, as this session is not stored in the sessionmanager
    .sessionId = {.namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric = 1},
    .maxRequestMessageSize = UA_UINT32_MAX,
    .maxResponseMessageSize = UA_UINT32_MAX,
    .timeout = UA_INT64_MAX,
    .validTill = UA_INT64_MAX,
    .channel = UA_NULL};

UA_Session * UA_Session_new(void) {
    UA_Session *s = UA_malloc(sizeof(UA_Session));
    if(s) UA_Session_init(s);
    return s;
}

/* TODO: Nobody seems to call this function right now */
static UA_StatusCode UA_Session_generateToken(UA_NodeId *newToken, UA_UInt32 *seed) {
    newToken->namespaceIndex = 0; // where else?
    newToken->identifierType = UA_NODEIDTYPE_GUID;
    newToken->identifier.guid = UA_Guid_random(seed);
    return UA_STATUSCODE_GOOD;
}

void UA_Session_init(UA_Session *session) {
    if(!session) return;
    UA_ApplicationDescription_init(&session->clientDescription);
    UA_NodeId_init(&session->authenticationToken);
    UA_NodeId_init(&session->sessionId);
    UA_String_init(&session->sessionName);
    session->maxRequestMessageSize  = 0;
    session->maxResponseMessageSize = 0;
    session->timeout = 0;
    UA_DateTime_init(&session->validTill);
    session->channel = UA_NULL;
}

void UA_Session_deleteMembers(UA_Session *session) {
    UA_ApplicationDescription_deleteMembers(&session->clientDescription);
    UA_NodeId_deleteMembers(&session->authenticationToken);
    UA_NodeId_deleteMembers(&session->sessionId);
    UA_String_deleteMembers(&session->sessionName);
    session->channel = UA_NULL;
}

void UA_Session_delete(UA_Session *session) {
    UA_Session_deleteMembers(session);
    UA_free(session);
}

UA_Boolean UA_Session_compare(UA_Session *session1, UA_Session *session2) {
    if(session1 && session2 && UA_NodeId_equal(&session1->sessionId, &session2->sessionId))
        return UA_TRUE;
    return UA_FALSE;
}

UA_StatusCode UA_Session_setExpirationDate(UA_Session *session) {
    if(!session)
        return UA_STATUSCODE_BADINTERNALERROR;

    session->validTill = UA_DateTime_now() + session->timeout * 100000; //timeout in ms
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Session_getPendingLifetime(UA_Session *session, UA_Double *pendingLifetime_ms) {
    if(!session)
        return UA_STATUSCODE_BADINTERNALERROR;

    *pendingLifetime_ms = (session->validTill - UA_DateTime_now())/10000000; //difference in ms
    return UA_STATUSCODE_GOOD;
}

void UA_SecureChannel_detachSession(UA_SecureChannel *channel) {
#ifdef UA_MULTITHREADING
    UA_Session *session = channel->session;
    if(session)
        uatomic_cmpxchg(&session->channel, channel, UA_NULL);
    uatomic_set(&channel->session, UA_NULL);
#else
    if(channel->session)
        channel->session->channel = UA_NULL;
    channel->session = UA_NULL;
#endif
}

void UA_SecureChannel_attachSession(UA_SecureChannel *channel, UA_Session *session) {
#ifdef UA_MULTITHREADING
    if(uatomic_cmpxchg(&session->channel, UA_NULL, channel) == UA_NULL)
        uatomic_set(&channel->session, session);
#else
    if(session->channel != UA_NULL)
        return;
    session->channel = channel;
    channel->session = session;
#endif
}

void UA_Session_detachSecureChannel(UA_Session *session) {
#ifdef UA_MULTITHREADING
    UA_SecureChannel *channel = session->channel;
    if(channel)
        uatomic_cmpxchg(&channel->session, session, UA_NULL);
    uatomic_set(&session->channel, UA_NULL);
#else
    if(session->channel)
        session->channel->session = UA_NULL;
    session->channel = UA_NULL;
#endif
}
