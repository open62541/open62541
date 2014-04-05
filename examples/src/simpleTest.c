/*
 * main.c
 *
 *  Created on: 07.03.2014
 *      Author: mrt
 */
#include <stdio.h>
#include "opcua.h"
int main() {
	// given
	UA_Int32 pos = 0;
	UA_Byte src[] = { UA_INT32_NS0 | UA_VARIANT_ENCODINGMASKTYPE_ARRAY, 0x02, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
	UA_Variant dst;
	// when
	UA_Int32 retval = UA_Variant_decode(src, &pos, &dst);
	UA_Variant_deleteMembers(&dst);
	return 0;
}

