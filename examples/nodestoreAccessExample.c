/*
 * nodestoreAccessExample.c
 *
 *  Created on: Oct 16, 2014
 *      Author: opcua
 */





#include "ua_statuscodes.h"


#include "ua_namespace_0.h"
#include "ua_util.h"
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

#define CHECK_NODECLASS(CLASS)                                 \
    if(!(node->nodeClass & (CLASS))) {                         \
        v[i].encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE; \
        v[i].status       = UA_STATUSCODE_BADNOTREADABLE;         \
        break;                                                 \
    }                                                          \


UA_Int32 readNodes(UA_ReadValueId * readValueIds, UA_UInt32 readValueIdsSize, UA_DataValue *v, UA_Boolean timeStampToReturn, UA_DiagnosticInfo *diagnosticInfo)
{
	UA_ReadValueId *id;
	UA_Int32 retval = UA_SUCCESS;
	for(UA_UInt32 i = 0; i<readValueIdsSize; i++){
		id = &readValueIds[i];



		UA_DataValue_init(&v[i]);

		UA_Node const *node   = UA_NULL;

		/*Access Node here */

		/*  */
		switch(id->attributeId) {
		case UA_ATTRIBUTEID_NODEID:
			v[i].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[i].value, &UA_[UA_NODEID], &node->nodeId);
			break;

		case UA_ATTRIBUTEID_NODECLASS:
			v[i].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[i].value, &UA_[UA_INT32], &node->nodeClass);
			break;

		case UA_ATTRIBUTEID_BROWSENAME:
			v[i].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[i].value, &UA_[UA_QUALIFIEDNAME], &node->browseName);
			break;

		case UA_ATTRIBUTEID_DISPLAYNAME:
			v[i].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[i].value, &UA_[UA_LOCALIZEDTEXT],
											  &node->displayName);
			break;

		case UA_ATTRIBUTEID_DESCRIPTION:
			v[i].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[i].value, &UA_[UA_LOCALIZEDTEXT],
											  &node->description);
			break;

		case UA_ATTRIBUTEID_WRITEMASK:
			v[i].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[i].value, &UA_[UA_UINT32], &node->writeMask);
			break;

		case UA_ATTRIBUTEID_USERWRITEMASK:
			v[i].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[i].value, &UA_[UA_UINT32], &node->userWriteMask);
			break;

		case UA_ATTRIBUTEID_ISABSTRACT:
			CHECK_NODECLASS(
				UA_NODECLASS_REFERENCETYPE | UA_NODECLASS_OBJECTTYPE | UA_NODECLASS_VARIABLETYPE |
				UA_NODECLASS_DATATYPE);
			v[i].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |=
				UA_Variant_copySetValue(&v[i].value, &UA_[UA_BOOLEAN],
										&((UA_ReferenceTypeNode *)node)->isAbstract);
			break;

		case UA_ATTRIBUTEID_SYMMETRIC:
			CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE);
			v[i].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[i].value, &UA_[UA_BOOLEAN],
											  &((UA_ReferenceTypeNode *)node)->symmetric);
			break;

		case UA_ATTRIBUTEID_INVERSENAME:
			CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE);
			v[i].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[i].value, &UA_[UA_LOCALIZEDTEXT],
											  &((UA_ReferenceTypeNode *)node)->inverseName);
			break;

		case UA_ATTRIBUTEID_CONTAINSNOLOOPS:
			CHECK_NODECLASS(UA_NODECLASS_VIEW);
			v[i].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[i].value, &UA_[UA_BOOLEAN],
											  &((UA_ViewNode *)node)->containsNoLoops);
			break;

		case UA_ATTRIBUTEID_EVENTNOTIFIER:
			CHECK_NODECLASS(UA_NODECLASS_VIEW | UA_NODECLASS_OBJECT);
			v[i].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[i].value, &UA_[UA_BYTE],
											  &((UA_ViewNode *)node)->eventNotifier);
			break;

		case UA_ATTRIBUTEID_VALUE:
			CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
			v[i].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copy(&((UA_VariableNode *)node)->value, &v[i].value); // todo: zero-copy
			break;

		case UA_ATTRIBUTEID_DATATYPE:
			CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
			v[i].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[i].value, &UA_[UA_NODEID],
											  &((UA_VariableTypeNode *)node)->dataType);
			break;

		case UA_ATTRIBUTEID_VALUERANK:
			CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
			v[i].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[i].value, &UA_[UA_INT32],
											  &((UA_VariableTypeNode *)node)->valueRank);
			break;

		case UA_ATTRIBUTEID_ARRAYDIMENSIONS:
			CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
			v[i].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			UA_Variant_copySetArray(&v[i].value, &UA_[UA_UINT32],
									((UA_VariableTypeNode *)node)->arrayDimensionsSize,
									&((UA_VariableTypeNode *)node)->arrayDimensions);
			break;

		case UA_ATTRIBUTEID_ACCESSLEVEL:
			CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
			v[i].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[i].value, &UA_[UA_BYTE],
											  &((UA_VariableNode *)node)->accessLevel);
			break;

		case UA_ATTRIBUTEID_USERACCESSLEVEL:
			CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
			v[i].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[i].value, &UA_[UA_BYTE],
											  &((UA_VariableNode *)node)->userAccessLevel);
			break;

		case UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
			CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
			v[i].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[i].value, &UA_[UA_DOUBLE],
											  &((UA_VariableNode *)node)->minimumSamplingInterval);
			break;

		case UA_ATTRIBUTEID_HISTORIZING:
			CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
			v[i].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[i].value, &UA_[UA_BOOLEAN],
											  &((UA_VariableNode *)node)->historizing);
			break;

		case UA_ATTRIBUTEID_EXECUTABLE:
			CHECK_NODECLASS(UA_NODECLASS_METHOD);
			v[i].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[i].value, &UA_[UA_BOOLEAN],
											  &((UA_MethodNode *)node)->executable);
			break;

		case UA_ATTRIBUTEID_USEREXECUTABLE:
			CHECK_NODECLASS(UA_NODECLASS_METHOD);
			v[i].encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
			retval |= UA_Variant_copySetValue(&v[i].value, &UA_[UA_BOOLEAN],
											  &((UA_MethodNode *)node)->userExecutable);
			break;

		default:
			v[i].encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
			v[i].status       = UA_STATUSCODE_BADATTRIBUTEIDINVALID;
			break;
		}

		if(retval != UA_SUCCESS) {
			v[i].encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
			v[i].status       = UA_STATUSCODE_BADNOTREADABLE;
		}

	}
	return retval;
}
