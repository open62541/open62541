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
T_IntegerId decodeIntegerId(char* buf, Int32 *pos)
{
	return decodeUInt32(buf, pos);
}

/**
 * DiagnosticInfo
 * Part: 4
 * Chapter: 7.9
 * Page: 116
 */
Int32 decodeToDiagnosticInfo(char* buf, Int32 *pos, T_DiagnosticInfo* dstDiagnosticInfo)
{

	dstDiagnosticInfo->namespaceUri = decodeInt32(buf,pos);
	dstDiagnosticInfo->symbolicId = decodeInt32(buf, pos);
	dstDiagnosticInfo->locale = decodeInt32(buf, pos);
	dstDiagnosticInfo->localizesText = decodeInt32(buf, pos);

	decodeUAByteString(buf, pos, dstDiagnosticInfo->additionalInfo);
	dstDiagnosticInfo->innerStatusCode = decodeUAStatusCode(buf, pos);

	//If the Flag InnerDiagnosticInfo is set, then the DiagnosticInfo will be encoded
	if ((dstDiagnosticInfo->innerStatusCode & DIEMT_INNER_DIAGNOSTIC_INFO) == 1)
	{
		dstDiagnosticInfo->innerDiagnosticInfo = decodeTDiagnosticInfo(buf,
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

	decodeUANodeId(srcRaw->message, pos,&(dstRequestHeader->authenticationToken));
	dstRequestHeader->timestamp = decodeUADateTime(srcRaw->message, pos);
	dstRequestHeader->requestHandle = decodeIntegerId(srcRaw->message, pos);
	dstRequestHeader->returnDiagnostics = decodeUInt32(srcRaw->message, pos);
	decodeUAString(srcRaw->message, pos, &dstRequestHeader->auditEntryId);
	dstRequestHeader->timeoutHint = decodeUInt32(srcRaw->message, pos);


	// AdditionalHeader will stay empty, need to be changed if there is relevant information

	return 0;
}

/**
 * ResponseHeader
 * Part: 4
 * Chapter: 7.27
 * Page: 133
 */
/** \copydoc encodeResponseHeader */
Int32 encodeResponseHeader(const T_ResponseHeader *responseHeader, Int32 *pos, AD_RawMessage *dstBuf)
{

	return 0;
}



