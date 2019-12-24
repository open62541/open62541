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

namespace Opc.MDIS
{
    #region SignatureStatusEnum Enumeration
    #if (!OPCUA_EXCLUDE_SignatureStatusEnum)
    /// <summary>
    /// 
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [DataContract(Namespace = Opc.MDIS.Namespaces.MDIS)]
    public enum SignatureStatusEnum
    {
        /// <remarks />
        [EnumMember(Value = "NotAvailable_1")]
        NotAvailable = 1,

        /// <remarks />
        [EnumMember(Value = "Completed_2")]
        Completed = 2,

        /// <remarks />
        [EnumMember(Value = "Failed_4")]
        Failed = 4,
    }

    #region SignatureStatusEnumCollection Class
    /// <summary>
    /// A collection of SignatureStatusEnum objects.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [CollectionDataContract(Name = "ListOfSignatureStatusEnum", Namespace = Opc.MDIS.Namespaces.MDIS, ItemName = "SignatureStatusEnum")]
    #if !NET_STANDARD
    public partial class SignatureStatusEnumCollection : List<SignatureStatusEnum>, ICloneable
    #else
    public partial class SignatureStatusEnumCollection : List<SignatureStatusEnum>
    #endif
    {
        #region Constructors
        /// <summary>
        /// Initializes the collection with default values.
        /// </summary>
        public SignatureStatusEnumCollection() {}

        /// <summary>
        /// Initializes the collection with an initial capacity.
        /// </summary>
        public SignatureStatusEnumCollection(int capacity) : base(capacity) {}

        /// <summary>
        /// Initializes the collection with another collection.
        /// </summary>
        public SignatureStatusEnumCollection(IEnumerable<SignatureStatusEnum> collection) : base(collection) {}
        #endregion

        #region Static Operators
        /// <summary>
        /// Converts an array to a collection.
        /// </summary>
        public static implicit operator SignatureStatusEnumCollection(SignatureStatusEnum[] values)
        {
            if (values != null)
            {
                return new SignatureStatusEnumCollection(values);
            }

            return new SignatureStatusEnumCollection();
        }

        /// <summary>
        /// Converts a collection to an array.
        /// </summary>
        public static explicit operator SignatureStatusEnum[](SignatureStatusEnumCollection values)
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
            return (SignatureStatusEnumCollection)this.MemberwiseClone();
        }
        #endregion
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            SignatureStatusEnumCollection clone = new SignatureStatusEnumCollection(this.Count);

            for (int ii = 0; ii < this.Count; ii++)
            {
                clone.Add((SignatureStatusEnum)Utils.Clone(this[ii]));
            }

            return clone;
        }
    }
    #endregion
    #endif
    #endregion

    #region SetCalculatedPositionEnum Enumeration
    #if (!OPCUA_EXCLUDE_SetCalculatedPositionEnum)
    /// <summary>
    /// 
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [DataContract(Namespace = Opc.MDIS.Namespaces.MDIS)]
    public enum SetCalculatedPositionEnum
    {
        /// <remarks />
        [EnumMember(Value = "Initial_0")]
        Initial = 0,

        /// <remarks />
        [EnumMember(Value = "Inprogress_1")]
        Inprogress = 1,

        /// <remarks />
        [EnumMember(Value = "Complete_2")]
        Complete = 2,

        /// <remarks />
        [EnumMember(Value = "Fault_4")]
        Fault = 4,
    }

    #region SetCalculatedPositionEnumCollection Class
    /// <summary>
    /// A collection of SetCalculatedPositionEnum objects.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [CollectionDataContract(Name = "ListOfSetCalculatedPositionEnum", Namespace = Opc.MDIS.Namespaces.MDIS, ItemName = "SetCalculatedPositionEnum")]
    #if !NET_STANDARD
    public partial class SetCalculatedPositionEnumCollection : List<SetCalculatedPositionEnum>, ICloneable
    #else
    public partial class SetCalculatedPositionEnumCollection : List<SetCalculatedPositionEnum>
    #endif
    {
        #region Constructors
        /// <summary>
        /// Initializes the collection with default values.
        /// </summary>
        public SetCalculatedPositionEnumCollection() {}

        /// <summary>
        /// Initializes the collection with an initial capacity.
        /// </summary>
        public SetCalculatedPositionEnumCollection(int capacity) : base(capacity) {}

        /// <summary>
        /// Initializes the collection with another collection.
        /// </summary>
        public SetCalculatedPositionEnumCollection(IEnumerable<SetCalculatedPositionEnum> collection) : base(collection) {}
        #endregion

        #region Static Operators
        /// <summary>
        /// Converts an array to a collection.
        /// </summary>
        public static implicit operator SetCalculatedPositionEnumCollection(SetCalculatedPositionEnum[] values)
        {
            if (values != null)
            {
                return new SetCalculatedPositionEnumCollection(values);
            }

            return new SetCalculatedPositionEnumCollection();
        }

        /// <summary>
        /// Converts a collection to an array.
        /// </summary>
        public static explicit operator SetCalculatedPositionEnum[](SetCalculatedPositionEnumCollection values)
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
            return (SetCalculatedPositionEnumCollection)this.MemberwiseClone();
        }
        #endregion
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            SetCalculatedPositionEnumCollection clone = new SetCalculatedPositionEnumCollection(this.Count);

            for (int ii = 0; ii < this.Count; ii++)
            {
                clone.Add((SetCalculatedPositionEnum)Utils.Clone(this[ii]));
            }

            return clone;
        }
    }
    #endregion
    #endif
    #endregion

    #region CommandEnum Enumeration
    #if (!OPCUA_EXCLUDE_CommandEnum)
    /// <summary>
    /// 
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [DataContract(Namespace = Opc.MDIS.Namespaces.MDIS)]
    public enum CommandEnum
    {
        /// <remarks />
        [EnumMember(Value = "Close_1")]
        Close = 1,

        /// <remarks />
        [EnumMember(Value = "Open_2")]
        Open = 2,

        /// <remarks />
        [EnumMember(Value = "None_4")]
        None = 4,
    }

    #region CommandEnumCollection Class
    /// <summary>
    /// A collection of CommandEnum objects.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [CollectionDataContract(Name = "ListOfCommandEnum", Namespace = Opc.MDIS.Namespaces.MDIS, ItemName = "CommandEnum")]
    #if !NET_STANDARD
    public partial class CommandEnumCollection : List<CommandEnum>, ICloneable
    #else
    public partial class CommandEnumCollection : List<CommandEnum>
    #endif
    {
        #region Constructors
        /// <summary>
        /// Initializes the collection with default values.
        /// </summary>
        public CommandEnumCollection() {}

        /// <summary>
        /// Initializes the collection with an initial capacity.
        /// </summary>
        public CommandEnumCollection(int capacity) : base(capacity) {}

        /// <summary>
        /// Initializes the collection with another collection.
        /// </summary>
        public CommandEnumCollection(IEnumerable<CommandEnum> collection) : base(collection) {}
        #endregion

        #region Static Operators
        /// <summary>
        /// Converts an array to a collection.
        /// </summary>
        public static implicit operator CommandEnumCollection(CommandEnum[] values)
        {
            if (values != null)
            {
                return new CommandEnumCollection(values);
            }

            return new CommandEnumCollection();
        }

        /// <summary>
        /// Converts a collection to an array.
        /// </summary>
        public static explicit operator CommandEnum[](CommandEnumCollection values)
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
            return (CommandEnumCollection)this.MemberwiseClone();
        }
        #endregion
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            CommandEnumCollection clone = new CommandEnumCollection(this.Count);

            for (int ii = 0; ii < this.Count; ii++)
            {
                clone.Add((CommandEnum)Utils.Clone(this[ii]));
            }

            return clone;
        }
    }
    #endregion
    #endif
    #endregion

    #region ChokeCommandEnum Enumeration
    #if (!OPCUA_EXCLUDE_ChokeCommandEnum)
    /// <summary>
    /// 
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [DataContract(Namespace = Opc.MDIS.Namespaces.MDIS)]
    public enum ChokeCommandEnum
    {
        /// <remarks />
        [EnumMember(Value = "Close_1")]
        Close = 1,

        /// <remarks />
        [EnumMember(Value = "Open_2")]
        Open = 2,
    }

    #region ChokeCommandEnumCollection Class
    /// <summary>
    /// A collection of ChokeCommandEnum objects.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [CollectionDataContract(Name = "ListOfChokeCommandEnum", Namespace = Opc.MDIS.Namespaces.MDIS, ItemName = "ChokeCommandEnum")]
    #if !NET_STANDARD
    public partial class ChokeCommandEnumCollection : List<ChokeCommandEnum>, ICloneable
    #else
    public partial class ChokeCommandEnumCollection : List<ChokeCommandEnum>
    #endif
    {
        #region Constructors
        /// <summary>
        /// Initializes the collection with default values.
        /// </summary>
        public ChokeCommandEnumCollection() {}

        /// <summary>
        /// Initializes the collection with an initial capacity.
        /// </summary>
        public ChokeCommandEnumCollection(int capacity) : base(capacity) {}

        /// <summary>
        /// Initializes the collection with another collection.
        /// </summary>
        public ChokeCommandEnumCollection(IEnumerable<ChokeCommandEnum> collection) : base(collection) {}
        #endregion

        #region Static Operators
        /// <summary>
        /// Converts an array to a collection.
        /// </summary>
        public static implicit operator ChokeCommandEnumCollection(ChokeCommandEnum[] values)
        {
            if (values != null)
            {
                return new ChokeCommandEnumCollection(values);
            }

            return new ChokeCommandEnumCollection();
        }

        /// <summary>
        /// Converts a collection to an array.
        /// </summary>
        public static explicit operator ChokeCommandEnum[](ChokeCommandEnumCollection values)
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
            return (ChokeCommandEnumCollection)this.MemberwiseClone();
        }
        #endregion
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            ChokeCommandEnumCollection clone = new ChokeCommandEnumCollection(this.Count);

            for (int ii = 0; ii < this.Count; ii++)
            {
                clone.Add((ChokeCommandEnum)Utils.Clone(this[ii]));
            }

            return clone;
        }
    }
    #endregion
    #endif
    #endregion

    #region SEMEnum Enumeration
    #if (!OPCUA_EXCLUDE_SEMEnum)
    /// <summary>
    /// 
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [DataContract(Namespace = Opc.MDIS.Namespaces.MDIS)]
    public enum SEMEnum
    {
        /// <remarks />
        [EnumMember(Value = "SEM_A_1")]
        SEM_A = 1,

        /// <remarks />
        [EnumMember(Value = "SEM_B_2")]
        SEM_B = 2,

        /// <remarks />
        [EnumMember(Value = "Auto_4")]
        Auto = 4,
    }

    #region SEMEnumCollection Class
    /// <summary>
    /// A collection of SEMEnum objects.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [CollectionDataContract(Name = "ListOfSEMEnum", Namespace = Opc.MDIS.Namespaces.MDIS, ItemName = "SEMEnum")]
    #if !NET_STANDARD
    public partial class SEMEnumCollection : List<SEMEnum>, ICloneable
    #else
    public partial class SEMEnumCollection : List<SEMEnum>
    #endif
    {
        #region Constructors
        /// <summary>
        /// Initializes the collection with default values.
        /// </summary>
        public SEMEnumCollection() {}

        /// <summary>
        /// Initializes the collection with an initial capacity.
        /// </summary>
        public SEMEnumCollection(int capacity) : base(capacity) {}

        /// <summary>
        /// Initializes the collection with another collection.
        /// </summary>
        public SEMEnumCollection(IEnumerable<SEMEnum> collection) : base(collection) {}
        #endregion

        #region Static Operators
        /// <summary>
        /// Converts an array to a collection.
        /// </summary>
        public static implicit operator SEMEnumCollection(SEMEnum[] values)
        {
            if (values != null)
            {
                return new SEMEnumCollection(values);
            }

            return new SEMEnumCollection();
        }

        /// <summary>
        /// Converts a collection to an array.
        /// </summary>
        public static explicit operator SEMEnum[](SEMEnumCollection values)
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
            return (SEMEnumCollection)this.MemberwiseClone();
        }
        #endregion
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            SEMEnumCollection clone = new SEMEnumCollection(this.Count);

            for (int ii = 0; ii < this.Count; ii++)
            {
                clone.Add((SEMEnum)Utils.Clone(this[ii]));
            }

            return clone;
        }
    }
    #endregion
    #endif
    #endregion

    #region ValvePositionEnum Enumeration
    #if (!OPCUA_EXCLUDE_ValvePositionEnum)
    /// <summary>
    /// 
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [DataContract(Namespace = Opc.MDIS.Namespaces.MDIS)]
    public enum ValvePositionEnum
    {
        /// <remarks />
        [EnumMember(Value = "Closed_1")]
        Closed = 1,

        /// <remarks />
        [EnumMember(Value = "Open_2")]
        Open = 2,

        /// <remarks />
        [EnumMember(Value = "Moving_4")]
        Moving = 4,

        /// <remarks />
        [EnumMember(Value = "Unknown_8")]
        Unknown = 8,
    }

    #region ValvePositionEnumCollection Class
    /// <summary>
    /// A collection of ValvePositionEnum objects.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [CollectionDataContract(Name = "ListOfValvePositionEnum", Namespace = Opc.MDIS.Namespaces.MDIS, ItemName = "ValvePositionEnum")]
    #if !NET_STANDARD
    public partial class ValvePositionEnumCollection : List<ValvePositionEnum>, ICloneable
    #else
    public partial class ValvePositionEnumCollection : List<ValvePositionEnum>
    #endif
    {
        #region Constructors
        /// <summary>
        /// Initializes the collection with default values.
        /// </summary>
        public ValvePositionEnumCollection() {}

        /// <summary>
        /// Initializes the collection with an initial capacity.
        /// </summary>
        public ValvePositionEnumCollection(int capacity) : base(capacity) {}

        /// <summary>
        /// Initializes the collection with another collection.
        /// </summary>
        public ValvePositionEnumCollection(IEnumerable<ValvePositionEnum> collection) : base(collection) {}
        #endregion

        #region Static Operators
        /// <summary>
        /// Converts an array to a collection.
        /// </summary>
        public static implicit operator ValvePositionEnumCollection(ValvePositionEnum[] values)
        {
            if (values != null)
            {
                return new ValvePositionEnumCollection(values);
            }

            return new ValvePositionEnumCollection();
        }

        /// <summary>
        /// Converts a collection to an array.
        /// </summary>
        public static explicit operator ValvePositionEnum[](ValvePositionEnumCollection values)
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
            return (ValvePositionEnumCollection)this.MemberwiseClone();
        }
        #endregion
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            ValvePositionEnumCollection clone = new ValvePositionEnumCollection(this.Count);

            for (int ii = 0; ii < this.Count; ii++)
            {
                clone.Add((ValvePositionEnum)Utils.Clone(this[ii]));
            }

            return clone;
        }
    }
    #endregion
    #endif
    #endregion

    #region ChokeMoveEnum Enumeration
    #if (!OPCUA_EXCLUDE_ChokeMoveEnum)
    /// <summary>
    /// 
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [DataContract(Namespace = Opc.MDIS.Namespaces.MDIS)]
    public enum ChokeMoveEnum
    {
        /// <remarks />
        [EnumMember(Value = "Moving_1")]
        Moving = 1,

        /// <remarks />
        [EnumMember(Value = "Stopped_2")]
        Stopped = 2,
    }

    #region ChokeMoveEnumCollection Class
    /// <summary>
    /// A collection of ChokeMoveEnum objects.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [CollectionDataContract(Name = "ListOfChokeMoveEnum", Namespace = Opc.MDIS.Namespaces.MDIS, ItemName = "ChokeMoveEnum")]
    #if !NET_STANDARD
    public partial class ChokeMoveEnumCollection : List<ChokeMoveEnum>, ICloneable
    #else
    public partial class ChokeMoveEnumCollection : List<ChokeMoveEnum>
    #endif
    {
        #region Constructors
        /// <summary>
        /// Initializes the collection with default values.
        /// </summary>
        public ChokeMoveEnumCollection() {}

        /// <summary>
        /// Initializes the collection with an initial capacity.
        /// </summary>
        public ChokeMoveEnumCollection(int capacity) : base(capacity) {}

        /// <summary>
        /// Initializes the collection with another collection.
        /// </summary>
        public ChokeMoveEnumCollection(IEnumerable<ChokeMoveEnum> collection) : base(collection) {}
        #endregion

        #region Static Operators
        /// <summary>
        /// Converts an array to a collection.
        /// </summary>
        public static implicit operator ChokeMoveEnumCollection(ChokeMoveEnum[] values)
        {
            if (values != null)
            {
                return new ChokeMoveEnumCollection(values);
            }

            return new ChokeMoveEnumCollection();
        }

        /// <summary>
        /// Converts a collection to an array.
        /// </summary>
        public static explicit operator ChokeMoveEnum[](ChokeMoveEnumCollection values)
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
            return (ChokeMoveEnumCollection)this.MemberwiseClone();
        }
        #endregion
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            ChokeMoveEnumCollection clone = new ChokeMoveEnumCollection(this.Count);

            for (int ii = 0; ii < this.Count; ii++)
            {
                clone.Add((ChokeMoveEnum)Utils.Clone(this[ii]));
            }

            return clone;
        }
    }
    #endregion
    #endif
    #endregion

    #region MDISVersionDataType Class
    #if (!OPCUA_EXCLUDE_MDISVersionDataType)
    /// <summary>
    /// 
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [DataContract(Namespace = Opc.MDIS.Namespaces.MDIS)]
    public partial class MDISVersionDataType : IEncodeable
    {
        #region Constructors
        /// <summary>
        /// The default constructor.
        /// </summary>
        public MDISVersionDataType()
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
            m_majorVersion = (byte)0;
            m_minorVersion = (byte)0;
            m_build = (byte)0;
        }
        #endregion

        #region Public Properties
        /// <remarks />
        [DataMember(Name = "MajorVersion", IsRequired = false, Order = 1)]
        public byte MajorVersion
        {
            get { return m_majorVersion;  }
            set { m_majorVersion = value; }
        }

        /// <remarks />
        [DataMember(Name = "MinorVersion", IsRequired = false, Order = 2)]
        public byte MinorVersion
        {
            get { return m_minorVersion;  }
            set { m_minorVersion = value; }
        }

        /// <remarks />
        [DataMember(Name = "Build", IsRequired = false, Order = 3)]
        public byte Build
        {
            get { return m_build;  }
            set { m_build = value; }
        }
        #endregion

        #region IEncodeable Members
        /// <summary cref="IEncodeable.TypeId" />
        public virtual ExpandedNodeId TypeId
        {
            get { return DataTypeIds.MDISVersionDataType; }
        }

        /// <summary cref="IEncodeable.BinaryEncodingId" />
        public virtual ExpandedNodeId BinaryEncodingId
        {
            get { return ObjectIds.MDISVersionDataType_Encoding_DefaultBinary; }
        }

        /// <summary cref="IEncodeable.XmlEncodingId" />
        public virtual ExpandedNodeId XmlEncodingId
        {
            get { return ObjectIds.MDISVersionDataType_Encoding_DefaultXml; }
        }

        /// <summary cref="IEncodeable.Encode(IEncoder)" />
        public virtual void Encode(IEncoder encoder)
        {
            encoder.PushNamespace(Opc.MDIS.Namespaces.MDIS);

            encoder.WriteByte("MajorVersion", MajorVersion);
            encoder.WriteByte("MinorVersion", MinorVersion);
            encoder.WriteByte("Build", Build);

            encoder.PopNamespace();
        }

        /// <summary cref="IEncodeable.Decode(IDecoder)" />
        public virtual void Decode(IDecoder decoder)
        {
            decoder.PushNamespace(Opc.MDIS.Namespaces.MDIS);

            MajorVersion = decoder.ReadByte("MajorVersion");
            MinorVersion = decoder.ReadByte("MinorVersion");
            Build = decoder.ReadByte("Build");

            decoder.PopNamespace();
        }

        /// <summary cref="IEncodeable.IsEqual(IEncodeable)" />
        public virtual bool IsEqual(IEncodeable encodeable)
        {
            if (Object.ReferenceEquals(this, encodeable))
            {
                return true;
            }

            MDISVersionDataType value = encodeable as MDISVersionDataType;

            if (value == null)
            {
                return false;
            }

            if (!Utils.IsEqual(m_majorVersion, value.m_majorVersion)) return false;
            if (!Utils.IsEqual(m_minorVersion, value.m_minorVersion)) return false;
            if (!Utils.IsEqual(m_build, value.m_build)) return false;

            return true;
        }

        #if !NET_STANDARD
        /// <summary cref="ICloneable.Clone" />
        public virtual object Clone()
        {
            return (MDISVersionDataType)this.MemberwiseClone();
        }
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            MDISVersionDataType clone = (MDISVersionDataType)base.MemberwiseClone();

            clone.m_majorVersion = (byte)Utils.Clone(this.m_majorVersion);
            clone.m_minorVersion = (byte)Utils.Clone(this.m_minorVersion);
            clone.m_build = (byte)Utils.Clone(this.m_build);

            return clone;
        }
        #endregion

        #region Private Fields
        private byte m_majorVersion;
        private byte m_minorVersion;
        private byte m_build;
        #endregion
    }

    #region MDISVersionDataTypeCollection Class
    /// <summary>
    /// A collection of MDISVersionDataType objects.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [CollectionDataContract(Name = "ListOfMDISVersionDataType", Namespace = Opc.MDIS.Namespaces.MDIS, ItemName = "MDISVersionDataType")]
    #if !NET_STANDARD
    public partial class MDISVersionDataTypeCollection : List<MDISVersionDataType>, ICloneable
    #else
    public partial class MDISVersionDataTypeCollection : List<MDISVersionDataType>
    #endif
    {
        #region Constructors
        /// <summary>
        /// Initializes the collection with default values.
        /// </summary>
        public MDISVersionDataTypeCollection() {}

        /// <summary>
        /// Initializes the collection with an initial capacity.
        /// </summary>
        public MDISVersionDataTypeCollection(int capacity) : base(capacity) {}

        /// <summary>
        /// Initializes the collection with another collection.
        /// </summary>
        public MDISVersionDataTypeCollection(IEnumerable<MDISVersionDataType> collection) : base(collection) {}
        #endregion

        #region Static Operators
        /// <summary>
        /// Converts an array to a collection.
        /// </summary>
        public static implicit operator MDISVersionDataTypeCollection(MDISVersionDataType[] values)
        {
            if (values != null)
            {
                return new MDISVersionDataTypeCollection(values);
            }

            return new MDISVersionDataTypeCollection();
        }

        /// <summary>
        /// Converts a collection to an array.
        /// </summary>
        public static explicit operator MDISVersionDataType[](MDISVersionDataTypeCollection values)
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
            return (MDISVersionDataTypeCollection)this.MemberwiseClone();
        }
        #endregion
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            MDISVersionDataTypeCollection clone = new MDISVersionDataTypeCollection(this.Count);

            for (int ii = 0; ii < this.Count; ii++)
            {
                clone.Add((MDISVersionDataType)Utils.Clone(this[ii]));
            }

            return clone;
        }
    }
    #endregion
    #endif
    #endregion
}
