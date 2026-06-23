/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH
 *
 * In-process handshake test for the ECC_nistP256_AesGcm SecurityPolicy
 * (v1.05.07 secureChannelEnhancements). Exercises the full OPN -> first
 * symmetric MSG (CreateSession) -> CreateSession/ActivateSession flow,
 * which is what fails against the Prosys server. */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_highlevel_async.h>
#include <open62541/plugin/create_certificate.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/plugin/certificategroup_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/accesscontrol_default.h>

#include "client/ua_client_internal.h"
#include "ua_server_internal.h"

#include <stdio.h>
#include <stdlib.h>

#include "test_helpers.h"
#include "certificates.h"
#include "check.h"
#include "testing_clock.h"
#include "testing_networklayers.h"
#include "thread_wrapper.h"

#define AESGCM_URI "http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP256_AesGcm"

UA_Server *server;
UA_Boolean running;
THREAD_HANDLE server_thread;

static const size_t usernamePasswordsSize = 2;
static UA_UsernamePasswordLogin usernamePasswords[2] = {
    {UA_STRING_STATIC("user1"), UA_STRING_STATIC("password")},
    {UA_STRING_STATIC("user2"), UA_STRING_STATIC("password1")}};

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup(void) {
    running = true;

    UA_ByteString certificate;
    certificate.length = CERT_P256_DER_LENGTH;
    certificate.data = CERT_P256_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_P256_DER_LENGTH;
    privateKey.data = KEY_P256_DER_DATA;

    server = UA_Server_newForUnitTestWithSecurityPolicies(4840, &certificate, &privateKey,
                                                          NULL, 0, NULL, 0, NULL, 0);
    ck_assert(server != NULL);

    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_CertificateGroup_AcceptAll(&config->secureChannelPKI);
    UA_CertificateGroup_AcceptAll(&config->sessionPKI);

    /* Allow anonymous and username/password. Bind the username token policy to
     * the ECC_nistP256_AesGcm policy so the password is carried in an
     * EccEncryptedSecret (exercises the ECC user-token decrypt path). */
    UA_String utpUri = UA_STRING(AESGCM_URI);
    UA_AccessControl_default(config, true, &utpUri,
                             usernamePasswordsSize, usernamePasswords);

    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
        UA_STRING_ALLOC("urn:unconfigured:application");

    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    UA_Int32 initValue = 42;
    UA_Variant_setScalar(&vattr.value, &initValue, &UA_TYPES[UA_TYPES_INT32]);
    vattr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 50000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestVar"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              vattr, NULL, NULL);

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static void pauseServer(void) {
    running = false;
    THREAD_JOIN(server_thread);
}

static void runServer(void) {
    running = true;
    THREAD_CREATE(server_thread, serverloop);
}

static UA_Client *
createAesGcmClient(UA_MessageSecurityMode mode) {
    UA_ByteString certificate;
    certificate.length = CERT_P256_DER_LENGTH;
    certificate.data = CERT_P256_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_P256_DER_LENGTH;
    privateKey.data = KEY_P256_DER_DATA;

    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                         NULL, 0, NULL, 0);
    cc->certificateVerification.clear(&cc->certificateVerification);
    UA_CertificateGroup_AcceptAll(&cc->certificateVerification);
    cc->securityPolicyUri = UA_STRING_ALLOC(AESGCM_URI);
    cc->securityMode = mode;

    UA_String_clear(&cc->clientDescription.applicationUri);
    cc->clientDescription.applicationUri =
        UA_STRING_ALLOC("urn:unconfigured:application");
    return client;
}

/* Sign mode (matches the observed Prosys handshake) */
START_TEST(aesgcm_connect_sign) {
    UA_Client *client = createAesGcmClient(UA_MESSAGESECURITYMODE_SIGN);
    ck_assert(client != NULL);

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant val;
    UA_Variant_init(&val);
    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* SignAndEncrypt mode */
START_TEST(aesgcm_connect_signandencrypt) {
    UA_Client *client = createAesGcmClient(UA_MESSAGESECURITYMODE_SIGNANDENCRYPT);
    ck_assert(client != NULL);

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_NodeId testVarId = UA_NODEID_NUMERIC(1, 50000);
    UA_Int32 writeValue = 1234;
    UA_Variant val;
    UA_Variant_setScalar(&val, &writeValue, &UA_TYPES[UA_TYPES_INT32]);
    retval = UA_Client_writeValueAttribute(client, testVarId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant readVal;
    UA_Variant_init(&readVal);
    retval = UA_Client_readValueAttribute(client, testVarId, &readVal);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(*(UA_Int32 *)readVal.data, 1234);
    UA_Variant_clear(&readVal);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* Username/password over ECC: exercises the EccEncryptedSecret password
 * encryption (client) + decryption (server) and the ECDH ephemeral-key
 * exchange via the request/response AdditionalHeader. */
START_TEST(aesgcm_connect_username) {
    UA_Client *client = createAesGcmClient(UA_MESSAGESECURITYMODE_SIGNANDENCRYPT);
    ck_assert(client != NULL);
    UA_ClientConfig *cc = UA_Client_getConfig(client);

    UA_UserNameIdentityToken *u = UA_UserNameIdentityToken_new();
    ck_assert(u != NULL);
    u->userName = UA_STRING_ALLOC("user1");
    u->password = UA_BYTESTRING_ALLOC("password");
    UA_ExtensionObject_clear(&cc->userIdentityToken);
    UA_ExtensionObject_setValue(&cc->userIdentityToken, u,
                                &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN]);

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant val;
    UA_Variant_init(&val);
    retval = UA_Client_readValueAttribute(client,
                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE), &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* X.509 certificate authentication over ECC: the client proves possession of
 * the user certificate's private key via the ActivateSession userTokenSignature.
 * The server (sessionPKI = AcceptAll) advertises a CERTIFICATE UserTokenPolicy
 * bound to the AesGcm policy. Exercises signUserTokenSignature (client) and
 * checkActivateSessionX509 (server) under secureChannelEnhancements. */
START_TEST(aesgcm_connect_certificate) {
    UA_Client *client = createAesGcmClient(UA_MESSAGESECURITYMODE_SIGNANDENCRYPT);
    ck_assert(client != NULL);
    UA_ClientConfig *cc = UA_Client_getConfig(client);

    /* Generate a DISTINCT P-256 user certificate (different DER from the
     * application/channel cert CERT_P256_DER). This is essential: the
     * user-token signature is bound to the client APPLICATION certificate, not
     * the user cert, so reusing one cert for both would let a wrong cert in the
     * signed data slip through (it did - the bug only showed against servers
     * that use a separate user cert). */
    UA_ByteString userCert = UA_BYTESTRING_NULL, userKey = UA_BYTESTRING_NULL;
    UA_String userSubj[3] = {UA_STRING_STATIC("C=DE"),
                             UA_STRING_STATIC("O=Test"),
                             UA_STRING_STATIC("CN=iama.tester@example.com")};
    UA_String userSan[2] = {UA_STRING_STATIC("DNS:localhost"),
                            UA_STRING_STATIC("URI:urn:open62541.tester")};
    UA_KeyValueMap *kvm = UA_KeyValueMap_new();
    UA_String ecType = UA_STRING("ec");
    UA_String ecCurve = UA_STRING("prime256v1");
    UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "key-type"),
                             &ecType, &UA_TYPES[UA_TYPES_STRING]);
    UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "ecc-curve"),
                             &ecCurve, &UA_TYPES[UA_TYPES_STRING]);
    UA_StatusCode retval =
        UA_CreateCertificate(UA_Log_Stdout, userSubj, 3, userSan, 2,
                             UA_CERTIFICATEFORMAT_DER, kvm, &userKey, &userCert);
    UA_KeyValueMap_delete(kvm);
    /* EC certificate generation requires OpenSSL >= 3.0; on older versions
     * UA_CreateCertificate returns BadNotImplemented. Skip the cert-auth case
     * then (the path is covered on OpenSSL 3.x and in live .NET interop). */
    if(retval == UA_STATUSCODE_BADNOTIMPLEMENTED) {
        UA_ByteString_clear(&userCert);
        UA_ByteString_clear(&userKey);
        UA_Client_delete(client);
        return;
    }
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_ClientConfig_setAuthenticationCert(cc, userCert, userKey);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&userCert);
    UA_ByteString_clear(&userKey);

    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant val;
    UA_Variant_init(&val);
    retval = UA_Client_readValueAttribute(client,
                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE), &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* SecureChannel renewal across multiple tokens. Exercises the OPN renewal path
 * (no first-OPN ChannelThumbprint request-signature append on renewals - per
 * Part 6 v1.05.07 / UA-.NETStandard, the request signature is appended only on
 * the first OPN, not on renewals) and the IKM chaining accumulator across
 * several renewals (IKM_n = IKM_{n-1} XOR sharedSecret_n). After each renewal a
 * read must still succeed with the rolled-over keys. */
START_TEST(aesgcm_renew_multiple) {
    UA_Client *client = createAesGcmClient(UA_MESSAGESECURITYMODE_SIGNANDENCRYPT);
    ck_assert(client != NULL);

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_UInt32 channelId = client->channel.securityToken.channelId;

    /* The server thread keeps running. Each round: advance past 75% of the
     * token lifetime so the next client iteration renews the channel, then a
     * read completes the token rollover (and exercises the new keys). This
     * walks the IKM accumulator across several renewals
     * (IKM_n = IKM_{n-1} XOR sharedSecret_n) and the renewal OPN path (no
     * first-OPN request-signature append on renewals). */
    for(int round = 0; round < 3; round++) {
        /* Drive the renewal deterministically: pause the live server thread so
         * the global mock-clock jump and the OPN renewal exchange cannot race
         * with it. (The race surfaces only under slow execution such as
         * valgrind and is a test-harness artifact, not a protocol issue - the
         * token-overlap and live .NET-interop renewals validate the logic.) */
        pauseServer();
        UA_fakeSleep((UA_UInt32)(client->channel.securityToken.revisedLifetime * 0.8));
        UA_Client_run_iterate(client, 1);     /* client sends the RenewSecureChannel OPN */
        UA_Server_run_iterate(server, false); /* server issues the new token + responds */
        UA_Client_run_iterate(client, 1);     /* client adopts the new token */
        runServer();

        UA_Variant val;
        UA_Variant_init(&val);
        retval = UA_Client_readValueAttribute(client,
                     UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE), &val);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        UA_Variant_clear(&val);

        /* Same channel throughout, but the token must have rolled over */
        ck_assert_uint_eq(channelId, client->channel.securityToken.channelId);
    }

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* Token-rollover overlap: the client renews (token T1 -> T2) while a Read
 * request still secured with the OLD token T1 is in flight. The open62541
 * server issues T2 but keeps answering the in-flight request with T1 (Part 4
 * 5.5.2: use the old SecurityToken until a message with the new one arrives).
 * The client processes the OPN response first (its channel now holds T2), then
 * the T1-secured Read response. The AEAD nonce must be masked with the
 * message's TokenId (T1), not the channel's current one (T2) - otherwise the
 * tag verification fails with BadSecurityChecksFailed. */
static UA_Boolean overlapReadDone;
static UA_StatusCode overlapReadStatus;

static void
overlapReadCallback(UA_Client *client, void *userdata,
                    UA_UInt32 requestId, UA_ReadResponse *rr) {
    (void)client; (void)userdata; (void)requestId;
    overlapReadDone = true;
    overlapReadStatus = rr->responseHeader.serviceResult;
}

START_TEST(aesgcm_renew_token_overlap) {
    UA_Client *client = createAesGcmClient(UA_MESSAGESECURITYMODE_SIGNANDENCRYPT);
    ck_assert(client != NULL);

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_UInt32 oldToken = client->channel.securityToken.tokenId;

    /* Take over manual stepping */
    pauseServer();

    /* Trigger the renewal: the client sends the OPN RenewSecureChannel secured
     * with the old token. The response is not processed yet (server paused), so
     * the channel still holds the old token. */
    UA_fakeSleep((UA_UInt32)(client->channel.securityToken.revisedLifetime * 0.8));
    UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(oldToken, client->channel.securityToken.tokenId);

    /* Send a Read still secured with the old token, queued after the OPN */
    overlapReadDone = false;
    overlapReadStatus = UA_STATUSCODE_BADINTERNALERROR;
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_ReadRequest rreq;
    UA_ReadRequest_init(&rreq);
    rreq.nodesToRead = &rvi;
    rreq.nodesToReadSize = 1;
    retval = UA_Client_sendAsyncReadRequest(client, &rreq, overlapReadCallback,
                                            NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Client_run_iterate(client, 1); /* flush the old-token Read */

    /* The server processes the OPN (issues T2, responds with T2) and then the
     * in-flight Read (still answered with the old token T1). */
    UA_Server_run_iterate(server, false);

    /* The client processes the OPN response (-> channel holds T2) and then the
     * T1-secured Read response. */
    UA_Client_run_iterate(client, 1);
    UA_Client_run_iterate(client, 1);

    runServer();

    /* The token rolled over and the old-token response still verified. */
    ck_assert_uint_ne(oldToken, client->channel.securityToken.tokenId);
    ck_assert(overlapReadDone);
    ck_assert_uint_eq(overlapReadStatus, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

static Suite* testSuite_encryption(void) {
    Suite *s = suite_create("EncryptionAesGcm");
    TCase *tc = tcase_create("Encryption ECC_nistP256_AesGcm");
    tcase_add_checked_fixture(tc, setup, teardown);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc, aesgcm_connect_sign);
    tcase_add_test(tc, aesgcm_connect_signandencrypt);
    tcase_add_test(tc, aesgcm_connect_username);
    tcase_add_test(tc, aesgcm_connect_certificate);
    tcase_add_test(tc, aesgcm_renew_multiple);
    tcase_add_test(tc, aesgcm_renew_token_overlap);
#endif
    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    Suite *s = testSuite_encryption();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
