/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 *
 */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/plugin/certificategroup_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "client/ua_client_internal.h"
#include "ua_server_internal.h"

#include <stdio.h>
#include <stdlib.h>

#include "test_helpers.h"
#include "certificates.h"
#include "check.h"
#include "thread_wrapper.h"

#if defined(__linux__) || defined(UA_ARCHITECTURE_WIN32)
#include "mp_printf.h"
#define TEST_PATH_MAX 256
#endif /* defined(__linux__) || defined(UA_ARCHITECTURE_WIN32) */

UA_Server *server;
UA_Boolean running;
THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
        while(running)
        UA_Server_run_iterate(server, true);
        return 0;
}

static void setup(void) {
    running = true;

    /* Load certificate and private key */
    UA_ByteString certificate;
    certificate.length = CERT_DER_LENGTH;
    certificate.data = CERT_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_DER_LENGTH;
    privateKey.data = KEY_DER_DATA;

    server = UA_Server_newForUnitTestWithSecurityPolicies(4840, &certificate, &privateKey,
                                                          &certificate, 1,
                                                          NULL, 0,
                                                          NULL, 0);
    ck_assert(server != NULL);
    UA_ServerConfig *config = UA_Server_getConfig(server);

    /* Set the ApplicationUri used in the certificate */
    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
            UA_STRING_ALLOC("urn:unconfigured:application");

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}
#if defined(__linux__) || defined(UA_ARCHITECTURE_WIN32)
static void setup2(void) {
    running = true;

    /* Load certificate and private key */
    UA_ByteString certificate;
    certificate.length = CERT_DER_LENGTH;
    certificate.data = CERT_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_DER_LENGTH;
    privateKey.data = KEY_DER_DATA;

    server = UA_Server_newForUnitTestWithSecurityPolicies(4840, &certificate, &privateKey,
                                                          NULL, 0,
                                                          NULL, 0,
                                                          NULL, 0);

    ck_assert(server != NULL);
    UA_ServerConfig *config = UA_Server_getConfig(server);

    char storePathDir[TEST_PATH_MAX];
    getcwd(storePathDir, TEST_PATH_MAX - 4);
    mp_snprintf(storePathDir, TEST_PATH_MAX, "%s/pki", storePathDir);

    const UA_String storePath = UA_STRING(storePathDir);

    UA_ServerConfig_setDefaultWithFilestore(config, 4840, &certificate, &privateKey, storePath);
    config->eventLoop->dateTime_now= UA_DateTime_now_fake;
    config->eventLoop->dateTime_nowMonotonic = UA_DateTime_now_fake;
    config->tcpReuseAddr = true;

    /* Set the ApplicationUri used in the certificate */
    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
            UA_STRING_ALLOC("urn:unconfigured:application");

    /* Clear old certificates */
    UA_ByteString empty[2] = {0};
    UA_NodeId defaultApplicationGroup = UA_NODEID_NUMERIC(
        0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    UA_StatusCode retval = UA_Server_addCertificates(server, defaultApplicationGroup,
                                                     empty, 0, empty, 0, true, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = UA_Server_addCertificates(server, defaultApplicationGroup, empty, 0, empty,
                                       0, false, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}
#endif /* defined(__linux__) || defined(UA_ARCHITECTURE_WIN32) */

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(set_trustlist) {

    UA_ServerConfig *config = UA_Server_getConfig(server);

    /* Load certificate and private key */
    UA_ByteString trustedCertificate;
    trustedCertificate.length = CERT_DER_LENGTH;
    trustedCertificate.data = CERT_DER_DATA;

    UA_ByteString issuerCertificate;
    issuerCertificate.length = CERT_PEM_LENGTH;
    issuerCertificate.data = CERT_PEM_DATA;

    /* Add the specified list to the default application group */
    UA_TrustListDataType trustListTmp;
    memset(&trustListTmp, 0, sizeof(UA_TrustListDataType));

    trustListTmp.specifiedLists = (UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES | UA_TRUSTLISTMASKS_ISSUERCERTIFICATES);
    trustListTmp.trustedCertificates = &trustedCertificate;
    trustListTmp.trustedCertificatesSize = 1;
    trustListTmp.issuerCertificates = &issuerCertificate;
    trustListTmp.issuerCertificatesSize = 1;

    UA_StatusCode retval = config->secureChannelPKI.setTrustList(&config->secureChannelPKI, &trustListTmp);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = config->sessionPKI.setTrustList(&config->sessionPKI, &trustListTmp);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

/* Add 2 certificates to reject list. 
 * Check if rejectListSize == 2
 * remove 1 certificate and check size 
 * remove 2 certificate and check size
 */
START_TEST(set_rejectlist_remove_from_rejectlist) {

    UA_ServerConfig *config = UA_Server_getConfig(server);

    /* Load rejected certificates */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ByteString *rejectedList = NULL;
    size_t rejectedListSize = 0;
    UA_ByteString certificate1;
    certificate1.length = CERT_DER_LENGTH;
    certificate1.data = CERT_DER_DATA;
    UA_ByteString certificate2;
    certificate2.length = APPLICATION_CERT_DER_DATA_WITH_EMAIL_LENGTH;
    certificate2.data = APPLICATION_CERT_DER_DATA_WITH_EMAIL;

    /* add certificate1 to the rejectedList*/
    retval = UA_Array_appendCopy((void**)&rejectedList, &rejectedListSize, &certificate1, &UA_TYPES[UA_TYPES_BYTESTRING]);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rejectedListSize, 1);

    /* add certificate2 to the rejectedList*/
    retval = UA_Array_appendCopy((void**)&rejectedList, &rejectedListSize, &certificate2, &UA_TYPES[UA_TYPES_BYTESTRING]);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rejectedListSize, 2);

    /* set rejected list */
    retval = config->secureChannelPKI.setRejectedList(&config->secureChannelPKI, rejectedList, rejectedListSize);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Array_delete(rejectedList, rejectedListSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
    rejectedList = NULL;
    rejectedListSize = 0;

    /* get reject list and reuse rejectedList and rejectedListSize */
    retval = config->secureChannelPKI.getRejectedList(&config->secureChannelPKI, &rejectedList, &rejectedListSize);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rejectedListSize, 2);
    /* Check existence but omit the order */
    UA_Boolean cert1exist = false;
    UA_Boolean cert2exist = false;
    for(unsigned i = 0; i < rejectedListSize; ++i) {
        if(UA_ByteString_equal(&rejectedList[i], &certificate1))
            cert1exist = true;
        if(UA_ByteString_equal(&rejectedList[i], &certificate2))
            cert2exist = true;
    }
    ck_assert(cert1exist);
    ck_assert(cert2exist);

    // free rejectedList
    UA_Array_delete(rejectedList, rejectedListSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
    rejectedList = NULL;
    rejectedListSize = 0;

    /* remove certificate1 from rejected list */
    retval = config->secureChannelPKI.removeFromRejectedList(&config->secureChannelPKI, &certificate1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* remove certificate1 and verify certificate2 remains */
    retval = config->secureChannelPKI.getRejectedList(&config->secureChannelPKI, &rejectedList, &rejectedListSize);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rejectedListSize, 1);
    ck_assert(UA_ByteString_equal(&rejectedList[0], &certificate2));

    UA_Array_delete(rejectedList, rejectedListSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
    rejectedList = NULL;
    rejectedListSize = 0;

    /* remove certificate2 from rejected list */
    retval = config->secureChannelPKI.removeFromRejectedList(&config->secureChannelPKI, &certificate2);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* check if it is indeed removed */
    retval = config->secureChannelPKI.getRejectedList(&config->secureChannelPKI, &rejectedList, &rejectedListSize);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rejectedListSize, 0);

    UA_Array_delete(rejectedList, rejectedListSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
    rejectedList = NULL;
    rejectedListSize = 0;
}
END_TEST

/* Try to add empty reject list */
START_TEST(set_rejectlist_null) {

    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ByteString *rejectedList = NULL;
    size_t rejectedListSize = 0;

    /* Load rejected certificates */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* set rejected list */
    retval = config->secureChannelPKI.setRejectedList(&config->secureChannelPKI, NULL, 0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* get reject list and reuse rejectedList and rejectedListSize */
    retval = config->secureChannelPKI.getRejectedList(&config->secureChannelPKI, &rejectedList, &rejectedListSize);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rejectedListSize, 0);
}
END_TEST

/* Try to remove certificate which is not in the list */
START_TEST(set_rejectlist_remove_non_existent_cert_from_rejectlist) {

    UA_ServerConfig *config = UA_Server_getConfig(server);

    /* Load rejected certificates */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ByteString *rejectedList = NULL;
    size_t rejectedListSize = 0;
    UA_ByteString certificate;
    certificate.length = CERT_DER_LENGTH;
    certificate.data = CERT_DER_DATA;

    UA_ByteString certificate_to_remove;
    certificate_to_remove.length = APPLICATION_CERT_DER_DATA_WITH_EMAIL_LENGTH;
    certificate_to_remove.data = APPLICATION_CERT_DER_DATA_WITH_EMAIL;

    /* add certificate to the rejectedList*/
    retval = UA_Array_appendCopy((void**)&rejectedList, &rejectedListSize, &certificate, &UA_TYPES[UA_TYPES_BYTESTRING]);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rejectedListSize, 1);

    /* set rejected list */
    retval = config->secureChannelPKI.setRejectedList(&config->secureChannelPKI, rejectedList, rejectedListSize);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Array_delete(rejectedList, rejectedListSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
    rejectedList = NULL;
    rejectedListSize = 0;

    /* get reject list and reuse rejectedList and rejectedListSize */
    retval = config->secureChannelPKI.getRejectedList(&config->secureChannelPKI, &rejectedList, &rejectedListSize);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rejectedListSize, 1);
    // free rejectedList
    UA_Array_delete(rejectedList, rejectedListSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
    rejectedList = NULL;
    rejectedListSize = 0;

    /* remove non existent certificate from rejected list so it doesn't influence on following tests */
    retval = config->secureChannelPKI.removeFromRejectedList(&config->secureChannelPKI, &certificate_to_remove);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* check that added certificate is not removed */
    retval = config->secureChannelPKI.getRejectedList(&config->secureChannelPKI, &rejectedList, &rejectedListSize);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rejectedListSize, 1);
    UA_Array_delete(rejectedList, rejectedListSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
    rejectedList = NULL;
    rejectedListSize = 0;

    /* remove existing certificate from rejected list so it doesn't influence on following tests */
    retval = config->secureChannelPKI.removeFromRejectedList(&config->secureChannelPKI, &certificate);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

/* Check rejectlist persistance between server restarts */
START_TEST(rejectedlist_filestore_persistence) {
    UA_ServerConfig *config = UA_Server_getConfig(server);

    UA_ByteString *rejectedList = NULL;
    size_t rejectedListSize = 0;

    UA_ByteString certificate1;
    certificate1.length = CERT_DER_LENGTH;
    certificate1.data = CERT_DER_DATA;
    UA_ByteString certificate2;
    certificate2.length = APPLICATION_CERT_DER_DATA_WITH_EMAIL_LENGTH;
    certificate2.data = APPLICATION_CERT_DER_DATA_WITH_EMAIL;

    /* Add certificate 1 */
    UA_StatusCode retval = UA_Array_appendCopy((void**)&rejectedList, &rejectedListSize,
                                                &certificate1, &UA_TYPES[UA_TYPES_BYTESTRING]);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Add certificate 2 */
    retval = UA_Array_appendCopy((void**)&rejectedList, &rejectedListSize,
                                 &certificate2, &UA_TYPES[UA_TYPES_BYTESTRING]);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* set rejected list */
    retval = config->secureChannelPKI.setRejectedList(&config->secureChannelPKI,
                                                      rejectedList,
                                                      rejectedListSize);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rejectedListSize, 2);

    UA_Array_delete(rejectedList, rejectedListSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
    rejectedList = NULL;
    rejectedListSize = 0;

    /* Recreate the server with the same pki filestore path. */
    teardown();
    setup2();

    /* Get rejected list and check if previously added certificates are still present */
    config = UA_Server_getConfig(server);
    retval = config->secureChannelPKI.getRejectedList(&config->secureChannelPKI,
                                                      &rejectedList,
                                                      &rejectedListSize);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rejectedListSize, 2);
    /* Check existence but omit the order */
    UA_Boolean cert1exist = false;
    UA_Boolean cert2exist = false;
    for(unsigned i = 0; i < rejectedListSize; ++i) {
        if(UA_ByteString_equal(&rejectedList[i], &certificate1))
            cert1exist = true;
        if(UA_ByteString_equal(&rejectedList[i], &certificate2))
            cert2exist = true;
    }
    ck_assert(cert1exist);
    ck_assert(cert2exist);

    UA_Array_delete(rejectedList, rejectedListSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
    rejectedList = NULL;
    rejectedListSize = 0;

    /* remove certificate1 from rejected list */
    retval = config->secureChannelPKI.removeFromRejectedList(&config->secureChannelPKI, &certificate1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* remove certificate1 and verify certificate2 remains */
    retval = config->secureChannelPKI.getRejectedList(&config->secureChannelPKI, &rejectedList, &rejectedListSize);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rejectedListSize, 1);
    ck_assert(UA_ByteString_equal(&rejectedList[0], &certificate2));

    UA_Array_delete(rejectedList, rejectedListSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
    rejectedList = NULL;
    rejectedListSize = 0;

    /* remove certificate2 from rejected list */
    retval = config->secureChannelPKI.removeFromRejectedList(&config->secureChannelPKI, &certificate2);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* check if it is indeed removed */
    retval = config->secureChannelPKI.getRejectedList(&config->secureChannelPKI, &rejectedList, &rejectedListSize);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rejectedListSize, 0);

    UA_Array_delete(rejectedList, rejectedListSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
    rejectedList = NULL;
    rejectedListSize = 0;
}
END_TEST

/* Add 2 certificates to reject list, confirm that were added. Fetch reject list
and check its content, delete list and fetch again to confirm that it is copied correctly */
START_TEST(get_rejectlist_twice) {
   UA_ServerConfig *config = UA_Server_getConfig(server);

    UA_ByteString *rejectedList = NULL;
    size_t rejectedListSize = 0;

    UA_ByteString certificate1;
    certificate1.length = CERT_DER_LENGTH;
    certificate1.data = CERT_DER_DATA;
    UA_ByteString certificate2;
    certificate2.length = APPLICATION_CERT_DER_DATA_WITH_EMAIL_LENGTH;
    certificate2.data = APPLICATION_CERT_DER_DATA_WITH_EMAIL;

    /* Add certificate 1 */
    UA_StatusCode retval = UA_Array_appendCopy((void**)&rejectedList, &rejectedListSize,
                                                &certificate1, &UA_TYPES[UA_TYPES_BYTESTRING]);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Add certificate 2 */
    retval = UA_Array_appendCopy((void**)&rejectedList, &rejectedListSize,
                                 &certificate2, &UA_TYPES[UA_TYPES_BYTESTRING]);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* set rejected list */
    retval = config->secureChannelPKI.setRejectedList(&config->secureChannelPKI,
                                                      rejectedList,
                                                      rejectedListSize);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rejectedListSize, 2);

    UA_Array_delete(rejectedList, rejectedListSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
    rejectedList = NULL;
    rejectedListSize = 0;

    /* Get rejected list and check if previously added certificates are still present */
    config = UA_Server_getConfig(server);
    retval = config->secureChannelPKI.getRejectedList(&config->secureChannelPKI,
                                                      &rejectedList,
                                                      &rejectedListSize);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rejectedListSize, 2);
    /* Check existence but omit the order */
    UA_Boolean cert1exist = false;
    UA_Boolean cert2exist = false;
    for(unsigned i = 0; i < rejectedListSize; ++i) {
        if(UA_ByteString_equal(&rejectedList[i], &certificate1))
            cert1exist = true;
        if(UA_ByteString_equal(&rejectedList[i], &certificate2))
            cert2exist = true;
    }
    ck_assert(cert1exist);
    ck_assert(cert2exist);

    UA_Array_delete(rejectedList, rejectedListSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
    rejectedList = NULL;
    rejectedListSize = 0;

    /* Get rejected list again to confirm that copy is returned */
    config = UA_Server_getConfig(server);
    retval = config->secureChannelPKI.getRejectedList(&config->secureChannelPKI,
                                                      &rejectedList,
                                                      &rejectedListSize);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rejectedListSize, 2);
    /* Check existence but omit the order */
    cert1exist = false;
    cert2exist = false;
    for(unsigned i = 0; i < rejectedListSize; ++i) {
        if(UA_ByteString_equal(&rejectedList[i], &certificate1))
            cert1exist = true;
        if(UA_ByteString_equal(&rejectedList[i], &certificate2))
            cert2exist = true;
    }
    ck_assert(cert1exist);
    ck_assert(cert2exist);

    UA_Array_delete(rejectedList, rejectedListSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
    rejectedList = NULL;
    rejectedListSize = 0;

    UA_Array_delete(rejectedList, rejectedListSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
    rejectedList = NULL;
    rejectedListSize = 0;

    /* remove certificate1 from rejected list */
    retval = config->secureChannelPKI.removeFromRejectedList(&config->secureChannelPKI, &certificate1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* remove certificate1 and verify certificate2 remains */
    retval = config->secureChannelPKI.getRejectedList(&config->secureChannelPKI, &rejectedList, &rejectedListSize);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rejectedListSize, 1);
    ck_assert(UA_ByteString_equal(&rejectedList[0], &certificate2));

    UA_Array_delete(rejectedList, rejectedListSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
    rejectedList = NULL;
    rejectedListSize = 0;

    /* remove certificate2 from rejected list */
    retval = config->secureChannelPKI.removeFromRejectedList(&config->secureChannelPKI, &certificate2);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* check if it is indeed removed */
    retval = config->secureChannelPKI.getRejectedList(&config->secureChannelPKI, &rejectedList, &rejectedListSize);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rejectedListSize, 0);

    UA_Array_delete(rejectedList, rejectedListSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
    rejectedList = NULL;
    rejectedListSize = 0;
}
END_TEST

START_TEST(add_to_trustlist) {

    UA_ServerConfig *config = UA_Server_getConfig(server);

    /* Load certificate and private key */
    UA_ByteString trustedCertificate;
    trustedCertificate.length = CERT_DER_LENGTH;
    trustedCertificate.data = CERT_DER_DATA;

    UA_ByteString issuerCertificate;
    issuerCertificate.length = CERT_PEM_LENGTH;
    issuerCertificate.data = CERT_PEM_DATA;

    /* Add the specified list to the default application group */
    UA_TrustListDataType trustListTmp;
    memset(&trustListTmp, 0, sizeof(UA_TrustListDataType));

    trustListTmp.specifiedLists = (UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES | UA_TRUSTLISTMASKS_ISSUERCERTIFICATES);
    trustListTmp.trustedCertificates = &trustedCertificate;
    trustListTmp.trustedCertificatesSize = 1;
    trustListTmp.issuerCertificates = &issuerCertificate;
    trustListTmp.issuerCertificatesSize = 1;

    UA_StatusCode retval = config->secureChannelPKI.addToTrustList(&config->secureChannelPKI, &trustListTmp);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = config->sessionPKI.addToTrustList(&config->sessionPKI, &trustListTmp);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(check_cert_common_name) {
    UA_String commonName;
    UA_String expected = UA_STRING("open62541Server@localhost"); // from certificates.h

    /* Load certificate and private key */
    UA_ByteString trustedCertificate;
    trustedCertificate.length = APPLICATION_CERT_DER_DATA_WITH_EMAIL_LENGTH;
    trustedCertificate.data = APPLICATION_CERT_DER_DATA_WITH_EMAIL;

    UA_CertificateUtils_getCertCommonName(&trustedCertificate, &commonName);

    ck_assert_uint_eq(commonName.length, expected.length);
    ck_assert(UA_String_equal(&commonName, &expected));

    UA_String_clear(&commonName);
}
END_TEST

START_TEST(add_to_trustlist_with_email) {

    UA_ServerConfig *config = UA_Server_getConfig(server);

    /* Load certificate and private key */
    UA_ByteString trustedCertificate;
    trustedCertificate.length = APPLICATION_CERT_DER_DATA_WITH_EMAIL_LENGTH;
    trustedCertificate.data = APPLICATION_CERT_DER_DATA_WITH_EMAIL;

    UA_ByteString issuerCertificate;
    issuerCertificate.length = APPLICATION_CERT_DER_DATA_WITH_EMAIL_LENGTH;
    issuerCertificate.data = APPLICATION_CERT_DER_DATA_WITH_EMAIL;

    /* Add the specified list to the default application group */
    UA_TrustListDataType trustListTmp;
    memset(&trustListTmp, 0, sizeof(UA_TrustListDataType));

    trustListTmp.specifiedLists = (UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES | UA_TRUSTLISTMASKS_ISSUERCERTIFICATES);
    trustListTmp.trustedCertificates = &trustedCertificate;
    trustListTmp.trustedCertificatesSize = 1;
    trustListTmp.issuerCertificates = &issuerCertificate;
    trustListTmp.issuerCertificatesSize = 1;

    UA_StatusCode retval = config->secureChannelPKI.addToTrustList(&config->secureChannelPKI, &trustListTmp);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = config->sessionPKI.addToTrustList(&config->sessionPKI, &trustListTmp);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(get_trustlist) {
    UA_ServerConfig *config = UA_Server_getConfig(server);

    /* Load certificate and private key */
    UA_ByteString trustedCertificate;
    trustedCertificate.length = CERT_DER_LENGTH;
    trustedCertificate.data = CERT_DER_DATA;

    UA_ByteString issuerCertificate;
    issuerCertificate.length = CERT_PEM_LENGTH;
    issuerCertificate.data = CERT_PEM_DATA;

    /* Add the specified list to the default application group */
    UA_TrustListDataType trustListTmp;
    memset(&trustListTmp, 0, sizeof(UA_TrustListDataType));

    trustListTmp.specifiedLists = (UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES | UA_TRUSTLISTMASKS_ISSUERCERTIFICATES);
    trustListTmp.trustedCertificates = &trustedCertificate;
    trustListTmp.trustedCertificatesSize = 1;
    trustListTmp.issuerCertificates = &issuerCertificate;
    trustListTmp.issuerCertificatesSize = 1;

    UA_StatusCode retval = config->secureChannelPKI.addToTrustList(&config->secureChannelPKI, &trustListTmp);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = config->sessionPKI.addToTrustList(&config->sessionPKI, &trustListTmp);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_TrustListDataType trustList;
    UA_TrustListDataType_init(&trustList);
    trustList.specifiedLists = UA_TRUSTLISTMASKS_ALL;

    retval = config->secureChannelPKI.getTrustList(&config->secureChannelPKI, &trustList);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(trustList.trustedCertificatesSize, 1);
    ck_assert_uint_eq(trustList.issuerCertificatesSize, 1);
    ck_assert_uint_eq(trustList.trustedCrlsSize, 0);
    ck_assert_uint_eq(trustList.issuerCrlsSize, 0);

    ck_assert(UA_ByteString_equal(&trustedCertificate, &trustList.trustedCertificates[0]));
    ck_assert(UA_ByteString_equal(&issuerCertificate, &trustList.issuerCertificates[0]));

    UA_TrustListDataType_clear(&trustList);
}
END_TEST

START_TEST(remove_from_trustlist) {

    UA_ServerConfig *config = UA_Server_getConfig(server);

    /* Load certificate and private key */
    UA_ByteString trustedCertificate;
    trustedCertificate.length = CERT_DER_LENGTH;
    trustedCertificate.data = CERT_DER_DATA;

    UA_ByteString issuerCertificate;
    issuerCertificate.length = CERT_PEM_LENGTH;
    issuerCertificate.data = CERT_PEM_DATA;

    /* Add the specified list to the default application group */
    UA_TrustListDataType trustListTmp;
    memset(&trustListTmp, 0, sizeof(UA_TrustListDataType));

    trustListTmp.specifiedLists = (UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES | UA_TRUSTLISTMASKS_ISSUERCERTIFICATES);
    trustListTmp.trustedCertificates = &trustedCertificate;
    trustListTmp.trustedCertificatesSize = 1;
    trustListTmp.issuerCertificates = &issuerCertificate;
    trustListTmp.issuerCertificatesSize = 1;

    UA_StatusCode retval = config->secureChannelPKI.setTrustList(&config->secureChannelPKI, &trustListTmp);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = config->sessionPKI.setTrustList(&config->sessionPKI, &trustListTmp);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = config->secureChannelPKI.removeFromTrustList(&config->secureChannelPKI, &trustListTmp);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = config->sessionPKI.removeFromTrustList(&config->sessionPKI, &trustListTmp);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(get_rejectedlist) {
    /* Init client and connect to server */
    UA_Client *client = NULL;
    UA_ByteString *trustList = NULL;
    size_t trustListSize = 0;
    UA_ByteString *revocationList = NULL;
    size_t revocationListSize = 0;

    /* Load certificate and private key.
     * Use a certificate that is not in the trustlist of the server. */
    UA_ByteString certificate;
    certificate.length = APPLICATION_CERT_DER_LENGTH;
    certificate.data = APPLICATION_CERT_DER_DATA;
    ck_assert_uint_ne(certificate.length, 0);

    UA_ByteString privateKey;
    privateKey.length = APPLICATION_KEY_DER_LENGTH;
    privateKey.data = APPLICATION_KEY_DER_DATA;
    ck_assert_uint_ne(privateKey.length, 0);

    /* Secure client initialization */
    client = UA_Client_newForUnitTest();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                         trustList, trustListSize,
                                         revocationList, revocationListSize);
    cc->certificateVerification.clear(&cc->certificateVerification);
    UA_CertificateGroup_AcceptAll(&cc->certificateVerification);
    cc->securityPolicyUri =
            UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
    ck_assert(client != NULL);

    /* Set server config */
    UA_ServerConfig *config = UA_Server_getConfig(server);
    /* Load certificate and private key */
    UA_ByteString issuerCertificate;
    issuerCertificate.length = CERT_DER_LENGTH;
    issuerCertificate.data = CERT_DER_DATA;

    /* Add the specified list to the default application group */
    UA_TrustListDataType trustListTmp;
    memset(&trustListTmp, 0, sizeof(UA_TrustListDataType));

    trustListTmp.specifiedLists = UA_TRUSTLISTMASKS_ISSUERCERTIFICATES;
    trustListTmp.issuerCertificates = &issuerCertificate;
    trustListTmp.issuerCertificatesSize = 1;

    UA_StatusCode retval = config->secureChannelPKI.setTrustList(&config->secureChannelPKI, &trustListTmp);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = config->sessionPKI.setTrustList(&config->sessionPKI, &trustListTmp);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Secure client connect */
    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    UA_ByteString *rejectedList = NULL;
    size_t rejectedListSize = 0;
    retval = config->secureChannelPKI.getRejectedList(&config->secureChannelPKI, &rejectedList, &rejectedListSize);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rejectedListSize, 1);

    UA_Array_delete(rejectedList, rejectedListSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Client_delete(client);
}
END_TEST

START_TEST(verify_expired_certificate_status_depends_on_trust) {
    UA_ServerConfig *config = UA_Server_getConfig(server);

    /* CERT_DER_DATA is expired at the current test date. */
    UA_ByteString expiredCertificate;
    expiredCertificate.length = CERT_DER_LENGTH;
    expiredCertificate.data = CERT_DER_DATA;

    /* First, explicitly trust the certificate and verify that validity period
     * checks are applied after trust has been established. */
    UA_TrustListDataType trustListTmp;
    memset(&trustListTmp, 0, sizeof(UA_TrustListDataType));
    trustListTmp.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
    trustListTmp.trustedCertificates = &expiredCertificate;
    trustListTmp.trustedCertificatesSize = 1;

    UA_StatusCode retval = config->secureChannelPKI.setTrustList(&config->secureChannelPKI, &trustListTmp);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = config->sessionPKI.setTrustList(&config->sessionPKI, &trustListTmp);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = config->secureChannelPKI.verifyCertificate(&config->secureChannelPKI, &expiredCertificate);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADCERTIFICATETIMEINVALID);
    retval = config->sessionPKI.verifyCertificate(&config->sessionPKI, &expiredCertificate);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADCERTIFICATETIMEINVALID);

    /* Then remove trust and verify that the status is no longer dominated by
     * expiration, but by missing trust. */
    retval = config->secureChannelPKI.removeFromTrustList(&config->secureChannelPKI, &trustListTmp);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = config->sessionPKI.removeFromTrustList(&config->sessionPKI, &trustListTmp);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = config->secureChannelPKI.verifyCertificate(&config->secureChannelPKI, &expiredCertificate);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADCERTIFICATEUNTRUSTED);
    retval = config->sessionPKI.verifyCertificate(&config->sessionPKI, &expiredCertificate);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADCERTIFICATEUNTRUSTED);
}
END_TEST

static Suite* testSuite_encryption(void) {
    Suite *s = suite_create("CertificateGroup");
    TCase *tc_encryption_memorystore = tcase_create("CertificateGroup Memorystore");
    tcase_add_checked_fixture(tc_encryption_memorystore, setup, teardown);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc_encryption_memorystore, get_trustlist);
    tcase_add_test(tc_encryption_memorystore, set_trustlist);
    tcase_add_test(tc_encryption_memorystore, set_rejectlist_remove_from_rejectlist);
    tcase_add_test(tc_encryption_memorystore, set_rejectlist_null);
    tcase_add_test(tc_encryption_memorystore, set_rejectlist_remove_non_existent_cert_from_rejectlist);
    tcase_add_test(tc_encryption_memorystore, get_rejectlist_twice);
    tcase_add_test(tc_encryption_memorystore, add_to_trustlist);
    tcase_add_test(tc_encryption_memorystore, add_to_trustlist_with_email);
    tcase_add_test(tc_encryption_memorystore, check_cert_common_name);
    tcase_add_test(tc_encryption_memorystore, remove_from_trustlist);
    tcase_add_test(tc_encryption_memorystore, get_rejectedlist);
    tcase_add_test(tc_encryption_memorystore, verify_expired_certificate_status_depends_on_trust);
#endif /* UA_ENABLE_ENCRYPTION */
    suite_add_tcase(s,tc_encryption_memorystore);

#if defined(__linux__) || defined(UA_ARCHITECTURE_WIN32)
    TCase *tc_encryption_filestore = tcase_create("CertificateGroup Filestore");
    tcase_add_checked_fixture(tc_encryption_filestore, setup2, teardown);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc_encryption_filestore, get_trustlist);
    tcase_add_test(tc_encryption_filestore, set_trustlist);
    tcase_add_test(tc_encryption_filestore, set_rejectlist_remove_from_rejectlist);
    tcase_add_test(tc_encryption_filestore, set_rejectlist_null);
    tcase_add_test(tc_encryption_filestore, set_rejectlist_remove_non_existent_cert_from_rejectlist);
    tcase_add_test(tc_encryption_filestore, get_rejectlist_twice);
    tcase_add_test(tc_encryption_filestore, rejectedlist_filestore_persistence);
    tcase_add_test(tc_encryption_filestore, add_to_trustlist);
    tcase_add_test(tc_encryption_filestore, add_to_trustlist_with_email);
    tcase_add_test(tc_encryption_filestore, check_cert_common_name);
    tcase_add_test(tc_encryption_filestore, remove_from_trustlist);
    tcase_add_test(tc_encryption_filestore, get_rejectedlist);
    tcase_add_test(tc_encryption_filestore, verify_expired_certificate_status_depends_on_trust);
    suite_add_tcase(s,tc_encryption_filestore);
#endif /* UA_ENABLE_ENCRYPTION */
#endif /* defined(__linux__) || defined(UA_ARCHITECTURE_WIN32) */
    return s;
}

int main(void) {
    Suite *s = testSuite_encryption();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
