#include <time.h>
#include <stdlib.h>

#include "ua_session.h"
#include "ua_util.h"

UA_Int32 UA_Session_new(UA_Session **session) {
    UA_Int32 retval = UA_SUCCESS;
    retval |= UA_alloc((void **)session, sizeof(UA_Session));
    if(retval == UA_SUCCESS)
        UA_Session_init(*session);
    return retval;
}

/* mock up function to generate tokens for authentication */
UA_Int32 UA_Session_generateToken(UA_NodeId *newToken) {
    //Random token generation
    UA_Int32 retval = UA_SUCCESS;
    srand(time(NULL));

    UA_Int32 i = 0;
    UA_Int32 r = 0;
    //retval |= UA_NodeId_new(newToken);

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
    if(session1 && session2 &&
       UA_NodeId_equal(&session1->sessionId, &session2->sessionId) == UA_EQUAL)
        return UA_TRUE;
    return UA_FALSE;
}

UA_Int32 UA_Session_setExpirationDate(UA_Session *session) {
    if(!session)
        return UA_ERROR;

    session->validTill = UA_DateTime_now() + session->timeout * 100000; //timeout in ms
    return UA_SUCCESS;
}

UA_Int32 UA_Session_getPendingLifetime(UA_Session *session, UA_Double *pendingLifetime_ms) {
    if(!session)
        return UA_ERROR;

    *pendingLifetime_ms = (session->validTill - UA_DateTime_now())/10000000; //difference in ms
    return UA_SUCCESS;
}
