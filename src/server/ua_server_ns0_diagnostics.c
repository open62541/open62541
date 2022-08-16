/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2022 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "ua_server_internal.h"
#include "ua_session.h"
#include "ua_subscription.h"

#ifdef UA_ENABLE_DIAGNOSTICS

static UA_Boolean
equalBrowseName(UA_String *bn, char *n) {
    UA_String name = UA_STRING(n);
    return UA_String_equal(bn, &name);
}

#ifdef UA_ENABLE_SUBSCRIPTIONS

/****************************/
/* Subscription Diagnostics */
/****************************/

static void
fillSubscriptionDiagnostics(UA_Subscription *sub,
                            UA_SubscriptionDiagnosticsDataType *diag) {
    UA_NodeId_copy(&sub->session->sessionId, &diag->sessionId); /* ignore status */
    diag->subscriptionId = sub->subscriptionId;
    diag->priority = sub->priority;
    diag->publishingInterval = sub->publishingInterval;
    diag->maxKeepAliveCount = sub->maxKeepAliveCount;
    diag->maxLifetimeCount = sub->lifeTimeCount;
    diag->maxNotificationsPerPublish = sub->notificationsPerPublish;
    diag->publishingEnabled = sub->publishingEnabled;
    diag->modifyCount = sub->modifyCount;
    diag->enableCount = sub->enableCount;
    diag->disableCount = sub->disableCount;
    diag->republishRequestCount = sub->republishRequestCount;
    diag->republishMessageRequestCount =
        sub->republishRequestCount; /* Always equal to the previous republishRequestCount */
    diag->republishMessageCount = sub->republishMessageCount;
    diag->transferRequestCount = sub->transferRequestCount;
    diag->transferredToAltClientCount = sub->transferredToAltClientCount;
    diag->transferredToSameClientCount = sub->transferredToSameClientCount;
    diag->publishRequestCount = sub->publishRequestCount;
    diag->dataChangeNotificationsCount = sub->dataChangeNotificationsCount;
    diag->eventNotificationsCount = sub->eventNotificationsCount;
    diag->notificationsCount = sub->notificationsCount;
    diag->latePublishRequestCount = sub->latePublishRequestCount;
    diag->currentKeepAliveCount = sub->currentKeepAliveCount;
    diag->currentLifetimeCount = sub->currentLifetimeCount;
    diag->unacknowledgedMessageCount = (UA_UInt32)sub->retransmissionQueueSize;
    diag->discardedMessageCount = sub->discardedMessageCount;
    diag->monitoredItemCount = sub->monitoredItemsSize;
    diag->monitoringQueueOverflowCount = sub->monitoringQueueOverflowCount;
    diag->nextSequenceNumber = sub->nextSequenceNumber;
    diag->eventQueueOverFlowCount = sub->eventQueueOverFlowCount;

    /* Count the disabled MonitoredItems */
    UA_MonitoredItem *mon;
    LIST_FOREACH(mon, &sub->monitoredItems, listEntry) {
        if(mon->monitoringMode == UA_MONITORINGMODE_DISABLED)
            diag->disabledMonitoredItemCount++;
    }
}

/* The node context points to the subscription */
static UA_StatusCode
readSubscriptionDiagnostics(UA_Server *server,
                            const UA_NodeId *sessionId, void *sessionContext,
                            const UA_NodeId *nodeId, void *nodeContext,
                            UA_Boolean sourceTimestamp,
                            const UA_NumericRange *range, UA_DataValue *value) {
    /* Check the Subscription pointer */
    UA_Subscription *sub = (UA_Subscription*)nodeContext;
    if(!sub)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Read the BrowseName */
    UA_QualifiedName bn;
    UA_StatusCode res = readWithReadValue(server, nodeId, UA_ATTRIBUTEID_BROWSENAME, &bn);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Set the value */
    UA_SubscriptionDiagnosticsDataType sddt;
    UA_SubscriptionDiagnosticsDataType_init(&sddt);
    fillSubscriptionDiagnostics(sub, &sddt);

    char memberName[128];
    memcpy(memberName, bn.name.data, bn.name.length);
    memberName[bn.name.length] = 0;

    size_t memberOffset;
    const UA_DataType *memberType;
    UA_Boolean isArray;
    UA_Boolean found =
        UA_DataType_getStructMember(&UA_TYPES[UA_TYPES_SUBSCRIPTIONDIAGNOSTICSDATATYPE],
                                    memberName, &memberOffset, &memberType, &isArray);
    if(!found) {
        /* Not the member, but the main subscription diagnostics variable... */
        memberOffset = 0;
        memberType = &UA_TYPES[UA_TYPES_SUBSCRIPTIONDIAGNOSTICSDATATYPE];
    }

    void *content = (void*)(((uintptr_t)&sddt) + memberOffset);
    res = UA_Variant_setScalarCopy(&value->value, content, memberType);
    if(UA_LIKELY(res == UA_STATUSCODE_GOOD))
        value->hasValue = true;

    UA_SubscriptionDiagnosticsDataType_clear(&sddt);
    UA_QualifiedName_clear(&bn);
    return res;
}

/* If the nodeContext == NULL, return all subscriptions in the server.
 * Otherwise only for the current session. */
UA_StatusCode
readSubscriptionDiagnosticsArray(UA_Server *server,
                                 const UA_NodeId *sessionId, void *sessionContext,
                                 const UA_NodeId *nodeId, void *nodeContext,
                                 UA_Boolean sourceTimestamp,
                                 const UA_NumericRange *range, UA_DataValue *value) {
    /* Get the current session */
    size_t sdSize = 0;
    UA_Session *session = NULL;
    session_list_entry *sentry;
    if(nodeContext) {
        session = UA_Server_getSessionById(server, sessionId);
        if(!session)
            return UA_STATUSCODE_BADINTERNALERROR;
        sdSize = session->subscriptionsSize;
    } else {
        LIST_FOREACH(sentry, &server->sessions, pointers) {
            sdSize += sentry->session.subscriptionsSize;
        }
    }

    /* Allocate the output array */
    UA_SubscriptionDiagnosticsDataType *sd = (UA_SubscriptionDiagnosticsDataType*)
        UA_Array_new(sdSize, &UA_TYPES[UA_TYPES_SUBSCRIPTIONDIAGNOSTICSDATATYPE]);
    if(!sd)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Collect the statistics */
    size_t i = 0;
    UA_Subscription *sub;
    if(session) {
        TAILQ_FOREACH(sub, &session->subscriptions, sessionListEntry) {
            fillSubscriptionDiagnostics(sub, &sd[i]);
            i++;
        }
    } else {
        LIST_FOREACH(sentry, &server->sessions, pointers) {
            TAILQ_FOREACH(sub, &sentry->session.subscriptions, sessionListEntry) {
                fillSubscriptionDiagnostics(sub, &sd[i]);
                i++;
            }
        }
    }

    /* Set the output */
    value->hasValue = true;
    UA_Variant_setArray(&value->value, sd, sdSize,
                        &UA_TYPES[UA_TYPES_SUBSCRIPTIONDIAGNOSTICSDATATYPE]);
    return UA_STATUSCODE_GOOD;
}

void
createSubscriptionObject(UA_Server *server, UA_Session *session,
                         UA_Subscription *sub) {
    UA_ExpandedNodeId *children = NULL;
    size_t childrenSize = 0;
    UA_ReferenceTypeSet refTypes;
    UA_NodeId hasComponent = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);

    char subIdStr[32];
    snprintf(subIdStr, 32, "%u", sub->subscriptionId);

    /* Find the NodeId of the SubscriptionDiagnosticsArray */
    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = sub->session->sessionId;
    UA_RelativePathElement rpe[1];
    memset(rpe, 0, sizeof(UA_RelativePathElement) * 1);
    rpe[0].targetName = UA_QUALIFIEDNAME(0, "SubscriptionDiagnosticsArray");
    bp.relativePath.elements = rpe;
    bp.relativePath.elementsSize = 1;
    UA_BrowsePathResult bpr = translateBrowsePathToNodeIds(server, &bp);
    if(bpr.targetsSize < 1)
        return;

    /* Create an object for the subscription. Instantiates all the mandatory
     * children. */
    UA_VariableAttributes var_attr = UA_VariableAttributes_default;
    var_attr.displayName.text = UA_STRING(subIdStr);
    var_attr.dataType = UA_TYPES[UA_TYPES_SUBSCRIPTIONDIAGNOSTICSDATATYPE].typeId;
    UA_NodeId refId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    UA_QualifiedName browseName = UA_QUALIFIEDNAME(0, subIdStr);
    UA_NodeId typeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SUBSCRIPTIONDIAGNOSTICSTYPE);
    UA_NodeId objId = UA_NODEID_NUMERIC(1, 0); /* 0 => assign a random free id */
    UA_StatusCode res = addNode(server, UA_NODECLASS_VARIABLE,
                                &objId,
                                &bpr.targets[0].targetId.nodeId /* parent */,
                                &refId, browseName, &typeId,
                                (UA_NodeAttributes*)&var_attr,
                                &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES], NULL, &objId);
    UA_CHECK_STATUS(res, goto cleanup);

    /* Add a second reference from the overall SubscriptionDiagnosticsArray variable */
    const UA_NodeId subDiagArray =
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SUBSCRIPTIONDIAGNOSTICSARRAY);
    res = addRef(server, session,  &subDiagArray, &refId, &objId, true);

    /* Get all children (including the variable itself) and set the contenxt + callback */
    res = referenceTypeIndices(server, &hasComponent, &refTypes, false);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;
    res = browseRecursive(server, 1, &objId,
                          UA_BROWSEDIRECTION_FORWARD, &refTypes,
                          UA_NODECLASS_VARIABLE, true, &childrenSize, &children);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Add the callback to all variables  */
    UA_DataSource subDiagSource = {readSubscriptionDiagnostics, NULL};
    for(size_t i = 0; i < childrenSize; i++) {
        setVariableNode_dataSource(server, children[i].nodeId, subDiagSource);
        setNodeContext(server, children[i].nodeId, sub);
    }

    UA_Array_delete(children, childrenSize, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);

 cleanup:
    UA_BrowsePathResult_clear(&bpr);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_SESSION(&server->config.logger, session,
                               "Creating the subscription diagnostics object failed "
                               "with StatusCode %s", UA_StatusCode_name(res));
    }
}

#endif /* UA_ENABLE_SUBSCRIPTIONS */

/***********************/
/* Session Diagnostics */
/***********************/

static void
setSessionDiagnostics(UA_Session *session, UA_SessionDiagnosticsDataType *sd) {
    UA_SessionDiagnosticsDataType_copy(&session->diagnostics, sd);
    UA_NodeId_copy(&session->sessionId, &sd->sessionId);
    UA_String_copy(&session->sessionName, &sd->sessionName);
    UA_ApplicationDescription_copy(&session->clientDescription,
                                   &sd->clientDescription);
    sd->maxResponseMessageSize = session->maxResponseMessageSize;
#ifdef UA_ENABLE_SUBSCRIPTIONS
    sd->currentPublishRequestsInQueue = (UA_UInt32)session->responseQueueSize;
#endif
    sd->actualSessionTimeout = session->timeout;

    /* Set LocaleIds */
    UA_StatusCode res =
        UA_Array_copy(session->localeIds, session->localeIdsSize,
                      (void **)&sd->localeIds, &UA_TYPES[UA_TYPES_STRING]);
    if(UA_LIKELY(res == UA_STATUSCODE_GOOD))
        sd->localeIdsSize = session->localeIdsSize;

        /* Set Subscription diagnostics */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    sd->currentSubscriptionsCount = (UA_UInt32)session->subscriptionsSize;

    UA_Subscription *sub;
    TAILQ_FOREACH(sub, &session->subscriptions, sessionListEntry) {
        sd->currentMonitoredItemsCount += (UA_UInt32)sub->monitoredItemsSize;
    }
#endif
}

UA_StatusCode
readSessionDiagnosticsArray(UA_Server *server,
                            const UA_NodeId *sessionId, void *sessionContext,
                            const UA_NodeId *nodeId, void *nodeContext,
                            UA_Boolean sourceTimestamp,
                            const UA_NumericRange *range, UA_DataValue *value) {
    /* Allocate the output array */
    UA_SessionDiagnosticsDataType *sd = (UA_SessionDiagnosticsDataType*)
        UA_Array_new(server->sessionCount,
                     &UA_TYPES[UA_TYPES_SESSIONDIAGNOSTICSDATATYPE]);
    if(!sd)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Collect the statistics */
    size_t i = 0;
    session_list_entry *session;
    LIST_FOREACH(session, &server->sessions, pointers) {
        setSessionDiagnostics(&session->session, &sd[i]);
        i++;
    }

    /* Set the output */
    value->hasValue = true;
    UA_Variant_setArray(&value->value, sd, server->sessionCount,
                        &UA_TYPES[UA_TYPES_SESSIONDIAGNOSTICSDATATYPE]);
    return UA_STATUSCODE_GOOD;
}

static void
setSessionSecurityDiagnostics(UA_Session *session,
                              UA_SessionSecurityDiagnosticsDataType *sd) {
    UA_SessionSecurityDiagnosticsDataType_copy(&session->securityDiagnostics, sd);
    UA_NodeId_copy(&session->sessionId, &sd->sessionId);
    UA_SecureChannel *channel = session->header.channel;
    if(channel) {
        UA_ByteString_copy(&channel->remoteCertificate, &sd->clientCertificate);
        UA_String_copy(&channel->securityPolicy->policyUri, &sd->securityPolicyUri);
        sd->securityMode = channel->securityMode;
        sd->encoding = UA_STRING_ALLOC("UA Binary"); /* The only one atm */
        sd->transportProtocol = UA_STRING_ALLOC("opc.tcp"); /* The only one atm */
    }
}

static UA_StatusCode
readSessionDiagnostics(UA_Server *server,
                       const UA_NodeId *sessionId, void *sessionContext,
                       const UA_NodeId *nodeId, void *nodeContext,
                       UA_Boolean sourceTimestamp,
                       const UA_NumericRange *range, UA_DataValue *value) {
    /* Get the Session */
    UA_Session *session = UA_Server_getSessionById(server, sessionId);
    if(!session)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Read the BrowseName */
    UA_QualifiedName bn;
    UA_StatusCode res = readWithReadValue(server, nodeId, UA_ATTRIBUTEID_BROWSENAME, &bn);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    union {
        UA_SessionDiagnosticsDataType sddt;
        UA_SessionSecurityDiagnosticsDataType ssddt;
    } data;
    void *content;
    UA_Boolean isArray = false;
    const UA_DataType *type = NULL;
    UA_Boolean securityDiagnostics = false;

    char memberName[128];
    size_t memberOffset;
    UA_Boolean found;

    if(equalBrowseName(&bn.name, "SubscriptionDiagnosticsArray")) {
        /* Reuse the datasource callback. Forward a non-null nodeContext to
         * indicate that we want to see only the subscriptions for the current
         * session. */
        res = readSubscriptionDiagnosticsArray(server, sessionId, sessionContext,
                                               nodeId, (void*)0x01,
                                               sourceTimestamp, range, value);
        goto cleanup;
    } else if(equalBrowseName(&bn.name, "SessionDiagnostics")) {
        setSessionDiagnostics(session, &data.sddt);
        content = &data.sddt;
        type = &UA_TYPES[UA_TYPES_SESSIONDIAGNOSTICSDATATYPE];
        goto set_value;
    } else if(equalBrowseName(&bn.name, "SessionSecurityDiagnostics")) {
        setSessionSecurityDiagnostics(session, &data.ssddt);
        securityDiagnostics = true;
        content = &data.ssddt;
        type = &UA_TYPES[UA_TYPES_SESSIONSECURITYDIAGNOSTICSDATATYPE];
        goto set_value;
    }

    /* Try to find the member in SessionDiagnosticsDataType and
     * SessionSecurityDiagnosticsDataType */
    memcpy(memberName, bn.name.data, bn.name.length);
    memberName[bn.name.length] = 0;
    found = UA_DataType_getStructMember(&UA_TYPES[UA_TYPES_SESSIONDIAGNOSTICSDATATYPE],
                                        memberName, &memberOffset, &type, &isArray);
    if(found) {
        setSessionDiagnostics(session, &data.sddt);
        content = (void*)(((uintptr_t)&data.sddt) + memberOffset);
    } else {
        found = UA_DataType_getStructMember(&UA_TYPES[UA_TYPES_SESSIONSECURITYDIAGNOSTICSDATATYPE],
                                            memberName, &memberOffset, &type, &isArray);
        if(!found) {
            res = UA_STATUSCODE_BADNOTIMPLEMENTED;
            goto cleanup;
        }
        setSessionSecurityDiagnostics(session, &data.ssddt);
        securityDiagnostics = true;
        content = (void*)(((uintptr_t)&data.ssddt) + memberOffset);
    }

 set_value:
    if(!isArray) {
        res = UA_Variant_setScalarCopy(&value->value, content, type);
    } else {
        size_t len = *(size_t*)content;
        content = (void*)(((uintptr_t)content) + sizeof(size_t));
        res = UA_Variant_setArrayCopy(&value->value, content, len, type);
    }
    if(UA_LIKELY(res == UA_STATUSCODE_GOOD))
        value->hasValue = true;

    if(securityDiagnostics)
        UA_SessionSecurityDiagnosticsDataType_clear(&data.ssddt);
    else
        UA_SessionDiagnosticsDataType_clear(&data.sddt);

 cleanup:
    UA_QualifiedName_clear(&bn);
    return res;
}

UA_StatusCode
readSessionSecurityDiagnostics(UA_Server *server,
                               const UA_NodeId *sessionId, void *sessionContext,
                               const UA_NodeId *nodeId, void *nodeContext,
                               UA_Boolean sourceTimestamp,
                               const UA_NumericRange *range, UA_DataValue *value) {
    /* Allocate the output array */
    UA_SessionSecurityDiagnosticsDataType *sd = (UA_SessionSecurityDiagnosticsDataType*)
        UA_Array_new(server->sessionCount,
                     &UA_TYPES[UA_TYPES_SESSIONSECURITYDIAGNOSTICSDATATYPE]);
    if(!sd)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Collect the statistics */
    size_t i = 0;
    session_list_entry *session;
    LIST_FOREACH(session, &server->sessions, pointers) {
        setSessionSecurityDiagnostics(&session->session, &sd[i]);
        i++;
    }

    /* Set the output */
    value->hasValue = true;
    UA_Variant_setArray(&value->value, sd, server->sessionCount,
                        &UA_TYPES[UA_TYPES_SESSIONSECURITYDIAGNOSTICSDATATYPE]);
    return UA_STATUSCODE_GOOD;
}

void
createSessionObject(UA_Server *server, UA_Session *session) {
    UA_ExpandedNodeId *children = NULL;
    size_t childrenSize = 0;
    UA_ReferenceTypeSet refTypes;
    UA_NodeId hasComponent = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);

    /* Create an object for the session. Instantiates all the mandatory children. */
    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.displayName.text = session->sessionName;
    UA_NodeId parentId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SESSIONSDIAGNOSTICSSUMMARY);
    UA_NodeId refId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    UA_QualifiedName browseName = UA_QUALIFIEDNAME(0, "");
    browseName.name = session->sessionName; /* shallow copy */
    UA_NodeId typeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SESSIONDIAGNOSTICSOBJECTTYPE);
    UA_StatusCode res = addNode(server, UA_NODECLASS_OBJECT,
                                &session->sessionId, &parentId, &refId, browseName, &typeId,
                                (UA_NodeAttributes*)&object_attr,
                                &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL, NULL);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Recursively browse all children */
    res = referenceTypeIndices(server, &hasComponent, &refTypes, false);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;
    res = browseRecursive(server, 1, &session->sessionId,
                          UA_BROWSEDIRECTION_FORWARD, &refTypes,
                          UA_NODECLASS_VARIABLE, false, &childrenSize, &children);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Add the callback to all variables  */
    UA_DataSource sessionDiagSource = {readSessionDiagnostics, NULL};
    for(size_t i = 0; i < childrenSize; i++) {
        setVariableNode_dataSource(server, children[i].nodeId, sessionDiagSource);
    }

 cleanup:
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_SESSION(&server->config.logger, session,
                               "Creating the session diagnostics object failed "
                               "with StatusCode %s", UA_StatusCode_name(res));
    }
    UA_Array_delete(children, childrenSize, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
}

/***************************/
/* Server-Wide Diagnostics */
/***************************/

UA_StatusCode
readDiagnostics(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                const UA_NodeId *nodeId, void *nodeContext, UA_Boolean sourceTimestamp,
                const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }

    if(sourceTimestamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }

    UA_assert(nodeId->identifierType == UA_NODEIDTYPE_NUMERIC);

    void *data = NULL;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_UINT32]; /* Default */

    switch(nodeId->identifier.numeric) {
    case UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY:
        server->serverDiagnosticsSummary.currentSessionCount =
            server->activeSessionCount;
        data = &server->serverDiagnosticsSummary;
        type = &UA_TYPES[UA_TYPES_SERVERDIAGNOSTICSSUMMARYDATATYPE];
        break;
    case UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_SERVERVIEWCOUNT:
        data = &server->serverDiagnosticsSummary.serverViewCount;
        break;
    case UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_CURRENTSESSIONCOUNT:
        data = &server->activeSessionCount;
        break;
    case UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_CUMULATEDSESSIONCOUNT:
        data = &server->serverDiagnosticsSummary.cumulatedSessionCount;
        break;
    case UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_SECURITYREJECTEDSESSIONCOUNT:
        data = &server->serverDiagnosticsSummary.securityRejectedSessionCount;
        break;
    case UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_REJECTEDSESSIONCOUNT:
        data = &server->serverDiagnosticsSummary.rejectedSessionCount;
        break;
    case UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_SESSIONTIMEOUTCOUNT:
        data = &server->serverDiagnosticsSummary.sessionTimeoutCount;
        break;
    case UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_SESSIONABORTCOUNT:
        data = &server->serverDiagnosticsSummary.sessionAbortCount;
        break;
    case UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_CURRENTSUBSCRIPTIONCOUNT:
        data = &server->serverDiagnosticsSummary.currentSubscriptionCount;
        break;
    case UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_CUMULATEDSUBSCRIPTIONCOUNT:
        data = &server->serverDiagnosticsSummary.cumulatedSubscriptionCount;
        break;
    case UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_PUBLISHINGINTERVALCOUNT:
        data = &server->serverDiagnosticsSummary.publishingIntervalCount;
        break;
    case UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_SECURITYREJECTEDREQUESTSCOUNT:
        data = &server->serverDiagnosticsSummary.securityRejectedRequestsCount;
        break;
    case UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_REJECTEDREQUESTSCOUNT:
        data = &server->serverDiagnosticsSummary.rejectedRequestsCount;
        break;
    default:
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode res = UA_Variant_setScalarCopy(&value->value, data, type);
    if(res == UA_STATUSCODE_GOOD)
        value->hasValue = true;
    return res;
}

#endif /* UA_ENABLE_DIAGNOSTICS */
