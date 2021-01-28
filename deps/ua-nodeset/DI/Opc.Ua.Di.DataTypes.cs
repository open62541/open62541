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
using Opc.Ua;

namespace Opc.Ua.Di
{
    #region DeviceHealthEnumeration Enumeration
    #if (!OPCUA_EXCLUDE_DeviceHealthEnumeration)
    /// <summary>
    /// 
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [DataContract(Namespace = Opc.Ua.Di.Namespaces.OpcUaDiXsd)]
    public enum DeviceHealthEnumeration
    {
        /// <remarks />
        [EnumMember(Value = "NORMAL_0")]
        NORMAL = 0,

        /// <remarks />
        [EnumMember(Value = "FAILURE_1")]
        FAILURE = 1,

        /// <remarks />
        [EnumMember(Value = "CHECK_FUNCTION_2")]
        CHECK_FUNCTION = 2,

        /// <remarks />
        [EnumMember(Value = "OFF_SPEC_3")]
        OFF_SPEC = 3,

        /// <remarks />
        [EnumMember(Value = "MAINTENANCE_REQUIRED_4")]
        MAINTENANCE_REQUIRED = 4,
    }

    #region DeviceHealthEnumerationCollection Class
    /// <summary>
    /// A collection of DeviceHealthEnumeration objects.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [CollectionDataContract(Name = "ListOfDeviceHealthEnumeration", Namespace = Opc.Ua.Di.Namespaces.OpcUaDiXsd, ItemName = "DeviceHealthEnumeration")]
    #if !NET_STANDARD
    public partial class DeviceHealthEnumerationCollection : List<DeviceHealthEnumeration>, ICloneable
    #else
    public partial class DeviceHealthEnumerationCollection : List<DeviceHealthEnumeration>
    #endif
    {
        #region Constructors
        /// <summary>
        /// Initializes the collection with default values.
        /// </summary>
        public DeviceHealthEnumerationCollection() {}

        /// <summary>
        /// Initializes the collection with an initial capacity.
        /// </summary>
        public DeviceHealthEnumerationCollection(int capacity) : base(capacity) {}

        /// <summary>
        /// Initializes the collection with another collection.
        /// </summary>
        public DeviceHealthEnumerationCollection(IEnumerable<DeviceHealthEnumeration> collection) : base(collection) {}
        #endregion

        #region Static Operators
        /// <summary>
        /// Converts an array to a collection.
        /// </summary>
        public static implicit operator DeviceHealthEnumerationCollection(DeviceHealthEnumeration[] values)
        {
            if (values != null)
            {
                return new DeviceHealthEnumerationCollection(values);
            }

            return new DeviceHealthEnumerationCollection();
        }

        /// <summary>
        /// Converts a collection to an array.
        /// </summary>
        public static explicit operator DeviceHealthEnumeration[](DeviceHealthEnumerationCollection values)
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
            return (DeviceHealthEnumerationCollection)this.MemberwiseClone();
        }
        #endregion
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            DeviceHealthEnumerationCollection clone = new DeviceHealthEnumerationCollection(this.Count);

            for (int ii = 0; ii < this.Count; ii++)
            {
                clone.Add((DeviceHealthEnumeration)Utils.Clone(this[ii]));
            }

            return clone;
        }
    }
    #endregion
    #endif
    #endregion

    #region FetchResultDataType Class
    #if (!OPCUA_EXCLUDE_FetchResultDataType)
    /// <summary>
    /// 
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [DataContract(Namespace = Opc.Ua.Di.Namespaces.OpcUaDiXsd)]
    public partial class FetchResultDataType : IEncodeable
    {
        #region Constructors
        /// <summary>
        /// The default constructor.
        /// </summary>
        public FetchResultDataType()
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
        }
        #endregion

        #region Public Properties
        #endregion

        #region IEncodeable Members
        /// <summary cref="IEncodeable.TypeId" />
        public virtual ExpandedNodeId TypeId
        {
            get { return DataTypeIds.FetchResultDataType; }
        }

        /// <summary cref="IEncodeable.BinaryEncodingId" />
        public virtual ExpandedNodeId BinaryEncodingId
        {
            get { return ObjectIds.FetchResultDataType_Encoding_DefaultBinary; }
        }

        /// <summary cref="IEncodeable.XmlEncodingId" />
        public virtual ExpandedNodeId XmlEncodingId
        {
            get { return ObjectIds.FetchResultDataType_Encoding_DefaultXml; }
        }

        /// <summary cref="IEncodeable.Encode(IEncoder)" />
        public virtual void Encode(IEncoder encoder)
        {
            encoder.PushNamespace(Opc.Ua.Di.Namespaces.OpcUaDiXsd);


            encoder.PopNamespace();
        }

        /// <summary cref="IEncodeable.Decode(IDecoder)" />
        public virtual void Decode(IDecoder decoder)
        {
            decoder.PushNamespace(Opc.Ua.Di.Namespaces.OpcUaDiXsd);


            decoder.PopNamespace();
        }

        /// <summary cref="IEncodeable.IsEqual(IEncodeable)" />
        public virtual bool IsEqual(IEncodeable encodeable)
        {
            if (Object.ReferenceEquals(this, encodeable))
            {
                return true;
            }

            FetchResultDataType value = encodeable as FetchResultDataType;

            if (value == null)
            {
                return false;
            }


            return true;
        }

        #if !NET_STANDARD
        /// <summary cref="ICloneable.Clone" />
        public virtual object Clone()
        {
            return (FetchResultDataType)this.MemberwiseClone();
        }
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            FetchResultDataType clone = (FetchResultDataType)base.MemberwiseClone();


            return clone;
        }
        #endregion

        #region Private Fields
        #endregion
    }

    #region FetchResultDataTypeCollection Class
    /// <summary>
    /// A collection of FetchResultDataType objects.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [CollectionDataContract(Name = "ListOfFetchResultDataType", Namespace = Opc.Ua.Di.Namespaces.OpcUaDiXsd, ItemName = "FetchResultDataType")]
    #if !NET_STANDARD
    public partial class FetchResultDataTypeCollection : List<FetchResultDataType>, ICloneable
    #else
    public partial class FetchResultDataTypeCollection : List<FetchResultDataType>
    #endif
    {
        #region Constructors
        /// <summary>
        /// Initializes the collection with default values.
        /// </summary>
        public FetchResultDataTypeCollection() {}

        /// <summary>
        /// Initializes the collection with an initial capacity.
        /// </summary>
        public FetchResultDataTypeCollection(int capacity) : base(capacity) {}

        /// <summary>
        /// Initializes the collection with another collection.
        /// </summary>
        public FetchResultDataTypeCollection(IEnumerable<FetchResultDataType> collection) : base(collection) {}
        #endregion

        #region Static Operators
        /// <summary>
        /// Converts an array to a collection.
        /// </summary>
        public static implicit operator FetchResultDataTypeCollection(FetchResultDataType[] values)
        {
            if (values != null)
            {
                return new FetchResultDataTypeCollection(values);
            }

            return new FetchResultDataTypeCollection();
        }

        /// <summary>
        /// Converts a collection to an array.
        /// </summary>
        public static explicit operator FetchResultDataType[](FetchResultDataTypeCollection values)
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
            return (FetchResultDataTypeCollection)this.MemberwiseClone();
        }
        #endregion
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            FetchResultDataTypeCollection clone = new FetchResultDataTypeCollection(this.Count);

            for (int ii = 0; ii < this.Count; ii++)
            {
                clone.Add((FetchResultDataType)Utils.Clone(this[ii]));
            }

            return clone;
        }
    }
    #endregion
    #endif
    #endregion

    #region TransferResultErrorDataType Class
    #if (!OPCUA_EXCLUDE_TransferResultErrorDataType)
    /// <summary>
    /// 
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [DataContract(Namespace = Opc.Ua.Di.Namespaces.OpcUaDiXsd)]
    public partial class TransferResultErrorDataType : FetchResultDataType
    {
        #region Constructors
        /// <summary>
        /// The default constructor.
        /// </summary>
        public TransferResultErrorDataType()
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
            m_diagnostics = null;
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

        /// <remarks />
        [DataMember(Name = "Diagnostics", IsRequired = false, Order = 2)]
        public DiagnosticInfo Diagnostics
        {
            get { return m_diagnostics;  }
            set { m_diagnostics = value; }
        }
        #endregion

        #region IEncodeable Members
        /// <summary cref="IEncodeable.TypeId" />
        public override ExpandedNodeId TypeId
        {
            get { return DataTypeIds.TransferResultErrorDataType; }
        }

        /// <summary cref="IEncodeable.BinaryEncodingId" />
        public override ExpandedNodeId BinaryEncodingId
        {
            get { return ObjectIds.TransferResultErrorDataType_Encoding_DefaultBinary; }
        }

        /// <summary cref="IEncodeable.XmlEncodingId" />
        public override ExpandedNodeId XmlEncodingId
        {
            get { return ObjectIds.TransferResultErrorDataType_Encoding_DefaultXml; }
        }

        /// <summary cref="IEncodeable.Encode(IEncoder)" />
        public override void Encode(IEncoder encoder)
        {
            base.Encode(encoder);

            encoder.PushNamespace(Opc.Ua.Di.Namespaces.OpcUaDiXsd);

            encoder.WriteInt32("Status", Status);
            encoder.WriteDiagnosticInfo("Diagnostics", Diagnostics);

            encoder.PopNamespace();
        }

        /// <summary cref="IEncodeable.Decode(IDecoder)" />
        public override void Decode(IDecoder decoder)
        {
            base.Decode(decoder);

            decoder.PushNamespace(Opc.Ua.Di.Namespaces.OpcUaDiXsd);

            Status = decoder.ReadInt32("Status");
            Diagnostics = decoder.ReadDiagnosticInfo("Diagnostics");

            decoder.PopNamespace();
        }

        /// <summary cref="IEncodeable.IsEqual(IEncodeable)" />
        public override bool IsEqual(IEncodeable encodeable)
        {
            if (Object.ReferenceEquals(this, encodeable))
            {
                return true;
            }

            TransferResultErrorDataType value = encodeable as TransferResultErrorDataType;

            if (value == null)
            {
                return false;
            }

            if (!base.IsEqual(encodeable)) return false;
            if (!Utils.IsEqual(m_status, value.m_status)) return false;
            if (!Utils.IsEqual(m_diagnostics, value.m_diagnostics)) return false;

            return true;
        }    

        #if !NET_STANDARD
        /// <summary cref="ICloneable.Clone" />
        public override object Clone()
        {
            return (TransferResultErrorDataType)this.MemberwiseClone();
        }
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            TransferResultErrorDataType clone = (TransferResultErrorDataType)base.MemberwiseClone();

            clone.m_status = (int)Utils.Clone(this.m_status);
            clone.m_diagnostics = (DiagnosticInfo)Utils.Clone(this.m_diagnostics);

            return clone;
        }
        #endregion

        #region Private Fields
        private int m_status;
        private DiagnosticInfo m_diagnostics;
        #endregion
    }

    #region TransferResultErrorDataTypeCollection Class
    /// <summary>
    /// A collection of TransferResultErrorDataType objects.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [CollectionDataContract(Name = "ListOfTransferResultErrorDataType", Namespace = Opc.Ua.Di.Namespaces.OpcUaDiXsd, ItemName = "TransferResultErrorDataType")]
    #if !NET_STANDARD
    public partial class TransferResultErrorDataTypeCollection : List<TransferResultErrorDataType>, ICloneable
    #else
    public partial class TransferResultErrorDataTypeCollection : List<TransferResultErrorDataType>
    #endif
    {
        #region Constructors
        /// <summary>
        /// Initializes the collection with default values.
        /// </summary>
        public TransferResultErrorDataTypeCollection() {}

        /// <summary>
        /// Initializes the collection with an initial capacity.
        /// </summary>
        public TransferResultErrorDataTypeCollection(int capacity) : base(capacity) {}

        /// <summary>
        /// Initializes the collection with another collection.
        /// </summary>
        public TransferResultErrorDataTypeCollection(IEnumerable<TransferResultErrorDataType> collection) : base(collection) {}
        #endregion

        #region Static Operators
        /// <summary>
        /// Converts an array to a collection.
        /// </summary>
        public static implicit operator TransferResultErrorDataTypeCollection(TransferResultErrorDataType[] values)
        {
            if (values != null)
            {
                return new TransferResultErrorDataTypeCollection(values);
            }

            return new TransferResultErrorDataTypeCollection();
        }

        /// <summary>
        /// Converts a collection to an array.
        /// </summary>
        public static explicit operator TransferResultErrorDataType[](TransferResultErrorDataTypeCollection values)
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
            return (TransferResultErrorDataTypeCollection)this.MemberwiseClone();
        }
        #endregion
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            TransferResultErrorDataTypeCollection clone = new TransferResultErrorDataTypeCollection(this.Count);

            for (int ii = 0; ii < this.Count; ii++)
            {
                clone.Add((TransferResultErrorDataType)Utils.Clone(this[ii]));
            }

            return clone;
        }
    }
    #endregion
    #endif
    #endregion

    #region TransferResultDataDataType Class
    #if (!OPCUA_EXCLUDE_TransferResultDataDataType)
    /// <summary>
    /// 
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [DataContract(Namespace = Opc.Ua.Di.Namespaces.OpcUaDiXsd)]
    public partial class TransferResultDataDataType : FetchResultDataType
    {
        #region Constructors
        /// <summary>
        /// The default constructor.
        /// </summary>
        public TransferResultDataDataType()
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
            m_sequenceNumber = (int)0;
            m_endOfResults = true;
            m_parameterDefs = new ParameterResultDataTypeCollection();
        }
        #endregion

        #region Public Properties
        /// <remarks />
        [DataMember(Name = "SequenceNumber", IsRequired = false, Order = 1)]
        public int SequenceNumber
        {
            get { return m_sequenceNumber;  }
            set { m_sequenceNumber = value; }
        }

        /// <remarks />
        [DataMember(Name = "EndOfResults", IsRequired = false, Order = 2)]
        public bool EndOfResults
        {
            get { return m_endOfResults;  }
            set { m_endOfResults = value; }
        }

        /// <summary>
        /// 
        /// </summary>
        [DataMember(Name = "ParameterDefs", IsRequired = false, Order = 3)]
        public ParameterResultDataTypeCollection ParameterDefs
        {
            get
            {
                return m_parameterDefs;
            }

            set
            {
                m_parameterDefs = value;

                if (value == null)
                {
                    m_parameterDefs = new ParameterResultDataTypeCollection();
                }
            }
        }
        #endregion

        #region IEncodeable Members
        /// <summary cref="IEncodeable.TypeId" />
        public override ExpandedNodeId TypeId
        {
            get { return DataTypeIds.TransferResultDataDataType; }
        }

        /// <summary cref="IEncodeable.BinaryEncodingId" />
        public override ExpandedNodeId BinaryEncodingId
        {
            get { return ObjectIds.TransferResultDataDataType_Encoding_DefaultBinary; }
        }

        /// <summary cref="IEncodeable.XmlEncodingId" />
        public override ExpandedNodeId XmlEncodingId
        {
            get { return ObjectIds.TransferResultDataDataType_Encoding_DefaultXml; }
        }

        /// <summary cref="IEncodeable.Encode(IEncoder)" />
        public override void Encode(IEncoder encoder)
        {
            base.Encode(encoder);

            encoder.PushNamespace(Opc.Ua.Di.Namespaces.OpcUaDiXsd);

            encoder.WriteInt32("SequenceNumber", SequenceNumber);
            encoder.WriteBoolean("EndOfResults", EndOfResults);
            encoder.WriteEncodeableArray("ParameterDefs", ParameterDefs.ToArray(), typeof(ParameterResultDataType));

            encoder.PopNamespace();
        }

        /// <summary cref="IEncodeable.Decode(IDecoder)" />
        public override void Decode(IDecoder decoder)
        {
            base.Decode(decoder);

            decoder.PushNamespace(Opc.Ua.Di.Namespaces.OpcUaDiXsd);

            SequenceNumber = decoder.ReadInt32("SequenceNumber");
            EndOfResults = decoder.ReadBoolean("EndOfResults");
            ParameterDefs = (ParameterResultDataTypeCollection)decoder.ReadEncodeableArray("ParameterDefs", typeof(ParameterResultDataType));

            decoder.PopNamespace();
        }

        /// <summary cref="IEncodeable.IsEqual(IEncodeable)" />
        public override bool IsEqual(IEncodeable encodeable)
        {
            if (Object.ReferenceEquals(this, encodeable))
            {
                return true;
            }

            TransferResultDataDataType value = encodeable as TransferResultDataDataType;

            if (value == null)
            {
                return false;
            }

            if (!base.IsEqual(encodeable)) return false;
            if (!Utils.IsEqual(m_sequenceNumber, value.m_sequenceNumber)) return false;
            if (!Utils.IsEqual(m_endOfResults, value.m_endOfResults)) return false;
            if (!Utils.IsEqual(m_parameterDefs, value.m_parameterDefs)) return false;

            return true;
        }    

        #if !NET_STANDARD
        /// <summary cref="ICloneable.Clone" />
        public override object Clone()
        {
            return (TransferResultDataDataType)this.MemberwiseClone();
        }
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            TransferResultDataDataType clone = (TransferResultDataDataType)base.MemberwiseClone();

            clone.m_sequenceNumber = (int)Utils.Clone(this.m_sequenceNumber);
            clone.m_endOfResults = (bool)Utils.Clone(this.m_endOfResults);
            clone.m_parameterDefs = (ParameterResultDataTypeCollection)Utils.Clone(this.m_parameterDefs);

            return clone;
        }
        #endregion

        #region Private Fields
        private int m_sequenceNumber;
        private bool m_endOfResults;
        private ParameterResultDataTypeCollection m_parameterDefs;
        #endregion
    }

    #region TransferResultDataDataTypeCollection Class
    /// <summary>
    /// A collection of TransferResultDataDataType objects.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [CollectionDataContract(Name = "ListOfTransferResultDataDataType", Namespace = Opc.Ua.Di.Namespaces.OpcUaDiXsd, ItemName = "TransferResultDataDataType")]
    #if !NET_STANDARD
    public partial class TransferResultDataDataTypeCollection : List<TransferResultDataDataType>, ICloneable
    #else
    public partial class TransferResultDataDataTypeCollection : List<TransferResultDataDataType>
    #endif
    {
        #region Constructors
        /// <summary>
        /// Initializes the collection with default values.
        /// </summary>
        public TransferResultDataDataTypeCollection() {}

        /// <summary>
        /// Initializes the collection with an initial capacity.
        /// </summary>
        public TransferResultDataDataTypeCollection(int capacity) : base(capacity) {}

        /// <summary>
        /// Initializes the collection with another collection.
        /// </summary>
        public TransferResultDataDataTypeCollection(IEnumerable<TransferResultDataDataType> collection) : base(collection) {}
        #endregion

        #region Static Operators
        /// <summary>
        /// Converts an array to a collection.
        /// </summary>
        public static implicit operator TransferResultDataDataTypeCollection(TransferResultDataDataType[] values)
        {
            if (values != null)
            {
                return new TransferResultDataDataTypeCollection(values);
            }

            return new TransferResultDataDataTypeCollection();
        }

        /// <summary>
        /// Converts a collection to an array.
        /// </summary>
        public static explicit operator TransferResultDataDataType[](TransferResultDataDataTypeCollection values)
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
            return (TransferResultDataDataTypeCollection)this.MemberwiseClone();
        }
        #endregion
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            TransferResultDataDataTypeCollection clone = new TransferResultDataDataTypeCollection(this.Count);

            for (int ii = 0; ii < this.Count; ii++)
            {
                clone.Add((TransferResultDataDataType)Utils.Clone(this[ii]));
            }

            return clone;
        }
    }
    #endregion
    #endif
    #endregion

    #region ParameterResultDataType Class
    #if (!OPCUA_EXCLUDE_ParameterResultDataType)
    /// <summary>
    /// 
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [DataContract(Namespace = Opc.Ua.Di.Namespaces.OpcUaDiXsd)]
    public partial class ParameterResultDataType : IEncodeable
    {
        #region Constructors
        /// <summary>
        /// The default constructor.
        /// </summary>
        public ParameterResultDataType()
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
            m_nodePath = new QualifiedNameCollection();
            m_statusCode = StatusCodes.Good;
            m_diagnostics = null;
        }
        #endregion

        #region Public Properties
        /// <summary>
        /// 
        /// </summary>
        [DataMember(Name = "NodePath", IsRequired = false, Order = 1)]
        public QualifiedNameCollection NodePath
        {
            get
            {
                return m_nodePath;
            }

            set
            {
                m_nodePath = value;

                if (value == null)
                {
                    m_nodePath = new QualifiedNameCollection();
                }
            }
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
            get { return DataTypeIds.ParameterResultDataType; }
        }

        /// <summary cref="IEncodeable.BinaryEncodingId" />
        public virtual ExpandedNodeId BinaryEncodingId
        {
            get { return ObjectIds.ParameterResultDataType_Encoding_DefaultBinary; }
        }

        /// <summary cref="IEncodeable.XmlEncodingId" />
        public virtual ExpandedNodeId XmlEncodingId
        {
            get { return ObjectIds.ParameterResultDataType_Encoding_DefaultXml; }
        }

        /// <summary cref="IEncodeable.Encode(IEncoder)" />
        public virtual void Encode(IEncoder encoder)
        {
            encoder.PushNamespace(Opc.Ua.Di.Namespaces.OpcUaDiXsd);

            encoder.WriteQualifiedNameArray("NodePath", NodePath);
            encoder.WriteStatusCode("StatusCode", StatusCode);
            encoder.WriteDiagnosticInfo("Diagnostics", Diagnostics);

            encoder.PopNamespace();
        }

        /// <summary cref="IEncodeable.Decode(IDecoder)" />
        public virtual void Decode(IDecoder decoder)
        {
            decoder.PushNamespace(Opc.Ua.Di.Namespaces.OpcUaDiXsd);

            NodePath = decoder.ReadQualifiedNameArray("NodePath");
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

            ParameterResultDataType value = encodeable as ParameterResultDataType;

            if (value == null)
            {
                return false;
            }

            if (!Utils.IsEqual(m_nodePath, value.m_nodePath)) return false;
            if (!Utils.IsEqual(m_statusCode, value.m_statusCode)) return false;
            if (!Utils.IsEqual(m_diagnostics, value.m_diagnostics)) return false;

            return true;
        }

        #if !NET_STANDARD
        /// <summary cref="ICloneable.Clone" />
        public virtual object Clone()
        {
            return (ParameterResultDataType)this.MemberwiseClone();
        }
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            ParameterResultDataType clone = (ParameterResultDataType)base.MemberwiseClone();

            clone.m_nodePath = (QualifiedNameCollection)Utils.Clone(this.m_nodePath);
            clone.m_statusCode = (StatusCode)Utils.Clone(this.m_statusCode);
            clone.m_diagnostics = (DiagnosticInfo)Utils.Clone(this.m_diagnostics);

            return clone;
        }
        #endregion

        #region Private Fields
        private QualifiedNameCollection m_nodePath;
        private StatusCode m_statusCode;
        private DiagnosticInfo m_diagnostics;
        #endregion
    }

    #region ParameterResultDataTypeCollection Class
    /// <summary>
    /// A collection of ParameterResultDataType objects.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [CollectionDataContract(Name = "ListOfParameterResultDataType", Namespace = Opc.Ua.Di.Namespaces.OpcUaDiXsd, ItemName = "ParameterResultDataType")]
    #if !NET_STANDARD
    public partial class ParameterResultDataTypeCollection : List<ParameterResultDataType>, ICloneable
    #else
    public partial class ParameterResultDataTypeCollection : List<ParameterResultDataType>
    #endif
    {
        #region Constructors
        /// <summary>
        /// Initializes the collection with default values.
        /// </summary>
        public ParameterResultDataTypeCollection() {}

        /// <summary>
        /// Initializes the collection with an initial capacity.
        /// </summary>
        public ParameterResultDataTypeCollection(int capacity) : base(capacity) {}

        /// <summary>
        /// Initializes the collection with another collection.
        /// </summary>
        public ParameterResultDataTypeCollection(IEnumerable<ParameterResultDataType> collection) : base(collection) {}
        #endregion

        #region Static Operators
        /// <summary>
        /// Converts an array to a collection.
        /// </summary>
        public static implicit operator ParameterResultDataTypeCollection(ParameterResultDataType[] values)
        {
            if (values != null)
            {
                return new ParameterResultDataTypeCollection(values);
            }

            return new ParameterResultDataTypeCollection();
        }

        /// <summary>
        /// Converts a collection to an array.
        /// </summary>
        public static explicit operator ParameterResultDataType[](ParameterResultDataTypeCollection values)
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
            return (ParameterResultDataTypeCollection)this.MemberwiseClone();
        }
        #endregion
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            ParameterResultDataTypeCollection clone = new ParameterResultDataTypeCollection(this.Count);

            for (int ii = 0; ii < this.Count; ii++)
            {
                clone.Add((ParameterResultDataType)Utils.Clone(this[ii]));
            }

            return clone;
        }
    }
    #endregion
    #endif
    #endregion
}
