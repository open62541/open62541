/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/* This example is just to see how fast we can process messages. The server does
   not open a TCP port. */

#include <open62541/server_config_default.h>
#include <open62541/plugin/nodestore_default.h>

#include "server/ua_services.h"
#include "ua_server_internal.h"
#include "ua_types_encoding_binary.h"

#include <check.h>
#include <time.h>

#include "testing_networklayers.h"
#include "testing_policy.h"

#define READNODES 1000 /* Number of nodes to be created for reading */
#define READS 1000  /* Number of reads to perform */

static UA_Server *server;
static UA_NodeId readNodeIds[READNODES];

static void setup(void) {
    server = UA_Server_new();
}

static void teardown(void) {
    UA_Server_delete(server);
}

START_TEST(readSpeed) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* Add variable nodes to the address space */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    for(size_t i = 0; i < READNODES; i++) {
        char varName[20];
        UA_snprintf(varName, 20, "Variable %u", (UA_UInt32)i);
        UA_NodeId myNodeId = UA_NODEID_STRING(1, varName);
        UA_QualifiedName myName = UA_QUALIFIEDNAME(1, varName);
        retval = UA_Server_addVariableNode(server, myNodeId, parentNodeId,
                                           parentReferenceNodeId, myName,
                                           UA_NODEID_NULL, attr, NULL,
                                           &readNodeIds[i]);
        UA_assert(retval == UA_STATUSCODE_GOOD);
    }

    UA_ByteString request_msg;
    retval |= UA_ByteString_allocBuffer(&request_msg, 1000);
    UA_ByteString response_msg;
    retval |= UA_ByteString_allocBuffer(&response_msg, 1000);
    UA_assert(retval == UA_STATUSCODE_GOOD);

    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    UA_ReadValueId rvi;
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;
    rvi.indexRange = UA_STRING_NULL;
    rvi.dataEncoding = UA_QUALIFIEDNAME(0, "Default Binary");
    request.timestampsToReturn = UA_TIMESTAMPSTORETURN_NEITHER;
    request.nodesToReadSize = 1;
    request.nodesToRead = &rvi;

    UA_ReadResponse res;
    UA_ReadResponse_init(&res);

    clock_t begin, finish;
    begin = clock();

    for(size_t i = 0; i < READS ; i++) {
        /* Set the NodeId */
        rvi.nodeId = readNodeIds[i % READNODES];

        UA_LOCK(&server->serviceMutex);
        Service_Read(server, &server->adminSession, &request, &res);
        UA_UNLOCK(&server->serviceMutex);

        UA_ReadResponse_clear(&res);
    }

    finish = clock();
    ck_assert(retval == UA_STATUSCODE_GOOD);

    double time_spent = (double)(finish - begin) / CLOCKS_PER_SEC;
    printf("duration was %f s\n", time_spent);
    printf("retval is %s\n", UA_StatusCode_name(retval));

    UA_ByteString_clear(&request_msg);
    UA_ByteString_clear(&response_msg);

    for(size_t i = 0; i < READNODES; i++)
        UA_NodeId_clear(&readNodeIds[i]);
}
END_TEST

START_TEST(readSpeedWithEncoding) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* Add variable nodes to the address space */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    for(size_t i = 0; i < READNODES; i++) {
        char varName[20];
        UA_snprintf(varName, 20, "Variable %u", (UA_UInt32)i);
        UA_NodeId myNodeId = UA_NODEID_STRING(1, varName);
        UA_QualifiedName myName = UA_QUALIFIEDNAME(1, varName);
        retval = UA_Server_addVariableNode(server, myNodeId, parentNodeId,
                                           parentReferenceNodeId, myName,
                                           UA_NODEID_NULL, attr, NULL,
                                           &readNodeIds[i]);
        UA_assert(retval == UA_STATUSCODE_GOOD);
    }

    UA_ByteString request_msg;
    retval |= UA_ByteString_allocBuffer(&request_msg, 1000);
    UA_ByteString response_msg;
    retval |= UA_ByteString_allocBuffer(&response_msg, 1000);
    UA_assert(retval == UA_STATUSCODE_GOOD);

    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    UA_ReadValueId rvi;
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;
    rvi.indexRange = UA_STRING_NULL;
    rvi.dataEncoding = UA_QUALIFIEDNAME(0, "Default Binary");
    request.timestampsToReturn = UA_TIMESTAMPSTORETURN_NEITHER;
    request.nodesToReadSize = 1;
    request.nodesToRead = &rvi;

    UA_ReadRequest req;
    UA_ReadResponse res;
    UA_ReadResponse_init(&res);

    clock_t begin, finish;
    begin = clock();

    for(size_t i = 0; i < READS; i++) {
        /* Set the NodeId */
        rvi.nodeId = readNodeIds[i % READNODES];

        /* Encode the request */
        UA_Byte *pos = request_msg.data;
        const UA_Byte *end = &request_msg.data[request_msg.length];
        retval |= UA_encodeBinaryInternal(&request, &UA_TYPES[UA_TYPES_READREQUEST], &pos, &end, NULL, NULL);
        ck_assert(retval == UA_STATUSCODE_GOOD);

        /* Decode the request */
        size_t offset = 0;
        retval |= UA_decodeBinaryInternal(&request_msg, &offset, &req, &UA_TYPES[UA_TYPES_READREQUEST], NULL);

        UA_LOCK(&server->serviceMutex);
        Service_Read(server, &server->adminSession, &req, &res);
        UA_UNLOCK(&server->serviceMutex);

        UA_Byte *rpos = response_msg.data;
        const UA_Byte *rend = &response_msg.data[response_msg.length];
        retval |= UA_encodeBinaryInternal(&res, &UA_TYPES[UA_TYPES_READRESPONSE],
                                  &rpos, &rend, NULL, NULL);

        UA_ReadRequest_clear(&req);
        UA_ReadResponse_clear(&res);
    }

    finish = clock();
    ck_assert(retval == UA_STATUSCODE_GOOD);

    double time_spent = (double)(finish - begin) / CLOCKS_PER_SEC;
    printf("duration with encoding was %f s\n", time_spent);
    printf("retval is %s\n", UA_StatusCode_name(retval));

    UA_ByteString_clear(&request_msg);
    UA_ByteString_clear(&response_msg);

    for(size_t i = 0; i < READNODES; i++)
        UA_NodeId_clear(&readNodeIds[i]);
}
END_TEST

static Suite * service_speed_suite (void) {
    Suite *s = suite_create ("Service Speed");

    TCase* tc_read = tcase_create ("Read");
    tcase_add_checked_fixture(tc_read, setup, teardown);
    tcase_add_test (tc_read, readSpeed);
    tcase_add_test (tc_read, readSpeedWithEncoding);
    suite_add_tcase (s, tc_read);

    return s;
}

int main (void) {
    int number_failed = 0;
    Suite *s = service_speed_suite();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr,CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed (sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
