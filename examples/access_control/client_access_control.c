/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>

#include <stdlib.h>

#define KALYCITO_WORKER_NODE_ID UA_NODEID_STRING(2, "KalycitoWorker")
#define WRITE_VALUE             100

UA_StatusCode retval;
UA_Client *client;
UA_Variant variant;
static UA_UInt32 testCount = 0;
static UA_UInt16 testCaseSuccessCount = 0;
static UA_UInt16 testCaseFailureCount = 0;

int main(int argc, char **argv) {
    client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    UA_NodeId node_1 = UA_NODEID_NUMERIC(1, 1001);
    UA_NodeId node_5 = UA_NODEID_NUMERIC(1, 1005);
    UA_NodeId node_6 = UA_NODEID_NUMERIC(1, 1006);
    UA_NodeId node_7 = UA_NODEID_NUMERIC(1, 1007);
    UA_NodeId node_8 = UA_NODEID_NUMERIC(1, 1008);
    UA_NodeId node_9 = UA_NODEID_NUMERIC(1, 1009);
    UA_NodeId node_10 = UA_NODEID_NUMERIC(1, 1010);
    UA_NodeId node_11 = UA_NODEID_NUMERIC(1, 1011);
    UA_NodeId node_12 = UA_NODEID_NUMERIC(1, 1012);
    UA_NodeId node_13 = UA_NODEID_NUMERIC(1, 1013);  // Only accessed by KalycitoWorker Role

    if (argc < 3) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Error: Please provide Endpoint eg., opc.tcp://172.17.4.10:4840, \
                      Username and password as command line arguments");
        return EXIT_FAILURE;
    }

    /* Check Client connection as Engineer */
    retval = UA_Client_connectUsername(client, argv[1], argv[2], argv[3]);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    if(retval != UA_STATUSCODE_GOOD) {
        testCaseFailureCount++;
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Provided wrong Credentials");
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }
    else {
        testCaseSuccessCount++;
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Login as Engineer");
    }

    /* "User does not have permission to read the value of the Node ID 1." */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    retval = UA_Client_readValueAttribute(client, node_1, &variant);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadValue of restricted Node 2 FOR ENGINEER");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: ReadValue of restricted Node 2 FOR ENGINEER: %s", UA_StatusCode_name(retval));
        testCaseFailureCount++;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    retval = UA_Client_readValueAttribute(client, node_11, &variant);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadValue Node 5 FOR ENGINEER");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: ReadValue Node 5 FOR ENGINEER: %s", UA_StatusCode_name(retval));
        testCaseFailureCount++;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
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

    if (browseResponse.responseHeader.serviceResult == 0) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: BrowseNode 5 FOR ENGINEER");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: BrowseNode 5 FOR ENGINEER: %s", UA_StatusCode_name(retval));
        testCaseFailureCount++;
    }
    UA_BrowseResponse_clear(&browseResponse);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    UA_RolePermissionType *readRolePermission = (UA_RolePermissionType *)UA_Array_new(3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    size_t readRolePermissionsize = 3;
    UA_UInt32 roleValue[3];
    retval = UA_Client_readRolePermissionAttribute(client, node_12, &readRolePermissionsize, &readRolePermission);
    for (size_t index = 0; index < 3; index++) {
        roleValue[index] = readRolePermission[index].permissions;
    }

    if ((roleValue[0] == 0x3) && (roleValue[1] == 0x1ffff) && (roleValue[2] == 0x1ffff)) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadRolePermission of Node 12");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: ReadRolePermission of Node 12");
        testCaseFailureCount++;
    }
    UA_Array_delete(readRolePermission, 3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    UA_RolePermissionType *readUserRolePermission = (UA_RolePermissionType *)UA_Array_new(3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);;
    size_t readUserRolePermissionsize = 3;
    retval = UA_Client_readUserRolePermissionAttribute(client, node_12, &readUserRolePermissionsize , &readUserRolePermission);
    for (size_t index = 0; index < 3; index++) {
        roleValue[index] = readUserRolePermission[index].permissions;
    }

    if ((roleValue[0] == 0x3) && (roleValue[1] == 0x3) && (roleValue[2] == 0x3)) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadUserRolePermission of Node 12");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: ReadUserRolePermission of Node 12");
        testCaseFailureCount++;
    }
    UA_Array_delete(readUserRolePermission, 3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);

    // Write and Read Role permission
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    readRolePermission = (UA_RolePermissionType *)UA_Array_new(3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    readRolePermissionsize = 3;
    UA_RolePermissionType *rolePermissions = (UA_RolePermissionType *)UA_Array_new(3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    rolePermissions[0].permissions = 0x21;
    rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    rolePermissions[1].permissions = 0x1FFFF;
    rolePermissions[2].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN);
    rolePermissions[2].permissions = 0x21;
    retval = UA_Client_writeRolePermissionAttribute(client, node_12, 3, rolePermissions);
    if (retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Write RolePermission of Node 12");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: Write RolePermission of Node 12");
        testCaseFailureCount++;
    }
    UA_Array_delete(rolePermissions, 3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    readRolePermission = (UA_RolePermissionType *)UA_Array_new(3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);;
    readRolePermissionsize = 3;
    retval = UA_Client_readRolePermissionAttribute(client, node_12, &readRolePermissionsize , &readRolePermission);
    for (size_t index = 0; index < 3; index++)
        roleValue[index] = readRolePermission[index].permissions;

    if ((roleValue[0] == 0x21) && (roleValue[1] == 0x1ffff) && (roleValue[2] == 0x21)) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Write and Read RolePermission of Node 12");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: Write and Read RolePermission of Node 12");
        testCaseFailureCount++;
    }
    UA_Array_delete(readRolePermission, 3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);

    // Write and Read UserRole permission
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    UA_RolePermissionType *userRolePermissions = (UA_RolePermissionType *)UA_Array_new(3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    userRolePermissions[0].permissions = 0x1837;
    userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    userRolePermissions[1].permissions = 0x1FFFF;
    userRolePermissions[2].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN);
    userRolePermissions[2].permissions = 0x1837;
    retval = UA_Client_writeUserRolePermissionAttribute(client, node_12, 3, userRolePermissions);
    if (retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Write UserRolePermission of Node 12");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: Write UserRolePermission of Node 12");
        testCaseFailureCount++;
    }
    UA_Array_delete(userRolePermissions, 3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    UA_UInt32 userRoleValue[3];
    readUserRolePermission = (UA_RolePermissionType *)UA_Array_new(3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);;
    readUserRolePermissionsize = 3;
    retval = UA_Client_readUserRolePermissionAttribute(client, node_12, &readUserRolePermissionsize , &readUserRolePermission);
    for (size_t index = 0; index < 3; index++)
        userRoleValue[index] = readUserRolePermission[index].permissions;

    if ((userRoleValue[0] == 0x1837) && (userRoleValue[1] == 0x1ffff) && (userRoleValue[2] == 0x1837)) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Write and Read UserRolePermission of Node 12");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: Write and Read UserRolePermission of Node 12");
        testCaseFailureCount++;
    }
    UA_Array_delete(readUserRolePermission, 3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    // Write Value
    UA_Variant val;
    UA_UInt16 value = 100;
    UA_Variant_setScalar(&val, &value, &UA_TYPES[UA_TYPES_UINT16]);
    retval = UA_Client_writeValueAttribute(client, node_11, &val);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: WriteValue of Node 11 FOR ENGINEER");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: WriteValue of Node 11 FOR ENGINEER: %s", UA_StatusCode_name(retval));
        testCaseFailureCount++;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    retval = UA_Client_writeValueAttribute(client, node_1, &val);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: WriteValue of restricted Node 1 FOR ENGINEER");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: WriteValue of restricted Node 1 FOR ENGINEER: %s", UA_StatusCode_name(retval));
        testCaseFailureCount++;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    /* Add new variable */
    UA_VariableAttributes newVariableAttributes = UA_VariableAttributes_default;
    newVariableAttributes.accessLevel = UA_ACCESSLEVELMASK_READ;
    newVariableAttributes.description = UA_LOCALIZEDTEXT_ALLOC("en-US", "NewVariable desc");
    newVariableAttributes.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US", "NewVariable");
    newVariableAttributes.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    value = 50;
    UA_Variant_setScalarCopy(&newVariableAttributes.value, &value, &UA_TYPES[UA_TYPES_UINT32]);

    retval = UA_Client_addVariableNode(client, UA_NODEID_NUMERIC(1, 4001),
                                       UA_NODEID_NUMERIC(1, 200000),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                       UA_QUALIFIEDNAME(1, "newVariable_1"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                       newVariableAttributes, NULL);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Add New Node 1 FOR ENGINEER");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: Add New Node 1 FOR ENGINEER: %s", UA_StatusCode_name(retval));
        testCaseFailureCount++;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    /* Add new Reference */
    UA_ExpandedNodeId extNodeId_1 = UA_EXPANDEDNODEID_NUMERIC(0, 0);
    extNodeId_1.nodeId = UA_NODEID_NUMERIC(1, 4001);
    retval = UA_Client_addReference(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_TRUE,
                                    UA_STRING_NULL, extNodeId_1, UA_NODECLASS_VARIABLE);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Add Reference Node 1 FOR ENGINEER");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: Add Reference Node 1 FOR ENGINEER: %s", UA_StatusCode_name(retval));
        testCaseFailureCount++;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    /* Delete reference */
    retval = UA_Client_deleteReference(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_TRUE, extNodeId_1, UA_TRUE);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Node 1 FOR ENGINEER");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: Node 1 FOR ENGINEER: %s", UA_StatusCode_name(retval));
        testCaseFailureCount++;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    /* Delete nodes */
    retval = UA_Client_deleteNode(client, UA_NODEID_NUMERIC(1, 4001), UA_TRUE);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Node 1 FOR ENGINEER");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: Node 1 FOR ENGINEER: %s", UA_StatusCode_name(retval));
        testCaseFailureCount++;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
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
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE to Add KalycitoWorker role to the server");
        testCaseFailureCount++;
    }

    UA_free(inputArguments);
    UA_CallResponse_clear(&response);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
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
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE to Add KalycitoWorker identity to the server");
        testCaseFailureCount++;
    }

    UA_free(inputArguments_2);
    UA_CallResponse_clear(&response_2);
    /* Clean up */
    UA_VariableAttributes_clear(&newVariableAttributes);
    UA_Client_disconnect(client);    // Disconnect as ENGINEER

    /* Check Client connection as KalycitoWorker */
    retval = UA_Client_connectUsername(client, argv[1], "user_kalycitoworker", "123456");

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    if(retval != UA_STATUSCODE_GOOD) {
        testCaseFailureCount++;
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Provided wrong Credentials: %s", UA_StatusCode_name(retval));
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }
    else {
        testCaseSuccessCount++;
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Login as KalycitoWorker");
    }

    // Read value fails for node 1
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    retval = UA_Client_readValueAttribute(client, node_1, &variant);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadValue Node 1 FOR KALYCITOWORKER");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: ReadValue Node 1 FOR KALYCITOWORKER: %s", UA_StatusCode_name(retval));
        testCaseFailureCount++;
    }

    /* "User does not have permission to read the value of the Node ID 13" */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    retval = UA_Client_readValueAttribute(client, node_13, &variant);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadValue of restricted Node 13 FOR KALYCITOWORKER");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: ReadValue of restricted Node 13 FOR KALYCITOWORKER: %s", UA_StatusCode_name(retval));
        testCaseFailureCount++;
    }
    /* Add TC HERE*/
    UA_Client_disconnect(client);    // Disconnect as KalycitoWorker

    // TEST CASES FOR OPERATOR //
    /* Check Client connection as Operator */
    retval = UA_Client_connectUsername(client, argv[1], "user_operator", "123456");

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    if(retval != UA_STATUSCODE_GOOD) {
        testCaseFailureCount++;
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Provided wrong Credentials: %s", UA_StatusCode_name(retval));
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }
    else {
        testCaseSuccessCount++;
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Login as Operator");
    }

    /* "User does not have permission to read the value of the Node ID 1." */
    retval = UA_Client_readValueAttribute(client, node_1, &variant);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadValue of restricted Node 1 FOR OPERATOR");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: ReadValue of restricted Node 1 FOR OPERATOR: %s", UA_StatusCode_name(retval));
        testCaseFailureCount++;
    }

//Read value
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    retval = UA_Client_readValueAttribute(client, node_9, &variant);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadValue Node 9 FOR OPERATOR");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: ReadValue Node 9 FOR OPERATOR: %s", UA_StatusCode_name(retval));
        testCaseFailureCount++;
    }

// Write value
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    UA_Variant_setScalar(&val, &value, &UA_TYPES[UA_TYPES_UINT16]);
    retval = UA_Client_writeValueAttribute(client, node_9, &val);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: WriteValue of Node 9 FOR OPERATOR");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: WriteValue of Node 9 FOR OPERATOR: %s", UA_StatusCode_name(retval));
        testCaseFailureCount++;
    }

//Read Role permission
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    readRolePermission = (UA_RolePermissionType *)UA_Array_new(3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);;
    readRolePermissionsize = 2;
    retval = UA_Client_readRolePermissionAttribute(client, node_9, &readRolePermissionsize , &readRolePermission);
    for (size_t index = 0; index < 2; index++)
        roleValue[index] = readRolePermission[index].permissions;

    if ((roleValue[0] == 0x1ffff) && (roleValue[1] == 0x1ffff)) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Write and Read RolePermission of Node 9");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: Write and Read RolePermission of Node 9");
        testCaseFailureCount++;
    }
    UA_Array_delete(readRolePermission, 2, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);

//Read User Role permission
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    readUserRolePermission = (UA_RolePermissionType *)UA_Array_new(3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);;
    readUserRolePermissionsize = 2;
    retval = UA_Client_readUserRolePermissionAttribute(client, node_9, &readUserRolePermissionsize , &readUserRolePermission);
    for (size_t index = 0; index < 2; index++)
        roleValue[index] = readUserRolePermission[index].permissions;

    if ((roleValue[0] == 0x7867) && (roleValue[1] == 0x1ffff)) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Read UserRolePermission of Node 9");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: Read UserRolePermission of Node 9");
        testCaseFailureCount++;
    }
    UA_Array_delete(readUserRolePermission, 2, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    // TEST CASES FOR OPERATOR //

    /* Add TC HERE*/
    UA_Client_disconnect(client);    // Disconnect as Operator

    // TEST CASES FOR Observer //
    /* Check Client connection as Observer */
    retval = UA_Client_connectUsername(client, argv[1], "user_observer", "123456");

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    if(retval != UA_STATUSCODE_GOOD) {
        testCaseFailureCount++;
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Provided wrong Credentials: %s", UA_StatusCode_name(retval));
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }
    else {
        testCaseSuccessCount++;
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Login as Observer");
    }

//Read value
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    retval = UA_Client_readValueAttribute(client, node_8, &variant);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadValue Node 6 FOR OBSERVER");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: ReadValue Node 6 FOR OBSERVER: %s", UA_StatusCode_name(retval));
        testCaseFailureCount++;
    }

// Write value
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    UA_Variant_setScalar(&val, &value, &UA_TYPES[UA_TYPES_UINT16]);
    retval = UA_Client_writeValueAttribute(client, node_8, &val);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: WriteValue of Node 6 FOR OBSERVER");
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Since, user access level is restricted, Role permission will not override this parameter");
        // If required, modify the accessPermissions and methodAccessPermission in the setUserRole_settings function in the
        //   src/server/ua_server_role_access.c file
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: WriteValue of Node 6 FOR OBSERVER: %s", UA_StatusCode_name(retval));
        testCaseFailureCount++;
    }

    UA_Client_disconnect(client);    // Disconnect as Observer

    // TEST CASES FOR Security_Admin //
    /* Check Client connection as Security_Admin */
    retval = UA_Client_connectUsername(client, argv[1], "user_securityadmin", "123456");

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    if(retval != UA_STATUSCODE_GOOD) {
        testCaseFailureCount++;
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Provided wrong Credentials: %s", UA_StatusCode_name(retval));
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }
    else {
        testCaseSuccessCount++;
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Login as Security_Admin");
    }

//Read value
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    retval = UA_Client_readValueAttribute(client, node_11, &variant);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadValue Node 11 FOR SECURITY_ADMIN");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: ReadValue Node 11 FOR SECURITY_ADMIN: %s", UA_StatusCode_name(retval));
        testCaseFailureCount++;
    }

// Write value
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    UA_Variant_setScalar(&val, &value, &UA_TYPES[UA_TYPES_UINT16]);
    retval = UA_Client_writeValueAttribute(client, node_11, &val);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: WriteValue of Node 11 FOR SECURITY_ADMIN");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: WriteValue of Node 11 FOR SECURITY_ADMIN: %s", UA_StatusCode_name(retval));
        testCaseFailureCount++;
    }

//Read value
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    retval = UA_Client_readValueAttribute(client, node_12, &variant);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadValue Node 12 FOR SECURITY_ADMIN");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: ReadValue Node 12 FOR SECURITY_ADMIN: %s", UA_StatusCode_name(retval));
        testCaseFailureCount++;
    }

// Write value
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    UA_Variant_setScalar(&val, &value, &UA_TYPES[UA_TYPES_UINT16]);
    retval = UA_Client_writeValueAttribute(client, node_12, &val);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: WriteValue of Node 12 FOR SECURITY_ADMIN");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: WriteValue of Node 12 FOR SECURITY_ADMIN: %s", UA_StatusCode_name(retval));
        testCaseFailureCount++;
    }

    UA_Client_disconnect(client);    // Disconnect as Security_Admin

    // TEST CASES FOR Supervisor //
    /* Check Client connection as Supervisor */
    retval = UA_Client_connectUsername(client, argv[1], "user_supervisor", "123456");

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    if(retval != UA_STATUSCODE_GOOD) {
        testCaseFailureCount++;
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Provided wrong Credentials: %s", UA_StatusCode_name(retval));
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }
    else {
        testCaseSuccessCount++;
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Login as Supervisor");
    }

//Read value
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    retval = UA_Client_readValueAttribute(client, node_10, &variant);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadValue Node 10 FOR SUPERVISOR");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: ReadValue Node 10 FOR SUPERVISOR: %s", UA_StatusCode_name(retval));
        testCaseFailureCount++;
    }

// Write value
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    UA_Variant_setScalar(&val, &value, &UA_TYPES[UA_TYPES_UINT16]);
    retval = UA_Client_writeValueAttribute(client, node_10, &val);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: WriteValue of Node 10 FOR SUPERVISOR");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: WriteValue of Node 10 FOR SUPERVISOR: %s", UA_StatusCode_name(retval));
        testCaseFailureCount++;
    }

    UA_Client_disconnect(client);    // Disconnect as Supervisor

    // TEST CASES FOR Configure_Admin //
    /* Check Client connection as Configure_Admin */
    retval = UA_Client_connectUsername(client, argv[1], "user_configureadmin", "123456");

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    if(retval != UA_STATUSCODE_GOOD) {
        testCaseFailureCount++;
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Provided wrong Credentials: %s", UA_StatusCode_name(retval));
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }
    else {
        testCaseSuccessCount++;
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Login as Configure_Admin");
    }

//Read value
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    retval = UA_Client_readValueAttribute(client, node_7, &variant);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadValue Node 7 FOR CONFIGURE_ADMIN");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: ReadValue Node 7 FOR CONFIGURE_ADMIN: %s", UA_StatusCode_name(retval));
        testCaseFailureCount++;
    }

// Write value
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    UA_Variant_setScalar(&val, &value, &UA_TYPES[UA_TYPES_UINT16]);
    retval = UA_Client_writeValueAttribute(client, node_7, &val);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: WriteValue of Node 7 FOR CONFIGURE_ADMIN");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: WriteValue of Node 7 FOR CONFIGURE_ADMIN: %s", UA_StatusCode_name(retval));
        testCaseFailureCount++;
    }

    UA_Client_disconnect(client);    // Disconnect as Configure_Admin

    // TEST CASES FOR Anonymous //
    /* Check Client connection as Anonymous */
    retval = UA_Client_connectUsername(client, argv[1], "user_anonymous", "123456");

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    if(retval != UA_STATUSCODE_GOOD) {
        testCaseFailureCount++;
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Provided wrong Credentials: %s", UA_StatusCode_name(retval));
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }
    else {
        testCaseSuccessCount++;
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Login as Anonymous");
    }

//Read value
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    retval = UA_Client_readValueAttribute(client, node_5, &variant);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadValue Node 5 FOR ANONYMOUS");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: ReadValue Node 5 FOR ANONYMOUS: %s", UA_StatusCode_name(retval));
        testCaseFailureCount++;
    }

// Write value
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    UA_Variant_setScalar(&val, &value, &UA_TYPES[UA_TYPES_UINT16]);
    retval = UA_Client_writeValueAttribute(client, node_5, &val);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: WriteValue of Node 5 FOR ANONYMOUS");
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Since, user access level is restricted, Role permission will not override this parameter");
        // If required, modify the accessPermissions and methodAccessPermission in the setUserRole_settings function in the
        //   src/server/ua_server_role_access.c file
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: WriteValue of Node 5 FOR ANONYMOUS: %s", UA_StatusCode_name(retval));
        testCaseFailureCount++;
    }

    UA_Client_disconnect(client);    // Disconnect as Anonymous

    // TEST CASES FOR AUTHENTICATED_USER //
    /* Check Client connection as Authenticated_User */
    retval = UA_Client_connectUsername(client, argv[1], "user_auth", "123456");

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    if(retval != UA_STATUSCODE_GOOD) {
        testCaseFailureCount++;
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Provided wrong Credentials: %s", UA_StatusCode_name(retval));
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }
    else {
        testCaseSuccessCount++;
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Login as Authenticated_User");
    }

    /* "User does not have permission to read the value of the Node ID 1." */
    retval = UA_Client_readValueAttribute(client, node_1, &variant);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadValue of restricted Node 1 FOR AUTHENTICATED_USER");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: ReadValue of restricted Node 1 FOR AUTHENTICATED_USER: %s", UA_StatusCode_name(retval));
        testCaseFailureCount++;
    }

//Read value
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    retval = UA_Client_readValueAttribute(client, node_6, &variant);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: ReadValue Node 6 FOR AUTHENTICATED_USER");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: ReadValue Node 6 FOR AUTHENTICATED_USER: %s", UA_StatusCode_name(retval));
        testCaseFailureCount++;
    }

// Write value
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    UA_Variant_setScalar(&val, &value, &UA_TYPES[UA_TYPES_UINT16]);
    retval = UA_Client_writeValueAttribute(client, node_6, &val);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: WriteValue of Node 6 FOR AUTHENTICATED_USER");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: WriteValue of Node 6 FOR AUTHENTICATED_USER: %s", UA_StatusCode_name(retval));
        testCaseFailureCount++;
    }

//Read Role permission
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    readRolePermission = (UA_RolePermissionType *)UA_Array_new(3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);;
    readRolePermissionsize = 2;
    retval = UA_Client_readRolePermissionAttribute(client, node_6, &readRolePermissionsize , &readRolePermission);
    for (size_t index = 0; index < 2; index++)
        roleValue[index] = readRolePermission[index].permissions;

    if ((roleValue[0] == 0x1ffff) && (roleValue[1] == 0x1ffff)) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Write and Read RolePermission of Node 6");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: Write and Read RolePermission of Node 6");
        testCaseFailureCount++;
    }
    UA_Array_delete(readRolePermission, 2, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);

//Read User Role permission
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE: %d #####################", ++testCount);
    readUserRolePermission = (UA_RolePermissionType *)UA_Array_new(3, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);;
    readUserRolePermissionsize = 2;
    retval = UA_Client_readUserRolePermissionAttribute(client, node_6, &readUserRolePermissionsize , &readUserRolePermission);
    for (size_t index = 0; index < 2; index++)
        roleValue[index] = readUserRolePermission[index].permissions;

    if ((roleValue[0] == 0x867) && (roleValue[1] == 0x867)) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "SUCCESS: Read UserRolePermission of Node 6");
        testCaseSuccessCount++;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "FAILURE: Read UserRolePermission of Node 6");
        testCaseFailureCount++;
    }
    UA_Array_delete(readUserRolePermission, 2, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    // TEST CASES FOR AUTHENTICATED_USER //

    // Results
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "################ TEST CASE RESULT #####################");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Total test case count: %d", testCount);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Success: %d, Failure: %d", testCaseSuccessCount, testCaseFailureCount);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "#######################################################");

    /* Clean up of client */
    UA_Client_disconnect(client);
    UA_Client_delete(client); /* Disconnects the client internally */
    return EXIT_SUCCESS;
}
