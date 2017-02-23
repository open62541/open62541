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


// set register timeout to 1 second so we are able to test it.
#define registerTimeout 1
// cleanup is only triggered every 10 seconds, thus wait a bit longer to check
#define checkWait registerTimeout + 11

UA_Server *server_lds;
UA_Boolean *running_lds;
UA_ServerNetworkLayer nl_lds;
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
    UA_ServerConfig config_lds = UA_ServerConfig_standard;
    config_lds.applicationDescription.applicationType = UA_APPLICATIONTYPE_DISCOVERYSERVER;
    config_lds.applicationDescription.applicationUri = UA_String_fromChars("urn:open62541.test.local_discovery_server");
    config_lds.applicationDescription.applicationName.locale = UA_String_fromChars("en");
    config_lds.applicationDescription.applicationName.text = UA_String_fromChars("LDS Server");
    config_lds.mdnsServerName = UA_String_fromChars("LDS_test");
    config_lds.serverCapabilitiesSize = 1;
    UA_String *caps = UA_String_new();
    *caps = UA_String_fromChars("LDS");
    config_lds.serverCapabilities = caps;
    config_lds.discoveryCleanupTimeout = registerTimeout;
    nl_lds = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 4840);
    config_lds.networkLayers = &nl_lds;
    config_lds.networkLayersSize = 1;
    server_lds = UA_Server_new(config_lds);
    UA_Server_run_startup(server_lds);
    pthread_create(&server_thread_lds, NULL, serverloop_lds, NULL);
    // wait until LDS started
    sleep(1);
}

static void teardown_lds(void) {
    *running_lds = false;
    pthread_join(server_thread_lds, NULL);
    UA_Server_run_shutdown(server_lds);
    UA_Boolean_delete(running_lds);
    UA_String_deleteMembers(&server_lds->config.applicationDescription.applicationUri);
    UA_LocalizedText_deleteMembers(&server_lds->config.applicationDescription.applicationName);
    UA_String_deleteMembers(&server_lds->config.mdnsServerName);
    UA_Array_delete(server_lds->config.serverCapabilities, server_lds->config.serverCapabilitiesSize, &UA_TYPES[UA_TYPES_STRING]);
    UA_Server_delete(server_lds);
    nl_lds.deleteMembers(&nl_lds);
}


UA_Server *server_register;
UA_Boolean *running_register;
UA_ServerNetworkLayer nl_register;
pthread_t server_thread_register;

UA_Guid periodicRegisterJobId;

static void * serverloop_register(void *_) {
    while(*running_register)
        UA_Server_run_iterate(server_register, true);
    return NULL;
}

static void setup_register(void) {
    // start register server
    running_register = UA_Boolean_new();
    *running_register = true;
    UA_ServerConfig config_register = UA_ServerConfig_standard;
    config_register.applicationDescription.applicationUri = UA_String_fromChars("urn:open62541.test.server_register");
    config_register.applicationDescription.applicationName.locale = UA_String_fromChars("de");
    config_register.applicationDescription.applicationName.text = UA_String_fromChars("Anmeldungsserver");
    config_register.mdnsServerName = UA_String_fromChars("Register_test");
    nl_register = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 16664);
    config_register.networkLayers = &nl_register;
    config_register.networkLayersSize = 1;
    server_register = UA_Server_new(config_register);
    UA_Server_run_startup(server_register);
    pthread_create(&server_thread_register, NULL, serverloop_register, NULL);
}

static void teardown_register(void) {
    *running_register = false;
    pthread_join(server_thread_register, NULL);
    UA_Server_run_shutdown(server_register);
    UA_Boolean_delete(running_register);
    UA_String_deleteMembers(&server_register->config.applicationDescription.applicationUri);
    UA_LocalizedText_deleteMembers(&server_register->config.applicationDescription.applicationName);
    UA_String_deleteMembers(&server_register->config.mdnsServerName);
    UA_Server_delete(server_register);
    nl_register.deleteMembers(&nl_register);
}


static char* UA_String_to_char_alloc(const UA_String *str) {
    char* ret = malloc(sizeof(char)*str->length+1);
    memcpy( ret, str->data, str->length );
    ret[str->length] = '\0';
    return ret;
}

START_TEST(Server_register) {
        UA_StatusCode retval = UA_Server_register_discovery(server_register, "opc.tcp://localhost:4840", NULL);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }
END_TEST

START_TEST(Server_unregister) {
        UA_StatusCode retval = UA_Server_unregister_discovery(server_register, "opc.tcp://localhost:4840");
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }
END_TEST


START_TEST(Server_register_semaphore) {
        // create the semaphore
        int fd = open("/tmp/open62541-unit-test-semaphore", O_RDWR|O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
        ck_assert_int_ne(fd, -1);
        close(fd);

        UA_StatusCode retval = UA_Server_register_discovery(server_register, "opc.tcp://localhost:4840", "/tmp/open62541-unit-test-semaphore");
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
        UA_StatusCode retval = UA_Server_addPeriodicServerRegisterJob(server_register, NULL, 60*1000, 100, &periodicRegisterJobId);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }
END_TEST

START_TEST(Server_unregister_periodic) {
        // wait for first register delay
        sleep(1);
        UA_Server_removeRepeatedJob(server_register, periodicRegisterJobId);
        UA_StatusCode retval = UA_Server_unregister_discovery(server_register, NULL);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }
END_TEST


static UA_StatusCode FindServers(const char* discoveryServerUrl, size_t* registeredServerSize, UA_ApplicationDescription** registeredServers, const char* filterUri, const char* filterLocale) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_standard);
    UA_StatusCode retval = UA_Client_connect(client, discoveryServerUrl);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return retval;
    }


    UA_FindServersRequest request;
    UA_FindServersRequest_init(&request);

    if (filterUri) {
        request.serverUrisSize = 1;
        request.serverUris = UA_malloc(sizeof(UA_String));
        request.serverUris[0] = UA_String_fromChars(filterUri);
    }

    if (filterLocale) {
        request.localeIdsSize = 1;
        request.localeIds = UA_malloc(sizeof(UA_String));
        request.localeIds[0] = UA_String_fromChars(filterLocale);
    }

    // now send the request
    UA_FindServersResponse response;
    UA_FindServersResponse_init(&response);
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST],
                        &response, &UA_TYPES[UA_TYPES_FINDSERVERSRESPONSE]);

    if (filterUri) {
        UA_Array_delete(request.serverUris, request.serverUrisSize, &UA_TYPES[UA_TYPES_STRING]);
    }

    if (filterLocale) {
        UA_Array_delete(request.localeIds, request.localeIdsSize, &UA_TYPES[UA_TYPES_STRING]);
    }

    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_FindServersResponse_deleteMembers(&response);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
        ck_abort_msg("FindServers failed with statuscode 0x%08x", response.responseHeader.serviceResult);
    }

    *registeredServerSize = response.serversSize;
    *registeredServers = (UA_ApplicationDescription*)UA_Array_new(response.serversSize, &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);
    for(size_t i=0;i<response.serversSize;i++)
        UA_ApplicationDescription_copy(&response.servers[i], &(*registeredServers)[i]);
    UA_FindServersResponse_deleteMembers(&response);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return (int) UA_STATUSCODE_GOOD;
}

static void FindAndCheck(const char* expectedUris[], size_t expectedUrisSize, const char* expectedLocales[], const char* expectedNames[], const char *filterUri, const char *filterLocale) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_standard);
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_ApplicationDescription* applicationDescriptionArray = NULL;
    size_t applicationDescriptionArraySize = 0;

    retval = FindServers("opc.tcp://localhost:4840", &applicationDescriptionArraySize, &applicationDescriptionArray, filterUri, filterLocale);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    // only the discovery server is expected
    ck_assert_uint_eq(applicationDescriptionArraySize, expectedUrisSize);
    assert(applicationDescriptionArray != NULL);

    for (size_t i=0; i < expectedUrisSize; ++i) {
        char* serverUri = UA_String_to_char_alloc(&applicationDescriptionArray[i].applicationUri);
        ck_assert_str_eq(serverUri, expectedUris[i]);
        free(serverUri);

        if (expectedNames && expectedNames[i] != NULL) {
            char *name = UA_String_to_char_alloc(&applicationDescriptionArray[i].applicationName.text);
            ck_assert_str_eq(name, expectedNames[i]);
            free(name);
        }
        if (expectedLocales && expectedLocales[i] != NULL) {
            char *locale = UA_String_to_char_alloc(&applicationDescriptionArray[i].applicationName.locale);
            ck_assert_str_eq(locale, expectedLocales[i]);
            free(locale);
        }
    }


    UA_Array_delete(applicationDescriptionArray, applicationDescriptionArraySize, &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}


static UA_StatusCode FindServersOnNetwork(const char* discoveryServerUrl, size_t* serverOnNetworkSize, UA_ServerOnNetwork** serverOnNetwork,
                                          const char** filterCapabilities, size_t filterCapabilitiesSize
) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_standard);
    UA_StatusCode retval = UA_Client_connect(client, discoveryServerUrl);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return retval;
    }


    UA_FindServersOnNetworkRequest request;
    UA_FindServersOnNetworkRequest_init(&request);

    request.startingRecordId = 0;
    request.maxRecordsToReturn = 0; // get all

    if (filterCapabilitiesSize) {

        request.serverCapabilityFilterSize = filterCapabilitiesSize;
        request.serverCapabilityFilter = UA_malloc(sizeof(UA_String) * filterCapabilitiesSize);
        for (size_t i=0; i<filterCapabilitiesSize; i++) {
            request.serverCapabilityFilter[i] = UA_String_fromChars(filterCapabilities[i]);
        }
    }
    // now send the request
    UA_FindServersOnNetworkResponse response;
    UA_FindServersOnNetworkResponse_init(&response);
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_FINDSERVERSONNETWORKREQUEST],
                        &response, &UA_TYPES[UA_TYPES_FINDSERVERSONNETWORKRESPONSE]);

    if (request.serverCapabilityFilterSize) {
        UA_Array_delete(request.serverCapabilityFilter, request.serverCapabilityFilterSize, &UA_TYPES[UA_TYPES_STRING]);
    }

    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_FindServersOnNetworkResponse_deleteMembers(&response);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
        ck_abort_msg("FindServersOnNetwork failed with statuscode 0x%08x", response.responseHeader.serviceResult);
    }

    *serverOnNetworkSize = response.serversSize;
    *serverOnNetwork = (UA_ServerOnNetwork*)UA_Array_new(response.serversSize, &UA_TYPES[UA_TYPES_SERVERONNETWORK]);
    for(size_t i=0;i<response.serversSize;i++)
        UA_ServerOnNetwork_copy(&response.servers[i], &(*serverOnNetwork)[i]);
    UA_FindServersOnNetworkResponse_deleteMembers(&response);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return (int) UA_STATUSCODE_GOOD;
}

static void FindOnNetworkAndCheck(char* expectedServerNames[], size_t expectedServerNamesSize, const char *filterUri, const char *filterLocale,
                                  const char** filterCapabilities, size_t filterCapabilitiesSize) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_standard);
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_ServerOnNetwork* serverOnNetwork = NULL;
    size_t serverOnNetworkSize = 0;

    retval = FindServersOnNetwork("opc.tcp://localhost:4840", &serverOnNetworkSize, &serverOnNetwork, filterCapabilities, filterCapabilitiesSize);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    // only the discovery server is expected
    ck_assert_uint_eq(serverOnNetworkSize , expectedServerNamesSize);

    if (expectedServerNamesSize > 0) {
        ck_assert_ptr_ne(serverOnNetwork, NULL);
    }

    if (serverOnNetwork != NULL) {
        for (size_t i=0; i<expectedServerNamesSize; i++) {
            char* serverName = malloc(sizeof(char) * (serverOnNetwork[i].serverName.length+1));
            memcpy( serverName, serverOnNetwork[i].serverName.data, serverOnNetwork[i].serverName.length );
            serverName[serverOnNetwork[i].serverName.length] = '\0';
            ck_assert_str_eq(serverName, expectedServerNames[i]);
            free(serverName);
        }
    }

    UA_Array_delete(serverOnNetwork, serverOnNetworkSize, &UA_TYPES[UA_TYPES_SERVERONNETWORK]);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}

static UA_StatusCode GetEndpoints(UA_Client *client, const UA_String* endpointUrl, size_t* endpointDescriptionsSize, UA_EndpointDescription** endpointDescriptions,
                                  const char* filterTransportProfileUri
) {
    UA_GetEndpointsRequest request;
    UA_GetEndpointsRequest_init(&request);
    //request.requestHeader.authenticationToken = client->authenticationToken;
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    request.endpointUrl = *endpointUrl; // assume the endpointurl outlives the service call
    if (filterTransportProfileUri) {
        request.profileUrisSize = 1;
        request.profileUris = UA_malloc(sizeof(UA_String));
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
    *endpointDescriptions = (UA_EndpointDescription*)UA_Array_new(response.endpointsSize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    for(size_t i=0;i<response.endpointsSize;i++) {
        UA_EndpointDescription_init(&(*endpointDescriptions)[i]);
        UA_EndpointDescription_copy(&response.endpoints[i], &(*endpointDescriptions)[i]);
    }
    UA_GetEndpointsResponse_deleteMembers(&response);
    return UA_STATUSCODE_GOOD;
}


static void GetEndpointsAndCheck(const char* discoveryUrl, const char* filterTransportProfileUri, const char* expectedEndpointUrls[], size_t expectedEndpointUrlsSize) {

    UA_Client *client = UA_Client_new(UA_ClientConfig_standard);

    ck_assert_uint_eq(UA_Client_connect(client, discoveryUrl), UA_STATUSCODE_GOOD);

    UA_EndpointDescription* endpointArray = NULL;
    size_t endpointArraySize = 0;
    UA_String discoveryUrlUA = UA_String_fromChars(discoveryUrl);
    UA_StatusCode retval = GetEndpoints(client, &discoveryUrlUA, &endpointArraySize, &endpointArray, filterTransportProfileUri);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_String_deleteMembers(&discoveryUrlUA);

    ck_assert_uint_eq(endpointArraySize , expectedEndpointUrlsSize);

    for(size_t j = 0; j < endpointArraySize && j < expectedEndpointUrlsSize; j++) {
        UA_EndpointDescription* endpoint = &endpointArray[j];
        char *eu = UA_String_to_char_alloc(&endpoint->endpointUrl);
        ck_assert_ptr_ne(eu, NULL); // clang static analysis fix
        ck_assert_str_eq(eu, expectedEndpointUrls[j]);
        free(eu);
    }

    UA_Array_delete(endpointArray, endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    UA_Client_delete(client);
}

// Test if discovery server lists himself as registered server, before any other registration.
START_TEST(Client_find_discovery) {
        const char* expectedUris[] ={"urn:open62541.test.local_discovery_server"};
        FindAndCheck(expectedUris, 1,NULL, NULL, NULL, NULL);
    }
END_TEST

// Test if discovery server lists himself as registered server if it is filtered by his uri
START_TEST(Client_filter_discovery) {
        const char* expectedUris[] ={"urn:open62541.test.local_discovery_server"};
        FindAndCheck(expectedUris, 1,NULL, NULL, "urn:open62541.test.local_discovery_server", NULL);
    }
END_TEST

// Test if server filters locale
START_TEST(Client_filter_locale) {
        const char* expectedUris[] ={"urn:open62541.test.local_discovery_server", "urn:open62541.test.server_register"};
        const char* expectedNames[] ={"LDS Server", "Anmeldungsserver"};
        const char* expectedLocales[] ={"en", "de"};
        // even if we request en_US, the server will return de_DE because it only has that name.
        FindAndCheck(expectedUris, 2,expectedLocales, expectedNames, NULL, "en");

    }
END_TEST

// Test if registered server is returned from LDS
START_TEST(Client_find_registered) {
        const char* expectedUris[] ={"urn:open62541.test.local_discovery_server", "urn:open62541.test.server_register"};
        FindAndCheck(expectedUris, 2, NULL, NULL, NULL, NULL);
    }
END_TEST

// Test if registered server is returned from LDS using FindServersOnNetwork
START_TEST(Client_find_on_network_registered) {
        char *expectedUris[2];
        char hostname[256];

        ck_assert_uint_eq(gethostname(hostname, 255), 0);

        //DNS limits name to max 63 chars (+ \0)
        expectedUris[0] = malloc(64);
        snprintf(expectedUris[0], 64, "LDS_test-%s", hostname);
        expectedUris[1] = malloc(64);
        snprintf(expectedUris[1], 64, "Register_test-%s", hostname);
        FindOnNetworkAndCheck(expectedUris, 2, NULL, NULL, NULL, 0);


        // filter by Capabilities
        const char* capsLDS[] ={"LDS"};
        const char* capsNA[] ={"NA"};
        const char* capsMultiple[] ={"LDS", "NA"};

        // only LDS expected
        FindOnNetworkAndCheck(expectedUris, 1, NULL, NULL, capsLDS, 1);
        // only register server expected
        FindOnNetworkAndCheck(&expectedUris[1], 1, NULL, NULL, capsNA, 1);
        // no server expected
        FindOnNetworkAndCheck(NULL, 0, NULL, NULL, capsMultiple, 2);

        free(expectedUris[0]);
        free(expectedUris[1]);

    }
END_TEST

// Test if filtering with uris works
START_TEST(Client_find_filter) {
        const char* expectedUris[] ={"urn:open62541.test.server_register"};
        FindAndCheck(expectedUris, 1,NULL, NULL, "urn:open62541.test.server_register", NULL);
    }
END_TEST


START_TEST(Client_get_endpoints) {
        const char* expectedEndpoints[] ={"opc.tcp://localhost:4840"};
        // general check if expected endpoints are returned
        GetEndpointsAndCheck("opc.tcp://localhost:4840", NULL,expectedEndpoints, 1);
        // check if filtering transport profile still returns the endpoint
        GetEndpointsAndCheck("opc.tcp://localhost:4840", "http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary", expectedEndpoints, 1);
        // filter transport profily by HTTPS, which should return no endpoint
        GetEndpointsAndCheck("opc.tcp://localhost:4840", "http://opcfoundation.org/UA-Profile/Transport/https-uabinary", NULL, 0);
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
        sleep(checkWait);
    }
END_TEST

START_TEST(Util_wait_mdns) {
        sleep(1);
    }
END_TEST

START_TEST(Util_wait_startup) {
        sleep(1);
    }
END_TEST

START_TEST(Util_wait_retry) {
        // first retry is after 2 seconds, then 4, so it should be enough to wait 3 seconds
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

    // register server again, then wait for timeout and auto unregister
    TCase *tc_register_timeout = tcase_create("RegisterServer timeout");
    tcase_add_unchecked_fixture(tc_register_timeout, setup_lds, teardown_lds);
    tcase_add_unchecked_fixture(tc_register_timeout, setup_register, teardown_register);
    tcase_set_timeout(tc_register_timeout, checkWait+2);
    tcase_add_test(tc_register_timeout, Server_register);
    tcase_add_test(tc_register_timeout, Client_find_registered);
    tcase_add_test(tc_register_timeout, Util_wait_timeout);
    tcase_add_test(tc_register_timeout, Client_find_discovery);
    // now check if semaphore file works
    tcase_add_test(tc_register_timeout, Server_register_semaphore);
    tcase_add_test(tc_register_timeout, Client_find_registered);
    tcase_add_test(tc_register_timeout, Server_unregister_semaphore);
    tcase_add_test(tc_register_timeout, Util_wait_timeout);
    tcase_add_test(tc_register_timeout, Client_find_discovery);
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
