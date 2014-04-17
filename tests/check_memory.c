/*
 * check_memory.c
 *
 *  Created on: 10.04.2014
 *      Author: mrt
 */

#include <stdio.h>
#include <stdlib.h>

#include "opcua.h"
#include "check.h"

START_TEST (encodeShallYieldDecode)
{
	void *obj1 = UA_NULL, *obj2 = UA_NULL;
	UA_ByteString msg1, msg2;
//	UA_ByteString x;
	UA_Int32 retval, pos;

//	printf("testing idx=%d,name=%s\n",_i,UA_[_i].name);
// create src object
	retval = UA_[_i].new(&obj1);
//	printf("retval=%d, ",retval); x.length = UA_[_i].calcSize(UA_NULL); x.data = (UA_Byte*) obj1; UA_ByteString_printx_hex("obj1=",&x);

// encode obj into buffer
	UA_ByteString_newMembers(&msg1,UA_[_i].calcSize(obj1));
	pos = 0;
	retval = UA_[_i].encodeBinary(obj1, &pos, &msg1);
//	printf("retval=%d, ",retval); x.length = pos; x.data = (UA_Byte*) msg1.data; UA_ByteString_printx_hex("msg1=",&x);

// create dst object
	UA_[_i].new(&obj2);
	pos = 0;
	retval = UA_[_i].decodeBinary(&msg1, &pos, obj2);
//	printf("retval=%d, ",retval); x.length = UA_[_i].calcSize(UA_NULL); x.data = (UA_Byte*) obj2; UA_ByteString_printx_hex("obj2=",&x);
	UA_ByteString_newMembers(&msg2,UA_[_i].calcSize(obj2));
	pos = 0;
	retval = UA_[_i].encodeBinary(obj2, &pos, &msg2);
//	printf("retval=%d, ",retval); x.length = pos; x.data = (UA_Byte*) msg2.data; UA_ByteString_printx_hex("msg2=",&x);

	ck_assert_msg(UA_ByteString_compare(&msg1,&msg2)==0,"messages differ idx=%d,name=%s",_i,UA_[_i].name);
	ck_assert_int_eq(retval,UA_SUCCESS);
}
END_TEST

START_TEST (decodeShallFailWithTruncatedBufferButSurvive)
{
	void *obj1 = UA_NULL, *obj2 = UA_NULL;
	UA_ByteString msg1;
	UA_Int32 retval, pos;

	retval = UA_[_i].new(&obj1);
	UA_ByteString_newMembers(&msg1,UA_[_i].calcSize(obj1));
	pos = 0;
	retval = UA_[_i].encodeBinary(obj1, &pos, &msg1);
	ck_assert_int_eq(retval,UA_SUCCESS);

	UA_[_i].new(&obj2);
	pos = 0;
	msg1.length = msg1.length / 2;
	retval = UA_[_i].decodeBinary(&msg1, &pos, obj2);

	ck_assert_msg(retval!=UA_SUCCESS,"testing %s with half buffer",UA_[_i].name);

	pos = 0;
	msg1.length = msg1.length / 4;
	retval = UA_[_i].decodeBinary(&msg1, &pos, obj2);

	ck_assert_msg(retval!=UA_SUCCESS,"testing %s with quarter buffer",UA_[_i].name);

}
END_TEST

int main() {
	int number_failed = 0;
	SRunner *sr;

	Suite *s = suite_create("testMemoryHandling");
	TCase *tc = tcase_create("Empty Objects");
	tcase_add_loop_test(tc, encodeShallYieldDecode,UA_BOOLEAN,UA_INVALIDTYPE-1);
	suite_add_tcase(s,tc);
	tc = tcase_create("Truncated Buffers");
	tcase_add_loop_test(tc, decodeShallFailWithTruncatedBufferButSurvive,UA_BOOLEAN,UA_INVALIDTYPE-1);
	suite_add_tcase(s,tc);

	sr = srunner_create(s);
	//for debugging puposes only, will break make check
	//srunner_set_fork_status(sr,CK_NOFORK);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
