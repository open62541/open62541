#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <ua_util.h>
#include <ua_types_generated.h>

#include "ua_client.h"
#include "ua_config_standard.h"
#include "ua_network_tcp.h"
#include "check.h"



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
	nl_lds = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 4840);
	config_lds.networkLayers = &nl_lds;
	config_lds.networkLayersSize = 1;
	server_lds = UA_Server_new(config_lds);
	UA_Server_run_startup(server_lds);
	pthread_create(&server_thread_lds, NULL, serverloop_lds, NULL);
}

static void teardown_lds(void) {
	*running_lds = false;
	pthread_join(server_thread_lds, NULL);
	UA_Server_run_shutdown(server_lds);
	UA_Boolean_delete(running_lds);
	UA_String_deleteMembers(&server_lds->config.applicationDescription.applicationUri);
	UA_Server_delete(server_lds);
	nl_lds.deleteMembers(&nl_lds);
}


UA_Server *server_register;
UA_Boolean *running_register;
UA_ServerNetworkLayer nl_register;
pthread_t server_thread_register;

static void * serverloop_register(void *_) {
	while(*running_register)
		UA_Server_run_iterate(server_register, true);
	return NULL;
}

static void setup_register(void) {
	// start LDS server
	running_register = UA_Boolean_new();
	*running_register = true;
	UA_ServerConfig config_register = UA_ServerConfig_standard;
	config_register.applicationDescription.applicationUri = UA_String_fromChars("open62541.test.server_register");
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


static UA_StatusCode FindServers(const char* discoveryServerUrl, size_t* registeredServerSize, UA_ApplicationDescription** registeredServers, const char* filterUri) {
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

	// now send the request
	UA_FindServersResponse response;
	UA_FindServersResponse_init(&response);
	__UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST],
						&response, &UA_TYPES[UA_TYPES_FINDSERVERSRESPONSE]);

	if (filterUri) {
		UA_Array_delete(request.serverUris, request.serverUrisSize, &UA_TYPES[UA_TYPES_STRING]);
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

// Test if discovery server lists himself as registered server, before any other registration.
START_TEST(Client_find_discovery) {
		UA_Client *client = UA_Client_new(UA_ClientConfig_standard);
		UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");

		ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

		UA_ApplicationDescription* applicationDescriptionArray = NULL;
		size_t applicationDescriptionArraySize = 0;

		retval = FindServers("opc.tcp://localhost:4840", &applicationDescriptionArraySize, &applicationDescriptionArray, NULL);
		ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

		// only the discovery server is expected
		ck_assert_uint_eq(applicationDescriptionArraySize , 1);


		char* serverUri = malloc(sizeof(char)*applicationDescriptionArray[0].applicationUri.length+1);
		memcpy( serverUri, applicationDescriptionArray[0].applicationUri.data, applicationDescriptionArray[0].applicationUri.length );
		serverUri[applicationDescriptionArray[0].applicationUri.length] = '\0';
		ck_assert_str_eq(serverUri, "open62541.test.local_discovery_server");
		free(serverUri);

		UA_Array_delete(applicationDescriptionArray, applicationDescriptionArraySize, &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);

		UA_Client_disconnect(client);
		UA_Client_delete(client);
	}
END_TEST

// Test if registered server is returned from LDS
START_TEST(Client_find_registered) {
		UA_Client *client = UA_Client_new(UA_ClientConfig_standard);
		UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");

		ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

		UA_ApplicationDescription* applicationDescriptionArray = NULL;
		size_t applicationDescriptionArraySize = 0;

		retval = FindServers("opc.tcp://localhost:4840", &applicationDescriptionArraySize, &applicationDescriptionArray, NULL);
		ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

		// only the discovery server is expected
		ck_assert_uint_eq(applicationDescriptionArraySize , 2);


		char* serverUri = malloc(sizeof(char)*applicationDescriptionArray[0].applicationUri.length+1);
		memcpy( serverUri, applicationDescriptionArray[0].applicationUri.data, applicationDescriptionArray[0].applicationUri.length );
		serverUri[applicationDescriptionArray[0].applicationUri.length] = '\0';
		ck_assert_str_eq(serverUri, "open62541.test.local_discovery_server");
		free(serverUri);


		serverUri = malloc(sizeof(char)*applicationDescriptionArray[1].applicationUri.length+1);
		memcpy( serverUri, applicationDescriptionArray[1].applicationUri.data, applicationDescriptionArray[1].applicationUri.length );
		serverUri[applicationDescriptionArray[1].applicationUri.length] = '\0';
		ck_assert_str_eq(serverUri, "open62541.test.server_register");
		free(serverUri);

		UA_Array_delete(applicationDescriptionArray, applicationDescriptionArraySize, &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);

		UA_Client_disconnect(client);
		UA_Client_delete(client);
	}
END_TEST

// Test if filtering with uris works
START_TEST(Client_find_filter) {
		UA_Client *client = UA_Client_new(UA_ClientConfig_standard);
		UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");

		ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

		UA_ApplicationDescription* applicationDescriptionArray = NULL;
		size_t applicationDescriptionArraySize = 0;

		retval = FindServers("opc.tcp://localhost:4840", &applicationDescriptionArraySize, &applicationDescriptionArray, "open62541.test.server_register");
		ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

		// only the discovery server is expected
		ck_assert_uint_eq(applicationDescriptionArraySize , 1);


		char* serverUri = malloc(sizeof(char)*applicationDescriptionArray[0].applicationUri.length+1);
		memcpy( serverUri, applicationDescriptionArray[0].applicationUri.data, applicationDescriptionArray[0].applicationUri.length );
		serverUri[applicationDescriptionArray[0].applicationUri.length] = '\0';
		ck_assert_str_eq(serverUri, "open62541.test.server_register");
		free(serverUri);

		UA_Array_delete(applicationDescriptionArray, applicationDescriptionArraySize, &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);

		UA_Client_disconnect(client);
		UA_Client_delete(client);
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
	suite_add_tcase(s,tc_register);

	TCase *tc_register_find = tcase_create("RegisterServer and FindServers");
	tcase_add_unchecked_fixture(tc_register_find, setup_lds, teardown_lds);
	tcase_add_unchecked_fixture(tc_register_find, setup_register, teardown_register);
	tcase_add_test(tc_register_find, Client_find_discovery);
	tcase_add_test(tc_register_find, Server_register);
	tcase_add_test(tc_register_find, Client_find_registered);
	tcase_add_test(tc_register_find, Client_find_filter);
	tcase_add_test(tc_register_find, Server_unregister);
	tcase_add_test(tc_register_find, Client_find_discovery);
	suite_add_tcase(s,tc_register_find);
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
