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
using Opc.Ua.Di;
using Opc.Ua;

namespace Opc.Ua.Robotics
{
    #region MotionDeviceSystemState Class
    #if (!OPCUA_EXCLUDE_MotionDeviceSystemState)
    /// <summary>
    /// Stores an instance of the MotionDeviceSystemType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class MotionDeviceSystemState : ComponentState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public MotionDeviceSystemState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Robotics.ObjectTypes.MotionDeviceSystemType, Opc.Ua.Robotics.Namespaces.OpcUaRobotics, namespaceUris);
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
           "AgAAACUAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvUm9ib3RpY3MvHwAAAGh0dHA6Ly9vcGNm" +
           "b3VuZGF0aW9uLm9yZy9VQS9ESS//////hGCAAgEAAAABAB4AAABNb3Rpb25EZXZpY2VTeXN0ZW1UeXBl" +
           "SW5zdGFuY2UBAeoDAQHqA+oDAAAB/////wMAAAAkYIAKAQAAAAEADQAAAE1vdGlvbkRldmljZXMBAYoT" +
           "AwAAAABSAAAAQ29udGFpbnMgYW55IGtpbmVtYXRpYyBvciBtb3Rpb24gZGV2aWNlIHdoaWNoIGlzIHBh" +
           "cnQgb2YgdGhlIG1vdGlvbiBkZXZpY2Ugc3lzdGVtLgAvAD2KEwAA/////wAAAAAkYIAKAQAAAAEACwAA" +
           "AENvbnRyb2xsZXJzAQGJEwMAAAAAPAAAAENvbnRhaW5zIHRoZSBzZXQgb2YgY29udHJvbGxlcnMgaW4g" +
           "dGhlIG1vdGlvbiBkZXZpY2Ugc3lzdGVtLgAvAD2JEwAA/////wAAAAAkYIAKAQAAAAEADAAAAFNhZmV0" +
           "eVN0YXRlcwEBkhMDAAAAADcAAABDb250YWlucyBzYWZldHktcmVsYXRlZCBkYXRhIGZyb20gbW90aW9u" +
           "IGRldmljZSBzeXN0ZW0uAC8APZITAAD/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public FolderState MotionDevices
        {
            get
            {
                return m_motionDevices;
            }

            set
            {
                if (!Object.ReferenceEquals(m_motionDevices, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_motionDevices = value;
            }
        }

        /// <remarks />
        public FolderState Controllers
        {
            get
            {
                return m_controllers;
            }

            set
            {
                if (!Object.ReferenceEquals(m_controllers, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_controllers = value;
            }
        }

        /// <remarks />
        public FolderState SafetyStates
        {
            get
            {
                return m_safetyStates;
            }

            set
            {
                if (!Object.ReferenceEquals(m_safetyStates, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_safetyStates = value;
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
            if (m_motionDevices != null)
            {
                children.Add(m_motionDevices);
            }

            if (m_controllers != null)
            {
                children.Add(m_controllers);
            }

            if (m_safetyStates != null)
            {
                children.Add(m_safetyStates);
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
                case Opc.Ua.Robotics.BrowseNames.MotionDevices:
                {
                    if (createOrReplace)
                    {
                        if (MotionDevices == null)
                        {
                            if (replacement == null)
                            {
                                MotionDevices = new FolderState(this);
                            }
                            else
                            {
                                MotionDevices = (FolderState)replacement;
                            }
                        }
                    }

                    instance = MotionDevices;
                    break;
                }

                case Opc.Ua.Robotics.BrowseNames.Controllers:
                {
                    if (createOrReplace)
                    {
                        if (Controllers == null)
                        {
                            if (replacement == null)
                            {
                                Controllers = new FolderState(this);
                            }
                            else
                            {
                                Controllers = (FolderState)replacement;
                            }
                        }
                    }

                    instance = Controllers;
                    break;
                }

                case Opc.Ua.Robotics.BrowseNames.SafetyStates:
                {
                    if (createOrReplace)
                    {
                        if (SafetyStates == null)
                        {
                            if (replacement == null)
                            {
                                SafetyStates = new FolderState(this);
                            }
                            else
                            {
                                SafetyStates = (FolderState)replacement;
                            }
                        }
                    }

                    instance = SafetyStates;
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
        private FolderState m_motionDevices;
        private FolderState m_controllers;
        private FolderState m_safetyStates;
        #endregion
    }
    #endif
    #endregion

    #region MotionDeviceState Class
    #if (!OPCUA_EXCLUDE_MotionDeviceState)
    /// <summary>
    /// Stores an instance of the MotionDeviceType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class MotionDeviceState : ComponentState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public MotionDeviceState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Robotics.ObjectTypes.MotionDeviceType, Opc.Ua.Robotics.Namespaces.OpcUaRobotics, namespaceUris);
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

            if (FlangeLoad != null)
            {
                FlangeLoad.Initialize(context, FlangeLoad_InitializationString);
            }

            if (AdditionalComponents != null)
            {
                AdditionalComponents.Initialize(context, AdditionalComponents_InitializationString);
            }
        }

        #region Initialization String
        private const string FlangeLoad_InitializationString =
           "AgAAACUAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvUm9ib3RpY3MvHwAAAGh0dHA6Ly9vcGNm" +
           "b3VuZGF0aW9uLm9yZy9VQS9ESS//////JGCACgEAAAABAAoAAABGbGFuZ2VMb2FkAQHjEwMAAAAAiAAA" +
           "AFRoZSBGbGFuZ2VMb2FkIGlzIHRoZSBsb2FkIG9uIHRoZSBmbGFuZ2Ugb3IgYXQgdGhlIG1vdW50aW5n" +
           "IHBvaW50IG9mIHRoZSBNb3Rpb25EZXZpY2UuIFRoaXMgY2FuIGJlIHRoZSBtYXhpbXVtIGxvYWQgb2Yg" +
           "dGhlIE1vdGlvbkRldmljZS4ALwEB+gPjEwAA/////wEAAAA1YIkKAgAAAAEABAAAAE1hc3MBAeAZAwAA" +
           "AAA1AAAAVGhlIHdlaWdodCBvZiB0aGUgbG9hZCBtb3VudGVkIG9uIG9uZSBtb3VudGluZyBwb2ludC4A" +
           "LwEAWUTgGQAAAAv/////AQH/////AQAAABVgiQoCAAAAAAAQAAAARW5naW5lZXJpbmdVbml0cwEBKz0A" +
           "LgBEKz0AAAEAdwP/////AQH/////AAAAAA==";

        private const string AdditionalComponents_InitializationString =
           "AgAAACUAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvUm9ib3RpY3MvHwAAAGh0dHA6Ly9vcGNm" +
           "b3VuZGF0aW9uLm9yZy9VQS9ESS//////JGCACgEAAAABABQAAABBZGRpdGlvbmFsQ29tcG9uZW50cwEB" +
           "tkADAAAAALwAAABBZGRpdGlvbmFsQ29tcG9uZW50cyBpcyBhIGNvbnRhaW5lciBmb3Igb25lIG9yIG1v" +
           "cmUgaW5zdGFuY2VzIG9mIHN1YnR5cGVzIG9mIENvbXBvbmVudFR5cGUgZGVmaW5lZCBpbiBPUEMgVUEg" +
           "REkuIFRoZSBsaXN0ZWQgY29tcG9uZW50cyBhcmUgaW5zdGFsbGVkIGF0IHRoZSBtb3Rpb24gZGV2aWNl" +
           "LCBlLmcuIGFuIElPLWJvYXJkLgAvAD22QAAA/////wAAAAA=";

        private const string InitializationString =
           "AgAAACUAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvUm9ib3RpY3MvHwAAAGh0dHA6Ly9vcGNm" +
           "b3VuZGF0aW9uLm9yZy9VQS9ESS//////BGCAAgEAAAABABgAAABNb3Rpb25EZXZpY2VUeXBlSW5zdGFu" +
           "Y2UBAewDAQHsA+wDAAD/////CgAAACRggAoBAAAAAgAMAAAAUGFyYW1ldGVyU2V0AQGlEwMAAAAAFwAA" +
           "AEZsYXQgbGlzdCBvZiBQYXJhbWV0ZXJzAC8AOqUTAAD/////AwAAADVgiQoCAAAAAQAGAAAAT25QYXRo" +
           "AQHrPwMAAAAASwEAAE9uUGF0aCBpcyB0cnVlIGlmIHRoZSBtb3Rpb24gZGV2aWNlIGlzIG9uIG9yIG5l" +
           "YXIgZW5vdWdoIHRoZSBwbGFubmVkIHByb2dyYW0gcGF0aCBzdWNoIHRoYXQgcHJvZ3JhbSBleGVjdXRp" +
           "b24gY2FuIGNvbnRpbnVlLiBJZiB0aGUgTW90aW9uRGV2aWNlIGRldmlhdGVzIHRvbyBtdWNoIGZyb20g" +
           "dGhpcyBwYXRoIGluIGNhc2Ugb2YgZXJyb3JzIG9yIGFuIGVtZXJnZW5jeSBzdG9wLCB0aGlzIHZhbHVl" +
           "IGJlY29tZXMgZmFsc2UuIElmIE9uUGF0aCBpcyBmYWxzZSwgdGhlIG1vdGlvbiBkZXZpY2UgbmVlZHMg" +
           "cmVwb3NpdGlvbmluZyB0byBjb250aW51ZSBwcm9ncmFtIGV4ZWN1dGlvbi4ALwA/6z8AAAAB/////wEB" +
           "/////wAAAAA1YIkKAgAAAAEACQAAAEluQ29udHJvbAEB7D8DAAAAALMAAABJbkNvbnRyb2wgcHJvdmlk" +
           "ZXMgdGhlIGluZm9ybWF0aW9uIGlmIHRoZSBhY3R1YXRvcnMgKGluIG1vc3QgY2FzZXMgYSBtb3Rvcikg" +
           "b2YgdGhlIG1vdGlvbiBkZXZpY2UgYXJlIHBvd2VyZWQgdXAgYW5kIGluIGNvbnRyb2w6ICJ0cnVlIi4g" +
           "VGhlIG1vdGlvbiBkZXZpY2UgbWlnaHQgYmUgaW4gYSBzdGFuZHN0aWxsLgAvAD/sPwAAAAH/////AQH/" +
           "////AAAAADVgiQoCAAAAAQANAAAAU3BlZWRPdmVycmlkZQEB7T8DAAAAAFsAAABTcGVlZE92ZXJyaWRl" +
           "IHByb3ZpZGVzIHRoZSBjdXJyZW50IHNwZWVkIHNldHRpbmcgaW4gcGVyY2VudCBvZiBwcm9ncmFtbWVk" +
           "IHNwZWVkICgwIC0gMTAwJSkuAC8AP+0/AAAAC/////8BAf////8AAAAAFWCJCgIAAAACAAwAAABNYW51" +
           "ZmFjdHVyZXIBAd8/AC4ARN8/AAAAFf////8BAf////8AAAAAFWCJCgIAAAACAAUAAABNb2RlbAEB4T8A" +
           "LgBE4T8AAAAV/////wEB/////wAAAAAVYIkKAgAAAAIACwAAAFByb2R1Y3RDb2RlAQHnPwAuAETnPwAA" +
           "AAz/////AQH/////AAAAABVgiQoCAAAAAgAMAAAAU2VyaWFsTnVtYmVyAQHiPwAuAETiPwAAAAz/////" +
           "AQH/////AAAAADVgiQoCAAAAAQAUAAAATW90aW9uRGV2aWNlQ2F0ZWdvcnkBAeo/AwAAAACCAAAAVGhl" +
           "IHZhcmlhYmxlIE1vdGlvbkRldmljZUNhdGVnb3J5IHByb3ZpZGVzIHRoZSBraW5kIG9mIG1vdGlvbiBk" +
           "ZXZpY2UgZGVmaW5lZCBieSBNb3Rpb25EZXZpY2VDYXRlZ29yeUVudW1lcmF0aW9uIGJhc2VkIG9uIElT" +
           "TyA4MzczLgAuAETqPwAAAQERR/////8BAf////8AAAAAJGCACgEAAAABAAQAAABBeGVzAQHJOwMAAAAA" +
           "PgAAAEF4ZXMgaXMgYSBjb250YWluZXIgZm9yIG9uZSBvciBtb3JlIGluc3RhbmNlcyBvZiB0aGUgQXhp" +
           "c1R5cGUuAC8APck7AAD/////AAAAACRggAoBAAAAAQALAAAAUG93ZXJUcmFpbnMBATtAAwAAAABLAAAA" +
           "UG93ZXJUcmFpbnMgaXMgYSBjb250YWluZXIgZm9yIG9uZSBvciBtb3JlIGluc3RhbmNlcyBvZiB0aGUg" +
           "UG93ZXJUcmFpblR5cGUuAC8APTtAAAD/////AAAAACRggAoBAAAAAQAKAAAARmxhbmdlTG9hZAEB4xMD" +
           "AAAAAIgAAABUaGUgRmxhbmdlTG9hZCBpcyB0aGUgbG9hZCBvbiB0aGUgZmxhbmdlIG9yIGF0IHRoZSBt" +
           "b3VudGluZyBwb2ludCBvZiB0aGUgTW90aW9uRGV2aWNlLiBUaGlzIGNhbiBiZSB0aGUgbWF4aW11bSBs" +
           "b2FkIG9mIHRoZSBNb3Rpb25EZXZpY2UuAC8BAfoD4xMAAP////8BAAAANWCJCgIAAAABAAQAAABNYXNz" +
           "AQHgGQMAAAAANQAAAFRoZSB3ZWlnaHQgb2YgdGhlIGxvYWQgbW91bnRlZCBvbiBvbmUgbW91bnRpbmcg" +
           "cG9pbnQuAC8BAFlE4BkAAAAL/////wEB/////wEAAAAVYIkKAgAAAAAAEAAAAEVuZ2luZWVyaW5nVW5p" +
           "dHMBASs9AC4ARCs9AAABAHcD/////wEB/////wAAAAAkYIAKAQAAAAEAFAAAAEFkZGl0aW9uYWxDb21w" +
           "b25lbnRzAQG2QAMAAAAAvAAAAEFkZGl0aW9uYWxDb21wb25lbnRzIGlzIGEgY29udGFpbmVyIGZvciBv" +
           "bmUgb3IgbW9yZSBpbnN0YW5jZXMgb2Ygc3VidHlwZXMgb2YgQ29tcG9uZW50VHlwZSBkZWZpbmVkIGlu" +
           "IE9QQyBVQSBESS4gVGhlIGxpc3RlZCBjb21wb25lbnRzIGFyZSBpbnN0YWxsZWQgYXQgdGhlIG1vdGlv" +
           "biBkZXZpY2UsIGUuZy4gYW4gSU8tYm9hcmQuAC8APbZAAAD/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public PropertyState<MotionDeviceCategoryEnumeration> MotionDeviceCategory
        {
            get
            {
                return m_motionDeviceCategory;
            }

            set
            {
                if (!Object.ReferenceEquals(m_motionDeviceCategory, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_motionDeviceCategory = value;
            }
        }

        /// <remarks />
        public FolderState Axes
        {
            get
            {
                return m_axes;
            }

            set
            {
                if (!Object.ReferenceEquals(m_axes, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_axes = value;
            }
        }

        /// <remarks />
        public FolderState PowerTrains
        {
            get
            {
                return m_powerTrains;
            }

            set
            {
                if (!Object.ReferenceEquals(m_powerTrains, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_powerTrains = value;
            }
        }

        /// <remarks />
        public LoadState FlangeLoad
        {
            get
            {
                return m_flangeLoad;
            }

            set
            {
                if (!Object.ReferenceEquals(m_flangeLoad, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_flangeLoad = value;
            }
        }

        /// <remarks />
        public FolderState AdditionalComponents
        {
            get
            {
                return m_additionalComponents;
            }

            set
            {
                if (!Object.ReferenceEquals(m_additionalComponents, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_additionalComponents = value;
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
            if (m_motionDeviceCategory != null)
            {
                children.Add(m_motionDeviceCategory);
            }

            if (m_axes != null)
            {
                children.Add(m_axes);
            }

            if (m_powerTrains != null)
            {
                children.Add(m_powerTrains);
            }

            if (m_flangeLoad != null)
            {
                children.Add(m_flangeLoad);
            }

            if (m_additionalComponents != null)
            {
                children.Add(m_additionalComponents);
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
                case Opc.Ua.Robotics.BrowseNames.MotionDeviceCategory:
                {
                    if (createOrReplace)
                    {
                        if (MotionDeviceCategory == null)
                        {
                            if (replacement == null)
                            {
                                MotionDeviceCategory = new PropertyState<MotionDeviceCategoryEnumeration>(this);
                            }
                            else
                            {
                                MotionDeviceCategory = (PropertyState<MotionDeviceCategoryEnumeration>)replacement;
                            }
                        }
                    }

                    instance = MotionDeviceCategory;
                    break;
                }

                case Opc.Ua.Robotics.BrowseNames.Axes:
                {
                    if (createOrReplace)
                    {
                        if (Axes == null)
                        {
                            if (replacement == null)
                            {
                                Axes = new FolderState(this);
                            }
                            else
                            {
                                Axes = (FolderState)replacement;
                            }
                        }
                    }

                    instance = Axes;
                    break;
                }

                case Opc.Ua.Robotics.BrowseNames.PowerTrains:
                {
                    if (createOrReplace)
                    {
                        if (PowerTrains == null)
                        {
                            if (replacement == null)
                            {
                                PowerTrains = new FolderState(this);
                            }
                            else
                            {
                                PowerTrains = (FolderState)replacement;
                            }
                        }
                    }

                    instance = PowerTrains;
                    break;
                }

                case Opc.Ua.Robotics.BrowseNames.FlangeLoad:
                {
                    if (createOrReplace)
                    {
                        if (FlangeLoad == null)
                        {
                            if (replacement == null)
                            {
                                FlangeLoad = new LoadState(this);
                            }
                            else
                            {
                                FlangeLoad = (LoadState)replacement;
                            }
                        }
                    }

                    instance = FlangeLoad;
                    break;
                }

                case Opc.Ua.Robotics.BrowseNames.AdditionalComponents:
                {
                    if (createOrReplace)
                    {
                        if (AdditionalComponents == null)
                        {
                            if (replacement == null)
                            {
                                AdditionalComponents = new FolderState(this);
                            }
                            else
                            {
                                AdditionalComponents = (FolderState)replacement;
                            }
                        }
                    }

                    instance = AdditionalComponents;
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
        private PropertyState<MotionDeviceCategoryEnumeration> m_motionDeviceCategory;
        private FolderState m_axes;
        private FolderState m_powerTrains;
        private LoadState m_flangeLoad;
        private FolderState m_additionalComponents;
        #endregion
    }
    #endif
    #endregion

    #region AxisState Class
    #if (!OPCUA_EXCLUDE_AxisState)
    /// <summary>
    /// Stores an instance of the AxisType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class AxisState : ComponentState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public AxisState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Robotics.ObjectTypes.AxisType, Opc.Ua.Robotics.Namespaces.OpcUaRobotics, namespaceUris);
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

            if (AdditionalLoad != null)
            {
                AdditionalLoad.Initialize(context, AdditionalLoad_InitializationString);
            }
        }

        #region Initialization String
        private const string AdditionalLoad_InitializationString =
           "AgAAACUAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvUm9ib3RpY3MvHwAAAGh0dHA6Ly9vcGNm" +
           "b3VuZGF0aW9uLm9yZy9VQS9ESS//////JGCACgEAAAABAA4AAABBZGRpdGlvbmFsTG9hZAEB/kADAAAA" +
           "AGMAAABUaGUgYWRkaXRpb25hbCBsb2FkIHdoaWNoIGlzIG1vdW50ZWQgb24gdGhpcyBheGlzLiBFLmcu" +
           "IGZvciBwcm9jZXNzLW5lZWQgYSB0cmFuc2Zvcm1lciBmb3Igd2VsZGluZy4ALwEB+gP+QAAA/////wEA" +
           "AAA1YIkKAgAAAAEABAAAAE1hc3MBAf9AAwAAAAA1AAAAVGhlIHdlaWdodCBvZiB0aGUgbG9hZCBtb3Vu" +
           "dGVkIG9uIG9uZSBtb3VudGluZyBwb2ludC4ALwEAWUT/QAAAAAv/////AQH/////AQAAABVgiQoCAAAA" +
           "AAAQAAAARW5naW5lZXJpbmdVbml0cwEBBEEALgBEBEEAAAEAdwP/////AQH/////AAAAAA==";

        private const string InitializationString =
           "AgAAACUAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvUm9ib3RpY3MvHwAAAGh0dHA6Ly9vcGNm" +
           "b3VuZGF0aW9uLm9yZy9VQS9ESS//////BGCAAgEAAAABABAAAABBeGlzVHlwZUluc3RhbmNlAQHZQAEB" +
           "2UDZQAAA/////wMAAAAkYIAKAQAAAAIADAAAAFBhcmFtZXRlclNldAEB2kADAAAAABcAAABGbGF0IGxp" +
           "c3Qgb2YgUGFyYW1ldGVycwAvADraQAAA/////wMAAAA1YIkKAgAAAAEADgAAAEFjdHVhbFBvc2l0aW9u" +
           "AQEWQQMAAAAAMwAAAFRoZSBheGlzIHBvc2l0aW9uIGluY2x1c2l2ZSBVbml0IGFuZCBSYW5nZU9mTW90" +
           "aW9uLgAvAQBZRBZBAAAAC/////8BAf////8BAAAAFWCJCgIAAAAAABAAAABFbmdpbmVlcmluZ1VuaXRz" +
           "AQEbQQAuAEQbQQAAAQB3A/////8BAf////8AAAAANWCJCgIAAAABAAsAAABBY3R1YWxTcGVlZAEBHEED" +
           "AAAAAEAAAABUaGUgYXhpcyBzcGVlZCBvbiBsb2FkIHNpZGUgKGFmdGVyIGdlYXIvc3BpbmRsZSkgaW5j" +
           "bHVzaXZlIFVuaXQuAC8BAFlEHEEAAAAL/////wEB/////wEAAAAVYIkKAgAAAAAAEAAAAEVuZ2luZWVy" +
           "aW5nVW5pdHMBASFBAC4ARCFBAAABAHcD/////wEB/////wAAAAA1YIkKAgAAAAEAEgAAAEFjdHVhbEFj" +
           "Y2VsZXJhdGlvbgEBIkEDAAAAAK0AAAA6IFRoZSBBY3R1YWxBY2NlbGVyYXRpb24gdmFyaWFibGUgcHJv" +
           "dmlkZXMgdGhlIGF4aXMgYWNjZWxlcmF0aW9uLiBBcHBsaWNhYmxlIGFjY2VsZXJhdGlvbiBsaW1pdHMg" +
           "b2YgdGhlIGF4aXMgc2hhbGwgYmUgcHJvdmlkZWQgYnkgdGhlIEVVUmFuZ2UgcHJvcGVydHkgb2YgdGhl" +
           "IEFuYWxvZ1VuaXRUeXBlLgAvAQBZRCJBAAAAC/////8BAf////8BAAAAFWCJCgIAAAAAABAAAABFbmdp" +
           "bmVlcmluZ1VuaXRzAQEnQQAuAEQnQQAAAQB3A/////8BAf////8AAAAANWCJCgIAAAABAA0AAABNb3Rp" +
           "b25Qcm9maWxlAQH9QAMAAAAASQAAAFRoZSBraW5kIG9mIGF4aXMgbW90aW9uIGFzIGRlZmluZWQgd2l0" +
           "aCB0aGUgQXhpc01vdGlvblByb2ZpbGVFbnVtZXJhdGlvbi4ALgBE/UAAAAEBwAv/////AQH/////AAAA" +
           "ACRggAoBAAAAAQAOAAAAQWRkaXRpb25hbExvYWQBAf5AAwAAAABjAAAAVGhlIGFkZGl0aW9uYWwgbG9h" +
           "ZCB3aGljaCBpcyBtb3VudGVkIG9uIHRoaXMgYXhpcy4gRS5nLiBmb3IgcHJvY2Vzcy1uZWVkIGEgdHJh" +
           "bnNmb3JtZXIgZm9yIHdlbGRpbmcuAC8BAfoD/kAAAP////8BAAAANWCJCgIAAAABAAQAAABNYXNzAQH/" +
           "QAMAAAAANQAAAFRoZSB3ZWlnaHQgb2YgdGhlIGxvYWQgbW91bnRlZCBvbiBvbmUgbW91bnRpbmcgcG9p" +
           "bnQuAC8BAFlE/0AAAAAL/////wEB/////wEAAAAVYIkKAgAAAAAAEAAAAEVuZ2luZWVyaW5nVW5pdHMB" +
           "AQRBAC4ARARBAAABAHcD/////wEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public PropertyState<AxisMotionProfileEnumeration> MotionProfile
        {
            get
            {
                return m_motionProfile;
            }

            set
            {
                if (!Object.ReferenceEquals(m_motionProfile, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_motionProfile = value;
            }
        }

        /// <remarks />
        public LoadState AdditionalLoad
        {
            get
            {
                return m_additionalLoad;
            }

            set
            {
                if (!Object.ReferenceEquals(m_additionalLoad, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_additionalLoad = value;
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
            if (m_motionProfile != null)
            {
                children.Add(m_motionProfile);
            }

            if (m_additionalLoad != null)
            {
                children.Add(m_additionalLoad);
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
                case Opc.Ua.Robotics.BrowseNames.MotionProfile:
                {
                    if (createOrReplace)
                    {
                        if (MotionProfile == null)
                        {
                            if (replacement == null)
                            {
                                MotionProfile = new PropertyState<AxisMotionProfileEnumeration>(this);
                            }
                            else
                            {
                                MotionProfile = (PropertyState<AxisMotionProfileEnumeration>)replacement;
                            }
                        }
                    }

                    instance = MotionProfile;
                    break;
                }

                case Opc.Ua.Robotics.BrowseNames.AdditionalLoad:
                {
                    if (createOrReplace)
                    {
                        if (AdditionalLoad == null)
                        {
                            if (replacement == null)
                            {
                                AdditionalLoad = new LoadState(this);
                            }
                            else
                            {
                                AdditionalLoad = (LoadState)replacement;
                            }
                        }
                    }

                    instance = AdditionalLoad;
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
        private PropertyState<AxisMotionProfileEnumeration> m_motionProfile;
        private LoadState m_additionalLoad;
        #endregion
    }
    #endif
    #endregion

    #region PowerTrainState Class
    #if (!OPCUA_EXCLUDE_PowerTrainState)
    /// <summary>
    /// Stores an instance of the PowerTrainType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class PowerTrainState : ComponentState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public PowerTrainState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Robotics.ObjectTypes.PowerTrainType, Opc.Ua.Robotics.Namespaces.OpcUaRobotics, namespaceUris);
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
           "AgAAACUAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvUm9ib3RpY3MvHwAAAGh0dHA6Ly9vcGNm" +
           "b3VuZGF0aW9uLm9yZy9VQS9ESS//////BGCAAgEAAAABABYAAABQb3dlclRyYWluVHlwZUluc3RhbmNl" +
           "AQGaQQEBmkGaQQAA/////wAAAAA=";
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

    #region MotorState Class
    #if (!OPCUA_EXCLUDE_MotorState)
    /// <summary>
    /// Stores an instance of the MotorType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class MotorState : ComponentState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public MotorState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Robotics.ObjectTypes.MotorType, Opc.Ua.Robotics.Namespaces.OpcUaRobotics, namespaceUris);
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
           "AgAAACUAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvUm9ib3RpY3MvHwAAAGh0dHA6Ly9vcGNm" +
           "b3VuZGF0aW9uLm9yZy9VQS9ESS//////BGCAAgEAAAABABEAAABNb3RvclR5cGVJbnN0YW5jZQEB+wMB" +
           "AfsD+wMAAP////8FAAAAJGCACgEAAAACAAwAAABQYXJhbWV0ZXJTZXQBAfETAwAAAAAXAAAARmxhdCBs" +
           "aXN0IG9mIFBhcmFtZXRlcnMALwA68RMAAP////8DAAAANWCJCgIAAAABAA0AAABCcmFrZVJlbGVhc2Vk" +
           "AQH+QgMAAAAArAAAAEluZGljYXRlcyBhbiBvcHRpb25hbCB2YXJpYWJsZSB1c2VkIG9ubHkgZm9yIG1v" +
           "dG9ycyB3aXRoIGJyYWtlcy4gSWYgQnJha2VSZWxlYXNlZCBpcyBUUlVFIHRoZSBtb3RvciBpcyBmcmVl" +
           "IHRvIHJ1bi4gRkFMU0UgbWVhbnMgdGhhdCB0aGUgbW90b3Igc2hhZnQgaXMgbG9ja2VkIGJ5IHRoZSBi" +
           "cmFrZS4ALwA//kIAAAAB/////wEB/////wAAAAA1YIkKAgAAAAEAEAAAAE1vdG9yVGVtcGVyYXR1cmUB" +
           "AWUaAwAAAAB8AAAAVGhlIG1vdG9yIHRlbXBlcmF0dXJlIHByb3ZpZGVzIHRoZSB0ZW1wZXJhdHVyZSBv" +
           "ZiB0aGUgbW90b3IuIElmIHRoZXJlIGlzIG5vIHRlbXBlcmF0dXJlIHNlbnNvciB0aGUgdmFsdWUgaXMg" +
           "c2V0IHRvIFwibnVsbFwiLgAvAQBZRGUaAAAAC/////8BAf////8BAAAAFWCJCgIAAAAAABAAAABFbmdp" +
           "bmVlcmluZ1VuaXRzAQFqGgAuAERqGgAAAQB3A/////8BAf////8AAAAANWCJCgIAAAABABEAAABFZmZl" +
           "Y3RpdmVMb2FkUmF0ZQEBeBoDAAAAAMcAAABFZmZlY3RpdmVMb2FkUmF0ZSBpcyBleHByZXNzZWQgYXMg" +
           "YSBwZXJjZW50YWdlIG9mIG1heGltdW0gY29udGludW91cyBsb2FkLiBUaGUgSm91bGUgaW50ZWdyYWwg" +
           "aXMgdHlwaWNhbGx5IHVzZWQgdG8gY2FsY3VsYXRlIHRoZSBjdXJyZW50IGxvYWQuIER1cmF0aW9uIHNo" +
           "b3VsZCBiZSBkZWZpbmVkIGFuZCBkb2N1bWVudGVkIGJ5IHRoZSB2ZW5kb3IuAC8AP3gaAAAABf////8B" +
           "Af////8AAAAAFWCJCgIAAAACAAwAAABNYW51ZmFjdHVyZXIBAc1CAC4ARM1CAAAAFf////8BAf////8A" +
           "AAAAFWCJCgIAAAACAAUAAABNb2RlbAEBz0IALgBEz0IAAAAV/////wEB/////wAAAAAVYIkKAgAAAAIA" +
           "CwAAAFByb2R1Y3RDb2RlAQHVQgAuAETVQgAAAAz/////AQH/////AAAAABVgiQoCAAAAAgAMAAAAU2Vy" +
           "aWFsTnVtYmVyAQHQQgAuAETQQgAAAAz/////AQH/////AAAAAA==";
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

    #region GearState Class
    #if (!OPCUA_EXCLUDE_GearState)
    /// <summary>
    /// Stores an instance of the GearType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class GearState : ComponentState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public GearState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Robotics.ObjectTypes.GearType, Opc.Ua.Robotics.Namespaces.OpcUaRobotics, namespaceUris);
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

            if (Pitch != null)
            {
                Pitch.Initialize(context, Pitch_InitializationString);
            }
        }

        #region Initialization String
        private const string Pitch_InitializationString =
           "AgAAACUAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvUm9ib3RpY3MvHwAAAGh0dHA6Ly9vcGNm" +
           "b3VuZGF0aW9uLm9yZy9VQS9ESS//////NWCJCgIAAAABAAUAAABQaXRjaAEBDUMDAAAAAPgAAABQaXRj" +
           "aCBkZXNjcmliZXMgdGhlIGRpc3RhbmNlIGNvdmVyZWQgaW4gbWlsbGltZXRlcnMgKG1tKSBmb3IgbGlu" +
           "ZWFyIG1vdGlvbiBwZXIgb25lIHJldm9sdXRpb24gb2YgdGhlIG91dHB1dCBzaWRlIG9mIHRoZSBkcml2" +
           "aW5nIHVuaXQuIFBpdGNoIGlzIHVzZWQgaW4gY29tYmluYXRpb24gd2l0aCBHZWFyUmF0aW8gdG8gZGVz" +
           "Y3JpYmUgdGhlIG92ZXJhbGwgdHJhbnNtaXNzaW9uIGZyb20gaW5wdXQgdG8gb3V0cHV0IG9mIHRoZSBn" +
           "ZWFyLgAvAD8NQwAAAAv/////AQH/////AAAAAA==";

        private const string InitializationString =
           "AgAAACUAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvUm9ib3RpY3MvHwAAAGh0dHA6Ly9vcGNm" +
           "b3VuZGF0aW9uLm9yZy9VQS9ESS//////BGCAAgEAAAABABAAAABHZWFyVHlwZUluc3RhbmNlAQH+AwEB" +
           "/gP+AwAA/////wYAAAAVYIkKAgAAAAIADAAAAE1hbnVmYWN0dXJlcgEBAEMALgBEAEMAAAAV/////wEB" +
           "/////wAAAAAVYIkKAgAAAAIABQAAAE1vZGVsAQECQwAuAEQCQwAAABX/////AQH/////AAAAABVgiQoC" +
           "AAAAAgALAAAAUHJvZHVjdENvZGUBAQhDAC4ARAhDAAAADP////8BAf////8AAAAAFWCJCgIAAAACAAwA" +
           "AABTZXJpYWxOdW1iZXIBAQNDAC4ARANDAAAADP////8BAf////8AAAAANWCJCgIAAAABAAkAAABHZWFy" +
           "UmF0aW8BAUU+AwAAAAB5AAAAVGhlIHRyYW5zbWlzc2lvbiByYXRpbyBvZiB0aGUgZ2VhciBleHByZXNz" +
           "ZWQgYXMgYSBmcmFjdGlvbiBhcyBpbnB1dCB2ZWxvY2l0eSAobW90b3Igc2lkZSkgYnkgb3V0cHV0IHZl" +
           "bG9jaXR5IChsb2FkIHNpZGUpLgAvAQAtRUU+AAABAHZJ/////wEB/////wIAAAAVYIkKAgAAAAAACQAA" +
           "AE51bWVyYXRvcgEB/zwALwA//zwAAAAG/////wEB/////wAAAAAVYIkKAgAAAAAACwAAAERlbm9taW5h" +
           "dG9yAQEAPQAvAD8APQAAAAf/////AQH/////AAAAADVgiQoCAAAAAQAFAAAAUGl0Y2gBAQ1DAwAAAAD4" +
           "AAAAUGl0Y2ggZGVzY3JpYmVzIHRoZSBkaXN0YW5jZSBjb3ZlcmVkIGluIG1pbGxpbWV0ZXJzIChtbSkg" +
           "Zm9yIGxpbmVhciBtb3Rpb24gcGVyIG9uZSByZXZvbHV0aW9uIG9mIHRoZSBvdXRwdXQgc2lkZSBvZiB0" +
           "aGUgZHJpdmluZyB1bml0LiBQaXRjaCBpcyB1c2VkIGluIGNvbWJpbmF0aW9uIHdpdGggR2VhclJhdGlv" +
           "IHRvIGRlc2NyaWJlIHRoZSBvdmVyYWxsIHRyYW5zbWlzc2lvbiBmcm9tIGlucHV0IHRvIG91dHB1dCBv" +
           "ZiB0aGUgZ2Vhci4ALwA/DUMAAAAL/////wEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public RationalNumberState GearRatio
        {
            get
            {
                return m_gearRatio;
            }

            set
            {
                if (!Object.ReferenceEquals(m_gearRatio, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_gearRatio = value;
            }
        }

        /// <remarks />
        public BaseDataVariableState<double> Pitch
        {
            get
            {
                return m_pitch;
            }

            set
            {
                if (!Object.ReferenceEquals(m_pitch, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_pitch = value;
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
            if (m_gearRatio != null)
            {
                children.Add(m_gearRatio);
            }

            if (m_pitch != null)
            {
                children.Add(m_pitch);
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
                case Opc.Ua.Robotics.BrowseNames.GearRatio:
                {
                    if (createOrReplace)
                    {
                        if (GearRatio == null)
                        {
                            if (replacement == null)
                            {
                                GearRatio = new RationalNumberState(this);
                            }
                            else
                            {
                                GearRatio = (RationalNumberState)replacement;
                            }
                        }
                    }

                    instance = GearRatio;
                    break;
                }

                case Opc.Ua.Robotics.BrowseNames.Pitch:
                {
                    if (createOrReplace)
                    {
                        if (Pitch == null)
                        {
                            if (replacement == null)
                            {
                                Pitch = new BaseDataVariableState<double>(this);
                            }
                            else
                            {
                                Pitch = (BaseDataVariableState<double>)replacement;
                            }
                        }
                    }

                    instance = Pitch;
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
        private RationalNumberState m_gearRatio;
        private BaseDataVariableState<double> m_pitch;
        #endregion
    }
    #endif
    #endregion

    #region SafetyStateState Class
    #if (!OPCUA_EXCLUDE_SafetyStateState)
    /// <summary>
    /// Stores an instance of the SafetyStateType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class SafetyStateState : ComponentState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public SafetyStateState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Robotics.ObjectTypes.SafetyStateType, Opc.Ua.Robotics.Namespaces.OpcUaRobotics, namespaceUris);
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

            if (EmergencyStopFunctions != null)
            {
                EmergencyStopFunctions.Initialize(context, EmergencyStopFunctions_InitializationString);
            }

            if (ProtectiveStopFunctions != null)
            {
                ProtectiveStopFunctions.Initialize(context, ProtectiveStopFunctions_InitializationString);
            }
        }

        #region Initialization String
        private const string EmergencyStopFunctions_InitializationString =
           "AgAAACUAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvUm9ib3RpY3MvHwAAAGh0dHA6Ly9vcGNm" +
           "b3VuZGF0aW9uLm9yZy9VQS9ESS//////JGCACgEAAAABABYAAABFbWVyZ2VuY3lTdG9wRnVuY3Rpb25z" +
           "AQFFQwMAAAAAYQAAAEVtZXJnZW5jeVN0b3BGdW5jdGlvbnMgaXMgYSBjb250YWluZXIgZm9yIG9uZSBv" +
           "ciBtb3JlIGluc3RhbmNlcyBvZiB0aGUgRW1lcmdlbmN5U3RvcEZ1bmN0aW9uVHlwZS4ALwA9RUMAAP//" +
           "//8AAAAA";

        private const string ProtectiveStopFunctions_InitializationString =
           "AgAAACUAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvUm9ib3RpY3MvHwAAAGh0dHA6Ly9vcGNm" +
           "b3VuZGF0aW9uLm9yZy9VQS9ESS//////JGCACgEAAAABABcAAABQcm90ZWN0aXZlU3RvcEZ1bmN0aW9u" +
           "cwEBSUMDAAAAAGMAAABQcm90ZWN0aXZlU3RvcEZ1bmN0aW9ucyBpcyBhIGNvbnRhaW5lciBmb3Igb25l" +
           "IG9yIG1vcmUgaW5zdGFuY2VzIG9mIHRoZSBQcm90ZWN0aXZlU3RvcEZ1bmN0aW9uVHlwZS4ALwA9SUMA" +
           "AP////8AAAAA";

        private const string InitializationString =
           "AgAAACUAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvUm9ib3RpY3MvHwAAAGh0dHA6Ly9vcGNm" +
           "b3VuZGF0aW9uLm9yZy9VQS9ESS//////BGCAAgEAAAABABcAAABTYWZldHlTdGF0ZVR5cGVJbnN0YW5j" +
           "ZQEB9QMBAfUD9QMAAP////8DAAAAJGCACgEAAAACAAwAAABQYXJhbWV0ZXJTZXQBAZgTAwAAAAAXAAAA" +
           "RmxhdCBsaXN0IG9mIFBhcmFtZXRlcnMALwA6mBMAAP////8DAAAANWCJCgIAAAABAA8AAABPcGVyYXRp" +
           "b25hbE1vZGUBASg+AwAAAADAAAAAVGhlIE9wZXJhdGlvbmFsTW9kZSB2YXJpYWJsZSBwcm92aWRlcyBp" +
           "bmZvcm1hdGlvbiBhYm91dCB0aGUgY3VycmVudCBvcGVyYXRpb25hbCBtb2RlLiBBbGxvd2VkIHZhbHVl" +
           "cyBhcmUgZGVzY3JpYmVkIGluIE9wZXJhdGlvbmFsTW9kZUVudW1lcmF0aW9uLCBzZWUgSVNPIDEwMjE4" +
           "LTE6MjAxMSBDaC41LjcgT3BlcmF0aW9uYWwgTW9kZXMuAC8APyg+AAABAb4L/////wEB/////wAAAAA1" +
           "YIkKAgAAAAEADQAAAEVtZXJnZW5jeVN0b3ABAQo+AwAAAAAeAQAAVGhlIEVtZXJnZW5jeVN0b3AgdmFy" +
           "aWFibGUgaXMgVFJVRSBpZiBvbmUgb3IgbW9yZSBvZiB0aGUgZW1lcmdlbmN5IHN0b3AgZnVuY3Rpb25z" +
           "IGluIHRoZSByb2JvdCBzeXN0ZW0gYXJlIGFjdGl2ZSwgRkFMU0Ugb3RoZXJ3aXNlLiBJZiB0aGUgRW1l" +
           "cmdlbmN5U3RvcEZ1bmN0aW9ucyBvYmplY3QgaXMgcHJvdmlkZWQsIHRoZW4gdGhlIHZhbHVlIG9mIHRo" +
           "aXMgdmFyaWFibGUgaXMgVFJVRSBpZiBvbmUgb3IgbW9yZSBvZiB0aGUgbGlzdGVkIGVtZXJnZW5jeSBz" +
           "dG9wIGZ1bmN0aW9ucyBhcmUgYWN0aXZlLgAvAD8KPgAAAAH/////AQH/////AAAAADVgiQoCAAAAAQAO" +
           "AAAAUHJvdGVjdGl2ZVN0b3ABASk+AwAAAAAwAQAAVGhlIFByb3RlY3RpdmVTdG9wIHZhcmlhYmxlIGlz" +
           "IFRSVUUgaWYgb25lIG9yIG1vcmUgb2YgdGhlIGVuYWJsZWQgcHJvdGVjdGl2ZSBzdG9wIGZ1bmN0aW9u" +
           "cyBpbiB0aGUgc3lzdGVtIGFyZSBhY3RpdmUsIEZBTFNFIG90aGVyd2lzZS4gSWYgdGhlIFByb3RlY3Rp" +
           "dmVTdG9wRnVuY3Rpb25zIG9iamVjdCBpcyBwcm92aWRlZCwgdGhlbiB0aGUgdmFsdWUgb2YgdGhpcyB2" +
           "YXJpYWJsZSBpcyBUUlVFIGlmIG9uZSBvciBtb3JlIG9mIHRoZSBsaXN0ZWQgcHJvdGVjdGl2ZSBzdG9w" +
           "IGZ1bmN0aW9ucyBhcmUgZW5hYmxlZCBhbmQgYWN0aXZlLgAvAD8pPgAAAAH/////AQH/////AAAAACRg" +
           "gAoBAAAAAQAWAAAARW1lcmdlbmN5U3RvcEZ1bmN0aW9ucwEBRUMDAAAAAGEAAABFbWVyZ2VuY3lTdG9w" +
           "RnVuY3Rpb25zIGlzIGEgY29udGFpbmVyIGZvciBvbmUgb3IgbW9yZSBpbnN0YW5jZXMgb2YgdGhlIEVt" +
           "ZXJnZW5jeVN0b3BGdW5jdGlvblR5cGUuAC8APUVDAAD/////AAAAACRggAoBAAAAAQAXAAAAUHJvdGVj" +
           "dGl2ZVN0b3BGdW5jdGlvbnMBAUlDAwAAAABjAAAAUHJvdGVjdGl2ZVN0b3BGdW5jdGlvbnMgaXMgYSBj" +
           "b250YWluZXIgZm9yIG9uZSBvciBtb3JlIGluc3RhbmNlcyBvZiB0aGUgUHJvdGVjdGl2ZVN0b3BGdW5j" +
           "dGlvblR5cGUuAC8APUlDAAD/////AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public FolderState EmergencyStopFunctions
        {
            get
            {
                return m_emergencyStopFunctions;
            }

            set
            {
                if (!Object.ReferenceEquals(m_emergencyStopFunctions, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_emergencyStopFunctions = value;
            }
        }

        /// <remarks />
        public FolderState ProtectiveStopFunctions
        {
            get
            {
                return m_protectiveStopFunctions;
            }

            set
            {
                if (!Object.ReferenceEquals(m_protectiveStopFunctions, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_protectiveStopFunctions = value;
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
            if (m_emergencyStopFunctions != null)
            {
                children.Add(m_emergencyStopFunctions);
            }

            if (m_protectiveStopFunctions != null)
            {
                children.Add(m_protectiveStopFunctions);
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
                case Opc.Ua.Robotics.BrowseNames.EmergencyStopFunctions:
                {
                    if (createOrReplace)
                    {
                        if (EmergencyStopFunctions == null)
                        {
                            if (replacement == null)
                            {
                                EmergencyStopFunctions = new FolderState(this);
                            }
                            else
                            {
                                EmergencyStopFunctions = (FolderState)replacement;
                            }
                        }
                    }

                    instance = EmergencyStopFunctions;
                    break;
                }

                case Opc.Ua.Robotics.BrowseNames.ProtectiveStopFunctions:
                {
                    if (createOrReplace)
                    {
                        if (ProtectiveStopFunctions == null)
                        {
                            if (replacement == null)
                            {
                                ProtectiveStopFunctions = new FolderState(this);
                            }
                            else
                            {
                                ProtectiveStopFunctions = (FolderState)replacement;
                            }
                        }
                    }

                    instance = ProtectiveStopFunctions;
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
        private FolderState m_emergencyStopFunctions;
        private FolderState m_protectiveStopFunctions;
        #endregion
    }
    #endif
    #endregion

    #region EmergencyStopFunctionState Class
    #if (!OPCUA_EXCLUDE_EmergencyStopFunctionState)
    /// <summary>
    /// Stores an instance of the EmergencyStopFunctionType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class EmergencyStopFunctionState : BaseObjectState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public EmergencyStopFunctionState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Robotics.ObjectTypes.EmergencyStopFunctionType, Opc.Ua.Robotics.Namespaces.OpcUaRobotics, namespaceUris);
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
           "AgAAACUAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvUm9ib3RpY3MvHwAAAGh0dHA6Ly9vcGNm" +
           "b3VuZGF0aW9uLm9yZy9VQS9ESS//////BGCAAgEAAAABACEAAABFbWVyZ2VuY3lTdG9wRnVuY3Rpb25U" +
           "eXBlSW5zdGFuY2UBAU5DAQFOQ05DAAD/////AgAAADVgiQoCAAAAAQAEAAAATmFtZQEBT0MDAAAAAIcA" +
           "AABUaGUgTmFtZSBvZiB0aGUgRW1lcmdlbmN5U3RvcEZ1bmN0aW9uVHlwZSBwcm92aWRlcyBhIG1hbnVm" +
           "YWN0dXJlci1zcGVjaWZpYyBlbWVyZ2VuY3kgc3RvcCBmdW5jdGlvbiBpZGVudGlmaWVyIHdpdGhpbiB0" +
           "aGUgc2FmZXR5IHN5c3RlbS4ALgBET0MAAAAM/////wEB/////wAAAAA1YIkKAgAAAAEABgAAAEFjdGl2" +
           "ZQEBUEMDAAAAAJIAAABUaGUgQWN0aXZlIHZhcmlhYmxlIGlzIFRSVUUgaWYgdGhpcyBwYXJ0aWN1bGFy" +
           "IGVtZXJnZW5jeSBzdG9wIGZ1bmN0aW9uIGlzIGFjdGl2ZSwgZS5nLiB0aGF0IHRoZSBlbWVyZ2VuY3kg" +
           "c3RvcCBidXR0b24gaXMgcHJlc3NlZCwgRkFMU0Ugb3RoZXJ3aXNlLgAvAD9QQwAAAAH/////AQH/////" +
           "AAAAAA==";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public PropertyState<string> Name
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
        public BaseDataVariableState<bool> Active
        {
            get
            {
                return m_active;
            }

            set
            {
                if (!Object.ReferenceEquals(m_active, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_active = value;
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

            if (m_active != null)
            {
                children.Add(m_active);
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
                case Opc.Ua.Robotics.BrowseNames.Name:
                {
                    if (createOrReplace)
                    {
                        if (Name == null)
                        {
                            if (replacement == null)
                            {
                                Name = new PropertyState<string>(this);
                            }
                            else
                            {
                                Name = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = Name;
                    break;
                }

                case Opc.Ua.Robotics.BrowseNames.Active:
                {
                    if (createOrReplace)
                    {
                        if (Active == null)
                        {
                            if (replacement == null)
                            {
                                Active = new BaseDataVariableState<bool>(this);
                            }
                            else
                            {
                                Active = (BaseDataVariableState<bool>)replacement;
                            }
                        }
                    }

                    instance = Active;
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
        private PropertyState<string> m_name;
        private BaseDataVariableState<bool> m_active;
        #endregion
    }
    #endif
    #endregion

    #region ProtectiveStopFunctionState Class
    #if (!OPCUA_EXCLUDE_ProtectiveStopFunctionState)
    /// <summary>
    /// Stores an instance of the ProtectiveStopFunctionType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ProtectiveStopFunctionState : BaseObjectState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ProtectiveStopFunctionState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Robotics.ObjectTypes.ProtectiveStopFunctionType, Opc.Ua.Robotics.Namespaces.OpcUaRobotics, namespaceUris);
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
           "AgAAACUAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvUm9ib3RpY3MvHwAAAGh0dHA6Ly9vcGNm" +
           "b3VuZGF0aW9uLm9yZy9VQS9ESS//////BGCAAgEAAAABACIAAABQcm90ZWN0aXZlU3RvcEZ1bmN0aW9u" +
           "VHlwZUluc3RhbmNlAQFRQwEBUUNRQwAA/////wMAAAA1YIkKAgAAAAEABAAAAE5hbWUBAVJDAwAAAACJ" +
           "AAAAVGhlIE5hbWUgb2YgdGhlIFByb3RlY3RpdmVTdG9wRnVuY3Rpb25UeXBlIHByb3ZpZGVzIGEgbWFu" +
           "dWZhY3R1cmVyLXNwZWNpZmljIHByb3RlY3RpdmUgc3RvcCBmdW5jdGlvbiBpZGVudGlmaWVyIHdpdGhp" +
           "biB0aGUgc2FmZXR5IHN5c3RlbS4ALgBEUkMAAAAM/////wEB/////wAAAAA1YIkKAgAAAAEABwAAAEVu" +
           "YWJsZWQBAVNDAwAAAADeAQAA4oCTCVRoZSBFbmFibGVkIHZhcmlhYmxlIGlzIFRSVUUgaWYgdGhpcyBw" +
           "cm90ZWN0aXZlIHN0b3AgZnVuY3Rpb24gaXMgY3VycmVudGx5IHN1cGVydmlzaW5nIHRoZSBzeXN0ZW0s" +
           "IEZBTFNFIG90aGVyd2lzZS4gQSBwcm90ZWN0aXZlIHN0b3AgZnVuY3Rpb24gbWF5IG9yIG1heSBub3Qg" +
           "YmUgZW5hYmxlZCBhdCBhbGwgdGltZXMsIGUuZy4gdGhlIHByb3RlY3RpdmUgc3RvcCBmdW5jdGlvbiBv" +
           "ZiB0aGUgc2FmZXR5IGRvb3JzIGFyZSB0eXBpY2FsbHkgZW5hYmxlZCBpbiBhdXRvbWF0aWMgb3BlcmF0" +
           "aW9uYWwgbW9kZSBhbmQgZGlzYWJsZWQgaW4gbWFudWFsIG1vZGUuIE9uIHRoZSBvdGhlciBoYW5kIGZv" +
           "ciBleGFtcGxlLCB0aGUgcHJvdGVjdGl2ZSBzdG9wIGZ1bmN0aW9uIG9mIHRoZSB0ZWFjaCBwZW5kYW50" +
           "IGVuYWJsaW5nIGRldmljZSBpcyBlbmFibGVkIGluIG1hbnVhbCBtb2RlcyBhbmQgZGlzYWJsZWQgaW4g" +
           "YXV0b21hdGljIG1vZGVzLgAvAD9TQwAAAAH/////AQH/////AAAAADVgiQoCAAAAAQAGAAAAQWN0aXZl" +
           "AQFUQwMAAAAAtgAAAOKAkwlUaGUgQWN0aXZlIHZhcmlhYmxlIGlzIFRSVUUgaWYgdGhpcyBwYXJ0aWN1" +
           "bGFyIHByb3RlY3RpdmUgc3RvcCBmdW5jdGlvbiBpcyBhY3RpdmUsIGkuZS4gdGhhdCBhIHN0b3AgaXMg" +
           "aW5pdGlhdGVkLCBGQUxTRSBvdGhlcndpc2UuIElmIEVuYWJsZWQgaXMgRkFMU0UgdGhlbiBBY3RpdmUg" +
           "c2hhbGwgYmUgRkFMU0UuAC8AP1RDAAAAAf////8BAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public PropertyState<string> Name
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
        public BaseDataVariableState<bool> Active
        {
            get
            {
                return m_active;
            }

            set
            {
                if (!Object.ReferenceEquals(m_active, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_active = value;
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

            if (m_enabled != null)
            {
                children.Add(m_enabled);
            }

            if (m_active != null)
            {
                children.Add(m_active);
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
                case Opc.Ua.Robotics.BrowseNames.Name:
                {
                    if (createOrReplace)
                    {
                        if (Name == null)
                        {
                            if (replacement == null)
                            {
                                Name = new PropertyState<string>(this);
                            }
                            else
                            {
                                Name = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = Name;
                    break;
                }

                case Opc.Ua.Robotics.BrowseNames.Enabled:
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

                case Opc.Ua.Robotics.BrowseNames.Active:
                {
                    if (createOrReplace)
                    {
                        if (Active == null)
                        {
                            if (replacement == null)
                            {
                                Active = new BaseDataVariableState<bool>(this);
                            }
                            else
                            {
                                Active = (BaseDataVariableState<bool>)replacement;
                            }
                        }
                    }

                    instance = Active;
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
        private PropertyState<string> m_name;
        private BaseDataVariableState<bool> m_enabled;
        private BaseDataVariableState<bool> m_active;
        #endregion
    }
    #endif
    #endregion

    #region ControllerState Class
    #if (!OPCUA_EXCLUDE_ControllerState)
    /// <summary>
    /// Stores an instance of the ControllerType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class ControllerState : ComponentState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public ControllerState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Robotics.ObjectTypes.ControllerType, Opc.Ua.Robotics.Namespaces.OpcUaRobotics, namespaceUris);
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

            if (Components != null)
            {
                Components.Initialize(context, Components_InitializationString);
            }

            if (ParameterSet != null)
            {
                ParameterSet.Initialize(context, ParameterSet_InitializationString);
            }
        }

        #region Initialization String
        private const string Components_InitializationString =
           "AgAAACUAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvUm9ib3RpY3MvHwAAAGh0dHA6Ly9vcGNm" +
           "b3VuZGF0aW9uLm9yZy9VQS9ESS//////JGCACgEAAAABAAoAAABDb21wb25lbnRzAQFkQwMAAAAAGwEA" +
           "AENvbXBvbmVudHMgaXMgYSBjb250YWluZXIgZm9yIG9uZSBvciBtb3JlIGluc3RhbmNlcyBvZiBzdWJ0" +
           "eXBlcyBvZiBDb21wb25lbnRUeXBlIGRlZmluZWQgaW4gT1BDIFVBIERJLiBUaGUgbGlzdGVkIGNvbXBv" +
           "bmVudHMgYXJlIGluc3RhbGxlZCBpbiB0aGUgbW90aW9uIGRldmljZSBzeXN0ZW0sIGUuZy4gYSBwcm9j" +
           "ZXNzaW5nLXVuaXQsIGEgcG93ZXItc3VwcGx5LCBhbiBJTy1ib2FyZCBvciBhIGRyaXZlLCBhbmQgaGF2" +
           "ZSBhbiBlbGVjdHJpY2FsIGludGVyZmFjZSB0byB0aGUgY29udHJvbGxlci4ALwA9ZEMAAP////8AAAAA";

        private const string ParameterSet_InitializationString =
           "AgAAACUAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvUm9ib3RpY3MvHwAAAGh0dHA6Ly9vcGNm" +
           "b3VuZGF0aW9uLm9yZy9VQS9ESS//////JGCACgEAAAACAAwAAABQYXJhbWV0ZXJTZXQBAYwTAwAAAAAX" +
           "AAAARmxhdCBsaXN0IG9mIFBhcmFtZXRlcnMALwA6jBMAAP////8IAAAANWCJCgIAAAABABAAAABUb3Rh" +
           "bFBvd2VyT25UaW1lAQHOQwMAAAAAOQAAAFRoZSB0b3RhbCBhY2N1bXVsYXRlZCB0aW1lIHRoZSBjb250" +
           "cm9sbGVyIHdhcyBwb3dlcmVkIG9uLgAvAD/OQwAAAQBPMv////8BAf////8AAAAANWCJCgIAAAABAAsA" +
           "AABTdGFydFVwVGltZQEBBjwDAAAAADkAAABUaGUgZGF0ZSBhbmQgdGltZSBvZiB0aGUgbGFzdCBzdGFy" +
           "dC11cCBvZiB0aGUgY29udHJvbGxlci4ALwA/BjwAAAAN/////wEB/////wAAAAA1YIkKAgAAAAEACAAA" +
           "AFVwc1N0YXRlAQEFPAMAAAAARgAAAFRoZSB2ZW5kb3Igc3BlY2lmaWMgc3RhdHVzIG9mIGFuIGludGVn" +
           "cmF0ZWQgVVBTIG9yIGFjY3VtdWxhdG9yIHN5c3RlbS4ALwA/BTwAAAAM/////wEB/////wAAAAA1YIkK" +
           "AgAAAAEAFgAAAFRvdGFsRW5lcmd5Q29uc3VtcHRpb24BAc9DAwAAAABiAAAAVGhlIHRvdGFsIGFjY3Vt" +
           "dWxhdGVkIGVuZXJneSBjb25zdW1lZCBieSB0aGUgbW90aW9uIGRldmljZXMgcmVsYXRlZCB3aXRoIHRo" +
           "aXMgY29udHJvbGxlciBpbnN0YW5jZS4ALwEAWUTPQwAAAAv/////AQH/////AQAAABVgiQoCAAAAAAAQ" +
           "AAAARW5naW5lZXJpbmdVbml0cwEB1EMALgBE1EMAAAEAdwP/////AQH/////AAAAADVgiQoCAAAAAQAP" +
           "AAAAQ2FiaW5ldEZhblNwZWVkAQHVQwMAAAAAHQAAAFRoZSBzcGVlZCBvZiB0aGUgY2FiaW5ldCBmYW4u" +
           "AC8BAFlE1UMAAAAL/////wEB/////wEAAAAVYIkKAgAAAAAAEAAAAEVuZ2luZWVyaW5nVW5pdHMBAdpD" +
           "AC4ARNpDAAABAHcD/////wEB/////wAAAAA1YIkKAgAAAAEACwAAAENQVUZhblNwZWVkAQHbQwMAAAAA" +
           "GQAAAFRoZSBzcGVlZCBvZiB0aGUgQ1BVIGZhbi4ALwEAWUTbQwAAAAv/////AQH/////AQAAABVgiQoC" +
           "AAAAAAAQAAAARW5naW5lZXJpbmdVbml0cwEB4EMALgBE4EMAAAEAdwP/////AQH/////AAAAADVgiQoC" +
           "AAAAAQAMAAAASW5wdXRWb2x0YWdlAQHhQwMAAAAAvAAAAFRoZSBpbnB1dCB2b2x0YWdlIG9mIHRoZSBj" +
           "b250cm9sbGVyIHdoaWNoIGNhbiBiZSBhIGNvbmZpZ3VyZWQgdmFsdWUuIFRvIGRpc3Rpbmd1aXNoIGJl" +
           "dHdlZW4gYW4gQUMgb3IgREMgc3VwcGx5IHRoZSBvcHRpb25hbCBwcm9wZXJ0eSBEZWZpbml0aW9uIG9m" +
           "IHRoZSBiYXNlIHR5cGUgRGF0YUl0ZW1UeXBlIHNoYWxsIGJlIHVzZWQuAC8BAFlE4UMAAAAL/////wEB" +
           "/////wEAAAAVYIkKAgAAAAAAEAAAAEVuZ2luZWVyaW5nVW5pdHMBAeZDAC4AROZDAAABAHcD/////wEB" +
           "/////wAAAAA1YIkKAgAAAAEACwAAAFRlbXBlcmF0dXJlAQHnQwMAAAAAUgAAAFRoZSBjb250cm9sbGVy" +
           "IHRlbXBlcmF0dXJlIGdpdmVuIGJ5IGEgdGVtcGVyYXR1cmUgc2Vuc29yIGluc2lkZSBvZiB0aGUgY29u" +
           "dHJvbGxlci4ALwEAWUTnQwAAAAv/////AQH/////AQAAABVgiQoCAAAAAAAQAAAARW5naW5lZXJpbmdV" +
           "bml0cwEB7EMALgBE7EMAAAEAdwP/////AQH/////AAAAAA==";

        private const string InitializationString =
           "AgAAACUAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvUm9ib3RpY3MvHwAAAGh0dHA6Ly9vcGNm" +
           "b3VuZGF0aW9uLm9yZy9VQS9ESS//////hGCAAgEAAAABABYAAABDb250cm9sbGVyVHlwZUluc3RhbmNl" +
           "AQHrAwEB6wPrAwAAAf////8JAAAAJGCACgEAAAACAAwAAABQYXJhbWV0ZXJTZXQBAYwTAwAAAAAXAAAA" +
           "RmxhdCBsaXN0IG9mIFBhcmFtZXRlcnMALwA6jBMAAP////8IAAAANWCJCgIAAAABABAAAABUb3RhbFBv" +
           "d2VyT25UaW1lAQHOQwMAAAAAOQAAAFRoZSB0b3RhbCBhY2N1bXVsYXRlZCB0aW1lIHRoZSBjb250cm9s" +
           "bGVyIHdhcyBwb3dlcmVkIG9uLgAvAD/OQwAAAQBPMv////8BAf////8AAAAANWCJCgIAAAABAAsAAABT" +
           "dGFydFVwVGltZQEBBjwDAAAAADkAAABUaGUgZGF0ZSBhbmQgdGltZSBvZiB0aGUgbGFzdCBzdGFydC11" +
           "cCBvZiB0aGUgY29udHJvbGxlci4ALwA/BjwAAAAN/////wEB/////wAAAAA1YIkKAgAAAAEACAAAAFVw" +
           "c1N0YXRlAQEFPAMAAAAARgAAAFRoZSB2ZW5kb3Igc3BlY2lmaWMgc3RhdHVzIG9mIGFuIGludGVncmF0" +
           "ZWQgVVBTIG9yIGFjY3VtdWxhdG9yIHN5c3RlbS4ALwA/BTwAAAAM/////wEB/////wAAAAA1YIkKAgAA" +
           "AAEAFgAAAFRvdGFsRW5lcmd5Q29uc3VtcHRpb24BAc9DAwAAAABiAAAAVGhlIHRvdGFsIGFjY3VtdWxh" +
           "dGVkIGVuZXJneSBjb25zdW1lZCBieSB0aGUgbW90aW9uIGRldmljZXMgcmVsYXRlZCB3aXRoIHRoaXMg" +
           "Y29udHJvbGxlciBpbnN0YW5jZS4ALwEAWUTPQwAAAAv/////AQH/////AQAAABVgiQoCAAAAAAAQAAAA" +
           "RW5naW5lZXJpbmdVbml0cwEB1EMALgBE1EMAAAEAdwP/////AQH/////AAAAADVgiQoCAAAAAQAPAAAA" +
           "Q2FiaW5ldEZhblNwZWVkAQHVQwMAAAAAHQAAAFRoZSBzcGVlZCBvZiB0aGUgY2FiaW5ldCBmYW4uAC8B" +
           "AFlE1UMAAAAL/////wEB/////wEAAAAVYIkKAgAAAAAAEAAAAEVuZ2luZWVyaW5nVW5pdHMBAdpDAC4A" +
           "RNpDAAABAHcD/////wEB/////wAAAAA1YIkKAgAAAAEACwAAAENQVUZhblNwZWVkAQHbQwMAAAAAGQAA" +
           "AFRoZSBzcGVlZCBvZiB0aGUgQ1BVIGZhbi4ALwEAWUTbQwAAAAv/////AQH/////AQAAABVgiQoCAAAA" +
           "AAAQAAAARW5naW5lZXJpbmdVbml0cwEB4EMALgBE4EMAAAEAdwP/////AQH/////AAAAADVgiQoCAAAA" +
           "AQAMAAAASW5wdXRWb2x0YWdlAQHhQwMAAAAAvAAAAFRoZSBpbnB1dCB2b2x0YWdlIG9mIHRoZSBjb250" +
           "cm9sbGVyIHdoaWNoIGNhbiBiZSBhIGNvbmZpZ3VyZWQgdmFsdWUuIFRvIGRpc3Rpbmd1aXNoIGJldHdl" +
           "ZW4gYW4gQUMgb3IgREMgc3VwcGx5IHRoZSBvcHRpb25hbCBwcm9wZXJ0eSBEZWZpbml0aW9uIG9mIHRo" +
           "ZSBiYXNlIHR5cGUgRGF0YUl0ZW1UeXBlIHNoYWxsIGJlIHVzZWQuAC8BAFlE4UMAAAAL/////wEB////" +
           "/wEAAAAVYIkKAgAAAAAAEAAAAEVuZ2luZWVyaW5nVW5pdHMBAeZDAC4AROZDAAABAHcD/////wEB////" +
           "/wAAAAA1YIkKAgAAAAEACwAAAFRlbXBlcmF0dXJlAQHnQwMAAAAAUgAAAFRoZSBjb250cm9sbGVyIHRl" +
           "bXBlcmF0dXJlIGdpdmVuIGJ5IGEgdGVtcGVyYXR1cmUgc2Vuc29yIGluc2lkZSBvZiB0aGUgY29udHJv" +
           "bGxlci4ALwEAWUTnQwAAAAv/////AQH/////AQAAABVgiQoCAAAAAAAQAAAARW5naW5lZXJpbmdVbml0" +
           "cwEB7EMALgBE7EMAAAEAdwP/////AQH/////AAAAABVgiQoCAAAAAgAMAAAATWFudWZhY3R1cmVyAQFV" +
           "QwAuAERVQwAAABX/////AQH/////AAAAABVgiQoCAAAAAgAFAAAATW9kZWwBAVdDAC4ARFdDAAAAFf//" +
           "//8BAf////8AAAAAFWCJCgIAAAACAAsAAABQcm9kdWN0Q29kZQEBXUMALgBEXUMAAAAM/////wEB////" +
           "/wAAAAAVYIkKAgAAAAIADAAAAFNlcmlhbE51bWJlcgEBWEMALgBEWEMAAAAM/////wEB/////wAAAAAk" +
           "YIAKAQAAAAEACwAAAEN1cnJlbnRVc2VyAQFhQwMAAAAAHQAAAFRoZSBnaXZlbiBuYW1lIG9mIHRoZSBk" +
           "ZXZpY2UuAC8BAf9GYUMAAP////8BAAAANWCJCgIAAAABAAUAAABMZXZlbAEBYkMDAAAAADUAAABUaGUg" +
           "d2VpZ2h0IG9mIHRoZSBsb2FkIG1vdW50ZWQgb24gb25lIG1vdW50aW5nIHBvaW50LgAuAERiQwAAAAz/" +
           "////AQH/////AAAAACRggAoBAAAAAQAKAAAAQ29tcG9uZW50cwEBZEMDAAAAABsBAABDb21wb25lbnRz" +
           "IGlzIGEgY29udGFpbmVyIGZvciBvbmUgb3IgbW9yZSBpbnN0YW5jZXMgb2Ygc3VidHlwZXMgb2YgQ29t" +
           "cG9uZW50VHlwZSBkZWZpbmVkIGluIE9QQyBVQSBESS4gVGhlIGxpc3RlZCBjb21wb25lbnRzIGFyZSBp" +
           "bnN0YWxsZWQgaW4gdGhlIG1vdGlvbiBkZXZpY2Ugc3lzdGVtLCBlLmcuIGEgcHJvY2Vzc2luZy11bml0" +
           "LCBhIHBvd2VyLXN1cHBseSwgYW4gSU8tYm9hcmQgb3IgYSBkcml2ZSwgYW5kIGhhdmUgYW4gZWxlY3Ry" +
           "aWNhbCBpbnRlcmZhY2UgdG8gdGhlIGNvbnRyb2xsZXIuAC8APWRDAAD/////AAAAACRggAoBAAAAAQAI" +
           "AAAAU29mdHdhcmUBAbg9AwAAAABXAAAAU29mdHdhcmUgaXMgYSBjb250YWluZXIgZm9yIG9uZSBvciBt" +
           "b3JlIGluc3RhbmNlcyBvZiBTb2Z0d2FyZVR5cGUgZGVmaW5lZCBpbiBPUEMgVUEgREkuAC8APbg9AAD/" +
           "////AAAAACRggAoBAAAAAQAMAAAAVGFza0NvbnRyb2xzAQHSPQMAAAAASQAAAFRhc2tDb250cm9scyBp" +
           "cyBhIGNvbnRhaW5lciBmb3Igb25lIG9yIG1vcmUgaW5zdGFuY2VzIG9mIFRhc2tDb250cm9sVHlwZS4A" +
           "LwA90j0AAP////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public UserState CurrentUser
        {
            get
            {
                return m_currentUser;
            }

            set
            {
                if (!Object.ReferenceEquals(m_currentUser, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_currentUser = value;
            }
        }

        /// <remarks />
        public FolderState Components
        {
            get
            {
                return m_components;
            }

            set
            {
                if (!Object.ReferenceEquals(m_components, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_components = value;
            }
        }

        /// <remarks />
        public FolderState Software
        {
            get
            {
                return m_software;
            }

            set
            {
                if (!Object.ReferenceEquals(m_software, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_software = value;
            }
        }

        /// <remarks />
        public FolderState TaskControls
        {
            get
            {
                return m_taskControls;
            }

            set
            {
                if (!Object.ReferenceEquals(m_taskControls, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_taskControls = value;
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
            if (m_currentUser != null)
            {
                children.Add(m_currentUser);
            }

            if (m_components != null)
            {
                children.Add(m_components);
            }

            if (m_software != null)
            {
                children.Add(m_software);
            }

            if (m_taskControls != null)
            {
                children.Add(m_taskControls);
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
                case Opc.Ua.Robotics.BrowseNames.CurrentUser:
                {
                    if (createOrReplace)
                    {
                        if (CurrentUser == null)
                        {
                            if (replacement == null)
                            {
                                CurrentUser = new UserState(this);
                            }
                            else
                            {
                                CurrentUser = (UserState)replacement;
                            }
                        }
                    }

                    instance = CurrentUser;
                    break;
                }

                case Opc.Ua.Robotics.BrowseNames.Components:
                {
                    if (createOrReplace)
                    {
                        if (Components == null)
                        {
                            if (replacement == null)
                            {
                                Components = new FolderState(this);
                            }
                            else
                            {
                                Components = (FolderState)replacement;
                            }
                        }
                    }

                    instance = Components;
                    break;
                }

                case Opc.Ua.Robotics.BrowseNames.Software:
                {
                    if (createOrReplace)
                    {
                        if (Software == null)
                        {
                            if (replacement == null)
                            {
                                Software = new FolderState(this);
                            }
                            else
                            {
                                Software = (FolderState)replacement;
                            }
                        }
                    }

                    instance = Software;
                    break;
                }

                case Opc.Ua.Robotics.BrowseNames.TaskControls:
                {
                    if (createOrReplace)
                    {
                        if (TaskControls == null)
                        {
                            if (replacement == null)
                            {
                                TaskControls = new FolderState(this);
                            }
                            else
                            {
                                TaskControls = (FolderState)replacement;
                            }
                        }
                    }

                    instance = TaskControls;
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
        private UserState m_currentUser;
        private FolderState m_components;
        private FolderState m_software;
        private FolderState m_taskControls;
        #endregion
    }
    #endif
    #endregion

    #region AuxiliaryComponentState Class
    #if (!OPCUA_EXCLUDE_AuxiliaryComponentState)
    /// <summary>
    /// Stores an instance of the AuxiliaryComponentType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class AuxiliaryComponentState : DeviceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public AuxiliaryComponentState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Robotics.ObjectTypes.AuxiliaryComponentType, Opc.Ua.Robotics.Namespaces.OpcUaRobotics, namespaceUris);
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
           "AgAAACUAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvUm9ib3RpY3MvHwAAAGh0dHA6Ly9vcGNm" +
           "b3VuZGF0aW9uLm9yZy9VQS9ESS//////BGCAAgEAAAABAB4AAABBdXhpbGlhcnlDb21wb25lbnRUeXBl" +
           "SW5zdGFuY2UBAT1FAQE9RT1FAAD/////CQAAADVgiQoCAAAAAgAMAAAATWFudWZhY3R1cmVyAQFURQMA" +
           "AAAAMAAAAE5hbWUgb2YgdGhlIGNvbXBhbnkgdGhhdCBtYW51ZmFjdHVyZWQgdGhlIGRldmljZQAuAERU" +
           "RQAAABX/////AQH/////AAAAADVgiQoCAAAAAgAFAAAATW9kZWwBAVZFAwAAAAAYAAAATW9kZWwgbmFt" +
           "ZSBvZiB0aGUgZGV2aWNlAC4ARFZFAAAAFf////8BAf////8AAAAANWCJCgIAAAACABAAAABIYXJkd2Fy" +
           "ZVJldmlzaW9uAQFYRQMAAAAALAAAAFJldmlzaW9uIGxldmVsIG9mIHRoZSBoYXJkd2FyZSBvZiB0aGUg" +
           "ZGV2aWNlAC4ARFhFAAAADP////8BAf////8AAAAANWCJCgIAAAACABAAAABTb2Z0d2FyZVJldmlzaW9u" +
           "AQFZRQMAAAAANQAAAFJldmlzaW9uIGxldmVsIG9mIHRoZSBzb2Z0d2FyZS9maXJtd2FyZSBvZiB0aGUg" +
           "ZGV2aWNlAC4ARFlFAAAADP////8BAf////8AAAAANWCJCgIAAAACAA4AAABEZXZpY2VSZXZpc2lvbgEB" +
           "WkUDAAAAACQAAABPdmVyYWxsIHJldmlzaW9uIGxldmVsIG9mIHRoZSBkZXZpY2UALgBEWkUAAAAM////" +
           "/wEB/////wAAAAA1YIkKAgAAAAIACwAAAFByb2R1Y3RDb2RlAQFcRQMAAAAAuAAAAFRoZSBQcm9kdWN0" +
           "Q29kZSBwcm9wZXJ0eSBwcm92aWRlcyBhIHVuaXF1ZSBjb21iaW5hdGlvbiBvZiBudW1iZXJzIGFuZCBs" +
           "ZXR0ZXJzIHVzZWQgdG8gaWRlbnRpZnkgdGhlIHByb2R1Y3QuIEl0IG1heSBiZSB0aGUgb3JkZXIgaW5m" +
           "b3JtYXRpb24gZGlzcGxheWVkIG9uIHR5cGUgc2hpZWxkcyBvciBpbiBFUlAgc3lzdGVtcy4ALgBEXEUA" +
           "AAAM/////wEB/////wAAAAA1YIkKAgAAAAIADAAAAERldmljZU1hbnVhbAEBXUUDAAAAAFoAAABBZGRy" +
           "ZXNzIChwYXRobmFtZSBpbiB0aGUgZmlsZSBzeXN0ZW0gb3IgYSBVUkwgfCBXZWIgYWRkcmVzcykgb2Yg" +
           "dXNlciBtYW51YWwgZm9yIHRoZSBkZXZpY2UALgBEXUUAAAAM/////wEB/////wAAAAA1YIkKAgAAAAIA" +
           "DAAAAFNlcmlhbE51bWJlcgEBV0UDAAAAAE0AAABJZGVudGlmaWVyIHRoYXQgdW5pcXVlbHkgaWRlbnRp" +
           "Zmllcywgd2l0aGluIGEgbWFudWZhY3R1cmVyLCBhIGRldmljZSBpbnN0YW5jZQAuAERXRQAAAAz/////" +
           "AQH/////AAAAADVgiQoCAAAAAgAPAAAAUmV2aXNpb25Db3VudGVyAQFbRQMAAAAAaQAAAEFuIGluY3Jl" +
           "bWVudGFsIGNvdW50ZXIgaW5kaWNhdGluZyB0aGUgbnVtYmVyIG9mIHRpbWVzIHRoZSBzdGF0aWMgZGF0" +
           "YSB3aXRoaW4gdGhlIERldmljZSBoYXMgYmVlbiBtb2RpZmllZAAuAERbRQAAAAb/////AQH/////AAAA" +
           "AA==";
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

    #region DriveState Class
    #if (!OPCUA_EXCLUDE_DriveState)
    /// <summary>
    /// Stores an instance of the DriveType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class DriveState : DeviceState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public DriveState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Robotics.ObjectTypes.DriveType, Opc.Ua.Robotics.Namespaces.OpcUaRobotics, namespaceUris);
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
           "AgAAACUAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvUm9ib3RpY3MvHwAAAGh0dHA6Ly9vcGNm" +
           "b3VuZGF0aW9uLm9yZy9VQS9ESS//////BGCAAgEAAAABABEAAABEcml2ZVR5cGVJbnN0YW5jZQEBgUUB" +
           "AYFFgUUAAP////8JAAAANWCJCgIAAAACAAwAAABNYW51ZmFjdHVyZXIBAZhFAwAAAAAwAAAATmFtZSBv" +
           "ZiB0aGUgY29tcGFueSB0aGF0IG1hbnVmYWN0dXJlZCB0aGUgZGV2aWNlAC4ARJhFAAAAFf////8BAf//" +
           "//8AAAAANWCJCgIAAAACAAUAAABNb2RlbAEBmkUDAAAAABgAAABNb2RlbCBuYW1lIG9mIHRoZSBkZXZp" +
           "Y2UALgBEmkUAAAAV/////wEB/////wAAAAA1YIkKAgAAAAIAEAAAAEhhcmR3YXJlUmV2aXNpb24BAZxF" +
           "AwAAAAAsAAAAUmV2aXNpb24gbGV2ZWwgb2YgdGhlIGhhcmR3YXJlIG9mIHRoZSBkZXZpY2UALgBEnEUA" +
           "AAAM/////wEB/////wAAAAA1YIkKAgAAAAIAEAAAAFNvZnR3YXJlUmV2aXNpb24BAZ1FAwAAAAA1AAAA" +
           "UmV2aXNpb24gbGV2ZWwgb2YgdGhlIHNvZnR3YXJlL2Zpcm13YXJlIG9mIHRoZSBkZXZpY2UALgBEnUUA" +
           "AAAM/////wEB/////wAAAAA1YIkKAgAAAAIADgAAAERldmljZVJldmlzaW9uAQGeRQMAAAAAJAAAAE92" +
           "ZXJhbGwgcmV2aXNpb24gbGV2ZWwgb2YgdGhlIGRldmljZQAuAESeRQAAAAz/////AQH/////AAAAADVg" +
           "iQoCAAAAAgALAAAAUHJvZHVjdENvZGUBAaBFAwAAAAC4AAAAVGhlIFByb2R1Y3RDb2RlIHByb3BlcnR5" +
           "IHByb3ZpZGVzIGEgdW5pcXVlIGNvbWJpbmF0aW9uIG9mIG51bWJlcnMgYW5kIGxldHRlcnMgdXNlZCB0" +
           "byBpZGVudGlmeSB0aGUgcHJvZHVjdC4gSXQgbWF5IGJlIHRoZSBvcmRlciBpbmZvcm1hdGlvbiBkaXNw" +
           "bGF5ZWQgb24gdHlwZSBzaGllbGRzIG9yIGluIEVSUCBzeXN0ZW1zLgAuAESgRQAAAAz/////AQH/////" +
           "AAAAADVgiQoCAAAAAgAMAAAARGV2aWNlTWFudWFsAQGhRQMAAAAAWgAAAEFkZHJlc3MgKHBhdGhuYW1l" +
           "IGluIHRoZSBmaWxlIHN5c3RlbSBvciBhIFVSTCB8IFdlYiBhZGRyZXNzKSBvZiB1c2VyIG1hbnVhbCBm" +
           "b3IgdGhlIGRldmljZQAuAEShRQAAAAz/////AQH/////AAAAADVgiQoCAAAAAgAMAAAAU2VyaWFsTnVt" +
           "YmVyAQGbRQMAAAAATQAAAElkZW50aWZpZXIgdGhhdCB1bmlxdWVseSBpZGVudGlmaWVzLCB3aXRoaW4g" +
           "YSBtYW51ZmFjdHVyZXIsIGEgZGV2aWNlIGluc3RhbmNlAC4ARJtFAAAADP////8BAf////8AAAAANWCJ" +
           "CgIAAAACAA8AAABSZXZpc2lvbkNvdW50ZXIBAZ9FAwAAAABpAAAAQW4gaW5jcmVtZW50YWwgY291bnRl" +
           "ciBpbmRpY2F0aW5nIHRoZSBudW1iZXIgb2YgdGltZXMgdGhlIHN0YXRpYyBkYXRhIHdpdGhpbiB0aGUg" +
           "RGV2aWNlIGhhcyBiZWVuIG1vZGlmaWVkAC4ARJ9FAAAABv////8BAf////8AAAAA";
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

    #region TaskControlState Class
    #if (!OPCUA_EXCLUDE_TaskControlState)
    /// <summary>
    /// Stores an instance of the TaskControlType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class TaskControlState : ComponentState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public TaskControlState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Robotics.ObjectTypes.TaskControlType, Opc.Ua.Robotics.Namespaces.OpcUaRobotics, namespaceUris);
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
           "AgAAACUAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvUm9ib3RpY3MvHwAAAGh0dHA6Ly9vcGNm" +
           "b3VuZGF0aW9uLm9yZy9VQS9ESS//////BGCAAgEAAAABABcAAABUYXNrQ29udHJvbFR5cGVJbnN0YW5j" +
           "ZQEB8wMBAfMD8wMAAP////8CAAAAJGCACgEAAAACAAwAAABQYXJhbWV0ZXJTZXQBAQs+AwAAAAAXAAAA" +
           "RmxhdCBsaXN0IG9mIFBhcmFtZXRlcnMALwA6Cz4AAP////8DAAAANWCJCgIAAAABAA8AAABUYXNrUHJv" +
           "Z3JhbU5hbWUBAdJFAwAAAAAxAAAAQSBjdXN0b21lciBnaXZlbiBpZGVudGlmaWVyIGZvciB0aGUgdGFz" +
           "ayBwcm9ncmFtLgAvAD/SRQAAAAz/////AQH/////AAAAADVgiQoCAAAAAQARAAAAVGFza1Byb2dyYW1M" +
           "b2FkZWQBAdNFAwAAAABoAAAAVGhlIFRhc2tQcm9ncmFtTG9hZGVkIHZhcmlhYmxlIGlzIFRSVUUgaWYg" +
           "YSB0YXNrIHByb2dyYW0gaXMgbG9hZGVkIGluIHRoZSB0YXNrIGNvbnRyb2wsIEZBTFNFIG90aGVyd2lz" +
           "ZS4ALwA/00UAAAAB/////wEB/////wAAAAA1YIkKAgAAAAEADQAAAEV4ZWN1dGlvbk1vZGUBAdRFAwAA" +
           "AAA9AAAARXhlY3V0aW9uIG1vZGUgb2YgdGhlIHRhc2sgY29udHJvbCAoY29udGludW91cyBvciBzdGVw" +
           "LXdpc2UpLgAvAD/URQAAAQEPR/////8BAf////8AAAAANWCJCgIAAAACAA0AAABDb21wb25lbnROYW1l" +
           "AQHRRQMAAAAATgAAAEEgdXNlciB3cml0YWJsZSBuYW1lIHByb3ZpZGVkIGJ5IHRoZSB2ZW5kb3IsIGlu" +
           "dGVncmF0b3Igb3IgdXNlciBvZiB0aGUgZGV2aWNlLgAuAETRRQAAABX/////AQH/////AAAAAA==";
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

    #region LoadState Class
    #if (!OPCUA_EXCLUDE_LoadState)
    /// <summary>
    /// Stores an instance of the LoadType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class LoadState : BaseObjectState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public LoadState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Robotics.ObjectTypes.LoadType, Opc.Ua.Robotics.Namespaces.OpcUaRobotics, namespaceUris);
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

            if (CenterOfMass != null)
            {
                CenterOfMass.Initialize(context, CenterOfMass_InitializationString);
            }

            if (Inertia != null)
            {
                Inertia.Initialize(context, Inertia_InitializationString);
            }
        }

        #region Initialization String
        private const string CenterOfMass_InitializationString =
           "AgAAACUAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvUm9ib3RpY3MvHwAAAGh0dHA6Ly9vcGNm" +
           "b3VuZGF0aW9uLm9yZy9VQS9ESS//////NWCJCgIAAAABAAwAAABDZW50ZXJPZk1hc3MBAX0XAwAAAACM" +
           "AQAAVGhlIHBvc2l0aW9uIGFuZCBvcmllbnRhdGlvbiBvZiB0aGUgY2VudGVyIG9mIHRoZSBtYXNzIHJl" +
           "bGF0ZWQgdG8gdGhlIG1vdW50aW5nIHBvaW50IHVzaW5nIGEgRnJhbWVUeXBlLiBYLCBZLCBaIGRlZmlu" +
           "ZSB0aGUgcG9zaXRpb24gb2YgdGhlIGNlbnRlciBvZiBncmF2aXR5IHJlbGF0aXZlIHRvIHRoZSBtb3Vu" +
           "dGluZyBwb2ludCBjb29yZGluYXRlIHN5c3RlbS4gQSwgQiwgQyBkZWZpbmUgdGhlIG9yaWVudGF0aW9u" +
           "IG9mIHRoZSBwcmluY2lwYWwgYXhlcyBvZiBpbmVydGlhIHJlbGF0aXZlIHRvIHRoZSBtb3VudGluZyBw" +
           "b2ludCBjb29yZGluYXRlIHN5c3RlbS4gT3JpZW50YXRpb24gQSwgQiwgQyBjYW4gYmUgIjAiIGZvciBz" +
           "eXN0ZW1zIHdoaWNoIGRvIG5vdCBuZWVkIHRoZXNlICB2YWx1ZXMuAC8BAGdJfRcAAAEAfkn/////AQH/" +
           "////AgAAABVgiQoCAAAAAAAUAAAAQ2FydGVzaWFuQ29vcmRpbmF0ZXMBAQI/AC8BAFZJAj8AAAEAekn/" +
           "////AQH/////AwAAABVgiQoCAAAAAAABAAAAWAEBBj8ALwA/Bj8AAAAL/////wEB/////wAAAAAVYIkK" +
           "AgAAAAAAAQAAAFkBAQc/AC8APwc/AAAAC/////8BAf////8AAAAAFWCJCgIAAAAAAAEAAABaAQEIPwAv" +
           "AD8IPwAAAAv/////AQH/////AAAAABVgiQoCAAAAAAALAAAAT3JpZW50YXRpb24BAQQ/AC8BAF1JBD8A" +
           "AAEAfEn/////AQH/////AwAAABVgiQoCAAAAAAABAAAAQQEBCT8ALwA/CT8AAAAL/////wEB/////wAA" +
           "AAAVYIkKAgAAAAAAAQAAAEIBAQo/AC8APwo/AAAAC/////8BAf////8AAAAAFWCJCgIAAAAAAAEAAABD" +
           "AQELPwAvAD8LPwAAAAv/////AQH/////AAAAAA==";

        private const string Inertia_InitializationString =
           "AgAAACUAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvUm9ib3RpY3MvHwAAAGh0dHA6Ly9vcGNm" +
           "b3VuZGF0aW9uLm9yZy9VQS9ESS//////NWCJCgIAAAABAAcAAABJbmVydGlhAQH6RgMAAAAA+gAAAFRo" +
           "ZSBJbmVydGlhIHVzZXMgdGhlIFZlY3RvclR5cGUgdG8gZGVzY3JpYmUgdGhlIHRocmVlIHZhbHVlcyBv" +
           "ZiB0aGUgcHJpbmNpcGFsIG1vbWVudHMgb2YgaW5lcnRpYSB3aXRoIHJlc3BlY3QgdG8gdGhlIG1vdW50" +
           "aW5nIHBvaW50IGNvb3JkaW5hdGUgc3lzdGVtLiBJZiBpbmVydGlhIHZhbHVlcyBhcmUgcHJvdmlkZWQg" +
           "Zm9yIHJvdGFyeSBheGlzIHRoZSBDZW50ZXJPZk1hc3Mgc2hhbGwgYmUgY29tcGxldGVseSBmaWxsZWQg" +
           "YXMgd2VsbC4ALwEANEX6RgAAAQB4Sf////8BAf////8DAAAAFWCJCgIAAAAAAAEAAABYAQH7RgAvAD/7" +
           "RgAAAAv/////AQH/////AAAAABVgiQoCAAAAAAABAAAAWQEB/EYALwA//EYAAAAL/////wEB/////wAA" +
           "AAAVYIkKAgAAAAAAAQAAAFoBAf1GAC8AP/1GAAAAC/////8BAf////8AAAAA";

        private const string InitializationString =
           "AgAAACUAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvUm9ib3RpY3MvHwAAAGh0dHA6Ly9vcGNm" +
           "b3VuZGF0aW9uLm9yZy9VQS9ESS//////BGCAAgEAAAABABAAAABMb2FkVHlwZUluc3RhbmNlAQH6AwEB" +
           "+gP6AwAA/////wMAAAA1YIkKAgAAAAEABAAAAE1hc3MBAUMaAwAAAAA1AAAAVGhlIHdlaWdodCBvZiB0" +
           "aGUgbG9hZCBtb3VudGVkIG9uIG9uZSBtb3VudGluZyBwb2ludC4ALwEAWURDGgAAAAv/////AQH/////" +
           "AQAAABVgiQoCAAAAAAAQAAAARW5naW5lZXJpbmdVbml0cwEBSBoALgBESBoAAAEAdwP/////AQH/////" +
           "AAAAADVgiQoCAAAAAQAMAAAAQ2VudGVyT2ZNYXNzAQF9FwMAAAAAjAEAAFRoZSBwb3NpdGlvbiBhbmQg" +
           "b3JpZW50YXRpb24gb2YgdGhlIGNlbnRlciBvZiB0aGUgbWFzcyByZWxhdGVkIHRvIHRoZSBtb3VudGlu" +
           "ZyBwb2ludCB1c2luZyBhIEZyYW1lVHlwZS4gWCwgWSwgWiBkZWZpbmUgdGhlIHBvc2l0aW9uIG9mIHRo" +
           "ZSBjZW50ZXIgb2YgZ3Jhdml0eSByZWxhdGl2ZSB0byB0aGUgbW91bnRpbmcgcG9pbnQgY29vcmRpbmF0" +
           "ZSBzeXN0ZW0uIEEsIEIsIEMgZGVmaW5lIHRoZSBvcmllbnRhdGlvbiBvZiB0aGUgcHJpbmNpcGFsIGF4" +
           "ZXMgb2YgaW5lcnRpYSByZWxhdGl2ZSB0byB0aGUgbW91bnRpbmcgcG9pbnQgY29vcmRpbmF0ZSBzeXN0" +
           "ZW0uIE9yaWVudGF0aW9uIEEsIEIsIEMgY2FuIGJlICIwIiBmb3Igc3lzdGVtcyB3aGljaCBkbyBub3Qg" +
           "bmVlZCB0aGVzZSAgdmFsdWVzLgAvAQBnSX0XAAABAH5J/////wEB/////wIAAAAVYIkKAgAAAAAAFAAA" +
           "AENhcnRlc2lhbkNvb3JkaW5hdGVzAQECPwAvAQBWSQI/AAABAHpJ/////wEB/////wMAAAAVYIkKAgAA" +
           "AAAAAQAAAFgBAQY/AC8APwY/AAAAC/////8BAf////8AAAAAFWCJCgIAAAAAAAEAAABZAQEHPwAvAD8H" +
           "PwAAAAv/////AQH/////AAAAABVgiQoCAAAAAAABAAAAWgEBCD8ALwA/CD8AAAAL/////wEB/////wAA" +
           "AAAVYIkKAgAAAAAACwAAAE9yaWVudGF0aW9uAQEEPwAvAQBdSQQ/AAABAHxJ/////wEB/////wMAAAAV" +
           "YIkKAgAAAAAAAQAAAEEBAQk/AC8APwk/AAAAC/////8BAf////8AAAAAFWCJCgIAAAAAAAEAAABCAQEK" +
           "PwAvAD8KPwAAAAv/////AQH/////AAAAABVgiQoCAAAAAAABAAAAQwEBCz8ALwA/Cz8AAAAL/////wEB" +
           "/////wAAAAA1YIkKAgAAAAEABwAAAEluZXJ0aWEBAfpGAwAAAAD6AAAAVGhlIEluZXJ0aWEgdXNlcyB0" +
           "aGUgVmVjdG9yVHlwZSB0byBkZXNjcmliZSB0aGUgdGhyZWUgdmFsdWVzIG9mIHRoZSBwcmluY2lwYWwg" +
           "bW9tZW50cyBvZiBpbmVydGlhIHdpdGggcmVzcGVjdCB0byB0aGUgbW91bnRpbmcgcG9pbnQgY29vcmRp" +
           "bmF0ZSBzeXN0ZW0uIElmIGluZXJ0aWEgdmFsdWVzIGFyZSBwcm92aWRlZCBmb3Igcm90YXJ5IGF4aXMg" +
           "dGhlIENlbnRlck9mTWFzcyBzaGFsbCBiZSBjb21wbGV0ZWx5IGZpbGxlZCBhcyB3ZWxsLgAvAQA0RfpG" +
           "AAABAHhJ/////wEB/////wMAAAAVYIkKAgAAAAAAAQAAAFgBAftGAC8AP/tGAAAAC/////8BAf////8A" +
           "AAAAFWCJCgIAAAAAAAEAAABZAQH8RgAvAD/8RgAAAAv/////AQH/////AAAAABVgiQoCAAAAAAABAAAA" +
           "WgEB/UYALwA//UYAAAAL/////wEB/////wAAAAA=";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public AnalogUnitState<double> Mass
        {
            get
            {
                return m_mass;
            }

            set
            {
                if (!Object.ReferenceEquals(m_mass, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_mass = value;
            }
        }

        /// <remarks />
        public ThreeDFrameState CenterOfMass
        {
            get
            {
                return m_centerOfMass;
            }

            set
            {
                if (!Object.ReferenceEquals(m_centerOfMass, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_centerOfMass = value;
            }
        }

        /// <remarks />
        public ThreeDVectorState Inertia
        {
            get
            {
                return m_inertia;
            }

            set
            {
                if (!Object.ReferenceEquals(m_inertia, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_inertia = value;
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
            if (m_mass != null)
            {
                children.Add(m_mass);
            }

            if (m_centerOfMass != null)
            {
                children.Add(m_centerOfMass);
            }

            if (m_inertia != null)
            {
                children.Add(m_inertia);
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
                case Opc.Ua.Robotics.BrowseNames.Mass:
                {
                    if (createOrReplace)
                    {
                        if (Mass == null)
                        {
                            if (replacement == null)
                            {
                                Mass = new AnalogUnitState<double>(this);
                            }
                            else
                            {
                                Mass = (AnalogUnitState<double>)replacement;
                            }
                        }
                    }

                    instance = Mass;
                    break;
                }

                case Opc.Ua.Robotics.BrowseNames.CenterOfMass:
                {
                    if (createOrReplace)
                    {
                        if (CenterOfMass == null)
                        {
                            if (replacement == null)
                            {
                                CenterOfMass = new ThreeDFrameState(this);
                            }
                            else
                            {
                                CenterOfMass = (ThreeDFrameState)replacement;
                            }
                        }
                    }

                    instance = CenterOfMass;
                    break;
                }

                case Opc.Ua.Robotics.BrowseNames.Inertia:
                {
                    if (createOrReplace)
                    {
                        if (Inertia == null)
                        {
                            if (replacement == null)
                            {
                                Inertia = new ThreeDVectorState(this);
                            }
                            else
                            {
                                Inertia = (ThreeDVectorState)replacement;
                            }
                        }
                    }

                    instance = Inertia;
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
        private AnalogUnitState<double> m_mass;
        private ThreeDFrameState m_centerOfMass;
        private ThreeDVectorState m_inertia;
        #endregion
    }
    #endif
    #endregion

    #region UserState Class
    #if (!OPCUA_EXCLUDE_UserState)
    /// <summary>
    /// Stores an instance of the UserType ObjectType.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public partial class UserState : BaseObjectState
    {
        #region Constructors
        /// <summary>
        /// Initializes the type with its default attribute values.
        /// </summary>
        public UserState(NodeState parent) : base(parent)
        {
        }

        /// <summary>
        /// Returns the id of the default type definition node for the instance.
        /// </summary>
        protected override NodeId GetDefaultTypeDefinitionId(NamespaceTable namespaceUris)
        {
            return Opc.Ua.NodeId.Create(Opc.Ua.Robotics.ObjectTypes.UserType, Opc.Ua.Robotics.Namespaces.OpcUaRobotics, namespaceUris);
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

            if (Name != null)
            {
                Name.Initialize(context, Name_InitializationString);
            }
        }

        #region Initialization String
        private const string Name_InitializationString =
           "AgAAACUAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvUm9ib3RpY3MvHwAAAGh0dHA6Ly9vcGNm" +
           "b3VuZGF0aW9uLm9yZy9VQS9ESS//////NWCJCgIAAAABAAQAAABOYW1lAQEBRwMAAAAAOAAAAFRoZSBu" +
           "YW1lIGZvciB0aGUgY3VycmVudCB1c2VyIHdpdGhpbiB0aGUgY29udHJvbCBzeXN0ZW0uAC4ARAFHAAAA" +
           "DP////8BAf////8AAAAA";

        private const string InitializationString =
           "AgAAACUAAABodHRwOi8vb3BjZm91bmRhdGlvbi5vcmcvVUEvUm9ib3RpY3MvHwAAAGh0dHA6Ly9vcGNm" +
           "b3VuZGF0aW9uLm9yZy9VQS9ESS//////BGCAAgEAAAABABAAAABVc2VyVHlwZUluc3RhbmNlAQH/RgEB" +
           "/0b/RgAA/////wIAAAA1YIkKAgAAAAEABQAAAExldmVsAQEARwMAAAAANQAAAFRoZSB3ZWlnaHQgb2Yg" +
           "dGhlIGxvYWQgbW91bnRlZCBvbiBvbmUgbW91bnRpbmcgcG9pbnQuAC4ARABHAAAADP////8BAf////8A" +
           "AAAANWCJCgIAAAABAAQAAABOYW1lAQEBRwMAAAAAOAAAAFRoZSBuYW1lIGZvciB0aGUgY3VycmVudCB1" +
           "c2VyIHdpdGhpbiB0aGUgY29udHJvbCBzeXN0ZW0uAC4ARAFHAAAADP////8BAf////8AAAAA";
        #endregion
        #endif
        #endregion

        #region Public Properties
        /// <remarks />
        public PropertyState<string> Level
        {
            get
            {
                return m_level;
            }

            set
            {
                if (!Object.ReferenceEquals(m_level, value))
                {
                    ChangeMasks |= NodeStateChangeMasks.Children;
                }

                m_level = value;
            }
        }

        /// <remarks />
        public PropertyState<string> Name
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
            if (m_level != null)
            {
                children.Add(m_level);
            }

            if (m_name != null)
            {
                children.Add(m_name);
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
                case Opc.Ua.Robotics.BrowseNames.Level:
                {
                    if (createOrReplace)
                    {
                        if (Level == null)
                        {
                            if (replacement == null)
                            {
                                Level = new PropertyState<string>(this);
                            }
                            else
                            {
                                Level = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = Level;
                    break;
                }

                case Opc.Ua.Robotics.BrowseNames.Name:
                {
                    if (createOrReplace)
                    {
                        if (Name == null)
                        {
                            if (replacement == null)
                            {
                                Name = new PropertyState<string>(this);
                            }
                            else
                            {
                                Name = (PropertyState<string>)replacement;
                            }
                        }
                    }

                    instance = Name;
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
        private PropertyState<string> m_level;
        private PropertyState<string> m_name;
        #endregion
    }
    #endif
    #endregion
}