/*
 * OPCUA_builtInDatatypes.h
 *
 *  Created on: Dec 19, 2013
 *      Author: opcua
 */
#include <stdint.h>
#include <string.h>
#ifndef OPCUA_BUILTINDATATYPES_H_
#define OPCUA_BUILTINDATATYPES_H_



#endif /* OPCUA_BUILTINDATATYPES_H_ */

typedef _Bool Boolean;

typedef int8_t SByte;

typedef uint8_t Byte;

typedef int16_t Int16;

typedef uint16_t UInt16;

typedef int32_t Int32;

typedef uint32_t UInt32;

typedef int64_t Int64;

typedef uint64_t UInt64;

typedef float Float;

typedef double Double;


struct UA_StringType
{
	int Length;
	char *Data;
};

typedef struct UA_StringType UA_String;




