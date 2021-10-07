/* ========================================================================
 * Copyright (c) 2005-2020 The OPC Foundation, Inc. All rights reserved.
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
using System.ServiceModel.Channels;
using System.Runtime.Serialization;

namespace Opc.Ua
{
    #region SessionChannel Class
    /// <summary>
    /// A channel object used by clients to access a UA service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class SessionChannel : WcfChannelBase<ISessionChannel>, ISessionChannel
    {
        /// <summary>
        /// Initializes the object with the endpoint address.
        /// </summary>
        internal SessionChannel()
        {
        }

        #if (!OPCUA_EXCLUDE_CreateSession)
        /// <summary>
        /// The client side implementation of the CreateSession service contract.
        /// </summary>
        public CreateSessionResponseMessage CreateSession(CreateSessionMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginCreateSession(request, null, null);
                }

                return this.Channel.EndCreateSession(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginCreateSession service contract.
        /// </summary>
        public IAsyncResult BeginCreateSession(CreateSessionMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginCreateSession(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndCreateSession service contract.
        /// </summary>
        public CreateSessionResponseMessage EndCreateSession(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndCreateSession(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_ActivateSession)
        /// <summary>
        /// The client side implementation of the ActivateSession service contract.
        /// </summary>
        public ActivateSessionResponseMessage ActivateSession(ActivateSessionMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginActivateSession(request, null, null);
                }

                return this.Channel.EndActivateSession(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginActivateSession service contract.
        /// </summary>
        public IAsyncResult BeginActivateSession(ActivateSessionMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginActivateSession(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndActivateSession service contract.
        /// </summary>
        public ActivateSessionResponseMessage EndActivateSession(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndActivateSession(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_CloseSession)
        /// <summary>
        /// The client side implementation of the CloseSession service contract.
        /// </summary>
        public CloseSessionResponseMessage CloseSession(CloseSessionMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginCloseSession(request, null, null);
                }

                return this.Channel.EndCloseSession(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginCloseSession service contract.
        /// </summary>
        public IAsyncResult BeginCloseSession(CloseSessionMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginCloseSession(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndCloseSession service contract.
        /// </summary>
        public CloseSessionResponseMessage EndCloseSession(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndCloseSession(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_Cancel)
        /// <summary>
        /// The client side implementation of the Cancel service contract.
        /// </summary>
        public CancelResponseMessage Cancel(CancelMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginCancel(request, null, null);
                }

                return this.Channel.EndCancel(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginCancel service contract.
        /// </summary>
        public IAsyncResult BeginCancel(CancelMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginCancel(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndCancel service contract.
        /// </summary>
        public CancelResponseMessage EndCancel(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndCancel(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_AddNodes)
        /// <summary>
        /// The client side implementation of the AddNodes service contract.
        /// </summary>
        public AddNodesResponseMessage AddNodes(AddNodesMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginAddNodes(request, null, null);
                }

                return this.Channel.EndAddNodes(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginAddNodes service contract.
        /// </summary>
        public IAsyncResult BeginAddNodes(AddNodesMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginAddNodes(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndAddNodes service contract.
        /// </summary>
        public AddNodesResponseMessage EndAddNodes(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndAddNodes(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_AddReferences)
        /// <summary>
        /// The client side implementation of the AddReferences service contract.
        /// </summary>
        public AddReferencesResponseMessage AddReferences(AddReferencesMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginAddReferences(request, null, null);
                }

                return this.Channel.EndAddReferences(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginAddReferences service contract.
        /// </summary>
        public IAsyncResult BeginAddReferences(AddReferencesMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginAddReferences(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndAddReferences service contract.
        /// </summary>
        public AddReferencesResponseMessage EndAddReferences(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndAddReferences(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_DeleteNodes)
        /// <summary>
        /// The client side implementation of the DeleteNodes service contract.
        /// </summary>
        public DeleteNodesResponseMessage DeleteNodes(DeleteNodesMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginDeleteNodes(request, null, null);
                }

                return this.Channel.EndDeleteNodes(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginDeleteNodes service contract.
        /// </summary>
        public IAsyncResult BeginDeleteNodes(DeleteNodesMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginDeleteNodes(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndDeleteNodes service contract.
        /// </summary>
        public DeleteNodesResponseMessage EndDeleteNodes(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndDeleteNodes(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_DeleteReferences)
        /// <summary>
        /// The client side implementation of the DeleteReferences service contract.
        /// </summary>
        public DeleteReferencesResponseMessage DeleteReferences(DeleteReferencesMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginDeleteReferences(request, null, null);
                }

                return this.Channel.EndDeleteReferences(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginDeleteReferences service contract.
        /// </summary>
        public IAsyncResult BeginDeleteReferences(DeleteReferencesMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginDeleteReferences(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndDeleteReferences service contract.
        /// </summary>
        public DeleteReferencesResponseMessage EndDeleteReferences(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndDeleteReferences(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_Browse)
        /// <summary>
        /// The client side implementation of the Browse service contract.
        /// </summary>
        public BrowseResponseMessage Browse(BrowseMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginBrowse(request, null, null);
                }

                return this.Channel.EndBrowse(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginBrowse service contract.
        /// </summary>
        public IAsyncResult BeginBrowse(BrowseMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginBrowse(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndBrowse service contract.
        /// </summary>
        public BrowseResponseMessage EndBrowse(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndBrowse(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_BrowseNext)
        /// <summary>
        /// The client side implementation of the BrowseNext service contract.
        /// </summary>
        public BrowseNextResponseMessage BrowseNext(BrowseNextMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginBrowseNext(request, null, null);
                }

                return this.Channel.EndBrowseNext(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginBrowseNext service contract.
        /// </summary>
        public IAsyncResult BeginBrowseNext(BrowseNextMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginBrowseNext(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndBrowseNext service contract.
        /// </summary>
        public BrowseNextResponseMessage EndBrowseNext(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndBrowseNext(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_TranslateBrowsePathsToNodeIds)
        /// <summary>
        /// The client side implementation of the TranslateBrowsePathsToNodeIds service contract.
        /// </summary>
        public TranslateBrowsePathsToNodeIdsResponseMessage TranslateBrowsePathsToNodeIds(TranslateBrowsePathsToNodeIdsMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginTranslateBrowsePathsToNodeIds(request, null, null);
                }

                return this.Channel.EndTranslateBrowsePathsToNodeIds(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginTranslateBrowsePathsToNodeIds service contract.
        /// </summary>
        public IAsyncResult BeginTranslateBrowsePathsToNodeIds(TranslateBrowsePathsToNodeIdsMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginTranslateBrowsePathsToNodeIds(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndTranslateBrowsePathsToNodeIds service contract.
        /// </summary>
        public TranslateBrowsePathsToNodeIdsResponseMessage EndTranslateBrowsePathsToNodeIds(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndTranslateBrowsePathsToNodeIds(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_RegisterNodes)
        /// <summary>
        /// The client side implementation of the RegisterNodes service contract.
        /// </summary>
        public RegisterNodesResponseMessage RegisterNodes(RegisterNodesMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginRegisterNodes(request, null, null);
                }

                return this.Channel.EndRegisterNodes(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginRegisterNodes service contract.
        /// </summary>
        public IAsyncResult BeginRegisterNodes(RegisterNodesMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginRegisterNodes(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndRegisterNodes service contract.
        /// </summary>
        public RegisterNodesResponseMessage EndRegisterNodes(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndRegisterNodes(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_UnregisterNodes)
        /// <summary>
        /// The client side implementation of the UnregisterNodes service contract.
        /// </summary>
        public UnregisterNodesResponseMessage UnregisterNodes(UnregisterNodesMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginUnregisterNodes(request, null, null);
                }

                return this.Channel.EndUnregisterNodes(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginUnregisterNodes service contract.
        /// </summary>
        public IAsyncResult BeginUnregisterNodes(UnregisterNodesMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginUnregisterNodes(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndUnregisterNodes service contract.
        /// </summary>
        public UnregisterNodesResponseMessage EndUnregisterNodes(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndUnregisterNodes(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_QueryFirst)
        /// <summary>
        /// The client side implementation of the QueryFirst service contract.
        /// </summary>
        public QueryFirstResponseMessage QueryFirst(QueryFirstMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginQueryFirst(request, null, null);
                }

                return this.Channel.EndQueryFirst(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginQueryFirst service contract.
        /// </summary>
        public IAsyncResult BeginQueryFirst(QueryFirstMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginQueryFirst(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndQueryFirst service contract.
        /// </summary>
        public QueryFirstResponseMessage EndQueryFirst(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndQueryFirst(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_QueryNext)
        /// <summary>
        /// The client side implementation of the QueryNext service contract.
        /// </summary>
        public QueryNextResponseMessage QueryNext(QueryNextMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginQueryNext(request, null, null);
                }

                return this.Channel.EndQueryNext(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginQueryNext service contract.
        /// </summary>
        public IAsyncResult BeginQueryNext(QueryNextMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginQueryNext(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndQueryNext service contract.
        /// </summary>
        public QueryNextResponseMessage EndQueryNext(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndQueryNext(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_Read)
        /// <summary>
        /// The client side implementation of the Read service contract.
        /// </summary>
        public ReadResponseMessage Read(ReadMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginRead(request, null, null);
                }

                return this.Channel.EndRead(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginRead service contract.
        /// </summary>
        public IAsyncResult BeginRead(ReadMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginRead(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndRead service contract.
        /// </summary>
        public ReadResponseMessage EndRead(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndRead(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_HistoryRead)
        /// <summary>
        /// The client side implementation of the HistoryRead service contract.
        /// </summary>
        public HistoryReadResponseMessage HistoryRead(HistoryReadMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginHistoryRead(request, null, null);
                }

                return this.Channel.EndHistoryRead(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginHistoryRead service contract.
        /// </summary>
        public IAsyncResult BeginHistoryRead(HistoryReadMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginHistoryRead(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndHistoryRead service contract.
        /// </summary>
        public HistoryReadResponseMessage EndHistoryRead(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndHistoryRead(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_Write)
        /// <summary>
        /// The client side implementation of the Write service contract.
        /// </summary>
        public WriteResponseMessage Write(WriteMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginWrite(request, null, null);
                }

                return this.Channel.EndWrite(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginWrite service contract.
        /// </summary>
        public IAsyncResult BeginWrite(WriteMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginWrite(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndWrite service contract.
        /// </summary>
        public WriteResponseMessage EndWrite(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndWrite(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_HistoryUpdate)
        /// <summary>
        /// The client side implementation of the HistoryUpdate service contract.
        /// </summary>
        public HistoryUpdateResponseMessage HistoryUpdate(HistoryUpdateMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginHistoryUpdate(request, null, null);
                }

                return this.Channel.EndHistoryUpdate(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginHistoryUpdate service contract.
        /// </summary>
        public IAsyncResult BeginHistoryUpdate(HistoryUpdateMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginHistoryUpdate(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndHistoryUpdate service contract.
        /// </summary>
        public HistoryUpdateResponseMessage EndHistoryUpdate(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndHistoryUpdate(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_Call)
        /// <summary>
        /// The client side implementation of the Call service contract.
        /// </summary>
        public CallResponseMessage Call(CallMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginCall(request, null, null);
                }

                return this.Channel.EndCall(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginCall service contract.
        /// </summary>
        public IAsyncResult BeginCall(CallMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginCall(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndCall service contract.
        /// </summary>
        public CallResponseMessage EndCall(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndCall(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_CreateMonitoredItems)
        /// <summary>
        /// The client side implementation of the CreateMonitoredItems service contract.
        /// </summary>
        public CreateMonitoredItemsResponseMessage CreateMonitoredItems(CreateMonitoredItemsMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginCreateMonitoredItems(request, null, null);
                }

                return this.Channel.EndCreateMonitoredItems(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginCreateMonitoredItems service contract.
        /// </summary>
        public IAsyncResult BeginCreateMonitoredItems(CreateMonitoredItemsMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginCreateMonitoredItems(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndCreateMonitoredItems service contract.
        /// </summary>
        public CreateMonitoredItemsResponseMessage EndCreateMonitoredItems(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndCreateMonitoredItems(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_ModifyMonitoredItems)
        /// <summary>
        /// The client side implementation of the ModifyMonitoredItems service contract.
        /// </summary>
        public ModifyMonitoredItemsResponseMessage ModifyMonitoredItems(ModifyMonitoredItemsMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginModifyMonitoredItems(request, null, null);
                }

                return this.Channel.EndModifyMonitoredItems(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginModifyMonitoredItems service contract.
        /// </summary>
        public IAsyncResult BeginModifyMonitoredItems(ModifyMonitoredItemsMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginModifyMonitoredItems(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndModifyMonitoredItems service contract.
        /// </summary>
        public ModifyMonitoredItemsResponseMessage EndModifyMonitoredItems(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndModifyMonitoredItems(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_SetMonitoringMode)
        /// <summary>
        /// The client side implementation of the SetMonitoringMode service contract.
        /// </summary>
        public SetMonitoringModeResponseMessage SetMonitoringMode(SetMonitoringModeMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginSetMonitoringMode(request, null, null);
                }

                return this.Channel.EndSetMonitoringMode(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginSetMonitoringMode service contract.
        /// </summary>
        public IAsyncResult BeginSetMonitoringMode(SetMonitoringModeMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginSetMonitoringMode(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndSetMonitoringMode service contract.
        /// </summary>
        public SetMonitoringModeResponseMessage EndSetMonitoringMode(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndSetMonitoringMode(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_SetTriggering)
        /// <summary>
        /// The client side implementation of the SetTriggering service contract.
        /// </summary>
        public SetTriggeringResponseMessage SetTriggering(SetTriggeringMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginSetTriggering(request, null, null);
                }

                return this.Channel.EndSetTriggering(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginSetTriggering service contract.
        /// </summary>
        public IAsyncResult BeginSetTriggering(SetTriggeringMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginSetTriggering(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndSetTriggering service contract.
        /// </summary>
        public SetTriggeringResponseMessage EndSetTriggering(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndSetTriggering(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_DeleteMonitoredItems)
        /// <summary>
        /// The client side implementation of the DeleteMonitoredItems service contract.
        /// </summary>
        public DeleteMonitoredItemsResponseMessage DeleteMonitoredItems(DeleteMonitoredItemsMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginDeleteMonitoredItems(request, null, null);
                }

                return this.Channel.EndDeleteMonitoredItems(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginDeleteMonitoredItems service contract.
        /// </summary>
        public IAsyncResult BeginDeleteMonitoredItems(DeleteMonitoredItemsMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginDeleteMonitoredItems(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndDeleteMonitoredItems service contract.
        /// </summary>
        public DeleteMonitoredItemsResponseMessage EndDeleteMonitoredItems(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndDeleteMonitoredItems(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_CreateSubscription)
        /// <summary>
        /// The client side implementation of the CreateSubscription service contract.
        /// </summary>
        public CreateSubscriptionResponseMessage CreateSubscription(CreateSubscriptionMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginCreateSubscription(request, null, null);
                }

                return this.Channel.EndCreateSubscription(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginCreateSubscription service contract.
        /// </summary>
        public IAsyncResult BeginCreateSubscription(CreateSubscriptionMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginCreateSubscription(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndCreateSubscription service contract.
        /// </summary>
        public CreateSubscriptionResponseMessage EndCreateSubscription(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndCreateSubscription(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_ModifySubscription)
        /// <summary>
        /// The client side implementation of the ModifySubscription service contract.
        /// </summary>
        public ModifySubscriptionResponseMessage ModifySubscription(ModifySubscriptionMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginModifySubscription(request, null, null);
                }

                return this.Channel.EndModifySubscription(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginModifySubscription service contract.
        /// </summary>
        public IAsyncResult BeginModifySubscription(ModifySubscriptionMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginModifySubscription(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndModifySubscription service contract.
        /// </summary>
        public ModifySubscriptionResponseMessage EndModifySubscription(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndModifySubscription(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_SetPublishingMode)
        /// <summary>
        /// The client side implementation of the SetPublishingMode service contract.
        /// </summary>
        public SetPublishingModeResponseMessage SetPublishingMode(SetPublishingModeMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginSetPublishingMode(request, null, null);
                }

                return this.Channel.EndSetPublishingMode(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginSetPublishingMode service contract.
        /// </summary>
        public IAsyncResult BeginSetPublishingMode(SetPublishingModeMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginSetPublishingMode(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndSetPublishingMode service contract.
        /// </summary>
        public SetPublishingModeResponseMessage EndSetPublishingMode(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndSetPublishingMode(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_Publish)
        /// <summary>
        /// The client side implementation of the Publish service contract.
        /// </summary>
        public PublishResponseMessage Publish(PublishMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginPublish(request, null, null);
                }

                return this.Channel.EndPublish(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginPublish service contract.
        /// </summary>
        public IAsyncResult BeginPublish(PublishMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginPublish(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndPublish service contract.
        /// </summary>
        public PublishResponseMessage EndPublish(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndPublish(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_Republish)
        /// <summary>
        /// The client side implementation of the Republish service contract.
        /// </summary>
        public RepublishResponseMessage Republish(RepublishMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginRepublish(request, null, null);
                }

                return this.Channel.EndRepublish(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginRepublish service contract.
        /// </summary>
        public IAsyncResult BeginRepublish(RepublishMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginRepublish(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndRepublish service contract.
        /// </summary>
        public RepublishResponseMessage EndRepublish(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndRepublish(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_TransferSubscriptions)
        /// <summary>
        /// The client side implementation of the TransferSubscriptions service contract.
        /// </summary>
        public TransferSubscriptionsResponseMessage TransferSubscriptions(TransferSubscriptionsMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginTransferSubscriptions(request, null, null);
                }

                return this.Channel.EndTransferSubscriptions(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginTransferSubscriptions service contract.
        /// </summary>
        public IAsyncResult BeginTransferSubscriptions(TransferSubscriptionsMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginTransferSubscriptions(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndTransferSubscriptions service contract.
        /// </summary>
        public TransferSubscriptionsResponseMessage EndTransferSubscriptions(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndTransferSubscriptions(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_DeleteSubscriptions)
        /// <summary>
        /// The client side implementation of the DeleteSubscriptions service contract.
        /// </summary>
        public DeleteSubscriptionsResponseMessage DeleteSubscriptions(DeleteSubscriptionsMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginDeleteSubscriptions(request, null, null);
                }

                return this.Channel.EndDeleteSubscriptions(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginDeleteSubscriptions service contract.
        /// </summary>
        public IAsyncResult BeginDeleteSubscriptions(DeleteSubscriptionsMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginDeleteSubscriptions(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndDeleteSubscriptions service contract.
        /// </summary>
        public DeleteSubscriptionsResponseMessage EndDeleteSubscriptions(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndDeleteSubscriptions(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif
    }
    #endregion

    #region DiscoveryChannel Class
    /// <summary>
    /// A channel object used by clients to access a UA service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class DiscoveryChannel : WcfChannelBase<IDiscoveryChannel>, IDiscoveryChannel
    {
        /// <summary>
        /// Initializes the object with the endpoint address.
        /// </summary>
        internal DiscoveryChannel()
        {
        }

        #if (!OPCUA_EXCLUDE_FindServers)
        /// <summary>
        /// The client side implementation of the FindServers service contract.
        /// </summary>
        public FindServersResponseMessage FindServers(FindServersMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginFindServers(request, null, null);
                }

                return this.Channel.EndFindServers(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginFindServers service contract.
        /// </summary>
        public IAsyncResult BeginFindServers(FindServersMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginFindServers(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndFindServers service contract.
        /// </summary>
        public FindServersResponseMessage EndFindServers(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndFindServers(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_FindServersOnNetwork)
        /// <summary>
        /// The client side implementation of the FindServersOnNetwork service contract.
        /// </summary>
        public FindServersOnNetworkResponseMessage FindServersOnNetwork(FindServersOnNetworkMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginFindServersOnNetwork(request, null, null);
                }

                return this.Channel.EndFindServersOnNetwork(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginFindServersOnNetwork service contract.
        /// </summary>
        public IAsyncResult BeginFindServersOnNetwork(FindServersOnNetworkMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginFindServersOnNetwork(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndFindServersOnNetwork service contract.
        /// </summary>
        public FindServersOnNetworkResponseMessage EndFindServersOnNetwork(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndFindServersOnNetwork(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_GetEndpoints)
        /// <summary>
        /// The client side implementation of the GetEndpoints service contract.
        /// </summary>
        public GetEndpointsResponseMessage GetEndpoints(GetEndpointsMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginGetEndpoints(request, null, null);
                }

                return this.Channel.EndGetEndpoints(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginGetEndpoints service contract.
        /// </summary>
        public IAsyncResult BeginGetEndpoints(GetEndpointsMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginGetEndpoints(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndGetEndpoints service contract.
        /// </summary>
        public GetEndpointsResponseMessage EndGetEndpoints(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndGetEndpoints(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif
    }
    #endregion

    #region RegistrationChannel Class
    /// <summary>
    /// A channel object used by clients to access a UA service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class RegistrationChannel : WcfChannelBase<IRegistrationChannel>, IRegistrationChannel
    {
        /// <summary>
        /// Initializes the object with the endpoint address.
        /// </summary>
        internal RegistrationChannel()
        {
        }

        #if (!OPCUA_EXCLUDE_RegisterServer)
        /// <summary>
        /// The client side implementation of the RegisterServer service contract.
        /// </summary>
        public RegisterServerResponseMessage RegisterServer(RegisterServerMessage request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginRegisterServer(request, null, null);
                }

                return this.Channel.EndRegisterServer(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginRegisterServer service contract.
        /// </summary>
        public IAsyncResult BeginRegisterServer(RegisterServerMessage request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginRegisterServer(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndRegisterServer service contract.
        /// </summary>
        public RegisterServerResponseMessage EndRegisterServer(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndRegisterServer(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif

        #if (!OPCUA_EXCLUDE_RegisterServer2)
        /// <summary>
        /// The client side implementation of the RegisterServer2 service contract.
        /// </summary>
        public RegisterServer2ResponseMessage RegisterServer2(RegisterServer2Message request)
        {
            try
            {
                IAsyncResult result = null;

                lock (this.Channel)
                {
                    result = this.Channel.BeginRegisterServer2(request, null, null);
                }

                return this.Channel.EndRegisterServer2(result);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }

        /// <summary>
        /// The client side implementation of the BeginRegisterServer2 service contract.
        /// </summary>
        public IAsyncResult BeginRegisterServer2(RegisterServer2Message request, AsyncCallback callback, object asyncState)
        {
            WcfChannelAsyncResult asyncResult = new WcfChannelAsyncResult(Channel, callback, asyncState);

            lock (asyncResult.Lock)
            {
                asyncResult.InnerResult = asyncResult.Channel.BeginRegisterServer2(request, asyncResult.OnOperationCompleted, null);
            }

            return asyncResult;
        }

        /// <summary>
        /// The client side implementation of the EndRegisterServer2 service contract.
        /// </summary>
        public RegisterServer2ResponseMessage EndRegisterServer2(IAsyncResult result)
        {
            try
            {
                WcfChannelAsyncResult asyncResult = WcfChannelAsyncResult.WaitForComplete(result);
                return asyncResult.Channel.EndRegisterServer2(asyncResult.InnerResult);
            }
            catch (FaultException<ServiceFault> e)
            {
                throw HandleSoapFault(e);
            }
        }
        #endif
    }
    #endregion
}
