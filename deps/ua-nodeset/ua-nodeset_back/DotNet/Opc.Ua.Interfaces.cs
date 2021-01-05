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

using System;
using System.Collections.Generic;
using System.Xml;
using System.ServiceModel;
using System.Runtime.Serialization;

namespace Opc.Ua
{
    #region ISessionEndpoint Interface
    #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
    /// <summary>
    /// The service contract which must be implemented by all UA servers.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.CodeGenerator", "1.0.0.0")]
    [ServiceContract(Namespace = Namespaces.OpcUaWsdl)]
    public interface ISessionEndpoint : IEndpointBase
    {
        #if (!OPCUA_EXCLUDE_CreateSession)
        /// <summary>
        /// The operation contract for the CreateSession service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/CreateSession", ReplyAction = Namespaces.OpcUaWsdl + "/CreateSessionResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/CreateSessionFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        CreateSessionResponseMessage CreateSession(CreateSessionMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_ActivateSession)
        /// <summary>
        /// The operation contract for the ActivateSession service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/ActivateSession", ReplyAction = Namespaces.OpcUaWsdl + "/ActivateSessionResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/ActivateSessionFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        ActivateSessionResponseMessage ActivateSession(ActivateSessionMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_CloseSession)
        /// <summary>
        /// The operation contract for the CloseSession service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/CloseSession", ReplyAction = Namespaces.OpcUaWsdl + "/CloseSessionResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/CloseSessionFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        CloseSessionResponseMessage CloseSession(CloseSessionMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_Cancel)
        /// <summary>
        /// The operation contract for the Cancel service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/Cancel", ReplyAction = Namespaces.OpcUaWsdl + "/CancelResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/CancelFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        CancelResponseMessage Cancel(CancelMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_AddNodes)
        /// <summary>
        /// The operation contract for the AddNodes service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/AddNodes", ReplyAction = Namespaces.OpcUaWsdl + "/AddNodesResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/AddNodesFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        AddNodesResponseMessage AddNodes(AddNodesMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_AddReferences)
        /// <summary>
        /// The operation contract for the AddReferences service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/AddReferences", ReplyAction = Namespaces.OpcUaWsdl + "/AddReferencesResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/AddReferencesFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        AddReferencesResponseMessage AddReferences(AddReferencesMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_DeleteNodes)
        /// <summary>
        /// The operation contract for the DeleteNodes service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/DeleteNodes", ReplyAction = Namespaces.OpcUaWsdl + "/DeleteNodesResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/DeleteNodesFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        DeleteNodesResponseMessage DeleteNodes(DeleteNodesMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_DeleteReferences)
        /// <summary>
        /// The operation contract for the DeleteReferences service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/DeleteReferences", ReplyAction = Namespaces.OpcUaWsdl + "/DeleteReferencesResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/DeleteReferencesFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        DeleteReferencesResponseMessage DeleteReferences(DeleteReferencesMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_Browse)
        /// <summary>
        /// The operation contract for the Browse service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/Browse", ReplyAction = Namespaces.OpcUaWsdl + "/BrowseResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/BrowseFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        BrowseResponseMessage Browse(BrowseMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_BrowseNext)
        /// <summary>
        /// The operation contract for the BrowseNext service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/BrowseNext", ReplyAction = Namespaces.OpcUaWsdl + "/BrowseNextResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/BrowseNextFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        BrowseNextResponseMessage BrowseNext(BrowseNextMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_TranslateBrowsePathsToNodeIds)
        /// <summary>
        /// The operation contract for the TranslateBrowsePathsToNodeIds service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/TranslateBrowsePathsToNodeIds", ReplyAction = Namespaces.OpcUaWsdl + "/TranslateBrowsePathsToNodeIdsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/TranslateBrowsePathsToNodeIdsFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        TranslateBrowsePathsToNodeIdsResponseMessage TranslateBrowsePathsToNodeIds(TranslateBrowsePathsToNodeIdsMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_RegisterNodes)
        /// <summary>
        /// The operation contract for the RegisterNodes service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/RegisterNodes", ReplyAction = Namespaces.OpcUaWsdl + "/RegisterNodesResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/RegisterNodesFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        RegisterNodesResponseMessage RegisterNodes(RegisterNodesMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_UnregisterNodes)
        /// <summary>
        /// The operation contract for the UnregisterNodes service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/UnregisterNodes", ReplyAction = Namespaces.OpcUaWsdl + "/UnregisterNodesResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/UnregisterNodesFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        UnregisterNodesResponseMessage UnregisterNodes(UnregisterNodesMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_QueryFirst)
        /// <summary>
        /// The operation contract for the QueryFirst service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/QueryFirst", ReplyAction = Namespaces.OpcUaWsdl + "/QueryFirstResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/QueryFirstFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        QueryFirstResponseMessage QueryFirst(QueryFirstMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_QueryNext)
        /// <summary>
        /// The operation contract for the QueryNext service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/QueryNext", ReplyAction = Namespaces.OpcUaWsdl + "/QueryNextResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/QueryNextFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        QueryNextResponseMessage QueryNext(QueryNextMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_Read)
        /// <summary>
        /// The operation contract for the Read service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/Read", ReplyAction = Namespaces.OpcUaWsdl + "/ReadResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/ReadFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        ReadResponseMessage Read(ReadMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_HistoryRead)
        /// <summary>
        /// The operation contract for the HistoryRead service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/HistoryRead", ReplyAction = Namespaces.OpcUaWsdl + "/HistoryReadResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/HistoryReadFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        HistoryReadResponseMessage HistoryRead(HistoryReadMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_Write)
        /// <summary>
        /// The operation contract for the Write service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/Write", ReplyAction = Namespaces.OpcUaWsdl + "/WriteResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/WriteFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        WriteResponseMessage Write(WriteMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_HistoryUpdate)
        /// <summary>
        /// The operation contract for the HistoryUpdate service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/HistoryUpdate", ReplyAction = Namespaces.OpcUaWsdl + "/HistoryUpdateResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/HistoryUpdateFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        HistoryUpdateResponseMessage HistoryUpdate(HistoryUpdateMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_Call)
        /// <summary>
        /// The operation contract for the Call service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/Call", ReplyAction = Namespaces.OpcUaWsdl + "/CallResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/CallFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        CallResponseMessage Call(CallMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_CreateMonitoredItems)
        /// <summary>
        /// The operation contract for the CreateMonitoredItems service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/CreateMonitoredItems", ReplyAction = Namespaces.OpcUaWsdl + "/CreateMonitoredItemsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/CreateMonitoredItemsFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        CreateMonitoredItemsResponseMessage CreateMonitoredItems(CreateMonitoredItemsMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_ModifyMonitoredItems)
        /// <summary>
        /// The operation contract for the ModifyMonitoredItems service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/ModifyMonitoredItems", ReplyAction = Namespaces.OpcUaWsdl + "/ModifyMonitoredItemsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/ModifyMonitoredItemsFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        ModifyMonitoredItemsResponseMessage ModifyMonitoredItems(ModifyMonitoredItemsMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_SetMonitoringMode)
        /// <summary>
        /// The operation contract for the SetMonitoringMode service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/SetMonitoringMode", ReplyAction = Namespaces.OpcUaWsdl + "/SetMonitoringModeResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/SetMonitoringModeFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        SetMonitoringModeResponseMessage SetMonitoringMode(SetMonitoringModeMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_SetTriggering)
        /// <summary>
        /// The operation contract for the SetTriggering service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/SetTriggering", ReplyAction = Namespaces.OpcUaWsdl + "/SetTriggeringResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/SetTriggeringFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        SetTriggeringResponseMessage SetTriggering(SetTriggeringMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_DeleteMonitoredItems)
        /// <summary>
        /// The operation contract for the DeleteMonitoredItems service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/DeleteMonitoredItems", ReplyAction = Namespaces.OpcUaWsdl + "/DeleteMonitoredItemsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/DeleteMonitoredItemsFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        DeleteMonitoredItemsResponseMessage DeleteMonitoredItems(DeleteMonitoredItemsMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_CreateSubscription)
        /// <summary>
        /// The operation contract for the CreateSubscription service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/CreateSubscription", ReplyAction = Namespaces.OpcUaWsdl + "/CreateSubscriptionResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/CreateSubscriptionFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        CreateSubscriptionResponseMessage CreateSubscription(CreateSubscriptionMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_ModifySubscription)
        /// <summary>
        /// The operation contract for the ModifySubscription service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/ModifySubscription", ReplyAction = Namespaces.OpcUaWsdl + "/ModifySubscriptionResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/ModifySubscriptionFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        ModifySubscriptionResponseMessage ModifySubscription(ModifySubscriptionMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_SetPublishingMode)
        /// <summary>
        /// The operation contract for the SetPublishingMode service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/SetPublishingMode", ReplyAction = Namespaces.OpcUaWsdl + "/SetPublishingModeResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/SetPublishingModeFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        SetPublishingModeResponseMessage SetPublishingMode(SetPublishingModeMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_Publish)
        /// <summary>
        /// The operation contract for the Publish service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/Publish", ReplyAction = Namespaces.OpcUaWsdl + "/PublishResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/PublishFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        PublishResponseMessage Publish(PublishMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_Republish)
        /// <summary>
        /// The operation contract for the Republish service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/Republish", ReplyAction = Namespaces.OpcUaWsdl + "/RepublishResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/RepublishFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        RepublishResponseMessage Republish(RepublishMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_TransferSubscriptions)
        /// <summary>
        /// The operation contract for the TransferSubscriptions service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/TransferSubscriptions", ReplyAction = Namespaces.OpcUaWsdl + "/TransferSubscriptionsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/TransferSubscriptionsFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        TransferSubscriptionsResponseMessage TransferSubscriptions(TransferSubscriptionsMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_DeleteSubscriptions)
        /// <summary>
        /// The operation contract for the DeleteSubscriptions service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/DeleteSubscriptions", ReplyAction = Namespaces.OpcUaWsdl + "/DeleteSubscriptionsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/DeleteSubscriptionsFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        DeleteSubscriptionsResponseMessage DeleteSubscriptions(DeleteSubscriptionsMessage request);
        #endif
    }
    #else
    /// <summary>
    /// The asynchronous service contract which must be implemented by UA servers.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.CodeGenerator", "1.0.0.0")]
    [ServiceContract(Namespace = Namespaces.OpcUaWsdl)]
    public interface ISessionEndpoint : IEndpointBase
    {
        #if (!OPCUA_EXCLUDE_CreateSession)
        /// <summary>
        /// The operation contract for the CreateSession service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/CreateSession", ReplyAction = Namespaces.OpcUaWsdl + "/CreateSessionResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/CreateSessionFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginCreateSession(CreateSessionMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a CreateSession service request.
        /// </summary>
        CreateSessionResponseMessage EndCreateSession(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_ActivateSession)
        /// <summary>
        /// The operation contract for the ActivateSession service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/ActivateSession", ReplyAction = Namespaces.OpcUaWsdl + "/ActivateSessionResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/ActivateSessionFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginActivateSession(ActivateSessionMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a ActivateSession service request.
        /// </summary>
        ActivateSessionResponseMessage EndActivateSession(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_CloseSession)
        /// <summary>
        /// The operation contract for the CloseSession service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/CloseSession", ReplyAction = Namespaces.OpcUaWsdl + "/CloseSessionResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/CloseSessionFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginCloseSession(CloseSessionMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a CloseSession service request.
        /// </summary>
        CloseSessionResponseMessage EndCloseSession(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_Cancel)
        /// <summary>
        /// The operation contract for the Cancel service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/Cancel", ReplyAction = Namespaces.OpcUaWsdl + "/CancelResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/CancelFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginCancel(CancelMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a Cancel service request.
        /// </summary>
        CancelResponseMessage EndCancel(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_AddNodes)
        /// <summary>
        /// The operation contract for the AddNodes service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/AddNodes", ReplyAction = Namespaces.OpcUaWsdl + "/AddNodesResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/AddNodesFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginAddNodes(AddNodesMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a AddNodes service request.
        /// </summary>
        AddNodesResponseMessage EndAddNodes(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_AddReferences)
        /// <summary>
        /// The operation contract for the AddReferences service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/AddReferences", ReplyAction = Namespaces.OpcUaWsdl + "/AddReferencesResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/AddReferencesFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginAddReferences(AddReferencesMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a AddReferences service request.
        /// </summary>
        AddReferencesResponseMessage EndAddReferences(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_DeleteNodes)
        /// <summary>
        /// The operation contract for the DeleteNodes service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/DeleteNodes", ReplyAction = Namespaces.OpcUaWsdl + "/DeleteNodesResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/DeleteNodesFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginDeleteNodes(DeleteNodesMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a DeleteNodes service request.
        /// </summary>
        DeleteNodesResponseMessage EndDeleteNodes(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_DeleteReferences)
        /// <summary>
        /// The operation contract for the DeleteReferences service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/DeleteReferences", ReplyAction = Namespaces.OpcUaWsdl + "/DeleteReferencesResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/DeleteReferencesFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginDeleteReferences(DeleteReferencesMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a DeleteReferences service request.
        /// </summary>
        DeleteReferencesResponseMessage EndDeleteReferences(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_Browse)
        /// <summary>
        /// The operation contract for the Browse service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/Browse", ReplyAction = Namespaces.OpcUaWsdl + "/BrowseResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/BrowseFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginBrowse(BrowseMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a Browse service request.
        /// </summary>
        BrowseResponseMessage EndBrowse(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_BrowseNext)
        /// <summary>
        /// The operation contract for the BrowseNext service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/BrowseNext", ReplyAction = Namespaces.OpcUaWsdl + "/BrowseNextResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/BrowseNextFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginBrowseNext(BrowseNextMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a BrowseNext service request.
        /// </summary>
        BrowseNextResponseMessage EndBrowseNext(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_TranslateBrowsePathsToNodeIds)
        /// <summary>
        /// The operation contract for the TranslateBrowsePathsToNodeIds service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/TranslateBrowsePathsToNodeIds", ReplyAction = Namespaces.OpcUaWsdl + "/TranslateBrowsePathsToNodeIdsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/TranslateBrowsePathsToNodeIdsFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginTranslateBrowsePathsToNodeIds(TranslateBrowsePathsToNodeIdsMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a TranslateBrowsePathsToNodeIds service request.
        /// </summary>
        TranslateBrowsePathsToNodeIdsResponseMessage EndTranslateBrowsePathsToNodeIds(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_RegisterNodes)
        /// <summary>
        /// The operation contract for the RegisterNodes service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/RegisterNodes", ReplyAction = Namespaces.OpcUaWsdl + "/RegisterNodesResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/RegisterNodesFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginRegisterNodes(RegisterNodesMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a RegisterNodes service request.
        /// </summary>
        RegisterNodesResponseMessage EndRegisterNodes(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_UnregisterNodes)
        /// <summary>
        /// The operation contract for the UnregisterNodes service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/UnregisterNodes", ReplyAction = Namespaces.OpcUaWsdl + "/UnregisterNodesResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/UnregisterNodesFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginUnregisterNodes(UnregisterNodesMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a UnregisterNodes service request.
        /// </summary>
        UnregisterNodesResponseMessage EndUnregisterNodes(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_QueryFirst)
        /// <summary>
        /// The operation contract for the QueryFirst service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/QueryFirst", ReplyAction = Namespaces.OpcUaWsdl + "/QueryFirstResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/QueryFirstFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginQueryFirst(QueryFirstMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a QueryFirst service request.
        /// </summary>
        QueryFirstResponseMessage EndQueryFirst(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_QueryNext)
        /// <summary>
        /// The operation contract for the QueryNext service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/QueryNext", ReplyAction = Namespaces.OpcUaWsdl + "/QueryNextResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/QueryNextFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginQueryNext(QueryNextMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a QueryNext service request.
        /// </summary>
        QueryNextResponseMessage EndQueryNext(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_Read)
        /// <summary>
        /// The operation contract for the Read service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/Read", ReplyAction = Namespaces.OpcUaWsdl + "/ReadResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/ReadFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginRead(ReadMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a Read service request.
        /// </summary>
        ReadResponseMessage EndRead(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_HistoryRead)
        /// <summary>
        /// The operation contract for the HistoryRead service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/HistoryRead", ReplyAction = Namespaces.OpcUaWsdl + "/HistoryReadResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/HistoryReadFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginHistoryRead(HistoryReadMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a HistoryRead service request.
        /// </summary>
        HistoryReadResponseMessage EndHistoryRead(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_Write)
        /// <summary>
        /// The operation contract for the Write service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/Write", ReplyAction = Namespaces.OpcUaWsdl + "/WriteResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/WriteFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginWrite(WriteMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a Write service request.
        /// </summary>
        WriteResponseMessage EndWrite(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_HistoryUpdate)
        /// <summary>
        /// The operation contract for the HistoryUpdate service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/HistoryUpdate", ReplyAction = Namespaces.OpcUaWsdl + "/HistoryUpdateResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/HistoryUpdateFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginHistoryUpdate(HistoryUpdateMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a HistoryUpdate service request.
        /// </summary>
        HistoryUpdateResponseMessage EndHistoryUpdate(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_Call)
        /// <summary>
        /// The operation contract for the Call service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/Call", ReplyAction = Namespaces.OpcUaWsdl + "/CallResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/CallFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginCall(CallMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a Call service request.
        /// </summary>
        CallResponseMessage EndCall(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_CreateMonitoredItems)
        /// <summary>
        /// The operation contract for the CreateMonitoredItems service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/CreateMonitoredItems", ReplyAction = Namespaces.OpcUaWsdl + "/CreateMonitoredItemsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/CreateMonitoredItemsFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginCreateMonitoredItems(CreateMonitoredItemsMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a CreateMonitoredItems service request.
        /// </summary>
        CreateMonitoredItemsResponseMessage EndCreateMonitoredItems(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_ModifyMonitoredItems)
        /// <summary>
        /// The operation contract for the ModifyMonitoredItems service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/ModifyMonitoredItems", ReplyAction = Namespaces.OpcUaWsdl + "/ModifyMonitoredItemsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/ModifyMonitoredItemsFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginModifyMonitoredItems(ModifyMonitoredItemsMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a ModifyMonitoredItems service request.
        /// </summary>
        ModifyMonitoredItemsResponseMessage EndModifyMonitoredItems(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_SetMonitoringMode)
        /// <summary>
        /// The operation contract for the SetMonitoringMode service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/SetMonitoringMode", ReplyAction = Namespaces.OpcUaWsdl + "/SetMonitoringModeResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/SetMonitoringModeFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginSetMonitoringMode(SetMonitoringModeMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a SetMonitoringMode service request.
        /// </summary>
        SetMonitoringModeResponseMessage EndSetMonitoringMode(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_SetTriggering)
        /// <summary>
        /// The operation contract for the SetTriggering service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/SetTriggering", ReplyAction = Namespaces.OpcUaWsdl + "/SetTriggeringResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/SetTriggeringFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginSetTriggering(SetTriggeringMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a SetTriggering service request.
        /// </summary>
        SetTriggeringResponseMessage EndSetTriggering(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_DeleteMonitoredItems)
        /// <summary>
        /// The operation contract for the DeleteMonitoredItems service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/DeleteMonitoredItems", ReplyAction = Namespaces.OpcUaWsdl + "/DeleteMonitoredItemsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/DeleteMonitoredItemsFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginDeleteMonitoredItems(DeleteMonitoredItemsMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a DeleteMonitoredItems service request.
        /// </summary>
        DeleteMonitoredItemsResponseMessage EndDeleteMonitoredItems(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_CreateSubscription)
        /// <summary>
        /// The operation contract for the CreateSubscription service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/CreateSubscription", ReplyAction = Namespaces.OpcUaWsdl + "/CreateSubscriptionResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/CreateSubscriptionFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginCreateSubscription(CreateSubscriptionMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a CreateSubscription service request.
        /// </summary>
        CreateSubscriptionResponseMessage EndCreateSubscription(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_ModifySubscription)
        /// <summary>
        /// The operation contract for the ModifySubscription service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/ModifySubscription", ReplyAction = Namespaces.OpcUaWsdl + "/ModifySubscriptionResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/ModifySubscriptionFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginModifySubscription(ModifySubscriptionMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a ModifySubscription service request.
        /// </summary>
        ModifySubscriptionResponseMessage EndModifySubscription(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_SetPublishingMode)
        /// <summary>
        /// The operation contract for the SetPublishingMode service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/SetPublishingMode", ReplyAction = Namespaces.OpcUaWsdl + "/SetPublishingModeResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/SetPublishingModeFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginSetPublishingMode(SetPublishingModeMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a SetPublishingMode service request.
        /// </summary>
        SetPublishingModeResponseMessage EndSetPublishingMode(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_Publish)
        /// <summary>
        /// The operation contract for the Publish service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/Publish", ReplyAction = Namespaces.OpcUaWsdl + "/PublishResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/PublishFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginPublish(PublishMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a Publish service request.
        /// </summary>
        PublishResponseMessage EndPublish(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_Republish)
        /// <summary>
        /// The operation contract for the Republish service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/Republish", ReplyAction = Namespaces.OpcUaWsdl + "/RepublishResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/RepublishFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginRepublish(RepublishMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a Republish service request.
        /// </summary>
        RepublishResponseMessage EndRepublish(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_TransferSubscriptions)
        /// <summary>
        /// The operation contract for the TransferSubscriptions service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/TransferSubscriptions", ReplyAction = Namespaces.OpcUaWsdl + "/TransferSubscriptionsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/TransferSubscriptionsFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginTransferSubscriptions(TransferSubscriptionsMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a TransferSubscriptions service request.
        /// </summary>
        TransferSubscriptionsResponseMessage EndTransferSubscriptions(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_DeleteSubscriptions)
        /// <summary>
        /// The operation contract for the DeleteSubscriptions service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/DeleteSubscriptions", ReplyAction = Namespaces.OpcUaWsdl + "/DeleteSubscriptionsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/DeleteSubscriptionsFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginDeleteSubscriptions(DeleteSubscriptionsMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a DeleteSubscriptions service request.
        /// </summary>
        DeleteSubscriptionsResponseMessage EndDeleteSubscriptions(IAsyncResult result);
        #endif
    }
    #endif
    #endregion

    #region ISessionChannel Interface
    /// <summary>
    /// An interface used by by clients to access a UA server.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.CodeGenerator", "1.0.0.0")]
    [ServiceContract(Namespace = Namespaces.OpcUaWsdl)]
    public interface ISessionChannel : IChannelBase
    {
        #if (!OPCUA_EXCLUDE_CreateSession)
        /// <summary>
        /// The operation contract for the CreateSession service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/CreateSession", ReplyAction = Namespaces.OpcUaWsdl + "/CreateSessionResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/CreateSessionFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        CreateSessionResponseMessage CreateSession(CreateSessionMessage request);

        /// <summary>
        /// The operation contract for the CreateSession service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/CreateSession", ReplyAction = Namespaces.OpcUaWsdl + "/CreateSessionResponse")]
        IAsyncResult BeginCreateSession(CreateSessionMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a CreateSession service request.
        /// </summary>
        CreateSessionResponseMessage EndCreateSession(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_ActivateSession)
        /// <summary>
        /// The operation contract for the ActivateSession service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/ActivateSession", ReplyAction = Namespaces.OpcUaWsdl + "/ActivateSessionResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/ActivateSessionFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        ActivateSessionResponseMessage ActivateSession(ActivateSessionMessage request);

        /// <summary>
        /// The operation contract for the ActivateSession service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/ActivateSession", ReplyAction = Namespaces.OpcUaWsdl + "/ActivateSessionResponse")]
        IAsyncResult BeginActivateSession(ActivateSessionMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a ActivateSession service request.
        /// </summary>
        ActivateSessionResponseMessage EndActivateSession(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_CloseSession)
        /// <summary>
        /// The operation contract for the CloseSession service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/CloseSession", ReplyAction = Namespaces.OpcUaWsdl + "/CloseSessionResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/CloseSessionFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        CloseSessionResponseMessage CloseSession(CloseSessionMessage request);

        /// <summary>
        /// The operation contract for the CloseSession service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/CloseSession", ReplyAction = Namespaces.OpcUaWsdl + "/CloseSessionResponse")]
        IAsyncResult BeginCloseSession(CloseSessionMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a CloseSession service request.
        /// </summary>
        CloseSessionResponseMessage EndCloseSession(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_Cancel)
        /// <summary>
        /// The operation contract for the Cancel service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/Cancel", ReplyAction = Namespaces.OpcUaWsdl + "/CancelResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/CancelFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        CancelResponseMessage Cancel(CancelMessage request);

        /// <summary>
        /// The operation contract for the Cancel service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/Cancel", ReplyAction = Namespaces.OpcUaWsdl + "/CancelResponse")]
        IAsyncResult BeginCancel(CancelMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a Cancel service request.
        /// </summary>
        CancelResponseMessage EndCancel(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_AddNodes)
        /// <summary>
        /// The operation contract for the AddNodes service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/AddNodes", ReplyAction = Namespaces.OpcUaWsdl + "/AddNodesResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/AddNodesFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        AddNodesResponseMessage AddNodes(AddNodesMessage request);

        /// <summary>
        /// The operation contract for the AddNodes service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/AddNodes", ReplyAction = Namespaces.OpcUaWsdl + "/AddNodesResponse")]
        IAsyncResult BeginAddNodes(AddNodesMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a AddNodes service request.
        /// </summary>
        AddNodesResponseMessage EndAddNodes(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_AddReferences)
        /// <summary>
        /// The operation contract for the AddReferences service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/AddReferences", ReplyAction = Namespaces.OpcUaWsdl + "/AddReferencesResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/AddReferencesFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        AddReferencesResponseMessage AddReferences(AddReferencesMessage request);

        /// <summary>
        /// The operation contract for the AddReferences service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/AddReferences", ReplyAction = Namespaces.OpcUaWsdl + "/AddReferencesResponse")]
        IAsyncResult BeginAddReferences(AddReferencesMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a AddReferences service request.
        /// </summary>
        AddReferencesResponseMessage EndAddReferences(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_DeleteNodes)
        /// <summary>
        /// The operation contract for the DeleteNodes service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/DeleteNodes", ReplyAction = Namespaces.OpcUaWsdl + "/DeleteNodesResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/DeleteNodesFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        DeleteNodesResponseMessage DeleteNodes(DeleteNodesMessage request);

        /// <summary>
        /// The operation contract for the DeleteNodes service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/DeleteNodes", ReplyAction = Namespaces.OpcUaWsdl + "/DeleteNodesResponse")]
        IAsyncResult BeginDeleteNodes(DeleteNodesMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a DeleteNodes service request.
        /// </summary>
        DeleteNodesResponseMessage EndDeleteNodes(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_DeleteReferences)
        /// <summary>
        /// The operation contract for the DeleteReferences service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/DeleteReferences", ReplyAction = Namespaces.OpcUaWsdl + "/DeleteReferencesResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/DeleteReferencesFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        DeleteReferencesResponseMessage DeleteReferences(DeleteReferencesMessage request);

        /// <summary>
        /// The operation contract for the DeleteReferences service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/DeleteReferences", ReplyAction = Namespaces.OpcUaWsdl + "/DeleteReferencesResponse")]
        IAsyncResult BeginDeleteReferences(DeleteReferencesMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a DeleteReferences service request.
        /// </summary>
        DeleteReferencesResponseMessage EndDeleteReferences(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_Browse)
        /// <summary>
        /// The operation contract for the Browse service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/Browse", ReplyAction = Namespaces.OpcUaWsdl + "/BrowseResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/BrowseFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        BrowseResponseMessage Browse(BrowseMessage request);

        /// <summary>
        /// The operation contract for the Browse service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/Browse", ReplyAction = Namespaces.OpcUaWsdl + "/BrowseResponse")]
        IAsyncResult BeginBrowse(BrowseMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a Browse service request.
        /// </summary>
        BrowseResponseMessage EndBrowse(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_BrowseNext)
        /// <summary>
        /// The operation contract for the BrowseNext service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/BrowseNext", ReplyAction = Namespaces.OpcUaWsdl + "/BrowseNextResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/BrowseNextFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        BrowseNextResponseMessage BrowseNext(BrowseNextMessage request);

        /// <summary>
        /// The operation contract for the BrowseNext service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/BrowseNext", ReplyAction = Namespaces.OpcUaWsdl + "/BrowseNextResponse")]
        IAsyncResult BeginBrowseNext(BrowseNextMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a BrowseNext service request.
        /// </summary>
        BrowseNextResponseMessage EndBrowseNext(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_TranslateBrowsePathsToNodeIds)
        /// <summary>
        /// The operation contract for the TranslateBrowsePathsToNodeIds service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/TranslateBrowsePathsToNodeIds", ReplyAction = Namespaces.OpcUaWsdl + "/TranslateBrowsePathsToNodeIdsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/TranslateBrowsePathsToNodeIdsFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        TranslateBrowsePathsToNodeIdsResponseMessage TranslateBrowsePathsToNodeIds(TranslateBrowsePathsToNodeIdsMessage request);

        /// <summary>
        /// The operation contract for the TranslateBrowsePathsToNodeIds service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/TranslateBrowsePathsToNodeIds", ReplyAction = Namespaces.OpcUaWsdl + "/TranslateBrowsePathsToNodeIdsResponse")]
        IAsyncResult BeginTranslateBrowsePathsToNodeIds(TranslateBrowsePathsToNodeIdsMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a TranslateBrowsePathsToNodeIds service request.
        /// </summary>
        TranslateBrowsePathsToNodeIdsResponseMessage EndTranslateBrowsePathsToNodeIds(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_RegisterNodes)
        /// <summary>
        /// The operation contract for the RegisterNodes service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/RegisterNodes", ReplyAction = Namespaces.OpcUaWsdl + "/RegisterNodesResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/RegisterNodesFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        RegisterNodesResponseMessage RegisterNodes(RegisterNodesMessage request);

        /// <summary>
        /// The operation contract for the RegisterNodes service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/RegisterNodes", ReplyAction = Namespaces.OpcUaWsdl + "/RegisterNodesResponse")]
        IAsyncResult BeginRegisterNodes(RegisterNodesMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a RegisterNodes service request.
        /// </summary>
        RegisterNodesResponseMessage EndRegisterNodes(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_UnregisterNodes)
        /// <summary>
        /// The operation contract for the UnregisterNodes service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/UnregisterNodes", ReplyAction = Namespaces.OpcUaWsdl + "/UnregisterNodesResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/UnregisterNodesFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        UnregisterNodesResponseMessage UnregisterNodes(UnregisterNodesMessage request);

        /// <summary>
        /// The operation contract for the UnregisterNodes service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/UnregisterNodes", ReplyAction = Namespaces.OpcUaWsdl + "/UnregisterNodesResponse")]
        IAsyncResult BeginUnregisterNodes(UnregisterNodesMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a UnregisterNodes service request.
        /// </summary>
        UnregisterNodesResponseMessage EndUnregisterNodes(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_QueryFirst)
        /// <summary>
        /// The operation contract for the QueryFirst service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/QueryFirst", ReplyAction = Namespaces.OpcUaWsdl + "/QueryFirstResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/QueryFirstFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        QueryFirstResponseMessage QueryFirst(QueryFirstMessage request);

        /// <summary>
        /// The operation contract for the QueryFirst service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/QueryFirst", ReplyAction = Namespaces.OpcUaWsdl + "/QueryFirstResponse")]
        IAsyncResult BeginQueryFirst(QueryFirstMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a QueryFirst service request.
        /// </summary>
        QueryFirstResponseMessage EndQueryFirst(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_QueryNext)
        /// <summary>
        /// The operation contract for the QueryNext service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/QueryNext", ReplyAction = Namespaces.OpcUaWsdl + "/QueryNextResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/QueryNextFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        QueryNextResponseMessage QueryNext(QueryNextMessage request);

        /// <summary>
        /// The operation contract for the QueryNext service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/QueryNext", ReplyAction = Namespaces.OpcUaWsdl + "/QueryNextResponse")]
        IAsyncResult BeginQueryNext(QueryNextMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a QueryNext service request.
        /// </summary>
        QueryNextResponseMessage EndQueryNext(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_Read)
        /// <summary>
        /// The operation contract for the Read service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/Read", ReplyAction = Namespaces.OpcUaWsdl + "/ReadResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/ReadFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        ReadResponseMessage Read(ReadMessage request);

        /// <summary>
        /// The operation contract for the Read service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/Read", ReplyAction = Namespaces.OpcUaWsdl + "/ReadResponse")]
        IAsyncResult BeginRead(ReadMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a Read service request.
        /// </summary>
        ReadResponseMessage EndRead(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_HistoryRead)
        /// <summary>
        /// The operation contract for the HistoryRead service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/HistoryRead", ReplyAction = Namespaces.OpcUaWsdl + "/HistoryReadResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/HistoryReadFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        HistoryReadResponseMessage HistoryRead(HistoryReadMessage request);

        /// <summary>
        /// The operation contract for the HistoryRead service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/HistoryRead", ReplyAction = Namespaces.OpcUaWsdl + "/HistoryReadResponse")]
        IAsyncResult BeginHistoryRead(HistoryReadMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a HistoryRead service request.
        /// </summary>
        HistoryReadResponseMessage EndHistoryRead(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_Write)
        /// <summary>
        /// The operation contract for the Write service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/Write", ReplyAction = Namespaces.OpcUaWsdl + "/WriteResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/WriteFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        WriteResponseMessage Write(WriteMessage request);

        /// <summary>
        /// The operation contract for the Write service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/Write", ReplyAction = Namespaces.OpcUaWsdl + "/WriteResponse")]
        IAsyncResult BeginWrite(WriteMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a Write service request.
        /// </summary>
        WriteResponseMessage EndWrite(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_HistoryUpdate)
        /// <summary>
        /// The operation contract for the HistoryUpdate service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/HistoryUpdate", ReplyAction = Namespaces.OpcUaWsdl + "/HistoryUpdateResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/HistoryUpdateFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        HistoryUpdateResponseMessage HistoryUpdate(HistoryUpdateMessage request);

        /// <summary>
        /// The operation contract for the HistoryUpdate service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/HistoryUpdate", ReplyAction = Namespaces.OpcUaWsdl + "/HistoryUpdateResponse")]
        IAsyncResult BeginHistoryUpdate(HistoryUpdateMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a HistoryUpdate service request.
        /// </summary>
        HistoryUpdateResponseMessage EndHistoryUpdate(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_Call)
        /// <summary>
        /// The operation contract for the Call service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/Call", ReplyAction = Namespaces.OpcUaWsdl + "/CallResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/CallFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        CallResponseMessage Call(CallMessage request);

        /// <summary>
        /// The operation contract for the Call service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/Call", ReplyAction = Namespaces.OpcUaWsdl + "/CallResponse")]
        IAsyncResult BeginCall(CallMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a Call service request.
        /// </summary>
        CallResponseMessage EndCall(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_CreateMonitoredItems)
        /// <summary>
        /// The operation contract for the CreateMonitoredItems service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/CreateMonitoredItems", ReplyAction = Namespaces.OpcUaWsdl + "/CreateMonitoredItemsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/CreateMonitoredItemsFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        CreateMonitoredItemsResponseMessage CreateMonitoredItems(CreateMonitoredItemsMessage request);

        /// <summary>
        /// The operation contract for the CreateMonitoredItems service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/CreateMonitoredItems", ReplyAction = Namespaces.OpcUaWsdl + "/CreateMonitoredItemsResponse")]
        IAsyncResult BeginCreateMonitoredItems(CreateMonitoredItemsMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a CreateMonitoredItems service request.
        /// </summary>
        CreateMonitoredItemsResponseMessage EndCreateMonitoredItems(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_ModifyMonitoredItems)
        /// <summary>
        /// The operation contract for the ModifyMonitoredItems service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/ModifyMonitoredItems", ReplyAction = Namespaces.OpcUaWsdl + "/ModifyMonitoredItemsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/ModifyMonitoredItemsFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        ModifyMonitoredItemsResponseMessage ModifyMonitoredItems(ModifyMonitoredItemsMessage request);

        /// <summary>
        /// The operation contract for the ModifyMonitoredItems service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/ModifyMonitoredItems", ReplyAction = Namespaces.OpcUaWsdl + "/ModifyMonitoredItemsResponse")]
        IAsyncResult BeginModifyMonitoredItems(ModifyMonitoredItemsMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a ModifyMonitoredItems service request.
        /// </summary>
        ModifyMonitoredItemsResponseMessage EndModifyMonitoredItems(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_SetMonitoringMode)
        /// <summary>
        /// The operation contract for the SetMonitoringMode service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/SetMonitoringMode", ReplyAction = Namespaces.OpcUaWsdl + "/SetMonitoringModeResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/SetMonitoringModeFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        SetMonitoringModeResponseMessage SetMonitoringMode(SetMonitoringModeMessage request);

        /// <summary>
        /// The operation contract for the SetMonitoringMode service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/SetMonitoringMode", ReplyAction = Namespaces.OpcUaWsdl + "/SetMonitoringModeResponse")]
        IAsyncResult BeginSetMonitoringMode(SetMonitoringModeMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a SetMonitoringMode service request.
        /// </summary>
        SetMonitoringModeResponseMessage EndSetMonitoringMode(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_SetTriggering)
        /// <summary>
        /// The operation contract for the SetTriggering service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/SetTriggering", ReplyAction = Namespaces.OpcUaWsdl + "/SetTriggeringResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/SetTriggeringFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        SetTriggeringResponseMessage SetTriggering(SetTriggeringMessage request);

        /// <summary>
        /// The operation contract for the SetTriggering service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/SetTriggering", ReplyAction = Namespaces.OpcUaWsdl + "/SetTriggeringResponse")]
        IAsyncResult BeginSetTriggering(SetTriggeringMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a SetTriggering service request.
        /// </summary>
        SetTriggeringResponseMessage EndSetTriggering(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_DeleteMonitoredItems)
        /// <summary>
        /// The operation contract for the DeleteMonitoredItems service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/DeleteMonitoredItems", ReplyAction = Namespaces.OpcUaWsdl + "/DeleteMonitoredItemsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/DeleteMonitoredItemsFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        DeleteMonitoredItemsResponseMessage DeleteMonitoredItems(DeleteMonitoredItemsMessage request);

        /// <summary>
        /// The operation contract for the DeleteMonitoredItems service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/DeleteMonitoredItems", ReplyAction = Namespaces.OpcUaWsdl + "/DeleteMonitoredItemsResponse")]
        IAsyncResult BeginDeleteMonitoredItems(DeleteMonitoredItemsMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a DeleteMonitoredItems service request.
        /// </summary>
        DeleteMonitoredItemsResponseMessage EndDeleteMonitoredItems(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_CreateSubscription)
        /// <summary>
        /// The operation contract for the CreateSubscription service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/CreateSubscription", ReplyAction = Namespaces.OpcUaWsdl + "/CreateSubscriptionResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/CreateSubscriptionFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        CreateSubscriptionResponseMessage CreateSubscription(CreateSubscriptionMessage request);

        /// <summary>
        /// The operation contract for the CreateSubscription service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/CreateSubscription", ReplyAction = Namespaces.OpcUaWsdl + "/CreateSubscriptionResponse")]
        IAsyncResult BeginCreateSubscription(CreateSubscriptionMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a CreateSubscription service request.
        /// </summary>
        CreateSubscriptionResponseMessage EndCreateSubscription(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_ModifySubscription)
        /// <summary>
        /// The operation contract for the ModifySubscription service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/ModifySubscription", ReplyAction = Namespaces.OpcUaWsdl + "/ModifySubscriptionResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/ModifySubscriptionFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        ModifySubscriptionResponseMessage ModifySubscription(ModifySubscriptionMessage request);

        /// <summary>
        /// The operation contract for the ModifySubscription service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/ModifySubscription", ReplyAction = Namespaces.OpcUaWsdl + "/ModifySubscriptionResponse")]
        IAsyncResult BeginModifySubscription(ModifySubscriptionMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a ModifySubscription service request.
        /// </summary>
        ModifySubscriptionResponseMessage EndModifySubscription(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_SetPublishingMode)
        /// <summary>
        /// The operation contract for the SetPublishingMode service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/SetPublishingMode", ReplyAction = Namespaces.OpcUaWsdl + "/SetPublishingModeResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/SetPublishingModeFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        SetPublishingModeResponseMessage SetPublishingMode(SetPublishingModeMessage request);

        /// <summary>
        /// The operation contract for the SetPublishingMode service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/SetPublishingMode", ReplyAction = Namespaces.OpcUaWsdl + "/SetPublishingModeResponse")]
        IAsyncResult BeginSetPublishingMode(SetPublishingModeMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a SetPublishingMode service request.
        /// </summary>
        SetPublishingModeResponseMessage EndSetPublishingMode(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_Publish)
        /// <summary>
        /// The operation contract for the Publish service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/Publish", ReplyAction = Namespaces.OpcUaWsdl + "/PublishResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/PublishFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        PublishResponseMessage Publish(PublishMessage request);

        /// <summary>
        /// The operation contract for the Publish service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/Publish", ReplyAction = Namespaces.OpcUaWsdl + "/PublishResponse")]
        IAsyncResult BeginPublish(PublishMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a Publish service request.
        /// </summary>
        PublishResponseMessage EndPublish(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_Republish)
        /// <summary>
        /// The operation contract for the Republish service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/Republish", ReplyAction = Namespaces.OpcUaWsdl + "/RepublishResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/RepublishFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        RepublishResponseMessage Republish(RepublishMessage request);

        /// <summary>
        /// The operation contract for the Republish service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/Republish", ReplyAction = Namespaces.OpcUaWsdl + "/RepublishResponse")]
        IAsyncResult BeginRepublish(RepublishMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a Republish service request.
        /// </summary>
        RepublishResponseMessage EndRepublish(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_TransferSubscriptions)
        /// <summary>
        /// The operation contract for the TransferSubscriptions service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/TransferSubscriptions", ReplyAction = Namespaces.OpcUaWsdl + "/TransferSubscriptionsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/TransferSubscriptionsFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        TransferSubscriptionsResponseMessage TransferSubscriptions(TransferSubscriptionsMessage request);

        /// <summary>
        /// The operation contract for the TransferSubscriptions service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/TransferSubscriptions", ReplyAction = Namespaces.OpcUaWsdl + "/TransferSubscriptionsResponse")]
        IAsyncResult BeginTransferSubscriptions(TransferSubscriptionsMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a TransferSubscriptions service request.
        /// </summary>
        TransferSubscriptionsResponseMessage EndTransferSubscriptions(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_DeleteSubscriptions)
        /// <summary>
        /// The operation contract for the DeleteSubscriptions service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/DeleteSubscriptions", ReplyAction = Namespaces.OpcUaWsdl + "/DeleteSubscriptionsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/DeleteSubscriptionsFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        DeleteSubscriptionsResponseMessage DeleteSubscriptions(DeleteSubscriptionsMessage request);

        /// <summary>
        /// The operation contract for the DeleteSubscriptions service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/DeleteSubscriptions", ReplyAction = Namespaces.OpcUaWsdl + "/DeleteSubscriptionsResponse")]
        IAsyncResult BeginDeleteSubscriptions(DeleteSubscriptionsMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a DeleteSubscriptions service request.
        /// </summary>
        DeleteSubscriptionsResponseMessage EndDeleteSubscriptions(IAsyncResult result);
        #endif
    }
    #endregion

    #region IDiscoveryEndpoint Interface
    #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
    /// <summary>
    /// The service contract which must be implemented by all UA servers.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.CodeGenerator", "1.0.0.0")]
    [ServiceContract(Namespace = Namespaces.OpcUaWsdl)]
    public interface IDiscoveryEndpoint : IEndpointBase
    {
        #if (!OPCUA_EXCLUDE_FindServers)
        /// <summary>
        /// The operation contract for the FindServers service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/FindServers", ReplyAction = Namespaces.OpcUaWsdl + "/FindServersResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/FindServersFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        FindServersResponseMessage FindServers(FindServersMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_FindServersOnNetwork)
        /// <summary>
        /// The operation contract for the FindServersOnNetwork service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/FindServersOnNetwork", ReplyAction = Namespaces.OpcUaWsdl + "/FindServersOnNetworkResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/FindServersOnNetworkFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        FindServersOnNetworkResponseMessage FindServersOnNetwork(FindServersOnNetworkMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_GetEndpoints)
        /// <summary>
        /// The operation contract for the GetEndpoints service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/GetEndpoints", ReplyAction = Namespaces.OpcUaWsdl + "/GetEndpointsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/GetEndpointsFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        GetEndpointsResponseMessage GetEndpoints(GetEndpointsMessage request);
        #endif
    }
    #else
    /// <summary>
    /// The asynchronous service contract which must be implemented by UA servers.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.CodeGenerator", "1.0.0.0")]
    [ServiceContract(Namespace = Namespaces.OpcUaWsdl)]
    public interface IDiscoveryEndpoint : IEndpointBase
    {
        #if (!OPCUA_EXCLUDE_FindServers)
        /// <summary>
        /// The operation contract for the FindServers service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/FindServers", ReplyAction = Namespaces.OpcUaWsdl + "/FindServersResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/FindServersFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginFindServers(FindServersMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a FindServers service request.
        /// </summary>
        FindServersResponseMessage EndFindServers(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_FindServersOnNetwork)
        /// <summary>
        /// The operation contract for the FindServersOnNetwork service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/FindServersOnNetwork", ReplyAction = Namespaces.OpcUaWsdl + "/FindServersOnNetworkResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/FindServersOnNetworkFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginFindServersOnNetwork(FindServersOnNetworkMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a FindServersOnNetwork service request.
        /// </summary>
        FindServersOnNetworkResponseMessage EndFindServersOnNetwork(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_GetEndpoints)
        /// <summary>
        /// The operation contract for the GetEndpoints service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/GetEndpoints", ReplyAction = Namespaces.OpcUaWsdl + "/GetEndpointsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/GetEndpointsFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginGetEndpoints(GetEndpointsMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a GetEndpoints service request.
        /// </summary>
        GetEndpointsResponseMessage EndGetEndpoints(IAsyncResult result);
        #endif
    }
    #endif
    #endregion

    #region IDiscoveryChannel Interface
    /// <summary>
    /// An interface used by by clients to access a UA server.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.CodeGenerator", "1.0.0.0")]
    [ServiceContract(Namespace = Namespaces.OpcUaWsdl)]
    public interface IDiscoveryChannel : IChannelBase
    {
        #if (!OPCUA_EXCLUDE_FindServers)
        /// <summary>
        /// The operation contract for the FindServers service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/FindServers", ReplyAction = Namespaces.OpcUaWsdl + "/FindServersResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/FindServersFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        FindServersResponseMessage FindServers(FindServersMessage request);

        /// <summary>
        /// The operation contract for the FindServers service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/FindServers", ReplyAction = Namespaces.OpcUaWsdl + "/FindServersResponse")]
        IAsyncResult BeginFindServers(FindServersMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a FindServers service request.
        /// </summary>
        FindServersResponseMessage EndFindServers(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_FindServersOnNetwork)
        /// <summary>
        /// The operation contract for the FindServersOnNetwork service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/FindServersOnNetwork", ReplyAction = Namespaces.OpcUaWsdl + "/FindServersOnNetworkResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/FindServersOnNetworkFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        FindServersOnNetworkResponseMessage FindServersOnNetwork(FindServersOnNetworkMessage request);

        /// <summary>
        /// The operation contract for the FindServersOnNetwork service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/FindServersOnNetwork", ReplyAction = Namespaces.OpcUaWsdl + "/FindServersOnNetworkResponse")]
        IAsyncResult BeginFindServersOnNetwork(FindServersOnNetworkMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a FindServersOnNetwork service request.
        /// </summary>
        FindServersOnNetworkResponseMessage EndFindServersOnNetwork(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_GetEndpoints)
        /// <summary>
        /// The operation contract for the GetEndpoints service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/GetEndpoints", ReplyAction = Namespaces.OpcUaWsdl + "/GetEndpointsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/GetEndpointsFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        GetEndpointsResponseMessage GetEndpoints(GetEndpointsMessage request);

        /// <summary>
        /// The operation contract for the GetEndpoints service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/GetEndpoints", ReplyAction = Namespaces.OpcUaWsdl + "/GetEndpointsResponse")]
        IAsyncResult BeginGetEndpoints(GetEndpointsMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a GetEndpoints service request.
        /// </summary>
        GetEndpointsResponseMessage EndGetEndpoints(IAsyncResult result);
        #endif
    }
    #endregion

    #region IRegistrationEndpoint Interface
    #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
    /// <summary>
    /// The service contract which must be implemented by all UA servers.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.CodeGenerator", "1.0.0.0")]
    [ServiceContract(Namespace = Namespaces.OpcUaWsdl)]
    public interface IRegistrationEndpoint : IEndpointBase
    {
        #if (!OPCUA_EXCLUDE_RegisterServer)
        /// <summary>
        /// The operation contract for the RegisterServer service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/RegisterServer", ReplyAction = Namespaces.OpcUaWsdl + "/RegisterServerResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/RegisterServerFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        RegisterServerResponseMessage RegisterServer(RegisterServerMessage request);
        #endif

        #if (!OPCUA_EXCLUDE_RegisterServer2)
        /// <summary>
        /// The operation contract for the RegisterServer2 service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/RegisterServer2", ReplyAction = Namespaces.OpcUaWsdl + "/RegisterServer2Response")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/RegisterServer2Fault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        RegisterServer2ResponseMessage RegisterServer2(RegisterServer2Message request);
        #endif
    }
    #else
    /// <summary>
    /// The asynchronous service contract which must be implemented by UA servers.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.CodeGenerator", "1.0.0.0")]
    [ServiceContract(Namespace = Namespaces.OpcUaWsdl)]
    public interface IRegistrationEndpoint : IEndpointBase
    {
        #if (!OPCUA_EXCLUDE_RegisterServer)
        /// <summary>
        /// The operation contract for the RegisterServer service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/RegisterServer", ReplyAction = Namespaces.OpcUaWsdl + "/RegisterServerResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/RegisterServerFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginRegisterServer(RegisterServerMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a RegisterServer service request.
        /// </summary>
        RegisterServerResponseMessage EndRegisterServer(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_RegisterServer2)
        /// <summary>
        /// The operation contract for the RegisterServer2 service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/RegisterServer2", ReplyAction = Namespaces.OpcUaWsdl + "/RegisterServer2Response")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/RegisterServer2Fault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        IAsyncResult BeginRegisterServer2(RegisterServer2Message request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a RegisterServer2 service request.
        /// </summary>
        RegisterServer2ResponseMessage EndRegisterServer2(IAsyncResult result);
        #endif
    }
    #endif
    #endregion

    #region IRegistrationChannel Interface
    /// <summary>
    /// An interface used by by clients to access a UA server.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.CodeGenerator", "1.0.0.0")]
    [ServiceContract(Namespace = Namespaces.OpcUaWsdl)]
    public interface IRegistrationChannel : IChannelBase
    {
        #if (!OPCUA_EXCLUDE_RegisterServer)
        /// <summary>
        /// The operation contract for the RegisterServer service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/RegisterServer", ReplyAction = Namespaces.OpcUaWsdl + "/RegisterServerResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/RegisterServerFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        RegisterServerResponseMessage RegisterServer(RegisterServerMessage request);

        /// <summary>
        /// The operation contract for the RegisterServer service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/RegisterServer", ReplyAction = Namespaces.OpcUaWsdl + "/RegisterServerResponse")]
        IAsyncResult BeginRegisterServer(RegisterServerMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a RegisterServer service request.
        /// </summary>
        RegisterServerResponseMessage EndRegisterServer(IAsyncResult result);
        #endif

        #if (!OPCUA_EXCLUDE_RegisterServer2)
        /// <summary>
        /// The operation contract for the RegisterServer2 service.
        /// </summary>
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/RegisterServer2", ReplyAction = Namespaces.OpcUaWsdl + "/RegisterServer2Response")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/RegisterServer2Fault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        RegisterServer2ResponseMessage RegisterServer2(RegisterServer2Message request);

        /// <summary>
        /// The operation contract for the RegisterServer2 service.
        /// </summary>
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/RegisterServer2", ReplyAction = Namespaces.OpcUaWsdl + "/RegisterServer2Response")]
        IAsyncResult BeginRegisterServer2(RegisterServer2Message request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a RegisterServer2 service request.
        /// </summary>
        RegisterServer2ResponseMessage EndRegisterServer2(IAsyncResult result);
        #endif
    }
    #endregion
}
