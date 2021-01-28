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

namespace Opc.Ua.Fdi7
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
        /// The identifier for the EddDataTypeEnum DataType.
        /// </summary>
        public const uint EddDataTypeEnum = 2048;

        /// <summary>
        /// The identifier for the EddDataTypeInfo DataType.
        /// </summary>
        public const uint EddDataTypeInfo = 2050;
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
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_Open Method.
        /// </summary>
        public const uint OpcUaFdi7NamespaceMetadata_NamespaceFile_Open = 15023;

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_Close Method.
        /// </summary>
        public const uint OpcUaFdi7NamespaceMetadata_NamespaceFile_Close = 15026;

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_Read Method.
        /// </summary>
        public const uint OpcUaFdi7NamespaceMetadata_NamespaceFile_Read = 15028;

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_Write Method.
        /// </summary>
        public const uint OpcUaFdi7NamespaceMetadata_NamespaceFile_Write = 15031;

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_GetPosition Method.
        /// </summary>
        public const uint OpcUaFdi7NamespaceMetadata_NamespaceFile_GetPosition = 15033;

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_SetPosition Method.
        /// </summary>
        public const uint OpcUaFdi7NamespaceMetadata_NamespaceFile_SetPosition = 15036;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_Lock_InitLock Method.
        /// </summary>
        public const uint ConnectionPoint_Foundation_H1_Lock_InitLock = 1394;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_Lock_RenewLock Method.
        /// </summary>
        public const uint ConnectionPoint_Foundation_H1_Lock_RenewLock = 1397;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_Lock_ExitLock Method.
        /// </summary>
        public const uint ConnectionPoint_Foundation_H1_Lock_ExitLock = 1399;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_Lock_BreakLock Method.
        /// </summary>
        public const uint ConnectionPoint_Foundation_H1_Lock_BreakLock = 1401;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_InitLock = 1412;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_RenewLock = 1415;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_ExitLock = 1417;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_BreakLock = 1419;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_Lock_InitLock Method.
        /// </summary>
        public const uint ConnectionPoint_Foundation_HSE_Lock_InitLock = 1438;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_Lock_RenewLock Method.
        /// </summary>
        public const uint ConnectionPoint_Foundation_HSE_Lock_RenewLock = 1441;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_Lock_ExitLock Method.
        /// </summary>
        public const uint ConnectionPoint_Foundation_HSE_Lock_ExitLock = 1443;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_Lock_BreakLock Method.
        /// </summary>
        public const uint ConnectionPoint_Foundation_HSE_Lock_BreakLock = 1445;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_InitLock = 1456;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_RenewLock = 1459;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_ExitLock = 1461;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_BreakLock = 1463;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_Lock_InitLock Method.
        /// </summary>
        public const uint ConnectionPoint_Profibus_DP_Lock_InitLock = 1481;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_Lock_RenewLock Method.
        /// </summary>
        public const uint ConnectionPoint_Profibus_DP_Lock_RenewLock = 1484;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_Lock_ExitLock Method.
        /// </summary>
        public const uint ConnectionPoint_Profibus_DP_Lock_ExitLock = 1486;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_Lock_BreakLock Method.
        /// </summary>
        public const uint ConnectionPoint_Profibus_DP_Lock_BreakLock = 1488;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_InitLock = 1499;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_RenewLock = 1502;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_ExitLock = 1504;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_BreakLock = 1506;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_Lock_InitLock Method.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO_Lock_InitLock = 1523;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_Lock_RenewLock Method.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO_Lock_RenewLock = 1526;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_Lock_ExitLock Method.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO_Lock_ExitLock = 1528;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_Lock_BreakLock Method.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO_Lock_BreakLock = 1530;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_InitLock = 1541;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_RenewLock = 1544;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_ExitLock = 1546;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_BreakLock = 1548;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_Lock_InitLock Method.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_Lock_InitLock = 1568;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_Lock_RenewLock Method.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_Lock_RenewLock = 1571;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_Lock_ExitLock Method.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_Lock_ExitLock = 1573;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_Lock_BreakLock Method.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_Lock_BreakLock = 1575;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_InitLock = 1586;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_RenewLock = 1589;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_ExitLock = 1591;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_BreakLock = 1593;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_Lock_InitLock Method.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_Lock_InitLock = 1615;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_Lock_RenewLock Method.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_Lock_RenewLock = 1618;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_Lock_ExitLock Method.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_Lock_ExitLock = 1620;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_Lock_BreakLock Method.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_Lock_BreakLock = 1622;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_InitLock = 1633;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_RenewLock = 1636;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_ExitLock = 1638;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_BreakLock = 1640;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_Lock_InitLock Method.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_Lock_InitLock = 1662;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_Lock_RenewLock Method.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_Lock_RenewLock = 1665;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_Lock_ExitLock Method.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_Lock_ExitLock = 1667;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_Lock_BreakLock Method.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_Lock_BreakLock = 1669;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_InitLock = 1680;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_RenewLock = 1683;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_ExitLock = 1685;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_BreakLock = 1687;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_Lock_InitLock Method.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_Lock_InitLock = 1709;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_Lock_RenewLock Method.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_Lock_RenewLock = 1712;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_Lock_ExitLock Method.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_Lock_ExitLock = 1714;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_Lock_BreakLock Method.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_Lock_BreakLock = 1716;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_InitLock = 1727;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_RenewLock = 1730;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_ExitLock = 1732;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_BreakLock = 1734;

        /// <summary>
        /// The identifier for the GenericConnectionPoint_Lock_InitLock Method.
        /// </summary>
        public const uint GenericConnectionPoint_Lock_InitLock = 1756;

        /// <summary>
        /// The identifier for the GenericConnectionPoint_Lock_RenewLock Method.
        /// </summary>
        public const uint GenericConnectionPoint_Lock_RenewLock = 1759;

        /// <summary>
        /// The identifier for the GenericConnectionPoint_Lock_ExitLock Method.
        /// </summary>
        public const uint GenericConnectionPoint_Lock_ExitLock = 1761;

        /// <summary>
        /// The identifier for the GenericConnectionPoint_Lock_BreakLock Method.
        /// </summary>
        public const uint GenericConnectionPoint_Lock_BreakLock = 1763;

        /// <summary>
        /// The identifier for the GenericConnectionPoint_NetworkIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint GenericConnectionPoint_NetworkIdentifier_Lock_InitLock = 1774;

        /// <summary>
        /// The identifier for the GenericConnectionPoint_NetworkIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint GenericConnectionPoint_NetworkIdentifier_Lock_RenewLock = 1777;

        /// <summary>
        /// The identifier for the GenericConnectionPoint_NetworkIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint GenericConnectionPoint_NetworkIdentifier_Lock_ExitLock = 1779;

        /// <summary>
        /// The identifier for the GenericConnectionPoint_NetworkIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint GenericConnectionPoint_NetworkIdentifier_Lock_BreakLock = 1781;

        /// <summary>
        /// The identifier for the CommunicationServerType_Lock_InitLock Method.
        /// </summary>
        public const uint CommunicationServerType_Lock_InitLock = 25;

        /// <summary>
        /// The identifier for the CommunicationServerType_Lock_RenewLock Method.
        /// </summary>
        public const uint CommunicationServerType_Lock_RenewLock = 28;

        /// <summary>
        /// The identifier for the CommunicationServerType_Lock_ExitLock Method.
        /// </summary>
        public const uint CommunicationServerType_Lock_ExitLock = 30;

        /// <summary>
        /// The identifier for the CommunicationServerType_Lock_BreakLock Method.
        /// </summary>
        public const uint CommunicationServerType_Lock_BreakLock = 32;

        /// <summary>
        /// The identifier for the CommunicationServerType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint CommunicationServerType_CPIdentifier_Lock_InitLock = 64;

        /// <summary>
        /// The identifier for the CommunicationServerType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint CommunicationServerType_CPIdentifier_Lock_RenewLock = 67;

        /// <summary>
        /// The identifier for the CommunicationServerType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint CommunicationServerType_CPIdentifier_Lock_ExitLock = 69;

        /// <summary>
        /// The identifier for the CommunicationServerType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint CommunicationServerType_CPIdentifier_Lock_BreakLock = 71;

        /// <summary>
        /// The identifier for the CommunicationServerType_MethodSet_Initialize Method.
        /// </summary>
        public const uint CommunicationServerType_MethodSet_Initialize = 75;

        /// <summary>
        /// The identifier for the CommunicationServerType_MethodSet_Reset Method.
        /// </summary>
        public const uint CommunicationServerType_MethodSet_Reset = 77;

        /// <summary>
        /// The identifier for the CommunicationServerType_MethodSet_AddComponent Method.
        /// </summary>
        public const uint CommunicationServerType_MethodSet_AddComponent = 79;

        /// <summary>
        /// The identifier for the CommunicationServerType_MethodSet_RemoveComponent Method.
        /// </summary>
        public const uint CommunicationServerType_MethodSet_RemoveComponent = 82;

        /// <summary>
        /// The identifier for the SetAddressMethodFFH1Type Method.
        /// </summary>
        public const uint SetAddressMethodFFH1Type = 309;

        /// <summary>
        /// The identifier for the SetAddressMethodFFHSEType Method.
        /// </summary>
        public const uint SetAddressMethodFFHSEType = 312;

        /// <summary>
        /// The identifier for the SetAddressMethodPROFIBUSType Method.
        /// </summary>
        public const uint SetAddressMethodPROFIBUSType = 315;

        /// <summary>
        /// The identifier for the SetAddressMethodPROFINETType Method.
        /// </summary>
        public const uint SetAddressMethodPROFINETType = 318;

        /// <summary>
        /// The identifier for the SetAddressMethodHARTType Method.
        /// </summary>
        public const uint SetAddressMethodHARTType = 321;

        /// <summary>
        /// The identifier for the SetAddressMethodGENERICType Method.
        /// </summary>
        public const uint SetAddressMethodGENERICType = 1785;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationDeviceType_Lock_InitLock = 107;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationDeviceType_Lock_RenewLock = 110;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationDeviceType_Lock_ExitLock = 112;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationDeviceType_Lock_BreakLock = 114;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationDeviceType_CPIdentifier_Lock_InitLock = 146;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationDeviceType_CPIdentifier_Lock_RenewLock = 149;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationDeviceType_CPIdentifier_Lock_ExitLock = 151;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationDeviceType_CPIdentifier_Lock_BreakLock = 153;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_MethodSet_Scan Method.
        /// </summary>
        public const uint ServerCommunicationDeviceType_MethodSet_Scan = 157;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_MethodSet_ResetScan Method.
        /// </summary>
        public const uint ServerCommunicationDeviceType_MethodSet_ResetScan = 159;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_MethodSet_AddComponent Method.
        /// </summary>
        public const uint ServerCommunicationDeviceType_MethodSet_AddComponent = 164;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_MethodSet_RemoveComponent Method.
        /// </summary>
        public const uint ServerCommunicationDeviceType_MethodSet_RemoveComponent = 167;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_Lock_InitLock = 338;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_Lock_RenewLock = 341;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_Lock_ExitLock = 343;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_Lock_BreakLock = 345;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_InitLock = 377;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_RenewLock = 380;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_ExitLock = 382;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_BreakLock = 384;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_MethodSet_SetAddress Method.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_MethodSet_SetAddress = 398;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_InitLock = 413;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_RenewLock = 416;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_ExitLock = 418;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_BreakLock = 420;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Disconnect Method.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Disconnect = 440;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Connect Method.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Connect = 443;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Transfer Method.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Transfer = 446;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_Lock_InitLock = 466;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_Lock_RenewLock = 469;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_Lock_ExitLock = 471;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_Lock_BreakLock = 473;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_InitLock = 505;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_RenewLock = 508;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_ExitLock = 510;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_BreakLock = 512;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_MethodSet_SetAddress Method.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_MethodSet_SetAddress = 526;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_InitLock = 541;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_RenewLock = 544;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_ExitLock = 546;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_BreakLock = 548;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Disconnect Method.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Disconnect = 568;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Connect Method.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Connect = 571;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Transfer Method.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Transfer = 574;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_Lock_InitLock = 594;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_Lock_RenewLock = 597;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_Lock_ExitLock = 599;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_Lock_BreakLock = 601;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_InitLock = 633;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_RenewLock = 636;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_ExitLock = 638;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_BreakLock = 640;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_MethodSet_SetAddress Method.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_MethodSet_SetAddress = 654;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_InitLock = 669;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_RenewLock = 672;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_ExitLock = 674;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_BreakLock = 676;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Disconnect Method.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Disconnect = 696;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Connect Method.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Connect = 699;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Transfer Method.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Transfer = 702;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_Lock_InitLock = 719;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_Lock_RenewLock = 722;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_Lock_ExitLock = 724;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_Lock_BreakLock = 726;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_InitLock = 758;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_RenewLock = 761;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_ExitLock = 763;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_BreakLock = 765;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_MethodSet_SetAddress Method.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_MethodSet_SetAddress = 779;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_InitLock = 794;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_RenewLock = 797;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_ExitLock = 799;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_BreakLock = 801;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Disconnect Method.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Disconnect = 821;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Connect Method.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Connect = 824;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Transfer Method.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Transfer = 827;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationHARType_Lock_InitLock = 844;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationHARType_Lock_RenewLock = 847;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationHARType_Lock_ExitLock = 849;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationHARType_Lock_BreakLock = 851;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationHARType_CPIdentifier_Lock_InitLock = 883;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationHARType_CPIdentifier_Lock_RenewLock = 886;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationHARType_CPIdentifier_Lock_ExitLock = 888;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationHARType_CPIdentifier_Lock_BreakLock = 890;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_MethodSet_SetAddress Method.
        /// </summary>
        public const uint ServerCommunicationHARType_MethodSet_SetAddress = 904;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_Lock_InitLock = 919;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_Lock_RenewLock = 922;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_Lock_ExitLock = 924;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_Lock_BreakLock = 926;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_MethodSet_Disconnect Method.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_MethodSet_Disconnect = 946;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_MethodSet_Connect Method.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_MethodSet_Connect = 949;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_MethodSet_Transfer Method.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_MethodSet_Transfer = 952;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_Lock_InitLock = 1802;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_Lock_RenewLock = 1805;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_Lock_ExitLock = 1807;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_Lock_BreakLock = 1809;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_InitLock = 1841;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_RenewLock = 1844;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_ExitLock = 1846;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_BreakLock = 1848;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_InitLock = 1874;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_RenewLock = 1877;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_ExitLock = 1879;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_BreakLock = 1881;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Disconnect Method.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Disconnect = 1901;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Connect Method.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Connect = 1904;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Transfer Method.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Transfer = 1907;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_Lock_InitLock = 1927;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_Lock_RenewLock = 1930;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_Lock_ExitLock = 1932;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_Lock_BreakLock = 1934;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_InitLock = 1966;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_RenewLock = 1969;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_ExitLock = 1971;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_BreakLock = 1973;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_MethodSet_SetAddress Method.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_MethodSet_SetAddress = 1988;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_InitLock = 2003;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_RenewLock = 2006;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_ExitLock = 2008;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_BreakLock = 2010;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Disconnect Method.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Disconnect = 2030;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Connect Method.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Connect = 2033;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Transfer Method.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Transfer = 2036;

        /// <summary>
        /// The identifier for the ConnectMethodFFH1Type Method.
        /// </summary>
        public const uint ConnectMethodFFH1Type = 958;

        /// <summary>
        /// The identifier for the ConnectMethodFFHSEType Method.
        /// </summary>
        public const uint ConnectMethodFFHSEType = 961;

        /// <summary>
        /// The identifier for the ConnectMethodPROFIBUSType Method.
        /// </summary>
        public const uint ConnectMethodPROFIBUSType = 964;

        /// <summary>
        /// The identifier for the ConnectMethodPROFINETType Method.
        /// </summary>
        public const uint ConnectMethodPROFINETType = 967;

        /// <summary>
        /// The identifier for the ConnectMethodHARTType Method.
        /// </summary>
        public const uint ConnectMethodHARTType = 970;

        /// <summary>
        /// The identifier for the ConnectMethodISA100_WirelessType Method.
        /// </summary>
        public const uint ConnectMethodISA100_WirelessType = 2039;

        /// <summary>
        /// The identifier for the ConnectMethodGENERICType Method.
        /// </summary>
        public const uint ConnectMethodGENERICType = 2042;

        /// <summary>
        /// The identifier for the TransferMethodFFH1Type Method.
        /// </summary>
        public const uint TransferMethodFFH1Type = 973;

        /// <summary>
        /// The identifier for the TransferMethodFFHSEType Method.
        /// </summary>
        public const uint TransferMethodFFHSEType = 976;

        /// <summary>
        /// The identifier for the TransferMethodPROFIBUSType Method.
        /// </summary>
        public const uint TransferMethodPROFIBUSType = 979;

        /// <summary>
        /// The identifier for the TransferMethodPROFINETType Method.
        /// </summary>
        public const uint TransferMethodPROFINETType = 982;

        /// <summary>
        /// The identifier for the TransferMethodHARTType Method.
        /// </summary>
        public const uint TransferMethodHARTType = 985;

        /// <summary>
        /// The identifier for the TransferMethodISA100_WirelessType Method.
        /// </summary>
        public const uint TransferMethodISA100_WirelessType = 2045;

        /// <summary>
        /// The identifier for the TransferMethodGENERICType Method.
        /// </summary>
        public const uint TransferMethodGENERICType = 2051;

        /// <summary>
        /// The identifier for the GetPublishedDataMethodFFH1Type Method.
        /// </summary>
        public const uint GetPublishedDataMethodFFH1Type = 988;

        /// <summary>
        /// The identifier for the GetPublishedDataMethodFFHSEType Method.
        /// </summary>
        public const uint GetPublishedDataMethodFFHSEType = 991;

        /// <summary>
        /// The identifier for the GetPublishedDataMethodHARTType Method.
        /// </summary>
        public const uint GetPublishedDataMethodHARTType = 994;

        /// <summary>
        /// The identifier for the GetPublishedDataMethodISA100_WirelessType Method.
        /// </summary>
        public const uint GetPublishedDataMethodISA100_WirelessType = 2054;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationServiceType_Lock_InitLock = 247;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationServiceType_Lock_RenewLock = 250;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationServiceType_Lock_ExitLock = 252;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationServiceType_Lock_BreakLock = 254;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationServiceType_CPIdentifier_Lock_InitLock = 286;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationServiceType_CPIdentifier_Lock_RenewLock = 289;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationServiceType_CPIdentifier_Lock_ExitLock = 291;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationServiceType_CPIdentifier_Lock_BreakLock = 293;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_MethodSet_Disconnect Method.
        /// </summary>
        public const uint ServerCommunicationServiceType_MethodSet_Disconnect = 300;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_Lock_InitLock = 1011;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_Lock_RenewLock = 1014;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_Lock_ExitLock = 1016;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_Lock_BreakLock = 1018;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_InitLock = 1050;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_RenewLock = 1053;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_ExitLock = 1055;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_BreakLock = 1057;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_MethodSet_Disconnect Method.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_MethodSet_Disconnect = 1061;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_MethodSet_Connect Method.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_MethodSet_Connect = 1064;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_MethodSet_Transfer Method.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_MethodSet_Transfer = 1067;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_MethodSet_GetPublishedData Method.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_MethodSet_GetPublishedData = 1070;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_Lock_InitLock = 1087;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_Lock_RenewLock = 1090;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_Lock_ExitLock = 1092;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_Lock_BreakLock = 1094;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_InitLock = 1126;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_RenewLock = 1129;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_ExitLock = 1131;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_BreakLock = 1133;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_MethodSet_Disconnect Method.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_MethodSet_Disconnect = 1137;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_MethodSet_Connect Method.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_MethodSet_Connect = 1140;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_MethodSet_Transfer Method.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_MethodSet_Transfer = 1143;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_MethodSet_GetPublishedData Method.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_MethodSet_GetPublishedData = 1146;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_Lock_InitLock = 1163;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_Lock_RenewLock = 1166;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_Lock_ExitLock = 1168;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_Lock_BreakLock = 1170;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_InitLock = 1202;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_RenewLock = 1205;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_ExitLock = 1207;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_BreakLock = 1209;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_MethodSet_Disconnect Method.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_MethodSet_Disconnect = 1213;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_MethodSet_Connect Method.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_MethodSet_Connect = 1216;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_MethodSet_Transfer Method.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_MethodSet_Transfer = 1219;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_Lock_InitLock = 1236;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_Lock_RenewLock = 1239;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_Lock_ExitLock = 1241;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_Lock_BreakLock = 1243;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_InitLock = 1275;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_RenewLock = 1278;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_ExitLock = 1280;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_BreakLock = 1282;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_MethodSet_Disconnect Method.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_MethodSet_Disconnect = 1286;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_MethodSet_Connect Method.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_MethodSet_Connect = 1289;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_MethodSet_Transfer Method.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_MethodSet_Transfer = 1292;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_Lock_InitLock = 1309;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_Lock_RenewLock = 1312;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_Lock_ExitLock = 1314;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_Lock_BreakLock = 1316;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_CPIdentifier_Lock_InitLock = 1348;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_CPIdentifier_Lock_RenewLock = 1351;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_CPIdentifier_Lock_ExitLock = 1353;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_CPIdentifier_Lock_BreakLock = 1355;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_MethodSet_Disconnect Method.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_MethodSet_Disconnect = 1359;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_MethodSet_Connect Method.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_MethodSet_Connect = 1362;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_MethodSet_Transfer Method.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_MethodSet_Transfer = 1365;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_MethodSet_GetPublishedData Method.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_MethodSet_GetPublishedData = 1368;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_Lock_InitLock = 2071;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_Lock_RenewLock = 2074;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_Lock_ExitLock = 2076;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_Lock_BreakLock = 2078;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_InitLock = 2110;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_RenewLock = 2113;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_ExitLock = 2115;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_BreakLock = 2117;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_MethodSet_Disconnect Method.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_MethodSet_Disconnect = 2121;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_MethodSet_Connect Method.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_MethodSet_Connect = 2124;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_MethodSet_Transfer Method.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_MethodSet_Transfer = 2127;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_MethodSet_GetPublishedData Method.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_MethodSet_GetPublishedData = 2130;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_Lock_InitLock = 2147;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_Lock_RenewLock = 2150;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_Lock_ExitLock = 2152;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_Lock_BreakLock = 2154;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_CPIdentifier_Lock_InitLock = 2186;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_CPIdentifier_Lock_RenewLock = 2189;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_CPIdentifier_Lock_ExitLock = 2191;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_CPIdentifier_Lock_BreakLock = 2193;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_MethodSet_Disconnect Method.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_MethodSet_Disconnect = 2197;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_MethodSet_Connect Method.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_MethodSet_Connect = 2200;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_MethodSet_Transfer Method.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_MethodSet_Transfer = 2203;
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
        /// The identifier for the OpcUaFdi7NamespaceMetadata Object.
        /// </summary>
        public const uint OpcUaFdi7NamespaceMetadata = 15009;

        /// <summary>
        /// The identifier for the CommunicationServerType_ParameterSet Object.
        /// </summary>
        public const uint CommunicationServerType_ParameterSet = 12;

        /// <summary>
        /// The identifier for the CommunicationServerType_MethodSet Object.
        /// </summary>
        public const uint CommunicationServerType_MethodSet = 14;

        /// <summary>
        /// The identifier for the CommunicationServerType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public const uint CommunicationServerType_CPIdentifier_NetworkAddress = 73;

        /// <summary>
        /// The identifier for the CommunicationServerType_SubDevices Object.
        /// </summary>
        public const uint CommunicationServerType_SubDevices = 85;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_MethodSet Object.
        /// </summary>
        public const uint ServerCommunicationDeviceType_MethodSet = 96;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public const uint ServerCommunicationDeviceType_CPIdentifier_NetworkAddress = 155;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_MethodSet Object.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_MethodSet = 327;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_CPIdentifier_NetworkAddress = 386;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider Object.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider = 401;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet Object.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet = 404;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_MethodSet Object.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_MethodSet = 455;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_CPIdentifier_NetworkAddress = 514;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider Object.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider = 529;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet Object.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet = 532;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_MethodSet Object.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_MethodSet = 583;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_CPIdentifier_NetworkAddress = 642;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider Object.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider = 657;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet Object.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet = 660;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_MethodSet Object.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_MethodSet = 708;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_CPIdentifier_NetworkAddress = 767;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider Object.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider = 782;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet Object.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet = 785;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_MethodSet Object.
        /// </summary>
        public const uint ServerCommunicationHARType_MethodSet = 833;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public const uint ServerCommunicationHARType_CPIdentifier_NetworkAddress = 892;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider Object.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider = 907;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_MethodSet Object.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_MethodSet = 910;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_NetworkAddress = 1850;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider Object.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider = 1862;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet Object.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet = 1865;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_MethodSet Object.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_MethodSet = 1916;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_CPIdentifier_NetworkAddress = 1975;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider Object.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider = 1991;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet Object.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet = 1994;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_MethodSet Object.
        /// </summary>
        public const uint ServerCommunicationServiceType_MethodSet = 236;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public const uint ServerCommunicationServiceType_CPIdentifier_NetworkAddress = 295;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_MethodSet Object.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_MethodSet = 1000;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_CPIdentifier_NetworkAddress = 1059;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_MethodSet Object.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_MethodSet = 1076;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_CPIdentifier_NetworkAddress = 1135;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_MethodSet Object.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_MethodSet = 1152;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_CPIdentifier_NetworkAddress = 1211;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_MethodSet Object.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_MethodSet = 1225;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_CPIdentifier_NetworkAddress = 1284;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_MethodSet Object.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_MethodSet = 1298;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_CPIdentifier_NetworkAddress = 1357;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_MethodSet Object.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_MethodSet = 2060;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_CPIdentifier_NetworkAddress = 2119;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_MethodSet Object.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_MethodSet = 2136;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_CPIdentifier_NetworkAddress = 2195;

        /// <summary>
        /// The identifier for the EddDataTypeInfo_Encoding_DefaultBinary Object.
        /// </summary>
        public const uint EddDataTypeInfo_Encoding_DefaultBinary = 2213;

        /// <summary>
        /// The identifier for the EddDataTypeInfo_Encoding_DefaultXml Object.
        /// </summary>
        public const uint EddDataTypeInfo_Encoding_DefaultXml = 2206;

        /// <summary>
        /// The identifier for the EddDataTypeInfo_Encoding_DefaultJson Object.
        /// </summary>
        public const uint EddDataTypeInfo_Encoding_DefaultJson = 8001;
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
        /// The identifier for the Foundation_H1 ObjectType.
        /// </summary>
        public const uint Foundation_H1 = 1371;

        /// <summary>
        /// The identifier for the Foundation_HSE ObjectType.
        /// </summary>
        public const uint Foundation_HSE = 1372;

        /// <summary>
        /// The identifier for the Profibus_DP ObjectType.
        /// </summary>
        public const uint Profibus_DP = 1373;

        /// <summary>
        /// The identifier for the Profibus_PA ObjectType.
        /// </summary>
        public const uint Profibus_PA = 1374;

        /// <summary>
        /// The identifier for the Profinet_IO ObjectType.
        /// </summary>
        public const uint Profinet_IO = 1375;

        /// <summary>
        /// The identifier for the HART ObjectType.
        /// </summary>
        public const uint HART = 1376;

        /// <summary>
        /// The identifier for the ISA100_Wireless ObjectType.
        /// </summary>
        public const uint ISA100_Wireless = 1377;

        /// <summary>
        /// The identifier for the GenericProtocol ObjectType.
        /// </summary>
        public const uint GenericProtocol = 1378;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1 ObjectType.
        /// </summary>
        public const uint ConnectionPoint_Foundation_H1 = 1380;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE ObjectType.
        /// </summary>
        public const uint ConnectionPoint_Foundation_HSE = 1424;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP ObjectType.
        /// </summary>
        public const uint ConnectionPoint_Profibus_DP = 1467;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO ObjectType.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO = 1509;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5 ObjectType.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5 = 1554;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6 ObjectType.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6 = 1601;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7 ObjectType.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7 = 1648;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless ObjectType.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless = 1695;

        /// <summary>
        /// The identifier for the GenericConnectionPoint ObjectType.
        /// </summary>
        public const uint GenericConnectionPoint = 1742;

        /// <summary>
        /// The identifier for the CommunicationServerType ObjectType.
        /// </summary>
        public const uint CommunicationServerType = 11;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType ObjectType.
        /// </summary>
        public const uint ServerCommunicationDeviceType = 93;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType ObjectType.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType = 324;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType ObjectType.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType = 452;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType ObjectType.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType = 580;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType ObjectType.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType = 705;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType ObjectType.
        /// </summary>
        public const uint ServerCommunicationHARType = 830;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType ObjectType.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType = 1788;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType ObjectType.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType = 1913;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType ObjectType.
        /// </summary>
        public const uint ServerCommunicationServiceType = 233;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType ObjectType.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType = 997;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType ObjectType.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType = 1073;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType ObjectType.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType = 1149;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType ObjectType.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType = 1222;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType ObjectType.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType = 1295;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType ObjectType.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType = 2057;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType ObjectType.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType = 2133;
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
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceUri Variable.
        /// </summary>
        public const uint OpcUaFdi7NamespaceMetadata_NamespaceUri = 15010;

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceVersion Variable.
        /// </summary>
        public const uint OpcUaFdi7NamespaceMetadata_NamespaceVersion = 15011;

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespacePublicationDate Variable.
        /// </summary>
        public const uint OpcUaFdi7NamespaceMetadata_NamespacePublicationDate = 15012;

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_IsNamespaceSubset Variable.
        /// </summary>
        public const uint OpcUaFdi7NamespaceMetadata_IsNamespaceSubset = 15013;

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_StaticNodeIdTypes Variable.
        /// </summary>
        public const uint OpcUaFdi7NamespaceMetadata_StaticNodeIdTypes = 15014;

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_StaticNumericNodeIdRange Variable.
        /// </summary>
        public const uint OpcUaFdi7NamespaceMetadata_StaticNumericNodeIdRange = 15015;

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_StaticStringNodeIdPattern Variable.
        /// </summary>
        public const uint OpcUaFdi7NamespaceMetadata_StaticStringNodeIdPattern = 15016;

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_Size Variable.
        /// </summary>
        public const uint OpcUaFdi7NamespaceMetadata_NamespaceFile_Size = 15018;

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_Writable Variable.
        /// </summary>
        public const uint OpcUaFdi7NamespaceMetadata_NamespaceFile_Writable = 15019;

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_UserWritable Variable.
        /// </summary>
        public const uint OpcUaFdi7NamespaceMetadata_NamespaceFile_UserWritable = 15020;

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_OpenCount Variable.
        /// </summary>
        public const uint OpcUaFdi7NamespaceMetadata_NamespaceFile_OpenCount = 15021;

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_Open_InputArguments Variable.
        /// </summary>
        public const uint OpcUaFdi7NamespaceMetadata_NamespaceFile_Open_InputArguments = 15024;

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_Open_OutputArguments Variable.
        /// </summary>
        public const uint OpcUaFdi7NamespaceMetadata_NamespaceFile_Open_OutputArguments = 15025;

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_Close_InputArguments Variable.
        /// </summary>
        public const uint OpcUaFdi7NamespaceMetadata_NamespaceFile_Close_InputArguments = 15027;

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_Read_InputArguments Variable.
        /// </summary>
        public const uint OpcUaFdi7NamespaceMetadata_NamespaceFile_Read_InputArguments = 15029;

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_Read_OutputArguments Variable.
        /// </summary>
        public const uint OpcUaFdi7NamespaceMetadata_NamespaceFile_Read_OutputArguments = 15030;

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_Write_InputArguments Variable.
        /// </summary>
        public const uint OpcUaFdi7NamespaceMetadata_NamespaceFile_Write_InputArguments = 15032;

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_GetPosition_InputArguments Variable.
        /// </summary>
        public const uint OpcUaFdi7NamespaceMetadata_NamespaceFile_GetPosition_InputArguments = 15034;

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_GetPosition_OutputArguments Variable.
        /// </summary>
        public const uint OpcUaFdi7NamespaceMetadata_NamespaceFile_GetPosition_OutputArguments = 15035;

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_SetPosition_InputArguments Variable.
        /// </summary>
        public const uint OpcUaFdi7NamespaceMetadata_NamespaceFile_SetPosition_InputArguments = 15037;

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_DefaultRolePermissions Variable.
        /// </summary>
        public const uint OpcUaFdi7NamespaceMetadata_DefaultRolePermissions = 15039;

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_DefaultUserRolePermissions Variable.
        /// </summary>
        public const uint OpcUaFdi7NamespaceMetadata_DefaultUserRolePermissions = 15040;

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_DefaultAccessRestrictions Variable.
        /// </summary>
        public const uint OpcUaFdi7NamespaceMetadata_DefaultAccessRestrictions = 15041;

        /// <summary>
        /// The identifier for the GenericProtocol_ProtocolIdentifier Variable.
        /// </summary>
        public const uint GenericProtocol_ProtocolIdentifier = 1379;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_Lock_Locked Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_H1_Lock_Locked = 1390;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_Lock_LockingClient Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_H1_Lock_LockingClient = 1391;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_Lock_LockingUser Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_H1_Lock_LockingUser = 1392;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_H1_Lock_RemainingLockTime = 1393;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_H1_Lock_InitLock_InputArguments = 1395;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_H1_Lock_InitLock_OutputArguments = 1396;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_H1_Lock_RenewLock_OutputArguments = 1398;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_H1_Lock_ExitLock_OutputArguments = 1400;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_H1_Lock_BreakLock_OutputArguments = 1402;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_Locked = 1408;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_LockingClient = 1409;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_LockingUser = 1410;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_RemainingLockTime = 1411;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_InitLock_InputArguments = 1413;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_InitLock_OutputArguments = 1414;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_RenewLock_OutputArguments = 1416;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_ExitLock_OutputArguments = 1418;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_BreakLock_OutputArguments = 1420;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_Address Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_H1_Address = 1421;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_OrdinalNumber Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_H1_OrdinalNumber = 1422;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_SIFConnection Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_H1_SIFConnection = 1423;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_Lock_Locked Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_HSE_Lock_Locked = 1434;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_Lock_LockingClient Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_HSE_Lock_LockingClient = 1435;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_Lock_LockingUser Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_HSE_Lock_LockingUser = 1436;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_HSE_Lock_RemainingLockTime = 1437;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_HSE_Lock_InitLock_InputArguments = 1439;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_HSE_Lock_InitLock_OutputArguments = 1440;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_HSE_Lock_RenewLock_OutputArguments = 1442;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_HSE_Lock_ExitLock_OutputArguments = 1444;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_HSE_Lock_BreakLock_OutputArguments = 1446;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_Locked = 1452;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_LockingClient = 1453;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_LockingUser = 1454;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_RemainingLockTime = 1455;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_InitLock_InputArguments = 1457;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_InitLock_OutputArguments = 1458;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_RenewLock_OutputArguments = 1460;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_ExitLock_OutputArguments = 1462;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_BreakLock_OutputArguments = 1464;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_Address Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_HSE_Address = 1465;

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_OrdinalNumber Variable.
        /// </summary>
        public const uint ConnectionPoint_Foundation_HSE_OrdinalNumber = 1466;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_Lock_Locked Variable.
        /// </summary>
        public const uint ConnectionPoint_Profibus_DP_Lock_Locked = 1477;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_Lock_LockingClient Variable.
        /// </summary>
        public const uint ConnectionPoint_Profibus_DP_Lock_LockingClient = 1478;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_Lock_LockingUser Variable.
        /// </summary>
        public const uint ConnectionPoint_Profibus_DP_Lock_LockingUser = 1479;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ConnectionPoint_Profibus_DP_Lock_RemainingLockTime = 1480;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Profibus_DP_Lock_InitLock_InputArguments = 1482;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Profibus_DP_Lock_InitLock_OutputArguments = 1483;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Profibus_DP_Lock_RenewLock_OutputArguments = 1485;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Profibus_DP_Lock_ExitLock_OutputArguments = 1487;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Profibus_DP_Lock_BreakLock_OutputArguments = 1489;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_Locked = 1495;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_LockingClient = 1496;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_LockingUser = 1497;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_RemainingLockTime = 1498;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_InitLock_InputArguments = 1500;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_InitLock_OutputArguments = 1501;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_RenewLock_OutputArguments = 1503;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_ExitLock_OutputArguments = 1505;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_BreakLock_OutputArguments = 1507;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_Address Variable.
        /// </summary>
        public const uint ConnectionPoint_Profibus_DP_Address = 1508;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_Lock_Locked Variable.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO_Lock_Locked = 1519;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_Lock_LockingClient Variable.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO_Lock_LockingClient = 1520;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_Lock_LockingUser Variable.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO_Lock_LockingUser = 1521;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO_Lock_RemainingLockTime = 1522;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO_Lock_InitLock_InputArguments = 1524;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO_Lock_InitLock_OutputArguments = 1525;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO_Lock_RenewLock_OutputArguments = 1527;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO_Lock_ExitLock_OutputArguments = 1529;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO_Lock_BreakLock_OutputArguments = 1531;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_Locked = 1537;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_LockingClient = 1538;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_LockingUser = 1539;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_RemainingLockTime = 1540;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_InitLock_InputArguments = 1542;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_InitLock_OutputArguments = 1543;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_RenewLock_OutputArguments = 1545;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_ExitLock_OutputArguments = 1547;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_BreakLock_OutputArguments = 1549;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_MAC Variable.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO_MAC = 1550;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_IPv4 Variable.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO_IPv4 = 1551;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_DNSNAME Variable.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO_DNSNAME = 1552;

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_VALID Variable.
        /// </summary>
        public const uint ConnectionPoint_Profinet_IO_VALID = 1553;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_Lock_Locked Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_Lock_Locked = 1564;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_Lock_LockingClient Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_Lock_LockingClient = 1565;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_Lock_LockingUser Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_Lock_LockingUser = 1566;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_Lock_RemainingLockTime = 1567;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_Lock_InitLock_InputArguments = 1569;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_Lock_InitLock_OutputArguments = 1570;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_Lock_RenewLock_OutputArguments = 1572;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_Lock_ExitLock_OutputArguments = 1574;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_Lock_BreakLock_OutputArguments = 1576;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_Locked = 1582;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_LockingClient = 1583;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_LockingUser = 1584;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_RemainingLockTime = 1585;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_InitLock_InputArguments = 1587;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_InitLock_OutputArguments = 1588;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_RenewLock_OutputArguments = 1590;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_ExitLock_OutputArguments = 1592;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_BreakLock_OutputArguments = 1594;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_DevAddr Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_DevAddr = 1595;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_DevMfg Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_DevMfg = 1596;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_DevType Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_DevType = 1597;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_DevRev Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_DevRev = 1598;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_DevTag Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_DevTag = 1599;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_DevPollAddr Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP5_DevPollAddr = 1600;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_Lock_Locked Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_Lock_Locked = 1611;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_Lock_LockingClient Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_Lock_LockingClient = 1612;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_Lock_LockingUser Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_Lock_LockingUser = 1613;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_Lock_RemainingLockTime = 1614;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_Lock_InitLock_InputArguments = 1616;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_Lock_InitLock_OutputArguments = 1617;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_Lock_RenewLock_OutputArguments = 1619;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_Lock_ExitLock_OutputArguments = 1621;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_Lock_BreakLock_OutputArguments = 1623;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_Locked = 1629;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_LockingClient = 1630;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_LockingUser = 1631;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_RemainingLockTime = 1632;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_InitLock_InputArguments = 1634;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_InitLock_OutputArguments = 1635;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_RenewLock_OutputArguments = 1637;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_ExitLock_OutputArguments = 1639;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_BreakLock_OutputArguments = 1641;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_DevAddr Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_DevAddr = 1642;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_DevMfg Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_DevMfg = 1643;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_DevType Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_DevType = 1644;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_DevRev Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_DevRev = 1645;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_DevTag Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_DevTag = 1646;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_DevPollAddr Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP6_DevPollAddr = 1647;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_Lock_Locked Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_Lock_Locked = 1658;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_Lock_LockingClient Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_Lock_LockingClient = 1659;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_Lock_LockingUser Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_Lock_LockingUser = 1660;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_Lock_RemainingLockTime = 1661;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_Lock_InitLock_InputArguments = 1663;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_Lock_InitLock_OutputArguments = 1664;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_Lock_RenewLock_OutputArguments = 1666;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_Lock_ExitLock_OutputArguments = 1668;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_Lock_BreakLock_OutputArguments = 1670;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_Locked = 1676;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_LockingClient = 1677;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_LockingUser = 1678;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_RemainingLockTime = 1679;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_InitLock_InputArguments = 1681;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_InitLock_OutputArguments = 1682;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_RenewLock_OutputArguments = 1684;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_ExitLock_OutputArguments = 1686;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_BreakLock_OutputArguments = 1688;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_DevAddr Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_DevAddr = 1689;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_DevMfg Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_DevMfg = 1690;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_DevType Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_DevType = 1691;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_DevRev Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_DevRev = 1692;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_DevTag Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_DevTag = 1693;

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_DevPollAddr Variable.
        /// </summary>
        public const uint ConnectionPoint_HART_TP7_DevPollAddr = 1694;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_Lock_Locked Variable.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_Lock_Locked = 1705;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_Lock_LockingClient Variable.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_Lock_LockingClient = 1706;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_Lock_LockingUser Variable.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_Lock_LockingUser = 1707;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_Lock_RemainingLockTime = 1708;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_Lock_InitLock_InputArguments = 1710;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_Lock_InitLock_OutputArguments = 1711;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_Lock_RenewLock_OutputArguments = 1713;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_Lock_ExitLock_OutputArguments = 1715;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_Lock_BreakLock_OutputArguments = 1717;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_Locked = 1723;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_LockingClient = 1724;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_LockingUser = 1725;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_RemainingLockTime = 1726;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_InitLock_InputArguments = 1728;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_InitLock_OutputArguments = 1729;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_RenewLock_OutputArguments = 1731;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_ExitLock_OutputArguments = 1733;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_BreakLock_OutputArguments = 1735;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_IPAddress Variable.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_IPAddress = 1736;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_DevMfg Variable.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_DevMfg = 1737;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_DevType Variable.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_DevType = 1738;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_DevRev Variable.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_DevRev = 1739;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_DevTag Variable.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_DevTag = 1740;

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_DevPollAddr Variable.
        /// </summary>
        public const uint ConnectionPoint_ISA100_Wireless_DevPollAddr = 1741;

        /// <summary>
        /// The identifier for the GenericConnectionPoint_Lock_Locked Variable.
        /// </summary>
        public const uint GenericConnectionPoint_Lock_Locked = 1752;

        /// <summary>
        /// The identifier for the GenericConnectionPoint_Lock_LockingClient Variable.
        /// </summary>
        public const uint GenericConnectionPoint_Lock_LockingClient = 1753;

        /// <summary>
        /// The identifier for the GenericConnectionPoint_Lock_LockingUser Variable.
        /// </summary>
        public const uint GenericConnectionPoint_Lock_LockingUser = 1754;

        /// <summary>
        /// The identifier for the GenericConnectionPoint_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint GenericConnectionPoint_Lock_RemainingLockTime = 1755;

        /// <summary>
        /// The identifier for the GenericConnectionPoint_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint GenericConnectionPoint_Lock_InitLock_InputArguments = 1757;

        /// <summary>
        /// The identifier for the GenericConnectionPoint_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint GenericConnectionPoint_Lock_InitLock_OutputArguments = 1758;

        /// <summary>
        /// The identifier for the GenericConnectionPoint_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint GenericConnectionPoint_Lock_RenewLock_OutputArguments = 1760;

        /// <summary>
        /// The identifier for the GenericConnectionPoint_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint GenericConnectionPoint_Lock_ExitLock_OutputArguments = 1762;

        /// <summary>
        /// The identifier for the GenericConnectionPoint_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint GenericConnectionPoint_Lock_BreakLock_OutputArguments = 1764;

        /// <summary>
        /// The identifier for the GenericConnectionPoint_NetworkIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint GenericConnectionPoint_NetworkIdentifier_Lock_Locked = 1770;

        /// <summary>
        /// The identifier for the GenericConnectionPoint_NetworkIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint GenericConnectionPoint_NetworkIdentifier_Lock_LockingClient = 1771;

        /// <summary>
        /// The identifier for the GenericConnectionPoint_NetworkIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint GenericConnectionPoint_NetworkIdentifier_Lock_LockingUser = 1772;

        /// <summary>
        /// The identifier for the GenericConnectionPoint_NetworkIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint GenericConnectionPoint_NetworkIdentifier_Lock_RemainingLockTime = 1773;

        /// <summary>
        /// The identifier for the GenericConnectionPoint_NetworkIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint GenericConnectionPoint_NetworkIdentifier_Lock_InitLock_InputArguments = 1775;

        /// <summary>
        /// The identifier for the GenericConnectionPoint_NetworkIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint GenericConnectionPoint_NetworkIdentifier_Lock_InitLock_OutputArguments = 1776;

        /// <summary>
        /// The identifier for the GenericConnectionPoint_NetworkIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint GenericConnectionPoint_NetworkIdentifier_Lock_RenewLock_OutputArguments = 1778;

        /// <summary>
        /// The identifier for the GenericConnectionPoint_NetworkIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint GenericConnectionPoint_NetworkIdentifier_Lock_ExitLock_OutputArguments = 1780;

        /// <summary>
        /// The identifier for the GenericConnectionPoint_NetworkIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint GenericConnectionPoint_NetworkIdentifier_Lock_BreakLock_OutputArguments = 1782;

        /// <summary>
        /// The identifier for the GenericConnectionPoint_Address Variable.
        /// </summary>
        public const uint GenericConnectionPoint_Address = 1783;

        /// <summary>
        /// The identifier for the GenericConnectionPoint_ProtocolIdentifier Variable.
        /// </summary>
        public const uint GenericConnectionPoint_ProtocolIdentifier = 1784;

        /// <summary>
        /// The identifier for the CommunicationServerType_Lock_Locked Variable.
        /// </summary>
        public const uint CommunicationServerType_Lock_Locked = 21;

        /// <summary>
        /// The identifier for the CommunicationServerType_Lock_LockingClient Variable.
        /// </summary>
        public const uint CommunicationServerType_Lock_LockingClient = 22;

        /// <summary>
        /// The identifier for the CommunicationServerType_Lock_LockingUser Variable.
        /// </summary>
        public const uint CommunicationServerType_Lock_LockingUser = 23;

        /// <summary>
        /// The identifier for the CommunicationServerType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint CommunicationServerType_Lock_RemainingLockTime = 24;

        /// <summary>
        /// The identifier for the CommunicationServerType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint CommunicationServerType_Lock_InitLock_InputArguments = 26;

        /// <summary>
        /// The identifier for the CommunicationServerType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint CommunicationServerType_Lock_InitLock_OutputArguments = 27;

        /// <summary>
        /// The identifier for the CommunicationServerType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint CommunicationServerType_Lock_RenewLock_OutputArguments = 29;

        /// <summary>
        /// The identifier for the CommunicationServerType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint CommunicationServerType_Lock_ExitLock_OutputArguments = 31;

        /// <summary>
        /// The identifier for the CommunicationServerType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint CommunicationServerType_Lock_BreakLock_OutputArguments = 33;

        /// <summary>
        /// The identifier for the CommunicationServerType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint CommunicationServerType_CPIdentifier_Lock_Locked = 60;

        /// <summary>
        /// The identifier for the CommunicationServerType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint CommunicationServerType_CPIdentifier_Lock_LockingClient = 61;

        /// <summary>
        /// The identifier for the CommunicationServerType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint CommunicationServerType_CPIdentifier_Lock_LockingUser = 62;

        /// <summary>
        /// The identifier for the CommunicationServerType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint CommunicationServerType_CPIdentifier_Lock_RemainingLockTime = 63;

        /// <summary>
        /// The identifier for the CommunicationServerType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint CommunicationServerType_CPIdentifier_Lock_InitLock_InputArguments = 65;

        /// <summary>
        /// The identifier for the CommunicationServerType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint CommunicationServerType_CPIdentifier_Lock_InitLock_OutputArguments = 66;

        /// <summary>
        /// The identifier for the CommunicationServerType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint CommunicationServerType_CPIdentifier_Lock_RenewLock_OutputArguments = 68;

        /// <summary>
        /// The identifier for the CommunicationServerType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint CommunicationServerType_CPIdentifier_Lock_ExitLock_OutputArguments = 70;

        /// <summary>
        /// The identifier for the CommunicationServerType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint CommunicationServerType_CPIdentifier_Lock_BreakLock_OutputArguments = 72;

        /// <summary>
        /// The identifier for the CommunicationServerType_MethodSet_Initialize_OutputArguments Variable.
        /// </summary>
        public const uint CommunicationServerType_MethodSet_Initialize_OutputArguments = 76;

        /// <summary>
        /// The identifier for the CommunicationServerType_MethodSet_Reset_OutputArguments Variable.
        /// </summary>
        public const uint CommunicationServerType_MethodSet_Reset_OutputArguments = 78;

        /// <summary>
        /// The identifier for the CommunicationServerType_MethodSet_AddComponent_InputArguments Variable.
        /// </summary>
        public const uint CommunicationServerType_MethodSet_AddComponent_InputArguments = 80;

        /// <summary>
        /// The identifier for the CommunicationServerType_MethodSet_AddComponent_OutputArguments Variable.
        /// </summary>
        public const uint CommunicationServerType_MethodSet_AddComponent_OutputArguments = 81;

        /// <summary>
        /// The identifier for the CommunicationServerType_MethodSet_RemoveComponent_InputArguments Variable.
        /// </summary>
        public const uint CommunicationServerType_MethodSet_RemoveComponent_InputArguments = 83;

        /// <summary>
        /// The identifier for the CommunicationServerType_MethodSet_RemoveComponent_OutputArguments Variable.
        /// </summary>
        public const uint CommunicationServerType_MethodSet_RemoveComponent_OutputArguments = 84;

        /// <summary>
        /// The identifier for the SetAddressMethodFFH1Type_InputArguments Variable.
        /// </summary>
        public const uint SetAddressMethodFFH1Type_InputArguments = 310;

        /// <summary>
        /// The identifier for the SetAddressMethodFFH1Type_OutputArguments Variable.
        /// </summary>
        public const uint SetAddressMethodFFH1Type_OutputArguments = 311;

        /// <summary>
        /// The identifier for the SetAddressMethodFFHSEType_InputArguments Variable.
        /// </summary>
        public const uint SetAddressMethodFFHSEType_InputArguments = 313;

        /// <summary>
        /// The identifier for the SetAddressMethodFFHSEType_OutputArguments Variable.
        /// </summary>
        public const uint SetAddressMethodFFHSEType_OutputArguments = 314;

        /// <summary>
        /// The identifier for the SetAddressMethodPROFIBUSType_InputArguments Variable.
        /// </summary>
        public const uint SetAddressMethodPROFIBUSType_InputArguments = 316;

        /// <summary>
        /// The identifier for the SetAddressMethodPROFIBUSType_OutputArguments Variable.
        /// </summary>
        public const uint SetAddressMethodPROFIBUSType_OutputArguments = 317;

        /// <summary>
        /// The identifier for the SetAddressMethodPROFINETType_InputArguments Variable.
        /// </summary>
        public const uint SetAddressMethodPROFINETType_InputArguments = 319;

        /// <summary>
        /// The identifier for the SetAddressMethodPROFINETType_OutputArguments Variable.
        /// </summary>
        public const uint SetAddressMethodPROFINETType_OutputArguments = 320;

        /// <summary>
        /// The identifier for the SetAddressMethodHARTType_InputArguments Variable.
        /// </summary>
        public const uint SetAddressMethodHARTType_InputArguments = 322;

        /// <summary>
        /// The identifier for the SetAddressMethodHARTType_OutputArguments Variable.
        /// </summary>
        public const uint SetAddressMethodHARTType_OutputArguments = 323;

        /// <summary>
        /// The identifier for the SetAddressMethodGENERICType_InputArguments Variable.
        /// </summary>
        public const uint SetAddressMethodGENERICType_InputArguments = 1786;

        /// <summary>
        /// The identifier for the SetAddressMethodGENERICType_OutputArguments Variable.
        /// </summary>
        public const uint SetAddressMethodGENERICType_OutputArguments = 1787;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationDeviceType_Lock_Locked = 103;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationDeviceType_Lock_LockingClient = 104;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationDeviceType_Lock_LockingUser = 105;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationDeviceType_Lock_RemainingLockTime = 106;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationDeviceType_Lock_InitLock_InputArguments = 108;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationDeviceType_Lock_InitLock_OutputArguments = 109;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationDeviceType_Lock_RenewLock_OutputArguments = 111;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationDeviceType_Lock_ExitLock_OutputArguments = 113;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationDeviceType_Lock_BreakLock_OutputArguments = 115;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationDeviceType_CPIdentifier_Lock_Locked = 142;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationDeviceType_CPIdentifier_Lock_LockingClient = 143;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationDeviceType_CPIdentifier_Lock_LockingUser = 144;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationDeviceType_CPIdentifier_Lock_RemainingLockTime = 145;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationDeviceType_CPIdentifier_Lock_InitLock_InputArguments = 147;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationDeviceType_CPIdentifier_Lock_InitLock_OutputArguments = 148;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments = 150;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments = 152;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments = 154;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_ListOfCommunicationProfiles Variable.
        /// </summary>
        public const uint ServerCommunicationDeviceType_ListOfCommunicationProfiles = 15001;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_MethodSet_Scan_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationDeviceType_MethodSet_Scan_OutputArguments = 158;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_MethodSet_ResetScan_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationDeviceType_MethodSet_ResetScan_OutputArguments = 160;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_MethodSet_AddComponent_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationDeviceType_MethodSet_AddComponent_InputArguments = 165;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_MethodSet_AddComponent_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationDeviceType_MethodSet_AddComponent_OutputArguments = 166;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_MethodSet_RemoveComponent_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationDeviceType_MethodSet_RemoveComponent_InputArguments = 168;

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_MethodSet_RemoveComponent_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationDeviceType_MethodSet_RemoveComponent_OutputArguments = 169;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_Lock_Locked = 334;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_Lock_LockingClient = 335;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_Lock_LockingUser = 336;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_Lock_RemainingLockTime = 337;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_Lock_InitLock_InputArguments = 339;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_Lock_InitLock_OutputArguments = 340;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_Lock_RenewLock_OutputArguments = 342;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_Lock_ExitLock_OutputArguments = 344;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_Lock_BreakLock_OutputArguments = 346;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_Locked = 373;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_LockingClient = 374;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_LockingUser = 375;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_RemainingLockTime = 376;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_InitLock_InputArguments = 378;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_InitLock_OutputArguments = 379;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_RenewLock_OutputArguments = 381;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_ExitLock_OutputArguments = 383;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_BreakLock_OutputArguments = 385;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_MethodSet_Scan_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_MethodSet_Scan_OutputArguments = 389;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_MethodSet_ResetScan_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_MethodSet_ResetScan_OutputArguments = 391;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_MethodSet_AddComponent_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_MethodSet_AddComponent_InputArguments = 393;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_MethodSet_AddComponent_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_MethodSet_AddComponent_OutputArguments = 394;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_MethodSet_RemoveComponent_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_MethodSet_RemoveComponent_InputArguments = 396;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_MethodSet_RemoveComponent_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_MethodSet_RemoveComponent_OutputArguments = 397;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_MethodSet_SetAddress_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_MethodSet_SetAddress_InputArguments = 399;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_MethodSet_SetAddress_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_MethodSet_SetAddress_OutputArguments = 400;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_Locked = 409;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_LockingClient = 410;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_LockingUser = 411;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_RemainingLockTime = 412;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_InitLock_InputArguments = 414;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_InitLock_OutputArguments = 415;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_RenewLock_OutputArguments = 417;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_ExitLock_OutputArguments = 419;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_BreakLock_OutputArguments = 421;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_Manufacturer Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_Manufacturer = 424;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_Model Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_Model = 425;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_HardwareRevision Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_HardwareRevision = 429;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_SoftwareRevision Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_SoftwareRevision = 428;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_DeviceRevision Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_DeviceRevision = 427;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_DeviceManual Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_DeviceManual = 426;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_SerialNumber Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_SerialNumber = 422;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_RevisionCounter Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_RevisionCounter = 423;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Disconnect_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Disconnect_InputArguments = 441;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Disconnect_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Disconnect_OutputArguments = 442;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Connect_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Connect_InputArguments = 444;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Connect_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Connect_OutputArguments = 445;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Transfer_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Transfer_InputArguments = 447;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Transfer_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Transfer_OutputArguments = 448;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_GetPublishedData_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_GetPublishedData_InputArguments = 450;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_GetPublishedData_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_GetPublishedData_OutputArguments = 451;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_Lock_Locked = 462;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_Lock_LockingClient = 463;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_Lock_LockingUser = 464;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_Lock_RemainingLockTime = 465;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_Lock_InitLock_InputArguments = 467;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_Lock_InitLock_OutputArguments = 468;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_Lock_RenewLock_OutputArguments = 470;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_Lock_ExitLock_OutputArguments = 472;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_Lock_BreakLock_OutputArguments = 474;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_Locked = 501;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_LockingClient = 502;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_LockingUser = 503;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_RemainingLockTime = 504;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_InitLock_InputArguments = 506;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_InitLock_OutputArguments = 507;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments = 509;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments = 511;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments = 513;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_MethodSet_Scan_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_MethodSet_Scan_OutputArguments = 517;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_MethodSet_ResetScan_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_MethodSet_ResetScan_OutputArguments = 519;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_MethodSet_AddComponent_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_MethodSet_AddComponent_InputArguments = 521;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_MethodSet_AddComponent_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_MethodSet_AddComponent_OutputArguments = 522;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_MethodSet_RemoveComponent_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_MethodSet_RemoveComponent_InputArguments = 524;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_MethodSet_RemoveComponent_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_MethodSet_RemoveComponent_OutputArguments = 525;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_MethodSet_SetAddress_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_MethodSet_SetAddress_InputArguments = 527;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_MethodSet_SetAddress_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_MethodSet_SetAddress_OutputArguments = 528;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_Locked = 537;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_LockingClient = 538;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_LockingUser = 539;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_RemainingLockTime = 540;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_InitLock_InputArguments = 542;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_InitLock_OutputArguments = 543;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_RenewLock_OutputArguments = 545;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_ExitLock_OutputArguments = 547;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_BreakLock_OutputArguments = 549;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_Manufacturer Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_Manufacturer = 552;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_Model Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_Model = 553;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_HardwareRevision Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_HardwareRevision = 557;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_SoftwareRevision Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_SoftwareRevision = 556;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_DeviceRevision Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_DeviceRevision = 555;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_DeviceManual Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_DeviceManual = 554;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_SerialNumber Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_SerialNumber = 550;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_RevisionCounter Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_RevisionCounter = 551;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Disconnect_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Disconnect_InputArguments = 569;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Disconnect_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Disconnect_OutputArguments = 570;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Connect_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Connect_InputArguments = 572;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Connect_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Connect_OutputArguments = 573;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Transfer_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Transfer_InputArguments = 575;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Transfer_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Transfer_OutputArguments = 576;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_GetPublishedData_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_GetPublishedData_InputArguments = 578;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_GetPublishedData_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_GetPublishedData_OutputArguments = 579;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_Lock_Locked = 590;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_Lock_LockingClient = 591;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_Lock_LockingUser = 592;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_Lock_RemainingLockTime = 593;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_Lock_InitLock_InputArguments = 595;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_Lock_InitLock_OutputArguments = 596;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_Lock_RenewLock_OutputArguments = 598;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_Lock_ExitLock_OutputArguments = 600;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_Lock_BreakLock_OutputArguments = 602;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_Locked = 629;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_LockingClient = 630;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_LockingUser = 631;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_RemainingLockTime = 632;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_InitLock_InputArguments = 634;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_InitLock_OutputArguments = 635;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments = 637;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments = 639;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments = 641;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_MethodSet_Scan_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_MethodSet_Scan_OutputArguments = 645;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_MethodSet_ResetScan_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_MethodSet_ResetScan_OutputArguments = 647;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_MethodSet_AddComponent_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_MethodSet_AddComponent_InputArguments = 649;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_MethodSet_AddComponent_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_MethodSet_AddComponent_OutputArguments = 650;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_MethodSet_RemoveComponent_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_MethodSet_RemoveComponent_InputArguments = 652;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_MethodSet_RemoveComponent_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_MethodSet_RemoveComponent_OutputArguments = 653;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_MethodSet_SetAddress_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_MethodSet_SetAddress_InputArguments = 655;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_MethodSet_SetAddress_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_MethodSet_SetAddress_OutputArguments = 656;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_Locked = 665;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_LockingClient = 666;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_LockingUser = 667;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_RemainingLockTime = 668;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_InitLock_InputArguments = 670;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_InitLock_OutputArguments = 671;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_RenewLock_OutputArguments = 673;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_ExitLock_OutputArguments = 675;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_BreakLock_OutputArguments = 677;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Manufacturer Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Manufacturer = 680;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Model Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Model = 681;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_HardwareRevision Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_HardwareRevision = 685;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_SoftwareRevision Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_SoftwareRevision = 684;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_DeviceRevision Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_DeviceRevision = 683;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_DeviceManual Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_DeviceManual = 682;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_SerialNumber Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_SerialNumber = 678;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_RevisionCounter Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_RevisionCounter = 679;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Disconnect_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Disconnect_InputArguments = 697;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Disconnect_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Disconnect_OutputArguments = 698;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Connect_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Connect_InputArguments = 700;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Connect_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Connect_OutputArguments = 701;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Transfer_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Transfer_InputArguments = 703;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Transfer_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Transfer_OutputArguments = 704;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_Lock_Locked = 715;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_Lock_LockingClient = 716;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_Lock_LockingUser = 717;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_Lock_RemainingLockTime = 718;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_Lock_InitLock_InputArguments = 720;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_Lock_InitLock_OutputArguments = 721;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_Lock_RenewLock_OutputArguments = 723;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_Lock_ExitLock_OutputArguments = 725;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_Lock_BreakLock_OutputArguments = 727;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_Locked = 754;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_LockingClient = 755;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_LockingUser = 756;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_RemainingLockTime = 757;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_InitLock_InputArguments = 759;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_InitLock_OutputArguments = 760;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments = 762;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments = 764;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments = 766;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_MethodSet_Scan_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_MethodSet_Scan_OutputArguments = 770;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_MethodSet_ResetScan_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_MethodSet_ResetScan_OutputArguments = 772;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_MethodSet_AddComponent_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_MethodSet_AddComponent_InputArguments = 774;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_MethodSet_AddComponent_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_MethodSet_AddComponent_OutputArguments = 775;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_MethodSet_RemoveComponent_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_MethodSet_RemoveComponent_InputArguments = 777;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_MethodSet_RemoveComponent_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_MethodSet_RemoveComponent_OutputArguments = 778;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_MethodSet_SetAddress_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_MethodSet_SetAddress_InputArguments = 780;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_MethodSet_SetAddress_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_MethodSet_SetAddress_OutputArguments = 781;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_Locked = 790;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_LockingClient = 791;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_LockingUser = 792;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_RemainingLockTime = 793;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_InitLock_InputArguments = 795;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_InitLock_OutputArguments = 796;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_RenewLock_OutputArguments = 798;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_ExitLock_OutputArguments = 800;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_BreakLock_OutputArguments = 802;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_Manufacturer Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_Manufacturer = 805;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_Model Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_Model = 806;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_HardwareRevision Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_HardwareRevision = 810;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_SoftwareRevision Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_SoftwareRevision = 809;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_DeviceRevision Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_DeviceRevision = 808;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_DeviceManual Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_DeviceManual = 807;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_SerialNumber Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_SerialNumber = 803;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_RevisionCounter Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_RevisionCounter = 804;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Disconnect_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Disconnect_InputArguments = 822;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Disconnect_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Disconnect_OutputArguments = 823;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Connect_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Connect_InputArguments = 825;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Connect_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Connect_OutputArguments = 826;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Transfer_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Transfer_InputArguments = 828;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Transfer_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Transfer_OutputArguments = 829;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_Lock_Locked = 840;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_Lock_LockingClient = 841;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_Lock_LockingUser = 842;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_Lock_RemainingLockTime = 843;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_Lock_InitLock_InputArguments = 845;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_Lock_InitLock_OutputArguments = 846;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_Lock_RenewLock_OutputArguments = 848;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_Lock_ExitLock_OutputArguments = 850;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_Lock_BreakLock_OutputArguments = 852;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_CPIdentifier_Lock_Locked = 879;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_CPIdentifier_Lock_LockingClient = 880;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_CPIdentifier_Lock_LockingUser = 881;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_CPIdentifier_Lock_RemainingLockTime = 882;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_CPIdentifier_Lock_InitLock_InputArguments = 884;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_CPIdentifier_Lock_InitLock_OutputArguments = 885;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_CPIdentifier_Lock_RenewLock_OutputArguments = 887;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_CPIdentifier_Lock_ExitLock_OutputArguments = 889;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_CPIdentifier_Lock_BreakLock_OutputArguments = 891;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_MethodSet_Scan_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_MethodSet_Scan_OutputArguments = 895;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_MethodSet_ResetScan_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_MethodSet_ResetScan_OutputArguments = 897;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_MethodSet_AddComponent_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_MethodSet_AddComponent_InputArguments = 899;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_MethodSet_AddComponent_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_MethodSet_AddComponent_OutputArguments = 900;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_MethodSet_RemoveComponent_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_MethodSet_RemoveComponent_InputArguments = 902;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_MethodSet_RemoveComponent_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_MethodSet_RemoveComponent_OutputArguments = 903;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_MethodSet_SetAddress_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_MethodSet_SetAddress_InputArguments = 905;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_MethodSet_SetAddress_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_MethodSet_SetAddress_OutputArguments = 906;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_Lock_Locked = 915;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_Lock_LockingClient = 916;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_Lock_LockingUser = 917;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_Lock_RemainingLockTime = 918;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_Lock_InitLock_InputArguments = 920;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_Lock_InitLock_OutputArguments = 921;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_Lock_RenewLock_OutputArguments = 923;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_Lock_ExitLock_OutputArguments = 925;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_Lock_BreakLock_OutputArguments = 927;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_Manufacturer Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_Manufacturer = 930;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_Model Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_Model = 931;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_HardwareRevision Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_HardwareRevision = 935;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_SoftwareRevision Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_SoftwareRevision = 934;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_DeviceRevision Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_DeviceRevision = 933;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_DeviceManual Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_DeviceManual = 932;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_SerialNumber Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_SerialNumber = 928;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_RevisionCounter Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_RevisionCounter = 929;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_MethodSet_Disconnect_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_MethodSet_Disconnect_InputArguments = 947;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_MethodSet_Disconnect_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_MethodSet_Disconnect_OutputArguments = 948;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_MethodSet_Connect_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_MethodSet_Connect_InputArguments = 950;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_MethodSet_Connect_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_MethodSet_Connect_OutputArguments = 951;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_MethodSet_Transfer_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_MethodSet_Transfer_InputArguments = 953;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_MethodSet_Transfer_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_MethodSet_Transfer_OutputArguments = 954;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_MethodSet_GetPublishedData_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_MethodSet_GetPublishedData_InputArguments = 956;

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_MethodSet_GetPublishedData_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARType_ServiceProvider_MethodSet_GetPublishedData_OutputArguments = 957;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_Lock_Locked = 1798;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_Lock_LockingClient = 1799;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_Lock_LockingUser = 1800;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_Lock_RemainingLockTime = 1801;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_Lock_InitLock_InputArguments = 1803;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_Lock_InitLock_OutputArguments = 1804;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_Lock_RenewLock_OutputArguments = 1806;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_Lock_ExitLock_OutputArguments = 1808;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_Lock_BreakLock_OutputArguments = 1810;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_Locked = 1837;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_LockingClient = 1838;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_LockingUser = 1839;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_RemainingLockTime = 1840;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_InitLock_InputArguments = 1842;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_InitLock_OutputArguments = 1843;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments = 1845;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments = 1847;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments = 1849;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_MethodSet_Scan_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_MethodSet_Scan_OutputArguments = 1853;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_MethodSet_ResetScan_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_MethodSet_ResetScan_OutputArguments = 1855;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_MethodSet_AddComponent_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_MethodSet_AddComponent_InputArguments = 1857;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_MethodSet_AddComponent_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_MethodSet_AddComponent_OutputArguments = 1858;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_MethodSet_RemoveComponent_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_MethodSet_RemoveComponent_InputArguments = 1860;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_MethodSet_RemoveComponent_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_MethodSet_RemoveComponent_OutputArguments = 1861;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_Locked = 1870;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_LockingClient = 1871;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_LockingUser = 1872;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_RemainingLockTime = 1873;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_InitLock_InputArguments = 1875;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_InitLock_OutputArguments = 1876;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_RenewLock_OutputArguments = 1878;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_ExitLock_OutputArguments = 1880;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_BreakLock_OutputArguments = 1882;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Manufacturer Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Manufacturer = 1885;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Model Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Model = 1886;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_HardwareRevision Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_HardwareRevision = 1890;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_SoftwareRevision Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_SoftwareRevision = 1889;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_DeviceRevision Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_DeviceRevision = 1888;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_DeviceManual Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_DeviceManual = 1887;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_SerialNumber Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_SerialNumber = 1883;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_RevisionCounter Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_RevisionCounter = 1884;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Disconnect_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Disconnect_InputArguments = 1902;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Disconnect_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Disconnect_OutputArguments = 1903;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Connect_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Connect_InputArguments = 1905;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Connect_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Connect_OutputArguments = 1906;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Transfer_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Transfer_InputArguments = 1908;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Transfer_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Transfer_OutputArguments = 1909;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_GetPublishedData_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_GetPublishedData_InputArguments = 1911;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_GetPublishedData_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_GetPublishedData_OutputArguments = 1912;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_Lock_Locked = 1923;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_Lock_LockingClient = 1924;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_Lock_LockingUser = 1925;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_Lock_RemainingLockTime = 1926;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_Lock_InitLock_InputArguments = 1928;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_Lock_InitLock_OutputArguments = 1929;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_Lock_RenewLock_OutputArguments = 1931;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_Lock_ExitLock_OutputArguments = 1933;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_Lock_BreakLock_OutputArguments = 1935;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_Locked = 1962;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_LockingClient = 1963;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_LockingUser = 1964;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_RemainingLockTime = 1965;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_InitLock_InputArguments = 1967;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_InitLock_OutputArguments = 1968;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments = 1970;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments = 1972;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments = 1974;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_MethodSet_Scan_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_MethodSet_Scan_OutputArguments = 1978;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_MethodSet_ResetScan_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_MethodSet_ResetScan_OutputArguments = 1980;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_MethodSet_AddComponent_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_MethodSet_AddComponent_InputArguments = 1982;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_MethodSet_AddComponent_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_MethodSet_AddComponent_OutputArguments = 1983;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_MethodSet_RemoveComponent_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_MethodSet_RemoveComponent_InputArguments = 1985;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_MethodSet_RemoveComponent_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_MethodSet_RemoveComponent_OutputArguments = 1986;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ProtocolIdentifier Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ProtocolIdentifier = 1987;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_MethodSet_SetAddress_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_MethodSet_SetAddress_InputArguments = 1989;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_MethodSet_SetAddress_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_MethodSet_SetAddress_OutputArguments = 1990;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_Locked = 1999;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_LockingClient = 2000;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_LockingUser = 2001;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_RemainingLockTime = 2002;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_InitLock_InputArguments = 2004;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_InitLock_OutputArguments = 2005;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_RenewLock_OutputArguments = 2007;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_ExitLock_OutputArguments = 2009;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_BreakLock_OutputArguments = 2011;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_Manufacturer Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_Manufacturer = 2014;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_Model Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_Model = 2015;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_HardwareRevision Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_HardwareRevision = 2019;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_SoftwareRevision Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_SoftwareRevision = 2018;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_DeviceRevision Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_DeviceRevision = 2017;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_DeviceManual Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_DeviceManual = 2016;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_SerialNumber Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_SerialNumber = 2012;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_RevisionCounter Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_RevisionCounter = 2013;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Disconnect_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Disconnect_InputArguments = 2031;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Disconnect_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Disconnect_OutputArguments = 2032;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Connect_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Connect_InputArguments = 2034;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Connect_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Connect_OutputArguments = 2035;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Transfer_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Transfer_InputArguments = 2037;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Transfer_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Transfer_OutputArguments = 2038;

        /// <summary>
        /// The identifier for the ConnectMethodFFH1Type_InputArguments Variable.
        /// </summary>
        public const uint ConnectMethodFFH1Type_InputArguments = 959;

        /// <summary>
        /// The identifier for the ConnectMethodFFH1Type_OutputArguments Variable.
        /// </summary>
        public const uint ConnectMethodFFH1Type_OutputArguments = 960;

        /// <summary>
        /// The identifier for the ConnectMethodFFHSEType_InputArguments Variable.
        /// </summary>
        public const uint ConnectMethodFFHSEType_InputArguments = 962;

        /// <summary>
        /// The identifier for the ConnectMethodFFHSEType_OutputArguments Variable.
        /// </summary>
        public const uint ConnectMethodFFHSEType_OutputArguments = 963;

        /// <summary>
        /// The identifier for the ConnectMethodPROFIBUSType_InputArguments Variable.
        /// </summary>
        public const uint ConnectMethodPROFIBUSType_InputArguments = 965;

        /// <summary>
        /// The identifier for the ConnectMethodPROFIBUSType_OutputArguments Variable.
        /// </summary>
        public const uint ConnectMethodPROFIBUSType_OutputArguments = 966;

        /// <summary>
        /// The identifier for the ConnectMethodPROFINETType_InputArguments Variable.
        /// </summary>
        public const uint ConnectMethodPROFINETType_InputArguments = 968;

        /// <summary>
        /// The identifier for the ConnectMethodPROFINETType_OutputArguments Variable.
        /// </summary>
        public const uint ConnectMethodPROFINETType_OutputArguments = 969;

        /// <summary>
        /// The identifier for the ConnectMethodHARTType_InputArguments Variable.
        /// </summary>
        public const uint ConnectMethodHARTType_InputArguments = 971;

        /// <summary>
        /// The identifier for the ConnectMethodHARTType_OutputArguments Variable.
        /// </summary>
        public const uint ConnectMethodHARTType_OutputArguments = 972;

        /// <summary>
        /// The identifier for the ConnectMethodISA100_WirelessType_InputArguments Variable.
        /// </summary>
        public const uint ConnectMethodISA100_WirelessType_InputArguments = 2040;

        /// <summary>
        /// The identifier for the ConnectMethodISA100_WirelessType_OutputArguments Variable.
        /// </summary>
        public const uint ConnectMethodISA100_WirelessType_OutputArguments = 2041;

        /// <summary>
        /// The identifier for the ConnectMethodGENERICType_InputArguments Variable.
        /// </summary>
        public const uint ConnectMethodGENERICType_InputArguments = 2043;

        /// <summary>
        /// The identifier for the ConnectMethodGENERICType_OutputArguments Variable.
        /// </summary>
        public const uint ConnectMethodGENERICType_OutputArguments = 2044;

        /// <summary>
        /// The identifier for the TransferMethodFFH1Type_InputArguments Variable.
        /// </summary>
        public const uint TransferMethodFFH1Type_InputArguments = 974;

        /// <summary>
        /// The identifier for the TransferMethodFFH1Type_OutputArguments Variable.
        /// </summary>
        public const uint TransferMethodFFH1Type_OutputArguments = 975;

        /// <summary>
        /// The identifier for the TransferMethodFFHSEType_InputArguments Variable.
        /// </summary>
        public const uint TransferMethodFFHSEType_InputArguments = 977;

        /// <summary>
        /// The identifier for the TransferMethodFFHSEType_OutputArguments Variable.
        /// </summary>
        public const uint TransferMethodFFHSEType_OutputArguments = 978;

        /// <summary>
        /// The identifier for the TransferMethodPROFIBUSType_InputArguments Variable.
        /// </summary>
        public const uint TransferMethodPROFIBUSType_InputArguments = 980;

        /// <summary>
        /// The identifier for the TransferMethodPROFIBUSType_OutputArguments Variable.
        /// </summary>
        public const uint TransferMethodPROFIBUSType_OutputArguments = 981;

        /// <summary>
        /// The identifier for the TransferMethodPROFINETType_InputArguments Variable.
        /// </summary>
        public const uint TransferMethodPROFINETType_InputArguments = 983;

        /// <summary>
        /// The identifier for the TransferMethodPROFINETType_OutputArguments Variable.
        /// </summary>
        public const uint TransferMethodPROFINETType_OutputArguments = 984;

        /// <summary>
        /// The identifier for the TransferMethodHARTType_InputArguments Variable.
        /// </summary>
        public const uint TransferMethodHARTType_InputArguments = 986;

        /// <summary>
        /// The identifier for the TransferMethodHARTType_OutputArguments Variable.
        /// </summary>
        public const uint TransferMethodHARTType_OutputArguments = 987;

        /// <summary>
        /// The identifier for the TransferMethodISA100_WirelessType_InputArguments Variable.
        /// </summary>
        public const uint TransferMethodISA100_WirelessType_InputArguments = 2046;

        /// <summary>
        /// The identifier for the TransferMethodISA100_WirelessType_OutputArguments Variable.
        /// </summary>
        public const uint TransferMethodISA100_WirelessType_OutputArguments = 2047;

        /// <summary>
        /// The identifier for the EddDataTypeEnum_EnumValues Variable.
        /// </summary>
        public const uint EddDataTypeEnum_EnumValues = 2049;

        /// <summary>
        /// The identifier for the TransferMethodGENERICType_InputArguments Variable.
        /// </summary>
        public const uint TransferMethodGENERICType_InputArguments = 2052;

        /// <summary>
        /// The identifier for the TransferMethodGENERICType_OutputArguments Variable.
        /// </summary>
        public const uint TransferMethodGENERICType_OutputArguments = 2053;

        /// <summary>
        /// The identifier for the GetPublishedDataMethodFFH1Type_InputArguments Variable.
        /// </summary>
        public const uint GetPublishedDataMethodFFH1Type_InputArguments = 989;

        /// <summary>
        /// The identifier for the GetPublishedDataMethodFFH1Type_OutputArguments Variable.
        /// </summary>
        public const uint GetPublishedDataMethodFFH1Type_OutputArguments = 990;

        /// <summary>
        /// The identifier for the GetPublishedDataMethodFFHSEType_InputArguments Variable.
        /// </summary>
        public const uint GetPublishedDataMethodFFHSEType_InputArguments = 992;

        /// <summary>
        /// The identifier for the GetPublishedDataMethodFFHSEType_OutputArguments Variable.
        /// </summary>
        public const uint GetPublishedDataMethodFFHSEType_OutputArguments = 993;

        /// <summary>
        /// The identifier for the GetPublishedDataMethodHARTType_InputArguments Variable.
        /// </summary>
        public const uint GetPublishedDataMethodHARTType_InputArguments = 995;

        /// <summary>
        /// The identifier for the GetPublishedDataMethodHARTType_OutputArguments Variable.
        /// </summary>
        public const uint GetPublishedDataMethodHARTType_OutputArguments = 996;

        /// <summary>
        /// The identifier for the GetPublishedDataMethodISA100_WirelessType_InputArguments Variable.
        /// </summary>
        public const uint GetPublishedDataMethodISA100_WirelessType_InputArguments = 2055;

        /// <summary>
        /// The identifier for the GetPublishedDataMethodISA100_WirelessType_OutputArguments Variable.
        /// </summary>
        public const uint GetPublishedDataMethodISA100_WirelessType_OutputArguments = 2056;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationServiceType_Lock_Locked = 243;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationServiceType_Lock_LockingClient = 244;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationServiceType_Lock_LockingUser = 245;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationServiceType_Lock_RemainingLockTime = 246;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationServiceType_Lock_InitLock_InputArguments = 248;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationServiceType_Lock_InitLock_OutputArguments = 249;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationServiceType_Lock_RenewLock_OutputArguments = 251;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationServiceType_Lock_ExitLock_OutputArguments = 253;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationServiceType_Lock_BreakLock_OutputArguments = 255;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationServiceType_CPIdentifier_Lock_Locked = 282;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationServiceType_CPIdentifier_Lock_LockingClient = 283;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationServiceType_CPIdentifier_Lock_LockingUser = 284;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationServiceType_CPIdentifier_Lock_RemainingLockTime = 285;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationServiceType_CPIdentifier_Lock_InitLock_InputArguments = 287;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationServiceType_CPIdentifier_Lock_InitLock_OutputArguments = 288;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationServiceType_CPIdentifier_Lock_RenewLock_OutputArguments = 290;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationServiceType_CPIdentifier_Lock_ExitLock_OutputArguments = 292;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationServiceType_CPIdentifier_Lock_BreakLock_OutputArguments = 294;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_MethodSet_Disconnect_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationServiceType_MethodSet_Disconnect_InputArguments = 301;

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_MethodSet_Disconnect_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationServiceType_MethodSet_Disconnect_OutputArguments = 302;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_Lock_Locked = 1007;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_Lock_LockingClient = 1008;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_Lock_LockingUser = 1009;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_Lock_RemainingLockTime = 1010;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_Lock_InitLock_InputArguments = 1012;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_Lock_InitLock_OutputArguments = 1013;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_Lock_RenewLock_OutputArguments = 1015;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_Lock_ExitLock_OutputArguments = 1017;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_Lock_BreakLock_OutputArguments = 1019;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_Locked = 1046;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_LockingClient = 1047;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_LockingUser = 1048;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_RemainingLockTime = 1049;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_InitLock_InputArguments = 1051;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_InitLock_OutputArguments = 1052;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_RenewLock_OutputArguments = 1054;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_ExitLock_OutputArguments = 1056;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_BreakLock_OutputArguments = 1058;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_MethodSet_Disconnect_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_MethodSet_Disconnect_InputArguments = 1062;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_MethodSet_Disconnect_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_MethodSet_Disconnect_OutputArguments = 1063;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_MethodSet_Connect_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_MethodSet_Connect_InputArguments = 1065;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_MethodSet_Connect_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_MethodSet_Connect_OutputArguments = 1066;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_MethodSet_Transfer_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_MethodSet_Transfer_InputArguments = 1068;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_MethodSet_Transfer_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_MethodSet_Transfer_OutputArguments = 1069;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_MethodSet_GetPublishedData_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_MethodSet_GetPublishedData_InputArguments = 1071;

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_MethodSet_GetPublishedData_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFH1ServiceType_MethodSet_GetPublishedData_OutputArguments = 1072;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_Lock_Locked = 1083;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_Lock_LockingClient = 1084;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_Lock_LockingUser = 1085;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_Lock_RemainingLockTime = 1086;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_Lock_InitLock_InputArguments = 1088;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_Lock_InitLock_OutputArguments = 1089;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_Lock_RenewLock_OutputArguments = 1091;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_Lock_ExitLock_OutputArguments = 1093;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_Lock_BreakLock_OutputArguments = 1095;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_Locked = 1122;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_LockingClient = 1123;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_LockingUser = 1124;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_RemainingLockTime = 1125;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_InitLock_InputArguments = 1127;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_InitLock_OutputArguments = 1128;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_RenewLock_OutputArguments = 1130;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_ExitLock_OutputArguments = 1132;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_BreakLock_OutputArguments = 1134;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_MethodSet_Disconnect_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_MethodSet_Disconnect_InputArguments = 1138;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_MethodSet_Disconnect_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_MethodSet_Disconnect_OutputArguments = 1139;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_MethodSet_Connect_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_MethodSet_Connect_InputArguments = 1141;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_MethodSet_Connect_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_MethodSet_Connect_OutputArguments = 1142;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_MethodSet_Transfer_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_MethodSet_Transfer_InputArguments = 1144;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_MethodSet_Transfer_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_MethodSet_Transfer_OutputArguments = 1145;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_MethodSet_GetPublishedData_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_MethodSet_GetPublishedData_InputArguments = 1147;

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_MethodSet_GetPublishedData_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationFFHSEServiceType_MethodSet_GetPublishedData_OutputArguments = 1148;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_Lock_Locked = 1159;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_Lock_LockingClient = 1160;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_Lock_LockingUser = 1161;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_Lock_RemainingLockTime = 1162;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_Lock_InitLock_InputArguments = 1164;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_Lock_InitLock_OutputArguments = 1165;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_Lock_RenewLock_OutputArguments = 1167;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_Lock_ExitLock_OutputArguments = 1169;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_Lock_BreakLock_OutputArguments = 1171;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_Locked = 1198;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_LockingClient = 1199;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_LockingUser = 1200;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_RemainingLockTime = 1201;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_InitLock_InputArguments = 1203;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_InitLock_OutputArguments = 1204;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_RenewLock_OutputArguments = 1206;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_ExitLock_OutputArguments = 1208;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_BreakLock_OutputArguments = 1210;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_MethodSet_Disconnect_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_MethodSet_Disconnect_InputArguments = 1214;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_MethodSet_Disconnect_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_MethodSet_Disconnect_OutputArguments = 1215;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_MethodSet_Connect_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_MethodSet_Connect_InputArguments = 1217;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_MethodSet_Connect_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_MethodSet_Connect_OutputArguments = 1218;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_MethodSet_Transfer_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_MethodSet_Transfer_InputArguments = 1220;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_MethodSet_Transfer_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFIBUSServiceType_MethodSet_Transfer_OutputArguments = 1221;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_Lock_Locked = 1232;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_Lock_LockingClient = 1233;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_Lock_LockingUser = 1234;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_Lock_RemainingLockTime = 1235;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_Lock_InitLock_InputArguments = 1237;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_Lock_InitLock_OutputArguments = 1238;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_Lock_RenewLock_OutputArguments = 1240;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_Lock_ExitLock_OutputArguments = 1242;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_Lock_BreakLock_OutputArguments = 1244;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_Locked = 1271;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_LockingClient = 1272;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_LockingUser = 1273;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_RemainingLockTime = 1274;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_InitLock_InputArguments = 1276;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_InitLock_OutputArguments = 1277;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_RenewLock_OutputArguments = 1279;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_ExitLock_OutputArguments = 1281;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_BreakLock_OutputArguments = 1283;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_MethodSet_Disconnect_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_MethodSet_Disconnect_InputArguments = 1287;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_MethodSet_Disconnect_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_MethodSet_Disconnect_OutputArguments = 1288;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_MethodSet_Connect_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_MethodSet_Connect_InputArguments = 1290;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_MethodSet_Connect_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_MethodSet_Connect_OutputArguments = 1291;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_MethodSet_Transfer_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_MethodSet_Transfer_InputArguments = 1293;

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_MethodSet_Transfer_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationPROFINETServiceType_MethodSet_Transfer_OutputArguments = 1294;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_Lock_Locked = 1305;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_Lock_LockingClient = 1306;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_Lock_LockingUser = 1307;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_Lock_RemainingLockTime = 1308;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_Lock_InitLock_InputArguments = 1310;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_Lock_InitLock_OutputArguments = 1311;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_Lock_RenewLock_OutputArguments = 1313;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_Lock_ExitLock_OutputArguments = 1315;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_Lock_BreakLock_OutputArguments = 1317;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_CPIdentifier_Lock_Locked = 1344;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_CPIdentifier_Lock_LockingClient = 1345;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_CPIdentifier_Lock_LockingUser = 1346;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_CPIdentifier_Lock_RemainingLockTime = 1347;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_CPIdentifier_Lock_InitLock_InputArguments = 1349;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_CPIdentifier_Lock_InitLock_OutputArguments = 1350;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_CPIdentifier_Lock_RenewLock_OutputArguments = 1352;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_CPIdentifier_Lock_ExitLock_OutputArguments = 1354;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_CPIdentifier_Lock_BreakLock_OutputArguments = 1356;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_MethodSet_Disconnect_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_MethodSet_Disconnect_InputArguments = 1360;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_MethodSet_Disconnect_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_MethodSet_Disconnect_OutputArguments = 1361;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_MethodSet_Connect_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_MethodSet_Connect_InputArguments = 1363;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_MethodSet_Connect_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_MethodSet_Connect_OutputArguments = 1364;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_MethodSet_Transfer_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_MethodSet_Transfer_InputArguments = 1366;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_MethodSet_Transfer_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_MethodSet_Transfer_OutputArguments = 1367;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_MethodSet_GetPublishedData_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_MethodSet_GetPublishedData_InputArguments = 1369;

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_MethodSet_GetPublishedData_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationHARTServiceType_MethodSet_GetPublishedData_OutputArguments = 1370;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_Lock_Locked = 2067;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_Lock_LockingClient = 2068;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_Lock_LockingUser = 2069;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_Lock_RemainingLockTime = 2070;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_Lock_InitLock_InputArguments = 2072;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_Lock_InitLock_OutputArguments = 2073;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_Lock_RenewLock_OutputArguments = 2075;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_Lock_ExitLock_OutputArguments = 2077;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_Lock_BreakLock_OutputArguments = 2079;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_Locked = 2106;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_LockingClient = 2107;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_LockingUser = 2108;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_RemainingLockTime = 2109;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_InitLock_InputArguments = 2111;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_InitLock_OutputArguments = 2112;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_RenewLock_OutputArguments = 2114;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_ExitLock_OutputArguments = 2116;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_BreakLock_OutputArguments = 2118;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_MethodSet_Disconnect_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_MethodSet_Disconnect_InputArguments = 2122;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_MethodSet_Disconnect_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_MethodSet_Disconnect_OutputArguments = 2123;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_MethodSet_Connect_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_MethodSet_Connect_InputArguments = 2125;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_MethodSet_Connect_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_MethodSet_Connect_OutputArguments = 2126;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_MethodSet_Transfer_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_MethodSet_Transfer_InputArguments = 2128;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_MethodSet_Transfer_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_MethodSet_Transfer_OutputArguments = 2129;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_MethodSet_GetPublishedData_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_MethodSet_GetPublishedData_InputArguments = 2131;

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_MethodSet_GetPublishedData_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationISA100_WirelessServiceType_MethodSet_GetPublishedData_OutputArguments = 2132;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_Lock_Locked = 2143;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_Lock_LockingClient = 2144;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_Lock_LockingUser = 2145;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_Lock_RemainingLockTime = 2146;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_Lock_InitLock_InputArguments = 2148;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_Lock_InitLock_OutputArguments = 2149;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_Lock_RenewLock_OutputArguments = 2151;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_Lock_ExitLock_OutputArguments = 2153;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_Lock_BreakLock_OutputArguments = 2155;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_CPIdentifier_Lock_Locked = 2182;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_CPIdentifier_Lock_LockingClient = 2183;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_CPIdentifier_Lock_LockingUser = 2184;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_CPIdentifier_Lock_RemainingLockTime = 2185;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_CPIdentifier_Lock_InitLock_InputArguments = 2187;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_CPIdentifier_Lock_InitLock_OutputArguments = 2188;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_CPIdentifier_Lock_RenewLock_OutputArguments = 2190;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_CPIdentifier_Lock_ExitLock_OutputArguments = 2192;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_CPIdentifier_Lock_BreakLock_OutputArguments = 2194;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_MethodSet_Disconnect_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_MethodSet_Disconnect_InputArguments = 2198;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_MethodSet_Disconnect_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_MethodSet_Disconnect_OutputArguments = 2199;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_MethodSet_Connect_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_MethodSet_Connect_InputArguments = 2201;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_MethodSet_Connect_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_MethodSet_Connect_OutputArguments = 2202;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_MethodSet_Transfer_InputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_MethodSet_Transfer_InputArguments = 2204;

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_MethodSet_Transfer_OutputArguments Variable.
        /// </summary>
        public const uint ServerCommunicationGENERICServiceType_MethodSet_Transfer_OutputArguments = 2205;

        /// <summary>
        /// The identifier for the OpcUaFdi7_BinarySchema Variable.
        /// </summary>
        public const uint OpcUaFdi7_BinarySchema = 2214;

        /// <summary>
        /// The identifier for the OpcUaFdi7_BinarySchema_NamespaceUri Variable.
        /// </summary>
        public const uint OpcUaFdi7_BinarySchema_NamespaceUri = 2216;

        /// <summary>
        /// The identifier for the OpcUaFdi7_BinarySchema_Deprecated Variable.
        /// </summary>
        public const uint OpcUaFdi7_BinarySchema_Deprecated = 8002;

        /// <summary>
        /// The identifier for the OpcUaFdi7_BinarySchema_EddDataTypeInfo Variable.
        /// </summary>
        public const uint OpcUaFdi7_BinarySchema_EddDataTypeInfo = 2217;

        /// <summary>
        /// The identifier for the OpcUaFdi7_XmlSchema Variable.
        /// </summary>
        public const uint OpcUaFdi7_XmlSchema = 2207;

        /// <summary>
        /// The identifier for the OpcUaFdi7_XmlSchema_NamespaceUri Variable.
        /// </summary>
        public const uint OpcUaFdi7_XmlSchema_NamespaceUri = 2209;

        /// <summary>
        /// The identifier for the OpcUaFdi7_XmlSchema_Deprecated Variable.
        /// </summary>
        public const uint OpcUaFdi7_XmlSchema_Deprecated = 8004;

        /// <summary>
        /// The identifier for the OpcUaFdi7_XmlSchema_EddDataTypeInfo Variable.
        /// </summary>
        public const uint OpcUaFdi7_XmlSchema_EddDataTypeInfo = 2210;
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
        /// The identifier for the EddDataTypeEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId EddDataTypeEnum = new ExpandedNodeId(Opc.Ua.Fdi7.DataTypes.EddDataTypeEnum, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the EddDataTypeInfo DataType.
        /// </summary>
        public static readonly ExpandedNodeId EddDataTypeInfo = new ExpandedNodeId(Opc.Ua.Fdi7.DataTypes.EddDataTypeInfo, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);
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
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_Open Method.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7NamespaceMetadata_NamespaceFile_Open = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.OpcUaFdi7NamespaceMetadata_NamespaceFile_Open, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_Close Method.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7NamespaceMetadata_NamespaceFile_Close = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.OpcUaFdi7NamespaceMetadata_NamespaceFile_Close, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_Read Method.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7NamespaceMetadata_NamespaceFile_Read = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.OpcUaFdi7NamespaceMetadata_NamespaceFile_Read, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_Write Method.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7NamespaceMetadata_NamespaceFile_Write = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.OpcUaFdi7NamespaceMetadata_NamespaceFile_Write, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_GetPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7NamespaceMetadata_NamespaceFile_GetPosition = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.OpcUaFdi7NamespaceMetadata_NamespaceFile_GetPosition, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_SetPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7NamespaceMetadata_NamespaceFile_SetPosition = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.OpcUaFdi7NamespaceMetadata_NamespaceFile_SetPosition, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_H1_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Foundation_H1_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_H1_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Foundation_H1_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_H1_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Foundation_H1_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_H1_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Foundation_H1_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_HSE_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Foundation_HSE_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_HSE_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Foundation_HSE_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_HSE_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Foundation_HSE_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_HSE_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Foundation_HSE_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profibus_DP_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Profibus_DP_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profibus_DP_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Profibus_DP_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profibus_DP_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Profibus_DP_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profibus_DP_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Profibus_DP_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Profinet_IO_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Profinet_IO_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Profinet_IO_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Profinet_IO_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_HART_TP5_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_HART_TP5_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_HART_TP5_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_HART_TP5_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_HART_TP6_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_HART_TP6_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_HART_TP6_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_HART_TP6_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_HART_TP7_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_HART_TP7_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_HART_TP7_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_HART_TP7_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_ISA100_Wireless_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_ISA100_Wireless_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_ISA100_Wireless_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_ISA100_Wireless_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericConnectionPoint_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId GenericConnectionPoint_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.GenericConnectionPoint_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericConnectionPoint_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId GenericConnectionPoint_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.GenericConnectionPoint_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericConnectionPoint_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId GenericConnectionPoint_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.GenericConnectionPoint_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericConnectionPoint_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId GenericConnectionPoint_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.GenericConnectionPoint_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericConnectionPoint_NetworkIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId GenericConnectionPoint_NetworkIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.GenericConnectionPoint_NetworkIdentifier_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericConnectionPoint_NetworkIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId GenericConnectionPoint_NetworkIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.GenericConnectionPoint_NetworkIdentifier_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericConnectionPoint_NetworkIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId GenericConnectionPoint_NetworkIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.GenericConnectionPoint_NetworkIdentifier_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericConnectionPoint_NetworkIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId GenericConnectionPoint_NetworkIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.GenericConnectionPoint_NetworkIdentifier_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.CommunicationServerType_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.CommunicationServerType_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.CommunicationServerType_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.CommunicationServerType_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_CPIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.CommunicationServerType_CPIdentifier_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_CPIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.CommunicationServerType_CPIdentifier_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_CPIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.CommunicationServerType_CPIdentifier_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_CPIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.CommunicationServerType_CPIdentifier_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_MethodSet_Initialize Method.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_MethodSet_Initialize = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.CommunicationServerType_MethodSet_Initialize, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_MethodSet_Reset Method.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_MethodSet_Reset = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.CommunicationServerType_MethodSet_Reset, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_MethodSet_AddComponent Method.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_MethodSet_AddComponent = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.CommunicationServerType_MethodSet_AddComponent, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_MethodSet_RemoveComponent Method.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_MethodSet_RemoveComponent = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.CommunicationServerType_MethodSet_RemoveComponent, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the SetAddressMethodFFH1Type Method.
        /// </summary>
        public static readonly ExpandedNodeId SetAddressMethodFFH1Type = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.SetAddressMethodFFH1Type, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the SetAddressMethodFFHSEType Method.
        /// </summary>
        public static readonly ExpandedNodeId SetAddressMethodFFHSEType = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.SetAddressMethodFFHSEType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the SetAddressMethodPROFIBUSType Method.
        /// </summary>
        public static readonly ExpandedNodeId SetAddressMethodPROFIBUSType = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.SetAddressMethodPROFIBUSType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the SetAddressMethodPROFINETType Method.
        /// </summary>
        public static readonly ExpandedNodeId SetAddressMethodPROFINETType = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.SetAddressMethodPROFINETType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the SetAddressMethodHARTType Method.
        /// </summary>
        public static readonly ExpandedNodeId SetAddressMethodHARTType = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.SetAddressMethodHARTType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the SetAddressMethodGENERICType Method.
        /// </summary>
        public static readonly ExpandedNodeId SetAddressMethodGENERICType = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.SetAddressMethodGENERICType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationDeviceType_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationDeviceType_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationDeviceType_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationDeviceType_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_CPIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationDeviceType_CPIdentifier_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_CPIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationDeviceType_CPIdentifier_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_CPIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationDeviceType_CPIdentifier_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_CPIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationDeviceType_CPIdentifier_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_MethodSet_Scan Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_MethodSet_Scan = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationDeviceType_MethodSet_Scan, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_MethodSet_ResetScan Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_MethodSet_ResetScan = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationDeviceType_MethodSet_ResetScan, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_MethodSet_AddComponent Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_MethodSet_AddComponent = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationDeviceType_MethodSet_AddComponent, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_MethodSet_RemoveComponent Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_MethodSet_RemoveComponent = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationDeviceType_MethodSet_RemoveComponent, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFH1DeviceType_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFH1DeviceType_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFH1DeviceType_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFH1DeviceType_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_MethodSet_SetAddress Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_MethodSet_SetAddress = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFH1DeviceType_MethodSet_SetAddress, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Disconnect Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Disconnect = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Disconnect, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Connect Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Connect = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Connect, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Transfer Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Transfer = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Transfer, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFHSEDeviceType_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFHSEDeviceType_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFHSEDeviceType_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFHSEDeviceType_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_MethodSet_SetAddress Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_MethodSet_SetAddress = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFHSEDeviceType_MethodSet_SetAddress, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Disconnect Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Disconnect = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Disconnect, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Connect Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Connect = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Connect, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Transfer Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Transfer = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Transfer, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFIBUSDeviceType_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFIBUSDeviceType_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFIBUSDeviceType_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFIBUSDeviceType_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_MethodSet_SetAddress Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_MethodSet_SetAddress = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFIBUSDeviceType_MethodSet_SetAddress, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Disconnect Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Disconnect = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Disconnect, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Connect Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Connect = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Connect, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Transfer Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Transfer = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Transfer, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFINETDeviceType_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFINETDeviceType_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFINETDeviceType_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFINETDeviceType_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_MethodSet_SetAddress Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_MethodSet_SetAddress = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFINETDeviceType_MethodSet_SetAddress, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Disconnect Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Disconnect = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Disconnect, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Connect Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Connect = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Connect, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Transfer Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Transfer = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Transfer, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationHARType_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationHARType_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationHARType_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationHARType_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_CPIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationHARType_CPIdentifier_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_CPIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationHARType_CPIdentifier_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_CPIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationHARType_CPIdentifier_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_CPIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationHARType_CPIdentifier_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_MethodSet_SetAddress Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_MethodSet_SetAddress = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationHARType_MethodSet_SetAddress, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationHARType_ServiceProvider_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationHARType_ServiceProvider_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationHARType_ServiceProvider_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationHARType_ServiceProvider_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_MethodSet_Disconnect Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_MethodSet_Disconnect = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationHARType_ServiceProvider_MethodSet_Disconnect, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_MethodSet_Connect Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_MethodSet_Connect = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationHARType_ServiceProvider_MethodSet_Connect, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_MethodSet_Transfer Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_MethodSet_Transfer = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationHARType_ServiceProvider_MethodSet_Transfer, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationISA100_WirelessDeviceType_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationISA100_WirelessDeviceType_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationISA100_WirelessDeviceType_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationISA100_WirelessDeviceType_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Disconnect Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Disconnect = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Disconnect, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Connect Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Connect = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Connect, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Transfer Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Transfer = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Transfer, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationGENERICDeviceType_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationGENERICDeviceType_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationGENERICDeviceType_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationGENERICDeviceType_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_MethodSet_SetAddress Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_MethodSet_SetAddress = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationGENERICDeviceType_MethodSet_SetAddress, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Disconnect Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Disconnect = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Disconnect, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Connect Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Connect = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Connect, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Transfer Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Transfer = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Transfer, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectMethodFFH1Type Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectMethodFFH1Type = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectMethodFFH1Type, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectMethodFFHSEType Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectMethodFFHSEType = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectMethodFFHSEType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectMethodPROFIBUSType Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectMethodPROFIBUSType = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectMethodPROFIBUSType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectMethodPROFINETType Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectMethodPROFINETType = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectMethodPROFINETType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectMethodHARTType Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectMethodHARTType = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectMethodHARTType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectMethodISA100_WirelessType Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectMethodISA100_WirelessType = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectMethodISA100_WirelessType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectMethodGENERICType Method.
        /// </summary>
        public static readonly ExpandedNodeId ConnectMethodGENERICType = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ConnectMethodGENERICType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the TransferMethodFFH1Type Method.
        /// </summary>
        public static readonly ExpandedNodeId TransferMethodFFH1Type = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.TransferMethodFFH1Type, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the TransferMethodFFHSEType Method.
        /// </summary>
        public static readonly ExpandedNodeId TransferMethodFFHSEType = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.TransferMethodFFHSEType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the TransferMethodPROFIBUSType Method.
        /// </summary>
        public static readonly ExpandedNodeId TransferMethodPROFIBUSType = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.TransferMethodPROFIBUSType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the TransferMethodPROFINETType Method.
        /// </summary>
        public static readonly ExpandedNodeId TransferMethodPROFINETType = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.TransferMethodPROFINETType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the TransferMethodHARTType Method.
        /// </summary>
        public static readonly ExpandedNodeId TransferMethodHARTType = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.TransferMethodHARTType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the TransferMethodISA100_WirelessType Method.
        /// </summary>
        public static readonly ExpandedNodeId TransferMethodISA100_WirelessType = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.TransferMethodISA100_WirelessType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the TransferMethodGENERICType Method.
        /// </summary>
        public static readonly ExpandedNodeId TransferMethodGENERICType = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.TransferMethodGENERICType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GetPublishedDataMethodFFH1Type Method.
        /// </summary>
        public static readonly ExpandedNodeId GetPublishedDataMethodFFH1Type = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.GetPublishedDataMethodFFH1Type, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GetPublishedDataMethodFFHSEType Method.
        /// </summary>
        public static readonly ExpandedNodeId GetPublishedDataMethodFFHSEType = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.GetPublishedDataMethodFFHSEType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GetPublishedDataMethodHARTType Method.
        /// </summary>
        public static readonly ExpandedNodeId GetPublishedDataMethodHARTType = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.GetPublishedDataMethodHARTType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GetPublishedDataMethodISA100_WirelessType Method.
        /// </summary>
        public static readonly ExpandedNodeId GetPublishedDataMethodISA100_WirelessType = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.GetPublishedDataMethodISA100_WirelessType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationServiceType_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationServiceType_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationServiceType_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationServiceType_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_CPIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationServiceType_CPIdentifier_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_CPIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationServiceType_CPIdentifier_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_CPIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationServiceType_CPIdentifier_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_CPIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationServiceType_CPIdentifier_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_MethodSet_Disconnect Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_MethodSet_Disconnect = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationServiceType_MethodSet_Disconnect, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFH1ServiceType_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFH1ServiceType_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFH1ServiceType_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFH1ServiceType_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_MethodSet_Disconnect Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_MethodSet_Disconnect = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFH1ServiceType_MethodSet_Disconnect, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_MethodSet_Connect Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_MethodSet_Connect = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFH1ServiceType_MethodSet_Connect, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_MethodSet_Transfer Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_MethodSet_Transfer = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFH1ServiceType_MethodSet_Transfer, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_MethodSet_GetPublishedData Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_MethodSet_GetPublishedData = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFH1ServiceType_MethodSet_GetPublishedData, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFHSEServiceType_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFHSEServiceType_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFHSEServiceType_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFHSEServiceType_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_MethodSet_Disconnect Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_MethodSet_Disconnect = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFHSEServiceType_MethodSet_Disconnect, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_MethodSet_Connect Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_MethodSet_Connect = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFHSEServiceType_MethodSet_Connect, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_MethodSet_Transfer Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_MethodSet_Transfer = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFHSEServiceType_MethodSet_Transfer, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_MethodSet_GetPublishedData Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_MethodSet_GetPublishedData = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationFFHSEServiceType_MethodSet_GetPublishedData, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFIBUSServiceType_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFIBUSServiceType_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFIBUSServiceType_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFIBUSServiceType_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_MethodSet_Disconnect Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_MethodSet_Disconnect = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFIBUSServiceType_MethodSet_Disconnect, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_MethodSet_Connect Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_MethodSet_Connect = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFIBUSServiceType_MethodSet_Connect, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_MethodSet_Transfer Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_MethodSet_Transfer = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFIBUSServiceType_MethodSet_Transfer, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFINETServiceType_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFINETServiceType_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFINETServiceType_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFINETServiceType_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_MethodSet_Disconnect Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_MethodSet_Disconnect = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFINETServiceType_MethodSet_Disconnect, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_MethodSet_Connect Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_MethodSet_Connect = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFINETServiceType_MethodSet_Connect, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_MethodSet_Transfer Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_MethodSet_Transfer = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationPROFINETServiceType_MethodSet_Transfer, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationHARTServiceType_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationHARTServiceType_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationHARTServiceType_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationHARTServiceType_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_CPIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationHARTServiceType_CPIdentifier_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_CPIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationHARTServiceType_CPIdentifier_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_CPIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationHARTServiceType_CPIdentifier_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_CPIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationHARTServiceType_CPIdentifier_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_MethodSet_Disconnect Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_MethodSet_Disconnect = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationHARTServiceType_MethodSet_Disconnect, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_MethodSet_Connect Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_MethodSet_Connect = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationHARTServiceType_MethodSet_Connect, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_MethodSet_Transfer Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_MethodSet_Transfer = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationHARTServiceType_MethodSet_Transfer, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_MethodSet_GetPublishedData Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_MethodSet_GetPublishedData = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationHARTServiceType_MethodSet_GetPublishedData, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationISA100_WirelessServiceType_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationISA100_WirelessServiceType_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationISA100_WirelessServiceType_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationISA100_WirelessServiceType_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_MethodSet_Disconnect Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_MethodSet_Disconnect = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationISA100_WirelessServiceType_MethodSet_Disconnect, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_MethodSet_Connect Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_MethodSet_Connect = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationISA100_WirelessServiceType_MethodSet_Connect, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_MethodSet_Transfer Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_MethodSet_Transfer = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationISA100_WirelessServiceType_MethodSet_Transfer, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_MethodSet_GetPublishedData Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_MethodSet_GetPublishedData = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationISA100_WirelessServiceType_MethodSet_GetPublishedData, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationGENERICServiceType_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationGENERICServiceType_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationGENERICServiceType_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationGENERICServiceType_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_CPIdentifier_Lock_InitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_CPIdentifier_Lock_InitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationGENERICServiceType_CPIdentifier_Lock_InitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_CPIdentifier_Lock_RenewLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_CPIdentifier_Lock_RenewLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationGENERICServiceType_CPIdentifier_Lock_RenewLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_CPIdentifier_Lock_ExitLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_CPIdentifier_Lock_ExitLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationGENERICServiceType_CPIdentifier_Lock_ExitLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_CPIdentifier_Lock_BreakLock Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_CPIdentifier_Lock_BreakLock = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationGENERICServiceType_CPIdentifier_Lock_BreakLock, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_MethodSet_Disconnect Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_MethodSet_Disconnect = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationGENERICServiceType_MethodSet_Disconnect, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_MethodSet_Connect Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_MethodSet_Connect = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationGENERICServiceType_MethodSet_Connect, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_MethodSet_Transfer Method.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_MethodSet_Transfer = new ExpandedNodeId(Opc.Ua.Fdi7.Methods.ServerCommunicationGENERICServiceType_MethodSet_Transfer, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);
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
        /// The identifier for the OpcUaFdi7NamespaceMetadata Object.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7NamespaceMetadata = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.OpcUaFdi7NamespaceMetadata, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_ParameterSet Object.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_ParameterSet = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.CommunicationServerType_ParameterSet, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_MethodSet Object.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_MethodSet = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.CommunicationServerType_MethodSet, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_CPIdentifier_NetworkAddress = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.CommunicationServerType_CPIdentifier_NetworkAddress, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_SubDevices Object.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_SubDevices = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.CommunicationServerType_SubDevices, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_MethodSet Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_MethodSet = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationDeviceType_MethodSet, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_CPIdentifier_NetworkAddress = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationDeviceType_CPIdentifier_NetworkAddress, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_MethodSet Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_MethodSet = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationFFH1DeviceType_MethodSet, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_CPIdentifier_NetworkAddress = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationFFH1DeviceType_CPIdentifier_NetworkAddress, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationFFH1DeviceType_ServiceProvider, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_MethodSet Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_MethodSet = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationFFHSEDeviceType_MethodSet, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_CPIdentifier_NetworkAddress = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationFFHSEDeviceType_CPIdentifier_NetworkAddress, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationFFHSEDeviceType_ServiceProvider, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_MethodSet Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_MethodSet = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationPROFIBUSDeviceType_MethodSet, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_CPIdentifier_NetworkAddress = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationPROFIBUSDeviceType_CPIdentifier_NetworkAddress, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationPROFIBUSDeviceType_ServiceProvider, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_MethodSet Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_MethodSet = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationPROFINETDeviceType_MethodSet, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_CPIdentifier_NetworkAddress = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationPROFINETDeviceType_CPIdentifier_NetworkAddress, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationPROFINETDeviceType_ServiceProvider, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_MethodSet Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_MethodSet = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationHARType_MethodSet, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_CPIdentifier_NetworkAddress = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationHARType_CPIdentifier_NetworkAddress, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationHARType_ServiceProvider, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_MethodSet Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_MethodSet = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationHARType_ServiceProvider_MethodSet, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_NetworkAddress = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_NetworkAddress, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_MethodSet Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_MethodSet = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationGENERICDeviceType_MethodSet, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_CPIdentifier_NetworkAddress = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationGENERICDeviceType_CPIdentifier_NetworkAddress, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationGENERICDeviceType_ServiceProvider, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_MethodSet Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_MethodSet = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationServiceType_MethodSet, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_CPIdentifier_NetworkAddress = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationServiceType_CPIdentifier_NetworkAddress, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_MethodSet Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_MethodSet = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationFFH1ServiceType_MethodSet, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_CPIdentifier_NetworkAddress = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationFFH1ServiceType_CPIdentifier_NetworkAddress, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_MethodSet Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_MethodSet = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationFFHSEServiceType_MethodSet, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_CPIdentifier_NetworkAddress = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationFFHSEServiceType_CPIdentifier_NetworkAddress, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_MethodSet Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_MethodSet = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationPROFIBUSServiceType_MethodSet, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_CPIdentifier_NetworkAddress = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationPROFIBUSServiceType_CPIdentifier_NetworkAddress, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_MethodSet Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_MethodSet = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationPROFINETServiceType_MethodSet, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_CPIdentifier_NetworkAddress = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationPROFINETServiceType_CPIdentifier_NetworkAddress, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_MethodSet Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_MethodSet = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationHARTServiceType_MethodSet, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_CPIdentifier_NetworkAddress = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationHARTServiceType_CPIdentifier_NetworkAddress, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_MethodSet Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_MethodSet = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationISA100_WirelessServiceType_MethodSet, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_CPIdentifier_NetworkAddress = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationISA100_WirelessServiceType_CPIdentifier_NetworkAddress, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_MethodSet Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_MethodSet = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationGENERICServiceType_MethodSet, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_CPIdentifier_NetworkAddress Object.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_CPIdentifier_NetworkAddress = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.ServerCommunicationGENERICServiceType_CPIdentifier_NetworkAddress, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the EddDataTypeInfo_Encoding_DefaultBinary Object.
        /// </summary>
        public static readonly ExpandedNodeId EddDataTypeInfo_Encoding_DefaultBinary = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.EddDataTypeInfo_Encoding_DefaultBinary, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the EddDataTypeInfo_Encoding_DefaultXml Object.
        /// </summary>
        public static readonly ExpandedNodeId EddDataTypeInfo_Encoding_DefaultXml = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.EddDataTypeInfo_Encoding_DefaultXml, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the EddDataTypeInfo_Encoding_DefaultJson Object.
        /// </summary>
        public static readonly ExpandedNodeId EddDataTypeInfo_Encoding_DefaultJson = new ExpandedNodeId(Opc.Ua.Fdi7.Objects.EddDataTypeInfo_Encoding_DefaultJson, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);
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
        /// The identifier for the Foundation_H1 ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId Foundation_H1 = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.Foundation_H1, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the Foundation_HSE ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId Foundation_HSE = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.Foundation_HSE, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the Profibus_DP ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId Profibus_DP = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.Profibus_DP, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the Profibus_PA ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId Profibus_PA = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.Profibus_PA, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the Profinet_IO ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId Profinet_IO = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.Profinet_IO, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the HART ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId HART = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.HART, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ISA100_Wireless ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ISA100_Wireless = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.ISA100_Wireless, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericProtocol ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId GenericProtocol = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.GenericProtocol, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1 ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_H1 = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.ConnectionPoint_Foundation_H1, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_HSE = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.ConnectionPoint_Foundation_HSE, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profibus_DP = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.ConnectionPoint_Profibus_DP, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.ConnectionPoint_Profinet_IO, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5 ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5 = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.ConnectionPoint_HART_TP5, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6 ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6 = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.ConnectionPoint_HART_TP6, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7 ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7 = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.ConnectionPoint_HART_TP7, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.ConnectionPoint_ISA100_Wireless, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericConnectionPoint ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId GenericConnectionPoint = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.GenericConnectionPoint, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.CommunicationServerType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationDeviceType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationFFH1DeviceType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationFFHSEDeviceType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationPROFIBUSDeviceType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationPROFINETDeviceType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationHARType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationISA100_WirelessDeviceType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationGENERICDeviceType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationServiceType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationFFH1ServiceType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationFFHSEServiceType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationPROFIBUSServiceType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationPROFINETServiceType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationHARTServiceType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationISA100_WirelessServiceType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType = new ExpandedNodeId(Opc.Ua.Fdi7.ObjectTypes.ServerCommunicationGENERICServiceType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);
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
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7NamespaceMetadata_NamespaceUri = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7NamespaceMetadata_NamespaceUri, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceVersion Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7NamespaceMetadata_NamespaceVersion = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7NamespaceMetadata_NamespaceVersion, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespacePublicationDate Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7NamespaceMetadata_NamespacePublicationDate = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7NamespaceMetadata_NamespacePublicationDate, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_IsNamespaceSubset Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7NamespaceMetadata_IsNamespaceSubset = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7NamespaceMetadata_IsNamespaceSubset, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_StaticNodeIdTypes Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7NamespaceMetadata_StaticNodeIdTypes = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7NamespaceMetadata_StaticNodeIdTypes, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_StaticNumericNodeIdRange Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7NamespaceMetadata_StaticNumericNodeIdRange = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7NamespaceMetadata_StaticNumericNodeIdRange, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_StaticStringNodeIdPattern Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7NamespaceMetadata_StaticStringNodeIdPattern = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7NamespaceMetadata_StaticStringNodeIdPattern, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_Size Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7NamespaceMetadata_NamespaceFile_Size = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7NamespaceMetadata_NamespaceFile_Size, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_Writable Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7NamespaceMetadata_NamespaceFile_Writable = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7NamespaceMetadata_NamespaceFile_Writable, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_UserWritable Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7NamespaceMetadata_NamespaceFile_UserWritable = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7NamespaceMetadata_NamespaceFile_UserWritable, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_OpenCount Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7NamespaceMetadata_NamespaceFile_OpenCount = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7NamespaceMetadata_NamespaceFile_OpenCount, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_Open_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7NamespaceMetadata_NamespaceFile_Open_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7NamespaceMetadata_NamespaceFile_Open_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_Open_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7NamespaceMetadata_NamespaceFile_Open_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7NamespaceMetadata_NamespaceFile_Open_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_Close_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7NamespaceMetadata_NamespaceFile_Close_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7NamespaceMetadata_NamespaceFile_Close_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_Read_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7NamespaceMetadata_NamespaceFile_Read_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7NamespaceMetadata_NamespaceFile_Read_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_Read_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7NamespaceMetadata_NamespaceFile_Read_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7NamespaceMetadata_NamespaceFile_Read_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_Write_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7NamespaceMetadata_NamespaceFile_Write_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7NamespaceMetadata_NamespaceFile_Write_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_GetPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7NamespaceMetadata_NamespaceFile_GetPosition_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7NamespaceMetadata_NamespaceFile_GetPosition_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_GetPosition_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7NamespaceMetadata_NamespaceFile_GetPosition_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7NamespaceMetadata_NamespaceFile_GetPosition_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_NamespaceFile_SetPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7NamespaceMetadata_NamespaceFile_SetPosition_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7NamespaceMetadata_NamespaceFile_SetPosition_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_DefaultRolePermissions Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7NamespaceMetadata_DefaultRolePermissions = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7NamespaceMetadata_DefaultRolePermissions, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_DefaultUserRolePermissions Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7NamespaceMetadata_DefaultUserRolePermissions = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7NamespaceMetadata_DefaultUserRolePermissions, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7NamespaceMetadata_DefaultAccessRestrictions Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7NamespaceMetadata_DefaultAccessRestrictions = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7NamespaceMetadata_DefaultAccessRestrictions, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericProtocol_ProtocolIdentifier Variable.
        /// </summary>
        public static readonly ExpandedNodeId GenericProtocol_ProtocolIdentifier = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.GenericProtocol_ProtocolIdentifier, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_H1_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_H1_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_H1_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_H1_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_H1_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_H1_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_H1_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_H1_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_H1_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_H1_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_H1_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_H1_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_H1_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_H1_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_H1_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_H1_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_H1_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_H1_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_H1_NetworkIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_Address Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_H1_Address = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_H1_Address, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_OrdinalNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_H1_OrdinalNumber = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_H1_OrdinalNumber, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_H1_SIFConnection Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_H1_SIFConnection = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_H1_SIFConnection, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_HSE_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_HSE_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_HSE_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_HSE_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_HSE_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_HSE_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_HSE_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_HSE_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_HSE_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_HSE_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_HSE_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_HSE_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_HSE_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_HSE_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_HSE_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_HSE_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_HSE_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_HSE_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_HSE_NetworkIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_Address Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_HSE_Address = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_HSE_Address, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Foundation_HSE_OrdinalNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Foundation_HSE_OrdinalNumber = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Foundation_HSE_OrdinalNumber, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profibus_DP_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profibus_DP_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profibus_DP_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profibus_DP_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profibus_DP_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profibus_DP_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profibus_DP_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profibus_DP_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profibus_DP_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profibus_DP_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profibus_DP_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profibus_DP_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profibus_DP_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profibus_DP_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profibus_DP_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profibus_DP_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profibus_DP_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profibus_DP_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profibus_DP_NetworkIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profibus_DP_Address Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profibus_DP_Address = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profibus_DP_Address, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profinet_IO_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profinet_IO_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profinet_IO_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profinet_IO_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profinet_IO_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profinet_IO_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profinet_IO_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profinet_IO_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profinet_IO_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profinet_IO_NetworkIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_MAC Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO_MAC = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profinet_IO_MAC, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_IPv4 Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO_IPv4 = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profinet_IO_IPv4, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_DNSNAME Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO_DNSNAME = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profinet_IO_DNSNAME, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_Profinet_IO_VALID Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_Profinet_IO_VALID = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_Profinet_IO_VALID, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP5_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP5_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP5_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP5_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP5_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP5_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP5_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP5_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP5_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP5_NetworkIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_DevAddr Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_DevAddr = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP5_DevAddr, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_DevMfg Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_DevMfg = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP5_DevMfg, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_DevType Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_DevType = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP5_DevType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_DevRev Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_DevRev = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP5_DevRev, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_DevTag Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_DevTag = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP5_DevTag, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP5_DevPollAddr Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP5_DevPollAddr = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP5_DevPollAddr, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP6_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP6_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP6_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP6_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP6_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP6_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP6_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP6_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP6_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP6_NetworkIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_DevAddr Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_DevAddr = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP6_DevAddr, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_DevMfg Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_DevMfg = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP6_DevMfg, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_DevType Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_DevType = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP6_DevType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_DevRev Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_DevRev = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP6_DevRev, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_DevTag Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_DevTag = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP6_DevTag, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP6_DevPollAddr Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP6_DevPollAddr = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP6_DevPollAddr, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP7_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP7_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP7_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP7_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP7_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP7_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP7_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP7_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP7_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP7_NetworkIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_DevAddr Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_DevAddr = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP7_DevAddr, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_DevMfg Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_DevMfg = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP7_DevMfg, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_DevType Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_DevType = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP7_DevType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_DevRev Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_DevRev = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP7_DevRev, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_DevTag Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_DevTag = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP7_DevTag, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_HART_TP7_DevPollAddr Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_HART_TP7_DevPollAddr = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_HART_TP7_DevPollAddr, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_ISA100_Wireless_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_ISA100_Wireless_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_ISA100_Wireless_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_ISA100_Wireless_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_ISA100_Wireless_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_ISA100_Wireless_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_ISA100_Wireless_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_ISA100_Wireless_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_ISA100_Wireless_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_ISA100_Wireless_NetworkIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_IPAddress Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_IPAddress = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_ISA100_Wireless_IPAddress, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_DevMfg Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_DevMfg = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_ISA100_Wireless_DevMfg, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_DevType Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_DevType = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_ISA100_Wireless_DevType, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_DevRev Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_DevRev = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_ISA100_Wireless_DevRev, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_DevTag Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_DevTag = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_ISA100_Wireless_DevTag, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectionPoint_ISA100_Wireless_DevPollAddr Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectionPoint_ISA100_Wireless_DevPollAddr = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectionPoint_ISA100_Wireless_DevPollAddr, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericConnectionPoint_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId GenericConnectionPoint_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.GenericConnectionPoint_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericConnectionPoint_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId GenericConnectionPoint_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.GenericConnectionPoint_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericConnectionPoint_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId GenericConnectionPoint_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.GenericConnectionPoint_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericConnectionPoint_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId GenericConnectionPoint_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.GenericConnectionPoint_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericConnectionPoint_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId GenericConnectionPoint_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.GenericConnectionPoint_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericConnectionPoint_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId GenericConnectionPoint_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.GenericConnectionPoint_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericConnectionPoint_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId GenericConnectionPoint_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.GenericConnectionPoint_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericConnectionPoint_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId GenericConnectionPoint_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.GenericConnectionPoint_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericConnectionPoint_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId GenericConnectionPoint_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.GenericConnectionPoint_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericConnectionPoint_NetworkIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId GenericConnectionPoint_NetworkIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.GenericConnectionPoint_NetworkIdentifier_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericConnectionPoint_NetworkIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId GenericConnectionPoint_NetworkIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.GenericConnectionPoint_NetworkIdentifier_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericConnectionPoint_NetworkIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId GenericConnectionPoint_NetworkIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.GenericConnectionPoint_NetworkIdentifier_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericConnectionPoint_NetworkIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId GenericConnectionPoint_NetworkIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.GenericConnectionPoint_NetworkIdentifier_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericConnectionPoint_NetworkIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId GenericConnectionPoint_NetworkIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.GenericConnectionPoint_NetworkIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericConnectionPoint_NetworkIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId GenericConnectionPoint_NetworkIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.GenericConnectionPoint_NetworkIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericConnectionPoint_NetworkIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId GenericConnectionPoint_NetworkIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.GenericConnectionPoint_NetworkIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericConnectionPoint_NetworkIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId GenericConnectionPoint_NetworkIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.GenericConnectionPoint_NetworkIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericConnectionPoint_NetworkIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId GenericConnectionPoint_NetworkIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.GenericConnectionPoint_NetworkIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericConnectionPoint_Address Variable.
        /// </summary>
        public static readonly ExpandedNodeId GenericConnectionPoint_Address = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.GenericConnectionPoint_Address, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GenericConnectionPoint_ProtocolIdentifier Variable.
        /// </summary>
        public static readonly ExpandedNodeId GenericConnectionPoint_ProtocolIdentifier = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.GenericConnectionPoint_ProtocolIdentifier, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.CommunicationServerType_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.CommunicationServerType_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.CommunicationServerType_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.CommunicationServerType_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.CommunicationServerType_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.CommunicationServerType_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.CommunicationServerType_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.CommunicationServerType_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.CommunicationServerType_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_CPIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.CommunicationServerType_CPIdentifier_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_CPIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.CommunicationServerType_CPIdentifier_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_CPIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.CommunicationServerType_CPIdentifier_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_CPIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.CommunicationServerType_CPIdentifier_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_CPIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.CommunicationServerType_CPIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_CPIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.CommunicationServerType_CPIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_CPIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.CommunicationServerType_CPIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_CPIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.CommunicationServerType_CPIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_CPIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.CommunicationServerType_CPIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_MethodSet_Initialize_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_MethodSet_Initialize_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.CommunicationServerType_MethodSet_Initialize_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_MethodSet_Reset_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_MethodSet_Reset_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.CommunicationServerType_MethodSet_Reset_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_MethodSet_AddComponent_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_MethodSet_AddComponent_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.CommunicationServerType_MethodSet_AddComponent_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_MethodSet_AddComponent_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_MethodSet_AddComponent_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.CommunicationServerType_MethodSet_AddComponent_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_MethodSet_RemoveComponent_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_MethodSet_RemoveComponent_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.CommunicationServerType_MethodSet_RemoveComponent_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the CommunicationServerType_MethodSet_RemoveComponent_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommunicationServerType_MethodSet_RemoveComponent_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.CommunicationServerType_MethodSet_RemoveComponent_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the SetAddressMethodFFH1Type_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SetAddressMethodFFH1Type_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.SetAddressMethodFFH1Type_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the SetAddressMethodFFH1Type_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SetAddressMethodFFH1Type_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.SetAddressMethodFFH1Type_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the SetAddressMethodFFHSEType_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SetAddressMethodFFHSEType_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.SetAddressMethodFFHSEType_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the SetAddressMethodFFHSEType_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SetAddressMethodFFHSEType_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.SetAddressMethodFFHSEType_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the SetAddressMethodPROFIBUSType_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SetAddressMethodPROFIBUSType_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.SetAddressMethodPROFIBUSType_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the SetAddressMethodPROFIBUSType_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SetAddressMethodPROFIBUSType_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.SetAddressMethodPROFIBUSType_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the SetAddressMethodPROFINETType_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SetAddressMethodPROFINETType_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.SetAddressMethodPROFINETType_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the SetAddressMethodPROFINETType_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SetAddressMethodPROFINETType_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.SetAddressMethodPROFINETType_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the SetAddressMethodHARTType_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SetAddressMethodHARTType_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.SetAddressMethodHARTType_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the SetAddressMethodHARTType_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SetAddressMethodHARTType_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.SetAddressMethodHARTType_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the SetAddressMethodGENERICType_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SetAddressMethodGENERICType_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.SetAddressMethodGENERICType_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the SetAddressMethodGENERICType_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId SetAddressMethodGENERICType_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.SetAddressMethodGENERICType_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationDeviceType_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationDeviceType_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationDeviceType_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationDeviceType_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationDeviceType_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationDeviceType_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationDeviceType_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationDeviceType_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationDeviceType_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_CPIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationDeviceType_CPIdentifier_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_CPIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationDeviceType_CPIdentifier_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_CPIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationDeviceType_CPIdentifier_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_CPIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationDeviceType_CPIdentifier_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_CPIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationDeviceType_CPIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_CPIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationDeviceType_CPIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_ListOfCommunicationProfiles Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_ListOfCommunicationProfiles = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationDeviceType_ListOfCommunicationProfiles, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_MethodSet_Scan_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_MethodSet_Scan_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationDeviceType_MethodSet_Scan_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_MethodSet_ResetScan_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_MethodSet_ResetScan_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationDeviceType_MethodSet_ResetScan_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_MethodSet_AddComponent_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_MethodSet_AddComponent_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationDeviceType_MethodSet_AddComponent_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_MethodSet_AddComponent_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_MethodSet_AddComponent_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationDeviceType_MethodSet_AddComponent_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_MethodSet_RemoveComponent_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_MethodSet_RemoveComponent_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationDeviceType_MethodSet_RemoveComponent_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationDeviceType_MethodSet_RemoveComponent_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationDeviceType_MethodSet_RemoveComponent_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationDeviceType_MethodSet_RemoveComponent_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_CPIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_MethodSet_Scan_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_MethodSet_Scan_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_MethodSet_Scan_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_MethodSet_ResetScan_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_MethodSet_ResetScan_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_MethodSet_ResetScan_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_MethodSet_AddComponent_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_MethodSet_AddComponent_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_MethodSet_AddComponent_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_MethodSet_AddComponent_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_MethodSet_AddComponent_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_MethodSet_AddComponent_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_MethodSet_RemoveComponent_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_MethodSet_RemoveComponent_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_MethodSet_RemoveComponent_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_MethodSet_RemoveComponent_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_MethodSet_RemoveComponent_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_MethodSet_RemoveComponent_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_MethodSet_SetAddress_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_MethodSet_SetAddress_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_MethodSet_SetAddress_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_MethodSet_SetAddress_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_MethodSet_SetAddress_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_MethodSet_SetAddress_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_ServiceProvider_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_Manufacturer Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_Manufacturer = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_ServiceProvider_Manufacturer, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_Model Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_Model = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_ServiceProvider_Model, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_HardwareRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_HardwareRevision = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_ServiceProvider_HardwareRevision, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_SoftwareRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_SoftwareRevision = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_ServiceProvider_SoftwareRevision, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_DeviceRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_DeviceRevision = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_ServiceProvider_DeviceRevision, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_DeviceManual Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_DeviceManual = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_ServiceProvider_DeviceManual, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_SerialNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_SerialNumber = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_ServiceProvider_SerialNumber, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_RevisionCounter Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_RevisionCounter = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_ServiceProvider_RevisionCounter, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Disconnect_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Disconnect_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Disconnect_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Disconnect_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Disconnect_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Disconnect_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Connect_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Connect_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Connect_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Connect_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Connect_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Connect_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Transfer_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Transfer_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Transfer_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Transfer_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Transfer_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_Transfer_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_GetPublishedData_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_GetPublishedData_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_GetPublishedData_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_GetPublishedData_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_GetPublishedData_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1DeviceType_ServiceProvider_MethodSet_GetPublishedData_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_MethodSet_Scan_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_MethodSet_Scan_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_MethodSet_Scan_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_MethodSet_ResetScan_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_MethodSet_ResetScan_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_MethodSet_ResetScan_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_MethodSet_AddComponent_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_MethodSet_AddComponent_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_MethodSet_AddComponent_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_MethodSet_AddComponent_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_MethodSet_AddComponent_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_MethodSet_AddComponent_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_MethodSet_RemoveComponent_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_MethodSet_RemoveComponent_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_MethodSet_RemoveComponent_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_MethodSet_RemoveComponent_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_MethodSet_RemoveComponent_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_MethodSet_RemoveComponent_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_MethodSet_SetAddress_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_MethodSet_SetAddress_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_MethodSet_SetAddress_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_MethodSet_SetAddress_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_MethodSet_SetAddress_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_MethodSet_SetAddress_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_ServiceProvider_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_Manufacturer Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_Manufacturer = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_ServiceProvider_Manufacturer, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_Model Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_Model = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_ServiceProvider_Model, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_HardwareRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_HardwareRevision = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_ServiceProvider_HardwareRevision, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_SoftwareRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_SoftwareRevision = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_ServiceProvider_SoftwareRevision, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_DeviceRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_DeviceRevision = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_ServiceProvider_DeviceRevision, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_DeviceManual Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_DeviceManual = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_ServiceProvider_DeviceManual, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_SerialNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_SerialNumber = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_ServiceProvider_SerialNumber, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_RevisionCounter Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_RevisionCounter = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_ServiceProvider_RevisionCounter, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Disconnect_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Disconnect_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Disconnect_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Disconnect_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Disconnect_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Disconnect_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Connect_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Connect_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Connect_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Connect_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Connect_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Connect_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Transfer_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Transfer_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Transfer_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Transfer_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Transfer_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_Transfer_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_GetPublishedData_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_GetPublishedData_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_GetPublishedData_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_GetPublishedData_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_GetPublishedData_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEDeviceType_ServiceProvider_MethodSet_GetPublishedData_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_MethodSet_Scan_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_MethodSet_Scan_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_MethodSet_Scan_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_MethodSet_ResetScan_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_MethodSet_ResetScan_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_MethodSet_ResetScan_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_MethodSet_AddComponent_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_MethodSet_AddComponent_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_MethodSet_AddComponent_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_MethodSet_AddComponent_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_MethodSet_AddComponent_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_MethodSet_AddComponent_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_MethodSet_RemoveComponent_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_MethodSet_RemoveComponent_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_MethodSet_RemoveComponent_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_MethodSet_RemoveComponent_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_MethodSet_RemoveComponent_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_MethodSet_RemoveComponent_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_MethodSet_SetAddress_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_MethodSet_SetAddress_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_MethodSet_SetAddress_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_MethodSet_SetAddress_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_MethodSet_SetAddress_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_MethodSet_SetAddress_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Manufacturer Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Manufacturer = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Manufacturer, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Model Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Model = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_Model, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_HardwareRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_HardwareRevision = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_HardwareRevision, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_SoftwareRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_SoftwareRevision = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_SoftwareRevision, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_DeviceRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_DeviceRevision = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_DeviceRevision, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_DeviceManual Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_DeviceManual = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_DeviceManual, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_SerialNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_SerialNumber = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_SerialNumber, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_RevisionCounter Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_RevisionCounter = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_RevisionCounter, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Disconnect_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Disconnect_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Disconnect_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Disconnect_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Disconnect_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Disconnect_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Connect_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Connect_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Connect_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Connect_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Connect_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Connect_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Transfer_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Transfer_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Transfer_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Transfer_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Transfer_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSDeviceType_ServiceProvider_MethodSet_Transfer_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_MethodSet_Scan_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_MethodSet_Scan_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_MethodSet_Scan_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_MethodSet_ResetScan_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_MethodSet_ResetScan_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_MethodSet_ResetScan_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_MethodSet_AddComponent_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_MethodSet_AddComponent_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_MethodSet_AddComponent_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_MethodSet_AddComponent_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_MethodSet_AddComponent_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_MethodSet_AddComponent_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_MethodSet_RemoveComponent_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_MethodSet_RemoveComponent_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_MethodSet_RemoveComponent_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_MethodSet_RemoveComponent_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_MethodSet_RemoveComponent_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_MethodSet_RemoveComponent_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_MethodSet_SetAddress_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_MethodSet_SetAddress_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_MethodSet_SetAddress_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_MethodSet_SetAddress_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_MethodSet_SetAddress_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_MethodSet_SetAddress_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_ServiceProvider_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_Manufacturer Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_Manufacturer = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_ServiceProvider_Manufacturer, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_Model Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_Model = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_ServiceProvider_Model, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_HardwareRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_HardwareRevision = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_ServiceProvider_HardwareRevision, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_SoftwareRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_SoftwareRevision = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_ServiceProvider_SoftwareRevision, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_DeviceRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_DeviceRevision = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_ServiceProvider_DeviceRevision, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_DeviceManual Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_DeviceManual = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_ServiceProvider_DeviceManual, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_SerialNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_SerialNumber = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_ServiceProvider_SerialNumber, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_RevisionCounter Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_RevisionCounter = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_ServiceProvider_RevisionCounter, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Disconnect_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Disconnect_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Disconnect_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Disconnect_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Disconnect_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Disconnect_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Connect_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Connect_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Connect_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Connect_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Connect_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Connect_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Transfer_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Transfer_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Transfer_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Transfer_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Transfer_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETDeviceType_ServiceProvider_MethodSet_Transfer_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_CPIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_CPIdentifier_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_CPIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_CPIdentifier_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_CPIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_CPIdentifier_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_CPIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_CPIdentifier_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_CPIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_CPIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_CPIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_CPIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_CPIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_CPIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_CPIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_CPIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_CPIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_CPIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_MethodSet_Scan_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_MethodSet_Scan_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_MethodSet_Scan_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_MethodSet_ResetScan_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_MethodSet_ResetScan_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_MethodSet_ResetScan_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_MethodSet_AddComponent_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_MethodSet_AddComponent_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_MethodSet_AddComponent_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_MethodSet_AddComponent_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_MethodSet_AddComponent_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_MethodSet_AddComponent_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_MethodSet_RemoveComponent_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_MethodSet_RemoveComponent_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_MethodSet_RemoveComponent_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_MethodSet_RemoveComponent_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_MethodSet_RemoveComponent_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_MethodSet_RemoveComponent_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_MethodSet_SetAddress_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_MethodSet_SetAddress_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_MethodSet_SetAddress_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_MethodSet_SetAddress_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_MethodSet_SetAddress_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_MethodSet_SetAddress_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_ServiceProvider_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_ServiceProvider_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_ServiceProvider_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_ServiceProvider_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_ServiceProvider_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_ServiceProvider_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_ServiceProvider_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_ServiceProvider_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_ServiceProvider_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_Manufacturer Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_Manufacturer = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_ServiceProvider_Manufacturer, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_Model Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_Model = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_ServiceProvider_Model, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_HardwareRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_HardwareRevision = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_ServiceProvider_HardwareRevision, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_SoftwareRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_SoftwareRevision = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_ServiceProvider_SoftwareRevision, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_DeviceRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_DeviceRevision = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_ServiceProvider_DeviceRevision, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_DeviceManual Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_DeviceManual = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_ServiceProvider_DeviceManual, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_SerialNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_SerialNumber = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_ServiceProvider_SerialNumber, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_RevisionCounter Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_RevisionCounter = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_ServiceProvider_RevisionCounter, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_MethodSet_Disconnect_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_MethodSet_Disconnect_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_ServiceProvider_MethodSet_Disconnect_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_MethodSet_Disconnect_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_MethodSet_Disconnect_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_ServiceProvider_MethodSet_Disconnect_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_MethodSet_Connect_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_MethodSet_Connect_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_ServiceProvider_MethodSet_Connect_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_MethodSet_Connect_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_MethodSet_Connect_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_ServiceProvider_MethodSet_Connect_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_MethodSet_Transfer_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_MethodSet_Transfer_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_ServiceProvider_MethodSet_Transfer_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_MethodSet_Transfer_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_MethodSet_Transfer_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_ServiceProvider_MethodSet_Transfer_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_MethodSet_GetPublishedData_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_MethodSet_GetPublishedData_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_ServiceProvider_MethodSet_GetPublishedData_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARType_ServiceProvider_MethodSet_GetPublishedData_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARType_ServiceProvider_MethodSet_GetPublishedData_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARType_ServiceProvider_MethodSet_GetPublishedData_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_MethodSet_Scan_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_MethodSet_Scan_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_MethodSet_Scan_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_MethodSet_ResetScan_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_MethodSet_ResetScan_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_MethodSet_ResetScan_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_MethodSet_AddComponent_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_MethodSet_AddComponent_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_MethodSet_AddComponent_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_MethodSet_AddComponent_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_MethodSet_AddComponent_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_MethodSet_AddComponent_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_MethodSet_RemoveComponent_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_MethodSet_RemoveComponent_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_MethodSet_RemoveComponent_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_MethodSet_RemoveComponent_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_MethodSet_RemoveComponent_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_MethodSet_RemoveComponent_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Manufacturer Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Manufacturer = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Manufacturer, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Model Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Model = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_Model, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_HardwareRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_HardwareRevision = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_HardwareRevision, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_SoftwareRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_SoftwareRevision = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_SoftwareRevision, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_DeviceRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_DeviceRevision = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_DeviceRevision, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_DeviceManual Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_DeviceManual = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_DeviceManual, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_SerialNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_SerialNumber = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_SerialNumber, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_RevisionCounter Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_RevisionCounter = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_RevisionCounter, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Disconnect_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Disconnect_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Disconnect_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Disconnect_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Disconnect_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Disconnect_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Connect_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Connect_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Connect_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Connect_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Connect_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Connect_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Transfer_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Transfer_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Transfer_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Transfer_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Transfer_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_Transfer_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_GetPublishedData_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_GetPublishedData_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_GetPublishedData_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_GetPublishedData_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_GetPublishedData_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessDeviceType_ServiceProvider_MethodSet_GetPublishedData_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_CPIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_MethodSet_Scan_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_MethodSet_Scan_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_MethodSet_Scan_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_MethodSet_ResetScan_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_MethodSet_ResetScan_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_MethodSet_ResetScan_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_MethodSet_AddComponent_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_MethodSet_AddComponent_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_MethodSet_AddComponent_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_MethodSet_AddComponent_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_MethodSet_AddComponent_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_MethodSet_AddComponent_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_MethodSet_RemoveComponent_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_MethodSet_RemoveComponent_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_MethodSet_RemoveComponent_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_MethodSet_RemoveComponent_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_MethodSet_RemoveComponent_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_MethodSet_RemoveComponent_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ProtocolIdentifier Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ProtocolIdentifier = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_ProtocolIdentifier, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_MethodSet_SetAddress_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_MethodSet_SetAddress_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_MethodSet_SetAddress_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_MethodSet_SetAddress_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_MethodSet_SetAddress_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_MethodSet_SetAddress_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_ServiceProvider_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_Manufacturer Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_Manufacturer = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_ServiceProvider_Manufacturer, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_Model Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_Model = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_ServiceProvider_Model, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_HardwareRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_HardwareRevision = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_ServiceProvider_HardwareRevision, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_SoftwareRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_SoftwareRevision = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_ServiceProvider_SoftwareRevision, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_DeviceRevision Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_DeviceRevision = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_ServiceProvider_DeviceRevision, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_DeviceManual Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_DeviceManual = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_ServiceProvider_DeviceManual, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_SerialNumber Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_SerialNumber = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_ServiceProvider_SerialNumber, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_RevisionCounter Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_RevisionCounter = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_ServiceProvider_RevisionCounter, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Disconnect_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Disconnect_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Disconnect_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Disconnect_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Disconnect_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Disconnect_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Connect_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Connect_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Connect_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Connect_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Connect_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Connect_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Transfer_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Transfer_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Transfer_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Transfer_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Transfer_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICDeviceType_ServiceProvider_MethodSet_Transfer_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectMethodFFH1Type_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectMethodFFH1Type_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectMethodFFH1Type_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectMethodFFH1Type_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectMethodFFH1Type_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectMethodFFH1Type_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectMethodFFHSEType_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectMethodFFHSEType_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectMethodFFHSEType_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectMethodFFHSEType_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectMethodFFHSEType_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectMethodFFHSEType_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectMethodPROFIBUSType_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectMethodPROFIBUSType_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectMethodPROFIBUSType_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectMethodPROFIBUSType_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectMethodPROFIBUSType_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectMethodPROFIBUSType_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectMethodPROFINETType_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectMethodPROFINETType_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectMethodPROFINETType_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectMethodPROFINETType_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectMethodPROFINETType_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectMethodPROFINETType_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectMethodHARTType_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectMethodHARTType_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectMethodHARTType_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectMethodHARTType_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectMethodHARTType_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectMethodHARTType_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectMethodISA100_WirelessType_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectMethodISA100_WirelessType_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectMethodISA100_WirelessType_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectMethodISA100_WirelessType_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectMethodISA100_WirelessType_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectMethodISA100_WirelessType_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectMethodGENERICType_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectMethodGENERICType_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectMethodGENERICType_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ConnectMethodGENERICType_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ConnectMethodGENERICType_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ConnectMethodGENERICType_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the TransferMethodFFH1Type_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TransferMethodFFH1Type_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.TransferMethodFFH1Type_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the TransferMethodFFH1Type_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TransferMethodFFH1Type_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.TransferMethodFFH1Type_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the TransferMethodFFHSEType_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TransferMethodFFHSEType_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.TransferMethodFFHSEType_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the TransferMethodFFHSEType_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TransferMethodFFHSEType_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.TransferMethodFFHSEType_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the TransferMethodPROFIBUSType_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TransferMethodPROFIBUSType_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.TransferMethodPROFIBUSType_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the TransferMethodPROFIBUSType_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TransferMethodPROFIBUSType_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.TransferMethodPROFIBUSType_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the TransferMethodPROFINETType_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TransferMethodPROFINETType_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.TransferMethodPROFINETType_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the TransferMethodPROFINETType_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TransferMethodPROFINETType_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.TransferMethodPROFINETType_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the TransferMethodHARTType_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TransferMethodHARTType_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.TransferMethodHARTType_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the TransferMethodHARTType_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TransferMethodHARTType_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.TransferMethodHARTType_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the TransferMethodISA100_WirelessType_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TransferMethodISA100_WirelessType_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.TransferMethodISA100_WirelessType_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the TransferMethodISA100_WirelessType_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TransferMethodISA100_WirelessType_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.TransferMethodISA100_WirelessType_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the EddDataTypeEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId EddDataTypeEnum_EnumValues = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.EddDataTypeEnum_EnumValues, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the TransferMethodGENERICType_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TransferMethodGENERICType_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.TransferMethodGENERICType_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the TransferMethodGENERICType_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId TransferMethodGENERICType_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.TransferMethodGENERICType_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GetPublishedDataMethodFFH1Type_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId GetPublishedDataMethodFFH1Type_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.GetPublishedDataMethodFFH1Type_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GetPublishedDataMethodFFH1Type_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId GetPublishedDataMethodFFH1Type_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.GetPublishedDataMethodFFH1Type_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GetPublishedDataMethodFFHSEType_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId GetPublishedDataMethodFFHSEType_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.GetPublishedDataMethodFFHSEType_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GetPublishedDataMethodFFHSEType_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId GetPublishedDataMethodFFHSEType_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.GetPublishedDataMethodFFHSEType_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GetPublishedDataMethodHARTType_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId GetPublishedDataMethodHARTType_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.GetPublishedDataMethodHARTType_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GetPublishedDataMethodHARTType_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId GetPublishedDataMethodHARTType_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.GetPublishedDataMethodHARTType_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GetPublishedDataMethodISA100_WirelessType_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId GetPublishedDataMethodISA100_WirelessType_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.GetPublishedDataMethodISA100_WirelessType_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the GetPublishedDataMethodISA100_WirelessType_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId GetPublishedDataMethodISA100_WirelessType_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.GetPublishedDataMethodISA100_WirelessType_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationServiceType_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationServiceType_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationServiceType_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationServiceType_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationServiceType_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationServiceType_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationServiceType_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationServiceType_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationServiceType_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_CPIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationServiceType_CPIdentifier_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_CPIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationServiceType_CPIdentifier_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_CPIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationServiceType_CPIdentifier_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_CPIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationServiceType_CPIdentifier_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_CPIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationServiceType_CPIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_CPIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationServiceType_CPIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_CPIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationServiceType_CPIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_CPIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationServiceType_CPIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_CPIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationServiceType_CPIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_MethodSet_Disconnect_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_MethodSet_Disconnect_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationServiceType_MethodSet_Disconnect_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationServiceType_MethodSet_Disconnect_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationServiceType_MethodSet_Disconnect_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationServiceType_MethodSet_Disconnect_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1ServiceType_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1ServiceType_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1ServiceType_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1ServiceType_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1ServiceType_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1ServiceType_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1ServiceType_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1ServiceType_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1ServiceType_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1ServiceType_CPIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_MethodSet_Disconnect_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_MethodSet_Disconnect_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1ServiceType_MethodSet_Disconnect_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_MethodSet_Disconnect_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_MethodSet_Disconnect_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1ServiceType_MethodSet_Disconnect_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_MethodSet_Connect_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_MethodSet_Connect_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1ServiceType_MethodSet_Connect_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_MethodSet_Connect_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_MethodSet_Connect_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1ServiceType_MethodSet_Connect_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_MethodSet_Transfer_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_MethodSet_Transfer_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1ServiceType_MethodSet_Transfer_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_MethodSet_Transfer_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_MethodSet_Transfer_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1ServiceType_MethodSet_Transfer_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_MethodSet_GetPublishedData_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_MethodSet_GetPublishedData_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1ServiceType_MethodSet_GetPublishedData_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFH1ServiceType_MethodSet_GetPublishedData_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFH1ServiceType_MethodSet_GetPublishedData_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFH1ServiceType_MethodSet_GetPublishedData_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEServiceType_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEServiceType_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEServiceType_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEServiceType_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEServiceType_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEServiceType_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEServiceType_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEServiceType_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEServiceType_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEServiceType_CPIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_MethodSet_Disconnect_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_MethodSet_Disconnect_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEServiceType_MethodSet_Disconnect_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_MethodSet_Disconnect_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_MethodSet_Disconnect_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEServiceType_MethodSet_Disconnect_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_MethodSet_Connect_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_MethodSet_Connect_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEServiceType_MethodSet_Connect_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_MethodSet_Connect_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_MethodSet_Connect_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEServiceType_MethodSet_Connect_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_MethodSet_Transfer_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_MethodSet_Transfer_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEServiceType_MethodSet_Transfer_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_MethodSet_Transfer_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_MethodSet_Transfer_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEServiceType_MethodSet_Transfer_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_MethodSet_GetPublishedData_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_MethodSet_GetPublishedData_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEServiceType_MethodSet_GetPublishedData_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationFFHSEServiceType_MethodSet_GetPublishedData_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationFFHSEServiceType_MethodSet_GetPublishedData_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationFFHSEServiceType_MethodSet_GetPublishedData_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSServiceType_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSServiceType_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSServiceType_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSServiceType_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSServiceType_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSServiceType_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSServiceType_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSServiceType_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSServiceType_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSServiceType_CPIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_MethodSet_Disconnect_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_MethodSet_Disconnect_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSServiceType_MethodSet_Disconnect_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_MethodSet_Disconnect_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_MethodSet_Disconnect_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSServiceType_MethodSet_Disconnect_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_MethodSet_Connect_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_MethodSet_Connect_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSServiceType_MethodSet_Connect_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_MethodSet_Connect_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_MethodSet_Connect_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSServiceType_MethodSet_Connect_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_MethodSet_Transfer_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_MethodSet_Transfer_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSServiceType_MethodSet_Transfer_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFIBUSServiceType_MethodSet_Transfer_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFIBUSServiceType_MethodSet_Transfer_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFIBUSServiceType_MethodSet_Transfer_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETServiceType_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETServiceType_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETServiceType_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETServiceType_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETServiceType_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETServiceType_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETServiceType_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETServiceType_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETServiceType_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETServiceType_CPIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_MethodSet_Disconnect_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_MethodSet_Disconnect_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETServiceType_MethodSet_Disconnect_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_MethodSet_Disconnect_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_MethodSet_Disconnect_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETServiceType_MethodSet_Disconnect_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_MethodSet_Connect_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_MethodSet_Connect_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETServiceType_MethodSet_Connect_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_MethodSet_Connect_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_MethodSet_Connect_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETServiceType_MethodSet_Connect_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_MethodSet_Transfer_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_MethodSet_Transfer_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETServiceType_MethodSet_Transfer_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationPROFINETServiceType_MethodSet_Transfer_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationPROFINETServiceType_MethodSet_Transfer_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationPROFINETServiceType_MethodSet_Transfer_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARTServiceType_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARTServiceType_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARTServiceType_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARTServiceType_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARTServiceType_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARTServiceType_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARTServiceType_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARTServiceType_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARTServiceType_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_CPIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARTServiceType_CPIdentifier_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_CPIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARTServiceType_CPIdentifier_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_CPIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARTServiceType_CPIdentifier_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_CPIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARTServiceType_CPIdentifier_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_CPIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARTServiceType_CPIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_CPIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARTServiceType_CPIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_CPIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARTServiceType_CPIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_CPIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARTServiceType_CPIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_CPIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARTServiceType_CPIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_MethodSet_Disconnect_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_MethodSet_Disconnect_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARTServiceType_MethodSet_Disconnect_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_MethodSet_Disconnect_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_MethodSet_Disconnect_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARTServiceType_MethodSet_Disconnect_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_MethodSet_Connect_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_MethodSet_Connect_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARTServiceType_MethodSet_Connect_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_MethodSet_Connect_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_MethodSet_Connect_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARTServiceType_MethodSet_Connect_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_MethodSet_Transfer_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_MethodSet_Transfer_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARTServiceType_MethodSet_Transfer_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_MethodSet_Transfer_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_MethodSet_Transfer_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARTServiceType_MethodSet_Transfer_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_MethodSet_GetPublishedData_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_MethodSet_GetPublishedData_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARTServiceType_MethodSet_GetPublishedData_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationHARTServiceType_MethodSet_GetPublishedData_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationHARTServiceType_MethodSet_GetPublishedData_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationHARTServiceType_MethodSet_GetPublishedData_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessServiceType_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessServiceType_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessServiceType_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessServiceType_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessServiceType_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessServiceType_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessServiceType_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessServiceType_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessServiceType_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessServiceType_CPIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_MethodSet_Disconnect_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_MethodSet_Disconnect_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessServiceType_MethodSet_Disconnect_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_MethodSet_Disconnect_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_MethodSet_Disconnect_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessServiceType_MethodSet_Disconnect_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_MethodSet_Connect_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_MethodSet_Connect_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessServiceType_MethodSet_Connect_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_MethodSet_Connect_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_MethodSet_Connect_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessServiceType_MethodSet_Connect_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_MethodSet_Transfer_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_MethodSet_Transfer_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessServiceType_MethodSet_Transfer_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_MethodSet_Transfer_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_MethodSet_Transfer_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessServiceType_MethodSet_Transfer_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_MethodSet_GetPublishedData_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_MethodSet_GetPublishedData_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessServiceType_MethodSet_GetPublishedData_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationISA100_WirelessServiceType_MethodSet_GetPublishedData_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationISA100_WirelessServiceType_MethodSet_GetPublishedData_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationISA100_WirelessServiceType_MethodSet_GetPublishedData_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICServiceType_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICServiceType_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICServiceType_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICServiceType_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICServiceType_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICServiceType_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICServiceType_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICServiceType_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICServiceType_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_CPIdentifier_Lock_Locked Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_CPIdentifier_Lock_Locked = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICServiceType_CPIdentifier_Lock_Locked, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_CPIdentifier_Lock_LockingClient Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_CPIdentifier_Lock_LockingClient = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICServiceType_CPIdentifier_Lock_LockingClient, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_CPIdentifier_Lock_LockingUser Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_CPIdentifier_Lock_LockingUser = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICServiceType_CPIdentifier_Lock_LockingUser, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_CPIdentifier_Lock_RemainingLockTime Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_CPIdentifier_Lock_RemainingLockTime = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICServiceType_CPIdentifier_Lock_RemainingLockTime, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_CPIdentifier_Lock_InitLock_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_CPIdentifier_Lock_InitLock_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICServiceType_CPIdentifier_Lock_InitLock_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_CPIdentifier_Lock_InitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_CPIdentifier_Lock_InitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICServiceType_CPIdentifier_Lock_InitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_CPIdentifier_Lock_RenewLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_CPIdentifier_Lock_RenewLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICServiceType_CPIdentifier_Lock_RenewLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_CPIdentifier_Lock_ExitLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_CPIdentifier_Lock_ExitLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICServiceType_CPIdentifier_Lock_ExitLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_CPIdentifier_Lock_BreakLock_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_CPIdentifier_Lock_BreakLock_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICServiceType_CPIdentifier_Lock_BreakLock_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_MethodSet_Disconnect_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_MethodSet_Disconnect_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICServiceType_MethodSet_Disconnect_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_MethodSet_Disconnect_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_MethodSet_Disconnect_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICServiceType_MethodSet_Disconnect_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_MethodSet_Connect_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_MethodSet_Connect_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICServiceType_MethodSet_Connect_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_MethodSet_Connect_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_MethodSet_Connect_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICServiceType_MethodSet_Connect_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_MethodSet_Transfer_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_MethodSet_Transfer_InputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICServiceType_MethodSet_Transfer_InputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the ServerCommunicationGENERICServiceType_MethodSet_Transfer_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId ServerCommunicationGENERICServiceType_MethodSet_Transfer_OutputArguments = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.ServerCommunicationGENERICServiceType_MethodSet_Transfer_OutputArguments, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7_BinarySchema Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7_BinarySchema = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7_BinarySchema, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7_BinarySchema_NamespaceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7_BinarySchema_NamespaceUri = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7_BinarySchema_NamespaceUri, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7_BinarySchema_Deprecated Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7_BinarySchema_Deprecated = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7_BinarySchema_Deprecated, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7_BinarySchema_EddDataTypeInfo Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7_BinarySchema_EddDataTypeInfo = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7_BinarySchema_EddDataTypeInfo, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7_XmlSchema Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7_XmlSchema = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7_XmlSchema, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7_XmlSchema_NamespaceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7_XmlSchema_NamespaceUri = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7_XmlSchema_NamespaceUri, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7_XmlSchema_Deprecated Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7_XmlSchema_Deprecated = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7_XmlSchema_Deprecated, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);

        /// <summary>
        /// The identifier for the OpcUaFdi7_XmlSchema_EddDataTypeInfo Variable.
        /// </summary>
        public static readonly ExpandedNodeId OpcUaFdi7_XmlSchema_EddDataTypeInfo = new ExpandedNodeId(Opc.Ua.Fdi7.Variables.OpcUaFdi7_XmlSchema_EddDataTypeInfo, Opc.Ua.Fdi7.Namespaces.OpcUaFdi7);
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
        /// The BrowseName for the Address component.
        /// </summary>
        public const string Address = "Address";

        /// <summary>
        /// The BrowseName for the CommunicationServerType component.
        /// </summary>
        public const string CommunicationServerType = "CommunicationServerType";

        /// <summary>
        /// The BrowseName for the ConnectionPoint_Foundation_H1 component.
        /// </summary>
        public const string ConnectionPoint_Foundation_H1 = "ConnectionPoint_Foundation_H1";

        /// <summary>
        /// The BrowseName for the ConnectionPoint_Foundation_HSE component.
        /// </summary>
        public const string ConnectionPoint_Foundation_HSE = "ConnectionPoint_Foundation_HSE";

        /// <summary>
        /// The BrowseName for the ConnectionPoint_HART_TP5 component.
        /// </summary>
        public const string ConnectionPoint_HART_TP5 = "ConnectionPoint_HART_TP5";

        /// <summary>
        /// The BrowseName for the ConnectionPoint_HART_TP6 component.
        /// </summary>
        public const string ConnectionPoint_HART_TP6 = "ConnectionPoint_HART_TP6";

        /// <summary>
        /// The BrowseName for the ConnectionPoint_HART_TP7 component.
        /// </summary>
        public const string ConnectionPoint_HART_TP7 = "ConnectionPoint_HART_TP7";

        /// <summary>
        /// The BrowseName for the ConnectionPoint_ISA100_Wireless component.
        /// </summary>
        public const string ConnectionPoint_ISA100_Wireless = "ConnectionPoint_ISA100_Wireless";

        /// <summary>
        /// The BrowseName for the ConnectionPoint_Profibus_DP component.
        /// </summary>
        public const string ConnectionPoint_Profibus_DP = "ConnectionPoint_Profibus_DP";

        /// <summary>
        /// The BrowseName for the ConnectionPoint_Profinet_IO component.
        /// </summary>
        public const string ConnectionPoint_Profinet_IO = "ConnectionPoint_Profinet_IO";

        /// <summary>
        /// The BrowseName for the ConnectMethodFFH1Type component.
        /// </summary>
        public const string ConnectMethodFFH1Type = "ConnectMethodFFH1Type";

        /// <summary>
        /// The BrowseName for the ConnectMethodFFHSEType component.
        /// </summary>
        public const string ConnectMethodFFHSEType = "ConnectMethodFFHSEType";

        /// <summary>
        /// The BrowseName for the ConnectMethodGENERICType component.
        /// </summary>
        public const string ConnectMethodGENERICType = "ConnectMethodGENERICType";

        /// <summary>
        /// The BrowseName for the ConnectMethodHARTType component.
        /// </summary>
        public const string ConnectMethodHARTType = "ConnectMethodHARTType";

        /// <summary>
        /// The BrowseName for the ConnectMethodISA100_WirelessType component.
        /// </summary>
        public const string ConnectMethodISA100_WirelessType = "ConnectMethodISA100_WirelessType";

        /// <summary>
        /// The BrowseName for the ConnectMethodPROFIBUSType component.
        /// </summary>
        public const string ConnectMethodPROFIBUSType = "ConnectMethodPROFIBUSType";

        /// <summary>
        /// The BrowseName for the ConnectMethodPROFINETType component.
        /// </summary>
        public const string ConnectMethodPROFINETType = "ConnectMethodPROFINETType";

        /// <summary>
        /// The BrowseName for the DevAddr component.
        /// </summary>
        public const string DevAddr = "DevAddr";

        /// <summary>
        /// The BrowseName for the DevMfg component.
        /// </summary>
        public const string DevMfg = "DevMfg";

        /// <summary>
        /// The BrowseName for the DevPollAddr component.
        /// </summary>
        public const string DevPollAddr = "DevPollAddr";

        /// <summary>
        /// The BrowseName for the DevRev component.
        /// </summary>
        public const string DevRev = "DevRev";

        /// <summary>
        /// The BrowseName for the DevTag component.
        /// </summary>
        public const string DevTag = "DevTag";

        /// <summary>
        /// The BrowseName for the DevType component.
        /// </summary>
        public const string DevType = "DevType";

        /// <summary>
        /// The BrowseName for the DNSNAME component.
        /// </summary>
        public const string DNSNAME = "DNSNAME";

        /// <summary>
        /// The BrowseName for the EddDataTypeEnum component.
        /// </summary>
        public const string EddDataTypeEnum = "EddDataTypeEnum";

        /// <summary>
        /// The BrowseName for the EddDataTypeInfo component.
        /// </summary>
        public const string EddDataTypeInfo = "EddDataTypeInfo";

        /// <summary>
        /// The BrowseName for the Foundation_H1 component.
        /// </summary>
        public const string Foundation_H1 = "Foundation_H1";

        /// <summary>
        /// The BrowseName for the Foundation_HSE component.
        /// </summary>
        public const string Foundation_HSE = "Foundation_HSE";

        /// <summary>
        /// The BrowseName for the GenericConnectionPoint component.
        /// </summary>
        public const string GenericConnectionPoint = "GenericConnectionPoint";

        /// <summary>
        /// The BrowseName for the GenericProtocol component.
        /// </summary>
        public const string GenericProtocol = "GenericProtocol";

        /// <summary>
        /// The BrowseName for the GetPublishedDataMethodFFH1Type component.
        /// </summary>
        public const string GetPublishedDataMethodFFH1Type = "GetPublishedDataMethodFFH1Type";

        /// <summary>
        /// The BrowseName for the GetPublishedDataMethodFFHSEType component.
        /// </summary>
        public const string GetPublishedDataMethodFFHSEType = "GetPublishedDataMethodFFHSEType";

        /// <summary>
        /// The BrowseName for the GetPublishedDataMethodHARTType component.
        /// </summary>
        public const string GetPublishedDataMethodHARTType = "GetPublishedDataMethodHARTType";

        /// <summary>
        /// The BrowseName for the GetPublishedDataMethodISA100_WirelessType component.
        /// </summary>
        public const string GetPublishedDataMethodISA100_WirelessType = "GetPublishedDataMethodISA100_WirelessType";

        /// <summary>
        /// The BrowseName for the HART component.
        /// </summary>
        public const string HART = "HART";

        /// <summary>
        /// The BrowseName for the IPAddress component.
        /// </summary>
        public const string IPAddress = "IPAddress";

        /// <summary>
        /// The BrowseName for the IPv4 component.
        /// </summary>
        public const string IPv4 = "IPv4";

        /// <summary>
        /// The BrowseName for the ISA100_Wireless component.
        /// </summary>
        public const string ISA100_Wireless = "ISA100_Wireless";

        /// <summary>
        /// The BrowseName for the ListOfCommunicationProfiles component.
        /// </summary>
        public const string ListOfCommunicationProfiles = "ListOfCommunicationProfiles";

        /// <summary>
        /// The BrowseName for the MAC component.
        /// </summary>
        public const string MAC = "MAC";

        /// <summary>
        /// The BrowseName for the OpcUaFdi7_BinarySchema component.
        /// </summary>
        public const string OpcUaFdi7_BinarySchema = "Opc.Ua.Fdi7";

        /// <summary>
        /// The BrowseName for the OpcUaFdi7_XmlSchema component.
        /// </summary>
        public const string OpcUaFdi7_XmlSchema = "Opc.Ua.Fdi7";

        /// <summary>
        /// The BrowseName for the OpcUaFdi7NamespaceMetadata component.
        /// </summary>
        public const string OpcUaFdi7NamespaceMetadata = "http://fdi-cooperation.com/OPCUA/FDI7/";

        /// <summary>
        /// The BrowseName for the OrdinalNumber component.
        /// </summary>
        public const string OrdinalNumber = "OrdinalNumber";

        /// <summary>
        /// The BrowseName for the Profibus_DP component.
        /// </summary>
        public const string Profibus_DP = "Profibus_DP";

        /// <summary>
        /// The BrowseName for the Profibus_PA component.
        /// </summary>
        public const string Profibus_PA = "Profibus_PA";

        /// <summary>
        /// The BrowseName for the Profinet_IO component.
        /// </summary>
        public const string Profinet_IO = "Profinet_IO";

        /// <summary>
        /// The BrowseName for the ProtocolIdentifier component.
        /// </summary>
        public const string ProtocolIdentifier = "ProtocolIdentifier";

        /// <summary>
        /// The BrowseName for the ServerCommunicationDeviceType component.
        /// </summary>
        public const string ServerCommunicationDeviceType = "ServerCommunicationDeviceType";

        /// <summary>
        /// The BrowseName for the ServerCommunicationFFH1DeviceType component.
        /// </summary>
        public const string ServerCommunicationFFH1DeviceType = "ServerCommunicationFFH1DeviceType";

        /// <summary>
        /// The BrowseName for the ServerCommunicationFFH1ServiceType component.
        /// </summary>
        public const string ServerCommunicationFFH1ServiceType = "ServerCommunicationFFH1ServiceType";

        /// <summary>
        /// The BrowseName for the ServerCommunicationFFHSEDeviceType component.
        /// </summary>
        public const string ServerCommunicationFFHSEDeviceType = "ServerCommunicationFFHSEDeviceType";

        /// <summary>
        /// The BrowseName for the ServerCommunicationFFHSEServiceType component.
        /// </summary>
        public const string ServerCommunicationFFHSEServiceType = "ServerCommunicationFFHSEServiceType";

        /// <summary>
        /// The BrowseName for the ServerCommunicationGENERICDeviceType component.
        /// </summary>
        public const string ServerCommunicationGENERICDeviceType = "ServerCommunicationGENERICDeviceType";

        /// <summary>
        /// The BrowseName for the ServerCommunicationGENERICServiceType component.
        /// </summary>
        public const string ServerCommunicationGENERICServiceType = "ServerCommunicationGENERICServiceType";

        /// <summary>
        /// The BrowseName for the ServerCommunicationHARTServiceType component.
        /// </summary>
        public const string ServerCommunicationHARTServiceType = "ServerCommunicationHARTServiceType";

        /// <summary>
        /// The BrowseName for the ServerCommunicationHARType component.
        /// </summary>
        public const string ServerCommunicationHARType = "ServerCommunicationHARType";

        /// <summary>
        /// The BrowseName for the ServerCommunicationISA100_WirelessDeviceType component.
        /// </summary>
        public const string ServerCommunicationISA100_WirelessDeviceType = "ServerCommunicationISA100_WirelessDeviceType";

        /// <summary>
        /// The BrowseName for the ServerCommunicationISA100_WirelessServiceType component.
        /// </summary>
        public const string ServerCommunicationISA100_WirelessServiceType = "ServerCommunicationISA100_WirelessServiceType";

        /// <summary>
        /// The BrowseName for the ServerCommunicationPROFIBUSDeviceType component.
        /// </summary>
        public const string ServerCommunicationPROFIBUSDeviceType = "ServerCommunicationPROFIBUSDeviceType";

        /// <summary>
        /// The BrowseName for the ServerCommunicationPROFIBUSServiceType component.
        /// </summary>
        public const string ServerCommunicationPROFIBUSServiceType = "ServerCommunicationPROFIBUSServiceType";

        /// <summary>
        /// The BrowseName for the ServerCommunicationPROFINETDeviceType component.
        /// </summary>
        public const string ServerCommunicationPROFINETDeviceType = "ServerCommunicationPROFINETDeviceType";

        /// <summary>
        /// The BrowseName for the ServerCommunicationPROFINETServiceType component.
        /// </summary>
        public const string ServerCommunicationPROFINETServiceType = "ServerCommunicationPROFINETServiceType";

        /// <summary>
        /// The BrowseName for the ServerCommunicationServiceType component.
        /// </summary>
        public const string ServerCommunicationServiceType = "ServerCommunicationServiceType";

        /// <summary>
        /// The BrowseName for the ServiceProvider component.
        /// </summary>
        public const string ServiceProvider = "ServiceProvider";

        /// <summary>
        /// The BrowseName for the SetAddressMethodFFH1Type component.
        /// </summary>
        public const string SetAddressMethodFFH1Type = "SetAddressMethodFFH1Type";

        /// <summary>
        /// The BrowseName for the SetAddressMethodFFHSEType component.
        /// </summary>
        public const string SetAddressMethodFFHSEType = "SetAddressMethodFFHSEType";

        /// <summary>
        /// The BrowseName for the SetAddressMethodGENERICType component.
        /// </summary>
        public const string SetAddressMethodGENERICType = "SetAddressMethodGENERICType";

        /// <summary>
        /// The BrowseName for the SetAddressMethodHARTType component.
        /// </summary>
        public const string SetAddressMethodHARTType = "SetAddressMethodHARTType";

        /// <summary>
        /// The BrowseName for the SetAddressMethodPROFIBUSType component.
        /// </summary>
        public const string SetAddressMethodPROFIBUSType = "SetAddressMethodPROFIBUSType";

        /// <summary>
        /// The BrowseName for the SetAddressMethodPROFINETType component.
        /// </summary>
        public const string SetAddressMethodPROFINETType = "SetAddressMethodPROFINETType";

        /// <summary>
        /// The BrowseName for the SIFConnection component.
        /// </summary>
        public const string SIFConnection = "SIFConnection";

        /// <summary>
        /// The BrowseName for the SubDevices component.
        /// </summary>
        public const string SubDevices = "SubDevices";

        /// <summary>
        /// The BrowseName for the TransferMethodFFH1Type component.
        /// </summary>
        public const string TransferMethodFFH1Type = "TransferMethodFFH1Type";

        /// <summary>
        /// The BrowseName for the TransferMethodFFHSEType component.
        /// </summary>
        public const string TransferMethodFFHSEType = "TransferMethodFFHSEType";

        /// <summary>
        /// The BrowseName for the TransferMethodGENERICType component.
        /// </summary>
        public const string TransferMethodGENERICType = "TransferMethodGENERICType";

        /// <summary>
        /// The BrowseName for the TransferMethodHARTType component.
        /// </summary>
        public const string TransferMethodHARTType = "TransferMethodHARTType";

        /// <summary>
        /// The BrowseName for the TransferMethodISA100_WirelessType component.
        /// </summary>
        public const string TransferMethodISA100_WirelessType = "TransferMethodISA100_WirelessType";

        /// <summary>
        /// The BrowseName for the TransferMethodPROFIBUSType component.
        /// </summary>
        public const string TransferMethodPROFIBUSType = "TransferMethodPROFIBUSType";

        /// <summary>
        /// The BrowseName for the TransferMethodPROFINETType component.
        /// </summary>
        public const string TransferMethodPROFINETType = "TransferMethodPROFINETType";

        /// <summary>
        /// The BrowseName for the VALID component.
        /// </summary>
        public const string VALID = "VALID";
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
        /// The URI for the OpcUaFdi7 namespace (.NET code namespace is 'Opc.Ua.Fdi7').
        /// </summary>
        public const string OpcUaFdi7 = "http://fdi-cooperation.com/OPCUA/FDI7/";

        /// <summary>
        /// The URI for the OpcUaFdi7Xsd namespace (.NET code namespace is 'Opc.Ua.Fdi7').
        /// </summary>
        public const string OpcUaFdi7Xsd = "http://fdi-cooperation.com/OPCUA/FDI7/Types.xsd";

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
