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
    #region FindApplicationsMethodState Class
    #if (!OPCUA_EXCLUDE_FindApplicationsMethodState)
    /// <summary>
    /// Stores an instance of the FindApplicationsMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class FindApplicationsMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public FindApplicationsMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new FindApplicationsMethodState(parent);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYYIKBAAAAAEAGgAAAEZp" +
           "bmRBcHBsaWNhdGlvbnNNZXRob2RUeXBlAQECAAAvAQECAAIAAAABAf////8CAAAAF2CpCgIAAAAAAA4A" +
           "AABJbnB1dEFyZ3VtZW50cwEBAwAALgBEAwAAAJYBAAAAAQAqAQEdAAAADgAAAEFwcGxpY2F0aW9uVXJp" +
           "AAz/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJn" +
           "dW1lbnRzAQEEAAAuAEQEAAAAlgEAAAABACoBASEAAAAMAAAAQXBwbGljYXRpb25zAQEBAAEAAAABAAAA" +
           "AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public FindApplicationsMethodStateMethodCallHandler OnCall;
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

            string applicationUri = (string)_inputArguments[0];

            ApplicationRecordDataType[] applications = (ApplicationRecordDataType[])_outputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    applicationUri,
                    ref applications);
            }

            _outputArguments[0] = applications;

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
    public delegate ServiceResult FindApplicationsMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        string applicationUri,
        ref ApplicationRecordDataType[] applications);
    #endif
    #endregion

    #region RegisterApplicationMethodState Class
    #if (!OPCUA_EXCLUDE_RegisterApplicationMethodState)
    /// <summary>
    /// Stores an instance of the RegisterApplicationMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class RegisterApplicationMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public RegisterApplicationMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new RegisterApplicationMethodState(parent);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYYIKBAAAAAEAHQAAAFJl" +
           "Z2lzdGVyQXBwbGljYXRpb25NZXRob2RUeXBlAQEFAAAvAQEFAAUAAAABAf////8CAAAAF2CpCgIAAAAA" +
           "AA4AAABJbnB1dEFyZ3VtZW50cwEBBgAALgBEBgAAAJYBAAAAAQAqAQEcAAAACwAAAEFwcGxpY2F0aW9u" +
           "AQEBAP////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIAAAAAAA8AAABPdXRwdXRB" +
           "cmd1bWVudHMBAQcAAC4ARAcAAACWAQAAAAEAKgEBHAAAAA0AAABBcHBsaWNhdGlvbklkABH/////AAAA" +
           "AAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public RegisterApplicationMethodStateMethodCallHandler OnCall;
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

            ApplicationRecordDataType application = (ApplicationRecordDataType)ExtensionObject.ToEncodeable((ExtensionObject)_inputArguments[0]);

            NodeId applicationId = (NodeId)_outputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    application,
                    ref applicationId);
            }

            _outputArguments[0] = applicationId;

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
    public delegate ServiceResult RegisterApplicationMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        ApplicationRecordDataType application,
        ref NodeId applicationId);
    #endif
    #endregion

    #region UpdateApplicationMethodState Class
    #if (!OPCUA_EXCLUDE_UpdateApplicationMethodState)
    /// <summary>
    /// Stores an instance of the UpdateApplicationMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class UpdateApplicationMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public UpdateApplicationMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new UpdateApplicationMethodState(parent);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYYIKBAAAAAEAGwAAAFVw" +
           "ZGF0ZUFwcGxpY2F0aW9uTWV0aG9kVHlwZQEBugAALwEBugC6AAAAAQH/////AQAAABdgqQoCAAAAAAAO" +
           "AAAASW5wdXRBcmd1bWVudHMBAbsAAC4ARLsAAACWAQAAAAEAKgEBHAAAAAsAAABBcHBsaWNhdGlvbgEB" +
           "AQD/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public UpdateApplicationMethodStateMethodCallHandler OnCall;
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

            ApplicationRecordDataType application = (ApplicationRecordDataType)ExtensionObject.ToEncodeable((ExtensionObject)_inputArguments[0]);

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    application);
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
    public delegate ServiceResult UpdateApplicationMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        ApplicationRecordDataType application);
    #endif
    #endregion

    #region UnregisterApplicationMethodState Class
    #if (!OPCUA_EXCLUDE_UnregisterApplicationMethodState)
    /// <summary>
    /// Stores an instance of the UnregisterApplicationMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class UnregisterApplicationMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public UnregisterApplicationMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new UnregisterApplicationMethodState(parent);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYYIKBAAAAAEAHwAAAFVu" +
           "cmVnaXN0ZXJBcHBsaWNhdGlvbk1ldGhvZFR5cGUBAQgAAC8BAQgACAAAAAEB/////wEAAAAXYKkKAgAA" +
           "AAAADgAAAElucHV0QXJndW1lbnRzAQEJAAAuAEQJAAAAlgEAAAABACoBARwAAAANAAAAQXBwbGljYXRp" +
           "b25JZAAR/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public UnregisterApplicationMethodStateMethodCallHandler OnCall;
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

            NodeId applicationId = (NodeId)_inputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    applicationId);
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
    public delegate ServiceResult UnregisterApplicationMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        NodeId applicationId);
    #endif
    #endregion

    #region GetApplicationMethodState Class
    #if (!OPCUA_EXCLUDE_GetApplicationMethodState)
    /// <summary>
    /// Stores an instance of the GetApplicationMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class GetApplicationMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public GetApplicationMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new GetApplicationMethodState(parent);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYYIKBAAAAAEAGAAAAEdl" +
           "dEFwcGxpY2F0aW9uTWV0aG9kVHlwZQEBzwAALwEBzwDPAAAAAQH/////AgAAABdgqQoCAAAAAAAOAAAA" +
           "SW5wdXRBcmd1bWVudHMBAdAAAC4ARNAAAACWAQAAAAEAKgEBHAAAAA0AAABBcHBsaWNhdGlvbklkABH/" +
           "////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1l" +
           "bnRzAQHRAAAuAETRAAAAlgEAAAABACoBARwAAAALAAAAQXBwbGljYXRpb24BAQEA/////wAAAAAAAQAo" +
           "AQEAAAABAAAAAAAAAAEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public GetApplicationMethodStateMethodCallHandler OnCall;
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

            NodeId applicationId = (NodeId)_inputArguments[0];

            ApplicationRecordDataType application = (ApplicationRecordDataType)_outputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    applicationId,
                    ref application);
            }

            _outputArguments[0] = application;

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
    public delegate ServiceResult GetApplicationMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        NodeId applicationId,
        ref ApplicationRecordDataType application);
    #endif
    #endregion

    #region QueryApplicationsMethodState Class
    #if (!OPCUA_EXCLUDE_QueryApplicationsMethodState)
    /// <summary>
    /// Stores an instance of the QueryApplicationsMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class QueryApplicationsMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public QueryApplicationsMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new QueryApplicationsMethodState(parent);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYYIKBAAAAAEAGwAAAFF1" +
           "ZXJ5QXBwbGljYXRpb25zTWV0aG9kVHlwZQEBYQMALwEBYQNhAwAAAQH/////AgAAABdgqQoCAAAAAAAO" +
           "AAAASW5wdXRBcmd1bWVudHMBAWIDAC4ARGIDAACWBwAAAAEAKgEBHwAAABAAAABTdGFydGluZ1JlY29y" +
           "ZElkAAf/////AAAAAAABACoBASEAAAASAAAATWF4UmVjb3Jkc1RvUmV0dXJuAAf/////AAAAAAABACoB" +
           "AR4AAAAPAAAAQXBwbGljYXRpb25OYW1lAAz/////AAAAAAABACoBAR0AAAAOAAAAQXBwbGljYXRpb25V" +
           "cmkADP////8AAAAAAAEAKgEBHgAAAA8AAABBcHBsaWNhdGlvblR5cGUAB/////8AAAAAAAEAKgEBGQAA" +
           "AAoAAABQcm9kdWN0VXJpAAz/////AAAAAAABACoBAR8AAAAMAAAAQ2FwYWJpbGl0aWVzAAwBAAAAAQAA" +
           "AAAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAXYKkKAgAAAAAADwAAAE91dHB1dEFyZ3VtZW50" +
           "cwEBYwMALgBEYwMAAJYDAAAAAQAqAQElAAAAFAAAAExhc3RDb3VudGVyUmVzZXRUaW1lAQAmAf////8A" +
           "AAAAAAEAKgEBGwAAAAwAAABOZXh0UmVjb3JkSWQAB/////8AAAAAAAEAKgEBIQAAAAwAAABBcHBsaWNh" +
           "dGlvbnMBADQBAQAAAAEAAAAAAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public QueryApplicationsMethodStateMethodCallHandler OnCall;
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

            uint startingRecordId = (uint)_inputArguments[0];
            uint maxRecordsToReturn = (uint)_inputArguments[1];
            string applicationName = (string)_inputArguments[2];
            string applicationUri = (string)_inputArguments[3];
            uint applicationType = (uint)_inputArguments[4];
            string productUri = (string)_inputArguments[5];
            string[] capabilities = (string[])_inputArguments[6];

            DateTime lastCounterResetTime = (DateTime)_outputArguments[0];
            uint nextRecordId = (uint)_outputArguments[1];
            ApplicationDescription[] applications = (ApplicationDescription[])_outputArguments[2];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    startingRecordId,
                    maxRecordsToReturn,
                    applicationName,
                    applicationUri,
                    applicationType,
                    productUri,
                    capabilities,
                    ref lastCounterResetTime,
                    ref nextRecordId,
                    ref applications);
            }

            _outputArguments[0] = lastCounterResetTime;
            _outputArguments[1] = nextRecordId;
            _outputArguments[2] = applications;

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
    public delegate ServiceResult QueryApplicationsMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        uint startingRecordId,
        uint maxRecordsToReturn,
        string applicationName,
        string applicationUri,
        uint applicationType,
        string productUri,
        string[] capabilities,
        ref DateTime lastCounterResetTime,
        ref uint nextRecordId,
        ref ApplicationDescription[] applications);
    #endif
    #endregion

    #region QueryServersMethodState Class
    #if (!OPCUA_EXCLUDE_QueryServersMethodState)
    /// <summary>
    /// Stores an instance of the QueryServersMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class QueryServersMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public QueryServersMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new QueryServersMethodState(parent);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYYIKBAAAAAEAFgAAAFF1" +
           "ZXJ5U2VydmVyc01ldGhvZFR5cGUBAQoAAC8BAQoACgAAAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElu" +
           "cHV0QXJndW1lbnRzAQELAAAuAEQLAAAAlgYAAAABACoBAR8AAAAQAAAAU3RhcnRpbmdSZWNvcmRJZAAH" +
           "/////wAAAAAAAQAqAQEhAAAAEgAAAE1heFJlY29yZHNUb1JldHVybgAH/////wAAAAAAAQAqAQEeAAAA" +
           "DwAAAEFwcGxpY2F0aW9uTmFtZQAM/////wAAAAAAAQAqAQEdAAAADgAAAEFwcGxpY2F0aW9uVXJpAAz/" +
           "////AAAAAAABACoBARkAAAAKAAAAUHJvZHVjdFVyaQAM/////wAAAAAAAQAqAQElAAAAEgAAAFNlcnZl" +
           "ckNhcGFiaWxpdGllcwAMAQAAAAEAAAAAAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIA" +
           "AAAAAA8AAABPdXRwdXRBcmd1bWVudHMBAQwAAC4ARAwAAACWAgAAAAEAKgEBJQAAABQAAABMYXN0Q291" +
           "bnRlclJlc2V0VGltZQEAJgH/////AAAAAAABACoBARwAAAAHAAAAU2VydmVycwEAnS8BAAAAAQAAAAAA" +
           "AAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public QueryServersMethodStateMethodCallHandler OnCall;
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

            uint startingRecordId = (uint)_inputArguments[0];
            uint maxRecordsToReturn = (uint)_inputArguments[1];
            string applicationName = (string)_inputArguments[2];
            string applicationUri = (string)_inputArguments[3];
            string productUri = (string)_inputArguments[4];
            string[] serverCapabilities = (string[])_inputArguments[5];

            DateTime lastCounterResetTime = (DateTime)_outputArguments[0];
            ServerOnNetwork[] servers = (ServerOnNetwork[])_outputArguments[1];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    startingRecordId,
                    maxRecordsToReturn,
                    applicationName,
                    applicationUri,
                    productUri,
                    serverCapabilities,
                    ref lastCounterResetTime,
                    ref servers);
            }

            _outputArguments[0] = lastCounterResetTime;
            _outputArguments[1] = servers;

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
    public delegate ServiceResult QueryServersMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        uint startingRecordId,
        uint maxRecordsToReturn,
        string applicationName,
        string applicationUri,
        string productUri,
        string[] serverCapabilities,
        ref DateTime lastCounterResetTime,
        ref ServerOnNetwork[] servers);
    #endif
    #endregion

    #region DirectoryState Class
    #if (!OPCUA_EXCLUDE_DirectoryState)
    /// <summary>
    /// Stores an instance of the DirectoryType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class DirectoryState : FolderState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public DirectoryState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Gds.ObjectTypes.DirectoryType, Opc.Ua.Gds.Namespaces.OpcUaGds, namespaceUris);
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
           "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" +
           "AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public FolderState Applications
        {
            get
            {
                return m_applications;
            }

            set
            {
                if (!Object.ReferenceEquals(m_applications, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_applications = value;
            }
        }

        /// <remarks />
        public FindApplicationsMethodState FindApplications
        {
            get
            {
                return m_findApplicationsMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_findApplicationsMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_findApplicationsMethod = value;
            }
        }

        /// <remarks />
        public RegisterApplicationMethodState RegisterApplication
        {
            get
            {
                return m_registerApplicationMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_registerApplicationMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_registerApplicationMethod = value;
            }
        }

        /// <remarks />
        public UpdateApplicationMethodState UpdateApplication
        {
            get
            {
                return m_updateApplicationMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_updateApplicationMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_updateApplicationMethod = value;
            }
        }

        /// <remarks />
        public UnregisterApplicationMethodState UnregisterApplication
        {
            get
            {
                return m_unregisterApplicationMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_unregisterApplicationMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_unregisterApplicationMethod = value;
            }
        }

        /// <remarks />
        public GetApplicationMethodState GetApplication
        {
            get
            {
                return m_getApplicationMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_getApplicationMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_getApplicationMethod = value;
            }
        }

        /// <remarks />
        public QueryApplicationsMethodState QueryApplications
        {
            get
            {
                return m_queryApplicationsMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_queryApplicationsMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_queryApplicationsMethod = value;
            }
        }

        /// <remarks />
        public QueryServersMethodState QueryServers
        {
            get
            {
                return m_queryServersMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_queryServersMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_queryServersMethod = value;
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
            if (m_applications != null)
            {
                children.Add(m_applications);
            }

            if (m_findApplicationsMethod != null)
            {
                children.Add(m_findApplicationsMethod);
            }

            if (m_registerApplicationMethod != null)
            {
                children.Add(m_registerApplicationMethod);
            }

            if (m_updateApplicationMethod != null)
            {
                children.Add(m_updateApplicationMethod);
            }

            if (m_unregisterApplicationMethod != null)
            {
                children.Add(m_unregisterApplicationMethod);
            }

            if (m_getApplicationMethod != null)
            {
                children.Add(m_getApplicationMethod);
            }

            if (m_queryApplicationsMethod != null)
            {
                children.Add(m_queryApplicationsMethod);
            }

            if (m_queryServersMethod != null)
            {
                children.Add(m_queryServersMethod);
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
                case Opc.Ua.Gds.BrowseNames.Applications:
                {
                    if (createOrReplace)
                    {
                        if (Applications == null)
                        {
                            if (replacement == null)
                            {
                                Applications = new FolderState(this);
                            }
                            else
                            {
                                Applications = (FolderState)replacement;
                            }
                        }
                    }

                    instance = Applications;
                    break;
                }

                case Opc.Ua.Gds.BrowseNames.FindApplications:
                {
                    if (createOrReplace)
                    {
                        if (FindApplications == null)
                        {
                            if (replacement == null)
                            {
                                FindApplications = new FindApplicationsMethodState(this);
                            }
                            else
                            {
                                FindApplications = (FindApplicationsMethodState)replacement;
                            }
                        }
                    }

                    instance = FindApplications;
                    break;
                }

                case Opc.Ua.Gds.BrowseNames.RegisterApplication:
                {
                    if (createOrReplace)
                    {
                        if (RegisterApplication == null)
                        {
                            if (replacement == null)
                            {
                                RegisterApplication = new RegisterApplicationMethodState(this);
                            }
                            else
                            {
                                RegisterApplication = (RegisterApplicationMethodState)replacement;
                            }
                        }
                    }

                    instance = RegisterApplication;
                    break;
                }

                case Opc.Ua.Gds.BrowseNames.UpdateApplication:
                {
                    if (createOrReplace)
                    {
                        if (UpdateApplication == null)
                        {
                            if (replacement == null)
                            {
                                UpdateApplication = new UpdateApplicationMethodState(this);
                            }
                            else
                            {
                                UpdateApplication = (UpdateApplicationMethodState)replacement;
                            }
                        }
                    }

                    instance = UpdateApplication;
                    break;
                }

                case Opc.Ua.Gds.BrowseNames.UnregisterApplication:
                {
                    if (createOrReplace)
                    {
                        if (UnregisterApplication == null)
                        {
                            if (replacement == null)
                            {
                                UnregisterApplication = new UnregisterApplicationMethodState(this);
                            }
                            else
                            {
                                UnregisterApplication = (UnregisterApplicationMethodState)replacement;
                            }
                        }
                    }

                    instance = UnregisterApplication;
                    break;
                }

                case Opc.Ua.Gds.BrowseNames.GetApplication:
                {
                    if (createOrReplace)
                    {
                        if (GetApplication == null)
                        {
                            if (replacement == null)
                            {
                                GetApplication = new GetApplicationMethodState(this);
                            }
                            else
                            {
                                GetApplication = (GetApplicationMethodState)replacement;
                            }
                        }
                    }

                    instance = GetApplication;
                    break;
                }

                case Opc.Ua.Gds.BrowseNames.QueryApplications:
                {
                    if (createOrReplace)
                    {
                        if (QueryApplications == null)
                        {
                            if (replacement == null)
                            {
                                QueryApplications = new QueryApplicationsMethodState(this);
                            }
                            else
                            {
                                QueryApplications = (QueryApplicationsMethodState)replacement;
                            }
                        }
                    }

                    instance = QueryApplications;
                    break;
                }

                case Opc.Ua.Gds.BrowseNames.QueryServers:
                {
                    if (createOrReplace)
                    {
                        if (QueryServers == null)
                        {
                            if (replacement == null)
                            {
                                QueryServers = new QueryServersMethodState(this);
                            }
                            else
                            {
                                QueryServers = (QueryServersMethodState)replacement;
                            }
                        }
                    }

                    instance = QueryServers;
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
        private FolderState m_applications;
        private FindApplicationsMethodState m_findApplicationsMethod;
        private RegisterApplicationMethodState m_registerApplicationMethod;
        private UpdateApplicationMethodState m_updateApplicationMethod;
        private UnregisterApplicationMethodState m_unregisterApplicationMethod;
        private GetApplicationMethodState m_getApplicationMethod;
        private QueryApplicationsMethodState m_queryApplicationsMethod;
        private QueryServersMethodState m_queryServersMethod;
        #endregion
    }
    #endif
    #endregion

    #region ApplicationRegistrationChangedAuditEventState Class
    #if (!OPCUA_EXCLUDE_ApplicationRegistrationChangedAuditEventState)
    /// <summary>
    /// Stores an instance of the ApplicationRegistrationChangedAuditEventType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ApplicationRegistrationChangedAuditEventState : AuditUpdateMethodEventState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ApplicationRegistrationChangedAuditEventState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Gds.ObjectTypes.ApplicationRegistrationChangedAuditEventType, Opc.Ua.Gds.Namespaces.OpcUaGds, namespaceUris);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYIACAQAAAAEANAAAAEFw" +
           "cGxpY2F0aW9uUmVnaXN0cmF0aW9uQ2hhbmdlZEF1ZGl0RXZlbnRUeXBlSW5zdGFuY2UBARoAAQEaABoA" +
           "AAD/////DwAAABVgiQoCAAAAAAAHAAAARXZlbnRJZAEBGwAALgBEGwAAAAAP/////wEB/////wAAAAAV" +
           "YIkKAgAAAAAACQAAAEV2ZW50VHlwZQEBHAAALgBEHAAAAAAR/////wEB/////wAAAAAVYIkKAgAAAAAA" +
           "CgAAAFNvdXJjZU5vZGUBAR0AAC4ARB0AAAAAEf////8BAf////8AAAAAFWCJCgIAAAAAAAoAAABTb3Vy" +
           "Y2VOYW1lAQEeAAAuAEQeAAAAAAz/////AQH/////AAAAABVgiQoCAAAAAAAEAAAAVGltZQEBHwAALgBE" +
           "HwAAAAEAJgH/////AQH/////AAAAABVgiQoCAAAAAAALAAAAUmVjZWl2ZVRpbWUBASAAAC4ARCAAAAAB" +
           "ACYB/////wEB/////wAAAAAVYIkKAgAAAAAABwAAAE1lc3NhZ2UBASIAAC4ARCIAAAAAFf////8BAf//" +
           "//8AAAAAFWCJCgIAAAAAAAgAAABTZXZlcml0eQEBIwAALgBEIwAAAAAF/////wEB/////wAAAAAVYIkK" +
           "AgAAAAAADwAAAEFjdGlvblRpbWVTdGFtcAEBJAAALgBEJAAAAAEAJgH/////AQH/////AAAAABVgiQoC" +
           "AAAAAAAGAAAAU3RhdHVzAQElAAAuAEQlAAAAAAH/////AQH/////AAAAABVgiQoCAAAAAAAIAAAAU2Vy" +
           "dmVySWQBASYAAC4ARCYAAAAADP////8BAf////8AAAAAFWCJCgIAAAAAABIAAABDbGllbnRBdWRpdEVu" +
           "dHJ5SWQBAScAAC4ARCcAAAAADP////8BAf////8AAAAAFWCJCgIAAAAAAAwAAABDbGllbnRVc2VySWQB" +
           "ASgAAC4ARCgAAAAADP////8BAf////8AAAAAFWCJCgIAAAAAAAgAAABNZXRob2RJZAEBKQAALgBEKQAA" +
           "AAAR/////wEB/////wAAAAAXYIkKAgAAAAAADgAAAElucHV0QXJndW1lbnRzAQEqAAAuAEQqAAAAABgB" +
           "AAAAAQAAAAAAAAABAf////8AAAAA";
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

    #region StartSigningRequestMethodState Class
    #if (!OPCUA_EXCLUDE_StartSigningRequestMethodState)
    /// <summary>
    /// Stores an instance of the StartSigningRequestMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class StartSigningRequestMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public StartSigningRequestMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new StartSigningRequestMethodState(parent);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYYIKBAAAAAEAHQAAAFN0" +
           "YXJ0U2lnbmluZ1JlcXVlc3RNZXRob2RUeXBlAQEzAAAvAQEzADMAAAABAf////8CAAAAF2CpCgIAAAAA" +
           "AA4AAABJbnB1dEFyZ3VtZW50cwEBNAAALgBENAAAAJYEAAAAAQAqAQEcAAAADQAAAEFwcGxpY2F0aW9u" +
           "SWQAEf////8AAAAAAAEAKgEBIQAAABIAAABDZXJ0aWZpY2F0ZUdyb3VwSWQAEf////8AAAAAAAEAKgEB" +
           "IAAAABEAAABDZXJ0aWZpY2F0ZVR5cGVJZAAR/////wAAAAAAAQAqAQEhAAAAEgAAAENlcnRpZmljYXRl" +
           "UmVxdWVzdAAP/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAXYKkKAgAAAAAADwAAAE91" +
           "dHB1dEFyZ3VtZW50cwEBNQAALgBENQAAAJYBAAAAAQAqAQEYAAAACQAAAFJlcXVlc3RJZAAR/////wAA" +
           "AAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public StartSigningRequestMethodStateMethodCallHandler OnCall;
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

            NodeId applicationId = (NodeId)_inputArguments[0];
            NodeId certificateGroupId = (NodeId)_inputArguments[1];
            NodeId certificateTypeId = (NodeId)_inputArguments[2];
            byte[] certificateRequest = (byte[])_inputArguments[3];

            NodeId requestId = (NodeId)_outputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    applicationId,
                    certificateGroupId,
                    certificateTypeId,
                    certificateRequest,
                    ref requestId);
            }

            _outputArguments[0] = requestId;

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
    public delegate ServiceResult StartSigningRequestMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        NodeId applicationId,
        NodeId certificateGroupId,
        NodeId certificateTypeId,
        byte[] certificateRequest,
        ref NodeId requestId);
    #endif
    #endregion

    #region StartNewKeyPairRequestMethodState Class
    #if (!OPCUA_EXCLUDE_StartNewKeyPairRequestMethodState)
    /// <summary>
    /// Stores an instance of the StartNewKeyPairRequestMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class StartNewKeyPairRequestMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public StartNewKeyPairRequestMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new StartNewKeyPairRequestMethodState(parent);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYYIKBAAAAAEAIAAAAFN0" +
           "YXJ0TmV3S2V5UGFpclJlcXVlc3RNZXRob2RUeXBlAQEwAAAvAQEwADAAAAABAf////8CAAAAF2CpCgIA" +
           "AAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBMQAALgBEMQAAAJYHAAAAAQAqAQEcAAAADQAAAEFwcGxpY2F0" +
           "aW9uSWQAEf////8AAAAAAAEAKgEBIQAAABIAAABDZXJ0aWZpY2F0ZUdyb3VwSWQAEf////8AAAAAAAEA" +
           "KgEBIAAAABEAAABDZXJ0aWZpY2F0ZVR5cGVJZAAR/////wAAAAAAAQAqAQEaAAAACwAAAFN1YmplY3RO" +
           "YW1lAAz/////AAAAAAABACoBAR4AAAALAAAARG9tYWluTmFtZXMADAEAAAABAAAAAAAAAAABACoBAR8A" +
           "AAAQAAAAUHJpdmF0ZUtleUZvcm1hdAAM/////wAAAAAAAQAqAQEhAAAAEgAAAFByaXZhdGVLZXlQYXNz" +
           "d29yZAAM/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAXYKkKAgAAAAAADwAAAE91dHB1" +
           "dEFyZ3VtZW50cwEBMgAALgBEMgAAAJYBAAAAAQAqAQEYAAAACQAAAFJlcXVlc3RJZAAR/////wAAAAAA" +
           "AQAoAQEAAAABAAAAAAAAAAEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public StartNewKeyPairRequestMethodStateMethodCallHandler OnCall;
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

            NodeId applicationId = (NodeId)_inputArguments[0];
            NodeId certificateGroupId = (NodeId)_inputArguments[1];
            NodeId certificateTypeId = (NodeId)_inputArguments[2];
            string subjectName = (string)_inputArguments[3];
            string[] domainNames = (string[])_inputArguments[4];
            string privateKeyFormat = (string)_inputArguments[5];
            string privateKeyPassword = (string)_inputArguments[6];

            NodeId requestId = (NodeId)_outputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    applicationId,
                    certificateGroupId,
                    certificateTypeId,
                    subjectName,
                    domainNames,
                    privateKeyFormat,
                    privateKeyPassword,
                    ref requestId);
            }

            _outputArguments[0] = requestId;

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
    public delegate ServiceResult StartNewKeyPairRequestMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        NodeId applicationId,
        NodeId certificateGroupId,
        NodeId certificateTypeId,
        string subjectName,
        string[] domainNames,
        string privateKeyFormat,
        string privateKeyPassword,
        ref NodeId requestId);
    #endif
    #endregion

    #region FinishRequestMethodState Class
    #if (!OPCUA_EXCLUDE_FinishRequestMethodState)
    /// <summary>
    /// Stores an instance of the FinishRequestMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class FinishRequestMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public FinishRequestMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new FinishRequestMethodState(parent);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYYIKBAAAAAEAFwAAAEZp" +
           "bmlzaFJlcXVlc3RNZXRob2RUeXBlAQE5AAAvAQE5ADkAAAABAf////8CAAAAF2CpCgIAAAAAAA4AAABJ" +
           "bnB1dEFyZ3VtZW50cwEBOgAALgBEOgAAAJYCAAAAAQAqAQEcAAAADQAAAEFwcGxpY2F0aW9uSWQAEf//" +
           "//8AAAAAAAEAKgEBGAAAAAkAAABSZXF1ZXN0SWQAEf////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf//" +
           "//8AAAAAF2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1bWVudHMBATsAAC4ARDsAAACWAwAAAAEAKgEBGgAA" +
           "AAsAAABDZXJ0aWZpY2F0ZQAP/////wAAAAAAAQAqAQEZAAAACgAAAFByaXZhdGVLZXkAD/////8AAAAA" +
           "AAEAKgEBJQAAABIAAABJc3N1ZXJDZXJ0aWZpY2F0ZXMADwEAAAABAAAAAAAAAAABACgBAQAAAAEAAAAA" +
           "AAAAAQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public FinishRequestMethodStateMethodCallHandler OnCall;
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

            NodeId applicationId = (NodeId)_inputArguments[0];
            NodeId requestId = (NodeId)_inputArguments[1];

            byte[] certificate = (byte[])_outputArguments[0];
            byte[] privateKey = (byte[])_outputArguments[1];
            byte[][] issuerCertificates = (byte[][])_outputArguments[2];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    applicationId,
                    requestId,
                    ref certificate,
                    ref privateKey,
                    ref issuerCertificates);
            }

            _outputArguments[0] = certificate;
            _outputArguments[1] = privateKey;
            _outputArguments[2] = issuerCertificates;

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
    public delegate ServiceResult FinishRequestMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        NodeId applicationId,
        NodeId requestId,
        ref byte[] certificate,
        ref byte[] privateKey,
        ref byte[][] issuerCertificates);
    #endif
    #endregion

    #region GetCertificateGroupsMethodState Class
    #if (!OPCUA_EXCLUDE_GetCertificateGroupsMethodState)
    /// <summary>
    /// Stores an instance of the GetCertificateGroupsMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class GetCertificateGroupsMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public GetCertificateGroupsMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new GetCertificateGroupsMethodState(parent);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYYIKBAAAAAEAHgAAAEdl" +
           "dENlcnRpZmljYXRlR3JvdXBzTWV0aG9kVHlwZQEB5gAALwEB5gDmAAAAAQH/////AgAAABdgqQoCAAAA" +
           "AAAOAAAASW5wdXRBcmd1bWVudHMBAecAAC4AROcAAACWAQAAAAEAKgEBHAAAAA0AAABBcHBsaWNhdGlv" +
           "bklkABH/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0" +
           "QXJndW1lbnRzAQHoAAAuAEToAAAAlgEAAAABACoBASYAAAATAAAAQ2VydGlmaWNhdGVHcm91cElkcwAR" +
           "AQAAAAEAAAAAAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public GetCertificateGroupsMethodStateMethodCallHandler OnCall;
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

            NodeId applicationId = (NodeId)_inputArguments[0];

            NodeId[] certificateGroupIds = (NodeId[])_outputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    applicationId,
                    ref certificateGroupIds);
            }

            _outputArguments[0] = certificateGroupIds;

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
    public delegate ServiceResult GetCertificateGroupsMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        NodeId applicationId,
        ref NodeId[] certificateGroupIds);
    #endif
    #endregion

    #region GetTrustListMethodState Class
    #if (!OPCUA_EXCLUDE_GetTrustListMethodState)
    /// <summary>
    /// Stores an instance of the GetTrustListMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class GetTrustListMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public GetTrustListMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new GetTrustListMethodState(parent);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYYIKBAAAAAEAFgAAAEdl" +
           "dFRydXN0TGlzdE1ldGhvZFR5cGUBAb4AAC8BAb4AvgAAAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElu" +
           "cHV0QXJndW1lbnRzAQG/AAAuAES/AAAAlgIAAAABACoBARwAAAANAAAAQXBwbGljYXRpb25JZAAR////" +
           "/wAAAAAAAQAqAQEhAAAAEgAAAENlcnRpZmljYXRlR3JvdXBJZAAR/////wAAAAAAAQAoAQEAAAABAAAA" +
           "AAAAAAEB/////wAAAAAXYKkKAgAAAAAADwAAAE91dHB1dEFyZ3VtZW50cwEBwAAALgBEwAAAAJYBAAAA" +
           "AQAqAQEaAAAACwAAAFRydXN0TGlzdElkABH/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAA" +
           "AA==";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public GetTrustListMethodStateMethodCallHandler OnCall;
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

            NodeId applicationId = (NodeId)_inputArguments[0];
            NodeId certificateGroupId = (NodeId)_inputArguments[1];

            NodeId trustListId = (NodeId)_outputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    applicationId,
                    certificateGroupId,
                    ref trustListId);
            }

            _outputArguments[0] = trustListId;

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
    public delegate ServiceResult GetTrustListMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        NodeId applicationId,
        NodeId certificateGroupId,
        ref NodeId trustListId);
    #endif
    #endregion

    #region RevokeCertificateMethodState Class
    #if (!OPCUA_EXCLUDE_RevokeCertificateMethodState)
    /// <summary>
    /// Stores an instance of the RevokeCertificateMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class RevokeCertificateMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public RevokeCertificateMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new RevokeCertificateMethodState(parent);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYYIKBAAAAAEAGwAAAFJl" +
           "dm9rZUNlcnRpZmljYXRlTWV0aG9kVHlwZQEBmToALwEBmTqZOgAAAQH/////AQAAABdgqQoCAAAAAAAO" +
           "AAAASW5wdXRBcmd1bWVudHMBAZo6AC4ARJo6AACWAgAAAAEAKgEBHAAAAA0AAABBcHBsaWNhdGlvbklk" +
           "ABH/////AAAAAAABACoBARoAAAALAAAAQ2VydGlmaWNhdGUAD/////8AAAAAAAEAKAEBAAAAAQAAAAAA" +
           "AAABAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public RevokeCertificateMethodStateMethodCallHandler OnCall;
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

            NodeId applicationId = (NodeId)_inputArguments[0];
            byte[] certificate = (byte[])_inputArguments[1];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    applicationId,
                    certificate);
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
    public delegate ServiceResult RevokeCertificateMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        NodeId applicationId,
        byte[] certificate);
    #endif
    #endregion

    #region GetCertificateStatusMethodState Class
    #if (!OPCUA_EXCLUDE_GetCertificateStatusMethodState)
    /// <summary>
    /// Stores an instance of the GetCertificateStatusMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class GetCertificateStatusMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public GetCertificateStatusMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new GetCertificateStatusMethodState(parent);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYYIKBAAAAAEAHgAAAEdl" +
           "dENlcnRpZmljYXRlU3RhdHVzTWV0aG9kVHlwZQEB2wAALwEB2wDbAAAAAQH/////AgAAABdgqQoCAAAA" +
           "AAAOAAAASW5wdXRBcmd1bWVudHMBAdwAAC4ARNwAAACWAwAAAAEAKgEBHAAAAA0AAABBcHBsaWNhdGlv" +
           "bklkABH/////AAAAAAABACoBASEAAAASAAAAQ2VydGlmaWNhdGVHcm91cElkABH/////AAAAAAABACoB" +
           "ASAAAAARAAAAQ2VydGlmaWNhdGVUeXBlSWQAEf////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8A" +
           "AAAAF2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1bWVudHMBAd0AAC4ARN0AAACWAQAAAAEAKgEBHQAAAA4A" +
           "AABVcGRhdGVSZXF1aXJlZAAB/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public GetCertificateStatusMethodStateMethodCallHandler OnCall;
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

            NodeId applicationId = (NodeId)_inputArguments[0];
            NodeId certificateGroupId = (NodeId)_inputArguments[1];
            NodeId certificateTypeId = (NodeId)_inputArguments[2];

            bool updateRequired = (bool)_outputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    applicationId,
                    certificateGroupId,
                    certificateTypeId,
                    ref updateRequired);
            }

            _outputArguments[0] = updateRequired;

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
    public delegate ServiceResult GetCertificateStatusMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        NodeId applicationId,
        NodeId certificateGroupId,
        NodeId certificateTypeId,
        ref bool updateRequired);
    #endif
    #endregion

    #region CertificateDirectoryState Class
    #if (!OPCUA_EXCLUDE_CertificateDirectoryState)
    /// <summary>
    /// Stores an instance of the CertificateDirectoryType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class CertificateDirectoryState : DirectoryState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public CertificateDirectoryState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Gds.ObjectTypes.CertificateDirectoryType, Opc.Ua.Gds.Namespaces.OpcUaGds, namespaceUris);
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

            if (RevokeCertificate != null)
            {
                RevokeCertificate.Initialize(context, RevokeCertificate_InitializationString);
            }
        }

        #region Initialization String
        private const string RevokeCertificate_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYYIKBAAAAAEAEQAAAFJl" +
           "dm9rZUNlcnRpZmljYXRlAQGbOgAvAQGbOps6AAABAf////8BAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFy" +
           "Z3VtZW50cwEBnDoALgBEnDoAAJYCAAAAAQAqAQEcAAAADQAAAEFwcGxpY2F0aW9uSWQAEf////8AAAAA" +
           "AAEAKgEBGgAAAAsAAABDZXJ0aWZpY2F0ZQAP/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAA" +
           "AAA=";

        private const string InitializationString =
           "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" +
           "AEZpbGVIYW5kbGUAB/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAABGGCCgQAAAAAAAQA" +
           "AABSZWFkAQEMAgAvAQBBLQwCAAABAf////8CAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEB" +
           "DQIALgBEDQIAAJYCAAAAAQAqAQEZAAAACgAAAEZpbGVIYW5kbGUAB/////8AAAAAAAEAKgEBFQAAAAYA" +
           "AABMZW5ndGgABv////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIAAAAAAA8AAABP" +
           "dXRwdXRBcmd1bWVudHMBAQ4CAC4ARA4CAACWAQAAAAEAKgEBEwAAAAQAAABEYXRhAA//////AAAAAAAB" +
           "ACgBAQAAAAEAAAAAAAAAAQH/////AAAAAARhggoEAAAAAAAFAAAAV3JpdGUBAQ8CAC8BAEQtDwIAAAEB" +
           "/////wEAAAAXYKkKAgAAAAAADgAAAElucHV0QXJndW1lbnRzAQEQAgAuAEQQAgAAlgIAAAABACoBARkA" +
           "AAAKAAAARmlsZUhhbmRsZQAH/////wAAAAAAAQAqAQETAAAABAAAAERhdGEAD/////8AAAAAAAEAKAEB" +
           "AAAAAQAAAAAAAAABAf////8AAAAABGGCCgQAAAAAAAsAAABHZXRQb3NpdGlvbgEBEQIALwEARi0RAgAA" +
           "AQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBARICAC4ARBICAACWAQAAAAEAKgEB" +
           "GQAAAAoAAABGaWxlSGFuZGxlAAf/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoC" +
           "AAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQETAgAuAEQTAgAAlgEAAAABACoBARcAAAAIAAAAUG9zaXRp" +
           "b24ACf////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAABGGCCgQAAAAAAAsAAABTZXRQb3Np" +
           "dGlvbgEBFAIALwEASS0UAgAAAQH/////AQAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBARUC" +
           "AC4ARBUCAACWAgAAAAEAKgEBGQAAAAoAAABGaWxlSGFuZGxlAAf/////AAAAAAABACoBARcAAAAIAAAA" +
           "UG9zaXRpb24ACf////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAAFWCJCgIAAAAAAA4AAABM" +
           "YXN0VXBkYXRlVGltZQEBFgIALgBEFgIAAAEAJgH/////AQH/////AAAAAARhggoEAAAAAAANAAAAT3Bl" +
           "bldpdGhNYXNrcwEBFwIALwEA/zAXAgAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVu" +
           "dHMBARgCAC4ARBgCAACWAQAAAAEAKgEBFAAAAAUAAABNYXNrcwAH/////wAAAAAAAQAoAQEAAAABAAAA" +
           "AAAAAAEB/////wAAAAAXYKkKAgAAAAAADwAAAE91dHB1dEFyZ3VtZW50cwEBGQIALgBEGQIAAJYBAAAA" +
           "AQAqAQEZAAAACgAAAEZpbGVIYW5kbGUAB/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAA" +
           "F2CJCgIAAAAAABAAAABDZXJ0aWZpY2F0ZVR5cGVzAQEhAgAuAEQhAgAAABEBAAAAAQAAAAAAAAABAf//" +
           "//8AAAAABGGCCgQAAAABABMAAABTdGFydFNpZ25pbmdSZXF1ZXN0AQFPAAAvAQFPAE8AAAABAf////8C" +
           "AAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBUAAALgBEUAAAAJYEAAAAAQAqAQEcAAAADQAA" +
           "AEFwcGxpY2F0aW9uSWQAEf////8AAAAAAAEAKgEBIQAAABIAAABDZXJ0aWZpY2F0ZUdyb3VwSWQAEf//" +
           "//8AAAAAAAEAKgEBIAAAABEAAABDZXJ0aWZpY2F0ZVR5cGVJZAAR/////wAAAAAAAQAqAQEhAAAAEgAA" +
           "AENlcnRpZmljYXRlUmVxdWVzdAAP/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAXYKkK" +
           "AgAAAAAADwAAAE91dHB1dEFyZ3VtZW50cwEBUQAALgBEUQAAAJYBAAAAAQAqAQEYAAAACQAAAFJlcXVl" +
           "c3RJZAAR/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAEYYIKBAAAAAEAFgAAAFN0YXJ0" +
           "TmV3S2V5UGFpclJlcXVlc3QBAUwAAC8BAUwATAAAAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElucHV0" +
           "QXJndW1lbnRzAQFNAAAuAERNAAAAlgcAAAABACoBARwAAAANAAAAQXBwbGljYXRpb25JZAAR/////wAA" +
           "AAAAAQAqAQEhAAAAEgAAAENlcnRpZmljYXRlR3JvdXBJZAAR/////wAAAAAAAQAqAQEgAAAAEQAAAENl" +
           "cnRpZmljYXRlVHlwZUlkABH/////AAAAAAABACoBARoAAAALAAAAU3ViamVjdE5hbWUADP////8AAAAA" +
           "AAEAKgEBHgAAAAsAAABEb21haW5OYW1lcwAMAQAAAAEAAAAAAAAAAAEAKgEBHwAAABAAAABQcml2YXRl" +
           "S2V5Rm9ybWF0AAz/////AAAAAAABACoBASEAAAASAAAAUHJpdmF0ZUtleVBhc3N3b3JkAAz/////AAAA" +
           "AAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQFO" +
           "AAAuAEROAAAAlgEAAAABACoBARgAAAAJAAAAUmVxdWVzdElkABH/////AAAAAAABACgBAQAAAAEAAAAA" +
           "AAAAAQH/////AAAAAARhggoEAAAAAQANAAAARmluaXNoUmVxdWVzdAEBVQAALwEBVQBVAAAAAQH/////" +
           "AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAVYAAC4ARFYAAACWAgAAAAEAKgEBHAAAAA0A" +
           "AABBcHBsaWNhdGlvbklkABH/////AAAAAAABACoBARgAAAAJAAAAUmVxdWVzdElkABH/////AAAAAAAB" +
           "ACgBAQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQFXAAAu" +
           "AERXAAAAlgMAAAABACoBARoAAAALAAAAQ2VydGlmaWNhdGUAD/////8AAAAAAAEAKgEBGQAAAAoAAABQ" +
           "cml2YXRlS2V5AA//////AAAAAAABACoBASUAAAASAAAASXNzdWVyQ2VydGlmaWNhdGVzAA8BAAAAAQAA" +
           "AAAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAEYYIKBAAAAAEAEQAAAFJldm9rZUNlcnRpZmlj" +
           "YXRlAQGbOgAvAQGbOps6AAABAf////8BAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBnDoA" +
           "LgBEnDoAAJYCAAAAAQAqAQEcAAAADQAAAEFwcGxpY2F0aW9uSWQAEf////8AAAAAAAEAKgEBGgAAAAsA" +
           "AABDZXJ0aWZpY2F0ZQAP/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAEYYIKBAAAAAEA" +
           "FAAAAEdldENlcnRpZmljYXRlR3JvdXBzAQFxAQAvAQFxAXEBAAABAf////8CAAAAF2CpCgIAAAAAAA4A" +
           "AABJbnB1dEFyZ3VtZW50cwEBcgEALgBEcgEAAJYBAAAAAQAqAQEcAAAADQAAAEFwcGxpY2F0aW9uSWQA" +
           "Ef////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1" +
           "bWVudHMBAXMBAC4ARHMBAACWAQAAAAEAKgEBJgAAABMAAABDZXJ0aWZpY2F0ZUdyb3VwSWRzABEBAAAA" +
           "AQAAAAAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAEYYIKBAAAAAEADAAAAEdldFRydXN0TGlz" +
           "dAEBxQAALwEBxQDFAAAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAcYAAC4A" +
           "RMYAAACWAgAAAAEAKgEBHAAAAA0AAABBcHBsaWNhdGlvbklkABH/////AAAAAAABACoBASEAAAASAAAA" +
           "Q2VydGlmaWNhdGVHcm91cElkABH/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoC" +
           "AAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQHHAAAuAETHAAAAlgEAAAABACoBARoAAAALAAAAVHJ1c3RM" +
           "aXN0SWQAEf////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAABGGCCgQAAAABABQAAABHZXRD" +
           "ZXJ0aWZpY2F0ZVN0YXR1cwEB3gAALwEB3gDeAAAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRB" +
           "cmd1bWVudHMBAd8AAC4ARN8AAACWAwAAAAEAKgEBHAAAAA0AAABBcHBsaWNhdGlvbklkABH/////AAAA" +
           "AAABACoBASEAAAASAAAAQ2VydGlmaWNhdGVHcm91cElkABH/////AAAAAAABACoBASAAAAARAAAAQ2Vy" +
           "dGlmaWNhdGVUeXBlSWQAEf////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIAAAAA" +
           "AA8AAABPdXRwdXRBcmd1bWVudHMBAeAAAC4AROAAAACWAQAAAAEAKgEBHQAAAA4AAABVcGRhdGVSZXF1" +
           "aXJlZAAB/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public CertificateGroupFolderState CertificateGroups
        {
            get
            {
                return m_certificateGroups;
            }

            set
            {
                if (!Object.ReferenceEquals(m_certificateGroups, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_certificateGroups = value;
            }
        }

        /// <remarks />
        public StartSigningRequestMethodState StartSigningRequest
        {
            get
            {
                return m_startSigningRequestMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_startSigningRequestMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_startSigningRequestMethod = value;
            }
        }

        /// <remarks />
        public StartNewKeyPairRequestMethodState StartNewKeyPairRequest
        {
            get
            {
                return m_startNewKeyPairRequestMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_startNewKeyPairRequestMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_startNewKeyPairRequestMethod = value;
            }
        }

        /// <remarks />
        public FinishRequestMethodState FinishRequest
        {
            get
            {
                return m_finishRequestMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_finishRequestMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_finishRequestMethod = value;
            }
        }

        /// <remarks />
        public RevokeCertificateMethodState RevokeCertificate
        {
            get
            {
                return m_revokeCertificateMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_revokeCertificateMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_revokeCertificateMethod = value;
            }
        }

        /// <remarks />
        public GetCertificateGroupsMethodState GetCertificateGroups
        {
            get
            {
                return m_getCertificateGroupsMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_getCertificateGroupsMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_getCertificateGroupsMethod = value;
            }
        }

        /// <remarks />
        public GetTrustListMethodState GetTrustList
        {
            get
            {
                return m_getTrustListMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_getTrustListMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_getTrustListMethod = value;
            }
        }

        /// <remarks />
        public GetCertificateStatusMethodState GetCertificateStatus
        {
            get
            {
                return m_getCertificateStatusMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_getCertificateStatusMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_getCertificateStatusMethod = value;
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
            if (m_certificateGroups != null)
            {
                children.Add(m_certificateGroups);
            }

            if (m_startSigningRequestMethod != null)
            {
                children.Add(m_startSigningRequestMethod);
            }

            if (m_startNewKeyPairRequestMethod != null)
            {
                children.Add(m_startNewKeyPairRequestMethod);
            }

            if (m_finishRequestMethod != null)
            {
                children.Add(m_finishRequestMethod);
            }

            if (m_revokeCertificateMethod != null)
            {
                children.Add(m_revokeCertificateMethod);
            }

            if (m_getCertificateGroupsMethod != null)
            {
                children.Add(m_getCertificateGroupsMethod);
            }

            if (m_getTrustListMethod != null)
            {
                children.Add(m_getTrustListMethod);
            }

            if (m_getCertificateStatusMethod != null)
            {
                children.Add(m_getCertificateStatusMethod);
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
                case Opc.Ua.Gds.BrowseNames.CertificateGroups:
                {
                    if (createOrReplace)
                    {
                        if (CertificateGroups == null)
                        {
                            if (replacement == null)
                            {
                                CertificateGroups = new CertificateGroupFolderState(this);
                            }
                            else
                            {
                                CertificateGroups = (CertificateGroupFolderState)replacement;
                            }
                        }
                    }

                    instance = CertificateGroups;
                    break;
                }

                case Opc.Ua.Gds.BrowseNames.StartSigningRequest:
                {
                    if (createOrReplace)
                    {
                        if (StartSigningRequest == null)
                        {
                            if (replacement == null)
                            {
                                StartSigningRequest = new StartSigningRequestMethodState(this);
                            }
                            else
                            {
                                StartSigningRequest = (StartSigningRequestMethodState)replacement;
                            }
                        }
                    }

                    instance = StartSigningRequest;
                    break;
                }

                case Opc.Ua.Gds.BrowseNames.StartNewKeyPairRequest:
                {
                    if (createOrReplace)
                    {
                        if (StartNewKeyPairRequest == null)
                        {
                            if (replacement == null)
                            {
                                StartNewKeyPairRequest = new StartNewKeyPairRequestMethodState(this);
                            }
                            else
                            {
                                StartNewKeyPairRequest = (StartNewKeyPairRequestMethodState)replacement;
                            }
                        }
                    }

                    instance = StartNewKeyPairRequest;
                    break;
                }

                case Opc.Ua.Gds.BrowseNames.FinishRequest:
                {
                    if (createOrReplace)
                    {
                        if (FinishRequest == null)
                        {
                            if (replacement == null)
                            {
                                FinishRequest = new FinishRequestMethodState(this);
                            }
                            else
                            {
                                FinishRequest = (FinishRequestMethodState)replacement;
                            }
                        }
                    }

                    instance = FinishRequest;
                    break;
                }

                case Opc.Ua.Gds.BrowseNames.RevokeCertificate:
                {
                    if (createOrReplace)
                    {
                        if (RevokeCertificate == null)
                        {
                            if (replacement == null)
                            {
                                RevokeCertificate = new RevokeCertificateMethodState(this);
                            }
                            else
                            {
                                RevokeCertificate = (RevokeCertificateMethodState)replacement;
                            }
                        }
                    }

                    instance = RevokeCertificate;
                    break;
                }

                case Opc.Ua.Gds.BrowseNames.GetCertificateGroups:
                {
                    if (createOrReplace)
                    {
                        if (GetCertificateGroups == null)
                        {
                            if (replacement == null)
                            {
                                GetCertificateGroups = new GetCertificateGroupsMethodState(this);
                            }
                            else
                            {
                                GetCertificateGroups = (GetCertificateGroupsMethodState)replacement;
                            }
                        }
                    }

                    instance = GetCertificateGroups;
                    break;
                }

                case Opc.Ua.Gds.BrowseNames.GetTrustList:
                {
                    if (createOrReplace)
                    {
                        if (GetTrustList == null)
                        {
                            if (replacement == null)
                            {
                                GetTrustList = new GetTrustListMethodState(this);
                            }
                            else
                            {
                                GetTrustList = (GetTrustListMethodState)replacement;
                            }
                        }
                    }

                    instance = GetTrustList;
                    break;
                }

                case Opc.Ua.Gds.BrowseNames.GetCertificateStatus:
                {
                    if (createOrReplace)
                    {
                        if (GetCertificateStatus == null)
                        {
                            if (replacement == null)
                            {
                                GetCertificateStatus = new GetCertificateStatusMethodState(this);
                            }
                            else
                            {
                                GetCertificateStatus = (GetCertificateStatusMethodState)replacement;
                            }
                        }
                    }

                    instance = GetCertificateStatus;
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
        private CertificateGroupFolderState m_certificateGroups;
        private StartSigningRequestMethodState m_startSigningRequestMethod;
        private StartNewKeyPairRequestMethodState m_startNewKeyPairRequestMethod;
        private FinishRequestMethodState m_finishRequestMethod;
        private RevokeCertificateMethodState m_revokeCertificateMethod;
        private GetCertificateGroupsMethodState m_getCertificateGroupsMethod;
        private GetTrustListMethodState m_getTrustListMethod;
        private GetCertificateStatusMethodState m_getCertificateStatusMethod;
        #endregion
    }
    #endif
    #endregion

    #region CertificateRequestedAuditEventState Class
    #if (!OPCUA_EXCLUDE_CertificateRequestedAuditEventState)
    /// <summary>
    /// Stores an instance of the CertificateRequestedAuditEventType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class CertificateRequestedAuditEventState : AuditUpdateMethodEventState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public CertificateRequestedAuditEventState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Gds.ObjectTypes.CertificateRequestedAuditEventType, Opc.Ua.Gds.Namespaces.OpcUaGds, namespaceUris);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYIACAQAAAAEAKgAAAENl" +
           "cnRpZmljYXRlUmVxdWVzdGVkQXVkaXRFdmVudFR5cGVJbnN0YW5jZQEBWwABAVsAWwAAAP////8RAAAA" +
           "FWCJCgIAAAAAAAcAAABFdmVudElkAQFcAAAuAERcAAAAAA//////AQH/////AAAAABVgiQoCAAAAAAAJ" +
           "AAAARXZlbnRUeXBlAQFdAAAuAERdAAAAABH/////AQH/////AAAAABVgiQoCAAAAAAAKAAAAU291cmNl" +
           "Tm9kZQEBXgAALgBEXgAAAAAR/////wEB/////wAAAAAVYIkKAgAAAAAACgAAAFNvdXJjZU5hbWUBAV8A" +
           "AC4ARF8AAAAADP////8BAf////8AAAAAFWCJCgIAAAAAAAQAAABUaW1lAQFgAAAuAERgAAAAAQAmAf//" +
           "//8BAf////8AAAAAFWCJCgIAAAAAAAsAAABSZWNlaXZlVGltZQEBYQAALgBEYQAAAAEAJgH/////AQH/" +
           "////AAAAABVgiQoCAAAAAAAHAAAATWVzc2FnZQEBYwAALgBEYwAAAAAV/////wEB/////wAAAAAVYIkK" +
           "AgAAAAAACAAAAFNldmVyaXR5AQFkAAAuAERkAAAAAAX/////AQH/////AAAAABVgiQoCAAAAAAAPAAAA" +
           "QWN0aW9uVGltZVN0YW1wAQFlAAAuAERlAAAAAQAmAf////8BAf////8AAAAAFWCJCgIAAAAAAAYAAABT" +
           "dGF0dXMBAWYAAC4ARGYAAAAAAf////8BAf////8AAAAAFWCJCgIAAAAAAAgAAABTZXJ2ZXJJZAEBZwAA" +
           "LgBEZwAAAAAM/////wEB/////wAAAAAVYIkKAgAAAAAAEgAAAENsaWVudEF1ZGl0RW50cnlJZAEBaAAA" +
           "LgBEaAAAAAAM/////wEB/////wAAAAAVYIkKAgAAAAAADAAAAENsaWVudFVzZXJJZAEBaQAALgBEaQAA" +
           "AAAM/////wEB/////wAAAAAVYIkKAgAAAAAACAAAAE1ldGhvZElkAQFqAAAuAERqAAAAABH/////AQH/" +
           "////AAAAABdgiQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAWsAAC4ARGsAAAAAGAEAAAABAAAAAAAA" +
           "AAEB/////wAAAAAVYIkKAgAAAAEAEAAAAENlcnRpZmljYXRlR3JvdXABAc0CAC4ARM0CAAAAEf////8B" +
           "Af////8AAAAAFWCJCgIAAAABAA8AAABDZXJ0aWZpY2F0ZVR5cGUBAc4CAC4ARM4CAAAAEf////8BAf//" +
           "//8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public PropertyState<NodeId> CertificateGroup
        {
            get
            {
                return m_certificateGroup;
            }

            set
            {
                if (!Object.ReferenceEquals(m_certificateGroup, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_certificateGroup = value;
            }
        }

        /// <remarks />
        public PropertyState<NodeId> CertificateType
        {
            get
            {
                return m_certificateType;
            }

            set
            {
                if (!Object.ReferenceEquals(m_certificateType, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_certificateType = value;
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
            if (m_certificateGroup != null)
            {
                children.Add(m_certificateGroup);
            }

            if (m_certificateType != null)
            {
                children.Add(m_certificateType);
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
                case Opc.Ua.Gds.BrowseNames.CertificateGroup:
                {
                    if (createOrReplace)
                    {
                        if (CertificateGroup == null)
                        {
                            if (replacement == null)
                            {
                                CertificateGroup = new PropertyState<NodeId>(this);
                            }
                            else
                            {
                                CertificateGroup = (PropertyState<NodeId>)replacement;
                            }
                        }
                    }

                    instance = CertificateGroup;
                    break;
                }

                case Opc.Ua.Gds.BrowseNames.CertificateType:
                {
                    if (createOrReplace)
                    {
                        if (CertificateType == null)
                        {
                            if (replacement == null)
                            {
                                CertificateType = new PropertyState<NodeId>(this);
                            }
                            else
                            {
                                CertificateType = (PropertyState<NodeId>)replacement;
                            }
                        }
                    }

                    instance = CertificateType;
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
        private PropertyState<NodeId> m_certificateGroup;
        private PropertyState<NodeId> m_certificateType;
        #endregion
    }
    #endif
    #endregion

    #region CertificateDeliveredAuditEventState Class
    #if (!OPCUA_EXCLUDE_CertificateDeliveredAuditEventState)
    /// <summary>
    /// Stores an instance of the CertificateDeliveredAuditEventType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class CertificateDeliveredAuditEventState : AuditUpdateMethodEventState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public CertificateDeliveredAuditEventState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Gds.ObjectTypes.CertificateDeliveredAuditEventType, Opc.Ua.Gds.Namespaces.OpcUaGds, namespaceUris);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYIACAQAAAAEAKgAAAENl" +
           "cnRpZmljYXRlRGVsaXZlcmVkQXVkaXRFdmVudFR5cGVJbnN0YW5jZQEBbQABAW0AbQAAAP////8RAAAA" +
           "FWCJCgIAAAAAAAcAAABFdmVudElkAQFuAAAuAERuAAAAAA//////AQH/////AAAAABVgiQoCAAAAAAAJ" +
           "AAAARXZlbnRUeXBlAQFvAAAuAERvAAAAABH/////AQH/////AAAAABVgiQoCAAAAAAAKAAAAU291cmNl" +
           "Tm9kZQEBcAAALgBEcAAAAAAR/////wEB/////wAAAAAVYIkKAgAAAAAACgAAAFNvdXJjZU5hbWUBAXEA" +
           "AC4ARHEAAAAADP////8BAf////8AAAAAFWCJCgIAAAAAAAQAAABUaW1lAQFyAAAuAERyAAAAAQAmAf//" +
           "//8BAf////8AAAAAFWCJCgIAAAAAAAsAAABSZWNlaXZlVGltZQEBcwAALgBEcwAAAAEAJgH/////AQH/" +
           "////AAAAABVgiQoCAAAAAAAHAAAATWVzc2FnZQEBdQAALgBEdQAAAAAV/////wEB/////wAAAAAVYIkK" +
           "AgAAAAAACAAAAFNldmVyaXR5AQF2AAAuAER2AAAAAAX/////AQH/////AAAAABVgiQoCAAAAAAAPAAAA" +
           "QWN0aW9uVGltZVN0YW1wAQF3AAAuAER3AAAAAQAmAf////8BAf////8AAAAAFWCJCgIAAAAAAAYAAABT" +
           "dGF0dXMBAXgAAC4ARHgAAAAAAf////8BAf////8AAAAAFWCJCgIAAAAAAAgAAABTZXJ2ZXJJZAEBeQAA" +
           "LgBEeQAAAAAM/////wEB/////wAAAAAVYIkKAgAAAAAAEgAAAENsaWVudEF1ZGl0RW50cnlJZAEBegAA" +
           "LgBEegAAAAAM/////wEB/////wAAAAAVYIkKAgAAAAAADAAAAENsaWVudFVzZXJJZAEBewAALgBEewAA" +
           "AAAM/////wEB/////wAAAAAVYIkKAgAAAAAACAAAAE1ldGhvZElkAQF8AAAuAER8AAAAABH/////AQH/" +
           "////AAAAABdgiQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAX0AAC4ARH0AAAAAGAEAAAABAAAAAAAA" +
           "AAEB/////wAAAAAVYIkKAgAAAAEAEAAAAENlcnRpZmljYXRlR3JvdXABAc8CAC4ARM8CAAAAEf////8B" +
           "Af////8AAAAAFWCJCgIAAAABAA8AAABDZXJ0aWZpY2F0ZVR5cGUBAdACAC4ARNACAAAAEf////8BAf//" +
           "//8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public PropertyState<NodeId> CertificateGroup
        {
            get
            {
                return m_certificateGroup;
            }

            set
            {
                if (!Object.ReferenceEquals(m_certificateGroup, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_certificateGroup = value;
            }
        }

        /// <remarks />
        public PropertyState<NodeId> CertificateType
        {
            get
            {
                return m_certificateType;
            }

            set
            {
                if (!Object.ReferenceEquals(m_certificateType, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_certificateType = value;
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
            if (m_certificateGroup != null)
            {
                children.Add(m_certificateGroup);
            }

            if (m_certificateType != null)
            {
                children.Add(m_certificateType);
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
                case Opc.Ua.Gds.BrowseNames.CertificateGroup:
                {
                    if (createOrReplace)
                    {
                        if (CertificateGroup == null)
                        {
                            if (replacement == null)
                            {
                                CertificateGroup = new PropertyState<NodeId>(this);
                            }
                            else
                            {
                                CertificateGroup = (PropertyState<NodeId>)replacement;
                            }
                        }
                    }

                    instance = CertificateGroup;
                    break;
                }

                case Opc.Ua.Gds.BrowseNames.CertificateType:
                {
                    if (createOrReplace)
                    {
                        if (CertificateType == null)
                        {
                            if (replacement == null)
                            {
                                CertificateType = new PropertyState<NodeId>(this);
                            }
                            else
                            {
                                CertificateType = (PropertyState<NodeId>)replacement;
                            }
                        }
                    }

                    instance = CertificateType;
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
        private PropertyState<NodeId> m_certificateGroup;
        private PropertyState<NodeId> m_certificateType;
        #endregion
    }
    #endif
    #endregion

    #region KeyCredentialServiceState Class
    #if (!OPCUA_EXCLUDE_KeyCredentialServiceState)
    /// <summary>
    /// Stores an instance of the KeyCredentialServiceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class KeyCredentialServiceState : BaseObjectState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public KeyCredentialServiceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Gds.ObjectTypes.KeyCredentialServiceType, Opc.Ua.Gds.Namespaces.OpcUaGds, namespaceUris);
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

            if (Revoke != null)
            {
                Revoke.Initialize(context, Revoke_InitializationString);
            }
        }

        #region Initialization String
        private const string Revoke_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYYIKBAAAAAEABgAAAFJl" +
           "dm9rZQEBBQQALwEBBQQFBAAAAQH/////AQAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAQYE" +
           "AC4ARAYEAACWAQAAAAEAKgEBGwAAAAwAAABDcmVkZW50aWFsSWQADP////8AAAAAAAEAKAEBAAAAAQAA" +
           "AAAAAAABAf////8AAAAA";

        private const string InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYIACAQAAAAEAIAAAAEtl" +
           "eUNyZWRlbnRpYWxTZXJ2aWNlVHlwZUluc3RhbmNlAQH8AwEB/AP8AwAA/////wUAAAAVYIkKAgAAAAEA" +
           "CwAAAFJlc291cmNlVXJpAQH9AwAuAET9AwAAAAz/////AQH/////AAAAABdgiQoCAAAAAQALAAAAUHJv" +
           "ZmlsZVVyaXMBAf4DAC4ARP4DAAAADAEAAAABAAAAAAAAAAEB/////wAAAAAEYYIKBAAAAAEADAAAAFN0" +
           "YXJ0UmVxdWVzdAEB/wMALwEB/wP/AwAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVu" +
           "dHMBAQAEAC4ARAAEAACWBAAAAAEAKgEBHQAAAA4AAABBcHBsaWNhdGlvblVyaQAM/////wAAAAAAAQAq" +
           "AQEYAAAACQAAAFB1YmxpY0tleQAP/////wAAAAAAAQAqAQEgAAAAEQAAAFNlY3VyaXR5UG9saWN5VXJp" +
           "AAz/////AAAAAAABACoBASEAAAAOAAAAUmVxdWVzdGVkUm9sZXMAEQEAAAABAAAAAAAAAAABACgBAQAA" +
           "AAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQEBBAAuAEQBBAAA" +
           "lgEAAAABACoBARgAAAAJAAAAUmVxdWVzdElkABH/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////" +
           "AAAAAARhggoEAAAAAQANAAAARmluaXNoUmVxdWVzdAEBAgQALwEBAgQCBAAAAQH/////AgAAABdgqQoC" +
           "AAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAQMEAC4ARAMEAACWAgAAAAEAKgEBGAAAAAkAAABSZXF1ZXN0" +
           "SWQAEf////8AAAAAAAEAKgEBHAAAAA0AAABDYW5jZWxSZXF1ZXN0AAH/////AAAAAAABACgBAQAAAAEA" +
           "AAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQEEBAAuAEQEBAAAlgUA" +
           "AAABACoBARsAAAAMAAAAQ3JlZGVudGlhbElkAAz/////AAAAAAABACoBAR8AAAAQAAAAQ3JlZGVudGlh" +
           "bFNlY3JldAAP/////wAAAAAAAQAqAQEkAAAAFQAAAENlcnRpZmljYXRlVGh1bWJwcmludAAM/////wAA" +
           "AAAAAQAqAQEgAAAAEQAAAFNlY3VyaXR5UG9saWN5VXJpAAz/////AAAAAAABACoBAR8AAAAMAAAAR3Jh" +
           "bnRlZFJvbGVzABEBAAAAAQAAAAAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAEYYIKBAAAAAEA" +
           "BgAAAFJldm9rZQEBBQQALwEBBQQFBAAAAQH/////AQAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVu" +
           "dHMBAQYEAC4ARAYEAACWAQAAAAEAKgEBGwAAAAwAAABDcmVkZW50aWFsSWQADP////8AAAAAAAEAKAEB" +
           "AAAAAQAAAAAAAAABAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public PropertyState<string> ResourceUri
        {
            get
            {
                return m_resourceUri;
            }

            set
            {
                if (!Object.ReferenceEquals(m_resourceUri, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_resourceUri = value;
            }
        }

        /// <remarks />
        public PropertyState<string[]> ProfileUris
        {
            get
            {
                return m_profileUris;
            }

            set
            {
                if (!Object.ReferenceEquals(m_profileUris, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_profileUris = value;
            }
        }

        /// <remarks />
        public KeyCredentialStartRequestMethodState StartRequest
        {
            get
            {
                return m_startRequestMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_startRequestMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_startRequestMethod = value;
            }
        }

        /// <remarks />
        public KeyCredentialFinishRequestMethodState FinishRequest
        {
            get
            {
                return m_finishRequestMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_finishRequestMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_finishRequestMethod = value;
            }
        }

        /// <remarks />
        public KeyCredentialRevokeMethodState Revoke
        {
            get
            {
                return m_revokeMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_revokeMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_revokeMethod = value;
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
            if (m_resourceUri != null)
            {
                children.Add(m_resourceUri);
            }

            if (m_profileUris != null)
            {
                children.Add(m_profileUris);
            }

            if (m_startRequestMethod != null)
            {
                children.Add(m_startRequestMethod);
            }

            if (m_finishRequestMethod != null)
            {
                children.Add(m_finishRequestMethod);
            }

            if (m_revokeMethod != null)
            {
                children.Add(m_revokeMethod);
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
                case Opc.Ua.Gds.BrowseNames.ResourceUri:
                {
                    if (createOrReplace)
                    {
                        if (ResourceUri == null)
                        {
                            if (replacement == null)
                            {
                                ResourceUri = new PropertyState<string>(this);
                            }
                            else
                            {
                                ResourceUri = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = ResourceUri;
                    break;
                }

                case Opc.Ua.Gds.BrowseNames.ProfileUris:
                {
                    if (createOrReplace)
                    {
                        if (ProfileUris == null)
                        {
                            if (replacement == null)
                            {
                                ProfileUris = new PropertyState<string[]>(this);
                            }
                            else
                            {
                                ProfileUris = (PropertyState<string[]>)replacement;
                            }
                        }
                    }

                    instance = ProfileUris;
                    break;
                }

                case Opc.Ua.Gds.BrowseNames.StartRequest:
                {
                    if (createOrReplace)
                    {
                        if (StartRequest == null)
                        {
                            if (replacement == null)
                            {
                                StartRequest = new KeyCredentialStartRequestMethodState(this);
                            }
                            else
                            {
                                StartRequest = (KeyCredentialStartRequestMethodState)replacement;
                            }
                        }
                    }

                    instance = StartRequest;
                    break;
                }

                case Opc.Ua.Gds.BrowseNames.FinishRequest:
                {
                    if (createOrReplace)
                    {
                        if (FinishRequest == null)
                        {
                            if (replacement == null)
                            {
                                FinishRequest = new KeyCredentialFinishRequestMethodState(this);
                            }
                            else
                            {
                                FinishRequest = (KeyCredentialFinishRequestMethodState)replacement;
                            }
                        }
                    }

                    instance = FinishRequest;
                    break;
                }

                case Opc.Ua.Gds.BrowseNames.Revoke:
                {
                    if (createOrReplace)
                    {
                        if (Revoke == null)
                        {
                            if (replacement == null)
                            {
                                Revoke = new KeyCredentialRevokeMethodState(this);
                            }
                            else
                            {
                                Revoke = (KeyCredentialRevokeMethodState)replacement;
                            }
                        }
                    }

                    instance = Revoke;
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
        private PropertyState<string> m_resourceUri;
        private PropertyState<string[]> m_profileUris;
        private KeyCredentialStartRequestMethodState m_startRequestMethod;
        private KeyCredentialFinishRequestMethodState m_finishRequestMethod;
        private KeyCredentialRevokeMethodState m_revokeMethod;
        #endregion
    }
    #endif
    #endregion

    #region KeyCredentialStartRequestMethodState Class
    #if (!OPCUA_EXCLUDE_KeyCredentialStartRequestMethodState)
    /// <summary>
    /// Stores an instance of the KeyCredentialStartRequestMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class KeyCredentialStartRequestMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public KeyCredentialStartRequestMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new KeyCredentialStartRequestMethodState(parent);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYYIKBAAAAAEAIwAAAEtl" +
           "eUNyZWRlbnRpYWxTdGFydFJlcXVlc3RNZXRob2RUeXBlAQEHBAAvAQEHBAcEAAABAf////8CAAAAF2Cp" +
           "CgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBCAQALgBECAQAAJYEAAAAAQAqAQEdAAAADgAAAEFwcGxp" +
           "Y2F0aW9uVXJpAAz/////AAAAAAABACoBARgAAAAJAAAAUHVibGljS2V5AA//////AAAAAAABACoBASAA" +
           "AAARAAAAU2VjdXJpdHlQb2xpY3lVcmkADP////8AAAAAAAEAKgEBIQAAAA4AAABSZXF1ZXN0ZWRSb2xl" +
           "cwARAQAAAAEAAAAAAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIAAAAAAA8AAABPdXRw" +
           "dXRBcmd1bWVudHMBAQkEAC4ARAkEAACWAQAAAAEAKgEBGAAAAAkAAABSZXF1ZXN0SWQAEf////8AAAAA" +
           "AAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public KeyCredentialStartRequestMethodStateMethodCallHandler OnCall;
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

            string applicationUri = (string)_inputArguments[0];
            byte[] publicKey = (byte[])_inputArguments[1];
            string securityPolicyUri = (string)_inputArguments[2];
            NodeId[] requestedRoles = (NodeId[])_inputArguments[3];

            NodeId requestId = (NodeId)_outputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    applicationUri,
                    publicKey,
                    securityPolicyUri,
                    requestedRoles,
                    ref requestId);
            }

            _outputArguments[0] = requestId;

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
    public delegate ServiceResult KeyCredentialStartRequestMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        string applicationUri,
        byte[] publicKey,
        string securityPolicyUri,
        NodeId[] requestedRoles,
        ref NodeId requestId);
    #endif
    #endregion

    #region KeyCredentialFinishRequestMethodState Class
    #if (!OPCUA_EXCLUDE_KeyCredentialFinishRequestMethodState)
    /// <summary>
    /// Stores an instance of the KeyCredentialFinishRequestMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class KeyCredentialFinishRequestMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public KeyCredentialFinishRequestMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new KeyCredentialFinishRequestMethodState(parent);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYYIKBAAAAAEAJAAAAEtl" +
           "eUNyZWRlbnRpYWxGaW5pc2hSZXF1ZXN0TWV0aG9kVHlwZQEBCgQALwEBCgQKBAAAAQH/////AgAAABdg" +
           "qQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAQsEAC4ARAsEAACWAgAAAAEAKgEBGAAAAAkAAABSZXF1" +
           "ZXN0SWQAEf////8AAAAAAAEAKgEBHAAAAA0AAABDYW5jZWxSZXF1ZXN0AAH/////AAAAAAABACgBAQAA" +
           "AAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQEMBAAuAEQMBAAA" +
           "lgUAAAABACoBARsAAAAMAAAAQ3JlZGVudGlhbElkAAz/////AAAAAAABACoBAR8AAAAQAAAAQ3JlZGVu" +
           "dGlhbFNlY3JldAAP/////wAAAAAAAQAqAQEkAAAAFQAAAENlcnRpZmljYXRlVGh1bWJwcmludAAM////" +
           "/wAAAAAAAQAqAQEgAAAAEQAAAFNlY3VyaXR5UG9saWN5VXJpAAz/////AAAAAAABACoBAR8AAAAMAAAA" +
           "R3JhbnRlZFJvbGVzABEBAAAAAQAAAAAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public KeyCredentialFinishRequestMethodStateMethodCallHandler OnCall;
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

            NodeId requestId = (NodeId)_inputArguments[0];
            bool cancelRequest = (bool)_inputArguments[1];

            string credentialId = (string)_outputArguments[0];
            byte[] credentialSecret = (byte[])_outputArguments[1];
            string certificateThumbprint = (string)_outputArguments[2];
            string securityPolicyUri = (string)_outputArguments[3];
            NodeId[] grantedRoles = (NodeId[])_outputArguments[4];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    requestId,
                    cancelRequest,
                    ref credentialId,
                    ref credentialSecret,
                    ref certificateThumbprint,
                    ref securityPolicyUri,
                    ref grantedRoles);
            }

            _outputArguments[0] = credentialId;
            _outputArguments[1] = credentialSecret;
            _outputArguments[2] = certificateThumbprint;
            _outputArguments[3] = securityPolicyUri;
            _outputArguments[4] = grantedRoles;

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
    public delegate ServiceResult KeyCredentialFinishRequestMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        NodeId requestId,
        bool cancelRequest,
        ref string credentialId,
        ref byte[] credentialSecret,
        ref string certificateThumbprint,
        ref string securityPolicyUri,
        ref NodeId[] grantedRoles);
    #endif
    #endregion

    #region KeyCredentialRevokeMethodState Class
    #if (!OPCUA_EXCLUDE_KeyCredentialRevokeMethodState)
    /// <summary>
    /// Stores an instance of the KeyCredentialRevokeMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class KeyCredentialRevokeMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public KeyCredentialRevokeMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new KeyCredentialRevokeMethodState(parent);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYYIKBAAAAAEAHQAAAEtl" +
           "eUNyZWRlbnRpYWxSZXZva2VNZXRob2RUeXBlAQENBAAvAQENBA0EAAABAf////8BAAAAF2CpCgIAAAAA" +
           "AA4AAABJbnB1dEFyZ3VtZW50cwEBDgQALgBEDgQAAJYBAAAAAQAqAQEbAAAADAAAAENyZWRlbnRpYWxJ" +
           "ZAAM/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public KeyCredentialRevokeMethodStateMethodCallHandler OnCall;
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

            string credentialId = (string)_inputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    credentialId);
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
    public delegate ServiceResult KeyCredentialRevokeMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        string credentialId);
    #endif
    #endregion

    #region KeyCredentialRequestedAuditEventState Class
    #if (!OPCUA_EXCLUDE_KeyCredentialRequestedAuditEventState)
    /// <summary>
    /// Stores an instance of the KeyCredentialRequestedAuditEventType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class KeyCredentialRequestedAuditEventState : KeyCredentialAuditEventState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public KeyCredentialRequestedAuditEventState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Gds.ObjectTypes.KeyCredentialRequestedAuditEventType, Opc.Ua.Gds.Namespaces.OpcUaGds, namespaceUris);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYIACAQAAAAEALAAAAEtl" +
           "eUNyZWRlbnRpYWxSZXF1ZXN0ZWRBdWRpdEV2ZW50VHlwZUluc3RhbmNlAQEPBAEBDwQPBAAA/////xAA" +
           "AAAVYIkKAgAAAAAABwAAAEV2ZW50SWQBARAEAC4ARBAEAAAAD/////8BAf////8AAAAAFWCJCgIAAAAA" +
           "AAkAAABFdmVudFR5cGUBAREEAC4ARBEEAAAAEf////8BAf////8AAAAAFWCJCgIAAAAAAAoAAABTb3Vy" +
           "Y2VOb2RlAQESBAAuAEQSBAAAABH/////AQH/////AAAAABVgiQoCAAAAAAAKAAAAU291cmNlTmFtZQEB" +
           "EwQALgBEEwQAAAAM/////wEB/////wAAAAAVYIkKAgAAAAAABAAAAFRpbWUBARQEAC4ARBQEAAABACYB" +
           "/////wEB/////wAAAAAVYIkKAgAAAAAACwAAAFJlY2VpdmVUaW1lAQEVBAAuAEQVBAAAAQAmAf////8B" +
           "Af////8AAAAAFWCJCgIAAAAAAAcAAABNZXNzYWdlAQEXBAAuAEQXBAAAABX/////AQH/////AAAAABVg" +
           "iQoCAAAAAAAIAAAAU2V2ZXJpdHkBARgEAC4ARBgEAAAABf////8BAf////8AAAAAFWCJCgIAAAAAAA8A" +
           "AABBY3Rpb25UaW1lU3RhbXABARkEAC4ARBkEAAABACYB/////wEB/////wAAAAAVYIkKAgAAAAAABgAA" +
           "AFN0YXR1cwEBGgQALgBEGgQAAAAB/////wEB/////wAAAAAVYIkKAgAAAAAACAAAAFNlcnZlcklkAQEb" +
           "BAAuAEQbBAAAAAz/////AQH/////AAAAABVgiQoCAAAAAAASAAAAQ2xpZW50QXVkaXRFbnRyeUlkAQEc" +
           "BAAuAEQcBAAAAAz/////AQH/////AAAAABVgiQoCAAAAAAAMAAAAQ2xpZW50VXNlcklkAQEdBAAuAEQd" +
           "BAAAAAz/////AQH/////AAAAABVgiQoCAAAAAAAIAAAATWV0aG9kSWQBAR4EAC4ARB4EAAAAEf////8B" +
           "Af////8AAAAAF2CJCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBHwQALgBEHwQAAAAYAQAAAAEAAAAA" +
           "AAAAAQH/////AAAAABVgiQoCAAAAAAALAAAAUmVzb3VyY2VVcmkBASAEAC4ARCAEAAAADP////8BAf//" +
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

    #region KeyCredentialDeliveredAuditEventState Class
    #if (!OPCUA_EXCLUDE_KeyCredentialDeliveredAuditEventState)
    /// <summary>
    /// Stores an instance of the KeyCredentialDeliveredAuditEventType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class KeyCredentialDeliveredAuditEventState : KeyCredentialAuditEventState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public KeyCredentialDeliveredAuditEventState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Gds.ObjectTypes.KeyCredentialDeliveredAuditEventType, Opc.Ua.Gds.Namespaces.OpcUaGds, namespaceUris);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYIACAQAAAAEALAAAAEtl" +
           "eUNyZWRlbnRpYWxEZWxpdmVyZWRBdWRpdEV2ZW50VHlwZUluc3RhbmNlAQEhBAEBIQQhBAAA/////xAA" +
           "AAAVYIkKAgAAAAAABwAAAEV2ZW50SWQBASIEAC4ARCIEAAAAD/////8BAf////8AAAAAFWCJCgIAAAAA" +
           "AAkAAABFdmVudFR5cGUBASMEAC4ARCMEAAAAEf////8BAf////8AAAAAFWCJCgIAAAAAAAoAAABTb3Vy" +
           "Y2VOb2RlAQEkBAAuAEQkBAAAABH/////AQH/////AAAAABVgiQoCAAAAAAAKAAAAU291cmNlTmFtZQEB" +
           "JQQALgBEJQQAAAAM/////wEB/////wAAAAAVYIkKAgAAAAAABAAAAFRpbWUBASYEAC4ARCYEAAABACYB" +
           "/////wEB/////wAAAAAVYIkKAgAAAAAACwAAAFJlY2VpdmVUaW1lAQEnBAAuAEQnBAAAAQAmAf////8B" +
           "Af////8AAAAAFWCJCgIAAAAAAAcAAABNZXNzYWdlAQEpBAAuAEQpBAAAABX/////AQH/////AAAAABVg" +
           "iQoCAAAAAAAIAAAAU2V2ZXJpdHkBASoEAC4ARCoEAAAABf////8BAf////8AAAAAFWCJCgIAAAAAAA8A" +
           "AABBY3Rpb25UaW1lU3RhbXABASsEAC4ARCsEAAABACYB/////wEB/////wAAAAAVYIkKAgAAAAAABgAA" +
           "AFN0YXR1cwEBLAQALgBELAQAAAAB/////wEB/////wAAAAAVYIkKAgAAAAAACAAAAFNlcnZlcklkAQEt" +
           "BAAuAEQtBAAAAAz/////AQH/////AAAAABVgiQoCAAAAAAASAAAAQ2xpZW50QXVkaXRFbnRyeUlkAQEu" +
           "BAAuAEQuBAAAAAz/////AQH/////AAAAABVgiQoCAAAAAAAMAAAAQ2xpZW50VXNlcklkAQEvBAAuAEQv" +
           "BAAAAAz/////AQH/////AAAAABVgiQoCAAAAAAAIAAAATWV0aG9kSWQBATAEAC4ARDAEAAAAEf////8B" +
           "Af////8AAAAAF2CJCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBMQQALgBEMQQAAAAYAQAAAAEAAAAA" +
           "AAAAAQH/////AAAAABVgiQoCAAAAAAALAAAAUmVzb3VyY2VVcmkBATIEAC4ARDIEAAAADP////8BAf//" +
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

    #region KeyCredentialRevokedAuditEventState Class
    #if (!OPCUA_EXCLUDE_KeyCredentialRevokedAuditEventState)
    /// <summary>
    /// Stores an instance of the KeyCredentialRevokedAuditEventType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class KeyCredentialRevokedAuditEventState : KeyCredentialAuditEventState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public KeyCredentialRevokedAuditEventState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Gds.ObjectTypes.KeyCredentialRevokedAuditEventType, Opc.Ua.Gds.Namespaces.OpcUaGds, namespaceUris);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYIACAQAAAAEAKgAAAEtl" +
           "eUNyZWRlbnRpYWxSZXZva2VkQXVkaXRFdmVudFR5cGVJbnN0YW5jZQEBMwQBATMEMwQAAP////8QAAAA" +
           "FWCJCgIAAAAAAAcAAABFdmVudElkAQE0BAAuAEQ0BAAAAA//////AQH/////AAAAABVgiQoCAAAAAAAJ" +
           "AAAARXZlbnRUeXBlAQE1BAAuAEQ1BAAAABH/////AQH/////AAAAABVgiQoCAAAAAAAKAAAAU291cmNl" +
           "Tm9kZQEBNgQALgBENgQAAAAR/////wEB/////wAAAAAVYIkKAgAAAAAACgAAAFNvdXJjZU5hbWUBATcE" +
           "AC4ARDcEAAAADP////8BAf////8AAAAAFWCJCgIAAAAAAAQAAABUaW1lAQE4BAAuAEQ4BAAAAQAmAf//" +
           "//8BAf////8AAAAAFWCJCgIAAAAAAAsAAABSZWNlaXZlVGltZQEBOQQALgBEOQQAAAEAJgH/////AQH/" +
           "////AAAAABVgiQoCAAAAAAAHAAAATWVzc2FnZQEBOwQALgBEOwQAAAAV/////wEB/////wAAAAAVYIkK" +
           "AgAAAAAACAAAAFNldmVyaXR5AQE8BAAuAEQ8BAAAAAX/////AQH/////AAAAABVgiQoCAAAAAAAPAAAA" +
           "QWN0aW9uVGltZVN0YW1wAQE9BAAuAEQ9BAAAAQAmAf////8BAf////8AAAAAFWCJCgIAAAAAAAYAAABT" +
           "dGF0dXMBAT4EAC4ARD4EAAAAAf////8BAf////8AAAAAFWCJCgIAAAAAAAgAAABTZXJ2ZXJJZAEBPwQA" +
           "LgBEPwQAAAAM/////wEB/////wAAAAAVYIkKAgAAAAAAEgAAAENsaWVudEF1ZGl0RW50cnlJZAEBQAQA" +
           "LgBEQAQAAAAM/////wEB/////wAAAAAVYIkKAgAAAAAADAAAAENsaWVudFVzZXJJZAEBQQQALgBEQQQA" +
           "AAAM/////wEB/////wAAAAAVYIkKAgAAAAAACAAAAE1ldGhvZElkAQFCBAAuAERCBAAAABH/////AQH/" +
           "////AAAAABdgiQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAUMEAC4AREMEAAAAGAEAAAABAAAAAAAA" +
           "AAEB/////wAAAAAVYIkKAgAAAAAACwAAAFJlc291cmNlVXJpAQFEBAAuAEREBAAAAAz/////AQH/////" +
           "AAAAAA==";
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

    #region AuthorizationServiceState Class
    #if (!OPCUA_EXCLUDE_AuthorizationServiceState)
    /// <summary>
    /// Stores an instance of the AuthorizationServiceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class AuthorizationServiceState : BaseObjectState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public AuthorizationServiceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Gds.ObjectTypes.AuthorizationServiceType, Opc.Ua.Gds.Namespaces.OpcUaGds, namespaceUris);
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

            if (UserTokenPolicies != null)
            {
                UserTokenPolicies.Initialize(context, UserTokenPolicies_InitializationString);
            }

            if (RequestAccessToken != null)
            {
                RequestAccessToken.Initialize(context, RequestAccessToken_InitializationString);
            }
        }

        #region Initialization String
        private const string UserTokenPolicies_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8XYIkKAgAAAAEAEQAAAFVz" +
           "ZXJUb2tlblBvbGljaWVzAQHHAwAuAETHAwAAAQAwAQEAAAABAAAAAAAAAAEB/////wAAAAA=";

        private const string RequestAccessToken_InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYYIKBAAAAAEAEgAAAFJl" +
           "cXVlc3RBY2Nlc3NUb2tlbgEByQMALwEByQPJAwAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRB" +
           "cmd1bWVudHMBAcoDAC4ARMoDAACWAgAAAAEAKgEBHgAAAA0AAABJZGVudGl0eVRva2VuAQA8Af////8A" +
           "AAAAAAEAKgEBGQAAAAoAAABSZXNvdXJjZUlkAAz/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////" +
           "AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQHLAwAuAETLAwAAlgEAAAABACoBARoAAAAL" +
           "AAAAQWNjZXNzVG9rZW4ADP////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAA";

        private const string InitializationString =
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYIACAQAAAAEAIAAAAEF1" +
           "dGhvcml6YXRpb25TZXJ2aWNlVHlwZUluc3RhbmNlAQHGAwEBxgPGAwAA/////wUAAAAVYIkKAgAAAAEA" +
           "CgAAAFNlcnZpY2VVcmkBAesDAC4AROsDAAAADP////8BAf////8AAAAAFWCJCgIAAAABABIAAABTZXJ2" +
           "aWNlQ2VydGlmaWNhdGUBAcgDAC4ARMgDAAAAD/////8BAf////8AAAAAF2CJCgIAAAABABEAAABVc2Vy" +
           "VG9rZW5Qb2xpY2llcwEBxwMALgBExwMAAAEAMAEBAAAAAQAAAAAAAAABAf////8AAAAABGGCCgQAAAAB" +
           "ABUAAABHZXRTZXJ2aWNlRGVzY3JpcHRpb24BAewDAC8BAewD7AMAAAEB/////wEAAAAXYKkKAgAAAAAA" +
           "DwAAAE91dHB1dEFyZ3VtZW50cwEB7QMALgBE7QMAAJYDAAAAAQAqAQEZAAAACgAAAFNlcnZpY2VVcmkA" +
           "DP////8AAAAAAAEAKgEBIQAAABIAAABTZXJ2aWNlQ2VydGlmaWNhdGUAD/////8AAAAAAAEAKgEBJgAA" +
           "ABEAAABVc2VyVG9rZW5Qb2xpY2llcwEAMAEBAAAAAQAAAAAAAAAAAQAoAQEAAAABAAAAAAAAAAEB////" +
           "/wAAAAAEYYIKBAAAAAEAEgAAAFJlcXVlc3RBY2Nlc3NUb2tlbgEByQMALwEByQPJAwAAAQH/////AgAA" +
           "ABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAcoDAC4ARMoDAACWAgAAAAEAKgEBHgAAAA0AAABJ" +
           "ZGVudGl0eVRva2VuAQA8Af////8AAAAAAAEAKgEBGQAAAAoAAABSZXNvdXJjZUlkAAz/////AAAAAAAB" +
           "ACgBAQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQHLAwAu" +
           "AETLAwAAlgEAAAABACoBARoAAAALAAAAQWNjZXNzVG9rZW4ADP////8AAAAAAAEAKAEBAAAAAQAAAAAA" +
           "AAABAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public PropertyState<string> ServiceUri
        {
            get
            {
                return m_serviceUri;
            }

            set
            {
                if (!Object.ReferenceEquals(m_serviceUri, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_serviceUri = value;
            }
        }

        /// <remarks />
        public PropertyState<byte[]> ServiceCertificate
        {
            get
            {
                return m_serviceCertificate;
            }

            set
            {
                if (!Object.ReferenceEquals(m_serviceCertificate, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_serviceCertificate = value;
            }
        }

        /// <remarks />
        public PropertyState<UserTokenPolicy[]> UserTokenPolicies
        {
            get
            {
                return m_userTokenPolicies;
            }

            set
            {
                if (!Object.ReferenceEquals(m_userTokenPolicies, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_userTokenPolicies = value;
            }
        }

        /// <remarks />
        public GetServiceDescriptionMethodState GetServiceDescription
        {
            get
            {
                return m_getServiceDescriptionMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_getServiceDescriptionMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_getServiceDescriptionMethod = value;
            }
        }

        /// <remarks />
        public RequestAccessTokenMethodState RequestAccessToken
        {
            get
            {
                return m_requestAccessTokenMethod;
            }

            set
            {
                if (!Object.ReferenceEquals(m_requestAccessTokenMethod, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_requestAccessTokenMethod = value;
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
            if (m_serviceUri != null)
            {
                children.Add(m_serviceUri);
            }

            if (m_serviceCertificate != null)
            {
                children.Add(m_serviceCertificate);
            }

            if (m_userTokenPolicies != null)
            {
                children.Add(m_userTokenPolicies);
            }

            if (m_getServiceDescriptionMethod != null)
            {
                children.Add(m_getServiceDescriptionMethod);
            }

            if (m_requestAccessTokenMethod != null)
            {
                children.Add(m_requestAccessTokenMethod);
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
                case Opc.Ua.Gds.BrowseNames.ServiceUri:
                {
                    if (createOrReplace)
                    {
                        if (ServiceUri == null)
                        {
                            if (replacement == null)
                            {
                                ServiceUri = new PropertyState<string>(this);
                            }
                            else
                            {
                                ServiceUri = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = ServiceUri;
                    break;
                }

                case Opc.Ua.Gds.BrowseNames.ServiceCertificate:
                {
                    if (createOrReplace)
                    {
                        if (ServiceCertificate == null)
                        {
                            if (replacement == null)
                            {
                                ServiceCertificate = new PropertyState<byte[]>(this);
                            }
                            else
                            {
                                ServiceCertificate = (PropertyState<byte[]>)replacement;
                            }
                        }
                    }

                    instance = ServiceCertificate;
                    break;
                }

                case Opc.Ua.Gds.BrowseNames.UserTokenPolicies:
                {
                    if (createOrReplace)
                    {
                        if (UserTokenPolicies == null)
                        {
                            if (replacement == null)
                            {
                                UserTokenPolicies = new PropertyState<UserTokenPolicy[]>(this);
                            }
                            else
                            {
                                UserTokenPolicies = (PropertyState<UserTokenPolicy[]>)replacement;
                            }
                        }
                    }

                    instance = UserTokenPolicies;
                    break;
                }

                case Opc.Ua.Gds.BrowseNames.GetServiceDescription:
                {
                    if (createOrReplace)
                    {
                        if (GetServiceDescription == null)
                        {
                            if (replacement == null)
                            {
                                GetServiceDescription = new GetServiceDescriptionMethodState(this);
                            }
                            else
                            {
                                GetServiceDescription = (GetServiceDescriptionMethodState)replacement;
                            }
                        }
                    }

                    instance = GetServiceDescription;
                    break;
                }

                case Opc.Ua.Gds.BrowseNames.RequestAccessToken:
                {
                    if (createOrReplace)
                    {
                        if (RequestAccessToken == null)
                        {
                            if (replacement == null)
                            {
                                RequestAccessToken = new RequestAccessTokenMethodState(this);
                            }
                            else
                            {
                                RequestAccessToken = (RequestAccessTokenMethodState)replacement;
                            }
                        }
                    }

                    instance = RequestAccessToken;
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
        private PropertyState<string> m_serviceUri;
        private PropertyState<byte[]> m_serviceCertificate;
        private PropertyState<UserTokenPolicy[]> m_userTokenPolicies;
        private GetServiceDescriptionMethodState m_getServiceDescriptionMethod;
        private RequestAccessTokenMethodState m_requestAccessTokenMethod;
        #endregion
    }
    #endif
    #endregion

    #region GetServiceDescriptionMethodState Class
    #if (!OPCUA_EXCLUDE_GetServiceDescriptionMethodState)
    /// <summary>
    /// Stores an instance of the GetServiceDescriptionMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class GetServiceDescriptionMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public GetServiceDescriptionMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new GetServiceDescriptionMethodState(parent);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYYIKBAAAAAEAHwAAAEdl" +
           "dFNlcnZpY2VEZXNjcmlwdGlvbk1ldGhvZFR5cGUBAe4DAC8BAe4D7gMAAAEB/////wEAAAAXYKkKAgAA" +
           "AAAADwAAAE91dHB1dEFyZ3VtZW50cwEB7wMALgBE7wMAAJYDAAAAAQAqAQEZAAAACgAAAFNlcnZpY2VV" +
           "cmkADP////8AAAAAAAEAKgEBIQAAABIAAABTZXJ2aWNlQ2VydGlmaWNhdGUAD/////8AAAAAAAEAKgEB" +
           "JgAAABEAAABVc2VyVG9rZW5Qb2xpY2llcwEAMAEBAAAAAQAAAAAAAAAAAQAoAQEAAAABAAAAAAAAAAEB" +
           "/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public GetServiceDescriptionMethodStateMethodCallHandler OnCall;
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

            string serviceUri = (string)_outputArguments[0];
            byte[] serviceCertificate = (byte[])_outputArguments[1];
            UserTokenPolicy[] userTokenPolicies = (UserTokenPolicy[])_outputArguments[2];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    ref serviceUri,
                    ref serviceCertificate,
                    ref userTokenPolicies);
            }

            _outputArguments[0] = serviceUri;
            _outputArguments[1] = serviceCertificate;
            _outputArguments[2] = userTokenPolicies;

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
    public delegate ServiceResult GetServiceDescriptionMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        ref string serviceUri,
        ref byte[] serviceCertificate,
        ref UserTokenPolicy[] userTokenPolicies);
    #endif
    #endregion

    #region RequestAccessTokenMethodState Class
    #if (!OPCUA_EXCLUDE_RequestAccessTokenMethodState)
    /// <summary>
    /// Stores an instance of the RequestAccessTokenMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class RequestAccessTokenMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public RequestAccessTokenMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new RequestAccessTokenMethodState(parent);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYYIKBAAAAAEAHAAAAFJl" +
           "cXVlc3RBY2Nlc3NUb2tlbk1ldGhvZFR5cGUBAeMDAC8BAeMD4wMAAAEB/////wIAAAAXYKkKAgAAAAAA" +
           "DgAAAElucHV0QXJndW1lbnRzAQHkAwAuAETkAwAAlgIAAAABACoBAR4AAAANAAAASWRlbnRpdHlUb2tl" +
           "bgEAPAH/////AAAAAAABACoBARkAAAAKAAAAUmVzb3VyY2VJZAAM/////wAAAAAAAQAoAQEAAAABAAAA" +
           "AAAAAAEB/////wAAAAAXYKkKAgAAAAAADwAAAE91dHB1dEFyZ3VtZW50cwEB5QMALgBE5QMAAJYBAAAA" +
           "AQAqAQEaAAAACwAAAEFjY2Vzc1Rva2VuAAz/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAA" +
           "AA==";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public RequestAccessTokenMethodStateMethodCallHandler OnCall;
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

            UserIdentityToken identityToken = (UserIdentityToken)ExtensionObject.ToEncodeable((ExtensionObject)_inputArguments[0]);
            string resourceId = (string)_inputArguments[1];

            string accessToken = (string)_outputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    identityToken,
                    resourceId,
                    ref accessToken);
            }

            _outputArguments[0] = accessToken;

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
    public delegate ServiceResult RequestAccessTokenMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        UserIdentityToken identityToken,
        string resourceId,
        ref string accessToken);
    #endif
    #endregion

    #region AccessTokenAuditIssuedAuditEventState Class
    #if (!OPCUA_EXCLUDE_AccessTokenAuditIssuedAuditEventState)
    /// <summary>
    /// Stores an instance of the AccessTokenAuditIssuedAuditEventType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class AccessTokenAuditIssuedAuditEventState : AuditUpdateMethodEventState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public AccessTokenAuditIssuedAuditEventState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Gds.ObjectTypes.AccessTokenAuditIssuedAuditEventType, Opc.Ua.Gds.Namespaces.OpcUaGds, namespaceUris);
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
           "AQAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvR0RTL/////8EYIACAQAAAAEALAAAAEFj" +
           "Y2Vzc1Rva2VuQXVkaXRJc3N1ZWRBdWRpdEV2ZW50VHlwZUluc3RhbmNlAQHPAwEBzwPPAwAA/////w8A" +
           "AAAVYIkKAgAAAAAABwAAAEV2ZW50SWQBAdADAC4ARNADAAAAD/////8BAf////8AAAAAFWCJCgIAAAAA" +
           "AAkAAABFdmVudFR5cGUBAdEDAC4ARNEDAAAAEf////8BAf////8AAAAAFWCJCgIAAAAAAAoAAABTb3Vy" +
           "Y2VOb2RlAQHSAwAuAETSAwAAABH/////AQH/////AAAAABVgiQoCAAAAAAAKAAAAU291cmNlTmFtZQEB" +
           "0wMALgBE0wMAAAAM/////wEB/////wAAAAAVYIkKAgAAAAAABAAAAFRpbWUBAdQDAC4ARNQDAAABACYB" +
           "/////wEB/////wAAAAAVYIkKAgAAAAAACwAAAFJlY2VpdmVUaW1lAQHVAwAuAETVAwAAAQAmAf////8B" +
           "Af////8AAAAAFWCJCgIAAAAAAAcAAABNZXNzYWdlAQHXAwAuAETXAwAAABX/////AQH/////AAAAABVg" +
           "iQoCAAAAAAAIAAAAU2V2ZXJpdHkBAdgDAC4ARNgDAAAABf////8BAf////8AAAAAFWCJCgIAAAAAAA8A" +
           "AABBY3Rpb25UaW1lU3RhbXABAdkDAC4ARNkDAAABACYB/////wEB/////wAAAAAVYIkKAgAAAAAABgAA" +
           "AFN0YXR1cwEB2gMALgBE2gMAAAAB/////wEB/////wAAAAAVYIkKAgAAAAAACAAAAFNlcnZlcklkAQHb" +
           "AwAuAETbAwAAAAz/////AQH/////AAAAABVgiQoCAAAAAAASAAAAQ2xpZW50QXVkaXRFbnRyeUlkAQHc" +
           "AwAuAETcAwAAAAz/////AQH/////AAAAABVgiQoCAAAAAAAMAAAAQ2xpZW50VXNlcklkAQHdAwAuAETd" +
           "AwAAAAz/////AQH/////AAAAABVgiQoCAAAAAAAIAAAATWV0aG9kSWQBAd4DAC4ARN4DAAAAEf////8B" +
           "Af////8AAAAAF2CJCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEB3wMALgBE3wMAAAAYAQAAAAEAAAAA" +
           "AAAAAQH/////AAAAAA==";
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
