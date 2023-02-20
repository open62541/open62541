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
using System.Reflection;
using System.Xml;
using System.Runtime.Serialization;
using Opc.Ua.Di;
using Opc.Ua;

namespace Opc.Ua.Robotics
{
    #region DataType Identifiers
    /// <summary>
    /// A class that declares constants for all DataTypes in the Model Design.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class DataTypes
    {
        /// <summary>
        /// The identifier for the ExecutionModeEnumeration DataType.
        /// </summary>
        public const uint ExecutionModeEnumeration = 18191;

        /// <summary>
        /// The identifier for the MotionDeviceCategoryEnumeration DataType.
        /// </summary>
        public const uint MotionDeviceCategoryEnumeration = 18193;

        /// <summary>
        /// The identifier for the AxisMotionProfileEnumeration DataType.
        /// </summary>
        public const uint AxisMotionProfileEnumeration = 3008;

        /// <summary>
        /// The identifier for the OperationalModeEnumeration DataType.
        /// </summary>
        public const uint OperationalModeEnumeration = 3006;
    }
    #endregion

    #region Method Identifiers
    /// <summary>
    /// A class that declares constants for all Methods in the Model Design.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class Methods
    {
        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Open Method.
        /// </summary>
        public const uint OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Open = 15476;

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Close Method.
        /// </summary>
        public const uint OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Close = 15550;

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Read Method.
        /// </summary>
        public const uint OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Read = 15553;

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Write Method.
        /// </summary>
        public const uint OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Write = 15575;

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_GetPosition Method.
        /// </summary>
        public const uint OPCUAROBOTICSNamespaceMetadata_NamespaceFile_GetPosition = 15577;

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_SetPosition Method.
        /// </summary>
        public const uint OPCUAROBOTICSNamespaceMetadata_NamespaceFile_SetPosition = 15580;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Lock_InitLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_Lock_InitLock = 15015;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Lock_RenewLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_Lock_RenewLock = 15018;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Lock_ExitLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_Lock_ExitLock = 15020;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Lock_BreakLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_Lock_BreakLock = 15022;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_InitLock = 15036;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_RenewLock = 15039;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_ExitLock = 15041;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_BreakLock = 15043;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock = 15076;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RenewLock = 15079;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_ExitLock = 15081;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_BreakLock = 15083;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock = 15228;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RenewLock = 15237;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_ExitLock = 15242;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_BreakLock = 15244;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock = 15382;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock = 15385;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock = 15387;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock = 15389;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_InitLock = 15417;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_RenewLock = 15420;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_ExitLock = 15422;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_BreakLock = 15424;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_InitLock = 15457;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_RenewLock = 15462;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_ExitLock = 15464;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_BreakLock = 15466;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_InitLock = 15496;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_RenewLock = 15499;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_ExitLock = 15501;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_BreakLock = 15503;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_InitLock = 15532;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_RenewLock = 15535;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_ExitLock = 15537;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_BreakLock = 15539;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_InitLock = 15709;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_RenewLock = 15712;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_ExitLock = 15714;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_BreakLock = 15716;

        /// <summary>
        /// The identifier for the MotionDeviceType_Lock_InitLock Method.
        /// </summary>
        public const uint MotionDeviceType_Lock_InitLock = 15118;

        /// <summary>
        /// The identifier for the MotionDeviceType_Lock_RenewLock Method.
        /// </summary>
        public const uint MotionDeviceType_Lock_RenewLock = 15121;

        /// <summary>
        /// The identifier for the MotionDeviceType_Lock_ExitLock Method.
        /// </summary>
        public const uint MotionDeviceType_Lock_ExitLock = 15123;

        /// <summary>
        /// The identifier for the MotionDeviceType_Lock_BreakLock Method.
        /// </summary>
        public const uint MotionDeviceType_Lock_BreakLock = 15125;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_Lock_InitLock = 15756;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_Lock_RenewLock = 15759;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_Lock_ExitLock = 15761;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_Lock_BreakLock = 15763;

        /// <summary>
        /// The identifier for the MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_InitLock = 18208;

        /// <summary>
        /// The identifier for the MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_RenewLock = 18211;

        /// <summary>
        /// The identifier for the MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_ExitLock = 18213;

        /// <summary>
        /// The identifier for the MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_BreakLock = 18215;

        /// <summary>
        /// The identifier for the MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock = 18322;

        /// <summary>
        /// The identifier for the MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock = 18325;

        /// <summary>
        /// The identifier for the MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock = 18327;

        /// <summary>
        /// The identifier for the MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock = 18329;

        /// <summary>
        /// The identifier for the AxisType_Lock_InitLock Method.
        /// </summary>
        public const uint AxisType_Lock_InitLock = 16615;

        /// <summary>
        /// The identifier for the AxisType_Lock_RenewLock Method.
        /// </summary>
        public const uint AxisType_Lock_RenewLock = 16618;

        /// <summary>
        /// The identifier for the AxisType_Lock_ExitLock Method.
        /// </summary>
        public const uint AxisType_Lock_ExitLock = 16620;

        /// <summary>
        /// The identifier for the AxisType_Lock_BreakLock Method.
        /// </summary>
        public const uint AxisType_Lock_BreakLock = 16622;

        /// <summary>
        /// The identifier for the AxisType_PowerTrainIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint AxisType_PowerTrainIdentifier_Lock_InitLock = 18356;

        /// <summary>
        /// The identifier for the AxisType_PowerTrainIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint AxisType_PowerTrainIdentifier_Lock_RenewLock = 18359;

        /// <summary>
        /// The identifier for the AxisType_PowerTrainIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint AxisType_PowerTrainIdentifier_Lock_ExitLock = 18361;

        /// <summary>
        /// The identifier for the AxisType_PowerTrainIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint AxisType_PowerTrainIdentifier_Lock_BreakLock = 18363;

        /// <summary>
        /// The identifier for the PowerTrainType_Lock_InitLock Method.
        /// </summary>
        public const uint PowerTrainType_Lock_InitLock = 16808;

        /// <summary>
        /// The identifier for the PowerTrainType_Lock_RenewLock Method.
        /// </summary>
        public const uint PowerTrainType_Lock_RenewLock = 16811;

        /// <summary>
        /// The identifier for the PowerTrainType_Lock_ExitLock Method.
        /// </summary>
        public const uint PowerTrainType_Lock_ExitLock = 16813;

        /// <summary>
        /// The identifier for the PowerTrainType_Lock_BreakLock Method.
        /// </summary>
        public const uint PowerTrainType_Lock_BreakLock = 16815;

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint PowerTrainType_MotorIdentifier_Lock_InitLock = 16010;

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint PowerTrainType_MotorIdentifier_Lock_RenewLock = 16013;

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint PowerTrainType_MotorIdentifier_Lock_ExitLock = 16015;

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint PowerTrainType_MotorIdentifier_Lock_BreakLock = 16017;

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint PowerTrainType_GearIdentifier_Lock_InitLock = 16053;

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint PowerTrainType_GearIdentifier_Lock_RenewLock = 16056;

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint PowerTrainType_GearIdentifier_Lock_ExitLock = 16058;

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint PowerTrainType_GearIdentifier_Lock_BreakLock = 16060;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_Lock_InitLock = 18548;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_Lock_RenewLock = 18551;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_Lock_ExitLock = 18553;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_Lock_BreakLock = 18555;

        /// <summary>
        /// The identifier for the PowerTrainType_PowerTrainIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint PowerTrainType_PowerTrainIdentifier_Lock_InitLock = 18625;

        /// <summary>
        /// The identifier for the PowerTrainType_PowerTrainIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint PowerTrainType_PowerTrainIdentifier_Lock_RenewLock = 18628;

        /// <summary>
        /// The identifier for the PowerTrainType_PowerTrainIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint PowerTrainType_PowerTrainIdentifier_Lock_ExitLock = 18630;

        /// <summary>
        /// The identifier for the PowerTrainType_PowerTrainIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint PowerTrainType_PowerTrainIdentifier_Lock_BreakLock = 18632;

        /// <summary>
        /// The identifier for the MotorType_Lock_InitLock Method.
        /// </summary>
        public const uint MotorType_Lock_InitLock = 15566;

        /// <summary>
        /// The identifier for the MotorType_Lock_RenewLock Method.
        /// </summary>
        public const uint MotorType_Lock_RenewLock = 15569;

        /// <summary>
        /// The identifier for the MotorType_Lock_ExitLock Method.
        /// </summary>
        public const uint MotorType_Lock_ExitLock = 15571;

        /// <summary>
        /// The identifier for the MotorType_Lock_BreakLock Method.
        /// </summary>
        public const uint MotorType_Lock_BreakLock = 15573;

        /// <summary>
        /// The identifier for the GearType_Lock_InitLock Method.
        /// </summary>
        public const uint GearType_Lock_InitLock = 15928;

        /// <summary>
        /// The identifier for the GearType_Lock_RenewLock Method.
        /// </summary>
        public const uint GearType_Lock_RenewLock = 15931;

        /// <summary>
        /// The identifier for the GearType_Lock_ExitLock Method.
        /// </summary>
        public const uint GearType_Lock_ExitLock = 15933;

        /// <summary>
        /// The identifier for the GearType_Lock_BreakLock Method.
        /// </summary>
        public const uint GearType_Lock_BreakLock = 15935;

        /// <summary>
        /// The identifier for the SafetyStateType_Lock_InitLock Method.
        /// </summary>
        public const uint SafetyStateType_Lock_InitLock = 15177;

        /// <summary>
        /// The identifier for the SafetyStateType_Lock_RenewLock Method.
        /// </summary>
        public const uint SafetyStateType_Lock_RenewLock = 15180;

        /// <summary>
        /// The identifier for the SafetyStateType_Lock_ExitLock Method.
        /// </summary>
        public const uint SafetyStateType_Lock_ExitLock = 15182;

        /// <summary>
        /// The identifier for the SafetyStateType_Lock_BreakLock Method.
        /// </summary>
        public const uint SafetyStateType_Lock_BreakLock = 15184;

        /// <summary>
        /// The identifier for the ControllerType_Lock_InitLock Method.
        /// </summary>
        public const uint ControllerType_Lock_InitLock = 15149;

        /// <summary>
        /// The identifier for the ControllerType_Lock_RenewLock Method.
        /// </summary>
        public const uint ControllerType_Lock_RenewLock = 15152;

        /// <summary>
        /// The identifier for the ControllerType_Lock_ExitLock Method.
        /// </summary>
        public const uint ControllerType_Lock_ExitLock = 15154;

        /// <summary>
        /// The identifier for the ControllerType_Lock_BreakLock Method.
        /// </summary>
        public const uint ControllerType_Lock_BreakLock = 15156;

        /// <summary>
        /// The identifier for the ControllerType_Components_ComponentIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ControllerType_Components_ComponentIdentifier_Lock_InitLock = 18825;

        /// <summary>
        /// The identifier for the ControllerType_Components_ComponentIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ControllerType_Components_ComponentIdentifier_Lock_RenewLock = 18828;

        /// <summary>
        /// The identifier for the ControllerType_Components_ComponentIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ControllerType_Components_ComponentIdentifier_Lock_ExitLock = 18830;

        /// <summary>
        /// The identifier for the ControllerType_Components_ComponentIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ControllerType_Components_ComponentIdentifier_Lock_BreakLock = 18832;

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ControllerType_Software_SoftwareIdentifier_Lock_InitLock = 18859;

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ControllerType_Software_SoftwareIdentifier_Lock_RenewLock = 18862;

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ControllerType_Software_SoftwareIdentifier_Lock_ExitLock = 18864;

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ControllerType_Software_SoftwareIdentifier_Lock_BreakLock = 18866;

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ControllerType_TaskControls_TaskControlIdentifier_Lock_InitLock = 18893;

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ControllerType_TaskControls_TaskControlIdentifier_Lock_RenewLock = 18896;

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ControllerType_TaskControls_TaskControlIdentifier_Lock_ExitLock = 18898;

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ControllerType_TaskControls_TaskControlIdentifier_Lock_BreakLock = 18900;

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ControllerType_SafetyStatesIdentifier_Lock_InitLock = 18930;

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ControllerType_SafetyStatesIdentifier_Lock_RenewLock = 18933;

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ControllerType_SafetyStatesIdentifier_Lock_ExitLock = 18935;

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ControllerType_SafetyStatesIdentifier_Lock_BreakLock = 18937;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Lock_InitLock = 18976;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Lock_RenewLock = 18979;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Lock_ExitLock = 18981;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Lock_BreakLock = 18983;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock = 19015;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RenewLock = 19018;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_ExitLock = 19020;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_BreakLock = 19022;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock = 19093;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RenewLock = 19096;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_ExitLock = 19098;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_BreakLock = 19100;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock = 19232;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock = 19235;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock = 19237;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock = 19239;

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_Lock_InitLock Method.
        /// </summary>
        public const uint AuxiliaryComponentType_Lock_InitLock = 17739;

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_Lock_RenewLock Method.
        /// </summary>
        public const uint AuxiliaryComponentType_Lock_RenewLock = 17742;

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_Lock_ExitLock Method.
        /// </summary>
        public const uint AuxiliaryComponentType_Lock_ExitLock = 17744;

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_Lock_BreakLock Method.
        /// </summary>
        public const uint AuxiliaryComponentType_Lock_BreakLock = 17746;

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint AuxiliaryComponentType_CPIdentifier_Lock_InitLock = 17773;

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint AuxiliaryComponentType_CPIdentifier_Lock_RenewLock = 17776;

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint AuxiliaryComponentType_CPIdentifier_Lock_ExitLock = 17778;

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint AuxiliaryComponentType_CPIdentifier_Lock_BreakLock = 17780;

        /// <summary>
        /// The identifier for the DriveType_Lock_InitLock Method.
        /// </summary>
        public const uint DriveType_Lock_InitLock = 17807;

        /// <summary>
        /// The identifier for the DriveType_Lock_RenewLock Method.
        /// </summary>
        public const uint DriveType_Lock_RenewLock = 17810;

        /// <summary>
        /// The identifier for the DriveType_Lock_ExitLock Method.
        /// </summary>
        public const uint DriveType_Lock_ExitLock = 17812;

        /// <summary>
        /// The identifier for the DriveType_Lock_BreakLock Method.
        /// </summary>
        public const uint DriveType_Lock_BreakLock = 17814;

        /// <summary>
        /// The identifier for the DriveType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint DriveType_CPIdentifier_Lock_InitLock = 17841;

        /// <summary>
        /// The identifier for the DriveType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint DriveType_CPIdentifier_Lock_RenewLock = 17844;

        /// <summary>
        /// The identifier for the DriveType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint DriveType_CPIdentifier_Lock_ExitLock = 17846;

        /// <summary>
        /// The identifier for the DriveType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint DriveType_CPIdentifier_Lock_BreakLock = 17848;

        /// <summary>
        /// The identifier for the TaskControlType_Lock_InitLock Method.
        /// </summary>
        public const uint TaskControlType_Lock_InitLock = 15896;

        /// <summary>
        /// The identifier for the TaskControlType_Lock_RenewLock Method.
        /// </summary>
        public const uint TaskControlType_Lock_RenewLock = 15899;

        /// <summary>
        /// The identifier for the TaskControlType_Lock_ExitLock Method.
        /// </summary>
        public const uint TaskControlType_Lock_ExitLock = 15901;

        /// <summary>
        /// The identifier for the TaskControlType_Lock_BreakLock Method.
        /// </summary>
        public const uint TaskControlType_Lock_BreakLock = 15903;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Lock_InitLock = 19266;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Lock_RenewLock = 19269;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Lock_ExitLock = 19271;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Lock_BreakLock = 19273;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock = 19305;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RenewLock = 19308;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_ExitLock = 19310;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_BreakLock = 19312;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock = 19383;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RenewLock = 19386;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_ExitLock = 19388;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_BreakLock = 19390;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock = 19522;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock = 19525;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock = 19527;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock = 19529;
    }
    #endregion

    #region Object Identifiers
    /// <summary>
    /// A class that declares constants for all Objects in the Model Design.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class Objects
    {
        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata Object.
        /// </summary>
        public const uint OPCUAROBOTICSNamespaceMetadata = 15011;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices Object.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices = 5002;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier Object.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier = 15008;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_ParameterSet Object.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_ParameterSet = 15024;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes Object.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes = 15062;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet Object.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet = 15065;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains Object.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains = 15208;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers Object.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers = 5001;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier Object.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier = 15405;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_CurrentUser Object.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_CurrentUser = 15440;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software Object.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Software = 15483;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls Object.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls = 15518;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_ParameterSet Object.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_ParameterSet = 15520;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates Object.
        /// </summary>
        public const uint MotionDeviceSystemType_SafetyStates = 5010;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier Object.
        /// </summary>
        public const uint MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier = 15697;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ParameterSet Object.
        /// </summary>
        public const uint MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ParameterSet = 15698;

        /// <summary>
        /// The identifier for the MotionDeviceType_ParameterSet Object.
        /// </summary>
        public const uint MotionDeviceType_ParameterSet = 5029;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes Object.
        /// </summary>
        public const uint MotionDeviceType_Axes = 15305;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier Object.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier = 15743;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_ParameterSet Object.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_ParameterSet = 15744;

        /// <summary>
        /// The identifier for the MotionDeviceType_PowerTrains Object.
        /// </summary>
        public const uint MotionDeviceType_PowerTrains = 16443;

        /// <summary>
        /// The identifier for the MotionDeviceType_PowerTrains_PowerTrainIdentifier Object.
        /// </summary>
        public const uint MotionDeviceType_PowerTrains_PowerTrainIdentifier = 15905;

        /// <summary>
        /// The identifier for the MotionDeviceType_FlangeLoad Object.
        /// </summary>
        public const uint MotionDeviceType_FlangeLoad = 5091;

        /// <summary>
        /// The identifier for the MotionDeviceType_AdditionalComponents Object.
        /// </summary>
        public const uint MotionDeviceType_AdditionalComponents = 16566;

        /// <summary>
        /// The identifier for the MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier Object.
        /// </summary>
        public const uint MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier = 18310;

        /// <summary>
        /// The identifier for the AxisType_ParameterSet Object.
        /// </summary>
        public const uint AxisType_ParameterSet = 16602;

        /// <summary>
        /// The identifier for the AxisType_AdditionalLoad Object.
        /// </summary>
        public const uint AxisType_AdditionalLoad = 16638;

        /// <summary>
        /// The identifier for the AxisType_PowerTrainIdentifier Object.
        /// </summary>
        public const uint AxisType_PowerTrainIdentifier = 18344;

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier Object.
        /// </summary>
        public const uint PowerTrainType_MotorIdentifier = 15998;

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_ParameterSet Object.
        /// </summary>
        public const uint PowerTrainType_MotorIdentifier_ParameterSet = 15999;

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier Object.
        /// </summary>
        public const uint PowerTrainType_GearIdentifier = 16041;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier Object.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier = 18536;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_ParameterSet Object.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_ParameterSet = 18537;

        /// <summary>
        /// The identifier for the PowerTrainType_PowerTrainIdentifier Object.
        /// </summary>
        public const uint PowerTrainType_PowerTrainIdentifier = 18613;

        /// <summary>
        /// The identifier for the MotorType_ParameterSet Object.
        /// </summary>
        public const uint MotorType_ParameterSet = 5105;

        /// <summary>
        /// The identifier for the MotorType_DriveIdentifier Object.
        /// </summary>
        public const uint MotorType_DriveIdentifier = 18763;

        /// <summary>
        /// The identifier for the SafetyStateType_ParameterSet Object.
        /// </summary>
        public const uint SafetyStateType_ParameterSet = 5016;

        /// <summary>
        /// The identifier for the SafetyStateType_EmergencyStopFunctions Object.
        /// </summary>
        public const uint SafetyStateType_EmergencyStopFunctions = 17221;

        /// <summary>
        /// The identifier for the SafetyStateType_EmergencyStopFunctions_EmergencyStopFunctionIdentifier Object.
        /// </summary>
        public const uint SafetyStateType_EmergencyStopFunctions_EmergencyStopFunctionIdentifier = 18806;

        /// <summary>
        /// The identifier for the SafetyStateType_ProtectiveStopFunctions Object.
        /// </summary>
        public const uint SafetyStateType_ProtectiveStopFunctions = 17225;

        /// <summary>
        /// The identifier for the SafetyStateType_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier Object.
        /// </summary>
        public const uint SafetyStateType_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier = 18809;

        /// <summary>
        /// The identifier for the ControllerType_ParameterSet Object.
        /// </summary>
        public const uint ControllerType_ParameterSet = 5004;

        /// <summary>
        /// The identifier for the ControllerType_CurrentUser Object.
        /// </summary>
        public const uint ControllerType_CurrentUser = 17249;

        /// <summary>
        /// The identifier for the ControllerType_Components Object.
        /// </summary>
        public const uint ControllerType_Components = 17252;

        /// <summary>
        /// The identifier for the ControllerType_Components_ComponentIdentifier Object.
        /// </summary>
        public const uint ControllerType_Components_ComponentIdentifier = 18813;

        /// <summary>
        /// The identifier for the ControllerType_Software Object.
        /// </summary>
        public const uint ControllerType_Software = 15800;

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier Object.
        /// </summary>
        public const uint ControllerType_Software_SoftwareIdentifier = 18847;

        /// <summary>
        /// The identifier for the ControllerType_TaskControls Object.
        /// </summary>
        public const uint ControllerType_TaskControls = 15826;

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier Object.
        /// </summary>
        public const uint ControllerType_TaskControls_TaskControlIdentifier = 18881;

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_ParameterSet Object.
        /// </summary>
        public const uint ControllerType_TaskControls_TaskControlIdentifier_ParameterSet = 18882;

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier Object.
        /// </summary>
        public const uint ControllerType_SafetyStatesIdentifier = 18918;

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_ParameterSet Object.
        /// </summary>
        public const uint ControllerType_SafetyStatesIdentifier_ParameterSet = 18919;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier Object.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier = 18964;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_ParameterSet Object.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_ParameterSet = 18965;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes Object.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes = 19002;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet Object.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet = 19004;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_PowerTrains Object.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_PowerTrains = 19080;

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public const uint AuxiliaryComponentType_CPIdentifier_NetworkAddress = 17782;

        /// <summary>
        /// The identifier for the DriveType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public const uint DriveType_CPIdentifier_NetworkAddress = 17850;

        /// <summary>
        /// The identifier for the TaskControlType_ParameterSet Object.
        /// </summary>
        public const uint TaskControlType_ParameterSet = 15883;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier Object.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier = 19254;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_ParameterSet Object.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_ParameterSet = 19255;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes Object.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes = 19292;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet Object.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet = 19294;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_PowerTrains Object.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_PowerTrains = 19370;
    }
    #endregion

    #region ObjectType Identifiers
    /// <summary>
    /// A class that declares constants for all ObjectTypes in the Model Design.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class ObjectTypes
    {
        /// <summary>
        /// The identifier for the MotionDeviceSystemType ObjectType.
        /// </summary>
        public const uint MotionDeviceSystemType = 1002;

        /// <summary>
        /// The identifier for the MotionDeviceType ObjectType.
        /// </summary>
        public const uint MotionDeviceType = 1004;

        /// <summary>
        /// The identifier for the AxisType ObjectType.
        /// </summary>
        public const uint AxisType = 16601;

        /// <summary>
        /// The identifier for the PowerTrainType ObjectType.
        /// </summary>
        public const uint PowerTrainType = 16794;

        /// <summary>
        /// The identifier for the MotorType ObjectType.
        /// </summary>
        public const uint MotorType = 1019;

        /// <summary>
        /// The identifier for the GearType ObjectType.
        /// </summary>
        public const uint GearType = 1022;

        /// <summary>
        /// The identifier for the SafetyStateType ObjectType.
        /// </summary>
        public const uint SafetyStateType = 1013;

        /// <summary>
        /// The identifier for the EmergencyStopFunctionType ObjectType.
        /// </summary>
        public const uint EmergencyStopFunctionType = 17230;

        /// <summary>
        /// The identifier for the ProtectiveStopFunctionType ObjectType.
        /// </summary>
        public const uint ProtectiveStopFunctionType = 17233;

        /// <summary>
        /// The identifier for the ControllerType ObjectType.
        /// </summary>
        public const uint ControllerType = 1003;

        /// <summary>
        /// The identifier for the AuxiliaryComponentType ObjectType.
        /// </summary>
        public const uint AuxiliaryComponentType = 17725;

        /// <summary>
        /// The identifier for the DriveType ObjectType.
        /// </summary>
        public const uint DriveType = 17793;

        /// <summary>
        /// The identifier for the TaskControlType ObjectType.
        /// </summary>
        public const uint TaskControlType = 1011;

        /// <summary>
        /// The identifier for the LoadType ObjectType.
        /// </summary>
        public const uint LoadType = 1018;

        /// <summary>
        /// The identifier for the UserType ObjectType.
        /// </summary>
        public const uint UserType = 18175;
    }
    #endregion

    #region ReferenceType Identifiers
    /// <summary>
    /// A class that declares constants for all ReferenceTypes in the Model Design.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class ReferenceTypes
    {
        /// <summary>
        /// The identifier for the Controls ReferenceType.
        /// </summary>
        public const uint Controls = 4002;

        /// <summary>
        /// The identifier for the Moves ReferenceType.
        /// </summary>
        public const uint Moves = 18178;

        /// <summary>
        /// The identifier for the Requires ReferenceType.
        /// </summary>
        public const uint Requires = 18179;

        /// <summary>
        /// The identifier for the IsDrivenBy ReferenceType.
        /// </summary>
        public const uint IsDrivenBy = 18180;

        /// <summary>
        /// The identifier for the IsConnectedTo ReferenceType.
        /// </summary>
        public const uint IsConnectedTo = 18181;

        /// <summary>
        /// The identifier for the HasSafetyStates ReferenceType.
        /// </summary>
        public const uint HasSafetyStates = 18182;

        /// <summary>
        /// The identifier for the HasSlave ReferenceType.
        /// </summary>
        public const uint HasSlave = 18183;
    }
    #endregion

    #region Variable Identifiers
    /// <summary>
    /// A class that declares constants for all Variables in the Model Design.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class Variables
    {
        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceUri Variable.
        /// </summary>
        public const uint OPCUAROBOTICSNamespaceMetadata_NamespaceUri = 15034;

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceVersion Variable.
        /// </summary>
        public const uint OPCUAROBOTICSNamespaceMetadata_NamespaceVersion = 15064;

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespacePublicationDate Variable.
        /// </summary>
        public const uint OPCUAROBOTICSNamespaceMetadata_NamespacePublicationDate = 15091;

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_IsNamespaceSubset Variable.
        /// </summary>
        public const uint OPCUAROBOTICSNamespaceMetadata_IsNamespaceSubset = 15114;

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_StaticNodeIdTypes Variable.
        /// </summary>
        public const uint OPCUAROBOTICSNamespaceMetadata_StaticNodeIdTypes = 15145;

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_StaticNumericNodeIdRange Variable.
        /// </summary>
        public const uint OPCUAROBOTICSNamespaceMetadata_StaticNumericNodeIdRange = 15173;

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_StaticStringNodeIdPattern Variable.
        /// </summary>
        public const uint OPCUAROBOTICSNamespaceMetadata_StaticStringNodeIdPattern = 15209;

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Size Variable.
        /// </summary>
        public const uint OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Size = 15314;

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Writable Variable.
        /// </summary>
        public const uint OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Writable = 15344;

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_UserWritable Variable.
        /// </summary>
        public const uint OPCUAROBOTICSNamespaceMetadata_NamespaceFile_UserWritable = 15390;

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_OpenCount Variable.
        /// </summary>
        public const uint OPCUAROBOTICSNamespaceMetadata_NamespaceFile_OpenCount = 15439;

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Open_InputArguments Variable.
        /// </summary>
        public const uint OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Open_InputArguments = 15524;

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Open_OutputArguments Variable.
        /// </summary>
        public const uint OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Open_OutputArguments = 15549;

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Close_InputArguments Variable.
        /// </summary>
        public const uint OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Close_InputArguments = 15551;

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Read_InputArguments Variable.
        /// </summary>
        public const uint OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Read_InputArguments = 15559;

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Read_OutputArguments Variable.
        /// </summary>
        public const uint OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Read_OutputArguments = 15562;

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Write_InputArguments Variable.
        /// </summary>
        public const uint OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Write_InputArguments = 15576;

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_GetPosition_InputArguments Variable.
        /// </summary>
        public const uint OPCUAROBOTICSNamespaceMetadata_NamespaceFile_GetPosition_InputArguments = 15578;

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_GetPosition_OutputArguments Variable.
        /// </summary>
        public const uint OPCUAROBOTICSNamespaceMetadata_NamespaceFile_GetPosition_OutputArguments = 15579;

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_SetPosition_InputArguments Variable.
        /// </summary>
        public const uint OPCUAROBOTICSNamespaceMetadata_NamespaceFile_SetPosition_InputArguments = 15581;

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_DefaultRolePermissions Variable.
        /// </summary>
        public const uint OPCUAROBOTICSNamespaceMetadata_DefaultRolePermissions = 15583;

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_DefaultUserRolePermissions Variable.
        /// </summary>
        public const uint OPCUAROBOTICSNamespaceMetadata_DefaultUserRolePermissions = 15584;

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_DefaultAccessRestrictions Variable.
        /// </summary>
        public const uint OPCUAROBOTICSNamespaceMetadata_DefaultAccessRestrictions = 15585;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Lock_Locked Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Lock_Locked = 15621;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Lock_LockingClient Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Lock_LockingClient = 15012;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Lock_LockingUser Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Lock_LockingUser = 15013;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Lock_RemainingLockTime = 15014;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Lock_InitLock_InputArguments = 15016;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Lock_InitLock_OutputArguments = 15017;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Lock_RenewLock_OutputArguments = 15019;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Lock_ExitLock_OutputArguments = 15021;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Lock_BreakLock_OutputArguments = 15023;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_Locked = 15623;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_LockingClient = 15032;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_LockingUser = 15033;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_RemainingLockTime = 15035;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_InitLock_InputArguments = 15037;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_InitLock_OutputArguments = 15038;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_RenewLock_OutputArguments = 15040;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_ExitLock_OutputArguments = 15042;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_BreakLock_OutputArguments = 15044;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Manufacturer Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Manufacturer = 15045;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Model Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Model = 15047;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_ProductCode Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_ProductCode = 15053;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_SerialNumber Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_SerialNumber = 15048;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_MotionDeviceCategory Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_MotionDeviceCategory = 15058;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_ParameterSet_SpeedOverride Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_ParameterSet_SpeedOverride = 15061;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_Locked = 15625;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_LockingClient = 15073;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_LockingUser = 15074;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RemainingLockTime = 15075;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock_InputArguments = 15077;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock_OutputArguments = 15078;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RenewLock_OutputArguments = 15080;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_ExitLock_OutputArguments = 15082;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_BreakLock_OutputArguments = 15084;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_MotionProfile Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_MotionProfile = 15099;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Mass Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Mass = 15101;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Mass_EngineeringUnits Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Mass_EngineeringUnits = 15127;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates = 15031;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation = 15224;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_X Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_X = 15313;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Y Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Y = 15378;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Z Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Z = 15413;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_A Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_A = 15452;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_B Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_B = 15453;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_C Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_C = 15459;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_X Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_X = 15161;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_Y Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_Y = 15162;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_Z Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_Z = 15163;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualPosition Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualPosition = 15186;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualPosition_EngineeringUnits Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualPosition_EngineeringUnits = 15191;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualSpeed_EngineeringUnits Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualSpeed_EngineeringUnits = 15201;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualAcceleration_EngineeringUnits Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualAcceleration_EngineeringUnits = 15207;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_Locked = 15627;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_LockingClient = 15225;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_LockingUser = 15226;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RemainingLockTime = 15227;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock_InputArguments = 15231;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock_OutputArguments = 15234;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RenewLock_OutputArguments = 15241;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_ExitLock_OutputArguments = 15243;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_BreakLock_OutputArguments = 15245;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_Mass Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_Mass = 15343;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_Mass_EngineeringUnits Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_Mass_EngineeringUnits = 15349;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates = 15461;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation = 15528;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_X Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_X = 15541;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_Y Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_Y = 15547;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_Z Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_Z = 15548;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_A Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_A = 15705;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_B Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_B = 15751;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_C Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_C = 15967;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_Inertia_X Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_Inertia_X = 15363;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_Inertia_Y Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_Inertia_Y = 15364;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_Inertia_Z Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_Inertia_Z = 15367;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_Locked = 15633;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingClient = 15379;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingUser = 15380;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RemainingLockTime = 15381;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_InputArguments = 15383;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_OutputArguments = 15384;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock_OutputArguments = 15386;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock_OutputArguments = 15388;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock_OutputArguments = 15391;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_Locked = 15635;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_LockingClient = 15414;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_LockingUser = 15415;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_RemainingLockTime = 15416;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_InitLock_InputArguments = 15418;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_InitLock_OutputArguments = 15419;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_RenewLock_OutputArguments = 15421;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_ExitLock_OutputArguments = 15423;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_BreakLock_OutputArguments = 15425;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Manufacturer Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Manufacturer = 15426;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Model Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Model = 15428;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_ProductCode Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_ProductCode = 15434;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_SerialNumber Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_SerialNumber = 15429;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_CurrentUser_Level Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_CurrentUser_Level = 15441;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_Locked = 15637;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_LockingClient = 15454;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_LockingUser = 15455;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_RemainingLockTime = 15456;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_InitLock_InputArguments = 15458;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_InitLock_OutputArguments = 15460;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_RenewLock_OutputArguments = 15463;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_ExitLock_OutputArguments = 15465;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_BreakLock_OutputArguments = 15467;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_Locked = 15639;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_LockingClient = 15493;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_LockingUser = 15494;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_RemainingLockTime = 15495;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_InitLock_InputArguments = 15497;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_InitLock_OutputArguments = 15498;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_RenewLock_OutputArguments = 15500;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_ExitLock_OutputArguments = 15502;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_BreakLock_OutputArguments = 15504;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Manufacturer Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Manufacturer = 15505;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Model Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Model = 15507;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_SoftwareRevision Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_SoftwareRevision = 15510;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_Locked = 15667;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_LockingClient = 15529;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_LockingUser = 15530;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_RemainingLockTime = 15531;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_InitLock_InputArguments = 15533;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_InitLock_OutputArguments = 15534;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_RenewLock_OutputArguments = 15536;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_ExitLock_OutputArguments = 15538;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_BreakLock_OutputArguments = 15542;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_ComponentName Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_ComponentName = 15644;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_ParameterSet_TaskProgramName Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_ParameterSet_TaskProgramName = 15654;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_ParameterSet_TaskProgramLoaded Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_ParameterSet_TaskProgramLoaded = 15660;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_ParameterSet_TotalEnergyConsumption_EngineeringUnits Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_ParameterSet_TotalEnergyConsumption_EngineeringUnits = 15671;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_ParameterSet_CabinetFanSpeed_EngineeringUnits Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_ParameterSet_CabinetFanSpeed_EngineeringUnits = 15677;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_ParameterSet_CPUFanSpeed_EngineeringUnits Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_ParameterSet_CPUFanSpeed_EngineeringUnits = 15683;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_ParameterSet_InputVoltage_EngineeringUnits Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_ParameterSet_InputVoltage_EngineeringUnits = 15689;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_ParameterSet_Temperature_EngineeringUnits Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_Controllers_ControllerIdentifier_ParameterSet_Temperature_EngineeringUnits = 15695;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_Locked = 15752;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_LockingClient = 15706;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_LockingUser = 15707;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_RemainingLockTime = 15708;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_InitLock_InputArguments = 15710;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_InitLock_OutputArguments = 15711;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_RenewLock_OutputArguments = 15713;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_ExitLock_OutputArguments = 15715;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_BreakLock_OutputArguments = 15717;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_EmergencyStopFunctions_EmergencyStopFunctionIdentifier_Name Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_EmergencyStopFunctions_EmergencyStopFunctionIdentifier_Name = 15733;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_EmergencyStopFunctions_EmergencyStopFunctionIdentifier_Active Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_EmergencyStopFunctions_EmergencyStopFunctionIdentifier_Active = 15734;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Name Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Name = 15737;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Enabled Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Enabled = 15738;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Active Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Active = 15739;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ParameterSet_OperationalMode Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ParameterSet_OperationalMode = 15740;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ParameterSet_EmergencyStop Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ParameterSet_EmergencyStop = 15741;

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ParameterSet_ProtectiveStop Variable.
        /// </summary>
        public const uint MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ParameterSet_ProtectiveStop = 15742;

        /// <summary>
        /// The identifier for the MotionDeviceType_Lock_Locked Variable.
        /// </summary>
        public const uint MotionDeviceType_Lock_Locked = 15771;

        /// <summary>
        /// The identifier for the MotionDeviceType_Lock_LockingClient Variable.
        /// </summary>
        public const uint MotionDeviceType_Lock_LockingClient = 15115;

        /// <summary>
        /// The identifier for the MotionDeviceType_Lock_LockingUser Variable.
        /// </summary>
        public const uint MotionDeviceType_Lock_LockingUser = 15116;

        /// <summary>
        /// The identifier for the MotionDeviceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint MotionDeviceType_Lock_RemainingLockTime = 15117;

        /// <summary>
        /// The identifier for the MotionDeviceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint MotionDeviceType_Lock_InitLock_InputArguments = 15119;

        /// <summary>
        /// The identifier for the MotionDeviceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceType_Lock_InitLock_OutputArguments = 15120;

        /// <summary>
        /// The identifier for the MotionDeviceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceType_Lock_RenewLock_OutputArguments = 15122;

        /// <summary>
        /// The identifier for the MotionDeviceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceType_Lock_ExitLock_OutputArguments = 15124;

        /// <summary>
        /// The identifier for the MotionDeviceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceType_Lock_BreakLock_OutputArguments = 15126;

        /// <summary>
        /// The identifier for the MotionDeviceType_Manufacturer Variable.
        /// </summary>
        public const uint MotionDeviceType_Manufacturer = 16351;

        /// <summary>
        /// The identifier for the MotionDeviceType_Model Variable.
        /// </summary>
        public const uint MotionDeviceType_Model = 16353;

        /// <summary>
        /// The identifier for the MotionDeviceType_ProductCode Variable.
        /// </summary>
        public const uint MotionDeviceType_ProductCode = 16359;

        /// <summary>
        /// The identifier for the MotionDeviceType_SerialNumber Variable.
        /// </summary>
        public const uint MotionDeviceType_SerialNumber = 16354;

        /// <summary>
        /// The identifier for the MotionDeviceType_MotionDeviceCategory Variable.
        /// </summary>
        public const uint MotionDeviceType_MotionDeviceCategory = 16362;

        /// <summary>
        /// The identifier for the MotionDeviceType_ParameterSet_OnPath Variable.
        /// </summary>
        public const uint MotionDeviceType_ParameterSet_OnPath = 16363;

        /// <summary>
        /// The identifier for the MotionDeviceType_ParameterSet_InControl Variable.
        /// </summary>
        public const uint MotionDeviceType_ParameterSet_InControl = 16364;

        /// <summary>
        /// The identifier for the MotionDeviceType_ParameterSet_SpeedOverride Variable.
        /// </summary>
        public const uint MotionDeviceType_ParameterSet_SpeedOverride = 16365;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_Lock_Locked = 15773;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_Lock_LockingClient = 15753;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_Lock_LockingUser = 15754;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_Lock_RemainingLockTime = 15755;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_Lock_InitLock_InputArguments = 15757;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_Lock_InitLock_OutputArguments = 15758;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_Lock_RenewLock_OutputArguments = 15760;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_Lock_ExitLock_OutputArguments = 15762;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_Lock_BreakLock_OutputArguments = 15764;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_MotionProfile Variable.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_MotionProfile = 15808;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_Mass Variable.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_Mass = 15811;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_Mass_EngineeringUnits Variable.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_Mass_EngineeringUnits = 15816;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates Variable.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates = 15968;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation Variable.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation = 15970;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_X Variable.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_X = 15972;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Y Variable.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Y = 15973;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Z Variable.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Z = 15974;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_A Variable.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_A = 15975;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_B Variable.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_B = 15976;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_C Variable.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_C = 15977;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_Inertia_X Variable.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_Inertia_X = 15859;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_Inertia_Y Variable.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_Inertia_Y = 15860;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_Inertia_Z Variable.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_Inertia_Z = 15861;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_ParameterSet_ActualPosition Variable.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_ParameterSet_ActualPosition = 15863;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_ParameterSet_ActualPosition_EngineeringUnits Variable.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_ParameterSet_ActualPosition_EngineeringUnits = 15869;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_ParameterSet_ActualSpeed_EngineeringUnits Variable.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_ParameterSet_ActualSpeed_EngineeringUnits = 15875;

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_ParameterSet_ActualAcceleration_EngineeringUnits Variable.
        /// </summary>
        public const uint MotionDeviceType_Axes_AxisIdentifier_ParameterSet_ActualAcceleration_EngineeringUnits = 15881;

        /// <summary>
        /// The identifier for the MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_Locked = 15775;

        /// <summary>
        /// The identifier for the MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_LockingClient = 18205;

        /// <summary>
        /// The identifier for the MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_LockingUser = 18206;

        /// <summary>
        /// The identifier for the MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_RemainingLockTime = 18207;

        /// <summary>
        /// The identifier for the MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_InitLock_InputArguments = 18209;

        /// <summary>
        /// The identifier for the MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_InitLock_OutputArguments = 18210;

        /// <summary>
        /// The identifier for the MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_RenewLock_OutputArguments = 18212;

        /// <summary>
        /// The identifier for the MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_ExitLock_OutputArguments = 18214;

        /// <summary>
        /// The identifier for the MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_BreakLock_OutputArguments = 18216;

        /// <summary>
        /// The identifier for the MotionDeviceType_FlangeLoad_Mass Variable.
        /// </summary>
        public const uint MotionDeviceType_FlangeLoad_Mass = 6624;

        /// <summary>
        /// The identifier for the MotionDeviceType_FlangeLoad_Mass_EngineeringUnits Variable.
        /// </summary>
        public const uint MotionDeviceType_FlangeLoad_Mass_EngineeringUnits = 15659;

        /// <summary>
        /// The identifier for the MotionDeviceType_FlangeLoad_CenterOfMass_CartesianCoordinates Variable.
        /// </summary>
        public const uint MotionDeviceType_FlangeLoad_CenterOfMass_CartesianCoordinates = 15978;

        /// <summary>
        /// The identifier for the MotionDeviceType_FlangeLoad_CenterOfMass_Orientation Variable.
        /// </summary>
        public const uint MotionDeviceType_FlangeLoad_CenterOfMass_Orientation = 15980;

        /// <summary>
        /// The identifier for the MotionDeviceType_FlangeLoad_CenterOfMass_CartesianCoordinates_X Variable.
        /// </summary>
        public const uint MotionDeviceType_FlangeLoad_CenterOfMass_CartesianCoordinates_X = 15982;

        /// <summary>
        /// The identifier for the MotionDeviceType_FlangeLoad_CenterOfMass_CartesianCoordinates_Y Variable.
        /// </summary>
        public const uint MotionDeviceType_FlangeLoad_CenterOfMass_CartesianCoordinates_Y = 15983;

        /// <summary>
        /// The identifier for the MotionDeviceType_FlangeLoad_CenterOfMass_CartesianCoordinates_Z Variable.
        /// </summary>
        public const uint MotionDeviceType_FlangeLoad_CenterOfMass_CartesianCoordinates_Z = 15984;

        /// <summary>
        /// The identifier for the MotionDeviceType_FlangeLoad_CenterOfMass_Orientation_A Variable.
        /// </summary>
        public const uint MotionDeviceType_FlangeLoad_CenterOfMass_Orientation_A = 15985;

        /// <summary>
        /// The identifier for the MotionDeviceType_FlangeLoad_CenterOfMass_Orientation_B Variable.
        /// </summary>
        public const uint MotionDeviceType_FlangeLoad_CenterOfMass_Orientation_B = 15986;

        /// <summary>
        /// The identifier for the MotionDeviceType_FlangeLoad_CenterOfMass_Orientation_C Variable.
        /// </summary>
        public const uint MotionDeviceType_FlangeLoad_CenterOfMass_Orientation_C = 15987;

        /// <summary>
        /// The identifier for the MotionDeviceType_FlangeLoad_Inertia_X Variable.
        /// </summary>
        public const uint MotionDeviceType_FlangeLoad_Inertia_X = 16562;

        /// <summary>
        /// The identifier for the MotionDeviceType_FlangeLoad_Inertia_Y Variable.
        /// </summary>
        public const uint MotionDeviceType_FlangeLoad_Inertia_Y = 16563;

        /// <summary>
        /// The identifier for the MotionDeviceType_FlangeLoad_Inertia_Z Variable.
        /// </summary>
        public const uint MotionDeviceType_FlangeLoad_Inertia_Z = 16564;

        /// <summary>
        /// The identifier for the MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_Locked = 15781;

        /// <summary>
        /// The identifier for the MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingClient = 18319;

        /// <summary>
        /// The identifier for the MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingUser = 18320;

        /// <summary>
        /// The identifier for the MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_RemainingLockTime = 18321;

        /// <summary>
        /// The identifier for the MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_InputArguments = 18323;

        /// <summary>
        /// The identifier for the MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_OutputArguments = 18324;

        /// <summary>
        /// The identifier for the MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock_OutputArguments = 18326;

        /// <summary>
        /// The identifier for the MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock_OutputArguments = 18328;

        /// <summary>
        /// The identifier for the MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock_OutputArguments = 18330;

        /// <summary>
        /// The identifier for the AxisType_Lock_Locked Variable.
        /// </summary>
        public const uint AxisType_Lock_Locked = 15783;

        /// <summary>
        /// The identifier for the AxisType_Lock_LockingClient Variable.
        /// </summary>
        public const uint AxisType_Lock_LockingClient = 16612;

        /// <summary>
        /// The identifier for the AxisType_Lock_LockingUser Variable.
        /// </summary>
        public const uint AxisType_Lock_LockingUser = 16613;

        /// <summary>
        /// The identifier for the AxisType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint AxisType_Lock_RemainingLockTime = 16614;

        /// <summary>
        /// The identifier for the AxisType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint AxisType_Lock_InitLock_InputArguments = 16616;

        /// <summary>
        /// The identifier for the AxisType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint AxisType_Lock_InitLock_OutputArguments = 16617;

        /// <summary>
        /// The identifier for the AxisType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint AxisType_Lock_RenewLock_OutputArguments = 16619;

        /// <summary>
        /// The identifier for the AxisType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint AxisType_Lock_ExitLock_OutputArguments = 16621;

        /// <summary>
        /// The identifier for the AxisType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint AxisType_Lock_BreakLock_OutputArguments = 16623;

        /// <summary>
        /// The identifier for the AxisType_MotionProfile Variable.
        /// </summary>
        public const uint AxisType_MotionProfile = 16637;

        /// <summary>
        /// The identifier for the AxisType_AdditionalLoad_Mass Variable.
        /// </summary>
        public const uint AxisType_AdditionalLoad_Mass = 16639;

        /// <summary>
        /// The identifier for the AxisType_AdditionalLoad_Mass_EngineeringUnits Variable.
        /// </summary>
        public const uint AxisType_AdditionalLoad_Mass_EngineeringUnits = 16644;

        /// <summary>
        /// The identifier for the AxisType_AdditionalLoad_CenterOfMass_CartesianCoordinates Variable.
        /// </summary>
        public const uint AxisType_AdditionalLoad_CenterOfMass_CartesianCoordinates = 15988;

        /// <summary>
        /// The identifier for the AxisType_AdditionalLoad_CenterOfMass_Orientation Variable.
        /// </summary>
        public const uint AxisType_AdditionalLoad_CenterOfMass_Orientation = 15990;

        /// <summary>
        /// The identifier for the AxisType_AdditionalLoad_CenterOfMass_CartesianCoordinates_X Variable.
        /// </summary>
        public const uint AxisType_AdditionalLoad_CenterOfMass_CartesianCoordinates_X = 15992;

        /// <summary>
        /// The identifier for the AxisType_AdditionalLoad_CenterOfMass_CartesianCoordinates_Y Variable.
        /// </summary>
        public const uint AxisType_AdditionalLoad_CenterOfMass_CartesianCoordinates_Y = 15993;

        /// <summary>
        /// The identifier for the AxisType_AdditionalLoad_CenterOfMass_CartesianCoordinates_Z Variable.
        /// </summary>
        public const uint AxisType_AdditionalLoad_CenterOfMass_CartesianCoordinates_Z = 15994;

        /// <summary>
        /// The identifier for the AxisType_AdditionalLoad_CenterOfMass_Orientation_A Variable.
        /// </summary>
        public const uint AxisType_AdditionalLoad_CenterOfMass_Orientation_A = 15995;

        /// <summary>
        /// The identifier for the AxisType_AdditionalLoad_CenterOfMass_Orientation_B Variable.
        /// </summary>
        public const uint AxisType_AdditionalLoad_CenterOfMass_Orientation_B = 15996;

        /// <summary>
        /// The identifier for the AxisType_AdditionalLoad_CenterOfMass_Orientation_C Variable.
        /// </summary>
        public const uint AxisType_AdditionalLoad_CenterOfMass_Orientation_C = 15997;

        /// <summary>
        /// The identifier for the AxisType_AdditionalLoad_Inertia_X Variable.
        /// </summary>
        public const uint AxisType_AdditionalLoad_Inertia_X = 16658;

        /// <summary>
        /// The identifier for the AxisType_AdditionalLoad_Inertia_Y Variable.
        /// </summary>
        public const uint AxisType_AdditionalLoad_Inertia_Y = 16659;

        /// <summary>
        /// The identifier for the AxisType_AdditionalLoad_Inertia_Z Variable.
        /// </summary>
        public const uint AxisType_AdditionalLoad_Inertia_Z = 16660;

        /// <summary>
        /// The identifier for the AxisType_ParameterSet_ActualPosition Variable.
        /// </summary>
        public const uint AxisType_ParameterSet_ActualPosition = 16662;

        /// <summary>
        /// The identifier for the AxisType_ParameterSet_ActualPosition_EngineeringUnits Variable.
        /// </summary>
        public const uint AxisType_ParameterSet_ActualPosition_EngineeringUnits = 16667;

        /// <summary>
        /// The identifier for the AxisType_ParameterSet_ActualSpeed Variable.
        /// </summary>
        public const uint AxisType_ParameterSet_ActualSpeed = 16668;

        /// <summary>
        /// The identifier for the AxisType_ParameterSet_ActualSpeed_EngineeringUnits Variable.
        /// </summary>
        public const uint AxisType_ParameterSet_ActualSpeed_EngineeringUnits = 16673;

        /// <summary>
        /// The identifier for the AxisType_ParameterSet_ActualAcceleration Variable.
        /// </summary>
        public const uint AxisType_ParameterSet_ActualAcceleration = 16674;

        /// <summary>
        /// The identifier for the AxisType_ParameterSet_ActualAcceleration_EngineeringUnits Variable.
        /// </summary>
        public const uint AxisType_ParameterSet_ActualAcceleration_EngineeringUnits = 16679;

        /// <summary>
        /// The identifier for the AxisType_PowerTrainIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint AxisType_PowerTrainIdentifier_Lock_Locked = 15785;

        /// <summary>
        /// The identifier for the AxisType_PowerTrainIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint AxisType_PowerTrainIdentifier_Lock_LockingClient = 18353;

        /// <summary>
        /// The identifier for the AxisType_PowerTrainIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint AxisType_PowerTrainIdentifier_Lock_LockingUser = 18354;

        /// <summary>
        /// The identifier for the AxisType_PowerTrainIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint AxisType_PowerTrainIdentifier_Lock_RemainingLockTime = 18355;

        /// <summary>
        /// The identifier for the AxisType_PowerTrainIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint AxisType_PowerTrainIdentifier_Lock_InitLock_InputArguments = 18357;

        /// <summary>
        /// The identifier for the AxisType_PowerTrainIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint AxisType_PowerTrainIdentifier_Lock_InitLock_OutputArguments = 18358;

        /// <summary>
        /// The identifier for the AxisType_PowerTrainIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint AxisType_PowerTrainIdentifier_Lock_RenewLock_OutputArguments = 18360;

        /// <summary>
        /// The identifier for the AxisType_PowerTrainIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint AxisType_PowerTrainIdentifier_Lock_ExitLock_OutputArguments = 18362;

        /// <summary>
        /// The identifier for the AxisType_PowerTrainIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint AxisType_PowerTrainIdentifier_Lock_BreakLock_OutputArguments = 18364;

        /// <summary>
        /// The identifier for the PowerTrainType_Lock_Locked Variable.
        /// </summary>
        public const uint PowerTrainType_Lock_Locked = 15791;

        /// <summary>
        /// The identifier for the PowerTrainType_Lock_LockingClient Variable.
        /// </summary>
        public const uint PowerTrainType_Lock_LockingClient = 16805;

        /// <summary>
        /// The identifier for the PowerTrainType_Lock_LockingUser Variable.
        /// </summary>
        public const uint PowerTrainType_Lock_LockingUser = 16806;

        /// <summary>
        /// The identifier for the PowerTrainType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint PowerTrainType_Lock_RemainingLockTime = 16807;

        /// <summary>
        /// The identifier for the PowerTrainType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint PowerTrainType_Lock_InitLock_InputArguments = 16809;

        /// <summary>
        /// The identifier for the PowerTrainType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint PowerTrainType_Lock_InitLock_OutputArguments = 16810;

        /// <summary>
        /// The identifier for the PowerTrainType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint PowerTrainType_Lock_RenewLock_OutputArguments = 16812;

        /// <summary>
        /// The identifier for the PowerTrainType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint PowerTrainType_Lock_ExitLock_OutputArguments = 16814;

        /// <summary>
        /// The identifier for the PowerTrainType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint PowerTrainType_Lock_BreakLock_OutputArguments = 16816;

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint PowerTrainType_MotorIdentifier_Lock_Locked = 16006;

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint PowerTrainType_MotorIdentifier_Lock_LockingClient = 16007;

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint PowerTrainType_MotorIdentifier_Lock_LockingUser = 16008;

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint PowerTrainType_MotorIdentifier_Lock_RemainingLockTime = 16009;

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint PowerTrainType_MotorIdentifier_Lock_InitLock_InputArguments = 16011;

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint PowerTrainType_MotorIdentifier_Lock_InitLock_OutputArguments = 16012;

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint PowerTrainType_MotorIdentifier_Lock_RenewLock_OutputArguments = 16014;

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint PowerTrainType_MotorIdentifier_Lock_ExitLock_OutputArguments = 16016;

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint PowerTrainType_MotorIdentifier_Lock_BreakLock_OutputArguments = 16018;

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_Manufacturer Variable.
        /// </summary>
        public const uint PowerTrainType_MotorIdentifier_Manufacturer = 16019;

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_Model Variable.
        /// </summary>
        public const uint PowerTrainType_MotorIdentifier_Model = 16021;

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_ProductCode Variable.
        /// </summary>
        public const uint PowerTrainType_MotorIdentifier_ProductCode = 16025;

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_SerialNumber Variable.
        /// </summary>
        public const uint PowerTrainType_MotorIdentifier_SerialNumber = 16028;

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_ParameterSet_MotorTemperature Variable.
        /// </summary>
        public const uint PowerTrainType_MotorIdentifier_ParameterSet_MotorTemperature = 16034;

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_ParameterSet_MotorTemperature_EngineeringUnits Variable.
        /// </summary>
        public const uint PowerTrainType_MotorIdentifier_ParameterSet_MotorTemperature_EngineeringUnits = 16039;

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint PowerTrainType_GearIdentifier_Lock_Locked = 16049;

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint PowerTrainType_GearIdentifier_Lock_LockingClient = 16050;

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint PowerTrainType_GearIdentifier_Lock_LockingUser = 16051;

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint PowerTrainType_GearIdentifier_Lock_RemainingLockTime = 16052;

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint PowerTrainType_GearIdentifier_Lock_InitLock_InputArguments = 16054;

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint PowerTrainType_GearIdentifier_Lock_InitLock_OutputArguments = 16055;

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint PowerTrainType_GearIdentifier_Lock_RenewLock_OutputArguments = 16057;

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint PowerTrainType_GearIdentifier_Lock_ExitLock_OutputArguments = 16059;

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint PowerTrainType_GearIdentifier_Lock_BreakLock_OutputArguments = 16061;

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_Manufacturer Variable.
        /// </summary>
        public const uint PowerTrainType_GearIdentifier_Manufacturer = 16062;

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_Model Variable.
        /// </summary>
        public const uint PowerTrainType_GearIdentifier_Model = 16064;

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_ProductCode Variable.
        /// </summary>
        public const uint PowerTrainType_GearIdentifier_ProductCode = 16068;

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_SerialNumber Variable.
        /// </summary>
        public const uint PowerTrainType_GearIdentifier_SerialNumber = 16071;

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_GearRatio Variable.
        /// </summary>
        public const uint PowerTrainType_GearIdentifier_GearRatio = 16076;

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_GearRatio_Numerator Variable.
        /// </summary>
        public const uint PowerTrainType_GearIdentifier_GearRatio_Numerator = 16077;

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_GearRatio_Denominator Variable.
        /// </summary>
        public const uint PowerTrainType_GearIdentifier_GearRatio_Denominator = 16078;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_Lock_Locked = 15797;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_Lock_LockingClient = 18545;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_Lock_LockingUser = 18546;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_Lock_RemainingLockTime = 18547;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_Lock_InitLock_InputArguments = 18549;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_Lock_InitLock_OutputArguments = 18550;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_Lock_RenewLock_OutputArguments = 18552;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_Lock_ExitLock_OutputArguments = 18554;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_Lock_BreakLock_OutputArguments = 18556;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_MotionProfile Variable.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_MotionProfile = 18570;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_AdditionalLoad_Mass Variable.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_AdditionalLoad_Mass = 18572;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_AdditionalLoad_Mass_EngineeringUnits Variable.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_AdditionalLoad_Mass_EngineeringUnits = 18577;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates Variable.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates = 16080;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation Variable.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation = 16082;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_X Variable.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_X = 16084;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Y Variable.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Y = 16085;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Z Variable.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Z = 16086;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_A Variable.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_A = 16087;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_B Variable.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_B = 16088;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_C Variable.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_C = 16089;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_AdditionalLoad_Inertia_X Variable.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_AdditionalLoad_Inertia_X = 18591;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_AdditionalLoad_Inertia_Y Variable.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_AdditionalLoad_Inertia_Y = 18592;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_AdditionalLoad_Inertia_Z Variable.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_AdditionalLoad_Inertia_Z = 18593;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_ParameterSet_ActualPosition Variable.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_ParameterSet_ActualPosition = 18595;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_ParameterSet_ActualPosition_EngineeringUnits Variable.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_ParameterSet_ActualPosition_EngineeringUnits = 18600;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_ParameterSet_ActualSpeed_EngineeringUnits Variable.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_ParameterSet_ActualSpeed_EngineeringUnits = 18606;

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_ParameterSet_ActualAcceleration_EngineeringUnits Variable.
        /// </summary>
        public const uint PowerTrainType_AxisIdentifier_ParameterSet_ActualAcceleration_EngineeringUnits = 18612;

        /// <summary>
        /// The identifier for the PowerTrainType_PowerTrainIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint PowerTrainType_PowerTrainIdentifier_Lock_Locked = 15827;

        /// <summary>
        /// The identifier for the PowerTrainType_PowerTrainIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint PowerTrainType_PowerTrainIdentifier_Lock_LockingClient = 18622;

        /// <summary>
        /// The identifier for the PowerTrainType_PowerTrainIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint PowerTrainType_PowerTrainIdentifier_Lock_LockingUser = 18623;

        /// <summary>
        /// The identifier for the PowerTrainType_PowerTrainIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint PowerTrainType_PowerTrainIdentifier_Lock_RemainingLockTime = 18624;

        /// <summary>
        /// The identifier for the PowerTrainType_PowerTrainIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint PowerTrainType_PowerTrainIdentifier_Lock_InitLock_InputArguments = 18626;

        /// <summary>
        /// The identifier for the PowerTrainType_PowerTrainIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint PowerTrainType_PowerTrainIdentifier_Lock_InitLock_OutputArguments = 18627;

        /// <summary>
        /// The identifier for the PowerTrainType_PowerTrainIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint PowerTrainType_PowerTrainIdentifier_Lock_RenewLock_OutputArguments = 18629;

        /// <summary>
        /// The identifier for the PowerTrainType_PowerTrainIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint PowerTrainType_PowerTrainIdentifier_Lock_ExitLock_OutputArguments = 18631;

        /// <summary>
        /// The identifier for the PowerTrainType_PowerTrainIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint PowerTrainType_PowerTrainIdentifier_Lock_BreakLock_OutputArguments = 18633;

        /// <summary>
        /// The identifier for the MotorType_Lock_Locked Variable.
        /// </summary>
        public const uint MotorType_Lock_Locked = 15833;

        /// <summary>
        /// The identifier for the MotorType_Lock_LockingClient Variable.
        /// </summary>
        public const uint MotorType_Lock_LockingClient = 15563;

        /// <summary>
        /// The identifier for the MotorType_Lock_LockingUser Variable.
        /// </summary>
        public const uint MotorType_Lock_LockingUser = 15564;

        /// <summary>
        /// The identifier for the MotorType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint MotorType_Lock_RemainingLockTime = 15565;

        /// <summary>
        /// The identifier for the MotorType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint MotorType_Lock_InitLock_InputArguments = 15567;

        /// <summary>
        /// The identifier for the MotorType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint MotorType_Lock_InitLock_OutputArguments = 15568;

        /// <summary>
        /// The identifier for the MotorType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint MotorType_Lock_RenewLock_OutputArguments = 15570;

        /// <summary>
        /// The identifier for the MotorType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint MotorType_Lock_ExitLock_OutputArguments = 15572;

        /// <summary>
        /// The identifier for the MotorType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint MotorType_Lock_BreakLock_OutputArguments = 15574;

        /// <summary>
        /// The identifier for the MotorType_Manufacturer Variable.
        /// </summary>
        public const uint MotorType_Manufacturer = 17101;

        /// <summary>
        /// The identifier for the MotorType_Model Variable.
        /// </summary>
        public const uint MotorType_Model = 17103;

        /// <summary>
        /// The identifier for the MotorType_ProductCode Variable.
        /// </summary>
        public const uint MotorType_ProductCode = 17109;

        /// <summary>
        /// The identifier for the MotorType_SerialNumber Variable.
        /// </summary>
        public const uint MotorType_SerialNumber = 17104;

        /// <summary>
        /// The identifier for the MotorType_ParameterSet_BrakeReleased Variable.
        /// </summary>
        public const uint MotorType_ParameterSet_BrakeReleased = 17150;

        /// <summary>
        /// The identifier for the MotorType_ParameterSet_MotorTemperature Variable.
        /// </summary>
        public const uint MotorType_ParameterSet_MotorTemperature = 6757;

        /// <summary>
        /// The identifier for the MotorType_ParameterSet_MotorTemperature_EngineeringUnits Variable.
        /// </summary>
        public const uint MotorType_ParameterSet_MotorTemperature_EngineeringUnits = 6762;

        /// <summary>
        /// The identifier for the MotorType_ParameterSet_EffectiveLoadRate Variable.
        /// </summary>
        public const uint MotorType_ParameterSet_EffectiveLoadRate = 6776;

        /// <summary>
        /// The identifier for the GearType_Lock_Locked Variable.
        /// </summary>
        public const uint GearType_Lock_Locked = 15837;

        /// <summary>
        /// The identifier for the GearType_Lock_LockingClient Variable.
        /// </summary>
        public const uint GearType_Lock_LockingClient = 15925;

        /// <summary>
        /// The identifier for the GearType_Lock_LockingUser Variable.
        /// </summary>
        public const uint GearType_Lock_LockingUser = 15926;

        /// <summary>
        /// The identifier for the GearType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint GearType_Lock_RemainingLockTime = 15927;

        /// <summary>
        /// The identifier for the GearType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint GearType_Lock_InitLock_InputArguments = 15929;

        /// <summary>
        /// The identifier for the GearType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint GearType_Lock_InitLock_OutputArguments = 15930;

        /// <summary>
        /// The identifier for the GearType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint GearType_Lock_RenewLock_OutputArguments = 15932;

        /// <summary>
        /// The identifier for the GearType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint GearType_Lock_ExitLock_OutputArguments = 15934;

        /// <summary>
        /// The identifier for the GearType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint GearType_Lock_BreakLock_OutputArguments = 15936;

        /// <summary>
        /// The identifier for the GearType_Manufacturer Variable.
        /// </summary>
        public const uint GearType_Manufacturer = 17152;

        /// <summary>
        /// The identifier for the GearType_Model Variable.
        /// </summary>
        public const uint GearType_Model = 17154;

        /// <summary>
        /// The identifier for the GearType_ProductCode Variable.
        /// </summary>
        public const uint GearType_ProductCode = 17160;

        /// <summary>
        /// The identifier for the GearType_SerialNumber Variable.
        /// </summary>
        public const uint GearType_SerialNumber = 17155;

        /// <summary>
        /// The identifier for the GearType_GearRatio Variable.
        /// </summary>
        public const uint GearType_GearRatio = 15941;

        /// <summary>
        /// The identifier for the GearType_GearRatio_Numerator Variable.
        /// </summary>
        public const uint GearType_GearRatio_Numerator = 15615;

        /// <summary>
        /// The identifier for the GearType_GearRatio_Denominator Variable.
        /// </summary>
        public const uint GearType_GearRatio_Denominator = 15616;

        /// <summary>
        /// The identifier for the GearType_Pitch Variable.
        /// </summary>
        public const uint GearType_Pitch = 17165;

        /// <summary>
        /// The identifier for the SafetyStateType_Lock_Locked Variable.
        /// </summary>
        public const uint SafetyStateType_Lock_Locked = 15841;

        /// <summary>
        /// The identifier for the SafetyStateType_Lock_LockingClient Variable.
        /// </summary>
        public const uint SafetyStateType_Lock_LockingClient = 15174;

        /// <summary>
        /// The identifier for the SafetyStateType_Lock_LockingUser Variable.
        /// </summary>
        public const uint SafetyStateType_Lock_LockingUser = 15175;

        /// <summary>
        /// The identifier for the SafetyStateType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint SafetyStateType_Lock_RemainingLockTime = 15176;

        /// <summary>
        /// The identifier for the SafetyStateType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint SafetyStateType_Lock_InitLock_InputArguments = 15178;

        /// <summary>
        /// The identifier for the SafetyStateType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint SafetyStateType_Lock_InitLock_OutputArguments = 15179;

        /// <summary>
        /// The identifier for the SafetyStateType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint SafetyStateType_Lock_RenewLock_OutputArguments = 15181;

        /// <summary>
        /// The identifier for the SafetyStateType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint SafetyStateType_Lock_ExitLock_OutputArguments = 15183;

        /// <summary>
        /// The identifier for the SafetyStateType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint SafetyStateType_Lock_BreakLock_OutputArguments = 15185;

        /// <summary>
        /// The identifier for the SafetyStateType_EmergencyStopFunctions_EmergencyStopFunctionIdentifier_Name Variable.
        /// </summary>
        public const uint SafetyStateType_EmergencyStopFunctions_EmergencyStopFunctionIdentifier_Name = 18807;

        /// <summary>
        /// The identifier for the SafetyStateType_EmergencyStopFunctions_EmergencyStopFunctionIdentifier_Active Variable.
        /// </summary>
        public const uint SafetyStateType_EmergencyStopFunctions_EmergencyStopFunctionIdentifier_Active = 18808;

        /// <summary>
        /// The identifier for the SafetyStateType_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Name Variable.
        /// </summary>
        public const uint SafetyStateType_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Name = 18810;

        /// <summary>
        /// The identifier for the SafetyStateType_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Enabled Variable.
        /// </summary>
        public const uint SafetyStateType_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Enabled = 18811;

        /// <summary>
        /// The identifier for the SafetyStateType_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Active Variable.
        /// </summary>
        public const uint SafetyStateType_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Active = 18812;

        /// <summary>
        /// The identifier for the SafetyStateType_ParameterSet_OperationalMode Variable.
        /// </summary>
        public const uint SafetyStateType_ParameterSet_OperationalMode = 15912;

        /// <summary>
        /// The identifier for the SafetyStateType_ParameterSet_EmergencyStop Variable.
        /// </summary>
        public const uint SafetyStateType_ParameterSet_EmergencyStop = 15882;

        /// <summary>
        /// The identifier for the SafetyStateType_ParameterSet_ProtectiveStop Variable.
        /// </summary>
        public const uint SafetyStateType_ParameterSet_ProtectiveStop = 15913;

        /// <summary>
        /// The identifier for the EmergencyStopFunctionType_Name Variable.
        /// </summary>
        public const uint EmergencyStopFunctionType_Name = 17231;

        /// <summary>
        /// The identifier for the EmergencyStopFunctionType_Active Variable.
        /// </summary>
        public const uint EmergencyStopFunctionType_Active = 17232;

        /// <summary>
        /// The identifier for the ProtectiveStopFunctionType_Name Variable.
        /// </summary>
        public const uint ProtectiveStopFunctionType_Name = 17234;

        /// <summary>
        /// The identifier for the ProtectiveStopFunctionType_Enabled Variable.
        /// </summary>
        public const uint ProtectiveStopFunctionType_Enabled = 17235;

        /// <summary>
        /// The identifier for the ProtectiveStopFunctionType_Active Variable.
        /// </summary>
        public const uint ProtectiveStopFunctionType_Active = 17236;

        /// <summary>
        /// The identifier for the ControllerType_Lock_Locked Variable.
        /// </summary>
        public const uint ControllerType_Lock_Locked = 15843;

        /// <summary>
        /// The identifier for the ControllerType_Lock_LockingClient Variable.
        /// </summary>
        public const uint ControllerType_Lock_LockingClient = 15146;

        /// <summary>
        /// The identifier for the ControllerType_Lock_LockingUser Variable.
        /// </summary>
        public const uint ControllerType_Lock_LockingUser = 15147;

        /// <summary>
        /// The identifier for the ControllerType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ControllerType_Lock_RemainingLockTime = 15148;

        /// <summary>
        /// The identifier for the ControllerType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ControllerType_Lock_InitLock_InputArguments = 15150;

        /// <summary>
        /// The identifier for the ControllerType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_Lock_InitLock_OutputArguments = 15151;

        /// <summary>
        /// The identifier for the ControllerType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_Lock_RenewLock_OutputArguments = 15153;

        /// <summary>
        /// The identifier for the ControllerType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_Lock_ExitLock_OutputArguments = 15155;

        /// <summary>
        /// The identifier for the ControllerType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_Lock_BreakLock_OutputArguments = 15157;

        /// <summary>
        /// The identifier for the ControllerType_Manufacturer Variable.
        /// </summary>
        public const uint ControllerType_Manufacturer = 17237;

        /// <summary>
        /// The identifier for the ControllerType_Model Variable.
        /// </summary>
        public const uint ControllerType_Model = 17239;

        /// <summary>
        /// The identifier for the ControllerType_ProductCode Variable.
        /// </summary>
        public const uint ControllerType_ProductCode = 17245;

        /// <summary>
        /// The identifier for the ControllerType_SerialNumber Variable.
        /// </summary>
        public const uint ControllerType_SerialNumber = 17240;

        /// <summary>
        /// The identifier for the ControllerType_CurrentUser_Level Variable.
        /// </summary>
        public const uint ControllerType_CurrentUser_Level = 17250;

        /// <summary>
        /// The identifier for the ControllerType_Components_ComponentIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ControllerType_Components_ComponentIdentifier_Lock_Locked = 15845;

        /// <summary>
        /// The identifier for the ControllerType_Components_ComponentIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ControllerType_Components_ComponentIdentifier_Lock_LockingClient = 18822;

        /// <summary>
        /// The identifier for the ControllerType_Components_ComponentIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ControllerType_Components_ComponentIdentifier_Lock_LockingUser = 18823;

        /// <summary>
        /// The identifier for the ControllerType_Components_ComponentIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ControllerType_Components_ComponentIdentifier_Lock_RemainingLockTime = 18824;

        /// <summary>
        /// The identifier for the ControllerType_Components_ComponentIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ControllerType_Components_ComponentIdentifier_Lock_InitLock_InputArguments = 18826;

        /// <summary>
        /// The identifier for the ControllerType_Components_ComponentIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_Components_ComponentIdentifier_Lock_InitLock_OutputArguments = 18827;

        /// <summary>
        /// The identifier for the ControllerType_Components_ComponentIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_Components_ComponentIdentifier_Lock_RenewLock_OutputArguments = 18829;

        /// <summary>
        /// The identifier for the ControllerType_Components_ComponentIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_Components_ComponentIdentifier_Lock_ExitLock_OutputArguments = 18831;

        /// <summary>
        /// The identifier for the ControllerType_Components_ComponentIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_Components_ComponentIdentifier_Lock_BreakLock_OutputArguments = 18833;

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ControllerType_Software_SoftwareIdentifier_Lock_Locked = 15847;

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ControllerType_Software_SoftwareIdentifier_Lock_LockingClient = 18856;

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ControllerType_Software_SoftwareIdentifier_Lock_LockingUser = 18857;

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ControllerType_Software_SoftwareIdentifier_Lock_RemainingLockTime = 18858;

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ControllerType_Software_SoftwareIdentifier_Lock_InitLock_InputArguments = 18860;

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_Software_SoftwareIdentifier_Lock_InitLock_OutputArguments = 18861;

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_Software_SoftwareIdentifier_Lock_RenewLock_OutputArguments = 18863;

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_Software_SoftwareIdentifier_Lock_ExitLock_OutputArguments = 18865;

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_Software_SoftwareIdentifier_Lock_BreakLock_OutputArguments = 18867;

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_Manufacturer Variable.
        /// </summary>
        public const uint ControllerType_Software_SoftwareIdentifier_Manufacturer = 18868;

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_Model Variable.
        /// </summary>
        public const uint ControllerType_Software_SoftwareIdentifier_Model = 18870;

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_SoftwareRevision Variable.
        /// </summary>
        public const uint ControllerType_Software_SoftwareIdentifier_SoftwareRevision = 18873;

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ControllerType_TaskControls_TaskControlIdentifier_Lock_Locked = 15849;

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ControllerType_TaskControls_TaskControlIdentifier_Lock_LockingClient = 18890;

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ControllerType_TaskControls_TaskControlIdentifier_Lock_LockingUser = 18891;

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ControllerType_TaskControls_TaskControlIdentifier_Lock_RemainingLockTime = 18892;

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ControllerType_TaskControls_TaskControlIdentifier_Lock_InitLock_InputArguments = 18894;

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_TaskControls_TaskControlIdentifier_Lock_InitLock_OutputArguments = 18895;

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_TaskControls_TaskControlIdentifier_Lock_RenewLock_OutputArguments = 18897;

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_TaskControls_TaskControlIdentifier_Lock_ExitLock_OutputArguments = 18899;

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_TaskControls_TaskControlIdentifier_Lock_BreakLock_OutputArguments = 18901;

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_ComponentName Variable.
        /// </summary>
        public const uint ControllerType_TaskControls_TaskControlIdentifier_ComponentName = 18914;

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_ParameterSet_TaskProgramName Variable.
        /// </summary>
        public const uint ControllerType_TaskControls_TaskControlIdentifier_ParameterSet_TaskProgramName = 18915;

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_ParameterSet_TaskProgramLoaded Variable.
        /// </summary>
        public const uint ControllerType_TaskControls_TaskControlIdentifier_ParameterSet_TaskProgramLoaded = 18916;

        /// <summary>
        /// The identifier for the ControllerType_ParameterSet_TotalPowerOnTime Variable.
        /// </summary>
        public const uint ControllerType_ParameterSet_TotalPowerOnTime = 17358;

        /// <summary>
        /// The identifier for the ControllerType_ParameterSet_StartUpTime Variable.
        /// </summary>
        public const uint ControllerType_ParameterSet_StartUpTime = 15366;

        /// <summary>
        /// The identifier for the ControllerType_ParameterSet_UpsState Variable.
        /// </summary>
        public const uint ControllerType_ParameterSet_UpsState = 15365;

        /// <summary>
        /// The identifier for the ControllerType_ParameterSet_TotalEnergyConsumption Variable.
        /// </summary>
        public const uint ControllerType_ParameterSet_TotalEnergyConsumption = 17359;

        /// <summary>
        /// The identifier for the ControllerType_ParameterSet_TotalEnergyConsumption_EngineeringUnits Variable.
        /// </summary>
        public const uint ControllerType_ParameterSet_TotalEnergyConsumption_EngineeringUnits = 17364;

        /// <summary>
        /// The identifier for the ControllerType_ParameterSet_CabinetFanSpeed Variable.
        /// </summary>
        public const uint ControllerType_ParameterSet_CabinetFanSpeed = 17365;

        /// <summary>
        /// The identifier for the ControllerType_ParameterSet_CabinetFanSpeed_EngineeringUnits Variable.
        /// </summary>
        public const uint ControllerType_ParameterSet_CabinetFanSpeed_EngineeringUnits = 17370;

        /// <summary>
        /// The identifier for the ControllerType_ParameterSet_CPUFanSpeed Variable.
        /// </summary>
        public const uint ControllerType_ParameterSet_CPUFanSpeed = 17371;

        /// <summary>
        /// The identifier for the ControllerType_ParameterSet_CPUFanSpeed_EngineeringUnits Variable.
        /// </summary>
        public const uint ControllerType_ParameterSet_CPUFanSpeed_EngineeringUnits = 17376;

        /// <summary>
        /// The identifier for the ControllerType_ParameterSet_InputVoltage Variable.
        /// </summary>
        public const uint ControllerType_ParameterSet_InputVoltage = 17377;

        /// <summary>
        /// The identifier for the ControllerType_ParameterSet_InputVoltage_EngineeringUnits Variable.
        /// </summary>
        public const uint ControllerType_ParameterSet_InputVoltage_EngineeringUnits = 17382;

        /// <summary>
        /// The identifier for the ControllerType_ParameterSet_Temperature Variable.
        /// </summary>
        public const uint ControllerType_ParameterSet_Temperature = 17383;

        /// <summary>
        /// The identifier for the ControllerType_ParameterSet_Temperature_EngineeringUnits Variable.
        /// </summary>
        public const uint ControllerType_ParameterSet_Temperature_EngineeringUnits = 17388;

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ControllerType_SafetyStatesIdentifier_Lock_Locked = 15851;

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ControllerType_SafetyStatesIdentifier_Lock_LockingClient = 18927;

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ControllerType_SafetyStatesIdentifier_Lock_LockingUser = 18928;

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ControllerType_SafetyStatesIdentifier_Lock_RemainingLockTime = 18929;

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ControllerType_SafetyStatesIdentifier_Lock_InitLock_InputArguments = 18931;

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_SafetyStatesIdentifier_Lock_InitLock_OutputArguments = 18932;

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_SafetyStatesIdentifier_Lock_RenewLock_OutputArguments = 18934;

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_SafetyStatesIdentifier_Lock_ExitLock_OutputArguments = 18936;

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_SafetyStatesIdentifier_Lock_BreakLock_OutputArguments = 18938;

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_EmergencyStopFunctions_EmergencyStopFunctionIdentifier_Name Variable.
        /// </summary>
        public const uint ControllerType_SafetyStatesIdentifier_EmergencyStopFunctions_EmergencyStopFunctionIdentifier_Name = 18954;

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_EmergencyStopFunctions_EmergencyStopFunctionIdentifier_Active Variable.
        /// </summary>
        public const uint ControllerType_SafetyStatesIdentifier_EmergencyStopFunctions_EmergencyStopFunctionIdentifier_Active = 18955;

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Name Variable.
        /// </summary>
        public const uint ControllerType_SafetyStatesIdentifier_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Name = 18958;

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Enabled Variable.
        /// </summary>
        public const uint ControllerType_SafetyStatesIdentifier_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Enabled = 18959;

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Active Variable.
        /// </summary>
        public const uint ControllerType_SafetyStatesIdentifier_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Active = 18960;

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_ParameterSet_OperationalMode Variable.
        /// </summary>
        public const uint ControllerType_SafetyStatesIdentifier_ParameterSet_OperationalMode = 18961;

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_ParameterSet_EmergencyStop Variable.
        /// </summary>
        public const uint ControllerType_SafetyStatesIdentifier_ParameterSet_EmergencyStop = 18962;

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_ParameterSet_ProtectiveStop Variable.
        /// </summary>
        public const uint ControllerType_SafetyStatesIdentifier_ParameterSet_ProtectiveStop = 18963;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Lock_Locked = 15853;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Lock_LockingClient = 18973;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Lock_LockingUser = 18974;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Lock_RemainingLockTime = 18975;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Lock_InitLock_InputArguments = 18977;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Lock_InitLock_OutputArguments = 18978;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Lock_RenewLock_OutputArguments = 18980;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Lock_ExitLock_OutputArguments = 18982;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Lock_BreakLock_OutputArguments = 18984;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Manufacturer Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Manufacturer = 18985;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Model Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Model = 18987;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_ProductCode Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_ProductCode = 18993;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_SerialNumber Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_SerialNumber = 18988;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_MotionDeviceCategory Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_MotionDeviceCategory = 18998;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_ParameterSet_SpeedOverride Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_ParameterSet_SpeedOverride = 19001;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_Locked = 15865;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_LockingClient = 19012;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_LockingUser = 19013;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RemainingLockTime = 19014;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock_InputArguments = 19016;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock_OutputArguments = 19017;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RenewLock_OutputArguments = 19019;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_ExitLock_OutputArguments = 19021;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_BreakLock_OutputArguments = 19023;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_MotionProfile Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_MotionProfile = 19037;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Mass Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Mass = 19039;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Mass_EngineeringUnits Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Mass_EngineeringUnits = 19044;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates = 16090;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation = 16092;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_X Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_X = 16094;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Y Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Y = 16095;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Z Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Z = 16096;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_A Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_A = 16097;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_B Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_B = 16098;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_C Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_C = 16099;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_X Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_X = 19058;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_Y Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_Y = 19059;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_Z Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_Z = 19060;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualPosition Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualPosition = 19062;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualPosition_EngineeringUnits Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualPosition_EngineeringUnits = 19067;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualSpeed_EngineeringUnits Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualSpeed_EngineeringUnits = 19073;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualAcceleration_EngineeringUnits Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualAcceleration_EngineeringUnits = 19079;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_Locked = 15906;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_LockingClient = 19090;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_LockingUser = 19091;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RemainingLockTime = 19092;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock_InputArguments = 19094;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock_OutputArguments = 19095;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RenewLock_OutputArguments = 19097;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_ExitLock_OutputArguments = 19099;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_BreakLock_OutputArguments = 19101;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_FlangeLoad_Mass Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_FlangeLoad_Mass = 19196;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_FlangeLoad_Mass_EngineeringUnits Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_FlangeLoad_Mass_EngineeringUnits = 19201;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates = 16100;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation = 16102;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_X Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_X = 16104;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_Y Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_Y = 16105;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_Z Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_Z = 16106;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_A Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_A = 16107;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_B Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_B = 16108;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_C Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_C = 16109;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_FlangeLoad_Inertia_X Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_FlangeLoad_Inertia_X = 19215;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_FlangeLoad_Inertia_Y Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_FlangeLoad_Inertia_Y = 19216;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_FlangeLoad_Inertia_Z Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_FlangeLoad_Inertia_Z = 19217;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_Locked = 15943;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingClient = 19229;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingUser = 19230;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RemainingLockTime = 19231;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_InputArguments = 19233;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_OutputArguments = 19234;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock_OutputArguments = 19236;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock_OutputArguments = 19238;

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock_OutputArguments = 19240;

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_Lock_Locked Variable.
        /// </summary>
        public const uint AuxiliaryComponentType_Lock_Locked = 15945;

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_Lock_LockingClient Variable.
        /// </summary>
        public const uint AuxiliaryComponentType_Lock_LockingClient = 17736;

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_Lock_LockingUser Variable.
        /// </summary>
        public const uint AuxiliaryComponentType_Lock_LockingUser = 17737;

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint AuxiliaryComponentType_Lock_RemainingLockTime = 17738;

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint AuxiliaryComponentType_Lock_InitLock_InputArguments = 17740;

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint AuxiliaryComponentType_Lock_InitLock_OutputArguments = 17741;

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint AuxiliaryComponentType_Lock_RenewLock_OutputArguments = 17743;

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint AuxiliaryComponentType_Lock_ExitLock_OutputArguments = 17745;

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint AuxiliaryComponentType_Lock_BreakLock_OutputArguments = 17747;

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_ProductCode Variable.
        /// </summary>
        public const uint AuxiliaryComponentType_ProductCode = 17756;

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint AuxiliaryComponentType_CPIdentifier_Lock_Locked = 15947;

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint AuxiliaryComponentType_CPIdentifier_Lock_LockingClient = 17770;

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint AuxiliaryComponentType_CPIdentifier_Lock_LockingUser = 17771;

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint AuxiliaryComponentType_CPIdentifier_Lock_RemainingLockTime = 17772;

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint AuxiliaryComponentType_CPIdentifier_Lock_InitLock_InputArguments = 17774;

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint AuxiliaryComponentType_CPIdentifier_Lock_InitLock_OutputArguments = 17775;

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint AuxiliaryComponentType_CPIdentifier_Lock_RenewLock_OutputArguments = 17777;

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint AuxiliaryComponentType_CPIdentifier_Lock_ExitLock_OutputArguments = 17779;

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint AuxiliaryComponentType_CPIdentifier_Lock_BreakLock_OutputArguments = 17781;

        /// <summary>
        /// The identifier for the DriveType_Lock_Locked Variable.
        /// </summary>
        public const uint DriveType_Lock_Locked = 15949;

        /// <summary>
        /// The identifier for the DriveType_Lock_LockingClient Variable.
        /// </summary>
        public const uint DriveType_Lock_LockingClient = 17804;

        /// <summary>
        /// The identifier for the DriveType_Lock_LockingUser Variable.
        /// </summary>
        public const uint DriveType_Lock_LockingUser = 17805;

        /// <summary>
        /// The identifier for the DriveType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint DriveType_Lock_RemainingLockTime = 17806;

        /// <summary>
        /// The identifier for the DriveType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint DriveType_Lock_InitLock_InputArguments = 17808;

        /// <summary>
        /// The identifier for the DriveType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint DriveType_Lock_InitLock_OutputArguments = 17809;

        /// <summary>
        /// The identifier for the DriveType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint DriveType_Lock_RenewLock_OutputArguments = 17811;

        /// <summary>
        /// The identifier for the DriveType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint DriveType_Lock_ExitLock_OutputArguments = 17813;

        /// <summary>
        /// The identifier for the DriveType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint DriveType_Lock_BreakLock_OutputArguments = 17815;

        /// <summary>
        /// The identifier for the DriveType_ProductCode Variable.
        /// </summary>
        public const uint DriveType_ProductCode = 17824;

        /// <summary>
        /// The identifier for the DriveType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint DriveType_CPIdentifier_Lock_Locked = 15951;

        /// <summary>
        /// The identifier for the DriveType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint DriveType_CPIdentifier_Lock_LockingClient = 17838;

        /// <summary>
        /// The identifier for the DriveType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint DriveType_CPIdentifier_Lock_LockingUser = 17839;

        /// <summary>
        /// The identifier for the DriveType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint DriveType_CPIdentifier_Lock_RemainingLockTime = 17840;

        /// <summary>
        /// The identifier for the DriveType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint DriveType_CPIdentifier_Lock_InitLock_InputArguments = 17842;

        /// <summary>
        /// The identifier for the DriveType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint DriveType_CPIdentifier_Lock_InitLock_OutputArguments = 17843;

        /// <summary>
        /// The identifier for the DriveType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint DriveType_CPIdentifier_Lock_RenewLock_OutputArguments = 17845;

        /// <summary>
        /// The identifier for the DriveType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint DriveType_CPIdentifier_Lock_ExitLock_OutputArguments = 17847;

        /// <summary>
        /// The identifier for the DriveType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint DriveType_CPIdentifier_Lock_BreakLock_OutputArguments = 17849;

        /// <summary>
        /// The identifier for the TaskControlType_Lock_Locked Variable.
        /// </summary>
        public const uint TaskControlType_Lock_Locked = 15953;

        /// <summary>
        /// The identifier for the TaskControlType_Lock_LockingClient Variable.
        /// </summary>
        public const uint TaskControlType_Lock_LockingClient = 15893;

        /// <summary>
        /// The identifier for the TaskControlType_Lock_LockingUser Variable.
        /// </summary>
        public const uint TaskControlType_Lock_LockingUser = 15894;

        /// <summary>
        /// The identifier for the TaskControlType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint TaskControlType_Lock_RemainingLockTime = 15895;

        /// <summary>
        /// The identifier for the TaskControlType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint TaskControlType_Lock_InitLock_InputArguments = 15897;

        /// <summary>
        /// The identifier for the TaskControlType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint TaskControlType_Lock_InitLock_OutputArguments = 15898;

        /// <summary>
        /// The identifier for the TaskControlType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint TaskControlType_Lock_RenewLock_OutputArguments = 15900;

        /// <summary>
        /// The identifier for the TaskControlType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint TaskControlType_Lock_ExitLock_OutputArguments = 15902;

        /// <summary>
        /// The identifier for the TaskControlType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint TaskControlType_Lock_BreakLock_OutputArguments = 15904;

        /// <summary>
        /// The identifier for the TaskControlType_ComponentName Variable.
        /// </summary>
        public const uint TaskControlType_ComponentName = 17873;

        /// <summary>
        /// The identifier for the TaskControlType_ParameterSet_TaskProgramName Variable.
        /// </summary>
        public const uint TaskControlType_ParameterSet_TaskProgramName = 17874;

        /// <summary>
        /// The identifier for the TaskControlType_ParameterSet_TaskProgramLoaded Variable.
        /// </summary>
        public const uint TaskControlType_ParameterSet_TaskProgramLoaded = 17875;

        /// <summary>
        /// The identifier for the TaskControlType_ParameterSet_ExecutionMode Variable.
        /// </summary>
        public const uint TaskControlType_ParameterSet_ExecutionMode = 17876;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Lock_Locked = 15955;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Lock_LockingClient = 19263;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Lock_LockingUser = 19264;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Lock_RemainingLockTime = 19265;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Lock_InitLock_InputArguments = 19267;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Lock_InitLock_OutputArguments = 19268;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Lock_RenewLock_OutputArguments = 19270;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Lock_ExitLock_OutputArguments = 19272;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Lock_BreakLock_OutputArguments = 19274;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Manufacturer Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Manufacturer = 19275;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Model Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Model = 19277;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_ProductCode Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_ProductCode = 19283;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_SerialNumber Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_SerialNumber = 19278;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_MotionDeviceCategory Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_MotionDeviceCategory = 19288;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_ParameterSet_SpeedOverride Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_ParameterSet_SpeedOverride = 19291;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_Locked = 15957;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_LockingClient = 19302;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_LockingUser = 19303;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RemainingLockTime = 19304;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock_InputArguments = 19306;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock_OutputArguments = 19307;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RenewLock_OutputArguments = 19309;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_ExitLock_OutputArguments = 19311;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_BreakLock_OutputArguments = 19313;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_MotionProfile Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_MotionProfile = 19327;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Mass Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Mass = 19329;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Mass_EngineeringUnits Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Mass_EngineeringUnits = 19334;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates = 16110;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation = 16112;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_X Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_X = 16114;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Y Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Y = 16115;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Z Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Z = 16116;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_A Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_A = 16117;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_B Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_B = 16118;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_C Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_C = 16119;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_X Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_X = 19348;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_Y Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_Y = 19349;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_Z Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_Z = 19350;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualPosition Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualPosition = 19352;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualPosition_EngineeringUnits Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualPosition_EngineeringUnits = 19357;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualSpeed_EngineeringUnits Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualSpeed_EngineeringUnits = 19363;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualAcceleration_EngineeringUnits Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualAcceleration_EngineeringUnits = 19369;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_Locked = 15959;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_LockingClient = 19380;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_LockingUser = 19381;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RemainingLockTime = 19382;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock_InputArguments = 19384;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock_OutputArguments = 19385;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RenewLock_OutputArguments = 19387;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_ExitLock_OutputArguments = 19389;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_BreakLock_OutputArguments = 19391;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_FlangeLoad_Mass Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_FlangeLoad_Mass = 19486;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_FlangeLoad_Mass_EngineeringUnits Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_FlangeLoad_Mass_EngineeringUnits = 19491;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates = 16120;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation = 16122;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_X Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_X = 16124;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_Y Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_Y = 16125;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_Z Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_Z = 16126;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_A Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_A = 16127;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_B Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_B = 16128;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_C Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_C = 16129;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_FlangeLoad_Inertia_X Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_FlangeLoad_Inertia_X = 19505;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_FlangeLoad_Inertia_Y Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_FlangeLoad_Inertia_Y = 19506;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_FlangeLoad_Inertia_Z Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_FlangeLoad_Inertia_Z = 19507;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_Locked = 15965;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingClient = 19519;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingUser = 19520;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RemainingLockTime = 19521;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_InputArguments = 19523;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_OutputArguments = 19524;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock_OutputArguments = 19526;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock_OutputArguments = 19528;

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock_OutputArguments = 19530;

        /// <summary>
        /// The identifier for the LoadType_Mass Variable.
        /// </summary>
        public const uint LoadType_Mass = 6723;

        /// <summary>
        /// The identifier for the LoadType_Mass_EngineeringUnits Variable.
        /// </summary>
        public const uint LoadType_Mass_EngineeringUnits = 6728;

        /// <summary>
        /// The identifier for the LoadType_CenterOfMass Variable.
        /// </summary>
        public const uint LoadType_CenterOfMass = 6013;

        /// <summary>
        /// The identifier for the LoadType_CenterOfMass_CartesianCoordinates Variable.
        /// </summary>
        public const uint LoadType_CenterOfMass_CartesianCoordinates = 16130;

        /// <summary>
        /// The identifier for the LoadType_CenterOfMass_Orientation Variable.
        /// </summary>
        public const uint LoadType_CenterOfMass_Orientation = 16132;

        /// <summary>
        /// The identifier for the LoadType_CenterOfMass_CartesianCoordinates_X Variable.
        /// </summary>
        public const uint LoadType_CenterOfMass_CartesianCoordinates_X = 16134;

        /// <summary>
        /// The identifier for the LoadType_CenterOfMass_CartesianCoordinates_Y Variable.
        /// </summary>
        public const uint LoadType_CenterOfMass_CartesianCoordinates_Y = 16135;

        /// <summary>
        /// The identifier for the LoadType_CenterOfMass_CartesianCoordinates_Z Variable.
        /// </summary>
        public const uint LoadType_CenterOfMass_CartesianCoordinates_Z = 16136;

        /// <summary>
        /// The identifier for the LoadType_CenterOfMass_Orientation_A Variable.
        /// </summary>
        public const uint LoadType_CenterOfMass_Orientation_A = 16137;

        /// <summary>
        /// The identifier for the LoadType_CenterOfMass_Orientation_B Variable.
        /// </summary>
        public const uint LoadType_CenterOfMass_Orientation_B = 16138;

        /// <summary>
        /// The identifier for the LoadType_CenterOfMass_Orientation_C Variable.
        /// </summary>
        public const uint LoadType_CenterOfMass_Orientation_C = 16139;

        /// <summary>
        /// The identifier for the LoadType_Inertia Variable.
        /// </summary>
        public const uint LoadType_Inertia = 18170;

        /// <summary>
        /// The identifier for the LoadType_Inertia_X Variable.
        /// </summary>
        public const uint LoadType_Inertia_X = 18171;

        /// <summary>
        /// The identifier for the LoadType_Inertia_Y Variable.
        /// </summary>
        public const uint LoadType_Inertia_Y = 18172;

        /// <summary>
        /// The identifier for the LoadType_Inertia_Z Variable.
        /// </summary>
        public const uint LoadType_Inertia_Z = 18173;

        /// <summary>
        /// The identifier for the UserType_Level Variable.
        /// </summary>
        public const uint UserType_Level = 18176;

        /// <summary>
        /// The identifier for the UserType_Name Variable.
        /// </summary>
        public const uint UserType_Name = 18177;

        /// <summary>
        /// The identifier for the ExecutionModeEnumeration_EnumStrings Variable.
        /// </summary>
        public const uint ExecutionModeEnumeration_EnumStrings = 18192;

        /// <summary>
        /// The identifier for the MotionDeviceCategoryEnumeration_EnumStrings Variable.
        /// </summary>
        public const uint MotionDeviceCategoryEnumeration_EnumStrings = 18194;

        /// <summary>
        /// The identifier for the AxisMotionProfileEnumeration_EnumStrings Variable.
        /// </summary>
        public const uint AxisMotionProfileEnumeration_EnumStrings = 6027;

        /// <summary>
        /// The identifier for the OperationalModeEnumeration_EnumStrings Variable.
        /// </summary>
        public const uint OperationalModeEnumeration_EnumStrings = 6022;

        /// <summary>
        /// The identifier for the OpcUaRobotics_BinarySchema Variable.
        /// </summary>
        public const uint OpcUaRobotics_BinarySchema = 6110;

        /// <summary>
        /// The identifier for the OpcUaRobotics_BinarySchema_NamespaceUri Variable.
        /// </summary>
        public const uint OpcUaRobotics_BinarySchema_NamespaceUri = 6117;

        /// <summary>
        /// The identifier for the OpcUaRobotics_BinarySchema_Deprecated Variable.
        /// </summary>
        public const uint OpcUaRobotics_BinarySchema_Deprecated = 15196;

        /// <summary>
        /// The identifier for the OpcUaRobotics_XmlSchema Variable.
        /// </summary>
        public const uint OpcUaRobotics_XmlSchema = 6118;

        /// <summary>
        /// The identifier for the OpcUaRobotics_XmlSchema_NamespaceUri Variable.
        /// </summary>
        public const uint OpcUaRobotics_XmlSchema_NamespaceUri = 6170;

        /// <summary>
        /// The identifier for the OpcUaRobotics_XmlSchema_Deprecated Variable.
        /// </summary>
        public const uint OpcUaRobotics_XmlSchema_Deprecated = 15200;
    }
    #endregion

    #region DataType Node Identifiers
    /// <summary>
    /// A class that declares constants for all DataTypes in the Model Design.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class DataTypeIds
    {
        /// <summary>
        /// The identifier for the ExecutionModeEnumeration DataType.
        /// </summary>
        public static readonly ExpandedNodeId ExecutionModeEnumeration = new ExpandedNodeId(Opc.Ua.Robotics.DataTypes.ExecutionModeEnumeration, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceCategoryEnumeration DataType.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceCategoryEnumeration = new ExpandedNodeId(Opc.Ua.Robotics.DataTypes.MotionDeviceCategoryEnumeration, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisMotionProfileEnumeration DataType.
        /// </summary>
        public static readonly ExpandedNodeId AxisMotionProfileEnumeration = new ExpandedNodeId(Opc.Ua.Robotics.DataTypes.AxisMotionProfileEnumeration, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OperationalModeEnumeration DataType.
        /// </summary>
        public static readonly ExpandedNodeId OperationalModeEnumeration = new ExpandedNodeId(Opc.Ua.Robotics.DataTypes.OperationalModeEnumeration, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);
    }
    #endregion

    #region Method Node Identifiers
    /// <summary>
    /// A class that declares constants for all Methods in the Model Design.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class MethodIds
    {
        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Open Method.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Open = new ExpandedNodeId(Opc.Ua.Robotics.Methods.OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Open, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Close Method.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Close = new ExpandedNodeId(Opc.Ua.Robotics.Methods.OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Close, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Read Method.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Read = new ExpandedNodeId(Opc.Ua.Robotics.Methods.OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Read, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Write Method.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Write = new ExpandedNodeId(Opc.Ua.Robotics.Methods.OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Write, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_GetPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAROBOTICSNamespaceMetadata_NamespaceFile_GetPosition = new ExpandedNodeId(Opc.Ua.Robotics.Methods.OPCUAROBOTICSNamespaceMetadata_NamespaceFile_GetPosition, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_SetPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAROBOTICSNamespaceMetadata_NamespaceFile_SetPosition = new ExpandedNodeId(Opc.Ua.Robotics.Methods.OPCUAROBOTICSNamespaceMetadata_NamespaceFile_SetPosition, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceType_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceType_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceType_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceType_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceType_Axes_AxisIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceType_Axes_AxisIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceType_Axes_AxisIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceType_Axes_AxisIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.AxisType_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.AxisType_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.AxisType_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.AxisType_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_PowerTrainIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_PowerTrainIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.AxisType_PowerTrainIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_PowerTrainIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_PowerTrainIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.AxisType_PowerTrainIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_PowerTrainIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_PowerTrainIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.AxisType_PowerTrainIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_PowerTrainIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_PowerTrainIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.AxisType_PowerTrainIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.PowerTrainType_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.PowerTrainType_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.PowerTrainType_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.PowerTrainType_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_MotorIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.PowerTrainType_MotorIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_MotorIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.PowerTrainType_MotorIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_MotorIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.PowerTrainType_MotorIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_MotorIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.PowerTrainType_MotorIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_GearIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.PowerTrainType_GearIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_GearIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.PowerTrainType_GearIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_GearIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.PowerTrainType_GearIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_GearIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.PowerTrainType_GearIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.PowerTrainType_AxisIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.PowerTrainType_AxisIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.PowerTrainType_AxisIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.PowerTrainType_AxisIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_PowerTrainIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_PowerTrainIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.PowerTrainType_PowerTrainIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_PowerTrainIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_PowerTrainIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.PowerTrainType_PowerTrainIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_PowerTrainIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_PowerTrainIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.PowerTrainType_PowerTrainIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_PowerTrainIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_PowerTrainIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.PowerTrainType_PowerTrainIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotorType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotorType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotorType_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotorType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotorType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotorType_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotorType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotorType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotorType_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotorType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId MotorType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.MotorType_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the GearType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId GearType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.GearType_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the GearType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId GearType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.GearType_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the GearType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId GearType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.GearType_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the GearType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId GearType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.GearType_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the SafetyStateType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId SafetyStateType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.SafetyStateType_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the SafetyStateType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId SafetyStateType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.SafetyStateType_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the SafetyStateType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId SafetyStateType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.SafetyStateType_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the SafetyStateType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId SafetyStateType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.SafetyStateType_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Components_ComponentIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Components_ComponentIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_Components_ComponentIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Components_ComponentIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Components_ComponentIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_Components_ComponentIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Components_ComponentIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Components_ComponentIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_Components_ComponentIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Components_ComponentIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Components_ComponentIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_Components_ComponentIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Software_SoftwareIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_Software_SoftwareIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Software_SoftwareIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_Software_SoftwareIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Software_SoftwareIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_Software_SoftwareIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Software_SoftwareIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_Software_SoftwareIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_TaskControls_TaskControlIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_TaskControls_TaskControlIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_TaskControls_TaskControlIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_TaskControls_TaskControlIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_TaskControls_TaskControlIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_TaskControls_TaskControlIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_TaskControls_TaskControlIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_TaskControls_TaskControlIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_SafetyStatesIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_SafetyStatesIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_SafetyStatesIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_SafetyStatesIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_SafetyStatesIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_SafetyStatesIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_SafetyStatesIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_SafetyStatesIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_MotionDeviceIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_MotionDeviceIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_MotionDeviceIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_MotionDeviceIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId AuxiliaryComponentType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.AuxiliaryComponentType_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId AuxiliaryComponentType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.AuxiliaryComponentType_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId AuxiliaryComponentType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.AuxiliaryComponentType_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId AuxiliaryComponentType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.AuxiliaryComponentType_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId AuxiliaryComponentType_CPIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.AuxiliaryComponentType_CPIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId AuxiliaryComponentType_CPIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.AuxiliaryComponentType_CPIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId AuxiliaryComponentType_CPIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.AuxiliaryComponentType_CPIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId AuxiliaryComponentType_CPIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.AuxiliaryComponentType_CPIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the DriveType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId DriveType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.DriveType_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the DriveType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId DriveType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.DriveType_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the DriveType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId DriveType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.DriveType_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the DriveType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId DriveType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.DriveType_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the DriveType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId DriveType_CPIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.DriveType_CPIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the DriveType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId DriveType_CPIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.DriveType_CPIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the DriveType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId DriveType_CPIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.DriveType_CPIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the DriveType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId DriveType_CPIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.DriveType_CPIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.TaskControlType_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.TaskControlType_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.TaskControlType_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.TaskControlType_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.TaskControlType_MotionDeviceIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.TaskControlType_MotionDeviceIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.TaskControlType_MotionDeviceIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.TaskControlType_MotionDeviceIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Robotics.Methods.TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);
    }
    #endregion

    #region Object Node Identifiers
    /// <summary>
    /// A class that declares constants for all Objects in the Model Design.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class ObjectIds
    {
        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata Object.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAROBOTICSNamespaceMetadata = new ExpandedNodeId(Opc.Ua.Robotics.Objects.OPCUAROBOTICSNamespaceMetadata, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices Object.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices = new ExpandedNodeId(Opc.Ua.Robotics.Objects.MotionDeviceSystemType_MotionDevices, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier Object.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier = new ExpandedNodeId(Opc.Ua.Robotics.Objects.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_ParameterSet Object.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_ParameterSet = new ExpandedNodeId(Opc.Ua.Robotics.Objects.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_ParameterSet, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes Object.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes = new ExpandedNodeId(Opc.Ua.Robotics.Objects.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet Object.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet = new ExpandedNodeId(Opc.Ua.Robotics.Objects.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains Object.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains = new ExpandedNodeId(Opc.Ua.Robotics.Objects.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers Object.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers = new ExpandedNodeId(Opc.Ua.Robotics.Objects.MotionDeviceSystemType_Controllers, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier Object.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier = new ExpandedNodeId(Opc.Ua.Robotics.Objects.MotionDeviceSystemType_Controllers_ControllerIdentifier, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_CurrentUser Object.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_CurrentUser = new ExpandedNodeId(Opc.Ua.Robotics.Objects.MotionDeviceSystemType_Controllers_ControllerIdentifier_CurrentUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software Object.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Software = new ExpandedNodeId(Opc.Ua.Robotics.Objects.MotionDeviceSystemType_Controllers_ControllerIdentifier_Software, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls Object.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls = new ExpandedNodeId(Opc.Ua.Robotics.Objects.MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_ParameterSet Object.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_ParameterSet = new ExpandedNodeId(Opc.Ua.Robotics.Objects.MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_ParameterSet, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates Object.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_SafetyStates = new ExpandedNodeId(Opc.Ua.Robotics.Objects.MotionDeviceSystemType_SafetyStates, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier Object.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier = new ExpandedNodeId(Opc.Ua.Robotics.Objects.MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ParameterSet Object.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ParameterSet = new ExpandedNodeId(Opc.Ua.Robotics.Objects.MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ParameterSet, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_ParameterSet Object.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_ParameterSet = new ExpandedNodeId(Opc.Ua.Robotics.Objects.MotionDeviceType_ParameterSet, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes Object.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes = new ExpandedNodeId(Opc.Ua.Robotics.Objects.MotionDeviceType_Axes, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier Object.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier = new ExpandedNodeId(Opc.Ua.Robotics.Objects.MotionDeviceType_Axes_AxisIdentifier, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_ParameterSet Object.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_ParameterSet = new ExpandedNodeId(Opc.Ua.Robotics.Objects.MotionDeviceType_Axes_AxisIdentifier_ParameterSet, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_PowerTrains Object.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_PowerTrains = new ExpandedNodeId(Opc.Ua.Robotics.Objects.MotionDeviceType_PowerTrains, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_PowerTrains_PowerTrainIdentifier Object.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_PowerTrains_PowerTrainIdentifier = new ExpandedNodeId(Opc.Ua.Robotics.Objects.MotionDeviceType_PowerTrains_PowerTrainIdentifier, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_FlangeLoad Object.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_FlangeLoad = new ExpandedNodeId(Opc.Ua.Robotics.Objects.MotionDeviceType_FlangeLoad, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_AdditionalComponents Object.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_AdditionalComponents = new ExpandedNodeId(Opc.Ua.Robotics.Objects.MotionDeviceType_AdditionalComponents, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier Object.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier = new ExpandedNodeId(Opc.Ua.Robotics.Objects.MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_ParameterSet Object.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_ParameterSet = new ExpandedNodeId(Opc.Ua.Robotics.Objects.AxisType_ParameterSet, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_AdditionalLoad Object.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_AdditionalLoad = new ExpandedNodeId(Opc.Ua.Robotics.Objects.AxisType_AdditionalLoad, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_PowerTrainIdentifier Object.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_PowerTrainIdentifier = new ExpandedNodeId(Opc.Ua.Robotics.Objects.AxisType_PowerTrainIdentifier, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier Object.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_MotorIdentifier = new ExpandedNodeId(Opc.Ua.Robotics.Objects.PowerTrainType_MotorIdentifier, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_ParameterSet Object.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_MotorIdentifier_ParameterSet = new ExpandedNodeId(Opc.Ua.Robotics.Objects.PowerTrainType_MotorIdentifier_ParameterSet, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier Object.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_GearIdentifier = new ExpandedNodeId(Opc.Ua.Robotics.Objects.PowerTrainType_GearIdentifier, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier Object.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier = new ExpandedNodeId(Opc.Ua.Robotics.Objects.PowerTrainType_AxisIdentifier, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_ParameterSet Object.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_ParameterSet = new ExpandedNodeId(Opc.Ua.Robotics.Objects.PowerTrainType_AxisIdentifier_ParameterSet, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_PowerTrainIdentifier Object.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_PowerTrainIdentifier = new ExpandedNodeId(Opc.Ua.Robotics.Objects.PowerTrainType_PowerTrainIdentifier, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotorType_ParameterSet Object.
        /// </summary>
        public static readonly ExpandedNodeId MotorType_ParameterSet = new ExpandedNodeId(Opc.Ua.Robotics.Objects.MotorType_ParameterSet, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotorType_DriveIdentifier Object.
        /// </summary>
        public static readonly ExpandedNodeId MotorType_DriveIdentifier = new ExpandedNodeId(Opc.Ua.Robotics.Objects.MotorType_DriveIdentifier, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the SafetyStateType_ParameterSet Object.
        /// </summary>
        public static readonly ExpandedNodeId SafetyStateType_ParameterSet = new ExpandedNodeId(Opc.Ua.Robotics.Objects.SafetyStateType_ParameterSet, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the SafetyStateType_EmergencyStopFunctions Object.
        /// </summary>
        public static readonly ExpandedNodeId SafetyStateType_EmergencyStopFunctions = new ExpandedNodeId(Opc.Ua.Robotics.Objects.SafetyStateType_EmergencyStopFunctions, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the SafetyStateType_EmergencyStopFunctions_EmergencyStopFunctionIdentifier Object.
        /// </summary>
        public static readonly ExpandedNodeId SafetyStateType_EmergencyStopFunctions_EmergencyStopFunctionIdentifier = new ExpandedNodeId(Opc.Ua.Robotics.Objects.SafetyStateType_EmergencyStopFunctions_EmergencyStopFunctionIdentifier, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the SafetyStateType_ProtectiveStopFunctions Object.
        /// </summary>
        public static readonly ExpandedNodeId SafetyStateType_ProtectiveStopFunctions = new ExpandedNodeId(Opc.Ua.Robotics.Objects.SafetyStateType_ProtectiveStopFunctions, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the SafetyStateType_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier Object.
        /// </summary>
        public static readonly ExpandedNodeId SafetyStateType_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier = new ExpandedNodeId(Opc.Ua.Robotics.Objects.SafetyStateType_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_ParameterSet Object.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_ParameterSet = new ExpandedNodeId(Opc.Ua.Robotics.Objects.ControllerType_ParameterSet, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_CurrentUser Object.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_CurrentUser = new ExpandedNodeId(Opc.Ua.Robotics.Objects.ControllerType_CurrentUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Components Object.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Components = new ExpandedNodeId(Opc.Ua.Robotics.Objects.ControllerType_Components, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Components_ComponentIdentifier Object.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Components_ComponentIdentifier = new ExpandedNodeId(Opc.Ua.Robotics.Objects.ControllerType_Components_ComponentIdentifier, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Software Object.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Software = new ExpandedNodeId(Opc.Ua.Robotics.Objects.ControllerType_Software, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier Object.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Software_SoftwareIdentifier = new ExpandedNodeId(Opc.Ua.Robotics.Objects.ControllerType_Software_SoftwareIdentifier, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_TaskControls Object.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_TaskControls = new ExpandedNodeId(Opc.Ua.Robotics.Objects.ControllerType_TaskControls, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier Object.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_TaskControls_TaskControlIdentifier = new ExpandedNodeId(Opc.Ua.Robotics.Objects.ControllerType_TaskControls_TaskControlIdentifier, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_ParameterSet Object.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_TaskControls_TaskControlIdentifier_ParameterSet = new ExpandedNodeId(Opc.Ua.Robotics.Objects.ControllerType_TaskControls_TaskControlIdentifier_ParameterSet, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier Object.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_SafetyStatesIdentifier = new ExpandedNodeId(Opc.Ua.Robotics.Objects.ControllerType_SafetyStatesIdentifier, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_ParameterSet Object.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_SafetyStatesIdentifier_ParameterSet = new ExpandedNodeId(Opc.Ua.Robotics.Objects.ControllerType_SafetyStatesIdentifier_ParameterSet, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier Object.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier = new ExpandedNodeId(Opc.Ua.Robotics.Objects.ControllerType_MotionDeviceIdentifier, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_ParameterSet Object.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_ParameterSet = new ExpandedNodeId(Opc.Ua.Robotics.Objects.ControllerType_MotionDeviceIdentifier_ParameterSet, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes Object.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes = new ExpandedNodeId(Opc.Ua.Robotics.Objects.ControllerType_MotionDeviceIdentifier_Axes, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet Object.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet = new ExpandedNodeId(Opc.Ua.Robotics.Objects.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_PowerTrains Object.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_PowerTrains = new ExpandedNodeId(Opc.Ua.Robotics.Objects.ControllerType_MotionDeviceIdentifier_PowerTrains, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public static readonly ExpandedNodeId AuxiliaryComponentType_CPIdentifier_NetworkAddress = new ExpandedNodeId(Opc.Ua.Robotics.Objects.AuxiliaryComponentType_CPIdentifier_NetworkAddress, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the DriveType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public static readonly ExpandedNodeId DriveType_CPIdentifier_NetworkAddress = new ExpandedNodeId(Opc.Ua.Robotics.Objects.DriveType_CPIdentifier_NetworkAddress, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_ParameterSet Object.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_ParameterSet = new ExpandedNodeId(Opc.Ua.Robotics.Objects.TaskControlType_ParameterSet, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier Object.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier = new ExpandedNodeId(Opc.Ua.Robotics.Objects.TaskControlType_MotionDeviceIdentifier, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_ParameterSet Object.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_ParameterSet = new ExpandedNodeId(Opc.Ua.Robotics.Objects.TaskControlType_MotionDeviceIdentifier_ParameterSet, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes Object.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes = new ExpandedNodeId(Opc.Ua.Robotics.Objects.TaskControlType_MotionDeviceIdentifier_Axes, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet Object.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet = new ExpandedNodeId(Opc.Ua.Robotics.Objects.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_PowerTrains Object.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_PowerTrains = new ExpandedNodeId(Opc.Ua.Robotics.Objects.TaskControlType_MotionDeviceIdentifier_PowerTrains, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);
    }
    #endregion

    #region ObjectType Node Identifiers
    /// <summary>
    /// A class that declares constants for all ObjectTypes in the Model Design.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class ObjectTypeIds
    {
        /// <summary>
        /// The identifier for the MotionDeviceSystemType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType = new ExpandedNodeId(Opc.Ua.Robotics.ObjectTypes.MotionDeviceSystemType, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType = new ExpandedNodeId(Opc.Ua.Robotics.ObjectTypes.MotionDeviceType, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId AxisType = new ExpandedNodeId(Opc.Ua.Robotics.ObjectTypes.AxisType, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType = new ExpandedNodeId(Opc.Ua.Robotics.ObjectTypes.PowerTrainType, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotorType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId MotorType = new ExpandedNodeId(Opc.Ua.Robotics.ObjectTypes.MotorType, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the GearType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId GearType = new ExpandedNodeId(Opc.Ua.Robotics.ObjectTypes.GearType, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the SafetyStateType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId SafetyStateType = new ExpandedNodeId(Opc.Ua.Robotics.ObjectTypes.SafetyStateType, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the EmergencyStopFunctionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId EmergencyStopFunctionType = new ExpandedNodeId(Opc.Ua.Robotics.ObjectTypes.EmergencyStopFunctionType, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ProtectiveStopFunctionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ProtectiveStopFunctionType = new ExpandedNodeId(Opc.Ua.Robotics.ObjectTypes.ProtectiveStopFunctionType, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType = new ExpandedNodeId(Opc.Ua.Robotics.ObjectTypes.ControllerType, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AuxiliaryComponentType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId AuxiliaryComponentType = new ExpandedNodeId(Opc.Ua.Robotics.ObjectTypes.AuxiliaryComponentType, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the DriveType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId DriveType = new ExpandedNodeId(Opc.Ua.Robotics.ObjectTypes.DriveType, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType = new ExpandedNodeId(Opc.Ua.Robotics.ObjectTypes.TaskControlType, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the LoadType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId LoadType = new ExpandedNodeId(Opc.Ua.Robotics.ObjectTypes.LoadType, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the UserType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId UserType = new ExpandedNodeId(Opc.Ua.Robotics.ObjectTypes.UserType, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);
    }
    #endregion

    #region ReferenceType Node Identifiers
    /// <summary>
    /// A class that declares constants for all ReferenceTypes in the Model Design.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class ReferenceTypeIds
    {
        /// <summary>
        /// The identifier for the Controls ReferenceType.
        /// </summary>
        public static readonly ExpandedNodeId Controls = new ExpandedNodeId(Opc.Ua.Robotics.ReferenceTypes.Controls, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the Moves ReferenceType.
        /// </summary>
        public static readonly ExpandedNodeId Moves = new ExpandedNodeId(Opc.Ua.Robotics.ReferenceTypes.Moves, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the Requires ReferenceType.
        /// </summary>
        public static readonly ExpandedNodeId Requires = new ExpandedNodeId(Opc.Ua.Robotics.ReferenceTypes.Requires, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the IsDrivenBy ReferenceType.
        /// </summary>
        public static readonly ExpandedNodeId IsDrivenBy = new ExpandedNodeId(Opc.Ua.Robotics.ReferenceTypes.IsDrivenBy, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the IsConnectedTo ReferenceType.
        /// </summary>
        public static readonly ExpandedNodeId IsConnectedTo = new ExpandedNodeId(Opc.Ua.Robotics.ReferenceTypes.IsConnectedTo, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the HasSafetyStates ReferenceType.
        /// </summary>
        public static readonly ExpandedNodeId HasSafetyStates = new ExpandedNodeId(Opc.Ua.Robotics.ReferenceTypes.HasSafetyStates, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the HasSlave ReferenceType.
        /// </summary>
        public static readonly ExpandedNodeId HasSlave = new ExpandedNodeId(Opc.Ua.Robotics.ReferenceTypes.HasSlave, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);
    }
    #endregion

    #region Variable Node Identifiers
    /// <summary>
    /// A class that declares constants for all Variables in the Model Design.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class VariableIds
    {
        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAROBOTICSNamespaceMetadata_NamespaceUri = new ExpandedNodeId(Opc.Ua.Robotics.Variables.OPCUAROBOTICSNamespaceMetadata_NamespaceUri, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceVersion Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAROBOTICSNamespaceMetadata_NamespaceVersion = new ExpandedNodeId(Opc.Ua.Robotics.Variables.OPCUAROBOTICSNamespaceMetadata_NamespaceVersion, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespacePublicationDate Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAROBOTICSNamespaceMetadata_NamespacePublicationDate = new ExpandedNodeId(Opc.Ua.Robotics.Variables.OPCUAROBOTICSNamespaceMetadata_NamespacePublicationDate, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_IsNamespaceSubset Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAROBOTICSNamespaceMetadata_IsNamespaceSubset = new ExpandedNodeId(Opc.Ua.Robotics.Variables.OPCUAROBOTICSNamespaceMetadata_IsNamespaceSubset, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_StaticNodeIdTypes Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAROBOTICSNamespaceMetadata_StaticNodeIdTypes = new ExpandedNodeId(Opc.Ua.Robotics.Variables.OPCUAROBOTICSNamespaceMetadata_StaticNodeIdTypes, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_StaticNumericNodeIdRange Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAROBOTICSNamespaceMetadata_StaticNumericNodeIdRange = new ExpandedNodeId(Opc.Ua.Robotics.Variables.OPCUAROBOTICSNamespaceMetadata_StaticNumericNodeIdRange, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_StaticStringNodeIdPattern Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAROBOTICSNamespaceMetadata_StaticStringNodeIdPattern = new ExpandedNodeId(Opc.Ua.Robotics.Variables.OPCUAROBOTICSNamespaceMetadata_StaticStringNodeIdPattern, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Size Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Size = new ExpandedNodeId(Opc.Ua.Robotics.Variables.OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Size, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Writable Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Writable = new ExpandedNodeId(Opc.Ua.Robotics.Variables.OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Writable, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_UserWritable Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAROBOTICSNamespaceMetadata_NamespaceFile_UserWritable = new ExpandedNodeId(Opc.Ua.Robotics.Variables.OPCUAROBOTICSNamespaceMetadata_NamespaceFile_UserWritable, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_OpenCount Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAROBOTICSNamespaceMetadata_NamespaceFile_OpenCount = new ExpandedNodeId(Opc.Ua.Robotics.Variables.OPCUAROBOTICSNamespaceMetadata_NamespaceFile_OpenCount, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Open_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Open_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Open_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Open_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Open_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Open_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Close_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Close_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Close_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Read_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Read_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Read_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Read_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Read_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Read_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Write_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Write_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.OPCUAROBOTICSNamespaceMetadata_NamespaceFile_Write_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_GetPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAROBOTICSNamespaceMetadata_NamespaceFile_GetPosition_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.OPCUAROBOTICSNamespaceMetadata_NamespaceFile_GetPosition_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_GetPosition_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAROBOTICSNamespaceMetadata_NamespaceFile_GetPosition_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.OPCUAROBOTICSNamespaceMetadata_NamespaceFile_GetPosition_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_NamespaceFile_SetPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAROBOTICSNamespaceMetadata_NamespaceFile_SetPosition_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.OPCUAROBOTICSNamespaceMetadata_NamespaceFile_SetPosition_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_DefaultRolePermissions Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAROBOTICSNamespaceMetadata_DefaultRolePermissions = new ExpandedNodeId(Opc.Ua.Robotics.Variables.OPCUAROBOTICSNamespaceMetadata_DefaultRolePermissions, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_DefaultUserRolePermissions Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAROBOTICSNamespaceMetadata_DefaultUserRolePermissions = new ExpandedNodeId(Opc.Ua.Robotics.Variables.OPCUAROBOTICSNamespaceMetadata_DefaultUserRolePermissions, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OPCUAROBOTICSNamespaceMetadata_DefaultAccessRestrictions Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAROBOTICSNamespaceMetadata_DefaultAccessRestrictions = new ExpandedNodeId(Opc.Ua.Robotics.Variables.OPCUAROBOTICSNamespaceMetadata_DefaultAccessRestrictions, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Manufacturer Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Manufacturer = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Manufacturer, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Model Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Model = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Model, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_ProductCode Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_ProductCode = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_ProductCode, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_SerialNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_SerialNumber = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_SerialNumber, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_MotionDeviceCategory Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_MotionDeviceCategory = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_MotionDeviceCategory, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_ParameterSet_SpeedOverride Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_ParameterSet_SpeedOverride = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_ParameterSet_SpeedOverride, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_MotionProfile Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_MotionProfile = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_MotionProfile, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Mass Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Mass = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Mass, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Mass_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Mass_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Mass_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_X Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_X = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_X, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Y Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Y = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Y, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Z Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Z = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Z, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_A Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_A = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_A, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_B Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_B = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_B, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_C Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_C = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_C, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_X Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_X = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_X, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_Y Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_Y = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_Y, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_Z Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_Z = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_Z, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualPosition Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualPosition = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualPosition, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualPosition_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualPosition_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualPosition_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualSpeed_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualSpeed_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualSpeed_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualAcceleration_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualAcceleration_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualAcceleration_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_Mass Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_Mass = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_Mass, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_Mass_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_Mass_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_Mass_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_X Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_X = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_X, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_Y Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_Y = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_Y, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_Z Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_Z = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_Z, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_A Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_A = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_A, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_B Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_B = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_B, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_C Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_C = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_C, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_Inertia_X Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_Inertia_X = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_Inertia_X, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_Inertia_Y Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_Inertia_Y = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_Inertia_Y, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_Inertia_Z Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_Inertia_Z = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_FlangeLoad_Inertia_Z, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_MotionDevices_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Manufacturer Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Manufacturer = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Manufacturer, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Model Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Model = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Model, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_ProductCode Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_ProductCode = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_ProductCode, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_SerialNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_SerialNumber = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_SerialNumber, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_CurrentUser_Level Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_CurrentUser_Level = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_CurrentUser_Level, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Components_ComponentIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Manufacturer Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Manufacturer = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Manufacturer, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Model Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Model = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_Model, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_SoftwareRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_SoftwareRevision = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_Software_SoftwareIdentifier_SoftwareRevision, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_ComponentName Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_ComponentName = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_ComponentName, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_ParameterSet_TaskProgramName Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_ParameterSet_TaskProgramName = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_ParameterSet_TaskProgramName, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_ParameterSet_TaskProgramLoaded Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_ParameterSet_TaskProgramLoaded = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_TaskControls_TaskControlIdentifier_ParameterSet_TaskProgramLoaded, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_ParameterSet_TotalEnergyConsumption_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_ParameterSet_TotalEnergyConsumption_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_ParameterSet_TotalEnergyConsumption_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_ParameterSet_CabinetFanSpeed_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_ParameterSet_CabinetFanSpeed_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_ParameterSet_CabinetFanSpeed_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_ParameterSet_CPUFanSpeed_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_ParameterSet_CPUFanSpeed_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_ParameterSet_CPUFanSpeed_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_ParameterSet_InputVoltage_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_ParameterSet_InputVoltage_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_ParameterSet_InputVoltage_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_Controllers_ControllerIdentifier_ParameterSet_Temperature_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_Controllers_ControllerIdentifier_ParameterSet_Temperature_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_Controllers_ControllerIdentifier_ParameterSet_Temperature_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_EmergencyStopFunctions_EmergencyStopFunctionIdentifier_Name Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_EmergencyStopFunctions_EmergencyStopFunctionIdentifier_Name = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_EmergencyStopFunctions_EmergencyStopFunctionIdentifier_Name, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_EmergencyStopFunctions_EmergencyStopFunctionIdentifier_Active Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_EmergencyStopFunctions_EmergencyStopFunctionIdentifier_Active = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_EmergencyStopFunctions_EmergencyStopFunctionIdentifier_Active, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Name Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Name = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Name, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Enabled Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Enabled = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Enabled, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Active Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Active = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Active, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ParameterSet_OperationalMode Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ParameterSet_OperationalMode = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ParameterSet_OperationalMode, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ParameterSet_EmergencyStop Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ParameterSet_EmergencyStop = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ParameterSet_EmergencyStop, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ParameterSet_ProtectiveStop Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ParameterSet_ProtectiveStop = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceSystemType_SafetyStates_SafetyStateIdentifier_ParameterSet_ProtectiveStop, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Manufacturer Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Manufacturer = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Manufacturer, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Model Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Model = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Model, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_ProductCode Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_ProductCode = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_ProductCode, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_SerialNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_SerialNumber = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_SerialNumber, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_MotionDeviceCategory Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_MotionDeviceCategory = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_MotionDeviceCategory, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_ParameterSet_OnPath Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_ParameterSet_OnPath = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_ParameterSet_OnPath, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_ParameterSet_InControl Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_ParameterSet_InControl = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_ParameterSet_InControl, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_ParameterSet_SpeedOverride Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_ParameterSet_SpeedOverride = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_ParameterSet_SpeedOverride, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Axes_AxisIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Axes_AxisIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Axes_AxisIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Axes_AxisIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Axes_AxisIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Axes_AxisIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Axes_AxisIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Axes_AxisIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Axes_AxisIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_MotionProfile Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_MotionProfile = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Axes_AxisIdentifier_MotionProfile, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_Mass Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_Mass = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_Mass, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_Mass_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_Mass_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_Mass_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_X Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_X = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_X, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Y Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Y = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Y, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Z Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Z = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Z, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_A Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_A = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_A, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_B Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_B = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_B, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_C Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_C = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_C, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_Inertia_X Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_Inertia_X = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_Inertia_X, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_Inertia_Y Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_Inertia_Y = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_Inertia_Y, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_Inertia_Z Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_Inertia_Z = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Axes_AxisIdentifier_AdditionalLoad_Inertia_Z, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_ParameterSet_ActualPosition Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_ParameterSet_ActualPosition = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Axes_AxisIdentifier_ParameterSet_ActualPosition, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_ParameterSet_ActualPosition_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_ParameterSet_ActualPosition_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Axes_AxisIdentifier_ParameterSet_ActualPosition_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_ParameterSet_ActualSpeed_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_ParameterSet_ActualSpeed_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Axes_AxisIdentifier_ParameterSet_ActualSpeed_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_Axes_AxisIdentifier_ParameterSet_ActualAcceleration_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_Axes_AxisIdentifier_ParameterSet_ActualAcceleration_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_Axes_AxisIdentifier_ParameterSet_ActualAcceleration_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_PowerTrains_PowerTrainIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_FlangeLoad_Mass Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_FlangeLoad_Mass = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_FlangeLoad_Mass, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_FlangeLoad_Mass_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_FlangeLoad_Mass_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_FlangeLoad_Mass_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_FlangeLoad_CenterOfMass_CartesianCoordinates Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_FlangeLoad_CenterOfMass_CartesianCoordinates = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_FlangeLoad_CenterOfMass_CartesianCoordinates, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_FlangeLoad_CenterOfMass_Orientation Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_FlangeLoad_CenterOfMass_Orientation = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_FlangeLoad_CenterOfMass_Orientation, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_FlangeLoad_CenterOfMass_CartesianCoordinates_X Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_FlangeLoad_CenterOfMass_CartesianCoordinates_X = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_FlangeLoad_CenterOfMass_CartesianCoordinates_X, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_FlangeLoad_CenterOfMass_CartesianCoordinates_Y Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_FlangeLoad_CenterOfMass_CartesianCoordinates_Y = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_FlangeLoad_CenterOfMass_CartesianCoordinates_Y, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_FlangeLoad_CenterOfMass_CartesianCoordinates_Z Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_FlangeLoad_CenterOfMass_CartesianCoordinates_Z = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_FlangeLoad_CenterOfMass_CartesianCoordinates_Z, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_FlangeLoad_CenterOfMass_Orientation_A Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_FlangeLoad_CenterOfMass_Orientation_A = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_FlangeLoad_CenterOfMass_Orientation_A, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_FlangeLoad_CenterOfMass_Orientation_B Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_FlangeLoad_CenterOfMass_Orientation_B = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_FlangeLoad_CenterOfMass_Orientation_B, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_FlangeLoad_CenterOfMass_Orientation_C Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_FlangeLoad_CenterOfMass_Orientation_C = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_FlangeLoad_CenterOfMass_Orientation_C, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_FlangeLoad_Inertia_X Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_FlangeLoad_Inertia_X = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_FlangeLoad_Inertia_X, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_FlangeLoad_Inertia_Y Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_FlangeLoad_Inertia_Y = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_FlangeLoad_Inertia_Y, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_FlangeLoad_Inertia_Z Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_FlangeLoad_Inertia_Z = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_FlangeLoad_Inertia_Z, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceType_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_MotionProfile Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_MotionProfile = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_MotionProfile, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_AdditionalLoad_Mass Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_AdditionalLoad_Mass = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_AdditionalLoad_Mass, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_AdditionalLoad_Mass_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_AdditionalLoad_Mass_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_AdditionalLoad_Mass_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_AdditionalLoad_CenterOfMass_CartesianCoordinates Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_AdditionalLoad_CenterOfMass_CartesianCoordinates = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_AdditionalLoad_CenterOfMass_CartesianCoordinates, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_AdditionalLoad_CenterOfMass_Orientation Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_AdditionalLoad_CenterOfMass_Orientation = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_AdditionalLoad_CenterOfMass_Orientation, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_AdditionalLoad_CenterOfMass_CartesianCoordinates_X Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_AdditionalLoad_CenterOfMass_CartesianCoordinates_X = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_AdditionalLoad_CenterOfMass_CartesianCoordinates_X, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_AdditionalLoad_CenterOfMass_CartesianCoordinates_Y Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_AdditionalLoad_CenterOfMass_CartesianCoordinates_Y = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_AdditionalLoad_CenterOfMass_CartesianCoordinates_Y, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_AdditionalLoad_CenterOfMass_CartesianCoordinates_Z Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_AdditionalLoad_CenterOfMass_CartesianCoordinates_Z = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_AdditionalLoad_CenterOfMass_CartesianCoordinates_Z, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_AdditionalLoad_CenterOfMass_Orientation_A Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_AdditionalLoad_CenterOfMass_Orientation_A = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_AdditionalLoad_CenterOfMass_Orientation_A, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_AdditionalLoad_CenterOfMass_Orientation_B Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_AdditionalLoad_CenterOfMass_Orientation_B = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_AdditionalLoad_CenterOfMass_Orientation_B, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_AdditionalLoad_CenterOfMass_Orientation_C Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_AdditionalLoad_CenterOfMass_Orientation_C = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_AdditionalLoad_CenterOfMass_Orientation_C, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_AdditionalLoad_Inertia_X Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_AdditionalLoad_Inertia_X = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_AdditionalLoad_Inertia_X, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_AdditionalLoad_Inertia_Y Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_AdditionalLoad_Inertia_Y = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_AdditionalLoad_Inertia_Y, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_AdditionalLoad_Inertia_Z Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_AdditionalLoad_Inertia_Z = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_AdditionalLoad_Inertia_Z, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_ParameterSet_ActualPosition Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_ParameterSet_ActualPosition = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_ParameterSet_ActualPosition, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_ParameterSet_ActualPosition_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_ParameterSet_ActualPosition_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_ParameterSet_ActualPosition_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_ParameterSet_ActualSpeed Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_ParameterSet_ActualSpeed = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_ParameterSet_ActualSpeed, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_ParameterSet_ActualSpeed_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_ParameterSet_ActualSpeed_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_ParameterSet_ActualSpeed_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_ParameterSet_ActualAcceleration Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_ParameterSet_ActualAcceleration = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_ParameterSet_ActualAcceleration, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_ParameterSet_ActualAcceleration_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_ParameterSet_ActualAcceleration_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_ParameterSet_ActualAcceleration_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_PowerTrainIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_PowerTrainIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_PowerTrainIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_PowerTrainIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_PowerTrainIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_PowerTrainIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_PowerTrainIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_PowerTrainIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_PowerTrainIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_PowerTrainIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_PowerTrainIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_PowerTrainIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_PowerTrainIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_PowerTrainIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_PowerTrainIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_PowerTrainIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_PowerTrainIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_PowerTrainIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_PowerTrainIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_PowerTrainIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_PowerTrainIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_PowerTrainIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_PowerTrainIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_PowerTrainIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisType_PowerTrainIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisType_PowerTrainIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisType_PowerTrainIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_MotorIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_MotorIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_MotorIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_MotorIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_MotorIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_MotorIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_MotorIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_MotorIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_MotorIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_MotorIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_MotorIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_MotorIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_MotorIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_MotorIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_MotorIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_MotorIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_MotorIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_MotorIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_Manufacturer Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_MotorIdentifier_Manufacturer = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_MotorIdentifier_Manufacturer, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_Model Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_MotorIdentifier_Model = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_MotorIdentifier_Model, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_ProductCode Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_MotorIdentifier_ProductCode = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_MotorIdentifier_ProductCode, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_SerialNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_MotorIdentifier_SerialNumber = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_MotorIdentifier_SerialNumber, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_ParameterSet_MotorTemperature Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_MotorIdentifier_ParameterSet_MotorTemperature = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_MotorIdentifier_ParameterSet_MotorTemperature, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_MotorIdentifier_ParameterSet_MotorTemperature_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_MotorIdentifier_ParameterSet_MotorTemperature_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_MotorIdentifier_ParameterSet_MotorTemperature_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_GearIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_GearIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_GearIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_GearIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_GearIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_GearIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_GearIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_GearIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_GearIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_GearIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_GearIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_GearIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_GearIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_GearIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_GearIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_GearIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_GearIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_GearIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_Manufacturer Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_GearIdentifier_Manufacturer = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_GearIdentifier_Manufacturer, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_Model Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_GearIdentifier_Model = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_GearIdentifier_Model, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_ProductCode Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_GearIdentifier_ProductCode = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_GearIdentifier_ProductCode, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_SerialNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_GearIdentifier_SerialNumber = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_GearIdentifier_SerialNumber, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_GearRatio Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_GearIdentifier_GearRatio = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_GearIdentifier_GearRatio, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_GearRatio_Numerator Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_GearIdentifier_GearRatio_Numerator = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_GearIdentifier_GearRatio_Numerator, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_GearIdentifier_GearRatio_Denominator Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_GearIdentifier_GearRatio_Denominator = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_GearIdentifier_GearRatio_Denominator, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_AxisIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_AxisIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_AxisIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_AxisIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_AxisIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_AxisIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_AxisIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_AxisIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_AxisIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_MotionProfile Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_MotionProfile = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_AxisIdentifier_MotionProfile, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_AdditionalLoad_Mass Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_AdditionalLoad_Mass = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_AxisIdentifier_AdditionalLoad_Mass, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_AdditionalLoad_Mass_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_AdditionalLoad_Mass_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_AxisIdentifier_AdditionalLoad_Mass_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_X Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_X = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_X, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Y Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Y = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Y, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Z Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Z = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Z, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_A Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_A = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_A, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_B Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_B = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_B, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_C Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_C = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_C, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_AdditionalLoad_Inertia_X Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_AdditionalLoad_Inertia_X = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_AxisIdentifier_AdditionalLoad_Inertia_X, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_AdditionalLoad_Inertia_Y Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_AdditionalLoad_Inertia_Y = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_AxisIdentifier_AdditionalLoad_Inertia_Y, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_AdditionalLoad_Inertia_Z Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_AdditionalLoad_Inertia_Z = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_AxisIdentifier_AdditionalLoad_Inertia_Z, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_ParameterSet_ActualPosition Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_ParameterSet_ActualPosition = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_AxisIdentifier_ParameterSet_ActualPosition, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_ParameterSet_ActualPosition_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_ParameterSet_ActualPosition_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_AxisIdentifier_ParameterSet_ActualPosition_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_ParameterSet_ActualSpeed_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_ParameterSet_ActualSpeed_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_AxisIdentifier_ParameterSet_ActualSpeed_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_AxisIdentifier_ParameterSet_ActualAcceleration_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_AxisIdentifier_ParameterSet_ActualAcceleration_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_AxisIdentifier_ParameterSet_ActualAcceleration_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_PowerTrainIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_PowerTrainIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_PowerTrainIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_PowerTrainIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_PowerTrainIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_PowerTrainIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_PowerTrainIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_PowerTrainIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_PowerTrainIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_PowerTrainIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_PowerTrainIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_PowerTrainIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_PowerTrainIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_PowerTrainIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_PowerTrainIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_PowerTrainIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_PowerTrainIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_PowerTrainIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_PowerTrainIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_PowerTrainIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_PowerTrainIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_PowerTrainIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_PowerTrainIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_PowerTrainIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the PowerTrainType_PowerTrainIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerTrainType_PowerTrainIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.PowerTrainType_PowerTrainIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotorType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotorType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotorType_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotorType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotorType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotorType_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotorType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotorType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotorType_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotorType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotorType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotorType_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotorType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotorType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotorType_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotorType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotorType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotorType_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotorType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotorType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotorType_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotorType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotorType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotorType_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotorType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotorType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotorType_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotorType_Manufacturer Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotorType_Manufacturer = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotorType_Manufacturer, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotorType_Model Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotorType_Model = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotorType_Model, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotorType_ProductCode Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotorType_ProductCode = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotorType_ProductCode, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotorType_SerialNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotorType_SerialNumber = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotorType_SerialNumber, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotorType_ParameterSet_BrakeReleased Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotorType_ParameterSet_BrakeReleased = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotorType_ParameterSet_BrakeReleased, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotorType_ParameterSet_MotorTemperature Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotorType_ParameterSet_MotorTemperature = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotorType_ParameterSet_MotorTemperature, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotorType_ParameterSet_MotorTemperature_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotorType_ParameterSet_MotorTemperature_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotorType_ParameterSet_MotorTemperature_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotorType_ParameterSet_EffectiveLoadRate Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotorType_ParameterSet_EffectiveLoadRate = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotorType_ParameterSet_EffectiveLoadRate, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the GearType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId GearType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.GearType_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the GearType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId GearType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.GearType_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the GearType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId GearType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.GearType_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the GearType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId GearType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.GearType_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the GearType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId GearType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.GearType_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the GearType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId GearType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.GearType_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the GearType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId GearType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.GearType_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the GearType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId GearType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.GearType_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the GearType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId GearType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.GearType_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the GearType_Manufacturer Variable.
        /// </summary>
        public static readonly ExpandedNodeId GearType_Manufacturer = new ExpandedNodeId(Opc.Ua.Robotics.Variables.GearType_Manufacturer, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the GearType_Model Variable.
        /// </summary>
        public static readonly ExpandedNodeId GearType_Model = new ExpandedNodeId(Opc.Ua.Robotics.Variables.GearType_Model, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the GearType_ProductCode Variable.
        /// </summary>
        public static readonly ExpandedNodeId GearType_ProductCode = new ExpandedNodeId(Opc.Ua.Robotics.Variables.GearType_ProductCode, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the GearType_SerialNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId GearType_SerialNumber = new ExpandedNodeId(Opc.Ua.Robotics.Variables.GearType_SerialNumber, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the GearType_GearRatio Variable.
        /// </summary>
        public static readonly ExpandedNodeId GearType_GearRatio = new ExpandedNodeId(Opc.Ua.Robotics.Variables.GearType_GearRatio, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the GearType_GearRatio_Numerator Variable.
        /// </summary>
        public static readonly ExpandedNodeId GearType_GearRatio_Numerator = new ExpandedNodeId(Opc.Ua.Robotics.Variables.GearType_GearRatio_Numerator, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the GearType_GearRatio_Denominator Variable.
        /// </summary>
        public static readonly ExpandedNodeId GearType_GearRatio_Denominator = new ExpandedNodeId(Opc.Ua.Robotics.Variables.GearType_GearRatio_Denominator, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the GearType_Pitch Variable.
        /// </summary>
        public static readonly ExpandedNodeId GearType_Pitch = new ExpandedNodeId(Opc.Ua.Robotics.Variables.GearType_Pitch, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the SafetyStateType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId SafetyStateType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.SafetyStateType_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the SafetyStateType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId SafetyStateType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.SafetyStateType_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the SafetyStateType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId SafetyStateType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.SafetyStateType_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the SafetyStateType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId SafetyStateType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.SafetyStateType_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the SafetyStateType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SafetyStateType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.SafetyStateType_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the SafetyStateType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SafetyStateType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.SafetyStateType_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the SafetyStateType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SafetyStateType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.SafetyStateType_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the SafetyStateType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SafetyStateType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.SafetyStateType_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the SafetyStateType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SafetyStateType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.SafetyStateType_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the SafetyStateType_EmergencyStopFunctions_EmergencyStopFunctionIdentifier_Name Variable.
        /// </summary>
        public static readonly ExpandedNodeId SafetyStateType_EmergencyStopFunctions_EmergencyStopFunctionIdentifier_Name = new ExpandedNodeId(Opc.Ua.Robotics.Variables.SafetyStateType_EmergencyStopFunctions_EmergencyStopFunctionIdentifier_Name, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the SafetyStateType_EmergencyStopFunctions_EmergencyStopFunctionIdentifier_Active Variable.
        /// </summary>
        public static readonly ExpandedNodeId SafetyStateType_EmergencyStopFunctions_EmergencyStopFunctionIdentifier_Active = new ExpandedNodeId(Opc.Ua.Robotics.Variables.SafetyStateType_EmergencyStopFunctions_EmergencyStopFunctionIdentifier_Active, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the SafetyStateType_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Name Variable.
        /// </summary>
        public static readonly ExpandedNodeId SafetyStateType_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Name = new ExpandedNodeId(Opc.Ua.Robotics.Variables.SafetyStateType_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Name, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the SafetyStateType_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Enabled Variable.
        /// </summary>
        public static readonly ExpandedNodeId SafetyStateType_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Enabled = new ExpandedNodeId(Opc.Ua.Robotics.Variables.SafetyStateType_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Enabled, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the SafetyStateType_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Active Variable.
        /// </summary>
        public static readonly ExpandedNodeId SafetyStateType_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Active = new ExpandedNodeId(Opc.Ua.Robotics.Variables.SafetyStateType_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Active, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the SafetyStateType_ParameterSet_OperationalMode Variable.
        /// </summary>
        public static readonly ExpandedNodeId SafetyStateType_ParameterSet_OperationalMode = new ExpandedNodeId(Opc.Ua.Robotics.Variables.SafetyStateType_ParameterSet_OperationalMode, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the SafetyStateType_ParameterSet_EmergencyStop Variable.
        /// </summary>
        public static readonly ExpandedNodeId SafetyStateType_ParameterSet_EmergencyStop = new ExpandedNodeId(Opc.Ua.Robotics.Variables.SafetyStateType_ParameterSet_EmergencyStop, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the SafetyStateType_ParameterSet_ProtectiveStop Variable.
        /// </summary>
        public static readonly ExpandedNodeId SafetyStateType_ParameterSet_ProtectiveStop = new ExpandedNodeId(Opc.Ua.Robotics.Variables.SafetyStateType_ParameterSet_ProtectiveStop, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the EmergencyStopFunctionType_Name Variable.
        /// </summary>
        public static readonly ExpandedNodeId EmergencyStopFunctionType_Name = new ExpandedNodeId(Opc.Ua.Robotics.Variables.EmergencyStopFunctionType_Name, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the EmergencyStopFunctionType_Active Variable.
        /// </summary>
        public static readonly ExpandedNodeId EmergencyStopFunctionType_Active = new ExpandedNodeId(Opc.Ua.Robotics.Variables.EmergencyStopFunctionType_Active, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ProtectiveStopFunctionType_Name Variable.
        /// </summary>
        public static readonly ExpandedNodeId ProtectiveStopFunctionType_Name = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ProtectiveStopFunctionType_Name, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ProtectiveStopFunctionType_Enabled Variable.
        /// </summary>
        public static readonly ExpandedNodeId ProtectiveStopFunctionType_Enabled = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ProtectiveStopFunctionType_Enabled, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ProtectiveStopFunctionType_Active Variable.
        /// </summary>
        public static readonly ExpandedNodeId ProtectiveStopFunctionType_Active = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ProtectiveStopFunctionType_Active, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Manufacturer Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Manufacturer = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Manufacturer, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Model Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Model = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Model, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_ProductCode Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_ProductCode = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_ProductCode, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_SerialNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_SerialNumber = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_SerialNumber, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_CurrentUser_Level Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_CurrentUser_Level = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_CurrentUser_Level, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Components_ComponentIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Components_ComponentIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Components_ComponentIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Components_ComponentIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Components_ComponentIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Components_ComponentIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Components_ComponentIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Components_ComponentIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Components_ComponentIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Components_ComponentIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Components_ComponentIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Components_ComponentIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Components_ComponentIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Components_ComponentIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Components_ComponentIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Components_ComponentIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Components_ComponentIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Components_ComponentIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Components_ComponentIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Components_ComponentIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Components_ComponentIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Components_ComponentIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Components_ComponentIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Components_ComponentIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Components_ComponentIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Components_ComponentIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Components_ComponentIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Software_SoftwareIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Software_SoftwareIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Software_SoftwareIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Software_SoftwareIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Software_SoftwareIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Software_SoftwareIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Software_SoftwareIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Software_SoftwareIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Software_SoftwareIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Software_SoftwareIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Software_SoftwareIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Software_SoftwareIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Software_SoftwareIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Software_SoftwareIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Software_SoftwareIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Software_SoftwareIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Software_SoftwareIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Software_SoftwareIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_Manufacturer Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Software_SoftwareIdentifier_Manufacturer = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Software_SoftwareIdentifier_Manufacturer, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_Model Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Software_SoftwareIdentifier_Model = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Software_SoftwareIdentifier_Model, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_Software_SoftwareIdentifier_SoftwareRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_Software_SoftwareIdentifier_SoftwareRevision = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_Software_SoftwareIdentifier_SoftwareRevision, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_TaskControls_TaskControlIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_TaskControls_TaskControlIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_TaskControls_TaskControlIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_TaskControls_TaskControlIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_TaskControls_TaskControlIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_TaskControls_TaskControlIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_TaskControls_TaskControlIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_TaskControls_TaskControlIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_TaskControls_TaskControlIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_TaskControls_TaskControlIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_TaskControls_TaskControlIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_TaskControls_TaskControlIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_TaskControls_TaskControlIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_TaskControls_TaskControlIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_TaskControls_TaskControlIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_TaskControls_TaskControlIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_TaskControls_TaskControlIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_TaskControls_TaskControlIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_ComponentName Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_TaskControls_TaskControlIdentifier_ComponentName = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_TaskControls_TaskControlIdentifier_ComponentName, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_ParameterSet_TaskProgramName Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_TaskControls_TaskControlIdentifier_ParameterSet_TaskProgramName = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_TaskControls_TaskControlIdentifier_ParameterSet_TaskProgramName, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_TaskControls_TaskControlIdentifier_ParameterSet_TaskProgramLoaded Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_TaskControls_TaskControlIdentifier_ParameterSet_TaskProgramLoaded = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_TaskControls_TaskControlIdentifier_ParameterSet_TaskProgramLoaded, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_ParameterSet_TotalPowerOnTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_ParameterSet_TotalPowerOnTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_ParameterSet_TotalPowerOnTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_ParameterSet_StartUpTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_ParameterSet_StartUpTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_ParameterSet_StartUpTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_ParameterSet_UpsState Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_ParameterSet_UpsState = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_ParameterSet_UpsState, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_ParameterSet_TotalEnergyConsumption Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_ParameterSet_TotalEnergyConsumption = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_ParameterSet_TotalEnergyConsumption, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_ParameterSet_TotalEnergyConsumption_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_ParameterSet_TotalEnergyConsumption_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_ParameterSet_TotalEnergyConsumption_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_ParameterSet_CabinetFanSpeed Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_ParameterSet_CabinetFanSpeed = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_ParameterSet_CabinetFanSpeed, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_ParameterSet_CabinetFanSpeed_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_ParameterSet_CabinetFanSpeed_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_ParameterSet_CabinetFanSpeed_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_ParameterSet_CPUFanSpeed Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_ParameterSet_CPUFanSpeed = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_ParameterSet_CPUFanSpeed, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_ParameterSet_CPUFanSpeed_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_ParameterSet_CPUFanSpeed_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_ParameterSet_CPUFanSpeed_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_ParameterSet_InputVoltage Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_ParameterSet_InputVoltage = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_ParameterSet_InputVoltage, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_ParameterSet_InputVoltage_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_ParameterSet_InputVoltage_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_ParameterSet_InputVoltage_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_ParameterSet_Temperature Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_ParameterSet_Temperature = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_ParameterSet_Temperature, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_ParameterSet_Temperature_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_ParameterSet_Temperature_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_ParameterSet_Temperature_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_SafetyStatesIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_SafetyStatesIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_SafetyStatesIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_SafetyStatesIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_SafetyStatesIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_SafetyStatesIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_SafetyStatesIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_SafetyStatesIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_SafetyStatesIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_SafetyStatesIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_SafetyStatesIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_SafetyStatesIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_SafetyStatesIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_SafetyStatesIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_SafetyStatesIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_SafetyStatesIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_SafetyStatesIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_SafetyStatesIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_EmergencyStopFunctions_EmergencyStopFunctionIdentifier_Name Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_SafetyStatesIdentifier_EmergencyStopFunctions_EmergencyStopFunctionIdentifier_Name = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_SafetyStatesIdentifier_EmergencyStopFunctions_EmergencyStopFunctionIdentifier_Name, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_EmergencyStopFunctions_EmergencyStopFunctionIdentifier_Active Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_SafetyStatesIdentifier_EmergencyStopFunctions_EmergencyStopFunctionIdentifier_Active = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_SafetyStatesIdentifier_EmergencyStopFunctions_EmergencyStopFunctionIdentifier_Active, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Name Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_SafetyStatesIdentifier_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Name = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_SafetyStatesIdentifier_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Name, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Enabled Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_SafetyStatesIdentifier_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Enabled = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_SafetyStatesIdentifier_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Enabled, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Active Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_SafetyStatesIdentifier_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Active = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_SafetyStatesIdentifier_ProtectiveStopFunctions_ProtectiveStopFunctionIdentifier_Active, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_ParameterSet_OperationalMode Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_SafetyStatesIdentifier_ParameterSet_OperationalMode = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_SafetyStatesIdentifier_ParameterSet_OperationalMode, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_ParameterSet_EmergencyStop Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_SafetyStatesIdentifier_ParameterSet_EmergencyStop = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_SafetyStatesIdentifier_ParameterSet_EmergencyStop, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_SafetyStatesIdentifier_ParameterSet_ProtectiveStop Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_SafetyStatesIdentifier_ParameterSet_ProtectiveStop = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_SafetyStatesIdentifier_ParameterSet_ProtectiveStop, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Manufacturer Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Manufacturer = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Manufacturer, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Model Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Model = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Model, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_ProductCode Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_ProductCode = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_ProductCode, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_SerialNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_SerialNumber = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_SerialNumber, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_MotionDeviceCategory Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_MotionDeviceCategory = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_MotionDeviceCategory, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_ParameterSet_SpeedOverride Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_ParameterSet_SpeedOverride = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_ParameterSet_SpeedOverride, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_MotionProfile Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_MotionProfile = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_MotionProfile, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Mass Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Mass = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Mass, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Mass_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Mass_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Mass_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_X Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_X = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_X, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Y Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Y = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Y, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Z Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Z = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Z, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_A Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_A = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_A, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_B Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_B = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_B, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_C Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_C = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_C, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_X Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_X = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_X, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_Y Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_Y = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_Y, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_Z Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_Z = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_Z, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualPosition Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualPosition = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualPosition, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualPosition_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualPosition_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualPosition_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualSpeed_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualSpeed_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualSpeed_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualAcceleration_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualAcceleration_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualAcceleration_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_FlangeLoad_Mass Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_FlangeLoad_Mass = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_FlangeLoad_Mass, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_FlangeLoad_Mass_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_FlangeLoad_Mass_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_FlangeLoad_Mass_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_X Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_X = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_X, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_Y Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_Y = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_Y, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_Z Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_Z = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_Z, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_A Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_A = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_A, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_B Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_B = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_B, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_C Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_C = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_C, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_FlangeLoad_Inertia_X Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_FlangeLoad_Inertia_X = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_FlangeLoad_Inertia_X, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_FlangeLoad_Inertia_Y Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_FlangeLoad_Inertia_Y = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_FlangeLoad_Inertia_Y, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_FlangeLoad_Inertia_Z Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_FlangeLoad_Inertia_Z = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_FlangeLoad_Inertia_Z, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ControllerType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId AuxiliaryComponentType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AuxiliaryComponentType_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId AuxiliaryComponentType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AuxiliaryComponentType_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId AuxiliaryComponentType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AuxiliaryComponentType_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId AuxiliaryComponentType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AuxiliaryComponentType_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AuxiliaryComponentType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AuxiliaryComponentType_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AuxiliaryComponentType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AuxiliaryComponentType_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AuxiliaryComponentType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AuxiliaryComponentType_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AuxiliaryComponentType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AuxiliaryComponentType_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AuxiliaryComponentType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AuxiliaryComponentType_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_ProductCode Variable.
        /// </summary>
        public static readonly ExpandedNodeId AuxiliaryComponentType_ProductCode = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AuxiliaryComponentType_ProductCode, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId AuxiliaryComponentType_CPIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AuxiliaryComponentType_CPIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId AuxiliaryComponentType_CPIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AuxiliaryComponentType_CPIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId AuxiliaryComponentType_CPIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AuxiliaryComponentType_CPIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId AuxiliaryComponentType_CPIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AuxiliaryComponentType_CPIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AuxiliaryComponentType_CPIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AuxiliaryComponentType_CPIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AuxiliaryComponentType_CPIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AuxiliaryComponentType_CPIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AuxiliaryComponentType_CPIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AuxiliaryComponentType_CPIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AuxiliaryComponentType_CPIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AuxiliaryComponentType_CPIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AuxiliaryComponentType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AuxiliaryComponentType_CPIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AuxiliaryComponentType_CPIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the DriveType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId DriveType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.DriveType_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the DriveType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId DriveType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.DriveType_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the DriveType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId DriveType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.DriveType_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the DriveType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId DriveType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.DriveType_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the DriveType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DriveType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.DriveType_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the DriveType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DriveType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.DriveType_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the DriveType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DriveType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.DriveType_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the DriveType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DriveType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.DriveType_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the DriveType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DriveType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.DriveType_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the DriveType_ProductCode Variable.
        /// </summary>
        public static readonly ExpandedNodeId DriveType_ProductCode = new ExpandedNodeId(Opc.Ua.Robotics.Variables.DriveType_ProductCode, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the DriveType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId DriveType_CPIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.DriveType_CPIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the DriveType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId DriveType_CPIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.DriveType_CPIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the DriveType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId DriveType_CPIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.DriveType_CPIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the DriveType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId DriveType_CPIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.DriveType_CPIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the DriveType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DriveType_CPIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.DriveType_CPIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the DriveType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DriveType_CPIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.DriveType_CPIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the DriveType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DriveType_CPIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.DriveType_CPIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the DriveType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DriveType_CPIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.DriveType_CPIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the DriveType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DriveType_CPIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.DriveType_CPIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_ComponentName Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_ComponentName = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_ComponentName, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_ParameterSet_TaskProgramName Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_ParameterSet_TaskProgramName = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_ParameterSet_TaskProgramName, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_ParameterSet_TaskProgramLoaded Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_ParameterSet_TaskProgramLoaded = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_ParameterSet_TaskProgramLoaded, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_ParameterSet_ExecutionMode Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_ParameterSet_ExecutionMode = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_ParameterSet_ExecutionMode, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Manufacturer Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Manufacturer = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Manufacturer, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Model Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Model = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Model, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_ProductCode Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_ProductCode = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_ProductCode, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_SerialNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_SerialNumber = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_SerialNumber, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_MotionDeviceCategory Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_MotionDeviceCategory = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_MotionDeviceCategory, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_ParameterSet_SpeedOverride Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_ParameterSet_SpeedOverride = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_ParameterSet_SpeedOverride, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_MotionProfile Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_MotionProfile = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_MotionProfile, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Mass Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Mass = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Mass, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Mass_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Mass_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Mass_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_X Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_X = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_X, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Y Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Y = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Y, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Z Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Z = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_CartesianCoordinates_Z, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_A Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_A = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_A, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_B Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_B = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_B, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_C Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_C = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_CenterOfMass_Orientation_C, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_X Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_X = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_X, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_Y Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_Y = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_Y, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_Z Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_Z = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_AdditionalLoad_Inertia_Z, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualPosition Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualPosition = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualPosition, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualPosition_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualPosition_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualPosition_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualSpeed_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualSpeed_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualSpeed_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualAcceleration_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualAcceleration_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_Axes_AxisIdentifier_ParameterSet_ActualAcceleration_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_PowerTrains_PowerTrainIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_FlangeLoad_Mass Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_FlangeLoad_Mass = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_FlangeLoad_Mass, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_FlangeLoad_Mass_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_FlangeLoad_Mass_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_FlangeLoad_Mass_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_X Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_X = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_X, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_Y Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_Y = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_Y, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_Z Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_Z = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_CartesianCoordinates_Z, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_A Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_A = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_A, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_B Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_B = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_B, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_C Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_C = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_FlangeLoad_CenterOfMass_Orientation_C, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_FlangeLoad_Inertia_X Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_FlangeLoad_Inertia_X = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_FlangeLoad_Inertia_X, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_FlangeLoad_Inertia_Y Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_FlangeLoad_Inertia_Y = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_FlangeLoad_Inertia_Y, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_FlangeLoad_Inertia_Z Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_FlangeLoad_Inertia_Z = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_FlangeLoad_Inertia_Z, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_Locked, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingClient, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_LockingUser, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RemainingLockTime, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Robotics.Variables.TaskControlType_MotionDeviceIdentifier_AdditionalComponents_AdditionalComponentIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the LoadType_Mass Variable.
        /// </summary>
        public static readonly ExpandedNodeId LoadType_Mass = new ExpandedNodeId(Opc.Ua.Robotics.Variables.LoadType_Mass, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the LoadType_Mass_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId LoadType_Mass_EngineeringUnits = new ExpandedNodeId(Opc.Ua.Robotics.Variables.LoadType_Mass_EngineeringUnits, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the LoadType_CenterOfMass Variable.
        /// </summary>
        public static readonly ExpandedNodeId LoadType_CenterOfMass = new ExpandedNodeId(Opc.Ua.Robotics.Variables.LoadType_CenterOfMass, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the LoadType_CenterOfMass_CartesianCoordinates Variable.
        /// </summary>
        public static readonly ExpandedNodeId LoadType_CenterOfMass_CartesianCoordinates = new ExpandedNodeId(Opc.Ua.Robotics.Variables.LoadType_CenterOfMass_CartesianCoordinates, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the LoadType_CenterOfMass_Orientation Variable.
        /// </summary>
        public static readonly ExpandedNodeId LoadType_CenterOfMass_Orientation = new ExpandedNodeId(Opc.Ua.Robotics.Variables.LoadType_CenterOfMass_Orientation, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the LoadType_CenterOfMass_CartesianCoordinates_X Variable.
        /// </summary>
        public static readonly ExpandedNodeId LoadType_CenterOfMass_CartesianCoordinates_X = new ExpandedNodeId(Opc.Ua.Robotics.Variables.LoadType_CenterOfMass_CartesianCoordinates_X, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the LoadType_CenterOfMass_CartesianCoordinates_Y Variable.
        /// </summary>
        public static readonly ExpandedNodeId LoadType_CenterOfMass_CartesianCoordinates_Y = new ExpandedNodeId(Opc.Ua.Robotics.Variables.LoadType_CenterOfMass_CartesianCoordinates_Y, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the LoadType_CenterOfMass_CartesianCoordinates_Z Variable.
        /// </summary>
        public static readonly ExpandedNodeId LoadType_CenterOfMass_CartesianCoordinates_Z = new ExpandedNodeId(Opc.Ua.Robotics.Variables.LoadType_CenterOfMass_CartesianCoordinates_Z, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the LoadType_CenterOfMass_Orientation_A Variable.
        /// </summary>
        public static readonly ExpandedNodeId LoadType_CenterOfMass_Orientation_A = new ExpandedNodeId(Opc.Ua.Robotics.Variables.LoadType_CenterOfMass_Orientation_A, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the LoadType_CenterOfMass_Orientation_B Variable.
        /// </summary>
        public static readonly ExpandedNodeId LoadType_CenterOfMass_Orientation_B = new ExpandedNodeId(Opc.Ua.Robotics.Variables.LoadType_CenterOfMass_Orientation_B, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the LoadType_CenterOfMass_Orientation_C Variable.
        /// </summary>
        public static readonly ExpandedNodeId LoadType_CenterOfMass_Orientation_C = new ExpandedNodeId(Opc.Ua.Robotics.Variables.LoadType_CenterOfMass_Orientation_C, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the LoadType_Inertia Variable.
        /// </summary>
        public static readonly ExpandedNodeId LoadType_Inertia = new ExpandedNodeId(Opc.Ua.Robotics.Variables.LoadType_Inertia, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the LoadType_Inertia_X Variable.
        /// </summary>
        public static readonly ExpandedNodeId LoadType_Inertia_X = new ExpandedNodeId(Opc.Ua.Robotics.Variables.LoadType_Inertia_X, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the LoadType_Inertia_Y Variable.
        /// </summary>
        public static readonly ExpandedNodeId LoadType_Inertia_Y = new ExpandedNodeId(Opc.Ua.Robotics.Variables.LoadType_Inertia_Y, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the LoadType_Inertia_Z Variable.
        /// </summary>
        public static readonly ExpandedNodeId LoadType_Inertia_Z = new ExpandedNodeId(Opc.Ua.Robotics.Variables.LoadType_Inertia_Z, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the UserType_Level Variable.
        /// </summary>
        public static readonly ExpandedNodeId UserType_Level = new ExpandedNodeId(Opc.Ua.Robotics.Variables.UserType_Level, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the UserType_Name Variable.
        /// </summary>
        public static readonly ExpandedNodeId UserType_Name = new ExpandedNodeId(Opc.Ua.Robotics.Variables.UserType_Name, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the ExecutionModeEnumeration_EnumStrings Variable.
        /// </summary>
        public static readonly ExpandedNodeId ExecutionModeEnumeration_EnumStrings = new ExpandedNodeId(Opc.Ua.Robotics.Variables.ExecutionModeEnumeration_EnumStrings, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the MotionDeviceCategoryEnumeration_EnumStrings Variable.
        /// </summary>
        public static readonly ExpandedNodeId MotionDeviceCategoryEnumeration_EnumStrings = new ExpandedNodeId(Opc.Ua.Robotics.Variables.MotionDeviceCategoryEnumeration_EnumStrings, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the AxisMotionProfileEnumeration_EnumStrings Variable.
        /// </summary>
        public static readonly ExpandedNodeId AxisMotionProfileEnumeration_EnumStrings = new ExpandedNodeId(Opc.Ua.Robotics.Variables.AxisMotionProfileEnumeration_EnumStrings, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OperationalModeEnumeration_EnumStrings Variable.
        /// </summary>
        public static readonly ExpandedNodeId OperationalModeEnumeration_EnumStrings = new ExpandedNodeId(Opc.Ua.Robotics.Variables.OperationalModeEnumeration_EnumStrings, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OpcUaRobotics_BinarySchema Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaRobotics_BinarySchema = new ExpandedNodeId(Opc.Ua.Robotics.Variables.OpcUaRobotics_BinarySchema, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OpcUaRobotics_BinarySchema_NamespaceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaRobotics_BinarySchema_NamespaceUri = new ExpandedNodeId(Opc.Ua.Robotics.Variables.OpcUaRobotics_BinarySchema_NamespaceUri, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OpcUaRobotics_BinarySchema_Deprecated Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaRobotics_BinarySchema_Deprecated = new ExpandedNodeId(Opc.Ua.Robotics.Variables.OpcUaRobotics_BinarySchema_Deprecated, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OpcUaRobotics_XmlSchema Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaRobotics_XmlSchema = new ExpandedNodeId(Opc.Ua.Robotics.Variables.OpcUaRobotics_XmlSchema, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OpcUaRobotics_XmlSchema_NamespaceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaRobotics_XmlSchema_NamespaceUri = new ExpandedNodeId(Opc.Ua.Robotics.Variables.OpcUaRobotics_XmlSchema_NamespaceUri, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);

        /// <summary>
        /// The identifier for the OpcUaRobotics_XmlSchema_Deprecated Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaRobotics_XmlSchema_Deprecated = new ExpandedNodeId(Opc.Ua.Robotics.Variables.OpcUaRobotics_XmlSchema_Deprecated, Opc.Ua.Robotics.Namespaces.OpcUaRobotics);
    }
    #endregion

    #region BrowseName Declarations
    /// <summary>
    /// Declares all of the BrowseNames used in the Model Design.
    /// </summary>
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class BrowseNames
    {
        /// <summary>
        /// The BrowseName for the Active component.
        /// </summary>
        public const string Active = "Active";

        /// <summary>
        /// The BrowseName for the AdditionalComponents component.
        /// </summary>
        public const string AdditionalComponents = "AdditionalComponents";

        /// <summary>
        /// The BrowseName for the AdditionalLoad component.
        /// </summary>
        public const string AdditionalLoad = "AdditionalLoad";

        /// <summary>
        /// The BrowseName for the AuxiliaryComponentType component.
        /// </summary>
        public const string AuxiliaryComponentType = "AuxiliaryComponentType";

        /// <summary>
        /// The BrowseName for the Axes component.
        /// </summary>
        public const string Axes = "Axes";

        /// <summary>
        /// The BrowseName for the AxisIdentifier component.
        /// </summary>
        public const string AxisIdentifier = "<AxisIdentifier>";

        /// <summary>
        /// The BrowseName for the AxisMotionProfileEnumeration component.
        /// </summary>
        public const string AxisMotionProfileEnumeration = "AxisMotionProfileEnumeration";

        /// <summary>
        /// The BrowseName for the AxisType component.
        /// </summary>
        public const string AxisType = "AxisType";

        /// <summary>
        /// The BrowseName for the CenterOfMass component.
        /// </summary>
        public const string CenterOfMass = "CenterOfMass";

        /// <summary>
        /// The BrowseName for the Components component.
        /// </summary>
        public const string Components = "Components";

        /// <summary>
        /// The BrowseName for the Controllers component.
        /// </summary>
        public const string Controllers = "Controllers";

        /// <summary>
        /// The BrowseName for the ControllerType component.
        /// </summary>
        public const string ControllerType = "ControllerType";

        /// <summary>
        /// The BrowseName for the Controls component.
        /// </summary>
        public const string Controls = "Controls";

        /// <summary>
        /// The BrowseName for the CurrentUser component.
        /// </summary>
        public const string CurrentUser = "CurrentUser";

        /// <summary>
        /// The BrowseName for the DriveIdentifier component.
        /// </summary>
        public const string DriveIdentifier = "<DriveIdentifier>";

        /// <summary>
        /// The BrowseName for the DriveType component.
        /// </summary>
        public const string DriveType = "DriveType";

        /// <summary>
        /// The BrowseName for the EmergencyStopFunctions component.
        /// </summary>
        public const string EmergencyStopFunctions = "EmergencyStopFunctions";

        /// <summary>
        /// The BrowseName for the EmergencyStopFunctionType component.
        /// </summary>
        public const string EmergencyStopFunctionType = "EmergencyStopFunctionType";

        /// <summary>
        /// The BrowseName for the Enabled component.
        /// </summary>
        public const string Enabled = "Enabled";

        /// <summary>
        /// The BrowseName for the ExecutionModeEnumeration component.
        /// </summary>
        public const string ExecutionModeEnumeration = "ExecutionModeEnumeration";

        /// <summary>
        /// The BrowseName for the FlangeLoad component.
        /// </summary>
        public const string FlangeLoad = "FlangeLoad";

        /// <summary>
        /// The BrowseName for the GearIdentifier component.
        /// </summary>
        public const string GearIdentifier = "<GearIdentifier>";

        /// <summary>
        /// The BrowseName for the GearRatio component.
        /// </summary>
        public const string GearRatio = "GearRatio";

        /// <summary>
        /// The BrowseName for the GearType component.
        /// </summary>
        public const string GearType = "GearType";

        /// <summary>
        /// The BrowseName for the HasSafetyStates component.
        /// </summary>
        public const string HasSafetyStates = "HasSafetyStates";

        /// <summary>
        /// The BrowseName for the HasSlave component.
        /// </summary>
        public const string HasSlave = "HasSlave";

        /// <summary>
        /// The BrowseName for the Inertia component.
        /// </summary>
        public const string Inertia = "Inertia";

        /// <summary>
        /// The BrowseName for the IsConnectedTo component.
        /// </summary>
        public const string IsConnectedTo = "IsConnectedTo";

        /// <summary>
        /// The BrowseName for the IsDrivenBy component.
        /// </summary>
        public const string IsDrivenBy = "IsDrivenBy";

        /// <summary>
        /// The BrowseName for the Level component.
        /// </summary>
        public const string Level = "Level";

        /// <summary>
        /// The BrowseName for the LoadType component.
        /// </summary>
        public const string LoadType = "LoadType";

        /// <summary>
        /// The BrowseName for the Mass component.
        /// </summary>
        public const string Mass = "Mass";

        /// <summary>
        /// The BrowseName for the MotionDeviceCategory component.
        /// </summary>
        public const string MotionDeviceCategory = "MotionDeviceCategory";

        /// <summary>
        /// The BrowseName for the MotionDeviceCategoryEnumeration component.
        /// </summary>
        public const string MotionDeviceCategoryEnumeration = "MotionDeviceCategoryEnumeration";

        /// <summary>
        /// The BrowseName for the MotionDeviceIdentifier component.
        /// </summary>
        public const string MotionDeviceIdentifier = "<MotionDeviceIdentifier>";

        /// <summary>
        /// The BrowseName for the MotionDevices component.
        /// </summary>
        public const string MotionDevices = "MotionDevices";

        /// <summary>
        /// The BrowseName for the MotionDeviceSystemType component.
        /// </summary>
        public const string MotionDeviceSystemType = "MotionDeviceSystemType";

        /// <summary>
        /// The BrowseName for the MotionDeviceType component.
        /// </summary>
        public const string MotionDeviceType = "MotionDeviceType";

        /// <summary>
        /// The BrowseName for the MotionProfile component.
        /// </summary>
        public const string MotionProfile = "MotionProfile";

        /// <summary>
        /// The BrowseName for the MotorIdentifier component.
        /// </summary>
        public const string MotorIdentifier = "<MotorIdentifier>";

        /// <summary>
        /// The BrowseName for the MotorType component.
        /// </summary>
        public const string MotorType = "MotorType";

        /// <summary>
        /// The BrowseName for the Moves component.
        /// </summary>
        public const string Moves = "Moves";

        /// <summary>
        /// The BrowseName for the Name component.
        /// </summary>
        public const string Name = "Name";

        /// <summary>
        /// The BrowseName for the OpcUaRobotics_BinarySchema component.
        /// </summary>
        public const string OpcUaRobotics_BinarySchema = "Opc.Ua.Robotics";

        /// <summary>
        /// The BrowseName for the OpcUaRobotics_XmlSchema component.
        /// </summary>
        public const string OpcUaRobotics_XmlSchema = "Opc.Ua.Robotics";

        /// <summary>
        /// The BrowseName for the OPCUAROBOTICSNamespaceMetadata component.
        /// </summary>
        public const string OPCUAROBOTICSNamespaceMetadata = "http://opcfoundation.org/UA/Robotics/";

        /// <summary>
        /// The BrowseName for the OperationalModeEnumeration component.
        /// </summary>
        public const string OperationalModeEnumeration = "OperationalModeEnumeration";

        /// <summary>
        /// The BrowseName for the Pitch component.
        /// </summary>
        public const string Pitch = "Pitch";

        /// <summary>
        /// The BrowseName for the PowerTrainIdentifier component.
        /// </summary>
        public const string PowerTrainIdentifier = "<PowerTrainIdentifier>";

        /// <summary>
        /// The BrowseName for the PowerTrains component.
        /// </summary>
        public const string PowerTrains = "PowerTrains";

        /// <summary>
        /// The BrowseName for the PowerTrainType component.
        /// </summary>
        public const string PowerTrainType = "PowerTrainType";

        /// <summary>
        /// The BrowseName for the ProtectiveStopFunctions component.
        /// </summary>
        public const string ProtectiveStopFunctions = "ProtectiveStopFunctions";

        /// <summary>
        /// The BrowseName for the ProtectiveStopFunctionType component.
        /// </summary>
        public const string ProtectiveStopFunctionType = "ProtectiveStopFunctionType";

        /// <summary>
        /// The BrowseName for the Requires component.
        /// </summary>
        public const string Requires = "Requires";

        /// <summary>
        /// The BrowseName for the SafetyStates component.
        /// </summary>
        public const string SafetyStates = "SafetyStates";

        /// <summary>
        /// The BrowseName for the SafetyStatesIdentifier component.
        /// </summary>
        public const string SafetyStatesIdentifier = "<SafetyStatesIdentifier>";

        /// <summary>
        /// The BrowseName for the SafetyStateType component.
        /// </summary>
        public const string SafetyStateType = "SafetyStateType";

        /// <summary>
        /// The BrowseName for the Software component.
        /// </summary>
        public const string Software = "Software";

        /// <summary>
        /// The BrowseName for the TaskControls component.
        /// </summary>
        public const string TaskControls = "TaskControls";

        /// <summary>
        /// The BrowseName for the TaskControlType component.
        /// </summary>
        public const string TaskControlType = "TaskControlType";

        /// <summary>
        /// The BrowseName for the UserType component.
        /// </summary>
        public const string UserType = "UserType";
    }
    #endregion

    #region Namespace Declarations
    /// <summary>
    /// Defines constants for all namespaces referenced by the model design.
    /// </summary>
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class Namespaces
    {
        /// <summary>
        /// The URI for the OpcUaRobotics namespace (.NET code namespace is 'Opc.Ua.Robotics').
        /// </summary>
        public const string OpcUaRobotics = "http://opcfoundation.org/UA/Robotics/";

        /// <summary>
        /// The URI for the OpcUaRoboticsXsd namespace (.NET code namespace is 'Opc.Ua.Robotics').
        /// </summary>
        public const string OpcUaRoboticsXsd = "http://opcfoundation.org/UA/Robotics/Types.xsd";

        /// <summary>
        /// The URI for the OpcUaDi namespace (.NET code namespace is 'Opc.Ua.Di').
        /// </summary>
        public const string OpcUaDi = "http://opcfoundation.org/UA/DI/";

        /// <summary>
        /// The URI for the OpcUaDiXsd namespace (.NET code namespace is 'Opc.Ua.Di').
        /// </summary>
        public const string OpcUaDiXsd = "http://opcfoundation.org/UA/DI/Types.xsd";

        /// <summary>
        /// The URI for the OpcUa namespace (.NET code namespace is 'Opc.Ua').
        /// </summary>
        public const string OpcUa = "http://opcfoundation.org/UA/";

        /// <summary>
        /// The URI for the OpcUaXsd namespace (.NET code namespace is 'Opc.Ua').
        /// </summary>
        public const string OpcUaXsd = "http://opcfoundation.org/UA/2008/02/Types.xsd";
    }
    #endregion
}