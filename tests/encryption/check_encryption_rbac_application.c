/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

/* RBAC Application criterion end-to-end (Part 18 §4.4.3)
 *
 * "If the criteriaType is Application, the criteria is the ApplicationUri from
 * the Client Certificate used for the Session." The server tolerates a client
 * whose ApplicationDescription declares another ApplicationUri than its
 * certificate (allowAllCertificateUris = WARN). The declared URI must then not
 * grant a Role, neither through an Application identity rule nor through the
 * Applications filter of a Role. */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/certificategroup_default.h>
#include <open62541/plugin/create_certificate.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "client/ua_client_internal.h"
#include "ua_server_internal.h"

#include <check.h>
#include <stdlib.h>

#include "certificates.h"
#include "test_helpers.h"
#include "thread_wrapper.h"

#define SERVER_URL "opc.tcp://localhost:4995"
#define CERT_URI "urn:cert"
#define DECLARED_URI "urn:declared"

static UA_Server *server;
static UA_atomic(uintptr_t) running;
static THREAD_HANDLE server_thread;

static UA_NodeId declaredRuleRoleId;   /* Application rule for DECLARED_URI */
static UA_NodeId certRuleRoleId;       /* Application rule for CERT_URI */
static UA_NodeId declaredFilterRoleId; /* Applications filter for DECLARED_URI */
static UA_NodeId certFilterRoleId;     /* Applications filter for CERT_URI */

THREAD_CALLBACK(serverloop) {
    while(UA_atomic_load(&running))
        UA_Server_run_iterate(server, true);
    return 0;
}

/* Add a Role with an Application identity rule for the URI */
static UA_NodeId
addApplicationRuleRole(const char *name, const char *applicationUri) {
    UA_IdentityMappingRuleType rule;
    UA_IdentityMappingRuleType_init(&rule);
    rule.criteriaType = UA_IDENTITYCRITERIATYPE_APPLICATION;
    rule.criteria = UA_STRING((char*)(uintptr_t)applicationUri);
    UA_Role role;
    UA_Role_init(&role);
    role.roleName = UA_QUALIFIEDNAME(1, (char*)(uintptr_t)name);
    role.identityMappingRules = &rule;
    role.identityMappingRulesSize = 1;
    UA_NodeId roleId = UA_NODEID_NULL;
    ck_assert_uint_eq(UA_Server_addRole(server, &role, &roleId),
                      UA_STATUSCODE_GOOD);
    return roleId;
}

/* Add a Role for anonymous Sessions that includes only the application */
static UA_NodeId
addApplicationFilterRole(const char *name, const char *applicationUri) {
    UA_IdentityMappingRuleType rule;
    UA_IdentityMappingRuleType_init(&rule);
    rule.criteriaType = UA_IDENTITYCRITERIATYPE_ANONYMOUS;
    UA_String application = UA_STRING((char*)(uintptr_t)applicationUri);
    UA_Role role;
    UA_Role_init(&role);
    role.roleName = UA_QUALIFIEDNAME(1, (char*)(uintptr_t)name);
    role.identityMappingRules = &rule;
    role.identityMappingRulesSize = 1;
    role.applications = &application;
    role.applicationsSize = 1;
    role.applicationsExclude = false;
    UA_NodeId roleId = UA_NODEID_NULL;
    ck_assert_uint_eq(UA_Server_addRole(server, &role, &roleId),
                      UA_STATUSCODE_GOOD);
    return roleId;
}

static void setup(void) {
    UA_atomic_store(&running, true);

    UA_ByteString certificate = {CERT_DER_LENGTH, CERT_DER_DATA};
    UA_ByteString privateKey = {KEY_DER_LENGTH, KEY_DER_DATA};
    server = UA_Server_newForUnitTestWithSecurityPolicies(
        4995, &certificate, &privateKey, NULL, 0, NULL, 0, NULL, 0);
    ck_assert(server != NULL);

    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_CertificateGroup_AcceptAll(&config->secureChannelPKI);
    UA_CertificateGroup_AcceptAll(&config->sessionPKI);
    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
        UA_STRING_ALLOC("urn:open62541.unconfigured.application");

    /* Accept a client whose declared ApplicationUri does not match the URI in
     * its certificate */
    config->allowAllCertificateUris = UA_RULEHANDLING_WARN;

    declaredRuleRoleId = addApplicationRuleRole("DeclaredRule", DECLARED_URI);
    certRuleRoleId = addApplicationRuleRole("CertRule", CERT_URI);
    declaredFilterRoleId =
        addApplicationFilterRole("DeclaredFilter", DECLARED_URI);
    certFilterRoleId = addApplicationFilterRole("CertFilter", CERT_URI);

    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_NodeId_clear(&declaredRuleRoleId);
    UA_NodeId_clear(&certRuleRoleId);
    UA_NodeId_clear(&declaredFilterRoleId);
    UA_NodeId_clear(&certFilterRoleId);
    UA_Server_delete(server);
}

/* Connect anonymously over SignAndEncrypt with a new client certificate that
 * has the given subjectAltName URIs. The client declares DECLARED_URI. */
static UA_Client *
connectWithCertificate(const UA_String *subjectAltName,
                       size_t subjectAltNameSize) {
    UA_String subject[2] = {UA_STRING_STATIC("O=open62541"),
                            UA_STRING_STATIC("CN=RBAC Application Test")};
    UA_KeyValueMap *kvm = UA_KeyValueMap_new();
    UA_UInt16 keyLength = 2048;
    UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "key-size-bits"),
                             (void *)&keyLength, &UA_TYPES[UA_TYPES_UINT16]);
    UA_ByteString clientKey = UA_BYTESTRING_NULL;
    UA_ByteString clientCert = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_CreateCertificate(
        UA_Log_Stdout, subject, 2, subjectAltName, subjectAltNameSize,
        UA_CERTIFICATEFORMAT_DER, kvm, &clientKey, &clientCert);
    UA_KeyValueMap_delete(kvm);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_ptr_nonnull(client);
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    res = UA_ClientConfig_setDefaultEncryption(cc, clientCert, clientKey,
                                               NULL, 0, NULL, 0);
    UA_ByteString_clear(&clientCert);
    UA_ByteString_clear(&clientKey);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_CertificateGroup_AcceptAll(&cc->certificateVerification);
    cc->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    UA_String_clear(&cc->securityPolicyUri);
    cc->securityPolicyUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
    UA_String_clear(&cc->clientDescription.applicationUri);
    cc->clientDescription.applicationUri = UA_STRING_ALLOC(DECLARED_URI);

    ck_assert_uint_eq(UA_Client_connect(client, SERVER_URL),
                      UA_STATUSCODE_GOOD);
    return client;
}

/* The Roles of the client's Session, read on the server side */
static UA_Boolean
sessionHasRole(UA_Client *client, const UA_NodeId *roleId) {
    UA_Variant roles;
    UA_Variant_init(&roles);
    UA_StatusCode res =
        UA_Server_getSessionAttributeCopy(server, &client->sessionId,
                                          UA_QUALIFIEDNAME(0, "roles"), &roles);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Boolean found = false;
    if(UA_Variant_hasArrayType(&roles, &UA_TYPES[UA_TYPES_NODEID])) {
        const UA_NodeId *ids = (const UA_NodeId*)roles.data;
        for(size_t i = 0; i < roles.arrayLength; i++) {
            if(UA_NodeId_equal(&ids[i], roleId))
                found = true;
        }
    }
    UA_Variant_clear(&roles);
    return found;
}

static void
disconnect(UA_Client *client) {
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}

/* The Application criterion is taken from the certificate, not from the
 * ApplicationDescription of CreateSession */
START_TEST(applicationCriterion_fromCertificate) {
    UA_String san[2] = {UA_STRING_STATIC("URI:" CERT_URI),
                        UA_STRING_STATIC("DNS:localhost")};
    UA_Client *client = connectWithCertificate(san, 2);
    ck_assert(sessionHasRole(client, &certRuleRoleId));
    ck_assert(!sessionHasRole(client, &declaredRuleRoleId));
    ck_assert(sessionHasRole(client, &certFilterRoleId));
    ck_assert(!sessionHasRole(client, &declaredFilterRoleId));
    disconnect(client);
}
END_TEST

/* A certificate with more than one URI has no ApplicationUri (Part 6 §6.2.2).
 * The Session is activated, but no Application criterion or filter matches,
 * even though the declared URI is one of the certificate URIs. */
START_TEST(applicationCriterion_ambiguousCertificate) {
    UA_String san[3] = {UA_STRING_STATIC("URI:" DECLARED_URI),
                        UA_STRING_STATIC("URI:" CERT_URI),
                        UA_STRING_STATIC("DNS:localhost")};
    UA_Client *client = connectWithCertificate(san, 3);
    ck_assert(!sessionHasRole(client, &certRuleRoleId));
    ck_assert(!sessionHasRole(client, &declaredRuleRoleId));
    ck_assert(!sessionHasRole(client, &certFilterRoleId));
    ck_assert(!sessionHasRole(client, &declaredFilterRoleId));
    disconnect(client);
}
END_TEST

static Suite *
testSuite_rbacApplication(void) {
    Suite *s = suite_create("RBAC Application criterion");
    TCase *tc = tcase_create("ApplicationUri from the client certificate");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, applicationCriterion_fromCertificate);
    tcase_add_test(tc, applicationCriterion_ambiguousCertificate);
    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    Suite *s = testSuite_rbacApplication();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
