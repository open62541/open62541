/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

/* RBAC interlocked-permission tests
 *
 * Compact regression suite for OPC UA Part 3 v1.05, §8.55 PermissionType
 * semantics. The tests verify that permission bits are independent and that
 * interlocked operations keep their conjunctive checks (notably Call on
 * Object+Method and ReceiveEvents on EventType+SourceNode). Coverage also
 * includes history and node-management permission mappings via the default
 * AccessControl callbacks.
 *
 * NOTE on ReceiveEvents (bit 11): the bit is now enforced in
 * src/server/ua_subscription_event.c::createEvent via the internal
 * helper getEffectivePermissions(). The unit-level tests below
 * validate the data path the enforcement reads from (independent
 * storage on EventType vs. SourceNode); a full client-subscription
 * end-to-end test belongs in check_server_rbac_client.c.
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/nodeids.h>

#include "test_helpers.h"
#include "ua_server_rbac.h"

#include <check.h>
#include <stdio.h>
#include <stdlib.h>

static UA_Server *server = NULL;

/* The well-known admin session id installed by UA_Server_new() so test
 * code can act as that session via the session-attribute API. */
static const UA_NodeId adminSessionId =
    {0, UA_NODEIDTYPE_GUID, {.guid = {1, 0, 0, {0,0,0,0,0,0,0,0}}}};

static void setup(void) {
    server = UA_Server_new();
    ck_assert(server != NULL);
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
}

static void teardown(void) {
    if(server) {
        UA_Server_delete(server);
        server = NULL;
    }
}

/* --------------------------------------------------------------------- */
/* Helpers                                                               */
/* --------------------------------------------------------------------- */

/* Add a custom role with AuthenticatedUser identity mapping. */
static UA_NodeId
addRole(const char *name) {
    UA_Role role;
    UA_Role_init(&role);
    role.roleName = UA_QUALIFIEDNAME(0, (char*)(uintptr_t)name);

    UA_NodeId roleId;
    UA_StatusCode r = UA_Server_addRole(server, &role, &roleId);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);

    UA_Role upd;
    r = UA_Server_getRoleById(server, roleId, &upd);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
    upd.identityMappingRules = (UA_IdentityMappingRuleType*)
        UA_calloc(1, sizeof(UA_IdentityMappingRuleType));
    ck_assert_ptr_nonnull(upd.identityMappingRules);
    upd.identityMappingRules[0].criteriaType =
        UA_IDENTITYCRITERIATYPE_AUTHENTICATEDUSER;
    upd.identityMappingRulesSize = 1;
    r = UA_Server_updateRole(server, &upd);
    UA_Role_clear(&upd);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);

    return roleId;
}

/* Make `roleId` the only role currently active on the admin session. */
static void
assignRoleToAdminSession(const UA_NodeId roleId) {
    UA_NodeId tmp = roleId;
    UA_Variant v;
    UA_Variant_setArray(&v, &tmp, 1, &UA_TYPES[UA_TYPES_NODEID]);
    UA_StatusCode r =
        UA_Server_setSessionAttribute(server, &adminSessionId,
                                      UA_QUALIFIEDNAME(0, "roles"), &v);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
}

static void
clearAdminSessionRoles(void) {
    (void)UA_Server_deleteSessionAttribute(server, &adminSessionId,
                                           UA_QUALIFIEDNAME(0, "roles"));
}

/* Add a Variable child of ObjectsFolder. */
static UA_NodeId
addVariable(const char *browseName) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", (char*)(uintptr_t)browseName);
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attr.userAccessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attr.writeMask = 0xFFFFFFFF;
    UA_Int32 v = 0;
    UA_Variant_setScalar(&attr.value, &v, &UA_TYPES[UA_TYPES_INT32]);

    UA_NodeId out;
    UA_StatusCode r = UA_Server_addVariableNode(server,
                          UA_NODEID_NULL,
                          UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                          UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                          UA_QUALIFIEDNAME(1, (char*)(uintptr_t)browseName),
                          UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                          attr, NULL, &out);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
    return out;
}

/* Add an Object child of ObjectsFolder (as a Method container). */
static UA_NodeId
addObject(const char *browseName) {
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", (char*)(uintptr_t)browseName);
    UA_NodeId out;
    UA_StatusCode r = UA_Server_addObjectNode(server,
                          UA_NODEID_NULL,
                          UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                          UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                          UA_QUALIFIEDNAME(1, (char*)(uintptr_t)browseName),
                          UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                          attr, NULL, &out);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
    return out;
}

#ifdef UA_ENABLE_METHODCALLS
static UA_StatusCode
dummyMethodCb(UA_Server *s, const UA_NodeId *sId, void *sCtx,
              const UA_NodeId *mId, void *mCtx,
              const UA_NodeId *oId, void *oCtx,
              size_t inSz, const UA_Variant *in,
              size_t outSz, UA_Variant *out) {
    (void)s; (void)sId; (void)sCtx; (void)mId; (void)mCtx;
    (void)oId; (void)oCtx; (void)inSz; (void)in; (void)outSz; (void)out;
    return UA_STATUSCODE_GOOD;
}

/* Add a Method on `parentObjectId`. */
static UA_NodeId
addMethod(UA_NodeId parentObjectId, const char *browseName) {
    UA_MethodAttributes attr = UA_MethodAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", (char*)(uintptr_t)browseName);
    attr.executable = true;
    attr.userExecutable = true;
    UA_NodeId out;
    UA_StatusCode r = UA_Server_addMethodNode(server, UA_NODEID_NULL,
                          parentObjectId,
                          UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                          UA_QUALIFIEDNAME(1, (char*)(uintptr_t)browseName),
                          attr, dummyMethodCb,
                          0, NULL, 0, NULL, NULL, &out);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
    return out;
}
#endif

/* --------------------------------------------------------------------- */
/* 1. Call interlock — Part 3 §8.55 bit 12                               */
/*    "...the Method if this bit is set on the Object or ObjectType Node */
/*    passed in the Call request and the Method Instance associated      */
/*    with that Object or ObjectType."                                   */
/* --------------------------------------------------------------------- */

#ifdef UA_ENABLE_METHODCALLS
START_TEST(Call_interlock_objectOnly) {
    UA_NodeId roleId = addRole("CallRole_objOnly");
    assignRoleToAdminSession(roleId);

    UA_NodeId obj = addObject("CallObj_objOnly");
    UA_NodeId mth = addMethod(obj, "CallMth_objOnly");

    /* CALL granted on object, but only BROWSE on method (RBAC engaged
     * for the method node, but no CALL bit). */
    ck_assert_uint_eq(UA_Server_addRolePermissions(server, obj, roleId,
        UA_PERMISSIONTYPE_CALL, false, false), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_addRolePermissions(server, mth, roleId,
        UA_PERMISSIONTYPE_BROWSE, false, false), UA_STATUSCODE_GOOD);

    UA_AccessControl *ac = &UA_Server_getConfig(server)->accessControl;
    UA_Boolean exec = ac->getUserExecutableOnObject(server, ac,
        &adminSessionId, NULL, &mth, NULL, &obj, NULL);
    ck_assert_msg(exec == false,
        "Spec §8.55: CALL on object alone must NOT permit method invocation");

    clearAdminSessionRoles();
    UA_Server_deleteNode(server, mth, true);
    UA_Server_deleteNode(server, obj, true);
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "CallRole_objOnly"));
    UA_NodeId_clear(&roleId);
}
END_TEST

START_TEST(Call_interlock_methodOnly) {
    UA_NodeId roleId = addRole("CallRole_mthOnly");
    assignRoleToAdminSession(roleId);

    UA_NodeId obj = addObject("CallObj_mthOnly");
    UA_NodeId mth = addMethod(obj, "CallMth_mthOnly");

    /* CALL granted on method only, only BROWSE on object. */
    ck_assert_uint_eq(UA_Server_addRolePermissions(server, obj, roleId,
        UA_PERMISSIONTYPE_BROWSE, false, false), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_addRolePermissions(server, mth, roleId,
        UA_PERMISSIONTYPE_CALL, false, false), UA_STATUSCODE_GOOD);

    UA_AccessControl *ac = &UA_Server_getConfig(server)->accessControl;
    UA_Boolean exec = ac->getUserExecutableOnObject(server, ac,
        &adminSessionId, NULL, &mth, NULL, &obj, NULL);
    ck_assert_msg(exec == false,
        "Spec §8.55: CALL on method alone must NOT permit method invocation");

    clearAdminSessionRoles();
    UA_Server_deleteNode(server, mth, true);
    UA_Server_deleteNode(server, obj, true);
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "CallRole_mthOnly"));
    UA_NodeId_clear(&roleId);
}
END_TEST

START_TEST(Call_interlock_both) {
    UA_NodeId roleId = addRole("CallRole_both");
    assignRoleToAdminSession(roleId);

    UA_NodeId obj = addObject("CallObj_both");
    UA_NodeId mth = addMethod(obj, "CallMth_both");

    ck_assert_uint_eq(UA_Server_addRolePermissions(server, obj, roleId,
        UA_PERMISSIONTYPE_CALL, false, false), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_addRolePermissions(server, mth, roleId,
        UA_PERMISSIONTYPE_CALL, false, false), UA_STATUSCODE_GOOD);

    UA_AccessControl *ac = &UA_Server_getConfig(server)->accessControl;
    UA_Boolean exec = ac->getUserExecutableOnObject(server, ac,
        &adminSessionId, NULL, &mth, NULL, &obj, NULL);
    ck_assert_msg(exec == true,
        "Spec §8.55: CALL on both object and method must permit invocation");

    clearAdminSessionRoles();
    UA_Server_deleteNode(server, mth, true);
    UA_Server_deleteNode(server, obj, true);
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "CallRole_both"));
    UA_NodeId_clear(&roleId);
}
END_TEST

START_TEST(Call_interlock_neither) {
    UA_NodeId roleId = addRole("CallRole_neither");
    assignRoleToAdminSession(roleId);

    UA_NodeId obj = addObject("CallObj_neither");
    UA_NodeId mth = addMethod(obj, "CallMth_neither");

    /* RBAC engaged on both nodes (with BROWSE only) so no node falls
     * back to the 0xFFFFFFFF "no entries" permissive default. */
    ck_assert_uint_eq(UA_Server_addRolePermissions(server, obj, roleId,
        UA_PERMISSIONTYPE_BROWSE, false, false), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_addRolePermissions(server, mth, roleId,
        UA_PERMISSIONTYPE_BROWSE, false, false), UA_STATUSCODE_GOOD);

    UA_AccessControl *ac = &UA_Server_getConfig(server)->accessControl;
    UA_Boolean exec = ac->getUserExecutableOnObject(server, ac,
        &adminSessionId, NULL, &mth, NULL, &obj, NULL);
    ck_assert_msg(exec == false,
        "Spec §8.55: no CALL on either node => method invocation denied");

    clearAdminSessionRoles();
    UA_Server_deleteNode(server, mth, true);
    UA_Server_deleteNode(server, obj, true);
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "CallRole_neither"));
    UA_NodeId_clear(&roleId);
}
END_TEST
#endif /* UA_ENABLE_METHODCALLS */

/* --------------------------------------------------------------------- */
/* 2. Read / Write / Browse / ReadRolePermissions are independent bits.  */
/*    Part 3 §8.55 bits 0/1/5/6.                                         */
/* --------------------------------------------------------------------- */

START_TEST(Browse_does_not_imply_Read) {
    UA_NodeId roleId = addRole("BrowseOnlyRole");
    assignRoleToAdminSession(roleId);

    UA_NodeId v = addVariable("VarBrowseOnly");
    ck_assert_uint_eq(UA_Server_addRolePermissions(server, v, roleId,
        UA_PERMISSIONTYPE_BROWSE, false, false), UA_STATUSCODE_GOOD);

    UA_AccessControl *ac = &UA_Server_getConfig(server)->accessControl;
    UA_Byte ual = ac->getUserAccessLevel(server, ac,
        &adminSessionId, NULL, &v, NULL);
    ck_assert_msg((ual & UA_ACCESSLEVELMASK_READ) == 0,
        "Spec §8.55 bit 5: BROWSE alone must not grant CurrentRead "
        "(got UserAccessLevel=0x%02x)", ual);
    ck_assert_msg((ual & UA_ACCESSLEVELMASK_WRITE) == 0,
        "BROWSE alone must not grant CurrentWrite");

    UA_Boolean canBrowse = ac->allowBrowseNode(server, ac,
        &adminSessionId, NULL, &v, NULL);
    ck_assert_msg(canBrowse == true,
        "BROWSE bit must allow allowBrowseNode");

    clearAdminSessionRoles();
    UA_Server_deleteNode(server, v, true);
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "BrowseOnlyRole"));
    UA_NodeId_clear(&roleId);
}
END_TEST

START_TEST(Read_does_not_imply_Browse) {
    UA_NodeId roleId = addRole("ReadOnlyRole");
    assignRoleToAdminSession(roleId);

    UA_NodeId v = addVariable("VarReadOnly");
    ck_assert_uint_eq(UA_Server_addRolePermissions(server, v, roleId,
        UA_PERMISSIONTYPE_READ, false, false), UA_STATUSCODE_GOOD);

    UA_AccessControl *ac = &UA_Server_getConfig(server)->accessControl;
    UA_Byte ual = ac->getUserAccessLevel(server, ac,
        &adminSessionId, NULL, &v, NULL);
    ck_assert_msg((ual & UA_ACCESSLEVELMASK_READ) != 0,
        "Spec §8.55 bit 5: READ must drive CurrentRead "
        "(got UserAccessLevel=0x%02x)", ual);

    UA_Boolean canBrowse = ac->allowBrowseNode(server, ac,
        &adminSessionId, NULL, &v, NULL);
    ck_assert_msg(canBrowse == false,
        "Spec §8.55 bit 0: READ alone must not grant Browse "
        "(got allowBrowseNode=%d)", (int)canBrowse);

    clearAdminSessionRoles();
    UA_Server_deleteNode(server, v, true);
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "ReadOnlyRole"));
    UA_NodeId_clear(&roleId);
}
END_TEST

START_TEST(ReadRolePermissions_distinct_from_Read) {
    UA_NodeId roleId = addRole("ReadValueOnlyRole");
    assignRoleToAdminSession(roleId);

    UA_NodeId v = addVariable("VarReadValueOnly");
    /* Grant READ + BROWSE but NOT ReadRolePermissions. */
    ck_assert_uint_eq(UA_Server_addRolePermissions(server, v, roleId,
        UA_PERMISSIONTYPE_READ | UA_PERMISSIONTYPE_BROWSE,
        false, false), UA_STATUSCODE_GOOD);

    UA_PermissionType eff = 0;
    ck_assert_uint_eq(UA_Server_getEffectivePermissions(server,
        &adminSessionId, &v, &eff), UA_STATUSCODE_GOOD);
    ck_assert_msg((eff & UA_PERMISSIONTYPE_READROLEPERMISSIONS) == 0,
        "Spec §8.55 bit 1: READ must not imply ReadRolePermissions");

    /* The plugin must not advertise WriteRolePermissions in WriteMask. */
    UA_AccessControl *ac = &UA_Server_getConfig(server)->accessControl;
    UA_UInt32 uwm = ac->getUserRightsMask(server, ac,
        &adminSessionId, NULL, &v, NULL);
    ck_assert_msg((uwm & UA_WRITEMASK_ROLEPERMISSIONS) == 0,
        "READ must not imply WriteRolePermissions (UserWriteMask=0x%08x)",
        uwm);

    clearAdminSessionRoles();
    UA_Server_deleteNode(server, v, true);
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "ReadValueOnlyRole"));
    UA_NodeId_clear(&roleId);
}
END_TEST

START_TEST(Write_does_not_imply_WriteAttribute) {
    UA_NodeId roleId = addRole("WriteValueOnlyRole");
    assignRoleToAdminSession(roleId);

    UA_NodeId v = addVariable("VarWriteValueOnly");
    /* Grant WRITE (Value) only, not WriteAttribute. */
    ck_assert_uint_eq(UA_Server_addRolePermissions(server, v, roleId,
        UA_PERMISSIONTYPE_WRITE, false, false), UA_STATUSCODE_GOOD);

    UA_AccessControl *ac = &UA_Server_getConfig(server)->accessControl;

    UA_Byte ual = ac->getUserAccessLevel(server, ac,
        &adminSessionId, NULL, &v, NULL);
    ck_assert_msg((ual & UA_ACCESSLEVELMASK_WRITE) != 0,
        "Spec §8.55 bit 6: WRITE must drive CurrentWrite "
        "(got UserAccessLevel=0x%02x)", ual);

    /* WriteMask must be 0 because WriteAttribute bit is not granted. */
    UA_UInt32 uwm = ac->getUserRightsMask(server, ac,
        &adminSessionId, NULL, &v, NULL);
    ck_assert_msg(uwm == 0,
        "Spec §8.55 bit 2: WRITE (Value) must not imply WriteAttribute "
        "(UserWriteMask=0x%08x)", uwm);

    clearAdminSessionRoles();
    UA_Server_deleteNode(server, v, true);
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "WriteValueOnlyRole"));
    UA_NodeId_clear(&roleId);
}
END_TEST

START_TEST(WriteAttribute_does_not_imply_WriteRolePermissions) {
    UA_NodeId roleId = addRole("WriteAttrOnlyRole");
    assignRoleToAdminSession(roleId);

    UA_NodeId v = addVariable("VarWriteAttrOnly");
    /* Only WriteAttribute. WriteMask should expose every writable
     * attribute except RolePermissions (bit 23) and Historizing
     * (bit 9), per Part 3 §8.55. */
    ck_assert_uint_eq(UA_Server_addRolePermissions(server, v, roleId,
        UA_PERMISSIONTYPE_WRITEATTRIBUTE, false, false),
        UA_STATUSCODE_GOOD);

    UA_AccessControl *ac = &UA_Server_getConfig(server)->accessControl;
    UA_UInt32 uwm = ac->getUserRightsMask(server, ac,
        &adminSessionId, NULL, &v, NULL);

    ck_assert_msg((uwm & UA_WRITEMASK_ROLEPERMISSIONS) == 0,
        "Spec §8.55 bit 3: WriteAttribute must NOT imply "
        "WriteRolePermissions (UserWriteMask=0x%08x)", uwm);
    ck_assert_msg((uwm & UA_WRITEMASK_HISTORIZING) == 0,
        "Spec §8.55 bit 4: WriteAttribute must NOT imply "
        "WriteHistorizing (UserWriteMask=0x%08x)", uwm);
    /* But it MUST grant some other writable bit (e.g. DisplayName). */
    ck_assert_msg((uwm & UA_WRITEMASK_DISPLAYNAME) != 0,
        "WriteAttribute must grant DisplayName writability "
        "(UserWriteMask=0x%08x)", uwm);

    clearAdminSessionRoles();
    UA_Server_deleteNode(server, v, true);
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "WriteAttrOnlyRole"));
    UA_NodeId_clear(&roleId);
}
END_TEST

START_TEST(WriteRolePermissions_distinct_bit) {
    UA_NodeId roleId = addRole("WriteRPRole");
    assignRoleToAdminSession(roleId);

    UA_NodeId v = addVariable("VarWriteRP");
    /* Only WriteRolePermissions, nothing else. */
    ck_assert_uint_eq(UA_Server_addRolePermissions(server, v, roleId,
        UA_PERMISSIONTYPE_WRITEROLEPERMISSIONS, false, false),
        UA_STATUSCODE_GOOD);

    UA_AccessControl *ac = &UA_Server_getConfig(server)->accessControl;
    UA_UInt32 uwm = ac->getUserRightsMask(server, ac,
        &adminSessionId, NULL, &v, NULL);

    ck_assert_msg((uwm & UA_WRITEMASK_ROLEPERMISSIONS) != 0,
        "Spec §8.55 bit 3: WriteRolePermissions must set "
        "WriteMask.RolePermissions (UserWriteMask=0x%08x)", uwm);
    ck_assert_msg((uwm & UA_WRITEMASK_DISPLAYNAME) == 0,
        "WriteRolePermissions alone must NOT grant general attribute "
        "writability (UserWriteMask=0x%08x)", uwm);

    clearAdminSessionRoles();
    UA_Server_deleteNode(server, v, true);
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "WriteRPRole"));
    UA_NodeId_clear(&roleId);
}
END_TEST

/* --------------------------------------------------------------------- */
/* 3. Multiple roles ORed together fulfil interlocked requirements.      */
/* --------------------------------------------------------------------- */

START_TEST(MultiRole_OR_satisfies_interlock) {
    UA_NodeId roleA = addRole("PartialA");
    UA_NodeId roleB = addRole("PartialB");

    /* Activate BOTH roles on the session. */
    UA_NodeId roles[2] = {roleA, roleB};
    UA_Variant v;
    UA_Variant_setArray(&v, roles, 2, &UA_TYPES[UA_TYPES_NODEID]);
    ck_assert_uint_eq(UA_Server_setSessionAttribute(server, &adminSessionId,
        UA_QUALIFIEDNAME(0, "roles"), &v), UA_STATUSCODE_GOOD);

    UA_NodeId var = addVariable("VarPartial");
    /* Role A gives BROWSE, role B gives READ. Effective should be both. */
    ck_assert_uint_eq(UA_Server_addRolePermissions(server, var, roleA,
        UA_PERMISSIONTYPE_BROWSE, false, false), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_addRolePermissions(server, var, roleB,
        UA_PERMISSIONTYPE_READ, false, false), UA_STATUSCODE_GOOD);

    UA_PermissionType eff = 0;
    ck_assert_uint_eq(UA_Server_getEffectivePermissions(server,
        &adminSessionId, &var, &eff), UA_STATUSCODE_GOOD);
    ck_assert_msg((eff & UA_PERMISSIONTYPE_BROWSE) &&
                  (eff & UA_PERMISSIONTYPE_READ),
        "OR over the session's roles must combine BROWSE+READ "
        "(effective=0x%08x)", eff);

    clearAdminSessionRoles();
    UA_Server_deleteNode(server, var, true);
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "PartialA"));
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "PartialB"));
    UA_NodeId_clear(&roleA);
    UA_NodeId_clear(&roleB);
}
END_TEST

/* --------------------------------------------------------------------- */
/* 4. ReceiveEvents (Part 3 §8.55 bit 11) — interlocked on EventType +   */
/*    SourceNode. As of this commit the bit is enforced by               */
/*    src/server/ua_subscription_event.c::createEvent which calls the    */
/*    internal getEffectivePermissions() helper for both the             */
/*    EventType and the SourceNode of every event being delivered to a   */
/*    non-admin session, skipping the MonitoredItem when either node is  */
/*    RBAC-restricted and lacks the bit.                                 */
/*                                                                       */
/*    A full end-to-end subscription test would require spinning up a    */
/*    networked client (see check_server_rbac_client.c). The unit-level  */
/*    coverage below validates the data path the enforcement reads from: */
/*    each node must independently expose the RECEIVEEVENTS bit, and a   */
/*    role with the bit on only one of the two nodes must NOT yield the  */
/*    bit on the other.                                                  */
/* --------------------------------------------------------------------- */

START_TEST(ReceiveEvents_storedSeparatelyOnEventTypeAndSource) {
    UA_NodeId roleId = addRole("EventRole");
    assignRoleToAdminSession(roleId);

    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId etype  = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);

    /* Grant RECEIVEEVENTS only on the EventType, not on the source. */
    ck_assert_uint_eq(UA_Server_addRolePermissions(server, etype, roleId,
        UA_PERMISSIONTYPE_RECEIVEEVENTS, false, false),
        UA_STATUSCODE_GOOD);

    UA_PermissionType effEt = 0, effSrc = 0;
    ck_assert_uint_eq(UA_Server_getEffectivePermissions(server,
        &adminSessionId, &etype, &effEt), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_getEffectivePermissions(server,
        &adminSessionId, &source, &effSrc), UA_STATUSCODE_GOOD);

    ck_assert_msg((effEt & UA_PERMISSIONTYPE_RECEIVEEVENTS) != 0,
        "Spec §8.55 bit 11: bit must be readable on the EventType "
        "(eff=0x%08x)", effEt);
    /* The source has no entries -> UA_PERMISSIONTYPE_ALL (permissive). The
     * createEvent enforcement treats UA_PERMISSIONTYPE_ALL as "no RBAC entries
     * configured" and so allows delivery; the bit is NOT inferred from
     * the EventType. */
    ck_assert_msg(effSrc == UA_PERMISSIONTYPE_ALL ||
                  !(effSrc & UA_PERMISSIONTYPE_RECEIVEEVENTS) ||
                  (effSrc & UA_PERMISSIONTYPE_RECEIVEEVENTS),
        "Source effective perms must be independently retrievable "
        "(eff=0x%08x)", effSrc);

    /* Now restrict the source to BROWSE only — no RECEIVEEVENTS. */
    ck_assert_uint_eq(UA_Server_addRolePermissions(server, source, roleId,
        UA_PERMISSIONTYPE_BROWSE, false, false), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_getEffectivePermissions(server,
        &adminSessionId, &source, &effSrc), UA_STATUSCODE_GOOD);
    ck_assert_msg(effSrc != UA_PERMISSIONTYPE_ALL &&
                  !(effSrc & UA_PERMISSIONTYPE_RECEIVEEVENTS),
        "Source must NOT inherit RECEIVEEVENTS from the EventType "
        "(effSrc=0x%08x)", effSrc);

    /* Cleanup the bits we set on well-known nodes. */
    (void)UA_Server_removeRolePermissions(server, etype, roleId,
        UA_PERMISSIONTYPE_RECEIVEEVENTS, false);
    (void)UA_Server_removeRolePermissions(server, source, roleId,
        UA_PERMISSIONTYPE_BROWSE, false);

    clearAdminSessionRoles();
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "EventRole"));
    UA_NodeId_clear(&roleId);
}
END_TEST

START_TEST(ReceiveEvents_grantedOnBothPropagates) {
    UA_NodeId roleId = addRole("EventRoleBoth");
    assignRoleToAdminSession(roleId);

    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId etype  = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);

    ck_assert_uint_eq(UA_Server_addRolePermissions(server, etype, roleId,
        UA_PERMISSIONTYPE_RECEIVEEVENTS, false, false),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_addRolePermissions(server, source, roleId,
        UA_PERMISSIONTYPE_RECEIVEEVENTS, false, false),
        UA_STATUSCODE_GOOD);

    UA_PermissionType effEt = 0, effSrc = 0;
    ck_assert_uint_eq(UA_Server_getEffectivePermissions(server,
        &adminSessionId, &etype, &effEt), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_getEffectivePermissions(server,
        &adminSessionId, &source, &effSrc), UA_STATUSCODE_GOOD);

    ck_assert_msg((effEt  & UA_PERMISSIONTYPE_RECEIVEEVENTS) != 0,
        "EventType missing RECEIVEEVENTS (eff=0x%08x)", effEt);
    ck_assert_msg((effSrc & UA_PERMISSIONTYPE_RECEIVEEVENTS) != 0,
        "Source missing RECEIVEEVENTS (eff=0x%08x)", effSrc);

    (void)UA_Server_removeRolePermissions(server, etype, roleId,
        UA_PERMISSIONTYPE_RECEIVEEVENTS, false);
    (void)UA_Server_removeRolePermissions(server, source, roleId,
        UA_PERMISSIONTYPE_RECEIVEEVENTS, false);

    clearAdminSessionRoles();
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "EventRoleBoth"));
    UA_NodeId_clear(&roleId);
}
END_TEST

/* --------------------------------------------------------------------- */
/* 5. WriteHistorizing (Part 3 §8.55 bit 4) — exposes the Historizing    */
/*    bit in the WriteMask, distinct from WriteAttribute (bit 2).        */
/* --------------------------------------------------------------------- */

START_TEST(WriteHistorizing_distinct_bit) {
    UA_NodeId roleId = addRole("WriteHistRole");
    assignRoleToAdminSession(roleId);

    UA_NodeId v = addVariable("VarWriteHist");
    ck_assert_uint_eq(UA_Server_addRolePermissions(server, v, roleId,
        UA_PERMISSIONTYPE_WRITEHISTORIZING, false, false),
        UA_STATUSCODE_GOOD);

    UA_AccessControl *ac = &UA_Server_getConfig(server)->accessControl;
    UA_UInt32 uwm = ac->getUserRightsMask(server, ac,
        &adminSessionId, NULL, &v, NULL);

    ck_assert_msg((uwm & UA_WRITEMASK_HISTORIZING) != 0,
        "Spec §8.55 bit 4: WriteHistorizing must set "
        "WriteMask.Historizing (UserWriteMask=0x%08x)", uwm);
    ck_assert_msg((uwm & UA_WRITEMASK_DISPLAYNAME) == 0,
        "WriteHistorizing alone must NOT grant general WriteAttribute "
        "(UserWriteMask=0x%08x)", uwm);

    clearAdminSessionRoles();
    UA_Server_deleteNode(server, v, true);
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "WriteHistRole"));
    UA_NodeId_clear(&roleId);
}
END_TEST

/* --------------------------------------------------------------------- */
/* 6. History permission bits (Part 3 §8.55 bits 7..10).                 */
/*    ReadHistory drives UserAccessLevel.HistoryRead;                    */
/*    Insert/Modify/DeleteHistory drive UserAccessLevel.HistoryWrite.    */
/* --------------------------------------------------------------------- */

START_TEST(ReadHistory_setsHistoryReadAccessLevel) {
    UA_NodeId roleId = addRole("HistReadRole");
    assignRoleToAdminSession(roleId);

    UA_NodeId v = addVariable("VarHistRead");
    ck_assert_uint_eq(UA_Server_addRolePermissions(server, v, roleId,
        UA_PERMISSIONTYPE_READHISTORY, false, false),
        UA_STATUSCODE_GOOD);

    UA_AccessControl *ac = &UA_Server_getConfig(server)->accessControl;
    UA_Byte ual = ac->getUserAccessLevel(server, ac,
        &adminSessionId, NULL, &v, NULL);

    ck_assert_msg((ual & UA_ACCESSLEVELMASK_HISTORYREAD) != 0,
        "Spec §8.55 bit 7: ReadHistory must set HistoryRead "
        "(UserAccessLevel=0x%02x)", ual);
    ck_assert_msg((ual & UA_ACCESSLEVELMASK_HISTORYWRITE) == 0,
        "ReadHistory must NOT set HistoryWrite "
        "(UserAccessLevel=0x%02x)", ual);
    ck_assert_msg((ual & UA_ACCESSLEVELMASK_READ) == 0,
        "ReadHistory must NOT imply CurrentRead "
        "(UserAccessLevel=0x%02x)", ual);

    clearAdminSessionRoles();
    UA_Server_deleteNode(server, v, true);
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "HistReadRole"));
    UA_NodeId_clear(&roleId);
}
END_TEST

START_TEST(InsertHistory_setsHistoryWriteAccessLevel) {
    UA_NodeId roleId = addRole("HistInsertRole");
    assignRoleToAdminSession(roleId);

    UA_NodeId v = addVariable("VarHistInsert");
    ck_assert_uint_eq(UA_Server_addRolePermissions(server, v, roleId,
        UA_PERMISSIONTYPE_INSERTHISTORY, false, false),
        UA_STATUSCODE_GOOD);

    UA_AccessControl *ac = &UA_Server_getConfig(server)->accessControl;
    UA_Byte ual = ac->getUserAccessLevel(server, ac,
        &adminSessionId, NULL, &v, NULL);

    ck_assert_msg((ual & UA_ACCESSLEVELMASK_HISTORYWRITE) != 0,
        "Spec §8.55 bit 8: InsertHistory must set HistoryWrite "
        "(UserAccessLevel=0x%02x)", ual);
    ck_assert_msg((ual & UA_ACCESSLEVELMASK_HISTORYREAD) == 0,
        "InsertHistory must NOT set HistoryRead "
        "(UserAccessLevel=0x%02x)", ual);

    clearAdminSessionRoles();
    UA_Server_deleteNode(server, v, true);
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "HistInsertRole"));
    UA_NodeId_clear(&roleId);
}
END_TEST

START_TEST(ModifyHistory_setsHistoryWriteAccessLevel) {
    UA_NodeId roleId = addRole("HistModifyRole");
    assignRoleToAdminSession(roleId);

    UA_NodeId v = addVariable("VarHistModify");
    ck_assert_uint_eq(UA_Server_addRolePermissions(server, v, roleId,
        UA_PERMISSIONTYPE_MODIFYHISTORY, false, false),
        UA_STATUSCODE_GOOD);

    UA_AccessControl *ac = &UA_Server_getConfig(server)->accessControl;
    UA_Byte ual = ac->getUserAccessLevel(server, ac,
        &adminSessionId, NULL, &v, NULL);

    ck_assert_msg((ual & UA_ACCESSLEVELMASK_HISTORYWRITE) != 0,
        "Spec §8.55 bit 9: ModifyHistory must set HistoryWrite "
        "(UserAccessLevel=0x%02x)", ual);

    clearAdminSessionRoles();
    UA_Server_deleteNode(server, v, true);
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "HistModifyRole"));
    UA_NodeId_clear(&roleId);
}
END_TEST

START_TEST(DeleteHistory_setsHistoryWriteAccessLevel) {
    UA_NodeId roleId = addRole("HistDeleteRole");
    assignRoleToAdminSession(roleId);

    UA_NodeId v = addVariable("VarHistDelete");
    ck_assert_uint_eq(UA_Server_addRolePermissions(server, v, roleId,
        UA_PERMISSIONTYPE_DELETEHISTORY, false, false),
        UA_STATUSCODE_GOOD);

    UA_AccessControl *ac = &UA_Server_getConfig(server)->accessControl;
    UA_Byte ual = ac->getUserAccessLevel(server, ac,
        &adminSessionId, NULL, &v, NULL);

    ck_assert_msg((ual & UA_ACCESSLEVELMASK_HISTORYWRITE) != 0,
        "Spec §8.55 bit 10: DeleteHistory must set HistoryWrite "
        "(UserAccessLevel=0x%02x)", ual);

    clearAdminSessionRoles();
    UA_Server_deleteNode(server, v, true);
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "HistDeleteRole"));
    UA_NodeId_clear(&roleId);
}
END_TEST

/* --------------------------------------------------------------------- */
/* 7. Node-management permissions (Part 3 §8.55 bits 13..16).            */
/*    AddReference / RemoveReference are checked on the source node,     */
/*    DeleteNode on the node itself, AddNode on the parent node.         */
/* --------------------------------------------------------------------- */

START_TEST(AddReference_checked_on_source) {
    UA_NodeId roleId = addRole("AddRefRole");
    assignRoleToAdminSession(roleId);

    UA_NodeId src = addVariable("AddRefSource");
    UA_NodeId tgt = addVariable("AddRefTarget");

    /* Only BROWSE on source — no AddReference bit. */
    ck_assert_uint_eq(UA_Server_addRolePermissions(server, src, roleId,
        UA_PERMISSIONTYPE_BROWSE, false, false), UA_STATUSCODE_GOOD);

    UA_AddReferencesItem item;
    UA_AddReferencesItem_init(&item);
    item.sourceNodeId = src;
    item.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    item.isForward = true;
    item.targetNodeId.nodeId = tgt;

    UA_AccessControl *ac = &UA_Server_getConfig(server)->accessControl;
    UA_Boolean allowed = ac->allowAddReference(server, ac,
        &adminSessionId, NULL, &item);
    ck_assert_msg(allowed == false,
        "Spec §8.55 bit 13: AddReference must be denied without bit "
        "on source (allowAddReference returned true)");

    /* Now grant AddReference — should be allowed. */
    ck_assert_uint_eq(UA_Server_addRolePermissions(server, src, roleId,
        UA_PERMISSIONTYPE_ADDREFERENCE, false, false),
        UA_STATUSCODE_GOOD);
    allowed = ac->allowAddReference(server, ac,
        &adminSessionId, NULL, &item);
    ck_assert_msg(allowed == true,
        "AddReference must be allowed once bit 13 is granted on source");

    clearAdminSessionRoles();
    UA_Server_deleteNode(server, src, true);
    UA_Server_deleteNode(server, tgt, true);
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "AddRefRole"));
    UA_NodeId_clear(&roleId);
}
END_TEST

START_TEST(RemoveReference_checked_on_source) {
    UA_NodeId roleId = addRole("RemRefRole");
    assignRoleToAdminSession(roleId);

    UA_NodeId src = addVariable("RemRefSource");
    UA_NodeId tgt = addVariable("RemRefTarget");
    ck_assert_uint_eq(UA_Server_addRolePermissions(server, src, roleId,
        UA_PERMISSIONTYPE_BROWSE, false, false), UA_STATUSCODE_GOOD);

    UA_DeleteReferencesItem item;
    UA_DeleteReferencesItem_init(&item);
    item.sourceNodeId = src;
    item.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    item.isForward = true;
    item.targetNodeId.nodeId = tgt;
    item.deleteBidirectional = true;

    UA_AccessControl *ac = &UA_Server_getConfig(server)->accessControl;
    UA_Boolean allowed = ac->allowDeleteReference(server, ac,
        &adminSessionId, NULL, &item);
    ck_assert_msg(allowed == false,
        "Spec §8.55 bit 14: RemoveReference must be denied without bit "
        "on source (allowDeleteReference returned true)");

    ck_assert_uint_eq(UA_Server_addRolePermissions(server, src, roleId,
        UA_PERMISSIONTYPE_REMOVEREFERENCE, false, false),
        UA_STATUSCODE_GOOD);
    allowed = ac->allowDeleteReference(server, ac,
        &adminSessionId, NULL, &item);
    ck_assert_msg(allowed == true,
        "RemoveReference must be allowed once bit 14 is granted");

    clearAdminSessionRoles();
    UA_Server_deleteNode(server, src, true);
    UA_Server_deleteNode(server, tgt, true);
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "RemRefRole"));
    UA_NodeId_clear(&roleId);
}
END_TEST

START_TEST(DeleteNode_checked_on_target) {
    UA_NodeId roleId = addRole("DelNodeRole");
    assignRoleToAdminSession(roleId);

    UA_NodeId v = addVariable("VarToDelete");
    ck_assert_uint_eq(UA_Server_addRolePermissions(server, v, roleId,
        UA_PERMISSIONTYPE_BROWSE, false, false), UA_STATUSCODE_GOOD);

    UA_DeleteNodesItem item;
    UA_DeleteNodesItem_init(&item);
    item.nodeId = v;
    item.deleteTargetReferences = true;

    UA_AccessControl *ac = &UA_Server_getConfig(server)->accessControl;
    UA_Boolean allowed = ac->allowDeleteNode(server, ac,
        &adminSessionId, NULL, &item);
    ck_assert_msg(allowed == false,
        "Spec §8.55 bit 15: DeleteNode must be denied without bit "
        "on the target node (allowDeleteNode returned true)");

    ck_assert_uint_eq(UA_Server_addRolePermissions(server, v, roleId,
        UA_PERMISSIONTYPE_DELETENODE, false, false),
        UA_STATUSCODE_GOOD);
    allowed = ac->allowDeleteNode(server, ac,
        &adminSessionId, NULL, &item);
    ck_assert_msg(allowed == true,
        "DeleteNode must be allowed once bit 15 is granted");

    clearAdminSessionRoles();
    UA_Server_deleteNode(server, v, true);
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "DelNodeRole"));
    UA_NodeId_clear(&roleId);
}
END_TEST

START_TEST(AddNode_checked_on_parent) {
    UA_NodeId roleId = addRole("AddNodeRole");
    assignRoleToAdminSession(roleId);

    /* Parent is an Object child of ObjectsFolder, RBAC engaged with
     * BROWSE only. */
    UA_NodeId parent = addObject("AddNodeParent");
    ck_assert_uint_eq(UA_Server_addRolePermissions(server, parent, roleId,
        UA_PERMISSIONTYPE_BROWSE, false, false), UA_STATUSCODE_GOOD);

    UA_AddNodesItem item;
    UA_AddNodesItem_init(&item);
    item.parentNodeId.nodeId = parent;
    item.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    item.requestedNewNodeId.nodeId = UA_NODEID_NULL;
    item.browseName = UA_QUALIFIEDNAME(1, "ChildVar");
    item.nodeClass = UA_NODECLASS_VARIABLE;
    item.typeDefinition.nodeId =
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);

    UA_AccessControl *ac = &UA_Server_getConfig(server)->accessControl;
    UA_Boolean allowed = ac->allowAddNode(server, ac,
        &adminSessionId, NULL, &item);
    ck_assert_msg(allowed == false,
        "Spec §8.55 bit 16: AddNode must be denied without bit on the "
        "parent (allowAddNode returned true)");

    ck_assert_uint_eq(UA_Server_addRolePermissions(server, parent, roleId,
        UA_PERMISSIONTYPE_ADDNODE, false, false),
        UA_STATUSCODE_GOOD);
    allowed = ac->allowAddNode(server, ac,
        &adminSessionId, NULL, &item);
    ck_assert_msg(allowed == true,
        "AddNode must be allowed once bit 16 is granted on parent");

    clearAdminSessionRoles();
    UA_Server_deleteNode(server, parent, true);
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "AddNodeRole"));
    UA_NodeId_clear(&roleId);
}
END_TEST

/* --------------------------------------------------------------------- */
/* Suite                                                                 */
/* --------------------------------------------------------------------- */

static Suite *testSuite(void) {
    Suite *s = suite_create("Server RBAC Interlocked Permissions");

#ifdef UA_ENABLE_METHODCALLS
    TCase *tc_call = tcase_create("Call interlock (object + method)");
    tcase_add_unchecked_fixture(tc_call, setup, teardown);
    tcase_add_test(tc_call, Call_interlock_objectOnly);
    tcase_add_test(tc_call, Call_interlock_methodOnly);
    tcase_add_test(tc_call, Call_interlock_both);
    tcase_add_test(tc_call, Call_interlock_neither);
    suite_add_tcase(s, tc_call);
#endif

    TCase *tc_bits = tcase_create("Permission bits are independent");
    tcase_add_unchecked_fixture(tc_bits, setup, teardown);
    tcase_add_test(tc_bits, Browse_does_not_imply_Read);
    tcase_add_test(tc_bits, Read_does_not_imply_Browse);
    tcase_add_test(tc_bits, ReadRolePermissions_distinct_from_Read);
    tcase_add_test(tc_bits, Write_does_not_imply_WriteAttribute);
    tcase_add_test(tc_bits, WriteAttribute_does_not_imply_WriteRolePermissions);
    tcase_add_test(tc_bits, WriteRolePermissions_distinct_bit);
    tcase_add_test(tc_bits, WriteHistorizing_distinct_bit);
    suite_add_tcase(s, tc_bits);

    TCase *tc_hist = tcase_create("History permission bits");
    tcase_add_unchecked_fixture(tc_hist, setup, teardown);
    tcase_add_test(tc_hist, ReadHistory_setsHistoryReadAccessLevel);
    tcase_add_test(tc_hist, InsertHistory_setsHistoryWriteAccessLevel);
    tcase_add_test(tc_hist, ModifyHistory_setsHistoryWriteAccessLevel);
    tcase_add_test(tc_hist, DeleteHistory_setsHistoryWriteAccessLevel);
    suite_add_tcase(s, tc_hist);

    TCase *tc_nm = tcase_create("Node-management permission bits");
    tcase_add_unchecked_fixture(tc_nm, setup, teardown);
    tcase_add_test(tc_nm, AddReference_checked_on_source);
    tcase_add_test(tc_nm, RemoveReference_checked_on_source);
    tcase_add_test(tc_nm, DeleteNode_checked_on_target);
    tcase_add_test(tc_nm, AddNode_checked_on_parent);
    suite_add_tcase(s, tc_nm);

    TCase *tc_multi = tcase_create("Multi-role OR semantics");
    tcase_add_unchecked_fixture(tc_multi, setup, teardown);
    tcase_add_test(tc_multi, MultiRole_OR_satisfies_interlock);
    suite_add_tcase(s, tc_multi);

    TCase *tc_evt = tcase_create("ReceiveEvents interlock");
    tcase_add_unchecked_fixture(tc_evt, setup, teardown);
    tcase_add_test(tc_evt, ReceiveEvents_storedSeparatelyOnEventTypeAndSource);
    tcase_add_test(tc_evt, ReceiveEvents_grantedOnBothPropagates);
    suite_add_tcase(s, tc_evt);

    return s;
}

int main(void) {
    Suite *s = testSuite();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
