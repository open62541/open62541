/*
 * opcua_BinaryEncDec.h
 *
 *  Created on: Dec 18, 2013
 *      Author: opcua
 */

#ifndef OPCUA_BINARYENCDEC_NEU_H_
#define OPCUA_BINARYENCDEC_NEU_H_

#include "opcua_builtInDatatypes.h"

enum BED_ApplicationType {SERVER_0, CLIENT_1, CLIENTANDSERVER_2, DISCOVERYSERVER_3};


/**
* ApplicationDescription
* Part: 4
* Chapter: 7.1
* Page: 103
*/
struct BED_ApplicationDescription
{
	UA_String applicationUri;
	UA_String productUri;
	UA_LocalizedText applicationName;
	enum BED_ApplicationType applicationType;
	UA_String gatewayServerUri;
	UA_String discoveryProfileUri;
	UA_String discoverUrls[];
};


/**
* ApplicationInstanceCertificate
* Part: 4
* Chapter: 7.2
* Page: 104
*/
struct BED_ApplicationInstanceCertificate
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
	UA_String *hostnames;
	UA_ByteString pubicKey;
	UA_String keyUsage[];

};


/**
* BrowseResult
* Part: 4
* Chapter: 7.3
* Page: 104
*/
struct BED_BrowseResult
{
	struct UA_StatusCode statusCode;
	struct BED_ContinuationPoint continuationPoint;
	struct BED_ReferenceDescription references[];
};


/**
* ContentFilter 						//ToDo
* Part: 4
* Chapter: 7.4.1
* Page: 104
*/
struct BED_ContentFilter
{
//ToDo	struct BED_ContentFilterElement elements[];		//ToDo
//ToDo	enum BED_FilterOperand filterOperator;			//ToDo table 110
//ToDo	struct BED_ExtensibleParamterFilterOperand filterOperands[]; //ToDo 7.4.4
};


/**
* Counter
* Part: 4
* Chapter: 7.5
* Page: 113
*/
typedef UInt32 BED_Counter;


/**
* ContinuationPoint 				//ToDo
* Part: 4
* Chapter: 7.6
* Page: 114
*/
struct BED_ContinuationPoint
{
};

/**
* DataValue
* Part: 4
* Chapter: 7.7.1
* Page: 114
*/
struct BED_DataValue
{
	UA_Variant value;	// BaseDataType are mapped to a UA_Variant -> part: 6 chapter: 5.1.5 page: 14
	struct UA_StatusCode statusCode;
	UA_DateTime sourceTimestamp;
//ToDo	UInt					//toBeDiscussed: Resolution of PicoSeconds
	UA_DateTime serverTimestamp;
//ToDo	UInt					//toBeDiscussed: Resolution of PicoSeconds
};


/**
* DiagnosticInfo
* Part: 4
* Chapter: 7.9
* Page: 116
*/
struct BED_DiagnosticInfo
{
//ToDo	struct ???? identifier;				//ToDo: what kind of strcuture?
	Int32 namespaceUri;
	Int32 symbolicId;
	Int32 locale;
	Int32 localizesText;
	struct UA_String additionalInfo;
	struct UA_StatusCode innerStatusCode;
	struct BED_DiagnosticInfo innerDiagnosticInfo;
};


/**
* EndpointDescription
* Part: 4
* Chapter: 7.9
* Page: 116
*/
struct BED_EndpointDescription
{
	struct UA_String endpointUrl;
	struct BED_ApplicationDescription server;
	struct BED_ApplicationInstanceCertificate serverCertificate;
	enum BED_MessageSecurityMode securityMode;
	struct UA_String securityPolicyUri;
	struct BED_UserTokenPolicy useridentyTokens[];
	struct UA_String transportProfileUri;
	Byte securtiyLevel;
};

/**
* ExpandedNodeId
* Part: 4
* Chapter: 7.10
* Page: 117
*/
struct BED_ExpandedNodeId
{
	BED_Index serverIndex;
	struct UA_String namespaceUri;
	BED_Index namespaveIndex;
//ToDo	enum BED_IdentifierType identiferType;		//ToDo: Is the enumeration correct?
//ToDo	UA_NodeIdentifier identifier;		//ToDo -> Part 3: Address Space Model
};


/**
* ExtensibleParameter
* Part: 4
* Chapter: 7.11
* Page: 117
*/
struct BED_ExtensibleParameter
{
	struct UA_NodeId parameterTypeId;
//ToDo	-- parameterData;			//toBeDiscussed
};


/**
* Index
* Part: 4
* Chapter: 7.12
* Page: 118
*/
typedef UInt32 BED_Index;


/**
* IntegerId
* Part: 4
* Chapter: 7.13
* Page: 118
*/
typedef UInt32 BED_IntegerId;


/**
* MessageSecurityMode
* Part: 4
* Chapter: 7.14
* Page: 118
*/
enum BED_MessageSecurityMode
{
	INVALID_0 = 0,
	SIGN_1 = 1,
	SIGNANDENCRYPT_2 = 2
};


/**
* MonitoringParameters
* Part: 4
* Chapter: 7.15
* Page: 118
*/
struct BED_MonitoringParameters
{
	BED_IntegerId clientHandle;
//ToDo	Duration???? samplingInterval;						//ToDo
//ToDo	struct BED_ExtensibleParameterMonitoringFilter filter			//ToDo
	BED_Counter queueSize;
	Boolean discardOldest;
};

//->ExtensibleParameter ->Part:4 Chapter:7.11 Page:117
struct BED_ExtensibleParameterMonitoringFilter		//ToDo: Ist die Umsetzung des ExtensibleParameter korrekt?
{
	enum BED_MonitoringFilter parameterTypeId;
	struct BED_DataChangeFilter dataChangeFilter;
	struct BED_EventFilter eventFilter;
	struct BED_AggregateFilter aggregateFilter;
};

/**
* MonitoringFilter parameterTypeIds
* Part: 4
* Chapter: 7.16.1
* Page: 119
*/
enum BED_MonitoringFilter
{
	DATA_CHANGE_FILTER = 1,
	EVENT_FILTER = 2,
	AGGREGATE_FILTER = 3
};


/**
* DataChangeFilter
* Part: 4
* Chapter: 7.16.2
* Page: 119
*/
struct BED_DataChangeFilter
{
//ToDo	enum BED_MonitoringFilter trigger = BED_MonitoringFilter.DATA_CHANGE_FILTER;
	UInt32 deadbandType;
	Double deadbandValue;
};


/**
* EventFilter
* Part: 4
* Chapter: 7.16.3
* Page: 120
*/
struct BED_EventFilter
{
//ToDo	SimpleAttributeOperantd selectClauses[]; 		//ToDo
//ToDo	ContenFilter whereClause;				//ToDo
};

struct BED_EventFilterResult
{
	struct UA_StatusCode selectClauseResults[3];
	struct UA_DiagnosticInfo selectClauseDiagnosticInfos[3];
	struct BED_ContentFilterResult whereClauseResult;
};


/**
* AggregateFilter
* Part: 4
* Chapter: 7.16.4
* Page: 122
*/
struct BED_AggregateFilter
{
	UA_DateTime startTime;
	struct UA_NodeId aggregateType;
	UA_Duration processingInterval;
//ToDo	AggregateConfiguration aggregateConfiguration;		//ToDo
	Boolean useServerCapabilitiesDafaults;
	Boolean treatUncertainAsBad;
	Byte percentDataBad;
	Byte percentDataGood;
	Boolean steppedSlopedExtrapolation;
};

struct BED_AggregateFilterResult
{
	UA_DateTime revisedStartTime;
	UA_Duration revisedProcessingInterval;
};


/**
* MonitoringMode
* Part: 4
* Chapter: 7.17
* Page: 123
*/
enum BED_MonitoringModeValues
{
	DISABLED_0 = 0,	//The item being monitored is not sampled or evaluated, and Notifications are not generated or queued. Notification reporting is disabled.
	SAMPLING_1 = 1,	//The item being monitored is sampled and evaluated, and Notifications are generated and queued. Notification reporting is disabled.
	REPORTING_2 = 2	//The item being monitored is sampled and evaluated, and Notifications are generated and queued. Notification reporting is enabled.
};


/**
* NodeAttributes parameters
* Part: 4
* Chapter: 7.18.1
* Page: 124
*/
enum BED_NodeAttributesParamterTypeIds
{
	ObjectAttributes,	//Defines the Attributes for the Object NodeClass.
	VariableAttributes,	//Defines the Attributes for the Variable NodeClass.
	MethodAttributes,	//Defines the Attributes for the Method NodeClass.
	ObjectTypeAttributes,	//Defines the Attributes for the ObjectType NodeClass.
	VariableTypeAttributes,	//Defines the Attributes for the VariableType NodeClass.
	ReferenceTypeAttributes,//Defines the Attributes for the ReferenceType NodeClass.
	DataTypeAttributes,	//Defines the Attributes for the DataType NodeClass.
	ViewAttributes		//Defines the Attributes for the View NodeClass.
};

enum BED_NodeAttributesBitMask
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
};


/**
* ObjectAttributes parameters
* Part: 4
* Chapter: 7.18.2
* Page: 125
*/
struct BED_ObjectAttributes
{
	UInt32 specifiedAttribute;	//BitMask corresponding to BED_NodeAttributesBitMask
	struct UA_LocalizedText displayName;
	struct UA_LocalizedText description;
	Byte eventNotifier;
	UInt32 writeMask;
	UInt32 userWriteMask;
};


/**
* VariableAttributes parameters
* Part: 4
* Chapter: 7.18.3
* Page: 125
*/
struct BED_VariableAttributes
{
	UInt32 specifiedAttributes; 	//BitMask corresponding to BED_NodeAttributesBitMask
	struct UA_LocalizedText displayName;
	struct UA_LocalizedText description;
//ToDo	DefinedByTheDataTypeAttribte??? value				//ToDo
	struct UA_NodeId dataType;
	Int32 valueRank;
	UInt32 arrayDimensions[];
	Byte accessLevel;
	Byte userAccesLevel;
//ToDo	Duration???? minimumSamplingInterval;			//ToDo
	Boolean historizing;
	UInt32 writeMask;
	UInt32 userWriteMask;
};


/**
* MethodAttributes parameters
* Part: 4
* Chapter: 7.18.4
* Page: 125
*/
struct BED_MethodAttributes
{
	UInt32 specifiedAttributes;	//BitMask corresponding to BED_NodeAttributesBitMask
	struct UA_LocalizedText displayName;
	struct UA_LocalizedText description;
	Boolean executable;
	Boolean userExecutable;
	UInt32 writeMask;
	UInt32 userWriteMask;
};


/**
* ObjectTypeAttributes parameters
* Part: 4
* Chapter: 7.18.5
* Page: 125
*/
struct BED_ObjectTypeAttributes
{
	UInt32 specifiedAttributes; 	//BitMask corresponding to BED_NodeAttributesBitMask
	struct UA_LocalizedText displayName;
	struct UA_LocalizedText description;
	Boolean isAbstract;
	UInt32 wirteMask;
	UInt32 userWriteMask;
};


/**
* VariableTypeAttributes parameters
* Part: 4
* Chapter: 7.18.6
* Page: 126
*/
struct BED_VariableTypeAttributes
{
	UInt32 specifiedAttributes; 	//BitMask corresponding to BED_NodeAttributesBitMask
	struct UA_LocalizedText displayName;
	struct UA_LocalizedText description;
//ToDo	DefinedByTheDataTypeAttribte??? value			//ToDo
	struct UA_NodeId dataType;
	Int32 valueRank;
	UInt32 arrayDimesions[];
	Boolean isAbstract;
	UInt32 writeMask;
	UInt32 userWriteMask;
};


/**
* ReferenceTypeAttributes parameters
* Part: 4
* Chapter: 7.18.7
* Page: 126
*/
struct BED_ReferenceTypeAttributes
{
	UInt32 specifiedAttributes; 	//BitMask corresponding to BED_NodeAttributesBitMask
	struct UA_LocalizedText displayName;
	struct UA_LocalizedText description;
	Boolean isAbstract;
	Boolean symmetric;
	struct UA_LocalizedText inverseName;
	UInt32 writeMask;
	UInt32 userWriteMask;
};



/**
* DataTypeAttributes parameters
* Part: 4
* Chapter: 7.18.8
* Page: 126
*/
struct BED_DataTypeAttributes
{
	UInt32 specifiedAttributes; 	//BitMask corresponding to BED_NodeAttributesBitMask
	struct UA_LocalizedText displayName;
	struct UA_LocalizedText description;
	Boolean isAbstract;
	UInt32 writeMask;
	UInt32 userWriteMask;
};



/**
* ViewAttributes parameters
* Part: 4
* Chapter: 7.18.9
* Page: 127
*/
struct BED_ViewAttributes
{
	UInt32 specifiedAttributes; 	//BitMask corresponding to BED_NodeAttributesBitMask
	struct UA_LocalizedText displayName;
	struct UA_LocalizedText description;
	Boolean containsNoLoops;
	Byte eventNotifier;
	UInt32 writeMask;
	UInt32 userWriteMask;
};


/**
* NotificationData parameters
* Part: 4
* Chapter: 7.19
* Page: 127
*/
enum BED_NotificationDataParameterTypeIds
{
	DATA_CHANGE = 1,
	EVENT = 2,
	STATUS_CHANGE = 3
};


/**
* DataChangeNotification parameter
* Part: 4
* Chapter: 7.19.2
* Page: 127
*/
struct BED_DataChangeNotification
{
//ToDo	struct BED_MonitoredItemNotification monitoredItems[];		//ToDo
	BED_IntegerId clientHandle;
	struct UA_DataValue value;
	UA_DiagnosticInfo diagnositcInfos[];
};


/**
* EventNotificationList parameter
* Part: 4
* Chapter: 7.19.3
* Page: 128
*/
struct BED_EventNotificationList
{
//ToDo	struct EventFieldList events[];			//ToDo
	BED_IntegerId clientHandle;
	UA_Variant eventFields[];	// BaseDataType are mapped to a UA_Variant -> part: 6 chapter: 5.1.5 page: 14
};


/**
* StatusChangeNotification parameter
* Part: 4
* Chapter: 7.19.4
* Page: 128
*/
struct BED_StatusChangeNotification
{
	struct UA_StatusCode status;
	struct UA_DiagnosticInfo diagnosticInfo;
};


/**
* NotificationMessage
* Part: 4
* Chapter: 7.20
* Page: 129
*/
struct BED_NotificationMessage
{
	BED_Counter sequenceNumber;
	struct UA_DateTime publishTime;
	struct ExtensibleParameterNotificationData notificationData[];
};

//->ExtensibleParameter ->Part:4 Chapter:7.11 Page:117
struct ExtensibleParameterNotificationData 		//ToDo: Ist die Umsetzung des ExtensibleParameter korrekt?
{
	enum BED_NotificationDataParameterTypeIds parameterTypeId;
	struct BED_DataChangeNotification dataChange;
	struct BED_EventNotificationList event;
	struct BED_StatusChangeNotification statusChange;
};


/**
* NumericRange
* Part: 4
* Chapter: 7.21
* Page: 129
*/
typedef UA_String NumericRange;


/**
* QueryDataSet
* Part: 4
* Chapter: 7.22
* Page: 130
*/
struct BED_QueryDataSet
{
	struct UA_ExpandedNodeId nodeId;
	struct UA_ExpandedNodeId typeDefinitionNode;
	struct UA_Variant values[];	// BaseDataType are mapped to a UA_Variant -> part: 6 chapter: 5.1.5 page: 14
};


/**
* ReadValueId
* Part: 4
* Chapter: 7.23
* Page: 130
*/
struct BED_QueryDataSet
{
	struct UA_NodeId nodeId;
	BED_IntegerId attributeId;
	struct BED_NumericRange indexRange;
	struct UA_QualifiedName dataEncoding;
};


/**
* ReferenceDescription
* Part: 4
* Chapter: 7.24
* Page: 131
*/
struct BED_ReferenceDescription
{
	struct UA_NodeId referenceTypeId;
	Boolean isForward;
	struct UA_ExpandedNodeId nodeId;
	struct UA_QualifiedName browseName;
	struct UA_LocalizedText displayName;
//ToDo	struct BED_NodeClass nodeClass;			//ToDo
	struct UA_ExpandedNodeId typeDefinition;
};


/**
* RelativePath
* Part: 4
* Chapter: 7.25
* Page: 131
*/
struct BED_RelativePath
{
//ToDo	struct BED_RelativePathElement elements[];		//ToDo
	struct UA_NodeId referenceTypeId;
	Boolean isInverse;
	Boolean includeSubtypes;
	struct UA_QualifiedName targetName;
};


/**
* RequestHeader
* Part: 4
* Chapter: 7.26
* Page: 132
*/
struct BED_RequestHeader
{
//ToDo	struct BED_SessionAuthenticationToken authenticationToken;		//ToDo
	UA_DateTime timestamp;
	BED_IntegerId requestHandle;
	UInt32 returnDiagnostics;
	struct UA_String auditEntryId;
	UInt32 timeoutHint;
//ToDo	struct BED_ExtensibleParameterAdditionalHeader additionalHeader;		//ToDo
};


enum BED_RequestReturnDiagnositcs
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
};


/**
* ResponseHeader
* Part: 4
* Chapter: 7.27
* Page: 133
*/
struct BED_ResponseHeader
{
	UA_DateTime timestamp;
	BED_IntegerId requestHandle;
	struct UA_StatusCode serviceResult;
	struct UA_DiagnosticInfo serviceDiagnostics;
	struct UA_String stringTable[];
//ToDo	struct BED_ExtensibleParameterAdditionalHeader additionalHeader;		//ToDo
};


/**
* ServiceFault
* Part: 4
* Chapter: 7.28
* Page: 133
*/
struct BED_ServiceFault
{
	struct BED_ResponseHeader responseHeader;
};


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
struct BED_SignatureData
{
	struct UA_ByteString signature;
	struct UA_String agorithm;
};


/**
* SignedSoftwareCertificate
* Part: 4
* Chapter: 7.31
* Page: 135
*/
struct BED_SignedSoftwareCertificate
{
	struct UA_String version;
	struct UA_ByteString serialNumber;
	struct UA_String signatureAlgorithm;
	struct UA_ByteString signature;
//ToDo	struct issuer 					//ToDo: ??? struct?
	UA_DateTime validFrom;
	UA_DateTime validTo;
//ToDo	struct subject;					//ToDo: ??? struct?
//ToDo	struct subjectAltName[];			//ToDo: ??? struct?
	struct UA_ByteString publicKey;
	struct UA_String keyUsage[];
	struct UA_ByteString softwareCertificate;
};


/**
* SoftwareCertificate
* Part: 4
* Chapter: 7.32
* Page: 135
*/
struct BED_SoftwareCertificate
{
	struct UA_String productName;
	struct UA_String productUri;
	struct UA_String vendorName;
	struct UA_ByteString vendorProductCertificate;
	struct UA_String softwareVersion;
	struct UA_String buildNumber;
	UA_DateTime buildDate;
	struct UA_String issuedBy;
	UA_DateTime issueDate;
	struct UA_ByteString vendorProductCertificate;
	struct BED_SupportedProfiles supportedProfiles;
};

struct BED_SupportedProfiles
{
	struct UA_String oranizationUri;
	struct UA_String profileId;
	struct UA_String complianceTool;
	UA_DateTime complianceDate;
	enum BED_ComplianceLevel complianceLevel;
	struct UA_String unsupportedUnitIds[];
};

enum BED_ComplianceLevel
{
	UNTESTED_0 = 0,		//the profiled capability has not been tested successfully.
	PARTIAL_1 = 1,		//the profiled capability has been partially tested and has
				//passed critical tests, as defined by the certifying authority.
	SELFTESTED_2 = 2,	//the profiled capability has been successfully tested using a
				//self-test system authorized by the certifying authority.
	CERTIFIED_3 = 3		//the profiled capability has been successfully tested by a
				//testing organisation authorized by the certifying authority.
};


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
enum BED_TimestampsToReturn
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
};


/**
* UserIdentityToken Encrypted Token Format
* Part: 4
* Chapter: 7.35.1
* Page: 140
*/
struct BED_UserIdentityTokenEncryptedTokenFormat
{
	Byte length[4];
	Byte tokenData[];
	Byte serverNonce[];
};


/**
* AnonymousIdentityToken
* Part: 4
* Chapter: 7.35.2
* Page: 141
*/
struct BED_AnonymousIdentityToken
{
	struct UA_String policyId;
};


/**
* UserNameIdentityToken
* Part: 4
* Chapter: 7.35.3
* Page: 141
*/
struct BED_UserNameIdentityToken
{
	struct UA_String policyId;
	struct UA_String userName;
	struct UA_ByteString password;
	struct UA_String encryptionAlogrithm;
};


/**
* X509IdentityTokens
* Part: 4
* Chapter: 7.35.4
* Page: 141
*/
struct BED_X509IdentityTokens
{
	struct UA_String policyId;
	struct UA_ByteString certificateData;
};


/**
* IssuedIdentityToken
* Part: 4
* Chapter: 7.35.5
* Page: 142
*/
struct BED_IssuedIdentityToken
{
	struct UA_String policyId;
	struct UA_ByteString tokenData;
	struct UA_String encryptionAlgorithm;
};


/**
* UserTokenPolicy
* Part: 4
* Chapter: 7.36
* Page: 142
*/
struct BED_UserTokenPolicy
{
	struct UA_String policyId;
	enum BED_UserIdentityTokenType tokenType;
	struct UA_String issuedTokenType;
	struct UA_String issuerEndpointUrl;
	struct UA_String securityPolicyUri;
};

enum BED_UserIdentityTokenType
{
	ANONYMOUS_0 = 0,
	USERNAME_1 = 1,
	CERTIFICATE_2 = 2,
	ISSUEDTOKEN_3 = 3
};


/**
* ViewDescription
* Part: 4
* Chapter: 7.37
* Page: 143
*/
struct BED_ViewDescription
{
	struct UA_NodeId viewId;
	UA_DateTime timestamp;
	UInt32 viewVersion;
};


#endif /* OPCUA_BINARYENCDEC_NEU_H_ */
