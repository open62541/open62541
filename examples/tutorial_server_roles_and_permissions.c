/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 * 
 * Copyright 2023 (c) Asish Ganesh, Eclatron Technologies Private Limited
 */

/**
 * Set user role permissions for nodes
 * -----------------------------------
 * In this example, we will verify the presence of a login user in the user database and
 * confirm their access rights before disclosing the attributes and references of the nodes.
 */

#include <open62541/plugin/accesscontrol_custom.h>
#include <open62541/client_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

// structure for user Data Queue
typedef struct user_data_buffer {
    bool userAvailable;
    UA_String username;
    UA_String password;
    UA_String role;
    uint64_t GroupID;
    uint64_t userID;
} userDatabase;

UA_NodeId outNewNodeId;

int           checkUser (userDatabase* userData);
UA_Boolean    checkUserDatabase(const UA_UserNameIdentityToken *userToken, UA_String *roleName);
UA_StatusCode addNewTestNode(UA_Server *server);

int checkUser (userDatabase* userData){
    char user_database[][256]= {"user1,password1,Engineer,1,1",
                                "user2,password2,ConfigureAdmin,2,2",
                                "user3,password3,Observer,3,3",
                                "user4,password4,Operator,4,4"};

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

/* Add an object node and three variable nodes with different
 * user role permissions to the server address space 
 */
UA_StatusCode addNewTestNode(UA_Server *server) {
    // Object one
    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.description = UA_LOCALIZEDTEXT("en-US", "Access_Rights");
    object_attr.displayName = UA_LOCALIZEDTEXT("en-US", "Access_Rights");
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
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissionsSize = 2;
    attributes.userRolePermissions = (UA_RolePermissionType *)UA_Array_new(attributes.rolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[0].permissions = 0x1FFFF;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    attributes.userRolePermissions[1].permissions = 0x2F;
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

/* Check if the user is present in the database
 */
UA_Boolean checkUserDatabase(const UA_UserNameIdentityToken *userToken, UA_String *roleName) {
    userDatabase userData;
    userData.userAvailable = false;

    UA_String_copy(&userToken->userName, &userData.username);
    UA_String_copy(&userToken->password, &userData.password);

    checkUser(&userData);

    if (userData.userAvailable != true)
       return false;

    UA_String_copy(&userData.role, roleName);

    if (userData.username.data != NULL)
        free(userData.username.data);

    if (userData.password.data != NULL)
        free(userData.password.data );

    return true;
}

/* It follows the main server code, making use of the above definitions. */
static volatile UA_Boolean running = true;

static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

int main(int argc, char **argv) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    // Create a server object
    UA_Server *server = UA_Server_new();

    // Create a configuration object for the server
    UA_ServerConfig *config = UA_Server_getConfig(server);

    // Add 1 - Object, & 3 - Variable nodes
    addNewTestNode(server);

    /* Add method override for checkUserDatabase
     * This is to check user role corresponding to the node
     */
    config->accessControl.checkUserDatabase = checkUserDatabase;

    // Disable anonymous logins, enable two user/password logins
    config->accessControl.clear(&config->accessControl);
    UA_StatusCode retval = UA_AccessControl_custom(config, true, NULL,
                           &config->securityPolicies[config->securityPoliciesSize-1].policyUri, 0, NULL);
    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "AccessControlDefaultFailed");

    retval = UA_Server_run(server, &running);

    UA_Server_deleteNode(server, outNewNodeId, true);
    UA_NodeId_clear(&outNewNodeId);
    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
