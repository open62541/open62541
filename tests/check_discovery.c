/* This Source Code Form is subject to the terms of the Mozilla Public
*  License, v. 2.0. If a copy of the MPL was not distributed with this
*  file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _XOPEN_SOURCE
# define _XOPEN_SOURCE 500
#endif
#ifndef _DEFAULT_SOURCE
# define _DEFAULT_SOURCE
#endif

// On older systems we need to define _BSD_SOURCE
// _DEFAULT_SOURCE is an alias for that
#ifndef _BSD_SOURCE
# define _BSD_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <ua_util.h>
#include <ua_types_generated.h>
#include <server/ua_server_internal.h>
#include <ua_types.h>
#include <fcntl.h>

#include "ua_client.h"
#include "ua_config_standard.h"
#include "ua_network_tcp.h"
#include "check.h"
#include "testing_clock.h"

// set register timeout to 1 second so we are able to test it.
#define registerTimeout 1
// cleanup is only triggered every 10 seconds, thus wait a bit longer to check
#define checkWait registerTimeout + 11

UA_Server *server_lds;
UA_ServerConfig *config_lds;
UA_Boolean *running_lds;
pthread_t server_thread_lds;

static void * serverloop_lds(void *_) {
    while(*running_lds)
        UA_Server_run_iterate(server_lds, true);
    return NULL;
}

static void setup_lds(void) {
    // start LDS server
    running_lds = UA_Boolean_new();
    *running_lds = true;
    config_lds = UA_ServerConfig_new_default();
    config_lds->applicationDescription.applicationType = UA_APPLICATIONTYPE_DISCOVERYSERVER;
    UA_String_deleteMembers(&config_lds->applicationDescription.applicationUri);
    config_lds->applicationDescription.applicationUri =
        UA_STRING_ALLOC("urn:open62541.test.local_discovery_server");
    UA_LocalizedText_deleteMembers(&config_lds->applicationDescription.applicationName);
    config_lds->applicationDescription.applicationName
        = UA_LOCALIZEDTEXT_ALLOC("en", "LDS Server");
    config_lds->mdnsServerName = UA_String_fromChars("LDS_test");
    config_lds->serverCapabilitiesSize = 1;
    UA_String *caps = UA_String_new();
    *caps = UA_String_fromChars("LDS");
    config_lds->serverCapabilities = caps;
    config_lds->discoveryCleanupTimeout = registerTimeout;
    server_lds = UA_Server_new(config_lds);
    UA_Server_run_startup(server_lds);
    pthread_create(&server_thread_lds, NULL, serverloop_lds, NULL);

    // wait until LDS started
    UA_sleep(1000);
    sleep(1);
}

static void teardown_lds(void) {
    *running_lds = false;
    pthread_join(server_thread_lds, NULL);
    UA_Server_run_shutdown(server_lds);
    UA_Boolean_delete(running_lds);
    UA_Server_delete(server_lds);
    UA_ServerConfig_delete(config_lds);
}

UA_Server *server_register;
UA_ServerConfig *config_register;
UA_Boolean *running_register;
pthread_t server_thread_register;

UA_UInt64 periodicRegisterCallbackId;

static void * serverloop_register(void *_) {
    while(*running_register)
        UA_Server_run_iterate(server_register, true);
    return NULL;
}

static void setup_register(void) {
    // start register server
    running_register = UA_Boolean_new();
    *running_register = true;
    config_register = UA_ServerConfig_new_minimal(16664, NULL);
    UA_String_deleteMembers(&config_register->applicationDescription.applicationUri);
    config_register->applicationDescription.applicationUri =
        UA_String_fromChars("urn:open62541.test.server_register");
    UA_LocalizedText_deleteMembers(&config_register->applicationDescription.applicationName);
    config_register->applicationDescription.applicationName =
        UA_LOCALIZEDTEXT_ALLOC("de", "Anmeldungsserver");
    config_register->mdnsServerName = UA_String_fromChars("Register_test");
    server_register = UA_Server_new(config_register);
    UA_Server_run_startup(server_register);
    pthread_create(&server_thread_register, NULL, serverloop_register, NULL);
}

static void teardown_register(void) {
    *running_register = false;
    pthread_join(server_thread_register, NULL);
    UA_Server_run_shutdown(server_register);
    UA_Boolean_delete(running_register);
    UA_Server_delete(server_register);
    UA_ServerConfig_delete(config_register);
}

START_TEST(Server_register) {
    UA_StatusCode retval =
        UA_Server_register_discovery(server_register, "opc.tcp://localhost:4840", NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(Server_unregister) {
    UA_StatusCode retval =
        UA_Server_unregister_discovery(server_register, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(Server_register_semaphore) {
    // create the semaphore
    int fd = open("/tmp/open62541-unit-test-semaphore", O_RDWR|O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
    ck_assert_int_ne(fd, -1);
    close(fd);

    UA_StatusCode retval =
        UA_Server_register_discovery(server_register, "opc.tcp://localhost:4840",
                                     "/tmp/open62541-unit-test-semaphore");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(Server_unregister_semaphore) {
    // delete the semaphore, this should remove the registration automatically on next check
    ck_assert_int_eq(remove("/tmp/open62541-unit-test-semaphore"), 0);
}
END_TEST

START_TEST(Server_register_periodic) {
    // periodic register every minute, first register immediately
    UA_StatusCode retval =
        UA_Server_addPeriodicServerRegisterCallback(server_register, "opc.tcp://localhost:4840",
                                                    60*1000, 100, &periodicRegisterCallbackId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(Server_unregister_periodic) {
    // wait for first register delay
    UA_sleep(1000);
    sleep(1);
    UA_Server_removeRepeatedCallback(server_register, periodicRegisterCallbackId);
    UA_StatusCode retval = UA_Server_unregister_discovery(server_register,
                                                          "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

static void
FindAndCheck(const UA_String expectedUris[], size_t expectedUrisSize,
             const UA_String expectedLocales[],
             const UA_String expectedNames[],
             const char *filterUri,
             const char *filterLocale) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_default);

    UA_ApplicationDescription* applicationDescriptionArray = NULL;
    size_t applicationDescriptionArraySize = 0;

    size_t serverUrisSize = 0;
    UA_String *serverUris = NULL;

    if(filterUri) {
        serverUrisSize = 1;
        serverUris = UA_String_new();
        serverUris[0] = UA_String_fromChars(filterUri);
    }

    size_t localeIdsSize = 0;
    UA_String *localeIds = NULL;

    if(filterLocale) {
        localeIdsSize = 1;
        localeIds = UA_String_new();
        localeIds[0] = UA_String_fromChars(filterLocale);
    }

    UA_StatusCode retval =
        UA_Client_findServers(client, "opc.tcp://localhost:4840",
                              serverUrisSize, serverUris, localeIdsSize, localeIds,
                              &applicationDescriptionArraySize, &applicationDescriptionArray);

    if(filterUri)
        UA_Array_delete(serverUris, serverUrisSize, &UA_TYPES[UA_TYPES_STRING]);

    if(filterLocale)
        UA_Array_delete(localeIds, localeIdsSize, &UA_TYPES[UA_TYPES_STRING]);

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    // only the discovery server is expected
    ck_assert_uint_eq(applicationDescriptionArraySize, expectedUrisSize);
    assert(applicationDescriptionArray != NULL);

    for(size_t i = 0; i < expectedUrisSize; ++i) {
        ck_assert(UA_String_equal(&applicationDescriptionArray[i].applicationUri,
                                  &expectedUris[i]));

        if(expectedNames)
            ck_assert(UA_String_equal(&applicationDescriptionArray[i].applicationName.text,
                                      &expectedNames[i]));

        if (expectedLocales)
            ck_assert(UA_String_equal(&applicationDescriptionArray[i].applicationName.locale,
                                      &expectedLocales[i]));
    }

    UA_Array_delete(applicationDescriptionArray, applicationDescriptionArraySize,
                    &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}

#ifdef UA_ENABLE_DISCOVERY_MULTICAST

static void
FindOnNetworkAndCheck(UA_String expectedServerNames[], size_t expectedServerNamesSize,
                      const char *filterUri, const char *filterLocale,
                      const char** filterCapabilities, size_t filterCapabilitiesSize) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_default);

    UA_ServerOnNetwork* serverOnNetwork = NULL;
    size_t serverOnNetworkSize = 0;


    size_t  serverCapabilityFilterSize = 0;
    UA_String *serverCapabilityFilter = NULL;

    if(filterCapabilitiesSize) {
        serverCapabilityFilterSize = filterCapabilitiesSize;
        serverCapabilityFilter = (UA_String*)UA_malloc(sizeof(UA_String) * filterCapabilitiesSize);
        for(size_t i = 0; i < filterCapabilitiesSize; i++)
            serverCapabilityFilter[i] = UA_String_fromChars(filterCapabilities[i]);
    }


    UA_StatusCode retval =
        UA_Client_findServersOnNetwork(client, "opc.tcp://localhost:4840", 0, 0,
                                       serverCapabilityFilterSize, serverCapabilityFilter,
                                       &serverOnNetworkSize, &serverOnNetwork);

    if(serverCapabilityFilterSize)
        UA_Array_delete(serverCapabilityFilter, serverCapabilityFilterSize,
                        &UA_TYPES[UA_TYPES_STRING]);

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    // only the discovery server is expected
    ck_assert_uint_eq(serverOnNetworkSize , expectedServerNamesSize);

    if(expectedServerNamesSize > 0)
        ck_assert_ptr_ne(serverOnNetwork, NULL);

    if(serverOnNetwork != NULL) {
        for(size_t i = 0; i < expectedServerNamesSize; i++)
            ck_assert(UA_String_equal(&serverOnNetwork[i].serverName,
                                      &expectedServerNames[i]));
    }

    UA_Array_delete(serverOnNetwork, serverOnNetworkSize, &UA_TYPES[UA_TYPES_SERVERONNETWORK]);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}

static UA_StatusCode
GetEndpoints(UA_Client *client, const UA_String* endpointUrl,
             size_t* endpointDescriptionsSize,
             UA_EndpointDescription** endpointDescriptions,
             const char* filterTransportProfileUri) {
    UA_GetEndpointsRequest request;
    UA_GetEndpointsRequest_init(&request);
    //request.requestHeader.authenticationToken = client->authenticationToken;
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    request.endpointUrl = *endpointUrl; // assume the endpointurl outlives the service call
    if (filterTransportProfileUri) {
        request.profileUrisSize = 1;
        request.profileUris = (UA_String*)UA_malloc(sizeof(UA_String));
        request.profileUris[0] = UA_String_fromChars(filterTransportProfileUri);
    }

    UA_GetEndpointsResponse response;
    UA_GetEndpointsResponse_init(&response);
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_GETENDPOINTSREQUEST],
                        &response, &UA_TYPES[UA_TYPES_GETENDPOINTSRESPONSE]);

    if (filterTransportProfileUri) {
        UA_Array_delete(request.profileUris, request.profileUrisSize, &UA_TYPES[UA_TYPES_STRING]);
    }

    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    *endpointDescriptionsSize = response.endpointsSize;
    *endpointDescriptions =
        (UA_EndpointDescription*)UA_Array_new(response.endpointsSize,
                                              &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    for(size_t i=0;i<response.endpointsSize;i++) {
        UA_EndpointDescription_init(&(*endpointDescriptions)[i]);
        UA_EndpointDescription_copy(&response.endpoints[i], &(*endpointDescriptions)[i]);
    }
    UA_GetEndpointsResponse_deleteMembers(&response);
    return UA_STATUSCODE_GOOD;
}

static void
GetEndpointsAndCheck(const char* discoveryUrl, const char* filterTransportProfileUri,
                     const UA_String expectedEndpointUrls[], size_t expectedEndpointUrlsSize) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_default);

    ck_assert_uint_eq(UA_Client_connect(client, discoveryUrl), UA_STATUSCODE_GOOD);

    UA_EndpointDescription* endpointArray = NULL;
    size_t endpointArraySize = 0;
    UA_String discoveryUrlUA = UA_String_fromChars(discoveryUrl);
    UA_StatusCode retval = GetEndpoints(client, &discoveryUrlUA, &endpointArraySize,
                                        &endpointArray, filterTransportProfileUri);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_String_deleteMembers(&discoveryUrlUA);

    ck_assert_uint_eq(endpointArraySize , expectedEndpointUrlsSize);

    for(size_t j = 0; j < endpointArraySize && j < expectedEndpointUrlsSize; j++) {
        UA_EndpointDescription* endpoint = &endpointArray[j];
        ck_assert(UA_String_equal(&endpoint->endpointUrl, &expectedEndpointUrls[j]));
    }

    UA_Array_delete(endpointArray, endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    UA_Client_delete(client);
}

// Test if discovery server lists himself as registered server if it is filtered by his uri
START_TEST(Client_filter_discovery) {
    const UA_String expectedUris[] = {UA_STRING("urn:open62541.test.local_discovery_server")};
    FindAndCheck(expectedUris, 1, NULL, NULL, "urn:open62541.test.local_discovery_server", NULL);
}
END_TEST

// Test if server filters locale
START_TEST(Client_filter_locale) {
    const UA_String expectedUris[] = {
        UA_STRING("urn:open62541.test.local_discovery_server"),
        UA_STRING("urn:open62541.test.server_register")
    };
    const UA_String expectedNames[] = {
        UA_STRING("LDS Server"),
        UA_STRING("Anmeldungsserver")
    };
    const UA_String expectedLocales[] = {UA_STRING("en"), UA_STRING("de")};
    // even if we request en_US, the server will return de_DE because it only has that name.
    FindAndCheck(expectedUris, 2, expectedLocales, expectedNames, NULL, "en");

}
END_TEST

// Test if registered server is returned from LDS using FindServersOnNetwork
START_TEST(Client_find_on_network_registered) {
    char urls[2][64];
    UA_String expectedUris[2];
    char hostname[256];

    ck_assert_uint_eq(gethostname(hostname, 255), 0);

    //DNS limits name to max 63 chars (+ \0)
    snprintf(urls[0], 64, "LDS_test-%s", hostname);
    snprintf(urls[1], 64, "Register_test-%s", hostname);
    expectedUris[0] = UA_STRING(urls[0]);
    expectedUris[1] = UA_STRING(urls[1]);
    FindOnNetworkAndCheck(expectedUris, 2, NULL, NULL, NULL, 0);

    // filter by Capabilities
    const char* capsLDS[] = {"LDS"};
    const char* capsNA[] = {"NA"};
    const char* capsMultiple[] = {"LDS", "NA"};

    // only LDS expected
    FindOnNetworkAndCheck(expectedUris, 1, NULL, NULL, capsLDS, 1);
    // only register server expected
    FindOnNetworkAndCheck(&expectedUris[1], 1, NULL, NULL, capsNA, 1);
    // no server expected
    FindOnNetworkAndCheck(NULL, 0, NULL, NULL, capsMultiple, 2);
}
END_TEST

// Test if filtering with uris works
START_TEST(Client_find_filter) {
    const UA_String expectedUris[] = {
        UA_STRING("urn:open62541.test.server_register")
    };
    FindAndCheck(expectedUris, 1, NULL, NULL, "urn:open62541.test.server_register", NULL);
}
END_TEST

START_TEST(Client_get_endpoints) {
    const UA_String  expectedEndpoints[] ={
        UA_STRING("opc.tcp://localhost:4840")
    };

    // general check if expected endpoints are returned
    GetEndpointsAndCheck("opc.tcp://localhost:4840", NULL,expectedEndpoints, 1);

    // check if filtering transport profile still returns the endpoint
    GetEndpointsAndCheck("opc.tcp://localhost:4840",
                         "http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary",
                         expectedEndpoints, 1);

    // filter transport profily by HTTPS, which should return no endpoint
    GetEndpointsAndCheck("opc.tcp://localhost:4840",
                         "http://opcfoundation.org/UA-Profile/Transport/https-uabinary", NULL, 0);
}
END_TEST

#endif

// Test if discovery server lists himself as registered server, before any other registration.
START_TEST(Client_find_discovery) {
    const UA_String expectedUris[] = {UA_STRING("urn:open62541.test.local_discovery_server")};
    FindAndCheck(expectedUris, 1, NULL, NULL, NULL, NULL);
}
END_TEST

// Test if registered server is returned from LDS
START_TEST(Client_find_registered) {
    const UA_String expectedUris[] = {
        UA_STRING("urn:open62541.test.local_discovery_server"),
        UA_STRING("urn:open62541.test.server_register")
    };
    FindAndCheck(expectedUris, 2, NULL, NULL, NULL, NULL);
}
END_TEST

START_TEST(Util_start_lds) {
    setup_lds();
}
END_TEST

START_TEST(Util_stop_lds) {
    teardown_lds();
}
END_TEST

START_TEST(Util_wait_timeout) {
    // wait until server is removed by timeout. Additionally wait a few seconds more to be sure.
    UA_sleep(100000 * checkWait);
    sleep(1);
}
END_TEST

#ifdef UA_ENABLE_DISCOVERY_MULTICAST
START_TEST(Util_wait_mdns) {
    UA_sleep(1000);
    sleep(1);
}
END_TEST
#endif

START_TEST(Util_wait_startup) {
    UA_sleep(1000);
    sleep(1);
}
END_TEST

START_TEST(Util_wait_retry) {
    // first retry is after 2 seconds, then 4, so it should be enough to wait 3 seconds
    UA_sleep(3000);
    sleep(3);
}
END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Register Server and Client");
    TCase *tc_register = tcase_create("RegisterServer");
    tcase_add_unchecked_fixture(tc_register, setup_lds, teardown_lds);
    tcase_add_unchecked_fixture(tc_register, setup_register, teardown_register);
    tcase_add_test(tc_register, Server_register);
    // register two times, just for fun
    tcase_add_test(tc_register, Server_register);
    tcase_add_test(tc_register, Server_unregister);
    tcase_add_test(tc_register, Server_register_periodic);
    tcase_add_test(tc_register, Server_unregister_periodic);
    suite_add_tcase(s,tc_register);

    TCase *tc_register_retry = tcase_create("RegisterServer Retry");
    //tcase_add_unchecked_fixture(tc_register, setup_lds, teardown_lds);
    tcase_add_unchecked_fixture(tc_register_retry, setup_register, teardown_register);
    tcase_add_test(tc_register_retry, Server_register_periodic);
    tcase_add_test(tc_register_retry, Util_wait_startup); // wait a bit to let first try run through
    // now start LDS
    tcase_add_test(tc_register_retry, Util_start_lds);
    tcase_add_test(tc_register_retry, Util_wait_retry);
    // check if there
    tcase_add_test(tc_register_retry, Client_find_registered);
    tcase_add_test(tc_register_retry, Server_unregister_periodic);
    tcase_add_test(tc_register_retry, Client_find_discovery);
    tcase_add_test(tc_register_retry, Util_stop_lds);

    suite_add_tcase(s,tc_register_retry);

#ifdef UA_ENABLE_DISCOVERY_MULTICAST
    TCase *tc_register_find = tcase_create("RegisterServer and FindServers");
    tcase_add_unchecked_fixture(tc_register_find, setup_lds, teardown_lds);
    tcase_add_unchecked_fixture(tc_register_find, setup_register, teardown_register);
    tcase_add_test(tc_register_find, Client_find_discovery);
    tcase_add_test(tc_register_find, Server_register);
    tcase_add_test(tc_register_find, Client_find_registered);
    tcase_add_test(tc_register_find, Util_wait_mdns);
    tcase_add_test(tc_register_find, Client_find_on_network_registered);
    tcase_add_test(tc_register_find, Client_find_filter);
    tcase_add_test(tc_register_find, Client_get_endpoints);
    tcase_add_test(tc_register_find, Client_filter_locale);
    tcase_add_test(tc_register_find, Server_unregister);
    tcase_add_test(tc_register_find, Client_find_discovery);
    tcase_add_test(tc_register_find, Client_filter_discovery);
    suite_add_tcase(s,tc_register_find);
#endif

    // register server again, then wait for timeout and auto unregister
    TCase *tc_register_timeout = tcase_create("RegisterServer timeout");
    tcase_add_unchecked_fixture(tc_register_timeout, setup_lds, teardown_lds);
    tcase_add_unchecked_fixture(tc_register_timeout, setup_register, teardown_register);
    tcase_set_timeout(tc_register_timeout, checkWait+2);
    tcase_add_test(tc_register_timeout, Server_register);
    tcase_add_test(tc_register_timeout, Client_find_registered);
    tcase_add_test(tc_register_timeout, Util_wait_timeout);
    tcase_add_test(tc_register_timeout, Client_find_discovery);
#ifdef UA_ENABLE_DISCOVERY_SEMAPHORE
    // now check if semaphore file works
    tcase_add_test(tc_register_timeout, Server_register_semaphore);
    tcase_add_test(tc_register_timeout, Client_find_registered);
    tcase_add_test(tc_register_timeout, Server_unregister_semaphore);
    tcase_add_test(tc_register_timeout, Util_wait_timeout);
    tcase_add_test(tc_register_timeout, Client_find_discovery);
#endif
    suite_add_tcase(s,tc_register_timeout);
    return s;
}

int main(void) {
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
