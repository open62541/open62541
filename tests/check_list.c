#include "UA_config.h"
#include "UA_list.h"

#include "check.h"

/* global test counters */
Int32 visit_count = 0;
Int32 free_count = 0;

void visitor(void* payload){
	visit_count++;
}

void freer(void* payload){
	if(payload){
		free_count++;
		opcua_free(payload);
	}
}

Boolean matcher(void* payload){
	if(payload == NULL){
		return FALSE;
	}
	if(*((Int32*)payload) == 42){
		return TRUE;
	}
	return FALSE;
}

START_TEST(list_test_basic)
{

	UA_list_List list;
	UA_list_init(&list);

	ck_assert_int_eq(list.size, 0);

	Int32* payload = (Int32*)opcua_malloc(sizeof(*payload));
	*payload = 42;
	UA_list_addPayloadToFront(&list, payload);
	payload = (Int32*)opcua_malloc(sizeof(*payload));
	*payload = 24;
	UA_list_addPayloadToFront(&list, payload);
	payload = (Int32*)opcua_malloc(sizeof(*payload));
	*payload = 1;
	UA_list_addPayloadToBack(&list, payload);

	ck_assert_int_eq(list.size, 3);

	UA_list_iteratePayload(&list, visitor);

	ck_assert_int_eq(visit_count, 3);

	UA_list_Element* elem = NULL;
	elem = UA_list_find(&list, matcher);
	if(elem){
		ck_assert_int_eq((*((Int32*)elem->payload)), 42);
		UA_list_removeElement(elem, freer);
		ck_assert_int_eq(free_count, 1);
		free_count = 0; //reset free counter
		ck_assert_int_eq(list.size, 2);
	}else{
		fail("Element 42 not found");
	}
	UA_list_destroy(&list, freer);

	ck_assert_int_eq(free_count, 2);

	ck_assert_int_eq(list.size, 0);

}
END_TEST

Suite*list_testSuite(void)
{
	Suite *s = suite_create("list_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, list_test_basic);
	suite_add_tcase(s,tc_core);
	return s;
}

int main (void)
{
	int number_failed = 0;

	Suite* s = list_testSuite();
	SRunner* sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;

}

