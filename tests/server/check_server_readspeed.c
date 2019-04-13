/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/* This example is just to see how fast we can process messages. The server does
   not open a TCP port. */

#include <open62541/server_config_default.h>

#include "server/ua_services.h"
#include "ua_server_internal.h"
#include "ua_types_encoding_binary.h"

#include <check.h>
#include <time.h>

#include "testing_networklayers.h"
#include "testing_policy.h"

static UA_SecureChannel testChannel;
static UA_SecurityPolicy dummyPolicy;
static UA_Connection testingConnection;
static funcs_called funcsCalled;
static key_sizes keySizes;

static UA_Server *server;

static void setup(void) {
    server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    TestingPolicy(&dummyPolicy, UA_BYTESTRING_NULL, &funcsCalled, &keySizes);
    UA_SecureChannel_init(&testChannel);
    UA_SecureChannel_setSecurityPolicy(&testChannel, &dummyPolicy, &UA_BYTESTRING_NULL);

    testingConnection = createDummyConnection(65535, NULL);
    UA_Connection_attachSecureChannel(&testingConnection, &testChannel);
    testChannel.connection = &testingConnection;
}

static void teardown(void) {
    UA_SecureChannel_close(&testChannel);
    UA_SecureChannel_deleteMembers(&testChannel);
    dummyPolicy.deleteMembers(&dummyPolicy);
    testingConnection.close(&testingConnection);

    UA_Server_delete(server);
}

START_TEST(readSpeed) {
    /* add a variable node to the address space */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US","the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","the answer");
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_StatusCode retval = UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                                                     parentReferenceNodeId, myIntegerName,
                                                     UA_NODEID_NULL, attr, NULL, NULL);
    UA_assert(retval == UA_STATUSCODE_GOOD);

    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    UA_ReadValueId rvi;
    rvi.nodeId = myIntegerNodeId;
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;
    rvi.indexRange = UA_STRING_NULL;
    rvi.dataEncoding = UA_QUALIFIEDNAME(0, "Default Binary");
    request.timestampsToReturn = UA_TIMESTAMPSTORETURN_NEITHER;
    request.nodesToReadSize = 1;
    request.nodesToRead = &rvi;

    UA_ByteString request_msg;
    retval |= UA_ByteString_allocBuffer(&request_msg, 1000);
    UA_ByteString response_msg;
    retval |= UA_ByteString_allocBuffer(&response_msg, 1000);

    UA_Byte *pos = request_msg.data;
    const UA_Byte *end = &request_msg.data[request_msg.length];
    retval |= UA_encodeBinary(&request, &UA_TYPES[UA_TYPES_READREQUEST], &pos, &end, NULL, NULL);
    UA_assert(retval == UA_STATUSCODE_GOOD);

    UA_ReadRequest rq;
    UA_MessageContext mc;

    UA_ResponseHeader rh;
    UA_ResponseHeader_init(&rh);

    clock_t begin, finish;
    begin = clock();

    for(size_t i = 0; i < 1000000; i++) {
        size_t offset = 0;
        retval |= UA_decodeBinary(&request_msg, &offset, &rq, &UA_TYPES[UA_TYPES_READREQUEST], NULL);

        UA_MessageContext_begin(&mc, &testChannel, 0, UA_MESSAGETYPE_MSG);
        retval |= Service_Read(server, &server->adminSession, &mc, &rq, &rh);
        UA_MessageContext_finish(&mc);

        UA_ReadRequest_deleteMembers(&rq);
    }

    finish = clock();

    UA_assert(retval == UA_STATUSCODE_GOOD);
    double time_spent = (double)(finish - begin) / CLOCKS_PER_SEC;
    printf("duration was %f s\n", time_spent);
    printf("retval is %s\n", UA_StatusCode_name(retval));

    UA_ByteString_deleteMembers(&request_msg);
    UA_ByteString_deleteMembers(&response_msg);
}
END_TEST

static Suite * service_speed_suite (void) {
    Suite *s = suite_create ("Service Speed");

    TCase* tc_read = tcase_create ("Read");
    tcase_add_checked_fixture(tc_read, setup, teardown);
    tcase_add_test (tc_read, readSpeed);
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
