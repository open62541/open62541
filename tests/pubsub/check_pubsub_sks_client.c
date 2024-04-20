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

#include "../encryption/certificates.h"
#include "testing_clock.h"
#include "thread_wrapper.h"
#include "open62541/plugin/accesscontrol_default.h"

#define PUBLISHER_ID 2234             /*Publisher Id*/
#define WRITER_GROUP_ID 100           /*Writer group Id*/
#define DATASET_WRITER_ID 62541       /*Dataset Writer id*/
#define PUBLISHVARIABLE_NODEID 1000   /* Published data nodeId */
#define SUBSCRIBEVARIABLE_NODEID 1002 /* Subscribed data nodeId */
#define UA_PUBSUB_KEYMATERIAL_NONCELENGTH 32
#define policUri "http://opcfoundation.org/UA/SecurityPolicy#PubSub-Aes256-CTR"
#define testingSKSEndpointUrl "opc.tcp://localhost:4840"
#define MAX_RETRIES 1000

#define TESTINGSECURITYMODE UA_MESSAGESECURITYMODE_SIGNANDENCRYPT

static UA_UsernamePasswordLogin userNamePW[2] = {
    {UA_STRING_STATIC("user1"), UA_STRING_STATIC("password")},
    {UA_STRING_STATIC("user2"), UA_STRING_STATIC("password2")}
};


UA_Boolean running;
UA_UInt32 maxKeyCount;
UA_String securityGroupId;
THREAD_HANDLE server_thread;
UA_Server *sksServer, *publisherApp, *subscriberApp;
UA_NodeId writerGroupId, readerGroupId;
UA_NodeId publisherConnection, subscriberConnection;
UA_ByteString allowedUsername;
THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(sksServer, true);
    return 0;
}

typedef struct {
    UA_Boolean allowAnonymous;
    size_t usernamePasswordLoginSize;
    UA_UsernamePasswordLogin *usernamePasswordLogin;
    UA_UsernamePasswordLoginCallback loginCallback;
    void *loginContext;
    UA_CertificateGroup verifyX509;
} AccessControlContext;

#define ANONYMOUS_POLICY "open62541-anonymous-policy"
#define CERTIFICATE_POLICY "open62541-certificate-policy"
#define USERNAME_POLICY "open62541-username-policy"
const UA_String anonymousPolicy = UA_STRING_STATIC(ANONYMOUS_POLICY);
const UA_String certificatePolicy = UA_STRING_STATIC(CERTIFICATE_POLICY);
const UA_String usernamePolicy = UA_STRING_STATIC(USERNAME_POLICY);


static void
disableAnonymous(UA_ServerConfig *config) {
    for(size_t i = 0; i < config->endpointsSize; i++) {
        UA_EndpointDescription *ep = &config->endpoints[i];

        for(size_t j = 0; j < ep->userIdentityTokensSize; j++) {
            UA_UserTokenPolicy *utp = &ep->userIdentityTokens[j];
            if(utp->tokenType != UA_USERTOKENTYPE_ANONYMOUS)
                continue;

            UA_UserTokenPolicy_clear(utp);
            /* Move the last to this position */
            if(j + 1 < ep->userIdentityTokensSize) {
                ep->userIdentityTokens[j] = ep->userIdentityTokens[ep->userIdentityTokensSize-1];
                j--;
            }
            ep->userIdentityTokensSize--;
        }

        /* Delete the entire array if the last UserTokenPolicy was removed */
        if(ep->userIdentityTokensSize == 0) {
            UA_free(ep->userIdentityTokens);
            ep->userIdentityTokens = NULL;
        }
    }
}

static void
addSecurityGroup(void) {
    UA_NodeId securityGroupParent =
        UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_SECURITYGROUPS);
    UA_NodeId outNodeId;
    UA_SecurityGroupConfig config;
    memset(&config, 0, sizeof(UA_SecurityGroupConfig));
    config.keyLifeTime = 1000;
    config.securityPolicyUri = UA_STRING(policUri);
    config.securityGroupName = UA_STRING("TestSecurityGroup");
    config.maxFutureKeyCount = 1;
    config.maxPastKeyCount = 1;

    maxKeyCount = config.maxPastKeyCount + 1 + config.maxFutureKeyCount;

    UA_Server_addSecurityGroup(sksServer, securityGroupParent, &config, &outNodeId);
    securityGroupId = config.securityGroupName;

    allowedUsername = UA_STRING("user1");
    ck_assert_int_eq(UA_Server_setNodeContext(sksServer, outNodeId, &allowedUsername),
                     UA_STATUSCODE_GOOD);
}

static UA_Boolean
getUserExecutableOnObject_sks(UA_Server *server, UA_AccessControl *ac,
                              const UA_NodeId *sessionId, void *sessionContext,
                              const UA_NodeId *methodId, void *methodContext,
                              const UA_NodeId *objectId, void *objectContext) {
    if(!objectContext)
        return true;

    UA_Session *session = getSessionById(server, sessionId);
    UA_ByteString *username = (UA_ByteString *)objectContext;
    return UA_ByteString_equal(username, &session->clientUserIdOfSession);
}

static void
skssetup(void) {
    running = true;

    UA_ByteString certificate;
    certificate.length = CERT_DER_LENGTH;
    certificate.data = CERT_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_DER_LENGTH;
    privateKey.data = KEY_DER_DATA;

    size_t trustListSize = 0;
    UA_ByteString *trustList = NULL;
    size_t issuerListSize = 0;
    UA_ByteString *issuerList = NULL;
    UA_ByteString *revocationList = NULL;
    size_t revocationListSize = 0;

    sksServer = UA_Server_newForUnitTestWithSecurityPolicies(4840, &certificate, &privateKey,
                                                             trustList, trustListSize,
                                                             issuerList, issuerListSize,
                                                             revocationList, revocationListSize);
    UA_ServerConfig *config = UA_Server_getConfig(sksServer);
    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
        UA_STRING_ALLOC("urn:unconfigured:application");

    UA_String basic256sha256 = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
    UA_AccessControl_default(config, true, &basic256sha256, 2, userNamePW);

    disableAnonymous(config);

    config->pubSubConfig.securityPolicies =
        (UA_PubSubSecurityPolicy *)UA_malloc(sizeof(UA_PubSubSecurityPolicy));
    config->pubSubConfig.securityPoliciesSize = 1;
    UA_PubSubSecurityPolicy_Aes256Ctr(config->pubSubConfig.securityPolicies,
                                      config->logging);

    /*User Access Control*/
    config->accessControl.getUserExecutableOnObject = getUserExecutableOnObject_sks;

    addSecurityGroup();

    UA_Server_run_startup(sksServer);
    THREAD_CREATE(server_thread, serverloop);
}

static void
publishersetup(void) {
    running = true;
    publisherApp = UA_Server_newForUnitTest();
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_ServerConfig *config = UA_Server_getConfig(publisherApp);
    retVal |= UA_ServerConfig_setMinimal(config, 4841, NULL);

    config->pubSubConfig.securityPolicies =
        (UA_PubSubSecurityPolicy *)UA_malloc(sizeof(UA_PubSubSecurityPolicy));
    config->pubSubConfig.securityPoliciesSize = 1;
    UA_PubSubSecurityPolicy_Aes256Ctr(config->pubSubConfig.securityPolicies,
                                      config->logging);

    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {
        UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    connectionConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    connectionConfig.publisherId.id.uint16 = PUBLISHER_ID;
    retVal |= UA_Server_addPubSubConnection(publisherApp, &connectionConfig, &publisherConnection);
    retVal |= UA_Server_run_startup(publisherApp);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
}

static void
subscribersetup(void) {
    running = true;
    subscriberApp = UA_Server_newForUnitTest();
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_ServerConfig *config = UA_Server_getConfig(subscriberApp);
    retVal |= UA_ServerConfig_setMinimal(config, 4842, NULL);

    config->pubSubConfig.securityPolicies =
        (UA_PubSubSecurityPolicy *)UA_malloc(sizeof(UA_PubSubSecurityPolicy));
    config->pubSubConfig.securityPoliciesSize = 1;
    UA_PubSubSecurityPolicy_Aes256Ctr(config->pubSubConfig.securityPolicies,
                                      config->logging);

    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {
        UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    retVal |= UA_Server_addPubSubConnection(subscriberApp, &connectionConfig,
                                  &subscriberConnection);
    retVal |= UA_Server_run_startup(subscriberApp);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
}

static void
sksteardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(sksServer);
    UA_Server_delete(sksServer);
}

static void
publisherteardown(void) {
    running = false;
    UA_Server_run_shutdown(publisherApp);
    UA_Server_delete(publisherApp);
}

static void
subscriberteardown(void) {
    running = false;
    UA_Server_run_shutdown(subscriberApp);
    UA_Server_delete(subscriberApp);
}

static UA_StatusCode
addPublisher(UA_Server *server) {
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    UA_NodeId publishedDataSetIdent;
    UA_NodeId dataSetFieldIdent;
    UA_NodeId dataSetWriterIdent;

    UA_ServerConfig *config = UA_Server_getConfig(server);

    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("Demo WriterGroup");
    writerGroupConfig.publishingInterval = 100;
    writerGroupConfig.enabled = UA_FALSE;
    writerGroupConfig.writerGroupId = WRITER_GROUP_ID;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;

    writerGroupConfig.securityMode = TESTINGSECURITYMODE;
    writerGroupConfig.securityGroupId = securityGroupId;
    writerGroupConfig.securityPolicy = &config->pubSubConfig.securityPolicies[0];

    writerGroupConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    writerGroupConfig.messageSettings.content.decoded.type =
        &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
    UA_UadpWriterGroupMessageDataType *writerGroupMessage =
        UA_UadpWriterGroupMessageDataType_new();
    writerGroupMessage->networkMessageContentMask =
        (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                           (UA_UadpNetworkMessageContentMask)
                                               UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
                                           (UA_UadpNetworkMessageContentMask)
                                               UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
                                           (UA_UadpNetworkMessageContentMask)
                                               UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
    writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;

    retval = UA_Server_addWriterGroup(server, publisherConnection, &writerGroupConfig,
                                      &writerGroupId);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
    UA_PublishedDataSetConfig publishedDataSetConfig;
    memset(&publishedDataSetConfig, 0, sizeof(UA_PublishedDataSetConfig));
    publishedDataSetConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    publishedDataSetConfig.name = UA_STRING("test PDS");
    retval = UA_Server_addPublishedDataSet(server, &publishedDataSetConfig,
                                  &publishedDataSetIdent).addResult;
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    /* Create variable to publish integer data */
    UA_NodeId publisherNode;
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en-US", "The answer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "The answer");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_Int32 publisherData = 42;
    UA_Variant_setScalar(&attr.value, &publisherData, &UA_TYPES[UA_TYPES_INT32]);

    retval = UA_Server_addVariableNode(
        server, UA_NODEID_NUMERIC(1, PUBLISHVARIABLE_NODEID),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(1, "The answer"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, &publisherNode);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    UA_DataSetFieldConfig dataSetFieldConfig;
    memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
    dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("The answer");
    dataSetFieldConfig.field.variable.promotedField = UA_FALSE;
    dataSetFieldConfig.field.variable.publishParameters.publishedVariable = publisherNode;
    dataSetFieldConfig.field.variable.publishParameters.attributeId =
        UA_ATTRIBUTEID_VALUE;
    retval = UA_Server_addDataSetField(server, publishedDataSetIdent, &dataSetFieldConfig,
                              &dataSetFieldIdent).result;
    ck_assert_int_eq(retval , UA_STATUSCODE_GOOD);
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Demo DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = DATASET_WRITER_ID;
    dataSetWriterConfig.keyFrameCount = 10;
    retval = UA_Server_addDataSetWriter(server, writerGroupId, publishedDataSetIdent,
                               &dataSetWriterConfig, &dataSetWriterIdent);
    ck_assert_int_eq(retval , UA_STATUSCODE_GOOD);
    return retval;
}

static UA_StatusCode
addSubscriber(UA_Server *server) {
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    UA_NodeId readerIdentifier;

    UA_ReaderGroupConfig readerGroupConfig;
    memset(&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
    readerGroupConfig.name = UA_STRING("ReaderGroup1");

    /* Encryption settings */
    UA_ServerConfig *config = UA_Server_getConfig(server);
    readerGroupConfig.securityMode = TESTINGSECURITYMODE;
    readerGroupConfig.securityGroupId = securityGroupId;
    readerGroupConfig.securityPolicy = &config->pubSubConfig.securityPolicies[0];

    retval |= UA_Server_addReaderGroup(server, subscriberConnection, &readerGroupConfig,
                                       &readerGroupId);

    UA_DataSetReaderConfig readerConfig;
    memset(&readerConfig, 0, sizeof(UA_DataSetReaderConfig));
    readerConfig.name = UA_STRING("DataSet Reader 1");
    UA_UInt16 publisherIdentifier = PUBLISHER_ID;
    readerConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    readerConfig.publisherId.id.uint16 = publisherIdentifier;
    readerConfig.writerGroupId = WRITER_GROUP_ID;
    readerConfig.dataSetWriterId = DATASET_WRITER_ID;

    UA_DataSetMetaDataType_init(&readerConfig.dataSetMetaData);
    readerConfig.dataSetMetaData.name = UA_STRING("DataSet 1");

    readerConfig.dataSetMetaData.fieldsSize = 1;
    readerConfig.dataSetMetaData.fields = (UA_FieldMetaData *)UA_Array_new(
        readerConfig.dataSetMetaData.fieldsSize, &UA_TYPES[UA_TYPES_FIELDMETADATA]);

    UA_FieldMetaData_init(&readerConfig.dataSetMetaData.fields[0]);
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_INT32].typeId,
                   &readerConfig.dataSetMetaData.fields[0].dataType);
    readerConfig.dataSetMetaData.fields[0].builtInType = UA_NS0ID_INT32;
    readerConfig.dataSetMetaData.fields[0].name = UA_STRING("The answer");
    readerConfig.dataSetMetaData.fields[0].valueRank = -1; /* scalar */

    retval = UA_Server_addDataSetReader(server, readerGroupId, &readerConfig,
                                        &readerIdentifier);
    UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_SERVER, "%s \n",
                UA_StatusCode_name(retval));
    /*Add Subscribed Variable and target variable */
    UA_NodeId folderId;
    UA_String folderName = readerConfig.dataSetMetaData.name;
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_QualifiedName folderBrowseName;
    if(folderName.length > 0) {
        oAttr.displayName.locale = UA_STRING("en-US");
        oAttr.displayName.text = folderName;
        folderBrowseName.namespaceIndex = 1;
        folderBrowseName.name = folderName;
    } else {
        oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Subscribed Variables");
        folderBrowseName = UA_QUALIFIEDNAME(1, "Subscribed Variables");
    }

    retval = UA_Server_addObjectNode(
        server, UA_NODEID_NULL, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), folderBrowseName,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &folderId);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);    
    UA_FieldTargetVariable *targetVars = (UA_FieldTargetVariable *)UA_calloc(
        readerConfig.dataSetMetaData.fieldsSize, sizeof(UA_FieldTargetVariable));
    /* Variable to subscribe data */
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    UA_LocalizedText_copy(&readerConfig.dataSetMetaData.fields->description,
                          &vAttr.description);
    vAttr.displayName.locale = UA_STRING("en-US");
    vAttr.displayName.text = readerConfig.dataSetMetaData.fields->name;
    vAttr.dataType = readerConfig.dataSetMetaData.fields->dataType;

    UA_NodeId newNode;
    retval = UA_Server_addVariableNode(
        server, UA_NODEID_NUMERIC(1, SUBSCRIBEVARIABLE_NODEID), folderId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, (char *)readerConfig.dataSetMetaData.fields->name.data),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vAttr, NULL, &newNode);

    /* For creating Targetvariables */
    UA_FieldTargetDataType_init(&targetVars->targetVariable);
    targetVars->targetVariable.attributeId = UA_ATTRIBUTEID_VALUE;
    targetVars->targetVariable.targetNodeId = newNode;

    retval = UA_Server_DataSetReader_createTargetVariables(
        server, readerIdentifier, readerConfig.dataSetMetaData.fieldsSize, targetVars);
    UA_FieldTargetDataType_clear(&targetVars->targetVariable);

    UA_free(targetVars);
    UA_free(readerConfig.dataSetMetaData.fields);
    return retval;
}

static UA_ClientConfig *
newEncryptedClientConfig(const char *username, const char *password) {
    UA_ByteString *trustList = NULL;
    size_t trustListSize = 0;
    UA_ByteString *revocationList = NULL;
    size_t revocationListSize = 0;

    /* Load certificate and private key */
    UA_ByteString certificate;
    certificate.length = CERT_DER_LENGTH;
    certificate.data = CERT_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_DER_LENGTH;
    privateKey.data = KEY_DER_DATA;

    /* Secure client initialization */

    UA_ClientConfig *cc = (UA_ClientConfig *)UA_calloc(1, sizeof(UA_ClientConfig));
    cc->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;

    UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey, trustList,
                                         trustListSize, revocationList,
                                         revocationListSize);
    cc->securityPolicyUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");

    UA_UserNameIdentityToken* identityToken = UA_UserNameIdentityToken_new();
    identityToken->userName = UA_STRING_ALLOC(username);
    identityToken->password = UA_STRING_ALLOC(password);
    UA_ExtensionObject_clear(&cc->userIdentityToken);
    cc->userIdentityToken.encoding = UA_EXTENSIONOBJECT_DECODED;
    cc->userIdentityToken.content.decoded.type = &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN];
    cc->userIdentityToken.content.decoded.data = identityToken;

    return cc;
}

UA_StatusCode sksPullStatus = UA_STATUSCODE_BAD;

static void
sksPullRequestCallback_publisher(UA_Server *server, UA_StatusCode sksPullRequestStatus, void *data) {
    sksPullStatus = sksPullRequestStatus;
    UA_Server_setWriterGroupActivateKey(server, writerGroupId);
}

static void
sksPullRequestCallback_subscriber(UA_Server *server, UA_StatusCode sksPullRequestStatus, void *data) {
    sksPullStatus = sksPullRequestStatus;
    UA_Server_setReaderGroupActivateKey(server, readerGroupId);
}

static void
sksPullRequestCallback_pubsub(UA_Server *server, UA_StatusCode sksPullRequestStatus, void *data) {
    sksPullStatus = sksPullRequestStatus;
    UA_Server_setWriterGroupActivateKey(server, writerGroupId);
    UA_Server_setReaderGroupActivateKey(server, readerGroupId);
}

START_TEST(AddValidSksClientwithWriterGroup) {
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    int retryCnt = 0;
    UA_ClientConfig *config = newEncryptedClientConfig("user1", "password");
    retval = addPublisher(publisherApp);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD,
                  "Expected Statuscode to be Good, but failed with: %s ",
                  UA_StatusCode_name(retval));

    retval = UA_Server_enableWriterGroup(publisherApp, writerGroupId);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD,
                  "Expected Statuscode to be Good, but failed with: %s ",
                  UA_StatusCode_name(retval));

    retval = UA_Server_setSksClient(publisherApp, securityGroupId, config, testingSKSEndpointUrl, sksPullRequestCallback_publisher, NULL);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD,
                  "Expected Statuscode to be Good, but failed with: %s ",
                  UA_StatusCode_name(retval));
    while(UA_StatusCode_isBad(sksPullStatus) && (retryCnt++ < MAX_RETRIES)) {
        UA_Server_run_iterate(publisherApp, true);
        UA_fakeSleep(50);
    }
    ck_assert_msg(sksPullStatus == UA_STATUSCODE_GOOD,
                  "Expected Statuscode to be Good, but failed with: %s (%u retries)",
                  UA_StatusCode_name(sksPullStatus), retryCnt);
    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(publisherApp, writerGroupId);
    ck_assert(wg != NULL);
    
    ck_assert(wg->keyStorage->keyListSize > 0);
    UA_LOCK(&sksServer->serviceMutex);
    UA_PubSubKeyListItem *sksKsItr =
        UA_PubSubKeyStorage_findKeyStorage(sksServer, securityGroupId)
            ->currentItem;
    UA_UNLOCK(&sksServer->serviceMutex);
    UA_PubSubKeyListItem *wgKsItr = TAILQ_FIRST(&wg->keyStorage->keyList);
    for(size_t i = 0; i < wg->keyStorage->keyListSize; i++) {
        ck_assert_msg(UA_ByteString_equal(&sksKsItr->key, &wgKsItr->key) == UA_TRUE,
                      "Expected the Current Key and future keys in the SKS to be equal "
                      "to the Current and future keys in Publisher");
        ck_assert_uint_eq(sksKsItr->keyID, wgKsItr->keyID);
        sksKsItr = TAILQ_NEXT(sksKsItr, keyListEntry);
        wgKsItr = TAILQ_NEXT(wgKsItr, keyListEntry);
    }
    UA_free(config);
}
END_TEST

START_TEST(AddValidSksClientwithReaderGroup) {
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    UA_ClientConfig *config = newEncryptedClientConfig("user1", "password");
    int retryCnt = 0;
    retval = addSubscriber(subscriberApp);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD,
                  "Expected Statuscode to be Good but failed with: %s ",
                  UA_StatusCode_name(retval));
    retval = UA_Server_setSksClient(subscriberApp, securityGroupId, config, testingSKSEndpointUrl, sksPullRequestCallback_subscriber, NULL);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD,
                  "Expected Statuscode to be Good, but failed with: %s ",
                  UA_StatusCode_name(retval));
    sksPullStatus = UA_STATUSCODE_BAD;
    while(UA_StatusCode_isBad(sksPullStatus) && (retryCnt++ < MAX_RETRIES)) {
        UA_Server_run_iterate(subscriberApp, true);
        UA_fakeSleep(50);
    }
    ck_assert_msg(sksPullStatus == UA_STATUSCODE_GOOD,
                  "Expected Statuscode to be Good, but failed with: %s (%u retries)",
                  UA_StatusCode_name(sksPullStatus), retryCnt);
    UA_ReaderGroup *rg = UA_ReaderGroup_findRGbyId(subscriberApp, readerGroupId);
    ck_assert(rg != NULL);

    retval = UA_Server_enableReaderGroup(subscriberApp, writerGroupId);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD,
                  "Expected Statuscode to be Good, but failed with: %s ",
                  UA_StatusCode_name(retval));
    ck_assert(rg->keyStorage->keyListSize > 0);
    UA_LOCK(&sksServer->serviceMutex);
    UA_PubSubKeyListItem *sksKsItr =
        UA_PubSubKeyStorage_findKeyStorage(sksServer, securityGroupId)
            ->currentItem;
    UA_UNLOCK(&sksServer->serviceMutex);
    UA_PubSubKeyListItem *rgKsItr = TAILQ_FIRST(&rg->keyStorage->keyList);
    for(size_t i = 0; i < rg->keyStorage->keyListSize; i++) {
        ck_assert_msg(UA_ByteString_equal(&sksKsItr->key, &rgKsItr->key) == UA_TRUE,
                      "Expected the Current Key and future keys in the SKS to be equal "
                      "to the Current and future keys in Publisher");
        ck_assert_uint_eq(sksKsItr->keyID, rgKsItr->keyID);
        sksKsItr = TAILQ_NEXT(sksKsItr, keyListEntry);
        rgKsItr = TAILQ_NEXT(rgKsItr, keyListEntry);
    }
    ck_assert(rg->keyStorage->keyListSize > 0);
    UA_free(config);
}
END_TEST

START_TEST(SetInvalidSKSClient) {
    addPublisher(publisherApp);
    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *config = UA_Client_getConfig(client);
    int retryCnt = 0;
    UA_Server_setSksClient(publisherApp, securityGroupId, config, testingSKSEndpointUrl, sksPullRequestCallback_pubsub, NULL);
    sksPullStatus = UA_STATUSCODE_GOOD;
    while(UA_StatusCode_isGood(sksPullStatus) && (retryCnt++ < MAX_RETRIES)) {
        UA_Server_run_iterate(publisherApp, true);
        UA_fakeSleep(50);
    }
    ck_assert_msg(sksPullStatus != UA_STATUSCODE_GOOD,
                  "Expected Statuscode not to be GOOD: (%u retries)", retryCnt);
    UA_Client_delete(client);
}
END_TEST

START_TEST(SetInvalidSKSEndpointUrl) {
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    retval = addPublisher(publisherApp);
    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *config = UA_Client_getConfig(client);
    retval = UA_Server_setSksClient(publisherApp, securityGroupId, config, "opc.tcp:[invalid:host]:4840", sksPullRequestCallback_publisher, NULL);
    ck_assert_msg(retval == UA_STATUSCODE_BADTCPENDPOINTURLINVALID,
                  "Expected Statuscode to be BADTCPENDPOINTURLINVALID, but failed with: %s ",
                  UA_StatusCode_name(retval));
    UA_Client_delete(client);
}
END_TEST

START_TEST(SetWrongSKSEndpointUrl) {
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    retval = addPublisher(publisherApp);
    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *config = UA_Client_getConfig(client);
    retval = UA_Server_setSksClient(publisherApp, securityGroupId, config, "opc.tcp://WrongHost:4840", sksPullRequestCallback_publisher, NULL);
    ck_assert_msg(retval == UA_STATUSCODE_BADCONNECTIONCLOSED,
                  "Expected Statuscode to be BADCONNECTIONCLOSED, but failed with: %s ",
                  UA_StatusCode_name(retval));
    UA_Client_delete(client);
}
END_TEST

START_TEST(CheckPublishedValuesInUserLand) {
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    int retryCnt = 0;
    retval = addPublisher(publisherApp);
    ck_assert(retval == UA_STATUSCODE_GOOD);
    retval = addSubscriber(subscriberApp);
    ck_assert(retval == UA_STATUSCODE_GOOD);

    retval = UA_Server_enableWriterGroup(publisherApp, writerGroupId);
    ck_assert(retval == UA_STATUSCODE_GOOD);
    retval = UA_Server_enableReaderGroup(subscriberApp, readerGroupId);
    ck_assert(retval == UA_STATUSCODE_GOOD);

    UA_ClientConfig *pubSksClientConfig = newEncryptedClientConfig("user1", "password");
    retval =
        UA_Server_setSksClient(publisherApp, securityGroupId, pubSksClientConfig, testingSKSEndpointUrl, sksPullRequestCallback_publisher, NULL);
    ck_assert(retval == UA_STATUSCODE_GOOD);
    sksPullStatus = UA_STATUSCODE_BAD;
    while(UA_StatusCode_isBad(sksPullStatus) && (retryCnt++ < MAX_RETRIES)) {
        UA_Server_run_iterate(publisherApp, true);
        UA_fakeSleep(50);
    }
    ck_assert_msg(sksPullStatus == UA_STATUSCODE_GOOD,
                  "Expected Statuscode to be Good, but failed with: %s (%u retries)",
                  UA_StatusCode_name(sksPullStatus), retryCnt);

    UA_ClientConfig *subSksClientConfig = newEncryptedClientConfig("user1", "password");
    retval =
        UA_Server_setSksClient(subscriberApp, securityGroupId, subSksClientConfig, testingSKSEndpointUrl, sksPullRequestCallback_subscriber, NULL);
    ck_assert(retval == UA_STATUSCODE_GOOD);
    sksPullStatus = UA_STATUSCODE_BAD;
    retryCnt = 0;
    while(UA_StatusCode_isBad(sksPullStatus) && (retryCnt++ < MAX_RETRIES)) {
        UA_Server_run_iterate(subscriberApp, true);
        UA_fakeSleep(50);
    }
    ck_assert(retryCnt < MAX_RETRIES);

    /* run server - publisher and subscriber */
    UA_fakeSleep(100 + 1);
    UA_Server_run_iterate(publisherApp, true);
    UA_Server_run_iterate(subscriberApp, true);
    UA_fakeSleep(100 + 1);
    UA_Server_run_iterate(publisherApp, true);
    UA_Server_run_iterate(subscriberApp, true);

    UA_Variant *publishedNodeData = UA_Variant_new();
    retval = UA_Server_readValue(publisherApp,
                                 UA_NODEID_NUMERIC(1, PUBLISHVARIABLE_NODEID),
                                 publishedNodeData);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    while(true) {
        UA_Variant *subscribedNodeData = UA_Variant_new();
        retval = UA_Server_readValue(subscriberApp,
                                     UA_NODEID_NUMERIC(1, SUBSCRIBEVARIABLE_NODEID),
                                     subscribedNodeData);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
        UA_Boolean isEqual = (UA_order(publishedNodeData->data, subscribedNodeData->data,
                                       publishedNodeData->type) == UA_ORDER_EQ);
        UA_Variant_delete(subscribedNodeData);
        if(isEqual)
            break;
        UA_Server_run_iterate(publisherApp, false);
        UA_Server_run_iterate(subscriberApp, false);
        UA_fakeSleep(50);
    }
    UA_Variant_delete(publishedNodeData);
    UA_free(pubSksClientConfig);
    UA_free(subSksClientConfig);
}
END_TEST

START_TEST(PublisherSubscriberTogethor) {
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    int retryCnt = 0;
    retval = addSubscriber(publisherApp);
    ck_assert(retval == UA_STATUSCODE_GOOD);
     retval = addPublisher(publisherApp);
    ck_assert(retval == UA_STATUSCODE_GOOD);

    retval = UA_Server_enableWriterGroup(publisherApp, writerGroupId);
    ck_assert(retval == UA_STATUSCODE_GOOD);
    retval = UA_Server_enableReaderGroup(publisherApp, readerGroupId);
    ck_assert(retval == UA_STATUSCODE_GOOD);
    
    UA_ClientConfig *pubSksClientConfig = newEncryptedClientConfig("user1", "password");
    retval =
        UA_Server_setSksClient(publisherApp, securityGroupId, pubSksClientConfig, testingSKSEndpointUrl, sksPullRequestCallback_pubsub, NULL);
    ck_assert(retval == UA_STATUSCODE_GOOD);

    sksPullStatus = UA_STATUSCODE_BAD;
    while(UA_StatusCode_isBad(sksPullStatus) && (retryCnt++ < MAX_RETRIES)) {
        UA_Server_run_iterate(publisherApp, true);
        UA_fakeSleep(50);
    }
    ck_assert_msg(sksPullStatus == UA_STATUSCODE_GOOD,
                  "Expected Statuscode to be Good, but failed with: %s (%u retries)",
                  UA_StatusCode_name(sksPullStatus), retryCnt);
    retval = UA_Server_setWriterGroupOperational(publisherApp, writerGroupId);
    ck_assert(retval == UA_STATUSCODE_GOOD);
    retval = UA_Server_setReaderGroupOperational(publisherApp, readerGroupId);
    ck_assert(retval == UA_STATUSCODE_GOOD);

    /* run server - publisher and subscriber */
    UA_fakeSleep(100 + 1);
    UA_Server_run_iterate(publisherApp, true);
    UA_fakeSleep(100 + 1);
    UA_Server_run_iterate(publisherApp, true);
    
    UA_Variant *publishedNodeData = UA_Variant_new();
    retval = UA_Server_readValue(publisherApp,
                                 UA_NODEID_NUMERIC(1, PUBLISHVARIABLE_NODEID),
                                 publishedNodeData);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    while(true) {
        UA_Variant *subscribedNodeData = UA_Variant_new();
        retval = UA_Server_readValue(publisherApp,
                                     UA_NODEID_NUMERIC(1, SUBSCRIBEVARIABLE_NODEID),
                                     subscribedNodeData);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
        UA_Boolean isEqual = (UA_order(publishedNodeData->data, subscribedNodeData->data,
                                       publishedNodeData->type) == UA_ORDER_EQ);
        UA_Variant_delete(subscribedNodeData);
        if(isEqual)
            break;
        UA_Server_run_iterate(publisherApp, false);
        UA_Server_run_iterate(subscriberApp, false);
        UA_fakeSleep(50);
    }

    UA_Variant_delete(publishedNodeData);
    UA_free(pubSksClientConfig);
}
END_TEST

START_TEST(PublisherDelayedSubscriberTogethor) {
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    int retryCnt = 0;

    retval = addPublisher(publisherApp);
    ck_assert(retval == UA_STATUSCODE_GOOD);

    UA_ClientConfig *pubSksClientConfig = newEncryptedClientConfig("user1", "password");

    retval = addSubscriber(publisherApp);
    ck_assert(retval == UA_STATUSCODE_GOOD);

    retval = UA_Server_enableWriterGroup(publisherApp, writerGroupId);
    ck_assert(retval == UA_STATUSCODE_GOOD);

    retval = UA_Server_enableReaderGroup(publisherApp, readerGroupId);
    ck_assert(retval == UA_STATUSCODE_GOOD);

    retval =
        UA_Server_setSksClient(publisherApp, securityGroupId, pubSksClientConfig, testingSKSEndpointUrl, sksPullRequestCallback_pubsub, NULL);
    ck_assert(retval == UA_STATUSCODE_GOOD);

    sksPullStatus = UA_STATUSCODE_BAD;
    while(UA_StatusCode_isBad(sksPullStatus) && (retryCnt++ < MAX_RETRIES)) {
        UA_Server_run_iterate(publisherApp, true);
        UA_fakeSleep(50);
    }
    ck_assert_msg(sksPullStatus == UA_STATUSCODE_GOOD,
                  "Expected Statuscode to be Good, but failed with: %s (%u retries)",
                  UA_StatusCode_name(sksPullStatus), retryCnt);

    /* run server - publisher and subscriber */
    UA_fakeSleep(100 + 1);
    UA_Server_run_iterate(publisherApp, true);
    UA_fakeSleep(100 + 1);
    UA_Server_run_iterate(publisherApp, true);

    UA_ReaderGroup *rg = UA_ReaderGroup_findRGbyId(publisherApp, readerGroupId);
    ck_assert(rg->securityPolicyContext != NULL);
    UA_Variant *publishedNodeData = UA_Variant_new();
    retval = UA_Server_readValue(
        publisherApp, UA_NODEID_NUMERIC(1, PUBLISHVARIABLE_NODEID), publishedNodeData);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    while(true) {
        UA_Variant *subscribedNodeData = UA_Variant_new();
        retval = UA_Server_readValue(publisherApp,
                                     UA_NODEID_NUMERIC(1, SUBSCRIBEVARIABLE_NODEID),
                                     subscribedNodeData);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
        UA_Boolean isEqual = (UA_order(publishedNodeData->data, subscribedNodeData->data,
                                       publishedNodeData->type) == UA_ORDER_EQ);
        UA_Variant_delete(subscribedNodeData);
        if(isEqual)
            break;
        UA_Server_run_iterate(publisherApp, false);
        UA_Server_run_iterate(subscriberApp, false);
        UA_fakeSleep(50);
    }

    UA_Variant_delete(publishedNodeData);
    UA_free(pubSksClientConfig);
}
END_TEST

START_TEST(FetchNextbatchOfKeys) {
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    int retryCnt = 0;

    retval = addPublisher(publisherApp);
    ck_assert(retval == UA_STATUSCODE_GOOD);

    UA_ClientConfig *pubSksClientConfig = newEncryptedClientConfig("user1", "password");
    retval =
        UA_Server_setSksClient(publisherApp, securityGroupId, pubSksClientConfig, testingSKSEndpointUrl, sksPullRequestCallback_publisher, NULL);
    ck_assert(retval == UA_STATUSCODE_GOOD);

    sksPullStatus = UA_STATUSCODE_BAD;
    while(UA_StatusCode_isBad(sksPullStatus) && (retryCnt++ < MAX_RETRIES)) {
        UA_Server_run_iterate(publisherApp, true);
        UA_fakeSleep(50);
    }
    ck_assert_msg(sksPullStatus == UA_STATUSCODE_GOOD,
                  "Expected Statuscode to be Good, but failed with: %s (%u retries)",
                  UA_StatusCode_name(sksPullStatus), retryCnt);

    retval = addSubscriber(subscriberApp);
    ck_assert(retval == UA_STATUSCODE_GOOD);

    retval = UA_Server_enableWriterGroup(publisherApp, writerGroupId);
    ck_assert(retval == UA_STATUSCODE_GOOD);

    retval = UA_Server_enableReaderGroup(subscriberApp, readerGroupId);
    ck_assert(retval == UA_STATUSCODE_GOOD);

    UA_ClientConfig *subSksClientConfig = newEncryptedClientConfig("user1", "password");

    retval =
        UA_Server_setSksClient(subscriberApp, securityGroupId, subSksClientConfig, testingSKSEndpointUrl, sksPullRequestCallback_subscriber, NULL);
    ck_assert(retval == UA_STATUSCODE_GOOD);

    sksPullStatus = UA_STATUSCODE_BAD;
    retryCnt = 0;
    while(UA_StatusCode_isBad(sksPullStatus) && (retryCnt++ < MAX_RETRIES)) {
        UA_Server_run_iterate(subscriberApp, true);
        UA_fakeSleep(50);
    }
    ck_assert(retryCnt < MAX_RETRIES);

    UA_LOCK(&publisherApp->serviceMutex);
    UA_PubSubKeyStorage *pubKs = UA_PubSubKeyStorage_findKeyStorage(
        publisherApp, securityGroupId);
    UA_UNLOCK(&publisherApp->serviceMutex);
    UA_LOCK(&subscriberApp->serviceMutex);
    UA_PubSubKeyStorage *subKs = UA_PubSubKeyStorage_findKeyStorage(
        subscriberApp, securityGroupId);
    UA_UNLOCK(&subscriberApp->serviceMutex);

    sksPullStatus = UA_STATUSCODE_BAD;
    UA_UInt16 sksPullIteration = 0;
    while(true) {
        UA_Server_run_iterate(subscriberApp, true);
        UA_Server_run_iterate(publisherApp, true);
        /* we make sure to iterate for few sks pull cycles */
        if(UA_StatusCode_isGood(sksPullStatus)) {
            sksPullStatus = UA_STATUSCODE_BAD;
            ++sksPullIteration;
        }
        /* we need to run extra iterations for key rollover callbacks */
        if(sksPullIteration > 10 &&
           subKs->currentItem->keyID == pubKs->currentItem->keyID)
            break;
        UA_fakeSleep(50);
    }
    ck_assert(subKs->currentItem->keyID == pubKs->currentItem->keyID);
    ck_assert(UA_ByteString_equal(&subKs->currentItem->key, &pubKs->currentItem->key));
    UA_Variant *publishedNodeData = UA_Variant_new();
    retval = UA_Server_readValue(
        publisherApp, UA_NODEID_NUMERIC(1, PUBLISHVARIABLE_NODEID), publishedNodeData);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant *subscribedNodeData = UA_Variant_new();
    retval =
        UA_Server_readValue(subscriberApp, UA_NODEID_NUMERIC(1, SUBSCRIBEVARIABLE_NODEID),
                            subscribedNodeData);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(*(UA_Int32 *)publishedNodeData->data,
                     *(UA_Int32 *)subscribedNodeData->data);
    UA_Variant_delete(subscribedNodeData);
    UA_Variant_delete(publishedNodeData);
    UA_free(pubSksClientConfig);
    UA_free(subSksClientConfig);
}
END_TEST

int
main(void) {
    int number_failed = 0;
    TCase *tc_pubsub_sks_client = tcase_create("PubSub SKS Client");
    tcase_add_checked_fixture(tc_pubsub_sks_client, skssetup, sksteardown);
    tcase_add_checked_fixture(tc_pubsub_sks_client, publishersetup, publisherteardown);
    tcase_add_checked_fixture(tc_pubsub_sks_client, subscribersetup, subscriberteardown);
    tcase_add_test(tc_pubsub_sks_client, AddValidSksClientwithWriterGroup);
    tcase_add_test(tc_pubsub_sks_client, AddValidSksClientwithReaderGroup);
    tcase_add_test(tc_pubsub_sks_client, SetInvalidSKSClient);
    tcase_add_test(tc_pubsub_sks_client, SetInvalidSKSEndpointUrl);
    tcase_add_test(tc_pubsub_sks_client, SetWrongSKSEndpointUrl);
    tcase_add_test(tc_pubsub_sks_client, CheckPublishedValuesInUserLand);
    tcase_add_test(tc_pubsub_sks_client, PublisherSubscriberTogethor);
    tcase_add_test(tc_pubsub_sks_client, PublisherDelayedSubscriberTogethor);
    tcase_add_test(tc_pubsub_sks_client, FetchNextbatchOfKeys);

    Suite *s = suite_create("PubSub SKS Client");
    suite_add_tcase(s, tc_pubsub_sks_client);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
