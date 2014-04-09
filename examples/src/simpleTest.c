/*
 * main.c
 *
 *  Created on: 07.03.2014
 *      Author: mrt
 */
#include <stdio.h>
#include "opcua.h"
void testString() {
	UA_Int32 pos = 0;
	UA_Int32 retval = UA_SUCCESS;
	UA_String string;

	UA_Byte binString[12] = {0x08,0x00,0x00,0x00,'A','C','P','L','T',' ','U','A'};
	UA_ByteString src = { 12, binString };

	retval = UA_String_decodeBinary(&src, &pos, &string);
	UA_String_deleteMembers(&string);

}
void testVariant() {
	// given
	UA_Int32 pos = 0;
	UA_Byte data[] = { UA_INT32_NS0 | UA_VARIANT_ENCODINGMASKTYPE_ARRAY, 0x02, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
	UA_ByteString src = { sizeof(data), data };
	UA_Variant dst;
	// when
	UA_Int32 retval = UA_Variant_decodeBinary(&src, &pos, &dst);
	UA_Variant_deleteMembers(&dst);
}
void testByte() {
	// given
	UA_Byte dst;
	UA_Byte data[] = { 0x08 };
	UA_ByteString src = { 1, data };
	UA_Int32 pos = 0;
	// when
	UA_Int32 retval = UA_Byte_decodeBinary(&src, &pos, &dst);
}
int main() {
	testByte();
	return 0;
}
