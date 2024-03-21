/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 * 
 * Copyright 2023 (c) Asish Ganesh, Eclatron Technologies Private Limited
 */

/**
 * Set user role permissions for nodes
 * -----------------------------------
 * In this example, we will verify the login user and confirm their
 * access rights before disclosing the details of the nodes.
 */

#include <open62541/server.h>
#include <open62541/plugin/log_stdout.h>
#include <server/ua_services.h>

UA_StatusCode addNewTestNode(UA_Server *server);

/* Add an object node and three variable nodes with different
 * user role permissions to the server address space 
 */
UA_StatusCode addNewTestNode(UA_Server *server) {
    // Object one
    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.description = UA_LOCALIZEDTEXT("en-US", "Access_Rights");
    object_attr.displayName = UA_LOCALIZEDTEXT("en-US", "Access_Rights");
    object_attr.rolePermissionsSize = 2;
    object_attr.rolePermissions = (UA_RolePermissionType *)UA_Array_new(object_attr.rolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    object_attr.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    object_attr.rolePermissions[0].permissions = 0x1FFFF;
    object_attr.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR);
    object_attr.rolePermissions[1].permissions = 0x1FFFF;
    object_attr.userRolePermissionsSize = 2;
    object_attr.userRolePermissions = (UA_RolePermissionType *)UA_Array_new(object_attr.userRolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    object_attr.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    object_attr.userRolePermissions[0].permissions = 0x1FFFF;
    object_attr.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR);
    object_attr.userRolePermissions[1].permissions = 0x1FFFF;
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 100000),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_QUALIFIEDNAME(1, "Access_Rights"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), object_attr, NULL, NULL);

    // Variable one
    UA_VariableAttributes attributes = UA_VariableAttributes_default;
    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "Node_1");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissionsSize = 3;
    attributes.rolePermissions = (UA_RolePermissionType *)UA_Array_new(attributes.rolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.rolePermissions[0].permissions = 0x1; //1ffff
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.rolePermissions[2].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    attributes.rolePermissions[2].permissions = 0x1FFFF;
    attributes.userRolePermissionsSize = 3;
    attributes.userRolePermissions = (UA_RolePermissionType *)UA_Array_new(attributes.rolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.userRolePermissions[0].permissions = 0x1;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissions[2].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    attributes.userRolePermissions[2].permissions = 0x21;
    UA_UInt16 value = 1;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1000), UA_NODEID_NUMERIC(1, 100000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Node_1"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    // Variable two
    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "Node_2");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissionsSize = 3;
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN);
    attributes.rolePermissions[0].permissions = 0x1FFFF;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.rolePermissions[2].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR);
    attributes.rolePermissions[2].permissions = 0x1FFFF;
    attributes.userRolePermissionsSize = 3;
    attributes.userRolePermissions = (UA_RolePermissionType *)UA_Array_new(attributes.rolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN);
    attributes.userRolePermissions[0].permissions = 0x1F867;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissions[2].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR);
    attributes.userRolePermissions[2].permissions = 0x21;
    value = 2;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1002), UA_NODEID_NUMERIC(1, 100000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Node_2"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    // Variable three
    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "Node_3");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissionsSize = 2;
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[0].permissions = 0x1FFFF;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    attributes.rolePermissions[1].permissions = UA_PERMISSIONTYPE_BROWSE;
    attributes.userRolePermissionsSize = 2;
    attributes.userRolePermissions = (UA_RolePermissionType *)UA_Array_new(attributes.rolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[0].permissions = UA_PERMISSIONTYPE_READ + UA_PERMISSIONTYPE_BROWSE;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    attributes.userRolePermissions[1].permissions = UA_PERMISSIONTYPE_BROWSE;
    value = 3;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1003), UA_NODEID_NUMERIC(1, 100000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Node_3"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    UA_free(attributes.rolePermissions);
    UA_free(attributes.userRolePermissions);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
helloWorldMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output) {
    UA_String *inputStr = (UA_String*)input->data;
    UA_String tmp = UA_STRING_ALLOC("Hello ");
    if(inputStr->length > 0) {
        tmp.data = (UA_Byte *)UA_realloc(tmp.data, tmp.length + inputStr->length);
        memcpy(&tmp.data[tmp.length], inputStr->data, inputStr->length);
        tmp.length += inputStr->length;
    }
    UA_Variant_setScalarCopy(output, &tmp, &UA_TYPES[UA_TYPES_STRING]);
    UA_String_clear(&tmp);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Hello World was called");
    return UA_STATUSCODE_GOOD;
}

static void
addHelloWorldMethod(UA_Server *server) {
    UA_Argument inputArgument;
    UA_Argument_init(&inputArgument);
    inputArgument.description = UA_LOCALIZEDTEXT("en-US", "A String");
    inputArgument.name = UA_STRING("MyInput");
    inputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArgument.valueRank = UA_VALUERANK_SCALAR;

    UA_Argument outputArgument;
    UA_Argument_init(&outputArgument);
    outputArgument.description = UA_LOCALIZEDTEXT("en-US", "A String");
    outputArgument.name = UA_STRING("MyOutput");
    outputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    outputArgument.valueRank = UA_VALUERANK_SCALAR;

    UA_MethodAttributes helloAttr = UA_MethodAttributes_default;
    helloAttr.description = UA_LOCALIZEDTEXT("en-US","Say `Hello World`");
    helloAttr.displayName = UA_LOCALIZEDTEXT("en-US","Hello World");
    helloAttr.executable = true;
    helloAttr.userExecutable = true;
    helloAttr.rolePermissionsSize = 2;
    helloAttr.rolePermissions = (UA_RolePermissionType *)UA_Array_new(helloAttr.rolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    helloAttr.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    helloAttr.rolePermissions[0].permissions = 0x1FFFF;
    helloAttr.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    helloAttr.rolePermissions[1].permissions = UA_PERMISSIONTYPE_BROWSE;
    helloAttr.userRolePermissionsSize = 1;
    helloAttr.userRolePermissions = (UA_RolePermissionType *)UA_Array_new(helloAttr.userRolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    helloAttr.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    helloAttr.userRolePermissions[0].permissions = 0x1FFFF;
    helloAttr.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    helloAttr.userRolePermissions[0].permissions = UA_PERMISSIONTYPE_BROWSE;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1,62541),
                            UA_NODEID_NUMERIC(1, 100000),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "hello world"),
                            helloAttr, &helloWorldMethodCallback,
                            1, &inputArgument, 1, &outputArgument, NULL, NULL);
}

int main(void) {
    const UA_String user1 = {5, (UA_Byte*)"user1"};
    const UA_String user2 = {5, (UA_Byte*)"user2"};
    const UA_String user3 = {5, (UA_Byte*)"user3"};
    
    const RoleGroup roleGroupInfo1 = (RoleGroup)(configureAdminRole | engineerRole);
    const RoleGroup roleGroupInfo2 = (RoleGroup)(authenticatedUserRole | operatorRole);
    const RoleGroup roleGroupInfo3 = (RoleGroup)(authenticatedUserRole | observerRole);

    UsernameRolePair userNameRolePairInfo1 = {user1, roleGroupInfo1};
    UsernameRolePair userNameRolePairInfo2 = {user2, roleGroupInfo2};
    UsernameRolePair userNameRolePairInfo3 = {user3, roleGroupInfo3};

    UsernameRolePair userNameRolePairInfo[3];
    userNameRolePairInfo[0] = userNameRolePairInfo1;
    userNameRolePairInfo[1] = userNameRolePairInfo2;
    userNameRolePairInfo[2] = userNameRolePairInfo3;
    setUserRoleDetailInfo(userNameRolePairInfo, 3);
    // setUserRoleDetails(userRoleDetail);

    UA_Server *server = UA_Server_new();

    // Add 1 - Object, & 3 - Variable nodes
    addNewTestNode(server);

    addHelloWorldMethod(server);

    UA_StatusCode retval = UA_Server_runUntilInterrupt(server);
    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
