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
using System.ServiceModel.Channels;
using System.Runtime.Serialization;
#endif

#if (NET_STANDARD_ASYNC)
using System.Threading.Tasks;
#endif

namespace Opc.Ua
{
    #region SessionChannel Class
    /// <summary>
    /// A channel object used by clients to access a UA service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    #if (!NET_STANDARD)
    public partial class SessionChannel : WcfChannelBase<ISessionChannel>, ISessionChannel
    #else
    public partial class SessionChannel : UaChannelBase<ISessionChannel>, ISessionChannel
    #endif
    {
        /// <summary>
        /// Initializes the object with the endpoint address.
        /// </summary>
        internal SessionChannel()
        {
        }

        #if (!OPCUA_EXCLUDE_CreateSession)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the CreateSession service contract.
        /// </summary>
        public CreateSessionResponseMessage CreateSession(CreateSessionMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginCreateSession(request, null, null);
            }

            return this.Channel.EndCreateSession(result);
        }

        /// <summary>
        /// The client side implementation of the BeginCreateSession service contract.
        /// </summary>
        public IAsyncResult BeginCreateSession(CreateSessionMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndCreateSession(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the CreateSession service contract.
        /// </summary>
        public Task<CreateSessionResponseMessage> CreateSessionAsync(CreateSessionMessage request)
        {
            return this.Channel.CreateSessionAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_ActivateSession)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the ActivateSession service contract.
        /// </summary>
        public ActivateSessionResponseMessage ActivateSession(ActivateSessionMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginActivateSession(request, null, null);
            }

            return this.Channel.EndActivateSession(result);
        }

        /// <summary>
        /// The client side implementation of the BeginActivateSession service contract.
        /// </summary>
        public IAsyncResult BeginActivateSession(ActivateSessionMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndActivateSession(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the ActivateSession service contract.
        /// </summary>
        public Task<ActivateSessionResponseMessage> ActivateSessionAsync(ActivateSessionMessage request)
        {
            return this.Channel.ActivateSessionAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_CloseSession)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the CloseSession service contract.
        /// </summary>
        public CloseSessionResponseMessage CloseSession(CloseSessionMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginCloseSession(request, null, null);
            }

            return this.Channel.EndCloseSession(result);
        }

        /// <summary>
        /// The client side implementation of the BeginCloseSession service contract.
        /// </summary>
        public IAsyncResult BeginCloseSession(CloseSessionMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndCloseSession(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the CloseSession service contract.
        /// </summary>
        public Task<CloseSessionResponseMessage> CloseSessionAsync(CloseSessionMessage request)
        {
            return this.Channel.CloseSessionAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_Cancel)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the Cancel service contract.
        /// </summary>
        public CancelResponseMessage Cancel(CancelMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginCancel(request, null, null);
            }

            return this.Channel.EndCancel(result);
        }

        /// <summary>
        /// The client side implementation of the BeginCancel service contract.
        /// </summary>
        public IAsyncResult BeginCancel(CancelMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndCancel(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the Cancel service contract.
        /// </summary>
        public Task<CancelResponseMessage> CancelAsync(CancelMessage request)
        {
            return this.Channel.CancelAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_AddNodes)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the AddNodes service contract.
        /// </summary>
        public AddNodesResponseMessage AddNodes(AddNodesMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginAddNodes(request, null, null);
            }

            return this.Channel.EndAddNodes(result);
        }

        /// <summary>
        /// The client side implementation of the BeginAddNodes service contract.
        /// </summary>
        public IAsyncResult BeginAddNodes(AddNodesMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndAddNodes(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the AddNodes service contract.
        /// </summary>
        public Task<AddNodesResponseMessage> AddNodesAsync(AddNodesMessage request)
        {
            return this.Channel.AddNodesAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_AddReferences)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the AddReferences service contract.
        /// </summary>
        public AddReferencesResponseMessage AddReferences(AddReferencesMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginAddReferences(request, null, null);
            }

            return this.Channel.EndAddReferences(result);
        }

        /// <summary>
        /// The client side implementation of the BeginAddReferences service contract.
        /// </summary>
        public IAsyncResult BeginAddReferences(AddReferencesMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndAddReferences(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the AddReferences service contract.
        /// </summary>
        public Task<AddReferencesResponseMessage> AddReferencesAsync(AddReferencesMessage request)
        {
            return this.Channel.AddReferencesAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_DeleteNodes)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the DeleteNodes service contract.
        /// </summary>
        public DeleteNodesResponseMessage DeleteNodes(DeleteNodesMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginDeleteNodes(request, null, null);
            }

            return this.Channel.EndDeleteNodes(result);
        }

        /// <summary>
        /// The client side implementation of the BeginDeleteNodes service contract.
        /// </summary>
        public IAsyncResult BeginDeleteNodes(DeleteNodesMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndDeleteNodes(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the DeleteNodes service contract.
        /// </summary>
        public Task<DeleteNodesResponseMessage> DeleteNodesAsync(DeleteNodesMessage request)
        {
            return this.Channel.DeleteNodesAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_DeleteReferences)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the DeleteReferences service contract.
        /// </summary>
        public DeleteReferencesResponseMessage DeleteReferences(DeleteReferencesMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginDeleteReferences(request, null, null);
            }

            return this.Channel.EndDeleteReferences(result);
        }

        /// <summary>
        /// The client side implementation of the BeginDeleteReferences service contract.
        /// </summary>
        public IAsyncResult BeginDeleteReferences(DeleteReferencesMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndDeleteReferences(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the DeleteReferences service contract.
        /// </summary>
        public Task<DeleteReferencesResponseMessage> DeleteReferencesAsync(DeleteReferencesMessage request)
        {
            return this.Channel.DeleteReferencesAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_Browse)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the Browse service contract.
        /// </summary>
        public BrowseResponseMessage Browse(BrowseMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginBrowse(request, null, null);
            }

            return this.Channel.EndBrowse(result);
        }

        /// <summary>
        /// The client side implementation of the BeginBrowse service contract.
        /// </summary>
        public IAsyncResult BeginBrowse(BrowseMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndBrowse(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the Browse service contract.
        /// </summary>
        public Task<BrowseResponseMessage> BrowseAsync(BrowseMessage request)
        {
            return this.Channel.BrowseAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_BrowseNext)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the BrowseNext service contract.
        /// </summary>
        public BrowseNextResponseMessage BrowseNext(BrowseNextMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginBrowseNext(request, null, null);
            }

            return this.Channel.EndBrowseNext(result);
        }

        /// <summary>
        /// The client side implementation of the BeginBrowseNext service contract.
        /// </summary>
        public IAsyncResult BeginBrowseNext(BrowseNextMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndBrowseNext(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the BrowseNext service contract.
        /// </summary>
        public Task<BrowseNextResponseMessage> BrowseNextAsync(BrowseNextMessage request)
        {
            return this.Channel.BrowseNextAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_TranslateBrowsePathsToNodeIds)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the TranslateBrowsePathsToNodeIds service contract.
        /// </summary>
        public TranslateBrowsePathsToNodeIdsResponseMessage TranslateBrowsePathsToNodeIds(TranslateBrowsePathsToNodeIdsMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginTranslateBrowsePathsToNodeIds(request, null, null);
            }

            return this.Channel.EndTranslateBrowsePathsToNodeIds(result);
        }

        /// <summary>
        /// The client side implementation of the BeginTranslateBrowsePathsToNodeIds service contract.
        /// </summary>
        public IAsyncResult BeginTranslateBrowsePathsToNodeIds(TranslateBrowsePathsToNodeIdsMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndTranslateBrowsePathsToNodeIds(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the TranslateBrowsePathsToNodeIds service contract.
        /// </summary>
        public Task<TranslateBrowsePathsToNodeIdsResponseMessage> TranslateBrowsePathsToNodeIdsAsync(TranslateBrowsePathsToNodeIdsMessage request)
        {
            return this.Channel.TranslateBrowsePathsToNodeIdsAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_RegisterNodes)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the RegisterNodes service contract.
        /// </summary>
        public RegisterNodesResponseMessage RegisterNodes(RegisterNodesMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginRegisterNodes(request, null, null);
            }

            return this.Channel.EndRegisterNodes(result);
        }

        /// <summary>
        /// The client side implementation of the BeginRegisterNodes service contract.
        /// </summary>
        public IAsyncResult BeginRegisterNodes(RegisterNodesMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndRegisterNodes(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the RegisterNodes service contract.
        /// </summary>
        public Task<RegisterNodesResponseMessage> RegisterNodesAsync(RegisterNodesMessage request)
        {
            return this.Channel.RegisterNodesAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_UnregisterNodes)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the UnregisterNodes service contract.
        /// </summary>
        public UnregisterNodesResponseMessage UnregisterNodes(UnregisterNodesMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginUnregisterNodes(request, null, null);
            }

            return this.Channel.EndUnregisterNodes(result);
        }

        /// <summary>
        /// The client side implementation of the BeginUnregisterNodes service contract.
        /// </summary>
        public IAsyncResult BeginUnregisterNodes(UnregisterNodesMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndUnregisterNodes(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the UnregisterNodes service contract.
        /// </summary>
        public Task<UnregisterNodesResponseMessage> UnregisterNodesAsync(UnregisterNodesMessage request)
        {
            return this.Channel.UnregisterNodesAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_QueryFirst)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the QueryFirst service contract.
        /// </summary>
        public QueryFirstResponseMessage QueryFirst(QueryFirstMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginQueryFirst(request, null, null);
            }

            return this.Channel.EndQueryFirst(result);
        }

        /// <summary>
        /// The client side implementation of the BeginQueryFirst service contract.
        /// </summary>
        public IAsyncResult BeginQueryFirst(QueryFirstMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndQueryFirst(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the QueryFirst service contract.
        /// </summary>
        public Task<QueryFirstResponseMessage> QueryFirstAsync(QueryFirstMessage request)
        {
            return this.Channel.QueryFirstAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_QueryNext)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the QueryNext service contract.
        /// </summary>
        public QueryNextResponseMessage QueryNext(QueryNextMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginQueryNext(request, null, null);
            }

            return this.Channel.EndQueryNext(result);
        }

        /// <summary>
        /// The client side implementation of the BeginQueryNext service contract.
        /// </summary>
        public IAsyncResult BeginQueryNext(QueryNextMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndQueryNext(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the QueryNext service contract.
        /// </summary>
        public Task<QueryNextResponseMessage> QueryNextAsync(QueryNextMessage request)
        {
            return this.Channel.QueryNextAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_Read)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the Read service contract.
        /// </summary>
        public ReadResponseMessage Read(ReadMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginRead(request, null, null);
            }

            return this.Channel.EndRead(result);
        }

        /// <summary>
        /// The client side implementation of the BeginRead service contract.
        /// </summary>
        public IAsyncResult BeginRead(ReadMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndRead(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the Read service contract.
        /// </summary>
        public Task<ReadResponseMessage> ReadAsync(ReadMessage request)
        {
            return this.Channel.ReadAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_HistoryRead)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the HistoryRead service contract.
        /// </summary>
        public HistoryReadResponseMessage HistoryRead(HistoryReadMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginHistoryRead(request, null, null);
            }

            return this.Channel.EndHistoryRead(result);
        }

        /// <summary>
        /// The client side implementation of the BeginHistoryRead service contract.
        /// </summary>
        public IAsyncResult BeginHistoryRead(HistoryReadMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndHistoryRead(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the HistoryRead service contract.
        /// </summary>
        public Task<HistoryReadResponseMessage> HistoryReadAsync(HistoryReadMessage request)
        {
            return this.Channel.HistoryReadAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_Write)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the Write service contract.
        /// </summary>
        public WriteResponseMessage Write(WriteMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginWrite(request, null, null);
            }

            return this.Channel.EndWrite(result);
        }

        /// <summary>
        /// The client side implementation of the BeginWrite service contract.
        /// </summary>
        public IAsyncResult BeginWrite(WriteMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndWrite(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the Write service contract.
        /// </summary>
        public Task<WriteResponseMessage> WriteAsync(WriteMessage request)
        {
            return this.Channel.WriteAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_HistoryUpdate)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the HistoryUpdate service contract.
        /// </summary>
        public HistoryUpdateResponseMessage HistoryUpdate(HistoryUpdateMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginHistoryUpdate(request, null, null);
            }

            return this.Channel.EndHistoryUpdate(result);
        }

        /// <summary>
        /// The client side implementation of the BeginHistoryUpdate service contract.
        /// </summary>
        public IAsyncResult BeginHistoryUpdate(HistoryUpdateMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndHistoryUpdate(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the HistoryUpdate service contract.
        /// </summary>
        public Task<HistoryUpdateResponseMessage> HistoryUpdateAsync(HistoryUpdateMessage request)
        {
            return this.Channel.HistoryUpdateAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_Call)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the Call service contract.
        /// </summary>
        public CallResponseMessage Call(CallMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginCall(request, null, null);
            }

            return this.Channel.EndCall(result);
        }

        /// <summary>
        /// The client side implementation of the BeginCall service contract.
        /// </summary>
        public IAsyncResult BeginCall(CallMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndCall(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the Call service contract.
        /// </summary>
        public Task<CallResponseMessage> CallAsync(CallMessage request)
        {
            return this.Channel.CallAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_CreateMonitoredItems)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the CreateMonitoredItems service contract.
        /// </summary>
        public CreateMonitoredItemsResponseMessage CreateMonitoredItems(CreateMonitoredItemsMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginCreateMonitoredItems(request, null, null);
            }

            return this.Channel.EndCreateMonitoredItems(result);
        }

        /// <summary>
        /// The client side implementation of the BeginCreateMonitoredItems service contract.
        /// </summary>
        public IAsyncResult BeginCreateMonitoredItems(CreateMonitoredItemsMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndCreateMonitoredItems(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the CreateMonitoredItems service contract.
        /// </summary>
        public Task<CreateMonitoredItemsResponseMessage> CreateMonitoredItemsAsync(CreateMonitoredItemsMessage request)
        {
            return this.Channel.CreateMonitoredItemsAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_ModifyMonitoredItems)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the ModifyMonitoredItems service contract.
        /// </summary>
        public ModifyMonitoredItemsResponseMessage ModifyMonitoredItems(ModifyMonitoredItemsMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginModifyMonitoredItems(request, null, null);
            }

            return this.Channel.EndModifyMonitoredItems(result);
        }

        /// <summary>
        /// The client side implementation of the BeginModifyMonitoredItems service contract.
        /// </summary>
        public IAsyncResult BeginModifyMonitoredItems(ModifyMonitoredItemsMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndModifyMonitoredItems(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the ModifyMonitoredItems service contract.
        /// </summary>
        public Task<ModifyMonitoredItemsResponseMessage> ModifyMonitoredItemsAsync(ModifyMonitoredItemsMessage request)
        {
            return this.Channel.ModifyMonitoredItemsAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_SetMonitoringMode)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the SetMonitoringMode service contract.
        /// </summary>
        public SetMonitoringModeResponseMessage SetMonitoringMode(SetMonitoringModeMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginSetMonitoringMode(request, null, null);
            }

            return this.Channel.EndSetMonitoringMode(result);
        }

        /// <summary>
        /// The client side implementation of the BeginSetMonitoringMode service contract.
        /// </summary>
        public IAsyncResult BeginSetMonitoringMode(SetMonitoringModeMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndSetMonitoringMode(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the SetMonitoringMode service contract.
        /// </summary>
        public Task<SetMonitoringModeResponseMessage> SetMonitoringModeAsync(SetMonitoringModeMessage request)
        {
            return this.Channel.SetMonitoringModeAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_SetTriggering)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the SetTriggering service contract.
        /// </summary>
        public SetTriggeringResponseMessage SetTriggering(SetTriggeringMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginSetTriggering(request, null, null);
            }

            return this.Channel.EndSetTriggering(result);
        }

        /// <summary>
        /// The client side implementation of the BeginSetTriggering service contract.
        /// </summary>
        public IAsyncResult BeginSetTriggering(SetTriggeringMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndSetTriggering(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the SetTriggering service contract.
        /// </summary>
        public Task<SetTriggeringResponseMessage> SetTriggeringAsync(SetTriggeringMessage request)
        {
            return this.Channel.SetTriggeringAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_DeleteMonitoredItems)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the DeleteMonitoredItems service contract.
        /// </summary>
        public DeleteMonitoredItemsResponseMessage DeleteMonitoredItems(DeleteMonitoredItemsMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginDeleteMonitoredItems(request, null, null);
            }

            return this.Channel.EndDeleteMonitoredItems(result);
        }

        /// <summary>
        /// The client side implementation of the BeginDeleteMonitoredItems service contract.
        /// </summary>
        public IAsyncResult BeginDeleteMonitoredItems(DeleteMonitoredItemsMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndDeleteMonitoredItems(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the DeleteMonitoredItems service contract.
        /// </summary>
        public Task<DeleteMonitoredItemsResponseMessage> DeleteMonitoredItemsAsync(DeleteMonitoredItemsMessage request)
        {
            return this.Channel.DeleteMonitoredItemsAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_CreateSubscription)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the CreateSubscription service contract.
        /// </summary>
        public CreateSubscriptionResponseMessage CreateSubscription(CreateSubscriptionMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginCreateSubscription(request, null, null);
            }

            return this.Channel.EndCreateSubscription(result);
        }

        /// <summary>
        /// The client side implementation of the BeginCreateSubscription service contract.
        /// </summary>
        public IAsyncResult BeginCreateSubscription(CreateSubscriptionMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndCreateSubscription(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the CreateSubscription service contract.
        /// </summary>
        public Task<CreateSubscriptionResponseMessage> CreateSubscriptionAsync(CreateSubscriptionMessage request)
        {
            return this.Channel.CreateSubscriptionAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_ModifySubscription)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the ModifySubscription service contract.
        /// </summary>
        public ModifySubscriptionResponseMessage ModifySubscription(ModifySubscriptionMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginModifySubscription(request, null, null);
            }

            return this.Channel.EndModifySubscription(result);
        }

        /// <summary>
        /// The client side implementation of the BeginModifySubscription service contract.
        /// </summary>
        public IAsyncResult BeginModifySubscription(ModifySubscriptionMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndModifySubscription(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the ModifySubscription service contract.
        /// </summary>
        public Task<ModifySubscriptionResponseMessage> ModifySubscriptionAsync(ModifySubscriptionMessage request)
        {
            return this.Channel.ModifySubscriptionAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_SetPublishingMode)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the SetPublishingMode service contract.
        /// </summary>
        public SetPublishingModeResponseMessage SetPublishingMode(SetPublishingModeMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginSetPublishingMode(request, null, null);
            }

            return this.Channel.EndSetPublishingMode(result);
        }

        /// <summary>
        /// The client side implementation of the BeginSetPublishingMode service contract.
        /// </summary>
        public IAsyncResult BeginSetPublishingMode(SetPublishingModeMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndSetPublishingMode(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the SetPublishingMode service contract.
        /// </summary>
        public Task<SetPublishingModeResponseMessage> SetPublishingModeAsync(SetPublishingModeMessage request)
        {
            return this.Channel.SetPublishingModeAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_Publish)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the Publish service contract.
        /// </summary>
        public PublishResponseMessage Publish(PublishMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginPublish(request, null, null);
            }

            return this.Channel.EndPublish(result);
        }

        /// <summary>
        /// The client side implementation of the BeginPublish service contract.
        /// </summary>
        public IAsyncResult BeginPublish(PublishMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndPublish(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the Publish service contract.
        /// </summary>
        public Task<PublishResponseMessage> PublishAsync(PublishMessage request)
        {
            return this.Channel.PublishAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_Republish)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the Republish service contract.
        /// </summary>
        public RepublishResponseMessage Republish(RepublishMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginRepublish(request, null, null);
            }

            return this.Channel.EndRepublish(result);
        }

        /// <summary>
        /// The client side implementation of the BeginRepublish service contract.
        /// </summary>
        public IAsyncResult BeginRepublish(RepublishMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndRepublish(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the Republish service contract.
        /// </summary>
        public Task<RepublishResponseMessage> RepublishAsync(RepublishMessage request)
        {
            return this.Channel.RepublishAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_TransferSubscriptions)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the TransferSubscriptions service contract.
        /// </summary>
        public TransferSubscriptionsResponseMessage TransferSubscriptions(TransferSubscriptionsMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginTransferSubscriptions(request, null, null);
            }

            return this.Channel.EndTransferSubscriptions(result);
        }

        /// <summary>
        /// The client side implementation of the BeginTransferSubscriptions service contract.
        /// </summary>
        public IAsyncResult BeginTransferSubscriptions(TransferSubscriptionsMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndTransferSubscriptions(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the TransferSubscriptions service contract.
        /// </summary>
        public Task<TransferSubscriptionsResponseMessage> TransferSubscriptionsAsync(TransferSubscriptionsMessage request)
        {
            return this.Channel.TransferSubscriptionsAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_DeleteSubscriptions)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the DeleteSubscriptions service contract.
        /// </summary>
        public DeleteSubscriptionsResponseMessage DeleteSubscriptions(DeleteSubscriptionsMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginDeleteSubscriptions(request, null, null);
            }

            return this.Channel.EndDeleteSubscriptions(result);
        }

        /// <summary>
        /// The client side implementation of the BeginDeleteSubscriptions service contract.
        /// </summary>
        public IAsyncResult BeginDeleteSubscriptions(DeleteSubscriptionsMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndDeleteSubscriptions(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the DeleteSubscriptions service contract.
        /// </summary>
        public Task<DeleteSubscriptionsResponseMessage> DeleteSubscriptionsAsync(DeleteSubscriptionsMessage request)
        {
            return this.Channel.DeleteSubscriptionsAsync(request);
        }
        #endif
        #endif
    }
    #endregion

    #region DiscoveryChannel Class
    /// <summary>
    /// A channel object used by clients to access a UA service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    #if (!NET_STANDARD)
    public partial class DiscoveryChannel : WcfChannelBase<IDiscoveryChannel>, IDiscoveryChannel
    #else
    public partial class DiscoveryChannel : UaChannelBase<IDiscoveryChannel>, IDiscoveryChannel
    #endif
    {
        /// <summary>
        /// Initializes the object with the endpoint address.
        /// </summary>
        internal DiscoveryChannel()
        {
        }

        #if (!OPCUA_EXCLUDE_FindServers)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the FindServers service contract.
        /// </summary>
        public FindServersResponseMessage FindServers(FindServersMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginFindServers(request, null, null);
            }

            return this.Channel.EndFindServers(result);
        }

        /// <summary>
        /// The client side implementation of the BeginFindServers service contract.
        /// </summary>
        public IAsyncResult BeginFindServers(FindServersMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndFindServers(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the FindServers service contract.
        /// </summary>
        public Task<FindServersResponseMessage> FindServersAsync(FindServersMessage request)
        {
            return this.Channel.FindServersAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_FindServersOnNetwork)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the FindServersOnNetwork service contract.
        /// </summary>
        public FindServersOnNetworkResponseMessage FindServersOnNetwork(FindServersOnNetworkMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginFindServersOnNetwork(request, null, null);
            }

            return this.Channel.EndFindServersOnNetwork(result);
        }

        /// <summary>
        /// The client side implementation of the BeginFindServersOnNetwork service contract.
        /// </summary>
        public IAsyncResult BeginFindServersOnNetwork(FindServersOnNetworkMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndFindServersOnNetwork(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the FindServersOnNetwork service contract.
        /// </summary>
        public Task<FindServersOnNetworkResponseMessage> FindServersOnNetworkAsync(FindServersOnNetworkMessage request)
        {
            return this.Channel.FindServersOnNetworkAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_GetEndpoints)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the GetEndpoints service contract.
        /// </summary>
        public GetEndpointsResponseMessage GetEndpoints(GetEndpointsMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginGetEndpoints(request, null, null);
            }

            return this.Channel.EndGetEndpoints(result);
        }

        /// <summary>
        /// The client side implementation of the BeginGetEndpoints service contract.
        /// </summary>
        public IAsyncResult BeginGetEndpoints(GetEndpointsMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndGetEndpoints(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the GetEndpoints service contract.
        /// </summary>
        public Task<GetEndpointsResponseMessage> GetEndpointsAsync(GetEndpointsMessage request)
        {
            return this.Channel.GetEndpointsAsync(request);
        }
        #endif
        #endif
    }
    #endregion

    #region RegistrationChannel Class
    /// <summary>
    /// A channel object used by clients to access a UA service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    #if (!NET_STANDARD)
    public partial class RegistrationChannel : WcfChannelBase<IRegistrationChannel>, IRegistrationChannel
    #else
    public partial class RegistrationChannel : UaChannelBase<IRegistrationChannel>, IRegistrationChannel
    #endif
    {
        /// <summary>
        /// Initializes the object with the endpoint address.
        /// </summary>
        internal RegistrationChannel()
        {
        }

        #if (!OPCUA_EXCLUDE_RegisterServer)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the RegisterServer service contract.
        /// </summary>
        public RegisterServerResponseMessage RegisterServer(RegisterServerMessage request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginRegisterServer(request, null, null);
            }

            return this.Channel.EndRegisterServer(result);
        }

        /// <summary>
        /// The client side implementation of the BeginRegisterServer service contract.
        /// </summary>
        public IAsyncResult BeginRegisterServer(RegisterServerMessage request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndRegisterServer(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the RegisterServer service contract.
        /// </summary>
        public Task<RegisterServerResponseMessage> RegisterServerAsync(RegisterServerMessage request)
        {
            return this.Channel.RegisterServerAsync(request);
        }
        #endif
        #endif

        #if (!OPCUA_EXCLUDE_RegisterServer2)
        #if (!NET_STANDARD)
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
        #else  // NET_STANDARD
        /// <summary>
        /// The client side implementation of the RegisterServer2 service contract.
        /// </summary>
        public RegisterServer2ResponseMessage RegisterServer2(RegisterServer2Message request)
        {
            IAsyncResult result = null;

            lock (this.Channel)
            {
                result = this.Channel.BeginRegisterServer2(request, null, null);
            }

            return this.Channel.EndRegisterServer2(result);
        }

        /// <summary>
        /// The client side implementation of the BeginRegisterServer2 service contract.
        /// </summary>
        public IAsyncResult BeginRegisterServer2(RegisterServer2Message request, AsyncCallback callback, object asyncState)
        {
            UaChannelAsyncResult asyncResult = new UaChannelAsyncResult(Channel, callback, asyncState);

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
            UaChannelAsyncResult asyncResult = UaChannelAsyncResult.WaitForComplete(result);
            return asyncResult.Channel.EndRegisterServer2(asyncResult.InnerResult);
        }
        #endif

        #if (NET_STANDARD_ASYNC)
        /// <summary>
        /// The async client side implementation of the RegisterServer2 service contract.
        /// </summary>
        public Task<RegisterServer2ResponseMessage> RegisterServer2Async(RegisterServer2Message request)
        {
            return this.Channel.RegisterServer2Async(request);
        }
        #endif
        #endif
    }
    #endregion
}
