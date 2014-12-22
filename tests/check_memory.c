#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>

#include "ua_types.h"
#include "ua_util.h"
#include "ua_namespace_0.h"
#include "check.h"

START_TEST(newAndEmptyObjectShallBeDeleted) {
	// given
	void *obj = UA_TYPES[_i].new();
	// when
#ifdef DEBUG //no print functions if not in debug mode
	UA_TYPES[_i].print(obj, stdout);
#endif
	// then
	ck_assert_ptr_ne(obj, UA_NULL);
    // finally
	UA_TYPES[_i].delete(obj);
}
END_TEST

START_TEST(arrayCopyShallMakeADeepCopy) {
	// given
	UA_String a1[3];
	a1[0] = (UA_String){1, (UA_Byte*)"a"};
	a1[1] = (UA_String){2, (UA_Byte*)"bb"};
	a1[2] = (UA_String){3, (UA_Byte*)"ccc"};
	// when
	UA_String *a2;
	UA_Int32   retval = UA_Array_copy((const void *)a1, 3, &UA_TYPES[UA_STRING], (void **)&a2);
	// then
	ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
	ck_assert_int_eq(a1[0].length, 1);
	ck_assert_int_eq(a1[1].length, 2);
	ck_assert_int_eq(a1[2].length, 3);
	ck_assert_int_eq(a1[0].length, a2[0].length);
	ck_assert_int_eq(a1[1].length, a2[1].length);
	ck_assert_int_eq(a1[2].length, a2[2].length);
	ck_assert_ptr_ne(a1[0].data, a2[0].data);
	ck_assert_ptr_ne(a1[1].data, a2[1].data);
	ck_assert_ptr_ne(a1[2].data, a2[2].data);
	ck_assert_int_eq(a1[0].data[0], a2[0].data[0]);
	ck_assert_int_eq(a1[1].data[0], a2[1].data[0]);
	ck_assert_int_eq(a1[2].data[0], a2[2].data[0]);
	// finally
	UA_Array_delete((void *)a2, 3, &UA_TYPES[UA_STRING]);
}
END_TEST

START_TEST(encodeShallYieldDecode) {
	// given
	UA_ByteString msg1, msg2;
	UA_UInt32     pos = 0;
	void *obj1 = UA_TYPES[_i].new();
	UA_ByteString_newMembers(&msg1, UA_TYPES[_i].encodings[UA_ENCODING_BINARY].calcSize(obj1));
	UA_StatusCode retval = UA_TYPES[_i].encodings[UA_ENCODING_BINARY].encode(obj1, &msg1, &pos);
	if(retval != UA_STATUSCODE_GOOD) {
		// this happens, e.g. when we encode a variant (with UA_TYPES[UA_INVALIDTYPE] in the vtable)
		UA_TYPES[_i].delete(obj1);
		UA_ByteString_deleteMembers(&msg1);
		return;	
	}

	// when
	void *obj2 = UA_TYPES[_i].new();
	pos = 0; retval = UA_TYPES[_i].encodings[UA_ENCODING_BINARY].decode(&msg1, &pos, obj2);
	ck_assert_msg(retval == UA_STATUSCODE_GOOD, "messages differ idx=%d,name=%s", _i, UA_TYPES[_i].name);
	retval = UA_ByteString_newMembers(&msg2, UA_TYPES[_i].encodings[UA_ENCODING_BINARY].calcSize(obj2));
	ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
	pos = 0; retval = UA_TYPES[_i].encodings[UA_ENCODING_BINARY].encode(obj2, &msg2, &pos);
	ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

	// then
	ck_assert_msg(UA_ByteString_equal(&msg1, &msg2) == UA_TRUE, "messages differ idx=%d,name=%s", _i, UA_TYPES[_i].name);

	// finally
	UA_TYPES[_i].delete(obj1);
	UA_TYPES[_i].delete(obj2);
	UA_ByteString_deleteMembers(&msg1);
	UA_ByteString_deleteMembers(&msg2);
}
END_TEST

START_TEST(decodeShallFailWithTruncatedBufferButSurvive) {
	// given
	UA_ByteString msg1;
	UA_UInt32 pos;
	void *obj1 = UA_TYPES[_i].new();
	UA_ByteString_newMembers(&msg1, UA_TYPES[_i].encodings[0].calcSize(obj1));
	pos = 0; UA_TYPES[_i].encodings[0].encode(obj1, &msg1, &pos);
	UA_TYPES[_i].delete(obj1);
	// when
	void *obj2 = UA_TYPES[_i].new();
	pos = 0;
	msg1.length = msg1.length / 2;
	//fprintf(stderr,"testing %s with half buffer\n",UA_TYPES[_i].name);
	UA_TYPES[_i].encodings[0].decode(&msg1, &pos, obj2);
	//then
	// finally
	//fprintf(stderr,"delete %s with half buffer\n",UA_TYPES[_i].name);
	UA_TYPES[_i].delete(obj2);
	UA_ByteString_deleteMembers(&msg1);
}
END_TEST

#define RANDOM_TESTS 1000

START_TEST(decodeScalarBasicTypeFromRandomBufferShallSucceed) {
	// given
	void *obj1 = UA_NULL;
	UA_ByteString msg1;
	UA_Int32 retval = UA_STATUSCODE_GOOD;
	UA_Int32 buflen = 256;
	UA_ByteString_newMembers(&msg1, buflen); // fixed size
#ifdef _WIN32
	srand(42);
#else
	srandom(42);
#endif
	for(int n = 0;n < RANDOM_TESTS;n++) {
		for(UA_Int32 i = 0;i < buflen;i++) {
#ifdef _WIN32
			UA_UInt32 rnd;
			rnd = rand();
			msg1.data[i] = rnd;
#else
			msg1.data[i] = (UA_Byte)random();  // when
#endif
		}
		UA_UInt32 pos = 0;
		obj1 = UA_TYPES[_i].new();
		retval |= UA_TYPES[_i].encodings[0].decode(&msg1, &pos, obj1);
		//then
		ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Decoding %s from random buffer", UA_TYPES[_i].name);
		// finally
		UA_TYPES[_i].delete(obj1);
	}
	UA_ByteString_deleteMembers(&msg1);
}
END_TEST

START_TEST(decodeComplexTypeFromRandomBufferShallSurvive) {
	// given
	UA_ByteString msg1;
	UA_Int32 retval = UA_STATUSCODE_GOOD;
	UA_Int32 buflen = 256;
	UA_ByteString_newMembers(&msg1, buflen); // fixed size
#ifdef _WIN32
	srand(42);
#else
	srandom(42);
#endif
	// when
	for(int n = 0;n < RANDOM_TESTS;n++) {
		for(UA_Int32 i = 0;i < buflen;i++) {
#ifdef _WIN32
			UA_UInt32 rnd;
			rnd = rand();
			msg1.data[i] = rnd;
#else
			msg1.data[i] = (UA_Byte)random();  // when
#endif
		}
		UA_UInt32 pos = 0;
		void *obj1 = UA_TYPES[_i].new();
		retval |= UA_TYPES[_i].encodings[0].decode(&msg1, &pos, obj1);
		UA_TYPES[_i].delete(obj1);
	}

	// finally
	UA_ByteString_deleteMembers(&msg1);
}
END_TEST

int main() {
	int number_failed = 0;
	SRunner *sr;

	Suite *s  = suite_create("testMemoryHandling");
	TCase *tc = tcase_create("Empty Objects");
	tcase_add_loop_test(tc, newAndEmptyObjectShallBeDeleted, UA_BOOLEAN, UA_INVALIDTYPE-1);
	tcase_add_test(tc, arrayCopyShallMakeADeepCopy);
	tcase_add_loop_test(tc, encodeShallYieldDecode, UA_BOOLEAN, UA_INVALIDTYPE-1);
	suite_add_tcase(s, tc);
	tc = tcase_create("Truncated Buffers");
	tcase_add_loop_test(tc, decodeShallFailWithTruncatedBufferButSurvive, UA_BOOLEAN, UA_INVALIDTYPE-1);
	suite_add_tcase(s, tc);

	tc = tcase_create("Fuzzing with Random Buffers");
	tcase_add_loop_test(tc, decodeScalarBasicTypeFromRandomBufferShallSucceed, UA_BOOLEAN, UA_DOUBLE);
	tcase_add_loop_test(tc, decodeComplexTypeFromRandomBufferShallSurvive, UA_STRING, UA_DIAGNOSTICINFO);
	tcase_add_loop_test(tc, decodeComplexTypeFromRandomBufferShallSurvive, UA_IDTYPE, UA_INVALIDTYPE);
	suite_add_tcase(s, tc);

	sr = srunner_create(s);
	//srunner_set_fork_status(sr, CK_NOFORK);
	srunner_run_all (sr, CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
