/*
 * opcua_BuiltInDatatypes.c
 *
 *  Created on: Dec 19, 2013
 *      Author: opcua
 */

#include "opcua_builtInDatatypes.h"
#include <stdio.h>

Int32 UA_String_compare(UA_String *string1, UA_String *string2) {
	Int32 i;
	Boolean equal;

	if (string1->Length == string2->Length&&
	string1->Length > 0 &&
	string1->Data != NULL && string2->Data != NULL) {for(i = 0; i < string1->Length; i++)
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

Int32 UA_ByteString_compare(UA_ByteString *string1, UA_ByteString *string2) {
	return UA_String_compare((UA_String*) string1, (UA_String*) string2);
}

void UA_String_printf(char* label, UA_ByteString* string) {
	printf("%s {Length=%d, Data=%.*s}\n", label, string->Length, string->Length,
			(char*) string->Data);
}

void UA_ByteString_printx(char* label, UA_ByteString* string) {
	int i;
	printf("%s {Length=%d, Data=", label, string->Length);
	if (string->Length > 0) {
		for (i = 0; i < string->Length; i++) {
			printf("%c%d", i == 0 ? '{' : ',', (string->Data)[i]);
		}
	} else {
		printf("{");
	}
	printf("}}\n");
}
void UA_ByteString_printx_hex(char* label, UA_ByteString* string) {
	int i;
	printf("%s {Length=%d, Data=", label, string->Length);
	if (string->Length > 0) {
		for (i = 0; i < string->Length; i++) {
			printf("%c%x", i == 0 ? '{' : ',', (string->Data)[i]);
		}
	} else {
		printf("{");
	}
	printf("}}\n");
}

void UA_NodeId_printf(char* label, UA_NodeId* node) {
	printf("%s {EncodingByte=%d, Namespace=%d, ", label,
			(int) node->EncodingByte, (int) node->Namespace);
	switch (node->EncodingByte) {
	case NIEVT_TWO_BYTE:
	case NIEVT_FOUR_BYTE:
	case NIEVT_NUMERIC:
		printf("Identifier=%d", node->Identifier.Numeric);
		break;
	case NIEVT_STRING:
	case NIEVT_BYTESTRING:
		// TODO: This implementation does not distinguish between String and Bytestring. Error?
		printf("Identifier={Length=%d, Data=%.*s}",
				node->Identifier.String.Length, node->Identifier.String.Length,
				(char*) (node->Identifier.String.Data));
		break;
	case NIEVT_GUID:
		printf(
				"Guid={Data1=%d, Data2=%d, Data3=%d, Data4=={Length=%d, Data=%.*s}}",
				node->Identifier.Guid.Data1, node->Identifier.Guid.Data2,
				node->Identifier.Guid.Data3, node->Identifier.Guid.Data4.Length,
				node->Identifier.Guid.Data4.Length,
				(char*) (node->Identifier.Guid.Data4.Data));
		break;
	default:
		printf("ups! shit happens");
		break;
	}
	printf("}\n");
}

