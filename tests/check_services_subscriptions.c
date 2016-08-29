#include "ua_server.h"
#include "server/ua_services.h"
#include "server/ua_server_internal.h"
#include "server/ua_subscription.h"
#include "ua_config_standard.h"

#include "check.h"
#include <unistd.h>

UA_Server *server = NULL;

static void setup(void) {
    server = UA_Server_new(UA_ServerConfig_standard);
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(Server_createSubscription) {
    /* Create a subscription */
    UA_CreateSubscriptionRequest request;
    UA_CreateSubscriptionRequest_init(&request);
    request.publishingEnabled = true;

    UA_CreateSubscriptionResponse response;
    UA_CreateSubscriptionResponse_init(&response);

    Service_CreateSubscription(server, &adminSession, &request, &response);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subscriptionId = response.subscriptionId;

    UA_CreateSubscriptionResponse_deleteMembers(&response);

    /* Remove the subscription */
    UA_DeleteSubscriptionsRequest del_request;
    UA_DeleteSubscriptionsRequest_init(&del_request);
    del_request.subscriptionIdsSize = 1;
    del_request.subscriptionIds = &subscriptionId;

    UA_DeleteSubscriptionsResponse del_response;
    UA_DeleteSubscriptionsResponse_init(&del_response);

    Service_DeleteSubscriptions(server, &adminSession, &del_request, &del_response);
    ck_assert_uint_eq(del_response.resultsSize, 1);
    ck_assert_uint_eq(del_response.results[0], UA_STATUSCODE_GOOD);

    UA_DeleteSubscriptionsResponse_deleteMembers(&del_response);
}
END_TEST

START_TEST(Server_publishCallback) {
    /* Create a subscription */
    UA_CreateSubscriptionRequest request;
    UA_CreateSubscriptionResponse response;

    UA_CreateSubscriptionRequest_init(&request);
    request.publishingEnabled = true;
    UA_CreateSubscriptionResponse_init(&response);
    Service_CreateSubscription(server, &adminSession, &request, &response);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subscriptionId1 = response.subscriptionId;
    UA_CreateSubscriptionResponse_deleteMembers(&response);

    /* Create a second subscription */
    UA_CreateSubscriptionRequest_init(&request);
    request.publishingEnabled = true;
    UA_CreateSubscriptionResponse_init(&response);
    Service_CreateSubscription(server, &adminSession, &request, &response);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subscriptionId2 = response.subscriptionId;
    UA_Double publishingInterval = response.revisedPublishingInterval;
    ck_assert(publishingInterval > 0.0f);
    UA_CreateSubscriptionResponse_deleteMembers(&response);

    /* Sleep until the publishing interval times out */
    usleep((useconds_t)(publishingInterval * 1000) + 1000);


    UA_Subscription *sub;
    LIST_FOREACH(sub, &adminSession.serverSubscriptions, listEntry)
        ck_assert_uint_eq(sub->currentKeepAliveCount, 0);

    UA_Server_run_iterate(server, false);

    LIST_FOREACH(sub, &adminSession.serverSubscriptions, listEntry)
        ck_assert_uint_eq(sub->currentKeepAliveCount, 1);

    /* Remove the subscriptions */
    UA_DeleteSubscriptionsRequest del_request;
    UA_DeleteSubscriptionsRequest_init(&del_request);
    UA_UInt32 removeIds[2] = {subscriptionId1, subscriptionId2};
    del_request.subscriptionIdsSize = 2;
    del_request.subscriptionIds = removeIds;

    UA_DeleteSubscriptionsResponse del_response;
    UA_DeleteSubscriptionsResponse_init(&del_response);

    Service_DeleteSubscriptions(server, &adminSession, &del_request, &del_response);
    ck_assert_uint_eq(del_response.resultsSize, 2);
    ck_assert_uint_eq(del_response.results[0], UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(del_response.results[1], UA_STATUSCODE_GOOD);

    UA_DeleteSubscriptionsResponse_deleteMembers(&del_response);
}
END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Server Subscription");
    TCase *tc_server = tcase_create("Server Subscription Basic");
    tcase_add_checked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Server_createSubscription);
    tcase_add_test(tc_server, Server_publishCallback);
    suite_add_tcase(s, tc_server);
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
