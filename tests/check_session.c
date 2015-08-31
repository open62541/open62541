/*
 * check_session.c
 *
 *  Created on: Jul 30, 2015
 *      Author: opcua
 */


#include <stdio.h>
#include <stdlib.h>

#include "ua_types.h"
#include "server/ua_services.h"
#include "ua_statuscodes.h"
#include "check.h"
#include "ua_util.h"


START_TEST(Session_init_ShallWork)
{
	UA_Session session;
	UA_Session_init(&session);


    UA_NodeId tmpNodeId;
    UA_NodeId_init(&tmpNodeId);
    UA_ApplicationDescription tmpAppDescription;
    UA_ApplicationDescription_init(&tmpAppDescription);
    UA_DateTime tmpDateTime;
    UA_DateTime_init(&tmpDateTime);
	ck_assert_int_eq(session.activated,UA_FALSE);
	ck_assert_int_eq(session.authenticationToken.identifier.numeric,tmpNodeId.identifier.numeric);
	ck_assert_int_eq(session.availableContinuationPoints,MAXCONTINUATIONPOINTS);
    ck_assert_int_eq(session.channel,UA_NULL);
    ck_assert_int_eq(session.clientDescription.applicationName.locale.data,UA_NULL);
    ck_assert_int_eq(session.continuationPoints.lh_first, UA_NULL);
    ck_assert_int_eq(session.maxRequestMessageSize,0);
    ck_assert_int_eq(session.maxResponseMessageSize,0);
    ck_assert_int_eq(session.sessionId.identifier.numeric,tmpNodeId.identifier.numeric);
    ck_assert_int_eq(session.sessionName.data,UA_NULL);
    ck_assert_int_eq(session.timeout,0);
    ck_assert_int_eq(session.validTill,tmpDateTime);


	//finally
}
END_TEST

START_TEST(Session_updateLifetime_ShallWork)
{
	UA_Session session;
	UA_Session_init(&session);
    UA_DateTime tmpDateTime;
    tmpDateTime = session.validTill;
	UA_Session_updateLifetime(&session);

	UA_Int32 result = (session.validTill > tmpDateTime);

	ck_assert_int_gt(result,0);



	//finally
}
END_TEST

static Suite* testSuite_Session(void) {
	Suite *s = suite_create("Session");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, Session_init_ShallWork);
	tcase_add_test(tc_core, Session_updateLifetime_ShallWork);

	suite_add_tcase(s,tc_core);
	return s;
}

int main(void) {
	int number_failed = 0;

	Suite *s;
	SRunner *sr;

	s = testSuite_Session();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}


