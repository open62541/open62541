/*
 * main.c
 *
 *  Created on: 07.03.2014
 *      Author: mrt
 */
#include <stdio.h>

#include "opcua.h"

typedef union Integer {
	Int32 i;
	SByte b[4];
} Integer;

int main() {
	Integer a = { 0x11 };
	Integer b;
	int pos = 0;

	a.i = 0;
	a.b[3] = 1;

	printf("%d, {%d,%d,%d,%d}\n", a.i, a.b[0], a.b[1], a.b[2], a.b[3]);

	UA_Int32_decode((char *) &a.b[0], &pos, &(b.i));
	printf("%d, {%d,%d,%d,%d}\n", b.i, b.b[0], b.b[1], b.b[2], b.b[3]);

	return 0;
}

