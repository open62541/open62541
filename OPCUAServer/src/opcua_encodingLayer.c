/*
 * opcua_encodingLayer.c
 *
 *  Created on: Jan 14, 2014
 *      Author: opcua
 */
#include "opcua_encodingLayer.h"
#include "opcua_binaryEncDec.h"
#include "opcua_types.h"

T_RequestHeader encodeRequestHeader(char* buf){
	T_RequestHeader tmpRequestHeader;
	int counter = 0 ;
	//ToDo: counter needs the length of the buffer,
	//		strings have in this type just the size of the pointer not of the content
	tmpRequestHeader.authenticationToken = convertToUANodeId(*buf, counter);
	if(tmpRequestHeader.authenticationToken.EncodingByte ==  NIEVT_STRING){
		counter = sizeof(tmpRequestHeader.authenticationToken.EncodingByte) +
				sizeof(tmpRequestHeader.authenticationToken.Namespace) +
				sizeof(tmpRequestHeader.authenticationToken.Identifier.String.Length) +
				sizeof(tmpRequestHeader.authenticationToken.Identifier.String.Data);
	}else{
		counter = sizeof(tmpRequestHeader.authenticationToken);
	}
	tmpRequestHeader.timestamp = convertToUADateTime(*buf, counter);
	counter += sizeof(tmpRequestHeader.timestamp);
	tmpRequestHeader.requestHandle = convertToInt64(*buf, counter);
	counter += sizeof(tmpRequestHeader.requestHandle);
	tmpRequestHeader.returnDiagnostics = convertToUInt32(*buf, counter);
	counter += sizeof(tmpRequestHeader.returnDiagnostics);
	tmpRequestHeader.auditEntryId = convertToUAString(*buf, counter);
	counter += sizeof(tmpRequestHeader.auditEntryId);
	tmpRequestHeader.timeoutHint = convertToUInt32(*buf, counter);
	counter += sizeof(tmpRequestHeader.timeoutHint);
	// AdditionalHeader will stay empty, need to be changed if there is relevant information

	return tmpRequestHeader;
}
