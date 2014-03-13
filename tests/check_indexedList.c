#include "UA_config.h"
#include "UA_indexedList.h"

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

START_TEST(linkedList_test_basic)
{

	UA_indexedList_List list;
	UA_indexedList_init(&list);

	ck_assert_int_eq(list.size, 0);

	Int32* payload = (Int32*)malloc(sizeof(*payload));
	*payload = 10;
	UA_indexedList_addValue(&list, 1, payload);
	payload = (Int32*)malloc(sizeof(*payload));
	*payload = 20;
	UA_indexedList_addValueToFront(&list, 2, payload);
	payload = (Int32*)malloc(sizeof(*payload));
	*payload = 30;
	UA_indexedList_addValue(&list, 3, payload);

	ck_assert_int_eq(list.size, 3);

	UA_indexedList_iterateValues(&list, visitor);
	ck_assert_int_eq(visit_count, 3);
	visit_count = 0;

	payload = (Int32*)UA_indexedList_findValue(&list, 2);
	if(payload){
		ck_assert_int_eq(*payload, 20);
	}else{
		fail("Element 20 not found");
	}

	UA_indexedList_removeElement(&list, UA_indexedList_find(&list, 2), freer);
	ck_assert_int_eq(free_count, 1);
	free_count=0;
	ck_assert_int_eq(list.size, 2);

	payload = (Int32*)UA_indexedList_findValue(&list, 2);
	if(payload){
		fail("Previously removed element 20 found");
	}

	UA_indexedList_iterateValues(&list, visitor);
	ck_assert_int_eq(visit_count, 2);

	UA_indexedList_destroy(&list, freer);

	ck_assert_int_eq(free_count, 2);
	ck_assert_int_eq(list.size, 0);
}
END_TEST

Suite*linkedList_testSuite(void)
{
	Suite *s = suite_create("linkedList_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, linkedList_test_basic);
	suite_add_tcase(s,tc_core);
	return s;
}

int main (void)
{
	int number_failed = 0;

	Suite* s = linkedList_testSuite();
	SRunner* sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;

}

