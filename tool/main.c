/*
 * main.c
 *
 *  Created on: 07.03.2014
 *      Author: mrt
 */
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "opcua.h"

int main() {
	char* buf;
	int pos = 0, retval, size;

	// the value to encode
	UA_Int32 i = -42, j;

	// get buffer for encoding
	size = UA_Int32_calcSize(UA_NULL);
	buf = (char *) malloc(size);
	printf("buf=%p, size=%d\n", buf, size);
	if (buf == UA_NULL) return -1;

	// encode
	pos = 0;
	retval = UA_Int32_encode(&i, &pos, buf);
	printf("retval=%d, src=%d, pos=%d, buf={%d,%d,%d,%d}\n", retval, i, pos, buf[0], buf[1], buf[2], buf[3]);

	// decode
	pos = 0;
	retval = UA_Int32_decode(buf, &pos, &j);
	printf("retval=%d, dst=%d, pos=%d, {%d,%d,%d,%d}\n", retval, j, pos, buf[0], buf[1], buf[2], buf[3]);

	// return memory
	free(buf);
	return 0;
}

