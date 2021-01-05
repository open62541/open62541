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

namespace Opc.Ua.Gds
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
        /// The identifier for the ApplicationRecordDataType DataType.
        /// </summary>
        public const uint ApplicationRecordDataType = 1;
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
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_Open Method.
        /// </summary>
        public const uint OPCUAGDSNamespaceMetadata_NamespaceFile_Open = 735;

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_Close Method.
        /// </summary>
        public const uint OPCUAGDSNamespaceMetadata_NamespaceFile_Close = 738;

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_Read Method.
        /// </summary>
        public const uint OPCUAGDSNamespaceMetadata_NamespaceFile_Read = 740;

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_Write Method.
        /// </summary>
        public const uint OPCUAGDSNamespaceMetadata_NamespaceFile_Write = 743;

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_GetPosition Method.
        /// </summary>
        public const uint OPCUAGDSNamespaceMetadata_NamespaceFile_GetPosition = 745;

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_SetPosition Method.
        /// </summary>
        public const uint OPCUAGDSNamespaceMetadata_NamespaceFile_SetPosition = 748;

        /// <summary>
        /// The identifier for the DirectoryType_FindApplications Method.
        /// </summary>
        public const uint DirectoryType_FindApplications = 15;

        /// <summary>
        /// The identifier for the DirectoryType_RegisterApplication Method.
        /// </summary>
        public const uint DirectoryType_RegisterApplication = 18;

        /// <summary>
        /// The identifier for the DirectoryType_UpdateApplication Method.
        /// </summary>
        public const uint DirectoryType_UpdateApplication = 188;

        /// <summary>
        /// The identifier for the DirectoryType_UnregisterApplication Method.
        /// </summary>
        public const uint DirectoryType_UnregisterApplication = 21;

        /// <summary>
        /// The identifier for the DirectoryType_GetApplication Method.
        /// </summary>
        public const uint DirectoryType_GetApplication = 210;

        /// <summary>
        /// The identifier for the DirectoryType_QueryApplications Method.
        /// </summary>
        public const uint DirectoryType_QueryApplications = 868;

        /// <summary>
        /// The identifier for the DirectoryType_QueryServers Method.
        /// </summary>
        public const uint DirectoryType_QueryServers = 23;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Open Method.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Open = 519;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Close Method.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Close = 522;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Read Method.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Read = 524;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Write Method.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Write = 527;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_GetPosition Method.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_GetPosition = 529;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_SetPosition Method.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_SetPosition = 532;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_OpenWithMasks Method.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_OpenWithMasks = 535;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Open Method.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Open = 553;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Close Method.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Close = 556;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Read Method.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Read = 558;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Write Method.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Write = 561;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_GetPosition Method.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_GetPosition = 563;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_SetPosition Method.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_SetPosition = 566;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_OpenWithMasks Method.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_OpenWithMasks = 569;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Open Method.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Open = 587;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Close Method.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Close = 590;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Read Method.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Read = 592;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Write Method.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Write = 595;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_GetPosition Method.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_GetPosition = 597;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_SetPosition Method.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_SetPosition = 600;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenWithMasks Method.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenWithMasks = 603;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_StartSigningRequest Method.
        /// </summary>
        public const uint CertificateDirectoryType_StartSigningRequest = 79;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_StartNewKeyPairRequest Method.
        /// </summary>
        public const uint CertificateDirectoryType_StartNewKeyPairRequest = 76;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_FinishRequest Method.
        /// </summary>
        public const uint CertificateDirectoryType_FinishRequest = 85;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_RevokeCertificate Method.
        /// </summary>
        public const uint CertificateDirectoryType_RevokeCertificate = 15003;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_GetCertificateGroups Method.
        /// </summary>
        public const uint CertificateDirectoryType_GetCertificateGroups = 369;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_GetTrustList Method.
        /// </summary>
        public const uint CertificateDirectoryType_GetTrustList = 197;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_GetCertificateStatus Method.
        /// </summary>
        public const uint CertificateDirectoryType_GetCertificateStatus = 222;

        /// <summary>
        /// The identifier for the KeyCredentialManagement_ServiceName_Placeholder_StartRequest Method.
        /// </summary>
        public const uint KeyCredentialManagement_ServiceName_Placeholder_StartRequest = 1012;

        /// <summary>
        /// The identifier for the KeyCredentialManagement_ServiceName_Placeholder_FinishRequest Method.
        /// </summary>
        public const uint KeyCredentialManagement_ServiceName_Placeholder_FinishRequest = 1015;

        /// <summary>
        /// The identifier for the KeyCredentialServiceType_StartRequest Method.
        /// </summary>
        public const uint KeyCredentialServiceType_StartRequest = 1023;

        /// <summary>
        /// The identifier for the KeyCredentialServiceType_FinishRequest Method.
        /// </summary>
        public const uint KeyCredentialServiceType_FinishRequest = 1026;

        /// <summary>
        /// The identifier for the KeyCredentialServiceType_Revoke Method.
        /// </summary>
        public const uint KeyCredentialServiceType_Revoke = 1029;

        /// <summary>
        /// The identifier for the AuthorizationServices_ServiceName_Placeholder_GetServiceDescription Method.
        /// </summary>
        public const uint AuthorizationServices_ServiceName_Placeholder_GetServiceDescription = 1001;

        /// <summary>
        /// The identifier for the AuthorizationServiceType_GetServiceDescription Method.
        /// </summary>
        public const uint AuthorizationServiceType_GetServiceDescription = 1004;

        /// <summary>
        /// The identifier for the AuthorizationServiceType_RequestAccessToken Method.
        /// </summary>
        public const uint AuthorizationServiceType_RequestAccessToken = 969;

        /// <summary>
        /// The identifier for the Directory_FindApplications Method.
        /// </summary>
        public const uint Directory_FindApplications = 143;

        /// <summary>
        /// The identifier for the Directory_RegisterApplication Method.
        /// </summary>
        public const uint Directory_RegisterApplication = 146;

        /// <summary>
        /// The identifier for the Directory_UpdateApplication Method.
        /// </summary>
        public const uint Directory_UpdateApplication = 200;

        /// <summary>
        /// The identifier for the Directory_UnregisterApplication Method.
        /// </summary>
        public const uint Directory_UnregisterApplication = 149;

        /// <summary>
        /// The identifier for the Directory_GetApplication Method.
        /// </summary>
        public const uint Directory_GetApplication = 216;

        /// <summary>
        /// The identifier for the Directory_QueryApplications Method.
        /// </summary>
        public const uint Directory_QueryApplications = 992;

        /// <summary>
        /// The identifier for the Directory_QueryServers Method.
        /// </summary>
        public const uint Directory_QueryServers = 151;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Open Method.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Open = 622;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Close Method.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Close = 625;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Read Method.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Read = 627;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Write Method.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Write = 630;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_GetPosition Method.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList_GetPosition = 632;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_SetPosition Method.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList_SetPosition = 635;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_OpenWithMasks Method.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList_OpenWithMasks = 638;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_CloseAndUpdate Method.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList_CloseAndUpdate = 641;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_AddCertificate Method.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList_AddCertificate = 644;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_RemoveCertificate Method.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList_RemoveCertificate = 646;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Open Method.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Open = 656;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Close Method.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Close = 659;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Read Method.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Read = 661;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Write Method.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Write = 664;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_GetPosition Method.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList_GetPosition = 666;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_SetPosition Method.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList_SetPosition = 669;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_OpenWithMasks Method.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList_OpenWithMasks = 672;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_CloseAndUpdate Method.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList_CloseAndUpdate = 675;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_AddCertificate Method.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList_AddCertificate = 678;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_RemoveCertificate Method.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList_RemoveCertificate = 680;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Open Method.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Open = 690;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Close Method.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Close = 693;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Read Method.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Read = 695;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Write Method.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Write = 698;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_GetPosition Method.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_GetPosition = 700;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_SetPosition Method.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_SetPosition = 703;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenWithMasks Method.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenWithMasks = 706;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_CloseAndUpdate Method.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_CloseAndUpdate = 709;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_AddCertificate Method.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_AddCertificate = 712;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_RemoveCertificate Method.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_RemoveCertificate = 714;

        /// <summary>
        /// The identifier for the Directory_StartSigningRequest Method.
        /// </summary>
        public const uint Directory_StartSigningRequest = 157;

        /// <summary>
        /// The identifier for the Directory_StartNewKeyPairRequest Method.
        /// </summary>
        public const uint Directory_StartNewKeyPairRequest = 154;

        /// <summary>
        /// The identifier for the Directory_FinishRequest Method.
        /// </summary>
        public const uint Directory_FinishRequest = 163;

        /// <summary>
        /// The identifier for the Directory_GetCertificateGroups Method.
        /// </summary>
        public const uint Directory_GetCertificateGroups = 508;

        /// <summary>
        /// The identifier for the Directory_GetTrustList Method.
        /// </summary>
        public const uint Directory_GetTrustList = 204;

        /// <summary>
        /// The identifier for the Directory_GetCertificateStatus Method.
        /// </summary>
        public const uint Directory_GetCertificateStatus = 225;
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
        /// The identifier for the OPCUAGDSNamespaceMetadata Object.
        /// </summary>
        public const uint OPCUAGDSNamespaceMetadata = 721;

        /// <summary>
        /// The identifier for the DirectoryType_Applications Object.
        /// </summary>
        public const uint DirectoryType_Applications = 14;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups Object.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups = 511;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup Object.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup = 512;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList Object.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList = 513;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList Object.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList = 547;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList Object.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList = 581;

        /// <summary>
        /// The identifier for the KeyCredentialManagement Object.
        /// </summary>
        public const uint KeyCredentialManagement = 1008;

        /// <summary>
        /// The identifier for the AuthorizationServices Object.
        /// </summary>
        public const uint AuthorizationServices = 959;

        /// <summary>
        /// The identifier for the Directory Object.
        /// </summary>
        public const uint Directory = 141;

        /// <summary>
        /// The identifier for the Directory_Applications Object.
        /// </summary>
        public const uint Directory_Applications = 142;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups Object.
        /// </summary>
        public const uint Directory_CertificateGroups = 614;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup Object.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup = 615;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList Object.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList = 616;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup Object.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup = 649;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList Object.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList = 650;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup Object.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup = 683;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList Object.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList = 684;

        /// <summary>
        /// The identifier for the ApplicationRecordDataType_Encoding_DefaultBinary Object.
        /// </summary>
        public const uint ApplicationRecordDataType_Encoding_DefaultBinary = 134;

        /// <summary>
        /// The identifier for the ApplicationRecordDataType_Encoding_DefaultXml Object.
        /// </summary>
        public const uint ApplicationRecordDataType_Encoding_DefaultXml = 127;

        /// <summary>
        /// The identifier for the ApplicationRecordDataType_Encoding_DefaultJson Object.
        /// </summary>
        public const uint ApplicationRecordDataType_Encoding_DefaultJson = 8001;
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
        /// The identifier for the DirectoryType ObjectType.
        /// </summary>
        public const uint DirectoryType = 13;

        /// <summary>
        /// The identifier for the ApplicationRegistrationChangedAuditEventType ObjectType.
        /// </summary>
        public const uint ApplicationRegistrationChangedAuditEventType = 26;

        /// <summary>
        /// The identifier for the CertificateDirectoryType ObjectType.
        /// </summary>
        public const uint CertificateDirectoryType = 63;

        /// <summary>
        /// The identifier for the CertificateRequestedAuditEventType ObjectType.
        /// </summary>
        public const uint CertificateRequestedAuditEventType = 91;

        /// <summary>
        /// The identifier for the CertificateDeliveredAuditEventType ObjectType.
        /// </summary>
        public const uint CertificateDeliveredAuditEventType = 109;

        /// <summary>
        /// The identifier for the KeyCredentialServiceType ObjectType.
        /// </summary>
        public const uint KeyCredentialServiceType = 1020;

        /// <summary>
        /// The identifier for the KeyCredentialRequestedAuditEventType ObjectType.
        /// </summary>
        public const uint KeyCredentialRequestedAuditEventType = 1039;

        /// <summary>
        /// The identifier for the KeyCredentialDeliveredAuditEventType ObjectType.
        /// </summary>
        public const uint KeyCredentialDeliveredAuditEventType = 1057;

        /// <summary>
        /// The identifier for the KeyCredentialRevokedAuditEventType ObjectType.
        /// </summary>
        public const uint KeyCredentialRevokedAuditEventType = 1075;

        /// <summary>
        /// The identifier for the AuthorizationServiceType ObjectType.
        /// </summary>
        public const uint AuthorizationServiceType = 966;

        /// <summary>
        /// The identifier for the AccessTokenAuditIssuedAuditEventType ObjectType.
        /// </summary>
        public const uint AccessTokenAuditIssuedAuditEventType = 975;
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
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceUri Variable.
        /// </summary>
        public const uint OPCUAGDSNamespaceMetadata_NamespaceUri = 722;

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceVersion Variable.
        /// </summary>
        public const uint OPCUAGDSNamespaceMetadata_NamespaceVersion = 723;

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespacePublicationDate Variable.
        /// </summary>
        public const uint OPCUAGDSNamespaceMetadata_NamespacePublicationDate = 724;

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_IsNamespaceSubset Variable.
        /// </summary>
        public const uint OPCUAGDSNamespaceMetadata_IsNamespaceSubset = 725;

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_StaticNodeIdTypes Variable.
        /// </summary>
        public const uint OPCUAGDSNamespaceMetadata_StaticNodeIdTypes = 726;

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_StaticNumericNodeIdRange Variable.
        /// </summary>
        public const uint OPCUAGDSNamespaceMetadata_StaticNumericNodeIdRange = 727;

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_StaticStringNodeIdPattern Variable.
        /// </summary>
        public const uint OPCUAGDSNamespaceMetadata_StaticStringNodeIdPattern = 728;

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_Size Variable.
        /// </summary>
        public const uint OPCUAGDSNamespaceMetadata_NamespaceFile_Size = 730;

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_Writable Variable.
        /// </summary>
        public const uint OPCUAGDSNamespaceMetadata_NamespaceFile_Writable = 731;

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_UserWritable Variable.
        /// </summary>
        public const uint OPCUAGDSNamespaceMetadata_NamespaceFile_UserWritable = 732;

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_OpenCount Variable.
        /// </summary>
        public const uint OPCUAGDSNamespaceMetadata_NamespaceFile_OpenCount = 733;

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_Open_InputArguments Variable.
        /// </summary>
        public const uint OPCUAGDSNamespaceMetadata_NamespaceFile_Open_InputArguments = 736;

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_Open_OutputArguments Variable.
        /// </summary>
        public const uint OPCUAGDSNamespaceMetadata_NamespaceFile_Open_OutputArguments = 737;

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_Close_InputArguments Variable.
        /// </summary>
        public const uint OPCUAGDSNamespaceMetadata_NamespaceFile_Close_InputArguments = 739;

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_Read_InputArguments Variable.
        /// </summary>
        public const uint OPCUAGDSNamespaceMetadata_NamespaceFile_Read_InputArguments = 741;

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_Read_OutputArguments Variable.
        /// </summary>
        public const uint OPCUAGDSNamespaceMetadata_NamespaceFile_Read_OutputArguments = 742;

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_Write_InputArguments Variable.
        /// </summary>
        public const uint OPCUAGDSNamespaceMetadata_NamespaceFile_Write_InputArguments = 744;

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_GetPosition_InputArguments Variable.
        /// </summary>
        public const uint OPCUAGDSNamespaceMetadata_NamespaceFile_GetPosition_InputArguments = 746;

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_GetPosition_OutputArguments Variable.
        /// </summary>
        public const uint OPCUAGDSNamespaceMetadata_NamespaceFile_GetPosition_OutputArguments = 747;

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_SetPosition_InputArguments Variable.
        /// </summary>
        public const uint OPCUAGDSNamespaceMetadata_NamespaceFile_SetPosition_InputArguments = 749;

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_DefaultRolePermissions Variable.
        /// </summary>
        public const uint OPCUAGDSNamespaceMetadata_DefaultRolePermissions = 862;

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_DefaultUserRolePermissions Variable.
        /// </summary>
        public const uint OPCUAGDSNamespaceMetadata_DefaultUserRolePermissions = 863;

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_DefaultAccessRestrictions Variable.
        /// </summary>
        public const uint OPCUAGDSNamespaceMetadata_DefaultAccessRestrictions = 864;

        /// <summary>
        /// The identifier for the DirectoryType_FindApplications_InputArguments Variable.
        /// </summary>
        public const uint DirectoryType_FindApplications_InputArguments = 16;

        /// <summary>
        /// The identifier for the DirectoryType_FindApplications_OutputArguments Variable.
        /// </summary>
        public const uint DirectoryType_FindApplications_OutputArguments = 17;

        /// <summary>
        /// The identifier for the DirectoryType_RegisterApplication_InputArguments Variable.
        /// </summary>
        public const uint DirectoryType_RegisterApplication_InputArguments = 19;

        /// <summary>
        /// The identifier for the DirectoryType_RegisterApplication_OutputArguments Variable.
        /// </summary>
        public const uint DirectoryType_RegisterApplication_OutputArguments = 20;

        /// <summary>
        /// The identifier for the DirectoryType_UpdateApplication_InputArguments Variable.
        /// </summary>
        public const uint DirectoryType_UpdateApplication_InputArguments = 189;

        /// <summary>
        /// The identifier for the DirectoryType_UnregisterApplication_InputArguments Variable.
        /// </summary>
        public const uint DirectoryType_UnregisterApplication_InputArguments = 22;

        /// <summary>
        /// The identifier for the DirectoryType_GetApplication_InputArguments Variable.
        /// </summary>
        public const uint DirectoryType_GetApplication_InputArguments = 211;

        /// <summary>
        /// The identifier for the DirectoryType_GetApplication_OutputArguments Variable.
        /// </summary>
        public const uint DirectoryType_GetApplication_OutputArguments = 212;

        /// <summary>
        /// The identifier for the DirectoryType_QueryApplications_InputArguments Variable.
        /// </summary>
        public const uint DirectoryType_QueryApplications_InputArguments = 869;

        /// <summary>
        /// The identifier for the DirectoryType_QueryApplications_OutputArguments Variable.
        /// </summary>
        public const uint DirectoryType_QueryApplications_OutputArguments = 870;

        /// <summary>
        /// The identifier for the DirectoryType_QueryServers_InputArguments Variable.
        /// </summary>
        public const uint DirectoryType_QueryServers_InputArguments = 24;

        /// <summary>
        /// The identifier for the DirectoryType_QueryServers_OutputArguments Variable.
        /// </summary>
        public const uint DirectoryType_QueryServers_OutputArguments = 25;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_FindApplications_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_FindApplications_InputArguments = 66;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_FindApplications_OutputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_FindApplications_OutputArguments = 67;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_RegisterApplication_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_RegisterApplication_InputArguments = 69;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_RegisterApplication_OutputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_RegisterApplication_OutputArguments = 70;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_UpdateApplication_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_UpdateApplication_InputArguments = 194;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_UnregisterApplication_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_UnregisterApplication_InputArguments = 72;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_GetApplication_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_GetApplication_InputArguments = 214;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_GetApplication_OutputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_GetApplication_OutputArguments = 215;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_QueryApplications_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_QueryApplications_InputArguments = 872;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_QueryApplications_OutputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_QueryApplications_OutputArguments = 873;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_QueryServers_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_QueryServers_InputArguments = 74;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_QueryServers_OutputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_QueryServers_OutputArguments = 75;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Size Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Size = 514;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Writable Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Writable = 515;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_UserWritable Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_UserWritable = 516;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_OpenCount Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_OpenCount = 517;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Open_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Open_InputArguments = 520;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Open_OutputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Open_OutputArguments = 521;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Close_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Close_InputArguments = 523;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Read_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Read_InputArguments = 525;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Read_OutputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Read_OutputArguments = 526;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Write_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Write_InputArguments = 528;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_GetPosition_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_GetPosition_InputArguments = 530;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_GetPosition_OutputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_GetPosition_OutputArguments = 531;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_SetPosition_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_SetPosition_InputArguments = 533;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_LastUpdateTime Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_LastUpdateTime = 534;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_OpenWithMasks_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_OpenWithMasks_InputArguments = 536;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_OpenWithMasks_OutputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_OpenWithMasks_OutputArguments = 537;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_CloseAndUpdate_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_CloseAndUpdate_InputArguments = 539;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_CloseAndUpdate_OutputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_CloseAndUpdate_OutputArguments = 540;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_AddCertificate_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_AddCertificate_InputArguments = 542;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_RemoveCertificate_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_RemoveCertificate_InputArguments = 544;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_CertificateTypes Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_CertificateTypes = 545;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Size Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Size = 548;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Writable Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Writable = 549;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_UserWritable Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_UserWritable = 550;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_OpenCount Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_OpenCount = 551;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Open_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Open_InputArguments = 554;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Open_OutputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Open_OutputArguments = 555;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Close_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Close_InputArguments = 557;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Read_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Read_InputArguments = 559;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Read_OutputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Read_OutputArguments = 560;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Write_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Write_InputArguments = 562;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_GetPosition_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_GetPosition_InputArguments = 564;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_GetPosition_OutputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_GetPosition_OutputArguments = 565;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_SetPosition_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_SetPosition_InputArguments = 567;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_LastUpdateTime Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_LastUpdateTime = 568;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_OpenWithMasks_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_OpenWithMasks_InputArguments = 570;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_OpenWithMasks_OutputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_OpenWithMasks_OutputArguments = 571;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_CloseAndUpdate_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_CloseAndUpdate_InputArguments = 573;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_CloseAndUpdate_OutputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_CloseAndUpdate_OutputArguments = 574;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_AddCertificate_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_AddCertificate_InputArguments = 576;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_RemoveCertificate_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_RemoveCertificate_InputArguments = 578;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_CertificateTypes Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_CertificateTypes = 579;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Size Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Size = 582;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Writable Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Writable = 583;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_UserWritable Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_UserWritable = 584;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenCount Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenCount = 585;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Open_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Open_InputArguments = 588;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Open_OutputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Open_OutputArguments = 589;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Close_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Close_InputArguments = 591;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Read_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Read_InputArguments = 593;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Read_OutputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Read_OutputArguments = 594;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Write_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Write_InputArguments = 596;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_GetPosition_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_GetPosition_InputArguments = 598;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_GetPosition_OutputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_GetPosition_OutputArguments = 599;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_SetPosition_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_SetPosition_InputArguments = 601;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_LastUpdateTime Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_LastUpdateTime = 602;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenWithMasks_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenWithMasks_InputArguments = 604;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenWithMasks_OutputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenWithMasks_OutputArguments = 605;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_CloseAndUpdate_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_CloseAndUpdate_InputArguments = 607;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_CloseAndUpdate_OutputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_CloseAndUpdate_OutputArguments = 608;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_AddCertificate_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_AddCertificate_InputArguments = 610;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_RemoveCertificate_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_RemoveCertificate_InputArguments = 612;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_CertificateTypes Variable.
        /// </summary>
        public const uint CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_CertificateTypes = 613;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_StartSigningRequest_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_StartSigningRequest_InputArguments = 80;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_StartSigningRequest_OutputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_StartSigningRequest_OutputArguments = 81;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_StartNewKeyPairRequest_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_StartNewKeyPairRequest_InputArguments = 77;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_StartNewKeyPairRequest_OutputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_StartNewKeyPairRequest_OutputArguments = 78;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_FinishRequest_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_FinishRequest_InputArguments = 86;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_FinishRequest_OutputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_FinishRequest_OutputArguments = 87;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_RevokeCertificate_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_RevokeCertificate_InputArguments = 15004;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_GetCertificateGroups_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_GetCertificateGroups_InputArguments = 370;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_GetCertificateGroups_OutputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_GetCertificateGroups_OutputArguments = 371;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_GetTrustList_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_GetTrustList_InputArguments = 198;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_GetTrustList_OutputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_GetTrustList_OutputArguments = 199;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_GetCertificateStatus_InputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_GetCertificateStatus_InputArguments = 223;

        /// <summary>
        /// The identifier for the CertificateDirectoryType_GetCertificateStatus_OutputArguments Variable.
        /// </summary>
        public const uint CertificateDirectoryType_GetCertificateStatus_OutputArguments = 224;

        /// <summary>
        /// The identifier for the CertificateRequestedAuditEventType_CertificateGroup Variable.
        /// </summary>
        public const uint CertificateRequestedAuditEventType_CertificateGroup = 717;

        /// <summary>
        /// The identifier for the CertificateRequestedAuditEventType_CertificateType Variable.
        /// </summary>
        public const uint CertificateRequestedAuditEventType_CertificateType = 718;

        /// <summary>
        /// The identifier for the CertificateDeliveredAuditEventType_CertificateGroup Variable.
        /// </summary>
        public const uint CertificateDeliveredAuditEventType_CertificateGroup = 719;

        /// <summary>
        /// The identifier for the CertificateDeliveredAuditEventType_CertificateType Variable.
        /// </summary>
        public const uint CertificateDeliveredAuditEventType_CertificateType = 720;

        /// <summary>
        /// The identifier for the KeyCredentialManagement_ServiceName_Placeholder_ResourceUri Variable.
        /// </summary>
        public const uint KeyCredentialManagement_ServiceName_Placeholder_ResourceUri = 1010;

        /// <summary>
        /// The identifier for the KeyCredentialManagement_ServiceName_Placeholder_ProfileUris Variable.
        /// </summary>
        public const uint KeyCredentialManagement_ServiceName_Placeholder_ProfileUris = 1011;

        /// <summary>
        /// The identifier for the KeyCredentialManagement_ServiceName_Placeholder_StartRequest_InputArguments Variable.
        /// </summary>
        public const uint KeyCredentialManagement_ServiceName_Placeholder_StartRequest_InputArguments = 1013;

        /// <summary>
        /// The identifier for the KeyCredentialManagement_ServiceName_Placeholder_StartRequest_OutputArguments Variable.
        /// </summary>
        public const uint KeyCredentialManagement_ServiceName_Placeholder_StartRequest_OutputArguments = 1014;

        /// <summary>
        /// The identifier for the KeyCredentialManagement_ServiceName_Placeholder_FinishRequest_InputArguments Variable.
        /// </summary>
        public const uint KeyCredentialManagement_ServiceName_Placeholder_FinishRequest_InputArguments = 1016;

        /// <summary>
        /// The identifier for the KeyCredentialManagement_ServiceName_Placeholder_FinishRequest_OutputArguments Variable.
        /// </summary>
        public const uint KeyCredentialManagement_ServiceName_Placeholder_FinishRequest_OutputArguments = 1017;

        /// <summary>
        /// The identifier for the KeyCredentialManagement_ServiceName_Placeholder_Revoke_InputArguments Variable.
        /// </summary>
        public const uint KeyCredentialManagement_ServiceName_Placeholder_Revoke_InputArguments = 1019;

        /// <summary>
        /// The identifier for the KeyCredentialServiceType_ResourceUri Variable.
        /// </summary>
        public const uint KeyCredentialServiceType_ResourceUri = 1021;

        /// <summary>
        /// The identifier for the KeyCredentialServiceType_ProfileUris Variable.
        /// </summary>
        public const uint KeyCredentialServiceType_ProfileUris = 1022;

        /// <summary>
        /// The identifier for the KeyCredentialServiceType_StartRequest_InputArguments Variable.
        /// </summary>
        public const uint KeyCredentialServiceType_StartRequest_InputArguments = 1024;

        /// <summary>
        /// The identifier for the KeyCredentialServiceType_StartRequest_OutputArguments Variable.
        /// </summary>
        public const uint KeyCredentialServiceType_StartRequest_OutputArguments = 1025;

        /// <summary>
        /// The identifier for the KeyCredentialServiceType_FinishRequest_InputArguments Variable.
        /// </summary>
        public const uint KeyCredentialServiceType_FinishRequest_InputArguments = 1027;

        /// <summary>
        /// The identifier for the KeyCredentialServiceType_FinishRequest_OutputArguments Variable.
        /// </summary>
        public const uint KeyCredentialServiceType_FinishRequest_OutputArguments = 1028;

        /// <summary>
        /// The identifier for the KeyCredentialServiceType_Revoke_InputArguments Variable.
        /// </summary>
        public const uint KeyCredentialServiceType_Revoke_InputArguments = 1030;

        /// <summary>
        /// The identifier for the AuthorizationServices_ServiceName_Placeholder_ServiceUri Variable.
        /// </summary>
        public const uint AuthorizationServices_ServiceName_Placeholder_ServiceUri = 1000;

        /// <summary>
        /// The identifier for the AuthorizationServices_ServiceName_Placeholder_ServiceCertificate Variable.
        /// </summary>
        public const uint AuthorizationServices_ServiceName_Placeholder_ServiceCertificate = 962;

        /// <summary>
        /// The identifier for the AuthorizationServices_ServiceName_Placeholder_GetServiceDescription_OutputArguments Variable.
        /// </summary>
        public const uint AuthorizationServices_ServiceName_Placeholder_GetServiceDescription_OutputArguments = 1002;

        /// <summary>
        /// The identifier for the AuthorizationServices_ServiceName_Placeholder_RequestAccessToken_InputArguments Variable.
        /// </summary>
        public const uint AuthorizationServices_ServiceName_Placeholder_RequestAccessToken_InputArguments = 964;

        /// <summary>
        /// The identifier for the AuthorizationServices_ServiceName_Placeholder_RequestAccessToken_OutputArguments Variable.
        /// </summary>
        public const uint AuthorizationServices_ServiceName_Placeholder_RequestAccessToken_OutputArguments = 965;

        /// <summary>
        /// The identifier for the AuthorizationServiceType_ServiceUri Variable.
        /// </summary>
        public const uint AuthorizationServiceType_ServiceUri = 1003;

        /// <summary>
        /// The identifier for the AuthorizationServiceType_ServiceCertificate Variable.
        /// </summary>
        public const uint AuthorizationServiceType_ServiceCertificate = 968;

        /// <summary>
        /// The identifier for the AuthorizationServiceType_UserTokenPolicies Variable.
        /// </summary>
        public const uint AuthorizationServiceType_UserTokenPolicies = 967;

        /// <summary>
        /// The identifier for the AuthorizationServiceType_GetServiceDescription_OutputArguments Variable.
        /// </summary>
        public const uint AuthorizationServiceType_GetServiceDescription_OutputArguments = 1005;

        /// <summary>
        /// The identifier for the AuthorizationServiceType_RequestAccessToken_InputArguments Variable.
        /// </summary>
        public const uint AuthorizationServiceType_RequestAccessToken_InputArguments = 970;

        /// <summary>
        /// The identifier for the AuthorizationServiceType_RequestAccessToken_OutputArguments Variable.
        /// </summary>
        public const uint AuthorizationServiceType_RequestAccessToken_OutputArguments = 971;

        /// <summary>
        /// The identifier for the Directory_FindApplications_InputArguments Variable.
        /// </summary>
        public const uint Directory_FindApplications_InputArguments = 144;

        /// <summary>
        /// The identifier for the Directory_FindApplications_OutputArguments Variable.
        /// </summary>
        public const uint Directory_FindApplications_OutputArguments = 145;

        /// <summary>
        /// The identifier for the Directory_RegisterApplication_InputArguments Variable.
        /// </summary>
        public const uint Directory_RegisterApplication_InputArguments = 147;

        /// <summary>
        /// The identifier for the Directory_RegisterApplication_OutputArguments Variable.
        /// </summary>
        public const uint Directory_RegisterApplication_OutputArguments = 148;

        /// <summary>
        /// The identifier for the Directory_UpdateApplication_InputArguments Variable.
        /// </summary>
        public const uint Directory_UpdateApplication_InputArguments = 201;

        /// <summary>
        /// The identifier for the Directory_UnregisterApplication_InputArguments Variable.
        /// </summary>
        public const uint Directory_UnregisterApplication_InputArguments = 150;

        /// <summary>
        /// The identifier for the Directory_GetApplication_InputArguments Variable.
        /// </summary>
        public const uint Directory_GetApplication_InputArguments = 217;

        /// <summary>
        /// The identifier for the Directory_GetApplication_OutputArguments Variable.
        /// </summary>
        public const uint Directory_GetApplication_OutputArguments = 218;

        /// <summary>
        /// The identifier for the Directory_QueryApplications_InputArguments Variable.
        /// </summary>
        public const uint Directory_QueryApplications_InputArguments = 993;

        /// <summary>
        /// The identifier for the Directory_QueryApplications_OutputArguments Variable.
        /// </summary>
        public const uint Directory_QueryApplications_OutputArguments = 994;

        /// <summary>
        /// The identifier for the Directory_QueryServers_InputArguments Variable.
        /// </summary>
        public const uint Directory_QueryServers_InputArguments = 152;

        /// <summary>
        /// The identifier for the Directory_QueryServers_OutputArguments Variable.
        /// </summary>
        public const uint Directory_QueryServers_OutputArguments = 153;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Size Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Size = 617;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Writable Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Writable = 618;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_UserWritable Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList_UserWritable = 619;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_OpenCount Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList_OpenCount = 620;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Open_InputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Open_InputArguments = 623;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Open_OutputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Open_OutputArguments = 624;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Close_InputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Close_InputArguments = 626;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Read_InputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Read_InputArguments = 628;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Read_OutputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Read_OutputArguments = 629;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Write_InputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Write_InputArguments = 631;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_GetPosition_InputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList_GetPosition_InputArguments = 633;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_GetPosition_OutputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList_GetPosition_OutputArguments = 634;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_SetPosition_InputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList_SetPosition_InputArguments = 636;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_LastUpdateTime Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList_LastUpdateTime = 637;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_OpenWithMasks_InputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList_OpenWithMasks_InputArguments = 639;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_OpenWithMasks_OutputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList_OpenWithMasks_OutputArguments = 640;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_CloseAndUpdate_InputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList_CloseAndUpdate_InputArguments = 642;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_CloseAndUpdate_OutputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList_CloseAndUpdate_OutputArguments = 643;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_AddCertificate_InputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList_AddCertificate_InputArguments = 645;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_RemoveCertificate_InputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_TrustList_RemoveCertificate_InputArguments = 647;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_CertificateTypes Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultApplicationGroup_CertificateTypes = 648;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Size Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Size = 651;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Writable Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Writable = 652;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_UserWritable Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList_UserWritable = 653;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_OpenCount Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList_OpenCount = 654;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Open_InputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Open_InputArguments = 657;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Open_OutputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Open_OutputArguments = 658;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Close_InputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Close_InputArguments = 660;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Read_InputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Read_InputArguments = 662;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Read_OutputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Read_OutputArguments = 663;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Write_InputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Write_InputArguments = 665;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_GetPosition_InputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList_GetPosition_InputArguments = 667;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_GetPosition_OutputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList_GetPosition_OutputArguments = 668;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_SetPosition_InputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList_SetPosition_InputArguments = 670;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_LastUpdateTime Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList_LastUpdateTime = 671;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_OpenWithMasks_InputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList_OpenWithMasks_InputArguments = 673;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_OpenWithMasks_OutputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList_OpenWithMasks_OutputArguments = 674;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_CloseAndUpdate_InputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList_CloseAndUpdate_InputArguments = 676;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_CloseAndUpdate_OutputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList_CloseAndUpdate_OutputArguments = 677;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_AddCertificate_InputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList_AddCertificate_InputArguments = 679;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_RemoveCertificate_InputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_TrustList_RemoveCertificate_InputArguments = 681;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_CertificateTypes Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultHttpsGroup_CertificateTypes = 682;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Size Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Size = 685;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Writable Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Writable = 686;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_UserWritable Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_UserWritable = 687;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenCount Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenCount = 688;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Open_InputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Open_InputArguments = 691;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Open_OutputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Open_OutputArguments = 692;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Close_InputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Close_InputArguments = 694;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Read_InputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Read_InputArguments = 696;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Read_OutputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Read_OutputArguments = 697;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Write_InputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Write_InputArguments = 699;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_GetPosition_InputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_GetPosition_InputArguments = 701;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_GetPosition_OutputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_GetPosition_OutputArguments = 702;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_SetPosition_InputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_SetPosition_InputArguments = 704;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_LastUpdateTime Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_LastUpdateTime = 705;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenWithMasks_InputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenWithMasks_InputArguments = 707;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenWithMasks_OutputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenWithMasks_OutputArguments = 708;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_CloseAndUpdate_InputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_CloseAndUpdate_InputArguments = 710;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_CloseAndUpdate_OutputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_CloseAndUpdate_OutputArguments = 711;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_AddCertificate_InputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_AddCertificate_InputArguments = 713;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_RemoveCertificate_InputArguments Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_RemoveCertificate_InputArguments = 715;

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_CertificateTypes Variable.
        /// </summary>
        public const uint Directory_CertificateGroups_DefaultUserTokenGroup_CertificateTypes = 716;

        /// <summary>
        /// The identifier for the Directory_StartSigningRequest_InputArguments Variable.
        /// </summary>
        public const uint Directory_StartSigningRequest_InputArguments = 158;

        /// <summary>
        /// The identifier for the Directory_StartSigningRequest_OutputArguments Variable.
        /// </summary>
        public const uint Directory_StartSigningRequest_OutputArguments = 159;

        /// <summary>
        /// The identifier for the Directory_StartNewKeyPairRequest_InputArguments Variable.
        /// </summary>
        public const uint Directory_StartNewKeyPairRequest_InputArguments = 155;

        /// <summary>
        /// The identifier for the Directory_StartNewKeyPairRequest_OutputArguments Variable.
        /// </summary>
        public const uint Directory_StartNewKeyPairRequest_OutputArguments = 156;

        /// <summary>
        /// The identifier for the Directory_FinishRequest_InputArguments Variable.
        /// </summary>
        public const uint Directory_FinishRequest_InputArguments = 164;

        /// <summary>
        /// The identifier for the Directory_FinishRequest_OutputArguments Variable.
        /// </summary>
        public const uint Directory_FinishRequest_OutputArguments = 165;

        /// <summary>
        /// The identifier for the Directory_RevokeCertificate_InputArguments Variable.
        /// </summary>
        public const uint Directory_RevokeCertificate_InputArguments = 15006;

        /// <summary>
        /// The identifier for the Directory_GetCertificateGroups_InputArguments Variable.
        /// </summary>
        public const uint Directory_GetCertificateGroups_InputArguments = 509;

        /// <summary>
        /// The identifier for the Directory_GetCertificateGroups_OutputArguments Variable.
        /// </summary>
        public const uint Directory_GetCertificateGroups_OutputArguments = 510;

        /// <summary>
        /// The identifier for the Directory_GetTrustList_InputArguments Variable.
        /// </summary>
        public const uint Directory_GetTrustList_InputArguments = 205;

        /// <summary>
        /// The identifier for the Directory_GetTrustList_OutputArguments Variable.
        /// </summary>
        public const uint Directory_GetTrustList_OutputArguments = 206;

        /// <summary>
        /// The identifier for the Directory_GetCertificateStatus_InputArguments Variable.
        /// </summary>
        public const uint Directory_GetCertificateStatus_InputArguments = 226;

        /// <summary>
        /// The identifier for the Directory_GetCertificateStatus_OutputArguments Variable.
        /// </summary>
        public const uint Directory_GetCertificateStatus_OutputArguments = 227;

        /// <summary>
        /// The identifier for the OpcUaGds_BinarySchema Variable.
        /// </summary>
        public const uint OpcUaGds_BinarySchema = 135;

        /// <summary>
        /// The identifier for the OpcUaGds_BinarySchema_NamespaceUri Variable.
        /// </summary>
        public const uint OpcUaGds_BinarySchema_NamespaceUri = 137;

        /// <summary>
        /// The identifier for the OpcUaGds_BinarySchema_Deprecated Variable.
        /// </summary>
        public const uint OpcUaGds_BinarySchema_Deprecated = 8002;

        /// <summary>
        /// The identifier for the OpcUaGds_BinarySchema_ApplicationRecordDataType Variable.
        /// </summary>
        public const uint OpcUaGds_BinarySchema_ApplicationRecordDataType = 138;

        /// <summary>
        /// The identifier for the OpcUaGds_XmlSchema Variable.
        /// </summary>
        public const uint OpcUaGds_XmlSchema = 128;

        /// <summary>
        /// The identifier for the OpcUaGds_XmlSchema_NamespaceUri Variable.
        /// </summary>
        public const uint OpcUaGds_XmlSchema_NamespaceUri = 130;

        /// <summary>
        /// The identifier for the OpcUaGds_XmlSchema_Deprecated Variable.
        /// </summary>
        public const uint OpcUaGds_XmlSchema_Deprecated = 8004;

        /// <summary>
        /// The identifier for the OpcUaGds_XmlSchema_ApplicationRecordDataType Variable.
        /// </summary>
        public const uint OpcUaGds_XmlSchema_ApplicationRecordDataType = 131;
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
        /// The identifier for the ApplicationRecordDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId ApplicationRecordDataType = new ExpandedNodeId(Opc.Ua.Gds.DataTypes.ApplicationRecordDataType, Opc.Ua.Gds.Namespaces.OpcUaGds);
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
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_Open Method.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAGDSNamespaceMetadata_NamespaceFile_Open = new ExpandedNodeId(Opc.Ua.Gds.Methods.OPCUAGDSNamespaceMetadata_NamespaceFile_Open, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_Close Method.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAGDSNamespaceMetadata_NamespaceFile_Close = new ExpandedNodeId(Opc.Ua.Gds.Methods.OPCUAGDSNamespaceMetadata_NamespaceFile_Close, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_Read Method.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAGDSNamespaceMetadata_NamespaceFile_Read = new ExpandedNodeId(Opc.Ua.Gds.Methods.OPCUAGDSNamespaceMetadata_NamespaceFile_Read, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_Write Method.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAGDSNamespaceMetadata_NamespaceFile_Write = new ExpandedNodeId(Opc.Ua.Gds.Methods.OPCUAGDSNamespaceMetadata_NamespaceFile_Write, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_GetPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAGDSNamespaceMetadata_NamespaceFile_GetPosition = new ExpandedNodeId(Opc.Ua.Gds.Methods.OPCUAGDSNamespaceMetadata_NamespaceFile_GetPosition, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_SetPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAGDSNamespaceMetadata_NamespaceFile_SetPosition = new ExpandedNodeId(Opc.Ua.Gds.Methods.OPCUAGDSNamespaceMetadata_NamespaceFile_SetPosition, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the DirectoryType_FindApplications Method.
        /// </summary>
        public static readonly ExpandedNodeId DirectoryType_FindApplications = new ExpandedNodeId(Opc.Ua.Gds.Methods.DirectoryType_FindApplications, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the DirectoryType_RegisterApplication Method.
        /// </summary>
        public static readonly ExpandedNodeId DirectoryType_RegisterApplication = new ExpandedNodeId(Opc.Ua.Gds.Methods.DirectoryType_RegisterApplication, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the DirectoryType_UpdateApplication Method.
        /// </summary>
        public static readonly ExpandedNodeId DirectoryType_UpdateApplication = new ExpandedNodeId(Opc.Ua.Gds.Methods.DirectoryType_UpdateApplication, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the DirectoryType_UnregisterApplication Method.
        /// </summary>
        public static readonly ExpandedNodeId DirectoryType_UnregisterApplication = new ExpandedNodeId(Opc.Ua.Gds.Methods.DirectoryType_UnregisterApplication, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the DirectoryType_GetApplication Method.
        /// </summary>
        public static readonly ExpandedNodeId DirectoryType_GetApplication = new ExpandedNodeId(Opc.Ua.Gds.Methods.DirectoryType_GetApplication, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the DirectoryType_QueryApplications Method.
        /// </summary>
        public static readonly ExpandedNodeId DirectoryType_QueryApplications = new ExpandedNodeId(Opc.Ua.Gds.Methods.DirectoryType_QueryApplications, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the DirectoryType_QueryServers Method.
        /// </summary>
        public static readonly ExpandedNodeId DirectoryType_QueryServers = new ExpandedNodeId(Opc.Ua.Gds.Methods.DirectoryType_QueryServers, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Open Method.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Open = new ExpandedNodeId(Opc.Ua.Gds.Methods.CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Open, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Close Method.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Close = new ExpandedNodeId(Opc.Ua.Gds.Methods.CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Close, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Read Method.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Read = new ExpandedNodeId(Opc.Ua.Gds.Methods.CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Read, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Write Method.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Write = new ExpandedNodeId(Opc.Ua.Gds.Methods.CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Write, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_GetPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_GetPosition = new ExpandedNodeId(Opc.Ua.Gds.Methods.CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_GetPosition, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_SetPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_SetPosition = new ExpandedNodeId(Opc.Ua.Gds.Methods.CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_SetPosition, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_OpenWithMasks Method.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_OpenWithMasks = new ExpandedNodeId(Opc.Ua.Gds.Methods.CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_OpenWithMasks, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Open Method.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Open = new ExpandedNodeId(Opc.Ua.Gds.Methods.CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Open, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Close Method.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Close = new ExpandedNodeId(Opc.Ua.Gds.Methods.CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Close, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Read Method.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Read = new ExpandedNodeId(Opc.Ua.Gds.Methods.CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Read, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Write Method.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Write = new ExpandedNodeId(Opc.Ua.Gds.Methods.CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Write, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_GetPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_GetPosition = new ExpandedNodeId(Opc.Ua.Gds.Methods.CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_GetPosition, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_SetPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_SetPosition = new ExpandedNodeId(Opc.Ua.Gds.Methods.CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_SetPosition, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_OpenWithMasks Method.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_OpenWithMasks = new ExpandedNodeId(Opc.Ua.Gds.Methods.CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_OpenWithMasks, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Open Method.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Open = new ExpandedNodeId(Opc.Ua.Gds.Methods.CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Open, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Close Method.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Close = new ExpandedNodeId(Opc.Ua.Gds.Methods.CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Close, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Read Method.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Read = new ExpandedNodeId(Opc.Ua.Gds.Methods.CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Read, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Write Method.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Write = new ExpandedNodeId(Opc.Ua.Gds.Methods.CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Write, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_GetPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_GetPosition = new ExpandedNodeId(Opc.Ua.Gds.Methods.CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_GetPosition, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_SetPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_SetPosition = new ExpandedNodeId(Opc.Ua.Gds.Methods.CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_SetPosition, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenWithMasks Method.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenWithMasks = new ExpandedNodeId(Opc.Ua.Gds.Methods.CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenWithMasks, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_StartSigningRequest Method.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_StartSigningRequest = new ExpandedNodeId(Opc.Ua.Gds.Methods.CertificateDirectoryType_StartSigningRequest, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_StartNewKeyPairRequest Method.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_StartNewKeyPairRequest = new ExpandedNodeId(Opc.Ua.Gds.Methods.CertificateDirectoryType_StartNewKeyPairRequest, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_FinishRequest Method.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_FinishRequest = new ExpandedNodeId(Opc.Ua.Gds.Methods.CertificateDirectoryType_FinishRequest, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_RevokeCertificate Method.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_RevokeCertificate = new ExpandedNodeId(Opc.Ua.Gds.Methods.CertificateDirectoryType_RevokeCertificate, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_GetCertificateGroups Method.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_GetCertificateGroups = new ExpandedNodeId(Opc.Ua.Gds.Methods.CertificateDirectoryType_GetCertificateGroups, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_GetTrustList Method.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_GetTrustList = new ExpandedNodeId(Opc.Ua.Gds.Methods.CertificateDirectoryType_GetTrustList, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_GetCertificateStatus Method.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_GetCertificateStatus = new ExpandedNodeId(Opc.Ua.Gds.Methods.CertificateDirectoryType_GetCertificateStatus, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the KeyCredentialManagement_ServiceName_Placeholder_StartRequest Method.
        /// </summary>
        public static readonly ExpandedNodeId KeyCredentialManagement_ServiceName_Placeholder_StartRequest = new ExpandedNodeId(Opc.Ua.Gds.Methods.KeyCredentialManagement_ServiceName_Placeholder_StartRequest, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the KeyCredentialManagement_ServiceName_Placeholder_FinishRequest Method.
        /// </summary>
        public static readonly ExpandedNodeId KeyCredentialManagement_ServiceName_Placeholder_FinishRequest = new ExpandedNodeId(Opc.Ua.Gds.Methods.KeyCredentialManagement_ServiceName_Placeholder_FinishRequest, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the KeyCredentialServiceType_StartRequest Method.
        /// </summary>
        public static readonly ExpandedNodeId KeyCredentialServiceType_StartRequest = new ExpandedNodeId(Opc.Ua.Gds.Methods.KeyCredentialServiceType_StartRequest, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the KeyCredentialServiceType_FinishRequest Method.
        /// </summary>
        public static readonly ExpandedNodeId KeyCredentialServiceType_FinishRequest = new ExpandedNodeId(Opc.Ua.Gds.Methods.KeyCredentialServiceType_FinishRequest, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the KeyCredentialServiceType_Revoke Method.
        /// </summary>
        public static readonly ExpandedNodeId KeyCredentialServiceType_Revoke = new ExpandedNodeId(Opc.Ua.Gds.Methods.KeyCredentialServiceType_Revoke, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the AuthorizationServices_ServiceName_Placeholder_GetServiceDescription Method.
        /// </summary>
        public static readonly ExpandedNodeId AuthorizationServices_ServiceName_Placeholder_GetServiceDescription = new ExpandedNodeId(Opc.Ua.Gds.Methods.AuthorizationServices_ServiceName_Placeholder_GetServiceDescription, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the AuthorizationServiceType_GetServiceDescription Method.
        /// </summary>
        public static readonly ExpandedNodeId AuthorizationServiceType_GetServiceDescription = new ExpandedNodeId(Opc.Ua.Gds.Methods.AuthorizationServiceType_GetServiceDescription, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the AuthorizationServiceType_RequestAccessToken Method.
        /// </summary>
        public static readonly ExpandedNodeId AuthorizationServiceType_RequestAccessToken = new ExpandedNodeId(Opc.Ua.Gds.Methods.AuthorizationServiceType_RequestAccessToken, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_FindApplications Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_FindApplications = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_FindApplications, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_RegisterApplication Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_RegisterApplication = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_RegisterApplication, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_UpdateApplication Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_UpdateApplication = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_UpdateApplication, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_UnregisterApplication Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_UnregisterApplication = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_UnregisterApplication, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_GetApplication Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_GetApplication = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_GetApplication, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_QueryApplications Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_QueryApplications = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_QueryApplications, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_QueryServers Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_QueryServers = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_QueryServers, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Open Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Open = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Open, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Close Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Close = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Close, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Read Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Read = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Read, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Write Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Write = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Write, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_GetPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList_GetPosition = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_CertificateGroups_DefaultApplicationGroup_TrustList_GetPosition, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_SetPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList_SetPosition = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_CertificateGroups_DefaultApplicationGroup_TrustList_SetPosition, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_OpenWithMasks Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList_OpenWithMasks = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_CertificateGroups_DefaultApplicationGroup_TrustList_OpenWithMasks, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_CloseAndUpdate Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList_CloseAndUpdate = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_CertificateGroups_DefaultApplicationGroup_TrustList_CloseAndUpdate, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_AddCertificate Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList_AddCertificate = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_CertificateGroups_DefaultApplicationGroup_TrustList_AddCertificate, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_RemoveCertificate Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList_RemoveCertificate = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_CertificateGroups_DefaultApplicationGroup_TrustList_RemoveCertificate, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Open Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Open = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Open, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Close Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Close = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Close, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Read Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Read = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Read, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Write Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Write = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Write, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_GetPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList_GetPosition = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_CertificateGroups_DefaultHttpsGroup_TrustList_GetPosition, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_SetPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList_SetPosition = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_CertificateGroups_DefaultHttpsGroup_TrustList_SetPosition, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_OpenWithMasks Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList_OpenWithMasks = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_CertificateGroups_DefaultHttpsGroup_TrustList_OpenWithMasks, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_CloseAndUpdate Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList_CloseAndUpdate = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_CertificateGroups_DefaultHttpsGroup_TrustList_CloseAndUpdate, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_AddCertificate Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList_AddCertificate = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_CertificateGroups_DefaultHttpsGroup_TrustList_AddCertificate, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_RemoveCertificate Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList_RemoveCertificate = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_CertificateGroups_DefaultHttpsGroup_TrustList_RemoveCertificate, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Open Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Open = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Open, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Close Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Close = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Close, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Read Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Read = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Read, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Write Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Write = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Write, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_GetPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_GetPosition = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_GetPosition, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_SetPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_SetPosition = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_SetPosition, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenWithMasks Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenWithMasks = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenWithMasks, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_CloseAndUpdate Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_CloseAndUpdate = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_CloseAndUpdate, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_AddCertificate Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_AddCertificate = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_AddCertificate, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_RemoveCertificate Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_RemoveCertificate = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_RemoveCertificate, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_StartSigningRequest Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_StartSigningRequest = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_StartSigningRequest, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_StartNewKeyPairRequest Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_StartNewKeyPairRequest = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_StartNewKeyPairRequest, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_FinishRequest Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_FinishRequest = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_FinishRequest, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_GetCertificateGroups Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_GetCertificateGroups = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_GetCertificateGroups, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_GetTrustList Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_GetTrustList = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_GetTrustList, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_GetCertificateStatus Method.
        /// </summary>
        public static readonly ExpandedNodeId Directory_GetCertificateStatus = new ExpandedNodeId(Opc.Ua.Gds.Methods.Directory_GetCertificateStatus, Opc.Ua.Gds.Namespaces.OpcUaGds);
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
        /// The identifier for the OPCUAGDSNamespaceMetadata Object.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAGDSNamespaceMetadata = new ExpandedNodeId(Opc.Ua.Gds.Objects.OPCUAGDSNamespaceMetadata, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the DirectoryType_Applications Object.
        /// </summary>
        public static readonly ExpandedNodeId DirectoryType_Applications = new ExpandedNodeId(Opc.Ua.Gds.Objects.DirectoryType_Applications, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups Object.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups = new ExpandedNodeId(Opc.Ua.Gds.Objects.CertificateDirectoryType_CertificateGroups, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup Object.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup = new ExpandedNodeId(Opc.Ua.Gds.Objects.CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList Object.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList = new ExpandedNodeId(Opc.Ua.Gds.Objects.CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList Object.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList = new ExpandedNodeId(Opc.Ua.Gds.Objects.CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList Object.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList = new ExpandedNodeId(Opc.Ua.Gds.Objects.CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the KeyCredentialManagement Object.
        /// </summary>
        public static readonly ExpandedNodeId KeyCredentialManagement = new ExpandedNodeId(Opc.Ua.Gds.Objects.KeyCredentialManagement, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the AuthorizationServices Object.
        /// </summary>
        public static readonly ExpandedNodeId AuthorizationServices = new ExpandedNodeId(Opc.Ua.Gds.Objects.AuthorizationServices, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory Object.
        /// </summary>
        public static readonly ExpandedNodeId Directory = new ExpandedNodeId(Opc.Ua.Gds.Objects.Directory, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_Applications Object.
        /// </summary>
        public static readonly ExpandedNodeId Directory_Applications = new ExpandedNodeId(Opc.Ua.Gds.Objects.Directory_Applications, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups Object.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups = new ExpandedNodeId(Opc.Ua.Gds.Objects.Directory_CertificateGroups, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup Object.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup = new ExpandedNodeId(Opc.Ua.Gds.Objects.Directory_CertificateGroups_DefaultApplicationGroup, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList Object.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList = new ExpandedNodeId(Opc.Ua.Gds.Objects.Directory_CertificateGroups_DefaultApplicationGroup_TrustList, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup Object.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup = new ExpandedNodeId(Opc.Ua.Gds.Objects.Directory_CertificateGroups_DefaultHttpsGroup, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList Object.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList = new ExpandedNodeId(Opc.Ua.Gds.Objects.Directory_CertificateGroups_DefaultHttpsGroup_TrustList, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup Object.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup = new ExpandedNodeId(Opc.Ua.Gds.Objects.Directory_CertificateGroups_DefaultUserTokenGroup, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList Object.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList = new ExpandedNodeId(Opc.Ua.Gds.Objects.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the ApplicationRecordDataType_Encoding_DefaultBinary Object.
        /// </summary>
        public static readonly ExpandedNodeId ApplicationRecordDataType_Encoding_DefaultBinary = new ExpandedNodeId(Opc.Ua.Gds.Objects.ApplicationRecordDataType_Encoding_DefaultBinary, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the ApplicationRecordDataType_Encoding_DefaultXml Object.
        /// </summary>
        public static readonly ExpandedNodeId ApplicationRecordDataType_Encoding_DefaultXml = new ExpandedNodeId(Opc.Ua.Gds.Objects.ApplicationRecordDataType_Encoding_DefaultXml, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the ApplicationRecordDataType_Encoding_DefaultJson Object.
        /// </summary>
        public static readonly ExpandedNodeId ApplicationRecordDataType_Encoding_DefaultJson = new ExpandedNodeId(Opc.Ua.Gds.Objects.ApplicationRecordDataType_Encoding_DefaultJson, Opc.Ua.Gds.Namespaces.OpcUaGds);
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
        /// The identifier for the DirectoryType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId DirectoryType = new ExpandedNodeId(Opc.Ua.Gds.ObjectTypes.DirectoryType, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the ApplicationRegistrationChangedAuditEventType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ApplicationRegistrationChangedAuditEventType = new ExpandedNodeId(Opc.Ua.Gds.ObjectTypes.ApplicationRegistrationChangedAuditEventType, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType = new ExpandedNodeId(Opc.Ua.Gds.ObjectTypes.CertificateDirectoryType, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateRequestedAuditEventType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId CertificateRequestedAuditEventType = new ExpandedNodeId(Opc.Ua.Gds.ObjectTypes.CertificateRequestedAuditEventType, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDeliveredAuditEventType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDeliveredAuditEventType = new ExpandedNodeId(Opc.Ua.Gds.ObjectTypes.CertificateDeliveredAuditEventType, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the KeyCredentialServiceType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId KeyCredentialServiceType = new ExpandedNodeId(Opc.Ua.Gds.ObjectTypes.KeyCredentialServiceType, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the KeyCredentialRequestedAuditEventType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId KeyCredentialRequestedAuditEventType = new ExpandedNodeId(Opc.Ua.Gds.ObjectTypes.KeyCredentialRequestedAuditEventType, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the KeyCredentialDeliveredAuditEventType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId KeyCredentialDeliveredAuditEventType = new ExpandedNodeId(Opc.Ua.Gds.ObjectTypes.KeyCredentialDeliveredAuditEventType, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the KeyCredentialRevokedAuditEventType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId KeyCredentialRevokedAuditEventType = new ExpandedNodeId(Opc.Ua.Gds.ObjectTypes.KeyCredentialRevokedAuditEventType, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the AuthorizationServiceType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId AuthorizationServiceType = new ExpandedNodeId(Opc.Ua.Gds.ObjectTypes.AuthorizationServiceType, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the AccessTokenAuditIssuedAuditEventType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId AccessTokenAuditIssuedAuditEventType = new ExpandedNodeId(Opc.Ua.Gds.ObjectTypes.AccessTokenAuditIssuedAuditEventType, Opc.Ua.Gds.Namespaces.OpcUaGds);
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
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAGDSNamespaceMetadata_NamespaceUri = new ExpandedNodeId(Opc.Ua.Gds.Variables.OPCUAGDSNamespaceMetadata_NamespaceUri, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceVersion Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAGDSNamespaceMetadata_NamespaceVersion = new ExpandedNodeId(Opc.Ua.Gds.Variables.OPCUAGDSNamespaceMetadata_NamespaceVersion, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespacePublicationDate Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAGDSNamespaceMetadata_NamespacePublicationDate = new ExpandedNodeId(Opc.Ua.Gds.Variables.OPCUAGDSNamespaceMetadata_NamespacePublicationDate, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_IsNamespaceSubset Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAGDSNamespaceMetadata_IsNamespaceSubset = new ExpandedNodeId(Opc.Ua.Gds.Variables.OPCUAGDSNamespaceMetadata_IsNamespaceSubset, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_StaticNodeIdTypes Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAGDSNamespaceMetadata_StaticNodeIdTypes = new ExpandedNodeId(Opc.Ua.Gds.Variables.OPCUAGDSNamespaceMetadata_StaticNodeIdTypes, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_StaticNumericNodeIdRange Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAGDSNamespaceMetadata_StaticNumericNodeIdRange = new ExpandedNodeId(Opc.Ua.Gds.Variables.OPCUAGDSNamespaceMetadata_StaticNumericNodeIdRange, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_StaticStringNodeIdPattern Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAGDSNamespaceMetadata_StaticStringNodeIdPattern = new ExpandedNodeId(Opc.Ua.Gds.Variables.OPCUAGDSNamespaceMetadata_StaticStringNodeIdPattern, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_Size Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAGDSNamespaceMetadata_NamespaceFile_Size = new ExpandedNodeId(Opc.Ua.Gds.Variables.OPCUAGDSNamespaceMetadata_NamespaceFile_Size, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_Writable Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAGDSNamespaceMetadata_NamespaceFile_Writable = new ExpandedNodeId(Opc.Ua.Gds.Variables.OPCUAGDSNamespaceMetadata_NamespaceFile_Writable, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_UserWritable Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAGDSNamespaceMetadata_NamespaceFile_UserWritable = new ExpandedNodeId(Opc.Ua.Gds.Variables.OPCUAGDSNamespaceMetadata_NamespaceFile_UserWritable, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_OpenCount Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAGDSNamespaceMetadata_NamespaceFile_OpenCount = new ExpandedNodeId(Opc.Ua.Gds.Variables.OPCUAGDSNamespaceMetadata_NamespaceFile_OpenCount, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_Open_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAGDSNamespaceMetadata_NamespaceFile_Open_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.OPCUAGDSNamespaceMetadata_NamespaceFile_Open_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_Open_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAGDSNamespaceMetadata_NamespaceFile_Open_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.OPCUAGDSNamespaceMetadata_NamespaceFile_Open_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_Close_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAGDSNamespaceMetadata_NamespaceFile_Close_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.OPCUAGDSNamespaceMetadata_NamespaceFile_Close_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_Read_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAGDSNamespaceMetadata_NamespaceFile_Read_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.OPCUAGDSNamespaceMetadata_NamespaceFile_Read_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_Read_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAGDSNamespaceMetadata_NamespaceFile_Read_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.OPCUAGDSNamespaceMetadata_NamespaceFile_Read_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_Write_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAGDSNamespaceMetadata_NamespaceFile_Write_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.OPCUAGDSNamespaceMetadata_NamespaceFile_Write_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_GetPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAGDSNamespaceMetadata_NamespaceFile_GetPosition_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.OPCUAGDSNamespaceMetadata_NamespaceFile_GetPosition_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_GetPosition_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAGDSNamespaceMetadata_NamespaceFile_GetPosition_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.OPCUAGDSNamespaceMetadata_NamespaceFile_GetPosition_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_NamespaceFile_SetPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAGDSNamespaceMetadata_NamespaceFile_SetPosition_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.OPCUAGDSNamespaceMetadata_NamespaceFile_SetPosition_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_DefaultRolePermissions Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAGDSNamespaceMetadata_DefaultRolePermissions = new ExpandedNodeId(Opc.Ua.Gds.Variables.OPCUAGDSNamespaceMetadata_DefaultRolePermissions, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_DefaultUserRolePermissions Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAGDSNamespaceMetadata_DefaultUserRolePermissions = new ExpandedNodeId(Opc.Ua.Gds.Variables.OPCUAGDSNamespaceMetadata_DefaultUserRolePermissions, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OPCUAGDSNamespaceMetadata_DefaultAccessRestrictions Variable.
        /// </summary>
        public static readonly ExpandedNodeId OPCUAGDSNamespaceMetadata_DefaultAccessRestrictions = new ExpandedNodeId(Opc.Ua.Gds.Variables.OPCUAGDSNamespaceMetadata_DefaultAccessRestrictions, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the DirectoryType_FindApplications_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectoryType_FindApplications_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.DirectoryType_FindApplications_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the DirectoryType_FindApplications_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectoryType_FindApplications_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.DirectoryType_FindApplications_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the DirectoryType_RegisterApplication_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectoryType_RegisterApplication_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.DirectoryType_RegisterApplication_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the DirectoryType_RegisterApplication_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectoryType_RegisterApplication_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.DirectoryType_RegisterApplication_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the DirectoryType_UpdateApplication_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectoryType_UpdateApplication_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.DirectoryType_UpdateApplication_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the DirectoryType_UnregisterApplication_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectoryType_UnregisterApplication_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.DirectoryType_UnregisterApplication_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the DirectoryType_GetApplication_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectoryType_GetApplication_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.DirectoryType_GetApplication_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the DirectoryType_GetApplication_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectoryType_GetApplication_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.DirectoryType_GetApplication_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the DirectoryType_QueryApplications_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectoryType_QueryApplications_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.DirectoryType_QueryApplications_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the DirectoryType_QueryApplications_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectoryType_QueryApplications_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.DirectoryType_QueryApplications_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the DirectoryType_QueryServers_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectoryType_QueryServers_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.DirectoryType_QueryServers_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the DirectoryType_QueryServers_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId DirectoryType_QueryServers_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.DirectoryType_QueryServers_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_FindApplications_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_FindApplications_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_FindApplications_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_FindApplications_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_FindApplications_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_FindApplications_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_RegisterApplication_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_RegisterApplication_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_RegisterApplication_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_RegisterApplication_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_RegisterApplication_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_RegisterApplication_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_UpdateApplication_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_UpdateApplication_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_UpdateApplication_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_UnregisterApplication_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_UnregisterApplication_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_UnregisterApplication_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_GetApplication_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_GetApplication_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_GetApplication_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_GetApplication_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_GetApplication_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_GetApplication_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_QueryApplications_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_QueryApplications_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_QueryApplications_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_QueryApplications_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_QueryApplications_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_QueryApplications_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_QueryServers_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_QueryServers_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_QueryServers_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_QueryServers_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_QueryServers_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_QueryServers_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Size Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Size = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Size, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Writable Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Writable = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Writable, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_UserWritable Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_UserWritable = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_UserWritable, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_OpenCount Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_OpenCount = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_OpenCount, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Open_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Open_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Open_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Open_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Open_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Open_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Close_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Close_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Close_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Read_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Read_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Read_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Read_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Read_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Read_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Write_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Write_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_Write_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_GetPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_GetPosition_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_GetPosition_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_GetPosition_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_GetPosition_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_GetPosition_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_SetPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_SetPosition_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_SetPosition_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_LastUpdateTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_LastUpdateTime = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_LastUpdateTime, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_OpenWithMasks_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_OpenWithMasks_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_OpenWithMasks_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_OpenWithMasks_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_OpenWithMasks_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_OpenWithMasks_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_CloseAndUpdate_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_CloseAndUpdate_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_CloseAndUpdate_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_CloseAndUpdate_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_CloseAndUpdate_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_CloseAndUpdate_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_AddCertificate_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_AddCertificate_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_AddCertificate_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_RemoveCertificate_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_RemoveCertificate_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_TrustList_RemoveCertificate_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_CertificateTypes Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_CertificateTypes = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultApplicationGroup_CertificateTypes, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Size Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Size = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Size, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Writable Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Writable = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Writable, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_UserWritable Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_UserWritable = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_UserWritable, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_OpenCount Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_OpenCount = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_OpenCount, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Open_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Open_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Open_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Open_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Open_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Open_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Close_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Close_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Close_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Read_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Read_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Read_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Read_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Read_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Read_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Write_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Write_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_Write_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_GetPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_GetPosition_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_GetPosition_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_GetPosition_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_GetPosition_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_GetPosition_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_SetPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_SetPosition_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_SetPosition_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_LastUpdateTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_LastUpdateTime = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_LastUpdateTime, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_OpenWithMasks_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_OpenWithMasks_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_OpenWithMasks_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_OpenWithMasks_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_OpenWithMasks_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_OpenWithMasks_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_CloseAndUpdate_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_CloseAndUpdate_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_CloseAndUpdate_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_CloseAndUpdate_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_CloseAndUpdate_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_CloseAndUpdate_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_AddCertificate_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_AddCertificate_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_AddCertificate_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_RemoveCertificate_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_RemoveCertificate_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_TrustList_RemoveCertificate_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_CertificateTypes Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_CertificateTypes = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultHttpsGroup_CertificateTypes, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Size Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Size = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Size, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Writable Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Writable = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Writable, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_UserWritable Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_UserWritable = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_UserWritable, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenCount Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenCount = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenCount, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Open_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Open_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Open_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Open_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Open_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Open_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Close_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Close_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Close_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Read_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Read_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Read_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Read_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Read_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Read_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Write_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Write_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_Write_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_GetPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_GetPosition_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_GetPosition_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_GetPosition_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_GetPosition_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_GetPosition_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_SetPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_SetPosition_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_SetPosition_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_LastUpdateTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_LastUpdateTime = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_LastUpdateTime, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenWithMasks_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenWithMasks_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenWithMasks_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenWithMasks_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenWithMasks_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenWithMasks_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_CloseAndUpdate_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_CloseAndUpdate_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_CloseAndUpdate_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_CloseAndUpdate_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_CloseAndUpdate_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_CloseAndUpdate_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_AddCertificate_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_AddCertificate_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_AddCertificate_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_RemoveCertificate_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_RemoveCertificate_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_TrustList_RemoveCertificate_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_CertificateTypes Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_CertificateTypes = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_CertificateGroups_DefaultUserTokenGroup_CertificateTypes, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_StartSigningRequest_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_StartSigningRequest_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_StartSigningRequest_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_StartSigningRequest_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_StartSigningRequest_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_StartSigningRequest_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_StartNewKeyPairRequest_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_StartNewKeyPairRequest_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_StartNewKeyPairRequest_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_StartNewKeyPairRequest_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_StartNewKeyPairRequest_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_StartNewKeyPairRequest_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_FinishRequest_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_FinishRequest_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_FinishRequest_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_FinishRequest_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_FinishRequest_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_FinishRequest_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_RevokeCertificate_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_RevokeCertificate_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_RevokeCertificate_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_GetCertificateGroups_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_GetCertificateGroups_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_GetCertificateGroups_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_GetCertificateGroups_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_GetCertificateGroups_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_GetCertificateGroups_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_GetTrustList_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_GetTrustList_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_GetTrustList_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_GetTrustList_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_GetTrustList_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_GetTrustList_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_GetCertificateStatus_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_GetCertificateStatus_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_GetCertificateStatus_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDirectoryType_GetCertificateStatus_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDirectoryType_GetCertificateStatus_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDirectoryType_GetCertificateStatus_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateRequestedAuditEventType_CertificateGroup Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateRequestedAuditEventType_CertificateGroup = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateRequestedAuditEventType_CertificateGroup, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateRequestedAuditEventType_CertificateType Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateRequestedAuditEventType_CertificateType = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateRequestedAuditEventType_CertificateType, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDeliveredAuditEventType_CertificateGroup Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDeliveredAuditEventType_CertificateGroup = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDeliveredAuditEventType_CertificateGroup, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the CertificateDeliveredAuditEventType_CertificateType Variable.
        /// </summary>
        public static readonly ExpandedNodeId CertificateDeliveredAuditEventType_CertificateType = new ExpandedNodeId(Opc.Ua.Gds.Variables.CertificateDeliveredAuditEventType_CertificateType, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the KeyCredentialManagement_ServiceName_Placeholder_ResourceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId KeyCredentialManagement_ServiceName_Placeholder_ResourceUri = new ExpandedNodeId(Opc.Ua.Gds.Variables.KeyCredentialManagement_ServiceName_Placeholder_ResourceUri, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the KeyCredentialManagement_ServiceName_Placeholder_ProfileUris Variable.
        /// </summary>
        public static readonly ExpandedNodeId KeyCredentialManagement_ServiceName_Placeholder_ProfileUris = new ExpandedNodeId(Opc.Ua.Gds.Variables.KeyCredentialManagement_ServiceName_Placeholder_ProfileUris, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the KeyCredentialManagement_ServiceName_Placeholder_StartRequest_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId KeyCredentialManagement_ServiceName_Placeholder_StartRequest_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.KeyCredentialManagement_ServiceName_Placeholder_StartRequest_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the KeyCredentialManagement_ServiceName_Placeholder_StartRequest_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId KeyCredentialManagement_ServiceName_Placeholder_StartRequest_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.KeyCredentialManagement_ServiceName_Placeholder_StartRequest_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the KeyCredentialManagement_ServiceName_Placeholder_FinishRequest_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId KeyCredentialManagement_ServiceName_Placeholder_FinishRequest_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.KeyCredentialManagement_ServiceName_Placeholder_FinishRequest_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the KeyCredentialManagement_ServiceName_Placeholder_FinishRequest_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId KeyCredentialManagement_ServiceName_Placeholder_FinishRequest_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.KeyCredentialManagement_ServiceName_Placeholder_FinishRequest_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the KeyCredentialManagement_ServiceName_Placeholder_Revoke_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId KeyCredentialManagement_ServiceName_Placeholder_Revoke_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.KeyCredentialManagement_ServiceName_Placeholder_Revoke_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the KeyCredentialServiceType_ResourceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId KeyCredentialServiceType_ResourceUri = new ExpandedNodeId(Opc.Ua.Gds.Variables.KeyCredentialServiceType_ResourceUri, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the KeyCredentialServiceType_ProfileUris Variable.
        /// </summary>
        public static readonly ExpandedNodeId KeyCredentialServiceType_ProfileUris = new ExpandedNodeId(Opc.Ua.Gds.Variables.KeyCredentialServiceType_ProfileUris, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the KeyCredentialServiceType_StartRequest_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId KeyCredentialServiceType_StartRequest_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.KeyCredentialServiceType_StartRequest_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the KeyCredentialServiceType_StartRequest_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId KeyCredentialServiceType_StartRequest_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.KeyCredentialServiceType_StartRequest_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the KeyCredentialServiceType_FinishRequest_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId KeyCredentialServiceType_FinishRequest_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.KeyCredentialServiceType_FinishRequest_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the KeyCredentialServiceType_FinishRequest_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId KeyCredentialServiceType_FinishRequest_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.KeyCredentialServiceType_FinishRequest_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the KeyCredentialServiceType_Revoke_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId KeyCredentialServiceType_Revoke_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.KeyCredentialServiceType_Revoke_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the AuthorizationServices_ServiceName_Placeholder_ServiceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId AuthorizationServices_ServiceName_Placeholder_ServiceUri = new ExpandedNodeId(Opc.Ua.Gds.Variables.AuthorizationServices_ServiceName_Placeholder_ServiceUri, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the AuthorizationServices_ServiceName_Placeholder_ServiceCertificate Variable.
        /// </summary>
        public static readonly ExpandedNodeId AuthorizationServices_ServiceName_Placeholder_ServiceCertificate = new ExpandedNodeId(Opc.Ua.Gds.Variables.AuthorizationServices_ServiceName_Placeholder_ServiceCertificate, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the AuthorizationServices_ServiceName_Placeholder_GetServiceDescription_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AuthorizationServices_ServiceName_Placeholder_GetServiceDescription_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.AuthorizationServices_ServiceName_Placeholder_GetServiceDescription_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the AuthorizationServices_ServiceName_Placeholder_RequestAccessToken_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AuthorizationServices_ServiceName_Placeholder_RequestAccessToken_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.AuthorizationServices_ServiceName_Placeholder_RequestAccessToken_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the AuthorizationServices_ServiceName_Placeholder_RequestAccessToken_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AuthorizationServices_ServiceName_Placeholder_RequestAccessToken_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.AuthorizationServices_ServiceName_Placeholder_RequestAccessToken_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the AuthorizationServiceType_ServiceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId AuthorizationServiceType_ServiceUri = new ExpandedNodeId(Opc.Ua.Gds.Variables.AuthorizationServiceType_ServiceUri, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the AuthorizationServiceType_ServiceCertificate Variable.
        /// </summary>
        public static readonly ExpandedNodeId AuthorizationServiceType_ServiceCertificate = new ExpandedNodeId(Opc.Ua.Gds.Variables.AuthorizationServiceType_ServiceCertificate, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the AuthorizationServiceType_UserTokenPolicies Variable.
        /// </summary>
        public static readonly ExpandedNodeId AuthorizationServiceType_UserTokenPolicies = new ExpandedNodeId(Opc.Ua.Gds.Variables.AuthorizationServiceType_UserTokenPolicies, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the AuthorizationServiceType_GetServiceDescription_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AuthorizationServiceType_GetServiceDescription_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.AuthorizationServiceType_GetServiceDescription_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the AuthorizationServiceType_RequestAccessToken_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AuthorizationServiceType_RequestAccessToken_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.AuthorizationServiceType_RequestAccessToken_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the AuthorizationServiceType_RequestAccessToken_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId AuthorizationServiceType_RequestAccessToken_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.AuthorizationServiceType_RequestAccessToken_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_FindApplications_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_FindApplications_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_FindApplications_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_FindApplications_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_FindApplications_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_FindApplications_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_RegisterApplication_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_RegisterApplication_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_RegisterApplication_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_RegisterApplication_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_RegisterApplication_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_RegisterApplication_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_UpdateApplication_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_UpdateApplication_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_UpdateApplication_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_UnregisterApplication_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_UnregisterApplication_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_UnregisterApplication_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_GetApplication_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_GetApplication_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_GetApplication_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_GetApplication_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_GetApplication_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_GetApplication_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_QueryApplications_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_QueryApplications_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_QueryApplications_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_QueryApplications_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_QueryApplications_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_QueryApplications_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_QueryServers_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_QueryServers_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_QueryServers_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_QueryServers_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_QueryServers_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_QueryServers_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Size Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Size = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Size, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Writable Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Writable = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Writable, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_UserWritable Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList_UserWritable = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultApplicationGroup_TrustList_UserWritable, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_OpenCount Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList_OpenCount = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultApplicationGroup_TrustList_OpenCount, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Open_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Open_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Open_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Open_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Open_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Open_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Close_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Close_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Close_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Read_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Read_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Read_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Read_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Read_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Read_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Write_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Write_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultApplicationGroup_TrustList_Write_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_GetPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList_GetPosition_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultApplicationGroup_TrustList_GetPosition_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_GetPosition_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList_GetPosition_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultApplicationGroup_TrustList_GetPosition_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_SetPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList_SetPosition_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultApplicationGroup_TrustList_SetPosition_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_LastUpdateTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList_LastUpdateTime = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultApplicationGroup_TrustList_LastUpdateTime, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_OpenWithMasks_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList_OpenWithMasks_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultApplicationGroup_TrustList_OpenWithMasks_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_OpenWithMasks_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList_OpenWithMasks_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultApplicationGroup_TrustList_OpenWithMasks_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_CloseAndUpdate_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList_CloseAndUpdate_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultApplicationGroup_TrustList_CloseAndUpdate_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_CloseAndUpdate_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList_CloseAndUpdate_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultApplicationGroup_TrustList_CloseAndUpdate_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_AddCertificate_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList_AddCertificate_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultApplicationGroup_TrustList_AddCertificate_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_TrustList_RemoveCertificate_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_TrustList_RemoveCertificate_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultApplicationGroup_TrustList_RemoveCertificate_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultApplicationGroup_CertificateTypes Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultApplicationGroup_CertificateTypes = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultApplicationGroup_CertificateTypes, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Size Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Size = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Size, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Writable Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Writable = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Writable, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_UserWritable Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList_UserWritable = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultHttpsGroup_TrustList_UserWritable, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_OpenCount Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList_OpenCount = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultHttpsGroup_TrustList_OpenCount, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Open_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Open_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Open_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Open_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Open_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Open_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Close_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Close_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Close_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Read_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Read_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Read_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Read_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Read_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Read_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Write_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Write_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultHttpsGroup_TrustList_Write_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_GetPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList_GetPosition_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultHttpsGroup_TrustList_GetPosition_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_GetPosition_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList_GetPosition_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultHttpsGroup_TrustList_GetPosition_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_SetPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList_SetPosition_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultHttpsGroup_TrustList_SetPosition_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_LastUpdateTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList_LastUpdateTime = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultHttpsGroup_TrustList_LastUpdateTime, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_OpenWithMasks_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList_OpenWithMasks_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultHttpsGroup_TrustList_OpenWithMasks_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_OpenWithMasks_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList_OpenWithMasks_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultHttpsGroup_TrustList_OpenWithMasks_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_CloseAndUpdate_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList_CloseAndUpdate_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultHttpsGroup_TrustList_CloseAndUpdate_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_CloseAndUpdate_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList_CloseAndUpdate_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultHttpsGroup_TrustList_CloseAndUpdate_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_AddCertificate_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList_AddCertificate_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultHttpsGroup_TrustList_AddCertificate_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_TrustList_RemoveCertificate_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_TrustList_RemoveCertificate_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultHttpsGroup_TrustList_RemoveCertificate_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultHttpsGroup_CertificateTypes Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultHttpsGroup_CertificateTypes = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultHttpsGroup_CertificateTypes, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Size Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Size = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Size, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Writable Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Writable = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Writable, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_UserWritable Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_UserWritable = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_UserWritable, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenCount Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenCount = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenCount, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Open_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Open_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Open_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Open_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Open_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Open_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Close_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Close_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Close_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Read_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Read_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Read_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Read_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Read_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Read_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Write_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Write_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_Write_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_GetPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_GetPosition_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_GetPosition_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_GetPosition_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_GetPosition_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_GetPosition_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_SetPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_SetPosition_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_SetPosition_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_LastUpdateTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_LastUpdateTime = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_LastUpdateTime, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenWithMasks_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenWithMasks_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenWithMasks_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenWithMasks_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenWithMasks_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_OpenWithMasks_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_CloseAndUpdate_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_CloseAndUpdate_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_CloseAndUpdate_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_CloseAndUpdate_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_CloseAndUpdate_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_CloseAndUpdate_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_AddCertificate_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_AddCertificate_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_AddCertificate_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_RemoveCertificate_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_RemoveCertificate_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultUserTokenGroup_TrustList_RemoveCertificate_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_CertificateGroups_DefaultUserTokenGroup_CertificateTypes Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_CertificateGroups_DefaultUserTokenGroup_CertificateTypes = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_CertificateGroups_DefaultUserTokenGroup_CertificateTypes, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_StartSigningRequest_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_StartSigningRequest_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_StartSigningRequest_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_StartSigningRequest_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_StartSigningRequest_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_StartSigningRequest_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_StartNewKeyPairRequest_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_StartNewKeyPairRequest_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_StartNewKeyPairRequest_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_StartNewKeyPairRequest_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_StartNewKeyPairRequest_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_StartNewKeyPairRequest_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_FinishRequest_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_FinishRequest_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_FinishRequest_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_FinishRequest_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_FinishRequest_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_FinishRequest_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_RevokeCertificate_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_RevokeCertificate_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_RevokeCertificate_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_GetCertificateGroups_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_GetCertificateGroups_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_GetCertificateGroups_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_GetCertificateGroups_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_GetCertificateGroups_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_GetCertificateGroups_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_GetTrustList_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_GetTrustList_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_GetTrustList_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_GetTrustList_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_GetTrustList_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_GetTrustList_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_GetCertificateStatus_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_GetCertificateStatus_InputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_GetCertificateStatus_InputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the Directory_GetCertificateStatus_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId Directory_GetCertificateStatus_OutputArguments = new ExpandedNodeId(Opc.Ua.Gds.Variables.Directory_GetCertificateStatus_OutputArguments, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OpcUaGds_BinarySchema Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaGds_BinarySchema = new ExpandedNodeId(Opc.Ua.Gds.Variables.OpcUaGds_BinarySchema, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OpcUaGds_BinarySchema_NamespaceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaGds_BinarySchema_NamespaceUri = new ExpandedNodeId(Opc.Ua.Gds.Variables.OpcUaGds_BinarySchema_NamespaceUri, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OpcUaGds_BinarySchema_Deprecated Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaGds_BinarySchema_Deprecated = new ExpandedNodeId(Opc.Ua.Gds.Variables.OpcUaGds_BinarySchema_Deprecated, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OpcUaGds_BinarySchema_ApplicationRecordDataType Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaGds_BinarySchema_ApplicationRecordDataType = new ExpandedNodeId(Opc.Ua.Gds.Variables.OpcUaGds_BinarySchema_ApplicationRecordDataType, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OpcUaGds_XmlSchema Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaGds_XmlSchema = new ExpandedNodeId(Opc.Ua.Gds.Variables.OpcUaGds_XmlSchema, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OpcUaGds_XmlSchema_NamespaceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaGds_XmlSchema_NamespaceUri = new ExpandedNodeId(Opc.Ua.Gds.Variables.OpcUaGds_XmlSchema_NamespaceUri, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OpcUaGds_XmlSchema_Deprecated Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaGds_XmlSchema_Deprecated = new ExpandedNodeId(Opc.Ua.Gds.Variables.OpcUaGds_XmlSchema_Deprecated, Opc.Ua.Gds.Namespaces.OpcUaGds);

        /// <summary>
        /// The identifier for the OpcUaGds_XmlSchema_ApplicationRecordDataType Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaGds_XmlSchema_ApplicationRecordDataType = new ExpandedNodeId(Opc.Ua.Gds.Variables.OpcUaGds_XmlSchema_ApplicationRecordDataType, Opc.Ua.Gds.Namespaces.OpcUaGds);
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
        /// The BrowseName for the AccessTokenAuditIssuedAuditEventType component.
        /// </summary>
        public const string AccessTokenAuditIssuedAuditEventType = "AccessTokenAuditIssuedAuditEventType";

        /// <summary>
        /// The BrowseName for the ApplicationRecordDataType component.
        /// </summary>
        public const string ApplicationRecordDataType = "ApplicationRecordDataType";

        /// <summary>
        /// The BrowseName for the ApplicationRegistrationChangedAuditEventType component.
        /// </summary>
        public const string ApplicationRegistrationChangedAuditEventType = "ApplicationRegistrationChangedAuditEventType";

        /// <summary>
        /// The BrowseName for the Applications component.
        /// </summary>
        public const string Applications = "Applications";

        /// <summary>
        /// The BrowseName for the AuthorizationServices component.
        /// </summary>
        public const string AuthorizationServices = "AuthorizationServices";

        /// <summary>
        /// The BrowseName for the AuthorizationServiceType component.
        /// </summary>
        public const string AuthorizationServiceType = "AuthorizationServiceType";

        /// <summary>
        /// The BrowseName for the CertificateDeliveredAuditEventType component.
        /// </summary>
        public const string CertificateDeliveredAuditEventType = "CertificateDeliveredAuditEventType";

        /// <summary>
        /// The BrowseName for the CertificateDirectoryType component.
        /// </summary>
        public const string CertificateDirectoryType = "CertificateDirectoryType";

        /// <summary>
        /// The BrowseName for the CertificateGroup component.
        /// </summary>
        public const string CertificateGroup = "CertificateGroup";

        /// <summary>
        /// The BrowseName for the CertificateGroups component.
        /// </summary>
        public const string CertificateGroups = "CertificateGroups";

        /// <summary>
        /// The BrowseName for the CertificateRequestedAuditEventType component.
        /// </summary>
        public const string CertificateRequestedAuditEventType = "CertificateRequestedAuditEventType";

        /// <summary>
        /// The BrowseName for the CertificateType component.
        /// </summary>
        public const string CertificateType = "CertificateType";

        /// <summary>
        /// The BrowseName for the Directory component.
        /// </summary>
        public const string Directory = "Directory";

        /// <summary>
        /// The BrowseName for the DirectoryType component.
        /// </summary>
        public const string DirectoryType = "DirectoryType";

        /// <summary>
        /// The BrowseName for the FindApplications component.
        /// </summary>
        public const string FindApplications = "FindApplications";

        /// <summary>
        /// The BrowseName for the FinishRequest component.
        /// </summary>
        public const string FinishRequest = "FinishRequest";

        /// <summary>
        /// The BrowseName for the GetApplication component.
        /// </summary>
        public const string GetApplication = "GetApplication";

        /// <summary>
        /// The BrowseName for the GetCertificateGroups component.
        /// </summary>
        public const string GetCertificateGroups = "GetCertificateGroups";

        /// <summary>
        /// The BrowseName for the GetCertificateStatus component.
        /// </summary>
        public const string GetCertificateStatus = "GetCertificateStatus";

        /// <summary>
        /// The BrowseName for the GetServiceDescription component.
        /// </summary>
        public const string GetServiceDescription = "GetServiceDescription";

        /// <summary>
        /// The BrowseName for the GetTrustList component.
        /// </summary>
        public const string GetTrustList = "GetTrustList";

        /// <summary>
        /// The BrowseName for the KeyCredentialDeliveredAuditEventType component.
        /// </summary>
        public const string KeyCredentialDeliveredAuditEventType = "KeyCredentialDeliveredAuditEventType";

        /// <summary>
        /// The BrowseName for the KeyCredentialManagement component.
        /// </summary>
        public const string KeyCredentialManagement = "KeyCredentialManagement";

        /// <summary>
        /// The BrowseName for the KeyCredentialRequestedAuditEventType component.
        /// </summary>
        public const string KeyCredentialRequestedAuditEventType = "KeyCredentialRequestedAuditEventType";

        /// <summary>
        /// The BrowseName for the KeyCredentialRevokedAuditEventType component.
        /// </summary>
        public const string KeyCredentialRevokedAuditEventType = "KeyCredentialRevokedAuditEventType";

        /// <summary>
        /// The BrowseName for the KeyCredentialServiceType component.
        /// </summary>
        public const string KeyCredentialServiceType = "KeyCredentialServiceType";

        /// <summary>
        /// The BrowseName for the OpcUaGds_BinarySchema component.
        /// </summary>
        public const string OpcUaGds_BinarySchema = "Opc.Ua.Gds";

        /// <summary>
        /// The BrowseName for the OpcUaGds_XmlSchema component.
        /// </summary>
        public const string OpcUaGds_XmlSchema = "Opc.Ua.Gds";

        /// <summary>
        /// The BrowseName for the OPCUAGDSNamespaceMetadata component.
        /// </summary>
        public const string OPCUAGDSNamespaceMetadata = "http://opcfoundation.org/UA/GDS/";

        /// <summary>
        /// The BrowseName for the ProfileUris component.
        /// </summary>
        public const string ProfileUris = "ProfileUris";

        /// <summary>
        /// The BrowseName for the QueryApplications component.
        /// </summary>
        public const string QueryApplications = "QueryApplications";

        /// <summary>
        /// The BrowseName for the QueryServers component.
        /// </summary>
        public const string QueryServers = "QueryServers";

        /// <summary>
        /// The BrowseName for the RegisterApplication component.
        /// </summary>
        public const string RegisterApplication = "RegisterApplication";

        /// <summary>
        /// The BrowseName for the RequestAccessToken component.
        /// </summary>
        public const string RequestAccessToken = "RequestAccessToken";

        /// <summary>
        /// The BrowseName for the ResourceUri component.
        /// </summary>
        public const string ResourceUri = "ResourceUri";

        /// <summary>
        /// The BrowseName for the Revoke component.
        /// </summary>
        public const string Revoke = "Revoke";

        /// <summary>
        /// The BrowseName for the RevokeCertificate component.
        /// </summary>
        public const string RevokeCertificate = "RevokeCertificate";

        /// <summary>
        /// The BrowseName for the ServiceCertificate component.
        /// </summary>
        public const string ServiceCertificate = "ServiceCertificate";

        /// <summary>
        /// The BrowseName for the ServiceName_Placeholder component.
        /// </summary>
        public const string ServiceName_Placeholder = "<ServiceName>";

        /// <summary>
        /// The BrowseName for the ServiceUri component.
        /// </summary>
        public const string ServiceUri = "ServiceUri";

        /// <summary>
        /// The BrowseName for the StartNewKeyPairRequest component.
        /// </summary>
        public const string StartNewKeyPairRequest = "StartNewKeyPairRequest";

        /// <summary>
        /// The BrowseName for the StartRequest component.
        /// </summary>
        public const string StartRequest = "StartRequest";

        /// <summary>
        /// The BrowseName for the StartSigningRequest component.
        /// </summary>
        public const string StartSigningRequest = "StartSigningRequest";

        /// <summary>
        /// The BrowseName for the UnregisterApplication component.
        /// </summary>
        public const string UnregisterApplication = "UnregisterApplication";

        /// <summary>
        /// The BrowseName for the UpdateApplication component.
        /// </summary>
        public const string UpdateApplication = "UpdateApplication";

        /// <summary>
        /// The BrowseName for the UserTokenPolicies component.
        /// </summary>
        public const string UserTokenPolicies = "UserTokenPolicies";
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
        /// The URI for the OpcUaGds namespace (.NET code namespace is 'Opc.Ua.Gds').
        /// </summary>
        public const string OpcUaGds = "http://opcfoundation.org/UA/GDS/";

        /// <summary>
        /// The URI for the OpcUaGdsXsd namespace (.NET code namespace is 'Opc.Ua.Gds').
        /// </summary>
        public const string OpcUaGdsXsd = "http://opcfoundation.org/UA/GDS/Types.xsd";

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
