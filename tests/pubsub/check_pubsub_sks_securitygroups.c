/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2022 Linutronix GmbH (Author: Muddasir Shakil)
 */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>

#include "test_helpers.h"
#include "ua_pubsub.h"
#include "ua_pubsub_keystorage.h"
#include "ua_server_internal.h"

#include <check.h>
#include <testing_clock.h>

#include "../encryption/certificates.h"
#include "thread_wrapper.h"

UA_Server *server = NULL;
UA_Boolean running;

static void
securityGroup_setup(void) {
    server = UA_Server_newForUnitTest();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    /* Instantiate the PubSub SecurityPolicy */
    config->pubSubConfig.securityPolicies =
        (UA_PubSubSecurityPolicy *)UA_calloc(2, sizeof(UA_PubSubSecurityPolicy));
    config->pubSubConfig.securityPoliciesSize = 2;
    UA_PubSubSecurityPolicy_Aes128Ctr(&config->pubSubConfig.securityPolicies[0],
                                      config->logging);
    UA_PubSubSecurityPolicy_Aes256Ctr(&config->pubSubConfig.securityPolicies[1],
                                      config->logging);

    UA_Server_run_startup(server);
}

static void
securityGroup_teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(AddSecurityGroupWithNullConfig) {
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    UA_NodeId securityGroupNodeId;
    retval = UA_Server_addSecurityGroup(
        server, UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_SECURITYGROUPS), NULL,
        &securityGroupNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);
}
END_TEST

START_TEST(AddSecurityGroupWithInvalidKeyLifeTime) {
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    UA_NodeId securityGroupNodeId;
    UA_SecurityGroupConfig config;
    memset(&config, 0, sizeof(UA_SecurityGroupConfig));
    config.keyLifeTime = 0;
    config.securityPolicyUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#PubSub-Aes256-CTR");
    config.securityGroupName = UA_STRING("TestSecurityGroup");
    config.maxFutureKeyCount = 10;
    config.maxPastKeyCount = 5;
    retval = UA_Server_addSecurityGroup(
        server, UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_SECURITYGROUPS), &config,
        &securityGroupNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);
}
END_TEST

START_TEST(AddSecurityGroupWithInvalidSecurityGroupName) {
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    UA_NodeId securityGroupNodeId;
    UA_SecurityGroupConfig config;
    memset(&config, 0, sizeof(UA_SecurityGroupConfig));
    config.keyLifeTime = 2000;
    config.securityPolicyUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#PubSub-Aes256-CTR");
    config.securityGroupName = UA_STRING_NULL;
    config.maxFutureKeyCount = 10;
    config.maxPastKeyCount = 5;
    retval = UA_Server_addSecurityGroup(
        server, UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_SECURITYGROUPS), &config,
        &securityGroupNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);
}
END_TEST

START_TEST(AddSecurityGroupWithInvalidPolicy) {
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    UA_NodeId securityGroupNodeId;
    UA_SecurityGroupConfig config;
    memset(&config, 0, sizeof(UA_SecurityGroupConfig));
    config.keyLifeTime = 2000;
    config.securityPolicyUri = UA_STRING_NULL;
    config.securityGroupName = UA_STRING("TestSecurityGroup");
    config.maxFutureKeyCount = 10;
    config.maxPastKeyCount = 5;
    retval = UA_Server_addSecurityGroup(
        server, UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_SECURITYGROUPS), &config,
        &securityGroupNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);

    config.securityPolicyUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#InvalidPolicy");
    retval = UA_Server_addSecurityGroup(
        server, UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_SECURITYGROUPS), &config,
        &securityGroupNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADSECURITYPOLICYREJECTED);
}
END_TEST

START_TEST(AddSecurityGroupWithInvalidSecurityGroupFolderNodeId) {
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    UA_NodeId securityGroupNodeId;
    UA_SecurityGroupConfig config;
    memset(&config, 0, sizeof(UA_SecurityGroupConfig));
    config.keyLifeTime = 2000;
    config.securityPolicyUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#PubSub-Aes256-CTR");
    config.securityGroupName = UA_STRING("TestSecurityGroup");
    config.maxFutureKeyCount = 10;
    config.maxPastKeyCount = 5;
    retval =
        UA_Server_addSecurityGroup(server, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                   &config, &securityGroupNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADPARENTNODEIDINVALID);
}
END_TEST

START_TEST(AddSecurityGroupWithvalidConfig) {
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    UA_NodeId securityGroupNodeId;
    UA_NodeId securityGroupParent =
        UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_SECURITYGROUPS);
    UA_SecurityGroupConfig config;
    memset(&config, 0, sizeof(UA_SecurityGroupConfig));
    config.keyLifeTime = 2000;
    config.securityPolicyUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#PubSub-Aes256-CTR");
    config.securityGroupName = UA_STRING("TestSecurityGroup");
    config.maxFutureKeyCount = 10;
    config.maxPastKeyCount = 5;

    retval = UA_Server_addSecurityGroup(server, securityGroupParent, &config,
                                        &securityGroupNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_LOCK(&server->serviceMutex);
    UA_SecurityGroup *sg = UA_SecurityGroup_findSGbyId(server, securityGroupNodeId);
    ck_assert_ptr_ne(sg, NULL);
    ck_assert(UA_NodeId_equal(&sg->securityGroupNodeId, &securityGroupNodeId) == UA_TRUE);
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    ck_assert(UA_NodeId_equal(&sg->securityGroupFolderId, &securityGroupParent) ==
              UA_TRUE);
#endif
    ck_assert(UA_String_equal(&sg->securityGroupId, &config.securityGroupName) ==
              UA_TRUE);
    UA_UNLOCK(&server->serviceMutex);

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    /*check properties*/
    UA_Variant value;
    UA_Variant_init(&value);
    retval = UA_Server_readObjectProperty(server, securityGroupNodeId,
                                          UA_QUALIFIEDNAME(0, "SecurityGroupId"), &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(value.type == &UA_TYPES[UA_TYPES_STRING]);
    ck_assert(UA_String_equal(&config.securityGroupName, (UA_String *)value.data) ==
              UA_TRUE);

    UA_Variant_clear(&value);
    retval = UA_Server_readObjectProperty(
        server, securityGroupNodeId, UA_QUALIFIEDNAME(0, "SecurityPolicyUri"), &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(UA_String_equal(&config.securityPolicyUri, (UA_String *)value.data) ==
              UA_TRUE);

    UA_Variant_clear(&value);
    retval = UA_Server_readObjectProperty(server, securityGroupNodeId,
                                          UA_QUALIFIEDNAME(0, "KeyLifetime"), &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(config.keyLifeTime == *(UA_Duration *)value.data);

    UA_Variant_clear(&value);
    retval = UA_Server_readObjectProperty(
        server, securityGroupNodeId, UA_QUALIFIEDNAME(0, "MaxFutureKeyCount"), &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(config.maxFutureKeyCount == *(UA_UInt32 *)value.data);

    UA_Variant_clear(&value);
    retval = UA_Server_readObjectProperty(server, securityGroupNodeId,
                                          UA_QUALIFIEDNAME(0, "MaxPastKeyCount"), &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(config.maxPastKeyCount == *(UA_UInt32 *)value.data);
    UA_Variant_clear(&value);
#endif /*UA_ENABLE_PUBSUB_INFORMATIONMODEL */
}
END_TEST

START_TEST(AddTwoSecurityGroupsWithSameSecurityGroupName) {
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    UA_NodeId securityGroupNodeId;
    UA_SecurityGroupConfig config;
    memset(&config, 0, sizeof(UA_SecurityGroupConfig));
    config.keyLifeTime = 2000;
    config.securityPolicyUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#PubSub-Aes256-CTR");
    config.securityGroupName = UA_STRING("TestSecurityGroup");
    config.maxFutureKeyCount = 10;
    config.maxPastKeyCount = 5;
    retval = UA_Server_addSecurityGroup(
        server, UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_SECURITYGROUPS), &config,
        &securityGroupNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = UA_Server_addSecurityGroup(
        server, UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_SECURITYGROUPS), &config,
        &securityGroupNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNODEIDEXISTS);
}
END_TEST

START_TEST(RemoveSecurityGroup) {
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    UA_NodeId securityGroupNodeId;
    UA_NodeId securityGroupParent =
        UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_SECURITYGROUPS);
    UA_SecurityGroupConfig config;
    memset(&config, 0, sizeof(UA_SecurityGroupConfig));
    config.keyLifeTime = 2000;
    config.securityPolicyUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#PubSub-Aes256-CTR");
    config.securityGroupName = UA_STRING("TestSecurityGroup");
    config.maxFutureKeyCount = 10;
    config.maxPastKeyCount = 5;

    retval = UA_Server_addSecurityGroup(server, securityGroupParent, &config,
                                        &securityGroupNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_LOCK(&server->serviceMutex);
    UA_SecurityGroup *sg = UA_SecurityGroup_findSGbyId(server, securityGroupNodeId);
    ck_assert_ptr_ne(sg, NULL);
    UA_UNLOCK(&server->serviceMutex);

    UA_Server_removeSecurityGroup(server, securityGroupNodeId);

    UA_LOCK(&server->serviceMutex);
    sg = UA_SecurityGroup_findSGbyId(server, securityGroupNodeId);
    ck_assert_ptr_eq(sg, NULL);
    UA_UNLOCK(&server->serviceMutex);
} END_TEST

START_TEST(AddSecurityGroupWithKeyManagement){
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    UA_NodeId securityGroupNodeId;
    UA_NodeId securityGroupParent =
        UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_SECURITYGROUPS);
    UA_SecurityGroupConfig config;
    memset(&config, 0, sizeof(UA_SecurityGroupConfig));
    config.keyLifeTime = 2000;
    config.securityPolicyUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#PubSub-Aes256-CTR");
    config.securityGroupName = UA_STRING("TestSecurityGroup");
    config.maxFutureKeyCount = 1;
    config.maxPastKeyCount = 1;

    retval = UA_Server_addSecurityGroup(server, securityGroupParent, &config,
                                        &securityGroupNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_LOCK(&server->serviceMutex);
    UA_SecurityGroup *sg = UA_SecurityGroup_findSGbyId(server, securityGroupNodeId);
    ck_assert_ptr_ne(sg, NULL);
    UA_PubSubKeyStorage *ks = UA_PubSubKeyStorage_findKeyStorage(server, sg->securityGroupId);
    ck_assert_ptr_ne(ks, NULL);
    ck_assert_uint_eq(ks->keyListSize, 1 + config.maxFutureKeyCount);
    UA_UInt32 expectKeyId = 1;
    UA_PubSubKeyListItem *iterator = TAILQ_FIRST(&ks->keyList);
    for (size_t i = 0; i < ks->keyListSize; i++) {
        ck_assert_ptr_ne(iterator, NULL);
        ck_assert_uint_eq(iterator->keyID, expectKeyId);
        ck_assert(UA_ByteString_equal(&iterator->key, &UA_BYTESTRING_NULL) != UA_TRUE);
        iterator = TAILQ_NEXT(iterator, keyListEntry);
        expectKeyId++;
    }
    UA_UNLOCK(&server->serviceMutex);
} END_TEST

START_TEST(SecurityGroupPeriodicInsertNewKeys){
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    UA_NodeId securityGroupNodeId;
    UA_NodeId securityGroupParent =
        UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_SECURITYGROUPS);
    UA_SecurityGroupConfig config;
    memset(&config, 0, sizeof(UA_SecurityGroupConfig));
    config.keyLifeTime = 500;
    config.securityPolicyUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#PubSub-Aes256-CTR");
    config.securityGroupName = UA_STRING("TestSecurityGroup");
    config.maxFutureKeyCount = 1;
    config.maxPastKeyCount = 1;

    retval = UA_Server_addSecurityGroup(server, securityGroupParent, &config,
                                        &securityGroupNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_LOCK(&server->serviceMutex);
    UA_SecurityGroup *sg = UA_SecurityGroup_findSGbyId(server, securityGroupNodeId);
    ck_assert_ptr_ne(sg, NULL);
    UA_PubSubKeyStorage *ks = UA_PubSubKeyStorage_findKeyStorage(server, sg->securityGroupId);
    ck_assert_ptr_ne(ks, NULL);

    UA_UInt32 expectKeyId = 1;
    UA_PubSubKeyListItem *preLastItem = TAILQ_LAST(&ks->keyList, keyListItems);
    UA_UNLOCK(&server->serviceMutex);
    for (size_t i = 0; i < ks->keyListSize; i++) {
        UA_fakeSleep(500);
        UA_Server_run_iterate(server, false);
        UA_PubSubKeyListItem *newlastItem = TAILQ_LAST(&ks->keyList, keyListItems);
        ck_assert_uint_eq(ks->keyListSize, config.maxPastKeyCount + 1 + config.maxFutureKeyCount);
        ck_assert_ptr_eq(preLastItem->keyListEntry.tqe_next, newlastItem);
        ck_assert_uint_eq(preLastItem->keyID + 1 ,  newlastItem->keyID);
        ck_assert(UA_ByteString_equal(&preLastItem->keyListEntry.tqe_next->key, &newlastItem->key) == UA_TRUE);
        preLastItem = newlastItem;
        expectKeyId++;
    }
} END_TEST

int
main(void) {
    int number_failed = 0;
    TCase *tc_pubsub_sks_securityGroup = tcase_create("PubSub SKS SecurityGroup");
    tcase_add_checked_fixture(tc_pubsub_sks_securityGroup, securityGroup_setup,
                              securityGroup_teardown);
    tcase_add_test(tc_pubsub_sks_securityGroup, AddSecurityGroupWithvalidConfig);
    tcase_add_test(tc_pubsub_sks_securityGroup, AddSecurityGroupWithNullConfig);
    tcase_add_test(tc_pubsub_sks_securityGroup, AddSecurityGroupWithInvalidKeyLifeTime);
    tcase_add_test(tc_pubsub_sks_securityGroup,
                   AddSecurityGroupWithInvalidSecurityGroupName);
    tcase_add_test(tc_pubsub_sks_securityGroup, AddSecurityGroupWithInvalidPolicy);
    tcase_add_test(tc_pubsub_sks_securityGroup,
                   AddSecurityGroupWithInvalidSecurityGroupFolderNodeId);
    tcase_add_test(tc_pubsub_sks_securityGroup,
                   AddTwoSecurityGroupsWithSameSecurityGroupName);
    tcase_add_test(tc_pubsub_sks_securityGroup, RemoveSecurityGroup);
    tcase_add_test(tc_pubsub_sks_securityGroup, AddSecurityGroupWithKeyManagement);
    tcase_add_test(tc_pubsub_sks_securityGroup, SecurityGroupPeriodicInsertNewKeys);
    Suite *s = suite_create("PubSub SKS SecurityGroups");
    suite_add_tcase(s, tc_pubsub_sks_securityGroup);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
