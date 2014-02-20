/*
 * opcua_BuiltInDatatypes.c
 *
 *  Created on: Dec 19, 2013
 *      Author: opcua
 */


#include "opcua_builtInDatatypes.h"


Int32 UA_String_compare(UA_String *string1,UA_String *string2)
{
	Int32 i;
	Boolean equal;

	if(string1->Length == string2->Length &&
	   string1->Length > 0 &&
	   string1->Data != NULL && string2->Data != NULL)
	{
		for(i = 0; i < string1->Length; i++)
		{
			if(string1->Data[i] != string2->Data[i])
			{
				return UA_NOT_EQUAL;
			}

		}
	}
	else
	{
		return UA_NOT_EQUAL;
	}
	return UA_EQUAL;
}

Int32 UA_ByteString_compare(UA_ByteString *string1,UA_ByteString *string2)
{
	return UA_String_compare((UA_String*)string1,(UA_String*)string2);
}
