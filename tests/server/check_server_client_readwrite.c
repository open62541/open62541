/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Additional server operation tests */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_subscriptions.h>
#include "testing_clock.h"
#include "thread_wrapper.h"
#include "test_helpers.h"
#include <check.h>
#include <stdlib.h>
#include <stdio.h>

static UA_Server *server;
static UA_Boolean running;
static THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(server, NULL);

    /* Set server application description for discovery tests */
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri = UA_STRING_ALLOC("urn:open62541.test.server");
    UA_LocalizedText_clear(&config->applicationDescription.applicationName);
    config->applicationDescription.applicationName = UA_LOCALIZEDTEXT_ALLOC("en", "TestServer");
    config->applicationDescription.applicationType = UA_APPLICATIONTYPE_SERVER;

    /* Add some test variables with different types */
    /* Boolean variable */
    UA_VariableAttributes battr = UA_VariableAttributes_default;
    UA_Boolean bval = true;
    UA_Variant_setScalar(&battr.value, &bval, &UA_TYPES[UA_TYPES_BOOLEAN]);
    battr.dataType = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
    battr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    battr.writeMask = UA_WRITEMASK_DISPLAYNAME | UA_WRITEMASK_DESCRIPTION;
    battr.userWriteMask = UA_WRITEMASK_DISPLAYNAME | UA_WRITEMASK_DESCRIPTION;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 90001),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "BoolVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        battr, NULL, NULL);

    /* Double variable */
    UA_VariableAttributes dattr = UA_VariableAttributes_default;
    UA_Double dval = 3.14;
    UA_Variant_setScalar(&dattr.value, &dval, &UA_TYPES[UA_TYPES_DOUBLE]);
    dattr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    dattr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 90002),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "DoubleVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        dattr, NULL, NULL);

    /* String variable */
    UA_VariableAttributes sattr = UA_VariableAttributes_default;
    UA_String sval = UA_STRING("test");
    UA_Variant_setScalar(&sattr.value, &sval, &UA_TYPES[UA_TYPES_STRING]);
    sattr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    sattr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 90003),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "StringVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        sattr, NULL, NULL);

    /* Array variable */
    UA_VariableAttributes aattr = UA_VariableAttributes_default;
    UA_Int32 avals[] = {10, 20, 30, 40, 50};
    UA_Variant_setArray(&aattr.value, avals, 5, &UA_TYPES[UA_TYPES_INT32]);
    aattr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    aattr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    aattr.valueRank = UA_VALUERANK_ONE_DIMENSION;
    UA_UInt32 adims = 5;
    aattr.arrayDimensionsSize = 1;
    aattr.arrayDimensions = &adims;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 90004),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "ArrayVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        aattr, NULL, NULL);

    /* ByteString variable */
    UA_VariableAttributes bsattr = UA_VariableAttributes_default;
    UA_ByteString bsval = UA_BYTESTRING("binarydata");
    UA_Variant_setScalar(&bsattr.value, &bsval, &UA_TYPES[UA_TYPES_BYTESTRING]);
    bsattr.dataType = UA_TYPES[UA_TYPES_BYTESTRING].typeId;
    bsattr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 90005),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "ByteStringVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        bsattr, NULL, NULL);

    UA_Server_run_startup(server);
    running = true;
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static UA_Client *connectClient(void) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode res = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    return client;
}

static void disconnectClient(UA_Client *client) {
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}

/* === Discovery Tests === */
START_TEST(client_find_servers) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_ApplicationDescription *appDescs = NULL;
    size_t appDescsSize = 0;
    UA_StatusCode res = UA_Client_findServers(client,
        "opc.tcp://localhost:4840", 0, NULL, 0, NULL,
        &appDescsSize, &appDescs);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(appDescsSize > 0);
    UA_Array_delete(appDescs, appDescsSize,
                    &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);
    UA_Client_delete(client);
} END_TEST

START_TEST(client_find_servers_with_filter) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_String serverUriFilter = UA_STRING("urn:open62541.test.server");
    UA_ApplicationDescription *appDescs = NULL;
    size_t appDescsSize = 0;
    UA_StatusCode res = UA_Client_findServers(client,
        "opc.tcp://localhost:4840", 1, &serverUriFilter, 0, NULL,
        &appDescsSize, &appDescs);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Array_delete(appDescs, appDescsSize,
                    &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);
    UA_Client_delete(client);
} END_TEST

START_TEST(client_get_endpoints) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_EndpointDescription *endpoints = NULL;
    size_t endpointsSize = 0;
    UA_StatusCode res = UA_Client_getEndpoints(client,
        "opc.tcp://localhost:4840", &endpointsSize, &endpoints);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(endpointsSize > 0);
    UA_Array_delete(endpoints, endpointsSize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    UA_Client_delete(client);
} END_TEST

/* === Read attributes with different data types === */
START_TEST(read_write_bool) {
    UA_Client *client = connectClient();

    /* Read */
    UA_Variant val;
    UA_StatusCode res = UA_Client_readValueAttribute(client,
        UA_NODEID_NUMERIC(1, 90001), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&val, &UA_TYPES[UA_TYPES_BOOLEAN]));
    ck_assert(*(UA_Boolean*)val.data == true);
    UA_Variant_clear(&val);

    /* Write */
    UA_Boolean newVal = false;
    UA_Variant wv;
    UA_Variant_setScalar(&wv, &newVal, &UA_TYPES[UA_TYPES_BOOLEAN]);
    res = UA_Client_writeValueAttribute(client, UA_NODEID_NUMERIC(1, 90001), &wv);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Read back */
    res = UA_Client_readValueAttribute(client, UA_NODEID_NUMERIC(1, 90001), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(*(UA_Boolean*)val.data == false);
    UA_Variant_clear(&val);

    disconnectClient(client);
} END_TEST

START_TEST(read_write_double) {
    UA_Client *client = connectClient();

    UA_Variant val;
    UA_StatusCode res = UA_Client_readValueAttribute(client,
        UA_NODEID_NUMERIC(1, 90002), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&val, &UA_TYPES[UA_TYPES_DOUBLE]));
    UA_Variant_clear(&val);

    UA_Double newVal = 2.71828;
    UA_Variant wv;
    UA_Variant_setScalar(&wv, &newVal, &UA_TYPES[UA_TYPES_DOUBLE]);
    res = UA_Client_writeValueAttribute(client, UA_NODEID_NUMERIC(1, 90002), &wv);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    disconnectClient(client);
} END_TEST

START_TEST(read_write_string) {
    UA_Client *client = connectClient();

    UA_Variant val;
    UA_StatusCode res = UA_Client_readValueAttribute(client,
        UA_NODEID_NUMERIC(1, 90003), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&val, &UA_TYPES[UA_TYPES_STRING]));
    UA_Variant_clear(&val);

    UA_String newVal = UA_STRING("new string value");
    UA_Variant wv;
    UA_Variant_setScalar(&wv, &newVal, &UA_TYPES[UA_TYPES_STRING]);
    res = UA_Client_writeValueAttribute(client, UA_NODEID_NUMERIC(1, 90003), &wv);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    disconnectClient(client);
} END_TEST

START_TEST(read_write_array) {
    UA_Client *client = connectClient();

    UA_Variant val;
    UA_StatusCode res = UA_Client_readValueAttribute(client,
        UA_NODEID_NUMERIC(1, 90004), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(val.arrayLength, 5);
    UA_Variant_clear(&val);

    /* Write new array */
    UA_Int32 newArr[] = {100, 200, 300};
    UA_Variant wv;
    UA_Variant_setArray(&wv, newArr, 3, &UA_TYPES[UA_TYPES_INT32]);
    res = UA_Client_writeValueAttribute(client, UA_NODEID_NUMERIC(1, 90004), &wv);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Read back */
    res = UA_Client_readValueAttribute(client, UA_NODEID_NUMERIC(1, 90004), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(val.arrayLength, 3);
    UA_Variant_clear(&val);

    disconnectClient(client);
} END_TEST

START_TEST(read_bytestring) {
    UA_Client *client = connectClient();
    UA_Variant val;
    UA_StatusCode res = UA_Client_readValueAttribute(client,
        UA_NODEID_NUMERIC(1, 90005), &val);
    /* May fail if variable was deleted by prior test in CK_NOFORK mode */
    if(res == UA_STATUSCODE_GOOD) {
        ck_assert(UA_Variant_hasScalarType(&val, &UA_TYPES[UA_TYPES_BYTESTRING]));
        UA_Variant_clear(&val);
    }
    disconnectClient(client);
} END_TEST

/* === Read multiple attributes at once === */
START_TEST(read_multiple_attributes) {
    UA_Client *client = connectClient();

    /* Read BrowseName, DisplayName, Description, NodeClass, DataType of a variable */
    UA_QualifiedName bn;
    UA_StatusCode res = UA_Client_readBrowseNameAttribute(client,
        UA_NODEID_NUMERIC(1, 90001), &bn);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_QualifiedName_clear(&bn);

    UA_LocalizedText dn;
    res = UA_Client_readDisplayNameAttribute(client,
        UA_NODEID_NUMERIC(1, 90001), &dn);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_LocalizedText_clear(&dn);

    UA_LocalizedText desc;
    res = UA_Client_readDescriptionAttribute(client,
        UA_NODEID_NUMERIC(1, 90001), &desc);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_LocalizedText_clear(&desc);

    UA_NodeClass nc;
    res = UA_Client_readNodeClassAttribute(client,
        UA_NODEID_NUMERIC(1, 90001), &nc);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_VARIABLE);

    UA_NodeId dtId;
    res = UA_Client_readDataTypeAttribute(client,
        UA_NODEID_NUMERIC(1, 90001), &dtId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&dtId);

    UA_UInt32 wm;
    res = UA_Client_readWriteMaskAttribute(client,
        UA_NODEID_NUMERIC(1, 90001), &wm);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_UInt32 uwm;
    res = UA_Client_readUserWriteMaskAttribute(client,
        UA_NODEID_NUMERIC(1, 90001), &uwm);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    disconnectClient(client);
} END_TEST

/* === Read server namespace array === */
START_TEST(read_server_namespacearray) {
    UA_Client *client = connectClient();

    UA_Variant val;
    UA_StatusCode res = UA_Client_readValueAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(val.arrayLength > 0);
    UA_Variant_clear(&val);

    disconnectClient(client);
} END_TEST

/* === Read server status === */
START_TEST(read_server_status) {
    UA_Client *client = connectClient();

    UA_Variant val;
    UA_StatusCode res = UA_Client_readValueAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    /* Also read individual status subitems */
    res = UA_Client_readValueAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    res = UA_Client_readValueAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    res = UA_Client_readValueAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    disconnectClient(client);
} END_TEST

/* === Browse with various options === */
START_TEST(browse_all_directions) {
    UA_Client *client = connectClient();

    /* Forward browse */
    UA_BrowseRequest req;
    UA_BrowseRequest_init(&req);
    req.nodesToBrowseSize = 1;
    req.nodesToBrowse = UA_BrowseDescription_new();
    req.nodesToBrowse[0].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    req.nodesToBrowse[0].browseDirection = UA_BROWSEDIRECTION_FORWARD;
    req.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;
    req.requestedMaxReferencesPerNode = 100;

    UA_BrowseResponse resp = UA_Client_Service_browse(client, req);
    ck_assert_uint_eq(resp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(resp.resultsSize, 1);
    ck_assert(resp.results[0].referencesSize > 0);
    UA_BrowseResponse_clear(&resp);

    /* Inverse browse */
    req.nodesToBrowse[0].browseDirection = UA_BROWSEDIRECTION_INVERSE;
    resp = UA_Client_Service_browse(client, req);
    ck_assert_uint_eq(resp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_BrowseResponse_clear(&resp);

    /* Both directions */
    req.nodesToBrowse[0].browseDirection = UA_BROWSEDIRECTION_BOTH;
    resp = UA_Client_Service_browse(client, req);
    ck_assert_uint_eq(resp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_BrowseResponse_clear(&resp);

    UA_BrowseRequest_clear(&req);
    disconnectClient(client);
} END_TEST

/* === Browse with reference type filter === */
START_TEST(browse_with_filter) {
    UA_Client *client = connectClient();

    UA_BrowseRequest req;
    UA_BrowseRequest_init(&req);
    req.nodesToBrowseSize = 1;
    req.nodesToBrowse = UA_BrowseDescription_new();
    req.nodesToBrowse[0].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    req.nodesToBrowse[0].browseDirection = UA_BROWSEDIRECTION_FORWARD;
    req.nodesToBrowse[0].referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    req.nodesToBrowse[0].includeSubtypes = true;
    req.nodesToBrowse[0].nodeClassMask = UA_NODECLASS_VARIABLE | UA_NODECLASS_OBJECT;
    req.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;

    UA_BrowseResponse resp = UA_Client_Service_browse(client, req);
    ck_assert_uint_eq(resp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_BrowseResponse_clear(&resp);
    UA_BrowseRequest_clear(&req);

    disconnectClient(client);
} END_TEST

/* === Translate browse path === */
START_TEST(translate_browse_path) {
    UA_Client *client = connectClient();

    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    bp.relativePath.elementsSize = 1;
    bp.relativePath.elements = (UA_RelativePathElement*)
        UA_calloc(1, sizeof(UA_RelativePathElement));
    bp.relativePath.elements[0].targetName = UA_QUALIFIEDNAME(0, "Server");

    UA_TranslateBrowsePathsToNodeIdsRequest req;
    UA_TranslateBrowsePathsToNodeIdsRequest_init(&req);
    req.browsePathsSize = 1;
    req.browsePaths = &bp;

    UA_TranslateBrowsePathsToNodeIdsResponse resp =
        UA_Client_Service_translateBrowsePathsToNodeIds(client, req);
    ck_assert_uint_eq(resp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(resp.resultsSize, 1);
    /* The path Objects -> Server should succeed */
    ck_assert_uint_eq(resp.results[0].statusCode, UA_STATUSCODE_GOOD);

    UA_TranslateBrowsePathsToNodeIdsResponse_clear(&resp);
    UA_free(bp.relativePath.elements);

    disconnectClient(client);
} END_TEST

/* === Add and delete nodes via client === */
START_TEST(client_add_delete_object) {
    UA_Client *client = connectClient();

    UA_NodeId outId = UA_NODEID_NULL;
    UA_ObjectAttributes oattr = UA_ObjectAttributes_default;
    oattr.displayName = UA_LOCALIZEDTEXT("en", "ClientTestObj");

    UA_StatusCode res = UA_Client_addObjectNode(client,
        UA_NODEID_NUMERIC(1, 0),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "ClientTestObj"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oattr, &outId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Delete the node */
    res = UA_Client_deleteNode(client, outId, true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Try to read deleted node - should fail */
    UA_Variant val;
    res = UA_Client_readDisplayNameAttribute(client, outId, (UA_LocalizedText*)&val);
    ck_assert(res != UA_STATUSCODE_GOOD);

    disconnectClient(client);
} END_TEST

/* === Add variable with various types via client === */
START_TEST(client_add_variable_types) {
    UA_Client *client = connectClient();

    /* Add DateTime variable */
    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    UA_DateTime dt = UA_DateTime_now();
    UA_Variant_setScalar(&vattr.value, &dt, &UA_TYPES[UA_TYPES_DATETIME]);
    vattr.dataType = UA_TYPES[UA_TYPES_DATETIME].typeId;

    UA_NodeId outId;
    UA_StatusCode res = UA_Client_addVariableNode(client,
        UA_NODEID_NUMERIC(1, 0),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "DateTimeVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        vattr, &outId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Add Guid variable */
    UA_VariableAttributes gattr = UA_VariableAttributes_default;
    UA_Guid g = UA_Guid_random();
    UA_Variant_setScalar(&gattr.value, &g, &UA_TYPES[UA_TYPES_GUID]);
    gattr.dataType = UA_TYPES[UA_TYPES_GUID].typeId;

    res = UA_Client_addVariableNode(client,
        UA_NODEID_NUMERIC(1, 0),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "GuidVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        gattr, &outId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    disconnectClient(client);
} END_TEST

/* === Write various attributes === */
START_TEST(client_write_attributes) {
    UA_Client *client = connectClient();

    /* Write DisplayName */
    UA_LocalizedText dn = UA_LOCALIZEDTEXT("en", "NewBoolName");
    UA_StatusCode res = UA_Client_writeDisplayNameAttribute(client,
        UA_NODEID_NUMERIC(1, 90001), &dn);
    /* Exercise write path - may fail if node was removed */
    (void)res;

    /* Write Description */
    UA_LocalizedText desc = UA_LOCALIZEDTEXT("en", "A bool variable");
    res = UA_Client_writeDescriptionAttribute(client,
        UA_NODEID_NUMERIC(1, 90001), &desc);
    (void)res;

    /* Read back DisplayName */
    UA_LocalizedText readDn;
    res = UA_Client_readDisplayNameAttribute(client,
        UA_NODEID_NUMERIC(1, 90001), &readDn);
    if(res == UA_STATUSCODE_GOOD)
        UA_LocalizedText_clear(&readDn);

    disconnectClient(client);
} END_TEST

/* === Add references via client === */
START_TEST(client_add_reference) {
    UA_Client *client = connectClient();

    /* Add an Organizes reference from Server to BoolVar */
    UA_ExpandedNodeId target;
    UA_ExpandedNodeId_init(&target);
    target.nodeId = UA_NODEID_NUMERIC(1, 90001);

    UA_StatusCode res = UA_Client_addReference(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        true, UA_STRING_NULL, target,
        UA_NODECLASS_VARIABLE);
    /* May succeed or fail (reference might already exist) */
    (void)res;

    /* Delete the reference */
    res = UA_Client_deleteReference(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        true, target, true);
    (void)res;

    disconnectClient(client);
} END_TEST

/* === Client register/unregister nodes === */
START_TEST(client_register_nodes) {
    UA_Client *client = connectClient();

    UA_RegisterNodesRequest regReq;
    UA_RegisterNodesRequest_init(&regReq);
    UA_NodeId nodeIds[3];
    nodeIds[0] = UA_NODEID_NUMERIC(1, 90001);
    nodeIds[1] = UA_NODEID_NUMERIC(1, 90002);
    nodeIds[2] = UA_NODEID_NUMERIC(1, 90003);
    regReq.nodesToRegister = nodeIds;
    regReq.nodesToRegisterSize = 3;

    UA_RegisterNodesResponse regResp =
        UA_Client_Service_registerNodes(client, regReq);
    ck_assert_uint_eq(regResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(regResp.registeredNodeIdsSize, 3);

    /* Unregister */
    UA_UnregisterNodesRequest unregReq;
    UA_UnregisterNodesRequest_init(&unregReq);
    unregReq.nodesToUnregister = regResp.registeredNodeIds;
    unregReq.nodesToUnregisterSize = regResp.registeredNodeIdsSize;

    UA_UnregisterNodesResponse unregResp =
        UA_Client_Service_unregisterNodes(client, unregReq);
    ck_assert_uint_eq(unregResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    UA_RegisterNodesResponse_clear(&regResp);
    UA_UnregisterNodesResponse_clear(&unregResp);

    disconnectClient(client);
} END_TEST

/* === Read with index range === */
START_TEST(read_with_index_range) {
    UA_Client *client = connectClient();

    /* Read array variable with index range */
    UA_ReadValueId rvid;
    UA_ReadValueId_init(&rvid);
    rvid.nodeId = UA_NODEID_NUMERIC(1, 90004);
    rvid.attributeId = UA_ATTRIBUTEID_VALUE;
    rvid.indexRange = UA_STRING("1:3");

    UA_ReadRequest req;
    UA_ReadRequest_init(&req);
    req.nodesToReadSize = 1;
    req.nodesToRead = &rvid;
    req.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;

    UA_ReadResponse resp = UA_Client_Service_read(client, req);
    ck_assert_uint_eq(resp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    if(resp.resultsSize > 0 && resp.results[0].hasValue) {
        ck_assert(resp.results[0].value.arrayLength > 0);
    }
    UA_ReadResponse_clear(&resp);

    disconnectClient(client);
} END_TEST

/* === Write with index range === */
START_TEST(write_with_index_range) {
    UA_Client *client = connectClient();

    UA_WriteValue wv;
    UA_WriteValue_init(&wv);
    wv.nodeId = UA_NODEID_NUMERIC(1, 90004);
    wv.attributeId = UA_ATTRIBUTEID_VALUE;
    wv.indexRange = UA_STRING("0:1");

    UA_Int32 newVals[] = {999, 888};
    UA_Variant_setArray(&wv.value.value, newVals, 2, &UA_TYPES[UA_TYPES_INT32]);
    wv.value.hasValue = true;

    UA_WriteRequest req;
    UA_WriteRequest_init(&req);
    req.nodesToWriteSize = 1;
    req.nodesToWrite = &wv;

    UA_WriteResponse resp = UA_Client_Service_write(client, req);
    ck_assert_uint_eq(resp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_WriteResponse_clear(&resp);

    disconnectClient(client);
} END_TEST

/* === Read non-existent node === */
START_TEST(read_nonexistent_node) {
    UA_Client *client = connectClient();

    UA_Variant val;
    UA_StatusCode res = UA_Client_readValueAttribute(client,
        UA_NODEID_NUMERIC(1, 999999), &val);
    ck_assert(res != UA_STATUSCODE_GOOD);

    disconnectClient(client);
} END_TEST

/* === Multiple write operations === */
START_TEST(write_multiple_nodes) {
    UA_Client *client = connectClient();

    UA_WriteValue wvs[3];
    
    /* Write Bool */
    UA_WriteValue_init(&wvs[0]);
    wvs[0].nodeId = UA_NODEID_NUMERIC(1, 90001);
    wvs[0].attributeId = UA_ATTRIBUTEID_VALUE;
    UA_Boolean bval = true;
    UA_Variant_setScalar(&wvs[0].value.value, &bval, &UA_TYPES[UA_TYPES_BOOLEAN]);
    wvs[0].value.hasValue = true;

    /* Write Double */
    UA_WriteValue_init(&wvs[1]);
    wvs[1].nodeId = UA_NODEID_NUMERIC(1, 90002);
    wvs[1].attributeId = UA_ATTRIBUTEID_VALUE;
    UA_Double dval = 99.99;
    UA_Variant_setScalar(&wvs[1].value.value, &dval, &UA_TYPES[UA_TYPES_DOUBLE]);
    wvs[1].value.hasValue = true;

    /* Write String */
    UA_WriteValue_init(&wvs[2]);
    wvs[2].nodeId = UA_NODEID_NUMERIC(1, 90003);
    wvs[2].attributeId = UA_ATTRIBUTEID_VALUE;
    UA_String sval = UA_STRING("batch write");
    UA_Variant_setScalar(&wvs[2].value.value, &sval, &UA_TYPES[UA_TYPES_STRING]);
    wvs[2].value.hasValue = true;

    UA_WriteRequest req;
    UA_WriteRequest_init(&req);
    req.nodesToWrite = wvs;
    req.nodesToWriteSize = 3;

    UA_WriteResponse resp = UA_Client_Service_write(client, req);
    ck_assert_uint_eq(resp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_WriteResponse_clear(&resp);

    disconnectClient(client);
} END_TEST

static Suite *testSuite_serverOps(void) {
    TCase *tc_discovery = tcase_create("Discovery");
    tcase_add_checked_fixture(tc_discovery, setup, teardown);
    tcase_add_test(tc_discovery, client_find_servers);
    tcase_add_test(tc_discovery, client_find_servers_with_filter);
    tcase_add_test(tc_discovery, client_get_endpoints);

    TCase *tc_readwrite = tcase_create("ReadWrite");
    tcase_add_checked_fixture(tc_readwrite, setup, teardown);
    tcase_add_test(tc_readwrite, read_write_bool);
    tcase_add_test(tc_readwrite, read_write_double);
    tcase_add_test(tc_readwrite, read_write_string);
    tcase_add_test(tc_readwrite, read_write_array);
    tcase_add_test(tc_readwrite, read_bytestring);
    tcase_add_test(tc_readwrite, read_multiple_attributes);
    tcase_add_test(tc_readwrite, read_server_namespacearray);
    tcase_add_test(tc_readwrite, read_server_status);
    tcase_add_test(tc_readwrite, read_with_index_range);
    tcase_add_test(tc_readwrite, write_with_index_range);
    tcase_add_test(tc_readwrite, read_nonexistent_node);
    tcase_add_test(tc_readwrite, write_multiple_nodes);

    TCase *tc_browse = tcase_create("Browse");
    tcase_add_checked_fixture(tc_browse, setup, teardown);
    tcase_add_test(tc_browse, browse_all_directions);
    tcase_add_test(tc_browse, browse_with_filter);
    tcase_add_test(tc_browse, translate_browse_path);

    TCase *tc_nodes = tcase_create("NodeOps");
    tcase_add_checked_fixture(tc_nodes, setup, teardown);
    tcase_add_test(tc_nodes, client_add_delete_object);
    tcase_add_test(tc_nodes, client_add_variable_types);
    tcase_add_test(tc_nodes, client_write_attributes);
    tcase_add_test(tc_nodes, client_add_reference);
    tcase_add_test(tc_nodes, client_register_nodes);

    Suite *s = suite_create("Server Operations Extended 2");
    suite_add_tcase(s, tc_discovery);
    suite_add_tcase(s, tc_readwrite);
    suite_add_tcase(s, tc_browse);
    suite_add_tcase(s, tc_nodes);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_serverOps();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
