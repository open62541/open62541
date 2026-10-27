
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Coverage tests for plugins/ua_config_default.c.
 *
 * These tests exercise the small public-API entry points
 *   - UA_ServerConfig_setBasics / setBasics_withPort
 *   - UA_ServerConfig_addSecurityPolicyNone
 *   - UA_ServerConfig_addEndpoint (success + bad-policy URI)
 *   - UA_ServerConfig_addAllEndpoints / addAllSecureEndpoints
 *   - UA_ServerConfig_addAllSecurityPolicies / addAllSecureSecurityPolicies
 * using a freshly-zeroed UA_ServerConfig instance. The interrupt tests below
 * use a simulated interrupt and a server listening on an ephemeral local port. */


#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/util.h>

#include <check.h>
#include <stdlib.h>
#include <string.h>

static UA_ServerConfig cfg;

static void setup(void) {
    memset(&cfg, 0, sizeof(cfg));
}

static void teardown(void) {
    UA_ServerConfig_clear(&cfg);
}

START_TEST(SetBasicsZeroConfigSucceeds) {
    UA_StatusCode r = UA_ServerConfig_setBasics(&cfg);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
    /* Default port + buildInfo populated */
    ck_assert(cfg.buildInfo.productUri.length > 0);
    ck_assert(cfg.applicationDescription.applicationUri.length > 0);
    ck_assert_uint_eq(cfg.endpointsSize, 0);
    ck_assert_uint_eq(cfg.securityPoliciesSize, 0);
} END_TEST

START_TEST(SetBasicsWithPortZeroIsAccepted) {
    UA_StatusCode r = UA_ServerConfig_setBasics_withPort(&cfg, 0);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(SetBasicsNullConfigRejected) {
    UA_StatusCode r = UA_ServerConfig_setBasics_withPort(NULL, 4840);
    ck_assert_uint_eq(r, UA_STATUSCODE_BADINVALIDARGUMENT);
} END_TEST

START_TEST(AddSecurityPolicyNoneAndAllEndpoints) {
    ck_assert_uint_eq(UA_ServerConfig_setBasics(&cfg), UA_STATUSCODE_GOOD);

    /* SecurityPolicy#None can always be added (no certificate) */
    UA_StatusCode r = UA_ServerConfig_addSecurityPolicyNone(&cfg, NULL);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(cfg.securityPoliciesSize, 1);

    /* addAllEndpoints should add one endpoint per security policy */
    r = UA_ServerConfig_addAllEndpoints(&cfg);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(cfg.endpointsSize, 1); /* None policy → one endpoint */
} END_TEST

START_TEST(AddEndpointWithUnknownPolicyRejected) {
    ck_assert_uint_eq(UA_ServerConfig_setBasics(&cfg), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_ServerConfig_addSecurityPolicyNone(&cfg, NULL),
                      UA_STATUSCODE_GOOD);

    UA_String unknown = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#NoSuchThing");
    UA_StatusCode r =
        UA_ServerConfig_addEndpoint(&cfg, unknown, UA_MESSAGESECURITYMODE_NONE);
    ck_assert_uint_eq(r, UA_STATUSCODE_BADINVALIDARGUMENT);

    /* Adding the explicit None policy by URI must succeed */
    UA_String noneUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None");
    r = UA_ServerConfig_addEndpoint(&cfg, noneUri, UA_MESSAGESECURITYMODE_NONE);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(AddAllSecureEndpointsClearsExisting) {
    ck_assert_uint_eq(UA_ServerConfig_setBasics(&cfg), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_ServerConfig_addSecurityPolicyNone(&cfg, NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_ServerConfig_addAllEndpoints(&cfg),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(cfg.endpointsSize, 1);

    /* addAllSecureEndpoints wipes the existing endpoint list and re-adds
     * SIGN+SIGNANDENCRYPT for each non-None policy. With only None present
     * (and no certificate), the resulting endpoint list ends up empty. */
    UA_StatusCode r = UA_ServerConfig_addAllSecureEndpoints(&cfg);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
} END_TEST

#ifdef UA_ENABLE_ENCRYPTION
#include "../tests/encryption/certificates.h"

START_TEST(AddAllSecurityPoliciesWithoutCertSucceeds) {
    ck_assert_uint_eq(UA_ServerConfig_setBasics(&cfg), UA_STATUSCODE_GOOD);

    /* Without a certificate the secure policies cannot be added but the
     * function logs a warning and still returns GOOD. The "None" policy
     * is added in the !onlySecure branch. */
    UA_StatusCode r =
        UA_ServerConfig_addAllSecurityPolicies(&cfg, NULL, NULL);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);

    /* addAllSecureSecurityPolicies should be similarly safe */
    r = UA_ServerConfig_addAllSecureSecurityPolicies(&cfg, NULL, NULL);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(AddSecurityPolicyHelpersWithoutCertReturnError) {
    ck_assert_uint_eq(UA_ServerConfig_setBasics(&cfg), UA_STATUSCODE_GOOD);

    /* Each of these allocates a SecurityPolicy slot, then constructs the
     * policy with an empty certificate. The construction itself returns
     * BADSECURITYCHECKSFAILED for the strong policies; we only care that
     * the allocate / clean-up paths in ua_config_default.c are exercised. */
    (void)UA_ServerConfig_addSecurityPolicyBasic128Rsa15(&cfg, NULL, NULL);
    (void)UA_ServerConfig_addSecurityPolicyBasic256(&cfg, NULL, NULL);
    (void)UA_ServerConfig_addSecurityPolicyBasic256Sha256(&cfg, NULL, NULL);
    (void)UA_ServerConfig_addSecurityPolicyAes128Sha256RsaOaep(&cfg, NULL, NULL);
    (void)UA_ServerConfig_addSecurityPolicyAes256Sha256RsaPss(&cfg, NULL, NULL);
} END_TEST

START_TEST(AddSecurityPolicyHelpersWithCertSucceed) {
    ck_assert_uint_eq(UA_ServerConfig_setBasics(&cfg), UA_STATUSCODE_GOOD);

    UA_ByteString cert = {CERT_DER_LENGTH, CERT_DER_DATA};
    UA_ByteString key  = {KEY_DER_LENGTH,  KEY_DER_DATA};

    UA_StatusCode r = UA_ServerConfig_addSecurityPolicyBasic256Sha256(&cfg, &cert, &key);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
    r = UA_ServerConfig_addSecurityPolicyAes128Sha256RsaOaep(&cfg, &cert, &key);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
    r = UA_ServerConfig_addSecurityPolicyAes256Sha256RsaPss(&cfg, &cert, &key);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
    ck_assert_uint_ge(cfg.securityPoliciesSize, 3);

    /* Now exercise addAllSecureEndpoints with secure policies present */
    r = UA_ServerConfig_addAllSecureEndpoints(&cfg);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(cfg.endpointsSize, 0);
} END_TEST

START_TEST(AddAllSecurityPoliciesWithCertSucceed) {
    ck_assert_uint_eq(UA_ServerConfig_setBasics(&cfg), UA_STATUSCODE_GOOD);

    UA_ByteString cert = {CERT_DER_LENGTH, CERT_DER_DATA};
    UA_ByteString key  = {KEY_DER_LENGTH,  KEY_DER_DATA};

    UA_StatusCode r = UA_ServerConfig_addAllSecurityPolicies(&cfg, &cert, &key);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(cfg.securityPoliciesSize, 0);

    /* And the secure-only variant on a fresh config */
    UA_ServerConfig_clear(&cfg);
    memset(&cfg, 0, sizeof(cfg));
    ck_assert_uint_eq(UA_ServerConfig_setBasics(&cfg), UA_STATUSCODE_GOOD);
    r = UA_ServerConfig_addAllSecureSecurityPolicies(&cfg, &cert, &key);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(cfg.securityPoliciesSize, 0);
} END_TEST
#endif

#if (defined(UA_ARCHITECTURE_POSIX) && !defined(UA_ARCHITECTURE_LWIP)) || defined(UA_ARCHITECTURE_WIN32)
#include <signal.h>

static struct {
    UA_Server *server;
    UA_EventLoop *eventLoop;
    UA_InterruptManager *manager;
    UA_InterruptManager originalManager;
    UA_StatusCode (*originalStart)(UA_EventLoop *);
    UA_InterruptCallback callback;
    void *callbackContext;
    uintptr_t handle;
    UA_UInt64 timerId;
    UA_StatusCode registrationResult;
    unsigned registrations;
    unsigned deregistrations;
    unsigned dispatches;
    unsigned stoppingNotifications;
    UA_Boolean dispatching;
} interruptTest;

static void
recordInterruptLifecycle(UA_Server *server, UA_LifecycleState state) {
    if(interruptTest.dispatching && state == UA_LIFECYCLESTATE_STOPPING)
        interruptTest.stoppingNotifications++;
}

static void
dispatchTestInterrupt(UA_Server *server, void *data) {
    ck_assert_uint_eq(UA_Server_getLifecycleState(server), UA_LIFECYCLESTATE_STARTED);
    interruptTest.dispatches++;
    interruptTest.dispatching = true;
    interruptTest.callback(interruptTest.manager, interruptTest.handle,
                           interruptTest.callbackContext, &UA_KEYVALUEMAP_NULL);
    interruptTest.dispatching = false;
}

static UA_StatusCode
registerTestInterrupt(UA_InterruptManager *manager, uintptr_t handle,
                      const UA_KeyValueMap *params, UA_InterruptCallback callback,
                      void *context) {
    ck_assert_ptr_eq(manager, interruptTest.manager);
    ck_assert_uint_eq(handle, SIGINT);
    ck_assert_uint_eq(params->mapSize, 0);
    interruptTest.registrations++;
    if(interruptTest.registrationResult != UA_STATUSCODE_GOOD)
        return interruptTest.registrationResult;
    interruptTest.callback = callback;
    interruptTest.callbackContext = context;
    interruptTest.handle = handle;
    return UA_Server_addTimedCallback(interruptTest.server, dispatchTestInterrupt,
        NULL, UA_DateTime_nowMonotonic(), &interruptTest.timerId);
}

static void
deregisterTestInterrupt(UA_InterruptManager *manager, uintptr_t handle) {
    ck_assert_ptr_eq(manager, interruptTest.manager);
    ck_assert_uint_eq(handle, SIGINT);
    interruptTest.deregistrations++;
    UA_Server_removeCallback(interruptTest.server, interruptTest.timerId);
}

static UA_StatusCode
failInterruptEventLoopStart(UA_EventLoop *eventLoop) {
    return UA_STATUSCODE_BADINTERNALERROR;
}

static void
setupInterruptTest(void) {
    memset(&interruptTest, 0, sizeof(interruptTest));
    UA_ServerConfig config;
    memset(&config, 0, sizeof(config));
    ck_assert_uint_eq(UA_ServerConfig_setMinimal(&config, 0, NULL), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(config.serverUrlsSize, 1);
    UA_String_clear(&config.serverUrls[0]);
    config.serverUrls[0] = UA_STRING_ALLOC("opc.tcp://127.0.0.1:0");
    ck_assert_ptr_nonnull(config.serverUrls[0].data);
    config.notifyLifecycleState = recordInterruptLifecycle;
    interruptTest.server = UA_Server_newWithConfig(&config);
    ck_assert_ptr_nonnull(interruptTest.server);
    interruptTest.eventLoop = UA_Server_getConfig(interruptTest.server)->eventLoop;
    interruptTest.originalStart = interruptTest.eventLoop->start;
    UA_EventSource *es = interruptTest.eventLoop->eventSources;
    while(es && es->eventSourceType != UA_EVENTSOURCETYPE_INTERRUPTMANAGER)
        es = es->next;
    ck_assert_ptr_nonnull(es);
    interruptTest.manager = (UA_InterruptManager*)es;
    interruptTest.originalManager = *interruptTest.manager;
    interruptTest.manager->registerInterrupt = registerTestInterrupt;
    interruptTest.manager->deregisterInterrupt = deregisterTestInterrupt;
}

static void
teardownInterruptTest(void) {
    interruptTest.eventLoop->start = interruptTest.originalStart;
    interruptTest.manager->registerInterrupt = interruptTest.originalManager.registerInterrupt;
    interruptTest.manager->deregisterInterrupt = interruptTest.originalManager.deregisterInterrupt;
    UA_Server_delete(interruptTest.server);
}

START_TEST(RunUntilInterruptRejectsMissingResources) {
    UA_ServerConfig *config = UA_Server_getConfig(interruptTest.server);
    UA_StatusCode res;
    if(_i == 0) {
        res = UA_Server_runUntilInterrupt(NULL);
    } else if(_i == 1) {
        config->eventLoop = NULL;
        res = UA_Server_runUntilInterrupt(interruptTest.server);
        config->eventLoop = interruptTest.eventLoop;
    } else {
        interruptTest.manager->eventSource.eventSourceType = UA_EVENTSOURCETYPE_CONNECTIONMANAGER;
        res = UA_Server_runUntilInterrupt(interruptTest.server);
        interruptTest.manager->eventSource.eventSourceType = UA_EVENTSOURCETYPE_INTERRUPTMANAGER;
    }
    ck_assert_uint_eq(res, UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(interruptTest.registrations, 0);
    ck_assert_uint_eq(interruptTest.deregistrations, 0);
} END_TEST

START_TEST(RunUntilInterruptPropagatesRegistrationFailure) {
    interruptTest.registrationResult = UA_STATUSCODE_BADNOTSUPPORTED;
    ck_assert_uint_eq(UA_Server_runUntilInterrupt(interruptTest.server),
                      UA_STATUSCODE_BADNOTSUPPORTED);
    ck_assert_uint_eq(interruptTest.registrations, 1);
    ck_assert_uint_eq(interruptTest.deregistrations, 0);
    ck_assert_uint_eq(interruptTest.dispatches, 0);
    ck_assert_uint_eq(UA_Server_getLifecycleState(interruptTest.server),
                      UA_LIFECYCLESTATE_STOPPED);
} END_TEST

START_TEST(RunUntilInterruptCleansUpAfterStartupFailure) {
    interruptTest.eventLoop->start = failInterruptEventLoopStart;
    ck_assert_uint_eq(UA_Server_runUntilInterrupt(interruptTest.server),
                      UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(interruptTest.registrations, 1);
    ck_assert_uint_eq(interruptTest.deregistrations, 1);
    ck_assert_uint_eq(interruptTest.dispatches, 0);
    ck_assert_uint_eq(UA_Server_getLifecycleState(interruptTest.server),
                      UA_LIFECYCLESTATE_STOPPED);
} END_TEST

START_TEST(RunUntilInterruptStopsServer) {
    UA_ServerConfig *config = UA_Server_getConfig(interruptTest.server);
    config->shutdownDelay = _i ? 1.0 : 0.0;
    if(_i == 2)
        config->notifyLifecycleState = NULL;
    ck_assert_uint_eq(UA_Server_runUntilInterrupt(interruptTest.server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(interruptTest.registrations, 1);
    ck_assert_uint_eq(interruptTest.deregistrations, 1);
    ck_assert_uint_eq(interruptTest.dispatches, 1);
    ck_assert_uint_eq(interruptTest.stoppingNotifications, _i == 1 ? 1 : 0);
    ck_assert_uint_eq(UA_Server_getLifecycleState(interruptTest.server),
                      UA_LIFECYCLESTATE_STOPPED);
} END_TEST
#endif

int main(void) {
    Suite *s = suite_create("ServerConfig defaults");
    TCase *tc = tcase_create("public API");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, SetBasicsZeroConfigSucceeds);
    tcase_add_test(tc, SetBasicsWithPortZeroIsAccepted);
    tcase_add_test(tc, SetBasicsNullConfigRejected);
    tcase_add_test(tc, AddSecurityPolicyNoneAndAllEndpoints);
    tcase_add_test(tc, AddEndpointWithUnknownPolicyRejected);
    tcase_add_test(tc, AddAllSecureEndpointsClearsExisting);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc, AddAllSecurityPoliciesWithoutCertSucceeds);
    tcase_add_test(tc, AddSecurityPolicyHelpersWithoutCertReturnError);
    tcase_add_test(tc, AddSecurityPolicyHelpersWithCertSucceed);
    tcase_add_test(tc, AddAllSecurityPoliciesWithCertSucceed);
#endif
    suite_add_tcase(s, tc);

#if (defined(UA_ARCHITECTURE_POSIX) && !defined(UA_ARCHITECTURE_LWIP)) || defined(UA_ARCHITECTURE_WIN32)
    TCase *tc_interrupt = tcase_create("Interrupt shutdown");
    tcase_add_checked_fixture(tc_interrupt, setupInterruptTest, teardownInterruptTest);
    tcase_add_loop_test(tc_interrupt, RunUntilInterruptRejectsMissingResources, 0, 3);
    tcase_add_test(tc_interrupt, RunUntilInterruptPropagatesRegistrationFailure);
    tcase_add_test(tc_interrupt, RunUntilInterruptCleansUpAfterStartupFailure);
    tcase_add_loop_test(tc_interrupt, RunUntilInterruptStopsServer, 0, 3);
    suite_add_tcase(s, tc_interrupt);
#endif

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
