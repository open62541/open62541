#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>

#include "ua_types.h"
#include "util/ua_util.h"
#include "ua_namespace_0.h"
#include "check.h"

START_TEST(newAndEmptyObjectShallBeDeleted) {
	// given
	UA_Int32 retval;
	void    *obj;
	// when
	retval  = UA_.types[_i].new(&obj);
#ifdef DEBUG //no print functions if not in debug mode
	UA_.types[_i].print(obj, stdout);
#endif
	retval |= UA_.types[_i].delete(obj);
	// then
	ck_assert_int_eq(retval, UA_SUCCESS);
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
	UA_Int32   retval = UA_Array_copy((const void *)a1, 3, &UA_.types[UA_STRING], (void **)&a2);
	// then
	ck_assert_int_eq(retval, UA_SUCCESS);
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
	UA_Array_delete((void *)a2, 3, &UA_.types[UA_STRING]);
}
END_TEST

START_TEST(encodeShallYieldDecode) {
	// given
	void         *obj1 = UA_NULL, *obj2 = UA_NULL;
	UA_ByteString msg1, msg2;
	UA_Int32      retval;
	UA_UInt32     pos = 0;
	retval = UA_.types[_i].new(&obj1);
	UA_ByteString_newMembers(&msg1, UA_.types[_i].encodings[UA_ENCODING_BINARY].calcSize(obj1));
	retval |= UA_.types[_i].encodings[UA_ENCODING_BINARY].encode(obj1, &msg1, &pos);
	if(retval != UA_SUCCESS) {
		// this happens, e.g. when we encode a variant (with UA_.types[UA_INVALIDTYPE] in the vtable)
		UA_.types[_i].delete(obj1);
		UA_ByteString_deleteMembers(&msg1);
		return;	
	}

	// when
	UA_.types[_i].new(&obj2);
	pos = 0; retval = UA_.types[_i].encodings[UA_ENCODING_BINARY].decode(&msg1, &pos, obj2);
	ck_assert_msg(retval == UA_SUCCESS, "messages differ idx=%d,name=%s", _i, UA_.types[_i].name);
	retval = UA_ByteString_newMembers(&msg2, UA_.types[_i].encodings[UA_ENCODING_BINARY].calcSize(obj2));
	ck_assert_int_eq(retval, UA_SUCCESS);
	pos = 0; retval = UA_.types[_i].encodings[UA_ENCODING_BINARY].encode(obj2, &msg2, &pos);
	ck_assert_int_eq(retval, UA_SUCCESS);

	// then
	ck_assert_msg(UA_ByteString_equal(&msg1, &msg2) == 0, "messages differ idx=%d,name=%s", _i, UA_.types[_i].name);

	// finally
	UA_.types[_i].delete(obj1);
	UA_.types[_i].delete(obj2);
	UA_ByteString_deleteMembers(&msg1);
	UA_ByteString_deleteMembers(&msg2);
}
END_TEST

START_TEST(decodeShallFailWithTruncatedBufferButSurvive) {
	// given
	void *obj1 = UA_NULL, *obj2 = UA_NULL;
	UA_ByteString msg1;
	UA_UInt32 pos;
	UA_.types[_i].new(&obj1);
	UA_ByteString_newMembers(&msg1, UA_.types[_i].encodings[0].calcSize(obj1));
	pos = 0; UA_.types[_i].encodings[0].encode(obj1, &msg1, &pos);
	UA_.types[_i].delete(obj1);
	// when
	UA_.types[_i].new(&obj2);
	pos = 0;
	msg1.length = msg1.length / 2;
	//fprintf(stderr,"testing %s with half buffer\n",UA_[_i].name);
	UA_.types[_i].encodings[0].decode(&msg1, &pos, obj2);
	//then
	// finally
	//fprintf(stderr,"delete %s with half buffer\n",UA_[_i].name);
	UA_.types[_i].delete(obj2);
	UA_ByteString_deleteMembers(&msg1);
}
END_TEST

START_TEST(decodeScalarBasicTypeFromRandomBufferShallSucceed) {
	// given
	void *obj1 = UA_NULL;
	UA_ByteString msg1;
	UA_Int32 retval = UA_SUCCESS;
	UA_Int32 buflen = 256;
	UA_ByteString_newMembers(&msg1, buflen); // fixed size
#ifdef WIN32
	srand(42);
#else
	srandom(42);
#endif
	for(int n = 0;n < 100;n++) {
		for(UA_Int32 i = 0;i < buflen;i++) {
#ifdef WIN32
			UA_UInt32 rnd;
			rnd = rand();
			msg1.data[i] = rnd;
#else
			msg1.data[i] = (UA_Byte)random();  // when
#endif
		}
		UA_UInt32 pos = 0;
		retval |= UA_.types[_i].new(&obj1);
		retval |= UA_.types[_i].encodings[0].decode(&msg1, &pos, obj1);
		//then
		ck_assert_msg(retval == UA_SUCCESS, "Decoding %s from random buffer", UA_.types[_i].name);
		// finally
		UA_.types[_i].delete(obj1);
	}
	UA_ByteString_deleteMembers(&msg1);
}
END_TEST

START_TEST(decodeComplexTypeFromRandomBufferShallSurvive) {
	// given
	void    *obj1 = UA_NULL;
	UA_ByteString msg1;
	UA_Int32 retval = UA_SUCCESS;
	UA_Int32 buflen = 256;
	UA_ByteString_newMembers(&msg1, buflen); // fixed size
#ifdef WIN32
	srand(42);
#else
	srandom(42);
#endif
	// when
	for(int n = 0;n < 100;n++) {
		for(UA_Int32 i = 0;i < buflen;i++){
#ifdef WIN32
			UA_UInt32 rnd;
			rnd = rand();
			msg1.data[i] = rnd;
#else
			msg1.data[i] = (UA_Byte)random();  // when
#endif
		}
		UA_UInt32 pos = 0;
		retval |= UA_.types[_i].new(&obj1);
		retval |= UA_.types[_i].encodings[0].decode(&msg1, &pos, obj1);

		//this is allowed to fail and return UA_ERROR
		//ck_assert_msg(retval == UA_SUCCESS, "Decoding %s from random buffer", UA_.types[_i].name);
		
		UA_.types[_i].delete(obj1);
	}

	// finally
	UA_ByteString_deleteMembers(&msg1);
}
END_TEST

int main() {
	int number_failed = 0;
	SRunner *sr;

	Suite   *s  = suite_create("testMemoryHandling");
	TCase   *tc = tcase_create("Empty Objects");
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
	//for debugging puposes only, will break make check
	//srunner_set_fork_status(sr, CK_NOFORK);
	srunner_run_all (sr, CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
