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
using System.Reflection;
using System.Xml;
using System.Runtime.Serialization;
using Opc.Ua.Di;
using Opc.Ua;

namespace Sercos
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
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_Open Method.
        /// </summary>
        public const uint SercosNamespaceMetadata_NamespaceFile_Open = 6095;

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_Close Method.
        /// </summary>
        public const uint SercosNamespaceMetadata_NamespaceFile_Close = 6098;

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_Read Method.
        /// </summary>
        public const uint SercosNamespaceMetadata_NamespaceFile_Read = 6100;

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_Write Method.
        /// </summary>
        public const uint SercosNamespaceMetadata_NamespaceFile_Write = 6103;

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_GetPosition Method.
        /// </summary>
        public const uint SercosNamespaceMetadata_NamespaceFile_GetPosition = 6105;

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_SetPosition Method.
        /// </summary>
        public const uint SercosNamespaceMetadata_NamespaceFile_SetPosition = 6108;

        /// <summary>
        /// The identifier for the SercosDeviceType_Lock_InitLock Method.
        /// </summary>
        public const uint SercosDeviceType_Lock_InitLock = 6025;

        /// <summary>
        /// The identifier for the SercosDeviceType_Lock_RenewLock Method.
        /// </summary>
        public const uint SercosDeviceType_Lock_RenewLock = 6028;

        /// <summary>
        /// The identifier for the SercosDeviceType_Lock_ExitLock Method.
        /// </summary>
        public const uint SercosDeviceType_Lock_ExitLock = 6030;

        /// <summary>
        /// The identifier for the SercosDeviceType_Lock_BreakLock Method.
        /// </summary>
        public const uint SercosDeviceType_Lock_BreakLock = 6032;

        /// <summary>
        /// The identifier for the SercosDeviceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint SercosDeviceType_CPIdentifier_Lock_InitLock = 6064;

        /// <summary>
        /// The identifier for the SercosDeviceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint SercosDeviceType_CPIdentifier_Lock_RenewLock = 6067;

        /// <summary>
        /// The identifier for the SercosDeviceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint SercosDeviceType_CPIdentifier_Lock_ExitLock = 6069;

        /// <summary>
        /// The identifier for the SercosDeviceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint SercosDeviceType_CPIdentifier_Lock_BreakLock = 6071;
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
        /// The identifier for the SercosNamespaceMetadata Object.
        /// </summary>
        public const uint SercosNamespaceMetadata = 6081;

        /// <summary>
        /// The identifier for the SercosDeviceType_ParameterSet Object.
        /// </summary>
        public const uint SercosDeviceType_ParameterSet = 5007;

        /// <summary>
        /// The identifier for the SercosDeviceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public const uint SercosDeviceType_CPIdentifier_NetworkAddress = 6073;

        /// <summary>
        /// The identifier for the SercosDeviceType_ProfileSet Object.
        /// </summary>
        public const uint SercosDeviceType_ProfileSet = 5001;

        /// <summary>
        /// The identifier for the SercosDeviceType_ClassSet Object.
        /// </summary>
        public const uint SercosDeviceType_ClassSet = 5002;

        /// <summary>
        /// The identifier for the SercosDeviceType_FunctionGroupSet Object.
        /// </summary>
        public const uint SercosDeviceType_FunctionGroupSet = 5003;

        /// <summary>
        /// The identifier for the ProfileSet_SercosProfileIdentifier_Placeholder Object.
        /// </summary>
        public const uint ProfileSet_SercosProfileIdentifier_Placeholder = 6076;

        /// <summary>
        /// The identifier for the ClassSet_SercosClassIdentifier_Placeholder Object.
        /// </summary>
        public const uint ClassSet_SercosClassIdentifier_Placeholder = 6078;

        /// <summary>
        /// The identifier for the FunctionGroupSet_FunctionGroupIdentifier_Placeholder Object.
        /// </summary>
        public const uint FunctionGroupSet_FunctionGroupIdentifier_Placeholder = 6080;
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
        /// The identifier for the FunctionalGroupType ObjectType.
        /// </summary>
        public const uint FunctionalGroupType = 6012;

        /// <summary>
        /// The identifier for the SercosProfileType ObjectType.
        /// </summary>
        public const uint SercosProfileType = 1002;

        /// <summary>
        /// The identifier for the SercosClassType ObjectType.
        /// </summary>
        public const uint SercosClassType = 1003;

        /// <summary>
        /// The identifier for the SercosFunctionGroupType ObjectType.
        /// </summary>
        public const uint SercosFunctionGroupType = 1004;

        /// <summary>
        /// The identifier for the SercosDeviceType ObjectType.
        /// </summary>
        public const uint SercosDeviceType = 1001;

        /// <summary>
        /// The identifier for the ProfileSet ObjectType.
        /// </summary>
        public const uint ProfileSet = 6075;

        /// <summary>
        /// The identifier for the ClassSet ObjectType.
        /// </summary>
        public const uint ClassSet = 6077;

        /// <summary>
        /// The identifier for the FunctionGroupSet ObjectType.
        /// </summary>
        public const uint FunctionGroupSet = 6079;
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
        /// The identifier for the SercosNamespaceMetadata_NamespaceUri Variable.
        /// </summary>
        public const uint SercosNamespaceMetadata_NamespaceUri = 6082;

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceVersion Variable.
        /// </summary>
        public const uint SercosNamespaceMetadata_NamespaceVersion = 6083;

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespacePublicationDate Variable.
        /// </summary>
        public const uint SercosNamespaceMetadata_NamespacePublicationDate = 6084;

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_IsNamespaceSubset Variable.
        /// </summary>
        public const uint SercosNamespaceMetadata_IsNamespaceSubset = 6085;

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_StaticNodeIdTypes Variable.
        /// </summary>
        public const uint SercosNamespaceMetadata_StaticNodeIdTypes = 6086;

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_StaticNumericNodeIdRange Variable.
        /// </summary>
        public const uint SercosNamespaceMetadata_StaticNumericNodeIdRange = 6087;

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_StaticStringNodeIdPattern Variable.
        /// </summary>
        public const uint SercosNamespaceMetadata_StaticStringNodeIdPattern = 6088;

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_Size Variable.
        /// </summary>
        public const uint SercosNamespaceMetadata_NamespaceFile_Size = 6090;

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_Writable Variable.
        /// </summary>
        public const uint SercosNamespaceMetadata_NamespaceFile_Writable = 6091;

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_UserWritable Variable.
        /// </summary>
        public const uint SercosNamespaceMetadata_NamespaceFile_UserWritable = 6092;

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_OpenCount Variable.
        /// </summary>
        public const uint SercosNamespaceMetadata_NamespaceFile_OpenCount = 6093;

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_Open_InputArguments Variable.
        /// </summary>
        public const uint SercosNamespaceMetadata_NamespaceFile_Open_InputArguments = 6096;

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_Open_OutputArguments Variable.
        /// </summary>
        public const uint SercosNamespaceMetadata_NamespaceFile_Open_OutputArguments = 6097;

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_Close_InputArguments Variable.
        /// </summary>
        public const uint SercosNamespaceMetadata_NamespaceFile_Close_InputArguments = 6099;

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_Read_InputArguments Variable.
        /// </summary>
        public const uint SercosNamespaceMetadata_NamespaceFile_Read_InputArguments = 6101;

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_Read_OutputArguments Variable.
        /// </summary>
        public const uint SercosNamespaceMetadata_NamespaceFile_Read_OutputArguments = 6102;

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_Write_InputArguments Variable.
        /// </summary>
        public const uint SercosNamespaceMetadata_NamespaceFile_Write_InputArguments = 6104;

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_GetPosition_InputArguments Variable.
        /// </summary>
        public const uint SercosNamespaceMetadata_NamespaceFile_GetPosition_InputArguments = 6106;

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_GetPosition_OutputArguments Variable.
        /// </summary>
        public const uint SercosNamespaceMetadata_NamespaceFile_GetPosition_OutputArguments = 6107;

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_SetPosition_InputArguments Variable.
        /// </summary>
        public const uint SercosNamespaceMetadata_NamespaceFile_SetPosition_InputArguments = 6109;

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_DefaultRolePermissions Variable.
        /// </summary>
        public const uint SercosNamespaceMetadata_DefaultRolePermissions = 6111;

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_DefaultUserRolePermissions Variable.
        /// </summary>
        public const uint SercosNamespaceMetadata_DefaultUserRolePermissions = 6112;

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_DefaultAccessRestrictions Variable.
        /// </summary>
        public const uint SercosNamespaceMetadata_DefaultAccessRestrictions = 6113;

        /// <summary>
        /// The identifier for the SercosDeviceType_Lock_Locked Variable.
        /// </summary>
        public const uint SercosDeviceType_Lock_Locked = 6021;

        /// <summary>
        /// The identifier for the SercosDeviceType_Lock_LockingClient Variable.
        /// </summary>
        public const uint SercosDeviceType_Lock_LockingClient = 6022;

        /// <summary>
        /// The identifier for the SercosDeviceType_Lock_LockingUser Variable.
        /// </summary>
        public const uint SercosDeviceType_Lock_LockingUser = 6023;

        /// <summary>
        /// The identifier for the SercosDeviceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint SercosDeviceType_Lock_RemainingLockTime = 6024;

        /// <summary>
        /// The identifier for the SercosDeviceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint SercosDeviceType_Lock_InitLock_InputArguments = 6026;

        /// <summary>
        /// The identifier for the SercosDeviceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint SercosDeviceType_Lock_InitLock_OutputArguments = 6027;

        /// <summary>
        /// The identifier for the SercosDeviceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint SercosDeviceType_Lock_RenewLock_OutputArguments = 6029;

        /// <summary>
        /// The identifier for the SercosDeviceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint SercosDeviceType_Lock_ExitLock_OutputArguments = 6031;

        /// <summary>
        /// The identifier for the SercosDeviceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint SercosDeviceType_Lock_BreakLock_OutputArguments = 6033;

        /// <summary>
        /// The identifier for the SercosDeviceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint SercosDeviceType_CPIdentifier_Lock_Locked = 6060;

        /// <summary>
        /// The identifier for the SercosDeviceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint SercosDeviceType_CPIdentifier_Lock_LockingClient = 6061;

        /// <summary>
        /// The identifier for the SercosDeviceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint SercosDeviceType_CPIdentifier_Lock_LockingUser = 6062;

        /// <summary>
        /// The identifier for the SercosDeviceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint SercosDeviceType_CPIdentifier_Lock_RemainingLockTime = 6063;

        /// <summary>
        /// The identifier for the SercosDeviceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint SercosDeviceType_CPIdentifier_Lock_InitLock_InputArguments = 6065;

        /// <summary>
        /// The identifier for the SercosDeviceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint SercosDeviceType_CPIdentifier_Lock_InitLock_OutputArguments = 6066;

        /// <summary>
        /// The identifier for the SercosDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint SercosDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments = 6068;

        /// <summary>
        /// The identifier for the SercosDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint SercosDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments = 6070;

        /// <summary>
        /// The identifier for the SercosDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint SercosDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments = 6072;

        /// <summary>
        /// The identifier for the SercosParameterType_Attribute Variable.
        /// </summary>
        public const uint SercosParameterType_Attribute = 6004;

        /// <summary>
        /// The identifier for the SercosParameterType_DisplayValue Variable.
        /// </summary>
        public const uint SercosParameterType_DisplayValue = 6009;

        /// <summary>
        /// The identifier for the SercosParameterType_DisplayMaxValue Variable.
        /// </summary>
        public const uint SercosParameterType_DisplayMaxValue = 6008;

        /// <summary>
        /// The identifier for the SercosParameterType_DisplayMinValue Variable.
        /// </summary>
        public const uint SercosParameterType_DisplayMinValue = 6007;

        /// <summary>
        /// The identifier for the SercosParameterType_Exponent Variable.
        /// </summary>
        public const uint SercosParameterType_Exponent = 6006;

        /// <summary>
        /// The identifier for the SercosParameterType_MaxValue Variable.
        /// </summary>
        public const uint SercosParameterType_MaxValue = 6001;

        /// <summary>
        /// The identifier for the SercosParameterType_MinValue Variable.
        /// </summary>
        public const uint SercosParameterType_MinValue = 6002;

        /// <summary>
        /// The identifier for the SercosParameterType_ProcedureCommand Variable.
        /// </summary>
        public const uint SercosParameterType_ProcedureCommand = 6005;
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
        /// The identifier for the SercosParameterType VariableType.
        /// </summary>
        public const uint SercosParameterType = 2001;
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
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_Open Method.
        /// </summary>
        public static readonly ExpandedNodeId SercosNamespaceMetadata_NamespaceFile_Open = new ExpandedNodeId(Sercos.Methods.SercosNamespaceMetadata_NamespaceFile_Open, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_Close Method.
        /// </summary>
        public static readonly ExpandedNodeId SercosNamespaceMetadata_NamespaceFile_Close = new ExpandedNodeId(Sercos.Methods.SercosNamespaceMetadata_NamespaceFile_Close, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_Read Method.
        /// </summary>
        public static readonly ExpandedNodeId SercosNamespaceMetadata_NamespaceFile_Read = new ExpandedNodeId(Sercos.Methods.SercosNamespaceMetadata_NamespaceFile_Read, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_Write Method.
        /// </summary>
        public static readonly ExpandedNodeId SercosNamespaceMetadata_NamespaceFile_Write = new ExpandedNodeId(Sercos.Methods.SercosNamespaceMetadata_NamespaceFile_Write, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_GetPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId SercosNamespaceMetadata_NamespaceFile_GetPosition = new ExpandedNodeId(Sercos.Methods.SercosNamespaceMetadata_NamespaceFile_GetPosition, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_SetPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId SercosNamespaceMetadata_NamespaceFile_SetPosition = new ExpandedNodeId(Sercos.Methods.SercosNamespaceMetadata_NamespaceFile_SetPosition, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_Lock_InitLock = new ExpandedNodeId(Sercos.Methods.SercosDeviceType_Lock_InitLock, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_Lock_RenewLock = new ExpandedNodeId(Sercos.Methods.SercosDeviceType_Lock_RenewLock, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_Lock_ExitLock = new ExpandedNodeId(Sercos.Methods.SercosDeviceType_Lock_ExitLock, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_Lock_BreakLock = new ExpandedNodeId(Sercos.Methods.SercosDeviceType_Lock_BreakLock, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_CPIdentifier_Lock_InitLock = new ExpandedNodeId(Sercos.Methods.SercosDeviceType_CPIdentifier_Lock_InitLock, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_CPIdentifier_Lock_RenewLock = new ExpandedNodeId(Sercos.Methods.SercosDeviceType_CPIdentifier_Lock_RenewLock, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_CPIdentifier_Lock_ExitLock = new ExpandedNodeId(Sercos.Methods.SercosDeviceType_CPIdentifier_Lock_ExitLock, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_CPIdentifier_Lock_BreakLock = new ExpandedNodeId(Sercos.Methods.SercosDeviceType_CPIdentifier_Lock_BreakLock, Sercos.Namespaces.Sercos);
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
        /// The identifier for the SercosNamespaceMetadata Object.
        /// </summary>
        public static readonly ExpandedNodeId SercosNamespaceMetadata = new ExpandedNodeId(Sercos.Objects.SercosNamespaceMetadata, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_ParameterSet Object.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_ParameterSet = new ExpandedNodeId(Sercos.Objects.SercosDeviceType_ParameterSet, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_CPIdentifier_NetworkAddress = new ExpandedNodeId(Sercos.Objects.SercosDeviceType_CPIdentifier_NetworkAddress, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_ProfileSet Object.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_ProfileSet = new ExpandedNodeId(Sercos.Objects.SercosDeviceType_ProfileSet, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_ClassSet Object.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_ClassSet = new ExpandedNodeId(Sercos.Objects.SercosDeviceType_ClassSet, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_FunctionGroupSet Object.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_FunctionGroupSet = new ExpandedNodeId(Sercos.Objects.SercosDeviceType_FunctionGroupSet, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the ProfileSet_SercosProfileIdentifier_Placeholder Object.
        /// </summary>
        public static readonly ExpandedNodeId ProfileSet_SercosProfileIdentifier_Placeholder = new ExpandedNodeId(Sercos.Objects.ProfileSet_SercosProfileIdentifier_Placeholder, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the ClassSet_SercosClassIdentifier_Placeholder Object.
        /// </summary>
        public static readonly ExpandedNodeId ClassSet_SercosClassIdentifier_Placeholder = new ExpandedNodeId(Sercos.Objects.ClassSet_SercosClassIdentifier_Placeholder, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the FunctionGroupSet_FunctionGroupIdentifier_Placeholder Object.
        /// </summary>
        public static readonly ExpandedNodeId FunctionGroupSet_FunctionGroupIdentifier_Placeholder = new ExpandedNodeId(Sercos.Objects.FunctionGroupSet_FunctionGroupIdentifier_Placeholder, Sercos.Namespaces.Sercos);
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
        /// The identifier for the FunctionalGroupType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId FunctionalGroupType = new ExpandedNodeId(Sercos.ObjectTypes.FunctionalGroupType, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosProfileType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId SercosProfileType = new ExpandedNodeId(Sercos.ObjectTypes.SercosProfileType, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosClassType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId SercosClassType = new ExpandedNodeId(Sercos.ObjectTypes.SercosClassType, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosFunctionGroupType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId SercosFunctionGroupType = new ExpandedNodeId(Sercos.ObjectTypes.SercosFunctionGroupType, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType = new ExpandedNodeId(Sercos.ObjectTypes.SercosDeviceType, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the ProfileSet ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ProfileSet = new ExpandedNodeId(Sercos.ObjectTypes.ProfileSet, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the ClassSet ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ClassSet = new ExpandedNodeId(Sercos.ObjectTypes.ClassSet, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the FunctionGroupSet ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId FunctionGroupSet = new ExpandedNodeId(Sercos.ObjectTypes.FunctionGroupSet, Sercos.Namespaces.Sercos);
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
        /// The identifier for the SercosNamespaceMetadata_NamespaceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosNamespaceMetadata_NamespaceUri = new ExpandedNodeId(Sercos.Variables.SercosNamespaceMetadata_NamespaceUri, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceVersion Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosNamespaceMetadata_NamespaceVersion = new ExpandedNodeId(Sercos.Variables.SercosNamespaceMetadata_NamespaceVersion, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespacePublicationDate Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosNamespaceMetadata_NamespacePublicationDate = new ExpandedNodeId(Sercos.Variables.SercosNamespaceMetadata_NamespacePublicationDate, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_IsNamespaceSubset Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosNamespaceMetadata_IsNamespaceSubset = new ExpandedNodeId(Sercos.Variables.SercosNamespaceMetadata_IsNamespaceSubset, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_StaticNodeIdTypes Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosNamespaceMetadata_StaticNodeIdTypes = new ExpandedNodeId(Sercos.Variables.SercosNamespaceMetadata_StaticNodeIdTypes, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_StaticNumericNodeIdRange Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosNamespaceMetadata_StaticNumericNodeIdRange = new ExpandedNodeId(Sercos.Variables.SercosNamespaceMetadata_StaticNumericNodeIdRange, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_StaticStringNodeIdPattern Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosNamespaceMetadata_StaticStringNodeIdPattern = new ExpandedNodeId(Sercos.Variables.SercosNamespaceMetadata_StaticStringNodeIdPattern, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_Size Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosNamespaceMetadata_NamespaceFile_Size = new ExpandedNodeId(Sercos.Variables.SercosNamespaceMetadata_NamespaceFile_Size, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_Writable Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosNamespaceMetadata_NamespaceFile_Writable = new ExpandedNodeId(Sercos.Variables.SercosNamespaceMetadata_NamespaceFile_Writable, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_UserWritable Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosNamespaceMetadata_NamespaceFile_UserWritable = new ExpandedNodeId(Sercos.Variables.SercosNamespaceMetadata_NamespaceFile_UserWritable, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_OpenCount Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosNamespaceMetadata_NamespaceFile_OpenCount = new ExpandedNodeId(Sercos.Variables.SercosNamespaceMetadata_NamespaceFile_OpenCount, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_Open_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosNamespaceMetadata_NamespaceFile_Open_InputArguments = new ExpandedNodeId(Sercos.Variables.SercosNamespaceMetadata_NamespaceFile_Open_InputArguments, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_Open_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosNamespaceMetadata_NamespaceFile_Open_OutputArguments = new ExpandedNodeId(Sercos.Variables.SercosNamespaceMetadata_NamespaceFile_Open_OutputArguments, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_Close_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosNamespaceMetadata_NamespaceFile_Close_InputArguments = new ExpandedNodeId(Sercos.Variables.SercosNamespaceMetadata_NamespaceFile_Close_InputArguments, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_Read_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosNamespaceMetadata_NamespaceFile_Read_InputArguments = new ExpandedNodeId(Sercos.Variables.SercosNamespaceMetadata_NamespaceFile_Read_InputArguments, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_Read_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosNamespaceMetadata_NamespaceFile_Read_OutputArguments = new ExpandedNodeId(Sercos.Variables.SercosNamespaceMetadata_NamespaceFile_Read_OutputArguments, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_Write_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosNamespaceMetadata_NamespaceFile_Write_InputArguments = new ExpandedNodeId(Sercos.Variables.SercosNamespaceMetadata_NamespaceFile_Write_InputArguments, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_GetPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosNamespaceMetadata_NamespaceFile_GetPosition_InputArguments = new ExpandedNodeId(Sercos.Variables.SercosNamespaceMetadata_NamespaceFile_GetPosition_InputArguments, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_GetPosition_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosNamespaceMetadata_NamespaceFile_GetPosition_OutputArguments = new ExpandedNodeId(Sercos.Variables.SercosNamespaceMetadata_NamespaceFile_GetPosition_OutputArguments, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_NamespaceFile_SetPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosNamespaceMetadata_NamespaceFile_SetPosition_InputArguments = new ExpandedNodeId(Sercos.Variables.SercosNamespaceMetadata_NamespaceFile_SetPosition_InputArguments, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_DefaultRolePermissions Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosNamespaceMetadata_DefaultRolePermissions = new ExpandedNodeId(Sercos.Variables.SercosNamespaceMetadata_DefaultRolePermissions, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_DefaultUserRolePermissions Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosNamespaceMetadata_DefaultUserRolePermissions = new ExpandedNodeId(Sercos.Variables.SercosNamespaceMetadata_DefaultUserRolePermissions, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosNamespaceMetadata_DefaultAccessRestrictions Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosNamespaceMetadata_DefaultAccessRestrictions = new ExpandedNodeId(Sercos.Variables.SercosNamespaceMetadata_DefaultAccessRestrictions, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_Lock_Locked = new ExpandedNodeId(Sercos.Variables.SercosDeviceType_Lock_Locked, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_Lock_LockingClient = new ExpandedNodeId(Sercos.Variables.SercosDeviceType_Lock_LockingClient, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_Lock_LockingUser = new ExpandedNodeId(Sercos.Variables.SercosDeviceType_Lock_LockingUser, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_Lock_RemainingLockTime = new ExpandedNodeId(Sercos.Variables.SercosDeviceType_Lock_RemainingLockTime, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_Lock_InitLock_InputArguments = new ExpandedNodeId(Sercos.Variables.SercosDeviceType_Lock_InitLock_InputArguments, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Sercos.Variables.SercosDeviceType_Lock_InitLock_OutputArguments, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Sercos.Variables.SercosDeviceType_Lock_RenewLock_OutputArguments, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Sercos.Variables.SercosDeviceType_Lock_ExitLock_OutputArguments, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Sercos.Variables.SercosDeviceType_Lock_BreakLock_OutputArguments, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_CPIdentifier_Lock_Locked = new ExpandedNodeId(Sercos.Variables.SercosDeviceType_CPIdentifier_Lock_Locked, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_CPIdentifier_Lock_LockingClient = new ExpandedNodeId(Sercos.Variables.SercosDeviceType_CPIdentifier_Lock_LockingClient, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_CPIdentifier_Lock_LockingUser = new ExpandedNodeId(Sercos.Variables.SercosDeviceType_CPIdentifier_Lock_LockingUser, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_CPIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Sercos.Variables.SercosDeviceType_CPIdentifier_Lock_RemainingLockTime, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_CPIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Sercos.Variables.SercosDeviceType_CPIdentifier_Lock_InitLock_InputArguments, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_CPIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Sercos.Variables.SercosDeviceType_CPIdentifier_Lock_InitLock_OutputArguments, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Sercos.Variables.SercosDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Sercos.Variables.SercosDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Sercos.Variables.SercosDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosParameterType_Attribute Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosParameterType_Attribute = new ExpandedNodeId(Sercos.Variables.SercosParameterType_Attribute, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosParameterType_DisplayValue Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosParameterType_DisplayValue = new ExpandedNodeId(Sercos.Variables.SercosParameterType_DisplayValue, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosParameterType_DisplayMaxValue Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosParameterType_DisplayMaxValue = new ExpandedNodeId(Sercos.Variables.SercosParameterType_DisplayMaxValue, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosParameterType_DisplayMinValue Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosParameterType_DisplayMinValue = new ExpandedNodeId(Sercos.Variables.SercosParameterType_DisplayMinValue, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosParameterType_Exponent Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosParameterType_Exponent = new ExpandedNodeId(Sercos.Variables.SercosParameterType_Exponent, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosParameterType_MaxValue Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosParameterType_MaxValue = new ExpandedNodeId(Sercos.Variables.SercosParameterType_MaxValue, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosParameterType_MinValue Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosParameterType_MinValue = new ExpandedNodeId(Sercos.Variables.SercosParameterType_MinValue, Sercos.Namespaces.Sercos);

        /// <summary>
        /// The identifier for the SercosParameterType_ProcedureCommand Variable.
        /// </summary>
        public static readonly ExpandedNodeId SercosParameterType_ProcedureCommand = new ExpandedNodeId(Sercos.Variables.SercosParameterType_ProcedureCommand, Sercos.Namespaces.Sercos);
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
        /// The identifier for the SercosParameterType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId SercosParameterType = new ExpandedNodeId(Sercos.VariableTypes.SercosParameterType, Sercos.Namespaces.Sercos);
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
        /// The BrowseName for the Attribute component.
        /// </summary>
        public const string Attribute = "Attribute";

        /// <summary>
        /// The BrowseName for the ClassSet component.
        /// </summary>
        public const string ClassSet = "ClassSet";

        /// <summary>
        /// The BrowseName for the DisplayMaxValue component.
        /// </summary>
        public const string DisplayMaxValue = "DisplayMaxValue";

        /// <summary>
        /// The BrowseName for the DisplayMinValue component.
        /// </summary>
        public const string DisplayMinValue = "DisplayMinValue";

        /// <summary>
        /// The BrowseName for the DisplayValue component.
        /// </summary>
        public const string DisplayValue = "DisplayValue";

        /// <summary>
        /// The BrowseName for the Exponent component.
        /// </summary>
        public const string Exponent = "Exponent";

        /// <summary>
        /// The BrowseName for the FunctionalGroupType component.
        /// </summary>
        public const string FunctionalGroupType = "FunctionalGroupType";

        /// <summary>
        /// The BrowseName for the FunctionGroupIdentifier_Placeholder component.
        /// </summary>
        public const string FunctionGroupIdentifier_Placeholder = "<FunctionGroupIdentifier>";

        /// <summary>
        /// The BrowseName for the FunctionGroupSet component.
        /// </summary>
        public const string FunctionGroupSet = "FunctionGroupSet";

        /// <summary>
        /// The BrowseName for the MaxValue component.
        /// </summary>
        public const string MaxValue = "MaxValue";

        /// <summary>
        /// The BrowseName for the MinValue component.
        /// </summary>
        public const string MinValue = "MinValue";

        /// <summary>
        /// The BrowseName for the ParameterSet component.
        /// </summary>
        public const string ParameterSet = "ParameterSet";

        /// <summary>
        /// The BrowseName for the ProcedureCommand component.
        /// </summary>
        public const string ProcedureCommand = "ProcedureCommand";

        /// <summary>
        /// The BrowseName for the ProfileSet component.
        /// </summary>
        public const string ProfileSet = "ProfileSet";

        /// <summary>
        /// The BrowseName for the SercosClassIdentifier_Placeholder component.
        /// </summary>
        public const string SercosClassIdentifier_Placeholder = "<SercosClassIdentifier>";

        /// <summary>
        /// The BrowseName for the SercosClassType component.
        /// </summary>
        public const string SercosClassType = "SercosClassType";

        /// <summary>
        /// The BrowseName for the SercosDeviceType component.
        /// </summary>
        public const string SercosDeviceType = "SercosDeviceType";

        /// <summary>
        /// The BrowseName for the SercosFunctionGroupType component.
        /// </summary>
        public const string SercosFunctionGroupType = "SercosFunctionGroupType";

        /// <summary>
        /// The BrowseName for the SercosNamespaceMetadata component.
        /// </summary>
        public const string SercosNamespaceMetadata = "http://sercos.org/UA/";

        /// <summary>
        /// The BrowseName for the SercosParameterType component.
        /// </summary>
        public const string SercosParameterType = "SercosParameterType";

        /// <summary>
        /// The BrowseName for the SercosProfileIdentifier_Placeholder component.
        /// </summary>
        public const string SercosProfileIdentifier_Placeholder = "<SercosProfileIdentifier>";

        /// <summary>
        /// The BrowseName for the SercosProfileType component.
        /// </summary>
        public const string SercosProfileType = "SercosProfileType";
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
        /// The URI for the Sercos namespace (.NET code namespace is 'Sercos').
        /// </summary>
        public const string Sercos = "http://sercos.org/UA/";

        /// <summary>
        /// The URI for the SercosXsd namespace (.NET code namespace is 'Sercos').
        /// </summary>
        public const string SercosXsd = "http://sercos.org/UA/Types.xsd";

        /// <summary>
        /// The URI for the DI namespace (.NET code namespace is 'Opc.Ua.Di').
        /// </summary>
        public const string DI = "http://opcfoundation.org/UA/DI/";

        /// <summary>
        /// The URI for the DIXsd namespace (.NET code namespace is 'Opc.Ua.Di').
        /// </summary>
        public const string DIXsd = "http://opcfoundation.org/UA/DI/Types.xsd";

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
