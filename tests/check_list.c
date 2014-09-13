#include <stdlib.h> // EXIT_SUCCESS
#include "util/ua_util.h"
#include "util/ua_list.h"
#include "check.h"

/* global test counters */
UA_Int32 visit_count = 0;
UA_Int32 free_count = 0;

void elementVisitor(UA_list_Element* payload){
	visit_count++;
}

void visitor(void* payload){
	visit_count++;
}

void freer(void* payload){
	if(payload){
		free_count++;
		UA_free(payload);
	}
}

_Bool matcher(void* payload){
	if(payload == UA_NULL){
		return UA_FALSE;
	}
	if(*((UA_Int32*)payload) == 42){
		return UA_TRUE;
	}
	return UA_FALSE;
}

_Bool matcher2(void* payload){
	if(payload == UA_NULL){
		return UA_FALSE;
	}
	if(*((UA_Int32*)payload) == 43){
		return UA_TRUE;
	}
	return UA_FALSE;
}

_Bool comparer(void* payload, void* otherPayload) {
	if(payload == UA_NULL || otherPayload == UA_NULL){
		return UA_FALSE;
	} else {
		return ( *((UA_Int32*)payload) == *((UA_Int32*)otherPayload) );
	}
}

START_TEST(list_test_basic)
{

	UA_list_List list;
	UA_list_init(&list);


	ck_assert_int_eq(UA_list_addPayloadToFront(UA_NULL, UA_NULL), UA_ERROR);
	ck_assert_int_eq(UA_list_addElementToBack(UA_NULL, UA_NULL), UA_ERROR);
	ck_assert_int_eq(UA_list_addElementToFront(UA_NULL, UA_NULL), UA_ERROR);
	ck_assert_int_eq(UA_list_addPayloadToBack(UA_NULL, UA_NULL), UA_ERROR);
	ck_assert_int_eq(UA_list_removeFirst(UA_NULL, UA_NULL), UA_ERROR);
	ck_assert_int_eq(UA_list_removeLast(UA_NULL, UA_NULL), UA_ERROR);
	ck_assert_int_eq(UA_list_removeElement(UA_NULL, UA_NULL), UA_ERROR);
	ck_assert_int_eq(UA_list_destroy(UA_NULL, UA_NULL), UA_ERROR);
	ck_assert_int_eq(UA_list_iterateElement(UA_NULL, UA_NULL), UA_ERROR);
	ck_assert_int_eq(UA_list_iteratePayload(UA_NULL, UA_NULL), UA_ERROR);

	ck_assert_int_eq(list.size, 0);

	UA_Int32* payload;
	UA_alloc((void**)&payload, sizeof(*payload));
	*payload = 42;
	UA_list_addPayloadToFront(&list, payload);
	UA_alloc((void**)&payload, sizeof(*payload));
	*payload = 24;
	UA_list_addPayloadToFront(&list, payload);
	UA_alloc((void**)&payload, sizeof(*payload));
	*payload = 1;
	UA_list_addPayloadToBack(&list, payload);

	ck_assert_int_eq(*(UA_Int32*)UA_list_getFirst(&list)->payload, 24);
	ck_assert_int_eq(*(UA_Int32*)UA_list_getLast(&list)->payload, 1);

	ck_assert_int_eq(list.size, 3);

	visit_count = 0;
	UA_list_iteratePayload(&list, visitor);
	ck_assert_int_eq(visit_count, 3);

	visit_count = 0;
	UA_list_iterateElement(&list, elementVisitor);
	ck_assert_int_eq(visit_count, 3);

	UA_list_Element* elem = NULL;
	elem = UA_list_find(&list, matcher);
	if(elem){
		ck_assert_int_eq((*((UA_Int32*)elem->payload)), 42);
		UA_list_removeElement(elem, freer);
		ck_assert_int_eq(free_count, 1);
		free_count = 0; //reset free counter
		ck_assert_int_eq(list.size, 2);
	}else{
		fail("Element 42 not found");
	}

	//search for a non-existent element
	ck_assert_ptr_eq((UA_list_find(&list, matcher2)), UA_NULL);

	UA_list_destroy(&list, freer);

	ck_assert_int_eq(free_count, 2);

	ck_assert_int_eq(list.size, 0);

}
END_TEST

void myAddPayloadValueToFront(UA_list_List *list, UA_Int32 payloadValue) {
	UA_Int32* payload;
	UA_alloc((void**)&payload, sizeof(*payload));
	*payload = payloadValue;
	UA_list_addPayloadToFront(list, payload);
}
void myAddPayloadVectorToFront(UA_list_List *list, UA_Int32 payloadValues[], UA_Int32 n) {
	UA_Int32 i = 0;
	for (;i<n;i++) {
		myAddPayloadValueToFront(list,payloadValues[i]);
	}
}

START_TEST(addElementsShallResultInRespectiveSize)
{
	// given
	UA_list_List list;
	UA_list_init(&list);
	// when
	UA_Int32 plv[] = {42,24,1};
	myAddPayloadVectorToFront(&list,plv,sizeof(plv)/sizeof(UA_Int32));
	// then
	ck_assert_int_eq(list.size, 3);
	// finally
	UA_list_destroy(&list, freer);
}
END_TEST

START_TEST(findElementShallFind42)
{
	// given
	UA_list_List list;
	UA_list_init(&list);
	UA_Int32 plv[] = {42,24,1};
	myAddPayloadVectorToFront(&list,plv,sizeof(plv)/sizeof(UA_Int32));
	// when
	UA_list_Element* e = UA_list_find(&list,matcher);
	// then
	ck_assert_ptr_ne(e, UA_NULL);
	ck_assert_int_eq(*(UA_Int32*)(e->payload), 42);
	// finally
	UA_list_destroy(&list, freer);
}
END_TEST

START_TEST(searchElementShallFind24)
{
	// given
	UA_list_List list;
	UA_list_init(&list);
	UA_Int32 plv[] = {42,24,1};
	myAddPayloadVectorToFront(&list,plv,sizeof(plv)/sizeof(UA_Int32));
	UA_Int32 payload = 24;
	// when
	UA_list_Element* e = UA_list_search(&list,comparer,(void*)&payload);
	// then
	ck_assert_ptr_ne(e, UA_NULL);
	ck_assert_int_eq(*(UA_Int32*)(e->payload), 24);
	// finally
	UA_list_destroy(&list, freer);
}
END_TEST

START_TEST(addAndRemoveShallYieldEmptyList)
{
	// given
	UA_list_List list;
	UA_list_init(&list);
	UA_Int32 plv[] = {42};
	myAddPayloadVectorToFront(&list,plv,sizeof(plv)/sizeof(UA_Int32));
	// when
	UA_list_Element* e = UA_list_search(&list,comparer,(void*)&plv[0]);
	UA_list_removeElement(e,visitor);
	ck_assert_int_eq(visit_count,1);
	visit_count = 0;
	UA_list_iteratePayload(&list,visitor);
	// then
	ck_assert_int_eq(list.size,0);
	ck_assert_int_eq(visit_count,0);
	// finally
	UA_list_destroy(&list, freer);
}
END_TEST

START_TEST(addAndRemoveShallYieldEmptyList2)
{
	// given
	UA_list_List list;
	UA_list_init(&list);
	UA_Int32 plv[] = {42};
	myAddPayloadVectorToFront(&list,plv,sizeof(plv)/sizeof(UA_Int32));
	// when
	UA_list_search(&list,comparer,(void*)&plv[0]);
	UA_list_removeLast(&list,visitor);  //additionally testing removeLast explicitly
	ck_assert_int_eq(visit_count,1);
	visit_count = 0;
	UA_list_iterateElement(&list,elementVisitor); //additionally testing iterateElement
	// then
	ck_assert_int_eq(list.size,0);
	ck_assert_int_eq(visit_count,0);
	// finally
	UA_list_destroy(&list, freer);
}
END_TEST

START_TEST(addTwiceAndRemoveFirstShallYieldListWithOneElement)
{
	// given
	UA_list_List list;
	UA_list_init(&list);
	UA_Int32 plv[] = {42,24};
	myAddPayloadVectorToFront(&list,plv,sizeof(plv)/sizeof(UA_Int32));
	// when
	UA_list_Element* e = UA_list_search(&list,comparer,(void*)&plv[0]);
	UA_list_removeElement(e,UA_NULL);
	visit_count = 0;
	UA_list_iteratePayload(&list,visitor);
	// then
	ck_assert_int_eq(list.size,1);
	ck_assert_int_eq(visit_count,1);
	// finally
	UA_list_destroy(&list, freer);
}
END_TEST

START_TEST(addTwiceAndRemoveLastShallYieldListWithOneElement)
{
	// given
	UA_list_List list;
	UA_list_init(&list);
	UA_Int32 plv[] = {42,24};
	myAddPayloadVectorToFront(&list,plv,sizeof(plv)/sizeof(UA_Int32));
	// when
	UA_list_Element* e = UA_list_search(&list,comparer,(void*)&plv[1]);
	UA_list_removeElement(e,UA_NULL);
	visit_count = 0;
	UA_list_iteratePayload(&list,visitor);
	// then
	ck_assert_int_eq(list.size,1);
	ck_assert_int_eq(visit_count,1);
	// finally
	UA_list_destroy(&list, freer);
}
END_TEST

Suite*list_testSuite(void)
{
	Suite *s = suite_create("list_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, list_test_basic);
	tcase_add_test(tc_core, addElementsShallResultInRespectiveSize);
	tcase_add_test(tc_core, findElementShallFind42);
	tcase_add_test(tc_core, searchElementShallFind24);
	tcase_add_test(tc_core, addAndRemoveShallYieldEmptyList);
	tcase_add_test(tc_core, addAndRemoveShallYieldEmptyList2);
	tcase_add_test(tc_core, addTwiceAndRemoveFirstShallYieldListWithOneElement);
	tcase_add_test(tc_core, addTwiceAndRemoveLastShallYieldListWithOneElement);
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

