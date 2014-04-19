/*
 * check_memory.c
 *
 *  Created on: 10.04.2014
 *      Author: mrt
 */

#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>

#include "opcua.h"
#include "check.h"

START_TEST (newAndEmptyObjectShallBeDeleted)
{
	// given
	UA_Int32 retval;
	void* obj;
	// when
	retval = UA_[_i].new(&obj);
	retval |= UA_[_i].delete(obj);
	// then
	ck_assert_int_eq(retval,UA_SUCCESS);
}
END_TEST

START_TEST (arrayCopyShallMakeADeepCopy)
{
	// given
	void **a1; UA_Array_new((void**)&a1,3,UA_STRING);
	UA_String_copycstring("a",((UA_String **)a1)[0]);
	UA_String_copycstring("bb",((UA_String **)a1)[1]);
	UA_String_copycstring("ccc",((UA_String **)a1)[2]);
	// when
	void **a2;
	UA_Int32 retval = UA_Array_copy((void const*const*)&a1,3,UA_STRING,(void**)&a2);
	// then
	ck_assert_int_eq(retval,UA_SUCCESS);
	ck_assert_int_eq(((UA_String **)a1)[0]->length,1);
	ck_assert_int_eq(((UA_String **)a1)[1]->length,2);
	ck_assert_int_eq(((UA_String **)a1)[2]->length,3);
	ck_assert_int_eq(((UA_String **)a1)[0]->length,((UA_String **)a2)[0]->length);
	ck_assert_int_eq(((UA_String **)a1)[1]->length,((UA_String **)a2)[1]->length);
	ck_assert_int_eq(((UA_String **)a1)[2]->length,((UA_String **)a2)[2]->length);
	ck_assert_ptr_ne(((UA_String **)a1)[0]->data,((UA_String **)a2)[0]->data);
	ck_assert_ptr_ne(((UA_String **)a1)[1]->data,((UA_String **)a2)[1]->data);
	ck_assert_ptr_ne(((UA_String **)a1)[2]->data,((UA_String **)a2)[2]->data);
	ck_assert_int_eq(((UA_String **)a1)[0]->data[0],((UA_String **)a2)[0]->data[0]);
	ck_assert_int_eq(((UA_String **)a1)[1]->data[0],((UA_String **)a2)[1]->data[0]);
	ck_assert_int_eq(((UA_String **)a1)[2]->data[0],((UA_String **)a2)[2]->data[0]);
	// finally
	UA_Array_delete(a1,3,UA_STRING);
	UA_Array_delete(a2,3,UA_STRING);
}
END_TEST

START_TEST (encodeShallYieldDecode)
{
	// given
	void *obj1 = UA_NULL, *obj2 = UA_NULL;
	UA_ByteString msg1, msg2;
	UA_Int32 retval, pos = 0;
	retval = UA_[_i].new(&obj1);
	UA_ByteString_newMembers(&msg1,UA_[_i].calcSize(obj1));
	retval = UA_[_i].encodeBinary(obj1, &pos, &msg1);
	// when
	UA_[_i].new(&obj2);
	pos = 0; retval = UA_[_i].decodeBinary(&msg1, &pos, obj2);
	UA_ByteString_newMembers(&msg2,UA_[_i].calcSize(obj2));
	pos = 0; retval = UA_[_i].encodeBinary(obj2, &pos, &msg2);
	// then
	ck_assert_msg(UA_ByteString_compare(&msg1,&msg2)==0,"messages differ idx=%d,name=%s",_i,UA_[_i].name);
	ck_assert_int_eq(retval,UA_SUCCESS);
	// finally
	UA_[_i].delete(obj1);
	UA_[_i].delete(obj2);
	UA_ByteString_deleteMembers(&msg1);
	UA_ByteString_deleteMembers(&msg2);
}
END_TEST

START_TEST (decodeShallFailWithTruncatedBufferButSurvive)
{
	// given
	void *obj1 = UA_NULL, *obj2 = UA_NULL;
	UA_ByteString msg1;
	UA_Int32 retval, pos;
	retval = UA_[_i].new(&obj1);
	UA_ByteString_newMembers(&msg1,UA_[_i].calcSize(obj1));
	pos = 0; retval = UA_[_i].encodeBinary(obj1, &pos, &msg1);
	UA_[_i].delete(obj1);
	// when
	UA_[_i].new(&obj2);
	pos = 0;
	msg1.length = msg1.length / 2;
	retval = UA_[_i].decodeBinary(&msg1, &pos, obj2);
	//then
	ck_assert_msg(retval!=UA_SUCCESS,"testing %s with half buffer",UA_[_i].name);
	// finally
	UA_[_i].delete(obj2);
	UA_ByteString_deleteMembers(&msg1);
}
END_TEST

START_TEST (fuzzDecodeWithRandomBuffer)
{
	// given
	void *obj1 = UA_NULL;
	UA_ByteString msg1;
	UA_Int32 retval, buflen;
	buflen = 256;
	retval = UA_[_i].new(&obj1);
	UA_ByteString_newMembers(&msg1,buflen); // fixed size
	// when
	srandom(42);
	memset(msg1.data, random(), buflen); // use the same random number throughout
	//then
	ck_assert_msg(retval==UA_SUCCESS,"Decoding %s from random buffer",UA_[_i].name);
	UA_Int32 pos = 0; retval = UA_[_i].decodeBinary(&msg1, &pos, obj1);
	// finally
	UA_[_i].delete(obj1);
	UA_ByteString_deleteMembers(&msg1);
}
END_TEST

int main() {
	int number_failed = 0;
	SRunner *sr;

	Suite *s = suite_create("testMemoryHandling");
	TCase *tc = tcase_create("Empty Objects");
	tcase_add_loop_test(tc, newAndEmptyObjectShallBeDeleted,UA_BOOLEAN,UA_INVALIDTYPE-1);
	tcase_add_test(tc, arrayCopyShallMakeADeepCopy);
	tcase_add_loop_test(tc, encodeShallYieldDecode,UA_BOOLEAN,UA_INVALIDTYPE-1);
	suite_add_tcase(s,tc);
	tc = tcase_create("Truncated Buffers");
	tcase_add_loop_test(tc, decodeShallFailWithTruncatedBufferButSurvive,UA_BOOLEAN,UA_INVALIDTYPE-1);
	suite_add_tcase(s,tc);

	tc = tcase_create("Fuzzing with Random Buffers");
	tcase_add_loop_test(tc, fuzzDecodeWithRandomBuffer,UA_BOOLEAN,UA_INVALIDTYPE-1);
	suite_add_tcase(s,tc);

	sr = srunner_create(s);
	//for debugging puposes only, will break make check
	//srunner_set_fork_status(sr,CK_NOFORK);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
