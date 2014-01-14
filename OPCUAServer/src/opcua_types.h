/*
 * opcua_types.h
 *
 *  Created on: Jan 07, 2014
 *      Author: opcua
 */

#ifndef OPCUA_TYPES_H_
#define OPCUA_TYPES_H_

#include "opcua_builtInDatatypes.h"

typedef enum _T_ApplicationType
{
	SERVER_0,
	CLIENT_1,
	CLIENTANDSERVER_2,
	DISCOVERYSERVER_3
}
T_ApplicationType;


/**
* Index
* Part: 4
* Chapter: 7.12
* Page: 118
*/
typedef UInt32 T_Index;


/**
* IntegerId
* Part: 4
* Chapter: 7.13
* Page: 118
*/
typedef UInt32 T_IntegerId;


/**
* ApplicationDescription
* Part: 4
* Chapter: 7.1
* Page: 103
*/
typedef struct _T_ApplicationDescription
{
	UA_String applicationUri;
	UA_String productUri;
	UA_LocalizedText applicationName;
	T_ApplicationType applicationType;
	UA_String gatewayServerUri;
	UA_String discoveryProfileUri;
	UInt16 noOfDiscoerUrls;
	UA_String* discoverUrls;
}
T_ApplicationDescription;

/**
* ApplicationInstanceCertificate
* Part: 4
* Chapter: 7.2
* Page: 104
*/
typedef struct _T_ApplicationInstanceCertificate
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
T_ApplicationInstanceCertificate;


/**
* ContinuationPoint 				//ToDo
* Part: 4
* Chapter: 7.6
* Page: 114
*/
typedef struct _T_ContinuationPoint
{
}
T_ContinuationPoint;


/**
* ReferenceDescription
* Part: 4
* Chapter: 7.24
* Page: 131
*/
typedef struct _T_ReferenceDescription
{
	UA_NodeId referenceTypeId;
	Boolean isForward;
	UA_ExpandedNodeId nodeId;
	UA_QualifiedName browseName;
	UA_LocalizedText displayName;
//ToDo	T_NodeClass nodeClass;			//ToDo
	UA_ExpandedNodeId typeDefinition;
}
T_ReferenceDescription;


/**
* BrowseResult
* Part: 4
* Chapter: 7.3
* Page: 104
*/
typedef struct _T_BrowseResult
{
	UA_StatusCode statusCode;
	T_ContinuationPoint continuationPoint;
	UInt16 noOfReferences;
	T_ReferenceDescription *references;
}
T_BrowseResult;


/**
* Counter
* Part: 4
* Chapter: 7.5
* Page: 113
*/
typedef UInt32 T_Counter;


/**
* DataValue
* Part: 4
* Chapter: 7.7.1
* Page: 114
*/
typedef struct _T_DataValue
{
	UA_Variant value;	// BaseDataType are mapped to a UA_Variant -> part: 6 chapter: 5.1.5 page: 14
	UA_StatusCode statusCode;
	UA_DateTime sourceTimestamp;
//ToDo	UInt					//toBeDiscussed: Resolution of PicoSeconds
	UA_DateTime serverTimestamp;
//ToDo	UInt					//toBeDiscussed: Resolution of PicoSeconds
}
T_DataValue;


/**
* DiagnosticInfo
* Part: 4
* Chapter: 7.9
* Page: 116
*/
typedef struct _T_DiagnosticInfo
{
//ToDo	struct ???? identifier;				//ToDo: what kind of strcuture?
	Int32 namespaceUri;
	Int32 symbolicId;
	Int32 locale;
	Int32 localizesText;
	UA_String additionalInfo;
	UA_StatusCode innerStatusCode;
//ToDo	T_DiagnosticInfo innerDiagnosticInfo;
}
T_DiagnosticInfo;


/**
* MessageSecurityMode
* Part: 4
* Chapter: 7.14
* Page: 118
*/
typedef enum _T_MessageSecurityMode
{
	INVALID_0 = 0,
	SIGN_1 = 1,
	SIGNANDENCRYPT_2 = 2
}
T_MessageSecurityMode;


/**
* UserTokenPolicy
* Part: 4
* Chapter: 7.36
* Page: 142
*/
typedef struct _T_UserTokenPolicy
{
	UA_String policyId;
//ToDo	T_UserIdentityTokenType tokenType;
	UA_String issuedTokenType;
	UA_String issuerEndpointUrl;
	UA_String securityPolicyUri;
}
T_UserTokenPolicy;


/**
* EndpointDescription
* Part: 4
* Chapter: 7.9
* Page: 116
*/
typedef struct _T_EndpointDescription
{
	UA_String endpointUrl;
	T_ApplicationDescription server;
	T_ApplicationInstanceCertificate serverCertificate;
	T_MessageSecurityMode securityMode;
	UA_String securityPolicyUri;
	UInt16 noOfUserIdentyTokens;
	T_UserTokenPolicy* useridentyTokens;
	UA_String transportProfileUri;
	Byte securtiyLevel;
}
T_EndpointDescription;


/**
* ExpandedNodeId
* Part: 4
* Chapter: 7.10
* Page: 117
*/
typedef struct _T_ExpandedNodeId
{
	T_Index serverIndex;
	UA_String namespaceUri;
	T_Index namespaveIndex;
//ToDo	enum BED_IdentifierType identiferType;		//ToDo: Is the enumeration correct?
//ToDo	UA_NodeIdentifier identifier;		//ToDo -> Part 3: Address Space Model
}
T_ExpandedNodeId;


/**
* ExtensibleParameter
* Part: 4
* Chapter: 7.11
* Page: 117
*/
typedef struct _T_ExtensibleParameter
{
	UA_NodeId parameterTypeId;
//ToDo	-- parameterData;			//toBeDiscussed
}
T_ExtensibleParameter;


/**
* DataChangeFilter
* Part: 4
* Chapter: 7.16.2
* Page: 119
*/
typedef struct _T_DataChangeFilter
{
//ToDo	enum BED_MonitoringFilter trigger = BED_MonitoringFilter.DATA_CHANGE_FILTER;
	UInt32 deadbandType;
	Double deadbandValue;
}
T_DataChangeFilter;


/**
* ContentFilter 						//ToDo
* Part: 4
* Chapter: 7.4.1
* Page: 104
*/
typedef struct _T_ContentFilter
{
//ToDo	struct BED_ContentFilterElement elements[];		//ToDo
//ToDo	enum BED_FilterOperand filterOperator;			//ToDo table 110
//ToDo	struct BED_ExtensibleParamterFilterOperand filterOperands[]; //ToDo 7.4.4
}
T_ContentFilter;


/**
* EventFilter
* Part: 4
* Chapter: 7.16.3
* Page: 120
*/
typedef struct _T_EventFilter
{
//ToDo	SimpleAttributeOperantd selectClauses[]; 		//ToDo
//ToDo	ContenFilter whereClause;				//ToDo
}
T_EventFilter;

typedef struct _T_EventFilterResult
{
	UA_StatusCode selectClauseResults[3];
	UA_DiagnosticInfo selectClauseDiagnosticInfos[3];
//ToDo	T_ContentFilterResult whereClauseResult;
}
T_EventFilterResult;


/**
* AggregateFilter
* Part: 4
* Chapter: 7.16.4
* Page: 122
*/
typedef struct _T_AggregateFilter
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
T_AggregateFilter;

typedef struct _T_AggregateFilterResult
{
	UA_DateTime revisedStartTime;
	UA_Duration revisedProcessingInterval;
}
T_AggregateFilterResult;



/**
* MonitoringParameters
* Part: 4
* Chapter: 7.15
* Page: 118
*/
typedef struct _T_MonitoringParameters
{
	T_IntegerId clientHandle;
//ToDo	Duration???? samplingInterval;						//ToDo
//ToDo	struct BED_ExtensibleParameterMonitoringFilter filter			//ToDo
	T_Counter queueSize;
	Boolean discardOldest;
}
T_MonitoringParameters;

//->ExtensibleParameter ->Part:4 Chapter:7.11 Page:117
typedef struct _T_ExtensibleParameterMonitoringFilter		//ToDo: Ist die Umsetzung des ExtensibleParameter korrekt?
{
//ToDo	T_MonitoringFilter parameterTypeId;
	T_DataChangeFilter dataChangeFilter;
	T_EventFilter eventFilter;
	T_AggregateFilter aggregateFilter;
}
T_ExtensibleParameterMonitoringFilter;

/**
* MonitoringFilter parameterTypeIds
* Part: 4
* Chapter: 7.16.1
* Page: 119
*/
typedef enum _T_MonitoringFilter
{
	DATA_CHANGE_FILTER = 1,
	EVENT_FILTER = 2,
	AGGREGATE_FILTER = 3
}
T_MonitoringFilter;




/**
* MonitoringMode
* Part: 4
* Chapter: 7.17
* Page: 123
*/
typedef enum _T_MonitoringModeValues
{
	DISABLED_0 = 0,	//The item being monitored is not sampled or evaluated, and Notifications are not generated or queued. Notification reporting is disabled.
	SAMPLING_1 = 1,	//The item being monitored is sampled and evaluated, and Notifications are generated and queued. Notification reporting is disabled.
	REPORTING_2 = 2	//The item being monitored is sampled and evaluated, and Notifications are generated and queued. Notification reporting is enabled.
}
T_MonitoringModeValues;


/**
* NodeAttributes parameters
* Part: 4
* Chapter: 7.18.1
* Page: 124
*/
typedef enum _T_NodeAttributesParamterTypeIds
{
	ObjectAttributes,	//Defines the Attributes for the Object NodeClass.
	VariableAttributes,	//Defines the Attributes for the Variable NodeClass.
	MethodAttributes,	//Defines the Attributes for the Method NodeClass.
	ObjectTypeAttributes,	//Defines the Attributes for the ObjectType NodeClass.
	VariableTypeAttributes,	//Defines the Attributes for the VariableType NodeClass.
	ReferenceTypeAttributes,//Defines the Attributes for the ReferenceType NodeClass.
	DataTypeAttributes,	//Defines the Attributes for the DataType NodeClass.
	ViewAttributes		//Defines the Attributes for the View NodeClass.
}
T_NodeAttributesParamterTypeIds;

typedef enum _T_NodeAttributesBitMask
{
	AccessLevel = 1, 	//Bit: 0 Indicates if the AccessLevel Attribute is set.
	ArrayDimensions = 2,	//Bit: 1 Indicates if the ArrayDimensions Attribute is set.
	//Reserved = 4, 	//Bit: 2 Reserved to be consistent with WriteMask defined in IEC 62541-3.
	ContainsNoLoops = 8,	//Bit: 3 Indicates if the ContainsNoLoops Attribute is set.
	DataType = 16,		//Bit: 4 Indicates if the DataType Attribute is set.
	Description = 32,	//Bit: 5 Indicates if the Description Attribute is set.
	DisplayName = 64,	//Bit: 6 Indicates if the DisplayName Attribute is set.
	EventNotifier = 128,	//Bit: 7 Indicates if the EventNotifier Attribute is set.
	Executable = 256,	//Bit: 8 Indicates if the Executable Attribute is set.
	Historizing = 512,	//Bit: 9 Indicates if the Historizing Attribute is set.
	InverseName = 1024,	//Bit:10 Indicates if the InverseName Attribute is set.
	IsAbstract = 2048,	//Bit:11 Indicates if the IsAbstract Attribute is set.
	MinimumSamplingInterval = 4096, //Bit:12 Indicates if the MinimumSamplingInterval Attribute is set.
	//Reserved = 8192,	//Bit:13 Reserved to be consistent with WriteMask defined in IEC 62541-3.
	//Reserved = 16384,	//Bit:14 Reserved to be consistent with WriteMask defined in IEC 62541-3.
	Symmetric = 32768,	//Bit:15 Indicates if the Symmetric Attribute is set.
	UserAccessLevel = 65536,//Bit:16 Indicates if the UserAccessLevel Attribute is set.
	UserExecutable = 131072,//Bit:17 Indicates if the UserExecutable Attribute is set.
	UserWriteMask = 262144, //Bit:18 Indicates if the UserWriteMask Attribute is set.
	ValueRank = 524288,	//Bit:19 Indicates if the ValueRank Attribute is set.
	WriteMask = 1048576,	//Bit:20 Indicates if the WriteMask Attribute is set.
	Value = 2097152		//Bit:21 Indicates if the Value Attribute is set.
	//Reserved		//Bit:22:32 Reserved for future use. Shall always be zero.
}
T_NodeAttributesBitMask;


/**
* ObjectAttributes parameters
* Part: 4
* Chapter: 7.18.2
* Page: 125
*/
typedef struct _T_ObjectAttributes
{
	UInt32 specifiedAttribute;	//BitMask corresponding to BED_NodeAttributesBitMask
	UA_LocalizedText displayName;
	UA_LocalizedText description;
	Byte eventNotifier;
	UInt32 writeMask;
	UInt32 userWriteMask;
}
T_ObjectAttributes;


/**
* VariableAttributes parameters
* Part: 4
* Chapter: 7.18.3
* Page: 125
*/
typedef struct _T_VariableAttributes
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
T_VariableAttributes;


/**
* MethodAttributes parameters
* Part: 4
* Chapter: 7.18.4
* Page: 125
*/
typedef struct _T_MethodAttributes
{
	UInt32 specifiedAttributes;	//BitMask corresponding to BED_NodeAttributesBitMask
	UA_LocalizedText displayName;
	UA_LocalizedText description;
	Boolean executable;
	Boolean userExecutable;
	UInt32 writeMask;
	UInt32 userWriteMask;
}
T_MethodAttributes;


/**
* ObjectTypeAttributes parameters
* Part: 4
* Chapter: 7.18.5
* Page: 125
*/
typedef struct _T_ObjectTypeAttributes
{
	UInt32 specifiedAttributes; 	//BitMask corresponding to BED_NodeAttributesBitMask
	UA_LocalizedText displayName;
	UA_LocalizedText description;
	Boolean isAbstract;
	UInt32 wirteMask;
	UInt32 userWriteMask;
}
T_ObjectTypeAttributes;


/**
* VariableTypeAttributes parameters
* Part: 4
* Chapter: 7.18.6
* Page: 126
*/
typedef struct _T_VariableTypeAttributes
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
T_VariableTypeAttributes;


/**
* ReferenceTypeAttributes parameters
* Part: 4
* Chapter: 7.18.7
* Page: 126
*/
typedef struct _T_ReferenceTypeAttributes
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
T_ReferenceTypeAttributes;



/**
* DataTypeAttributes parameters
* Part: 4
* Chapter: 7.18.8
* Page: 126
*/
typedef struct _T_DataTypeAttributes
{
	UInt32 specifiedAttributes; 	//BitMask corresponding to BED_NodeAttributesBitMask
	UA_LocalizedText displayName;
	UA_LocalizedText description;
	Boolean isAbstract;
	UInt32 writeMask;
	UInt32 userWriteMask;
}
T_DataTypeAttributes;



/**
* ViewAttributes parameters
* Part: 4
* Chapter: 7.18.9
* Page: 127
*/
typedef struct _T_ViewAttributes
{
	UInt32 specifiedAttributes; 	//BitMask corresponding to BED_NodeAttributesBitMask
	UA_LocalizedText displayName;
	UA_LocalizedText description;
	Boolean containsNoLoops;
	Byte eventNotifier;
	UInt32 writeMask;
	UInt32 userWriteMask;
}
T_ViewAttributes;


/**
* NotificationData parameters
* Part: 4
* Chapter: 7.19
* Page: 127
*/
typedef enum _T_NotificationDataParameterTypeIds
{
	DATA_CHANGE = 1,
	EVENT = 2,
	STATUS_CHANGE = 3
}
T_NotificationDataParameterTypeIds;


/**
* DataChangeNotification parameter
* Part: 4
* Chapter: 7.19.2
* Page: 127
*/
typedef struct _T_DataChangeNotification
{
//ToDo	struct BED_MonitoredItemNotification monitoredItems[];		//ToDo
	T_IntegerId clientHandle;
	UA_DataValue value;
	UInt16 noOfdiagnosticInfos;
	UA_DiagnosticInfo* diagnosticInfos;
}
T_DataChangeNotification;


/**
* EventNotificationList parameter
* Part: 4
* Chapter: 7.19.3
* Page: 128
*/
typedef struct _T_EventNotificationList
{
//ToDo	struct EventFieldList events[];			//ToDo
	T_IntegerId clientHandle;
	UInt16 noOfEventFields;
	UA_Variant* eventFields;	// BaseDataType are mapped to a UA_Variant -> part: 6 chapter: 5.1.5 page: 14
}
T_EventNotificationList;


/**
* StatusChangeNotification parameter
* Part: 4
* Chapter: 7.19.4
* Page: 128
*/
typedef struct _T_StatusChangeNotification
{
	UA_StatusCode status;
	UA_DiagnosticInfo diagnosticInfo;
}
T_StatusChangeNotification;


/**
* NotificationMessage
* Part: 4
* Chapter: 7.20
* Page: 129
*/
//->ExtensibleParameter ->Part:4 Chapter:7.11 Page:117
typedef struct _T_ExtensibleParameterNotificationData 		//ToDo: Ist die Umsetzung des ExtensibleParameter korrekt?
{
	T_NotificationDataParameterTypeIds parameterTypeId;
	T_DataChangeNotification dataChange;
	T_EventNotificationList event;
	T_StatusChangeNotification statusChange;
}
T_ExtensibleParameterNotificationData;

typedef struct _T_NotificationMessage
{
	T_Counter sequenceNumber;
	UA_DateTime publishTime;
	UInt16 noOfNotificationData;
	T_ExtensibleParameterNotificationData* notificationData;
}
T_NotificationMessage;




/**
* NumericRange
* Part: 4
* Chapter: 7.21
* Page: 129
*/
typedef UA_String T_NumericRange;


/**
* QueryDataSet
* Part: 4
* Chapter: 7.22
* Page: 130
*/
typedef struct _T_QueryDataSet
{
	UA_ExpandedNodeId nodeId;
	UA_ExpandedNodeId typeDefinitionNode;
	UInt16 noOfValues;
	UA_Variant* values;	// BaseDataType are mapped to a UA_Variant -> part: 6 chapter: 5.1.5 page: 14
}
T_QueryDataSet;


/**
* ReadValueId
* Part: 4
* Chapter: 7.23
* Page: 130
*/
typedef struct _T_ReadValueId
{
	UA_NodeId nodeId;
	T_IntegerId attributeId;
	T_NumericRange indexRange;
	UA_QualifiedName dataEncoding;
}
T_ReadValueId;


/**
* RelativePath
* Part: 4
* Chapter: 7.25
* Page: 131
*/
typedef struct _T_RelativePath
{
//ToDo	struct BED_RelativePathElement elements[];		//ToDo
	UA_NodeId referenceTypeId;
	Boolean isInverse;
	Boolean includeSubtypes;
	UA_QualifiedName targetName;
}
T_RelativePath;


/**
* RequestHeader
* Part: 4
* Chapter: 7.26
* Page: 132
*/
typedef struct _T_RequestHeader
{
	UA_NodeId authenticationToken;
	UA_DateTime timestamp;
	T_IntegerId requestHandle;
	UInt32 returnDiagnostics;
	UA_String auditEntryId;
	UInt32 timeoutHint;
	UA_ExpandedNodeId additionalHeader;
}
T_RequestHeader;


typedef enum _T_RequestReturnDiagnositcs
{
	SERVICE_LEVEL_SYMBOLIC_ID = 1,				//Hex 0x01
	SERVICE_LEVEL_LOCALIZED_TEXT= 2,			//Hex 0x02
	SERVICE_LEVEL_ADDITIONAL_INFO = 4,			//Hex 0x04
	SERVICE_LEVEL_INNER_STATUS_CODE = 8,		//Hex 0x08
	SERVICE_LEVEL_INNER_DIAGNOSTICS = 16,		//Hex 0x10
	OPERATION_LEVEL_SYMBOLIC_ID = 32,			//Hex 0x20
	OPERATION_LEVEL_LOCALIZED_TEXT= 64,			//Hex 0x40
	OPERATION_LEVEL_ADDITIONAL_INFO = 128,		//Hex 0x80
	OPERATION_LEVEL_INNER_STATUS_CODE = 256,	//Hex 0x100
	OPERATION_LEVEL_INNER_DIAGNOSTICS = 512		//Hex 0x200
}
T_RequestReturnDiagnositcs;


/**
* ResponseHeader
* Part: 4
* Chapter: 7.27
* Page: 133
*/
typedef struct _T_ResponseHeader
{
	UA_DateTime timestamp;
	T_IntegerId requestHandle;
	UA_StatusCode serviceResult;
	UA_DiagnosticInfo serviceDiagnostics;
	UInt16 noOfStringTable;
	UA_String* stringTable;
//ToDo	struct BED_ExtensibleParameterAdditionalHeader additionalHeader;		//ToDo
}
T_ResponseHeader;


/**
* ServiceFault
* Part: 4
* Chapter: 7.28
* Page: 133
*/
typedef struct _T_ServiceFault
{
	T_ResponseHeader responseHeader;
}
T_ServiceFault;


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
typedef struct _T_SignatureData
{
	UA_ByteString signature;
	UA_String agorithm;
}
T_SignatureData;


/**
* SignedSoftwareCertificate
* Part: 4
* Chapter: 7.31
* Page: 135
*/
typedef struct _T_SignedSoftwareCertificate
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
T_SignedSoftwareCertificate;


/**
* SoftwareCertificate
* Part: 4
* Chapter: 7.32
* Page: 135
*/
typedef enum _T_ComplianceLevel
{
	UNTESTED_0 = 0,		//the profiled capability has not been tested successfully.
	PARTIAL_1 = 1,		//the profiled capability has been partially tested and has
				//passed critical tests, as defined by the certifying authority.
	SELFTESTED_2 = 2,	//the profiled capability has been successfully tested using a
				//self-test system authorized by the certifying authority.
	CERTIFIED_3 = 3		//the profiled capability has been successfully tested by a
				//testing organisation authorized by the certifying authority.
}
T_ComplianceLevel;

typedef struct _T_SupportedProfiles
{
	UA_String oranizationUri;
	UA_String profileId;
	UA_String complianceTool;
	UA_DateTime complianceDate;
	T_ComplianceLevel complianceLevel;
	UInt16 noOfUnsupportedUnitIds;
	UA_String* unsupportedUnitIds;
}
T_SupportedProfiles;

typedef struct _T_SoftwareCertificate
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
	T_SupportedProfiles supportedProfiles;
}
T_SoftwareCertificate;


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
typedef enum _T_TimestampsToReturn
{
	SOURCE_0 = 1,	//Return the source timestamp.
			//If used in HistoryRead the source timestamp is used to determine which historical data values are returned.
	SERVER_1 = 1,	//Return the Server timestamp.
			//If used in HistoryRead the Server timestamp is used to determine which historical data values are returned.
	BOTH_2 = 2,	//Return both the source and Server timestamps.
			//If used in HistoryRead the source timestamp is used to determine which historical data values are returned.
	NEITHER_3 = 3	//Return neither timestamp.
			//This is the default value for MonitoredItems if a Variable value is not being accessed.
			//For HistoryRead this is not a valid setting.
}
T_TimestampsToReturn;


/**
* UserIdentityToken Encrypted Token Format
* Part: 4
* Chapter: 7.35.1
* Page: 140
*/
typedef struct _T_UserIdentityTokenEncryptedTokenFormat
{
	Byte length[4];
	UInt16 noOfTokenData;
	Byte* tokenData;
	UInt16 noOfServerNonce;
	Byte* serverNonce;
}
T_UserIdentityTokenEncryptedTokenFormat;


/**
* AnonymousIdentityToken
* Part: 4
* Chapter: 7.35.2
* Page: 141
*/
typedef struct _T_AnonymousIdentityToken
{
	UA_String policyId;
}
_T_AnonymousIdentityToken;


/**
* UserNameIdentityToken
* Part: 4
* Chapter: 7.35.3
* Page: 141
*/
typedef struct _T_UserNameIdentityToken
{
	UA_String policyId;
	UA_String userName;
	UA_ByteString password;
	UA_String encryptionAlogrithm;
}
T_UserNameIdentityToken;


/**
* X509IdentityTokens
* Part: 4
* Chapter: 7.35.4
* Page: 141
*/
typedef struct _T_X509IdentityTokens
{
	UA_String policyId;
	UA_ByteString certificateData;
}
T_X509IdentityTokens;


/**
* IssuedIdentityToken
* Part: 4
* Chapter: 7.35.5
* Page: 142
*/
typedef struct _T_IssuedIdentityToken
{
	UA_String policyId;
	UA_ByteString tokenData;
	UA_String encryptionAlgorithm;
}
T_IssuedIdentityToken;





/**
* ViewDescription
* Part: 4
* Chapter: 7.37
* Page: 143
*/
typedef struct _T_ViewDescription
{
	UA_NodeId viewId;
	UA_DateTime timestamp;
	UInt32 viewVersion;
}
T_ViewDescription;


#endif /* OPCUA_TYPES_H_ */
