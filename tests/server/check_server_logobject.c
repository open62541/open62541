/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/driver/logobject.h>

#include "test_helpers.h"
#include "../../drivers/logobject/logobject_internal.h"

#include <check.h>
#include <stdlib.h>
#include <string.h>

#ifdef UA_ENABLE_DRIVER_LOGOBJECT

static UA_Server *server = NULL;

static void
setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
}

/* Runs after every test, also when a test aborted on a failed assertion */
static void
teardown(void) {
    if(!server)
        return;
    if(UA_Server_getLifecycleState(server) != UA_LIFECYCLESTATE_STOPPED)
        UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    server = NULL;
}

/* --- Driver lifecycle --- */

START_TEST(driverLifecycle) {
    UA_Driver *drv = NULL;
    ck_assert_uint_eq(UA_LogObjectDriver_new(NULL, &drv), UA_STATUSCODE_GOOD);
    ck_assert(drv != NULL);
    ck_assert(isLogObjectDriver(drv));
    ck_assert_uint_eq(drv->driverType, UA_DRIVERTYPE_LOGOBJECT);
    ck_assert_uint_eq(UA_Server_addDriver(server, drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(drv->state, UA_LIFECYCLESTATE_STOPPED);

    /* Started with the server */
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(drv->state, UA_LIFECYCLESTATE_STARTED);
    ck_assert_uint_eq(drv->start(drv), UA_STATUSCODE_BADINVALIDSTATE);

    /* Stopping is idempotent, the driver can be started again */
    drv->stop(drv);
    ck_assert_uint_eq(drv->state, UA_LIFECYCLESTATE_STOPPED);
    drv->stop(drv);
    ck_assert_uint_eq(drv->start(drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(drv->state, UA_LIFECYCLESTATE_STARTED);

    /* Stopped with the server, freed with the server in the teardown */
    UA_Server_run_shutdown(server);
    ck_assert_uint_eq(drv->state, UA_LIFECYCLESTATE_STOPPED);
} END_TEST

START_TEST(onlyOneDriver) {
    UA_Driver *first = NULL;
    UA_Driver *second = NULL;
    ck_assert_uint_eq(UA_LogObjectDriver_new(NULL, &first), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_LogObjectDriver_new(NULL, &second), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_addDriver(server, first), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_addDriver(server, second), UA_STATUSCODE_BADALREADYEXISTS);
    ck_assert_uint_eq(second->free(second), UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(removeAtRuntime) {
    UA_Driver *drv = NULL;
    ck_assert_uint_eq(UA_LogObjectDriver_new(NULL, &drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_addDriver(server, drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);

    /* A started driver cannot be removed */
    ck_assert(UA_Server_removeDriver(server, drv) != UA_STATUSCODE_GOOD);
    drv->stop(drv);
    ck_assert_uint_eq(UA_Server_removeDriver(server, drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(drv->free(drv), UA_STATUSCODE_GOOD);
    UA_Server_run_iterate(server, false);
} END_TEST

/* A driver that never was added to a server can be freed */
START_TEST(freeUnregisteredDriver) {
    UA_Driver *drv = NULL;
    ck_assert_uint_eq(UA_LogObjectDriver_new(NULL, &drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(drv->free(drv), UA_STATUSCODE_GOOD);
} END_TEST

static unsigned backendClears = 0;

static void
countingClear(UA_LogObjectBackend *backend) {
    backendClears++;
    UA_LogObjectBackend inner = *(UA_LogObjectBackend*)backend->context;
    UA_free(backend->context);
    inner.clear(&inner);
    backend->context = NULL;
}

/* The driver takes over a complete backend and clears it when freed */
START_TEST(backendOwnership) {
    UA_Driver *drv = NULL;
    ck_assert_uint_eq(UA_LogObjectDriver_new(NULL, NULL), UA_STATUSCODE_BADINVALIDARGUMENT);

    /* An incomplete backend is rejected, the caller keeps it */
    UA_LogObjectBackend incomplete;
    memset(&incomplete, 0, sizeof(UA_LogObjectBackend));
    ck_assert_uint_eq(UA_LogObjectDriver_new(&incomplete, &drv),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert(drv == NULL);

    /* Wrap the memory backend to count the clear */
    UA_LogObjectBackend inner;
    ck_assert_uint_eq(UA_LogObjectBackend_memory(&inner), UA_STATUSCODE_GOOD);
    UA_LogObjectBackend wrapper = inner;
    wrapper.context = UA_malloc(sizeof(UA_LogObjectBackend));
    ck_assert(wrapper.context != NULL);
    *(UA_LogObjectBackend*)wrapper.context = inner;
    wrapper.clear = countingClear;
    backendClears = 0;
    ck_assert_uint_eq(UA_LogObjectDriver_new(&wrapper, &drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(drv->free(drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(backendClears, 1);
} END_TEST

int main(void) {
    Suite *s = suite_create("LogObject driver");

    TCase *tc_life = tcase_create("Lifecycle");
    tcase_add_checked_fixture(tc_life, setup, teardown);
    tcase_add_test(tc_life, driverLifecycle);
    tcase_add_test(tc_life, onlyOneDriver);
    tcase_add_test(tc_life, removeAtRuntime);
    tcase_add_test(tc_life, freeUnregisteredDriver);
    tcase_add_test(tc_life, backendOwnership);
    suite_add_tcase(s, tc_life);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

#else /* UA_ENABLE_DRIVER_LOGOBJECT */

int main(void) { return EXIT_SUCCESS; }

#endif
