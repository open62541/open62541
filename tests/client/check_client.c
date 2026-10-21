/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel_async.h>
#include <open62541/server_config_default.h>

#include "client/ua_client_internal.h"
#include "ua_server_internal.h"

#include <check.h>
#include <stdlib.h>

#include "testing_clock.h"
#include "testing_descriptors.h"
#include "testing_networklayers.h"
#include "thread_wrapper.h"

UA_Server *server;
UA_Boolean running;
UA_ServerNetworkLayer nl;
THREAD_HANDLE server_thread;

static void
addVariable(size_t size) {
    /* Define the attribute of the myInteger variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32* array = (UA_Int32*)UA_malloc(size * sizeof(UA_Int32));
    for(size_t i = 0; i < size; i++)
        array[i] = (UA_Int32)i;
    UA_Variant_setArray(&attr.value, array, size, &UA_TYPES[UA_TYPES_INT32]);

    char name[] = "my.variable";
    attr.description = UA_LOCALIZEDTEXT("en-US", name);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", name);
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;

    /* Add the variable node to the information model */
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, name);
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, name);
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                              parentReferenceNodeId, myIntegerName,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              attr, NULL, NULL);

    UA_free(array);
}

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

#define VARLENGTH 16366

static void setup(void) {
    running = true;
    server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
    UA_Server_run_startup(server);
    addVariable(VARLENGTH);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(Client_connect) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_connect_username) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_StatusCode retval =
        UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "user1", "password");

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_endpoints) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    UA_EndpointDescription* endpointArray = NULL;
    size_t endpointArraySize = 0;
    UA_StatusCode retval = UA_Client_getEndpoints(client, "opc.tcp://localhost:4840",
                                                  &endpointArraySize, &endpointArray);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(endpointArraySize > 0);

    UA_Array_delete(endpointArray,endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);

    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_endpoints_empty) {
    /* Issue a getEndpoints call with empty endpointUrl.
     * Using UA_Client_getEndpoints automatically passes the client->endpointUrl as requested endpointUrl.
     * The spec says:
     * The Server should return a suitable default URL if it does not recognize the HostName in the URL.
     *
     * See https://github.com/open62541/open62541/issues/775
     */
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_GetEndpointsRequest request;
    UA_GetEndpointsRequest_init(&request);
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;

    UA_GetEndpointsResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_GETENDPOINTSREQUEST],
                        &response, &UA_TYPES[UA_TYPES_GETENDPOINTSRESPONSE]);

    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    ck_assert(response.endpointsSize > 0);

    UA_GetEndpointsResponse_clear(&response);
    UA_GetEndpointsRequest_clear(&request);

    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_read) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant val;
    UA_NodeId nodeId = UA_NODEID_STRING(1, "my.variable");
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert(val.type == &UA_TYPES[UA_TYPES_INT32]);
    UA_Int32 *var = (UA_Int32*)val.data;
    for(size_t i = 0; i < VARLENGTH; i++)
        ck_assert_uint_eq((size_t)var[i], i);

    UA_Variant_clear(&val);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_renewSecureChannel) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Forward the time */
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_fakeSleep((UA_UInt32)((UA_Double)cc->secureChannelLifeTime * 0.8));

    /* Make the client renew the channel */
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Now read */
    UA_Variant val;
    UA_NodeId nodeId = UA_NODEID_STRING(1, "my.variable");
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert(retval == UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_Client_disconnect(client);
    UA_Client_delete(client);

} END_TEST

#ifdef UA_ENABLE_SUBSCRIPTIONS
START_TEST(Client_renewSecureChannelWithActiveSubscription) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    cc->secureChannelLifeTime = 10000;

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    /* Force the server to send keep alive responses every second to trigg
     * the client to send new publish requests. Requests from the client
     * will make the server to change to the new SecurityToken after renewal.
     */
    request.requestedPublishingInterval = 1000;
    request.requestedMaxKeepAliveCount = 1;
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                            NULL, NULL, NULL);

    UA_CreateSubscriptionResponse_clear(&response);

    /* manually control the server thread */
    running = false;
    THREAD_JOIN(server_thread);

    for(int i = 0; i < 15; ++i) {
        UA_fakeSleep(1000);
        UA_Server_run_iterate(server, true);
        retval = UA_Client_run_iterate(client, 1);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }

    /* run the server in an independent thread again */
    running = true;
    THREAD_CREATE(server_thread, serverloop);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST
#endif

START_TEST(Client_reconnect) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant val;
    UA_NodeId nodeId = UA_NODEID_STRING(1, "my.variable");
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    // FIX THIS FOR THE AZURE BUILD

#if 0
    printf("Restart server to test reconnect\n");
    fflush(stdout);
    teardown();
    setup();

    printf("Read attribute with closed connection\n");
    fflush(stdout);
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADCONNECTIONCLOSED);

    printf("Reconnect client \n");
    fflush(stdout);
    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, UA_StatusCode_name(retval));

    UA_SessionState ss;
    UA_Client_getState(client, NULL, &ss, NULL);
    ck_assert_int_eq(ss, UA_SESSIONSTATE_ACTIVATED);

    printf("Read attribute again\n");
    fflush(stdout);
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    printf("Finish up\n");
    fflush(stdout);
#endif

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_delete_without_connect) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    ck_assert(client != NULL);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_activateSessionClose) {
    // restart server
    teardown();
    setup();
    ck_assert_uint_eq(server->sessionCount, 0);

    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->sessionCount, 1);

    UA_Variant val;
    UA_NodeId nodeId = UA_NODEID_STRING(1, "my.variable");
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_Client_disconnect(client);

    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->sessionCount, 1);

    nodeId = UA_NODEID_STRING(1, "my.variable");
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    ck_assert_uint_eq(server->sessionCount, 0);
}
END_TEST

START_TEST(Client_activateSessionTimeout) {
    // restart server
    teardown();
    setup();
    ck_assert_uint_eq(server->sessionCount, 0);

    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(server->sessionCount, 1);

    UA_Variant val;
    UA_Variant_init(&val);
    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_Client_recv = client->connection.recv;
    client->connection.recv = UA_Client_recvTesting;

    /* Simulate network cable unplugged (no response from server) */
    UA_Client_recvTesting_result = UA_STATUSCODE_GOODNONCRITICALTIMEOUT;

    UA_Variant_init(&val);
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADCONNECTIONCLOSED);

    UA_Client_recvTesting_result = UA_STATUSCODE_GOOD;
    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->sessionCount, 1);

    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_Client_delete(client);

    ck_assert_uint_eq(server->sessionCount, 0);
}
END_TEST

START_TEST(Client_activateSessionLocaleIds) {
    // restart server
    teardown();
    setup();
    ck_assert_uint_eq(server->sessionCount, 0);

    UA_Client *client = UA_Client_new();
    UA_ClientConfig *config = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(config);

    config->sessionLocaleIdsSize = 1;
    config->sessionLocaleIds = (UA_LocaleId*)UA_Array_new(1, &UA_TYPES[UA_TYPES_LOCALEID]);
    config->sessionLocaleIds[0] = UA_STRING_ALLOC("en");

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(server->sessionCount, 1);

    UA_Client_delete(client);

    ck_assert_uint_eq(server->sessionCount, 0);
}
END_TEST

START_TEST(Client_activateSessionSameUserAllowed) {
    ck_assert_uint_eq(server->sessionCount, 0);

    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_StatusCode retval =
        UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "user1", "password");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->sessionCount, 1);

    /* Re-activate the same Session on a new SecureChannel with the same user */
    UA_Client_disconnectSecureChannel(client);
    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->sessionCount, 1);

    UA_Variant val;
    UA_Variant_init(&val);
    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_Client_delete(client);
    ck_assert_uint_eq(server->sessionCount, 0);
}
END_TEST

START_TEST(Client_activateSessionUserChangeRejected) {
    ck_assert_uint_eq(server->sessionCount, 0);

    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_StatusCode retval =
        UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "user1", "password");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->sessionCount, 1);

    /* Try to re-activate the Session on a new SecureChannel for another user */
    UA_Client_disconnectSecureChannel(client);
    retval = UA_Client_connectUsername(client, "opc.tcp://localhost:4840",
                                       "user2", "password1");
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADIDENTITYCHANGENOTSUPPORTED);
    ck_assert_uint_eq(server->sessionCount, 1);

    /* The rejected activation does not damage the original Session. It can
     * still be re-activated for the original user. */
    UA_Client_disconnectSecureChannel(client);
    retval = UA_Client_connectUsername(client, "opc.tcp://localhost:4840",
                                       "user1", "password");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->sessionCount, 1);

    UA_Variant val;
    UA_Variant_init(&val);
    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_Client_delete(client);
    ck_assert_uint_eq(server->sessionCount, 0);
}
END_TEST

START_TEST(Client_activateSessionAnonymousUserChangeRejected) {
    ck_assert_uint_eq(server->sessionCount, 0);

    UA_Client *client = UA_Client_new();
    UA_ClientConfig *config = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(config);
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->sessionCount, 1);

    /* Try to re-activate the anonymous Session for a named user. The client
     * does not select a new UserTokenPolicy for the existing Session. */
    UA_String_clear(&config->userTokenPolicy.policyId);
    config->userTokenPolicy.policyId = UA_STRING_ALLOC("open62541-username-policy");
    config->userTokenPolicy.tokenType = UA_USERTOKENTYPE_USERNAME;
    UA_Client_disconnectSecureChannel(client);
    retval = UA_Client_connectUsername(client, "opc.tcp://localhost:4840",
                                       "user1", "password");
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADIDENTITYCHANGENOTSUPPORTED);
    ck_assert_uint_eq(server->sessionCount, 1);

    /* The client has no activated Session that it could close */
    UA_Client_delete(client);
}
END_TEST

#ifdef UA_ARCHITECTURE_POSIX
#include <arpa/inet.h>
#include <sys/socket.h>

/* A client socket that select() cannot handle is rejected */
START_TEST(Client_connectWithoutSelectableDescriptor) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    if(testing_fillDescriptors()) {
        UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
        ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);
    }
    testing_releaseDescriptors();

    /* The client works again with free descriptors */
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Client_delete(client);
} END_TEST

/* The server closes accepted connections that select() cannot handle. It keeps
 * serving the other connections. */
START_TEST(Server_acceptWithoutSelectableDescriptor) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    ck_assert_int_ge(sock, 0);
    struct timeval tv = {5, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    if(testing_fillDescriptors()) {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(4840);
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        ck_assert_int_eq(connect(sock, (struct sockaddr*)&addr, sizeof(addr)), 0);

        /* The server accepts the connection and closes it right away */
        char c;
        ck_assert_int_eq((int)recv(sock, &c, 1, 0), 0);
    }
    testing_releaseDescriptors();
    close(sock);

    /* The server still works */
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Client_delete(client);
} END_TEST
#endif

static char notConnectedLog[512];

static UA_FORMAT(4,0) void
captureNotConnectedLog(void *logContext, UA_LogLevel level, UA_LogCategory category,
                       const char *msg, va_list args) {
    char buf[512];
    vsnprintf(buf, sizeof(buf), msg, args);
    if(strstr(buf, "SecureChannel must be connected"))
        snprintf(notConnectedLog, sizeof(notConnectedLog), "%s", buf);
}

/* The log of a request on a client that is not connected has the request type
 * and the endpoint */
START_TEST(Client_notConnectedLogsRequestAndEndpoint) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig *config = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(config);
    UA_Logger oldLogger = config->logger;
    config->logger.log = captureNotConnectedLog;
    config->logger.context = NULL;
    config->logger.clear = NULL;
    UA_String_clear(&client->endpointUrl);
    client->endpointUrl = UA_STRING_ALLOC("opc.tcp://localhost:4840");

    /* Synchronous request */
    UA_Variant val;
    UA_Variant_init(&val);
    notConnectedLog[0] = 0;
    UA_StatusCode retval =
        UA_Client_readValueAttribute(client, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE), &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADCONNECTIONCLOSED);
    ck_assert_ptr_ne(strstr(notConnectedLog, "opc.tcp://localhost:4840"), NULL);
#ifdef UA_ENABLE_TYPEDESCRIPTION
    ck_assert_ptr_ne(strstr(notConnectedLog, "ReadRequest"), NULL);
#endif

    /* Asynchronous request */
    UA_BrowseRequest request;
    UA_BrowseRequest_init(&request);
    notConnectedLog[0] = 0;
    retval = UA_Client_sendAsyncBrowseRequest(client, &request, NULL, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADSERVERNOTCONNECTED);
    ck_assert_ptr_ne(strstr(notConnectedLog, "opc.tcp://localhost:4840"), NULL);
#ifdef UA_ENABLE_TYPEDESCRIPTION
    ck_assert_ptr_ne(strstr(notConnectedLog, "BrowseRequest"), NULL);
#endif

    config->logger = oldLogger;
    UA_Client_delete(client);
} END_TEST

/* A client needs a hostname. The URL is rejected before the name resolution. */
static char urlLog[512];

static UA_FORMAT(4,0) void
captureUrlLog(void *logContext, UA_LogLevel level, UA_LogCategory category,
              const char *msg, va_list args) {
    char buf[512];
    vsnprintf(buf, sizeof(buf), msg, args);
    if(strstr(buf, "Server url is invalid") || strstr(buf, "DNS lookup"))
        snprintf(urlLog, sizeof(urlLog), "%s", buf);
}

START_TEST(Client_connectWithoutHostname) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig *config = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(config);
    UA_Logger oldLogger = config->logger;
    config->logger.log = captureUrlLog;
    config->logger.context = NULL;
    config->logger.clear = NULL;

    urlLog[0] = 0;
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp:///127.0.0.1:4840");
    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);
    ck_assert_ptr_ne(strstr(urlLog, "Server url is invalid"), NULL);

    /* A normal URL still works */
    config->logger = oldLogger;
    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Client_delete(client);
} END_TEST

static UA_Boolean rejectActivation;
static UA_StatusCode
(*originalActivateSession)(UA_Server *s, UA_AccessControl *ac,
                           const UA_EndpointDescription *endpointDescription,
                           const UA_ByteString *secureChannelRemoteCertificate,
                           const UA_NodeId *sessionId,
                           const UA_ExtensionObject *userIdentityToken,
                           void **sessionContext);

static UA_StatusCode
activateSessionOrReject(UA_Server *s, UA_AccessControl *ac,
                        const UA_EndpointDescription *endpointDescription,
                        const UA_ByteString *secureChannelRemoteCertificate,
                        const UA_NodeId *sessionId,
                        const UA_ExtensionObject *userIdentityToken,
                        void **sessionContext) {
    if(rejectActivation) {
        rejectActivation = false;
        return UA_STATUSCODE_BADSERVERHALTED;
    }
    return originalActivateSession(s, ac, endpointDescription,
                                   secureChannelRemoteCertificate, sessionId,
                                   userIdentityToken, sessionContext);
}

START_TEST(Client_activateSessionRejectedOnceThenRetry) {
    UA_ServerConfig *sc = UA_Server_getConfig(server);
    originalActivateSession = sc->accessControl.activateSession;
    sc->accessControl.activateSession = activateSessionOrReject;

    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* The server rejects the re-activation of the Session once */
    UA_Client_disconnectSecureChannel(client);
    rejectActivation = true;
    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADSERVERHALTED);

    /* The next explicit connect starts over with a new SecureChannel */
    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant val;
    UA_Variant_init(&val);
    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_Client_delete(client);
    sc->accessControl.activateSession = originalActivateSession;
}
END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Client");
    TCase *tc_client = tcase_create("Client Basic");
    tcase_add_checked_fixture(tc_client, setup, teardown);
    tcase_add_test(tc_client, Client_connect);
    tcase_add_test(tc_client, Client_connect_username);
    tcase_add_test(tc_client, Client_delete_without_connect);
    tcase_add_test(tc_client, Client_endpoints);
    tcase_add_test(tc_client, Client_endpoints_empty);
    tcase_add_test(tc_client, Client_read);
    tcase_add_test(tc_client, Client_notConnectedLogsRequestAndEndpoint);
    tcase_add_test(tc_client, Client_connectWithoutHostname);
#ifdef UA_ARCHITECTURE_POSIX
    tcase_add_test(tc_client, Client_connectWithoutSelectableDescriptor);
    tcase_add_test(tc_client, Server_acceptWithoutSelectableDescriptor);
#endif
    suite_add_tcase(s,tc_client);
    TCase *tc_client_reconnect = tcase_create("Client Reconnect");
    tcase_add_checked_fixture(tc_client_reconnect, setup, teardown);
    tcase_add_test(tc_client_reconnect, Client_renewSecureChannel);
#ifdef UA_ENABLE_SUBSCRIPTIONS
    tcase_add_test(tc_client_reconnect, Client_renewSecureChannelWithActiveSubscription);
#endif
    tcase_add_test(tc_client_reconnect, Client_reconnect);
    tcase_add_test(tc_client_reconnect, Client_activateSessionClose);
    tcase_add_test(tc_client_reconnect, Client_activateSessionTimeout);
    tcase_add_test(tc_client_reconnect, Client_activateSessionLocaleIds);
    tcase_add_test(tc_client_reconnect, Client_activateSessionSameUserAllowed);
    tcase_add_test(tc_client_reconnect, Client_activateSessionUserChangeRejected);
    tcase_add_test(tc_client_reconnect, Client_activateSessionAnonymousUserChangeRejected);
    tcase_add_test(tc_client_reconnect, Client_activateSessionRejectedOnceThenRetry);
    suite_add_tcase(s,tc_client_reconnect);
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
