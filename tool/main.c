/*
 * main.c
 *
 *  Created on: 07.03.2014
 *      Author: mrt
 */
#include <stdio.h>

#include "opcua.h"

#define UA_NO_ERROR ((Int32) 0)

Int32 decodeInt32(char const * buf, Int32* pos, Int32* dst) {
	Int32 t1 = (Int32) (((SByte) (buf[*pos]) & 0xFF));
	Int32 t2 = (Int32) (((SByte) (buf[*pos + 1]) & 0xFF) << 8);
	Int32 t3 = (Int32) (((SByte) (buf[*pos + 2]) & 0xFF) << 16);
	Int32 t4 = (Int32) (((SByte) (buf[*pos + 3]) & 0xFF) << 24);
	*pos += sizeof(Int32);
	*dst = t1 + t2 + t3 + t4;
	return UA_NO_ERROR;
}

typedef union Integer {
	Int32 i;
	SByte b[4];
} Integer;

int main() {
	Integer a = { UA_IdType_Guid };
	Integer b;
	int pos = 0;

	a.i = 0;
	a.b[3] = 1;

	printf("%d, {%d,%d,%d,%d}\n", a.i, a.b[0], a.b[1], a.b[2], a.b[3]);

	decodeInt32((char *) &a.b[0], &pos, &(b.i));
	printf("%d, {%d,%d,%d,%d}\n", b.i, b.b[0], b.b[1], b.b[2], b.b[3]);

	return 0;
}

