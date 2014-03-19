/*
 * main.c
 *
 *  Created on: 07.03.2014
 *      Author: mrt
 */
#include <stdio.h>

#include "opcua.h"

typedef union Integer {
	UA_Int32 i;
	SByte b[4];
} Integer;

int main() {
	Integer a;
	Integer b;
	int pos = 0;

	UA_Int32 i = -42;

	UA_Int32_encode(&i, &pos, &a.b[0]);
	printf("%d, {%d,%d,%d,%d}\n", a.i, a.b[0], a.b[1], a.b[2], a.b[3]);

	pos = 0;
	UA_Int32_decode((char *) &a.b[0], &pos, &(b.i));
	printf("%d, {%d,%d,%d,%d}\n", b.i, b.b[0], b.b[1], b.b[2], b.b[3]);

	printf("%i\n", UA_Int32_calcSize(b.i));

	return 0;
}

