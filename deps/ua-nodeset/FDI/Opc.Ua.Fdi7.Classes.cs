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
using System.Text;
using System.Xml;
using System.Runtime.Serialization;
using Opc.Ua.Di;
using Opc.Ua;

namespace Opc.Ua.Fdi7
{
    #region Foundation_H1State Class
    #if (!OPCUA_EXCLUDE_Foundation_H1State)
    /// <summary>
    /// Stores an instance of the Foundation_H1 ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class Foundation_H1State : ProtocolState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public Foundation_H1State(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.Foundation_H1, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAVAAAARm91bmRhdGlvbl9IMUluc3RhbmNl" +
           "AQFbBQEBWwVbBQAA/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        #endregion

        #region Private Fields
        #endregion
    }
    #endif
    #endregion

    #region Foundation_HSEState Class
    #if (!OPCUA_EXCLUDE_Foundation_HSEState)
    /// <summary>
    /// Stores an instance of the Foundation_HSE ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class Foundation_HSEState : ProtocolState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public Foundation_HSEState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.Foundation_HSE, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAWAAAARm91bmRhdGlvbl9IU0VJbnN0YW5j" +
           "ZQEBXAUBAVwFXAUAAP////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        #endregion

        #region Private Fields
        #endregion
    }
    #endif
    #endregion

    #region Profibus_DPState Class
    #if (!OPCUA_EXCLUDE_Profibus_DPState)
    /// <summary>
    /// Stores an instance of the Profibus_DP ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class Profibus_DPState : ProtocolState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public Profibus_DPState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.Profibus_DP, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQATAAAAUHJvZmlidXNfRFBJbnN0YW5jZQEB" +
           "XQUBAV0FXQUAAP////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        #endregion

        #region Private Fields
        #endregion
    }
    #endif
    #endregion

    #region Profibus_PAState Class
    #if (!OPCUA_EXCLUDE_Profibus_PAState)
    /// <summary>
    /// Stores an instance of the Profibus_PA ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class Profibus_PAState : ProtocolState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public Profibus_PAState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.Profibus_PA, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQATAAAAUHJvZmlidXNfUEFJbnN0YW5jZQEB" +
           "XgUBAV4FXgUAAP////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        #endregion

        #region Private Fields
        #endregion
    }
    #endif
    #endregion

    #region Profinet_IOState Class
    #if (!OPCUA_EXCLUDE_Profinet_IOState)
    /// <summary>
    /// Stores an instance of the Profinet_IO ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class Profinet_IOState : ProtocolState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public Profinet_IOState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.Profinet_IO, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQATAAAAUHJvZmluZXRfSU9JbnN0YW5jZQEB" +
           "XwUBAV8FXwUAAP////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        #endregion

        #region Private Fields
        #endregion
    }
    #endif
    #endregion

    #region HARTState Class
    #if (!OPCUA_EXCLUDE_HARTState)
    /// <summary>
    /// Stores an instance of the HART ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class HARTState : ProtocolState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public HARTState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.HART, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAMAAAASEFSVEluc3RhbmNlAQFgBQEBYAVg" +
           "BQAA/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        #endregion

        #region Private Fields
        #endregion
    }
    #endif
    #endregion

    #region ISA100_WirelessState Class
    #if (!OPCUA_EXCLUDE_ISA100_WirelessState)
    /// <summary>
    /// Stores an instance of the ISA100_Wireless ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ISA100_WirelessState : ProtocolState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ISA100_WirelessState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.ISA100_Wireless, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAXAAAASVNBMTAwX1dpcmVsZXNzSW5zdGFu" +
           "Y2UBAWEFAQFhBWEFAAD/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        #endregion

        #region Private Fields
        #endregion
    }
    #endif
    #endregion

    #region GenericProtocolState Class
    #if (!OPCUA_EXCLUDE_GenericProtocolState)
    /// <summary>
    /// Stores an instance of the GenericProtocol ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class GenericProtocolState : ProtocolState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public GenericProtocolState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.GenericProtocol, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAXAAAAR2VuZXJpY1Byb3RvY29sSW5zdGFu" +
           "Y2UBAWIFAQFiBWIFAAD/////AQAAABVgiQoCAAAAAQASAAAAUHJvdG9jb2xJZGVudGlmaWVyAQFjBQAu" +
           "AERjBQAAAAz/////AQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public PropertyState<string> ProtocolIdentifier
        {
            get
            {
                return m_protocolIdentifier;
            }

            set
            {
                if (!Object.ReferenceEquals(m_protocolIdentifier, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_protocolIdentifier = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_protocolIdentifier != null)
            {
                children.Add(m_protocolIdentifier);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Opc.Ua.Fdi7.BrowseNames.ProtocolIdentifier:
                {
                    if (createOrReplace)
                    {
                        if (ProtocolIdentifier == null)
                        {
                            if (replacement == null)
                            {
                                ProtocolIdentifier = new PropertyState<string>(this);
                            }
                            else
                            {
                                ProtocolIdentifier = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = ProtocolIdentifier;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private PropertyState<string> m_protocolIdentifier;
        #endregion
    }
    #endif
    #endregion

    #region ConnectionPoint_Foundation_H1State Class
    #if (!OPCUA_EXCLUDE_ConnectionPoint_Foundation_H1State)
    /// <summary>
    /// Stores an instance of the ConnectionPoint_Foundation_H1 ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ConnectionPoint_Foundation_H1State : ConnectionPointState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ConnectionPoint_Foundation_H1State(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.ConnectionPoint_Foundation_H1, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);

            if (OrdinalNumber != null)
            {
                OrdinalNumber.Initialize(context, OrdinalNumber_InitializationString);
            }

            if (SIFConnection != null)
            {
                SIFConnection.Initialize(context, SIFConnection_InitializationString);
            }
        }

        #region Initialization String
        private const string OrdinalNumber_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////xVgiQoCAAAAAQANAAAAT3JkaW5hbE51bWJlcgEBjgUALgBE" +
           "jgUAAAAG/////wEB/////wAAAAA=";

        private const string SIFConnection_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////xVgiQoCAAAAAQANAAAAU0lGQ29ubmVjdGlvbgEBjwUALgBE" +
           "jwUAAAAB/////wEB/////wAAAAA=";

        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAlAAAAQ29ubmVjdGlvblBvaW50X0ZvdW5k" +
           "YXRpb25fSDFJbnN0YW5jZQEBZAUBAWQFZAUAAP////8EAAAAJGCACgEAAAACAA4AAABOZXR3b3JrQWRk" +
           "cmVzcwEBewUDAAAAACoAAABUaGUgYWRkcmVzcyBvZiB0aGUgZGV2aWNlIG9uIHRoaXMgbmV0d29yay4A" +
           "LwEC7QN7BQAA/////wAAAAAVYIkKAgAAAAEABwAAAEFkZHJlc3MBAY0FAC4ARI0FAAAAA/////8BAf//" +
           "//8AAAAAFWCJCgIAAAABAA0AAABPcmRpbmFsTnVtYmVyAQGOBQAuAESOBQAAAAb/////AQH/////AAAA" +
           "ABVgiQoCAAAAAQANAAAAU0lGQ29ubmVjdGlvbgEBjwUALgBEjwUAAAAB/////wEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public PropertyState<byte> Address
        {
            get
            {
                return m_address;
            }

            set
            {
                if (!Object.ReferenceEquals(m_address, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_address = value;
            }
        }

        /// <remarks />
        public PropertyState<int> OrdinalNumber
        {
            get
            {
                return m_ordinalNumber;
            }

            set
            {
                if (!Object.ReferenceEquals(m_ordinalNumber, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_ordinalNumber = value;
            }
        }

        /// <remarks />
        public PropertyState<bool> SIFConnection
        {
            get
            {
                return m_sIFConnection;
            }

            set
            {
                if (!Object.ReferenceEquals(m_sIFConnection, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_sIFConnection = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_address != null)
            {
                children.Add(m_address);
            }

            if (m_ordinalNumber != null)
            {
                children.Add(m_ordinalNumber);
            }

            if (m_sIFConnection != null)
            {
                children.Add(m_sIFConnection);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Opc.Ua.Fdi7.BrowseNames.Address:
                {
                    if (createOrReplace)
                    {
                        if (Address == null)
                        {
                            if (replacement == null)
                            {
                                Address = new PropertyState<byte>(this);
                            }
                            else
                            {
                                Address = (PropertyState<byte>)replacement;
                            }
                        }
                    }

                    instance = Address;
                    break;
                }

                case Opc.Ua.Fdi7.BrowseNames.OrdinalNumber:
                {
                    if (createOrReplace)
                    {
                        if (OrdinalNumber == null)
                        {
                            if (replacement == null)
                            {
                                OrdinalNumber = new PropertyState<int>(this);
                            }
                            else
                            {
                                OrdinalNumber = (PropertyState<int>)replacement;
                            }
                        }
                    }

                    instance = OrdinalNumber;
                    break;
                }

                case Opc.Ua.Fdi7.BrowseNames.SIFConnection:
                {
                    if (createOrReplace)
                    {
                        if (SIFConnection == null)
                        {
                            if (replacement == null)
                            {
                                SIFConnection = new PropertyState<bool>(this);
                            }
                            else
                            {
                                SIFConnection = (PropertyState<bool>)replacement;
                            }
                        }
                    }

                    instance = SIFConnection;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private PropertyState<byte> m_address;
        private PropertyState<int> m_ordinalNumber;
        private PropertyState<bool> m_sIFConnection;
        #endregion
    }
    #endif
    #endregion

    #region ConnectionPoint_Foundation_HSEState Class
    #if (!OPCUA_EXCLUDE_ConnectionPoint_Foundation_HSEState)
    /// <summary>
    /// Stores an instance of the ConnectionPoint_Foundation_HSE ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ConnectionPoint_Foundation_HSEState : ConnectionPointState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ConnectionPoint_Foundation_HSEState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.ConnectionPoint_Foundation_HSE, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);

            if (OrdinalNumber != null)
            {
                OrdinalNumber.Initialize(context, OrdinalNumber_InitializationString);
            }
        }

        #region Initialization String
        private const string OrdinalNumber_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////xVgiQoCAAAAAQANAAAAT3JkaW5hbE51bWJlcgEBugUALgBE" +
           "ugUAAAAG/////wEB/////wAAAAA=";

        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAmAAAAQ29ubmVjdGlvblBvaW50X0ZvdW5k" +
           "YXRpb25fSFNFSW5zdGFuY2UBAZAFAQGQBZAFAAD/////AwAAACRggAoBAAAAAgAOAAAATmV0d29ya0Fk" +
           "ZHJlc3MBAacFAwAAAAAqAAAAVGhlIGFkZHJlc3Mgb2YgdGhlIGRldmljZSBvbiB0aGlzIG5ldHdvcmsu" +
           "AC8BAu0DpwUAAP////8AAAAAF2CJCgIAAAABAAcAAABBZGRyZXNzAQG5BQAuAES5BQAAAAMBAAAAAQAA" +
           "ABAAAAABAf////8AAAAAFWCJCgIAAAABAA0AAABPcmRpbmFsTnVtYmVyAQG6BQAuAES6BQAAAAb/////" +
           "AQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public PropertyState<byte[]> Address
        {
            get
            {
                return m_address;
            }

            set
            {
                if (!Object.ReferenceEquals(m_address, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_address = value;
            }
        }

        /// <remarks />
        public PropertyState<int> OrdinalNumber
        {
            get
            {
                return m_ordinalNumber;
            }

            set
            {
                if (!Object.ReferenceEquals(m_ordinalNumber, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_ordinalNumber = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_address != null)
            {
                children.Add(m_address);
            }

            if (m_ordinalNumber != null)
            {
                children.Add(m_ordinalNumber);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Opc.Ua.Fdi7.BrowseNames.Address:
                {
                    if (createOrReplace)
                    {
                        if (Address == null)
                        {
                            if (replacement == null)
                            {
                                Address = new PropertyState<byte[]>(this);
                            }
                            else
                            {
                                Address = (PropertyState<byte[]>)replacement;
                            }
                        }
                    }

                    instance = Address;
                    break;
                }

                case Opc.Ua.Fdi7.BrowseNames.OrdinalNumber:
                {
                    if (createOrReplace)
                    {
                        if (OrdinalNumber == null)
                        {
                            if (replacement == null)
                            {
                                OrdinalNumber = new PropertyState<int>(this);
                            }
                            else
                            {
                                OrdinalNumber = (PropertyState<int>)replacement;
                            }
                        }
                    }

                    instance = OrdinalNumber;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private PropertyState<byte[]> m_address;
        private PropertyState<int> m_ordinalNumber;
        #endregion
    }
    #endif
    #endregion

    #region ConnectionPoint_Profibus_DPState Class
    #if (!OPCUA_EXCLUDE_ConnectionPoint_Profibus_DPState)
    /// <summary>
    /// Stores an instance of the ConnectionPoint_Profibus_DP ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ConnectionPoint_Profibus_DPState : ConnectionPointState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ConnectionPoint_Profibus_DPState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.ConnectionPoint_Profibus_DP, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAjAAAAQ29ubmVjdGlvblBvaW50X1Byb2Zp" +
           "YnVzX0RQSW5zdGFuY2UBAbsFAQG7BbsFAAD/////AgAAACRggAoBAAAAAgAOAAAATmV0d29ya0FkZHJl" +
           "c3MBAdIFAwAAAAAqAAAAVGhlIGFkZHJlc3Mgb2YgdGhlIGRldmljZSBvbiB0aGlzIG5ldHdvcmsuAC8B" +
           "Au0D0gUAAP////8AAAAAFWCJCgIAAAABAAcAAABBZGRyZXNzAQHkBQAuAETkBQAAAAP/////AQH/////" +
           "AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public PropertyState<byte> Address
        {
            get
            {
                return m_address;
            }

            set
            {
                if (!Object.ReferenceEquals(m_address, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_address = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_address != null)
            {
                children.Add(m_address);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Opc.Ua.Fdi7.BrowseNames.Address:
                {
                    if (createOrReplace)
                    {
                        if (Address == null)
                        {
                            if (replacement == null)
                            {
                                Address = new PropertyState<byte>(this);
                            }
                            else
                            {
                                Address = (PropertyState<byte>)replacement;
                            }
                        }
                    }

                    instance = Address;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private PropertyState<byte> m_address;
        #endregion
    }
    #endif
    #endregion

    #region ConnectionPoint_Profinet_IOState Class
    #if (!OPCUA_EXCLUDE_ConnectionPoint_Profinet_IOState)
    /// <summary>
    /// Stores an instance of the ConnectionPoint_Profinet_IO ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ConnectionPoint_Profinet_IOState : ConnectionPointState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ConnectionPoint_Profinet_IOState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.ConnectionPoint_Profinet_IO, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAjAAAAQ29ubmVjdGlvblBvaW50X1Byb2Zp" +
           "bmV0X0lPSW5zdGFuY2UBAeUFAQHlBeUFAAD/////BQAAACRggAoBAAAAAgAOAAAATmV0d29ya0FkZHJl" +
           "c3MBAfwFAwAAAAAqAAAAVGhlIGFkZHJlc3Mgb2YgdGhlIGRldmljZSBvbiB0aGlzIG5ldHdvcmsuAC8B" +
           "Au0D/AUAAP////8AAAAAF2CJCgIAAAABAAMAAABNQUMBAQ4GAC4ARA4GAAAAAwEAAAABAAAAEAAAAAEB" +
           "/////wAAAAAXYIkKAgAAAAEABAAAAElQdjQBAQ8GAC4ARA8GAAAAAwEAAAABAAAABAAAAAEB/////wAA" +
           "AAAVYIkKAgAAAAEABwAAAEROU05BTUUBARAGAC4ARBAGAAAADP////8BAf////8AAAAAFWCJCgIAAAAB" +
           "AAUAAABWQUxJRAEBEQYALgBEEQYAAAAB/////wEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public PropertyState<byte[]> MAC
        {
            get
            {
                return m_mAC;
            }

            set
            {
                if (!Object.ReferenceEquals(m_mAC, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_mAC = value;
            }
        }

        /// <remarks />
        public PropertyState<byte[]> IPv4
        {
            get
            {
                return m_iPv4;
            }

            set
            {
                if (!Object.ReferenceEquals(m_iPv4, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_iPv4 = value;
            }
        }

        /// <remarks />
        public PropertyState<string> DNSNAME
        {
            get
            {
                return m_dNSNAME;
            }

            set
            {
                if (!Object.ReferenceEquals(m_dNSNAME, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_dNSNAME = value;
            }
        }

        /// <remarks />
        public PropertyState<bool> VALID
        {
            get
            {
                return m_vALID;
            }

            set
            {
                if (!Object.ReferenceEquals(m_vALID, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_vALID = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_mAC != null)
            {
                children.Add(m_mAC);
            }

            if (m_iPv4 != null)
            {
                children.Add(m_iPv4);
            }

            if (m_dNSNAME != null)
            {
                children.Add(m_dNSNAME);
            }

            if (m_vALID != null)
            {
                children.Add(m_vALID);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Opc.Ua.Fdi7.BrowseNames.MAC:
                {
                    if (createOrReplace)
                    {
                        if (MAC == null)
                        {
                            if (replacement == null)
                            {
                                MAC = new PropertyState<byte[]>(this);
                            }
                            else
                            {
                                MAC = (PropertyState<byte[]>)replacement;
                            }
                        }
                    }

                    instance = MAC;
                    break;
                }

                case Opc.Ua.Fdi7.BrowseNames.IPv4:
                {
                    if (createOrReplace)
                    {
                        if (IPv4 == null)
                        {
                            if (replacement == null)
                            {
                                IPv4 = new PropertyState<byte[]>(this);
                            }
                            else
                            {
                                IPv4 = (PropertyState<byte[]>)replacement;
                            }
                        }
                    }

                    instance = IPv4;
                    break;
                }

                case Opc.Ua.Fdi7.BrowseNames.DNSNAME:
                {
                    if (createOrReplace)
                    {
                        if (DNSNAME == null)
                        {
                            if (replacement == null)
                            {
                                DNSNAME = new PropertyState<string>(this);
                            }
                            else
                            {
                                DNSNAME = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = DNSNAME;
                    break;
                }

                case Opc.Ua.Fdi7.BrowseNames.VALID:
                {
                    if (createOrReplace)
                    {
                        if (VALID == null)
                        {
                            if (replacement == null)
                            {
                                VALID = new PropertyState<bool>(this);
                            }
                            else
                            {
                                VALID = (PropertyState<bool>)replacement;
                            }
                        }
                    }

                    instance = VALID;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private PropertyState<byte[]> m_mAC;
        private PropertyState<byte[]> m_iPv4;
        private PropertyState<string> m_dNSNAME;
        private PropertyState<bool> m_vALID;
        #endregion
    }
    #endif
    #endregion

    #region ConnectionPoint_HART_TP5State Class
    #if (!OPCUA_EXCLUDE_ConnectionPoint_HART_TP5State)
    /// <summary>
    /// Stores an instance of the ConnectionPoint_HART_TP5 ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ConnectionPoint_HART_TP5State : ConnectionPointState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ConnectionPoint_HART_TP5State(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.ConnectionPoint_HART_TP5, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);

            if (DevMfg != null)
            {
                DevMfg.Initialize(context, DevMfg_InitializationString);
            }

            if (DevType != null)
            {
                DevType.Initialize(context, DevType_InitializationString);
            }

            if (DevRev != null)
            {
                DevRev.Initialize(context, DevRev_InitializationString);
            }

            if (DevTag != null)
            {
                DevTag.Initialize(context, DevTag_InitializationString);
            }

            if (DevPollAddr != null)
            {
                DevPollAddr.Initialize(context, DevPollAddr_InitializationString);
            }
        }

        #region Initialization String
        private const string DevMfg_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////xVgiQoCAAAAAQAGAAAARGV2TWZnAQE8BgAuAEQ8BgAAAAX/" +
           "////AQH/////AAAAAA==";

        private const string DevType_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////xVgiQoCAAAAAQAHAAAARGV2VHlwZQEBPQYALgBEPQYAAAAF" +
           "/////wEB/////wAAAAA=";

        private const string DevRev_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////xVgiQoCAAAAAQAGAAAARGV2UmV2AQE+BgAuAEQ+BgAAAAX/" +
           "////AQH/////AAAAAA==";

        private const string DevTag_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////xVgiQoCAAAAAQAGAAAARGV2VGFnAQE/BgAuAEQ/BgAAAAz/" +
           "////AQH/////AAAAAA==";

        private const string DevPollAddr_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////xVgiQoCAAAAAQALAAAARGV2UG9sbEFkZHIBAUAGAC4AREAG" +
           "AAAAA/////8BAf////8AAAAA";

        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAgAAAAQ29ubmVjdGlvblBvaW50X0hBUlRf" +
           "VFA1SW5zdGFuY2UBARIGAQESBhIGAAD/////BwAAACRggAoBAAAAAgAOAAAATmV0d29ya0FkZHJlc3MB" +
           "ASkGAwAAAAAqAAAAVGhlIGFkZHJlc3Mgb2YgdGhlIGRldmljZSBvbiB0aGlzIG5ldHdvcmsuAC8BAu0D" +
           "KQYAAP////8AAAAAF2CJCgIAAAABAAcAAABEZXZBZGRyAQE7BgAuAEQ7BgAAAAMBAAAAAQAAAAUAAAAB" +
           "Af////8AAAAAFWCJCgIAAAABAAYAAABEZXZNZmcBATwGAC4ARDwGAAAABf////8BAf////8AAAAAFWCJ" +
           "CgIAAAABAAcAAABEZXZUeXBlAQE9BgAuAEQ9BgAAAAX/////AQH/////AAAAABVgiQoCAAAAAQAGAAAA" +
           "RGV2UmV2AQE+BgAuAEQ+BgAAAAX/////AQH/////AAAAABVgiQoCAAAAAQAGAAAARGV2VGFnAQE/BgAu" +
           "AEQ/BgAAAAz/////AQH/////AAAAABVgiQoCAAAAAQALAAAARGV2UG9sbEFkZHIBAUAGAC4AREAGAAAA" +
           "A/////8BAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public PropertyState<byte[]> DevAddr
        {
            get
            {
                return m_devAddr;
            }

            set
            {
                if (!Object.ReferenceEquals(m_devAddr, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_devAddr = value;
            }
        }

        /// <remarks />
        public PropertyState<ushort> DevMfg
        {
            get
            {
                return m_devMfg;
            }

            set
            {
                if (!Object.ReferenceEquals(m_devMfg, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_devMfg = value;
            }
        }

        /// <remarks />
        public PropertyState<ushort> DevType
        {
            get
            {
                return m_devType;
            }

            set
            {
                if (!Object.ReferenceEquals(m_devType, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_devType = value;
            }
        }

        /// <remarks />
        public PropertyState<ushort> DevRev
        {
            get
            {
                return m_devRev;
            }

            set
            {
                if (!Object.ReferenceEquals(m_devRev, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_devRev = value;
            }
        }

        /// <remarks />
        public PropertyState<string> DevTag
        {
            get
            {
                return m_devTag;
            }

            set
            {
                if (!Object.ReferenceEquals(m_devTag, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_devTag = value;
            }
        }

        /// <remarks />
        public PropertyState<byte> DevPollAddr
        {
            get
            {
                return m_devPollAddr;
            }

            set
            {
                if (!Object.ReferenceEquals(m_devPollAddr, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_devPollAddr = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_devAddr != null)
            {
                children.Add(m_devAddr);
            }

            if (m_devMfg != null)
            {
                children.Add(m_devMfg);
            }

            if (m_devType != null)
            {
                children.Add(m_devType);
            }

            if (m_devRev != null)
            {
                children.Add(m_devRev);
            }

            if (m_devTag != null)
            {
                children.Add(m_devTag);
            }

            if (m_devPollAddr != null)
            {
                children.Add(m_devPollAddr);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Opc.Ua.Fdi7.BrowseNames.DevAddr:
                {
                    if (createOrReplace)
                    {
                        if (DevAddr == null)
                        {
                            if (replacement == null)
                            {
                                DevAddr = new PropertyState<byte[]>(this);
                            }
                            else
                            {
                                DevAddr = (PropertyState<byte[]>)replacement;
                            }
                        }
                    }

                    instance = DevAddr;
                    break;
                }

                case Opc.Ua.Fdi7.BrowseNames.DevMfg:
                {
                    if (createOrReplace)
                    {
                        if (DevMfg == null)
                        {
                            if (replacement == null)
                            {
                                DevMfg = new PropertyState<ushort>(this);
                            }
                            else
                            {
                                DevMfg = (PropertyState<ushort>)replacement;
                            }
                        }
                    }

                    instance = DevMfg;
                    break;
                }

                case Opc.Ua.Fdi7.BrowseNames.DevType:
                {
                    if (createOrReplace)
                    {
                        if (DevType == null)
                        {
                            if (replacement == null)
                            {
                                DevType = new PropertyState<ushort>(this);
                            }
                            else
                            {
                                DevType = (PropertyState<ushort>)replacement;
                            }
                        }
                    }

                    instance = DevType;
                    break;
                }

                case Opc.Ua.Fdi7.BrowseNames.DevRev:
                {
                    if (createOrReplace)
                    {
                        if (DevRev == null)
                        {
                            if (replacement == null)
                            {
                                DevRev = new PropertyState<ushort>(this);
                            }
                            else
                            {
                                DevRev = (PropertyState<ushort>)replacement;
                            }
                        }
                    }

                    instance = DevRev;
                    break;
                }

                case Opc.Ua.Fdi7.BrowseNames.DevTag:
                {
                    if (createOrReplace)
                    {
                        if (DevTag == null)
                        {
                            if (replacement == null)
                            {
                                DevTag = new PropertyState<string>(this);
                            }
                            else
                            {
                                DevTag = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = DevTag;
                    break;
                }

                case Opc.Ua.Fdi7.BrowseNames.DevPollAddr:
                {
                    if (createOrReplace)
                    {
                        if (DevPollAddr == null)
                        {
                            if (replacement == null)
                            {
                                DevPollAddr = new PropertyState<byte>(this);
                            }
                            else
                            {
                                DevPollAddr = (PropertyState<byte>)replacement;
                            }
                        }
                    }

                    instance = DevPollAddr;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private PropertyState<byte[]> m_devAddr;
        private PropertyState<ushort> m_devMfg;
        private PropertyState<ushort> m_devType;
        private PropertyState<ushort> m_devRev;
        private PropertyState<string> m_devTag;
        private PropertyState<byte> m_devPollAddr;
        #endregion
    }
    #endif
    #endregion

    #region ConnectionPoint_HART_TP6State Class
    #if (!OPCUA_EXCLUDE_ConnectionPoint_HART_TP6State)
    /// <summary>
    /// Stores an instance of the ConnectionPoint_HART_TP6 ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ConnectionPoint_HART_TP6State : ConnectionPointState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ConnectionPoint_HART_TP6State(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.ConnectionPoint_HART_TP6, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);

            if (DevMfg != null)
            {
                DevMfg.Initialize(context, DevMfg_InitializationString);
            }

            if (DevType != null)
            {
                DevType.Initialize(context, DevType_InitializationString);
            }

            if (DevRev != null)
            {
                DevRev.Initialize(context, DevRev_InitializationString);
            }

            if (DevTag != null)
            {
                DevTag.Initialize(context, DevTag_InitializationString);
            }

            if (DevPollAddr != null)
            {
                DevPollAddr.Initialize(context, DevPollAddr_InitializationString);
            }
        }

        #region Initialization String
        private const string DevMfg_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////xVgiQoCAAAAAQAGAAAARGV2TWZnAQFrBgAuAERrBgAAAAX/" +
           "////AQH/////AAAAAA==";

        private const string DevType_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////xVgiQoCAAAAAQAHAAAARGV2VHlwZQEBbAYALgBEbAYAAAAF" +
           "/////wEB/////wAAAAA=";

        private const string DevRev_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////xVgiQoCAAAAAQAGAAAARGV2UmV2AQFtBgAuAERtBgAAAAX/" +
           "////AQH/////AAAAAA==";

        private const string DevTag_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////xVgiQoCAAAAAQAGAAAARGV2VGFnAQFuBgAuAERuBgAAAAz/" +
           "////AQH/////AAAAAA==";

        private const string DevPollAddr_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////xVgiQoCAAAAAQALAAAARGV2UG9sbEFkZHIBAW8GAC4ARG8G" +
           "AAAAA/////8BAf////8AAAAA";

        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAgAAAAQ29ubmVjdGlvblBvaW50X0hBUlRf" +
           "VFA2SW5zdGFuY2UBAUEGAQFBBkEGAAD/////BwAAACRggAoBAAAAAgAOAAAATmV0d29ya0FkZHJlc3MB" +
           "AVgGAwAAAAAqAAAAVGhlIGFkZHJlc3Mgb2YgdGhlIGRldmljZSBvbiB0aGlzIG5ldHdvcmsuAC8BAu0D" +
           "WAYAAP////8AAAAAF2CJCgIAAAABAAcAAABEZXZBZGRyAQFqBgAuAERqBgAAAAMBAAAAAQAAAAUAAAAB" +
           "Af////8AAAAAFWCJCgIAAAABAAYAAABEZXZNZmcBAWsGAC4ARGsGAAAABf////8BAf////8AAAAAFWCJ" +
           "CgIAAAABAAcAAABEZXZUeXBlAQFsBgAuAERsBgAAAAX/////AQH/////AAAAABVgiQoCAAAAAQAGAAAA" +
           "RGV2UmV2AQFtBgAuAERtBgAAAAX/////AQH/////AAAAABVgiQoCAAAAAQAGAAAARGV2VGFnAQFuBgAu" +
           "AERuBgAAAAz/////AQH/////AAAAABVgiQoCAAAAAQALAAAARGV2UG9sbEFkZHIBAW8GAC4ARG8GAAAA" +
           "A/////8BAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public PropertyState<byte[]> DevAddr
        {
            get
            {
                return m_devAddr;
            }

            set
            {
                if (!Object.ReferenceEquals(m_devAddr, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_devAddr = value;
            }
        }

        /// <remarks />
        public PropertyState<ushort> DevMfg
        {
            get
            {
                return m_devMfg;
            }

            set
            {
                if (!Object.ReferenceEquals(m_devMfg, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_devMfg = value;
            }
        }

        /// <remarks />
        public PropertyState<ushort> DevType
        {
            get
            {
                return m_devType;
            }

            set
            {
                if (!Object.ReferenceEquals(m_devType, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_devType = value;
            }
        }

        /// <remarks />
        public PropertyState<ushort> DevRev
        {
            get
            {
                return m_devRev;
            }

            set
            {
                if (!Object.ReferenceEquals(m_devRev, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_devRev = value;
            }
        }

        /// <remarks />
        public PropertyState<string> DevTag
        {
            get
            {
                return m_devTag;
            }

            set
            {
                if (!Object.ReferenceEquals(m_devTag, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_devTag = value;
            }
        }

        /// <remarks />
        public PropertyState<byte> DevPollAddr
        {
            get
            {
                return m_devPollAddr;
            }

            set
            {
                if (!Object.ReferenceEquals(m_devPollAddr, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_devPollAddr = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_devAddr != null)
            {
                children.Add(m_devAddr);
            }

            if (m_devMfg != null)
            {
                children.Add(m_devMfg);
            }

            if (m_devType != null)
            {
                children.Add(m_devType);
            }

            if (m_devRev != null)
            {
                children.Add(m_devRev);
            }

            if (m_devTag != null)
            {
                children.Add(m_devTag);
            }

            if (m_devPollAddr != null)
            {
                children.Add(m_devPollAddr);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Opc.Ua.Fdi7.BrowseNames.DevAddr:
                {
                    if (createOrReplace)
                    {
                        if (DevAddr == null)
                        {
                            if (replacement == null)
                            {
                                DevAddr = new PropertyState<byte[]>(this);
                            }
                            else
                            {
                                DevAddr = (PropertyState<byte[]>)replacement;
                            }
                        }
                    }

                    instance = DevAddr;
                    break;
                }

                case Opc.Ua.Fdi7.BrowseNames.DevMfg:
                {
                    if (createOrReplace)
                    {
                        if (DevMfg == null)
                        {
                            if (replacement == null)
                            {
                                DevMfg = new PropertyState<ushort>(this);
                            }
                            else
                            {
                                DevMfg = (PropertyState<ushort>)replacement;
                            }
                        }
                    }

                    instance = DevMfg;
                    break;
                }

                case Opc.Ua.Fdi7.BrowseNames.DevType:
                {
                    if (createOrReplace)
                    {
                        if (DevType == null)
                        {
                            if (replacement == null)
                            {
                                DevType = new PropertyState<ushort>(this);
                            }
                            else
                            {
                                DevType = (PropertyState<ushort>)replacement;
                            }
                        }
                    }

                    instance = DevType;
                    break;
                }

                case Opc.Ua.Fdi7.BrowseNames.DevRev:
                {
                    if (createOrReplace)
                    {
                        if (DevRev == null)
                        {
                            if (replacement == null)
                            {
                                DevRev = new PropertyState<ushort>(this);
                            }
                            else
                            {
                                DevRev = (PropertyState<ushort>)replacement;
                            }
                        }
                    }

                    instance = DevRev;
                    break;
                }

                case Opc.Ua.Fdi7.BrowseNames.DevTag:
                {
                    if (createOrReplace)
                    {
                        if (DevTag == null)
                        {
                            if (replacement == null)
                            {
                                DevTag = new PropertyState<string>(this);
                            }
                            else
                            {
                                DevTag = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = DevTag;
                    break;
                }

                case Opc.Ua.Fdi7.BrowseNames.DevPollAddr:
                {
                    if (createOrReplace)
                    {
                        if (DevPollAddr == null)
                        {
                            if (replacement == null)
                            {
                                DevPollAddr = new PropertyState<byte>(this);
                            }
                            else
                            {
                                DevPollAddr = (PropertyState<byte>)replacement;
                            }
                        }
                    }

                    instance = DevPollAddr;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private PropertyState<byte[]> m_devAddr;
        private PropertyState<ushort> m_devMfg;
        private PropertyState<ushort> m_devType;
        private PropertyState<ushort> m_devRev;
        private PropertyState<string> m_devTag;
        private PropertyState<byte> m_devPollAddr;
        #endregion
    }
    #endif
    #endregion

    #region ConnectionPoint_HART_TP7State Class
    #if (!OPCUA_EXCLUDE_ConnectionPoint_HART_TP7State)
    /// <summary>
    /// Stores an instance of the ConnectionPoint_HART_TP7 ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ConnectionPoint_HART_TP7State : ConnectionPointState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ConnectionPoint_HART_TP7State(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.ConnectionPoint_HART_TP7, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);

            if (DevMfg != null)
            {
                DevMfg.Initialize(context, DevMfg_InitializationString);
            }

            if (DevType != null)
            {
                DevType.Initialize(context, DevType_InitializationString);
            }

            if (DevRev != null)
            {
                DevRev.Initialize(context, DevRev_InitializationString);
            }

            if (DevTag != null)
            {
                DevTag.Initialize(context, DevTag_InitializationString);
            }

            if (DevPollAddr != null)
            {
                DevPollAddr.Initialize(context, DevPollAddr_InitializationString);
            }
        }

        #region Initialization String
        private const string DevMfg_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////xVgiQoCAAAAAQAGAAAARGV2TWZnAQGaBgAuAESaBgAAAAX/" +
           "////AQH/////AAAAAA==";

        private const string DevType_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////xVgiQoCAAAAAQAHAAAARGV2VHlwZQEBmwYALgBEmwYAAAAF" +
           "/////wEB/////wAAAAA=";

        private const string DevRev_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////xVgiQoCAAAAAQAGAAAARGV2UmV2AQGcBgAuAEScBgAAAAX/" +
           "////AQH/////AAAAAA==";

        private const string DevTag_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////xVgiQoCAAAAAQAGAAAARGV2VGFnAQGdBgAuAESdBgAAAAz/" +
           "////AQH/////AAAAAA==";

        private const string DevPollAddr_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////xVgiQoCAAAAAQALAAAARGV2UG9sbEFkZHIBAZ4GAC4ARJ4G" +
           "AAAAA/////8BAf////8AAAAA";

        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAgAAAAQ29ubmVjdGlvblBvaW50X0hBUlRf" +
           "VFA3SW5zdGFuY2UBAXAGAQFwBnAGAAD/////BwAAACRggAoBAAAAAgAOAAAATmV0d29ya0FkZHJlc3MB" +
           "AYcGAwAAAAAqAAAAVGhlIGFkZHJlc3Mgb2YgdGhlIGRldmljZSBvbiB0aGlzIG5ldHdvcmsuAC8BAu0D" +
           "hwYAAP////8AAAAAF2CJCgIAAAABAAcAAABEZXZBZGRyAQGZBgAuAESZBgAAAAMBAAAAAQAAAAUAAAAB" +
           "Af////8AAAAAFWCJCgIAAAABAAYAAABEZXZNZmcBAZoGAC4ARJoGAAAABf////8BAf////8AAAAAFWCJ" +
           "CgIAAAABAAcAAABEZXZUeXBlAQGbBgAuAESbBgAAAAX/////AQH/////AAAAABVgiQoCAAAAAQAGAAAA" +
           "RGV2UmV2AQGcBgAuAEScBgAAAAX/////AQH/////AAAAABVgiQoCAAAAAQAGAAAARGV2VGFnAQGdBgAu" +
           "AESdBgAAAAz/////AQH/////AAAAABVgiQoCAAAAAQALAAAARGV2UG9sbEFkZHIBAZ4GAC4ARJ4GAAAA" +
           "A/////8BAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public PropertyState<byte[]> DevAddr
        {
            get
            {
                return m_devAddr;
            }

            set
            {
                if (!Object.ReferenceEquals(m_devAddr, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_devAddr = value;
            }
        }

        /// <remarks />
        public PropertyState<ushort> DevMfg
        {
            get
            {
                return m_devMfg;
            }

            set
            {
                if (!Object.ReferenceEquals(m_devMfg, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_devMfg = value;
            }
        }

        /// <remarks />
        public PropertyState<ushort> DevType
        {
            get
            {
                return m_devType;
            }

            set
            {
                if (!Object.ReferenceEquals(m_devType, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_devType = value;
            }
        }

        /// <remarks />
        public PropertyState<ushort> DevRev
        {
            get
            {
                return m_devRev;
            }

            set
            {
                if (!Object.ReferenceEquals(m_devRev, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_devRev = value;
            }
        }

        /// <remarks />
        public PropertyState<string> DevTag
        {
            get
            {
                return m_devTag;
            }

            set
            {
                if (!Object.ReferenceEquals(m_devTag, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_devTag = value;
            }
        }

        /// <remarks />
        public PropertyState<byte> DevPollAddr
        {
            get
            {
                return m_devPollAddr;
            }

            set
            {
                if (!Object.ReferenceEquals(m_devPollAddr, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_devPollAddr = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_devAddr != null)
            {
                children.Add(m_devAddr);
            }

            if (m_devMfg != null)
            {
                children.Add(m_devMfg);
            }

            if (m_devType != null)
            {
                children.Add(m_devType);
            }

            if (m_devRev != null)
            {
                children.Add(m_devRev);
            }

            if (m_devTag != null)
            {
                children.Add(m_devTag);
            }

            if (m_devPollAddr != null)
            {
                children.Add(m_devPollAddr);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Opc.Ua.Fdi7.BrowseNames.DevAddr:
                {
                    if (createOrReplace)
                    {
                        if (DevAddr == null)
                        {
                            if (replacement == null)
                            {
                                DevAddr = new PropertyState<byte[]>(this);
                            }
                            else
                            {
                                DevAddr = (PropertyState<byte[]>)replacement;
                            }
                        }
                    }

                    instance = DevAddr;
                    break;
                }

                case Opc.Ua.Fdi7.BrowseNames.DevMfg:
                {
                    if (createOrReplace)
                    {
                        if (DevMfg == null)
                        {
                            if (replacement == null)
                            {
                                DevMfg = new PropertyState<ushort>(this);
                            }
                            else
                            {
                                DevMfg = (PropertyState<ushort>)replacement;
                            }
                        }
                    }

                    instance = DevMfg;
                    break;
                }

                case Opc.Ua.Fdi7.BrowseNames.DevType:
                {
                    if (createOrReplace)
                    {
                        if (DevType == null)
                        {
                            if (replacement == null)
                            {
                                DevType = new PropertyState<ushort>(this);
                            }
                            else
                            {
                                DevType = (PropertyState<ushort>)replacement;
                            }
                        }
                    }

                    instance = DevType;
                    break;
                }

                case Opc.Ua.Fdi7.BrowseNames.DevRev:
                {
                    if (createOrReplace)
                    {
                        if (DevRev == null)
                        {
                            if (replacement == null)
                            {
                                DevRev = new PropertyState<ushort>(this);
                            }
                            else
                            {
                                DevRev = (PropertyState<ushort>)replacement;
                            }
                        }
                    }

                    instance = DevRev;
                    break;
                }

                case Opc.Ua.Fdi7.BrowseNames.DevTag:
                {
                    if (createOrReplace)
                    {
                        if (DevTag == null)
                        {
                            if (replacement == null)
                            {
                                DevTag = new PropertyState<string>(this);
                            }
                            else
                            {
                                DevTag = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = DevTag;
                    break;
                }

                case Opc.Ua.Fdi7.BrowseNames.DevPollAddr:
                {
                    if (createOrReplace)
                    {
                        if (DevPollAddr == null)
                        {
                            if (replacement == null)
                            {
                                DevPollAddr = new PropertyState<byte>(this);
                            }
                            else
                            {
                                DevPollAddr = (PropertyState<byte>)replacement;
                            }
                        }
                    }

                    instance = DevPollAddr;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private PropertyState<byte[]> m_devAddr;
        private PropertyState<ushort> m_devMfg;
        private PropertyState<ushort> m_devType;
        private PropertyState<ushort> m_devRev;
        private PropertyState<string> m_devTag;
        private PropertyState<byte> m_devPollAddr;
        #endregion
    }
    #endif
    #endregion

    #region ConnectionPoint_ISA100_WirelessState Class
    #if (!OPCUA_EXCLUDE_ConnectionPoint_ISA100_WirelessState)
    /// <summary>
    /// Stores an instance of the ConnectionPoint_ISA100_Wireless ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ConnectionPoint_ISA100_WirelessState : ConnectionPointState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ConnectionPoint_ISA100_WirelessState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.ConnectionPoint_ISA100_Wireless, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);

            if (DevMfg != null)
            {
                DevMfg.Initialize(context, DevMfg_InitializationString);
            }

            if (DevType != null)
            {
                DevType.Initialize(context, DevType_InitializationString);
            }

            if (DevRev != null)
            {
                DevRev.Initialize(context, DevRev_InitializationString);
            }

            if (DevTag != null)
            {
                DevTag.Initialize(context, DevTag_InitializationString);
            }

            if (DevPollAddr != null)
            {
                DevPollAddr.Initialize(context, DevPollAddr_InitializationString);
            }
        }

        #region Initialization String
        private const string DevMfg_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////xVgiQoCAAAAAQAGAAAARGV2TWZnAQHJBgAuAETJBgAAAAf/" +
           "////AQH/////AAAAAA==";

        private const string DevType_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////xVgiQoCAAAAAQAHAAAARGV2VHlwZQEBygYALgBEygYAAAAF" +
           "/////wEB/////wAAAAA=";

        private const string DevRev_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////xVgiQoCAAAAAQAGAAAARGV2UmV2AQHLBgAuAETLBgAAAAX/" +
           "////AQH/////AAAAAA==";

        private const string DevTag_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////xVgiQoCAAAAAQAGAAAARGV2VGFnAQHMBgAuAETMBgAAAAz/" +
           "////AQH/////AAAAAA==";

        private const string DevPollAddr_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////xVgiQoCAAAAAQALAAAARGV2UG9sbEFkZHIBAc0GAC4ARM0G" +
           "AAAAA/////8BAf////8AAAAA";

        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAnAAAAQ29ubmVjdGlvblBvaW50X0lTQTEw" +
           "MF9XaXJlbGVzc0luc3RhbmNlAQGfBgEBnwafBgAA/////wcAAAAkYIAKAQAAAAIADgAAAE5ldHdvcmtB" +
           "ZGRyZXNzAQG2BgMAAAAAKgAAAFRoZSBhZGRyZXNzIG9mIHRoZSBkZXZpY2Ugb24gdGhpcyBuZXR3b3Jr" +
           "LgAvAQLtA7YGAAD/////AAAAABVgiQoCAAAAAQAJAAAASVBBZGRyZXNzAQHIBgAuAETIBgAAAA//////" +
           "AQH/////AAAAABVgiQoCAAAAAQAGAAAARGV2TWZnAQHJBgAuAETJBgAAAAf/////AQH/////AAAAABVg" +
           "iQoCAAAAAQAHAAAARGV2VHlwZQEBygYALgBEygYAAAAF/////wEB/////wAAAAAVYIkKAgAAAAEABgAA" +
           "AERldlJldgEBywYALgBEywYAAAAF/////wEB/////wAAAAAVYIkKAgAAAAEABgAAAERldlRhZwEBzAYA" +
           "LgBEzAYAAAAM/////wEB/////wAAAAAVYIkKAgAAAAEACwAAAERldlBvbGxBZGRyAQHNBgAuAETNBgAA" +
           "AAP/////AQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public PropertyState<byte[]> IPAddress
        {
            get
            {
                return m_iPAddress;
            }

            set
            {
                if (!Object.ReferenceEquals(m_iPAddress, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_iPAddress = value;
            }
        }

        /// <remarks />
        public PropertyState<uint> DevMfg
        {
            get
            {
                return m_devMfg;
            }

            set
            {
                if (!Object.ReferenceEquals(m_devMfg, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_devMfg = value;
            }
        }

        /// <remarks />
        public PropertyState<ushort> DevType
        {
            get
            {
                return m_devType;
            }

            set
            {
                if (!Object.ReferenceEquals(m_devType, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_devType = value;
            }
        }

        /// <remarks />
        public PropertyState<ushort> DevRev
        {
            get
            {
                return m_devRev;
            }

            set
            {
                if (!Object.ReferenceEquals(m_devRev, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_devRev = value;
            }
        }

        /// <remarks />
        public PropertyState<string> DevTag
        {
            get
            {
                return m_devTag;
            }

            set
            {
                if (!Object.ReferenceEquals(m_devTag, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_devTag = value;
            }
        }

        /// <remarks />
        public PropertyState<byte> DevPollAddr
        {
            get
            {
                return m_devPollAddr;
            }

            set
            {
                if (!Object.ReferenceEquals(m_devPollAddr, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_devPollAddr = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_iPAddress != null)
            {
                children.Add(m_iPAddress);
            }

            if (m_devMfg != null)
            {
                children.Add(m_devMfg);
            }

            if (m_devType != null)
            {
                children.Add(m_devType);
            }

            if (m_devRev != null)
            {
                children.Add(m_devRev);
            }

            if (m_devTag != null)
            {
                children.Add(m_devTag);
            }

            if (m_devPollAddr != null)
            {
                children.Add(m_devPollAddr);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Opc.Ua.Fdi7.BrowseNames.IPAddress:
                {
                    if (createOrReplace)
                    {
                        if (IPAddress == null)
                        {
                            if (replacement == null)
                            {
                                IPAddress = new PropertyState<byte[]>(this);
                            }
                            else
                            {
                                IPAddress = (PropertyState<byte[]>)replacement;
                            }
                        }
                    }

                    instance = IPAddress;
                    break;
                }

                case Opc.Ua.Fdi7.BrowseNames.DevMfg:
                {
                    if (createOrReplace)
                    {
                        if (DevMfg == null)
                        {
                            if (replacement == null)
                            {
                                DevMfg = new PropertyState<uint>(this);
                            }
                            else
                            {
                                DevMfg = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = DevMfg;
                    break;
                }

                case Opc.Ua.Fdi7.BrowseNames.DevType:
                {
                    if (createOrReplace)
                    {
                        if (DevType == null)
                        {
                            if (replacement == null)
                            {
                                DevType = new PropertyState<ushort>(this);
                            }
                            else
                            {
                                DevType = (PropertyState<ushort>)replacement;
                            }
                        }
                    }

                    instance = DevType;
                    break;
                }

                case Opc.Ua.Fdi7.BrowseNames.DevRev:
                {
                    if (createOrReplace)
                    {
                        if (DevRev == null)
                        {
                            if (replacement == null)
                            {
                                DevRev = new PropertyState<ushort>(this);
                            }
                            else
                            {
                                DevRev = (PropertyState<ushort>)replacement;
                            }
                        }
                    }

                    instance = DevRev;
                    break;
                }

                case Opc.Ua.Fdi7.BrowseNames.DevTag:
                {
                    if (createOrReplace)
                    {
                        if (DevTag == null)
                        {
                            if (replacement == null)
                            {
                                DevTag = new PropertyState<string>(this);
                            }
                            else
                            {
                                DevTag = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = DevTag;
                    break;
                }

                case Opc.Ua.Fdi7.BrowseNames.DevPollAddr:
                {
                    if (createOrReplace)
                    {
                        if (DevPollAddr == null)
                        {
                            if (replacement == null)
                            {
                                DevPollAddr = new PropertyState<byte>(this);
                            }
                            else
                            {
                                DevPollAddr = (PropertyState<byte>)replacement;
                            }
                        }
                    }

                    instance = DevPollAddr;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private PropertyState<byte[]> m_iPAddress;
        private PropertyState<uint> m_devMfg;
        private PropertyState<ushort> m_devType;
        private PropertyState<ushort> m_devRev;
        private PropertyState<string> m_devTag;
        private PropertyState<byte> m_devPollAddr;
        #endregion
    }
    #endif
    #endregion

    #region GenericConnectionPointState Class
    #if (!OPCUA_EXCLUDE_GenericConnectionPointState)
    /// <summary>
    /// Stores an instance of the GenericConnectionPoint ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class GenericConnectionPointState : ConnectionPointState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public GenericConnectionPointState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.GenericConnectionPoint, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAeAAAAR2VuZXJpY0Nvbm5lY3Rpb25Qb2lu" +
           "dEluc3RhbmNlAQHOBgEBzgbOBgAA/////wMAAAAkYIAKAQAAAAIADgAAAE5ldHdvcmtBZGRyZXNzAQHl" +
           "BgMAAAAAKgAAAFRoZSBhZGRyZXNzIG9mIHRoZSBkZXZpY2Ugb24gdGhpcyBuZXR3b3JrLgAvAQLtA+UG" +
           "AAD/////AAAAABVgiQoCAAAAAQAHAAAAQWRkcmVzcwEB9wYALgBE9wYAAAAP/////wEB/////wAAAAAV" +
           "YIkKAgAAAAEAEgAAAFByb3RvY29sSWRlbnRpZmllcgEB+AYALgBE+AYAAAAM/////wEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public PropertyState<byte[]> Address
        {
            get
            {
                return m_address;
            }

            set
            {
                if (!Object.ReferenceEquals(m_address, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_address = value;
            }
        }

        /// <remarks />
        public PropertyState<string> ProtocolIdentifier
        {
            get
            {
                return m_protocolIdentifier;
            }

            set
            {
                if (!Object.ReferenceEquals(m_protocolIdentifier, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_protocolIdentifier = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_address != null)
            {
                children.Add(m_address);
            }

            if (m_protocolIdentifier != null)
            {
                children.Add(m_protocolIdentifier);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Opc.Ua.Fdi7.BrowseNames.Address:
                {
                    if (createOrReplace)
                    {
                        if (Address == null)
                        {
                            if (replacement == null)
                            {
                                Address = new PropertyState<byte[]>(this);
                            }
                            else
                            {
                                Address = (PropertyState<byte[]>)replacement;
                            }
                        }
                    }

                    instance = Address;
                    break;
                }

                case Opc.Ua.Fdi7.BrowseNames.ProtocolIdentifier:
                {
                    if (createOrReplace)
                    {
                        if (ProtocolIdentifier == null)
                        {
                            if (replacement == null)
                            {
                                ProtocolIdentifier = new PropertyState<string>(this);
                            }
                            else
                            {
                                ProtocolIdentifier = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = ProtocolIdentifier;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private PropertyState<byte[]> m_address;
        private PropertyState<string> m_protocolIdentifier;
        #endregion
    }
    #endif
    #endregion

    #region CommunicationServerTypeInitializeMethodState Class
    #if (!OPCUA_EXCLUDE_CommunicationServerTypeInitializeMethodState)
    /// <summary>
    /// Stores an instance of the CommunicationServerTypeInitializeMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class CommunicationServerTypeInitializeMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public CommunicationServerTypeInitializeMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new CommunicationServerTypeInitializeMethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQArAAAAQ29tbXVuaWNhdGlvblNlcnZlclR5" +
           "cGVJbml0aWFsaXplTWV0aG9kVHlwZQEBAQAALwEBAQABAAAAAQH/////AQAAABdgqQoCAAAAAAAPAAAA" +
           "T3V0cHV0QXJndW1lbnRzAQECAAAuAEQCAAAAlgEAAAABACoBARsAAAAMAAAAU2VydmljZUVycm9yABv/" +
           "////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public CommunicationServerTypeInitializeMethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            object serviceError = (object)_outputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    ref serviceError);
            }

            _outputArguments[0] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult CommunicationServerTypeInitializeMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        ref object serviceError);
    #endif
    #endregion

    #region CommunicationServerTypeResetMethodState Class
    #if (!OPCUA_EXCLUDE_CommunicationServerTypeResetMethodState)
    /// <summary>
    /// Stores an instance of the CommunicationServerTypeResetMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class CommunicationServerTypeResetMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public CommunicationServerTypeResetMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new CommunicationServerTypeResetMethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAmAAAAQ29tbXVuaWNhdGlvblNlcnZlclR5" +
           "cGVSZXNldE1ldGhvZFR5cGUBAQMAAC8BAQMAAwAAAAEB/////wEAAAAXYKkKAgAAAAAADwAAAE91dHB1" +
           "dEFyZ3VtZW50cwEBBAAALgBEBAAAAJYBAAAAAQAqAQEbAAAADAAAAFNlcnZpY2VFcnJvcgAb/////wAA" +
           "AAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public CommunicationServerTypeResetMethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            object serviceError = (object)_outputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    ref serviceError);
            }

            _outputArguments[0] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult CommunicationServerTypeResetMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        ref object serviceError);
    #endif
    #endregion

    #region AddComponentMethodState Class
    #if (!OPCUA_EXCLUDE_AddComponentMethodState)
    /// <summary>
    /// Stores an instance of the AddComponentMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class AddComponentMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public AddComponentMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new AddComponentMethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAWAAAAQWRkQ29tcG9uZW50TWV0aG9kVHlw" +
           "ZQEBBQAALwEBBQAFAAAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAQYAAC4A" +
           "RAYAAACWAwAAAAEAKgEBHQAAAA4AAABNb2R1bGVUeXBlTmFtZQAM/////wAAAAAAAQAqAQEbAAAADAAA" +
           "AEluc3RhbmNlTmFtZQAM/////wAAAAAAAQAqAQEcAAAADQAAAEluc3RhbmNlTGFiZWwADP////8AAAAA" +
           "AAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1bWVudHMBAQcA" +
           "AC4ARAcAAACWAgAAAAEAKgEBHQAAAA4AAABJbnN0YW5jZU5vZGVJZAAR/////wAAAAAAAQAqAQEbAAAA" +
           "DAAAAFNlcnZpY2VFcnJvcgAb/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public AddComponentMethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            string moduleTypeName = (string)_inputArguments[0];
            string instanceName = (string)_inputArguments[1];
            string instanceLabel = (string)_inputArguments[2];

            NodeId instanceNodeId = (NodeId)_outputArguments[0];
            object serviceError = (object)_outputArguments[1];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    moduleTypeName,
                    instanceName,
                    instanceLabel,
                    ref instanceNodeId,
                    ref serviceError);
            }

            _outputArguments[0] = instanceNodeId;
            _outputArguments[1] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult AddComponentMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        string moduleTypeName,
        string instanceName,
        string instanceLabel,
        ref NodeId instanceNodeId,
        ref object serviceError);
    #endif
    #endregion

    #region RemoveComponentMethodState Class
    #if (!OPCUA_EXCLUDE_RemoveComponentMethodState)
    /// <summary>
    /// Stores an instance of the RemoveComponentMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class RemoveComponentMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public RemoveComponentMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new RemoveComponentMethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAZAAAAUmVtb3ZlQ29tcG9uZW50TWV0aG9k" +
           "VHlwZQEBCAAALwEBCAAIAAAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAQkA" +
           "AC4ARAkAAACWAQAAAAEAKgEBGwAAAAwAAABNb2R1bGVOb2RlSWQAEf////8AAAAAAAEAKAEBAAAAAQAA" +
           "AAAAAAABAf////8AAAAAF2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1bWVudHMBAQoAAC4ARAoAAACWAQAA" +
           "AAEAKgEBGwAAAAwAAABTZXJ2aWNlRXJyb3IAG/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8A" +
           "AAAA";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public RemoveComponentMethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            NodeId moduleNodeId = (NodeId)_inputArguments[0];

            object serviceError = (object)_outputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    moduleNodeId,
                    ref serviceError);
            }

            _outputArguments[0] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult RemoveComponentMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        NodeId moduleNodeId,
        ref object serviceError);
    #endif
    #endregion

    #region CommunicationServerState Class
    #if (!OPCUA_EXCLUDE_CommunicationServerState)
    /// <summary>
    /// Stores an instance of the CommunicationServerType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class CommunicationServerState : DeviceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public CommunicationServerState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.CommunicationServerType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);

            if (ParameterSet != null)
            {
                ParameterSet.Initialize(context, ParameterSet_InitializationString);
            }
        }

        #region Initialization String
        private const string ParameterSet_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////yRggAoBAAAAAgAMAAAAUGFyYW1ldGVyU2V0AQEMAAMAAAAA" +
           "FwAAAEZsYXQgbGlzdCBvZiBQYXJhbWV0ZXJzAC8AOgwAAAD/////AAAAAA==";

        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAfAAAAQ29tbXVuaWNhdGlvblNlcnZlclR5" +
           "cGVJbnN0YW5jZQEBCwABAQsACwAAAP////8LAAAAJGCACgEAAAACAAwAAABQYXJhbWV0ZXJTZXQBAQwA" +
           "AwAAAAAXAAAARmxhdCBsaXN0IG9mIFBhcmFtZXRlcnMALwA6DAAAAP////8AAAAAJGCACgEAAAACAAkA" +
           "AABNZXRob2RTZXQBAQ4AAwAAAAAUAAAARmxhdCBsaXN0IG9mIE1ldGhvZHMALwA6DgAAAP////8EAAAA" +
           "BGGCCgQAAAABAAoAAABJbml0aWFsaXplAQFLAAAvAQFLAEsAAAABAf////8BAAAAF2CpCgIAAAAAAA8A" +
           "AABPdXRwdXRBcmd1bWVudHMBAUwAAC4AREwAAACWAQAAAAEAKgEBGwAAAAwAAABTZXJ2aWNlRXJyb3IA" +
           "G/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAABGGCCgQAAAABAAUAAABSZXNldAEBTQAA" +
           "LwEBTQBNAAAAAQH/////AQAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQFOAAAuAEROAAAA" +
           "lgEAAAABACoBARsAAAAMAAAAU2VydmljZUVycm9yABv/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/" +
           "////AAAAAARhggoEAAAAAQAMAAAAQWRkQ29tcG9uZW50AQFPAAAvAQFPAE8AAAABAf////8CAAAAF2Cp" +
           "CgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBUAAALgBEUAAAAJYDAAAAAQAqAQEdAAAADgAAAE1vZHVs" +
           "ZVR5cGVOYW1lAAz/////AAAAAAABACoBARsAAAAMAAAASW5zdGFuY2VOYW1lAAz/////AAAAAAABACoB" +
           "ARwAAAANAAAASW5zdGFuY2VMYWJlbAAM/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAX" +
           "YKkKAgAAAAAADwAAAE91dHB1dEFyZ3VtZW50cwEBUQAALgBEUQAAAJYCAAAAAQAqAQEdAAAADgAAAElu" +
           "c3RhbmNlTm9kZUlkABH/////AAAAAAABACoBARsAAAAMAAAAU2VydmljZUVycm9yABv/////AAAAAAAB" +
           "ACgBAQAAAAEAAAAAAAAAAQH/////AAAAAARhggoEAAAAAQAPAAAAUmVtb3ZlQ29tcG9uZW50AQFSAAAv" +
           "AQFSAFIAAAABAf////8CAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBUwAALgBEUwAAAJYB" +
           "AAAAAQAqAQEbAAAADAAAAE1vZHVsZU5vZGVJZAAR/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB////" +
           "/wAAAAAXYKkKAgAAAAAADwAAAE91dHB1dEFyZ3VtZW50cwEBVAAALgBEVAAAAJYBAAAAAQAqAQEbAAAA" +
           "DAAAAFNlcnZpY2VFcnJvcgAb/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAA1YIkKAgAA" +
           "AAIADAAAAE1hbnVmYWN0dXJlcgEBJAADAAAAADAAAABOYW1lIG9mIHRoZSBjb21wYW55IHRoYXQgbWFu" +
           "dWZhY3R1cmVkIHRoZSBkZXZpY2UALgBEJAAAAAAV/////wEB/////wAAAAA1YIkKAgAAAAIABQAAAE1v" +
           "ZGVsAQElAAMAAAAAGAAAAE1vZGVsIG5hbWUgb2YgdGhlIGRldmljZQAuAEQlAAAAABX/////AQH/////" +
           "AAAAADVgiQoCAAAAAgAQAAAASGFyZHdhcmVSZXZpc2lvbgEBKQADAAAAACwAAABSZXZpc2lvbiBsZXZl" +
           "bCBvZiB0aGUgaGFyZHdhcmUgb2YgdGhlIGRldmljZQAuAEQpAAAAAAz/////AQH/////AAAAADVgiQoC" +
           "AAAAAgAQAAAAU29mdHdhcmVSZXZpc2lvbgEBKAADAAAAADUAAABSZXZpc2lvbiBsZXZlbCBvZiB0aGUg" +
           "c29mdHdhcmUvZmlybXdhcmUgb2YgdGhlIGRldmljZQAuAEQoAAAAAAz/////AQH/////AAAAADVgiQoC" +
           "AAAAAgAOAAAARGV2aWNlUmV2aXNpb24BAScAAwAAAAAkAAAAT3ZlcmFsbCByZXZpc2lvbiBsZXZlbCBv" +
           "ZiB0aGUgZGV2aWNlAC4ARCcAAAAADP////8BAf////8AAAAANWCJCgIAAAACAAwAAABEZXZpY2VNYW51" +
           "YWwBASYAAwAAAABaAAAAQWRkcmVzcyAocGF0aG5hbWUgaW4gdGhlIGZpbGUgc3lzdGVtIG9yIGEgVVJM" +
           "IHwgV2ViIGFkZHJlc3MpIG9mIHVzZXIgbWFudWFsIGZvciB0aGUgZGV2aWNlAC4ARCYAAAAADP////8B" +
           "Af////8AAAAANWCJCgIAAAACAAwAAABTZXJpYWxOdW1iZXIBASIAAwAAAABNAAAASWRlbnRpZmllciB0" +
           "aGF0IHVuaXF1ZWx5IGlkZW50aWZpZXMsIHdpdGhpbiBhIG1hbnVmYWN0dXJlciwgYSBkZXZpY2UgaW5z" +
           "dGFuY2UALgBEIgAAAAAM/////wEB/////wAAAAA1YIkKAgAAAAIADwAAAFJldmlzaW9uQ291bnRlcgEB" +
           "IwADAAAAAGkAAABBbiBpbmNyZW1lbnRhbCBjb3VudGVyIGluZGljYXRpbmcgdGhlIG51bWJlciBvZiB0" +
           "aW1lcyB0aGUgc3RhdGljIGRhdGEgd2l0aGluIHRoZSBEZXZpY2UgaGFzIGJlZW4gbW9kaWZpZWQALgBE" +
           "IwAAAAAG/////wEB/////wAAAAAEYIAKAQAAAAEACgAAAFN1YkRldmljZXMBAVUAAC8APVUAAAD/////" +
           "AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public FolderState SubDevices
        {
            get
            {
                return m_subDevices;
            }

            set
            {
                if (!Object.ReferenceEquals(m_subDevices, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_subDevices = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_subDevices != null)
            {
                children.Add(m_subDevices);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Opc.Ua.Fdi7.BrowseNames.SubDevices:
                {
                    if (createOrReplace)
                    {
                        if (SubDevices == null)
                        {
                            if (replacement == null)
                            {
                                SubDevices = new FolderState(this);
                            }
                            else
                            {
                                SubDevices = (FolderState)replacement;
                            }
                        }
                    }

                    instance = SubDevices;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private FolderState m_subDevices;
        #endregion
    }
    #endif
    #endregion

    #region ScanMethodState Class
    #if (!OPCUA_EXCLUDE_ScanMethodState)
    /// <summary>
    /// Stores an instance of the ScanMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ScanMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ScanMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new ScanMethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAOAAAAU2Nhbk1ldGhvZFR5cGUBAVYAAC8B" +
           "AVYAVgAAAAEB/////wEAAAAXYKkKAgAAAAAADwAAAE91dHB1dEFyZ3VtZW50cwEBVwAALgBEVwAAAJYC" +
           "AAAAAQAqAQEhAAAAEgAAAFRvcG9sb2d5U2NhblJlc3VsdAAQ/////wAAAAAAAQAqAQEbAAAADAAAAFNl" +
           "cnZpY2VFcnJvcgAb/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public ScanMethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            XmlElement topologyScanResult = (XmlElement)_outputArguments[0];
            object serviceError = (object)_outputArguments[1];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    ref topologyScanResult,
                    ref serviceError);
            }

            _outputArguments[0] = topologyScanResult;
            _outputArguments[1] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult ScanMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        ref XmlElement topologyScanResult,
        ref object serviceError);
    #endif
    #endregion

    #region ResetScanMethodState Class
    #if (!OPCUA_EXCLUDE_ResetScanMethodState)
    /// <summary>
    /// Stores an instance of the ResetScanMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ResetScanMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ResetScanMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new ResetScanMethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQATAAAAUmVzZXRTY2FuTWV0aG9kVHlwZQEB" +
           "WAAALwEBWABYAAAAAQH/////AQAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQFZAAAuAERZ" +
           "AAAAlgEAAAABACoBARsAAAAMAAAAU2VydmljZUVycm9yABv/////AAAAAAABACgBAQAAAAEAAAAAAAAA" +
           "AQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public ResetScanMethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            object serviceError = (object)_outputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    ref serviceError);
            }

            _outputArguments[0] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult ResetScanMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        ref object serviceError);
    #endif
    #endregion

    #region SetAddressMethodFFH1MethodState Class
    #if (!OPCUA_EXCLUDE_SetAddressMethodFFH1MethodState)
    /// <summary>
    /// Stores an instance of the SetAddressMethodFFH1Type Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class SetAddressMethodFFH1MethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public SetAddressMethodFFH1MethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new SetAddressMethodFFH1MethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAYAAAAU2V0QWRkcmVzc01ldGhvZEZGSDFU" +
           "eXBlAQE1AQAvAQE1ATUBAAABAf////8CAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBNgEA" +
           "LgBENgEAAJYGAAAAAQAqAQEYAAAACQAAAE9QRVJBVElPTgAM/////wAAAAAAAQAqAQEVAAAABgAAAExp" +
           "bmtJZAAF/////wAAAAAAAQAqAQEZAAAACgAAAE9sZEFkZHJlc3MAA/////8AAAAAAAEAKgEBGQAAAAoA" +
           "AABOZXdBZGRyZXNzAAP/////AAAAAAABACoBARcAAAAIAAAATmV3UERUYWcADP////8AAAAAAAEAKgEB" +
           "GAAAAAkAAABTZXJ2aWNlSWQAB/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIA" +
           "AAAAAA8AAABPdXRwdXRBcmd1bWVudHMBATcBAC4ARDcBAACWAgAAAAEAKgEBHwAAABAAAABEZWxheUZv" +
           "ck5leHRDYWxsAAf/////AAAAAAABACoBARsAAAAMAAAAU2VydmljZUVycm9yABv/////AAAAAAABACgB" +
           "AQAAAAEAAAAAAAAAAQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public SetAddressMethodFFH1MethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            string oPERATION = (string)_inputArguments[0];
            ushort linkId = (ushort)_inputArguments[1];
            byte oldAddress = (byte)_inputArguments[2];
            byte newAddress = (byte)_inputArguments[3];
            string newPDTag = (string)_inputArguments[4];
            uint serviceId = (uint)_inputArguments[5];

            uint delayForNextCall = (uint)_outputArguments[0];
            object serviceError = (object)_outputArguments[1];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    oPERATION,
                    linkId,
                    oldAddress,
                    newAddress,
                    newPDTag,
                    serviceId,
                    ref delayForNextCall,
                    ref serviceError);
            }

            _outputArguments[0] = delayForNextCall;
            _outputArguments[1] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult SetAddressMethodFFH1MethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        string oPERATION,
        ushort linkId,
        byte oldAddress,
        byte newAddress,
        string newPDTag,
        uint serviceId,
        ref uint delayForNextCall,
        ref object serviceError);
    #endif
    #endregion

    #region SetAddressMethodFFHSEMethodState Class
    #if (!OPCUA_EXCLUDE_SetAddressMethodFFHSEMethodState)
    /// <summary>
    /// Stores an instance of the SetAddressMethodFFHSEType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class SetAddressMethodFFHSEMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public SetAddressMethodFFHSEMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new SetAddressMethodFFHSEMethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAZAAAAU2V0QWRkcmVzc01ldGhvZEZGSFNF" +
           "VHlwZQEBOAEALwEBOAE4AQAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBATkB" +
           "AC4ARDkBAACWAwAAAAEAKgEBGAAAAAkAAABPUEVSQVRJT04ADP////8AAAAAAAEAKgEBFwAAAAgAAABO" +
           "ZXdQRFRhZwAM/////wAAAAAAAQAqAQEYAAAACQAAAFNlcnZpY2VJZAAH/////wAAAAAAAQAoAQEAAAAB" +
           "AAAAAAAAAAEB/////wAAAAAXYKkKAgAAAAAADwAAAE91dHB1dEFyZ3VtZW50cwEBOgEALgBEOgEAAJYC" +
           "AAAAAQAqAQEfAAAAEAAAAERlbGF5Rm9yTmV4dENhbGwAB/////8AAAAAAAEAKgEBGwAAAAwAAABTZXJ2" +
           "aWNlRXJyb3IAG/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public SetAddressMethodFFHSEMethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            string oPERATION = (string)_inputArguments[0];
            string newPDTag = (string)_inputArguments[1];
            uint serviceId = (uint)_inputArguments[2];

            uint delayForNextCall = (uint)_outputArguments[0];
            object serviceError = (object)_outputArguments[1];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    oPERATION,
                    newPDTag,
                    serviceId,
                    ref delayForNextCall,
                    ref serviceError);
            }

            _outputArguments[0] = delayForNextCall;
            _outputArguments[1] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult SetAddressMethodFFHSEMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        string oPERATION,
        string newPDTag,
        uint serviceId,
        ref uint delayForNextCall,
        ref object serviceError);
    #endif
    #endregion

    #region SetAddressMethodPROFIBUSMethodState Class
    #if (!OPCUA_EXCLUDE_SetAddressMethodPROFIBUSMethodState)
    /// <summary>
    /// Stores an instance of the SetAddressMethodPROFIBUSType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class SetAddressMethodPROFIBUSMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public SetAddressMethodPROFIBUSMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new SetAddressMethodPROFIBUSMethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAcAAAAU2V0QWRkcmVzc01ldGhvZFBST0ZJ" +
           "QlVTVHlwZQEBOwEALwEBOwE7AQAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMB" +
           "ATwBAC4ARDwBAACWAgAAAAEAKgEBGQAAAAoAAABPbGRBZGRyZXNzAAP/////AAAAAAABACoBARkAAAAK" +
           "AAAATmV3QWRkcmVzcwAD/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAXYKkKAgAAAAAA" +
           "DwAAAE91dHB1dEFyZ3VtZW50cwEBPQEALgBEPQEAAJYBAAAAAQAqAQEbAAAADAAAAFNlcnZpY2VFcnJv" +
           "cgAb/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public SetAddressMethodPROFIBUSMethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            byte oldAddress = (byte)_inputArguments[0];
            byte newAddress = (byte)_inputArguments[1];

            object serviceError = (object)_outputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    oldAddress,
                    newAddress,
                    ref serviceError);
            }

            _outputArguments[0] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult SetAddressMethodPROFIBUSMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        byte oldAddress,
        byte newAddress,
        ref object serviceError);
    #endif
    #endregion

    #region SetAddressMethodPROFINETMethodState Class
    #if (!OPCUA_EXCLUDE_SetAddressMethodPROFINETMethodState)
    /// <summary>
    /// Stores an instance of the SetAddressMethodPROFINETType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class SetAddressMethodPROFINETMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public SetAddressMethodPROFINETMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new SetAddressMethodPROFINETMethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAcAAAAU2V0QWRkcmVzc01ldGhvZFBST0ZJ" +
           "TkVUVHlwZQEBPgEALwEBPgE+AQAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMB" +
           "AT8BAC4ARD8BAACWBQAAAAEAKgEBFgAAAAMAAABNQUMAAwEAAAABAAAABgAAAAABACoBARUAAAACAAAA" +
           "SVAAAwEAAAABAAAABAAAAAABACoBARYAAAAHAAAARE5TTkFNRQAM/////wAAAAAAAQAqAQEdAAAACgAA" +
           "AFN1Ym5ldE1hc2sAAwEAAAABAAAABAAAAAABACoBARoAAAAHAAAAR2F0ZXdheQADAQAAAAEAAAAEAAAA" +
           "AAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1bWVudHMBAUAB" +
           "AC4AREABAACWAQAAAAEAKgEBGwAAAAwAAABTZXJ2aWNlRXJyb3IAG/////8AAAAAAAEAKAEBAAAAAQAA" +
           "AAAAAAABAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public SetAddressMethodPROFINETMethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            byte[] mAC = (byte[])_inputArguments[0];
            byte[] iP = (byte[])_inputArguments[1];
            string dNSNAME = (string)_inputArguments[2];
            byte[] subnetMask = (byte[])_inputArguments[3];
            byte[] gateway = (byte[])_inputArguments[4];

            object serviceError = (object)_outputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    mAC,
                    iP,
                    dNSNAME,
                    subnetMask,
                    gateway,
                    ref serviceError);
            }

            _outputArguments[0] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult SetAddressMethodPROFINETMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        byte[] mAC,
        byte[] iP,
        string dNSNAME,
        byte[] subnetMask,
        byte[] gateway,
        ref object serviceError);
    #endif
    #endregion

    #region SetAddressMethodHARTMethodState Class
    #if (!OPCUA_EXCLUDE_SetAddressMethodHARTMethodState)
    /// <summary>
    /// Stores an instance of the SetAddressMethodHARTType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class SetAddressMethodHARTMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public SetAddressMethodHARTMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new SetAddressMethodHARTMethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAYAAAAU2V0QWRkcmVzc01ldGhvZEhBUlRU" +
           "eXBlAQFBAQAvAQFBAUEBAAABAf////8CAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBQgEA" +
           "LgBEQgEAAJYCAAAAAQAqAQEdAAAADgAAAE9sZFBvbGxBZGRyZXNzAAP/////AAAAAAABACoBAR0AAAAO" +
           "AAAATmV3UG9sbEFkZHJlc3MAA/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIA" +
           "AAAAAA8AAABPdXRwdXRBcmd1bWVudHMBAUMBAC4AREMBAACWAQAAAAEAKgEBGwAAAAwAAABTZXJ2aWNl" +
           "RXJyb3IAG/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public SetAddressMethodHARTMethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            byte oldPollAddress = (byte)_inputArguments[0];
            byte newPollAddress = (byte)_inputArguments[1];

            object serviceError = (object)_outputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    oldPollAddress,
                    newPollAddress,
                    ref serviceError);
            }

            _outputArguments[0] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult SetAddressMethodHARTMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        byte oldPollAddress,
        byte newPollAddress,
        ref object serviceError);
    #endif
    #endregion

    #region SetAddressMethodGENERICMethodState Class
    #if (!OPCUA_EXCLUDE_SetAddressMethodGENERICMethodState)
    /// <summary>
    /// Stores an instance of the SetAddressMethodGENERICType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class SetAddressMethodGENERICMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public SetAddressMethodGENERICMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new SetAddressMethodGENERICMethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAbAAAAU2V0QWRkcmVzc01ldGhvZEdFTkVS" +
           "SUNUeXBlAQH5BgAvAQH5BvkGAAABAf////8CAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEB" +
           "+gYALgBE+gYAAJYCAAAAAQAqAQEZAAAACgAAAE9sZEFkZHJlc3MAD/////8AAAAAAAEAKgEBGQAAAAoA" +
           "AABOZXdBZGRyZXNzAA//////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAP" +
           "AAAAT3V0cHV0QXJndW1lbnRzAQH7BgAuAET7BgAAlgEAAAABACoBARsAAAAMAAAAU2VydmljZUVycm9y" +
           "ABv/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public SetAddressMethodGENERICMethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            byte[] oldAddress = (byte[])_inputArguments[0];
            byte[] newAddress = (byte[])_inputArguments[1];

            object serviceError = (object)_outputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    oldAddress,
                    newAddress,
                    ref serviceError);
            }

            _outputArguments[0] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult SetAddressMethodGENERICMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        byte[] oldAddress,
        byte[] newAddress,
        ref object serviceError);
    #endif
    #endregion

    #region ServerCommunicationDeviceState Class
    #if (!OPCUA_EXCLUDE_ServerCommunicationDeviceState)
    /// <summary>
    /// Stores an instance of the ServerCommunicationDeviceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ServerCommunicationDeviceState : DeviceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ServerCommunicationDeviceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationDeviceType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);

            if (MethodSet != null)
            {
                MethodSet.Initialize(context, MethodSet_InitializationString);
            }
        }

        #region Initialization String
        private const string MethodSet_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////yRggAoBAAAAAgAJAAAATWV0aG9kU2V0AQFgAAMAAAAAFAAA" +
           "AEZsYXQgbGlzdCBvZiBNZXRob2RzAC8AOmAAAAD/////BAAAAARhggoEAAAAAQAEAAAAU2NhbgEBnQAA" +
           "LwEBnQCdAAAAAQH/////AQAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQGeAAAuAESeAAAA" +
           "lgIAAAABACoBASEAAAASAAAAVG9wb2xvZ3lTY2FuUmVzdWx0ABD/////AAAAAAABACoBARsAAAAMAAAA" +
           "U2VydmljZUVycm9yABv/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAAARhggoEAAAAAQAJ" +
           "AAAAUmVzZXRTY2FuAQGfAAAvAQGfAJ8AAAABAf////8BAAAAF2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1" +
           "bWVudHMBAaAAAC4ARKAAAACWAQAAAAEAKgEBGwAAAAwAAABTZXJ2aWNlRXJyb3IAG/////8AAAAAAAEA" +
           "KAEBAAAAAQAAAAAAAAABAf////8AAAAABGGCCgQAAAABAAwAAABBZGRDb21wb25lbnQBAaQAAC8BAaQA" +
           "pAAAAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElucHV0QXJndW1lbnRzAQGlAAAuAESlAAAAlgMAAAAB" +
           "ACoBAR0AAAAOAAAATW9kdWxlVHlwZU5hbWUADP////8AAAAAAAEAKgEBGwAAAAwAAABJbnN0YW5jZU5h" +
           "bWUADP////8AAAAAAAEAKgEBHAAAAA0AAABJbnN0YW5jZUxhYmVsAAz/////AAAAAAABACgBAQAAAAEA" +
           "AAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQGmAAAuAESmAAAAlgIA" +
           "AAABACoBAR0AAAAOAAAASW5zdGFuY2VOb2RlSWQAEf////8AAAAAAAEAKgEBGwAAAAwAAABTZXJ2aWNl" +
           "RXJyb3IAG/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAABGGCCgQAAAABAA8AAABSZW1v" +
           "dmVDb21wb25lbnQBAacAAC8BAacApwAAAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElucHV0QXJndW1l" +
           "bnRzAQGoAAAuAESoAAAAlgEAAAABACoBARsAAAAMAAAATW9kdWxlTm9kZUlkABH/////AAAAAAABACgB" +
           "AQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQGpAAAuAESp" +
           "AAAAlgEAAAABACoBARsAAAAMAAAAU2VydmljZUVycm9yABv/////AAAAAAABACgBAQAAAAEAAAAAAAAA" +
           "AQH/////AAAAAA==";

        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAlAAAAU2VydmVyQ29tbXVuaWNhdGlvbkRl" +
           "dmljZVR5cGVJbnN0YW5jZQEBXQABAV0AXQAAAP////8KAAAAJGCACgEAAAACAAkAAABNZXRob2RTZXQB" +
           "AWAAAwAAAAAUAAAARmxhdCBsaXN0IG9mIE1ldGhvZHMALwA6YAAAAP////8EAAAABGGCCgQAAAABAAQA" +
           "AABTY2FuAQGdAAAvAQGdAJ0AAAABAf////8BAAAAF2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1bWVudHMB" +
           "AZ4AAC4ARJ4AAACWAgAAAAEAKgEBIQAAABIAAABUb3BvbG9neVNjYW5SZXN1bHQAEP////8AAAAAAAEA" +
           "KgEBGwAAAAwAAABTZXJ2aWNlRXJyb3IAG/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAA" +
           "BGGCCgQAAAABAAkAAABSZXNldFNjYW4BAZ8AAC8BAZ8AnwAAAAEB/////wEAAAAXYKkKAgAAAAAADwAA" +
           "AE91dHB1dEFyZ3VtZW50cwEBoAAALgBEoAAAAJYBAAAAAQAqAQEbAAAADAAAAFNlcnZpY2VFcnJvcgAb" +
           "/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAEYYIKBAAAAAEADAAAAEFkZENvbXBvbmVu" +
           "dAEBpAAALwEBpACkAAAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAaUAAC4A" +
           "RKUAAACWAwAAAAEAKgEBHQAAAA4AAABNb2R1bGVUeXBlTmFtZQAM/////wAAAAAAAQAqAQEbAAAADAAA" +
           "AEluc3RhbmNlTmFtZQAM/////wAAAAAAAQAqAQEcAAAADQAAAEluc3RhbmNlTGFiZWwADP////8AAAAA" +
           "AAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1bWVudHMBAaYA" +
           "AC4ARKYAAACWAgAAAAEAKgEBHQAAAA4AAABJbnN0YW5jZU5vZGVJZAAR/////wAAAAAAAQAqAQEbAAAA" +
           "DAAAAFNlcnZpY2VFcnJvcgAb/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAEYYIKBAAA" +
           "AAEADwAAAFJlbW92ZUNvbXBvbmVudAEBpwAALwEBpwCnAAAAAQH/////AgAAABdgqQoCAAAAAAAOAAAA" +
           "SW5wdXRBcmd1bWVudHMBAagAAC4ARKgAAACWAQAAAAEAKgEBGwAAAAwAAABNb2R1bGVOb2RlSWQAEf//" +
           "//8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1bWVu" +
           "dHMBAakAAC4ARKkAAACWAQAAAAEAKgEBGwAAAAwAAABTZXJ2aWNlRXJyb3IAG/////8AAAAAAAEAKAEB" +
           "AAAAAQAAAAAAAAABAf////8AAAAANWCJCgIAAAACAAwAAABNYW51ZmFjdHVyZXIBAXYAAwAAAAAwAAAA" +
           "TmFtZSBvZiB0aGUgY29tcGFueSB0aGF0IG1hbnVmYWN0dXJlZCB0aGUgZGV2aWNlAC4ARHYAAAAAFf//" +
           "//8BAf////8AAAAANWCJCgIAAAACAAUAAABNb2RlbAEBdwADAAAAABgAAABNb2RlbCBuYW1lIG9mIHRo" +
           "ZSBkZXZpY2UALgBEdwAAAAAV/////wEB/////wAAAAA1YIkKAgAAAAIAEAAAAEhhcmR3YXJlUmV2aXNp" +
           "b24BAXsAAwAAAAAsAAAAUmV2aXNpb24gbGV2ZWwgb2YgdGhlIGhhcmR3YXJlIG9mIHRoZSBkZXZpY2UA" +
           "LgBEewAAAAAM/////wEB/////wAAAAA1YIkKAgAAAAIAEAAAAFNvZnR3YXJlUmV2aXNpb24BAXoAAwAA" +
           "AAA1AAAAUmV2aXNpb24gbGV2ZWwgb2YgdGhlIHNvZnR3YXJlL2Zpcm13YXJlIG9mIHRoZSBkZXZpY2UA" +
           "LgBEegAAAAAM/////wEB/////wAAAAA1YIkKAgAAAAIADgAAAERldmljZVJldmlzaW9uAQF5AAMAAAAA" +
           "JAAAAE92ZXJhbGwgcmV2aXNpb24gbGV2ZWwgb2YgdGhlIGRldmljZQAuAER5AAAAAAz/////AQH/////" +
           "AAAAADVgiQoCAAAAAgAMAAAARGV2aWNlTWFudWFsAQF4AAMAAAAAWgAAAEFkZHJlc3MgKHBhdGhuYW1l" +
           "IGluIHRoZSBmaWxlIHN5c3RlbSBvciBhIFVSTCB8IFdlYiBhZGRyZXNzKSBvZiB1c2VyIG1hbnVhbCBm" +
           "b3IgdGhlIGRldmljZQAuAER4AAAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAMAAAAU2VyaWFsTnVt" +
           "YmVyAQF0AAMAAAAATQAAAElkZW50aWZpZXIgdGhhdCB1bmlxdWVseSBpZGVudGlmaWVzLCB3aXRoaW4g" +
           "YSBtYW51ZmFjdHVyZXIsIGEgZGV2aWNlIGluc3RhbmNlAC4ARHQAAAAADP////8BAf////8AAAAANWCJ" +
           "CgIAAAACAA8AAABSZXZpc2lvbkNvdW50ZXIBAXUAAwAAAABpAAAAQW4gaW5jcmVtZW50YWwgY291bnRl" +
           "ciBpbmRpY2F0aW5nIHRoZSBudW1iZXIgb2YgdGltZXMgdGhlIHN0YXRpYyBkYXRhIHdpdGhpbiB0aGUg" +
           "RGV2aWNlIGhhcyBiZWVuIG1vZGlmaWVkAC4ARHUAAAAABv////8BAf////8AAAAAF2CJCgIAAAABABsA" +
           "AABMaXN0T2ZDb21tdW5pY2F0aW9uUHJvZmlsZXMBAZk6AC4ARJk6AAAADAEAAAABAAAAAAAAAAEB////" +
           "/wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public PropertyState<string[]> ListOfCommunicationProfiles
        {
            get
            {
                return m_listOfCommunicationProfiles;
            }

            set
            {
                if (!Object.ReferenceEquals(m_listOfCommunicationProfiles, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_listOfCommunicationProfiles = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_listOfCommunicationProfiles != null)
            {
                children.Add(m_listOfCommunicationProfiles);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Opc.Ua.Fdi7.BrowseNames.ListOfCommunicationProfiles:
                {
                    if (createOrReplace)
                    {
                        if (ListOfCommunicationProfiles == null)
                        {
                            if (replacement == null)
                            {
                                ListOfCommunicationProfiles = new PropertyState<string[]>(this);
                            }
                            else
                            {
                                ListOfCommunicationProfiles = (PropertyState<string[]>)replacement;
                            }
                        }
                    }

                    instance = ListOfCommunicationProfiles;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private PropertyState<string[]> m_listOfCommunicationProfiles;
        #endregion
    }
    #endif
    #endregion

    #region ServerCommunicationFFH1DeviceState Class
    #if (!OPCUA_EXCLUDE_ServerCommunicationFFH1DeviceState)
    /// <summary>
    /// Stores an instance of the ServerCommunicationFFH1DeviceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ServerCommunicationFFH1DeviceState : ServerCommunicationDeviceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ServerCommunicationFFH1DeviceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationFFH1DeviceType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);

            if (MethodSet != null)
            {
                MethodSet.Initialize(context, MethodSet_InitializationString);
            }
        }

        #region Initialization String
        private const string MethodSet_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////yRggAoBAAAAAgAJAAAATWV0aG9kU2V0AQFHAQMAAAAAFAAA" +
           "AEZsYXQgbGlzdCBvZiBNZXRob2RzAC8AOkcBAAD/////AQAAAARhggoEAAAAAQAKAAAAU2V0QWRkcmVz" +
           "cwEBjgEALwEBjgGOAQAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAY8BAC4A" +
           "RI8BAACWBgAAAAEAKgEBGAAAAAkAAABPUEVSQVRJT04ADP////8AAAAAAAEAKgEBFQAAAAYAAABMaW5r" +
           "SWQABf////8AAAAAAAEAKgEBGQAAAAoAAABPbGRBZGRyZXNzAAP/////AAAAAAABACoBARkAAAAKAAAA" +
           "TmV3QWRkcmVzcwAD/////wAAAAAAAQAqAQEXAAAACAAAAE5ld1BEVGFnAAz/////AAAAAAABACoBARgA" +
           "AAAJAAAAU2VydmljZUlkAAf/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAA" +
           "AAAPAAAAT3V0cHV0QXJndW1lbnRzAQGQAQAuAESQAQAAlgIAAAABACoBAR8AAAAQAAAARGVsYXlGb3JO" +
           "ZXh0Q2FsbAAH/////wAAAAAAAQAqAQEbAAAADAAAAFNlcnZpY2VFcnJvcgAb/////wAAAAAAAQAoAQEA" +
           "AAABAAAAAAAAAAEB/////wAAAAA=";

        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQApAAAAU2VydmVyQ29tbXVuaWNhdGlvbkZG" +
           "SDFEZXZpY2VUeXBlSW5zdGFuY2UBAUQBAQFEAUQBAAD/////CwAAACRggAoBAAAAAgAJAAAATWV0aG9k" +
           "U2V0AQFHAQMAAAAAFAAAAEZsYXQgbGlzdCBvZiBNZXRob2RzAC8AOkcBAAD/////AQAAAARhggoEAAAA" +
           "AQAKAAAAU2V0QWRkcmVzcwEBjgEALwEBjgGOAQAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRB" +
           "cmd1bWVudHMBAY8BAC4ARI8BAACWBgAAAAEAKgEBGAAAAAkAAABPUEVSQVRJT04ADP////8AAAAAAAEA" +
           "KgEBFQAAAAYAAABMaW5rSWQABf////8AAAAAAAEAKgEBGQAAAAoAAABPbGRBZGRyZXNzAAP/////AAAA" +
           "AAABACoBARkAAAAKAAAATmV3QWRkcmVzcwAD/////wAAAAAAAQAqAQEXAAAACAAAAE5ld1BEVGFnAAz/" +
           "////AAAAAAABACoBARgAAAAJAAAAU2VydmljZUlkAAf/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/" +
           "////AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQGQAQAuAESQAQAAlgIAAAABACoBAR8A" +
           "AAAQAAAARGVsYXlGb3JOZXh0Q2FsbAAH/////wAAAAAAAQAqAQEbAAAADAAAAFNlcnZpY2VFcnJvcgAb" +
           "/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAA1YIkKAgAAAAIADAAAAE1hbnVmYWN0dXJl" +
           "cgEBXQEDAAAAADAAAABOYW1lIG9mIHRoZSBjb21wYW55IHRoYXQgbWFudWZhY3R1cmVkIHRoZSBkZXZp" +
           "Y2UALgBEXQEAAAAV/////wEB/////wAAAAA1YIkKAgAAAAIABQAAAE1vZGVsAQFeAQMAAAAAGAAAAE1v" +
           "ZGVsIG5hbWUgb2YgdGhlIGRldmljZQAuAEReAQAAABX/////AQH/////AAAAADVgiQoCAAAAAgAQAAAA" +
           "SGFyZHdhcmVSZXZpc2lvbgEBYgEDAAAAACwAAABSZXZpc2lvbiBsZXZlbCBvZiB0aGUgaGFyZHdhcmUg" +
           "b2YgdGhlIGRldmljZQAuAERiAQAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAQAAAAU29mdHdhcmVS" +
           "ZXZpc2lvbgEBYQEDAAAAADUAAABSZXZpc2lvbiBsZXZlbCBvZiB0aGUgc29mdHdhcmUvZmlybXdhcmUg" +
           "b2YgdGhlIGRldmljZQAuAERhAQAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAOAAAARGV2aWNlUmV2" +
           "aXNpb24BAWABAwAAAAAkAAAAT3ZlcmFsbCByZXZpc2lvbiBsZXZlbCBvZiB0aGUgZGV2aWNlAC4ARGAB" +
           "AAAADP////8BAf////8AAAAANWCJCgIAAAACAAwAAABEZXZpY2VNYW51YWwBAV8BAwAAAABaAAAAQWRk" +
           "cmVzcyAocGF0aG5hbWUgaW4gdGhlIGZpbGUgc3lzdGVtIG9yIGEgVVJMIHwgV2ViIGFkZHJlc3MpIG9m" +
           "IHVzZXIgbWFudWFsIGZvciB0aGUgZGV2aWNlAC4ARF8BAAAADP////8BAf////8AAAAANWCJCgIAAAAC" +
           "AAwAAABTZXJpYWxOdW1iZXIBAVsBAwAAAABNAAAASWRlbnRpZmllciB0aGF0IHVuaXF1ZWx5IGlkZW50" +
           "aWZpZXMsIHdpdGhpbiBhIG1hbnVmYWN0dXJlciwgYSBkZXZpY2UgaW5zdGFuY2UALgBEWwEAAAAM////" +
           "/wEB/////wAAAAA1YIkKAgAAAAIADwAAAFJldmlzaW9uQ291bnRlcgEBXAEDAAAAAGkAAABBbiBpbmNy" +
           "ZW1lbnRhbCBjb3VudGVyIGluZGljYXRpbmcgdGhlIG51bWJlciBvZiB0aW1lcyB0aGUgc3RhdGljIGRh" +
           "dGEgd2l0aGluIHRoZSBEZXZpY2UgaGFzIGJlZW4gbW9kaWZpZWQALgBEXAEAAAAG/////wEB/////wAA" +
           "AAAXYIkKAgAAAAEAGwAAAExpc3RPZkNvbW11bmljYXRpb25Qcm9maWxlcwEBmjoALgBEmjoAAAAMAQAA" +
           "AAEAAAAAAAAAAQH/////AAAAAARggAoBAAAAAQAPAAAAU2VydmljZVByb3ZpZGVyAQGRAQAvAQHlA5EB" +
           "AAD/////CQAAACRggAoBAAAAAgAJAAAATWV0aG9kU2V0AQGUAQMAAAAAFAAAAEZsYXQgbGlzdCBvZiBN" +
           "ZXRob2RzAC8AOpQBAAD/////AwAAAARhggoEAAAAAQAKAAAARGlzY29ubmVjdAEBuAEALwEBLAG4AQAA" +
           "AQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAbkBAC4ARLkBAACWAQAAAAEAKgEB" +
           "JgAAABcAAABDb21tdW5pY2F0aW9uUmVsYXRpb25JZAAP/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB" +
           "/////wAAAAAXYKkKAgAAAAAADwAAAE91dHB1dEFyZ3VtZW50cwEBugEALgBEugEAAJYBAAAAAQAqAQEb" +
           "AAAADAAAAFNlcnZpY2VFcnJvcgAb/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAEYYIK" +
           "BAAAAAEABwAAAENvbm5lY3QBAbsBAC8BASgEuwEAAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElucHV0" +
           "QXJndW1lbnRzAQG8AQAuAES8AQAAlgYAAAABACoBASYAAAAXAAAAQ29tbXVuaWNhdGlvblJlbGF0aW9u" +
           "SWQAD/////8AAAAAAAEAKgEBFQAAAAYAAABMaW5rSWQABf////8AAAAAAAEAKgEBFgAAAAcAAABBZGRy" +
           "ZXNzAAP/////AAAAAAABACoBARwAAAANAAAAT3JkaW5hbE51bWJlcgAG/////wAAAAAAAQAqAQEcAAAA" +
           "DQAAAFNJRkNvbm5lY3Rpb24AAf////8AAAAAAAEAKgEBGAAAAAkAAABTZXJ2aWNlSWQAB/////8AAAAA" +
           "AAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1bWVudHMBAb0B" +
           "AC4ARL0BAACWAgAAAAEAKgEBHwAAABAAAABEZWxheUZvck5leHRDYWxsAAf/////AAAAAAABACoBARsA" +
           "AAAMAAAAU2VydmljZUVycm9yABv/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAAARhggoE" +
           "AAAAAQAIAAAAVHJhbnNmZXIBAb4BAC8BASsEvgEAAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElucHV0" +
           "QXJndW1lbnRzAQG/AQAuAES/AQAAlgcAAAABACoBASYAAAAXAAAAQ29tbXVuaWNhdGlvblJlbGF0aW9u" +
           "SWQAD/////8AAAAAAAEAKgEBGAAAAAkAAABPUEVSQVRJT04ADP////8AAAAAAAEAKgEBFwAAAAgAAABC" +
           "bG9ja1RhZwAM/////wAAAAAAAQAqAQEUAAAABQAAAElOREVYAAf/////AAAAAAABACoBARgAAAAJAAAA" +
           "U1VCX0lOREVYAAf/////AAAAAAABACoBARwAAAAJAAAAV3JpdGVEYXRhAAMBAAAAAQAAAAAAAAAAAQAq" +
           "AQEYAAAACQAAAFNlcnZpY2VJZAAH/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAXYKkK" +
           "AgAAAAAADwAAAE91dHB1dEFyZ3VtZW50cwEBwAEALgBEwAEAAJYDAAAAAQAqAQEbAAAACAAAAFJlYWRE" +
           "YXRhAAMBAAAAAQAAAAAAAAAAAQAqAQEfAAAAEAAAAERlbGF5Rm9yTmV4dENhbGwAB/////8AAAAAAAEA" +
           "KgEBGwAAAAwAAABTZXJ2aWNlRXJyb3IAG/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAA" +
           "NWCJCgIAAAACAAwAAABNYW51ZmFjdHVyZXIBAagBAwAAAAAwAAAATmFtZSBvZiB0aGUgY29tcGFueSB0" +
           "aGF0IG1hbnVmYWN0dXJlZCB0aGUgZGV2aWNlAC4ARKgBAAAAFf////8BAf////8AAAAANWCJCgIAAAAC" +
           "AAUAAABNb2RlbAEBqQEDAAAAABgAAABNb2RlbCBuYW1lIG9mIHRoZSBkZXZpY2UALgBEqQEAAAAV////" +
           "/wEB/////wAAAAA1YIkKAgAAAAIAEAAAAEhhcmR3YXJlUmV2aXNpb24BAa0BAwAAAAAsAAAAUmV2aXNp" +
           "b24gbGV2ZWwgb2YgdGhlIGhhcmR3YXJlIG9mIHRoZSBkZXZpY2UALgBErQEAAAAM/////wEB/////wAA" +
           "AAA1YIkKAgAAAAIAEAAAAFNvZnR3YXJlUmV2aXNpb24BAawBAwAAAAA1AAAAUmV2aXNpb24gbGV2ZWwg" +
           "b2YgdGhlIHNvZnR3YXJlL2Zpcm13YXJlIG9mIHRoZSBkZXZpY2UALgBErAEAAAAM/////wEB/////wAA" +
           "AAA1YIkKAgAAAAIADgAAAERldmljZVJldmlzaW9uAQGrAQMAAAAAJAAAAE92ZXJhbGwgcmV2aXNpb24g" +
           "bGV2ZWwgb2YgdGhlIGRldmljZQAuAESrAQAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAMAAAARGV2" +
           "aWNlTWFudWFsAQGqAQMAAAAAWgAAAEFkZHJlc3MgKHBhdGhuYW1lIGluIHRoZSBmaWxlIHN5c3RlbSBv" +
           "ciBhIFVSTCB8IFdlYiBhZGRyZXNzKSBvZiB1c2VyIG1hbnVhbCBmb3IgdGhlIGRldmljZQAuAESqAQAA" +
           "AAz/////AQH/////AAAAADVgiQoCAAAAAgAMAAAAU2VyaWFsTnVtYmVyAQGmAQMAAAAATQAAAElkZW50" +
           "aWZpZXIgdGhhdCB1bmlxdWVseSBpZGVudGlmaWVzLCB3aXRoaW4gYSBtYW51ZmFjdHVyZXIsIGEgZGV2" +
           "aWNlIGluc3RhbmNlAC4ARKYBAAAADP////8BAf////8AAAAANWCJCgIAAAACAA8AAABSZXZpc2lvbkNv" +
           "dW50ZXIBAacBAwAAAABpAAAAQW4gaW5jcmVtZW50YWwgY291bnRlciBpbmRpY2F0aW5nIHRoZSBudW1i" +
           "ZXIgb2YgdGltZXMgdGhlIHN0YXRpYyBkYXRhIHdpdGhpbiB0aGUgRGV2aWNlIGhhcyBiZWVuIG1vZGlm" +
           "aWVkAC4ARKcBAAAABv////8BAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public ServerCommunicationFFH1ServiceState ServiceProvider
        {
            get
            {
                return m_serviceProvider;
            }

            set
            {
                if (!Object.ReferenceEquals(m_serviceProvider, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_serviceProvider = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_serviceProvider != null)
            {
                children.Add(m_serviceProvider);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Opc.Ua.Fdi7.BrowseNames.ServiceProvider:
                {
                    if (createOrReplace)
                    {
                        if (ServiceProvider == null)
                        {
                            if (replacement == null)
                            {
                                ServiceProvider = new ServerCommunicationFFH1ServiceState(this);
                            }
                            else
                            {
                                ServiceProvider = (ServerCommunicationFFH1ServiceState)replacement;
                            }
                        }
                    }

                    instance = ServiceProvider;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private ServerCommunicationFFH1ServiceState m_serviceProvider;
        #endregion
    }
    #endif
    #endregion

    #region ServerCommunicationFFHSEDeviceState Class
    #if (!OPCUA_EXCLUDE_ServerCommunicationFFHSEDeviceState)
    /// <summary>
    /// Stores an instance of the ServerCommunicationFFHSEDeviceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ServerCommunicationFFHSEDeviceState : ServerCommunicationDeviceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ServerCommunicationFFHSEDeviceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationFFHSEDeviceType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);

            if (MethodSet != null)
            {
                MethodSet.Initialize(context, MethodSet_InitializationString);
            }
        }

        #region Initialization String
        private const string MethodSet_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////yRggAoBAAAAAgAJAAAATWV0aG9kU2V0AQHHAQMAAAAAFAAA" +
           "AEZsYXQgbGlzdCBvZiBNZXRob2RzAC8AOscBAAD/////AQAAAARhggoEAAAAAQAKAAAAU2V0QWRkcmVz" +
           "cwEBDgIALwEBDgIOAgAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAQ8CAC4A" +
           "RA8CAACWAwAAAAEAKgEBGAAAAAkAAABPUEVSQVRJT04ADP////8AAAAAAAEAKgEBFwAAAAgAAABOZXdQ" +
           "RFRhZwAM/////wAAAAAAAQAqAQEYAAAACQAAAFNlcnZpY2VJZAAH/////wAAAAAAAQAoAQEAAAABAAAA" +
           "AAAAAAEB/////wAAAAAXYKkKAgAAAAAADwAAAE91dHB1dEFyZ3VtZW50cwEBEAIALgBEEAIAAJYCAAAA" +
           "AQAqAQEfAAAAEAAAAERlbGF5Rm9yTmV4dENhbGwAB/////8AAAAAAAEAKgEBGwAAAAwAAABTZXJ2aWNl" +
           "RXJyb3IAG/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAA";

        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAqAAAAU2VydmVyQ29tbXVuaWNhdGlvbkZG" +
           "SFNFRGV2aWNlVHlwZUluc3RhbmNlAQHEAQEBxAHEAQAA/////wsAAAAkYIAKAQAAAAIACQAAAE1ldGhv" +
           "ZFNldAEBxwEDAAAAABQAAABGbGF0IGxpc3Qgb2YgTWV0aG9kcwAvADrHAQAA/////wEAAAAEYYIKBAAA" +
           "AAEACgAAAFNldEFkZHJlc3MBAQ4CAC8BAQ4CDgIAAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElucHV0" +
           "QXJndW1lbnRzAQEPAgAuAEQPAgAAlgMAAAABACoBARgAAAAJAAAAT1BFUkFUSU9OAAz/////AAAAAAAB" +
           "ACoBARcAAAAIAAAATmV3UERUYWcADP////8AAAAAAAEAKgEBGAAAAAkAAABTZXJ2aWNlSWQAB/////8A" +
           "AAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1bWVudHMB" +
           "ARACAC4ARBACAACWAgAAAAEAKgEBHwAAABAAAABEZWxheUZvck5leHRDYWxsAAf/////AAAAAAABACoB" +
           "ARsAAAAMAAAAU2VydmljZUVycm9yABv/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAADVg" +
           "iQoCAAAAAgAMAAAATWFudWZhY3R1cmVyAQHdAQMAAAAAMAAAAE5hbWUgb2YgdGhlIGNvbXBhbnkgdGhh" +
           "dCBtYW51ZmFjdHVyZWQgdGhlIGRldmljZQAuAETdAQAAABX/////AQH/////AAAAADVgiQoCAAAAAgAF" +
           "AAAATW9kZWwBAd4BAwAAAAAYAAAATW9kZWwgbmFtZSBvZiB0aGUgZGV2aWNlAC4ARN4BAAAAFf////8B" +
           "Af////8AAAAANWCJCgIAAAACABAAAABIYXJkd2FyZVJldmlzaW9uAQHiAQMAAAAALAAAAFJldmlzaW9u" +
           "IGxldmVsIG9mIHRoZSBoYXJkd2FyZSBvZiB0aGUgZGV2aWNlAC4AROIBAAAADP////8BAf////8AAAAA" +
           "NWCJCgIAAAACABAAAABTb2Z0d2FyZVJldmlzaW9uAQHhAQMAAAAANQAAAFJldmlzaW9uIGxldmVsIG9m" +
           "IHRoZSBzb2Z0d2FyZS9maXJtd2FyZSBvZiB0aGUgZGV2aWNlAC4AROEBAAAADP////8BAf////8AAAAA" +
           "NWCJCgIAAAACAA4AAABEZXZpY2VSZXZpc2lvbgEB4AEDAAAAACQAAABPdmVyYWxsIHJldmlzaW9uIGxl" +
           "dmVsIG9mIHRoZSBkZXZpY2UALgBE4AEAAAAM/////wEB/////wAAAAA1YIkKAgAAAAIADAAAAERldmlj" +
           "ZU1hbnVhbAEB3wEDAAAAAFoAAABBZGRyZXNzIChwYXRobmFtZSBpbiB0aGUgZmlsZSBzeXN0ZW0gb3Ig" +
           "YSBVUkwgfCBXZWIgYWRkcmVzcykgb2YgdXNlciBtYW51YWwgZm9yIHRoZSBkZXZpY2UALgBE3wEAAAAM" +
           "/////wEB/////wAAAAA1YIkKAgAAAAIADAAAAFNlcmlhbE51bWJlcgEB2wEDAAAAAE0AAABJZGVudGlm" +
           "aWVyIHRoYXQgdW5pcXVlbHkgaWRlbnRpZmllcywgd2l0aGluIGEgbWFudWZhY3R1cmVyLCBhIGRldmlj" +
           "ZSBpbnN0YW5jZQAuAETbAQAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAPAAAAUmV2aXNpb25Db3Vu" +
           "dGVyAQHcAQMAAAAAaQAAAEFuIGluY3JlbWVudGFsIGNvdW50ZXIgaW5kaWNhdGluZyB0aGUgbnVtYmVy" +
           "IG9mIHRpbWVzIHRoZSBzdGF0aWMgZGF0YSB3aXRoaW4gdGhlIERldmljZSBoYXMgYmVlbiBtb2RpZmll" +
           "ZAAuAETcAQAAAAb/////AQH/////AAAAABdgiQoCAAAAAQAbAAAATGlzdE9mQ29tbXVuaWNhdGlvblBy" +
           "b2ZpbGVzAQGbOgAuAESbOgAAAAwBAAAAAQAAAAAAAAABAf////8AAAAABGCACgEAAAABAA8AAABTZXJ2" +
           "aWNlUHJvdmlkZXIBARECAC8BATEEEQIAAP////8JAAAAJGCACgEAAAACAAkAAABNZXRob2RTZXQBARQC" +
           "AwAAAAAUAAAARmxhdCBsaXN0IG9mIE1ldGhvZHMALwA6FAIAAP////8DAAAABGGCCgQAAAABAAoAAABE" +
           "aXNjb25uZWN0AQE4AgAvAQEsATgCAAABAf////8CAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50" +
           "cwEBOQIALgBEOQIAAJYBAAAAAQAqAQEmAAAAFwAAAENvbW11bmljYXRpb25SZWxhdGlvbklkAA//////" +
           "AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRz" +
           "AQE6AgAuAEQ6AgAAlgEAAAABACoBARsAAAAMAAAAU2VydmljZUVycm9yABv/////AAAAAAABACgBAQAA" +
           "AAEAAAAAAAAAAQH/////AAAAAARhggoEAAAAAQAHAAAAQ29ubmVjdAEBOwIALwEBdAQ7AgAAAQH/////" +
           "AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBATwCAC4ARDwCAACWBAAAAAEAKgEBJgAAABcA" +
           "AABDb21tdW5pY2F0aW9uUmVsYXRpb25JZAAP/////wAAAAAAAQAqAQEWAAAABwAAAEFkZHJlc3MAD///" +
           "//8AAAAAAAEAKgEBHAAAAA0AAABPcmRpbmFsTnVtYmVyAAb/////AAAAAAABACoBARgAAAAJAAAAU2Vy" +
           "dmljZUlkAAf/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0" +
           "cHV0QXJndW1lbnRzAQE9AgAuAEQ9AgAAlgIAAAABACoBAR8AAAAQAAAARGVsYXlGb3JOZXh0Q2FsbAAH" +
           "/////wAAAAAAAQAqAQEbAAAADAAAAFNlcnZpY2VFcnJvcgAb/////wAAAAAAAQAoAQEAAAABAAAAAAAA" +
           "AAEB/////wAAAAAEYYIKBAAAAAEACAAAAFRyYW5zZmVyAQE+AgAvAQF3BD4CAAABAf////8CAAAAF2Cp" +
           "CgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBPwIALgBEPwIAAJYHAAAAAQAqAQEmAAAAFwAAAENvbW11" +
           "bmljYXRpb25SZWxhdGlvbklkAA//////AAAAAAABACoBARgAAAAJAAAAT1BFUkFUSU9OAAz/////AAAA" +
           "AAABACoBARcAAAAIAAAAQmxvY2tUYWcADP////8AAAAAAAEAKgEBFAAAAAUAAABJTkRFWAAH/////wAA" +
           "AAAAAQAqAQEYAAAACQAAAFNVQl9JTkRFWAAH/////wAAAAAAAQAqAQEcAAAACQAAAFdyaXRlRGF0YQAD" +
           "AQAAAAEAAAAAAAAAAAEAKgEBGAAAAAkAAABTZXJ2aWNlSWQAB/////8AAAAAAAEAKAEBAAAAAQAAAAAA" +
           "AAABAf////8AAAAAF2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1bWVudHMBAUACAC4AREACAACWAwAAAAEA" +
           "KgEBGwAAAAgAAABSZWFkRGF0YQADAQAAAAEAAAAAAAAAAAEAKgEBHwAAABAAAABEZWxheUZvck5leHRD" +
           "YWxsAAf/////AAAAAAABACoBARsAAAAMAAAAU2VydmljZUVycm9yABv/////AAAAAAABACgBAQAAAAEA" +
           "AAAAAAAAAQH/////AAAAADVgiQoCAAAAAgAMAAAATWFudWZhY3R1cmVyAQEoAgMAAAAAMAAAAE5hbWUg" +
           "b2YgdGhlIGNvbXBhbnkgdGhhdCBtYW51ZmFjdHVyZWQgdGhlIGRldmljZQAuAEQoAgAAABX/////AQH/" +
           "////AAAAADVgiQoCAAAAAgAFAAAATW9kZWwBASkCAwAAAAAYAAAATW9kZWwgbmFtZSBvZiB0aGUgZGV2" +
           "aWNlAC4ARCkCAAAAFf////8BAf////8AAAAANWCJCgIAAAACABAAAABIYXJkd2FyZVJldmlzaW9uAQEt" +
           "AgMAAAAALAAAAFJldmlzaW9uIGxldmVsIG9mIHRoZSBoYXJkd2FyZSBvZiB0aGUgZGV2aWNlAC4ARC0C" +
           "AAAADP////8BAf////8AAAAANWCJCgIAAAACABAAAABTb2Z0d2FyZVJldmlzaW9uAQEsAgMAAAAANQAA" +
           "AFJldmlzaW9uIGxldmVsIG9mIHRoZSBzb2Z0d2FyZS9maXJtd2FyZSBvZiB0aGUgZGV2aWNlAC4ARCwC" +
           "AAAADP////8BAf////8AAAAANWCJCgIAAAACAA4AAABEZXZpY2VSZXZpc2lvbgEBKwIDAAAAACQAAABP" +
           "dmVyYWxsIHJldmlzaW9uIGxldmVsIG9mIHRoZSBkZXZpY2UALgBEKwIAAAAM/////wEB/////wAAAAA1" +
           "YIkKAgAAAAIADAAAAERldmljZU1hbnVhbAEBKgIDAAAAAFoAAABBZGRyZXNzIChwYXRobmFtZSBpbiB0" +
           "aGUgZmlsZSBzeXN0ZW0gb3IgYSBVUkwgfCBXZWIgYWRkcmVzcykgb2YgdXNlciBtYW51YWwgZm9yIHRo" +
           "ZSBkZXZpY2UALgBEKgIAAAAM/////wEB/////wAAAAA1YIkKAgAAAAIADAAAAFNlcmlhbE51bWJlcgEB" +
           "JgIDAAAAAE0AAABJZGVudGlmaWVyIHRoYXQgdW5pcXVlbHkgaWRlbnRpZmllcywgd2l0aGluIGEgbWFu" +
           "dWZhY3R1cmVyLCBhIGRldmljZSBpbnN0YW5jZQAuAEQmAgAAAAz/////AQH/////AAAAADVgiQoCAAAA" +
           "AgAPAAAAUmV2aXNpb25Db3VudGVyAQEnAgMAAAAAaQAAAEFuIGluY3JlbWVudGFsIGNvdW50ZXIgaW5k" +
           "aWNhdGluZyB0aGUgbnVtYmVyIG9mIHRpbWVzIHRoZSBzdGF0aWMgZGF0YSB3aXRoaW4gdGhlIERldmlj" +
           "ZSBoYXMgYmVlbiBtb2RpZmllZAAuAEQnAgAAAAb/////AQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public ServerCommunicationFFHSEServiceState ServiceProvider
        {
            get
            {
                return m_serviceProvider;
            }

            set
            {
                if (!Object.ReferenceEquals(m_serviceProvider, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_serviceProvider = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_serviceProvider != null)
            {
                children.Add(m_serviceProvider);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Opc.Ua.Fdi7.BrowseNames.ServiceProvider:
                {
                    if (createOrReplace)
                    {
                        if (ServiceProvider == null)
                        {
                            if (replacement == null)
                            {
                                ServiceProvider = new ServerCommunicationFFHSEServiceState(this);
                            }
                            else
                            {
                                ServiceProvider = (ServerCommunicationFFHSEServiceState)replacement;
                            }
                        }
                    }

                    instance = ServiceProvider;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private ServerCommunicationFFHSEServiceState m_serviceProvider;
        #endregion
    }
    #endif
    #endregion

    #region ServerCommunicationPROFIBUSDeviceState Class
    #if (!OPCUA_EXCLUDE_ServerCommunicationPROFIBUSDeviceState)
    /// <summary>
    /// Stores an instance of the ServerCommunicationPROFIBUSDeviceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ServerCommunicationPROFIBUSDeviceState : ServerCommunicationDeviceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ServerCommunicationPROFIBUSDeviceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationPROFIBUSDeviceType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);

            if (MethodSet != null)
            {
                MethodSet.Initialize(context, MethodSet_InitializationString);
            }
        }

        #region Initialization String
        private const string MethodSet_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////yRggAoBAAAAAgAJAAAATWV0aG9kU2V0AQFHAgMAAAAAFAAA" +
           "AEZsYXQgbGlzdCBvZiBNZXRob2RzAC8AOkcCAAD/////AQAAAARhggoEAAAAAQAKAAAAU2V0QWRkcmVz" +
           "cwEBjgIALwEBjgKOAgAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAY8CAC4A" +
           "RI8CAACWAgAAAAEAKgEBGQAAAAoAAABPbGRBZGRyZXNzAAP/////AAAAAAABACoBARkAAAAKAAAATmV3" +
           "QWRkcmVzcwAD/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAXYKkKAgAAAAAADwAAAE91" +
           "dHB1dEFyZ3VtZW50cwEBkAIALgBEkAIAAJYBAAAAAQAqAQEbAAAADAAAAFNlcnZpY2VFcnJvcgAb////" +
           "/wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAA=";

        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAtAAAAU2VydmVyQ29tbXVuaWNhdGlvblBS" +
           "T0ZJQlVTRGV2aWNlVHlwZUluc3RhbmNlAQFEAgEBRAJEAgAA/////wsAAAAkYIAKAQAAAAIACQAAAE1l" +
           "dGhvZFNldAEBRwIDAAAAABQAAABGbGF0IGxpc3Qgb2YgTWV0aG9kcwAvADpHAgAA/////wEAAAAEYYIK" +
           "BAAAAAEACgAAAFNldEFkZHJlc3MBAY4CAC8BAY4CjgIAAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElu" +
           "cHV0QXJndW1lbnRzAQGPAgAuAESPAgAAlgIAAAABACoBARkAAAAKAAAAT2xkQWRkcmVzcwAD/////wAA" +
           "AAAAAQAqAQEZAAAACgAAAE5ld0FkZHJlc3MAA/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8A" +
           "AAAAF2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1bWVudHMBAZACAC4ARJACAACWAQAAAAEAKgEBGwAAAAwA" +
           "AABTZXJ2aWNlRXJyb3IAG/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAANWCJCgIAAAAC" +
           "AAwAAABNYW51ZmFjdHVyZXIBAV0CAwAAAAAwAAAATmFtZSBvZiB0aGUgY29tcGFueSB0aGF0IG1hbnVm" +
           "YWN0dXJlZCB0aGUgZGV2aWNlAC4ARF0CAAAAFf////8BAf////8AAAAANWCJCgIAAAACAAUAAABNb2Rl" +
           "bAEBXgIDAAAAABgAAABNb2RlbCBuYW1lIG9mIHRoZSBkZXZpY2UALgBEXgIAAAAV/////wEB/////wAA" +
           "AAA1YIkKAgAAAAIAEAAAAEhhcmR3YXJlUmV2aXNpb24BAWICAwAAAAAsAAAAUmV2aXNpb24gbGV2ZWwg" +
           "b2YgdGhlIGhhcmR3YXJlIG9mIHRoZSBkZXZpY2UALgBEYgIAAAAM/////wEB/////wAAAAA1YIkKAgAA" +
           "AAIAEAAAAFNvZnR3YXJlUmV2aXNpb24BAWECAwAAAAA1AAAAUmV2aXNpb24gbGV2ZWwgb2YgdGhlIHNv" +
           "ZnR3YXJlL2Zpcm13YXJlIG9mIHRoZSBkZXZpY2UALgBEYQIAAAAM/////wEB/////wAAAAA1YIkKAgAA" +
           "AAIADgAAAERldmljZVJldmlzaW9uAQFgAgMAAAAAJAAAAE92ZXJhbGwgcmV2aXNpb24gbGV2ZWwgb2Yg" +
           "dGhlIGRldmljZQAuAERgAgAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAMAAAARGV2aWNlTWFudWFs" +
           "AQFfAgMAAAAAWgAAAEFkZHJlc3MgKHBhdGhuYW1lIGluIHRoZSBmaWxlIHN5c3RlbSBvciBhIFVSTCB8" +
           "IFdlYiBhZGRyZXNzKSBvZiB1c2VyIG1hbnVhbCBmb3IgdGhlIGRldmljZQAuAERfAgAAAAz/////AQH/" +
           "////AAAAADVgiQoCAAAAAgAMAAAAU2VyaWFsTnVtYmVyAQFbAgMAAAAATQAAAElkZW50aWZpZXIgdGhh" +
           "dCB1bmlxdWVseSBpZGVudGlmaWVzLCB3aXRoaW4gYSBtYW51ZmFjdHVyZXIsIGEgZGV2aWNlIGluc3Rh" +
           "bmNlAC4ARFsCAAAADP////8BAf////8AAAAANWCJCgIAAAACAA8AAABSZXZpc2lvbkNvdW50ZXIBAVwC" +
           "AwAAAABpAAAAQW4gaW5jcmVtZW50YWwgY291bnRlciBpbmRpY2F0aW5nIHRoZSBudW1iZXIgb2YgdGlt" +
           "ZXMgdGhlIHN0YXRpYyBkYXRhIHdpdGhpbiB0aGUgRGV2aWNlIGhhcyBiZWVuIG1vZGlmaWVkAC4ARFwC" +
           "AAAABv////8BAf////8AAAAAF2CJCgIAAAABABsAAABMaXN0T2ZDb21tdW5pY2F0aW9uUHJvZmlsZXMB" +
           "AZw6AC4ARJw6AAAADAEAAAABAAAAAAAAAAEB/////wAAAAAEYIAKAQAAAAEADwAAAFNlcnZpY2VQcm92" +
           "aWRlcgEBkQIALwEBfQSRAgAA/////wkAAAAkYIAKAQAAAAIACQAAAE1ldGhvZFNldAEBlAIDAAAAABQA" +
           "AABGbGF0IGxpc3Qgb2YgTWV0aG9kcwAvADqUAgAA/////wMAAAAEYYIKBAAAAAEACgAAAERpc2Nvbm5l" +
           "Y3QBAbgCAC8BASwBuAIAAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElucHV0QXJndW1lbnRzAQG5AgAu" +
           "AES5AgAAlgEAAAABACoBASYAAAAXAAAAQ29tbXVuaWNhdGlvblJlbGF0aW9uSWQAD/////8AAAAAAAEA" +
           "KAEBAAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1bWVudHMBAboCAC4A" +
           "RLoCAACWAQAAAAEAKgEBGwAAAAwAAABTZXJ2aWNlRXJyb3IAG/////8AAAAAAAEAKAEBAAAAAQAAAAAA" +
           "AAABAf////8AAAAABGGCCgQAAAABAAcAAABDb25uZWN0AQG7AgAvAQHABLsCAAABAf////8CAAAAF2Cp" +
           "CgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBvAIALgBEvAIAAJYDAAAAAQAqAQEmAAAAFwAAAENvbW11" +
           "bmljYXRpb25SZWxhdGlvbklkAA//////AAAAAAABACoBARYAAAAHAAAAQWRkcmVzcwAD/////wAAAAAA" +
           "AQAqAQEdAAAADgAAAE1hbnVmYWN0dXJlcklkAAX/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////" +
           "AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQG9AgAuAES9AgAAlgEAAAABACoBARsAAAAM" +
           "AAAAU2VydmljZUVycm9yABv/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAAARhggoEAAAA" +
           "AQAIAAAAVHJhbnNmZXIBAb4CAC8BAcMEvgIAAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElucHV0QXJn" +
           "dW1lbnRzAQG/AgAuAES/AgAAlgUAAAABACoBASYAAAAXAAAAQ29tbXVuaWNhdGlvblJlbGF0aW9uSWQA" +
           "D/////8AAAAAAAEAKgEBGAAAAAkAAABPUEVSQVRJT04ADP////8AAAAAAAEAKgEBEwAAAAQAAABTTE9U" +
           "AAP/////AAAAAAABACoBARQAAAAFAAAASU5ERVgAA/////8AAAAAAAEAKgEBFgAAAAcAAABSRVFVRVNU" +
           "AA//////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJn" +
           "dW1lbnRzAQHAAgAuAETAAgAAlgMAAAABACoBARQAAAAFAAAAUkVQTFkAD/////8AAAAAAAEAKgEBHQAA" +
           "AA4AAABSRVNQT05TRV9DT0RFUwAP/////wAAAAAAAQAqAQEbAAAADAAAAFNlcnZpY2VFcnJvcgAb////" +
           "/wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAA1YIkKAgAAAAIADAAAAE1hbnVmYWN0dXJlcgEB" +
           "qAIDAAAAADAAAABOYW1lIG9mIHRoZSBjb21wYW55IHRoYXQgbWFudWZhY3R1cmVkIHRoZSBkZXZpY2UA" +
           "LgBEqAIAAAAV/////wEB/////wAAAAA1YIkKAgAAAAIABQAAAE1vZGVsAQGpAgMAAAAAGAAAAE1vZGVs" +
           "IG5hbWUgb2YgdGhlIGRldmljZQAuAESpAgAAABX/////AQH/////AAAAADVgiQoCAAAAAgAQAAAASGFy" +
           "ZHdhcmVSZXZpc2lvbgEBrQIDAAAAACwAAABSZXZpc2lvbiBsZXZlbCBvZiB0aGUgaGFyZHdhcmUgb2Yg" +
           "dGhlIGRldmljZQAuAEStAgAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAQAAAAU29mdHdhcmVSZXZp" +
           "c2lvbgEBrAIDAAAAADUAAABSZXZpc2lvbiBsZXZlbCBvZiB0aGUgc29mdHdhcmUvZmlybXdhcmUgb2Yg" +
           "dGhlIGRldmljZQAuAESsAgAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAOAAAARGV2aWNlUmV2aXNp" +
           "b24BAasCAwAAAAAkAAAAT3ZlcmFsbCByZXZpc2lvbiBsZXZlbCBvZiB0aGUgZGV2aWNlAC4ARKsCAAAA" +
           "DP////8BAf////8AAAAANWCJCgIAAAACAAwAAABEZXZpY2VNYW51YWwBAaoCAwAAAABaAAAAQWRkcmVz" +
           "cyAocGF0aG5hbWUgaW4gdGhlIGZpbGUgc3lzdGVtIG9yIGEgVVJMIHwgV2ViIGFkZHJlc3MpIG9mIHVz" +
           "ZXIgbWFudWFsIGZvciB0aGUgZGV2aWNlAC4ARKoCAAAADP////8BAf////8AAAAANWCJCgIAAAACAAwA" +
           "AABTZXJpYWxOdW1iZXIBAaYCAwAAAABNAAAASWRlbnRpZmllciB0aGF0IHVuaXF1ZWx5IGlkZW50aWZp" +
           "ZXMsIHdpdGhpbiBhIG1hbnVmYWN0dXJlciwgYSBkZXZpY2UgaW5zdGFuY2UALgBEpgIAAAAM/////wEB" +
           "/////wAAAAA1YIkKAgAAAAIADwAAAFJldmlzaW9uQ291bnRlcgEBpwIDAAAAAGkAAABBbiBpbmNyZW1l" +
           "bnRhbCBjb3VudGVyIGluZGljYXRpbmcgdGhlIG51bWJlciBvZiB0aW1lcyB0aGUgc3RhdGljIGRhdGEg" +
           "d2l0aGluIHRoZSBEZXZpY2UgaGFzIGJlZW4gbW9kaWZpZWQALgBEpwIAAAAG/////wEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public ServerCommunicationPROFIBUSServiceState ServiceProvider
        {
            get
            {
                return m_serviceProvider;
            }

            set
            {
                if (!Object.ReferenceEquals(m_serviceProvider, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_serviceProvider = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_serviceProvider != null)
            {
                children.Add(m_serviceProvider);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Opc.Ua.Fdi7.BrowseNames.ServiceProvider:
                {
                    if (createOrReplace)
                    {
                        if (ServiceProvider == null)
                        {
                            if (replacement == null)
                            {
                                ServiceProvider = new ServerCommunicationPROFIBUSServiceState(this);
                            }
                            else
                            {
                                ServiceProvider = (ServerCommunicationPROFIBUSServiceState)replacement;
                            }
                        }
                    }

                    instance = ServiceProvider;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private ServerCommunicationPROFIBUSServiceState m_serviceProvider;
        #endregion
    }
    #endif
    #endregion

    #region ServerCommunicationPROFINETDeviceState Class
    #if (!OPCUA_EXCLUDE_ServerCommunicationPROFINETDeviceState)
    /// <summary>
    /// Stores an instance of the ServerCommunicationPROFINETDeviceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ServerCommunicationPROFINETDeviceState : ServerCommunicationDeviceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ServerCommunicationPROFINETDeviceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationPROFINETDeviceType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);

            if (MethodSet != null)
            {
                MethodSet.Initialize(context, MethodSet_InitializationString);
            }
        }

        #region Initialization String
        private const string MethodSet_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////yRggAoBAAAAAgAJAAAATWV0aG9kU2V0AQHEAgMAAAAAFAAA" +
           "AEZsYXQgbGlzdCBvZiBNZXRob2RzAC8AOsQCAAD/////AQAAAARhggoEAAAAAQAKAAAAU2V0QWRkcmVz" +
           "cwEBCwMALwEBCwMLAwAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAQwDAC4A" +
           "RAwDAACWBQAAAAEAKgEBFgAAAAMAAABNQUMAAwEAAAABAAAABgAAAAABACoBARUAAAACAAAASVAAAwEA" +
           "AAABAAAABAAAAAABACoBARYAAAAHAAAARE5TTkFNRQAM/////wAAAAAAAQAqAQEdAAAACgAAAFN1Ym5l" +
           "dE1hc2sAAwEAAAABAAAABAAAAAABACoBARoAAAAHAAAAR2F0ZXdheQADAQAAAAEAAAAEAAAAAAEAKAEB" +
           "AAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1bWVudHMBAQ0DAC4ARA0D" +
           "AACWAQAAAAEAKgEBGwAAAAwAAABTZXJ2aWNlRXJyb3IAG/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAAB" +
           "Af////8AAAAA";

        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAtAAAAU2VydmVyQ29tbXVuaWNhdGlvblBS" +
           "T0ZJTkVURGV2aWNlVHlwZUluc3RhbmNlAQHBAgEBwQLBAgAA/////wsAAAAkYIAKAQAAAAIACQAAAE1l" +
           "dGhvZFNldAEBxAIDAAAAABQAAABGbGF0IGxpc3Qgb2YgTWV0aG9kcwAvADrEAgAA/////wEAAAAEYYIK" +
           "BAAAAAEACgAAAFNldEFkZHJlc3MBAQsDAC8BAQsDCwMAAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElu" +
           "cHV0QXJndW1lbnRzAQEMAwAuAEQMAwAAlgUAAAABACoBARYAAAADAAAATUFDAAMBAAAAAQAAAAYAAAAA" +
           "AQAqAQEVAAAAAgAAAElQAAMBAAAAAQAAAAQAAAAAAQAqAQEWAAAABwAAAEROU05BTUUADP////8AAAAA" +
           "AAEAKgEBHQAAAAoAAABTdWJuZXRNYXNrAAMBAAAAAQAAAAQAAAAAAQAqAQEaAAAABwAAAEdhdGV3YXkA" +
           "AwEAAAABAAAABAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0" +
           "QXJndW1lbnRzAQENAwAuAEQNAwAAlgEAAAABACoBARsAAAAMAAAAU2VydmljZUVycm9yABv/////AAAA" +
           "AAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAADVgiQoCAAAAAgAMAAAATWFudWZhY3R1cmVyAQHaAgMA" +
           "AAAAMAAAAE5hbWUgb2YgdGhlIGNvbXBhbnkgdGhhdCBtYW51ZmFjdHVyZWQgdGhlIGRldmljZQAuAETa" +
           "AgAAABX/////AQH/////AAAAADVgiQoCAAAAAgAFAAAATW9kZWwBAdsCAwAAAAAYAAAATW9kZWwgbmFt" +
           "ZSBvZiB0aGUgZGV2aWNlAC4ARNsCAAAAFf////8BAf////8AAAAANWCJCgIAAAACABAAAABIYXJkd2Fy" +
           "ZVJldmlzaW9uAQHfAgMAAAAALAAAAFJldmlzaW9uIGxldmVsIG9mIHRoZSBoYXJkd2FyZSBvZiB0aGUg" +
           "ZGV2aWNlAC4ARN8CAAAADP////8BAf////8AAAAANWCJCgIAAAACABAAAABTb2Z0d2FyZVJldmlzaW9u" +
           "AQHeAgMAAAAANQAAAFJldmlzaW9uIGxldmVsIG9mIHRoZSBzb2Z0d2FyZS9maXJtd2FyZSBvZiB0aGUg" +
           "ZGV2aWNlAC4ARN4CAAAADP////8BAf////8AAAAANWCJCgIAAAACAA4AAABEZXZpY2VSZXZpc2lvbgEB" +
           "3QIDAAAAACQAAABPdmVyYWxsIHJldmlzaW9uIGxldmVsIG9mIHRoZSBkZXZpY2UALgBE3QIAAAAM////" +
           "/wEB/////wAAAAA1YIkKAgAAAAIADAAAAERldmljZU1hbnVhbAEB3AIDAAAAAFoAAABBZGRyZXNzIChw" +
           "YXRobmFtZSBpbiB0aGUgZmlsZSBzeXN0ZW0gb3IgYSBVUkwgfCBXZWIgYWRkcmVzcykgb2YgdXNlciBt" +
           "YW51YWwgZm9yIHRoZSBkZXZpY2UALgBE3AIAAAAM/////wEB/////wAAAAA1YIkKAgAAAAIADAAAAFNl" +
           "cmlhbE51bWJlcgEB2AIDAAAAAE0AAABJZGVudGlmaWVyIHRoYXQgdW5pcXVlbHkgaWRlbnRpZmllcywg" +
           "d2l0aGluIGEgbWFudWZhY3R1cmVyLCBhIGRldmljZSBpbnN0YW5jZQAuAETYAgAAAAz/////AQH/////" +
           "AAAAADVgiQoCAAAAAgAPAAAAUmV2aXNpb25Db3VudGVyAQHZAgMAAAAAaQAAAEFuIGluY3JlbWVudGFs" +
           "IGNvdW50ZXIgaW5kaWNhdGluZyB0aGUgbnVtYmVyIG9mIHRpbWVzIHRoZSBzdGF0aWMgZGF0YSB3aXRo" +
           "aW4gdGhlIERldmljZSBoYXMgYmVlbiBtb2RpZmllZAAuAETZAgAAAAb/////AQH/////AAAAABdgiQoC" +
           "AAAAAQAbAAAATGlzdE9mQ29tbXVuaWNhdGlvblByb2ZpbGVzAQGdOgAuAESdOgAAAAwBAAAAAQAAAAAA" +
           "AAABAf////8AAAAABGCACgEAAAABAA8AAABTZXJ2aWNlUHJvdmlkZXIBAQ4DAC8BAcYEDgMAAP////8J" +
           "AAAAJGCACgEAAAACAAkAAABNZXRob2RTZXQBAREDAwAAAAAUAAAARmxhdCBsaXN0IG9mIE1ldGhvZHMA" +
           "LwA6EQMAAP////8DAAAABGGCCgQAAAABAAoAAABEaXNjb25uZWN0AQE1AwAvAQEsATUDAAABAf////8C" +
           "AAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBNgMALgBENgMAAJYBAAAAAQAqAQEmAAAAFwAA" +
           "AENvbW11bmljYXRpb25SZWxhdGlvbklkAA//////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAA" +
           "ABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQE3AwAuAEQ3AwAAlgEAAAABACoBARsAAAAMAAAA" +
           "U2VydmljZUVycm9yABv/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAAARhggoEAAAAAQAH" +
           "AAAAQ29ubmVjdAEBOAMALwEBCQU4AwAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVu" +
           "dHMBATkDAC4ARDkDAACWBAAAAAEAKgEBJgAAABcAAABDb21tdW5pY2F0aW9uUmVsYXRpb25JZAAP////" +
           "/wAAAAAAAQAqAQEWAAAABwAAAEROU05BTUUADP////8AAAAAAAEAKgEBFwAAAAgAAABEZXZpY2VJRAAF" +
           "/////wAAAAAAAQAqAQEXAAAACAAAAFZlbmRvcklEAAX/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/" +
           "////AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQE6AwAuAEQ6AwAAlgEAAAABACoBARsA" +
           "AAAMAAAAU2VydmljZUVycm9yABv/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAAARhggoE" +
           "AAAAAQAIAAAAVHJhbnNmZXIBATsDAC8BAQwFOwMAAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElucHV0" +
           "QXJndW1lbnRzAQE8AwAuAEQ8AwAAlgcAAAABACoBASYAAAAXAAAAQ29tbXVuaWNhdGlvblJlbGF0aW9u" +
           "SWQAD/////8AAAAAAAEAKgEBGAAAAAkAAABPUEVSQVRJT04ADP////8AAAAAAAEAKgEBEwAAAAQAAABT" +
           "TE9UAAX/////AAAAAAABACoBARYAAAAHAAAAU1VCU0xPVAAF/////wAAAAAAAQAqAQEUAAAABQAAAElO" +
           "REVYAAX/////AAAAAAABACoBARIAAAADAAAAQVBJAAf/////AAAAAAABACoBARYAAAAHAAAAUkVRVUVT" +
           "VAAP/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAXYKkKAgAAAAAADwAAAE91dHB1dEFy" +
           "Z3VtZW50cwEBPQMALgBEPQMAAJYDAAAAAQAqAQEUAAAABQAAAFJFUExZAA//////AAAAAAABACoBAR0A" +
           "AAAOAAAAUkVTUE9OU0VfQ09ERVMAD/////8AAAAAAAEAKgEBGwAAAAwAAABTZXJ2aWNlRXJyb3IAG///" +
           "//8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAANWCJCgIAAAACAAwAAABNYW51ZmFjdHVyZXIB" +
           "ASUDAwAAAAAwAAAATmFtZSBvZiB0aGUgY29tcGFueSB0aGF0IG1hbnVmYWN0dXJlZCB0aGUgZGV2aWNl" +
           "AC4ARCUDAAAAFf////8BAf////8AAAAANWCJCgIAAAACAAUAAABNb2RlbAEBJgMDAAAAABgAAABNb2Rl" +
           "bCBuYW1lIG9mIHRoZSBkZXZpY2UALgBEJgMAAAAV/////wEB/////wAAAAA1YIkKAgAAAAIAEAAAAEhh" +
           "cmR3YXJlUmV2aXNpb24BASoDAwAAAAAsAAAAUmV2aXNpb24gbGV2ZWwgb2YgdGhlIGhhcmR3YXJlIG9m" +
           "IHRoZSBkZXZpY2UALgBEKgMAAAAM/////wEB/////wAAAAA1YIkKAgAAAAIAEAAAAFNvZnR3YXJlUmV2" +
           "aXNpb24BASkDAwAAAAA1AAAAUmV2aXNpb24gbGV2ZWwgb2YgdGhlIHNvZnR3YXJlL2Zpcm13YXJlIG9m" +
           "IHRoZSBkZXZpY2UALgBEKQMAAAAM/////wEB/////wAAAAA1YIkKAgAAAAIADgAAAERldmljZVJldmlz" +
           "aW9uAQEoAwMAAAAAJAAAAE92ZXJhbGwgcmV2aXNpb24gbGV2ZWwgb2YgdGhlIGRldmljZQAuAEQoAwAA" +
           "AAz/////AQH/////AAAAADVgiQoCAAAAAgAMAAAARGV2aWNlTWFudWFsAQEnAwMAAAAAWgAAAEFkZHJl" +
           "c3MgKHBhdGhuYW1lIGluIHRoZSBmaWxlIHN5c3RlbSBvciBhIFVSTCB8IFdlYiBhZGRyZXNzKSBvZiB1" +
           "c2VyIG1hbnVhbCBmb3IgdGhlIGRldmljZQAuAEQnAwAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAM" +
           "AAAAU2VyaWFsTnVtYmVyAQEjAwMAAAAATQAAAElkZW50aWZpZXIgdGhhdCB1bmlxdWVseSBpZGVudGlm" +
           "aWVzLCB3aXRoaW4gYSBtYW51ZmFjdHVyZXIsIGEgZGV2aWNlIGluc3RhbmNlAC4ARCMDAAAADP////8B" +
           "Af////8AAAAANWCJCgIAAAACAA8AAABSZXZpc2lvbkNvdW50ZXIBASQDAwAAAABpAAAAQW4gaW5jcmVt" +
           "ZW50YWwgY291bnRlciBpbmRpY2F0aW5nIHRoZSBudW1iZXIgb2YgdGltZXMgdGhlIHN0YXRpYyBkYXRh" +
           "IHdpdGhpbiB0aGUgRGV2aWNlIGhhcyBiZWVuIG1vZGlmaWVkAC4ARCQDAAAABv////8BAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public ServerCommunicationPROFINETServiceState ServiceProvider
        {
            get
            {
                return m_serviceProvider;
            }

            set
            {
                if (!Object.ReferenceEquals(m_serviceProvider, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_serviceProvider = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_serviceProvider != null)
            {
                children.Add(m_serviceProvider);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Opc.Ua.Fdi7.BrowseNames.ServiceProvider:
                {
                    if (createOrReplace)
                    {
                        if (ServiceProvider == null)
                        {
                            if (replacement == null)
                            {
                                ServiceProvider = new ServerCommunicationPROFINETServiceState(this);
                            }
                            else
                            {
                                ServiceProvider = (ServerCommunicationPROFINETServiceState)replacement;
                            }
                        }
                    }

                    instance = ServiceProvider;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private ServerCommunicationPROFINETServiceState m_serviceProvider;
        #endregion
    }
    #endif
    #endregion

    #region ServerCommunicationHARState Class
    #if (!OPCUA_EXCLUDE_ServerCommunicationHARState)
    /// <summary>
    /// Stores an instance of the ServerCommunicationHARType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ServerCommunicationHARState : ServerCommunicationDeviceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ServerCommunicationHARState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationHARType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);

            if (MethodSet != null)
            {
                MethodSet.Initialize(context, MethodSet_InitializationString);
            }
        }

        #region Initialization String
        private const string MethodSet_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////yRggAoBAAAAAgAJAAAATWV0aG9kU2V0AQFBAwMAAAAAFAAA" +
           "AEZsYXQgbGlzdCBvZiBNZXRob2RzAC8AOkEDAAD/////AQAAAARhggoEAAAAAQAKAAAAU2V0QWRkcmVz" +
           "cwEBiAMALwEBiAOIAwAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAYkDAC4A" +
           "RIkDAACWAgAAAAEAKgEBHQAAAA4AAABPbGRQb2xsQWRkcmVzcwAD/////wAAAAAAAQAqAQEdAAAADgAA" +
           "AE5ld1BvbGxBZGRyZXNzAAP/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAA" +
           "AAAPAAAAT3V0cHV0QXJndW1lbnRzAQGKAwAuAESKAwAAlgEAAAABACoBARsAAAAMAAAAU2VydmljZUVy" +
           "cm9yABv/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAAA==";

        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAiAAAAU2VydmVyQ29tbXVuaWNhdGlvbkhB" +
           "UlR5cGVJbnN0YW5jZQEBPgMBAT4DPgMAAP////8LAAAAJGCACgEAAAACAAkAAABNZXRob2RTZXQBAUED" +
           "AwAAAAAUAAAARmxhdCBsaXN0IG9mIE1ldGhvZHMALwA6QQMAAP////8BAAAABGGCCgQAAAABAAoAAABT" +
           "ZXRBZGRyZXNzAQGIAwAvAQGIA4gDAAABAf////8CAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50" +
           "cwEBiQMALgBEiQMAAJYCAAAAAQAqAQEdAAAADgAAAE9sZFBvbGxBZGRyZXNzAAP/////AAAAAAABACoB" +
           "AR0AAAAOAAAATmV3UG9sbEFkZHJlc3MAA/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAA" +
           "F2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1bWVudHMBAYoDAC4ARIoDAACWAQAAAAEAKgEBGwAAAAwAAABT" +
           "ZXJ2aWNlRXJyb3IAG/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAANWCJCgIAAAACAAwA" +
           "AABNYW51ZmFjdHVyZXIBAVcDAwAAAAAwAAAATmFtZSBvZiB0aGUgY29tcGFueSB0aGF0IG1hbnVmYWN0" +
           "dXJlZCB0aGUgZGV2aWNlAC4ARFcDAAAAFf////8BAf////8AAAAANWCJCgIAAAACAAUAAABNb2RlbAEB" +
           "WAMDAAAAABgAAABNb2RlbCBuYW1lIG9mIHRoZSBkZXZpY2UALgBEWAMAAAAV/////wEB/////wAAAAA1" +
           "YIkKAgAAAAIAEAAAAEhhcmR3YXJlUmV2aXNpb24BAVwDAwAAAAAsAAAAUmV2aXNpb24gbGV2ZWwgb2Yg" +
           "dGhlIGhhcmR3YXJlIG9mIHRoZSBkZXZpY2UALgBEXAMAAAAM/////wEB/////wAAAAA1YIkKAgAAAAIA" +
           "EAAAAFNvZnR3YXJlUmV2aXNpb24BAVsDAwAAAAA1AAAAUmV2aXNpb24gbGV2ZWwgb2YgdGhlIHNvZnR3" +
           "YXJlL2Zpcm13YXJlIG9mIHRoZSBkZXZpY2UALgBEWwMAAAAM/////wEB/////wAAAAA1YIkKAgAAAAIA" +
           "DgAAAERldmljZVJldmlzaW9uAQFaAwMAAAAAJAAAAE92ZXJhbGwgcmV2aXNpb24gbGV2ZWwgb2YgdGhl" +
           "IGRldmljZQAuAERaAwAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAMAAAARGV2aWNlTWFudWFsAQFZ" +
           "AwMAAAAAWgAAAEFkZHJlc3MgKHBhdGhuYW1lIGluIHRoZSBmaWxlIHN5c3RlbSBvciBhIFVSTCB8IFdl" +
           "YiBhZGRyZXNzKSBvZiB1c2VyIG1hbnVhbCBmb3IgdGhlIGRldmljZQAuAERZAwAAAAz/////AQH/////" +
           "AAAAADVgiQoCAAAAAgAMAAAAU2VyaWFsTnVtYmVyAQFVAwMAAAAATQAAAElkZW50aWZpZXIgdGhhdCB1" +
           "bmlxdWVseSBpZGVudGlmaWVzLCB3aXRoaW4gYSBtYW51ZmFjdHVyZXIsIGEgZGV2aWNlIGluc3RhbmNl" +
           "AC4ARFUDAAAADP////8BAf////8AAAAANWCJCgIAAAACAA8AAABSZXZpc2lvbkNvdW50ZXIBAVYDAwAA" +
           "AABpAAAAQW4gaW5jcmVtZW50YWwgY291bnRlciBpbmRpY2F0aW5nIHRoZSBudW1iZXIgb2YgdGltZXMg" +
           "dGhlIHN0YXRpYyBkYXRhIHdpdGhpbiB0aGUgRGV2aWNlIGhhcyBiZWVuIG1vZGlmaWVkAC4ARFYDAAAA" +
           "Bv////8BAf////8AAAAAF2CJCgIAAAABABsAAABMaXN0T2ZDb21tdW5pY2F0aW9uUHJvZmlsZXMBAZ46" +
           "AC4ARJ46AAAADAEAAAABAAAAAAAAAAEB/////wAAAAAEYIAKAQAAAAEADwAAAFNlcnZpY2VQcm92aWRl" +
           "cgEBiwMALwEBDwWLAwAA/////wkAAAAkYIAKAQAAAAIACQAAAE1ldGhvZFNldAEBjgMDAAAAABQAAABG" +
           "bGF0IGxpc3Qgb2YgTWV0aG9kcwAvADqOAwAA/////wMAAAAEYYIKBAAAAAEACgAAAERpc2Nvbm5lY3QB" +
           "AbIDAC8BASwBsgMAAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElucHV0QXJndW1lbnRzAQGzAwAuAESz" +
           "AwAAlgEAAAABACoBASYAAAAXAAAAQ29tbXVuaWNhdGlvblJlbGF0aW9uSWQAD/////8AAAAAAAEAKAEB" +
           "AAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1bWVudHMBAbQDAC4ARLQD" +
           "AACWAQAAAAEAKgEBGwAAAAwAAABTZXJ2aWNlRXJyb3IAG/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAAB" +
           "Af////8AAAAABGGCCgQAAAABAAcAAABDb25uZWN0AQG1AwAvAQFSBbUDAAABAf////8CAAAAF2CpCgIA" +
           "AAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBtgMALgBEtgMAAJYCAAAAAQAqAQEmAAAAFwAAAENvbW11bmlj" +
           "YXRpb25SZWxhdGlvbklkAA//////AAAAAAABACoBARoAAAALAAAATG9uZ0FkZHJlc3MAD/////8AAAAA" +
           "AAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1bWVudHMBAbcD" +
           "AC4ARLcDAACWAQAAAAEAKgEBGwAAAAwAAABTZXJ2aWNlRXJyb3IAG/////8AAAAAAAEAKAEBAAAAAQAA" +
           "AAAAAAABAf////8AAAAABGGCCgQAAAABAAgAAABUcmFuc2ZlcgEBuAMALwEBVQW4AwAAAQH/////AgAA" +
           "ABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAbkDAC4ARLkDAACWAwAAAAEAKgEBJgAAABcAAABD" +
           "b21tdW5pY2F0aW9uUmVsYXRpb25JZAAP/////wAAAAAAAQAqAQEWAAAABwAAAENvbW1hbmQABf////8A" +
           "AAAAAAEAKgEBFgAAAAcAAABSZXF1ZXN0AA//////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAA" +
           "ABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQG6AwAuAES6AwAAlgIAAAABACoBARQAAAAFAAAA" +
           "UmVwbHkAD/////8AAAAAAAEAKgEBGwAAAAwAAABTZXJ2aWNlRXJyb3IAG/////8AAAAAAAEAKAEBAAAA" +
           "AQAAAAAAAAABAf////8AAAAANWCJCgIAAAACAAwAAABNYW51ZmFjdHVyZXIBAaIDAwAAAAAwAAAATmFt" +
           "ZSBvZiB0aGUgY29tcGFueSB0aGF0IG1hbnVmYWN0dXJlZCB0aGUgZGV2aWNlAC4ARKIDAAAAFf////8B" +
           "Af////8AAAAANWCJCgIAAAACAAUAAABNb2RlbAEBowMDAAAAABgAAABNb2RlbCBuYW1lIG9mIHRoZSBk" +
           "ZXZpY2UALgBEowMAAAAV/////wEB/////wAAAAA1YIkKAgAAAAIAEAAAAEhhcmR3YXJlUmV2aXNpb24B" +
           "AacDAwAAAAAsAAAAUmV2aXNpb24gbGV2ZWwgb2YgdGhlIGhhcmR3YXJlIG9mIHRoZSBkZXZpY2UALgBE" +
           "pwMAAAAM/////wEB/////wAAAAA1YIkKAgAAAAIAEAAAAFNvZnR3YXJlUmV2aXNpb24BAaYDAwAAAAA1" +
           "AAAAUmV2aXNpb24gbGV2ZWwgb2YgdGhlIHNvZnR3YXJlL2Zpcm13YXJlIG9mIHRoZSBkZXZpY2UALgBE" +
           "pgMAAAAM/////wEB/////wAAAAA1YIkKAgAAAAIADgAAAERldmljZVJldmlzaW9uAQGlAwMAAAAAJAAA" +
           "AE92ZXJhbGwgcmV2aXNpb24gbGV2ZWwgb2YgdGhlIGRldmljZQAuAESlAwAAAAz/////AQH/////AAAA" +
           "ADVgiQoCAAAAAgAMAAAARGV2aWNlTWFudWFsAQGkAwMAAAAAWgAAAEFkZHJlc3MgKHBhdGhuYW1lIGlu" +
           "IHRoZSBmaWxlIHN5c3RlbSBvciBhIFVSTCB8IFdlYiBhZGRyZXNzKSBvZiB1c2VyIG1hbnVhbCBmb3Ig" +
           "dGhlIGRldmljZQAuAESkAwAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAMAAAAU2VyaWFsTnVtYmVy" +
           "AQGgAwMAAAAATQAAAElkZW50aWZpZXIgdGhhdCB1bmlxdWVseSBpZGVudGlmaWVzLCB3aXRoaW4gYSBt" +
           "YW51ZmFjdHVyZXIsIGEgZGV2aWNlIGluc3RhbmNlAC4ARKADAAAADP////8BAf////8AAAAANWCJCgIA" +
           "AAACAA8AAABSZXZpc2lvbkNvdW50ZXIBAaEDAwAAAABpAAAAQW4gaW5jcmVtZW50YWwgY291bnRlciBp" +
           "bmRpY2F0aW5nIHRoZSBudW1iZXIgb2YgdGltZXMgdGhlIHN0YXRpYyBkYXRhIHdpdGhpbiB0aGUgRGV2" +
           "aWNlIGhhcyBiZWVuIG1vZGlmaWVkAC4ARKEDAAAABv////8BAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public ServerCommunicationHARTServiceState ServiceProvider
        {
            get
            {
                return m_serviceProvider;
            }

            set
            {
                if (!Object.ReferenceEquals(m_serviceProvider, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_serviceProvider = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_serviceProvider != null)
            {
                children.Add(m_serviceProvider);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Opc.Ua.Fdi7.BrowseNames.ServiceProvider:
                {
                    if (createOrReplace)
                    {
                        if (ServiceProvider == null)
                        {
                            if (replacement == null)
                            {
                                ServiceProvider = new ServerCommunicationHARTServiceState(this);
                            }
                            else
                            {
                                ServiceProvider = (ServerCommunicationHARTServiceState)replacement;
                            }
                        }
                    }

                    instance = ServiceProvider;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private ServerCommunicationHARTServiceState m_serviceProvider;
        #endregion
    }
    #endif
    #endregion

    #region ServerCommunicationISA100_WirelessDeviceState Class
    #if (!OPCUA_EXCLUDE_ServerCommunicationISA100_WirelessDeviceState)
    /// <summary>
    /// Stores an instance of the ServerCommunicationISA100_WirelessDeviceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ServerCommunicationISA100_WirelessDeviceState : ServerCommunicationDeviceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ServerCommunicationISA100_WirelessDeviceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationISA100_WirelessDeviceType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQA0AAAAU2VydmVyQ29tbXVuaWNhdGlvbklT" +
           "QTEwMF9XaXJlbGVzc0RldmljZVR5cGVJbnN0YW5jZQEB/AYBAfwG/AYAAP////8KAAAANWCJCgIAAAAC" +
           "AAwAAABNYW51ZmFjdHVyZXIBARUHAwAAAAAwAAAATmFtZSBvZiB0aGUgY29tcGFueSB0aGF0IG1hbnVm" +
           "YWN0dXJlZCB0aGUgZGV2aWNlAC4ARBUHAAAAFf////8BAf////8AAAAANWCJCgIAAAACAAUAAABNb2Rl" +
           "bAEBFgcDAAAAABgAAABNb2RlbCBuYW1lIG9mIHRoZSBkZXZpY2UALgBEFgcAAAAV/////wEB/////wAA" +
           "AAA1YIkKAgAAAAIAEAAAAEhhcmR3YXJlUmV2aXNpb24BARoHAwAAAAAsAAAAUmV2aXNpb24gbGV2ZWwg" +
           "b2YgdGhlIGhhcmR3YXJlIG9mIHRoZSBkZXZpY2UALgBEGgcAAAAM/////wEB/////wAAAAA1YIkKAgAA" +
           "AAIAEAAAAFNvZnR3YXJlUmV2aXNpb24BARkHAwAAAAA1AAAAUmV2aXNpb24gbGV2ZWwgb2YgdGhlIHNv" +
           "ZnR3YXJlL2Zpcm13YXJlIG9mIHRoZSBkZXZpY2UALgBEGQcAAAAM/////wEB/////wAAAAA1YIkKAgAA" +
           "AAIADgAAAERldmljZVJldmlzaW9uAQEYBwMAAAAAJAAAAE92ZXJhbGwgcmV2aXNpb24gbGV2ZWwgb2Yg" +
           "dGhlIGRldmljZQAuAEQYBwAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAMAAAARGV2aWNlTWFudWFs" +
           "AQEXBwMAAAAAWgAAAEFkZHJlc3MgKHBhdGhuYW1lIGluIHRoZSBmaWxlIHN5c3RlbSBvciBhIFVSTCB8" +
           "IFdlYiBhZGRyZXNzKSBvZiB1c2VyIG1hbnVhbCBmb3IgdGhlIGRldmljZQAuAEQXBwAAAAz/////AQH/" +
           "////AAAAADVgiQoCAAAAAgAMAAAAU2VyaWFsTnVtYmVyAQETBwMAAAAATQAAAElkZW50aWZpZXIgdGhh" +
           "dCB1bmlxdWVseSBpZGVudGlmaWVzLCB3aXRoaW4gYSBtYW51ZmFjdHVyZXIsIGEgZGV2aWNlIGluc3Rh" +
           "bmNlAC4ARBMHAAAADP////8BAf////8AAAAANWCJCgIAAAACAA8AAABSZXZpc2lvbkNvdW50ZXIBARQH" +
           "AwAAAABpAAAAQW4gaW5jcmVtZW50YWwgY291bnRlciBpbmRpY2F0aW5nIHRoZSBudW1iZXIgb2YgdGlt" +
           "ZXMgdGhlIHN0YXRpYyBkYXRhIHdpdGhpbiB0aGUgRGV2aWNlIGhhcyBiZWVuIG1vZGlmaWVkAC4ARBQH" +
           "AAAABv////8BAf////8AAAAAF2CJCgIAAAABABsAAABMaXN0T2ZDb21tdW5pY2F0aW9uUHJvZmlsZXMB" +
           "AZ86AC4ARJ86AAAADAEAAAABAAAAAAAAAAEB/////wAAAAAEYIAKAQAAAAEADwAAAFNlcnZpY2VQcm92" +
           "aWRlcgEBRgcALwEBCQhGBwAA/////wkAAAAkYIAKAQAAAAIACQAAAE1ldGhvZFNldAEBSQcDAAAAABQA" +
           "AABGbGF0IGxpc3Qgb2YgTWV0aG9kcwAvADpJBwAA/////wMAAAAEYYIKBAAAAAEACgAAAERpc2Nvbm5l" +
           "Y3QBAW0HAC8BASwBbQcAAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElucHV0QXJndW1lbnRzAQFuBwAu" +
           "AERuBwAAlgEAAAABACoBASYAAAAXAAAAQ29tbXVuaWNhdGlvblJlbGF0aW9uSWQAD/////8AAAAAAAEA" +
           "KAEBAAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1bWVudHMBAW8HAC4A" +
           "RG8HAACWAQAAAAEAKgEBGwAAAAwAAABTZXJ2aWNlRXJyb3IAG/////8AAAAAAAEAKAEBAAAAAQAAAAAA" +
           "AAABAf////8AAAAABGGCCgQAAAABAAcAAABDb25uZWN0AQFwBwAvAQFMCHAHAAABAf////8CAAAAF2Cp" +
           "CgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBcQcALgBEcQcAAJYDAAAAAQAqAQEmAAAAFwAAAENvbW11" +
           "bmljYXRpb25SZWxhdGlvbklkAA//////AAAAAAABACoBARgAAAAJAAAASVBBZGRyZXNzAA//////AAAA" +
           "AAABACoBARoAAAALAAAAQ29ubmVjdFR5cGUAB/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8A" +
           "AAAAF2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1bWVudHMBAXIHAC4ARHIHAACWAQAAAAEAKgEBGwAAAAwA" +
           "AABTZXJ2aWNlRXJyb3IAG/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAABGGCCgQAAAAB" +
           "AAgAAABUcmFuc2ZlcgEBcwcALwEBTwhzBwAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1" +
           "bWVudHMBAXQHAC4ARHQHAACWCAAAAAEAKgEBJgAAABcAAABDb21tdW5pY2F0aW9uUmVsYXRpb25JZAAP" +
           "/////wAAAAAAAQAqAQEYAAAACQAAAE9QRVJBVElPTgAM/////wAAAAAAAQAqAQEUAAAABQAAAEFwcElE" +
           "AAX/////AAAAAAABACoBARcAAAAIAAAAT2JqZWN0SUQABf////8AAAAAAAEAKgEBGwAAAAwAAABBdHRy" +
           "T3JNZXRoSUQABf////8AAAAAAAEAKgEBGAAAAAkAAABTVUJfSU5ERVgAB/////8AAAAAAAEAKgEBHAAA" +
           "AAkAAABXcml0ZURhdGEAAwEAAAABAAAAAAAAAAABACoBARgAAAAJAAAAUmVxdWVzdElkAAf/////AAAA" +
           "AAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQF1" +
           "BwAuAER1BwAAlgIAAAABACoBARsAAAAIAAAAUmVhZERhdGEAAwEAAAABAAAAAAAAAAABACoBARsAAAAM" +
           "AAAAU2VydmljZUVycm9yABv/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAADVgiQoCAAAA" +
           "AgAMAAAATWFudWZhY3R1cmVyAQFdBwMAAAAAMAAAAE5hbWUgb2YgdGhlIGNvbXBhbnkgdGhhdCBtYW51" +
           "ZmFjdHVyZWQgdGhlIGRldmljZQAuAERdBwAAABX/////AQH/////AAAAADVgiQoCAAAAAgAFAAAATW9k" +
           "ZWwBAV4HAwAAAAAYAAAATW9kZWwgbmFtZSBvZiB0aGUgZGV2aWNlAC4ARF4HAAAAFf////8BAf////8A" +
           "AAAANWCJCgIAAAACABAAAABIYXJkd2FyZVJldmlzaW9uAQFiBwMAAAAALAAAAFJldmlzaW9uIGxldmVs" +
           "IG9mIHRoZSBoYXJkd2FyZSBvZiB0aGUgZGV2aWNlAC4ARGIHAAAADP////8BAf////8AAAAANWCJCgIA" +
           "AAACABAAAABTb2Z0d2FyZVJldmlzaW9uAQFhBwMAAAAANQAAAFJldmlzaW9uIGxldmVsIG9mIHRoZSBz" +
           "b2Z0d2FyZS9maXJtd2FyZSBvZiB0aGUgZGV2aWNlAC4ARGEHAAAADP////8BAf////8AAAAANWCJCgIA" +
           "AAACAA4AAABEZXZpY2VSZXZpc2lvbgEBYAcDAAAAACQAAABPdmVyYWxsIHJldmlzaW9uIGxldmVsIG9m" +
           "IHRoZSBkZXZpY2UALgBEYAcAAAAM/////wEB/////wAAAAA1YIkKAgAAAAIADAAAAERldmljZU1hbnVh" +
           "bAEBXwcDAAAAAFoAAABBZGRyZXNzIChwYXRobmFtZSBpbiB0aGUgZmlsZSBzeXN0ZW0gb3IgYSBVUkwg" +
           "fCBXZWIgYWRkcmVzcykgb2YgdXNlciBtYW51YWwgZm9yIHRoZSBkZXZpY2UALgBEXwcAAAAM/////wEB" +
           "/////wAAAAA1YIkKAgAAAAIADAAAAFNlcmlhbE51bWJlcgEBWwcDAAAAAE0AAABJZGVudGlmaWVyIHRo" +
           "YXQgdW5pcXVlbHkgaWRlbnRpZmllcywgd2l0aGluIGEgbWFudWZhY3R1cmVyLCBhIGRldmljZSBpbnN0" +
           "YW5jZQAuAERbBwAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAPAAAAUmV2aXNpb25Db3VudGVyAQFc" +
           "BwMAAAAAaQAAAEFuIGluY3JlbWVudGFsIGNvdW50ZXIgaW5kaWNhdGluZyB0aGUgbnVtYmVyIG9mIHRp" +
           "bWVzIHRoZSBzdGF0aWMgZGF0YSB3aXRoaW4gdGhlIERldmljZSBoYXMgYmVlbiBtb2RpZmllZAAuAERc" +
           "BwAAAAb/////AQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public ServerCommunicationISA100_WirelessServiceState ServiceProvider
        {
            get
            {
                return m_serviceProvider;
            }

            set
            {
                if (!Object.ReferenceEquals(m_serviceProvider, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_serviceProvider = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_serviceProvider != null)
            {
                children.Add(m_serviceProvider);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Opc.Ua.Fdi7.BrowseNames.ServiceProvider:
                {
                    if (createOrReplace)
                    {
                        if (ServiceProvider == null)
                        {
                            if (replacement == null)
                            {
                                ServiceProvider = new ServerCommunicationISA100_WirelessServiceState(this);
                            }
                            else
                            {
                                ServiceProvider = (ServerCommunicationISA100_WirelessServiceState)replacement;
                            }
                        }
                    }

                    instance = ServiceProvider;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private ServerCommunicationISA100_WirelessServiceState m_serviceProvider;
        #endregion
    }
    #endif
    #endregion

    #region ServerCommunicationGENERICDeviceState Class
    #if (!OPCUA_EXCLUDE_ServerCommunicationGENERICDeviceState)
    /// <summary>
    /// Stores an instance of the ServerCommunicationGENERICDeviceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ServerCommunicationGENERICDeviceState : ServerCommunicationDeviceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ServerCommunicationGENERICDeviceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationGENERICDeviceType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);

            if (MethodSet != null)
            {
                MethodSet.Initialize(context, MethodSet_InitializationString);
            }
        }

        #region Initialization String
        private const string MethodSet_InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////yRggAoBAAAAAgAJAAAATWV0aG9kU2V0AQF8BwMAAAAAFAAA" +
           "AEZsYXQgbGlzdCBvZiBNZXRob2RzAC8AOnwHAAD/////AQAAAARhggoEAAAAAQAKAAAAU2V0QWRkcmVz" +
           "cwEBxAcALwEBxAfEBwAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAcUHAC4A" +
           "RMUHAACWAgAAAAEAKgEBGQAAAAoAAABPbGRBZGRyZXNzAA//////AAAAAAABACoBARkAAAAKAAAATmV3" +
           "QWRkcmVzcwAP/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAXYKkKAgAAAAAADwAAAE91" +
           "dHB1dEFyZ3VtZW50cwEBxgcALgBExgcAAJYBAAAAAQAqAQEbAAAADAAAAFNlcnZpY2VFcnJvcgAb////" +
           "/wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAA=";

        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAsAAAAU2VydmVyQ29tbXVuaWNhdGlvbkdF" +
           "TkVSSUNEZXZpY2VUeXBlSW5zdGFuY2UBAXkHAQF5B3kHAAD/////DAAAACRggAoBAAAAAgAJAAAATWV0" +
           "aG9kU2V0AQF8BwMAAAAAFAAAAEZsYXQgbGlzdCBvZiBNZXRob2RzAC8AOnwHAAD/////AQAAAARhggoE" +
           "AAAAAQAKAAAAU2V0QWRkcmVzcwEBxAcALwEBxAfEBwAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5w" +
           "dXRBcmd1bWVudHMBAcUHAC4ARMUHAACWAgAAAAEAKgEBGQAAAAoAAABPbGRBZGRyZXNzAA//////AAAA" +
           "AAABACoBARkAAAAKAAAATmV3QWRkcmVzcwAP/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAA" +
           "AAAXYKkKAgAAAAAADwAAAE91dHB1dEFyZ3VtZW50cwEBxgcALgBExgcAAJYBAAAAAQAqAQEbAAAADAAA" +
           "AFNlcnZpY2VFcnJvcgAb/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAA1YIkKAgAAAAIA" +
           "DAAAAE1hbnVmYWN0dXJlcgEBkgcDAAAAADAAAABOYW1lIG9mIHRoZSBjb21wYW55IHRoYXQgbWFudWZh" +
           "Y3R1cmVkIHRoZSBkZXZpY2UALgBEkgcAAAAV/////wEB/////wAAAAA1YIkKAgAAAAIABQAAAE1vZGVs" +
           "AQGTBwMAAAAAGAAAAE1vZGVsIG5hbWUgb2YgdGhlIGRldmljZQAuAESTBwAAABX/////AQH/////AAAA" +
           "ADVgiQoCAAAAAgAQAAAASGFyZHdhcmVSZXZpc2lvbgEBlwcDAAAAACwAAABSZXZpc2lvbiBsZXZlbCBv" +
           "ZiB0aGUgaGFyZHdhcmUgb2YgdGhlIGRldmljZQAuAESXBwAAAAz/////AQH/////AAAAADVgiQoCAAAA" +
           "AgAQAAAAU29mdHdhcmVSZXZpc2lvbgEBlgcDAAAAADUAAABSZXZpc2lvbiBsZXZlbCBvZiB0aGUgc29m" +
           "dHdhcmUvZmlybXdhcmUgb2YgdGhlIGRldmljZQAuAESWBwAAAAz/////AQH/////AAAAADVgiQoCAAAA" +
           "AgAOAAAARGV2aWNlUmV2aXNpb24BAZUHAwAAAAAkAAAAT3ZlcmFsbCByZXZpc2lvbiBsZXZlbCBvZiB0" +
           "aGUgZGV2aWNlAC4ARJUHAAAADP////8BAf////8AAAAANWCJCgIAAAACAAwAAABEZXZpY2VNYW51YWwB" +
           "AZQHAwAAAABaAAAAQWRkcmVzcyAocGF0aG5hbWUgaW4gdGhlIGZpbGUgc3lzdGVtIG9yIGEgVVJMIHwg" +
           "V2ViIGFkZHJlc3MpIG9mIHVzZXIgbWFudWFsIGZvciB0aGUgZGV2aWNlAC4ARJQHAAAADP////8BAf//" +
           "//8AAAAANWCJCgIAAAACAAwAAABTZXJpYWxOdW1iZXIBAZAHAwAAAABNAAAASWRlbnRpZmllciB0aGF0" +
           "IHVuaXF1ZWx5IGlkZW50aWZpZXMsIHdpdGhpbiBhIG1hbnVmYWN0dXJlciwgYSBkZXZpY2UgaW5zdGFu" +
           "Y2UALgBEkAcAAAAM/////wEB/////wAAAAA1YIkKAgAAAAIADwAAAFJldmlzaW9uQ291bnRlcgEBkQcD" +
           "AAAAAGkAAABBbiBpbmNyZW1lbnRhbCBjb3VudGVyIGluZGljYXRpbmcgdGhlIG51bWJlciBvZiB0aW1l" +
           "cyB0aGUgc3RhdGljIGRhdGEgd2l0aGluIHRoZSBEZXZpY2UgaGFzIGJlZW4gbW9kaWZpZWQALgBEkQcA" +
           "AAAG/////wEB/////wAAAAAXYIkKAgAAAAEAGwAAAExpc3RPZkNvbW11bmljYXRpb25Qcm9maWxlcwEB" +
           "oDoALgBEoDoAAAAMAQAAAAEAAAAAAAAAAQH/////AAAAABVgiQoCAAAAAQASAAAAUHJvdG9jb2xJZGVu" +
           "dGlmaWVyAQHDBwAuAETDBwAAAAz/////AQH/////AAAAAARggAoBAAAAAQAPAAAAU2VydmljZVByb3Zp" +
           "ZGVyAQHHBwAvAQFVCMcHAAD/////CQAAACRggAoBAAAAAgAJAAAATWV0aG9kU2V0AQHKBwMAAAAAFAAA" +
           "AEZsYXQgbGlzdCBvZiBNZXRob2RzAC8AOsoHAAD/////AwAAAARhggoEAAAAAQAKAAAARGlzY29ubmVj" +
           "dAEB7gcALwEBLAHuBwAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAe8HAC4A" +
           "RO8HAACWAQAAAAEAKgEBJgAAABcAAABDb21tdW5pY2F0aW9uUmVsYXRpb25JZAAP/////wAAAAAAAQAo" +
           "AQEAAAABAAAAAAAAAAEB/////wAAAAAXYKkKAgAAAAAADwAAAE91dHB1dEFyZ3VtZW50cwEB8AcALgBE" +
           "8AcAAJYBAAAAAQAqAQEbAAAADAAAAFNlcnZpY2VFcnJvcgAb/////wAAAAAAAQAoAQEAAAABAAAAAAAA" +
           "AAEB/////wAAAAAEYYIKBAAAAAEABwAAAENvbm5lY3QBAfEHAC8BAZgI8QcAAAEB/////wIAAAAXYKkK" +
           "AgAAAAAADgAAAElucHV0QXJndW1lbnRzAQHyBwAuAETyBwAAlgIAAAABACoBASYAAAAXAAAAQ29tbXVu" +
           "aWNhdGlvblJlbGF0aW9uSWQAD/////8AAAAAAAEAKgEBFgAAAAcAAABBZGRyZXNzAA//////AAAAAAAB" +
           "ACgBAQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQHzBwAu" +
           "AETzBwAAlgEAAAABACoBARsAAAAMAAAAU2VydmljZUVycm9yABv/////AAAAAAABACgBAQAAAAEAAAAA" +
           "AAAAAQH/////AAAAAARhggoEAAAAAQAIAAAAVHJhbnNmZXIBAfQHAC8BAZsI9AcAAAEB/////wIAAAAX" +
           "YKkKAgAAAAAADgAAAElucHV0QXJndW1lbnRzAQH1BwAuAET1BwAAlgUAAAABACoBASYAAAAXAAAAQ29t" +
           "bXVuaWNhdGlvblJlbGF0aW9uSWQAD/////8AAAAAAAEAKgEBFQAAAAYAAABIZWFkZXIADP////8AAAAA" +
           "AAEAKgEBGgAAAAsAAABSZXF1ZXN0RGF0YQAP/////wAAAAAAAQAqAQElAAAAEAAAAFJlcXVlc3REYXRh" +
           "VHlwZXMBAQIIAQAAAAEAAAAAAAAAAAEAKgEBJgAAABEAAABSZXNwb25zZURhdGFUeXBlcwEBAggBAAAA" +
           "AQAAAAAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAXYKkKAgAAAAAADwAAAE91dHB1dEFyZ3Vt" +
           "ZW50cwEB9gcALgBE9gcAAJYDAAAAAQAqAQEbAAAADAAAAFJlc3BvbnNlRGF0YQAP/////wAAAAAAAQAq" +
           "AQEdAAAADgAAAFJFU1BPTlNFX0NPREVTAA//////AAAAAAABACoBARsAAAAMAAAAU2VydmljZUVycm9y" +
           "ABv/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAADVgiQoCAAAAAgAMAAAATWFudWZhY3R1" +
           "cmVyAQHeBwMAAAAAMAAAAE5hbWUgb2YgdGhlIGNvbXBhbnkgdGhhdCBtYW51ZmFjdHVyZWQgdGhlIGRl" +
           "dmljZQAuAETeBwAAABX/////AQH/////AAAAADVgiQoCAAAAAgAFAAAATW9kZWwBAd8HAwAAAAAYAAAA" +
           "TW9kZWwgbmFtZSBvZiB0aGUgZGV2aWNlAC4ARN8HAAAAFf////8BAf////8AAAAANWCJCgIAAAACABAA" +
           "AABIYXJkd2FyZVJldmlzaW9uAQHjBwMAAAAALAAAAFJldmlzaW9uIGxldmVsIG9mIHRoZSBoYXJkd2Fy" +
           "ZSBvZiB0aGUgZGV2aWNlAC4AROMHAAAADP////8BAf////8AAAAANWCJCgIAAAACABAAAABTb2Z0d2Fy" +
           "ZVJldmlzaW9uAQHiBwMAAAAANQAAAFJldmlzaW9uIGxldmVsIG9mIHRoZSBzb2Z0d2FyZS9maXJtd2Fy" +
           "ZSBvZiB0aGUgZGV2aWNlAC4AROIHAAAADP////8BAf////8AAAAANWCJCgIAAAACAA4AAABEZXZpY2VS" +
           "ZXZpc2lvbgEB4QcDAAAAACQAAABPdmVyYWxsIHJldmlzaW9uIGxldmVsIG9mIHRoZSBkZXZpY2UALgBE" +
           "4QcAAAAM/////wEB/////wAAAAA1YIkKAgAAAAIADAAAAERldmljZU1hbnVhbAEB4AcDAAAAAFoAAABB" +
           "ZGRyZXNzIChwYXRobmFtZSBpbiB0aGUgZmlsZSBzeXN0ZW0gb3IgYSBVUkwgfCBXZWIgYWRkcmVzcykg" +
           "b2YgdXNlciBtYW51YWwgZm9yIHRoZSBkZXZpY2UALgBE4AcAAAAM/////wEB/////wAAAAA1YIkKAgAA" +
           "AAIADAAAAFNlcmlhbE51bWJlcgEB3AcDAAAAAE0AAABJZGVudGlmaWVyIHRoYXQgdW5pcXVlbHkgaWRl" +
           "bnRpZmllcywgd2l0aGluIGEgbWFudWZhY3R1cmVyLCBhIGRldmljZSBpbnN0YW5jZQAuAETcBwAAAAz/" +
           "////AQH/////AAAAADVgiQoCAAAAAgAPAAAAUmV2aXNpb25Db3VudGVyAQHdBwMAAAAAaQAAAEFuIGlu" +
           "Y3JlbWVudGFsIGNvdW50ZXIgaW5kaWNhdGluZyB0aGUgbnVtYmVyIG9mIHRpbWVzIHRoZSBzdGF0aWMg" +
           "ZGF0YSB3aXRoaW4gdGhlIERldmljZSBoYXMgYmVlbiBtb2RpZmllZAAuAETdBwAAAAb/////AQH/////" +
           "AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public PropertyState<string> ProtocolIdentifier
        {
            get
            {
                return m_protocolIdentifier;
            }

            set
            {
                if (!Object.ReferenceEquals(m_protocolIdentifier, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_protocolIdentifier = value;
            }
        }

        /// <remarks />
        public ServerCommunicationGENERICServiceState ServiceProvider
        {
            get
            {
                return m_serviceProvider;
            }

            set
            {
                if (!Object.ReferenceEquals(m_serviceProvider, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_serviceProvider = value;
            }
        }
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Populates a list with the children that belong to the node.
        /// </summary>
        /// <param name="context">The context for the system being accessed.</param>
        /// <param name="children">The list of children to populate.</param>
        public override void GetChildren(
            ISystemContext context,
            IList<BaseInstanceState> children)
        {
            if (m_protocolIdentifier != null)
            {
                children.Add(m_protocolIdentifier);
            }

            if (m_serviceProvider != null)
            {
                children.Add(m_serviceProvider);
            }

            base.GetChildren(context, children);
        }

        /// <summary>
        /// Finds the child with the specified browse name.
        /// </summary>
        protected override BaseInstanceState FindChild(
            ISystemContext context,
            QualifiedName browseName,
            bool createOrReplace,
            BaseInstanceState replacement)
        {
            if (QualifiedName.IsNull(browseName))
            {
                return null;
            }

            BaseInstanceState instance = null;

            switch (browseName.Name)
            {
                case Opc.Ua.Fdi7.BrowseNames.ProtocolIdentifier:
                {
                    if (createOrReplace)
                    {
                        if (ProtocolIdentifier == null)
                        {
                            if (replacement == null)
                            {
                                ProtocolIdentifier = new PropertyState<string>(this);
                            }
                            else
                            {
                                ProtocolIdentifier = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = ProtocolIdentifier;
                    break;
                }

                case Opc.Ua.Fdi7.BrowseNames.ServiceProvider:
                {
                    if (createOrReplace)
                    {
                        if (ServiceProvider == null)
                        {
                            if (replacement == null)
                            {
                                ServiceProvider = new ServerCommunicationGENERICServiceState(this);
                            }
                            else
                            {
                                ServiceProvider = (ServerCommunicationGENERICServiceState)replacement;
                            }
                        }
                    }

                    instance = ServiceProvider;
                    break;
                }
            }

            if (instance != null)
            {
                return instance;
            }

            return base.FindChild(context, browseName, createOrReplace, replacement);
        }
        #endregion

        #region Private Fields
        private PropertyState<string> m_protocolIdentifier;
        private ServerCommunicationGENERICServiceState m_serviceProvider;
        #endregion
    }
    #endif
    #endregion

    #region ConnectMethodFFH1MethodState Class
    #if (!OPCUA_EXCLUDE_ConnectMethodFFH1MethodState)
    /// <summary>
    /// Stores an instance of the ConnectMethodFFH1Type Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ConnectMethodFFH1MethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ConnectMethodFFH1MethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new ConnectMethodFFH1MethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAVAAAAQ29ubmVjdE1ldGhvZEZGSDFUeXBl" +
           "AQG+AwAvAQG+A74DAAABAf////8CAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBvwMALgBE" +
           "vwMAAJYGAAAAAQAqAQEmAAAAFwAAAENvbW11bmljYXRpb25SZWxhdGlvbklkAA//////AAAAAAABACoB" +
           "ARUAAAAGAAAATGlua0lkAAX/////AAAAAAABACoBARYAAAAHAAAAQWRkcmVzcwAD/////wAAAAAAAQAq" +
           "AQEcAAAADQAAAE9yZGluYWxOdW1iZXIABv////8AAAAAAAEAKgEBHAAAAA0AAABTSUZDb25uZWN0aW9u" +
           "AAH/////AAAAAAABACoBARgAAAAJAAAAU2VydmljZUlkAAf/////AAAAAAABACgBAQAAAAEAAAAAAAAA" +
           "AQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQHAAwAuAETAAwAAlgIAAAABACoB" +
           "AR8AAAAQAAAARGVsYXlGb3JOZXh0Q2FsbAAH/////wAAAAAAAQAqAQEbAAAADAAAAFNlcnZpY2VFcnJv" +
           "cgAb/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public ConnectMethodFFH1MethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            byte[] communicationRelationId = (byte[])_inputArguments[0];
            ushort linkId = (ushort)_inputArguments[1];
            byte address = (byte)_inputArguments[2];
            int ordinalNumber = (int)_inputArguments[3];
            bool sIFConnection = (bool)_inputArguments[4];
            uint serviceId = (uint)_inputArguments[5];

            uint delayForNextCall = (uint)_outputArguments[0];
            object serviceError = (object)_outputArguments[1];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    communicationRelationId,
                    linkId,
                    address,
                    ordinalNumber,
                    sIFConnection,
                    serviceId,
                    ref delayForNextCall,
                    ref serviceError);
            }

            _outputArguments[0] = delayForNextCall;
            _outputArguments[1] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult ConnectMethodFFH1MethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        byte[] communicationRelationId,
        ushort linkId,
        byte address,
        int ordinalNumber,
        bool sIFConnection,
        uint serviceId,
        ref uint delayForNextCall,
        ref object serviceError);
    #endif
    #endregion

    #region ConnectMethodFFHSEMethodState Class
    #if (!OPCUA_EXCLUDE_ConnectMethodFFHSEMethodState)
    /// <summary>
    /// Stores an instance of the ConnectMethodFFHSEType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ConnectMethodFFHSEMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ConnectMethodFFHSEMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new ConnectMethodFFHSEMethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAWAAAAQ29ubmVjdE1ldGhvZEZGSFNFVHlw" +
           "ZQEBwQMALwEBwQPBAwAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAcIDAC4A" +
           "RMIDAACWBAAAAAEAKgEBJgAAABcAAABDb21tdW5pY2F0aW9uUmVsYXRpb25JZAAP/////wAAAAAAAQAq" +
           "AQEWAAAABwAAAEFkZHJlc3MAD/////8AAAAAAAEAKgEBHAAAAA0AAABPcmRpbmFsTnVtYmVyAAb/////" +
           "AAAAAAABACoBARgAAAAJAAAAU2VydmljZUlkAAf/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////" +
           "AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQHDAwAuAETDAwAAlgIAAAABACoBAR8AAAAQ" +
           "AAAARGVsYXlGb3JOZXh0Q2FsbAAH/////wAAAAAAAQAqAQEbAAAADAAAAFNlcnZpY2VFcnJvcgAb////" +
           "/wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public ConnectMethodFFHSEMethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            byte[] communicationRelationId = (byte[])_inputArguments[0];
            byte[] address = (byte[])_inputArguments[1];
            int ordinalNumber = (int)_inputArguments[2];
            uint serviceId = (uint)_inputArguments[3];

            uint delayForNextCall = (uint)_outputArguments[0];
            object serviceError = (object)_outputArguments[1];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    communicationRelationId,
                    address,
                    ordinalNumber,
                    serviceId,
                    ref delayForNextCall,
                    ref serviceError);
            }

            _outputArguments[0] = delayForNextCall;
            _outputArguments[1] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult ConnectMethodFFHSEMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        byte[] communicationRelationId,
        byte[] address,
        int ordinalNumber,
        uint serviceId,
        ref uint delayForNextCall,
        ref object serviceError);
    #endif
    #endregion

    #region ConnectMethodPROFIBUSMethodState Class
    #if (!OPCUA_EXCLUDE_ConnectMethodPROFIBUSMethodState)
    /// <summary>
    /// Stores an instance of the ConnectMethodPROFIBUSType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ConnectMethodPROFIBUSMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ConnectMethodPROFIBUSMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new ConnectMethodPROFIBUSMethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAZAAAAQ29ubmVjdE1ldGhvZFBST0ZJQlVT" +
           "VHlwZQEBxAMALwEBxAPEAwAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAcUD" +
           "AC4ARMUDAACWAwAAAAEAKgEBJgAAABcAAABDb21tdW5pY2F0aW9uUmVsYXRpb25JZAAP/////wAAAAAA" +
           "AQAqAQEWAAAABwAAAEFkZHJlc3MAA/////8AAAAAAAEAKgEBHQAAAA4AAABNYW51ZmFjdHVyZXJJZAAF" +
           "/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAXYKkKAgAAAAAADwAAAE91dHB1dEFyZ3Vt" +
           "ZW50cwEBxgMALgBExgMAAJYBAAAAAQAqAQEbAAAADAAAAFNlcnZpY2VFcnJvcgAb/////wAAAAAAAQAo" +
           "AQEAAAABAAAAAAAAAAEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public ConnectMethodPROFIBUSMethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            byte[] communicationRelationId = (byte[])_inputArguments[0];
            byte address = (byte)_inputArguments[1];
            ushort manufacturerId = (ushort)_inputArguments[2];

            object serviceError = (object)_outputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    communicationRelationId,
                    address,
                    manufacturerId,
                    ref serviceError);
            }

            _outputArguments[0] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult ConnectMethodPROFIBUSMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        byte[] communicationRelationId,
        byte address,
        ushort manufacturerId,
        ref object serviceError);
    #endif
    #endregion

    #region ConnectMethodPROFINETMethodState Class
    #if (!OPCUA_EXCLUDE_ConnectMethodPROFINETMethodState)
    /// <summary>
    /// Stores an instance of the ConnectMethodPROFINETType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ConnectMethodPROFINETMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ConnectMethodPROFINETMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new ConnectMethodPROFINETMethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAZAAAAQ29ubmVjdE1ldGhvZFBST0ZJTkVU" +
           "VHlwZQEBxwMALwEBxwPHAwAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAcgD" +
           "AC4ARMgDAACWBAAAAAEAKgEBJgAAABcAAABDb21tdW5pY2F0aW9uUmVsYXRpb25JZAAP/////wAAAAAA" +
           "AQAqAQEWAAAABwAAAEROU05BTUUADP////8AAAAAAAEAKgEBFwAAAAgAAABEZXZpY2VJRAAF/////wAA" +
           "AAAAAQAqAQEXAAAACAAAAFZlbmRvcklEAAX/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAA" +
           "ABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQHJAwAuAETJAwAAlgEAAAABACoBARsAAAAMAAAA" +
           "U2VydmljZUVycm9yABv/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public ConnectMethodPROFINETMethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            byte[] communicationRelationId = (byte[])_inputArguments[0];
            string dNSNAME = (string)_inputArguments[1];
            ushort deviceID = (ushort)_inputArguments[2];
            ushort vendorID = (ushort)_inputArguments[3];

            object serviceError = (object)_outputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    communicationRelationId,
                    dNSNAME,
                    deviceID,
                    vendorID,
                    ref serviceError);
            }

            _outputArguments[0] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult ConnectMethodPROFINETMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        byte[] communicationRelationId,
        string dNSNAME,
        ushort deviceID,
        ushort vendorID,
        ref object serviceError);
    #endif
    #endregion

    #region ConnectMethodHARTMethodState Class
    #if (!OPCUA_EXCLUDE_ConnectMethodHARTMethodState)
    /// <summary>
    /// Stores an instance of the ConnectMethodHARTType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ConnectMethodHARTMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ConnectMethodHARTMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new ConnectMethodHARTMethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAVAAAAQ29ubmVjdE1ldGhvZEhBUlRUeXBl" +
           "AQHKAwAvAQHKA8oDAAABAf////8CAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBywMALgBE" +
           "ywMAAJYCAAAAAQAqAQEmAAAAFwAAAENvbW11bmljYXRpb25SZWxhdGlvbklkAA//////AAAAAAABACoB" +
           "ARoAAAALAAAATG9uZ0FkZHJlc3MAD/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAAF2Cp" +
           "CgIAAAAAAA8AAABPdXRwdXRBcmd1bWVudHMBAcwDAC4ARMwDAACWAQAAAAEAKgEBGwAAAAwAAABTZXJ2" +
           "aWNlRXJyb3IAG/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public ConnectMethodHARTMethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            byte[] communicationRelationId = (byte[])_inputArguments[0];
            byte[] longAddress = (byte[])_inputArguments[1];

            object serviceError = (object)_outputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    communicationRelationId,
                    longAddress,
                    ref serviceError);
            }

            _outputArguments[0] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult ConnectMethodHARTMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        byte[] communicationRelationId,
        byte[] longAddress,
        ref object serviceError);
    #endif
    #endregion

    #region ConnectMethodISA100_WirelessMethodState Class
    #if (!OPCUA_EXCLUDE_ConnectMethodISA100_WirelessMethodState)
    /// <summary>
    /// Stores an instance of the ConnectMethodISA100_WirelessType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ConnectMethodISA100_WirelessMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ConnectMethodISA100_WirelessMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new ConnectMethodISA100_WirelessMethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAgAAAAQ29ubmVjdE1ldGhvZElTQTEwMF9X" +
           "aXJlbGVzc1R5cGUBAfcHAC8BAfcH9wcAAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElucHV0QXJndW1l" +
           "bnRzAQH4BwAuAET4BwAAlgMAAAABACoBASYAAAAXAAAAQ29tbXVuaWNhdGlvblJlbGF0aW9uSWQAD///" +
           "//8AAAAAAAEAKgEBGAAAAAkAAABJUEFkZHJlc3MAD/////8AAAAAAAEAKgEBGgAAAAsAAABDb25uZWN0" +
           "VHlwZQAH/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAXYKkKAgAAAAAADwAAAE91dHB1" +
           "dEFyZ3VtZW50cwEB+QcALgBE+QcAAJYBAAAAAQAqAQEbAAAADAAAAFNlcnZpY2VFcnJvcgAb/////wAA" +
           "AAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public ConnectMethodISA100_WirelessMethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            byte[] communicationRelationId = (byte[])_inputArguments[0];
            byte[] iPAddress = (byte[])_inputArguments[1];
            uint connectType = (uint)_inputArguments[2];

            object serviceError = (object)_outputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    communicationRelationId,
                    iPAddress,
                    connectType,
                    ref serviceError);
            }

            _outputArguments[0] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult ConnectMethodISA100_WirelessMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        byte[] communicationRelationId,
        byte[] iPAddress,
        uint connectType,
        ref object serviceError);
    #endif
    #endregion

    #region ConnectMethodGENERICMethodState Class
    #if (!OPCUA_EXCLUDE_ConnectMethodGENERICMethodState)
    /// <summary>
    /// Stores an instance of the ConnectMethodGENERICType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ConnectMethodGENERICMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ConnectMethodGENERICMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new ConnectMethodGENERICMethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAYAAAAQ29ubmVjdE1ldGhvZEdFTkVSSUNU" +
           "eXBlAQH6BwAvAQH6B/oHAAABAf////8CAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEB+wcA" +
           "LgBE+wcAAJYCAAAAAQAqAQEmAAAAFwAAAENvbW11bmljYXRpb25SZWxhdGlvbklkAA//////AAAAAAAB" +
           "ACoBARYAAAAHAAAAQWRkcmVzcwAP/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAXYKkK" +
           "AgAAAAAADwAAAE91dHB1dEFyZ3VtZW50cwEB/AcALgBE/AcAAJYBAAAAAQAqAQEbAAAADAAAAFNlcnZp" +
           "Y2VFcnJvcgAb/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public ConnectMethodGENERICMethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            byte[] communicationRelationId = (byte[])_inputArguments[0];
            byte[] address = (byte[])_inputArguments[1];

            object serviceError = (object)_outputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    communicationRelationId,
                    address,
                    ref serviceError);
            }

            _outputArguments[0] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult ConnectMethodGENERICMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        byte[] communicationRelationId,
        byte[] address,
        ref object serviceError);
    #endif
    #endregion

    #region DisconnectMethodState Class
    #if (!OPCUA_EXCLUDE_DisconnectMethodState)
    /// <summary>
    /// Stores an instance of the DisconnectMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class DisconnectMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public DisconnectMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new DisconnectMethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAUAAAARGlzY29ubmVjdE1ldGhvZFR5cGUB" +
           "AeAAAC8BAeAA4AAAAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElucHV0QXJndW1lbnRzAQHhAAAuAETh" +
           "AAAAlgEAAAABACoBASYAAAAXAAAAQ29tbXVuaWNhdGlvblJlbGF0aW9uSWQAD/////8AAAAAAAEAKAEB" +
           "AAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1bWVudHMBAeIAAC4AROIA" +
           "AACWAQAAAAEAKgEBGwAAAAwAAABTZXJ2aWNlRXJyb3IAG/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAAB" +
           "Af////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public DisconnectMethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            byte[] communicationRelationId = (byte[])_inputArguments[0];

            object serviceError = (object)_outputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    communicationRelationId,
                    ref serviceError);
            }

            _outputArguments[0] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult DisconnectMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        byte[] communicationRelationId,
        ref object serviceError);
    #endif
    #endregion

    #region TransferMethodFFH1MethodState Class
    #if (!OPCUA_EXCLUDE_TransferMethodFFH1MethodState)
    /// <summary>
    /// Stores an instance of the TransferMethodFFH1Type Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class TransferMethodFFH1MethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public TransferMethodFFH1MethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new TransferMethodFFH1MethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAWAAAAVHJhbnNmZXJNZXRob2RGRkgxVHlw" +
           "ZQEBzQMALwEBzQPNAwAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAc4DAC4A" +
           "RM4DAACWBwAAAAEAKgEBJgAAABcAAABDb21tdW5pY2F0aW9uUmVsYXRpb25JZAAP/////wAAAAAAAQAq" +
           "AQEYAAAACQAAAE9QRVJBVElPTgAM/////wAAAAAAAQAqAQEXAAAACAAAAEJsb2NrVGFnAAz/////AAAA" +
           "AAABACoBARQAAAAFAAAASU5ERVgAB/////8AAAAAAAEAKgEBGAAAAAkAAABTVUJfSU5ERVgAB/////8A" +
           "AAAAAAEAKgEBHAAAAAkAAABXcml0ZURhdGEAAwEAAAABAAAAAAAAAAABACoBARgAAAAJAAAAU2Vydmlj" +
           "ZUlkAAf/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0" +
           "QXJndW1lbnRzAQHPAwAuAETPAwAAlgMAAAABACoBARsAAAAIAAAAUmVhZERhdGEAAwEAAAABAAAAAAAA" +
           "AAABACoBAR8AAAAQAAAARGVsYXlGb3JOZXh0Q2FsbAAH/////wAAAAAAAQAqAQEbAAAADAAAAFNlcnZp" +
           "Y2VFcnJvcgAb/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public TransferMethodFFH1MethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            byte[] communicationRelationId = (byte[])_inputArguments[0];
            string oPERATION = (string)_inputArguments[1];
            string blockTag = (string)_inputArguments[2];
            uint iNDEX = (uint)_inputArguments[3];
            uint sUB_INDEX = (uint)_inputArguments[4];
            byte[] writeData = (byte[])_inputArguments[5];
            uint serviceId = (uint)_inputArguments[6];

            byte[] readData = (byte[])_outputArguments[0];
            uint delayForNextCall = (uint)_outputArguments[1];
            object serviceError = (object)_outputArguments[2];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    communicationRelationId,
                    oPERATION,
                    blockTag,
                    iNDEX,
                    sUB_INDEX,
                    writeData,
                    serviceId,
                    ref readData,
                    ref delayForNextCall,
                    ref serviceError);
            }

            _outputArguments[0] = readData;
            _outputArguments[1] = delayForNextCall;
            _outputArguments[2] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult TransferMethodFFH1MethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        byte[] communicationRelationId,
        string oPERATION,
        string blockTag,
        uint iNDEX,
        uint sUB_INDEX,
        byte[] writeData,
        uint serviceId,
        ref byte[] readData,
        ref uint delayForNextCall,
        ref object serviceError);
    #endif
    #endregion

    #region TransferMethodFFHSEMethodState Class
    #if (!OPCUA_EXCLUDE_TransferMethodFFHSEMethodState)
    /// <summary>
    /// Stores an instance of the TransferMethodFFHSEType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class TransferMethodFFHSEMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public TransferMethodFFHSEMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new TransferMethodFFHSEMethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAXAAAAVHJhbnNmZXJNZXRob2RGRkhTRVR5" +
           "cGUBAdADAC8BAdAD0AMAAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElucHV0QXJndW1lbnRzAQHRAwAu" +
           "AETRAwAAlgcAAAABACoBASYAAAAXAAAAQ29tbXVuaWNhdGlvblJlbGF0aW9uSWQAD/////8AAAAAAAEA" +
           "KgEBGAAAAAkAAABPUEVSQVRJT04ADP////8AAAAAAAEAKgEBFwAAAAgAAABCbG9ja1RhZwAM/////wAA" +
           "AAAAAQAqAQEUAAAABQAAAElOREVYAAf/////AAAAAAABACoBARgAAAAJAAAAU1VCX0lOREVYAAf/////" +
           "AAAAAAABACoBARwAAAAJAAAAV3JpdGVEYXRhAAMBAAAAAQAAAAAAAAAAAQAqAQEYAAAACQAAAFNlcnZp" +
           "Y2VJZAAH/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAXYKkKAgAAAAAADwAAAE91dHB1" +
           "dEFyZ3VtZW50cwEB0gMALgBE0gMAAJYDAAAAAQAqAQEbAAAACAAAAFJlYWREYXRhAAMBAAAAAQAAAAAA" +
           "AAAAAQAqAQEfAAAAEAAAAERlbGF5Rm9yTmV4dENhbGwAB/////8AAAAAAAEAKgEBGwAAAAwAAABTZXJ2" +
           "aWNlRXJyb3IAG/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public TransferMethodFFHSEMethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            byte[] communicationRelationId = (byte[])_inputArguments[0];
            string oPERATION = (string)_inputArguments[1];
            string blockTag = (string)_inputArguments[2];
            uint iNDEX = (uint)_inputArguments[3];
            uint sUB_INDEX = (uint)_inputArguments[4];
            byte[] writeData = (byte[])_inputArguments[5];
            uint serviceId = (uint)_inputArguments[6];

            byte[] readData = (byte[])_outputArguments[0];
            uint delayForNextCall = (uint)_outputArguments[1];
            object serviceError = (object)_outputArguments[2];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    communicationRelationId,
                    oPERATION,
                    blockTag,
                    iNDEX,
                    sUB_INDEX,
                    writeData,
                    serviceId,
                    ref readData,
                    ref delayForNextCall,
                    ref serviceError);
            }

            _outputArguments[0] = readData;
            _outputArguments[1] = delayForNextCall;
            _outputArguments[2] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult TransferMethodFFHSEMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        byte[] communicationRelationId,
        string oPERATION,
        string blockTag,
        uint iNDEX,
        uint sUB_INDEX,
        byte[] writeData,
        uint serviceId,
        ref byte[] readData,
        ref uint delayForNextCall,
        ref object serviceError);
    #endif
    #endregion

    #region TransferMethodPROFIBUSMethodState Class
    #if (!OPCUA_EXCLUDE_TransferMethodPROFIBUSMethodState)
    /// <summary>
    /// Stores an instance of the TransferMethodPROFIBUSType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class TransferMethodPROFIBUSMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public TransferMethodPROFIBUSMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new TransferMethodPROFIBUSMethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAaAAAAVHJhbnNmZXJNZXRob2RQUk9GSUJV" +
           "U1R5cGUBAdMDAC8BAdMD0wMAAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElucHV0QXJndW1lbnRzAQHU" +
           "AwAuAETUAwAAlgUAAAABACoBASYAAAAXAAAAQ29tbXVuaWNhdGlvblJlbGF0aW9uSWQAD/////8AAAAA" +
           "AAEAKgEBGAAAAAkAAABPUEVSQVRJT04ADP////8AAAAAAAEAKgEBEwAAAAQAAABTTE9UAAP/////AAAA" +
           "AAABACoBARQAAAAFAAAASU5ERVgAA/////8AAAAAAAEAKgEBFgAAAAcAAABSRVFVRVNUAA//////AAAA" +
           "AAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQHV" +
           "AwAuAETVAwAAlgMAAAABACoBARQAAAAFAAAAUkVQTFkAD/////8AAAAAAAEAKgEBHQAAAA4AAABSRVNQ" +
           "T05TRV9DT0RFUwAP/////wAAAAAAAQAqAQEbAAAADAAAAFNlcnZpY2VFcnJvcgAb/////wAAAAAAAQAo" +
           "AQEAAAABAAAAAAAAAAEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public TransferMethodPROFIBUSMethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            byte[] communicationRelationId = (byte[])_inputArguments[0];
            string oPERATION = (string)_inputArguments[1];
            byte sLOT = (byte)_inputArguments[2];
            byte iNDEX = (byte)_inputArguments[3];
            byte[] rEQUEST = (byte[])_inputArguments[4];

            byte[] rEPLY = (byte[])_outputArguments[0];
            byte[] rESPONSE_CODES = (byte[])_outputArguments[1];
            object serviceError = (object)_outputArguments[2];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    communicationRelationId,
                    oPERATION,
                    sLOT,
                    iNDEX,
                    rEQUEST,
                    ref rEPLY,
                    ref rESPONSE_CODES,
                    ref serviceError);
            }

            _outputArguments[0] = rEPLY;
            _outputArguments[1] = rESPONSE_CODES;
            _outputArguments[2] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult TransferMethodPROFIBUSMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        byte[] communicationRelationId,
        string oPERATION,
        byte sLOT,
        byte iNDEX,
        byte[] rEQUEST,
        ref byte[] rEPLY,
        ref byte[] rESPONSE_CODES,
        ref object serviceError);
    #endif
    #endregion

    #region TransferMethodPROFINETMethodState Class
    #if (!OPCUA_EXCLUDE_TransferMethodPROFINETMethodState)
    /// <summary>
    /// Stores an instance of the TransferMethodPROFINETType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class TransferMethodPROFINETMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public TransferMethodPROFINETMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new TransferMethodPROFINETMethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAaAAAAVHJhbnNmZXJNZXRob2RQUk9GSU5F" +
           "VFR5cGUBAdYDAC8BAdYD1gMAAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElucHV0QXJndW1lbnRzAQHX" +
           "AwAuAETXAwAAlgcAAAABACoBASYAAAAXAAAAQ29tbXVuaWNhdGlvblJlbGF0aW9uSWQAD/////8AAAAA" +
           "AAEAKgEBGAAAAAkAAABPUEVSQVRJT04ADP////8AAAAAAAEAKgEBEwAAAAQAAABTTE9UAAX/////AAAA" +
           "AAABACoBARYAAAAHAAAAU1VCU0xPVAAF/////wAAAAAAAQAqAQEUAAAABQAAAElOREVYAAX/////AAAA" +
           "AAABACoBARIAAAADAAAAQVBJAAf/////AAAAAAABACoBARYAAAAHAAAAUkVRVUVTVAAP/////wAAAAAA" +
           "AQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAXYKkKAgAAAAAADwAAAE91dHB1dEFyZ3VtZW50cwEB2AMA" +
           "LgBE2AMAAJYDAAAAAQAqAQEUAAAABQAAAFJFUExZAA//////AAAAAAABACoBAR0AAAAOAAAAUkVTUE9O" +
           "U0VfQ09ERVMAD/////8AAAAAAAEAKgEBGwAAAAwAAABTZXJ2aWNlRXJyb3IAG/////8AAAAAAAEAKAEB" +
           "AAAAAQAAAAAAAAABAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public TransferMethodPROFINETMethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            byte[] communicationRelationId = (byte[])_inputArguments[0];
            string oPERATION = (string)_inputArguments[1];
            ushort sLOT = (ushort)_inputArguments[2];
            ushort sUBSLOT = (ushort)_inputArguments[3];
            ushort iNDEX = (ushort)_inputArguments[4];
            uint aPI = (uint)_inputArguments[5];
            byte[] rEQUEST = (byte[])_inputArguments[6];

            byte[] rEPLY = (byte[])_outputArguments[0];
            byte[] rESPONSE_CODES = (byte[])_outputArguments[1];
            object serviceError = (object)_outputArguments[2];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    communicationRelationId,
                    oPERATION,
                    sLOT,
                    sUBSLOT,
                    iNDEX,
                    aPI,
                    rEQUEST,
                    ref rEPLY,
                    ref rESPONSE_CODES,
                    ref serviceError);
            }

            _outputArguments[0] = rEPLY;
            _outputArguments[1] = rESPONSE_CODES;
            _outputArguments[2] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult TransferMethodPROFINETMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        byte[] communicationRelationId,
        string oPERATION,
        ushort sLOT,
        ushort sUBSLOT,
        ushort iNDEX,
        uint aPI,
        byte[] rEQUEST,
        ref byte[] rEPLY,
        ref byte[] rESPONSE_CODES,
        ref object serviceError);
    #endif
    #endregion

    #region TransferMethodHARTMethodState Class
    #if (!OPCUA_EXCLUDE_TransferMethodHARTMethodState)
    /// <summary>
    /// Stores an instance of the TransferMethodHARTType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class TransferMethodHARTMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public TransferMethodHARTMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new TransferMethodHARTMethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAWAAAAVHJhbnNmZXJNZXRob2RIQVJUVHlw" +
           "ZQEB2QMALwEB2QPZAwAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAdoDAC4A" +
           "RNoDAACWAwAAAAEAKgEBJgAAABcAAABDb21tdW5pY2F0aW9uUmVsYXRpb25JZAAP/////wAAAAAAAQAq" +
           "AQEWAAAABwAAAENvbW1hbmQABf////8AAAAAAAEAKgEBFgAAAAcAAABSZXF1ZXN0AA//////AAAAAAAB" +
           "ACgBAQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQHbAwAu" +
           "AETbAwAAlgIAAAABACoBARQAAAAFAAAAUmVwbHkAD/////8AAAAAAAEAKgEBGwAAAAwAAABTZXJ2aWNl" +
           "RXJyb3IAG/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public TransferMethodHARTMethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            byte[] communicationRelationId = (byte[])_inputArguments[0];
            ushort command = (ushort)_inputArguments[1];
            byte[] request = (byte[])_inputArguments[2];

            byte[] reply = (byte[])_outputArguments[0];
            object serviceError = (object)_outputArguments[1];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    communicationRelationId,
                    command,
                    request,
                    ref reply,
                    ref serviceError);
            }

            _outputArguments[0] = reply;
            _outputArguments[1] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult TransferMethodHARTMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        byte[] communicationRelationId,
        ushort command,
        byte[] request,
        ref byte[] reply,
        ref object serviceError);
    #endif
    #endregion

    #region TransferMethodISA100_WirelessMethodState Class
    #if (!OPCUA_EXCLUDE_TransferMethodISA100_WirelessMethodState)
    /// <summary>
    /// Stores an instance of the TransferMethodISA100_WirelessType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class TransferMethodISA100_WirelessMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public TransferMethodISA100_WirelessMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new TransferMethodISA100_WirelessMethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAhAAAAVHJhbnNmZXJNZXRob2RJU0ExMDBf" +
           "V2lyZWxlc3NUeXBlAQH9BwAvAQH9B/0HAAABAf////8CAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3Vt" +
           "ZW50cwEB/gcALgBE/gcAAJYIAAAAAQAqAQEmAAAAFwAAAENvbW11bmljYXRpb25SZWxhdGlvbklkAA//" +
           "////AAAAAAABACoBARgAAAAJAAAAT1BFUkFUSU9OAAz/////AAAAAAABACoBARQAAAAFAAAAQXBwSUQA" +
           "Bf////8AAAAAAAEAKgEBFwAAAAgAAABPYmplY3RJRAAF/////wAAAAAAAQAqAQEbAAAADAAAAEF0dHJP" +
           "ck1ldGhJRAAF/////wAAAAAAAQAqAQEYAAAACQAAAFNVQl9JTkRFWAAH/////wAAAAAAAQAqAQEcAAAA" +
           "CQAAAFdyaXRlRGF0YQADAQAAAAEAAAAAAAAAAAEAKgEBGAAAAAkAAABSZXF1ZXN0SWQAB/////8AAAAA" +
           "AAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1bWVudHMBAf8H" +
           "AC4ARP8HAACWAgAAAAEAKgEBGwAAAAgAAABSZWFkRGF0YQADAQAAAAEAAAAAAAAAAAEAKgEBGwAAAAwA" +
           "AABTZXJ2aWNlRXJyb3IAG/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public TransferMethodISA100_WirelessMethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            byte[] communicationRelationId = (byte[])_inputArguments[0];
            string oPERATION = (string)_inputArguments[1];
            ushort appID = (ushort)_inputArguments[2];
            ushort objectID = (ushort)_inputArguments[3];
            ushort attrOrMethID = (ushort)_inputArguments[4];
            uint sUB_INDEX = (uint)_inputArguments[5];
            byte[] writeData = (byte[])_inputArguments[6];
            uint requestId = (uint)_inputArguments[7];

            byte[] readData = (byte[])_outputArguments[0];
            object serviceError = (object)_outputArguments[1];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    communicationRelationId,
                    oPERATION,
                    appID,
                    objectID,
                    attrOrMethID,
                    sUB_INDEX,
                    writeData,
                    requestId,
                    ref readData,
                    ref serviceError);
            }

            _outputArguments[0] = readData;
            _outputArguments[1] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult TransferMethodISA100_WirelessMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        byte[] communicationRelationId,
        string oPERATION,
        ushort appID,
        ushort objectID,
        ushort attrOrMethID,
        uint sUB_INDEX,
        byte[] writeData,
        uint requestId,
        ref byte[] readData,
        ref object serviceError);
    #endif
    #endregion

    #region TransferMethodGENERICMethodState Class
    #if (!OPCUA_EXCLUDE_TransferMethodGENERICMethodState)
    /// <summary>
    /// Stores an instance of the TransferMethodGENERICType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class TransferMethodGENERICMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public TransferMethodGENERICMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new TransferMethodGENERICMethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAZAAAAVHJhbnNmZXJNZXRob2RHRU5FUklD" +
           "VHlwZQEBAwgALwEBAwgDCAAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAQQI" +
           "AC4ARAQIAACWBQAAAAEAKgEBJgAAABcAAABDb21tdW5pY2F0aW9uUmVsYXRpb25JZAAP/////wAAAAAA" +
           "AQAqAQEVAAAABgAAAEhlYWRlcgAM/////wAAAAAAAQAqAQEaAAAACwAAAFJlcXVlc3REYXRhAA//////" +
           "AAAAAAABACoBASUAAAAQAAAAUmVxdWVzdERhdGFUeXBlcwEBAggBAAAAAQAAAAAAAAAAAQAqAQEmAAAA" +
           "EQAAAFJlc3BvbnNlRGF0YVR5cGVzAQECCAEAAAABAAAAAAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////" +
           "AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQEFCAAuAEQFCAAAlgMAAAABACoBARsAAAAM" +
           "AAAAUmVzcG9uc2VEYXRhAA//////AAAAAAABACoBAR0AAAAOAAAAUkVTUE9OU0VfQ09ERVMAD/////8A" +
           "AAAAAAEAKgEBGwAAAAwAAABTZXJ2aWNlRXJyb3IAG/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf//" +
           "//8AAAAA";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public TransferMethodGENERICMethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            byte[] communicationRelationId = (byte[])_inputArguments[0];
            string header = (string)_inputArguments[1];
            byte[] requestData = (byte[])_inputArguments[2];
            EddDataTypeInfo[] requestDataTypes = (EddDataTypeInfo[])ExtensionObject.ToArray(_inputArguments[3], typeof(EddDataTypeInfo));
            EddDataTypeInfo[] responseDataTypes = (EddDataTypeInfo[])ExtensionObject.ToArray(_inputArguments[4], typeof(EddDataTypeInfo));

            byte[] responseData = (byte[])_outputArguments[0];
            byte[] rESPONSE_CODES = (byte[])_outputArguments[1];
            object serviceError = (object)_outputArguments[2];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    communicationRelationId,
                    header,
                    requestData,
                    requestDataTypes,
                    responseDataTypes,
                    ref responseData,
                    ref rESPONSE_CODES,
                    ref serviceError);
            }

            _outputArguments[0] = responseData;
            _outputArguments[1] = rESPONSE_CODES;
            _outputArguments[2] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult TransferMethodGENERICMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        byte[] communicationRelationId,
        string header,
        byte[] requestData,
        EddDataTypeInfo[] requestDataTypes,
        EddDataTypeInfo[] responseDataTypes,
        ref byte[] responseData,
        ref byte[] rESPONSE_CODES,
        ref object serviceError);
    #endif
    #endregion

    #region GetPublishedDataMethodFFH1MethodState Class
    #if (!OPCUA_EXCLUDE_GetPublishedDataMethodFFH1MethodState)
    /// <summary>
    /// Stores an instance of the GetPublishedDataMethodFFH1Type Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class GetPublishedDataMethodFFH1MethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public GetPublishedDataMethodFFH1MethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new GetPublishedDataMethodFFH1MethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAeAAAAR2V0UHVibGlzaGVkRGF0YU1ldGhv" +
           "ZEZGSDFUeXBlAQHcAwAvAQHcA9wDAAABAf////8CAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50" +
           "cwEB3QMALgBE3QMAAJYCAAAAAQAqAQEmAAAAFwAAAENvbW11bmljYXRpb25SZWxhdGlvbklkAA//////" +
           "AAAAAAABACoBARgAAAAJAAAAU2VydmljZUlkAAf/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////" +
           "AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQHeAwAuAETeAwAAlgYAAAABACoBARcAAAAI" +
           "AAAAQmxvY2tUYWcADP////8AAAAAAAEAKgEBIQAAAA4AAABBbGFybUV2ZW50RGF0YQADAQAAAAEAAAAA" +
           "AAAAAAEAKgEBHQAAAA4AAABBbGFybUV2ZW50VHlwZQAR/////wAAAAAAAQAqAQEYAAAACQAAAFRpbWVT" +
           "dGFtcAAN/////wAAAAAAAQAqAQEfAAAAEAAAAERlbGF5Rm9yTmV4dENhbGwAB/////8AAAAAAAEAKgEB" +
           "GwAAAAwAAABTZXJ2aWNlRXJyb3IAG/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public GetPublishedDataMethodFFH1MethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            byte[] communicationRelationId = (byte[])_inputArguments[0];
            uint serviceId = (uint)_inputArguments[1];

            string blockTag = (string)_outputArguments[0];
            byte[] alarmEventData = (byte[])_outputArguments[1];
            NodeId alarmEventType = (NodeId)_outputArguments[2];
            DateTime timeStamp = (DateTime)_outputArguments[3];
            uint delayForNextCall = (uint)_outputArguments[4];
            object serviceError = (object)_outputArguments[5];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    communicationRelationId,
                    serviceId,
                    ref blockTag,
                    ref alarmEventData,
                    ref alarmEventType,
                    ref timeStamp,
                    ref delayForNextCall,
                    ref serviceError);
            }

            _outputArguments[0] = blockTag;
            _outputArguments[1] = alarmEventData;
            _outputArguments[2] = alarmEventType;
            _outputArguments[3] = timeStamp;
            _outputArguments[4] = delayForNextCall;
            _outputArguments[5] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult GetPublishedDataMethodFFH1MethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        byte[] communicationRelationId,
        uint serviceId,
        ref string blockTag,
        ref byte[] alarmEventData,
        ref NodeId alarmEventType,
        ref DateTime timeStamp,
        ref uint delayForNextCall,
        ref object serviceError);
    #endif
    #endregion

    #region GetPublishedDataMethodFFHSEMethodState Class
    #if (!OPCUA_EXCLUDE_GetPublishedDataMethodFFHSEMethodState)
    /// <summary>
    /// Stores an instance of the GetPublishedDataMethodFFHSEType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class GetPublishedDataMethodFFHSEMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public GetPublishedDataMethodFFHSEMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new GetPublishedDataMethodFFHSEMethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAfAAAAR2V0UHVibGlzaGVkRGF0YU1ldGhv" +
           "ZEZGSFNFVHlwZQEB3wMALwEB3wPfAwAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVu" +
           "dHMBAeADAC4AROADAACWAgAAAAEAKgEBJgAAABcAAABDb21tdW5pY2F0aW9uUmVsYXRpb25JZAAP////" +
           "/wAAAAAAAQAqAQEYAAAACQAAAFNlcnZpY2VJZAAH/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB////" +
           "/wAAAAAXYKkKAgAAAAAADwAAAE91dHB1dEFyZ3VtZW50cwEB4QMALgBE4QMAAJYGAAAAAQAqAQEXAAAA" +
           "CAAAAEJsb2NrVGFnAAz/////AAAAAAABACoBASEAAAAOAAAAQWxhcm1FdmVudERhdGEAAwEAAAABAAAA" +
           "AAAAAAABACoBAR0AAAAOAAAAQWxhcm1FdmVudFR5cGUAEf////8AAAAAAAEAKgEBGAAAAAkAAABUaW1l" +
           "U3RhbXAADf////8AAAAAAAEAKgEBHwAAABAAAABEZWxheUZvck5leHRDYWxsAAf/////AAAAAAABACoB" +
           "ARsAAAAMAAAAU2VydmljZUVycm9yABv/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public GetPublishedDataMethodFFHSEMethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            byte[] communicationRelationId = (byte[])_inputArguments[0];
            uint serviceId = (uint)_inputArguments[1];

            string blockTag = (string)_outputArguments[0];
            byte[] alarmEventData = (byte[])_outputArguments[1];
            NodeId alarmEventType = (NodeId)_outputArguments[2];
            DateTime timeStamp = (DateTime)_outputArguments[3];
            uint delayForNextCall = (uint)_outputArguments[4];
            object serviceError = (object)_outputArguments[5];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    communicationRelationId,
                    serviceId,
                    ref blockTag,
                    ref alarmEventData,
                    ref alarmEventType,
                    ref timeStamp,
                    ref delayForNextCall,
                    ref serviceError);
            }

            _outputArguments[0] = blockTag;
            _outputArguments[1] = alarmEventData;
            _outputArguments[2] = alarmEventType;
            _outputArguments[3] = timeStamp;
            _outputArguments[4] = delayForNextCall;
            _outputArguments[5] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult GetPublishedDataMethodFFHSEMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        byte[] communicationRelationId,
        uint serviceId,
        ref string blockTag,
        ref byte[] alarmEventData,
        ref NodeId alarmEventType,
        ref DateTime timeStamp,
        ref uint delayForNextCall,
        ref object serviceError);
    #endif
    #endregion

    #region GetPublishedDataMethodHARTMethodState Class
    #if (!OPCUA_EXCLUDE_GetPublishedDataMethodHARTMethodState)
    /// <summary>
    /// Stores an instance of the GetPublishedDataMethodHARTType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class GetPublishedDataMethodHARTMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public GetPublishedDataMethodHARTMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new GetPublishedDataMethodHARTMethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAeAAAAR2V0UHVibGlzaGVkRGF0YU1ldGhv" +
           "ZEhBUlRUeXBlAQHiAwAvAQHiA+IDAAABAf////8CAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50" +
           "cwEB4wMALgBE4wMAAJYBAAAAAQAqAQEmAAAAFwAAAENvbW11bmljYXRpb25SZWxhdGlvbklkAA//////" +
           "AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRz" +
           "AQHkAwAuAETkAwAAlgQAAAABACoBARYAAAAHAAAAQ29tbWFuZAAF/////wAAAAAAAQAqAQEUAAAABQAA" +
           "AFJlcGx5AA//////AAAAAAABACoBARgAAAAJAAAAVGltZVN0YW1wAA3/////AAAAAAABACoBARsAAAAM" +
           "AAAAU2VydmljZUVycm9yABv/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public GetPublishedDataMethodHARTMethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            byte[] communicationRelationId = (byte[])_inputArguments[0];

            ushort command = (ushort)_outputArguments[0];
            byte[] reply = (byte[])_outputArguments[1];
            DateTime timeStamp = (DateTime)_outputArguments[2];
            object serviceError = (object)_outputArguments[3];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    communicationRelationId,
                    ref command,
                    ref reply,
                    ref timeStamp,
                    ref serviceError);
            }

            _outputArguments[0] = command;
            _outputArguments[1] = reply;
            _outputArguments[2] = timeStamp;
            _outputArguments[3] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult GetPublishedDataMethodHARTMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        byte[] communicationRelationId,
        ref ushort command,
        ref byte[] reply,
        ref DateTime timeStamp,
        ref object serviceError);
    #endif
    #endregion

    #region GetPublishedDataMethodISA100_WirelessMethodState Class
    #if (!OPCUA_EXCLUDE_GetPublishedDataMethodISA100_WirelessMethodState)
    /// <summary>
    /// Stores an instance of the GetPublishedDataMethodISA100_WirelessType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class GetPublishedDataMethodISA100_WirelessMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public GetPublishedDataMethodISA100_WirelessMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new GetPublishedDataMethodISA100_WirelessMethodState(parent);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQApAAAAR2V0UHVibGlzaGVkRGF0YU1ldGhv" +
           "ZElTQTEwMF9XaXJlbGVzc1R5cGUBAQYIAC8BAQYIBggAAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElu" +
           "cHV0QXJndW1lbnRzAQEHCAAuAEQHCAAAlgEAAAABACoBASYAAAAXAAAAQ29tbXVuaWNhdGlvblJlbGF0" +
           "aW9uSWQAD/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIAAAAAAA8AAABPdXRw" +
           "dXRBcmd1bWVudHMBAQgIAC4ARAgIAACWBgAAAAEAKgEBFAAAAAUAAABBcHBJRAAF/////wAAAAAAAQAq" +
           "AQEXAAAACAAAAE9iamVjdElEAAX/////AAAAAAABACoBASEAAAAOAAAAQWxhcm1FdmVudERhdGEAAwEA" +
           "AAABAAAAAAAAAAABACoBAR0AAAAOAAAAQWxhcm1FdmVudFR5cGUABf////8AAAAAAAEAKgEBGAAAAAkA" +
           "AABUaW1lU3RhbXAADf////8AAAAAAAEAKgEBGwAAAAwAAABTZXJ2aWNlRXJyb3IAG/////8AAAAAAAEA" +
           "KAEBAAAAAQAAAAAAAAABAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public GetPublishedDataMethodISA100_WirelessMethodStateMethodCallHandler OnCall;
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        /// <summary>
        /// Invokes the method, returns the result and output argument.
        /// </summary>
        protected override ServiceResult Call(
            ISystemContext _context,
            NodeId _objectId,
            IList<object> _inputArguments,
            IList<object> _outputArguments)
        {
            if (OnCall == null)
            {
                return base.Call(_context, _objectId, _inputArguments, _outputArguments);
            }

            ServiceResult result = null;

            byte[] communicationRelationId = (byte[])_inputArguments[0];

            ushort appID = (ushort)_outputArguments[0];
            ushort objectID = (ushort)_outputArguments[1];
            byte[] alarmEventData = (byte[])_outputArguments[2];
            ushort alarmEventType = (ushort)_outputArguments[3];
            DateTime timeStamp = (DateTime)_outputArguments[4];
            object serviceError = (object)_outputArguments[5];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    communicationRelationId,
                    ref appID,
                    ref objectID,
                    ref alarmEventData,
                    ref alarmEventType,
                    ref timeStamp,
                    ref serviceError);
            }

            _outputArguments[0] = appID;
            _outputArguments[1] = objectID;
            _outputArguments[2] = alarmEventData;
            _outputArguments[3] = alarmEventType;
            _outputArguments[4] = timeStamp;
            _outputArguments[5] = serviceError;

            return result;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    /// <summary>
    /// Used to receive notifications when the method is called.
    /// </summary>
    /// <exclude />
    public delegate ServiceResult GetPublishedDataMethodISA100_WirelessMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        byte[] communicationRelationId,
        ref ushort appID,
        ref ushort objectID,
        ref byte[] alarmEventData,
        ref ushort alarmEventType,
        ref DateTime timeStamp,
        ref object serviceError);
    #endif
    #endregion

    #region ServerCommunicationServiceState Class
    #if (!OPCUA_EXCLUDE_ServerCommunicationServiceState)
    /// <summary>
    /// Stores an instance of the ServerCommunicationServiceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ServerCommunicationServiceState : DeviceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ServerCommunicationServiceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationServiceType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAmAAAAU2VydmVyQ29tbXVuaWNhdGlvblNl" +
           "cnZpY2VUeXBlSW5zdGFuY2UBAekAAQHpAOkAAAD/////CQAAACRggAoBAAAAAgAJAAAATWV0aG9kU2V0" +
           "AQHsAAMAAAAAFAAAAEZsYXQgbGlzdCBvZiBNZXRob2RzAC8AOuwAAAD/////AQAAAARhggoEAAAAAQAK" +
           "AAAARGlzY29ubmVjdAEBLAEALwEBLAEsAQAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1" +
           "bWVudHMBAS0BAC4ARC0BAACWAQAAAAEAKgEBJgAAABcAAABDb21tdW5pY2F0aW9uUmVsYXRpb25JZAAP" +
           "/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAXYKkKAgAAAAAADwAAAE91dHB1dEFyZ3Vt" +
           "ZW50cwEBLgEALgBELgEAAJYBAAAAAQAqAQEbAAAADAAAAFNlcnZpY2VFcnJvcgAb/////wAAAAAAAQAo" +
           "AQEAAAABAAAAAAAAAAEB/////wAAAAA1YIkKAgAAAAIADAAAAE1hbnVmYWN0dXJlcgEBAgEDAAAAADAA" +
           "AABOYW1lIG9mIHRoZSBjb21wYW55IHRoYXQgbWFudWZhY3R1cmVkIHRoZSBkZXZpY2UALgBEAgEAAAAV" +
           "/////wEB/////wAAAAA1YIkKAgAAAAIABQAAAE1vZGVsAQEDAQMAAAAAGAAAAE1vZGVsIG5hbWUgb2Yg" +
           "dGhlIGRldmljZQAuAEQDAQAAABX/////AQH/////AAAAADVgiQoCAAAAAgAQAAAASGFyZHdhcmVSZXZp" +
           "c2lvbgEBBwEDAAAAACwAAABSZXZpc2lvbiBsZXZlbCBvZiB0aGUgaGFyZHdhcmUgb2YgdGhlIGRldmlj" +
           "ZQAuAEQHAQAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAQAAAAU29mdHdhcmVSZXZpc2lvbgEBBgED" +
           "AAAAADUAAABSZXZpc2lvbiBsZXZlbCBvZiB0aGUgc29mdHdhcmUvZmlybXdhcmUgb2YgdGhlIGRldmlj" +
           "ZQAuAEQGAQAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAOAAAARGV2aWNlUmV2aXNpb24BAQUBAwAA" +
           "AAAkAAAAT3ZlcmFsbCByZXZpc2lvbiBsZXZlbCBvZiB0aGUgZGV2aWNlAC4ARAUBAAAADP////8BAf//" +
           "//8AAAAANWCJCgIAAAACAAwAAABEZXZpY2VNYW51YWwBAQQBAwAAAABaAAAAQWRkcmVzcyAocGF0aG5h" +
           "bWUgaW4gdGhlIGZpbGUgc3lzdGVtIG9yIGEgVVJMIHwgV2ViIGFkZHJlc3MpIG9mIHVzZXIgbWFudWFs" +
           "IGZvciB0aGUgZGV2aWNlAC4ARAQBAAAADP////8BAf////8AAAAANWCJCgIAAAACAAwAAABTZXJpYWxO" +
           "dW1iZXIBAQABAwAAAABNAAAASWRlbnRpZmllciB0aGF0IHVuaXF1ZWx5IGlkZW50aWZpZXMsIHdpdGhp" +
           "biBhIG1hbnVmYWN0dXJlciwgYSBkZXZpY2UgaW5zdGFuY2UALgBEAAEAAAAM/////wEB/////wAAAAA1" +
           "YIkKAgAAAAIADwAAAFJldmlzaW9uQ291bnRlcgEBAQEDAAAAAGkAAABBbiBpbmNyZW1lbnRhbCBjb3Vu" +
           "dGVyIGluZGljYXRpbmcgdGhlIG51bWJlciBvZiB0aW1lcyB0aGUgc3RhdGljIGRhdGEgd2l0aGluIHRo" +
           "ZSBEZXZpY2UgaGFzIGJlZW4gbW9kaWZpZWQALgBEAQEAAAAG/////wEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        #endregion

        #region Private Fields
        #endregion
    }
    #endif
    #endregion

    #region ServerCommunicationFFH1ServiceState Class
    #if (!OPCUA_EXCLUDE_ServerCommunicationFFH1ServiceState)
    /// <summary>
    /// Stores an instance of the ServerCommunicationFFH1ServiceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ServerCommunicationFFH1ServiceState : ServerCommunicationServiceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ServerCommunicationFFH1ServiceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationFFH1ServiceType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAqAAAAU2VydmVyQ29tbXVuaWNhdGlvbkZG" +
           "SDFTZXJ2aWNlVHlwZUluc3RhbmNlAQHlAwEB5QPlAwAA/////wkAAAAkYIAKAQAAAAIACQAAAE1ldGhv" +
           "ZFNldAEB6AMDAAAAABQAAABGbGF0IGxpc3Qgb2YgTWV0aG9kcwAvADroAwAA/////wQAAAAEYYIKBAAA" +
           "AAEACgAAAERpc2Nvbm5lY3QBASUEAC8BASwBJQQAAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElucHV0" +
           "QXJndW1lbnRzAQEmBAAuAEQmBAAAlgEAAAABACoBASYAAAAXAAAAQ29tbXVuaWNhdGlvblJlbGF0aW9u" +
           "SWQAD/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIAAAAAAA8AAABPdXRwdXRB" +
           "cmd1bWVudHMBAScEAC4ARCcEAACWAQAAAAEAKgEBGwAAAAwAAABTZXJ2aWNlRXJyb3IAG/////8AAAAA" +
           "AAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAABGGCCgQAAAABAAcAAABDb25uZWN0AQEoBAAvAQEoBCgE" +
           "AAABAf////8CAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBKQQALgBEKQQAAJYGAAAAAQAq" +
           "AQEmAAAAFwAAAENvbW11bmljYXRpb25SZWxhdGlvbklkAA//////AAAAAAABACoBARUAAAAGAAAATGlu" +
           "a0lkAAX/////AAAAAAABACoBARYAAAAHAAAAQWRkcmVzcwAD/////wAAAAAAAQAqAQEcAAAADQAAAE9y" +
           "ZGluYWxOdW1iZXIABv////8AAAAAAAEAKgEBHAAAAA0AAABTSUZDb25uZWN0aW9uAAH/////AAAAAAAB" +
           "ACoBARgAAAAJAAAAU2VydmljZUlkAAf/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAABdg" +
           "qQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQEqBAAuAEQqBAAAlgIAAAABACoBAR8AAAAQAAAARGVs" +
           "YXlGb3JOZXh0Q2FsbAAH/////wAAAAAAAQAqAQEbAAAADAAAAFNlcnZpY2VFcnJvcgAb/////wAAAAAA" +
           "AQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAEYYIKBAAAAAEACAAAAFRyYW5zZmVyAQErBAAvAQErBCsE" +
           "AAABAf////8CAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBLAQALgBELAQAAJYHAAAAAQAq" +
           "AQEmAAAAFwAAAENvbW11bmljYXRpb25SZWxhdGlvbklkAA//////AAAAAAABACoBARgAAAAJAAAAT1BF" +
           "UkFUSU9OAAz/////AAAAAAABACoBARcAAAAIAAAAQmxvY2tUYWcADP////8AAAAAAAEAKgEBFAAAAAUA" +
           "AABJTkRFWAAH/////wAAAAAAAQAqAQEYAAAACQAAAFNVQl9JTkRFWAAH/////wAAAAAAAQAqAQEcAAAA" +
           "CQAAAFdyaXRlRGF0YQADAQAAAAEAAAAAAAAAAAEAKgEBGAAAAAkAAABTZXJ2aWNlSWQAB/////8AAAAA" +
           "AAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1bWVudHMBAS0E" +
           "AC4ARC0EAACWAwAAAAEAKgEBGwAAAAgAAABSZWFkRGF0YQADAQAAAAEAAAAAAAAAAAEAKgEBHwAAABAA" +
           "AABEZWxheUZvck5leHRDYWxsAAf/////AAAAAAABACoBARsAAAAMAAAAU2VydmljZUVycm9yABv/////" +
           "AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAAARhggoEAAAAAQAQAAAAR2V0UHVibGlzaGVkRGF0" +
           "YQEBLgQALwEBLgQuBAAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAS8EAC4A" +
           "RC8EAACWAgAAAAEAKgEBJgAAABcAAABDb21tdW5pY2F0aW9uUmVsYXRpb25JZAAP/////wAAAAAAAQAq" +
           "AQEYAAAACQAAAFNlcnZpY2VJZAAH/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAXYKkK" +
           "AgAAAAAADwAAAE91dHB1dEFyZ3VtZW50cwEBMAQALgBEMAQAAJYGAAAAAQAqAQEXAAAACAAAAEJsb2Nr" +
           "VGFnAAz/////AAAAAAABACoBASEAAAAOAAAAQWxhcm1FdmVudERhdGEAAwEAAAABAAAAAAAAAAABACoB" +
           "AR0AAAAOAAAAQWxhcm1FdmVudFR5cGUAEf////8AAAAAAAEAKgEBGAAAAAkAAABUaW1lU3RhbXAADf//" +
           "//8AAAAAAAEAKgEBHwAAABAAAABEZWxheUZvck5leHRDYWxsAAf/////AAAAAAABACoBARsAAAAMAAAA" +
           "U2VydmljZUVycm9yABv/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAADVgiQoCAAAAAgAM" +
           "AAAATWFudWZhY3R1cmVyAQH+AwMAAAAAMAAAAE5hbWUgb2YgdGhlIGNvbXBhbnkgdGhhdCBtYW51ZmFj" +
           "dHVyZWQgdGhlIGRldmljZQAuAET+AwAAABX/////AQH/////AAAAADVgiQoCAAAAAgAFAAAATW9kZWwB" +
           "Af8DAwAAAAAYAAAATW9kZWwgbmFtZSBvZiB0aGUgZGV2aWNlAC4ARP8DAAAAFf////8BAf////8AAAAA" +
           "NWCJCgIAAAACABAAAABIYXJkd2FyZVJldmlzaW9uAQEDBAMAAAAALAAAAFJldmlzaW9uIGxldmVsIG9m" +
           "IHRoZSBoYXJkd2FyZSBvZiB0aGUgZGV2aWNlAC4ARAMEAAAADP////8BAf////8AAAAANWCJCgIAAAAC" +
           "ABAAAABTb2Z0d2FyZVJldmlzaW9uAQECBAMAAAAANQAAAFJldmlzaW9uIGxldmVsIG9mIHRoZSBzb2Z0" +
           "d2FyZS9maXJtd2FyZSBvZiB0aGUgZGV2aWNlAC4ARAIEAAAADP////8BAf////8AAAAANWCJCgIAAAAC" +
           "AA4AAABEZXZpY2VSZXZpc2lvbgEBAQQDAAAAACQAAABPdmVyYWxsIHJldmlzaW9uIGxldmVsIG9mIHRo" +
           "ZSBkZXZpY2UALgBEAQQAAAAM/////wEB/////wAAAAA1YIkKAgAAAAIADAAAAERldmljZU1hbnVhbAEB" +
           "AAQDAAAAAFoAAABBZGRyZXNzIChwYXRobmFtZSBpbiB0aGUgZmlsZSBzeXN0ZW0gb3IgYSBVUkwgfCBX" +
           "ZWIgYWRkcmVzcykgb2YgdXNlciBtYW51YWwgZm9yIHRoZSBkZXZpY2UALgBEAAQAAAAM/////wEB////" +
           "/wAAAAA1YIkKAgAAAAIADAAAAFNlcmlhbE51bWJlcgEB/AMDAAAAAE0AAABJZGVudGlmaWVyIHRoYXQg" +
           "dW5pcXVlbHkgaWRlbnRpZmllcywgd2l0aGluIGEgbWFudWZhY3R1cmVyLCBhIGRldmljZSBpbnN0YW5j" +
           "ZQAuAET8AwAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAPAAAAUmV2aXNpb25Db3VudGVyAQH9AwMA" +
           "AAAAaQAAAEFuIGluY3JlbWVudGFsIGNvdW50ZXIgaW5kaWNhdGluZyB0aGUgbnVtYmVyIG9mIHRpbWVz" +
           "IHRoZSBzdGF0aWMgZGF0YSB3aXRoaW4gdGhlIERldmljZSBoYXMgYmVlbiBtb2RpZmllZAAuAET9AwAA" +
           "AAb/////AQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        #endregion

        #region Private Fields
        #endregion
    }
    #endif
    #endregion

    #region ServerCommunicationFFHSEServiceState Class
    #if (!OPCUA_EXCLUDE_ServerCommunicationFFHSEServiceState)
    /// <summary>
    /// Stores an instance of the ServerCommunicationFFHSEServiceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ServerCommunicationFFHSEServiceState : ServerCommunicationServiceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ServerCommunicationFFHSEServiceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationFFHSEServiceType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQArAAAAU2VydmVyQ29tbXVuaWNhdGlvbkZG" +
           "SFNFU2VydmljZVR5cGVJbnN0YW5jZQEBMQQBATEEMQQAAP////8JAAAAJGCACgEAAAACAAkAAABNZXRo" +
           "b2RTZXQBATQEAwAAAAAUAAAARmxhdCBsaXN0IG9mIE1ldGhvZHMALwA6NAQAAP////8EAAAABGGCCgQA" +
           "AAABAAoAAABEaXNjb25uZWN0AQFxBAAvAQEsAXEEAAABAf////8CAAAAF2CpCgIAAAAAAA4AAABJbnB1" +
           "dEFyZ3VtZW50cwEBcgQALgBEcgQAAJYBAAAAAQAqAQEmAAAAFwAAAENvbW11bmljYXRpb25SZWxhdGlv" +
           "bklkAA//////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0" +
           "QXJndW1lbnRzAQFzBAAuAERzBAAAlgEAAAABACoBARsAAAAMAAAAU2VydmljZUVycm9yABv/////AAAA" +
           "AAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAAARhggoEAAAAAQAHAAAAQ29ubmVjdAEBdAQALwEBdAR0" +
           "BAAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAXUEAC4ARHUEAACWBAAAAAEA" +
           "KgEBJgAAABcAAABDb21tdW5pY2F0aW9uUmVsYXRpb25JZAAP/////wAAAAAAAQAqAQEWAAAABwAAAEFk" +
           "ZHJlc3MAD/////8AAAAAAAEAKgEBHAAAAA0AAABPcmRpbmFsTnVtYmVyAAb/////AAAAAAABACoBARgA" +
           "AAAJAAAAU2VydmljZUlkAAf/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAA" +
           "AAAPAAAAT3V0cHV0QXJndW1lbnRzAQF2BAAuAER2BAAAlgIAAAABACoBAR8AAAAQAAAARGVsYXlGb3JO" +
           "ZXh0Q2FsbAAH/////wAAAAAAAQAqAQEbAAAADAAAAFNlcnZpY2VFcnJvcgAb/////wAAAAAAAQAoAQEA" +
           "AAABAAAAAAAAAAEB/////wAAAAAEYYIKBAAAAAEACAAAAFRyYW5zZmVyAQF3BAAvAQF3BHcEAAABAf//" +
           "//8CAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBeAQALgBEeAQAAJYHAAAAAQAqAQEmAAAA" +
           "FwAAAENvbW11bmljYXRpb25SZWxhdGlvbklkAA//////AAAAAAABACoBARgAAAAJAAAAT1BFUkFUSU9O" +
           "AAz/////AAAAAAABACoBARcAAAAIAAAAQmxvY2tUYWcADP////8AAAAAAAEAKgEBFAAAAAUAAABJTkRF" +
           "WAAH/////wAAAAAAAQAqAQEYAAAACQAAAFNVQl9JTkRFWAAH/////wAAAAAAAQAqAQEcAAAACQAAAFdy" +
           "aXRlRGF0YQADAQAAAAEAAAAAAAAAAAEAKgEBGAAAAAkAAABTZXJ2aWNlSWQAB/////8AAAAAAAEAKAEB" +
           "AAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1bWVudHMBAXkEAC4ARHkE" +
           "AACWAwAAAAEAKgEBGwAAAAgAAABSZWFkRGF0YQADAQAAAAEAAAAAAAAAAAEAKgEBHwAAABAAAABEZWxh" +
           "eUZvck5leHRDYWxsAAf/////AAAAAAABACoBARsAAAAMAAAAU2VydmljZUVycm9yABv/////AAAAAAAB" +
           "ACgBAQAAAAEAAAAAAAAAAQH/////AAAAAARhggoEAAAAAQAQAAAAR2V0UHVibGlzaGVkRGF0YQEBegQA" +
           "LwEBegR6BAAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAXsEAC4ARHsEAACW" +
           "AgAAAAEAKgEBJgAAABcAAABDb21tdW5pY2F0aW9uUmVsYXRpb25JZAAP/////wAAAAAAAQAqAQEYAAAA" +
           "CQAAAFNlcnZpY2VJZAAH/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAXYKkKAgAAAAAA" +
           "DwAAAE91dHB1dEFyZ3VtZW50cwEBfAQALgBEfAQAAJYGAAAAAQAqAQEXAAAACAAAAEJsb2NrVGFnAAz/" +
           "////AAAAAAABACoBASEAAAAOAAAAQWxhcm1FdmVudERhdGEAAwEAAAABAAAAAAAAAAABACoBAR0AAAAO" +
           "AAAAQWxhcm1FdmVudFR5cGUAEf////8AAAAAAAEAKgEBGAAAAAkAAABUaW1lU3RhbXAADf////8AAAAA" +
           "AAEAKgEBHwAAABAAAABEZWxheUZvck5leHRDYWxsAAf/////AAAAAAABACoBARsAAAAMAAAAU2Vydmlj" +
           "ZUVycm9yABv/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAADVgiQoCAAAAAgAMAAAATWFu" +
           "dWZhY3R1cmVyAQFKBAMAAAAAMAAAAE5hbWUgb2YgdGhlIGNvbXBhbnkgdGhhdCBtYW51ZmFjdHVyZWQg" +
           "dGhlIGRldmljZQAuAERKBAAAABX/////AQH/////AAAAADVgiQoCAAAAAgAFAAAATW9kZWwBAUsEAwAA" +
           "AAAYAAAATW9kZWwgbmFtZSBvZiB0aGUgZGV2aWNlAC4AREsEAAAAFf////8BAf////8AAAAANWCJCgIA" +
           "AAACABAAAABIYXJkd2FyZVJldmlzaW9uAQFPBAMAAAAALAAAAFJldmlzaW9uIGxldmVsIG9mIHRoZSBo" +
           "YXJkd2FyZSBvZiB0aGUgZGV2aWNlAC4ARE8EAAAADP////8BAf////8AAAAANWCJCgIAAAACABAAAABT" +
           "b2Z0d2FyZVJldmlzaW9uAQFOBAMAAAAANQAAAFJldmlzaW9uIGxldmVsIG9mIHRoZSBzb2Z0d2FyZS9m" +
           "aXJtd2FyZSBvZiB0aGUgZGV2aWNlAC4ARE4EAAAADP////8BAf////8AAAAANWCJCgIAAAACAA4AAABE" +
           "ZXZpY2VSZXZpc2lvbgEBTQQDAAAAACQAAABPdmVyYWxsIHJldmlzaW9uIGxldmVsIG9mIHRoZSBkZXZp" +
           "Y2UALgBETQQAAAAM/////wEB/////wAAAAA1YIkKAgAAAAIADAAAAERldmljZU1hbnVhbAEBTAQDAAAA" +
           "AFoAAABBZGRyZXNzIChwYXRobmFtZSBpbiB0aGUgZmlsZSBzeXN0ZW0gb3IgYSBVUkwgfCBXZWIgYWRk" +
           "cmVzcykgb2YgdXNlciBtYW51YWwgZm9yIHRoZSBkZXZpY2UALgBETAQAAAAM/////wEB/////wAAAAA1" +
           "YIkKAgAAAAIADAAAAFNlcmlhbE51bWJlcgEBSAQDAAAAAE0AAABJZGVudGlmaWVyIHRoYXQgdW5pcXVl" +
           "bHkgaWRlbnRpZmllcywgd2l0aGluIGEgbWFudWZhY3R1cmVyLCBhIGRldmljZSBpbnN0YW5jZQAuAERI" +
           "BAAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAPAAAAUmV2aXNpb25Db3VudGVyAQFJBAMAAAAAaQAA" +
           "AEFuIGluY3JlbWVudGFsIGNvdW50ZXIgaW5kaWNhdGluZyB0aGUgbnVtYmVyIG9mIHRpbWVzIHRoZSBz" +
           "dGF0aWMgZGF0YSB3aXRoaW4gdGhlIERldmljZSBoYXMgYmVlbiBtb2RpZmllZAAuAERJBAAAAAb/////" +
           "AQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        #endregion

        #region Private Fields
        #endregion
    }
    #endif
    #endregion

    #region ServerCommunicationPROFIBUSServiceState Class
    #if (!OPCUA_EXCLUDE_ServerCommunicationPROFIBUSServiceState)
    /// <summary>
    /// Stores an instance of the ServerCommunicationPROFIBUSServiceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ServerCommunicationPROFIBUSServiceState : ServerCommunicationServiceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ServerCommunicationPROFIBUSServiceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationPROFIBUSServiceType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAuAAAAU2VydmVyQ29tbXVuaWNhdGlvblBS" +
           "T0ZJQlVTU2VydmljZVR5cGVJbnN0YW5jZQEBfQQBAX0EfQQAAP////8JAAAAJGCACgEAAAACAAkAAABN" +
           "ZXRob2RTZXQBAYAEAwAAAAAUAAAARmxhdCBsaXN0IG9mIE1ldGhvZHMALwA6gAQAAP////8DAAAABGGC" +
           "CgQAAAABAAoAAABEaXNjb25uZWN0AQG9BAAvAQEsAb0EAAABAf////8CAAAAF2CpCgIAAAAAAA4AAABJ" +
           "bnB1dEFyZ3VtZW50cwEBvgQALgBEvgQAAJYBAAAAAQAqAQEmAAAAFwAAAENvbW11bmljYXRpb25SZWxh" +
           "dGlvbklkAA//////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0" +
           "cHV0QXJndW1lbnRzAQG/BAAuAES/BAAAlgEAAAABACoBARsAAAAMAAAAU2VydmljZUVycm9yABv/////" +
           "AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAAARhggoEAAAAAQAHAAAAQ29ubmVjdAEBwAQALwEB" +
           "wATABAAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAcEEAC4ARMEEAACWAwAA" +
           "AAEAKgEBJgAAABcAAABDb21tdW5pY2F0aW9uUmVsYXRpb25JZAAP/////wAAAAAAAQAqAQEWAAAABwAA" +
           "AEFkZHJlc3MAA/////8AAAAAAAEAKgEBHQAAAA4AAABNYW51ZmFjdHVyZXJJZAAF/////wAAAAAAAQAo" +
           "AQEAAAABAAAAAAAAAAEB/////wAAAAAXYKkKAgAAAAAADwAAAE91dHB1dEFyZ3VtZW50cwEBwgQALgBE" +
           "wgQAAJYBAAAAAQAqAQEbAAAADAAAAFNlcnZpY2VFcnJvcgAb/////wAAAAAAAQAoAQEAAAABAAAAAAAA" +
           "AAEB/////wAAAAAEYYIKBAAAAAEACAAAAFRyYW5zZmVyAQHDBAAvAQHDBMMEAAABAf////8CAAAAF2Cp" +
           "CgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBxAQALgBExAQAAJYFAAAAAQAqAQEmAAAAFwAAAENvbW11" +
           "bmljYXRpb25SZWxhdGlvbklkAA//////AAAAAAABACoBARgAAAAJAAAAT1BFUkFUSU9OAAz/////AAAA" +
           "AAABACoBARMAAAAEAAAAU0xPVAAD/////wAAAAAAAQAqAQEUAAAABQAAAElOREVYAAP/////AAAAAAAB" +
           "ACoBARYAAAAHAAAAUkVRVUVTVAAP/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAXYKkK" +
           "AgAAAAAADwAAAE91dHB1dEFyZ3VtZW50cwEBxQQALgBExQQAAJYDAAAAAQAqAQEUAAAABQAAAFJFUExZ" +
           "AA//////AAAAAAABACoBAR0AAAAOAAAAUkVTUE9OU0VfQ09ERVMAD/////8AAAAAAAEAKgEBGwAAAAwA" +
           "AABTZXJ2aWNlRXJyb3IAG/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAANWCJCgIAAAAC" +
           "AAwAAABNYW51ZmFjdHVyZXIBAZYEAwAAAAAwAAAATmFtZSBvZiB0aGUgY29tcGFueSB0aGF0IG1hbnVm" +
           "YWN0dXJlZCB0aGUgZGV2aWNlAC4ARJYEAAAAFf////8BAf////8AAAAANWCJCgIAAAACAAUAAABNb2Rl" +
           "bAEBlwQDAAAAABgAAABNb2RlbCBuYW1lIG9mIHRoZSBkZXZpY2UALgBElwQAAAAV/////wEB/////wAA" +
           "AAA1YIkKAgAAAAIAEAAAAEhhcmR3YXJlUmV2aXNpb24BAZsEAwAAAAAsAAAAUmV2aXNpb24gbGV2ZWwg" +
           "b2YgdGhlIGhhcmR3YXJlIG9mIHRoZSBkZXZpY2UALgBEmwQAAAAM/////wEB/////wAAAAA1YIkKAgAA" +
           "AAIAEAAAAFNvZnR3YXJlUmV2aXNpb24BAZoEAwAAAAA1AAAAUmV2aXNpb24gbGV2ZWwgb2YgdGhlIHNv" +
           "ZnR3YXJlL2Zpcm13YXJlIG9mIHRoZSBkZXZpY2UALgBEmgQAAAAM/////wEB/////wAAAAA1YIkKAgAA" +
           "AAIADgAAAERldmljZVJldmlzaW9uAQGZBAMAAAAAJAAAAE92ZXJhbGwgcmV2aXNpb24gbGV2ZWwgb2Yg" +
           "dGhlIGRldmljZQAuAESZBAAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAMAAAARGV2aWNlTWFudWFs" +
           "AQGYBAMAAAAAWgAAAEFkZHJlc3MgKHBhdGhuYW1lIGluIHRoZSBmaWxlIHN5c3RlbSBvciBhIFVSTCB8" +
           "IFdlYiBhZGRyZXNzKSBvZiB1c2VyIG1hbnVhbCBmb3IgdGhlIGRldmljZQAuAESYBAAAAAz/////AQH/" +
           "////AAAAADVgiQoCAAAAAgAMAAAAU2VyaWFsTnVtYmVyAQGUBAMAAAAATQAAAElkZW50aWZpZXIgdGhh" +
           "dCB1bmlxdWVseSBpZGVudGlmaWVzLCB3aXRoaW4gYSBtYW51ZmFjdHVyZXIsIGEgZGV2aWNlIGluc3Rh" +
           "bmNlAC4ARJQEAAAADP////8BAf////8AAAAANWCJCgIAAAACAA8AAABSZXZpc2lvbkNvdW50ZXIBAZUE" +
           "AwAAAABpAAAAQW4gaW5jcmVtZW50YWwgY291bnRlciBpbmRpY2F0aW5nIHRoZSBudW1iZXIgb2YgdGlt" +
           "ZXMgdGhlIHN0YXRpYyBkYXRhIHdpdGhpbiB0aGUgRGV2aWNlIGhhcyBiZWVuIG1vZGlmaWVkAC4ARJUE" +
           "AAAABv////8BAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        #endregion

        #region Private Fields
        #endregion
    }
    #endif
    #endregion

    #region ServerCommunicationPROFINETServiceState Class
    #if (!OPCUA_EXCLUDE_ServerCommunicationPROFINETServiceState)
    /// <summary>
    /// Stores an instance of the ServerCommunicationPROFINETServiceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ServerCommunicationPROFINETServiceState : ServerCommunicationServiceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ServerCommunicationPROFINETServiceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationPROFINETServiceType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAuAAAAU2VydmVyQ29tbXVuaWNhdGlvblBS" +
           "T0ZJTkVUU2VydmljZVR5cGVJbnN0YW5jZQEBxgQBAcYExgQAAP////8JAAAAJGCACgEAAAACAAkAAABN" +
           "ZXRob2RTZXQBAckEAwAAAAAUAAAARmxhdCBsaXN0IG9mIE1ldGhvZHMALwA6yQQAAP////8DAAAABGGC" +
           "CgQAAAABAAoAAABEaXNjb25uZWN0AQEGBQAvAQEsAQYFAAABAf////8CAAAAF2CpCgIAAAAAAA4AAABJ" +
           "bnB1dEFyZ3VtZW50cwEBBwUALgBEBwUAAJYBAAAAAQAqAQEmAAAAFwAAAENvbW11bmljYXRpb25SZWxh" +
           "dGlvbklkAA//////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0" +
           "cHV0QXJndW1lbnRzAQEIBQAuAEQIBQAAlgEAAAABACoBARsAAAAMAAAAU2VydmljZUVycm9yABv/////" +
           "AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAAARhggoEAAAAAQAHAAAAQ29ubmVjdAEBCQUALwEB" +
           "CQUJBQAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAQoFAC4ARAoFAACWBAAA" +
           "AAEAKgEBJgAAABcAAABDb21tdW5pY2F0aW9uUmVsYXRpb25JZAAP/////wAAAAAAAQAqAQEWAAAABwAA" +
           "AEROU05BTUUADP////8AAAAAAAEAKgEBFwAAAAgAAABEZXZpY2VJRAAF/////wAAAAAAAQAqAQEXAAAA" +
           "CAAAAFZlbmRvcklEAAX/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAP" +
           "AAAAT3V0cHV0QXJndW1lbnRzAQELBQAuAEQLBQAAlgEAAAABACoBARsAAAAMAAAAU2VydmljZUVycm9y" +
           "ABv/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAAARhggoEAAAAAQAIAAAAVHJhbnNmZXIB" +
           "AQwFAC8BAQwFDAUAAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElucHV0QXJndW1lbnRzAQENBQAuAEQN" +
           "BQAAlgcAAAABACoBASYAAAAXAAAAQ29tbXVuaWNhdGlvblJlbGF0aW9uSWQAD/////8AAAAAAAEAKgEB" +
           "GAAAAAkAAABPUEVSQVRJT04ADP////8AAAAAAAEAKgEBEwAAAAQAAABTTE9UAAX/////AAAAAAABACoB" +
           "ARYAAAAHAAAAU1VCU0xPVAAF/////wAAAAAAAQAqAQEUAAAABQAAAElOREVYAAX/////AAAAAAABACoB" +
           "ARIAAAADAAAAQVBJAAf/////AAAAAAABACoBARYAAAAHAAAAUkVRVUVTVAAP/////wAAAAAAAQAoAQEA" +
           "AAABAAAAAAAAAAEB/////wAAAAAXYKkKAgAAAAAADwAAAE91dHB1dEFyZ3VtZW50cwEBDgUALgBEDgUA" +
           "AJYDAAAAAQAqAQEUAAAABQAAAFJFUExZAA//////AAAAAAABACoBAR0AAAAOAAAAUkVTUE9OU0VfQ09E" +
           "RVMAD/////8AAAAAAAEAKgEBGwAAAAwAAABTZXJ2aWNlRXJyb3IAG/////8AAAAAAAEAKAEBAAAAAQAA" +
           "AAAAAAABAf////8AAAAANWCJCgIAAAACAAwAAABNYW51ZmFjdHVyZXIBAd8EAwAAAAAwAAAATmFtZSBv" +
           "ZiB0aGUgY29tcGFueSB0aGF0IG1hbnVmYWN0dXJlZCB0aGUgZGV2aWNlAC4ARN8EAAAAFf////8BAf//" +
           "//8AAAAANWCJCgIAAAACAAUAAABNb2RlbAEB4AQDAAAAABgAAABNb2RlbCBuYW1lIG9mIHRoZSBkZXZp" +
           "Y2UALgBE4AQAAAAV/////wEB/////wAAAAA1YIkKAgAAAAIAEAAAAEhhcmR3YXJlUmV2aXNpb24BAeQE" +
           "AwAAAAAsAAAAUmV2aXNpb24gbGV2ZWwgb2YgdGhlIGhhcmR3YXJlIG9mIHRoZSBkZXZpY2UALgBE5AQA" +
           "AAAM/////wEB/////wAAAAA1YIkKAgAAAAIAEAAAAFNvZnR3YXJlUmV2aXNpb24BAeMEAwAAAAA1AAAA" +
           "UmV2aXNpb24gbGV2ZWwgb2YgdGhlIHNvZnR3YXJlL2Zpcm13YXJlIG9mIHRoZSBkZXZpY2UALgBE4wQA" +
           "AAAM/////wEB/////wAAAAA1YIkKAgAAAAIADgAAAERldmljZVJldmlzaW9uAQHiBAMAAAAAJAAAAE92" +
           "ZXJhbGwgcmV2aXNpb24gbGV2ZWwgb2YgdGhlIGRldmljZQAuAETiBAAAAAz/////AQH/////AAAAADVg" +
           "iQoCAAAAAgAMAAAARGV2aWNlTWFudWFsAQHhBAMAAAAAWgAAAEFkZHJlc3MgKHBhdGhuYW1lIGluIHRo" +
           "ZSBmaWxlIHN5c3RlbSBvciBhIFVSTCB8IFdlYiBhZGRyZXNzKSBvZiB1c2VyIG1hbnVhbCBmb3IgdGhl" +
           "IGRldmljZQAuAEThBAAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAMAAAAU2VyaWFsTnVtYmVyAQHd" +
           "BAMAAAAATQAAAElkZW50aWZpZXIgdGhhdCB1bmlxdWVseSBpZGVudGlmaWVzLCB3aXRoaW4gYSBtYW51" +
           "ZmFjdHVyZXIsIGEgZGV2aWNlIGluc3RhbmNlAC4ARN0EAAAADP////8BAf////8AAAAANWCJCgIAAAAC" +
           "AA8AAABSZXZpc2lvbkNvdW50ZXIBAd4EAwAAAABpAAAAQW4gaW5jcmVtZW50YWwgY291bnRlciBpbmRp" +
           "Y2F0aW5nIHRoZSBudW1iZXIgb2YgdGltZXMgdGhlIHN0YXRpYyBkYXRhIHdpdGhpbiB0aGUgRGV2aWNl" +
           "IGhhcyBiZWVuIG1vZGlmaWVkAC4ARN4EAAAABv////8BAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        #endregion

        #region Private Fields
        #endregion
    }
    #endif
    #endregion

    #region ServerCommunicationHARTServiceState Class
    #if (!OPCUA_EXCLUDE_ServerCommunicationHARTServiceState)
    /// <summary>
    /// Stores an instance of the ServerCommunicationHARTServiceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ServerCommunicationHARTServiceState : ServerCommunicationServiceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ServerCommunicationHARTServiceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationHARTServiceType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAqAAAAU2VydmVyQ29tbXVuaWNhdGlvbkhB" +
           "UlRTZXJ2aWNlVHlwZUluc3RhbmNlAQEPBQEBDwUPBQAA/////wkAAAAkYIAKAQAAAAIACQAAAE1ldGhv" +
           "ZFNldAEBEgUDAAAAABQAAABGbGF0IGxpc3Qgb2YgTWV0aG9kcwAvADoSBQAA/////wQAAAAEYYIKBAAA" +
           "AAEACgAAAERpc2Nvbm5lY3QBAU8FAC8BASwBTwUAAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElucHV0" +
           "QXJndW1lbnRzAQFQBQAuAERQBQAAlgEAAAABACoBASYAAAAXAAAAQ29tbXVuaWNhdGlvblJlbGF0aW9u" +
           "SWQAD/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIAAAAAAA8AAABPdXRwdXRB" +
           "cmd1bWVudHMBAVEFAC4ARFEFAACWAQAAAAEAKgEBGwAAAAwAAABTZXJ2aWNlRXJyb3IAG/////8AAAAA" +
           "AAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAABGGCCgQAAAABAAcAAABDb25uZWN0AQFSBQAvAQFSBVIF" +
           "AAABAf////8CAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBUwUALgBEUwUAAJYCAAAAAQAq" +
           "AQEmAAAAFwAAAENvbW11bmljYXRpb25SZWxhdGlvbklkAA//////AAAAAAABACoBARoAAAALAAAATG9u" +
           "Z0FkZHJlc3MAD/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIAAAAAAA8AAABP" +
           "dXRwdXRBcmd1bWVudHMBAVQFAC4ARFQFAACWAQAAAAEAKgEBGwAAAAwAAABTZXJ2aWNlRXJyb3IAG///" +
           "//8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAABGGCCgQAAAABAAgAAABUcmFuc2ZlcgEBVQUA" +
           "LwEBVQVVBQAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAVYFAC4ARFYFAACW" +
           "AwAAAAEAKgEBJgAAABcAAABDb21tdW5pY2F0aW9uUmVsYXRpb25JZAAP/////wAAAAAAAQAqAQEWAAAA" +
           "BwAAAENvbW1hbmQABf////8AAAAAAAEAKgEBFgAAAAcAAABSZXF1ZXN0AA//////AAAAAAABACgBAQAA" +
           "AAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQFXBQAuAERXBQAA" +
           "lgIAAAABACoBARQAAAAFAAAAUmVwbHkAD/////8AAAAAAAEAKgEBGwAAAAwAAABTZXJ2aWNlRXJyb3IA" +
           "G/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAABGGCCgQAAAABABAAAABHZXRQdWJsaXNo" +
           "ZWREYXRhAQFYBQAvAQFYBVgFAAABAf////8CAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEB" +
           "WQUALgBEWQUAAJYBAAAAAQAqAQEmAAAAFwAAAENvbW11bmljYXRpb25SZWxhdGlvbklkAA//////AAAA" +
           "AAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQFa" +
           "BQAuAERaBQAAlgQAAAABACoBARYAAAAHAAAAQ29tbWFuZAAF/////wAAAAAAAQAqAQEUAAAABQAAAFJl" +
           "cGx5AA//////AAAAAAABACoBARgAAAAJAAAAVGltZVN0YW1wAA3/////AAAAAAABACoBARsAAAAMAAAA" +
           "U2VydmljZUVycm9yABv/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAADVgiQoCAAAAAgAM" +
           "AAAATWFudWZhY3R1cmVyAQEoBQMAAAAAMAAAAE5hbWUgb2YgdGhlIGNvbXBhbnkgdGhhdCBtYW51ZmFj" +
           "dHVyZWQgdGhlIGRldmljZQAuAEQoBQAAABX/////AQH/////AAAAADVgiQoCAAAAAgAFAAAATW9kZWwB" +
           "ASkFAwAAAAAYAAAATW9kZWwgbmFtZSBvZiB0aGUgZGV2aWNlAC4ARCkFAAAAFf////8BAf////8AAAAA" +
           "NWCJCgIAAAACABAAAABIYXJkd2FyZVJldmlzaW9uAQEtBQMAAAAALAAAAFJldmlzaW9uIGxldmVsIG9m" +
           "IHRoZSBoYXJkd2FyZSBvZiB0aGUgZGV2aWNlAC4ARC0FAAAADP////8BAf////8AAAAANWCJCgIAAAAC" +
           "ABAAAABTb2Z0d2FyZVJldmlzaW9uAQEsBQMAAAAANQAAAFJldmlzaW9uIGxldmVsIG9mIHRoZSBzb2Z0" +
           "d2FyZS9maXJtd2FyZSBvZiB0aGUgZGV2aWNlAC4ARCwFAAAADP////8BAf////8AAAAANWCJCgIAAAAC" +
           "AA4AAABEZXZpY2VSZXZpc2lvbgEBKwUDAAAAACQAAABPdmVyYWxsIHJldmlzaW9uIGxldmVsIG9mIHRo" +
           "ZSBkZXZpY2UALgBEKwUAAAAM/////wEB/////wAAAAA1YIkKAgAAAAIADAAAAERldmljZU1hbnVhbAEB" +
           "KgUDAAAAAFoAAABBZGRyZXNzIChwYXRobmFtZSBpbiB0aGUgZmlsZSBzeXN0ZW0gb3IgYSBVUkwgfCBX" +
           "ZWIgYWRkcmVzcykgb2YgdXNlciBtYW51YWwgZm9yIHRoZSBkZXZpY2UALgBEKgUAAAAM/////wEB////" +
           "/wAAAAA1YIkKAgAAAAIADAAAAFNlcmlhbE51bWJlcgEBJgUDAAAAAE0AAABJZGVudGlmaWVyIHRoYXQg" +
           "dW5pcXVlbHkgaWRlbnRpZmllcywgd2l0aGluIGEgbWFudWZhY3R1cmVyLCBhIGRldmljZSBpbnN0YW5j" +
           "ZQAuAEQmBQAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAPAAAAUmV2aXNpb25Db3VudGVyAQEnBQMA" +
           "AAAAaQAAAEFuIGluY3JlbWVudGFsIGNvdW50ZXIgaW5kaWNhdGluZyB0aGUgbnVtYmVyIG9mIHRpbWVz" +
           "IHRoZSBzdGF0aWMgZGF0YSB3aXRoaW4gdGhlIERldmljZSBoYXMgYmVlbiBtb2RpZmllZAAuAEQnBQAA" +
           "AAb/////AQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        #endregion

        #region Private Fields
        #endregion
    }
    #endif
    #endregion

    #region ServerCommunicationISA100_WirelessServiceState Class
    #if (!OPCUA_EXCLUDE_ServerCommunicationISA100_WirelessServiceState)
    /// <summary>
    /// Stores an instance of the ServerCommunicationISA100_WirelessServiceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ServerCommunicationISA100_WirelessServiceState : ServerCommunicationServiceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ServerCommunicationISA100_WirelessServiceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationISA100_WirelessServiceType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQA1AAAAU2VydmVyQ29tbXVuaWNhdGlvbklT" +
           "QTEwMF9XaXJlbGVzc1NlcnZpY2VUeXBlSW5zdGFuY2UBAQkIAQEJCAkIAAD/////CQAAACRggAoBAAAA" +
           "AgAJAAAATWV0aG9kU2V0AQEMCAMAAAAAFAAAAEZsYXQgbGlzdCBvZiBNZXRob2RzAC8AOgwIAAD/////" +
           "BAAAAARhggoEAAAAAQAKAAAARGlzY29ubmVjdAEBSQgALwEBLAFJCAAAAQH/////AgAAABdgqQoCAAAA" +
           "AAAOAAAASW5wdXRBcmd1bWVudHMBAUoIAC4AREoIAACWAQAAAAEAKgEBJgAAABcAAABDb21tdW5pY2F0" +
           "aW9uUmVsYXRpb25JZAAP/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAXYKkKAgAAAAAA" +
           "DwAAAE91dHB1dEFyZ3VtZW50cwEBSwgALgBESwgAAJYBAAAAAQAqAQEbAAAADAAAAFNlcnZpY2VFcnJv" +
           "cgAb/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAEYYIKBAAAAAEABwAAAENvbm5lY3QB" +
           "AUwIAC8BAUwITAgAAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElucHV0QXJndW1lbnRzAQFNCAAuAERN" +
           "CAAAlgMAAAABACoBASYAAAAXAAAAQ29tbXVuaWNhdGlvblJlbGF0aW9uSWQAD/////8AAAAAAAEAKgEB" +
           "GAAAAAkAAABJUEFkZHJlc3MAD/////8AAAAAAAEAKgEBGgAAAAsAAABDb25uZWN0VHlwZQAH/////wAA" +
           "AAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAXYKkKAgAAAAAADwAAAE91dHB1dEFyZ3VtZW50cwEB" +
           "TggALgBETggAAJYBAAAAAQAqAQEbAAAADAAAAFNlcnZpY2VFcnJvcgAb/////wAAAAAAAQAoAQEAAAAB" +
           "AAAAAAAAAAEB/////wAAAAAEYYIKBAAAAAEACAAAAFRyYW5zZmVyAQFPCAAvAQFPCE8IAAABAf////8C" +
           "AAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBUAgALgBEUAgAAJYIAAAAAQAqAQEmAAAAFwAA" +
           "AENvbW11bmljYXRpb25SZWxhdGlvbklkAA//////AAAAAAABACoBARgAAAAJAAAAT1BFUkFUSU9OAAz/" +
           "////AAAAAAABACoBARQAAAAFAAAAQXBwSUQABf////8AAAAAAAEAKgEBFwAAAAgAAABPYmplY3RJRAAF" +
           "/////wAAAAAAAQAqAQEbAAAADAAAAEF0dHJPck1ldGhJRAAF/////wAAAAAAAQAqAQEYAAAACQAAAFNV" +
           "Ql9JTkRFWAAH/////wAAAAAAAQAqAQEcAAAACQAAAFdyaXRlRGF0YQADAQAAAAEAAAAAAAAAAAEAKgEB" +
           "GAAAAAkAAABSZXF1ZXN0SWQAB/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIA" +
           "AAAAAA8AAABPdXRwdXRBcmd1bWVudHMBAVEIAC4ARFEIAACWAgAAAAEAKgEBGwAAAAgAAABSZWFkRGF0" +
           "YQADAQAAAAEAAAAAAAAAAAEAKgEBGwAAAAwAAABTZXJ2aWNlRXJyb3IAG/////8AAAAAAAEAKAEBAAAA" +
           "AQAAAAAAAAABAf////8AAAAABGGCCgQAAAABABAAAABHZXRQdWJsaXNoZWREYXRhAQFSCAAvAQFSCFII" +
           "AAABAf////8CAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBUwgALgBEUwgAAJYBAAAAAQAq" +
           "AQEmAAAAFwAAAENvbW11bmljYXRpb25SZWxhdGlvbklkAA//////AAAAAAABACgBAQAAAAEAAAAAAAAA" +
           "AQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQFUCAAuAERUCAAAlgYAAAABACoB" +
           "ARQAAAAFAAAAQXBwSUQABf////8AAAAAAAEAKgEBFwAAAAgAAABPYmplY3RJRAAF/////wAAAAAAAQAq" +
           "AQEhAAAADgAAAEFsYXJtRXZlbnREYXRhAAMBAAAAAQAAAAAAAAAAAQAqAQEdAAAADgAAAEFsYXJtRXZl" +
           "bnRUeXBlAAX/////AAAAAAABACoBARgAAAAJAAAAVGltZVN0YW1wAA3/////AAAAAAABACoBARsAAAAM" +
           "AAAAU2VydmljZUVycm9yABv/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAADVgiQoCAAAA" +
           "AgAMAAAATWFudWZhY3R1cmVyAQEiCAMAAAAAMAAAAE5hbWUgb2YgdGhlIGNvbXBhbnkgdGhhdCBtYW51" +
           "ZmFjdHVyZWQgdGhlIGRldmljZQAuAEQiCAAAABX/////AQH/////AAAAADVgiQoCAAAAAgAFAAAATW9k" +
           "ZWwBASMIAwAAAAAYAAAATW9kZWwgbmFtZSBvZiB0aGUgZGV2aWNlAC4ARCMIAAAAFf////8BAf////8A" +
           "AAAANWCJCgIAAAACABAAAABIYXJkd2FyZVJldmlzaW9uAQEnCAMAAAAALAAAAFJldmlzaW9uIGxldmVs" +
           "IG9mIHRoZSBoYXJkd2FyZSBvZiB0aGUgZGV2aWNlAC4ARCcIAAAADP////8BAf////8AAAAANWCJCgIA" +
           "AAACABAAAABTb2Z0d2FyZVJldmlzaW9uAQEmCAMAAAAANQAAAFJldmlzaW9uIGxldmVsIG9mIHRoZSBz" +
           "b2Z0d2FyZS9maXJtd2FyZSBvZiB0aGUgZGV2aWNlAC4ARCYIAAAADP////8BAf////8AAAAANWCJCgIA" +
           "AAACAA4AAABEZXZpY2VSZXZpc2lvbgEBJQgDAAAAACQAAABPdmVyYWxsIHJldmlzaW9uIGxldmVsIG9m" +
           "IHRoZSBkZXZpY2UALgBEJQgAAAAM/////wEB/////wAAAAA1YIkKAgAAAAIADAAAAERldmljZU1hbnVh" +
           "bAEBJAgDAAAAAFoAAABBZGRyZXNzIChwYXRobmFtZSBpbiB0aGUgZmlsZSBzeXN0ZW0gb3IgYSBVUkwg" +
           "fCBXZWIgYWRkcmVzcykgb2YgdXNlciBtYW51YWwgZm9yIHRoZSBkZXZpY2UALgBEJAgAAAAM/////wEB" +
           "/////wAAAAA1YIkKAgAAAAIADAAAAFNlcmlhbE51bWJlcgEBIAgDAAAAAE0AAABJZGVudGlmaWVyIHRo" +
           "YXQgdW5pcXVlbHkgaWRlbnRpZmllcywgd2l0aGluIGEgbWFudWZhY3R1cmVyLCBhIGRldmljZSBpbnN0" +
           "YW5jZQAuAEQgCAAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAPAAAAUmV2aXNpb25Db3VudGVyAQEh" +
           "CAMAAAAAaQAAAEFuIGluY3JlbWVudGFsIGNvdW50ZXIgaW5kaWNhdGluZyB0aGUgbnVtYmVyIG9mIHRp" +
           "bWVzIHRoZSBzdGF0aWMgZGF0YSB3aXRoaW4gdGhlIERldmljZSBoYXMgYmVlbiBtb2RpZmllZAAuAEQh" +
           "CAAAAAb/////AQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        #endregion

        #region Private Fields
        #endregion
    }
    #endif
    #endregion

    #region ServerCommunicationGENERICServiceState Class
    #if (!OPCUA_EXCLUDE_ServerCommunicationGENERICServiceState)
    /// <summary>
    /// Stores an instance of the ServerCommunicationGENERICServiceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ServerCommunicationGENERICServiceState : ServerCommunicationServiceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ServerCommunicationGENERICServiceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationGENERICServiceType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7, namespaceUris);
        }

        #if (!OPCUA_EXCLUDE_InitializationStrings)
        /// <summary>
        /// Initializes the instance.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            Initialize(context, InitializationString);
            InitializeOptionalChildren(context);
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }

        /// <summary>
        /// Initializes the any option children defined for the instance.
        /// </summary>
        protected override void InitializeOptionalChildren(ISystemContext context)
        {
            base.InitializeOptionalChildren(context);
        }

        #region Initialization String
        private const string InitializationString =
           "AgAAACYAAABodHRwOi8vZmRpLWNvb3BlcmF0aW9uLmNvbS9PUENVQS9GREk3Lx8AAABodHRwOi8vb3Bj" +
           "Zm91bmRhdGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAtAAAAU2VydmVyQ29tbXVuaWNhdGlvbkdF" +
           "TkVSSUNTZXJ2aWNlVHlwZUluc3RhbmNlAQFVCAEBVQhVCAAA/////wkAAAAkYIAKAQAAAAIACQAAAE1l" +
           "dGhvZFNldAEBWAgDAAAAABQAAABGbGF0IGxpc3Qgb2YgTWV0aG9kcwAvADpYCAAA/////wMAAAAEYYIK" +
           "BAAAAAEACgAAAERpc2Nvbm5lY3QBAZUIAC8BASwBlQgAAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElu" +
           "cHV0QXJndW1lbnRzAQGWCAAuAESWCAAAlgEAAAABACoBASYAAAAXAAAAQ29tbXVuaWNhdGlvblJlbGF0" +
           "aW9uSWQAD/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIAAAAAAA8AAABPdXRw" +
           "dXRBcmd1bWVudHMBAZcIAC4ARJcIAACWAQAAAAEAKgEBGwAAAAwAAABTZXJ2aWNlRXJyb3IAG/////8A" +
           "AAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAABGGCCgQAAAABAAcAAABDb25uZWN0AQGYCAAvAQGY" +
           "CJgIAAABAf////8CAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBmQgALgBEmQgAAJYCAAAA" +
           "AQAqAQEmAAAAFwAAAENvbW11bmljYXRpb25SZWxhdGlvbklkAA//////AAAAAAABACoBARYAAAAHAAAA" +
           "QWRkcmVzcwAP/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAXYKkKAgAAAAAADwAAAE91" +
           "dHB1dEFyZ3VtZW50cwEBmggALgBEmggAAJYBAAAAAQAqAQEbAAAADAAAAFNlcnZpY2VFcnJvcgAb////" +
           "/wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAEYYIKBAAAAAEACAAAAFRyYW5zZmVyAQGbCAAv" +
           "AQGbCJsIAAABAf////8CAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBnAgALgBEnAgAAJYF" +
           "AAAAAQAqAQEmAAAAFwAAAENvbW11bmljYXRpb25SZWxhdGlvbklkAA//////AAAAAAABACoBARUAAAAG" +
           "AAAASGVhZGVyAAz/////AAAAAAABACoBARoAAAALAAAAUmVxdWVzdERhdGEAD/////8AAAAAAAEAKgEB" +
           "JQAAABAAAABSZXF1ZXN0RGF0YVR5cGVzAQECCAEAAAABAAAAAAAAAAABACoBASYAAAARAAAAUmVzcG9u" +
           "c2VEYXRhVHlwZXMBAQIIAQAAAAEAAAAAAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIA" +
           "AAAAAA8AAABPdXRwdXRBcmd1bWVudHMBAZ0IAC4ARJ0IAACWAwAAAAEAKgEBGwAAAAwAAABSZXNwb25z" +
           "ZURhdGEAD/////8AAAAAAAEAKgEBHQAAAA4AAABSRVNQT05TRV9DT0RFUwAP/////wAAAAAAAQAqAQEb" +
           "AAAADAAAAFNlcnZpY2VFcnJvcgAb/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAA1YIkK" +
           "AgAAAAIADAAAAE1hbnVmYWN0dXJlcgEBbggDAAAAADAAAABOYW1lIG9mIHRoZSBjb21wYW55IHRoYXQg" +
           "bWFudWZhY3R1cmVkIHRoZSBkZXZpY2UALgBEbggAAAAV/////wEB/////wAAAAA1YIkKAgAAAAIABQAA" +
           "AE1vZGVsAQFvCAMAAAAAGAAAAE1vZGVsIG5hbWUgb2YgdGhlIGRldmljZQAuAERvCAAAABX/////AQH/" +
           "////AAAAADVgiQoCAAAAAgAQAAAASGFyZHdhcmVSZXZpc2lvbgEBcwgDAAAAACwAAABSZXZpc2lvbiBs" +
           "ZXZlbCBvZiB0aGUgaGFyZHdhcmUgb2YgdGhlIGRldmljZQAuAERzCAAAAAz/////AQH/////AAAAADVg" +
           "iQoCAAAAAgAQAAAAU29mdHdhcmVSZXZpc2lvbgEBcggDAAAAADUAAABSZXZpc2lvbiBsZXZlbCBvZiB0" +
           "aGUgc29mdHdhcmUvZmlybXdhcmUgb2YgdGhlIGRldmljZQAuAERyCAAAAAz/////AQH/////AAAAADVg" +
           "iQoCAAAAAgAOAAAARGV2aWNlUmV2aXNpb24BAXEIAwAAAAAkAAAAT3ZlcmFsbCByZXZpc2lvbiBsZXZl" +
           "bCBvZiB0aGUgZGV2aWNlAC4ARHEIAAAADP////8BAf////8AAAAANWCJCgIAAAACAAwAAABEZXZpY2VN" +
           "YW51YWwBAXAIAwAAAABaAAAAQWRkcmVzcyAocGF0aG5hbWUgaW4gdGhlIGZpbGUgc3lzdGVtIG9yIGEg" +
           "VVJMIHwgV2ViIGFkZHJlc3MpIG9mIHVzZXIgbWFudWFsIGZvciB0aGUgZGV2aWNlAC4ARHAIAAAADP//" +
           "//8BAf////8AAAAANWCJCgIAAAACAAwAAABTZXJpYWxOdW1iZXIBAWwIAwAAAABNAAAASWRlbnRpZmll" +
           "ciB0aGF0IHVuaXF1ZWx5IGlkZW50aWZpZXMsIHdpdGhpbiBhIG1hbnVmYWN0dXJlciwgYSBkZXZpY2Ug" +
           "aW5zdGFuY2UALgBEbAgAAAAM/////wEB/////wAAAAA1YIkKAgAAAAIADwAAAFJldmlzaW9uQ291bnRl" +
           "cgEBbQgDAAAAAGkAAABBbiBpbmNyZW1lbnRhbCBjb3VudGVyIGluZGljYXRpbmcgdGhlIG51bWJlciBv" +
           "ZiB0aW1lcyB0aGUgc3RhdGljIGRhdGEgd2l0aGluIHRoZSBEZXZpY2UgaGFzIGJlZW4gbW9kaWZpZWQA" +
           "LgBEbQgAAAAG/////wEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Public Properties
        #endregion

        #region Overridden Methods
        #endregion

        #region Private Fields
        #endregion
    }
    #endif
    #endregion
}
