/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) Andreas Hellenbrand
 */

/* The twelve ECC AEAD SecurityPolicies of OPC UA Part 7, one row each, every
 * row through the same tests: the connection in both security modes, both
 * certificate encodings and with every user token type that the policy secures,
 * the SecureChannel renewals (which chain the IKM of the v1.05.07
 * SecureChannelEnhancements), the certificate management of the policy, and the
 * AEAD itself against tampering.
 *
 * The policies are not part of the default policy set, so the server offers
 * only the policy of the row (plus None for GetEndpoints), and the client adds
 * it to the policies of its default configuration. */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_highlevel_async.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/plugin/certificategroup_default.h>
#include <open62541/plugin/create_certificate.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

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

/**************/
/* The policies */
/**************/

typedef UA_StatusCode (*EccAeadCtor)(UA_SecurityPolicy *, const UA_ApplicationType,
                                     const UA_ByteString, const UA_ByteString,
                                     const UA_Logger *);

typedef struct {
    const char *policyUri;
    EccAeadCtor ctor;
    UA_Byte *certDer; size_t certDerLen;
    UA_Byte *keyDer;  size_t keyDerLen;
    UA_Byte *certPem; size_t certPemLen;
    UA_Byte *keyPem;  size_t keyPemLen;
    const char *curve; /* the ecc-curve of UA_CreateCertificate */
} AeadPolicyRow;

#define ROW(FRAG, CTOR, C, CURVE)                                       \
    {"http://opcfoundation.org/UA/SecurityPolicy#" FRAG,                \
     UA_SecurityPolicy_##CTOR,                                          \
     CERT_##C##_DER_DATA, CERT_##C##_DER_LENGTH,                        \
     KEY_##C##_DER_DATA, KEY_##C##_DER_LENGTH,                          \
     CERT_##C##_PEM_DATA, CERT_##C##_PEM_LENGTH,                        \
     KEY_##C##_PEM_DATA, KEY_##C##_PEM_LENGTH, CURVE}

static AeadPolicyRow rows[] = {
    ROW("ECC_nistP256_AesGcm", EccNistP256AesGcm, P256, "prime256v1"),
    ROW("ECC_nistP256_ChaChaPoly", EccNistP256ChaChaPoly, P256, "prime256v1"),
    ROW("ECC_nistP384_AesGcm", EccNistP384AesGcm, P384, "secp384r1"),
    ROW("ECC_nistP384_ChaChaPoly", EccNistP384ChaChaPoly, P384, "secp384r1"),
    ROW("ECC_brainpoolP256r1_AesGcm", EccBrainpoolP256r1AesGcm, BP256R1, "brainpoolP256r1"),
    ROW("ECC_brainpoolP256r1_ChaChaPoly", EccBrainpoolP256r1ChaChaPoly, BP256R1,
        "brainpoolP256r1"),
    ROW("ECC_brainpoolP384r1_AesGcm", EccBrainpoolP384r1AesGcm, BP384R1, "brainpoolP384r1"),
    ROW("ECC_brainpoolP384r1_ChaChaPoly", EccBrainpoolP384r1ChaChaPoly, BP384R1,
        "brainpoolP384r1"),
    ROW("ECC_curve25519_AesGcm", EccCurve25519AesGcm, ED25519, "ed25519"),
    ROW("ECC_curve25519_ChaChaPoly", EccCurve25519ChaChaPoly, ED25519, "ed25519"),
    ROW("ECC_curve448_AesGcm", EccCurve448AesGcm, ED448, "ed448"),
    ROW("ECC_curve448_ChaChaPoly", EccCurve448ChaChaPoly, ED448, "ed448")
};

#define ROWS (sizeof(rows) / sizeof(rows[0]))

/****************/
/* Server setup */
/****************/

static UA_Server *server;
static UA_atomic(uintptr_t) running;
static THREAD_HANDLE server_thread;
static size_t activeRow;

static const size_t usernamePasswordsSize = 1;
static UA_UsernamePasswordLogin usernamePasswords[1] = {
    {UA_STRING_STATIC("user1"), UA_STRING_STATIC("password")}};

THREAD_CALLBACK(serverloop) {
    while(UA_atomic_load(&running))
        UA_Server_run_iterate(server, true);
    return 0;
}

static UA_ByteString certDer(const AeadPolicyRow *r) { return (UA_ByteString){r->certDerLen, r->certDer}; }
static UA_ByteString keyDer(const AeadPolicyRow *r) { return (UA_ByteString){r->keyDerLen, r->keyDer}; }

/* Append a policy built by the constructor of a row to an array of policies */
static UA_StatusCode
appendPolicy(UA_SecurityPolicy **policies, size_t *policiesSize,
             const AeadPolicyRow *r, UA_ApplicationType applicationType,
             UA_ByteString cert, UA_ByteString key, const UA_Logger *logger) {
    UA_SecurityPolicy *tmp = (UA_SecurityPolicy *)
        UA_realloc(*policies, sizeof(UA_SecurityPolicy) * (*policiesSize + 1));
    if(!tmp)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    *policies = tmp;
    UA_StatusCode res = r->ctor(&tmp[*policiesSize], applicationType, cert, key, logger);
    if(res == UA_STATUSCODE_GOOD)
        (*policiesSize)++;
    return res;
}

static UA_SecurityPolicy *
serverPolicy(void) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_String uri = UA_STRING((char *)(uintptr_t)rows[activeRow].policyUri);
    for(size_t i = 0; i < config->securityPoliciesSize; i++) {
        if(UA_String_equal(&config->securityPolicies[i].policyUri, &uri))
            return &config->securityPolicies[i];
    }
    return NULL;
}

static void
setup_common(void) {
    UA_atomic_store(&running, true);
    const AeadPolicyRow *r = &rows[activeRow];

    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_StatusCode res =
        appendPolicy(&config->securityPolicies, &config->securityPoliciesSize, r,
                     UA_APPLICATIONTYPE_SERVER, certDer(r), keyDer(r), config->logging);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_String uri = UA_STRING((char *)(uintptr_t)r->policyUri);
    res = UA_ServerConfig_addEndpoint(config, uri, UA_MESSAGESECURITYMODE_SIGN);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    res = UA_ServerConfig_addEndpoint(config, uri, UA_MESSAGESECURITYMODE_SIGNANDENCRYPT);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_CertificateGroup_AcceptAll(&config->secureChannelPKI);
    UA_CertificateGroup_AcceptAll(&config->sessionPKI);

    /* Anonymous, username/password and X.509, the user tokens secured with the
     * policy of the row */
    res = UA_AccessControl_default(config, true, &uri,
                                   usernamePasswordsSize, usernamePasswords);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
        UA_STRING_ALLOC("urn:unconfigured:application");

    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 initValue = 42;
    UA_Variant_setScalar(&attr.value, &initValue, &UA_TYPES[UA_TYPES_INT32]);
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    res = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 50000),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                    UA_QUALIFIEDNAME(1, "TestVar"),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                    attr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

#define DEFINE_ROW_SETUP(N) \
    static void setup_row_##N(void) { activeRow = N; setup_common(); }

DEFINE_ROW_SETUP(0)
DEFINE_ROW_SETUP(1)
DEFINE_ROW_SETUP(2)
DEFINE_ROW_SETUP(3)
DEFINE_ROW_SETUP(4)
DEFINE_ROW_SETUP(5)
DEFINE_ROW_SETUP(6)
DEFINE_ROW_SETUP(7)
DEFINE_ROW_SETUP(8)
DEFINE_ROW_SETUP(9)
DEFINE_ROW_SETUP(10)
DEFINE_ROW_SETUP(11)

static void (*rowSetups[])(void) = {
    setup_row_0, setup_row_1, setup_row_2, setup_row_3, setup_row_4, setup_row_5,
    setup_row_6, setup_row_7, setup_row_8, setup_row_9, setup_row_10, setup_row_11
};

static void
teardown(void) {
    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static void
pauseServer(void) {
    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);
}

static void
runServer(void) {
    UA_atomic_store(&running, true);
    THREAD_CREATE(server_thread, serverloop);
}

/**********/
/* Client */
/**********/

static UA_Client *
createClient(UA_MessageSecurityMode mode, UA_Boolean pem) {
    const AeadPolicyRow *r = &rows[activeRow];
    UA_ByteString cert = pem ? (UA_ByteString){r->certPemLen, r->certPem} : certDer(r);
    UA_ByteString key = pem ? (UA_ByteString){r->keyPemLen, r->keyPem} : keyDer(r);

    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefaultEncryption(cc, cert, key, NULL, 0, NULL, 0);
    UA_StatusCode res = appendPolicy(&cc->securityPolicies, &cc->securityPoliciesSize, r,
                                     UA_APPLICATIONTYPE_CLIENT, cert, key, cc->logging);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    cc->certificateVerification.clear(&cc->certificateVerification);
    UA_CertificateGroup_AcceptAll(&cc->certificateVerification);
    cc->securityPolicyUri = UA_STRING_ALLOC(r->policyUri);
    cc->securityMode = mode;
    UA_String_clear(&cc->clientDescription.applicationUri);
    cc->clientDescription.applicationUri =
        UA_STRING_ALLOC("urn:unconfigured:application");
    return client;
}

static void
readState(UA_Client *client) {
    UA_Variant val;
    UA_Variant_init(&val);
    UA_StatusCode res = UA_Client_readValueAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);
}

static void
connectReadClose(UA_Client *client) {
    UA_StatusCode res = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    readState(client);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}

/* One renewal with the server paused, so that the mock-clock jump and the OPN
 * exchange cannot race with the server thread */
static void
renew(UA_Client *client, UA_Boolean beforeTokenExpiry) {
    UA_UInt32 oldToken = client->channel.securityToken.tokenId;
    if(beforeTokenExpiry) {
        client->nextChannelRenewal = 0;
        ck_assert_uint_eq(UA_Client_renewSecureChannel(client), UA_STATUSCODE_GOOD);
    } else {
        UA_fakeSleep((UA_UInt32)(client->channel.securityToken.revisedLifetime * 0.8));
        UA_Client_run_iterate(client, 1);     /* client sends the RenewSecureChannel OPN */
    }
    UA_Server_run_iterate(server, false);     /* server issues the new token + responds */
    UA_Client_run_iterate(client, 1);         /* client adopts the new token */
    ck_assert_uint_ne(oldToken, client->channel.securityToken.tokenId);
}

/**************/
/* Connection */
/**************/

START_TEST(connect_sign) {
    connectReadClose(createClient(UA_MESSAGESECURITYMODE_SIGN, false));
} END_TEST

START_TEST(connect_signandencrypt) {
    UA_Client *client = createClient(UA_MESSAGESECURITYMODE_SIGNANDENCRYPT, false);
    UA_StatusCode res = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    readState(client);

    /* A write for a request larger than a read */
    UA_Int32 value = 9999;
    UA_Variant v;
    UA_Variant_setScalar(&v, &value, &UA_TYPES[UA_TYPES_INT32]);
    res = UA_Client_writeValueAttribute(client, UA_NODEID_NUMERIC(1, 50000), &v);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(connect_pem) {
    connectReadClose(createClient(UA_MESSAGESECURITYMODE_SIGNANDENCRYPT, true));
} END_TEST

/* The password travels in an EccEncryptedSecret: the client derives its key
 * from the ephemeral key the server sent in the CreateSession response */
START_TEST(connect_username) {
    UA_Client *client = createClient(UA_MESSAGESECURITYMODE_SIGNANDENCRYPT, false);
    UA_ClientConfig_setAuthenticationUsername(UA_Client_getConfig(client),
                                              "user1", "password");
    connectReadClose(client);
} END_TEST

/* The wrong password must be refused, not merely decrypted to garbage */
START_TEST(connect_username_wrong_password) {
    UA_Client *client = createClient(UA_MESSAGESECURITYMODE_SIGNANDENCRYPT, false);
    UA_ClientConfig_setAuthenticationUsername(UA_Client_getConfig(client),
                                              "user1", "wrong");
    UA_StatusCode res = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_ne(res, UA_STATUSCODE_GOOD);
    UA_Client_delete(client);
} END_TEST

/* The client proves possession of the key of a user certificate on the curve of
 * the policy with the userTokenSignature. The user certificate differs from the
 * application certificate, since the signature is bound to the latter. */
START_TEST(connect_x509) {
    const AeadPolicyRow *r = &rows[activeRow];
    UA_ByteString userCert = UA_BYTESTRING_NULL, userKey = UA_BYTESTRING_NULL;
    UA_String subject[2] = {UA_STRING_STATIC("O=Test"),
                            UA_STRING_STATIC("CN=user@example.com")};
    UA_String san[1] = {UA_STRING_STATIC("URI:urn:open62541.user")};
    UA_KeyValueMap *kvm = UA_KeyValueMap_new();
    UA_String keyType = UA_STRING("ec");
    UA_String curve = UA_STRING((char *)(uintptr_t)r->curve);
    UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "key-type"),
                             &keyType, &UA_TYPES[UA_TYPES_STRING]);
    UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "ecc-curve"),
                             &curve, &UA_TYPES[UA_TYPES_STRING]);
    UA_StatusCode res = UA_CreateCertificate(UA_Log_Stdout, subject, 2, san, 1,
                                             UA_CERTIFICATEFORMAT_DER, kvm,
                                             &userKey, &userCert);
    UA_KeyValueMap_delete(kvm);
    if(res == UA_STATUSCODE_BADNOTIMPLEMENTED)
        return; /* EC certificate generation needs OpenSSL 3 */
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_Client *client = createClient(UA_MESSAGESECURITYMODE_SIGNANDENCRYPT, false);
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    res = UA_ClientConfig_setAuthenticationCert(cc, userCert, userKey);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    /* The default policy set has no instance of this policy for the user key */
    res = appendPolicy(&cc->authSecurityPolicies, &cc->authSecurityPoliciesSize, r,
                       UA_APPLICATIONTYPE_CLIENT, userCert, userKey, cc->logging);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&userCert);
    UA_ByteString_clear(&userKey);
    connectReadClose(client);
} END_TEST

START_TEST(reconnect) {
    UA_Client *client = createClient(UA_MESSAGESECURITYMODE_SIGNANDENCRYPT, false);
    UA_StatusCode res = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    readState(client);
    UA_Client_disconnect(client);
    res = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    readState(client);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

/************/
/* Renewals */
/************/

/* A read after every renewal. Each one walks the IKM accumulator
 * (IKM_n = IKM_n-1 XOR sharedSecret_n), whose width is the shared secret of the
 * policy: 32 or 48 bytes for the Weierstrass curves, 32 or 56 for X25519/X448. */
START_TEST(renew_multiple) {
    UA_Client *client = createClient(UA_MESSAGESECURITYMODE_SIGNANDENCRYPT, false);
    UA_StatusCode res = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_UInt32 channelId = client->channel.securityToken.channelId;
    for(int round = 0; round < 3; round++) {
        pauseServer();
        renew(client, false);
        runServer();
        readState(client);
    }
    ck_assert_uint_eq(channelId, client->channel.securityToken.channelId);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

/* Two renewals without a message in between: the client derives the local keys
 * of both, but the remote keys of neither, so both peers must advance the IKM
 * in the local-keys pass */
START_TEST(renew_twice_without_traffic) {
    UA_Client *client = createClient(UA_MESSAGESECURITYMODE_SIGNANDENCRYPT, false);
    UA_StatusCode res = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    pauseServer();
    renew(client, false);
    renew(client, false);
    runServer();
    readState(client);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

/* As above, but the second renewal arrives before the old token expired, so the
 * server never rolls over to the first new token */
START_TEST(renew_twice_before_token_expiry) {
    UA_Client *client = createClient(UA_MESSAGESECURITYMODE_SIGNANDENCRYPT, false);
    UA_StatusCode res = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    pauseServer();
    renew(client, false);
    renew(client, true);
    runServer();
    readState(client);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

/* A Read secured with the old token is in flight while the client renews. The
 * server answers it with the old token (Part 4 5.5.2), so the AEAD nonce of the
 * response must be masked with the TokenId of the message, not the current one
 * of the channel. */
static UA_Boolean overlapReadDone;
static UA_StatusCode overlapReadStatus;

static void
overlapReadCallback(UA_Client *client, void *userdata,
                    UA_UInt32 requestId, UA_ReadResponse *rr) {
    overlapReadDone = true;
    overlapReadStatus = rr->responseHeader.serviceResult;
}

START_TEST(renew_token_overlap) {
    UA_Client *client = createClient(UA_MESSAGESECURITYMODE_SIGNANDENCRYPT, false);
    UA_StatusCode res = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_UInt32 oldToken = client->channel.securityToken.tokenId;

    pauseServer();
    UA_fakeSleep((UA_UInt32)(client->channel.securityToken.revisedLifetime * 0.8));
    UA_Client_run_iterate(client, 1); /* the renewal OPN, response not processed yet */
    ck_assert_uint_eq(oldToken, client->channel.securityToken.tokenId);

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
    res = UA_Client_sendAsyncReadRequest(client, &rreq, overlapReadCallback, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Client_run_iterate(client, 1); /* the Read, still with the old token */

    UA_Server_run_iterate(server, false); /* the OPN, then the Read */
    UA_Client_run_iterate(client, 1);     /* the OPN response: new token */
    UA_Client_run_iterate(client, 1);     /* the Read response: old token */
    runServer();

    ck_assert_uint_ne(oldToken, client->channel.securityToken.tokenId);
    ck_assert(overlapReadDone);
    ck_assert_uint_eq(overlapReadStatus, UA_STATUSCODE_GOOD);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

/***************************/
/* Certificate management  */
/***************************/

START_TEST(csr_generation) {
    UA_SecurityPolicy *sp = serverPolicy();
    ck_assert_ptr_ne(sp, NULL);
    UA_String subjectName = UA_STRING("CN=TestECC O=open62541");

    /* With the current private key */
    UA_ByteString csr = UA_BYTESTRING_NULL;
    UA_StatusCode res = sp->createSigningRequest(sp, &subjectName, NULL, NULL, &csr, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(csr.length, 0);
    UA_ByteString_clear(&csr);

    /* With a new private key */
    UA_ByteString newKey = UA_BYTESTRING_NULL;
    res = sp->createSigningRequest(sp, &subjectName, NULL, NULL, &csr, &newKey);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(csr.length, 0);
    /* A key of the type of the policy: every ECC key is below 200 bytes in
     * DER, an RSA key of the default size is several times that */
    ck_assert_uint_gt(newKey.length, 0);
    ck_assert_uint_lt(newKey.length, 200);
    UA_ByteString_clear(&csr);
    UA_ByteString_clear(&newKey);
} END_TEST

START_TEST(update_certificate) {
    UA_SecurityPolicy *sp = serverPolicy();
    ck_assert_ptr_ne(sp, NULL);
    const AeadPolicyRow *r = &rows[activeRow];
    UA_StatusCode res = sp->updateCertificate(sp, certDer(r), keyDer(r));
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* The policy still serves connections */
    connectReadClose(createClient(UA_MESSAGESECURITYMODE_SIGNANDENCRYPT, false));
} END_TEST

/********/
/* AEAD */
/********/

/* The symmetric callbacks of the policy directly: a message encrypted (or
 * signed) under one set of parameters must not decrypt (or verify) under any
 * other. The nonce is the IV masked with the TokenId and the sequence number,
 * the header is the additional authenticated data. */
START_TEST(aead_rejects_tampering) {
    UA_SecurityPolicy *sp = serverPolicy();
    ck_assert_ptr_ne(sp, NULL);
    void *cc = NULL;
    UA_StatusCode res = sp->newChannelContext(sp, &sp->localCertificate, &cc);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* The same keys in both directions, so that the policy decrypts what it
     * encrypted */
    size_t keyLen = sp->symEncryptionAlgorithm.getLocalKeyLength(sp, cc);
    size_t ivLen = sp->symEncryptionAlgorithm.getLocalIvLength(sp, cc);
    ck_assert(keyLen == 16 || keyLen == 32);
    ck_assert_uint_eq(ivLen, 12);
    UA_Byte keyBytes[32], ivBytes[12];
    memset(keyBytes, 0x11, sizeof(keyBytes));
    memset(ivBytes, 0x22, sizeof(ivBytes));
    UA_ByteString key = {keyLen, keyBytes}, iv = {ivLen, ivBytes};
    ck_assert_uint_eq(sp->setLocalSymEncryptingKey(sp, cc, &key), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(sp->setRemoteSymEncryptingKey(sp, cc, &key), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(sp->setLocalSymIv(sp, cc, &iv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(sp->setRemoteSymIv(sp, cc, &iv), UA_STATUSCODE_GOOD);

    UA_ByteString aad = UA_BYTESTRING("header");
    UA_ByteString otherAad = UA_BYTESTRING("Header");
    ck_assert_uint_eq(sp->setMessageSecurityParameters(sp, cc, 7, 41, &aad),
                      UA_STATUSCODE_GOOD);

    /* Encrypt: 32 bytes of plaintext and room for the 16-byte tag */
    UA_Byte plain[32], sealed[48], buf[48];
    for(size_t i = 0; i < sizeof(plain); i++)
        plain[i] = (UA_Byte)i;
    memcpy(sealed, plain, sizeof(plain));
    UA_ByteString data = {sizeof(sealed), sealed};
    ck_assert_uint_eq(sp->symEncryptionAlgorithm.encrypt(sp, cc, &data),
                      UA_STATUSCODE_GOOD);
    ck_assert(memcmp(sealed, plain, sizeof(plain)) != 0);

    /* Decrypts with the same parameters */
    memcpy(buf, sealed, sizeof(buf));
    data = (UA_ByteString){sizeof(buf), buf};
    ck_assert_uint_eq(sp->symEncryptionAlgorithm.decrypt(sp, cc, &data),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(data.length, sizeof(plain));
    ck_assert_mem_eq(buf, plain, sizeof(plain));

    /* Does not decrypt with any change */
    for(int change = 0; change < 5; change++) {
        UA_UInt32 tokenId = 7, seqNo = 41;
        const UA_ByteString *ad = &aad;
        memcpy(buf, sealed, sizeof(buf));
        switch(change) {
        case 0: buf[0] ^= 1; break;        /* ciphertext */
        case 1: buf[47] ^= 1; break;       /* tag */
        case 2: ad = &otherAad; break;     /* header */
        case 3: tokenId = 8; break;        /* nonce: TokenId */
        default: seqNo = 42; break;        /* nonce: sequence number */
        }
        ck_assert_uint_eq(sp->setMessageSecurityParameters(sp, cc, tokenId, seqNo, ad),
                          UA_STATUSCODE_GOOD);
        data = (UA_ByteString){sizeof(buf), buf};
        ck_assert_uint_eq(sp->symEncryptionAlgorithm.decrypt(sp, cc, &data),
                          UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    }

    /* Sign only: the tag over the whole message, in its own buffer */
    ck_assert_uint_eq(sp->setMessageSecurityParameters(sp, cc, 7, 41, &aad),
                      UA_STATUSCODE_GOOD);
    UA_ByteString message = {sizeof(plain), plain};
    UA_Byte tagBytes[16];
    UA_ByteString tag = {sizeof(tagBytes), tagBytes};
    ck_assert_uint_eq(sp->symSignatureAlgorithm.getLocalSignatureSize(sp, cc), 16);
    ck_assert_uint_eq(sp->symSignatureAlgorithm.sign(sp, cc, &message, &tag),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(sp->symSignatureAlgorithm.verify(sp, cc, &message, &tag),
                      UA_STATUSCODE_GOOD);
    plain[0] ^= 1;
    ck_assert_uint_eq(sp->symSignatureAlgorithm.verify(sp, cc, &message, &tag),
                      UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    plain[0] ^= 1;
    tag.length = 15;
    ck_assert_uint_eq(sp->symSignatureAlgorithm.verify(sp, cc, &message, &tag),
                      UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    sp->deleteChannelContext(sp, cc);
} END_TEST

static Suite *
testSuite_eccAead(void) {
    Suite *s = suite_create("Encryption ECC AEAD");
    for(size_t i = 0; i < ROWS; i++) {
        /* The TCase name is the policy */
        TCase *tc = tcase_create(strchr(rows[i].policyUri, '#') + 1);
        tcase_add_checked_fixture(tc, rowSetups[i], teardown);
        tcase_add_test(tc, connect_sign);
        tcase_add_test(tc, connect_signandencrypt);
        tcase_add_test(tc, connect_pem);
        tcase_add_test(tc, connect_username);
        tcase_add_test(tc, connect_x509);
        tcase_add_test(tc, renew_multiple);
        tcase_add_test(tc, renew_twice_without_traffic);
        tcase_add_test(tc, renew_twice_before_token_expiry);
        tcase_add_test(tc, csr_generation);
        tcase_add_test(tc, update_certificate);
        tcase_add_test(tc, aead_rejects_tampering);

        /* Not specific to the policy, and each one a full handshake: only for
         * the first row (ECDH, AES-GCM) and the last (XDH, ChaCha20-Poly1305).
         * The nonce masking of renew_token_overlap is covered for every row by
         * aead_rejects_tampering. */
        if(i == 0 || i == ROWS - 1) {
            tcase_add_test(tc, connect_username_wrong_password);
            tcase_add_test(tc, reconnect);
            tcase_add_test(tc, renew_token_overlap);
        }
        suite_add_tcase(s, tc);
    }
    return s;
}

int main(void) {
    Suite *s = testSuite_eccAead();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
