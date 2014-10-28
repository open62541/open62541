#include <time.h>
#include <stdlib.h>

#include "ua_session.h"
#include "ua_util.h"
#include "ua_statuscodes.h"

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

UA_StatusCode UA_Session_new(UA_Session **session) {
    if(!(*session = UA_alloc(sizeof(UA_Session))))
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_Session_init(*session);
    return UA_STATUSCODE_GOOD;
}

/* mock up function to generate tokens for authentication */
UA_StatusCode UA_Session_generateToken(UA_NodeId *newToken) {
    //Random token generation
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    srand(time(NULL));

    UA_Int32 i, r = 0;
    newToken->namespaceIndex = 0; // where else?
    newToken->identifierType = UA_NODEIDTYPE_GUID;
    newToken->identifier.guid.data1 = rand();
    r = rand();
    newToken->identifier.guid.data2 = (UA_UInt16)((r>>16) );
    r = rand();
    /* UA_Int32 r1 = (r>>16); */
    /* UA_Int32 r2 = r1 & 0xFFFF; */
    /* r2 = r2 * 1; */
    newToken->identifier.guid.data3 = (UA_UInt16)((r>>16) );
    for(i = 0;i < 8;i++) {
        r = rand();
        newToken->identifier.guid.data4[i] = (UA_Byte)((r>>28) );
    }
    return retval;
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
