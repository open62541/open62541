/* This Source Code Form is subject to the terms of the Mozilla Public
*  License, v. 2.0. If a copy of the MPL was not distributed with this
*  file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/server_config_default.h>

#include "server/ua_server_internal.h"

#include <fcntl.h>

#include "testing_clock.h"
#include "thread_wrapper.h"
#ifndef WIN32
#include <sys/stat.h>
#endif
#include <check.h>

#ifndef _WIN32
#include <sys/stat.h>
#endif

// set register timeout to 1 second so we are able to test it.
#define registerTimeout 1
// cleanup is only triggered every 10 seconds, thus wait a bit longer to check
#define checkWait registerTimeout + 11

UA_Server *server_lds;
UA_Boolean *running_lds;
THREAD_HANDLE server_thread_lds;
UA_Client *clientRegisterRepeated;

THREAD_CALLBACK(serverloop_lds) {
    while(*running_lds)
        UA_Server_run_iterate(server_lds, true);
    return 0;
}

static void configure_lds_server(UA_Server *pServer)
{
    UA_ServerConfig *config_lds = UA_Server_getConfig(pServer);
    UA_ServerConfig_setDefault(config_lds);

    config_lds->applicationDescription.applicationType = UA_APPLICATIONTYPE_DISCOVERYSERVER;
    UA_String_deleteMembers(&config_lds->applicationDescription.applicationUri);
    config_lds->applicationDescription.applicationUri =
        UA_STRING_ALLOC("urn:open62541.test.local_discovery_server");
    UA_LocalizedText_deleteMembers(&config_lds->applicationDescription.applicationName);
    config_lds->applicationDescription.applicationName
        = UA_LOCALIZEDTEXT_ALLOC("en", "LDS Server");
    config_lds->discovery.mdnsEnable = true;
#ifdef UA_ENABLE_DISCOVERY_MULTICAST
    config_lds->discovery.mdns.mdnsServerName = UA_String_fromChars("LDS_test");
    config_lds->discovery.mdns.serverCapabilitiesSize = 1;
    UA_String *caps = UA_String_new();
    *caps = UA_String_fromChars("LDS");
    config_lds->discovery.mdns.serverCapabilities = caps;
#endif
    config_lds->discovery.cleanupTimeout = registerTimeout;
}

static void setup_lds(void) {
    // start LDS server
    running_lds = UA_Boolean_new();
    *running_lds = true;

    server_lds = UA_Server_new();
    configure_lds_server(server_lds);

    UA_Server_run_startup(server_lds);
    THREAD_CREATE(server_thread_lds, serverloop_lds);

    // wait until LDS started
    UA_fakeSleep(1000);
    UA_realSleep(1000);
}

static void teardown_lds(void) {
    *running_lds = false;
    THREAD_JOIN(server_thread_lds);
    UA_Server_run_shutdown(server_lds);
    UA_Boolean_delete(running_lds);
    UA_Server_delete(server_lds);
}

UA_Server *server_register;
UA_Boolean *running_register;
THREAD_HANDLE server_thread_register;

UA_UInt64 periodicRegisterCallbackId;

THREAD_CALLBACK(serverloop_register) {
    while(*running_register)
        UA_Server_run_iterate(server_register, true);
    return 0;
}

static void setup_register(void) {
    // start register server
    running_register = UA_Boolean_new();
    *running_register = true;

    server_register = UA_Server_new();
    UA_ServerConfig *config_register = UA_Server_getConfig(server_register);
    UA_ServerConfig_setMinimal(config_register, 16664, NULL);

    UA_String_deleteMembers(&config_register->applicationDescription.applicationUri);
    config_register->applicationDescription.applicationUri =
        UA_String_fromChars("urn:open62541.test.server_register");
    UA_LocalizedText_deleteMembers(&config_register->applicationDescription.applicationName);
    config_register->applicationDescription.applicationName =
        UA_LOCALIZEDTEXT_ALLOC("de", "Anmeldungsserver");
#ifdef UA_ENABLE_DISCOVERY_MULTICAST
    config_register->discovery.mdns.mdnsServerName = UA_String_fromChars("Register_test");
#endif

    UA_Server_run_startup(server_register);
    THREAD_CREATE(server_thread_register, serverloop_register);
}

static void teardown_register(void) {
    *running_register = false;
    THREAD_JOIN(server_thread_register);
    UA_Server_run_shutdown(server_register);
    UA_Boolean_delete(running_register);
    UA_Server_delete(server_register);
}

START_TEST(Server_new_delete) {
    UA_Server *pServer = UA_Server_new();
    configure_lds_server(pServer);
    UA_Server_delete(pServer);
}
END_TEST

START_TEST(Server_new_shutdown_delete) {
        UA_Server *pServer = UA_Server_new();
        configure_lds_server(pServer);
        UA_StatusCode retval = UA_Server_run_shutdown(pServer);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        UA_Server_delete(pServer);
}
END_TEST

START_TEST(Server_register) {
    UA_Client *clientRegister = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(clientRegister));

    UA_StatusCode retval = UA_Client_connect_noSession(clientRegister, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = UA_Server_register_discovery(server_register, clientRegister , NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Client_disconnect(clientRegister);
    UA_Client_delete(clientRegister);
}
END_TEST

START_TEST(Server_unregister) {
    UA_Client *clientRegister = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(clientRegister));

    UA_StatusCode retval = UA_Client_connect_noSession(clientRegister, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = UA_Server_unregister_discovery(server_register, clientRegister);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Client_disconnect(clientRegister);
    UA_Client_delete(clientRegister);
}
END_TEST

#ifdef UA_ENABLE_DISCOVERY_SEMAPHORE

#ifndef WIN32
#define SEMAPHORE_PATH "/tmp/open62541-unit-test-semaphore"
#else
#define SEMAPHORE_PATH ".\\open62541-unit-test-semaphore"
#endif

START_TEST(Server_register_semaphore) {
    // create the semaphore
#ifndef WIN32
    int fd = open(SEMAPHORE_PATH, O_RDWR|O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
    ck_assert_int_ne(fd, -1);
    close(fd);
#else
    FILE *fp;
    fopen_s(&fp, SEMAPHORE_PATH, "ab+");
    ck_assert_ptr_ne(fp, NULL);
    fclose(fp);
#endif

    UA_Client *clientRegister = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(clientRegister));

    UA_StatusCode retval = UA_Client_connect_noSession(clientRegister, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = UA_Server_register_discovery(server_register, clientRegister, SEMAPHORE_PATH);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Client_disconnect(clientRegister);
    UA_Client_delete(clientRegister);
}
END_TEST

START_TEST(Server_unregister_semaphore) {
    // delete the semaphore, this should remove the registration automatically on next check
    ck_assert_int_eq(remove(SEMAPHORE_PATH), 0);
}
END_TEST

#endif /* UA_ENABLE_DISCOVERY_SEMAPHORE */

START_TEST(Server_register_periodic) {
    ck_assert(clientRegisterRepeated == NULL);

    clientRegisterRepeated = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(clientRegisterRepeated));

    ck_assert(clientRegisterRepeated != NULL);
    // periodic register every minute, first register immediately
    UA_StatusCode retval = UA_Server_addPeriodicServerRegisterCallback(server_register, clientRegisterRepeated, "opc.tcp://localhost:4840",
                                                    60*1000, 100, &periodicRegisterCallbackId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(Server_unregister_periodic) {
    // wait for first register delay
    UA_fakeSleep(1000);
    UA_realSleep(1000);
    UA_Server_removeRepeatedCallback(server_register, periodicRegisterCallbackId);
    UA_StatusCode retval = UA_Server_unregister_discovery(server_register, clientRegisterRepeated);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Client_disconnect(clientRegisterRepeated);
    UA_Client_delete(clientRegisterRepeated);
    clientRegisterRepeated=NULL;
}
END_TEST

static void
FindAndCheck(const UA_String expectedUris[], size_t expectedUrisSize,
             const UA_String expectedLocales[],
             const UA_String expectedNames[],
             const char *filterUri,
             const char *filterLocale) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

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
    ck_assert(applicationDescriptionArray != NULL);

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
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

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
        for(size_t i = 0; i < expectedServerNamesSize; i++) {
            UA_Boolean expectedServerNameInServerOnNetwork = false;
            for(size_t j = 0; j < expectedServerNamesSize && !expectedServerNameInServerOnNetwork; j++) {
                expectedServerNameInServerOnNetwork = UA_String_equal(&serverOnNetwork[j].serverName,
                                        &expectedServerNames[i]);
            }
            ck_assert_msg(expectedServerNameInServerOnNetwork, "Expected %.*s in serverOnNetwork list, but not found",
                expectedServerNames[i].length, expectedServerNames[i].data);
        }
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
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

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
    UA_String expectedUris[1];
    expectedUris[0] = UA_STRING("urn:open62541.test.local_discovery_server");
    FindAndCheck(expectedUris, 1, NULL, NULL, "urn:open62541.test.local_discovery_server", NULL);
}
END_TEST

// Test if server filters locale
START_TEST(Client_filter_locale) {
    UA_String expectedUris[2];
    expectedUris[0] = UA_STRING("urn:open62541.test.local_discovery_server"),
    expectedUris[1] = UA_STRING("urn:open62541.test.server_register");
    UA_String expectedNames[2];
    expectedNames[0]= UA_STRING("LDS Server");
    expectedNames[1]= UA_STRING("Anmeldungsserver");
    UA_String expectedLocales[2];
    expectedLocales[0] = UA_STRING("en");
    expectedLocales[1] = UA_STRING("de");
    // even if we request en-US, the server will return de-DE because it only has that name.
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
    //We need this ugly casting, otherwise gcc >7.2 will complain about format-truncation, but we want it here
    void *hostnameVoid = (void*)hostname;
    snprintf(urls[0], 64, "LDS_test-%s", (char*)hostnameVoid);
    snprintf(urls[1], 64, "Register_test-%s", (char*)hostnameVoid);
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
    UA_String expectedUris[1];
    expectedUris[0] = UA_STRING("urn:open62541.test.server_register");
    FindAndCheck(expectedUris, 1, NULL, NULL, "urn:open62541.test.server_register", NULL);
}
END_TEST

START_TEST(Client_get_endpoints) {
    UA_String  expectedEndpoints[1];
    expectedEndpoints[0] = UA_STRING("opc.tcp://localhost:4840");

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
    UA_String expectedUris[1];
    expectedUris[0] = UA_STRING("urn:open62541.test.local_discovery_server");
    FindAndCheck(expectedUris, 1, NULL, NULL, NULL, NULL);
}
END_TEST

// Test if registered server is returned from LDS
START_TEST(Client_find_registered) {
    UA_String expectedUris[2];
    expectedUris[0] = UA_STRING("urn:open62541.test.local_discovery_server");
    expectedUris[1] = UA_STRING("urn:open62541.test.server_register");
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
    UA_fakeSleep(100000 * checkWait);
    UA_realSleep(1000);
}
END_TEST

#ifdef UA_ENABLE_DISCOVERY_MULTICAST
START_TEST(Util_wait_mdns) {
    UA_fakeSleep(5000);
    UA_realSleep(5000);
}
END_TEST
#endif

START_TEST(Util_wait_startup) {
    UA_fakeSleep(1000);
    UA_realSleep(1000);
}
END_TEST

START_TEST(Util_wait_retry) {
    // first retry is after 2 seconds, then 4, so it should be enough to wait 3 seconds
    UA_fakeSleep(3000);
    UA_realSleep(3000);
}
END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Register Server and Client");

    TCase *tc_new_del = tcase_create("New Delete");
    tcase_add_test(tc_new_del, Server_new_delete);
    tcase_add_test(tc_new_del, Server_new_shutdown_delete);
    suite_add_tcase(s,tc_new_del);

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
