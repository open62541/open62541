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

namespace Opc.Ua
{
    /// <summary>
    /// The client side interface for a UA server.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class SessionClient : ClientBase
    {
        #region Constructors
        /// <summary>
        /// Intializes the object with a channel and a message context.
        /// </summary>
        public SessionClient(ITransportChannel channel)
        :
            base(channel)
        {
        }
        #endregion

        #region Public Properties
        /// <summary>
        /// The component  contains classes  object use to communicate with the server.
        /// </summary>
        public new ISessionChannel InnerChannel
        {
            get { return (ISessionChannel)base.InnerChannel; }
        }
        #endregion

        #region Client API
        #region CreateSession Methods
        #if (!OPCUA_EXCLUDE_CreateSession)
        /// <summary>
        /// Invokes the CreateSession service.
        /// </summary>
        public virtual ResponseHeader CreateSession(
            RequestHeader                           requestHeader,
            ApplicationDescription                  clientDescription,
            string                                  serverUri,
            string                                  endpointUrl,
            string                                  sessionName,
            byte[]                                  clientNonce,
            byte[]                                  clientCertificate,
            double                                  requestedSessionTimeout,
            uint                                    maxResponseMessageSize,
            out NodeId                              sessionId,
            out NodeId                              authenticationToken,
            out double                              revisedSessionTimeout,
            out byte[]                              serverNonce,
            out byte[]                              serverCertificate,
            out EndpointDescriptionCollection       serverEndpoints,
            out SignedSoftwareCertificateCollection serverSoftwareCertificates,
            out SignatureData                       serverSignature,
            out uint                                maxRequestMessageSize)
        {
            CreateSessionRequest request = new CreateSessionRequest();
            CreateSessionResponse response = null;

            request.RequestHeader           = requestHeader;
            request.ClientDescription       = clientDescription;
            request.ServerUri               = serverUri;
            request.EndpointUrl             = endpointUrl;
            request.SessionName             = sessionName;
            request.ClientNonce             = clientNonce;
            request.ClientCertificate       = clientCertificate;
            request.RequestedSessionTimeout = requestedSessionTimeout;
            request.MaxResponseMessageSize  = maxResponseMessageSize;

            UpdateRequestHeader(request, requestHeader == null, "CreateSession");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (CreateSessionResponse)genericResponse;
                }
                else
                {
                    CreateSessionResponseMessage responseMessage = InnerChannel.CreateSession(new CreateSessionMessage(request));

                    if (responseMessage == null || responseMessage.CreateSessionResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.CreateSessionResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                sessionId                  = response.SessionId;
                authenticationToken        = response.AuthenticationToken;
                revisedSessionTimeout      = response.RevisedSessionTimeout;
                serverNonce                = response.ServerNonce;
                serverCertificate          = response.ServerCertificate;
                serverEndpoints            = response.ServerEndpoints;
                serverSoftwareCertificates = response.ServerSoftwareCertificates;
                serverSignature            = response.ServerSignature;
                maxRequestMessageSize      = response.MaxRequestMessageSize;
            }
            finally
            {
                RequestCompleted(request, response, "CreateSession");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the CreateSession service.
        /// </summary>
        public IAsyncResult BeginCreateSession(
            RequestHeader          requestHeader,
            ApplicationDescription clientDescription,
            string                 serverUri,
            string                 endpointUrl,
            string                 sessionName,
            byte[]                 clientNonce,
            byte[]                 clientCertificate,
            double                 requestedSessionTimeout,
            uint                   maxResponseMessageSize,
            AsyncCallback          callback,
            object                 asyncState)
        {
            CreateSessionRequest request = new CreateSessionRequest();

            request.RequestHeader           = requestHeader;
            request.ClientDescription       = clientDescription;
            request.ServerUri               = serverUri;
            request.EndpointUrl             = endpointUrl;
            request.SessionName             = sessionName;
            request.ClientNonce             = clientNonce;
            request.ClientCertificate       = clientCertificate;
            request.RequestedSessionTimeout = requestedSessionTimeout;
            request.MaxResponseMessageSize  = maxResponseMessageSize;

            UpdateRequestHeader(request, requestHeader == null, "CreateSession");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginCreateSession(new CreateSessionMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the CreateSession service.
        /// </summary>
        public ResponseHeader EndCreateSession(
            IAsyncResult                            result,
            out NodeId                              sessionId,
            out NodeId                              authenticationToken,
            out double                              revisedSessionTimeout,
            out byte[]                              serverNonce,
            out byte[]                              serverCertificate,
            out EndpointDescriptionCollection       serverEndpoints,
            out SignedSoftwareCertificateCollection serverSoftwareCertificates,
            out SignatureData                       serverSignature,
            out uint                                maxRequestMessageSize)
        {
            CreateSessionResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (CreateSessionResponse)genericResponse;
                }
                else
                {
                    CreateSessionResponseMessage responseMessage = InnerChannel.EndCreateSession(result);

                    if (responseMessage == null || responseMessage.CreateSessionResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.CreateSessionResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                sessionId                  = response.SessionId;
                authenticationToken        = response.AuthenticationToken;
                revisedSessionTimeout      = response.RevisedSessionTimeout;
                serverNonce                = response.ServerNonce;
                serverCertificate          = response.ServerCertificate;
                serverEndpoints            = response.ServerEndpoints;
                serverSoftwareCertificates = response.ServerSoftwareCertificates;
                serverSignature            = response.ServerSignature;
                maxRequestMessageSize      = response.MaxRequestMessageSize;
            }
            finally
            {
                RequestCompleted(null, response, "CreateSession");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region ActivateSession Methods
        #if (!OPCUA_EXCLUDE_ActivateSession)
        /// <summary>
        /// Invokes the ActivateSession service.
        /// </summary>
        public virtual ResponseHeader ActivateSession(
            RequestHeader                       requestHeader,
            SignatureData                       clientSignature,
            SignedSoftwareCertificateCollection clientSoftwareCertificates,
            StringCollection                    localeIds,
            ExtensionObject                     userIdentityToken,
            SignatureData                       userTokenSignature,
            out byte[]                          serverNonce,
            out StatusCodeCollection            results,
            out DiagnosticInfoCollection        diagnosticInfos)
        {
            ActivateSessionRequest request = new ActivateSessionRequest();
            ActivateSessionResponse response = null;

            request.RequestHeader              = requestHeader;
            request.ClientSignature            = clientSignature;
            request.ClientSoftwareCertificates = clientSoftwareCertificates;
            request.LocaleIds                  = localeIds;
            request.UserIdentityToken          = userIdentityToken;
            request.UserTokenSignature         = userTokenSignature;

            UpdateRequestHeader(request, requestHeader == null, "ActivateSession");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (ActivateSessionResponse)genericResponse;
                }
                else
                {
                    ActivateSessionResponseMessage responseMessage = InnerChannel.ActivateSession(new ActivateSessionMessage(request));

                    if (responseMessage == null || responseMessage.ActivateSessionResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.ActivateSessionResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                serverNonce     = response.ServerNonce;
                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(request, response, "ActivateSession");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the ActivateSession service.
        /// </summary>
        public IAsyncResult BeginActivateSession(
            RequestHeader                       requestHeader,
            SignatureData                       clientSignature,
            SignedSoftwareCertificateCollection clientSoftwareCertificates,
            StringCollection                    localeIds,
            ExtensionObject                     userIdentityToken,
            SignatureData                       userTokenSignature,
            AsyncCallback                       callback,
            object                              asyncState)
        {
            ActivateSessionRequest request = new ActivateSessionRequest();

            request.RequestHeader              = requestHeader;
            request.ClientSignature            = clientSignature;
            request.ClientSoftwareCertificates = clientSoftwareCertificates;
            request.LocaleIds                  = localeIds;
            request.UserIdentityToken          = userIdentityToken;
            request.UserTokenSignature         = userTokenSignature;

            UpdateRequestHeader(request, requestHeader == null, "ActivateSession");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginActivateSession(new ActivateSessionMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the ActivateSession service.
        /// </summary>
        public ResponseHeader EndActivateSession(
            IAsyncResult                 result,
            out byte[]                   serverNonce,
            out StatusCodeCollection     results,
            out DiagnosticInfoCollection diagnosticInfos)
        {
            ActivateSessionResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (ActivateSessionResponse)genericResponse;
                }
                else
                {
                    ActivateSessionResponseMessage responseMessage = InnerChannel.EndActivateSession(result);

                    if (responseMessage == null || responseMessage.ActivateSessionResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.ActivateSessionResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                serverNonce     = response.ServerNonce;
                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(null, response, "ActivateSession");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region CloseSession Methods
        #if (!OPCUA_EXCLUDE_CloseSession)
        /// <summary>
        /// Invokes the CloseSession service.
        /// </summary>
        public virtual ResponseHeader CloseSession(
            RequestHeader requestHeader,
            bool          deleteSubscriptions)
        {
            CloseSessionRequest request = new CloseSessionRequest();
            CloseSessionResponse response = null;

            request.RequestHeader       = requestHeader;
            request.DeleteSubscriptions = deleteSubscriptions;

            UpdateRequestHeader(request, requestHeader == null, "CloseSession");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (CloseSessionResponse)genericResponse;
                }
                else
                {
                    CloseSessionResponseMessage responseMessage = InnerChannel.CloseSession(new CloseSessionMessage(request));

                    if (responseMessage == null || responseMessage.CloseSessionResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.CloseSessionResponse;
                    ValidateResponse(response.ResponseHeader);
                }

            }
            finally
            {
                RequestCompleted(request, response, "CloseSession");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the CloseSession service.
        /// </summary>
        public IAsyncResult BeginCloseSession(
            RequestHeader requestHeader,
            bool          deleteSubscriptions,
            AsyncCallback callback,
            object        asyncState)
        {
            CloseSessionRequest request = new CloseSessionRequest();

            request.RequestHeader       = requestHeader;
            request.DeleteSubscriptions = deleteSubscriptions;

            UpdateRequestHeader(request, requestHeader == null, "CloseSession");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginCloseSession(new CloseSessionMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the CloseSession service.
        /// </summary>
        public ResponseHeader EndCloseSession(
            IAsyncResult result)
        {
            CloseSessionResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (CloseSessionResponse)genericResponse;
                }
                else
                {
                    CloseSessionResponseMessage responseMessage = InnerChannel.EndCloseSession(result);

                    if (responseMessage == null || responseMessage.CloseSessionResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.CloseSessionResponse;
                    ValidateResponse(response.ResponseHeader);
                }

            }
            finally
            {
                RequestCompleted(null, response, "CloseSession");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region Cancel Methods
        #if (!OPCUA_EXCLUDE_Cancel)
        /// <summary>
        /// Invokes the Cancel service.
        /// </summary>
        public virtual ResponseHeader Cancel(
            RequestHeader requestHeader,
            uint          requestHandle,
            out uint      cancelCount)
        {
            CancelRequest request = new CancelRequest();
            CancelResponse response = null;

            request.RequestHeader = requestHeader;
            request.RequestHandle = requestHandle;

            UpdateRequestHeader(request, requestHeader == null, "Cancel");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (CancelResponse)genericResponse;
                }
                else
                {
                    CancelResponseMessage responseMessage = InnerChannel.Cancel(new CancelMessage(request));

                    if (responseMessage == null || responseMessage.CancelResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.CancelResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                cancelCount = response.CancelCount;
            }
            finally
            {
                RequestCompleted(request, response, "Cancel");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the Cancel service.
        /// </summary>
        public IAsyncResult BeginCancel(
            RequestHeader requestHeader,
            uint          requestHandle,
            AsyncCallback callback,
            object        asyncState)
        {
            CancelRequest request = new CancelRequest();

            request.RequestHeader = requestHeader;
            request.RequestHandle = requestHandle;

            UpdateRequestHeader(request, requestHeader == null, "Cancel");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginCancel(new CancelMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the Cancel service.
        /// </summary>
        public ResponseHeader EndCancel(
            IAsyncResult result,
            out uint cancelCount)
        {
            CancelResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (CancelResponse)genericResponse;
                }
                else
                {
                    CancelResponseMessage responseMessage = InnerChannel.EndCancel(result);

                    if (responseMessage == null || responseMessage.CancelResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.CancelResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                cancelCount = response.CancelCount;
            }
            finally
            {
                RequestCompleted(null, response, "Cancel");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region AddNodes Methods
        #if (!OPCUA_EXCLUDE_AddNodes)
        /// <summary>
        /// Invokes the AddNodes service.
        /// </summary>
        public virtual ResponseHeader AddNodes(
            RequestHeader                requestHeader,
            AddNodesItemCollection       nodesToAdd,
            out AddNodesResultCollection results,
            out DiagnosticInfoCollection diagnosticInfos)
        {
            AddNodesRequest request = new AddNodesRequest();
            AddNodesResponse response = null;

            request.RequestHeader = requestHeader;
            request.NodesToAdd    = nodesToAdd;

            UpdateRequestHeader(request, requestHeader == null, "AddNodes");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (AddNodesResponse)genericResponse;
                }
                else
                {
                    AddNodesResponseMessage responseMessage = InnerChannel.AddNodes(new AddNodesMessage(request));

                    if (responseMessage == null || responseMessage.AddNodesResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.AddNodesResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(request, response, "AddNodes");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the AddNodes service.
        /// </summary>
        public IAsyncResult BeginAddNodes(
            RequestHeader          requestHeader,
            AddNodesItemCollection nodesToAdd,
            AsyncCallback          callback,
            object                 asyncState)
        {
            AddNodesRequest request = new AddNodesRequest();

            request.RequestHeader = requestHeader;
            request.NodesToAdd    = nodesToAdd;

            UpdateRequestHeader(request, requestHeader == null, "AddNodes");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginAddNodes(new AddNodesMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the AddNodes service.
        /// </summary>
        public ResponseHeader EndAddNodes(
            IAsyncResult                 result,
            out AddNodesResultCollection results,
            out DiagnosticInfoCollection diagnosticInfos)
        {
            AddNodesResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (AddNodesResponse)genericResponse;
                }
                else
                {
                    AddNodesResponseMessage responseMessage = InnerChannel.EndAddNodes(result);

                    if (responseMessage == null || responseMessage.AddNodesResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.AddNodesResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(null, response, "AddNodes");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region AddReferences Methods
        #if (!OPCUA_EXCLUDE_AddReferences)
        /// <summary>
        /// Invokes the AddReferences service.
        /// </summary>
        public virtual ResponseHeader AddReferences(
            RequestHeader                requestHeader,
            AddReferencesItemCollection  referencesToAdd,
            out StatusCodeCollection     results,
            out DiagnosticInfoCollection diagnosticInfos)
        {
            AddReferencesRequest request = new AddReferencesRequest();
            AddReferencesResponse response = null;

            request.RequestHeader   = requestHeader;
            request.ReferencesToAdd = referencesToAdd;

            UpdateRequestHeader(request, requestHeader == null, "AddReferences");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (AddReferencesResponse)genericResponse;
                }
                else
                {
                    AddReferencesResponseMessage responseMessage = InnerChannel.AddReferences(new AddReferencesMessage(request));

                    if (responseMessage == null || responseMessage.AddReferencesResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.AddReferencesResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(request, response, "AddReferences");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the AddReferences service.
        /// </summary>
        public IAsyncResult BeginAddReferences(
            RequestHeader               requestHeader,
            AddReferencesItemCollection referencesToAdd,
            AsyncCallback               callback,
            object                      asyncState)
        {
            AddReferencesRequest request = new AddReferencesRequest();

            request.RequestHeader   = requestHeader;
            request.ReferencesToAdd = referencesToAdd;

            UpdateRequestHeader(request, requestHeader == null, "AddReferences");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginAddReferences(new AddReferencesMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the AddReferences service.
        /// </summary>
        public ResponseHeader EndAddReferences(
            IAsyncResult                 result,
            out StatusCodeCollection     results,
            out DiagnosticInfoCollection diagnosticInfos)
        {
            AddReferencesResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (AddReferencesResponse)genericResponse;
                }
                else
                {
                    AddReferencesResponseMessage responseMessage = InnerChannel.EndAddReferences(result);

                    if (responseMessage == null || responseMessage.AddReferencesResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.AddReferencesResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(null, response, "AddReferences");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region DeleteNodes Methods
        #if (!OPCUA_EXCLUDE_DeleteNodes)
        /// <summary>
        /// Invokes the DeleteNodes service.
        /// </summary>
        public virtual ResponseHeader DeleteNodes(
            RequestHeader                requestHeader,
            DeleteNodesItemCollection    nodesToDelete,
            out StatusCodeCollection     results,
            out DiagnosticInfoCollection diagnosticInfos)
        {
            DeleteNodesRequest request = new DeleteNodesRequest();
            DeleteNodesResponse response = null;

            request.RequestHeader = requestHeader;
            request.NodesToDelete = nodesToDelete;

            UpdateRequestHeader(request, requestHeader == null, "DeleteNodes");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (DeleteNodesResponse)genericResponse;
                }
                else
                {
                    DeleteNodesResponseMessage responseMessage = InnerChannel.DeleteNodes(new DeleteNodesMessage(request));

                    if (responseMessage == null || responseMessage.DeleteNodesResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.DeleteNodesResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(request, response, "DeleteNodes");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the DeleteNodes service.
        /// </summary>
        public IAsyncResult BeginDeleteNodes(
            RequestHeader             requestHeader,
            DeleteNodesItemCollection nodesToDelete,
            AsyncCallback             callback,
            object                    asyncState)
        {
            DeleteNodesRequest request = new DeleteNodesRequest();

            request.RequestHeader = requestHeader;
            request.NodesToDelete = nodesToDelete;

            UpdateRequestHeader(request, requestHeader == null, "DeleteNodes");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginDeleteNodes(new DeleteNodesMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the DeleteNodes service.
        /// </summary>
        public ResponseHeader EndDeleteNodes(
            IAsyncResult                 result,
            out StatusCodeCollection     results,
            out DiagnosticInfoCollection diagnosticInfos)
        {
            DeleteNodesResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (DeleteNodesResponse)genericResponse;
                }
                else
                {
                    DeleteNodesResponseMessage responseMessage = InnerChannel.EndDeleteNodes(result);

                    if (responseMessage == null || responseMessage.DeleteNodesResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.DeleteNodesResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(null, response, "DeleteNodes");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region DeleteReferences Methods
        #if (!OPCUA_EXCLUDE_DeleteReferences)
        /// <summary>
        /// Invokes the DeleteReferences service.
        /// </summary>
        public virtual ResponseHeader DeleteReferences(
            RequestHeader                  requestHeader,
            DeleteReferencesItemCollection referencesToDelete,
            out StatusCodeCollection       results,
            out DiagnosticInfoCollection   diagnosticInfos)
        {
            DeleteReferencesRequest request = new DeleteReferencesRequest();
            DeleteReferencesResponse response = null;

            request.RequestHeader      = requestHeader;
            request.ReferencesToDelete = referencesToDelete;

            UpdateRequestHeader(request, requestHeader == null, "DeleteReferences");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (DeleteReferencesResponse)genericResponse;
                }
                else
                {
                    DeleteReferencesResponseMessage responseMessage = InnerChannel.DeleteReferences(new DeleteReferencesMessage(request));

                    if (responseMessage == null || responseMessage.DeleteReferencesResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.DeleteReferencesResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(request, response, "DeleteReferences");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the DeleteReferences service.
        /// </summary>
        public IAsyncResult BeginDeleteReferences(
            RequestHeader                  requestHeader,
            DeleteReferencesItemCollection referencesToDelete,
            AsyncCallback                  callback,
            object                         asyncState)
        {
            DeleteReferencesRequest request = new DeleteReferencesRequest();

            request.RequestHeader      = requestHeader;
            request.ReferencesToDelete = referencesToDelete;

            UpdateRequestHeader(request, requestHeader == null, "DeleteReferences");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginDeleteReferences(new DeleteReferencesMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the DeleteReferences service.
        /// </summary>
        public ResponseHeader EndDeleteReferences(
            IAsyncResult                 result,
            out StatusCodeCollection     results,
            out DiagnosticInfoCollection diagnosticInfos)
        {
            DeleteReferencesResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (DeleteReferencesResponse)genericResponse;
                }
                else
                {
                    DeleteReferencesResponseMessage responseMessage = InnerChannel.EndDeleteReferences(result);

                    if (responseMessage == null || responseMessage.DeleteReferencesResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.DeleteReferencesResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(null, response, "DeleteReferences");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region Browse Methods
        #if (!OPCUA_EXCLUDE_Browse)
        /// <summary>
        /// Invokes the Browse service.
        /// </summary>
        public virtual ResponseHeader Browse(
            RequestHeader                requestHeader,
            ViewDescription              view,
            uint                         requestedMaxReferencesPerNode,
            BrowseDescriptionCollection  nodesToBrowse,
            out BrowseResultCollection   results,
            out DiagnosticInfoCollection diagnosticInfos)
        {
            BrowseRequest request = new BrowseRequest();
            BrowseResponse response = null;

            request.RequestHeader                 = requestHeader;
            request.View                          = view;
            request.RequestedMaxReferencesPerNode = requestedMaxReferencesPerNode;
            request.NodesToBrowse                 = nodesToBrowse;

            UpdateRequestHeader(request, requestHeader == null, "Browse");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (BrowseResponse)genericResponse;
                }
                else
                {
                    BrowseResponseMessage responseMessage = InnerChannel.Browse(new BrowseMessage(request));

                    if (responseMessage == null || responseMessage.BrowseResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.BrowseResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(request, response, "Browse");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the Browse service.
        /// </summary>
        public IAsyncResult BeginBrowse(
            RequestHeader               requestHeader,
            ViewDescription             view,
            uint                        requestedMaxReferencesPerNode,
            BrowseDescriptionCollection nodesToBrowse,
            AsyncCallback               callback,
            object                      asyncState)
        {
            BrowseRequest request = new BrowseRequest();

            request.RequestHeader                 = requestHeader;
            request.View                          = view;
            request.RequestedMaxReferencesPerNode = requestedMaxReferencesPerNode;
            request.NodesToBrowse                 = nodesToBrowse;

            UpdateRequestHeader(request, requestHeader == null, "Browse");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginBrowse(new BrowseMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the Browse service.
        /// </summary>
        public ResponseHeader EndBrowse(
            IAsyncResult                 result,
            out BrowseResultCollection   results,
            out DiagnosticInfoCollection diagnosticInfos)
        {
            BrowseResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (BrowseResponse)genericResponse;
                }
                else
                {
                    BrowseResponseMessage responseMessage = InnerChannel.EndBrowse(result);

                    if (responseMessage == null || responseMessage.BrowseResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.BrowseResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(null, response, "Browse");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region BrowseNext Methods
        #if (!OPCUA_EXCLUDE_BrowseNext)
        /// <summary>
        /// Invokes the BrowseNext service.
        /// </summary>
        public virtual ResponseHeader BrowseNext(
            RequestHeader                requestHeader,
            bool                         releaseContinuationPoints,
            ByteStringCollection         continuationPoints,
            out BrowseResultCollection   results,
            out DiagnosticInfoCollection diagnosticInfos)
        {
            BrowseNextRequest request = new BrowseNextRequest();
            BrowseNextResponse response = null;

            request.RequestHeader             = requestHeader;
            request.ReleaseContinuationPoints = releaseContinuationPoints;
            request.ContinuationPoints        = continuationPoints;

            UpdateRequestHeader(request, requestHeader == null, "BrowseNext");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (BrowseNextResponse)genericResponse;
                }
                else
                {
                    BrowseNextResponseMessage responseMessage = InnerChannel.BrowseNext(new BrowseNextMessage(request));

                    if (responseMessage == null || responseMessage.BrowseNextResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.BrowseNextResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(request, response, "BrowseNext");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the BrowseNext service.
        /// </summary>
        public IAsyncResult BeginBrowseNext(
            RequestHeader        requestHeader,
            bool                 releaseContinuationPoints,
            ByteStringCollection continuationPoints,
            AsyncCallback        callback,
            object               asyncState)
        {
            BrowseNextRequest request = new BrowseNextRequest();

            request.RequestHeader             = requestHeader;
            request.ReleaseContinuationPoints = releaseContinuationPoints;
            request.ContinuationPoints        = continuationPoints;

            UpdateRequestHeader(request, requestHeader == null, "BrowseNext");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginBrowseNext(new BrowseNextMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the BrowseNext service.
        /// </summary>
        public ResponseHeader EndBrowseNext(
            IAsyncResult                 result,
            out BrowseResultCollection   results,
            out DiagnosticInfoCollection diagnosticInfos)
        {
            BrowseNextResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (BrowseNextResponse)genericResponse;
                }
                else
                {
                    BrowseNextResponseMessage responseMessage = InnerChannel.EndBrowseNext(result);

                    if (responseMessage == null || responseMessage.BrowseNextResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.BrowseNextResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(null, response, "BrowseNext");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region TranslateBrowsePathsToNodeIds Methods
        #if (!OPCUA_EXCLUDE_TranslateBrowsePathsToNodeIds)
        /// <summary>
        /// Invokes the TranslateBrowsePathsToNodeIds service.
        /// </summary>
        public virtual ResponseHeader TranslateBrowsePathsToNodeIds(
            RequestHeader                  requestHeader,
            BrowsePathCollection           browsePaths,
            out BrowsePathResultCollection results,
            out DiagnosticInfoCollection   diagnosticInfos)
        {
            TranslateBrowsePathsToNodeIdsRequest request = new TranslateBrowsePathsToNodeIdsRequest();
            TranslateBrowsePathsToNodeIdsResponse response = null;

            request.RequestHeader = requestHeader;
            request.BrowsePaths   = browsePaths;

            UpdateRequestHeader(request, requestHeader == null, "TranslateBrowsePathsToNodeIds");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (TranslateBrowsePathsToNodeIdsResponse)genericResponse;
                }
                else
                {
                    TranslateBrowsePathsToNodeIdsResponseMessage responseMessage = InnerChannel.TranslateBrowsePathsToNodeIds(new TranslateBrowsePathsToNodeIdsMessage(request));

                    if (responseMessage == null || responseMessage.TranslateBrowsePathsToNodeIdsResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.TranslateBrowsePathsToNodeIdsResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(request, response, "TranslateBrowsePathsToNodeIds");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the TranslateBrowsePathsToNodeIds service.
        /// </summary>
        public IAsyncResult BeginTranslateBrowsePathsToNodeIds(
            RequestHeader        requestHeader,
            BrowsePathCollection browsePaths,
            AsyncCallback        callback,
            object               asyncState)
        {
            TranslateBrowsePathsToNodeIdsRequest request = new TranslateBrowsePathsToNodeIdsRequest();

            request.RequestHeader = requestHeader;
            request.BrowsePaths   = browsePaths;

            UpdateRequestHeader(request, requestHeader == null, "TranslateBrowsePathsToNodeIds");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginTranslateBrowsePathsToNodeIds(new TranslateBrowsePathsToNodeIdsMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the TranslateBrowsePathsToNodeIds service.
        /// </summary>
        public ResponseHeader EndTranslateBrowsePathsToNodeIds(
            IAsyncResult                   result,
            out BrowsePathResultCollection results,
            out DiagnosticInfoCollection   diagnosticInfos)
        {
            TranslateBrowsePathsToNodeIdsResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (TranslateBrowsePathsToNodeIdsResponse)genericResponse;
                }
                else
                {
                    TranslateBrowsePathsToNodeIdsResponseMessage responseMessage = InnerChannel.EndTranslateBrowsePathsToNodeIds(result);

                    if (responseMessage == null || responseMessage.TranslateBrowsePathsToNodeIdsResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.TranslateBrowsePathsToNodeIdsResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(null, response, "TranslateBrowsePathsToNodeIds");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region RegisterNodes Methods
        #if (!OPCUA_EXCLUDE_RegisterNodes)
        /// <summary>
        /// Invokes the RegisterNodes service.
        /// </summary>
        public virtual ResponseHeader RegisterNodes(
            RequestHeader        requestHeader,
            NodeIdCollection     nodesToRegister,
            out NodeIdCollection registeredNodeIds)
        {
            RegisterNodesRequest request = new RegisterNodesRequest();
            RegisterNodesResponse response = null;

            request.RequestHeader   = requestHeader;
            request.NodesToRegister = nodesToRegister;

            UpdateRequestHeader(request, requestHeader == null, "RegisterNodes");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (RegisterNodesResponse)genericResponse;
                }
                else
                {
                    RegisterNodesResponseMessage responseMessage = InnerChannel.RegisterNodes(new RegisterNodesMessage(request));

                    if (responseMessage == null || responseMessage.RegisterNodesResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.RegisterNodesResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                registeredNodeIds = response.RegisteredNodeIds;
            }
            finally
            {
                RequestCompleted(request, response, "RegisterNodes");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the RegisterNodes service.
        /// </summary>
        public IAsyncResult BeginRegisterNodes(
            RequestHeader    requestHeader,
            NodeIdCollection nodesToRegister,
            AsyncCallback    callback,
            object           asyncState)
        {
            RegisterNodesRequest request = new RegisterNodesRequest();

            request.RequestHeader   = requestHeader;
            request.NodesToRegister = nodesToRegister;

            UpdateRequestHeader(request, requestHeader == null, "RegisterNodes");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginRegisterNodes(new RegisterNodesMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the RegisterNodes service.
        /// </summary>
        public ResponseHeader EndRegisterNodes(
            IAsyncResult         result,
            out NodeIdCollection registeredNodeIds)
        {
            RegisterNodesResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (RegisterNodesResponse)genericResponse;
                }
                else
                {
                    RegisterNodesResponseMessage responseMessage = InnerChannel.EndRegisterNodes(result);

                    if (responseMessage == null || responseMessage.RegisterNodesResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.RegisterNodesResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                registeredNodeIds = response.RegisteredNodeIds;
            }
            finally
            {
                RequestCompleted(null, response, "RegisterNodes");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region UnregisterNodes Methods
        #if (!OPCUA_EXCLUDE_UnregisterNodes)
        /// <summary>
        /// Invokes the UnregisterNodes service.
        /// </summary>
        public virtual ResponseHeader UnregisterNodes(
            RequestHeader    requestHeader,
            NodeIdCollection nodesToUnregister)
        {
            UnregisterNodesRequest request = new UnregisterNodesRequest();
            UnregisterNodesResponse response = null;

            request.RequestHeader     = requestHeader;
            request.NodesToUnregister = nodesToUnregister;

            UpdateRequestHeader(request, requestHeader == null, "UnregisterNodes");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (UnregisterNodesResponse)genericResponse;
                }
                else
                {
                    UnregisterNodesResponseMessage responseMessage = InnerChannel.UnregisterNodes(new UnregisterNodesMessage(request));

                    if (responseMessage == null || responseMessage.UnregisterNodesResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.UnregisterNodesResponse;
                    ValidateResponse(response.ResponseHeader);
                }

            }
            finally
            {
                RequestCompleted(request, response, "UnregisterNodes");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the UnregisterNodes service.
        /// </summary>
        public IAsyncResult BeginUnregisterNodes(
            RequestHeader    requestHeader,
            NodeIdCollection nodesToUnregister,
            AsyncCallback    callback,
            object           asyncState)
        {
            UnregisterNodesRequest request = new UnregisterNodesRequest();

            request.RequestHeader     = requestHeader;
            request.NodesToUnregister = nodesToUnregister;

            UpdateRequestHeader(request, requestHeader == null, "UnregisterNodes");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginUnregisterNodes(new UnregisterNodesMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the UnregisterNodes service.
        /// </summary>
        public ResponseHeader EndUnregisterNodes(
            IAsyncResult result)
        {
            UnregisterNodesResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (UnregisterNodesResponse)genericResponse;
                }
                else
                {
                    UnregisterNodesResponseMessage responseMessage = InnerChannel.EndUnregisterNodes(result);

                    if (responseMessage == null || responseMessage.UnregisterNodesResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.UnregisterNodesResponse;
                    ValidateResponse(response.ResponseHeader);
                }

            }
            finally
            {
                RequestCompleted(null, response, "UnregisterNodes");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region QueryFirst Methods
        #if (!OPCUA_EXCLUDE_QueryFirst)
        /// <summary>
        /// Invokes the QueryFirst service.
        /// </summary>
        public virtual ResponseHeader QueryFirst(
            RequestHeader                 requestHeader,
            ViewDescription               view,
            NodeTypeDescriptionCollection nodeTypes,
            ContentFilter                 filter,
            uint                          maxDataSetsToReturn,
            uint                          maxReferencesToReturn,
            out QueryDataSetCollection    queryDataSets,
            out byte[]                    continuationPoint,
            out ParsingResultCollection   parsingResults,
            out DiagnosticInfoCollection  diagnosticInfos,
            out ContentFilterResult       filterResult)
        {
            QueryFirstRequest request = new QueryFirstRequest();
            QueryFirstResponse response = null;

            request.RequestHeader         = requestHeader;
            request.View                  = view;
            request.NodeTypes             = nodeTypes;
            request.Filter                = filter;
            request.MaxDataSetsToReturn   = maxDataSetsToReturn;
            request.MaxReferencesToReturn = maxReferencesToReturn;

            UpdateRequestHeader(request, requestHeader == null, "QueryFirst");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (QueryFirstResponse)genericResponse;
                }
                else
                {
                    QueryFirstResponseMessage responseMessage = InnerChannel.QueryFirst(new QueryFirstMessage(request));

                    if (responseMessage == null || responseMessage.QueryFirstResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.QueryFirstResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                queryDataSets     = response.QueryDataSets;
                continuationPoint = response.ContinuationPoint;
                parsingResults    = response.ParsingResults;
                diagnosticInfos   = response.DiagnosticInfos;
                filterResult      = response.FilterResult;
            }
            finally
            {
                RequestCompleted(request, response, "QueryFirst");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the QueryFirst service.
        /// </summary>
        public IAsyncResult BeginQueryFirst(
            RequestHeader                 requestHeader,
            ViewDescription               view,
            NodeTypeDescriptionCollection nodeTypes,
            ContentFilter                 filter,
            uint                          maxDataSetsToReturn,
            uint                          maxReferencesToReturn,
            AsyncCallback                 callback,
            object                        asyncState)
        {
            QueryFirstRequest request = new QueryFirstRequest();

            request.RequestHeader         = requestHeader;
            request.View                  = view;
            request.NodeTypes             = nodeTypes;
            request.Filter                = filter;
            request.MaxDataSetsToReturn   = maxDataSetsToReturn;
            request.MaxReferencesToReturn = maxReferencesToReturn;

            UpdateRequestHeader(request, requestHeader == null, "QueryFirst");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginQueryFirst(new QueryFirstMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the QueryFirst service.
        /// </summary>
        public ResponseHeader EndQueryFirst(
            IAsyncResult                 result,
            out QueryDataSetCollection   queryDataSets,
            out byte[]                   continuationPoint,
            out ParsingResultCollection  parsingResults,
            out DiagnosticInfoCollection diagnosticInfos,
            out ContentFilterResult      filterResult)
        {
            QueryFirstResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (QueryFirstResponse)genericResponse;
                }
                else
                {
                    QueryFirstResponseMessage responseMessage = InnerChannel.EndQueryFirst(result);

                    if (responseMessage == null || responseMessage.QueryFirstResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.QueryFirstResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                queryDataSets     = response.QueryDataSets;
                continuationPoint = response.ContinuationPoint;
                parsingResults    = response.ParsingResults;
                diagnosticInfos   = response.DiagnosticInfos;
                filterResult      = response.FilterResult;
            }
            finally
            {
                RequestCompleted(null, response, "QueryFirst");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region QueryNext Methods
        #if (!OPCUA_EXCLUDE_QueryNext)
        /// <summary>
        /// Invokes the QueryNext service.
        /// </summary>
        public virtual ResponseHeader QueryNext(
            RequestHeader              requestHeader,
            bool                       releaseContinuationPoint,
            byte[]                     continuationPoint,
            out QueryDataSetCollection queryDataSets,
            out byte[]                 revisedContinuationPoint)
        {
            QueryNextRequest request = new QueryNextRequest();
            QueryNextResponse response = null;

            request.RequestHeader            = requestHeader;
            request.ReleaseContinuationPoint = releaseContinuationPoint;
            request.ContinuationPoint        = continuationPoint;

            UpdateRequestHeader(request, requestHeader == null, "QueryNext");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (QueryNextResponse)genericResponse;
                }
                else
                {
                    QueryNextResponseMessage responseMessage = InnerChannel.QueryNext(new QueryNextMessage(request));

                    if (responseMessage == null || responseMessage.QueryNextResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.QueryNextResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                queryDataSets            = response.QueryDataSets;
                revisedContinuationPoint = response.RevisedContinuationPoint;
            }
            finally
            {
                RequestCompleted(request, response, "QueryNext");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the QueryNext service.
        /// </summary>
        public IAsyncResult BeginQueryNext(
            RequestHeader requestHeader,
            bool          releaseContinuationPoint,
            byte[]        continuationPoint,
            AsyncCallback callback,
            object        asyncState)
        {
            QueryNextRequest request = new QueryNextRequest();

            request.RequestHeader            = requestHeader;
            request.ReleaseContinuationPoint = releaseContinuationPoint;
            request.ContinuationPoint        = continuationPoint;

            UpdateRequestHeader(request, requestHeader == null, "QueryNext");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginQueryNext(new QueryNextMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the QueryNext service.
        /// </summary>
        public ResponseHeader EndQueryNext(
            IAsyncResult               result,
            out QueryDataSetCollection queryDataSets,
            out byte[]                 revisedContinuationPoint)
        {
            QueryNextResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (QueryNextResponse)genericResponse;
                }
                else
                {
                    QueryNextResponseMessage responseMessage = InnerChannel.EndQueryNext(result);

                    if (responseMessage == null || responseMessage.QueryNextResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.QueryNextResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                queryDataSets            = response.QueryDataSets;
                revisedContinuationPoint = response.RevisedContinuationPoint;
            }
            finally
            {
                RequestCompleted(null, response, "QueryNext");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region Read Methods
        #if (!OPCUA_EXCLUDE_Read)
        /// <summary>
        /// Invokes the Read service.
        /// </summary>
        public virtual ResponseHeader Read(
            RequestHeader                requestHeader,
            double                       maxAge,
            TimestampsToReturn           timestampsToReturn,
            ReadValueIdCollection        nodesToRead,
            out DataValueCollection      results,
            out DiagnosticInfoCollection diagnosticInfos)
        {
            ReadRequest request = new ReadRequest();
            ReadResponse response = null;

            request.RequestHeader      = requestHeader;
            request.MaxAge             = maxAge;
            request.TimestampsToReturn = timestampsToReturn;
            request.NodesToRead        = nodesToRead;

            UpdateRequestHeader(request, requestHeader == null, "Read");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (ReadResponse)genericResponse;
                }
                else
                {
                    ReadResponseMessage responseMessage = InnerChannel.Read(new ReadMessage(request));

                    if (responseMessage == null || responseMessage.ReadResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.ReadResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(request, response, "Read");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the Read service.
        /// </summary>
        public IAsyncResult BeginRead(
            RequestHeader         requestHeader,
            double                maxAge,
            TimestampsToReturn    timestampsToReturn,
            ReadValueIdCollection nodesToRead,
            AsyncCallback         callback,
            object                asyncState)
        {
            ReadRequest request = new ReadRequest();

            request.RequestHeader      = requestHeader;
            request.MaxAge             = maxAge;
            request.TimestampsToReturn = timestampsToReturn;
            request.NodesToRead        = nodesToRead;

            UpdateRequestHeader(request, requestHeader == null, "Read");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginRead(new ReadMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the Read service.
        /// </summary>
        public ResponseHeader EndRead(
            IAsyncResult                 result,
            out DataValueCollection      results,
            out DiagnosticInfoCollection diagnosticInfos)
        {
            ReadResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (ReadResponse)genericResponse;
                }
                else
                {
                    ReadResponseMessage responseMessage = InnerChannel.EndRead(result);

                    if (responseMessage == null || responseMessage.ReadResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.ReadResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(null, response, "Read");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region HistoryRead Methods
        #if (!OPCUA_EXCLUDE_HistoryRead)
        /// <summary>
        /// Invokes the HistoryRead service.
        /// </summary>
        public virtual ResponseHeader HistoryRead(
            RequestHeader                   requestHeader,
            ExtensionObject                 historyReadDetails,
            TimestampsToReturn              timestampsToReturn,
            bool                            releaseContinuationPoints,
            HistoryReadValueIdCollection    nodesToRead,
            out HistoryReadResultCollection results,
            out DiagnosticInfoCollection    diagnosticInfos)
        {
            HistoryReadRequest request = new HistoryReadRequest();
            HistoryReadResponse response = null;

            request.RequestHeader             = requestHeader;
            request.HistoryReadDetails        = historyReadDetails;
            request.TimestampsToReturn        = timestampsToReturn;
            request.ReleaseContinuationPoints = releaseContinuationPoints;
            request.NodesToRead               = nodesToRead;

            UpdateRequestHeader(request, requestHeader == null, "HistoryRead");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (HistoryReadResponse)genericResponse;
                }
                else
                {
                    HistoryReadResponseMessage responseMessage = InnerChannel.HistoryRead(new HistoryReadMessage(request));

                    if (responseMessage == null || responseMessage.HistoryReadResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.HistoryReadResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(request, response, "HistoryRead");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the HistoryRead service.
        /// </summary>
        public IAsyncResult BeginHistoryRead(
            RequestHeader                requestHeader,
            ExtensionObject              historyReadDetails,
            TimestampsToReturn           timestampsToReturn,
            bool                         releaseContinuationPoints,
            HistoryReadValueIdCollection nodesToRead,
            AsyncCallback                callback,
            object                       asyncState)
        {
            HistoryReadRequest request = new HistoryReadRequest();

            request.RequestHeader             = requestHeader;
            request.HistoryReadDetails        = historyReadDetails;
            request.TimestampsToReturn        = timestampsToReturn;
            request.ReleaseContinuationPoints = releaseContinuationPoints;
            request.NodesToRead               = nodesToRead;

            UpdateRequestHeader(request, requestHeader == null, "HistoryRead");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginHistoryRead(new HistoryReadMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the HistoryRead service.
        /// </summary>
        public ResponseHeader EndHistoryRead(
            IAsyncResult                    result,
            out HistoryReadResultCollection results,
            out DiagnosticInfoCollection    diagnosticInfos)
        {
            HistoryReadResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (HistoryReadResponse)genericResponse;
                }
                else
                {
                    HistoryReadResponseMessage responseMessage = InnerChannel.EndHistoryRead(result);

                    if (responseMessage == null || responseMessage.HistoryReadResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.HistoryReadResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(null, response, "HistoryRead");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region Write Methods
        #if (!OPCUA_EXCLUDE_Write)
        /// <summary>
        /// Invokes the Write service.
        /// </summary>
        public virtual ResponseHeader Write(
            RequestHeader                requestHeader,
            WriteValueCollection         nodesToWrite,
            out StatusCodeCollection     results,
            out DiagnosticInfoCollection diagnosticInfos)
        {
            WriteRequest request = new WriteRequest();
            WriteResponse response = null;

            request.RequestHeader = requestHeader;
            request.NodesToWrite  = nodesToWrite;

            UpdateRequestHeader(request, requestHeader == null, "Write");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (WriteResponse)genericResponse;
                }
                else
                {
                    WriteResponseMessage responseMessage = InnerChannel.Write(new WriteMessage(request));

                    if (responseMessage == null || responseMessage.WriteResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.WriteResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(request, response, "Write");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the Write service.
        /// </summary>
        public IAsyncResult BeginWrite(
            RequestHeader        requestHeader,
            WriteValueCollection nodesToWrite,
            AsyncCallback        callback,
            object               asyncState)
        {
            WriteRequest request = new WriteRequest();

            request.RequestHeader = requestHeader;
            request.NodesToWrite  = nodesToWrite;

            UpdateRequestHeader(request, requestHeader == null, "Write");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginWrite(new WriteMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the Write service.
        /// </summary>
        public ResponseHeader EndWrite(
            IAsyncResult                 result,
            out StatusCodeCollection     results,
            out DiagnosticInfoCollection diagnosticInfos)
        {
            WriteResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (WriteResponse)genericResponse;
                }
                else
                {
                    WriteResponseMessage responseMessage = InnerChannel.EndWrite(result);

                    if (responseMessage == null || responseMessage.WriteResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.WriteResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(null, response, "Write");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region HistoryUpdate Methods
        #if (!OPCUA_EXCLUDE_HistoryUpdate)
        /// <summary>
        /// Invokes the HistoryUpdate service.
        /// </summary>
        public virtual ResponseHeader HistoryUpdate(
            RequestHeader                     requestHeader,
            ExtensionObjectCollection         historyUpdateDetails,
            out HistoryUpdateResultCollection results,
            out DiagnosticInfoCollection      diagnosticInfos)
        {
            HistoryUpdateRequest request = new HistoryUpdateRequest();
            HistoryUpdateResponse response = null;

            request.RequestHeader        = requestHeader;
            request.HistoryUpdateDetails = historyUpdateDetails;

            UpdateRequestHeader(request, requestHeader == null, "HistoryUpdate");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (HistoryUpdateResponse)genericResponse;
                }
                else
                {
                    HistoryUpdateResponseMessage responseMessage = InnerChannel.HistoryUpdate(new HistoryUpdateMessage(request));

                    if (responseMessage == null || responseMessage.HistoryUpdateResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.HistoryUpdateResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(request, response, "HistoryUpdate");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the HistoryUpdate service.
        /// </summary>
        public IAsyncResult BeginHistoryUpdate(
            RequestHeader             requestHeader,
            ExtensionObjectCollection historyUpdateDetails,
            AsyncCallback             callback,
            object                    asyncState)
        {
            HistoryUpdateRequest request = new HistoryUpdateRequest();

            request.RequestHeader        = requestHeader;
            request.HistoryUpdateDetails = historyUpdateDetails;

            UpdateRequestHeader(request, requestHeader == null, "HistoryUpdate");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginHistoryUpdate(new HistoryUpdateMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the HistoryUpdate service.
        /// </summary>
        public ResponseHeader EndHistoryUpdate(
            IAsyncResult                      result,
            out HistoryUpdateResultCollection results,
            out DiagnosticInfoCollection      diagnosticInfos)
        {
            HistoryUpdateResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (HistoryUpdateResponse)genericResponse;
                }
                else
                {
                    HistoryUpdateResponseMessage responseMessage = InnerChannel.EndHistoryUpdate(result);

                    if (responseMessage == null || responseMessage.HistoryUpdateResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.HistoryUpdateResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(null, response, "HistoryUpdate");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region Call Methods
        #if (!OPCUA_EXCLUDE_Call)
        /// <summary>
        /// Invokes the Call service.
        /// </summary>
        public virtual ResponseHeader Call(
            RequestHeader                  requestHeader,
            CallMethodRequestCollection    methodsToCall,
            out CallMethodResultCollection results,
            out DiagnosticInfoCollection   diagnosticInfos)
        {
            CallRequest request = new CallRequest();
            CallResponse response = null;

            request.RequestHeader = requestHeader;
            request.MethodsToCall = methodsToCall;

            UpdateRequestHeader(request, requestHeader == null, "Call");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (CallResponse)genericResponse;
                }
                else
                {
                    CallResponseMessage responseMessage = InnerChannel.Call(new CallMessage(request));

                    if (responseMessage == null || responseMessage.CallResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.CallResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(request, response, "Call");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the Call service.
        /// </summary>
        public IAsyncResult BeginCall(
            RequestHeader               requestHeader,
            CallMethodRequestCollection methodsToCall,
            AsyncCallback               callback,
            object                      asyncState)
        {
            CallRequest request = new CallRequest();

            request.RequestHeader = requestHeader;
            request.MethodsToCall = methodsToCall;

            UpdateRequestHeader(request, requestHeader == null, "Call");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginCall(new CallMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the Call service.
        /// </summary>
        public ResponseHeader EndCall(
            IAsyncResult                   result,
            out CallMethodResultCollection results,
            out DiagnosticInfoCollection   diagnosticInfos)
        {
            CallResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (CallResponse)genericResponse;
                }
                else
                {
                    CallResponseMessage responseMessage = InnerChannel.EndCall(result);

                    if (responseMessage == null || responseMessage.CallResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.CallResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(null, response, "Call");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region CreateMonitoredItems Methods
        #if (!OPCUA_EXCLUDE_CreateMonitoredItems)
        /// <summary>
        /// Invokes the CreateMonitoredItems service.
        /// </summary>
        public virtual ResponseHeader CreateMonitoredItems(
            RequestHeader                           requestHeader,
            uint                                    subscriptionId,
            TimestampsToReturn                      timestampsToReturn,
            MonitoredItemCreateRequestCollection    itemsToCreate,
            out MonitoredItemCreateResultCollection results,
            out DiagnosticInfoCollection            diagnosticInfos)
        {
            CreateMonitoredItemsRequest request = new CreateMonitoredItemsRequest();
            CreateMonitoredItemsResponse response = null;

            request.RequestHeader      = requestHeader;
            request.SubscriptionId     = subscriptionId;
            request.TimestampsToReturn = timestampsToReturn;
            request.ItemsToCreate      = itemsToCreate;

            UpdateRequestHeader(request, requestHeader == null, "CreateMonitoredItems");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (CreateMonitoredItemsResponse)genericResponse;
                }
                else
                {
                    CreateMonitoredItemsResponseMessage responseMessage = InnerChannel.CreateMonitoredItems(new CreateMonitoredItemsMessage(request));

                    if (responseMessage == null || responseMessage.CreateMonitoredItemsResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.CreateMonitoredItemsResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(request, response, "CreateMonitoredItems");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the CreateMonitoredItems service.
        /// </summary>
        public IAsyncResult BeginCreateMonitoredItems(
            RequestHeader                        requestHeader,
            uint                                 subscriptionId,
            TimestampsToReturn                   timestampsToReturn,
            MonitoredItemCreateRequestCollection itemsToCreate,
            AsyncCallback                        callback,
            object                               asyncState)
        {
            CreateMonitoredItemsRequest request = new CreateMonitoredItemsRequest();

            request.RequestHeader      = requestHeader;
            request.SubscriptionId     = subscriptionId;
            request.TimestampsToReturn = timestampsToReturn;
            request.ItemsToCreate      = itemsToCreate;

            UpdateRequestHeader(request, requestHeader == null, "CreateMonitoredItems");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginCreateMonitoredItems(new CreateMonitoredItemsMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the CreateMonitoredItems service.
        /// </summary>
        public ResponseHeader EndCreateMonitoredItems(
            IAsyncResult                            result,
            out MonitoredItemCreateResultCollection results,
            out DiagnosticInfoCollection            diagnosticInfos)
        {
            CreateMonitoredItemsResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (CreateMonitoredItemsResponse)genericResponse;
                }
                else
                {
                    CreateMonitoredItemsResponseMessage responseMessage = InnerChannel.EndCreateMonitoredItems(result);

                    if (responseMessage == null || responseMessage.CreateMonitoredItemsResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.CreateMonitoredItemsResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(null, response, "CreateMonitoredItems");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region ModifyMonitoredItems Methods
        #if (!OPCUA_EXCLUDE_ModifyMonitoredItems)
        /// <summary>
        /// Invokes the ModifyMonitoredItems service.
        /// </summary>
        public virtual ResponseHeader ModifyMonitoredItems(
            RequestHeader                           requestHeader,
            uint                                    subscriptionId,
            TimestampsToReturn                      timestampsToReturn,
            MonitoredItemModifyRequestCollection    itemsToModify,
            out MonitoredItemModifyResultCollection results,
            out DiagnosticInfoCollection            diagnosticInfos)
        {
            ModifyMonitoredItemsRequest request = new ModifyMonitoredItemsRequest();
            ModifyMonitoredItemsResponse response = null;

            request.RequestHeader      = requestHeader;
            request.SubscriptionId     = subscriptionId;
            request.TimestampsToReturn = timestampsToReturn;
            request.ItemsToModify      = itemsToModify;

            UpdateRequestHeader(request, requestHeader == null, "ModifyMonitoredItems");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (ModifyMonitoredItemsResponse)genericResponse;
                }
                else
                {
                    ModifyMonitoredItemsResponseMessage responseMessage = InnerChannel.ModifyMonitoredItems(new ModifyMonitoredItemsMessage(request));

                    if (responseMessage == null || responseMessage.ModifyMonitoredItemsResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.ModifyMonitoredItemsResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(request, response, "ModifyMonitoredItems");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the ModifyMonitoredItems service.
        /// </summary>
        public IAsyncResult BeginModifyMonitoredItems(
            RequestHeader                        requestHeader,
            uint                                 subscriptionId,
            TimestampsToReturn                   timestampsToReturn,
            MonitoredItemModifyRequestCollection itemsToModify,
            AsyncCallback                        callback,
            object                               asyncState)
        {
            ModifyMonitoredItemsRequest request = new ModifyMonitoredItemsRequest();

            request.RequestHeader      = requestHeader;
            request.SubscriptionId     = subscriptionId;
            request.TimestampsToReturn = timestampsToReturn;
            request.ItemsToModify      = itemsToModify;

            UpdateRequestHeader(request, requestHeader == null, "ModifyMonitoredItems");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginModifyMonitoredItems(new ModifyMonitoredItemsMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the ModifyMonitoredItems service.
        /// </summary>
        public ResponseHeader EndModifyMonitoredItems(
            IAsyncResult                            result,
            out MonitoredItemModifyResultCollection results,
            out DiagnosticInfoCollection            diagnosticInfos)
        {
            ModifyMonitoredItemsResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (ModifyMonitoredItemsResponse)genericResponse;
                }
                else
                {
                    ModifyMonitoredItemsResponseMessage responseMessage = InnerChannel.EndModifyMonitoredItems(result);

                    if (responseMessage == null || responseMessage.ModifyMonitoredItemsResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.ModifyMonitoredItemsResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(null, response, "ModifyMonitoredItems");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region SetMonitoringMode Methods
        #if (!OPCUA_EXCLUDE_SetMonitoringMode)
        /// <summary>
        /// Invokes the SetMonitoringMode service.
        /// </summary>
        public virtual ResponseHeader SetMonitoringMode(
            RequestHeader                requestHeader,
            uint                         subscriptionId,
            MonitoringMode               monitoringMode,
            UInt32Collection             monitoredItemIds,
            out StatusCodeCollection     results,
            out DiagnosticInfoCollection diagnosticInfos)
        {
            SetMonitoringModeRequest request = new SetMonitoringModeRequest();
            SetMonitoringModeResponse response = null;

            request.RequestHeader    = requestHeader;
            request.SubscriptionId   = subscriptionId;
            request.MonitoringMode   = monitoringMode;
            request.MonitoredItemIds = monitoredItemIds;

            UpdateRequestHeader(request, requestHeader == null, "SetMonitoringMode");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (SetMonitoringModeResponse)genericResponse;
                }
                else
                {
                    SetMonitoringModeResponseMessage responseMessage = InnerChannel.SetMonitoringMode(new SetMonitoringModeMessage(request));

                    if (responseMessage == null || responseMessage.SetMonitoringModeResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.SetMonitoringModeResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(request, response, "SetMonitoringMode");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the SetMonitoringMode service.
        /// </summary>
        public IAsyncResult BeginSetMonitoringMode(
            RequestHeader    requestHeader,
            uint             subscriptionId,
            MonitoringMode   monitoringMode,
            UInt32Collection monitoredItemIds,
            AsyncCallback    callback,
            object           asyncState)
        {
            SetMonitoringModeRequest request = new SetMonitoringModeRequest();

            request.RequestHeader    = requestHeader;
            request.SubscriptionId   = subscriptionId;
            request.MonitoringMode   = monitoringMode;
            request.MonitoredItemIds = monitoredItemIds;

            UpdateRequestHeader(request, requestHeader == null, "SetMonitoringMode");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginSetMonitoringMode(new SetMonitoringModeMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the SetMonitoringMode service.
        /// </summary>
        public ResponseHeader EndSetMonitoringMode(
            IAsyncResult                 result,
            out StatusCodeCollection     results,
            out DiagnosticInfoCollection diagnosticInfos)
        {
            SetMonitoringModeResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (SetMonitoringModeResponse)genericResponse;
                }
                else
                {
                    SetMonitoringModeResponseMessage responseMessage = InnerChannel.EndSetMonitoringMode(result);

                    if (responseMessage == null || responseMessage.SetMonitoringModeResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.SetMonitoringModeResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(null, response, "SetMonitoringMode");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region SetTriggering Methods
        #if (!OPCUA_EXCLUDE_SetTriggering)
        /// <summary>
        /// Invokes the SetTriggering service.
        /// </summary>
        public virtual ResponseHeader SetTriggering(
            RequestHeader                requestHeader,
            uint                         subscriptionId,
            uint                         triggeringItemId,
            UInt32Collection             linksToAdd,
            UInt32Collection             linksToRemove,
            out StatusCodeCollection     addResults,
            out DiagnosticInfoCollection addDiagnosticInfos,
            out StatusCodeCollection     removeResults,
            out DiagnosticInfoCollection removeDiagnosticInfos)
        {
            SetTriggeringRequest request = new SetTriggeringRequest();
            SetTriggeringResponse response = null;

            request.RequestHeader    = requestHeader;
            request.SubscriptionId   = subscriptionId;
            request.TriggeringItemId = triggeringItemId;
            request.LinksToAdd       = linksToAdd;
            request.LinksToRemove    = linksToRemove;

            UpdateRequestHeader(request, requestHeader == null, "SetTriggering");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (SetTriggeringResponse)genericResponse;
                }
                else
                {
                    SetTriggeringResponseMessage responseMessage = InnerChannel.SetTriggering(new SetTriggeringMessage(request));

                    if (responseMessage == null || responseMessage.SetTriggeringResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.SetTriggeringResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                addResults            = response.AddResults;
                addDiagnosticInfos    = response.AddDiagnosticInfos;
                removeResults         = response.RemoveResults;
                removeDiagnosticInfos = response.RemoveDiagnosticInfos;
            }
            finally
            {
                RequestCompleted(request, response, "SetTriggering");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the SetTriggering service.
        /// </summary>
        public IAsyncResult BeginSetTriggering(
            RequestHeader    requestHeader,
            uint             subscriptionId,
            uint             triggeringItemId,
            UInt32Collection linksToAdd,
            UInt32Collection linksToRemove,
            AsyncCallback    callback,
            object           asyncState)
        {
            SetTriggeringRequest request = new SetTriggeringRequest();

            request.RequestHeader    = requestHeader;
            request.SubscriptionId   = subscriptionId;
            request.TriggeringItemId = triggeringItemId;
            request.LinksToAdd       = linksToAdd;
            request.LinksToRemove    = linksToRemove;

            UpdateRequestHeader(request, requestHeader == null, "SetTriggering");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginSetTriggering(new SetTriggeringMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the SetTriggering service.
        /// </summary>
        public ResponseHeader EndSetTriggering(
            IAsyncResult                 result,
            out StatusCodeCollection     addResults,
            out DiagnosticInfoCollection addDiagnosticInfos,
            out StatusCodeCollection     removeResults,
            out DiagnosticInfoCollection removeDiagnosticInfos)
        {
            SetTriggeringResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (SetTriggeringResponse)genericResponse;
                }
                else
                {
                    SetTriggeringResponseMessage responseMessage = InnerChannel.EndSetTriggering(result);

                    if (responseMessage == null || responseMessage.SetTriggeringResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.SetTriggeringResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                addResults            = response.AddResults;
                addDiagnosticInfos    = response.AddDiagnosticInfos;
                removeResults         = response.RemoveResults;
                removeDiagnosticInfos = response.RemoveDiagnosticInfos;
            }
            finally
            {
                RequestCompleted(null, response, "SetTriggering");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region DeleteMonitoredItems Methods
        #if (!OPCUA_EXCLUDE_DeleteMonitoredItems)
        /// <summary>
        /// Invokes the DeleteMonitoredItems service.
        /// </summary>
        public virtual ResponseHeader DeleteMonitoredItems(
            RequestHeader                requestHeader,
            uint                         subscriptionId,
            UInt32Collection             monitoredItemIds,
            out StatusCodeCollection     results,
            out DiagnosticInfoCollection diagnosticInfos)
        {
            DeleteMonitoredItemsRequest request = new DeleteMonitoredItemsRequest();
            DeleteMonitoredItemsResponse response = null;

            request.RequestHeader    = requestHeader;
            request.SubscriptionId   = subscriptionId;
            request.MonitoredItemIds = monitoredItemIds;

            UpdateRequestHeader(request, requestHeader == null, "DeleteMonitoredItems");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (DeleteMonitoredItemsResponse)genericResponse;
                }
                else
                {
                    DeleteMonitoredItemsResponseMessage responseMessage = InnerChannel.DeleteMonitoredItems(new DeleteMonitoredItemsMessage(request));

                    if (responseMessage == null || responseMessage.DeleteMonitoredItemsResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.DeleteMonitoredItemsResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(request, response, "DeleteMonitoredItems");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the DeleteMonitoredItems service.
        /// </summary>
        public IAsyncResult BeginDeleteMonitoredItems(
            RequestHeader    requestHeader,
            uint             subscriptionId,
            UInt32Collection monitoredItemIds,
            AsyncCallback    callback,
            object           asyncState)
        {
            DeleteMonitoredItemsRequest request = new DeleteMonitoredItemsRequest();

            request.RequestHeader    = requestHeader;
            request.SubscriptionId   = subscriptionId;
            request.MonitoredItemIds = monitoredItemIds;

            UpdateRequestHeader(request, requestHeader == null, "DeleteMonitoredItems");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginDeleteMonitoredItems(new DeleteMonitoredItemsMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the DeleteMonitoredItems service.
        /// </summary>
        public ResponseHeader EndDeleteMonitoredItems(
            IAsyncResult                 result,
            out StatusCodeCollection     results,
            out DiagnosticInfoCollection diagnosticInfos)
        {
            DeleteMonitoredItemsResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (DeleteMonitoredItemsResponse)genericResponse;
                }
                else
                {
                    DeleteMonitoredItemsResponseMessage responseMessage = InnerChannel.EndDeleteMonitoredItems(result);

                    if (responseMessage == null || responseMessage.DeleteMonitoredItemsResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.DeleteMonitoredItemsResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(null, response, "DeleteMonitoredItems");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region CreateSubscription Methods
        #if (!OPCUA_EXCLUDE_CreateSubscription)
        /// <summary>
        /// Invokes the CreateSubscription service.
        /// </summary>
        public virtual ResponseHeader CreateSubscription(
            RequestHeader requestHeader,
            double        requestedPublishingInterval,
            uint          requestedLifetimeCount,
            uint          requestedMaxKeepAliveCount,
            uint          maxNotificationsPerPublish,
            bool          publishingEnabled,
            byte          priority,
            out uint      subscriptionId,
            out double    revisedPublishingInterval,
            out uint      revisedLifetimeCount,
            out uint      revisedMaxKeepAliveCount)
        {
            CreateSubscriptionRequest request = new CreateSubscriptionRequest();
            CreateSubscriptionResponse response = null;

            request.RequestHeader               = requestHeader;
            request.RequestedPublishingInterval = requestedPublishingInterval;
            request.RequestedLifetimeCount      = requestedLifetimeCount;
            request.RequestedMaxKeepAliveCount  = requestedMaxKeepAliveCount;
            request.MaxNotificationsPerPublish  = maxNotificationsPerPublish;
            request.PublishingEnabled           = publishingEnabled;
            request.Priority                    = priority;

            UpdateRequestHeader(request, requestHeader == null, "CreateSubscription");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (CreateSubscriptionResponse)genericResponse;
                }
                else
                {
                    CreateSubscriptionResponseMessage responseMessage = InnerChannel.CreateSubscription(new CreateSubscriptionMessage(request));

                    if (responseMessage == null || responseMessage.CreateSubscriptionResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.CreateSubscriptionResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                subscriptionId            = response.SubscriptionId;
                revisedPublishingInterval = response.RevisedPublishingInterval;
                revisedLifetimeCount      = response.RevisedLifetimeCount;
                revisedMaxKeepAliveCount  = response.RevisedMaxKeepAliveCount;
            }
            finally
            {
                RequestCompleted(request, response, "CreateSubscription");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the CreateSubscription service.
        /// </summary>
        public IAsyncResult BeginCreateSubscription(
            RequestHeader requestHeader,
            double        requestedPublishingInterval,
            uint          requestedLifetimeCount,
            uint          requestedMaxKeepAliveCount,
            uint          maxNotificationsPerPublish,
            bool          publishingEnabled,
            byte          priority,
            AsyncCallback callback,
            object        asyncState)
        {
            CreateSubscriptionRequest request = new CreateSubscriptionRequest();

            request.RequestHeader               = requestHeader;
            request.RequestedPublishingInterval = requestedPublishingInterval;
            request.RequestedLifetimeCount      = requestedLifetimeCount;
            request.RequestedMaxKeepAliveCount  = requestedMaxKeepAliveCount;
            request.MaxNotificationsPerPublish  = maxNotificationsPerPublish;
            request.PublishingEnabled           = publishingEnabled;
            request.Priority                    = priority;

            UpdateRequestHeader(request, requestHeader == null, "CreateSubscription");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginCreateSubscription(new CreateSubscriptionMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the CreateSubscription service.
        /// </summary>
        public ResponseHeader EndCreateSubscription(
            IAsyncResult result,
            out uint   subscriptionId,
            out double revisedPublishingInterval,
            out uint   revisedLifetimeCount,
            out uint   revisedMaxKeepAliveCount)
        {
            CreateSubscriptionResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (CreateSubscriptionResponse)genericResponse;
                }
                else
                {
                    CreateSubscriptionResponseMessage responseMessage = InnerChannel.EndCreateSubscription(result);

                    if (responseMessage == null || responseMessage.CreateSubscriptionResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.CreateSubscriptionResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                subscriptionId            = response.SubscriptionId;
                revisedPublishingInterval = response.RevisedPublishingInterval;
                revisedLifetimeCount      = response.RevisedLifetimeCount;
                revisedMaxKeepAliveCount  = response.RevisedMaxKeepAliveCount;
            }
            finally
            {
                RequestCompleted(null, response, "CreateSubscription");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region ModifySubscription Methods
        #if (!OPCUA_EXCLUDE_ModifySubscription)
        /// <summary>
        /// Invokes the ModifySubscription service.
        /// </summary>
        public virtual ResponseHeader ModifySubscription(
            RequestHeader requestHeader,
            uint          subscriptionId,
            double        requestedPublishingInterval,
            uint          requestedLifetimeCount,
            uint          requestedMaxKeepAliveCount,
            uint          maxNotificationsPerPublish,
            byte          priority,
            out double    revisedPublishingInterval,
            out uint      revisedLifetimeCount,
            out uint      revisedMaxKeepAliveCount)
        {
            ModifySubscriptionRequest request = new ModifySubscriptionRequest();
            ModifySubscriptionResponse response = null;

            request.RequestHeader               = requestHeader;
            request.SubscriptionId              = subscriptionId;
            request.RequestedPublishingInterval = requestedPublishingInterval;
            request.RequestedLifetimeCount      = requestedLifetimeCount;
            request.RequestedMaxKeepAliveCount  = requestedMaxKeepAliveCount;
            request.MaxNotificationsPerPublish  = maxNotificationsPerPublish;
            request.Priority                    = priority;

            UpdateRequestHeader(request, requestHeader == null, "ModifySubscription");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (ModifySubscriptionResponse)genericResponse;
                }
                else
                {
                    ModifySubscriptionResponseMessage responseMessage = InnerChannel.ModifySubscription(new ModifySubscriptionMessage(request));

                    if (responseMessage == null || responseMessage.ModifySubscriptionResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.ModifySubscriptionResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                revisedPublishingInterval = response.RevisedPublishingInterval;
                revisedLifetimeCount      = response.RevisedLifetimeCount;
                revisedMaxKeepAliveCount  = response.RevisedMaxKeepAliveCount;
            }
            finally
            {
                RequestCompleted(request, response, "ModifySubscription");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the ModifySubscription service.
        /// </summary>
        public IAsyncResult BeginModifySubscription(
            RequestHeader requestHeader,
            uint          subscriptionId,
            double        requestedPublishingInterval,
            uint          requestedLifetimeCount,
            uint          requestedMaxKeepAliveCount,
            uint          maxNotificationsPerPublish,
            byte          priority,
            AsyncCallback callback,
            object        asyncState)
        {
            ModifySubscriptionRequest request = new ModifySubscriptionRequest();

            request.RequestHeader               = requestHeader;
            request.SubscriptionId              = subscriptionId;
            request.RequestedPublishingInterval = requestedPublishingInterval;
            request.RequestedLifetimeCount      = requestedLifetimeCount;
            request.RequestedMaxKeepAliveCount  = requestedMaxKeepAliveCount;
            request.MaxNotificationsPerPublish  = maxNotificationsPerPublish;
            request.Priority                    = priority;

            UpdateRequestHeader(request, requestHeader == null, "ModifySubscription");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginModifySubscription(new ModifySubscriptionMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the ModifySubscription service.
        /// </summary>
        public ResponseHeader EndModifySubscription(
            IAsyncResult result,
            out double revisedPublishingInterval,
            out uint   revisedLifetimeCount,
            out uint   revisedMaxKeepAliveCount)
        {
            ModifySubscriptionResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (ModifySubscriptionResponse)genericResponse;
                }
                else
                {
                    ModifySubscriptionResponseMessage responseMessage = InnerChannel.EndModifySubscription(result);

                    if (responseMessage == null || responseMessage.ModifySubscriptionResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.ModifySubscriptionResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                revisedPublishingInterval = response.RevisedPublishingInterval;
                revisedLifetimeCount      = response.RevisedLifetimeCount;
                revisedMaxKeepAliveCount  = response.RevisedMaxKeepAliveCount;
            }
            finally
            {
                RequestCompleted(null, response, "ModifySubscription");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region SetPublishingMode Methods
        #if (!OPCUA_EXCLUDE_SetPublishingMode)
        /// <summary>
        /// Invokes the SetPublishingMode service.
        /// </summary>
        public virtual ResponseHeader SetPublishingMode(
            RequestHeader                requestHeader,
            bool                         publishingEnabled,
            UInt32Collection             subscriptionIds,
            out StatusCodeCollection     results,
            out DiagnosticInfoCollection diagnosticInfos)
        {
            SetPublishingModeRequest request = new SetPublishingModeRequest();
            SetPublishingModeResponse response = null;

            request.RequestHeader     = requestHeader;
            request.PublishingEnabled = publishingEnabled;
            request.SubscriptionIds   = subscriptionIds;

            UpdateRequestHeader(request, requestHeader == null, "SetPublishingMode");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (SetPublishingModeResponse)genericResponse;
                }
                else
                {
                    SetPublishingModeResponseMessage responseMessage = InnerChannel.SetPublishingMode(new SetPublishingModeMessage(request));

                    if (responseMessage == null || responseMessage.SetPublishingModeResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.SetPublishingModeResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(request, response, "SetPublishingMode");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the SetPublishingMode service.
        /// </summary>
        public IAsyncResult BeginSetPublishingMode(
            RequestHeader    requestHeader,
            bool             publishingEnabled,
            UInt32Collection subscriptionIds,
            AsyncCallback    callback,
            object           asyncState)
        {
            SetPublishingModeRequest request = new SetPublishingModeRequest();

            request.RequestHeader     = requestHeader;
            request.PublishingEnabled = publishingEnabled;
            request.SubscriptionIds   = subscriptionIds;

            UpdateRequestHeader(request, requestHeader == null, "SetPublishingMode");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginSetPublishingMode(new SetPublishingModeMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the SetPublishingMode service.
        /// </summary>
        public ResponseHeader EndSetPublishingMode(
            IAsyncResult                 result,
            out StatusCodeCollection     results,
            out DiagnosticInfoCollection diagnosticInfos)
        {
            SetPublishingModeResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (SetPublishingModeResponse)genericResponse;
                }
                else
                {
                    SetPublishingModeResponseMessage responseMessage = InnerChannel.EndSetPublishingMode(result);

                    if (responseMessage == null || responseMessage.SetPublishingModeResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.SetPublishingModeResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(null, response, "SetPublishingMode");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region Publish Methods
        #if (!OPCUA_EXCLUDE_Publish)
        /// <summary>
        /// Invokes the Publish service.
        /// </summary>
        public virtual ResponseHeader Publish(
            RequestHeader                         requestHeader,
            SubscriptionAcknowledgementCollection subscriptionAcknowledgements,
            out uint                              subscriptionId,
            out UInt32Collection                  availableSequenceNumbers,
            out bool                              moreNotifications,
            out NotificationMessage               notificationMessage,
            out StatusCodeCollection              results,
            out DiagnosticInfoCollection          diagnosticInfos)
        {
            PublishRequest request = new PublishRequest();
            PublishResponse response = null;

            request.RequestHeader                = requestHeader;
            request.SubscriptionAcknowledgements = subscriptionAcknowledgements;

            UpdateRequestHeader(request, requestHeader == null, "Publish");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (PublishResponse)genericResponse;
                }
                else
                {
                    PublishResponseMessage responseMessage = InnerChannel.Publish(new PublishMessage(request));

                    if (responseMessage == null || responseMessage.PublishResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.PublishResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                subscriptionId           = response.SubscriptionId;
                availableSequenceNumbers = response.AvailableSequenceNumbers;
                moreNotifications        = response.MoreNotifications;
                notificationMessage      = response.NotificationMessage;
                results                  = response.Results;
                diagnosticInfos          = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(request, response, "Publish");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the Publish service.
        /// </summary>
        public IAsyncResult BeginPublish(
            RequestHeader                         requestHeader,
            SubscriptionAcknowledgementCollection subscriptionAcknowledgements,
            AsyncCallback                         callback,
            object                                asyncState)
        {
            PublishRequest request = new PublishRequest();

            request.RequestHeader                = requestHeader;
            request.SubscriptionAcknowledgements = subscriptionAcknowledgements;

            UpdateRequestHeader(request, requestHeader == null, "Publish");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginPublish(new PublishMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the Publish service.
        /// </summary>
        public ResponseHeader EndPublish(
            IAsyncResult                 result,
            out uint                     subscriptionId,
            out UInt32Collection         availableSequenceNumbers,
            out bool                     moreNotifications,
            out NotificationMessage      notificationMessage,
            out StatusCodeCollection     results,
            out DiagnosticInfoCollection diagnosticInfos)
        {
            PublishResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (PublishResponse)genericResponse;
                }
                else
                {
                    PublishResponseMessage responseMessage = InnerChannel.EndPublish(result);

                    if (responseMessage == null || responseMessage.PublishResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.PublishResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                subscriptionId           = response.SubscriptionId;
                availableSequenceNumbers = response.AvailableSequenceNumbers;
                moreNotifications        = response.MoreNotifications;
                notificationMessage      = response.NotificationMessage;
                results                  = response.Results;
                diagnosticInfos          = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(null, response, "Publish");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region Republish Methods
        #if (!OPCUA_EXCLUDE_Republish)
        /// <summary>
        /// Invokes the Republish service.
        /// </summary>
        public virtual ResponseHeader Republish(
            RequestHeader           requestHeader,
            uint                    subscriptionId,
            uint                    retransmitSequenceNumber,
            out NotificationMessage notificationMessage)
        {
            RepublishRequest request = new RepublishRequest();
            RepublishResponse response = null;

            request.RequestHeader            = requestHeader;
            request.SubscriptionId           = subscriptionId;
            request.RetransmitSequenceNumber = retransmitSequenceNumber;

            UpdateRequestHeader(request, requestHeader == null, "Republish");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (RepublishResponse)genericResponse;
                }
                else
                {
                    RepublishResponseMessage responseMessage = InnerChannel.Republish(new RepublishMessage(request));

                    if (responseMessage == null || responseMessage.RepublishResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.RepublishResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                notificationMessage = response.NotificationMessage;
            }
            finally
            {
                RequestCompleted(request, response, "Republish");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the Republish service.
        /// </summary>
        public IAsyncResult BeginRepublish(
            RequestHeader requestHeader,
            uint          subscriptionId,
            uint          retransmitSequenceNumber,
            AsyncCallback callback,
            object        asyncState)
        {
            RepublishRequest request = new RepublishRequest();

            request.RequestHeader            = requestHeader;
            request.SubscriptionId           = subscriptionId;
            request.RetransmitSequenceNumber = retransmitSequenceNumber;

            UpdateRequestHeader(request, requestHeader == null, "Republish");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginRepublish(new RepublishMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the Republish service.
        /// </summary>
        public ResponseHeader EndRepublish(
            IAsyncResult            result,
            out NotificationMessage notificationMessage)
        {
            RepublishResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (RepublishResponse)genericResponse;
                }
                else
                {
                    RepublishResponseMessage responseMessage = InnerChannel.EndRepublish(result);

                    if (responseMessage == null || responseMessage.RepublishResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.RepublishResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                notificationMessage = response.NotificationMessage;
            }
            finally
            {
                RequestCompleted(null, response, "Republish");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region TransferSubscriptions Methods
        #if (!OPCUA_EXCLUDE_TransferSubscriptions)
        /// <summary>
        /// Invokes the TransferSubscriptions service.
        /// </summary>
        public virtual ResponseHeader TransferSubscriptions(
            RequestHeader                requestHeader,
            UInt32Collection             subscriptionIds,
            bool                         sendInitialValues,
            out TransferResultCollection results,
            out DiagnosticInfoCollection diagnosticInfos)
        {
            TransferSubscriptionsRequest request = new TransferSubscriptionsRequest();
            TransferSubscriptionsResponse response = null;

            request.RequestHeader     = requestHeader;
            request.SubscriptionIds   = subscriptionIds;
            request.SendInitialValues = sendInitialValues;

            UpdateRequestHeader(request, requestHeader == null, "TransferSubscriptions");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (TransferSubscriptionsResponse)genericResponse;
                }
                else
                {
                    TransferSubscriptionsResponseMessage responseMessage = InnerChannel.TransferSubscriptions(new TransferSubscriptionsMessage(request));

                    if (responseMessage == null || responseMessage.TransferSubscriptionsResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.TransferSubscriptionsResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(request, response, "TransferSubscriptions");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the TransferSubscriptions service.
        /// </summary>
        public IAsyncResult BeginTransferSubscriptions(
            RequestHeader    requestHeader,
            UInt32Collection subscriptionIds,
            bool             sendInitialValues,
            AsyncCallback    callback,
            object           asyncState)
        {
            TransferSubscriptionsRequest request = new TransferSubscriptionsRequest();

            request.RequestHeader     = requestHeader;
            request.SubscriptionIds   = subscriptionIds;
            request.SendInitialValues = sendInitialValues;

            UpdateRequestHeader(request, requestHeader == null, "TransferSubscriptions");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginTransferSubscriptions(new TransferSubscriptionsMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the TransferSubscriptions service.
        /// </summary>
        public ResponseHeader EndTransferSubscriptions(
            IAsyncResult                 result,
            out TransferResultCollection results,
            out DiagnosticInfoCollection diagnosticInfos)
        {
            TransferSubscriptionsResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (TransferSubscriptionsResponse)genericResponse;
                }
                else
                {
                    TransferSubscriptionsResponseMessage responseMessage = InnerChannel.EndTransferSubscriptions(result);

                    if (responseMessage == null || responseMessage.TransferSubscriptionsResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.TransferSubscriptionsResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(null, response, "TransferSubscriptions");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region DeleteSubscriptions Methods
        #if (!OPCUA_EXCLUDE_DeleteSubscriptions)
        /// <summary>
        /// Invokes the DeleteSubscriptions service.
        /// </summary>
        public virtual ResponseHeader DeleteSubscriptions(
            RequestHeader                requestHeader,
            UInt32Collection             subscriptionIds,
            out StatusCodeCollection     results,
            out DiagnosticInfoCollection diagnosticInfos)
        {
            DeleteSubscriptionsRequest request = new DeleteSubscriptionsRequest();
            DeleteSubscriptionsResponse response = null;

            request.RequestHeader   = requestHeader;
            request.SubscriptionIds = subscriptionIds;

            UpdateRequestHeader(request, requestHeader == null, "DeleteSubscriptions");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (DeleteSubscriptionsResponse)genericResponse;
                }
                else
                {
                    DeleteSubscriptionsResponseMessage responseMessage = InnerChannel.DeleteSubscriptions(new DeleteSubscriptionsMessage(request));

                    if (responseMessage == null || responseMessage.DeleteSubscriptionsResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.DeleteSubscriptionsResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(request, response, "DeleteSubscriptions");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the DeleteSubscriptions service.
        /// </summary>
        public IAsyncResult BeginDeleteSubscriptions(
            RequestHeader    requestHeader,
            UInt32Collection subscriptionIds,
            AsyncCallback    callback,
            object           asyncState)
        {
            DeleteSubscriptionsRequest request = new DeleteSubscriptionsRequest();

            request.RequestHeader   = requestHeader;
            request.SubscriptionIds = subscriptionIds;

            UpdateRequestHeader(request, requestHeader == null, "DeleteSubscriptions");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginDeleteSubscriptions(new DeleteSubscriptionsMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the DeleteSubscriptions service.
        /// </summary>
        public ResponseHeader EndDeleteSubscriptions(
            IAsyncResult                 result,
            out StatusCodeCollection     results,
            out DiagnosticInfoCollection diagnosticInfos)
        {
            DeleteSubscriptionsResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (DeleteSubscriptionsResponse)genericResponse;
                }
                else
                {
                    DeleteSubscriptionsResponseMessage responseMessage = InnerChannel.EndDeleteSubscriptions(result);

                    if (responseMessage == null || responseMessage.DeleteSubscriptionsResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.DeleteSubscriptionsResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                results         = response.Results;
                diagnosticInfos = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(null, response, "DeleteSubscriptions");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion
        #endregion
    }

    /// <summary>
    /// The client side interface for a UA server.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class DiscoveryClient : ClientBase
    {
        #region Constructors
        /// <summary>
        /// Intializes the object with a channel and a message context.
        /// </summary>
        public DiscoveryClient(ITransportChannel channel)
        :
            base(channel)
        {
        }
        #endregion

        #region Public Properties
        /// <summary>
        /// The component  contains classes  object use to communicate with the server.
        /// </summary>
        public new IDiscoveryChannel InnerChannel
        {
            get { return (IDiscoveryChannel)base.InnerChannel; }
        }
        #endregion

        #region Client API
        #region FindServers Methods
        #if (!OPCUA_EXCLUDE_FindServers)
        /// <summary>
        /// Invokes the FindServers service.
        /// </summary>
        public virtual ResponseHeader FindServers(
            RequestHeader                        requestHeader,
            string                               endpointUrl,
            StringCollection                     localeIds,
            StringCollection                     serverUris,
            out ApplicationDescriptionCollection servers)
        {
            FindServersRequest request = new FindServersRequest();
            FindServersResponse response = null;

            request.RequestHeader = requestHeader;
            request.EndpointUrl   = endpointUrl;
            request.LocaleIds     = localeIds;
            request.ServerUris    = serverUris;

            UpdateRequestHeader(request, requestHeader == null, "FindServers");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (FindServersResponse)genericResponse;
                }
                else
                {
                    FindServersResponseMessage responseMessage = InnerChannel.FindServers(new FindServersMessage(request));

                    if (responseMessage == null || responseMessage.FindServersResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.FindServersResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                servers = response.Servers;
            }
            finally
            {
                RequestCompleted(request, response, "FindServers");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the FindServers service.
        /// </summary>
        public IAsyncResult BeginFindServers(
            RequestHeader    requestHeader,
            string           endpointUrl,
            StringCollection localeIds,
            StringCollection serverUris,
            AsyncCallback    callback,
            object           asyncState)
        {
            FindServersRequest request = new FindServersRequest();

            request.RequestHeader = requestHeader;
            request.EndpointUrl   = endpointUrl;
            request.LocaleIds     = localeIds;
            request.ServerUris    = serverUris;

            UpdateRequestHeader(request, requestHeader == null, "FindServers");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginFindServers(new FindServersMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the FindServers service.
        /// </summary>
        public ResponseHeader EndFindServers(
            IAsyncResult                         result,
            out ApplicationDescriptionCollection servers)
        {
            FindServersResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (FindServersResponse)genericResponse;
                }
                else
                {
                    FindServersResponseMessage responseMessage = InnerChannel.EndFindServers(result);

                    if (responseMessage == null || responseMessage.FindServersResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.FindServersResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                servers = response.Servers;
            }
            finally
            {
                RequestCompleted(null, response, "FindServers");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region FindServersOnNetwork Methods
        #if (!OPCUA_EXCLUDE_FindServersOnNetwork)
        /// <summary>
        /// Invokes the FindServersOnNetwork service.
        /// </summary>
        public virtual ResponseHeader FindServersOnNetwork(
            RequestHeader                 requestHeader,
            uint                          startingRecordId,
            uint                          maxRecordsToReturn,
            StringCollection              serverCapabilityFilter,
            out DateTime                  lastCounterResetTime,
            out ServerOnNetworkCollection servers)
        {
            FindServersOnNetworkRequest request = new FindServersOnNetworkRequest();
            FindServersOnNetworkResponse response = null;

            request.RequestHeader          = requestHeader;
            request.StartingRecordId       = startingRecordId;
            request.MaxRecordsToReturn     = maxRecordsToReturn;
            request.ServerCapabilityFilter = serverCapabilityFilter;

            UpdateRequestHeader(request, requestHeader == null, "FindServersOnNetwork");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (FindServersOnNetworkResponse)genericResponse;
                }
                else
                {
                    FindServersOnNetworkResponseMessage responseMessage = InnerChannel.FindServersOnNetwork(new FindServersOnNetworkMessage(request));

                    if (responseMessage == null || responseMessage.FindServersOnNetworkResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.FindServersOnNetworkResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                lastCounterResetTime = response.LastCounterResetTime;
                servers              = response.Servers;
            }
            finally
            {
                RequestCompleted(request, response, "FindServersOnNetwork");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the FindServersOnNetwork service.
        /// </summary>
        public IAsyncResult BeginFindServersOnNetwork(
            RequestHeader    requestHeader,
            uint             startingRecordId,
            uint             maxRecordsToReturn,
            StringCollection serverCapabilityFilter,
            AsyncCallback    callback,
            object           asyncState)
        {
            FindServersOnNetworkRequest request = new FindServersOnNetworkRequest();

            request.RequestHeader          = requestHeader;
            request.StartingRecordId       = startingRecordId;
            request.MaxRecordsToReturn     = maxRecordsToReturn;
            request.ServerCapabilityFilter = serverCapabilityFilter;

            UpdateRequestHeader(request, requestHeader == null, "FindServersOnNetwork");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginFindServersOnNetwork(new FindServersOnNetworkMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the FindServersOnNetwork service.
        /// </summary>
        public ResponseHeader EndFindServersOnNetwork(
            IAsyncResult                  result,
            out DateTime                  lastCounterResetTime,
            out ServerOnNetworkCollection servers)
        {
            FindServersOnNetworkResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (FindServersOnNetworkResponse)genericResponse;
                }
                else
                {
                    FindServersOnNetworkResponseMessage responseMessage = InnerChannel.EndFindServersOnNetwork(result);

                    if (responseMessage == null || responseMessage.FindServersOnNetworkResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.FindServersOnNetworkResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                lastCounterResetTime = response.LastCounterResetTime;
                servers              = response.Servers;
            }
            finally
            {
                RequestCompleted(null, response, "FindServersOnNetwork");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region GetEndpoints Methods
        #if (!OPCUA_EXCLUDE_GetEndpoints)
        /// <summary>
        /// Invokes the GetEndpoints service.
        /// </summary>
        public virtual ResponseHeader GetEndpoints(
            RequestHeader                     requestHeader,
            string                            endpointUrl,
            StringCollection                  localeIds,
            StringCollection                  profileUris,
            out EndpointDescriptionCollection endpoints)
        {
            GetEndpointsRequest request = new GetEndpointsRequest();
            GetEndpointsResponse response = null;

            request.RequestHeader = requestHeader;
            request.EndpointUrl   = endpointUrl;
            request.LocaleIds     = localeIds;
            request.ProfileUris   = profileUris;

            UpdateRequestHeader(request, requestHeader == null, "GetEndpoints");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (GetEndpointsResponse)genericResponse;
                }
                else
                {
                    GetEndpointsResponseMessage responseMessage = InnerChannel.GetEndpoints(new GetEndpointsMessage(request));

                    if (responseMessage == null || responseMessage.GetEndpointsResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.GetEndpointsResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                endpoints = response.Endpoints;
            }
            finally
            {
                RequestCompleted(request, response, "GetEndpoints");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the GetEndpoints service.
        /// </summary>
        public IAsyncResult BeginGetEndpoints(
            RequestHeader    requestHeader,
            string           endpointUrl,
            StringCollection localeIds,
            StringCollection profileUris,
            AsyncCallback    callback,
            object           asyncState)
        {
            GetEndpointsRequest request = new GetEndpointsRequest();

            request.RequestHeader = requestHeader;
            request.EndpointUrl   = endpointUrl;
            request.LocaleIds     = localeIds;
            request.ProfileUris   = profileUris;

            UpdateRequestHeader(request, requestHeader == null, "GetEndpoints");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginGetEndpoints(new GetEndpointsMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the GetEndpoints service.
        /// </summary>
        public ResponseHeader EndGetEndpoints(
            IAsyncResult                      result,
            out EndpointDescriptionCollection endpoints)
        {
            GetEndpointsResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (GetEndpointsResponse)genericResponse;
                }
                else
                {
                    GetEndpointsResponseMessage responseMessage = InnerChannel.EndGetEndpoints(result);

                    if (responseMessage == null || responseMessage.GetEndpointsResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.GetEndpointsResponse;
                    ValidateResponse(response.ResponseHeader);
                }

                endpoints = response.Endpoints;
            }
            finally
            {
                RequestCompleted(null, response, "GetEndpoints");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion
        #endregion
    }

    /// <summary>
    /// The client side interface for a UA server.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class RegistrationClient : ClientBase
    {
        #region Constructors
        /// <summary>
        /// Intializes the object with a channel and a message context.
        /// </summary>
        public RegistrationClient(ITransportChannel channel)
        :
            base(channel)
        {
        }
        #endregion

        #region Public Properties
        /// <summary>
        /// The component  contains classes  object use to communicate with the server.
        /// </summary>
        public new IRegistrationChannel InnerChannel
        {
            get { return (IRegistrationChannel)base.InnerChannel; }
        }
        #endregion

        #region Client API
        #region RegisterServer Methods
        #if (!OPCUA_EXCLUDE_RegisterServer)
        /// <summary>
        /// Invokes the RegisterServer service.
        /// </summary>
        public virtual ResponseHeader RegisterServer(
            RequestHeader    requestHeader,
            RegisteredServer server)
        {
            RegisterServerRequest request = new RegisterServerRequest();
            RegisterServerResponse response = null;

            request.RequestHeader = requestHeader;
            request.Server        = server;

            UpdateRequestHeader(request, requestHeader == null, "RegisterServer");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (RegisterServerResponse)genericResponse;
                }
                else
                {
                    RegisterServerResponseMessage responseMessage = InnerChannel.RegisterServer(new RegisterServerMessage(request));

                    if (responseMessage == null || responseMessage.RegisterServerResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.RegisterServerResponse;
                    ValidateResponse(response.ResponseHeader);
                }

            }
            finally
            {
                RequestCompleted(request, response, "RegisterServer");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the RegisterServer service.
        /// </summary>
        public IAsyncResult BeginRegisterServer(
            RequestHeader    requestHeader,
            RegisteredServer server,
            AsyncCallback    callback,
            object           asyncState)
        {
            RegisterServerRequest request = new RegisterServerRequest();

            request.RequestHeader = requestHeader;
            request.Server        = server;

            UpdateRequestHeader(request, requestHeader == null, "RegisterServer");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginRegisterServer(new RegisterServerMessage(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the RegisterServer service.
        /// </summary>
        public ResponseHeader EndRegisterServer(
            IAsyncResult result)
        {
            RegisterServerResponse response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (RegisterServerResponse)genericResponse;
                }
                else
                {
                    RegisterServerResponseMessage responseMessage = InnerChannel.EndRegisterServer(result);

                    if (responseMessage == null || responseMessage.RegisterServerResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.RegisterServerResponse;
                    ValidateResponse(response.ResponseHeader);
                }

            }
            finally
            {
                RequestCompleted(null, response, "RegisterServer");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion

        #region RegisterServer2 Methods
        #if (!OPCUA_EXCLUDE_RegisterServer2)
        /// <summary>
        /// Invokes the RegisterServer2 service.
        /// </summary>
        public virtual ResponseHeader RegisterServer2(
            RequestHeader                requestHeader,
            RegisteredServer             server,
            ExtensionObjectCollection    discoveryConfiguration,
            out StatusCodeCollection     configurationResults,
            out DiagnosticInfoCollection diagnosticInfos)
        {
            RegisterServer2Request request = new RegisterServer2Request();
            RegisterServer2Response response = null;

            request.RequestHeader          = requestHeader;
            request.Server                 = server;
            request.DiscoveryConfiguration = discoveryConfiguration;

            UpdateRequestHeader(request, requestHeader == null, "RegisterServer2");

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.SendRequest(request);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (RegisterServer2Response)genericResponse;
                }
                else
                {
                    RegisterServer2ResponseMessage responseMessage = InnerChannel.RegisterServer2(new RegisterServer2Message(request));

                    if (responseMessage == null || responseMessage.RegisterServer2Response == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.RegisterServer2Response;
                    ValidateResponse(response.ResponseHeader);
                }

                configurationResults = response.ConfigurationResults;
                diagnosticInfos      = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(request, response, "RegisterServer2");
            }

            return response.ResponseHeader;
        }

        /// <summary>
        /// Begins an asynchronous invocation of the RegisterServer2 service.
        /// </summary>
        public IAsyncResult BeginRegisterServer2(
            RequestHeader             requestHeader,
            RegisteredServer          server,
            ExtensionObjectCollection discoveryConfiguration,
            AsyncCallback             callback,
            object                    asyncState)
        {
            RegisterServer2Request request = new RegisterServer2Request();

            request.RequestHeader          = requestHeader;
            request.Server                 = server;
            request.DiscoveryConfiguration = discoveryConfiguration;

            UpdateRequestHeader(request, requestHeader == null, "RegisterServer2");

            if (UseTransportChannel)
            {
                return TransportChannel.BeginSendRequest(request, callback, asyncState);
            }

            return InnerChannel.BeginRegisterServer2(new RegisterServer2Message(request), callback, asyncState);
        }

        /// <summary>
        /// Finishes an asynchronous invocation of the RegisterServer2 service.
        /// </summary>
        public ResponseHeader EndRegisterServer2(
            IAsyncResult                 result,
            out StatusCodeCollection     configurationResults,
            out DiagnosticInfoCollection diagnosticInfos)
        {
            RegisterServer2Response response = null;

            try
            {
                if (UseTransportChannel)
                {
                    IServiceResponse genericResponse = TransportChannel.EndSendRequest(result);

                    if (genericResponse == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    ValidateResponse(genericResponse.ResponseHeader);
                    response = (RegisterServer2Response)genericResponse;
                }
                else
                {
                    RegisterServer2ResponseMessage responseMessage = InnerChannel.EndRegisterServer2(result);

                    if (responseMessage == null || responseMessage.RegisterServer2Response == null)
                    {
                        throw new ServiceResultException(StatusCodes.BadUnknownResponse);
                    }

                    response = responseMessage.RegisterServer2Response;
                    ValidateResponse(response.ResponseHeader);
                }

                configurationResults = response.ConfigurationResults;
                diagnosticInfos      = response.DiagnosticInfos;
            }
            finally
            {
                RequestCompleted(null, response, "RegisterServer2");
            }

            return response.ResponseHeader;
        }
        #endif
        #endregion
        #endregion
    }
}
