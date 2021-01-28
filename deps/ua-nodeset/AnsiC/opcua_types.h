/* ========================================================================
 * Copyright (c) 2005-2019 The OPC Foundation, Inc. All rights reserved.
 *
 * OPC Foundation MIT License 1.00
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * The complete license agreement can be found here:
 * http://opcfoundation.org/License/MIT/1.00/
 * ======================================================================*/

#ifndef _OpcUa_Types_H_
#define _OpcUa_Types_H_ 1

#include <opcua_builtintypes.h>

OPCUA_BEGIN_EXTERN_C

struct _OpcUa_Encoder;
struct _OpcUa_Decoder;
struct _OpcUa_EncodeableType;
struct _OpcUa_EnumeratedType;

#ifndef OPCUA_EXCLUDE_IdType
/*============================================================================
 * The IdType enumeration.
 *===========================================================================*/
typedef enum _OpcUa_IdType
{
    OpcUa_IdType_Numeric = 0,
    OpcUa_IdType_String  = 1,
    OpcUa_IdType_Guid    = 2,
    OpcUa_IdType_Opaque  = 3
#if OPCUA_FORCE_INT32_ENUMS
    ,_OpcUa_IdType_MaxEnumerationValue = OpcUa_Int32_Max
#endif
}
OpcUa_IdType;

#define OpcUa_IdType_Clear(xValue) OpcUa_EnumeratedType_Clear(xValue, OpcUa_IdType_Numeric)

#define OpcUa_IdType_Initialize(xValue) OpcUa_EnumeratedType_Initialize(xValue, OpcUa_IdType_Numeric)

OPCUA_IMEXPORT extern struct _OpcUa_EnumeratedType OpcUa_IdType_EnumeratedType;
#endif

#ifndef OPCUA_EXCLUDE_NodeClass
/*============================================================================
 * The NodeClass enumeration.
 *===========================================================================*/
typedef enum _OpcUa_NodeClass
{
    OpcUa_NodeClass_Unspecified   = 0,
    OpcUa_NodeClass_Object        = 1,
    OpcUa_NodeClass_Variable      = 2,
    OpcUa_NodeClass_Method        = 4,
    OpcUa_NodeClass_ObjectType    = 8,
    OpcUa_NodeClass_VariableType  = 16,
    OpcUa_NodeClass_ReferenceType = 32,
    OpcUa_NodeClass_DataType      = 64,
    OpcUa_NodeClass_View          = 128
#if OPCUA_FORCE_INT32_ENUMS
    ,_OpcUa_NodeClass_MaxEnumerationValue = OpcUa_Int32_Max
#endif
}
OpcUa_NodeClass;

#define OpcUa_NodeClass_Clear(xValue) OpcUa_EnumeratedType_Clear(xValue, OpcUa_NodeClass_Unspecified)

#define OpcUa_NodeClass_Initialize(xValue) OpcUa_EnumeratedType_Initialize(xValue, OpcUa_NodeClass_Unspecified)

OPCUA_IMEXPORT extern struct _OpcUa_EnumeratedType OpcUa_NodeClass_EnumeratedType;
#endif

#ifndef OPCUA_EXCLUDE_PermissionType
/*============================================================================
 * The PermissionType enumeration.
 *===========================================================================*/
typedef enum _OpcUa_PermissionType
{
    OpcUa_PermissionType_None                 = 0,
    OpcUa_PermissionType_Browse               = 1,
    OpcUa_PermissionType_ReadRolePermissions  = 2,
    OpcUa_PermissionType_WriteAttribute       = 4,
    OpcUa_PermissionType_WriteRolePermissions = 8,
    OpcUa_PermissionType_WriteHistorizing     = 16,
    OpcUa_PermissionType_Read                 = 32,
    OpcUa_PermissionType_Write                = 64,
    OpcUa_PermissionType_ReadHistory          = 128,
    OpcUa_PermissionType_InsertHistory        = 256,
    OpcUa_PermissionType_ModifyHistory        = 512,
    OpcUa_PermissionType_DeleteHistory        = 1024,
    OpcUa_PermissionType_ReceiveEvents        = 2048,
    OpcUa_PermissionType_Call                 = 4096,
    OpcUa_PermissionType_AddReference         = 8192,
    OpcUa_PermissionType_RemoveReference      = 16384,
    OpcUa_PermissionType_DeleteNode           = 32768,
    OpcUa_PermissionType_AddNode              = 65536
#if OPCUA_FORCE_INT32_ENUMS
    ,_OpcUa_PermissionType_MaxEnumerationValue = OpcUa_Int32_Max
#endif
}
OpcUa_PermissionType;

#define OpcUa_PermissionType_Clear(xValue) OpcUa_EnumeratedType_Clear(xValue, OpcUa_PermissionType_None)

#define OpcUa_PermissionType_Initialize(xValue) OpcUa_EnumeratedType_Initialize(xValue, OpcUa_PermissionType_None)

OPCUA_IMEXPORT extern struct _OpcUa_EnumeratedType OpcUa_PermissionType_EnumeratedType;
#endif

#ifndef OPCUA_EXCLUDE_AccessLevelType
/*============================================================================
 * The AccessLevelType enumeration.
 *===========================================================================*/
typedef enum _OpcUa_AccessLevelType
{
    OpcUa_AccessLevelType_None           = 0,
    OpcUa_AccessLevelType_CurrentRead    = 1,
    OpcUa_AccessLevelType_CurrentWrite   = 2,
    OpcUa_AccessLevelType_HistoryRead    = 4,
    OpcUa_AccessLevelType_HistoryWrite   = 8,
    OpcUa_AccessLevelType_SemanticChange = 16,
    OpcUa_AccessLevelType_StatusWrite    = 32,
    OpcUa_AccessLevelType_TimestampWrite = 64
#if OPCUA_FORCE_INT32_ENUMS
    ,_OpcUa_AccessLevelType_MaxEnumerationValue = OpcUa_Int32_Max
#endif
}
OpcUa_AccessLevelType;

#define OpcUa_AccessLevelType_Clear(xValue) OpcUa_EnumeratedType_Clear(xValue, OpcUa_AccessLevelType_None)

#define OpcUa_AccessLevelType_Initialize(xValue) OpcUa_EnumeratedType_Initialize(xValue, OpcUa_AccessLevelType_None)

OPCUA_IMEXPORT extern struct _OpcUa_EnumeratedType OpcUa_AccessLevelType_EnumeratedType;
#endif

#ifndef OPCUA_EXCLUDE_AccessLevelExType
/*============================================================================
 * The AccessLevelExType enumeration.
 *===========================================================================*/
typedef enum _OpcUa_AccessLevelExType
{
    OpcUa_AccessLevelExType_None               = 0,
    OpcUa_AccessLevelExType_CurrentRead        = 1,
    OpcUa_AccessLevelExType_CurrentWrite       = 2,
    OpcUa_AccessLevelExType_HistoryRead        = 4,
    OpcUa_AccessLevelExType_HistoryWrite       = 8,
    OpcUa_AccessLevelExType_SemanticChange     = 16,
    OpcUa_AccessLevelExType_StatusWrite        = 32,
    OpcUa_AccessLevelExType_TimestampWrite     = 64,
    OpcUa_AccessLevelExType_NonatomicRead      = 256,
    OpcUa_AccessLevelExType_NonatomicWrite     = 512,
    OpcUa_AccessLevelExType_WriteFullArrayOnly = 1024
#if OPCUA_FORCE_INT32_ENUMS
    ,_OpcUa_AccessLevelExType_MaxEnumerationValue = OpcUa_Int32_Max
#endif
}
OpcUa_AccessLevelExType;

#define OpcUa_AccessLevelExType_Clear(xValue) OpcUa_EnumeratedType_Clear(xValue, OpcUa_AccessLevelExType_None)

#define OpcUa_AccessLevelExType_Initialize(xValue) OpcUa_EnumeratedType_Initialize(xValue, OpcUa_AccessLevelExType_None)

OPCUA_IMEXPORT extern struct _OpcUa_EnumeratedType OpcUa_AccessLevelExType_EnumeratedType;
#endif

#ifndef OPCUA_EXCLUDE_EventNotifierType
/*============================================================================
 * The EventNotifierType enumeration.
 *===========================================================================*/
typedef enum _OpcUa_EventNotifierType
{
    OpcUa_EventNotifierType_None              = 0,
    OpcUa_EventNotifierType_SubscribeToEvents = 1,
    OpcUa_EventNotifierType_HistoryRead       = 4,
    OpcUa_EventNotifierType_HistoryWrite      = 8
#if OPCUA_FORCE_INT32_ENUMS
    ,_OpcUa_EventNotifierType_MaxEnumerationValue = OpcUa_Int32_Max
#endif
}
OpcUa_EventNotifierType;

#define OpcUa_EventNotifierType_Clear(xValue) OpcUa_EnumeratedType_Clear(xValue, OpcUa_EventNotifierType_None)

#define OpcUa_EventNotifierType_Initialize(xValue) OpcUa_EnumeratedType_Initialize(xValue, OpcUa_EventNotifierType_None)

OPCUA_IMEXPORT extern struct _OpcUa_EnumeratedType OpcUa_EventNotifierType_EnumeratedType;
#endif

#ifndef OPCUA_EXCLUDE_AccessRestrictionType
/*============================================================================
 * The AccessRestrictionType enumeration.
 *===========================================================================*/
typedef enum _OpcUa_AccessRestrictionType
{
    OpcUa_AccessRestrictionType_None               = 0,
    OpcUa_AccessRestrictionType_SigningRequired    = 1,
    OpcUa_AccessRestrictionType_EncryptionRequired = 2,
    OpcUa_AccessRestrictionType_SessionRequired    = 4
#if OPCUA_FORCE_INT32_ENUMS
    ,_OpcUa_AccessRestrictionType_MaxEnumerationValue = OpcUa_Int32_Max
#endif
}
OpcUa_AccessRestrictionType;

#define OpcUa_AccessRestrictionType_Clear(xValue) OpcUa_EnumeratedType_Clear(xValue, OpcUa_AccessRestrictionType_None)

#define OpcUa_AccessRestrictionType_Initialize(xValue) OpcUa_EnumeratedType_Initialize(xValue, OpcUa_AccessRestrictionType_None)

OPCUA_IMEXPORT extern struct _OpcUa_EnumeratedType OpcUa_AccessRestrictionType_EnumeratedType;
#endif

#ifndef OPCUA_EXCLUDE_RolePermissionType
/*============================================================================
 * The RolePermissionType structure.
 *===========================================================================*/
typedef struct _OpcUa_RolePermissionType
{
    OpcUa_NodeId         RoleId;
    OpcUa_PermissionType Permissions;
}
OpcUa_RolePermissionType;

OPCUA_EXPORT OpcUa_Void OpcUa_RolePermissionType_Initialize(OpcUa_RolePermissionType* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_RolePermissionType_Clear(OpcUa_RolePermissionType* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RolePermissionType_GetSize(OpcUa_RolePermissionType* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RolePermissionType_Encode(OpcUa_RolePermissionType* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RolePermissionType_Decode(OpcUa_RolePermissionType* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_RolePermissionType_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_StructureType
/*============================================================================
 * The StructureType enumeration.
 *===========================================================================*/
typedef enum _OpcUa_StructureType
{
    OpcUa_StructureType_Structure                   = 0,
    OpcUa_StructureType_StructureWithOptionalFields = 1,
    OpcUa_StructureType_Union                       = 2
#if OPCUA_FORCE_INT32_ENUMS
    ,_OpcUa_StructureType_MaxEnumerationValue = OpcUa_Int32_Max
#endif
}
OpcUa_StructureType;

#define OpcUa_StructureType_Clear(xValue) OpcUa_EnumeratedType_Clear(xValue, OpcUa_StructureType_Structure)

#define OpcUa_StructureType_Initialize(xValue) OpcUa_EnumeratedType_Initialize(xValue, OpcUa_StructureType_Structure)

OPCUA_IMEXPORT extern struct _OpcUa_EnumeratedType OpcUa_StructureType_EnumeratedType;
#endif

#ifndef OPCUA_EXCLUDE_StructureField
/*============================================================================
 * The StructureField structure.
 *===========================================================================*/
typedef struct _OpcUa_StructureField
{
    OpcUa_String        Name;
    OpcUa_LocalizedText Description;
    OpcUa_NodeId        DataType;
    OpcUa_Int32         ValueRank;
    OpcUa_Int32         NoOfArrayDimensions;
    OpcUa_UInt32*       ArrayDimensions;
    OpcUa_UInt32        MaxStringLength;
    OpcUa_Boolean       IsOptional;
}
OpcUa_StructureField;

OPCUA_EXPORT OpcUa_Void OpcUa_StructureField_Initialize(OpcUa_StructureField* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_StructureField_Clear(OpcUa_StructureField* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_StructureField_GetSize(OpcUa_StructureField* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_StructureField_Encode(OpcUa_StructureField* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_StructureField_Decode(OpcUa_StructureField* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_StructureField_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_StructureDefinition
/*============================================================================
 * The StructureDefinition structure.
 *===========================================================================*/
typedef struct _OpcUa_StructureDefinition
{
    OpcUa_NodeId          DefaultEncodingId;
    OpcUa_NodeId          BaseDataType;
    OpcUa_StructureType   StructureType;
    OpcUa_Int32           NoOfFields;
    OpcUa_StructureField* Fields;
}
OpcUa_StructureDefinition;

OPCUA_EXPORT OpcUa_Void OpcUa_StructureDefinition_Initialize(OpcUa_StructureDefinition* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_StructureDefinition_Clear(OpcUa_StructureDefinition* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_StructureDefinition_GetSize(OpcUa_StructureDefinition* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_StructureDefinition_Encode(OpcUa_StructureDefinition* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_StructureDefinition_Decode(OpcUa_StructureDefinition* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_StructureDefinition_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_EnumValueType
/*============================================================================
 * The EnumValueType structure.
 *===========================================================================*/
typedef struct _OpcUa_EnumValueType
{
    OpcUa_Int64         Value;
    OpcUa_LocalizedText DisplayName;
    OpcUa_LocalizedText Description;
}
OpcUa_EnumValueType;

OPCUA_EXPORT OpcUa_Void OpcUa_EnumValueType_Initialize(OpcUa_EnumValueType* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_EnumValueType_Clear(OpcUa_EnumValueType* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EnumValueType_GetSize(OpcUa_EnumValueType* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EnumValueType_Encode(OpcUa_EnumValueType* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EnumValueType_Decode(OpcUa_EnumValueType* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_EnumValueType_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_EnumField
/*============================================================================
 * The EnumField structure.
 *===========================================================================*/
typedef struct _OpcUa_EnumField
{
    OpcUa_Int64         Value;
    OpcUa_LocalizedText DisplayName;
    OpcUa_LocalizedText Description;
    OpcUa_String        Name;
}
OpcUa_EnumField;

OPCUA_EXPORT OpcUa_Void OpcUa_EnumField_Initialize(OpcUa_EnumField* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_EnumField_Clear(OpcUa_EnumField* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EnumField_GetSize(OpcUa_EnumField* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EnumField_Encode(OpcUa_EnumField* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EnumField_Decode(OpcUa_EnumField* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_EnumField_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_EnumDefinition
/*============================================================================
 * The EnumDefinition structure.
 *===========================================================================*/
typedef struct _OpcUa_EnumDefinition
{
    OpcUa_Int32      NoOfFields;
    OpcUa_EnumField* Fields;
}
OpcUa_EnumDefinition;

OPCUA_EXPORT OpcUa_Void OpcUa_EnumDefinition_Initialize(OpcUa_EnumDefinition* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_EnumDefinition_Clear(OpcUa_EnumDefinition* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EnumDefinition_GetSize(OpcUa_EnumDefinition* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EnumDefinition_Encode(OpcUa_EnumDefinition* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EnumDefinition_Decode(OpcUa_EnumDefinition* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_EnumDefinition_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ReferenceNode
/*============================================================================
 * The ReferenceNode structure.
 *===========================================================================*/
typedef struct _OpcUa_ReferenceNode
{
    OpcUa_NodeId         ReferenceTypeId;
    OpcUa_Boolean        IsInverse;
    OpcUa_ExpandedNodeId TargetId;
}
OpcUa_ReferenceNode;

OPCUA_EXPORT OpcUa_Void OpcUa_ReferenceNode_Initialize(OpcUa_ReferenceNode* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ReferenceNode_Clear(OpcUa_ReferenceNode* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReferenceNode_GetSize(OpcUa_ReferenceNode* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReferenceNode_Encode(OpcUa_ReferenceNode* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReferenceNode_Decode(OpcUa_ReferenceNode* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ReferenceNode_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_Node
/*============================================================================
 * The Node structure.
 *===========================================================================*/
typedef struct _OpcUa_Node
{
    OpcUa_NodeId              NodeId;
    OpcUa_NodeClass           NodeClass;
    OpcUa_QualifiedName       BrowseName;
    OpcUa_LocalizedText       DisplayName;
    OpcUa_LocalizedText       Description;
    OpcUa_UInt32              WriteMask;
    OpcUa_UInt32              UserWriteMask;
    OpcUa_Int32               NoOfRolePermissions;
    OpcUa_RolePermissionType* RolePermissions;
    OpcUa_Int32               NoOfUserRolePermissions;
    OpcUa_RolePermissionType* UserRolePermissions;
    OpcUa_UInt16              AccessRestrictions;
    OpcUa_Int32               NoOfReferences;
    OpcUa_ReferenceNode*      References;
}
OpcUa_Node;

OPCUA_EXPORT OpcUa_Void OpcUa_Node_Initialize(OpcUa_Node* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_Node_Clear(OpcUa_Node* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_Node_GetSize(OpcUa_Node* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_Node_Encode(OpcUa_Node* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_Node_Decode(OpcUa_Node* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_Node_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_InstanceNode
/*============================================================================
 * The InstanceNode structure.
 *===========================================================================*/
typedef struct _OpcUa_InstanceNode
{
    OpcUa_NodeId              NodeId;
    OpcUa_NodeClass           NodeClass;
    OpcUa_QualifiedName       BrowseName;
    OpcUa_LocalizedText       DisplayName;
    OpcUa_LocalizedText       Description;
    OpcUa_UInt32              WriteMask;
    OpcUa_UInt32              UserWriteMask;
    OpcUa_Int32               NoOfRolePermissions;
    OpcUa_RolePermissionType* RolePermissions;
    OpcUa_Int32               NoOfUserRolePermissions;
    OpcUa_RolePermissionType* UserRolePermissions;
    OpcUa_UInt16              AccessRestrictions;
    OpcUa_Int32               NoOfReferences;
    OpcUa_ReferenceNode*      References;
}
OpcUa_InstanceNode;

OPCUA_EXPORT OpcUa_Void OpcUa_InstanceNode_Initialize(OpcUa_InstanceNode* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_InstanceNode_Clear(OpcUa_InstanceNode* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_InstanceNode_GetSize(OpcUa_InstanceNode* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_InstanceNode_Encode(OpcUa_InstanceNode* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_InstanceNode_Decode(OpcUa_InstanceNode* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_InstanceNode_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_TypeNode
/*============================================================================
 * The TypeNode structure.
 *===========================================================================*/
typedef struct _OpcUa_TypeNode
{
    OpcUa_NodeId              NodeId;
    OpcUa_NodeClass           NodeClass;
    OpcUa_QualifiedName       BrowseName;
    OpcUa_LocalizedText       DisplayName;
    OpcUa_LocalizedText       Description;
    OpcUa_UInt32              WriteMask;
    OpcUa_UInt32              UserWriteMask;
    OpcUa_Int32               NoOfRolePermissions;
    OpcUa_RolePermissionType* RolePermissions;
    OpcUa_Int32               NoOfUserRolePermissions;
    OpcUa_RolePermissionType* UserRolePermissions;
    OpcUa_UInt16              AccessRestrictions;
    OpcUa_Int32               NoOfReferences;
    OpcUa_ReferenceNode*      References;
}
OpcUa_TypeNode;

OPCUA_EXPORT OpcUa_Void OpcUa_TypeNode_Initialize(OpcUa_TypeNode* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_TypeNode_Clear(OpcUa_TypeNode* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_TypeNode_GetSize(OpcUa_TypeNode* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_TypeNode_Encode(OpcUa_TypeNode* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_TypeNode_Decode(OpcUa_TypeNode* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_TypeNode_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ObjectNode
/*============================================================================
 * The ObjectNode structure.
 *===========================================================================*/
typedef struct _OpcUa_ObjectNode
{
    OpcUa_NodeId              NodeId;
    OpcUa_NodeClass           NodeClass;
    OpcUa_QualifiedName       BrowseName;
    OpcUa_LocalizedText       DisplayName;
    OpcUa_LocalizedText       Description;
    OpcUa_UInt32              WriteMask;
    OpcUa_UInt32              UserWriteMask;
    OpcUa_Int32               NoOfRolePermissions;
    OpcUa_RolePermissionType* RolePermissions;
    OpcUa_Int32               NoOfUserRolePermissions;
    OpcUa_RolePermissionType* UserRolePermissions;
    OpcUa_UInt16              AccessRestrictions;
    OpcUa_Int32               NoOfReferences;
    OpcUa_ReferenceNode*      References;
    OpcUa_Byte                EventNotifier;
}
OpcUa_ObjectNode;

OPCUA_EXPORT OpcUa_Void OpcUa_ObjectNode_Initialize(OpcUa_ObjectNode* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ObjectNode_Clear(OpcUa_ObjectNode* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ObjectNode_GetSize(OpcUa_ObjectNode* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ObjectNode_Encode(OpcUa_ObjectNode* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ObjectNode_Decode(OpcUa_ObjectNode* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ObjectNode_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ObjectTypeNode
/*============================================================================
 * The ObjectTypeNode structure.
 *===========================================================================*/
typedef struct _OpcUa_ObjectTypeNode
{
    OpcUa_NodeId              NodeId;
    OpcUa_NodeClass           NodeClass;
    OpcUa_QualifiedName       BrowseName;
    OpcUa_LocalizedText       DisplayName;
    OpcUa_LocalizedText       Description;
    OpcUa_UInt32              WriteMask;
    OpcUa_UInt32              UserWriteMask;
    OpcUa_Int32               NoOfRolePermissions;
    OpcUa_RolePermissionType* RolePermissions;
    OpcUa_Int32               NoOfUserRolePermissions;
    OpcUa_RolePermissionType* UserRolePermissions;
    OpcUa_UInt16              AccessRestrictions;
    OpcUa_Int32               NoOfReferences;
    OpcUa_ReferenceNode*      References;
    OpcUa_Boolean             IsAbstract;
}
OpcUa_ObjectTypeNode;

OPCUA_EXPORT OpcUa_Void OpcUa_ObjectTypeNode_Initialize(OpcUa_ObjectTypeNode* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ObjectTypeNode_Clear(OpcUa_ObjectTypeNode* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ObjectTypeNode_GetSize(OpcUa_ObjectTypeNode* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ObjectTypeNode_Encode(OpcUa_ObjectTypeNode* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ObjectTypeNode_Decode(OpcUa_ObjectTypeNode* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ObjectTypeNode_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_VariableNode
/*============================================================================
 * The VariableNode structure.
 *===========================================================================*/
typedef struct _OpcUa_VariableNode
{
    OpcUa_NodeId              NodeId;
    OpcUa_NodeClass           NodeClass;
    OpcUa_QualifiedName       BrowseName;
    OpcUa_LocalizedText       DisplayName;
    OpcUa_LocalizedText       Description;
    OpcUa_UInt32              WriteMask;
    OpcUa_UInt32              UserWriteMask;
    OpcUa_Int32               NoOfRolePermissions;
    OpcUa_RolePermissionType* RolePermissions;
    OpcUa_Int32               NoOfUserRolePermissions;
    OpcUa_RolePermissionType* UserRolePermissions;
    OpcUa_UInt16              AccessRestrictions;
    OpcUa_Int32               NoOfReferences;
    OpcUa_ReferenceNode*      References;
    OpcUa_Variant             Value;
    OpcUa_NodeId              DataType;
    OpcUa_Int32               ValueRank;
    OpcUa_Int32               NoOfArrayDimensions;
    OpcUa_UInt32*             ArrayDimensions;
    OpcUa_Byte                AccessLevel;
    OpcUa_Byte                UserAccessLevel;
    OpcUa_Double              MinimumSamplingInterval;
    OpcUa_Boolean             Historizing;
    OpcUa_UInt32              AccessLevelEx;
}
OpcUa_VariableNode;

OPCUA_EXPORT OpcUa_Void OpcUa_VariableNode_Initialize(OpcUa_VariableNode* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_VariableNode_Clear(OpcUa_VariableNode* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_VariableNode_GetSize(OpcUa_VariableNode* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_VariableNode_Encode(OpcUa_VariableNode* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_VariableNode_Decode(OpcUa_VariableNode* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_VariableNode_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_VariableTypeNode
/*============================================================================
 * The VariableTypeNode structure.
 *===========================================================================*/
typedef struct _OpcUa_VariableTypeNode
{
    OpcUa_NodeId              NodeId;
    OpcUa_NodeClass           NodeClass;
    OpcUa_QualifiedName       BrowseName;
    OpcUa_LocalizedText       DisplayName;
    OpcUa_LocalizedText       Description;
    OpcUa_UInt32              WriteMask;
    OpcUa_UInt32              UserWriteMask;
    OpcUa_Int32               NoOfRolePermissions;
    OpcUa_RolePermissionType* RolePermissions;
    OpcUa_Int32               NoOfUserRolePermissions;
    OpcUa_RolePermissionType* UserRolePermissions;
    OpcUa_UInt16              AccessRestrictions;
    OpcUa_Int32               NoOfReferences;
    OpcUa_ReferenceNode*      References;
    OpcUa_Variant             Value;
    OpcUa_NodeId              DataType;
    OpcUa_Int32               ValueRank;
    OpcUa_Int32               NoOfArrayDimensions;
    OpcUa_UInt32*             ArrayDimensions;
    OpcUa_Boolean             IsAbstract;
}
OpcUa_VariableTypeNode;

OPCUA_EXPORT OpcUa_Void OpcUa_VariableTypeNode_Initialize(OpcUa_VariableTypeNode* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_VariableTypeNode_Clear(OpcUa_VariableTypeNode* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_VariableTypeNode_GetSize(OpcUa_VariableTypeNode* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_VariableTypeNode_Encode(OpcUa_VariableTypeNode* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_VariableTypeNode_Decode(OpcUa_VariableTypeNode* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_VariableTypeNode_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ReferenceTypeNode
/*============================================================================
 * The ReferenceTypeNode structure.
 *===========================================================================*/
typedef struct _OpcUa_ReferenceTypeNode
{
    OpcUa_NodeId              NodeId;
    OpcUa_NodeClass           NodeClass;
    OpcUa_QualifiedName       BrowseName;
    OpcUa_LocalizedText       DisplayName;
    OpcUa_LocalizedText       Description;
    OpcUa_UInt32              WriteMask;
    OpcUa_UInt32              UserWriteMask;
    OpcUa_Int32               NoOfRolePermissions;
    OpcUa_RolePermissionType* RolePermissions;
    OpcUa_Int32               NoOfUserRolePermissions;
    OpcUa_RolePermissionType* UserRolePermissions;
    OpcUa_UInt16              AccessRestrictions;
    OpcUa_Int32               NoOfReferences;
    OpcUa_ReferenceNode*      References;
    OpcUa_Boolean             IsAbstract;
    OpcUa_Boolean             Symmetric;
    OpcUa_LocalizedText       InverseName;
}
OpcUa_ReferenceTypeNode;

OPCUA_EXPORT OpcUa_Void OpcUa_ReferenceTypeNode_Initialize(OpcUa_ReferenceTypeNode* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ReferenceTypeNode_Clear(OpcUa_ReferenceTypeNode* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReferenceTypeNode_GetSize(OpcUa_ReferenceTypeNode* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReferenceTypeNode_Encode(OpcUa_ReferenceTypeNode* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReferenceTypeNode_Decode(OpcUa_ReferenceTypeNode* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ReferenceTypeNode_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_MethodNode
/*============================================================================
 * The MethodNode structure.
 *===========================================================================*/
typedef struct _OpcUa_MethodNode
{
    OpcUa_NodeId              NodeId;
    OpcUa_NodeClass           NodeClass;
    OpcUa_QualifiedName       BrowseName;
    OpcUa_LocalizedText       DisplayName;
    OpcUa_LocalizedText       Description;
    OpcUa_UInt32              WriteMask;
    OpcUa_UInt32              UserWriteMask;
    OpcUa_Int32               NoOfRolePermissions;
    OpcUa_RolePermissionType* RolePermissions;
    OpcUa_Int32               NoOfUserRolePermissions;
    OpcUa_RolePermissionType* UserRolePermissions;
    OpcUa_UInt16              AccessRestrictions;
    OpcUa_Int32               NoOfReferences;
    OpcUa_ReferenceNode*      References;
    OpcUa_Boolean             Executable;
    OpcUa_Boolean             UserExecutable;
}
OpcUa_MethodNode;

OPCUA_EXPORT OpcUa_Void OpcUa_MethodNode_Initialize(OpcUa_MethodNode* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_MethodNode_Clear(OpcUa_MethodNode* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_MethodNode_GetSize(OpcUa_MethodNode* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_MethodNode_Encode(OpcUa_MethodNode* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_MethodNode_Decode(OpcUa_MethodNode* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_MethodNode_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ViewNode
/*============================================================================
 * The ViewNode structure.
 *===========================================================================*/
typedef struct _OpcUa_ViewNode
{
    OpcUa_NodeId              NodeId;
    OpcUa_NodeClass           NodeClass;
    OpcUa_QualifiedName       BrowseName;
    OpcUa_LocalizedText       DisplayName;
    OpcUa_LocalizedText       Description;
    OpcUa_UInt32              WriteMask;
    OpcUa_UInt32              UserWriteMask;
    OpcUa_Int32               NoOfRolePermissions;
    OpcUa_RolePermissionType* RolePermissions;
    OpcUa_Int32               NoOfUserRolePermissions;
    OpcUa_RolePermissionType* UserRolePermissions;
    OpcUa_UInt16              AccessRestrictions;
    OpcUa_Int32               NoOfReferences;
    OpcUa_ReferenceNode*      References;
    OpcUa_Boolean             ContainsNoLoops;
    OpcUa_Byte                EventNotifier;
}
OpcUa_ViewNode;

OPCUA_EXPORT OpcUa_Void OpcUa_ViewNode_Initialize(OpcUa_ViewNode* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ViewNode_Clear(OpcUa_ViewNode* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ViewNode_GetSize(OpcUa_ViewNode* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ViewNode_Encode(OpcUa_ViewNode* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ViewNode_Decode(OpcUa_ViewNode* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ViewNode_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_DataTypeNode
/*============================================================================
 * The DataTypeNode structure.
 *===========================================================================*/
typedef struct _OpcUa_DataTypeNode
{
    OpcUa_NodeId              NodeId;
    OpcUa_NodeClass           NodeClass;
    OpcUa_QualifiedName       BrowseName;
    OpcUa_LocalizedText       DisplayName;
    OpcUa_LocalizedText       Description;
    OpcUa_UInt32              WriteMask;
    OpcUa_UInt32              UserWriteMask;
    OpcUa_Int32               NoOfRolePermissions;
    OpcUa_RolePermissionType* RolePermissions;
    OpcUa_Int32               NoOfUserRolePermissions;
    OpcUa_RolePermissionType* UserRolePermissions;
    OpcUa_UInt16              AccessRestrictions;
    OpcUa_Int32               NoOfReferences;
    OpcUa_ReferenceNode*      References;
    OpcUa_Boolean             IsAbstract;
    OpcUa_ExtensionObject     DataTypeDefinition;
}
OpcUa_DataTypeNode;

OPCUA_EXPORT OpcUa_Void OpcUa_DataTypeNode_Initialize(OpcUa_DataTypeNode* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_DataTypeNode_Clear(OpcUa_DataTypeNode* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DataTypeNode_GetSize(OpcUa_DataTypeNode* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DataTypeNode_Encode(OpcUa_DataTypeNode* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DataTypeNode_Decode(OpcUa_DataTypeNode* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_DataTypeNode_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_Argument
/*============================================================================
 * The Argument structure.
 *===========================================================================*/
typedef struct _OpcUa_Argument
{
    OpcUa_String        Name;
    OpcUa_NodeId        DataType;
    OpcUa_Int32         ValueRank;
    OpcUa_Int32         NoOfArrayDimensions;
    OpcUa_UInt32*       ArrayDimensions;
    OpcUa_LocalizedText Description;
}
OpcUa_Argument;

OPCUA_EXPORT OpcUa_Void OpcUa_Argument_Initialize(OpcUa_Argument* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_Argument_Clear(OpcUa_Argument* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_Argument_GetSize(OpcUa_Argument* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_Argument_Encode(OpcUa_Argument* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_Argument_Decode(OpcUa_Argument* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_Argument_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_OptionSet
/*============================================================================
 * The OptionSet structure.
 *===========================================================================*/
typedef struct _OpcUa_OptionSet
{
    OpcUa_ByteString Value;
    OpcUa_ByteString ValidBits;
}
OpcUa_OptionSet;

OPCUA_EXPORT OpcUa_Void OpcUa_OptionSet_Initialize(OpcUa_OptionSet* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_OptionSet_Clear(OpcUa_OptionSet* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_OptionSet_GetSize(OpcUa_OptionSet* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_OptionSet_Encode(OpcUa_OptionSet* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_OptionSet_Decode(OpcUa_OptionSet* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_OptionSet_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_TimeZoneDataType
/*============================================================================
 * The TimeZoneDataType structure.
 *===========================================================================*/
typedef struct _OpcUa_TimeZoneDataType
{
    OpcUa_Int16   Offset;
    OpcUa_Boolean DaylightSavingInOffset;
}
OpcUa_TimeZoneDataType;

OPCUA_EXPORT OpcUa_Void OpcUa_TimeZoneDataType_Initialize(OpcUa_TimeZoneDataType* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_TimeZoneDataType_Clear(OpcUa_TimeZoneDataType* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_TimeZoneDataType_GetSize(OpcUa_TimeZoneDataType* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_TimeZoneDataType_Encode(OpcUa_TimeZoneDataType* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_TimeZoneDataType_Decode(OpcUa_TimeZoneDataType* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_TimeZoneDataType_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ApplicationType
/*============================================================================
 * The ApplicationType enumeration.
 *===========================================================================*/
typedef enum _OpcUa_ApplicationType
{
    OpcUa_ApplicationType_Server          = 0,
    OpcUa_ApplicationType_Client          = 1,
    OpcUa_ApplicationType_ClientAndServer = 2,
    OpcUa_ApplicationType_DiscoveryServer = 3
#if OPCUA_FORCE_INT32_ENUMS
    ,_OpcUa_ApplicationType_MaxEnumerationValue = OpcUa_Int32_Max
#endif
}
OpcUa_ApplicationType;

#define OpcUa_ApplicationType_Clear(xValue) OpcUa_EnumeratedType_Clear(xValue, OpcUa_ApplicationType_Server)

#define OpcUa_ApplicationType_Initialize(xValue) OpcUa_EnumeratedType_Initialize(xValue, OpcUa_ApplicationType_Server)

OPCUA_IMEXPORT extern struct _OpcUa_EnumeratedType OpcUa_ApplicationType_EnumeratedType;
#endif

#ifndef OPCUA_EXCLUDE_ApplicationDescription
/*============================================================================
 * The ApplicationDescription structure.
 *===========================================================================*/
typedef struct _OpcUa_ApplicationDescription
{
    OpcUa_String          ApplicationUri;
    OpcUa_String          ProductUri;
    OpcUa_LocalizedText   ApplicationName;
    OpcUa_ApplicationType ApplicationType;
    OpcUa_String          GatewayServerUri;
    OpcUa_String          DiscoveryProfileUri;
    OpcUa_Int32           NoOfDiscoveryUrls;
    OpcUa_String*         DiscoveryUrls;
}
OpcUa_ApplicationDescription;

OPCUA_EXPORT OpcUa_Void OpcUa_ApplicationDescription_Initialize(OpcUa_ApplicationDescription* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ApplicationDescription_Clear(OpcUa_ApplicationDescription* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ApplicationDescription_GetSize(OpcUa_ApplicationDescription* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ApplicationDescription_Encode(OpcUa_ApplicationDescription* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ApplicationDescription_Decode(OpcUa_ApplicationDescription* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ApplicationDescription_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_RequestHeader
/*============================================================================
 * The RequestHeader structure.
 *===========================================================================*/
typedef struct _OpcUa_RequestHeader
{
    OpcUa_NodeId          AuthenticationToken;
    OpcUa_DateTime        Timestamp;
    OpcUa_UInt32          RequestHandle;
    OpcUa_UInt32          ReturnDiagnostics;
    OpcUa_String          AuditEntryId;
    OpcUa_UInt32          TimeoutHint;
    OpcUa_ExtensionObject AdditionalHeader;
}
OpcUa_RequestHeader;

OPCUA_EXPORT OpcUa_Void OpcUa_RequestHeader_Initialize(OpcUa_RequestHeader* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_RequestHeader_Clear(OpcUa_RequestHeader* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RequestHeader_GetSize(OpcUa_RequestHeader* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RequestHeader_Encode(OpcUa_RequestHeader* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RequestHeader_Decode(OpcUa_RequestHeader* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_RequestHeader_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ResponseHeader
/*============================================================================
 * The ResponseHeader structure.
 *===========================================================================*/
typedef struct _OpcUa_ResponseHeader
{
    OpcUa_DateTime        Timestamp;
    OpcUa_UInt32          RequestHandle;
    OpcUa_StatusCode      ServiceResult;
    OpcUa_DiagnosticInfo  ServiceDiagnostics;
    OpcUa_Int32           NoOfStringTable;
    OpcUa_String*         StringTable;
    OpcUa_ExtensionObject AdditionalHeader;
}
OpcUa_ResponseHeader;

OPCUA_EXPORT OpcUa_Void OpcUa_ResponseHeader_Initialize(OpcUa_ResponseHeader* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ResponseHeader_Clear(OpcUa_ResponseHeader* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ResponseHeader_GetSize(OpcUa_ResponseHeader* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ResponseHeader_Encode(OpcUa_ResponseHeader* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ResponseHeader_Decode(OpcUa_ResponseHeader* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ResponseHeader_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ServiceFault
/*============================================================================
 * The ServiceFault structure.
 *===========================================================================*/
typedef struct _OpcUa_ServiceFault
{
    OpcUa_ResponseHeader ResponseHeader;
}
OpcUa_ServiceFault;

OPCUA_EXPORT OpcUa_Void OpcUa_ServiceFault_Initialize(OpcUa_ServiceFault* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ServiceFault_Clear(OpcUa_ServiceFault* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ServiceFault_GetSize(OpcUa_ServiceFault* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ServiceFault_Encode(OpcUa_ServiceFault* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ServiceFault_Decode(OpcUa_ServiceFault* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ServiceFault_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_SessionlessInvokeRequestType
/*============================================================================
 * The SessionlessInvokeRequestType structure.
 *===========================================================================*/
typedef struct _OpcUa_SessionlessInvokeRequestType
{
    OpcUa_Int32   NoOfUrisVersion;
    OpcUa_UInt32* UrisVersion;
    OpcUa_Int32   NoOfNamespaceUris;
    OpcUa_String* NamespaceUris;
    OpcUa_Int32   NoOfServerUris;
    OpcUa_String* ServerUris;
    OpcUa_Int32   NoOfLocaleIds;
    OpcUa_String* LocaleIds;
    OpcUa_UInt32  ServiceId;
}
OpcUa_SessionlessInvokeRequestType;

OPCUA_EXPORT OpcUa_Void OpcUa_SessionlessInvokeRequestType_Initialize(OpcUa_SessionlessInvokeRequestType* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_SessionlessInvokeRequestType_Clear(OpcUa_SessionlessInvokeRequestType* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SessionlessInvokeRequestType_GetSize(OpcUa_SessionlessInvokeRequestType* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SessionlessInvokeRequestType_Encode(OpcUa_SessionlessInvokeRequestType* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SessionlessInvokeRequestType_Decode(OpcUa_SessionlessInvokeRequestType* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_SessionlessInvokeRequestType_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_SessionlessInvokeResponseType
/*============================================================================
 * The SessionlessInvokeResponseType structure.
 *===========================================================================*/
typedef struct _OpcUa_SessionlessInvokeResponseType
{
    OpcUa_Int32   NoOfNamespaceUris;
    OpcUa_String* NamespaceUris;
    OpcUa_Int32   NoOfServerUris;
    OpcUa_String* ServerUris;
    OpcUa_UInt32  ServiceId;
}
OpcUa_SessionlessInvokeResponseType;

OPCUA_EXPORT OpcUa_Void OpcUa_SessionlessInvokeResponseType_Initialize(OpcUa_SessionlessInvokeResponseType* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_SessionlessInvokeResponseType_Clear(OpcUa_SessionlessInvokeResponseType* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SessionlessInvokeResponseType_GetSize(OpcUa_SessionlessInvokeResponseType* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SessionlessInvokeResponseType_Encode(OpcUa_SessionlessInvokeResponseType* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SessionlessInvokeResponseType_Decode(OpcUa_SessionlessInvokeResponseType* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_SessionlessInvokeResponseType_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_FindServers
#ifndef OPCUA_EXCLUDE_FindServersRequest
/*============================================================================
 * The FindServersRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_FindServersRequest
{
    OpcUa_RequestHeader RequestHeader;
    OpcUa_String        EndpointUrl;
    OpcUa_Int32         NoOfLocaleIds;
    OpcUa_String*       LocaleIds;
    OpcUa_Int32         NoOfServerUris;
    OpcUa_String*       ServerUris;
}
OpcUa_FindServersRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_FindServersRequest_Initialize(OpcUa_FindServersRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_FindServersRequest_Clear(OpcUa_FindServersRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_FindServersRequest_GetSize(OpcUa_FindServersRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_FindServersRequest_Encode(OpcUa_FindServersRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_FindServersRequest_Decode(OpcUa_FindServersRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_FindServersRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_FindServersResponse
/*============================================================================
 * The FindServersResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_FindServersResponse
{
    OpcUa_ResponseHeader          ResponseHeader;
    OpcUa_Int32                   NoOfServers;
    OpcUa_ApplicationDescription* Servers;
}
OpcUa_FindServersResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_FindServersResponse_Initialize(OpcUa_FindServersResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_FindServersResponse_Clear(OpcUa_FindServersResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_FindServersResponse_GetSize(OpcUa_FindServersResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_FindServersResponse_Encode(OpcUa_FindServersResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_FindServersResponse_Decode(OpcUa_FindServersResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_FindServersResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_ServerOnNetwork
/*============================================================================
 * The ServerOnNetwork structure.
 *===========================================================================*/
typedef struct _OpcUa_ServerOnNetwork
{
    OpcUa_UInt32  RecordId;
    OpcUa_String  ServerName;
    OpcUa_String  DiscoveryUrl;
    OpcUa_Int32   NoOfServerCapabilities;
    OpcUa_String* ServerCapabilities;
}
OpcUa_ServerOnNetwork;

OPCUA_EXPORT OpcUa_Void OpcUa_ServerOnNetwork_Initialize(OpcUa_ServerOnNetwork* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ServerOnNetwork_Clear(OpcUa_ServerOnNetwork* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ServerOnNetwork_GetSize(OpcUa_ServerOnNetwork* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ServerOnNetwork_Encode(OpcUa_ServerOnNetwork* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ServerOnNetwork_Decode(OpcUa_ServerOnNetwork* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ServerOnNetwork_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_FindServersOnNetwork
#ifndef OPCUA_EXCLUDE_FindServersOnNetworkRequest
/*============================================================================
 * The FindServersOnNetworkRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_FindServersOnNetworkRequest
{
    OpcUa_RequestHeader RequestHeader;
    OpcUa_UInt32        StartingRecordId;
    OpcUa_UInt32        MaxRecordsToReturn;
    OpcUa_Int32         NoOfServerCapabilityFilter;
    OpcUa_String*       ServerCapabilityFilter;
}
OpcUa_FindServersOnNetworkRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_FindServersOnNetworkRequest_Initialize(OpcUa_FindServersOnNetworkRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_FindServersOnNetworkRequest_Clear(OpcUa_FindServersOnNetworkRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_FindServersOnNetworkRequest_GetSize(OpcUa_FindServersOnNetworkRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_FindServersOnNetworkRequest_Encode(OpcUa_FindServersOnNetworkRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_FindServersOnNetworkRequest_Decode(OpcUa_FindServersOnNetworkRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_FindServersOnNetworkRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_FindServersOnNetworkResponse
/*============================================================================
 * The FindServersOnNetworkResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_FindServersOnNetworkResponse
{
    OpcUa_ResponseHeader   ResponseHeader;
    OpcUa_DateTime         LastCounterResetTime;
    OpcUa_Int32            NoOfServers;
    OpcUa_ServerOnNetwork* Servers;
}
OpcUa_FindServersOnNetworkResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_FindServersOnNetworkResponse_Initialize(OpcUa_FindServersOnNetworkResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_FindServersOnNetworkResponse_Clear(OpcUa_FindServersOnNetworkResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_FindServersOnNetworkResponse_GetSize(OpcUa_FindServersOnNetworkResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_FindServersOnNetworkResponse_Encode(OpcUa_FindServersOnNetworkResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_FindServersOnNetworkResponse_Decode(OpcUa_FindServersOnNetworkResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_FindServersOnNetworkResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_MessageSecurityMode
/*============================================================================
 * The MessageSecurityMode enumeration.
 *===========================================================================*/
typedef enum _OpcUa_MessageSecurityMode
{
    OpcUa_MessageSecurityMode_Invalid        = 0,
    OpcUa_MessageSecurityMode_None           = 1,
    OpcUa_MessageSecurityMode_Sign           = 2,
    OpcUa_MessageSecurityMode_SignAndEncrypt = 3
#if OPCUA_FORCE_INT32_ENUMS
    ,_OpcUa_MessageSecurityMode_MaxEnumerationValue = OpcUa_Int32_Max
#endif
}
OpcUa_MessageSecurityMode;

#define OpcUa_MessageSecurityMode_Clear(xValue) OpcUa_EnumeratedType_Clear(xValue, OpcUa_MessageSecurityMode_Invalid)

#define OpcUa_MessageSecurityMode_Initialize(xValue) OpcUa_EnumeratedType_Initialize(xValue, OpcUa_MessageSecurityMode_Invalid)

OPCUA_IMEXPORT extern struct _OpcUa_EnumeratedType OpcUa_MessageSecurityMode_EnumeratedType;
#endif

#ifndef OPCUA_EXCLUDE_UserTokenType
/*============================================================================
 * The UserTokenType enumeration.
 *===========================================================================*/
typedef enum _OpcUa_UserTokenType
{
    OpcUa_UserTokenType_Anonymous   = 0,
    OpcUa_UserTokenType_UserName    = 1,
    OpcUa_UserTokenType_Certificate = 2,
    OpcUa_UserTokenType_IssuedToken = 3
#if OPCUA_FORCE_INT32_ENUMS
    ,_OpcUa_UserTokenType_MaxEnumerationValue = OpcUa_Int32_Max
#endif
}
OpcUa_UserTokenType;

#define OpcUa_UserTokenType_Clear(xValue) OpcUa_EnumeratedType_Clear(xValue, OpcUa_UserTokenType_Anonymous)

#define OpcUa_UserTokenType_Initialize(xValue) OpcUa_EnumeratedType_Initialize(xValue, OpcUa_UserTokenType_Anonymous)

OPCUA_IMEXPORT extern struct _OpcUa_EnumeratedType OpcUa_UserTokenType_EnumeratedType;
#endif

#ifndef OPCUA_EXCLUDE_UserTokenPolicy
/*============================================================================
 * The UserTokenPolicy structure.
 *===========================================================================*/
typedef struct _OpcUa_UserTokenPolicy
{
    OpcUa_String        PolicyId;
    OpcUa_UserTokenType TokenType;
    OpcUa_String        IssuedTokenType;
    OpcUa_String        IssuerEndpointUrl;
    OpcUa_String        SecurityPolicyUri;
}
OpcUa_UserTokenPolicy;

OPCUA_EXPORT OpcUa_Void OpcUa_UserTokenPolicy_Initialize(OpcUa_UserTokenPolicy* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_UserTokenPolicy_Clear(OpcUa_UserTokenPolicy* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_UserTokenPolicy_GetSize(OpcUa_UserTokenPolicy* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_UserTokenPolicy_Encode(OpcUa_UserTokenPolicy* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_UserTokenPolicy_Decode(OpcUa_UserTokenPolicy* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_UserTokenPolicy_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_EndpointDescription
/*============================================================================
 * The EndpointDescription structure.
 *===========================================================================*/
typedef struct _OpcUa_EndpointDescription
{
    OpcUa_String                 EndpointUrl;
    OpcUa_ApplicationDescription Server;
    OpcUa_ByteString             ServerCertificate;
    OpcUa_MessageSecurityMode    SecurityMode;
    OpcUa_String                 SecurityPolicyUri;
    OpcUa_Int32                  NoOfUserIdentityTokens;
    OpcUa_UserTokenPolicy*       UserIdentityTokens;
    OpcUa_String                 TransportProfileUri;
    OpcUa_Byte                   SecurityLevel;
}
OpcUa_EndpointDescription;

OPCUA_EXPORT OpcUa_Void OpcUa_EndpointDescription_Initialize(OpcUa_EndpointDescription* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_EndpointDescription_Clear(OpcUa_EndpointDescription* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EndpointDescription_GetSize(OpcUa_EndpointDescription* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EndpointDescription_Encode(OpcUa_EndpointDescription* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EndpointDescription_Decode(OpcUa_EndpointDescription* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_EndpointDescription_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_GetEndpoints
#ifndef OPCUA_EXCLUDE_GetEndpointsRequest
/*============================================================================
 * The GetEndpointsRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_GetEndpointsRequest
{
    OpcUa_RequestHeader RequestHeader;
    OpcUa_String        EndpointUrl;
    OpcUa_Int32         NoOfLocaleIds;
    OpcUa_String*       LocaleIds;
    OpcUa_Int32         NoOfProfileUris;
    OpcUa_String*       ProfileUris;
}
OpcUa_GetEndpointsRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_GetEndpointsRequest_Initialize(OpcUa_GetEndpointsRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_GetEndpointsRequest_Clear(OpcUa_GetEndpointsRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_GetEndpointsRequest_GetSize(OpcUa_GetEndpointsRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_GetEndpointsRequest_Encode(OpcUa_GetEndpointsRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_GetEndpointsRequest_Decode(OpcUa_GetEndpointsRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_GetEndpointsRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_GetEndpointsResponse
/*============================================================================
 * The GetEndpointsResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_GetEndpointsResponse
{
    OpcUa_ResponseHeader       ResponseHeader;
    OpcUa_Int32                NoOfEndpoints;
    OpcUa_EndpointDescription* Endpoints;
}
OpcUa_GetEndpointsResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_GetEndpointsResponse_Initialize(OpcUa_GetEndpointsResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_GetEndpointsResponse_Clear(OpcUa_GetEndpointsResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_GetEndpointsResponse_GetSize(OpcUa_GetEndpointsResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_GetEndpointsResponse_Encode(OpcUa_GetEndpointsResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_GetEndpointsResponse_Decode(OpcUa_GetEndpointsResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_GetEndpointsResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_RegisteredServer
/*============================================================================
 * The RegisteredServer structure.
 *===========================================================================*/
typedef struct _OpcUa_RegisteredServer
{
    OpcUa_String          ServerUri;
    OpcUa_String          ProductUri;
    OpcUa_Int32           NoOfServerNames;
    OpcUa_LocalizedText*  ServerNames;
    OpcUa_ApplicationType ServerType;
    OpcUa_String          GatewayServerUri;
    OpcUa_Int32           NoOfDiscoveryUrls;
    OpcUa_String*         DiscoveryUrls;
    OpcUa_String          SemaphoreFilePath;
    OpcUa_Boolean         IsOnline;
}
OpcUa_RegisteredServer;

OPCUA_EXPORT OpcUa_Void OpcUa_RegisteredServer_Initialize(OpcUa_RegisteredServer* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_RegisteredServer_Clear(OpcUa_RegisteredServer* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RegisteredServer_GetSize(OpcUa_RegisteredServer* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RegisteredServer_Encode(OpcUa_RegisteredServer* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RegisteredServer_Decode(OpcUa_RegisteredServer* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_RegisteredServer_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_RegisterServer
#ifndef OPCUA_EXCLUDE_RegisterServerRequest
/*============================================================================
 * The RegisterServerRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_RegisterServerRequest
{
    OpcUa_RequestHeader    RequestHeader;
    OpcUa_RegisteredServer Server;
}
OpcUa_RegisterServerRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_RegisterServerRequest_Initialize(OpcUa_RegisterServerRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_RegisterServerRequest_Clear(OpcUa_RegisterServerRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RegisterServerRequest_GetSize(OpcUa_RegisterServerRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RegisterServerRequest_Encode(OpcUa_RegisterServerRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RegisterServerRequest_Decode(OpcUa_RegisterServerRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_RegisterServerRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_RegisterServerResponse
/*============================================================================
 * The RegisterServerResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_RegisterServerResponse
{
    OpcUa_ResponseHeader ResponseHeader;
}
OpcUa_RegisterServerResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_RegisterServerResponse_Initialize(OpcUa_RegisterServerResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_RegisterServerResponse_Clear(OpcUa_RegisterServerResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RegisterServerResponse_GetSize(OpcUa_RegisterServerResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RegisterServerResponse_Encode(OpcUa_RegisterServerResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RegisterServerResponse_Decode(OpcUa_RegisterServerResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_RegisterServerResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_MdnsDiscoveryConfiguration
/*============================================================================
 * The MdnsDiscoveryConfiguration structure.
 *===========================================================================*/
typedef struct _OpcUa_MdnsDiscoveryConfiguration
{
    OpcUa_String  MdnsServerName;
    OpcUa_Int32   NoOfServerCapabilities;
    OpcUa_String* ServerCapabilities;
}
OpcUa_MdnsDiscoveryConfiguration;

OPCUA_EXPORT OpcUa_Void OpcUa_MdnsDiscoveryConfiguration_Initialize(OpcUa_MdnsDiscoveryConfiguration* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_MdnsDiscoveryConfiguration_Clear(OpcUa_MdnsDiscoveryConfiguration* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_MdnsDiscoveryConfiguration_GetSize(OpcUa_MdnsDiscoveryConfiguration* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_MdnsDiscoveryConfiguration_Encode(OpcUa_MdnsDiscoveryConfiguration* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_MdnsDiscoveryConfiguration_Decode(OpcUa_MdnsDiscoveryConfiguration* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_MdnsDiscoveryConfiguration_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_RegisterServer2
#ifndef OPCUA_EXCLUDE_RegisterServer2Request
/*============================================================================
 * The RegisterServer2Request structure.
 *===========================================================================*/
typedef struct _OpcUa_RegisterServer2Request
{
    OpcUa_RequestHeader    RequestHeader;
    OpcUa_RegisteredServer Server;
    OpcUa_Int32            NoOfDiscoveryConfiguration;
    OpcUa_ExtensionObject* DiscoveryConfiguration;
}
OpcUa_RegisterServer2Request;

OPCUA_EXPORT OpcUa_Void OpcUa_RegisterServer2Request_Initialize(OpcUa_RegisterServer2Request* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_RegisterServer2Request_Clear(OpcUa_RegisterServer2Request* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RegisterServer2Request_GetSize(OpcUa_RegisterServer2Request* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RegisterServer2Request_Encode(OpcUa_RegisterServer2Request* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RegisterServer2Request_Decode(OpcUa_RegisterServer2Request* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_RegisterServer2Request_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_RegisterServer2Response
/*============================================================================
 * The RegisterServer2Response structure.
 *===========================================================================*/
typedef struct _OpcUa_RegisterServer2Response
{
    OpcUa_ResponseHeader  ResponseHeader;
    OpcUa_Int32           NoOfConfigurationResults;
    OpcUa_StatusCode*     ConfigurationResults;
    OpcUa_Int32           NoOfDiagnosticInfos;
    OpcUa_DiagnosticInfo* DiagnosticInfos;
}
OpcUa_RegisterServer2Response;

OPCUA_EXPORT OpcUa_Void OpcUa_RegisterServer2Response_Initialize(OpcUa_RegisterServer2Response* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_RegisterServer2Response_Clear(OpcUa_RegisterServer2Response* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RegisterServer2Response_GetSize(OpcUa_RegisterServer2Response* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RegisterServer2Response_Encode(OpcUa_RegisterServer2Response* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RegisterServer2Response_Decode(OpcUa_RegisterServer2Response* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_RegisterServer2Response_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_SecurityTokenRequestType
/*============================================================================
 * The SecurityTokenRequestType enumeration.
 *===========================================================================*/
typedef enum _OpcUa_SecurityTokenRequestType
{
    OpcUa_SecurityTokenRequestType_Issue = 0,
    OpcUa_SecurityTokenRequestType_Renew = 1
#if OPCUA_FORCE_INT32_ENUMS
    ,_OpcUa_SecurityTokenRequestType_MaxEnumerationValue = OpcUa_Int32_Max
#endif
}
OpcUa_SecurityTokenRequestType;

#define OpcUa_SecurityTokenRequestType_Clear(xValue) OpcUa_EnumeratedType_Clear(xValue, OpcUa_SecurityTokenRequestType_Issue)

#define OpcUa_SecurityTokenRequestType_Initialize(xValue) OpcUa_EnumeratedType_Initialize(xValue, OpcUa_SecurityTokenRequestType_Issue)

OPCUA_IMEXPORT extern struct _OpcUa_EnumeratedType OpcUa_SecurityTokenRequestType_EnumeratedType;
#endif

#ifndef OPCUA_EXCLUDE_ChannelSecurityToken
/*============================================================================
 * The ChannelSecurityToken structure.
 *===========================================================================*/
typedef struct _OpcUa_ChannelSecurityToken
{
    OpcUa_UInt32   ChannelId;
    OpcUa_UInt32   TokenId;
    OpcUa_DateTime CreatedAt;
    OpcUa_UInt32   RevisedLifetime;
}
OpcUa_ChannelSecurityToken;

OPCUA_EXPORT OpcUa_Void OpcUa_ChannelSecurityToken_Initialize(OpcUa_ChannelSecurityToken* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ChannelSecurityToken_Clear(OpcUa_ChannelSecurityToken* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ChannelSecurityToken_GetSize(OpcUa_ChannelSecurityToken* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ChannelSecurityToken_Encode(OpcUa_ChannelSecurityToken* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ChannelSecurityToken_Decode(OpcUa_ChannelSecurityToken* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ChannelSecurityToken_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_OpenSecureChannel
#ifndef OPCUA_EXCLUDE_OpenSecureChannelRequest
/*============================================================================
 * The OpenSecureChannelRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_OpenSecureChannelRequest
{
    OpcUa_RequestHeader            RequestHeader;
    OpcUa_UInt32                   ClientProtocolVersion;
    OpcUa_SecurityTokenRequestType RequestType;
    OpcUa_MessageSecurityMode      SecurityMode;
    OpcUa_ByteString               ClientNonce;
    OpcUa_UInt32                   RequestedLifetime;
}
OpcUa_OpenSecureChannelRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_OpenSecureChannelRequest_Initialize(OpcUa_OpenSecureChannelRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_OpenSecureChannelRequest_Clear(OpcUa_OpenSecureChannelRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_OpenSecureChannelRequest_GetSize(OpcUa_OpenSecureChannelRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_OpenSecureChannelRequest_Encode(OpcUa_OpenSecureChannelRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_OpenSecureChannelRequest_Decode(OpcUa_OpenSecureChannelRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_OpenSecureChannelRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_OpenSecureChannelResponse
/*============================================================================
 * The OpenSecureChannelResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_OpenSecureChannelResponse
{
    OpcUa_ResponseHeader       ResponseHeader;
    OpcUa_UInt32               ServerProtocolVersion;
    OpcUa_ChannelSecurityToken SecurityToken;
    OpcUa_ByteString           ServerNonce;
}
OpcUa_OpenSecureChannelResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_OpenSecureChannelResponse_Initialize(OpcUa_OpenSecureChannelResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_OpenSecureChannelResponse_Clear(OpcUa_OpenSecureChannelResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_OpenSecureChannelResponse_GetSize(OpcUa_OpenSecureChannelResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_OpenSecureChannelResponse_Encode(OpcUa_OpenSecureChannelResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_OpenSecureChannelResponse_Decode(OpcUa_OpenSecureChannelResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_OpenSecureChannelResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_CloseSecureChannel
#ifndef OPCUA_EXCLUDE_CloseSecureChannelRequest
/*============================================================================
 * The CloseSecureChannelRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_CloseSecureChannelRequest
{
    OpcUa_RequestHeader RequestHeader;
}
OpcUa_CloseSecureChannelRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_CloseSecureChannelRequest_Initialize(OpcUa_CloseSecureChannelRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_CloseSecureChannelRequest_Clear(OpcUa_CloseSecureChannelRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CloseSecureChannelRequest_GetSize(OpcUa_CloseSecureChannelRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CloseSecureChannelRequest_Encode(OpcUa_CloseSecureChannelRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CloseSecureChannelRequest_Decode(OpcUa_CloseSecureChannelRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_CloseSecureChannelRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_CloseSecureChannelResponse
/*============================================================================
 * The CloseSecureChannelResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_CloseSecureChannelResponse
{
    OpcUa_ResponseHeader ResponseHeader;
}
OpcUa_CloseSecureChannelResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_CloseSecureChannelResponse_Initialize(OpcUa_CloseSecureChannelResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_CloseSecureChannelResponse_Clear(OpcUa_CloseSecureChannelResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CloseSecureChannelResponse_GetSize(OpcUa_CloseSecureChannelResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CloseSecureChannelResponse_Encode(OpcUa_CloseSecureChannelResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CloseSecureChannelResponse_Decode(OpcUa_CloseSecureChannelResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_CloseSecureChannelResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_SignedSoftwareCertificate
/*============================================================================
 * The SignedSoftwareCertificate structure.
 *===========================================================================*/
typedef struct _OpcUa_SignedSoftwareCertificate
{
    OpcUa_ByteString CertificateData;
    OpcUa_ByteString Signature;
}
OpcUa_SignedSoftwareCertificate;

OPCUA_EXPORT OpcUa_Void OpcUa_SignedSoftwareCertificate_Initialize(OpcUa_SignedSoftwareCertificate* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_SignedSoftwareCertificate_Clear(OpcUa_SignedSoftwareCertificate* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SignedSoftwareCertificate_GetSize(OpcUa_SignedSoftwareCertificate* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SignedSoftwareCertificate_Encode(OpcUa_SignedSoftwareCertificate* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SignedSoftwareCertificate_Decode(OpcUa_SignedSoftwareCertificate* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_SignedSoftwareCertificate_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_SignatureData
/*============================================================================
 * The SignatureData structure.
 *===========================================================================*/
typedef struct _OpcUa_SignatureData
{
    OpcUa_String     Algorithm;
    OpcUa_ByteString Signature;
}
OpcUa_SignatureData;

OPCUA_EXPORT OpcUa_Void OpcUa_SignatureData_Initialize(OpcUa_SignatureData* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_SignatureData_Clear(OpcUa_SignatureData* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SignatureData_GetSize(OpcUa_SignatureData* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SignatureData_Encode(OpcUa_SignatureData* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SignatureData_Decode(OpcUa_SignatureData* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_SignatureData_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_CreateSession
#ifndef OPCUA_EXCLUDE_CreateSessionRequest
/*============================================================================
 * The CreateSessionRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_CreateSessionRequest
{
    OpcUa_RequestHeader          RequestHeader;
    OpcUa_ApplicationDescription ClientDescription;
    OpcUa_String                 ServerUri;
    OpcUa_String                 EndpointUrl;
    OpcUa_String                 SessionName;
    OpcUa_ByteString             ClientNonce;
    OpcUa_ByteString             ClientCertificate;
    OpcUa_Double                 RequestedSessionTimeout;
    OpcUa_UInt32                 MaxResponseMessageSize;
}
OpcUa_CreateSessionRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_CreateSessionRequest_Initialize(OpcUa_CreateSessionRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_CreateSessionRequest_Clear(OpcUa_CreateSessionRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CreateSessionRequest_GetSize(OpcUa_CreateSessionRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CreateSessionRequest_Encode(OpcUa_CreateSessionRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CreateSessionRequest_Decode(OpcUa_CreateSessionRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_CreateSessionRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_CreateSessionResponse
/*============================================================================
 * The CreateSessionResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_CreateSessionResponse
{
    OpcUa_ResponseHeader             ResponseHeader;
    OpcUa_NodeId                     SessionId;
    OpcUa_NodeId                     AuthenticationToken;
    OpcUa_Double                     RevisedSessionTimeout;
    OpcUa_ByteString                 ServerNonce;
    OpcUa_ByteString                 ServerCertificate;
    OpcUa_Int32                      NoOfServerEndpoints;
    OpcUa_EndpointDescription*       ServerEndpoints;
    OpcUa_Int32                      NoOfServerSoftwareCertificates;
    OpcUa_SignedSoftwareCertificate* ServerSoftwareCertificates;
    OpcUa_SignatureData              ServerSignature;
    OpcUa_UInt32                     MaxRequestMessageSize;
}
OpcUa_CreateSessionResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_CreateSessionResponse_Initialize(OpcUa_CreateSessionResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_CreateSessionResponse_Clear(OpcUa_CreateSessionResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CreateSessionResponse_GetSize(OpcUa_CreateSessionResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CreateSessionResponse_Encode(OpcUa_CreateSessionResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CreateSessionResponse_Decode(OpcUa_CreateSessionResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_CreateSessionResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_UserIdentityToken
/*============================================================================
 * The UserIdentityToken structure.
 *===========================================================================*/
typedef struct _OpcUa_UserIdentityToken
{
    OpcUa_String PolicyId;
}
OpcUa_UserIdentityToken;

OPCUA_EXPORT OpcUa_Void OpcUa_UserIdentityToken_Initialize(OpcUa_UserIdentityToken* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_UserIdentityToken_Clear(OpcUa_UserIdentityToken* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_UserIdentityToken_GetSize(OpcUa_UserIdentityToken* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_UserIdentityToken_Encode(OpcUa_UserIdentityToken* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_UserIdentityToken_Decode(OpcUa_UserIdentityToken* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_UserIdentityToken_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_AnonymousIdentityToken
/*============================================================================
 * The AnonymousIdentityToken structure.
 *===========================================================================*/
typedef struct _OpcUa_AnonymousIdentityToken
{
    OpcUa_String PolicyId;
}
OpcUa_AnonymousIdentityToken;

OPCUA_EXPORT OpcUa_Void OpcUa_AnonymousIdentityToken_Initialize(OpcUa_AnonymousIdentityToken* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_AnonymousIdentityToken_Clear(OpcUa_AnonymousIdentityToken* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AnonymousIdentityToken_GetSize(OpcUa_AnonymousIdentityToken* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AnonymousIdentityToken_Encode(OpcUa_AnonymousIdentityToken* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AnonymousIdentityToken_Decode(OpcUa_AnonymousIdentityToken* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_AnonymousIdentityToken_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_UserNameIdentityToken
/*============================================================================
 * The UserNameIdentityToken structure.
 *===========================================================================*/
typedef struct _OpcUa_UserNameIdentityToken
{
    OpcUa_String     PolicyId;
    OpcUa_String     UserName;
    OpcUa_ByteString Password;
    OpcUa_String     EncryptionAlgorithm;
}
OpcUa_UserNameIdentityToken;

OPCUA_EXPORT OpcUa_Void OpcUa_UserNameIdentityToken_Initialize(OpcUa_UserNameIdentityToken* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_UserNameIdentityToken_Clear(OpcUa_UserNameIdentityToken* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_UserNameIdentityToken_GetSize(OpcUa_UserNameIdentityToken* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_UserNameIdentityToken_Encode(OpcUa_UserNameIdentityToken* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_UserNameIdentityToken_Decode(OpcUa_UserNameIdentityToken* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_UserNameIdentityToken_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_X509IdentityToken
/*============================================================================
 * The X509IdentityToken structure.
 *===========================================================================*/
typedef struct _OpcUa_X509IdentityToken
{
    OpcUa_String     PolicyId;
    OpcUa_ByteString CertificateData;
}
OpcUa_X509IdentityToken;

OPCUA_EXPORT OpcUa_Void OpcUa_X509IdentityToken_Initialize(OpcUa_X509IdentityToken* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_X509IdentityToken_Clear(OpcUa_X509IdentityToken* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_X509IdentityToken_GetSize(OpcUa_X509IdentityToken* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_X509IdentityToken_Encode(OpcUa_X509IdentityToken* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_X509IdentityToken_Decode(OpcUa_X509IdentityToken* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_X509IdentityToken_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_IssuedIdentityToken
/*============================================================================
 * The IssuedIdentityToken structure.
 *===========================================================================*/
typedef struct _OpcUa_IssuedIdentityToken
{
    OpcUa_String     PolicyId;
    OpcUa_ByteString TokenData;
    OpcUa_String     EncryptionAlgorithm;
}
OpcUa_IssuedIdentityToken;

OPCUA_EXPORT OpcUa_Void OpcUa_IssuedIdentityToken_Initialize(OpcUa_IssuedIdentityToken* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_IssuedIdentityToken_Clear(OpcUa_IssuedIdentityToken* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_IssuedIdentityToken_GetSize(OpcUa_IssuedIdentityToken* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_IssuedIdentityToken_Encode(OpcUa_IssuedIdentityToken* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_IssuedIdentityToken_Decode(OpcUa_IssuedIdentityToken* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_IssuedIdentityToken_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ActivateSession
#ifndef OPCUA_EXCLUDE_ActivateSessionRequest
/*============================================================================
 * The ActivateSessionRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_ActivateSessionRequest
{
    OpcUa_RequestHeader              RequestHeader;
    OpcUa_SignatureData              ClientSignature;
    OpcUa_Int32                      NoOfClientSoftwareCertificates;
    OpcUa_SignedSoftwareCertificate* ClientSoftwareCertificates;
    OpcUa_Int32                      NoOfLocaleIds;
    OpcUa_String*                    LocaleIds;
    OpcUa_ExtensionObject            UserIdentityToken;
    OpcUa_SignatureData              UserTokenSignature;
}
OpcUa_ActivateSessionRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_ActivateSessionRequest_Initialize(OpcUa_ActivateSessionRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ActivateSessionRequest_Clear(OpcUa_ActivateSessionRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ActivateSessionRequest_GetSize(OpcUa_ActivateSessionRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ActivateSessionRequest_Encode(OpcUa_ActivateSessionRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ActivateSessionRequest_Decode(OpcUa_ActivateSessionRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ActivateSessionRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ActivateSessionResponse
/*============================================================================
 * The ActivateSessionResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_ActivateSessionResponse
{
    OpcUa_ResponseHeader  ResponseHeader;
    OpcUa_ByteString      ServerNonce;
    OpcUa_Int32           NoOfResults;
    OpcUa_StatusCode*     Results;
    OpcUa_Int32           NoOfDiagnosticInfos;
    OpcUa_DiagnosticInfo* DiagnosticInfos;
}
OpcUa_ActivateSessionResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_ActivateSessionResponse_Initialize(OpcUa_ActivateSessionResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ActivateSessionResponse_Clear(OpcUa_ActivateSessionResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ActivateSessionResponse_GetSize(OpcUa_ActivateSessionResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ActivateSessionResponse_Encode(OpcUa_ActivateSessionResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ActivateSessionResponse_Decode(OpcUa_ActivateSessionResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ActivateSessionResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_CloseSession
#ifndef OPCUA_EXCLUDE_CloseSessionRequest
/*============================================================================
 * The CloseSessionRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_CloseSessionRequest
{
    OpcUa_RequestHeader RequestHeader;
    OpcUa_Boolean       DeleteSubscriptions;
}
OpcUa_CloseSessionRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_CloseSessionRequest_Initialize(OpcUa_CloseSessionRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_CloseSessionRequest_Clear(OpcUa_CloseSessionRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CloseSessionRequest_GetSize(OpcUa_CloseSessionRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CloseSessionRequest_Encode(OpcUa_CloseSessionRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CloseSessionRequest_Decode(OpcUa_CloseSessionRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_CloseSessionRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_CloseSessionResponse
/*============================================================================
 * The CloseSessionResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_CloseSessionResponse
{
    OpcUa_ResponseHeader ResponseHeader;
}
OpcUa_CloseSessionResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_CloseSessionResponse_Initialize(OpcUa_CloseSessionResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_CloseSessionResponse_Clear(OpcUa_CloseSessionResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CloseSessionResponse_GetSize(OpcUa_CloseSessionResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CloseSessionResponse_Encode(OpcUa_CloseSessionResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CloseSessionResponse_Decode(OpcUa_CloseSessionResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_CloseSessionResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_Cancel
#ifndef OPCUA_EXCLUDE_CancelRequest
/*============================================================================
 * The CancelRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_CancelRequest
{
    OpcUa_RequestHeader RequestHeader;
    OpcUa_UInt32        RequestHandle;
}
OpcUa_CancelRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_CancelRequest_Initialize(OpcUa_CancelRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_CancelRequest_Clear(OpcUa_CancelRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CancelRequest_GetSize(OpcUa_CancelRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CancelRequest_Encode(OpcUa_CancelRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CancelRequest_Decode(OpcUa_CancelRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_CancelRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_CancelResponse
/*============================================================================
 * The CancelResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_CancelResponse
{
    OpcUa_ResponseHeader ResponseHeader;
    OpcUa_UInt32         CancelCount;
}
OpcUa_CancelResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_CancelResponse_Initialize(OpcUa_CancelResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_CancelResponse_Clear(OpcUa_CancelResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CancelResponse_GetSize(OpcUa_CancelResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CancelResponse_Encode(OpcUa_CancelResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CancelResponse_Decode(OpcUa_CancelResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_CancelResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_NodeAttributesMask
/*============================================================================
 * The NodeAttributesMask enumeration.
 *===========================================================================*/
typedef enum _OpcUa_NodeAttributesMask
{
    OpcUa_NodeAttributesMask_None                    = 0,
    OpcUa_NodeAttributesMask_AccessLevel             = 1,
    OpcUa_NodeAttributesMask_ArrayDimensions         = 2,
    OpcUa_NodeAttributesMask_BrowseName              = 4,
    OpcUa_NodeAttributesMask_ContainsNoLoops         = 8,
    OpcUa_NodeAttributesMask_DataType                = 16,
    OpcUa_NodeAttributesMask_Description             = 32,
    OpcUa_NodeAttributesMask_DisplayName             = 64,
    OpcUa_NodeAttributesMask_EventNotifier           = 128,
    OpcUa_NodeAttributesMask_Executable              = 256,
    OpcUa_NodeAttributesMask_Historizing             = 512,
    OpcUa_NodeAttributesMask_InverseName             = 1024,
    OpcUa_NodeAttributesMask_IsAbstract              = 2048,
    OpcUa_NodeAttributesMask_MinimumSamplingInterval = 4096,
    OpcUa_NodeAttributesMask_NodeClass               = 8192,
    OpcUa_NodeAttributesMask_NodeId                  = 16384,
    OpcUa_NodeAttributesMask_Symmetric               = 32768,
    OpcUa_NodeAttributesMask_UserAccessLevel         = 65536,
    OpcUa_NodeAttributesMask_UserExecutable          = 131072,
    OpcUa_NodeAttributesMask_UserWriteMask           = 262144,
    OpcUa_NodeAttributesMask_ValueRank               = 524288,
    OpcUa_NodeAttributesMask_WriteMask               = 1048576,
    OpcUa_NodeAttributesMask_Value                   = 2097152,
    OpcUa_NodeAttributesMask_DataTypeDefinition      = 4194304,
    OpcUa_NodeAttributesMask_RolePermissions         = 8388608,
    OpcUa_NodeAttributesMask_AccessRestrictions      = 16777216,
    OpcUa_NodeAttributesMask_All                     = 33554431,
    OpcUa_NodeAttributesMask_BaseNode                = 26501220,
    OpcUa_NodeAttributesMask_Object                  = 26501348,
    OpcUa_NodeAttributesMask_ObjectType              = 26503268,
    OpcUa_NodeAttributesMask_Variable                = 26571383,
    OpcUa_NodeAttributesMask_VariableType            = 28600438,
    OpcUa_NodeAttributesMask_Method                  = 26632548,
    OpcUa_NodeAttributesMask_ReferenceType           = 26537060,
    OpcUa_NodeAttributesMask_View                    = 26501356
#if OPCUA_FORCE_INT32_ENUMS
    ,_OpcUa_NodeAttributesMask_MaxEnumerationValue = OpcUa_Int32_Max
#endif
}
OpcUa_NodeAttributesMask;

#define OpcUa_NodeAttributesMask_Clear(xValue) OpcUa_EnumeratedType_Clear(xValue, OpcUa_NodeAttributesMask_None)

#define OpcUa_NodeAttributesMask_Initialize(xValue) OpcUa_EnumeratedType_Initialize(xValue, OpcUa_NodeAttributesMask_None)

OPCUA_IMEXPORT extern struct _OpcUa_EnumeratedType OpcUa_NodeAttributesMask_EnumeratedType;
#endif

#ifndef OPCUA_EXCLUDE_NodeAttributes
/*============================================================================
 * The NodeAttributes structure.
 *===========================================================================*/
typedef struct _OpcUa_NodeAttributes
{
    OpcUa_UInt32        SpecifiedAttributes;
    OpcUa_LocalizedText DisplayName;
    OpcUa_LocalizedText Description;
    OpcUa_UInt32        WriteMask;
    OpcUa_UInt32        UserWriteMask;
}
OpcUa_NodeAttributes;

OPCUA_EXPORT OpcUa_Void OpcUa_NodeAttributes_Initialize(OpcUa_NodeAttributes* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_NodeAttributes_Clear(OpcUa_NodeAttributes* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_NodeAttributes_GetSize(OpcUa_NodeAttributes* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_NodeAttributes_Encode(OpcUa_NodeAttributes* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_NodeAttributes_Decode(OpcUa_NodeAttributes* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_NodeAttributes_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ObjectAttributes
/*============================================================================
 * The ObjectAttributes structure.
 *===========================================================================*/
typedef struct _OpcUa_ObjectAttributes
{
    OpcUa_UInt32        SpecifiedAttributes;
    OpcUa_LocalizedText DisplayName;
    OpcUa_LocalizedText Description;
    OpcUa_UInt32        WriteMask;
    OpcUa_UInt32        UserWriteMask;
    OpcUa_Byte          EventNotifier;
}
OpcUa_ObjectAttributes;

OPCUA_EXPORT OpcUa_Void OpcUa_ObjectAttributes_Initialize(OpcUa_ObjectAttributes* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ObjectAttributes_Clear(OpcUa_ObjectAttributes* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ObjectAttributes_GetSize(OpcUa_ObjectAttributes* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ObjectAttributes_Encode(OpcUa_ObjectAttributes* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ObjectAttributes_Decode(OpcUa_ObjectAttributes* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ObjectAttributes_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_VariableAttributes
/*============================================================================
 * The VariableAttributes structure.
 *===========================================================================*/
typedef struct _OpcUa_VariableAttributes
{
    OpcUa_UInt32        SpecifiedAttributes;
    OpcUa_LocalizedText DisplayName;
    OpcUa_LocalizedText Description;
    OpcUa_UInt32        WriteMask;
    OpcUa_UInt32        UserWriteMask;
    OpcUa_Variant       Value;
    OpcUa_NodeId        DataType;
    OpcUa_Int32         ValueRank;
    OpcUa_Int32         NoOfArrayDimensions;
    OpcUa_UInt32*       ArrayDimensions;
    OpcUa_Byte          AccessLevel;
    OpcUa_Byte          UserAccessLevel;
    OpcUa_Double        MinimumSamplingInterval;
    OpcUa_Boolean       Historizing;
}
OpcUa_VariableAttributes;

OPCUA_EXPORT OpcUa_Void OpcUa_VariableAttributes_Initialize(OpcUa_VariableAttributes* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_VariableAttributes_Clear(OpcUa_VariableAttributes* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_VariableAttributes_GetSize(OpcUa_VariableAttributes* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_VariableAttributes_Encode(OpcUa_VariableAttributes* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_VariableAttributes_Decode(OpcUa_VariableAttributes* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_VariableAttributes_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_MethodAttributes
/*============================================================================
 * The MethodAttributes structure.
 *===========================================================================*/
typedef struct _OpcUa_MethodAttributes
{
    OpcUa_UInt32        SpecifiedAttributes;
    OpcUa_LocalizedText DisplayName;
    OpcUa_LocalizedText Description;
    OpcUa_UInt32        WriteMask;
    OpcUa_UInt32        UserWriteMask;
    OpcUa_Boolean       Executable;
    OpcUa_Boolean       UserExecutable;
}
OpcUa_MethodAttributes;

OPCUA_EXPORT OpcUa_Void OpcUa_MethodAttributes_Initialize(OpcUa_MethodAttributes* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_MethodAttributes_Clear(OpcUa_MethodAttributes* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_MethodAttributes_GetSize(OpcUa_MethodAttributes* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_MethodAttributes_Encode(OpcUa_MethodAttributes* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_MethodAttributes_Decode(OpcUa_MethodAttributes* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_MethodAttributes_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ObjectTypeAttributes
/*============================================================================
 * The ObjectTypeAttributes structure.
 *===========================================================================*/
typedef struct _OpcUa_ObjectTypeAttributes
{
    OpcUa_UInt32        SpecifiedAttributes;
    OpcUa_LocalizedText DisplayName;
    OpcUa_LocalizedText Description;
    OpcUa_UInt32        WriteMask;
    OpcUa_UInt32        UserWriteMask;
    OpcUa_Boolean       IsAbstract;
}
OpcUa_ObjectTypeAttributes;

OPCUA_EXPORT OpcUa_Void OpcUa_ObjectTypeAttributes_Initialize(OpcUa_ObjectTypeAttributes* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ObjectTypeAttributes_Clear(OpcUa_ObjectTypeAttributes* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ObjectTypeAttributes_GetSize(OpcUa_ObjectTypeAttributes* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ObjectTypeAttributes_Encode(OpcUa_ObjectTypeAttributes* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ObjectTypeAttributes_Decode(OpcUa_ObjectTypeAttributes* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ObjectTypeAttributes_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_VariableTypeAttributes
/*============================================================================
 * The VariableTypeAttributes structure.
 *===========================================================================*/
typedef struct _OpcUa_VariableTypeAttributes
{
    OpcUa_UInt32        SpecifiedAttributes;
    OpcUa_LocalizedText DisplayName;
    OpcUa_LocalizedText Description;
    OpcUa_UInt32        WriteMask;
    OpcUa_UInt32        UserWriteMask;
    OpcUa_Variant       Value;
    OpcUa_NodeId        DataType;
    OpcUa_Int32         ValueRank;
    OpcUa_Int32         NoOfArrayDimensions;
    OpcUa_UInt32*       ArrayDimensions;
    OpcUa_Boolean       IsAbstract;
}
OpcUa_VariableTypeAttributes;

OPCUA_EXPORT OpcUa_Void OpcUa_VariableTypeAttributes_Initialize(OpcUa_VariableTypeAttributes* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_VariableTypeAttributes_Clear(OpcUa_VariableTypeAttributes* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_VariableTypeAttributes_GetSize(OpcUa_VariableTypeAttributes* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_VariableTypeAttributes_Encode(OpcUa_VariableTypeAttributes* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_VariableTypeAttributes_Decode(OpcUa_VariableTypeAttributes* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_VariableTypeAttributes_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ReferenceTypeAttributes
/*============================================================================
 * The ReferenceTypeAttributes structure.
 *===========================================================================*/
typedef struct _OpcUa_ReferenceTypeAttributes
{
    OpcUa_UInt32        SpecifiedAttributes;
    OpcUa_LocalizedText DisplayName;
    OpcUa_LocalizedText Description;
    OpcUa_UInt32        WriteMask;
    OpcUa_UInt32        UserWriteMask;
    OpcUa_Boolean       IsAbstract;
    OpcUa_Boolean       Symmetric;
    OpcUa_LocalizedText InverseName;
}
OpcUa_ReferenceTypeAttributes;

OPCUA_EXPORT OpcUa_Void OpcUa_ReferenceTypeAttributes_Initialize(OpcUa_ReferenceTypeAttributes* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ReferenceTypeAttributes_Clear(OpcUa_ReferenceTypeAttributes* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReferenceTypeAttributes_GetSize(OpcUa_ReferenceTypeAttributes* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReferenceTypeAttributes_Encode(OpcUa_ReferenceTypeAttributes* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReferenceTypeAttributes_Decode(OpcUa_ReferenceTypeAttributes* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ReferenceTypeAttributes_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_DataTypeAttributes
/*============================================================================
 * The DataTypeAttributes structure.
 *===========================================================================*/
typedef struct _OpcUa_DataTypeAttributes
{
    OpcUa_UInt32        SpecifiedAttributes;
    OpcUa_LocalizedText DisplayName;
    OpcUa_LocalizedText Description;
    OpcUa_UInt32        WriteMask;
    OpcUa_UInt32        UserWriteMask;
    OpcUa_Boolean       IsAbstract;
}
OpcUa_DataTypeAttributes;

OPCUA_EXPORT OpcUa_Void OpcUa_DataTypeAttributes_Initialize(OpcUa_DataTypeAttributes* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_DataTypeAttributes_Clear(OpcUa_DataTypeAttributes* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DataTypeAttributes_GetSize(OpcUa_DataTypeAttributes* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DataTypeAttributes_Encode(OpcUa_DataTypeAttributes* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DataTypeAttributes_Decode(OpcUa_DataTypeAttributes* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_DataTypeAttributes_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ViewAttributes
/*============================================================================
 * The ViewAttributes structure.
 *===========================================================================*/
typedef struct _OpcUa_ViewAttributes
{
    OpcUa_UInt32        SpecifiedAttributes;
    OpcUa_LocalizedText DisplayName;
    OpcUa_LocalizedText Description;
    OpcUa_UInt32        WriteMask;
    OpcUa_UInt32        UserWriteMask;
    OpcUa_Boolean       ContainsNoLoops;
    OpcUa_Byte          EventNotifier;
}
OpcUa_ViewAttributes;

OPCUA_EXPORT OpcUa_Void OpcUa_ViewAttributes_Initialize(OpcUa_ViewAttributes* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ViewAttributes_Clear(OpcUa_ViewAttributes* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ViewAttributes_GetSize(OpcUa_ViewAttributes* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ViewAttributes_Encode(OpcUa_ViewAttributes* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ViewAttributes_Decode(OpcUa_ViewAttributes* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ViewAttributes_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_GenericAttributeValue
/*============================================================================
 * The GenericAttributeValue structure.
 *===========================================================================*/
typedef struct _OpcUa_GenericAttributeValue
{
    OpcUa_UInt32  AttributeId;
    OpcUa_Variant Value;
}
OpcUa_GenericAttributeValue;

OPCUA_EXPORT OpcUa_Void OpcUa_GenericAttributeValue_Initialize(OpcUa_GenericAttributeValue* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_GenericAttributeValue_Clear(OpcUa_GenericAttributeValue* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_GenericAttributeValue_GetSize(OpcUa_GenericAttributeValue* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_GenericAttributeValue_Encode(OpcUa_GenericAttributeValue* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_GenericAttributeValue_Decode(OpcUa_GenericAttributeValue* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_GenericAttributeValue_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_GenericAttributes
/*============================================================================
 * The GenericAttributes structure.
 *===========================================================================*/
typedef struct _OpcUa_GenericAttributes
{
    OpcUa_UInt32                 SpecifiedAttributes;
    OpcUa_LocalizedText          DisplayName;
    OpcUa_LocalizedText          Description;
    OpcUa_UInt32                 WriteMask;
    OpcUa_UInt32                 UserWriteMask;
    OpcUa_Int32                  NoOfAttributeValues;
    OpcUa_GenericAttributeValue* AttributeValues;
}
OpcUa_GenericAttributes;

OPCUA_EXPORT OpcUa_Void OpcUa_GenericAttributes_Initialize(OpcUa_GenericAttributes* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_GenericAttributes_Clear(OpcUa_GenericAttributes* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_GenericAttributes_GetSize(OpcUa_GenericAttributes* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_GenericAttributes_Encode(OpcUa_GenericAttributes* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_GenericAttributes_Decode(OpcUa_GenericAttributes* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_GenericAttributes_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_AddNodesItem
/*============================================================================
 * The AddNodesItem structure.
 *===========================================================================*/
typedef struct _OpcUa_AddNodesItem
{
    OpcUa_ExpandedNodeId  ParentNodeId;
    OpcUa_NodeId          ReferenceTypeId;
    OpcUa_ExpandedNodeId  RequestedNewNodeId;
    OpcUa_QualifiedName   BrowseName;
    OpcUa_NodeClass       NodeClass;
    OpcUa_ExtensionObject NodeAttributes;
    OpcUa_ExpandedNodeId  TypeDefinition;
}
OpcUa_AddNodesItem;

OPCUA_EXPORT OpcUa_Void OpcUa_AddNodesItem_Initialize(OpcUa_AddNodesItem* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_AddNodesItem_Clear(OpcUa_AddNodesItem* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AddNodesItem_GetSize(OpcUa_AddNodesItem* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AddNodesItem_Encode(OpcUa_AddNodesItem* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AddNodesItem_Decode(OpcUa_AddNodesItem* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_AddNodesItem_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_AddNodesResult
/*============================================================================
 * The AddNodesResult structure.
 *===========================================================================*/
typedef struct _OpcUa_AddNodesResult
{
    OpcUa_StatusCode StatusCode;
    OpcUa_NodeId     AddedNodeId;
}
OpcUa_AddNodesResult;

OPCUA_EXPORT OpcUa_Void OpcUa_AddNodesResult_Initialize(OpcUa_AddNodesResult* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_AddNodesResult_Clear(OpcUa_AddNodesResult* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AddNodesResult_GetSize(OpcUa_AddNodesResult* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AddNodesResult_Encode(OpcUa_AddNodesResult* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AddNodesResult_Decode(OpcUa_AddNodesResult* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_AddNodesResult_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_AddNodes
#ifndef OPCUA_EXCLUDE_AddNodesRequest
/*============================================================================
 * The AddNodesRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_AddNodesRequest
{
    OpcUa_RequestHeader RequestHeader;
    OpcUa_Int32         NoOfNodesToAdd;
    OpcUa_AddNodesItem* NodesToAdd;
}
OpcUa_AddNodesRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_AddNodesRequest_Initialize(OpcUa_AddNodesRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_AddNodesRequest_Clear(OpcUa_AddNodesRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AddNodesRequest_GetSize(OpcUa_AddNodesRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AddNodesRequest_Encode(OpcUa_AddNodesRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AddNodesRequest_Decode(OpcUa_AddNodesRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_AddNodesRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_AddNodesResponse
/*============================================================================
 * The AddNodesResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_AddNodesResponse
{
    OpcUa_ResponseHeader  ResponseHeader;
    OpcUa_Int32           NoOfResults;
    OpcUa_AddNodesResult* Results;
    OpcUa_Int32           NoOfDiagnosticInfos;
    OpcUa_DiagnosticInfo* DiagnosticInfos;
}
OpcUa_AddNodesResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_AddNodesResponse_Initialize(OpcUa_AddNodesResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_AddNodesResponse_Clear(OpcUa_AddNodesResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AddNodesResponse_GetSize(OpcUa_AddNodesResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AddNodesResponse_Encode(OpcUa_AddNodesResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AddNodesResponse_Decode(OpcUa_AddNodesResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_AddNodesResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_AddReferencesItem
/*============================================================================
 * The AddReferencesItem structure.
 *===========================================================================*/
typedef struct _OpcUa_AddReferencesItem
{
    OpcUa_NodeId         SourceNodeId;
    OpcUa_NodeId         ReferenceTypeId;
    OpcUa_Boolean        IsForward;
    OpcUa_String         TargetServerUri;
    OpcUa_ExpandedNodeId TargetNodeId;
    OpcUa_NodeClass      TargetNodeClass;
}
OpcUa_AddReferencesItem;

OPCUA_EXPORT OpcUa_Void OpcUa_AddReferencesItem_Initialize(OpcUa_AddReferencesItem* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_AddReferencesItem_Clear(OpcUa_AddReferencesItem* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AddReferencesItem_GetSize(OpcUa_AddReferencesItem* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AddReferencesItem_Encode(OpcUa_AddReferencesItem* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AddReferencesItem_Decode(OpcUa_AddReferencesItem* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_AddReferencesItem_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_AddReferences
#ifndef OPCUA_EXCLUDE_AddReferencesRequest
/*============================================================================
 * The AddReferencesRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_AddReferencesRequest
{
    OpcUa_RequestHeader      RequestHeader;
    OpcUa_Int32              NoOfReferencesToAdd;
    OpcUa_AddReferencesItem* ReferencesToAdd;
}
OpcUa_AddReferencesRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_AddReferencesRequest_Initialize(OpcUa_AddReferencesRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_AddReferencesRequest_Clear(OpcUa_AddReferencesRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AddReferencesRequest_GetSize(OpcUa_AddReferencesRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AddReferencesRequest_Encode(OpcUa_AddReferencesRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AddReferencesRequest_Decode(OpcUa_AddReferencesRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_AddReferencesRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_AddReferencesResponse
/*============================================================================
 * The AddReferencesResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_AddReferencesResponse
{
    OpcUa_ResponseHeader  ResponseHeader;
    OpcUa_Int32           NoOfResults;
    OpcUa_StatusCode*     Results;
    OpcUa_Int32           NoOfDiagnosticInfos;
    OpcUa_DiagnosticInfo* DiagnosticInfos;
}
OpcUa_AddReferencesResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_AddReferencesResponse_Initialize(OpcUa_AddReferencesResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_AddReferencesResponse_Clear(OpcUa_AddReferencesResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AddReferencesResponse_GetSize(OpcUa_AddReferencesResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AddReferencesResponse_Encode(OpcUa_AddReferencesResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AddReferencesResponse_Decode(OpcUa_AddReferencesResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_AddReferencesResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_DeleteNodesItem
/*============================================================================
 * The DeleteNodesItem structure.
 *===========================================================================*/
typedef struct _OpcUa_DeleteNodesItem
{
    OpcUa_NodeId  NodeId;
    OpcUa_Boolean DeleteTargetReferences;
}
OpcUa_DeleteNodesItem;

OPCUA_EXPORT OpcUa_Void OpcUa_DeleteNodesItem_Initialize(OpcUa_DeleteNodesItem* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_DeleteNodesItem_Clear(OpcUa_DeleteNodesItem* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteNodesItem_GetSize(OpcUa_DeleteNodesItem* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteNodesItem_Encode(OpcUa_DeleteNodesItem* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteNodesItem_Decode(OpcUa_DeleteNodesItem* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_DeleteNodesItem_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_DeleteNodes
#ifndef OPCUA_EXCLUDE_DeleteNodesRequest
/*============================================================================
 * The DeleteNodesRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_DeleteNodesRequest
{
    OpcUa_RequestHeader    RequestHeader;
    OpcUa_Int32            NoOfNodesToDelete;
    OpcUa_DeleteNodesItem* NodesToDelete;
}
OpcUa_DeleteNodesRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_DeleteNodesRequest_Initialize(OpcUa_DeleteNodesRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_DeleteNodesRequest_Clear(OpcUa_DeleteNodesRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteNodesRequest_GetSize(OpcUa_DeleteNodesRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteNodesRequest_Encode(OpcUa_DeleteNodesRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteNodesRequest_Decode(OpcUa_DeleteNodesRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_DeleteNodesRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_DeleteNodesResponse
/*============================================================================
 * The DeleteNodesResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_DeleteNodesResponse
{
    OpcUa_ResponseHeader  ResponseHeader;
    OpcUa_Int32           NoOfResults;
    OpcUa_StatusCode*     Results;
    OpcUa_Int32           NoOfDiagnosticInfos;
    OpcUa_DiagnosticInfo* DiagnosticInfos;
}
OpcUa_DeleteNodesResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_DeleteNodesResponse_Initialize(OpcUa_DeleteNodesResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_DeleteNodesResponse_Clear(OpcUa_DeleteNodesResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteNodesResponse_GetSize(OpcUa_DeleteNodesResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteNodesResponse_Encode(OpcUa_DeleteNodesResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteNodesResponse_Decode(OpcUa_DeleteNodesResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_DeleteNodesResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_DeleteReferencesItem
/*============================================================================
 * The DeleteReferencesItem structure.
 *===========================================================================*/
typedef struct _OpcUa_DeleteReferencesItem
{
    OpcUa_NodeId         SourceNodeId;
    OpcUa_NodeId         ReferenceTypeId;
    OpcUa_Boolean        IsForward;
    OpcUa_ExpandedNodeId TargetNodeId;
    OpcUa_Boolean        DeleteBidirectional;
}
OpcUa_DeleteReferencesItem;

OPCUA_EXPORT OpcUa_Void OpcUa_DeleteReferencesItem_Initialize(OpcUa_DeleteReferencesItem* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_DeleteReferencesItem_Clear(OpcUa_DeleteReferencesItem* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteReferencesItem_GetSize(OpcUa_DeleteReferencesItem* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteReferencesItem_Encode(OpcUa_DeleteReferencesItem* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteReferencesItem_Decode(OpcUa_DeleteReferencesItem* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_DeleteReferencesItem_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_DeleteReferences
#ifndef OPCUA_EXCLUDE_DeleteReferencesRequest
/*============================================================================
 * The DeleteReferencesRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_DeleteReferencesRequest
{
    OpcUa_RequestHeader         RequestHeader;
    OpcUa_Int32                 NoOfReferencesToDelete;
    OpcUa_DeleteReferencesItem* ReferencesToDelete;
}
OpcUa_DeleteReferencesRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_DeleteReferencesRequest_Initialize(OpcUa_DeleteReferencesRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_DeleteReferencesRequest_Clear(OpcUa_DeleteReferencesRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteReferencesRequest_GetSize(OpcUa_DeleteReferencesRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteReferencesRequest_Encode(OpcUa_DeleteReferencesRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteReferencesRequest_Decode(OpcUa_DeleteReferencesRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_DeleteReferencesRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_DeleteReferencesResponse
/*============================================================================
 * The DeleteReferencesResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_DeleteReferencesResponse
{
    OpcUa_ResponseHeader  ResponseHeader;
    OpcUa_Int32           NoOfResults;
    OpcUa_StatusCode*     Results;
    OpcUa_Int32           NoOfDiagnosticInfos;
    OpcUa_DiagnosticInfo* DiagnosticInfos;
}
OpcUa_DeleteReferencesResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_DeleteReferencesResponse_Initialize(OpcUa_DeleteReferencesResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_DeleteReferencesResponse_Clear(OpcUa_DeleteReferencesResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteReferencesResponse_GetSize(OpcUa_DeleteReferencesResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteReferencesResponse_Encode(OpcUa_DeleteReferencesResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteReferencesResponse_Decode(OpcUa_DeleteReferencesResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_DeleteReferencesResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_AttributeWriteMask
/*============================================================================
 * The AttributeWriteMask enumeration.
 *===========================================================================*/
typedef enum _OpcUa_AttributeWriteMask
{
    OpcUa_AttributeWriteMask_None                    = 0,
    OpcUa_AttributeWriteMask_AccessLevel             = 1,
    OpcUa_AttributeWriteMask_ArrayDimensions         = 2,
    OpcUa_AttributeWriteMask_BrowseName              = 4,
    OpcUa_AttributeWriteMask_ContainsNoLoops         = 8,
    OpcUa_AttributeWriteMask_DataType                = 16,
    OpcUa_AttributeWriteMask_Description             = 32,
    OpcUa_AttributeWriteMask_DisplayName             = 64,
    OpcUa_AttributeWriteMask_EventNotifier           = 128,
    OpcUa_AttributeWriteMask_Executable              = 256,
    OpcUa_AttributeWriteMask_Historizing             = 512,
    OpcUa_AttributeWriteMask_InverseName             = 1024,
    OpcUa_AttributeWriteMask_IsAbstract              = 2048,
    OpcUa_AttributeWriteMask_MinimumSamplingInterval = 4096,
    OpcUa_AttributeWriteMask_NodeClass               = 8192,
    OpcUa_AttributeWriteMask_NodeId                  = 16384,
    OpcUa_AttributeWriteMask_Symmetric               = 32768,
    OpcUa_AttributeWriteMask_UserAccessLevel         = 65536,
    OpcUa_AttributeWriteMask_UserExecutable          = 131072,
    OpcUa_AttributeWriteMask_UserWriteMask           = 262144,
    OpcUa_AttributeWriteMask_ValueRank               = 524288,
    OpcUa_AttributeWriteMask_WriteMask               = 1048576,
    OpcUa_AttributeWriteMask_ValueForVariableType    = 2097152,
    OpcUa_AttributeWriteMask_DataTypeDefinition      = 4194304,
    OpcUa_AttributeWriteMask_RolePermissions         = 8388608,
    OpcUa_AttributeWriteMask_AccessRestrictions      = 16777216,
    OpcUa_AttributeWriteMask_AccessLevelEx           = 33554432
#if OPCUA_FORCE_INT32_ENUMS
    ,_OpcUa_AttributeWriteMask_MaxEnumerationValue = OpcUa_Int32_Max
#endif
}
OpcUa_AttributeWriteMask;

#define OpcUa_AttributeWriteMask_Clear(xValue) OpcUa_EnumeratedType_Clear(xValue, OpcUa_AttributeWriteMask_None)

#define OpcUa_AttributeWriteMask_Initialize(xValue) OpcUa_EnumeratedType_Initialize(xValue, OpcUa_AttributeWriteMask_None)

OPCUA_IMEXPORT extern struct _OpcUa_EnumeratedType OpcUa_AttributeWriteMask_EnumeratedType;
#endif

#ifndef OPCUA_EXCLUDE_BrowseDirection
/*============================================================================
 * The BrowseDirection enumeration.
 *===========================================================================*/
typedef enum _OpcUa_BrowseDirection
{
    OpcUa_BrowseDirection_Forward = 0,
    OpcUa_BrowseDirection_Inverse = 1,
    OpcUa_BrowseDirection_Both    = 2,
    OpcUa_BrowseDirection_Invalid = 3
#if OPCUA_FORCE_INT32_ENUMS
    ,_OpcUa_BrowseDirection_MaxEnumerationValue = OpcUa_Int32_Max
#endif
}
OpcUa_BrowseDirection;

#define OpcUa_BrowseDirection_Clear(xValue) OpcUa_EnumeratedType_Clear(xValue, OpcUa_BrowseDirection_Forward)

#define OpcUa_BrowseDirection_Initialize(xValue) OpcUa_EnumeratedType_Initialize(xValue, OpcUa_BrowseDirection_Forward)

OPCUA_IMEXPORT extern struct _OpcUa_EnumeratedType OpcUa_BrowseDirection_EnumeratedType;
#endif

#ifndef OPCUA_EXCLUDE_ViewDescription
/*============================================================================
 * The ViewDescription structure.
 *===========================================================================*/
typedef struct _OpcUa_ViewDescription
{
    OpcUa_NodeId   ViewId;
    OpcUa_DateTime Timestamp;
    OpcUa_UInt32   ViewVersion;
}
OpcUa_ViewDescription;

OPCUA_EXPORT OpcUa_Void OpcUa_ViewDescription_Initialize(OpcUa_ViewDescription* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ViewDescription_Clear(OpcUa_ViewDescription* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ViewDescription_GetSize(OpcUa_ViewDescription* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ViewDescription_Encode(OpcUa_ViewDescription* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ViewDescription_Decode(OpcUa_ViewDescription* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ViewDescription_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_BrowseDescription
/*============================================================================
 * The BrowseDescription structure.
 *===========================================================================*/
typedef struct _OpcUa_BrowseDescription
{
    OpcUa_NodeId          NodeId;
    OpcUa_BrowseDirection BrowseDirection;
    OpcUa_NodeId          ReferenceTypeId;
    OpcUa_Boolean         IncludeSubtypes;
    OpcUa_UInt32          NodeClassMask;
    OpcUa_UInt32          ResultMask;
}
OpcUa_BrowseDescription;

OPCUA_EXPORT OpcUa_Void OpcUa_BrowseDescription_Initialize(OpcUa_BrowseDescription* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_BrowseDescription_Clear(OpcUa_BrowseDescription* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_BrowseDescription_GetSize(OpcUa_BrowseDescription* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_BrowseDescription_Encode(OpcUa_BrowseDescription* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_BrowseDescription_Decode(OpcUa_BrowseDescription* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_BrowseDescription_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_BrowseResultMask
/*============================================================================
 * The BrowseResultMask enumeration.
 *===========================================================================*/
typedef enum _OpcUa_BrowseResultMask
{
    OpcUa_BrowseResultMask_None              = 0,
    OpcUa_BrowseResultMask_ReferenceTypeId   = 1,
    OpcUa_BrowseResultMask_IsForward         = 2,
    OpcUa_BrowseResultMask_NodeClass         = 4,
    OpcUa_BrowseResultMask_BrowseName        = 8,
    OpcUa_BrowseResultMask_DisplayName       = 16,
    OpcUa_BrowseResultMask_TypeDefinition    = 32,
    OpcUa_BrowseResultMask_All               = 63,
    OpcUa_BrowseResultMask_ReferenceTypeInfo = 3,
    OpcUa_BrowseResultMask_TargetInfo        = 60
#if OPCUA_FORCE_INT32_ENUMS
    ,_OpcUa_BrowseResultMask_MaxEnumerationValue = OpcUa_Int32_Max
#endif
}
OpcUa_BrowseResultMask;

#define OpcUa_BrowseResultMask_Clear(xValue) OpcUa_EnumeratedType_Clear(xValue, OpcUa_BrowseResultMask_None)

#define OpcUa_BrowseResultMask_Initialize(xValue) OpcUa_EnumeratedType_Initialize(xValue, OpcUa_BrowseResultMask_None)

OPCUA_IMEXPORT extern struct _OpcUa_EnumeratedType OpcUa_BrowseResultMask_EnumeratedType;
#endif

#ifndef OPCUA_EXCLUDE_ReferenceDescription
/*============================================================================
 * The ReferenceDescription structure.
 *===========================================================================*/
typedef struct _OpcUa_ReferenceDescription
{
    OpcUa_NodeId         ReferenceTypeId;
    OpcUa_Boolean        IsForward;
    OpcUa_ExpandedNodeId NodeId;
    OpcUa_QualifiedName  BrowseName;
    OpcUa_LocalizedText  DisplayName;
    OpcUa_NodeClass      NodeClass;
    OpcUa_ExpandedNodeId TypeDefinition;
}
OpcUa_ReferenceDescription;

OPCUA_EXPORT OpcUa_Void OpcUa_ReferenceDescription_Initialize(OpcUa_ReferenceDescription* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ReferenceDescription_Clear(OpcUa_ReferenceDescription* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReferenceDescription_GetSize(OpcUa_ReferenceDescription* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReferenceDescription_Encode(OpcUa_ReferenceDescription* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReferenceDescription_Decode(OpcUa_ReferenceDescription* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ReferenceDescription_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_BrowseResult
/*============================================================================
 * The BrowseResult structure.
 *===========================================================================*/
typedef struct _OpcUa_BrowseResult
{
    OpcUa_StatusCode            StatusCode;
    OpcUa_ByteString            ContinuationPoint;
    OpcUa_Int32                 NoOfReferences;
    OpcUa_ReferenceDescription* References;
}
OpcUa_BrowseResult;

OPCUA_EXPORT OpcUa_Void OpcUa_BrowseResult_Initialize(OpcUa_BrowseResult* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_BrowseResult_Clear(OpcUa_BrowseResult* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_BrowseResult_GetSize(OpcUa_BrowseResult* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_BrowseResult_Encode(OpcUa_BrowseResult* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_BrowseResult_Decode(OpcUa_BrowseResult* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_BrowseResult_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_Browse
#ifndef OPCUA_EXCLUDE_BrowseRequest
/*============================================================================
 * The BrowseRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_BrowseRequest
{
    OpcUa_RequestHeader      RequestHeader;
    OpcUa_ViewDescription    View;
    OpcUa_UInt32             RequestedMaxReferencesPerNode;
    OpcUa_Int32              NoOfNodesToBrowse;
    OpcUa_BrowseDescription* NodesToBrowse;
}
OpcUa_BrowseRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_BrowseRequest_Initialize(OpcUa_BrowseRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_BrowseRequest_Clear(OpcUa_BrowseRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_BrowseRequest_GetSize(OpcUa_BrowseRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_BrowseRequest_Encode(OpcUa_BrowseRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_BrowseRequest_Decode(OpcUa_BrowseRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_BrowseRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_BrowseResponse
/*============================================================================
 * The BrowseResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_BrowseResponse
{
    OpcUa_ResponseHeader  ResponseHeader;
    OpcUa_Int32           NoOfResults;
    OpcUa_BrowseResult*   Results;
    OpcUa_Int32           NoOfDiagnosticInfos;
    OpcUa_DiagnosticInfo* DiagnosticInfos;
}
OpcUa_BrowseResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_BrowseResponse_Initialize(OpcUa_BrowseResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_BrowseResponse_Clear(OpcUa_BrowseResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_BrowseResponse_GetSize(OpcUa_BrowseResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_BrowseResponse_Encode(OpcUa_BrowseResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_BrowseResponse_Decode(OpcUa_BrowseResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_BrowseResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_BrowseNext
#ifndef OPCUA_EXCLUDE_BrowseNextRequest
/*============================================================================
 * The BrowseNextRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_BrowseNextRequest
{
    OpcUa_RequestHeader RequestHeader;
    OpcUa_Boolean       ReleaseContinuationPoints;
    OpcUa_Int32         NoOfContinuationPoints;
    OpcUa_ByteString*   ContinuationPoints;
}
OpcUa_BrowseNextRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_BrowseNextRequest_Initialize(OpcUa_BrowseNextRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_BrowseNextRequest_Clear(OpcUa_BrowseNextRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_BrowseNextRequest_GetSize(OpcUa_BrowseNextRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_BrowseNextRequest_Encode(OpcUa_BrowseNextRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_BrowseNextRequest_Decode(OpcUa_BrowseNextRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_BrowseNextRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_BrowseNextResponse
/*============================================================================
 * The BrowseNextResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_BrowseNextResponse
{
    OpcUa_ResponseHeader  ResponseHeader;
    OpcUa_Int32           NoOfResults;
    OpcUa_BrowseResult*   Results;
    OpcUa_Int32           NoOfDiagnosticInfos;
    OpcUa_DiagnosticInfo* DiagnosticInfos;
}
OpcUa_BrowseNextResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_BrowseNextResponse_Initialize(OpcUa_BrowseNextResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_BrowseNextResponse_Clear(OpcUa_BrowseNextResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_BrowseNextResponse_GetSize(OpcUa_BrowseNextResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_BrowseNextResponse_Encode(OpcUa_BrowseNextResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_BrowseNextResponse_Decode(OpcUa_BrowseNextResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_BrowseNextResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_RelativePathElement
/*============================================================================
 * The RelativePathElement structure.
 *===========================================================================*/
typedef struct _OpcUa_RelativePathElement
{
    OpcUa_NodeId        ReferenceTypeId;
    OpcUa_Boolean       IsInverse;
    OpcUa_Boolean       IncludeSubtypes;
    OpcUa_QualifiedName TargetName;
}
OpcUa_RelativePathElement;

OPCUA_EXPORT OpcUa_Void OpcUa_RelativePathElement_Initialize(OpcUa_RelativePathElement* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_RelativePathElement_Clear(OpcUa_RelativePathElement* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RelativePathElement_GetSize(OpcUa_RelativePathElement* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RelativePathElement_Encode(OpcUa_RelativePathElement* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RelativePathElement_Decode(OpcUa_RelativePathElement* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_RelativePathElement_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_RelativePath
/*============================================================================
 * The RelativePath structure.
 *===========================================================================*/
typedef struct _OpcUa_RelativePath
{
    OpcUa_Int32                NoOfElements;
    OpcUa_RelativePathElement* Elements;
}
OpcUa_RelativePath;

OPCUA_EXPORT OpcUa_Void OpcUa_RelativePath_Initialize(OpcUa_RelativePath* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_RelativePath_Clear(OpcUa_RelativePath* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RelativePath_GetSize(OpcUa_RelativePath* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RelativePath_Encode(OpcUa_RelativePath* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RelativePath_Decode(OpcUa_RelativePath* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_RelativePath_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_BrowsePath
/*============================================================================
 * The BrowsePath structure.
 *===========================================================================*/
typedef struct _OpcUa_BrowsePath
{
    OpcUa_NodeId       StartingNode;
    OpcUa_RelativePath RelativePath;
}
OpcUa_BrowsePath;

OPCUA_EXPORT OpcUa_Void OpcUa_BrowsePath_Initialize(OpcUa_BrowsePath* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_BrowsePath_Clear(OpcUa_BrowsePath* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_BrowsePath_GetSize(OpcUa_BrowsePath* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_BrowsePath_Encode(OpcUa_BrowsePath* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_BrowsePath_Decode(OpcUa_BrowsePath* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_BrowsePath_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_BrowsePathTarget
/*============================================================================
 * The BrowsePathTarget structure.
 *===========================================================================*/
typedef struct _OpcUa_BrowsePathTarget
{
    OpcUa_ExpandedNodeId TargetId;
    OpcUa_UInt32         RemainingPathIndex;
}
OpcUa_BrowsePathTarget;

OPCUA_EXPORT OpcUa_Void OpcUa_BrowsePathTarget_Initialize(OpcUa_BrowsePathTarget* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_BrowsePathTarget_Clear(OpcUa_BrowsePathTarget* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_BrowsePathTarget_GetSize(OpcUa_BrowsePathTarget* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_BrowsePathTarget_Encode(OpcUa_BrowsePathTarget* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_BrowsePathTarget_Decode(OpcUa_BrowsePathTarget* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_BrowsePathTarget_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_BrowsePathResult
/*============================================================================
 * The BrowsePathResult structure.
 *===========================================================================*/
typedef struct _OpcUa_BrowsePathResult
{
    OpcUa_StatusCode        StatusCode;
    OpcUa_Int32             NoOfTargets;
    OpcUa_BrowsePathTarget* Targets;
}
OpcUa_BrowsePathResult;

OPCUA_EXPORT OpcUa_Void OpcUa_BrowsePathResult_Initialize(OpcUa_BrowsePathResult* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_BrowsePathResult_Clear(OpcUa_BrowsePathResult* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_BrowsePathResult_GetSize(OpcUa_BrowsePathResult* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_BrowsePathResult_Encode(OpcUa_BrowsePathResult* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_BrowsePathResult_Decode(OpcUa_BrowsePathResult* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_BrowsePathResult_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_TranslateBrowsePathsToNodeIds
#ifndef OPCUA_EXCLUDE_TranslateBrowsePathsToNodeIdsRequest
/*============================================================================
 * The TranslateBrowsePathsToNodeIdsRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_TranslateBrowsePathsToNodeIdsRequest
{
    OpcUa_RequestHeader RequestHeader;
    OpcUa_Int32         NoOfBrowsePaths;
    OpcUa_BrowsePath*   BrowsePaths;
}
OpcUa_TranslateBrowsePathsToNodeIdsRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_TranslateBrowsePathsToNodeIdsRequest_Initialize(OpcUa_TranslateBrowsePathsToNodeIdsRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_TranslateBrowsePathsToNodeIdsRequest_Clear(OpcUa_TranslateBrowsePathsToNodeIdsRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_TranslateBrowsePathsToNodeIdsRequest_GetSize(OpcUa_TranslateBrowsePathsToNodeIdsRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_TranslateBrowsePathsToNodeIdsRequest_Encode(OpcUa_TranslateBrowsePathsToNodeIdsRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_TranslateBrowsePathsToNodeIdsRequest_Decode(OpcUa_TranslateBrowsePathsToNodeIdsRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_TranslateBrowsePathsToNodeIdsRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_TranslateBrowsePathsToNodeIdsResponse
/*============================================================================
 * The TranslateBrowsePathsToNodeIdsResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_TranslateBrowsePathsToNodeIdsResponse
{
    OpcUa_ResponseHeader    ResponseHeader;
    OpcUa_Int32             NoOfResults;
    OpcUa_BrowsePathResult* Results;
    OpcUa_Int32             NoOfDiagnosticInfos;
    OpcUa_DiagnosticInfo*   DiagnosticInfos;
}
OpcUa_TranslateBrowsePathsToNodeIdsResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_TranslateBrowsePathsToNodeIdsResponse_Initialize(OpcUa_TranslateBrowsePathsToNodeIdsResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_TranslateBrowsePathsToNodeIdsResponse_Clear(OpcUa_TranslateBrowsePathsToNodeIdsResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_TranslateBrowsePathsToNodeIdsResponse_GetSize(OpcUa_TranslateBrowsePathsToNodeIdsResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_TranslateBrowsePathsToNodeIdsResponse_Encode(OpcUa_TranslateBrowsePathsToNodeIdsResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_TranslateBrowsePathsToNodeIdsResponse_Decode(OpcUa_TranslateBrowsePathsToNodeIdsResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_TranslateBrowsePathsToNodeIdsResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_RegisterNodes
#ifndef OPCUA_EXCLUDE_RegisterNodesRequest
/*============================================================================
 * The RegisterNodesRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_RegisterNodesRequest
{
    OpcUa_RequestHeader RequestHeader;
    OpcUa_Int32         NoOfNodesToRegister;
    OpcUa_NodeId*       NodesToRegister;
}
OpcUa_RegisterNodesRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_RegisterNodesRequest_Initialize(OpcUa_RegisterNodesRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_RegisterNodesRequest_Clear(OpcUa_RegisterNodesRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RegisterNodesRequest_GetSize(OpcUa_RegisterNodesRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RegisterNodesRequest_Encode(OpcUa_RegisterNodesRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RegisterNodesRequest_Decode(OpcUa_RegisterNodesRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_RegisterNodesRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_RegisterNodesResponse
/*============================================================================
 * The RegisterNodesResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_RegisterNodesResponse
{
    OpcUa_ResponseHeader ResponseHeader;
    OpcUa_Int32          NoOfRegisteredNodeIds;
    OpcUa_NodeId*        RegisteredNodeIds;
}
OpcUa_RegisterNodesResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_RegisterNodesResponse_Initialize(OpcUa_RegisterNodesResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_RegisterNodesResponse_Clear(OpcUa_RegisterNodesResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RegisterNodesResponse_GetSize(OpcUa_RegisterNodesResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RegisterNodesResponse_Encode(OpcUa_RegisterNodesResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RegisterNodesResponse_Decode(OpcUa_RegisterNodesResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_RegisterNodesResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_UnregisterNodes
#ifndef OPCUA_EXCLUDE_UnregisterNodesRequest
/*============================================================================
 * The UnregisterNodesRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_UnregisterNodesRequest
{
    OpcUa_RequestHeader RequestHeader;
    OpcUa_Int32         NoOfNodesToUnregister;
    OpcUa_NodeId*       NodesToUnregister;
}
OpcUa_UnregisterNodesRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_UnregisterNodesRequest_Initialize(OpcUa_UnregisterNodesRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_UnregisterNodesRequest_Clear(OpcUa_UnregisterNodesRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_UnregisterNodesRequest_GetSize(OpcUa_UnregisterNodesRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_UnregisterNodesRequest_Encode(OpcUa_UnregisterNodesRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_UnregisterNodesRequest_Decode(OpcUa_UnregisterNodesRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_UnregisterNodesRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_UnregisterNodesResponse
/*============================================================================
 * The UnregisterNodesResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_UnregisterNodesResponse
{
    OpcUa_ResponseHeader ResponseHeader;
}
OpcUa_UnregisterNodesResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_UnregisterNodesResponse_Initialize(OpcUa_UnregisterNodesResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_UnregisterNodesResponse_Clear(OpcUa_UnregisterNodesResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_UnregisterNodesResponse_GetSize(OpcUa_UnregisterNodesResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_UnregisterNodesResponse_Encode(OpcUa_UnregisterNodesResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_UnregisterNodesResponse_Decode(OpcUa_UnregisterNodesResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_UnregisterNodesResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_EndpointConfiguration
/*============================================================================
 * The EndpointConfiguration structure.
 *===========================================================================*/
typedef struct _OpcUa_EndpointConfiguration
{
    OpcUa_Int32   OperationTimeout;
    OpcUa_Boolean UseBinaryEncoding;
    OpcUa_Int32   MaxStringLength;
    OpcUa_Int32   MaxByteStringLength;
    OpcUa_Int32   MaxArrayLength;
    OpcUa_Int32   MaxMessageSize;
    OpcUa_Int32   MaxBufferSize;
    OpcUa_Int32   ChannelLifetime;
    OpcUa_Int32   SecurityTokenLifetime;
}
OpcUa_EndpointConfiguration;

OPCUA_EXPORT OpcUa_Void OpcUa_EndpointConfiguration_Initialize(OpcUa_EndpointConfiguration* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_EndpointConfiguration_Clear(OpcUa_EndpointConfiguration* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EndpointConfiguration_GetSize(OpcUa_EndpointConfiguration* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EndpointConfiguration_Encode(OpcUa_EndpointConfiguration* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EndpointConfiguration_Decode(OpcUa_EndpointConfiguration* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_EndpointConfiguration_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_QueryDataDescription
/*============================================================================
 * The QueryDataDescription structure.
 *===========================================================================*/
typedef struct _OpcUa_QueryDataDescription
{
    OpcUa_RelativePath RelativePath;
    OpcUa_UInt32       AttributeId;
    OpcUa_String       IndexRange;
}
OpcUa_QueryDataDescription;

OPCUA_EXPORT OpcUa_Void OpcUa_QueryDataDescription_Initialize(OpcUa_QueryDataDescription* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_QueryDataDescription_Clear(OpcUa_QueryDataDescription* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_QueryDataDescription_GetSize(OpcUa_QueryDataDescription* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_QueryDataDescription_Encode(OpcUa_QueryDataDescription* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_QueryDataDescription_Decode(OpcUa_QueryDataDescription* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_QueryDataDescription_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_NodeTypeDescription
/*============================================================================
 * The NodeTypeDescription structure.
 *===========================================================================*/
typedef struct _OpcUa_NodeTypeDescription
{
    OpcUa_ExpandedNodeId        TypeDefinitionNode;
    OpcUa_Boolean               IncludeSubTypes;
    OpcUa_Int32                 NoOfDataToReturn;
    OpcUa_QueryDataDescription* DataToReturn;
}
OpcUa_NodeTypeDescription;

OPCUA_EXPORT OpcUa_Void OpcUa_NodeTypeDescription_Initialize(OpcUa_NodeTypeDescription* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_NodeTypeDescription_Clear(OpcUa_NodeTypeDescription* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_NodeTypeDescription_GetSize(OpcUa_NodeTypeDescription* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_NodeTypeDescription_Encode(OpcUa_NodeTypeDescription* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_NodeTypeDescription_Decode(OpcUa_NodeTypeDescription* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_NodeTypeDescription_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_FilterOperator
/*============================================================================
 * The FilterOperator enumeration.
 *===========================================================================*/
typedef enum _OpcUa_FilterOperator
{
    OpcUa_FilterOperator_Equals             = 0,
    OpcUa_FilterOperator_IsNull             = 1,
    OpcUa_FilterOperator_GreaterThan        = 2,
    OpcUa_FilterOperator_LessThan           = 3,
    OpcUa_FilterOperator_GreaterThanOrEqual = 4,
    OpcUa_FilterOperator_LessThanOrEqual    = 5,
    OpcUa_FilterOperator_Like               = 6,
    OpcUa_FilterOperator_Not                = 7,
    OpcUa_FilterOperator_Between            = 8,
    OpcUa_FilterOperator_InList             = 9,
    OpcUa_FilterOperator_And                = 10,
    OpcUa_FilterOperator_Or                 = 11,
    OpcUa_FilterOperator_Cast               = 12,
    OpcUa_FilterOperator_InView             = 13,
    OpcUa_FilterOperator_OfType             = 14,
    OpcUa_FilterOperator_RelatedTo          = 15,
    OpcUa_FilterOperator_BitwiseAnd         = 16,
    OpcUa_FilterOperator_BitwiseOr          = 17
#if OPCUA_FORCE_INT32_ENUMS
    ,_OpcUa_FilterOperator_MaxEnumerationValue = OpcUa_Int32_Max
#endif
}
OpcUa_FilterOperator;

#define OpcUa_FilterOperator_Clear(xValue) OpcUa_EnumeratedType_Clear(xValue, OpcUa_FilterOperator_Equals)

#define OpcUa_FilterOperator_Initialize(xValue) OpcUa_EnumeratedType_Initialize(xValue, OpcUa_FilterOperator_Equals)

OPCUA_IMEXPORT extern struct _OpcUa_EnumeratedType OpcUa_FilterOperator_EnumeratedType;
#endif

#ifndef OPCUA_EXCLUDE_QueryDataSet
/*============================================================================
 * The QueryDataSet structure.
 *===========================================================================*/
typedef struct _OpcUa_QueryDataSet
{
    OpcUa_ExpandedNodeId NodeId;
    OpcUa_ExpandedNodeId TypeDefinitionNode;
    OpcUa_Int32          NoOfValues;
    OpcUa_Variant*       Values;
}
OpcUa_QueryDataSet;

OPCUA_EXPORT OpcUa_Void OpcUa_QueryDataSet_Initialize(OpcUa_QueryDataSet* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_QueryDataSet_Clear(OpcUa_QueryDataSet* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_QueryDataSet_GetSize(OpcUa_QueryDataSet* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_QueryDataSet_Encode(OpcUa_QueryDataSet* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_QueryDataSet_Decode(OpcUa_QueryDataSet* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_QueryDataSet_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_NodeReference
/*============================================================================
 * The NodeReference structure.
 *===========================================================================*/
typedef struct _OpcUa_NodeReference
{
    OpcUa_NodeId  NodeId;
    OpcUa_NodeId  ReferenceTypeId;
    OpcUa_Boolean IsForward;
    OpcUa_Int32   NoOfReferencedNodeIds;
    OpcUa_NodeId* ReferencedNodeIds;
}
OpcUa_NodeReference;

OPCUA_EXPORT OpcUa_Void OpcUa_NodeReference_Initialize(OpcUa_NodeReference* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_NodeReference_Clear(OpcUa_NodeReference* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_NodeReference_GetSize(OpcUa_NodeReference* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_NodeReference_Encode(OpcUa_NodeReference* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_NodeReference_Decode(OpcUa_NodeReference* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_NodeReference_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ContentFilterElement
/*============================================================================
 * The ContentFilterElement structure.
 *===========================================================================*/
typedef struct _OpcUa_ContentFilterElement
{
    OpcUa_FilterOperator   FilterOperator;
    OpcUa_Int32            NoOfFilterOperands;
    OpcUa_ExtensionObject* FilterOperands;
}
OpcUa_ContentFilterElement;

OPCUA_EXPORT OpcUa_Void OpcUa_ContentFilterElement_Initialize(OpcUa_ContentFilterElement* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ContentFilterElement_Clear(OpcUa_ContentFilterElement* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ContentFilterElement_GetSize(OpcUa_ContentFilterElement* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ContentFilterElement_Encode(OpcUa_ContentFilterElement* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ContentFilterElement_Decode(OpcUa_ContentFilterElement* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ContentFilterElement_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ContentFilter
/*============================================================================
 * The ContentFilter structure.
 *===========================================================================*/
typedef struct _OpcUa_ContentFilter
{
    OpcUa_Int32                 NoOfElements;
    OpcUa_ContentFilterElement* Elements;
}
OpcUa_ContentFilter;

OPCUA_EXPORT OpcUa_Void OpcUa_ContentFilter_Initialize(OpcUa_ContentFilter* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ContentFilter_Clear(OpcUa_ContentFilter* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ContentFilter_GetSize(OpcUa_ContentFilter* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ContentFilter_Encode(OpcUa_ContentFilter* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ContentFilter_Decode(OpcUa_ContentFilter* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ContentFilter_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ElementOperand
/*============================================================================
 * The ElementOperand structure.
 *===========================================================================*/
typedef struct _OpcUa_ElementOperand
{
    OpcUa_UInt32 Index;
}
OpcUa_ElementOperand;

OPCUA_EXPORT OpcUa_Void OpcUa_ElementOperand_Initialize(OpcUa_ElementOperand* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ElementOperand_Clear(OpcUa_ElementOperand* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ElementOperand_GetSize(OpcUa_ElementOperand* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ElementOperand_Encode(OpcUa_ElementOperand* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ElementOperand_Decode(OpcUa_ElementOperand* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ElementOperand_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_LiteralOperand
/*============================================================================
 * The LiteralOperand structure.
 *===========================================================================*/
typedef struct _OpcUa_LiteralOperand
{
    OpcUa_Variant Value;
}
OpcUa_LiteralOperand;

OPCUA_EXPORT OpcUa_Void OpcUa_LiteralOperand_Initialize(OpcUa_LiteralOperand* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_LiteralOperand_Clear(OpcUa_LiteralOperand* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_LiteralOperand_GetSize(OpcUa_LiteralOperand* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_LiteralOperand_Encode(OpcUa_LiteralOperand* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_LiteralOperand_Decode(OpcUa_LiteralOperand* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_LiteralOperand_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_AttributeOperand
/*============================================================================
 * The AttributeOperand structure.
 *===========================================================================*/
typedef struct _OpcUa_AttributeOperand
{
    OpcUa_NodeId       NodeId;
    OpcUa_String       Alias;
    OpcUa_RelativePath BrowsePath;
    OpcUa_UInt32       AttributeId;
    OpcUa_String       IndexRange;
}
OpcUa_AttributeOperand;

OPCUA_EXPORT OpcUa_Void OpcUa_AttributeOperand_Initialize(OpcUa_AttributeOperand* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_AttributeOperand_Clear(OpcUa_AttributeOperand* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AttributeOperand_GetSize(OpcUa_AttributeOperand* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AttributeOperand_Encode(OpcUa_AttributeOperand* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AttributeOperand_Decode(OpcUa_AttributeOperand* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_AttributeOperand_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_SimpleAttributeOperand
/*============================================================================
 * The SimpleAttributeOperand structure.
 *===========================================================================*/
typedef struct _OpcUa_SimpleAttributeOperand
{
    OpcUa_NodeId         TypeDefinitionId;
    OpcUa_Int32          NoOfBrowsePath;
    OpcUa_QualifiedName* BrowsePath;
    OpcUa_UInt32         AttributeId;
    OpcUa_String         IndexRange;
}
OpcUa_SimpleAttributeOperand;

OPCUA_EXPORT OpcUa_Void OpcUa_SimpleAttributeOperand_Initialize(OpcUa_SimpleAttributeOperand* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_SimpleAttributeOperand_Clear(OpcUa_SimpleAttributeOperand* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SimpleAttributeOperand_GetSize(OpcUa_SimpleAttributeOperand* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SimpleAttributeOperand_Encode(OpcUa_SimpleAttributeOperand* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SimpleAttributeOperand_Decode(OpcUa_SimpleAttributeOperand* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_SimpleAttributeOperand_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ContentFilterElementResult
/*============================================================================
 * The ContentFilterElementResult structure.
 *===========================================================================*/
typedef struct _OpcUa_ContentFilterElementResult
{
    OpcUa_StatusCode      StatusCode;
    OpcUa_Int32           NoOfOperandStatusCodes;
    OpcUa_StatusCode*     OperandStatusCodes;
    OpcUa_Int32           NoOfOperandDiagnosticInfos;
    OpcUa_DiagnosticInfo* OperandDiagnosticInfos;
}
OpcUa_ContentFilterElementResult;

OPCUA_EXPORT OpcUa_Void OpcUa_ContentFilterElementResult_Initialize(OpcUa_ContentFilterElementResult* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ContentFilterElementResult_Clear(OpcUa_ContentFilterElementResult* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ContentFilterElementResult_GetSize(OpcUa_ContentFilterElementResult* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ContentFilterElementResult_Encode(OpcUa_ContentFilterElementResult* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ContentFilterElementResult_Decode(OpcUa_ContentFilterElementResult* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ContentFilterElementResult_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ContentFilterResult
/*============================================================================
 * The ContentFilterResult structure.
 *===========================================================================*/
typedef struct _OpcUa_ContentFilterResult
{
    OpcUa_Int32                       NoOfElementResults;
    OpcUa_ContentFilterElementResult* ElementResults;
    OpcUa_Int32                       NoOfElementDiagnosticInfos;
    OpcUa_DiagnosticInfo*             ElementDiagnosticInfos;
}
OpcUa_ContentFilterResult;

OPCUA_EXPORT OpcUa_Void OpcUa_ContentFilterResult_Initialize(OpcUa_ContentFilterResult* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ContentFilterResult_Clear(OpcUa_ContentFilterResult* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ContentFilterResult_GetSize(OpcUa_ContentFilterResult* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ContentFilterResult_Encode(OpcUa_ContentFilterResult* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ContentFilterResult_Decode(OpcUa_ContentFilterResult* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ContentFilterResult_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ParsingResult
/*============================================================================
 * The ParsingResult structure.
 *===========================================================================*/
typedef struct _OpcUa_ParsingResult
{
    OpcUa_StatusCode      StatusCode;
    OpcUa_Int32           NoOfDataStatusCodes;
    OpcUa_StatusCode*     DataStatusCodes;
    OpcUa_Int32           NoOfDataDiagnosticInfos;
    OpcUa_DiagnosticInfo* DataDiagnosticInfos;
}
OpcUa_ParsingResult;

OPCUA_EXPORT OpcUa_Void OpcUa_ParsingResult_Initialize(OpcUa_ParsingResult* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ParsingResult_Clear(OpcUa_ParsingResult* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ParsingResult_GetSize(OpcUa_ParsingResult* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ParsingResult_Encode(OpcUa_ParsingResult* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ParsingResult_Decode(OpcUa_ParsingResult* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ParsingResult_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_QueryFirst
#ifndef OPCUA_EXCLUDE_QueryFirstRequest
/*============================================================================
 * The QueryFirstRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_QueryFirstRequest
{
    OpcUa_RequestHeader        RequestHeader;
    OpcUa_ViewDescription      View;
    OpcUa_Int32                NoOfNodeTypes;
    OpcUa_NodeTypeDescription* NodeTypes;
    OpcUa_ContentFilter        Filter;
    OpcUa_UInt32               MaxDataSetsToReturn;
    OpcUa_UInt32               MaxReferencesToReturn;
}
OpcUa_QueryFirstRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_QueryFirstRequest_Initialize(OpcUa_QueryFirstRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_QueryFirstRequest_Clear(OpcUa_QueryFirstRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_QueryFirstRequest_GetSize(OpcUa_QueryFirstRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_QueryFirstRequest_Encode(OpcUa_QueryFirstRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_QueryFirstRequest_Decode(OpcUa_QueryFirstRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_QueryFirstRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_QueryFirstResponse
/*============================================================================
 * The QueryFirstResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_QueryFirstResponse
{
    OpcUa_ResponseHeader      ResponseHeader;
    OpcUa_Int32               NoOfQueryDataSets;
    OpcUa_QueryDataSet*       QueryDataSets;
    OpcUa_ByteString          ContinuationPoint;
    OpcUa_Int32               NoOfParsingResults;
    OpcUa_ParsingResult*      ParsingResults;
    OpcUa_Int32               NoOfDiagnosticInfos;
    OpcUa_DiagnosticInfo*     DiagnosticInfos;
    OpcUa_ContentFilterResult FilterResult;
}
OpcUa_QueryFirstResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_QueryFirstResponse_Initialize(OpcUa_QueryFirstResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_QueryFirstResponse_Clear(OpcUa_QueryFirstResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_QueryFirstResponse_GetSize(OpcUa_QueryFirstResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_QueryFirstResponse_Encode(OpcUa_QueryFirstResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_QueryFirstResponse_Decode(OpcUa_QueryFirstResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_QueryFirstResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_QueryNext
#ifndef OPCUA_EXCLUDE_QueryNextRequest
/*============================================================================
 * The QueryNextRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_QueryNextRequest
{
    OpcUa_RequestHeader RequestHeader;
    OpcUa_Boolean       ReleaseContinuationPoint;
    OpcUa_ByteString    ContinuationPoint;
}
OpcUa_QueryNextRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_QueryNextRequest_Initialize(OpcUa_QueryNextRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_QueryNextRequest_Clear(OpcUa_QueryNextRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_QueryNextRequest_GetSize(OpcUa_QueryNextRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_QueryNextRequest_Encode(OpcUa_QueryNextRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_QueryNextRequest_Decode(OpcUa_QueryNextRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_QueryNextRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_QueryNextResponse
/*============================================================================
 * The QueryNextResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_QueryNextResponse
{
    OpcUa_ResponseHeader ResponseHeader;
    OpcUa_Int32          NoOfQueryDataSets;
    OpcUa_QueryDataSet*  QueryDataSets;
    OpcUa_ByteString     RevisedContinuationPoint;
}
OpcUa_QueryNextResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_QueryNextResponse_Initialize(OpcUa_QueryNextResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_QueryNextResponse_Clear(OpcUa_QueryNextResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_QueryNextResponse_GetSize(OpcUa_QueryNextResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_QueryNextResponse_Encode(OpcUa_QueryNextResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_QueryNextResponse_Decode(OpcUa_QueryNextResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_QueryNextResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_TimestampsToReturn
/*============================================================================
 * The TimestampsToReturn enumeration.
 *===========================================================================*/
typedef enum _OpcUa_TimestampsToReturn
{
    OpcUa_TimestampsToReturn_Source  = 0,
    OpcUa_TimestampsToReturn_Server  = 1,
    OpcUa_TimestampsToReturn_Both    = 2,
    OpcUa_TimestampsToReturn_Neither = 3,
    OpcUa_TimestampsToReturn_Invalid = 4
#if OPCUA_FORCE_INT32_ENUMS
    ,_OpcUa_TimestampsToReturn_MaxEnumerationValue = OpcUa_Int32_Max
#endif
}
OpcUa_TimestampsToReturn;

#define OpcUa_TimestampsToReturn_Clear(xValue) OpcUa_EnumeratedType_Clear(xValue, OpcUa_TimestampsToReturn_Source)

#define OpcUa_TimestampsToReturn_Initialize(xValue) OpcUa_EnumeratedType_Initialize(xValue, OpcUa_TimestampsToReturn_Source)

OPCUA_IMEXPORT extern struct _OpcUa_EnumeratedType OpcUa_TimestampsToReturn_EnumeratedType;
#endif

#ifndef OPCUA_EXCLUDE_ReadValueId
/*============================================================================
 * The ReadValueId structure.
 *===========================================================================*/
typedef struct _OpcUa_ReadValueId
{
    OpcUa_NodeId        NodeId;
    OpcUa_UInt32        AttributeId;
    OpcUa_String        IndexRange;
    OpcUa_QualifiedName DataEncoding;
}
OpcUa_ReadValueId;

OPCUA_EXPORT OpcUa_Void OpcUa_ReadValueId_Initialize(OpcUa_ReadValueId* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ReadValueId_Clear(OpcUa_ReadValueId* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReadValueId_GetSize(OpcUa_ReadValueId* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReadValueId_Encode(OpcUa_ReadValueId* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReadValueId_Decode(OpcUa_ReadValueId* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ReadValueId_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_Read
#ifndef OPCUA_EXCLUDE_ReadRequest
/*============================================================================
 * The ReadRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_ReadRequest
{
    OpcUa_RequestHeader      RequestHeader;
    OpcUa_Double             MaxAge;
    OpcUa_TimestampsToReturn TimestampsToReturn;
    OpcUa_Int32              NoOfNodesToRead;
    OpcUa_ReadValueId*       NodesToRead;
}
OpcUa_ReadRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_ReadRequest_Initialize(OpcUa_ReadRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ReadRequest_Clear(OpcUa_ReadRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReadRequest_GetSize(OpcUa_ReadRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReadRequest_Encode(OpcUa_ReadRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReadRequest_Decode(OpcUa_ReadRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ReadRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ReadResponse
/*============================================================================
 * The ReadResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_ReadResponse
{
    OpcUa_ResponseHeader  ResponseHeader;
    OpcUa_Int32           NoOfResults;
    OpcUa_DataValue*      Results;
    OpcUa_Int32           NoOfDiagnosticInfos;
    OpcUa_DiagnosticInfo* DiagnosticInfos;
}
OpcUa_ReadResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_ReadResponse_Initialize(OpcUa_ReadResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ReadResponse_Clear(OpcUa_ReadResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReadResponse_GetSize(OpcUa_ReadResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReadResponse_Encode(OpcUa_ReadResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReadResponse_Decode(OpcUa_ReadResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ReadResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_HistoryReadValueId
/*============================================================================
 * The HistoryReadValueId structure.
 *===========================================================================*/
typedef struct _OpcUa_HistoryReadValueId
{
    OpcUa_NodeId        NodeId;
    OpcUa_String        IndexRange;
    OpcUa_QualifiedName DataEncoding;
    OpcUa_ByteString    ContinuationPoint;
}
OpcUa_HistoryReadValueId;

OPCUA_EXPORT OpcUa_Void OpcUa_HistoryReadValueId_Initialize(OpcUa_HistoryReadValueId* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_HistoryReadValueId_Clear(OpcUa_HistoryReadValueId* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryReadValueId_GetSize(OpcUa_HistoryReadValueId* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryReadValueId_Encode(OpcUa_HistoryReadValueId* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryReadValueId_Decode(OpcUa_HistoryReadValueId* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_HistoryReadValueId_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_HistoryReadResult
/*============================================================================
 * The HistoryReadResult structure.
 *===========================================================================*/
typedef struct _OpcUa_HistoryReadResult
{
    OpcUa_StatusCode      StatusCode;
    OpcUa_ByteString      ContinuationPoint;
    OpcUa_ExtensionObject HistoryData;
}
OpcUa_HistoryReadResult;

OPCUA_EXPORT OpcUa_Void OpcUa_HistoryReadResult_Initialize(OpcUa_HistoryReadResult* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_HistoryReadResult_Clear(OpcUa_HistoryReadResult* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryReadResult_GetSize(OpcUa_HistoryReadResult* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryReadResult_Encode(OpcUa_HistoryReadResult* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryReadResult_Decode(OpcUa_HistoryReadResult* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_HistoryReadResult_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_EventFilter
/*============================================================================
 * The EventFilter structure.
 *===========================================================================*/
typedef struct _OpcUa_EventFilter
{
    OpcUa_Int32                   NoOfSelectClauses;
    OpcUa_SimpleAttributeOperand* SelectClauses;
    OpcUa_ContentFilter           WhereClause;
}
OpcUa_EventFilter;

OPCUA_EXPORT OpcUa_Void OpcUa_EventFilter_Initialize(OpcUa_EventFilter* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_EventFilter_Clear(OpcUa_EventFilter* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EventFilter_GetSize(OpcUa_EventFilter* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EventFilter_Encode(OpcUa_EventFilter* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EventFilter_Decode(OpcUa_EventFilter* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_EventFilter_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ReadEventDetails
/*============================================================================
 * The ReadEventDetails structure.
 *===========================================================================*/
typedef struct _OpcUa_ReadEventDetails
{
    OpcUa_UInt32      NumValuesPerNode;
    OpcUa_DateTime    StartTime;
    OpcUa_DateTime    EndTime;
    OpcUa_EventFilter Filter;
}
OpcUa_ReadEventDetails;

OPCUA_EXPORT OpcUa_Void OpcUa_ReadEventDetails_Initialize(OpcUa_ReadEventDetails* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ReadEventDetails_Clear(OpcUa_ReadEventDetails* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReadEventDetails_GetSize(OpcUa_ReadEventDetails* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReadEventDetails_Encode(OpcUa_ReadEventDetails* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReadEventDetails_Decode(OpcUa_ReadEventDetails* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ReadEventDetails_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ReadRawModifiedDetails
/*============================================================================
 * The ReadRawModifiedDetails structure.
 *===========================================================================*/
typedef struct _OpcUa_ReadRawModifiedDetails
{
    OpcUa_Boolean  IsReadModified;
    OpcUa_DateTime StartTime;
    OpcUa_DateTime EndTime;
    OpcUa_UInt32   NumValuesPerNode;
    OpcUa_Boolean  ReturnBounds;
}
OpcUa_ReadRawModifiedDetails;

OPCUA_EXPORT OpcUa_Void OpcUa_ReadRawModifiedDetails_Initialize(OpcUa_ReadRawModifiedDetails* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ReadRawModifiedDetails_Clear(OpcUa_ReadRawModifiedDetails* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReadRawModifiedDetails_GetSize(OpcUa_ReadRawModifiedDetails* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReadRawModifiedDetails_Encode(OpcUa_ReadRawModifiedDetails* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReadRawModifiedDetails_Decode(OpcUa_ReadRawModifiedDetails* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ReadRawModifiedDetails_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_AggregateConfiguration
/*============================================================================
 * The AggregateConfiguration structure.
 *===========================================================================*/
typedef struct _OpcUa_AggregateConfiguration
{
    OpcUa_Boolean UseServerCapabilitiesDefaults;
    OpcUa_Boolean TreatUncertainAsBad;
    OpcUa_Byte    PercentDataBad;
    OpcUa_Byte    PercentDataGood;
    OpcUa_Boolean UseSlopedExtrapolation;
}
OpcUa_AggregateConfiguration;

OPCUA_EXPORT OpcUa_Void OpcUa_AggregateConfiguration_Initialize(OpcUa_AggregateConfiguration* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_AggregateConfiguration_Clear(OpcUa_AggregateConfiguration* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AggregateConfiguration_GetSize(OpcUa_AggregateConfiguration* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AggregateConfiguration_Encode(OpcUa_AggregateConfiguration* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AggregateConfiguration_Decode(OpcUa_AggregateConfiguration* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_AggregateConfiguration_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ReadProcessedDetails
/*============================================================================
 * The ReadProcessedDetails structure.
 *===========================================================================*/
typedef struct _OpcUa_ReadProcessedDetails
{
    OpcUa_DateTime               StartTime;
    OpcUa_DateTime               EndTime;
    OpcUa_Double                 ProcessingInterval;
    OpcUa_Int32                  NoOfAggregateType;
    OpcUa_NodeId*                AggregateType;
    OpcUa_AggregateConfiguration AggregateConfiguration;
}
OpcUa_ReadProcessedDetails;

OPCUA_EXPORT OpcUa_Void OpcUa_ReadProcessedDetails_Initialize(OpcUa_ReadProcessedDetails* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ReadProcessedDetails_Clear(OpcUa_ReadProcessedDetails* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReadProcessedDetails_GetSize(OpcUa_ReadProcessedDetails* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReadProcessedDetails_Encode(OpcUa_ReadProcessedDetails* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReadProcessedDetails_Decode(OpcUa_ReadProcessedDetails* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ReadProcessedDetails_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ReadAtTimeDetails
/*============================================================================
 * The ReadAtTimeDetails structure.
 *===========================================================================*/
typedef struct _OpcUa_ReadAtTimeDetails
{
    OpcUa_Int32     NoOfReqTimes;
    OpcUa_DateTime* ReqTimes;
    OpcUa_Boolean   UseSimpleBounds;
}
OpcUa_ReadAtTimeDetails;

OPCUA_EXPORT OpcUa_Void OpcUa_ReadAtTimeDetails_Initialize(OpcUa_ReadAtTimeDetails* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ReadAtTimeDetails_Clear(OpcUa_ReadAtTimeDetails* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReadAtTimeDetails_GetSize(OpcUa_ReadAtTimeDetails* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReadAtTimeDetails_Encode(OpcUa_ReadAtTimeDetails* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReadAtTimeDetails_Decode(OpcUa_ReadAtTimeDetails* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ReadAtTimeDetails_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ReadAnnotationDataDetails
/*============================================================================
 * The ReadAnnotationDataDetails structure.
 *===========================================================================*/
typedef struct _OpcUa_ReadAnnotationDataDetails
{
    OpcUa_Int32     NoOfReqTimes;
    OpcUa_DateTime* ReqTimes;
}
OpcUa_ReadAnnotationDataDetails;

OPCUA_EXPORT OpcUa_Void OpcUa_ReadAnnotationDataDetails_Initialize(OpcUa_ReadAnnotationDataDetails* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ReadAnnotationDataDetails_Clear(OpcUa_ReadAnnotationDataDetails* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReadAnnotationDataDetails_GetSize(OpcUa_ReadAnnotationDataDetails* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReadAnnotationDataDetails_Encode(OpcUa_ReadAnnotationDataDetails* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ReadAnnotationDataDetails_Decode(OpcUa_ReadAnnotationDataDetails* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ReadAnnotationDataDetails_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_HistoryData
/*============================================================================
 * The HistoryData structure.
 *===========================================================================*/
typedef struct _OpcUa_HistoryData
{
    OpcUa_Int32      NoOfDataValues;
    OpcUa_DataValue* DataValues;
}
OpcUa_HistoryData;

OPCUA_EXPORT OpcUa_Void OpcUa_HistoryData_Initialize(OpcUa_HistoryData* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_HistoryData_Clear(OpcUa_HistoryData* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryData_GetSize(OpcUa_HistoryData* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryData_Encode(OpcUa_HistoryData* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryData_Decode(OpcUa_HistoryData* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_HistoryData_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_HistoryUpdateType
/*============================================================================
 * The HistoryUpdateType enumeration.
 *===========================================================================*/
typedef enum _OpcUa_HistoryUpdateType
{
    OpcUa_HistoryUpdateType_Insert  = 1,
    OpcUa_HistoryUpdateType_Replace = 2,
    OpcUa_HistoryUpdateType_Update  = 3,
    OpcUa_HistoryUpdateType_Delete  = 4
#if OPCUA_FORCE_INT32_ENUMS
    ,_OpcUa_HistoryUpdateType_MaxEnumerationValue = OpcUa_Int32_Max
#endif
}
OpcUa_HistoryUpdateType;

#define OpcUa_HistoryUpdateType_Clear(xValue) OpcUa_EnumeratedType_Clear(xValue, OpcUa_HistoryUpdateType_Insert)

#define OpcUa_HistoryUpdateType_Initialize(xValue) OpcUa_EnumeratedType_Initialize(xValue, OpcUa_HistoryUpdateType_Insert)

OPCUA_IMEXPORT extern struct _OpcUa_EnumeratedType OpcUa_HistoryUpdateType_EnumeratedType;
#endif

#ifndef OPCUA_EXCLUDE_ModificationInfo
/*============================================================================
 * The ModificationInfo structure.
 *===========================================================================*/
typedef struct _OpcUa_ModificationInfo
{
    OpcUa_DateTime          ModificationTime;
    OpcUa_HistoryUpdateType UpdateType;
    OpcUa_String            UserName;
}
OpcUa_ModificationInfo;

OPCUA_EXPORT OpcUa_Void OpcUa_ModificationInfo_Initialize(OpcUa_ModificationInfo* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ModificationInfo_Clear(OpcUa_ModificationInfo* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ModificationInfo_GetSize(OpcUa_ModificationInfo* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ModificationInfo_Encode(OpcUa_ModificationInfo* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ModificationInfo_Decode(OpcUa_ModificationInfo* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ModificationInfo_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_HistoryModifiedData
/*============================================================================
 * The HistoryModifiedData structure.
 *===========================================================================*/
typedef struct _OpcUa_HistoryModifiedData
{
    OpcUa_Int32             NoOfDataValues;
    OpcUa_DataValue*        DataValues;
    OpcUa_Int32             NoOfModificationInfos;
    OpcUa_ModificationInfo* ModificationInfos;
}
OpcUa_HistoryModifiedData;

OPCUA_EXPORT OpcUa_Void OpcUa_HistoryModifiedData_Initialize(OpcUa_HistoryModifiedData* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_HistoryModifiedData_Clear(OpcUa_HistoryModifiedData* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryModifiedData_GetSize(OpcUa_HistoryModifiedData* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryModifiedData_Encode(OpcUa_HistoryModifiedData* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryModifiedData_Decode(OpcUa_HistoryModifiedData* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_HistoryModifiedData_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_HistoryEventFieldList
/*============================================================================
 * The HistoryEventFieldList structure.
 *===========================================================================*/
typedef struct _OpcUa_HistoryEventFieldList
{
    OpcUa_Int32    NoOfEventFields;
    OpcUa_Variant* EventFields;
}
OpcUa_HistoryEventFieldList;

OPCUA_EXPORT OpcUa_Void OpcUa_HistoryEventFieldList_Initialize(OpcUa_HistoryEventFieldList* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_HistoryEventFieldList_Clear(OpcUa_HistoryEventFieldList* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryEventFieldList_GetSize(OpcUa_HistoryEventFieldList* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryEventFieldList_Encode(OpcUa_HistoryEventFieldList* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryEventFieldList_Decode(OpcUa_HistoryEventFieldList* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_HistoryEventFieldList_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_HistoryEvent
/*============================================================================
 * The HistoryEvent structure.
 *===========================================================================*/
typedef struct _OpcUa_HistoryEvent
{
    OpcUa_Int32                  NoOfEvents;
    OpcUa_HistoryEventFieldList* Events;
}
OpcUa_HistoryEvent;

OPCUA_EXPORT OpcUa_Void OpcUa_HistoryEvent_Initialize(OpcUa_HistoryEvent* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_HistoryEvent_Clear(OpcUa_HistoryEvent* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryEvent_GetSize(OpcUa_HistoryEvent* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryEvent_Encode(OpcUa_HistoryEvent* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryEvent_Decode(OpcUa_HistoryEvent* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_HistoryEvent_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_HistoryRead
#ifndef OPCUA_EXCLUDE_HistoryReadRequest
/*============================================================================
 * The HistoryReadRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_HistoryReadRequest
{
    OpcUa_RequestHeader       RequestHeader;
    OpcUa_ExtensionObject     HistoryReadDetails;
    OpcUa_TimestampsToReturn  TimestampsToReturn;
    OpcUa_Boolean             ReleaseContinuationPoints;
    OpcUa_Int32               NoOfNodesToRead;
    OpcUa_HistoryReadValueId* NodesToRead;
}
OpcUa_HistoryReadRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_HistoryReadRequest_Initialize(OpcUa_HistoryReadRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_HistoryReadRequest_Clear(OpcUa_HistoryReadRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryReadRequest_GetSize(OpcUa_HistoryReadRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryReadRequest_Encode(OpcUa_HistoryReadRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryReadRequest_Decode(OpcUa_HistoryReadRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_HistoryReadRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_HistoryReadResponse
/*============================================================================
 * The HistoryReadResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_HistoryReadResponse
{
    OpcUa_ResponseHeader     ResponseHeader;
    OpcUa_Int32              NoOfResults;
    OpcUa_HistoryReadResult* Results;
    OpcUa_Int32              NoOfDiagnosticInfos;
    OpcUa_DiagnosticInfo*    DiagnosticInfos;
}
OpcUa_HistoryReadResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_HistoryReadResponse_Initialize(OpcUa_HistoryReadResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_HistoryReadResponse_Clear(OpcUa_HistoryReadResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryReadResponse_GetSize(OpcUa_HistoryReadResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryReadResponse_Encode(OpcUa_HistoryReadResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryReadResponse_Decode(OpcUa_HistoryReadResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_HistoryReadResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_WriteValue
/*============================================================================
 * The WriteValue structure.
 *===========================================================================*/
typedef struct _OpcUa_WriteValue
{
    OpcUa_NodeId    NodeId;
    OpcUa_UInt32    AttributeId;
    OpcUa_String    IndexRange;
    OpcUa_DataValue Value;
}
OpcUa_WriteValue;

OPCUA_EXPORT OpcUa_Void OpcUa_WriteValue_Initialize(OpcUa_WriteValue* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_WriteValue_Clear(OpcUa_WriteValue* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_WriteValue_GetSize(OpcUa_WriteValue* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_WriteValue_Encode(OpcUa_WriteValue* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_WriteValue_Decode(OpcUa_WriteValue* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_WriteValue_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_Write
#ifndef OPCUA_EXCLUDE_WriteRequest
/*============================================================================
 * The WriteRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_WriteRequest
{
    OpcUa_RequestHeader RequestHeader;
    OpcUa_Int32         NoOfNodesToWrite;
    OpcUa_WriteValue*   NodesToWrite;
}
OpcUa_WriteRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_WriteRequest_Initialize(OpcUa_WriteRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_WriteRequest_Clear(OpcUa_WriteRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_WriteRequest_GetSize(OpcUa_WriteRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_WriteRequest_Encode(OpcUa_WriteRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_WriteRequest_Decode(OpcUa_WriteRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_WriteRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_WriteResponse
/*============================================================================
 * The WriteResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_WriteResponse
{
    OpcUa_ResponseHeader  ResponseHeader;
    OpcUa_Int32           NoOfResults;
    OpcUa_StatusCode*     Results;
    OpcUa_Int32           NoOfDiagnosticInfos;
    OpcUa_DiagnosticInfo* DiagnosticInfos;
}
OpcUa_WriteResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_WriteResponse_Initialize(OpcUa_WriteResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_WriteResponse_Clear(OpcUa_WriteResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_WriteResponse_GetSize(OpcUa_WriteResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_WriteResponse_Encode(OpcUa_WriteResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_WriteResponse_Decode(OpcUa_WriteResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_WriteResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_HistoryUpdateDetails
/*============================================================================
 * The HistoryUpdateDetails structure.
 *===========================================================================*/
typedef struct _OpcUa_HistoryUpdateDetails
{
    OpcUa_NodeId NodeId;
}
OpcUa_HistoryUpdateDetails;

OPCUA_EXPORT OpcUa_Void OpcUa_HistoryUpdateDetails_Initialize(OpcUa_HistoryUpdateDetails* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_HistoryUpdateDetails_Clear(OpcUa_HistoryUpdateDetails* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryUpdateDetails_GetSize(OpcUa_HistoryUpdateDetails* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryUpdateDetails_Encode(OpcUa_HistoryUpdateDetails* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryUpdateDetails_Decode(OpcUa_HistoryUpdateDetails* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_HistoryUpdateDetails_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_PerformUpdateType
/*============================================================================
 * The PerformUpdateType enumeration.
 *===========================================================================*/
typedef enum _OpcUa_PerformUpdateType
{
    OpcUa_PerformUpdateType_Insert  = 1,
    OpcUa_PerformUpdateType_Replace = 2,
    OpcUa_PerformUpdateType_Update  = 3,
    OpcUa_PerformUpdateType_Remove  = 4
#if OPCUA_FORCE_INT32_ENUMS
    ,_OpcUa_PerformUpdateType_MaxEnumerationValue = OpcUa_Int32_Max
#endif
}
OpcUa_PerformUpdateType;

#define OpcUa_PerformUpdateType_Clear(xValue) OpcUa_EnumeratedType_Clear(xValue, OpcUa_PerformUpdateType_Insert)

#define OpcUa_PerformUpdateType_Initialize(xValue) OpcUa_EnumeratedType_Initialize(xValue, OpcUa_PerformUpdateType_Insert)

OPCUA_IMEXPORT extern struct _OpcUa_EnumeratedType OpcUa_PerformUpdateType_EnumeratedType;
#endif

#ifndef OPCUA_EXCLUDE_UpdateDataDetails
/*============================================================================
 * The UpdateDataDetails structure.
 *===========================================================================*/
typedef struct _OpcUa_UpdateDataDetails
{
    OpcUa_NodeId            NodeId;
    OpcUa_PerformUpdateType PerformInsertReplace;
    OpcUa_Int32             NoOfUpdateValues;
    OpcUa_DataValue*        UpdateValues;
}
OpcUa_UpdateDataDetails;

OPCUA_EXPORT OpcUa_Void OpcUa_UpdateDataDetails_Initialize(OpcUa_UpdateDataDetails* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_UpdateDataDetails_Clear(OpcUa_UpdateDataDetails* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_UpdateDataDetails_GetSize(OpcUa_UpdateDataDetails* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_UpdateDataDetails_Encode(OpcUa_UpdateDataDetails* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_UpdateDataDetails_Decode(OpcUa_UpdateDataDetails* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_UpdateDataDetails_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_UpdateStructureDataDetails
/*============================================================================
 * The UpdateStructureDataDetails structure.
 *===========================================================================*/
typedef struct _OpcUa_UpdateStructureDataDetails
{
    OpcUa_NodeId            NodeId;
    OpcUa_PerformUpdateType PerformInsertReplace;
    OpcUa_Int32             NoOfUpdateValues;
    OpcUa_DataValue*        UpdateValues;
}
OpcUa_UpdateStructureDataDetails;

OPCUA_EXPORT OpcUa_Void OpcUa_UpdateStructureDataDetails_Initialize(OpcUa_UpdateStructureDataDetails* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_UpdateStructureDataDetails_Clear(OpcUa_UpdateStructureDataDetails* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_UpdateStructureDataDetails_GetSize(OpcUa_UpdateStructureDataDetails* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_UpdateStructureDataDetails_Encode(OpcUa_UpdateStructureDataDetails* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_UpdateStructureDataDetails_Decode(OpcUa_UpdateStructureDataDetails* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_UpdateStructureDataDetails_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_UpdateEventDetails
/*============================================================================
 * The UpdateEventDetails structure.
 *===========================================================================*/
typedef struct _OpcUa_UpdateEventDetails
{
    OpcUa_NodeId                 NodeId;
    OpcUa_PerformUpdateType      PerformInsertReplace;
    OpcUa_EventFilter            Filter;
    OpcUa_Int32                  NoOfEventData;
    OpcUa_HistoryEventFieldList* EventData;
}
OpcUa_UpdateEventDetails;

OPCUA_EXPORT OpcUa_Void OpcUa_UpdateEventDetails_Initialize(OpcUa_UpdateEventDetails* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_UpdateEventDetails_Clear(OpcUa_UpdateEventDetails* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_UpdateEventDetails_GetSize(OpcUa_UpdateEventDetails* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_UpdateEventDetails_Encode(OpcUa_UpdateEventDetails* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_UpdateEventDetails_Decode(OpcUa_UpdateEventDetails* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_UpdateEventDetails_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_DeleteRawModifiedDetails
/*============================================================================
 * The DeleteRawModifiedDetails structure.
 *===========================================================================*/
typedef struct _OpcUa_DeleteRawModifiedDetails
{
    OpcUa_NodeId   NodeId;
    OpcUa_Boolean  IsDeleteModified;
    OpcUa_DateTime StartTime;
    OpcUa_DateTime EndTime;
}
OpcUa_DeleteRawModifiedDetails;

OPCUA_EXPORT OpcUa_Void OpcUa_DeleteRawModifiedDetails_Initialize(OpcUa_DeleteRawModifiedDetails* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_DeleteRawModifiedDetails_Clear(OpcUa_DeleteRawModifiedDetails* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteRawModifiedDetails_GetSize(OpcUa_DeleteRawModifiedDetails* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteRawModifiedDetails_Encode(OpcUa_DeleteRawModifiedDetails* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteRawModifiedDetails_Decode(OpcUa_DeleteRawModifiedDetails* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_DeleteRawModifiedDetails_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_DeleteAtTimeDetails
/*============================================================================
 * The DeleteAtTimeDetails structure.
 *===========================================================================*/
typedef struct _OpcUa_DeleteAtTimeDetails
{
    OpcUa_NodeId    NodeId;
    OpcUa_Int32     NoOfReqTimes;
    OpcUa_DateTime* ReqTimes;
}
OpcUa_DeleteAtTimeDetails;

OPCUA_EXPORT OpcUa_Void OpcUa_DeleteAtTimeDetails_Initialize(OpcUa_DeleteAtTimeDetails* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_DeleteAtTimeDetails_Clear(OpcUa_DeleteAtTimeDetails* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteAtTimeDetails_GetSize(OpcUa_DeleteAtTimeDetails* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteAtTimeDetails_Encode(OpcUa_DeleteAtTimeDetails* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteAtTimeDetails_Decode(OpcUa_DeleteAtTimeDetails* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_DeleteAtTimeDetails_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_DeleteEventDetails
/*============================================================================
 * The DeleteEventDetails structure.
 *===========================================================================*/
typedef struct _OpcUa_DeleteEventDetails
{
    OpcUa_NodeId      NodeId;
    OpcUa_Int32       NoOfEventIds;
    OpcUa_ByteString* EventIds;
}
OpcUa_DeleteEventDetails;

OPCUA_EXPORT OpcUa_Void OpcUa_DeleteEventDetails_Initialize(OpcUa_DeleteEventDetails* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_DeleteEventDetails_Clear(OpcUa_DeleteEventDetails* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteEventDetails_GetSize(OpcUa_DeleteEventDetails* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteEventDetails_Encode(OpcUa_DeleteEventDetails* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteEventDetails_Decode(OpcUa_DeleteEventDetails* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_DeleteEventDetails_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_HistoryUpdateResult
/*============================================================================
 * The HistoryUpdateResult structure.
 *===========================================================================*/
typedef struct _OpcUa_HistoryUpdateResult
{
    OpcUa_StatusCode      StatusCode;
    OpcUa_Int32           NoOfOperationResults;
    OpcUa_StatusCode*     OperationResults;
    OpcUa_Int32           NoOfDiagnosticInfos;
    OpcUa_DiagnosticInfo* DiagnosticInfos;
}
OpcUa_HistoryUpdateResult;

OPCUA_EXPORT OpcUa_Void OpcUa_HistoryUpdateResult_Initialize(OpcUa_HistoryUpdateResult* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_HistoryUpdateResult_Clear(OpcUa_HistoryUpdateResult* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryUpdateResult_GetSize(OpcUa_HistoryUpdateResult* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryUpdateResult_Encode(OpcUa_HistoryUpdateResult* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryUpdateResult_Decode(OpcUa_HistoryUpdateResult* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_HistoryUpdateResult_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_HistoryUpdate
#ifndef OPCUA_EXCLUDE_HistoryUpdateRequest
/*============================================================================
 * The HistoryUpdateRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_HistoryUpdateRequest
{
    OpcUa_RequestHeader    RequestHeader;
    OpcUa_Int32            NoOfHistoryUpdateDetails;
    OpcUa_ExtensionObject* HistoryUpdateDetails;
}
OpcUa_HistoryUpdateRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_HistoryUpdateRequest_Initialize(OpcUa_HistoryUpdateRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_HistoryUpdateRequest_Clear(OpcUa_HistoryUpdateRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryUpdateRequest_GetSize(OpcUa_HistoryUpdateRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryUpdateRequest_Encode(OpcUa_HistoryUpdateRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryUpdateRequest_Decode(OpcUa_HistoryUpdateRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_HistoryUpdateRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_HistoryUpdateResponse
/*============================================================================
 * The HistoryUpdateResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_HistoryUpdateResponse
{
    OpcUa_ResponseHeader       ResponseHeader;
    OpcUa_Int32                NoOfResults;
    OpcUa_HistoryUpdateResult* Results;
    OpcUa_Int32                NoOfDiagnosticInfos;
    OpcUa_DiagnosticInfo*      DiagnosticInfos;
}
OpcUa_HistoryUpdateResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_HistoryUpdateResponse_Initialize(OpcUa_HistoryUpdateResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_HistoryUpdateResponse_Clear(OpcUa_HistoryUpdateResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryUpdateResponse_GetSize(OpcUa_HistoryUpdateResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryUpdateResponse_Encode(OpcUa_HistoryUpdateResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_HistoryUpdateResponse_Decode(OpcUa_HistoryUpdateResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_HistoryUpdateResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_CallMethodRequest
/*============================================================================
 * The CallMethodRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_CallMethodRequest
{
    OpcUa_NodeId   ObjectId;
    OpcUa_NodeId   MethodId;
    OpcUa_Int32    NoOfInputArguments;
    OpcUa_Variant* InputArguments;
}
OpcUa_CallMethodRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_CallMethodRequest_Initialize(OpcUa_CallMethodRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_CallMethodRequest_Clear(OpcUa_CallMethodRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CallMethodRequest_GetSize(OpcUa_CallMethodRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CallMethodRequest_Encode(OpcUa_CallMethodRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CallMethodRequest_Decode(OpcUa_CallMethodRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_CallMethodRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_CallMethodResult
/*============================================================================
 * The CallMethodResult structure.
 *===========================================================================*/
typedef struct _OpcUa_CallMethodResult
{
    OpcUa_StatusCode      StatusCode;
    OpcUa_Int32           NoOfInputArgumentResults;
    OpcUa_StatusCode*     InputArgumentResults;
    OpcUa_Int32           NoOfInputArgumentDiagnosticInfos;
    OpcUa_DiagnosticInfo* InputArgumentDiagnosticInfos;
    OpcUa_Int32           NoOfOutputArguments;
    OpcUa_Variant*        OutputArguments;
}
OpcUa_CallMethodResult;

OPCUA_EXPORT OpcUa_Void OpcUa_CallMethodResult_Initialize(OpcUa_CallMethodResult* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_CallMethodResult_Clear(OpcUa_CallMethodResult* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CallMethodResult_GetSize(OpcUa_CallMethodResult* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CallMethodResult_Encode(OpcUa_CallMethodResult* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CallMethodResult_Decode(OpcUa_CallMethodResult* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_CallMethodResult_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_Call
#ifndef OPCUA_EXCLUDE_CallRequest
/*============================================================================
 * The CallRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_CallRequest
{
    OpcUa_RequestHeader      RequestHeader;
    OpcUa_Int32              NoOfMethodsToCall;
    OpcUa_CallMethodRequest* MethodsToCall;
}
OpcUa_CallRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_CallRequest_Initialize(OpcUa_CallRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_CallRequest_Clear(OpcUa_CallRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CallRequest_GetSize(OpcUa_CallRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CallRequest_Encode(OpcUa_CallRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CallRequest_Decode(OpcUa_CallRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_CallRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_CallResponse
/*============================================================================
 * The CallResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_CallResponse
{
    OpcUa_ResponseHeader    ResponseHeader;
    OpcUa_Int32             NoOfResults;
    OpcUa_CallMethodResult* Results;
    OpcUa_Int32             NoOfDiagnosticInfos;
    OpcUa_DiagnosticInfo*   DiagnosticInfos;
}
OpcUa_CallResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_CallResponse_Initialize(OpcUa_CallResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_CallResponse_Clear(OpcUa_CallResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CallResponse_GetSize(OpcUa_CallResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CallResponse_Encode(OpcUa_CallResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CallResponse_Decode(OpcUa_CallResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_CallResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_MonitoringMode
/*============================================================================
 * The MonitoringMode enumeration.
 *===========================================================================*/
typedef enum _OpcUa_MonitoringMode
{
    OpcUa_MonitoringMode_Disabled  = 0,
    OpcUa_MonitoringMode_Sampling  = 1,
    OpcUa_MonitoringMode_Reporting = 2
#if OPCUA_FORCE_INT32_ENUMS
    ,_OpcUa_MonitoringMode_MaxEnumerationValue = OpcUa_Int32_Max
#endif
}
OpcUa_MonitoringMode;

#define OpcUa_MonitoringMode_Clear(xValue) OpcUa_EnumeratedType_Clear(xValue, OpcUa_MonitoringMode_Disabled)

#define OpcUa_MonitoringMode_Initialize(xValue) OpcUa_EnumeratedType_Initialize(xValue, OpcUa_MonitoringMode_Disabled)

OPCUA_IMEXPORT extern struct _OpcUa_EnumeratedType OpcUa_MonitoringMode_EnumeratedType;
#endif

#ifndef OPCUA_EXCLUDE_DataChangeTrigger
/*============================================================================
 * The DataChangeTrigger enumeration.
 *===========================================================================*/
typedef enum _OpcUa_DataChangeTrigger
{
    OpcUa_DataChangeTrigger_Status               = 0,
    OpcUa_DataChangeTrigger_StatusValue          = 1,
    OpcUa_DataChangeTrigger_StatusValueTimestamp = 2
#if OPCUA_FORCE_INT32_ENUMS
    ,_OpcUa_DataChangeTrigger_MaxEnumerationValue = OpcUa_Int32_Max
#endif
}
OpcUa_DataChangeTrigger;

#define OpcUa_DataChangeTrigger_Clear(xValue) OpcUa_EnumeratedType_Clear(xValue, OpcUa_DataChangeTrigger_Status)

#define OpcUa_DataChangeTrigger_Initialize(xValue) OpcUa_EnumeratedType_Initialize(xValue, OpcUa_DataChangeTrigger_Status)

OPCUA_IMEXPORT extern struct _OpcUa_EnumeratedType OpcUa_DataChangeTrigger_EnumeratedType;
#endif

#ifndef OPCUA_EXCLUDE_DeadbandType
/*============================================================================
 * The DeadbandType enumeration.
 *===========================================================================*/
typedef enum _OpcUa_DeadbandType
{
    OpcUa_DeadbandType_None     = 0,
    OpcUa_DeadbandType_Absolute = 1,
    OpcUa_DeadbandType_Percent  = 2
#if OPCUA_FORCE_INT32_ENUMS
    ,_OpcUa_DeadbandType_MaxEnumerationValue = OpcUa_Int32_Max
#endif
}
OpcUa_DeadbandType;

#define OpcUa_DeadbandType_Clear(xValue) OpcUa_EnumeratedType_Clear(xValue, OpcUa_DeadbandType_None)

#define OpcUa_DeadbandType_Initialize(xValue) OpcUa_EnumeratedType_Initialize(xValue, OpcUa_DeadbandType_None)

OPCUA_IMEXPORT extern struct _OpcUa_EnumeratedType OpcUa_DeadbandType_EnumeratedType;
#endif

#ifndef OPCUA_EXCLUDE_DataChangeFilter
/*============================================================================
 * The DataChangeFilter structure.
 *===========================================================================*/
typedef struct _OpcUa_DataChangeFilter
{
    OpcUa_DataChangeTrigger Trigger;
    OpcUa_UInt32            DeadbandType;
    OpcUa_Double            DeadbandValue;
}
OpcUa_DataChangeFilter;

OPCUA_EXPORT OpcUa_Void OpcUa_DataChangeFilter_Initialize(OpcUa_DataChangeFilter* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_DataChangeFilter_Clear(OpcUa_DataChangeFilter* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DataChangeFilter_GetSize(OpcUa_DataChangeFilter* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DataChangeFilter_Encode(OpcUa_DataChangeFilter* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DataChangeFilter_Decode(OpcUa_DataChangeFilter* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_DataChangeFilter_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_AggregateFilter
/*============================================================================
 * The AggregateFilter structure.
 *===========================================================================*/
typedef struct _OpcUa_AggregateFilter
{
    OpcUa_DateTime               StartTime;
    OpcUa_NodeId                 AggregateType;
    OpcUa_Double                 ProcessingInterval;
    OpcUa_AggregateConfiguration AggregateConfiguration;
}
OpcUa_AggregateFilter;

OPCUA_EXPORT OpcUa_Void OpcUa_AggregateFilter_Initialize(OpcUa_AggregateFilter* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_AggregateFilter_Clear(OpcUa_AggregateFilter* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AggregateFilter_GetSize(OpcUa_AggregateFilter* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AggregateFilter_Encode(OpcUa_AggregateFilter* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AggregateFilter_Decode(OpcUa_AggregateFilter* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_AggregateFilter_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_EventFilterResult
/*============================================================================
 * The EventFilterResult structure.
 *===========================================================================*/
typedef struct _OpcUa_EventFilterResult
{
    OpcUa_Int32               NoOfSelectClauseResults;
    OpcUa_StatusCode*         SelectClauseResults;
    OpcUa_Int32               NoOfSelectClauseDiagnosticInfos;
    OpcUa_DiagnosticInfo*     SelectClauseDiagnosticInfos;
    OpcUa_ContentFilterResult WhereClauseResult;
}
OpcUa_EventFilterResult;

OPCUA_EXPORT OpcUa_Void OpcUa_EventFilterResult_Initialize(OpcUa_EventFilterResult* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_EventFilterResult_Clear(OpcUa_EventFilterResult* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EventFilterResult_GetSize(OpcUa_EventFilterResult* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EventFilterResult_Encode(OpcUa_EventFilterResult* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EventFilterResult_Decode(OpcUa_EventFilterResult* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_EventFilterResult_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_AggregateFilterResult
/*============================================================================
 * The AggregateFilterResult structure.
 *===========================================================================*/
typedef struct _OpcUa_AggregateFilterResult
{
    OpcUa_DateTime               RevisedStartTime;
    OpcUa_Double                 RevisedProcessingInterval;
    OpcUa_AggregateConfiguration RevisedAggregateConfiguration;
}
OpcUa_AggregateFilterResult;

OPCUA_EXPORT OpcUa_Void OpcUa_AggregateFilterResult_Initialize(OpcUa_AggregateFilterResult* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_AggregateFilterResult_Clear(OpcUa_AggregateFilterResult* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AggregateFilterResult_GetSize(OpcUa_AggregateFilterResult* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AggregateFilterResult_Encode(OpcUa_AggregateFilterResult* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AggregateFilterResult_Decode(OpcUa_AggregateFilterResult* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_AggregateFilterResult_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_MonitoringParameters
/*============================================================================
 * The MonitoringParameters structure.
 *===========================================================================*/
typedef struct _OpcUa_MonitoringParameters
{
    OpcUa_UInt32          ClientHandle;
    OpcUa_Double          SamplingInterval;
    OpcUa_ExtensionObject Filter;
    OpcUa_UInt32          QueueSize;
    OpcUa_Boolean         DiscardOldest;
}
OpcUa_MonitoringParameters;

OPCUA_EXPORT OpcUa_Void OpcUa_MonitoringParameters_Initialize(OpcUa_MonitoringParameters* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_MonitoringParameters_Clear(OpcUa_MonitoringParameters* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_MonitoringParameters_GetSize(OpcUa_MonitoringParameters* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_MonitoringParameters_Encode(OpcUa_MonitoringParameters* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_MonitoringParameters_Decode(OpcUa_MonitoringParameters* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_MonitoringParameters_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_MonitoredItemCreateRequest
/*============================================================================
 * The MonitoredItemCreateRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_MonitoredItemCreateRequest
{
    OpcUa_ReadValueId          ItemToMonitor;
    OpcUa_MonitoringMode       MonitoringMode;
    OpcUa_MonitoringParameters RequestedParameters;
}
OpcUa_MonitoredItemCreateRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_MonitoredItemCreateRequest_Initialize(OpcUa_MonitoredItemCreateRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_MonitoredItemCreateRequest_Clear(OpcUa_MonitoredItemCreateRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_MonitoredItemCreateRequest_GetSize(OpcUa_MonitoredItemCreateRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_MonitoredItemCreateRequest_Encode(OpcUa_MonitoredItemCreateRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_MonitoredItemCreateRequest_Decode(OpcUa_MonitoredItemCreateRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_MonitoredItemCreateRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_MonitoredItemCreateResult
/*============================================================================
 * The MonitoredItemCreateResult structure.
 *===========================================================================*/
typedef struct _OpcUa_MonitoredItemCreateResult
{
    OpcUa_StatusCode      StatusCode;
    OpcUa_UInt32          MonitoredItemId;
    OpcUa_Double          RevisedSamplingInterval;
    OpcUa_UInt32          RevisedQueueSize;
    OpcUa_ExtensionObject FilterResult;
}
OpcUa_MonitoredItemCreateResult;

OPCUA_EXPORT OpcUa_Void OpcUa_MonitoredItemCreateResult_Initialize(OpcUa_MonitoredItemCreateResult* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_MonitoredItemCreateResult_Clear(OpcUa_MonitoredItemCreateResult* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_MonitoredItemCreateResult_GetSize(OpcUa_MonitoredItemCreateResult* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_MonitoredItemCreateResult_Encode(OpcUa_MonitoredItemCreateResult* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_MonitoredItemCreateResult_Decode(OpcUa_MonitoredItemCreateResult* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_MonitoredItemCreateResult_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_CreateMonitoredItems
#ifndef OPCUA_EXCLUDE_CreateMonitoredItemsRequest
/*============================================================================
 * The CreateMonitoredItemsRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_CreateMonitoredItemsRequest
{
    OpcUa_RequestHeader               RequestHeader;
    OpcUa_UInt32                      SubscriptionId;
    OpcUa_TimestampsToReturn          TimestampsToReturn;
    OpcUa_Int32                       NoOfItemsToCreate;
    OpcUa_MonitoredItemCreateRequest* ItemsToCreate;
}
OpcUa_CreateMonitoredItemsRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_CreateMonitoredItemsRequest_Initialize(OpcUa_CreateMonitoredItemsRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_CreateMonitoredItemsRequest_Clear(OpcUa_CreateMonitoredItemsRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CreateMonitoredItemsRequest_GetSize(OpcUa_CreateMonitoredItemsRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CreateMonitoredItemsRequest_Encode(OpcUa_CreateMonitoredItemsRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CreateMonitoredItemsRequest_Decode(OpcUa_CreateMonitoredItemsRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_CreateMonitoredItemsRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_CreateMonitoredItemsResponse
/*============================================================================
 * The CreateMonitoredItemsResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_CreateMonitoredItemsResponse
{
    OpcUa_ResponseHeader             ResponseHeader;
    OpcUa_Int32                      NoOfResults;
    OpcUa_MonitoredItemCreateResult* Results;
    OpcUa_Int32                      NoOfDiagnosticInfos;
    OpcUa_DiagnosticInfo*            DiagnosticInfos;
}
OpcUa_CreateMonitoredItemsResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_CreateMonitoredItemsResponse_Initialize(OpcUa_CreateMonitoredItemsResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_CreateMonitoredItemsResponse_Clear(OpcUa_CreateMonitoredItemsResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CreateMonitoredItemsResponse_GetSize(OpcUa_CreateMonitoredItemsResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CreateMonitoredItemsResponse_Encode(OpcUa_CreateMonitoredItemsResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CreateMonitoredItemsResponse_Decode(OpcUa_CreateMonitoredItemsResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_CreateMonitoredItemsResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_MonitoredItemModifyRequest
/*============================================================================
 * The MonitoredItemModifyRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_MonitoredItemModifyRequest
{
    OpcUa_UInt32               MonitoredItemId;
    OpcUa_MonitoringParameters RequestedParameters;
}
OpcUa_MonitoredItemModifyRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_MonitoredItemModifyRequest_Initialize(OpcUa_MonitoredItemModifyRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_MonitoredItemModifyRequest_Clear(OpcUa_MonitoredItemModifyRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_MonitoredItemModifyRequest_GetSize(OpcUa_MonitoredItemModifyRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_MonitoredItemModifyRequest_Encode(OpcUa_MonitoredItemModifyRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_MonitoredItemModifyRequest_Decode(OpcUa_MonitoredItemModifyRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_MonitoredItemModifyRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_MonitoredItemModifyResult
/*============================================================================
 * The MonitoredItemModifyResult structure.
 *===========================================================================*/
typedef struct _OpcUa_MonitoredItemModifyResult
{
    OpcUa_StatusCode      StatusCode;
    OpcUa_Double          RevisedSamplingInterval;
    OpcUa_UInt32          RevisedQueueSize;
    OpcUa_ExtensionObject FilterResult;
}
OpcUa_MonitoredItemModifyResult;

OPCUA_EXPORT OpcUa_Void OpcUa_MonitoredItemModifyResult_Initialize(OpcUa_MonitoredItemModifyResult* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_MonitoredItemModifyResult_Clear(OpcUa_MonitoredItemModifyResult* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_MonitoredItemModifyResult_GetSize(OpcUa_MonitoredItemModifyResult* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_MonitoredItemModifyResult_Encode(OpcUa_MonitoredItemModifyResult* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_MonitoredItemModifyResult_Decode(OpcUa_MonitoredItemModifyResult* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_MonitoredItemModifyResult_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ModifyMonitoredItems
#ifndef OPCUA_EXCLUDE_ModifyMonitoredItemsRequest
/*============================================================================
 * The ModifyMonitoredItemsRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_ModifyMonitoredItemsRequest
{
    OpcUa_RequestHeader               RequestHeader;
    OpcUa_UInt32                      SubscriptionId;
    OpcUa_TimestampsToReturn          TimestampsToReturn;
    OpcUa_Int32                       NoOfItemsToModify;
    OpcUa_MonitoredItemModifyRequest* ItemsToModify;
}
OpcUa_ModifyMonitoredItemsRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_ModifyMonitoredItemsRequest_Initialize(OpcUa_ModifyMonitoredItemsRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ModifyMonitoredItemsRequest_Clear(OpcUa_ModifyMonitoredItemsRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ModifyMonitoredItemsRequest_GetSize(OpcUa_ModifyMonitoredItemsRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ModifyMonitoredItemsRequest_Encode(OpcUa_ModifyMonitoredItemsRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ModifyMonitoredItemsRequest_Decode(OpcUa_ModifyMonitoredItemsRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ModifyMonitoredItemsRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ModifyMonitoredItemsResponse
/*============================================================================
 * The ModifyMonitoredItemsResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_ModifyMonitoredItemsResponse
{
    OpcUa_ResponseHeader             ResponseHeader;
    OpcUa_Int32                      NoOfResults;
    OpcUa_MonitoredItemModifyResult* Results;
    OpcUa_Int32                      NoOfDiagnosticInfos;
    OpcUa_DiagnosticInfo*            DiagnosticInfos;
}
OpcUa_ModifyMonitoredItemsResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_ModifyMonitoredItemsResponse_Initialize(OpcUa_ModifyMonitoredItemsResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ModifyMonitoredItemsResponse_Clear(OpcUa_ModifyMonitoredItemsResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ModifyMonitoredItemsResponse_GetSize(OpcUa_ModifyMonitoredItemsResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ModifyMonitoredItemsResponse_Encode(OpcUa_ModifyMonitoredItemsResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ModifyMonitoredItemsResponse_Decode(OpcUa_ModifyMonitoredItemsResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ModifyMonitoredItemsResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_SetMonitoringMode
#ifndef OPCUA_EXCLUDE_SetMonitoringModeRequest
/*============================================================================
 * The SetMonitoringModeRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_SetMonitoringModeRequest
{
    OpcUa_RequestHeader  RequestHeader;
    OpcUa_UInt32         SubscriptionId;
    OpcUa_MonitoringMode MonitoringMode;
    OpcUa_Int32          NoOfMonitoredItemIds;
    OpcUa_UInt32*        MonitoredItemIds;
}
OpcUa_SetMonitoringModeRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_SetMonitoringModeRequest_Initialize(OpcUa_SetMonitoringModeRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_SetMonitoringModeRequest_Clear(OpcUa_SetMonitoringModeRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SetMonitoringModeRequest_GetSize(OpcUa_SetMonitoringModeRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SetMonitoringModeRequest_Encode(OpcUa_SetMonitoringModeRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SetMonitoringModeRequest_Decode(OpcUa_SetMonitoringModeRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_SetMonitoringModeRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_SetMonitoringModeResponse
/*============================================================================
 * The SetMonitoringModeResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_SetMonitoringModeResponse
{
    OpcUa_ResponseHeader  ResponseHeader;
    OpcUa_Int32           NoOfResults;
    OpcUa_StatusCode*     Results;
    OpcUa_Int32           NoOfDiagnosticInfos;
    OpcUa_DiagnosticInfo* DiagnosticInfos;
}
OpcUa_SetMonitoringModeResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_SetMonitoringModeResponse_Initialize(OpcUa_SetMonitoringModeResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_SetMonitoringModeResponse_Clear(OpcUa_SetMonitoringModeResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SetMonitoringModeResponse_GetSize(OpcUa_SetMonitoringModeResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SetMonitoringModeResponse_Encode(OpcUa_SetMonitoringModeResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SetMonitoringModeResponse_Decode(OpcUa_SetMonitoringModeResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_SetMonitoringModeResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_SetTriggering
#ifndef OPCUA_EXCLUDE_SetTriggeringRequest
/*============================================================================
 * The SetTriggeringRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_SetTriggeringRequest
{
    OpcUa_RequestHeader RequestHeader;
    OpcUa_UInt32        SubscriptionId;
    OpcUa_UInt32        TriggeringItemId;
    OpcUa_Int32         NoOfLinksToAdd;
    OpcUa_UInt32*       LinksToAdd;
    OpcUa_Int32         NoOfLinksToRemove;
    OpcUa_UInt32*       LinksToRemove;
}
OpcUa_SetTriggeringRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_SetTriggeringRequest_Initialize(OpcUa_SetTriggeringRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_SetTriggeringRequest_Clear(OpcUa_SetTriggeringRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SetTriggeringRequest_GetSize(OpcUa_SetTriggeringRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SetTriggeringRequest_Encode(OpcUa_SetTriggeringRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SetTriggeringRequest_Decode(OpcUa_SetTriggeringRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_SetTriggeringRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_SetTriggeringResponse
/*============================================================================
 * The SetTriggeringResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_SetTriggeringResponse
{
    OpcUa_ResponseHeader  ResponseHeader;
    OpcUa_Int32           NoOfAddResults;
    OpcUa_StatusCode*     AddResults;
    OpcUa_Int32           NoOfAddDiagnosticInfos;
    OpcUa_DiagnosticInfo* AddDiagnosticInfos;
    OpcUa_Int32           NoOfRemoveResults;
    OpcUa_StatusCode*     RemoveResults;
    OpcUa_Int32           NoOfRemoveDiagnosticInfos;
    OpcUa_DiagnosticInfo* RemoveDiagnosticInfos;
}
OpcUa_SetTriggeringResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_SetTriggeringResponse_Initialize(OpcUa_SetTriggeringResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_SetTriggeringResponse_Clear(OpcUa_SetTriggeringResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SetTriggeringResponse_GetSize(OpcUa_SetTriggeringResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SetTriggeringResponse_Encode(OpcUa_SetTriggeringResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SetTriggeringResponse_Decode(OpcUa_SetTriggeringResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_SetTriggeringResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_DeleteMonitoredItems
#ifndef OPCUA_EXCLUDE_DeleteMonitoredItemsRequest
/*============================================================================
 * The DeleteMonitoredItemsRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_DeleteMonitoredItemsRequest
{
    OpcUa_RequestHeader RequestHeader;
    OpcUa_UInt32        SubscriptionId;
    OpcUa_Int32         NoOfMonitoredItemIds;
    OpcUa_UInt32*       MonitoredItemIds;
}
OpcUa_DeleteMonitoredItemsRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_DeleteMonitoredItemsRequest_Initialize(OpcUa_DeleteMonitoredItemsRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_DeleteMonitoredItemsRequest_Clear(OpcUa_DeleteMonitoredItemsRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteMonitoredItemsRequest_GetSize(OpcUa_DeleteMonitoredItemsRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteMonitoredItemsRequest_Encode(OpcUa_DeleteMonitoredItemsRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteMonitoredItemsRequest_Decode(OpcUa_DeleteMonitoredItemsRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_DeleteMonitoredItemsRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_DeleteMonitoredItemsResponse
/*============================================================================
 * The DeleteMonitoredItemsResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_DeleteMonitoredItemsResponse
{
    OpcUa_ResponseHeader  ResponseHeader;
    OpcUa_Int32           NoOfResults;
    OpcUa_StatusCode*     Results;
    OpcUa_Int32           NoOfDiagnosticInfos;
    OpcUa_DiagnosticInfo* DiagnosticInfos;
}
OpcUa_DeleteMonitoredItemsResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_DeleteMonitoredItemsResponse_Initialize(OpcUa_DeleteMonitoredItemsResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_DeleteMonitoredItemsResponse_Clear(OpcUa_DeleteMonitoredItemsResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteMonitoredItemsResponse_GetSize(OpcUa_DeleteMonitoredItemsResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteMonitoredItemsResponse_Encode(OpcUa_DeleteMonitoredItemsResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteMonitoredItemsResponse_Decode(OpcUa_DeleteMonitoredItemsResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_DeleteMonitoredItemsResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_CreateSubscription
#ifndef OPCUA_EXCLUDE_CreateSubscriptionRequest
/*============================================================================
 * The CreateSubscriptionRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_CreateSubscriptionRequest
{
    OpcUa_RequestHeader RequestHeader;
    OpcUa_Double        RequestedPublishingInterval;
    OpcUa_UInt32        RequestedLifetimeCount;
    OpcUa_UInt32        RequestedMaxKeepAliveCount;
    OpcUa_UInt32        MaxNotificationsPerPublish;
    OpcUa_Boolean       PublishingEnabled;
    OpcUa_Byte          Priority;
}
OpcUa_CreateSubscriptionRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_CreateSubscriptionRequest_Initialize(OpcUa_CreateSubscriptionRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_CreateSubscriptionRequest_Clear(OpcUa_CreateSubscriptionRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CreateSubscriptionRequest_GetSize(OpcUa_CreateSubscriptionRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CreateSubscriptionRequest_Encode(OpcUa_CreateSubscriptionRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CreateSubscriptionRequest_Decode(OpcUa_CreateSubscriptionRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_CreateSubscriptionRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_CreateSubscriptionResponse
/*============================================================================
 * The CreateSubscriptionResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_CreateSubscriptionResponse
{
    OpcUa_ResponseHeader ResponseHeader;
    OpcUa_UInt32         SubscriptionId;
    OpcUa_Double         RevisedPublishingInterval;
    OpcUa_UInt32         RevisedLifetimeCount;
    OpcUa_UInt32         RevisedMaxKeepAliveCount;
}
OpcUa_CreateSubscriptionResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_CreateSubscriptionResponse_Initialize(OpcUa_CreateSubscriptionResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_CreateSubscriptionResponse_Clear(OpcUa_CreateSubscriptionResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CreateSubscriptionResponse_GetSize(OpcUa_CreateSubscriptionResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CreateSubscriptionResponse_Encode(OpcUa_CreateSubscriptionResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_CreateSubscriptionResponse_Decode(OpcUa_CreateSubscriptionResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_CreateSubscriptionResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_ModifySubscription
#ifndef OPCUA_EXCLUDE_ModifySubscriptionRequest
/*============================================================================
 * The ModifySubscriptionRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_ModifySubscriptionRequest
{
    OpcUa_RequestHeader RequestHeader;
    OpcUa_UInt32        SubscriptionId;
    OpcUa_Double        RequestedPublishingInterval;
    OpcUa_UInt32        RequestedLifetimeCount;
    OpcUa_UInt32        RequestedMaxKeepAliveCount;
    OpcUa_UInt32        MaxNotificationsPerPublish;
    OpcUa_Byte          Priority;
}
OpcUa_ModifySubscriptionRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_ModifySubscriptionRequest_Initialize(OpcUa_ModifySubscriptionRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ModifySubscriptionRequest_Clear(OpcUa_ModifySubscriptionRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ModifySubscriptionRequest_GetSize(OpcUa_ModifySubscriptionRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ModifySubscriptionRequest_Encode(OpcUa_ModifySubscriptionRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ModifySubscriptionRequest_Decode(OpcUa_ModifySubscriptionRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ModifySubscriptionRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ModifySubscriptionResponse
/*============================================================================
 * The ModifySubscriptionResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_ModifySubscriptionResponse
{
    OpcUa_ResponseHeader ResponseHeader;
    OpcUa_Double         RevisedPublishingInterval;
    OpcUa_UInt32         RevisedLifetimeCount;
    OpcUa_UInt32         RevisedMaxKeepAliveCount;
}
OpcUa_ModifySubscriptionResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_ModifySubscriptionResponse_Initialize(OpcUa_ModifySubscriptionResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ModifySubscriptionResponse_Clear(OpcUa_ModifySubscriptionResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ModifySubscriptionResponse_GetSize(OpcUa_ModifySubscriptionResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ModifySubscriptionResponse_Encode(OpcUa_ModifySubscriptionResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ModifySubscriptionResponse_Decode(OpcUa_ModifySubscriptionResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ModifySubscriptionResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_SetPublishingMode
#ifndef OPCUA_EXCLUDE_SetPublishingModeRequest
/*============================================================================
 * The SetPublishingModeRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_SetPublishingModeRequest
{
    OpcUa_RequestHeader RequestHeader;
    OpcUa_Boolean       PublishingEnabled;
    OpcUa_Int32         NoOfSubscriptionIds;
    OpcUa_UInt32*       SubscriptionIds;
}
OpcUa_SetPublishingModeRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_SetPublishingModeRequest_Initialize(OpcUa_SetPublishingModeRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_SetPublishingModeRequest_Clear(OpcUa_SetPublishingModeRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SetPublishingModeRequest_GetSize(OpcUa_SetPublishingModeRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SetPublishingModeRequest_Encode(OpcUa_SetPublishingModeRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SetPublishingModeRequest_Decode(OpcUa_SetPublishingModeRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_SetPublishingModeRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_SetPublishingModeResponse
/*============================================================================
 * The SetPublishingModeResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_SetPublishingModeResponse
{
    OpcUa_ResponseHeader  ResponseHeader;
    OpcUa_Int32           NoOfResults;
    OpcUa_StatusCode*     Results;
    OpcUa_Int32           NoOfDiagnosticInfos;
    OpcUa_DiagnosticInfo* DiagnosticInfos;
}
OpcUa_SetPublishingModeResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_SetPublishingModeResponse_Initialize(OpcUa_SetPublishingModeResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_SetPublishingModeResponse_Clear(OpcUa_SetPublishingModeResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SetPublishingModeResponse_GetSize(OpcUa_SetPublishingModeResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SetPublishingModeResponse_Encode(OpcUa_SetPublishingModeResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SetPublishingModeResponse_Decode(OpcUa_SetPublishingModeResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_SetPublishingModeResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_NotificationMessage
/*============================================================================
 * The NotificationMessage structure.
 *===========================================================================*/
typedef struct _OpcUa_NotificationMessage
{
    OpcUa_UInt32           SequenceNumber;
    OpcUa_DateTime         PublishTime;
    OpcUa_Int32            NoOfNotificationData;
    OpcUa_ExtensionObject* NotificationData;
}
OpcUa_NotificationMessage;

OPCUA_EXPORT OpcUa_Void OpcUa_NotificationMessage_Initialize(OpcUa_NotificationMessage* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_NotificationMessage_Clear(OpcUa_NotificationMessage* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_NotificationMessage_GetSize(OpcUa_NotificationMessage* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_NotificationMessage_Encode(OpcUa_NotificationMessage* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_NotificationMessage_Decode(OpcUa_NotificationMessage* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_NotificationMessage_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_MonitoredItemNotification
/*============================================================================
 * The MonitoredItemNotification structure.
 *===========================================================================*/
typedef struct _OpcUa_MonitoredItemNotification
{
    OpcUa_UInt32    ClientHandle;
    OpcUa_DataValue Value;
}
OpcUa_MonitoredItemNotification;

OPCUA_EXPORT OpcUa_Void OpcUa_MonitoredItemNotification_Initialize(OpcUa_MonitoredItemNotification* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_MonitoredItemNotification_Clear(OpcUa_MonitoredItemNotification* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_MonitoredItemNotification_GetSize(OpcUa_MonitoredItemNotification* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_MonitoredItemNotification_Encode(OpcUa_MonitoredItemNotification* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_MonitoredItemNotification_Decode(OpcUa_MonitoredItemNotification* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_MonitoredItemNotification_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_DataChangeNotification
/*============================================================================
 * The DataChangeNotification structure.
 *===========================================================================*/
typedef struct _OpcUa_DataChangeNotification
{
    OpcUa_Int32                      NoOfMonitoredItems;
    OpcUa_MonitoredItemNotification* MonitoredItems;
    OpcUa_Int32                      NoOfDiagnosticInfos;
    OpcUa_DiagnosticInfo*            DiagnosticInfos;
}
OpcUa_DataChangeNotification;

OPCUA_EXPORT OpcUa_Void OpcUa_DataChangeNotification_Initialize(OpcUa_DataChangeNotification* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_DataChangeNotification_Clear(OpcUa_DataChangeNotification* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DataChangeNotification_GetSize(OpcUa_DataChangeNotification* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DataChangeNotification_Encode(OpcUa_DataChangeNotification* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DataChangeNotification_Decode(OpcUa_DataChangeNotification* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_DataChangeNotification_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_EventFieldList
/*============================================================================
 * The EventFieldList structure.
 *===========================================================================*/
typedef struct _OpcUa_EventFieldList
{
    OpcUa_UInt32   ClientHandle;
    OpcUa_Int32    NoOfEventFields;
    OpcUa_Variant* EventFields;
}
OpcUa_EventFieldList;

OPCUA_EXPORT OpcUa_Void OpcUa_EventFieldList_Initialize(OpcUa_EventFieldList* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_EventFieldList_Clear(OpcUa_EventFieldList* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EventFieldList_GetSize(OpcUa_EventFieldList* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EventFieldList_Encode(OpcUa_EventFieldList* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EventFieldList_Decode(OpcUa_EventFieldList* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_EventFieldList_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_EventNotificationList
/*============================================================================
 * The EventNotificationList structure.
 *===========================================================================*/
typedef struct _OpcUa_EventNotificationList
{
    OpcUa_Int32           NoOfEvents;
    OpcUa_EventFieldList* Events;
}
OpcUa_EventNotificationList;

OPCUA_EXPORT OpcUa_Void OpcUa_EventNotificationList_Initialize(OpcUa_EventNotificationList* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_EventNotificationList_Clear(OpcUa_EventNotificationList* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EventNotificationList_GetSize(OpcUa_EventNotificationList* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EventNotificationList_Encode(OpcUa_EventNotificationList* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EventNotificationList_Decode(OpcUa_EventNotificationList* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_EventNotificationList_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_StatusChangeNotification
/*============================================================================
 * The StatusChangeNotification structure.
 *===========================================================================*/
typedef struct _OpcUa_StatusChangeNotification
{
    OpcUa_StatusCode     Status;
    OpcUa_DiagnosticInfo DiagnosticInfo;
}
OpcUa_StatusChangeNotification;

OPCUA_EXPORT OpcUa_Void OpcUa_StatusChangeNotification_Initialize(OpcUa_StatusChangeNotification* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_StatusChangeNotification_Clear(OpcUa_StatusChangeNotification* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_StatusChangeNotification_GetSize(OpcUa_StatusChangeNotification* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_StatusChangeNotification_Encode(OpcUa_StatusChangeNotification* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_StatusChangeNotification_Decode(OpcUa_StatusChangeNotification* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_StatusChangeNotification_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_SubscriptionAcknowledgement
/*============================================================================
 * The SubscriptionAcknowledgement structure.
 *===========================================================================*/
typedef struct _OpcUa_SubscriptionAcknowledgement
{
    OpcUa_UInt32 SubscriptionId;
    OpcUa_UInt32 SequenceNumber;
}
OpcUa_SubscriptionAcknowledgement;

OPCUA_EXPORT OpcUa_Void OpcUa_SubscriptionAcknowledgement_Initialize(OpcUa_SubscriptionAcknowledgement* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_SubscriptionAcknowledgement_Clear(OpcUa_SubscriptionAcknowledgement* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SubscriptionAcknowledgement_GetSize(OpcUa_SubscriptionAcknowledgement* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SubscriptionAcknowledgement_Encode(OpcUa_SubscriptionAcknowledgement* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SubscriptionAcknowledgement_Decode(OpcUa_SubscriptionAcknowledgement* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_SubscriptionAcknowledgement_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_Publish
#ifndef OPCUA_EXCLUDE_PublishRequest
/*============================================================================
 * The PublishRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_PublishRequest
{
    OpcUa_RequestHeader                RequestHeader;
    OpcUa_Int32                        NoOfSubscriptionAcknowledgements;
    OpcUa_SubscriptionAcknowledgement* SubscriptionAcknowledgements;
}
OpcUa_PublishRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_PublishRequest_Initialize(OpcUa_PublishRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_PublishRequest_Clear(OpcUa_PublishRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_PublishRequest_GetSize(OpcUa_PublishRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_PublishRequest_Encode(OpcUa_PublishRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_PublishRequest_Decode(OpcUa_PublishRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_PublishRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_PublishResponse
/*============================================================================
 * The PublishResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_PublishResponse
{
    OpcUa_ResponseHeader      ResponseHeader;
    OpcUa_UInt32              SubscriptionId;
    OpcUa_Int32               NoOfAvailableSequenceNumbers;
    OpcUa_UInt32*             AvailableSequenceNumbers;
    OpcUa_Boolean             MoreNotifications;
    OpcUa_NotificationMessage NotificationMessage;
    OpcUa_Int32               NoOfResults;
    OpcUa_StatusCode*         Results;
    OpcUa_Int32               NoOfDiagnosticInfos;
    OpcUa_DiagnosticInfo*     DiagnosticInfos;
}
OpcUa_PublishResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_PublishResponse_Initialize(OpcUa_PublishResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_PublishResponse_Clear(OpcUa_PublishResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_PublishResponse_GetSize(OpcUa_PublishResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_PublishResponse_Encode(OpcUa_PublishResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_PublishResponse_Decode(OpcUa_PublishResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_PublishResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_Republish
#ifndef OPCUA_EXCLUDE_RepublishRequest
/*============================================================================
 * The RepublishRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_RepublishRequest
{
    OpcUa_RequestHeader RequestHeader;
    OpcUa_UInt32        SubscriptionId;
    OpcUa_UInt32        RetransmitSequenceNumber;
}
OpcUa_RepublishRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_RepublishRequest_Initialize(OpcUa_RepublishRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_RepublishRequest_Clear(OpcUa_RepublishRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RepublishRequest_GetSize(OpcUa_RepublishRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RepublishRequest_Encode(OpcUa_RepublishRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RepublishRequest_Decode(OpcUa_RepublishRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_RepublishRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_RepublishResponse
/*============================================================================
 * The RepublishResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_RepublishResponse
{
    OpcUa_ResponseHeader      ResponseHeader;
    OpcUa_NotificationMessage NotificationMessage;
}
OpcUa_RepublishResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_RepublishResponse_Initialize(OpcUa_RepublishResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_RepublishResponse_Clear(OpcUa_RepublishResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RepublishResponse_GetSize(OpcUa_RepublishResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RepublishResponse_Encode(OpcUa_RepublishResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RepublishResponse_Decode(OpcUa_RepublishResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_RepublishResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_TransferResult
/*============================================================================
 * The TransferResult structure.
 *===========================================================================*/
typedef struct _OpcUa_TransferResult
{
    OpcUa_StatusCode StatusCode;
    OpcUa_Int32      NoOfAvailableSequenceNumbers;
    OpcUa_UInt32*    AvailableSequenceNumbers;
}
OpcUa_TransferResult;

OPCUA_EXPORT OpcUa_Void OpcUa_TransferResult_Initialize(OpcUa_TransferResult* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_TransferResult_Clear(OpcUa_TransferResult* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_TransferResult_GetSize(OpcUa_TransferResult* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_TransferResult_Encode(OpcUa_TransferResult* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_TransferResult_Decode(OpcUa_TransferResult* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_TransferResult_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_TransferSubscriptions
#ifndef OPCUA_EXCLUDE_TransferSubscriptionsRequest
/*============================================================================
 * The TransferSubscriptionsRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_TransferSubscriptionsRequest
{
    OpcUa_RequestHeader RequestHeader;
    OpcUa_Int32         NoOfSubscriptionIds;
    OpcUa_UInt32*       SubscriptionIds;
    OpcUa_Boolean       SendInitialValues;
}
OpcUa_TransferSubscriptionsRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_TransferSubscriptionsRequest_Initialize(OpcUa_TransferSubscriptionsRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_TransferSubscriptionsRequest_Clear(OpcUa_TransferSubscriptionsRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_TransferSubscriptionsRequest_GetSize(OpcUa_TransferSubscriptionsRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_TransferSubscriptionsRequest_Encode(OpcUa_TransferSubscriptionsRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_TransferSubscriptionsRequest_Decode(OpcUa_TransferSubscriptionsRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_TransferSubscriptionsRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_TransferSubscriptionsResponse
/*============================================================================
 * The TransferSubscriptionsResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_TransferSubscriptionsResponse
{
    OpcUa_ResponseHeader  ResponseHeader;
    OpcUa_Int32           NoOfResults;
    OpcUa_TransferResult* Results;
    OpcUa_Int32           NoOfDiagnosticInfos;
    OpcUa_DiagnosticInfo* DiagnosticInfos;
}
OpcUa_TransferSubscriptionsResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_TransferSubscriptionsResponse_Initialize(OpcUa_TransferSubscriptionsResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_TransferSubscriptionsResponse_Clear(OpcUa_TransferSubscriptionsResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_TransferSubscriptionsResponse_GetSize(OpcUa_TransferSubscriptionsResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_TransferSubscriptionsResponse_Encode(OpcUa_TransferSubscriptionsResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_TransferSubscriptionsResponse_Decode(OpcUa_TransferSubscriptionsResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_TransferSubscriptionsResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_DeleteSubscriptions
#ifndef OPCUA_EXCLUDE_DeleteSubscriptionsRequest
/*============================================================================
 * The DeleteSubscriptionsRequest structure.
 *===========================================================================*/
typedef struct _OpcUa_DeleteSubscriptionsRequest
{
    OpcUa_RequestHeader RequestHeader;
    OpcUa_Int32         NoOfSubscriptionIds;
    OpcUa_UInt32*       SubscriptionIds;
}
OpcUa_DeleteSubscriptionsRequest;

OPCUA_EXPORT OpcUa_Void OpcUa_DeleteSubscriptionsRequest_Initialize(OpcUa_DeleteSubscriptionsRequest* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_DeleteSubscriptionsRequest_Clear(OpcUa_DeleteSubscriptionsRequest* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteSubscriptionsRequest_GetSize(OpcUa_DeleteSubscriptionsRequest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteSubscriptionsRequest_Encode(OpcUa_DeleteSubscriptionsRequest* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteSubscriptionsRequest_Decode(OpcUa_DeleteSubscriptionsRequest* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_DeleteSubscriptionsRequest_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_DeleteSubscriptionsResponse
/*============================================================================
 * The DeleteSubscriptionsResponse structure.
 *===========================================================================*/
typedef struct _OpcUa_DeleteSubscriptionsResponse
{
    OpcUa_ResponseHeader  ResponseHeader;
    OpcUa_Int32           NoOfResults;
    OpcUa_StatusCode*     Results;
    OpcUa_Int32           NoOfDiagnosticInfos;
    OpcUa_DiagnosticInfo* DiagnosticInfos;
}
OpcUa_DeleteSubscriptionsResponse;

OPCUA_EXPORT OpcUa_Void OpcUa_DeleteSubscriptionsResponse_Initialize(OpcUa_DeleteSubscriptionsResponse* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_DeleteSubscriptionsResponse_Clear(OpcUa_DeleteSubscriptionsResponse* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteSubscriptionsResponse_GetSize(OpcUa_DeleteSubscriptionsResponse* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteSubscriptionsResponse_Encode(OpcUa_DeleteSubscriptionsResponse* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DeleteSubscriptionsResponse_Decode(OpcUa_DeleteSubscriptionsResponse* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_DeleteSubscriptionsResponse_EncodeableType;
#endif
#endif

#ifndef OPCUA_EXCLUDE_BuildInfo
/*============================================================================
 * The BuildInfo structure.
 *===========================================================================*/
typedef struct _OpcUa_BuildInfo
{
    OpcUa_String   ProductUri;
    OpcUa_String   ManufacturerName;
    OpcUa_String   ProductName;
    OpcUa_String   SoftwareVersion;
    OpcUa_String   BuildNumber;
    OpcUa_DateTime BuildDate;
}
OpcUa_BuildInfo;

OPCUA_EXPORT OpcUa_Void OpcUa_BuildInfo_Initialize(OpcUa_BuildInfo* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_BuildInfo_Clear(OpcUa_BuildInfo* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_BuildInfo_GetSize(OpcUa_BuildInfo* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_BuildInfo_Encode(OpcUa_BuildInfo* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_BuildInfo_Decode(OpcUa_BuildInfo* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_BuildInfo_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_RedundancySupport
/*============================================================================
 * The RedundancySupport enumeration.
 *===========================================================================*/
typedef enum _OpcUa_RedundancySupport
{
    OpcUa_RedundancySupport_None           = 0,
    OpcUa_RedundancySupport_Cold           = 1,
    OpcUa_RedundancySupport_Warm           = 2,
    OpcUa_RedundancySupport_Hot            = 3,
    OpcUa_RedundancySupport_Transparent    = 4,
    OpcUa_RedundancySupport_HotAndMirrored = 5
#if OPCUA_FORCE_INT32_ENUMS
    ,_OpcUa_RedundancySupport_MaxEnumerationValue = OpcUa_Int32_Max
#endif
}
OpcUa_RedundancySupport;

#define OpcUa_RedundancySupport_Clear(xValue) OpcUa_EnumeratedType_Clear(xValue, OpcUa_RedundancySupport_None)

#define OpcUa_RedundancySupport_Initialize(xValue) OpcUa_EnumeratedType_Initialize(xValue, OpcUa_RedundancySupport_None)

OPCUA_IMEXPORT extern struct _OpcUa_EnumeratedType OpcUa_RedundancySupport_EnumeratedType;
#endif

#ifndef OPCUA_EXCLUDE_ServerState
/*============================================================================
 * The ServerState enumeration.
 *===========================================================================*/
typedef enum _OpcUa_ServerState
{
    OpcUa_ServerState_Running            = 0,
    OpcUa_ServerState_Failed             = 1,
    OpcUa_ServerState_NoConfiguration    = 2,
    OpcUa_ServerState_Suspended          = 3,
    OpcUa_ServerState_Shutdown           = 4,
    OpcUa_ServerState_Test               = 5,
    OpcUa_ServerState_CommunicationFault = 6,
    OpcUa_ServerState_Unknown            = 7
#if OPCUA_FORCE_INT32_ENUMS
    ,_OpcUa_ServerState_MaxEnumerationValue = OpcUa_Int32_Max
#endif
}
OpcUa_ServerState;

#define OpcUa_ServerState_Clear(xValue) OpcUa_EnumeratedType_Clear(xValue, OpcUa_ServerState_Running)

#define OpcUa_ServerState_Initialize(xValue) OpcUa_EnumeratedType_Initialize(xValue, OpcUa_ServerState_Running)

OPCUA_IMEXPORT extern struct _OpcUa_EnumeratedType OpcUa_ServerState_EnumeratedType;
#endif

#ifndef OPCUA_EXCLUDE_RedundantServerDataType
/*============================================================================
 * The RedundantServerDataType structure.
 *===========================================================================*/
typedef struct _OpcUa_RedundantServerDataType
{
    OpcUa_String      ServerId;
    OpcUa_Byte        ServiceLevel;
    OpcUa_ServerState ServerState;
}
OpcUa_RedundantServerDataType;

OPCUA_EXPORT OpcUa_Void OpcUa_RedundantServerDataType_Initialize(OpcUa_RedundantServerDataType* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_RedundantServerDataType_Clear(OpcUa_RedundantServerDataType* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RedundantServerDataType_GetSize(OpcUa_RedundantServerDataType* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RedundantServerDataType_Encode(OpcUa_RedundantServerDataType* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_RedundantServerDataType_Decode(OpcUa_RedundantServerDataType* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_RedundantServerDataType_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_EndpointUrlListDataType
/*============================================================================
 * The EndpointUrlListDataType structure.
 *===========================================================================*/
typedef struct _OpcUa_EndpointUrlListDataType
{
    OpcUa_Int32   NoOfEndpointUrlList;
    OpcUa_String* EndpointUrlList;
}
OpcUa_EndpointUrlListDataType;

OPCUA_EXPORT OpcUa_Void OpcUa_EndpointUrlListDataType_Initialize(OpcUa_EndpointUrlListDataType* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_EndpointUrlListDataType_Clear(OpcUa_EndpointUrlListDataType* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EndpointUrlListDataType_GetSize(OpcUa_EndpointUrlListDataType* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EndpointUrlListDataType_Encode(OpcUa_EndpointUrlListDataType* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EndpointUrlListDataType_Decode(OpcUa_EndpointUrlListDataType* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_EndpointUrlListDataType_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_NetworkGroupDataType
/*============================================================================
 * The NetworkGroupDataType structure.
 *===========================================================================*/
typedef struct _OpcUa_NetworkGroupDataType
{
    OpcUa_String                   ServerUri;
    OpcUa_Int32                    NoOfNetworkPaths;
    OpcUa_EndpointUrlListDataType* NetworkPaths;
}
OpcUa_NetworkGroupDataType;

OPCUA_EXPORT OpcUa_Void OpcUa_NetworkGroupDataType_Initialize(OpcUa_NetworkGroupDataType* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_NetworkGroupDataType_Clear(OpcUa_NetworkGroupDataType* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_NetworkGroupDataType_GetSize(OpcUa_NetworkGroupDataType* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_NetworkGroupDataType_Encode(OpcUa_NetworkGroupDataType* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_NetworkGroupDataType_Decode(OpcUa_NetworkGroupDataType* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_NetworkGroupDataType_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_SamplingIntervalDiagnosticsDataType
/*============================================================================
 * The SamplingIntervalDiagnosticsDataType structure.
 *===========================================================================*/
typedef struct _OpcUa_SamplingIntervalDiagnosticsDataType
{
    OpcUa_Double SamplingInterval;
    OpcUa_UInt32 MonitoredItemCount;
    OpcUa_UInt32 MaxMonitoredItemCount;
    OpcUa_UInt32 DisabledMonitoredItemCount;
}
OpcUa_SamplingIntervalDiagnosticsDataType;

OPCUA_EXPORT OpcUa_Void OpcUa_SamplingIntervalDiagnosticsDataType_Initialize(OpcUa_SamplingIntervalDiagnosticsDataType* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_SamplingIntervalDiagnosticsDataType_Clear(OpcUa_SamplingIntervalDiagnosticsDataType* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SamplingIntervalDiagnosticsDataType_GetSize(OpcUa_SamplingIntervalDiagnosticsDataType* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SamplingIntervalDiagnosticsDataType_Encode(OpcUa_SamplingIntervalDiagnosticsDataType* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SamplingIntervalDiagnosticsDataType_Decode(OpcUa_SamplingIntervalDiagnosticsDataType* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_SamplingIntervalDiagnosticsDataType_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ServerDiagnosticsSummaryDataType
/*============================================================================
 * The ServerDiagnosticsSummaryDataType structure.
 *===========================================================================*/
typedef struct _OpcUa_ServerDiagnosticsSummaryDataType
{
    OpcUa_UInt32 ServerViewCount;
    OpcUa_UInt32 CurrentSessionCount;
    OpcUa_UInt32 CumulatedSessionCount;
    OpcUa_UInt32 SecurityRejectedSessionCount;
    OpcUa_UInt32 RejectedSessionCount;
    OpcUa_UInt32 SessionTimeoutCount;
    OpcUa_UInt32 SessionAbortCount;
    OpcUa_UInt32 CurrentSubscriptionCount;
    OpcUa_UInt32 CumulatedSubscriptionCount;
    OpcUa_UInt32 PublishingIntervalCount;
    OpcUa_UInt32 SecurityRejectedRequestsCount;
    OpcUa_UInt32 RejectedRequestsCount;
}
OpcUa_ServerDiagnosticsSummaryDataType;

OPCUA_EXPORT OpcUa_Void OpcUa_ServerDiagnosticsSummaryDataType_Initialize(OpcUa_ServerDiagnosticsSummaryDataType* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ServerDiagnosticsSummaryDataType_Clear(OpcUa_ServerDiagnosticsSummaryDataType* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ServerDiagnosticsSummaryDataType_GetSize(OpcUa_ServerDiagnosticsSummaryDataType* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ServerDiagnosticsSummaryDataType_Encode(OpcUa_ServerDiagnosticsSummaryDataType* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ServerDiagnosticsSummaryDataType_Decode(OpcUa_ServerDiagnosticsSummaryDataType* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ServerDiagnosticsSummaryDataType_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ServerStatusDataType
/*============================================================================
 * The ServerStatusDataType structure.
 *===========================================================================*/
typedef struct _OpcUa_ServerStatusDataType
{
    OpcUa_DateTime      StartTime;
    OpcUa_DateTime      CurrentTime;
    OpcUa_ServerState   State;
    OpcUa_BuildInfo     BuildInfo;
    OpcUa_UInt32        SecondsTillShutdown;
    OpcUa_LocalizedText ShutdownReason;
}
OpcUa_ServerStatusDataType;

OPCUA_EXPORT OpcUa_Void OpcUa_ServerStatusDataType_Initialize(OpcUa_ServerStatusDataType* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ServerStatusDataType_Clear(OpcUa_ServerStatusDataType* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ServerStatusDataType_GetSize(OpcUa_ServerStatusDataType* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ServerStatusDataType_Encode(OpcUa_ServerStatusDataType* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ServerStatusDataType_Decode(OpcUa_ServerStatusDataType* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ServerStatusDataType_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ServiceCounterDataType
/*============================================================================
 * The ServiceCounterDataType structure.
 *===========================================================================*/
typedef struct _OpcUa_ServiceCounterDataType
{
    OpcUa_UInt32 TotalCount;
    OpcUa_UInt32 ErrorCount;
}
OpcUa_ServiceCounterDataType;

OPCUA_EXPORT OpcUa_Void OpcUa_ServiceCounterDataType_Initialize(OpcUa_ServiceCounterDataType* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ServiceCounterDataType_Clear(OpcUa_ServiceCounterDataType* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ServiceCounterDataType_GetSize(OpcUa_ServiceCounterDataType* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ServiceCounterDataType_Encode(OpcUa_ServiceCounterDataType* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ServiceCounterDataType_Decode(OpcUa_ServiceCounterDataType* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ServiceCounterDataType_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_SessionDiagnosticsDataType
/*============================================================================
 * The SessionDiagnosticsDataType structure.
 *===========================================================================*/
typedef struct _OpcUa_SessionDiagnosticsDataType
{
    OpcUa_NodeId                 SessionId;
    OpcUa_String                 SessionName;
    OpcUa_ApplicationDescription ClientDescription;
    OpcUa_String                 ServerUri;
    OpcUa_String                 EndpointUrl;
    OpcUa_Int32                  NoOfLocaleIds;
    OpcUa_String*                LocaleIds;
    OpcUa_Double                 ActualSessionTimeout;
    OpcUa_UInt32                 MaxResponseMessageSize;
    OpcUa_DateTime               ClientConnectionTime;
    OpcUa_DateTime               ClientLastContactTime;
    OpcUa_UInt32                 CurrentSubscriptionsCount;
    OpcUa_UInt32                 CurrentMonitoredItemsCount;
    OpcUa_UInt32                 CurrentPublishRequestsInQueue;
    OpcUa_ServiceCounterDataType TotalRequestCount;
    OpcUa_UInt32                 UnauthorizedRequestCount;
    OpcUa_ServiceCounterDataType ReadCount;
    OpcUa_ServiceCounterDataType HistoryReadCount;
    OpcUa_ServiceCounterDataType WriteCount;
    OpcUa_ServiceCounterDataType HistoryUpdateCount;
    OpcUa_ServiceCounterDataType CallCount;
    OpcUa_ServiceCounterDataType CreateMonitoredItemsCount;
    OpcUa_ServiceCounterDataType ModifyMonitoredItemsCount;
    OpcUa_ServiceCounterDataType SetMonitoringModeCount;
    OpcUa_ServiceCounterDataType SetTriggeringCount;
    OpcUa_ServiceCounterDataType DeleteMonitoredItemsCount;
    OpcUa_ServiceCounterDataType CreateSubscriptionCount;
    OpcUa_ServiceCounterDataType ModifySubscriptionCount;
    OpcUa_ServiceCounterDataType SetPublishingModeCount;
    OpcUa_ServiceCounterDataType PublishCount;
    OpcUa_ServiceCounterDataType RepublishCount;
    OpcUa_ServiceCounterDataType TransferSubscriptionsCount;
    OpcUa_ServiceCounterDataType DeleteSubscriptionsCount;
    OpcUa_ServiceCounterDataType AddNodesCount;
    OpcUa_ServiceCounterDataType AddReferencesCount;
    OpcUa_ServiceCounterDataType DeleteNodesCount;
    OpcUa_ServiceCounterDataType DeleteReferencesCount;
    OpcUa_ServiceCounterDataType BrowseCount;
    OpcUa_ServiceCounterDataType BrowseNextCount;
    OpcUa_ServiceCounterDataType TranslateBrowsePathsToNodeIdsCount;
    OpcUa_ServiceCounterDataType QueryFirstCount;
    OpcUa_ServiceCounterDataType QueryNextCount;
    OpcUa_ServiceCounterDataType RegisterNodesCount;
    OpcUa_ServiceCounterDataType UnregisterNodesCount;
}
OpcUa_SessionDiagnosticsDataType;

OPCUA_EXPORT OpcUa_Void OpcUa_SessionDiagnosticsDataType_Initialize(OpcUa_SessionDiagnosticsDataType* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_SessionDiagnosticsDataType_Clear(OpcUa_SessionDiagnosticsDataType* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SessionDiagnosticsDataType_GetSize(OpcUa_SessionDiagnosticsDataType* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SessionDiagnosticsDataType_Encode(OpcUa_SessionDiagnosticsDataType* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SessionDiagnosticsDataType_Decode(OpcUa_SessionDiagnosticsDataType* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_SessionDiagnosticsDataType_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_SessionSecurityDiagnosticsDataType
/*============================================================================
 * The SessionSecurityDiagnosticsDataType structure.
 *===========================================================================*/
typedef struct _OpcUa_SessionSecurityDiagnosticsDataType
{
    OpcUa_NodeId              SessionId;
    OpcUa_String              ClientUserIdOfSession;
    OpcUa_Int32               NoOfClientUserIdHistory;
    OpcUa_String*             ClientUserIdHistory;
    OpcUa_String              AuthenticationMechanism;
    OpcUa_String              Encoding;
    OpcUa_String              TransportProtocol;
    OpcUa_MessageSecurityMode SecurityMode;
    OpcUa_String              SecurityPolicyUri;
    OpcUa_ByteString          ClientCertificate;
}
OpcUa_SessionSecurityDiagnosticsDataType;

OPCUA_EXPORT OpcUa_Void OpcUa_SessionSecurityDiagnosticsDataType_Initialize(OpcUa_SessionSecurityDiagnosticsDataType* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_SessionSecurityDiagnosticsDataType_Clear(OpcUa_SessionSecurityDiagnosticsDataType* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SessionSecurityDiagnosticsDataType_GetSize(OpcUa_SessionSecurityDiagnosticsDataType* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SessionSecurityDiagnosticsDataType_Encode(OpcUa_SessionSecurityDiagnosticsDataType* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SessionSecurityDiagnosticsDataType_Decode(OpcUa_SessionSecurityDiagnosticsDataType* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_SessionSecurityDiagnosticsDataType_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_StatusResult
/*============================================================================
 * The StatusResult structure.
 *===========================================================================*/
typedef struct _OpcUa_StatusResult
{
    OpcUa_StatusCode     StatusCode;
    OpcUa_DiagnosticInfo DiagnosticInfo;
}
OpcUa_StatusResult;

OPCUA_EXPORT OpcUa_Void OpcUa_StatusResult_Initialize(OpcUa_StatusResult* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_StatusResult_Clear(OpcUa_StatusResult* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_StatusResult_GetSize(OpcUa_StatusResult* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_StatusResult_Encode(OpcUa_StatusResult* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_StatusResult_Decode(OpcUa_StatusResult* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_StatusResult_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_SubscriptionDiagnosticsDataType
/*============================================================================
 * The SubscriptionDiagnosticsDataType structure.
 *===========================================================================*/
typedef struct _OpcUa_SubscriptionDiagnosticsDataType
{
    OpcUa_NodeId  SessionId;
    OpcUa_UInt32  SubscriptionId;
    OpcUa_Byte    Priority;
    OpcUa_Double  PublishingInterval;
    OpcUa_UInt32  MaxKeepAliveCount;
    OpcUa_UInt32  MaxLifetimeCount;
    OpcUa_UInt32  MaxNotificationsPerPublish;
    OpcUa_Boolean PublishingEnabled;
    OpcUa_UInt32  ModifyCount;
    OpcUa_UInt32  EnableCount;
    OpcUa_UInt32  DisableCount;
    OpcUa_UInt32  RepublishRequestCount;
    OpcUa_UInt32  RepublishMessageRequestCount;
    OpcUa_UInt32  RepublishMessageCount;
    OpcUa_UInt32  TransferRequestCount;
    OpcUa_UInt32  TransferredToAltClientCount;
    OpcUa_UInt32  TransferredToSameClientCount;
    OpcUa_UInt32  PublishRequestCount;
    OpcUa_UInt32  DataChangeNotificationsCount;
    OpcUa_UInt32  EventNotificationsCount;
    OpcUa_UInt32  NotificationsCount;
    OpcUa_UInt32  LatePublishRequestCount;
    OpcUa_UInt32  CurrentKeepAliveCount;
    OpcUa_UInt32  CurrentLifetimeCount;
    OpcUa_UInt32  UnacknowledgedMessageCount;
    OpcUa_UInt32  DiscardedMessageCount;
    OpcUa_UInt32  MonitoredItemCount;
    OpcUa_UInt32  DisabledMonitoredItemCount;
    OpcUa_UInt32  MonitoringQueueOverflowCount;
    OpcUa_UInt32  NextSequenceNumber;
    OpcUa_UInt32  EventQueueOverFlowCount;
}
OpcUa_SubscriptionDiagnosticsDataType;

OPCUA_EXPORT OpcUa_Void OpcUa_SubscriptionDiagnosticsDataType_Initialize(OpcUa_SubscriptionDiagnosticsDataType* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_SubscriptionDiagnosticsDataType_Clear(OpcUa_SubscriptionDiagnosticsDataType* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SubscriptionDiagnosticsDataType_GetSize(OpcUa_SubscriptionDiagnosticsDataType* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SubscriptionDiagnosticsDataType_Encode(OpcUa_SubscriptionDiagnosticsDataType* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SubscriptionDiagnosticsDataType_Decode(OpcUa_SubscriptionDiagnosticsDataType* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_SubscriptionDiagnosticsDataType_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ModelChangeStructureVerbMask
/*============================================================================
 * The ModelChangeStructureVerbMask enumeration.
 *===========================================================================*/
typedef enum _OpcUa_ModelChangeStructureVerbMask
{
    OpcUa_ModelChangeStructureVerbMask_NodeAdded        = 1,
    OpcUa_ModelChangeStructureVerbMask_NodeDeleted      = 2,
    OpcUa_ModelChangeStructureVerbMask_ReferenceAdded   = 4,
    OpcUa_ModelChangeStructureVerbMask_ReferenceDeleted = 8,
    OpcUa_ModelChangeStructureVerbMask_DataTypeChanged  = 16
#if OPCUA_FORCE_INT32_ENUMS
    ,_OpcUa_ModelChangeStructureVerbMask_MaxEnumerationValue = OpcUa_Int32_Max
#endif
}
OpcUa_ModelChangeStructureVerbMask;

#define OpcUa_ModelChangeStructureVerbMask_Clear(xValue) OpcUa_EnumeratedType_Clear(xValue, OpcUa_ModelChangeStructureVerbMask_NodeAdded)

#define OpcUa_ModelChangeStructureVerbMask_Initialize(xValue) OpcUa_EnumeratedType_Initialize(xValue, OpcUa_ModelChangeStructureVerbMask_NodeAdded)

OPCUA_IMEXPORT extern struct _OpcUa_EnumeratedType OpcUa_ModelChangeStructureVerbMask_EnumeratedType;
#endif

#ifndef OPCUA_EXCLUDE_ModelChangeStructureDataType
/*============================================================================
 * The ModelChangeStructureDataType structure.
 *===========================================================================*/
typedef struct _OpcUa_ModelChangeStructureDataType
{
    OpcUa_NodeId Affected;
    OpcUa_NodeId AffectedType;
    OpcUa_Byte   Verb;
}
OpcUa_ModelChangeStructureDataType;

OPCUA_EXPORT OpcUa_Void OpcUa_ModelChangeStructureDataType_Initialize(OpcUa_ModelChangeStructureDataType* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ModelChangeStructureDataType_Clear(OpcUa_ModelChangeStructureDataType* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ModelChangeStructureDataType_GetSize(OpcUa_ModelChangeStructureDataType* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ModelChangeStructureDataType_Encode(OpcUa_ModelChangeStructureDataType* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ModelChangeStructureDataType_Decode(OpcUa_ModelChangeStructureDataType* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ModelChangeStructureDataType_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_SemanticChangeStructureDataType
/*============================================================================
 * The SemanticChangeStructureDataType structure.
 *===========================================================================*/
typedef struct _OpcUa_SemanticChangeStructureDataType
{
    OpcUa_NodeId Affected;
    OpcUa_NodeId AffectedType;
}
OpcUa_SemanticChangeStructureDataType;

OPCUA_EXPORT OpcUa_Void OpcUa_SemanticChangeStructureDataType_Initialize(OpcUa_SemanticChangeStructureDataType* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_SemanticChangeStructureDataType_Clear(OpcUa_SemanticChangeStructureDataType* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SemanticChangeStructureDataType_GetSize(OpcUa_SemanticChangeStructureDataType* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SemanticChangeStructureDataType_Encode(OpcUa_SemanticChangeStructureDataType* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_SemanticChangeStructureDataType_Decode(OpcUa_SemanticChangeStructureDataType* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_SemanticChangeStructureDataType_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_Range
/*============================================================================
 * The Range structure.
 *===========================================================================*/
typedef struct _OpcUa_Range
{
    OpcUa_Double Low;
    OpcUa_Double High;
}
OpcUa_Range;

OPCUA_EXPORT OpcUa_Void OpcUa_Range_Initialize(OpcUa_Range* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_Range_Clear(OpcUa_Range* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_Range_GetSize(OpcUa_Range* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_Range_Encode(OpcUa_Range* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_Range_Decode(OpcUa_Range* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_Range_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_EUInformation
/*============================================================================
 * The EUInformation structure.
 *===========================================================================*/
typedef struct _OpcUa_EUInformation
{
    OpcUa_String        NamespaceUri;
    OpcUa_Int32         UnitId;
    OpcUa_LocalizedText DisplayName;
    OpcUa_LocalizedText Description;
}
OpcUa_EUInformation;

OPCUA_EXPORT OpcUa_Void OpcUa_EUInformation_Initialize(OpcUa_EUInformation* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_EUInformation_Clear(OpcUa_EUInformation* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EUInformation_GetSize(OpcUa_EUInformation* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EUInformation_Encode(OpcUa_EUInformation* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_EUInformation_Decode(OpcUa_EUInformation* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_EUInformation_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_AxisScaleEnumeration
/*============================================================================
 * The AxisScaleEnumeration enumeration.
 *===========================================================================*/
typedef enum _OpcUa_AxisScaleEnumeration
{
    OpcUa_AxisScaleEnumeration_Linear = 0,
    OpcUa_AxisScaleEnumeration_Log    = 1,
    OpcUa_AxisScaleEnumeration_Ln     = 2
#if OPCUA_FORCE_INT32_ENUMS
    ,_OpcUa_AxisScaleEnumeration_MaxEnumerationValue = OpcUa_Int32_Max
#endif
}
OpcUa_AxisScaleEnumeration;

#define OpcUa_AxisScaleEnumeration_Clear(xValue) OpcUa_EnumeratedType_Clear(xValue, OpcUa_AxisScaleEnumeration_Linear)

#define OpcUa_AxisScaleEnumeration_Initialize(xValue) OpcUa_EnumeratedType_Initialize(xValue, OpcUa_AxisScaleEnumeration_Linear)

OPCUA_IMEXPORT extern struct _OpcUa_EnumeratedType OpcUa_AxisScaleEnumeration_EnumeratedType;
#endif

#ifndef OPCUA_EXCLUDE_ComplexNumberType
/*============================================================================
 * The ComplexNumberType structure.
 *===========================================================================*/
typedef struct _OpcUa_ComplexNumberType
{
    OpcUa_Float Real;
    OpcUa_Float Imaginary;
}
OpcUa_ComplexNumberType;

OPCUA_EXPORT OpcUa_Void OpcUa_ComplexNumberType_Initialize(OpcUa_ComplexNumberType* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ComplexNumberType_Clear(OpcUa_ComplexNumberType* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ComplexNumberType_GetSize(OpcUa_ComplexNumberType* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ComplexNumberType_Encode(OpcUa_ComplexNumberType* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ComplexNumberType_Decode(OpcUa_ComplexNumberType* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ComplexNumberType_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_DoubleComplexNumberType
/*============================================================================
 * The DoubleComplexNumberType structure.
 *===========================================================================*/
typedef struct _OpcUa_DoubleComplexNumberType
{
    OpcUa_Double Real;
    OpcUa_Double Imaginary;
}
OpcUa_DoubleComplexNumberType;

OPCUA_EXPORT OpcUa_Void OpcUa_DoubleComplexNumberType_Initialize(OpcUa_DoubleComplexNumberType* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_DoubleComplexNumberType_Clear(OpcUa_DoubleComplexNumberType* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DoubleComplexNumberType_GetSize(OpcUa_DoubleComplexNumberType* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DoubleComplexNumberType_Encode(OpcUa_DoubleComplexNumberType* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_DoubleComplexNumberType_Decode(OpcUa_DoubleComplexNumberType* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_DoubleComplexNumberType_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_AxisInformation
/*============================================================================
 * The AxisInformation structure.
 *===========================================================================*/
typedef struct _OpcUa_AxisInformation
{
    OpcUa_EUInformation        EngineeringUnits;
    OpcUa_Range                EURange;
    OpcUa_LocalizedText        Title;
    OpcUa_AxisScaleEnumeration AxisScaleType;
    OpcUa_Int32                NoOfAxisSteps;
    OpcUa_Double*              AxisSteps;
}
OpcUa_AxisInformation;

OPCUA_EXPORT OpcUa_Void OpcUa_AxisInformation_Initialize(OpcUa_AxisInformation* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_AxisInformation_Clear(OpcUa_AxisInformation* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AxisInformation_GetSize(OpcUa_AxisInformation* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AxisInformation_Encode(OpcUa_AxisInformation* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_AxisInformation_Decode(OpcUa_AxisInformation* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_AxisInformation_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_XVType
/*============================================================================
 * The XVType structure.
 *===========================================================================*/
typedef struct _OpcUa_XVType
{
    OpcUa_Double X;
    OpcUa_Float  Value;
}
OpcUa_XVType;

OPCUA_EXPORT OpcUa_Void OpcUa_XVType_Initialize(OpcUa_XVType* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_XVType_Clear(OpcUa_XVType* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_XVType_GetSize(OpcUa_XVType* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_XVType_Encode(OpcUa_XVType* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_XVType_Decode(OpcUa_XVType* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_XVType_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ProgramDiagnosticDataType
/*============================================================================
 * The ProgramDiagnosticDataType structure.
 *===========================================================================*/
typedef struct _OpcUa_ProgramDiagnosticDataType
{
    OpcUa_NodeId       CreateSessionId;
    OpcUa_String       CreateClientName;
    OpcUa_DateTime     InvocationCreationTime;
    OpcUa_DateTime     LastTransitionTime;
    OpcUa_String       LastMethodCall;
    OpcUa_NodeId       LastMethodSessionId;
    OpcUa_Int32        NoOfLastMethodInputArguments;
    OpcUa_Argument*    LastMethodInputArguments;
    OpcUa_Int32        NoOfLastMethodOutputArguments;
    OpcUa_Argument*    LastMethodOutputArguments;
    OpcUa_DateTime     LastMethodCallTime;
    OpcUa_StatusResult LastMethodReturnStatus;
}
OpcUa_ProgramDiagnosticDataType;

OPCUA_EXPORT OpcUa_Void OpcUa_ProgramDiagnosticDataType_Initialize(OpcUa_ProgramDiagnosticDataType* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ProgramDiagnosticDataType_Clear(OpcUa_ProgramDiagnosticDataType* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ProgramDiagnosticDataType_GetSize(OpcUa_ProgramDiagnosticDataType* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ProgramDiagnosticDataType_Encode(OpcUa_ProgramDiagnosticDataType* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ProgramDiagnosticDataType_Decode(OpcUa_ProgramDiagnosticDataType* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ProgramDiagnosticDataType_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ProgramDiagnostic2DataType
/*============================================================================
 * The ProgramDiagnostic2DataType structure.
 *===========================================================================*/
typedef struct _OpcUa_ProgramDiagnostic2DataType
{
    OpcUa_NodeId       CreateSessionId;
    OpcUa_String       CreateClientName;
    OpcUa_DateTime     InvocationCreationTime;
    OpcUa_DateTime     LastTransitionTime;
    OpcUa_String       LastMethodCall;
    OpcUa_NodeId       LastMethodSessionId;
    OpcUa_Int32        NoOfLastMethodInputArguments;
    OpcUa_Argument*    LastMethodInputArguments;
    OpcUa_Int32        NoOfLastMethodOutputArguments;
    OpcUa_Argument*    LastMethodOutputArguments;
    OpcUa_Int32        NoOfLastMethodInputValues;
    OpcUa_Variant*     LastMethodInputValues;
    OpcUa_Int32        NoOfLastMethodOutputValues;
    OpcUa_Variant*     LastMethodOutputValues;
    OpcUa_DateTime     LastMethodCallTime;
    OpcUa_StatusResult LastMethodReturnStatus;
}
OpcUa_ProgramDiagnostic2DataType;

OPCUA_EXPORT OpcUa_Void OpcUa_ProgramDiagnostic2DataType_Initialize(OpcUa_ProgramDiagnostic2DataType* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_ProgramDiagnostic2DataType_Clear(OpcUa_ProgramDiagnostic2DataType* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ProgramDiagnostic2DataType_GetSize(OpcUa_ProgramDiagnostic2DataType* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ProgramDiagnostic2DataType_Encode(OpcUa_ProgramDiagnostic2DataType* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_ProgramDiagnostic2DataType_Decode(OpcUa_ProgramDiagnostic2DataType* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_ProgramDiagnostic2DataType_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_Annotation
/*============================================================================
 * The Annotation structure.
 *===========================================================================*/
typedef struct _OpcUa_Annotation
{
    OpcUa_String   Message;
    OpcUa_String   UserName;
    OpcUa_DateTime AnnotationTime;
}
OpcUa_Annotation;

OPCUA_EXPORT OpcUa_Void OpcUa_Annotation_Initialize(OpcUa_Annotation* pValue);

OPCUA_EXPORT OpcUa_Void OpcUa_Annotation_Clear(OpcUa_Annotation* pValue);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_Annotation_GetSize(OpcUa_Annotation* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_Annotation_Encode(OpcUa_Annotation* pValue, struct _OpcUa_Encoder* pEncoder);

OPCUA_EXPORT OpcUa_StatusCode OpcUa_Annotation_Decode(OpcUa_Annotation* pValue, struct _OpcUa_Decoder* pDecoder);

OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType OpcUa_Annotation_EncodeableType;
#endif

#ifndef OPCUA_EXCLUDE_ExceptionDeviationFormat
/*============================================================================
 * The ExceptionDeviationFormat enumeration.
 *===========================================================================*/
typedef enum _OpcUa_ExceptionDeviationFormat
{
    OpcUa_ExceptionDeviationFormat_AbsoluteValue    = 0,
    OpcUa_ExceptionDeviationFormat_PercentOfValue   = 1,
    OpcUa_ExceptionDeviationFormat_PercentOfRange   = 2,
    OpcUa_ExceptionDeviationFormat_PercentOfEURange = 3,
    OpcUa_ExceptionDeviationFormat_Unknown          = 4
#if OPCUA_FORCE_INT32_ENUMS
    ,_OpcUa_ExceptionDeviationFormat_MaxEnumerationValue = OpcUa_Int32_Max
#endif
}
OpcUa_ExceptionDeviationFormat;

#define OpcUa_ExceptionDeviationFormat_Clear(xValue) OpcUa_EnumeratedType_Clear(xValue, OpcUa_ExceptionDeviationFormat_AbsoluteValue)

#define OpcUa_ExceptionDeviationFormat_Initialize(xValue) OpcUa_EnumeratedType_Initialize(xValue, OpcUa_ExceptionDeviationFormat_AbsoluteValue)

OPCUA_IMEXPORT extern struct _OpcUa_EnumeratedType OpcUa_ExceptionDeviationFormat_EnumeratedType;
#endif

/*============================================================================
 * Table of known types.
 *===========================================================================*/
OPCUA_IMEXPORT extern struct _OpcUa_EncodeableType** OpcUa_KnownEncodeableTypes;

OPCUA_END_EXTERN_C

#endif
/* This is the last line of an autogenerated file. */
