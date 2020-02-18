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
using Opc.Ua;
using Opc.Ua.Di;

namespace Opc.Ua.Plc
{
    #region Method Identifiers
    /// <summary>
    /// A class that declares constants for all Methods in the Model Design.
    /// </summary>
    /// <exclude />
    [System.CodeDom.Compiler.GeneratedCodeAttribute("Opc.Ua.ModelCompiler", "1.0.0.0")]
    public static partial class Methods
    {
        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_Open Method.
        /// </summary>
        public const uint OpcUaPlcNamespaceMetadata_NamespaceFile_Open = 15015;

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_Close Method.
        /// </summary>
        public const uint OpcUaPlcNamespaceMetadata_NamespaceFile_Close = 15018;

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_Read Method.
        /// </summary>
        public const uint OpcUaPlcNamespaceMetadata_NamespaceFile_Read = 15020;

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_Write Method.
        /// </summary>
        public const uint OpcUaPlcNamespaceMetadata_NamespaceFile_Write = 15023;

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_GetPosition Method.
        /// </summary>
        public const uint OpcUaPlcNamespaceMetadata_NamespaceFile_GetPosition = 15025;

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_SetPosition Method.
        /// </summary>
        public const uint OpcUaPlcNamespaceMetadata_NamespaceFile_SetPosition = 15028;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Lock_InitLock Method.
        /// </summary>
        public const uint CtrlConfigurationType_Lock_InitLock = 7028;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Lock_RenewLock Method.
        /// </summary>
        public const uint CtrlConfigurationType_Lock_RenewLock = 7031;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Lock_ExitLock Method.
        /// </summary>
        public const uint CtrlConfigurationType_Lock_ExitLock = 7033;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Lock_BreakLock Method.
        /// </summary>
        public const uint CtrlConfigurationType_Lock_BreakLock = 7035;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint CtrlConfigurationType_CPIdentifier_Lock_InitLock = 7238;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint CtrlConfigurationType_CPIdentifier_Lock_RenewLock = 7241;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint CtrlConfigurationType_CPIdentifier_Lock_ExitLock = 7243;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint CtrlConfigurationType_CPIdentifier_Lock_BreakLock = 7245;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_MethodSet_Start Method.
        /// </summary>
        public const uint CtrlConfigurationType_MethodSet_Start = 7001;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_MethodSet_Stop Method.
        /// </summary>
        public const uint CtrlConfigurationType_MethodSet_Stop = 7002;

        /// <summary>
        /// The identifier for the CtrlResourceType_Lock_InitLock Method.
        /// </summary>
        public const uint CtrlResourceType_Lock_InitLock = 7067;

        /// <summary>
        /// The identifier for the CtrlResourceType_Lock_RenewLock Method.
        /// </summary>
        public const uint CtrlResourceType_Lock_RenewLock = 7070;

        /// <summary>
        /// The identifier for the CtrlResourceType_Lock_ExitLock Method.
        /// </summary>
        public const uint CtrlResourceType_Lock_ExitLock = 7072;

        /// <summary>
        /// The identifier for the CtrlResourceType_Lock_BreakLock Method.
        /// </summary>
        public const uint CtrlResourceType_Lock_BreakLock = 7074;

        /// <summary>
        /// The identifier for the CtrlResourceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint CtrlResourceType_CPIdentifier_Lock_InitLock = 7263;

        /// <summary>
        /// The identifier for the CtrlResourceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint CtrlResourceType_CPIdentifier_Lock_RenewLock = 7266;

        /// <summary>
        /// The identifier for the CtrlResourceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint CtrlResourceType_CPIdentifier_Lock_ExitLock = 7268;

        /// <summary>
        /// The identifier for the CtrlResourceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint CtrlResourceType_CPIdentifier_Lock_BreakLock = 7270;

        /// <summary>
        /// The identifier for the CtrlResourceType_MethodSet_Start Method.
        /// </summary>
        public const uint CtrlResourceType_MethodSet_Start = 7003;

        /// <summary>
        /// The identifier for the CtrlResourceType_MethodSet_Stop Method.
        /// </summary>
        public const uint CtrlResourceType_MethodSet_Stop = 7004;

        /// <summary>
        /// The identifier for the CtrlProgramOrganizationUnitType_Lock_InitLock Method.
        /// </summary>
        public const uint CtrlProgramOrganizationUnitType_Lock_InitLock = 7104;

        /// <summary>
        /// The identifier for the CtrlProgramOrganizationUnitType_Lock_RenewLock Method.
        /// </summary>
        public const uint CtrlProgramOrganizationUnitType_Lock_RenewLock = 7107;

        /// <summary>
        /// The identifier for the CtrlProgramOrganizationUnitType_Lock_ExitLock Method.
        /// </summary>
        public const uint CtrlProgramOrganizationUnitType_Lock_ExitLock = 7109;

        /// <summary>
        /// The identifier for the CtrlProgramOrganizationUnitType_Lock_BreakLock Method.
        /// </summary>
        public const uint CtrlProgramOrganizationUnitType_Lock_BreakLock = 7111;

        /// <summary>
        /// The identifier for the CtrlProgramType_Lock_InitLock Method.
        /// </summary>
        public const uint CtrlProgramType_Lock_InitLock = 7129;

        /// <summary>
        /// The identifier for the CtrlProgramType_Lock_RenewLock Method.
        /// </summary>
        public const uint CtrlProgramType_Lock_RenewLock = 7132;

        /// <summary>
        /// The identifier for the CtrlProgramType_Lock_ExitLock Method.
        /// </summary>
        public const uint CtrlProgramType_Lock_ExitLock = 7134;

        /// <summary>
        /// The identifier for the CtrlProgramType_Lock_BreakLock Method.
        /// </summary>
        public const uint CtrlProgramType_Lock_BreakLock = 7136;

        /// <summary>
        /// The identifier for the CtrlFunctionBlockType_Lock_InitLock Method.
        /// </summary>
        public const uint CtrlFunctionBlockType_Lock_InitLock = 7154;

        /// <summary>
        /// The identifier for the CtrlFunctionBlockType_Lock_RenewLock Method.
        /// </summary>
        public const uint CtrlFunctionBlockType_Lock_RenewLock = 7157;

        /// <summary>
        /// The identifier for the CtrlFunctionBlockType_Lock_ExitLock Method.
        /// </summary>
        public const uint CtrlFunctionBlockType_Lock_ExitLock = 7159;

        /// <summary>
        /// The identifier for the CtrlFunctionBlockType_Lock_BreakLock Method.
        /// </summary>
        public const uint CtrlFunctionBlockType_Lock_BreakLock = 7161;
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
        /// The identifier for the OpcUaPlcNamespaceMetadata Object.
        /// </summary>
        public const uint OpcUaPlcNamespaceMetadata = 15001;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_MethodSet Object.
        /// </summary>
        public const uint CtrlConfigurationType_MethodSet = 5002;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public const uint CtrlConfigurationType_CPIdentifier_NetworkAddress = 7247;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Resources Object.
        /// </summary>
        public const uint CtrlConfigurationType_Resources = 5004;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Resources_SupportedTypes Object.
        /// </summary>
        public const uint CtrlConfigurationType_Resources_SupportedTypes = 5005;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_GlobalVars Object.
        /// </summary>
        public const uint CtrlConfigurationType_GlobalVars = 5006;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_AccessVars Object.
        /// </summary>
        public const uint CtrlConfigurationType_AccessVars = 5007;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_ConfigVars Object.
        /// </summary>
        public const uint CtrlConfigurationType_ConfigVars = 5008;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Configuration Object.
        /// </summary>
        public const uint CtrlConfigurationType_Configuration = 5009;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Diagnostic Object.
        /// </summary>
        public const uint CtrlConfigurationType_Diagnostic = 5010;

        /// <summary>
        /// The identifier for the CtrlResourceType_MethodSet Object.
        /// </summary>
        public const uint CtrlResourceType_MethodSet = 5012;

        /// <summary>
        /// The identifier for the CtrlResourceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public const uint CtrlResourceType_CPIdentifier_NetworkAddress = 7272;

        /// <summary>
        /// The identifier for the CtrlResourceType_Tasks Object.
        /// </summary>
        public const uint CtrlResourceType_Tasks = 5014;

        /// <summary>
        /// The identifier for the CtrlResourceType_Tasks_SupportedTypes Object.
        /// </summary>
        public const uint CtrlResourceType_Tasks_SupportedTypes = 5015;

        /// <summary>
        /// The identifier for the CtrlResourceType_Programs Object.
        /// </summary>
        public const uint CtrlResourceType_Programs = 5016;

        /// <summary>
        /// The identifier for the CtrlResourceType_Programs_SupportedTypes Object.
        /// </summary>
        public const uint CtrlResourceType_Programs_SupportedTypes = 5017;

        /// <summary>
        /// The identifier for the CtrlResourceType_GlobalVars Object.
        /// </summary>
        public const uint CtrlResourceType_GlobalVars = 5018;

        /// <summary>
        /// The identifier for the CtrlResourceType_Configuration Object.
        /// </summary>
        public const uint CtrlResourceType_Configuration = 5019;

        /// <summary>
        /// The identifier for the CtrlResourceType_Diagnostic Object.
        /// </summary>
        public const uint CtrlResourceType_Diagnostic = 5020;
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
        /// The identifier for the CtrlConfigurationType ObjectType.
        /// </summary>
        public const uint CtrlConfigurationType = 1001;

        /// <summary>
        /// The identifier for the CtrlResourceType ObjectType.
        /// </summary>
        public const uint CtrlResourceType = 1002;

        /// <summary>
        /// The identifier for the CtrlProgramOrganizationUnitType ObjectType.
        /// </summary>
        public const uint CtrlProgramOrganizationUnitType = 1003;

        /// <summary>
        /// The identifier for the CtrlProgramType ObjectType.
        /// </summary>
        public const uint CtrlProgramType = 1004;

        /// <summary>
        /// The identifier for the CtrlFunctionBlockType ObjectType.
        /// </summary>
        public const uint CtrlFunctionBlockType = 1005;

        /// <summary>
        /// The identifier for the CtrlTaskType ObjectType.
        /// </summary>
        public const uint CtrlTaskType = 1006;

        /// <summary>
        /// The identifier for the SFCType ObjectType.
        /// </summary>
        public const uint SFCType = 1007;
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
        /// The identifier for the HasInputVars ReferenceType.
        /// </summary>
        public const uint HasInputVars = 4001;

        /// <summary>
        /// The identifier for the HasOutputVars ReferenceType.
        /// </summary>
        public const uint HasOutputVars = 4002;

        /// <summary>
        /// The identifier for the HasInOutVars ReferenceType.
        /// </summary>
        public const uint HasInOutVars = 4003;

        /// <summary>
        /// The identifier for the HasLocalVars ReferenceType.
        /// </summary>
        public const uint HasLocalVars = 4004;

        /// <summary>
        /// The identifier for the HasExternalVars ReferenceType.
        /// </summary>
        public const uint HasExternalVars = 4005;

        /// <summary>
        /// The identifier for the With ReferenceType.
        /// </summary>
        public const uint With = 4006;
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
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceUri Variable.
        /// </summary>
        public const uint OpcUaPlcNamespaceMetadata_NamespaceUri = 15002;

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceVersion Variable.
        /// </summary>
        public const uint OpcUaPlcNamespaceMetadata_NamespaceVersion = 15003;

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespacePublicationDate Variable.
        /// </summary>
        public const uint OpcUaPlcNamespaceMetadata_NamespacePublicationDate = 15004;

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_IsNamespaceSubset Variable.
        /// </summary>
        public const uint OpcUaPlcNamespaceMetadata_IsNamespaceSubset = 15005;

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_StaticNodeIdTypes Variable.
        /// </summary>
        public const uint OpcUaPlcNamespaceMetadata_StaticNodeIdTypes = 15006;

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_StaticNumericNodeIdRange Variable.
        /// </summary>
        public const uint OpcUaPlcNamespaceMetadata_StaticNumericNodeIdRange = 15007;

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_StaticStringNodeIdPattern Variable.
        /// </summary>
        public const uint OpcUaPlcNamespaceMetadata_StaticStringNodeIdPattern = 15008;

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_Size Variable.
        /// </summary>
        public const uint OpcUaPlcNamespaceMetadata_NamespaceFile_Size = 15010;

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_Writable Variable.
        /// </summary>
        public const uint OpcUaPlcNamespaceMetadata_NamespaceFile_Writable = 15011;

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_UserWritable Variable.
        /// </summary>
        public const uint OpcUaPlcNamespaceMetadata_NamespaceFile_UserWritable = 15012;

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_OpenCount Variable.
        /// </summary>
        public const uint OpcUaPlcNamespaceMetadata_NamespaceFile_OpenCount = 15013;

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_Open_InputArguments Variable.
        /// </summary>
        public const uint OpcUaPlcNamespaceMetadata_NamespaceFile_Open_InputArguments = 15016;

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_Open_OutputArguments Variable.
        /// </summary>
        public const uint OpcUaPlcNamespaceMetadata_NamespaceFile_Open_OutputArguments = 15017;

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_Close_InputArguments Variable.
        /// </summary>
        public const uint OpcUaPlcNamespaceMetadata_NamespaceFile_Close_InputArguments = 15019;

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_Read_InputArguments Variable.
        /// </summary>
        public const uint OpcUaPlcNamespaceMetadata_NamespaceFile_Read_InputArguments = 15021;

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_Read_OutputArguments Variable.
        /// </summary>
        public const uint OpcUaPlcNamespaceMetadata_NamespaceFile_Read_OutputArguments = 15022;

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_Write_InputArguments Variable.
        /// </summary>
        public const uint OpcUaPlcNamespaceMetadata_NamespaceFile_Write_InputArguments = 15024;

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_GetPosition_InputArguments Variable.
        /// </summary>
        public const uint OpcUaPlcNamespaceMetadata_NamespaceFile_GetPosition_InputArguments = 15026;

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_GetPosition_OutputArguments Variable.
        /// </summary>
        public const uint OpcUaPlcNamespaceMetadata_NamespaceFile_GetPosition_OutputArguments = 15027;

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_SetPosition_InputArguments Variable.
        /// </summary>
        public const uint OpcUaPlcNamespaceMetadata_NamespaceFile_SetPosition_InputArguments = 15029;

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_DefaultRolePermissions Variable.
        /// </summary>
        public const uint OpcUaPlcNamespaceMetadata_DefaultRolePermissions = 15031;

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_DefaultUserRolePermissions Variable.
        /// </summary>
        public const uint OpcUaPlcNamespaceMetadata_DefaultUserRolePermissions = 15032;

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_DefaultAccessRestrictions Variable.
        /// </summary>
        public const uint OpcUaPlcNamespaceMetadata_DefaultAccessRestrictions = 15033;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Lock_Locked Variable.
        /// </summary>
        public const uint CtrlConfigurationType_Lock_Locked = 7171;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Lock_LockingClient Variable.
        /// </summary>
        public const uint CtrlConfigurationType_Lock_LockingClient = 7025;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Lock_LockingUser Variable.
        /// </summary>
        public const uint CtrlConfigurationType_Lock_LockingUser = 7026;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint CtrlConfigurationType_Lock_RemainingLockTime = 7027;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint CtrlConfigurationType_Lock_InitLock_InputArguments = 7029;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint CtrlConfigurationType_Lock_InitLock_OutputArguments = 7030;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint CtrlConfigurationType_Lock_RenewLock_OutputArguments = 7032;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint CtrlConfigurationType_Lock_ExitLock_OutputArguments = 7034;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint CtrlConfigurationType_Lock_BreakLock_OutputArguments = 7036;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint CtrlConfigurationType_CPIdentifier_Lock_Locked = 7234;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint CtrlConfigurationType_CPIdentifier_Lock_LockingClient = 7235;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint CtrlConfigurationType_CPIdentifier_Lock_LockingUser = 7236;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint CtrlConfigurationType_CPIdentifier_Lock_RemainingLockTime = 7237;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint CtrlConfigurationType_CPIdentifier_Lock_InitLock_InputArguments = 7239;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint CtrlConfigurationType_CPIdentifier_Lock_InitLock_OutputArguments = 7240;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint CtrlConfigurationType_CPIdentifier_Lock_RenewLock_OutputArguments = 7242;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint CtrlConfigurationType_CPIdentifier_Lock_ExitLock_OutputArguments = 7244;

        /// <summary>
        /// The identifier for the CtrlConfigurationType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint CtrlConfigurationType_CPIdentifier_Lock_BreakLock_OutputArguments = 7246;

        /// <summary>
        /// The identifier for the CtrlResourceType_Lock_Locked Variable.
        /// </summary>
        public const uint CtrlResourceType_Lock_Locked = 7196;

        /// <summary>
        /// The identifier for the CtrlResourceType_Lock_LockingClient Variable.
        /// </summary>
        public const uint CtrlResourceType_Lock_LockingClient = 7064;

        /// <summary>
        /// The identifier for the CtrlResourceType_Lock_LockingUser Variable.
        /// </summary>
        public const uint CtrlResourceType_Lock_LockingUser = 7065;

        /// <summary>
        /// The identifier for the CtrlResourceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint CtrlResourceType_Lock_RemainingLockTime = 7066;

        /// <summary>
        /// The identifier for the CtrlResourceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint CtrlResourceType_Lock_InitLock_InputArguments = 7068;

        /// <summary>
        /// The identifier for the CtrlResourceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint CtrlResourceType_Lock_InitLock_OutputArguments = 7069;

        /// <summary>
        /// The identifier for the CtrlResourceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint CtrlResourceType_Lock_RenewLock_OutputArguments = 7071;

        /// <summary>
        /// The identifier for the CtrlResourceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint CtrlResourceType_Lock_ExitLock_OutputArguments = 7073;

        /// <summary>
        /// The identifier for the CtrlResourceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint CtrlResourceType_Lock_BreakLock_OutputArguments = 7075;

        /// <summary>
        /// The identifier for the CtrlResourceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint CtrlResourceType_CPIdentifier_Lock_Locked = 7259;

        /// <summary>
        /// The identifier for the CtrlResourceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint CtrlResourceType_CPIdentifier_Lock_LockingClient = 7260;

        /// <summary>
        /// The identifier for the CtrlResourceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint CtrlResourceType_CPIdentifier_Lock_LockingUser = 7261;

        /// <summary>
        /// The identifier for the CtrlResourceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint CtrlResourceType_CPIdentifier_Lock_RemainingLockTime = 7262;

        /// <summary>
        /// The identifier for the CtrlResourceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint CtrlResourceType_CPIdentifier_Lock_InitLock_InputArguments = 7264;

        /// <summary>
        /// The identifier for the CtrlResourceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint CtrlResourceType_CPIdentifier_Lock_InitLock_OutputArguments = 7265;

        /// <summary>
        /// The identifier for the CtrlResourceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint CtrlResourceType_CPIdentifier_Lock_RenewLock_OutputArguments = 7267;

        /// <summary>
        /// The identifier for the CtrlResourceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint CtrlResourceType_CPIdentifier_Lock_ExitLock_OutputArguments = 7269;

        /// <summary>
        /// The identifier for the CtrlResourceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint CtrlResourceType_CPIdentifier_Lock_BreakLock_OutputArguments = 7271;

        /// <summary>
        /// The identifier for the CtrlProgramOrganizationUnitType_Lock_Locked Variable.
        /// </summary>
        public const uint CtrlProgramOrganizationUnitType_Lock_Locked = 7221;

        /// <summary>
        /// The identifier for the CtrlProgramOrganizationUnitType_Lock_LockingClient Variable.
        /// </summary>
        public const uint CtrlProgramOrganizationUnitType_Lock_LockingClient = 7101;

        /// <summary>
        /// The identifier for the CtrlProgramOrganizationUnitType_Lock_LockingUser Variable.
        /// </summary>
        public const uint CtrlProgramOrganizationUnitType_Lock_LockingUser = 7102;

        /// <summary>
        /// The identifier for the CtrlProgramOrganizationUnitType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint CtrlProgramOrganizationUnitType_Lock_RemainingLockTime = 7103;

        /// <summary>
        /// The identifier for the CtrlProgramOrganizationUnitType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint CtrlProgramOrganizationUnitType_Lock_InitLock_InputArguments = 7105;

        /// <summary>
        /// The identifier for the CtrlProgramOrganizationUnitType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint CtrlProgramOrganizationUnitType_Lock_InitLock_OutputArguments = 7106;

        /// <summary>
        /// The identifier for the CtrlProgramOrganizationUnitType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint CtrlProgramOrganizationUnitType_Lock_RenewLock_OutputArguments = 7108;

        /// <summary>
        /// The identifier for the CtrlProgramOrganizationUnitType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint CtrlProgramOrganizationUnitType_Lock_ExitLock_OutputArguments = 7110;

        /// <summary>
        /// The identifier for the CtrlProgramOrganizationUnitType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint CtrlProgramOrganizationUnitType_Lock_BreakLock_OutputArguments = 7112;

        /// <summary>
        /// The identifier for the CtrlProgramOrganizationUnitType_Body Variable.
        /// </summary>
        public const uint CtrlProgramOrganizationUnitType_Body = 6001;

        /// <summary>
        /// The identifier for the CtrlProgramType_Lock_Locked Variable.
        /// </summary>
        public const uint CtrlProgramType_Lock_Locked = 7222;

        /// <summary>
        /// The identifier for the CtrlProgramType_Lock_LockingClient Variable.
        /// </summary>
        public const uint CtrlProgramType_Lock_LockingClient = 7126;

        /// <summary>
        /// The identifier for the CtrlProgramType_Lock_LockingUser Variable.
        /// </summary>
        public const uint CtrlProgramType_Lock_LockingUser = 7127;

        /// <summary>
        /// The identifier for the CtrlProgramType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint CtrlProgramType_Lock_RemainingLockTime = 7128;

        /// <summary>
        /// The identifier for the CtrlProgramType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint CtrlProgramType_Lock_InitLock_InputArguments = 7130;

        /// <summary>
        /// The identifier for the CtrlProgramType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint CtrlProgramType_Lock_InitLock_OutputArguments = 7131;

        /// <summary>
        /// The identifier for the CtrlProgramType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint CtrlProgramType_Lock_RenewLock_OutputArguments = 7133;

        /// <summary>
        /// The identifier for the CtrlProgramType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint CtrlProgramType_Lock_ExitLock_OutputArguments = 7135;

        /// <summary>
        /// The identifier for the CtrlProgramType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint CtrlProgramType_Lock_BreakLock_OutputArguments = 7137;

        /// <summary>
        /// The identifier for the CtrlProgramType_Program Variable.
        /// </summary>
        public const uint CtrlProgramType_Program = 6002;

        /// <summary>
        /// The identifier for the CtrlFunctionBlockType_Lock_Locked Variable.
        /// </summary>
        public const uint CtrlFunctionBlockType_Lock_Locked = 7223;

        /// <summary>
        /// The identifier for the CtrlFunctionBlockType_Lock_LockingClient Variable.
        /// </summary>
        public const uint CtrlFunctionBlockType_Lock_LockingClient = 7151;

        /// <summary>
        /// The identifier for the CtrlFunctionBlockType_Lock_LockingUser Variable.
        /// </summary>
        public const uint CtrlFunctionBlockType_Lock_LockingUser = 7152;

        /// <summary>
        /// The identifier for the CtrlFunctionBlockType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint CtrlFunctionBlockType_Lock_RemainingLockTime = 7153;

        /// <summary>
        /// The identifier for the CtrlFunctionBlockType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint CtrlFunctionBlockType_Lock_InitLock_InputArguments = 7155;

        /// <summary>
        /// The identifier for the CtrlFunctionBlockType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint CtrlFunctionBlockType_Lock_InitLock_OutputArguments = 7156;

        /// <summary>
        /// The identifier for the CtrlFunctionBlockType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint CtrlFunctionBlockType_Lock_RenewLock_OutputArguments = 7158;

        /// <summary>
        /// The identifier for the CtrlFunctionBlockType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint CtrlFunctionBlockType_Lock_ExitLock_OutputArguments = 7160;

        /// <summary>
        /// The identifier for the CtrlFunctionBlockType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint CtrlFunctionBlockType_Lock_BreakLock_OutputArguments = 7162;

        /// <summary>
        /// The identifier for the CtrlFunctionBlockType_FunctionBlock Variable.
        /// </summary>
        public const uint CtrlFunctionBlockType_FunctionBlock = 6003;

        /// <summary>
        /// The identifier for the CtrlTaskType_Priority Variable.
        /// </summary>
        public const uint CtrlTaskType_Priority = 6004;

        /// <summary>
        /// The identifier for the CtrlTaskType_Interval Variable.
        /// </summary>
        public const uint CtrlTaskType_Interval = 6005;

        /// <summary>
        /// The identifier for the CtrlTaskType_Single Variable.
        /// </summary>
        public const uint CtrlTaskType_Single = 6006;
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
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_Open Method.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaPlcNamespaceMetadata_NamespaceFile_Open = new ExpandedNodeId(Opc.Ua.Plc.Methods.OpcUaPlcNamespaceMetadata_NamespaceFile_Open, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_Close Method.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaPlcNamespaceMetadata_NamespaceFile_Close = new ExpandedNodeId(Opc.Ua.Plc.Methods.OpcUaPlcNamespaceMetadata_NamespaceFile_Close, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_Read Method.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaPlcNamespaceMetadata_NamespaceFile_Read = new ExpandedNodeId(Opc.Ua.Plc.Methods.OpcUaPlcNamespaceMetadata_NamespaceFile_Read, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_Write Method.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaPlcNamespaceMetadata_NamespaceFile_Write = new ExpandedNodeId(Opc.Ua.Plc.Methods.OpcUaPlcNamespaceMetadata_NamespaceFile_Write, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_GetPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaPlcNamespaceMetadata_NamespaceFile_GetPosition = new ExpandedNodeId(Opc.Ua.Plc.Methods.OpcUaPlcNamespaceMetadata_NamespaceFile_GetPosition, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_SetPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaPlcNamespaceMetadata_NamespaceFile_SetPosition = new ExpandedNodeId(Opc.Ua.Plc.Methods.OpcUaPlcNamespaceMetadata_NamespaceFile_SetPosition, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlConfigurationType_Lock_InitLock, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlConfigurationType_Lock_RenewLock, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlConfigurationType_Lock_ExitLock, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlConfigurationType_Lock_BreakLock, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_CPIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlConfigurationType_CPIdentifier_Lock_InitLock, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_CPIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlConfigurationType_CPIdentifier_Lock_RenewLock, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_CPIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlConfigurationType_CPIdentifier_Lock_ExitLock, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_CPIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlConfigurationType_CPIdentifier_Lock_BreakLock, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_MethodSet_Start Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_MethodSet_Start = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlConfigurationType_MethodSet_Start, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_MethodSet_Stop Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_MethodSet_Stop = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlConfigurationType_MethodSet_Stop, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlResourceType_Lock_InitLock, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlResourceType_Lock_RenewLock, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlResourceType_Lock_ExitLock, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlResourceType_Lock_BreakLock, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_CPIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlResourceType_CPIdentifier_Lock_InitLock, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_CPIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlResourceType_CPIdentifier_Lock_RenewLock, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_CPIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlResourceType_CPIdentifier_Lock_ExitLock, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_CPIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlResourceType_CPIdentifier_Lock_BreakLock, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_MethodSet_Start Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_MethodSet_Start = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlResourceType_MethodSet_Start, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_MethodSet_Stop Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_MethodSet_Stop = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlResourceType_MethodSet_Stop, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlProgramOrganizationUnitType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlProgramOrganizationUnitType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlProgramOrganizationUnitType_Lock_InitLock, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlProgramOrganizationUnitType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlProgramOrganizationUnitType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlProgramOrganizationUnitType_Lock_RenewLock, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlProgramOrganizationUnitType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlProgramOrganizationUnitType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlProgramOrganizationUnitType_Lock_ExitLock, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlProgramOrganizationUnitType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlProgramOrganizationUnitType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlProgramOrganizationUnitType_Lock_BreakLock, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlProgramType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlProgramType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlProgramType_Lock_InitLock, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlProgramType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlProgramType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlProgramType_Lock_RenewLock, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlProgramType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlProgramType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlProgramType_Lock_ExitLock, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlProgramType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlProgramType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlProgramType_Lock_BreakLock, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlFunctionBlockType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlFunctionBlockType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlFunctionBlockType_Lock_InitLock, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlFunctionBlockType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlFunctionBlockType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlFunctionBlockType_Lock_RenewLock, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlFunctionBlockType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlFunctionBlockType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlFunctionBlockType_Lock_ExitLock, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlFunctionBlockType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CtrlFunctionBlockType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Plc.Methods.CtrlFunctionBlockType_Lock_BreakLock, Opc.Ua.Plc.Namespaces.OpcUaPlc);
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
        /// The identifier for the OpcUaPlcNamespaceMetadata Object.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaPlcNamespaceMetadata = new ExpandedNodeId(Opc.Ua.Plc.Objects.OpcUaPlcNamespaceMetadata, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_MethodSet Object.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_MethodSet = new ExpandedNodeId(Opc.Ua.Plc.Objects.CtrlConfigurationType_MethodSet, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_CPIdentifier_NetworkAddress = new ExpandedNodeId(Opc.Ua.Plc.Objects.CtrlConfigurationType_CPIdentifier_NetworkAddress, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Resources Object.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_Resources = new ExpandedNodeId(Opc.Ua.Plc.Objects.CtrlConfigurationType_Resources, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Resources_SupportedTypes Object.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_Resources_SupportedTypes = new ExpandedNodeId(Opc.Ua.Plc.Objects.CtrlConfigurationType_Resources_SupportedTypes, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_GlobalVars Object.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_GlobalVars = new ExpandedNodeId(Opc.Ua.Plc.Objects.CtrlConfigurationType_GlobalVars, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_AccessVars Object.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_AccessVars = new ExpandedNodeId(Opc.Ua.Plc.Objects.CtrlConfigurationType_AccessVars, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_ConfigVars Object.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_ConfigVars = new ExpandedNodeId(Opc.Ua.Plc.Objects.CtrlConfigurationType_ConfigVars, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Configuration Object.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_Configuration = new ExpandedNodeId(Opc.Ua.Plc.Objects.CtrlConfigurationType_Configuration, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Diagnostic Object.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_Diagnostic = new ExpandedNodeId(Opc.Ua.Plc.Objects.CtrlConfigurationType_Diagnostic, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_MethodSet Object.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_MethodSet = new ExpandedNodeId(Opc.Ua.Plc.Objects.CtrlResourceType_MethodSet, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_CPIdentifier_NetworkAddress = new ExpandedNodeId(Opc.Ua.Plc.Objects.CtrlResourceType_CPIdentifier_NetworkAddress, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_Tasks Object.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_Tasks = new ExpandedNodeId(Opc.Ua.Plc.Objects.CtrlResourceType_Tasks, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_Tasks_SupportedTypes Object.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_Tasks_SupportedTypes = new ExpandedNodeId(Opc.Ua.Plc.Objects.CtrlResourceType_Tasks_SupportedTypes, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_Programs Object.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_Programs = new ExpandedNodeId(Opc.Ua.Plc.Objects.CtrlResourceType_Programs, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_Programs_SupportedTypes Object.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_Programs_SupportedTypes = new ExpandedNodeId(Opc.Ua.Plc.Objects.CtrlResourceType_Programs_SupportedTypes, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_GlobalVars Object.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_GlobalVars = new ExpandedNodeId(Opc.Ua.Plc.Objects.CtrlResourceType_GlobalVars, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_Configuration Object.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_Configuration = new ExpandedNodeId(Opc.Ua.Plc.Objects.CtrlResourceType_Configuration, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_Diagnostic Object.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_Diagnostic = new ExpandedNodeId(Opc.Ua.Plc.Objects.CtrlResourceType_Diagnostic, Opc.Ua.Plc.Namespaces.OpcUaPlc);
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
        /// The identifier for the CtrlConfigurationType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType = new ExpandedNodeId(Opc.Ua.Plc.ObjectTypes.CtrlConfigurationType, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType = new ExpandedNodeId(Opc.Ua.Plc.ObjectTypes.CtrlResourceType, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlProgramOrganizationUnitType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId CtrlProgramOrganizationUnitType = new ExpandedNodeId(Opc.Ua.Plc.ObjectTypes.CtrlProgramOrganizationUnitType, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlProgramType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId CtrlProgramType = new ExpandedNodeId(Opc.Ua.Plc.ObjectTypes.CtrlProgramType, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlFunctionBlockType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId CtrlFunctionBlockType = new ExpandedNodeId(Opc.Ua.Plc.ObjectTypes.CtrlFunctionBlockType, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlTaskType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId CtrlTaskType = new ExpandedNodeId(Opc.Ua.Plc.ObjectTypes.CtrlTaskType, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the SFCType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId SFCType = new ExpandedNodeId(Opc.Ua.Plc.ObjectTypes.SFCType, Opc.Ua.Plc.Namespaces.OpcUaPlc);
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
        /// The identifier for the HasInputVars ReferenceType.
        /// </summary>
        public static readonly ExpandedNodeId HasInputVars = new ExpandedNodeId(Opc.Ua.Plc.ReferenceTypes.HasInputVars, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the HasOutputVars ReferenceType.
        /// </summary>
        public static readonly ExpandedNodeId HasOutputVars = new ExpandedNodeId(Opc.Ua.Plc.ReferenceTypes.HasOutputVars, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the HasInOutVars ReferenceType.
        /// </summary>
        public static readonly ExpandedNodeId HasInOutVars = new ExpandedNodeId(Opc.Ua.Plc.ReferenceTypes.HasInOutVars, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the HasLocalVars ReferenceType.
        /// </summary>
        public static readonly ExpandedNodeId HasLocalVars = new ExpandedNodeId(Opc.Ua.Plc.ReferenceTypes.HasLocalVars, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the HasExternalVars ReferenceType.
        /// </summary>
        public static readonly ExpandedNodeId HasExternalVars = new ExpandedNodeId(Opc.Ua.Plc.ReferenceTypes.HasExternalVars, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the With ReferenceType.
        /// </summary>
        public static readonly ExpandedNodeId With = new ExpandedNodeId(Opc.Ua.Plc.ReferenceTypes.With, Opc.Ua.Plc.Namespaces.OpcUaPlc);
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
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaPlcNamespaceMetadata_NamespaceUri = new ExpandedNodeId(Opc.Ua.Plc.Variables.OpcUaPlcNamespaceMetadata_NamespaceUri, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceVersion Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaPlcNamespaceMetadata_NamespaceVersion = new ExpandedNodeId(Opc.Ua.Plc.Variables.OpcUaPlcNamespaceMetadata_NamespaceVersion, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespacePublicationDate Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaPlcNamespaceMetadata_NamespacePublicationDate = new ExpandedNodeId(Opc.Ua.Plc.Variables.OpcUaPlcNamespaceMetadata_NamespacePublicationDate, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_IsNamespaceSubset Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaPlcNamespaceMetadata_IsNamespaceSubset = new ExpandedNodeId(Opc.Ua.Plc.Variables.OpcUaPlcNamespaceMetadata_IsNamespaceSubset, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_StaticNodeIdTypes Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaPlcNamespaceMetadata_StaticNodeIdTypes = new ExpandedNodeId(Opc.Ua.Plc.Variables.OpcUaPlcNamespaceMetadata_StaticNodeIdTypes, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_StaticNumericNodeIdRange Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaPlcNamespaceMetadata_StaticNumericNodeIdRange = new ExpandedNodeId(Opc.Ua.Plc.Variables.OpcUaPlcNamespaceMetadata_StaticNumericNodeIdRange, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_StaticStringNodeIdPattern Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaPlcNamespaceMetadata_StaticStringNodeIdPattern = new ExpandedNodeId(Opc.Ua.Plc.Variables.OpcUaPlcNamespaceMetadata_StaticStringNodeIdPattern, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_Size Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaPlcNamespaceMetadata_NamespaceFile_Size = new ExpandedNodeId(Opc.Ua.Plc.Variables.OpcUaPlcNamespaceMetadata_NamespaceFile_Size, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_Writable Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaPlcNamespaceMetadata_NamespaceFile_Writable = new ExpandedNodeId(Opc.Ua.Plc.Variables.OpcUaPlcNamespaceMetadata_NamespaceFile_Writable, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_UserWritable Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaPlcNamespaceMetadata_NamespaceFile_UserWritable = new ExpandedNodeId(Opc.Ua.Plc.Variables.OpcUaPlcNamespaceMetadata_NamespaceFile_UserWritable, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_OpenCount Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaPlcNamespaceMetadata_NamespaceFile_OpenCount = new ExpandedNodeId(Opc.Ua.Plc.Variables.OpcUaPlcNamespaceMetadata_NamespaceFile_OpenCount, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_Open_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaPlcNamespaceMetadata_NamespaceFile_Open_InputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.OpcUaPlcNamespaceMetadata_NamespaceFile_Open_InputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_Open_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaPlcNamespaceMetadata_NamespaceFile_Open_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.OpcUaPlcNamespaceMetadata_NamespaceFile_Open_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_Close_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaPlcNamespaceMetadata_NamespaceFile_Close_InputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.OpcUaPlcNamespaceMetadata_NamespaceFile_Close_InputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_Read_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaPlcNamespaceMetadata_NamespaceFile_Read_InputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.OpcUaPlcNamespaceMetadata_NamespaceFile_Read_InputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_Read_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaPlcNamespaceMetadata_NamespaceFile_Read_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.OpcUaPlcNamespaceMetadata_NamespaceFile_Read_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_Write_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaPlcNamespaceMetadata_NamespaceFile_Write_InputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.OpcUaPlcNamespaceMetadata_NamespaceFile_Write_InputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_GetPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaPlcNamespaceMetadata_NamespaceFile_GetPosition_InputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.OpcUaPlcNamespaceMetadata_NamespaceFile_GetPosition_InputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_GetPosition_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaPlcNamespaceMetadata_NamespaceFile_GetPosition_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.OpcUaPlcNamespaceMetadata_NamespaceFile_GetPosition_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_NamespaceFile_SetPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaPlcNamespaceMetadata_NamespaceFile_SetPosition_InputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.OpcUaPlcNamespaceMetadata_NamespaceFile_SetPosition_InputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_DefaultRolePermissions Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaPlcNamespaceMetadata_DefaultRolePermissions = new ExpandedNodeId(Opc.Ua.Plc.Variables.OpcUaPlcNamespaceMetadata_DefaultRolePermissions, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_DefaultUserRolePermissions Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaPlcNamespaceMetadata_DefaultUserRolePermissions = new ExpandedNodeId(Opc.Ua.Plc.Variables.OpcUaPlcNamespaceMetadata_DefaultUserRolePermissions, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the OpcUaPlcNamespaceMetadata_DefaultAccessRestrictions Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaPlcNamespaceMetadata_DefaultAccessRestrictions = new ExpandedNodeId(Opc.Ua.Plc.Variables.OpcUaPlcNamespaceMetadata_DefaultAccessRestrictions, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlConfigurationType_Lock_Locked, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlConfigurationType_Lock_LockingClient, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlConfigurationType_Lock_LockingUser, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlConfigurationType_Lock_RemainingLockTime, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlConfigurationType_Lock_InitLock_InputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlConfigurationType_Lock_InitLock_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlConfigurationType_Lock_RenewLock_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlConfigurationType_Lock_ExitLock_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlConfigurationType_Lock_BreakLock_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_CPIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlConfigurationType_CPIdentifier_Lock_Locked, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_CPIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlConfigurationType_CPIdentifier_Lock_LockingClient, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_CPIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlConfigurationType_CPIdentifier_Lock_LockingUser, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_CPIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlConfigurationType_CPIdentifier_Lock_RemainingLockTime, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_CPIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlConfigurationType_CPIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_CPIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlConfigurationType_CPIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_CPIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlConfigurationType_CPIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_CPIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlConfigurationType_CPIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlConfigurationType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlConfigurationType_CPIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlConfigurationType_CPIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlResourceType_Lock_Locked, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlResourceType_Lock_LockingClient, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlResourceType_Lock_LockingUser, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlResourceType_Lock_RemainingLockTime, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlResourceType_Lock_InitLock_InputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlResourceType_Lock_InitLock_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlResourceType_Lock_RenewLock_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlResourceType_Lock_ExitLock_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlResourceType_Lock_BreakLock_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_CPIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlResourceType_CPIdentifier_Lock_Locked, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_CPIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlResourceType_CPIdentifier_Lock_LockingClient, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_CPIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlResourceType_CPIdentifier_Lock_LockingUser, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_CPIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlResourceType_CPIdentifier_Lock_RemainingLockTime, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_CPIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlResourceType_CPIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_CPIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlResourceType_CPIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_CPIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlResourceType_CPIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_CPIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlResourceType_CPIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlResourceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlResourceType_CPIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlResourceType_CPIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlProgramOrganizationUnitType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlProgramOrganizationUnitType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlProgramOrganizationUnitType_Lock_Locked, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlProgramOrganizationUnitType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlProgramOrganizationUnitType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlProgramOrganizationUnitType_Lock_LockingClient, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlProgramOrganizationUnitType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlProgramOrganizationUnitType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlProgramOrganizationUnitType_Lock_LockingUser, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlProgramOrganizationUnitType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlProgramOrganizationUnitType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlProgramOrganizationUnitType_Lock_RemainingLockTime, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlProgramOrganizationUnitType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlProgramOrganizationUnitType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlProgramOrganizationUnitType_Lock_InitLock_InputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlProgramOrganizationUnitType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlProgramOrganizationUnitType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlProgramOrganizationUnitType_Lock_InitLock_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlProgramOrganizationUnitType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlProgramOrganizationUnitType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlProgramOrganizationUnitType_Lock_RenewLock_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlProgramOrganizationUnitType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlProgramOrganizationUnitType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlProgramOrganizationUnitType_Lock_ExitLock_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlProgramOrganizationUnitType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlProgramOrganizationUnitType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlProgramOrganizationUnitType_Lock_BreakLock_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlProgramOrganizationUnitType_Body Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlProgramOrganizationUnitType_Body = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlProgramOrganizationUnitType_Body, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlProgramType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlProgramType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlProgramType_Lock_Locked, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlProgramType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlProgramType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlProgramType_Lock_LockingClient, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlProgramType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlProgramType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlProgramType_Lock_LockingUser, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlProgramType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlProgramType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlProgramType_Lock_RemainingLockTime, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlProgramType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlProgramType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlProgramType_Lock_InitLock_InputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlProgramType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlProgramType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlProgramType_Lock_InitLock_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlProgramType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlProgramType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlProgramType_Lock_RenewLock_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlProgramType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlProgramType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlProgramType_Lock_ExitLock_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlProgramType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlProgramType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlProgramType_Lock_BreakLock_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlProgramType_Program Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlProgramType_Program = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlProgramType_Program, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlFunctionBlockType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlFunctionBlockType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlFunctionBlockType_Lock_Locked, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlFunctionBlockType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlFunctionBlockType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlFunctionBlockType_Lock_LockingClient, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlFunctionBlockType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlFunctionBlockType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlFunctionBlockType_Lock_LockingUser, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlFunctionBlockType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlFunctionBlockType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlFunctionBlockType_Lock_RemainingLockTime, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlFunctionBlockType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlFunctionBlockType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlFunctionBlockType_Lock_InitLock_InputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlFunctionBlockType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlFunctionBlockType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlFunctionBlockType_Lock_InitLock_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlFunctionBlockType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlFunctionBlockType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlFunctionBlockType_Lock_RenewLock_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlFunctionBlockType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlFunctionBlockType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlFunctionBlockType_Lock_ExitLock_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlFunctionBlockType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlFunctionBlockType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlFunctionBlockType_Lock_BreakLock_OutputArguments, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlFunctionBlockType_FunctionBlock Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlFunctionBlockType_FunctionBlock = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlFunctionBlockType_FunctionBlock, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlTaskType_Priority Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlTaskType_Priority = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlTaskType_Priority, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlTaskType_Interval Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlTaskType_Interval = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlTaskType_Interval, Opc.Ua.Plc.Namespaces.OpcUaPlc);

        /// <summary>
        /// The identifier for the CtrlTaskType_Single Variable.
        /// </summary>
        public static readonly ExpandedNodeId CtrlTaskType_Single = new ExpandedNodeId(Opc.Ua.Plc.Variables.CtrlTaskType_Single, Opc.Ua.Plc.Namespaces.OpcUaPlc);
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
        /// The BrowseName for the AccessVars component.
        /// </summary>
        public const string AccessVars = "AccessVars";

        /// <summary>
        /// The BrowseName for the Body component.
        /// </summary>
        public const string Body = "Body";

        /// <summary>
        /// The BrowseName for the Configuration component.
        /// </summary>
        public const string Configuration = "Configuration";

        /// <summary>
        /// The BrowseName for the ConfigVars component.
        /// </summary>
        public const string ConfigVars = "ConfigVars";

        /// <summary>
        /// The BrowseName for the CtrlConfigurationType component.
        /// </summary>
        public const string CtrlConfigurationType = "CtrlConfigurationType";

        /// <summary>
        /// The BrowseName for the CtrlFunctionBlockType component.
        /// </summary>
        public const string CtrlFunctionBlockType = "CtrlFunctionBlockType";

        /// <summary>
        /// The BrowseName for the CtrlProgramOrganizationUnitType component.
        /// </summary>
        public const string CtrlProgramOrganizationUnitType = "CtrlProgramOrganizationUnitType";

        /// <summary>
        /// The BrowseName for the CtrlProgramType component.
        /// </summary>
        public const string CtrlProgramType = "CtrlProgramType";

        /// <summary>
        /// The BrowseName for the CtrlResourceType component.
        /// </summary>
        public const string CtrlResourceType = "CtrlResourceType";

        /// <summary>
        /// The BrowseName for the CtrlTaskType component.
        /// </summary>
        public const string CtrlTaskType = "CtrlTaskType";

        /// <summary>
        /// The BrowseName for the Diagnostic component.
        /// </summary>
        public const string Diagnostic = "Diagnostic";

        /// <summary>
        /// The BrowseName for the FunctionBlock component.
        /// </summary>
        public const string FunctionBlock = "FunctionBlock";

        /// <summary>
        /// The BrowseName for the GlobalVars component.
        /// </summary>
        public const string GlobalVars = "GlobalVars";

        /// <summary>
        /// The BrowseName for the HasExternalVars component.
        /// </summary>
        public const string HasExternalVars = "HasExternalVars";

        /// <summary>
        /// The BrowseName for the HasInOutVars component.
        /// </summary>
        public const string HasInOutVars = "HasInOutVars";

        /// <summary>
        /// The BrowseName for the HasInputVars component.
        /// </summary>
        public const string HasInputVars = "HasInputVars";

        /// <summary>
        /// The BrowseName for the HasLocalVars component.
        /// </summary>
        public const string HasLocalVars = "HasLocalVars";

        /// <summary>
        /// The BrowseName for the HasOutputVars component.
        /// </summary>
        public const string HasOutputVars = "HasOutputVars";

        /// <summary>
        /// The BrowseName for the Interval component.
        /// </summary>
        public const string Interval = "Interval";

        /// <summary>
        /// The BrowseName for the MethodSet component.
        /// </summary>
        public const string MethodSet = "MethodSet";

        /// <summary>
        /// The BrowseName for the OpcUaPlcNamespaceMetadata component.
        /// </summary>
        public const string OpcUaPlcNamespaceMetadata = "http://PLCopen.org/OpcUa/IEC61131-3/";

        /// <summary>
        /// The BrowseName for the Priority component.
        /// </summary>
        public const string Priority = "Priority";

        /// <summary>
        /// The BrowseName for the Program component.
        /// </summary>
        public const string Program = "Program";

        /// <summary>
        /// The BrowseName for the Programs component.
        /// </summary>
        public const string Programs = "Programs";

        /// <summary>
        /// The BrowseName for the Resources component.
        /// </summary>
        public const string Resources = "Resources";

        /// <summary>
        /// The BrowseName for the SFCType component.
        /// </summary>
        public const string SFCType = "SFCType";

        /// <summary>
        /// The BrowseName for the Single component.
        /// </summary>
        public const string Single = "Single";

        /// <summary>
        /// The BrowseName for the Tasks component.
        /// </summary>
        public const string Tasks = "Tasks";

        /// <summary>
        /// The BrowseName for the With component.
        /// </summary>
        public const string With = "With";
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
        /// The URI for the OpcUa namespace (.NET code namespace is 'Opc.Ua').
        /// </summary>
        public const string OpcUa = "http://opcfoundation.org/UA/";

        /// <summary>
        /// The URI for the OpcUaXsd namespace (.NET code namespace is 'Opc.Ua').
        /// </summary>
        public const string OpcUaXsd = "http://opcfoundation.org/UA/2008/02/Types.xsd";

        /// <summary>
        /// The URI for the OpcUaDi namespace (.NET code namespace is 'Opc.Ua.Di').
        /// </summary>
        public const string OpcUaDi = "http://opcfoundation.org/UA/DI/";

        /// <summary>
        /// The URI for the OpcUaDiXsd namespace (.NET code namespace is 'Opc.Ua.Di').
        /// </summary>
        public const string OpcUaDiXsd = "http://opcfoundation.org/UA/DI/Types.xsd";

        /// <summary>
        /// The URI for the OpcUaPlc namespace (.NET code namespace is 'Opc.Ua.Plc').
        /// </summary>
        public const string OpcUaPlc = "http://PLCopen.org/OpcUa/IEC61131-3/";

        /// <summary>
        /// The URI for the OpcUaPlcXsd namespace (.NET code namespace is 'Opc.Ua.Plc').
        /// </summary>
        public const string OpcUaPlcXsd = "http://PLCopen.org/OpcUa/IEC61131-3/Types.xsd";
    }
    #endregion
}
