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
using Opc.Ua.Di;

namespace Opc.Ua.Plc
{
    #region CtrlConfigurationState Class
    #if (!OPCUA_EXCLUDE_CtrlConfigurationState)
    /// <summary>
    /// Stores an instance of the CtrlConfigurationType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class CtrlConfigurationState : DeviceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public CtrlConfigurationState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Plc.ObjectTypes.CtrlConfigurationType, Opc.Ua.Plc.Namespaces.OpcUaPlc, namespaceUris);
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
           "AgAAAB8AAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvREkvJAAAAGh0dHA6Ly9QTENvcGVuLm9y" +
           "Zy9PcGNVYS9JRUM2MTEzMS0zL/////8EYIACAQAAAAIAHQAAAEN0cmxDb25maWd1cmF0aW9uVHlwZUlu" +
           "c3RhbmNlAQLpAwEC6QPpAwAA/////w8AAAAkYIAKAQAAAAIACQAAAE1ldGhvZFNldAECihMDAAAAABQA" +
           "AABGbGF0IGxpc3Qgb2YgTWV0aG9kcwAvADqKEwAA/////wIAAAAEYYIKBAAAAAIABQAAAFN0YXJ0AQJZ" +
           "GwAvAQJZG1kbAAABAf////8AAAAABGGCCgQAAAACAAQAAABTdG9wAQJaGwAvAQJaG1obAAABAf////8A" +
           "AAAANWCJCgIAAAABAAwAAABNYW51ZmFjdHVyZXIBAg0AAwAAAAAwAAAATmFtZSBvZiB0aGUgY29tcGFu" +
           "eSB0aGF0IG1hbnVmYWN0dXJlZCB0aGUgZGV2aWNlAC4ARA0AAAAAFf////8BAf////8AAAAANWCJCgIA" +
           "AAABAAUAAABNb2RlbAECDgADAAAAABgAAABNb2RlbCBuYW1lIG9mIHRoZSBkZXZpY2UALgBEDgAAAAAV" +
           "/////wEB/////wAAAAA1YIkKAgAAAAEAEAAAAEhhcmR3YXJlUmV2aXNpb24BAhIAAwAAAAAsAAAAUmV2" +
           "aXNpb24gbGV2ZWwgb2YgdGhlIGhhcmR3YXJlIG9mIHRoZSBkZXZpY2UALgBEEgAAAAAM/////wEB////" +
           "/wAAAAA1YIkKAgAAAAEAEAAAAFNvZnR3YXJlUmV2aXNpb24BAhEAAwAAAAA1AAAAUmV2aXNpb24gbGV2" +
           "ZWwgb2YgdGhlIHNvZnR3YXJlL2Zpcm13YXJlIG9mIHRoZSBkZXZpY2UALgBEEQAAAAAM/////wEB////" +
           "/wAAAAA1YIkKAgAAAAEADgAAAERldmljZVJldmlzaW9uAQIQAAMAAAAAJAAAAE92ZXJhbGwgcmV2aXNp" +
           "b24gbGV2ZWwgb2YgdGhlIGRldmljZQAuAEQQAAAAAAz/////AQH/////AAAAADVgiQoCAAAAAQAMAAAA" +
           "RGV2aWNlTWFudWFsAQIPAAMAAAAAWgAAAEFkZHJlc3MgKHBhdGhuYW1lIGluIHRoZSBmaWxlIHN5c3Rl" +
           "bSBvciBhIFVSTCB8IFdlYiBhZGRyZXNzKSBvZiB1c2VyIG1hbnVhbCBmb3IgdGhlIGRldmljZQAuAEQP" +
           "AAAAAAz/////AQH/////AAAAADVgiQoCAAAAAQAMAAAAU2VyaWFsTnVtYmVyAQILAAMAAAAATQAAAElk" +
           "ZW50aWZpZXIgdGhhdCB1bmlxdWVseSBpZGVudGlmaWVzLCB3aXRoaW4gYSBtYW51ZmFjdHVyZXIsIGEg" +
           "ZGV2aWNlIGluc3RhbmNlAC4ARAsAAAAADP////8BAf////8AAAAANWCJCgIAAAABAA8AAABSZXZpc2lv" +
           "bkNvdW50ZXIBAgwAAwAAAABpAAAAQW4gaW5jcmVtZW50YWwgY291bnRlciBpbmRpY2F0aW5nIHRoZSBu" +
           "dW1iZXIgb2YgdGltZXMgdGhlIHN0YXRpYyBkYXRhIHdpdGhpbiB0aGUgRGV2aWNlIGhhcyBiZWVuIG1v" +
           "ZGlmaWVkAC4ARAwAAAAABv////8BAf////8AAAAABGCACgEAAAACAAkAAABSZXNvdXJjZXMBAowTAC8B" +
           "AewDjBMAAP////8BAAAAJGCACgEAAAABAA4AAABTdXBwb3J0ZWRUeXBlcwECjRMDAAAAAHIAAABGb2xk" +
           "ZXIgbWFpbnRhaW5pbmcgdGhlIHNldCBvZiAoc3ViLXR5cGVzIG9mKSBCYXNlT2JqZWN0VHlwZXMgdGhh" +
           "dCBjYW4gYmUgaW5zdGFudGlhdGVkIGluIHRoZSBDb25maWd1cmFibGVDb21wb25lbnQALwA9jRMAAP//" +
           "//8AAAAABGCACgEAAAACAAoAAABHbG9iYWxWYXJzAQKOEwAvAQHtA44TAAD/////AAAAAARggAoBAAAA" +
           "AgAKAAAAQWNjZXNzVmFycwECjxMALwEB7QOPEwAA/////wAAAAAEYIAKAQAAAAIACgAAAENvbmZpZ1Zh" +
           "cnMBApATAC8BAe0DkBMAAP////8AAAAABGCACgEAAAACAA0AAABDb25maWd1cmF0aW9uAQKREwAvAQHt" +
           "A5ETAAD/////AAAAAARggAoBAAAAAgAKAAAARGlhZ25vc3RpYwECkhMALwEB7QOSEwAA/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public ConfigurableObjectState Resources
        {
            get
            {
                return m_resources;
            }

            set
            {
                if (!Object.ReferenceEquals(m_resources, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_resources = value;
            }
        }

        /// <remarks />
        public FunctionalGroupState GlobalVars
        {
            get
            {
                return m_globalVars;
            }

            set
            {
                if (!Object.ReferenceEquals(m_globalVars, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_globalVars = value;
            }
        }

        /// <remarks />
        public FunctionalGroupState AccessVars
        {
            get
            {
                return m_accessVars;
            }

            set
            {
                if (!Object.ReferenceEquals(m_accessVars, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_accessVars = value;
            }
        }

        /// <remarks />
        public FunctionalGroupState ConfigVars
        {
            get
            {
                return m_configVars;
            }

            set
            {
                if (!Object.ReferenceEquals(m_configVars, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_configVars = value;
            }
        }

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
        public FunctionalGroupState Diagnostic
        {
            get
            {
                return m_diagnostic;
            }

            set
            {
                if (!Object.ReferenceEquals(m_diagnostic, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_diagnostic = value;
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
            if (m_resources != null)
            {
                children.Add(m_resources);
            }

            if (m_globalVars != null)
            {
                children.Add(m_globalVars);
            }

            if (m_accessVars != null)
            {
                children.Add(m_accessVars);
            }

            if (m_configVars != null)
            {
                children.Add(m_configVars);
            }

            if (m_configuration != null)
            {
                children.Add(m_configuration);
            }

            if (m_diagnostic != null)
            {
                children.Add(m_diagnostic);
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
                case Opc.Ua.Plc.BrowseNames.Resources:
                {
                    if (createOrReplace)
                    {
                        if (Resources == null)
                        {
                            if (replacement == null)
                            {
                                Resources = new ConfigurableObjectState(this);
                            }
                            else
                            {
                                Resources = (ConfigurableObjectState)replacement;
                            }
                        }
                    }

                    instance = Resources;
                    break;
                }

                case Opc.Ua.Plc.BrowseNames.GlobalVars:
                {
                    if (createOrReplace)
                    {
                        if (GlobalVars == null)
                        {
                            if (replacement == null)
                            {
                                GlobalVars = new FunctionalGroupState(this);
                            }
                            else
                            {
                                GlobalVars = (FunctionalGroupState)replacement;
                            }
                        }
                    }

                    instance = GlobalVars;
                    break;
                }

                case Opc.Ua.Plc.BrowseNames.AccessVars:
                {
                    if (createOrReplace)
                    {
                        if (AccessVars == null)
                        {
                            if (replacement == null)
                            {
                                AccessVars = new FunctionalGroupState(this);
                            }
                            else
                            {
                                AccessVars = (FunctionalGroupState)replacement;
                            }
                        }
                    }

                    instance = AccessVars;
                    break;
                }

                case Opc.Ua.Plc.BrowseNames.ConfigVars:
                {
                    if (createOrReplace)
                    {
                        if (ConfigVars == null)
                        {
                            if (replacement == null)
                            {
                                ConfigVars = new FunctionalGroupState(this);
                            }
                            else
                            {
                                ConfigVars = (FunctionalGroupState)replacement;
                            }
                        }
                    }

                    instance = ConfigVars;
                    break;
                }

                case Opc.Ua.Plc.BrowseNames.Configuration:
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

                case Opc.Ua.Plc.BrowseNames.Diagnostic:
                {
                    if (createOrReplace)
                    {
                        if (Diagnostic == null)
                        {
                            if (replacement == null)
                            {
                                Diagnostic = new FunctionalGroupState(this);
                            }
                            else
                            {
                                Diagnostic = (FunctionalGroupState)replacement;
                            }
                        }
                    }

                    instance = Diagnostic;
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
        private ConfigurableObjectState m_resources;
        private FunctionalGroupState m_globalVars;
        private FunctionalGroupState m_accessVars;
        private FunctionalGroupState m_configVars;
        private FunctionalGroupState m_configuration;
        private FunctionalGroupState m_diagnostic;
        #endregion
    }
    #endif
    #endregion

    #region CtrlResourceState Class
    #if (!OPCUA_EXCLUDE_CtrlResourceState)
    /// <summary>
    /// Stores an instance of the CtrlResourceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class CtrlResourceState : DeviceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public CtrlResourceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Plc.ObjectTypes.CtrlResourceType, Opc.Ua.Plc.Namespaces.OpcUaPlc, namespaceUris);
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

            if (GlobalVars != null)
            {
                GlobalVars.Initialize(context, GlobalVars_InitializationString);
            }

            if (Configuration != null)
            {
                Configuration.Initialize(context, Configuration_InitializationString);
            }

            if (Diagnostic != null)
            {
                Diagnostic.Initialize(context, Diagnostic_InitializationString);
            }
        }

        #region Initialization String
        private const string MethodSet_InitializationString =
           "AgAAAB8AAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvREkvJAAAAGh0dHA6Ly9QTENvcGVuLm9y" +
           "Zy9PcGNVYS9JRUM2MTEzMS0zL/////8kYIAKAQAAAAIACQAAAE1ldGhvZFNldAEClBMDAAAAABQAAABG" +
           "bGF0IGxpc3Qgb2YgTWV0aG9kcwAvADqUEwAA/////wIAAAAEYYIKBAAAAAIABQAAAFN0YXJ0AQJbGwAv" +
           "AQJbG1sbAAABAf////8AAAAABGGCCgQAAAACAAQAAABTdG9wAQJcGwAvAQJcG1wbAAABAf////8AAAAA";

        private const string GlobalVars_InitializationString =
           "AgAAAB8AAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvREkvJAAAAGh0dHA6Ly9QTENvcGVuLm9y" +
           "Zy9PcGNVYS9JRUM2MTEzMS0zL/////8EYIAKAQAAAAIACgAAAEdsb2JhbFZhcnMBApoTAC8BAe0DmhMA" +
           "AP////8AAAAA";

        private const string Configuration_InitializationString =
           "AgAAAB8AAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvREkvJAAAAGh0dHA6Ly9QTENvcGVuLm9y" +
           "Zy9PcGNVYS9JRUM2MTEzMS0zL/////8EYIAKAQAAAAIADQAAAENvbmZpZ3VyYXRpb24BApsTAC8BAe0D" +
           "mxMAAP////8AAAAA";

        private const string Diagnostic_InitializationString =
           "AgAAAB8AAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvREkvJAAAAGh0dHA6Ly9QTENvcGVuLm9y" +
           "Zy9PcGNVYS9JRUM2MTEzMS0zL/////8EYIAKAQAAAAIACgAAAERpYWdub3N0aWMBApwTAC8BAe0DnBMA" +
           "AP////8AAAAA";

        private const string InitializationString =
           "AgAAAB8AAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvREkvJAAAAGh0dHA6Ly9QTENvcGVuLm9y" +
           "Zy9PcGNVYS9JRUM2MTEzMS0zL/////8EYIACAQAAAAIAGAAAAEN0cmxSZXNvdXJjZVR5cGVJbnN0YW5j" +
           "ZQEC6gMBAuoD6gMAAP////8OAAAAJGCACgEAAAACAAkAAABNZXRob2RTZXQBApQTAwAAAAAUAAAARmxh" +
           "dCBsaXN0IG9mIE1ldGhvZHMALwA6lBMAAP////8CAAAABGGCCgQAAAACAAUAAABTdGFydAECWxsALwEC" +
           "WxtbGwAAAQH/////AAAAAARhggoEAAAAAgAEAAAAU3RvcAECXBsALwECXBtcGwAAAQH/////AAAAADVg" +
           "iQoCAAAAAQAMAAAATWFudWZhY3R1cmVyAQIiAAMAAAAAMAAAAE5hbWUgb2YgdGhlIGNvbXBhbnkgdGhh" +
           "dCBtYW51ZmFjdHVyZWQgdGhlIGRldmljZQAuAEQiAAAAABX/////AQH/////AAAAADVgiQoCAAAAAQAF" +
           "AAAATW9kZWwBAiMAAwAAAAAYAAAATW9kZWwgbmFtZSBvZiB0aGUgZGV2aWNlAC4ARCMAAAAAFf////8B" +
           "Af////8AAAAANWCJCgIAAAABABAAAABIYXJkd2FyZVJldmlzaW9uAQInAAMAAAAALAAAAFJldmlzaW9u" +
           "IGxldmVsIG9mIHRoZSBoYXJkd2FyZSBvZiB0aGUgZGV2aWNlAC4ARCcAAAAADP////8BAf////8AAAAA" +
           "NWCJCgIAAAABABAAAABTb2Z0d2FyZVJldmlzaW9uAQImAAMAAAAANQAAAFJldmlzaW9uIGxldmVsIG9m" +
           "IHRoZSBzb2Z0d2FyZS9maXJtd2FyZSBvZiB0aGUgZGV2aWNlAC4ARCYAAAAADP////8BAf////8AAAAA" +
           "NWCJCgIAAAABAA4AAABEZXZpY2VSZXZpc2lvbgECJQADAAAAACQAAABPdmVyYWxsIHJldmlzaW9uIGxl" +
           "dmVsIG9mIHRoZSBkZXZpY2UALgBEJQAAAAAM/////wEB/////wAAAAA1YIkKAgAAAAEADAAAAERldmlj" +
           "ZU1hbnVhbAECJAADAAAAAFoAAABBZGRyZXNzIChwYXRobmFtZSBpbiB0aGUgZmlsZSBzeXN0ZW0gb3Ig" +
           "YSBVUkwgfCBXZWIgYWRkcmVzcykgb2YgdXNlciBtYW51YWwgZm9yIHRoZSBkZXZpY2UALgBEJAAAAAAM" +
           "/////wEB/////wAAAAA1YIkKAgAAAAEADAAAAFNlcmlhbE51bWJlcgECIAADAAAAAE0AAABJZGVudGlm" +
           "aWVyIHRoYXQgdW5pcXVlbHkgaWRlbnRpZmllcywgd2l0aGluIGEgbWFudWZhY3R1cmVyLCBhIGRldmlj" +
           "ZSBpbnN0YW5jZQAuAEQgAAAAAAz/////AQH/////AAAAADVgiQoCAAAAAQAPAAAAUmV2aXNpb25Db3Vu" +
           "dGVyAQIhAAMAAAAAaQAAAEFuIGluY3JlbWVudGFsIGNvdW50ZXIgaW5kaWNhdGluZyB0aGUgbnVtYmVy" +
           "IG9mIHRpbWVzIHRoZSBzdGF0aWMgZGF0YSB3aXRoaW4gdGhlIERldmljZSBoYXMgYmVlbiBtb2RpZmll" +
           "ZAAuAEQhAAAAAAb/////AQH/////AAAAAARggAoBAAAAAgAFAAAAVGFza3MBApYTAC8BAewDlhMAAP//" +
           "//8BAAAAJGCACgEAAAABAA4AAABTdXBwb3J0ZWRUeXBlcwEClxMDAAAAAHIAAABGb2xkZXIgbWFpbnRh" +
           "aW5pbmcgdGhlIHNldCBvZiAoc3ViLXR5cGVzIG9mKSBCYXNlT2JqZWN0VHlwZXMgdGhhdCBjYW4gYmUg" +
           "aW5zdGFudGlhdGVkIGluIHRoZSBDb25maWd1cmFibGVDb21wb25lbnQALwA9lxMAAP////8AAAAABGCA" +
           "CgEAAAACAAgAAABQcm9ncmFtcwECmBMALwEB7AOYEwAA/////wEAAAAkYIAKAQAAAAEADgAAAFN1cHBv" +
           "cnRlZFR5cGVzAQKZEwMAAAAAcgAAAEZvbGRlciBtYWludGFpbmluZyB0aGUgc2V0IG9mIChzdWItdHlw" +
           "ZXMgb2YpIEJhc2VPYmplY3RUeXBlcyB0aGF0IGNhbiBiZSBpbnN0YW50aWF0ZWQgaW4gdGhlIENvbmZp" +
           "Z3VyYWJsZUNvbXBvbmVudAAvAD2ZEwAA/////wAAAAAEYIAKAQAAAAIACgAAAEdsb2JhbFZhcnMBApoT" +
           "AC8BAe0DmhMAAP////8AAAAABGCACgEAAAACAA0AAABDb25maWd1cmF0aW9uAQKbEwAvAQHtA5sTAAD/" +
           "////AAAAAARggAoBAAAAAgAKAAAARGlhZ25vc3RpYwECnBMALwEB7QOcEwAA/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public ConfigurableObjectState Tasks
        {
            get
            {
                return m_tasks;
            }

            set
            {
                if (!Object.ReferenceEquals(m_tasks, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_tasks = value;
            }
        }

        /// <remarks />
        public ConfigurableObjectState Programs
        {
            get
            {
                return m_programs;
            }

            set
            {
                if (!Object.ReferenceEquals(m_programs, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_programs = value;
            }
        }

        /// <remarks />
        public FunctionalGroupState GlobalVars
        {
            get
            {
                return m_globalVars;
            }

            set
            {
                if (!Object.ReferenceEquals(m_globalVars, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_globalVars = value;
            }
        }

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
        public FunctionalGroupState Diagnostic
        {
            get
            {
                return m_diagnostic;
            }

            set
            {
                if (!Object.ReferenceEquals(m_diagnostic, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_diagnostic = value;
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
            if (m_tasks != null)
            {
                children.Add(m_tasks);
            }

            if (m_programs != null)
            {
                children.Add(m_programs);
            }

            if (m_globalVars != null)
            {
                children.Add(m_globalVars);
            }

            if (m_configuration != null)
            {
                children.Add(m_configuration);
            }

            if (m_diagnostic != null)
            {
                children.Add(m_diagnostic);
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
                case Opc.Ua.Plc.BrowseNames.Tasks:
                {
                    if (createOrReplace)
                    {
                        if (Tasks == null)
                        {
                            if (replacement == null)
                            {
                                Tasks = new ConfigurableObjectState(this);
                            }
                            else
                            {
                                Tasks = (ConfigurableObjectState)replacement;
                            }
                        }
                    }

                    instance = Tasks;
                    break;
                }

                case Opc.Ua.Plc.BrowseNames.Programs:
                {
                    if (createOrReplace)
                    {
                        if (Programs == null)
                        {
                            if (replacement == null)
                            {
                                Programs = new ConfigurableObjectState(this);
                            }
                            else
                            {
                                Programs = (ConfigurableObjectState)replacement;
                            }
                        }
                    }

                    instance = Programs;
                    break;
                }

                case Opc.Ua.Plc.BrowseNames.GlobalVars:
                {
                    if (createOrReplace)
                    {
                        if (GlobalVars == null)
                        {
                            if (replacement == null)
                            {
                                GlobalVars = new FunctionalGroupState(this);
                            }
                            else
                            {
                                GlobalVars = (FunctionalGroupState)replacement;
                            }
                        }
                    }

                    instance = GlobalVars;
                    break;
                }

                case Opc.Ua.Plc.BrowseNames.Configuration:
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

                case Opc.Ua.Plc.BrowseNames.Diagnostic:
                {
                    if (createOrReplace)
                    {
                        if (Diagnostic == null)
                        {
                            if (replacement == null)
                            {
                                Diagnostic = new FunctionalGroupState(this);
                            }
                            else
                            {
                                Diagnostic = (FunctionalGroupState)replacement;
                            }
                        }
                    }

                    instance = Diagnostic;
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
        private ConfigurableObjectState m_tasks;
        private ConfigurableObjectState m_programs;
        private FunctionalGroupState m_globalVars;
        private FunctionalGroupState m_configuration;
        private FunctionalGroupState m_diagnostic;
        #endregion
    }
    #endif
    #endregion

    #region CtrlProgramOrganizationUnitState Class
    #if (!OPCUA_EXCLUDE_CtrlProgramOrganizationUnitState)
    /// <summary>
    /// Stores an instance of the CtrlProgramOrganizationUnitType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class CtrlProgramOrganizationUnitState : BlockState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public CtrlProgramOrganizationUnitState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Plc.ObjectTypes.CtrlProgramOrganizationUnitType, Opc.Ua.Plc.Namespaces.OpcUaPlc, namespaceUris);
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

            if (Body != null)
            {
                Body.Initialize(context, Body_InitializationString);
            }
        }

        #region Initialization String
        private const string Body_InitializationString =
           "AgAAAB8AAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvREkvJAAAAGh0dHA6Ly9QTENvcGVuLm9y" +
           "Zy9PcGNVYS9JRUM2MTEzMS0zL/////8VYIkKAgAAAAIABAAAAEJvZHkBAnEXAC8AP3EXAAAAEP////8B" +
           "Af////8AAAAA";

        private const string InitializationString =
           "AgAAAB8AAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvREkvJAAAAGh0dHA6Ly9QTENvcGVuLm9y" +
           "Zy9PcGNVYS9JRUM2MTEzMS0zL/////8EYIACAQAAAAIAJwAAAEN0cmxQcm9ncmFtT3JnYW5pemF0aW9u" +
           "VW5pdFR5cGVJbnN0YW5jZQEC6wMBAusD6wMAAP////8BAAAAFWCJCgIAAAACAAQAAABCb2R5AQJxFwAv" +
           "AD9xFwAAABD/////AQH/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public BaseDataVariableState<XmlElement> Body
        {
            get
            {
                return m_body;
            }

            set
            {
                if (!Object.ReferenceEquals(m_body, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_body = value;
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
            if (m_body != null)
            {
                children.Add(m_body);
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
                case Opc.Ua.Plc.BrowseNames.Body:
                {
                    if (createOrReplace)
                    {
                        if (Body == null)
                        {
                            if (replacement == null)
                            {
                                Body = new BaseDataVariableState<XmlElement>(this);
                            }
                            else
                            {
                                Body = (BaseDataVariableState<XmlElement>)replacement;
                            }
                        }
                    }

                    instance = Body;
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
        private BaseDataVariableState<XmlElement> m_body;
        #endregion
    }
    #endif
    #endregion

    #region CtrlProgramState Class
    #if (!OPCUA_EXCLUDE_CtrlProgramState)
    /// <summary>
    /// Stores an instance of the CtrlProgramType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class CtrlProgramState : CtrlProgramOrganizationUnitState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public CtrlProgramState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Plc.ObjectTypes.CtrlProgramType, Opc.Ua.Plc.Namespaces.OpcUaPlc, namespaceUris);
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

            if (Program != null)
            {
                Program.Initialize(context, Program_InitializationString);
            }
        }

        #region Initialization String
        private const string Program_InitializationString =
           "AgAAAB8AAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvREkvJAAAAGh0dHA6Ly9QTENvcGVuLm9y" +
           "Zy9PcGNVYS9JRUM2MTEzMS0zL/////8VYIkKAgAAAAIABwAAAFByb2dyYW0BAnIXAC8AP3IXAAAAFv//" +
           "//8BAf////8AAAAA";

        private const string InitializationString =
           "AgAAAB8AAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvREkvJAAAAGh0dHA6Ly9QTENvcGVuLm9y" +
           "Zy9PcGNVYS9JRUM2MTEzMS0zL/////8EYIACAQAAAAIAFwAAAEN0cmxQcm9ncmFtVHlwZUluc3RhbmNl" +
           "AQLsAwEC7APsAwAA/////wEAAAAVYIkKAgAAAAIABwAAAFByb2dyYW0BAnIXAC8AP3IXAAAAFv////8B" +
           "Af////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public BaseDataVariableState<ExtensionObject> Program
        {
            get
            {
                return m_program;
            }

            set
            {
                if (!Object.ReferenceEquals(m_program, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_program = value;
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
            if (m_program != null)
            {
                children.Add(m_program);
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
                case Opc.Ua.Plc.BrowseNames.Program:
                {
                    if (createOrReplace)
                    {
                        if (Program == null)
                        {
                            if (replacement == null)
                            {
                                Program = new BaseDataVariableState<ExtensionObject>(this);
                            }
                            else
                            {
                                Program = (BaseDataVariableState<ExtensionObject>)replacement;
                            }
                        }
                    }

                    instance = Program;
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
        private BaseDataVariableState<ExtensionObject> m_program;
        #endregion
    }
    #endif
    #endregion

    #region CtrlFunctionBlockState Class
    #if (!OPCUA_EXCLUDE_CtrlFunctionBlockState)
    /// <summary>
    /// Stores an instance of the CtrlFunctionBlockType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class CtrlFunctionBlockState : CtrlProgramOrganizationUnitState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public CtrlFunctionBlockState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Plc.ObjectTypes.CtrlFunctionBlockType, Opc.Ua.Plc.Namespaces.OpcUaPlc, namespaceUris);
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

            if (FunctionBlock != null)
            {
                FunctionBlock.Initialize(context, FunctionBlock_InitializationString);
            }
        }

        #region Initialization String
        private const string FunctionBlock_InitializationString =
           "AgAAAB8AAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvREkvJAAAAGh0dHA6Ly9QTENvcGVuLm9y" +
           "Zy9PcGNVYS9JRUM2MTEzMS0zL/////8VYIkKAgAAAAIADQAAAEZ1bmN0aW9uQmxvY2sBAnMXAC8AP3MX" +
           "AAAAGP////8BAf////8AAAAA";

        private const string InitializationString =
           "AgAAAB8AAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvREkvJAAAAGh0dHA6Ly9QTENvcGVuLm9y" +
           "Zy9PcGNVYS9JRUM2MTEzMS0zL/////8EYIACAQAAAAIAHQAAAEN0cmxGdW5jdGlvbkJsb2NrVHlwZUlu" +
           "c3RhbmNlAQLtAwEC7QPtAwAA/////wEAAAAVYIkKAgAAAAIADQAAAEZ1bmN0aW9uQmxvY2sBAnMXAC8A" +
           "P3MXAAAAGP////8BAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public BaseDataVariableState FunctionBlock
        {
            get
            {
                return m_functionBlock;
            }

            set
            {
                if (!Object.ReferenceEquals(m_functionBlock, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_functionBlock = value;
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
            if (m_functionBlock != null)
            {
                children.Add(m_functionBlock);
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
                case Opc.Ua.Plc.BrowseNames.FunctionBlock:
                {
                    if (createOrReplace)
                    {
                        if (FunctionBlock == null)
                        {
                            if (replacement == null)
                            {
                                FunctionBlock = new BaseDataVariableState(this);
                            }
                            else
                            {
                                FunctionBlock = (BaseDataVariableState)replacement;
                            }
                        }
                    }

                    instance = FunctionBlock;
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
        private BaseDataVariableState m_functionBlock;
        #endregion
    }
    #endif
    #endregion

    #region CtrlTaskState Class
    #if (!OPCUA_EXCLUDE_CtrlTaskState)
    /// <summary>
    /// Stores an instance of the CtrlTaskType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class CtrlTaskState : BaseObjectState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public CtrlTaskState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Plc.ObjectTypes.CtrlTaskType, Opc.Ua.Plc.Namespaces.OpcUaPlc, namespaceUris);
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

            if (Interval != null)
            {
                Interval.Initialize(context, Interval_InitializationString);
            }

            if (Single != null)
            {
                Single.Initialize(context, Single_InitializationString);
            }
        }

        #region Initialization String
        private const string Interval_InitializationString =
           "AgAAAB8AAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvREkvJAAAAGh0dHA6Ly9QTENvcGVuLm9y" +
           "Zy9PcGNVYS9JRUM2MTEzMS0zL/////8VYIkKAgAAAAIACAAAAEludGVydmFsAQJ1FwAuAER1FwAAAAz/" +
           "////AQH/////AAAAAA==";

        private const string Single_InitializationString =
           "AgAAAB8AAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvREkvJAAAAGh0dHA6Ly9QTENvcGVuLm9y" +
           "Zy9PcGNVYS9JRUM2MTEzMS0zL/////8VYIkKAgAAAAIABgAAAFNpbmdsZQECdhcALgBEdhcAAAAM////" +
           "/wEB/////wAAAAA=";

        private const string InitializationString =
           "AgAAAB8AAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvREkvJAAAAGh0dHA6Ly9QTENvcGVuLm9y" +
           "Zy9PcGNVYS9JRUM2MTEzMS0zL/////8EYIACAQAAAAIAFAAAAEN0cmxUYXNrVHlwZUluc3RhbmNlAQLu" +
           "AwEC7gPuAwAA/////wMAAAAVYIkKAgAAAAIACAAAAFByaW9yaXR5AQJ0FwAuAER0FwAAAAf/////AQH/" +
           "////AAAAABVgiQoCAAAAAgAIAAAASW50ZXJ2YWwBAnUXAC4ARHUXAAAADP////8BAf////8AAAAAFWCJ" +
           "CgIAAAACAAYAAABTaW5nbGUBAnYXAC4ARHYXAAAADP////8BAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public PropertyState<uint> Priority
        {
            get
            {
                return m_priority;
            }

            set
            {
                if (!Object.ReferenceEquals(m_priority, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_priority = value;
            }
        }

        /// <remarks />
        public PropertyState<string> Interval
        {
            get
            {
                return m_interval;
            }

            set
            {
                if (!Object.ReferenceEquals(m_interval, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_interval = value;
            }
        }

        /// <remarks />
        public PropertyState<string> Single
        {
            get
            {
                return m_single;
            }

            set
            {
                if (!Object.ReferenceEquals(m_single, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_single = value;
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
            if (m_priority != null)
            {
                children.Add(m_priority);
            }

            if (m_interval != null)
            {
                children.Add(m_interval);
            }

            if (m_single != null)
            {
                children.Add(m_single);
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
                case Opc.Ua.Plc.BrowseNames.Priority:
                {
                    if (createOrReplace)
                    {
                        if (Priority == null)
                        {
                            if (replacement == null)
                            {
                                Priority = new PropertyState<uint>(this);
                            }
                            else
                            {
                                Priority = (PropertyState<uint>)replacement;
                            }
                        }
                    }

                    instance = Priority;
                    break;
                }

                case Opc.Ua.Plc.BrowseNames.Interval:
                {
                    if (createOrReplace)
                    {
                        if (Interval == null)
                        {
                            if (replacement == null)
                            {
                                Interval = new PropertyState<string>(this);
                            }
                            else
                            {
                                Interval = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = Interval;
                    break;
                }

                case Opc.Ua.Plc.BrowseNames.Single:
                {
                    if (createOrReplace)
                    {
                        if (Single == null)
                        {
                            if (replacement == null)
                            {
                                Single = new PropertyState<string>(this);
                            }
                            else
                            {
                                Single = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = Single;
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
        private PropertyState<uint> m_priority;
        private PropertyState<string> m_interval;
        private PropertyState<string> m_single;
        #endregion
    }
    #endif
    #endregion

    #region SFCState Class
    #if (!OPCUA_EXCLUDE_SFCState)
    /// <summary>
    /// Stores an instance of the SFCType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class SFCState : BaseObjectState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public SFCState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Plc.ObjectTypes.SFCType, Opc.Ua.Plc.Namespaces.OpcUaPlc, namespaceUris);
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
           "AgAAAB8AAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvREkvJAAAAGh0dHA6Ly9QTENvcGVuLm9y" +
           "Zy9PcGNVYS9JRUM2MTEzMS0zL/////8EYIACAQAAAAIADwAAAFNGQ1R5cGVJbnN0YW5jZQEC7wMBAu8D" +
           "7wMAAP////8AAAAA";
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
