/*
 * nodestoreAccessExample.c
 *
 *  Created on: Oct 16, 2014
 *      Author: opcua
 */

#include "nodestoreAccessExample.h"







enum UA_AttributeId {
    UA_ATTRIBUTEID_NODEID                  = 1,
    UA_ATTRIBUTEID_NODECLASS               = 2,
    UA_ATTRIBUTEID_BROWSENAME              = 3,
    UA_ATTRIBUTEID_DISPLAYNAME             = 4,
    UA_ATTRIBUTEID_DESCRIPTION             = 5,
    UA_ATTRIBUTEID_WRITEMASK               = 6,
    UA_ATTRIBUTEID_USERWRITEMASK           = 7,
    UA_ATTRIBUTEID_ISABSTRACT              = 8,
    UA_ATTRIBUTEID_SYMMETRIC               = 9,
    UA_ATTRIBUTEID_INVERSENAME             = 10,
    UA_ATTRIBUTEID_CONTAINSNOLOOPS         = 11,
    UA_ATTRIBUTEID_EVENTNOTIFIER           = 12,
    UA_ATTRIBUTEID_VALUE                   = 13,
    UA_ATTRIBUTEID_DATATYPE                = 14,
    UA_ATTRIBUTEID_VALUERANK               = 15,
    UA_ATTRIBUTEID_ARRAYDIMENSIONS         = 16,
    UA_ATTRIBUTEID_ACCESSLEVEL             = 17,
    UA_ATTRIBUTEID_USERACCESSLEVEL         = 18,
    UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL = 19,
    UA_ATTRIBUTEID_HISTORIZING             = 20,
    UA_ATTRIBUTEID_EXECUTABLE              = 21,
    UA_ATTRIBUTEID_USEREXECUTABLE          = 22
};

static UA_VariableNode *myNode;
UA_Int32 initMyNode()
{

    UA_ExpandedNodeId ObjId_NamespaceArray; NS0EXPANDEDNODEID(ObjId_NamespaceArray, 2255);


    UA_VariableNode_new(&myNode);
    myNode->nodeId    = ObjId_NamespaceArray.nodeId;
    myNode->nodeClass = UA_NODECLASS_VARIABLE; //FIXME: this should go into _new?
    UA_QualifiedName_copycstring("myNode", &myNode->browseName);
    UA_LocalizedText_copycstring("myNode", &myNode->displayName);
    UA_LocalizedText_copycstring("myNode", &myNode->description);
    UA_Array_new((void **)&myNode->value.storage.data.dataPtr, 2, &UA_[UA_STRING]);
    myNode->value.vt = &UA_[UA_STRING];
    myNode->value.storage.data.arrayLength = 2;
    UA_String_copycstring("http://opcfoundation.org/UA/", &((UA_String *)(myNode->value.storage.data.dataPtr))[0]);
    UA_String_copycstring("http://localhost:16664/open62541/", &((UA_String *)(myNode->value.storage.data.dataPtr))[1]);
    myNode->arrayDimensionsSize = 1;
    UA_UInt32 *dimensions = UA_NULL;

    dimensions = malloc(sizeof(UA_UInt32));
    *dimensions = 2;
    myNode->arrayDimensions = dimensions;
    myNode->dataType = NS0NODEID(UA_STRING_NS0);
    myNode->valueRank       = 1;
    myNode->minimumSamplingInterval = 1.0;
    myNode->historizing     = UA_FALSE;
    return 0;

}


#define CHECK_NODECLASS(CLASS)                                 \
    if(!(myNode->nodeClass & (CLASS))) {                         \
        v[readValueIdIndices[i]].encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE; \
        v[readValueIdIndices[i]].status       = UA_STATUSCODE_BADNOTREADABLE;         \
        break;                                                 \
    }                                                          \


UA_Int32 readNodes(UA_ReadValueId * readValueIds, UA_UInt32 *readValueIdIndices, UA_UInt32 readValueIdsSize, UA_DataValue *v, UA_Boolean timeStampToReturn, UA_DiagnosticInfo *diagnosticInfo)
{
	UA_ReadValueId *id;
	UA_Int32 retval = UA_STATUSCODE_GOOD;;
	for(UA_UInt32 i = 0; i<readValueIdsSize; i++){
		id = &readValueIds[readValueIdIndices[i]];



		UA_DataValue_init(&v[readValueIdIndices[i]]);


		/*Access Node here */

		/*  */
		switch(id->attributeId) {
		case UA_ATTRIBUTEID_NODEID:
			v[readValueIdIndices[i]].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[readValueIdIndices[i]].value, &UA_[UA_NODEID], &myNode->nodeId);
			break;

		case UA_ATTRIBUTEID_NODECLASS:
			v[readValueIdIndices[i]].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[readValueIdIndices[i]].value, &UA_[UA_INT32], &myNode->nodeClass);
			break;

		case UA_ATTRIBUTEID_BROWSENAME:
			v[readValueIdIndices[i]].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[readValueIdIndices[i]].value, &UA_[UA_QUALIFIEDNAME], &myNode->browseName);
			break;

		case UA_ATTRIBUTEID_DISPLAYNAME:
			v[readValueIdIndices[i]].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[readValueIdIndices[i]].value, &UA_[UA_LOCALIZEDTEXT],
											  &myNode->displayName);
			break;

		case UA_ATTRIBUTEID_DESCRIPTION:
			v[readValueIdIndices[i]].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[readValueIdIndices[i]].value, &UA_[UA_LOCALIZEDTEXT],
											  &myNode->description);
			break;

		case UA_ATTRIBUTEID_WRITEMASK:
			v[readValueIdIndices[i]].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[readValueIdIndices[i]].value, &UA_[UA_UINT32], &myNode->writeMask);
			break;

		case UA_ATTRIBUTEID_USERWRITEMASK:
			v[readValueIdIndices[i]].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[readValueIdIndices[i]].value, &UA_[UA_UINT32], &myNode->userWriteMask);
			break;

		case UA_ATTRIBUTEID_ISABSTRACT:
			CHECK_NODECLASS(
				UA_NODECLASS_REFERENCETYPE | UA_NODECLASS_OBJECTTYPE | UA_NODECLASS_VARIABLETYPE |
				UA_NODECLASS_DATATYPE);
			v[readValueIdIndices[i]].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |=
				UA_Variant_copySetValue(&v[readValueIdIndices[i]].value, &UA_[UA_BOOLEAN],
										&((UA_ReferenceTypeNode *)myNode)->isAbstract);
			break;

		case UA_ATTRIBUTEID_SYMMETRIC:
			CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE);
			v[readValueIdIndices[i]].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[readValueIdIndices[i]].value, &UA_[UA_BOOLEAN],
											  &((UA_ReferenceTypeNode *)myNode)->symmetric);
			break;

		case UA_ATTRIBUTEID_INVERSENAME:
			CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE);
			v[readValueIdIndices[i]].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[readValueIdIndices[i]].value, &UA_[UA_LOCALIZEDTEXT],
											  &((UA_ReferenceTypeNode *)myNode)->inverseName);
			break;

		case UA_ATTRIBUTEID_CONTAINSNOLOOPS:
			CHECK_NODECLASS(UA_NODECLASS_VIEW);
			v[readValueIdIndices[i]].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[readValueIdIndices[i]].value, &UA_[UA_BOOLEAN],
											  &((UA_ViewNode *)myNode)->containsNoLoops);
			break;

		case UA_ATTRIBUTEID_EVENTNOTIFIER:
			CHECK_NODECLASS(UA_NODECLASS_VIEW | UA_NODECLASS_OBJECT);
			v[readValueIdIndices[i]].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[readValueIdIndices[i]].value, &UA_[UA_BYTE],
											  &((UA_ViewNode *)myNode)->eventNotifier);
			break;

		case UA_ATTRIBUTEID_VALUE:
			CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
			v[readValueIdIndices[i]].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copy(&((UA_VariableNode *)myNode)->value, &v[readValueIdIndices[i]].value); // todo: zero-copy
			break;

		case UA_ATTRIBUTEID_DATATYPE:
			CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
			v[readValueIdIndices[i]].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[readValueIdIndices[i]].value, &UA_[UA_NODEID],
											  &((UA_VariableTypeNode *)myNode)->dataType);
			break;

		case UA_ATTRIBUTEID_VALUERANK:
			CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
			v[readValueIdIndices[i]].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[readValueIdIndices[i]].value, &UA_[UA_INT32],
											  &((UA_VariableTypeNode *)myNode)->valueRank);
			break;

		case UA_ATTRIBUTEID_ARRAYDIMENSIONS:
			CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
			v[readValueIdIndices[i]].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			UA_Variant_copySetArray(&v[readValueIdIndices[i]].value, &UA_[UA_UINT32],
									((UA_VariableTypeNode *)myNode)->arrayDimensionsSize,
									&((UA_VariableTypeNode *)myNode)->arrayDimensions);
			break;

		case UA_ATTRIBUTEID_ACCESSLEVEL:
			CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
			v[readValueIdIndices[i]].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[readValueIdIndices[i]].value, &UA_[UA_BYTE],
											  &((UA_VariableNode *)myNode)->accessLevel);
			break;

		case UA_ATTRIBUTEID_USERACCESSLEVEL:
			CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
			v[readValueIdIndices[i]].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[readValueIdIndices[i]].value, &UA_[UA_BYTE],
											  &((UA_VariableNode *)myNode)->userAccessLevel);
			break;

		case UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
			CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
			v[readValueIdIndices[i]].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[readValueIdIndices[i]].value, &UA_[UA_DOUBLE],
											  &((UA_VariableNode *)myNode)->minimumSamplingInterval);
			break;

		case UA_ATTRIBUTEID_HISTORIZING:
			CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
			v[readValueIdIndices[i]].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[readValueIdIndices[i]].value, &UA_[UA_BOOLEAN],
											  &((UA_VariableNode *)myNode)->historizing);
			break;

		case UA_ATTRIBUTEID_EXECUTABLE:
			CHECK_NODECLASS(UA_NODECLASS_METHOD);
			v[readValueIdIndices[i]].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[readValueIdIndices[i]].value, &UA_[UA_BOOLEAN],
											  &((UA_MethodNode *)myNode)->executable);
			break;

		case UA_ATTRIBUTEID_USEREXECUTABLE:
			CHECK_NODECLASS(UA_NODECLASS_METHOD);
			v[readValueIdIndices[i]].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[readValueIdIndices[i]].value, &UA_[UA_BOOLEAN],
											  &((UA_MethodNode *)myNode)->userExecutable);
			break;

		default:
			v[readValueIdIndices[i]].encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
			v[readValueIdIndices[i]].status       = UA_STATUSCODE_BADATTRIBUTEIDINVALID;
			break;
		}

		if(retval != UA_STATUSCODE_GOOD) {
			v[readValueIdIndices[i]].encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
			v[readValueIdIndices[i]].status       = UA_STATUSCODE_BADNOTREADABLE;
		}

	}
	return retval;
}

UA_Int32 writeNodes(UA_WriteValue *writeValues,UA_UInt32 *indices ,UA_UInt32 indicesSize, UA_StatusCode *writeNodesResults, UA_DiagnosticInfo *diagnosticInfos)
{
    UA_Int32 retval = UA_STATUSCODE_GOOD;
    for(UA_UInt32 i=0;i<indicesSize;i++){
		switch(writeValues[indices[i]].attributeId) {
		case UA_ATTRIBUTEID_NODEID:
			/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){ } */
			writeNodesResults[indices[i]] = UA_STATUSCODE_BADWRITENOTSUPPORTED;
			return UA_STATUSCODE_BADINTERNALERROR;
			break;
		case UA_ATTRIBUTEID_NODECLASS:
			/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){ } */
			writeNodesResults[indices[i]] = UA_STATUSCODE_BADWRITENOTSUPPORTED;
			return UA_STATUSCODE_BADINTERNALERROR;
			break;

		case UA_ATTRIBUTEID_BROWSENAME:
			/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
			writeNodesResults[indices[i]] = UA_STATUSCODE_BADWRITENOTSUPPORTED;
			return UA_STATUSCODE_BADINTERNALERROR;
			break;

		case UA_ATTRIBUTEID_DISPLAYNAME:
			/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
			writeNodesResults[indices[i]] = UA_STATUSCODE_BADWRITENOTSUPPORTED;
			return UA_STATUSCODE_BADINTERNALERROR;
			break;

		case UA_ATTRIBUTEID_DESCRIPTION:
			/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
			writeNodesResults[indices[i]] = UA_STATUSCODE_BADWRITENOTSUPPORTED;
			return UA_STATUSCODE_BADINTERNALERROR;
			break;

		case UA_ATTRIBUTEID_WRITEMASK:
			/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
			break;

		case UA_ATTRIBUTEID_USERWRITEMASK:
			/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
			writeNodesResults[indices[i]] = UA_STATUSCODE_BADWRITENOTSUPPORTED;
			return UA_STATUSCODE_BADINTERNALERROR;
			break;

		case UA_ATTRIBUTEID_ISABSTRACT:

			/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
			writeNodesResults[indices[i]] = UA_STATUSCODE_BADWRITENOTSUPPORTED;

			break;

		case UA_ATTRIBUTEID_SYMMETRIC:
			/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
			writeNodesResults[indices[i]] = UA_STATUSCODE_BADWRITENOTSUPPORTED;
			return UA_STATUSCODE_BADINTERNALERROR;
			break;

		case UA_ATTRIBUTEID_INVERSENAME:
			/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
			writeNodesResults[indices[i]] = UA_STATUSCODE_BADWRITENOTSUPPORTED;
			break;

		case UA_ATTRIBUTEID_CONTAINSNOLOOPS:
			/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
			writeNodesResults[indices[i]] = UA_STATUSCODE_BADWRITENOTSUPPORTED;
			return UA_STATUSCODE_BADINTERNALERROR;
			break;

		case UA_ATTRIBUTEID_EVENTNOTIFIER:
			/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
			writeNodesResults[indices[i]] = UA_STATUSCODE_BADWRITENOTSUPPORTED;
			break;

		case UA_ATTRIBUTEID_VALUE:
			if(writeValues[indices[i]].value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT) {
				retval |= UA_Variant_copy(&writeValues[indices[i]].value.value, &((UA_VariableNode *)myNode)->value); // todo: zero-copy
				writeNodesResults[indices[i]] = UA_STATUSCODE_GOOD;
			}

			break;

		case UA_ATTRIBUTEID_DATATYPE:
			/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
			writeNodesResults[indices[i]]   = UA_STATUSCODE_BADWRITENOTSUPPORTED;
			break;

		case UA_ATTRIBUTEID_VALUERANK:
			/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
			writeNodesResults[indices[i]]   = UA_STATUSCODE_BADWRITENOTSUPPORTED;
			break;

		case UA_ATTRIBUTEID_ARRAYDIMENSIONS:
			/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
			writeNodesResults[indices[i]]   = UA_STATUSCODE_BADWRITENOTSUPPORTED;
			break;

		case UA_ATTRIBUTEID_ACCESSLEVEL:
			/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
			writeNodesResults[indices[i]]   = UA_STATUSCODE_BADWRITENOTSUPPORTED;
			break;

		case UA_ATTRIBUTEID_USERACCESSLEVEL:
			/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
			writeNodesResults[indices[i]]   = UA_STATUSCODE_BADWRITENOTSUPPORTED;
			return UA_STATUSCODE_BADINTERNALERROR;
			break;

		case UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
			/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
			writeNodesResults[indices[i]]   = UA_STATUSCODE_BADWRITENOTSUPPORTED;
			break;

		case UA_ATTRIBUTEID_HISTORIZING:
			/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
			writeNodesResults[indices[i]]   = UA_STATUSCODE_BADWRITENOTSUPPORTED;
			break;

		case UA_ATTRIBUTEID_EXECUTABLE:
			/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
			writeNodesResults[indices[i]]   = UA_STATUSCODE_BADWRITENOTSUPPORTED;
			break;

		case UA_ATTRIBUTEID_USEREXECUTABLE:
			/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
			writeNodesResults[indices[i]]   = UA_STATUSCODE_BADWRITENOTSUPPORTED;
			break;

		default:
			writeNodesResults[indices[i]]   = UA_STATUSCODE_BADATTRIBUTEIDINVALID;
			break;
		}
    }

    return retval;
}
