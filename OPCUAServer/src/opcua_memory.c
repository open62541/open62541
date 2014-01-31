/*
 * opcua_memory.c
 *
 *  Created on: Jan 30, 2014
 *      Author: opcua
 */


void *opcua_malloc(size_t size)
{
	return malloc(size);
}


void opcua_free(void *pointer)
{
	free(pointer);
}
