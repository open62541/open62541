/*
 * opcua_encodingLayer.c
 *
 *  Created on: Jan 14, 2014
 *      Author: opcua
 */
#include "opcua_encodingLayer.h"
#include "opcua_binaryEncDec.h"
#include "opcua_types.h"
#include "opcua_builtInDatatypes.h"

/**
 * IntegerId
 * Part: 4
 * Chapter: 7.13
 * Page: 118
 */
T_IntegerId convertToIntegerId(char* buf, Int32 *pos)
{
	return convertToUInt32(buf, pos);
}

/**
 * DiagnosticInfo
 * Part: 4
 * Chapter: 7.9
 * Page: 116
 */
Int32 convertToDiagnosticInfo(char* buf, Int32 *pos, T_DiagnosticInfo* dstDiagnosticInfo)
{

	dstDiagnosticInfo->namespaceUri = convertToInt32(buf,pos);
	dstDiagnosticInfo->symbolicId = convertToInt32(buf, pos);
	dstDiagnosticInfo->locale = convertToInt32(buf, pos);
	dstDiagnosticInfo->localizesText = convertToInt32(buf, pos);
	dstDiagnosticInfo->additionalInfo = convertToUAString(buf, pos);
	dstDiagnosticInfo->innerStatusCode = convertToUAStatusCode(buf, pos);

	//If the Flag InnerDiagnosticInfo is set, then the DiagnosticInfo will be encoded
	if ((dstDiagnosticInfo->innerStatusCode & DIEMT_INNER_DIAGNOSTIC_INFO) == 1)
	{
		dstDiagnosticInfo->innerDiagnosticInfo = convertToTDiagnosticInfo(buf,
				pos);
	}

	return 0;
}

/**
 * RequestHeader
 * Part: 4
 * Chapter: 7.26
 * Page: 132
 */

/** \copydoc decodeRequestHeader */
Int32 decodeRequestHeader(const AD_RawMessage *srcRaw, Int32 *pos,
		T_RequestHeader *dstRequestHeader)
{

	convertToUANodeId(srcRaw->message, pos,&(dstRequestHeader->authenticationToken));
	dstRequestHeader->timestamp = convertToUADateTime(srcRaw->message, pos);
	dstRequestHeader->requestHandle = convertToIntegerId(srcRaw->message, pos);
	dstRequestHeader->returnDiagnostics = convertToUInt32(srcRaw->message, pos);
	convertToUAString(srcRaw->message, pos, &dstRequestHeader->auditEntryId);
	dstRequestHeader->timeoutHint = convertToUInt32(srcRaw->message, pos);

	// AdditionalHeader will stay empty, need to be changed if there is relevant information

	return 0;
}

/**
 * ResponseHeader
 * Part: 4
 * Chapter: 7.27
 * Page: 133
 */
/** \copydoc decodeResponseHeader */
Int32 decodeResponseHeader(const T_ResponseHeader *responseHeader, Int32 *pos, AD_RawMessage *buf)
{
	responseHeader->timestamp = convertToUADateTime(buf->message, pos);
	responseHeader->requestHandle = convertToTIntegerId(buf->message, pos);
	responseHeader->serviceResult = convertToUAStatusCode(buf->message,
			pos);

	responseHeader->serviceDiagnostics = convertToTDiagnosticInfo(buf->message,
			pos);
	return 0;
}



