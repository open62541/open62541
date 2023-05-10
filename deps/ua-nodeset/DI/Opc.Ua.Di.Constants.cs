/* ========================================================================
 * Copyright (c) 2005-2021 The OPC Foundation, Inc. All rights reserved.
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
using Opc.Ua;

namespace Opc.Ua.Di
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
        /// The identifier for the DeviceHealthEnumeration DataType.
        /// </summary>
        public const uint DeviceHealthEnumeration = 6244;

        /// <summary>
        /// The identifier for the FetchResultDataType DataType.
        /// </summary>
        public const uint FetchResultDataType = 6522;

        /// <summary>
        /// The identifier for the TransferResultErrorDataType DataType.
        /// </summary>
        public const uint TransferResultErrorDataType = 15888;

        /// <summary>
        /// The identifier for the TransferResultDataDataType DataType.
        /// </summary>
        public const uint TransferResultDataDataType = 15889;

        /// <summary>
        /// The identifier for the ParameterResultDataType DataType.
        /// </summary>
        public const uint ParameterResultDataType = 6525;

        /// <summary>
        /// The identifier for the SoftwareVersionFileType DataType.
        /// </summary>
        public const uint SoftwareVersionFileType = 331;

        /// <summary>
        /// The identifier for the UpdateBehavior DataType.
        /// </summary>
        public const uint UpdateBehavior = 333;
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
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_Open Method.
        /// </summary>
        public const uint OPCUADINamespaceMetadata_NamespaceFile_Open = 15015;

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_Close Method.
        /// </summary>
        public const uint OPCUADINamespaceMetadata_NamespaceFile_Close = 15018;

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_Read Method.
        /// </summary>
        public const uint OPCUADINamespaceMetadata_NamespaceFile_Read = 15020;

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_Write Method.
        /// </summary>
        public const uint OPCUADINamespaceMetadata_NamespaceFile_Write = 15023;

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_GetPosition Method.
        /// </summary>
        public const uint OPCUADINamespaceMetadata_NamespaceFile_GetPosition = 15025;

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_SetPosition Method.
        /// </summary>
        public const uint OPCUADINamespaceMetadata_NamespaceFile_SetPosition = 15028;

        /// <summary>
        /// The identifier for the TopologyElementType_Lock_InitLock Method.
        /// </summary>
        public const uint TopologyElementType_Lock_InitLock = 6166;

        /// <summary>
        /// The identifier for the TopologyElementType_Lock_RenewLock Method.
        /// </summary>
        public const uint TopologyElementType_Lock_RenewLock = 6169;

        /// <summary>
        /// The identifier for the TopologyElementType_Lock_ExitLock Method.
        /// </summary>
        public const uint TopologyElementType_Lock_ExitLock = 6171;

        /// <summary>
        /// The identifier for the TopologyElementType_Lock_BreakLock Method.
        /// </summary>
        public const uint TopologyElementType_Lock_BreakLock = 6173;

        /// <summary>
        /// The identifier for the ComponentType_Lock_InitLock Method.
        /// </summary>
        public const uint ComponentType_Lock_InitLock = 15077;

        /// <summary>
        /// The identifier for the ComponentType_Lock_RenewLock Method.
        /// </summary>
        public const uint ComponentType_Lock_RenewLock = 15080;

        /// <summary>
        /// The identifier for the ComponentType_Lock_ExitLock Method.
        /// </summary>
        public const uint ComponentType_Lock_ExitLock = 15082;

        /// <summary>
        /// The identifier for the ComponentType_Lock_BreakLock Method.
        /// </summary>
        public const uint ComponentType_Lock_BreakLock = 15084;

        /// <summary>
        /// The identifier for the DeviceType_Lock_InitLock Method.
        /// </summary>
        public const uint DeviceType_Lock_InitLock = 6191;

        /// <summary>
        /// The identifier for the DeviceType_Lock_RenewLock Method.
        /// </summary>
        public const uint DeviceType_Lock_RenewLock = 6194;

        /// <summary>
        /// The identifier for the DeviceType_Lock_ExitLock Method.
        /// </summary>
        public const uint DeviceType_Lock_ExitLock = 6196;

        /// <summary>
        /// The identifier for the DeviceType_Lock_BreakLock Method.
        /// </summary>
        public const uint DeviceType_Lock_BreakLock = 6198;

        /// <summary>
        /// The identifier for the DeviceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint DeviceType_CPIdentifier_Lock_InitLock = 6583;

        /// <summary>
        /// The identifier for the DeviceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint DeviceType_CPIdentifier_Lock_RenewLock = 6586;

        /// <summary>
        /// The identifier for the DeviceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint DeviceType_CPIdentifier_Lock_ExitLock = 6588;

        /// <summary>
        /// The identifier for the DeviceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint DeviceType_CPIdentifier_Lock_BreakLock = 6590;

        /// <summary>
        /// The identifier for the SoftwareType_Lock_InitLock Method.
        /// </summary>
        public const uint SoftwareType_Lock_InitLock = 15120;

        /// <summary>
        /// The identifier for the SoftwareType_Lock_RenewLock Method.
        /// </summary>
        public const uint SoftwareType_Lock_RenewLock = 15123;

        /// <summary>
        /// The identifier for the SoftwareType_Lock_ExitLock Method.
        /// </summary>
        public const uint SoftwareType_Lock_ExitLock = 15125;

        /// <summary>
        /// The identifier for the SoftwareType_Lock_BreakLock Method.
        /// </summary>
        public const uint SoftwareType_Lock_BreakLock = 15127;

        /// <summary>
        /// The identifier for the BlockType_Lock_InitLock Method.
        /// </summary>
        public const uint BlockType_Lock_InitLock = 6225;

        /// <summary>
        /// The identifier for the BlockType_Lock_RenewLock Method.
        /// </summary>
        public const uint BlockType_Lock_RenewLock = 6228;

        /// <summary>
        /// The identifier for the BlockType_Lock_ExitLock Method.
        /// </summary>
        public const uint BlockType_Lock_ExitLock = 6230;

        /// <summary>
        /// The identifier for the BlockType_Lock_BreakLock Method.
        /// </summary>
        public const uint BlockType_Lock_BreakLock = 6232;

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_ShelvingState_TimedShelve Method.
        /// </summary>
        public const uint DeviceHealthDiagnosticAlarmType_ShelvingState_TimedShelve = 15249;

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_ShelvingState_Unshelve Method.
        /// </summary>
        public const uint DeviceHealthDiagnosticAlarmType_ShelvingState_Unshelve = 15251;

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_ShelvingState_OneShotShelve Method.
        /// </summary>
        public const uint DeviceHealthDiagnosticAlarmType_ShelvingState_OneShotShelve = 15252;

        /// <summary>
        /// The identifier for the FailureAlarmType_ShelvingState_TimedShelve Method.
        /// </summary>
        public const uint FailureAlarmType_ShelvingState_TimedShelve = 15398;

        /// <summary>
        /// The identifier for the FailureAlarmType_ShelvingState_Unshelve Method.
        /// </summary>
        public const uint FailureAlarmType_ShelvingState_Unshelve = 15400;

        /// <summary>
        /// The identifier for the FailureAlarmType_ShelvingState_OneShotShelve Method.
        /// </summary>
        public const uint FailureAlarmType_ShelvingState_OneShotShelve = 15401;

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_ShelvingState_TimedShelve Method.
        /// </summary>
        public const uint CheckFunctionAlarmType_ShelvingState_TimedShelve = 15547;

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_ShelvingState_Unshelve Method.
        /// </summary>
        public const uint CheckFunctionAlarmType_ShelvingState_Unshelve = 15549;

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_ShelvingState_OneShotShelve Method.
        /// </summary>
        public const uint CheckFunctionAlarmType_ShelvingState_OneShotShelve = 15550;

        /// <summary>
        /// The identifier for the OffSpecAlarmType_ShelvingState_TimedShelve Method.
        /// </summary>
        public const uint OffSpecAlarmType_ShelvingState_TimedShelve = 15696;

        /// <summary>
        /// The identifier for the OffSpecAlarmType_ShelvingState_Unshelve Method.
        /// </summary>
        public const uint OffSpecAlarmType_ShelvingState_Unshelve = 15698;

        /// <summary>
        /// The identifier for the OffSpecAlarmType_ShelvingState_OneShotShelve Method.
        /// </summary>
        public const uint OffSpecAlarmType_ShelvingState_OneShotShelve = 15699;

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_ShelvingState_TimedShelve Method.
        /// </summary>
        public const uint MaintenanceRequiredAlarmType_ShelvingState_TimedShelve = 15845;

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_ShelvingState_Unshelve Method.
        /// </summary>
        public const uint MaintenanceRequiredAlarmType_ShelvingState_Unshelve = 15847;

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_ShelvingState_OneShotShelve Method.
        /// </summary>
        public const uint MaintenanceRequiredAlarmType_ShelvingState_OneShotShelve = 15848;

        /// <summary>
        /// The identifier for the NetworkType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint NetworkType_CPIdentifier_Lock_InitLock = 6260;

        /// <summary>
        /// The identifier for the NetworkType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint NetworkType_CPIdentifier_Lock_RenewLock = 6263;

        /// <summary>
        /// The identifier for the NetworkType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint NetworkType_CPIdentifier_Lock_ExitLock = 6265;

        /// <summary>
        /// The identifier for the NetworkType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint NetworkType_CPIdentifier_Lock_BreakLock = 6267;

        /// <summary>
        /// The identifier for the NetworkType_Lock_InitLock Method.
        /// </summary>
        public const uint NetworkType_Lock_InitLock = 6299;

        /// <summary>
        /// The identifier for the NetworkType_Lock_RenewLock Method.
        /// </summary>
        public const uint NetworkType_Lock_RenewLock = 6302;

        /// <summary>
        /// The identifier for the NetworkType_Lock_ExitLock Method.
        /// </summary>
        public const uint NetworkType_Lock_ExitLock = 6304;

        /// <summary>
        /// The identifier for the NetworkType_Lock_BreakLock Method.
        /// </summary>
        public const uint NetworkType_Lock_BreakLock = 6306;

        /// <summary>
        /// The identifier for the ConnectionPointType_Lock_InitLock Method.
        /// </summary>
        public const uint ConnectionPointType_Lock_InitLock = 6322;

        /// <summary>
        /// The identifier for the ConnectionPointType_Lock_RenewLock Method.
        /// </summary>
        public const uint ConnectionPointType_Lock_RenewLock = 6325;

        /// <summary>
        /// The identifier for the ConnectionPointType_Lock_ExitLock Method.
        /// </summary>
        public const uint ConnectionPointType_Lock_ExitLock = 6327;

        /// <summary>
        /// The identifier for the ConnectionPointType_Lock_BreakLock Method.
        /// </summary>
        public const uint ConnectionPointType_Lock_BreakLock = 6329;

        /// <summary>
        /// The identifier for the ConnectionPointType_NetworkIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ConnectionPointType_NetworkIdentifier_Lock_InitLock = 6605;

        /// <summary>
        /// The identifier for the ConnectionPointType_NetworkIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ConnectionPointType_NetworkIdentifier_Lock_RenewLock = 6608;

        /// <summary>
        /// The identifier for the ConnectionPointType_NetworkIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ConnectionPointType_NetworkIdentifier_Lock_ExitLock = 6610;

        /// <summary>
        /// The identifier for the ConnectionPointType_NetworkIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ConnectionPointType_NetworkIdentifier_Lock_BreakLock = 6612;

        /// <summary>
        /// The identifier for the TransferServicesType_TransferToDevice Method.
        /// </summary>
        public const uint TransferServicesType_TransferToDevice = 6527;

        /// <summary>
        /// The identifier for the TransferServicesType_TransferFromDevice Method.
        /// </summary>
        public const uint TransferServicesType_TransferFromDevice = 6529;

        /// <summary>
        /// The identifier for the TransferServicesType_FetchTransferResultData Method.
        /// </summary>
        public const uint TransferServicesType_FetchTransferResultData = 6531;

        /// <summary>
        /// The identifier for the LockingServicesType_InitLock Method.
        /// </summary>
        public const uint LockingServicesType_InitLock = 6393;

        /// <summary>
        /// The identifier for the LockingServicesType_RenewLock Method.
        /// </summary>
        public const uint LockingServicesType_RenewLock = 6396;

        /// <summary>
        /// The identifier for the LockingServicesType_ExitLock Method.
        /// </summary>
        public const uint LockingServicesType_ExitLock = 6398;

        /// <summary>
        /// The identifier for the LockingServicesType_BreakLock Method.
        /// </summary>
        public const uint LockingServicesType_BreakLock = 6400;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_PrepareForUpdate_Prepare Method.
        /// </summary>
        public const uint SoftwareUpdateType_PrepareForUpdate_Prepare = 19;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_PrepareForUpdate_Abort Method.
        /// </summary>
        public const uint SoftwareUpdateType_PrepareForUpdate_Abort = 20;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Installation_Resume Method.
        /// </summary>
        public const uint SoftwareUpdateType_Installation_Resume = 61;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Confirmation_Confirm Method.
        /// </summary>
        public const uint SoftwareUpdateType_Confirmation_Confirm = 112;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Parameters_GenerateFileForRead Method.
        /// </summary>
        public const uint SoftwareUpdateType_Parameters_GenerateFileForRead = 124;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Parameters_GenerateFileForWrite Method.
        /// </summary>
        public const uint SoftwareUpdateType_Parameters_GenerateFileForWrite = 127;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Parameters_CloseAndCommit Method.
        /// </summary>
        public const uint SoftwareUpdateType_Parameters_CloseAndCommit = 130;

        /// <summary>
        /// The identifier for the PackageLoadingType_FileTransfer_GenerateFileForRead Method.
        /// </summary>
        public const uint PackageLoadingType_FileTransfer_GenerateFileForRead = 142;

        /// <summary>
        /// The identifier for the PackageLoadingType_FileTransfer_GenerateFileForWrite Method.
        /// </summary>
        public const uint PackageLoadingType_FileTransfer_GenerateFileForWrite = 145;

        /// <summary>
        /// The identifier for the PackageLoadingType_FileTransfer_CloseAndCommit Method.
        /// </summary>
        public const uint PackageLoadingType_FileTransfer_CloseAndCommit = 148;

        /// <summary>
        /// The identifier for the DirectLoadingType_FileTransfer_GenerateFileForRead Method.
        /// </summary>
        public const uint DirectLoadingType_FileTransfer_GenerateFileForRead = 158;

        /// <summary>
        /// The identifier for the DirectLoadingType_FileTransfer_GenerateFileForWrite Method.
        /// </summary>
        public const uint DirectLoadingType_FileTransfer_GenerateFileForWrite = 161;

        /// <summary>
        /// The identifier for the DirectLoadingType_FileTransfer_CloseAndCommit Method.
        /// </summary>
        public const uint DirectLoadingType_FileTransfer_CloseAndCommit = 164;

        /// <summary>
        /// The identifier for the CachedLoadingType_FileTransfer_GenerateFileForRead Method.
        /// </summary>
        public const uint CachedLoadingType_FileTransfer_GenerateFileForRead = 176;

        /// <summary>
        /// The identifier for the CachedLoadingType_FileTransfer_GenerateFileForWrite Method.
        /// </summary>
        public const uint CachedLoadingType_FileTransfer_GenerateFileForWrite = 179;

        /// <summary>
        /// The identifier for the CachedLoadingType_FileTransfer_CloseAndCommit Method.
        /// </summary>
        public const uint CachedLoadingType_FileTransfer_CloseAndCommit = 182;

        /// <summary>
        /// The identifier for the CachedLoadingType_GetUpdateBehavior Method.
        /// </summary>
        public const uint CachedLoadingType_GetUpdateBehavior = 189;

        /// <summary>
        /// The identifier for the FileSystemLoadingType_FileSystem_CreateDirectory Method.
        /// </summary>
        public const uint FileSystemLoadingType_FileSystem_CreateDirectory = 195;

        /// <summary>
        /// The identifier for the FileSystemLoadingType_FileSystem_CreateFile Method.
        /// </summary>
        public const uint FileSystemLoadingType_FileSystem_CreateFile = 198;

        /// <summary>
        /// The identifier for the FileSystemLoadingType_FileSystem_DeleteFileSystemObject Method.
        /// </summary>
        public const uint FileSystemLoadingType_FileSystem_DeleteFileSystemObject = 201;

        /// <summary>
        /// The identifier for the FileSystemLoadingType_FileSystem_MoveOrCopy Method.
        /// </summary>
        public const uint FileSystemLoadingType_FileSystem_MoveOrCopy = 203;

        /// <summary>
        /// The identifier for the FileSystemLoadingType_GetUpdateBehavior Method.
        /// </summary>
        public const uint FileSystemLoadingType_GetUpdateBehavior = 206;

        /// <summary>
        /// The identifier for the FileSystemLoadingType_ValidateFiles Method.
        /// </summary>
        public const uint FileSystemLoadingType_ValidateFiles = 209;

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_Prepare Method.
        /// </summary>
        public const uint PrepareForUpdateStateMachineType_Prepare = 228;

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_Abort Method.
        /// </summary>
        public const uint PrepareForUpdateStateMachineType_Abort = 229;

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_Resume Method.
        /// </summary>
        public const uint PrepareForUpdateStateMachineType_Resume = 230;

        /// <summary>
        /// The identifier for the InstallationStateMachineType_InstallSoftwarePackage Method.
        /// </summary>
        public const uint InstallationStateMachineType_InstallSoftwarePackage = 265;

        /// <summary>
        /// The identifier for the InstallationStateMachineType_InstallFiles Method.
        /// </summary>
        public const uint InstallationStateMachineType_InstallFiles = 268;

        /// <summary>
        /// The identifier for the InstallationStateMachineType_Resume Method.
        /// </summary>
        public const uint InstallationStateMachineType_Resume = 270;

        /// <summary>
        /// The identifier for the ConfirmationStateMachineType_Confirm Method.
        /// </summary>
        public const uint ConfirmationStateMachineType_Confirm = 321;
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
        /// The identifier for the OPCUADINamespaceMetadata Object.
        /// </summary>
        public const uint OPCUADINamespaceMetadata = 15001;

        /// <summary>
        /// The identifier for the DeviceSet Object.
        /// </summary>
        public const uint DeviceSet = 5001;

        /// <summary>
        /// The identifier for the DeviceFeatures Object.
        /// </summary>
        public const uint DeviceFeatures = 15034;

        /// <summary>
        /// The identifier for the NetworkSet Object.
        /// </summary>
        public const uint NetworkSet = 6078;

        /// <summary>
        /// The identifier for the DeviceTopology Object.
        /// </summary>
        public const uint DeviceTopology = 6094;

        /// <summary>
        /// The identifier for the TopologyElementType_ParameterSet Object.
        /// </summary>
        public const uint TopologyElementType_ParameterSet = 5002;

        /// <summary>
        /// The identifier for the TopologyElementType_MethodSet Object.
        /// </summary>
        public const uint TopologyElementType_MethodSet = 5003;

        /// <summary>
        /// The identifier for the TopologyElementType_GroupIdentifier Object.
        /// </summary>
        public const uint TopologyElementType_GroupIdentifier = 6567;

        /// <summary>
        /// The identifier for the TopologyElementType_Identification Object.
        /// </summary>
        public const uint TopologyElementType_Identification = 6014;

        /// <summary>
        /// The identifier for the TopologyElementType_Lock Object.
        /// </summary>
        public const uint TopologyElementType_Lock = 6161;

        /// <summary>
        /// The identifier for the IDeviceHealthType_DeviceHealthAlarms Object.
        /// </summary>
        public const uint IDeviceHealthType_DeviceHealthAlarms = 15053;

        /// <summary>
        /// The identifier for the ISupportInfoType_DeviceTypeImage Object.
        /// </summary>
        public const uint ISupportInfoType_DeviceTypeImage = 15055;

        /// <summary>
        /// The identifier for the ISupportInfoType_Documentation Object.
        /// </summary>
        public const uint ISupportInfoType_Documentation = 15057;

        /// <summary>
        /// The identifier for the ISupportInfoType_ProtocolSupport Object.
        /// </summary>
        public const uint ISupportInfoType_ProtocolSupport = 15059;

        /// <summary>
        /// The identifier for the ISupportInfoType_ImageSet Object.
        /// </summary>
        public const uint ISupportInfoType_ImageSet = 15061;

        /// <summary>
        /// The identifier for the DeviceType_CPIdentifier Object.
        /// </summary>
        public const uint DeviceType_CPIdentifier = 6571;

        /// <summary>
        /// The identifier for the DeviceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public const uint DeviceType_CPIdentifier_NetworkAddress = 6592;

        /// <summary>
        /// The identifier for the DeviceType_DeviceHealthAlarms Object.
        /// </summary>
        public const uint DeviceType_DeviceHealthAlarms = 15105;

        /// <summary>
        /// The identifier for the DeviceType_DeviceTypeImage Object.
        /// </summary>
        public const uint DeviceType_DeviceTypeImage = 6209;

        /// <summary>
        /// The identifier for the DeviceType_Documentation Object.
        /// </summary>
        public const uint DeviceType_Documentation = 6211;

        /// <summary>
        /// The identifier for the DeviceType_ProtocolSupport Object.
        /// </summary>
        public const uint DeviceType_ProtocolSupport = 6213;

        /// <summary>
        /// The identifier for the DeviceType_ImageSet Object.
        /// </summary>
        public const uint DeviceType_ImageSet = 6215;

        /// <summary>
        /// The identifier for the ConfigurableObjectType_SupportedTypes Object.
        /// </summary>
        public const uint ConfigurableObjectType_SupportedTypes = 5004;

        /// <summary>
        /// The identifier for the ConfigurableObjectType_ObjectIdentifier Object.
        /// </summary>
        public const uint ConfigurableObjectType_ObjectIdentifier = 6026;

        /// <summary>
        /// The identifier for the FunctionalGroupType_GroupIdentifier Object.
        /// </summary>
        public const uint FunctionalGroupType_GroupIdentifier = 6027;

        /// <summary>
        /// The identifier for the NetworkType_ProfileIdentifier Object.
        /// </summary>
        public const uint NetworkType_ProfileIdentifier = 6596;

        /// <summary>
        /// The identifier for the NetworkType_CPIdentifier Object.
        /// </summary>
        public const uint NetworkType_CPIdentifier = 6248;

        /// <summary>
        /// The identifier for the NetworkType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public const uint NetworkType_CPIdentifier_NetworkAddress = 6292;

        /// <summary>
        /// The identifier for the NetworkType_Lock Object.
        /// </summary>
        public const uint NetworkType_Lock = 6294;

        /// <summary>
        /// The identifier for the ConnectionPointType_NetworkAddress Object.
        /// </summary>
        public const uint ConnectionPointType_NetworkAddress = 6354;

        /// <summary>
        /// The identifier for the ConnectionPointType_ProfileIdentifier Object.
        /// </summary>
        public const uint ConnectionPointType_ProfileIdentifier = 6499;

        /// <summary>
        /// The identifier for the ConnectionPointType_NetworkIdentifier Object.
        /// </summary>
        public const uint ConnectionPointType_NetworkIdentifier = 6599;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Loading Object.
        /// </summary>
        public const uint SoftwareUpdateType_Loading = 2;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_PrepareForUpdate Object.
        /// </summary>
        public const uint SoftwareUpdateType_PrepareForUpdate = 4;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Installation Object.
        /// </summary>
        public const uint SoftwareUpdateType_Installation = 40;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_PowerCycle Object.
        /// </summary>
        public const uint SoftwareUpdateType_PowerCycle = 76;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Confirmation Object.
        /// </summary>
        public const uint SoftwareUpdateType_Confirmation = 98;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Parameters Object.
        /// </summary>
        public const uint SoftwareUpdateType_Parameters = 122;

        /// <summary>
        /// The identifier for the PackageLoadingType_CurrentVersion Object.
        /// </summary>
        public const uint PackageLoadingType_CurrentVersion = 139;

        /// <summary>
        /// The identifier for the PackageLoadingType_FileTransfer Object.
        /// </summary>
        public const uint PackageLoadingType_FileTransfer = 140;

        /// <summary>
        /// The identifier for the CachedLoadingType_PendingVersion Object.
        /// </summary>
        public const uint CachedLoadingType_PendingVersion = 187;

        /// <summary>
        /// The identifier for the CachedLoadingType_FallbackVersion Object.
        /// </summary>
        public const uint CachedLoadingType_FallbackVersion = 188;

        /// <summary>
        /// The identifier for the FileSystemLoadingType_FileSystem Object.
        /// </summary>
        public const uint FileSystemLoadingType_FileSystem = 194;

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_Idle Object.
        /// </summary>
        public const uint PrepareForUpdateStateMachineType_Idle = 231;

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_Preparing Object.
        /// </summary>
        public const uint PrepareForUpdateStateMachineType_Preparing = 233;

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_PreparedForUpdate Object.
        /// </summary>
        public const uint PrepareForUpdateStateMachineType_PreparedForUpdate = 235;

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_Resuming Object.
        /// </summary>
        public const uint PrepareForUpdateStateMachineType_Resuming = 237;

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_IdleToPreparing Object.
        /// </summary>
        public const uint PrepareForUpdateStateMachineType_IdleToPreparing = 239;

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_PreparingToIdle Object.
        /// </summary>
        public const uint PrepareForUpdateStateMachineType_PreparingToIdle = 241;

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_PreparingToPreparedForUpdate Object.
        /// </summary>
        public const uint PrepareForUpdateStateMachineType_PreparingToPreparedForUpdate = 243;

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_PreparedForUpdateToResuming Object.
        /// </summary>
        public const uint PrepareForUpdateStateMachineType_PreparedForUpdateToResuming = 245;

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_ResumingToIdle Object.
        /// </summary>
        public const uint PrepareForUpdateStateMachineType_ResumingToIdle = 247;

        /// <summary>
        /// The identifier for the InstallationStateMachineType_Idle Object.
        /// </summary>
        public const uint InstallationStateMachineType_Idle = 271;

        /// <summary>
        /// The identifier for the InstallationStateMachineType_Installing Object.
        /// </summary>
        public const uint InstallationStateMachineType_Installing = 273;

        /// <summary>
        /// The identifier for the InstallationStateMachineType_Error Object.
        /// </summary>
        public const uint InstallationStateMachineType_Error = 275;

        /// <summary>
        /// The identifier for the InstallationStateMachineType_IdleToInstalling Object.
        /// </summary>
        public const uint InstallationStateMachineType_IdleToInstalling = 277;

        /// <summary>
        /// The identifier for the InstallationStateMachineType_InstallingToIdle Object.
        /// </summary>
        public const uint InstallationStateMachineType_InstallingToIdle = 279;

        /// <summary>
        /// The identifier for the InstallationStateMachineType_InstallingToError Object.
        /// </summary>
        public const uint InstallationStateMachineType_InstallingToError = 281;

        /// <summary>
        /// The identifier for the InstallationStateMachineType_ErrorToIdle Object.
        /// </summary>
        public const uint InstallationStateMachineType_ErrorToIdle = 283;

        /// <summary>
        /// The identifier for the PowerCycleStateMachineType_NotWaitingForPowerCycle Object.
        /// </summary>
        public const uint PowerCycleStateMachineType_NotWaitingForPowerCycle = 299;

        /// <summary>
        /// The identifier for the PowerCycleStateMachineType_WaitingForPowerCycle Object.
        /// </summary>
        public const uint PowerCycleStateMachineType_WaitingForPowerCycle = 301;

        /// <summary>
        /// The identifier for the PowerCycleStateMachineType_NotWaitingForPowerCycleToWaitingForPowerCycle Object.
        /// </summary>
        public const uint PowerCycleStateMachineType_NotWaitingForPowerCycleToWaitingForPowerCycle = 303;

        /// <summary>
        /// The identifier for the PowerCycleStateMachineType_WaitingForPowerCycleToNotWaitingForPowerCycle Object.
        /// </summary>
        public const uint PowerCycleStateMachineType_WaitingForPowerCycleToNotWaitingForPowerCycle = 305;

        /// <summary>
        /// The identifier for the ConfirmationStateMachineType_NotWaitingForConfirm Object.
        /// </summary>
        public const uint ConfirmationStateMachineType_NotWaitingForConfirm = 323;

        /// <summary>
        /// The identifier for the ConfirmationStateMachineType_WaitingForConfirm Object.
        /// </summary>
        public const uint ConfirmationStateMachineType_WaitingForConfirm = 325;

        /// <summary>
        /// The identifier for the ConfirmationStateMachineType_NotWaitingForConfirmToWaitingForConfirm Object.
        /// </summary>
        public const uint ConfirmationStateMachineType_NotWaitingForConfirmToWaitingForConfirm = 327;

        /// <summary>
        /// The identifier for the ConfirmationStateMachineType_WaitingForConfirmToNotWaitingForConfirm Object.
        /// </summary>
        public const uint ConfirmationStateMachineType_WaitingForConfirmToNotWaitingForConfirm = 329;

        /// <summary>
        /// The identifier for the FetchResultDataType_Encoding_DefaultBinary Object.
        /// </summary>
        public const uint FetchResultDataType_Encoding_DefaultBinary = 6551;

        /// <summary>
        /// The identifier for the TransferResultErrorDataType_Encoding_DefaultBinary Object.
        /// </summary>
        public const uint TransferResultErrorDataType_Encoding_DefaultBinary = 15891;

        /// <summary>
        /// The identifier for the TransferResultDataDataType_Encoding_DefaultBinary Object.
        /// </summary>
        public const uint TransferResultDataDataType_Encoding_DefaultBinary = 15892;

        /// <summary>
        /// The identifier for the ParameterResultDataType_Encoding_DefaultBinary Object.
        /// </summary>
        public const uint ParameterResultDataType_Encoding_DefaultBinary = 6554;

        /// <summary>
        /// The identifier for the FetchResultDataType_Encoding_DefaultXml Object.
        /// </summary>
        public const uint FetchResultDataType_Encoding_DefaultXml = 6535;

        /// <summary>
        /// The identifier for the TransferResultErrorDataType_Encoding_DefaultXml Object.
        /// </summary>
        public const uint TransferResultErrorDataType_Encoding_DefaultXml = 15900;

        /// <summary>
        /// The identifier for the TransferResultDataDataType_Encoding_DefaultXml Object.
        /// </summary>
        public const uint TransferResultDataDataType_Encoding_DefaultXml = 15901;

        /// <summary>
        /// The identifier for the ParameterResultDataType_Encoding_DefaultXml Object.
        /// </summary>
        public const uint ParameterResultDataType_Encoding_DefaultXml = 6538;

        /// <summary>
        /// The identifier for the FetchResultDataType_Encoding_DefaultJson Object.
        /// </summary>
        public const uint FetchResultDataType_Encoding_DefaultJson = 15909;

        /// <summary>
        /// The identifier for the TransferResultErrorDataType_Encoding_DefaultJson Object.
        /// </summary>
        public const uint TransferResultErrorDataType_Encoding_DefaultJson = 15910;

        /// <summary>
        /// The identifier for the TransferResultDataDataType_Encoding_DefaultJson Object.
        /// </summary>
        public const uint TransferResultDataDataType_Encoding_DefaultJson = 15911;

        /// <summary>
        /// The identifier for the ParameterResultDataType_Encoding_DefaultJson Object.
        /// </summary>
        public const uint ParameterResultDataType_Encoding_DefaultJson = 15912;
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
        /// The identifier for the TopologyElementType ObjectType.
        /// </summary>
        public const uint TopologyElementType = 1001;

        /// <summary>
        /// The identifier for the IVendorNameplateType ObjectType.
        /// </summary>
        public const uint IVendorNameplateType = 15035;

        /// <summary>
        /// The identifier for the ITagNameplateType ObjectType.
        /// </summary>
        public const uint ITagNameplateType = 15048;

        /// <summary>
        /// The identifier for the IDeviceHealthType ObjectType.
        /// </summary>
        public const uint IDeviceHealthType = 15051;

        /// <summary>
        /// The identifier for the ISupportInfoType ObjectType.
        /// </summary>
        public const uint ISupportInfoType = 15054;

        /// <summary>
        /// The identifier for the ComponentType ObjectType.
        /// </summary>
        public const uint ComponentType = 15063;

        /// <summary>
        /// The identifier for the DeviceType ObjectType.
        /// </summary>
        public const uint DeviceType = 1002;

        /// <summary>
        /// The identifier for the SoftwareType ObjectType.
        /// </summary>
        public const uint SoftwareType = 15106;

        /// <summary>
        /// The identifier for the BlockType ObjectType.
        /// </summary>
        public const uint BlockType = 1003;

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType ObjectType.
        /// </summary>
        public const uint DeviceHealthDiagnosticAlarmType = 15143;

        /// <summary>
        /// The identifier for the FailureAlarmType ObjectType.
        /// </summary>
        public const uint FailureAlarmType = 15292;

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType ObjectType.
        /// </summary>
        public const uint CheckFunctionAlarmType = 15441;

        /// <summary>
        /// The identifier for the OffSpecAlarmType ObjectType.
        /// </summary>
        public const uint OffSpecAlarmType = 15590;

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType ObjectType.
        /// </summary>
        public const uint MaintenanceRequiredAlarmType = 15739;

        /// <summary>
        /// The identifier for the ConfigurableObjectType ObjectType.
        /// </summary>
        public const uint ConfigurableObjectType = 1004;

        /// <summary>
        /// The identifier for the FunctionalGroupType ObjectType.
        /// </summary>
        public const uint FunctionalGroupType = 1005;

        /// <summary>
        /// The identifier for the ProtocolType ObjectType.
        /// </summary>
        public const uint ProtocolType = 1006;

        /// <summary>
        /// The identifier for the NetworkType ObjectType.
        /// </summary>
        public const uint NetworkType = 6247;

        /// <summary>
        /// The identifier for the ConnectionPointType ObjectType.
        /// </summary>
        public const uint ConnectionPointType = 6308;

        /// <summary>
        /// The identifier for the TransferServicesType ObjectType.
        /// </summary>
        public const uint TransferServicesType = 6526;

        /// <summary>
        /// The identifier for the LockingServicesType ObjectType.
        /// </summary>
        public const uint LockingServicesType = 6388;

        /// <summary>
        /// The identifier for the SoftwareUpdateType ObjectType.
        /// </summary>
        public const uint SoftwareUpdateType = 1;

        /// <summary>
        /// The identifier for the SoftwareLoadingType ObjectType.
        /// </summary>
        public const uint SoftwareLoadingType = 135;

        /// <summary>
        /// The identifier for the PackageLoadingType ObjectType.
        /// </summary>
        public const uint PackageLoadingType = 137;

        /// <summary>
        /// The identifier for the DirectLoadingType ObjectType.
        /// </summary>
        public const uint DirectLoadingType = 153;

        /// <summary>
        /// The identifier for the CachedLoadingType ObjectType.
        /// </summary>
        public const uint CachedLoadingType = 171;

        /// <summary>
        /// The identifier for the FileSystemLoadingType ObjectType.
        /// </summary>
        public const uint FileSystemLoadingType = 192;

        /// <summary>
        /// The identifier for the SoftwareVersionType ObjectType.
        /// </summary>
        public const uint SoftwareVersionType = 212;

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType ObjectType.
        /// </summary>
        public const uint PrepareForUpdateStateMachineType = 213;

        /// <summary>
        /// The identifier for the InstallationStateMachineType ObjectType.
        /// </summary>
        public const uint InstallationStateMachineType = 249;

        /// <summary>
        /// The identifier for the PowerCycleStateMachineType ObjectType.
        /// </summary>
        public const uint PowerCycleStateMachineType = 285;

        /// <summary>
        /// The identifier for the ConfirmationStateMachineType ObjectType.
        /// </summary>
        public const uint ConfirmationStateMachineType = 307;
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
        /// The identifier for the ConnectsTo ReferenceType.
        /// </summary>
        public const uint ConnectsTo = 6030;

        /// <summary>
        /// The identifier for the ConnectsToParent ReferenceType.
        /// </summary>
        public const uint ConnectsToParent = 6467;

        /// <summary>
        /// The identifier for the IsOnline ReferenceType.
        /// </summary>
        public const uint IsOnline = 6031;
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
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceUri Variable.
        /// </summary>
        public const uint OPCUADINamespaceMetadata_NamespaceUri = 15002;

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceVersion Variable.
        /// </summary>
        public const uint OPCUADINamespaceMetadata_NamespaceVersion = 15003;

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespacePublicationDate Variable.
        /// </summary>
        public const uint OPCUADINamespaceMetadata_NamespacePublicationDate = 15004;

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_IsNamespaceSubset Variable.
        /// </summary>
        public const uint OPCUADINamespaceMetadata_IsNamespaceSubset = 15005;

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_StaticNodeIdTypes Variable.
        /// </summary>
        public const uint OPCUADINamespaceMetadata_StaticNodeIdTypes = 15006;

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_StaticNumericNodeIdRange Variable.
        /// </summary>
        public const uint OPCUADINamespaceMetadata_StaticNumericNodeIdRange = 15007;

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_StaticStringNodeIdPattern Variable.
        /// </summary>
        public const uint OPCUADINamespaceMetadata_StaticStringNodeIdPattern = 15008;

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_Size Variable.
        /// </summary>
        public const uint OPCUADINamespaceMetadata_NamespaceFile_Size = 15010;

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_Writable Variable.
        /// </summary>
        public const uint OPCUADINamespaceMetadata_NamespaceFile_Writable = 15011;

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_UserWritable Variable.
        /// </summary>
        public const uint OPCUADINamespaceMetadata_NamespaceFile_UserWritable = 15012;

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_OpenCount Variable.
        /// </summary>
        public const uint OPCUADINamespaceMetadata_NamespaceFile_OpenCount = 15013;

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_Open_InputArguments Variable.
        /// </summary>
        public const uint OPCUADINamespaceMetadata_NamespaceFile_Open_InputArguments = 15016;

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_Open_OutputArguments Variable.
        /// </summary>
        public const uint OPCUADINamespaceMetadata_NamespaceFile_Open_OutputArguments = 15017;

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_Close_InputArguments Variable.
        /// </summary>
        public const uint OPCUADINamespaceMetadata_NamespaceFile_Close_InputArguments = 15019;

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_Read_InputArguments Variable.
        /// </summary>
        public const uint OPCUADINamespaceMetadata_NamespaceFile_Read_InputArguments = 15021;

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_Read_OutputArguments Variable.
        /// </summary>
        public const uint OPCUADINamespaceMetadata_NamespaceFile_Read_OutputArguments = 15022;

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_Write_InputArguments Variable.
        /// </summary>
        public const uint OPCUADINamespaceMetadata_NamespaceFile_Write_InputArguments = 15024;

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_GetPosition_InputArguments Variable.
        /// </summary>
        public const uint OPCUADINamespaceMetadata_NamespaceFile_GetPosition_InputArguments = 15026;

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_GetPosition_OutputArguments Variable.
        /// </summary>
        public const uint OPCUADINamespaceMetadata_NamespaceFile_GetPosition_OutputArguments = 15027;

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_SetPosition_InputArguments Variable.
        /// </summary>
        public const uint OPCUADINamespaceMetadata_NamespaceFile_SetPosition_InputArguments = 15029;

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_DefaultRolePermissions Variable.
        /// </summary>
        public const uint OPCUADINamespaceMetadata_DefaultRolePermissions = 15031;

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_DefaultUserRolePermissions Variable.
        /// </summary>
        public const uint OPCUADINamespaceMetadata_DefaultUserRolePermissions = 15032;

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_DefaultAccessRestrictions Variable.
        /// </summary>
        public const uint OPCUADINamespaceMetadata_DefaultAccessRestrictions = 15033;

        /// <summary>
        /// The identifier for the DeviceTopology_OnlineAccess Variable.
        /// </summary>
        public const uint DeviceTopology_OnlineAccess = 6095;

        /// <summary>
        /// The identifier for the TopologyElementType_ParameterSet_ParameterIdentifier Variable.
        /// </summary>
        public const uint TopologyElementType_ParameterSet_ParameterIdentifier = 6017;

        /// <summary>
        /// The identifier for the TopologyElementType_Lock_Locked Variable.
        /// </summary>
        public const uint TopologyElementType_Lock_Locked = 6468;

        /// <summary>
        /// The identifier for the TopologyElementType_Lock_LockingClient Variable.
        /// </summary>
        public const uint TopologyElementType_Lock_LockingClient = 6163;

        /// <summary>
        /// The identifier for the TopologyElementType_Lock_LockingUser Variable.
        /// </summary>
        public const uint TopologyElementType_Lock_LockingUser = 6164;

        /// <summary>
        /// The identifier for the TopologyElementType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint TopologyElementType_Lock_RemainingLockTime = 6165;

        /// <summary>
        /// The identifier for the TopologyElementType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint TopologyElementType_Lock_InitLock_InputArguments = 6167;

        /// <summary>
        /// The identifier for the TopologyElementType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint TopologyElementType_Lock_InitLock_OutputArguments = 6168;

        /// <summary>
        /// The identifier for the TopologyElementType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint TopologyElementType_Lock_RenewLock_OutputArguments = 6170;

        /// <summary>
        /// The identifier for the TopologyElementType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint TopologyElementType_Lock_ExitLock_OutputArguments = 6172;

        /// <summary>
        /// The identifier for the TopologyElementType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint TopologyElementType_Lock_BreakLock_OutputArguments = 6174;

        /// <summary>
        /// The identifier for the IVendorNameplateType_Manufacturer Variable.
        /// </summary>
        public const uint IVendorNameplateType_Manufacturer = 15036;

        /// <summary>
        /// The identifier for the IVendorNameplateType_ManufacturerUri Variable.
        /// </summary>
        public const uint IVendorNameplateType_ManufacturerUri = 15037;

        /// <summary>
        /// The identifier for the IVendorNameplateType_Model Variable.
        /// </summary>
        public const uint IVendorNameplateType_Model = 15038;

        /// <summary>
        /// The identifier for the IVendorNameplateType_HardwareRevision Variable.
        /// </summary>
        public const uint IVendorNameplateType_HardwareRevision = 15039;

        /// <summary>
        /// The identifier for the IVendorNameplateType_SoftwareRevision Variable.
        /// </summary>
        public const uint IVendorNameplateType_SoftwareRevision = 15040;

        /// <summary>
        /// The identifier for the IVendorNameplateType_DeviceRevision Variable.
        /// </summary>
        public const uint IVendorNameplateType_DeviceRevision = 15041;

        /// <summary>
        /// The identifier for the IVendorNameplateType_ProductCode Variable.
        /// </summary>
        public const uint IVendorNameplateType_ProductCode = 15042;

        /// <summary>
        /// The identifier for the IVendorNameplateType_DeviceManual Variable.
        /// </summary>
        public const uint IVendorNameplateType_DeviceManual = 15043;

        /// <summary>
        /// The identifier for the IVendorNameplateType_DeviceClass Variable.
        /// </summary>
        public const uint IVendorNameplateType_DeviceClass = 15044;

        /// <summary>
        /// The identifier for the IVendorNameplateType_SerialNumber Variable.
        /// </summary>
        public const uint IVendorNameplateType_SerialNumber = 15045;

        /// <summary>
        /// The identifier for the IVendorNameplateType_ProductInstanceUri Variable.
        /// </summary>
        public const uint IVendorNameplateType_ProductInstanceUri = 15046;

        /// <summary>
        /// The identifier for the IVendorNameplateType_RevisionCounter Variable.
        /// </summary>
        public const uint IVendorNameplateType_RevisionCounter = 15047;

        /// <summary>
        /// The identifier for the ITagNameplateType_AssetId Variable.
        /// </summary>
        public const uint ITagNameplateType_AssetId = 15049;

        /// <summary>
        /// The identifier for the ITagNameplateType_ComponentName Variable.
        /// </summary>
        public const uint ITagNameplateType_ComponentName = 15050;

        /// <summary>
        /// The identifier for the IDeviceHealthType_DeviceHealth Variable.
        /// </summary>
        public const uint IDeviceHealthType_DeviceHealth = 15052;

        /// <summary>
        /// The identifier for the ISupportInfoType_DeviceTypeImage_ImageIdentifier Variable.
        /// </summary>
        public const uint ISupportInfoType_DeviceTypeImage_ImageIdentifier = 15056;

        /// <summary>
        /// The identifier for the ISupportInfoType_Documentation_DocumentIdentifier Variable.
        /// </summary>
        public const uint ISupportInfoType_Documentation_DocumentIdentifier = 15058;

        /// <summary>
        /// The identifier for the ISupportInfoType_ProtocolSupport_ProtocolSupportIdentifier Variable.
        /// </summary>
        public const uint ISupportInfoType_ProtocolSupport_ProtocolSupportIdentifier = 15060;

        /// <summary>
        /// The identifier for the ISupportInfoType_ImageSet_ImageIdentifier Variable.
        /// </summary>
        public const uint ISupportInfoType_ImageSet_ImageIdentifier = 15062;

        /// <summary>
        /// The identifier for the ComponentType_Lock_Locked Variable.
        /// </summary>
        public const uint ComponentType_Lock_Locked = 15073;

        /// <summary>
        /// The identifier for the ComponentType_Lock_LockingClient Variable.
        /// </summary>
        public const uint ComponentType_Lock_LockingClient = 15074;

        /// <summary>
        /// The identifier for the ComponentType_Lock_LockingUser Variable.
        /// </summary>
        public const uint ComponentType_Lock_LockingUser = 15075;

        /// <summary>
        /// The identifier for the ComponentType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ComponentType_Lock_RemainingLockTime = 15076;

        /// <summary>
        /// The identifier for the ComponentType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ComponentType_Lock_InitLock_InputArguments = 15078;

        /// <summary>
        /// The identifier for the ComponentType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ComponentType_Lock_InitLock_OutputArguments = 15079;

        /// <summary>
        /// The identifier for the ComponentType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ComponentType_Lock_RenewLock_OutputArguments = 15081;

        /// <summary>
        /// The identifier for the ComponentType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ComponentType_Lock_ExitLock_OutputArguments = 15083;

        /// <summary>
        /// The identifier for the ComponentType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ComponentType_Lock_BreakLock_OutputArguments = 15085;

        /// <summary>
        /// The identifier for the ComponentType_Manufacturer Variable.
        /// </summary>
        public const uint ComponentType_Manufacturer = 15086;

        /// <summary>
        /// The identifier for the ComponentType_ManufacturerUri Variable.
        /// </summary>
        public const uint ComponentType_ManufacturerUri = 15087;

        /// <summary>
        /// The identifier for the ComponentType_Model Variable.
        /// </summary>
        public const uint ComponentType_Model = 15088;

        /// <summary>
        /// The identifier for the ComponentType_HardwareRevision Variable.
        /// </summary>
        public const uint ComponentType_HardwareRevision = 15089;

        /// <summary>
        /// The identifier for the ComponentType_SoftwareRevision Variable.
        /// </summary>
        public const uint ComponentType_SoftwareRevision = 15090;

        /// <summary>
        /// The identifier for the ComponentType_DeviceRevision Variable.
        /// </summary>
        public const uint ComponentType_DeviceRevision = 15091;

        /// <summary>
        /// The identifier for the ComponentType_ProductCode Variable.
        /// </summary>
        public const uint ComponentType_ProductCode = 15092;

        /// <summary>
        /// The identifier for the ComponentType_DeviceManual Variable.
        /// </summary>
        public const uint ComponentType_DeviceManual = 15093;

        /// <summary>
        /// The identifier for the ComponentType_DeviceClass Variable.
        /// </summary>
        public const uint ComponentType_DeviceClass = 15094;

        /// <summary>
        /// The identifier for the ComponentType_SerialNumber Variable.
        /// </summary>
        public const uint ComponentType_SerialNumber = 15095;

        /// <summary>
        /// The identifier for the ComponentType_ProductInstanceUri Variable.
        /// </summary>
        public const uint ComponentType_ProductInstanceUri = 15096;

        /// <summary>
        /// The identifier for the ComponentType_RevisionCounter Variable.
        /// </summary>
        public const uint ComponentType_RevisionCounter = 15097;

        /// <summary>
        /// The identifier for the ComponentType_AssetId Variable.
        /// </summary>
        public const uint ComponentType_AssetId = 15098;

        /// <summary>
        /// The identifier for the ComponentType_ComponentName Variable.
        /// </summary>
        public const uint ComponentType_ComponentName = 15099;

        /// <summary>
        /// The identifier for the DeviceType_Lock_Locked Variable.
        /// </summary>
        public const uint DeviceType_Lock_Locked = 6469;

        /// <summary>
        /// The identifier for the DeviceType_Lock_LockingClient Variable.
        /// </summary>
        public const uint DeviceType_Lock_LockingClient = 6188;

        /// <summary>
        /// The identifier for the DeviceType_Lock_LockingUser Variable.
        /// </summary>
        public const uint DeviceType_Lock_LockingUser = 6189;

        /// <summary>
        /// The identifier for the DeviceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint DeviceType_Lock_RemainingLockTime = 6190;

        /// <summary>
        /// The identifier for the DeviceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint DeviceType_Lock_InitLock_InputArguments = 6192;

        /// <summary>
        /// The identifier for the DeviceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint DeviceType_Lock_InitLock_OutputArguments = 6193;

        /// <summary>
        /// The identifier for the DeviceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint DeviceType_Lock_RenewLock_OutputArguments = 6195;

        /// <summary>
        /// The identifier for the DeviceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint DeviceType_Lock_ExitLock_OutputArguments = 6197;

        /// <summary>
        /// The identifier for the DeviceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint DeviceType_Lock_BreakLock_OutputArguments = 6199;

        /// <summary>
        /// The identifier for the DeviceType_Manufacturer Variable.
        /// </summary>
        public const uint DeviceType_Manufacturer = 6003;

        /// <summary>
        /// The identifier for the DeviceType_ManufacturerUri Variable.
        /// </summary>
        public const uint DeviceType_ManufacturerUri = 15100;

        /// <summary>
        /// The identifier for the DeviceType_Model Variable.
        /// </summary>
        public const uint DeviceType_Model = 6004;

        /// <summary>
        /// The identifier for the DeviceType_HardwareRevision Variable.
        /// </summary>
        public const uint DeviceType_HardwareRevision = 6008;

        /// <summary>
        /// The identifier for the DeviceType_SoftwareRevision Variable.
        /// </summary>
        public const uint DeviceType_SoftwareRevision = 6007;

        /// <summary>
        /// The identifier for the DeviceType_DeviceRevision Variable.
        /// </summary>
        public const uint DeviceType_DeviceRevision = 6006;

        /// <summary>
        /// The identifier for the DeviceType_ProductCode Variable.
        /// </summary>
        public const uint DeviceType_ProductCode = 15101;

        /// <summary>
        /// The identifier for the DeviceType_DeviceManual Variable.
        /// </summary>
        public const uint DeviceType_DeviceManual = 6005;

        /// <summary>
        /// The identifier for the DeviceType_DeviceClass Variable.
        /// </summary>
        public const uint DeviceType_DeviceClass = 6470;

        /// <summary>
        /// The identifier for the DeviceType_SerialNumber Variable.
        /// </summary>
        public const uint DeviceType_SerialNumber = 6001;

        /// <summary>
        /// The identifier for the DeviceType_ProductInstanceUri Variable.
        /// </summary>
        public const uint DeviceType_ProductInstanceUri = 15102;

        /// <summary>
        /// The identifier for the DeviceType_RevisionCounter Variable.
        /// </summary>
        public const uint DeviceType_RevisionCounter = 6002;

        /// <summary>
        /// The identifier for the DeviceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint DeviceType_CPIdentifier_Lock_Locked = 6579;

        /// <summary>
        /// The identifier for the DeviceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint DeviceType_CPIdentifier_Lock_LockingClient = 6580;

        /// <summary>
        /// The identifier for the DeviceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint DeviceType_CPIdentifier_Lock_LockingUser = 6581;

        /// <summary>
        /// The identifier for the DeviceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint DeviceType_CPIdentifier_Lock_RemainingLockTime = 6582;

        /// <summary>
        /// The identifier for the DeviceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint DeviceType_CPIdentifier_Lock_InitLock_InputArguments = 6584;

        /// <summary>
        /// The identifier for the DeviceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint DeviceType_CPIdentifier_Lock_InitLock_OutputArguments = 6585;

        /// <summary>
        /// The identifier for the DeviceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint DeviceType_CPIdentifier_Lock_RenewLock_OutputArguments = 6587;

        /// <summary>
        /// The identifier for the DeviceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint DeviceType_CPIdentifier_Lock_ExitLock_OutputArguments = 6589;

        /// <summary>
        /// The identifier for the DeviceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint DeviceType_CPIdentifier_Lock_BreakLock_OutputArguments = 6591;

        /// <summary>
        /// The identifier for the DeviceType_DeviceHealth Variable.
        /// </summary>
        public const uint DeviceType_DeviceHealth = 6208;

        /// <summary>
        /// The identifier for the DeviceType_DeviceTypeImage_ImageIdentifier Variable.
        /// </summary>
        public const uint DeviceType_DeviceTypeImage_ImageIdentifier = 6210;

        /// <summary>
        /// The identifier for the DeviceType_Documentation_DocumentIdentifier Variable.
        /// </summary>
        public const uint DeviceType_Documentation_DocumentIdentifier = 6212;

        /// <summary>
        /// The identifier for the DeviceType_ProtocolSupport_ProtocolSupportIdentifier Variable.
        /// </summary>
        public const uint DeviceType_ProtocolSupport_ProtocolSupportIdentifier = 6214;

        /// <summary>
        /// The identifier for the DeviceType_ImageSet_ImageIdentifier Variable.
        /// </summary>
        public const uint DeviceType_ImageSet_ImageIdentifier = 6216;

        /// <summary>
        /// The identifier for the SoftwareType_Lock_Locked Variable.
        /// </summary>
        public const uint SoftwareType_Lock_Locked = 15116;

        /// <summary>
        /// The identifier for the SoftwareType_Lock_LockingClient Variable.
        /// </summary>
        public const uint SoftwareType_Lock_LockingClient = 15117;

        /// <summary>
        /// The identifier for the SoftwareType_Lock_LockingUser Variable.
        /// </summary>
        public const uint SoftwareType_Lock_LockingUser = 15118;

        /// <summary>
        /// The identifier for the SoftwareType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint SoftwareType_Lock_RemainingLockTime = 15119;

        /// <summary>
        /// The identifier for the SoftwareType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint SoftwareType_Lock_InitLock_InputArguments = 15121;

        /// <summary>
        /// The identifier for the SoftwareType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint SoftwareType_Lock_InitLock_OutputArguments = 15122;

        /// <summary>
        /// The identifier for the SoftwareType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint SoftwareType_Lock_RenewLock_OutputArguments = 15124;

        /// <summary>
        /// The identifier for the SoftwareType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint SoftwareType_Lock_ExitLock_OutputArguments = 15126;

        /// <summary>
        /// The identifier for the SoftwareType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint SoftwareType_Lock_BreakLock_OutputArguments = 15128;

        /// <summary>
        /// The identifier for the SoftwareType_Manufacturer Variable.
        /// </summary>
        public const uint SoftwareType_Manufacturer = 15129;

        /// <summary>
        /// The identifier for the SoftwareType_Model Variable.
        /// </summary>
        public const uint SoftwareType_Model = 15131;

        /// <summary>
        /// The identifier for the SoftwareType_SoftwareRevision Variable.
        /// </summary>
        public const uint SoftwareType_SoftwareRevision = 15133;

        /// <summary>
        /// The identifier for the BlockType_Lock_Locked Variable.
        /// </summary>
        public const uint BlockType_Lock_Locked = 6494;

        /// <summary>
        /// The identifier for the BlockType_Lock_LockingClient Variable.
        /// </summary>
        public const uint BlockType_Lock_LockingClient = 6222;

        /// <summary>
        /// The identifier for the BlockType_Lock_LockingUser Variable.
        /// </summary>
        public const uint BlockType_Lock_LockingUser = 6223;

        /// <summary>
        /// The identifier for the BlockType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint BlockType_Lock_RemainingLockTime = 6224;

        /// <summary>
        /// The identifier for the BlockType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint BlockType_Lock_InitLock_InputArguments = 6226;

        /// <summary>
        /// The identifier for the BlockType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint BlockType_Lock_InitLock_OutputArguments = 6227;

        /// <summary>
        /// The identifier for the BlockType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint BlockType_Lock_RenewLock_OutputArguments = 6229;

        /// <summary>
        /// The identifier for the BlockType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint BlockType_Lock_ExitLock_OutputArguments = 6231;

        /// <summary>
        /// The identifier for the BlockType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint BlockType_Lock_BreakLock_OutputArguments = 6233;

        /// <summary>
        /// The identifier for the BlockType_RevisionCounter Variable.
        /// </summary>
        public const uint BlockType_RevisionCounter = 6009;

        /// <summary>
        /// The identifier for the BlockType_ActualMode Variable.
        /// </summary>
        public const uint BlockType_ActualMode = 6010;

        /// <summary>
        /// The identifier for the BlockType_PermittedMode Variable.
        /// </summary>
        public const uint BlockType_PermittedMode = 6011;

        /// <summary>
        /// The identifier for the BlockType_NormalMode Variable.
        /// </summary>
        public const uint BlockType_NormalMode = 6012;

        /// <summary>
        /// The identifier for the BlockType_TargetMode Variable.
        /// </summary>
        public const uint BlockType_TargetMode = 6013;

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_EnabledState_Id Variable.
        /// </summary>
        public const uint DeviceHealthDiagnosticAlarmType_EnabledState_Id = 15161;

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint DeviceHealthDiagnosticAlarmType_Quality_SourceTimestamp = 15170;

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint DeviceHealthDiagnosticAlarmType_LastSeverity_SourceTimestamp = 15172;

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint DeviceHealthDiagnosticAlarmType_Comment_SourceTimestamp = 15174;

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint DeviceHealthDiagnosticAlarmType_AddComment_InputArguments = 15179;

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint DeviceHealthDiagnosticAlarmType_ConditionRefresh_InputArguments = 15181;

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint DeviceHealthDiagnosticAlarmType_ConditionRefresh2_InputArguments = 15183;

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_AckedState_Id Variable.
        /// </summary>
        public const uint DeviceHealthDiagnosticAlarmType_AckedState_Id = 15185;

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_ConfirmedState_Id Variable.
        /// </summary>
        public const uint DeviceHealthDiagnosticAlarmType_ConfirmedState_Id = 15194;

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_Acknowledge_InputArguments Variable.
        /// </summary>
        public const uint DeviceHealthDiagnosticAlarmType_Acknowledge_InputArguments = 15203;

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_Confirm_InputArguments Variable.
        /// </summary>
        public const uint DeviceHealthDiagnosticAlarmType_Confirm_InputArguments = 15205;

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_ActiveState_Id Variable.
        /// </summary>
        public const uint DeviceHealthDiagnosticAlarmType_ActiveState_Id = 15207;

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_SuppressedState_Id Variable.
        /// </summary>
        public const uint DeviceHealthDiagnosticAlarmType_SuppressedState_Id = 15217;

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_OutOfServiceState_Id Variable.
        /// </summary>
        public const uint DeviceHealthDiagnosticAlarmType_OutOfServiceState_Id = 15226;

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_ShelvingState_CurrentState Variable.
        /// </summary>
        public const uint DeviceHealthDiagnosticAlarmType_ShelvingState_CurrentState = 15235;

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_ShelvingState_CurrentState_Id Variable.
        /// </summary>
        public const uint DeviceHealthDiagnosticAlarmType_ShelvingState_CurrentState_Id = 15236;

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_ShelvingState_LastTransition_Id Variable.
        /// </summary>
        public const uint DeviceHealthDiagnosticAlarmType_ShelvingState_LastTransition_Id = 15241;

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_ShelvingState_UnshelveTime Variable.
        /// </summary>
        public const uint DeviceHealthDiagnosticAlarmType_ShelvingState_UnshelveTime = 15248;

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_ShelvingState_TimedShelve_InputArguments Variable.
        /// </summary>
        public const uint DeviceHealthDiagnosticAlarmType_ShelvingState_TimedShelve_InputArguments = 15250;

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_SilenceState_Id Variable.
        /// </summary>
        public const uint DeviceHealthDiagnosticAlarmType_SilenceState_Id = 15261;

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_LatchedState_Id Variable.
        /// </summary>
        public const uint DeviceHealthDiagnosticAlarmType_LatchedState_Id = 15274;

        /// <summary>
        /// The identifier for the FailureAlarmType_EnabledState_Id Variable.
        /// </summary>
        public const uint FailureAlarmType_EnabledState_Id = 15310;

        /// <summary>
        /// The identifier for the FailureAlarmType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint FailureAlarmType_Quality_SourceTimestamp = 15319;

        /// <summary>
        /// The identifier for the FailureAlarmType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint FailureAlarmType_LastSeverity_SourceTimestamp = 15321;

        /// <summary>
        /// The identifier for the FailureAlarmType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint FailureAlarmType_Comment_SourceTimestamp = 15323;

        /// <summary>
        /// The identifier for the FailureAlarmType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint FailureAlarmType_AddComment_InputArguments = 15328;

        /// <summary>
        /// The identifier for the FailureAlarmType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint FailureAlarmType_ConditionRefresh_InputArguments = 15330;

        /// <summary>
        /// The identifier for the FailureAlarmType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint FailureAlarmType_ConditionRefresh2_InputArguments = 15332;

        /// <summary>
        /// The identifier for the FailureAlarmType_AckedState_Id Variable.
        /// </summary>
        public const uint FailureAlarmType_AckedState_Id = 15334;

        /// <summary>
        /// The identifier for the FailureAlarmType_ConfirmedState_Id Variable.
        /// </summary>
        public const uint FailureAlarmType_ConfirmedState_Id = 15343;

        /// <summary>
        /// The identifier for the FailureAlarmType_Acknowledge_InputArguments Variable.
        /// </summary>
        public const uint FailureAlarmType_Acknowledge_InputArguments = 15352;

        /// <summary>
        /// The identifier for the FailureAlarmType_Confirm_InputArguments Variable.
        /// </summary>
        public const uint FailureAlarmType_Confirm_InputArguments = 15354;

        /// <summary>
        /// The identifier for the FailureAlarmType_ActiveState_Id Variable.
        /// </summary>
        public const uint FailureAlarmType_ActiveState_Id = 15356;

        /// <summary>
        /// The identifier for the FailureAlarmType_SuppressedState_Id Variable.
        /// </summary>
        public const uint FailureAlarmType_SuppressedState_Id = 15366;

        /// <summary>
        /// The identifier for the FailureAlarmType_OutOfServiceState_Id Variable.
        /// </summary>
        public const uint FailureAlarmType_OutOfServiceState_Id = 15375;

        /// <summary>
        /// The identifier for the FailureAlarmType_ShelvingState_CurrentState Variable.
        /// </summary>
        public const uint FailureAlarmType_ShelvingState_CurrentState = 15384;

        /// <summary>
        /// The identifier for the FailureAlarmType_ShelvingState_CurrentState_Id Variable.
        /// </summary>
        public const uint FailureAlarmType_ShelvingState_CurrentState_Id = 15385;

        /// <summary>
        /// The identifier for the FailureAlarmType_ShelvingState_LastTransition_Id Variable.
        /// </summary>
        public const uint FailureAlarmType_ShelvingState_LastTransition_Id = 15390;

        /// <summary>
        /// The identifier for the FailureAlarmType_ShelvingState_UnshelveTime Variable.
        /// </summary>
        public const uint FailureAlarmType_ShelvingState_UnshelveTime = 15397;

        /// <summary>
        /// The identifier for the FailureAlarmType_ShelvingState_TimedShelve_InputArguments Variable.
        /// </summary>
        public const uint FailureAlarmType_ShelvingState_TimedShelve_InputArguments = 15399;

        /// <summary>
        /// The identifier for the FailureAlarmType_SilenceState_Id Variable.
        /// </summary>
        public const uint FailureAlarmType_SilenceState_Id = 15410;

        /// <summary>
        /// The identifier for the FailureAlarmType_LatchedState_Id Variable.
        /// </summary>
        public const uint FailureAlarmType_LatchedState_Id = 15423;

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_EnabledState_Id Variable.
        /// </summary>
        public const uint CheckFunctionAlarmType_EnabledState_Id = 15459;

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint CheckFunctionAlarmType_Quality_SourceTimestamp = 15468;

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint CheckFunctionAlarmType_LastSeverity_SourceTimestamp = 15470;

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint CheckFunctionAlarmType_Comment_SourceTimestamp = 15472;

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint CheckFunctionAlarmType_AddComment_InputArguments = 15477;

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint CheckFunctionAlarmType_ConditionRefresh_InputArguments = 15479;

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint CheckFunctionAlarmType_ConditionRefresh2_InputArguments = 15481;

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_AckedState_Id Variable.
        /// </summary>
        public const uint CheckFunctionAlarmType_AckedState_Id = 15483;

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_ConfirmedState_Id Variable.
        /// </summary>
        public const uint CheckFunctionAlarmType_ConfirmedState_Id = 15492;

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_Acknowledge_InputArguments Variable.
        /// </summary>
        public const uint CheckFunctionAlarmType_Acknowledge_InputArguments = 15501;

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_Confirm_InputArguments Variable.
        /// </summary>
        public const uint CheckFunctionAlarmType_Confirm_InputArguments = 15503;

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_ActiveState_Id Variable.
        /// </summary>
        public const uint CheckFunctionAlarmType_ActiveState_Id = 15505;

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_SuppressedState_Id Variable.
        /// </summary>
        public const uint CheckFunctionAlarmType_SuppressedState_Id = 15515;

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_OutOfServiceState_Id Variable.
        /// </summary>
        public const uint CheckFunctionAlarmType_OutOfServiceState_Id = 15524;

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_ShelvingState_CurrentState Variable.
        /// </summary>
        public const uint CheckFunctionAlarmType_ShelvingState_CurrentState = 15533;

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_ShelvingState_CurrentState_Id Variable.
        /// </summary>
        public const uint CheckFunctionAlarmType_ShelvingState_CurrentState_Id = 15534;

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_ShelvingState_LastTransition_Id Variable.
        /// </summary>
        public const uint CheckFunctionAlarmType_ShelvingState_LastTransition_Id = 15539;

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_ShelvingState_UnshelveTime Variable.
        /// </summary>
        public const uint CheckFunctionAlarmType_ShelvingState_UnshelveTime = 15546;

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_ShelvingState_TimedShelve_InputArguments Variable.
        /// </summary>
        public const uint CheckFunctionAlarmType_ShelvingState_TimedShelve_InputArguments = 15548;

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_SilenceState_Id Variable.
        /// </summary>
        public const uint CheckFunctionAlarmType_SilenceState_Id = 15559;

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_LatchedState_Id Variable.
        /// </summary>
        public const uint CheckFunctionAlarmType_LatchedState_Id = 15572;

        /// <summary>
        /// The identifier for the OffSpecAlarmType_EnabledState_Id Variable.
        /// </summary>
        public const uint OffSpecAlarmType_EnabledState_Id = 15608;

        /// <summary>
        /// The identifier for the OffSpecAlarmType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint OffSpecAlarmType_Quality_SourceTimestamp = 15617;

        /// <summary>
        /// The identifier for the OffSpecAlarmType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint OffSpecAlarmType_LastSeverity_SourceTimestamp = 15619;

        /// <summary>
        /// The identifier for the OffSpecAlarmType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint OffSpecAlarmType_Comment_SourceTimestamp = 15621;

        /// <summary>
        /// The identifier for the OffSpecAlarmType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint OffSpecAlarmType_AddComment_InputArguments = 15626;

        /// <summary>
        /// The identifier for the OffSpecAlarmType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint OffSpecAlarmType_ConditionRefresh_InputArguments = 15628;

        /// <summary>
        /// The identifier for the OffSpecAlarmType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint OffSpecAlarmType_ConditionRefresh2_InputArguments = 15630;

        /// <summary>
        /// The identifier for the OffSpecAlarmType_AckedState_Id Variable.
        /// </summary>
        public const uint OffSpecAlarmType_AckedState_Id = 15632;

        /// <summary>
        /// The identifier for the OffSpecAlarmType_ConfirmedState_Id Variable.
        /// </summary>
        public const uint OffSpecAlarmType_ConfirmedState_Id = 15641;

        /// <summary>
        /// The identifier for the OffSpecAlarmType_Acknowledge_InputArguments Variable.
        /// </summary>
        public const uint OffSpecAlarmType_Acknowledge_InputArguments = 15650;

        /// <summary>
        /// The identifier for the OffSpecAlarmType_Confirm_InputArguments Variable.
        /// </summary>
        public const uint OffSpecAlarmType_Confirm_InputArguments = 15652;

        /// <summary>
        /// The identifier for the OffSpecAlarmType_ActiveState_Id Variable.
        /// </summary>
        public const uint OffSpecAlarmType_ActiveState_Id = 15654;

        /// <summary>
        /// The identifier for the OffSpecAlarmType_SuppressedState_Id Variable.
        /// </summary>
        public const uint OffSpecAlarmType_SuppressedState_Id = 15664;

        /// <summary>
        /// The identifier for the OffSpecAlarmType_OutOfServiceState_Id Variable.
        /// </summary>
        public const uint OffSpecAlarmType_OutOfServiceState_Id = 15673;

        /// <summary>
        /// The identifier for the OffSpecAlarmType_ShelvingState_CurrentState Variable.
        /// </summary>
        public const uint OffSpecAlarmType_ShelvingState_CurrentState = 15682;

        /// <summary>
        /// The identifier for the OffSpecAlarmType_ShelvingState_CurrentState_Id Variable.
        /// </summary>
        public const uint OffSpecAlarmType_ShelvingState_CurrentState_Id = 15683;

        /// <summary>
        /// The identifier for the OffSpecAlarmType_ShelvingState_LastTransition_Id Variable.
        /// </summary>
        public const uint OffSpecAlarmType_ShelvingState_LastTransition_Id = 15688;

        /// <summary>
        /// The identifier for the OffSpecAlarmType_ShelvingState_UnshelveTime Variable.
        /// </summary>
        public const uint OffSpecAlarmType_ShelvingState_UnshelveTime = 15695;

        /// <summary>
        /// The identifier for the OffSpecAlarmType_ShelvingState_TimedShelve_InputArguments Variable.
        /// </summary>
        public const uint OffSpecAlarmType_ShelvingState_TimedShelve_InputArguments = 15697;

        /// <summary>
        /// The identifier for the OffSpecAlarmType_SilenceState_Id Variable.
        /// </summary>
        public const uint OffSpecAlarmType_SilenceState_Id = 15708;

        /// <summary>
        /// The identifier for the OffSpecAlarmType_LatchedState_Id Variable.
        /// </summary>
        public const uint OffSpecAlarmType_LatchedState_Id = 15721;

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_EnabledState_Id Variable.
        /// </summary>
        public const uint MaintenanceRequiredAlarmType_EnabledState_Id = 15757;

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_Quality_SourceTimestamp Variable.
        /// </summary>
        public const uint MaintenanceRequiredAlarmType_Quality_SourceTimestamp = 15766;

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public const uint MaintenanceRequiredAlarmType_LastSeverity_SourceTimestamp = 15768;

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_Comment_SourceTimestamp Variable.
        /// </summary>
        public const uint MaintenanceRequiredAlarmType_Comment_SourceTimestamp = 15770;

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_AddComment_InputArguments Variable.
        /// </summary>
        public const uint MaintenanceRequiredAlarmType_AddComment_InputArguments = 15775;

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public const uint MaintenanceRequiredAlarmType_ConditionRefresh_InputArguments = 15777;

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public const uint MaintenanceRequiredAlarmType_ConditionRefresh2_InputArguments = 15779;

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_AckedState_Id Variable.
        /// </summary>
        public const uint MaintenanceRequiredAlarmType_AckedState_Id = 15781;

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_ConfirmedState_Id Variable.
        /// </summary>
        public const uint MaintenanceRequiredAlarmType_ConfirmedState_Id = 15790;

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_Acknowledge_InputArguments Variable.
        /// </summary>
        public const uint MaintenanceRequiredAlarmType_Acknowledge_InputArguments = 15799;

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_Confirm_InputArguments Variable.
        /// </summary>
        public const uint MaintenanceRequiredAlarmType_Confirm_InputArguments = 15801;

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_ActiveState_Id Variable.
        /// </summary>
        public const uint MaintenanceRequiredAlarmType_ActiveState_Id = 15803;

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_SuppressedState_Id Variable.
        /// </summary>
        public const uint MaintenanceRequiredAlarmType_SuppressedState_Id = 15813;

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_OutOfServiceState_Id Variable.
        /// </summary>
        public const uint MaintenanceRequiredAlarmType_OutOfServiceState_Id = 15822;

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_ShelvingState_CurrentState Variable.
        /// </summary>
        public const uint MaintenanceRequiredAlarmType_ShelvingState_CurrentState = 15831;

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_ShelvingState_CurrentState_Id Variable.
        /// </summary>
        public const uint MaintenanceRequiredAlarmType_ShelvingState_CurrentState_Id = 15832;

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_ShelvingState_LastTransition_Id Variable.
        /// </summary>
        public const uint MaintenanceRequiredAlarmType_ShelvingState_LastTransition_Id = 15837;

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_ShelvingState_UnshelveTime Variable.
        /// </summary>
        public const uint MaintenanceRequiredAlarmType_ShelvingState_UnshelveTime = 15844;

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_ShelvingState_TimedShelve_InputArguments Variable.
        /// </summary>
        public const uint MaintenanceRequiredAlarmType_ShelvingState_TimedShelve_InputArguments = 15846;

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_SilenceState_Id Variable.
        /// </summary>
        public const uint MaintenanceRequiredAlarmType_SilenceState_Id = 15857;

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_LatchedState_Id Variable.
        /// </summary>
        public const uint MaintenanceRequiredAlarmType_LatchedState_Id = 15870;

        /// <summary>
        /// The identifier for the FunctionalGroupType_GroupIdentifier_UIElement Variable.
        /// </summary>
        public const uint FunctionalGroupType_GroupIdentifier_UIElement = 6242;

        /// <summary>
        /// The identifier for the FunctionalGroupType_UIElement Variable.
        /// </summary>
        public const uint FunctionalGroupType_UIElement = 6243;

        /// <summary>
        /// The identifier for the DeviceHealthEnumeration_EnumStrings Variable.
        /// </summary>
        public const uint DeviceHealthEnumeration_EnumStrings = 6450;

        /// <summary>
        /// The identifier for the NetworkType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint NetworkType_CPIdentifier_Lock_Locked = 6496;

        /// <summary>
        /// The identifier for the NetworkType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint NetworkType_CPIdentifier_Lock_LockingClient = 6257;

        /// <summary>
        /// The identifier for the NetworkType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint NetworkType_CPIdentifier_Lock_LockingUser = 6258;

        /// <summary>
        /// The identifier for the NetworkType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint NetworkType_CPIdentifier_Lock_RemainingLockTime = 6259;

        /// <summary>
        /// The identifier for the NetworkType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint NetworkType_CPIdentifier_Lock_InitLock_InputArguments = 6261;

        /// <summary>
        /// The identifier for the NetworkType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint NetworkType_CPIdentifier_Lock_InitLock_OutputArguments = 6262;

        /// <summary>
        /// The identifier for the NetworkType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint NetworkType_CPIdentifier_Lock_RenewLock_OutputArguments = 6264;

        /// <summary>
        /// The identifier for the NetworkType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint NetworkType_CPIdentifier_Lock_ExitLock_OutputArguments = 6266;

        /// <summary>
        /// The identifier for the NetworkType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint NetworkType_CPIdentifier_Lock_BreakLock_OutputArguments = 6268;

        /// <summary>
        /// The identifier for the NetworkType_Lock_Locked Variable.
        /// </summary>
        public const uint NetworkType_Lock_Locked = 6497;

        /// <summary>
        /// The identifier for the NetworkType_Lock_LockingClient Variable.
        /// </summary>
        public const uint NetworkType_Lock_LockingClient = 6296;

        /// <summary>
        /// The identifier for the NetworkType_Lock_LockingUser Variable.
        /// </summary>
        public const uint NetworkType_Lock_LockingUser = 6297;

        /// <summary>
        /// The identifier for the NetworkType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint NetworkType_Lock_RemainingLockTime = 6298;

        /// <summary>
        /// The identifier for the NetworkType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint NetworkType_Lock_InitLock_InputArguments = 6300;

        /// <summary>
        /// The identifier for the NetworkType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint NetworkType_Lock_InitLock_OutputArguments = 6301;

        /// <summary>
        /// The identifier for the NetworkType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint NetworkType_Lock_RenewLock_OutputArguments = 6303;

        /// <summary>
        /// The identifier for the NetworkType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint NetworkType_Lock_ExitLock_OutputArguments = 6305;

        /// <summary>
        /// The identifier for the NetworkType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint NetworkType_Lock_BreakLock_OutputArguments = 6307;

        /// <summary>
        /// The identifier for the ConnectionPointType_Lock_Locked Variable.
        /// </summary>
        public const uint ConnectionPointType_Lock_Locked = 6498;

        /// <summary>
        /// The identifier for the ConnectionPointType_Lock_LockingClient Variable.
        /// </summary>
        public const uint ConnectionPointType_Lock_LockingClient = 6319;

        /// <summary>
        /// The identifier for the ConnectionPointType_Lock_LockingUser Variable.
        /// </summary>
        public const uint ConnectionPointType_Lock_LockingUser = 6320;

        /// <summary>
        /// The identifier for the ConnectionPointType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ConnectionPointType_Lock_RemainingLockTime = 6321;

        /// <summary>
        /// The identifier for the ConnectionPointType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ConnectionPointType_Lock_InitLock_InputArguments = 6323;

        /// <summary>
        /// The identifier for the ConnectionPointType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPointType_Lock_InitLock_OutputArguments = 6324;

        /// <summary>
        /// The identifier for the ConnectionPointType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPointType_Lock_RenewLock_OutputArguments = 6326;

        /// <summary>
        /// The identifier for the ConnectionPointType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPointType_Lock_ExitLock_OutputArguments = 6328;

        /// <summary>
        /// The identifier for the ConnectionPointType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPointType_Lock_BreakLock_OutputArguments = 6330;

        /// <summary>
        /// The identifier for the ConnectionPointType_NetworkIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ConnectionPointType_NetworkIdentifier_Lock_Locked = 6601;

        /// <summary>
        /// The identifier for the ConnectionPointType_NetworkIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ConnectionPointType_NetworkIdentifier_Lock_LockingClient = 6602;

        /// <summary>
        /// The identifier for the ConnectionPointType_NetworkIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ConnectionPointType_NetworkIdentifier_Lock_LockingUser = 6603;

        /// <summary>
        /// The identifier for the ConnectionPointType_NetworkIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ConnectionPointType_NetworkIdentifier_Lock_RemainingLockTime = 6604;

        /// <summary>
        /// The identifier for the ConnectionPointType_NetworkIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ConnectionPointType_NetworkIdentifier_Lock_InitLock_InputArguments = 6606;

        /// <summary>
        /// The identifier for the ConnectionPointType_NetworkIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPointType_NetworkIdentifier_Lock_InitLock_OutputArguments = 6607;

        /// <summary>
        /// The identifier for the ConnectionPointType_NetworkIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPointType_NetworkIdentifier_Lock_RenewLock_OutputArguments = 6609;

        /// <summary>
        /// The identifier for the ConnectionPointType_NetworkIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPointType_NetworkIdentifier_Lock_ExitLock_OutputArguments = 6611;

        /// <summary>
        /// The identifier for the ConnectionPointType_NetworkIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPointType_NetworkIdentifier_Lock_BreakLock_OutputArguments = 6613;

        /// <summary>
        /// The identifier for the TransferServicesType_TransferToDevice_OutputArguments Variable.
        /// </summary>
        public const uint TransferServicesType_TransferToDevice_OutputArguments = 6528;

        /// <summary>
        /// The identifier for the TransferServicesType_TransferFromDevice_OutputArguments Variable.
        /// </summary>
        public const uint TransferServicesType_TransferFromDevice_OutputArguments = 6530;

        /// <summary>
        /// The identifier for the TransferServicesType_FetchTransferResultData_InputArguments Variable.
        /// </summary>
        public const uint TransferServicesType_FetchTransferResultData_InputArguments = 6532;

        /// <summary>
        /// The identifier for the TransferServicesType_FetchTransferResultData_OutputArguments Variable.
        /// </summary>
        public const uint TransferServicesType_FetchTransferResultData_OutputArguments = 6533;

        /// <summary>
        /// The identifier for the MaxInactiveLockTime Variable.
        /// </summary>
        public const uint MaxInactiveLockTime = 6387;

        /// <summary>
        /// The identifier for the LockingServicesType_DefaultInstanceBrowseName Variable.
        /// </summary>
        public const uint LockingServicesType_DefaultInstanceBrowseName = 15890;

        /// <summary>
        /// The identifier for the LockingServicesType_Locked Variable.
        /// </summary>
        public const uint LockingServicesType_Locked = 6534;

        /// <summary>
        /// The identifier for the LockingServicesType_LockingClient Variable.
        /// </summary>
        public const uint LockingServicesType_LockingClient = 6390;

        /// <summary>
        /// The identifier for the LockingServicesType_LockingUser Variable.
        /// </summary>
        public const uint LockingServicesType_LockingUser = 6391;

        /// <summary>
        /// The identifier for the LockingServicesType_RemainingLockTime Variable.
        /// </summary>
        public const uint LockingServicesType_RemainingLockTime = 6392;

        /// <summary>
        /// The identifier for the LockingServicesType_InitLock_InputArguments Variable.
        /// </summary>
        public const uint LockingServicesType_InitLock_InputArguments = 6394;

        /// <summary>
        /// The identifier for the LockingServicesType_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint LockingServicesType_InitLock_OutputArguments = 6395;

        /// <summary>
        /// The identifier for the LockingServicesType_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint LockingServicesType_RenewLock_OutputArguments = 6397;

        /// <summary>
        /// The identifier for the LockingServicesType_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint LockingServicesType_ExitLock_OutputArguments = 6399;

        /// <summary>
        /// The identifier for the LockingServicesType_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint LockingServicesType_BreakLock_OutputArguments = 6401;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_PrepareForUpdate_CurrentState Variable.
        /// </summary>
        public const uint SoftwareUpdateType_PrepareForUpdate_CurrentState = 5;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_PrepareForUpdate_CurrentState_Id Variable.
        /// </summary>
        public const uint SoftwareUpdateType_PrepareForUpdate_CurrentState_Id = 6;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_PrepareForUpdate_LastTransition_Id Variable.
        /// </summary>
        public const uint SoftwareUpdateType_PrepareForUpdate_LastTransition_Id = 11;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Installation_CurrentState Variable.
        /// </summary>
        public const uint SoftwareUpdateType_Installation_CurrentState = 41;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Installation_CurrentState_Id Variable.
        /// </summary>
        public const uint SoftwareUpdateType_Installation_CurrentState_Id = 42;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Installation_LastTransition_Id Variable.
        /// </summary>
        public const uint SoftwareUpdateType_Installation_LastTransition_Id = 47;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Installation_InstallSoftwarePackage_InputArguments Variable.
        /// </summary>
        public const uint SoftwareUpdateType_Installation_InstallSoftwarePackage_InputArguments = 57;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Installation_InstallFiles_InputArguments Variable.
        /// </summary>
        public const uint SoftwareUpdateType_Installation_InstallFiles_InputArguments = 60;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_PowerCycle_CurrentState Variable.
        /// </summary>
        public const uint SoftwareUpdateType_PowerCycle_CurrentState = 77;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_PowerCycle_CurrentState_Id Variable.
        /// </summary>
        public const uint SoftwareUpdateType_PowerCycle_CurrentState_Id = 78;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_PowerCycle_LastTransition_Id Variable.
        /// </summary>
        public const uint SoftwareUpdateType_PowerCycle_LastTransition_Id = 83;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Confirmation_CurrentState Variable.
        /// </summary>
        public const uint SoftwareUpdateType_Confirmation_CurrentState = 99;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Confirmation_CurrentState_Id Variable.
        /// </summary>
        public const uint SoftwareUpdateType_Confirmation_CurrentState_Id = 100;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Confirmation_LastTransition_Id Variable.
        /// </summary>
        public const uint SoftwareUpdateType_Confirmation_LastTransition_Id = 105;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Confirmation_ConfirmationTimeout Variable.
        /// </summary>
        public const uint SoftwareUpdateType_Confirmation_ConfirmationTimeout = 113;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Parameters_ClientProcessingTimeout Variable.
        /// </summary>
        public const uint SoftwareUpdateType_Parameters_ClientProcessingTimeout = 123;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Parameters_GenerateFileForRead_InputArguments Variable.
        /// </summary>
        public const uint SoftwareUpdateType_Parameters_GenerateFileForRead_InputArguments = 125;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Parameters_GenerateFileForRead_OutputArguments Variable.
        /// </summary>
        public const uint SoftwareUpdateType_Parameters_GenerateFileForRead_OutputArguments = 126;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Parameters_GenerateFileForWrite_InputArguments Variable.
        /// </summary>
        public const uint SoftwareUpdateType_Parameters_GenerateFileForWrite_InputArguments = 128;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Parameters_GenerateFileForWrite_OutputArguments Variable.
        /// </summary>
        public const uint SoftwareUpdateType_Parameters_GenerateFileForWrite_OutputArguments = 129;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Parameters_CloseAndCommit_InputArguments Variable.
        /// </summary>
        public const uint SoftwareUpdateType_Parameters_CloseAndCommit_InputArguments = 131;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Parameters_CloseAndCommit_OutputArguments Variable.
        /// </summary>
        public const uint SoftwareUpdateType_Parameters_CloseAndCommit_OutputArguments = 132;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_UpdateStatus Variable.
        /// </summary>
        public const uint SoftwareUpdateType_UpdateStatus = 133;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_VendorErrorCode Variable.
        /// </summary>
        public const uint SoftwareUpdateType_VendorErrorCode = 402;

        /// <summary>
        /// The identifier for the SoftwareUpdateType_DefaultInstanceBrowseName Variable.
        /// </summary>
        public const uint SoftwareUpdateType_DefaultInstanceBrowseName = 134;

        /// <summary>
        /// The identifier for the SoftwareLoadingType_UpdateKey Variable.
        /// </summary>
        public const uint SoftwareLoadingType_UpdateKey = 136;

        /// <summary>
        /// The identifier for the PackageLoadingType_CurrentVersion_Manufacturer Variable.
        /// </summary>
        public const uint PackageLoadingType_CurrentVersion_Manufacturer = 345;

        /// <summary>
        /// The identifier for the PackageLoadingType_CurrentVersion_ManufacturerUri Variable.
        /// </summary>
        public const uint PackageLoadingType_CurrentVersion_ManufacturerUri = 346;

        /// <summary>
        /// The identifier for the PackageLoadingType_CurrentVersion_SoftwareRevision Variable.
        /// </summary>
        public const uint PackageLoadingType_CurrentVersion_SoftwareRevision = 347;

        /// <summary>
        /// The identifier for the PackageLoadingType_FileTransfer_ClientProcessingTimeout Variable.
        /// </summary>
        public const uint PackageLoadingType_FileTransfer_ClientProcessingTimeout = 141;

        /// <summary>
        /// The identifier for the PackageLoadingType_FileTransfer_GenerateFileForRead_InputArguments Variable.
        /// </summary>
        public const uint PackageLoadingType_FileTransfer_GenerateFileForRead_InputArguments = 143;

        /// <summary>
        /// The identifier for the PackageLoadingType_FileTransfer_GenerateFileForRead_OutputArguments Variable.
        /// </summary>
        public const uint PackageLoadingType_FileTransfer_GenerateFileForRead_OutputArguments = 144;

        /// <summary>
        /// The identifier for the PackageLoadingType_FileTransfer_GenerateFileForWrite_InputArguments Variable.
        /// </summary>
        public const uint PackageLoadingType_FileTransfer_GenerateFileForWrite_InputArguments = 146;

        /// <summary>
        /// The identifier for the PackageLoadingType_FileTransfer_GenerateFileForWrite_OutputArguments Variable.
        /// </summary>
        public const uint PackageLoadingType_FileTransfer_GenerateFileForWrite_OutputArguments = 147;

        /// <summary>
        /// The identifier for the PackageLoadingType_FileTransfer_CloseAndCommit_InputArguments Variable.
        /// </summary>
        public const uint PackageLoadingType_FileTransfer_CloseAndCommit_InputArguments = 149;

        /// <summary>
        /// The identifier for the PackageLoadingType_FileTransfer_CloseAndCommit_OutputArguments Variable.
        /// </summary>
        public const uint PackageLoadingType_FileTransfer_CloseAndCommit_OutputArguments = 150;

        /// <summary>
        /// The identifier for the PackageLoadingType_ErrorMessage Variable.
        /// </summary>
        public const uint PackageLoadingType_ErrorMessage = 151;

        /// <summary>
        /// The identifier for the PackageLoadingType_WriteBlockSize Variable.
        /// </summary>
        public const uint PackageLoadingType_WriteBlockSize = 152;

        /// <summary>
        /// The identifier for the DirectLoadingType_CurrentVersion_Manufacturer Variable.
        /// </summary>
        public const uint DirectLoadingType_CurrentVersion_Manufacturer = 352;

        /// <summary>
        /// The identifier for the DirectLoadingType_CurrentVersion_ManufacturerUri Variable.
        /// </summary>
        public const uint DirectLoadingType_CurrentVersion_ManufacturerUri = 353;

        /// <summary>
        /// The identifier for the DirectLoadingType_CurrentVersion_SoftwareRevision Variable.
        /// </summary>
        public const uint DirectLoadingType_CurrentVersion_SoftwareRevision = 354;

        /// <summary>
        /// The identifier for the DirectLoadingType_FileTransfer_ClientProcessingTimeout Variable.
        /// </summary>
        public const uint DirectLoadingType_FileTransfer_ClientProcessingTimeout = 157;

        /// <summary>
        /// The identifier for the DirectLoadingType_FileTransfer_GenerateFileForRead_InputArguments Variable.
        /// </summary>
        public const uint DirectLoadingType_FileTransfer_GenerateFileForRead_InputArguments = 159;

        /// <summary>
        /// The identifier for the DirectLoadingType_FileTransfer_GenerateFileForRead_OutputArguments Variable.
        /// </summary>
        public const uint DirectLoadingType_FileTransfer_GenerateFileForRead_OutputArguments = 160;

        /// <summary>
        /// The identifier for the DirectLoadingType_FileTransfer_GenerateFileForWrite_InputArguments Variable.
        /// </summary>
        public const uint DirectLoadingType_FileTransfer_GenerateFileForWrite_InputArguments = 162;

        /// <summary>
        /// The identifier for the DirectLoadingType_FileTransfer_GenerateFileForWrite_OutputArguments Variable.
        /// </summary>
        public const uint DirectLoadingType_FileTransfer_GenerateFileForWrite_OutputArguments = 163;

        /// <summary>
        /// The identifier for the DirectLoadingType_FileTransfer_CloseAndCommit_InputArguments Variable.
        /// </summary>
        public const uint DirectLoadingType_FileTransfer_CloseAndCommit_InputArguments = 165;

        /// <summary>
        /// The identifier for the DirectLoadingType_FileTransfer_CloseAndCommit_OutputArguments Variable.
        /// </summary>
        public const uint DirectLoadingType_FileTransfer_CloseAndCommit_OutputArguments = 166;

        /// <summary>
        /// The identifier for the DirectLoadingType_UpdateBehavior Variable.
        /// </summary>
        public const uint DirectLoadingType_UpdateBehavior = 169;

        /// <summary>
        /// The identifier for the DirectLoadingType_WriteTimeout Variable.
        /// </summary>
        public const uint DirectLoadingType_WriteTimeout = 170;

        /// <summary>
        /// The identifier for the CachedLoadingType_CurrentVersion_Manufacturer Variable.
        /// </summary>
        public const uint CachedLoadingType_CurrentVersion_Manufacturer = 359;

        /// <summary>
        /// The identifier for the CachedLoadingType_CurrentVersion_ManufacturerUri Variable.
        /// </summary>
        public const uint CachedLoadingType_CurrentVersion_ManufacturerUri = 360;

        /// <summary>
        /// The identifier for the CachedLoadingType_CurrentVersion_SoftwareRevision Variable.
        /// </summary>
        public const uint CachedLoadingType_CurrentVersion_SoftwareRevision = 361;

        /// <summary>
        /// The identifier for the CachedLoadingType_FileTransfer_ClientProcessingTimeout Variable.
        /// </summary>
        public const uint CachedLoadingType_FileTransfer_ClientProcessingTimeout = 175;

        /// <summary>
        /// The identifier for the CachedLoadingType_FileTransfer_GenerateFileForRead_InputArguments Variable.
        /// </summary>
        public const uint CachedLoadingType_FileTransfer_GenerateFileForRead_InputArguments = 177;

        /// <summary>
        /// The identifier for the CachedLoadingType_FileTransfer_GenerateFileForRead_OutputArguments Variable.
        /// </summary>
        public const uint CachedLoadingType_FileTransfer_GenerateFileForRead_OutputArguments = 178;

        /// <summary>
        /// The identifier for the CachedLoadingType_FileTransfer_GenerateFileForWrite_InputArguments Variable.
        /// </summary>
        public const uint CachedLoadingType_FileTransfer_GenerateFileForWrite_InputArguments = 180;

        /// <summary>
        /// The identifier for the CachedLoadingType_FileTransfer_GenerateFileForWrite_OutputArguments Variable.
        /// </summary>
        public const uint CachedLoadingType_FileTransfer_GenerateFileForWrite_OutputArguments = 181;

        /// <summary>
        /// The identifier for the CachedLoadingType_FileTransfer_CloseAndCommit_InputArguments Variable.
        /// </summary>
        public const uint CachedLoadingType_FileTransfer_CloseAndCommit_InputArguments = 183;

        /// <summary>
        /// The identifier for the CachedLoadingType_FileTransfer_CloseAndCommit_OutputArguments Variable.
        /// </summary>
        public const uint CachedLoadingType_FileTransfer_CloseAndCommit_OutputArguments = 184;

        /// <summary>
        /// The identifier for the CachedLoadingType_PendingVersion_Manufacturer Variable.
        /// </summary>
        public const uint CachedLoadingType_PendingVersion_Manufacturer = 366;

        /// <summary>
        /// The identifier for the CachedLoadingType_PendingVersion_ManufacturerUri Variable.
        /// </summary>
        public const uint CachedLoadingType_PendingVersion_ManufacturerUri = 367;

        /// <summary>
        /// The identifier for the CachedLoadingType_PendingVersion_SoftwareRevision Variable.
        /// </summary>
        public const uint CachedLoadingType_PendingVersion_SoftwareRevision = 368;

        /// <summary>
        /// The identifier for the CachedLoadingType_FallbackVersion_Manufacturer Variable.
        /// </summary>
        public const uint CachedLoadingType_FallbackVersion_Manufacturer = 373;

        /// <summary>
        /// The identifier for the CachedLoadingType_FallbackVersion_ManufacturerUri Variable.
        /// </summary>
        public const uint CachedLoadingType_FallbackVersion_ManufacturerUri = 374;

        /// <summary>
        /// The identifier for the CachedLoadingType_FallbackVersion_SoftwareRevision Variable.
        /// </summary>
        public const uint CachedLoadingType_FallbackVersion_SoftwareRevision = 375;

        /// <summary>
        /// The identifier for the CachedLoadingType_GetUpdateBehavior_InputArguments Variable.
        /// </summary>
        public const uint CachedLoadingType_GetUpdateBehavior_InputArguments = 190;

        /// <summary>
        /// The identifier for the CachedLoadingType_GetUpdateBehavior_OutputArguments Variable.
        /// </summary>
        public const uint CachedLoadingType_GetUpdateBehavior_OutputArguments = 191;

        /// <summary>
        /// The identifier for the FileSystemLoadingType_FileSystem_CreateDirectory_InputArguments Variable.
        /// </summary>
        public const uint FileSystemLoadingType_FileSystem_CreateDirectory_InputArguments = 196;

        /// <summary>
        /// The identifier for the FileSystemLoadingType_FileSystem_CreateDirectory_OutputArguments Variable.
        /// </summary>
        public const uint FileSystemLoadingType_FileSystem_CreateDirectory_OutputArguments = 197;

        /// <summary>
        /// The identifier for the FileSystemLoadingType_FileSystem_CreateFile_InputArguments Variable.
        /// </summary>
        public const uint FileSystemLoadingType_FileSystem_CreateFile_InputArguments = 199;

        /// <summary>
        /// The identifier for the FileSystemLoadingType_FileSystem_CreateFile_OutputArguments Variable.
        /// </summary>
        public const uint FileSystemLoadingType_FileSystem_CreateFile_OutputArguments = 200;

        /// <summary>
        /// The identifier for the FileSystemLoadingType_FileSystem_DeleteFileSystemObject_InputArguments Variable.
        /// </summary>
        public const uint FileSystemLoadingType_FileSystem_DeleteFileSystemObject_InputArguments = 202;

        /// <summary>
        /// The identifier for the FileSystemLoadingType_FileSystem_MoveOrCopy_InputArguments Variable.
        /// </summary>
        public const uint FileSystemLoadingType_FileSystem_MoveOrCopy_InputArguments = 204;

        /// <summary>
        /// The identifier for the FileSystemLoadingType_FileSystem_MoveOrCopy_OutputArguments Variable.
        /// </summary>
        public const uint FileSystemLoadingType_FileSystem_MoveOrCopy_OutputArguments = 205;

        /// <summary>
        /// The identifier for the FileSystemLoadingType_GetUpdateBehavior_InputArguments Variable.
        /// </summary>
        public const uint FileSystemLoadingType_GetUpdateBehavior_InputArguments = 207;

        /// <summary>
        /// The identifier for the FileSystemLoadingType_GetUpdateBehavior_OutputArguments Variable.
        /// </summary>
        public const uint FileSystemLoadingType_GetUpdateBehavior_OutputArguments = 208;

        /// <summary>
        /// The identifier for the FileSystemLoadingType_ValidateFiles_InputArguments Variable.
        /// </summary>
        public const uint FileSystemLoadingType_ValidateFiles_InputArguments = 210;

        /// <summary>
        /// The identifier for the FileSystemLoadingType_ValidateFiles_OutputArguments Variable.
        /// </summary>
        public const uint FileSystemLoadingType_ValidateFiles_OutputArguments = 211;

        /// <summary>
        /// The identifier for the SoftwareVersionType_Manufacturer Variable.
        /// </summary>
        public const uint SoftwareVersionType_Manufacturer = 380;

        /// <summary>
        /// The identifier for the SoftwareVersionType_ManufacturerUri Variable.
        /// </summary>
        public const uint SoftwareVersionType_ManufacturerUri = 381;

        /// <summary>
        /// The identifier for the SoftwareVersionType_SoftwareRevision Variable.
        /// </summary>
        public const uint SoftwareVersionType_SoftwareRevision = 382;

        /// <summary>
        /// The identifier for the SoftwareVersionType_PatchIdentifiers Variable.
        /// </summary>
        public const uint SoftwareVersionType_PatchIdentifiers = 383;

        /// <summary>
        /// The identifier for the SoftwareVersionType_ReleaseDate Variable.
        /// </summary>
        public const uint SoftwareVersionType_ReleaseDate = 384;

        /// <summary>
        /// The identifier for the SoftwareVersionType_ChangeLogReference Variable.
        /// </summary>
        public const uint SoftwareVersionType_ChangeLogReference = 385;

        /// <summary>
        /// The identifier for the SoftwareVersionType_Hash Variable.
        /// </summary>
        public const uint SoftwareVersionType_Hash = 386;

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_CurrentState_Id Variable.
        /// </summary>
        public const uint PrepareForUpdateStateMachineType_CurrentState_Id = 215;

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_LastTransition_Id Variable.
        /// </summary>
        public const uint PrepareForUpdateStateMachineType_LastTransition_Id = 220;

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_PercentComplete Variable.
        /// </summary>
        public const uint PrepareForUpdateStateMachineType_PercentComplete = 227;

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_Idle_StateNumber Variable.
        /// </summary>
        public const uint PrepareForUpdateStateMachineType_Idle_StateNumber = 232;

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_Preparing_StateNumber Variable.
        /// </summary>
        public const uint PrepareForUpdateStateMachineType_Preparing_StateNumber = 234;

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_PreparedForUpdate_StateNumber Variable.
        /// </summary>
        public const uint PrepareForUpdateStateMachineType_PreparedForUpdate_StateNumber = 236;

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_Resuming_StateNumber Variable.
        /// </summary>
        public const uint PrepareForUpdateStateMachineType_Resuming_StateNumber = 238;

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_IdleToPreparing_TransitionNumber Variable.
        /// </summary>
        public const uint PrepareForUpdateStateMachineType_IdleToPreparing_TransitionNumber = 240;

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_PreparingToIdle_TransitionNumber Variable.
        /// </summary>
        public const uint PrepareForUpdateStateMachineType_PreparingToIdle_TransitionNumber = 242;

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_PreparingToPreparedForUpdate_TransitionNumber Variable.
        /// </summary>
        public const uint PrepareForUpdateStateMachineType_PreparingToPreparedForUpdate_TransitionNumber = 244;

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_PreparedForUpdateToResuming_TransitionNumber Variable.
        /// </summary>
        public const uint PrepareForUpdateStateMachineType_PreparedForUpdateToResuming_TransitionNumber = 246;

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_ResumingToIdle_TransitionNumber Variable.
        /// </summary>
        public const uint PrepareForUpdateStateMachineType_ResumingToIdle_TransitionNumber = 248;

        /// <summary>
        /// The identifier for the InstallationStateMachineType_CurrentState_Id Variable.
        /// </summary>
        public const uint InstallationStateMachineType_CurrentState_Id = 251;

        /// <summary>
        /// The identifier for the InstallationStateMachineType_LastTransition_Id Variable.
        /// </summary>
        public const uint InstallationStateMachineType_LastTransition_Id = 256;

        /// <summary>
        /// The identifier for the InstallationStateMachineType_PercentComplete Variable.
        /// </summary>
        public const uint InstallationStateMachineType_PercentComplete = 263;

        /// <summary>
        /// The identifier for the InstallationStateMachineType_InstallationDelay Variable.
        /// </summary>
        public const uint InstallationStateMachineType_InstallationDelay = 264;

        /// <summary>
        /// The identifier for the InstallationStateMachineType_InstallSoftwarePackage_InputArguments Variable.
        /// </summary>
        public const uint InstallationStateMachineType_InstallSoftwarePackage_InputArguments = 266;

        /// <summary>
        /// The identifier for the InstallationStateMachineType_InstallSoftwarePackage_OutputArguments Variable.
        /// </summary>
        public const uint InstallationStateMachineType_InstallSoftwarePackage_OutputArguments = 267;

        /// <summary>
        /// The identifier for the InstallationStateMachineType_InstallFiles_InputArguments Variable.
        /// </summary>
        public const uint InstallationStateMachineType_InstallFiles_InputArguments = 269;

        /// <summary>
        /// The identifier for the InstallationStateMachineType_Idle_StateNumber Variable.
        /// </summary>
        public const uint InstallationStateMachineType_Idle_StateNumber = 272;

        /// <summary>
        /// The identifier for the InstallationStateMachineType_Installing_StateNumber Variable.
        /// </summary>
        public const uint InstallationStateMachineType_Installing_StateNumber = 274;

        /// <summary>
        /// The identifier for the InstallationStateMachineType_Error_StateNumber Variable.
        /// </summary>
        public const uint InstallationStateMachineType_Error_StateNumber = 276;

        /// <summary>
        /// The identifier for the InstallationStateMachineType_IdleToInstalling_TransitionNumber Variable.
        /// </summary>
        public const uint InstallationStateMachineType_IdleToInstalling_TransitionNumber = 387;

        /// <summary>
        /// The identifier for the InstallationStateMachineType_InstallingToIdle_TransitionNumber Variable.
        /// </summary>
        public const uint InstallationStateMachineType_InstallingToIdle_TransitionNumber = 280;

        /// <summary>
        /// The identifier for the InstallationStateMachineType_InstallingToError_TransitionNumber Variable.
        /// </summary>
        public const uint InstallationStateMachineType_InstallingToError_TransitionNumber = 282;

        /// <summary>
        /// The identifier for the InstallationStateMachineType_ErrorToIdle_TransitionNumber Variable.
        /// </summary>
        public const uint InstallationStateMachineType_ErrorToIdle_TransitionNumber = 284;

        /// <summary>
        /// The identifier for the PowerCycleStateMachineType_CurrentState_Id Variable.
        /// </summary>
        public const uint PowerCycleStateMachineType_CurrentState_Id = 287;

        /// <summary>
        /// The identifier for the PowerCycleStateMachineType_LastTransition_Id Variable.
        /// </summary>
        public const uint PowerCycleStateMachineType_LastTransition_Id = 292;

        /// <summary>
        /// The identifier for the PowerCycleStateMachineType_NotWaitingForPowerCycle_StateNumber Variable.
        /// </summary>
        public const uint PowerCycleStateMachineType_NotWaitingForPowerCycle_StateNumber = 300;

        /// <summary>
        /// The identifier for the PowerCycleStateMachineType_WaitingForPowerCycle_StateNumber Variable.
        /// </summary>
        public const uint PowerCycleStateMachineType_WaitingForPowerCycle_StateNumber = 302;

        /// <summary>
        /// The identifier for the PowerCycleStateMachineType_NotWaitingForPowerCycleToWaitingForPowerCycle_TransitionNumber Variable.
        /// </summary>
        public const uint PowerCycleStateMachineType_NotWaitingForPowerCycleToWaitingForPowerCycle_TransitionNumber = 304;

        /// <summary>
        /// The identifier for the PowerCycleStateMachineType_WaitingForPowerCycleToNotWaitingForPowerCycle_TransitionNumber Variable.
        /// </summary>
        public const uint PowerCycleStateMachineType_WaitingForPowerCycleToNotWaitingForPowerCycle_TransitionNumber = 306;

        /// <summary>
        /// The identifier for the ConfirmationStateMachineType_CurrentState_Id Variable.
        /// </summary>
        public const uint ConfirmationStateMachineType_CurrentState_Id = 309;

        /// <summary>
        /// The identifier for the ConfirmationStateMachineType_LastTransition_Id Variable.
        /// </summary>
        public const uint ConfirmationStateMachineType_LastTransition_Id = 314;

        /// <summary>
        /// The identifier for the ConfirmationStateMachineType_ConfirmationTimeout Variable.
        /// </summary>
        public const uint ConfirmationStateMachineType_ConfirmationTimeout = 322;

        /// <summary>
        /// The identifier for the ConfirmationStateMachineType_NotWaitingForConfirm_StateNumber Variable.
        /// </summary>
        public const uint ConfirmationStateMachineType_NotWaitingForConfirm_StateNumber = 324;

        /// <summary>
        /// The identifier for the ConfirmationStateMachineType_WaitingForConfirm_StateNumber Variable.
        /// </summary>
        public const uint ConfirmationStateMachineType_WaitingForConfirm_StateNumber = 326;

        /// <summary>
        /// The identifier for the ConfirmationStateMachineType_NotWaitingForConfirmToWaitingForConfirm_TransitionNumber Variable.
        /// </summary>
        public const uint ConfirmationStateMachineType_NotWaitingForConfirmToWaitingForConfirm_TransitionNumber = 328;

        /// <summary>
        /// The identifier for the ConfirmationStateMachineType_WaitingForConfirmToNotWaitingForConfirm_TransitionNumber Variable.
        /// </summary>
        public const uint ConfirmationStateMachineType_WaitingForConfirmToNotWaitingForConfirm_TransitionNumber = 330;

        /// <summary>
        /// The identifier for the SoftwareVersionFileType_EnumStrings Variable.
        /// </summary>
        public const uint SoftwareVersionFileType_EnumStrings = 332;

        /// <summary>
        /// The identifier for the UpdateBehavior_OptionSetValues Variable.
        /// </summary>
        public const uint UpdateBehavior_OptionSetValues = 388;

        /// <summary>
        /// The identifier for the OpcUaDi_BinarySchema Variable.
        /// </summary>
        public const uint OpcUaDi_BinarySchema = 6435;

        /// <summary>
        /// The identifier for the OpcUaDi_BinarySchema_NamespaceUri Variable.
        /// </summary>
        public const uint OpcUaDi_BinarySchema_NamespaceUri = 6437;

        /// <summary>
        /// The identifier for the OpcUaDi_BinarySchema_Deprecated Variable.
        /// </summary>
        public const uint OpcUaDi_BinarySchema_Deprecated = 15893;

        /// <summary>
        /// The identifier for the OpcUaDi_BinarySchema_FetchResultDataType Variable.
        /// </summary>
        public const uint OpcUaDi_BinarySchema_FetchResultDataType = 6555;

        /// <summary>
        /// The identifier for the OpcUaDi_BinarySchema_TransferResultErrorDataType Variable.
        /// </summary>
        public const uint OpcUaDi_BinarySchema_TransferResultErrorDataType = 15894;

        /// <summary>
        /// The identifier for the OpcUaDi_BinarySchema_TransferResultDataDataType Variable.
        /// </summary>
        public const uint OpcUaDi_BinarySchema_TransferResultDataDataType = 15897;

        /// <summary>
        /// The identifier for the OpcUaDi_BinarySchema_ParameterResultDataType Variable.
        /// </summary>
        public const uint OpcUaDi_BinarySchema_ParameterResultDataType = 6564;

        /// <summary>
        /// The identifier for the OpcUaDi_XmlSchema Variable.
        /// </summary>
        public const uint OpcUaDi_XmlSchema = 6423;

        /// <summary>
        /// The identifier for the OpcUaDi_XmlSchema_NamespaceUri Variable.
        /// </summary>
        public const uint OpcUaDi_XmlSchema_NamespaceUri = 6425;

        /// <summary>
        /// The identifier for the OpcUaDi_XmlSchema_Deprecated Variable.
        /// </summary>
        public const uint OpcUaDi_XmlSchema_Deprecated = 15902;

        /// <summary>
        /// The identifier for the OpcUaDi_XmlSchema_FetchResultDataType Variable.
        /// </summary>
        public const uint OpcUaDi_XmlSchema_FetchResultDataType = 6539;

        /// <summary>
        /// The identifier for the OpcUaDi_XmlSchema_TransferResultErrorDataType Variable.
        /// </summary>
        public const uint OpcUaDi_XmlSchema_TransferResultErrorDataType = 15903;

        /// <summary>
        /// The identifier for the OpcUaDi_XmlSchema_TransferResultDataDataType Variable.
        /// </summary>
        public const uint OpcUaDi_XmlSchema_TransferResultDataDataType = 15906;

        /// <summary>
        /// The identifier for the OpcUaDi_XmlSchema_ParameterResultDataType Variable.
        /// </summary>
        public const uint OpcUaDi_XmlSchema_ParameterResultDataType = 6548;
    }
    #endregion

    #region VariableType Identifiers
    /// <summary>
    /// A class that declares constants for all VariableTypes in the Model Design.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class VariableTypes
    {
        /// <summary>
        /// The identifier for the UIElementType VariableType.
        /// </summary>
        public const uint UIElementType = 6246;
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
        /// The identifier for the DeviceHealthEnumeration DataType.
        /// </summary>
        public static readonly ExpandedNodeId DeviceHealthEnumeration = new ExpandedNodeId(Opc.Ua.Di.DataTypes.DeviceHealthEnumeration, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FetchResultDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId FetchResultDataType = new ExpandedNodeId(Opc.Ua.Di.DataTypes.FetchResultDataType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TransferResultErrorDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId TransferResultErrorDataType = new ExpandedNodeId(Opc.Ua.Di.DataTypes.TransferResultErrorDataType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TransferResultDataDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId TransferResultDataDataType = new ExpandedNodeId(Opc.Ua.Di.DataTypes.TransferResultDataDataType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ParameterResultDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId ParameterResultDataType = new ExpandedNodeId(Opc.Ua.Di.DataTypes.ParameterResultDataType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareVersionFileType DataType.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareVersionFileType = new ExpandedNodeId(Opc.Ua.Di.DataTypes.SoftwareVersionFileType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the UpdateBehavior DataType.
        /// </summary>
        public static readonly ExpandedNodeId UpdateBehavior = new ExpandedNodeId(Opc.Ua.Di.DataTypes.UpdateBehavior, Opc.Ua.Di.Namespaces.OpcUaDi);
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
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_Open Method.
        /// </summary>
        public static readonly ExpandedNodeId OPCUADINamespaceMetadata_NamespaceFile_Open = new ExpandedNodeId(Opc.Ua.Di.Methods.OPCUADINamespaceMetadata_NamespaceFile_Open, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_Close Method.
        /// </summary>
        public static readonly ExpandedNodeId OPCUADINamespaceMetadata_NamespaceFile_Close = new ExpandedNodeId(Opc.Ua.Di.Methods.OPCUADINamespaceMetadata_NamespaceFile_Close, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_Read Method.
        /// </summary>
        public static readonly ExpandedNodeId OPCUADINamespaceMetadata_NamespaceFile_Read = new ExpandedNodeId(Opc.Ua.Di.Methods.OPCUADINamespaceMetadata_NamespaceFile_Read, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_Write Method.
        /// </summary>
        public static readonly ExpandedNodeId OPCUADINamespaceMetadata_NamespaceFile_Write = new ExpandedNodeId(Opc.Ua.Di.Methods.OPCUADINamespaceMetadata_NamespaceFile_Write, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_GetPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId OPCUADINamespaceMetadata_NamespaceFile_GetPosition = new ExpandedNodeId(Opc.Ua.Di.Methods.OPCUADINamespaceMetadata_NamespaceFile_GetPosition, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_SetPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId OPCUADINamespaceMetadata_NamespaceFile_SetPosition = new ExpandedNodeId(Opc.Ua.Di.Methods.OPCUADINamespaceMetadata_NamespaceFile_SetPosition, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TopologyElementType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId TopologyElementType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Di.Methods.TopologyElementType_Lock_InitLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TopologyElementType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId TopologyElementType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Di.Methods.TopologyElementType_Lock_RenewLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TopologyElementType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId TopologyElementType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Di.Methods.TopologyElementType_Lock_ExitLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TopologyElementType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId TopologyElementType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Di.Methods.TopologyElementType_Lock_BreakLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ComponentType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ComponentType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Di.Methods.ComponentType_Lock_InitLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ComponentType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ComponentType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Di.Methods.ComponentType_Lock_RenewLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ComponentType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ComponentType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Di.Methods.ComponentType_Lock_ExitLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ComponentType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ComponentType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Di.Methods.ComponentType_Lock_BreakLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Di.Methods.DeviceType_Lock_InitLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Di.Methods.DeviceType_Lock_RenewLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Di.Methods.DeviceType_Lock_ExitLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Di.Methods.DeviceType_Lock_BreakLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_CPIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Di.Methods.DeviceType_CPIdentifier_Lock_InitLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_CPIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Di.Methods.DeviceType_CPIdentifier_Lock_RenewLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_CPIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Di.Methods.DeviceType_CPIdentifier_Lock_ExitLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_CPIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Di.Methods.DeviceType_CPIdentifier_Lock_BreakLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Di.Methods.SoftwareType_Lock_InitLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Di.Methods.SoftwareType_Lock_RenewLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Di.Methods.SoftwareType_Lock_ExitLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Di.Methods.SoftwareType_Lock_BreakLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the BlockType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId BlockType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Di.Methods.BlockType_Lock_InitLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the BlockType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId BlockType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Di.Methods.BlockType_Lock_RenewLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the BlockType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId BlockType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Di.Methods.BlockType_Lock_ExitLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the BlockType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId BlockType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Di.Methods.BlockType_Lock_BreakLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_ShelvingState_TimedShelve Method.
        /// </summary>
        public static readonly ExpandedNodeId DeviceHealthDiagnosticAlarmType_ShelvingState_TimedShelve = new ExpandedNodeId(Opc.Ua.Di.Methods.DeviceHealthDiagnosticAlarmType_ShelvingState_TimedShelve, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_ShelvingState_Unshelve Method.
        /// </summary>
        public static readonly ExpandedNodeId DeviceHealthDiagnosticAlarmType_ShelvingState_Unshelve = new ExpandedNodeId(Opc.Ua.Di.Methods.DeviceHealthDiagnosticAlarmType_ShelvingState_Unshelve, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_ShelvingState_OneShotShelve Method.
        /// </summary>
        public static readonly ExpandedNodeId DeviceHealthDiagnosticAlarmType_ShelvingState_OneShotShelve = new ExpandedNodeId(Opc.Ua.Di.Methods.DeviceHealthDiagnosticAlarmType_ShelvingState_OneShotShelve, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FailureAlarmType_ShelvingState_TimedShelve Method.
        /// </summary>
        public static readonly ExpandedNodeId FailureAlarmType_ShelvingState_TimedShelve = new ExpandedNodeId(Opc.Ua.Di.Methods.FailureAlarmType_ShelvingState_TimedShelve, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FailureAlarmType_ShelvingState_Unshelve Method.
        /// </summary>
        public static readonly ExpandedNodeId FailureAlarmType_ShelvingState_Unshelve = new ExpandedNodeId(Opc.Ua.Di.Methods.FailureAlarmType_ShelvingState_Unshelve, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FailureAlarmType_ShelvingState_OneShotShelve Method.
        /// </summary>
        public static readonly ExpandedNodeId FailureAlarmType_ShelvingState_OneShotShelve = new ExpandedNodeId(Opc.Ua.Di.Methods.FailureAlarmType_ShelvingState_OneShotShelve, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_ShelvingState_TimedShelve Method.
        /// </summary>
        public static readonly ExpandedNodeId CheckFunctionAlarmType_ShelvingState_TimedShelve = new ExpandedNodeId(Opc.Ua.Di.Methods.CheckFunctionAlarmType_ShelvingState_TimedShelve, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_ShelvingState_Unshelve Method.
        /// </summary>
        public static readonly ExpandedNodeId CheckFunctionAlarmType_ShelvingState_Unshelve = new ExpandedNodeId(Opc.Ua.Di.Methods.CheckFunctionAlarmType_ShelvingState_Unshelve, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_ShelvingState_OneShotShelve Method.
        /// </summary>
        public static readonly ExpandedNodeId CheckFunctionAlarmType_ShelvingState_OneShotShelve = new ExpandedNodeId(Opc.Ua.Di.Methods.CheckFunctionAlarmType_ShelvingState_OneShotShelve, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OffSpecAlarmType_ShelvingState_TimedShelve Method.
        /// </summary>
        public static readonly ExpandedNodeId OffSpecAlarmType_ShelvingState_TimedShelve = new ExpandedNodeId(Opc.Ua.Di.Methods.OffSpecAlarmType_ShelvingState_TimedShelve, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OffSpecAlarmType_ShelvingState_Unshelve Method.
        /// </summary>
        public static readonly ExpandedNodeId OffSpecAlarmType_ShelvingState_Unshelve = new ExpandedNodeId(Opc.Ua.Di.Methods.OffSpecAlarmType_ShelvingState_Unshelve, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OffSpecAlarmType_ShelvingState_OneShotShelve Method.
        /// </summary>
        public static readonly ExpandedNodeId OffSpecAlarmType_ShelvingState_OneShotShelve = new ExpandedNodeId(Opc.Ua.Di.Methods.OffSpecAlarmType_ShelvingState_OneShotShelve, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_ShelvingState_TimedShelve Method.
        /// </summary>
        public static readonly ExpandedNodeId MaintenanceRequiredAlarmType_ShelvingState_TimedShelve = new ExpandedNodeId(Opc.Ua.Di.Methods.MaintenanceRequiredAlarmType_ShelvingState_TimedShelve, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_ShelvingState_Unshelve Method.
        /// </summary>
        public static readonly ExpandedNodeId MaintenanceRequiredAlarmType_ShelvingState_Unshelve = new ExpandedNodeId(Opc.Ua.Di.Methods.MaintenanceRequiredAlarmType_ShelvingState_Unshelve, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_ShelvingState_OneShotShelve Method.
        /// </summary>
        public static readonly ExpandedNodeId MaintenanceRequiredAlarmType_ShelvingState_OneShotShelve = new ExpandedNodeId(Opc.Ua.Di.Methods.MaintenanceRequiredAlarmType_ShelvingState_OneShotShelve, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType_CPIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Di.Methods.NetworkType_CPIdentifier_Lock_InitLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType_CPIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Di.Methods.NetworkType_CPIdentifier_Lock_RenewLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType_CPIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Di.Methods.NetworkType_CPIdentifier_Lock_ExitLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType_CPIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Di.Methods.NetworkType_CPIdentifier_Lock_BreakLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Di.Methods.NetworkType_Lock_InitLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Di.Methods.NetworkType_Lock_RenewLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Di.Methods.NetworkType_Lock_ExitLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Di.Methods.NetworkType_Lock_BreakLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectionPointType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPointType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Di.Methods.ConnectionPointType_Lock_InitLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectionPointType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPointType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Di.Methods.ConnectionPointType_Lock_RenewLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectionPointType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPointType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Di.Methods.ConnectionPointType_Lock_ExitLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectionPointType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPointType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Di.Methods.ConnectionPointType_Lock_BreakLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectionPointType_NetworkIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPointType_NetworkIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Di.Methods.ConnectionPointType_NetworkIdentifier_Lock_InitLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectionPointType_NetworkIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPointType_NetworkIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Di.Methods.ConnectionPointType_NetworkIdentifier_Lock_RenewLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectionPointType_NetworkIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPointType_NetworkIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Di.Methods.ConnectionPointType_NetworkIdentifier_Lock_ExitLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectionPointType_NetworkIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPointType_NetworkIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Di.Methods.ConnectionPointType_NetworkIdentifier_Lock_BreakLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TransferServicesType_TransferToDevice Method.
        /// </summary>
        public static readonly ExpandedNodeId TransferServicesType_TransferToDevice = new ExpandedNodeId(Opc.Ua.Di.Methods.TransferServicesType_TransferToDevice, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TransferServicesType_TransferFromDevice Method.
        /// </summary>
        public static readonly ExpandedNodeId TransferServicesType_TransferFromDevice = new ExpandedNodeId(Opc.Ua.Di.Methods.TransferServicesType_TransferFromDevice, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TransferServicesType_FetchTransferResultData Method.
        /// </summary>
        public static readonly ExpandedNodeId TransferServicesType_FetchTransferResultData = new ExpandedNodeId(Opc.Ua.Di.Methods.TransferServicesType_FetchTransferResultData, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the LockingServicesType_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId LockingServicesType_InitLock = new ExpandedNodeId(Opc.Ua.Di.Methods.LockingServicesType_InitLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the LockingServicesType_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId LockingServicesType_RenewLock = new ExpandedNodeId(Opc.Ua.Di.Methods.LockingServicesType_RenewLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the LockingServicesType_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId LockingServicesType_ExitLock = new ExpandedNodeId(Opc.Ua.Di.Methods.LockingServicesType_ExitLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the LockingServicesType_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId LockingServicesType_BreakLock = new ExpandedNodeId(Opc.Ua.Di.Methods.LockingServicesType_BreakLock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_PrepareForUpdate_Prepare Method.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_PrepareForUpdate_Prepare = new ExpandedNodeId(Opc.Ua.Di.Methods.SoftwareUpdateType_PrepareForUpdate_Prepare, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_PrepareForUpdate_Abort Method.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_PrepareForUpdate_Abort = new ExpandedNodeId(Opc.Ua.Di.Methods.SoftwareUpdateType_PrepareForUpdate_Abort, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Installation_Resume Method.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_Installation_Resume = new ExpandedNodeId(Opc.Ua.Di.Methods.SoftwareUpdateType_Installation_Resume, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Confirmation_Confirm Method.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_Confirmation_Confirm = new ExpandedNodeId(Opc.Ua.Di.Methods.SoftwareUpdateType_Confirmation_Confirm, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Parameters_GenerateFileForRead Method.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_Parameters_GenerateFileForRead = new ExpandedNodeId(Opc.Ua.Di.Methods.SoftwareUpdateType_Parameters_GenerateFileForRead, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Parameters_GenerateFileForWrite Method.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_Parameters_GenerateFileForWrite = new ExpandedNodeId(Opc.Ua.Di.Methods.SoftwareUpdateType_Parameters_GenerateFileForWrite, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Parameters_CloseAndCommit Method.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_Parameters_CloseAndCommit = new ExpandedNodeId(Opc.Ua.Di.Methods.SoftwareUpdateType_Parameters_CloseAndCommit, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PackageLoadingType_FileTransfer_GenerateFileForRead Method.
        /// </summary>
        public static readonly ExpandedNodeId PackageLoadingType_FileTransfer_GenerateFileForRead = new ExpandedNodeId(Opc.Ua.Di.Methods.PackageLoadingType_FileTransfer_GenerateFileForRead, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PackageLoadingType_FileTransfer_GenerateFileForWrite Method.
        /// </summary>
        public static readonly ExpandedNodeId PackageLoadingType_FileTransfer_GenerateFileForWrite = new ExpandedNodeId(Opc.Ua.Di.Methods.PackageLoadingType_FileTransfer_GenerateFileForWrite, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PackageLoadingType_FileTransfer_CloseAndCommit Method.
        /// </summary>
        public static readonly ExpandedNodeId PackageLoadingType_FileTransfer_CloseAndCommit = new ExpandedNodeId(Opc.Ua.Di.Methods.PackageLoadingType_FileTransfer_CloseAndCommit, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DirectLoadingType_FileTransfer_GenerateFileForRead Method.
        /// </summary>
        public static readonly ExpandedNodeId DirectLoadingType_FileTransfer_GenerateFileForRead = new ExpandedNodeId(Opc.Ua.Di.Methods.DirectLoadingType_FileTransfer_GenerateFileForRead, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DirectLoadingType_FileTransfer_GenerateFileForWrite Method.
        /// </summary>
        public static readonly ExpandedNodeId DirectLoadingType_FileTransfer_GenerateFileForWrite = new ExpandedNodeId(Opc.Ua.Di.Methods.DirectLoadingType_FileTransfer_GenerateFileForWrite, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DirectLoadingType_FileTransfer_CloseAndCommit Method.
        /// </summary>
        public static readonly ExpandedNodeId DirectLoadingType_FileTransfer_CloseAndCommit = new ExpandedNodeId(Opc.Ua.Di.Methods.DirectLoadingType_FileTransfer_CloseAndCommit, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CachedLoadingType_FileTransfer_GenerateFileForRead Method.
        /// </summary>
        public static readonly ExpandedNodeId CachedLoadingType_FileTransfer_GenerateFileForRead = new ExpandedNodeId(Opc.Ua.Di.Methods.CachedLoadingType_FileTransfer_GenerateFileForRead, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CachedLoadingType_FileTransfer_GenerateFileForWrite Method.
        /// </summary>
        public static readonly ExpandedNodeId CachedLoadingType_FileTransfer_GenerateFileForWrite = new ExpandedNodeId(Opc.Ua.Di.Methods.CachedLoadingType_FileTransfer_GenerateFileForWrite, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CachedLoadingType_FileTransfer_CloseAndCommit Method.
        /// </summary>
        public static readonly ExpandedNodeId CachedLoadingType_FileTransfer_CloseAndCommit = new ExpandedNodeId(Opc.Ua.Di.Methods.CachedLoadingType_FileTransfer_CloseAndCommit, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CachedLoadingType_GetUpdateBehavior Method.
        /// </summary>
        public static readonly ExpandedNodeId CachedLoadingType_GetUpdateBehavior = new ExpandedNodeId(Opc.Ua.Di.Methods.CachedLoadingType_GetUpdateBehavior, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FileSystemLoadingType_FileSystem_CreateDirectory Method.
        /// </summary>
        public static readonly ExpandedNodeId FileSystemLoadingType_FileSystem_CreateDirectory = new ExpandedNodeId(Opc.Ua.Di.Methods.FileSystemLoadingType_FileSystem_CreateDirectory, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FileSystemLoadingType_FileSystem_CreateFile Method.
        /// </summary>
        public static readonly ExpandedNodeId FileSystemLoadingType_FileSystem_CreateFile = new ExpandedNodeId(Opc.Ua.Di.Methods.FileSystemLoadingType_FileSystem_CreateFile, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FileSystemLoadingType_FileSystem_DeleteFileSystemObject Method.
        /// </summary>
        public static readonly ExpandedNodeId FileSystemLoadingType_FileSystem_DeleteFileSystemObject = new ExpandedNodeId(Opc.Ua.Di.Methods.FileSystemLoadingType_FileSystem_DeleteFileSystemObject, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FileSystemLoadingType_FileSystem_MoveOrCopy Method.
        /// </summary>
        public static readonly ExpandedNodeId FileSystemLoadingType_FileSystem_MoveOrCopy = new ExpandedNodeId(Opc.Ua.Di.Methods.FileSystemLoadingType_FileSystem_MoveOrCopy, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FileSystemLoadingType_GetUpdateBehavior Method.
        /// </summary>
        public static readonly ExpandedNodeId FileSystemLoadingType_GetUpdateBehavior = new ExpandedNodeId(Opc.Ua.Di.Methods.FileSystemLoadingType_GetUpdateBehavior, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FileSystemLoadingType_ValidateFiles Method.
        /// </summary>
        public static readonly ExpandedNodeId FileSystemLoadingType_ValidateFiles = new ExpandedNodeId(Opc.Ua.Di.Methods.FileSystemLoadingType_ValidateFiles, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_Prepare Method.
        /// </summary>
        public static readonly ExpandedNodeId PrepareForUpdateStateMachineType_Prepare = new ExpandedNodeId(Opc.Ua.Di.Methods.PrepareForUpdateStateMachineType_Prepare, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_Abort Method.
        /// </summary>
        public static readonly ExpandedNodeId PrepareForUpdateStateMachineType_Abort = new ExpandedNodeId(Opc.Ua.Di.Methods.PrepareForUpdateStateMachineType_Abort, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_Resume Method.
        /// </summary>
        public static readonly ExpandedNodeId PrepareForUpdateStateMachineType_Resume = new ExpandedNodeId(Opc.Ua.Di.Methods.PrepareForUpdateStateMachineType_Resume, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the InstallationStateMachineType_InstallSoftwarePackage Method.
        /// </summary>
        public static readonly ExpandedNodeId InstallationStateMachineType_InstallSoftwarePackage = new ExpandedNodeId(Opc.Ua.Di.Methods.InstallationStateMachineType_InstallSoftwarePackage, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the InstallationStateMachineType_InstallFiles Method.
        /// </summary>
        public static readonly ExpandedNodeId InstallationStateMachineType_InstallFiles = new ExpandedNodeId(Opc.Ua.Di.Methods.InstallationStateMachineType_InstallFiles, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the InstallationStateMachineType_Resume Method.
        /// </summary>
        public static readonly ExpandedNodeId InstallationStateMachineType_Resume = new ExpandedNodeId(Opc.Ua.Di.Methods.InstallationStateMachineType_Resume, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConfirmationStateMachineType_Confirm Method.
        /// </summary>
        public static readonly ExpandedNodeId ConfirmationStateMachineType_Confirm = new ExpandedNodeId(Opc.Ua.Di.Methods.ConfirmationStateMachineType_Confirm, Opc.Ua.Di.Namespaces.OpcUaDi);
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
        /// The identifier for the OPCUADINamespaceMetadata Object.
        /// </summary>
        public static readonly ExpandedNodeId OPCUADINamespaceMetadata = new ExpandedNodeId(Opc.Ua.Di.Objects.OPCUADINamespaceMetadata, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceSet Object.
        /// </summary>
        public static readonly ExpandedNodeId DeviceSet = new ExpandedNodeId(Opc.Ua.Di.Objects.DeviceSet, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceFeatures Object.
        /// </summary>
        public static readonly ExpandedNodeId DeviceFeatures = new ExpandedNodeId(Opc.Ua.Di.Objects.DeviceFeatures, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkSet Object.
        /// </summary>
        public static readonly ExpandedNodeId NetworkSet = new ExpandedNodeId(Opc.Ua.Di.Objects.NetworkSet, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceTopology Object.
        /// </summary>
        public static readonly ExpandedNodeId DeviceTopology = new ExpandedNodeId(Opc.Ua.Di.Objects.DeviceTopology, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TopologyElementType_ParameterSet Object.
        /// </summary>
        public static readonly ExpandedNodeId TopologyElementType_ParameterSet = new ExpandedNodeId(Opc.Ua.Di.Objects.TopologyElementType_ParameterSet, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TopologyElementType_MethodSet Object.
        /// </summary>
        public static readonly ExpandedNodeId TopologyElementType_MethodSet = new ExpandedNodeId(Opc.Ua.Di.Objects.TopologyElementType_MethodSet, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TopologyElementType_GroupIdentifier Object.
        /// </summary>
        public static readonly ExpandedNodeId TopologyElementType_GroupIdentifier = new ExpandedNodeId(Opc.Ua.Di.Objects.TopologyElementType_GroupIdentifier, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TopologyElementType_Identification Object.
        /// </summary>
        public static readonly ExpandedNodeId TopologyElementType_Identification = new ExpandedNodeId(Opc.Ua.Di.Objects.TopologyElementType_Identification, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TopologyElementType_Lock Object.
        /// </summary>
        public static readonly ExpandedNodeId TopologyElementType_Lock = new ExpandedNodeId(Opc.Ua.Di.Objects.TopologyElementType_Lock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the IDeviceHealthType_DeviceHealthAlarms Object.
        /// </summary>
        public static readonly ExpandedNodeId IDeviceHealthType_DeviceHealthAlarms = new ExpandedNodeId(Opc.Ua.Di.Objects.IDeviceHealthType_DeviceHealthAlarms, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ISupportInfoType_DeviceTypeImage Object.
        /// </summary>
        public static readonly ExpandedNodeId ISupportInfoType_DeviceTypeImage = new ExpandedNodeId(Opc.Ua.Di.Objects.ISupportInfoType_DeviceTypeImage, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ISupportInfoType_Documentation Object.
        /// </summary>
        public static readonly ExpandedNodeId ISupportInfoType_Documentation = new ExpandedNodeId(Opc.Ua.Di.Objects.ISupportInfoType_Documentation, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ISupportInfoType_ProtocolSupport Object.
        /// </summary>
        public static readonly ExpandedNodeId ISupportInfoType_ProtocolSupport = new ExpandedNodeId(Opc.Ua.Di.Objects.ISupportInfoType_ProtocolSupport, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ISupportInfoType_ImageSet Object.
        /// </summary>
        public static readonly ExpandedNodeId ISupportInfoType_ImageSet = new ExpandedNodeId(Opc.Ua.Di.Objects.ISupportInfoType_ImageSet, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_CPIdentifier Object.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_CPIdentifier = new ExpandedNodeId(Opc.Ua.Di.Objects.DeviceType_CPIdentifier, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_CPIdentifier_NetworkAddress = new ExpandedNodeId(Opc.Ua.Di.Objects.DeviceType_CPIdentifier_NetworkAddress, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_DeviceHealthAlarms Object.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_DeviceHealthAlarms = new ExpandedNodeId(Opc.Ua.Di.Objects.DeviceType_DeviceHealthAlarms, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_DeviceTypeImage Object.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_DeviceTypeImage = new ExpandedNodeId(Opc.Ua.Di.Objects.DeviceType_DeviceTypeImage, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_Documentation Object.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_Documentation = new ExpandedNodeId(Opc.Ua.Di.Objects.DeviceType_Documentation, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_ProtocolSupport Object.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_ProtocolSupport = new ExpandedNodeId(Opc.Ua.Di.Objects.DeviceType_ProtocolSupport, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_ImageSet Object.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_ImageSet = new ExpandedNodeId(Opc.Ua.Di.Objects.DeviceType_ImageSet, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConfigurableObjectType_SupportedTypes Object.
        /// </summary>
        public static readonly ExpandedNodeId ConfigurableObjectType_SupportedTypes = new ExpandedNodeId(Opc.Ua.Di.Objects.ConfigurableObjectType_SupportedTypes, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConfigurableObjectType_ObjectIdentifier Object.
        /// </summary>
        public static readonly ExpandedNodeId ConfigurableObjectType_ObjectIdentifier = new ExpandedNodeId(Opc.Ua.Di.Objects.ConfigurableObjectType_ObjectIdentifier, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FunctionalGroupType_GroupIdentifier Object.
        /// </summary>
        public static readonly ExpandedNodeId FunctionalGroupType_GroupIdentifier = new ExpandedNodeId(Opc.Ua.Di.Objects.FunctionalGroupType_GroupIdentifier, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType_ProfileIdentifier Object.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType_ProfileIdentifier = new ExpandedNodeId(Opc.Ua.Di.Objects.NetworkType_ProfileIdentifier, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType_CPIdentifier Object.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType_CPIdentifier = new ExpandedNodeId(Opc.Ua.Di.Objects.NetworkType_CPIdentifier, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType_CPIdentifier_NetworkAddress = new ExpandedNodeId(Opc.Ua.Di.Objects.NetworkType_CPIdentifier_NetworkAddress, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType_Lock Object.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType_Lock = new ExpandedNodeId(Opc.Ua.Di.Objects.NetworkType_Lock, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectionPointType_NetworkAddress Object.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPointType_NetworkAddress = new ExpandedNodeId(Opc.Ua.Di.Objects.ConnectionPointType_NetworkAddress, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectionPointType_ProfileIdentifier Object.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPointType_ProfileIdentifier = new ExpandedNodeId(Opc.Ua.Di.Objects.ConnectionPointType_ProfileIdentifier, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectionPointType_NetworkIdentifier Object.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPointType_NetworkIdentifier = new ExpandedNodeId(Opc.Ua.Di.Objects.ConnectionPointType_NetworkIdentifier, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Loading Object.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_Loading = new ExpandedNodeId(Opc.Ua.Di.Objects.SoftwareUpdateType_Loading, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_PrepareForUpdate Object.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_PrepareForUpdate = new ExpandedNodeId(Opc.Ua.Di.Objects.SoftwareUpdateType_PrepareForUpdate, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Installation Object.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_Installation = new ExpandedNodeId(Opc.Ua.Di.Objects.SoftwareUpdateType_Installation, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_PowerCycle Object.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_PowerCycle = new ExpandedNodeId(Opc.Ua.Di.Objects.SoftwareUpdateType_PowerCycle, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Confirmation Object.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_Confirmation = new ExpandedNodeId(Opc.Ua.Di.Objects.SoftwareUpdateType_Confirmation, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Parameters Object.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_Parameters = new ExpandedNodeId(Opc.Ua.Di.Objects.SoftwareUpdateType_Parameters, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PackageLoadingType_CurrentVersion Object.
        /// </summary>
        public static readonly ExpandedNodeId PackageLoadingType_CurrentVersion = new ExpandedNodeId(Opc.Ua.Di.Objects.PackageLoadingType_CurrentVersion, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PackageLoadingType_FileTransfer Object.
        /// </summary>
        public static readonly ExpandedNodeId PackageLoadingType_FileTransfer = new ExpandedNodeId(Opc.Ua.Di.Objects.PackageLoadingType_FileTransfer, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CachedLoadingType_PendingVersion Object.
        /// </summary>
        public static readonly ExpandedNodeId CachedLoadingType_PendingVersion = new ExpandedNodeId(Opc.Ua.Di.Objects.CachedLoadingType_PendingVersion, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CachedLoadingType_FallbackVersion Object.
        /// </summary>
        public static readonly ExpandedNodeId CachedLoadingType_FallbackVersion = new ExpandedNodeId(Opc.Ua.Di.Objects.CachedLoadingType_FallbackVersion, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FileSystemLoadingType_FileSystem Object.
        /// </summary>
        public static readonly ExpandedNodeId FileSystemLoadingType_FileSystem = new ExpandedNodeId(Opc.Ua.Di.Objects.FileSystemLoadingType_FileSystem, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_Idle Object.
        /// </summary>
        public static readonly ExpandedNodeId PrepareForUpdateStateMachineType_Idle = new ExpandedNodeId(Opc.Ua.Di.Objects.PrepareForUpdateStateMachineType_Idle, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_Preparing Object.
        /// </summary>
        public static readonly ExpandedNodeId PrepareForUpdateStateMachineType_Preparing = new ExpandedNodeId(Opc.Ua.Di.Objects.PrepareForUpdateStateMachineType_Preparing, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_PreparedForUpdate Object.
        /// </summary>
        public static readonly ExpandedNodeId PrepareForUpdateStateMachineType_PreparedForUpdate = new ExpandedNodeId(Opc.Ua.Di.Objects.PrepareForUpdateStateMachineType_PreparedForUpdate, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_Resuming Object.
        /// </summary>
        public static readonly ExpandedNodeId PrepareForUpdateStateMachineType_Resuming = new ExpandedNodeId(Opc.Ua.Di.Objects.PrepareForUpdateStateMachineType_Resuming, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_IdleToPreparing Object.
        /// </summary>
        public static readonly ExpandedNodeId PrepareForUpdateStateMachineType_IdleToPreparing = new ExpandedNodeId(Opc.Ua.Di.Objects.PrepareForUpdateStateMachineType_IdleToPreparing, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_PreparingToIdle Object.
        /// </summary>
        public static readonly ExpandedNodeId PrepareForUpdateStateMachineType_PreparingToIdle = new ExpandedNodeId(Opc.Ua.Di.Objects.PrepareForUpdateStateMachineType_PreparingToIdle, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_PreparingToPreparedForUpdate Object.
        /// </summary>
        public static readonly ExpandedNodeId PrepareForUpdateStateMachineType_PreparingToPreparedForUpdate = new ExpandedNodeId(Opc.Ua.Di.Objects.PrepareForUpdateStateMachineType_PreparingToPreparedForUpdate, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_PreparedForUpdateToResuming Object.
        /// </summary>
        public static readonly ExpandedNodeId PrepareForUpdateStateMachineType_PreparedForUpdateToResuming = new ExpandedNodeId(Opc.Ua.Di.Objects.PrepareForUpdateStateMachineType_PreparedForUpdateToResuming, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_ResumingToIdle Object.
        /// </summary>
        public static readonly ExpandedNodeId PrepareForUpdateStateMachineType_ResumingToIdle = new ExpandedNodeId(Opc.Ua.Di.Objects.PrepareForUpdateStateMachineType_ResumingToIdle, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the InstallationStateMachineType_Idle Object.
        /// </summary>
        public static readonly ExpandedNodeId InstallationStateMachineType_Idle = new ExpandedNodeId(Opc.Ua.Di.Objects.InstallationStateMachineType_Idle, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the InstallationStateMachineType_Installing Object.
        /// </summary>
        public static readonly ExpandedNodeId InstallationStateMachineType_Installing = new ExpandedNodeId(Opc.Ua.Di.Objects.InstallationStateMachineType_Installing, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the InstallationStateMachineType_Error Object.
        /// </summary>
        public static readonly ExpandedNodeId InstallationStateMachineType_Error = new ExpandedNodeId(Opc.Ua.Di.Objects.InstallationStateMachineType_Error, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the InstallationStateMachineType_IdleToInstalling Object.
        /// </summary>
        public static readonly ExpandedNodeId InstallationStateMachineType_IdleToInstalling = new ExpandedNodeId(Opc.Ua.Di.Objects.InstallationStateMachineType_IdleToInstalling, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the InstallationStateMachineType_InstallingToIdle Object.
        /// </summary>
        public static readonly ExpandedNodeId InstallationStateMachineType_InstallingToIdle = new ExpandedNodeId(Opc.Ua.Di.Objects.InstallationStateMachineType_InstallingToIdle, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the InstallationStateMachineType_InstallingToError Object.
        /// </summary>
        public static readonly ExpandedNodeId InstallationStateMachineType_InstallingToError = new ExpandedNodeId(Opc.Ua.Di.Objects.InstallationStateMachineType_InstallingToError, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the InstallationStateMachineType_ErrorToIdle Object.
        /// </summary>
        public static readonly ExpandedNodeId InstallationStateMachineType_ErrorToIdle = new ExpandedNodeId(Opc.Ua.Di.Objects.InstallationStateMachineType_ErrorToIdle, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PowerCycleStateMachineType_NotWaitingForPowerCycle Object.
        /// </summary>
        public static readonly ExpandedNodeId PowerCycleStateMachineType_NotWaitingForPowerCycle = new ExpandedNodeId(Opc.Ua.Di.Objects.PowerCycleStateMachineType_NotWaitingForPowerCycle, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PowerCycleStateMachineType_WaitingForPowerCycle Object.
        /// </summary>
        public static readonly ExpandedNodeId PowerCycleStateMachineType_WaitingForPowerCycle = new ExpandedNodeId(Opc.Ua.Di.Objects.PowerCycleStateMachineType_WaitingForPowerCycle, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PowerCycleStateMachineType_NotWaitingForPowerCycleToWaitingForPowerCycle Object.
        /// </summary>
        public static readonly ExpandedNodeId PowerCycleStateMachineType_NotWaitingForPowerCycleToWaitingForPowerCycle = new ExpandedNodeId(Opc.Ua.Di.Objects.PowerCycleStateMachineType_NotWaitingForPowerCycleToWaitingForPowerCycle, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PowerCycleStateMachineType_WaitingForPowerCycleToNotWaitingForPowerCycle Object.
        /// </summary>
        public static readonly ExpandedNodeId PowerCycleStateMachineType_WaitingForPowerCycleToNotWaitingForPowerCycle = new ExpandedNodeId(Opc.Ua.Di.Objects.PowerCycleStateMachineType_WaitingForPowerCycleToNotWaitingForPowerCycle, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConfirmationStateMachineType_NotWaitingForConfirm Object.
        /// </summary>
        public static readonly ExpandedNodeId ConfirmationStateMachineType_NotWaitingForConfirm = new ExpandedNodeId(Opc.Ua.Di.Objects.ConfirmationStateMachineType_NotWaitingForConfirm, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConfirmationStateMachineType_WaitingForConfirm Object.
        /// </summary>
        public static readonly ExpandedNodeId ConfirmationStateMachineType_WaitingForConfirm = new ExpandedNodeId(Opc.Ua.Di.Objects.ConfirmationStateMachineType_WaitingForConfirm, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConfirmationStateMachineType_NotWaitingForConfirmToWaitingForConfirm Object.
        /// </summary>
        public static readonly ExpandedNodeId ConfirmationStateMachineType_NotWaitingForConfirmToWaitingForConfirm = new ExpandedNodeId(Opc.Ua.Di.Objects.ConfirmationStateMachineType_NotWaitingForConfirmToWaitingForConfirm, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConfirmationStateMachineType_WaitingForConfirmToNotWaitingForConfirm Object.
        /// </summary>
        public static readonly ExpandedNodeId ConfirmationStateMachineType_WaitingForConfirmToNotWaitingForConfirm = new ExpandedNodeId(Opc.Ua.Di.Objects.ConfirmationStateMachineType_WaitingForConfirmToNotWaitingForConfirm, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FetchResultDataType_Encoding_DefaultBinary Object.
        /// </summary>
        public static readonly ExpandedNodeId FetchResultDataType_Encoding_DefaultBinary = new ExpandedNodeId(Opc.Ua.Di.Objects.FetchResultDataType_Encoding_DefaultBinary, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TransferResultErrorDataType_Encoding_DefaultBinary Object.
        /// </summary>
        public static readonly ExpandedNodeId TransferResultErrorDataType_Encoding_DefaultBinary = new ExpandedNodeId(Opc.Ua.Di.Objects.TransferResultErrorDataType_Encoding_DefaultBinary, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TransferResultDataDataType_Encoding_DefaultBinary Object.
        /// </summary>
        public static readonly ExpandedNodeId TransferResultDataDataType_Encoding_DefaultBinary = new ExpandedNodeId(Opc.Ua.Di.Objects.TransferResultDataDataType_Encoding_DefaultBinary, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ParameterResultDataType_Encoding_DefaultBinary Object.
        /// </summary>
        public static readonly ExpandedNodeId ParameterResultDataType_Encoding_DefaultBinary = new ExpandedNodeId(Opc.Ua.Di.Objects.ParameterResultDataType_Encoding_DefaultBinary, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FetchResultDataType_Encoding_DefaultXml Object.
        /// </summary>
        public static readonly ExpandedNodeId FetchResultDataType_Encoding_DefaultXml = new ExpandedNodeId(Opc.Ua.Di.Objects.FetchResultDataType_Encoding_DefaultXml, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TransferResultErrorDataType_Encoding_DefaultXml Object.
        /// </summary>
        public static readonly ExpandedNodeId TransferResultErrorDataType_Encoding_DefaultXml = new ExpandedNodeId(Opc.Ua.Di.Objects.TransferResultErrorDataType_Encoding_DefaultXml, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TransferResultDataDataType_Encoding_DefaultXml Object.
        /// </summary>
        public static readonly ExpandedNodeId TransferResultDataDataType_Encoding_DefaultXml = new ExpandedNodeId(Opc.Ua.Di.Objects.TransferResultDataDataType_Encoding_DefaultXml, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ParameterResultDataType_Encoding_DefaultXml Object.
        /// </summary>
        public static readonly ExpandedNodeId ParameterResultDataType_Encoding_DefaultXml = new ExpandedNodeId(Opc.Ua.Di.Objects.ParameterResultDataType_Encoding_DefaultXml, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FetchResultDataType_Encoding_DefaultJson Object.
        /// </summary>
        public static readonly ExpandedNodeId FetchResultDataType_Encoding_DefaultJson = new ExpandedNodeId(Opc.Ua.Di.Objects.FetchResultDataType_Encoding_DefaultJson, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TransferResultErrorDataType_Encoding_DefaultJson Object.
        /// </summary>
        public static readonly ExpandedNodeId TransferResultErrorDataType_Encoding_DefaultJson = new ExpandedNodeId(Opc.Ua.Di.Objects.TransferResultErrorDataType_Encoding_DefaultJson, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TransferResultDataDataType_Encoding_DefaultJson Object.
        /// </summary>
        public static readonly ExpandedNodeId TransferResultDataDataType_Encoding_DefaultJson = new ExpandedNodeId(Opc.Ua.Di.Objects.TransferResultDataDataType_Encoding_DefaultJson, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ParameterResultDataType_Encoding_DefaultJson Object.
        /// </summary>
        public static readonly ExpandedNodeId ParameterResultDataType_Encoding_DefaultJson = new ExpandedNodeId(Opc.Ua.Di.Objects.ParameterResultDataType_Encoding_DefaultJson, Opc.Ua.Di.Namespaces.OpcUaDi);
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
        /// The identifier for the TopologyElementType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId TopologyElementType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.TopologyElementType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the IVendorNameplateType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId IVendorNameplateType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.IVendorNameplateType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ITagNameplateType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ITagNameplateType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.ITagNameplateType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the IDeviceHealthType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId IDeviceHealthType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.IDeviceHealthType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ISupportInfoType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ISupportInfoType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.ISupportInfoType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ComponentType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ComponentType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.ComponentType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.DeviceType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.SoftwareType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the BlockType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId BlockType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.BlockType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId DeviceHealthDiagnosticAlarmType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.DeviceHealthDiagnosticAlarmType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FailureAlarmType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId FailureAlarmType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.FailureAlarmType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId CheckFunctionAlarmType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.CheckFunctionAlarmType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OffSpecAlarmType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId OffSpecAlarmType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.OffSpecAlarmType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId MaintenanceRequiredAlarmType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.MaintenanceRequiredAlarmType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConfigurableObjectType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ConfigurableObjectType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.ConfigurableObjectType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FunctionalGroupType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId FunctionalGroupType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.FunctionalGroupType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ProtocolType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ProtocolType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.ProtocolType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.NetworkType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectionPointType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPointType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.ConnectionPointType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TransferServicesType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId TransferServicesType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.TransferServicesType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the LockingServicesType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId LockingServicesType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.LockingServicesType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.SoftwareUpdateType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareLoadingType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareLoadingType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.SoftwareLoadingType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PackageLoadingType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId PackageLoadingType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.PackageLoadingType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DirectLoadingType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId DirectLoadingType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.DirectLoadingType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CachedLoadingType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId CachedLoadingType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.CachedLoadingType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FileSystemLoadingType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId FileSystemLoadingType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.FileSystemLoadingType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareVersionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareVersionType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.SoftwareVersionType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId PrepareForUpdateStateMachineType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.PrepareForUpdateStateMachineType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the InstallationStateMachineType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId InstallationStateMachineType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.InstallationStateMachineType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PowerCycleStateMachineType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId PowerCycleStateMachineType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.PowerCycleStateMachineType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConfirmationStateMachineType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ConfirmationStateMachineType = new ExpandedNodeId(Opc.Ua.Di.ObjectTypes.ConfirmationStateMachineType, Opc.Ua.Di.Namespaces.OpcUaDi);
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
        /// The identifier for the ConnectsTo ReferenceType.
        /// </summary>
        public static readonly ExpandedNodeId ConnectsTo = new ExpandedNodeId(Opc.Ua.Di.ReferenceTypes.ConnectsTo, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectsToParent ReferenceType.
        /// </summary>
        public static readonly ExpandedNodeId ConnectsToParent = new ExpandedNodeId(Opc.Ua.Di.ReferenceTypes.ConnectsToParent, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the IsOnline ReferenceType.
        /// </summary>
        public static readonly ExpandedNodeId IsOnline = new ExpandedNodeId(Opc.Ua.Di.ReferenceTypes.IsOnline, Opc.Ua.Di.Namespaces.OpcUaDi);
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
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUADINamespaceMetadata_NamespaceUri = new ExpandedNodeId(Opc.Ua.Di.Variables.OPCUADINamespaceMetadata_NamespaceUri, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceVersion Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUADINamespaceMetadata_NamespaceVersion = new ExpandedNodeId(Opc.Ua.Di.Variables.OPCUADINamespaceMetadata_NamespaceVersion, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespacePublicationDate Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUADINamespaceMetadata_NamespacePublicationDate = new ExpandedNodeId(Opc.Ua.Di.Variables.OPCUADINamespaceMetadata_NamespacePublicationDate, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_IsNamespaceSubset Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUADINamespaceMetadata_IsNamespaceSubset = new ExpandedNodeId(Opc.Ua.Di.Variables.OPCUADINamespaceMetadata_IsNamespaceSubset, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_StaticNodeIdTypes Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUADINamespaceMetadata_StaticNodeIdTypes = new ExpandedNodeId(Opc.Ua.Di.Variables.OPCUADINamespaceMetadata_StaticNodeIdTypes, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_StaticNumericNodeIdRange Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUADINamespaceMetadata_StaticNumericNodeIdRange = new ExpandedNodeId(Opc.Ua.Di.Variables.OPCUADINamespaceMetadata_StaticNumericNodeIdRange, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_StaticStringNodeIdPattern Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUADINamespaceMetadata_StaticStringNodeIdPattern = new ExpandedNodeId(Opc.Ua.Di.Variables.OPCUADINamespaceMetadata_StaticStringNodeIdPattern, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_Size Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUADINamespaceMetadata_NamespaceFile_Size = new ExpandedNodeId(Opc.Ua.Di.Variables.OPCUADINamespaceMetadata_NamespaceFile_Size, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_Writable Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUADINamespaceMetadata_NamespaceFile_Writable = new ExpandedNodeId(Opc.Ua.Di.Variables.OPCUADINamespaceMetadata_NamespaceFile_Writable, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_UserWritable Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUADINamespaceMetadata_NamespaceFile_UserWritable = new ExpandedNodeId(Opc.Ua.Di.Variables.OPCUADINamespaceMetadata_NamespaceFile_UserWritable, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_OpenCount Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUADINamespaceMetadata_NamespaceFile_OpenCount = new ExpandedNodeId(Opc.Ua.Di.Variables.OPCUADINamespaceMetadata_NamespaceFile_OpenCount, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_Open_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUADINamespaceMetadata_NamespaceFile_Open_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.OPCUADINamespaceMetadata_NamespaceFile_Open_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_Open_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUADINamespaceMetadata_NamespaceFile_Open_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.OPCUADINamespaceMetadata_NamespaceFile_Open_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_Close_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUADINamespaceMetadata_NamespaceFile_Close_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.OPCUADINamespaceMetadata_NamespaceFile_Close_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_Read_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUADINamespaceMetadata_NamespaceFile_Read_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.OPCUADINamespaceMetadata_NamespaceFile_Read_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_Read_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUADINamespaceMetadata_NamespaceFile_Read_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.OPCUADINamespaceMetadata_NamespaceFile_Read_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_Write_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUADINamespaceMetadata_NamespaceFile_Write_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.OPCUADINamespaceMetadata_NamespaceFile_Write_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_GetPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUADINamespaceMetadata_NamespaceFile_GetPosition_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.OPCUADINamespaceMetadata_NamespaceFile_GetPosition_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_GetPosition_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUADINamespaceMetadata_NamespaceFile_GetPosition_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.OPCUADINamespaceMetadata_NamespaceFile_GetPosition_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_NamespaceFile_SetPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUADINamespaceMetadata_NamespaceFile_SetPosition_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.OPCUADINamespaceMetadata_NamespaceFile_SetPosition_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_DefaultRolePermissions Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUADINamespaceMetadata_DefaultRolePermissions = new ExpandedNodeId(Opc.Ua.Di.Variables.OPCUADINamespaceMetadata_DefaultRolePermissions, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_DefaultUserRolePermissions Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUADINamespaceMetadata_DefaultUserRolePermissions = new ExpandedNodeId(Opc.Ua.Di.Variables.OPCUADINamespaceMetadata_DefaultUserRolePermissions, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OPCUADINamespaceMetadata_DefaultAccessRestrictions Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUADINamespaceMetadata_DefaultAccessRestrictions = new ExpandedNodeId(Opc.Ua.Di.Variables.OPCUADINamespaceMetadata_DefaultAccessRestrictions, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceTopology_OnlineAccess Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceTopology_OnlineAccess = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceTopology_OnlineAccess, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TopologyElementType_ParameterSet_ParameterIdentifier Variable.
        /// </summary>
        public static readonly ExpandedNodeId TopologyElementType_ParameterSet_ParameterIdentifier = new ExpandedNodeId(Opc.Ua.Di.Variables.TopologyElementType_ParameterSet_ParameterIdentifier, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TopologyElementType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId TopologyElementType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Di.Variables.TopologyElementType_Lock_Locked, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TopologyElementType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId TopologyElementType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Di.Variables.TopologyElementType_Lock_LockingClient, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TopologyElementType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId TopologyElementType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Di.Variables.TopologyElementType_Lock_LockingUser, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TopologyElementType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId TopologyElementType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Di.Variables.TopologyElementType_Lock_RemainingLockTime, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TopologyElementType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TopologyElementType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.TopologyElementType_Lock_InitLock_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TopologyElementType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TopologyElementType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.TopologyElementType_Lock_InitLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TopologyElementType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TopologyElementType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.TopologyElementType_Lock_RenewLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TopologyElementType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TopologyElementType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.TopologyElementType_Lock_ExitLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TopologyElementType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TopologyElementType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.TopologyElementType_Lock_BreakLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the IVendorNameplateType_Manufacturer Variable.
        /// </summary>
        public static readonly ExpandedNodeId IVendorNameplateType_Manufacturer = new ExpandedNodeId(Opc.Ua.Di.Variables.IVendorNameplateType_Manufacturer, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the IVendorNameplateType_ManufacturerUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId IVendorNameplateType_ManufacturerUri = new ExpandedNodeId(Opc.Ua.Di.Variables.IVendorNameplateType_ManufacturerUri, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the IVendorNameplateType_Model Variable.
        /// </summary>
        public static readonly ExpandedNodeId IVendorNameplateType_Model = new ExpandedNodeId(Opc.Ua.Di.Variables.IVendorNameplateType_Model, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the IVendorNameplateType_HardwareRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId IVendorNameplateType_HardwareRevision = new ExpandedNodeId(Opc.Ua.Di.Variables.IVendorNameplateType_HardwareRevision, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the IVendorNameplateType_SoftwareRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId IVendorNameplateType_SoftwareRevision = new ExpandedNodeId(Opc.Ua.Di.Variables.IVendorNameplateType_SoftwareRevision, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the IVendorNameplateType_DeviceRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId IVendorNameplateType_DeviceRevision = new ExpandedNodeId(Opc.Ua.Di.Variables.IVendorNameplateType_DeviceRevision, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the IVendorNameplateType_ProductCode Variable.
        /// </summary>
        public static readonly ExpandedNodeId IVendorNameplateType_ProductCode = new ExpandedNodeId(Opc.Ua.Di.Variables.IVendorNameplateType_ProductCode, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the IVendorNameplateType_DeviceManual Variable.
        /// </summary>
        public static readonly ExpandedNodeId IVendorNameplateType_DeviceManual = new ExpandedNodeId(Opc.Ua.Di.Variables.IVendorNameplateType_DeviceManual, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the IVendorNameplateType_DeviceClass Variable.
        /// </summary>
        public static readonly ExpandedNodeId IVendorNameplateType_DeviceClass = new ExpandedNodeId(Opc.Ua.Di.Variables.IVendorNameplateType_DeviceClass, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the IVendorNameplateType_SerialNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId IVendorNameplateType_SerialNumber = new ExpandedNodeId(Opc.Ua.Di.Variables.IVendorNameplateType_SerialNumber, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the IVendorNameplateType_ProductInstanceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId IVendorNameplateType_ProductInstanceUri = new ExpandedNodeId(Opc.Ua.Di.Variables.IVendorNameplateType_ProductInstanceUri, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the IVendorNameplateType_RevisionCounter Variable.
        /// </summary>
        public static readonly ExpandedNodeId IVendorNameplateType_RevisionCounter = new ExpandedNodeId(Opc.Ua.Di.Variables.IVendorNameplateType_RevisionCounter, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ITagNameplateType_AssetId Variable.
        /// </summary>
        public static readonly ExpandedNodeId ITagNameplateType_AssetId = new ExpandedNodeId(Opc.Ua.Di.Variables.ITagNameplateType_AssetId, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ITagNameplateType_ComponentName Variable.
        /// </summary>
        public static readonly ExpandedNodeId ITagNameplateType_ComponentName = new ExpandedNodeId(Opc.Ua.Di.Variables.ITagNameplateType_ComponentName, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the IDeviceHealthType_DeviceHealth Variable.
        /// </summary>
        public static readonly ExpandedNodeId IDeviceHealthType_DeviceHealth = new ExpandedNodeId(Opc.Ua.Di.Variables.IDeviceHealthType_DeviceHealth, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ISupportInfoType_DeviceTypeImage_ImageIdentifier Variable.
        /// </summary>
        public static readonly ExpandedNodeId ISupportInfoType_DeviceTypeImage_ImageIdentifier = new ExpandedNodeId(Opc.Ua.Di.Variables.ISupportInfoType_DeviceTypeImage_ImageIdentifier, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ISupportInfoType_Documentation_DocumentIdentifier Variable.
        /// </summary>
        public static readonly ExpandedNodeId ISupportInfoType_Documentation_DocumentIdentifier = new ExpandedNodeId(Opc.Ua.Di.Variables.ISupportInfoType_Documentation_DocumentIdentifier, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ISupportInfoType_ProtocolSupport_ProtocolSupportIdentifier Variable.
        /// </summary>
        public static readonly ExpandedNodeId ISupportInfoType_ProtocolSupport_ProtocolSupportIdentifier = new ExpandedNodeId(Opc.Ua.Di.Variables.ISupportInfoType_ProtocolSupport_ProtocolSupportIdentifier, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ISupportInfoType_ImageSet_ImageIdentifier Variable.
        /// </summary>
        public static readonly ExpandedNodeId ISupportInfoType_ImageSet_ImageIdentifier = new ExpandedNodeId(Opc.Ua.Di.Variables.ISupportInfoType_ImageSet_ImageIdentifier, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ComponentType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ComponentType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Di.Variables.ComponentType_Lock_Locked, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ComponentType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ComponentType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Di.Variables.ComponentType_Lock_LockingClient, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ComponentType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ComponentType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Di.Variables.ComponentType_Lock_LockingUser, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ComponentType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ComponentType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Di.Variables.ComponentType_Lock_RemainingLockTime, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ComponentType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ComponentType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.ComponentType_Lock_InitLock_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ComponentType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ComponentType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.ComponentType_Lock_InitLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ComponentType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ComponentType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.ComponentType_Lock_RenewLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ComponentType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ComponentType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.ComponentType_Lock_ExitLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ComponentType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ComponentType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.ComponentType_Lock_BreakLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ComponentType_Manufacturer Variable.
        /// </summary>
        public static readonly ExpandedNodeId ComponentType_Manufacturer = new ExpandedNodeId(Opc.Ua.Di.Variables.ComponentType_Manufacturer, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ComponentType_ManufacturerUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId ComponentType_ManufacturerUri = new ExpandedNodeId(Opc.Ua.Di.Variables.ComponentType_ManufacturerUri, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ComponentType_Model Variable.
        /// </summary>
        public static readonly ExpandedNodeId ComponentType_Model = new ExpandedNodeId(Opc.Ua.Di.Variables.ComponentType_Model, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ComponentType_HardwareRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId ComponentType_HardwareRevision = new ExpandedNodeId(Opc.Ua.Di.Variables.ComponentType_HardwareRevision, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ComponentType_SoftwareRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId ComponentType_SoftwareRevision = new ExpandedNodeId(Opc.Ua.Di.Variables.ComponentType_SoftwareRevision, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ComponentType_DeviceRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId ComponentType_DeviceRevision = new ExpandedNodeId(Opc.Ua.Di.Variables.ComponentType_DeviceRevision, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ComponentType_ProductCode Variable.
        /// </summary>
        public static readonly ExpandedNodeId ComponentType_ProductCode = new ExpandedNodeId(Opc.Ua.Di.Variables.ComponentType_ProductCode, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ComponentType_DeviceManual Variable.
        /// </summary>
        public static readonly ExpandedNodeId ComponentType_DeviceManual = new ExpandedNodeId(Opc.Ua.Di.Variables.ComponentType_DeviceManual, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ComponentType_DeviceClass Variable.
        /// </summary>
        public static readonly ExpandedNodeId ComponentType_DeviceClass = new ExpandedNodeId(Opc.Ua.Di.Variables.ComponentType_DeviceClass, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ComponentType_SerialNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId ComponentType_SerialNumber = new ExpandedNodeId(Opc.Ua.Di.Variables.ComponentType_SerialNumber, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ComponentType_ProductInstanceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId ComponentType_ProductInstanceUri = new ExpandedNodeId(Opc.Ua.Di.Variables.ComponentType_ProductInstanceUri, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ComponentType_RevisionCounter Variable.
        /// </summary>
        public static readonly ExpandedNodeId ComponentType_RevisionCounter = new ExpandedNodeId(Opc.Ua.Di.Variables.ComponentType_RevisionCounter, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ComponentType_AssetId Variable.
        /// </summary>
        public static readonly ExpandedNodeId ComponentType_AssetId = new ExpandedNodeId(Opc.Ua.Di.Variables.ComponentType_AssetId, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ComponentType_ComponentName Variable.
        /// </summary>
        public static readonly ExpandedNodeId ComponentType_ComponentName = new ExpandedNodeId(Opc.Ua.Di.Variables.ComponentType_ComponentName, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_Lock_Locked, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_Lock_LockingClient, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_Lock_LockingUser, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_Lock_RemainingLockTime, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_Lock_InitLock_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_Lock_InitLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_Lock_RenewLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_Lock_ExitLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_Lock_BreakLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_Manufacturer Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_Manufacturer = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_Manufacturer, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_ManufacturerUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_ManufacturerUri = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_ManufacturerUri, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_Model Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_Model = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_Model, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_HardwareRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_HardwareRevision = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_HardwareRevision, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_SoftwareRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_SoftwareRevision = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_SoftwareRevision, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_DeviceRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_DeviceRevision = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_DeviceRevision, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_ProductCode Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_ProductCode = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_ProductCode, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_DeviceManual Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_DeviceManual = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_DeviceManual, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_DeviceClass Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_DeviceClass = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_DeviceClass, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_SerialNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_SerialNumber = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_SerialNumber, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_ProductInstanceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_ProductInstanceUri = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_ProductInstanceUri, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_RevisionCounter Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_RevisionCounter = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_RevisionCounter, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_CPIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_CPIdentifier_Lock_Locked, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_CPIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_CPIdentifier_Lock_LockingClient, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_CPIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_CPIdentifier_Lock_LockingUser, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_CPIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_CPIdentifier_Lock_RemainingLockTime, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_CPIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_CPIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_CPIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_CPIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_CPIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_CPIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_CPIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_CPIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_CPIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_CPIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_DeviceHealth Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_DeviceHealth = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_DeviceHealth, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_DeviceTypeImage_ImageIdentifier Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_DeviceTypeImage_ImageIdentifier = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_DeviceTypeImage_ImageIdentifier, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_Documentation_DocumentIdentifier Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_Documentation_DocumentIdentifier = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_Documentation_DocumentIdentifier, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_ProtocolSupport_ProtocolSupportIdentifier Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_ProtocolSupport_ProtocolSupportIdentifier = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_ProtocolSupport_ProtocolSupportIdentifier, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceType_ImageSet_ImageIdentifier Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceType_ImageSet_ImageIdentifier = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceType_ImageSet_ImageIdentifier, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareType_Lock_Locked, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareType_Lock_LockingClient, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareType_Lock_LockingUser, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareType_Lock_RemainingLockTime, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareType_Lock_InitLock_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareType_Lock_InitLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareType_Lock_RenewLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareType_Lock_ExitLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareType_Lock_BreakLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareType_Manufacturer Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareType_Manufacturer = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareType_Manufacturer, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareType_Model Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareType_Model = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareType_Model, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareType_SoftwareRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareType_SoftwareRevision = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareType_SoftwareRevision, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the BlockType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId BlockType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Di.Variables.BlockType_Lock_Locked, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the BlockType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId BlockType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Di.Variables.BlockType_Lock_LockingClient, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the BlockType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId BlockType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Di.Variables.BlockType_Lock_LockingUser, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the BlockType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId BlockType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Di.Variables.BlockType_Lock_RemainingLockTime, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the BlockType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId BlockType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.BlockType_Lock_InitLock_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the BlockType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId BlockType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.BlockType_Lock_InitLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the BlockType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId BlockType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.BlockType_Lock_RenewLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the BlockType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId BlockType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.BlockType_Lock_ExitLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the BlockType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId BlockType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.BlockType_Lock_BreakLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the BlockType_RevisionCounter Variable.
        /// </summary>
        public static readonly ExpandedNodeId BlockType_RevisionCounter = new ExpandedNodeId(Opc.Ua.Di.Variables.BlockType_RevisionCounter, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the BlockType_ActualMode Variable.
        /// </summary>
        public static readonly ExpandedNodeId BlockType_ActualMode = new ExpandedNodeId(Opc.Ua.Di.Variables.BlockType_ActualMode, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the BlockType_PermittedMode Variable.
        /// </summary>
        public static readonly ExpandedNodeId BlockType_PermittedMode = new ExpandedNodeId(Opc.Ua.Di.Variables.BlockType_PermittedMode, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the BlockType_NormalMode Variable.
        /// </summary>
        public static readonly ExpandedNodeId BlockType_NormalMode = new ExpandedNodeId(Opc.Ua.Di.Variables.BlockType_NormalMode, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the BlockType_TargetMode Variable.
        /// </summary>
        public static readonly ExpandedNodeId BlockType_TargetMode = new ExpandedNodeId(Opc.Ua.Di.Variables.BlockType_TargetMode, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceHealthDiagnosticAlarmType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceHealthDiagnosticAlarmType_EnabledState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceHealthDiagnosticAlarmType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceHealthDiagnosticAlarmType_Quality_SourceTimestamp, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceHealthDiagnosticAlarmType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceHealthDiagnosticAlarmType_LastSeverity_SourceTimestamp, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceHealthDiagnosticAlarmType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceHealthDiagnosticAlarmType_Comment_SourceTimestamp, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceHealthDiagnosticAlarmType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceHealthDiagnosticAlarmType_AddComment_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceHealthDiagnosticAlarmType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceHealthDiagnosticAlarmType_ConditionRefresh_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceHealthDiagnosticAlarmType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceHealthDiagnosticAlarmType_ConditionRefresh2_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_AckedState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceHealthDiagnosticAlarmType_AckedState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceHealthDiagnosticAlarmType_AckedState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_ConfirmedState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceHealthDiagnosticAlarmType_ConfirmedState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceHealthDiagnosticAlarmType_ConfirmedState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_Acknowledge_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceHealthDiagnosticAlarmType_Acknowledge_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceHealthDiagnosticAlarmType_Acknowledge_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_Confirm_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceHealthDiagnosticAlarmType_Confirm_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceHealthDiagnosticAlarmType_Confirm_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceHealthDiagnosticAlarmType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceHealthDiagnosticAlarmType_ActiveState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_SuppressedState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceHealthDiagnosticAlarmType_SuppressedState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceHealthDiagnosticAlarmType_SuppressedState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_OutOfServiceState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceHealthDiagnosticAlarmType_OutOfServiceState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceHealthDiagnosticAlarmType_OutOfServiceState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_ShelvingState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceHealthDiagnosticAlarmType_ShelvingState_CurrentState = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceHealthDiagnosticAlarmType_ShelvingState_CurrentState, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_ShelvingState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceHealthDiagnosticAlarmType_ShelvingState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceHealthDiagnosticAlarmType_ShelvingState_CurrentState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_ShelvingState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceHealthDiagnosticAlarmType_ShelvingState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceHealthDiagnosticAlarmType_ShelvingState_LastTransition_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_ShelvingState_UnshelveTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceHealthDiagnosticAlarmType_ShelvingState_UnshelveTime = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceHealthDiagnosticAlarmType_ShelvingState_UnshelveTime, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_ShelvingState_TimedShelve_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceHealthDiagnosticAlarmType_ShelvingState_TimedShelve_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceHealthDiagnosticAlarmType_ShelvingState_TimedShelve_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_SilenceState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceHealthDiagnosticAlarmType_SilenceState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceHealthDiagnosticAlarmType_SilenceState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceHealthDiagnosticAlarmType_LatchedState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceHealthDiagnosticAlarmType_LatchedState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceHealthDiagnosticAlarmType_LatchedState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FailureAlarmType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId FailureAlarmType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.FailureAlarmType_EnabledState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FailureAlarmType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId FailureAlarmType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.Di.Variables.FailureAlarmType_Quality_SourceTimestamp, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FailureAlarmType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId FailureAlarmType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.Di.Variables.FailureAlarmType_LastSeverity_SourceTimestamp, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FailureAlarmType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId FailureAlarmType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.Di.Variables.FailureAlarmType_Comment_SourceTimestamp, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FailureAlarmType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId FailureAlarmType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.FailureAlarmType_AddComment_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FailureAlarmType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId FailureAlarmType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.FailureAlarmType_ConditionRefresh_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FailureAlarmType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId FailureAlarmType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.FailureAlarmType_ConditionRefresh2_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FailureAlarmType_AckedState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId FailureAlarmType_AckedState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.FailureAlarmType_AckedState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FailureAlarmType_ConfirmedState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId FailureAlarmType_ConfirmedState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.FailureAlarmType_ConfirmedState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FailureAlarmType_Acknowledge_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId FailureAlarmType_Acknowledge_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.FailureAlarmType_Acknowledge_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FailureAlarmType_Confirm_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId FailureAlarmType_Confirm_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.FailureAlarmType_Confirm_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FailureAlarmType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId FailureAlarmType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.FailureAlarmType_ActiveState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FailureAlarmType_SuppressedState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId FailureAlarmType_SuppressedState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.FailureAlarmType_SuppressedState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FailureAlarmType_OutOfServiceState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId FailureAlarmType_OutOfServiceState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.FailureAlarmType_OutOfServiceState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FailureAlarmType_ShelvingState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId FailureAlarmType_ShelvingState_CurrentState = new ExpandedNodeId(Opc.Ua.Di.Variables.FailureAlarmType_ShelvingState_CurrentState, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FailureAlarmType_ShelvingState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId FailureAlarmType_ShelvingState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.FailureAlarmType_ShelvingState_CurrentState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FailureAlarmType_ShelvingState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId FailureAlarmType_ShelvingState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.FailureAlarmType_ShelvingState_LastTransition_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FailureAlarmType_ShelvingState_UnshelveTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId FailureAlarmType_ShelvingState_UnshelveTime = new ExpandedNodeId(Opc.Ua.Di.Variables.FailureAlarmType_ShelvingState_UnshelveTime, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FailureAlarmType_ShelvingState_TimedShelve_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId FailureAlarmType_ShelvingState_TimedShelve_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.FailureAlarmType_ShelvingState_TimedShelve_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FailureAlarmType_SilenceState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId FailureAlarmType_SilenceState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.FailureAlarmType_SilenceState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FailureAlarmType_LatchedState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId FailureAlarmType_LatchedState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.FailureAlarmType_LatchedState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId CheckFunctionAlarmType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.CheckFunctionAlarmType_EnabledState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId CheckFunctionAlarmType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.Di.Variables.CheckFunctionAlarmType_Quality_SourceTimestamp, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId CheckFunctionAlarmType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.Di.Variables.CheckFunctionAlarmType_LastSeverity_SourceTimestamp, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId CheckFunctionAlarmType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.Di.Variables.CheckFunctionAlarmType_Comment_SourceTimestamp, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CheckFunctionAlarmType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.CheckFunctionAlarmType_AddComment_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CheckFunctionAlarmType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.CheckFunctionAlarmType_ConditionRefresh_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CheckFunctionAlarmType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.CheckFunctionAlarmType_ConditionRefresh2_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_AckedState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId CheckFunctionAlarmType_AckedState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.CheckFunctionAlarmType_AckedState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_ConfirmedState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId CheckFunctionAlarmType_ConfirmedState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.CheckFunctionAlarmType_ConfirmedState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_Acknowledge_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CheckFunctionAlarmType_Acknowledge_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.CheckFunctionAlarmType_Acknowledge_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_Confirm_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CheckFunctionAlarmType_Confirm_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.CheckFunctionAlarmType_Confirm_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId CheckFunctionAlarmType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.CheckFunctionAlarmType_ActiveState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_SuppressedState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId CheckFunctionAlarmType_SuppressedState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.CheckFunctionAlarmType_SuppressedState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_OutOfServiceState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId CheckFunctionAlarmType_OutOfServiceState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.CheckFunctionAlarmType_OutOfServiceState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_ShelvingState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId CheckFunctionAlarmType_ShelvingState_CurrentState = new ExpandedNodeId(Opc.Ua.Di.Variables.CheckFunctionAlarmType_ShelvingState_CurrentState, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_ShelvingState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId CheckFunctionAlarmType_ShelvingState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.CheckFunctionAlarmType_ShelvingState_CurrentState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_ShelvingState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId CheckFunctionAlarmType_ShelvingState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.CheckFunctionAlarmType_ShelvingState_LastTransition_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_ShelvingState_UnshelveTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId CheckFunctionAlarmType_ShelvingState_UnshelveTime = new ExpandedNodeId(Opc.Ua.Di.Variables.CheckFunctionAlarmType_ShelvingState_UnshelveTime, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_ShelvingState_TimedShelve_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CheckFunctionAlarmType_ShelvingState_TimedShelve_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.CheckFunctionAlarmType_ShelvingState_TimedShelve_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_SilenceState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId CheckFunctionAlarmType_SilenceState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.CheckFunctionAlarmType_SilenceState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CheckFunctionAlarmType_LatchedState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId CheckFunctionAlarmType_LatchedState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.CheckFunctionAlarmType_LatchedState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OffSpecAlarmType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId OffSpecAlarmType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.OffSpecAlarmType_EnabledState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OffSpecAlarmType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId OffSpecAlarmType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.Di.Variables.OffSpecAlarmType_Quality_SourceTimestamp, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OffSpecAlarmType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId OffSpecAlarmType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.Di.Variables.OffSpecAlarmType_LastSeverity_SourceTimestamp, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OffSpecAlarmType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId OffSpecAlarmType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.Di.Variables.OffSpecAlarmType_Comment_SourceTimestamp, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OffSpecAlarmType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OffSpecAlarmType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.OffSpecAlarmType_AddComment_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OffSpecAlarmType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OffSpecAlarmType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.OffSpecAlarmType_ConditionRefresh_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OffSpecAlarmType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OffSpecAlarmType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.OffSpecAlarmType_ConditionRefresh2_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OffSpecAlarmType_AckedState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId OffSpecAlarmType_AckedState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.OffSpecAlarmType_AckedState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OffSpecAlarmType_ConfirmedState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId OffSpecAlarmType_ConfirmedState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.OffSpecAlarmType_ConfirmedState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OffSpecAlarmType_Acknowledge_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OffSpecAlarmType_Acknowledge_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.OffSpecAlarmType_Acknowledge_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OffSpecAlarmType_Confirm_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OffSpecAlarmType_Confirm_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.OffSpecAlarmType_Confirm_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OffSpecAlarmType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId OffSpecAlarmType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.OffSpecAlarmType_ActiveState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OffSpecAlarmType_SuppressedState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId OffSpecAlarmType_SuppressedState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.OffSpecAlarmType_SuppressedState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OffSpecAlarmType_OutOfServiceState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId OffSpecAlarmType_OutOfServiceState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.OffSpecAlarmType_OutOfServiceState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OffSpecAlarmType_ShelvingState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId OffSpecAlarmType_ShelvingState_CurrentState = new ExpandedNodeId(Opc.Ua.Di.Variables.OffSpecAlarmType_ShelvingState_CurrentState, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OffSpecAlarmType_ShelvingState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId OffSpecAlarmType_ShelvingState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.OffSpecAlarmType_ShelvingState_CurrentState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OffSpecAlarmType_ShelvingState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId OffSpecAlarmType_ShelvingState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.OffSpecAlarmType_ShelvingState_LastTransition_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OffSpecAlarmType_ShelvingState_UnshelveTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId OffSpecAlarmType_ShelvingState_UnshelveTime = new ExpandedNodeId(Opc.Ua.Di.Variables.OffSpecAlarmType_ShelvingState_UnshelveTime, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OffSpecAlarmType_ShelvingState_TimedShelve_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OffSpecAlarmType_ShelvingState_TimedShelve_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.OffSpecAlarmType_ShelvingState_TimedShelve_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OffSpecAlarmType_SilenceState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId OffSpecAlarmType_SilenceState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.OffSpecAlarmType_SilenceState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OffSpecAlarmType_LatchedState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId OffSpecAlarmType_LatchedState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.OffSpecAlarmType_LatchedState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_EnabledState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId MaintenanceRequiredAlarmType_EnabledState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.MaintenanceRequiredAlarmType_EnabledState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_Quality_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId MaintenanceRequiredAlarmType_Quality_SourceTimestamp = new ExpandedNodeId(Opc.Ua.Di.Variables.MaintenanceRequiredAlarmType_Quality_SourceTimestamp, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_LastSeverity_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId MaintenanceRequiredAlarmType_LastSeverity_SourceTimestamp = new ExpandedNodeId(Opc.Ua.Di.Variables.MaintenanceRequiredAlarmType_LastSeverity_SourceTimestamp, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_Comment_SourceTimestamp Variable.
        /// </summary>
        public static readonly ExpandedNodeId MaintenanceRequiredAlarmType_Comment_SourceTimestamp = new ExpandedNodeId(Opc.Ua.Di.Variables.MaintenanceRequiredAlarmType_Comment_SourceTimestamp, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_AddComment_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MaintenanceRequiredAlarmType_AddComment_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.MaintenanceRequiredAlarmType_AddComment_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_ConditionRefresh_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MaintenanceRequiredAlarmType_ConditionRefresh_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.MaintenanceRequiredAlarmType_ConditionRefresh_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_ConditionRefresh2_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MaintenanceRequiredAlarmType_ConditionRefresh2_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.MaintenanceRequiredAlarmType_ConditionRefresh2_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_AckedState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId MaintenanceRequiredAlarmType_AckedState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.MaintenanceRequiredAlarmType_AckedState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_ConfirmedState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId MaintenanceRequiredAlarmType_ConfirmedState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.MaintenanceRequiredAlarmType_ConfirmedState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_Acknowledge_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MaintenanceRequiredAlarmType_Acknowledge_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.MaintenanceRequiredAlarmType_Acknowledge_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_Confirm_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MaintenanceRequiredAlarmType_Confirm_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.MaintenanceRequiredAlarmType_Confirm_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_ActiveState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId MaintenanceRequiredAlarmType_ActiveState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.MaintenanceRequiredAlarmType_ActiveState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_SuppressedState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId MaintenanceRequiredAlarmType_SuppressedState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.MaintenanceRequiredAlarmType_SuppressedState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_OutOfServiceState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId MaintenanceRequiredAlarmType_OutOfServiceState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.MaintenanceRequiredAlarmType_OutOfServiceState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_ShelvingState_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId MaintenanceRequiredAlarmType_ShelvingState_CurrentState = new ExpandedNodeId(Opc.Ua.Di.Variables.MaintenanceRequiredAlarmType_ShelvingState_CurrentState, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_ShelvingState_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId MaintenanceRequiredAlarmType_ShelvingState_CurrentState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.MaintenanceRequiredAlarmType_ShelvingState_CurrentState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_ShelvingState_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId MaintenanceRequiredAlarmType_ShelvingState_LastTransition_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.MaintenanceRequiredAlarmType_ShelvingState_LastTransition_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_ShelvingState_UnshelveTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId MaintenanceRequiredAlarmType_ShelvingState_UnshelveTime = new ExpandedNodeId(Opc.Ua.Di.Variables.MaintenanceRequiredAlarmType_ShelvingState_UnshelveTime, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_ShelvingState_TimedShelve_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MaintenanceRequiredAlarmType_ShelvingState_TimedShelve_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.MaintenanceRequiredAlarmType_ShelvingState_TimedShelve_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_SilenceState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId MaintenanceRequiredAlarmType_SilenceState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.MaintenanceRequiredAlarmType_SilenceState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the MaintenanceRequiredAlarmType_LatchedState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId MaintenanceRequiredAlarmType_LatchedState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.MaintenanceRequiredAlarmType_LatchedState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FunctionalGroupType_GroupIdentifier_UIElement Variable.
        /// </summary>
        public static readonly ExpandedNodeId FunctionalGroupType_GroupIdentifier_UIElement = new ExpandedNodeId(Opc.Ua.Di.Variables.FunctionalGroupType_GroupIdentifier_UIElement, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FunctionalGroupType_UIElement Variable.
        /// </summary>
        public static readonly ExpandedNodeId FunctionalGroupType_UIElement = new ExpandedNodeId(Opc.Ua.Di.Variables.FunctionalGroupType_UIElement, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DeviceHealthEnumeration_EnumStrings Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceHealthEnumeration_EnumStrings = new ExpandedNodeId(Opc.Ua.Di.Variables.DeviceHealthEnumeration_EnumStrings, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType_CPIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Di.Variables.NetworkType_CPIdentifier_Lock_Locked, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType_CPIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Di.Variables.NetworkType_CPIdentifier_Lock_LockingClient, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType_CPIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Di.Variables.NetworkType_CPIdentifier_Lock_LockingUser, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType_CPIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Di.Variables.NetworkType_CPIdentifier_Lock_RemainingLockTime, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType_CPIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.NetworkType_CPIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType_CPIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.NetworkType_CPIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType_CPIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.NetworkType_CPIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType_CPIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.NetworkType_CPIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType_CPIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.NetworkType_CPIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Di.Variables.NetworkType_Lock_Locked, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Di.Variables.NetworkType_Lock_LockingClient, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Di.Variables.NetworkType_Lock_LockingUser, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Di.Variables.NetworkType_Lock_RemainingLockTime, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.NetworkType_Lock_InitLock_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.NetworkType_Lock_InitLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.NetworkType_Lock_RenewLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.NetworkType_Lock_ExitLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the NetworkType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId NetworkType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.NetworkType_Lock_BreakLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectionPointType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPointType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Di.Variables.ConnectionPointType_Lock_Locked, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectionPointType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPointType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Di.Variables.ConnectionPointType_Lock_LockingClient, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectionPointType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPointType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Di.Variables.ConnectionPointType_Lock_LockingUser, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectionPointType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPointType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Di.Variables.ConnectionPointType_Lock_RemainingLockTime, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectionPointType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPointType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.ConnectionPointType_Lock_InitLock_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectionPointType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPointType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.ConnectionPointType_Lock_InitLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectionPointType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPointType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.ConnectionPointType_Lock_RenewLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectionPointType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPointType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.ConnectionPointType_Lock_ExitLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectionPointType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPointType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.ConnectionPointType_Lock_BreakLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectionPointType_NetworkIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPointType_NetworkIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Di.Variables.ConnectionPointType_NetworkIdentifier_Lock_Locked, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectionPointType_NetworkIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPointType_NetworkIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Di.Variables.ConnectionPointType_NetworkIdentifier_Lock_LockingClient, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectionPointType_NetworkIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPointType_NetworkIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Di.Variables.ConnectionPointType_NetworkIdentifier_Lock_LockingUser, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectionPointType_NetworkIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPointType_NetworkIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Di.Variables.ConnectionPointType_NetworkIdentifier_Lock_RemainingLockTime, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectionPointType_NetworkIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPointType_NetworkIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.ConnectionPointType_NetworkIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectionPointType_NetworkIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPointType_NetworkIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.ConnectionPointType_NetworkIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectionPointType_NetworkIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPointType_NetworkIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.ConnectionPointType_NetworkIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectionPointType_NetworkIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPointType_NetworkIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.ConnectionPointType_NetworkIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConnectionPointType_NetworkIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPointType_NetworkIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.ConnectionPointType_NetworkIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TransferServicesType_TransferToDevice_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TransferServicesType_TransferToDevice_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.TransferServicesType_TransferToDevice_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TransferServicesType_TransferFromDevice_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TransferServicesType_TransferFromDevice_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.TransferServicesType_TransferFromDevice_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TransferServicesType_FetchTransferResultData_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TransferServicesType_FetchTransferResultData_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.TransferServicesType_FetchTransferResultData_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the TransferServicesType_FetchTransferResultData_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TransferServicesType_FetchTransferResultData_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.TransferServicesType_FetchTransferResultData_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the MaxInactiveLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId MaxInactiveLockTime = new ExpandedNodeId(Opc.Ua.Di.Variables.MaxInactiveLockTime, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the LockingServicesType_DefaultInstanceBrowseName Variable.
        /// </summary>
        public static readonly ExpandedNodeId LockingServicesType_DefaultInstanceBrowseName = new ExpandedNodeId(Opc.Ua.Di.Variables.LockingServicesType_DefaultInstanceBrowseName, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the LockingServicesType_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId LockingServicesType_Locked = new ExpandedNodeId(Opc.Ua.Di.Variables.LockingServicesType_Locked, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the LockingServicesType_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId LockingServicesType_LockingClient = new ExpandedNodeId(Opc.Ua.Di.Variables.LockingServicesType_LockingClient, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the LockingServicesType_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId LockingServicesType_LockingUser = new ExpandedNodeId(Opc.Ua.Di.Variables.LockingServicesType_LockingUser, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the LockingServicesType_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId LockingServicesType_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Di.Variables.LockingServicesType_RemainingLockTime, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the LockingServicesType_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId LockingServicesType_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.LockingServicesType_InitLock_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the LockingServicesType_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId LockingServicesType_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.LockingServicesType_InitLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the LockingServicesType_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId LockingServicesType_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.LockingServicesType_RenewLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the LockingServicesType_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId LockingServicesType_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.LockingServicesType_ExitLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the LockingServicesType_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId LockingServicesType_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.LockingServicesType_BreakLock_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_PrepareForUpdate_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_PrepareForUpdate_CurrentState = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareUpdateType_PrepareForUpdate_CurrentState, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_PrepareForUpdate_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_PrepareForUpdate_CurrentState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareUpdateType_PrepareForUpdate_CurrentState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_PrepareForUpdate_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_PrepareForUpdate_LastTransition_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareUpdateType_PrepareForUpdate_LastTransition_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Installation_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_Installation_CurrentState = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareUpdateType_Installation_CurrentState, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Installation_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_Installation_CurrentState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareUpdateType_Installation_CurrentState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Installation_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_Installation_LastTransition_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareUpdateType_Installation_LastTransition_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Installation_InstallSoftwarePackage_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_Installation_InstallSoftwarePackage_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareUpdateType_Installation_InstallSoftwarePackage_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Installation_InstallFiles_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_Installation_InstallFiles_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareUpdateType_Installation_InstallFiles_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_PowerCycle_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_PowerCycle_CurrentState = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareUpdateType_PowerCycle_CurrentState, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_PowerCycle_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_PowerCycle_CurrentState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareUpdateType_PowerCycle_CurrentState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_PowerCycle_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_PowerCycle_LastTransition_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareUpdateType_PowerCycle_LastTransition_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Confirmation_CurrentState Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_Confirmation_CurrentState = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareUpdateType_Confirmation_CurrentState, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Confirmation_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_Confirmation_CurrentState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareUpdateType_Confirmation_CurrentState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Confirmation_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_Confirmation_LastTransition_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareUpdateType_Confirmation_LastTransition_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Confirmation_ConfirmationTimeout Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_Confirmation_ConfirmationTimeout = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareUpdateType_Confirmation_ConfirmationTimeout, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Parameters_ClientProcessingTimeout Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_Parameters_ClientProcessingTimeout = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareUpdateType_Parameters_ClientProcessingTimeout, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Parameters_GenerateFileForRead_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_Parameters_GenerateFileForRead_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareUpdateType_Parameters_GenerateFileForRead_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Parameters_GenerateFileForRead_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_Parameters_GenerateFileForRead_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareUpdateType_Parameters_GenerateFileForRead_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Parameters_GenerateFileForWrite_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_Parameters_GenerateFileForWrite_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareUpdateType_Parameters_GenerateFileForWrite_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Parameters_GenerateFileForWrite_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_Parameters_GenerateFileForWrite_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareUpdateType_Parameters_GenerateFileForWrite_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Parameters_CloseAndCommit_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_Parameters_CloseAndCommit_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareUpdateType_Parameters_CloseAndCommit_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_Parameters_CloseAndCommit_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_Parameters_CloseAndCommit_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareUpdateType_Parameters_CloseAndCommit_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_UpdateStatus Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_UpdateStatus = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareUpdateType_UpdateStatus, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_VendorErrorCode Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_VendorErrorCode = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareUpdateType_VendorErrorCode, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareUpdateType_DefaultInstanceBrowseName Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareUpdateType_DefaultInstanceBrowseName = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareUpdateType_DefaultInstanceBrowseName, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareLoadingType_UpdateKey Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareLoadingType_UpdateKey = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareLoadingType_UpdateKey, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PackageLoadingType_CurrentVersion_Manufacturer Variable.
        /// </summary>
        public static readonly ExpandedNodeId PackageLoadingType_CurrentVersion_Manufacturer = new ExpandedNodeId(Opc.Ua.Di.Variables.PackageLoadingType_CurrentVersion_Manufacturer, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PackageLoadingType_CurrentVersion_ManufacturerUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId PackageLoadingType_CurrentVersion_ManufacturerUri = new ExpandedNodeId(Opc.Ua.Di.Variables.PackageLoadingType_CurrentVersion_ManufacturerUri, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PackageLoadingType_CurrentVersion_SoftwareRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId PackageLoadingType_CurrentVersion_SoftwareRevision = new ExpandedNodeId(Opc.Ua.Di.Variables.PackageLoadingType_CurrentVersion_SoftwareRevision, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PackageLoadingType_FileTransfer_ClientProcessingTimeout Variable.
        /// </summary>
        public static readonly ExpandedNodeId PackageLoadingType_FileTransfer_ClientProcessingTimeout = new ExpandedNodeId(Opc.Ua.Di.Variables.PackageLoadingType_FileTransfer_ClientProcessingTimeout, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PackageLoadingType_FileTransfer_GenerateFileForRead_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PackageLoadingType_FileTransfer_GenerateFileForRead_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.PackageLoadingType_FileTransfer_GenerateFileForRead_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PackageLoadingType_FileTransfer_GenerateFileForRead_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PackageLoadingType_FileTransfer_GenerateFileForRead_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.PackageLoadingType_FileTransfer_GenerateFileForRead_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PackageLoadingType_FileTransfer_GenerateFileForWrite_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PackageLoadingType_FileTransfer_GenerateFileForWrite_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.PackageLoadingType_FileTransfer_GenerateFileForWrite_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PackageLoadingType_FileTransfer_GenerateFileForWrite_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PackageLoadingType_FileTransfer_GenerateFileForWrite_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.PackageLoadingType_FileTransfer_GenerateFileForWrite_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PackageLoadingType_FileTransfer_CloseAndCommit_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PackageLoadingType_FileTransfer_CloseAndCommit_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.PackageLoadingType_FileTransfer_CloseAndCommit_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PackageLoadingType_FileTransfer_CloseAndCommit_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId PackageLoadingType_FileTransfer_CloseAndCommit_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.PackageLoadingType_FileTransfer_CloseAndCommit_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PackageLoadingType_ErrorMessage Variable.
        /// </summary>
        public static readonly ExpandedNodeId PackageLoadingType_ErrorMessage = new ExpandedNodeId(Opc.Ua.Di.Variables.PackageLoadingType_ErrorMessage, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PackageLoadingType_WriteBlockSize Variable.
        /// </summary>
        public static readonly ExpandedNodeId PackageLoadingType_WriteBlockSize = new ExpandedNodeId(Opc.Ua.Di.Variables.PackageLoadingType_WriteBlockSize, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DirectLoadingType_CurrentVersion_Manufacturer Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectLoadingType_CurrentVersion_Manufacturer = new ExpandedNodeId(Opc.Ua.Di.Variables.DirectLoadingType_CurrentVersion_Manufacturer, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DirectLoadingType_CurrentVersion_ManufacturerUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectLoadingType_CurrentVersion_ManufacturerUri = new ExpandedNodeId(Opc.Ua.Di.Variables.DirectLoadingType_CurrentVersion_ManufacturerUri, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DirectLoadingType_CurrentVersion_SoftwareRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectLoadingType_CurrentVersion_SoftwareRevision = new ExpandedNodeId(Opc.Ua.Di.Variables.DirectLoadingType_CurrentVersion_SoftwareRevision, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DirectLoadingType_FileTransfer_ClientProcessingTimeout Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectLoadingType_FileTransfer_ClientProcessingTimeout = new ExpandedNodeId(Opc.Ua.Di.Variables.DirectLoadingType_FileTransfer_ClientProcessingTimeout, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DirectLoadingType_FileTransfer_GenerateFileForRead_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectLoadingType_FileTransfer_GenerateFileForRead_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.DirectLoadingType_FileTransfer_GenerateFileForRead_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DirectLoadingType_FileTransfer_GenerateFileForRead_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectLoadingType_FileTransfer_GenerateFileForRead_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.DirectLoadingType_FileTransfer_GenerateFileForRead_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DirectLoadingType_FileTransfer_GenerateFileForWrite_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectLoadingType_FileTransfer_GenerateFileForWrite_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.DirectLoadingType_FileTransfer_GenerateFileForWrite_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DirectLoadingType_FileTransfer_GenerateFileForWrite_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectLoadingType_FileTransfer_GenerateFileForWrite_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.DirectLoadingType_FileTransfer_GenerateFileForWrite_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DirectLoadingType_FileTransfer_CloseAndCommit_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectLoadingType_FileTransfer_CloseAndCommit_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.DirectLoadingType_FileTransfer_CloseAndCommit_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DirectLoadingType_FileTransfer_CloseAndCommit_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectLoadingType_FileTransfer_CloseAndCommit_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.DirectLoadingType_FileTransfer_CloseAndCommit_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DirectLoadingType_UpdateBehavior Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectLoadingType_UpdateBehavior = new ExpandedNodeId(Opc.Ua.Di.Variables.DirectLoadingType_UpdateBehavior, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the DirectLoadingType_WriteTimeout Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectLoadingType_WriteTimeout = new ExpandedNodeId(Opc.Ua.Di.Variables.DirectLoadingType_WriteTimeout, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CachedLoadingType_CurrentVersion_Manufacturer Variable.
        /// </summary>
        public static readonly ExpandedNodeId CachedLoadingType_CurrentVersion_Manufacturer = new ExpandedNodeId(Opc.Ua.Di.Variables.CachedLoadingType_CurrentVersion_Manufacturer, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CachedLoadingType_CurrentVersion_ManufacturerUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId CachedLoadingType_CurrentVersion_ManufacturerUri = new ExpandedNodeId(Opc.Ua.Di.Variables.CachedLoadingType_CurrentVersion_ManufacturerUri, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CachedLoadingType_CurrentVersion_SoftwareRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId CachedLoadingType_CurrentVersion_SoftwareRevision = new ExpandedNodeId(Opc.Ua.Di.Variables.CachedLoadingType_CurrentVersion_SoftwareRevision, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CachedLoadingType_FileTransfer_ClientProcessingTimeout Variable.
        /// </summary>
        public static readonly ExpandedNodeId CachedLoadingType_FileTransfer_ClientProcessingTimeout = new ExpandedNodeId(Opc.Ua.Di.Variables.CachedLoadingType_FileTransfer_ClientProcessingTimeout, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CachedLoadingType_FileTransfer_GenerateFileForRead_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CachedLoadingType_FileTransfer_GenerateFileForRead_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.CachedLoadingType_FileTransfer_GenerateFileForRead_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CachedLoadingType_FileTransfer_GenerateFileForRead_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CachedLoadingType_FileTransfer_GenerateFileForRead_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.CachedLoadingType_FileTransfer_GenerateFileForRead_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CachedLoadingType_FileTransfer_GenerateFileForWrite_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CachedLoadingType_FileTransfer_GenerateFileForWrite_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.CachedLoadingType_FileTransfer_GenerateFileForWrite_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CachedLoadingType_FileTransfer_GenerateFileForWrite_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CachedLoadingType_FileTransfer_GenerateFileForWrite_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.CachedLoadingType_FileTransfer_GenerateFileForWrite_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CachedLoadingType_FileTransfer_CloseAndCommit_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CachedLoadingType_FileTransfer_CloseAndCommit_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.CachedLoadingType_FileTransfer_CloseAndCommit_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CachedLoadingType_FileTransfer_CloseAndCommit_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CachedLoadingType_FileTransfer_CloseAndCommit_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.CachedLoadingType_FileTransfer_CloseAndCommit_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CachedLoadingType_PendingVersion_Manufacturer Variable.
        /// </summary>
        public static readonly ExpandedNodeId CachedLoadingType_PendingVersion_Manufacturer = new ExpandedNodeId(Opc.Ua.Di.Variables.CachedLoadingType_PendingVersion_Manufacturer, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CachedLoadingType_PendingVersion_ManufacturerUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId CachedLoadingType_PendingVersion_ManufacturerUri = new ExpandedNodeId(Opc.Ua.Di.Variables.CachedLoadingType_PendingVersion_ManufacturerUri, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CachedLoadingType_PendingVersion_SoftwareRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId CachedLoadingType_PendingVersion_SoftwareRevision = new ExpandedNodeId(Opc.Ua.Di.Variables.CachedLoadingType_PendingVersion_SoftwareRevision, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CachedLoadingType_FallbackVersion_Manufacturer Variable.
        /// </summary>
        public static readonly ExpandedNodeId CachedLoadingType_FallbackVersion_Manufacturer = new ExpandedNodeId(Opc.Ua.Di.Variables.CachedLoadingType_FallbackVersion_Manufacturer, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CachedLoadingType_FallbackVersion_ManufacturerUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId CachedLoadingType_FallbackVersion_ManufacturerUri = new ExpandedNodeId(Opc.Ua.Di.Variables.CachedLoadingType_FallbackVersion_ManufacturerUri, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CachedLoadingType_FallbackVersion_SoftwareRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId CachedLoadingType_FallbackVersion_SoftwareRevision = new ExpandedNodeId(Opc.Ua.Di.Variables.CachedLoadingType_FallbackVersion_SoftwareRevision, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CachedLoadingType_GetUpdateBehavior_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CachedLoadingType_GetUpdateBehavior_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.CachedLoadingType_GetUpdateBehavior_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the CachedLoadingType_GetUpdateBehavior_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CachedLoadingType_GetUpdateBehavior_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.CachedLoadingType_GetUpdateBehavior_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FileSystemLoadingType_FileSystem_CreateDirectory_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId FileSystemLoadingType_FileSystem_CreateDirectory_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.FileSystemLoadingType_FileSystem_CreateDirectory_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FileSystemLoadingType_FileSystem_CreateDirectory_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId FileSystemLoadingType_FileSystem_CreateDirectory_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.FileSystemLoadingType_FileSystem_CreateDirectory_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FileSystemLoadingType_FileSystem_CreateFile_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId FileSystemLoadingType_FileSystem_CreateFile_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.FileSystemLoadingType_FileSystem_CreateFile_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FileSystemLoadingType_FileSystem_CreateFile_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId FileSystemLoadingType_FileSystem_CreateFile_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.FileSystemLoadingType_FileSystem_CreateFile_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FileSystemLoadingType_FileSystem_DeleteFileSystemObject_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId FileSystemLoadingType_FileSystem_DeleteFileSystemObject_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.FileSystemLoadingType_FileSystem_DeleteFileSystemObject_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FileSystemLoadingType_FileSystem_MoveOrCopy_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId FileSystemLoadingType_FileSystem_MoveOrCopy_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.FileSystemLoadingType_FileSystem_MoveOrCopy_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FileSystemLoadingType_FileSystem_MoveOrCopy_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId FileSystemLoadingType_FileSystem_MoveOrCopy_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.FileSystemLoadingType_FileSystem_MoveOrCopy_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FileSystemLoadingType_GetUpdateBehavior_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId FileSystemLoadingType_GetUpdateBehavior_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.FileSystemLoadingType_GetUpdateBehavior_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FileSystemLoadingType_GetUpdateBehavior_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId FileSystemLoadingType_GetUpdateBehavior_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.FileSystemLoadingType_GetUpdateBehavior_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FileSystemLoadingType_ValidateFiles_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId FileSystemLoadingType_ValidateFiles_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.FileSystemLoadingType_ValidateFiles_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the FileSystemLoadingType_ValidateFiles_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId FileSystemLoadingType_ValidateFiles_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.FileSystemLoadingType_ValidateFiles_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareVersionType_Manufacturer Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareVersionType_Manufacturer = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareVersionType_Manufacturer, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareVersionType_ManufacturerUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareVersionType_ManufacturerUri = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareVersionType_ManufacturerUri, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareVersionType_SoftwareRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareVersionType_SoftwareRevision = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareVersionType_SoftwareRevision, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareVersionType_PatchIdentifiers Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareVersionType_PatchIdentifiers = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareVersionType_PatchIdentifiers, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareVersionType_ReleaseDate Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareVersionType_ReleaseDate = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareVersionType_ReleaseDate, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareVersionType_ChangeLogReference Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareVersionType_ChangeLogReference = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareVersionType_ChangeLogReference, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareVersionType_Hash Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareVersionType_Hash = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareVersionType_Hash, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId PrepareForUpdateStateMachineType_CurrentState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.PrepareForUpdateStateMachineType_CurrentState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId PrepareForUpdateStateMachineType_LastTransition_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.PrepareForUpdateStateMachineType_LastTransition_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_PercentComplete Variable.
        /// </summary>
        public static readonly ExpandedNodeId PrepareForUpdateStateMachineType_PercentComplete = new ExpandedNodeId(Opc.Ua.Di.Variables.PrepareForUpdateStateMachineType_PercentComplete, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_Idle_StateNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId PrepareForUpdateStateMachineType_Idle_StateNumber = new ExpandedNodeId(Opc.Ua.Di.Variables.PrepareForUpdateStateMachineType_Idle_StateNumber, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_Preparing_StateNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId PrepareForUpdateStateMachineType_Preparing_StateNumber = new ExpandedNodeId(Opc.Ua.Di.Variables.PrepareForUpdateStateMachineType_Preparing_StateNumber, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_PreparedForUpdate_StateNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId PrepareForUpdateStateMachineType_PreparedForUpdate_StateNumber = new ExpandedNodeId(Opc.Ua.Di.Variables.PrepareForUpdateStateMachineType_PreparedForUpdate_StateNumber, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_Resuming_StateNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId PrepareForUpdateStateMachineType_Resuming_StateNumber = new ExpandedNodeId(Opc.Ua.Di.Variables.PrepareForUpdateStateMachineType_Resuming_StateNumber, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_IdleToPreparing_TransitionNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId PrepareForUpdateStateMachineType_IdleToPreparing_TransitionNumber = new ExpandedNodeId(Opc.Ua.Di.Variables.PrepareForUpdateStateMachineType_IdleToPreparing_TransitionNumber, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_PreparingToIdle_TransitionNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId PrepareForUpdateStateMachineType_PreparingToIdle_TransitionNumber = new ExpandedNodeId(Opc.Ua.Di.Variables.PrepareForUpdateStateMachineType_PreparingToIdle_TransitionNumber, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_PreparingToPreparedForUpdate_TransitionNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId PrepareForUpdateStateMachineType_PreparingToPreparedForUpdate_TransitionNumber = new ExpandedNodeId(Opc.Ua.Di.Variables.PrepareForUpdateStateMachineType_PreparingToPreparedForUpdate_TransitionNumber, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_PreparedForUpdateToResuming_TransitionNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId PrepareForUpdateStateMachineType_PreparedForUpdateToResuming_TransitionNumber = new ExpandedNodeId(Opc.Ua.Di.Variables.PrepareForUpdateStateMachineType_PreparedForUpdateToResuming_TransitionNumber, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PrepareForUpdateStateMachineType_ResumingToIdle_TransitionNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId PrepareForUpdateStateMachineType_ResumingToIdle_TransitionNumber = new ExpandedNodeId(Opc.Ua.Di.Variables.PrepareForUpdateStateMachineType_ResumingToIdle_TransitionNumber, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the InstallationStateMachineType_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId InstallationStateMachineType_CurrentState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.InstallationStateMachineType_CurrentState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the InstallationStateMachineType_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId InstallationStateMachineType_LastTransition_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.InstallationStateMachineType_LastTransition_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the InstallationStateMachineType_PercentComplete Variable.
        /// </summary>
        public static readonly ExpandedNodeId InstallationStateMachineType_PercentComplete = new ExpandedNodeId(Opc.Ua.Di.Variables.InstallationStateMachineType_PercentComplete, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the InstallationStateMachineType_InstallationDelay Variable.
        /// </summary>
        public static readonly ExpandedNodeId InstallationStateMachineType_InstallationDelay = new ExpandedNodeId(Opc.Ua.Di.Variables.InstallationStateMachineType_InstallationDelay, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the InstallationStateMachineType_InstallSoftwarePackage_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId InstallationStateMachineType_InstallSoftwarePackage_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.InstallationStateMachineType_InstallSoftwarePackage_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the InstallationStateMachineType_InstallSoftwarePackage_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId InstallationStateMachineType_InstallSoftwarePackage_OutputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.InstallationStateMachineType_InstallSoftwarePackage_OutputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the InstallationStateMachineType_InstallFiles_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId InstallationStateMachineType_InstallFiles_InputArguments = new ExpandedNodeId(Opc.Ua.Di.Variables.InstallationStateMachineType_InstallFiles_InputArguments, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the InstallationStateMachineType_Idle_StateNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId InstallationStateMachineType_Idle_StateNumber = new ExpandedNodeId(Opc.Ua.Di.Variables.InstallationStateMachineType_Idle_StateNumber, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the InstallationStateMachineType_Installing_StateNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId InstallationStateMachineType_Installing_StateNumber = new ExpandedNodeId(Opc.Ua.Di.Variables.InstallationStateMachineType_Installing_StateNumber, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the InstallationStateMachineType_Error_StateNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId InstallationStateMachineType_Error_StateNumber = new ExpandedNodeId(Opc.Ua.Di.Variables.InstallationStateMachineType_Error_StateNumber, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the InstallationStateMachineType_IdleToInstalling_TransitionNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId InstallationStateMachineType_IdleToInstalling_TransitionNumber = new ExpandedNodeId(Opc.Ua.Di.Variables.InstallationStateMachineType_IdleToInstalling_TransitionNumber, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the InstallationStateMachineType_InstallingToIdle_TransitionNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId InstallationStateMachineType_InstallingToIdle_TransitionNumber = new ExpandedNodeId(Opc.Ua.Di.Variables.InstallationStateMachineType_InstallingToIdle_TransitionNumber, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the InstallationStateMachineType_InstallingToError_TransitionNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId InstallationStateMachineType_InstallingToError_TransitionNumber = new ExpandedNodeId(Opc.Ua.Di.Variables.InstallationStateMachineType_InstallingToError_TransitionNumber, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the InstallationStateMachineType_ErrorToIdle_TransitionNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId InstallationStateMachineType_ErrorToIdle_TransitionNumber = new ExpandedNodeId(Opc.Ua.Di.Variables.InstallationStateMachineType_ErrorToIdle_TransitionNumber, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PowerCycleStateMachineType_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerCycleStateMachineType_CurrentState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.PowerCycleStateMachineType_CurrentState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PowerCycleStateMachineType_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerCycleStateMachineType_LastTransition_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.PowerCycleStateMachineType_LastTransition_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PowerCycleStateMachineType_NotWaitingForPowerCycle_StateNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerCycleStateMachineType_NotWaitingForPowerCycle_StateNumber = new ExpandedNodeId(Opc.Ua.Di.Variables.PowerCycleStateMachineType_NotWaitingForPowerCycle_StateNumber, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PowerCycleStateMachineType_WaitingForPowerCycle_StateNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerCycleStateMachineType_WaitingForPowerCycle_StateNumber = new ExpandedNodeId(Opc.Ua.Di.Variables.PowerCycleStateMachineType_WaitingForPowerCycle_StateNumber, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PowerCycleStateMachineType_NotWaitingForPowerCycleToWaitingForPowerCycle_TransitionNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerCycleStateMachineType_NotWaitingForPowerCycleToWaitingForPowerCycle_TransitionNumber = new ExpandedNodeId(Opc.Ua.Di.Variables.PowerCycleStateMachineType_NotWaitingForPowerCycleToWaitingForPowerCycle_TransitionNumber, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the PowerCycleStateMachineType_WaitingForPowerCycleToNotWaitingForPowerCycle_TransitionNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId PowerCycleStateMachineType_WaitingForPowerCycleToNotWaitingForPowerCycle_TransitionNumber = new ExpandedNodeId(Opc.Ua.Di.Variables.PowerCycleStateMachineType_WaitingForPowerCycleToNotWaitingForPowerCycle_TransitionNumber, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConfirmationStateMachineType_CurrentState_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConfirmationStateMachineType_CurrentState_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.ConfirmationStateMachineType_CurrentState_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConfirmationStateMachineType_LastTransition_Id Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConfirmationStateMachineType_LastTransition_Id = new ExpandedNodeId(Opc.Ua.Di.Variables.ConfirmationStateMachineType_LastTransition_Id, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConfirmationStateMachineType_ConfirmationTimeout Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConfirmationStateMachineType_ConfirmationTimeout = new ExpandedNodeId(Opc.Ua.Di.Variables.ConfirmationStateMachineType_ConfirmationTimeout, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConfirmationStateMachineType_NotWaitingForConfirm_StateNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConfirmationStateMachineType_NotWaitingForConfirm_StateNumber = new ExpandedNodeId(Opc.Ua.Di.Variables.ConfirmationStateMachineType_NotWaitingForConfirm_StateNumber, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConfirmationStateMachineType_WaitingForConfirm_StateNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConfirmationStateMachineType_WaitingForConfirm_StateNumber = new ExpandedNodeId(Opc.Ua.Di.Variables.ConfirmationStateMachineType_WaitingForConfirm_StateNumber, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConfirmationStateMachineType_NotWaitingForConfirmToWaitingForConfirm_TransitionNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConfirmationStateMachineType_NotWaitingForConfirmToWaitingForConfirm_TransitionNumber = new ExpandedNodeId(Opc.Ua.Di.Variables.ConfirmationStateMachineType_NotWaitingForConfirmToWaitingForConfirm_TransitionNumber, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the ConfirmationStateMachineType_WaitingForConfirmToNotWaitingForConfirm_TransitionNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConfirmationStateMachineType_WaitingForConfirmToNotWaitingForConfirm_TransitionNumber = new ExpandedNodeId(Opc.Ua.Di.Variables.ConfirmationStateMachineType_WaitingForConfirmToNotWaitingForConfirm_TransitionNumber, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the SoftwareVersionFileType_EnumStrings Variable.
        /// </summary>
        public static readonly ExpandedNodeId SoftwareVersionFileType_EnumStrings = new ExpandedNodeId(Opc.Ua.Di.Variables.SoftwareVersionFileType_EnumStrings, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the UpdateBehavior_OptionSetValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId UpdateBehavior_OptionSetValues = new ExpandedNodeId(Opc.Ua.Di.Variables.UpdateBehavior_OptionSetValues, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OpcUaDi_BinarySchema Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaDi_BinarySchema = new ExpandedNodeId(Opc.Ua.Di.Variables.OpcUaDi_BinarySchema, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OpcUaDi_BinarySchema_NamespaceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaDi_BinarySchema_NamespaceUri = new ExpandedNodeId(Opc.Ua.Di.Variables.OpcUaDi_BinarySchema_NamespaceUri, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OpcUaDi_BinarySchema_Deprecated Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaDi_BinarySchema_Deprecated = new ExpandedNodeId(Opc.Ua.Di.Variables.OpcUaDi_BinarySchema_Deprecated, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OpcUaDi_BinarySchema_FetchResultDataType Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaDi_BinarySchema_FetchResultDataType = new ExpandedNodeId(Opc.Ua.Di.Variables.OpcUaDi_BinarySchema_FetchResultDataType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OpcUaDi_BinarySchema_TransferResultErrorDataType Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaDi_BinarySchema_TransferResultErrorDataType = new ExpandedNodeId(Opc.Ua.Di.Variables.OpcUaDi_BinarySchema_TransferResultErrorDataType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OpcUaDi_BinarySchema_TransferResultDataDataType Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaDi_BinarySchema_TransferResultDataDataType = new ExpandedNodeId(Opc.Ua.Di.Variables.OpcUaDi_BinarySchema_TransferResultDataDataType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OpcUaDi_BinarySchema_ParameterResultDataType Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaDi_BinarySchema_ParameterResultDataType = new ExpandedNodeId(Opc.Ua.Di.Variables.OpcUaDi_BinarySchema_ParameterResultDataType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OpcUaDi_XmlSchema Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaDi_XmlSchema = new ExpandedNodeId(Opc.Ua.Di.Variables.OpcUaDi_XmlSchema, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OpcUaDi_XmlSchema_NamespaceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaDi_XmlSchema_NamespaceUri = new ExpandedNodeId(Opc.Ua.Di.Variables.OpcUaDi_XmlSchema_NamespaceUri, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OpcUaDi_XmlSchema_Deprecated Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaDi_XmlSchema_Deprecated = new ExpandedNodeId(Opc.Ua.Di.Variables.OpcUaDi_XmlSchema_Deprecated, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OpcUaDi_XmlSchema_FetchResultDataType Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaDi_XmlSchema_FetchResultDataType = new ExpandedNodeId(Opc.Ua.Di.Variables.OpcUaDi_XmlSchema_FetchResultDataType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OpcUaDi_XmlSchema_TransferResultErrorDataType Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaDi_XmlSchema_TransferResultErrorDataType = new ExpandedNodeId(Opc.Ua.Di.Variables.OpcUaDi_XmlSchema_TransferResultErrorDataType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OpcUaDi_XmlSchema_TransferResultDataDataType Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaDi_XmlSchema_TransferResultDataDataType = new ExpandedNodeId(Opc.Ua.Di.Variables.OpcUaDi_XmlSchema_TransferResultDataDataType, Opc.Ua.Di.Namespaces.OpcUaDi);

        /// <summary>
        /// The identifier for the OpcUaDi_XmlSchema_ParameterResultDataType Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaDi_XmlSchema_ParameterResultDataType = new ExpandedNodeId(Opc.Ua.Di.Variables.OpcUaDi_XmlSchema_ParameterResultDataType, Opc.Ua.Di.Namespaces.OpcUaDi);
    }
    #endregion

    #region VariableType Node Identifiers
    /// <summary>
    /// A class that declares constants for all VariableTypes in the Model Design.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class VariableTypeIds
    {
        /// <summary>
        /// The identifier for the UIElementType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId UIElementType = new ExpandedNodeId(Opc.Ua.Di.VariableTypes.UIElementType, Opc.Ua.Di.Namespaces.OpcUaDi);
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
        /// The BrowseName for the Abort component.
        /// </summary>
        public const string Abort = "Abort";

        /// <summary>
        /// The BrowseName for the ActualMode component.
        /// </summary>
        public const string ActualMode = "ActualMode";

        /// <summary>
        /// The BrowseName for the AssetId component.
        /// </summary>
        public const string AssetId = "AssetId";

        /// <summary>
        /// The BrowseName for the BlockType component.
        /// </summary>
        public const string BlockType = "BlockType";

        /// <summary>
        /// The BrowseName for the BreakLock component.
        /// </summary>
        public const string BreakLock = "BreakLock";

        /// <summary>
        /// The BrowseName for the CachedLoadingType component.
        /// </summary>
        public const string CachedLoadingType = "CachedLoadingType";

        /// <summary>
        /// The BrowseName for the ChangeLogReference component.
        /// </summary>
        public const string ChangeLogReference = "ChangeLogReference";

        /// <summary>
        /// The BrowseName for the CheckFunctionAlarmType component.
        /// </summary>
        public const string CheckFunctionAlarmType = "CheckFunctionAlarmType";

        /// <summary>
        /// The BrowseName for the ComponentName component.
        /// </summary>
        public const string ComponentName = "ComponentName";

        /// <summary>
        /// The BrowseName for the ComponentType component.
        /// </summary>
        public const string ComponentType = "ComponentType";

        /// <summary>
        /// The BrowseName for the ConfigurableObjectType component.
        /// </summary>
        public const string ConfigurableObjectType = "ConfigurableObjectType";

        /// <summary>
        /// The BrowseName for the Confirm component.
        /// </summary>
        public const string Confirm = "Confirm";

        /// <summary>
        /// The BrowseName for the Confirmation component.
        /// </summary>
        public const string Confirmation = "Confirmation";

        /// <summary>
        /// The BrowseName for the ConfirmationStateMachineType component.
        /// </summary>
        public const string ConfirmationStateMachineType = "ConfirmationStateMachineType";

        /// <summary>
        /// The BrowseName for the ConfirmationTimeout component.
        /// </summary>
        public const string ConfirmationTimeout = "ConfirmationTimeout";

        /// <summary>
        /// The BrowseName for the ConnectionPointType component.
        /// </summary>
        public const string ConnectionPointType = "ConnectionPointType";

        /// <summary>
        /// The BrowseName for the ConnectsTo component.
        /// </summary>
        public const string ConnectsTo = "ConnectsTo";

        /// <summary>
        /// The BrowseName for the ConnectsToParent component.
        /// </summary>
        public const string ConnectsToParent = "ConnectsToParent";

        /// <summary>
        /// The BrowseName for the CPIdentifier component.
        /// </summary>
        public const string CPIdentifier = "<CPIdentifier>";

        /// <summary>
        /// The BrowseName for the CurrentVersion component.
        /// </summary>
        public const string CurrentVersion = "CurrentVersion";

        /// <summary>
        /// The BrowseName for the DeviceClass component.
        /// </summary>
        public const string DeviceClass = "DeviceClass";

        /// <summary>
        /// The BrowseName for the DeviceFeatures component.
        /// </summary>
        public const string DeviceFeatures = "DeviceFeatures";

        /// <summary>
        /// The BrowseName for the DeviceHealth component.
        /// </summary>
        public const string DeviceHealth = "DeviceHealth";

        /// <summary>
        /// The BrowseName for the DeviceHealthAlarms component.
        /// </summary>
        public const string DeviceHealthAlarms = "DeviceHealthAlarms";

        /// <summary>
        /// The BrowseName for the DeviceHealthDiagnosticAlarmType component.
        /// </summary>
        public const string DeviceHealthDiagnosticAlarmType = "DeviceHealthDiagnosticAlarmType";

        /// <summary>
        /// The BrowseName for the DeviceHealthEnumeration component.
        /// </summary>
        public const string DeviceHealthEnumeration = "DeviceHealthEnumeration";

        /// <summary>
        /// The BrowseName for the DeviceManual component.
        /// </summary>
        public const string DeviceManual = "DeviceManual";

        /// <summary>
        /// The BrowseName for the DeviceRevision component.
        /// </summary>
        public const string DeviceRevision = "DeviceRevision";

        /// <summary>
        /// The BrowseName for the DeviceSet component.
        /// </summary>
        public const string DeviceSet = "DeviceSet";

        /// <summary>
        /// The BrowseName for the DeviceTopology component.
        /// </summary>
        public const string DeviceTopology = "DeviceTopology";

        /// <summary>
        /// The BrowseName for the DeviceType component.
        /// </summary>
        public const string DeviceType = "DeviceType";

        /// <summary>
        /// The BrowseName for the DeviceTypeImage component.
        /// </summary>
        public const string DeviceTypeImage = "DeviceTypeImage";

        /// <summary>
        /// The BrowseName for the DirectLoadingType component.
        /// </summary>
        public const string DirectLoadingType = "DirectLoadingType";

        /// <summary>
        /// The BrowseName for the Documentation component.
        /// </summary>
        public const string Documentation = "Documentation";

        /// <summary>
        /// The BrowseName for the Error component.
        /// </summary>
        public const string Error = "Error";

        /// <summary>
        /// The BrowseName for the ErrorMessage component.
        /// </summary>
        public const string ErrorMessage = "ErrorMessage";

        /// <summary>
        /// The BrowseName for the ErrorToIdle component.
        /// </summary>
        public const string ErrorToIdle = "ErrorToIdle";

        /// <summary>
        /// The BrowseName for the ExitLock component.
        /// </summary>
        public const string ExitLock = "ExitLock";

        /// <summary>
        /// The BrowseName for the FailureAlarmType component.
        /// </summary>
        public const string FailureAlarmType = "FailureAlarmType";

        /// <summary>
        /// The BrowseName for the FallbackVersion component.
        /// </summary>
        public const string FallbackVersion = "FallbackVersion";

        /// <summary>
        /// The BrowseName for the FetchResultDataType component.
        /// </summary>
        public const string FetchResultDataType = "FetchResultDataType";

        /// <summary>
        /// The BrowseName for the FetchTransferResultData component.
        /// </summary>
        public const string FetchTransferResultData = "FetchTransferResultData";

        /// <summary>
        /// The BrowseName for the FileSystemLoadingType component.
        /// </summary>
        public const string FileSystemLoadingType = "FileSystemLoadingType";

        /// <summary>
        /// The BrowseName for the FileTransfer component.
        /// </summary>
        public const string FileTransfer = "FileTransfer";

        /// <summary>
        /// The BrowseName for the FunctionalGroupType component.
        /// </summary>
        public const string FunctionalGroupType = "FunctionalGroupType";

        /// <summary>
        /// The BrowseName for the GetUpdateBehavior component.
        /// </summary>
        public const string GetUpdateBehavior = "GetUpdateBehavior";

        /// <summary>
        /// The BrowseName for the GroupIdentifier component.
        /// </summary>
        public const string GroupIdentifier = "<GroupIdentifier>";

        /// <summary>
        /// The BrowseName for the HardwareRevision component.
        /// </summary>
        public const string HardwareRevision = "HardwareRevision";

        /// <summary>
        /// The BrowseName for the Hash component.
        /// </summary>
        public const string Hash = "Hash";

        /// <summary>
        /// The BrowseName for the Identification component.
        /// </summary>
        public const string Identification = "Identification";

        /// <summary>
        /// The BrowseName for the IDeviceHealthType component.
        /// </summary>
        public const string IDeviceHealthType = "IDeviceHealthType";

        /// <summary>
        /// The BrowseName for the Idle component.
        /// </summary>
        public const string Idle = "Idle";

        /// <summary>
        /// The BrowseName for the IdleToInstalling component.
        /// </summary>
        public const string IdleToInstalling = "IdleToInstalling";

        /// <summary>
        /// The BrowseName for the IdleToPreparing component.
        /// </summary>
        public const string IdleToPreparing = "IdleToPreparing";

        /// <summary>
        /// The BrowseName for the ImageSet component.
        /// </summary>
        public const string ImageSet = "ImageSet";

        /// <summary>
        /// The BrowseName for the InitLock component.
        /// </summary>
        public const string InitLock = "InitLock";

        /// <summary>
        /// The BrowseName for the Installation component.
        /// </summary>
        public const string Installation = "Installation";

        /// <summary>
        /// The BrowseName for the InstallationDelay component.
        /// </summary>
        public const string InstallationDelay = "InstallationDelay";

        /// <summary>
        /// The BrowseName for the InstallationStateMachineType component.
        /// </summary>
        public const string InstallationStateMachineType = "InstallationStateMachineType";

        /// <summary>
        /// The BrowseName for the InstallFiles component.
        /// </summary>
        public const string InstallFiles = "InstallFiles";

        /// <summary>
        /// The BrowseName for the Installing component.
        /// </summary>
        public const string Installing = "Installing";

        /// <summary>
        /// The BrowseName for the InstallingToError component.
        /// </summary>
        public const string InstallingToError = "InstallingToError";

        /// <summary>
        /// The BrowseName for the InstallingToIdle component.
        /// </summary>
        public const string InstallingToIdle = "InstallingToIdle";

        /// <summary>
        /// The BrowseName for the InstallSoftwarePackage component.
        /// </summary>
        public const string InstallSoftwarePackage = "InstallSoftwarePackage";

        /// <summary>
        /// The BrowseName for the IsOnline component.
        /// </summary>
        public const string IsOnline = "IsOnline";

        /// <summary>
        /// The BrowseName for the ISupportInfoType component.
        /// </summary>
        public const string ISupportInfoType = "ISupportInfoType";

        /// <summary>
        /// The BrowseName for the ITagNameplateType component.
        /// </summary>
        public const string ITagNameplateType = "ITagNameplateType";

        /// <summary>
        /// The BrowseName for the IVendorNameplateType component.
        /// </summary>
        public const string IVendorNameplateType = "IVendorNameplateType";

        /// <summary>
        /// The BrowseName for the Loading component.
        /// </summary>
        public const string Loading = "Loading";

        /// <summary>
        /// The BrowseName for the Lock component.
        /// </summary>
        public const string Lock = "Lock";

        /// <summary>
        /// The BrowseName for the Locked component.
        /// </summary>
        public const string Locked = "Locked";

        /// <summary>
        /// The BrowseName for the LockingClient component.
        /// </summary>
        public const string LockingClient = "LockingClient";

        /// <summary>
        /// The BrowseName for the LockingServicesType component.
        /// </summary>
        public const string LockingServicesType = "LockingServicesType";

        /// <summary>
        /// The BrowseName for the LockingUser component.
        /// </summary>
        public const string LockingUser = "LockingUser";

        /// <summary>
        /// The BrowseName for the MaintenanceRequiredAlarmType component.
        /// </summary>
        public const string MaintenanceRequiredAlarmType = "MaintenanceRequiredAlarmType";

        /// <summary>
        /// The BrowseName for the Manufacturer component.
        /// </summary>
        public const string Manufacturer = "Manufacturer";

        /// <summary>
        /// The BrowseName for the ManufacturerUri component.
        /// </summary>
        public const string ManufacturerUri = "ManufacturerUri";

        /// <summary>
        /// The BrowseName for the MaxInactiveLockTime component.
        /// </summary>
        public const string MaxInactiveLockTime = "MaxInactiveLockTime";

        /// <summary>
        /// The BrowseName for the MethodSet component.
        /// </summary>
        public const string MethodSet = "MethodSet";

        /// <summary>
        /// The BrowseName for the Model component.
        /// </summary>
        public const string Model = "Model";

        /// <summary>
        /// The BrowseName for the NetworkAddress component.
        /// </summary>
        public const string NetworkAddress = "NetworkAddress";

        /// <summary>
        /// The BrowseName for the NetworkIdentifier component.
        /// </summary>
        public const string NetworkIdentifier = "<NetworkIdentifier>";

        /// <summary>
        /// The BrowseName for the NetworkSet component.
        /// </summary>
        public const string NetworkSet = "NetworkSet";

        /// <summary>
        /// The BrowseName for the NetworkType component.
        /// </summary>
        public const string NetworkType = "NetworkType";

        /// <summary>
        /// The BrowseName for the NormalMode component.
        /// </summary>
        public const string NormalMode = "NormalMode";

        /// <summary>
        /// The BrowseName for the NotWaitingForConfirm component.
        /// </summary>
        public const string NotWaitingForConfirm = "NotWaitingForConfirm";

        /// <summary>
        /// The BrowseName for the NotWaitingForConfirmToWaitingForConfirm component.
        /// </summary>
        public const string NotWaitingForConfirmToWaitingForConfirm = "NotWaitingForConfirmToWaitingForConfirm";

        /// <summary>
        /// The BrowseName for the NotWaitingForPowerCycle component.
        /// </summary>
        public const string NotWaitingForPowerCycle = "NotWaitingForPowerCycle";

        /// <summary>
        /// The BrowseName for the NotWaitingForPowerCycleToWaitingForPowerCycle component.
        /// </summary>
        public const string NotWaitingForPowerCycleToWaitingForPowerCycle = "NotWaitingForPowerCycleToWaitingForPowerCycle";

        /// <summary>
        /// The BrowseName for the ObjectIdentifier component.
        /// </summary>
        public const string ObjectIdentifier = "<ObjectIdentifier>";

        /// <summary>
        /// The BrowseName for the OffSpecAlarmType component.
        /// </summary>
        public const string OffSpecAlarmType = "OffSpecAlarmType";

        /// <summary>
        /// The BrowseName for the OnlineAccess component.
        /// </summary>
        public const string OnlineAccess = "OnlineAccess";

        /// <summary>
        /// The BrowseName for the OpcUaDi_BinarySchema component.
        /// </summary>
        public const string OpcUaDi_BinarySchema = "Opc.Ua.Di";

        /// <summary>
        /// The BrowseName for the OpcUaDi_XmlSchema component.
        /// </summary>
        public const string OpcUaDi_XmlSchema = "Opc.Ua.Di";

        /// <summary>
        /// The BrowseName for the OPCUADINamespaceMetadata component.
        /// </summary>
        public const string OPCUADINamespaceMetadata = "http://opcfoundation.org/UA/DI/";

        /// <summary>
        /// The BrowseName for the PackageLoadingType component.
        /// </summary>
        public const string PackageLoadingType = "PackageLoadingType";

        /// <summary>
        /// The BrowseName for the ParameterResultDataType component.
        /// </summary>
        public const string ParameterResultDataType = "ParameterResultDataType";

        /// <summary>
        /// The BrowseName for the Parameters component.
        /// </summary>
        public const string Parameters = "Parameters";

        /// <summary>
        /// The BrowseName for the ParameterSet component.
        /// </summary>
        public const string ParameterSet = "ParameterSet";

        /// <summary>
        /// The BrowseName for the PatchIdentifiers component.
        /// </summary>
        public const string PatchIdentifiers = "PatchIdentifiers";

        /// <summary>
        /// The BrowseName for the PendingVersion component.
        /// </summary>
        public const string PendingVersion = "PendingVersion";

        /// <summary>
        /// The BrowseName for the PercentComplete component.
        /// </summary>
        public const string PercentComplete = "PercentComplete";

        /// <summary>
        /// The BrowseName for the PermittedMode component.
        /// </summary>
        public const string PermittedMode = "PermittedMode";

        /// <summary>
        /// The BrowseName for the PowerCycle component.
        /// </summary>
        public const string PowerCycle = "PowerCycle";

        /// <summary>
        /// The BrowseName for the PowerCycleStateMachineType component.
        /// </summary>
        public const string PowerCycleStateMachineType = "PowerCycleStateMachineType";

        /// <summary>
        /// The BrowseName for the Prepare component.
        /// </summary>
        public const string Prepare = "Prepare";

        /// <summary>
        /// The BrowseName for the PreparedForUpdate component.
        /// </summary>
        public const string PreparedForUpdate = "PreparedForUpdate";

        /// <summary>
        /// The BrowseName for the PreparedForUpdateToResuming component.
        /// </summary>
        public const string PreparedForUpdateToResuming = "PreparedForUpdateToResuming";

        /// <summary>
        /// The BrowseName for the PrepareForUpdate component.
        /// </summary>
        public const string PrepareForUpdate = "PrepareForUpdate";

        /// <summary>
        /// The BrowseName for the PrepareForUpdateStateMachineType component.
        /// </summary>
        public const string PrepareForUpdateStateMachineType = "PrepareForUpdateStateMachineType";

        /// <summary>
        /// The BrowseName for the Preparing component.
        /// </summary>
        public const string Preparing = "Preparing";

        /// <summary>
        /// The BrowseName for the PreparingToIdle component.
        /// </summary>
        public const string PreparingToIdle = "PreparingToIdle";

        /// <summary>
        /// The BrowseName for the PreparingToPreparedForUpdate component.
        /// </summary>
        public const string PreparingToPreparedForUpdate = "PreparingToPreparedForUpdate";

        /// <summary>
        /// The BrowseName for the ProductCode component.
        /// </summary>
        public const string ProductCode = "ProductCode";

        /// <summary>
        /// The BrowseName for the ProductInstanceUri component.
        /// </summary>
        public const string ProductInstanceUri = "ProductInstanceUri";

        /// <summary>
        /// The BrowseName for the ProfileIdentifier component.
        /// </summary>
        public const string ProfileIdentifier = "<ProfileIdentifier>";

        /// <summary>
        /// The BrowseName for the ProtocolSupport component.
        /// </summary>
        public const string ProtocolSupport = "ProtocolSupport";

        /// <summary>
        /// The BrowseName for the ProtocolType component.
        /// </summary>
        public const string ProtocolType = "ProtocolType";

        /// <summary>
        /// The BrowseName for the ReleaseDate component.
        /// </summary>
        public const string ReleaseDate = "ReleaseDate";

        /// <summary>
        /// The BrowseName for the RemainingLockTime component.
        /// </summary>
        public const string RemainingLockTime = "RemainingLockTime";

        /// <summary>
        /// The BrowseName for the RenewLock component.
        /// </summary>
        public const string RenewLock = "RenewLock";

        /// <summary>
        /// The BrowseName for the Resume component.
        /// </summary>
        public const string Resume = "Resume";

        /// <summary>
        /// The BrowseName for the Resuming component.
        /// </summary>
        public const string Resuming = "Resuming";

        /// <summary>
        /// The BrowseName for the ResumingToIdle component.
        /// </summary>
        public const string ResumingToIdle = "ResumingToIdle";

        /// <summary>
        /// The BrowseName for the RevisionCounter component.
        /// </summary>
        public const string RevisionCounter = "RevisionCounter";

        /// <summary>
        /// The BrowseName for the SerialNumber component.
        /// </summary>
        public const string SerialNumber = "SerialNumber";

        /// <summary>
        /// The BrowseName for the SoftwareLoadingType component.
        /// </summary>
        public const string SoftwareLoadingType = "SoftwareLoadingType";

        /// <summary>
        /// The BrowseName for the SoftwareRevision component.
        /// </summary>
        public const string SoftwareRevision = "SoftwareRevision";

        /// <summary>
        /// The BrowseName for the SoftwareType component.
        /// </summary>
        public const string SoftwareType = "SoftwareType";

        /// <summary>
        /// The BrowseName for the SoftwareUpdateType component.
        /// </summary>
        public const string SoftwareUpdateType = "SoftwareUpdateType";

        /// <summary>
        /// The BrowseName for the SoftwareVersionFileType component.
        /// </summary>
        public const string SoftwareVersionFileType = "SoftwareVersionFileType";

        /// <summary>
        /// The BrowseName for the SoftwareVersionType component.
        /// </summary>
        public const string SoftwareVersionType = "SoftwareVersionType";

        /// <summary>
        /// The BrowseName for the SupportedTypes component.
        /// </summary>
        public const string SupportedTypes = "SupportedTypes";

        /// <summary>
        /// The BrowseName for the TargetMode component.
        /// </summary>
        public const string TargetMode = "TargetMode";

        /// <summary>
        /// The BrowseName for the TopologyElementType component.
        /// </summary>
        public const string TopologyElementType = "TopologyElementType";

        /// <summary>
        /// The BrowseName for the TransferFromDevice component.
        /// </summary>
        public const string TransferFromDevice = "TransferFromDevice";

        /// <summary>
        /// The BrowseName for the TransferResultDataDataType component.
        /// </summary>
        public const string TransferResultDataDataType = "TransferResultDataDataType";

        /// <summary>
        /// The BrowseName for the TransferResultErrorDataType component.
        /// </summary>
        public const string TransferResultErrorDataType = "TransferResultErrorDataType";

        /// <summary>
        /// The BrowseName for the TransferServicesType component.
        /// </summary>
        public const string TransferServicesType = "TransferServicesType";

        /// <summary>
        /// The BrowseName for the TransferToDevice component.
        /// </summary>
        public const string TransferToDevice = "TransferToDevice";

        /// <summary>
        /// The BrowseName for the UIElement component.
        /// </summary>
        public const string UIElement = "UIElement";

        /// <summary>
        /// The BrowseName for the UIElementType component.
        /// </summary>
        public const string UIElementType = "UIElementType";

        /// <summary>
        /// The BrowseName for the UpdateBehavior component.
        /// </summary>
        public const string UpdateBehavior = "UpdateBehavior";

        /// <summary>
        /// The BrowseName for the UpdateKey component.
        /// </summary>
        public const string UpdateKey = "UpdateKey";

        /// <summary>
        /// The BrowseName for the UpdateStatus component.
        /// </summary>
        public const string UpdateStatus = "UpdateStatus";

        /// <summary>
        /// The BrowseName for the ValidateFiles component.
        /// </summary>
        public const string ValidateFiles = "ValidateFiles";

        /// <summary>
        /// The BrowseName for the VendorErrorCode component.
        /// </summary>
        public const string VendorErrorCode = "VendorErrorCode";

        /// <summary>
        /// The BrowseName for the WaitingForConfirm component.
        /// </summary>
        public const string WaitingForConfirm = "WaitingForConfirm";

        /// <summary>
        /// The BrowseName for the WaitingForConfirmToNotWaitingForConfirm component.
        /// </summary>
        public const string WaitingForConfirmToNotWaitingForConfirm = "WaitingForConfirmToNotWaitingForConfirm";

        /// <summary>
        /// The BrowseName for the WaitingForPowerCycle component.
        /// </summary>
        public const string WaitingForPowerCycle = "WaitingForPowerCycle";

        /// <summary>
        /// The BrowseName for the WaitingForPowerCycleToNotWaitingForPowerCycle component.
        /// </summary>
        public const string WaitingForPowerCycleToNotWaitingForPowerCycle = "WaitingForPowerCycleToNotWaitingForPowerCycle";

        /// <summary>
        /// The BrowseName for the WriteBlockSize component.
        /// </summary>
        public const string WriteBlockSize = "WriteBlockSize";

        /// <summary>
        /// The BrowseName for the WriteTimeout component.
        /// </summary>
        public const string WriteTimeout = "WriteTimeout";
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
