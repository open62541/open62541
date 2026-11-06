/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

/* In-process Sessions for the RBAC tests */

#ifndef TEST_HELPERS_RBAC_H_
#define TEST_HELPERS_RBAC_H_

#include <open62541/server.h>

#include "server/ua_server_internal.h"
#include "server/ua_services.h"
#include "server/ua_subscription.h"

#include <check.h>

typedef UA_Boolean (*RbacService)(UA_Server *server, UA_Session *session,
                                  const void *request, void *response);

static UA_INLINE void
rbacService(UA_Server *server, UA_Session *session, RbacService service,
            const void *request, void *response) {
    lockServer(server);
    service(server, session, request, response);
    unlockServer(server);
}

static UA_INLINE void
rbacSetRoles(UA_Server *server, const UA_NodeId *sessionId,
             size_t rolesSize, const UA_NodeId *roles) {
    UA_Variant v;
    UA_Variant_setArray(&v, (void*)(uintptr_t)roles, rolesSize,
                        &UA_TYPES[UA_TYPES_NODEID]);
    ck_assert_uint_eq(UA_Server_setSessionAttribute(server, sessionId,
                                                    UA_QUALIFIEDNAME(0, "roles"),
                                                    &v),
                      UA_STATUSCODE_GOOD);
}

/* A Session on the channel (may be NULL) that holds exactly the Role */
static UA_INLINE UA_Session *
rbacSession(UA_Server *server, UA_SecureChannel *channel, UA_NodeId role) {
    UA_CreateSessionRequest request;
    UA_CreateSessionRequest_init(&request);
    request.requestedSessionTimeout = UA_UINT32_MAX;
    UA_Session *session = NULL;
    lockServer(server);
    UA_StatusCode res = UA_Session_create(server, channel, &request, &session);
    unlockServer(server);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    rbacSetRoles(server, &session->sessionId, 1, &role);
    return session;
}

#ifdef UA_ENABLE_SUBSCRIPTIONS
/* A Subscription with one MonitoredItem. For the EventNotifier the filter
 * selects the Event field with the BrowseName (namespace 0). */
static UA_INLINE UA_MonitoredItem *
rbacMonitor(UA_Server *server, UA_Session *session, const UA_NodeId nodeId,
            UA_UInt32 attributeId, const char *eventField, UA_StatusCode *status) {
    UA_CreateSubscriptionRequest subRequest;
    UA_CreateSubscriptionRequest_init(&subRequest);
    subRequest.publishingEnabled = true;
    UA_CreateSubscriptionResponse subResponse;
    UA_CreateSubscriptionResponse_init(&subResponse);
    rbacService(server, session, Service_CreateSubscription, &subRequest, &subResponse);
    ck_assert_uint_eq(subResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    UA_MonitoredItemCreateRequest item;
    UA_MonitoredItemCreateRequest_init(&item);
    item.itemToMonitor.nodeId = nodeId;
    item.itemToMonitor.attributeId = attributeId;
    item.monitoringMode = UA_MONITORINGMODE_REPORTING;
    item.requestedParameters.queueSize = 10;
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    UA_QualifiedName field = UA_QUALIFIEDNAME(0, (char*)(uintptr_t)eventField);
    UA_SimpleAttributeOperand select;
    UA_SimpleAttributeOperand_init(&select);
    select.typeDefinitionId = UA_NS0ID(BASEEVENTTYPE);
    select.browsePath = &field;
    select.browsePathSize = 1;
    select.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);
    filter.selectClauses = &select;
    filter.selectClausesSize = 1;
    if(eventField)
        UA_ExtensionObject_setValueNoDelete(&item.requestedParameters.filter,
                                            &filter, &UA_TYPES[UA_TYPES_EVENTFILTER]);
#endif
    UA_CreateMonitoredItemsRequest request;
    UA_CreateMonitoredItemsRequest_init(&request);
    request.subscriptionId = subResponse.subscriptionId;
    request.timestampsToReturn = UA_TIMESTAMPSTORETURN_NEITHER;
    request.itemsToCreate = &item;
    request.itemsToCreateSize = 1;
    UA_CreateMonitoredItemsResponse response;
    UA_CreateMonitoredItemsResponse_init(&response);
    rbacService(server, session, Service_CreateMonitoredItems, &request, &response);
    ck_assert_uint_eq(response.resultsSize, 1);
    *status = response.results[0].statusCode;
    lockServer(server);
    UA_Subscription *sub = getSubscriptionById(server, subResponse.subscriptionId);
    UA_MonitoredItem *mon =
        UA_Subscription_getMonitoredItem(sub, response.results[0].monitoredItemId);
    unlockServer(server);
    UA_CreateMonitoredItemsResponse_clear(&response);
    return mon;
}
#endif

#endif /* TEST_HELPERS_RBAC_H_ */
