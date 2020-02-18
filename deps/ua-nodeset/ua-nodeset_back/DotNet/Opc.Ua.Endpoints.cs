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
using System.Threading;
using System.Security.Principal;
using System.ServiceModel;
using System.Runtime.Serialization;

namespace Opc.Ua
{
    #region SessionEndpoint Class
    /// <summary>
    /// A endpoint object used by clients to access a UA service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.CodeGenerator", "1.0.0.0")]
    #if !NET_STANDARD
    [ServiceMessageContextBehavior()]
    [ServiceBehavior(Namespace = Namespaces.OpcUaWsdl, InstanceContextMode=InstanceContextMode.PerSession, ConcurrencyMode=ConcurrencyMode.Multiple)]
    #endif
    public partial class SessionEndpoint : EndpointBase, ISessionEndpoint, IDiscoveryEndpoint
    {
        #region Constructors
        /// <summary>
        /// Initializes the object when it is created by the WCF framework.
        /// </summary>
        public SessionEndpoint()
        {
            this.CreateKnownTypes();
        }

        /// <summary>
        /// Initializes the when it is created directly.
        /// </summary>
        public SessionEndpoint(IServiceHostBase host) : base(host)
        {
            this.CreateKnownTypes();
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="SessionEndpoint"/> class.
        /// </summary>
        /// <param name="server">The server.</param>
        public SessionEndpoint(ServerBase server) : base(server)
        {
            this.CreateKnownTypes();
        }
        #endregion

        #region Public Members
        /// <summary>
        /// The UA server instance that the endpoint is connected to.
        /// </summary>
        protected ISessionServer ServerInstance
        {
            get
            {
                if (ServiceResult.IsBad(ServerError))
                {
                    throw new ServiceResultException(ServerError);
                }

                return ServerForContext as ISessionServer;
             }
        }
        #endregion

        #region ISessionEndpoint Members
        #region FindServers Service
        #if (!OPCUA_EXCLUDE_FindServers)
        /// <summary>
        /// Invokes the FindServers service.
        /// </summary>
        public IServiceResponse FindServers(IServiceRequest incoming)
        {
            FindServersResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                FindServersRequest request = (FindServersRequest)incoming;

                ApplicationDescriptionCollection servers = null;

                response = new FindServersResponse();

                response.ResponseHeader = ServerInstance.FindServers(
                   request.RequestHeader,
                   request.EndpointUrl,
                   request.LocaleIds,
                   request.ServerUris,
                   out servers);

                response.Servers = servers;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the FindServers service.
        /// </summary>
        public virtual FindServersResponseMessage FindServers(FindServersMessage request)
        {
            FindServersResponse response = null;

            try
            {
                // OnRequestReceived(message.FindServersRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (FindServersResponse)FindServers(request.FindServersRequest);
                // OnResponseSent(response);
                return new FindServersResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.FindServersRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the FindServers service.
        /// </summary>
        public virtual IAsyncResult BeginFindServers(FindServersMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.FindServersRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.FindServersRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.FindServersRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the FindServers service to complete.
        /// </summary>
        public virtual FindServersResponseMessage EndFindServers(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new FindServersResponseMessage((FindServersResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region FindServersOnNetwork Service
        #if (!OPCUA_EXCLUDE_FindServersOnNetwork)
        /// <summary>
        /// Invokes the FindServersOnNetwork service.
        /// </summary>
        public IServiceResponse FindServersOnNetwork(IServiceRequest incoming)
        {
            FindServersOnNetworkResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                FindServersOnNetworkRequest request = (FindServersOnNetworkRequest)incoming;

                DateTime lastCounterResetTime = DateTime.MinValue;
                ServerOnNetworkCollection servers = null;

                response = new FindServersOnNetworkResponse();

                response.ResponseHeader = ServerInstance.FindServersOnNetwork(
                   request.RequestHeader,
                   request.StartingRecordId,
                   request.MaxRecordsToReturn,
                   request.ServerCapabilityFilter,
                   out lastCounterResetTime,
                   out servers);

                response.LastCounterResetTime = lastCounterResetTime;
                response.Servers              = servers;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the FindServersOnNetwork service.
        /// </summary>
        public virtual FindServersOnNetworkResponseMessage FindServersOnNetwork(FindServersOnNetworkMessage request)
        {
            FindServersOnNetworkResponse response = null;

            try
            {
                // OnRequestReceived(message.FindServersOnNetworkRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (FindServersOnNetworkResponse)FindServersOnNetwork(request.FindServersOnNetworkRequest);
                // OnResponseSent(response);
                return new FindServersOnNetworkResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.FindServersOnNetworkRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the FindServersOnNetwork service.
        /// </summary>
        public virtual IAsyncResult BeginFindServersOnNetwork(FindServersOnNetworkMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.FindServersOnNetworkRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.FindServersOnNetworkRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.FindServersOnNetworkRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the FindServersOnNetwork service to complete.
        /// </summary>
        public virtual FindServersOnNetworkResponseMessage EndFindServersOnNetwork(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new FindServersOnNetworkResponseMessage((FindServersOnNetworkResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region GetEndpoints Service
        #if (!OPCUA_EXCLUDE_GetEndpoints)
        /// <summary>
        /// Invokes the GetEndpoints service.
        /// </summary>
        public IServiceResponse GetEndpoints(IServiceRequest incoming)
        {
            GetEndpointsResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                GetEndpointsRequest request = (GetEndpointsRequest)incoming;

                EndpointDescriptionCollection endpoints = null;

                response = new GetEndpointsResponse();

                response.ResponseHeader = ServerInstance.GetEndpoints(
                   request.RequestHeader,
                   request.EndpointUrl,
                   request.LocaleIds,
                   request.ProfileUris,
                   out endpoints);

                response.Endpoints = endpoints;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the GetEndpoints service.
        /// </summary>
        public virtual GetEndpointsResponseMessage GetEndpoints(GetEndpointsMessage request)
        {
            GetEndpointsResponse response = null;

            try
            {
                // OnRequestReceived(message.GetEndpointsRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (GetEndpointsResponse)GetEndpoints(request.GetEndpointsRequest);
                // OnResponseSent(response);
                return new GetEndpointsResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.GetEndpointsRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the GetEndpoints service.
        /// </summary>
        public virtual IAsyncResult BeginGetEndpoints(GetEndpointsMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.GetEndpointsRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.GetEndpointsRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.GetEndpointsRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the GetEndpoints service to complete.
        /// </summary>
        public virtual GetEndpointsResponseMessage EndGetEndpoints(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new GetEndpointsResponseMessage((GetEndpointsResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region CreateSession Service
        #if (!OPCUA_EXCLUDE_CreateSession)
        /// <summary>
        /// Invokes the CreateSession service.
        /// </summary>
        public IServiceResponse CreateSession(IServiceRequest incoming)
        {
            CreateSessionResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                CreateSessionRequest request = (CreateSessionRequest)incoming;

                NodeId sessionId = null;
                NodeId authenticationToken = null;
                double revisedSessionTimeout = 0;
                byte[] serverNonce = null;
                byte[] serverCertificate = null;
                EndpointDescriptionCollection serverEndpoints = null;
                SignedSoftwareCertificateCollection serverSoftwareCertificates = null;
                SignatureData serverSignature = null;
                uint maxRequestMessageSize = 0;

                response = new CreateSessionResponse();

                response.ResponseHeader = ServerInstance.CreateSession(
                   request.RequestHeader,
                   request.ClientDescription,
                   request.ServerUri,
                   request.EndpointUrl,
                   request.SessionName,
                   request.ClientNonce,
                   request.ClientCertificate,
                   request.RequestedSessionTimeout,
                   request.MaxResponseMessageSize,
                   out sessionId,
                   out authenticationToken,
                   out revisedSessionTimeout,
                   out serverNonce,
                   out serverCertificate,
                   out serverEndpoints,
                   out serverSoftwareCertificates,
                   out serverSignature,
                   out maxRequestMessageSize);

                response.SessionId                  = sessionId;
                response.AuthenticationToken        = authenticationToken;
                response.RevisedSessionTimeout      = revisedSessionTimeout;
                response.ServerNonce                = serverNonce;
                response.ServerCertificate          = serverCertificate;
                response.ServerEndpoints            = serverEndpoints;
                response.ServerSoftwareCertificates = serverSoftwareCertificates;
                response.ServerSignature            = serverSignature;
                response.MaxRequestMessageSize      = maxRequestMessageSize;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the CreateSession service.
        /// </summary>
        public virtual CreateSessionResponseMessage CreateSession(CreateSessionMessage request)
        {
            CreateSessionResponse response = null;

            try
            {
                // OnRequestReceived(message.CreateSessionRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (CreateSessionResponse)CreateSession(request.CreateSessionRequest);
                // OnResponseSent(response);
                return new CreateSessionResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.CreateSessionRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the CreateSession service.
        /// </summary>
        public virtual IAsyncResult BeginCreateSession(CreateSessionMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.CreateSessionRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.CreateSessionRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.CreateSessionRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the CreateSession service to complete.
        /// </summary>
        public virtual CreateSessionResponseMessage EndCreateSession(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new CreateSessionResponseMessage((CreateSessionResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region ActivateSession Service
        #if (!OPCUA_EXCLUDE_ActivateSession)
        /// <summary>
        /// Invokes the ActivateSession service.
        /// </summary>
        public IServiceResponse ActivateSession(IServiceRequest incoming)
        {
            ActivateSessionResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                ActivateSessionRequest request = (ActivateSessionRequest)incoming;

                byte[] serverNonce = null;
                StatusCodeCollection results = null;
                DiagnosticInfoCollection diagnosticInfos = null;

                response = new ActivateSessionResponse();

                response.ResponseHeader = ServerInstance.ActivateSession(
                   request.RequestHeader,
                   request.ClientSignature,
                   request.ClientSoftwareCertificates,
                   request.LocaleIds,
                   request.UserIdentityToken,
                   request.UserTokenSignature,
                   out serverNonce,
                   out results,
                   out diagnosticInfos);

                response.ServerNonce     = serverNonce;
                response.Results         = results;
                response.DiagnosticInfos = diagnosticInfos;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the ActivateSession service.
        /// </summary>
        public virtual ActivateSessionResponseMessage ActivateSession(ActivateSessionMessage request)
        {
            ActivateSessionResponse response = null;

            try
            {
                // OnRequestReceived(message.ActivateSessionRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (ActivateSessionResponse)ActivateSession(request.ActivateSessionRequest);
                // OnResponseSent(response);
                return new ActivateSessionResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.ActivateSessionRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the ActivateSession service.
        /// </summary>
        public virtual IAsyncResult BeginActivateSession(ActivateSessionMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.ActivateSessionRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.ActivateSessionRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.ActivateSessionRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the ActivateSession service to complete.
        /// </summary>
        public virtual ActivateSessionResponseMessage EndActivateSession(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new ActivateSessionResponseMessage((ActivateSessionResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region CloseSession Service
        #if (!OPCUA_EXCLUDE_CloseSession)
        /// <summary>
        /// Invokes the CloseSession service.
        /// </summary>
        public IServiceResponse CloseSession(IServiceRequest incoming)
        {
            CloseSessionResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                CloseSessionRequest request = (CloseSessionRequest)incoming;


                response = new CloseSessionResponse();

                response.ResponseHeader = ServerInstance.CloseSession(
                   request.RequestHeader,
                   request.DeleteSubscriptions);

            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the CloseSession service.
        /// </summary>
        public virtual CloseSessionResponseMessage CloseSession(CloseSessionMessage request)
        {
            CloseSessionResponse response = null;

            try
            {
                // OnRequestReceived(message.CloseSessionRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (CloseSessionResponse)CloseSession(request.CloseSessionRequest);
                // OnResponseSent(response);
                return new CloseSessionResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.CloseSessionRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the CloseSession service.
        /// </summary>
        public virtual IAsyncResult BeginCloseSession(CloseSessionMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.CloseSessionRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.CloseSessionRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.CloseSessionRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the CloseSession service to complete.
        /// </summary>
        public virtual CloseSessionResponseMessage EndCloseSession(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new CloseSessionResponseMessage((CloseSessionResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region Cancel Service
        #if (!OPCUA_EXCLUDE_Cancel)
        /// <summary>
        /// Invokes the Cancel service.
        /// </summary>
        public IServiceResponse Cancel(IServiceRequest incoming)
        {
            CancelResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                CancelRequest request = (CancelRequest)incoming;

                uint cancelCount = 0;

                response = new CancelResponse();

                response.ResponseHeader = ServerInstance.Cancel(
                   request.RequestHeader,
                   request.RequestHandle,
                   out cancelCount);

                response.CancelCount = cancelCount;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the Cancel service.
        /// </summary>
        public virtual CancelResponseMessage Cancel(CancelMessage request)
        {
            CancelResponse response = null;

            try
            {
                // OnRequestReceived(message.CancelRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (CancelResponse)Cancel(request.CancelRequest);
                // OnResponseSent(response);
                return new CancelResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.CancelRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the Cancel service.
        /// </summary>
        public virtual IAsyncResult BeginCancel(CancelMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.CancelRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.CancelRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.CancelRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the Cancel service to complete.
        /// </summary>
        public virtual CancelResponseMessage EndCancel(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new CancelResponseMessage((CancelResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region AddNodes Service
        #if (!OPCUA_EXCLUDE_AddNodes)
        /// <summary>
        /// Invokes the AddNodes service.
        /// </summary>
        public IServiceResponse AddNodes(IServiceRequest incoming)
        {
            AddNodesResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                AddNodesRequest request = (AddNodesRequest)incoming;

                AddNodesResultCollection results = null;
                DiagnosticInfoCollection diagnosticInfos = null;

                response = new AddNodesResponse();

                response.ResponseHeader = ServerInstance.AddNodes(
                   request.RequestHeader,
                   request.NodesToAdd,
                   out results,
                   out diagnosticInfos);

                response.Results         = results;
                response.DiagnosticInfos = diagnosticInfos;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the AddNodes service.
        /// </summary>
        public virtual AddNodesResponseMessage AddNodes(AddNodesMessage request)
        {
            AddNodesResponse response = null;

            try
            {
                // OnRequestReceived(message.AddNodesRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (AddNodesResponse)AddNodes(request.AddNodesRequest);
                // OnResponseSent(response);
                return new AddNodesResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.AddNodesRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the AddNodes service.
        /// </summary>
        public virtual IAsyncResult BeginAddNodes(AddNodesMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.AddNodesRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.AddNodesRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.AddNodesRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the AddNodes service to complete.
        /// </summary>
        public virtual AddNodesResponseMessage EndAddNodes(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new AddNodesResponseMessage((AddNodesResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region AddReferences Service
        #if (!OPCUA_EXCLUDE_AddReferences)
        /// <summary>
        /// Invokes the AddReferences service.
        /// </summary>
        public IServiceResponse AddReferences(IServiceRequest incoming)
        {
            AddReferencesResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                AddReferencesRequest request = (AddReferencesRequest)incoming;

                StatusCodeCollection results = null;
                DiagnosticInfoCollection diagnosticInfos = null;

                response = new AddReferencesResponse();

                response.ResponseHeader = ServerInstance.AddReferences(
                   request.RequestHeader,
                   request.ReferencesToAdd,
                   out results,
                   out diagnosticInfos);

                response.Results         = results;
                response.DiagnosticInfos = diagnosticInfos;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the AddReferences service.
        /// </summary>
        public virtual AddReferencesResponseMessage AddReferences(AddReferencesMessage request)
        {
            AddReferencesResponse response = null;

            try
            {
                // OnRequestReceived(message.AddReferencesRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (AddReferencesResponse)AddReferences(request.AddReferencesRequest);
                // OnResponseSent(response);
                return new AddReferencesResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.AddReferencesRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the AddReferences service.
        /// </summary>
        public virtual IAsyncResult BeginAddReferences(AddReferencesMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.AddReferencesRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.AddReferencesRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.AddReferencesRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the AddReferences service to complete.
        /// </summary>
        public virtual AddReferencesResponseMessage EndAddReferences(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new AddReferencesResponseMessage((AddReferencesResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region DeleteNodes Service
        #if (!OPCUA_EXCLUDE_DeleteNodes)
        /// <summary>
        /// Invokes the DeleteNodes service.
        /// </summary>
        public IServiceResponse DeleteNodes(IServiceRequest incoming)
        {
            DeleteNodesResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                DeleteNodesRequest request = (DeleteNodesRequest)incoming;

                StatusCodeCollection results = null;
                DiagnosticInfoCollection diagnosticInfos = null;

                response = new DeleteNodesResponse();

                response.ResponseHeader = ServerInstance.DeleteNodes(
                   request.RequestHeader,
                   request.NodesToDelete,
                   out results,
                   out diagnosticInfos);

                response.Results         = results;
                response.DiagnosticInfos = diagnosticInfos;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the DeleteNodes service.
        /// </summary>
        public virtual DeleteNodesResponseMessage DeleteNodes(DeleteNodesMessage request)
        {
            DeleteNodesResponse response = null;

            try
            {
                // OnRequestReceived(message.DeleteNodesRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (DeleteNodesResponse)DeleteNodes(request.DeleteNodesRequest);
                // OnResponseSent(response);
                return new DeleteNodesResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.DeleteNodesRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the DeleteNodes service.
        /// </summary>
        public virtual IAsyncResult BeginDeleteNodes(DeleteNodesMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.DeleteNodesRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.DeleteNodesRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.DeleteNodesRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the DeleteNodes service to complete.
        /// </summary>
        public virtual DeleteNodesResponseMessage EndDeleteNodes(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new DeleteNodesResponseMessage((DeleteNodesResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region DeleteReferences Service
        #if (!OPCUA_EXCLUDE_DeleteReferences)
        /// <summary>
        /// Invokes the DeleteReferences service.
        /// </summary>
        public IServiceResponse DeleteReferences(IServiceRequest incoming)
        {
            DeleteReferencesResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                DeleteReferencesRequest request = (DeleteReferencesRequest)incoming;

                StatusCodeCollection results = null;
                DiagnosticInfoCollection diagnosticInfos = null;

                response = new DeleteReferencesResponse();

                response.ResponseHeader = ServerInstance.DeleteReferences(
                   request.RequestHeader,
                   request.ReferencesToDelete,
                   out results,
                   out diagnosticInfos);

                response.Results         = results;
                response.DiagnosticInfos = diagnosticInfos;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the DeleteReferences service.
        /// </summary>
        public virtual DeleteReferencesResponseMessage DeleteReferences(DeleteReferencesMessage request)
        {
            DeleteReferencesResponse response = null;

            try
            {
                // OnRequestReceived(message.DeleteReferencesRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (DeleteReferencesResponse)DeleteReferences(request.DeleteReferencesRequest);
                // OnResponseSent(response);
                return new DeleteReferencesResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.DeleteReferencesRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the DeleteReferences service.
        /// </summary>
        public virtual IAsyncResult BeginDeleteReferences(DeleteReferencesMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.DeleteReferencesRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.DeleteReferencesRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.DeleteReferencesRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the DeleteReferences service to complete.
        /// </summary>
        public virtual DeleteReferencesResponseMessage EndDeleteReferences(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new DeleteReferencesResponseMessage((DeleteReferencesResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region Browse Service
        #if (!OPCUA_EXCLUDE_Browse)
        /// <summary>
        /// Invokes the Browse service.
        /// </summary>
        public IServiceResponse Browse(IServiceRequest incoming)
        {
            BrowseResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                BrowseRequest request = (BrowseRequest)incoming;

                BrowseResultCollection results = null;
                DiagnosticInfoCollection diagnosticInfos = null;

                response = new BrowseResponse();

                response.ResponseHeader = ServerInstance.Browse(
                   request.RequestHeader,
                   request.View,
                   request.RequestedMaxReferencesPerNode,
                   request.NodesToBrowse,
                   out results,
                   out diagnosticInfos);

                response.Results         = results;
                response.DiagnosticInfos = diagnosticInfos;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the Browse service.
        /// </summary>
        public virtual BrowseResponseMessage Browse(BrowseMessage request)
        {
            BrowseResponse response = null;

            try
            {
                // OnRequestReceived(message.BrowseRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (BrowseResponse)Browse(request.BrowseRequest);
                // OnResponseSent(response);
                return new BrowseResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.BrowseRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the Browse service.
        /// </summary>
        public virtual IAsyncResult BeginBrowse(BrowseMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.BrowseRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.BrowseRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.BrowseRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the Browse service to complete.
        /// </summary>
        public virtual BrowseResponseMessage EndBrowse(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new BrowseResponseMessage((BrowseResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region BrowseNext Service
        #if (!OPCUA_EXCLUDE_BrowseNext)
        /// <summary>
        /// Invokes the BrowseNext service.
        /// </summary>
        public IServiceResponse BrowseNext(IServiceRequest incoming)
        {
            BrowseNextResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                BrowseNextRequest request = (BrowseNextRequest)incoming;

                BrowseResultCollection results = null;
                DiagnosticInfoCollection diagnosticInfos = null;

                response = new BrowseNextResponse();

                response.ResponseHeader = ServerInstance.BrowseNext(
                   request.RequestHeader,
                   request.ReleaseContinuationPoints,
                   request.ContinuationPoints,
                   out results,
                   out diagnosticInfos);

                response.Results         = results;
                response.DiagnosticInfos = diagnosticInfos;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the BrowseNext service.
        /// </summary>
        public virtual BrowseNextResponseMessage BrowseNext(BrowseNextMessage request)
        {
            BrowseNextResponse response = null;

            try
            {
                // OnRequestReceived(message.BrowseNextRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (BrowseNextResponse)BrowseNext(request.BrowseNextRequest);
                // OnResponseSent(response);
                return new BrowseNextResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.BrowseNextRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the BrowseNext service.
        /// </summary>
        public virtual IAsyncResult BeginBrowseNext(BrowseNextMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.BrowseNextRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.BrowseNextRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.BrowseNextRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the BrowseNext service to complete.
        /// </summary>
        public virtual BrowseNextResponseMessage EndBrowseNext(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new BrowseNextResponseMessage((BrowseNextResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region TranslateBrowsePathsToNodeIds Service
        #if (!OPCUA_EXCLUDE_TranslateBrowsePathsToNodeIds)
        /// <summary>
        /// Invokes the TranslateBrowsePathsToNodeIds service.
        /// </summary>
        public IServiceResponse TranslateBrowsePathsToNodeIds(IServiceRequest incoming)
        {
            TranslateBrowsePathsToNodeIdsResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                TranslateBrowsePathsToNodeIdsRequest request = (TranslateBrowsePathsToNodeIdsRequest)incoming;

                BrowsePathResultCollection results = null;
                DiagnosticInfoCollection diagnosticInfos = null;

                response = new TranslateBrowsePathsToNodeIdsResponse();

                response.ResponseHeader = ServerInstance.TranslateBrowsePathsToNodeIds(
                   request.RequestHeader,
                   request.BrowsePaths,
                   out results,
                   out diagnosticInfos);

                response.Results         = results;
                response.DiagnosticInfos = diagnosticInfos;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the TranslateBrowsePathsToNodeIds service.
        /// </summary>
        public virtual TranslateBrowsePathsToNodeIdsResponseMessage TranslateBrowsePathsToNodeIds(TranslateBrowsePathsToNodeIdsMessage request)
        {
            TranslateBrowsePathsToNodeIdsResponse response = null;

            try
            {
                // OnRequestReceived(message.TranslateBrowsePathsToNodeIdsRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (TranslateBrowsePathsToNodeIdsResponse)TranslateBrowsePathsToNodeIds(request.TranslateBrowsePathsToNodeIdsRequest);
                // OnResponseSent(response);
                return new TranslateBrowsePathsToNodeIdsResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.TranslateBrowsePathsToNodeIdsRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the TranslateBrowsePathsToNodeIds service.
        /// </summary>
        public virtual IAsyncResult BeginTranslateBrowsePathsToNodeIds(TranslateBrowsePathsToNodeIdsMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.TranslateBrowsePathsToNodeIdsRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.TranslateBrowsePathsToNodeIdsRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.TranslateBrowsePathsToNodeIdsRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the TranslateBrowsePathsToNodeIds service to complete.
        /// </summary>
        public virtual TranslateBrowsePathsToNodeIdsResponseMessage EndTranslateBrowsePathsToNodeIds(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new TranslateBrowsePathsToNodeIdsResponseMessage((TranslateBrowsePathsToNodeIdsResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region RegisterNodes Service
        #if (!OPCUA_EXCLUDE_RegisterNodes)
        /// <summary>
        /// Invokes the RegisterNodes service.
        /// </summary>
        public IServiceResponse RegisterNodes(IServiceRequest incoming)
        {
            RegisterNodesResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                RegisterNodesRequest request = (RegisterNodesRequest)incoming;

                NodeIdCollection registeredNodeIds = null;

                response = new RegisterNodesResponse();

                response.ResponseHeader = ServerInstance.RegisterNodes(
                   request.RequestHeader,
                   request.NodesToRegister,
                   out registeredNodeIds);

                response.RegisteredNodeIds = registeredNodeIds;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the RegisterNodes service.
        /// </summary>
        public virtual RegisterNodesResponseMessage RegisterNodes(RegisterNodesMessage request)
        {
            RegisterNodesResponse response = null;

            try
            {
                // OnRequestReceived(message.RegisterNodesRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (RegisterNodesResponse)RegisterNodes(request.RegisterNodesRequest);
                // OnResponseSent(response);
                return new RegisterNodesResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.RegisterNodesRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the RegisterNodes service.
        /// </summary>
        public virtual IAsyncResult BeginRegisterNodes(RegisterNodesMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.RegisterNodesRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.RegisterNodesRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.RegisterNodesRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the RegisterNodes service to complete.
        /// </summary>
        public virtual RegisterNodesResponseMessage EndRegisterNodes(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new RegisterNodesResponseMessage((RegisterNodesResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region UnregisterNodes Service
        #if (!OPCUA_EXCLUDE_UnregisterNodes)
        /// <summary>
        /// Invokes the UnregisterNodes service.
        /// </summary>
        public IServiceResponse UnregisterNodes(IServiceRequest incoming)
        {
            UnregisterNodesResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                UnregisterNodesRequest request = (UnregisterNodesRequest)incoming;


                response = new UnregisterNodesResponse();

                response.ResponseHeader = ServerInstance.UnregisterNodes(
                   request.RequestHeader,
                   request.NodesToUnregister);

            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the UnregisterNodes service.
        /// </summary>
        public virtual UnregisterNodesResponseMessage UnregisterNodes(UnregisterNodesMessage request)
        {
            UnregisterNodesResponse response = null;

            try
            {
                // OnRequestReceived(message.UnregisterNodesRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (UnregisterNodesResponse)UnregisterNodes(request.UnregisterNodesRequest);
                // OnResponseSent(response);
                return new UnregisterNodesResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.UnregisterNodesRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the UnregisterNodes service.
        /// </summary>
        public virtual IAsyncResult BeginUnregisterNodes(UnregisterNodesMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.UnregisterNodesRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.UnregisterNodesRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.UnregisterNodesRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the UnregisterNodes service to complete.
        /// </summary>
        public virtual UnregisterNodesResponseMessage EndUnregisterNodes(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new UnregisterNodesResponseMessage((UnregisterNodesResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region QueryFirst Service
        #if (!OPCUA_EXCLUDE_QueryFirst)
        /// <summary>
        /// Invokes the QueryFirst service.
        /// </summary>
        public IServiceResponse QueryFirst(IServiceRequest incoming)
        {
            QueryFirstResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                QueryFirstRequest request = (QueryFirstRequest)incoming;

                QueryDataSetCollection queryDataSets = null;
                byte[] continuationPoint = null;
                ParsingResultCollection parsingResults = null;
                DiagnosticInfoCollection diagnosticInfos = null;
                ContentFilterResult filterResult = null;

                response = new QueryFirstResponse();

                response.ResponseHeader = ServerInstance.QueryFirst(
                   request.RequestHeader,
                   request.View,
                   request.NodeTypes,
                   request.Filter,
                   request.MaxDataSetsToReturn,
                   request.MaxReferencesToReturn,
                   out queryDataSets,
                   out continuationPoint,
                   out parsingResults,
                   out diagnosticInfos,
                   out filterResult);

                response.QueryDataSets     = queryDataSets;
                response.ContinuationPoint = continuationPoint;
                response.ParsingResults    = parsingResults;
                response.DiagnosticInfos   = diagnosticInfos;
                response.FilterResult      = filterResult;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the QueryFirst service.
        /// </summary>
        public virtual QueryFirstResponseMessage QueryFirst(QueryFirstMessage request)
        {
            QueryFirstResponse response = null;

            try
            {
                // OnRequestReceived(message.QueryFirstRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (QueryFirstResponse)QueryFirst(request.QueryFirstRequest);
                // OnResponseSent(response);
                return new QueryFirstResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.QueryFirstRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the QueryFirst service.
        /// </summary>
        public virtual IAsyncResult BeginQueryFirst(QueryFirstMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.QueryFirstRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.QueryFirstRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.QueryFirstRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the QueryFirst service to complete.
        /// </summary>
        public virtual QueryFirstResponseMessage EndQueryFirst(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new QueryFirstResponseMessage((QueryFirstResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region QueryNext Service
        #if (!OPCUA_EXCLUDE_QueryNext)
        /// <summary>
        /// Invokes the QueryNext service.
        /// </summary>
        public IServiceResponse QueryNext(IServiceRequest incoming)
        {
            QueryNextResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                QueryNextRequest request = (QueryNextRequest)incoming;

                QueryDataSetCollection queryDataSets = null;
                byte[] revisedContinuationPoint = null;

                response = new QueryNextResponse();

                response.ResponseHeader = ServerInstance.QueryNext(
                   request.RequestHeader,
                   request.ReleaseContinuationPoint,
                   request.ContinuationPoint,
                   out queryDataSets,
                   out revisedContinuationPoint);

                response.QueryDataSets            = queryDataSets;
                response.RevisedContinuationPoint = revisedContinuationPoint;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the QueryNext service.
        /// </summary>
        public virtual QueryNextResponseMessage QueryNext(QueryNextMessage request)
        {
            QueryNextResponse response = null;

            try
            {
                // OnRequestReceived(message.QueryNextRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (QueryNextResponse)QueryNext(request.QueryNextRequest);
                // OnResponseSent(response);
                return new QueryNextResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.QueryNextRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the QueryNext service.
        /// </summary>
        public virtual IAsyncResult BeginQueryNext(QueryNextMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.QueryNextRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.QueryNextRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.QueryNextRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the QueryNext service to complete.
        /// </summary>
        public virtual QueryNextResponseMessage EndQueryNext(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new QueryNextResponseMessage((QueryNextResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region Read Service
        #if (!OPCUA_EXCLUDE_Read)
        /// <summary>
        /// Invokes the Read service.
        /// </summary>
        public IServiceResponse Read(IServiceRequest incoming)
        {
            ReadResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                ReadRequest request = (ReadRequest)incoming;

                DataValueCollection results = null;
                DiagnosticInfoCollection diagnosticInfos = null;

                response = new ReadResponse();

                response.ResponseHeader = ServerInstance.Read(
                   request.RequestHeader,
                   request.MaxAge,
                   request.TimestampsToReturn,
                   request.NodesToRead,
                   out results,
                   out diagnosticInfos);

                response.Results         = results;
                response.DiagnosticInfos = diagnosticInfos;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the Read service.
        /// </summary>
        public virtual ReadResponseMessage Read(ReadMessage request)
        {
            ReadResponse response = null;

            try
            {
                // OnRequestReceived(message.ReadRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (ReadResponse)Read(request.ReadRequest);
                // OnResponseSent(response);
                return new ReadResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.ReadRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the Read service.
        /// </summary>
        public virtual IAsyncResult BeginRead(ReadMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.ReadRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.ReadRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.ReadRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the Read service to complete.
        /// </summary>
        public virtual ReadResponseMessage EndRead(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new ReadResponseMessage((ReadResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region HistoryRead Service
        #if (!OPCUA_EXCLUDE_HistoryRead)
        /// <summary>
        /// Invokes the HistoryRead service.
        /// </summary>
        public IServiceResponse HistoryRead(IServiceRequest incoming)
        {
            HistoryReadResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                HistoryReadRequest request = (HistoryReadRequest)incoming;

                HistoryReadResultCollection results = null;
                DiagnosticInfoCollection diagnosticInfos = null;

                response = new HistoryReadResponse();

                response.ResponseHeader = ServerInstance.HistoryRead(
                   request.RequestHeader,
                   request.HistoryReadDetails,
                   request.TimestampsToReturn,
                   request.ReleaseContinuationPoints,
                   request.NodesToRead,
                   out results,
                   out diagnosticInfos);

                response.Results         = results;
                response.DiagnosticInfos = diagnosticInfos;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the HistoryRead service.
        /// </summary>
        public virtual HistoryReadResponseMessage HistoryRead(HistoryReadMessage request)
        {
            HistoryReadResponse response = null;

            try
            {
                // OnRequestReceived(message.HistoryReadRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (HistoryReadResponse)HistoryRead(request.HistoryReadRequest);
                // OnResponseSent(response);
                return new HistoryReadResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.HistoryReadRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the HistoryRead service.
        /// </summary>
        public virtual IAsyncResult BeginHistoryRead(HistoryReadMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.HistoryReadRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.HistoryReadRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.HistoryReadRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the HistoryRead service to complete.
        /// </summary>
        public virtual HistoryReadResponseMessage EndHistoryRead(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new HistoryReadResponseMessage((HistoryReadResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region Write Service
        #if (!OPCUA_EXCLUDE_Write)
        /// <summary>
        /// Invokes the Write service.
        /// </summary>
        public IServiceResponse Write(IServiceRequest incoming)
        {
            WriteResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                WriteRequest request = (WriteRequest)incoming;

                StatusCodeCollection results = null;
                DiagnosticInfoCollection diagnosticInfos = null;

                response = new WriteResponse();

                response.ResponseHeader = ServerInstance.Write(
                   request.RequestHeader,
                   request.NodesToWrite,
                   out results,
                   out diagnosticInfos);

                response.Results         = results;
                response.DiagnosticInfos = diagnosticInfos;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the Write service.
        /// </summary>
        public virtual WriteResponseMessage Write(WriteMessage request)
        {
            WriteResponse response = null;

            try
            {
                // OnRequestReceived(message.WriteRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (WriteResponse)Write(request.WriteRequest);
                // OnResponseSent(response);
                return new WriteResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.WriteRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the Write service.
        /// </summary>
        public virtual IAsyncResult BeginWrite(WriteMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.WriteRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.WriteRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.WriteRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the Write service to complete.
        /// </summary>
        public virtual WriteResponseMessage EndWrite(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new WriteResponseMessage((WriteResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region HistoryUpdate Service
        #if (!OPCUA_EXCLUDE_HistoryUpdate)
        /// <summary>
        /// Invokes the HistoryUpdate service.
        /// </summary>
        public IServiceResponse HistoryUpdate(IServiceRequest incoming)
        {
            HistoryUpdateResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                HistoryUpdateRequest request = (HistoryUpdateRequest)incoming;

                HistoryUpdateResultCollection results = null;
                DiagnosticInfoCollection diagnosticInfos = null;

                response = new HistoryUpdateResponse();

                response.ResponseHeader = ServerInstance.HistoryUpdate(
                   request.RequestHeader,
                   request.HistoryUpdateDetails,
                   out results,
                   out diagnosticInfos);

                response.Results         = results;
                response.DiagnosticInfos = diagnosticInfos;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the HistoryUpdate service.
        /// </summary>
        public virtual HistoryUpdateResponseMessage HistoryUpdate(HistoryUpdateMessage request)
        {
            HistoryUpdateResponse response = null;

            try
            {
                // OnRequestReceived(message.HistoryUpdateRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (HistoryUpdateResponse)HistoryUpdate(request.HistoryUpdateRequest);
                // OnResponseSent(response);
                return new HistoryUpdateResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.HistoryUpdateRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the HistoryUpdate service.
        /// </summary>
        public virtual IAsyncResult BeginHistoryUpdate(HistoryUpdateMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.HistoryUpdateRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.HistoryUpdateRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.HistoryUpdateRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the HistoryUpdate service to complete.
        /// </summary>
        public virtual HistoryUpdateResponseMessage EndHistoryUpdate(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new HistoryUpdateResponseMessage((HistoryUpdateResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region Call Service
        #if (!OPCUA_EXCLUDE_Call)
        /// <summary>
        /// Invokes the Call service.
        /// </summary>
        public IServiceResponse Call(IServiceRequest incoming)
        {
            CallResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                CallRequest request = (CallRequest)incoming;

                CallMethodResultCollection results = null;
                DiagnosticInfoCollection diagnosticInfos = null;

                response = new CallResponse();

                response.ResponseHeader = ServerInstance.Call(
                   request.RequestHeader,
                   request.MethodsToCall,
                   out results,
                   out diagnosticInfos);

                response.Results         = results;
                response.DiagnosticInfos = diagnosticInfos;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the Call service.
        /// </summary>
        public virtual CallResponseMessage Call(CallMessage request)
        {
            CallResponse response = null;

            try
            {
                // OnRequestReceived(message.CallRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (CallResponse)Call(request.CallRequest);
                // OnResponseSent(response);
                return new CallResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.CallRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the Call service.
        /// </summary>
        public virtual IAsyncResult BeginCall(CallMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.CallRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.CallRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.CallRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the Call service to complete.
        /// </summary>
        public virtual CallResponseMessage EndCall(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new CallResponseMessage((CallResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region CreateMonitoredItems Service
        #if (!OPCUA_EXCLUDE_CreateMonitoredItems)
        /// <summary>
        /// Invokes the CreateMonitoredItems service.
        /// </summary>
        public IServiceResponse CreateMonitoredItems(IServiceRequest incoming)
        {
            CreateMonitoredItemsResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                CreateMonitoredItemsRequest request = (CreateMonitoredItemsRequest)incoming;

                MonitoredItemCreateResultCollection results = null;
                DiagnosticInfoCollection diagnosticInfos = null;

                response = new CreateMonitoredItemsResponse();

                response.ResponseHeader = ServerInstance.CreateMonitoredItems(
                   request.RequestHeader,
                   request.SubscriptionId,
                   request.TimestampsToReturn,
                   request.ItemsToCreate,
                   out results,
                   out diagnosticInfos);

                response.Results         = results;
                response.DiagnosticInfos = diagnosticInfos;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the CreateMonitoredItems service.
        /// </summary>
        public virtual CreateMonitoredItemsResponseMessage CreateMonitoredItems(CreateMonitoredItemsMessage request)
        {
            CreateMonitoredItemsResponse response = null;

            try
            {
                // OnRequestReceived(message.CreateMonitoredItemsRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (CreateMonitoredItemsResponse)CreateMonitoredItems(request.CreateMonitoredItemsRequest);
                // OnResponseSent(response);
                return new CreateMonitoredItemsResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.CreateMonitoredItemsRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the CreateMonitoredItems service.
        /// </summary>
        public virtual IAsyncResult BeginCreateMonitoredItems(CreateMonitoredItemsMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.CreateMonitoredItemsRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.CreateMonitoredItemsRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.CreateMonitoredItemsRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the CreateMonitoredItems service to complete.
        /// </summary>
        public virtual CreateMonitoredItemsResponseMessage EndCreateMonitoredItems(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new CreateMonitoredItemsResponseMessage((CreateMonitoredItemsResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region ModifyMonitoredItems Service
        #if (!OPCUA_EXCLUDE_ModifyMonitoredItems)
        /// <summary>
        /// Invokes the ModifyMonitoredItems service.
        /// </summary>
        public IServiceResponse ModifyMonitoredItems(IServiceRequest incoming)
        {
            ModifyMonitoredItemsResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                ModifyMonitoredItemsRequest request = (ModifyMonitoredItemsRequest)incoming;

                MonitoredItemModifyResultCollection results = null;
                DiagnosticInfoCollection diagnosticInfos = null;

                response = new ModifyMonitoredItemsResponse();

                response.ResponseHeader = ServerInstance.ModifyMonitoredItems(
                   request.RequestHeader,
                   request.SubscriptionId,
                   request.TimestampsToReturn,
                   request.ItemsToModify,
                   out results,
                   out diagnosticInfos);

                response.Results         = results;
                response.DiagnosticInfos = diagnosticInfos;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the ModifyMonitoredItems service.
        /// </summary>
        public virtual ModifyMonitoredItemsResponseMessage ModifyMonitoredItems(ModifyMonitoredItemsMessage request)
        {
            ModifyMonitoredItemsResponse response = null;

            try
            {
                // OnRequestReceived(message.ModifyMonitoredItemsRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (ModifyMonitoredItemsResponse)ModifyMonitoredItems(request.ModifyMonitoredItemsRequest);
                // OnResponseSent(response);
                return new ModifyMonitoredItemsResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.ModifyMonitoredItemsRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the ModifyMonitoredItems service.
        /// </summary>
        public virtual IAsyncResult BeginModifyMonitoredItems(ModifyMonitoredItemsMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.ModifyMonitoredItemsRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.ModifyMonitoredItemsRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.ModifyMonitoredItemsRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the ModifyMonitoredItems service to complete.
        /// </summary>
        public virtual ModifyMonitoredItemsResponseMessage EndModifyMonitoredItems(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new ModifyMonitoredItemsResponseMessage((ModifyMonitoredItemsResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region SetMonitoringMode Service
        #if (!OPCUA_EXCLUDE_SetMonitoringMode)
        /// <summary>
        /// Invokes the SetMonitoringMode service.
        /// </summary>
        public IServiceResponse SetMonitoringMode(IServiceRequest incoming)
        {
            SetMonitoringModeResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                SetMonitoringModeRequest request = (SetMonitoringModeRequest)incoming;

                StatusCodeCollection results = null;
                DiagnosticInfoCollection diagnosticInfos = null;

                response = new SetMonitoringModeResponse();

                response.ResponseHeader = ServerInstance.SetMonitoringMode(
                   request.RequestHeader,
                   request.SubscriptionId,
                   request.MonitoringMode,
                   request.MonitoredItemIds,
                   out results,
                   out diagnosticInfos);

                response.Results         = results;
                response.DiagnosticInfos = diagnosticInfos;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the SetMonitoringMode service.
        /// </summary>
        public virtual SetMonitoringModeResponseMessage SetMonitoringMode(SetMonitoringModeMessage request)
        {
            SetMonitoringModeResponse response = null;

            try
            {
                // OnRequestReceived(message.SetMonitoringModeRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (SetMonitoringModeResponse)SetMonitoringMode(request.SetMonitoringModeRequest);
                // OnResponseSent(response);
                return new SetMonitoringModeResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.SetMonitoringModeRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the SetMonitoringMode service.
        /// </summary>
        public virtual IAsyncResult BeginSetMonitoringMode(SetMonitoringModeMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.SetMonitoringModeRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.SetMonitoringModeRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.SetMonitoringModeRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the SetMonitoringMode service to complete.
        /// </summary>
        public virtual SetMonitoringModeResponseMessage EndSetMonitoringMode(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new SetMonitoringModeResponseMessage((SetMonitoringModeResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region SetTriggering Service
        #if (!OPCUA_EXCLUDE_SetTriggering)
        /// <summary>
        /// Invokes the SetTriggering service.
        /// </summary>
        public IServiceResponse SetTriggering(IServiceRequest incoming)
        {
            SetTriggeringResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                SetTriggeringRequest request = (SetTriggeringRequest)incoming;

                StatusCodeCollection addResults = null;
                DiagnosticInfoCollection addDiagnosticInfos = null;
                StatusCodeCollection removeResults = null;
                DiagnosticInfoCollection removeDiagnosticInfos = null;

                response = new SetTriggeringResponse();

                response.ResponseHeader = ServerInstance.SetTriggering(
                   request.RequestHeader,
                   request.SubscriptionId,
                   request.TriggeringItemId,
                   request.LinksToAdd,
                   request.LinksToRemove,
                   out addResults,
                   out addDiagnosticInfos,
                   out removeResults,
                   out removeDiagnosticInfos);

                response.AddResults            = addResults;
                response.AddDiagnosticInfos    = addDiagnosticInfos;
                response.RemoveResults         = removeResults;
                response.RemoveDiagnosticInfos = removeDiagnosticInfos;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the SetTriggering service.
        /// </summary>
        public virtual SetTriggeringResponseMessage SetTriggering(SetTriggeringMessage request)
        {
            SetTriggeringResponse response = null;

            try
            {
                // OnRequestReceived(message.SetTriggeringRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (SetTriggeringResponse)SetTriggering(request.SetTriggeringRequest);
                // OnResponseSent(response);
                return new SetTriggeringResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.SetTriggeringRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the SetTriggering service.
        /// </summary>
        public virtual IAsyncResult BeginSetTriggering(SetTriggeringMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.SetTriggeringRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.SetTriggeringRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.SetTriggeringRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the SetTriggering service to complete.
        /// </summary>
        public virtual SetTriggeringResponseMessage EndSetTriggering(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new SetTriggeringResponseMessage((SetTriggeringResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region DeleteMonitoredItems Service
        #if (!OPCUA_EXCLUDE_DeleteMonitoredItems)
        /// <summary>
        /// Invokes the DeleteMonitoredItems service.
        /// </summary>
        public IServiceResponse DeleteMonitoredItems(IServiceRequest incoming)
        {
            DeleteMonitoredItemsResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                DeleteMonitoredItemsRequest request = (DeleteMonitoredItemsRequest)incoming;

                StatusCodeCollection results = null;
                DiagnosticInfoCollection diagnosticInfos = null;

                response = new DeleteMonitoredItemsResponse();

                response.ResponseHeader = ServerInstance.DeleteMonitoredItems(
                   request.RequestHeader,
                   request.SubscriptionId,
                   request.MonitoredItemIds,
                   out results,
                   out diagnosticInfos);

                response.Results         = results;
                response.DiagnosticInfos = diagnosticInfos;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the DeleteMonitoredItems service.
        /// </summary>
        public virtual DeleteMonitoredItemsResponseMessage DeleteMonitoredItems(DeleteMonitoredItemsMessage request)
        {
            DeleteMonitoredItemsResponse response = null;

            try
            {
                // OnRequestReceived(message.DeleteMonitoredItemsRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (DeleteMonitoredItemsResponse)DeleteMonitoredItems(request.DeleteMonitoredItemsRequest);
                // OnResponseSent(response);
                return new DeleteMonitoredItemsResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.DeleteMonitoredItemsRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the DeleteMonitoredItems service.
        /// </summary>
        public virtual IAsyncResult BeginDeleteMonitoredItems(DeleteMonitoredItemsMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.DeleteMonitoredItemsRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.DeleteMonitoredItemsRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.DeleteMonitoredItemsRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the DeleteMonitoredItems service to complete.
        /// </summary>
        public virtual DeleteMonitoredItemsResponseMessage EndDeleteMonitoredItems(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new DeleteMonitoredItemsResponseMessage((DeleteMonitoredItemsResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region CreateSubscription Service
        #if (!OPCUA_EXCLUDE_CreateSubscription)
        /// <summary>
        /// Invokes the CreateSubscription service.
        /// </summary>
        public IServiceResponse CreateSubscription(IServiceRequest incoming)
        {
            CreateSubscriptionResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                CreateSubscriptionRequest request = (CreateSubscriptionRequest)incoming;

                uint subscriptionId = 0;
                double revisedPublishingInterval = 0;
                uint revisedLifetimeCount = 0;
                uint revisedMaxKeepAliveCount = 0;

                response = new CreateSubscriptionResponse();

                response.ResponseHeader = ServerInstance.CreateSubscription(
                   request.RequestHeader,
                   request.RequestedPublishingInterval,
                   request.RequestedLifetimeCount,
                   request.RequestedMaxKeepAliveCount,
                   request.MaxNotificationsPerPublish,
                   request.PublishingEnabled,
                   request.Priority,
                   out subscriptionId,
                   out revisedPublishingInterval,
                   out revisedLifetimeCount,
                   out revisedMaxKeepAliveCount);

                response.SubscriptionId            = subscriptionId;
                response.RevisedPublishingInterval = revisedPublishingInterval;
                response.RevisedLifetimeCount      = revisedLifetimeCount;
                response.RevisedMaxKeepAliveCount  = revisedMaxKeepAliveCount;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the CreateSubscription service.
        /// </summary>
        public virtual CreateSubscriptionResponseMessage CreateSubscription(CreateSubscriptionMessage request)
        {
            CreateSubscriptionResponse response = null;

            try
            {
                // OnRequestReceived(message.CreateSubscriptionRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (CreateSubscriptionResponse)CreateSubscription(request.CreateSubscriptionRequest);
                // OnResponseSent(response);
                return new CreateSubscriptionResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.CreateSubscriptionRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the CreateSubscription service.
        /// </summary>
        public virtual IAsyncResult BeginCreateSubscription(CreateSubscriptionMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.CreateSubscriptionRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.CreateSubscriptionRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.CreateSubscriptionRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the CreateSubscription service to complete.
        /// </summary>
        public virtual CreateSubscriptionResponseMessage EndCreateSubscription(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new CreateSubscriptionResponseMessage((CreateSubscriptionResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region ModifySubscription Service
        #if (!OPCUA_EXCLUDE_ModifySubscription)
        /// <summary>
        /// Invokes the ModifySubscription service.
        /// </summary>
        public IServiceResponse ModifySubscription(IServiceRequest incoming)
        {
            ModifySubscriptionResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                ModifySubscriptionRequest request = (ModifySubscriptionRequest)incoming;

                double revisedPublishingInterval = 0;
                uint revisedLifetimeCount = 0;
                uint revisedMaxKeepAliveCount = 0;

                response = new ModifySubscriptionResponse();

                response.ResponseHeader = ServerInstance.ModifySubscription(
                   request.RequestHeader,
                   request.SubscriptionId,
                   request.RequestedPublishingInterval,
                   request.RequestedLifetimeCount,
                   request.RequestedMaxKeepAliveCount,
                   request.MaxNotificationsPerPublish,
                   request.Priority,
                   out revisedPublishingInterval,
                   out revisedLifetimeCount,
                   out revisedMaxKeepAliveCount);

                response.RevisedPublishingInterval = revisedPublishingInterval;
                response.RevisedLifetimeCount      = revisedLifetimeCount;
                response.RevisedMaxKeepAliveCount  = revisedMaxKeepAliveCount;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the ModifySubscription service.
        /// </summary>
        public virtual ModifySubscriptionResponseMessage ModifySubscription(ModifySubscriptionMessage request)
        {
            ModifySubscriptionResponse response = null;

            try
            {
                // OnRequestReceived(message.ModifySubscriptionRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (ModifySubscriptionResponse)ModifySubscription(request.ModifySubscriptionRequest);
                // OnResponseSent(response);
                return new ModifySubscriptionResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.ModifySubscriptionRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the ModifySubscription service.
        /// </summary>
        public virtual IAsyncResult BeginModifySubscription(ModifySubscriptionMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.ModifySubscriptionRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.ModifySubscriptionRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.ModifySubscriptionRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the ModifySubscription service to complete.
        /// </summary>
        public virtual ModifySubscriptionResponseMessage EndModifySubscription(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new ModifySubscriptionResponseMessage((ModifySubscriptionResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region SetPublishingMode Service
        #if (!OPCUA_EXCLUDE_SetPublishingMode)
        /// <summary>
        /// Invokes the SetPublishingMode service.
        /// </summary>
        public IServiceResponse SetPublishingMode(IServiceRequest incoming)
        {
            SetPublishingModeResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                SetPublishingModeRequest request = (SetPublishingModeRequest)incoming;

                StatusCodeCollection results = null;
                DiagnosticInfoCollection diagnosticInfos = null;

                response = new SetPublishingModeResponse();

                response.ResponseHeader = ServerInstance.SetPublishingMode(
                   request.RequestHeader,
                   request.PublishingEnabled,
                   request.SubscriptionIds,
                   out results,
                   out diagnosticInfos);

                response.Results         = results;
                response.DiagnosticInfos = diagnosticInfos;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the SetPublishingMode service.
        /// </summary>
        public virtual SetPublishingModeResponseMessage SetPublishingMode(SetPublishingModeMessage request)
        {
            SetPublishingModeResponse response = null;

            try
            {
                // OnRequestReceived(message.SetPublishingModeRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (SetPublishingModeResponse)SetPublishingMode(request.SetPublishingModeRequest);
                // OnResponseSent(response);
                return new SetPublishingModeResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.SetPublishingModeRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the SetPublishingMode service.
        /// </summary>
        public virtual IAsyncResult BeginSetPublishingMode(SetPublishingModeMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.SetPublishingModeRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.SetPublishingModeRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.SetPublishingModeRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the SetPublishingMode service to complete.
        /// </summary>
        public virtual SetPublishingModeResponseMessage EndSetPublishingMode(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new SetPublishingModeResponseMessage((SetPublishingModeResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region Publish Service
        #if (!OPCUA_EXCLUDE_Publish)
        /// <summary>
        /// Invokes the Publish service.
        /// </summary>
        public IServiceResponse Publish(IServiceRequest incoming)
        {
            PublishResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                PublishRequest request = (PublishRequest)incoming;

                uint subscriptionId = 0;
                UInt32Collection availableSequenceNumbers = null;
                bool moreNotifications = false;
                NotificationMessage notificationMessage = null;
                StatusCodeCollection results = null;
                DiagnosticInfoCollection diagnosticInfos = null;

                response = new PublishResponse();

                response.ResponseHeader = ServerInstance.Publish(
                   request.RequestHeader,
                   request.SubscriptionAcknowledgements,
                   out subscriptionId,
                   out availableSequenceNumbers,
                   out moreNotifications,
                   out notificationMessage,
                   out results,
                   out diagnosticInfos);

                response.SubscriptionId           = subscriptionId;
                response.AvailableSequenceNumbers = availableSequenceNumbers;
                response.MoreNotifications        = moreNotifications;
                response.NotificationMessage      = notificationMessage;
                response.Results                  = results;
                response.DiagnosticInfos          = diagnosticInfos;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the Publish service.
        /// </summary>
        public virtual PublishResponseMessage Publish(PublishMessage request)
        {
            PublishResponse response = null;

            try
            {
                // OnRequestReceived(message.PublishRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (PublishResponse)Publish(request.PublishRequest);
                // OnResponseSent(response);
                return new PublishResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.PublishRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the Publish service.
        /// </summary>
        public virtual IAsyncResult BeginPublish(PublishMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.PublishRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.PublishRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.PublishRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the Publish service to complete.
        /// </summary>
        public virtual PublishResponseMessage EndPublish(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new PublishResponseMessage((PublishResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region Republish Service
        #if (!OPCUA_EXCLUDE_Republish)
        /// <summary>
        /// Invokes the Republish service.
        /// </summary>
        public IServiceResponse Republish(IServiceRequest incoming)
        {
            RepublishResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                RepublishRequest request = (RepublishRequest)incoming;

                NotificationMessage notificationMessage = null;

                response = new RepublishResponse();

                response.ResponseHeader = ServerInstance.Republish(
                   request.RequestHeader,
                   request.SubscriptionId,
                   request.RetransmitSequenceNumber,
                   out notificationMessage);

                response.NotificationMessage = notificationMessage;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the Republish service.
        /// </summary>
        public virtual RepublishResponseMessage Republish(RepublishMessage request)
        {
            RepublishResponse response = null;

            try
            {
                // OnRequestReceived(message.RepublishRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (RepublishResponse)Republish(request.RepublishRequest);
                // OnResponseSent(response);
                return new RepublishResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.RepublishRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the Republish service.
        /// </summary>
        public virtual IAsyncResult BeginRepublish(RepublishMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.RepublishRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.RepublishRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.RepublishRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the Republish service to complete.
        /// </summary>
        public virtual RepublishResponseMessage EndRepublish(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new RepublishResponseMessage((RepublishResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region TransferSubscriptions Service
        #if (!OPCUA_EXCLUDE_TransferSubscriptions)
        /// <summary>
        /// Invokes the TransferSubscriptions service.
        /// </summary>
        public IServiceResponse TransferSubscriptions(IServiceRequest incoming)
        {
            TransferSubscriptionsResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                TransferSubscriptionsRequest request = (TransferSubscriptionsRequest)incoming;

                TransferResultCollection results = null;
                DiagnosticInfoCollection diagnosticInfos = null;

                response = new TransferSubscriptionsResponse();

                response.ResponseHeader = ServerInstance.TransferSubscriptions(
                   request.RequestHeader,
                   request.SubscriptionIds,
                   request.SendInitialValues,
                   out results,
                   out diagnosticInfos);

                response.Results         = results;
                response.DiagnosticInfos = diagnosticInfos;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the TransferSubscriptions service.
        /// </summary>
        public virtual TransferSubscriptionsResponseMessage TransferSubscriptions(TransferSubscriptionsMessage request)
        {
            TransferSubscriptionsResponse response = null;

            try
            {
                // OnRequestReceived(message.TransferSubscriptionsRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (TransferSubscriptionsResponse)TransferSubscriptions(request.TransferSubscriptionsRequest);
                // OnResponseSent(response);
                return new TransferSubscriptionsResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.TransferSubscriptionsRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the TransferSubscriptions service.
        /// </summary>
        public virtual IAsyncResult BeginTransferSubscriptions(TransferSubscriptionsMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.TransferSubscriptionsRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.TransferSubscriptionsRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.TransferSubscriptionsRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the TransferSubscriptions service to complete.
        /// </summary>
        public virtual TransferSubscriptionsResponseMessage EndTransferSubscriptions(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new TransferSubscriptionsResponseMessage((TransferSubscriptionsResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region DeleteSubscriptions Service
        #if (!OPCUA_EXCLUDE_DeleteSubscriptions)
        /// <summary>
        /// Invokes the DeleteSubscriptions service.
        /// </summary>
        public IServiceResponse DeleteSubscriptions(IServiceRequest incoming)
        {
            DeleteSubscriptionsResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                DeleteSubscriptionsRequest request = (DeleteSubscriptionsRequest)incoming;

                StatusCodeCollection results = null;
                DiagnosticInfoCollection diagnosticInfos = null;

                response = new DeleteSubscriptionsResponse();

                response.ResponseHeader = ServerInstance.DeleteSubscriptions(
                   request.RequestHeader,
                   request.SubscriptionIds,
                   out results,
                   out diagnosticInfos);

                response.Results         = results;
                response.DiagnosticInfos = diagnosticInfos;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the DeleteSubscriptions service.
        /// </summary>
        public virtual DeleteSubscriptionsResponseMessage DeleteSubscriptions(DeleteSubscriptionsMessage request)
        {
            DeleteSubscriptionsResponse response = null;

            try
            {
                // OnRequestReceived(message.DeleteSubscriptionsRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (DeleteSubscriptionsResponse)DeleteSubscriptions(request.DeleteSubscriptionsRequest);
                // OnResponseSent(response);
                return new DeleteSubscriptionsResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.DeleteSubscriptionsRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the DeleteSubscriptions service.
        /// </summary>
        public virtual IAsyncResult BeginDeleteSubscriptions(DeleteSubscriptionsMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.DeleteSubscriptionsRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.DeleteSubscriptionsRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.DeleteSubscriptionsRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the DeleteSubscriptions service to complete.
        /// </summary>
        public virtual DeleteSubscriptionsResponseMessage EndDeleteSubscriptions(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new DeleteSubscriptionsResponseMessage((DeleteSubscriptionsResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion
        #endregion

        #region Protected Members
        /// <summary>
        /// Populates the known types table.
        /// </summary>
        protected virtual void CreateKnownTypes()
        {
            #if (!OPCUA_EXCLUDE_FindServers)
            SupportedServices.Add(DataTypeIds.FindServersRequest, new ServiceDefinition(typeof(FindServersRequest), new InvokeServiceEventHandler(FindServers)));
            #endif
            #if (!OPCUA_EXCLUDE_FindServersOnNetwork)
            SupportedServices.Add(DataTypeIds.FindServersOnNetworkRequest, new ServiceDefinition(typeof(FindServersOnNetworkRequest), new InvokeServiceEventHandler(FindServersOnNetwork)));
            #endif
            #if (!OPCUA_EXCLUDE_GetEndpoints)
            SupportedServices.Add(DataTypeIds.GetEndpointsRequest, new ServiceDefinition(typeof(GetEndpointsRequest), new InvokeServiceEventHandler(GetEndpoints)));
            #endif
            #if (!OPCUA_EXCLUDE_CreateSession)
            SupportedServices.Add(DataTypeIds.CreateSessionRequest, new ServiceDefinition(typeof(CreateSessionRequest), new InvokeServiceEventHandler(CreateSession)));
            #endif
            #if (!OPCUA_EXCLUDE_ActivateSession)
            SupportedServices.Add(DataTypeIds.ActivateSessionRequest, new ServiceDefinition(typeof(ActivateSessionRequest), new InvokeServiceEventHandler(ActivateSession)));
            #endif
            #if (!OPCUA_EXCLUDE_CloseSession)
            SupportedServices.Add(DataTypeIds.CloseSessionRequest, new ServiceDefinition(typeof(CloseSessionRequest), new InvokeServiceEventHandler(CloseSession)));
            #endif
            #if (!OPCUA_EXCLUDE_Cancel)
            SupportedServices.Add(DataTypeIds.CancelRequest, new ServiceDefinition(typeof(CancelRequest), new InvokeServiceEventHandler(Cancel)));
            #endif
            #if (!OPCUA_EXCLUDE_AddNodes)
            SupportedServices.Add(DataTypeIds.AddNodesRequest, new ServiceDefinition(typeof(AddNodesRequest), new InvokeServiceEventHandler(AddNodes)));
            #endif
            #if (!OPCUA_EXCLUDE_AddReferences)
            SupportedServices.Add(DataTypeIds.AddReferencesRequest, new ServiceDefinition(typeof(AddReferencesRequest), new InvokeServiceEventHandler(AddReferences)));
            #endif
            #if (!OPCUA_EXCLUDE_DeleteNodes)
            SupportedServices.Add(DataTypeIds.DeleteNodesRequest, new ServiceDefinition(typeof(DeleteNodesRequest), new InvokeServiceEventHandler(DeleteNodes)));
            #endif
            #if (!OPCUA_EXCLUDE_DeleteReferences)
            SupportedServices.Add(DataTypeIds.DeleteReferencesRequest, new ServiceDefinition(typeof(DeleteReferencesRequest), new InvokeServiceEventHandler(DeleteReferences)));
            #endif
            #if (!OPCUA_EXCLUDE_Browse)
            SupportedServices.Add(DataTypeIds.BrowseRequest, new ServiceDefinition(typeof(BrowseRequest), new InvokeServiceEventHandler(Browse)));
            #endif
            #if (!OPCUA_EXCLUDE_BrowseNext)
            SupportedServices.Add(DataTypeIds.BrowseNextRequest, new ServiceDefinition(typeof(BrowseNextRequest), new InvokeServiceEventHandler(BrowseNext)));
            #endif
            #if (!OPCUA_EXCLUDE_TranslateBrowsePathsToNodeIds)
            SupportedServices.Add(DataTypeIds.TranslateBrowsePathsToNodeIdsRequest, new ServiceDefinition(typeof(TranslateBrowsePathsToNodeIdsRequest), new InvokeServiceEventHandler(TranslateBrowsePathsToNodeIds)));
            #endif
            #if (!OPCUA_EXCLUDE_RegisterNodes)
            SupportedServices.Add(DataTypeIds.RegisterNodesRequest, new ServiceDefinition(typeof(RegisterNodesRequest), new InvokeServiceEventHandler(RegisterNodes)));
            #endif
            #if (!OPCUA_EXCLUDE_UnregisterNodes)
            SupportedServices.Add(DataTypeIds.UnregisterNodesRequest, new ServiceDefinition(typeof(UnregisterNodesRequest), new InvokeServiceEventHandler(UnregisterNodes)));
            #endif
            #if (!OPCUA_EXCLUDE_QueryFirst)
            SupportedServices.Add(DataTypeIds.QueryFirstRequest, new ServiceDefinition(typeof(QueryFirstRequest), new InvokeServiceEventHandler(QueryFirst)));
            #endif
            #if (!OPCUA_EXCLUDE_QueryNext)
            SupportedServices.Add(DataTypeIds.QueryNextRequest, new ServiceDefinition(typeof(QueryNextRequest), new InvokeServiceEventHandler(QueryNext)));
            #endif
            #if (!OPCUA_EXCLUDE_Read)
            SupportedServices.Add(DataTypeIds.ReadRequest, new ServiceDefinition(typeof(ReadRequest), new InvokeServiceEventHandler(Read)));
            #endif
            #if (!OPCUA_EXCLUDE_HistoryRead)
            SupportedServices.Add(DataTypeIds.HistoryReadRequest, new ServiceDefinition(typeof(HistoryReadRequest), new InvokeServiceEventHandler(HistoryRead)));
            #endif
            #if (!OPCUA_EXCLUDE_Write)
            SupportedServices.Add(DataTypeIds.WriteRequest, new ServiceDefinition(typeof(WriteRequest), new InvokeServiceEventHandler(Write)));
            #endif
            #if (!OPCUA_EXCLUDE_HistoryUpdate)
            SupportedServices.Add(DataTypeIds.HistoryUpdateRequest, new ServiceDefinition(typeof(HistoryUpdateRequest), new InvokeServiceEventHandler(HistoryUpdate)));
            #endif
            #if (!OPCUA_EXCLUDE_Call)
            SupportedServices.Add(DataTypeIds.CallRequest, new ServiceDefinition(typeof(CallRequest), new InvokeServiceEventHandler(Call)));
            #endif
            #if (!OPCUA_EXCLUDE_CreateMonitoredItems)
            SupportedServices.Add(DataTypeIds.CreateMonitoredItemsRequest, new ServiceDefinition(typeof(CreateMonitoredItemsRequest), new InvokeServiceEventHandler(CreateMonitoredItems)));
            #endif
            #if (!OPCUA_EXCLUDE_ModifyMonitoredItems)
            SupportedServices.Add(DataTypeIds.ModifyMonitoredItemsRequest, new ServiceDefinition(typeof(ModifyMonitoredItemsRequest), new InvokeServiceEventHandler(ModifyMonitoredItems)));
            #endif
            #if (!OPCUA_EXCLUDE_SetMonitoringMode)
            SupportedServices.Add(DataTypeIds.SetMonitoringModeRequest, new ServiceDefinition(typeof(SetMonitoringModeRequest), new InvokeServiceEventHandler(SetMonitoringMode)));
            #endif
            #if (!OPCUA_EXCLUDE_SetTriggering)
            SupportedServices.Add(DataTypeIds.SetTriggeringRequest, new ServiceDefinition(typeof(SetTriggeringRequest), new InvokeServiceEventHandler(SetTriggering)));
            #endif
            #if (!OPCUA_EXCLUDE_DeleteMonitoredItems)
            SupportedServices.Add(DataTypeIds.DeleteMonitoredItemsRequest, new ServiceDefinition(typeof(DeleteMonitoredItemsRequest), new InvokeServiceEventHandler(DeleteMonitoredItems)));
            #endif
            #if (!OPCUA_EXCLUDE_CreateSubscription)
            SupportedServices.Add(DataTypeIds.CreateSubscriptionRequest, new ServiceDefinition(typeof(CreateSubscriptionRequest), new InvokeServiceEventHandler(CreateSubscription)));
            #endif
            #if (!OPCUA_EXCLUDE_ModifySubscription)
            SupportedServices.Add(DataTypeIds.ModifySubscriptionRequest, new ServiceDefinition(typeof(ModifySubscriptionRequest), new InvokeServiceEventHandler(ModifySubscription)));
            #endif
            #if (!OPCUA_EXCLUDE_SetPublishingMode)
            SupportedServices.Add(DataTypeIds.SetPublishingModeRequest, new ServiceDefinition(typeof(SetPublishingModeRequest), new InvokeServiceEventHandler(SetPublishingMode)));
            #endif
            #if (!OPCUA_EXCLUDE_Publish)
            SupportedServices.Add(DataTypeIds.PublishRequest, new ServiceDefinition(typeof(PublishRequest), new InvokeServiceEventHandler(Publish)));
            #endif
            #if (!OPCUA_EXCLUDE_Republish)
            SupportedServices.Add(DataTypeIds.RepublishRequest, new ServiceDefinition(typeof(RepublishRequest), new InvokeServiceEventHandler(Republish)));
            #endif
            #if (!OPCUA_EXCLUDE_TransferSubscriptions)
            SupportedServices.Add(DataTypeIds.TransferSubscriptionsRequest, new ServiceDefinition(typeof(TransferSubscriptionsRequest), new InvokeServiceEventHandler(TransferSubscriptions)));
            #endif
            #if (!OPCUA_EXCLUDE_DeleteSubscriptions)
            SupportedServices.Add(DataTypeIds.DeleteSubscriptionsRequest, new ServiceDefinition(typeof(DeleteSubscriptionsRequest), new InvokeServiceEventHandler(DeleteSubscriptions)));
            #endif
        }
        #endregion
    }
    #endregion

    #region DiscoveryEndpoint Class
    /// <summary>
    /// A endpoint object used by clients to access a UA service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.CodeGenerator", "1.0.0.0")]
    #if !NET_STANDARD
    [ServiceMessageContextBehavior()]
    [ServiceBehavior(Namespace = Namespaces.OpcUaWsdl, InstanceContextMode=InstanceContextMode.PerSession, ConcurrencyMode=ConcurrencyMode.Multiple)]
    #endif
    public partial class DiscoveryEndpoint : EndpointBase, IDiscoveryEndpoint, IRegistrationEndpoint
    {
        #region Constructors
        /// <summary>
        /// Initializes the object when it is created by the WCF framework.
        /// </summary>
        public DiscoveryEndpoint()
        {
            this.CreateKnownTypes();
        }

        /// <summary>
        /// Initializes the when it is created directly.
        /// </summary>
        public DiscoveryEndpoint(IServiceHostBase host) : base(host)
        {
            this.CreateKnownTypes();
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="DiscoveryEndpoint"/> class.
        /// </summary>
        /// <param name="server">The server.</param>
        public DiscoveryEndpoint(ServerBase server) : base(server)
        {
            this.CreateKnownTypes();
        }
        #endregion

        #region Public Members
        /// <summary>
        /// The UA server instance that the endpoint is connected to.
        /// </summary>
        protected IDiscoveryServer ServerInstance
        {
            get
            {
                if (ServiceResult.IsBad(ServerError))
                {
                    throw new ServiceResultException(ServerError);
                }

                return ServerForContext as IDiscoveryServer;
             }
        }
        #endregion

        #region IDiscoveryEndpoint Members
        #region FindServers Service
        #if (!OPCUA_EXCLUDE_FindServers)
        /// <summary>
        /// Invokes the FindServers service.
        /// </summary>
        public IServiceResponse FindServers(IServiceRequest incoming)
        {
            FindServersResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                FindServersRequest request = (FindServersRequest)incoming;

                ApplicationDescriptionCollection servers = null;

                response = new FindServersResponse();

                response.ResponseHeader = ServerInstance.FindServers(
                   request.RequestHeader,
                   request.EndpointUrl,
                   request.LocaleIds,
                   request.ServerUris,
                   out servers);

                response.Servers = servers;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the FindServers service.
        /// </summary>
        public virtual FindServersResponseMessage FindServers(FindServersMessage request)
        {
            FindServersResponse response = null;

            try
            {
                // OnRequestReceived(message.FindServersRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (FindServersResponse)FindServers(request.FindServersRequest);
                // OnResponseSent(response);
                return new FindServersResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.FindServersRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the FindServers service.
        /// </summary>
        public virtual IAsyncResult BeginFindServers(FindServersMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.FindServersRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.FindServersRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.FindServersRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the FindServers service to complete.
        /// </summary>
        public virtual FindServersResponseMessage EndFindServers(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new FindServersResponseMessage((FindServersResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region FindServersOnNetwork Service
        #if (!OPCUA_EXCLUDE_FindServersOnNetwork)
        /// <summary>
        /// Invokes the FindServersOnNetwork service.
        /// </summary>
        public IServiceResponse FindServersOnNetwork(IServiceRequest incoming)
        {
            FindServersOnNetworkResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                FindServersOnNetworkRequest request = (FindServersOnNetworkRequest)incoming;

                DateTime lastCounterResetTime = DateTime.MinValue;
                ServerOnNetworkCollection servers = null;

                response = new FindServersOnNetworkResponse();

                response.ResponseHeader = ServerInstance.FindServersOnNetwork(
                   request.RequestHeader,
                   request.StartingRecordId,
                   request.MaxRecordsToReturn,
                   request.ServerCapabilityFilter,
                   out lastCounterResetTime,
                   out servers);

                response.LastCounterResetTime = lastCounterResetTime;
                response.Servers              = servers;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the FindServersOnNetwork service.
        /// </summary>
        public virtual FindServersOnNetworkResponseMessage FindServersOnNetwork(FindServersOnNetworkMessage request)
        {
            FindServersOnNetworkResponse response = null;

            try
            {
                // OnRequestReceived(message.FindServersOnNetworkRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (FindServersOnNetworkResponse)FindServersOnNetwork(request.FindServersOnNetworkRequest);
                // OnResponseSent(response);
                return new FindServersOnNetworkResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.FindServersOnNetworkRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the FindServersOnNetwork service.
        /// </summary>
        public virtual IAsyncResult BeginFindServersOnNetwork(FindServersOnNetworkMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.FindServersOnNetworkRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.FindServersOnNetworkRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.FindServersOnNetworkRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the FindServersOnNetwork service to complete.
        /// </summary>
        public virtual FindServersOnNetworkResponseMessage EndFindServersOnNetwork(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new FindServersOnNetworkResponseMessage((FindServersOnNetworkResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region GetEndpoints Service
        #if (!OPCUA_EXCLUDE_GetEndpoints)
        /// <summary>
        /// Invokes the GetEndpoints service.
        /// </summary>
        public IServiceResponse GetEndpoints(IServiceRequest incoming)
        {
            GetEndpointsResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                GetEndpointsRequest request = (GetEndpointsRequest)incoming;

                EndpointDescriptionCollection endpoints = null;

                response = new GetEndpointsResponse();

                response.ResponseHeader = ServerInstance.GetEndpoints(
                   request.RequestHeader,
                   request.EndpointUrl,
                   request.LocaleIds,
                   request.ProfileUris,
                   out endpoints);

                response.Endpoints = endpoints;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the GetEndpoints service.
        /// </summary>
        public virtual GetEndpointsResponseMessage GetEndpoints(GetEndpointsMessage request)
        {
            GetEndpointsResponse response = null;

            try
            {
                // OnRequestReceived(message.GetEndpointsRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (GetEndpointsResponse)GetEndpoints(request.GetEndpointsRequest);
                // OnResponseSent(response);
                return new GetEndpointsResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.GetEndpointsRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the GetEndpoints service.
        /// </summary>
        public virtual IAsyncResult BeginGetEndpoints(GetEndpointsMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.GetEndpointsRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.GetEndpointsRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.GetEndpointsRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the GetEndpoints service to complete.
        /// </summary>
        public virtual GetEndpointsResponseMessage EndGetEndpoints(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new GetEndpointsResponseMessage((GetEndpointsResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region RegisterServer Service
        #if (!OPCUA_EXCLUDE_RegisterServer)
        /// <summary>
        /// Invokes the RegisterServer service.
        /// </summary>
        public IServiceResponse RegisterServer(IServiceRequest incoming)
        {
            RegisterServerResponse response = null;

            try
            {
                OnRequestReceived(incoming);

                RegisterServerRequest request = (RegisterServerRequest)incoming;


                response = new RegisterServerResponse();

                response.ResponseHeader = ServerInstance.RegisterServer(
                   request.RequestHeader,
                   request.Server);

            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the RegisterServer service.
        /// </summary>
        public virtual RegisterServerResponseMessage RegisterServer(RegisterServerMessage request)
        {
            RegisterServerResponse response = null;

            try
            {
                // OnRequestReceived(message.RegisterServerRequest);

                SetRequestContext(RequestEncoding.Xml);
                response = (RegisterServerResponse)RegisterServer(request.RegisterServerRequest);
                // OnResponseSent(response);
                return new RegisterServerResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.RegisterServerRequest, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the RegisterServer service.
        /// </summary>
        public virtual IAsyncResult BeginRegisterServer(RegisterServerMessage message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.RegisterServerRequest);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.RegisterServerRequest);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.RegisterServerRequest, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the RegisterServer service to complete.
        /// </summary>
        public virtual RegisterServerResponseMessage EndRegisterServer(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new RegisterServerResponseMessage((RegisterServerResponse)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion

        #region RegisterServer2 Service
        #if (!OPCUA_EXCLUDE_RegisterServer2)
        /// <summary>
        /// Invokes the RegisterServer2 service.
        /// </summary>
        public IServiceResponse RegisterServer2(IServiceRequest incoming)
        {
            RegisterServer2Response response = null;

            try
            {
                OnRequestReceived(incoming);

                RegisterServer2Request request = (RegisterServer2Request)incoming;

                StatusCodeCollection configurationResults = null;
                DiagnosticInfoCollection diagnosticInfos = null;

                response = new RegisterServer2Response();

                response.ResponseHeader = ServerInstance.RegisterServer2(
                   request.RequestHeader,
                   request.Server,
                   request.DiscoveryConfiguration,
                   out configurationResults,
                   out diagnosticInfos);

                response.ConfigurationResults = configurationResults;
                response.DiagnosticInfos      = diagnosticInfos;
            }
            finally
            {
                OnResponseSent(response);
            }

            return response;
        }

        #if OPCUA_USE_SYNCHRONOUS_ENDPOINTS
        /// <summary>
        /// The operation contract for the RegisterServer2 service.
        /// </summary>
        public virtual RegisterServer2ResponseMessage RegisterServer2(RegisterServer2Message request)
        {
            RegisterServer2Response response = null;

            try
            {
                // OnRequestReceived(message.RegisterServer2Request);

                SetRequestContext(RequestEncoding.Xml);
                response = (RegisterServer2Response)RegisterServer2(request.RegisterServer2Request);
                // OnResponseSent(response);
                return new RegisterServer2ResponseMessage(response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(request.RegisterServer2Request, e);
                // OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #else
        /// <summary>
        /// Asynchronously calls the RegisterServer2 service.
        /// </summary>
        public virtual IAsyncResult BeginRegisterServer2(RegisterServer2Message message, AsyncCallback callback, object callbackData)
        {
            try
            {
                OnRequestReceived(message.RegisterServer2Request);

                // check for bad data.
                if (message == null) throw new ArgumentNullException("message");

                // set the request context.
                SetRequestContext(RequestEncoding.Xml);

                // create handler.
                ProcessRequestAsyncResult result = new ProcessRequestAsyncResult(this, callback, callbackData, 0);
                return result.BeginProcessRequest(SecureChannelContext.Current, message.RegisterServer2Request);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(message.RegisterServer2Request, e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }

        /// <summary>
        /// Waits for an asynchronous call to the RegisterServer2 service to complete.
        /// </summary>
        public virtual RegisterServer2ResponseMessage EndRegisterServer2(IAsyncResult ar)
        {
            try
            {
                IServiceResponse response = ProcessRequestAsyncResult.WaitForComplete(ar, true);
                OnResponseSent(response);
                return new RegisterServer2ResponseMessage((RegisterServer2Response)response);
            }
            catch (Exception e)
            {
                Exception fault = CreateSoapFault(ProcessRequestAsyncResult.GetRequest(ar), e);
                OnResponseFaultSent(fault);
                throw fault;
            }
        }
        #endif
        #endif
        #endregion
        #endregion

        #region Protected Members
        /// <summary>
        /// Populates the known types table.
        /// </summary>
        protected virtual void CreateKnownTypes()
        {
            #if (!OPCUA_EXCLUDE_FindServers)
            SupportedServices.Add(DataTypeIds.FindServersRequest, new ServiceDefinition(typeof(FindServersRequest), new InvokeServiceEventHandler(FindServers)));
            #endif
            #if (!OPCUA_EXCLUDE_FindServersOnNetwork)
            SupportedServices.Add(DataTypeIds.FindServersOnNetworkRequest, new ServiceDefinition(typeof(FindServersOnNetworkRequest), new InvokeServiceEventHandler(FindServersOnNetwork)));
            #endif
            #if (!OPCUA_EXCLUDE_GetEndpoints)
            SupportedServices.Add(DataTypeIds.GetEndpointsRequest, new ServiceDefinition(typeof(GetEndpointsRequest), new InvokeServiceEventHandler(GetEndpoints)));
            #endif
            #if (!OPCUA_EXCLUDE_RegisterServer)
            SupportedServices.Add(DataTypeIds.RegisterServerRequest, new ServiceDefinition(typeof(RegisterServerRequest), new InvokeServiceEventHandler(RegisterServer)));
            #endif
            #if (!OPCUA_EXCLUDE_RegisterServer2)
            SupportedServices.Add(DataTypeIds.RegisterServer2Request, new ServiceDefinition(typeof(RegisterServer2Request), new InvokeServiceEventHandler(RegisterServer2)));
            #endif
        }
        #endregion
    }
    #endregion
}
