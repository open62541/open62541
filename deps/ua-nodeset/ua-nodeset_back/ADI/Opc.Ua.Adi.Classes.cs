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

namespace Opc.Ua.Adi
{
    #region AnalyserDeviceState Class
    #if (!OPCUA_EXCLUDE_AnalyserDeviceState)
    /// <summary>
    /// Stores an instance of the AnalyserDeviceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class AnalyserDeviceState : DeviceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public AnalyserDeviceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.ObjectTypes.AnalyserDeviceType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////4RggAIBAAAAAQAaAAAAQW5hbHlzZXJEZXZpY2VUeXBlSW5zdGFuY2UB" +
           "AekDAQHpA+kDAAAB/////w8AAAAkYIAKAQAAAAIADAAAAFBhcmFtZXRlclNldAEBiRMDAAAAABcAAABG" +
           "bGF0IGxpc3Qgb2YgUGFyYW1ldGVycwAvADqJEwAA/////wIAAAA1YIkKAgAAAAEAEAAAAERpYWdub3N0" +
           "aWNTdGF0dXMBAfMkAwAAAAAlAAAAR2VuZXJhbCBoZWFsdGggc3RhdHVzIG9mIHRoZSBhbmFseXNlcgAv" +
           "AQA9CfMkAAABAmQY/////wEBAQAAAAAjAQEBDCUAAAAAJGCACgEAAAABAAoAAABDb25maWdEYXRhAQH2" +
           "JAMAAAAALAAAAE9wdGlvbmFsIGFuYWx5c2VyIGRldmljZSBsYXJnZSBjb25maWd1cmF0aW9uAC8BADct" +
           "9iQAAAEAAAAAIwEBAQolCgAAABVgiQoCAAAAAAAEAAAAU2l6ZQEB9yQALgBE9yQAAAAJ/////wEB////" +
           "/wAAAAAVYIkKAgAAAAAACAAAAFdyaXRhYmxlAQEOMwAuAEQOMwAAAAH/////AQH/////AAAAABVgiQoC" +
           "AAAAAAAMAAAAVXNlcldyaXRhYmxlAQEPMwAuAEQPMwAAAAH/////AQH/////AAAAABVgiQoCAAAAAAAJ" +
           "AAAAT3BlbkNvdW50AQH6JAAuAET6JAAAAAX/////AQH/////AAAAAARhggoEAAAAAAAEAAAAT3BlbgEB" +
           "+yQALwEAPC37JAAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAfwkAC4ARPwk" +
           "AACWAQAAAAEAKgEBEwAAAAQAAABNb2RlAAP/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAA" +
           "ABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQH9JAAuAET9JAAAlgEAAAABACoBARkAAAAKAAAA" +
           "RmlsZUhhbmRsZQAH/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAEYYIKBAAAAAAABQAA" +
           "AENsb3NlAQH+JAAvAQA/Lf4kAAABAf////8BAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEB" +
           "/yQALgBE/yQAAJYBAAAAAQAqAQEZAAAACgAAAEZpbGVIYW5kbGUAB/////8AAAAAAAEAKAEBAAAAAQAA" +
           "AAAAAAABAf////8AAAAABGGCCgQAAAAAAAQAAABSZWFkAQEAJQAvAQBBLQAlAAABAf////8CAAAAF2Cp" +
           "CgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBASUALgBEASUAAJYCAAAAAQAqAQEZAAAACgAAAEZpbGVI" +
           "YW5kbGUAB/////8AAAAAAAEAKgEBFQAAAAYAAABMZW5ndGgABv////8AAAAAAAEAKAEBAAAAAQAAAAAA" +
           "AAABAf////8AAAAAF2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1bWVudHMBAQIlAC4ARAIlAACWAQAAAAEA" +
           "KgEBEwAAAAQAAABEYXRhAA//////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAAARhggoEAAAA" +
           "AAAFAAAAV3JpdGUBAQMlAC8BAEQtAyUAAAEB/////wEAAAAXYKkKAgAAAAAADgAAAElucHV0QXJndW1l" +
           "bnRzAQEEJQAuAEQEJQAAlgIAAAABACoBARkAAAAKAAAARmlsZUhhbmRsZQAH/////wAAAAAAAQAqAQET" +
           "AAAABAAAAERhdGEAD/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAABGGCCgQAAAAAAAsA" +
           "AABHZXRQb3NpdGlvbgEBBSUALwEARi0FJQAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1" +
           "bWVudHMBAQYlAC4ARAYlAACWAQAAAAEAKgEBGQAAAAoAAABGaWxlSGFuZGxlAAf/////AAAAAAABACgB" +
           "AQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQEHJQAuAEQH" +
           "JQAAlgEAAAABACoBARcAAAAIAAAAUG9zaXRpb24ACf////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf//" +
           "//8AAAAABGGCCgQAAAAAAAsAAABTZXRQb3NpdGlvbgEBCCUALwEASS0IJQAAAQH/////AQAAABdgqQoC" +
           "AAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAQklAC4ARAklAACWAgAAAAEAKgEBGQAAAAoAAABGaWxlSGFu" +
           "ZGxlAAf/////AAAAAAABACoBARcAAAAIAAAAUG9zaXRpb24ACf////8AAAAAAAEAKAEBAAAAAQAAAAAA" +
           "AAABAf////8AAAAAJGCACgEAAAACAAkAAABNZXRob2RTZXQBAaYkAwAAAAAUAAAARmxhdCBsaXN0IG9m" +
           "IE1ldGhvZHMALwA6piQAAP////8KAAAABGGCCgQAAAABABAAAABHZXRDb25maWd1cmF0aW9uAQHjJAAv" +
           "AQHjJOMkAAABAf////8BAAAAF2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1bWVudHMBAeQkAC4AROQkAACW" +
           "AQAAAAEAKgEBGQAAAAoAAABDb25maWdEYXRhAA//////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////" +
           "AAAAAARhggoEAAAAAQAQAAAAU2V0Q29uZmlndXJhdGlvbgEB5SQALwEB5STlJAAAAQH/////AgAAABdg" +
           "qQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAeYkAC4AROYkAACWAQAAAAEAKgEBGQAAAAoAAABDb25m" +
           "aWdEYXRhAA//////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0" +
           "cHV0QXJndW1lbnRzAQHnJAAuAETnJAAAlgEAAAABACoBAR8AAAAQAAAAQ29uZmlnRGF0YURpZ2VzdAAM" +
           "/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAEYYIKBAAAAAEAEwAAAEdldENvbmZpZ0Rh" +
           "dGFEaWdlc3QBAegkAC8BAegk6CQAAAEB/////wEAAAAXYKkKAgAAAAAADwAAAE91dHB1dEFyZ3VtZW50" +
           "cwEB6SQALgBE6SQAAJYBAAAAAQAqAQEfAAAAEAAAAENvbmZpZ0RhdGFEaWdlc3QADP////8AAAAAAAEA" +
           "KAEBAAAAAQAAAAAAAAABAf////8AAAAABGGCCgQAAAABABcAAABDb21wYXJlQ29uZmlnRGF0YURpZ2Vz" +
           "dAEB6iQALwEB6iTqJAAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAeskAC4A" +
           "ROskAACWAQAAAAEAKgEBHwAAABAAAABDb25maWdEYXRhRGlnZXN0AAz/////AAAAAAABACgBAQAAAAEA" +
           "AAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQHsJAAuAETsJAAAlgEA" +
           "AAABACoBARYAAAAHAAAASXNFcXVhbAAB/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAk" +
           "YYIKBAAAAAEAEAAAAFJlc2V0QWxsQ2hhbm5lbHMBAe0kAwAAAAA8AAAAUmVzZXQgYWxsIEFuYWx5c2Vy" +
           "Q2hhbm5lbHMgYmVsb25naW5nIHRvIHRoaXMgQW5hbHlzZXJEZXZpY2UuAC8BAe0k7SQAAAEB/////wAA" +
           "AAAkYYIKBAAAAAEAEAAAAFN0YXJ0QWxsQ2hhbm5lbHMBAe4kAwAAAAA8AAAAU3RhcnQgYWxsIEFuYWx5" +
           "c2VyQ2hhbm5lbHMgYmVsb25naW5nIHRvIHRoaXMgQW5hbHlzZXJEZXZpY2UuAC8BAe4k7iQAAAEB////" +
           "/wAAAAAkYYIKBAAAAAEADwAAAFN0b3BBbGxDaGFubmVscwEB7yQDAAAAADsAAABTdG9wIGFsbCBBbmFs" +
           "eXNlckNoYW5uZWxzIGJlbG9uZ2luZyB0byB0aGlzIEFuYWx5c2VyRGV2aWNlLgAvAQHvJO8kAAABAf//" +
           "//8AAAAAJGGCCgQAAAABABAAAABBYm9ydEFsbENoYW5uZWxzAQHwJAMAAAAAPAAAAEFib3J0IGFsbCBB" +
           "bmFseXNlckNoYW5uZWxzIGJlbG9uZ2luZyB0byB0aGlzIEFuYWx5c2VyRGV2aWNlLgAvAQHwJPAkAAAB" +
           "Af////8AAAAAJGGCCgQAAAABAA0AAABHb3RvT3BlcmF0aW5nAQHxJAMAAAAAjQAAAEFuYWx5c2VyRGV2" +
           "aWNlU3RhdGVNYWNoaW5lIHRvIGdvIHRvIE9wZXJhdGluZyBzdGF0ZSwgZm9yY2luZyBhbGwgQW5hbHlz" +
           "ZXJDaGFubmVscyB0byBsZWF2ZSB0aGUgU2xhdmVNb2RlIHN0YXRlIGFuZCBnbyB0byB0aGUgT3BlcmF0" +
           "aW5nIHN0YXRlLgAvAQHxJPEkAAABAf////8AAAAAJGGCCgQAAAABAA8AAABHb3RvTWFpbnRlbmFuY2UB" +
           "AfIkAwAAAABnAAAAQW5hbHlzZXJEZXZpY2VTdGF0ZU1hY2hpbmUgdG8gZ28gdG8gTWFpbnRlbmFuY2Ug" +
           "c3RhdGUsIGZvcmNpbmcgYWxsIEFuYWx5c2VyQ2hhbm5lbHMgdG8gU2xhdmVNb2RlIHN0YXRlLgAvAQHy" +
           "JPIkAAABAf////8AAAAAJGCACgEAAAACAA4AAABJZGVudGlmaWNhdGlvbgEBqiQDAAAAAEYAAABVc2Vk" +
           "IHRvIG9yZ2FuaXplIHBhcmFtZXRlcnMgZm9yIGlkZW50aWZpY2F0aW9uIG9mIHRoaXMgVG9wb2xvZ3lF" +
           "bGVtZW50AC8BAu0DqiQAAAMAAAAAIwABAnMXACMAAQJ0FwAjAAECcRcAAAAANWCJCgIAAAACAAwAAABN" +
           "YW51ZmFjdHVyZXIBAbwkAwAAAAAwAAAATmFtZSBvZiB0aGUgY29tcGFueSB0aGF0IG1hbnVmYWN0dXJl" +
           "ZCB0aGUgZGV2aWNlAC4ARLwkAAAAFf////8BAf////8AAAAANWCJCgIAAAACAAUAAABNb2RlbAEBvSQD" +
           "AAAAABgAAABNb2RlbCBuYW1lIG9mIHRoZSBkZXZpY2UALgBEvSQAAAAV/////wEB/////wAAAAA1YIkK" +
           "AgAAAAIAEAAAAEhhcmR3YXJlUmV2aXNpb24BAcEkAwAAAAAsAAAAUmV2aXNpb24gbGV2ZWwgb2YgdGhl" +
           "IGhhcmR3YXJlIG9mIHRoZSBkZXZpY2UALgBEwSQAAAAM/////wEB/////wAAAAA1YIkKAgAAAAIAEAAA" +
           "AFNvZnR3YXJlUmV2aXNpb24BAcAkAwAAAAA1AAAAUmV2aXNpb24gbGV2ZWwgb2YgdGhlIHNvZnR3YXJl" +
           "L2Zpcm13YXJlIG9mIHRoZSBkZXZpY2UALgBEwCQAAAAM/////wEB/////wAAAAA1YIkKAgAAAAIADgAA" +
           "AERldmljZVJldmlzaW9uAQG/JAMAAAAAJAAAAE92ZXJhbGwgcmV2aXNpb24gbGV2ZWwgb2YgdGhlIGRl" +
           "dmljZQAuAES/JAAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAMAAAARGV2aWNlTWFudWFsAQG+JAMA" +
           "AAAAWgAAAEFkZHJlc3MgKHBhdGhuYW1lIGluIHRoZSBmaWxlIHN5c3RlbSBvciBhIFVSTCB8IFdlYiBh" +
           "ZGRyZXNzKSBvZiB1c2VyIG1hbnVhbCBmb3IgdGhlIGRldmljZQAuAES+JAAAAAz/////AQH/////AAAA" +
           "ADVgiQoCAAAAAgAMAAAAU2VyaWFsTnVtYmVyAQG6JAMAAAAATQAAAElkZW50aWZpZXIgdGhhdCB1bmlx" +
           "dWVseSBpZGVudGlmaWVzLCB3aXRoaW4gYSBtYW51ZmFjdHVyZXIsIGEgZGV2aWNlIGluc3RhbmNlAC4A" +
           "RLokAAAADP////8BAf////8AAAAANWCJCgIAAAACAA8AAABSZXZpc2lvbkNvdW50ZXIBAbskAwAAAABp" +
           "AAAAQW4gaW5jcmVtZW50YWwgY291bnRlciBpbmRpY2F0aW5nIHRoZSBudW1iZXIgb2YgdGltZXMgdGhl" +
           "IHN0YXRpYyBkYXRhIHdpdGhpbiB0aGUgRGV2aWNlIGhhcyBiZWVuIG1vZGlmaWVkAC4ARLskAAAABv//" +
           "//8BAf////8AAAAABGCACgEAAAABAA0AAABDb25maWd1cmF0aW9uAQEKJQAvAQLtAwolAAABAAAAACMA" +
           "AQH2JAAAAAAEYIAKAQAAAAEABgAAAFN0YXR1cwEBDCUALwEC7QMMJQAAAQAAAAAjAAEB8yQAAAAABGCA" +
           "CgEAAAABAA8AAABGYWN0b3J5U2V0dGluZ3MBAQ4lAC8BAu0DDiUAAP////8AAAAABGCACgEAAAABABQA" +
           "AABBbmFseXNlclN0YXRlTWFjaGluZQEBECUALwEB6gMQJQAA/////wEAAAAVYIkKAgAAAAAADAAAAEN1" +
           "cnJlbnRTdGF0ZQEBESUALwEAyAoRJQAAABX/////AQH/////AQAAABVgiQoCAAAAAAACAAAASWQBARIl" +
           "AC4ARBIlAAAAEf////8BAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public FunctionalGroupState Configuration
        {
            get
            {
                return m_configuration;
            }

            set
            {
                if (!Object.ReferenceEquals(m_configuration, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_configuration = value;
            }
        }

        /// <remarks />
        public FunctionalGroupState Status
        {
            get
            {
                return m_status;
            }

            set
            {
                if (!Object.ReferenceEquals(m_status, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_status = value;
            }
        }

        /// <remarks />
        public FunctionalGroupState FactorySettings
        {
            get
            {
                return m_factorySettings;
            }

            set
            {
                if (!Object.ReferenceEquals(m_factorySettings, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_factorySettings = value;
            }
        }

        /// <remarks />
        public AnalyserDeviceStateMachineState AnalyserStateMachine
        {
            get
            {
                return m_analyserStateMachine;
            }

            set
            {
                if (!Object.ReferenceEquals(m_analyserStateMachine, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_analyserStateMachine = value;
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
            if (m_configuration != null)
            {
                children.Add(m_configuration);
            }

            if (m_status != null)
            {
                children.Add(m_status);
            }

            if (m_factorySettings != null)
            {
                children.Add(m_factorySettings);
            }

            if (m_analyserStateMachine != null)
            {
                children.Add(m_analyserStateMachine);
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
                case Opc.Ua.Adi.BrowseNames.Configuration:
                {
                    if (createOrReplace)
                    {
                        if (Configuration == null)
                        {
                            if (replacement == null)
                            {
                                Configuration = new FunctionalGroupState(this);
                            }
                            else
                            {
                                Configuration = (FunctionalGroupState)replacement;
                            }
                        }
                    }

                    instance = Configuration;
                    break;
                }

                case Opc.Ua.Adi.BrowseNames.Status:
                {
                    if (createOrReplace)
                    {
                        if (Status == null)
                        {
                            if (replacement == null)
                            {
                                Status = new FunctionalGroupState(this);
                            }
                            else
                            {
                                Status = (FunctionalGroupState)replacement;
                            }
                        }
                    }

                    instance = Status;
                    break;
                }

                case Opc.Ua.Adi.BrowseNames.FactorySettings:
                {
                    if (createOrReplace)
                    {
                        if (FactorySettings == null)
                        {
                            if (replacement == null)
                            {
                                FactorySettings = new FunctionalGroupState(this);
                            }
                            else
                            {
                                FactorySettings = (FunctionalGroupState)replacement;
                            }
                        }
                    }

                    instance = FactorySettings;
                    break;
                }

                case Opc.Ua.Adi.BrowseNames.AnalyserStateMachine:
                {
                    if (createOrReplace)
                    {
                        if (AnalyserStateMachine == null)
                        {
                            if (replacement == null)
                            {
                                AnalyserStateMachine = new AnalyserDeviceStateMachineState(this);
                            }
                            else
                            {
                                AnalyserStateMachine = (AnalyserDeviceStateMachineState)replacement;
                            }
                        }
                    }

                    instance = AnalyserStateMachine;
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
        private FunctionalGroupState m_configuration;
        private FunctionalGroupState m_status;
        private FunctionalGroupState m_factorySettings;
        private AnalyserDeviceStateMachineState m_analyserStateMachine;
        #endregion
    }
    #endif
    #endregion

    #region AnalyserDeviceTypeGetConfigurationMethodState Class
    #if (!OPCUA_EXCLUDE_AnalyserDeviceTypeGetConfigurationMethodState)
    /// <summary>
    /// Stores an instance of the AnalyserDeviceTypeGetConfigurationMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class AnalyserDeviceTypeGetConfigurationMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public AnalyserDeviceTypeGetConfigurationMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new AnalyserDeviceTypeGetConfigurationMethodState(parent);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAsAAAAQW5hbHlzZXJEZXZpY2VUeXBlR2V0Q29uZmln" +
           "dXJhdGlvbk1ldGhvZFR5cGUBAZolAC8BAZolmiUAAAEB/////wEAAAAXYKkKAgAAAAAADwAAAE91dHB1" +
           "dEFyZ3VtZW50cwEBmyUALgBEmyUAAJYBAAAAAQAqAQEZAAAACgAAAENvbmZpZ0RhdGEAD/////8AAAAA" +
           "AAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public AnalyserDeviceTypeGetConfigurationMethodStateMethodCallHandler OnCall;
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

            byte[] configData = (byte[])_outputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    ref configData);
            }

            _outputArguments[0] = configData;

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
    public delegate ServiceResult AnalyserDeviceTypeGetConfigurationMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        ref byte[] configData);
    #endif
    #endregion

    #region AnalyserDeviceTypeSetConfigurationMethodState Class
    #if (!OPCUA_EXCLUDE_AnalyserDeviceTypeSetConfigurationMethodState)
    /// <summary>
    /// Stores an instance of the AnalyserDeviceTypeSetConfigurationMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class AnalyserDeviceTypeSetConfigurationMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public AnalyserDeviceTypeSetConfigurationMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new AnalyserDeviceTypeSetConfigurationMethodState(parent);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAsAAAAQW5hbHlzZXJEZXZpY2VUeXBlU2V0Q29uZmln" +
           "dXJhdGlvbk1ldGhvZFR5cGUBAZwlAC8BAZwlnCUAAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElucHV0" +
           "QXJndW1lbnRzAQGdJQAuAESdJQAAlgEAAAABACoBARkAAAAKAAAAQ29uZmlnRGF0YQAP/////wAAAAAA" +
           "AQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAXYKkKAgAAAAAADwAAAE91dHB1dEFyZ3VtZW50cwEBniUA" +
           "LgBEniUAAJYBAAAAAQAqAQEfAAAAEAAAAENvbmZpZ0RhdGFEaWdlc3QADP////8AAAAAAAEAKAEBAAAA" +
           "AQAAAAAAAAABAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public AnalyserDeviceTypeSetConfigurationMethodStateMethodCallHandler OnCall;
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

            byte[] configData = (byte[])_inputArguments[0];

            string configDataDigest = (string)_outputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    configData,
                    ref configDataDigest);
            }

            _outputArguments[0] = configDataDigest;

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
    public delegate ServiceResult AnalyserDeviceTypeSetConfigurationMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        byte[] configData,
        ref string configDataDigest);
    #endif
    #endregion

    #region AnalyserDeviceTypeGetConfigDataDigestMethodState Class
    #if (!OPCUA_EXCLUDE_AnalyserDeviceTypeGetConfigDataDigestMethodState)
    /// <summary>
    /// Stores an instance of the AnalyserDeviceTypeGetConfigDataDigestMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class AnalyserDeviceTypeGetConfigDataDigestMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public AnalyserDeviceTypeGetConfigDataDigestMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new AnalyserDeviceTypeGetConfigDataDigestMethodState(parent);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAvAAAAQW5hbHlzZXJEZXZpY2VUeXBlR2V0Q29uZmln" +
           "RGF0YURpZ2VzdE1ldGhvZFR5cGUBAZ8lAC8BAZ8lnyUAAAEB/////wEAAAAXYKkKAgAAAAAADwAAAE91" +
           "dHB1dEFyZ3VtZW50cwEBoCUALgBEoCUAAJYBAAAAAQAqAQEfAAAAEAAAAENvbmZpZ0RhdGFEaWdlc3QA" +
           "DP////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public AnalyserDeviceTypeGetConfigDataDigestMethodStateMethodCallHandler OnCall;
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

            string configDataDigest = (string)_outputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    ref configDataDigest);
            }

            _outputArguments[0] = configDataDigest;

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
    public delegate ServiceResult AnalyserDeviceTypeGetConfigDataDigestMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        ref string configDataDigest);
    #endif
    #endregion

    #region AnalyserDeviceTypeCompareConfigDataDigestMethodState Class
    #if (!OPCUA_EXCLUDE_AnalyserDeviceTypeCompareConfigDataDigestMethodState)
    /// <summary>
    /// Stores an instance of the AnalyserDeviceTypeCompareConfigDataDigestMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class AnalyserDeviceTypeCompareConfigDataDigestMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public AnalyserDeviceTypeCompareConfigDataDigestMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new AnalyserDeviceTypeCompareConfigDataDigestMethodState(parent);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAzAAAAQW5hbHlzZXJEZXZpY2VUeXBlQ29tcGFyZUNv" +
           "bmZpZ0RhdGFEaWdlc3RNZXRob2RUeXBlAQGhJQAvAQGhJaElAAABAf////8CAAAAF2CpCgIAAAAAAA4A" +
           "AABJbnB1dEFyZ3VtZW50cwEBoiUALgBEoiUAAJYBAAAAAQAqAQEfAAAAEAAAAENvbmZpZ0RhdGFEaWdl" +
           "c3QADP////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIAAAAAAA8AAABPdXRwdXRB" +
           "cmd1bWVudHMBAaMlAC4ARKMlAACWAQAAAAEAKgEBFgAAAAcAAABJc0VxdWFsAAH/////AAAAAAABACgB" +
           "AQAAAAEAAAAAAAAAAQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public AnalyserDeviceTypeCompareConfigDataDigestMethodStateMethodCallHandler OnCall;
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

            string configDataDigest = (string)_inputArguments[0];

            bool isEqual = (bool)_outputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    configDataDigest,
                    ref isEqual);
            }

            _outputArguments[0] = isEqual;

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
    public delegate ServiceResult AnalyserDeviceTypeCompareConfigDataDigestMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        string configDataDigest,
        ref bool isEqual);
    #endif
    #endregion

    #region AnalyserDeviceStateMachineState Class
    #if (!OPCUA_EXCLUDE_AnalyserDeviceStateMachineState)
    /// <summary>
    /// Stores an instance of the AnalyserDeviceStateMachineType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class AnalyserDeviceStateMachineState : FiniteStateMachineState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public AnalyserDeviceStateMachineState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.ObjectTypes.AnalyserDeviceStateMachineType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////4RggAIBAAAAAQAmAAAAQW5hbHlzZXJEZXZpY2VTdGF0ZU1hY2hpbmVU" +
           "eXBlSW5zdGFuY2UBAeoDAQHqA+oDAAAB/////wEAAAAVYIkKAgAAAAAADAAAAEN1cnJlbnRTdGF0ZQEB" +
           "pCUALwEAyAqkJQAAABX/////AQH/////AQAAABVgiQoCAAAAAAACAAAASWQBAaUlAC4ARKUlAAAAEf//" +
           "//8BAf////8AAAAA";
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

    #region AnalyserChannelState Class
    #if (!OPCUA_EXCLUDE_AnalyserChannelState)
    /// <summary>
    /// Stores an instance of the AnalyserChannelType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class AnalyserChannelState : TopologyElementState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public AnalyserChannelState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.ObjectTypes.AnalyserChannelType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////4RggAIBAAAAAQAbAAAAQW5hbHlzZXJDaGFubmVsVHlwZUluc3RhbmNl" +
           "AQHrAwEB6wPrAwAAAf////8FAAAAJGCACgEAAAACAAwAAABQYXJhbWV0ZXJTZXQBAc0lAwAAAAAXAAAA" +
           "RmxhdCBsaXN0IG9mIFBhcmFtZXRlcnMALwA6zSUAAP////8EAAAANWCJCgIAAAABAAkAAABDaGFubmVs" +
           "SWQBAfAlAwAAAAAaAAAAQ2hhbm5lbCBJZCBkZWZpbmVkIGJ5IHVzZXIALwEAPQnwJQAAAAz/////AQH/" +
           "////AAAAADVgiQoCAAAAAQAJAAAASXNFbmFibGVkAQHzJQMAAAAANQAAAFRydWUgaWYgdGhlIGNoYW5u" +
           "ZWwgaXMgZW5hYmxlZCBhbmQgYWNjZXB0aW5nIGNvbW1hbmRzAC8BAD0J8yUAAAAB/////wEBAQAAAAAj" +
           "AQEB/CUAAAAANWCJCgIAAAABABAAAABEaWFnbm9zdGljU3RhdHVzAQH2JQMAAAAAHQAAAEFuYWx5c2Vy" +
           "Q2hhbm5lbCBoZWFsdGggc3RhdHVzAC8BAD0J9iUAAAECZBj/////AQEBAAAAACMBAQH+JQAAAAA1YIkK" +
           "AgAAAAEADAAAAEFjdGl2ZVN0cmVhbQEB+SUDAAAAACYAAABBY3RpdmUgc3RyZWFtIGZvciB0aGlzIEFu" +
           "YWx5c2VyQ2hhbm5lbAAvAQA9CfklAAAADP////8BAQEAAAAAIwEBAf4lAAAAACRggAoBAAAAAgAJAAAA" +
           "TWV0aG9kU2V0AQHPJQMAAAAAFAAAAEZsYXQgbGlzdCBvZiBNZXRob2RzAC8AOs8lAAD/////DAAAACRh" +
           "ggoEAAAAAQANAAAAR290b09wZXJhdGluZwEB4yUDAAAAADIAAABUcmFuc2l0aW9ucyB0aGUgQW5hbHlz" +
           "ZXJDaGFubmVsIHRvIE9wZXJhdGluZyBtb2RlLgAvAQHjJeMlAAABAf////8AAAAAJGGCCgQAAAABAA8A" +
           "AABHb3RvTWFpbnRlbmFuY2UBAeQlAwAAAAA0AAAAVHJhbnNpdGlvbnMgdGhlIEFuYWx5c2VyQ2hhbm5l" +
           "bCB0byBNYWludGVuYW5jZSBtb2RlLgAvAQHkJeQlAAABAf////8AAAAABGGCCgQAAAABABYAAABTdGFy" +
           "dFNpbmdsZUFjcXVpc2l0aW9uAQHlJQAvAQHlJeUlAAABAf////8BAAAAF2CpCgIAAAAAAA4AAABJbnB1" +
           "dEFyZ3VtZW50cwEB5iUALgBE5iUAAJYDAAAAAQAqAQEfAAAADgAAAEV4ZWN1dGlvbkN5Y2xlAQGiJP//" +
           "//8AAAAAAAEAKgEBJAAAABUAAABFeGVjdXRpb25DeWNsZVN1YmNvZGUAB/////8AAAAAAAEAKgEBHQAA" +
           "AA4AAABTZWxlY3RlZFN0cmVhbQAM/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAkYYIK" +
           "BAAAAAEABQAAAFJlc2V0AQHnJQMAAAAAKQAAAENhdXNlcyB0cmFuc2l0aW9uIHRvIHRoZSBSZXNldHRp" +
           "bmcgc3RhdGUuAC8BAecl5yUAAAEB/////wAAAAAkYYIKBAAAAAEABQAAAFN0YXJ0AQHoJQMAAAAAKAAA" +
           "AENhdXNlcyB0cmFuc2l0aW9uIHRvIHRoZSBTdGFydGluZyBzdGF0ZS4ALwEB6CXoJQAAAQH/////AAAA" +
           "ACRhggoEAAAAAQAEAAAAU3RvcAEB6SUDAAAAACgAAABDYXVzZXMgdHJhbnNpdGlvbiB0byB0aGUgU3Rv" +
           "cHBpbmcgc3RhdGUuAC8BAekl6SUAAAEB/////wAAAAAkYYIKBAAAAAEABAAAAEhvbGQBAeolAwAAAAAn" +
           "AAAAQ2F1c2VzIHRyYW5zaXRpb24gdG8gdGhlIEhvbGRpbmcgc3RhdGUuAC8BAeol6iUAAAEB/////wAA" +
           "AAAkYYIKBAAAAAEABgAAAFVuaG9sZAEB6yUDAAAAACkAAABDYXVzZXMgdHJhbnNpdGlvbiB0byB0aGUg" +
           "VW5ob2xkaW5nIHN0YXRlLgAvAQHrJeslAAABAf////8AAAAAJGGCCgQAAAABAAcAAABTdXNwZW5kAQHs" +
           "JQMAAAAAKgAAAENhdXNlcyB0cmFuc2l0aW9uIHRvIHRoZSBTdXNwZW5kaW5nIHN0YXRlLgAvAQHsJewl" +
           "AAABAf////8AAAAAJGGCCgQAAAABAAkAAABVbnN1c3BlbmQBAe0lAwAAAAAsAAAAQ2F1c2VzIHRyYW5z" +
           "aXRpb24gdG8gdGhlIFVuc3VzcGVuZGluZyBzdGF0ZS4ALwEB7SXtJQAAAQH/////AAAAACRhggoEAAAA" +
           "AQAFAAAAQWJvcnQBAe4lAwAAAAAoAAAAQ2F1c2VzIHRyYW5zaXRpb24gdG8gdGhlIEFib3J0aW5nIHN0" +
           "YXRlLgAvAQHuJe4lAAABAf////8AAAAAJGGCCgQAAAABAAUAAABDbGVhcgEB7yUDAAAAACgAAABDYXVz" +
           "ZXMgdHJhbnNpdGlvbiB0byB0aGUgQ2xlYXJpbmcgc3RhdGUuAC8BAe8l7yUAAAEB/////wAAAAAEYIAK" +
           "AQAAAAEADQAAAENvbmZpZ3VyYXRpb24BAfwlAC8BAu0D/CUAAAEAAAAAIwABAfMlAAAAAARggAoBAAAA" +
           "AQAGAAAAU3RhdHVzAQH+JQAvAQLtA/4lAAACAAAAACMAAQH2JQAjAAEB+SUAAAAAhGCACgEAAAABABMA" +
           "AABDaGFubmVsU3RhdGVNYWNoaW5lAQEAJgAvAQHvAwAmAAAB/////wIAAAAVYIkKAgAAAAAADAAAAEN1" +
           "cnJlbnRTdGF0ZQEBASYALwEAyAoBJgAAABX/////AQH/////AQAAABVgiQoCAAAAAAACAAAASWQBAQIm" +
           "AC4ARAImAAAAEf////8BAf////8AAAAABGCACgEAAAABABgAAABPcGVyYXRpbmdTdWJTdGF0ZU1hY2hp" +
           "bmUBAQwmAC8BAfADDCYAAP////8CAAAAFWCJCgIAAAAAAAwAAABDdXJyZW50U3RhdGUBAQ0mAC8BAMgK" +
           "DSYAAAAV/////wEB/////wEAAAAVYIkKAgAAAAAAAgAAAElkAQEOJgAuAEQOJgAAABH/////AQH/////" +
           "AAAAAARggAoBAAAAAQAfAAAAT3BlcmF0aW5nRXhlY3V0ZVN1YlN0YXRlTWFjaGluZQEBGCYALwEB8QMY" +
           "JgAA/////wEAAAAVYIkKAgAAAAAADAAAAEN1cnJlbnRTdGF0ZQEBGSYALwEAyAoZJgAAABX/////AQH/" +
           "////AQAAABVgiQoCAAAAAAACAAAASWQBARomAC4ARBomAAAAEf////8BAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public FunctionalGroupState Configuration
        {
            get
            {
                return m_configuration;
            }

            set
            {
                if (!Object.ReferenceEquals(m_configuration, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_configuration = value;
            }
        }

        /// <remarks />
        public FunctionalGroupState Status
        {
            get
            {
                return m_status;
            }

            set
            {
                if (!Object.ReferenceEquals(m_status, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_status = value;
            }
        }

        /// <remarks />
        public AnalyserChannelStateMachineState ChannelStateMachine
        {
            get
            {
                return m_channelStateMachine;
            }

            set
            {
                if (!Object.ReferenceEquals(m_channelStateMachine, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_channelStateMachine = value;
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
            if (m_configuration != null)
            {
                children.Add(m_configuration);
            }

            if (m_status != null)
            {
                children.Add(m_status);
            }

            if (m_channelStateMachine != null)
            {
                children.Add(m_channelStateMachine);
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
                case Opc.Ua.Adi.BrowseNames.Configuration:
                {
                    if (createOrReplace)
                    {
                        if (Configuration == null)
                        {
                            if (replacement == null)
                            {
                                Configuration = new FunctionalGroupState(this);
                            }
                            else
                            {
                                Configuration = (FunctionalGroupState)replacement;
                            }
                        }
                    }

                    instance = Configuration;
                    break;
                }

                case Opc.Ua.Adi.BrowseNames.Status:
                {
                    if (createOrReplace)
                    {
                        if (Status == null)
                        {
                            if (replacement == null)
                            {
                                Status = new FunctionalGroupState(this);
                            }
                            else
                            {
                                Status = (FunctionalGroupState)replacement;
                            }
                        }
                    }

                    instance = Status;
                    break;
                }

                case Opc.Ua.Adi.BrowseNames.ChannelStateMachine:
                {
                    if (createOrReplace)
                    {
                        if (ChannelStateMachine == null)
                        {
                            if (replacement == null)
                            {
                                ChannelStateMachine = new AnalyserChannelStateMachineState(this);
                            }
                            else
                            {
                                ChannelStateMachine = (AnalyserChannelStateMachineState)replacement;
                            }
                        }
                    }

                    instance = ChannelStateMachine;
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
        private FunctionalGroupState m_configuration;
        private FunctionalGroupState m_status;
        private AnalyserChannelStateMachineState m_channelStateMachine;
        #endregion
    }
    #endif
    #endregion

    #region AnalyserChannelTypeStartSingleAcquisitionMethodState Class
    #if (!OPCUA_EXCLUDE_AnalyserChannelTypeStartSingleAcquisitionMethodState)
    /// <summary>
    /// Stores an instance of the AnalyserChannelTypeStartSingleAcquisitionMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class AnalyserChannelTypeStartSingleAcquisitionMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public AnalyserChannelTypeStartSingleAcquisitionMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new AnalyserChannelTypeStartSingleAcquisitionMethodState(parent);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAzAAAAQW5hbHlzZXJDaGFubmVsVHlwZVN0YXJ0U2lu" +
           "Z2xlQWNxdWlzaXRpb25NZXRob2RUeXBlAQHMJgAvAQHMJswmAAABAf////8BAAAAF2CpCgIAAAAAAA4A" +
           "AABJbnB1dEFyZ3VtZW50cwEBzSYALgBEzSYAAJYDAAAAAQAqAQEfAAAADgAAAEV4ZWN1dGlvbkN5Y2xl" +
           "AQGiJP////8AAAAAAAEAKgEBJAAAABUAAABFeGVjdXRpb25DeWNsZVN1YmNvZGUAB/////8AAAAAAAEA" +
           "KgEBHQAAAA4AAABTZWxlY3RlZFN0cmVhbQAM/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAA" +
           "AAA=";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public AnalyserChannelTypeStartSingleAcquisitionMethodStateMethodCallHandler OnCall;
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

            ExecutionCycleEnumeration executionCycle = (ExecutionCycleEnumeration)_inputArguments[0];
            uint executionCycleSubcode = (uint)_inputArguments[1];
            string selectedStream = (string)_inputArguments[2];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    executionCycle,
                    executionCycleSubcode,
                    selectedStream);
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
    public delegate ServiceResult AnalyserChannelTypeStartSingleAcquisitionMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        ExecutionCycleEnumeration executionCycle,
        uint executionCycleSubcode,
        string selectedStream);
    #endif
    #endregion

    #region AnalyserChannelOperatingStateState Class
    #if (!OPCUA_EXCLUDE_AnalyserChannelOperatingStateState)
    /// <summary>
    /// Stores an instance of the AnalyserChannelOperatingStateType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class AnalyserChannelOperatingStateState : StateMachineStateState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public AnalyserChannelOperatingStateState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.ObjectTypes.AnalyserChannelOperatingStateType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////4RggAIBAAAAAQApAAAAQW5hbHlzZXJDaGFubmVsT3BlcmF0aW5nU3Rh" +
           "dGVUeXBlSW5zdGFuY2UBAewDAQHsA+wDAAABAQAAAAB1AAEB3CYBAAAAFWCJCgIAAAAAAAsAAABTdGF0" +
           "ZU51bWJlcgEBziYALgBEziYAAAAH/////wEB/////wAAAAA=";
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

    #region AnalyserChannelLocalStateState Class
    #if (!OPCUA_EXCLUDE_AnalyserChannelLocalStateState)
    /// <summary>
    /// Stores an instance of the AnalyserChannelLocalStateType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class AnalyserChannelLocalStateState : StateMachineStateState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public AnalyserChannelLocalStateState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.ObjectTypes.AnalyserChannelLocalStateType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAlAAAAQW5hbHlzZXJDaGFubmVsTG9jYWxTdGF0ZVR5" +
           "cGVJbnN0YW5jZQEB7QMBAe0D7QMAAP////8BAAAAFWCJCgIAAAAAAAsAAABTdGF0ZU51bWJlcgEBzyYA" +
           "LgBEzyYAAAAH/////wEB/////wAAAAA=";
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

    #region AnalyserChannelMaintenanceStateState Class
    #if (!OPCUA_EXCLUDE_AnalyserChannelMaintenanceStateState)
    /// <summary>
    /// Stores an instance of the AnalyserChannelMaintenanceStateType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class AnalyserChannelMaintenanceStateState : StateMachineStateState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public AnalyserChannelMaintenanceStateState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.ObjectTypes.AnalyserChannelMaintenanceStateType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQArAAAAQW5hbHlzZXJDaGFubmVsTWFpbnRlbmFuY2VT" +
           "dGF0ZVR5cGVJbnN0YW5jZQEB7gMBAe4D7gMAAP////8BAAAAFWCJCgIAAAAAAAsAAABTdGF0ZU51bWJl" +
           "cgEB0CYALgBE0CYAAAAH/////wEB/////wAAAAA=";
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

    #region AnalyserChannelStateMachineState Class
    #if (!OPCUA_EXCLUDE_AnalyserChannelStateMachineState)
    /// <summary>
    /// Stores an instance of the AnalyserChannelStateMachineType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class AnalyserChannelStateMachineState : FiniteStateMachineState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public AnalyserChannelStateMachineState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.ObjectTypes.AnalyserChannelStateMachineType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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

            if (LocalSubStateMachine != null)
            {
                LocalSubStateMachine.Initialize(context, LocalSubStateMachine_InitializationString);
            }

            if (MaintenanceSubStateMachine != null)
            {
                MaintenanceSubStateMachine.Initialize(context, MaintenanceSubStateMachine_InitializationString);
            }
        }

        #region Initialization String
        private const string LocalSubStateMachine_InitializationString =
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////wRggAoBAAAAAQAUAAAATG9jYWxTdWJTdGF0ZU1hY2hpbmUBAfQmAC8B" +
           "ANMK9CYAAAEAAAAAdQEBARAnAQAAABVgiQoCAAAAAAAMAAAAQ3VycmVudFN0YXRlAQH1JgAvAQDICvUm" +
           "AAAAFf////8BAf////8BAAAAFWCJCgIAAAAAAAIAAABJZAEB9iYALgBE9iYAAAAR/////wEB/////wAA" +
           "AAA=";

        private const string MaintenanceSubStateMachine_InitializationString =
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////wRggAoBAAAAAQAaAAAATWFpbnRlbmFuY2VTdWJTdGF0ZU1hY2hpbmUB" +
           "AQAnAC8BANMKACcAAAEAAAAAdQEBARInAQAAABVgiQoCAAAAAAAMAAAAQ3VycmVudFN0YXRlAQEBJwAv" +
           "AQDICgEnAAAAFf////8BAf////8BAAAAFWCJCgIAAAAAAAIAAABJZAEBAicALgBEAicAAAAR/////wEB" +
           "/////wAAAAA=";

        private const string InitializationString =
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////4RggAIBAAAAAQAnAAAAQW5hbHlzZXJDaGFubmVsU3RhdGVNYWNoaW5l" +
           "VHlwZUluc3RhbmNlAQHvAwEB7wPvAwAAAf////8EAAAAFWCJCgIAAAAAAAwAAABDdXJyZW50U3RhdGUB" +
           "AdEmAC8BAMgK0SYAAAAV/////wEB/////wEAAAAVYIkKAgAAAAAAAgAAAElkAQHSJgAuAETSJgAAABH/" +
           "////AQH/////AAAAAARggAoBAAAAAQAYAAAAT3BlcmF0aW5nU3ViU3RhdGVNYWNoaW5lAQHcJgAvAQHw" +
           "A9wmAAABAAAAAHUBAQEOJwIAAAAVYIkKAgAAAAAADAAAAEN1cnJlbnRTdGF0ZQEB3SYALwEAyArdJgAA" +
           "ABX/////AQH/////AQAAABVgiQoCAAAAAAACAAAASWQBAd4mAC4ARN4mAAAAEf////8BAf////8AAAAA" +
           "BGCACgEAAAABAB8AAABPcGVyYXRpbmdFeGVjdXRlU3ViU3RhdGVNYWNoaW5lAQHoJgAvAQHxA+gmAAD/" +
           "////AQAAABVgiQoCAAAAAAAMAAAAQ3VycmVudFN0YXRlAQHpJgAvAQDICukmAAAAFf////8BAf////8B" +
           "AAAAFWCJCgIAAAAAAAIAAABJZAEB6iYALgBE6iYAAAAR/////wEB/////wAAAAAEYIAKAQAAAAEAFAAA" +
           "AExvY2FsU3ViU3RhdGVNYWNoaW5lAQH0JgAvAQDTCvQmAAABAAAAAHUBAQEQJwEAAAAVYIkKAgAAAAAA" +
           "DAAAAEN1cnJlbnRTdGF0ZQEB9SYALwEAyAr1JgAAABX/////AQH/////AQAAABVgiQoCAAAAAAACAAAA" +
           "SWQBAfYmAC4ARPYmAAAAEf////8BAf////8AAAAABGCACgEAAAABABoAAABNYWludGVuYW5jZVN1YlN0" +
           "YXRlTWFjaGluZQEBACcALwEA0woAJwAAAQAAAAB1AQEBEicBAAAAFWCJCgIAAAAAAAwAAABDdXJyZW50" +
           "U3RhdGUBAQEnAC8BAMgKAScAAAAV/////wEB/////wEAAAAVYIkKAgAAAAAAAgAAAElkAQECJwAuAEQC" +
           "JwAAABH/////AQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public AnalyserChannel_OperatingModeSubStateMachineState OperatingSubStateMachine
        {
            get
            {
                return m_operatingSubStateMachine;
            }

            set
            {
                if (!Object.ReferenceEquals(m_operatingSubStateMachine, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_operatingSubStateMachine = value;
            }
        }

        /// <remarks />
        public FiniteStateMachineState LocalSubStateMachine
        {
            get
            {
                return m_localSubStateMachine;
            }

            set
            {
                if (!Object.ReferenceEquals(m_localSubStateMachine, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_localSubStateMachine = value;
            }
        }

        /// <remarks />
        public FiniteStateMachineState MaintenanceSubStateMachine
        {
            get
            {
                return m_maintenanceSubStateMachine;
            }

            set
            {
                if (!Object.ReferenceEquals(m_maintenanceSubStateMachine, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_maintenanceSubStateMachine = value;
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
            if (m_operatingSubStateMachine != null)
            {
                children.Add(m_operatingSubStateMachine);
            }

            if (m_localSubStateMachine != null)
            {
                children.Add(m_localSubStateMachine);
            }

            if (m_maintenanceSubStateMachine != null)
            {
                children.Add(m_maintenanceSubStateMachine);
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
                case Opc.Ua.Adi.BrowseNames.OperatingSubStateMachine:
                {
                    if (createOrReplace)
                    {
                        if (OperatingSubStateMachine == null)
                        {
                            if (replacement == null)
                            {
                                OperatingSubStateMachine = new AnalyserChannel_OperatingModeSubStateMachineState(this);
                            }
                            else
                            {
                                OperatingSubStateMachine = (AnalyserChannel_OperatingModeSubStateMachineState)replacement;
                            }
                        }
                    }

                    instance = OperatingSubStateMachine;
                    break;
                }

                case Opc.Ua.Adi.BrowseNames.LocalSubStateMachine:
                {
                    if (createOrReplace)
                    {
                        if (LocalSubStateMachine == null)
                        {
                            if (replacement == null)
                            {
                                LocalSubStateMachine = new FiniteStateMachineState(this);
                            }
                            else
                            {
                                LocalSubStateMachine = (FiniteStateMachineState)replacement;
                            }
                        }
                    }

                    instance = LocalSubStateMachine;
                    break;
                }

                case Opc.Ua.Adi.BrowseNames.MaintenanceSubStateMachine:
                {
                    if (createOrReplace)
                    {
                        if (MaintenanceSubStateMachine == null)
                        {
                            if (replacement == null)
                            {
                                MaintenanceSubStateMachine = new FiniteStateMachineState(this);
                            }
                            else
                            {
                                MaintenanceSubStateMachine = (FiniteStateMachineState)replacement;
                            }
                        }
                    }

                    instance = MaintenanceSubStateMachine;
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
        private AnalyserChannel_OperatingModeSubStateMachineState m_operatingSubStateMachine;
        private FiniteStateMachineState m_localSubStateMachine;
        private FiniteStateMachineState m_maintenanceSubStateMachine;
        #endregion
    }
    #endif
    #endregion

    #region AnalyserChannelOperatingExecuteStateState Class
    #if (!OPCUA_EXCLUDE_AnalyserChannelOperatingExecuteStateState)
    /// <summary>
    /// Stores an instance of the AnalyserChannelOperatingExecuteStateType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class AnalyserChannelOperatingExecuteStateState : StateMachineStateState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public AnalyserChannelOperatingExecuteStateState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.ObjectTypes.AnalyserChannelOperatingExecuteStateType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////4RggAIBAAAAAQAwAAAAQW5hbHlzZXJDaGFubmVsT3BlcmF0aW5nRXhl" +
           "Y3V0ZVN0YXRlVHlwZUluc3RhbmNlAQEEIwEBBCMEIwAAAQEAAAAAdQABATQnAQAAABVgiQoCAAAAAAAL" +
           "AAAAU3RhdGVOdW1iZXIBASgnAC4ARCgnAAAAB/////8BAf////8AAAAA";
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

    #region AnalyserChannel_OperatingModeSubStateMachineState Class
    #if (!OPCUA_EXCLUDE_AnalyserChannel_OperatingModeSubStateMachineState)
    /// <summary>
    /// Stores an instance of the AnalyserChannel_OperatingModeSubStateMachineType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class AnalyserChannel_OperatingModeSubStateMachineState : FiniteStateMachineState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public AnalyserChannel_OperatingModeSubStateMachineState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.ObjectTypes.AnalyserChannel_OperatingModeSubStateMachineType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////4RggAIBAAAAAQA4AAAAQW5hbHlzZXJDaGFubmVsX09wZXJhdGluZ01v" +
           "ZGVTdWJTdGF0ZU1hY2hpbmVUeXBlSW5zdGFuY2UBAfADAQHwA/ADAAAB/////wIAAAAVYIkKAgAAAAAA" +
           "DAAAAEN1cnJlbnRTdGF0ZQEBKScALwEAyAopJwAAABX/////AQH/////AQAAABVgiQoCAAAAAAACAAAA" +
           "SWQBASonAC4ARConAAAAEf////8BAf////8AAAAABGCACgEAAAABAB8AAABPcGVyYXRpbmdFeGVjdXRl" +
           "U3ViU3RhdGVNYWNoaW5lAQE0JwAvAQHxAzQnAAABAAAAAHUBAQFIJwEAAAAVYIkKAgAAAAAADAAAAEN1" +
           "cnJlbnRTdGF0ZQEBNScALwEAyAo1JwAAABX/////AQH/////AQAAABVgiQoCAAAAAAACAAAASWQBATYn" +
           "AC4ARDYnAAAAEf////8BAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public AnalyserChannel_OperatingModeExecuteSubStateMachineState OperatingExecuteSubStateMachine
        {
            get
            {
                return m_operatingExecuteSubStateMachine;
            }

            set
            {
                if (!Object.ReferenceEquals(m_operatingExecuteSubStateMachine, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_operatingExecuteSubStateMachine = value;
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
            if (m_operatingExecuteSubStateMachine != null)
            {
                children.Add(m_operatingExecuteSubStateMachine);
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
                case Opc.Ua.Adi.BrowseNames.OperatingExecuteSubStateMachine:
                {
                    if (createOrReplace)
                    {
                        if (OperatingExecuteSubStateMachine == null)
                        {
                            if (replacement == null)
                            {
                                OperatingExecuteSubStateMachine = new AnalyserChannel_OperatingModeExecuteSubStateMachineState(this);
                            }
                            else
                            {
                                OperatingExecuteSubStateMachine = (AnalyserChannel_OperatingModeExecuteSubStateMachineState)replacement;
                            }
                        }
                    }

                    instance = OperatingExecuteSubStateMachine;
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
        private AnalyserChannel_OperatingModeExecuteSubStateMachineState m_operatingExecuteSubStateMachine;
        #endregion
    }
    #endif
    #endregion

    #region AnalyserChannel_OperatingModeExecuteSubStateMachineState Class
    #if (!OPCUA_EXCLUDE_AnalyserChannel_OperatingModeExecuteSubStateMachineState)
    /// <summary>
    /// Stores an instance of the AnalyserChannel_OperatingModeExecuteSubStateMachineType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class AnalyserChannel_OperatingModeExecuteSubStateMachineState : FiniteStateMachineState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public AnalyserChannel_OperatingModeExecuteSubStateMachineState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.ObjectTypes.AnalyserChannel_OperatingModeExecuteSubStateMachineType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////4RggAIBAAAAAQA/AAAAQW5hbHlzZXJDaGFubmVsX09wZXJhdGluZ01v" +
           "ZGVFeGVjdXRlU3ViU3RhdGVNYWNoaW5lVHlwZUluc3RhbmNlAQHxAwEB8QPxAwAAAf////8BAAAAFWCJ" +
           "CgIAAAAAAAwAAABDdXJyZW50U3RhdGUBAc4nAC8BAMgKzicAAAAV/////wEB/////wEAAAAVYIkKAgAA" +
           "AAAAAgAAAElkAQHPJwAuAETPJwAAABH/////AQH/////AAAAAA==";
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

    #region StreamState Class
    #if (!OPCUA_EXCLUDE_StreamState)
    /// <summary>
    /// Stores an instance of the StreamType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class StreamState : TopologyElementState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public StreamState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.ObjectTypes.StreamType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQASAAAAU3RyZWFtVHlwZUluc3RhbmNlAQHyAwEB8gPy" +
           "AwAA/////wgAAAAkYIAKAQAAAAIADAAAAFBhcmFtZXRlclNldAEBTSgDAAAAABcAAABGbGF0IGxpc3Qg" +
           "b2YgUGFyYW1ldGVycwAvADpNKAAA/////xwAAAA1YIkKAgAAAAEACQAAAElzRW5hYmxlZAEBYygDAAAA" +
           "ADUAAABUcnVlIGlmIHRoaXMgc3RyZWFtIG1heWJlIHVzZWQgdG8gcGVyZm9ybSBhY3F1aXNpdGlvbgAv" +
           "AQA9CWMoAAAAAf////8BAQEAAAAAIwEBAb4oAAAAADVgiQoCAAAAAQAIAAAASXNGb3JjZWQBAWYoAwAA" +
           "AACCAAAAVHJ1ZSBpZiB0aGlzIHN0cmVhbSBpcyBmb3JjZWQsIHdoaWNoIG1lYW5zIHRoYXQgaXMgdGhl" +
           "IG9ubHkgU3RyZWFtIG9uIHRoaXMgQW5hbHlzZXJDaGFubmVsIHRoYXQgY2FuIGJlIHVzZWQgdG8gcGVy" +
           "Zm9ybSBhY3F1aXNpdGlvbgAvAQA9CWYoAAAAAf////8BAQEAAAAAIwEBAb4oAAAAADVgiQoCAAAAAQAQ" +
           "AAAARGlhZ25vc3RpY1N0YXR1cwEBaSgDAAAAABQAAABTdHJlYW0gaGVhbHRoIHN0YXR1cwAvAQA9CWko" +
           "AAABAmQY/////wEBAQAAAAAjAQEBwCgAAAAANWCJCgIAAAABABMAAABMYXN0Q2FsaWJyYXRpb25UaW1l" +
           "AQFsKAMAAAAAKgAAAFRpbWUgYXQgd2hpY2ggdGhlIGxhc3QgY2FsaWJyYXRpb24gd2FzIHJ1bgAvAQA9" +
           "CWwoAAAADf////8BAQEAAAAAIwEBAcAoAAAAADVgiQoCAAAAAQASAAAATGFzdFZhbGlkYXRpb25UaW1l" +
           "AQFvKAMAAAAAKQAAAFRpbWUgYXQgd2hpY2ggdGhlIGxhc3QgdmFsaWRhdGlvbiB3YXMgcnVuAC8BAD0J" +
           "bygAAAAN/////wEBAQAAAAAjAQEBwCgAAAAANWCJCgIAAAABAA4AAABMYXN0U2FtcGxlVGltZQEBcigD" +
           "AAAAACoAAABUaW1lIGF0IHdoaWNoIHRoZSBsYXN0IHNhbXBsZSB3YXMgYWNxdWlyZWQALwEAPQlyKAAA" +
           "AA3/////AQEBAAAAACMBAQHAKAAAAAA1YIkKAgAAAAEAEgAAAFRpbWVCZXR3ZWVuU2FtcGxlcwEBdSgD" +
           "AAAAAEQAAABOdW1iZXIgb2YgbWlsbGlzZWNvbmRzIGJldHdlZW4gdHdvIGNvbnNlY3V0aXZlIHN0YXJ0" +
           "cyBvZiBhY3F1aXNpdGlvbgAvAQBACXUoAAABACIB/////wEBAQAAAAAjAQEBwigBAAAAFWCJCgIAAAAA" +
           "AAcAAABFVVJhbmdlAQF5KAAuAER5KAAAAQB0A/////8BAf////8AAAAANWCJCgIAAAABAAgAAABJc0Fj" +
           "dGl2ZQEBeygDAAAAADcAAABUcnVlIGlmIHRoaXMgc3RyZWFtIGlzIGFjdHVhbGx5IHJ1bm5pbmcsIGFj" +
           "cXVpcmluZyBkYXRhAC8BAD0JeygAAAAB/////wEBAQAAAAAjAQEBxCgAAAAANWCJCgIAAAABAA4AAABF" +
           "eGVjdXRpb25DeWNsZQEBfigDAAAAAC4AAABJbmRpY2F0ZXMgd2hpY2ggRXhlY3V0aW9uIGN5Y2xlIGlz" +
           "IGluIHByb2dyZXNzAC8BAD0JfigAAAEBoiT/////AQEBAAAAACMBAQHEKAAAAAA1YIkKAgAAAAEAFQAA" +
           "AEV4ZWN1dGlvbkN5Y2xlU3ViY29kZQEBgSgDAAAAADYAAABJbmRpY2F0ZXMgd2hpY2ggRXhlY3V0aW9u" +
           "IGN5Y2xlIHN1YmNvZGUgaXMgaW4gcHJvZ3Jlc3MALwEASAmBKAAAABz/////AQEBAAAAACMBAQHEKAEA" +
           "AAAXYIkKAgAAAAAACwAAAEVudW1TdHJpbmdzAQGEKAAuAESEKAAAABUBAAAAAQAAAAAAAAABAf////8A" +
           "AAAANWCJCgIAAAABAAgAAABQcm9ncmVzcwEBhSgDAAAAAHQAAABJbmRpY2F0ZXMgdGhlIHByb2dyZXNz" +
           "IG9mIGFuIGFjcXVpc2l0aW9uIGluIHRlcm1zIG9mIHBlcmNlbnRhZ2Ugb2YgY29tcGxldGlvbi4gSXRz" +
           "IHZhbHVlIHNoYWxsIGJlIGJldHdlZW4gMCBhbmQgMTAwLgAvAQA9CYUoAAAACv////8BAQEAAAAAIwEB" +
           "AcQoAAAAADVgiQoCAAAAAQASAAAAQWNxdWlzaXRpb25Db3VudGVyAQGIKAMAAAAAUwAAAFNpbXBsZSBj" +
           "b3VudGVyIGluY3JlbWVudGVkIGFmdGVyIGVhY2ggU2FtcGxpbmcgYWNxdWlzaXRpb24gcGVyZm9ybWVk" +
           "IG9uIHRoaXMgU3RyZWFtAC8BAEAJiCgAAAEAIQH/////AQEBAAAAACMBAQHGKAEAAAAVYIkKAgAAAAAA" +
           "BwAAAEVVUmFuZ2UBAYwoAC4ARIwoAAABAHQD/////wEB/////wAAAAA1YIkKAgAAAAEAFwAAAEFjcXVp" +
           "c2l0aW9uUmVzdWx0U3RhdHVzAQGOKAMAAAAAGgAAAFF1YWxpdHkgb2YgdGhlIGFjcXVpc2l0aW9uAC8B" +
           "AD0JjigAAAEBuwv/////AQEBAAAAACMBAQHGKAAAAAA1YIkKAgAAAAEABwAAAFJhd0RhdGEBAZEoAwAA" +
           "AAA/AAAAUmF3IGRhdGEgcHJvZHVjZWQgYXMgYSByZXN1bHQgb2YgZGF0YSBhY3F1aXNpdGlvbiBvbiB0" +
           "aGUgU3RyZWFtAC8BAD0JkSgAAAAY/////wEBAQAAAAAjAQEBxigAAAAANWCJCgIAAAABAAoAAABTY2Fs" +
           "ZWREYXRhAQGUKAMAAAAAaAAAAFNjYWxlZCBkYXRhIHByb2R1Y2VkIGFzIGEgcmVzdWx0IG9mIGRhdGEg" +
           "YWNxdWlzaXRpb24gb24gdGhlIFN0cmVhbSBhbmQgYXBwbGljYXRpb24gb2YgdGhlIGFuYWx5c2VyIG1v" +
           "ZGVsAC8BAD0JlCgAAAAY/////wEBAQAAAAAjAQEBxigAAAAANWCJCgIAAAABAAYAAABPZmZzZXQBAZco" +
           "AwAAAABgAAAARGlmZmVyZW5jZSBpbiBtaWxsaXNlY29uZHMgYmV0d2VlbiB0aGUgc3RhcnQgb2Ygc2Ft" +
           "cGxlIGV4dHJhY3Rpb24gYW5kIHRoZSBzdGFydCBvZiB0aGUgYW5hbHlzaXMuAC8BAD0JlygAAAEAIgH/" +
           "////AQEBAAAAACMBAQHGKAAAAAA1YIkKAgAAAAEAEgAAAEFjcXVpc2l0aW9uRW5kVGltZQEBmigDAAAA" +
           "AKcAAABUaGUgZW5kIHRpbWUgb2YgdGhlIEFuYWx5c2VTYW1wbGUgb3IgQW5hbHlzZUNhbGlicmF0aW9u" +
           "U2FtcGxlIG9yIEFuYWx5c2VWYWxpZGF0aW9uU2FtcGxlIHN0YXRlIG9mIHRoZSBBbmFseXNlckNoYW5u" +
           "ZWxfT3BlcmF0aW5nTW9kZUV4ZWN1dGVTdWJTdGF0ZU1hY2hpbmUgc3RhdGUgbWFjaGluZQAvAQA9CZoo" +
           "AAAADf////8BAQEAAAAAIwEBAcYoAAAAADVgiQoCAAAAAQAKAAAAQ2FtcGFpZ25JZAEBnSgDAAAAABwA" +
           "AABEZWZpbmVzIHRoZSBjdXJyZW50IGNhbXBhaWduAC8BAD0JnSgAAAAM/////wMDAQAAAAAjAQEByigA" +
           "AAAANWCJCgIAAAABAAcAAABCYXRjaElkAQGgKAMAAAAAGQAAAERlZmluZXMgdGhlIGN1cnJlbnQgYmF0" +
           "Y2gALwEAPQmgKAAAAAz/////AwMBAAAAACMBAQHKKAAAAAA1YIkKAgAAAAEACgAAAFN1YkJhdGNoSWQB" +
           "AaMoAwAAAAAdAAAARGVmaW5lcyB0aGUgY3VycmVudCBzdWItYmF0Y2gALwEAPQmjKAAAAAz/////AwMB" +
           "AAAAACMBAQHKKAAAAAA1YIkKAgAAAAEABQAAAExvdElkAQGmKAMAAAAAFwAAAERlZmluZXMgdGhlIGN1" +
           "cnJlbnQgbG90AC8BAD0JpigAAAAM/////wMDAQAAAAAjAQEByigAAAAANWCJCgIAAAABAAoAAABNYXRl" +
           "cmlhbElkAQGpKAMAAAAAHAAAAERlZmluZXMgdGhlIGN1cnJlbnQgbWF0ZXJpYWwALwEAPQmpKAAAAAz/" +
           "////AwMBAAAAACMBAQHKKAAAAAA1YIkKAgAAAAEABwAAAFByb2Nlc3MBAawoAwAAAAAUAAAAQ3VycmVu" +
           "dCBQcm9jZXNzIG5hbWUALwEAPQmsKAAAAAz/////AwMBAAAAACMBAQHKKAAAAAA1YIkKAgAAAAEABAAA" +
           "AFVuaXQBAa8oAwAAAAARAAAAQ3VycmVudCBVbml0IG5hbWUALwEAPQmvKAAAAAz/////AwMBAAAAACMB" +
           "AQHKKAAAAAA1YIkKAgAAAAEACQAAAE9wZXJhdGlvbgEBsigDAAAAABYAAABDdXJyZW50IE9wZXJhdGlv" +
           "biBuYW1lAC8BAD0JsigAAAAM/////wMDAQAAAAAjAQEByigAAAAANWCJCgIAAAABAAUAAABQaGFzZQEB" +
           "tSgDAAAAABIAAABDdXJyZW50IFBoYXNlIG5hbWUALwEAPQm1KAAAAAz/////AwMBAAAAACMBAQHKKAAA" +
           "AAA1YIkKAgAAAAEABgAAAFVzZXJJZAEBuCgDAAAAAD0AAABMb2dpbiBuYW1lIG9mIHRoZSB1c2VyIHdo" +
           "byBpcyBsb2dnZWQgb24gYXQgdGhlIGRldmljZSBjb25zb2xlAC8BAD0JuCgAAAAM/////wMDAQAAAAAj" +
           "AQEByigAAAAANWCJCgIAAAABAAgAAABTYW1wbGVJZAEBuygDAAAAABkAAABJZGVudGlmaWVyIGZvciB0" +
           "aGUgc2FtcGxlAC8BAD0JuygAAAAM/////wMDAQAAAAAjAQEByigAAAAABGCACgEAAAABAA0AAABDb25m" +
           "aWd1cmF0aW9uAQG+KAAvAQLtA74oAAACAAAAACMAAQFjKAAjAAEBZigAAAAABGCACgEAAAABAAYAAABT" +
           "dGF0dXMBAcAoAC8BAu0DwCgAAAQAAAAAIwABAWkoACMAAQFsKAAjAAEBbygAIwABAXIoAAAAAARggAoB" +
           "AAAAAQATAAAAQWNxdWlzaXRpb25TZXR0aW5ncwEBwigALwEC7QPCKAAAAQAAAAAjAAEBdSgAAAAABGCA" +
           "CgEAAAABABEAAABBY3F1aXNpdGlvblN0YXR1cwEBxCgALwEC7QPEKAAABAAAAAAjAAEBeygAIwABAX4o" +
           "ACMAAQGBKAAjAAEBhSgAAAAABGCACgEAAAABAA8AAABBY3F1aXNpdGlvbkRhdGEBAcYoAC8BAu0DxigA" +
           "AAYAAAAAIwABAYgoACMAAQGOKAAjAAEBkSgAIwABAZQoACMAAQGXKAAjAAEBmigAAAAABGCACgEAAAAB" +
           "ABgAAABDaGVtb21ldHJpY01vZGVsU2V0dGluZ3MBAcgoAC8BAu0DyCgAAP////8AAAAABGCACgEAAAAB" +
           "AAcAAABDb250ZXh0AQHKKAAvAQLtA8ooAAALAAAAACMAAQGdKAAjAAEBoCgAIwABAaMoACMAAQGmKAAj" +
           "AAEBqSgAIwABAawoACMAAQGvKAAjAAEBsigAIwABAbUoACMAAQG4KAAjAAEBuygAAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public FunctionalGroupState Configuration
        {
            get
            {
                return m_configuration;
            }

            set
            {
                if (!Object.ReferenceEquals(m_configuration, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_configuration = value;
            }
        }

        /// <remarks />
        public FunctionalGroupState Status
        {
            get
            {
                return m_status;
            }

            set
            {
                if (!Object.ReferenceEquals(m_status, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_status = value;
            }
        }

        /// <remarks />
        public FunctionalGroupState AcquisitionSettings
        {
            get
            {
                return m_acquisitionSettings;
            }

            set
            {
                if (!Object.ReferenceEquals(m_acquisitionSettings, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_acquisitionSettings = value;
            }
        }

        /// <remarks />
        public FunctionalGroupState AcquisitionStatus
        {
            get
            {
                return m_acquisitionStatus;
            }

            set
            {
                if (!Object.ReferenceEquals(m_acquisitionStatus, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_acquisitionStatus = value;
            }
        }

        /// <remarks />
        public FunctionalGroupState AcquisitionData
        {
            get
            {
                return m_acquisitionData;
            }

            set
            {
                if (!Object.ReferenceEquals(m_acquisitionData, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_acquisitionData = value;
            }
        }

        /// <remarks />
        public FunctionalGroupState ChemometricModelSettings
        {
            get
            {
                return m_chemometricModelSettings;
            }

            set
            {
                if (!Object.ReferenceEquals(m_chemometricModelSettings, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_chemometricModelSettings = value;
            }
        }

        /// <remarks />
        public FunctionalGroupState Context
        {
            get
            {
                return m_context;
            }

            set
            {
                if (!Object.ReferenceEquals(m_context, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_context = value;
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
            if (m_configuration != null)
            {
                children.Add(m_configuration);
            }

            if (m_status != null)
            {
                children.Add(m_status);
            }

            if (m_acquisitionSettings != null)
            {
                children.Add(m_acquisitionSettings);
            }

            if (m_acquisitionStatus != null)
            {
                children.Add(m_acquisitionStatus);
            }

            if (m_acquisitionData != null)
            {
                children.Add(m_acquisitionData);
            }

            if (m_chemometricModelSettings != null)
            {
                children.Add(m_chemometricModelSettings);
            }

            if (m_context != null)
            {
                children.Add(m_context);
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
                case Opc.Ua.Adi.BrowseNames.Configuration:
                {
                    if (createOrReplace)
                    {
                        if (Configuration == null)
                        {
                            if (replacement == null)
                            {
                                Configuration = new FunctionalGroupState(this);
                            }
                            else
                            {
                                Configuration = (FunctionalGroupState)replacement;
                            }
                        }
                    }

                    instance = Configuration;
                    break;
                }

                case Opc.Ua.Adi.BrowseNames.Status:
                {
                    if (createOrReplace)
                    {
                        if (Status == null)
                        {
                            if (replacement == null)
                            {
                                Status = new FunctionalGroupState(this);
                            }
                            else
                            {
                                Status = (FunctionalGroupState)replacement;
                            }
                        }
                    }

                    instance = Status;
                    break;
                }

                case Opc.Ua.Adi.BrowseNames.AcquisitionSettings:
                {
                    if (createOrReplace)
                    {
                        if (AcquisitionSettings == null)
                        {
                            if (replacement == null)
                            {
                                AcquisitionSettings = new FunctionalGroupState(this);
                            }
                            else
                            {
                                AcquisitionSettings = (FunctionalGroupState)replacement;
                            }
                        }
                    }

                    instance = AcquisitionSettings;
                    break;
                }

                case Opc.Ua.Adi.BrowseNames.AcquisitionStatus:
                {
                    if (createOrReplace)
                    {
                        if (AcquisitionStatus == null)
                        {
                            if (replacement == null)
                            {
                                AcquisitionStatus = new FunctionalGroupState(this);
                            }
                            else
                            {
                                AcquisitionStatus = (FunctionalGroupState)replacement;
                            }
                        }
                    }

                    instance = AcquisitionStatus;
                    break;
                }

                case Opc.Ua.Adi.BrowseNames.AcquisitionData:
                {
                    if (createOrReplace)
                    {
                        if (AcquisitionData == null)
                        {
                            if (replacement == null)
                            {
                                AcquisitionData = new FunctionalGroupState(this);
                            }
                            else
                            {
                                AcquisitionData = (FunctionalGroupState)replacement;
                            }
                        }
                    }

                    instance = AcquisitionData;
                    break;
                }

                case Opc.Ua.Adi.BrowseNames.ChemometricModelSettings:
                {
                    if (createOrReplace)
                    {
                        if (ChemometricModelSettings == null)
                        {
                            if (replacement == null)
                            {
                                ChemometricModelSettings = new FunctionalGroupState(this);
                            }
                            else
                            {
                                ChemometricModelSettings = (FunctionalGroupState)replacement;
                            }
                        }
                    }

                    instance = ChemometricModelSettings;
                    break;
                }

                case Opc.Ua.Adi.BrowseNames.Context:
                {
                    if (createOrReplace)
                    {
                        if (Context == null)
                        {
                            if (replacement == null)
                            {
                                Context = new FunctionalGroupState(this);
                            }
                            else
                            {
                                Context = (FunctionalGroupState)replacement;
                            }
                        }
                    }

                    instance = Context;
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
        private FunctionalGroupState m_configuration;
        private FunctionalGroupState m_status;
        private FunctionalGroupState m_acquisitionSettings;
        private FunctionalGroupState m_acquisitionStatus;
        private FunctionalGroupState m_acquisitionData;
        private FunctionalGroupState m_chemometricModelSettings;
        private FunctionalGroupState m_context;
        #endregion
    }
    #endif
    #endregion

    #region SpectrometerDeviceStreamState Class
    #if (!OPCUA_EXCLUDE_SpectrometerDeviceStreamState)
    /// <summary>
    /// Stores an instance of the SpectrometerDeviceStreamType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class SpectrometerDeviceStreamState : StreamState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public SpectrometerDeviceStreamState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.ObjectTypes.SpectrometerDeviceStreamType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAkAAAAU3BlY3Ryb21ldGVyRGV2aWNlU3RyZWFtVHlw" +
           "ZUluc3RhbmNlAQEGBAEBBgQGBAAA/////wkAAAAkYIAKAQAAAAIADAAAAFBhcmFtZXRlclNldAEBzigD" +
           "AAAAABcAAABGbGF0IGxpc3Qgb2YgUGFyYW1ldGVycwAvADrOKAAA/////xgAAAA1YIkKAgAAAAEACQAA" +
           "AElzRW5hYmxlZAEB5CgDAAAAADUAAABUcnVlIGlmIHRoaXMgc3RyZWFtIG1heWJlIHVzZWQgdG8gcGVy" +
           "Zm9ybSBhY3F1aXNpdGlvbgAvAQA9CeQoAAAAAf////8BAQEAAAAAIwEBAT8pAAAAADVgiQoCAAAAAQAQ" +
           "AAAARGlhZ25vc3RpY1N0YXR1cwEB6igDAAAAABQAAABTdHJlYW0gaGVhbHRoIHN0YXR1cwAvAQA9Ceoo" +
           "AAABAmQY/////wEBAQAAAAAjAQEBQSkAAAAANWCJCgIAAAABAA4AAABMYXN0U2FtcGxlVGltZQEB8ygD" +
           "AAAAACoAAABUaW1lIGF0IHdoaWNoIHRoZSBsYXN0IHNhbXBsZSB3YXMgYWNxdWlyZWQALwEAPQnzKAAA" +
           "AA3/////AQEBAAAAACMBAQFBKQAAAAA1YIkKAgAAAAEACAAAAElzQWN0aXZlAQH8KAMAAAAANwAAAFRy" +
           "dWUgaWYgdGhpcyBzdHJlYW0gaXMgYWN0dWFsbHkgcnVubmluZywgYWNxdWlyaW5nIGRhdGEALwEAPQn8" +
           "KAAAAAH/////AQEBAAAAACMBAQFFKQAAAAA1YIkKAgAAAAEADgAAAEV4ZWN1dGlvbkN5Y2xlAQH/KAMA" +
           "AAAALgAAAEluZGljYXRlcyB3aGljaCBFeGVjdXRpb24gY3ljbGUgaXMgaW4gcHJvZ3Jlc3MALwEAPQn/" +
           "KAAAAQGiJP////8BAQEAAAAAIwEBAUUpAAAAADVgiQoCAAAAAQAVAAAARXhlY3V0aW9uQ3ljbGVTdWJj" +
           "b2RlAQECKQMAAAAANgAAAEluZGljYXRlcyB3aGljaCBFeGVjdXRpb24gY3ljbGUgc3ViY29kZSBpcyBp" +
           "biBwcm9ncmVzcwAvAQBICQIpAAAAHP////8BAQEAAAAAIwEBAUUpAQAAABdgiQoCAAAAAAALAAAARW51" +
           "bVN0cmluZ3MBAQUpAC4ARAUpAAAAFQEAAAABAAAAAAAAAAEB/////wAAAAA1YIkKAgAAAAEACAAAAFBy" +
           "b2dyZXNzAQEGKQMAAAAAdAAAAEluZGljYXRlcyB0aGUgcHJvZ3Jlc3Mgb2YgYW4gYWNxdWlzaXRpb24g" +
           "aW4gdGVybXMgb2YgcGVyY2VudGFnZSBvZiBjb21wbGV0aW9uLiBJdHMgdmFsdWUgc2hhbGwgYmUgYmV0" +
           "d2VlbiAwIGFuZCAxMDAuAC8BAD0JBikAAAAK/////wEBAQAAAAAjAQEBRSkAAAAANWCJCgIAAAABABIA" +
           "AABBY3F1aXNpdGlvbkNvdW50ZXIBAQkpAwAAAABTAAAAU2ltcGxlIGNvdW50ZXIgaW5jcmVtZW50ZWQg" +
           "YWZ0ZXIgZWFjaCBTYW1wbGluZyBhY3F1aXNpdGlvbiBwZXJmb3JtZWQgb24gdGhpcyBTdHJlYW0ALwEA" +
           "QAkJKQAAAQAhAf////8BAQEAAAAAIwEBAUcpAQAAABVgiQoCAAAAAAAHAAAARVVSYW5nZQEBDSkALgBE" +
           "DSkAAAEAdAP/////AQH/////AAAAADVgiQoCAAAAAQAXAAAAQWNxdWlzaXRpb25SZXN1bHRTdGF0dXMB" +
           "AQ8pAwAAAAAaAAAAUXVhbGl0eSBvZiB0aGUgYWNxdWlzaXRpb24ALwEAPQkPKQAAAQG7C/////8BAQEA" +
           "AAAAIwEBAUcpAAAAADVgiQoCAAAAAQAKAAAAU2NhbGVkRGF0YQEBFSkDAAAAAGgAAABTY2FsZWQgZGF0" +
           "YSBwcm9kdWNlZCBhcyBhIHJlc3VsdCBvZiBkYXRhIGFjcXVpc2l0aW9uIG9uIHRoZSBTdHJlYW0gYW5k" +
           "IGFwcGxpY2F0aW9uIG9mIHRoZSBhbmFseXNlciBtb2RlbAAvAQA9CRUpAAAAGP////8BAQEAAAAAIwEB" +
           "AUcpAAAAADVgiQoCAAAAAQASAAAAQWNxdWlzaXRpb25FbmRUaW1lAQEbKQMAAAAApwAAAFRoZSBlbmQg" +
           "dGltZSBvZiB0aGUgQW5hbHlzZVNhbXBsZSBvciBBbmFseXNlQ2FsaWJyYXRpb25TYW1wbGUgb3IgQW5h" +
           "bHlzZVZhbGlkYXRpb25TYW1wbGUgc3RhdGUgb2YgdGhlIEFuYWx5c2VyQ2hhbm5lbF9PcGVyYXRpbmdN" +
           "b2RlRXhlY3V0ZVN1YlN0YXRlTWFjaGluZSBzdGF0ZSBtYWNoaW5lAC8BAD0JGykAAAAN/////wEBAQAA" +
           "AAAjAQEBRykAAAAAFWCJCgIAAAABABAAAABBY3RpdmVCYWNrZ3JvdW5kAQFPKQAvAQD9Lk8pAAAACv//" +
           "//8BAQEAAAAAIwEBAT8pBQAAABVgiQoCAAAAAAAHAAAARVVSYW5nZQEBUykALgBEUykAAAEAdAP/////" +
           "AQH/////AAAAABVgiQoCAAAAAAAQAAAARW5naW5lZXJpbmdVbml0cwEBVCkALgBEVCkAAAEAdwP/////" +
           "AQH/////AAAAABVgiQoCAAAAAAAFAAAAVGl0bGUBAVUpAC4ARFUpAAAAFf////8BAf////8AAAAAFWCJ" +
           "CgIAAAAAAA0AAABBeGlzU2NhbGVUeXBlAQFWKQAuAERWKQAAAQAtL/////8BAf////8AAAAAFWCJCgIA" +
           "AAAAAA8AAABYQXhpc0RlZmluaXRpb24BAVcpAC4ARFcpAAABAC8v/////wEB/////wAAAAAVYIkKAgAA" +
           "AAEAEQAAAEFjdGl2ZUJhY2tncm91bmQxAQFYKQAvAQD9LlgpAAAACv////8BAQEAAAAAIwEBAT8pBQAA" +
           "ABVgiQoCAAAAAAAHAAAARVVSYW5nZQEBXCkALgBEXCkAAAEAdAP/////AQH/////AAAAABVgiQoCAAAA" +
           "AAAQAAAARW5naW5lZXJpbmdVbml0cwEBXSkALgBEXSkAAAEAdwP/////AQH/////AAAAABVgiQoCAAAA" +
           "AAAFAAAAVGl0bGUBAV4pAC4ARF4pAAAAFf////8BAf////8AAAAAFWCJCgIAAAAAAA0AAABBeGlzU2Nh" +
           "bGVUeXBlAQFfKQAuAERfKQAAAQAtL/////8BAf////8AAAAAFWCJCgIAAAAAAA8AAABYQXhpc0RlZmlu" +
           "aXRpb24BAWApAC4ARGApAAABAC8v/////wEB/////wAAAAAXYIkKAgAAAAEADQAAAFNwZWN0cmFsUmFu" +
           "Z2UBAWEpAC8BAD0JYSkAAAEAdAMBAAAAAQAAAAAAAAABAQEAAAAAIwEBAUMpAAAAABVgiQoCAAAAAQAK" +
           "AAAAUmVzb2x1dGlvbgEBZCkALwEAPQlkKQAAABj/////AQEBAAAAACMBAQFDKQAAAAAVYIkKAgAAAAEA" +
           "FgAAAFJlcXVlc3RlZE51bWJlck9mU2NhbnMBAWcpAC8BAD0JZykAAAAG/////wEBAQAAAAAjAQEBQykA" +
           "AAAAFWCJCgIAAAABAAQAAABHYWluAQFqKQAvAQA9CWopAAAAGP////8BAQEAAAAAIwEBAUMpAAAAABVg" +
           "iQoCAAAAAQATAAAAVHJhbnNtaXR0YW5jZUN1dG9mZgEBbSkALwEAPQltKQAAAQB0A/////8BAQEAAAAA" +
           "IwEBAUMpAAAAABVgiQoCAAAAAQAQAAAAQWJzb3JiYW5jZUN1dG9mZgEBcCkALwEAPQlwKQAAAQB0A///" +
           "//8BAQEAAAAAIwEBAUMpAAAAABVgiQoCAAAAAQARAAAATnVtYmVyT2ZTY2Fuc0RvbmUBAXMpAC8BAD0J" +
           "cykAAAAG/////wEBAQAAAAAjAQEBRSkAAAAAFWCJCgIAAAABABYAAABUb3RhbE51bWJlck9mU2NhbnNE" +
           "b25lAQF2KQAvAQA9CXYpAAAABv////8BAQEAAAAAIwEBAUcpAAAAABVgiQoCAAAAAQAZAAAAQmFja2dy" +
           "b3VuZEFjcXVpc2l0aW9uVGltZQEBeSkALwEAPQl5KQAAAA3/////AQEBAAAAACMBAQFHKQAAAAAVYIkK" +
           "AgAAAAEAEQAAAFBlbmRpbmdCYWNrZ3JvdW5kAQF8KQAvAQD9LnwpAAAACv////8BAQEAAAAAIwEBAUcp" +
           "BQAAABVgiQoCAAAAAAAHAAAARVVSYW5nZQEBgCkALgBEgCkAAAEAdAP/////AQH/////AAAAABVgiQoC" +
           "AAAAAAAQAAAARW5naW5lZXJpbmdVbml0cwEBgSkALgBEgSkAAAEAdwP/////AQH/////AAAAABVgiQoC" +
           "AAAAAAAFAAAAVGl0bGUBAYIpAC4ARIIpAAAAFf////8BAf////8AAAAAFWCJCgIAAAAAAA0AAABBeGlz" +
           "U2NhbGVUeXBlAQGDKQAuAESDKQAAAQAtL/////8BAf////8AAAAAFWCJCgIAAAAAAA8AAABYQXhpc0Rl" +
           "ZmluaXRpb24BAYQpAC4ARIQpAAABAC8v/////wEB/////wAAAAAVYIkKAgAAAAEAEgAAAFBlbmRpbmdC" +
           "YWNrZ3JvdW5kMQEBhSkALwEA/S6FKQAAAAr/////AQEBAAAAACMBAQFHKQUAAAAVYIkKAgAAAAAABwAA" +
           "AEVVUmFuZ2UBAYkpAC4ARIkpAAABAHQD/////wEB/////wAAAAAVYIkKAgAAAAAAEAAAAEVuZ2luZWVy" +
           "aW5nVW5pdHMBAYopAC4ARIopAAABAHcD/////wEB/////wAAAAAVYIkKAgAAAAAABQAAAFRpdGxlAQGL" +
           "KQAuAESLKQAAABX/////AQH/////AAAAABVgiQoCAAAAAAANAAAAQXhpc1NjYWxlVHlwZQEBjCkALgBE" +
           "jCkAAAEALS//////AQH/////AAAAABVgiQoCAAAAAAAPAAAAWEF4aXNEZWZpbml0aW9uAQGNKQAuAESN" +
           "KQAAAQAvL/////8BAf////8AAAAABGCACgEAAAABAA0AAABDb25maWd1cmF0aW9uAQE/KQAvAQLtAz8p" +
           "AAAEAAAAACMAAQHkKAAjAAEB5ygAIwABAU8pACMAAQFYKQAAAAAEYIAKAQAAAAEABgAAAFN0YXR1cwEB" +
           "QSkALwEC7QNBKQAABAAAAAAjAAEB6igAIwABAe0oACMAAQHwKAAjAAEB8ygAAAAABGCACgEAAAABABMA" +
           "AABBY3F1aXNpdGlvblNldHRpbmdzAQFDKQAvAQLtA0MpAAAHAAAAACMAAQH2KAAjAAEBYSkAIwABAWQp" +
           "ACMAAQFnKQAjAAEBaikAIwABAW0pACMAAQFwKQAAAAAEYIAKAQAAAAEAEQAAAEFjcXVpc2l0aW9uU3Rh" +
           "dHVzAQFFKQAvAQLtA0UpAAAFAAAAACMAAQH8KAAjAAEB/ygAIwABAQIpACMAAQEGKQAjAAEBcykAAAAA" +
           "BGCACgEAAAABAA8AAABBY3F1aXNpdGlvbkRhdGEBAUcpAC8BAu0DRykAAAoAAAAAIwABAQkpACMAAQEP" +
           "KQAjAAEBEikAIwABARUpACMAAQEYKQAjAAEBGykAIwABAXYpACMAAQF5KQAjAAEBfCkAIwABAYUpAAAA" +
           "AARggAoBAAAAAQAYAAAAQ2hlbW9tZXRyaWNNb2RlbFNldHRpbmdzAQFJKQAvAQLtA0kpAAD/////AAAA" +
           "AARggAoBAAAAAQAHAAAAQ29udGV4dAEBSykALwEC7QNLKQAACwAAAAAjAAEBHikAIwABASEpACMAAQEk" +
           "KQAjAAEBJykAIwABASopACMAAQEtKQAjAAEBMCkAIwABATMpACMAAQE2KQAjAAEBOSkAIwABATwpAAAA" +
           "AARggAoBAAAAAQAPAAAARmFjdG9yeVNldHRpbmdzAQGOKQAvADqOKQAA/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public BaseObjectState FactorySettings
        {
            get
            {
                return m_factorySettings;
            }

            set
            {
                if (!Object.ReferenceEquals(m_factorySettings, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_factorySettings = value;
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
            if (m_factorySettings != null)
            {
                children.Add(m_factorySettings);
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
                case Opc.Ua.Adi.BrowseNames.FactorySettings:
                {
                    if (createOrReplace)
                    {
                        if (FactorySettings == null)
                        {
                            if (replacement == null)
                            {
                                FactorySettings = new BaseObjectState(this);
                            }
                            else
                            {
                                FactorySettings = (BaseObjectState)replacement;
                            }
                        }
                    }

                    instance = FactorySettings;
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
        private BaseObjectState m_factorySettings;
        #endregion
    }
    #endif
    #endregion

    #region MassSpectrometerDeviceStreamState Class
    #if (!OPCUA_EXCLUDE_MassSpectrometerDeviceStreamState)
    /// <summary>
    /// Stores an instance of the MassSpectrometerDeviceStreamType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class MassSpectrometerDeviceStreamState : StreamState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public MassSpectrometerDeviceStreamState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.ObjectTypes.MassSpectrometerDeviceStreamType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAoAAAATWFzc1NwZWN0cm9tZXRlckRldmljZVN0cmVh" +
           "bVR5cGVJbnN0YW5jZQEBBwQBAQcEBwQAAP////8HAAAABGCACgEAAAABAA0AAABDb25maWd1cmF0aW9u" +
           "AQEAKgAvAQLtAwAqAAACAAAAACMAAQGlKQAjAAEBqCkAAAAABGCACgEAAAABAAYAAABTdGF0dXMBAQIq" +
           "AC8BAu0DAioAAAQAAAAAIwABAaspACMAAQGuKQAjAAEBsSkAIwABAbQpAAAAAARggAoBAAAAAQATAAAA" +
           "QWNxdWlzaXRpb25TZXR0aW5ncwEBBCoALwEC7QMEKgAAAQAAAAAjAAEBtykAAAAABGCACgEAAAABABEA" +
           "AABBY3F1aXNpdGlvblN0YXR1cwEBBioALwEC7QMGKgAABAAAAAAjAAEBvSkAIwABAcApACMAAQHDKQAj" +
           "AAEBxykAAAAABGCACgEAAAABAA8AAABBY3F1aXNpdGlvbkRhdGEBAQgqAC8BAu0DCCoAAAYAAAAAIwAB" +
           "AcopACMAAQHQKQAjAAEB0ykAIwABAdYpACMAAQHZKQAjAAEB3CkAAAAABGCACgEAAAABABgAAABDaGVt" +
           "b21ldHJpY01vZGVsU2V0dGluZ3MBAQoqAC8BAu0DCioAAP////8AAAAABGCACgEAAAABAAcAAABDb250" +
           "ZXh0AQEMKgAvAQLtAwwqAAALAAAAACMAAQHfKQAjAAEB4ikAIwABAeUpACMAAQHoKQAjAAEB6ykAIwAB" +
           "Ae4pACMAAQHxKQAjAAEB9CkAIwABAfcpACMAAQH6KQAjAAEB/SkAAAAA";
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

    #region ParticleSizeMonitorDeviceStreamState Class
    #if (!OPCUA_EXCLUDE_ParticleSizeMonitorDeviceStreamState)
    /// <summary>
    /// Stores an instance of the ParticleSizeMonitorDeviceStreamType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ParticleSizeMonitorDeviceStreamState : StreamState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ParticleSizeMonitorDeviceStreamState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.ObjectTypes.ParticleSizeMonitorDeviceStreamType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQArAAAAUGFydGljbGVTaXplTW9uaXRvckRldmljZVN0" +
           "cmVhbVR5cGVJbnN0YW5jZQEBCAQBAQgECAQAAP////8IAAAAJGCACgEAAAACAAwAAABQYXJhbWV0ZXJT" +
           "ZXQBARAqAwAAAAAXAAAARmxhdCBsaXN0IG9mIFBhcmFtZXRlcnMALwA6ECoAAP////8OAAAANWCJCgIA" +
           "AAABAAkAAABJc0VuYWJsZWQBASYqAwAAAAA1AAAAVHJ1ZSBpZiB0aGlzIHN0cmVhbSBtYXliZSB1c2Vk" +
           "IHRvIHBlcmZvcm0gYWNxdWlzaXRpb24ALwEAPQkmKgAAAAH/////AQEBAAAAACMBAQGBKgAAAAA1YIkK" +
           "AgAAAAEAEAAAAERpYWdub3N0aWNTdGF0dXMBASwqAwAAAAAUAAAAU3RyZWFtIGhlYWx0aCBzdGF0dXMA" +
           "LwEAPQksKgAAAQJkGP////8BAQEAAAAAIwEBAYMqAAAAADVgiQoCAAAAAQAOAAAATGFzdFNhbXBsZVRp" +
           "bWUBATUqAwAAAAAqAAAAVGltZSBhdCB3aGljaCB0aGUgbGFzdCBzYW1wbGUgd2FzIGFjcXVpcmVkAC8B" +
           "AD0JNSoAAAAN/////wEBAQAAAAAjAQEBgyoAAAAANWCJCgIAAAABAAgAAABJc0FjdGl2ZQEBPioDAAAA" +
           "ADcAAABUcnVlIGlmIHRoaXMgc3RyZWFtIGlzIGFjdHVhbGx5IHJ1bm5pbmcsIGFjcXVpcmluZyBkYXRh" +
           "AC8BAD0JPioAAAAB/////wEBAQAAAAAjAQEBhyoAAAAANWCJCgIAAAABAA4AAABFeGVjdXRpb25DeWNs" +
           "ZQEBQSoDAAAAAC4AAABJbmRpY2F0ZXMgd2hpY2ggRXhlY3V0aW9uIGN5Y2xlIGlzIGluIHByb2dyZXNz" +
           "AC8BAD0JQSoAAAEBoiT/////AQEBAAAAACMBAQGHKgAAAAA1YIkKAgAAAAEAFQAAAEV4ZWN1dGlvbkN5" +
           "Y2xlU3ViY29kZQEBRCoDAAAAADYAAABJbmRpY2F0ZXMgd2hpY2ggRXhlY3V0aW9uIGN5Y2xlIHN1YmNv" +
           "ZGUgaXMgaW4gcHJvZ3Jlc3MALwEASAlEKgAAABz/////AQEBAAAAACMBAQGHKgEAAAAXYIkKAgAAAAAA" +
           "CwAAAEVudW1TdHJpbmdzAQFHKgAuAERHKgAAABUBAAAAAQAAAAAAAAABAf////8AAAAANWCJCgIAAAAB" +
           "AAgAAABQcm9ncmVzcwEBSCoDAAAAAHQAAABJbmRpY2F0ZXMgdGhlIHByb2dyZXNzIG9mIGFuIGFjcXVp" +
           "c2l0aW9uIGluIHRlcm1zIG9mIHBlcmNlbnRhZ2Ugb2YgY29tcGxldGlvbi4gSXRzIHZhbHVlIHNoYWxs" +
           "IGJlIGJldHdlZW4gMCBhbmQgMTAwLgAvAQA9CUgqAAAACv////8BAQEAAAAAIwEBAYcqAAAAADVgiQoC" +
           "AAAAAQASAAAAQWNxdWlzaXRpb25Db3VudGVyAQFLKgMAAAAAUwAAAFNpbXBsZSBjb3VudGVyIGluY3Jl" +
           "bWVudGVkIGFmdGVyIGVhY2ggU2FtcGxpbmcgYWNxdWlzaXRpb24gcGVyZm9ybWVkIG9uIHRoaXMgU3Ry" +
           "ZWFtAC8BAEAJSyoAAAEAIQH/////AQEBAAAAACMBAQGJKgEAAAAVYIkKAgAAAAAABwAAAEVVUmFuZ2UB" +
           "AU8qAC4ARE8qAAABAHQD/////wEB/////wAAAAA1YIkKAgAAAAEAFwAAAEFjcXVpc2l0aW9uUmVzdWx0" +
           "U3RhdHVzAQFRKgMAAAAAGgAAAFF1YWxpdHkgb2YgdGhlIGFjcXVpc2l0aW9uAC8BAD0JUSoAAAEBuwv/" +
           "////AQEBAAAAACMBAQGJKgAAAAA1YIkKAgAAAAEACgAAAFNjYWxlZERhdGEBAVcqAwAAAABoAAAAU2Nh" +
           "bGVkIGRhdGEgcHJvZHVjZWQgYXMgYSByZXN1bHQgb2YgZGF0YSBhY3F1aXNpdGlvbiBvbiB0aGUgU3Ry" +
           "ZWFtIGFuZCBhcHBsaWNhdGlvbiBvZiB0aGUgYW5hbHlzZXIgbW9kZWwALwEAPQlXKgAAABj/////AQEB" +
           "AAAAACMBAQGJKgAAAAA1YIkKAgAAAAEAEgAAAEFjcXVpc2l0aW9uRW5kVGltZQEBXSoDAAAAAKcAAABU" +
           "aGUgZW5kIHRpbWUgb2YgdGhlIEFuYWx5c2VTYW1wbGUgb3IgQW5hbHlzZUNhbGlicmF0aW9uU2FtcGxl" +
           "IG9yIEFuYWx5c2VWYWxpZGF0aW9uU2FtcGxlIHN0YXRlIG9mIHRoZSBBbmFseXNlckNoYW5uZWxfT3Bl" +
           "cmF0aW5nTW9kZUV4ZWN1dGVTdWJTdGF0ZU1hY2hpbmUgc3RhdGUgbWFjaGluZQAvAQA9CV0qAAAADf//" +
           "//8BAQEAAAAAIwEBAYkqAAAAABVgiQoCAAAAAQAKAAAAQmFja2dyb3VuZAEBkSoALwEA/S6RKgAAAAr/" +
           "////AQEBAAAAACMBAQGJKgUAAAAVYIkKAgAAAAAABwAAAEVVUmFuZ2UBAZUqAC4ARJUqAAABAHQD////" +
           "/wEB/////wAAAAAVYIkKAgAAAAAAEAAAAEVuZ2luZWVyaW5nVW5pdHMBAZYqAC4ARJYqAAABAHcD////" +
           "/wEB/////wAAAAAVYIkKAgAAAAAABQAAAFRpdGxlAQGXKgAuAESXKgAAABX/////AQH/////AAAAABVg" +
           "iQoCAAAAAAANAAAAQXhpc1NjYWxlVHlwZQEBmCoALgBEmCoAAAEALS//////AQH/////AAAAABVgiQoC" +
           "AAAAAAAPAAAAWEF4aXNEZWZpbml0aW9uAQGZKgAuAESZKgAAAQAvL/////8BAf////8AAAAAFWCJCgIA" +
           "AAABABAAAABTaXplRGlzdHJpYnV0aW9uAQGaKgAvAQD9LpoqAAAACv////8BAQEAAAAAIwEBAYkqBQAA" +
           "ABVgiQoCAAAAAAAHAAAARVVSYW5nZQEBnioALgBEnioAAAEAdAP/////AQH/////AAAAABVgiQoCAAAA" +
           "AAAQAAAARW5naW5lZXJpbmdVbml0cwEBnyoALgBEnyoAAAEAdwP/////AQH/////AAAAABVgiQoCAAAA" +
           "AAAFAAAAVGl0bGUBAaAqAC4ARKAqAAAAFf////8BAf////8AAAAAFWCJCgIAAAAAAA0AAABBeGlzU2Nh" +
           "bGVUeXBlAQGhKgAuAEShKgAAAQAtL/////8BAf////8AAAAAFWCJCgIAAAAAAA8AAABYQXhpc0RlZmlu" +
           "aXRpb24BAaIqAC4ARKIqAAABAC8v/////wEB/////wAAAAAVYIkKAgAAAAEAGQAAAEJhY2tncm91bmRB" +
           "Y3F1aXNpdGlvblRpbWUBAaMqAC8BAD0JoyoAAAAN/////wEBAQAAAAAjAQEBiSoAAAAABGCACgEAAAAB" +
           "AA0AAABDb25maWd1cmF0aW9uAQGBKgAvAQLtA4EqAAACAAAAACMAAQEmKgAjAAEBKSoAAAAABGCACgEA" +
           "AAABAAYAAABTdGF0dXMBAYMqAC8BAu0DgyoAAAQAAAAAIwABASwqACMAAQEvKgAjAAEBMioAIwABATUq" +
           "AAAAAARggAoBAAAAAQATAAAAQWNxdWlzaXRpb25TZXR0aW5ncwEBhSoALwEC7QOFKgAAAQAAAAAjAAEB" +
           "OCoAAAAABGCACgEAAAABABEAAABBY3F1aXNpdGlvblN0YXR1cwEBhyoALwEC7QOHKgAABAAAAAAjAAEB" +
           "PioAIwABAUEqACMAAQFEKgAjAAEBSCoAAAAABGCACgEAAAABAA8AAABBY3F1aXNpdGlvbkRhdGEBAYkq" +
           "AC8BAu0DiSoAAAkAAAAAIwABAUsqACMAAQFRKgAjAAEBVCoAIwABAVcqACMAAQFaKgAjAAEBXSoAIwAB" +
           "AZEqACMAAQGaKgAjAAEBoyoAAAAABGCACgEAAAABABgAAABDaGVtb21ldHJpY01vZGVsU2V0dGluZ3MB" +
           "AYsqAC8BAu0DiyoAAP////8AAAAABGCACgEAAAABAAcAAABDb250ZXh0AQGNKgAvAQLtA40qAAALAAAA" +
           "ACMAAQFgKgAjAAEBYyoAIwABAWYqACMAAQFpKgAjAAEBbCoAIwABAW8qACMAAQFyKgAjAAEBdSoAIwAB" +
           "AXgqACMAAQF7KgAjAAEBfioAAAAA";
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

    #region AcousticSpectrometerDeviceStreamState Class
    #if (!OPCUA_EXCLUDE_AcousticSpectrometerDeviceStreamState)
    /// <summary>
    /// Stores an instance of the AcousticSpectrometerDeviceStreamType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class AcousticSpectrometerDeviceStreamState : StreamState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public AcousticSpectrometerDeviceStreamState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.ObjectTypes.AcousticSpectrometerDeviceStreamType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAsAAAAQWNvdXN0aWNTcGVjdHJvbWV0ZXJEZXZpY2VT" +
           "dHJlYW1UeXBlSW5zdGFuY2UBAQkEAQEJBAkEAAD/////BwAAAARggAoBAAAAAQANAAAAQ29uZmlndXJh" +
           "dGlvbgEBFysALwEC7QMXKwAAAgAAAAAjAAEBvCoAIwABAb8qAAAAAARggAoBAAAAAQAGAAAAU3RhdHVz" +
           "AQEZKwAvAQLtAxkrAAAEAAAAACMAAQHCKgAjAAEBxSoAIwABAcgqACMAAQHLKgAAAAAEYIAKAQAAAAEA" +
           "EwAAAEFjcXVpc2l0aW9uU2V0dGluZ3MBARsrAC8BAu0DGysAAAEAAAAAIwABAc4qAAAAAARggAoBAAAA" +
           "AQARAAAAQWNxdWlzaXRpb25TdGF0dXMBAR0rAC8BAu0DHSsAAAQAAAAAIwABAdQqACMAAQHXKgAjAAEB" +
           "2ioAIwABAd4qAAAAAARggAoBAAAAAQAPAAAAQWNxdWlzaXRpb25EYXRhAQEfKwAvAQLtAx8rAAAGAAAA" +
           "ACMAAQHhKgAjAAEB5yoAIwABAeoqACMAAQHtKgAjAAEB8CoAIwABAfMqAAAAAARggAoBAAAAAQAYAAAA" +
           "Q2hlbW9tZXRyaWNNb2RlbFNldHRpbmdzAQEhKwAvAQLtAyErAAD/////AAAAAARggAoBAAAAAQAHAAAA" +
           "Q29udGV4dAEBIysALwEC7QMjKwAACwAAAAAjAAEB9ioAIwABAfkqACMAAQH8KgAjAAEB/yoAIwABAQIr" +
           "ACMAAQEFKwAjAAEBCCsAIwABAQsrACMAAQEOKwAjAAEBESsAIwABARQrAAAAAA==";
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

    #region ChromatographDeviceStreamState Class
    #if (!OPCUA_EXCLUDE_ChromatographDeviceStreamState)
    /// <summary>
    /// Stores an instance of the ChromatographDeviceStreamType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ChromatographDeviceStreamState : StreamState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ChromatographDeviceStreamState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.ObjectTypes.ChromatographDeviceStreamType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAlAAAAQ2hyb21hdG9ncmFwaERldmljZVN0cmVhbVR5" +
           "cGVJbnN0YW5jZQEBCgQBAQoECgQAAP////8HAAAABGCACgEAAAABAA0AAABDb25maWd1cmF0aW9uAQGY" +
           "KwAvAQLtA5grAAACAAAAACMAAQE9KwAjAAEBQCsAAAAABGCACgEAAAABAAYAAABTdGF0dXMBAZorAC8B" +
           "Au0DmisAAAQAAAAAIwABAUMrACMAAQFGKwAjAAEBSSsAIwABAUwrAAAAAARggAoBAAAAAQATAAAAQWNx" +
           "dWlzaXRpb25TZXR0aW5ncwEBnCsALwEC7QOcKwAAAQAAAAAjAAEBTysAAAAABGCACgEAAAABABEAAABB" +
           "Y3F1aXNpdGlvblN0YXR1cwEBnisALwEC7QOeKwAABAAAAAAjAAEBVSsAIwABAVgrACMAAQFbKwAjAAEB" +
           "XysAAAAABGCACgEAAAABAA8AAABBY3F1aXNpdGlvbkRhdGEBAaArAC8BAu0DoCsAAAYAAAAAIwABAWIr" +
           "ACMAAQFoKwAjAAEBaysAIwABAW4rACMAAQFxKwAjAAEBdCsAAAAABGCACgEAAAABABgAAABDaGVtb21l" +
           "dHJpY01vZGVsU2V0dGluZ3MBAaIrAC8BAu0DoisAAP////8AAAAABGCACgEAAAABAAcAAABDb250ZXh0" +
           "AQGkKwAvAQLtA6QrAAALAAAAACMAAQF3KwAjAAEBeisAIwABAX0rACMAAQGAKwAjAAEBgysAIwABAYYr" +
           "ACMAAQGJKwAjAAEBjCsAIwABAY8rACMAAQGSKwAjAAEBlSsAAAAA";
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

    #region MNRDeviceStreamState Class
    #if (!OPCUA_EXCLUDE_MNRDeviceStreamState)
    /// <summary>
    /// Stores an instance of the MNRDeviceStreamType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class MNRDeviceStreamState : StreamState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public MNRDeviceStreamState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.ObjectTypes.MNRDeviceStreamType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAbAAAATU5SRGV2aWNlU3RyZWFtVHlwZUluc3RhbmNl" +
           "AQELBAEBCwQLBAAA/////wcAAAAEYIAKAQAAAAEADQAAAENvbmZpZ3VyYXRpb24BARksAC8BAu0DGSwA" +
           "AAIAAAAAIwABAb4rACMAAQHBKwAAAAAEYIAKAQAAAAEABgAAAFN0YXR1cwEBGywALwEC7QMbLAAABAAA" +
           "AAAjAAEBxCsAIwABAccrACMAAQHKKwAjAAEBzSsAAAAABGCACgEAAAABABMAAABBY3F1aXNpdGlvblNl" +
           "dHRpbmdzAQEdLAAvAQLtAx0sAAABAAAAACMAAQHQKwAAAAAEYIAKAQAAAAEAEQAAAEFjcXVpc2l0aW9u" +
           "U3RhdHVzAQEfLAAvAQLtAx8sAAAEAAAAACMAAQHWKwAjAAEB2SsAIwABAdwrACMAAQHgKwAAAAAEYIAK" +
           "AQAAAAEADwAAAEFjcXVpc2l0aW9uRGF0YQEBISwALwEC7QMhLAAABgAAAAAjAAEB4ysAIwABAekrACMA" +
           "AQHsKwAjAAEB7ysAIwABAfIrACMAAQH1KwAAAAAEYIAKAQAAAAEAGAAAAENoZW1vbWV0cmljTW9kZWxT" +
           "ZXR0aW5ncwEBIywALwEC7QMjLAAA/////wAAAAAEYIAKAQAAAAEABwAAAENvbnRleHQBASUsAC8BAu0D" +
           "JSwAAAsAAAAAIwABAfgrACMAAQH7KwAjAAEB/isAIwABAQEsACMAAQEELAAjAAEBBywAIwABAQosACMA" +
           "AQENLAAjAAEBECwAIwABARMsACMAAQEWLAAAAAA=";
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

    #region SpectrometerDeviceState Class
    #if (!OPCUA_EXCLUDE_SpectrometerDeviceState)
    /// <summary>
    /// Stores an instance of the SpectrometerDeviceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class SpectrometerDeviceState : AnalyserDeviceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public SpectrometerDeviceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.ObjectTypes.SpectrometerDeviceType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////4RggAIBAAAAAQAeAAAAU3BlY3Ryb21ldGVyRGV2aWNlVHlwZUluc3Rh" +
           "bmNlAQHzAwEB8wPzAwAAAf////8PAAAAJGCACgEAAAACAAwAAABQYXJhbWV0ZXJTZXQBASksAwAAAAAX" +
           "AAAARmxhdCBsaXN0IG9mIFBhcmFtZXRlcnMALwA6KSwAAP////8CAAAANWCJCgIAAAABABAAAABEaWFn" +
           "bm9zdGljU3RhdHVzAQF4LAMAAAAAJQAAAEdlbmVyYWwgaGVhbHRoIHN0YXR1cyBvZiB0aGUgYW5hbHlz" +
           "ZXIALwEAPQl4LAAAAQJkGP////8BAQEAAAAAIwEBAZEsAAAAABdgiQoCAAAAAQANAAAAU3BlY3RyYWxS" +
           "YW5nZQEBHy0ALwEAPQkfLQAAAQB0AwEAAAABAAAAAAAAAAEB/////wAAAAAkYIAKAQAAAAIACQAAAE1l" +
           "dGhvZFNldAEBKywDAAAAABQAAABGbGF0IGxpc3Qgb2YgTWV0aG9kcwAvADorLAAA/////woAAAAEYYIK" +
           "BAAAAAEAEAAAAEdldENvbmZpZ3VyYXRpb24BAWgsAC8BAeMkaCwAAAEB/////wEAAAAXYKkKAgAAAAAA" +
           "DwAAAE91dHB1dEFyZ3VtZW50cwEBaSwALgBEaSwAAJYBAAAAAQAqAQEZAAAACgAAAENvbmZpZ0RhdGEA" +
           "D/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAABGGCCgQAAAABABAAAABTZXRDb25maWd1" +
           "cmF0aW9uAQFqLAAvAQHlJGosAAABAf////8CAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEB" +
           "aywALgBEaywAAJYBAAAAAQAqAQEZAAAACgAAAENvbmZpZ0RhdGEAD/////8AAAAAAAEAKAEBAAAAAQAA" +
           "AAAAAAABAf////8AAAAAF2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1bWVudHMBAWwsAC4ARGwsAACWAQAA" +
           "AAEAKgEBHwAAABAAAABDb25maWdEYXRhRGlnZXN0AAz/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/" +
           "////AAAAAARhggoEAAAAAQATAAAAR2V0Q29uZmlnRGF0YURpZ2VzdAEBbSwALwEB6CRtLAAAAQH/////" +
           "AQAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQFuLAAuAERuLAAAlgEAAAABACoBAR8AAAAQ" +
           "AAAAQ29uZmlnRGF0YURpZ2VzdAAM/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAEYYIK" +
           "BAAAAAEAFwAAAENvbXBhcmVDb25maWdEYXRhRGlnZXN0AQFvLAAvAQHqJG8sAAABAf////8CAAAAF2Cp" +
           "CgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBcCwALgBEcCwAAJYBAAAAAQAqAQEfAAAAEAAAAENvbmZp" +
           "Z0RhdGFEaWdlc3QADP////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIAAAAAAA8A" +
           "AABPdXRwdXRBcmd1bWVudHMBAXEsAC4ARHEsAACWAQAAAAEAKgEBFgAAAAcAAABJc0VxdWFsAAH/////" +
           "AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAACRhggoEAAAAAQAQAAAAUmVzZXRBbGxDaGFubmVs" +
           "cwEBciwDAAAAADwAAABSZXNldCBhbGwgQW5hbHlzZXJDaGFubmVscyBiZWxvbmdpbmcgdG8gdGhpcyBB" +
           "bmFseXNlckRldmljZS4ALwEB7SRyLAAAAQH/////AAAAACRhggoEAAAAAQAQAAAAU3RhcnRBbGxDaGFu" +
           "bmVscwEBcywDAAAAADwAAABTdGFydCBhbGwgQW5hbHlzZXJDaGFubmVscyBiZWxvbmdpbmcgdG8gdGhp" +
           "cyBBbmFseXNlckRldmljZS4ALwEB7iRzLAAAAQH/////AAAAACRhggoEAAAAAQAPAAAAU3RvcEFsbENo" +
           "YW5uZWxzAQF0LAMAAAAAOwAAAFN0b3AgYWxsIEFuYWx5c2VyQ2hhbm5lbHMgYmVsb25naW5nIHRvIHRo" +
           "aXMgQW5hbHlzZXJEZXZpY2UuAC8BAe8kdCwAAAEB/////wAAAAAkYYIKBAAAAAEAEAAAAEFib3J0QWxs" +
           "Q2hhbm5lbHMBAXUsAwAAAAA8AAAAQWJvcnQgYWxsIEFuYWx5c2VyQ2hhbm5lbHMgYmVsb25naW5nIHRv" +
           "IHRoaXMgQW5hbHlzZXJEZXZpY2UuAC8BAfAkdSwAAAEB/////wAAAAAkYYIKBAAAAAEADQAAAEdvdG9P" +
           "cGVyYXRpbmcBAXYsAwAAAACNAAAAQW5hbHlzZXJEZXZpY2VTdGF0ZU1hY2hpbmUgdG8gZ28gdG8gT3Bl" +
           "cmF0aW5nIHN0YXRlLCBmb3JjaW5nIGFsbCBBbmFseXNlckNoYW5uZWxzIHRvIGxlYXZlIHRoZSBTbGF2" +
           "ZU1vZGUgc3RhdGUgYW5kIGdvIHRvIHRoZSBPcGVyYXRpbmcgc3RhdGUuAC8BAfEkdiwAAAEB/////wAA" +
           "AAAkYYIKBAAAAAEADwAAAEdvdG9NYWludGVuYW5jZQEBdywDAAAAAGcAAABBbmFseXNlckRldmljZVN0" +
           "YXRlTWFjaGluZSB0byBnbyB0byBNYWludGVuYW5jZSBzdGF0ZSwgZm9yY2luZyBhbGwgQW5hbHlzZXJD" +
           "aGFubmVscyB0byBTbGF2ZU1vZGUgc3RhdGUuAC8BAfIkdywAAAEB/////wAAAAAkYIAKAQAAAAIADgAA" +
           "AElkZW50aWZpY2F0aW9uAQEvLAMAAAAARgAAAFVzZWQgdG8gb3JnYW5pemUgcGFyYW1ldGVycyBmb3Ig" +
           "aWRlbnRpZmljYXRpb24gb2YgdGhpcyBUb3BvbG9neUVsZW1lbnQALwEC7QMvLAAAAwAAAAAjAAECcxcA" +
           "IwABAnQXACMAAQJxFwAAAAA1YIkKAgAAAAIADAAAAE1hbnVmYWN0dXJlcgEBQSwDAAAAADAAAABOYW1l" +
           "IG9mIHRoZSBjb21wYW55IHRoYXQgbWFudWZhY3R1cmVkIHRoZSBkZXZpY2UALgBEQSwAAAAV/////wEB" +
           "/////wAAAAA1YIkKAgAAAAIABQAAAE1vZGVsAQFCLAMAAAAAGAAAAE1vZGVsIG5hbWUgb2YgdGhlIGRl" +
           "dmljZQAuAERCLAAAABX/////AQH/////AAAAADVgiQoCAAAAAgAQAAAASGFyZHdhcmVSZXZpc2lvbgEB" +
           "RiwDAAAAACwAAABSZXZpc2lvbiBsZXZlbCBvZiB0aGUgaGFyZHdhcmUgb2YgdGhlIGRldmljZQAuAERG" +
           "LAAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAQAAAAU29mdHdhcmVSZXZpc2lvbgEBRSwDAAAAADUA" +
           "AABSZXZpc2lvbiBsZXZlbCBvZiB0aGUgc29mdHdhcmUvZmlybXdhcmUgb2YgdGhlIGRldmljZQAuAERF" +
           "LAAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAOAAAARGV2aWNlUmV2aXNpb24BAUQsAwAAAAAkAAAA" +
           "T3ZlcmFsbCByZXZpc2lvbiBsZXZlbCBvZiB0aGUgZGV2aWNlAC4AREQsAAAADP////8BAf////8AAAAA" +
           "NWCJCgIAAAACAAwAAABEZXZpY2VNYW51YWwBAUMsAwAAAABaAAAAQWRkcmVzcyAocGF0aG5hbWUgaW4g" +
           "dGhlIGZpbGUgc3lzdGVtIG9yIGEgVVJMIHwgV2ViIGFkZHJlc3MpIG9mIHVzZXIgbWFudWFsIGZvciB0" +
           "aGUgZGV2aWNlAC4AREMsAAAADP////8BAf////8AAAAANWCJCgIAAAACAAwAAABTZXJpYWxOdW1iZXIB" +
           "AT8sAwAAAABNAAAASWRlbnRpZmllciB0aGF0IHVuaXF1ZWx5IGlkZW50aWZpZXMsIHdpdGhpbiBhIG1h" +
           "bnVmYWN0dXJlciwgYSBkZXZpY2UgaW5zdGFuY2UALgBEPywAAAAM/////wEB/////wAAAAA1YIkKAgAA" +
           "AAIADwAAAFJldmlzaW9uQ291bnRlcgEBQCwDAAAAAGkAAABBbiBpbmNyZW1lbnRhbCBjb3VudGVyIGlu" +
           "ZGljYXRpbmcgdGhlIG51bWJlciBvZiB0aW1lcyB0aGUgc3RhdGljIGRhdGEgd2l0aGluIHRoZSBEZXZp" +
           "Y2UgaGFzIGJlZW4gbW9kaWZpZWQALgBEQCwAAAAG/////wEB/////wAAAAAEYIAKAQAAAAEADQAAAENv" +
           "bmZpZ3VyYXRpb24BAY8sAC8BAu0DjywAAAEAAAAAIwABAXssAAAAAARggAoBAAAAAQAGAAAAU3RhdHVz" +
           "AQGRLAAvAQLtA5EsAAABAAAAACMAAQF4LAAAAAAEYIAKAQAAAAEADwAAAEZhY3RvcnlTZXR0aW5ncwEB" +
           "kywALwEC7QOTLAAA/////wAAAAAEYIAKAQAAAAEAFAAAAEFuYWx5c2VyU3RhdGVNYWNoaW5lAQGVLAAv" +
           "AQHqA5UsAAD/////AQAAABVgiQoCAAAAAAAMAAAAQ3VycmVudFN0YXRlAQGWLAAvAQDICpYsAAAAFf//" +
           "//8BAf////8BAAAAFWCJCgIAAAAAAAIAAABJZAEBlywALgBElywAAAAR/////wEB/////wAAAAA=";
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

    #region ParticleSizeMonitorDeviceState Class
    #if (!OPCUA_EXCLUDE_ParticleSizeMonitorDeviceState)
    /// <summary>
    /// Stores an instance of the ParticleSizeMonitorDeviceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ParticleSizeMonitorDeviceState : AnalyserDeviceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ParticleSizeMonitorDeviceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.ObjectTypes.ParticleSizeMonitorDeviceType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////4RggAIBAAAAAQAlAAAAUGFydGljbGVTaXplTW9uaXRvckRldmljZVR5" +
           "cGVJbnN0YW5jZQEB9AMBAfQD9AMAAAH/////DgAAACRggAoBAAAAAgAJAAAATWV0aG9kU2V0AQEkLQMA" +
           "AAAAFAAAAEZsYXQgbGlzdCBvZiBNZXRob2RzAC8AOiQtAAD/////CgAAAARhggoEAAAAAQAQAAAAR2V0" +
           "Q29uZmlndXJhdGlvbgEBYS0ALwEB4yRhLQAAAQH/////AQAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJn" +
           "dW1lbnRzAQFiLQAuAERiLQAAlgEAAAABACoBARkAAAAKAAAAQ29uZmlnRGF0YQAP/////wAAAAAAAQAo" +
           "AQEAAAABAAAAAAAAAAEB/////wAAAAAEYYIKBAAAAAEAEAAAAFNldENvbmZpZ3VyYXRpb24BAWMtAC8B" +
           "AeUkYy0AAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElucHV0QXJndW1lbnRzAQFkLQAuAERkLQAAlgEA" +
           "AAABACoBARkAAAAKAAAAQ29uZmlnRGF0YQAP/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAA" +
           "AAAXYKkKAgAAAAAADwAAAE91dHB1dEFyZ3VtZW50cwEBZS0ALgBEZS0AAJYBAAAAAQAqAQEfAAAAEAAA" +
           "AENvbmZpZ0RhdGFEaWdlc3QADP////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAABGGCCgQA" +
           "AAABABMAAABHZXRDb25maWdEYXRhRGlnZXN0AQFmLQAvAQHoJGYtAAABAf////8BAAAAF2CpCgIAAAAA" +
           "AA8AAABPdXRwdXRBcmd1bWVudHMBAWctAC4ARGctAACWAQAAAAEAKgEBHwAAABAAAABDb25maWdEYXRh" +
           "RGlnZXN0AAz/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAAARhggoEAAAAAQAXAAAAQ29t" +
           "cGFyZUNvbmZpZ0RhdGFEaWdlc3QBAWgtAC8BAeokaC0AAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElu" +
           "cHV0QXJndW1lbnRzAQFpLQAuAERpLQAAlgEAAAABACoBAR8AAAAQAAAAQ29uZmlnRGF0YURpZ2VzdAAM" +
           "/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAXYKkKAgAAAAAADwAAAE91dHB1dEFyZ3Vt" +
           "ZW50cwEBai0ALgBEai0AAJYBAAAAAQAqAQEWAAAABwAAAElzRXF1YWwAAf////8AAAAAAAEAKAEBAAAA" +
           "AQAAAAAAAAABAf////8AAAAAJGGCCgQAAAABABAAAABSZXNldEFsbENoYW5uZWxzAQFrLQMAAAAAPAAA" +
           "AFJlc2V0IGFsbCBBbmFseXNlckNoYW5uZWxzIGJlbG9uZ2luZyB0byB0aGlzIEFuYWx5c2VyRGV2aWNl" +
           "LgAvAQHtJGstAAABAf////8AAAAAJGGCCgQAAAABABAAAABTdGFydEFsbENoYW5uZWxzAQFsLQMAAAAA" +
           "PAAAAFN0YXJ0IGFsbCBBbmFseXNlckNoYW5uZWxzIGJlbG9uZ2luZyB0byB0aGlzIEFuYWx5c2VyRGV2" +
           "aWNlLgAvAQHuJGwtAAABAf////8AAAAAJGGCCgQAAAABAA8AAABTdG9wQWxsQ2hhbm5lbHMBAW0tAwAA" +
           "AAA7AAAAU3RvcCBhbGwgQW5hbHlzZXJDaGFubmVscyBiZWxvbmdpbmcgdG8gdGhpcyBBbmFseXNlckRl" +
           "dmljZS4ALwEB7yRtLQAAAQH/////AAAAACRhggoEAAAAAQAQAAAAQWJvcnRBbGxDaGFubmVscwEBbi0D" +
           "AAAAADwAAABBYm9ydCBhbGwgQW5hbHlzZXJDaGFubmVscyBiZWxvbmdpbmcgdG8gdGhpcyBBbmFseXNl" +
           "ckRldmljZS4ALwEB8CRuLQAAAQH/////AAAAACRhggoEAAAAAQANAAAAR290b09wZXJhdGluZwEBby0D" +
           "AAAAAI0AAABBbmFseXNlckRldmljZVN0YXRlTWFjaGluZSB0byBnbyB0byBPcGVyYXRpbmcgc3RhdGUs" +
           "IGZvcmNpbmcgYWxsIEFuYWx5c2VyQ2hhbm5lbHMgdG8gbGVhdmUgdGhlIFNsYXZlTW9kZSBzdGF0ZSBh" +
           "bmQgZ28gdG8gdGhlIE9wZXJhdGluZyBzdGF0ZS4ALwEB8SRvLQAAAQH/////AAAAACRhggoEAAAAAQAP" +
           "AAAAR290b01haW50ZW5hbmNlAQFwLQMAAAAAZwAAAEFuYWx5c2VyRGV2aWNlU3RhdGVNYWNoaW5lIHRv" +
           "IGdvIHRvIE1haW50ZW5hbmNlIHN0YXRlLCBmb3JjaW5nIGFsbCBBbmFseXNlckNoYW5uZWxzIHRvIFNs" +
           "YXZlTW9kZSBzdGF0ZS4ALwEB8iRwLQAAAQH/////AAAAACRggAoBAAAAAgAOAAAASWRlbnRpZmljYXRp" +
           "b24BASgtAwAAAABGAAAAVXNlZCB0byBvcmdhbml6ZSBwYXJhbWV0ZXJzIGZvciBpZGVudGlmaWNhdGlv" +
           "biBvZiB0aGlzIFRvcG9sb2d5RWxlbWVudAAvAQLtAygtAAADAAAAACMAAQJzFwAjAAECdBcAIwABAnEX" +
           "AAAAADVgiQoCAAAAAgAMAAAATWFudWZhY3R1cmVyAQE6LQMAAAAAMAAAAE5hbWUgb2YgdGhlIGNvbXBh" +
           "bnkgdGhhdCBtYW51ZmFjdHVyZWQgdGhlIGRldmljZQAuAEQ6LQAAABX/////AQH/////AAAAADVgiQoC" +
           "AAAAAgAFAAAATW9kZWwBATstAwAAAAAYAAAATW9kZWwgbmFtZSBvZiB0aGUgZGV2aWNlAC4ARDstAAAA" +
           "Ff////8BAf////8AAAAANWCJCgIAAAACABAAAABIYXJkd2FyZVJldmlzaW9uAQE/LQMAAAAALAAAAFJl" +
           "dmlzaW9uIGxldmVsIG9mIHRoZSBoYXJkd2FyZSBvZiB0aGUgZGV2aWNlAC4ARD8tAAAADP////8BAf//" +
           "//8AAAAANWCJCgIAAAACABAAAABTb2Z0d2FyZVJldmlzaW9uAQE+LQMAAAAANQAAAFJldmlzaW9uIGxl" +
           "dmVsIG9mIHRoZSBzb2Z0d2FyZS9maXJtd2FyZSBvZiB0aGUgZGV2aWNlAC4ARD4tAAAADP////8BAf//" +
           "//8AAAAANWCJCgIAAAACAA4AAABEZXZpY2VSZXZpc2lvbgEBPS0DAAAAACQAAABPdmVyYWxsIHJldmlz" +
           "aW9uIGxldmVsIG9mIHRoZSBkZXZpY2UALgBEPS0AAAAM/////wEB/////wAAAAA1YIkKAgAAAAIADAAA" +
           "AERldmljZU1hbnVhbAEBPC0DAAAAAFoAAABBZGRyZXNzIChwYXRobmFtZSBpbiB0aGUgZmlsZSBzeXN0" +
           "ZW0gb3IgYSBVUkwgfCBXZWIgYWRkcmVzcykgb2YgdXNlciBtYW51YWwgZm9yIHRoZSBkZXZpY2UALgBE" +
           "PC0AAAAM/////wEB/////wAAAAA1YIkKAgAAAAIADAAAAFNlcmlhbE51bWJlcgEBOC0DAAAAAE0AAABJ" +
           "ZGVudGlmaWVyIHRoYXQgdW5pcXVlbHkgaWRlbnRpZmllcywgd2l0aGluIGEgbWFudWZhY3R1cmVyLCBh" +
           "IGRldmljZSBpbnN0YW5jZQAuAEQ4LQAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAPAAAAUmV2aXNp" +
           "b25Db3VudGVyAQE5LQMAAAAAaQAAAEFuIGluY3JlbWVudGFsIGNvdW50ZXIgaW5kaWNhdGluZyB0aGUg" +
           "bnVtYmVyIG9mIHRpbWVzIHRoZSBzdGF0aWMgZGF0YSB3aXRoaW4gdGhlIERldmljZSBoYXMgYmVlbiBt" +
           "b2RpZmllZAAuAEQ5LQAAAAb/////AQH/////AAAAAARggAoBAAAAAQANAAAAQ29uZmlndXJhdGlvbgEB" +
           "iC0ALwEC7QOILQAAAQAAAAAjAAEBdC0AAAAABGCACgEAAAABAAYAAABTdGF0dXMBAYotAC8BAu0Dii0A" +
           "AAEAAAAAIwABAXEtAAAAAARggAoBAAAAAQAPAAAARmFjdG9yeVNldHRpbmdzAQGMLQAvAQLtA4wtAAD/" +
           "////AAAAAARggAoBAAAAAQAUAAAAQW5hbHlzZXJTdGF0ZU1hY2hpbmUBAY4tAC8BAeoDji0AAP////8B" +
           "AAAAFWCJCgIAAAAAAAwAAABDdXJyZW50U3RhdGUBAY8tAC8BAMgKjy0AAAAV/////wEB/////wEAAAAV" +
           "YIkKAgAAAAAAAgAAAElkAQGQLQAuAESQLQAAABH/////AQH/////AAAAAA==";
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

    #region ChromatographDeviceState Class
    #if (!OPCUA_EXCLUDE_ChromatographDeviceState)
    /// <summary>
    /// Stores an instance of the ChromatographDeviceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ChromatographDeviceState : AnalyserDeviceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ChromatographDeviceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.ObjectTypes.ChromatographDeviceType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////4RggAIBAAAAAQAfAAAAQ2hyb21hdG9ncmFwaERldmljZVR5cGVJbnN0" +
           "YW5jZQEB9QMBAfUD9QMAAAH/////DgAAACRggAoBAAAAAgAJAAAATWV0aG9kU2V0AQEaLgMAAAAAFAAA" +
           "AEZsYXQgbGlzdCBvZiBNZXRob2RzAC8AOhouAAD/////CgAAAARhggoEAAAAAQAQAAAAR2V0Q29uZmln" +
           "dXJhdGlvbgEBVy4ALwEB4yRXLgAAAQH/////AQAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRz" +
           "AQFYLgAuAERYLgAAlgEAAAABACoBARkAAAAKAAAAQ29uZmlnRGF0YQAP/////wAAAAAAAQAoAQEAAAAB" +
           "AAAAAAAAAAEB/////wAAAAAEYYIKBAAAAAEAEAAAAFNldENvbmZpZ3VyYXRpb24BAVkuAC8BAeUkWS4A" +
           "AAEB/////wIAAAAXYKkKAgAAAAAADgAAAElucHV0QXJndW1lbnRzAQFaLgAuAERaLgAAlgEAAAABACoB" +
           "ARkAAAAKAAAAQ29uZmlnRGF0YQAP/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAXYKkK" +
           "AgAAAAAADwAAAE91dHB1dEFyZ3VtZW50cwEBWy4ALgBEWy4AAJYBAAAAAQAqAQEfAAAAEAAAAENvbmZp" +
           "Z0RhdGFEaWdlc3QADP////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAABGGCCgQAAAABABMA" +
           "AABHZXRDb25maWdEYXRhRGlnZXN0AQFcLgAvAQHoJFwuAAABAf////8BAAAAF2CpCgIAAAAAAA8AAABP" +
           "dXRwdXRBcmd1bWVudHMBAV0uAC4ARF0uAACWAQAAAAEAKgEBHwAAABAAAABDb25maWdEYXRhRGlnZXN0" +
           "AAz/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAAARhggoEAAAAAQAXAAAAQ29tcGFyZUNv" +
           "bmZpZ0RhdGFEaWdlc3QBAV4uAC8BAeokXi4AAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElucHV0QXJn" +
           "dW1lbnRzAQFfLgAuAERfLgAAlgEAAAABACoBAR8AAAAQAAAAQ29uZmlnRGF0YURpZ2VzdAAM/////wAA" +
           "AAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAXYKkKAgAAAAAADwAAAE91dHB1dEFyZ3VtZW50cwEB" +
           "YC4ALgBEYC4AAJYBAAAAAQAqAQEWAAAABwAAAElzRXF1YWwAAf////8AAAAAAAEAKAEBAAAAAQAAAAAA" +
           "AAABAf////8AAAAAJGGCCgQAAAABABAAAABSZXNldEFsbENoYW5uZWxzAQFhLgMAAAAAPAAAAFJlc2V0" +
           "IGFsbCBBbmFseXNlckNoYW5uZWxzIGJlbG9uZ2luZyB0byB0aGlzIEFuYWx5c2VyRGV2aWNlLgAvAQHt" +
           "JGEuAAABAf////8AAAAAJGGCCgQAAAABABAAAABTdGFydEFsbENoYW5uZWxzAQFiLgMAAAAAPAAAAFN0" +
           "YXJ0IGFsbCBBbmFseXNlckNoYW5uZWxzIGJlbG9uZ2luZyB0byB0aGlzIEFuYWx5c2VyRGV2aWNlLgAv" +
           "AQHuJGIuAAABAf////8AAAAAJGGCCgQAAAABAA8AAABTdG9wQWxsQ2hhbm5lbHMBAWMuAwAAAAA7AAAA" +
           "U3RvcCBhbGwgQW5hbHlzZXJDaGFubmVscyBiZWxvbmdpbmcgdG8gdGhpcyBBbmFseXNlckRldmljZS4A" +
           "LwEB7yRjLgAAAQH/////AAAAACRhggoEAAAAAQAQAAAAQWJvcnRBbGxDaGFubmVscwEBZC4DAAAAADwA" +
           "AABBYm9ydCBhbGwgQW5hbHlzZXJDaGFubmVscyBiZWxvbmdpbmcgdG8gdGhpcyBBbmFseXNlckRldmlj" +
           "ZS4ALwEB8CRkLgAAAQH/////AAAAACRhggoEAAAAAQANAAAAR290b09wZXJhdGluZwEBZS4DAAAAAI0A" +
           "AABBbmFseXNlckRldmljZVN0YXRlTWFjaGluZSB0byBnbyB0byBPcGVyYXRpbmcgc3RhdGUsIGZvcmNp" +
           "bmcgYWxsIEFuYWx5c2VyQ2hhbm5lbHMgdG8gbGVhdmUgdGhlIFNsYXZlTW9kZSBzdGF0ZSBhbmQgZ28g" +
           "dG8gdGhlIE9wZXJhdGluZyBzdGF0ZS4ALwEB8SRlLgAAAQH/////AAAAACRhggoEAAAAAQAPAAAAR290" +
           "b01haW50ZW5hbmNlAQFmLgMAAAAAZwAAAEFuYWx5c2VyRGV2aWNlU3RhdGVNYWNoaW5lIHRvIGdvIHRv" +
           "IE1haW50ZW5hbmNlIHN0YXRlLCBmb3JjaW5nIGFsbCBBbmFseXNlckNoYW5uZWxzIHRvIFNsYXZlTW9k" +
           "ZSBzdGF0ZS4ALwEB8iRmLgAAAQH/////AAAAACRggAoBAAAAAgAOAAAASWRlbnRpZmljYXRpb24BAR4u" +
           "AwAAAABGAAAAVXNlZCB0byBvcmdhbml6ZSBwYXJhbWV0ZXJzIGZvciBpZGVudGlmaWNhdGlvbiBvZiB0" +
           "aGlzIFRvcG9sb2d5RWxlbWVudAAvAQLtAx4uAAADAAAAACMAAQJzFwAjAAECdBcAIwABAnEXAAAAADVg" +
           "iQoCAAAAAgAMAAAATWFudWZhY3R1cmVyAQEwLgMAAAAAMAAAAE5hbWUgb2YgdGhlIGNvbXBhbnkgdGhh" +
           "dCBtYW51ZmFjdHVyZWQgdGhlIGRldmljZQAuAEQwLgAAABX/////AQH/////AAAAADVgiQoCAAAAAgAF" +
           "AAAATW9kZWwBATEuAwAAAAAYAAAATW9kZWwgbmFtZSBvZiB0aGUgZGV2aWNlAC4ARDEuAAAAFf////8B" +
           "Af////8AAAAANWCJCgIAAAACABAAAABIYXJkd2FyZVJldmlzaW9uAQE1LgMAAAAALAAAAFJldmlzaW9u" +
           "IGxldmVsIG9mIHRoZSBoYXJkd2FyZSBvZiB0aGUgZGV2aWNlAC4ARDUuAAAADP////8BAf////8AAAAA" +
           "NWCJCgIAAAACABAAAABTb2Z0d2FyZVJldmlzaW9uAQE0LgMAAAAANQAAAFJldmlzaW9uIGxldmVsIG9m" +
           "IHRoZSBzb2Z0d2FyZS9maXJtd2FyZSBvZiB0aGUgZGV2aWNlAC4ARDQuAAAADP////8BAf////8AAAAA" +
           "NWCJCgIAAAACAA4AAABEZXZpY2VSZXZpc2lvbgEBMy4DAAAAACQAAABPdmVyYWxsIHJldmlzaW9uIGxl" +
           "dmVsIG9mIHRoZSBkZXZpY2UALgBEMy4AAAAM/////wEB/////wAAAAA1YIkKAgAAAAIADAAAAERldmlj" +
           "ZU1hbnVhbAEBMi4DAAAAAFoAAABBZGRyZXNzIChwYXRobmFtZSBpbiB0aGUgZmlsZSBzeXN0ZW0gb3Ig" +
           "YSBVUkwgfCBXZWIgYWRkcmVzcykgb2YgdXNlciBtYW51YWwgZm9yIHRoZSBkZXZpY2UALgBEMi4AAAAM" +
           "/////wEB/////wAAAAA1YIkKAgAAAAIADAAAAFNlcmlhbE51bWJlcgEBLi4DAAAAAE0AAABJZGVudGlm" +
           "aWVyIHRoYXQgdW5pcXVlbHkgaWRlbnRpZmllcywgd2l0aGluIGEgbWFudWZhY3R1cmVyLCBhIGRldmlj" +
           "ZSBpbnN0YW5jZQAuAEQuLgAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAPAAAAUmV2aXNpb25Db3Vu" +
           "dGVyAQEvLgMAAAAAaQAAAEFuIGluY3JlbWVudGFsIGNvdW50ZXIgaW5kaWNhdGluZyB0aGUgbnVtYmVy" +
           "IG9mIHRpbWVzIHRoZSBzdGF0aWMgZGF0YSB3aXRoaW4gdGhlIERldmljZSBoYXMgYmVlbiBtb2RpZmll" +
           "ZAAuAEQvLgAAAAb/////AQH/////AAAAAARggAoBAAAAAQANAAAAQ29uZmlndXJhdGlvbgEBfi4ALwEC" +
           "7QN+LgAAAQAAAAAjAAEBai4AAAAABGCACgEAAAABAAYAAABTdGF0dXMBAYAuAC8BAu0DgC4AAAEAAAAA" +
           "IwABAWcuAAAAAARggAoBAAAAAQAPAAAARmFjdG9yeVNldHRpbmdzAQGCLgAvAQLtA4IuAAD/////AAAA" +
           "AARggAoBAAAAAQAUAAAAQW5hbHlzZXJTdGF0ZU1hY2hpbmUBAYQuAC8BAeoDhC4AAP////8BAAAAFWCJ" +
           "CgIAAAAAAAwAAABDdXJyZW50U3RhdGUBAYUuAC8BAMgKhS4AAAAV/////wEB/////wEAAAAVYIkKAgAA" +
           "AAAAAgAAAElkAQGGLgAuAESGLgAAABH/////AQH/////AAAAAA==";
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

    #region MassSpectrometerDeviceState Class
    #if (!OPCUA_EXCLUDE_MassSpectrometerDeviceState)
    /// <summary>
    /// Stores an instance of the MassSpectrometerDeviceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class MassSpectrometerDeviceState : AnalyserDeviceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public MassSpectrometerDeviceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.ObjectTypes.MassSpectrometerDeviceType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////4RggAIBAAAAAQAiAAAATWFzc1NwZWN0cm9tZXRlckRldmljZVR5cGVJ" +
           "bnN0YW5jZQEB9gMBAfYD9gMAAAH/////DgAAACRggAoBAAAAAgAJAAAATWV0aG9kU2V0AQEQLwMAAAAA" +
           "FAAAAEZsYXQgbGlzdCBvZiBNZXRob2RzAC8AOhAvAAD/////CgAAAARhggoEAAAAAQAQAAAAR2V0Q29u" +
           "ZmlndXJhdGlvbgEBTS8ALwEB4yRNLwAAAQH/////AQAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1l" +
           "bnRzAQFOLwAuAEROLwAAlgEAAAABACoBARkAAAAKAAAAQ29uZmlnRGF0YQAP/////wAAAAAAAQAoAQEA" +
           "AAABAAAAAAAAAAEB/////wAAAAAEYYIKBAAAAAEAEAAAAFNldENvbmZpZ3VyYXRpb24BAU8vAC8BAeUk" +
           "Ty8AAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElucHV0QXJndW1lbnRzAQFQLwAuAERQLwAAlgEAAAAB" +
           "ACoBARkAAAAKAAAAQ29uZmlnRGF0YQAP/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAX" +
           "YKkKAgAAAAAADwAAAE91dHB1dEFyZ3VtZW50cwEBUS8ALgBEUS8AAJYBAAAAAQAqAQEfAAAAEAAAAENv" +
           "bmZpZ0RhdGFEaWdlc3QADP////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAABGGCCgQAAAAB" +
           "ABMAAABHZXRDb25maWdEYXRhRGlnZXN0AQFSLwAvAQHoJFIvAAABAf////8BAAAAF2CpCgIAAAAAAA8A" +
           "AABPdXRwdXRBcmd1bWVudHMBAVMvAC4ARFMvAACWAQAAAAEAKgEBHwAAABAAAABDb25maWdEYXRhRGln" +
           "ZXN0AAz/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAAARhggoEAAAAAQAXAAAAQ29tcGFy" +
           "ZUNvbmZpZ0RhdGFEaWdlc3QBAVQvAC8BAeokVC8AAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElucHV0" +
           "QXJndW1lbnRzAQFVLwAuAERVLwAAlgEAAAABACoBAR8AAAAQAAAAQ29uZmlnRGF0YURpZ2VzdAAM////" +
           "/wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAXYKkKAgAAAAAADwAAAE91dHB1dEFyZ3VtZW50" +
           "cwEBVi8ALgBEVi8AAJYBAAAAAQAqAQEWAAAABwAAAElzRXF1YWwAAf////8AAAAAAAEAKAEBAAAAAQAA" +
           "AAAAAAABAf////8AAAAAJGGCCgQAAAABABAAAABSZXNldEFsbENoYW5uZWxzAQFXLwMAAAAAPAAAAFJl" +
           "c2V0IGFsbCBBbmFseXNlckNoYW5uZWxzIGJlbG9uZ2luZyB0byB0aGlzIEFuYWx5c2VyRGV2aWNlLgAv" +
           "AQHtJFcvAAABAf////8AAAAAJGGCCgQAAAABABAAAABTdGFydEFsbENoYW5uZWxzAQFYLwMAAAAAPAAA" +
           "AFN0YXJ0IGFsbCBBbmFseXNlckNoYW5uZWxzIGJlbG9uZ2luZyB0byB0aGlzIEFuYWx5c2VyRGV2aWNl" +
           "LgAvAQHuJFgvAAABAf////8AAAAAJGGCCgQAAAABAA8AAABTdG9wQWxsQ2hhbm5lbHMBAVkvAwAAAAA7" +
           "AAAAU3RvcCBhbGwgQW5hbHlzZXJDaGFubmVscyBiZWxvbmdpbmcgdG8gdGhpcyBBbmFseXNlckRldmlj" +
           "ZS4ALwEB7yRZLwAAAQH/////AAAAACRhggoEAAAAAQAQAAAAQWJvcnRBbGxDaGFubmVscwEBWi8DAAAA" +
           "ADwAAABBYm9ydCBhbGwgQW5hbHlzZXJDaGFubmVscyBiZWxvbmdpbmcgdG8gdGhpcyBBbmFseXNlckRl" +
           "dmljZS4ALwEB8CRaLwAAAQH/////AAAAACRhggoEAAAAAQANAAAAR290b09wZXJhdGluZwEBWy8DAAAA" +
           "AI0AAABBbmFseXNlckRldmljZVN0YXRlTWFjaGluZSB0byBnbyB0byBPcGVyYXRpbmcgc3RhdGUsIGZv" +
           "cmNpbmcgYWxsIEFuYWx5c2VyQ2hhbm5lbHMgdG8gbGVhdmUgdGhlIFNsYXZlTW9kZSBzdGF0ZSBhbmQg" +
           "Z28gdG8gdGhlIE9wZXJhdGluZyBzdGF0ZS4ALwEB8SRbLwAAAQH/////AAAAACRhggoEAAAAAQAPAAAA" +
           "R290b01haW50ZW5hbmNlAQFcLwMAAAAAZwAAAEFuYWx5c2VyRGV2aWNlU3RhdGVNYWNoaW5lIHRvIGdv" +
           "IHRvIE1haW50ZW5hbmNlIHN0YXRlLCBmb3JjaW5nIGFsbCBBbmFseXNlckNoYW5uZWxzIHRvIFNsYXZl" +
           "TW9kZSBzdGF0ZS4ALwEB8iRcLwAAAQH/////AAAAACRggAoBAAAAAgAOAAAASWRlbnRpZmljYXRpb24B" +
           "ARQvAwAAAABGAAAAVXNlZCB0byBvcmdhbml6ZSBwYXJhbWV0ZXJzIGZvciBpZGVudGlmaWNhdGlvbiBv" +
           "ZiB0aGlzIFRvcG9sb2d5RWxlbWVudAAvAQLtAxQvAAADAAAAACMAAQJzFwAjAAECdBcAIwABAnEXAAAA" +
           "ADVgiQoCAAAAAgAMAAAATWFudWZhY3R1cmVyAQEmLwMAAAAAMAAAAE5hbWUgb2YgdGhlIGNvbXBhbnkg" +
           "dGhhdCBtYW51ZmFjdHVyZWQgdGhlIGRldmljZQAuAEQmLwAAABX/////AQH/////AAAAADVgiQoCAAAA" +
           "AgAFAAAATW9kZWwBAScvAwAAAAAYAAAATW9kZWwgbmFtZSBvZiB0aGUgZGV2aWNlAC4ARCcvAAAAFf//" +
           "//8BAf////8AAAAANWCJCgIAAAACABAAAABIYXJkd2FyZVJldmlzaW9uAQErLwMAAAAALAAAAFJldmlz" +
           "aW9uIGxldmVsIG9mIHRoZSBoYXJkd2FyZSBvZiB0aGUgZGV2aWNlAC4ARCsvAAAADP////8BAf////8A" +
           "AAAANWCJCgIAAAACABAAAABTb2Z0d2FyZVJldmlzaW9uAQEqLwMAAAAANQAAAFJldmlzaW9uIGxldmVs" +
           "IG9mIHRoZSBzb2Z0d2FyZS9maXJtd2FyZSBvZiB0aGUgZGV2aWNlAC4ARCovAAAADP////8BAf////8A" +
           "AAAANWCJCgIAAAACAA4AAABEZXZpY2VSZXZpc2lvbgEBKS8DAAAAACQAAABPdmVyYWxsIHJldmlzaW9u" +
           "IGxldmVsIG9mIHRoZSBkZXZpY2UALgBEKS8AAAAM/////wEB/////wAAAAA1YIkKAgAAAAIADAAAAERl" +
           "dmljZU1hbnVhbAEBKC8DAAAAAFoAAABBZGRyZXNzIChwYXRobmFtZSBpbiB0aGUgZmlsZSBzeXN0ZW0g" +
           "b3IgYSBVUkwgfCBXZWIgYWRkcmVzcykgb2YgdXNlciBtYW51YWwgZm9yIHRoZSBkZXZpY2UALgBEKC8A" +
           "AAAM/////wEB/////wAAAAA1YIkKAgAAAAIADAAAAFNlcmlhbE51bWJlcgEBJC8DAAAAAE0AAABJZGVu" +
           "dGlmaWVyIHRoYXQgdW5pcXVlbHkgaWRlbnRpZmllcywgd2l0aGluIGEgbWFudWZhY3R1cmVyLCBhIGRl" +
           "dmljZSBpbnN0YW5jZQAuAEQkLwAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAPAAAAUmV2aXNpb25D" +
           "b3VudGVyAQElLwMAAAAAaQAAAEFuIGluY3JlbWVudGFsIGNvdW50ZXIgaW5kaWNhdGluZyB0aGUgbnVt" +
           "YmVyIG9mIHRpbWVzIHRoZSBzdGF0aWMgZGF0YSB3aXRoaW4gdGhlIERldmljZSBoYXMgYmVlbiBtb2Rp" +
           "ZmllZAAuAEQlLwAAAAb/////AQH/////AAAAAARggAoBAAAAAQANAAAAQ29uZmlndXJhdGlvbgEBdC8A" +
           "LwEC7QN0LwAAAQAAAAAjAAEBYC8AAAAABGCACgEAAAABAAYAAABTdGF0dXMBAXYvAC8BAu0Ddi8AAAEA" +
           "AAAAIwABAV0vAAAAAARggAoBAAAAAQAPAAAARmFjdG9yeVNldHRpbmdzAQF4LwAvAQLtA3gvAAD/////" +
           "AAAAAARggAoBAAAAAQAUAAAAQW5hbHlzZXJTdGF0ZU1hY2hpbmUBAXovAC8BAeoDei8AAP////8BAAAA" +
           "FWCJCgIAAAAAAAwAAABDdXJyZW50U3RhdGUBAXsvAC8BAMgKey8AAAAV/////wEB/////wEAAAAVYIkK" +
           "AgAAAAAAAgAAAElkAQF8LwAuAER8LwAAABH/////AQH/////AAAAAA==";
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

    #region AcousticSpectrometerDeviceState Class
    #if (!OPCUA_EXCLUDE_AcousticSpectrometerDeviceState)
    /// <summary>
    /// Stores an instance of the AcousticSpectrometerDeviceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class AcousticSpectrometerDeviceState : AnalyserDeviceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public AcousticSpectrometerDeviceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.ObjectTypes.AcousticSpectrometerDeviceType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////4RggAIBAAAAAQAmAAAAQWNvdXN0aWNTcGVjdHJvbWV0ZXJEZXZpY2VU" +
           "eXBlSW5zdGFuY2UBAfcDAQH3A/cDAAAB/////w4AAAAkYIAKAQAAAAIACQAAAE1ldGhvZFNldAEBBjAD" +
           "AAAAABQAAABGbGF0IGxpc3Qgb2YgTWV0aG9kcwAvADoGMAAA/////woAAAAEYYIKBAAAAAEAEAAAAEdl" +
           "dENvbmZpZ3VyYXRpb24BAUMwAC8BAeMkQzAAAAEB/////wEAAAAXYKkKAgAAAAAADwAAAE91dHB1dEFy" +
           "Z3VtZW50cwEBRDAALgBERDAAAJYBAAAAAQAqAQEZAAAACgAAAENvbmZpZ0RhdGEAD/////8AAAAAAAEA" +
           "KAEBAAAAAQAAAAAAAAABAf////8AAAAABGGCCgQAAAABABAAAABTZXRDb25maWd1cmF0aW9uAQFFMAAv" +
           "AQHlJEUwAAABAf////8CAAAAF2CpCgIAAAAAAA4AAABJbnB1dEFyZ3VtZW50cwEBRjAALgBERjAAAJYB" +
           "AAAAAQAqAQEZAAAACgAAAENvbmZpZ0RhdGEAD/////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8A" +
           "AAAAF2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1bWVudHMBAUcwAC4AREcwAACWAQAAAAEAKgEBHwAAABAA" +
           "AABDb25maWdEYXRhRGlnZXN0AAz/////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAAARhggoE" +
           "AAAAAQATAAAAR2V0Q29uZmlnRGF0YURpZ2VzdAEBSDAALwEB6CRIMAAAAQH/////AQAAABdgqQoCAAAA" +
           "AAAPAAAAT3V0cHV0QXJndW1lbnRzAQFJMAAuAERJMAAAlgEAAAABACoBAR8AAAAQAAAAQ29uZmlnRGF0" +
           "YURpZ2VzdAAM/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAEYYIKBAAAAAEAFwAAAENv" +
           "bXBhcmVDb25maWdEYXRhRGlnZXN0AQFKMAAvAQHqJEowAAABAf////8CAAAAF2CpCgIAAAAAAA4AAABJ" +
           "bnB1dEFyZ3VtZW50cwEBSzAALgBESzAAAJYBAAAAAQAqAQEfAAAAEAAAAENvbmZpZ0RhdGFEaWdlc3QA" +
           "DP////8AAAAAAAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAAF2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1" +
           "bWVudHMBAUwwAC4AREwwAACWAQAAAAEAKgEBFgAAAAcAAABJc0VxdWFsAAH/////AAAAAAABACgBAQAA" +
           "AAEAAAAAAAAAAQH/////AAAAACRhggoEAAAAAQAQAAAAUmVzZXRBbGxDaGFubmVscwEBTTADAAAAADwA" +
           "AABSZXNldCBhbGwgQW5hbHlzZXJDaGFubmVscyBiZWxvbmdpbmcgdG8gdGhpcyBBbmFseXNlckRldmlj" +
           "ZS4ALwEB7SRNMAAAAQH/////AAAAACRhggoEAAAAAQAQAAAAU3RhcnRBbGxDaGFubmVscwEBTjADAAAA" +
           "ADwAAABTdGFydCBhbGwgQW5hbHlzZXJDaGFubmVscyBiZWxvbmdpbmcgdG8gdGhpcyBBbmFseXNlckRl" +
           "dmljZS4ALwEB7iROMAAAAQH/////AAAAACRhggoEAAAAAQAPAAAAU3RvcEFsbENoYW5uZWxzAQFPMAMA" +
           "AAAAOwAAAFN0b3AgYWxsIEFuYWx5c2VyQ2hhbm5lbHMgYmVsb25naW5nIHRvIHRoaXMgQW5hbHlzZXJE" +
           "ZXZpY2UuAC8BAe8kTzAAAAEB/////wAAAAAkYYIKBAAAAAEAEAAAAEFib3J0QWxsQ2hhbm5lbHMBAVAw" +
           "AwAAAAA8AAAAQWJvcnQgYWxsIEFuYWx5c2VyQ2hhbm5lbHMgYmVsb25naW5nIHRvIHRoaXMgQW5hbHlz" +
           "ZXJEZXZpY2UuAC8BAfAkUDAAAAEB/////wAAAAAkYYIKBAAAAAEADQAAAEdvdG9PcGVyYXRpbmcBAVEw" +
           "AwAAAACNAAAAQW5hbHlzZXJEZXZpY2VTdGF0ZU1hY2hpbmUgdG8gZ28gdG8gT3BlcmF0aW5nIHN0YXRl" +
           "LCBmb3JjaW5nIGFsbCBBbmFseXNlckNoYW5uZWxzIHRvIGxlYXZlIHRoZSBTbGF2ZU1vZGUgc3RhdGUg" +
           "YW5kIGdvIHRvIHRoZSBPcGVyYXRpbmcgc3RhdGUuAC8BAfEkUTAAAAEB/////wAAAAAkYYIKBAAAAAEA" +
           "DwAAAEdvdG9NYWludGVuYW5jZQEBUjADAAAAAGcAAABBbmFseXNlckRldmljZVN0YXRlTWFjaGluZSB0" +
           "byBnbyB0byBNYWludGVuYW5jZSBzdGF0ZSwgZm9yY2luZyBhbGwgQW5hbHlzZXJDaGFubmVscyB0byBT" +
           "bGF2ZU1vZGUgc3RhdGUuAC8BAfIkUjAAAAEB/////wAAAAAkYIAKAQAAAAIADgAAAElkZW50aWZpY2F0" +
           "aW9uAQEKMAMAAAAARgAAAFVzZWQgdG8gb3JnYW5pemUgcGFyYW1ldGVycyBmb3IgaWRlbnRpZmljYXRp" +
           "b24gb2YgdGhpcyBUb3BvbG9neUVsZW1lbnQALwEC7QMKMAAAAwAAAAAjAAECcxcAIwABAnQXACMAAQJx" +
           "FwAAAAA1YIkKAgAAAAIADAAAAE1hbnVmYWN0dXJlcgEBHDADAAAAADAAAABOYW1lIG9mIHRoZSBjb21w" +
           "YW55IHRoYXQgbWFudWZhY3R1cmVkIHRoZSBkZXZpY2UALgBEHDAAAAAV/////wEB/////wAAAAA1YIkK" +
           "AgAAAAIABQAAAE1vZGVsAQEdMAMAAAAAGAAAAE1vZGVsIG5hbWUgb2YgdGhlIGRldmljZQAuAEQdMAAA" +
           "ABX/////AQH/////AAAAADVgiQoCAAAAAgAQAAAASGFyZHdhcmVSZXZpc2lvbgEBITADAAAAACwAAABS" +
           "ZXZpc2lvbiBsZXZlbCBvZiB0aGUgaGFyZHdhcmUgb2YgdGhlIGRldmljZQAuAEQhMAAAAAz/////AQH/" +
           "////AAAAADVgiQoCAAAAAgAQAAAAU29mdHdhcmVSZXZpc2lvbgEBIDADAAAAADUAAABSZXZpc2lvbiBs" +
           "ZXZlbCBvZiB0aGUgc29mdHdhcmUvZmlybXdhcmUgb2YgdGhlIGRldmljZQAuAEQgMAAAAAz/////AQH/" +
           "////AAAAADVgiQoCAAAAAgAOAAAARGV2aWNlUmV2aXNpb24BAR8wAwAAAAAkAAAAT3ZlcmFsbCByZXZp" +
           "c2lvbiBsZXZlbCBvZiB0aGUgZGV2aWNlAC4ARB8wAAAADP////8BAf////8AAAAANWCJCgIAAAACAAwA" +
           "AABEZXZpY2VNYW51YWwBAR4wAwAAAABaAAAAQWRkcmVzcyAocGF0aG5hbWUgaW4gdGhlIGZpbGUgc3lz" +
           "dGVtIG9yIGEgVVJMIHwgV2ViIGFkZHJlc3MpIG9mIHVzZXIgbWFudWFsIGZvciB0aGUgZGV2aWNlAC4A" +
           "RB4wAAAADP////8BAf////8AAAAANWCJCgIAAAACAAwAAABTZXJpYWxOdW1iZXIBARowAwAAAABNAAAA" +
           "SWRlbnRpZmllciB0aGF0IHVuaXF1ZWx5IGlkZW50aWZpZXMsIHdpdGhpbiBhIG1hbnVmYWN0dXJlciwg" +
           "YSBkZXZpY2UgaW5zdGFuY2UALgBEGjAAAAAM/////wEB/////wAAAAA1YIkKAgAAAAIADwAAAFJldmlz" +
           "aW9uQ291bnRlcgEBGzADAAAAAGkAAABBbiBpbmNyZW1lbnRhbCBjb3VudGVyIGluZGljYXRpbmcgdGhl" +
           "IG51bWJlciBvZiB0aW1lcyB0aGUgc3RhdGljIGRhdGEgd2l0aGluIHRoZSBEZXZpY2UgaGFzIGJlZW4g" +
           "bW9kaWZpZWQALgBEGzAAAAAG/////wEB/////wAAAAAEYIAKAQAAAAEADQAAAENvbmZpZ3VyYXRpb24B" +
           "AWowAC8BAu0DajAAAAEAAAAAIwABAVYwAAAAAARggAoBAAAAAQAGAAAAU3RhdHVzAQFsMAAvAQLtA2ww" +
           "AAABAAAAACMAAQFTMAAAAAAEYIAKAQAAAAEADwAAAEZhY3RvcnlTZXR0aW5ncwEBbjAALwEC7QNuMAAA" +
           "/////wAAAAAEYIAKAQAAAAEAFAAAAEFuYWx5c2VyU3RhdGVNYWNoaW5lAQFwMAAvAQHqA3AwAAD/////" +
           "AQAAABVgiQoCAAAAAAAMAAAAQ3VycmVudFN0YXRlAQFxMAAvAQDICnEwAAAAFf////8BAf////8BAAAA" +
           "FWCJCgIAAAAAAAIAAABJZAEBcjAALgBEcjAAAAAR/////wEB/////wAAAAA=";
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

    #region NMRDeviceState Class
    #if (!OPCUA_EXCLUDE_NMRDeviceState)
    /// <summary>
    /// Stores an instance of the NMRDeviceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class NMRDeviceState : AnalyserDeviceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public NMRDeviceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.ObjectTypes.NMRDeviceType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////4RggAIBAAAAAQAVAAAATk1SRGV2aWNlVHlwZUluc3RhbmNlAQH4AwEB" +
           "+AP4AwAAAf////8OAAAAJGCACgEAAAACAAkAAABNZXRob2RTZXQBAfwwAwAAAAAUAAAARmxhdCBsaXN0" +
           "IG9mIE1ldGhvZHMALwA6/DAAAP////8KAAAABGGCCgQAAAABABAAAABHZXRDb25maWd1cmF0aW9uAQE5" +
           "MQAvAQHjJDkxAAABAf////8BAAAAF2CpCgIAAAAAAA8AAABPdXRwdXRBcmd1bWVudHMBAToxAC4ARDox" +
           "AACWAQAAAAEAKgEBGQAAAAoAAABDb25maWdEYXRhAA//////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/" +
           "////AAAAAARhggoEAAAAAQAQAAAAU2V0Q29uZmlndXJhdGlvbgEBOzEALwEB5SQ7MQAAAQH/////AgAA" +
           "ABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBATwxAC4ARDwxAACWAQAAAAEAKgEBGQAAAAoAAABD" +
           "b25maWdEYXRhAA//////AAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAA" +
           "T3V0cHV0QXJndW1lbnRzAQE9MQAuAEQ9MQAAlgEAAAABACoBAR8AAAAQAAAAQ29uZmlnRGF0YURpZ2Vz" +
           "dAAM/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAAAAAEYYIKBAAAAAEAEwAAAEdldENvbmZp" +
           "Z0RhdGFEaWdlc3QBAT4xAC8BAegkPjEAAAEB/////wEAAAAXYKkKAgAAAAAADwAAAE91dHB1dEFyZ3Vt" +
           "ZW50cwEBPzEALgBEPzEAAJYBAAAAAQAqAQEfAAAAEAAAAENvbmZpZ0RhdGFEaWdlc3QADP////8AAAAA" +
           "AAEAKAEBAAAAAQAAAAAAAAABAf////8AAAAABGGCCgQAAAABABcAAABDb21wYXJlQ29uZmlnRGF0YURp" +
           "Z2VzdAEBQDEALwEB6iRAMQAAAQH/////AgAAABdgqQoCAAAAAAAOAAAASW5wdXRBcmd1bWVudHMBAUEx" +
           "AC4AREExAACWAQAAAAEAKgEBHwAAABAAAABDb25maWdEYXRhRGlnZXN0AAz/////AAAAAAABACgBAQAA" +
           "AAEAAAAAAAAAAQH/////AAAAABdgqQoCAAAAAAAPAAAAT3V0cHV0QXJndW1lbnRzAQFCMQAuAERCMQAA" +
           "lgEAAAABACoBARYAAAAHAAAASXNFcXVhbAAB/////wAAAAAAAQAoAQEAAAABAAAAAAAAAAEB/////wAA" +
           "AAAkYYIKBAAAAAEAEAAAAFJlc2V0QWxsQ2hhbm5lbHMBAUMxAwAAAAA8AAAAUmVzZXQgYWxsIEFuYWx5" +
           "c2VyQ2hhbm5lbHMgYmVsb25naW5nIHRvIHRoaXMgQW5hbHlzZXJEZXZpY2UuAC8BAe0kQzEAAAEB////" +
           "/wAAAAAkYYIKBAAAAAEAEAAAAFN0YXJ0QWxsQ2hhbm5lbHMBAUQxAwAAAAA8AAAAU3RhcnQgYWxsIEFu" +
           "YWx5c2VyQ2hhbm5lbHMgYmVsb25naW5nIHRvIHRoaXMgQW5hbHlzZXJEZXZpY2UuAC8BAe4kRDEAAAEB" +
           "/////wAAAAAkYYIKBAAAAAEADwAAAFN0b3BBbGxDaGFubmVscwEBRTEDAAAAADsAAABTdG9wIGFsbCBB" +
           "bmFseXNlckNoYW5uZWxzIGJlbG9uZ2luZyB0byB0aGlzIEFuYWx5c2VyRGV2aWNlLgAvAQHvJEUxAAAB" +
           "Af////8AAAAAJGGCCgQAAAABABAAAABBYm9ydEFsbENoYW5uZWxzAQFGMQMAAAAAPAAAAEFib3J0IGFs" +
           "bCBBbmFseXNlckNoYW5uZWxzIGJlbG9uZ2luZyB0byB0aGlzIEFuYWx5c2VyRGV2aWNlLgAvAQHwJEYx" +
           "AAABAf////8AAAAAJGGCCgQAAAABAA0AAABHb3RvT3BlcmF0aW5nAQFHMQMAAAAAjQAAAEFuYWx5c2Vy" +
           "RGV2aWNlU3RhdGVNYWNoaW5lIHRvIGdvIHRvIE9wZXJhdGluZyBzdGF0ZSwgZm9yY2luZyBhbGwgQW5h" +
           "bHlzZXJDaGFubmVscyB0byBsZWF2ZSB0aGUgU2xhdmVNb2RlIHN0YXRlIGFuZCBnbyB0byB0aGUgT3Bl" +
           "cmF0aW5nIHN0YXRlLgAvAQHxJEcxAAABAf////8AAAAAJGGCCgQAAAABAA8AAABHb3RvTWFpbnRlbmFu" +
           "Y2UBAUgxAwAAAABnAAAAQW5hbHlzZXJEZXZpY2VTdGF0ZU1hY2hpbmUgdG8gZ28gdG8gTWFpbnRlbmFu" +
           "Y2Ugc3RhdGUsIGZvcmNpbmcgYWxsIEFuYWx5c2VyQ2hhbm5lbHMgdG8gU2xhdmVNb2RlIHN0YXRlLgAv" +
           "AQHyJEgxAAABAf////8AAAAAJGCACgEAAAACAA4AAABJZGVudGlmaWNhdGlvbgEBADEDAAAAAEYAAABV" +
           "c2VkIHRvIG9yZ2FuaXplIHBhcmFtZXRlcnMgZm9yIGlkZW50aWZpY2F0aW9uIG9mIHRoaXMgVG9wb2xv" +
           "Z3lFbGVtZW50AC8BAu0DADEAAAMAAAAAIwABAnMXACMAAQJ0FwAjAAECcRcAAAAANWCJCgIAAAACAAwA" +
           "AABNYW51ZmFjdHVyZXIBARIxAwAAAAAwAAAATmFtZSBvZiB0aGUgY29tcGFueSB0aGF0IG1hbnVmYWN0" +
           "dXJlZCB0aGUgZGV2aWNlAC4ARBIxAAAAFf////8BAf////8AAAAANWCJCgIAAAACAAUAAABNb2RlbAEB" +
           "EzEDAAAAABgAAABNb2RlbCBuYW1lIG9mIHRoZSBkZXZpY2UALgBEEzEAAAAV/////wEB/////wAAAAA1" +
           "YIkKAgAAAAIAEAAAAEhhcmR3YXJlUmV2aXNpb24BARcxAwAAAAAsAAAAUmV2aXNpb24gbGV2ZWwgb2Yg" +
           "dGhlIGhhcmR3YXJlIG9mIHRoZSBkZXZpY2UALgBEFzEAAAAM/////wEB/////wAAAAA1YIkKAgAAAAIA" +
           "EAAAAFNvZnR3YXJlUmV2aXNpb24BARYxAwAAAAA1AAAAUmV2aXNpb24gbGV2ZWwgb2YgdGhlIHNvZnR3" +
           "YXJlL2Zpcm13YXJlIG9mIHRoZSBkZXZpY2UALgBEFjEAAAAM/////wEB/////wAAAAA1YIkKAgAAAAIA" +
           "DgAAAERldmljZVJldmlzaW9uAQEVMQMAAAAAJAAAAE92ZXJhbGwgcmV2aXNpb24gbGV2ZWwgb2YgdGhl" +
           "IGRldmljZQAuAEQVMQAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAMAAAARGV2aWNlTWFudWFsAQEU" +
           "MQMAAAAAWgAAAEFkZHJlc3MgKHBhdGhuYW1lIGluIHRoZSBmaWxlIHN5c3RlbSBvciBhIFVSTCB8IFdl" +
           "YiBhZGRyZXNzKSBvZiB1c2VyIG1hbnVhbCBmb3IgdGhlIGRldmljZQAuAEQUMQAAAAz/////AQH/////" +
           "AAAAADVgiQoCAAAAAgAMAAAAU2VyaWFsTnVtYmVyAQEQMQMAAAAATQAAAElkZW50aWZpZXIgdGhhdCB1" +
           "bmlxdWVseSBpZGVudGlmaWVzLCB3aXRoaW4gYSBtYW51ZmFjdHVyZXIsIGEgZGV2aWNlIGluc3RhbmNl" +
           "AC4ARBAxAAAADP////8BAf////8AAAAANWCJCgIAAAACAA8AAABSZXZpc2lvbkNvdW50ZXIBARExAwAA" +
           "AABpAAAAQW4gaW5jcmVtZW50YWwgY291bnRlciBpbmRpY2F0aW5nIHRoZSBudW1iZXIgb2YgdGltZXMg" +
           "dGhlIHN0YXRpYyBkYXRhIHdpdGhpbiB0aGUgRGV2aWNlIGhhcyBiZWVuIG1vZGlmaWVkAC4ARBExAAAA" +
           "Bv////8BAf////8AAAAABGCACgEAAAABAA0AAABDb25maWd1cmF0aW9uAQFgMQAvAQLtA2AxAAABAAAA" +
           "ACMAAQFMMQAAAAAEYIAKAQAAAAEABgAAAFN0YXR1cwEBYjEALwEC7QNiMQAAAQAAAAAjAAEBSTEAAAAA" +
           "BGCACgEAAAABAA8AAABGYWN0b3J5U2V0dGluZ3MBAWQxAC8BAu0DZDEAAP////8AAAAABGCACgEAAAAB" +
           "ABQAAABBbmFseXNlclN0YXRlTWFjaGluZQEBZjEALwEB6gNmMQAA/////wEAAAAVYIkKAgAAAAAADAAA" +
           "AEN1cnJlbnRTdGF0ZQEBZzEALwEAyApnMQAAABX/////AQH/////AQAAABVgiQoCAAAAAAACAAAASWQB" +
           "AWgxAC4ARGgxAAAAEf////8BAf////8AAAAA";
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

    #region AccessorySlotState Class
    #if (!OPCUA_EXCLUDE_AccessorySlotState)
    /// <summary>
    /// Stores an instance of the AccessorySlotType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class AccessorySlotState : ConfigurableObjectState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public AccessorySlotState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.ObjectTypes.AccessorySlotType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAZAAAAQWNjZXNzb3J5U2xvdFR5cGVJbnN0YW5jZQEB" +
           "+QMBAfkD+QMAAP////8EAAAAJGCACgEAAAACAA4AAABTdXBwb3J0ZWRUeXBlcwEB8DEDAAAAAHIAAABG" +
           "b2xkZXIgbWFpbnRhaW5pbmcgdGhlIHNldCBvZiAoc3ViLXR5cGVzIG9mKSBCYXNlT2JqZWN0VHlwZXMg" +
           "dGhhdCBjYW4gYmUgaW5zdGFudGlhdGVkIGluIHRoZSBDb25maWd1cmFibGVDb21wb25lbnQALwA98DEA" +
           "AP////8AAAAANWCJCgIAAAABAA4AAABJc0hvdFN3YXBwYWJsZQEB8jEDAAAAAE4AAABUcnVlIGlmIGFu" +
           "IGFjY2Vzc29yeSBjYW4gYmUgaW5zZXJ0ZWQgaW4gdGhlIGFjY2Vzc29yeSBzbG90IHdoaWxlIGl0IGlz" +
           "IHBvd2VyZWQALgBE8jEAAAAB/////wEB/////wAAAAA1YIkKAgAAAAEACQAAAElzRW5hYmxlZAEB8zED" +
           "AAAAAEYAAABUcnVlIGlmIHRoaXMgYWNjZXNzb3J5IHNsb3QgaXMgY2FwYWJsZSBvZiBhY2NlcHRpbmcg" +
           "YW4gYWNjZXNzb3J5IGluIGl0AC4ARPMxAAAAAf////8BAf////8AAAAABGCACgEAAAABABkAAABBY2Nl" +
           "c3NvcnlTbG90U3RhdGVNYWNoaW5lAQH0MQAvAQH6A/QxAAD/////AQAAABVgiQoCAAAAAAAMAAAAQ3Vy" +
           "cmVudFN0YXRlAQH1MQAvAQDICvUxAAAAFf////8BAf////8BAAAAFWCJCgIAAAAAAAIAAABJZAEB9jEA" +
           "LgBE9jEAAAAR/////wEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public PropertyState<bool> IsHotSwappable
        {
            get
            {
                return m_isHotSwappable;
            }

            set
            {
                if (!Object.ReferenceEquals(m_isHotSwappable, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_isHotSwappable = value;
            }
        }

        /// <remarks />
        public PropertyState<bool> IsEnabled
        {
            get
            {
                return m_isEnabled;
            }

            set
            {
                if (!Object.ReferenceEquals(m_isEnabled, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_isEnabled = value;
            }
        }

        /// <remarks />
        public AccessorySlotStateMachineState AccessorySlotStateMachine
        {
            get
            {
                return m_accessorySlotStateMachine;
            }

            set
            {
                if (!Object.ReferenceEquals(m_accessorySlotStateMachine, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_accessorySlotStateMachine = value;
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
            if (m_isHotSwappable != null)
            {
                children.Add(m_isHotSwappable);
            }

            if (m_isEnabled != null)
            {
                children.Add(m_isEnabled);
            }

            if (m_accessorySlotStateMachine != null)
            {
                children.Add(m_accessorySlotStateMachine);
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
                case Opc.Ua.Adi.BrowseNames.IsHotSwappable:
                {
                    if (createOrReplace)
                    {
                        if (IsHotSwappable == null)
                        {
                            if (replacement == null)
                            {
                                IsHotSwappable = new PropertyState<bool>(this);
                            }
                            else
                            {
                                IsHotSwappable = (PropertyState<bool>)replacement;
                            }
                        }
                    }

                    instance = IsHotSwappable;
                    break;
                }

                case Opc.Ua.Adi.BrowseNames.IsEnabled:
                {
                    if (createOrReplace)
                    {
                        if (IsEnabled == null)
                        {
                            if (replacement == null)
                            {
                                IsEnabled = new PropertyState<bool>(this);
                            }
                            else
                            {
                                IsEnabled = (PropertyState<bool>)replacement;
                            }
                        }
                    }

                    instance = IsEnabled;
                    break;
                }

                case Opc.Ua.Adi.BrowseNames.AccessorySlotStateMachine:
                {
                    if (createOrReplace)
                    {
                        if (AccessorySlotStateMachine == null)
                        {
                            if (replacement == null)
                            {
                                AccessorySlotStateMachine = new AccessorySlotStateMachineState(this);
                            }
                            else
                            {
                                AccessorySlotStateMachine = (AccessorySlotStateMachineState)replacement;
                            }
                        }
                    }

                    instance = AccessorySlotStateMachine;
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
        private PropertyState<bool> m_isHotSwappable;
        private PropertyState<bool> m_isEnabled;
        private AccessorySlotStateMachineState m_accessorySlotStateMachine;
        #endregion
    }
    #endif
    #endregion

    #region AccessorySlotStateMachineState Class
    #if (!OPCUA_EXCLUDE_AccessorySlotStateMachineState)
    /// <summary>
    /// Stores an instance of the AccessorySlotStateMachineType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class AccessorySlotStateMachineState : FiniteStateMachineState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public AccessorySlotStateMachineState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.ObjectTypes.AccessorySlotStateMachineType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAlAAAAQWNjZXNzb3J5U2xvdFN0YXRlTWFjaGluZVR5" +
           "cGVJbnN0YW5jZQEB+gMBAfoD+gMAAP////8BAAAAFWCJCgIAAAAAAAwAAABDdXJyZW50U3RhdGUBAR0y" +
           "AC8BAMgKHTIAAAAV/////wEB/////wEAAAAVYIkKAgAAAAAAAgAAAElkAQEeMgAuAEQeMgAAABH/////" +
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

    #region AccessoryState Class
    #if (!OPCUA_EXCLUDE_AccessoryState)
    /// <summary>
    /// Stores an instance of the AccessoryType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class AccessoryState : TopologyElementState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public AccessoryState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.ObjectTypes.AccessoryType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAVAAAAQWNjZXNzb3J5VHlwZUluc3RhbmNlAQH7AwEB" +
           "+wP7AwAA/////wUAAAAEYIAKAQAAAAEADQAAAENvbmZpZ3VyYXRpb24BAWIyAC8BAu0DYjIAAP////8A" +
           "AAAABGCACgEAAAABAAYAAABTdGF0dXMBAWQyAC8BAu0DZDIAAP////8AAAAABGCACgEAAAABAA8AAABG" +
           "YWN0b3J5U2V0dGluZ3MBAWYyAC8BAu0DZjIAAP////8AAAAANWCJCgIAAAABAA4AAABJc0hvdFN3YXBw" +
           "YWJsZQEBaDIDAAAAAFAAAABUcnVlIGlmIHRoaXMgYWNjZXNzb3J5IGNhbiBiZSBpbnNlcnRlZCBpbiB0" +
           "aGUgYWNjZXNzb3J5IHNsb3Qgd2hpbGUgaXQgaXMgcG93ZXJlZAAuAERoMgAAAAH/////AQH/////AAAA" +
           "ADVgiQoCAAAAAQAHAAAASXNSZWFkeQEBaTIDAAAAACcAAABUcnVlIGlmIHRoaXMgYWNjZXNzb3J5IGlz" +
           "IHJlYWR5IGZvciB1c2UALgBEaTIAAAAB/////wEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public FunctionalGroupState Configuration
        {
            get
            {
                return m_configuration;
            }

            set
            {
                if (!Object.ReferenceEquals(m_configuration, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_configuration = value;
            }
        }

        /// <remarks />
        public FunctionalGroupState Status
        {
            get
            {
                return m_status;
            }

            set
            {
                if (!Object.ReferenceEquals(m_status, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_status = value;
            }
        }

        /// <remarks />
        public FunctionalGroupState FactorySettings
        {
            get
            {
                return m_factorySettings;
            }

            set
            {
                if (!Object.ReferenceEquals(m_factorySettings, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_factorySettings = value;
            }
        }

        /// <remarks />
        public PropertyState<bool> IsHotSwappable
        {
            get
            {
                return m_isHotSwappable;
            }

            set
            {
                if (!Object.ReferenceEquals(m_isHotSwappable, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_isHotSwappable = value;
            }
        }

        /// <remarks />
        public PropertyState<bool> IsReady
        {
            get
            {
                return m_isReady;
            }

            set
            {
                if (!Object.ReferenceEquals(m_isReady, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_isReady = value;
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
            if (m_configuration != null)
            {
                children.Add(m_configuration);
            }

            if (m_status != null)
            {
                children.Add(m_status);
            }

            if (m_factorySettings != null)
            {
                children.Add(m_factorySettings);
            }

            if (m_isHotSwappable != null)
            {
                children.Add(m_isHotSwappable);
            }

            if (m_isReady != null)
            {
                children.Add(m_isReady);
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
                case Opc.Ua.Adi.BrowseNames.Configuration:
                {
                    if (createOrReplace)
                    {
                        if (Configuration == null)
                        {
                            if (replacement == null)
                            {
                                Configuration = new FunctionalGroupState(this);
                            }
                            else
                            {
                                Configuration = (FunctionalGroupState)replacement;
                            }
                        }
                    }

                    instance = Configuration;
                    break;
                }

                case Opc.Ua.Adi.BrowseNames.Status:
                {
                    if (createOrReplace)
                    {
                        if (Status == null)
                        {
                            if (replacement == null)
                            {
                                Status = new FunctionalGroupState(this);
                            }
                            else
                            {
                                Status = (FunctionalGroupState)replacement;
                            }
                        }
                    }

                    instance = Status;
                    break;
                }

                case Opc.Ua.Adi.BrowseNames.FactorySettings:
                {
                    if (createOrReplace)
                    {
                        if (FactorySettings == null)
                        {
                            if (replacement == null)
                            {
                                FactorySettings = new FunctionalGroupState(this);
                            }
                            else
                            {
                                FactorySettings = (FunctionalGroupState)replacement;
                            }
                        }
                    }

                    instance = FactorySettings;
                    break;
                }

                case Opc.Ua.Adi.BrowseNames.IsHotSwappable:
                {
                    if (createOrReplace)
                    {
                        if (IsHotSwappable == null)
                        {
                            if (replacement == null)
                            {
                                IsHotSwappable = new PropertyState<bool>(this);
                            }
                            else
                            {
                                IsHotSwappable = (PropertyState<bool>)replacement;
                            }
                        }
                    }

                    instance = IsHotSwappable;
                    break;
                }

                case Opc.Ua.Adi.BrowseNames.IsReady:
                {
                    if (createOrReplace)
                    {
                        if (IsReady == null)
                        {
                            if (replacement == null)
                            {
                                IsReady = new PropertyState<bool>(this);
                            }
                            else
                            {
                                IsReady = (PropertyState<bool>)replacement;
                            }
                        }
                    }

                    instance = IsReady;
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
        private FunctionalGroupState m_configuration;
        private FunctionalGroupState m_status;
        private FunctionalGroupState m_factorySettings;
        private PropertyState<bool> m_isHotSwappable;
        private PropertyState<bool> m_isReady;
        #endregion
    }
    #endif
    #endregion

    #region DetectorState Class
    #if (!OPCUA_EXCLUDE_DetectorState)
    /// <summary>
    /// Stores an instance of the DetectorType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class DetectorState : AccessoryState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public DetectorState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.ObjectTypes.DetectorType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAUAAAARGV0ZWN0b3JUeXBlSW5zdGFuY2UBAYYkAQGG" +
           "JIYkAAD/////BQAAAARggAoBAAAAAQANAAAAQ29uZmlndXJhdGlvbgEBgDIALwEC7QOAMgAA/////wAA" +
           "AAAEYIAKAQAAAAEABgAAAFN0YXR1cwEBgjIALwEC7QOCMgAA/////wAAAAAEYIAKAQAAAAEADwAAAEZh" +
           "Y3RvcnlTZXR0aW5ncwEBhDIALwEC7QOEMgAA/////wAAAAA1YIkKAgAAAAEADgAAAElzSG90U3dhcHBh" +
           "YmxlAQGGMgMAAAAAUAAAAFRydWUgaWYgdGhpcyBhY2Nlc3NvcnkgY2FuIGJlIGluc2VydGVkIGluIHRo" +
           "ZSBhY2Nlc3Nvcnkgc2xvdCB3aGlsZSBpdCBpcyBwb3dlcmVkAC4ARIYyAAAAAf////8BAf////8AAAAA" +
           "NWCJCgIAAAABAAcAAABJc1JlYWR5AQGHMgMAAAAAJwAAAFRydWUgaWYgdGhpcyBhY2Nlc3NvcnkgaXMg" +
           "cmVhZHkgZm9yIHVzZQAuAESHMgAAAAH/////AQH/////AAAAAA==";
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

    #region SmartSamplingSystemState Class
    #if (!OPCUA_EXCLUDE_SmartSamplingSystemState)
    /// <summary>
    /// Stores an instance of the SmartSamplingSystemType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class SmartSamplingSystemState : AccessoryState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public SmartSamplingSystemState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.ObjectTypes.SmartSamplingSystemType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQAfAAAAU21hcnRTYW1wbGluZ1N5c3RlbVR5cGVJbnN0" +
           "YW5jZQEBjyQBAY8kjyQAAP////8FAAAABGCACgEAAAABAA0AAABDb25maWd1cmF0aW9uAQGeMgAvAQLt" +
           "A54yAAD/////AAAAAARggAoBAAAAAQAGAAAAU3RhdHVzAQGgMgAvAQLtA6AyAAD/////AAAAAARggAoB" +
           "AAAAAQAPAAAARmFjdG9yeVNldHRpbmdzAQGiMgAvAQLtA6IyAAD/////AAAAADVgiQoCAAAAAQAOAAAA" +
           "SXNIb3RTd2FwcGFibGUBAaQyAwAAAABQAAAAVHJ1ZSBpZiB0aGlzIGFjY2Vzc29yeSBjYW4gYmUgaW5z" +
           "ZXJ0ZWQgaW4gdGhlIGFjY2Vzc29yeSBzbG90IHdoaWxlIGl0IGlzIHBvd2VyZWQALgBEpDIAAAAB////" +
           "/wEB/////wAAAAA1YIkKAgAAAAEABwAAAElzUmVhZHkBAaUyAwAAAAAnAAAAVHJ1ZSBpZiB0aGlzIGFj" +
           "Y2Vzc29yeSBpcyByZWFkeSBmb3IgdXNlAC4ARKUyAAAAAf////8BAf////8AAAAA";
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

    #region SourceState Class
    #if (!OPCUA_EXCLUDE_SourceState)
    /// <summary>
    /// Stores an instance of the SourceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class SourceState : AccessoryState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public SourceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.ObjectTypes.SourceType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQASAAAAU291cmNlVHlwZUluc3RhbmNlAQGYJAEBmCSY" +
           "JAAA/////wUAAAAEYIAKAQAAAAEADQAAAENvbmZpZ3VyYXRpb24BAbwyAC8BAu0DvDIAAP////8AAAAA" +
           "BGCACgEAAAABAAYAAABTdGF0dXMBAb4yAC8BAu0DvjIAAP////8AAAAABGCACgEAAAABAA8AAABGYWN0" +
           "b3J5U2V0dGluZ3MBAcAyAC8BAu0DwDIAAP////8AAAAANWCJCgIAAAABAA4AAABJc0hvdFN3YXBwYWJs" +
           "ZQEBwjIDAAAAAFAAAABUcnVlIGlmIHRoaXMgYWNjZXNzb3J5IGNhbiBiZSBpbnNlcnRlZCBpbiB0aGUg" +
           "YWNjZXNzb3J5IHNsb3Qgd2hpbGUgaXQgaXMgcG93ZXJlZAAuAETCMgAAAAH/////AQH/////AAAAADVg" +
           "iQoCAAAAAQAHAAAASXNSZWFkeQEBwzIDAAAAACcAAABUcnVlIGlmIHRoaXMgYWNjZXNzb3J5IGlzIHJl" +
           "YWR5IGZvciB1c2UALgBEwzIAAAAB/////wEB/////wAAAAA=";
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

    #region GcOvenState Class
    #if (!OPCUA_EXCLUDE_GcOvenState)
    /// <summary>
    /// Stores an instance of the GcOvenType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class GcOvenState : AccessoryState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public GcOvenState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.ObjectTypes.GcOvenType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////wRggAIBAAAAAQASAAAAR2NPdmVuVHlwZUluc3RhbmNlAQH8AwEB/AP8" +
           "AwAA/////wUAAAAEYIAKAQAAAAEADQAAAENvbmZpZ3VyYXRpb24BAdoyAC8BAu0D2jIAAP////8AAAAA" +
           "BGCACgEAAAABAAYAAABTdGF0dXMBAdwyAC8BAu0D3DIAAP////8AAAAABGCACgEAAAABAA8AAABGYWN0" +
           "b3J5U2V0dGluZ3MBAd4yAC8BAu0D3jIAAP////8AAAAANWCJCgIAAAABAA4AAABJc0hvdFN3YXBwYWJs" +
           "ZQEB4DIDAAAAAFAAAABUcnVlIGlmIHRoaXMgYWNjZXNzb3J5IGNhbiBiZSBpbnNlcnRlZCBpbiB0aGUg" +
           "YWNjZXNzb3J5IHNsb3Qgd2hpbGUgaXQgaXMgcG93ZXJlZAAuAETgMgAAAAH/////AQH/////AAAAADVg" +
           "iQoCAAAAAQAHAAAASXNSZWFkeQEB4TIDAAAAACcAAABUcnVlIGlmIHRoaXMgYWNjZXNzb3J5IGlzIHJl" +
           "YWR5IGZvciB1c2UALgBE4TIAAAAB/////wEB/////wAAAAA=";
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

    #region EngineeringValueState Class
    #if (!OPCUA_EXCLUDE_EngineeringValueState)
    /// <summary>
    /// Stores an instance of the EngineeringValueType VariableType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class EngineeringValueState : DataItemState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public EngineeringValueState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.VariableTypes.EngineeringValueType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////xVgiQICAAAAAQAcAAAARW5naW5lZXJpbmdWYWx1ZVR5cGVJbnN0YW5j" +
           "ZQEBpCQBAaQkpCQAAAAY/v///wEB/////wAAAAA=";
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

    #region EngineeringValueState<T> Class
    /// <summary>
    /// A typed version of the EngineeringValueType variable.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public class EngineeringValueState<T> : EngineeringValueState
    {
        #region Constructors
        /// <summary>
        /// Initializes the instance with its defalt attribute values.
        /// </summary>
        public EngineeringValueState(NodeState parent) : base(parent)
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

    #region ChemometricModelState Class
    #if (!OPCUA_EXCLUDE_ChemometricModelState)
    /// <summary>
    /// Stores an instance of the ChemometricModelType VariableType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ChemometricModelState : BaseDataVariableState<byte[]>
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ChemometricModelState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.VariableTypes.ChemometricModelType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
        }

        /// <summary>
        /// Returns the id of the default data type node for the instance.
        /// </summary>
        protected override NodeId GetDefaultDataTypeId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.DataTypes.ByteString, Opc.Ua.Namespaces.OpcUa, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////xVgiQICAAAAAQAcAAAAQ2hlbW9tZXRyaWNNb2RlbFR5cGVJbnN0YW5j" +
           "ZQEB1wcBAdcH1wcAAAAP/v///wEB/////wMAAAAVYIkKAgAAAAEABAAAAE5hbWUBAekyAC4AROkyAAAA" +
           "Ff////8BAf////8AAAAAFWCJCgIAAAABAAwAAABDcmVhdGlvbkRhdGUBAeoyAC4AROoyAAAADf////8B" +
           "Af////8AAAAAFWCJCgIAAAABABAAAABNb2RlbERlc2NyaXB0aW9uAQHrMgAuAETrMgAAABX/////AQH/" +
           "////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public PropertyState<LocalizedText> Name
        {
            get
            {
                return m_name;
            }

            set
            {
                if (!Object.ReferenceEquals(m_name, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_name = value;
            }
        }

        /// <remarks />
        public PropertyState<DateTime> CreationDate
        {
            get
            {
                return m_creationDate;
            }

            set
            {
                if (!Object.ReferenceEquals(m_creationDate, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_creationDate = value;
            }
        }

        /// <remarks />
        public PropertyState<LocalizedText> ModelDescription
        {
            get
            {
                return m_modelDescription;
            }

            set
            {
                if (!Object.ReferenceEquals(m_modelDescription, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_modelDescription = value;
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
            if (m_name != null)
            {
                children.Add(m_name);
            }

            if (m_creationDate != null)
            {
                children.Add(m_creationDate);
            }

            if (m_modelDescription != null)
            {
                children.Add(m_modelDescription);
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
                case Opc.Ua.Adi.BrowseNames.Name:
                {
                    if (createOrReplace)
                    {
                        if (Name == null)
                        {
                            if (replacement == null)
                            {
                                Name = new PropertyState<LocalizedText>(this);
                            }
                            else
                            {
                                Name = (PropertyState<LocalizedText>)replacement;
                            }
                        }
                    }

                    instance = Name;
                    break;
                }

                case Opc.Ua.Adi.BrowseNames.CreationDate:
                {
                    if (createOrReplace)
                    {
                        if (CreationDate == null)
                        {
                            if (replacement == null)
                            {
                                CreationDate = new PropertyState<DateTime>(this);
                            }
                            else
                            {
                                CreationDate = (PropertyState<DateTime>)replacement;
                            }
                        }
                    }

                    instance = CreationDate;
                    break;
                }

                case Opc.Ua.Adi.BrowseNames.ModelDescription:
                {
                    if (createOrReplace)
                    {
                        if (ModelDescription == null)
                        {
                            if (replacement == null)
                            {
                                ModelDescription = new PropertyState<LocalizedText>(this);
                            }
                            else
                            {
                                ModelDescription = (PropertyState<LocalizedText>)replacement;
                            }
                        }
                    }

                    instance = ModelDescription;
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
        private PropertyState<LocalizedText> m_name;
        private PropertyState<DateTime> m_creationDate;
        private PropertyState<LocalizedText> m_modelDescription;
        #endregion
    }
    #endif
    #endregion

    #region ProcessVariableState Class
    #if (!OPCUA_EXCLUDE_ProcessVariableState)
    /// <summary>
    /// Stores an instance of the ProcessVariableType VariableType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ProcessVariableState : DataItemState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ProcessVariableState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.VariableTypes.ProcessVariableType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////xVgiQICAAAAAQAbAAAAUHJvY2Vzc1ZhcmlhYmxlVHlwZUluc3RhbmNl" +
           "AQHYBwEB2AfYBwAAABj+////AQH/////AAAAAA==";
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

    #region ProcessVariableState<T> Class
    /// <summary>
    /// A typed version of the ProcessVariableType variable.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public class ProcessVariableState<T> : ProcessVariableState
    {
        #region Constructors
        /// <summary>
        /// Initializes the instance with its defalt attribute values.
        /// </summary>
        public ProcessVariableState(NodeState parent) : base(parent)
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

    #region MVAModelState Class
    #if (!OPCUA_EXCLUDE_MVAModelState)
    /// <summary>
    /// Stores an instance of the MVAModelType VariableType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class MVAModelState : ChemometricModelState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public MVAModelState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.VariableTypes.MVAModelType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
        }

        /// <summary>
        /// Returns the id of the default data type node for the instance.
        /// </summary>
        protected override NodeId GetDefaultDataTypeId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.DataTypes.ByteString, Opc.Ua.Namespaces.OpcUa, namespaceUris);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////xVgiQICAAAAAQAUAAAATVZBTW9kZWxUeXBlSW5zdGFuY2UBAdkHAQHZ" +
           "B9kHAAAAD/7///8BAf////8EAAAAFWCJCgIAAAABAAQAAABOYW1lAQHxMgAuAETxMgAAABX/////AQH/" +
           "////AAAAABVgiQoCAAAAAQAMAAAAQ3JlYXRpb25EYXRlAQHyMgAuAETyMgAAAA3/////AQH/////AAAA" +
           "ABVgiQoCAAAAAQAQAAAATW9kZWxEZXNjcmlwdGlvbgEB8zIALgBE8zIAAAAV/////wEB/////wAAAAAV" +
           "YIkKAgAAAAEADQAAAE1haW5EYXRhSW5kZXgBAfYyAC4ARPYyAAAABv////8BAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public PropertyState<int> MainDataIndex
        {
            get
            {
                return m_mainDataIndex;
            }

            set
            {
                if (!Object.ReferenceEquals(m_mainDataIndex, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_mainDataIndex = value;
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
            if (m_mainDataIndex != null)
            {
                children.Add(m_mainDataIndex);
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
                case Opc.Ua.Adi.BrowseNames.MainDataIndex:
                {
                    if (createOrReplace)
                    {
                        if (MainDataIndex == null)
                        {
                            if (replacement == null)
                            {
                                MainDataIndex = new PropertyState<int>(this);
                            }
                            else
                            {
                                MainDataIndex = (PropertyState<int>)replacement;
                            }
                        }
                    }

                    instance = MainDataIndex;
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
        private PropertyState<int> m_mainDataIndex;
        #endregion
    }
    #endif
    #endregion

    #region MVAPredictMethodState Class
    #if (!OPCUA_EXCLUDE_MVAPredictMethodState)
    /// <summary>
    /// Stores an instance of the MVAPredictMethodType Method.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class MVAPredictMethodState : MethodState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public MVAPredictMethodState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Constructs an instance of a node.
        /// </summary>
        /// <param name="parent">The parent.</param>
        /// <returns>The new node.</returns>
        public new static NodeState Construct(NodeState parent)
        {
            return new MVAPredictMethodState(parent);
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
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////wRhggoEAAAAAQAUAAAATVZBUHJlZGljdE1ldGhvZFR5cGUBAfsyAC8B" +
           "Afsy+zIAAAEB/////wIAAAAXYKkKAgAAAAAADgAAAElucHV0QXJndW1lbnRzAQH8MgAuAET8MgAAlgMA" +
           "AAABACoBARoAAAALAAAAVGFyZ2V0TW9kZWwAEf////8AAAAAAAEAKgEBHAAAAA0AAABNYWluRGF0YUlu" +
           "ZGV4AAb/////AAAAAAABACoBARkAAAAGAAAASW5wdXRzABgBAAAAAQAAAAAAAAAAAQAoAQEAAAABAAAA" +
           "AAAAAAEB/////wAAAAAXYKkKAgAAAAAADwAAAE91dHB1dEFyZ3VtZW50cwEB/TIALgBE/TIAAJYBAAAA" +
           "AQAqAQEaAAAABwAAAE91dHB1dHMAGAEAAAABAAAAAAAAAAABACgBAQAAAAEAAAAAAAAAAQH/////AAAA" +
           "AA==";
        #endregion
        #endif
        #endregion

        #region Event Callbacks
        /// <summary>
        /// Raised when the the method is called.
        /// </summary>
        public MVAPredictMethodStateMethodCallHandler OnCall;
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

            NodeId targetModel = (NodeId)_inputArguments[0];
            int mainDataIndex = (int)_inputArguments[1];
            Variant[] inputs = (Variant[])_inputArguments[2];

            Variant[] outputs = (Variant[])_outputArguments[0];

            if (OnCall != null)
            {
                result = OnCall(
                    _context,
                    this,
                    _objectId,
                    targetModel,
                    mainDataIndex,
                    inputs,
                    ref outputs);
            }

            _outputArguments[0] = outputs;

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
    public delegate ServiceResult MVAPredictMethodStateMethodCallHandler(
        ISystemContext context,
        MethodState method,
        NodeId objectId,
        NodeId targetModel,
        int mainDataIndex,
        Variant[] inputs,
        ref Variant[] outputs);
    #endif
    #endregion

    #region MVAOutputParameterState Class
    #if (!OPCUA_EXCLUDE_MVAOutputParameterState)
    /// <summary>
    /// Stores an instance of the MVAOutputParameterType VariableType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class MVAOutputParameterState : BaseDataVariableState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public MVAOutputParameterState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Adi.VariableTypes.MVAOutputParameterType, Opc.Ua.Adi.Namespaces.OpcUaAdi, namespaceUris);
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

            if (WarningLimits != null)
            {
                WarningLimits.Initialize(context, WarningLimits_InitializationString);
            }

            if (AlarmLimits != null)
            {
                AlarmLimits.Initialize(context, AlarmLimits_InitializationString);
            }

            if (VendorSpecificError != null)
            {
                VendorSpecificError.Initialize(context, VendorSpecificError_InitializationString);
            }
        }

        #region Initialization String
        private const string WarningLimits_InitializationString =
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////xVgiQoCAAAAAQANAAAAV2FybmluZ0xpbWl0cwEB/jIALgBE/jIAAAEA" +
           "dAP/////AQH/////AAAAAA==";

        private const string AlarmLimits_InitializationString =
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////xVgiQoCAAAAAQALAAAAQWxhcm1MaW1pdHMBAf8yAC4ARP8yAAABAHQD" +
           "/////wEB/////wAAAAA=";

        private const string VendorSpecificError_InitializationString =
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////xVgiQoCAAAAAQATAAAAVmVuZG9yU3BlY2lmaWNFcnJvcgEBATMALgBE" +
           "ATMAAAAM/////wEB/////wAAAAA=";

        private const string InitializationString =
           "AgAAACAAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvQURJLx8AAABodHRwOi8vb3BjZm91bmRh" +
           "dGlvbi5vcmcvVUEvREkv/////xVgiQICAAAAAQAeAAAATVZBT3V0cHV0UGFyYW1ldGVyVHlwZUluc3Rh" +
           "bmNlAQHaBwEB2gfaBwAAABj+////AQH/////BAAAABVgiQoCAAAAAQANAAAAV2FybmluZ0xpbWl0cwEB" +
           "/jIALgBE/jIAAAEAdAP/////AQH/////AAAAABVgiQoCAAAAAQALAAAAQWxhcm1MaW1pdHMBAf8yAC4A" +
           "RP8yAAABAHQD/////wEB/////wAAAAAVYIkKAgAAAAEACgAAAEFsYXJtU3RhdGUBAQAzAC4ARAAzAAAB" +
           "AcEL/////wEB/////wAAAAAVYIkKAgAAAAEAEwAAAFZlbmRvclNwZWNpZmljRXJyb3IBAQEzAC4ARAEz" +
           "AAAADP////8BAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public PropertyState<Range> WarningLimits
        {
            get
            {
                return m_warningLimits;
            }

            set
            {
                if (!Object.ReferenceEquals(m_warningLimits, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_warningLimits = value;
            }
        }

        /// <remarks />
        public PropertyState<Range> AlarmLimits
        {
            get
            {
                return m_alarmLimits;
            }

            set
            {
                if (!Object.ReferenceEquals(m_alarmLimits, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_alarmLimits = value;
            }
        }

        /// <remarks />
        public PropertyState<AlarmStateEnumeration> AlarmState
        {
            get
            {
                return m_alarmState;
            }

            set
            {
                if (!Object.ReferenceEquals(m_alarmState, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_alarmState = value;
            }
        }

        /// <remarks />
        public PropertyState<string> VendorSpecificError
        {
            get
            {
                return m_vendorSpecificError;
            }

            set
            {
                if (!Object.ReferenceEquals(m_vendorSpecificError, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_vendorSpecificError = value;
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
            if (m_warningLimits != null)
            {
                children.Add(m_warningLimits);
            }

            if (m_alarmLimits != null)
            {
                children.Add(m_alarmLimits);
            }

            if (m_alarmState != null)
            {
                children.Add(m_alarmState);
            }

            if (m_vendorSpecificError != null)
            {
                children.Add(m_vendorSpecificError);
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
                case Opc.Ua.Adi.BrowseNames.WarningLimits:
                {
                    if (createOrReplace)
                    {
                        if (WarningLimits == null)
                        {
                            if (replacement == null)
                            {
                                WarningLimits = new PropertyState<Range>(this);
                            }
                            else
                            {
                                WarningLimits = (PropertyState<Range>)replacement;
                            }
                        }
                    }

                    instance = WarningLimits;
                    break;
                }

                case Opc.Ua.Adi.BrowseNames.AlarmLimits:
                {
                    if (createOrReplace)
                    {
                        if (AlarmLimits == null)
                        {
                            if (replacement == null)
                            {
                                AlarmLimits = new PropertyState<Range>(this);
                            }
                            else
                            {
                                AlarmLimits = (PropertyState<Range>)replacement;
                            }
                        }
                    }

                    instance = AlarmLimits;
                    break;
                }

                case Opc.Ua.Adi.BrowseNames.AlarmState:
                {
                    if (createOrReplace)
                    {
                        if (AlarmState == null)
                        {
                            if (replacement == null)
                            {
                                AlarmState = new PropertyState<AlarmStateEnumeration>(this);
                            }
                            else
                            {
                                AlarmState = (PropertyState<AlarmStateEnumeration>)replacement;
                            }
                        }
                    }

                    instance = AlarmState;
                    break;
                }

                case Opc.Ua.Adi.BrowseNames.VendorSpecificError:
                {
                    if (createOrReplace)
                    {
                        if (VendorSpecificError == null)
                        {
                            if (replacement == null)
                            {
                                VendorSpecificError = new PropertyState<string>(this);
                            }
                            else
                            {
                                VendorSpecificError = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = VendorSpecificError;
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
        private PropertyState<Range> m_warningLimits;
        private PropertyState<Range> m_alarmLimits;
        private PropertyState<AlarmStateEnumeration> m_alarmState;
        private PropertyState<string> m_vendorSpecificError;
        #endregion
    }

    #region MVAOutputParameterState<T> Class
    /// <summary>
    /// A typed version of the MVAOutputParameterType variable.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public class MVAOutputParameterState<T> : MVAOutputParameterState
    {
        #region Constructors
        /// <summary>
        /// Initializes the instance with its defalt attribute values.
        /// </summary>
        public MVAOutputParameterState(NodeState parent) : base(parent)
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
