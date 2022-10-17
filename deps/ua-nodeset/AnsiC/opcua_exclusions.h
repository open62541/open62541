/* ========================================================================
 * Copyright (c) 2005-2021 The OPC Foundation, Inc. All rights reserved.
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

#ifndef _OpcUa_Exclusions_H_
#define _OpcUa_Exclusions_H_ 1


/*============================================================================
* Service Exclusions
 *===========================================================================*/
/* #define OPCUA_EXCLUDE_FindServers */
/* #define OPCUA_EXCLUDE_FindServersOnNetwork */
/* #define OPCUA_EXCLUDE_GetEndpoints */
/* #define OPCUA_EXCLUDE_RegisterServer */
/* #define OPCUA_EXCLUDE_RegisterServer2 */
/* #define OPCUA_EXCLUDE_OpenSecureChannel */
/* #define OPCUA_EXCLUDE_CloseSecureChannel */
/* #define OPCUA_EXCLUDE_CreateSession */
/* #define OPCUA_EXCLUDE_ActivateSession */
/* #define OPCUA_EXCLUDE_CloseSession */
/* #define OPCUA_EXCLUDE_Cancel */
/* #define OPCUA_EXCLUDE_AddNodes */
/* #define OPCUA_EXCLUDE_AddReferences */
/* #define OPCUA_EXCLUDE_DeleteNodes */
/* #define OPCUA_EXCLUDE_DeleteReferences */
/* #define OPCUA_EXCLUDE_Browse */
/* #define OPCUA_EXCLUDE_BrowseNext */
/* #define OPCUA_EXCLUDE_TranslateBrowsePathsToNodeIds */
/* #define OPCUA_EXCLUDE_RegisterNodes */
/* #define OPCUA_EXCLUDE_UnregisterNodes */
/* #define OPCUA_EXCLUDE_QueryFirst */
/* #define OPCUA_EXCLUDE_QueryNext */
/* #define OPCUA_EXCLUDE_Read */
/* #define OPCUA_EXCLUDE_HistoryRead */
/* #define OPCUA_EXCLUDE_Write */
/* #define OPCUA_EXCLUDE_HistoryUpdate */
/* #define OPCUA_EXCLUDE_Call */
/* #define OPCUA_EXCLUDE_CreateMonitoredItems */
/* #define OPCUA_EXCLUDE_ModifyMonitoredItems */
/* #define OPCUA_EXCLUDE_SetMonitoringMode */
/* #define OPCUA_EXCLUDE_SetTriggering */
/* #define OPCUA_EXCLUDE_DeleteMonitoredItems */
/* #define OPCUA_EXCLUDE_CreateSubscription */
/* #define OPCUA_EXCLUDE_ModifySubscription */
/* #define OPCUA_EXCLUDE_SetPublishingMode */
/* #define OPCUA_EXCLUDE_Publish */
/* #define OPCUA_EXCLUDE_Republish */
/* #define OPCUA_EXCLUDE_TransferSubscriptions */
/* #define OPCUA_EXCLUDE_DeleteSubscriptions */

/*============================================================================
* DataType Exclusions
 *===========================================================================*/
/* #define OPCUA_EXCLUDE_Union */
/* #define OPCUA_EXCLUDE_KeyValuePair */
/* #define OPCUA_EXCLUDE_AdditionalParametersType */
/* #define OPCUA_EXCLUDE_EphemeralKeyType */
/* #define OPCUA_EXCLUDE_EndpointType */
/* #define OPCUA_EXCLUDE_RationalNumber */
/* #define OPCUA_EXCLUDE_Vector */
/* #define OPCUA_EXCLUDE_ThreeDVector */
/* #define OPCUA_EXCLUDE_CartesianCoordinates */
/* #define OPCUA_EXCLUDE_ThreeDCartesianCoordinates */
/* #define OPCUA_EXCLUDE_Orientation */
/* #define OPCUA_EXCLUDE_ThreeDOrientation */
/* #define OPCUA_EXCLUDE_Frame */
/* #define OPCUA_EXCLUDE_ThreeDFrame */
/* #define OPCUA_EXCLUDE_IdentityMappingRuleType */
/* #define OPCUA_EXCLUDE_CurrencyUnitType */
/* #define OPCUA_EXCLUDE_TrustListDataType */
/* #define OPCUA_EXCLUDE_DecimalDataType */
/* #define OPCUA_EXCLUDE_DataTypeSchemaHeader */
/* #define OPCUA_EXCLUDE_DataTypeDescription */
/* #define OPCUA_EXCLUDE_StructureDescription */
/* #define OPCUA_EXCLUDE_EnumDescription */
/* #define OPCUA_EXCLUDE_SimpleTypeDescription */
/* #define OPCUA_EXCLUDE_UABinaryFileDataType */
/* #define OPCUA_EXCLUDE_PortableQualifiedName */
/* #define OPCUA_EXCLUDE_PortableNodeId */
/* #define OPCUA_EXCLUDE_UnsignedRationalNumber */
/* #define OPCUA_EXCLUDE_DataSetMetaDataType */
/* #define OPCUA_EXCLUDE_FieldMetaData */
/* #define OPCUA_EXCLUDE_ConfigurationVersionDataType */
/* #define OPCUA_EXCLUDE_PublishedDataSetDataType */
/* #define OPCUA_EXCLUDE_PublishedDataSetSourceDataType */
/* #define OPCUA_EXCLUDE_PublishedVariableDataType */
/* #define OPCUA_EXCLUDE_PublishedDataItemsDataType */
/* #define OPCUA_EXCLUDE_PublishedEventsDataType */
/* #define OPCUA_EXCLUDE_PublishedDataSetCustomSourceDataType */
/* #define OPCUA_EXCLUDE_DataSetWriterDataType */
/* #define OPCUA_EXCLUDE_DataSetWriterTransportDataType */
/* #define OPCUA_EXCLUDE_DataSetWriterMessageDataType */
/* #define OPCUA_EXCLUDE_PubSubGroupDataType */
/* #define OPCUA_EXCLUDE_WriterGroupDataType */
/* #define OPCUA_EXCLUDE_WriterGroupTransportDataType */
/* #define OPCUA_EXCLUDE_WriterGroupMessageDataType */
/* #define OPCUA_EXCLUDE_PubSubConnectionDataType */
/* #define OPCUA_EXCLUDE_ConnectionTransportDataType */
/* #define OPCUA_EXCLUDE_NetworkAddressDataType */
/* #define OPCUA_EXCLUDE_NetworkAddressUrlDataType */
/* #define OPCUA_EXCLUDE_ReaderGroupDataType */
/* #define OPCUA_EXCLUDE_ReaderGroupTransportDataType */
/* #define OPCUA_EXCLUDE_ReaderGroupMessageDataType */
/* #define OPCUA_EXCLUDE_DataSetReaderDataType */
/* #define OPCUA_EXCLUDE_DataSetReaderTransportDataType */
/* #define OPCUA_EXCLUDE_DataSetReaderMessageDataType */
/* #define OPCUA_EXCLUDE_SubscribedDataSetDataType */
/* #define OPCUA_EXCLUDE_TargetVariablesDataType */
/* #define OPCUA_EXCLUDE_FieldTargetDataType */
/* #define OPCUA_EXCLUDE_SubscribedDataSetMirrorDataType */
/* #define OPCUA_EXCLUDE_PubSubConfigurationDataType */
/* #define OPCUA_EXCLUDE_StandaloneSubscribedDataSetRefDataType */
/* #define OPCUA_EXCLUDE_StandaloneSubscribedDataSetDataType */
/* #define OPCUA_EXCLUDE_SecurityGroupDataType */
/* #define OPCUA_EXCLUDE_PubSubKeyPushTargetDataType */
/* #define OPCUA_EXCLUDE_PubSubConfiguration2DataType */
/* #define OPCUA_EXCLUDE_UadpWriterGroupMessageDataType */
/* #define OPCUA_EXCLUDE_UadpDataSetWriterMessageDataType */
/* #define OPCUA_EXCLUDE_UadpDataSetReaderMessageDataType */
/* #define OPCUA_EXCLUDE_JsonWriterGroupMessageDataType */
/* #define OPCUA_EXCLUDE_JsonDataSetWriterMessageDataType */
/* #define OPCUA_EXCLUDE_JsonDataSetReaderMessageDataType */
/* #define OPCUA_EXCLUDE_QosDataType */
/* #define OPCUA_EXCLUDE_TransmitQosDataType */
/* #define OPCUA_EXCLUDE_TransmitQosPriorityDataType */
/* #define OPCUA_EXCLUDE_ReceiveQosDataType */
/* #define OPCUA_EXCLUDE_ReceiveQosPriorityDataType */
/* #define OPCUA_EXCLUDE_DatagramConnectionTransportDataType */
/* #define OPCUA_EXCLUDE_DatagramConnectionTransport2DataType */
/* #define OPCUA_EXCLUDE_DatagramWriterGroupTransportDataType */
/* #define OPCUA_EXCLUDE_DatagramWriterGroupTransport2DataType */
/* #define OPCUA_EXCLUDE_DatagramDataSetReaderTransportDataType */
/* #define OPCUA_EXCLUDE_BrokerConnectionTransportDataType */
/* #define OPCUA_EXCLUDE_BrokerWriterGroupTransportDataType */
/* #define OPCUA_EXCLUDE_BrokerDataSetWriterTransportDataType */
/* #define OPCUA_EXCLUDE_BrokerDataSetReaderTransportDataType */
/* #define OPCUA_EXCLUDE_PubSubConfigurationRefDataType */
/* #define OPCUA_EXCLUDE_PubSubConfigurationValueDataType */
/* #define OPCUA_EXCLUDE_AliasNameDataType */
/* #define OPCUA_EXCLUDE_UserManagementDataType */
/* #define OPCUA_EXCLUDE_PriorityMappingEntryType */
/* #define OPCUA_EXCLUDE_RolePermissionType */
/* #define OPCUA_EXCLUDE_DataTypeDefinition */
/* #define OPCUA_EXCLUDE_StructureField */
/* #define OPCUA_EXCLUDE_StructureDefinition */
/* #define OPCUA_EXCLUDE_EnumDefinition */
/* #define OPCUA_EXCLUDE_Node */
/* #define OPCUA_EXCLUDE_InstanceNode */
/* #define OPCUA_EXCLUDE_TypeNode */
/* #define OPCUA_EXCLUDE_ObjectNode */
/* #define OPCUA_EXCLUDE_ObjectTypeNode */
/* #define OPCUA_EXCLUDE_VariableNode */
/* #define OPCUA_EXCLUDE_VariableTypeNode */
/* #define OPCUA_EXCLUDE_ReferenceTypeNode */
/* #define OPCUA_EXCLUDE_MethodNode */
/* #define OPCUA_EXCLUDE_ViewNode */
/* #define OPCUA_EXCLUDE_DataTypeNode */
/* #define OPCUA_EXCLUDE_ReferenceNode */
/* #define OPCUA_EXCLUDE_Argument */
/* #define OPCUA_EXCLUDE_EnumValueType */
/* #define OPCUA_EXCLUDE_EnumField */
/* #define OPCUA_EXCLUDE_OptionSet */
/* #define OPCUA_EXCLUDE_TimeZoneDataType */
/* #define OPCUA_EXCLUDE_ApplicationDescription */
/* #define OPCUA_EXCLUDE_RequestHeader */
/* #define OPCUA_EXCLUDE_ResponseHeader */
/* #define OPCUA_EXCLUDE_ServiceFault */
/* #define OPCUA_EXCLUDE_SessionlessInvokeRequestType */
/* #define OPCUA_EXCLUDE_SessionlessInvokeResponseType */
/* #define OPCUA_EXCLUDE_ServerOnNetwork */
/* #define OPCUA_EXCLUDE_UserTokenPolicy */
/* #define OPCUA_EXCLUDE_EndpointDescription */
/* #define OPCUA_EXCLUDE_RegisteredServer */
/* #define OPCUA_EXCLUDE_DiscoveryConfiguration */
/* #define OPCUA_EXCLUDE_MdnsDiscoveryConfiguration */
/* #define OPCUA_EXCLUDE_ChannelSecurityToken */
/* #define OPCUA_EXCLUDE_SignedSoftwareCertificate */
/* #define OPCUA_EXCLUDE_SignatureData */
/* #define OPCUA_EXCLUDE_UserIdentityToken */
/* #define OPCUA_EXCLUDE_AnonymousIdentityToken */
/* #define OPCUA_EXCLUDE_UserNameIdentityToken */
/* #define OPCUA_EXCLUDE_X509IdentityToken */
/* #define OPCUA_EXCLUDE_IssuedIdentityToken */
/* #define OPCUA_EXCLUDE_NodeAttributes */
/* #define OPCUA_EXCLUDE_ObjectAttributes */
/* #define OPCUA_EXCLUDE_VariableAttributes */
/* #define OPCUA_EXCLUDE_MethodAttributes */
/* #define OPCUA_EXCLUDE_ObjectTypeAttributes */
/* #define OPCUA_EXCLUDE_VariableTypeAttributes */
/* #define OPCUA_EXCLUDE_ReferenceTypeAttributes */
/* #define OPCUA_EXCLUDE_DataTypeAttributes */
/* #define OPCUA_EXCLUDE_ViewAttributes */
/* #define OPCUA_EXCLUDE_GenericAttributeValue */
/* #define OPCUA_EXCLUDE_GenericAttributes */
/* #define OPCUA_EXCLUDE_AddNodesItem */
/* #define OPCUA_EXCLUDE_AddNodesResult */
/* #define OPCUA_EXCLUDE_AddReferencesItem */
/* #define OPCUA_EXCLUDE_DeleteNodesItem */
/* #define OPCUA_EXCLUDE_DeleteReferencesItem */
/* #define OPCUA_EXCLUDE_ViewDescription */
/* #define OPCUA_EXCLUDE_BrowseDescription */
/* #define OPCUA_EXCLUDE_ReferenceDescription */
/* #define OPCUA_EXCLUDE_BrowseResult */
/* #define OPCUA_EXCLUDE_RelativePathElement */
/* #define OPCUA_EXCLUDE_RelativePath */
/* #define OPCUA_EXCLUDE_BrowsePath */
/* #define OPCUA_EXCLUDE_BrowsePathTarget */
/* #define OPCUA_EXCLUDE_BrowsePathResult */
/* #define OPCUA_EXCLUDE_EndpointConfiguration */
/* #define OPCUA_EXCLUDE_QueryDataDescription */
/* #define OPCUA_EXCLUDE_NodeTypeDescription */
/* #define OPCUA_EXCLUDE_QueryDataSet */
/* #define OPCUA_EXCLUDE_NodeReference */
/* #define OPCUA_EXCLUDE_ContentFilterElement */
/* #define OPCUA_EXCLUDE_ContentFilter */
/* #define OPCUA_EXCLUDE_FilterOperand */
/* #define OPCUA_EXCLUDE_ElementOperand */
/* #define OPCUA_EXCLUDE_LiteralOperand */
/* #define OPCUA_EXCLUDE_AttributeOperand */
/* #define OPCUA_EXCLUDE_SimpleAttributeOperand */
/* #define OPCUA_EXCLUDE_ContentFilterElementResult */
/* #define OPCUA_EXCLUDE_ContentFilterResult */
/* #define OPCUA_EXCLUDE_ParsingResult */
/* #define OPCUA_EXCLUDE_ReadValueId */
/* #define OPCUA_EXCLUDE_HistoryReadValueId */
/* #define OPCUA_EXCLUDE_HistoryReadResult */
/* #define OPCUA_EXCLUDE_HistoryReadDetails */
/* #define OPCUA_EXCLUDE_ReadEventDetails */
/* #define OPCUA_EXCLUDE_ReadRawModifiedDetails */
/* #define OPCUA_EXCLUDE_ReadProcessedDetails */
/* #define OPCUA_EXCLUDE_ReadAtTimeDetails */
/* #define OPCUA_EXCLUDE_ReadAnnotationDataDetails */
/* #define OPCUA_EXCLUDE_HistoryData */
/* #define OPCUA_EXCLUDE_ModificationInfo */
/* #define OPCUA_EXCLUDE_HistoryModifiedData */
/* #define OPCUA_EXCLUDE_HistoryEvent */
/* #define OPCUA_EXCLUDE_WriteValue */
/* #define OPCUA_EXCLUDE_HistoryUpdateDetails */
/* #define OPCUA_EXCLUDE_UpdateDataDetails */
/* #define OPCUA_EXCLUDE_UpdateStructureDataDetails */
/* #define OPCUA_EXCLUDE_UpdateEventDetails */
/* #define OPCUA_EXCLUDE_DeleteRawModifiedDetails */
/* #define OPCUA_EXCLUDE_DeleteAtTimeDetails */
/* #define OPCUA_EXCLUDE_DeleteEventDetails */
/* #define OPCUA_EXCLUDE_HistoryUpdateResult */
/* #define OPCUA_EXCLUDE_CallMethodRequest */
/* #define OPCUA_EXCLUDE_CallMethodResult */
/* #define OPCUA_EXCLUDE_MonitoringFilter */
/* #define OPCUA_EXCLUDE_DataChangeFilter */
/* #define OPCUA_EXCLUDE_EventFilter */
/* #define OPCUA_EXCLUDE_AggregateConfiguration */
/* #define OPCUA_EXCLUDE_AggregateFilter */
/* #define OPCUA_EXCLUDE_MonitoringFilterResult */
/* #define OPCUA_EXCLUDE_EventFilterResult */
/* #define OPCUA_EXCLUDE_AggregateFilterResult */
/* #define OPCUA_EXCLUDE_MonitoringParameters */
/* #define OPCUA_EXCLUDE_MonitoredItemCreateRequest */
/* #define OPCUA_EXCLUDE_MonitoredItemCreateResult */
/* #define OPCUA_EXCLUDE_MonitoredItemModifyRequest */
/* #define OPCUA_EXCLUDE_MonitoredItemModifyResult */
/* #define OPCUA_EXCLUDE_NotificationMessage */
/* #define OPCUA_EXCLUDE_NotificationData */
/* #define OPCUA_EXCLUDE_DataChangeNotification */
/* #define OPCUA_EXCLUDE_MonitoredItemNotification */
/* #define OPCUA_EXCLUDE_EventNotificationList */
/* #define OPCUA_EXCLUDE_EventFieldList */
/* #define OPCUA_EXCLUDE_HistoryEventFieldList */
/* #define OPCUA_EXCLUDE_StatusChangeNotification */
/* #define OPCUA_EXCLUDE_SubscriptionAcknowledgement */
/* #define OPCUA_EXCLUDE_TransferResult */
/* #define OPCUA_EXCLUDE_BuildInfo */
/* #define OPCUA_EXCLUDE_RedundantServerDataType */
/* #define OPCUA_EXCLUDE_EndpointUrlListDataType */
/* #define OPCUA_EXCLUDE_NetworkGroupDataType */
/* #define OPCUA_EXCLUDE_SamplingIntervalDiagnosticsDataType */
/* #define OPCUA_EXCLUDE_ServerDiagnosticsSummaryDataType */
/* #define OPCUA_EXCLUDE_ServerStatusDataType */
/* #define OPCUA_EXCLUDE_SessionDiagnosticsDataType */
/* #define OPCUA_EXCLUDE_SessionSecurityDiagnosticsDataType */
/* #define OPCUA_EXCLUDE_ServiceCounterDataType */
/* #define OPCUA_EXCLUDE_StatusResult */
/* #define OPCUA_EXCLUDE_SubscriptionDiagnosticsDataType */
/* #define OPCUA_EXCLUDE_ModelChangeStructureDataType */
/* #define OPCUA_EXCLUDE_SemanticChangeStructureDataType */
/* #define OPCUA_EXCLUDE_Range */
/* #define OPCUA_EXCLUDE_EUInformation */
/* #define OPCUA_EXCLUDE_ComplexNumberType */
/* #define OPCUA_EXCLUDE_DoubleComplexNumberType */
/* #define OPCUA_EXCLUDE_AxisInformation */
/* #define OPCUA_EXCLUDE_XVType */
/* #define OPCUA_EXCLUDE_ProgramDiagnosticDataType */
/* #define OPCUA_EXCLUDE_ProgramDiagnostic2DataType */
/* #define OPCUA_EXCLUDE_Annotation */

#endif /* _OpcUa_Exclusions_H_ */
/* This is the last line of an autogenerated file. */
