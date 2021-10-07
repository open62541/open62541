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
using System.Runtime.Serialization;

namespace Opc.Ua
{
    #region FindServers Service Messages
    #if (!OPCUA_EXCLUDE_FindServers)
    public partial class FindServersRequest : IServiceRequest
    {
    }

    public partial class FindServersResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the FindServers service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class FindServersMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public FindServersRequest FindServersRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public FindServersMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public FindServersMessage(FindServersRequest FindServersRequest)
        {
            this.FindServersRequest = FindServersRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return FindServersRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            FindServersResponse body = response as FindServersResponse;

            if (body == null)
            {
                body = new FindServersResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new FindServersResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the FindServers service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class FindServersResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public FindServersResponse FindServersResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public FindServersResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public FindServersResponseMessage(FindServersResponse FindServersResponse)
        {
            this.FindServersResponse = FindServersResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public FindServersResponseMessage(ServiceFault ServiceFault)
        {
            this.FindServersResponse = new FindServersResponse();

            if (ServiceFault != null)
            {
                this.FindServersResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region FindServersOnNetwork Service Messages
    #if (!OPCUA_EXCLUDE_FindServersOnNetwork)
    public partial class FindServersOnNetworkRequest : IServiceRequest
    {
    }

    public partial class FindServersOnNetworkResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the FindServersOnNetwork service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class FindServersOnNetworkMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public FindServersOnNetworkRequest FindServersOnNetworkRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public FindServersOnNetworkMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public FindServersOnNetworkMessage(FindServersOnNetworkRequest FindServersOnNetworkRequest)
        {
            this.FindServersOnNetworkRequest = FindServersOnNetworkRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return FindServersOnNetworkRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            FindServersOnNetworkResponse body = response as FindServersOnNetworkResponse;

            if (body == null)
            {
                body = new FindServersOnNetworkResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new FindServersOnNetworkResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the FindServersOnNetwork service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class FindServersOnNetworkResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public FindServersOnNetworkResponse FindServersOnNetworkResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public FindServersOnNetworkResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public FindServersOnNetworkResponseMessage(FindServersOnNetworkResponse FindServersOnNetworkResponse)
        {
            this.FindServersOnNetworkResponse = FindServersOnNetworkResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public FindServersOnNetworkResponseMessage(ServiceFault ServiceFault)
        {
            this.FindServersOnNetworkResponse = new FindServersOnNetworkResponse();

            if (ServiceFault != null)
            {
                this.FindServersOnNetworkResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region GetEndpoints Service Messages
    #if (!OPCUA_EXCLUDE_GetEndpoints)
    public partial class GetEndpointsRequest : IServiceRequest
    {
    }

    public partial class GetEndpointsResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the GetEndpoints service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class GetEndpointsMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public GetEndpointsRequest GetEndpointsRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public GetEndpointsMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public GetEndpointsMessage(GetEndpointsRequest GetEndpointsRequest)
        {
            this.GetEndpointsRequest = GetEndpointsRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return GetEndpointsRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            GetEndpointsResponse body = response as GetEndpointsResponse;

            if (body == null)
            {
                body = new GetEndpointsResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new GetEndpointsResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the GetEndpoints service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class GetEndpointsResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public GetEndpointsResponse GetEndpointsResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public GetEndpointsResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public GetEndpointsResponseMessage(GetEndpointsResponse GetEndpointsResponse)
        {
            this.GetEndpointsResponse = GetEndpointsResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public GetEndpointsResponseMessage(ServiceFault ServiceFault)
        {
            this.GetEndpointsResponse = new GetEndpointsResponse();

            if (ServiceFault != null)
            {
                this.GetEndpointsResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region RegisterServer Service Messages
    #if (!OPCUA_EXCLUDE_RegisterServer)
    public partial class RegisterServerRequest : IServiceRequest
    {
    }

    public partial class RegisterServerResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the RegisterServer service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class RegisterServerMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public RegisterServerRequest RegisterServerRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public RegisterServerMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public RegisterServerMessage(RegisterServerRequest RegisterServerRequest)
        {
            this.RegisterServerRequest = RegisterServerRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return RegisterServerRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            RegisterServerResponse body = response as RegisterServerResponse;

            if (body == null)
            {
                body = new RegisterServerResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new RegisterServerResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the RegisterServer service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class RegisterServerResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public RegisterServerResponse RegisterServerResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public RegisterServerResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public RegisterServerResponseMessage(RegisterServerResponse RegisterServerResponse)
        {
            this.RegisterServerResponse = RegisterServerResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public RegisterServerResponseMessage(ServiceFault ServiceFault)
        {
            this.RegisterServerResponse = new RegisterServerResponse();

            if (ServiceFault != null)
            {
                this.RegisterServerResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region RegisterServer2 Service Messages
    #if (!OPCUA_EXCLUDE_RegisterServer2)
    public partial class RegisterServer2Request : IServiceRequest
    {
    }

    public partial class RegisterServer2Response : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the RegisterServer2 service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class RegisterServer2Message : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public RegisterServer2Request RegisterServer2Request;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public RegisterServer2Message()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public RegisterServer2Message(RegisterServer2Request RegisterServer2Request)
        {
            this.RegisterServer2Request = RegisterServer2Request;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return RegisterServer2Request;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            RegisterServer2Response body = response as RegisterServer2Response;

            if (body == null)
            {
                body = new RegisterServer2Response();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new RegisterServer2ResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the RegisterServer2 service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class RegisterServer2ResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public RegisterServer2Response RegisterServer2Response;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public RegisterServer2ResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public RegisterServer2ResponseMessage(RegisterServer2Response RegisterServer2Response)
        {
            this.RegisterServer2Response = RegisterServer2Response;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public RegisterServer2ResponseMessage(ServiceFault ServiceFault)
        {
            this.RegisterServer2Response = new RegisterServer2Response();

            if (ServiceFault != null)
            {
                this.RegisterServer2Response.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region OpenSecureChannel Service Messages
    #if (!OPCUA_EXCLUDE_OpenSecureChannel)
    public partial class OpenSecureChannelRequest : IServiceRequest
    {
    }

    public partial class OpenSecureChannelResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the OpenSecureChannel service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class OpenSecureChannelMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public OpenSecureChannelRequest OpenSecureChannelRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public OpenSecureChannelMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public OpenSecureChannelMessage(OpenSecureChannelRequest OpenSecureChannelRequest)
        {
            this.OpenSecureChannelRequest = OpenSecureChannelRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return OpenSecureChannelRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            OpenSecureChannelResponse body = response as OpenSecureChannelResponse;

            if (body == null)
            {
                body = new OpenSecureChannelResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new OpenSecureChannelResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the OpenSecureChannel service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class OpenSecureChannelResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public OpenSecureChannelResponse OpenSecureChannelResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public OpenSecureChannelResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public OpenSecureChannelResponseMessage(OpenSecureChannelResponse OpenSecureChannelResponse)
        {
            this.OpenSecureChannelResponse = OpenSecureChannelResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public OpenSecureChannelResponseMessage(ServiceFault ServiceFault)
        {
            this.OpenSecureChannelResponse = new OpenSecureChannelResponse();

            if (ServiceFault != null)
            {
                this.OpenSecureChannelResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region CloseSecureChannel Service Messages
    #if (!OPCUA_EXCLUDE_CloseSecureChannel)
    public partial class CloseSecureChannelRequest : IServiceRequest
    {
    }

    public partial class CloseSecureChannelResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the CloseSecureChannel service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class CloseSecureChannelMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public CloseSecureChannelRequest CloseSecureChannelRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public CloseSecureChannelMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public CloseSecureChannelMessage(CloseSecureChannelRequest CloseSecureChannelRequest)
        {
            this.CloseSecureChannelRequest = CloseSecureChannelRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return CloseSecureChannelRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            CloseSecureChannelResponse body = response as CloseSecureChannelResponse;

            if (body == null)
            {
                body = new CloseSecureChannelResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new CloseSecureChannelResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the CloseSecureChannel service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class CloseSecureChannelResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public CloseSecureChannelResponse CloseSecureChannelResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public CloseSecureChannelResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public CloseSecureChannelResponseMessage(CloseSecureChannelResponse CloseSecureChannelResponse)
        {
            this.CloseSecureChannelResponse = CloseSecureChannelResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public CloseSecureChannelResponseMessage(ServiceFault ServiceFault)
        {
            this.CloseSecureChannelResponse = new CloseSecureChannelResponse();

            if (ServiceFault != null)
            {
                this.CloseSecureChannelResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region CreateSession Service Messages
    #if (!OPCUA_EXCLUDE_CreateSession)
    public partial class CreateSessionRequest : IServiceRequest
    {
    }

    public partial class CreateSessionResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the CreateSession service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class CreateSessionMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public CreateSessionRequest CreateSessionRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public CreateSessionMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public CreateSessionMessage(CreateSessionRequest CreateSessionRequest)
        {
            this.CreateSessionRequest = CreateSessionRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return CreateSessionRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            CreateSessionResponse body = response as CreateSessionResponse;

            if (body == null)
            {
                body = new CreateSessionResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new CreateSessionResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the CreateSession service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class CreateSessionResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public CreateSessionResponse CreateSessionResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public CreateSessionResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public CreateSessionResponseMessage(CreateSessionResponse CreateSessionResponse)
        {
            this.CreateSessionResponse = CreateSessionResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public CreateSessionResponseMessage(ServiceFault ServiceFault)
        {
            this.CreateSessionResponse = new CreateSessionResponse();

            if (ServiceFault != null)
            {
                this.CreateSessionResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region ActivateSession Service Messages
    #if (!OPCUA_EXCLUDE_ActivateSession)
    public partial class ActivateSessionRequest : IServiceRequest
    {
    }

    public partial class ActivateSessionResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the ActivateSession service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class ActivateSessionMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public ActivateSessionRequest ActivateSessionRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public ActivateSessionMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public ActivateSessionMessage(ActivateSessionRequest ActivateSessionRequest)
        {
            this.ActivateSessionRequest = ActivateSessionRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return ActivateSessionRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            ActivateSessionResponse body = response as ActivateSessionResponse;

            if (body == null)
            {
                body = new ActivateSessionResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new ActivateSessionResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the ActivateSession service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class ActivateSessionResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public ActivateSessionResponse ActivateSessionResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public ActivateSessionResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public ActivateSessionResponseMessage(ActivateSessionResponse ActivateSessionResponse)
        {
            this.ActivateSessionResponse = ActivateSessionResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public ActivateSessionResponseMessage(ServiceFault ServiceFault)
        {
            this.ActivateSessionResponse = new ActivateSessionResponse();

            if (ServiceFault != null)
            {
                this.ActivateSessionResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region CloseSession Service Messages
    #if (!OPCUA_EXCLUDE_CloseSession)
    public partial class CloseSessionRequest : IServiceRequest
    {
    }

    public partial class CloseSessionResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the CloseSession service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class CloseSessionMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public CloseSessionRequest CloseSessionRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public CloseSessionMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public CloseSessionMessage(CloseSessionRequest CloseSessionRequest)
        {
            this.CloseSessionRequest = CloseSessionRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return CloseSessionRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            CloseSessionResponse body = response as CloseSessionResponse;

            if (body == null)
            {
                body = new CloseSessionResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new CloseSessionResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the CloseSession service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class CloseSessionResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public CloseSessionResponse CloseSessionResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public CloseSessionResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public CloseSessionResponseMessage(CloseSessionResponse CloseSessionResponse)
        {
            this.CloseSessionResponse = CloseSessionResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public CloseSessionResponseMessage(ServiceFault ServiceFault)
        {
            this.CloseSessionResponse = new CloseSessionResponse();

            if (ServiceFault != null)
            {
                this.CloseSessionResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region Cancel Service Messages
    #if (!OPCUA_EXCLUDE_Cancel)
    public partial class CancelRequest : IServiceRequest
    {
    }

    public partial class CancelResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the Cancel service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class CancelMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public CancelRequest CancelRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public CancelMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public CancelMessage(CancelRequest CancelRequest)
        {
            this.CancelRequest = CancelRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return CancelRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            CancelResponse body = response as CancelResponse;

            if (body == null)
            {
                body = new CancelResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new CancelResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the Cancel service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class CancelResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public CancelResponse CancelResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public CancelResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public CancelResponseMessage(CancelResponse CancelResponse)
        {
            this.CancelResponse = CancelResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public CancelResponseMessage(ServiceFault ServiceFault)
        {
            this.CancelResponse = new CancelResponse();

            if (ServiceFault != null)
            {
                this.CancelResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region AddNodes Service Messages
    #if (!OPCUA_EXCLUDE_AddNodes)
    public partial class AddNodesRequest : IServiceRequest
    {
    }

    public partial class AddNodesResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the AddNodes service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class AddNodesMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public AddNodesRequest AddNodesRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public AddNodesMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public AddNodesMessage(AddNodesRequest AddNodesRequest)
        {
            this.AddNodesRequest = AddNodesRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return AddNodesRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            AddNodesResponse body = response as AddNodesResponse;

            if (body == null)
            {
                body = new AddNodesResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new AddNodesResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the AddNodes service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class AddNodesResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public AddNodesResponse AddNodesResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public AddNodesResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public AddNodesResponseMessage(AddNodesResponse AddNodesResponse)
        {
            this.AddNodesResponse = AddNodesResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public AddNodesResponseMessage(ServiceFault ServiceFault)
        {
            this.AddNodesResponse = new AddNodesResponse();

            if (ServiceFault != null)
            {
                this.AddNodesResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region AddReferences Service Messages
    #if (!OPCUA_EXCLUDE_AddReferences)
    public partial class AddReferencesRequest : IServiceRequest
    {
    }

    public partial class AddReferencesResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the AddReferences service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class AddReferencesMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public AddReferencesRequest AddReferencesRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public AddReferencesMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public AddReferencesMessage(AddReferencesRequest AddReferencesRequest)
        {
            this.AddReferencesRequest = AddReferencesRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return AddReferencesRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            AddReferencesResponse body = response as AddReferencesResponse;

            if (body == null)
            {
                body = new AddReferencesResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new AddReferencesResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the AddReferences service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class AddReferencesResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public AddReferencesResponse AddReferencesResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public AddReferencesResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public AddReferencesResponseMessage(AddReferencesResponse AddReferencesResponse)
        {
            this.AddReferencesResponse = AddReferencesResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public AddReferencesResponseMessage(ServiceFault ServiceFault)
        {
            this.AddReferencesResponse = new AddReferencesResponse();

            if (ServiceFault != null)
            {
                this.AddReferencesResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region DeleteNodes Service Messages
    #if (!OPCUA_EXCLUDE_DeleteNodes)
    public partial class DeleteNodesRequest : IServiceRequest
    {
    }

    public partial class DeleteNodesResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the DeleteNodes service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class DeleteNodesMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public DeleteNodesRequest DeleteNodesRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public DeleteNodesMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public DeleteNodesMessage(DeleteNodesRequest DeleteNodesRequest)
        {
            this.DeleteNodesRequest = DeleteNodesRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return DeleteNodesRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            DeleteNodesResponse body = response as DeleteNodesResponse;

            if (body == null)
            {
                body = new DeleteNodesResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new DeleteNodesResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the DeleteNodes service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class DeleteNodesResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public DeleteNodesResponse DeleteNodesResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public DeleteNodesResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public DeleteNodesResponseMessage(DeleteNodesResponse DeleteNodesResponse)
        {
            this.DeleteNodesResponse = DeleteNodesResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public DeleteNodesResponseMessage(ServiceFault ServiceFault)
        {
            this.DeleteNodesResponse = new DeleteNodesResponse();

            if (ServiceFault != null)
            {
                this.DeleteNodesResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region DeleteReferences Service Messages
    #if (!OPCUA_EXCLUDE_DeleteReferences)
    public partial class DeleteReferencesRequest : IServiceRequest
    {
    }

    public partial class DeleteReferencesResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the DeleteReferences service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class DeleteReferencesMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public DeleteReferencesRequest DeleteReferencesRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public DeleteReferencesMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public DeleteReferencesMessage(DeleteReferencesRequest DeleteReferencesRequest)
        {
            this.DeleteReferencesRequest = DeleteReferencesRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return DeleteReferencesRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            DeleteReferencesResponse body = response as DeleteReferencesResponse;

            if (body == null)
            {
                body = new DeleteReferencesResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new DeleteReferencesResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the DeleteReferences service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class DeleteReferencesResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public DeleteReferencesResponse DeleteReferencesResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public DeleteReferencesResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public DeleteReferencesResponseMessage(DeleteReferencesResponse DeleteReferencesResponse)
        {
            this.DeleteReferencesResponse = DeleteReferencesResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public DeleteReferencesResponseMessage(ServiceFault ServiceFault)
        {
            this.DeleteReferencesResponse = new DeleteReferencesResponse();

            if (ServiceFault != null)
            {
                this.DeleteReferencesResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region Browse Service Messages
    #if (!OPCUA_EXCLUDE_Browse)
    public partial class BrowseRequest : IServiceRequest
    {
    }

    public partial class BrowseResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the Browse service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class BrowseMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public BrowseRequest BrowseRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public BrowseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public BrowseMessage(BrowseRequest BrowseRequest)
        {
            this.BrowseRequest = BrowseRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return BrowseRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            BrowseResponse body = response as BrowseResponse;

            if (body == null)
            {
                body = new BrowseResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new BrowseResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the Browse service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class BrowseResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public BrowseResponse BrowseResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public BrowseResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public BrowseResponseMessage(BrowseResponse BrowseResponse)
        {
            this.BrowseResponse = BrowseResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public BrowseResponseMessage(ServiceFault ServiceFault)
        {
            this.BrowseResponse = new BrowseResponse();

            if (ServiceFault != null)
            {
                this.BrowseResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region BrowseNext Service Messages
    #if (!OPCUA_EXCLUDE_BrowseNext)
    public partial class BrowseNextRequest : IServiceRequest
    {
    }

    public partial class BrowseNextResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the BrowseNext service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class BrowseNextMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public BrowseNextRequest BrowseNextRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public BrowseNextMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public BrowseNextMessage(BrowseNextRequest BrowseNextRequest)
        {
            this.BrowseNextRequest = BrowseNextRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return BrowseNextRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            BrowseNextResponse body = response as BrowseNextResponse;

            if (body == null)
            {
                body = new BrowseNextResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new BrowseNextResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the BrowseNext service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class BrowseNextResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public BrowseNextResponse BrowseNextResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public BrowseNextResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public BrowseNextResponseMessage(BrowseNextResponse BrowseNextResponse)
        {
            this.BrowseNextResponse = BrowseNextResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public BrowseNextResponseMessage(ServiceFault ServiceFault)
        {
            this.BrowseNextResponse = new BrowseNextResponse();

            if (ServiceFault != null)
            {
                this.BrowseNextResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region TranslateBrowsePathsToNodeIds Service Messages
    #if (!OPCUA_EXCLUDE_TranslateBrowsePathsToNodeIds)
    public partial class TranslateBrowsePathsToNodeIdsRequest : IServiceRequest
    {
    }

    public partial class TranslateBrowsePathsToNodeIdsResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the TranslateBrowsePathsToNodeIds service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class TranslateBrowsePathsToNodeIdsMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public TranslateBrowsePathsToNodeIdsRequest TranslateBrowsePathsToNodeIdsRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public TranslateBrowsePathsToNodeIdsMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public TranslateBrowsePathsToNodeIdsMessage(TranslateBrowsePathsToNodeIdsRequest TranslateBrowsePathsToNodeIdsRequest)
        {
            this.TranslateBrowsePathsToNodeIdsRequest = TranslateBrowsePathsToNodeIdsRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return TranslateBrowsePathsToNodeIdsRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            TranslateBrowsePathsToNodeIdsResponse body = response as TranslateBrowsePathsToNodeIdsResponse;

            if (body == null)
            {
                body = new TranslateBrowsePathsToNodeIdsResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new TranslateBrowsePathsToNodeIdsResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the TranslateBrowsePathsToNodeIds service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class TranslateBrowsePathsToNodeIdsResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public TranslateBrowsePathsToNodeIdsResponse TranslateBrowsePathsToNodeIdsResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public TranslateBrowsePathsToNodeIdsResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public TranslateBrowsePathsToNodeIdsResponseMessage(TranslateBrowsePathsToNodeIdsResponse TranslateBrowsePathsToNodeIdsResponse)
        {
            this.TranslateBrowsePathsToNodeIdsResponse = TranslateBrowsePathsToNodeIdsResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public TranslateBrowsePathsToNodeIdsResponseMessage(ServiceFault ServiceFault)
        {
            this.TranslateBrowsePathsToNodeIdsResponse = new TranslateBrowsePathsToNodeIdsResponse();

            if (ServiceFault != null)
            {
                this.TranslateBrowsePathsToNodeIdsResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region RegisterNodes Service Messages
    #if (!OPCUA_EXCLUDE_RegisterNodes)
    public partial class RegisterNodesRequest : IServiceRequest
    {
    }

    public partial class RegisterNodesResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the RegisterNodes service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class RegisterNodesMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public RegisterNodesRequest RegisterNodesRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public RegisterNodesMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public RegisterNodesMessage(RegisterNodesRequest RegisterNodesRequest)
        {
            this.RegisterNodesRequest = RegisterNodesRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return RegisterNodesRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            RegisterNodesResponse body = response as RegisterNodesResponse;

            if (body == null)
            {
                body = new RegisterNodesResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new RegisterNodesResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the RegisterNodes service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class RegisterNodesResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public RegisterNodesResponse RegisterNodesResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public RegisterNodesResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public RegisterNodesResponseMessage(RegisterNodesResponse RegisterNodesResponse)
        {
            this.RegisterNodesResponse = RegisterNodesResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public RegisterNodesResponseMessage(ServiceFault ServiceFault)
        {
            this.RegisterNodesResponse = new RegisterNodesResponse();

            if (ServiceFault != null)
            {
                this.RegisterNodesResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region UnregisterNodes Service Messages
    #if (!OPCUA_EXCLUDE_UnregisterNodes)
    public partial class UnregisterNodesRequest : IServiceRequest
    {
    }

    public partial class UnregisterNodesResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the UnregisterNodes service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class UnregisterNodesMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public UnregisterNodesRequest UnregisterNodesRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public UnregisterNodesMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public UnregisterNodesMessage(UnregisterNodesRequest UnregisterNodesRequest)
        {
            this.UnregisterNodesRequest = UnregisterNodesRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return UnregisterNodesRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            UnregisterNodesResponse body = response as UnregisterNodesResponse;

            if (body == null)
            {
                body = new UnregisterNodesResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new UnregisterNodesResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the UnregisterNodes service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class UnregisterNodesResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public UnregisterNodesResponse UnregisterNodesResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public UnregisterNodesResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public UnregisterNodesResponseMessage(UnregisterNodesResponse UnregisterNodesResponse)
        {
            this.UnregisterNodesResponse = UnregisterNodesResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public UnregisterNodesResponseMessage(ServiceFault ServiceFault)
        {
            this.UnregisterNodesResponse = new UnregisterNodesResponse();

            if (ServiceFault != null)
            {
                this.UnregisterNodesResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region QueryFirst Service Messages
    #if (!OPCUA_EXCLUDE_QueryFirst)
    public partial class QueryFirstRequest : IServiceRequest
    {
    }

    public partial class QueryFirstResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the QueryFirst service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class QueryFirstMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public QueryFirstRequest QueryFirstRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public QueryFirstMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public QueryFirstMessage(QueryFirstRequest QueryFirstRequest)
        {
            this.QueryFirstRequest = QueryFirstRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return QueryFirstRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            QueryFirstResponse body = response as QueryFirstResponse;

            if (body == null)
            {
                body = new QueryFirstResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new QueryFirstResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the QueryFirst service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class QueryFirstResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public QueryFirstResponse QueryFirstResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public QueryFirstResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public QueryFirstResponseMessage(QueryFirstResponse QueryFirstResponse)
        {
            this.QueryFirstResponse = QueryFirstResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public QueryFirstResponseMessage(ServiceFault ServiceFault)
        {
            this.QueryFirstResponse = new QueryFirstResponse();

            if (ServiceFault != null)
            {
                this.QueryFirstResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region QueryNext Service Messages
    #if (!OPCUA_EXCLUDE_QueryNext)
    public partial class QueryNextRequest : IServiceRequest
    {
    }

    public partial class QueryNextResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the QueryNext service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class QueryNextMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public QueryNextRequest QueryNextRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public QueryNextMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public QueryNextMessage(QueryNextRequest QueryNextRequest)
        {
            this.QueryNextRequest = QueryNextRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return QueryNextRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            QueryNextResponse body = response as QueryNextResponse;

            if (body == null)
            {
                body = new QueryNextResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new QueryNextResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the QueryNext service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class QueryNextResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public QueryNextResponse QueryNextResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public QueryNextResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public QueryNextResponseMessage(QueryNextResponse QueryNextResponse)
        {
            this.QueryNextResponse = QueryNextResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public QueryNextResponseMessage(ServiceFault ServiceFault)
        {
            this.QueryNextResponse = new QueryNextResponse();

            if (ServiceFault != null)
            {
                this.QueryNextResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region Read Service Messages
    #if (!OPCUA_EXCLUDE_Read)
    public partial class ReadRequest : IServiceRequest
    {
    }

    public partial class ReadResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the Read service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class ReadMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public ReadRequest ReadRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public ReadMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public ReadMessage(ReadRequest ReadRequest)
        {
            this.ReadRequest = ReadRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return ReadRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            ReadResponse body = response as ReadResponse;

            if (body == null)
            {
                body = new ReadResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new ReadResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the Read service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class ReadResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public ReadResponse ReadResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public ReadResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public ReadResponseMessage(ReadResponse ReadResponse)
        {
            this.ReadResponse = ReadResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public ReadResponseMessage(ServiceFault ServiceFault)
        {
            this.ReadResponse = new ReadResponse();

            if (ServiceFault != null)
            {
                this.ReadResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region HistoryRead Service Messages
    #if (!OPCUA_EXCLUDE_HistoryRead)
    public partial class HistoryReadRequest : IServiceRequest
    {
    }

    public partial class HistoryReadResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the HistoryRead service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class HistoryReadMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public HistoryReadRequest HistoryReadRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public HistoryReadMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public HistoryReadMessage(HistoryReadRequest HistoryReadRequest)
        {
            this.HistoryReadRequest = HistoryReadRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return HistoryReadRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            HistoryReadResponse body = response as HistoryReadResponse;

            if (body == null)
            {
                body = new HistoryReadResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new HistoryReadResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the HistoryRead service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class HistoryReadResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public HistoryReadResponse HistoryReadResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public HistoryReadResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public HistoryReadResponseMessage(HistoryReadResponse HistoryReadResponse)
        {
            this.HistoryReadResponse = HistoryReadResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public HistoryReadResponseMessage(ServiceFault ServiceFault)
        {
            this.HistoryReadResponse = new HistoryReadResponse();

            if (ServiceFault != null)
            {
                this.HistoryReadResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region Write Service Messages
    #if (!OPCUA_EXCLUDE_Write)
    public partial class WriteRequest : IServiceRequest
    {
    }

    public partial class WriteResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the Write service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class WriteMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public WriteRequest WriteRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public WriteMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public WriteMessage(WriteRequest WriteRequest)
        {
            this.WriteRequest = WriteRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return WriteRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            WriteResponse body = response as WriteResponse;

            if (body == null)
            {
                body = new WriteResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new WriteResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the Write service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class WriteResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public WriteResponse WriteResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public WriteResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public WriteResponseMessage(WriteResponse WriteResponse)
        {
            this.WriteResponse = WriteResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public WriteResponseMessage(ServiceFault ServiceFault)
        {
            this.WriteResponse = new WriteResponse();

            if (ServiceFault != null)
            {
                this.WriteResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region HistoryUpdate Service Messages
    #if (!OPCUA_EXCLUDE_HistoryUpdate)
    public partial class HistoryUpdateRequest : IServiceRequest
    {
    }

    public partial class HistoryUpdateResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the HistoryUpdate service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class HistoryUpdateMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public HistoryUpdateRequest HistoryUpdateRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public HistoryUpdateMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public HistoryUpdateMessage(HistoryUpdateRequest HistoryUpdateRequest)
        {
            this.HistoryUpdateRequest = HistoryUpdateRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return HistoryUpdateRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            HistoryUpdateResponse body = response as HistoryUpdateResponse;

            if (body == null)
            {
                body = new HistoryUpdateResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new HistoryUpdateResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the HistoryUpdate service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class HistoryUpdateResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public HistoryUpdateResponse HistoryUpdateResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public HistoryUpdateResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public HistoryUpdateResponseMessage(HistoryUpdateResponse HistoryUpdateResponse)
        {
            this.HistoryUpdateResponse = HistoryUpdateResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public HistoryUpdateResponseMessage(ServiceFault ServiceFault)
        {
            this.HistoryUpdateResponse = new HistoryUpdateResponse();

            if (ServiceFault != null)
            {
                this.HistoryUpdateResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region Call Service Messages
    #if (!OPCUA_EXCLUDE_Call)
    public partial class CallRequest : IServiceRequest
    {
    }

    public partial class CallResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the Call service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class CallMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public CallRequest CallRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public CallMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public CallMessage(CallRequest CallRequest)
        {
            this.CallRequest = CallRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return CallRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            CallResponse body = response as CallResponse;

            if (body == null)
            {
                body = new CallResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new CallResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the Call service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class CallResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public CallResponse CallResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public CallResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public CallResponseMessage(CallResponse CallResponse)
        {
            this.CallResponse = CallResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public CallResponseMessage(ServiceFault ServiceFault)
        {
            this.CallResponse = new CallResponse();

            if (ServiceFault != null)
            {
                this.CallResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region CreateMonitoredItems Service Messages
    #if (!OPCUA_EXCLUDE_CreateMonitoredItems)
    public partial class CreateMonitoredItemsRequest : IServiceRequest
    {
    }

    public partial class CreateMonitoredItemsResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the CreateMonitoredItems service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class CreateMonitoredItemsMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public CreateMonitoredItemsRequest CreateMonitoredItemsRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public CreateMonitoredItemsMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public CreateMonitoredItemsMessage(CreateMonitoredItemsRequest CreateMonitoredItemsRequest)
        {
            this.CreateMonitoredItemsRequest = CreateMonitoredItemsRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return CreateMonitoredItemsRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            CreateMonitoredItemsResponse body = response as CreateMonitoredItemsResponse;

            if (body == null)
            {
                body = new CreateMonitoredItemsResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new CreateMonitoredItemsResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the CreateMonitoredItems service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class CreateMonitoredItemsResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public CreateMonitoredItemsResponse CreateMonitoredItemsResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public CreateMonitoredItemsResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public CreateMonitoredItemsResponseMessage(CreateMonitoredItemsResponse CreateMonitoredItemsResponse)
        {
            this.CreateMonitoredItemsResponse = CreateMonitoredItemsResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public CreateMonitoredItemsResponseMessage(ServiceFault ServiceFault)
        {
            this.CreateMonitoredItemsResponse = new CreateMonitoredItemsResponse();

            if (ServiceFault != null)
            {
                this.CreateMonitoredItemsResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region ModifyMonitoredItems Service Messages
    #if (!OPCUA_EXCLUDE_ModifyMonitoredItems)
    public partial class ModifyMonitoredItemsRequest : IServiceRequest
    {
    }

    public partial class ModifyMonitoredItemsResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the ModifyMonitoredItems service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class ModifyMonitoredItemsMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public ModifyMonitoredItemsRequest ModifyMonitoredItemsRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public ModifyMonitoredItemsMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public ModifyMonitoredItemsMessage(ModifyMonitoredItemsRequest ModifyMonitoredItemsRequest)
        {
            this.ModifyMonitoredItemsRequest = ModifyMonitoredItemsRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return ModifyMonitoredItemsRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            ModifyMonitoredItemsResponse body = response as ModifyMonitoredItemsResponse;

            if (body == null)
            {
                body = new ModifyMonitoredItemsResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new ModifyMonitoredItemsResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the ModifyMonitoredItems service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class ModifyMonitoredItemsResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public ModifyMonitoredItemsResponse ModifyMonitoredItemsResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public ModifyMonitoredItemsResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public ModifyMonitoredItemsResponseMessage(ModifyMonitoredItemsResponse ModifyMonitoredItemsResponse)
        {
            this.ModifyMonitoredItemsResponse = ModifyMonitoredItemsResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public ModifyMonitoredItemsResponseMessage(ServiceFault ServiceFault)
        {
            this.ModifyMonitoredItemsResponse = new ModifyMonitoredItemsResponse();

            if (ServiceFault != null)
            {
                this.ModifyMonitoredItemsResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region SetMonitoringMode Service Messages
    #if (!OPCUA_EXCLUDE_SetMonitoringMode)
    public partial class SetMonitoringModeRequest : IServiceRequest
    {
    }

    public partial class SetMonitoringModeResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the SetMonitoringMode service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class SetMonitoringModeMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public SetMonitoringModeRequest SetMonitoringModeRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public SetMonitoringModeMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public SetMonitoringModeMessage(SetMonitoringModeRequest SetMonitoringModeRequest)
        {
            this.SetMonitoringModeRequest = SetMonitoringModeRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return SetMonitoringModeRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            SetMonitoringModeResponse body = response as SetMonitoringModeResponse;

            if (body == null)
            {
                body = new SetMonitoringModeResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new SetMonitoringModeResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the SetMonitoringMode service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class SetMonitoringModeResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public SetMonitoringModeResponse SetMonitoringModeResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public SetMonitoringModeResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public SetMonitoringModeResponseMessage(SetMonitoringModeResponse SetMonitoringModeResponse)
        {
            this.SetMonitoringModeResponse = SetMonitoringModeResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public SetMonitoringModeResponseMessage(ServiceFault ServiceFault)
        {
            this.SetMonitoringModeResponse = new SetMonitoringModeResponse();

            if (ServiceFault != null)
            {
                this.SetMonitoringModeResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region SetTriggering Service Messages
    #if (!OPCUA_EXCLUDE_SetTriggering)
    public partial class SetTriggeringRequest : IServiceRequest
    {
    }

    public partial class SetTriggeringResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the SetTriggering service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class SetTriggeringMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public SetTriggeringRequest SetTriggeringRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public SetTriggeringMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public SetTriggeringMessage(SetTriggeringRequest SetTriggeringRequest)
        {
            this.SetTriggeringRequest = SetTriggeringRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return SetTriggeringRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            SetTriggeringResponse body = response as SetTriggeringResponse;

            if (body == null)
            {
                body = new SetTriggeringResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new SetTriggeringResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the SetTriggering service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class SetTriggeringResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public SetTriggeringResponse SetTriggeringResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public SetTriggeringResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public SetTriggeringResponseMessage(SetTriggeringResponse SetTriggeringResponse)
        {
            this.SetTriggeringResponse = SetTriggeringResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public SetTriggeringResponseMessage(ServiceFault ServiceFault)
        {
            this.SetTriggeringResponse = new SetTriggeringResponse();

            if (ServiceFault != null)
            {
                this.SetTriggeringResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region DeleteMonitoredItems Service Messages
    #if (!OPCUA_EXCLUDE_DeleteMonitoredItems)
    public partial class DeleteMonitoredItemsRequest : IServiceRequest
    {
    }

    public partial class DeleteMonitoredItemsResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the DeleteMonitoredItems service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class DeleteMonitoredItemsMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public DeleteMonitoredItemsRequest DeleteMonitoredItemsRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public DeleteMonitoredItemsMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public DeleteMonitoredItemsMessage(DeleteMonitoredItemsRequest DeleteMonitoredItemsRequest)
        {
            this.DeleteMonitoredItemsRequest = DeleteMonitoredItemsRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return DeleteMonitoredItemsRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            DeleteMonitoredItemsResponse body = response as DeleteMonitoredItemsResponse;

            if (body == null)
            {
                body = new DeleteMonitoredItemsResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new DeleteMonitoredItemsResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the DeleteMonitoredItems service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class DeleteMonitoredItemsResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public DeleteMonitoredItemsResponse DeleteMonitoredItemsResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public DeleteMonitoredItemsResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public DeleteMonitoredItemsResponseMessage(DeleteMonitoredItemsResponse DeleteMonitoredItemsResponse)
        {
            this.DeleteMonitoredItemsResponse = DeleteMonitoredItemsResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public DeleteMonitoredItemsResponseMessage(ServiceFault ServiceFault)
        {
            this.DeleteMonitoredItemsResponse = new DeleteMonitoredItemsResponse();

            if (ServiceFault != null)
            {
                this.DeleteMonitoredItemsResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region CreateSubscription Service Messages
    #if (!OPCUA_EXCLUDE_CreateSubscription)
    public partial class CreateSubscriptionRequest : IServiceRequest
    {
    }

    public partial class CreateSubscriptionResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the CreateSubscription service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class CreateSubscriptionMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public CreateSubscriptionRequest CreateSubscriptionRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public CreateSubscriptionMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public CreateSubscriptionMessage(CreateSubscriptionRequest CreateSubscriptionRequest)
        {
            this.CreateSubscriptionRequest = CreateSubscriptionRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return CreateSubscriptionRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            CreateSubscriptionResponse body = response as CreateSubscriptionResponse;

            if (body == null)
            {
                body = new CreateSubscriptionResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new CreateSubscriptionResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the CreateSubscription service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class CreateSubscriptionResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public CreateSubscriptionResponse CreateSubscriptionResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public CreateSubscriptionResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public CreateSubscriptionResponseMessage(CreateSubscriptionResponse CreateSubscriptionResponse)
        {
            this.CreateSubscriptionResponse = CreateSubscriptionResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public CreateSubscriptionResponseMessage(ServiceFault ServiceFault)
        {
            this.CreateSubscriptionResponse = new CreateSubscriptionResponse();

            if (ServiceFault != null)
            {
                this.CreateSubscriptionResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region ModifySubscription Service Messages
    #if (!OPCUA_EXCLUDE_ModifySubscription)
    public partial class ModifySubscriptionRequest : IServiceRequest
    {
    }

    public partial class ModifySubscriptionResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the ModifySubscription service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class ModifySubscriptionMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public ModifySubscriptionRequest ModifySubscriptionRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public ModifySubscriptionMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public ModifySubscriptionMessage(ModifySubscriptionRequest ModifySubscriptionRequest)
        {
            this.ModifySubscriptionRequest = ModifySubscriptionRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return ModifySubscriptionRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            ModifySubscriptionResponse body = response as ModifySubscriptionResponse;

            if (body == null)
            {
                body = new ModifySubscriptionResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new ModifySubscriptionResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the ModifySubscription service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class ModifySubscriptionResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public ModifySubscriptionResponse ModifySubscriptionResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public ModifySubscriptionResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public ModifySubscriptionResponseMessage(ModifySubscriptionResponse ModifySubscriptionResponse)
        {
            this.ModifySubscriptionResponse = ModifySubscriptionResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public ModifySubscriptionResponseMessage(ServiceFault ServiceFault)
        {
            this.ModifySubscriptionResponse = new ModifySubscriptionResponse();

            if (ServiceFault != null)
            {
                this.ModifySubscriptionResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region SetPublishingMode Service Messages
    #if (!OPCUA_EXCLUDE_SetPublishingMode)
    public partial class SetPublishingModeRequest : IServiceRequest
    {
    }

    public partial class SetPublishingModeResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the SetPublishingMode service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class SetPublishingModeMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public SetPublishingModeRequest SetPublishingModeRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public SetPublishingModeMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public SetPublishingModeMessage(SetPublishingModeRequest SetPublishingModeRequest)
        {
            this.SetPublishingModeRequest = SetPublishingModeRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return SetPublishingModeRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            SetPublishingModeResponse body = response as SetPublishingModeResponse;

            if (body == null)
            {
                body = new SetPublishingModeResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new SetPublishingModeResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the SetPublishingMode service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class SetPublishingModeResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public SetPublishingModeResponse SetPublishingModeResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public SetPublishingModeResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public SetPublishingModeResponseMessage(SetPublishingModeResponse SetPublishingModeResponse)
        {
            this.SetPublishingModeResponse = SetPublishingModeResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public SetPublishingModeResponseMessage(ServiceFault ServiceFault)
        {
            this.SetPublishingModeResponse = new SetPublishingModeResponse();

            if (ServiceFault != null)
            {
                this.SetPublishingModeResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region Publish Service Messages
    #if (!OPCUA_EXCLUDE_Publish)
    public partial class PublishRequest : IServiceRequest
    {
    }

    public partial class PublishResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the Publish service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class PublishMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public PublishRequest PublishRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public PublishMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public PublishMessage(PublishRequest PublishRequest)
        {
            this.PublishRequest = PublishRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return PublishRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            PublishResponse body = response as PublishResponse;

            if (body == null)
            {
                body = new PublishResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new PublishResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the Publish service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class PublishResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public PublishResponse PublishResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public PublishResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public PublishResponseMessage(PublishResponse PublishResponse)
        {
            this.PublishResponse = PublishResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public PublishResponseMessage(ServiceFault ServiceFault)
        {
            this.PublishResponse = new PublishResponse();

            if (ServiceFault != null)
            {
                this.PublishResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region Republish Service Messages
    #if (!OPCUA_EXCLUDE_Republish)
    public partial class RepublishRequest : IServiceRequest
    {
    }

    public partial class RepublishResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the Republish service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class RepublishMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public RepublishRequest RepublishRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public RepublishMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public RepublishMessage(RepublishRequest RepublishRequest)
        {
            this.RepublishRequest = RepublishRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return RepublishRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            RepublishResponse body = response as RepublishResponse;

            if (body == null)
            {
                body = new RepublishResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new RepublishResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the Republish service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class RepublishResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public RepublishResponse RepublishResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public RepublishResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public RepublishResponseMessage(RepublishResponse RepublishResponse)
        {
            this.RepublishResponse = RepublishResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public RepublishResponseMessage(ServiceFault ServiceFault)
        {
            this.RepublishResponse = new RepublishResponse();

            if (ServiceFault != null)
            {
                this.RepublishResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region TransferSubscriptions Service Messages
    #if (!OPCUA_EXCLUDE_TransferSubscriptions)
    public partial class TransferSubscriptionsRequest : IServiceRequest
    {
    }

    public partial class TransferSubscriptionsResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the TransferSubscriptions service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class TransferSubscriptionsMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public TransferSubscriptionsRequest TransferSubscriptionsRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public TransferSubscriptionsMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public TransferSubscriptionsMessage(TransferSubscriptionsRequest TransferSubscriptionsRequest)
        {
            this.TransferSubscriptionsRequest = TransferSubscriptionsRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return TransferSubscriptionsRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            TransferSubscriptionsResponse body = response as TransferSubscriptionsResponse;

            if (body == null)
            {
                body = new TransferSubscriptionsResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new TransferSubscriptionsResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the TransferSubscriptions service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class TransferSubscriptionsResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public TransferSubscriptionsResponse TransferSubscriptionsResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public TransferSubscriptionsResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public TransferSubscriptionsResponseMessage(TransferSubscriptionsResponse TransferSubscriptionsResponse)
        {
            this.TransferSubscriptionsResponse = TransferSubscriptionsResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public TransferSubscriptionsResponseMessage(ServiceFault ServiceFault)
        {
            this.TransferSubscriptionsResponse = new TransferSubscriptionsResponse();

            if (ServiceFault != null)
            {
                this.TransferSubscriptionsResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion

    #region DeleteSubscriptions Service Messages
    #if (!OPCUA_EXCLUDE_DeleteSubscriptions)
    public partial class DeleteSubscriptionsRequest : IServiceRequest
    {
    }

    public partial class DeleteSubscriptionsResponse : IServiceResponse
    {
    }

    /// <summary>
    /// The message contract for the DeleteSubscriptions service.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class DeleteSubscriptionsMessage : IServiceMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace = Namespaces.OpcUaXsd, Order = 0)]
        public DeleteSubscriptionsRequest DeleteSubscriptionsRequest;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public DeleteSubscriptionsMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public DeleteSubscriptionsMessage(DeleteSubscriptionsRequest DeleteSubscriptionsRequest)
        {
            this.DeleteSubscriptionsRequest = DeleteSubscriptionsRequest;
        }

        #region IServiceMessage Members
        /// <summary cref="IServiceMessage.GetRequest" />
        public IServiceRequest GetRequest()
        {
            return DeleteSubscriptionsRequest;
        }

        /// <summary cref="IServiceMessage.CreateResponse" />
        public object CreateResponse(IServiceResponse response)
        {
            DeleteSubscriptionsResponse body = response as DeleteSubscriptionsResponse;

            if (body == null)
            {
                body = new DeleteSubscriptionsResponse();
                body.ResponseHeader = ((ServiceFault)response).ResponseHeader;
            }

            return new DeleteSubscriptionsResponseMessage(body);
        }
        #endregion
    }

    /// <summary>
    /// The message contract for the DeleteSubscriptions service response.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [MessageContract(IsWrapped=false)]
    public class DeleteSubscriptionsResponseMessage
    {
        /// <summary>
        /// The body of the message.
        /// </summary>
        [MessageBodyMember(Namespace=Namespaces.OpcUaXsd, Order=0)]
        public DeleteSubscriptionsResponse DeleteSubscriptionsResponse;

        /// <summary>
        /// Initializes an empty message.
        /// </summary>
        public DeleteSubscriptionsResponseMessage()
        {
        }

        /// <summary>
        /// Initializes the message with the body.
        /// </summary>
        public DeleteSubscriptionsResponseMessage(DeleteSubscriptionsResponse DeleteSubscriptionsResponse)
        {
            this.DeleteSubscriptionsResponse = DeleteSubscriptionsResponse;
        }

        /// <summary>
        /// Initializes the message with a service fault.
        /// </summary>
        public DeleteSubscriptionsResponseMessage(ServiceFault ServiceFault)
        {
            this.DeleteSubscriptionsResponse = new DeleteSubscriptionsResponse();

            if (ServiceFault != null)
            {
                this.DeleteSubscriptionsResponse.ResponseHeader = ServiceFault.ResponseHeader;
            }
        }
    }
    #endif
    #endregion
}
