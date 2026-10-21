/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define UA_DiscoveryManager_mdnsCyclicTimer test_UA_DiscoveryManager_mdnsCyclicTimer
/* Compile a private copy to exercise static allocation error paths. */
#define UA_DiscoveryManager_addEntryToServersOnNetwork test_UA_DiscoveryManager_addEntryToServersOnNetwork
#define UA_DiscoveryManager_addServerOnNetworkRecord test_UA_DiscoveryManager_addServerOnNetworkRecord
#define UA_DiscoveryManager_clearMdns test_UA_DiscoveryManager_clearMdns
#define UA_DiscoveryManager_clearServerOnNetwork test_UA_DiscoveryManager_clearServerOnNetwork
#define UA_DiscoveryManager_getMdnsConnectionCount test_UA_DiscoveryManager_getMdnsConnectionCount
#define UA_DiscoveryManager_getNextServerOnNetworkRecord test_UA_DiscoveryManager_getNextServerOnNetworkRecord
#define UA_DiscoveryManager_getServerOnNetworkCounterResetTime test_UA_DiscoveryManager_getServerOnNetworkCounterResetTime
#define UA_DiscoveryManager_getServerOnNetworkList test_UA_DiscoveryManager_getServerOnNetworkList
#define UA_DiscoveryManager_getServerOnNetworkRecordIdCounter test_UA_DiscoveryManager_getServerOnNetworkRecordIdCounter
#define UA_DiscoveryManager_removeEntryFromServersOnNetwork test_UA_DiscoveryManager_removeEntryFromServersOnNetwork
#define UA_DiscoveryManager_resetServerOnNetworkRecordCounter test_UA_DiscoveryManager_resetServerOnNetworkRecordCounter
#define UA_DiscoveryManager_sendMulticastMessages test_UA_DiscoveryManager_sendMulticastMessages
#define UA_DiscoveryManager_startMulticast test_UA_DiscoveryManager_startMulticast
#define UA_DiscoveryManager_stopMulticast test_UA_DiscoveryManager_stopMulticast
#define UA_Discovery_addRecord test_UA_Discovery_addRecord
#define UA_Discovery_multicastConflict test_UA_Discovery_multicastConflict
#define UA_Discovery_recordExists test_UA_Discovery_recordExists
#define UA_Discovery_removeRecord test_UA_Discovery_removeRecord
#define UA_Discovery_updateMdnsForDiscoveryUrl test_UA_Discovery_updateMdnsForDiscoveryUrl
#define UA_Server_setServerOnNetworkCallback test_UA_Server_setServerOnNetworkCallback
#include "server/ua_discovery_mdns.c"

#include <open62541/server_config_default.h>

#include "server/ua_server_internal.h"
#include "server/ua_discovery.h"

#include <check.h>
#include <stdlib.h>

/* Allocation failures in the mDNS record handling. A single allocation of a
 * given size is made to fail. The allocation counters detect leaks of the
 * partially built state on the error paths. A missing NULL check results in a
 * crash. */

static UA_Server *server;
static UA_DiscoveryManager *dm;

/* Number of live allocations and the allocation to fail (at most one at a
 * time) */
static long liveAllocations;
static size_t failMallocSize;
static size_t failCallocNelem;
static size_t failCallocElsize;
static UA_Boolean failureTriggered;

static void *
testMalloc(size_t size) {
    if(failMallocSize > 0 && size == failMallocSize) {
        failMallocSize = 0;
        failureTriggered = true;
        return NULL;
    }
    void *p = malloc(size);
    if(p)
        liveAllocations++;
    return p;
}

static void *
testCalloc(size_t nelem, size_t elsize) {
    if(failCallocNelem > 0 && nelem == failCallocNelem &&
       elsize == failCallocElsize) {
        failCallocNelem = 0;
        failureTriggered = true;
        return NULL;
    }
    void *p = calloc(nelem, elsize);
    if(p)
        liveAllocations++;
    return p;
}

static void *
testRealloc(void *ptr, size_t size) {
    void *p = realloc(ptr, size);
    if(p && !ptr)
        liveAllocations++;
    return p;
}

static void
testFree(void *ptr) {
    if(ptr)
        liveAllocations--;
    free(ptr);
}

static void
setup(void) {
    liveAllocations = 0;
    failMallocSize = 0;
    failCallocNelem = 0;
    failureTriggered = false;
    UA_mallocSingleton = testMalloc;
    UA_callocSingleton = testCalloc;
    UA_reallocSingleton = testRealloc;
    UA_freeSingleton = testFree;

    server = UA_Server_new();
    ck_assert_ptr_ne(server, NULL);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    ck_assert_uint_eq(UA_ServerConfig_setDefault(config), UA_STATUSCODE_GOOD);
    dm = (UA_DiscoveryManager*)getServerComponentByName(server, UA_STRING("discovery"));
    ck_assert_ptr_ne(dm, NULL);
    dm->sc.server = server;
    memset(&mdnsPrivateData, 0, sizeof(mdnsPrivateData));
    LIST_INIT(&mdnsPrivateData.serverOnNetwork);
}

static void
teardown(void) {
    UA_DiscoveryManager_clearMdns(dm);
    UA_Server_delete(server);
    server = NULL;
    UA_mallocSingleton = malloc;
    UA_callocSingleton = calloc;
    UA_reallocSingleton = realloc;
    UA_freeSingleton = free;
    ck_assert_int_eq(liveAllocations, 0);
}

/* Put a TXT or SRV record for "<serverName>._opcua-tcp._tcp.local." through the
 * mDNS receive callback */
static const char *srvHost = "myhost.local.";
static const char *srvUrl = "opc.tcp://myhost.local:4840";

static void
receiveTxt(const char *name, const char *path, const char *caps) {
    unsigned char rdata[256];
    size_t pos = 0;
    if(path) {
        size_t len = strlen("path=") + strlen(path);
        rdata[pos++] = (unsigned char)len;
        memcpy(&rdata[pos], "path=", 5);
        memcpy(&rdata[pos + 5], path, strlen(path));
        pos += len;
    }
    if(caps) {
        size_t len = strlen("caps=") + strlen(caps);
        rdata[pos++] = (unsigned char)len;
        memcpy(&rdata[pos], "caps=", 5);
        memcpy(&rdata[pos + 5], caps, strlen(caps));
        pos += len;
    }

    struct resource r;
    memset(&r, 0, sizeof(r));
    r.name = (char*)(uintptr_t)name;
    r.type = QTYPE_TXT;
    r.clazz = QCLASS_IN;
    r.ttl = 120;
    r.rdlength = (unsigned short)pos;
    r.rdata = rdata;
    mdns_record_received(&r, dm);
}

static void
receiveSrv(const char *name) {
    struct resource r;
    memset(&r, 0, sizeof(r));
    r.name = (char*)(uintptr_t)name;
    r.type = QTYPE_SRV;
    r.clazz = QCLASS_IN;
    r.ttl = 120;
    r.known.srv.port = 4840;
    r.known.srv.name = (char*)(uintptr_t)srvHost;
    mdns_record_received(&r, dm);
}

static serverOnNetwork *
firstEntry(void) {
    serverOnNetwork *entry =
        LIST_FIRST(&mdnsPrivateData.serverOnNetwork);
    ck_assert_ptr_ne(entry, NULL);
    return entry;
}

START_TEST(txtCapabilitiesArrayAllocation) {
    failCallocNelem = 2;
    failCallocElsize = sizeof(UA_String);
    receiveTxt("srv._opcua-tcp._tcp.local.", NULL, "LDS,ABCDEFG");
    ck_assert(failureTriggered);

    serverOnNetwork *entry = firstEntry();
    ck_assert(entry->txtSet);
    ck_assert_ptr_eq(entry->serverOnNetwork.serverCapabilities, NULL);
    ck_assert_uint_eq(entry->serverOnNetwork.serverCapabilitiesSize, 0);
}
END_TEST

START_TEST(txtCapabilityAllocation) {
    /* Fail the allocation of the second capability. The first one has to be
     * cleaned up again. */
    failMallocSize = 7;
    receiveTxt("srv._opcua-tcp._tcp.local.", NULL, "LDS,ABCDEFG");
    ck_assert(failureTriggered);

    serverOnNetwork *entry = firstEntry();
    ck_assert(entry->txtSet);
    ck_assert_ptr_eq(entry->serverOnNetwork.serverCapabilities, NULL);
    ck_assert_uint_eq(entry->serverOnNetwork.serverCapabilitiesSize, 0);
}
END_TEST

START_TEST(txtAppendPathToDiscoveryUrl) {
    receiveSrv("srv._opcua-tcp._tcp.local.");
    serverOnNetwork *entry = firstEntry();
    ck_assert(entry->srvSet);
    UA_String expected = UA_STRING((char*)(uintptr_t)srvUrl);
    ck_assert(UA_String_equal(&entry->serverOnNetwork.discoveryUrl, &expected));

    /* The discovery url stays as it is */
    failMallocSize = strlen(srvUrl) + strlen("/discovery");
    receiveTxt("srv._opcua-tcp._tcp.local.", "/discovery", NULL);
    ck_assert(failureTriggered);
    ck_assert(entry->txtSet);
    ck_assert(UA_String_equal(&entry->serverOnNetwork.discoveryUrl, &expected));
}
END_TEST

START_TEST(srvAppendPathToDiscoveryUrl) {
    receiveTxt("srv._opcua-tcp._tcp.local.", "/discovery", NULL);
    serverOnNetwork *entry = firstEntry();
    ck_assert(entry->txtSet);
    ck_assert_ptr_ne(entry->pathTmp, NULL);

    /* The discovery url is set without the path */
    failMallocSize = strlen(srvUrl) + strlen("/discovery");
    receiveSrv("srv._opcua-tcp._tcp.local.");
    ck_assert(failureTriggered);
    ck_assert(entry->srvSet);
    ck_assert_ptr_eq(entry->pathTmp, NULL);
    UA_String expected = UA_STRING((char*)(uintptr_t)srvUrl);
    ck_assert(UA_String_equal(&entry->serverOnNetwork.discoveryUrl, &expected));
}
END_TEST

START_TEST(createTxtCapabilitiesAllocation) {
    /* mdns_create_txt needs a daemon, but no network */
    mdnsPrivateData.mdnsDaemon = mdnsd_new(QCLASS_IN, 1000);
    ck_assert_ptr_ne(mdnsPrivateData.mdnsDaemon, NULL);

    UA_String caps[2] = {UA_STRING_STATIC("LDS"), UA_STRING_STATIC("ABCDEFGH")};
    failMallocSize = 4 + 9; /* Capabilities including the separators */
    mdns_create_txt(dm, "srv._opcua-tcp._tcp.local.", "/discovery",
                    caps, 2, NULL);
    ck_assert(failureTriggered);

    mdnsd_shutdown(mdnsPrivateData.mdnsDaemon);
    mdnsd_free(mdnsPrivateData.mdnsDaemon);
    mdnsPrivateData.mdnsDaemon = NULL;
}
END_TEST

static Suite *
testSuite(void) {
    Suite *s = suite_create("Discovery mDNS allocation failures");
    TCase *tc = tcase_create("mDNS records");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, txtCapabilitiesArrayAllocation);
    tcase_add_test(tc, txtCapabilityAllocation);
    tcase_add_test(tc, txtAppendPathToDiscoveryUrl);
    tcase_add_test(tc, srvAppendPathToDiscoveryUrl);
    tcase_add_test(tc, createTxtCapabilitiesAllocation);
    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    Suite *s = testSuite();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
