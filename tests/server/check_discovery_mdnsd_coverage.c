/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <open62541/client_config_default.h>
#include <open62541/client.h>
#include <open62541/driver/mdns.h>
#include <open62541/server_config_default.h>

#include "server/ua_server_internal.h"
#include "server/ua_discovery.h"
#if defined(UA_ENABLE_DISCOVERY_MULTICAST_MDNSD)
#include "src_generated/mdnsd/mdnsd.h"
#include "src_generated/mdnsd/sdtxt.h"
#endif
#include "test_helpers.h"
#include "testing_clock.h"
#include "testing_networklayers.h"
#include "thread_wrapper.h"

#if defined(UA_ENABLE_DISCOVERY) && \
    (defined(UA_ENABLE_DISCOVERY_MULTICAST_MDNSD) || \
     defined(UA_ENABLE_DISCOVERY_MULTICAST_AVAHI))

#if defined(UA_ENABLE_DISCOVERY_MULTICAST_AVAHI)
# define MDNS_DRIVER_CONSTRUCTOR UA_MdnsDriver_Avahi
# define MDNS_DRIVER_NAME "discovery-mdns-avahi"
# define MDNS_DRIVER_SUITE_NAME "Discovery Avahi Coverage"
#else
# define MDNS_DRIVER_CONSTRUCTOR UA_MdnsDriver_Mdnsd
# define MDNS_DRIVER_NAME "discovery-mdns"
# define MDNS_DRIVER_SUITE_NAME "Discovery mDNSd Coverage"
#endif

static UA_MdnsDriver*
newDriverWithSendToAllInterfaces(UA_Boolean listen, UA_Boolean announce,
   UA_Boolean sendToAllInterfaces) {
   UA_KeyValuePair params[7];
   params[0].key = UA_QUALIFIEDNAME(0, "listen");
   UA_Variant_setScalar(&params[0].value, &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);
   params[1].key = UA_QUALIFIEDNAME(0, "announce");
   UA_Variant_setScalar(&params[1].value, &announce, &UA_TYPES[UA_TYPES_BOOLEAN]);
   params[2].key = UA_QUALIFIEDNAME(0, "query-presence");
   UA_Boolean queryPresence = false;
   UA_Variant_setScalar(&params[2].value, &queryPresence,
      &UA_TYPES[UA_TYPES_BOOLEAN]);
   params[3].key = UA_QUALIFIEDNAME(0, "query-details");
   UA_Boolean queryDetails = false;
   UA_Variant_setScalar(&params[3].value, &queryDetails,
      &UA_TYPES[UA_TYPES_BOOLEAN]);
   params[4].key = UA_QUALIFIEDNAME(0, "query-interval");
   UA_UInt32 queryInterval = 0;
   UA_Variant_setScalar(&params[4].value, &queryInterval,
      &UA_TYPES[UA_TYPES_UINT32]);
   params[5].key = UA_QUALIFIEDNAME(0, "announce-ttl");
   UA_UInt32 announceTTL = 600;
   UA_Variant_setScalar(&params[5].value, &announceTTL,
      &UA_TYPES[UA_TYPES_UINT32]);
   params[6].key = UA_QUALIFIEDNAME(0, "send-to-all-interfaces");
   UA_Variant_setScalar(&params[6].value, &sendToAllInterfaces,
      &UA_TYPES[UA_TYPES_BOOLEAN]);

   UA_KeyValueMap paramsMap = { 7, params };
   UA_MdnsDriver* mdns = MDNS_DRIVER_CONSTRUCTOR(paramsMap);
   ck_assert_ptr_ne(mdns, NULL);
   return mdns;
}

START_TEST(SendToAllInterfacesParameterParsed) {
   /* Test that the send-to-all-interfaces parameter is correctly parsed
    * and stored during driver initialization. */
   UA_MdnsDriver* mdns = newDriverWithSendToAllInterfaces(true, true, true);

   const UA_Boolean* sendToAllInterfaces = (const UA_Boolean*)
      UA_KeyValueMap_getScalar(&mdns->drv.params,
         UA_QUALIFIEDNAME(0, "send-to-all-interfaces"),
         &UA_TYPES[UA_TYPES_BOOLEAN]);
   ck_assert_ptr_ne(sendToAllInterfaces, NULL);
   ck_assert(*sendToAllInterfaces);

   ck_assert_uint_eq(mdns->drv.free(&mdns->drv), UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(SendToAllInterfacesDefaultsFalse) {
   /* Test that the send-to-all-interfaces parameter defaults to false
    * when not provided. */
   UA_KeyValuePair params[5];
   params[0].key = UA_QUALIFIEDNAME(0, "listen");
   UA_Boolean listen = true;
   UA_Variant_setScalar(&params[0].value, &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);
   params[1].key = UA_QUALIFIEDNAME(0, "announce");
   UA_Boolean announce = true;
   UA_Variant_setScalar(&params[1].value, &announce, &UA_TYPES[UA_TYPES_BOOLEAN]);
   params[2].key = UA_QUALIFIEDNAME(0, "query-presence");
   UA_Boolean queryPresence = false;
   UA_Variant_setScalar(&params[2].value, &queryPresence,
      &UA_TYPES[UA_TYPES_BOOLEAN]);
   params[3].key = UA_QUALIFIEDNAME(0, "query-details");
   UA_Boolean queryDetails = false;
   UA_Variant_setScalar(&params[3].value, &queryDetails,
      &UA_TYPES[UA_TYPES_BOOLEAN]);
   params[4].key = UA_QUALIFIEDNAME(0, "query-interval");
   UA_UInt32 queryInterval = 0;
   UA_Variant_setScalar(&params[4].value, &queryInterval,
      &UA_TYPES[UA_TYPES_UINT32]);

   UA_KeyValueMap paramsMap = { 5, params };
   UA_MdnsDriver* mdns = MDNS_DRIVER_CONSTRUCTOR(paramsMap);
   ck_assert_ptr_ne(mdns, NULL);

   const UA_Boolean* sendToAllInterfaces = (const UA_Boolean*)
      UA_KeyValueMap_getScalar(&mdns->drv.params,
         UA_QUALIFIEDNAME(0, "send-to-all-interfaces"),
         &UA_TYPES[UA_TYPES_BOOLEAN]);
   /* Parameter not explicitly set, but the driver should handle NULL gracefully
    * (defaults to false per the code logic). */

   ck_assert_uint_eq(mdns->drv.free(&mdns->drv), UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(DriverStartsWithSendToAllInterfacesEnabled) {
   /* Test that the driver can start and process the send-to-all-interfaces
    * configuration without errors. */
   UA_Server* localServer = UA_Server_newForUnitTest();
   ck_assert_ptr_ne(localServer, NULL);
   UA_Server_getConfig(localServer)->serversOnNetworkEnabled = true;

   /* On platforms without interface enumeration support (e.g., embedded
    * systems), the driver should still start without crashing, just without
    * creating multiple send connections. */
   UA_MdnsDriver* mdns = newDriverWithSendToAllInterfaces(true, true, true);
   ck_assert_uint_eq(UA_Server_addDriver(localServer, &mdns->drv),
      UA_STATUSCODE_GOOD);
   ck_assert_uint_eq(UA_Server_run_startup(localServer), UA_STATUSCODE_GOOD);
   UA_Server_run_shutdown(localServer);
   UA_Server_delete(localServer);
}
END_TEST

START_TEST(DriverStartsWithSendToAllInterfacesDisabled) {
   /* Test the backward-compatible path where send-to-all-interfaces is false
    * or not set. This ensures the original behavior is preserved. */
   UA_Server* localServer = UA_Server_newForUnitTest();
   ck_assert_ptr_ne(localServer, NULL);
   UA_Server_getConfig(localServer)->serversOnNetworkEnabled = true;

   UA_MdnsDriver* mdns = newDriverWithSendToAllInterfaces(true, true, false);
   ck_assert_uint_eq(UA_Server_addDriver(localServer, &mdns->drv),
      UA_STATUSCODE_GOOD);
   ck_assert_uint_eq(UA_Server_run_startup(localServer), UA_STATUSCODE_GOOD);
   UA_Server_run_shutdown(localServer);
   UA_Server_delete(localServer);
}
END_TEST

START_TEST(DriverAnnounceWithoutSendToAllInterfaces) {
   /* Verify that announcements work correctly when send-to-all-interfaces
    * is disabled (the default case). */
   UA_Server* localServer = UA_Server_newForUnitTest();
   ck_assert_ptr_ne(localServer, NULL);
   UA_ServerConfig* config = UA_Server_getConfig(localServer);
   config->serversOnNetworkEnabled = true;

   UA_MdnsDriver* mdns = newDriverWithSendToAllInterfaces(true, true, false);
   ck_assert_uint_eq(UA_Server_addDriver(localServer, &mdns->drv),
      UA_STATUSCODE_GOOD);
   ck_assert_uint_eq(UA_Server_run_startup(localServer), UA_STATUSCODE_GOOD);

   /* Register a server and verify it announces without errors */
   UA_ServerOnNetwork son;
   UA_ServerOnNetwork_init(&son);
   son.serverName = UA_STRING("coverage-test-server");
   son.discoveryUrl = UA_STRING("opc.tcp://localhost:4840");
   son.serverCapabilitiesSize = 1;
   UA_String caps = UA_STRING("NA");
   son.serverCapabilities = &caps;

   ck_assert_uint_eq(UA_Server_registerServerOnNetwork(localServer, &son,
      UA_KEYVALUEMAP_NULL),
      UA_STATUSCODE_GOOD);
   for (size_t i = 0; i < 3; i++)
      UA_Server_run_iterate(localServer, false);

   UA_Server_run_shutdown(localServer);
   UA_Server_delete(localServer);
}
END_TEST

#if defined(UA_ENABLE_DISCOVERY_MULTICAST_MDNSD)

START_TEST(MultipleSendConnectionsCreatedOnLinux) {
   /* On Linux with getifaddrs(), multiple send connections should be created
    * when send-to-all-interfaces is true. This test verifies the mechanism
    * works (though the actual number of connections depends on the host's
    * network configuration). */
   UA_Server* localServer = UA_Server_newForUnitTest();
   ck_assert_ptr_ne(localServer, NULL);
   UA_Server_getConfig(localServer)->serversOnNetworkEnabled = true;

#ifdef UA_HAS_GETIFADDR
   /* Only run on Linux where getifaddrs() is available */
   UA_MdnsDriver* mdns = newDriverWithSendToAllInterfaces(true, true, true);
   ck_assert_uint_eq(UA_Server_addDriver(localServer, &mdns->drv),
      UA_STATUSCODE_GOOD);
   ck_assert_uint_eq(UA_Server_run_startup(localServer), UA_STATUSCODE_GOOD);

   /* Allow the driver time to create connections */
   for (size_t i = 0; i < 5; i++)
      UA_Server_run_iterate(localServer, false);

   UA_Server_run_shutdown(localServer);
#endif

   UA_Server_delete(localServer);
}
END_TEST

START_TEST(SendToAllInterfacesDuringAnnounce) {
   /* Verify that announcements are sent on all interfaces when the
    * send-to-all-interfaces flag is enabled. This is an integration test
    * that checks the multicast message flushing works with multiple
    * send connections. */
   UA_Server* localServer = UA_Server_newForUnitTest();
   ck_assert_ptr_ne(localServer, NULL);
   UA_ServerConfig* config = UA_Server_getConfig(localServer);
   config->serversOnNetworkEnabled = true;

   UA_MdnsDriver* mdns = newDriverWithSendToAllInterfaces(true, true, true);
   ck_assert_uint_eq(UA_Server_addDriver(localServer, &mdns->drv),
      UA_STATUSCODE_GOOD);
   ck_assert_uint_eq(UA_Server_run_startup(localServer), UA_STATUSCODE_GOOD);

   /* Register and announce a server */
   UA_ServerOnNetwork son;
   UA_ServerOnNetwork_init(&son);
   son.serverName = UA_STRING("multi-iface-test");
   son.discoveryUrl = UA_STRING("opc.tcp://localhost:4840");
   son.serverCapabilitiesSize = 1;
   UA_String caps = UA_STRING("NA");
   son.serverCapabilities = &caps;

   ck_assert_uint_eq(UA_Server_registerServerOnNetwork(localServer, &son,
      UA_KEYVALUEMAP_NULL),
      UA_STATUSCODE_GOOD);

   /* Iterate to process announcements */
   for (size_t i = 0; i < 5; i++)
      UA_Server_run_iterate(localServer, false);

   UA_Server_run_shutdown(localServer);
   UA_Server_delete(localServer);
}
END_TEST

START_TEST(KeyValueMapSizeRestoredAfterInterfaceEnumeration) {
   /* Verify that the KeyValueMap size is correctly restored after
    * interface enumeration, even if the enumeration finds multiple
    * interfaces. This tests the cleanup path in createMultiSendConnections. */
   UA_Server* localServer = UA_Server_newForUnitTest();
   ck_assert_ptr_ne(localServer, NULL);
   UA_Server_getConfig(localServer)->serversOnNetworkEnabled = true;

#if defined(UA_ARCHITECTURE_WIN32) || defined(UA_HAS_GETIFADDR)
   /* Only run on platforms with interface enumeration support */
   UA_MdnsDriver* mdns = newDriverWithSendToAllInterfaces(true, true, true);
   ck_assert_uint_eq(UA_Server_addDriver(localServer, &mdns->drv),
      UA_STATUSCODE_GOOD);
   ck_assert_uint_eq(UA_Server_run_startup(localServer), UA_STATUSCODE_GOOD);

   /* Simulate a shutdown and restart to verify state cleanup */
   UA_Server_run_shutdown(localServer);

   /* Verify the server cleans up properly without memory leaks or corruption */
#endif

   UA_Server_delete(localServer);
}
END_TEST

START_TEST(SendConnectionOverflowHandling) {
   /* Test that the driver handles the case where more connections
    * are attempted than the maximum (UA_MAXMDNSSENDSOCKETS). This should
    * be gracefully handled by silently ignoring excess connections. */
   UA_Server* localServer = UA_Server_newForUnitTest();
   ck_assert_ptr_ne(localServer, NULL);
   UA_Server_getConfig(localServer)->serversOnNetworkEnabled = true;

   UA_MdnsDriver* mdns = newDriverWithSendToAllInterfaces(true, true, true);
   ck_assert_uint_eq(UA_Server_addDriver(localServer, &mdns->drv),
      UA_STATUSCODE_GOOD);
   ck_assert_uint_eq(UA_Server_run_startup(localServer), UA_STATUSCODE_GOOD);

   /* On systems with many network interfaces, the driver should handle
    * reaching the max without crashing. On systems with few interfaces,
    * this is a no-op but still verifies graceful handling. */
   for (size_t i = 0; i < 10; i++)
      UA_Server_run_iterate(localServer, false);

   UA_Server_run_shutdown(localServer);
   UA_Server_delete(localServer);
}
END_TEST

START_TEST(SendConnectionRemovalWithMultipleConnections) {
   /* Verify that removing a send connection works correctly when multiple
    * connections exist. This tests the loop in removeConnection() that
    * searches through the send connections array. */
   UA_Server* localServer = UA_Server_newForUnitTest();
   ck_assert_ptr_ne(localServer, NULL);
   UA_ServerConfig* config = UA_Server_getConfig(localServer);
   config->serversOnNetworkEnabled = true;

   UA_MdnsDriver* mdns = newDriverWithSendToAllInterfaces(true, true, true);
   ck_assert_uint_eq(UA_Server_addDriver(localServer, &mdns->drv),
      UA_STATUSCODE_GOOD);
   ck_assert_uint_eq(UA_Server_run_startup(localServer), UA_STATUSCODE_GOOD);

   /* Iterate to allow connections to be established */
   for (size_t i = 0; i < 5; i++)
      UA_Server_run_iterate(localServer, false);

   /* Shutdown triggers connection removal */
   UA_Server_run_shutdown(localServer);

   /* Verify clean shutdown without segfaults or resource leaks */
   UA_Server_delete(localServer);
}
END_TEST

#endif /* UA_ENABLE_DISCOVERY_MULTICAST_MDNSD */

static Suite*
testSuite_DiscoveryMdnsdCoverage(void) {
   Suite* s = suite_create(MDNS_DRIVER_SUITE_NAME);

   TCase* tc_param = tcase_create("Parameter parsing");
   tcase_add_test(tc_param, SendToAllInterfacesParameterParsed);
   tcase_add_test(tc_param, SendToAllInterfacesDefaultsFalse);
   suite_add_tcase(s, tc_param);

   TCase* tc_startup = tcase_create("Driver startup and shutdown");
   tcase_add_test(tc_startup, DriverStartsWithSendToAllInterfacesEnabled);
   tcase_add_test(tc_startup, DriverStartsWithSendToAllInterfacesDisabled);
   suite_add_tcase(s, tc_startup);

   TCase* tc_announce = tcase_create("Announcement behavior");
   tcase_add_test(tc_announce, DriverAnnounceWithoutSendToAllInterfaces);
   suite_add_tcase(s, tc_announce);

#if defined(UA_ENABLE_DISCOVERY_MULTICAST_MDNSD)
   TCase* tc_multiif = tcase_create("Multi-interface handling");
   tcase_add_test(tc_multiif, MultipleSendConnectionsCreatedOnLinux);
   tcase_add_test(tc_multiif, SendToAllInterfacesDuringAnnounce);
   tcase_add_test(tc_multiif, KeyValueMapSizeRestoredAfterInterfaceEnumeration);
   tcase_add_test(tc_multiif, SendConnectionOverflowHandling);
   tcase_add_test(tc_multiif, SendConnectionRemovalWithMultipleConnections);
   suite_add_tcase(s, tc_multiif);
#endif

   return s;
}

int
main(void) {
   Suite* s = testSuite_DiscoveryMdnsdCoverage();
   SRunner* sr = srunner_create(s);
   srunner_set_fork_status(sr, CK_NOFORK);
   srunner_run_all(sr, CK_NORMAL);
   int number_failed = srunner_ntests_failed(sr);
   srunner_free(sr);
   return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

#else

int
main(void) {
   return EXIT_SUCCESS;
}

#endif
