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
using Opc.Ua;

namespace Opc.MDIS
{
    #region EnableDisableMethodState Class
    #if (!OPCUA_EXCLUDE_EnableDisableMethodState)
    /// <summary>
    /// Stores an instance of the EnableDisableMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class EnableDisableMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public EnableDisableMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new EnableDisableMethodState(parent);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8EYYIKBAAAAAEAFwAAAEVu" +
           "YWJsZURpc2FibGVNZXRob2RUeXBlAQGdOgAvAQGdOp06AAABAf////8BAAAAF2CpCgIAAAAAAA4AAABJ" +
           "bnB1dEFyZ3VtZW50cwEBnjoALgBEnjoAAJYBAAAAAQAqAQFUAAAABgAAAEVuYWJsZQAB/////wAAAAAD" +
           "AAAAADcAAABEaXNhYmxlIHRoZSBkZXZpY2UgKGZhbHNlKSwgb3IgZW5hYmxlIHRoZSBkZXZpY2UgKHRy" +
           "dWUpAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public EnableDisableMethodStateMethodCallHandler OnCall;
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

            bool enable = (bool)_inputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    enable);
            }

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
    public delegate ServiceResult EnableDisableMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        bool enable);
    #endif
    #endregion

    #region MDISBaseObjectState Class
    #if (!OPCUA_EXCLUDE_MDISBaseObjectState)
    /// <summary>
    /// Stores an instance of the MDISBaseObjectType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class MDISBaseObjectState : BaseObjectState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public MDISBaseObjectState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.MDIS.ObjectTypes.MDISBaseObjectType, Opc.MDIS.Namespaces.MDIS, namespaceUris);
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

            if (FaultCode != null)
            {
                FaultCode.Initialize(context, FaultCode_InitializationString);
            }

            if (Warning != null)
            {
                Warning.Initialize(context, Warning_InitializationString);
            }

            if (WarningCode != null)
            {
                WarningCode.Initialize(context, WarningCode_InitializationString);
            }

            if (Enabled != null)
            {
                Enabled.Initialize(context, Enabled_InitializationString);
            }

            if (EnableDisable != null)
            {
                EnableDisable.Initialize(context, EnableDisable_InitializationString);
            }

            if (TagId != null)
            {
                TagId.Initialize(context, TagId_InitializationString);
            }
        }

        #region Initialization String
        private const string FaultCode_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEACQAAAEZh" +
           "dWx0Q29kZQEBjQQALwA/jQQAAAAH/////wEB/////wAAAAA=";

        private const string Warning_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEABwAAAFdh" +
           "cm5pbmcBAfEBAC8AP/EBAAAAAf////8BAf////8AAAAA";

        private const string WarningCode_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEACwAAAFdh" +
           "cm5pbmdDb2RlAQGOBAAvAD+OBAAAAAf/////AQH/////AAAAAA==";

        private const string Enabled_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEABwAAAEVu" +
           "YWJsZWQBAdwBAC8AP9wBAAAAAf////8BAf////8AAAAA";

        private const string EnableDisable_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8EYYIKBAAAAAEADQAAAEVu" +
           "YWJsZURpc2FibGUBAcMAAC8BAcMAwwAAAAEB/////wEAAAAXYKkKAgAAAAAADgAAAElucHV0QXJndW1l" +
           "bnRzAQHEAAAuAETEAAAAlgEAAAABACoBAVQAAAAGAAAARW5hYmxlAAH/////AAAAAAMAAAAANwAAAERp" +
           "c2FibGUgdGhlIGRldmljZSAoZmFsc2UpLCBvciBlbmFibGUgdGhlIGRldmljZSAodHJ1ZSkBACgBAQAA" +
           "AAEAAAAAAAAAAQH/////AAAAAA==";

        private const string TagId_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////91YIkKAgAAAAEABQAAAFRh" +
           "Z0lkAQHFAAMAAAAABgAAAFRhZyBJRAMAAAAAKAAAAFRoZSBpZCB1c2VkIGluIG90aGVyIHBhcnRzIG9m" +
           "IHRoZSBzeXN0ZW0ALgBExQAAAAAM/////wEB/////wAAAAA=";

        private const string InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////+EYIACAQAAAAEAGgAAAE1E" +
           "SVNCYXNlT2JqZWN0VHlwZUluc3RhbmNlAQHCAAEBwgDCAAAAAf////8HAAAAFWCJCgIAAAABAAUAAABG" +
           "YXVsdAEB6QEALwA/6QEAAAAB/////wEB/////wAAAAAVYIkKAgAAAAEACQAAAEZhdWx0Q29kZQEBjQQA" +
           "LwA/jQQAAAAH/////wEB/////wAAAAAVYIkKAgAAAAEABwAAAFdhcm5pbmcBAfEBAC8AP/EBAAAAAf//" +
           "//8BAf////8AAAAAFWCJCgIAAAABAAsAAABXYXJuaW5nQ29kZQEBjgQALwA/jgQAAAAH/////wEB////" +
           "/wAAAAAVYIkKAgAAAAEABwAAAEVuYWJsZWQBAdwBAC8AP9wBAAAAAf////8BAf////8AAAAABGGCCgQA" +
           "AAABAA0AAABFbmFibGVEaXNhYmxlAQHDAAAvAQHDAMMAAAABAf////8BAAAAF2CpCgIAAAAAAA4AAABJ" +
           "bnB1dEFyZ3VtZW50cwEBxAAALgBExAAAAJYBAAAAAQAqAQFUAAAABgAAAEVuYWJsZQAB/////wAAAAAD" +
           "AAAAADcAAABEaXNhYmxlIHRoZSBkZXZpY2UgKGZhbHNlKSwgb3IgZW5hYmxlIHRoZSBkZXZpY2UgKHRy" +
           "dWUpAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAB1YIkKAgAAAAEABQAAAFRhZ0lkAQHFAAMAAAAABgAA" +
           "AFRhZyBJRAMAAAAAKAAAAFRoZSBpZCB1c2VkIGluIG90aGVyIHBhcnRzIG9mIHRoZSBzeXN0ZW0ALgBE" +
           "xQAAAAAM/////wEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public BaseDataVariableState<bool> Fault
        {
            get
            {
                return m_fault;
            }

            set
            {
                if (!Object.ReferenceEquals(m_fault, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_fault = value;
            }
        }

        /// <remarks />
        public BaseDataVariableState<uint> FaultCode
        {
            get
            {
                return m_faultCode;
            }

            set
            {
                if (!Object.ReferenceEquals(m_faultCode, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_faultCode = value;
            }
        }

        /// <remarks />
        public BaseDataVariableState<bool> Warning
        {
            get
            {
                return m_warning;
            }

            set
            {
                if (!Object.ReferenceEquals(m_warning, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_warning = value;
            }
        }

        /// <remarks />
        public BaseDataVariableState<uint> WarningCode
        {
            get
            {
                return m_warningCode;
            }

            set
            {
                if (!Object.ReferenceEquals(m_warningCode, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_warningCode = value;
            }
        }

        /// <remarks />
        public BaseDataVariableState<bool> Enabled
        {
            get
            {
                return m_enabled;
            }

            set
            {
                if (!Object.ReferenceEquals(m_enabled, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_enabled = value;
            }
        }

        /// <remarks />
        public EnableDisableMethodState EnableDisable
        {
            get
            {
                return m_enableDisableMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_enableDisableMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_enableDisableMethod = value;
            }
        }

        /// <remarks />
        public PropertyState<string> TagId
        {
            get
            {
                return m_tagId;
            }

            set
            {
                if (!Object.ReferenceEquals(m_tagId, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_tagId = value;
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
            if (m_fault != null)
            {
                children.Add(m_fault);
            }

            if (m_faultCode != null)
            {
                children.Add(m_faultCode);
            }

            if (m_warning != null)
            {
                children.Add(m_warning);
            }

            if (m_warningCode != null)
            {
                children.Add(m_warningCode);
            }

            if (m_enabled != null)
            {
                children.Add(m_enabled);
            }

            if (m_enableDisableMethod != null)
            {
                children.Add(m_enableDisableMethod);
            }

            if (m_tagId != null)
            {
                children.Add(m_tagId);
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
                case Opc.MDIS.BrowseNames.Fault:
                {
                    if (createOrReplace)
                    {
                        if (Fault == null)
                        {
                            if (replacement == null)
                            {
                                Fault = new BaseDataVariableState<bool>(this);
                            }
                            else
                            {
                                Fault = (BaseDataVariableState<bool>)replacement;
                            }
                        }
                    }

                    instance = Fault;
                    break;
                }

                case Opc.MDIS.BrowseNames.FaultCode:
                {
                    if (createOrReplace)
                    {
                        if (FaultCode == null)
                        {
                            if (replacement == null)
                            {
                                FaultCode = new BaseDataVariableState<uint>(this);
                            }
                            else
                            {
                                FaultCode = (BaseDataVariableState<uint>)replacement;
                            }
                        }
                    }

                    instance = FaultCode;
                    break;
                }

                case Opc.MDIS.BrowseNames.Warning:
                {
                    if (createOrReplace)
                    {
                        if (Warning == null)
                        {
                            if (replacement == null)
                            {
                                Warning = new BaseDataVariableState<bool>(this);
                            }
                            else
                            {
                                Warning = (BaseDataVariableState<bool>)replacement;
                            }
                        }
                    }

                    instance = Warning;
                    break;
                }

                case Opc.MDIS.BrowseNames.WarningCode:
                {
                    if (createOrReplace)
                    {
                        if (WarningCode == null)
                        {
                            if (replacement == null)
                            {
                                WarningCode = new BaseDataVariableState<uint>(this);
                            }
                            else
                            {
                                WarningCode = (BaseDataVariableState<uint>)replacement;
                            }
                        }
                    }

                    instance = WarningCode;
                    break;
                }

                case Opc.MDIS.BrowseNames.Enabled:
                {
                    if (createOrReplace)
                    {
                        if (Enabled == null)
                        {
                            if (replacement == null)
                            {
                                Enabled = new BaseDataVariableState<bool>(this);
                            }
                            else
                            {
                                Enabled = (BaseDataVariableState<bool>)replacement;
                            }
                        }
                    }

                    instance = Enabled;
                    break;
                }

                case Opc.MDIS.BrowseNames.EnableDisable:
                {
                    if (createOrReplace)
                    {
                        if (EnableDisable == null)
                        {
                            if (replacement == null)
                            {
                                EnableDisable = new EnableDisableMethodState(this);
                            }
                            else
                            {
                                EnableDisable = (EnableDisableMethodState)replacement;
                            }
                        }
                    }

                    instance = EnableDisable;
                    break;
                }

                case Opc.MDIS.BrowseNames.TagId:
                {
                    if (createOrReplace)
                    {
                        if (TagId == null)
                        {
                            if (replacement == null)
                            {
                                TagId = new PropertyState<string>(this);
                            }
                            else
                            {
                                TagId = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = TagId;
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
        private BaseDataVariableState<bool> m_fault;
        private BaseDataVariableState<uint> m_faultCode;
        private BaseDataVariableState<bool> m_warning;
        private BaseDataVariableState<uint> m_warningCode;
        private BaseDataVariableState<bool> m_enabled;
        private EnableDisableMethodState m_enableDisableMethod;
        private PropertyState<string> m_tagId;
        #endregion
    }
    #endif
    #endregion

    #region InterlockVariableState Class
    #if (!OPCUA_EXCLUDE_InterlockVariableState)
    /// <summary>
    /// Stores an instance of the InterlockVariableType VariableType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class InterlockVariableState : BaseDataVariableState<bool>
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public InterlockVariableState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.MDIS.VariableTypes.InterlockVariableType, Opc.MDIS.Namespaces.MDIS, namespaceUris);
        }

        /// <summary>
        /// Returns the id of the default data type node for the instance.
        /// </summary>
        protected override NodeId GetDefaultDataTypeId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.DataTypes.Boolean, Opc.Ua.Namespaces.OpcUa, namespaceUris);
        }

        /// <summary>
        /// Returns the id of the default value rank for the instance.
        /// </summary>
        protected override int GetDefaultValueRank()
        {
            return ValueRanks.Scalar;
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkCAgAAAAEAHQAAAElu" +
           "dGVybG9ja1ZhcmlhYmxlVHlwZUluc3RhbmNlAQH/BAEB/wT/BAAAAAH/////AQH/////AAAAAA==";
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

    #region MDISVersionVariableState Class
    #if (!OPCUA_EXCLUDE_MDISVersionVariableState)
    /// <summary>
    /// Stores an instance of the MDISVersionVariableType VariableType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class MDISVersionVariableState : BaseDataVariableState<MDISVersionDataType>
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public MDISVersionVariableState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.MDIS.VariableTypes.MDISVersionVariableType, Opc.MDIS.Namespaces.MDIS, namespaceUris);
        }

        /// <summary>
        /// Returns the id of the default data type node for the instance.
        /// </summary>
        protected override NodeId GetDefaultDataTypeId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.MDIS.DataTypes.MDISVersionDataType, Opc.MDIS.Namespaces.MDIS, namespaceUris);
        }

        /// <summary>
        /// Returns the id of the default value rank for the instance.
        /// </summary>
        protected override int GetDefaultValueRank()
        {
            return ValueRanks.Scalar;
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkCAgAAAAEAHwAAAE1E" +
           "SVNWZXJzaW9uVmFyaWFibGVUeXBlSW5zdGFuY2UBAQoFAQEKBQoFAAABAQkF/////wEB/////wMAAAAV" +
           "YIkKAgAAAAEADAAAAE1ham9yVmVyc2lvbgEBCwUALgA/CwUAAAAD/////wEB/////wAAAAAVYIkKAgAA" +
           "AAEADAAAAE1pbm9yVmVyc2lvbgEBDAUALgA/DAUAAAAD/////wEB/////wAAAAAVYIkKAgAAAAEABQAA" +
           "AEJ1aWxkAQENBQAuAD8NBQAAAAP/////AQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public BaseDataVariableState<byte> MajorVersion
        {
            get
            {
                return m_majorVersion;
            }

            set
            {
                if (!Object.ReferenceEquals(m_majorVersion, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_majorVersion = value;
            }
        }

        /// <remarks />
        public BaseDataVariableState<byte> MinorVersion
        {
            get
            {
                return m_minorVersion;
            }

            set
            {
                if (!Object.ReferenceEquals(m_minorVersion, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_minorVersion = value;
            }
        }

        /// <remarks />
        public BaseDataVariableState<byte> Build
        {
            get
            {
                return m_build;
            }

            set
            {
                if (!Object.ReferenceEquals(m_build, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_build = value;
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
            if (m_majorVersion != null)
            {
                children.Add(m_majorVersion);
            }

            if (m_minorVersion != null)
            {
                children.Add(m_minorVersion);
            }

            if (m_build != null)
            {
                children.Add(m_build);
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
                case Opc.MDIS.BrowseNames.MajorVersion:
                {
                    if (createOrReplace)
                    {
                        if (MajorVersion == null)
                        {
                            if (replacement == null)
                            {
                                MajorVersion = new BaseDataVariableState<byte>(this);
                            }
                            else
                            {
                                MajorVersion = (BaseDataVariableState<byte>)replacement;
                            }
                        }
                    }

                    instance = MajorVersion;
                    break;
                }

                case Opc.MDIS.BrowseNames.MinorVersion:
                {
                    if (createOrReplace)
                    {
                        if (MinorVersion == null)
                        {
                            if (replacement == null)
                            {
                                MinorVersion = new BaseDataVariableState<byte>(this);
                            }
                            else
                            {
                                MinorVersion = (BaseDataVariableState<byte>)replacement;
                            }
                        }
                    }

                    instance = MinorVersion;
                    break;
                }

                case Opc.MDIS.BrowseNames.Build:
                {
                    if (createOrReplace)
                    {
                        if (Build == null)
                        {
                            if (replacement == null)
                            {
                                Build = new BaseDataVariableState<byte>(this);
                            }
                            else
                            {
                                Build = (BaseDataVariableState<byte>)replacement;
                            }
                        }
                    }

                    instance = Build;
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
        private BaseDataVariableState<byte> m_majorVersion;
        private BaseDataVariableState<byte> m_minorVersion;
        private BaseDataVariableState<byte> m_build;
        #endregion
    }

    #region MDISVersionVariableValue Class
    /// <summary>
    /// A typed version of the _BrowseName_ variable.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public class MDISVersionVariableValue : BaseVariableValue
    {
        #region Constructors
        /// <summary>
        /// Initializes the instance with its defalt attribute values.
        /// </summary>
        public MDISVersionVariableValue(MDISVersionVariableState variable, MDISVersionDataType value, object dataLock) : base(dataLock)
        {
            m_value = value;

            if (m_value == null)
            {
                m_value = new MDISVersionDataType();
            }

            Initialize(variable);
        }
        #endregion

        #region Public Members
        /// <summary>
        /// The variable that the value belongs to.
        /// </summary>
        public MDISVersionVariableState Variable
        {
            get { return m_variable; }
        }

        /// <summary>
        /// The value of the variable.
        /// </summary>
        public MDISVersionDataType Value
        {
            get { return m_value;  }
            set { m_value = value; }
        }
        #endregion

        #region Private Methods
        /// <summary>
        /// Initializes the object.
        /// </summary>
        private void Initialize(MDISVersionVariableState variable)
        {
            lock (Lock)
            {
                m_variable = variable;

                variable.Value = m_value;

                variable.OnReadValue = OnReadValue;
                variable.OnSimpleWriteValue = OnWriteValue;

                BaseVariableState instance = null;
                List<BaseInstanceState> updateList = new List<BaseInstanceState>();
                updateList.Add(variable);

                instance = m_variable.MajorVersion;
                instance.OnReadValue = OnRead_MajorVersion;
                instance.OnSimpleWriteValue = OnWrite_MajorVersion;
                updateList.Add(instance);
                instance = m_variable.MinorVersion;
                instance.OnReadValue = OnRead_MinorVersion;
                instance.OnSimpleWriteValue = OnWrite_MinorVersion;
                updateList.Add(instance);
                instance = m_variable.Build;
                instance.OnReadValue = OnRead_Build;
                instance.OnSimpleWriteValue = OnWrite_Build;
                updateList.Add(instance);

                SetUpdateList(updateList);
            }
        }

        /// <summary>
        /// Reads the value of the variable.
        /// </summary>
        protected ServiceResult OnReadValue(
            ISystemContext context,
            NodeState node,
            NumericRange indexRange,
            QualifiedName dataEncoding,
            ref object value,
            ref StatusCode statusCode,
            ref DateTime timestamp)
        {
            lock (Lock)
            {
                DoBeforeReadProcessing(context, node);

                if (m_value != null)
                {
                    value = m_value;
                }

                return Read(context, node, indexRange, dataEncoding, ref value, ref statusCode, ref timestamp);
            }
        }

        /// <summary>
        /// Writes the value of the variable.
        /// </summary>
        private ServiceResult OnWriteValue(ISystemContext context, NodeState node, ref object value)
        {
            lock (Lock)
            {
                m_value = (MDISVersionDataType)Write(value);
            }

            return ServiceResult.Good;
        }

        #region MajorVersion Access Methods
        /// <summary>
        /// Reads the value of the variable child.
        /// </summary>
        private ServiceResult OnRead_MajorVersion(
            ISystemContext context,
            NodeState node,
            NumericRange indexRange,
            QualifiedName dataEncoding,
            ref object value,
            ref StatusCode statusCode,
            ref DateTime timestamp)
        {
            lock (Lock)
            {
                DoBeforeReadProcessing(context, node);

                if (m_value != null)
                {
                    value = m_value.MajorVersion;
                }

                return Read(context, node, indexRange, dataEncoding, ref value, ref statusCode, ref timestamp);
            }
        }

        /// <summary>
        /// Writes the value of the variable child.
        /// </summary>
        private ServiceResult OnWrite_MajorVersion(ISystemContext context, NodeState node, ref object value)
        {
            lock (Lock)
            {
                m_value.MajorVersion = (byte)Write(value);
            }

            return ServiceResult.Good;
        }
        #endregion

        #region MinorVersion Access Methods
        /// <summary>
        /// Reads the value of the variable child.
        /// </summary>
        private ServiceResult OnRead_MinorVersion(
            ISystemContext context,
            NodeState node,
            NumericRange indexRange,
            QualifiedName dataEncoding,
            ref object value,
            ref StatusCode statusCode,
            ref DateTime timestamp)
        {
            lock (Lock)
            {
                DoBeforeReadProcessing(context, node);

                if (m_value != null)
                {
                    value = m_value.MinorVersion;
                }

                return Read(context, node, indexRange, dataEncoding, ref value, ref statusCode, ref timestamp);
            }
        }

        /// <summary>
        /// Writes the value of the variable child.
        /// </summary>
        private ServiceResult OnWrite_MinorVersion(ISystemContext context, NodeState node, ref object value)
        {
            lock (Lock)
            {
                m_value.MinorVersion = (byte)Write(value);
            }

            return ServiceResult.Good;
        }
        #endregion

        #region Build Access Methods
        /// <summary>
        /// Reads the value of the variable child.
        /// </summary>
        private ServiceResult OnRead_Build(
            ISystemContext context,
            NodeState node,
            NumericRange indexRange,
            QualifiedName dataEncoding,
            ref object value,
            ref StatusCode statusCode,
            ref DateTime timestamp)
        {
            lock (Lock)
            {
                DoBeforeReadProcessing(context, node);

                if (m_value != null)
                {
                    value = m_value.Build;
                }

                return Read(context, node, indexRange, dataEncoding, ref value, ref statusCode, ref timestamp);
            }
        }

        /// <summary>
        /// Writes the value of the variable child.
        /// </summary>
        private ServiceResult OnWrite_Build(ISystemContext context, NodeState node, ref object value)
        {
            lock (Lock)
            {
                m_value.Build = (byte)Write(value);
            }

            return ServiceResult.Good;
        }
        #endregion
        #endregion

        #region Private Fields
        private MDISVersionDataType m_value;
        private MDISVersionVariableState m_variable;
        #endregion
    }
    #endregion
    #endif
    #endregion

    #region MoveMethodState Class
    #if (!OPCUA_EXCLUDE_MoveMethodState)
    /// <summary>
    /// Stores an instance of the MoveMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class MoveMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public MoveMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new MoveMethodState(parent);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8EYYIKBAAAAAEADgAAAE1v" +
           "dmVNZXRob2RUeXBlAQGfOgAvAQGfOp86AAABAf////8BAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3Vt" +
           "ZW50cwEBoDoALgBEoDoAAJYFAAAAAQAqAQF6AAAACQAAAERpcmVjdGlvbgEBAwD/////AAAAAAMAAAAA" +
           "WAAAAFRoZSBlbnVtZXJhdGlvbiBpbmRpY2F0ZXMgd2hldGhlciB0aGUgY29tbWFuZCBpcyB0byBvcGVu" +
           "IHRoZSB2YWx2ZSBvciB0byBjbG9zZSB0aGUgdmFsdmUBACoBAYIAAAARAAAAT3ZlcnJpZGVJbnRlcmxv" +
           "Y2sAAf////8AAAAAAwAAAABaAAAAQm9vbGVhbiBpbmRpY2F0aW5nIGlmIHRoZSBvcGVuIG9yIGNsb3Nl" +
           "IGNvbW1hbmQgc2hvdWxkIG92ZXJyaWRlIGFueSBkZWZlYXQgYWJsZSBpbnRlcmxvY2tzAQAqAQFPAAAA" +
           "AwAAAFNFTQEBBQD/////AAAAAAMAAAAAMwAAAFRoZSBzZWxlY3Rpb24gb2Ygd2hpY2ggU0VNIHRvIHNl" +
           "bmQgdGhlIGNvbW1hbmQgdG8uIAEAKgEBcQAAAAkAAABTaWduYXR1cmUAAf////8AAAAAAwAAAABRAAAA" +
           "Qm9vbGVhbiBpbmRpY2F0aW5nIGlmIGEgcHJvZmlsZSBzaG91bGQgYmUgZ2VuZXJhdGVkIGJ5IHRoaXMg" +
           "bW92ZSBjb21tYW5kIHJlcXVlc3QuAQAqAQFlAAAADwAAAFNodXRkb3duUmVxdWVzdAAB/////wAAAAAD" +
           "AAAAAD8AAABCb29sZWFuIGluZGljYXRlcyB0aGF0IHRoaXMgY29tbWFuZCBpcyBhIHNodXRkb3duIG1v" +
           "dmUgY29tbWFuZC4BACgBAQAAAAEAAAAAAAAAAQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public MoveMethodStateMethodCallHandler OnCall;
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

            CommandEnum direction = (CommandEnum)_inputArguments[0];
            bool overrideInterlock = (bool)_inputArguments[1];
            SEMEnum sEM = (SEMEnum)_inputArguments[2];
            bool signature = (bool)_inputArguments[3];
            bool shutdownRequest = (bool)_inputArguments[4];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    direction,
                    overrideInterlock,
                    sEM,
                    signature,
                    shutdownRequest);
            }

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
    public delegate ServiceResult MoveMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        CommandEnum direction,
        bool overrideInterlock,
        SEMEnum sEM,
        bool signature,
        bool shutdownRequest);
    #endif
    #endregion

    #region ChokeMoveMethodState Class
    #if (!OPCUA_EXCLUDE_ChokeMoveMethodState)
    /// <summary>
    /// Stores an instance of the ChokeMoveMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ChokeMoveMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ChokeMoveMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new ChokeMoveMethodState(parent);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8EYYIKBAAAAAEAEwAAAENo" +
           "b2tlTW92ZU1ldGhvZFR5cGUBAaE6AC8BAaE6oToAAAEB/////wEAAAAXYKkKAgAAAAAADgAAAElucHV0" +
           "QXJndW1lbnRzAQGiOgAuAESiOgAAlgMAAAABACoBAVEAAAAIAAAAUG9zaXRpb24ACv////8AAAAAAwAA" +
           "AAAyAAAAQSBudW1iZXIgKGluIHByZXJjZW50KSBpbmRpY2F0aW5nIHRoZSBwZXJjZW50IG9wZW4BACoB" +
           "AYIAAAARAAAAT3ZlcnJpZGVJbnRlcmxvY2sAAf////8AAAAAAwAAAABaAAAAQm9vbGVhbiBpbmRpY2F0" +
           "aW5nIGlmIHRoZSBvcGVuIG9yIGNsb3NlIGNvbW1hbmQgc2hvdWxkIG92ZXJyaWRlIGFueSBkZWZlYXQg" +
           "YWJsZSBpbnRlcmxvY2tzAQAqAQFPAAAAAwAAAFNFTQEBBQD/////AAAAAAMAAAAAMwAAAFRoZSBzZWxl" +
           "Y3Rpb24gb2Ygd2hpY2ggU0VNIHRvIHNlbmQgdGhlIGNvbW1hbmQgdG8uIAEAKAEBAAAAAQAAAAAAAAAB" +
           "Af////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public ChokeMoveMethodStateMethodCallHandler OnCall;
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

            float position = (float)_inputArguments[0];
            bool overrideInterlock = (bool)_inputArguments[1];
            SEMEnum sEM = (SEMEnum)_inputArguments[2];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    position,
                    overrideInterlock,
                    sEM);
            }

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
    public delegate ServiceResult ChokeMoveMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        float position,
        bool overrideInterlock,
        SEMEnum sEM);
    #endif
    #endregion

    #region ChokeStepMethodState Class
    #if (!OPCUA_EXCLUDE_ChokeStepMethodState)
    /// <summary>
    /// Stores an instance of the ChokeStepMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ChokeStepMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ChokeStepMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new ChokeStepMethodState(parent);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8EYYIKBAAAAAEAEwAAAENo" +
           "b2tlU3RlcE1ldGhvZFR5cGUBAaM6AC8BAaM6ozoAAAEB/////wEAAAAXYKkKAgAAAAAADgAAAElucHV0" +
           "QXJndW1lbnRzAQGkOgAuAESkOgAAlgQAAAABACoBAVUAAAAJAAAARGlyZWN0aW9uAQG9Av////8AAAAA" +
           "AwAAAAAzAAAAdHJ1ZSBpcyBvcGVuaW5nIGEgdmFsdmUsIGZhbHNlIGlmIGNsb3NpbmcgdGhlIHZhbHZl" +
           "AQAqAQFDAAAABQAAAFN0ZXBzAAX/////AAAAAAMAAAAAJwAAAG51bWJlciBvZiBzdGVwcyB0byBvcGVu" +
           "L2Nsb3NlIHRoZSB2YWx2ZQEAKgEBggAAABEAAABPdmVycmlkZUludGVybG9jawAB/////wAAAAADAAAA" +
           "AFoAAABCb29sZWFuIGluZGljYXRpbmcgaWYgdGhlIG9wZW4gb3IgY2xvc2UgY29tbWFuZCBzaG91bGQg" +
           "b3ZlcnJpZGUgYW55IGRlZmVhdCBhYmxlIGludGVybG9ja3MBACoBAU8AAAADAAAAU0VNAQEFAP////8A" +
           "AAAAAwAAAAAzAAAAVGhlIHNlbGVjdGlvbiBvZiB3aGljaCBTRU0gdG8gc2VuZCB0aGUgY29tbWFuZCB0" +
           "by4gAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public ChokeStepMethodStateMethodCallHandler OnCall;
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

            ChokeCommandEnum direction = (ChokeCommandEnum)_inputArguments[0];
            ushort steps = (ushort)_inputArguments[1];
            bool overrideInterlock = (bool)_inputArguments[2];
            SEMEnum sEM = (SEMEnum)_inputArguments[3];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    direction,
                    steps,
                    overrideInterlock,
                    sEM);
            }

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
    public delegate ServiceResult ChokeStepMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        ChokeCommandEnum direction,
        ushort steps,
        bool overrideInterlock,
        SEMEnum sEM);
    #endif
    #endregion

    #region ChokeSetCalculatedPositionMethodState Class
    #if (!OPCUA_EXCLUDE_ChokeSetCalculatedPositionMethodState)
    /// <summary>
    /// Stores an instance of the ChokeSetCalculatedPositionMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ChokeSetCalculatedPositionMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ChokeSetCalculatedPositionMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new ChokeSetCalculatedPositionMethodState(parent);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8EYYIKBAAAAAEAJAAAAENo" +
           "b2tlU2V0Q2FsY3VsYXRlZFBvc2l0aW9uTWV0aG9kVHlwZQEBpjoALwEBpjqmOgAAAQH/////AQAAABdg" +
           "qQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAac6AC4ARKc6AACWAQAAAAEAKgEBUAAAAAgAAABQb3Np" +
           "dGlvbgAK/////wAAAAADAAAAADEAAABBIG51bWJlciAoaW4gcGVyY2VudCkgaW5kaWNhdGluZyB0aGUg" +
           "cGVyY2VudCBvcGVuAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public ChokeSetCalculatedPositionMethodStateMethodCallHandler OnCall;
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

            float position = (float)_inputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    position);
            }

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
    public delegate ServiceResult ChokeSetCalculatedPositionMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        float position);
    #endif
    #endregion

    #region WriteStateMethodState Class
    #if (!OPCUA_EXCLUDE_WriteStateMethodState)
    /// <summary>
    /// Stores an instance of the WriteStateMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class WriteStateMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public WriteStateMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new WriteStateMethodState(parent);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8EYYIKBAAAAAEAFAAAAFdy" +
           "aXRlU3RhdGVNZXRob2RUeXBlAQGoOgAvAQGoOqg6AAABAf////8BAAAAF2CpCgIAAAAAAA4AAABJbnB1" +
           "dEFyZ3VtZW50cwEBqToALgBEqToAAJYBAAAAAQAqAQFNAAAABQAAAFN0YXRlAAH/////AAAAAAMAAAAA" +
           "MQAAAEJvb2xlYW4gc3RhdGUgdGhhdCBpcyBiZWluZyB3cml0dGVuIHRvIHRoZSBvYmplY3QBACgBAQAA" +
           "AAEAAAAAAAAAAQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public WriteStateMethodStateMethodCallHandler OnCall;
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

            bool state = (bool)_inputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    state);
            }

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
    public delegate ServiceResult WriteStateMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        bool state);
    #endif
    #endregion

    #region WriteValueMethodState Class
    #if (!OPCUA_EXCLUDE_WriteValueMethodState)
    /// <summary>
    /// Stores an instance of the WriteValueMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class WriteValueMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public WriteValueMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new WriteValueMethodState(parent);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8EYYIKBAAAAAEAFAAAAFdy" +
           "aXRlVmFsdWVNZXRob2RUeXBlAQGqOgAvAQGqOqo6AAABAf////8BAAAAF2CpCgIAAAAAAA4AAABJbnB1" +
           "dEFyZ3VtZW50cwEBqzoALgBEqzoAAJYBAAAAAQAqAQFMAAAABQAAAFN0YXRlAAf/////AAAAAAMAAAAA" +
           "MAAAAFVuaXQzMiBzdGF0ZSB0aGF0IGlzIGJlaW5nIHdyaXR0ZW4gdG8gdGhlIG9iamVjdAEAKAEBAAAA" +
           "AQAAAAAAAAABAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public WriteValueMethodStateMethodCallHandler OnCall;
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

            uint state = (uint)_inputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    state);
            }

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
    public delegate ServiceResult WriteValueMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        uint state);
    #endif
    #endregion

    #region WriteInstrumentValueMethodState Class
    #if (!OPCUA_EXCLUDE_WriteInstrumentValueMethodState)
    /// <summary>
    /// Stores an instance of the WriteInstrumentValueMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class WriteInstrumentValueMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public WriteInstrumentValueMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new WriteInstrumentValueMethodState(parent);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8EYYIKBAAAAAEAHgAAAFdy" +
           "aXRlSW5zdHJ1bWVudFZhbHVlTWV0aG9kVHlwZQEBrDoALwEBrDqsOgAAAQH/////AQAAABdgqQoCAAAA" +
           "AAAOAAAASW5wdXRBcmd1bWVudHMBAa06AC4ARK06AACWAQAAAAEAKgEBSwAAAAUAAABWYWx1ZQAK////" +
           "/wAAAAADAAAAAC8AAABGbG9hdCB2YWx1ZSB0aGF0IGlzIGJlaW5nIHdyaXR0ZW4gdG8gdGhlIG9iamVj" +
           "dAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public WriteInstrumentValueMethodStateMethodCallHandler OnCall;
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

            float value = (float)_inputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    value);
            }

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
    public delegate ServiceResult WriteInstrumentValueMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        float value);
    #endif
    #endregion

    #region MDISValveObjectState Class
    #if (!OPCUA_EXCLUDE_MDISValveObjectState)
    /// <summary>
    /// Stores an instance of the MDISValveObjectType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class MDISValveObjectState : MDISBaseObjectState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public MDISValveObjectState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.MDIS.ObjectTypes.MDISValveObjectType, Opc.MDIS.Namespaces.MDIS, namespaceUris);
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

            if (CommandRejected != null)
            {
                CommandRejected.Initialize(context, CommandRejected_InitializationString);
            }

            if (SignatureRequestStatus != null)
            {
                SignatureRequestStatus.Initialize(context, SignatureRequestStatus_InitializationString);
            }

            if (LastCommand != null)
            {
                LastCommand.Initialize(context, LastCommand_InitializationString);
            }

            if (NonDefeatableOpenInterlock != null)
            {
                NonDefeatableOpenInterlock.Initialize(context, NonDefeatableOpenInterlock_InitializationString);
            }

            if (DefeatableOpenInterlock != null)
            {
                DefeatableOpenInterlock.Initialize(context, DefeatableOpenInterlock_InitializationString);
            }

            if (NonDefeatableCloseInterlock != null)
            {
                NonDefeatableCloseInterlock.Initialize(context, NonDefeatableCloseInterlock_InitializationString);
            }

            if (DefeatableCloseInterlock != null)
            {
                DefeatableCloseInterlock.Initialize(context, DefeatableCloseInterlock_InitializationString);
            }

            if (OpenTimeDuration != null)
            {
                OpenTimeDuration.Initialize(context, OpenTimeDuration_InitializationString);
            }

            if (CloseTimeDuration != null)
            {
                CloseTimeDuration.Initialize(context, CloseTimeDuration_InitializationString);
            }
        }

        #region Initialization String
        private const string CommandRejected_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEADwAAAENv" +
           "bW1hbmRSZWplY3RlZAEBbAMALwA/bAMAAAAB/////wEB/////wAAAAA=";

        private const string SignatureRequestStatus_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEAFgAAAFNp" +
           "Z25hdHVyZVJlcXVlc3RTdGF0dXMBAW0DAC8AP20DAAABAbsC/////wEB/////wAAAAA=";

        private const string LastCommand_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEACwAAAExh" +
           "c3RDb21tYW5kAQFuAwAvAD9uAwAAAQEDAP////8BAf////8AAAAA";

        private const string NonDefeatableOpenInterlock_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEAGgAAAE5v" +
           "bkRlZmVhdGFibGVPcGVuSW50ZXJsb2NrAQFvAwAvAD9vAwAAAAH/////AQH/////AAAAAA==";

        private const string DefeatableOpenInterlock_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEAFwAAAERl" +
           "ZmVhdGFibGVPcGVuSW50ZXJsb2NrAQFwAwAvAD9wAwAAAAH/////AQH/////AAAAAA==";

        private const string NonDefeatableCloseInterlock_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEAGwAAAE5v" +
           "bkRlZmVhdGFibGVDbG9zZUludGVybG9jawEBcQMALwA/cQMAAAAB/////wEB/////wAAAAA=";

        private const string DefeatableCloseInterlock_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEAGAAAAERl" +
           "ZmVhdGFibGVDbG9zZUludGVybG9jawEBcgMALwA/cgMAAAAB/////wEB/////wAAAAA=";

        private const string OpenTimeDuration_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEAEAAAAE9w" +
           "ZW5UaW1lRHVyYXRpb24BAXcDAC4ARHcDAAABACIB/////wEB/////wAAAAA=";

        private const string CloseTimeDuration_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEAEQAAAENs" +
           "b3NlVGltZUR1cmF0aW9uAQF4AwAuAER4AwAAAQAiAf////8BAf////8AAAAA";

        private const string InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////+EYIACAQAAAAEAGwAAAE1E" +
           "SVNWYWx2ZU9iamVjdFR5cGVJbnN0YW5jZQEBGgMBARoDGgMAAAH/////DAAAABVgiQoCAAAAAQAFAAAA" +
           "RmF1bHQBARsDAC8APxsDAAAAAf////8BAf////8AAAAAFWCJCgIAAAABAAgAAABQb3NpdGlvbgEBawMA" +
           "LwA/awMAAAEBvwL/////AQH/////AAAAABVgiQoCAAAAAQAPAAAAQ29tbWFuZFJlamVjdGVkAQFsAwAv" +
           "AD9sAwAAAAH/////AQH/////AAAAABVgiQoCAAAAAQAWAAAAU2lnbmF0dXJlUmVxdWVzdFN0YXR1cwEB" +
           "bQMALwA/bQMAAAEBuwL/////AQH/////AAAAABVgiQoCAAAAAQALAAAATGFzdENvbW1hbmQBAW4DAC8A" +
           "P24DAAABAQMA/////wEB/////wAAAAAVYIkKAgAAAAEAGgAAAE5vbkRlZmVhdGFibGVPcGVuSW50ZXJs" +
           "b2NrAQFvAwAvAD9vAwAAAAH/////AQH/////AAAAABVgiQoCAAAAAQAXAAAARGVmZWF0YWJsZU9wZW5J" +
           "bnRlcmxvY2sBAXADAC8AP3ADAAAAAf////8BAf////8AAAAAFWCJCgIAAAABABsAAABOb25EZWZlYXRh" +
           "YmxlQ2xvc2VJbnRlcmxvY2sBAXEDAC8AP3EDAAAAAf////8BAf////8AAAAAFWCJCgIAAAABABgAAABE" +
           "ZWZlYXRhYmxlQ2xvc2VJbnRlcmxvY2sBAXIDAC8AP3IDAAAAAf////8BAf////8AAAAABGGCCgQAAAAB" +
           "AAQAAABNb3ZlAQFzAwAvAQFzA3MDAAABAf////8BAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50" +
           "cwEBdAMALgBEdAMAAJYFAAAAAQAqAQF6AAAACQAAAERpcmVjdGlvbgEBAwD/////AAAAAAMAAAAAWAAA" +
           "AFRoZSBlbnVtZXJhdGlvbiBpbmRpY2F0ZXMgd2hldGhlciB0aGUgY29tbWFuZCBpcyB0byBvcGVuIHRo" +
           "ZSB2YWx2ZSBvciB0byBjbG9zZSB0aGUgdmFsdmUBACoBAYIAAAARAAAAT3ZlcnJpZGVJbnRlcmxvY2sA" +
           "Af////8AAAAAAwAAAABaAAAAQm9vbGVhbiBpbmRpY2F0aW5nIGlmIHRoZSBvcGVuIG9yIGNsb3NlIGNv" +
           "bW1hbmQgc2hvdWxkIG92ZXJyaWRlIGFueSBkZWZlYXQgYWJsZSBpbnRlcmxvY2tzAQAqAQFPAAAAAwAA" +
           "AFNFTQEBBQD/////AAAAAAMAAAAAMwAAAFRoZSBzZWxlY3Rpb24gb2Ygd2hpY2ggU0VNIHRvIHNlbmQg" +
           "dGhlIGNvbW1hbmQgdG8uIAEAKgEBcQAAAAkAAABTaWduYXR1cmUAAf////8AAAAAAwAAAABRAAAAQm9v" +
           "bGVhbiBpbmRpY2F0aW5nIGlmIGEgcHJvZmlsZSBzaG91bGQgYmUgZ2VuZXJhdGVkIGJ5IHRoaXMgbW92" +
           "ZSBjb21tYW5kIHJlcXVlc3QuAQAqAQFlAAAADwAAAFNodXRkb3duUmVxdWVzdAAB/////wAAAAADAAAA" +
           "AD8AAABCb29sZWFuIGluZGljYXRlcyB0aGF0IHRoaXMgY29tbWFuZCBpcyBhIHNodXRkb3duIG1vdmUg" +
           "Y29tbWFuZC4BACgBAQAAAAEAAAAAAAAAAQH/////AAAAABVgiQoCAAAAAQAQAAAAT3BlblRpbWVEdXJh" +
           "dGlvbgEBdwMALgBEdwMAAAEAIgH/////AQH/////AAAAABVgiQoCAAAAAQARAAAAQ2xvc2VUaW1lRHVy" +
           "YXRpb24BAXgDAC4ARHgDAAABACIB/////wEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public BaseDataVariableState<ValvePositionEnum> Position
        {
            get
            {
                return m_position;
            }

            set
            {
                if (!Object.ReferenceEquals(m_position, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_position = value;
            }
        }

        /// <remarks />
        public BaseDataVariableState<bool> CommandRejected
        {
            get
            {
                return m_commandRejected;
            }

            set
            {
                if (!Object.ReferenceEquals(m_commandRejected, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_commandRejected = value;
            }
        }

        /// <remarks />
        public BaseDataVariableState<SignatureStatusEnum> SignatureRequestStatus
        {
            get
            {
                return m_signatureRequestStatus;
            }

            set
            {
                if (!Object.ReferenceEquals(m_signatureRequestStatus, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_signatureRequestStatus = value;
            }
        }

        /// <remarks />
        public BaseDataVariableState<CommandEnum> LastCommand
        {
            get
            {
                return m_lastCommand;
            }

            set
            {
                if (!Object.ReferenceEquals(m_lastCommand, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_lastCommand = value;
            }
        }

        /// <remarks />
        public BaseDataVariableState<bool> NonDefeatableOpenInterlock
        {
            get
            {
                return m_nonDefeatableOpenInterlock;
            }

            set
            {
                if (!Object.ReferenceEquals(m_nonDefeatableOpenInterlock, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_nonDefeatableOpenInterlock = value;
            }
        }

        /// <remarks />
        public BaseDataVariableState<bool> DefeatableOpenInterlock
        {
            get
            {
                return m_defeatableOpenInterlock;
            }

            set
            {
                if (!Object.ReferenceEquals(m_defeatableOpenInterlock, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_defeatableOpenInterlock = value;
            }
        }

        /// <remarks />
        public BaseDataVariableState<bool> NonDefeatableCloseInterlock
        {
            get
            {
                return m_nonDefeatableCloseInterlock;
            }

            set
            {
                if (!Object.ReferenceEquals(m_nonDefeatableCloseInterlock, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_nonDefeatableCloseInterlock = value;
            }
        }

        /// <remarks />
        public BaseDataVariableState<bool> DefeatableCloseInterlock
        {
            get
            {
                return m_defeatableCloseInterlock;
            }

            set
            {
                if (!Object.ReferenceEquals(m_defeatableCloseInterlock, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_defeatableCloseInterlock = value;
            }
        }

        /// <remarks />
        public MoveMethodState Move
        {
            get
            {
                return m_moveMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_moveMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_moveMethod = value;
            }
        }

        /// <remarks />
        public PropertyState<double> OpenTimeDuration
        {
            get
            {
                return m_openTimeDuration;
            }

            set
            {
                if (!Object.ReferenceEquals(m_openTimeDuration, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_openTimeDuration = value;
            }
        }

        /// <remarks />
        public PropertyState<double> CloseTimeDuration
        {
            get
            {
                return m_closeTimeDuration;
            }

            set
            {
                if (!Object.ReferenceEquals(m_closeTimeDuration, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_closeTimeDuration = value;
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
            if (m_position != null)
            {
                children.Add(m_position);
            }

            if (m_commandRejected != null)
            {
                children.Add(m_commandRejected);
            }

            if (m_signatureRequestStatus != null)
            {
                children.Add(m_signatureRequestStatus);
            }

            if (m_lastCommand != null)
            {
                children.Add(m_lastCommand);
            }

            if (m_nonDefeatableOpenInterlock != null)
            {
                children.Add(m_nonDefeatableOpenInterlock);
            }

            if (m_defeatableOpenInterlock != null)
            {
                children.Add(m_defeatableOpenInterlock);
            }

            if (m_nonDefeatableCloseInterlock != null)
            {
                children.Add(m_nonDefeatableCloseInterlock);
            }

            if (m_defeatableCloseInterlock != null)
            {
                children.Add(m_defeatableCloseInterlock);
            }

            if (m_moveMethod != null)
            {
                children.Add(m_moveMethod);
            }

            if (m_openTimeDuration != null)
            {
                children.Add(m_openTimeDuration);
            }

            if (m_closeTimeDuration != null)
            {
                children.Add(m_closeTimeDuration);
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
                case Opc.MDIS.BrowseNames.Position:
                {
                    if (createOrReplace)
                    {
                        if (Position == null)
                        {
                            if (replacement == null)
                            {
                                Position = new BaseDataVariableState<ValvePositionEnum>(this);
                            }
                            else
                            {
                                Position = (BaseDataVariableState<ValvePositionEnum>)replacement;
                            }
                        }
                    }

                    instance = Position;
                    break;
                }

                case Opc.MDIS.BrowseNames.CommandRejected:
                {
                    if (createOrReplace)
                    {
                        if (CommandRejected == null)
                        {
                            if (replacement == null)
                            {
                                CommandRejected = new BaseDataVariableState<bool>(this);
                            }
                            else
                            {
                                CommandRejected = (BaseDataVariableState<bool>)replacement;
                            }
                        }
                    }

                    instance = CommandRejected;
                    break;
                }

                case Opc.MDIS.BrowseNames.SignatureRequestStatus:
                {
                    if (createOrReplace)
                    {
                        if (SignatureRequestStatus == null)
                        {
                            if (replacement == null)
                            {
                                SignatureRequestStatus = new BaseDataVariableState<SignatureStatusEnum>(this);
                            }
                            else
                            {
                                SignatureRequestStatus = (BaseDataVariableState<SignatureStatusEnum>)replacement;
                            }
                        }
                    }

                    instance = SignatureRequestStatus;
                    break;
                }

                case Opc.MDIS.BrowseNames.LastCommand:
                {
                    if (createOrReplace)
                    {
                        if (LastCommand == null)
                        {
                            if (replacement == null)
                            {
                                LastCommand = new BaseDataVariableState<CommandEnum>(this);
                            }
                            else
                            {
                                LastCommand = (BaseDataVariableState<CommandEnum>)replacement;
                            }
                        }
                    }

                    instance = LastCommand;
                    break;
                }

                case Opc.MDIS.BrowseNames.NonDefeatableOpenInterlock:
                {
                    if (createOrReplace)
                    {
                        if (NonDefeatableOpenInterlock == null)
                        {
                            if (replacement == null)
                            {
                                NonDefeatableOpenInterlock = new BaseDataVariableState<bool>(this);
                            }
                            else
                            {
                                NonDefeatableOpenInterlock = (BaseDataVariableState<bool>)replacement;
                            }
                        }
                    }

                    instance = NonDefeatableOpenInterlock;
                    break;
                }

                case Opc.MDIS.BrowseNames.DefeatableOpenInterlock:
                {
                    if (createOrReplace)
                    {
                        if (DefeatableOpenInterlock == null)
                        {
                            if (replacement == null)
                            {
                                DefeatableOpenInterlock = new BaseDataVariableState<bool>(this);
                            }
                            else
                            {
                                DefeatableOpenInterlock = (BaseDataVariableState<bool>)replacement;
                            }
                        }
                    }

                    instance = DefeatableOpenInterlock;
                    break;
                }

                case Opc.MDIS.BrowseNames.NonDefeatableCloseInterlock:
                {
                    if (createOrReplace)
                    {
                        if (NonDefeatableCloseInterlock == null)
                        {
                            if (replacement == null)
                            {
                                NonDefeatableCloseInterlock = new BaseDataVariableState<bool>(this);
                            }
                            else
                            {
                                NonDefeatableCloseInterlock = (BaseDataVariableState<bool>)replacement;
                            }
                        }
                    }

                    instance = NonDefeatableCloseInterlock;
                    break;
                }

                case Opc.MDIS.BrowseNames.DefeatableCloseInterlock:
                {
                    if (createOrReplace)
                    {
                        if (DefeatableCloseInterlock == null)
                        {
                            if (replacement == null)
                            {
                                DefeatableCloseInterlock = new BaseDataVariableState<bool>(this);
                            }
                            else
                            {
                                DefeatableCloseInterlock = (BaseDataVariableState<bool>)replacement;
                            }
                        }
                    }

                    instance = DefeatableCloseInterlock;
                    break;
                }

                case Opc.MDIS.BrowseNames.Move:
                {
                    if (createOrReplace)
                    {
                        if (Move == null)
                        {
                            if (replacement == null)
                            {
                                Move = new MoveMethodState(this);
                            }
                            else
                            {
                                Move = (MoveMethodState)replacement;
                            }
                        }
                    }

                    instance = Move;
                    break;
                }

                case Opc.MDIS.BrowseNames.OpenTimeDuration:
                {
                    if (createOrReplace)
                    {
                        if (OpenTimeDuration == null)
                        {
                            if (replacement == null)
                            {
                                OpenTimeDuration = new PropertyState<double>(this);
                            }
                            else
                            {
                                OpenTimeDuration = (PropertyState<double>)replacement;
                            }
                        }
                    }

                    instance = OpenTimeDuration;
                    break;
                }

                case Opc.MDIS.BrowseNames.CloseTimeDuration:
                {
                    if (createOrReplace)
                    {
                        if (CloseTimeDuration == null)
                        {
                            if (replacement == null)
                            {
                                CloseTimeDuration = new PropertyState<double>(this);
                            }
                            else
                            {
                                CloseTimeDuration = (PropertyState<double>)replacement;
                            }
                        }
                    }

                    instance = CloseTimeDuration;
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
        private BaseDataVariableState<ValvePositionEnum> m_position;
        private BaseDataVariableState<bool> m_commandRejected;
        private BaseDataVariableState<SignatureStatusEnum> m_signatureRequestStatus;
        private BaseDataVariableState<CommandEnum> m_lastCommand;
        private BaseDataVariableState<bool> m_nonDefeatableOpenInterlock;
        private BaseDataVariableState<bool> m_defeatableOpenInterlock;
        private BaseDataVariableState<bool> m_nonDefeatableCloseInterlock;
        private BaseDataVariableState<bool> m_defeatableCloseInterlock;
        private MoveMethodState m_moveMethod;
        private PropertyState<double> m_openTimeDuration;
        private PropertyState<double> m_closeTimeDuration;
        #endregion
    }
    #endif
    #endregion

    #region MDISDigitalInstrumentObjectState Class
    #if (!OPCUA_EXCLUDE_MDISDigitalInstrumentObjectState)
    /// <summary>
    /// Stores an instance of the MDISDigitalInstrumentObjectType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class MDISDigitalInstrumentObjectState : MDISBaseObjectState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public MDISDigitalInstrumentObjectState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.MDIS.ObjectTypes.MDISDigitalInstrumentObjectType, Opc.MDIS.Namespaces.MDIS, namespaceUris);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////+EYIACAQAAAAEAJwAAAE1E" +
           "SVNEaWdpdGFsSW5zdHJ1bWVudE9iamVjdFR5cGVJbnN0YW5jZQEBeQMBAXkDeQMAAAH/////AgAAABVg" +
           "iQoCAAAAAQAFAAAARmF1bHQBAXoDAC8AP3oDAAAAAf////8BAf////8AAAAAFWCJCgIAAAABAAUAAABT" +
           "dGF0ZQEBygMALwA/ygMAAAAB/////wEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public BaseDataVariableState<bool> State
        {
            get
            {
                return m_state;
            }

            set
            {
                if (!Object.ReferenceEquals(m_state, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_state = value;
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
            if (m_state != null)
            {
                children.Add(m_state);
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
                case Opc.MDIS.BrowseNames.State:
                {
                    if (createOrReplace)
                    {
                        if (State == null)
                        {
                            if (replacement == null)
                            {
                                State = new BaseDataVariableState<bool>(this);
                            }
                            else
                            {
                                State = (BaseDataVariableState<bool>)replacement;
                            }
                        }
                    }

                    instance = State;
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
        private BaseDataVariableState<bool> m_state;
        #endregion
    }
    #endif
    #endregion

    #region MDISDigitalOutObjectState Class
    #if (!OPCUA_EXCLUDE_MDISDigitalOutObjectState)
    /// <summary>
    /// Stores an instance of the MDISDigitalOutObjectType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class MDISDigitalOutObjectState : MDISDigitalInstrumentObjectState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public MDISDigitalOutObjectState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.MDIS.ObjectTypes.MDISDigitalOutObjectType, Opc.MDIS.Namespaces.MDIS, namespaceUris);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////+EYIACAQAAAAEAIAAAAE1E" +
           "SVNEaWdpdGFsT3V0T2JqZWN0VHlwZUluc3RhbmNlAQHOBAEBzgTOBAAAAf////8DAAAAFWCJCgIAAAAB" +
           "AAUAAABGYXVsdAEBzwQALwA/zwQAAAAB/////wEB/////wAAAAAVYIkKAgAAAAEABQAAAFN0YXRlAQHX" +
           "BAAvAD/XBAAAAAH/////AQH/////AAAAAARhggoEAAAAAQAKAAAAV3JpdGVTdGF0ZQEB2AQALwEB2ATY" +
           "BAAAAQH/////AQAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAdkEAC4ARNkEAACWAQAAAAEA" +
           "KgEBTQAAAAUAAABTdGF0ZQAB/////wAAAAADAAAAADEAAABCb29sZWFuIHN0YXRlIHRoYXQgaXMgYmVp" +
           "bmcgd3JpdHRlbiB0byB0aGUgb2JqZWN0AQAoAQEAAAABAAAAAAAAAAEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public WriteStateMethodState WriteState
        {
            get
            {
                return m_writeStateMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_writeStateMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_writeStateMethod = value;
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
            if (m_writeStateMethod != null)
            {
                children.Add(m_writeStateMethod);
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
                case Opc.MDIS.BrowseNames.WriteState:
                {
                    if (createOrReplace)
                    {
                        if (WriteState == null)
                        {
                            if (replacement == null)
                            {
                                WriteState = new WriteStateMethodState(this);
                            }
                            else
                            {
                                WriteState = (WriteStateMethodState)replacement;
                            }
                        }
                    }

                    instance = WriteState;
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
        private WriteStateMethodState m_writeStateMethod;
        #endregion
    }
    #endif
    #endregion

    #region MDISDiscreteInstrumentObjectState Class
    #if (!OPCUA_EXCLUDE_MDISDiscreteInstrumentObjectState)
    /// <summary>
    /// Stores an instance of the MDISDiscreteInstrumentObjectType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class MDISDiscreteInstrumentObjectState : MDISBaseObjectState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public MDISDiscreteInstrumentObjectState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.MDIS.ObjectTypes.MDISDiscreteInstrumentObjectType, Opc.MDIS.Namespaces.MDIS, namespaceUris);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////+EYIACAQAAAAEAKAAAAE1E" +
           "SVNEaXNjcmV0ZUluc3RydW1lbnRPYmplY3RUeXBlSW5zdGFuY2UBAb4EAQG+BL4EAAAB/////wIAAAAV" +
           "YIkKAgAAAAEABQAAAEZhdWx0AQG/BAAvAD+/BAAAAAH/////AQH/////AAAAABVgiQoCAAAAAQAFAAAA" +
           "U3RhdGUBAccEAC8AP8cEAAAAB/////8BAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public BaseDataVariableState<uint> State
        {
            get
            {
                return m_state;
            }

            set
            {
                if (!Object.ReferenceEquals(m_state, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_state = value;
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
            if (m_state != null)
            {
                children.Add(m_state);
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
                case Opc.MDIS.BrowseNames.State:
                {
                    if (createOrReplace)
                    {
                        if (State == null)
                        {
                            if (replacement == null)
                            {
                                State = new BaseDataVariableState<uint>(this);
                            }
                            else
                            {
                                State = (BaseDataVariableState<uint>)replacement;
                            }
                        }
                    }

                    instance = State;
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
        private BaseDataVariableState<uint> m_state;
        #endregion
    }
    #endif
    #endregion

    #region MDISDiscreteOutObjectState Class
    #if (!OPCUA_EXCLUDE_MDISDiscreteOutObjectState)
    /// <summary>
    /// Stores an instance of the MDISDiscreteOutObjectType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class MDISDiscreteOutObjectState : MDISDiscreteInstrumentObjectState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public MDISDiscreteOutObjectState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.MDIS.ObjectTypes.MDISDiscreteOutObjectType, Opc.MDIS.Namespaces.MDIS, namespaceUris);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////+EYIACAQAAAAEAIQAAAE1E" +
           "SVNEaXNjcmV0ZU91dE9iamVjdFR5cGVJbnN0YW5jZQEB2gQBAdoE2gQAAAH/////AwAAABVgiQoCAAAA" +
           "AQAFAAAARmF1bHQBAdsEAC8AP9sEAAAAAf////8BAf////8AAAAAFWCJCgIAAAABAAUAAABTdGF0ZQEB" +
           "4wQALwA/4wQAAAAH/////wEB/////wAAAAAEYYIKBAAAAAEACgAAAFdyaXRlVmFsdWUBAeQEAC8BAeQE" +
           "5AQAAAEB/////wEAAAAXYKkKAgAAAAAADgAAAElucHV0QXJndW1lbnRzAQHlBAAuAETlBAAAlgEAAAAB" +
           "ACoBAUwAAAAFAAAAU3RhdGUAB/////8AAAAAAwAAAAAwAAAAVW5pdDMyIHN0YXRlIHRoYXQgaXMgYmVp" +
           "bmcgd3JpdHRlbiB0byB0aGUgb2JqZWN0AQAoAQEAAAABAAAAAAAAAAEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public WriteValueMethodState WriteValue
        {
            get
            {
                return m_writeValueMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_writeValueMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_writeValueMethod = value;
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
            if (m_writeValueMethod != null)
            {
                children.Add(m_writeValueMethod);
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
                case Opc.MDIS.BrowseNames.WriteValue:
                {
                    if (createOrReplace)
                    {
                        if (WriteValue == null)
                        {
                            if (replacement == null)
                            {
                                WriteValue = new WriteValueMethodState(this);
                            }
                            else
                            {
                                WriteValue = (WriteValueMethodState)replacement;
                            }
                        }
                    }

                    instance = WriteValue;
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
        private WriteValueMethodState m_writeValueMethod;
        #endregion
    }
    #endif
    #endregion

    #region MDISInstrumentObjectState Class
    #if (!OPCUA_EXCLUDE_MDISInstrumentObjectState)
    /// <summary>
    /// Stores an instance of the MDISInstrumentObjectType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class MDISInstrumentObjectState : MDISBaseObjectState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public MDISInstrumentObjectState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.MDIS.ObjectTypes.MDISInstrumentObjectType, Opc.MDIS.Namespaces.MDIS, namespaceUris);
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

            if (HHlimit != null)
            {
                HHlimit.Initialize(context, HHlimit_InitializationString);
            }

            if (Hlimit != null)
            {
                Hlimit.Initialize(context, Hlimit_InitializationString);
            }

            if (Llimit != null)
            {
                Llimit.Initialize(context, Llimit_InitializationString);
            }

            if (LLlimit != null)
            {
                LLlimit.Initialize(context, LLlimit_InitializationString);
            }

            if (HHSetPoint != null)
            {
                HHSetPoint.Initialize(context, HHSetPoint_InitializationString);
            }

            if (HSetPoint != null)
            {
                HSetPoint.Initialize(context, HSetPoint_InitializationString);
            }

            if (LSetPoint != null)
            {
                LSetPoint.Initialize(context, LSetPoint_InitializationString);
            }

            if (LLSetPoint != null)
            {
                LLSetPoint.Initialize(context, LLSetPoint_InitializationString);
            }
        }

        #region Initialization String
        private const string HHlimit_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEABwAAAEhI" +
           "bGltaXQBASIEAC8APyIEAAAAAf////8BAf////8AAAAA";

        private const string Hlimit_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEABgAAAEhs" +
           "aW1pdAEBIwQALwA/IwQAAAAB/////wEB/////wAAAAA=";

        private const string Llimit_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEABgAAAExs" +
           "aW1pdAEBJAQALwA/JAQAAAAB/////wEB/////wAAAAA=";

        private const string LLlimit_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEABwAAAExM" +
           "bGltaXQBASUEAC8APyUEAAAAAf////8BAf////8AAAAA";

        private const string HHSetPoint_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEACgAAAEhI" +
           "U2V0UG9pbnQBASYEAC4ARCYEAAAACv////8DA/////8AAAAA";

        private const string HSetPoint_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEACQAAAEhT" +
           "ZXRQb2ludAEBJwQALgBEJwQAAAAK/////wMD/////wAAAAA=";

        private const string LSetPoint_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEACQAAAExT" +
           "ZXRQb2ludAEBKAQALgBEKAQAAAAK/////wMD/////wAAAAA=";

        private const string LLSetPoint_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEACgAAAExM" +
           "U2V0UG9pbnQBASkEAC4ARCkEAAAACv////8DA/////8AAAAA";

        private const string InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////+EYIACAQAAAAEAIAAAAE1E" +
           "SVNJbnN0cnVtZW50T2JqZWN0VHlwZUluc3RhbmNlAQHLAwEBywPLAwAAAf////8KAAAAFWCJCgIAAAAB" +
           "AAUAAABGYXVsdAEBzAMALwA/zAMAAAAB/////wEB/////wAAAAAVYIkKAgAAAAEADwAAAFByb2Nlc3NW" +
           "YXJpYWJsZQEBHAQALwEAQAkcBAAAAAr/////AQH/////AgAAABVgiQoCAAAAAAAHAAAARVVSYW5nZQEB" +
           "IAQALgBEIAQAAAEAdAP/////AQH/////AAAAABVgiQoCAAAAAAAQAAAARW5naW5lZXJpbmdVbml0cwEB" +
           "IQQALgBEIQQAAAEAdwP/////AQH/////AAAAABVgiQoCAAAAAQAHAAAASEhsaW1pdAEBIgQALwA/IgQA" +
           "AAAB/////wEB/////wAAAAAVYIkKAgAAAAEABgAAAEhsaW1pdAEBIwQALwA/IwQAAAAB/////wEB////" +
           "/wAAAAAVYIkKAgAAAAEABgAAAExsaW1pdAEBJAQALwA/JAQAAAAB/////wEB/////wAAAAAVYIkKAgAA" +
           "AAEABwAAAExMbGltaXQBASUEAC8APyUEAAAAAf////8BAf////8AAAAAFWCJCgIAAAABAAoAAABISFNl" +
           "dFBvaW50AQEmBAAuAEQmBAAAAAr/////AwP/////AAAAABVgiQoCAAAAAQAJAAAASFNldFBvaW50AQEn" +
           "BAAuAEQnBAAAAAr/////AwP/////AAAAABVgiQoCAAAAAQAJAAAATFNldFBvaW50AQEoBAAuAEQoBAAA" +
           "AAr/////AwP/////AAAAABVgiQoCAAAAAQAKAAAATExTZXRQb2ludAEBKQQALgBEKQQAAAAK/////wMD" +
           "/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public AnalogItemState<float> ProcessVariable
        {
            get
            {
                return m_processVariable;
            }

            set
            {
                if (!Object.ReferenceEquals(m_processVariable, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_processVariable = value;
            }
        }

        /// <remarks />
        public BaseDataVariableState<bool> HHlimit
        {
            get
            {
                return m_hHlimit;
            }

            set
            {
                if (!Object.ReferenceEquals(m_hHlimit, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_hHlimit = value;
            }
        }

        /// <remarks />
        public BaseDataVariableState<bool> Hlimit
        {
            get
            {
                return m_hlimit;
            }

            set
            {
                if (!Object.ReferenceEquals(m_hlimit, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_hlimit = value;
            }
        }

        /// <remarks />
        public BaseDataVariableState<bool> Llimit
        {
            get
            {
                return m_llimit;
            }

            set
            {
                if (!Object.ReferenceEquals(m_llimit, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_llimit = value;
            }
        }

        /// <remarks />
        public BaseDataVariableState<bool> LLlimit
        {
            get
            {
                return m_lLlimit;
            }

            set
            {
                if (!Object.ReferenceEquals(m_lLlimit, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_lLlimit = value;
            }
        }

        /// <remarks />
        public PropertyState<float> HHSetPoint
        {
            get
            {
                return m_hHSetPoint;
            }

            set
            {
                if (!Object.ReferenceEquals(m_hHSetPoint, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_hHSetPoint = value;
            }
        }

        /// <remarks />
        public PropertyState<float> HSetPoint
        {
            get
            {
                return m_hSetPoint;
            }

            set
            {
                if (!Object.ReferenceEquals(m_hSetPoint, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_hSetPoint = value;
            }
        }

        /// <remarks />
        public PropertyState<float> LSetPoint
        {
            get
            {
                return m_lSetPoint;
            }

            set
            {
                if (!Object.ReferenceEquals(m_lSetPoint, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_lSetPoint = value;
            }
        }

        /// <remarks />
        public PropertyState<float> LLSetPoint
        {
            get
            {
                return m_lLSetPoint;
            }

            set
            {
                if (!Object.ReferenceEquals(m_lLSetPoint, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_lLSetPoint = value;
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
            if (m_processVariable != null)
            {
                children.Add(m_processVariable);
            }

            if (m_hHlimit != null)
            {
                children.Add(m_hHlimit);
            }

            if (m_hlimit != null)
            {
                children.Add(m_hlimit);
            }

            if (m_llimit != null)
            {
                children.Add(m_llimit);
            }

            if (m_lLlimit != null)
            {
                children.Add(m_lLlimit);
            }

            if (m_hHSetPoint != null)
            {
                children.Add(m_hHSetPoint);
            }

            if (m_hSetPoint != null)
            {
                children.Add(m_hSetPoint);
            }

            if (m_lSetPoint != null)
            {
                children.Add(m_lSetPoint);
            }

            if (m_lLSetPoint != null)
            {
                children.Add(m_lLSetPoint);
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
                case Opc.MDIS.BrowseNames.ProcessVariable:
                {
                    if (createOrReplace)
                    {
                        if (ProcessVariable == null)
                        {
                            if (replacement == null)
                            {
                                ProcessVariable = new AnalogItemState<float>(this);
                            }
                            else
                            {
                                ProcessVariable = (AnalogItemState<float>)replacement;
                            }
                        }
                    }

                    instance = ProcessVariable;
                    break;
                }

                case Opc.MDIS.BrowseNames.HHlimit:
                {
                    if (createOrReplace)
                    {
                        if (HHlimit == null)
                        {
                            if (replacement == null)
                            {
                                HHlimit = new BaseDataVariableState<bool>(this);
                            }
                            else
                            {
                                HHlimit = (BaseDataVariableState<bool>)replacement;
                            }
                        }
                    }

                    instance = HHlimit;
                    break;
                }

                case Opc.MDIS.BrowseNames.Hlimit:
                {
                    if (createOrReplace)
                    {
                        if (Hlimit == null)
                        {
                            if (replacement == null)
                            {
                                Hlimit = new BaseDataVariableState<bool>(this);
                            }
                            else
                            {
                                Hlimit = (BaseDataVariableState<bool>)replacement;
                            }
                        }
                    }

                    instance = Hlimit;
                    break;
                }

                case Opc.MDIS.BrowseNames.Llimit:
                {
                    if (createOrReplace)
                    {
                        if (Llimit == null)
                        {
                            if (replacement == null)
                            {
                                Llimit = new BaseDataVariableState<bool>(this);
                            }
                            else
                            {
                                Llimit = (BaseDataVariableState<bool>)replacement;
                            }
                        }
                    }

                    instance = Llimit;
                    break;
                }

                case Opc.MDIS.BrowseNames.LLlimit:
                {
                    if (createOrReplace)
                    {
                        if (LLlimit == null)
                        {
                            if (replacement == null)
                            {
                                LLlimit = new BaseDataVariableState<bool>(this);
                            }
                            else
                            {
                                LLlimit = (BaseDataVariableState<bool>)replacement;
                            }
                        }
                    }

                    instance = LLlimit;
                    break;
                }

                case Opc.MDIS.BrowseNames.HHSetPoint:
                {
                    if (createOrReplace)
                    {
                        if (HHSetPoint == null)
                        {
                            if (replacement == null)
                            {
                                HHSetPoint = new PropertyState<float>(this);
                            }
                            else
                            {
                                HHSetPoint = (PropertyState<float>)replacement;
                            }
                        }
                    }

                    instance = HHSetPoint;
                    break;
                }

                case Opc.MDIS.BrowseNames.HSetPoint:
                {
                    if (createOrReplace)
                    {
                        if (HSetPoint == null)
                        {
                            if (replacement == null)
                            {
                                HSetPoint = new PropertyState<float>(this);
                            }
                            else
                            {
                                HSetPoint = (PropertyState<float>)replacement;
                            }
                        }
                    }

                    instance = HSetPoint;
                    break;
                }

                case Opc.MDIS.BrowseNames.LSetPoint:
                {
                    if (createOrReplace)
                    {
                        if (LSetPoint == null)
                        {
                            if (replacement == null)
                            {
                                LSetPoint = new PropertyState<float>(this);
                            }
                            else
                            {
                                LSetPoint = (PropertyState<float>)replacement;
                            }
                        }
                    }

                    instance = LSetPoint;
                    break;
                }

                case Opc.MDIS.BrowseNames.LLSetPoint:
                {
                    if (createOrReplace)
                    {
                        if (LLSetPoint == null)
                        {
                            if (replacement == null)
                            {
                                LLSetPoint = new PropertyState<float>(this);
                            }
                            else
                            {
                                LLSetPoint = (PropertyState<float>)replacement;
                            }
                        }
                    }

                    instance = LLSetPoint;
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
        private AnalogItemState<float> m_processVariable;
        private BaseDataVariableState<bool> m_hHlimit;
        private BaseDataVariableState<bool> m_hlimit;
        private BaseDataVariableState<bool> m_llimit;
        private BaseDataVariableState<bool> m_lLlimit;
        private PropertyState<float> m_hHSetPoint;
        private PropertyState<float> m_hSetPoint;
        private PropertyState<float> m_lSetPoint;
        private PropertyState<float> m_lLSetPoint;
        #endregion
    }
    #endif
    #endregion

    #region MDISInstrumentOutObjectState Class
    #if (!OPCUA_EXCLUDE_MDISInstrumentOutObjectState)
    /// <summary>
    /// Stores an instance of the MDISInstrumentOutObjectType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class MDISInstrumentOutObjectState : MDISInstrumentObjectState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public MDISInstrumentOutObjectState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.MDIS.ObjectTypes.MDISInstrumentOutObjectType, Opc.MDIS.Namespaces.MDIS, namespaceUris);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////+EYIACAQAAAAEAIwAAAE1E" +
           "SVNJbnN0cnVtZW50T3V0T2JqZWN0VHlwZUluc3RhbmNlAQHmBAEB5gTmBAAAAf////8DAAAAFWCJCgIA" +
           "AAABAAUAAABGYXVsdAEB5wQALwA/5wQAAAAB/////wEB/////wAAAAAVYIkKAgAAAAEADwAAAFByb2Nl" +
           "c3NWYXJpYWJsZQEB7wQALwEAQAnvBAAAAAr/////AQH/////AgAAABVgiQoCAAAAAAAHAAAARVVSYW5n" +
           "ZQEB8wQALgBE8wQAAAEAdAP/////AQH/////AAAAABVgiQoCAAAAAAAQAAAARW5naW5lZXJpbmdVbml0" +
           "cwEB9AQALgBE9AQAAAEAdwP/////AQH/////AAAAAARhggoEAAAAAQAKAAAAV3JpdGVWYWx1ZQEB/QQA" +
           "LwEB/QT9BAAAAQH/////AQAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAf4EAC4ARP4EAACW" +
           "AQAAAAEAKgEBSwAAAAUAAABWYWx1ZQAK/////wAAAAADAAAAAC8AAABGbG9hdCB2YWx1ZSB0aGF0IGlz" +
           "IGJlaW5nIHdyaXR0ZW4gdG8gdGhlIG9iamVjdAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public WriteInstrumentValueMethodState WriteValue
        {
            get
            {
                return m_writeValueMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_writeValueMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_writeValueMethod = value;
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
            if (m_writeValueMethod != null)
            {
                children.Add(m_writeValueMethod);
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
                case Opc.MDIS.BrowseNames.WriteValue:
                {
                    if (createOrReplace)
                    {
                        if (WriteValue == null)
                        {
                            if (replacement == null)
                            {
                                WriteValue = new WriteInstrumentValueMethodState(this);
                            }
                            else
                            {
                                WriteValue = (WriteInstrumentValueMethodState)replacement;
                            }
                        }
                    }

                    instance = WriteValue;
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
        private WriteInstrumentValueMethodState m_writeValueMethod;
        #endregion
    }
    #endif
    #endregion

    #region MDISChokeObjectState Class
    #if (!OPCUA_EXCLUDE_MDISChokeObjectState)
    /// <summary>
    /// Stores an instance of the MDISChokeObjectType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class MDISChokeObjectState : MDISBaseObjectState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public MDISChokeObjectState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.MDIS.ObjectTypes.MDISChokeObjectType, Opc.MDIS.Namespaces.MDIS, namespaceUris);
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

            if (SetCalculatedPositionStatus != null)
            {
                SetCalculatedPositionStatus.Initialize(context, SetCalculatedPositionStatus_InitializationString);
            }

            if (PositionInSteps != null)
            {
                PositionInSteps.Initialize(context, PositionInSteps_InitializationString);
            }

            if (CommandRejected != null)
            {
                CommandRejected.Initialize(context, CommandRejected_InitializationString);
            }

            if (NonDefeatableOpenInterlock != null)
            {
                NonDefeatableOpenInterlock.Initialize(context, NonDefeatableOpenInterlock_InitializationString);
            }

            if (DefeatableOpenInterlock != null)
            {
                DefeatableOpenInterlock.Initialize(context, DefeatableOpenInterlock_InitializationString);
            }

            if (NonDefeatableCloseInterlock != null)
            {
                NonDefeatableCloseInterlock.Initialize(context, NonDefeatableCloseInterlock_InitializationString);
            }

            if (DefeatableCloseInterlock != null)
            {
                DefeatableCloseInterlock.Initialize(context, DefeatableCloseInterlock_InitializationString);
            }

            if (Step != null)
            {
                Step.Initialize(context, Step_InitializationString);
            }

            if (StepDurationOpen != null)
            {
                StepDurationOpen.Initialize(context, StepDurationOpen_InitializationString);
            }

            if (StepDurationClose != null)
            {
                StepDurationClose.Initialize(context, StepDurationClose_InitializationString);
            }

            if (TotalSteps != null)
            {
                TotalSteps.Initialize(context, TotalSteps_InitializationString);
            }
        }

        #region Initialization String
        private const string SetCalculatedPositionStatus_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEAGwAAAFNl" +
           "dENhbGN1bGF0ZWRQb3NpdGlvblN0YXR1cwEBIgUALwA/IgUAAAEBBwX/////AQH/////AAAAAA==";

        private const string PositionInSteps_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEADwAAAFBv" +
           "c2l0aW9uSW5TdGVwcwEBfAQALwA/fAQAAAAE/////wEB/////wAAAAA=";

        private const string CommandRejected_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEADwAAAENv" +
           "bW1hbmRSZWplY3RlZAEBfgQALwA/fgQAAAAB/////wEB/////wAAAAA=";

        private const string NonDefeatableOpenInterlock_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEAGgAAAE5v" +
           "bkRlZmVhdGFibGVPcGVuSW50ZXJsb2NrAQF/BAAvAD9/BAAAAAH/////AQH/////AAAAAA==";

        private const string DefeatableOpenInterlock_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEAFwAAAERl" +
           "ZmVhdGFibGVPcGVuSW50ZXJsb2NrAQGABAAvAD+ABAAAAAH/////AQH/////AAAAAA==";

        private const string NonDefeatableCloseInterlock_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEAGwAAAE5v" +
           "bkRlZmVhdGFibGVDbG9zZUludGVybG9jawEBgQQALwA/gQQAAAAB/////wEB/////wAAAAA=";

        private const string DefeatableCloseInterlock_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEAGAAAAERl" +
           "ZmVhdGFibGVDbG9zZUludGVybG9jawEBggQALwA/ggQAAAAB/////wEB/////wAAAAA=";

        private const string Step_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8EYYIKBAAAAAEABAAAAFN0" +
           "ZXABAYUEAC8BAYUEhQQAAAEB/////wEAAAAXYKkKAgAAAAAADgAAAElucHV0QXJndW1lbnRzAQGGBAAu" +
           "AESGBAAAlgQAAAABACoBAVUAAAAJAAAARGlyZWN0aW9uAQG9Av////8AAAAAAwAAAAAzAAAAdHJ1ZSBp" +
           "cyBvcGVuaW5nIGEgdmFsdmUsIGZhbHNlIGlmIGNsb3NpbmcgdGhlIHZhbHZlAQAqAQFDAAAABQAAAFN0" +
           "ZXBzAAX/////AAAAAAMAAAAAJwAAAG51bWJlciBvZiBzdGVwcyB0byBvcGVuL2Nsb3NlIHRoZSB2YWx2" +
           "ZQEAKgEBggAAABEAAABPdmVycmlkZUludGVybG9jawAB/////wAAAAADAAAAAFoAAABCb29sZWFuIGlu" +
           "ZGljYXRpbmcgaWYgdGhlIG9wZW4gb3IgY2xvc2UgY29tbWFuZCBzaG91bGQgb3ZlcnJpZGUgYW55IGRl" +
           "ZmVhdCBhYmxlIGludGVybG9ja3MBACoBAU8AAAADAAAAU0VNAQEFAP////8AAAAAAwAAAAAzAAAAVGhl" +
           "IHNlbGVjdGlvbiBvZiB3aGljaCBTRU0gdG8gc2VuZCB0aGUgY29tbWFuZCB0by4gAQAoAQEAAAABAAAA" +
           "AAAAAAEB/////wAAAAA=";

        private const string StepDurationOpen_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEAEAAAAFN0" +
           "ZXBEdXJhdGlvbk9wZW4BAYoEAC4ARIoEAAABACIB/////wEB/////wAAAAA=";

        private const string StepDurationClose_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEAEQAAAFN0" +
           "ZXBEdXJhdGlvbkNsb3NlAQGLBAAuAESLBAAAAQAiAf////8BAf////8AAAAA";

        private const string TotalSteps_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8VYIkKAgAAAAEACgAAAFRv" +
           "dGFsU3RlcHMBAYwEAC4ARIwEAAAABf////8BAf////8AAAAA";

        private const string InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////+EYIACAQAAAAEAGwAAAE1E" +
           "SVNDaG9rZU9iamVjdFR5cGVJbnN0YW5jZQEBKgQBASoEKgQAAAH/////EQAAABVgiQoCAAAAAQAFAAAA" +
           "RmF1bHQBASsEAC8APysEAAAAAf////8BAf////8AAAAAFWCJCgIAAAABABIAAABDYWxjdWxhdGVkUG9z" +
           "aXRpb24BAXsEAC8AP3sEAAAACv////8BAf////8AAAAAFWCJCgIAAAABABsAAABTZXRDYWxjdWxhdGVk" +
           "UG9zaXRpb25TdGF0dXMBASIFAC8APyIFAAABAQcF/////wEB/////wAAAAAVYIkKAgAAAAEADwAAAFBv" +
           "c2l0aW9uSW5TdGVwcwEBfAQALwA/fAQAAAAE/////wEB/////wAAAAAVYIkKAgAAAAEABgAAAE1vdmlu" +
           "ZwEBfQQALwA/fQQAAAEBWgL/////AQH/////AAAAABVgiQoCAAAAAQAPAAAAQ29tbWFuZFJlamVjdGVk" +
           "AQF+BAAvAD9+BAAAAAH/////AQH/////AAAAABVgiQoCAAAAAQAaAAAATm9uRGVmZWF0YWJsZU9wZW5J" +
           "bnRlcmxvY2sBAX8EAC8AP38EAAAAAf////8BAf////8AAAAAFWCJCgIAAAABABcAAABEZWZlYXRhYmxl" +
           "T3BlbkludGVybG9jawEBgAQALwA/gAQAAAAB/////wEB/////wAAAAAVYIkKAgAAAAEAGwAAAE5vbkRl" +
           "ZmVhdGFibGVDbG9zZUludGVybG9jawEBgQQALwA/gQQAAAAB/////wEB/////wAAAAAVYIkKAgAAAAEA" +
           "GAAAAERlZmVhdGFibGVDbG9zZUludGVybG9jawEBggQALwA/ggQAAAAB/////wEB/////wAAAAAEYYIK" +
           "BAAAAAEABAAAAE1vdmUBAYMEAC8BAYMEgwQAAAEB/////wEAAAAXYKkKAgAAAAAADgAAAElucHV0QXJn" +
           "dW1lbnRzAQGEBAAuAESEBAAAlgMAAAABACoBAVEAAAAIAAAAUG9zaXRpb24ACv////8AAAAAAwAAAAAy" +
           "AAAAQSBudW1iZXIgKGluIHByZXJjZW50KSBpbmRpY2F0aW5nIHRoZSBwZXJjZW50IG9wZW4BACoBAYIA" +
           "AAARAAAAT3ZlcnJpZGVJbnRlcmxvY2sAAf////8AAAAAAwAAAABaAAAAQm9vbGVhbiBpbmRpY2F0aW5n" +
           "IGlmIHRoZSBvcGVuIG9yIGNsb3NlIGNvbW1hbmQgc2hvdWxkIG92ZXJyaWRlIGFueSBkZWZlYXQgYWJs" +
           "ZSBpbnRlcmxvY2tzAQAqAQFPAAAAAwAAAFNFTQEBBQD/////AAAAAAMAAAAAMwAAAFRoZSBzZWxlY3Rp" +
           "b24gb2Ygd2hpY2ggU0VNIHRvIHNlbmQgdGhlIGNvbW1hbmQgdG8uIAEAKAEBAAAAAQAAAAAAAAABAf//" +
           "//8AAAAABGGCCgQAAAABAAQAAABTdGVwAQGFBAAvAQGFBIUEAAABAf////8BAAAAF2CpCgIAAAAAAA4A" +
           "AABJbnB1dEFyZ3VtZW50cwEBhgQALgBEhgQAAJYEAAAAAQAqAQFVAAAACQAAAERpcmVjdGlvbgEBvQL/" +
           "////AAAAAAMAAAAAMwAAAHRydWUgaXMgb3BlbmluZyBhIHZhbHZlLCBmYWxzZSBpZiBjbG9zaW5nIHRo" +
           "ZSB2YWx2ZQEAKgEBQwAAAAUAAABTdGVwcwAF/////wAAAAADAAAAACcAAABudW1iZXIgb2Ygc3RlcHMg" +
           "dG8gb3Blbi9jbG9zZSB0aGUgdmFsdmUBACoBAYIAAAARAAAAT3ZlcnJpZGVJbnRlcmxvY2sAAf////8A" +
           "AAAAAwAAAABaAAAAQm9vbGVhbiBpbmRpY2F0aW5nIGlmIHRoZSBvcGVuIG9yIGNsb3NlIGNvbW1hbmQg" +
           "c2hvdWxkIG92ZXJyaWRlIGFueSBkZWZlYXQgYWJsZSBpbnRlcmxvY2tzAQAqAQFPAAAAAwAAAFNFTQEB" +
           "BQD/////AAAAAAMAAAAAMwAAAFRoZSBzZWxlY3Rpb24gb2Ygd2hpY2ggU0VNIHRvIHNlbmQgdGhlIGNv" +
           "bW1hbmQgdG8uIAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAABGGCCgQAAAABAAUAAABBYm9ydAEBhwQA" +
           "LwEBhwSHBAAAAQH/////AAAAAARhggoEAAAAAQAVAAAAU2V0Q2FsY3VsYXRlZFBvc2l0aW9uAQEEBQAv" +
           "AQEEBQQFAAABAf////8BAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBBQUALgBEBQUAAJYB" +
           "AAAAAQAqAQFQAAAACAAAAFBvc2l0aW9uAAr/////AAAAAAMAAAAAMQAAAEEgbnVtYmVyIChpbiBwZXJj" +
           "ZW50KSBpbmRpY2F0aW5nIHRoZSBwZXJjZW50IG9wZW4BACgBAQAAAAEAAAAAAAAAAQH/////AAAAABVg" +
           "iQoCAAAAAQAQAAAAU3RlcER1cmF0aW9uT3BlbgEBigQALgBEigQAAAEAIgH/////AQH/////AAAAABVg" +
           "iQoCAAAAAQARAAAAU3RlcER1cmF0aW9uQ2xvc2UBAYsEAC4ARIsEAAABACIB/////wEB/////wAAAAAV" +
           "YIkKAgAAAAEACgAAAFRvdGFsU3RlcHMBAYwEAC4ARIwEAAAABf////8BAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public BaseDataVariableState<float> CalculatedPosition
        {
            get
            {
                return m_calculatedPosition;
            }

            set
            {
                if (!Object.ReferenceEquals(m_calculatedPosition, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_calculatedPosition = value;
            }
        }

        /// <remarks />
        public BaseDataVariableState<SetCalculatedPositionEnum> SetCalculatedPositionStatus
        {
            get
            {
                return m_setCalculatedPositionStatus;
            }

            set
            {
                if (!Object.ReferenceEquals(m_setCalculatedPositionStatus, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_setCalculatedPositionStatus = value;
            }
        }

        /// <remarks />
        public BaseDataVariableState<short> PositionInSteps
        {
            get
            {
                return m_positionInSteps;
            }

            set
            {
                if (!Object.ReferenceEquals(m_positionInSteps, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_positionInSteps = value;
            }
        }

        /// <remarks />
        public BaseDataVariableState<ChokeMoveEnum> Moving
        {
            get
            {
                return m_moving;
            }

            set
            {
                if (!Object.ReferenceEquals(m_moving, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_moving = value;
            }
        }

        /// <remarks />
        public BaseDataVariableState<bool> CommandRejected
        {
            get
            {
                return m_commandRejected;
            }

            set
            {
                if (!Object.ReferenceEquals(m_commandRejected, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_commandRejected = value;
            }
        }

        /// <remarks />
        public BaseDataVariableState<bool> NonDefeatableOpenInterlock
        {
            get
            {
                return m_nonDefeatableOpenInterlock;
            }

            set
            {
                if (!Object.ReferenceEquals(m_nonDefeatableOpenInterlock, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_nonDefeatableOpenInterlock = value;
            }
        }

        /// <remarks />
        public BaseDataVariableState<bool> DefeatableOpenInterlock
        {
            get
            {
                return m_defeatableOpenInterlock;
            }

            set
            {
                if (!Object.ReferenceEquals(m_defeatableOpenInterlock, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_defeatableOpenInterlock = value;
            }
        }

        /// <remarks />
        public BaseDataVariableState<bool> NonDefeatableCloseInterlock
        {
            get
            {
                return m_nonDefeatableCloseInterlock;
            }

            set
            {
                if (!Object.ReferenceEquals(m_nonDefeatableCloseInterlock, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_nonDefeatableCloseInterlock = value;
            }
        }

        /// <remarks />
        public BaseDataVariableState<bool> DefeatableCloseInterlock
        {
            get
            {
                return m_defeatableCloseInterlock;
            }

            set
            {
                if (!Object.ReferenceEquals(m_defeatableCloseInterlock, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_defeatableCloseInterlock = value;
            }
        }

        /// <remarks />
        public ChokeMoveMethodState Move
        {
            get
            {
                return m_moveMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_moveMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_moveMethod = value;
            }
        }

        /// <remarks />
        public ChokeStepMethodState Step
        {
            get
            {
                return m_stepMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_stepMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_stepMethod = value;
            }
        }

        /// <remarks />
        public MethodState Abort
        {
            get
            {
                return m_abortMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_abortMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_abortMethod = value;
            }
        }

        /// <remarks />
        public ChokeSetCalculatedPositionMethodState SetCalculatedPosition
        {
            get
            {
                return m_setCalculatedPositionMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_setCalculatedPositionMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_setCalculatedPositionMethod = value;
            }
        }

        /// <remarks />
        public PropertyState<double> StepDurationOpen
        {
            get
            {
                return m_stepDurationOpen;
            }

            set
            {
                if (!Object.ReferenceEquals(m_stepDurationOpen, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_stepDurationOpen = value;
            }
        }

        /// <remarks />
        public PropertyState<double> StepDurationClose
        {
            get
            {
                return m_stepDurationClose;
            }

            set
            {
                if (!Object.ReferenceEquals(m_stepDurationClose, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_stepDurationClose = value;
            }
        }

        /// <remarks />
        public PropertyState<ushort> TotalSteps
        {
            get
            {
                return m_totalSteps;
            }

            set
            {
                if (!Object.ReferenceEquals(m_totalSteps, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_totalSteps = value;
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
            if (m_calculatedPosition != null)
            {
                children.Add(m_calculatedPosition);
            }

            if (m_setCalculatedPositionStatus != null)
            {
                children.Add(m_setCalculatedPositionStatus);
            }

            if (m_positionInSteps != null)
            {
                children.Add(m_positionInSteps);
            }

            if (m_moving != null)
            {
                children.Add(m_moving);
            }

            if (m_commandRejected != null)
            {
                children.Add(m_commandRejected);
            }

            if (m_nonDefeatableOpenInterlock != null)
            {
                children.Add(m_nonDefeatableOpenInterlock);
            }

            if (m_defeatableOpenInterlock != null)
            {
                children.Add(m_defeatableOpenInterlock);
            }

            if (m_nonDefeatableCloseInterlock != null)
            {
                children.Add(m_nonDefeatableCloseInterlock);
            }

            if (m_defeatableCloseInterlock != null)
            {
                children.Add(m_defeatableCloseInterlock);
            }

            if (m_moveMethod != null)
            {
                children.Add(m_moveMethod);
            }

            if (m_stepMethod != null)
            {
                children.Add(m_stepMethod);
            }

            if (m_abortMethod != null)
            {
                children.Add(m_abortMethod);
            }

            if (m_setCalculatedPositionMethod != null)
            {
                children.Add(m_setCalculatedPositionMethod);
            }

            if (m_stepDurationOpen != null)
            {
                children.Add(m_stepDurationOpen);
            }

            if (m_stepDurationClose != null)
            {
                children.Add(m_stepDurationClose);
            }

            if (m_totalSteps != null)
            {
                children.Add(m_totalSteps);
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
                case Opc.MDIS.BrowseNames.CalculatedPosition:
                {
                    if (createOrReplace)
                    {
                        if (CalculatedPosition == null)
                        {
                            if (replacement == null)
                            {
                                CalculatedPosition = new BaseDataVariableState<float>(this);
                            }
                            else
                            {
                                CalculatedPosition = (BaseDataVariableState<float>)replacement;
                            }
                        }
                    }

                    instance = CalculatedPosition;
                    break;
                }

                case Opc.MDIS.BrowseNames.SetCalculatedPositionStatus:
                {
                    if (createOrReplace)
                    {
                        if (SetCalculatedPositionStatus == null)
                        {
                            if (replacement == null)
                            {
                                SetCalculatedPositionStatus = new BaseDataVariableState<SetCalculatedPositionEnum>(this);
                            }
                            else
                            {
                                SetCalculatedPositionStatus = (BaseDataVariableState<SetCalculatedPositionEnum>)replacement;
                            }
                        }
                    }

                    instance = SetCalculatedPositionStatus;
                    break;
                }

                case Opc.MDIS.BrowseNames.PositionInSteps:
                {
                    if (createOrReplace)
                    {
                        if (PositionInSteps == null)
                        {
                            if (replacement == null)
                            {
                                PositionInSteps = new BaseDataVariableState<short>(this);
                            }
                            else
                            {
                                PositionInSteps = (BaseDataVariableState<short>)replacement;
                            }
                        }
                    }

                    instance = PositionInSteps;
                    break;
                }

                case Opc.MDIS.BrowseNames.Moving:
                {
                    if (createOrReplace)
                    {
                        if (Moving == null)
                        {
                            if (replacement == null)
                            {
                                Moving = new BaseDataVariableState<ChokeMoveEnum>(this);
                            }
                            else
                            {
                                Moving = (BaseDataVariableState<ChokeMoveEnum>)replacement;
                            }
                        }
                    }

                    instance = Moving;
                    break;
                }

                case Opc.MDIS.BrowseNames.CommandRejected:
                {
                    if (createOrReplace)
                    {
                        if (CommandRejected == null)
                        {
                            if (replacement == null)
                            {
                                CommandRejected = new BaseDataVariableState<bool>(this);
                            }
                            else
                            {
                                CommandRejected = (BaseDataVariableState<bool>)replacement;
                            }
                        }
                    }

                    instance = CommandRejected;
                    break;
                }

                case Opc.MDIS.BrowseNames.NonDefeatableOpenInterlock:
                {
                    if (createOrReplace)
                    {
                        if (NonDefeatableOpenInterlock == null)
                        {
                            if (replacement == null)
                            {
                                NonDefeatableOpenInterlock = new BaseDataVariableState<bool>(this);
                            }
                            else
                            {
                                NonDefeatableOpenInterlock = (BaseDataVariableState<bool>)replacement;
                            }
                        }
                    }

                    instance = NonDefeatableOpenInterlock;
                    break;
                }

                case Opc.MDIS.BrowseNames.DefeatableOpenInterlock:
                {
                    if (createOrReplace)
                    {
                        if (DefeatableOpenInterlock == null)
                        {
                            if (replacement == null)
                            {
                                DefeatableOpenInterlock = new BaseDataVariableState<bool>(this);
                            }
                            else
                            {
                                DefeatableOpenInterlock = (BaseDataVariableState<bool>)replacement;
                            }
                        }
                    }

                    instance = DefeatableOpenInterlock;
                    break;
                }

                case Opc.MDIS.BrowseNames.NonDefeatableCloseInterlock:
                {
                    if (createOrReplace)
                    {
                        if (NonDefeatableCloseInterlock == null)
                        {
                            if (replacement == null)
                            {
                                NonDefeatableCloseInterlock = new BaseDataVariableState<bool>(this);
                            }
                            else
                            {
                                NonDefeatableCloseInterlock = (BaseDataVariableState<bool>)replacement;
                            }
                        }
                    }

                    instance = NonDefeatableCloseInterlock;
                    break;
                }

                case Opc.MDIS.BrowseNames.DefeatableCloseInterlock:
                {
                    if (createOrReplace)
                    {
                        if (DefeatableCloseInterlock == null)
                        {
                            if (replacement == null)
                            {
                                DefeatableCloseInterlock = new BaseDataVariableState<bool>(this);
                            }
                            else
                            {
                                DefeatableCloseInterlock = (BaseDataVariableState<bool>)replacement;
                            }
                        }
                    }

                    instance = DefeatableCloseInterlock;
                    break;
                }

                case Opc.MDIS.BrowseNames.Move:
                {
                    if (createOrReplace)
                    {
                        if (Move == null)
                        {
                            if (replacement == null)
                            {
                                Move = new ChokeMoveMethodState(this);
                            }
                            else
                            {
                                Move = (ChokeMoveMethodState)replacement;
                            }
                        }
                    }

                    instance = Move;
                    break;
                }

                case Opc.MDIS.BrowseNames.Step:
                {
                    if (createOrReplace)
                    {
                        if (Step == null)
                        {
                            if (replacement == null)
                            {
                                Step = new ChokeStepMethodState(this);
                            }
                            else
                            {
                                Step = (ChokeStepMethodState)replacement;
                            }
                        }
                    }

                    instance = Step;
                    break;
                }

                case Opc.MDIS.BrowseNames.Abort:
                {
                    if (createOrReplace)
                    {
                        if (Abort == null)
                        {
                            if (replacement == null)
                            {
                                Abort = new MethodState(this);
                            }
                            else
                            {
                                Abort = (MethodState)replacement;
                            }
                        }
                    }

                    instance = Abort;
                    break;
                }

                case Opc.MDIS.BrowseNames.SetCalculatedPosition:
                {
                    if (createOrReplace)
                    {
                        if (SetCalculatedPosition == null)
                        {
                            if (replacement == null)
                            {
                                SetCalculatedPosition = new ChokeSetCalculatedPositionMethodState(this);
                            }
                            else
                            {
                                SetCalculatedPosition = (ChokeSetCalculatedPositionMethodState)replacement;
                            }
                        }
                    }

                    instance = SetCalculatedPosition;
                    break;
                }

                case Opc.MDIS.BrowseNames.StepDurationOpen:
                {
                    if (createOrReplace)
                    {
                        if (StepDurationOpen == null)
                        {
                            if (replacement == null)
                            {
                                StepDurationOpen = new PropertyState<double>(this);
                            }
                            else
                            {
                                StepDurationOpen = (PropertyState<double>)replacement;
                            }
                        }
                    }

                    instance = StepDurationOpen;
                    break;
                }

                case Opc.MDIS.BrowseNames.StepDurationClose:
                {
                    if (createOrReplace)
                    {
                        if (StepDurationClose == null)
                        {
                            if (replacement == null)
                            {
                                StepDurationClose = new PropertyState<double>(this);
                            }
                            else
                            {
                                StepDurationClose = (PropertyState<double>)replacement;
                            }
                        }
                    }

                    instance = StepDurationClose;
                    break;
                }

                case Opc.MDIS.BrowseNames.TotalSteps:
                {
                    if (createOrReplace)
                    {
                        if (TotalSteps == null)
                        {
                            if (replacement == null)
                            {
                                TotalSteps = new PropertyState<ushort>(this);
                            }
                            else
                            {
                                TotalSteps = (PropertyState<ushort>)replacement;
                            }
                        }
                    }

                    instance = TotalSteps;
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
        private BaseDataVariableState<float> m_calculatedPosition;
        private BaseDataVariableState<SetCalculatedPositionEnum> m_setCalculatedPositionStatus;
        private BaseDataVariableState<short> m_positionInSteps;
        private BaseDataVariableState<ChokeMoveEnum> m_moving;
        private BaseDataVariableState<bool> m_commandRejected;
        private BaseDataVariableState<bool> m_nonDefeatableOpenInterlock;
        private BaseDataVariableState<bool> m_defeatableOpenInterlock;
        private BaseDataVariableState<bool> m_nonDefeatableCloseInterlock;
        private BaseDataVariableState<bool> m_defeatableCloseInterlock;
        private ChokeMoveMethodState m_moveMethod;
        private ChokeStepMethodState m_stepMethod;
        private MethodState m_abortMethod;
        private ChokeSetCalculatedPositionMethodState m_setCalculatedPositionMethod;
        private PropertyState<double> m_stepDurationOpen;
        private PropertyState<double> m_stepDurationClose;
        private PropertyState<ushort> m_totalSteps;
        #endregion
    }
    #endif
    #endregion

    #region MDISAggregateObjectState Class
    #if (!OPCUA_EXCLUDE_MDISAggregateObjectState)
    /// <summary>
    /// Stores an instance of the MDISAggregateObjectType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class MDISAggregateObjectState : MDISBaseObjectState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public MDISAggregateObjectState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.MDIS.ObjectTypes.MDISAggregateObjectType, Opc.MDIS.Namespaces.MDIS, namespaceUris);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////+EYIACAQAAAAEAHwAAAE1E" +
           "SVNBZ2dyZWdhdGVPYmplY3RUeXBlSW5zdGFuY2UBASMFAQEjBSMFAAAB/////wEAAAAVYIkKAgAAAAEA" +
           "BQAAAEZhdWx0AQEkBQAvAD8kBQAAAAH/////AQH/////AAAAAA==";
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

    #region SetTimeMethodState Class
    #if (!OPCUA_EXCLUDE_SetTimeMethodState)
    /// <summary>
    /// Stores an instance of the SetTimeMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class SetTimeMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public SetTimeMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new SetTimeMethodState(parent);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8EYYIKBAAAAAEAEQAAAFNl" +
           "dFRpbWVNZXRob2RUeXBlAQHCOgAvAQHCOsI6AAABAf////8BAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFy" +
           "Z3VtZW50cwEBwzoALgBEwzoAAJYBAAAAAQAqAQFnAAAACgAAAFRhcmdldFRpbWUBACYB/////wAAAAAD" +
           "AAAAAEQAAABUaGUgVVRDIFRpbWUgdGhhdCB0aGUgU2VydmVyIHNoYWxsIHVzZSB0byB1cGRhdGUgaXRz" +
           "IGludGVybmFsIGNsb2NrLgEAKAEBAAAAAQAAAAAAAAABAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public SetTimeMethodStateMethodCallHandler OnCall;
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

            DateTime targetTime = (DateTime)_inputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    targetTime);
            }

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
    public delegate ServiceResult SetTimeMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        DateTime targetTime);
    #endif
    #endregion

    #region MDISTimeSyncObjectState Class
    #if (!OPCUA_EXCLUDE_MDISTimeSyncObjectState)
    /// <summary>
    /// Stores an instance of the MDISTimeSyncObjectType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class MDISTimeSyncObjectState : BaseObjectState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public MDISTimeSyncObjectState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.MDIS.ObjectTypes.MDISTimeSyncObjectType, Opc.MDIS.Namespaces.MDIS, namespaceUris);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8EYIACAQAAAAEAHgAAAE1E" +
           "SVNUaW1lU3luY09iamVjdFR5cGVJbnN0YW5jZQEBvAUBAbwFvAUAAP////8BAAAABGGCCgQAAAABAAcA" +
           "AABTZXRUaW1lAQG9BQAvAQG9Bb0FAAABAf////8BAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50" +
           "cwEBvgUALgBEvgUAAJYBAAAAAQAqAQFnAAAACgAAAFRhcmdldFRpbWUBACYB/////wAAAAADAAAAAEQA" +
           "AABUaGUgVVRDIFRpbWUgdGhhdCB0aGUgU2VydmVyIHNoYWxsIHVzZSB0byB1cGRhdGUgaXRzIGludGVy" +
           "bmFsIGNsb2NrLgEAKAEBAAAAAQAAAAAAAAABAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public SetTimeMethodState SetTime
        {
            get
            {
                return m_setTimeMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_setTimeMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_setTimeMethod = value;
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
            if (m_setTimeMethod != null)
            {
                children.Add(m_setTimeMethod);
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
                case Opc.MDIS.BrowseNames.SetTime:
                {
                    if (createOrReplace)
                    {
                        if (SetTime == null)
                        {
                            if (replacement == null)
                            {
                                SetTime = new SetTimeMethodState(this);
                            }
                            else
                            {
                                SetTime = (SetTimeMethodState)replacement;
                            }
                        }
                    }

                    instance = SetTime;
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
        private SetTimeMethodState m_setTimeMethod;
        #endregion
    }
    #endif
    #endregion

    #region MDISInformationObjectState Class
    #if (!OPCUA_EXCLUDE_MDISInformationObjectState)
    /// <summary>
    /// Stores an instance of the MDISInformationObjectType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class MDISInformationObjectState : BaseObjectState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public MDISInformationObjectState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.MDIS.ObjectTypes.MDISInformationObjectType, Opc.MDIS.Namespaces.MDIS, namespaceUris);
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

            if (TimeSynchronization != null)
            {
                TimeSynchronization.Initialize(context, TimeSynchronization_InitializationString);
            }

            if (Signatures != null)
            {
                Signatures.Initialize(context, Signatures_InitializationString);
            }
        }

        #region Initialization String
        private const string TimeSynchronization_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8EYIAKAQAAAAEAEwAAAFRp" +
           "bWVTeW5jaHJvbml6YXRpb24BAcAFAC8BAbwFwAUAAP////8BAAAABGGCCgQAAAABAAcAAABTZXRUaW1l" +
           "AQHBBQAvAQG9BcEFAAABAf////8BAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBwgUALgBE" +
           "wgUAAJYBAAAAAQAqAQFnAAAACgAAAFRhcmdldFRpbWUBACYB/////wAAAAADAAAAAEQAAABUaGUgVVRD" +
           "IFRpbWUgdGhhdCB0aGUgU2VydmVyIHNoYWxsIHVzZSB0byB1cGRhdGUgaXRzIGludGVybmFsIGNsb2Nr" +
           "LgEAKAEBAAAAAQAAAAAAAAABAf////8AAAAA";

        private const string Signatures_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8EYIAKAQAAAAEACgAAAFNp" +
           "Z25hdHVyZXMBAcMFAC8APcMFAAD/////AAAAAA==";

        private const string InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvTURJU/////8EYIACAQAAAAEAIQAAAE1E" +
           "SVNJbmZvcm1hdGlvbk9iamVjdFR5cGVJbnN0YW5jZQEBvwUBAb8FvwUAAP////8DAAAABGCACgEAAAAB" +
           "ABMAAABUaW1lU3luY2hyb25pemF0aW9uAQHABQAvAQG8BcAFAAD/////AQAAAARhggoEAAAAAQAHAAAA" +
           "U2V0VGltZQEBwQUALwEBvQXBBQAAAQH/////AQAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMB" +
           "AcIFAC4ARMIFAACWAQAAAAEAKgEBZwAAAAoAAABUYXJnZXRUaW1lAQAmAf////8AAAAAAwAAAABEAAAA" +
           "VGhlIFVUQyBUaW1lIHRoYXQgdGhlIFNlcnZlciBzaGFsbCB1c2UgdG8gdXBkYXRlIGl0cyBpbnRlcm5h" +
           "bCBjbG9jay4BACgBAQAAAAEAAAAAAAAAAQH/////AAAAAARggAoBAAAAAQAKAAAAU2lnbmF0dXJlcwEB" +
           "wwUALwA9wwUAAP////8AAAAAFWCJCgIAAAABAAsAAABNRElTVmVyc2lvbgEBxAUALwEBCgXEBQAAAQEJ" +
           "Bf////8BAf////8DAAAAFWCJCgIAAAABAAwAAABNYWpvclZlcnNpb24BAcUFAC4AP8UFAAAAA/////8B" +
           "Af////8AAAAAFWCJCgIAAAABAAwAAABNaW5vclZlcnNpb24BAcYFAC4AP8YFAAAAA/////8BAf////8A" +
           "AAAAFWCJCgIAAAABAAUAAABCdWlsZAEBxwUALgA/xwUAAAAD/////wEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public MDISTimeSyncObjectState TimeSynchronization
        {
            get
            {
                return m_timeSynchronization;
            }

            set
            {
                if (!Object.ReferenceEquals(m_timeSynchronization, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_timeSynchronization = value;
            }
        }

        /// <remarks />
        public FolderState Signatures
        {
            get
            {
                return m_signatures;
            }

            set
            {
                if (!Object.ReferenceEquals(m_signatures, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_signatures = value;
            }
        }

        /// <remarks />
        public MDISVersionVariableState MDISVersion
        {
            get
            {
                return m_mDISVersion;
            }

            set
            {
                if (!Object.ReferenceEquals(m_mDISVersion, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_mDISVersion = value;
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
            if (m_timeSynchronization != null)
            {
                children.Add(m_timeSynchronization);
            }

            if (m_signatures != null)
            {
                children.Add(m_signatures);
            }

            if (m_mDISVersion != null)
            {
                children.Add(m_mDISVersion);
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
                case Opc.MDIS.BrowseNames.TimeSynchronization:
                {
                    if (createOrReplace)
                    {
                        if (TimeSynchronization == null)
                        {
                            if (replacement == null)
                            {
                                TimeSynchronization = new MDISTimeSyncObjectState(this);
                            }
                            else
                            {
                                TimeSynchronization = (MDISTimeSyncObjectState)replacement;
                            }
                        }
                    }

                    instance = TimeSynchronization;
                    break;
                }

                case Opc.MDIS.BrowseNames.Signatures:
                {
                    if (createOrReplace)
                    {
                        if (Signatures == null)
                        {
                            if (replacement == null)
                            {
                                Signatures = new FolderState(this);
                            }
                            else
                            {
                                Signatures = (FolderState)replacement;
                            }
                        }
                    }

                    instance = Signatures;
                    break;
                }

                case Opc.MDIS.BrowseNames.MDISVersion:
                {
                    if (createOrReplace)
                    {
                        if (MDISVersion == null)
                        {
                            if (replacement == null)
                            {
                                MDISVersion = new MDISVersionVariableState(this);
                            }
                            else
                            {
                                MDISVersion = (MDISVersionVariableState)replacement;
                            }
                        }
                    }

                    instance = MDISVersion;
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
        private MDISTimeSyncObjectState m_timeSynchronization;
        private FolderState m_signatures;
        private MDISVersionVariableState m_mDISVersion;
        #endregion
    }
    #endif
    #endregion
}
