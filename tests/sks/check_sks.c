/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 */

#include <open62541/client_highlevel.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/server_sks.h>
#include <open62541/plugin/nodestore_default.h>

#include "open62541/types_generated_encoding_binary.h"

#include "ua_pubsub.h"
#include "ua_server_internal.h"

#include <check.h>

#include "../encryption/certificates.h"

/* Global declaration for test cases  */
UA_Server *server = NULL;
UA_ServerConfig *config = NULL;

#define POLICY_URI1 "http://opcfoundation.org/UA/SecurityPolicy#PubSub-Aes128-CTR"
#define POLICY_URI2 "http://opcfoundation.org/UA/SecurityPolicy#PubSub-Aes256-CTR"

/* setup() is to create an environment for test cases */
static void
setup(void) {
    /*Add setup by creating new server with valid configuration */
    UA_StatusCode retVal;

    server = UA_Server_new();
    config = UA_Server_getConfig(server);

    UA_ByteString key;
    key.data = KEY_DER_DATA;
    key.length = KEY_DER_LENGTH;

    UA_ByteString cert;
    cert.data = CERT_DER_DATA;
    cert.length = CERT_DER_LENGTH;

    retVal = UA_ServerConfig_setDefaultWithSecurityPolicies(config, 4840, &cert, &key,
                                                            NULL, 0, NULL, 0, NULL, 0);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    retVal = UA_ServerConfig_addSecurityPolicy_Pubsub_Aes128ctr(
        config, &UA_BYTESTRING_NULL, &UA_BYTESTRING_NULL);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    UA_Server_run_startup(server);
}

/* teardown() is to delete the environment set for test cases */
static void
teardown(void) {
    /*Call server delete functions */
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(AddSKS) {
    /* Test if UA_Server_addSKS returns with success and all relevant items are present */
    UA_NodeId retNodeId;

    ck_assert_int_eq(UA_Server_addSKS(server), UA_STATUSCODE_GOOD);

    ck_assert_int_eq(UA_Server_readNodeId(server, NODEID_SKS_GetSecurityKeys, &retNodeId),
                     UA_STATUSCODE_GOOD);
    ck_assert_int_eq(
        UA_Server_readNodeId(server, NODEID_SKS_GetSecurityGroup, &retNodeId),
        UA_STATUSCODE_GOOD);
    ck_assert_int_eq(
        UA_Server_readNodeId(server, NODEID_SKS_AddSecurityGroup, &retNodeId),
        UA_STATUSCODE_GOOD);
    ck_assert_int_eq(
        UA_Server_readNodeId(server, NODEID_SKS_RemoveSecurityGroup, &retNodeId),
        UA_STATUSCODE_GOOD);
    ck_assert_int_eq(
        UA_Server_readNodeId(server, NODEID_SKS_SecurityGroupType, &retNodeId),
        UA_STATUSCODE_GOOD);
    ck_assert_int_eq(
        UA_Server_readNodeId(server, NODEID_SKS_SecurityRootFolder, &retNodeId),
        UA_STATUSCODE_GOOD);
}
END_TEST

static UA_MethodNode *
getMethod(UA_NodeId id) {
    UA_Node *node;
    UA_StatusCode ret = UA_NODESTORE_GETCOPY(server, &id, &node);
    ck_assert_int_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_ptr_ne(node, NULL);
    ck_assert_int_eq(node->nodeClass, UA_NODECLASS_METHOD);
    return (UA_MethodNode *)node;
}

static void
releaseMethod(UA_MethodNode* method) {
    UA_NODESTORE_DELETE(server, (UA_Node* )method);
}

static UA_StatusCode
testAddSecurityGroup(const char *groupName, double lifeTime, const char *uri,
                     unsigned int futureKeys, unsigned int pastKeys,
                     UA_NodeId *newNodeId) {
    UA_StatusCode retVal;
    UA_NodeId n = NODEID_SKS_SecurityRootFolder;

    ck_assert_int_eq(UA_Server_addSKS(server), UA_STATUSCODE_GOOD);
    UA_MethodNode *addSecurityGroup = getMethod(NODEID_SKS_AddSecurityGroup);

    UA_Variant inputs[5];
    UA_Variant outputs[2];

    UA_String securityGroupName = UA_STRING_ALLOC(groupName);
    UA_Variant_setScalar(&inputs[0], &securityGroupName, &UA_TYPES[UA_TYPES_STRING]);

    UA_Double time = lifeTime;
    UA_Variant_setScalar(&inputs[1], &time, &UA_TYPES[UA_TYPES_DOUBLE]);

    UA_String u = UA_STRING_ALLOC(uri);
    UA_Variant_setScalar(&inputs[2], &u, &UA_TYPES[UA_TYPES_STRING]);

    UA_UInt32 f = futureKeys;
    UA_Variant_setScalar(&inputs[3], &f, &UA_TYPES[UA_TYPES_UINT32]);

    UA_UInt32 p = pastKeys;
    UA_Variant_setScalar(&inputs[4], &p, &UA_TYPES[UA_TYPES_UINT32]);

    retVal = addSecurityGroup->method(server, NULL, NULL, NULL, NULL, &n, NULL, 5, inputs,
                                      2, outputs);

    if(retVal == UA_STATUSCODE_GOOD) {
        ck_assert_ptr_ne(outputs[0].type, NULL);
        ck_assert_int_eq(outputs[0].type->typeIndex, UA_TYPES_STRING);

        ck_assert_ptr_ne(outputs[1].type, NULL);
        ck_assert_int_eq(outputs[1].type->typeIndex, UA_TYPES_NODEID);

        UA_NodeId *retNodeId = (UA_NodeId *)outputs[1].data;
        if(newNodeId)
            *newNodeId = *retNodeId;
        UA_NodeId temp;
        ck_assert_int_eq(UA_Server_readNodeId(server, *retNodeId, &temp),
                         UA_STATUSCODE_GOOD);
    }

    releaseMethod(addSecurityGroup);
    return retVal;
}

START_TEST(AddSecurityGroup) {
    ck_assert_int_eq(testAddSecurityGroup("1", 1, POLICY_URI1, 1, 1, NULL),
                     UA_STATUSCODE_GOOD);
    ck_assert_int_eq(testAddSecurityGroup("1", 1, POLICY_URI1, 1, 1, NULL),
                     UA_STATUSCODE_BADNODEIDEXISTS);
    ck_assert_int_eq(testAddSecurityGroup("2", 1, POLICY_URI1, 1, 1, NULL),
                     UA_STATUSCODE_GOOD);
    //ck_assert_int_eq(testAddSecurityGroup("3", 1, POLICY_URI2, 1, 1, NULL), UA_STATUSCODE_GOOD);

    ck_assert_int_ne(testAddSecurityGroup("4", 1, "", 1, 1, NULL), UA_STATUSCODE_GOOD);
    ck_assert_int_ne(testAddSecurityGroup("5", 1, NULL, 1, 1, NULL), UA_STATUSCODE_GOOD);
    ck_assert_int_ne(testAddSecurityGroup(NULL, 1, POLICY_URI1, 1, 1, NULL),
                     UA_STATUSCODE_GOOD);
    ck_assert_int_ne(testAddSecurityGroup("", 1, POLICY_URI1, 1, 1, NULL),
                     UA_STATUSCODE_GOOD);
    ck_assert_int_ne(testAddSecurityGroup("6", -1, POLICY_URI1, 1, 1, NULL),
                     UA_STATUSCODE_GOOD);
}
END_TEST

static UA_StatusCode
testRemoveSecurityGroup(UA_NodeId securityGroupId) {
    UA_StatusCode retVal;
    UA_NodeId n = NODEID_SKS_SecurityRootFolder;

    ck_assert_int_eq(UA_Server_addSKS(server), UA_STATUSCODE_GOOD);
    UA_MethodNode *removeSecurityGroup = getMethod(NODEID_SKS_RemoveSecurityGroup);

    UA_Variant inputs[1];

    UA_Variant_setScalar(&inputs[0], &securityGroupId, &UA_TYPES[UA_TYPES_NODEID]);

    retVal = removeSecurityGroup->method(server, NULL, NULL, NULL, NULL, &n, NULL, 1,
                                         inputs, 0, NULL);

    releaseMethod(removeSecurityGroup);
    return retVal;
}

START_TEST(RemoveSecurityGroup) {
    UA_NodeId n;
    ck_assert_int_eq(testAddSecurityGroup("1", 1, POLICY_URI1, 1, 1, &n),
                     UA_STATUSCODE_GOOD);
    ck_assert_int_eq(testRemoveSecurityGroup(n), UA_STATUSCODE_GOOD);
    ck_assert_int_ne(testRemoveSecurityGroup(n), UA_STATUSCODE_GOOD);
    ck_assert_int_ne(testRemoveSecurityGroup(NODEID_SKS_SecurityRootFolder),
                     UA_STATUSCODE_GOOD);
}
END_TEST

static UA_StatusCode
testGetSecurityGroup(const char *groupName, UA_NodeId *newNodeId) {
    UA_StatusCode retVal;
    UA_NodeId n = NODEID_SKS_SecurityRootFolder;

    ck_assert_int_eq(UA_Server_addSKS(server), UA_STATUSCODE_GOOD);
    UA_MethodNode *getSecurityGroup = getMethod(NODEID_SKS_GetSecurityGroup);

    UA_Variant inputs[1];
    UA_Variant outputs[1];

    UA_String securityGroupName = UA_STRING_ALLOC(groupName);
    UA_Variant_setScalar(&inputs[0], &securityGroupName, &UA_TYPES[UA_TYPES_STRING]);

    retVal = getSecurityGroup->method(server, NULL, NULL, NULL, NULL, &n, NULL, 1, inputs,
                                      1, outputs);

    if(retVal == UA_STATUSCODE_GOOD) {
        ck_assert_ptr_ne(outputs[0].type, NULL);
        ck_assert_int_eq(outputs[0].type->typeIndex, UA_TYPES_NODEID);

        UA_NodeId *retNodeId = (UA_NodeId *)outputs[0].data;
        if(newNodeId)
            *newNodeId = *retNodeId;
        UA_NodeId temp;
        ck_assert_int_eq(UA_Server_readNodeId(server, *retNodeId, &temp),
                         UA_STATUSCODE_GOOD);
    }

    releaseMethod(getSecurityGroup);
    return retVal;
}

START_TEST(GetSecurityGroup) {
    UA_NodeId n1;
    UA_NodeId n2;
    ck_assert_int_eq(testAddSecurityGroup("1", 1, POLICY_URI1, 1, 1, &n1),
                     UA_STATUSCODE_GOOD);

    ck_assert_int_eq(testGetSecurityGroup("1", &n2), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_TRUE, UA_NodeId_equal(&n1, &n2));

    ck_assert_int_ne(testGetSecurityGroup("2", &n2), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(testRemoveSecurityGroup(n1), UA_STATUSCODE_GOOD);
    ck_assert_int_ne(testGetSecurityGroup("1", &n2), UA_STATUSCODE_GOOD);
}
END_TEST

static UA_StatusCode
testGetSecurityKeys(const char *groupId, unsigned int startingTokenId, unsigned int count,
        const char* expectedPolicyUri, 
        unsigned int expectedReturnedKeyCount,UA_Variant* outputsCopy) {
    UA_StatusCode retVal;
    UA_NodeId n = NODEID_SKS_SecurityRootFolder;

    ck_assert_int_eq(UA_Server_addSKS(server), UA_STATUSCODE_GOOD);
    UA_MethodNode *getSecurityKeys = getMethod(NODEID_SKS_GetSecurityKeys);

    UA_Variant inputs[3];
    UA_Variant outputs[5];

    UA_String securityGroupId = UA_STRING_ALLOC(groupId);
    UA_Variant_setScalar(&inputs[0], &securityGroupId, &UA_TYPES[UA_TYPES_STRING]);

    UA_UInt32 p = startingTokenId;
    UA_Variant_setScalar(&inputs[1], &p, &UA_TYPES[UA_TYPES_UINT32]);

    UA_UInt32 p1 = count;
    UA_Variant_setScalar(&inputs[2], &p1, &UA_TYPES[UA_TYPES_UINT32]);

    retVal = getSecurityKeys->method(server, NULL, NULL, NULL, NULL, &n, NULL, 3, inputs,
                                      5, outputs);
    
    
    
    if(retVal == UA_STATUSCODE_GOOD) {
        
        for (size_t i=0;i<5;i++)
        {
        retVal=UA_Variant_copy(&outputs[i],&outputsCopy[i]);
        }
        /* security policy uri */
        ck_assert_ptr_ne(outputs[0].type, NULL);
        ck_assert_int_eq(outputs[0].type->typeIndex, UA_TYPES_STRING);
        UA_String expectedUriString = UA_String_fromChars(expectedPolicyUri);
        ck_assert_int_ne(0, UA_String_equal(&expectedUriString, (UA_String* )outputs[0].data));

        /* FirstTokenId */
        ck_assert_ptr_ne(outputs[1].type, NULL);
        ck_assert_int_eq(outputs[1].type->typeIndex, UA_TYPES_UINT32);

        /* Keys */
        ck_assert_ptr_ne(outputs[2].type, NULL);
        ck_assert_int_eq(outputs[2].type->typeIndex, UA_TYPES_BYTESTRING);
        ck_assert_int_eq(outputs[2].arrayLength, expectedReturnedKeyCount);

        /* TimeToNextKey */
        ck_assert_ptr_ne(outputs[3].type, NULL);
        ck_assert_int_ne(0, outputs[3].type->typeIndex == UA_TYPES_DURATION || outputs[3].type->typeIndex == UA_TYPES_DOUBLE);
        ck_assert_int_ne(0, *(UA_Duration *)outputs[3].data > 0);

        /* KeyLifetime */
        ck_assert_ptr_ne(outputs[4].type, NULL);
        ck_assert_int_ne(0, outputs[4].type->typeIndex == UA_TYPES_DURATION || outputs[3].type->typeIndex == UA_TYPES_DOUBLE);
        ck_assert_int_ne(0, *(UA_Duration *)outputs[3].data > 0);
    }

    releaseMethod(getSecurityKeys);
    return retVal;
}

START_TEST(GetSecurityKeysParameters) {
    UA_NodeId n1;
    UA_Variant outputCopy[5];
    UA_ByteString *Keys;
    ck_assert_int_eq(testAddSecurityGroup("1", 1, POLICY_URI1, 2, 1, &n1),
                     UA_STATUSCODE_GOOD);

    ck_assert_int_eq(testGetSecurityKeys("1", 0, 2, POLICY_URI1, 2, outputCopy),
                     UA_STATUSCODE_GOOD);
    /* Check if server corectly update keys, first ask for currentKey KeyID should be 1*/
    Keys= (UA_ByteString *)outputCopy[2].data;
    UA_ByteString oldKeyID1;
    UA_ByteString_init(&oldKeyID1);
    UA_ByteString_copy(&Keys[0], &oldKeyID1);
    /* Sleep for a key update period,First check if can get the old key ,then check if another key is different*/
    sleep(10);
    ck_assert_int_eq(testGetSecurityKeys("1", 1, 2, POLICY_URI1, 2, outputCopy),
                     UA_STATUSCODE_GOOD);
    Keys = (UA_ByteString *)outputCopy[2].data;
    UA_ByteString newKeyID1;
    UA_ByteString_init(&newKeyID1);
    UA_ByteString_copy(&Keys[0], &newKeyID1);
    UA_ByteString newKeyID2;
    UA_ByteString_init(&newKeyID2);
    UA_ByteString_copy(&Keys[1], &newKeyID2);
    ck_assert_int_eq(UA_ByteString_equal(&oldKeyID1, &newKeyID1), UA_TRUE);
    ck_assert_int_eq(UA_ByteString_equal(&oldKeyID1, &newKeyID2), UA_FALSE);
    /* "If the caller requests a number larger than the Security Key Service
permits, then the SKS shall return the maximum it allows": 2 permitted, 3 requested -> 2 returned*/
    ck_assert_int_eq(testGetSecurityKeys("1", 0, 4, POLICY_URI1, 3,outputCopy), UA_STATUSCODE_GOOD);

    /* "If 0 is requested, no future keys are returned" -> only 1 key (current key) returned */
    ck_assert_int_eq(testGetSecurityKeys("1", 0, 0, POLICY_URI1, 1,outputCopy), UA_STATUSCODE_GOOD);

    /* "If 1 is requested, 1 returned */
    ck_assert_int_eq(testGetSecurityKeys("1", 0, 0, POLICY_URI1, 1,outputCopy), UA_STATUSCODE_GOOD);

    //ck_assert_int_ne(testGetSecurityKeys("1", 0, 2, POLICY_URI2, 2), UA_STATUSCODE_GOOD);
    ck_assert_int_ne(testGetSecurityKeys("2", 0, 2, POLICY_URI1, 2,outputCopy), UA_STATUSCODE_GOOD);
}
END_TEST

int
main(void) {
    TCase *tc_sks = tcase_create("SKS testcase");
    tcase_add_checked_fixture(tc_sks, setup, teardown);

    tcase_add_test(tc_sks, AddSKS);
    tcase_add_test(tc_sks, AddSecurityGroup);
    tcase_add_test(tc_sks, RemoveSecurityGroup);
    tcase_add_test(tc_sks, GetSecurityGroup);
    tcase_add_test(tc_sks, GetSecurityKeysParameters);

    Suite *suite = suite_create("SKS - Tests for SKS Server API");
    suite_add_tcase(suite, tc_sks);

    SRunner *suiteRunner = srunner_create(suite);
    srunner_set_fork_status(suiteRunner, CK_NOFORK);
    srunner_run_all(suiteRunner, CK_NORMAL);
    int number_failed = srunner_ntests_failed(suiteRunner);
    srunner_free(suiteRunner);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
