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
	config_lds.applicationDescription.applicationUri = UA_String_fromChars("open62541.test.local_discovery_server");
	config_lds.applicationDescription.applicationName.locale = UA_String_fromChars("EN");
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
	config_register.applicationDescription.applicationUri = UA_String_fromChars("open62541.test.server_register");
	config_register.applicationDescription.applicationName.locale = UA_String_fromChars("EN");
	config_register.applicationDescription.applicationName.text = UA_String_fromChars("Register Server");
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

START_TEST(Server_register_periodic) {
		// periodic register every minute, first register immediately
		UA_StatusCode retval = UA_Server_addPeriodicServerRegisterJob(server_register, 60*1000, 100, &periodicRegisterJobId);
		ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
	}
END_TEST

START_TEST(Server_unregister_periodic) {
		// wait for first register delay
		sleep(1);
		UA_Server_removeRepeatedJob(server_register, periodicRegisterJobId);
		UA_StatusCode retval = UA_Server_unregister_discovery(server_register, "opc.tcp://localhost:4840");
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

static void FindAndCheck(const char* expectedUris[], size_t expectedUrisSize, const char *filterUri, const char *filterLocale) {
	UA_Client *client = UA_Client_new(UA_ClientConfig_standard);
	UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");

	ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

	UA_ApplicationDescription* applicationDescriptionArray = NULL;
	size_t applicationDescriptionArraySize = 0;

	retval = FindServers("opc.tcp://localhost:4840", &applicationDescriptionArraySize, &applicationDescriptionArray, filterUri, filterLocale);
	ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

	// only the discovery server is expected
	ck_assert_uint_eq(applicationDescriptionArraySize , expectedUrisSize);

	for (size_t i=0; i<expectedUrisSize; i++) {
		char* serverUri = malloc(sizeof(char)*applicationDescriptionArray[i].applicationUri.length+1);
		memcpy( serverUri, applicationDescriptionArray[i].applicationUri.data, applicationDescriptionArray[i].applicationUri.length );
		serverUri[applicationDescriptionArray[i].applicationUri.length] = '\0';
		ck_assert_str_eq(serverUri, expectedUris[i]);
		free(serverUri);
	}

	UA_Array_delete(applicationDescriptionArray, applicationDescriptionArraySize, &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);

	UA_Client_disconnect(client);
	UA_Client_delete(client);
}

static UA_StatusCode FindServersOnNetwork(const char* discoveryServerUrl, size_t* serverOnNetworkSize, UA_ServerOnNetwork** serverOnNetwork) {
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
	/*
	 * Here you can define some filtering rules:
	 */
	//request.serverCapabilityFilterSize = 1;
	//request.serverCapabilityFilter[0] = UA_String_fromChars("LDS");

	// now send the request
	UA_FindServersOnNetworkResponse response;
	UA_FindServersOnNetworkResponse_init(&response);
	__UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_FINDSERVERSONNETWORKREQUEST],
						&response, &UA_TYPES[UA_TYPES_FINDSERVERSONNETWORKRESPONSE]);

	//UA_Array_delete(request.serverCapabilityFilter, request.serverCapabilityFilterSize, &UA_TYPES[UA_TYPES_STRING]);

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

static void FindOnNetworkAndCheck(char* expectedServerNames[], size_t expectedServerNamesSize, const char *filterUri, const char *filterLocale) {
	UA_Client *client = UA_Client_new(UA_ClientConfig_standard);
	UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");

	ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

	UA_ServerOnNetwork* serverOnNetwork = NULL;
	size_t serverOnNetworkSize = 0;

	retval = FindServersOnNetwork("opc.tcp://localhost:4840", &serverOnNetworkSize, &serverOnNetwork);
	ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

	// only the discovery server is expected
	ck_assert_uint_eq(serverOnNetworkSize , expectedServerNamesSize);

	for (size_t i=0; i<expectedServerNamesSize; i++) {
		char* serverName = malloc(sizeof(char)*serverOnNetwork[i].serverName.length+1);
		memcpy( serverName, serverOnNetwork[i].serverName.data, serverOnNetwork[i].serverName.length );
		serverName[serverOnNetwork[i].serverName.length] = '\0';
		ck_assert_str_eq(serverName, expectedServerNames[i]);
		free(serverName);
	}

	UA_Array_delete(serverOnNetwork, serverOnNetworkSize, &UA_TYPES[UA_TYPES_SERVERONNETWORK]);

	UA_Client_disconnect(client);
	UA_Client_delete(client);
}


// Test if discovery server lists himself as registered server, before any other registration.
START_TEST(Client_find_discovery) {
		const char* expectedUris[] ={"open62541.test.local_discovery_server"};
		FindAndCheck(expectedUris, 1, NULL, NULL);
	}
END_TEST

// Test if discovery server lists himself as registered server if it is filtered by his uri
START_TEST(Client_filter_discovery) {
		const char* expectedUris[] ={"open62541.test.local_discovery_server"};
		FindAndCheck(expectedUris, 1, "open62541.test.local_discovery_server", "en");
	}
END_TEST

// Test if registered server is returned from LDS
START_TEST(Client_find_registered) {
		const char* expectedUris[] ={"open62541.test.local_discovery_server", "open62541.test.server_register"};
		FindAndCheck(expectedUris, 2, NULL, NULL);
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
		FindOnNetworkAndCheck(expectedUris, 2, NULL, NULL);

		free(expectedUris[0]);
		free(expectedUris[1]);

	}
END_TEST

// Test if filtering with uris works
START_TEST(Client_find_filter) {
		const char* expectedUris[] ={"open62541.test.server_register"};
		FindAndCheck(expectedUris, 1, "open62541.test.server_register", NULL);
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

static Suite* testSuite_Client(void) {
	Suite *s = suite_create("Register Server and Client");
	TCase *tc_register = tcase_create("RegisterServer");
	tcase_add_unchecked_fixture(tc_register, setup_lds, teardown_lds);
	tcase_add_unchecked_fixture(tc_register, setup_register, teardown_register);
	tcase_add_test(tc_register, Server_register);
	// register two times
	tcase_add_test(tc_register, Server_register);
	tcase_add_test(tc_register, Server_unregister);
	tcase_add_test(tc_register, Server_register_periodic);
	tcase_add_test(tc_register, Server_unregister_periodic);
	suite_add_tcase(s,tc_register);

	TCase *tc_register_find = tcase_create("RegisterServer and FindServers");
	tcase_add_unchecked_fixture(tc_register_find, setup_lds, teardown_lds);
	tcase_add_unchecked_fixture(tc_register_find, setup_register, teardown_register);
	tcase_add_test(tc_register_find, Client_find_discovery);
	tcase_add_test(tc_register_find, Server_register);
	tcase_add_test(tc_register_find, Client_find_registered);
	tcase_add_test(tc_register_find, Util_wait_mdns);
	tcase_add_test(tc_register_find, Client_find_on_network_registered);
	tcase_add_test(tc_register_find, Client_find_filter);
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
