/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Exercise the driver's static parsers without opening multicast sockets. */
#define UA_MdnsDriver_Mdnsd test_UA_MdnsDriver_Mdnsd
#include "../../drivers/discovery_mdns_mdnsd.c"

#include <check.h>
#include <stdlib.h>

static size_t failSize;
static UA_Boolean failureTriggered;
static void *
failMalloc(size_t size) {
    if(size == failSize) {
        failSize = 0;
        failureTriggered = true;
        return NULL;
    }
    return malloc(size);
}
static void *
failCalloc(size_t count, size_t size) {
    if(count * size == failSize) {
        failSize = 0;
        failureTriggered = true;
        return NULL;
    }
    return calloc(count, size);
}

START_TEST(txtAllocationFailures) {
    /* Fail the path, the capabilities array, or its second string. */
    const size_t sizes[] = {10, 2 * sizeof(UA_String), 7};
    unsigned char txt[] = "\x0f" "path=/discovery" "\x10" "caps=LDS,ABCDEFG";
    struct resource r;
    memset(&r, 0, sizeof(r));
    r.rdata = txt;
    r.rdlength = sizeof(txt) - 1;
    ServerOnNetworkRecord entry;
    memset(&entry, 0, sizeof(entry));
    failSize = sizes[_i];
    failureTriggered = false;
    UA_mallocSingleton = failMalloc;
    UA_callocSingleton = failCalloc;
    UA_StatusCode res = processTxt(&entry, &r);
    UA_mallocSingleton = malloc;
    UA_callocSingleton = calloc;
    ck_assert(failureTriggered);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADOUTOFMEMORY);
    ck_assert(!entry.txtSet);
    ck_assert_ptr_null(entry.serverOnNetwork.serverCapabilities);
    ck_assert_uint_eq(entry.serverOnNetwork.serverCapabilitiesSize, 0);
    UA_String_clear(&entry.path);
    UA_ServerOnNetwork_clear(&entry.serverOnNetwork);
} END_TEST

START_TEST(discoveryUrlAllocationFailure) {
    ServerOnNetworkRecord entry;
    memset(&entry, 0, sizeof(entry));
    entry.srvSet = true;
    entry.srvHostname = UA_STRING("host.local");
    entry.srvPort = 4840;
    entry.path = UA_STRING("/discovery");
    entry.serverOnNetwork.discoveryUrl = UA_STRING_ALLOC("opc.tcp://old:4840");
    failSize = strlen("opc.tcp://host.local:4840/discovery") + 1;
    failureTriggered = false;
    UA_mallocSingleton = failMalloc;
    UA_callocSingleton = failCalloc;
    UA_StatusCode res = updateDiscoveryUrl(&entry);
    UA_callocSingleton = calloc;
    UA_mallocSingleton = malloc;
    ck_assert(failureTriggered);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADOUTOFMEMORY);
    UA_String expected = UA_STRING("opc.tcp://old:4840");
    ck_assert(UA_String_equal(&entry.serverOnNetwork.discoveryUrl, &expected));
    UA_String_clear(&entry.serverOnNetwork.discoveryUrl);
} END_TEST

int main(void) {
    Suite *s = suite_create("mDNS allocation failures");
    TCase *tc = tcase_create("Driver parsers");
    tcase_add_loop_test(tc, txtAllocationFailures, 0, 3);
    tcase_add_test(tc, discoveryUrlAllocationFailure);
    suite_add_tcase(s, tc);
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
