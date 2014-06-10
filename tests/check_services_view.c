/*
 ============================================================================
 Name        : check_stack.c
 Author      :
 Version     :
 Copyright   : Your copyright notice
 Description :
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>

#include "ua_types.h"
#include "ua_services.h"
#include "ua_statuscodes.h"
#include "check.h"

START_TEST(Service_TranslateBrowsePathsToNodeIds_SmokeTest)
{
	UA_TranslateBrowsePathsToNodeIdsRequest request;
	UA_TranslateBrowsePathsToNodeIdsRequest_init(&request);

	UA_TranslateBrowsePathsToNodeIdsResponse response;
	UA_TranslateBrowsePathsToNodeIdsResponse_init(&response);

	request.browsePathsSize = 1;
	UA_Array_new((void**)&request.browsePaths,request.browsePathsSize, UA_BROWSEPATH);

	Service_TranslateBrowsePathsToNodeIds(UA_NULL,&request,&response);

	ck_assert_int_eq(response.resultsSize,request.browsePathsSize);
	ck_assert_int_eq(response.results[0].statusCode,UA_STATUSCODE_BADQUERYTOOCOMPLEX);

	//finally
	UA_TranslateBrowsePathsToNodeIdsRequest_deleteMembers(&request);
	UA_TranslateBrowsePathsToNodeIdsResponse_deleteMembers(&response);
}
END_TEST

Suite* testSuite_Service_TranslateBrowsePathsToNodeIds()
{
	Suite *s = suite_create("Service_TranslateBrowsePathsToNodeIds");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, Service_TranslateBrowsePathsToNodeIds_SmokeTest);
	suite_add_tcase(s,tc_core);
	return s;
}

int main (void)
{
	int number_failed = 0;

	Suite *s;
	SRunner *sr;


	s = testSuite_Service_TranslateBrowsePathsToNodeIds();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}


