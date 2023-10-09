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

namespace Sercos
{
    #region FunctionalGroupState Class
    #if (!OPCUA_EXCLUDE_FunctionalGroupState)
    /// <summary>
    /// Stores an instance of the FunctionalGroupType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class FunctionalGroupState : FolderState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public FunctionalGroupState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Sercos.ObjectTypes.FunctionalGroupType, Sercos.Namespaces.Sercos, namespaceUris);
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
           "AgAAABUAAABodHRwOi8vc2VyY29zLm9yZy9VQS8fAAAAaHR0cDovL29wY2ZvdW5kYXRpb24ub3JnL1VB" +
           "L0RJL/////8EYIACAQAAAAEAGwAAAEZ1bmN0aW9uYWxHcm91cFR5cGVJbnN0YW5jZQEBfBcBAXwXfBcA" +
           "AP////8AAAAA";
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

    #region SercosProfileState Class
    #if (!OPCUA_EXCLUDE_SercosProfileState)
    /// <summary>
    /// Stores an instance of the SercosProfileType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class SercosProfileState : FunctionalGroupState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public SercosProfileState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Sercos.ObjectTypes.SercosProfileType, Sercos.Namespaces.Sercos, namespaceUris);
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
           "AgAAABUAAABodHRwOi8vc2VyY29zLm9yZy9VQS8fAAAAaHR0cDovL29wY2ZvdW5kYXRpb24ub3JnL1VB" +
           "L0RJL/////8EYIACAQAAAAEAGQAAAFNlcmNvc1Byb2ZpbGVUeXBlSW5zdGFuY2UBAeoDAQHqA+oDAAD/" +
           "////AAAAAA==";
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

    #region SercosClassState Class
    #if (!OPCUA_EXCLUDE_SercosClassState)
    /// <summary>
    /// Stores an instance of the SercosClassType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class SercosClassState : FunctionalGroupState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public SercosClassState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Sercos.ObjectTypes.SercosClassType, Sercos.Namespaces.Sercos, namespaceUris);
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
           "AgAAABUAAABodHRwOi8vc2VyY29zLm9yZy9VQS8fAAAAaHR0cDovL29wY2ZvdW5kYXRpb24ub3JnL1VB" +
           "L0RJL/////8EYIACAQAAAAEAFwAAAFNlcmNvc0NsYXNzVHlwZUluc3RhbmNlAQHrAwEB6wPrAwAA////" +
           "/wAAAAA=";
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

    #region SercosFunctionGroupState Class
    #if (!OPCUA_EXCLUDE_SercosFunctionGroupState)
    /// <summary>
    /// Stores an instance of the SercosFunctionGroupType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class SercosFunctionGroupState : FunctionalGroupState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public SercosFunctionGroupState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Sercos.ObjectTypes.SercosFunctionGroupType, Sercos.Namespaces.Sercos, namespaceUris);
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
           "AgAAABUAAABodHRwOi8vc2VyY29zLm9yZy9VQS8fAAAAaHR0cDovL29wY2ZvdW5kYXRpb24ub3JnL1VB" +
           "L0RJL/////8EYIACAQAAAAEAHwAAAFNlcmNvc0Z1bmN0aW9uR3JvdXBUeXBlSW5zdGFuY2UBAewDAQHs" +
           "A+wDAAD/////AAAAAA==";
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

    #region SercosDeviceState Class
    #if (!OPCUA_EXCLUDE_SercosDeviceState)
    /// <summary>
    /// Stores an instance of the SercosDeviceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class SercosDeviceState : DeviceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public SercosDeviceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Sercos.ObjectTypes.SercosDeviceType, Sercos.Namespaces.Sercos, namespaceUris);
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
           "AgAAABUAAABodHRwOi8vc2VyY29zLm9yZy9VQS8fAAAAaHR0cDovL29wY2ZvdW5kYXRpb24ub3JnL1VB" +
           "L0RJL/////8EYIACAQAAAAEAGAAAAFNlcmNvc0RldmljZVR5cGVJbnN0YW5jZQEB6QMBAekD6QMAAP//" +
           "//8MAAAAJGCACgEAAAABAAwAAABQYXJhbWV0ZXJTZXQBAY8TAwAAAAAXAAAARmxhdCBsaXN0IG9mIFBh" +
           "cmFtZXRlcnMALwEBfBePEwAA/////wAAAAA1YIkKAgAAAAIADAAAAE1hbnVmYWN0dXJlcgEBlBcDAAAA" +
           "ADAAAABOYW1lIG9mIHRoZSBjb21wYW55IHRoYXQgbWFudWZhY3R1cmVkIHRoZSBkZXZpY2UALgBElBcA" +
           "AAAV/////wEB/////wAAAAA1YIkKAgAAAAIABQAAAE1vZGVsAQGVFwMAAAAAGAAAAE1vZGVsIG5hbWUg" +
           "b2YgdGhlIGRldmljZQAuAESVFwAAABX/////AQH/////AAAAADVgiQoCAAAAAgAQAAAASGFyZHdhcmVS" +
           "ZXZpc2lvbgEBmRcDAAAAACwAAABSZXZpc2lvbiBsZXZlbCBvZiB0aGUgaGFyZHdhcmUgb2YgdGhlIGRl" +
           "dmljZQAuAESZFwAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAQAAAAU29mdHdhcmVSZXZpc2lvbgEB" +
           "mBcDAAAAADUAAABSZXZpc2lvbiBsZXZlbCBvZiB0aGUgc29mdHdhcmUvZmlybXdhcmUgb2YgdGhlIGRl" +
           "dmljZQAuAESYFwAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAOAAAARGV2aWNlUmV2aXNpb24BAZcX" +
           "AwAAAAAkAAAAT3ZlcmFsbCByZXZpc2lvbiBsZXZlbCBvZiB0aGUgZGV2aWNlAC4ARJcXAAAADP////8B" +
           "Af////8AAAAANWCJCgIAAAACAAwAAABEZXZpY2VNYW51YWwBAZYXAwAAAABaAAAAQWRkcmVzcyAocGF0" +
           "aG5hbWUgaW4gdGhlIGZpbGUgc3lzdGVtIG9yIGEgVVJMIHwgV2ViIGFkZHJlc3MpIG9mIHVzZXIgbWFu" +
           "dWFsIGZvciB0aGUgZGV2aWNlAC4ARJYXAAAADP////8BAf////8AAAAANWCJCgIAAAACAAwAAABTZXJp" +
           "YWxOdW1iZXIBAZIXAwAAAABNAAAASWRlbnRpZmllciB0aGF0IHVuaXF1ZWx5IGlkZW50aWZpZXMsIHdp" +
           "dGhpbiBhIG1hbnVmYWN0dXJlciwgYSBkZXZpY2UgaW5zdGFuY2UALgBEkhcAAAAM/////wEB/////wAA" +
           "AAA1YIkKAgAAAAIADwAAAFJldmlzaW9uQ291bnRlcgEBkxcDAAAAAGkAAABBbiBpbmNyZW1lbnRhbCBj" +
           "b3VudGVyIGluZGljYXRpbmcgdGhlIG51bWJlciBvZiB0aW1lcyB0aGUgc3RhdGljIGRhdGEgd2l0aGlu" +
           "IHRoZSBEZXZpY2UgaGFzIGJlZW4gbW9kaWZpZWQALgBEkxcAAAAG/////wEB/////wAAAAAEYIAKAQAA" +
           "AAEACgAAAFByb2ZpbGVTZXQBAYkTAC8BAXwXiRMAAP////8AAAAABGCACgEAAAABAAgAAABDbGFzc1Nl" +
           "dAEBihMALwEBfBeKEwAA/////wAAAAAEYIAKAQAAAAEAEAAAAEZ1bmN0aW9uR3JvdXBTZXQBAYsTAC8B" +
           "AXwXixMAAP////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public new FunctionalGroupState ParameterSet
        {
            get { return (FunctionalGroupState)base.ParameterSet; }
            set { base.ParameterSet = value; }
        }

        /// <remarks />
        public FunctionalGroupState ProfileSet
        {
            get
            {
                return m_profileSet;
            }

            set
            {
                if (!Object.ReferenceEquals(m_profileSet, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_profileSet = value;
            }
        }

        /// <remarks />
        public FunctionalGroupState ClassSet
        {
            get
            {
                return m_classSet;
            }

            set
            {
                if (!Object.ReferenceEquals(m_classSet, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_classSet = value;
            }
        }

        /// <remarks />
        public FunctionalGroupState FunctionGroupSet
        {
            get
            {
                return m_functionGroupSet;
            }

            set
            {
                if (!Object.ReferenceEquals(m_functionGroupSet, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_functionGroupSet = value;
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
            if (m_profileSet != null)
            {
                children.Add(m_profileSet);
            }

            if (m_classSet != null)
            {
                children.Add(m_classSet);
            }

            if (m_functionGroupSet != null)
            {
                children.Add(m_functionGroupSet);
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
                case Sercos.BrowseNames.ParameterSet:
                {
                    if (createOrReplace)
                    {
                        if (ParameterSet == null)
                        {
                            if (replacement == null)
                            {
                                ParameterSet = new FunctionalGroupState(this);
                            }
                            else
                            {
                                ParameterSet = (FunctionalGroupState)replacement;
                            }
                        }
                    }

                    instance = ParameterSet;
                    break;
                }

                case Sercos.BrowseNames.ProfileSet:
                {
                    if (createOrReplace)
                    {
                        if (ProfileSet == null)
                        {
                            if (replacement == null)
                            {
                                ProfileSet = new FunctionalGroupState(this);
                            }
                            else
                            {
                                ProfileSet = (FunctionalGroupState)replacement;
                            }
                        }
                    }

                    instance = ProfileSet;
                    break;
                }

                case Sercos.BrowseNames.ClassSet:
                {
                    if (createOrReplace)
                    {
                        if (ClassSet == null)
                        {
                            if (replacement == null)
                            {
                                ClassSet = new FunctionalGroupState(this);
                            }
                            else
                            {
                                ClassSet = (FunctionalGroupState)replacement;
                            }
                        }
                    }

                    instance = ClassSet;
                    break;
                }

                case Sercos.BrowseNames.FunctionGroupSet:
                {
                    if (createOrReplace)
                    {
                        if (FunctionGroupSet == null)
                        {
                            if (replacement == null)
                            {
                                FunctionGroupSet = new FunctionalGroupState(this);
                            }
                            else
                            {
                                FunctionGroupSet = (FunctionalGroupState)replacement;
                            }
                        }
                    }

                    instance = FunctionGroupSet;
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
        private FunctionalGroupState m_profileSet;
        private FunctionalGroupState m_classSet;
        private FunctionalGroupState m_functionGroupSet;
        #endregion
    }
    #endif
    #endregion

    #region ProfileSetState Class
    #if (!OPCUA_EXCLUDE_ProfileSetState)
    /// <summary>
    /// Stores an instance of the ProfileSet ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ProfileSetState : FunctionalGroupState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ProfileSetState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Sercos.ObjectTypes.ProfileSet, Sercos.Namespaces.Sercos, namespaceUris);
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
           "AgAAABUAAABodHRwOi8vc2VyY29zLm9yZy9VQS8fAAAAaHR0cDovL29wY2ZvdW5kYXRpb24ub3JnL1VB" +
           "L0RJL/////8EYIACAQAAAAEAEgAAAFByb2ZpbGVTZXRJbnN0YW5jZQEBuxcBAbsXuxcAAP////8AAAAA";
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

    #region ClassSetState Class
    #if (!OPCUA_EXCLUDE_ClassSetState)
    /// <summary>
    /// Stores an instance of the ClassSet ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ClassSetState : FunctionalGroupState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ClassSetState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Sercos.ObjectTypes.ClassSet, Sercos.Namespaces.Sercos, namespaceUris);
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
           "AgAAABUAAABodHRwOi8vc2VyY29zLm9yZy9VQS8fAAAAaHR0cDovL29wY2ZvdW5kYXRpb24ub3JnL1VB" +
           "L0RJL/////8EYIACAQAAAAEAEAAAAENsYXNzU2V0SW5zdGFuY2UBAb0XAQG9F70XAAD/////AAAAAA==";
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

    #region FunctionGroupSetState Class
    #if (!OPCUA_EXCLUDE_FunctionGroupSetState)
    /// <summary>
    /// Stores an instance of the FunctionGroupSet ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class FunctionGroupSetState : FunctionalGroupState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public FunctionGroupSetState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Sercos.ObjectTypes.FunctionGroupSet, Sercos.Namespaces.Sercos, namespaceUris);
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
           "AgAAABUAAABodHRwOi8vc2VyY29zLm9yZy9VQS8fAAAAaHR0cDovL29wY2ZvdW5kYXRpb24ub3JnL1VB" +
           "L0RJL/////8EYIACAQAAAAEAGAAAAEZ1bmN0aW9uR3JvdXBTZXRJbnN0YW5jZQEBvxcBAb8XvxcAAP//" +
           "//8AAAAA";
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

    #region SercosParameterState Class
    #if (!OPCUA_EXCLUDE_SercosParameterState)
    /// <summary>
    /// Stores an instance of the SercosParameterType VariableType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class SercosParameterState : BaseDataVariableState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public SercosParameterState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Sercos.VariableTypes.SercosParameterType, Sercos.Namespaces.Sercos, namespaceUris);
        }

        /// <summary>
        /// Returns the id of the default data type node for the instance.
        /// </summary>
        protected override NodeId GetDefaultDataTypeId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.DataTypes.BaseDataType, Opc.Ua.Namespaces.OpcUa, namespaceUris);
        }

        /// <summary>
        /// Returns the id of the default value rank for the instance.
        /// </summary>
        protected override int GetDefaultValueRank()
        {
            return ValueRanks.Any;
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

            if (DisplayMaxValue != null)
            {
                DisplayMaxValue.Initialize(context, DisplayMaxValue_InitializationString);
            }

            if (DisplayMinValue != null)
            {
                DisplayMinValue.Initialize(context, DisplayMinValue_InitializationString);
            }

            if (Exponent != null)
            {
                Exponent.Initialize(context, Exponent_InitializationString);
            }

            if (MaxValue != null)
            {
                MaxValue.Initialize(context, MaxValue_InitializationString);
            }

            if (MinValue != null)
            {
                MinValue.Initialize(context, MinValue_InitializationString);
            }

            if (ProcedureCommand != null)
            {
                ProcedureCommand.Initialize(context, ProcedureCommand_InitializationString);
            }
        }

        #region Initialization String
        private const string DisplayMaxValue_InitializationString =
           "AgAAABUAAABodHRwOi8vc2VyY29zLm9yZy9VQS8fAAAAaHR0cDovL29wY2ZvdW5kYXRpb24ub3JnL1VB" +
           "L0RJL/////8VYIkKAgAAAAEADwAAAERpc3BsYXlNYXhWYWx1ZQEBeBcALgBEeBcAAAAM/////wEB////" +
           "/wAAAAA=";

        private const string DisplayMinValue_InitializationString =
           "AgAAABUAAABodHRwOi8vc2VyY29zLm9yZy9VQS8fAAAAaHR0cDovL29wY2ZvdW5kYXRpb24ub3JnL1VB" +
           "L0RJL/////8VYIkKAgAAAAEADwAAAERpc3BsYXlNaW5WYWx1ZQEBdxcALgBEdxcAAAAM/////wEB////" +
           "/wAAAAA=";

        private const string Exponent_InitializationString =
           "AgAAABUAAABodHRwOi8vc2VyY29zLm9yZy9VQS8fAAAAaHR0cDovL29wY2ZvdW5kYXRpb24ub3JnL1VB" +
           "L0RJL/////8VYIkKAgAAAAEACAAAAEV4cG9uZW50AQF2FwAuAER2FwAAAAL/////AQH/////AAAAAA==";

        private const string MaxValue_InitializationString =
           "AgAAABUAAABodHRwOi8vc2VyY29zLm9yZy9VQS8fAAAAaHR0cDovL29wY2ZvdW5kYXRpb24ub3JnL1VB" +
           "L0RJL/////8VYIkKAgAAAAEACAAAAE1heFZhbHVlAQFxFwAuAERxFwAAABj/////AQH/////AAAAAA==";

        private const string MinValue_InitializationString =
           "AgAAABUAAABodHRwOi8vc2VyY29zLm9yZy9VQS8fAAAAaHR0cDovL29wY2ZvdW5kYXRpb24ub3JnL1VB" +
           "L0RJL/////8VYIkKAgAAAAEACAAAAE1pblZhbHVlAQFyFwAuAERyFwAAABj/////AQH/////AAAAAA==";

        private const string ProcedureCommand_InitializationString =
           "AgAAABUAAABodHRwOi8vc2VyY29zLm9yZy9VQS8fAAAAaHR0cDovL29wY2ZvdW5kYXRpb24ub3JnL1VB" +
           "L0RJL/////8VYIkKAgAAAAEAEAAAAFByb2NlZHVyZUNvbW1hbmQBAXUXAC4ARHUXAAAAAf////8BAf//" +
           "//8AAAAA";

        private const string InitializationString =
           "AgAAABUAAABodHRwOi8vc2VyY29zLm9yZy9VQS8fAAAAaHR0cDovL29wY2ZvdW5kYXRpb24ub3JnL1VB" +
           "L0RJL/////8VYIkCAgAAAAEAGwAAAFNlcmNvc1BhcmFtZXRlclR5cGVJbnN0YW5jZQEB0QcBAdEH0QcA" +
           "AAAY/v///wEB/////wgAAAAVYIkKAgAAAAEACQAAAEF0dHJpYnV0ZQEBdBcALgBEdBcAAAAH/////wEB" +
           "/////wAAAAAVYIkKAgAAAAEADAAAAERpc3BsYXlWYWx1ZQEBeRcALgBEeRcAAAAM/////wEB/////wAA" +
           "AAAVYIkKAgAAAAEADwAAAERpc3BsYXlNYXhWYWx1ZQEBeBcALgBEeBcAAAAM/////wEB/////wAAAAAV" +
           "YIkKAgAAAAEADwAAAERpc3BsYXlNaW5WYWx1ZQEBdxcALgBEdxcAAAAM/////wEB/////wAAAAAVYIkK" +
           "AgAAAAEACAAAAEV4cG9uZW50AQF2FwAuAER2FwAAAAL/////AQH/////AAAAABVgiQoCAAAAAQAIAAAA" +
           "TWF4VmFsdWUBAXEXAC4ARHEXAAAAGP////8BAf////8AAAAAFWCJCgIAAAABAAgAAABNaW5WYWx1ZQEB" +
           "chcALgBEchcAAAAY/////wEB/////wAAAAAVYIkKAgAAAAEAEAAAAFByb2NlZHVyZUNvbW1hbmQBAXUX" +
           "AC4ARHUXAAAAAf////8BAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public PropertyState<uint> Attribute
        {
            get
            {
                return m_attribute;
            }

            set
            {
                if (!Object.ReferenceEquals(m_attribute, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_attribute = value;
            }
        }

        /// <remarks />
        public PropertyState<string> DisplayValue
        {
            get
            {
                return m_displayValue;
            }

            set
            {
                if (!Object.ReferenceEquals(m_displayValue, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_displayValue = value;
            }
        }

        /// <remarks />
        public PropertyState<string> DisplayMaxValue
        {
            get
            {
                return m_displayMaxValue;
            }

            set
            {
                if (!Object.ReferenceEquals(m_displayMaxValue, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_displayMaxValue = value;
            }
        }

        /// <remarks />
        public PropertyState<string> DisplayMinValue
        {
            get
            {
                return m_displayMinValue;
            }

            set
            {
                if (!Object.ReferenceEquals(m_displayMinValue, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_displayMinValue = value;
            }
        }

        /// <remarks />
        public PropertyState<sbyte> Exponent
        {
            get
            {
                return m_exponent;
            }

            set
            {
                if (!Object.ReferenceEquals(m_exponent, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_exponent = value;
            }
        }

        /// <remarks />
        public PropertyState MaxValue
        {
            get
            {
                return m_maxValue;
            }

            set
            {
                if (!Object.ReferenceEquals(m_maxValue, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_maxValue = value;
            }
        }

        /// <remarks />
        public PropertyState MinValue
        {
            get
            {
                return m_minValue;
            }

            set
            {
                if (!Object.ReferenceEquals(m_minValue, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_minValue = value;
            }
        }

        /// <remarks />
        public PropertyState<bool> ProcedureCommand
        {
            get
            {
                return m_procedureCommand;
            }

            set
            {
                if (!Object.ReferenceEquals(m_procedureCommand, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_procedureCommand = value;
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
            if (m_attribute != null)
            {
                children.Add(m_attribute);
            }

            if (m_displayValue != null)
            {
                children.Add(m_displayValue);
            }

            if (m_displayMaxValue != null)
            {
                children.Add(m_displayMaxValue);
            }

            if (m_displayMinValue != null)
            {
                children.Add(m_displayMinValue);
            }

            if (m_exponent != null)
            {
                children.Add(m_exponent);
            }

            if (m_maxValue != null)
            {
                children.Add(m_maxValue);
            }

            if (m_minValue != null)
            {
                children.Add(m_minValue);
            }

            if (m_procedureCommand != null)
            {
                children.Add(m_procedureCommand);
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
                case Sercos.BrowseNames.Attribute:
                {
                    if (createOrReplace)
                    {
                        if (Attribute == null)
                        {
                            if (replacement == null)
                            {
                                Attribute = new PropertyState<uint>(this);
                            }
                            else
                            {
                                Attribute = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = Attribute;
                    break;
                }

                case Sercos.BrowseNames.DisplayValue:
                {
                    if (createOrReplace)
                    {
                        if (DisplayValue == null)
                        {
                            if (replacement == null)
                            {
                                DisplayValue = new PropertyState<string>(this);
                            }
                            else
                            {
                                DisplayValue = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = DisplayValue;
                    break;
                }

                case Sercos.BrowseNames.DisplayMaxValue:
                {
                    if (createOrReplace)
                    {
                        if (DisplayMaxValue == null)
                        {
                            if (replacement == null)
                            {
                                DisplayMaxValue = new PropertyState<string>(this);
                            }
                            else
                            {
                                DisplayMaxValue = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = DisplayMaxValue;
                    break;
                }

                case Sercos.BrowseNames.DisplayMinValue:
                {
                    if (createOrReplace)
                    {
                        if (DisplayMinValue == null)
                        {
                            if (replacement == null)
                            {
                                DisplayMinValue = new PropertyState<string>(this);
                            }
                            else
                            {
                                DisplayMinValue = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = DisplayMinValue;
                    break;
                }

                case Sercos.BrowseNames.Exponent:
                {
                    if (createOrReplace)
                    {
                        if (Exponent == null)
                        {
                            if (replacement == null)
                            {
                                Exponent = new PropertyState<sbyte>(this);
                            }
                            else
                            {
                                Exponent = (PropertyState<sbyte>)replacement;
                            }
                        }
                    }

                    instance = Exponent;
                    break;
                }

                case Sercos.BrowseNames.MaxValue:
                {
                    if (createOrReplace)
                    {
                        if (MaxValue == null)
                        {
                            if (replacement == null)
                            {
                                MaxValue = new PropertyState(this);
                            }
                            else
                            {
                                MaxValue = (PropertyState)replacement;
                            }
                        }
                    }

                    instance = MaxValue;
                    break;
                }

                case Sercos.BrowseNames.MinValue:
                {
                    if (createOrReplace)
                    {
                        if (MinValue == null)
                        {
                            if (replacement == null)
                            {
                                MinValue = new PropertyState(this);
                            }
                            else
                            {
                                MinValue = (PropertyState)replacement;
                            }
                        }
                    }

                    instance = MinValue;
                    break;
                }

                case Sercos.BrowseNames.ProcedureCommand:
                {
                    if (createOrReplace)
                    {
                        if (ProcedureCommand == null)
                        {
                            if (replacement == null)
                            {
                                ProcedureCommand = new PropertyState<bool>(this);
                            }
                            else
                            {
                                ProcedureCommand = (PropertyState<bool>)replacement;
                            }
                        }
                    }

                    instance = ProcedureCommand;
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
        private PropertyState<uint> m_attribute;
        private PropertyState<string> m_displayValue;
        private PropertyState<string> m_displayMaxValue;
        private PropertyState<string> m_displayMinValue;
        private PropertyState<sbyte> m_exponent;
        private PropertyState m_maxValue;
        private PropertyState m_minValue;
        private PropertyState<bool> m_procedureCommand;
        #endregion
    }

    #region SercosParameterState<T> Class
    /// <summary>
    /// A typed version of the SercosParameterType variable.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public class SercosParameterState<T> : SercosParameterState
    {
        #region Constructors
        /// <summary>
        /// Initializes the instance with its defalt attribute values.
        /// </summary>
        public SercosParameterState(NodeState parent) : base(parent)
        {
            Value = default(T);
        }

        /// <summary>
        /// Initializes the instance with the default values.
        /// </summary>
        protected override void Initialize(ISystemContext context)
        {
            base.Initialize(context);

            Value = default(T);
            DataType = TypeInfo.GetDataTypeId(typeof(T));
            ValueRank = TypeInfo.GetValueRank(typeof(T));
        }

        /// <summary>
        /// Initializes the instance with a node.
        /// </summary>
        protected override void Initialize(ISystemContext context, NodeState source)
        {
            InitializeOptionalChildren(context);
            base.Initialize(context, source);
        }
        #endregion

        #region Public Members
        /// <summary>
        /// The value of the variable.
        /// </summary>
        public new T Value
        {
            get
            {
                return CheckTypeBeforeCast<T>(base.Value, true);
            }

            set
            {
                base.Value = value;
            }
        }
        #endregion
    }
    #endregion
    #endif
    #endregion
}
