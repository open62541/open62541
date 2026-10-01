/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

/* RBAC enforcement in the services, called with in-process Sessions. The
 * Sessions have no SecureChannel and the server has no TCP listener. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/nodeids.h>

#include "server/ua_server_internal.h"
#include "server/ua_services.h"

#include "test_helpers.h"

#include <check.h>
#include <stdlib.h>

static UA_Server *server = NULL;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_getConfig(server)->tcpEnabled = false;
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    server = NULL;
}

/* Create an in-process Session that holds exactly one Role */
static UA_Session *
createSessionWithRole(UA_UInt32 roleId) {
    UA_CreateSessionRequest request;
    UA_CreateSessionRequest_init(&request);
    request.requestedSessionTimeout = UA_UINT32_MAX;
    UA_Session *session = NULL;
    lockServer(server);
    UA_StatusCode res = UA_Session_create(server, NULL, &request, &session);
    unlockServer(server);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_ptr_ne(session, NULL);

    UA_NodeId role = UA_NODEID_NUMERIC(0, roleId);
    UA_Variant v;
    UA_Variant_setArray(&v, &role, 1, &UA_TYPES[UA_TYPES_NODEID]);
    ck_assert_uint_eq(UA_Server_setSessionAttribute(server, &session->sessionId,
                                                    UA_QUALIFIEDNAME(0, "roles"), &v),
                      UA_STATUSCODE_GOOD);
    return session;
}

/* Anonymous may browse. ConfigureAdmin gets the given permissions. */
static void
setPermissions(const UA_NodeId nodeId, UA_PermissionType configureAdminPermissions) {
    UA_RolePermission rp[2];
    rp[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    rp[0].permissions = UA_PERMISSIONTYPE_BROWSE;
    rp[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN);
    rp[1].permissions = configureAdminPermissions;
    ck_assert_uint_eq(UA_Server_setNodeRolePermissions(server, nodeId, 2, rp,
                                                       false, NULL),
                      UA_STATUSCODE_GOOD);
}

static UA_NodeId
addObject(const char *name, const UA_NodeId parentId,
          const UA_NodeId referenceTypeId, UA_Byte eventNotifier) {
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", (char*)(uintptr_t)name);
    attr.eventNotifier = eventNotifier;
    UA_NodeId nodeId = UA_NODEID_STRING(1, (char*)(uintptr_t)name);
    ck_assert_uint_eq(UA_Server_addObjectNode(server, nodeId, parentId,
                                              referenceTypeId,
                                              UA_QUALIFIEDNAME(1, (char*)(uintptr_t)name),
                                              UA_NS0ID(BASEOBJECTTYPE),
                                              attr, NULL, NULL),
                      UA_STATUSCODE_GOOD);
    return nodeId;
}

static UA_Boolean
nodeExists(const UA_NodeId nodeId) {
    UA_NodeClass nodeClass;
    return (UA_Server_readNodeClass(server, nodeId, &nodeClass) == UA_STATUSCODE_GOOD);
}

static void *
matchReferenceTarget(void *context, UA_ReferenceTarget *t) {
    const UA_NodeId *target = (const UA_NodeId*)context;
    if(!UA_NodePointer_isLocal(t->targetId))
        return NULL;
    UA_NodeId id = UA_NodePointer_toNodeId(t->targetId);
    return UA_NodeId_equal(&id, target) ? (void*)0x01 : NULL;
}

/* Inspect the references stored in the source node. Browse would hide
 * references to deleted nodes. */
static UA_Boolean
hasReferenceTo(const UA_NodeId source, const UA_NodeId target) {
    UA_Boolean found = false;
    lockServer(server);
    const UA_Node *node = UA_NODESTORE_GET(server, &source);
    ck_assert_ptr_ne(node, NULL);
    for(size_t i = 0; i < node->head.referencesSize && !found; i++) {
        found = (UA_NodeReferenceKind_iterate(&node->head.references[i],
                                              matchReferenceTarget,
                                              (void*)(uintptr_t)&target) != NULL);
    }
    UA_NODESTORE_RELEASE(server, node);
    unlockServer(server);
    return found;
}

static UA_StatusCode
deleteNodeAs(UA_Session *session, const UA_NodeId nodeId) {
    UA_DeleteNodesItem item;
    UA_DeleteNodesItem_init(&item);
    item.nodeId = nodeId;
    item.deleteTargetReferences = true;
    UA_DeleteNodesRequest request;
    UA_DeleteNodesRequest_init(&request);
    request.nodesToDeleteSize = 1;
    request.nodesToDelete = &item;

    UA_DeleteNodesResponse response;
    UA_DeleteNodesResponse_init(&response);
    lockServer(server);
    Service_DeleteNodes(server, session, &request, &response);
    unlockServer(server);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(response.resultsSize, 1);
    UA_StatusCode res = response.results[0];
    UA_DeleteNodesResponse_clear(&response);
    return res;
}

/****************/
/* Delete Nodes */
/****************/

/* The child is deleted together with its parent. Without DeleteNode on the
 * child, the whole delete is refused and nothing is deleted. */
START_TEST(deleteNodes_recursive_refusedWithoutChildPermission) {
    UA_NodeId parent = addObject("Parent", UA_NS0ID(OBJECTSFOLDER),
                                 UA_NS0ID(ORGANIZES), 0);
    UA_NodeId child = addObject("Child", parent, UA_NS0ID(HASCOMPONENT), 0);
    setPermissions(parent, UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_DELETENODE);
    setPermissions(child, UA_PERMISSIONTYPE_BROWSE);
    UA_Session *session = createSessionWithRole(UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN);

    ck_assert_uint_eq(deleteNodeAs(session, parent), UA_STATUSCODE_BADUSERACCESSDENIED);
    ck_assert(nodeExists(parent));
    ck_assert(nodeExists(child));
    ck_assert(hasReferenceTo(UA_NS0ID(OBJECTSFOLDER), parent));
    ck_assert(hasReferenceTo(parent, child));

    /* The local admin is not restricted */
    ck_assert_uint_eq(UA_Server_deleteNode(server, parent, true), UA_STATUSCODE_GOOD);
    ck_assert(!nodeExists(parent));
    ck_assert(!nodeExists(child));
} END_TEST

START_TEST(deleteNodes_recursive_grantedWithChildPermission) {
    UA_NodeId parent = addObject("Parent", UA_NS0ID(OBJECTSFOLDER),
                                 UA_NS0ID(ORGANIZES), 0);
    UA_NodeId child = addObject("Child", parent, UA_NS0ID(HASCOMPONENT), 0);
    setPermissions(parent, UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_DELETENODE);
    setPermissions(child, UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_DELETENODE);
    UA_Session *session = createSessionWithRole(UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN);

    ck_assert_uint_eq(deleteNodeAs(session, parent), UA_STATUSCODE_GOOD);
    ck_assert(!nodeExists(parent));
    ck_assert(!nodeExists(child));
    ck_assert(!hasReferenceTo(UA_NS0ID(OBJECTSFOLDER), parent));
} END_TEST

/* The AccessRestrictions of the child apply as well. The in-process Session
 * has no SecureChannel and cannot satisfy EncryptionRequired. */
START_TEST(deleteNodes_recursive_childEncryptionRequired) {
    UA_NodeId parent = addObject("Parent", UA_NS0ID(OBJECTSFOLDER),
                                 UA_NS0ID(ORGANIZES), 0);
    UA_NodeId child = addObject("Child", parent, UA_NS0ID(HASCOMPONENT), 0);
    setPermissions(parent, UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_DELETENODE);
    setPermissions(child, UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_DELETENODE);
    ck_assert_uint_eq(UA_Server_setNodeAccessRestrictions(server, child,
                          UA_ACCESSRESTRICTIONTYPE_ENCRYPTIONREQUIRED),
                      UA_STATUSCODE_GOOD);
    UA_Session *session = createSessionWithRole(UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN);

    ck_assert_uint_eq(deleteNodeAs(session, parent),
                      UA_STATUSCODE_BADSECURITYMODEINSUFFICIENT);
    ck_assert(nodeExists(parent));
    ck_assert(nodeExists(child));
    ck_assert(hasReferenceTo(parent, child));
} END_TEST

/* Deleting a node removes the references that point to it. This must not
 * depend on the RemoveReference permission on the referencing nodes. Otherwise
 * the references would dangle. */
START_TEST(deleteNodes_removesIncomingReferences) {
    UA_NodeId referrer = addObject("Referrer", UA_NS0ID(OBJECTSFOLDER),
                                   UA_NS0ID(ORGANIZES), 0);
    UA_NodeId target = addObject("Target", UA_NS0ID(OBJECTSFOLDER),
                                 UA_NS0ID(ORGANIZES), 0);
    ck_assert_uint_eq(UA_Server_addReference(server, referrer,
                                             UA_NS0ID(HASNOTIFIER),
                                             UA_EXPANDEDNODEID_NODEID(target), true),
                      UA_STATUSCODE_GOOD);
    setPermissions(referrer, UA_PERMISSIONTYPE_BROWSE);
    setPermissions(target, UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_DELETENODE);
    UA_Session *session = createSessionWithRole(UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN);
    ck_assert(hasReferenceTo(referrer, target));

    ck_assert_uint_eq(deleteNodeAs(session, target), UA_STATUSCODE_GOOD);
    ck_assert(!nodeExists(target));
    ck_assert(nodeExists(referrer));
    ck_assert(!hasReferenceTo(referrer, target));
    ck_assert(!hasReferenceTo(UA_NS0ID(OBJECTSFOLDER), target));
} END_TEST

/* A child with another parent is not deleted along. So its permissions do
 * not matter for the delete. */
START_TEST(deleteNodes_childWithOtherParentNotCollected) {
    UA_NodeId parent = addObject("Parent", UA_NS0ID(OBJECTSFOLDER),
                                 UA_NS0ID(ORGANIZES), 0);
    UA_NodeId otherParent = addObject("OtherParent", UA_NS0ID(OBJECTSFOLDER),
                                      UA_NS0ID(ORGANIZES), 0);
    UA_NodeId child = addObject("Child", parent, UA_NS0ID(HASCOMPONENT), 0);
    ck_assert_uint_eq(UA_Server_addReference(server, otherParent,
                                             UA_NS0ID(ORGANIZES),
                                             UA_EXPANDEDNODEID_NODEID(child), true),
                      UA_STATUSCODE_GOOD);
    setPermissions(parent, UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_DELETENODE);
    setPermissions(child, UA_PERMISSIONTYPE_BROWSE);
    UA_Session *session = createSessionWithRole(UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN);

    ck_assert_uint_eq(deleteNodeAs(session, parent), UA_STATUSCODE_GOOD);
    ck_assert(!nodeExists(parent));
    ck_assert(nodeExists(child));
    ck_assert(hasReferenceTo(otherParent, child));
    ck_assert(!hasReferenceTo(child, parent));
} END_TEST

/*************/
/* Add Nodes */
/*************/

static UA_StatusCode
addObjectAs(UA_Session *session, const char *name, const UA_NodeId parentId,
            const UA_NodeId typeId, UA_NodeId *outNodeId) {
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", (char*)(uintptr_t)name);
    UA_AddNodesItem item;
    UA_AddNodesItem_init(&item);
    item.parentNodeId.nodeId = parentId;
    item.referenceTypeId = UA_NS0ID(HASCOMPONENT);
    item.requestedNewNodeId.nodeId = UA_NODEID_STRING(1, (char*)(uintptr_t)name);
    item.browseName = UA_QUALIFIEDNAME(1, (char*)(uintptr_t)name);
    item.nodeClass = UA_NODECLASS_OBJECT;
    item.typeDefinition.nodeId = typeId;
    UA_ExtensionObject_setValueNoDelete(&item.nodeAttributes, &attr,
                                        &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES]);
    UA_AddNodesRequest request;
    UA_AddNodesRequest_init(&request);
    request.nodesToAddSize = 1;
    request.nodesToAdd = &item;

    UA_AddNodesResponse response;
    UA_AddNodesResponse_init(&response);
    lockServer(server);
    Service_AddNodes(server, session, &request, &response);
    unlockServer(server);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(response.resultsSize, 1);
    UA_StatusCode res = response.results[0].statusCode;
    if(outNodeId)
        UA_NodeId_copy(&response.results[0].addedNodeId, outNodeId);
    UA_AddNodesResponse_clear(&response);
    return res;
}

/* The parent gets a Reference to the new Node. Without AddReference on the
 * parent the Node is not added. */
START_TEST(addNodes_refusedWithoutParentAddReference) {
    UA_NodeId parent = addObject("Parent", UA_NS0ID(OBJECTSFOLDER),
                                 UA_NS0ID(ORGANIZES), 0);
    setPermissions(parent, UA_PERMISSIONTYPE_BROWSE);
    UA_Session *session = createSessionWithRole(UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN);

    UA_NodeId child = UA_NODEID_STRING(1, "Child");
    ck_assert_uint_eq(addObjectAs(session, "Child", parent,
                                  UA_NS0ID(BASEOBJECTTYPE), NULL),
                      UA_STATUSCODE_BADUSERACCESSDENIED);
    ck_assert(!nodeExists(child));
    ck_assert(!hasReferenceTo(parent, child));
} END_TEST

START_TEST(addNodes_grantedWithParentAddReference) {
    UA_NodeId parent = addObject("Parent", UA_NS0ID(OBJECTSFOLDER),
                                 UA_NS0ID(ORGANIZES), 0);
    setPermissions(parent, UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_ADDREFERENCE);
    UA_Session *session = createSessionWithRole(UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN);

    UA_NodeId child = UA_NODEID_STRING(1, "Child");
    ck_assert_uint_eq(addObjectAs(session, "Child", parent,
                                  UA_NS0ID(BASEOBJECTTYPE), NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert(nodeExists(child));
    ck_assert(hasReferenceTo(parent, child));
} END_TEST

/* Adding the Reference from the parent also needs a SecureChannel that
 * satisfies the AccessRestrictions of the parent, as for AddReferences. The
 * parent Reference is added with the Session, which enforces them, and the
 * Node is removed again. */
START_TEST(addNodes_refusedByParentAccessRestrictions) {
    UA_NodeId parent = addObject("Parent", UA_NS0ID(OBJECTSFOLDER),
                                 UA_NS0ID(ORGANIZES), 0);
    setPermissions(parent, UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_ADDREFERENCE);
    ck_assert_uint_eq(UA_Server_setNodeAccessRestrictions(server, parent,
                          UA_ACCESSRESTRICTIONTYPE_ENCRYPTIONREQUIRED),
                      UA_STATUSCODE_GOOD);
    UA_Session *session = createSessionWithRole(UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN);

    UA_NodeId child = UA_NODEID_STRING(1, "Child");
    ck_assert_uint_eq(addObjectAs(session, "Child", parent,
                                  UA_NS0ID(BASEOBJECTTYPE), NULL),
                      UA_STATUSCODE_BADSECURITYMODEINSUFFICIENT);
    ck_assert(!nodeExists(child));
    ck_assert(!hasReferenceTo(parent, child));
} END_TEST

/* The children of an instantiated type are referenced from the new instance,
 * not from the parent. AddReference on the parent is sufficient. The type and
 * its InstanceDeclarations only allow Browse. */
START_TEST(addNodes_instantiationNeedsOnlyParentAddReference) {
    UA_ObjectTypeAttributes typeAttr = UA_ObjectTypeAttributes_default;
    typeAttr.displayName = UA_LOCALIZEDTEXT("en-US", "DeviceType");
    UA_NodeId typeId = UA_NODEID_STRING(1, "DeviceType");
    ck_assert_uint_eq(UA_Server_addObjectTypeNode(server, typeId,
                          UA_NS0ID(BASEOBJECTTYPE), UA_NS0ID(HASSUBTYPE),
                          UA_QUALIFIEDNAME(1, "DeviceType"), typeAttr,
                          NULL, NULL), UA_STATUSCODE_GOOD);

    UA_VariableAttributes varAttr = UA_VariableAttributes_default;
    varAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Status");
    UA_NodeId statusDecl = UA_NODEID_STRING(1, "DeviceType.Status");
    ck_assert_uint_eq(UA_Server_addVariableNode(server, statusDecl, typeId,
                          UA_NS0ID(HASCOMPONENT), UA_QUALIFIEDNAME(1, "Status"),
                          UA_NS0ID(BASEDATAVARIABLETYPE), varAttr, NULL, NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_addReference(server, statusDecl,
                          UA_NS0ID(HASMODELLINGRULE),
                          UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY),
                          true), UA_STATUSCODE_GOOD);

    UA_MethodAttributes methodAttr = UA_MethodAttributes_default;
    methodAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Reset");
    methodAttr.executable = true;
    UA_NodeId resetDecl = UA_NODEID_STRING(1, "DeviceType.Reset");
    ck_assert_uint_eq(UA_Server_addMethodNode(server, resetDecl, typeId,
                          UA_NS0ID(HASCOMPONENT), UA_QUALIFIEDNAME(1, "Reset"),
                          methodAttr, NULL, 0, NULL, 0, NULL, NULL, NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_addReference(server, resetDecl,
                          UA_NS0ID(HASMODELLINGRULE),
                          UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY),
                          true), UA_STATUSCODE_GOOD);

    setPermissions(typeId, UA_PERMISSIONTYPE_BROWSE);
    setPermissions(statusDecl, UA_PERMISSIONTYPE_BROWSE);
    setPermissions(resetDecl, UA_PERMISSIONTYPE_BROWSE);

    UA_NodeId parent = addObject("Parent", UA_NS0ID(OBJECTSFOLDER),
                                 UA_NS0ID(ORGANIZES), 0);
    setPermissions(parent, UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_ADDREFERENCE);
    UA_Session *session = createSessionWithRole(UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN);

    UA_NodeId device = UA_NODEID_NULL;
    ck_assert_uint_eq(addObjectAs(session, "Device", parent, typeId, &device),
                      UA_STATUSCODE_GOOD);
    ck_assert(nodeExists(device));
    ck_assert(hasReferenceTo(parent, device));
    ck_assert(hasReferenceTo(device, resetDecl));

    /* The mandatory Variable was instantiated */
    UA_QualifiedName statusName = UA_QUALIFIEDNAME(1, "Status");
    UA_BrowsePathResult bpr =
        UA_Server_browseSimplifiedBrowsePath(server, device, 1, &statusName);
    ck_assert_uint_eq(bpr.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(bpr.targetsSize, 1);
    ck_assert(!UA_NodeId_equal(&bpr.targets[0].targetId.nodeId, &statusDecl));
    UA_BrowsePathResult_clear(&bpr);
    UA_NodeId_clear(&device);
} END_TEST

/**************************/
/* Event MonitoredItems   */
/**************************/

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
static UA_StatusCode
createEventItemAs(UA_Session *session, const UA_NodeId nodeId) {
    UA_CreateSubscriptionRequest subRequest;
    UA_CreateSubscriptionRequest_init(&subRequest);
    subRequest.publishingEnabled = true;
    UA_CreateSubscriptionResponse subResponse;
    UA_CreateSubscriptionResponse_init(&subResponse);
    lockServer(server);
    Service_CreateSubscription(server, session, &subRequest, &subResponse);
    unlockServer(server);
    ck_assert_uint_eq(subResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    UA_QualifiedName message = UA_QUALIFIEDNAME(0, "Message");
    UA_SimpleAttributeOperand select;
    UA_SimpleAttributeOperand_init(&select);
    select.typeDefinitionId = UA_NS0ID(BASEEVENTTYPE);
    select.browsePathSize = 1;
    select.browsePath = &message;
    select.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);
    filter.selectClausesSize = 1;
    filter.selectClauses = &select;

    UA_MonitoredItemCreateRequest item;
    UA_MonitoredItemCreateRequest_init(&item);
    item.itemToMonitor.nodeId = nodeId;
    item.itemToMonitor.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
    item.monitoringMode = UA_MONITORINGMODE_REPORTING;
    UA_ExtensionObject_setValue(&item.requestedParameters.filter, &filter,
                                &UA_TYPES[UA_TYPES_EVENTFILTER]);
    UA_CreateMonitoredItemsRequest request;
    UA_CreateMonitoredItemsRequest_init(&request);
    request.subscriptionId = subResponse.subscriptionId;
    request.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    request.itemsToCreateSize = 1;
    request.itemsToCreate = &item;

    UA_CreateMonitoredItemsResponse response;
    UA_CreateMonitoredItemsResponse_init(&response);
    lockServer(server);
    Service_CreateMonitoredItems(server, session, &request, &response);
    unlockServer(server);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(response.resultsSize, 1);
    UA_StatusCode res = response.results[0].statusCode;
    UA_CreateMonitoredItemsResponse_clear(&response);
    UA_CreateSubscriptionResponse_clear(&subResponse);
    return res;
}

/* The EventNotifier of a node the Session may not browse cannot be read. The
 * reason for that is returned. */
START_TEST(eventItem_unbrowsableNode) {
    UA_NodeId nodeId = addObject("Notifier", UA_NS0ID(OBJECTSFOLDER),
                                 UA_NS0ID(ORGANIZES),
                                 UA_EVENTNOTIFIER_SUBSCRIBE_TO_EVENT);
    setPermissions(nodeId, UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_RECEIVEEVENTS);
    UA_Session *session =
        createSessionWithRole(UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER);
    ck_assert_uint_eq(createEventItemAs(session, nodeId),
                      UA_STATUSCODE_BADUSERACCESSDENIED);
} END_TEST

START_TEST(eventItem_encryptionRequired) {
    UA_NodeId nodeId = addObject("Notifier", UA_NS0ID(OBJECTSFOLDER),
                                 UA_NS0ID(ORGANIZES),
                                 UA_EVENTNOTIFIER_SUBSCRIBE_TO_EVENT);
    setPermissions(nodeId, UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_RECEIVEEVENTS);
    ck_assert_uint_eq(UA_Server_setNodeAccessRestrictions(server, nodeId,
                          UA_ACCESSRESTRICTIONTYPE_ENCRYPTIONREQUIRED),
                      UA_STATUSCODE_GOOD);
    UA_Session *session = createSessionWithRole(UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN);
    ck_assert_uint_eq(createEventItemAs(session, nodeId),
                      UA_STATUSCODE_BADSECURITYMODEINSUFFICIENT);
} END_TEST

/* Control: with Browse and without restrictions the item is created */
START_TEST(eventItem_browsableNode) {
    UA_NodeId nodeId = addObject("Notifier", UA_NS0ID(OBJECTSFOLDER),
                                 UA_NS0ID(ORGANIZES),
                                 UA_EVENTNOTIFIER_SUBSCRIBE_TO_EVENT);
    setPermissions(nodeId, UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_RECEIVEEVENTS);
    UA_Session *session = createSessionWithRole(UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN);
    ck_assert_uint_eq(createEventItemAs(session, nodeId), UA_STATUSCODE_GOOD);
} END_TEST
#endif /* UA_ENABLE_SUBSCRIPTIONS_EVENTS */

static Suite *testSuite_rbacServices(void) {
    Suite *s = suite_create("RBAC Services");

    TCase *tc_delete = tcase_create("DeleteNodes");
    tcase_add_checked_fixture(tc_delete, setup, teardown);
    tcase_add_test(tc_delete, deleteNodes_recursive_refusedWithoutChildPermission);
    tcase_add_test(tc_delete, deleteNodes_recursive_grantedWithChildPermission);
    tcase_add_test(tc_delete, deleteNodes_recursive_childEncryptionRequired);
    tcase_add_test(tc_delete, deleteNodes_removesIncomingReferences);
    tcase_add_test(tc_delete, deleteNodes_childWithOtherParentNotCollected);
    suite_add_tcase(s, tc_delete);

    TCase *tc_add = tcase_create("AddNodes");
    tcase_add_checked_fixture(tc_add, setup, teardown);
    tcase_add_test(tc_add, addNodes_refusedWithoutParentAddReference);
    tcase_add_test(tc_add, addNodes_grantedWithParentAddReference);
    tcase_add_test(tc_add, addNodes_refusedByParentAccessRestrictions);
    tcase_add_test(tc_add, addNodes_instantiationNeedsOnlyParentAddReference);
    suite_add_tcase(s, tc_add);

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    TCase *tc_events = tcase_create("Event MonitoredItems");
    tcase_add_checked_fixture(tc_events, setup, teardown);
    tcase_add_test(tc_events, eventItem_unbrowsableNode);
    tcase_add_test(tc_events, eventItem_encryptionRequired);
    tcase_add_test(tc_events, eventItem_browsableNode);
    suite_add_tcase(s, tc_events);
#endif

    return s;
}

int main(void) {
    Suite *s = testSuite_rbacServices();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
