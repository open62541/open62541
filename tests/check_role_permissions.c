/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/accesscontrol_custom.h>
#include <open62541/client_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/client_highlevel.h>
#include "open62541/types.h"
#include "open62541/types_generated.h"

#include <signal.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include <check.h>

#include "thread_wrapper.h"

#define VARLENGTH 16366

THREAD_HANDLE server_thread;

// structure for user Data Queue
typedef struct user_data_buffer {
    bool userAvailable;
    UA_String username;
    UA_String password;
    UA_String role;
    uint64_t GroupID;
    uint64_t userID;
} userDatabase;

UA_Boolean running = 0;
UA_Client *client ;
UA_NodeId outNewNodeId;

UA_ServerConfig *config;
int checkUser(userDatabase *userData);
UA_StatusCode addNewNamespaceandSetDefaultPermission(UA_Server *server);
UA_Boolean checkUserDatabase(const UA_UserNameIdentityToken *userToken, UA_String *roleName);
UA_StatusCode checkTheRoleSessionLoggedIn(UA_Server *server);

void* serverloop(void* server);
UA_PermissionType readUserProvidedRoles(UA_Server *server, UA_AccessControlSettings* accessControlSettings);
UA_NodeId     findIdentityNodeID(UA_Server *server, UA_NodeId startingNode);
UA_StatusCode addNewTestNode(UA_Server *server);
UA_StatusCode addNewTestNodeObject(UA_Server *server);
void* serverRun(void* server);

static UA_Boolean addRoleBasedNodes(UA_Server *server,
                  const UA_NodeId *sessionId,
                  void *sessionContext,
                  const UA_NodeId *sourceNodeId,
                  const UA_NodeId *targetParentNodeId,
                  const UA_NodeId *referenceTypeId)
 {
    UA_NodeId addIdentityNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_ADDIDENTITY);
    UA_NodeId removeIdentityNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_REMOVEIDENTITY);

    //  Add the AddIdentity Method and RemoveIdentity Method, when creating the RoleType Object
    if ((UA_NodeId_equal(sourceNodeId, &addIdentityNodeId)) || (UA_NodeId_equal(sourceNodeId, &removeIdentityNodeId)))
        return UA_TRUE;
    else
        return UA_FALSE;
}

void* serverRun(void* server) {
    UA_Server *s;
    s=(UA_Server *)server;
    UA_Server_run(s, &running);
    return 0;
}

UA_StatusCode addNewTestNodeObject(UA_Server *server) {
    /* Object one */
    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.description = UA_LOCALIZEDTEXT("en-US", "Demo_Object");
    object_attr.displayName = UA_LOCALIZEDTEXT("en-US", "Demo_Object");
    object_attr.rolePermissionsSize = 2;
    object_attr.rolePermissions = (UA_RolePermissionType *)UA_Array_new(object_attr.rolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    object_attr.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    object_attr.rolePermissions[0].permissions = 0x1FFFF;
    object_attr.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    object_attr.rolePermissions[1].permissions = 0x1FFFF;
    object_attr.userRolePermissionsSize = 2;
    object_attr.userRolePermissions = (UA_RolePermissionType *)UA_Array_new(object_attr.rolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    object_attr.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    object_attr.userRolePermissions[0].permissions = 0x1FFFF;
    object_attr.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    object_attr.userRolePermissions[1].permissions = 0x1FFFF;
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 200000),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_QUALIFIEDNAME(1, "Demo_Object"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), object_attr, NULL, NULL);
    UA_Array_delete(object_attr.rolePermissions, object_attr.rolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    UA_Array_delete(object_attr.userRolePermissions, object_attr.userRolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);

    /* Variable one */
    UA_VariableAttributes attributes = UA_VariableAttributes_default;
    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_1");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissionsSize = 2;
    attributes.rolePermissions = (UA_RolePermissionType *)UA_Array_new(attributes.rolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.rolePermissions[0].permissions = 0x67;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissionsSize = 2;
    attributes.userRolePermissions = (UA_RolePermissionType *)UA_Array_new(attributes.rolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.userRolePermissions[0].permissions = 0x1;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x1;
    UA_UInt16 value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 2000), UA_NODEID_NUMERIC(1, 200000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_1"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_2");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.rolePermissions[0].permissions = 0x67;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.userRolePermissions[0].permissions = 0x3;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x3;
    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 2002), UA_NODEID_NUMERIC(1, 200000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_2"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_3");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.rolePermissions[0].permissions = 0x67;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.userRolePermissions[0].permissions = 0x7;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    attributes.userRolePermissions[1].permissions = 0x7;
    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 2003), UA_NODEID_NUMERIC(1, 200000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_3"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_4");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.rolePermissions[0].permissions = 0x67;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.userRolePermissions[0].permissions = 0x27;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x27;
    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 2004), UA_NODEID_NUMERIC(1, 200000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_4"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_5");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.rolePermissions[0].permissions = 0x67;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.userRolePermissions[0].permissions = 0x67;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x67;
    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 2005), UA_NODEID_NUMERIC(1, 200000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_5"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode addNewTestNode(UA_Server *server) {
    /* Object one */
    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.description = UA_LOCALIZEDTEXT("en-US", "Demo_Variables");
    object_attr.displayName = UA_LOCALIZEDTEXT("en-US", "Demo_Variables");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 100000),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_QUALIFIEDNAME(1, "Demo_Variables"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), object_attr, NULL, NULL);

    /* Variable one */
    UA_VariableAttributes attributes = UA_VariableAttributes_default;
    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_1");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissionsSize = 2;
    attributes.rolePermissions = (UA_RolePermissionType *)UA_Array_new(attributes.rolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.rolePermissions[0].permissions = 0x1FFFF; //1ffff
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissionsSize = 2;
    attributes.userRolePermissions = (UA_RolePermissionType *)UA_Array_new(attributes.rolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.userRolePermissions[0].permissions = 0x1;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x1;
    UA_UInt16 value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1000), UA_NODEID_NUMERIC(1, 100000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_1"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_2");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.rolePermissions[0].permissions = 0x1FFFF;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.userRolePermissions[0].permissions = 0x3;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x3;
    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1002), UA_NODEID_NUMERIC(1, 100000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_2"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_3");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.rolePermissions[0].permissions = 0x1FFFF;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.userRolePermissions[0].permissions = 0x7;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x7;
    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1003), UA_NODEID_NUMERIC(1, 100000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_3"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_4");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.rolePermissions[0].permissions = 0x1FFFF;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.userRolePermissions[0].permissions = 0x27;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x27;
    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1004), UA_NODEID_NUMERIC(1, 100000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_4"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_5");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.rolePermissions[0].permissions = 0x1FFFF;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.userRolePermissions[0].permissions = 0x67;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x67;
    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1005), UA_NODEID_NUMERIC(1, 100000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_5"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_6");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER);
    attributes.rolePermissions[0].permissions = 0x1FFFF;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER);
    attributes.userRolePermissions[0].permissions = 0x867;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x867;
    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1006), UA_NODEID_NUMERIC(1, 100000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_6"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);


    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_7");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN);
    attributes.rolePermissions[0].permissions = 0x1FFFF;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN);
    attributes.userRolePermissions[0].permissions = 0x186F;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x1867;
    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1007), UA_NODEID_NUMERIC(1, 100000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_7"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_8");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    attributes.rolePermissions[0].permissions = 0x1FFFF;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    attributes.userRolePermissions[0].permissions = 0x3867;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x1FFFF;
    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1008), UA_NODEID_NUMERIC(1, 100000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_8"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_9");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR);
    attributes.rolePermissions[0].permissions = 0x1FFFF;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR);
    attributes.userRolePermissions[0].permissions = 0x7867;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x1FFFF;
    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1009), UA_NODEID_NUMERIC(1, 100000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_9"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_10");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SUPERVISOR);
    attributes.rolePermissions[0].permissions = 0x1FFFF;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SUPERVISOR);
    attributes.userRolePermissions[0].permissions = 0xF867;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x1FFFF;
    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1010), UA_NODEID_NUMERIC(1, 100000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_10"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_11");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN);
    attributes.rolePermissions[0].permissions = 0x1FFFF;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN);
    attributes.userRolePermissions[0].permissions = 0x1F867;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x1FFFF;
    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1011), UA_NODEID_NUMERIC(1, 100000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_11"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    UA_Array_delete(attributes.rolePermissions, attributes.rolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    UA_Array_delete(attributes.userRolePermissions, attributes.userRolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_12");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissionsSize = 3;
    attributes.rolePermissions = (UA_RolePermissionType *)UA_Array_new(attributes.rolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.rolePermissions[0].permissions = 0x3;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.rolePermissions[2].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN);
    attributes.rolePermissions[2].permissions = 0x1FFFF;

    attributes.userRolePermissionsSize = 3;
    attributes.userRolePermissions = (UA_RolePermissionType *)UA_Array_new(attributes.rolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.userRolePermissions[0].permissions = 0x3;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x3;
    attributes.userRolePermissions[2].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN);
    attributes.userRolePermissions[2].permissions = 0x3;

    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1012), UA_NODEID_NUMERIC(1, 100000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_12"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    UA_Array_delete(attributes.rolePermissions, attributes.rolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    UA_Array_delete(attributes.userRolePermissions, attributes.userRolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_13");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissionsSize = 1;
    attributes.userRolePermissionsSize = 1;
    attributes.rolePermissions = (UA_RolePermissionType *)calloc(attributes.rolePermissionsSize, sizeof(UA_RolePermissionType));
    attributes.userRolePermissions = (UA_RolePermissionType *)calloc(attributes.userRolePermissionsSize, sizeof(UA_RolePermissionType));
    UA_String newRole = UA_STRING("KalycitoWorker");
    attributes.rolePermissions[0].roleId = UA_NODEID_STRING(0, (char *)newRole.data);
    attributes.rolePermissions[0].permissions = 0x1FFFF;
    attributes.userRolePermissions[0].roleId = UA_NODEID_STRING(0, (char *)newRole.data);
    attributes.userRolePermissions[0].permissions = 0x1FFFF;

    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1013), UA_NODEID_NUMERIC(1, 100000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_13"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);
    UA_free(attributes.rolePermissions);
    UA_free(attributes.userRolePermissions);

    return UA_STATUSCODE_GOOD;
}

UA_NodeId
findIdentityNodeID(UA_Server *server, UA_NodeId startingNode) {
    UA_NodeId resultNodeId;
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    rpe.isInverse = false;
    rpe.includeSubtypes = false;
    rpe.targetName = UA_QUALIFIEDNAME(0, "Identities");
    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = startingNode;
    bp.relativePath.elementsSize = 1;
    bp.relativePath.elements = &rpe;
    UA_BrowsePathResult bpr = UA_Server_translateBrowsePathToNodeIds(server, &bp);
    if(bpr.statusCode != UA_STATUSCODE_GOOD ||
       bpr.targetsSize < 1)
        return UA_NODEID_NULL;

    UA_StatusCode res = UA_NodeId_copy(&bpr.targets[0].targetId.nodeId, &resultNodeId);
    if(res != UA_STATUSCODE_GOOD){
        UA_BrowsePathResult_clear(&bpr);
        return UA_NODEID_NULL;
    }

    UA_BrowsePathResult_clear(&bpr);
    return resultNodeId;
}

UA_PermissionType readUserProvidedRoles(UA_Server *server, UA_AccessControlSettings* accessControlSettings)
{
    UA_UInt16 nsIdx = UA_Server_addNamespace(server, "http://yourorganisation.org/test/");
    UA_Variant dims;
    UA_Variant_init(&dims);
    UA_Server_readValue(server, UA_NODEID_STRING(nsIdx, "DefaultUserRolePermissions"), &dims);
    UA_RolePermissionType *p = (UA_RolePermissionType *)dims.data;

    switch(accessControlSettings->accessControlGroup)
    {
        case UA_ANONYMOUS_WELL_KNOWN_RULE: {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "UA_ANONYMOUS_WELL_KNOWN_RULE 0x23 - %0X", p[0].permissions);
            return p[0].permissions;
            break;
        }
        case UA_AUTHENTICATEDUSER_WELL_KNOWN_RULE:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "UA_AUTHENTICATEDUSER_WELL_KNOWN_RULE - %0X", p[1].permissions);
            return p[1].permissions;
            break;
        case UA_CONFIGUREADMIN_WELL_KNOWN_RULE:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "UA_CONFIGUREADMIN_WELL_KNOWN_RULE - %0X", p[2].permissions);
            return p[2].permissions;
            break;
        case UA_ENGINEER_WELL_KNOWN_RULE:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "UA_ENGINEER_WELL_KNOWN_RULE - %0X", p[3].permissions);
            return p[3].permissions;
            break;
        case UA_OBSERVER_WELL_KNOWN_RULE:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "UA_OBSERVER_WELL_KNOWN_RULE - %0X", p[4].permissions);
            return p[4].permissions;
            break;
        case UA_OPERATOR_WELL_KNOWN_RULE:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "UA_OPERATOR_WELL_KNOWN_RULE - %0X", p[5].permissions);
            return p[5].permissions;
            break;
        case UA_SECURITYADMIN_WELL_KNOWN_RULE:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "UA_SECURITYADMIN_WELL_KNOWN_RULE - %0X", p[6].permissions);
            return p[6].permissions;
            break;
        case UA_SUPERVISOR_WELL_KNOWN_RULE:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "UA_SUPERVISOR_WELL_KNOWN_RULE - %0X", p[7].permissions);
            return p[7].permissions;
            break;
        default:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "UNKNOWN ROLE USED");
            return 0x0;
    }
    UA_Variant_clear(&dims);
}

UA_StatusCode checkTheRoleSessionLoggedIn(UA_Server *server){
    printf("\ncheckTheRoleSessionLoggedIn\n");
    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.methodId = UA_NODEID_STRING(1, "SessionRoleIdentification");
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);

    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = UA_Server_call(server, &callMethodRequest);
    if (result.statusCode != UA_STATUSCODE_GOOD)
        printf("\nMethodCall Failed\n");
    else{
    printf("\ncheckTheRoleSessionStartedEnd\n");
    printf("\nOutputArgumentSize:%ld\n", result.outputArgumentsSize);
    UA_String *roleNameSessionLoggedIn = (UA_String *)result.outputArguments[0].data;
    printf("\nroleNameSessionLoggedIn:%s\n", roleNameSessionLoggedIn->data);
    }

    return result.statusCode;
}

UA_StatusCode addNewNamespaceandSetDefaultPermission(UA_Server *server) {
    UA_UInt16 nsIdx = UA_Server_addNamespace(server, "http://yourorganisation.org/test/");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "NameSpaceIndex: %d", nsIdx);
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en-US", "http://yourorganisation.org/test/");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "http://yourorganisation.org/test/");
    UA_Server_addObjectNode(server, UA_NODEID_STRING(nsIdx, "http://yourorganisation.org/test/"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACES), UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(nsIdx, "http://yourorganisation.org/test/"), UA_NODEID_NUMERIC(0, UA_NS0ID_NAMESPACEMETADATATYPE),
                            attr, NULL, &outNewNodeId);

    UA_VariableAttributes attributes = UA_VariableAttributes_default;
    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "DefaultAccessRestrictions");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.dataType = UA_TYPES[UA_TYPES_ACCESSRESTRICTIONTYPE].typeId;
    UA_AccessRestrictionType restrictionValue = 15;
    UA_Variant_setScalar(&attributes.value, &restrictionValue, &UA_TYPES[UA_TYPES_ACCESSRESTRICTIONTYPE]);
    UA_Server_addVariableNode(
        server, UA_NODEID_STRING(nsIdx, "DefaultAccessRestrictions"), outNewNodeId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
        UA_QUALIFIEDNAME(nsIdx, "DefaultAccessRestrictions"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), attributes, NULL, NULL);
    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "DefaultRolePermissions");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.dataType = UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE].typeId;
    UA_RolePermissionType rolePermission[8];
    rolePermission[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    rolePermission[0].permissions = 0x1FFFF;
    rolePermission[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER);
    rolePermission[1].permissions = 0x1FFFF;
    rolePermission[2].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN);
    rolePermission[2].permissions = 0x1FFFF;
    rolePermission[3].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    rolePermission[3].permissions = 0x1FFFF;
    rolePermission[4].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    rolePermission[4].permissions = 0x1FFFF;
    rolePermission[5].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR);
    rolePermission[5].permissions = 0x1FFFF;
    rolePermission[6].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN);
    rolePermission[6].permissions = 0x1FFFF;
    rolePermission[7].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SUPERVISOR);
    rolePermission[7].permissions = 0x1FFFF;
    UA_Variant_setArray(&attributes.value, rolePermission, 8, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    UA_Server_addVariableNode(server, UA_NODEID_STRING(nsIdx, "DefaultRolePermissions"), outNewNodeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                              UA_QUALIFIEDNAME(nsIdx, "DefaultRolePermissions"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), attributes, NULL, NULL);
    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "DefaultUserRolePermissions");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.dataType = UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE].typeId;
    rolePermission[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    rolePermission[0].permissions = 0x23;
    rolePermission[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER);
    rolePermission[1].permissions = 0x1FFFF;
    rolePermission[2].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN);
    rolePermission[2].permissions = 0x1FFFF;
    rolePermission[3].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    rolePermission[3].permissions = 0x1FFFF;
    rolePermission[4].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    rolePermission[4].permissions = 0x1FFFF;
    rolePermission[5].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR);
    rolePermission[5].permissions = 0x1FFFF;
    rolePermission[6].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN);
    rolePermission[6].permissions = 0x1FFFF;
    rolePermission[7].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SUPERVISOR);
    rolePermission[7].permissions = 0x1FFFF;
    UA_Variant_setArray(&attributes.value, rolePermission, 8, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    UA_Server_addVariableNode(server, UA_NODEID_STRING(nsIdx, "DefaultUserRolePermissions"), outNewNodeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                              UA_QUALIFIEDNAME(nsIdx, "DefaultUserRolePermissions"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), attributes, NULL, NULL);

    return UA_STATUSCODE_GOOD;
}

int checkUser (userDatabase* userData){

    char user_database[][256]= {"user_engineer,123456,Engineer,156,145",
                                "user_operator,123456,Operator,157,165",
                                "user_observer,123456,Observer,158,166",
                                "user_securityadmin,123456,SecurityAdmin,159,167",
                                "user_supervisor,123456,Supervisor,160,168",
                                "user_configureadmin,123456,ConfigureAdmin,162,168",
                                "user_authenticated,123456,AuthenticatedUser,163,169",
                                "user_anonymous,123456,Anonymous,164,170",
                                "user_kalycitoworker,123456,KalycitoWorker,165,171"};

    int i;
    char* pass;
    for(i=0;i<10;i++)
    {
       if(strstr(user_database[i], (char*)userData->username.data))
        {
            pass= strtok(user_database[i],",");
            pass= strtok(NULL,",");
            if(strcmp((char*) userData->password.data, pass)==0)
            {
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "User and Password are correct");
                userData->userAvailable = true;

                pass= strtok(NULL,",");

                userData->role.data = (uint8_t*)malloc(strlen(pass));
                strncpy((char *)userData->role.data, pass, strlen(pass));
                userData->role.length =  strlen((char *)userData->role.data);

                pass= strtok(NULL,",");
                int groupID = atoi(pass);
                userData->GroupID = (uint64_t) groupID;

                pass= strtok(NULL,",");
                int userID = atoi(pass);
                userData->userID = (uint64_t) userID;

                pass= NULL;
                break;
            }
            else
            {
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Wrong Password");
                break;
            }
        }
        else
        {
            userData->userAvailable = false;
        }
    }
    return 0;
}

UA_Boolean checkUserDatabase(const UA_UserNameIdentityToken *userToken, UA_String *roleName){
    userDatabase userData;
    userData.userAvailable = false;
    UA_String_copy(&userToken->userName, &userData.username);
    UA_String_copy(&userToken->password, &userData.password);
    checkUser(&userData);
    if (userData.userAvailable != true)
       return false;

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "PlatformUseravailale:%d",userData.userAvailable);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "PlatformUserName:%s", userData.username.data);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "PlatformPassword:%s", userData.password.data);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "PlatformRole:%s", userData.role.data);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "PlatformLength:%ld", userData.role.length);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "PlatformGroupID:%ld", userData.GroupID);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "PlatformUserID:%ld", userData.userID);

    UA_String_copy(&userData.role, roleName);

    if (userData.username.data != NULL)
        free(userData.username.data);

    if (userData.password.data != NULL)
        free(userData.password.data );

    if (userData.password.data != NULL)
        free(userData.role.data );

    return true;
}

//############################TEST_CASES_STARTS##############################

START_TEST(login_user_engineer)
{
    UA_StatusCode retval = UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "user_engineer", "123456");
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Login Success");
    }

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(node1_no_read_access)
{
    UA_NodeId node_1 = UA_NODEID_NUMERIC(1, 1001);
    UA_Variant variant;
    UA_StatusCode retval;

    retval = UA_Client_readValueAttribute(client, node_1, &variant);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadValue of restricted Node 1 FOR ENGINEER");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: ReadValue of restricted Node 1 FOR ENGINEER: %s", UA_StatusCode_name(retval));
    }

    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(node11_read_access)
{
    UA_NodeId node_11 = UA_NODEID_NUMERIC(1, 1011);
    UA_Variant variant;
    UA_StatusCode retval;

    retval = UA_Client_readValueAttribute(client, node_11, &variant);

    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadValue Node 11 FOR ENGINEER");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: ReadValue Node 11 FOR ENGINEER: %s", UA_StatusCode_name(retval));
    }

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}END_TEST

START_TEST(node1_browse_access)
{
    UA_NodeId node_1 = UA_NODEID_NUMERIC(1, 1001);

    /* Browse nodes */
    UA_BrowseRequest browseRequest;
    UA_BrowseRequest_init(&browseRequest);
    UA_BrowseDescription *browseDescription = UA_BrowseDescription_new();
    browseDescription->browseDirection = UA_BROWSEDIRECTION_INVERSE;
    browseDescription->resultMask = UA_BROWSERESULTMASK_ALL;
    browseDescription->nodeId = node_1;
    browseDescription->referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    browseDescription->includeSubtypes = UA_TRUE;
    browseRequest.nodesToBrowseSize = 1;
    browseRequest.nodesToBrowse = browseDescription;
    browseRequest.requestedMaxReferencesPerNode = 1000;

    UA_BrowseResponse browseResponse = UA_Client_Service_browse(client, browseRequest);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT,"Result size: %i result: %s, ref size: %i ",
                (int) browseResponse.resultsSize, UA_StatusCode_name(browseResponse.responseHeader.serviceResult),
                (int) browseResponse.results->referencesSize);

    UA_BrowseResponse_clear(&browseResponse);

    ck_assert_uint_eq(browseResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(node12_read_role_permission)
{
    UA_NodeId node_12 = UA_NODEID_NUMERIC(1, 1012);
    UA_StatusCode retval;
    UA_RolePermissionType *readRolePermission = (UA_RolePermissionType *)UA_Array_new(3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    size_t readRolePermissionsize = 3;
    UA_UInt32 roleValue[3];
    retval = UA_Client_readRolePermissionAttribute(client, node_12, &readRolePermissionsize, &readRolePermission);
    for (size_t index = 0; index < 3; index++) {
        roleValue[index] = readRolePermission[index].permissions;
    }

    if ((roleValue[0] == 0x3) && (roleValue[1] == 0x1ffff) && (roleValue[2] == 0x1ffff)) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadRolePermission of Node 12");
    }
    UA_Array_delete(readRolePermission, 3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(node12_user_read_role_permission)
{
    UA_NodeId node_12 = UA_NODEID_NUMERIC(1, 1012);
    UA_StatusCode retval;
    UA_RolePermissionType *readUserRolePermission = (UA_RolePermissionType *)UA_Array_new(3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);;
    size_t readUserRolePermissionsize = 3;
    UA_UInt32 roleValue[3];
    retval = UA_Client_readUserRolePermissionAttribute(client, node_12, &readUserRolePermissionsize , &readUserRolePermission);
    for (size_t index = 0; index < 3; index++) {
        roleValue[index] = readUserRolePermission[index].permissions;
    }

    if ((roleValue[0] == 0x3) && (roleValue[1] == 0x3) && (roleValue[2] == 0x3)) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadUserRolePermission of Node 12");
    }

    UA_Array_delete(readUserRolePermission, 3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(node12_read_write_role_permission)
{
    UA_NodeId node_12 = UA_NODEID_NUMERIC(1, 1012);
    UA_StatusCode retval;

    UA_UInt32 roleValue[3];

    // Write and Read Role permission
    UA_RolePermissionType * readRolePermission = (UA_RolePermissionType *)UA_Array_new(3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    size_t readRolePermissionsize = 3;
    UA_RolePermissionType *rolePermissions = (UA_RolePermissionType *)UA_Array_new(3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    rolePermissions[0].permissions = 0x21;
    rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    rolePermissions[1].permissions = 0x1FFFF;
    rolePermissions[2].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN);
    rolePermissions[2].permissions = 0x21;
    retval = UA_Client_writeRolePermissionAttribute(client, node_12, 3, rolePermissions);
    UA_Array_delete(rolePermissions, 3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);

    readRolePermission = (UA_RolePermissionType *)UA_Array_new(3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);;
    readRolePermissionsize = 3;
    retval = UA_Client_readRolePermissionAttribute(client, node_12, &readRolePermissionsize , &readRolePermission);
    for (size_t index = 0; index < 3; index++)
        roleValue[index] = readRolePermission[index].permissions;

    if ((roleValue[0] == 0x21) && (roleValue[1] == 0x1ffff) && (roleValue[2] == 0x21)) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Write and Read RolePermission of Node 12");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: Write and Read RolePermission of Node 12");
    }
    UA_Array_delete(readRolePermission, 3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(node12_user_read_write_role_permission)
{
    UA_NodeId node_12 = UA_NODEID_NUMERIC(1, 1012);
    UA_StatusCode retval;

    // Write and UserRead Role permission
    UA_RolePermissionType *userRolePermissions = (UA_RolePermissionType *)UA_Array_new(3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    userRolePermissions[0].permissions = 0x1837;
    userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    userRolePermissions[1].permissions = 0x1FFFF;
    userRolePermissions[2].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN);
    userRolePermissions[2].permissions = 0x1837;
    retval = UA_Client_writeUserRolePermissionAttribute(client, node_12, 3, userRolePermissions);
    UA_Array_delete(userRolePermissions, 3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);

    UA_UInt32 userRoleValue[3];
    UA_RolePermissionType *  readUserRolePermission = (UA_RolePermissionType *)UA_Array_new(3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);;
    size_t readUserRolePermissionsize = 3;
    retval = UA_Client_readUserRolePermissionAttribute(client, node_12, &readUserRolePermissionsize , &readUserRolePermission);
    for (size_t index = 0; index < 3; index++)
        userRoleValue[index] = readUserRolePermission[index].permissions;

    if ((userRoleValue[0] == 0x1837) && (userRoleValue[1] == 0x1ffff) && (userRoleValue[2] == 0x1837)) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Write and Read UserRolePermission of Node 12");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: Write and Read UserRolePermission of Node 12");
    }
    UA_Array_delete(readUserRolePermission, 3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(node11_write_role_permission)
{
    UA_NodeId node_11 = UA_NODEID_NUMERIC(1, 1011);
    UA_StatusCode retval;

    // Write Value
    UA_Variant val;
    UA_UInt16 value = 100;
    UA_Variant_setScalar(&val, &value, &UA_TYPES[UA_TYPES_UINT16]);
    retval = UA_Client_writeValueAttribute(client, node_11, &val);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: WriteValue of Node 11 FOR ENGINEER");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: WriteValue of Node 11 FOR ENGINEER: %s", UA_StatusCode_name(retval));
    }

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(node1_write_role_permission)
{
    UA_NodeId node_1 = UA_NODEID_NUMERIC(1, 1001);
    UA_StatusCode retval;

    // Write Value
    UA_Variant val;
    UA_UInt16 value = 100;
    UA_Variant_setScalar(&val, &value, &UA_TYPES[UA_TYPES_UINT16]);

    retval = UA_Client_writeValueAttribute(client, node_1, &val);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: WriteValue of restricted Node 1 FOR ENGINEER");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: WriteValue of restricted Node 1 FOR ENGINEER: %s", UA_StatusCode_name(retval));
    }
    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(add_new_node_Engineer)
{
    UA_StatusCode retval;

    /* Add new variable */
    UA_VariableAttributes newVariableAttributes = UA_VariableAttributes_default;
    newVariableAttributes.accessLevel = UA_ACCESSLEVELMASK_READ;
    newVariableAttributes.description = UA_LOCALIZEDTEXT_ALLOC("en-US", "NewVariable desc");
    newVariableAttributes.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US", "NewVariable");
    newVariableAttributes.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    UA_UInt16  value = 50;
    UA_Variant_setScalarCopy(&newVariableAttributes.value, &value, &UA_TYPES[UA_TYPES_UINT32]);

    retval = UA_Client_addVariableNode(client, UA_NODEID_NUMERIC(1, 4001),
                                       UA_NODEID_NUMERIC(1, 200000),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                       UA_QUALIFIEDNAME(1, "newVariable_1"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                       newVariableAttributes, NULL);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Add New Node 1 FOR ENGINEER");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: Add New Node 1 FOR ENGINEER: %s", UA_StatusCode_name(retval));
    }
     UA_VariableAttributes_clear(&newVariableAttributes);

     ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(add_new_reference_Engineer)
{
    UA_StatusCode retval;

    /* Add new Reference */
    UA_ExpandedNodeId extNodeId_1 = UA_EXPANDEDNODEID_NUMERIC(0, 0);
    extNodeId_1.nodeId = UA_NODEID_NUMERIC(1, 4001);
    retval = UA_Client_addReference(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_TRUE,
                                    UA_STRING_NULL, extNodeId_1, UA_NODECLASS_VARIABLE);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Add Reference Node 1 FOR ENGINEER");

    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: Add Reference Node 1 FOR ENGINEER: %s", UA_StatusCode_name(retval));
    }
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(delete_new_reference_Engineer)
{

    UA_StatusCode retval;

    UA_ExpandedNodeId extNodeId_1 = UA_EXPANDEDNODEID_NUMERIC(0, 0);
    extNodeId_1.nodeId = UA_NODEID_NUMERIC(1, 4001);
    /* Delete reference */
    retval = UA_Client_deleteReference(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_TRUE, extNodeId_1, UA_TRUE);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Node 1 FOR ENGINEER");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: Node 1 FOR ENGINEER: %s", UA_StatusCode_name(retval));
    }

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(delete_node_as_Engineer)
{
    UA_StatusCode retval;
    retval = UA_Client_deleteNode(client, UA_NODEID_NUMERIC(1, 4001), UA_TRUE);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: delete node FOR ENGINEER");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: delete node FOR ENGINEER: %s", UA_StatusCode_name(retval));
    }
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

#define KALYCITO_WORKER_NODE_ID UA_NODEID_STRING(2, "KalycitoWorker")

START_TEST(method_call_to_add_kalycito_worker_role)
{
    UA_StatusCode retval;

    /* Start Method call to add KalycitoWorker Role */
    UA_NodeId output_Node;
    UA_NodeId kalycitoWorker = KALYCITO_WORKER_NODE_ID;
    UA_String roleName = UA_STRING("KalycitoWorker");
    UA_String namespaceURI = UA_STRING("http://yourorganisation.org/test/");
    UA_Variant *inputArguments = (UA_Variant *) UA_calloc(2, (sizeof(UA_Variant)));
    UA_Variant_setScalar(&inputArguments[0], &roleName, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&inputArguments[1], &namespaceURI, &UA_TYPES[UA_TYPES_STRING]);

    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.inputArgumentsSize = 2;
    callMethodRequest.inputArguments = inputArguments;
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_ROLESET);
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_ROLESETTYPE_ADDROLE);

    UA_CallRequest callRoleMethodCall;
    UA_CallRequest_init(&callRoleMethodCall);
    callRoleMethodCall.methodsToCallSize = 1;
    callRoleMethodCall.methodsToCall = &callMethodRequest;

    UA_CallResponse response = UA_Client_Service_call(client, callRoleMethodCall);

    output_Node =  *((UA_NodeId *) response.results->outputArguments->data);
    if(UA_NodeId_equal(&kalycitoWorker, &output_Node)) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Added KalycitoWorker role to the server");

        UA_free(inputArguments);
        UA_CallResponse_clear(&response);

        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE to Add KalycitoWorker role to the server");
    }

}END_TEST

START_TEST(method_call_to_add_kalycito_worker_identity)
{
    UA_StatusCode retval;

    /* Start Method call to add KalycitoWorker Identity */
    UA_IdentityMappingRuleType identityMappingRule;
    identityMappingRule.criteria = UA_STRING("KalycitoWorker");
    identityMappingRule.criteriaType = UA_IDENTITYCRITERIATYPE_ROLE;

    UA_Variant *inputArguments_2 = (UA_Variant *) UA_calloc(1, (sizeof(UA_Variant)));
    UA_Variant_setScalar(&inputArguments_2[0], &identityMappingRule, &UA_TYPES[UA_TYPES_IDENTITYMAPPINGRULETYPE]);

    UA_CallMethodRequest callMethodRequest_2;
    UA_CallMethodRequest_init(&callMethodRequest_2);
    callMethodRequest_2.inputArgumentsSize = 1;
    callMethodRequest_2.inputArguments = inputArguments_2;
    callMethodRequest_2.objectId = KALYCITO_WORKER_NODE_ID;
    callMethodRequest_2.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_ADDIDENTITY);

    UA_CallRequest callIdentityMethodCall;
    UA_CallRequest_init(&callIdentityMethodCall);
    callIdentityMethodCall.methodsToCallSize = 1;
    callIdentityMethodCall.methodsToCall = &callMethodRequest_2;

    UA_CallResponse response_2 = UA_Client_Service_call(client, callIdentityMethodCall);

    if(response_2.results->statusCode == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Added KalycitoWorker identity to the server");
    /* Clean up */
        UA_free(inputArguments_2);
        UA_CallResponse_clear(&response_2);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE to Add KalycitoWorker identity to the server");
    }


    UA_Client_disconnect(client);    // Disconnect as ENGINEER

}END_TEST

START_TEST(login_as_kalycito_worker)
{

    UA_StatusCode retval = UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "user_kalycitoworker", "123456");
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Login Success");
    }
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(node13_read_kalycito_worker)
{
    UA_StatusCode retval;

    UA_NodeId node_13 = UA_NODEID_NUMERIC(1, 1013);  // Only accessed by KalycitoWorker Role
    UA_Variant variant;

    retval = UA_Client_readValueAttribute(client, node_13, &variant);
    UA_Client_disconnect(client);    // Disconnect as KalycitoWorker

    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);

}END_TEST


START_TEST(login_as_user_operator)
{
    UA_StatusCode retval = UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "user_operator", "123456");
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Login Success");
    }

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}END_TEST

START_TEST(node1_read_user_operator)
{
    UA_NodeId node_1 = UA_NODEID_NUMERIC(1, 1001);
    UA_Variant variant;

    UA_StatusCode retval = UA_Client_readValueAttribute(client, node_1, &variant);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadValue Node 1 FOR OPERATOR");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: ReadValue Node 1 FOR OPERATOR: %s", UA_StatusCode_name(retval));
    }

    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(node9_read_user_operator)
{
    UA_NodeId node_9 = UA_NODEID_NUMERIC(1, 1009);
    UA_Variant variant;

    UA_StatusCode retval = UA_Client_readValueAttribute(client, node_9, &variant);

    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadValue Node 9 FOR OPERATOR");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: ReadValue Node 9 FOR OPERATOR: %s", UA_StatusCode_name(retval));
    }

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(node9_write_user_operator)
{
    UA_NodeId node_9 = UA_NODEID_NUMERIC(1, 1009);
    UA_Variant val;
    UA_UInt16  value = 50;

    UA_Variant_setScalar(&val, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_StatusCode retval = UA_Client_writeValueAttribute(client, node_9, &val);

    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: WriteValue of Node 9 FOR OPERATOR");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: WriteValue of Node 9 FOR OPERATOR: %s", UA_StatusCode_name(retval));
    }

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(read_role_permission_user_operator)
{
    UA_NodeId node_9 = UA_NODEID_NUMERIC(1, 1009);
    UA_UInt32 roleValue[3];

    // Write and Read Role permission
    UA_RolePermissionType *readRolePermission = (UA_RolePermissionType *)UA_Array_new(3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);;
    size_t  readRolePermissionsize = 2;

    UA_StatusCode retval = UA_Client_readRolePermissionAttribute(client, node_9, &readRolePermissionsize , &readRolePermission);
    for (size_t index = 0; index < 2; index++)
        roleValue[index] = readRolePermission[index].permissions;

    if ((roleValue[0] == 0x1ffff) && (roleValue[1] == 0x1ffff)) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Write and Read RolePermission of Node 9");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: Write and Read RolePermission of Node 9");
    }
    UA_Array_delete(readRolePermission, 2, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(read_user_role_permission_user_operator)
{
    UA_NodeId node_9 = UA_NODEID_NUMERIC(1, 1009);

    UA_UInt32 roleValue[3];

    UA_RolePermissionType * readUserRolePermission = (UA_RolePermissionType *)UA_Array_new(3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);;
    size_t readUserRolePermissionsize = 2;
    UA_StatusCode retval = UA_Client_readUserRolePermissionAttribute(client, node_9, &readUserRolePermissionsize , &readUserRolePermission);
    for (size_t index = 0; index < 2; index++)
        roleValue[index] = readUserRolePermission[index].permissions;

    if ((roleValue[0] == 0x7867) && (roleValue[1] == 0x1ffff)) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Read UserRolePermission of Node 9");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: Read UserRolePermission of Node 9");
    }
    UA_Array_delete(readUserRolePermission, 2, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    UA_Client_disconnect(client);

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(login_user_observer)
{
    UA_StatusCode retval = UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "user_observer", "123456");
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Login Success");
    }

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(node8_read_user_observer)
{
    UA_NodeId node_8 = UA_NODEID_NUMERIC(1, 1008);
    UA_Variant variant;

    UA_StatusCode retval = UA_Client_readValueAttribute(client, node_8, &variant);

    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadValue Node 8 FOR OBSERVER");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: ReadValue Node 8 FOR OBSERVER: %s", UA_StatusCode_name(retval));
    }

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(node8_write_user_observer)
{
    UA_NodeId node_8 = UA_NODEID_NUMERIC(1, 1008);
    UA_Variant val;
    UA_UInt16  value = 50;

    UA_Variant_setScalar(&val, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_StatusCode retval = UA_Client_writeValueAttribute(client, node_8, &val);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: WriteValue of Node 8 FOR OBSERVER");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: WriteValue of Node 8 FOR OBSERVER: %s", UA_StatusCode_name(retval));
    }
     UA_Client_disconnect(client);
    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(login_user_securityadmin)
{
    UA_StatusCode retval = UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "user_securityadmin", "123456");
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Login Success");
    }

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(node11_read_user_securityadmin)
{
    UA_NodeId node_11 = UA_NODEID_NUMERIC(1, 1011);
    UA_Variant variant;

    UA_StatusCode retval = UA_Client_readValueAttribute(client, node_11, &variant);

    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadValue Node 11 FOR securityadmin");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: ReadValue Node 11 FOR securityadmin: %s", UA_StatusCode_name(retval));
    }

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(node11_write_user_securityadmin)
{
    UA_NodeId node_11 = UA_NODEID_NUMERIC(1, 1011);
    UA_Variant val;
     UA_UInt16  value = 50;

    UA_Variant_setScalar(&val, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_StatusCode retval = UA_Client_writeValueAttribute(client, node_11, &val);

    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: WriteValue of Node 11 FOR securityadmin");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: WriteValue of Node 11 FOR securityadmin: %s", UA_StatusCode_name(retval));
    }

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(node12_read_user_securityadmin)
{
    UA_NodeId node_12 = UA_NODEID_NUMERIC(1, 1012);
    UA_Variant variant;

    UA_StatusCode retval = UA_Client_readValueAttribute(client, node_12, &variant);

    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadValue Node 12 FOR securityadmin");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: ReadValue Node 12 FOR securityadmin: %s", UA_StatusCode_name(retval));
    }

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(node12_write_user_securityadmin)
{
    UA_NodeId node_12 = UA_NODEID_NUMERIC(1, 1012);
    UA_Variant val;
    UA_UInt16  value = 50;

    UA_Variant_setScalar(&val, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_StatusCode retval = UA_Client_writeValueAttribute(client, node_12, &val);

    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: WriteValue of Node 12 FOR securityadmin");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: WriteValue of Node 12 FOR securityadmin: %s", UA_StatusCode_name(retval));
    }

    UA_Client_disconnect(client);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(login_user_supervisor)
{
    UA_StatusCode retval = UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "user_supervisor", "123456");
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Login Success");
    }

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(node10_read_user_supervisor)
{
    UA_NodeId node_10 = UA_NODEID_NUMERIC(1, 1010);
    UA_Variant variant;

    UA_StatusCode retval = UA_Client_readValueAttribute(client, node_10, &variant);

    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadValue Node 10 FOR supervisor");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: ReadValue Node 10 FOR supervisor: %s", UA_StatusCode_name(retval));
    }

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(node10_write_user_supervisor)
{
    UA_NodeId node_10 = UA_NODEID_NUMERIC(1, 1010);
    UA_Variant val;
    UA_UInt16  value = 50;

    UA_Variant_setScalar(&val, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_StatusCode retval = UA_Client_writeValueAttribute(client, node_10, &val);

    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: WriteValue of Node 10 FOR supervisor");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: WriteValue of Node 10 FOR supervisor: %s", UA_StatusCode_name(retval));
    }

    UA_Client_disconnect(client);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(login_user_configureadmin)
{
    UA_StatusCode retval = UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "user_configureadmin", "123456");
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Login Success");
    }

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(node7_read_user_configureadmin)
{
    UA_NodeId node_7 = UA_NODEID_NUMERIC(1, 1007);
    UA_Variant variant;

    UA_StatusCode retval = UA_Client_readValueAttribute(client, node_7, &variant);

    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadValue Node 7 FOR configureadmin");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: ReadValue Node 7 FOR configureadmin: %s", UA_StatusCode_name(retval));
    }

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(node7_write_user_configureadmin)
{
    UA_NodeId node_7 = UA_NODEID_NUMERIC(1, 1007);
    UA_Variant val;
    UA_UInt16  value = 50;

    UA_Variant_setScalar(&val, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_StatusCode retval = UA_Client_writeValueAttribute(client, node_7, &val);

    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: WriteValue of Node 7 FOR configureadmin");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: WriteValue of Node 7 FOR configureadmin: %s", UA_StatusCode_name(retval));
    }

    UA_Client_disconnect(client);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(login_anon)
{
    UA_StatusCode retval = UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "user_anonymous", "123456");
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Login Success");
    }

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(node3_read_anon)
{
    UA_NodeId node_3 = UA_NODEID_NUMERIC(1, 1003);
    UA_Variant variant;

    UA_StatusCode retval = UA_Client_readValueAttribute(client, node_3, &variant);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadValue Node 3 FOR anonymous");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: ReadValue Node 3 FOR anonymous: %s", UA_StatusCode_name(retval));
    }

    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(node4_read_anon)
{
    UA_NodeId node_4 = UA_NODEID_NUMERIC(1, 1004);
    UA_Variant variant;

    UA_StatusCode retval = UA_Client_readValueAttribute(client, node_4, &variant);

    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadValue Node 4 FOR anonymous");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: ReadValue Node 4 FOR anonymous: %s", UA_StatusCode_name(retval));
    }

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(node4_write_anon)
{
    UA_NodeId node_4 = UA_NODEID_NUMERIC(1, 1004);
    UA_Variant val;
    UA_UInt16  value = 50;

    UA_Variant_setScalar(&val, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_StatusCode retval = UA_Client_writeValueAttribute(client, node_4, &val);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: WriteValue of Node 4 FOR anonymous");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: WriteValue of Node 4 FOR anonymous: %s", UA_StatusCode_name(retval));
    }

    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(node5_read_anon)
{
    UA_NodeId node_5 = UA_NODEID_NUMERIC(1, 1005);
    UA_Variant variant;

    UA_StatusCode retval = UA_Client_readValueAttribute(client, node_5, &variant);

    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: read of Node 5 FOR anonymous");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: read of Node 5 FOR anonymous: %s", UA_StatusCode_name(retval));
    }

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(node5_write_anon)
{
    UA_NodeId node_5 = UA_NODEID_NUMERIC(1, 1005);
    UA_Variant val;
    UA_UInt16  value = 50;

    UA_Variant_setScalar(&val, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_StatusCode retval = UA_Client_writeValueAttribute(client, node_5, &val);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: WriteValue of Node 5 FOR ANONYMOUS");
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Since, user access level is restricted, Role permission will not override this parameter");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: WriteValue of Node 5 FOR anonymous: %s", UA_StatusCode_name(retval));
    }

    UA_Client_disconnect(client);
    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);

}END_TEST


START_TEST(login_user_auth)
{
    UA_StatusCode retval = UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "user_auth", "123456");
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Login Success");
    }

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(node1_read_user_auth)
{
    UA_NodeId node_1 = UA_NODEID_NUMERIC(1, 1001);
    UA_Variant variant;

    UA_StatusCode retval = UA_Client_readValueAttribute(client, node_1, &variant);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadValue Node 1 FOR user_auth");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: ReadValue Node 1 FOR user_auth %s", UA_StatusCode_name(retval));
    }

    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);

}END_TEST


START_TEST(node6_read_user_auth)
{
    UA_NodeId node_6 = UA_NODEID_NUMERIC(1, 1006);
    UA_Variant variant;

    UA_StatusCode retval = UA_Client_readValueAttribute(client, node_6, &variant);

    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadValue Node 6 FOR user_auth");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: ReadValue Node 6 FOR user_auth: %s", UA_StatusCode_name(retval));
    }

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(node6_write_user_auth)
{
    UA_NodeId node_6 = UA_NODEID_NUMERIC(1, 1006);
    UA_Variant val;
    UA_UInt16  value = 50;

    UA_Variant_setScalar(&val, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_StatusCode retval = UA_Client_writeValueAttribute(client, node_6, &val);

    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: WriteValue of Node 6 FOR user_auth");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: WriteValue of Node 6 FOR user_auth: %s", UA_StatusCode_name(retval));
    }

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(read_role_permission_user_auth)
{
    UA_NodeId node_6 = UA_NODEID_NUMERIC(1, 1006);

    UA_UInt32 roleValue[3];
    // Write and Read Role permission

    UA_RolePermissionType *readRolePermission = (UA_RolePermissionType *)UA_Array_new(3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);;
    size_t  readRolePermissionsize = 2;

    UA_StatusCode retval = UA_Client_readRolePermissionAttribute(client, node_6, &readRolePermissionsize , &readRolePermission);
    for (size_t index = 0; index < 2; index++)
        roleValue[index] = readRolePermission[index].permissions;

    if ((roleValue[0] == 0x1ffff) && (roleValue[1] == 0x1ffff)) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Write and Read RolePermission of Node 6");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: Write and Read RolePermission of Node 6");
    }
    UA_Array_delete(readRolePermission, 2, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

START_TEST(read_user_role_permission_user_auth)
{
    UA_NodeId node_6 = UA_NODEID_NUMERIC(1, 1006);


    UA_RolePermissionType * readUserRolePermission = (UA_RolePermissionType *)UA_Array_new(3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    size_t readUserRolePermissionsize = 2;
    UA_StatusCode retval = UA_Client_readUserRolePermissionAttribute(client, node_6, &readUserRolePermissionsize , &readUserRolePermission);

    if (retval == UA_STATUSCODE_GOOD ) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Read UserRolePermission of Node 6");
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: Read UserRolePermission of Node 6");
    }
    UA_Array_delete(readUserRolePermission, 2, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    UA_Client_disconnect(client);

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}END_TEST

static Suite * testSuite_check_user_data_from_platform(void) {
    Suite *s = suite_create("check User data from Platform");
    TCase *tc_check_user = tcase_create("check data");

    //------login engineer---------//
    tcase_add_test(tc_check_user, login_user_engineer);
    tcase_add_test(tc_check_user, node1_no_read_access);
    tcase_add_test(tc_check_user, node11_read_access);
    tcase_add_test(tc_check_user, node1_browse_access);
    tcase_add_test(tc_check_user, node12_read_role_permission);
    tcase_add_test(tc_check_user, node12_user_read_role_permission);
    tcase_add_test(tc_check_user, node12_read_write_role_permission);
    tcase_add_test(tc_check_user, node12_user_read_write_role_permission);
    tcase_add_test(tc_check_user, node11_write_role_permission);
    tcase_add_test(tc_check_user, node1_write_role_permission);
    tcase_add_test(tc_check_user, add_new_node_Engineer);
    tcase_add_test(tc_check_user, add_new_reference_Engineer);
    tcase_add_test(tc_check_user, delete_new_reference_Engineer);
    tcase_add_test(tc_check_user, delete_node_as_Engineer);
    tcase_add_test(tc_check_user, method_call_to_add_kalycito_worker_role);
    tcase_add_test(tc_check_user, method_call_to_add_kalycito_worker_identity);

    tcase_add_test(tc_check_user,login_as_kalycito_worker);
    tcase_add_test(tc_check_user,node13_read_kalycito_worker);

    tcase_add_test(tc_check_user,login_as_user_operator);
    tcase_add_test(tc_check_user,node1_read_user_operator);
    tcase_add_test(tc_check_user,node9_read_user_operator);
    tcase_add_test(tc_check_user,node9_write_user_operator);
    tcase_add_test(tc_check_user,read_role_permission_user_operator);
    tcase_add_test(tc_check_user,read_user_role_permission_user_operator);

    tcase_add_test(tc_check_user,login_user_observer);
    tcase_add_test(tc_check_user,node8_read_user_observer);
    tcase_add_test(tc_check_user,node8_write_user_observer);

    tcase_add_test(tc_check_user,login_user_securityadmin);
    tcase_add_test(tc_check_user,node11_read_user_securityadmin);
    tcase_add_test(tc_check_user,node11_write_user_securityadmin);
    tcase_add_test(tc_check_user,node12_read_user_securityadmin);
    tcase_add_test(tc_check_user,node12_write_user_securityadmin);

    tcase_add_test(tc_check_user,login_user_supervisor);
    tcase_add_test(tc_check_user,node10_read_user_supervisor);
    tcase_add_test(tc_check_user,node10_write_user_supervisor);

    tcase_add_test(tc_check_user,login_user_configureadmin);
    tcase_add_test(tc_check_user,node7_read_user_configureadmin);
    tcase_add_test(tc_check_user,node7_write_user_configureadmin);

    tcase_add_test(tc_check_user,login_anon);
    tcase_add_test(tc_check_user,node3_read_anon);
    tcase_add_test(tc_check_user,node4_read_anon);
    tcase_add_test(tc_check_user,node4_write_anon);
    tcase_add_test(tc_check_user,node5_read_anon);
    tcase_add_test(tc_check_user,node5_write_anon);

    tcase_add_test(tc_check_user,login_user_auth);
    tcase_add_test(tc_check_user,node1_read_user_auth);
    tcase_add_test(tc_check_user,node6_read_user_auth);
    tcase_add_test(tc_check_user,node6_write_user_auth);
    tcase_add_test(tc_check_user,read_role_permission_user_auth);
    tcase_add_test(tc_check_user,read_user_role_permission_user_auth);

    suite_add_tcase(s, tc_check_user);
    return s;
}

int main(void) {
    running = true;
    UA_Server * server = UA_Server_new();
    config = UA_Server_getConfig(server);

    client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    config->nodeLifecycle.createOptionalChild = addRoleBasedNodes;
    addNewNamespaceandSetDefaultPermission(server);

    addNewTestNode(server);

    addNewTestNodeObject(server);

    config->accessControl.checkUserDatabase = checkUserDatabase;

    config->accessControl.readUserDefinedRolePermission = readUserProvidedRoles;

    config->accessControl.clear(&config->accessControl);
    UA_AccessControl_custom(config, true, NULL,&config->securityPolicies[config->securityPoliciesSize-1].policyUri, 0, NULL);

    pthread_create (&server_thread,NULL,serverRun,server);
    sleep(1);
    Suite *s = testSuite_check_user_data_from_platform();

    SRunner *sr = srunner_create(s);

    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    running= false;
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}