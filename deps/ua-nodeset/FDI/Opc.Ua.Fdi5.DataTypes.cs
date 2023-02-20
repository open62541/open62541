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
using System.Text;
using System.Xml;
using System.Runtime.Serialization;
using Opc.Ua.Di;
using Opc.Ua;

namespace Opc.Ua.Fdi5
{
    #region WindowModeType Enumeration
    #if (!OPCUA_EXCLUDE_WindowModeType)
    /// <summary>
    /// 
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [DataContract(Namespace = Opc.Ua.Fdi5.Namespaces.OpcUaFdi5Xsd)]
    public enum WindowModeType
    {
        /// <remarks />
        [EnumMember(Value = "ModalWindow_1")]
        ModalWindow = 1,

        /// <remarks />
        [EnumMember(Value = "NonModalWindow_2")]
        NonModalWindow = 2,

        /// <remarks />
        [EnumMember(Value = "UIP_3")]
        UIP = 3,
    }

    #region WindowModeTypeCollection Class
    /// <summary>
    /// A collection of WindowModeType objects.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [CollectionDataContract(Name = "ListOfWindowModeType", Namespace = Opc.Ua.Fdi5.Namespaces.OpcUaFdi5Xsd, ItemName = "WindowModeType")]
    #if !NET_STANDARD
    public partial class WindowModeTypeCollection : List<WindowModeType>, ICloneable
    #else
    public partial class WindowModeTypeCollection : List<WindowModeType>
    #endif
    {
        #region Constructors
        /// <summary>
        /// Initializes the collection with default values.
        /// </summary>
        public WindowModeTypeCollection() {}

        /// <summary>
        /// Initializes the collection with an initial capacity.
        /// </summary>
        public WindowModeTypeCollection(int capacity) : base(capacity) {}

        /// <summary>
        /// Initializes the collection with another collection.
        /// </summary>
        public WindowModeTypeCollection(IEnumerable<WindowModeType> collection) : base(collection) {}
        #endregion

        #region Static Operators
        /// <summary>
        /// Converts an array to a collection.
        /// </summary>
        public static implicit operator WindowModeTypeCollection(WindowModeType[] values)
        {
            if (values != null)
            {
                return new WindowModeTypeCollection(values);
            }

            return new WindowModeTypeCollection();
        }

        /// <summary>
        /// Converts a collection to an array.
        /// </summary>
        public static explicit operator WindowModeType[](WindowModeTypeCollection values)
        {
            if (values != null)
            {
                return values.ToArray();
            }

            return null;
        }
        #endregion

        #if !NET_STANDARD
        #region ICloneable Methods
        /// <summary>
        /// Creates a deep copy of the collection.
        /// </summary>
        public object Clone()
        {
            return (WindowModeTypeCollection)this.MemberwiseClone();
        }
        #endregion
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            WindowModeTypeCollection clone = new WindowModeTypeCollection(this.Count);

            for (int ii = 0; ii < this.Count; ii++)
            {
                clone.Add((WindowModeType)Utils.Clone(this[ii]));
            }

            return clone;
        }
    }
    #endregion
    #endif
    #endregion

    #region StyleType Enumeration
    #if (!OPCUA_EXCLUDE_StyleType)
    /// <summary>
    /// 
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [DataContract(Namespace = Opc.Ua.Fdi5.Namespaces.OpcUaFdi5Xsd)]
    public enum StyleType
    {
        /// <remarks />
        [EnumMember(Value = "Window_1")]
        Window = 1,

        /// <remarks />
        [EnumMember(Value = "Dialog_2")]
        Dialog = 2,
    }

    #region StyleTypeCollection Class
    /// <summary>
    /// A collection of StyleType objects.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [CollectionDataContract(Name = "ListOfStyleType", Namespace = Opc.Ua.Fdi5.Namespaces.OpcUaFdi5Xsd, ItemName = "StyleType")]
    #if !NET_STANDARD
    public partial class StyleTypeCollection : List<StyleType>, ICloneable
    #else
    public partial class StyleTypeCollection : List<StyleType>
    #endif
    {
        #region Constructors
        /// <summary>
        /// Initializes the collection with default values.
        /// </summary>
        public StyleTypeCollection() {}

        /// <summary>
        /// Initializes the collection with an initial capacity.
        /// </summary>
        public StyleTypeCollection(int capacity) : base(capacity) {}

        /// <summary>
        /// Initializes the collection with another collection.
        /// </summary>
        public StyleTypeCollection(IEnumerable<StyleType> collection) : base(collection) {}
        #endregion

        #region Static Operators
        /// <summary>
        /// Converts an array to a collection.
        /// </summary>
        public static implicit operator StyleTypeCollection(StyleType[] values)
        {
            if (values != null)
            {
                return new StyleTypeCollection(values);
            }

            return new StyleTypeCollection();
        }

        /// <summary>
        /// Converts a collection to an array.
        /// </summary>
        public static explicit operator StyleType[](StyleTypeCollection values)
        {
            if (values != null)
            {
                return values.ToArray();
            }

            return null;
        }
        #endregion

        #if !NET_STANDARD
        #region ICloneable Methods
        /// <summary>
        /// Creates a deep copy of the collection.
        /// </summary>
        public object Clone()
        {
            return (StyleTypeCollection)this.MemberwiseClone();
        }
        #endregion
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            StyleTypeCollection clone = new StyleTypeCollection(this.Count);

            for (int ii = 0; ii < this.Count; ii++)
            {
                clone.Add((StyleType)Utils.Clone(this[ii]));
            }

            return clone;
        }
    }
    #endregion
    #endif
    #endregion

    #region RegistrationParameters Class
    #if (!OPCUA_EXCLUDE_RegistrationParameters)
    /// <summary>
    /// 
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [DataContract(Namespace = Opc.Ua.Fdi5.Namespaces.OpcUaFdi5Xsd)]
    public partial class RegistrationParameters : IEncodeable
    {
        #region Constructors
        /// <summary>
        /// The default constructor.
        /// </summary>
        public RegistrationParameters()
        {
            Initialize();
        }

        /// <summary>
        /// Called by the .NET framework during deserialization.
        /// </summary>
        [OnDeserializing]
        private void Initialize(StreamingContext context)
        {
            Initialize();
        }

        /// <summary>
        /// Sets private members to default values.
        /// </summary>
        private void Initialize()
        {
            m_path = new RelativePath();
            m_selectionFlags = (uint)0;
        }
        #endregion

        #region Public Properties
        /// <summary>
        /// 
        /// </summary>
        [DataMember(Name = "Path", IsRequired = false, Order = 1)]
        public RelativePath Path
        {
            get
            {
                return m_path;
            }

            set
            {
                m_path = value;

                if (value == null)
                {
                    m_path = new RelativePath();
                }
            }
        }

        /// <remarks />
        [DataMember(Name = "SelectionFlags", IsRequired = false, Order = 2)]
        public uint SelectionFlags
        {
            get { return m_selectionFlags;  }
            set { m_selectionFlags = value; }
        }
        #endregion

        #region IEncodeable Members
        /// <summary cref="IEncodeable.TypeId" />
        public virtual ExpandedNodeId TypeId
        {
            get { return DataTypeIds.RegistrationParameters; }
        }

        /// <summary cref="IEncodeable.BinaryEncodingId" />
        public virtual ExpandedNodeId BinaryEncodingId
        {
            get { return ObjectIds.RegistrationParameters_Encoding_DefaultBinary; }
        }

        /// <summary cref="IEncodeable.XmlEncodingId" />
        public virtual ExpandedNodeId XmlEncodingId
        {
            get { return ObjectIds.RegistrationParameters_Encoding_DefaultXml; }
        }

        /// <summary cref="IEncodeable.Encode(IEncoder)" />
        public virtual void Encode(IEncoder encoder)
        {
            encoder.PushNamespace(Opc.Ua.Fdi5.Namespaces.OpcUaFdi5Xsd);

            encoder.WriteEncodeable("Path", Path, typeof(RelativePath));
            encoder.WriteUInt32("SelectionFlags", SelectionFlags);

            encoder.PopNamespace();
        }

        /// <summary cref="IEncodeable.Decode(IDecoder)" />
        public virtual void Decode(IDecoder decoder)
        {
            decoder.PushNamespace(Opc.Ua.Fdi5.Namespaces.OpcUaFdi5Xsd);

            Path = (RelativePath)decoder.ReadEncodeable("Path", typeof(RelativePath));
            SelectionFlags = decoder.ReadUInt32("SelectionFlags");

            decoder.PopNamespace();
        }

        /// <summary cref="IEncodeable.IsEqual(IEncodeable)" />
        public virtual bool IsEqual(IEncodeable encodeable)
        {
            if (Object.ReferenceEquals(this, encodeable))
            {
                return true;
            }

            RegistrationParameters value = encodeable as RegistrationParameters;

            if (value == null)
            {
                return false;
            }

            if (!Utils.IsEqual(m_path, value.m_path)) return false;
            if (!Utils.IsEqual(m_selectionFlags, value.m_selectionFlags)) return false;

            return true;
        }

        #if !NET_STANDARD
        /// <summary cref="ICloneable.Clone" />
        public virtual object Clone()
        {
            return (RegistrationParameters)this.MemberwiseClone();
        }
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            RegistrationParameters clone = (RegistrationParameters)base.MemberwiseClone();

            clone.m_path = (RelativePath)Utils.Clone(this.m_path);
            clone.m_selectionFlags = (uint)Utils.Clone(this.m_selectionFlags);

            return clone;
        }
        #endregion

        #region Private Fields
        private RelativePath m_path;
        private uint m_selectionFlags;
        #endregion
    }

    #region RegistrationParametersCollection Class
    /// <summary>
    /// A collection of RegistrationParameters objects.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [CollectionDataContract(Name = "ListOfRegistrationParameters", Namespace = Opc.Ua.Fdi5.Namespaces.OpcUaFdi5Xsd, ItemName = "RegistrationParameters")]
    #if !NET_STANDARD
    public partial class RegistrationParametersCollection : List<RegistrationParameters>, ICloneable
    #else
    public partial class RegistrationParametersCollection : List<RegistrationParameters>
    #endif
    {
        #region Constructors
        /// <summary>
        /// Initializes the collection with default values.
        /// </summary>
        public RegistrationParametersCollection() {}

        /// <summary>
        /// Initializes the collection with an initial capacity.
        /// </summary>
        public RegistrationParametersCollection(int capacity) : base(capacity) {}

        /// <summary>
        /// Initializes the collection with another collection.
        /// </summary>
        public RegistrationParametersCollection(IEnumerable<RegistrationParameters> collection) : base(collection) {}
        #endregion

        #region Static Operators
        /// <summary>
        /// Converts an array to a collection.
        /// </summary>
        public static implicit operator RegistrationParametersCollection(RegistrationParameters[] values)
        {
            if (values != null)
            {
                return new RegistrationParametersCollection(values);
            }

            return new RegistrationParametersCollection();
        }

        /// <summary>
        /// Converts a collection to an array.
        /// </summary>
        public static explicit operator RegistrationParameters[](RegistrationParametersCollection values)
        {
            if (values != null)
            {
                return values.ToArray();
            }

            return null;
        }
        #endregion

        #if !NET_STANDARD
        #region ICloneable Methods
        /// <summary>
        /// Creates a deep copy of the collection.
        /// </summary>
        public object Clone()
        {
            return (RegistrationParametersCollection)this.MemberwiseClone();
        }
        #endregion
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            RegistrationParametersCollection clone = new RegistrationParametersCollection(this.Count);

            for (int ii = 0; ii < this.Count; ii++)
            {
                clone.Add((RegistrationParameters)Utils.Clone(this[ii]));
            }

            return clone;
        }
    }
    #endregion
    #endif
    #endregion

    #region RegisteredNode Class
    #if (!OPCUA_EXCLUDE_RegisteredNode)
    /// <summary>
    /// 
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [DataContract(Namespace = Opc.Ua.Fdi5.Namespaces.OpcUaFdi5Xsd)]
    public partial class RegisteredNode : IEncodeable
    {
        #region Constructors
        /// <summary>
        /// The default constructor.
        /// </summary>
        public RegisteredNode()
        {
            Initialize();
        }

        /// <summary>
        /// Called by the .NET framework during deserialization.
        /// </summary>
        [OnDeserializing]
        private void Initialize(StreamingContext context)
        {
            Initialize();
        }

        /// <summary>
        /// Sets private members to default values.
        /// </summary>
        private void Initialize()
        {
            m_nodeStatus = (int)0;
            m_onlineContextNodeId = null;
            m_onlineDeviceNodeId = null;
            m_offlineContextNodeId = null;
            m_offlineDeviceNodeId = null;
        }
        #endregion

        #region Public Properties
        /// <remarks />
        [DataMember(Name = "NodeStatus", IsRequired = false, Order = 1)]
        public int NodeStatus
        {
            get { return m_nodeStatus;  }
            set { m_nodeStatus = value; }
        }

        /// <remarks />
        [DataMember(Name = "OnlineContextNodeId", IsRequired = false, Order = 2)]
        public NodeId OnlineContextNodeId
        {
            get { return m_onlineContextNodeId;  }
            set { m_onlineContextNodeId = value; }
        }

        /// <remarks />
        [DataMember(Name = "OnlineDeviceNodeId", IsRequired = false, Order = 3)]
        public NodeId OnlineDeviceNodeId
        {
            get { return m_onlineDeviceNodeId;  }
            set { m_onlineDeviceNodeId = value; }
        }

        /// <remarks />
        [DataMember(Name = "OfflineContextNodeId", IsRequired = false, Order = 4)]
        public NodeId OfflineContextNodeId
        {
            get { return m_offlineContextNodeId;  }
            set { m_offlineContextNodeId = value; }
        }

        /// <remarks />
        [DataMember(Name = "OfflineDeviceNodeId", IsRequired = false, Order = 5)]
        public NodeId OfflineDeviceNodeId
        {
            get { return m_offlineDeviceNodeId;  }
            set { m_offlineDeviceNodeId = value; }
        }
        #endregion

        #region IEncodeable Members
        /// <summary cref="IEncodeable.TypeId" />
        public virtual ExpandedNodeId TypeId
        {
            get { return DataTypeIds.RegisteredNode; }
        }

        /// <summary cref="IEncodeable.BinaryEncodingId" />
        public virtual ExpandedNodeId BinaryEncodingId
        {
            get { return ObjectIds.RegisteredNode_Encoding_DefaultBinary; }
        }

        /// <summary cref="IEncodeable.XmlEncodingId" />
        public virtual ExpandedNodeId XmlEncodingId
        {
            get { return ObjectIds.RegisteredNode_Encoding_DefaultXml; }
        }

        /// <summary cref="IEncodeable.Encode(IEncoder)" />
        public virtual void Encode(IEncoder encoder)
        {
            encoder.PushNamespace(Opc.Ua.Fdi5.Namespaces.OpcUaFdi5Xsd);

            encoder.WriteInt32("NodeStatus", NodeStatus);
            encoder.WriteNodeId("OnlineContextNodeId", OnlineContextNodeId);
            encoder.WriteNodeId("OnlineDeviceNodeId", OnlineDeviceNodeId);
            encoder.WriteNodeId("OfflineContextNodeId", OfflineContextNodeId);
            encoder.WriteNodeId("OfflineDeviceNodeId", OfflineDeviceNodeId);

            encoder.PopNamespace();
        }

        /// <summary cref="IEncodeable.Decode(IDecoder)" />
        public virtual void Decode(IDecoder decoder)
        {
            decoder.PushNamespace(Opc.Ua.Fdi5.Namespaces.OpcUaFdi5Xsd);

            NodeStatus = decoder.ReadInt32("NodeStatus");
            OnlineContextNodeId = decoder.ReadNodeId("OnlineContextNodeId");
            OnlineDeviceNodeId = decoder.ReadNodeId("OnlineDeviceNodeId");
            OfflineContextNodeId = decoder.ReadNodeId("OfflineContextNodeId");
            OfflineDeviceNodeId = decoder.ReadNodeId("OfflineDeviceNodeId");

            decoder.PopNamespace();
        }

        /// <summary cref="IEncodeable.IsEqual(IEncodeable)" />
        public virtual bool IsEqual(IEncodeable encodeable)
        {
            if (Object.ReferenceEquals(this, encodeable))
            {
                return true;
            }

            RegisteredNode value = encodeable as RegisteredNode;

            if (value == null)
            {
                return false;
            }

            if (!Utils.IsEqual(m_nodeStatus, value.m_nodeStatus)) return false;
            if (!Utils.IsEqual(m_onlineContextNodeId, value.m_onlineContextNodeId)) return false;
            if (!Utils.IsEqual(m_onlineDeviceNodeId, value.m_onlineDeviceNodeId)) return false;
            if (!Utils.IsEqual(m_offlineContextNodeId, value.m_offlineContextNodeId)) return false;
            if (!Utils.IsEqual(m_offlineDeviceNodeId, value.m_offlineDeviceNodeId)) return false;

            return true;
        }

        #if !NET_STANDARD
        /// <summary cref="ICloneable.Clone" />
        public virtual object Clone()
        {
            return (RegisteredNode)this.MemberwiseClone();
        }
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            RegisteredNode clone = (RegisteredNode)base.MemberwiseClone();

            clone.m_nodeStatus = (int)Utils.Clone(this.m_nodeStatus);
            clone.m_onlineContextNodeId = (NodeId)Utils.Clone(this.m_onlineContextNodeId);
            clone.m_onlineDeviceNodeId = (NodeId)Utils.Clone(this.m_onlineDeviceNodeId);
            clone.m_offlineContextNodeId = (NodeId)Utils.Clone(this.m_offlineContextNodeId);
            clone.m_offlineDeviceNodeId = (NodeId)Utils.Clone(this.m_offlineDeviceNodeId);

            return clone;
        }
        #endregion

        #region Private Fields
        private int m_nodeStatus;
        private NodeId m_onlineContextNodeId;
        private NodeId m_onlineDeviceNodeId;
        private NodeId m_offlineContextNodeId;
        private NodeId m_offlineDeviceNodeId;
        #endregion
    }

    #region RegisteredNodeCollection Class
    /// <summary>
    /// A collection of RegisteredNode objects.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [CollectionDataContract(Name = "ListOfRegisteredNode", Namespace = Opc.Ua.Fdi5.Namespaces.OpcUaFdi5Xsd, ItemName = "RegisteredNode")]
    #if !NET_STANDARD
    public partial class RegisteredNodeCollection : List<RegisteredNode>, ICloneable
    #else
    public partial class RegisteredNodeCollection : List<RegisteredNode>
    #endif
    {
        #region Constructors
        /// <summary>
        /// Initializes the collection with default values.
        /// </summary>
        public RegisteredNodeCollection() {}

        /// <summary>
        /// Initializes the collection with an initial capacity.
        /// </summary>
        public RegisteredNodeCollection(int capacity) : base(capacity) {}

        /// <summary>
        /// Initializes the collection with another collection.
        /// </summary>
        public RegisteredNodeCollection(IEnumerable<RegisteredNode> collection) : base(collection) {}
        #endregion

        #region Static Operators
        /// <summary>
        /// Converts an array to a collection.
        /// </summary>
        public static implicit operator RegisteredNodeCollection(RegisteredNode[] values)
        {
            if (values != null)
            {
                return new RegisteredNodeCollection(values);
            }

            return new RegisteredNodeCollection();
        }

        /// <summary>
        /// Converts a collection to an array.
        /// </summary>
        public static explicit operator RegisteredNode[](RegisteredNodeCollection values)
        {
            if (values != null)
            {
                return values.ToArray();
            }

            return null;
        }
        #endregion

        #if !NET_STANDARD
        #region ICloneable Methods
        /// <summary>
        /// Creates a deep copy of the collection.
        /// </summary>
        public object Clone()
        {
            return (RegisteredNodeCollection)this.MemberwiseClone();
        }
        #endregion
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            RegisteredNodeCollection clone = new RegisteredNodeCollection(this.Count);

            for (int ii = 0; ii < this.Count; ii++)
            {
                clone.Add((RegisteredNode)Utils.Clone(this[ii]));
            }

            return clone;
        }
    }
    #endregion
    #endif
    #endregion

    #region RegisterNodesResult Class
    #if (!OPCUA_EXCLUDE_RegisterNodesResult)
    /// <summary>
    /// 
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [DataContract(Namespace = Opc.Ua.Fdi5.Namespaces.OpcUaFdi5Xsd)]
    public partial class RegisterNodesResult : IEncodeable
    {
        #region Constructors
        /// <summary>
        /// The default constructor.
        /// </summary>
        public RegisterNodesResult()
        {
            Initialize();
        }

        /// <summary>
        /// Called by the .NET framework during deserialization.
        /// </summary>
        [OnDeserializing]
        private void Initialize(StreamingContext context)
        {
            Initialize();
        }

        /// <summary>
        /// Sets private members to default values.
        /// </summary>
        private void Initialize()
        {
            m_status = (int)0;
            m_registeredNodes = new RegisteredNodeCollection();
        }
        #endregion

        #region Public Properties
        /// <remarks />
        [DataMember(Name = "Status", IsRequired = false, Order = 1)]
        public int Status
        {
            get { return m_status;  }
            set { m_status = value; }
        }

        /// <summary>
        /// 
        /// </summary>
        [DataMember(Name = "RegisteredNodes", IsRequired = false, Order = 2)]
        public RegisteredNodeCollection RegisteredNodes
        {
            get
            {
                return m_registeredNodes;
            }

            set
            {
                m_registeredNodes = value;

                if (value == null)
                {
                    m_registeredNodes = new RegisteredNodeCollection();
                }
            }
        }
        #endregion

        #region IEncodeable Members
        /// <summary cref="IEncodeable.TypeId" />
        public virtual ExpandedNodeId TypeId
        {
            get { return DataTypeIds.RegisterNodesResult; }
        }

        /// <summary cref="IEncodeable.BinaryEncodingId" />
        public virtual ExpandedNodeId BinaryEncodingId
        {
            get { return ObjectIds.RegisterNodesResult_Encoding_DefaultBinary; }
        }

        /// <summary cref="IEncodeable.XmlEncodingId" />
        public virtual ExpandedNodeId XmlEncodingId
        {
            get { return ObjectIds.RegisterNodesResult_Encoding_DefaultXml; }
        }

        /// <summary cref="IEncodeable.Encode(IEncoder)" />
        public virtual void Encode(IEncoder encoder)
        {
            encoder.PushNamespace(Opc.Ua.Fdi5.Namespaces.OpcUaFdi5Xsd);

            encoder.WriteInt32("Status", Status);
            encoder.WriteEncodeableArray("RegisteredNodes", RegisteredNodes.ToArray(), typeof(RegisteredNode));

            encoder.PopNamespace();
        }

        /// <summary cref="IEncodeable.Decode(IDecoder)" />
        public virtual void Decode(IDecoder decoder)
        {
            decoder.PushNamespace(Opc.Ua.Fdi5.Namespaces.OpcUaFdi5Xsd);

            Status = decoder.ReadInt32("Status");
            RegisteredNodes = (RegisteredNodeCollection)decoder.ReadEncodeableArray("RegisteredNodes", typeof(RegisteredNode));

            decoder.PopNamespace();
        }

        /// <summary cref="IEncodeable.IsEqual(IEncodeable)" />
        public virtual bool IsEqual(IEncodeable encodeable)
        {
            if (Object.ReferenceEquals(this, encodeable))
            {
                return true;
            }

            RegisterNodesResult value = encodeable as RegisterNodesResult;

            if (value == null)
            {
                return false;
            }

            if (!Utils.IsEqual(m_status, value.m_status)) return false;
            if (!Utils.IsEqual(m_registeredNodes, value.m_registeredNodes)) return false;

            return true;
        }

        #if !NET_STANDARD
        /// <summary cref="ICloneable.Clone" />
        public virtual object Clone()
        {
            return (RegisterNodesResult)this.MemberwiseClone();
        }
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            RegisterNodesResult clone = (RegisterNodesResult)base.MemberwiseClone();

            clone.m_status = (int)Utils.Clone(this.m_status);
            clone.m_registeredNodes = (RegisteredNodeCollection)Utils.Clone(this.m_registeredNodes);

            return clone;
        }
        #endregion

        #region Private Fields
        private int m_status;
        private RegisteredNodeCollection m_registeredNodes;
        #endregion
    }

    #region RegisterNodesResultCollection Class
    /// <summary>
    /// A collection of RegisterNodesResult objects.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [CollectionDataContract(Name = "ListOfRegisterNodesResult", Namespace = Opc.Ua.Fdi5.Namespaces.OpcUaFdi5Xsd, ItemName = "RegisterNodesResult")]
    #if !NET_STANDARD
    public partial class RegisterNodesResultCollection : List<RegisterNodesResult>, ICloneable
    #else
    public partial class RegisterNodesResultCollection : List<RegisterNodesResult>
    #endif
    {
        #region Constructors
        /// <summary>
        /// Initializes the collection with default values.
        /// </summary>
        public RegisterNodesResultCollection() {}

        /// <summary>
        /// Initializes the collection with an initial capacity.
        /// </summary>
        public RegisterNodesResultCollection(int capacity) : base(capacity) {}

        /// <summary>
        /// Initializes the collection with another collection.
        /// </summary>
        public RegisterNodesResultCollection(IEnumerable<RegisterNodesResult> collection) : base(collection) {}
        #endregion

        #region Static Operators
        /// <summary>
        /// Converts an array to a collection.
        /// </summary>
        public static implicit operator RegisterNodesResultCollection(RegisterNodesResult[] values)
        {
            if (values != null)
            {
                return new RegisterNodesResultCollection(values);
            }

            return new RegisterNodesResultCollection();
        }

        /// <summary>
        /// Converts a collection to an array.
        /// </summary>
        public static explicit operator RegisterNodesResult[](RegisterNodesResultCollection values)
        {
            if (values != null)
            {
                return values.ToArray();
            }

            return null;
        }
        #endregion

        #if !NET_STANDARD
        #region ICloneable Methods
        /// <summary>
        /// Creates a deep copy of the collection.
        /// </summary>
        public object Clone()
        {
            return (RegisterNodesResultCollection)this.MemberwiseClone();
        }
        #endregion
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            RegisterNodesResultCollection clone = new RegisterNodesResultCollection(this.Count);

            for (int ii = 0; ii < this.Count; ii++)
            {
                clone.Add((RegisterNodesResult)Utils.Clone(this[ii]));
            }

            return clone;
        }
    }
    #endregion
    #endif
    #endregion

    #region TransferIncident Class
    #if (!OPCUA_EXCLUDE_TransferIncident)
    /// <summary>
    /// 
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [DataContract(Namespace = Opc.Ua.Fdi5.Namespaces.OpcUaFdi5Xsd)]
    public partial class TransferIncident : IEncodeable
    {
        #region Constructors
        /// <summary>
        /// The default constructor.
        /// </summary>
        public TransferIncident()
        {
            Initialize();
        }

        /// <summary>
        /// Called by the .NET framework during deserialization.
        /// </summary>
        [OnDeserializing]
        private void Initialize(StreamingContext context)
        {
            Initialize();
        }

        /// <summary>
        /// Sets private members to default values.
        /// </summary>
        private void Initialize()
        {
            m_contextNodeId = null;
            m_statusCode = StatusCodes.Good;
            m_diagnostics = null;
        }
        #endregion

        #region Public Properties
        /// <remarks />
        [DataMember(Name = "ContextNodeId", IsRequired = false, Order = 1)]
        public NodeId ContextNodeId
        {
            get { return m_contextNodeId;  }
            set { m_contextNodeId = value; }
        }

        /// <remarks />
        [DataMember(Name = "StatusCode", IsRequired = false, Order = 2)]
        public StatusCode StatusCode
        {
            get { return m_statusCode;  }
            set { m_statusCode = value; }
        }

        /// <remarks />
        [DataMember(Name = "Diagnostics", IsRequired = false, Order = 3)]
        public DiagnosticInfo Diagnostics
        {
            get { return m_diagnostics;  }
            set { m_diagnostics = value; }
        }
        #endregion

        #region IEncodeable Members
        /// <summary cref="IEncodeable.TypeId" />
        public virtual ExpandedNodeId TypeId
        {
            get { return DataTypeIds.TransferIncident; }
        }

        /// <summary cref="IEncodeable.BinaryEncodingId" />
        public virtual ExpandedNodeId BinaryEncodingId
        {
            get { return ObjectIds.TransferIncident_Encoding_DefaultBinary; }
        }

        /// <summary cref="IEncodeable.XmlEncodingId" />
        public virtual ExpandedNodeId XmlEncodingId
        {
            get { return ObjectIds.TransferIncident_Encoding_DefaultXml; }
        }

        /// <summary cref="IEncodeable.Encode(IEncoder)" />
        public virtual void Encode(IEncoder encoder)
        {
            encoder.PushNamespace(Opc.Ua.Fdi5.Namespaces.OpcUaFdi5Xsd);

            encoder.WriteNodeId("ContextNodeId", ContextNodeId);
            encoder.WriteStatusCode("StatusCode", StatusCode);
            encoder.WriteDiagnosticInfo("Diagnostics", Diagnostics);

            encoder.PopNamespace();
        }

        /// <summary cref="IEncodeable.Decode(IDecoder)" />
        public virtual void Decode(IDecoder decoder)
        {
            decoder.PushNamespace(Opc.Ua.Fdi5.Namespaces.OpcUaFdi5Xsd);

            ContextNodeId = decoder.ReadNodeId("ContextNodeId");
            StatusCode = decoder.ReadStatusCode("StatusCode");
            Diagnostics = decoder.ReadDiagnosticInfo("Diagnostics");

            decoder.PopNamespace();
        }

        /// <summary cref="IEncodeable.IsEqual(IEncodeable)" />
        public virtual bool IsEqual(IEncodeable encodeable)
        {
            if (Object.ReferenceEquals(this, encodeable))
            {
                return true;
            }

            TransferIncident value = encodeable as TransferIncident;

            if (value == null)
            {
                return false;
            }

            if (!Utils.IsEqual(m_contextNodeId, value.m_contextNodeId)) return false;
            if (!Utils.IsEqual(m_statusCode, value.m_statusCode)) return false;
            if (!Utils.IsEqual(m_diagnostics, value.m_diagnostics)) return false;

            return true;
        }

        #if !NET_STANDARD
        /// <summary cref="ICloneable.Clone" />
        public virtual object Clone()
        {
            return (TransferIncident)this.MemberwiseClone();
        }
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            TransferIncident clone = (TransferIncident)base.MemberwiseClone();

            clone.m_contextNodeId = (NodeId)Utils.Clone(this.m_contextNodeId);
            clone.m_statusCode = (StatusCode)Utils.Clone(this.m_statusCode);
            clone.m_diagnostics = (DiagnosticInfo)Utils.Clone(this.m_diagnostics);

            return clone;
        }
        #endregion

        #region Private Fields
        private NodeId m_contextNodeId;
        private StatusCode m_statusCode;
        private DiagnosticInfo m_diagnostics;
        #endregion
    }

    #region TransferIncidentCollection Class
    /// <summary>
    /// A collection of TransferIncident objects.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [CollectionDataContract(Name = "ListOfTransferIncident", Namespace = Opc.Ua.Fdi5.Namespaces.OpcUaFdi5Xsd, ItemName = "TransferIncident")]
    #if !NET_STANDARD
    public partial class TransferIncidentCollection : List<TransferIncident>, ICloneable
    #else
    public partial class TransferIncidentCollection : List<TransferIncident>
    #endif
    {
        #region Constructors
        /// <summary>
        /// Initializes the collection with default values.
        /// </summary>
        public TransferIncidentCollection() {}

        /// <summary>
        /// Initializes the collection with an initial capacity.
        /// </summary>
        public TransferIncidentCollection(int capacity) : base(capacity) {}

        /// <summary>
        /// Initializes the collection with another collection.
        /// </summary>
        public TransferIncidentCollection(IEnumerable<TransferIncident> collection) : base(collection) {}
        #endregion

        #region Static Operators
        /// <summary>
        /// Converts an array to a collection.
        /// </summary>
        public static implicit operator TransferIncidentCollection(TransferIncident[] values)
        {
            if (values != null)
            {
                return new TransferIncidentCollection(values);
            }

            return new TransferIncidentCollection();
        }

        /// <summary>
        /// Converts a collection to an array.
        /// </summary>
        public static explicit operator TransferIncident[](TransferIncidentCollection values)
        {
            if (values != null)
            {
                return values.ToArray();
            }

            return null;
        }
        #endregion

        #if !NET_STANDARD
        #region ICloneable Methods
        /// <summary>
        /// Creates a deep copy of the collection.
        /// </summary>
        public object Clone()
        {
            return (TransferIncidentCollection)this.MemberwiseClone();
        }
        #endregion
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            TransferIncidentCollection clone = new TransferIncidentCollection(this.Count);

            for (int ii = 0; ii < this.Count; ii++)
            {
                clone.Add((TransferIncident)Utils.Clone(this[ii]));
            }

            return clone;
        }
    }
    #endregion
    #endif
    #endregion

    #region ApplyResult Class
    #if (!OPCUA_EXCLUDE_ApplyResult)
    /// <summary>
    /// 
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [DataContract(Namespace = Opc.Ua.Fdi5.Namespaces.OpcUaFdi5Xsd)]
    public partial class ApplyResult : IEncodeable
    {
        #region Constructors
        /// <summary>
        /// The default constructor.
        /// </summary>
        public ApplyResult()
        {
            Initialize();
        }

        /// <summary>
        /// Called by the .NET framework during deserialization.
        /// </summary>
        [OnDeserializing]
        private void Initialize(StreamingContext context)
        {
            Initialize();
        }

        /// <summary>
        /// Sets private members to default values.
        /// </summary>
        private void Initialize()
        {
            m_status = (int)0;
            m_transferIncidents = new TransferIncidentCollection();
        }
        #endregion

        #region Public Properties
        /// <remarks />
        [DataMember(Name = "Status", IsRequired = false, Order = 1)]
        public int Status
        {
            get { return m_status;  }
            set { m_status = value; }
        }

        /// <summary>
        /// 
        /// </summary>
        [DataMember(Name = "TransferIncidents", IsRequired = false, Order = 2)]
        public TransferIncidentCollection TransferIncidents
        {
            get
            {
                return m_transferIncidents;
            }

            set
            {
                m_transferIncidents = value;

                if (value == null)
                {
                    m_transferIncidents = new TransferIncidentCollection();
                }
            }
        }
        #endregion

        #region IEncodeable Members
        /// <summary cref="IEncodeable.TypeId" />
        public virtual ExpandedNodeId TypeId
        {
            get { return DataTypeIds.ApplyResult; }
        }

        /// <summary cref="IEncodeable.BinaryEncodingId" />
        public virtual ExpandedNodeId BinaryEncodingId
        {
            get { return ObjectIds.ApplyResult_Encoding_DefaultBinary; }
        }

        /// <summary cref="IEncodeable.XmlEncodingId" />
        public virtual ExpandedNodeId XmlEncodingId
        {
            get { return ObjectIds.ApplyResult_Encoding_DefaultXml; }
        }

        /// <summary cref="IEncodeable.Encode(IEncoder)" />
        public virtual void Encode(IEncoder encoder)
        {
            encoder.PushNamespace(Opc.Ua.Fdi5.Namespaces.OpcUaFdi5Xsd);

            encoder.WriteInt32("Status", Status);
            encoder.WriteEncodeableArray("TransferIncidents", TransferIncidents.ToArray(), typeof(TransferIncident));

            encoder.PopNamespace();
        }

        /// <summary cref="IEncodeable.Decode(IDecoder)" />
        public virtual void Decode(IDecoder decoder)
        {
            decoder.PushNamespace(Opc.Ua.Fdi5.Namespaces.OpcUaFdi5Xsd);

            Status = decoder.ReadInt32("Status");
            TransferIncidents = (TransferIncidentCollection)decoder.ReadEncodeableArray("TransferIncidents", typeof(TransferIncident));

            decoder.PopNamespace();
        }

        /// <summary cref="IEncodeable.IsEqual(IEncodeable)" />
        public virtual bool IsEqual(IEncodeable encodeable)
        {
            if (Object.ReferenceEquals(this, encodeable))
            {
                return true;
            }

            ApplyResult value = encodeable as ApplyResult;

            if (value == null)
            {
                return false;
            }

            if (!Utils.IsEqual(m_status, value.m_status)) return false;
            if (!Utils.IsEqual(m_transferIncidents, value.m_transferIncidents)) return false;

            return true;
        }

        #if !NET_STANDARD
        /// <summary cref="ICloneable.Clone" />
        public virtual object Clone()
        {
            return (ApplyResult)this.MemberwiseClone();
        }
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            ApplyResult clone = (ApplyResult)base.MemberwiseClone();

            clone.m_status = (int)Utils.Clone(this.m_status);
            clone.m_transferIncidents = (TransferIncidentCollection)Utils.Clone(this.m_transferIncidents);

            return clone;
        }
        #endregion

        #region Private Fields
        private int m_status;
        private TransferIncidentCollection m_transferIncidents;
        #endregion
    }

    #region ApplyResultCollection Class
    /// <summary>
    /// A collection of ApplyResult objects.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [CollectionDataContract(Name = "ListOfApplyResult", Namespace = Opc.Ua.Fdi5.Namespaces.OpcUaFdi5Xsd, ItemName = "ApplyResult")]
    #if !NET_STANDARD
    public partial class ApplyResultCollection : List<ApplyResult>, ICloneable
    #else
    public partial class ApplyResultCollection : List<ApplyResult>
    #endif
    {
        #region Constructors
        /// <summary>
        /// Initializes the collection with default values.
        /// </summary>
        public ApplyResultCollection() {}

        /// <summary>
        /// Initializes the collection with an initial capacity.
        /// </summary>
        public ApplyResultCollection(int capacity) : base(capacity) {}

        /// <summary>
        /// Initializes the collection with another collection.
        /// </summary>
        public ApplyResultCollection(IEnumerable<ApplyResult> collection) : base(collection) {}
        #endregion

        #region Static Operators
        /// <summary>
        /// Converts an array to a collection.
        /// </summary>
        public static implicit operator ApplyResultCollection(ApplyResult[] values)
        {
            if (values != null)
            {
                return new ApplyResultCollection(values);
            }

            return new ApplyResultCollection();
        }

        /// <summary>
        /// Converts a collection to an array.
        /// </summary>
        public static explicit operator ApplyResult[](ApplyResultCollection values)
        {
            if (values != null)
            {
                return values.ToArray();
            }

            return null;
        }
        #endregion

        #if !NET_STANDARD
        #region ICloneable Methods
        /// <summary>
        /// Creates a deep copy of the collection.
        /// </summary>
        public object Clone()
        {
            return (ApplyResultCollection)this.MemberwiseClone();
        }
        #endregion
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            ApplyResultCollection clone = new ApplyResultCollection(this.Count);

            for (int ii = 0; ii < this.Count; ii++)
            {
                clone.Add((ApplyResult)Utils.Clone(this[ii]));
            }

            return clone;
        }
    }
    #endregion
    #endif
    #endregion
}
