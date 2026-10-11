/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

/* RBAC enforcement in the services, called with in-process Sessions without a
 * SecureChannel */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/nodeids.h>

#include "test_helpers.h"
#include "test_helpers_rbac.h"

#include <check.h>
#include <stdlib.h>

#define CONFIGUREADMIN UA_NS0ID(WELLKNOWNROLE_CONFIGUREADMIN)
#define OBSERVER UA_NS0ID(WELLKNOWNROLE_OBSERVER)

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
    UA_Session *session = rbacSession(server, NULL, CONFIGUREADMIN);

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
    UA_Session *session = rbacSession(server, NULL, CONFIGUREADMIN);

    ck_assert_uint_eq(deleteNodeAs(session, parent), UA_STATUSCODE_GOOD);
    ck_assert(!nodeExists(parent));
    ck_assert(!nodeExists(child));
    ck_assert(!hasReferenceTo(UA_NS0ID(OBJECTSFOLDER), parent));
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
    UA_Session *session = rbacSession(server, NULL, CONFIGUREADMIN);
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
    UA_Session *session = rbacSession(server, NULL, CONFIGUREADMIN);

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
    setPermissions(parent, UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_ADDNODE);
    UA_Session *session = rbacSession(server, NULL, CONFIGUREADMIN);

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
    setPermissions(parent, UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_ADDNODE |
                   UA_PERMISSIONTYPE_ADDREFERENCE);
    UA_Session *session = rbacSession(server, NULL, CONFIGUREADMIN);

    UA_NodeId child = UA_NODEID_STRING(1, "Child");
    ck_assert_uint_eq(addObjectAs(session, "Child", parent,
                                  UA_NS0ID(BASEOBJECTTYPE), NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert(nodeExists(child));
    ck_assert(hasReferenceTo(parent, child));
} END_TEST

/* The children of an instantiated type are referenced from the new instance,
 * not from the parent. AddNode and AddReference on the parent are sufficient.
 * The type and its InstanceDeclarations only allow Browse. */
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
    setPermissions(parent, UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_ADDNODE |
                   UA_PERMISSIONTYPE_ADDREFERENCE);
    UA_Session *session = rbacSession(server, NULL, CONFIGUREADMIN);

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

/**********/
/* Browse */
/**********/

/* Whether the forward references of the node list the target */
static UA_Boolean
browseFinds(UA_Session *session, const UA_NodeId nodeId, const UA_NodeId target) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = nodeId;
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.resultMask = UA_BROWSERESULTMASK_ALL;
    UA_BrowseRequest request;
    UA_BrowseRequest_init(&request);
    request.nodesToBrowseSize = 1;
    request.nodesToBrowse = &bd;

    UA_BrowseResponse response;
    UA_BrowseResponse_init(&response);
    lockServer(server);
    Service_Browse(server, session, &request, &response);
    unlockServer(server);
    ck_assert_uint_eq(response.resultsSize, 1);
    ck_assert_uint_eq(response.results[0].statusCode, UA_STATUSCODE_GOOD);
    UA_Boolean found = false;
    for(size_t i = 0; i < response.results[0].referencesSize; i++)
        found |= UA_NodeId_equal(&response.results[0].references[i].nodeId.nodeId,
                                 &target);
    UA_BrowseResponse_clear(&response);
    return found;
}

/* Browsing the parent does not list a child without the Browse permission.
 * The local admin Session sees it. */
START_TEST(browse_hidesTargetWithoutBrowse) {
    UA_NodeId parent = addObject("Parent", UA_NS0ID(OBJECTSFOLDER),
                                 UA_NS0ID(ORGANIZES), 0);
    UA_NodeId visible = addObject("Visible", parent, UA_NS0ID(HASCOMPONENT), 0);
    UA_NodeId hidden = addObject("Hidden", parent, UA_NS0ID(HASCOMPONENT), 0);
    setPermissions(parent, UA_PERMISSIONTYPE_BROWSE);
    setPermissions(visible, UA_PERMISSIONTYPE_BROWSE);
    setPermissions(hidden, 0);
    UA_Session *session = rbacSession(server, NULL, CONFIGUREADMIN);

    ck_assert(browseFinds(session, parent, visible));
    ck_assert(!browseFinds(session, parent, hidden));
    ck_assert(browseFinds(&server->adminSession, parent, hidden));
} END_TEST

/**************************/
/* Event MonitoredItems   */
/**************************/

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
/* The EventNotifier of a node the Session may not browse cannot be read. The
 * reason for that is returned. */
START_TEST(eventItem_needsBrowse) {
    UA_NodeId nodeId = addObject("Notifier", UA_NS0ID(OBJECTSFOLDER),
                                 UA_NS0ID(ORGANIZES),
                                 UA_EVENTNOTIFIER_SUBSCRIBE_TO_EVENT);
    setPermissions(nodeId, UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_RECEIVEEVENTS);
    UA_StatusCode res;
    rbacMonitor(server, rbacSession(server, NULL, OBSERVER), nodeId,
                UA_ATTRIBUTEID_EVENTNOTIFIER, "Message", &res);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADUSERACCESSDENIED);
    rbacMonitor(server, rbacSession(server, NULL, CONFIGUREADMIN), nodeId,
                UA_ATTRIBUTEID_EVENTNOTIFIER, "Message", &res);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

/* An Event is delivered only with ReceiveEvents on its EventType and on its
 * SourceNode */
START_TEST(eventDelivery_needsReceiveEvents) {
    UA_NodeId source = addObject("Source", UA_NS0ID(OBJECTSFOLDER),
                                 UA_NS0ID(ORGANIZES), 0);
    UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
    UA_NodeId eventType = UA_NODEID_STRING(1, "TestEventType");
    ck_assert_uint_eq(UA_Server_addObjectTypeNode(server, eventType,
                          UA_NS0ID(BASEEVENTTYPE), UA_NS0ID(HASSUBTYPE),
                          UA_QUALIFIEDNAME(1, "TestEventType"), attr, NULL, NULL),
                      UA_STATUSCODE_GOOD);
    UA_StatusCode res;
    UA_MonitoredItem *mon = rbacMonitor(server, rbacSession(server, NULL, OBSERVER),
                                        UA_NS0ID(SERVER), UA_ATTRIBUTEID_EVENTNOTIFIER,
                                        "Message", &res);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    const UA_PermissionType receive = UA_PERMISSIONTYPE_BROWSE |
        UA_PERMISSIONTYPE_RECEIVEEVENTS;
    const UA_PermissionType cases[3][2] = { /* {SourceNode, EventType} */
        {UA_PERMISSIONTYPE_BROWSE, receive}, {receive, UA_PERMISSIONTYPE_BROWSE},
        {receive, receive}};
    for(size_t i = 0; i < 3; i++) {
        UA_RolePermission rp = {OBSERVER, cases[i][0]};
        ck_assert_uint_eq(UA_Server_setNodeRolePermissions(server, source, 1, &rp,
                                                           false, NULL),
                          UA_STATUSCODE_GOOD);
        rp.permissions = cases[i][1];
        ck_assert_uint_eq(UA_Server_setNodeRolePermissions(server, eventType, 1, &rp,
                                                           false, NULL),
                          UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(UA_Server_createEvent(server, source, eventType, 100,
                                                UA_LOCALIZEDTEXT("", "event"),
                                                NULL, NULL, NULL),
                          UA_STATUSCODE_GOOD);
        lockServer(server);
        size_t queued = mon->queueSize;
        unlockServer(server);
        ck_assert_uint_eq(queued, i == 2 ? 1 : 0);
    }
} END_TEST
#endif /* UA_ENABLE_SUBSCRIPTIONS_EVENTS */

/********************/
/* User Attributes  */
/********************/

/* UserAccessLevel, UserWriteMask and UserExecutable never exceed the
 * attributes of the Node */
START_TEST(userAttributes_cappedByNodeAttributes) {
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    vAttr.accessLevel = UA_ACCESSLEVELMASK_READ;
    vAttr.writeMask = UA_WRITEMASK_DISPLAYNAME;
    UA_NodeId var = UA_NODEID_STRING(1, "Var");
    ck_assert_uint_eq(UA_Server_addVariableNode(server, var, UA_NS0ID(OBJECTSFOLDER),
                          UA_NS0ID(ORGANIZES), UA_QUALIFIEDNAME(1, "Var"),
                          UA_NS0ID(BASEDATAVARIABLETYPE), vAttr, NULL, NULL),
                      UA_STATUSCODE_GOOD);
    UA_MethodAttributes mAttr = UA_MethodAttributes_default;
    mAttr.executable = false;
    UA_NodeId method = UA_NODEID_STRING(1, "Method");
    ck_assert_uint_eq(UA_Server_addMethodNode(server, method, UA_NS0ID(OBJECTSFOLDER),
                          UA_NS0ID(HASCOMPONENT), UA_QUALIFIEDNAME(1, "Method"),
                          mAttr, NULL, 0, NULL, 0, NULL, NULL, NULL),
                      UA_STATUSCODE_GOOD);
    setPermissions(var, UA_PERMISSIONTYPE_ALL);
    setPermissions(method, UA_PERMISSIONTYPE_ALL);
    UA_Session *session = rbacSession(server, NULL, CONFIGUREADMIN);

    const UA_NodeId nodes[3] = {var, var, method};
    const UA_UInt32 attributes[3] = {UA_ATTRIBUTEID_USERACCESSLEVEL,
        UA_ATTRIBUTEID_USERWRITEMASK, UA_ATTRIBUTEID_USEREXECUTABLE};
    const UA_UInt32 expected[3] = {UA_ACCESSLEVELMASK_READ, UA_WRITEMASK_DISPLAYNAME, 0};
    for(size_t i = 0; i < 3; i++) {
        UA_ReadValueId rvi;
        UA_ReadValueId_init(&rvi);
        rvi.nodeId = nodes[i];
        rvi.attributeId = attributes[i];
        UA_DataValue dv;
        UA_DataValue_init(&dv);
        lockServer(server);
        Operation_Read(server, session, UA_TIMESTAMPSTORETURN_NEITHER, &rvi, &dv);
        unlockServer(server);
        ck_assert(!dv.hasStatus);
        UA_UInt32 value = (i == 0) ? *(UA_Byte*)dv.value.data :
            (i == 1) ? *(UA_UInt32*)dv.value.data : *(UA_Boolean*)dv.value.data;
        ck_assert_uint_eq(value, expected[i]);
        UA_DataValue_clear(&dv);
    }
} END_TEST

/**********************/
/* RolePermissions    */
/**********************/

/* The local admin Session bypasses all checks, also for the RolePermissions of
 * a Node whose list does not grant ReadRolePermissions */
START_TEST(adminSession_readsRolePermissions) {
    UA_NodeId nodeId = addObject("Protected", UA_NS0ID(OBJECTSFOLDER),
                                 UA_NS0ID(ORGANIZES), 0);
    UA_RolePermission rp = {OBSERVER, UA_PERMISSIONTYPE_BROWSE};
    ck_assert_uint_eq(UA_Server_setNodeRolePermissions(server, nodeId, 1, &rp,
                                                       false, NULL),
                      UA_STATUSCODE_GOOD);
    UA_Variant v;
    ck_assert_uint_eq(UA_Server_readRolePermissions(server, nodeId, &v),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(v.arrayLength, 1);
    UA_Variant_clear(&v);
} END_TEST

#ifdef UA_ENABLE_SUBSCRIPTIONS
/* A detached Subscription samples without a Session and gets no
 * RolePermissions */
START_TEST(detachedSubscription_samplesNoRolePermissions) {
    UA_NodeId nodeId = addObject("Protected", UA_NS0ID(OBJECTSFOLDER),
                                 UA_NS0ID(ORGANIZES), 0);
    setPermissions(nodeId, UA_PERMISSIONTYPE_BROWSE |
                   UA_PERMISSIONTYPE_READROLEPERMISSIONS);
    UA_Session *session = rbacSession(server, NULL, CONFIGUREADMIN);
    UA_StatusCode res;
    UA_MonitoredItem *mon = rbacMonitor(server, session, nodeId,
                                        UA_ATTRIBUTEID_ROLEPERMISSIONS, NULL, &res);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    lockServer(server);
    UA_Boolean attachedReadable =
        !TAILQ_LAST(&mon->queue, NotificationQueue)->data.dataChange.value.hasStatus;
    UA_Session_detachSubscription(server, session, mon->subscription, false);
    UA_MonitoredItem_sample(server, mon);
    UA_StatusCode detachedStatus =
        TAILQ_LAST(&mon->queue, NotificationQueue)->data.dataChange.value.status;
    unlockServer(server);
    ck_assert(attachedReadable);
    ck_assert_uint_eq(detachedStatus, UA_STATUSCODE_BADUSERACCESSDENIED);
} END_TEST
#endif /* UA_ENABLE_SUBSCRIPTIONS */

static Suite *testSuite_rbacServices(void) {
    Suite *s = suite_create("RBAC Services");

    TCase *tc_delete = tcase_create("DeleteNodes");
    tcase_add_checked_fixture(tc_delete, setup, teardown);
    tcase_add_test(tc_delete, deleteNodes_recursive_refusedWithoutChildPermission);
    tcase_add_test(tc_delete, deleteNodes_recursive_grantedWithChildPermission);
    tcase_add_test(tc_delete, deleteNodes_removesIncomingReferences);
    tcase_add_test(tc_delete, deleteNodes_childWithOtherParentNotCollected);
    suite_add_tcase(s, tc_delete);

    TCase *tc_add = tcase_create("AddNodes");
    tcase_add_checked_fixture(tc_add, setup, teardown);
    tcase_add_test(tc_add, addNodes_refusedWithoutParentAddReference);
    tcase_add_test(tc_add, addNodes_grantedWithParentAddReference);
    tcase_add_test(tc_add, addNodes_instantiationNeedsOnlyParentAddReference);
    suite_add_tcase(s, tc_add);

    TCase *tc_browse = tcase_create("Browse");
    tcase_add_checked_fixture(tc_browse, setup, teardown);
    tcase_add_test(tc_browse, browse_hidesTargetWithoutBrowse);
    suite_add_tcase(s, tc_browse);

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    TCase *tc_events = tcase_create("Event MonitoredItems");
    tcase_add_checked_fixture(tc_events, setup, teardown);
    tcase_add_test(tc_events, eventItem_needsBrowse);
    tcase_add_test(tc_events, eventDelivery_needsReceiveEvents);
    suite_add_tcase(s, tc_events);
#endif

    TCase *tc_attr = tcase_create("User Attributes");
    tcase_add_checked_fixture(tc_attr, setup, teardown);
    tcase_add_test(tc_attr, userAttributes_cappedByNodeAttributes);
    suite_add_tcase(s, tc_attr);

    TCase *tc_rp = tcase_create("RolePermissions");
    tcase_add_checked_fixture(tc_rp, setup, teardown);
    tcase_add_test(tc_rp, adminSession_readsRolePermissions);
#ifdef UA_ENABLE_SUBSCRIPTIONS
    tcase_add_test(tc_rp, detachedSubscription_samplesNoRolePermissions);
#endif
    suite_add_tcase(s, tc_rp);

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
