/*
 * opcua_memory.h
 *
 *  Created on: Jan 30, 2014
 *      Author: opcua
 */

#ifndef OPCUA_MEMORY_H_
#define OPCUA_MEMORY_H_

/**
 *
 * @param size
 */
void* opcua_malloc(size_t size);

/**
 *
 * @param pointer
 */
void opcua_free(void* pointer);


#endif /* OPCUA_MEMORY_H_ */
