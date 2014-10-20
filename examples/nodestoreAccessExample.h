/*
 * nodestoreAccessExample.h
 *
 *  Created on: Oct 16, 2014
 *      Author: opcua
 */

#ifndef NODESTOREACCESSEXAMPLE_H_
#define NODESTOREACCESSEXAMPLE_H_


UA_Int32 readNodes(UA_ReadValueId * readValueIds, UA_UInt32 *readValueIdIndices, UA_UInt32 readValueIdsSize, UA_DataValue *v, UA_Boolean timeStampToReturn, UA_DiagnosticInfo *diagnosticInfo);

#endif /* NODESTOREACCESSEXAMPLE_H_ */
