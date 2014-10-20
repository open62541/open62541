/*
 * nodestoreAccessExample.h
 *
 *  Created on: Oct 16, 2014
 *      Author: opcua
 */

#ifndef NODESTOREACCESSEXAMPLE_H_
#define NODESTOREACCESSEXAMPLE_H_
#include "ua_types.h"
#include "ua_statuscodes.h"
#include "ua_namespace_0.h"
#include "ua_config.h"
#include "ua_util.h"


UA_Int32 readNodes(UA_ReadValueId * readValueIds, UA_UInt32 *readValueIdIndices, UA_UInt32 readValueIdsSize, UA_DataValue *v, UA_Boolean timeStampToReturn, UA_DiagnosticInfo *diagnosticInfos);
UA_Int32 writeNodes(UA_WriteValue *writeValues,UA_UInt32 *indices ,UA_UInt32 indicesSize, UA_StatusCode *writeNodesResults, UA_DiagnosticInfo *diagnosticInfos);
UA_Int32 initMyNode();
#endif /* NODESTOREACCESSEXAMPLE_H_ */
