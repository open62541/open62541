/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/nodestore.h>

#include <check.h>
#include <stdlib.h>
#include <string.h>

static const UA_NodeId profileId =
    {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_SERVER_SERVERCAPABILITIES_SERVERPROFILEARRAY}};
static UA_String profiles[] = {
    UA_STRING_STATIC("urn:example:profile:server"),
    UA_STRING_STATIC("urn:example:profile:facet")
};

static void
setProfiles(UA_ServerConfig *config, const UA_String *values, size_t count) {
    UA_Array_delete(config->serverProfileArray, config->serverProfileArraySize,
                    &UA_TYPES[UA_TYPES_STRING]);
    config->serverProfileArray = NULL;
    config->serverProfileArraySize = 0;
    ck_assert_uint_eq(UA_Array_copy(values, count, (void**)&config->serverProfileArray,
                                    &UA_TYPES[UA_TYPES_STRING]), UA_STATUSCODE_GOOD);
    config->serverProfileArraySize = count;
}

static void
prepareConfig(UA_ServerConfig *config) {
    memset(config, 0, sizeof(*config));
    ck_assert_uint_eq(UA_ServerConfig_setDefault(config), UA_STATUSCODE_GOOD);
    config->tcpEnabled = false; /* No listening sockets needed for local reads. */
#ifdef UA_ENABLE_PUBSUB
    config->pubsubEnabled = false;
#endif
}

#ifndef UA_GENERATED_NAMESPACE_ZERO
/* Model a profile node supplied by an application or an external nodestore.
 * Raw insertion avoids requiring any other namespace-zero nodes in NONE. */
static void
insertProfileNode(UA_ServerConfig *config, const UA_String *values, size_t count) {
    UA_Nodestore *ns = config->nodestore;
    UA_Node *node = ns->newNode(ns, UA_NODECLASS_VARIABLE);
    ck_assert_ptr_nonnull(node);
    node->head.nodeId = profileId;
    node->head.browseName = UA_QUALIFIEDNAME_ALLOC(0, "ServerProfileArray");
    node->variableNode.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    node->variableNode.valueRank = 1;
    node->variableNode.accessLevel = UA_ACCESSLEVELMASK_READ;
    node->variableNode.valueSource.internal.value.hasValue = true;
    UA_Variant *value = &node->variableNode.valueSource.internal.value.value;
    if(count == 0)
        UA_Variant_setArray(value, UA_EMPTY_ARRAY_SENTINEL, 0, &UA_TYPES[UA_TYPES_STRING]);
    else
        ck_assert_uint_eq(UA_Variant_setArrayCopy(value, values, count,
                                                  &UA_TYPES[UA_TYPES_STRING]),
                          UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ns->insertNode(ns, node, NULL), UA_STATUSCODE_GOOD);
}
#endif

static UA_Server *
newServerWithProfiles(size_t count) {
    UA_ServerConfig config;
    prepareConfig(&config);
#ifndef UA_GENERATED_NAMESPACE_ZERO
    insertProfileNode(&config, NULL, 0);
#endif
    setProfiles(&config, profiles, count);
    UA_Server *server = UA_Server_newWithConfig(&config);
    ck_assert_ptr_nonnull(server);
    ck_assert_ptr_null(config.serverProfileArray); /* Ownership was moved. */
    ck_assert_uint_eq(config.serverProfileArraySize, 0);
    return server;
}

static void
assertProfiles(UA_Server *server, const UA_String *expected, size_t count) {
    UA_Variant value;
    UA_Variant_init(&value);
    ck_assert_uint_eq(UA_Server_readValue(server, profileId, &value), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasArrayType(&value, &UA_TYPES[UA_TYPES_STRING]));
    ck_assert_uint_eq(value.arrayLength, count);
    if(count == 0)
        ck_assert_ptr_eq(value.data, UA_EMPTY_ARRAY_SENTINEL);
    for(size_t i = 0; i < count; i++)
        ck_assert(UA_String_equal(&((UA_String*)value.data)[i], &expected[i]));
    UA_Variant_clear(&value);
}

static void
startServer(UA_Server *server) {
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
}

static void
stopServer(UA_Server *server) {
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
}

START_TEST(defaultDeclaration) {
    UA_Server *server = UA_Server_new();
    ck_assert_ptr_nonnull(server);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->tcpEnabled = false;
    ck_assert_uint_eq(config->serverProfileArraySize, 0);
    ck_assert_ptr_null(config->serverProfileArray);
#ifdef UA_GENERATED_NAMESPACE_ZERO
    assertProfiles(server, NULL, 0);
#else
    UA_Variant value;
    UA_Variant_init(&value);
    ck_assert_uint_eq(UA_Server_readValue(server, profileId, &value),
                      UA_STATUSCODE_BADNODEIDUNKNOWN);
    UA_Variant_clear(&value);
#endif
    startServer(server);
    stopServer(server);
    UA_Server_delete(server);
} END_TEST

START_TEST(configuredAtCreation) {
    UA_Server *server = newServerWithProfiles(2);
    assertProfiles(server, profiles, 2);
    startServer(server);
    assertProfiles(server, profiles, 2);
    stopServer(server);
    UA_Server_delete(server);
} END_TEST

START_TEST(refreshAtStartupAndRestart) {
    UA_Server *server = newServerWithProfiles(0);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    setProfiles(config, profiles, 2);
    startServer(server);
    assertProfiles(server, profiles, 2);
    stopServer(server);

    setProfiles(config, profiles + 1, 1);
    startServer(server);
    assertProfiles(server, profiles + 1, 1);
    stopServer(server);

    setProfiles(config, NULL, 0);
    startServer(server);
    assertProfiles(server, NULL, 0); /* The earlier declaration is retracted. */
    stopServer(server);
    UA_Server_delete(server);
} END_TEST

START_TEST(independentOwnershipAndDirectWrites) {
    UA_Server *server = newServerWithProfiles(2);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    /* Releasing the config's strings must not invalidate the node's copy. */
    setProfiles(config, NULL, 0);
    assertProfiles(server, profiles, 2);

    UA_Variant value;
    UA_Variant_setArray(&value, profiles + 1, 1, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_uint_eq(UA_Server_writeValue(server, profileId, value), UA_STATUSCODE_GOOD);
    assertProfiles(server, profiles + 1, 1);
    startServer(server);
    assertProfiles(server, NULL, 0); /* Startup applies the authoritative config. */
    stopServer(server);
    UA_Server_delete(server);
} END_TEST

START_TEST(invalidArrayAtCreation) {
    UA_ServerConfig config;
    prepareConfig(&config);
    config.serverProfileArraySize = 2;
    config.serverProfileArray = _i ? (UA_String*)UA_EMPTY_ARRAY_SENTINEL : NULL;
    ck_assert_ptr_null(UA_Server_newWithConfig(&config));
    ck_assert_ptr_null(config.serverProfileArray);
    ck_assert_uint_eq(config.serverProfileArraySize, 0);
    UA_ServerConfig_clear(&config); /* Failed creation already cleaned ownership. */
} END_TEST

START_TEST(invalidArrayAtStartup) {
    UA_Server *server = newServerWithProfiles(0);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->serverProfileArraySize = 1;
    config->serverProfileArray = NULL;
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_BADCONFIGURATIONERROR);
    ck_assert_uint_eq(UA_Server_getLifecycleState(server), UA_LIFECYCLESTATE_STOPPED);
    ck_assert_uint_eq(config->eventLoop->state, UA_EVENTLOOPSTATE_FRESH);
    config->serverProfileArraySize = 0;
    startServer(server);
    stopServer(server);
    UA_Server_delete(server);
} END_TEST

/* Part 5, 6.3.2: Every entry is the URI of a Profile */
START_TEST(emptyUriRejected) {
    UA_String invalid[2] = {profiles[0], UA_STRING_NULL};
    if(_i)
        invalid[1] = UA_STRING("");

    UA_ServerConfig config;
    prepareConfig(&config);
    setProfiles(&config, invalid, 2);
    ck_assert_ptr_null(UA_Server_newWithConfig(&config));
    UA_ServerConfig_clear(&config);

    UA_Server *server = newServerWithProfiles(0);
    setProfiles(UA_Server_getConfig(server), invalid, 2);
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_BADCONFIGURATIONERROR);
    ck_assert_uint_eq(UA_Server_getLifecycleState(server), UA_LIFECYCLESTATE_STOPPED);
    UA_Server_delete(server);
} END_TEST

START_TEST(missingNodeAtStartup) {
    UA_Server *server = newServerWithProfiles(0);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    ck_assert_uint_eq(config->nodestore->removeNode(config->nodestore, &profileId),
                      UA_STATUSCODE_GOOD);
    setProfiles(config, profiles, 2);
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_BADCONFIGURATIONERROR);
    ck_assert_uint_eq(UA_Server_getLifecycleState(server), UA_LIFECYCLESTATE_STOPPED);
    setProfiles(config, NULL, 0);
#ifdef UA_GENERATED_NAMESPACE_ZERO
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_BADNODEIDUNKNOWN);
#else
    startServer(server);
    stopServer(server);
#endif
    UA_Server_delete(server);
} END_TEST

static UA_StatusCode
readEmptyProfiles(UA_Server *server, const UA_NodeId *sessionId,
                  void *sessionContext, const UA_NodeId *nodeId,
                  void *nodeContext, UA_Boolean sourceTimestamp,
                  const UA_NumericRange *range, UA_DataValue *value) {
    UA_Variant_setArray(&value->value, UA_EMPTY_ARRAY_SENTINEL, 0,
                        &UA_TYPES[UA_TYPES_STRING]);
    value->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readAppProfiles(UA_Server *server, const UA_NodeId *sessionId,
                void *sessionContext, const UA_NodeId *nodeId,
                void *nodeContext, UA_Boolean sourceTimestamp,
                const UA_NumericRange *range, UA_DataValue *value) {
    UA_StatusCode res = UA_Variant_setArrayCopy(&value->value, profiles, 2,
                                                &UA_TYPES[UA_TYPES_STRING]);
    value->hasValue = (res == UA_STATUSCODE_GOOD);
    return res;
}

static UA_StatusCode
failProfileWrite(UA_Server *server, const UA_NodeId *sessionId,
                 void *sessionContext, const UA_NodeId *nodeId,
                 void *nodeContext, const UA_NumericRange *range,
                 const UA_DataValue *value) {
    return UA_STATUSCODE_BADOUTOFMEMORY;
}

START_TEST(writeFailureAtStartup) {
    UA_Server *server = newServerWithProfiles(0);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    setProfiles(config, profiles, 2);
    UA_CallbackValueSource source = {readEmptyProfiles, failProfileWrite};
    ck_assert_uint_eq(UA_Server_setVariableNode_callbackValueSource(server, profileId, source),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_BADOUTOFMEMORY);
    ck_assert_uint_eq(UA_Server_getLifecycleState(server), UA_LIFECYCLESTATE_STOPPED);
    ck_assert_uint_eq(config->eventLoop->state, UA_EVENTLOOPSTATE_FRESH);
    ck_assert_uint_eq(UA_Server_setVariableNode_internalValueSource(server, profileId, NULL, NULL),
                      UA_STATUSCODE_GOOD);
    startServer(server);
    assertProfiles(server, profiles, 2);
    stopServer(server);
    UA_Server_delete(server);
} END_TEST

/* An application may provide the profiles through a read-only value source. */
START_TEST(readOnlySourceKeepsValue) {
    UA_Server *server = newServerWithProfiles(0);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_CallbackValueSource source = {readAppProfiles, NULL};
    ck_assert_uint_eq(UA_Server_setVariableNode_callbackValueSource(server, profileId, source),
                      UA_STATUSCODE_GOOD);
    startServer(server);
    assertProfiles(server, profiles, 2);
    stopServer(server);

    /* A declaration in the config has to be applied to the node */
    setProfiles(config, profiles + 1, 1);
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_BADWRITENOTSUPPORTED);
    ck_assert_uint_eq(UA_Server_getLifecycleState(server), UA_LIFECYCLESTATE_STOPPED);
    UA_Server_delete(server);
} END_TEST

#ifdef UA_GENERATED_NAMESPACE_ZERO
static size_t emptyWarnings;

static void
countEmptyWarnings(void *context, UA_LogLevel level, UA_LogCategory category,
                   const char *msg, va_list args) {
    if(level == UA_LOGLEVEL_WARNING && strstr(msg, "ServerProfileArray"))
        emptyWarnings++;
}

START_TEST(warnEmptyAtStartup) {
    UA_Server *server = newServerWithProfiles(0);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_Logger *logging = config->logging;
    UA_Logger counter = {countEmptyWarnings, NULL, NULL};
    config->logging = &counter;
    emptyWarnings = 0;
    startServer(server);
    stopServer(server);
    ck_assert_uint_eq(emptyWarnings, 1);

    setProfiles(config, profiles, 2);
    startServer(server);
    stopServer(server);
    ck_assert_uint_eq(emptyWarnings, 1);
    config->logging = logging;
    UA_Server_delete(server);
} END_TEST
#endif

#ifndef UA_GENERATED_NAMESPACE_ZERO
START_TEST(missingNodeAtCreation) {
    UA_ServerConfig config;
    prepareConfig(&config);
    setProfiles(&config, profiles, 2);
    ck_assert_ptr_null(UA_Server_newWithConfig(&config));
    UA_ServerConfig_clear(&config);
} END_TEST

/* Without a declaration, the node supplied by the application or an external
 * nodestore keeps its value. */
START_TEST(externalValueKept) {
    UA_ServerConfig config;
    prepareConfig(&config);
    insertProfileNode(&config, profiles, 2);
    UA_Server *server = UA_Server_newWithConfig(&config);
    ck_assert_ptr_nonnull(server);
    assertProfiles(server, profiles, 2);
    startServer(server);
    assertProfiles(server, profiles, 2);
    stopServer(server);

    UA_ServerConfig *sc = UA_Server_getConfig(server);
    setProfiles(sc, profiles + 1, 1);
    startServer(server);
    assertProfiles(server, profiles + 1, 1);
    stopServer(server);

    setProfiles(sc, NULL, 0);
    startServer(server);
    assertProfiles(server, NULL, 0);
    stopServer(server);
    UA_Server_delete(server);
} END_TEST
#endif

#ifdef UA_ENABLE_JSON_ENCODING
static const char *validJson[] = {
    "{tcpEnabled:false}",
    "{serverProfileArray:[]}",
    "{serverProfileArray:['urn:example:profile:server','urn:example:profile:facet']}",
    "{serverProfileArray:['urn:example:old'],serverProfileArray:[]}",
    "{serverProfileArray:[],serverProfileArray:[],tcpEnabled:false}"
};

START_TEST(jsonDeclarations) {
    UA_ServerConfig config;
    UA_ByteString json = UA_STRING((char*)(uintptr_t)validJson[_i]);
    ck_assert_uint_eq(UA_ServerConfig_loadFromFile(&config, json), UA_STATUSCODE_GOOD);
    size_t count = (_i == 2) ? 2 : 0;
    ck_assert_uint_eq(config.serverProfileArraySize, count);
    config.tcpEnabled = false;
#ifndef UA_GENERATED_NAMESPACE_ZERO
    insertProfileNode(&config, NULL, 0);
#endif
    UA_Server *server = UA_Server_newWithConfig(&config);
    ck_assert_ptr_nonnull(server);
    assertProfiles(server, profiles, count);
    startServer(server);
    assertProfiles(server, profiles, count);
    stopServer(server);
    UA_Server_delete(server);
} END_TEST

static const char *invalidJson[] = {
    "{serverProfileArray:null}",
    "{serverProfileArray:'urn:example:profile'}",
    "{serverProfileArray:7}",
    "{serverProfileArray:{uri:'urn:example:profile'}}",
    "{serverProfileArray:['urn:example:profile',null]}",
    "{serverProfileArray:['urn:example:profile',7]}",
    "{serverProfileArray:[false]}",
    "{serverProfileArray:[[]]}",
    "{serverProfileArray:[{}]}",
    "{serverProfileArray:['urn:example:old'],serverProfileArray:[null]}"
};

START_TEST(invalidJsonClearsResources) {
    UA_ServerConfig config;
    UA_ByteString json = UA_STRING((char*)(uintptr_t)invalidJson[_i]);
    ck_assert_uint_eq(UA_ServerConfig_loadFromFile(&config, json),
                      UA_STATUSCODE_BADDECODINGERROR);
    ck_assert_ptr_null(config.serverProfileArray);
    ck_assert_uint_eq(config.serverProfileArraySize, 0);
    ck_assert_ptr_null(config.eventLoop);
    UA_ServerConfig_clear(&config);
} END_TEST
#endif

int main(void) {
    Suite *s = suite_create("Server profile declarations");
    TCase *tc = tcase_create("profiles");
    tcase_add_test(tc, defaultDeclaration);
    tcase_add_test(tc, configuredAtCreation);
    tcase_add_test(tc, refreshAtStartupAndRestart);
    tcase_add_test(tc, independentOwnershipAndDirectWrites);
    tcase_add_loop_test(tc, invalidArrayAtCreation, 0, 2);
    tcase_add_test(tc, invalidArrayAtStartup);
    tcase_add_loop_test(tc, emptyUriRejected, 0, 2);
    tcase_add_test(tc, missingNodeAtStartup);
    tcase_add_test(tc, writeFailureAtStartup);
    tcase_add_test(tc, readOnlySourceKeepsValue);
#ifdef UA_GENERATED_NAMESPACE_ZERO
    tcase_add_test(tc, warnEmptyAtStartup);
#else
    tcase_add_test(tc, missingNodeAtCreation);
    tcase_add_test(tc, externalValueKept);
#endif
#ifdef UA_ENABLE_JSON_ENCODING
    tcase_add_loop_test(tc, jsonDeclarations, 0, (int)(sizeof(validJson) / sizeof(validJson[0])));
    tcase_add_loop_test(tc, invalidJsonClearsResources, 0,
                       (int)(sizeof(invalidJson) / sizeof(invalidJson[0])));
#endif
    suite_add_tcase(s, tc);
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
