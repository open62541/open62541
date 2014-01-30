/*
 * opcua_memory.c
 *
 *  Created on: Jan 30, 2014
 *      Author: opcua
 */


void *opcua_malloc(size_t size)
{
	malloc(size);
}


void free(void *pointer)
{
	free(pointer);
}
