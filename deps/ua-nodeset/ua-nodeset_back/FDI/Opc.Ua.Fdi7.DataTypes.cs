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

namespace Opc.Ua.Fdi7
{
    #region EddDataTypeEnum Enumeration
    #if (!OPCUA_EXCLUDE_EddDataTypeEnum)
    /// <summary>
    /// 
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [DataContract(Namespace = Opc.Ua.Fdi7.Namespaces.OpcUaFdi7Xsd)]
    public enum EddDataTypeEnum
    {
        /// <remarks />
        [EnumMember(Value = "BOOLEAN_1")]
        BOOLEAN = 1,

        /// <remarks />
        [EnumMember(Value = "DOUBLE_2")]
        DOUBLE = 2,

        /// <remarks />
        [EnumMember(Value = "FLOAT_3")]
        FLOAT = 3,

        /// <remarks />
        [EnumMember(Value = "INTEGER_4")]
        INTEGER = 4,

        /// <remarks />
        [EnumMember(Value = "UNSIGNED_INTEGER_5")]
        UNSIGNED_INTEGER = 5,

        /// <remarks />
        [EnumMember(Value = "DATE_6")]
        DATE = 6,

        /// <remarks />
        [EnumMember(Value = "DATE_AND_TIME_7")]
        DATE_AND_TIME = 7,

        /// <remarks />
        [EnumMember(Value = "DURATION_8")]
        DURATION = 8,

        /// <remarks />
        [EnumMember(Value = "TIME_9")]
        TIME = 9,

        /// <remarks />
        [EnumMember(Value = "TIME_VALUE_10")]
        TIME_VALUE = 10,

        /// <remarks />
        [EnumMember(Value = "BIT_ENUMERATED_11")]
        BIT_ENUMERATED = 11,

        /// <remarks />
        [EnumMember(Value = "ENUMERATED_12")]
        ENUMERATED = 12,

        /// <remarks />
        [EnumMember(Value = "ASCII_13")]
        ASCII = 13,

        /// <remarks />
        [EnumMember(Value = "BITSTRING_14")]
        BITSTRING = 14,

        /// <remarks />
        [EnumMember(Value = "EUC_15")]
        EUC = 15,

        /// <remarks />
        [EnumMember(Value = "OCTET_16")]
        OCTET = 16,

        /// <remarks />
        [EnumMember(Value = "PACKED_ASCII_17")]
        PACKED_ASCII = 17,

        /// <remarks />
        [EnumMember(Value = "PASSWORD_18")]
        PASSWORD = 18,

        /// <remarks />
        [EnumMember(Value = "VISIBLE_19")]
        VISIBLE = 19,
    }

    #region EddDataTypeEnumCollection Class
    /// <summary>
    /// A collection of EddDataTypeEnum objects.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [CollectionDataContract(Name = "ListOfEddDataTypeEnum", Namespace = Opc.Ua.Fdi7.Namespaces.OpcUaFdi7Xsd, ItemName = "EddDataTypeEnum")]
    #if !NET_STANDARD
    public partial class EddDataTypeEnumCollection : List<EddDataTypeEnum>, ICloneable
    #else
    public partial class EddDataTypeEnumCollection : List<EddDataTypeEnum>
    #endif
    {
        #region Constructors
        /// <summary>
        /// Initializes the collection with default values.
        /// </summary>
        public EddDataTypeEnumCollection() {}

        /// <summary>
        /// Initializes the collection with an initial capacity.
        /// </summary>
        public EddDataTypeEnumCollection(int capacity) : base(capacity) {}

        /// <summary>
        /// Initializes the collection with another collection.
        /// </summary>
        public EddDataTypeEnumCollection(IEnumerable<EddDataTypeEnum> collection) : base(collection) {}
        #endregion

        #region Static Operators
        /// <summary>
        /// Converts an array to a collection.
        /// </summary>
        public static implicit operator EddDataTypeEnumCollection(EddDataTypeEnum[] values)
        {
            if (values != null)
            {
                return new EddDataTypeEnumCollection(values);
            }

            return new EddDataTypeEnumCollection();
        }

        /// <summary>
        /// Converts a collection to an array.
        /// </summary>
        public static explicit operator EddDataTypeEnum[](EddDataTypeEnumCollection values)
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
            return (EddDataTypeEnumCollection)this.MemberwiseClone();
        }
        #endregion
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            EddDataTypeEnumCollection clone = new EddDataTypeEnumCollection(this.Count);

            for (int ii = 0; ii < this.Count; ii++)
            {
                clone.Add((EddDataTypeEnum)Utils.Clone(this[ii]));
            }

            return clone;
        }
    }
    #endregion
    #endif
    #endregion

    #region EddDataTypeInfo Class
    #if (!OPCUA_EXCLUDE_EddDataTypeInfo)
    /// <summary>
    /// 
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [DataContract(Namespace = Opc.Ua.Fdi7.Namespaces.OpcUaFdi7Xsd)]
    public partial class EddDataTypeInfo : IEncodeable
    {
        #region Constructors
        /// <summary>
        /// The default constructor.
        /// </summary>
        public EddDataTypeInfo()
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
            m_eddDataType = EddDataTypeEnum.BOOLEAN;
            m_size = (uint)0;
        }
        #endregion

        #region Public Properties
        /// <remarks />
        [DataMember(Name = "EddDataType", IsRequired = false, Order = 1)]
        public EddDataTypeEnum EddDataType
        {
            get { return m_eddDataType;  }
            set { m_eddDataType = value; }
        }

        /// <remarks />
        [DataMember(Name = "Size", IsRequired = false, Order = 2)]
        public uint Size
        {
            get { return m_size;  }
            set { m_size = value; }
        }
        #endregion

        #region IEncodeable Members
        /// <summary cref="IEncodeable.TypeId" />
        public virtual ExpandedNodeId TypeId
        {
            get { return DataTypeIds.EddDataTypeInfo; }
        }

        /// <summary cref="IEncodeable.BinaryEncodingId" />
        public virtual ExpandedNodeId BinaryEncodingId
        {
            get { return ObjectIds.EddDataTypeInfo_Encoding_DefaultBinary; }
        }

        /// <summary cref="IEncodeable.XmlEncodingId" />
        public virtual ExpandedNodeId XmlEncodingId
        {
            get { return ObjectIds.EddDataTypeInfo_Encoding_DefaultXml; }
        }

        /// <summary cref="IEncodeable.Encode(IEncoder)" />
        public virtual void Encode(IEncoder encoder)
        {
            encoder.PushNamespace(Opc.Ua.Fdi7.Namespaces.OpcUaFdi7Xsd);

            encoder.WriteEnumerated("EddDataType", EddDataType);
            encoder.WriteUInt32("Size", Size);

            encoder.PopNamespace();
        }

        /// <summary cref="IEncodeable.Decode(IDecoder)" />
        public virtual void Decode(IDecoder decoder)
        {
            decoder.PushNamespace(Opc.Ua.Fdi7.Namespaces.OpcUaFdi7Xsd);

            EddDataType = (EddDataTypeEnum)decoder.ReadEnumerated("EddDataType", typeof(EddDataTypeEnum));
            Size = decoder.ReadUInt32("Size");

            decoder.PopNamespace();
        }

        /// <summary cref="IEncodeable.IsEqual(IEncodeable)" />
        public virtual bool IsEqual(IEncodeable encodeable)
        {
            if (Object.ReferenceEquals(this, encodeable))
            {
                return true;
            }

            EddDataTypeInfo value = encodeable as EddDataTypeInfo;

            if (value == null)
            {
                return false;
            }

            if (!Utils.IsEqual(m_eddDataType, value.m_eddDataType)) return false;
            if (!Utils.IsEqual(m_size, value.m_size)) return false;

            return true;
        }

        #if !NET_STANDARD
        /// <summary cref="ICloneable.Clone" />
        public virtual object Clone()
        {
            return (EddDataTypeInfo)this.MemberwiseClone();
        }
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            EddDataTypeInfo clone = (EddDataTypeInfo)base.MemberwiseClone();

            clone.m_eddDataType = (EddDataTypeEnum)Utils.Clone(this.m_eddDataType);
            clone.m_size = (uint)Utils.Clone(this.m_size);

            return clone;
        }
        #endregion

        #region Private Fields
        private EddDataTypeEnum m_eddDataType;
        private uint m_size;
        #endregion
    }

    #region EddDataTypeInfoCollection Class
    /// <summary>
    /// A collection of EddDataTypeInfo objects.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [CollectionDataContract(Name = "ListOfEddDataTypeInfo", Namespace = Opc.Ua.Fdi7.Namespaces.OpcUaFdi7Xsd, ItemName = "EddDataTypeInfo")]
    #if !NET_STANDARD
    public partial class EddDataTypeInfoCollection : List<EddDataTypeInfo>, ICloneable
    #else
    public partial class EddDataTypeInfoCollection : List<EddDataTypeInfo>
    #endif
    {
        #region Constructors
        /// <summary>
        /// Initializes the collection with default values.
        /// </summary>
        public EddDataTypeInfoCollection() {}

        /// <summary>
        /// Initializes the collection with an initial capacity.
        /// </summary>
        public EddDataTypeInfoCollection(int capacity) : base(capacity) {}

        /// <summary>
        /// Initializes the collection with another collection.
        /// </summary>
        public EddDataTypeInfoCollection(IEnumerable<EddDataTypeInfo> collection) : base(collection) {}
        #endregion

        #region Static Operators
        /// <summary>
        /// Converts an array to a collection.
        /// </summary>
        public static implicit operator EddDataTypeInfoCollection(EddDataTypeInfo[] values)
        {
            if (values != null)
            {
                return new EddDataTypeInfoCollection(values);
            }

            return new EddDataTypeInfoCollection();
        }

        /// <summary>
        /// Converts a collection to an array.
        /// </summary>
        public static explicit operator EddDataTypeInfo[](EddDataTypeInfoCollection values)
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
            return (EddDataTypeInfoCollection)this.MemberwiseClone();
        }
        #endregion
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            EddDataTypeInfoCollection clone = new EddDataTypeInfoCollection(this.Count);

            for (int ii = 0; ii < this.Count; ii++)
            {
                clone.Add((EddDataTypeInfo)Utils.Clone(this[ii]));
            }

            return clone;
        }
    }
    #endregion
    #endif
    #endregion
}
