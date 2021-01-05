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

namespace Opc.Ua.Fdi5
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
        /// The identifier for the WindowModeType DataType.
        /// </summary>
        public const uint WindowModeType = 194;

        /// <summary>
        /// The identifier for the StyleType DataType.
        /// </summary>
        public const uint StyleType = 196;

        /// <summary>
        /// The identifier for the RegistrationParameters DataType.
        /// </summary>
        public const uint RegistrationParameters = 37;

        /// <summary>
        /// The identifier for the RegisteredNode DataType.
        /// </summary>
        public const uint RegisteredNode = 38;

        /// <summary>
        /// The identifier for the RegisterNodesResult DataType.
        /// </summary>
        public const uint RegisterNodesResult = 39;

        /// <summary>
        /// The identifier for the TransferIncident DataType.
        /// </summary>
        public const uint TransferIncident = 43;

        /// <summary>
        /// The identifier for the ApplyResult DataType.
        /// </summary>
        public const uint ApplyResult = 44;
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
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_Open Method.
        /// </summary>
        public const uint OpcUaFdi5NamespaceMetadata_NamespaceFile_Open = 15015;

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_Close Method.
        /// </summary>
        public const uint OpcUaFdi5NamespaceMetadata_NamespaceFile_Close = 15018;

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_Read Method.
        /// </summary>
        public const uint OpcUaFdi5NamespaceMetadata_NamespaceFile_Read = 15020;

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_Write Method.
        /// </summary>
        public const uint OpcUaFdi5NamespaceMetadata_NamespaceFile_Write = 15023;

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_GetPosition Method.
        /// </summary>
        public const uint OpcUaFdi5NamespaceMetadata_NamespaceFile_GetPosition = 15025;

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_SetPosition Method.
        /// </summary>
        public const uint OpcUaFdi5NamespaceMetadata_NamespaceFile_SetPosition = 15028;

        /// <summary>
        /// The identifier for the ActionServiceType_InvokeAction Method.
        /// </summary>
        public const uint ActionServiceType_InvokeAction = 22;

        /// <summary>
        /// The identifier for the ActionServiceType_RespondAction Method.
        /// </summary>
        public const uint ActionServiceType_RespondAction = 25;

        /// <summary>
        /// The identifier for the ActionServiceType_AbortAction Method.
        /// </summary>
        public const uint ActionServiceType_AbortAction = 28;

        /// <summary>
        /// The identifier for the EditContextType_GetEditContext Method.
        /// </summary>
        public const uint EditContextType_GetEditContext = 55;

        /// <summary>
        /// The identifier for the EditContextType_RegisterNodesById Method.
        /// </summary>
        public const uint EditContextType_RegisterNodesById = 58;

        /// <summary>
        /// The identifier for the EditContextType_RegisterNodesByRelativePath Method.
        /// </summary>
        public const uint EditContextType_RegisterNodesByRelativePath = 61;

        /// <summary>
        /// The identifier for the EditContextType_Apply Method.
        /// </summary>
        public const uint EditContextType_Apply = 64;

        /// <summary>
        /// The identifier for the EditContextType_Reset Method.
        /// </summary>
        public const uint EditContextType_Reset = 67;

        /// <summary>
        /// The identifier for the EditContextType_Discard Method.
        /// </summary>
        public const uint EditContextType_Discard = 70;

        /// <summary>
        /// The identifier for the DirectDeviceAccessType_InitDirectAccess Method.
        /// </summary>
        public const uint DirectDeviceAccessType_InitDirectAccess = 83;

        /// <summary>
        /// The identifier for the DirectDeviceAccessType_Transfer Method.
        /// </summary>
        public const uint DirectDeviceAccessType_Transfer = 86;

        /// <summary>
        /// The identifier for the DirectDeviceAccessType_EndDirectAccess Method.
        /// </summary>
        public const uint DirectDeviceAccessType_EndDirectAccess = 89;

        /// <summary>
        /// The identifier for the LogAuditTrailMessage Method.
        /// </summary>
        public const uint LogAuditTrailMessage = 92;

        /// <summary>
        /// The identifier for the ActionSet_InvokeAction Method.
        /// </summary>
        public const uint ActionSet_InvokeAction = 184;

        /// <summary>
        /// The identifier for the ActionSet_RespondAction Method.
        /// </summary>
        public const uint ActionSet_RespondAction = 187;

        /// <summary>
        /// The identifier for the ActionSet_AbortAction Method.
        /// </summary>
        public const uint ActionSet_AbortAction = 190;
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
        /// The identifier for the OpcUaFdi5NamespaceMetadata Object.
        /// </summary>
        public const uint OpcUaFdi5NamespaceMetadata = 15001;

        /// <summary>
        /// The identifier for the UIPlugInType_Documentation Object.
        /// </summary>
        public const uint UIPlugInType_Documentation = 10;

        /// <summary>
        /// The identifier for the ActionServiceType_ActionIdentifier_PH Object.
        /// </summary>
        public const uint ActionServiceType_ActionIdentifier_PH = 181;

        /// <summary>
        /// The identifier for the ActionIdentifier Object.
        /// </summary>
        public const uint ActionIdentifier = 182;

        /// <summary>
        /// The identifier for the ActionSet Object.
        /// </summary>
        public const uint ActionSet = 183;

        /// <summary>
        /// The identifier for the RegistrationParameters_Encoding_DefaultBinary Object.
        /// </summary>
        public const uint RegistrationParameters_Encoding_DefaultBinary = 118;

        /// <summary>
        /// The identifier for the RegisteredNode_Encoding_DefaultBinary Object.
        /// </summary>
        public const uint RegisteredNode_Encoding_DefaultBinary = 119;

        /// <summary>
        /// The identifier for the RegisterNodesResult_Encoding_DefaultBinary Object.
        /// </summary>
        public const uint RegisterNodesResult_Encoding_DefaultBinary = 120;

        /// <summary>
        /// The identifier for the TransferIncident_Encoding_DefaultBinary Object.
        /// </summary>
        public const uint TransferIncident_Encoding_DefaultBinary = 121;

        /// <summary>
        /// The identifier for the ApplyResult_Encoding_DefaultBinary Object.
        /// </summary>
        public const uint ApplyResult_Encoding_DefaultBinary = 122;

        /// <summary>
        /// The identifier for the RegistrationParameters_Encoding_DefaultXml Object.
        /// </summary>
        public const uint RegistrationParameters_Encoding_DefaultXml = 95;

        /// <summary>
        /// The identifier for the RegisteredNode_Encoding_DefaultXml Object.
        /// </summary>
        public const uint RegisteredNode_Encoding_DefaultXml = 96;

        /// <summary>
        /// The identifier for the RegisterNodesResult_Encoding_DefaultXml Object.
        /// </summary>
        public const uint RegisterNodesResult_Encoding_DefaultXml = 97;

        /// <summary>
        /// The identifier for the TransferIncident_Encoding_DefaultXml Object.
        /// </summary>
        public const uint TransferIncident_Encoding_DefaultXml = 98;

        /// <summary>
        /// The identifier for the ApplyResult_Encoding_DefaultXml Object.
        /// </summary>
        public const uint ApplyResult_Encoding_DefaultXml = 99;

        /// <summary>
        /// The identifier for the RegistrationParameters_Encoding_DefaultJson Object.
        /// </summary>
        public const uint RegistrationParameters_Encoding_DefaultJson = 8001;

        /// <summary>
        /// The identifier for the RegisteredNode_Encoding_DefaultJson Object.
        /// </summary>
        public const uint RegisteredNode_Encoding_DefaultJson = 8002;

        /// <summary>
        /// The identifier for the RegisterNodesResult_Encoding_DefaultJson Object.
        /// </summary>
        public const uint RegisterNodesResult_Encoding_DefaultJson = 8003;

        /// <summary>
        /// The identifier for the TransferIncident_Encoding_DefaultJson Object.
        /// </summary>
        public const uint TransferIncident_Encoding_DefaultJson = 8004;

        /// <summary>
        /// The identifier for the ApplyResult_Encoding_DefaultJson Object.
        /// </summary>
        public const uint ApplyResult_Encoding_DefaultJson = 8005;
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
        /// The identifier for the ActionType ObjectType.
        /// </summary>
        public const uint ActionType = 11;

        /// <summary>
        /// The identifier for the ActionServiceType ObjectType.
        /// </summary>
        public const uint ActionServiceType = 21;

        /// <summary>
        /// The identifier for the EditContextType ObjectType.
        /// </summary>
        public const uint EditContextType = 54;

        /// <summary>
        /// The identifier for the DirectDeviceAccessType ObjectType.
        /// </summary>
        public const uint DirectDeviceAccessType = 82;
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
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceUri Variable.
        /// </summary>
        public const uint OpcUaFdi5NamespaceMetadata_NamespaceUri = 15002;

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceVersion Variable.
        /// </summary>
        public const uint OpcUaFdi5NamespaceMetadata_NamespaceVersion = 15003;

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespacePublicationDate Variable.
        /// </summary>
        public const uint OpcUaFdi5NamespaceMetadata_NamespacePublicationDate = 15004;

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_IsNamespaceSubset Variable.
        /// </summary>
        public const uint OpcUaFdi5NamespaceMetadata_IsNamespaceSubset = 15005;

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_StaticNodeIdTypes Variable.
        /// </summary>
        public const uint OpcUaFdi5NamespaceMetadata_StaticNodeIdTypes = 15006;

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_StaticNumericNodeIdRange Variable.
        /// </summary>
        public const uint OpcUaFdi5NamespaceMetadata_StaticNumericNodeIdRange = 15007;

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_StaticStringNodeIdPattern Variable.
        /// </summary>
        public const uint OpcUaFdi5NamespaceMetadata_StaticStringNodeIdPattern = 15008;

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_Size Variable.
        /// </summary>
        public const uint OpcUaFdi5NamespaceMetadata_NamespaceFile_Size = 15010;

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_Writable Variable.
        /// </summary>
        public const uint OpcUaFdi5NamespaceMetadata_NamespaceFile_Writable = 15011;

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_UserWritable Variable.
        /// </summary>
        public const uint OpcUaFdi5NamespaceMetadata_NamespaceFile_UserWritable = 15012;

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_OpenCount Variable.
        /// </summary>
        public const uint OpcUaFdi5NamespaceMetadata_NamespaceFile_OpenCount = 15013;

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_Open_InputArguments Variable.
        /// </summary>
        public const uint OpcUaFdi5NamespaceMetadata_NamespaceFile_Open_InputArguments = 15016;

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_Open_OutputArguments Variable.
        /// </summary>
        public const uint OpcUaFdi5NamespaceMetadata_NamespaceFile_Open_OutputArguments = 15017;

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_Close_InputArguments Variable.
        /// </summary>
        public const uint OpcUaFdi5NamespaceMetadata_NamespaceFile_Close_InputArguments = 15019;

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_Read_InputArguments Variable.
        /// </summary>
        public const uint OpcUaFdi5NamespaceMetadata_NamespaceFile_Read_InputArguments = 15021;

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_Read_OutputArguments Variable.
        /// </summary>
        public const uint OpcUaFdi5NamespaceMetadata_NamespaceFile_Read_OutputArguments = 15022;

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_Write_InputArguments Variable.
        /// </summary>
        public const uint OpcUaFdi5NamespaceMetadata_NamespaceFile_Write_InputArguments = 15024;

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_GetPosition_InputArguments Variable.
        /// </summary>
        public const uint OpcUaFdi5NamespaceMetadata_NamespaceFile_GetPosition_InputArguments = 15026;

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_GetPosition_OutputArguments Variable.
        /// </summary>
        public const uint OpcUaFdi5NamespaceMetadata_NamespaceFile_GetPosition_OutputArguments = 15027;

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_SetPosition_InputArguments Variable.
        /// </summary>
        public const uint OpcUaFdi5NamespaceMetadata_NamespaceFile_SetPosition_InputArguments = 15029;

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_DefaultRolePermissions Variable.
        /// </summary>
        public const uint OpcUaFdi5NamespaceMetadata_DefaultRolePermissions = 15031;

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_DefaultUserRolePermissions Variable.
        /// </summary>
        public const uint OpcUaFdi5NamespaceMetadata_DefaultUserRolePermissions = 15032;

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_DefaultAccessRestrictions Variable.
        /// </summary>
        public const uint OpcUaFdi5NamespaceMetadata_DefaultAccessRestrictions = 15033;

        /// <summary>
        /// The identifier for the UIPlugInType_UIPVariantVersion Variable.
        /// </summary>
        public const uint UIPlugInType_UIPVariantVersion = 3;

        /// <summary>
        /// The identifier for the UIPlugInType_FDITechnologyVersion Variable.
        /// </summary>
        public const uint UIPlugInType_FDITechnologyVersion = 4;

        /// <summary>
        /// The identifier for the UIPlugInType_RuntimeId Variable.
        /// </summary>
        public const uint UIPlugInType_RuntimeId = 5;

        /// <summary>
        /// The identifier for the UIPlugInType_CpuInformation Variable.
        /// </summary>
        public const uint UIPlugInType_CpuInformation = 6;

        /// <summary>
        /// The identifier for the UIPlugInType_PlatformId Variable.
        /// </summary>
        public const uint UIPlugInType_PlatformId = 7;

        /// <summary>
        /// The identifier for the UIPlugInType_Style Variable.
        /// </summary>
        public const uint UIPlugInType_Style = 8;

        /// <summary>
        /// The identifier for the UIPlugInType_StartElementName Variable.
        /// </summary>
        public const uint UIPlugInType_StartElementName = 9;

        /// <summary>
        /// The identifier for the ActionServiceType_InvokeAction_InputArguments Variable.
        /// </summary>
        public const uint ActionServiceType_InvokeAction_InputArguments = 23;

        /// <summary>
        /// The identifier for the ActionServiceType_InvokeAction_OutputArguments Variable.
        /// </summary>
        public const uint ActionServiceType_InvokeAction_OutputArguments = 24;

        /// <summary>
        /// The identifier for the ActionServiceType_RespondAction_InputArguments Variable.
        /// </summary>
        public const uint ActionServiceType_RespondAction_InputArguments = 26;

        /// <summary>
        /// The identifier for the ActionServiceType_RespondAction_OutputArguments Variable.
        /// </summary>
        public const uint ActionServiceType_RespondAction_OutputArguments = 27;

        /// <summary>
        /// The identifier for the ActionServiceType_AbortAction_InputArguments Variable.
        /// </summary>
        public const uint ActionServiceType_AbortAction_InputArguments = 29;

        /// <summary>
        /// The identifier for the ActionServiceType_AbortAction_OutputArguments Variable.
        /// </summary>
        public const uint ActionServiceType_AbortAction_OutputArguments = 30;

        /// <summary>
        /// The identifier for the WindowModeType_EnumValues Variable.
        /// </summary>
        public const uint WindowModeType_EnumValues = 195;

        /// <summary>
        /// The identifier for the StyleType_EnumValues Variable.
        /// </summary>
        public const uint StyleType_EnumValues = 197;

        /// <summary>
        /// The identifier for the EditContextType_GetEditContext_InputArguments Variable.
        /// </summary>
        public const uint EditContextType_GetEditContext_InputArguments = 56;

        /// <summary>
        /// The identifier for the EditContextType_GetEditContext_OutputArguments Variable.
        /// </summary>
        public const uint EditContextType_GetEditContext_OutputArguments = 57;

        /// <summary>
        /// The identifier for the EditContextType_RegisterNodesById_InputArguments Variable.
        /// </summary>
        public const uint EditContextType_RegisterNodesById_InputArguments = 59;

        /// <summary>
        /// The identifier for the EditContextType_RegisterNodesById_OutputArguments Variable.
        /// </summary>
        public const uint EditContextType_RegisterNodesById_OutputArguments = 60;

        /// <summary>
        /// The identifier for the EditContextType_RegisterNodesByRelativePath_InputArguments Variable.
        /// </summary>
        public const uint EditContextType_RegisterNodesByRelativePath_InputArguments = 62;

        /// <summary>
        /// The identifier for the EditContextType_RegisterNodesByRelativePath_OutputArguments Variable.
        /// </summary>
        public const uint EditContextType_RegisterNodesByRelativePath_OutputArguments = 63;

        /// <summary>
        /// The identifier for the EditContextType_Apply_InputArguments Variable.
        /// </summary>
        public const uint EditContextType_Apply_InputArguments = 65;

        /// <summary>
        /// The identifier for the EditContextType_Apply_OutputArguments Variable.
        /// </summary>
        public const uint EditContextType_Apply_OutputArguments = 66;

        /// <summary>
        /// The identifier for the EditContextType_Reset_InputArguments Variable.
        /// </summary>
        public const uint EditContextType_Reset_InputArguments = 68;

        /// <summary>
        /// The identifier for the EditContextType_Reset_OutputArguments Variable.
        /// </summary>
        public const uint EditContextType_Reset_OutputArguments = 69;

        /// <summary>
        /// The identifier for the EditContextType_Discard_InputArguments Variable.
        /// </summary>
        public const uint EditContextType_Discard_InputArguments = 71;

        /// <summary>
        /// The identifier for the EditContextType_Discard_OutputArguments Variable.
        /// </summary>
        public const uint EditContextType_Discard_OutputArguments = 72;

        /// <summary>
        /// The identifier for the DirectDeviceAccessType_InitDirectAccess_InputArguments Variable.
        /// </summary>
        public const uint DirectDeviceAccessType_InitDirectAccess_InputArguments = 84;

        /// <summary>
        /// The identifier for the DirectDeviceAccessType_InitDirectAccess_OutputArguments Variable.
        /// </summary>
        public const uint DirectDeviceAccessType_InitDirectAccess_OutputArguments = 85;

        /// <summary>
        /// The identifier for the DirectDeviceAccessType_Transfer_InputArguments Variable.
        /// </summary>
        public const uint DirectDeviceAccessType_Transfer_InputArguments = 87;

        /// <summary>
        /// The identifier for the DirectDeviceAccessType_Transfer_OutputArguments Variable.
        /// </summary>
        public const uint DirectDeviceAccessType_Transfer_OutputArguments = 88;

        /// <summary>
        /// The identifier for the DirectDeviceAccessType_EndDirectAccess_InputArguments Variable.
        /// </summary>
        public const uint DirectDeviceAccessType_EndDirectAccess_InputArguments = 90;

        /// <summary>
        /// The identifier for the DirectDeviceAccessType_EndDirectAccess_OutputArguments Variable.
        /// </summary>
        public const uint DirectDeviceAccessType_EndDirectAccess_OutputArguments = 91;

        /// <summary>
        /// The identifier for the LogAuditTrailMessage_InputArguments Variable.
        /// </summary>
        public const uint LogAuditTrailMessage_InputArguments = 93;

        /// <summary>
        /// The identifier for the FDIServerVersion Variable.
        /// </summary>
        public const uint FDIServerVersion = 94;

        /// <summary>
        /// The identifier for the DeviceHealthDiagnostics Variable.
        /// </summary>
        public const uint DeviceHealthDiagnostics = 198;

        /// <summary>
        /// The identifier for the ActionSet_InvokeAction_InputArguments Variable.
        /// </summary>
        public const uint ActionSet_InvokeAction_InputArguments = 185;

        /// <summary>
        /// The identifier for the ActionSet_InvokeAction_OutputArguments Variable.
        /// </summary>
        public const uint ActionSet_InvokeAction_OutputArguments = 186;

        /// <summary>
        /// The identifier for the ActionSet_RespondAction_InputArguments Variable.
        /// </summary>
        public const uint ActionSet_RespondAction_InputArguments = 188;

        /// <summary>
        /// The identifier for the ActionSet_RespondAction_OutputArguments Variable.
        /// </summary>
        public const uint ActionSet_RespondAction_OutputArguments = 189;

        /// <summary>
        /// The identifier for the ActionSet_AbortAction_InputArguments Variable.
        /// </summary>
        public const uint ActionSet_AbortAction_InputArguments = 191;

        /// <summary>
        /// The identifier for the ActionSet_AbortAction_OutputArguments Variable.
        /// </summary>
        public const uint ActionSet_AbortAction_OutputArguments = 192;

        /// <summary>
        /// The identifier for the OpcUaFdi5_BinarySchema Variable.
        /// </summary>
        public const uint OpcUaFdi5_BinarySchema = 123;

        /// <summary>
        /// The identifier for the OpcUaFdi5_BinarySchema_NamespaceUri Variable.
        /// </summary>
        public const uint OpcUaFdi5_BinarySchema_NamespaceUri = 125;

        /// <summary>
        /// The identifier for the OpcUaFdi5_BinarySchema_Deprecated Variable.
        /// </summary>
        public const uint OpcUaFdi5_BinarySchema_Deprecated = 8006;

        /// <summary>
        /// The identifier for the OpcUaFdi5_BinarySchema_RegistrationParameters Variable.
        /// </summary>
        public const uint OpcUaFdi5_BinarySchema_RegistrationParameters = 126;

        /// <summary>
        /// The identifier for the OpcUaFdi5_BinarySchema_RegisteredNode Variable.
        /// </summary>
        public const uint OpcUaFdi5_BinarySchema_RegisteredNode = 129;

        /// <summary>
        /// The identifier for the OpcUaFdi5_BinarySchema_RegisterNodesResult Variable.
        /// </summary>
        public const uint OpcUaFdi5_BinarySchema_RegisterNodesResult = 132;

        /// <summary>
        /// The identifier for the OpcUaFdi5_BinarySchema_TransferIncident Variable.
        /// </summary>
        public const uint OpcUaFdi5_BinarySchema_TransferIncident = 135;

        /// <summary>
        /// The identifier for the OpcUaFdi5_BinarySchema_ApplyResult Variable.
        /// </summary>
        public const uint OpcUaFdi5_BinarySchema_ApplyResult = 138;

        /// <summary>
        /// The identifier for the OpcUaFdi5_XmlSchema Variable.
        /// </summary>
        public const uint OpcUaFdi5_XmlSchema = 100;

        /// <summary>
        /// The identifier for the OpcUaFdi5_XmlSchema_NamespaceUri Variable.
        /// </summary>
        public const uint OpcUaFdi5_XmlSchema_NamespaceUri = 102;

        /// <summary>
        /// The identifier for the OpcUaFdi5_XmlSchema_Deprecated Variable.
        /// </summary>
        public const uint OpcUaFdi5_XmlSchema_Deprecated = 8008;

        /// <summary>
        /// The identifier for the OpcUaFdi5_XmlSchema_RegistrationParameters Variable.
        /// </summary>
        public const uint OpcUaFdi5_XmlSchema_RegistrationParameters = 103;

        /// <summary>
        /// The identifier for the OpcUaFdi5_XmlSchema_RegisteredNode Variable.
        /// </summary>
        public const uint OpcUaFdi5_XmlSchema_RegisteredNode = 106;

        /// <summary>
        /// The identifier for the OpcUaFdi5_XmlSchema_RegisterNodesResult Variable.
        /// </summary>
        public const uint OpcUaFdi5_XmlSchema_RegisterNodesResult = 109;

        /// <summary>
        /// The identifier for the OpcUaFdi5_XmlSchema_TransferIncident Variable.
        /// </summary>
        public const uint OpcUaFdi5_XmlSchema_TransferIncident = 112;

        /// <summary>
        /// The identifier for the OpcUaFdi5_XmlSchema_ApplyResult Variable.
        /// </summary>
        public const uint OpcUaFdi5_XmlSchema_ApplyResult = 115;
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
        /// The identifier for the UIDescriptionType VariableType.
        /// </summary>
        public const uint UIDescriptionType = 1;

        /// <summary>
        /// The identifier for the UIPlugInType VariableType.
        /// </summary>
        public const uint UIPlugInType = 2;
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
        /// The identifier for the WindowModeType DataType.
        /// </summary>
        public static readonly ExpandedNodeId WindowModeType = new ExpandedNodeId(Opc.Ua.Fdi5.DataTypes.WindowModeType, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the StyleType DataType.
        /// </summary>
        public static readonly ExpandedNodeId StyleType = new ExpandedNodeId(Opc.Ua.Fdi5.DataTypes.StyleType, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the RegistrationParameters DataType.
        /// </summary>
        public static readonly ExpandedNodeId RegistrationParameters = new ExpandedNodeId(Opc.Ua.Fdi5.DataTypes.RegistrationParameters, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the RegisteredNode DataType.
        /// </summary>
        public static readonly ExpandedNodeId RegisteredNode = new ExpandedNodeId(Opc.Ua.Fdi5.DataTypes.RegisteredNode, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the RegisterNodesResult DataType.
        /// </summary>
        public static readonly ExpandedNodeId RegisterNodesResult = new ExpandedNodeId(Opc.Ua.Fdi5.DataTypes.RegisterNodesResult, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the TransferIncident DataType.
        /// </summary>
        public static readonly ExpandedNodeId TransferIncident = new ExpandedNodeId(Opc.Ua.Fdi5.DataTypes.TransferIncident, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the ApplyResult DataType.
        /// </summary>
        public static readonly ExpandedNodeId ApplyResult = new ExpandedNodeId(Opc.Ua.Fdi5.DataTypes.ApplyResult, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);
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
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_Open Method.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5NamespaceMetadata_NamespaceFile_Open = new ExpandedNodeId(Opc.Ua.Fdi5.Methods.OpcUaFdi5NamespaceMetadata_NamespaceFile_Open, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_Close Method.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5NamespaceMetadata_NamespaceFile_Close = new ExpandedNodeId(Opc.Ua.Fdi5.Methods.OpcUaFdi5NamespaceMetadata_NamespaceFile_Close, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_Read Method.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5NamespaceMetadata_NamespaceFile_Read = new ExpandedNodeId(Opc.Ua.Fdi5.Methods.OpcUaFdi5NamespaceMetadata_NamespaceFile_Read, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_Write Method.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5NamespaceMetadata_NamespaceFile_Write = new ExpandedNodeId(Opc.Ua.Fdi5.Methods.OpcUaFdi5NamespaceMetadata_NamespaceFile_Write, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_GetPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5NamespaceMetadata_NamespaceFile_GetPosition = new ExpandedNodeId(Opc.Ua.Fdi5.Methods.OpcUaFdi5NamespaceMetadata_NamespaceFile_GetPosition, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_SetPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5NamespaceMetadata_NamespaceFile_SetPosition = new ExpandedNodeId(Opc.Ua.Fdi5.Methods.OpcUaFdi5NamespaceMetadata_NamespaceFile_SetPosition, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the ActionServiceType_InvokeAction Method.
        /// </summary>
        public static readonly ExpandedNodeId ActionServiceType_InvokeAction = new ExpandedNodeId(Opc.Ua.Fdi5.Methods.ActionServiceType_InvokeAction, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the ActionServiceType_RespondAction Method.
        /// </summary>
        public static readonly ExpandedNodeId ActionServiceType_RespondAction = new ExpandedNodeId(Opc.Ua.Fdi5.Methods.ActionServiceType_RespondAction, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the ActionServiceType_AbortAction Method.
        /// </summary>
        public static readonly ExpandedNodeId ActionServiceType_AbortAction = new ExpandedNodeId(Opc.Ua.Fdi5.Methods.ActionServiceType_AbortAction, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the EditContextType_GetEditContext Method.
        /// </summary>
        public static readonly ExpandedNodeId EditContextType_GetEditContext = new ExpandedNodeId(Opc.Ua.Fdi5.Methods.EditContextType_GetEditContext, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the EditContextType_RegisterNodesById Method.
        /// </summary>
        public static readonly ExpandedNodeId EditContextType_RegisterNodesById = new ExpandedNodeId(Opc.Ua.Fdi5.Methods.EditContextType_RegisterNodesById, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the EditContextType_RegisterNodesByRelativePath Method.
        /// </summary>
        public static readonly ExpandedNodeId EditContextType_RegisterNodesByRelativePath = new ExpandedNodeId(Opc.Ua.Fdi5.Methods.EditContextType_RegisterNodesByRelativePath, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the EditContextType_Apply Method.
        /// </summary>
        public static readonly ExpandedNodeId EditContextType_Apply = new ExpandedNodeId(Opc.Ua.Fdi5.Methods.EditContextType_Apply, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the EditContextType_Reset Method.
        /// </summary>
        public static readonly ExpandedNodeId EditContextType_Reset = new ExpandedNodeId(Opc.Ua.Fdi5.Methods.EditContextType_Reset, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the EditContextType_Discard Method.
        /// </summary>
        public static readonly ExpandedNodeId EditContextType_Discard = new ExpandedNodeId(Opc.Ua.Fdi5.Methods.EditContextType_Discard, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the DirectDeviceAccessType_InitDirectAccess Method.
        /// </summary>
        public static readonly ExpandedNodeId DirectDeviceAccessType_InitDirectAccess = new ExpandedNodeId(Opc.Ua.Fdi5.Methods.DirectDeviceAccessType_InitDirectAccess, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the DirectDeviceAccessType_Transfer Method.
        /// </summary>
        public static readonly ExpandedNodeId DirectDeviceAccessType_Transfer = new ExpandedNodeId(Opc.Ua.Fdi5.Methods.DirectDeviceAccessType_Transfer, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the DirectDeviceAccessType_EndDirectAccess Method.
        /// </summary>
        public static readonly ExpandedNodeId DirectDeviceAccessType_EndDirectAccess = new ExpandedNodeId(Opc.Ua.Fdi5.Methods.DirectDeviceAccessType_EndDirectAccess, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the LogAuditTrailMessage Method.
        /// </summary>
        public static readonly ExpandedNodeId LogAuditTrailMessage = new ExpandedNodeId(Opc.Ua.Fdi5.Methods.LogAuditTrailMessage, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the ActionSet_InvokeAction Method.
        /// </summary>
        public static readonly ExpandedNodeId ActionSet_InvokeAction = new ExpandedNodeId(Opc.Ua.Fdi5.Methods.ActionSet_InvokeAction, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the ActionSet_RespondAction Method.
        /// </summary>
        public static readonly ExpandedNodeId ActionSet_RespondAction = new ExpandedNodeId(Opc.Ua.Fdi5.Methods.ActionSet_RespondAction, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the ActionSet_AbortAction Method.
        /// </summary>
        public static readonly ExpandedNodeId ActionSet_AbortAction = new ExpandedNodeId(Opc.Ua.Fdi5.Methods.ActionSet_AbortAction, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);
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
        /// The identifier for the OpcUaFdi5NamespaceMetadata Object.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5NamespaceMetadata = new ExpandedNodeId(Opc.Ua.Fdi5.Objects.OpcUaFdi5NamespaceMetadata, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the UIPlugInType_Documentation Object.
        /// </summary>
        public static readonly ExpandedNodeId UIPlugInType_Documentation = new ExpandedNodeId(Opc.Ua.Fdi5.Objects.UIPlugInType_Documentation, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the ActionServiceType_ActionIdentifier_PH Object.
        /// </summary>
        public static readonly ExpandedNodeId ActionServiceType_ActionIdentifier_PH = new ExpandedNodeId(Opc.Ua.Fdi5.Objects.ActionServiceType_ActionIdentifier_PH, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the ActionIdentifier Object.
        /// </summary>
        public static readonly ExpandedNodeId ActionIdentifier = new ExpandedNodeId(Opc.Ua.Fdi5.Objects.ActionIdentifier, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the ActionSet Object.
        /// </summary>
        public static readonly ExpandedNodeId ActionSet = new ExpandedNodeId(Opc.Ua.Fdi5.Objects.ActionSet, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the RegistrationParameters_Encoding_DefaultBinary Object.
        /// </summary>
        public static readonly ExpandedNodeId RegistrationParameters_Encoding_DefaultBinary = new ExpandedNodeId(Opc.Ua.Fdi5.Objects.RegistrationParameters_Encoding_DefaultBinary, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the RegisteredNode_Encoding_DefaultBinary Object.
        /// </summary>
        public static readonly ExpandedNodeId RegisteredNode_Encoding_DefaultBinary = new ExpandedNodeId(Opc.Ua.Fdi5.Objects.RegisteredNode_Encoding_DefaultBinary, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the RegisterNodesResult_Encoding_DefaultBinary Object.
        /// </summary>
        public static readonly ExpandedNodeId RegisterNodesResult_Encoding_DefaultBinary = new ExpandedNodeId(Opc.Ua.Fdi5.Objects.RegisterNodesResult_Encoding_DefaultBinary, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the TransferIncident_Encoding_DefaultBinary Object.
        /// </summary>
        public static readonly ExpandedNodeId TransferIncident_Encoding_DefaultBinary = new ExpandedNodeId(Opc.Ua.Fdi5.Objects.TransferIncident_Encoding_DefaultBinary, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the ApplyResult_Encoding_DefaultBinary Object.
        /// </summary>
        public static readonly ExpandedNodeId ApplyResult_Encoding_DefaultBinary = new ExpandedNodeId(Opc.Ua.Fdi5.Objects.ApplyResult_Encoding_DefaultBinary, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the RegistrationParameters_Encoding_DefaultXml Object.
        /// </summary>
        public static readonly ExpandedNodeId RegistrationParameters_Encoding_DefaultXml = new ExpandedNodeId(Opc.Ua.Fdi5.Objects.RegistrationParameters_Encoding_DefaultXml, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the RegisteredNode_Encoding_DefaultXml Object.
        /// </summary>
        public static readonly ExpandedNodeId RegisteredNode_Encoding_DefaultXml = new ExpandedNodeId(Opc.Ua.Fdi5.Objects.RegisteredNode_Encoding_DefaultXml, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the RegisterNodesResult_Encoding_DefaultXml Object.
        /// </summary>
        public static readonly ExpandedNodeId RegisterNodesResult_Encoding_DefaultXml = new ExpandedNodeId(Opc.Ua.Fdi5.Objects.RegisterNodesResult_Encoding_DefaultXml, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the TransferIncident_Encoding_DefaultXml Object.
        /// </summary>
        public static readonly ExpandedNodeId TransferIncident_Encoding_DefaultXml = new ExpandedNodeId(Opc.Ua.Fdi5.Objects.TransferIncident_Encoding_DefaultXml, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the ApplyResult_Encoding_DefaultXml Object.
        /// </summary>
        public static readonly ExpandedNodeId ApplyResult_Encoding_DefaultXml = new ExpandedNodeId(Opc.Ua.Fdi5.Objects.ApplyResult_Encoding_DefaultXml, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the RegistrationParameters_Encoding_DefaultJson Object.
        /// </summary>
        public static readonly ExpandedNodeId RegistrationParameters_Encoding_DefaultJson = new ExpandedNodeId(Opc.Ua.Fdi5.Objects.RegistrationParameters_Encoding_DefaultJson, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the RegisteredNode_Encoding_DefaultJson Object.
        /// </summary>
        public static readonly ExpandedNodeId RegisteredNode_Encoding_DefaultJson = new ExpandedNodeId(Opc.Ua.Fdi5.Objects.RegisteredNode_Encoding_DefaultJson, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the RegisterNodesResult_Encoding_DefaultJson Object.
        /// </summary>
        public static readonly ExpandedNodeId RegisterNodesResult_Encoding_DefaultJson = new ExpandedNodeId(Opc.Ua.Fdi5.Objects.RegisterNodesResult_Encoding_DefaultJson, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the TransferIncident_Encoding_DefaultJson Object.
        /// </summary>
        public static readonly ExpandedNodeId TransferIncident_Encoding_DefaultJson = new ExpandedNodeId(Opc.Ua.Fdi5.Objects.TransferIncident_Encoding_DefaultJson, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the ApplyResult_Encoding_DefaultJson Object.
        /// </summary>
        public static readonly ExpandedNodeId ApplyResult_Encoding_DefaultJson = new ExpandedNodeId(Opc.Ua.Fdi5.Objects.ApplyResult_Encoding_DefaultJson, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);
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
        /// The identifier for the ActionType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ActionType = new ExpandedNodeId(Opc.Ua.Fdi5.ObjectTypes.ActionType, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the ActionServiceType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ActionServiceType = new ExpandedNodeId(Opc.Ua.Fdi5.ObjectTypes.ActionServiceType, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the EditContextType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId EditContextType = new ExpandedNodeId(Opc.Ua.Fdi5.ObjectTypes.EditContextType, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the DirectDeviceAccessType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId DirectDeviceAccessType = new ExpandedNodeId(Opc.Ua.Fdi5.ObjectTypes.DirectDeviceAccessType, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);
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
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5NamespaceMetadata_NamespaceUri = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5NamespaceMetadata_NamespaceUri, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceVersion Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5NamespaceMetadata_NamespaceVersion = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5NamespaceMetadata_NamespaceVersion, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespacePublicationDate Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5NamespaceMetadata_NamespacePublicationDate = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5NamespaceMetadata_NamespacePublicationDate, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_IsNamespaceSubset Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5NamespaceMetadata_IsNamespaceSubset = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5NamespaceMetadata_IsNamespaceSubset, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_StaticNodeIdTypes Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5NamespaceMetadata_StaticNodeIdTypes = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5NamespaceMetadata_StaticNodeIdTypes, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_StaticNumericNodeIdRange Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5NamespaceMetadata_StaticNumericNodeIdRange = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5NamespaceMetadata_StaticNumericNodeIdRange, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_StaticStringNodeIdPattern Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5NamespaceMetadata_StaticStringNodeIdPattern = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5NamespaceMetadata_StaticStringNodeIdPattern, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_Size Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5NamespaceMetadata_NamespaceFile_Size = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5NamespaceMetadata_NamespaceFile_Size, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_Writable Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5NamespaceMetadata_NamespaceFile_Writable = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5NamespaceMetadata_NamespaceFile_Writable, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_UserWritable Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5NamespaceMetadata_NamespaceFile_UserWritable = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5NamespaceMetadata_NamespaceFile_UserWritable, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_OpenCount Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5NamespaceMetadata_NamespaceFile_OpenCount = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5NamespaceMetadata_NamespaceFile_OpenCount, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_Open_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5NamespaceMetadata_NamespaceFile_Open_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5NamespaceMetadata_NamespaceFile_Open_InputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_Open_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5NamespaceMetadata_NamespaceFile_Open_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5NamespaceMetadata_NamespaceFile_Open_OutputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_Close_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5NamespaceMetadata_NamespaceFile_Close_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5NamespaceMetadata_NamespaceFile_Close_InputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_Read_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5NamespaceMetadata_NamespaceFile_Read_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5NamespaceMetadata_NamespaceFile_Read_InputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_Read_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5NamespaceMetadata_NamespaceFile_Read_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5NamespaceMetadata_NamespaceFile_Read_OutputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_Write_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5NamespaceMetadata_NamespaceFile_Write_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5NamespaceMetadata_NamespaceFile_Write_InputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_GetPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5NamespaceMetadata_NamespaceFile_GetPosition_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5NamespaceMetadata_NamespaceFile_GetPosition_InputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_GetPosition_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5NamespaceMetadata_NamespaceFile_GetPosition_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5NamespaceMetadata_NamespaceFile_GetPosition_OutputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_NamespaceFile_SetPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5NamespaceMetadata_NamespaceFile_SetPosition_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5NamespaceMetadata_NamespaceFile_SetPosition_InputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_DefaultRolePermissions Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5NamespaceMetadata_DefaultRolePermissions = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5NamespaceMetadata_DefaultRolePermissions, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_DefaultUserRolePermissions Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5NamespaceMetadata_DefaultUserRolePermissions = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5NamespaceMetadata_DefaultUserRolePermissions, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5NamespaceMetadata_DefaultAccessRestrictions Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5NamespaceMetadata_DefaultAccessRestrictions = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5NamespaceMetadata_DefaultAccessRestrictions, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the UIPlugInType_UIPVariantVersion Variable.
        /// </summary>
        public static readonly ExpandedNodeId UIPlugInType_UIPVariantVersion = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.UIPlugInType_UIPVariantVersion, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the UIPlugInType_FDITechnologyVersion Variable.
        /// </summary>
        public static readonly ExpandedNodeId UIPlugInType_FDITechnologyVersion = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.UIPlugInType_FDITechnologyVersion, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the UIPlugInType_RuntimeId Variable.
        /// </summary>
        public static readonly ExpandedNodeId UIPlugInType_RuntimeId = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.UIPlugInType_RuntimeId, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the UIPlugInType_CpuInformation Variable.
        /// </summary>
        public static readonly ExpandedNodeId UIPlugInType_CpuInformation = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.UIPlugInType_CpuInformation, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the UIPlugInType_PlatformId Variable.
        /// </summary>
        public static readonly ExpandedNodeId UIPlugInType_PlatformId = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.UIPlugInType_PlatformId, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the UIPlugInType_Style Variable.
        /// </summary>
        public static readonly ExpandedNodeId UIPlugInType_Style = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.UIPlugInType_Style, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the UIPlugInType_StartElementName Variable.
        /// </summary>
        public static readonly ExpandedNodeId UIPlugInType_StartElementName = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.UIPlugInType_StartElementName, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the ActionServiceType_InvokeAction_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ActionServiceType_InvokeAction_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.ActionServiceType_InvokeAction_InputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the ActionServiceType_InvokeAction_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ActionServiceType_InvokeAction_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.ActionServiceType_InvokeAction_OutputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the ActionServiceType_RespondAction_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ActionServiceType_RespondAction_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.ActionServiceType_RespondAction_InputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the ActionServiceType_RespondAction_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ActionServiceType_RespondAction_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.ActionServiceType_RespondAction_OutputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the ActionServiceType_AbortAction_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ActionServiceType_AbortAction_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.ActionServiceType_AbortAction_InputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the ActionServiceType_AbortAction_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ActionServiceType_AbortAction_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.ActionServiceType_AbortAction_OutputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the WindowModeType_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId WindowModeType_EnumValues = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.WindowModeType_EnumValues, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the StyleType_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId StyleType_EnumValues = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.StyleType_EnumValues, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the EditContextType_GetEditContext_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId EditContextType_GetEditContext_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.EditContextType_GetEditContext_InputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the EditContextType_GetEditContext_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId EditContextType_GetEditContext_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.EditContextType_GetEditContext_OutputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the EditContextType_RegisterNodesById_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId EditContextType_RegisterNodesById_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.EditContextType_RegisterNodesById_InputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the EditContextType_RegisterNodesById_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId EditContextType_RegisterNodesById_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.EditContextType_RegisterNodesById_OutputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the EditContextType_RegisterNodesByRelativePath_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId EditContextType_RegisterNodesByRelativePath_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.EditContextType_RegisterNodesByRelativePath_InputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the EditContextType_RegisterNodesByRelativePath_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId EditContextType_RegisterNodesByRelativePath_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.EditContextType_RegisterNodesByRelativePath_OutputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the EditContextType_Apply_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId EditContextType_Apply_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.EditContextType_Apply_InputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the EditContextType_Apply_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId EditContextType_Apply_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.EditContextType_Apply_OutputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the EditContextType_Reset_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId EditContextType_Reset_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.EditContextType_Reset_InputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the EditContextType_Reset_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId EditContextType_Reset_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.EditContextType_Reset_OutputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the EditContextType_Discard_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId EditContextType_Discard_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.EditContextType_Discard_InputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the EditContextType_Discard_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId EditContextType_Discard_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.EditContextType_Discard_OutputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the DirectDeviceAccessType_InitDirectAccess_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectDeviceAccessType_InitDirectAccess_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.DirectDeviceAccessType_InitDirectAccess_InputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the DirectDeviceAccessType_InitDirectAccess_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectDeviceAccessType_InitDirectAccess_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.DirectDeviceAccessType_InitDirectAccess_OutputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the DirectDeviceAccessType_Transfer_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectDeviceAccessType_Transfer_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.DirectDeviceAccessType_Transfer_InputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the DirectDeviceAccessType_Transfer_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectDeviceAccessType_Transfer_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.DirectDeviceAccessType_Transfer_OutputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the DirectDeviceAccessType_EndDirectAccess_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectDeviceAccessType_EndDirectAccess_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.DirectDeviceAccessType_EndDirectAccess_InputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the DirectDeviceAccessType_EndDirectAccess_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectDeviceAccessType_EndDirectAccess_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.DirectDeviceAccessType_EndDirectAccess_OutputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the LogAuditTrailMessage_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId LogAuditTrailMessage_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.LogAuditTrailMessage_InputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the FDIServerVersion Variable.
        /// </summary>
        public static readonly ExpandedNodeId FDIServerVersion = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.FDIServerVersion, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the DeviceHealthDiagnostics Variable.
        /// </summary>
        public static readonly ExpandedNodeId DeviceHealthDiagnostics = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.DeviceHealthDiagnostics, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the ActionSet_InvokeAction_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ActionSet_InvokeAction_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.ActionSet_InvokeAction_InputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the ActionSet_InvokeAction_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ActionSet_InvokeAction_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.ActionSet_InvokeAction_OutputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the ActionSet_RespondAction_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ActionSet_RespondAction_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.ActionSet_RespondAction_InputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the ActionSet_RespondAction_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ActionSet_RespondAction_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.ActionSet_RespondAction_OutputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the ActionSet_AbortAction_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ActionSet_AbortAction_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.ActionSet_AbortAction_InputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the ActionSet_AbortAction_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ActionSet_AbortAction_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.ActionSet_AbortAction_OutputArguments, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5_BinarySchema Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5_BinarySchema = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5_BinarySchema, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5_BinarySchema_NamespaceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5_BinarySchema_NamespaceUri = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5_BinarySchema_NamespaceUri, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5_BinarySchema_Deprecated Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5_BinarySchema_Deprecated = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5_BinarySchema_Deprecated, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5_BinarySchema_RegistrationParameters Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5_BinarySchema_RegistrationParameters = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5_BinarySchema_RegistrationParameters, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5_BinarySchema_RegisteredNode Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5_BinarySchema_RegisteredNode = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5_BinarySchema_RegisteredNode, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5_BinarySchema_RegisterNodesResult Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5_BinarySchema_RegisterNodesResult = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5_BinarySchema_RegisterNodesResult, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5_BinarySchema_TransferIncident Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5_BinarySchema_TransferIncident = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5_BinarySchema_TransferIncident, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5_BinarySchema_ApplyResult Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5_BinarySchema_ApplyResult = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5_BinarySchema_ApplyResult, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5_XmlSchema Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5_XmlSchema = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5_XmlSchema, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5_XmlSchema_NamespaceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5_XmlSchema_NamespaceUri = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5_XmlSchema_NamespaceUri, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5_XmlSchema_Deprecated Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5_XmlSchema_Deprecated = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5_XmlSchema_Deprecated, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5_XmlSchema_RegistrationParameters Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5_XmlSchema_RegistrationParameters = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5_XmlSchema_RegistrationParameters, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5_XmlSchema_RegisteredNode Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5_XmlSchema_RegisteredNode = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5_XmlSchema_RegisteredNode, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5_XmlSchema_RegisterNodesResult Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5_XmlSchema_RegisterNodesResult = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5_XmlSchema_RegisterNodesResult, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5_XmlSchema_TransferIncident Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5_XmlSchema_TransferIncident = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5_XmlSchema_TransferIncident, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the OpcUaFdi5_XmlSchema_ApplyResult Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi5_XmlSchema_ApplyResult = new ExpandedNodeId(Opc.Ua.Fdi5.Variables.OpcUaFdi5_XmlSchema_ApplyResult, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);
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
        /// The identifier for the UIDescriptionType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId UIDescriptionType = new ExpandedNodeId(Opc.Ua.Fdi5.VariableTypes.UIDescriptionType, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);

        /// <summary>
        /// The identifier for the UIPlugInType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId UIPlugInType = new ExpandedNodeId(Opc.Ua.Fdi5.VariableTypes.UIPlugInType, Opc.Ua.Fdi5.Namespaces.OpcUaFdi5);
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
        /// The BrowseName for the AbortAction component.
        /// </summary>
        public const string AbortAction = "AbortAction";

        /// <summary>
        /// The BrowseName for the ActionIdentifier component.
        /// </summary>
        public const string ActionIdentifier = "ActionIdentifier";

        /// <summary>
        /// The BrowseName for the ActionIdentifier_PH component.
        /// </summary>
        public const string ActionIdentifier_PH = "<ActionIdentifier>";

        /// <summary>
        /// The BrowseName for the ActionServiceType component.
        /// </summary>
        public const string ActionServiceType = "ActionServiceType";

        /// <summary>
        /// The BrowseName for the ActionSet component.
        /// </summary>
        public const string ActionSet = "ActionSet";

        /// <summary>
        /// The BrowseName for the ActionType component.
        /// </summary>
        public const string ActionType = "ActionType";

        /// <summary>
        /// The BrowseName for the Apply component.
        /// </summary>
        public const string Apply = "Apply";

        /// <summary>
        /// The BrowseName for the ApplyResult component.
        /// </summary>
        public const string ApplyResult = "ApplyResult";

        /// <summary>
        /// The BrowseName for the CpuInformation component.
        /// </summary>
        public const string CpuInformation = "CpuInformation";

        /// <summary>
        /// The BrowseName for the DeviceHealthDiagnostics component.
        /// </summary>
        public const string DeviceHealthDiagnostics = "DeviceHealthDiagnostics";

        /// <summary>
        /// The BrowseName for the DirectDeviceAccessType component.
        /// </summary>
        public const string DirectDeviceAccessType = "DirectDeviceAccessType";

        /// <summary>
        /// The BrowseName for the Discard component.
        /// </summary>
        public const string Discard = "Discard";

        /// <summary>
        /// The BrowseName for the Documentation component.
        /// </summary>
        public const string Documentation = "Documentation";

        /// <summary>
        /// The BrowseName for the EditContextType component.
        /// </summary>
        public const string EditContextType = "EditContextType";

        /// <summary>
        /// The BrowseName for the EndDirectAccess component.
        /// </summary>
        public const string EndDirectAccess = "EndDirectAccess";

        /// <summary>
        /// The BrowseName for the FDIServerVersion component.
        /// </summary>
        public const string FDIServerVersion = "FDIServerVersion";

        /// <summary>
        /// The BrowseName for the FDITechnologyVersion component.
        /// </summary>
        public const string FDITechnologyVersion = "FDITechnologyVersion";

        /// <summary>
        /// The BrowseName for the GetEditContext component.
        /// </summary>
        public const string GetEditContext = "GetEditContext";

        /// <summary>
        /// The BrowseName for the InitDirectAccess component.
        /// </summary>
        public const string InitDirectAccess = "InitDirectAccess";

        /// <summary>
        /// The BrowseName for the InvokeAction component.
        /// </summary>
        public const string InvokeAction = "InvokeAction";

        /// <summary>
        /// The BrowseName for the LogAuditTrailMessage component.
        /// </summary>
        public const string LogAuditTrailMessage = "LogAuditTrailMessage";

        /// <summary>
        /// The BrowseName for the OpcUaFdi5_BinarySchema component.
        /// </summary>
        public const string OpcUaFdi5_BinarySchema = "Opc.Ua.Fdi5";

        /// <summary>
        /// The BrowseName for the OpcUaFdi5_XmlSchema component.
        /// </summary>
        public const string OpcUaFdi5_XmlSchema = "Opc.Ua.Fdi5";

        /// <summary>
        /// The BrowseName for the OpcUaFdi5NamespaceMetadata component.
        /// </summary>
        public const string OpcUaFdi5NamespaceMetadata = "http://fdi-cooperation.com/OPCUA/FDI5/";

        /// <summary>
        /// The BrowseName for the PlatformId component.
        /// </summary>
        public const string PlatformId = "PlatformId";

        /// <summary>
        /// The BrowseName for the RegisteredNode component.
        /// </summary>
        public const string RegisteredNode = "RegisteredNode";

        /// <summary>
        /// The BrowseName for the RegisterNodesById component.
        /// </summary>
        public const string RegisterNodesById = "RegisterNodesById";

        /// <summary>
        /// The BrowseName for the RegisterNodesByRelativePath component.
        /// </summary>
        public const string RegisterNodesByRelativePath = "RegisterNodesByRelativePath";

        /// <summary>
        /// The BrowseName for the RegisterNodesResult component.
        /// </summary>
        public const string RegisterNodesResult = "RegisterNodesResult";

        /// <summary>
        /// The BrowseName for the RegistrationParameters component.
        /// </summary>
        public const string RegistrationParameters = "RegistrationParameters";

        /// <summary>
        /// The BrowseName for the Reset component.
        /// </summary>
        public const string Reset = "Reset";

        /// <summary>
        /// The BrowseName for the RespondAction component.
        /// </summary>
        public const string RespondAction = "RespondAction";

        /// <summary>
        /// The BrowseName for the RuntimeId component.
        /// </summary>
        public const string RuntimeId = "RuntimeId";

        /// <summary>
        /// The BrowseName for the StartElementName component.
        /// </summary>
        public const string StartElementName = "StartElementName";

        /// <summary>
        /// The BrowseName for the Style component.
        /// </summary>
        public const string Style = "Style";

        /// <summary>
        /// The BrowseName for the StyleType component.
        /// </summary>
        public const string StyleType = "StyleType";

        /// <summary>
        /// The BrowseName for the Transfer component.
        /// </summary>
        public const string Transfer = "Transfer";

        /// <summary>
        /// The BrowseName for the TransferIncident component.
        /// </summary>
        public const string TransferIncident = "TransferIncident";

        /// <summary>
        /// The BrowseName for the UIDescriptionType component.
        /// </summary>
        public const string UIDescriptionType = "UIDescriptionType";

        /// <summary>
        /// The BrowseName for the UIPlugInType component.
        /// </summary>
        public const string UIPlugInType = "UIPlugInType";

        /// <summary>
        /// The BrowseName for the UIPVariantVersion component.
        /// </summary>
        public const string UIPVariantVersion = "UIPVariantVersion";

        /// <summary>
        /// The BrowseName for the WindowModeType component.
        /// </summary>
        public const string WindowModeType = "WindowModeType";
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
        /// The URI for the OpcUaFdi5 namespace (.NET code namespace is 'Opc.Ua.Fdi5').
        /// </summary>
        public const string OpcUaFdi5 = "http://fdi-cooperation.com/OPCUA/FDI5/";

        /// <summary>
        /// The URI for the OpcUaFdi5Xsd namespace (.NET code namespace is 'Opc.Ua.Fdi5').
        /// </summary>
        public const string OpcUaFdi5Xsd = "http://fdi-cooperation.com/OPCUA/FDI5/Types.xsd";

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
