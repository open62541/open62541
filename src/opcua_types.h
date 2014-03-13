/*
 * opcua_types.h
 *
 *  Created on: Jan 07, 2014
 *      Author: opcua
 */

#ifndef OPCUA_TYPES_H_
#define OPCUA_TYPES_H_

#include "opcua_builtInDatatypes.h"


typedef enum
{
	ApplicationType_SERVER_0,
	ApplicationType_CLIENUA_1,
	ApplicationType_CLIENTANDSERVER_2,
	ApplicationType_DISCOVERYSERVER_3
}
UA_AD_ApplicationType;


/**
* Index
* Part: 4
* Chapter: 7.12
* Page: 118
*/
typedef UInt32 UA_AD_Index;


/**
* IntegerId
* Part: 4
* Chapter: 7.13
* Page: 118
*/
typedef UInt32 UA_AD_IntegerId;


/**
* ApplicationDescription
* Part: 4
* Chapter: 7.1
* Page: 103
*/
typedef struct
{
	UA_String applicationUri;
	UA_String productUri;
	UA_LocalizedText applicationName;
	UA_AD_ApplicationType applicationType;
	UA_String gatewayServerUri;
	UA_String discoveryProfileUri;
	UInt16 noOfDiscoerUrls;
	UA_String* discoverUrls;
}
UA_AD_ApplicationDescription;

/**
* ApplicationInstanceCertificate
* Part: 4
* Chapter: 7.2
* Page: 104
*/
typedef struct
{
	UA_String version;
	UA_ByteString serialNumber;
	UA_String signatureAlgorithm;
	UA_ByteString signature;
//ToDo	struct issuer				//????????????? S. 108 table 104
	UA_DateTime validFrom;
	UA_DateTime valdiTo;
//ToDo	struct subject;				//????????????? S. 108 table 104
	UA_String applicationUri;
	UInt16 noOfHostnames;
	UA_String* hostnames;
	UA_ByteString pubicKey;
	UInt16 noOfKeyUsages;
	UA_String* keyUsage;
}
UA_AD_ApplicationInstanceCertificate;


/**
* ContinuationPoint 				//ToDo
* Part: 4
* Chapter: 7.6
* Page: 114
*/
typedef struct UA_AD_ContinuationPoint
{
}
UA_AD_ContinuationPoint;


/**
* ReferenceDescription
* Part: 4
* Chapter: 7.24
* Page: 131
*/
typedef struct
{
	UA_NodeId referenceTypeId;
	Boolean isForward;
	UA_ExpandedNodeId nodeId;
	UA_QualifiedName browseName;
	UA_LocalizedText displayName;
//ToDo	UA_NodeClass nodeClass;			//ToDo
	UA_ExpandedNodeId typeDefinition;
}
UA_AD_ReferenceDescription;


/**
* BrowseResult
* Part: 4
* Chapter: 7.3
* Page: 104
*/
typedef struct
{
	UA_StatusCode statusCode;
	UA_AD_ContinuationPoint continuationPoint;
	UInt16 noOfReferences;
	UA_AD_ReferenceDescription *references;
}
UA_AD_BrowseResult;


/**
* Counter
* Part: 4
* Chapter: 7.5
* Page: 113
*/
typedef UInt32 UA_Counter;


/**
* DataValue
* Part: 4
* Chapter: 7.7.1
* Page: 114
*/
typedef struct UA_AD_DataValue
{
	UA_Variant value;	// BaseDataType are mapped to a UA_Variant -> part: 6 chapter: 5.1.5 page: 14
	UA_StatusCode statusCode;
	UA_DateTime sourceTimestamp;
//ToDo	UInt					//toBeDiscussed: Resolution of PicoSeconds
	UA_DateTime serverTimestamp;
//ToDo	UInt					//toBeDiscussed: Resolution of PicoSeconds
}UA_AD_DataValue;


/**
* DiagnosticInfo
* Part: 4
* Chapter: 7.9
* Page: 116
*/
typedef struct
{
	//ToDo	struct ???? identifier;	identifier is only needed for encoding????
	Int32 namespaceUri;
	Int32 symbolicId;
	Int32 locale;
	Int32 localizesText;
	UA_String additionalInfo;
	UA_StatusCode innerStatusCode;
	struct _UA_DiagnosticInfo* innerDiagnosticInfo;
}
UA_AD_DiagnosticInfo;


/**
* MessageSecurityMode
* Part: 4
* Chapter: 7.14
* Page: 118
*/
typedef enum
{
	MessageSecurityMode_INVALID_0 = 0,
	MessageSecurityMode_SIGN_1 = 1,
	MessageSecurityMode_SIGNANDENCRYPUA_2 = 2
}
UA_AD_MessageSecurityMode;


/**
* UserTokenPolicy
* Part: 4
* Chapter: 7.36
* Page: 142
*/
typedef struct
{
	UA_String policyId;
//ToDo	UA_UserIdentityTokenType tokenType;
	UA_String issuedTokenType;
	UA_String issuerEndpointUrl;
	UA_String securityPolicyUri;
}
UA_AD_UserTokenPolicy;


/**
* EndpointDescription
* Part: 4
* Chapter: 7.9
* Page: 116
*/
typedef struct
{
	UA_String endpointUrl;
	UA_AD_ApplicationDescription server;
	UA_AD_ApplicationInstanceCertificate serverCertificate;
	UA_AD_MessageSecurityMode securityMode;
	UA_String securityPolicyUri;
	UInt16 noOfUserIdentyTokens;
	UA_AD_UserTokenPolicy* useridentyTokens;
	UA_String transportProfileUri;
	Byte securtiyLevel;
}
UA_AD_EndpointDescription;


/**
* ExpandedNodeId
* Part: 4
* Chapter: 7.10
* Page: 117
*/
typedef struct
{
	UA_AD_Index serverIndex;
	UA_String namespaceUri;
	UA_AD_Index namespaveIndex;
//ToDo	enum BED_IdentifierType identiferType;		//ToDo: Is the enumeration correct?
//ToDo	UA_NodeIdentifier identifier;		//ToDo -> Part 3: Address Space Model
}
UA_AD_ExpandedNodeId;


/**
* ExtensibleParameter
* Part: 4
* Chapter: 7.11
* Page: 117
*/
typedef struct
{
	UA_NodeId parameterTypeId;
//ToDo	-- parameterData;			//toBeDiscussed
}
UA_AD_ExtensibleParameter;


/**
* DataChangeFilter
* Part: 4
* Chapter: 7.16.2
* Page: 119
*/
typedef struct
{
//ToDo	enum BED_MonitoringFilter trigger = BED_MonitoringFilter.DATA_CHANGE_FILTER;
	UInt32 deadbandType;
	Double deadbandValue;
}
UA_AD_DataChangeFilter;


/**
* ContentFilter 						//ToDo
* Part: 4
* Chapter: 7.4.1
* Page: 104
*/
typedef struct
{
//ToDo	struct BED_ContentFilterElement elements[];		//ToDo
//ToDo	enum BED_FilterOperand filterOperator;			//ToDo table 110
//ToDo	struct BED_ExtensibleParamterFilterOperand filterOperands[]; //ToDo 7.4.4
}
UA_AD_ContentFilter;


/**
* EventFilter
* Part: 4
* Chapter: 7.16.3
* Page: 120
*/
typedef struct
{
//ToDo	SimpleAttributeOperantd selectClauses[]; 		//ToDo
//ToDo	ContenFilter whereClause;				//ToDo
}
UA_AD_EventFilter;

typedef struct
{
	UA_StatusCode selectClauseResults[3];
	UA_DiagnosticInfo selectClauseDiagnosticInfos[3];
//ToDo	UA_ContentFilterResult whereClauseResult;
}
UA_AD_EventFilterResult;


/**
* AggregateFilter
* Part: 4
* Chapter: 7.16.4
* Page: 122
*/
typedef struct
{
	UA_DateTime startTime;
	UA_NodeId aggregateType;
	UA_Duration processingInterval;
//ToDo	AggregateConfiguration aggregateConfiguration;		//ToDo
	Boolean useServerCapabilitiesDafaults;
	Boolean treatUncertainAsBad;
	Byte percentDataBad;
	Byte percentDataGood;
	Boolean steppedSlopedExtrapolation;
}
UA_AD_AggregateFilter;

typedef struct
{
	UA_DateTime revisedStartTime;
	UA_Duration revisedProcessingInterval;
}
UA_AD_AggregateFilterResult;



/**
* MonitoringParameters
* Part: 4
* Chapter: 7.15
* Page: 118
*/
typedef struct
{
	UA_AD_IntegerId clientHandle;
//ToDo	Duration???? samplingInterval;						//ToDo
//ToDo	struct BED_ExtensibleParameterMonitoringFilter filter			//ToDo
	UA_Counter queueSize;
	Boolean discardOldest;
}
UA_AD_MonitoringParameters;

//->ExtensibleParameter ->Part:4 Chapter:7.11 Page:117
typedef struct 		//ToDo: Ist die Umsetzung des ExtensibleParameter korrekt?
{
//ToDo	UA_MonitoringFilter parameterTypeId;
	UA_AD_DataChangeFilter dataChangeFilter;
	UA_AD_EventFilter eventFilter;
	UA_AD_AggregateFilter aggregateFilter;
}
UA_AD_ExtensibleParameterMonitoringFilter;

/**
* MonitoringFilter parameterTypeIds
* Part: 4
* Chapter: 7.16.1
* Page: 119
*/
typedef enum
{
	MonitoringFilter_DATA_CHANGE_FILTER = 1,
	MonitoringFilter_EVENUA_FILTER = 2,
	MonitoringFilter_AGGREGATE_FILTER = 3
}
UA_AD_MonitoringFilter;




/**
* MonitoringMode
* Part: 4
* Chapter: 7.17
* Page: 123
*/
typedef enum
{
	MonitoringModeValues_DISABLED_0 = 0,	//The item being monitored is not sampled or evaluated, and Notifications are not generated or queued. Notification reporting is disabled.
	MonitoringModeValues_SAMPLING_1 = 1,	//The item being monitored is sampled and evaluated, and Notifications are generated and queued. Notification reporting is disabled.
	MonitoringModeValues_REPORTING_2 = 2	//The item being monitored is sampled and evaluated, and Notifications are generated and queued. Notification reporting is enabled.
}
UA_AD_MonitoringModeValues;


/**
* NodeAttributes parameters
* Part: 4
* Chapter: 7.18.1
* Page: 124
*/
typedef enum
{
	NodeAttributesParamterTypeIds_ObjectAttributes,	//Defines the Attributes for the Object NodeClass.
	NodeAttributesParamterTypeIds_VariableAttributes,	//Defines the Attributes for the Variable NodeClass.
	NodeAttributesParamterTypeIds_MethodAttributes,	//Defines the Attributes for the Method NodeClass.
	NodeAttributesParamterTypeIds_ObjectTypeAttributes,	//Defines the Attributes for the ObjectType NodeClass.
	NodeAttributesParamterTypeIds_VariableTypeAttributes,	//Defines the Attributes for the VariableType NodeClass.
	NodeAttributesParamterTypeIds_ReferenceTypeAttributes,//Defines the Attributes for the ReferenceType NodeClass.
	NodeAttributesParamterTypeIds_DataTypeAttributes,	//Defines the Attributes for the DataType NodeClass.
	NodeAttributesParamterTypeIds_ViewAttributes		//Defines the Attributes for the View NodeClass.
}
UA_AD_NodeAttributesParamterTypeIds;

typedef enum
{
	NodeAttributesBitMask_AccessLevel = 1, 	//Bit: 0 Indicates if the AccessLevel Attribute is set.
	NodeAttributesBitMask_ArrayDimensions = 2,	//Bit: 1 Indicates if the ArrayDimensions Attribute is set.
	//Reserved = 4, 	//Bit: 2 Reserved to be consistent with WriteMask defined in IEC 62541-3.
	NodeAttributesBitMask_ContainsNoLoops = 8,	//Bit: 3 Indicates if the ContainsNoLoops Attribute is set.
	NodeAttributesBitMask_DataType = 16,		//Bit: 4 Indicates if the DataType Attribute is set.
	NodeAttributesBitMask_Description = 32,	//Bit: 5 Indicates if the Description Attribute is set.
	NodeAttributesBitMask_DisplayName = 64,	//Bit: 6 Indicates if the DisplayName Attribute is set.
	NodeAttributesBitMask_EventNotifier = 128,	//Bit: 7 Indicates if the EventNotifier Attribute is set.
	NodeAttributesBitMask_Executable = 256,	//Bit: 8 Indicates if the Executable Attribute is set.
	NodeAttributesBitMask_Historizing = 512,	//Bit: 9 Indicates if the Historizing Attribute is set.
	NodeAttributesBitMask_InverseName = 1024,	//Bit:10 Indicates if the InverseName Attribute is set.
	NodeAttributesBitMask_IsAbstract = 2048,	//Bit:11 Indicates if the IsAbstract Attribute is set.
	NodeAttributesBitMask_MinimumSamplingInterval = 4096, //Bit:12 Indicates if the MinimumSamplingInterval Attribute is set.
	//Reserved = 8192,	//Bit:13 Reserved to be consistent with WriteMask defined in IEC 62541-3.
	//Reserved = 16384,	//Bit:14 Reserved to be consistent with WriteMask defined in IEC 62541-3.
	NodeAttributesBitMask_Symmetric = 32768,	//Bit:15 Indicates if the Symmetric Attribute is set.
	NodeAttributesBitMask_UserAccessLevel = 65536,//Bit:16 Indicates if the UserAccessLevel Attribute is set.
	NodeAttributesBitMask_UserExecutable = 131072,//Bit:17 Indicates if the UserExecutable Attribute is set.
	NodeAttributesBitMask_UserWriteMask = 262144, //Bit:18 Indicates if the UserWriteMask Attribute is set.
	NodeAttributesBitMask_ValueRank = 524288,	//Bit:19 Indicates if the ValueRank Attribute is set.
	NodeAttributesBitMask_WriteMask = 1048576,	//Bit:20 Indicates if the WriteMask Attribute is set.
	NodeAttributesBitMask_Value = 2097152		//Bit:21 Indicates if the Value Attribute is set.
	//Reserved		//Bit:22:32 Reserved for future use. Shall always be zero.
}
UA_AD_NodeAttributesBitMask;


/**
* ObjectAttributes parameters
* Part: 4
* Chapter: 7.18.2
* Page: 125
*/
typedef struct
{
	UInt32 specifiedAttribute;	//BitMask corresponding to BED_NodeAttributesBitMask
	UA_LocalizedText displayName;
	UA_LocalizedText description;
	Byte eventNotifier;
	UInt32 writeMask;
	UInt32 userWriteMask;
}
UA_AD_ObjectAttributes;


/**
* VariableAttributes parameters
* Part: 4
* Chapter: 7.18.3
* Page: 125
*/
typedef struct
{
	UInt32 specifiedAttributes; 	//BitMask corresponding to BED_NodeAttributesBitMask
	UA_LocalizedText displayName;
	UA_LocalizedText description;
//ToDo	DefinedByTheDataTypeAttribte??? value				//ToDo
	UA_NodeId dataType;
	Int32 valueRank;
	UInt16 noOfArrayDimensions;
	UInt32* arrayDimensions;
	Byte accessLevel;
	Byte userAccesLevel;
//ToDo	Duration???? minimumSamplingInterval;			//ToDo
	Boolean historizing;
	UInt32 writeMask;
	UInt32 userWriteMask;
}
UA_AD_VariableAttributes;


/**
* MethodAttributes parameters
* Part: 4
* Chapter: 7.18.4
* Page: 125
*/
typedef struct
{
	UInt32 specifiedAttributes;	//BitMask corresponding to BED_NodeAttributesBitMask
	UA_LocalizedText displayName;
	UA_LocalizedText description;
	Boolean executable;
	Boolean userExecutable;
	UInt32 writeMask;
	UInt32 userWriteMask;
}
UA_AD_MethodAttributes;


/**
* ObjectTypeAttributes parameters
* Part: 4
* Chapter: 7.18.5
* Page: 125
*/
typedef struct
{
	UInt32 specifiedAttributes; 	//BitMask corresponding to BED_NodeAttributesBitMask
	UA_LocalizedText displayName;
	UA_LocalizedText description;
	Boolean isAbstract;
	UInt32 wirteMask;
	UInt32 userWriteMask;
}
UA_AD_ObjectTypeAttributes;


/**
* VariableTypeAttributes parameters
* Part: 4
* Chapter: 7.18.6
* Page: 126
*/
typedef struct
{
	UInt32 specifiedAttributes; 	//BitMask corresponding to BED_NodeAttributesBitMask
	UA_LocalizedText displayName;
	UA_LocalizedText description;
//ToDo	DefinedByTheDataTypeAttribte??? value			//ToDo
	UA_NodeId dataType;
	Int32 valueRank;
	UInt16 noOfArrayDimensions;
	UInt32* arrayDimesions;
	Boolean isAbstract;
	UInt32 writeMask;
	UInt32 userWriteMask;
}
UA_AD_VariableTypeAttributes;


/**
* ReferenceTypeAttributes parameters
* Part: 4
* Chapter: 7.18.7
* Page: 126
*/
typedef struct
{
	UInt32 specifiedAttributes; 	//BitMask corresponding to BED_NodeAttributesBitMask
	UA_LocalizedText displayName;
	UA_LocalizedText description;
	Boolean isAbstract;
	Boolean symmetric;
	UA_LocalizedText inverseName;
	UInt32 writeMask;
	UInt32 userWriteMask;
}
UA_AD_ReferenceTypeAttributes;



/**
* DataTypeAttributes parameters
* Part: 4
* Chapter: 7.18.8
* Page: 126
*/
typedef struct
{
	UInt32 specifiedAttributes; 	//BitMask corresponding to BED_NodeAttributesBitMask
	UA_LocalizedText displayName;
	UA_LocalizedText description;
	Boolean isAbstract;
	UInt32 writeMask;
	UInt32 userWriteMask;
}
UA_AD_DataTypeAttributes;



/**
* ViewAttributes parameters
* Part: 4
* Chapter: 7.18.9
* Page: 127
*/
typedef struct
{
	UInt32 specifiedAttributes; 	//BitMask corresponding to BED_NodeAttributesBitMask
	UA_LocalizedText displayName;
	UA_LocalizedText description;
	Boolean containsNoLoops;
	Byte eventNotifier;
	UInt32 writeMask;
	UInt32 userWriteMask;
}
UA_AD_ViewAttributes;


/**
* NotificationData parameters
* Part: 4
* Chapter: 7.19
* Page: 127
*/
typedef enum
{
	NotificationDataParameterTypeIds_DATA_CHANGE = 1,
	NotificationDataParameterTypeIds_EVENT = 2,
	NotificationDataParameterTypeIds_STATUS_CHANGE = 3
}
UA_AD_NotificationDataParameterTypeIds;


/**
* DataChangeNotification parameter
* Part: 4
* Chapter: 7.19.2
* Page: 127
*/
typedef struct UA_DataChangeNotification
{
//ToDo	struct BED_MonitoredItemNotification monitoredItems[];		//ToDo
	UA_AD_IntegerId clientHandle;
	UA_DataValue value;
	UInt16 noOfdiagnosticInfos;
	UA_DiagnosticInfo* diagnosticInfos;
}
UA_AD_DataChangeNotification;


/**
* EventNotificationList parameter
* Part: 4
* Chapter: 7.19.3
* Page: 128
*/
typedef struct
{
//ToDo	struct EventFieldList events[];			//ToDo
	UA_AD_IntegerId clientHandle;
	UInt16 noOfEventFields;
	UA_Variant* eventFields;	// BaseDataType are mapped to a UA_Variant -> part: 6 chapter: 5.1.5 page: 14
}
UA_AD_EventNotificationList;


/**
* StatusChangeNotification parameter
* Part: 4
* Chapter: 7.19.4
* Page: 128
*/
typedef struct
{
	UA_StatusCode status;
	UA_DiagnosticInfo diagnosticInfo;
}
UA_AD_StatusChangeNotification;


/**
* NotificationMessage
* Part: 4
* Chapter: 7.20
* Page: 129
*/
//->ExtensibleParameter ->Part:4 Chapter:7.11 Page:117
typedef struct 		//ToDo: Ist die Umsetzung des ExtensibleParameter korrekt?
{
	UA_AD_NotificationDataParameterTypeIds parameterTypeId;
	UA_AD_DataChangeNotification dataChange;
	UA_AD_EventNotificationList event;
	UA_AD_StatusChangeNotification statusChange;
}
UA_AD_ExtensibleParameterNotificationData;

typedef struct
{
	UA_Counter sequenceNumber;
	UA_DateTime publishTime;
	UInt16 noOfNotificationData;
	UA_AD_ExtensibleParameterNotificationData* notificationData;
}
UA_AD_NotificationMessage;




/**
* NumericRange
* Part: 4
* Chapter: 7.21
* Page: 129
*/
typedef UA_String UA_NumericRange;


/**
* QueryDataSet
* Part: 4
* Chapter: 7.22
* Page: 130
*/
typedef struct
{
	UA_ExpandedNodeId nodeId;
	UA_ExpandedNodeId typeDefinitionNode;
	UInt16 noOfValues;
	UA_Variant* values;	// BaseDataType are mapped to a UA_Variant -> part: 6 chapter: 5.1.5 page: 14
}
UA_AD_QueryDataSet;


/**
* ReadValueId
* Part: 4
* Chapter: 7.23
* Page: 130
*/
typedef struct
{
	UA_NodeId nodeId;
	UA_AD_IntegerId attributeId;
	UA_NumericRange indexRange;
	UA_QualifiedName dataEncoding;
}
UA_AD_ReadValueId;


/**
* RelativePath
* Part: 4
* Chapter: 7.25
* Page: 131
*/
typedef struct
{
//ToDo	struct BED_RelativePathElement elements[];		//ToDo
	UA_NodeId referenceTypeId;
	Boolean isInverse;
	Boolean includeSubtypes;
	UA_QualifiedName targetName;
}
UA_AD_RelativePath;


/**
* RequestHeader
* Part: 4
* Chapter: 7.26
* Page: 132
*/
typedef struct
{
	UA_NodeId authenticationToken;
	UA_DateTime timestamp;
	UA_AD_IntegerId requestHandle;
	UInt32 returnDiagnostics;
	UA_String auditEntryId;
	UInt32 timeoutHint;
	UA_ExtensionObject additionalHeader;
}
UA_AD_RequestHeader;


typedef enum
{
	RequestReturnDiagnositcs_SERVICE_LEVEL_SYMBOLIC_ID = 1,				//Hex 0x01
	RequestReturnDiagnositcs_SERVICE_LEVEL_LOCALIZED_TEXT= 2,			//Hex 0x02
	RequestReturnDiagnositcs_SERVICE_LEVEL_ADDITIONAL_INFO = 4,			//Hex 0x04
	RequestReturnDiagnositcs_SERVICE_LEVEL_INNER_STATUS_CODE = 8,		//Hex 0x08
	RequestReturnDiagnositcs_SERVICE_LEVEL_INNER_DIAGNOSTICS = 16,		//Hex 0x10
	RequestReturnDiagnositcs_OPERATION_LEVEL_SYMBOLIC_ID = 32,			//Hex 0x20
	RequestReturnDiagnositcs_OPERATION_LEVEL_LOCALIZED_TEXT= 64,			//Hex 0x40
	RequestReturnDiagnositcs_OPERATION_LEVEL_ADDITIONAL_INFO = 128,		//Hex 0x80
	RequestReturnDiagnositcs_OPERATION_LEVEL_INNER_STATUS_CODE = 256,	//Hex 0x100
	RequestReturnDiagnositcs_OPERATION_LEVEL_INNER_DIAGNOSTICS = 512		//Hex 0x200
}
UA_AD_RequestReturnDiagnositcs;


/**
* ResponseHeader
* Part: 4
* Chapter: 7.27
* Page: 133
*/
typedef struct UA_AD_ResponseHeader
{
	UA_DateTime timestamp;
	UA_AD_IntegerId requestHandle;
	UA_StatusCode serviceResult;
	UA_DiagnosticInfo *serviceDiagnostics;
	Int16 noOfStringTable;		// Unsigned: -1 == Empty Array
	UA_String** stringTable;	//  this is an array of strings, i.e. String** or String* ... []
	UA_ExtensionObject *additionalHeader;
}
UA_AD_ResponseHeader;


/**
* ServiceFault
* Part: 4
* Chapter: 7.28
* Page: 133
*/
typedef struct
{
	UA_AD_ResponseHeader responseHeader;
}
UA_AD_ServiceFault;


//ToDo: Own DataType with typeDef?
/**
* SessionAuthenticationToken
* Part: 4
* Chapter: 7.29
* Page: 133
*/



/**
* SignatureData
* Part: 4
* Chapter: 7.30
* Page: 135
*/
typedef struct
{
	UA_ByteString signature;
	UA_String agorithm;
}
UA_AD_SignatureData;


/**
* SignedSoftwareCertificate
* Part: 4
* Chapter: 7.31
* Page: 135
*/
typedef struct
{
	UA_String version;
	UA_ByteString serialNumber;
	UA_String signatureAlgorithm;
	UA_ByteString signature;
//ToDo	struct issuer 					//ToDo: ??? struct?
	UA_DateTime validFrom;
	UA_DateTime validTo;
//ToDo	struct subject;					//ToDo: ??? struct?
//ToDo	struct subjectAltName[];			//ToDo: ??? struct?
	UA_ByteString publicKey;
	UInt16 noOfKeyUsage;
	UA_String* keyUsage;
	UA_ByteString softwareCertificate;
}
UA_AD_SignedSoftwareCertificate;


/**
* SoftwareCertificate
* Part: 4
* Chapter: 7.32
* Page: 135
*/
typedef enum
{
	ComplianceLevel_UNTESTED_0 = 0,		//the profiled capability has not been tested successfully.
	ComplianceLevel_PARTIAL_1 = 1,		//the profiled capability has been partially tested and has
				//passed critical tests, as defined by the certifying authority.
	ComplianceLevel_SELFTESTED_2 = 2,	//the profiled capability has been successfully tested using a
				//self-test system authorized by the certifying authority.
	ComplianceLevel_CERTIFIED_3 = 3		//the profiled capability has been successfully tested by a
				//testing organisation authorized by the certifying authority.
}
UA_AD_ComplianceLevel;

typedef struct
{
	UA_String oranizationUri;
	UA_String profileId;
	UA_String complianceTool;
	UA_DateTime complianceDate;
	UA_AD_ComplianceLevel complianceLevel;
	UInt16 noOfUnsupportedUnitIds;
	UA_String* unsupportedUnitIds;
}
UA_AD_SupportedProfiles;

typedef struct
{
	UA_String productName;
	UA_String productUri;
	UA_String vendorName;
	UA_ByteString vendorProductCertificate;
	UA_String softwareVersion;
	UA_String buildNumber;
	UA_DateTime buildDate;
	UA_String issuedBy;
	UA_DateTime issueDate;
//ToDo	UA_ByteString vendorProductCertificate; //Name is double in this struct
	UA_AD_SupportedProfiles supportedProfiles;
}
UA_AD_SoftwareCertificate;


/**
* StatusCode					//ToDo: Do we need them? How do we implement them (enum)?
* Part: 4
* Chapter: 7.33
* Page: 136
*/


/**
* TimestampsToReturn
* Part: 4
* Chapter: 7.34
* Page: 140
*/
typedef enum
{
	TimestampsToReturn_SOURCE_0 = 1,	//Return the source timestamp.
			//If used in HistoryRead the source timestamp is used to determine which historical data values are returned.
	TimestampsToReturn_SERVER_1 = 1,	//Return the Server timestamp.
			//If used in HistoryRead the Server timestamp is used to determine which historical data values are returned.
	TimestampsToReturn_BOTH_2 = 2,	//Return both the source and Server timestamps.
			//If used in HistoryRead the source timestamp is used to determine which historical data values are returned.
	TimestampsToReturn_NEITHER_3 = 3	//Return neither timestamp.
			//This is the default value for MonitoredItems if a Variable value is not being accessed.
			//For HistoryRead this is not a valid setting.
}
UA_AD_TimestampsToReturn;


/**
* UserIdentityToken Encrypted Token Format
* Part: 4
* Chapter: 7.35.1
* Page: 140
*/
typedef struct
{
	Byte length[4];
	UInt16 noOfTokenData;
	Byte* tokenData;
	UInt16 noOfServerNonce;
	Byte* serverNonce;
}
UA_AD_UserIdentityTokenEncryptedTokenFormat;


/**
* AnonymousIdentityToken
* Part: 4
* Chapter: 7.35.2
* Page: 141
*/
typedef struct
{
	UA_String policyId;
}
UA_AD_AnonymousIdentityToken;


/**
* UserNameIdentityToken
* Part: 4
* Chapter: 7.35.3
* Page: 141
*/
typedef struct
{
	UA_String policyId;
	UA_String userName;
	UA_ByteString password;
	UA_String encryptionAlogrithm;
}
UA_AD_UserNameIdentityToken;


/**
* X509IdentityTokens
* Part: 4
* Chapter: 7.35.4
* Page: 141
*/
typedef struct
{
	UA_String policyId;
	UA_ByteString certificateData;
}
UA_AD_X509IdentityTokens;


/**
* IssuedIdentityToken
* Part: 4
* Chapter: 7.35.5
* Page: 142
*/
typedef struct
{
	UA_String policyId;
	UA_ByteString tokenData;
	UA_String encryptionAlgorithm;
}
UA_AD_IssuedIdentityToken;





/**
* ViewDescription
* Part: 4
* Chapter: 7.37
* Page: 143
*/
typedef struct
{
	UA_NodeId viewId;
	UA_DateTime timestamp;
	UInt32 viewVersion;
}
UA_AD_ViewDescription;


#endif /* OPCUA_TYPES_H_ */
