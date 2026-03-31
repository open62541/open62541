#include <open62541/client.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>
#include <open62541/types_generated.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/plugin/securitypolicy_default.h>

#include "thread_wrapper.h"
#include "test_helpers.h"
#include "../encryption/certificates.h"

#include <check.h>
#include <stdlib.h>

UA_Server *server;
UA_Boolean running;
THREAD_HANDLE server_thread;

static const size_t usernamePasswordsSize = 1;
static UA_UsernamePasswordLogin usernamePasswords[1] = {
    {UA_STRING_STATIC("user"), UA_STRING_STATIC("password")}};

#define BASIC256SHA256_URI "http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256"
#define NONE_URI "http://opcfoundation.org/UA/SecurityPolicy#None"
#define TRANSPORT_PROFILE_URI "http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary"

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

/* Set up all three user token policies (anonymous, username, certificate)
 * on an endpoint. If withSecurityPolicy is true, the username and certificate
 * tokens get the Basic256Sha256 securityPolicyUri set. */
static UA_StatusCode
setUserTokenPolicies(UA_EndpointDescription *ep, UA_Boolean withSecurityPolicy) {
    ep->userIdentityTokensSize = 3;
    ep->userIdentityTokens = (UA_UserTokenPolicy *)
        UA_Array_new(3, &UA_TYPES[UA_TYPES_USERTOKENPOLICY]);
    if(!ep->userIdentityTokens)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Anonymous */
    ep->userIdentityTokens[0].tokenType = UA_USERTOKENTYPE_ANONYMOUS;
    ep->userIdentityTokens[0].policyId = UA_STRING_ALLOC("anonymous");

    /* Username */
    ep->userIdentityTokens[1].tokenType = UA_USERTOKENTYPE_USERNAME;
    ep->userIdentityTokens[1].policyId = UA_STRING_ALLOC("username");
    if(withSecurityPolicy)
        ep->userIdentityTokens[1].securityPolicyUri = UA_STRING_ALLOC(BASIC256SHA256_URI);

    /* Certificate */
    ep->userIdentityTokens[2].tokenType = UA_USERTOKENTYPE_CERTIFICATE;
    ep->userIdentityTokens[2].policyId = UA_STRING_ALLOC("certificate");
    if(withSecurityPolicy)
        ep->userIdentityTokens[2].securityPolicyUri = UA_STRING_ALLOC(BASIC256SHA256_URI);

    return UA_STATUSCODE_GOOD;
}

static void setup(void) {
    running = true;
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);

    UA_ServerConfig *config = UA_Server_getConfig(server);

    /* Add the Basic256Sha256 security policy */
    UA_ByteString certificate;
    certificate.length = CERT_DER_LENGTH;
    certificate.data = CERT_DER_DATA;
    UA_ByteString privateKey;
    privateKey.length = KEY_DER_LENGTH;
    privateKey.data = KEY_DER_DATA;
    UA_StatusCode retval =
        UA_ServerConfig_addSecurityPolicyBasic256Sha256(config, &certificate, &privateKey);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Instantiate a new AccessControl plugin that knows username/pw */
    UA_SecurityPolicy *sp = &config->securityPolicies[config->securityPoliciesSize - 1];
    UA_AccessControl_default(config, true, &sp->policyUri,
                             usernamePasswordsSize, usernamePasswords);
    config->allowNonePolicyPassword = true;

    /* Clear existing endpoints */
    for(size_t i = 0; i < config->endpointsSize; i++)
        UA_EndpointDescription_clear(&config->endpoints[i]);
    UA_free(config->endpoints);
    config->endpoints = NULL;
    config->endpointsSize = 0;

    /* Allocate 4 custom endpoints */
    config->endpoints = (UA_EndpointDescription *)
        UA_calloc(4, sizeof(UA_EndpointDescription));
    ck_assert(config->endpoints != NULL);
    config->endpointsSize = 4;

    /* Endpoint 0: None security policy, all user token policies, no encryption */
    UA_EndpointDescription *ep0 = &config->endpoints[0];
    ep0->securityMode = UA_MESSAGESECURITYMODE_NONE;
    ep0->securityPolicyUri = UA_STRING_ALLOC(NONE_URI);
    ep0->transportProfileUri = UA_STRING_ALLOC(TRANSPORT_PROFILE_URI);
    ep0->securityLevel = 0;
    retval = setUserTokenPolicies(ep0, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Endpoint 1: None security policy, all user token policies, with encryption
     * where possible (securityPolicyUri set on username and certificate tokens) */
    UA_EndpointDescription *ep1 = &config->endpoints[1];
    ep1->securityMode = UA_MESSAGESECURITYMODE_NONE;
    ep1->securityPolicyUri = UA_STRING_ALLOC(NONE_URI);
    ep1->transportProfileUri = UA_STRING_ALLOC(TRANSPORT_PROFILE_URI);
    ep1->securityLevel = 0;
    retval = setUserTokenPolicies(ep1, true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Endpoint 2: Basic256Sha256/Sign, all user token policies, no securityPolicy
     * on user tokens */
    UA_EndpointDescription *ep2 = &config->endpoints[2];
    ep2->securityMode = UA_MESSAGESECURITYMODE_SIGN;
    ep2->securityPolicyUri = UA_STRING_ALLOC(BASIC256SHA256_URI);
    ep2->transportProfileUri = UA_STRING_ALLOC(TRANSPORT_PROFILE_URI);
    ep2->securityLevel = 1;
    retval = setUserTokenPolicies(ep2, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Endpoint 3: Basic256Sha256/SignAndEncrypt, all user token policies, with
     * securityPolicy set on user tokens */
    UA_EndpointDescription *ep3 = &config->endpoints[3];
    ep3->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    ep3->securityPolicyUri = UA_STRING_ALLOC(BASIC256SHA256_URI);
    ep3->transportProfileUri = UA_STRING_ALLOC(TRANSPORT_PROFILE_URI);
    ep3->securityLevel = 2;
    retval = setUserTokenPolicies(ep3, true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(testServerCertificateInGetEndpoints) {
    UA_Client *client = UA_Client_newForUnitTest();

    size_t endpointDescriptionsSize = 0;
    UA_EndpointDescription *endpointDescriptions = NULL;
    UA_StatusCode retval =
        UA_Client_getEndpoints(client, "opc.tcp://localhost:4840",
                               &endpointDescriptionsSize, &endpointDescriptions);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert(endpointDescriptionsSize > 0);

    /* Check server certificate presence on each endpoint.
     * The certificate is always required, except when the endpoint security
     * policy is None AND the user token policy has no security policy
     * (or the security policy is None). */
    UA_String noneUri = UA_STRING(NONE_URI);
    for(size_t i = 0; i < endpointDescriptionsSize; i++) {
        UA_EndpointDescription *ep = &endpointDescriptions[i];
        UA_Boolean epIsNone = UA_String_equal(&ep->securityPolicyUri, &noneUri);

        if(!epIsNone) {
            /* For all non-None endpoints, the certificate must be set */
            ck_assert_msg(ep->serverCertificate.length > 0,
                          "Endpoint %zu: server certificate must be set for "
                          "non-None security policy", i);
            continue;
        } else {
            /* For None endpoints, check if any user token policy requires encryption */
            UA_Int32 utpIndexRequiresEncryption = -1;
            for(size_t j = 0; j < ep->userIdentityTokensSize; j++) {
                UA_UserTokenPolicy *utp = &ep->userIdentityTokens[j];
                UA_Boolean utpHasSecPolicy =
                    utp->securityPolicyUri.length > 0 &&
                    !UA_String_equal(&utp->securityPolicyUri, &noneUri);
                if(utpHasSecPolicy) {
                    utpIndexRequiresEncryption = (UA_Int32)j;
                    break;
                }
            }

            if(utpIndexRequiresEncryption != -1) {
                /* Certificate is required */
                ck_assert_msg(ep->serverCertificate.length > 0,
                            "Endpoint %zu, UTP %d: server certificate must be set "
                            "when encryption is needed", i, utpIndexRequiresEncryption);
            }
        }
    }

    UA_Array_delete(endpointDescriptions, endpointDescriptionsSize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);

    UA_Client_delete(client);
}
END_TEST

int main(void) {
    Suite *s = suite_create("Server GetEndpoints");
    TCase *tc = tcase_create("ServerCertificate present in Endpoint");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, testServerCertificateInGetEndpoints);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}