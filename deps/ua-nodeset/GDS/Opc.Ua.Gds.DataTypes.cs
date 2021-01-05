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

namespace Opc.Ua.Gds
{
    #region ApplicationRecordDataType Class
    #if (!OPCUA_EXCLUDE_ApplicationRecordDataType)
    /// <summary>
    /// 
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [DataContract(Namespace = Opc.Ua.Gds.Namespaces.OpcUaGdsXsd)]
    public partial class ApplicationRecordDataType : IEncodeable
    {
        #region Constructors
        /// <summary>
        /// The default constructor.
        /// </summary>
        public ApplicationRecordDataType()
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
            m_applicationId = null;
            m_applicationUri = null;
            m_applicationType = ApplicationType.Server;
            m_applicationNames = new LocalizedTextCollection();
            m_productUri = null;
            m_discoveryUrls = new StringCollection();
            m_serverCapabilities = new StringCollection();
        }
        #endregion

        #region Public Properties
        /// <remarks />
        [DataMember(Name = "ApplicationId", IsRequired = false, Order = 1)]
        public NodeId ApplicationId
        {
            get { return m_applicationId;  }
            set { m_applicationId = value; }
        }

        /// <remarks />
        [DataMember(Name = "ApplicationUri", IsRequired = false, Order = 2)]
        public string ApplicationUri
        {
            get { return m_applicationUri;  }
            set { m_applicationUri = value; }
        }

        /// <remarks />
        [DataMember(Name = "ApplicationType", IsRequired = false, Order = 3)]
        public ApplicationType ApplicationType
        {
            get { return m_applicationType;  }
            set { m_applicationType = value; }
        }

        /// <summary>
        /// 
        /// </summary>
        [DataMember(Name = "ApplicationNames", IsRequired = false, Order = 4)]
        public LocalizedTextCollection ApplicationNames
        {
            get
            {
                return m_applicationNames;
            }

            set
            {
                m_applicationNames = value;

                if (value == null)
                {
                    m_applicationNames = new LocalizedTextCollection();
                }
            }
        }

        /// <remarks />
        [DataMember(Name = "ProductUri", IsRequired = false, Order = 5)]
        public string ProductUri
        {
            get { return m_productUri;  }
            set { m_productUri = value; }
        }

        /// <summary>
        /// 
        /// </summary>
        [DataMember(Name = "DiscoveryUrls", IsRequired = false, Order = 6)]
        public StringCollection DiscoveryUrls
        {
            get
            {
                return m_discoveryUrls;
            }

            set
            {
                m_discoveryUrls = value;

                if (value == null)
                {
                    m_discoveryUrls = new StringCollection();
                }
            }
        }

        /// <summary>
        /// 
        /// </summary>
        [DataMember(Name = "ServerCapabilities", IsRequired = false, Order = 7)]
        public StringCollection ServerCapabilities
        {
            get
            {
                return m_serverCapabilities;
            }

            set
            {
                m_serverCapabilities = value;

                if (value == null)
                {
                    m_serverCapabilities = new StringCollection();
                }
            }
        }
        #endregion

        #region IEncodeable Members
        /// <summary cref="IEncodeable.TypeId" />
        public virtual ExpandedNodeId TypeId
        {
            get { return DataTypeIds.ApplicationRecordDataType; }
        }

        /// <summary cref="IEncodeable.BinaryEncodingId" />
        public virtual ExpandedNodeId BinaryEncodingId
        {
            get { return ObjectIds.ApplicationRecordDataType_Encoding_DefaultBinary; }
        }

        /// <summary cref="IEncodeable.XmlEncodingId" />
        public virtual ExpandedNodeId XmlEncodingId
        {
            get { return ObjectIds.ApplicationRecordDataType_Encoding_DefaultXml; }
        }

        /// <summary cref="IEncodeable.Encode(IEncoder)" />
        public virtual void Encode(IEncoder encoder)
        {
            encoder.PushNamespace(Opc.Ua.Gds.Namespaces.OpcUaGdsXsd);

            encoder.WriteNodeId("ApplicationId", ApplicationId);
            encoder.WriteString("ApplicationUri", ApplicationUri);
            encoder.WriteEnumerated("ApplicationType", ApplicationType);
            encoder.WriteLocalizedTextArray("ApplicationNames", ApplicationNames);
            encoder.WriteString("ProductUri", ProductUri);
            encoder.WriteStringArray("DiscoveryUrls", DiscoveryUrls);
            encoder.WriteStringArray("ServerCapabilities", ServerCapabilities);

            encoder.PopNamespace();
        }

        /// <summary cref="IEncodeable.Decode(IDecoder)" />
        public virtual void Decode(IDecoder decoder)
        {
            decoder.PushNamespace(Opc.Ua.Gds.Namespaces.OpcUaGdsXsd);

            ApplicationId = decoder.ReadNodeId("ApplicationId");
            ApplicationUri = decoder.ReadString("ApplicationUri");
            ApplicationType = (ApplicationType)decoder.ReadEnumerated("ApplicationType", typeof(ApplicationType));
            ApplicationNames = decoder.ReadLocalizedTextArray("ApplicationNames");
            ProductUri = decoder.ReadString("ProductUri");
            DiscoveryUrls = decoder.ReadStringArray("DiscoveryUrls");
            ServerCapabilities = decoder.ReadStringArray("ServerCapabilities");

            decoder.PopNamespace();
        }

        /// <summary cref="IEncodeable.IsEqual(IEncodeable)" />
        public virtual bool IsEqual(IEncodeable encodeable)
        {
            if (Object.ReferenceEquals(this, encodeable))
            {
                return true;
            }

            ApplicationRecordDataType value = encodeable as ApplicationRecordDataType;

            if (value == null)
            {
                return false;
            }

            if (!Utils.IsEqual(m_applicationId, value.m_applicationId)) return false;
            if (!Utils.IsEqual(m_applicationUri, value.m_applicationUri)) return false;
            if (!Utils.IsEqual(m_applicationType, value.m_applicationType)) return false;
            if (!Utils.IsEqual(m_applicationNames, value.m_applicationNames)) return false;
            if (!Utils.IsEqual(m_productUri, value.m_productUri)) return false;
            if (!Utils.IsEqual(m_discoveryUrls, value.m_discoveryUrls)) return false;
            if (!Utils.IsEqual(m_serverCapabilities, value.m_serverCapabilities)) return false;

            return true;
        }

        #if !NET_STANDARD
        /// <summary cref="ICloneable.Clone" />
        public virtual object Clone()
        {
            return (ApplicationRecordDataType)this.MemberwiseClone();
        }
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            ApplicationRecordDataType clone = (ApplicationRecordDataType)base.MemberwiseClone();

            clone.m_applicationId = (NodeId)Utils.Clone(this.m_applicationId);
            clone.m_applicationUri = (string)Utils.Clone(this.m_applicationUri);
            clone.m_applicationType = (ApplicationType)Utils.Clone(this.m_applicationType);
            clone.m_applicationNames = (LocalizedTextCollection)Utils.Clone(this.m_applicationNames);
            clone.m_productUri = (string)Utils.Clone(this.m_productUri);
            clone.m_discoveryUrls = (StringCollection)Utils.Clone(this.m_discoveryUrls);
            clone.m_serverCapabilities = (StringCollection)Utils.Clone(this.m_serverCapabilities);

            return clone;
        }
        #endregion

        #region Private Fields
        private NodeId m_applicationId;
        private string m_applicationUri;
        private ApplicationType m_applicationType;
        private LocalizedTextCollection m_applicationNames;
        private string m_productUri;
        private StringCollection m_discoveryUrls;
        private StringCollection m_serverCapabilities;
        #endregion
    }

    #region ApplicationRecordDataTypeCollection Class
    /// <summary>
    /// A collection of ApplicationRecordDataType objects.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    [CollectionDataContract(Name = "ListOfApplicationRecordDataType", Namespace = Opc.Ua.Gds.Namespaces.OpcUaGdsXsd, ItemName = "ApplicationRecordDataType")]
    #if !NET_STANDARD
    public partial class ApplicationRecordDataTypeCollection : List<ApplicationRecordDataType>, ICloneable
    #else
    public partial class ApplicationRecordDataTypeCollection : List<ApplicationRecordDataType>
    #endif
    {
        #region Constructors
        /// <summary>
        /// Initializes the collection with default values.
        /// </summary>
        public ApplicationRecordDataTypeCollection() {}

        /// <summary>
        /// Initializes the collection with an initial capacity.
        /// </summary>
        public ApplicationRecordDataTypeCollection(int capacity) : base(capacity) {}

        /// <summary>
        /// Initializes the collection with another collection.
        /// </summary>
        public ApplicationRecordDataTypeCollection(IEnumerable<ApplicationRecordDataType> collection) : base(collection) {}
        #endregion

        #region Static Operators
        /// <summary>
        /// Converts an array to a collection.
        /// </summary>
        public static implicit operator ApplicationRecordDataTypeCollection(ApplicationRecordDataType[] values)
        {
            if (values != null)
            {
                return new ApplicationRecordDataTypeCollection(values);
            }

            return new ApplicationRecordDataTypeCollection();
        }

        /// <summary>
        /// Converts a collection to an array.
        /// </summary>
        public static explicit operator ApplicationRecordDataType[](ApplicationRecordDataTypeCollection values)
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
            return (ApplicationRecordDataTypeCollection)this.MemberwiseClone();
        }
        #endregion
        #endif

        /// <summary cref="Object.MemberwiseClone" />
        public new object MemberwiseClone()
        {
            ApplicationRecordDataTypeCollection clone = new ApplicationRecordDataTypeCollection(this.Count);

            for (int ii = 0; ii < this.Count; ii++)
            {
                clone.Add((ApplicationRecordDataType)Utils.Clone(this[ii]));
            }

            return clone;
        }
    }
    #endregion
    #endif
    #endregion
}
