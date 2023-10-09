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

using System;

#if (!NET_STANDARD)
using System.Collections.Generic;
using System.Xml;
using System.ServiceModel;
using System.Runtime.Serialization;
#endif

#if (NET_STANDARD_ASYNC)
using System.Threading.Tasks;
#endif

namespace Opc.Ua
{
    #region ISessionEndpoint Interface
    #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
    /// <summary>
    /// The service contract which must be implemented by all UA servers.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
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
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    #if (!NET_STANDARD)
    [ServiceContract(Namespace = Namespaces.OpcUaWsdl)]
    #endif
    public interface ISessionEndpoint : IEndpointBase
    {
        #if (!OPCUA_EXCLUDE_CreateSession)
        /// <summary>
        /// The operation contract for the CreateSession service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/CreateSession", ReplyAction = Namespaces.OpcUaWsdl + "/CreateSessionResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/CreateSessionFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/ActivateSession", ReplyAction = Namespaces.OpcUaWsdl + "/ActivateSessionResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/ActivateSessionFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/CloseSession", ReplyAction = Namespaces.OpcUaWsdl + "/CloseSessionResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/CloseSessionFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/Cancel", ReplyAction = Namespaces.OpcUaWsdl + "/CancelResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/CancelFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/AddNodes", ReplyAction = Namespaces.OpcUaWsdl + "/AddNodesResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/AddNodesFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/AddReferences", ReplyAction = Namespaces.OpcUaWsdl + "/AddReferencesResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/AddReferencesFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/DeleteNodes", ReplyAction = Namespaces.OpcUaWsdl + "/DeleteNodesResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/DeleteNodesFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/DeleteReferences", ReplyAction = Namespaces.OpcUaWsdl + "/DeleteReferencesResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/DeleteReferencesFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/Browse", ReplyAction = Namespaces.OpcUaWsdl + "/BrowseResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/BrowseFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/BrowseNext", ReplyAction = Namespaces.OpcUaWsdl + "/BrowseNextResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/BrowseNextFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/TranslateBrowsePathsToNodeIds", ReplyAction = Namespaces.OpcUaWsdl + "/TranslateBrowsePathsToNodeIdsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/TranslateBrowsePathsToNodeIdsFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/RegisterNodes", ReplyAction = Namespaces.OpcUaWsdl + "/RegisterNodesResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/RegisterNodesFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/UnregisterNodes", ReplyAction = Namespaces.OpcUaWsdl + "/UnregisterNodesResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/UnregisterNodesFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/QueryFirst", ReplyAction = Namespaces.OpcUaWsdl + "/QueryFirstResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/QueryFirstFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/QueryNext", ReplyAction = Namespaces.OpcUaWsdl + "/QueryNextResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/QueryNextFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/Read", ReplyAction = Namespaces.OpcUaWsdl + "/ReadResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/ReadFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/HistoryRead", ReplyAction = Namespaces.OpcUaWsdl + "/HistoryReadResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/HistoryReadFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/Write", ReplyAction = Namespaces.OpcUaWsdl + "/WriteResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/WriteFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/HistoryUpdate", ReplyAction = Namespaces.OpcUaWsdl + "/HistoryUpdateResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/HistoryUpdateFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/Call", ReplyAction = Namespaces.OpcUaWsdl + "/CallResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/CallFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/CreateMonitoredItems", ReplyAction = Namespaces.OpcUaWsdl + "/CreateMonitoredItemsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/CreateMonitoredItemsFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/ModifyMonitoredItems", ReplyAction = Namespaces.OpcUaWsdl + "/ModifyMonitoredItemsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/ModifyMonitoredItemsFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/SetMonitoringMode", ReplyAction = Namespaces.OpcUaWsdl + "/SetMonitoringModeResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/SetMonitoringModeFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/SetTriggering", ReplyAction = Namespaces.OpcUaWsdl + "/SetTriggeringResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/SetTriggeringFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/DeleteMonitoredItems", ReplyAction = Namespaces.OpcUaWsdl + "/DeleteMonitoredItemsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/DeleteMonitoredItemsFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/CreateSubscription", ReplyAction = Namespaces.OpcUaWsdl + "/CreateSubscriptionResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/CreateSubscriptionFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/ModifySubscription", ReplyAction = Namespaces.OpcUaWsdl + "/ModifySubscriptionResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/ModifySubscriptionFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/SetPublishingMode", ReplyAction = Namespaces.OpcUaWsdl + "/SetPublishingModeResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/SetPublishingModeFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/Publish", ReplyAction = Namespaces.OpcUaWsdl + "/PublishResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/PublishFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/Republish", ReplyAction = Namespaces.OpcUaWsdl + "/RepublishResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/RepublishFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/TransferSubscriptions", ReplyAction = Namespaces.OpcUaWsdl + "/TransferSubscriptionsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/TransferSubscriptionsFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/DeleteSubscriptions", ReplyAction = Namespaces.OpcUaWsdl + "/DeleteSubscriptionsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/DeleteSubscriptionsFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    #if (!NET_STANDARD)
    [ServiceContract(Namespace = Namespaces.OpcUaWsdl)]
    #endif
    public interface ISessionChannel : IChannelBase
    {
        #if (!OPCUA_EXCLUDE_CreateSession)
        /// <summary>
        /// The operation contract for the CreateSession service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/CreateSession", ReplyAction = Namespaces.OpcUaWsdl + "/CreateSessionResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/CreateSessionFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        CreateSessionResponseMessage CreateSession(CreateSessionMessage request);

        /// <summary>
        /// The operation contract for the CreateSession service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/CreateSession", ReplyAction = Namespaces.OpcUaWsdl + "/CreateSessionResponse")]
        #endif
        IAsyncResult BeginCreateSession(CreateSessionMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a CreateSession service request.
        /// </summary>
        CreateSessionResponseMessage EndCreateSession(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the CreateSession service.
        /// </summary>
        Task<CreateSessionResponseMessage> CreateSessionAsync(CreateSessionMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_ActivateSession)
        /// <summary>
        /// The operation contract for the ActivateSession service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/ActivateSession", ReplyAction = Namespaces.OpcUaWsdl + "/ActivateSessionResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/ActivateSessionFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        ActivateSessionResponseMessage ActivateSession(ActivateSessionMessage request);

        /// <summary>
        /// The operation contract for the ActivateSession service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/ActivateSession", ReplyAction = Namespaces.OpcUaWsdl + "/ActivateSessionResponse")]
        #endif
        IAsyncResult BeginActivateSession(ActivateSessionMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a ActivateSession service request.
        /// </summary>
        ActivateSessionResponseMessage EndActivateSession(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the ActivateSession service.
        /// </summary>
        Task<ActivateSessionResponseMessage> ActivateSessionAsync(ActivateSessionMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_CloseSession)
        /// <summary>
        /// The operation contract for the CloseSession service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/CloseSession", ReplyAction = Namespaces.OpcUaWsdl + "/CloseSessionResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/CloseSessionFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        CloseSessionResponseMessage CloseSession(CloseSessionMessage request);

        /// <summary>
        /// The operation contract for the CloseSession service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/CloseSession", ReplyAction = Namespaces.OpcUaWsdl + "/CloseSessionResponse")]
        #endif
        IAsyncResult BeginCloseSession(CloseSessionMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a CloseSession service request.
        /// </summary>
        CloseSessionResponseMessage EndCloseSession(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the CloseSession service.
        /// </summary>
        Task<CloseSessionResponseMessage> CloseSessionAsync(CloseSessionMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_Cancel)
        /// <summary>
        /// The operation contract for the Cancel service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/Cancel", ReplyAction = Namespaces.OpcUaWsdl + "/CancelResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/CancelFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        CancelResponseMessage Cancel(CancelMessage request);

        /// <summary>
        /// The operation contract for the Cancel service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/Cancel", ReplyAction = Namespaces.OpcUaWsdl + "/CancelResponse")]
        #endif
        IAsyncResult BeginCancel(CancelMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a Cancel service request.
        /// </summary>
        CancelResponseMessage EndCancel(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the Cancel service.
        /// </summary>
        Task<CancelResponseMessage> CancelAsync(CancelMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_AddNodes)
        /// <summary>
        /// The operation contract for the AddNodes service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/AddNodes", ReplyAction = Namespaces.OpcUaWsdl + "/AddNodesResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/AddNodesFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        AddNodesResponseMessage AddNodes(AddNodesMessage request);

        /// <summary>
        /// The operation contract for the AddNodes service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/AddNodes", ReplyAction = Namespaces.OpcUaWsdl + "/AddNodesResponse")]
        #endif
        IAsyncResult BeginAddNodes(AddNodesMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a AddNodes service request.
        /// </summary>
        AddNodesResponseMessage EndAddNodes(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the AddNodes service.
        /// </summary>
        Task<AddNodesResponseMessage> AddNodesAsync(AddNodesMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_AddReferences)
        /// <summary>
        /// The operation contract for the AddReferences service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/AddReferences", ReplyAction = Namespaces.OpcUaWsdl + "/AddReferencesResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/AddReferencesFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        AddReferencesResponseMessage AddReferences(AddReferencesMessage request);

        /// <summary>
        /// The operation contract for the AddReferences service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/AddReferences", ReplyAction = Namespaces.OpcUaWsdl + "/AddReferencesResponse")]
        #endif
        IAsyncResult BeginAddReferences(AddReferencesMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a AddReferences service request.
        /// </summary>
        AddReferencesResponseMessage EndAddReferences(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the AddReferences service.
        /// </summary>
        Task<AddReferencesResponseMessage> AddReferencesAsync(AddReferencesMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_DeleteNodes)
        /// <summary>
        /// The operation contract for the DeleteNodes service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/DeleteNodes", ReplyAction = Namespaces.OpcUaWsdl + "/DeleteNodesResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/DeleteNodesFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        DeleteNodesResponseMessage DeleteNodes(DeleteNodesMessage request);

        /// <summary>
        /// The operation contract for the DeleteNodes service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/DeleteNodes", ReplyAction = Namespaces.OpcUaWsdl + "/DeleteNodesResponse")]
        #endif
        IAsyncResult BeginDeleteNodes(DeleteNodesMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a DeleteNodes service request.
        /// </summary>
        DeleteNodesResponseMessage EndDeleteNodes(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the DeleteNodes service.
        /// </summary>
        Task<DeleteNodesResponseMessage> DeleteNodesAsync(DeleteNodesMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_DeleteReferences)
        /// <summary>
        /// The operation contract for the DeleteReferences service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/DeleteReferences", ReplyAction = Namespaces.OpcUaWsdl + "/DeleteReferencesResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/DeleteReferencesFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        DeleteReferencesResponseMessage DeleteReferences(DeleteReferencesMessage request);

        /// <summary>
        /// The operation contract for the DeleteReferences service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/DeleteReferences", ReplyAction = Namespaces.OpcUaWsdl + "/DeleteReferencesResponse")]
        #endif
        IAsyncResult BeginDeleteReferences(DeleteReferencesMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a DeleteReferences service request.
        /// </summary>
        DeleteReferencesResponseMessage EndDeleteReferences(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the DeleteReferences service.
        /// </summary>
        Task<DeleteReferencesResponseMessage> DeleteReferencesAsync(DeleteReferencesMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_Browse)
        /// <summary>
        /// The operation contract for the Browse service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/Browse", ReplyAction = Namespaces.OpcUaWsdl + "/BrowseResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/BrowseFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        BrowseResponseMessage Browse(BrowseMessage request);

        /// <summary>
        /// The operation contract for the Browse service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/Browse", ReplyAction = Namespaces.OpcUaWsdl + "/BrowseResponse")]
        #endif
        IAsyncResult BeginBrowse(BrowseMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a Browse service request.
        /// </summary>
        BrowseResponseMessage EndBrowse(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the Browse service.
        /// </summary>
        Task<BrowseResponseMessage> BrowseAsync(BrowseMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_BrowseNext)
        /// <summary>
        /// The operation contract for the BrowseNext service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/BrowseNext", ReplyAction = Namespaces.OpcUaWsdl + "/BrowseNextResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/BrowseNextFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        BrowseNextResponseMessage BrowseNext(BrowseNextMessage request);

        /// <summary>
        /// The operation contract for the BrowseNext service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/BrowseNext", ReplyAction = Namespaces.OpcUaWsdl + "/BrowseNextResponse")]
        #endif
        IAsyncResult BeginBrowseNext(BrowseNextMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a BrowseNext service request.
        /// </summary>
        BrowseNextResponseMessage EndBrowseNext(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the BrowseNext service.
        /// </summary>
        Task<BrowseNextResponseMessage> BrowseNextAsync(BrowseNextMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_TranslateBrowsePathsToNodeIds)
        /// <summary>
        /// The operation contract for the TranslateBrowsePathsToNodeIds service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/TranslateBrowsePathsToNodeIds", ReplyAction = Namespaces.OpcUaWsdl + "/TranslateBrowsePathsToNodeIdsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/TranslateBrowsePathsToNodeIdsFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        TranslateBrowsePathsToNodeIdsResponseMessage TranslateBrowsePathsToNodeIds(TranslateBrowsePathsToNodeIdsMessage request);

        /// <summary>
        /// The operation contract for the TranslateBrowsePathsToNodeIds service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/TranslateBrowsePathsToNodeIds", ReplyAction = Namespaces.OpcUaWsdl + "/TranslateBrowsePathsToNodeIdsResponse")]
        #endif
        IAsyncResult BeginTranslateBrowsePathsToNodeIds(TranslateBrowsePathsToNodeIdsMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a TranslateBrowsePathsToNodeIds service request.
        /// </summary>
        TranslateBrowsePathsToNodeIdsResponseMessage EndTranslateBrowsePathsToNodeIds(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the TranslateBrowsePathsToNodeIds service.
        /// </summary>
        Task<TranslateBrowsePathsToNodeIdsResponseMessage> TranslateBrowsePathsToNodeIdsAsync(TranslateBrowsePathsToNodeIdsMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_RegisterNodes)
        /// <summary>
        /// The operation contract for the RegisterNodes service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/RegisterNodes", ReplyAction = Namespaces.OpcUaWsdl + "/RegisterNodesResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/RegisterNodesFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        RegisterNodesResponseMessage RegisterNodes(RegisterNodesMessage request);

        /// <summary>
        /// The operation contract for the RegisterNodes service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/RegisterNodes", ReplyAction = Namespaces.OpcUaWsdl + "/RegisterNodesResponse")]
        #endif
        IAsyncResult BeginRegisterNodes(RegisterNodesMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a RegisterNodes service request.
        /// </summary>
        RegisterNodesResponseMessage EndRegisterNodes(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the RegisterNodes service.
        /// </summary>
        Task<RegisterNodesResponseMessage> RegisterNodesAsync(RegisterNodesMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_UnregisterNodes)
        /// <summary>
        /// The operation contract for the UnregisterNodes service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/UnregisterNodes", ReplyAction = Namespaces.OpcUaWsdl + "/UnregisterNodesResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/UnregisterNodesFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        UnregisterNodesResponseMessage UnregisterNodes(UnregisterNodesMessage request);

        /// <summary>
        /// The operation contract for the UnregisterNodes service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/UnregisterNodes", ReplyAction = Namespaces.OpcUaWsdl + "/UnregisterNodesResponse")]
        #endif
        IAsyncResult BeginUnregisterNodes(UnregisterNodesMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a UnregisterNodes service request.
        /// </summary>
        UnregisterNodesResponseMessage EndUnregisterNodes(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the UnregisterNodes service.
        /// </summary>
        Task<UnregisterNodesResponseMessage> UnregisterNodesAsync(UnregisterNodesMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_QueryFirst)
        /// <summary>
        /// The operation contract for the QueryFirst service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/QueryFirst", ReplyAction = Namespaces.OpcUaWsdl + "/QueryFirstResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/QueryFirstFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        QueryFirstResponseMessage QueryFirst(QueryFirstMessage request);

        /// <summary>
        /// The operation contract for the QueryFirst service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/QueryFirst", ReplyAction = Namespaces.OpcUaWsdl + "/QueryFirstResponse")]
        #endif
        IAsyncResult BeginQueryFirst(QueryFirstMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a QueryFirst service request.
        /// </summary>
        QueryFirstResponseMessage EndQueryFirst(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the QueryFirst service.
        /// </summary>
        Task<QueryFirstResponseMessage> QueryFirstAsync(QueryFirstMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_QueryNext)
        /// <summary>
        /// The operation contract for the QueryNext service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/QueryNext", ReplyAction = Namespaces.OpcUaWsdl + "/QueryNextResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/QueryNextFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        QueryNextResponseMessage QueryNext(QueryNextMessage request);

        /// <summary>
        /// The operation contract for the QueryNext service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/QueryNext", ReplyAction = Namespaces.OpcUaWsdl + "/QueryNextResponse")]
        #endif
        IAsyncResult BeginQueryNext(QueryNextMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a QueryNext service request.
        /// </summary>
        QueryNextResponseMessage EndQueryNext(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the QueryNext service.
        /// </summary>
        Task<QueryNextResponseMessage> QueryNextAsync(QueryNextMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_Read)
        /// <summary>
        /// The operation contract for the Read service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/Read", ReplyAction = Namespaces.OpcUaWsdl + "/ReadResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/ReadFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        ReadResponseMessage Read(ReadMessage request);

        /// <summary>
        /// The operation contract for the Read service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/Read", ReplyAction = Namespaces.OpcUaWsdl + "/ReadResponse")]
        #endif
        IAsyncResult BeginRead(ReadMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a Read service request.
        /// </summary>
        ReadResponseMessage EndRead(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the Read service.
        /// </summary>
        Task<ReadResponseMessage> ReadAsync(ReadMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_HistoryRead)
        /// <summary>
        /// The operation contract for the HistoryRead service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/HistoryRead", ReplyAction = Namespaces.OpcUaWsdl + "/HistoryReadResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/HistoryReadFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        HistoryReadResponseMessage HistoryRead(HistoryReadMessage request);

        /// <summary>
        /// The operation contract for the HistoryRead service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/HistoryRead", ReplyAction = Namespaces.OpcUaWsdl + "/HistoryReadResponse")]
        #endif
        IAsyncResult BeginHistoryRead(HistoryReadMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a HistoryRead service request.
        /// </summary>
        HistoryReadResponseMessage EndHistoryRead(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the HistoryRead service.
        /// </summary>
        Task<HistoryReadResponseMessage> HistoryReadAsync(HistoryReadMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_Write)
        /// <summary>
        /// The operation contract for the Write service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/Write", ReplyAction = Namespaces.OpcUaWsdl + "/WriteResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/WriteFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        WriteResponseMessage Write(WriteMessage request);

        /// <summary>
        /// The operation contract for the Write service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/Write", ReplyAction = Namespaces.OpcUaWsdl + "/WriteResponse")]
        #endif
        IAsyncResult BeginWrite(WriteMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a Write service request.
        /// </summary>
        WriteResponseMessage EndWrite(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the Write service.
        /// </summary>
        Task<WriteResponseMessage> WriteAsync(WriteMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_HistoryUpdate)
        /// <summary>
        /// The operation contract for the HistoryUpdate service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/HistoryUpdate", ReplyAction = Namespaces.OpcUaWsdl + "/HistoryUpdateResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/HistoryUpdateFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        HistoryUpdateResponseMessage HistoryUpdate(HistoryUpdateMessage request);

        /// <summary>
        /// The operation contract for the HistoryUpdate service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/HistoryUpdate", ReplyAction = Namespaces.OpcUaWsdl + "/HistoryUpdateResponse")]
        #endif
        IAsyncResult BeginHistoryUpdate(HistoryUpdateMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a HistoryUpdate service request.
        /// </summary>
        HistoryUpdateResponseMessage EndHistoryUpdate(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the HistoryUpdate service.
        /// </summary>
        Task<HistoryUpdateResponseMessage> HistoryUpdateAsync(HistoryUpdateMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_Call)
        /// <summary>
        /// The operation contract for the Call service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/Call", ReplyAction = Namespaces.OpcUaWsdl + "/CallResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/CallFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        CallResponseMessage Call(CallMessage request);

        /// <summary>
        /// The operation contract for the Call service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/Call", ReplyAction = Namespaces.OpcUaWsdl + "/CallResponse")]
        #endif
        IAsyncResult BeginCall(CallMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a Call service request.
        /// </summary>
        CallResponseMessage EndCall(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the Call service.
        /// </summary>
        Task<CallResponseMessage> CallAsync(CallMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_CreateMonitoredItems)
        /// <summary>
        /// The operation contract for the CreateMonitoredItems service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/CreateMonitoredItems", ReplyAction = Namespaces.OpcUaWsdl + "/CreateMonitoredItemsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/CreateMonitoredItemsFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        CreateMonitoredItemsResponseMessage CreateMonitoredItems(CreateMonitoredItemsMessage request);

        /// <summary>
        /// The operation contract for the CreateMonitoredItems service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/CreateMonitoredItems", ReplyAction = Namespaces.OpcUaWsdl + "/CreateMonitoredItemsResponse")]
        #endif
        IAsyncResult BeginCreateMonitoredItems(CreateMonitoredItemsMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a CreateMonitoredItems service request.
        /// </summary>
        CreateMonitoredItemsResponseMessage EndCreateMonitoredItems(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the CreateMonitoredItems service.
        /// </summary>
        Task<CreateMonitoredItemsResponseMessage> CreateMonitoredItemsAsync(CreateMonitoredItemsMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_ModifyMonitoredItems)
        /// <summary>
        /// The operation contract for the ModifyMonitoredItems service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/ModifyMonitoredItems", ReplyAction = Namespaces.OpcUaWsdl + "/ModifyMonitoredItemsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/ModifyMonitoredItemsFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        ModifyMonitoredItemsResponseMessage ModifyMonitoredItems(ModifyMonitoredItemsMessage request);

        /// <summary>
        /// The operation contract for the ModifyMonitoredItems service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/ModifyMonitoredItems", ReplyAction = Namespaces.OpcUaWsdl + "/ModifyMonitoredItemsResponse")]
        #endif
        IAsyncResult BeginModifyMonitoredItems(ModifyMonitoredItemsMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a ModifyMonitoredItems service request.
        /// </summary>
        ModifyMonitoredItemsResponseMessage EndModifyMonitoredItems(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the ModifyMonitoredItems service.
        /// </summary>
        Task<ModifyMonitoredItemsResponseMessage> ModifyMonitoredItemsAsync(ModifyMonitoredItemsMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_SetMonitoringMode)
        /// <summary>
        /// The operation contract for the SetMonitoringMode service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/SetMonitoringMode", ReplyAction = Namespaces.OpcUaWsdl + "/SetMonitoringModeResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/SetMonitoringModeFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        SetMonitoringModeResponseMessage SetMonitoringMode(SetMonitoringModeMessage request);

        /// <summary>
        /// The operation contract for the SetMonitoringMode service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/SetMonitoringMode", ReplyAction = Namespaces.OpcUaWsdl + "/SetMonitoringModeResponse")]
        #endif
        IAsyncResult BeginSetMonitoringMode(SetMonitoringModeMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a SetMonitoringMode service request.
        /// </summary>
        SetMonitoringModeResponseMessage EndSetMonitoringMode(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the SetMonitoringMode service.
        /// </summary>
        Task<SetMonitoringModeResponseMessage> SetMonitoringModeAsync(SetMonitoringModeMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_SetTriggering)
        /// <summary>
        /// The operation contract for the SetTriggering service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/SetTriggering", ReplyAction = Namespaces.OpcUaWsdl + "/SetTriggeringResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/SetTriggeringFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        SetTriggeringResponseMessage SetTriggering(SetTriggeringMessage request);

        /// <summary>
        /// The operation contract for the SetTriggering service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/SetTriggering", ReplyAction = Namespaces.OpcUaWsdl + "/SetTriggeringResponse")]
        #endif
        IAsyncResult BeginSetTriggering(SetTriggeringMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a SetTriggering service request.
        /// </summary>
        SetTriggeringResponseMessage EndSetTriggering(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the SetTriggering service.
        /// </summary>
        Task<SetTriggeringResponseMessage> SetTriggeringAsync(SetTriggeringMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_DeleteMonitoredItems)
        /// <summary>
        /// The operation contract for the DeleteMonitoredItems service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/DeleteMonitoredItems", ReplyAction = Namespaces.OpcUaWsdl + "/DeleteMonitoredItemsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/DeleteMonitoredItemsFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        DeleteMonitoredItemsResponseMessage DeleteMonitoredItems(DeleteMonitoredItemsMessage request);

        /// <summary>
        /// The operation contract for the DeleteMonitoredItems service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/DeleteMonitoredItems", ReplyAction = Namespaces.OpcUaWsdl + "/DeleteMonitoredItemsResponse")]
        #endif
        IAsyncResult BeginDeleteMonitoredItems(DeleteMonitoredItemsMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a DeleteMonitoredItems service request.
        /// </summary>
        DeleteMonitoredItemsResponseMessage EndDeleteMonitoredItems(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the DeleteMonitoredItems service.
        /// </summary>
        Task<DeleteMonitoredItemsResponseMessage> DeleteMonitoredItemsAsync(DeleteMonitoredItemsMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_CreateSubscription)
        /// <summary>
        /// The operation contract for the CreateSubscription service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/CreateSubscription", ReplyAction = Namespaces.OpcUaWsdl + "/CreateSubscriptionResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/CreateSubscriptionFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        CreateSubscriptionResponseMessage CreateSubscription(CreateSubscriptionMessage request);

        /// <summary>
        /// The operation contract for the CreateSubscription service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/CreateSubscription", ReplyAction = Namespaces.OpcUaWsdl + "/CreateSubscriptionResponse")]
        #endif
        IAsyncResult BeginCreateSubscription(CreateSubscriptionMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a CreateSubscription service request.
        /// </summary>
        CreateSubscriptionResponseMessage EndCreateSubscription(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the CreateSubscription service.
        /// </summary>
        Task<CreateSubscriptionResponseMessage> CreateSubscriptionAsync(CreateSubscriptionMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_ModifySubscription)
        /// <summary>
        /// The operation contract for the ModifySubscription service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/ModifySubscription", ReplyAction = Namespaces.OpcUaWsdl + "/ModifySubscriptionResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/ModifySubscriptionFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        ModifySubscriptionResponseMessage ModifySubscription(ModifySubscriptionMessage request);

        /// <summary>
        /// The operation contract for the ModifySubscription service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/ModifySubscription", ReplyAction = Namespaces.OpcUaWsdl + "/ModifySubscriptionResponse")]
        #endif
        IAsyncResult BeginModifySubscription(ModifySubscriptionMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a ModifySubscription service request.
        /// </summary>
        ModifySubscriptionResponseMessage EndModifySubscription(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the ModifySubscription service.
        /// </summary>
        Task<ModifySubscriptionResponseMessage> ModifySubscriptionAsync(ModifySubscriptionMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_SetPublishingMode)
        /// <summary>
        /// The operation contract for the SetPublishingMode service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/SetPublishingMode", ReplyAction = Namespaces.OpcUaWsdl + "/SetPublishingModeResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/SetPublishingModeFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        SetPublishingModeResponseMessage SetPublishingMode(SetPublishingModeMessage request);

        /// <summary>
        /// The operation contract for the SetPublishingMode service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/SetPublishingMode", ReplyAction = Namespaces.OpcUaWsdl + "/SetPublishingModeResponse")]
        #endif
        IAsyncResult BeginSetPublishingMode(SetPublishingModeMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a SetPublishingMode service request.
        /// </summary>
        SetPublishingModeResponseMessage EndSetPublishingMode(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the SetPublishingMode service.
        /// </summary>
        Task<SetPublishingModeResponseMessage> SetPublishingModeAsync(SetPublishingModeMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_Publish)
        /// <summary>
        /// The operation contract for the Publish service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/Publish", ReplyAction = Namespaces.OpcUaWsdl + "/PublishResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/PublishFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        PublishResponseMessage Publish(PublishMessage request);

        /// <summary>
        /// The operation contract for the Publish service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/Publish", ReplyAction = Namespaces.OpcUaWsdl + "/PublishResponse")]
        #endif
        IAsyncResult BeginPublish(PublishMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a Publish service request.
        /// </summary>
        PublishResponseMessage EndPublish(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the Publish service.
        /// </summary>
        Task<PublishResponseMessage> PublishAsync(PublishMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_Republish)
        /// <summary>
        /// The operation contract for the Republish service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/Republish", ReplyAction = Namespaces.OpcUaWsdl + "/RepublishResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/RepublishFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        RepublishResponseMessage Republish(RepublishMessage request);

        /// <summary>
        /// The operation contract for the Republish service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/Republish", ReplyAction = Namespaces.OpcUaWsdl + "/RepublishResponse")]
        #endif
        IAsyncResult BeginRepublish(RepublishMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a Republish service request.
        /// </summary>
        RepublishResponseMessage EndRepublish(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the Republish service.
        /// </summary>
        Task<RepublishResponseMessage> RepublishAsync(RepublishMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_TransferSubscriptions)
        /// <summary>
        /// The operation contract for the TransferSubscriptions service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/TransferSubscriptions", ReplyAction = Namespaces.OpcUaWsdl + "/TransferSubscriptionsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/TransferSubscriptionsFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        TransferSubscriptionsResponseMessage TransferSubscriptions(TransferSubscriptionsMessage request);

        /// <summary>
        /// The operation contract for the TransferSubscriptions service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/TransferSubscriptions", ReplyAction = Namespaces.OpcUaWsdl + "/TransferSubscriptionsResponse")]
        #endif
        IAsyncResult BeginTransferSubscriptions(TransferSubscriptionsMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a TransferSubscriptions service request.
        /// </summary>
        TransferSubscriptionsResponseMessage EndTransferSubscriptions(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the TransferSubscriptions service.
        /// </summary>
        Task<TransferSubscriptionsResponseMessage> TransferSubscriptionsAsync(TransferSubscriptionsMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_DeleteSubscriptions)
        /// <summary>
        /// The operation contract for the DeleteSubscriptions service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/DeleteSubscriptions", ReplyAction = Namespaces.OpcUaWsdl + "/DeleteSubscriptionsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/DeleteSubscriptionsFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        DeleteSubscriptionsResponseMessage DeleteSubscriptions(DeleteSubscriptionsMessage request);

        /// <summary>
        /// The operation contract for the DeleteSubscriptions service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/DeleteSubscriptions", ReplyAction = Namespaces.OpcUaWsdl + "/DeleteSubscriptionsResponse")]
        #endif
        IAsyncResult BeginDeleteSubscriptions(DeleteSubscriptionsMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a DeleteSubscriptions service request.
        /// </summary>
        DeleteSubscriptionsResponseMessage EndDeleteSubscriptions(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the DeleteSubscriptions service.
        /// </summary>
        Task<DeleteSubscriptionsResponseMessage> DeleteSubscriptionsAsync(DeleteSubscriptionsMessage request);
        #endif
        #endif
    }
    #endregion

    #region IDiscoveryEndpoint Interface
    #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
    /// <summary>
    /// The service contract which must be implemented by all UA servers.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
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
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    #if (!NET_STANDARD)
    [ServiceContract(Namespace = Namespaces.OpcUaWsdl)]
    #endif
    public interface IDiscoveryEndpoint : IEndpointBase
    {
        #if (!OPCUA_EXCLUDE_FindServers)
        /// <summary>
        /// The operation contract for the FindServers service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/FindServers", ReplyAction = Namespaces.OpcUaWsdl + "/FindServersResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/FindServersFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/FindServersOnNetwork", ReplyAction = Namespaces.OpcUaWsdl + "/FindServersOnNetworkResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/FindServersOnNetworkFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/GetEndpoints", ReplyAction = Namespaces.OpcUaWsdl + "/GetEndpointsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/GetEndpointsFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    #if (!NET_STANDARD)
    [ServiceContract(Namespace = Namespaces.OpcUaWsdl)]
    #endif
    public interface IDiscoveryChannel : IChannelBase
    {
        #if (!OPCUA_EXCLUDE_FindServers)
        /// <summary>
        /// The operation contract for the FindServers service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/FindServers", ReplyAction = Namespaces.OpcUaWsdl + "/FindServersResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/FindServersFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        FindServersResponseMessage FindServers(FindServersMessage request);

        /// <summary>
        /// The operation contract for the FindServers service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/FindServers", ReplyAction = Namespaces.OpcUaWsdl + "/FindServersResponse")]
        #endif
        IAsyncResult BeginFindServers(FindServersMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a FindServers service request.
        /// </summary>
        FindServersResponseMessage EndFindServers(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the FindServers service.
        /// </summary>
        Task<FindServersResponseMessage> FindServersAsync(FindServersMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_FindServersOnNetwork)
        /// <summary>
        /// The operation contract for the FindServersOnNetwork service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/FindServersOnNetwork", ReplyAction = Namespaces.OpcUaWsdl + "/FindServersOnNetworkResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/FindServersOnNetworkFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        FindServersOnNetworkResponseMessage FindServersOnNetwork(FindServersOnNetworkMessage request);

        /// <summary>
        /// The operation contract for the FindServersOnNetwork service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/FindServersOnNetwork", ReplyAction = Namespaces.OpcUaWsdl + "/FindServersOnNetworkResponse")]
        #endif
        IAsyncResult BeginFindServersOnNetwork(FindServersOnNetworkMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a FindServersOnNetwork service request.
        /// </summary>
        FindServersOnNetworkResponseMessage EndFindServersOnNetwork(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the FindServersOnNetwork service.
        /// </summary>
        Task<FindServersOnNetworkResponseMessage> FindServersOnNetworkAsync(FindServersOnNetworkMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_GetEndpoints)
        /// <summary>
        /// The operation contract for the GetEndpoints service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/GetEndpoints", ReplyAction = Namespaces.OpcUaWsdl + "/GetEndpointsResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/GetEndpointsFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        GetEndpointsResponseMessage GetEndpoints(GetEndpointsMessage request);

        /// <summary>
        /// The operation contract for the GetEndpoints service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/GetEndpoints", ReplyAction = Namespaces.OpcUaWsdl + "/GetEndpointsResponse")]
        #endif
        IAsyncResult BeginGetEndpoints(GetEndpointsMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a GetEndpoints service request.
        /// </summary>
        GetEndpointsResponseMessage EndGetEndpoints(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the GetEndpoints service.
        /// </summary>
        Task<GetEndpointsResponseMessage> GetEndpointsAsync(GetEndpointsMessage request);
        #endif
        #endif
    }
    #endregion

    #region IRegistrationEndpoint Interface
    #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
    /// <summary>
    /// The service contract which must be implemented by all UA servers.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
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
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    #if (!NET_STANDARD)
    [ServiceContract(Namespace = Namespaces.OpcUaWsdl)]
    #endif
    public interface IRegistrationEndpoint : IEndpointBase
    {
        #if (!OPCUA_EXCLUDE_RegisterServer)
        /// <summary>
        /// The operation contract for the RegisterServer service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/RegisterServer", ReplyAction = Namespaces.OpcUaWsdl + "/RegisterServerResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/RegisterServerFault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/RegisterServer2", ReplyAction = Namespaces.OpcUaWsdl + "/RegisterServer2Response")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/RegisterServer2Fault", Name = "ServiceFault", Namespace = Namespaces.OpcUaXsd)]
        #endif
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
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    #if (!NET_STANDARD)
    [ServiceContract(Namespace = Namespaces.OpcUaWsdl)]
    #endif
    public interface IRegistrationChannel : IChannelBase
    {
        #if (!OPCUA_EXCLUDE_RegisterServer)
        /// <summary>
        /// The operation contract for the RegisterServer service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/RegisterServer", ReplyAction = Namespaces.OpcUaWsdl + "/RegisterServerResponse")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/RegisterServerFault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        RegisterServerResponseMessage RegisterServer(RegisterServerMessage request);

        /// <summary>
        /// The operation contract for the RegisterServer service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/RegisterServer", ReplyAction = Namespaces.OpcUaWsdl + "/RegisterServerResponse")]
        #endif
        IAsyncResult BeginRegisterServer(RegisterServerMessage request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a RegisterServer service request.
        /// </summary>
        RegisterServerResponseMessage EndRegisterServer(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the RegisterServer service.
        /// </summary>
        Task<RegisterServerResponseMessage> RegisterServerAsync(RegisterServerMessage request);
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_RegisterServer2)
        /// <summary>
        /// The operation contract for the RegisterServer2 service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContract(Action = Namespaces.OpcUaWsdl + "/RegisterServer2", ReplyAction = Namespaces.OpcUaWsdl + "/RegisterServer2Response")]
        [FaultContract(typeof(ServiceFault), Action = Namespaces.OpcUaWsdl + "/RegisterServer2Fault", Name="ServiceFault", Namespace=Namespaces.OpcUaXsd)]
        #endif
        RegisterServer2ResponseMessage RegisterServer2(RegisterServer2Message request);

        /// <summary>
        /// The operation contract for the RegisterServer2 service.
        /// </summary>
        #if (!NET_STANDARD)
        [OperationContractAttribute(AsyncPattern=true, Action=Namespaces.OpcUaWsdl + "/RegisterServer2", ReplyAction = Namespaces.OpcUaWsdl + "/RegisterServer2Response")]
        #endif
        IAsyncResult BeginRegisterServer2(RegisterServer2Message request, AsyncCallback callback, object asyncState);

        /// <summary>
        /// The method used to retrieve the results of a RegisterServer2 service request.
        /// </summary>
        RegisterServer2ResponseMessage EndRegisterServer2(IAsyncResult result);

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async operation contract for the RegisterServer2 service.
        /// </summary>
        Task<RegisterServer2ResponseMessage> RegisterServer2Async(RegisterServer2Message request);
        #endif
        #endif
    }
    #endregion
}
