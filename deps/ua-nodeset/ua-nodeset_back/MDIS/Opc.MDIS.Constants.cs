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

namespace Opc.MDIS
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
        /// The identifier for the SignatureStatusEnum DataType.
        /// </summary>
        public const uint SignatureStatusEnum = 699;

        /// <summary>
        /// The identifier for the SetCalculatedPositionEnum DataType.
        /// </summary>
        public const uint SetCalculatedPositionEnum = 1287;

        /// <summary>
        /// The identifier for the CommandEnum DataType.
        /// </summary>
        public const uint CommandEnum = 3;

        /// <summary>
        /// The identifier for the ChokeCommandEnum DataType.
        /// </summary>
        public const uint ChokeCommandEnum = 701;

        /// <summary>
        /// The identifier for the SEMEnum DataType.
        /// </summary>
        public const uint SEMEnum = 5;

        /// <summary>
        /// The identifier for the ValvePositionEnum DataType.
        /// </summary>
        public const uint ValvePositionEnum = 703;

        /// <summary>
        /// The identifier for the ChokeMoveEnum DataType.
        /// </summary>
        public const uint ChokeMoveEnum = 602;

        /// <summary>
        /// The identifier for the MDISVersionDataType DataType.
        /// </summary>
        public const uint MDISVersionDataType = 1289;
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
        /// The identifier for the MDISBaseObjectType_EnableDisable Method.
        /// </summary>
        public const uint MDISBaseObjectType_EnableDisable = 195;

        /// <summary>
        /// The identifier for the MDISValveObjectType_Move Method.
        /// </summary>
        public const uint MDISValveObjectType_Move = 883;

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_Open Method.
        /// </summary>
        public const uint MDISValveObjectType_ValveSignature_Open = 1299;

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_Close Method.
        /// </summary>
        public const uint MDISValveObjectType_ValveSignature_Close = 1302;

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_Read Method.
        /// </summary>
        public const uint MDISValveObjectType_ValveSignature_Read = 1304;

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_Write Method.
        /// </summary>
        public const uint MDISValveObjectType_ValveSignature_Write = 1307;

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_GetPosition Method.
        /// </summary>
        public const uint MDISValveObjectType_ValveSignature_GetPosition = 1309;

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_SetPosition Method.
        /// </summary>
        public const uint MDISValveObjectType_ValveSignature_SetPosition = 1312;

        /// <summary>
        /// The identifier for the MDISDigitalOutObjectType_WriteState Method.
        /// </summary>
        public const uint MDISDigitalOutObjectType_WriteState = 1240;

        /// <summary>
        /// The identifier for the MDISDiscreteOutObjectType_WriteValue Method.
        /// </summary>
        public const uint MDISDiscreteOutObjectType_WriteValue = 1252;

        /// <summary>
        /// The identifier for the MDISInstrumentOutObjectType_WriteValue Method.
        /// </summary>
        public const uint MDISInstrumentOutObjectType_WriteValue = 1277;

        /// <summary>
        /// The identifier for the MDISChokeObjectType_Move Method.
        /// </summary>
        public const uint MDISChokeObjectType_Move = 1155;

        /// <summary>
        /// The identifier for the MDISChokeObjectType_Step Method.
        /// </summary>
        public const uint MDISChokeObjectType_Step = 1157;

        /// <summary>
        /// The identifier for the MDISChokeObjectType_Abort Method.
        /// </summary>
        public const uint MDISChokeObjectType_Abort = 1159;

        /// <summary>
        /// The identifier for the MDISChokeObjectType_SetCalculatedPosition Method.
        /// </summary>
        public const uint MDISChokeObjectType_SetCalculatedPosition = 1284;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_InstrumentOutPlaceholder_WriteValue Method.
        /// </summary>
        public const uint MDISAggregateObjectType_InstrumentOutPlaceholder_WriteValue = 1390;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DigitalOutPlaceholder_WriteState Method.
        /// </summary>
        public const uint MDISAggregateObjectType_DigitalOutPlaceholder_WriteState = 1402;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DiscreteOutPlaceholder_WriteValue Method.
        /// </summary>
        public const uint MDISAggregateObjectType_DiscreteOutPlaceholder_WriteValue = 1414;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ValvePlaceholder_Move Method.
        /// </summary>
        public const uint MDISAggregateObjectType_ValvePlaceholder_Move = 1433;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ChokePlaceholder_Move Method.
        /// </summary>
        public const uint MDISAggregateObjectType_ChokePlaceholder_Move = 1455;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ChokePlaceholder_Abort Method.
        /// </summary>
        public const uint MDISAggregateObjectType_ChokePlaceholder_Abort = 1459;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ChokePlaceholder_SetCalculatedPosition Method.
        /// </summary>
        public const uint MDISAggregateObjectType_ChokePlaceholder_SetCalculatedPosition = 1460;

        /// <summary>
        /// The identifier for the MDISTimeSyncObjectType_SetTime Method.
        /// </summary>
        public const uint MDISTimeSyncObjectType_SetTime = 1469;

        /// <summary>
        /// The identifier for the MDISInformationObjectType_TimeSynchronization_SetTime Method.
        /// </summary>
        public const uint MDISInformationObjectType_TimeSynchronization_SetTime = 1473;

        /// <summary>
        /// The identifier for the MDISInformation_TimeSynchronization_SetTime Method.
        /// </summary>
        public const uint MDISInformation_TimeSynchronization_SetTime = 15046;
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
        /// The identifier for the MDISValveObjectType_ValveSignature Object.
        /// </summary>
        public const uint MDISValveObjectType_ValveSignature = 1294;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_InstrumentPlaceholder Object.
        /// </summary>
        public const uint MDISAggregateObjectType_InstrumentPlaceholder = 1324;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_InstrumentOutPlaceholder Object.
        /// </summary>
        public const uint MDISAggregateObjectType_InstrumentOutPlaceholder = 1367;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DigitalInstrumentPlaceholder Object.
        /// </summary>
        public const uint MDISAggregateObjectType_DigitalInstrumentPlaceholder = 15022;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DiscreteInstrumentPlaceholder Object.
        /// </summary>
        public const uint MDISAggregateObjectType_DiscreteInstrumentPlaceholder = 15032;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DigitalOutPlaceholder Object.
        /// </summary>
        public const uint MDISAggregateObjectType_DigitalOutPlaceholder = 1392;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DiscreteOutPlaceholder Object.
        /// </summary>
        public const uint MDISAggregateObjectType_DiscreteOutPlaceholder = 1404;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ValvePlaceholder Object.
        /// </summary>
        public const uint MDISAggregateObjectType_ValvePlaceholder = 1416;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ChokePlaceholder Object.
        /// </summary>
        public const uint MDISAggregateObjectType_ChokePlaceholder = 1437;

        /// <summary>
        /// The identifier for the MDISInformationObjectType_TimeSynchronization Object.
        /// </summary>
        public const uint MDISInformationObjectType_TimeSynchronization = 1472;

        /// <summary>
        /// The identifier for the MDISInformationObjectType_Signatures Object.
        /// </summary>
        public const uint MDISInformationObjectType_Signatures = 1475;

        /// <summary>
        /// The identifier for the MDISInformation Object.
        /// </summary>
        public const uint MDISInformation = 15044;

        /// <summary>
        /// The identifier for the MDISInformation_TimeSynchronization Object.
        /// </summary>
        public const uint MDISInformation_TimeSynchronization = 15045;

        /// <summary>
        /// The identifier for the MDISInformation_Signatures Object.
        /// </summary>
        public const uint MDISInformation_Signatures = 15048;

        /// <summary>
        /// The identifier for the MDISVersionDataType_Encoding_DefaultBinary Object.
        /// </summary>
        public const uint MDISVersionDataType_Encoding_DefaultBinary = 1484;

        /// <summary>
        /// The identifier for the MDISVersionDataType_Encoding_DefaultXml Object.
        /// </summary>
        public const uint MDISVersionDataType_Encoding_DefaultXml = 1480;

        /// <summary>
        /// The identifier for the MDISVersionDataType_Encoding_DefaultJson Object.
        /// </summary>
        public const uint MDISVersionDataType_Encoding_DefaultJson = 15004;
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
        /// The identifier for the MDISBaseObjectType ObjectType.
        /// </summary>
        public const uint MDISBaseObjectType = 194;

        /// <summary>
        /// The identifier for the MDISValveObjectType ObjectType.
        /// </summary>
        public const uint MDISValveObjectType = 794;

        /// <summary>
        /// The identifier for the MDISDigitalInstrumentObjectType ObjectType.
        /// </summary>
        public const uint MDISDigitalInstrumentObjectType = 889;

        /// <summary>
        /// The identifier for the MDISDigitalOutObjectType ObjectType.
        /// </summary>
        public const uint MDISDigitalOutObjectType = 1230;

        /// <summary>
        /// The identifier for the MDISDiscreteInstrumentObjectType ObjectType.
        /// </summary>
        public const uint MDISDiscreteInstrumentObjectType = 1214;

        /// <summary>
        /// The identifier for the MDISDiscreteOutObjectType ObjectType.
        /// </summary>
        public const uint MDISDiscreteOutObjectType = 1242;

        /// <summary>
        /// The identifier for the MDISInstrumentObjectType ObjectType.
        /// </summary>
        public const uint MDISInstrumentObjectType = 971;

        /// <summary>
        /// The identifier for the MDISInstrumentOutObjectType ObjectType.
        /// </summary>
        public const uint MDISInstrumentOutObjectType = 1254;

        /// <summary>
        /// The identifier for the MDISChokeObjectType ObjectType.
        /// </summary>
        public const uint MDISChokeObjectType = 1066;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType ObjectType.
        /// </summary>
        public const uint MDISAggregateObjectType = 1315;

        /// <summary>
        /// The identifier for the MDISTimeSyncObjectType ObjectType.
        /// </summary>
        public const uint MDISTimeSyncObjectType = 1468;

        /// <summary>
        /// The identifier for the MDISInformationObjectType ObjectType.
        /// </summary>
        public const uint MDISInformationObjectType = 1471;
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
        /// The identifier for the HasInterlock ReferenceType.
        /// </summary>
        public const uint HasInterlock = 1183;

        /// <summary>
        /// The identifier for the HasSignature ReferenceType.
        /// </summary>
        public const uint HasSignature = 1286;

        /// <summary>
        /// The identifier for the InterlockFor ReferenceType.
        /// </summary>
        public const uint InterlockFor = 1184;
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
        /// The identifier for the MDISBaseObjectType_Fault Variable.
        /// </summary>
        public const uint MDISBaseObjectType_Fault = 489;

        /// <summary>
        /// The identifier for the MDISBaseObjectType_FaultCode Variable.
        /// </summary>
        public const uint MDISBaseObjectType_FaultCode = 1165;

        /// <summary>
        /// The identifier for the MDISBaseObjectType_Warning Variable.
        /// </summary>
        public const uint MDISBaseObjectType_Warning = 497;

        /// <summary>
        /// The identifier for the MDISBaseObjectType_WarningCode Variable.
        /// </summary>
        public const uint MDISBaseObjectType_WarningCode = 1166;

        /// <summary>
        /// The identifier for the MDISBaseObjectType_Enabled Variable.
        /// </summary>
        public const uint MDISBaseObjectType_Enabled = 476;

        /// <summary>
        /// The identifier for the MDISBaseObjectType_EnableDisable_InputArguments Variable.
        /// </summary>
        public const uint MDISBaseObjectType_EnableDisable_InputArguments = 196;

        /// <summary>
        /// The identifier for the MDISBaseObjectType_TagId Variable.
        /// </summary>
        public const uint MDISBaseObjectType_TagId = 197;

        /// <summary>
        /// The identifier for the SignatureStatusEnum_EnumValues Variable.
        /// </summary>
        public const uint SignatureStatusEnum_EnumValues = 700;

        /// <summary>
        /// The identifier for the SetCalculatedPositionEnum_EnumValues Variable.
        /// </summary>
        public const uint SetCalculatedPositionEnum_EnumValues = 1288;

        /// <summary>
        /// The identifier for the CommandEnum_EnumValues Variable.
        /// </summary>
        public const uint CommandEnum_EnumValues = 616;

        /// <summary>
        /// The identifier for the ChokeCommandEnum_EnumValues Variable.
        /// </summary>
        public const uint ChokeCommandEnum_EnumValues = 702;

        /// <summary>
        /// The identifier for the SEMEnum_EnumValues Variable.
        /// </summary>
        public const uint SEMEnum_EnumValues = 6;

        /// <summary>
        /// The identifier for the ValvePositionEnum_EnumValues Variable.
        /// </summary>
        public const uint ValvePositionEnum_EnumValues = 704;

        /// <summary>
        /// The identifier for the ChokeMoveEnum_EnumValues Variable.
        /// </summary>
        public const uint ChokeMoveEnum_EnumValues = 603;

        /// <summary>
        /// The identifier for the MDISVersionVariableType_MajorVersion Variable.
        /// </summary>
        public const uint MDISVersionVariableType_MajorVersion = 1291;

        /// <summary>
        /// The identifier for the MDISVersionVariableType_MinorVersion Variable.
        /// </summary>
        public const uint MDISVersionVariableType_MinorVersion = 1292;

        /// <summary>
        /// The identifier for the MDISVersionVariableType_Build Variable.
        /// </summary>
        public const uint MDISVersionVariableType_Build = 1293;

        /// <summary>
        /// The identifier for the MDISValveObjectType_EnableDisable_InputArguments Variable.
        /// </summary>
        public const uint MDISValveObjectType_EnableDisable_InputArguments = 799;

        /// <summary>
        /// The identifier for the MDISValveObjectType_Position Variable.
        /// </summary>
        public const uint MDISValveObjectType_Position = 875;

        /// <summary>
        /// The identifier for the MDISValveObjectType_CommandRejected Variable.
        /// </summary>
        public const uint MDISValveObjectType_CommandRejected = 876;

        /// <summary>
        /// The identifier for the MDISValveObjectType_SignatureRequestStatus Variable.
        /// </summary>
        public const uint MDISValveObjectType_SignatureRequestStatus = 877;

        /// <summary>
        /// The identifier for the MDISValveObjectType_LastCommand Variable.
        /// </summary>
        public const uint MDISValveObjectType_LastCommand = 878;

        /// <summary>
        /// The identifier for the MDISValveObjectType_NonDefeatableOpenInterlock Variable.
        /// </summary>
        public const uint MDISValveObjectType_NonDefeatableOpenInterlock = 879;

        /// <summary>
        /// The identifier for the MDISValveObjectType_DefeatableOpenInterlock Variable.
        /// </summary>
        public const uint MDISValveObjectType_DefeatableOpenInterlock = 880;

        /// <summary>
        /// The identifier for the MDISValveObjectType_NonDefeatableCloseInterlock Variable.
        /// </summary>
        public const uint MDISValveObjectType_NonDefeatableCloseInterlock = 881;

        /// <summary>
        /// The identifier for the MDISValveObjectType_DefeatableCloseInterlock Variable.
        /// </summary>
        public const uint MDISValveObjectType_DefeatableCloseInterlock = 882;

        /// <summary>
        /// The identifier for the MDISValveObjectType_Move_InputArguments Variable.
        /// </summary>
        public const uint MDISValveObjectType_Move_InputArguments = 884;

        /// <summary>
        /// The identifier for the MDISValveObjectType_InterlockPlaceholder Variable.
        /// </summary>
        public const uint MDISValveObjectType_InterlockPlaceholder = 1280;

        /// <summary>
        /// The identifier for the MDISValveObjectType_OpenTimeDuration Variable.
        /// </summary>
        public const uint MDISValveObjectType_OpenTimeDuration = 887;

        /// <summary>
        /// The identifier for the MDISValveObjectType_CloseTimeDuration Variable.
        /// </summary>
        public const uint MDISValveObjectType_CloseTimeDuration = 888;

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_Size Variable.
        /// </summary>
        public const uint MDISValveObjectType_ValveSignature_Size = 1295;

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_Writable Variable.
        /// </summary>
        public const uint MDISValveObjectType_ValveSignature_Writable = 1296;

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_UserWritable Variable.
        /// </summary>
        public const uint MDISValveObjectType_ValveSignature_UserWritable = 1297;

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_OpenCount Variable.
        /// </summary>
        public const uint MDISValveObjectType_ValveSignature_OpenCount = 1298;

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_Open_InputArguments Variable.
        /// </summary>
        public const uint MDISValveObjectType_ValveSignature_Open_InputArguments = 1300;

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_Open_OutputArguments Variable.
        /// </summary>
        public const uint MDISValveObjectType_ValveSignature_Open_OutputArguments = 1301;

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_Close_InputArguments Variable.
        /// </summary>
        public const uint MDISValveObjectType_ValveSignature_Close_InputArguments = 1303;

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_Read_InputArguments Variable.
        /// </summary>
        public const uint MDISValveObjectType_ValveSignature_Read_InputArguments = 1305;

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_Read_OutputArguments Variable.
        /// </summary>
        public const uint MDISValveObjectType_ValveSignature_Read_OutputArguments = 1306;

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_Write_InputArguments Variable.
        /// </summary>
        public const uint MDISValveObjectType_ValveSignature_Write_InputArguments = 1308;

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_GetPosition_InputArguments Variable.
        /// </summary>
        public const uint MDISValveObjectType_ValveSignature_GetPosition_InputArguments = 1310;

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_GetPosition_OutputArguments Variable.
        /// </summary>
        public const uint MDISValveObjectType_ValveSignature_GetPosition_OutputArguments = 1311;

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_SetPosition_InputArguments Variable.
        /// </summary>
        public const uint MDISValveObjectType_ValveSignature_SetPosition_InputArguments = 1313;

        /// <summary>
        /// The identifier for the MDISDigitalInstrumentObjectType_EnableDisable_InputArguments Variable.
        /// </summary>
        public const uint MDISDigitalInstrumentObjectType_EnableDisable_InputArguments = 894;

        /// <summary>
        /// The identifier for the MDISDigitalInstrumentObjectType_State Variable.
        /// </summary>
        public const uint MDISDigitalInstrumentObjectType_State = 970;

        /// <summary>
        /// The identifier for the MDISDigitalOutObjectType_EnableDisable_InputArguments Variable.
        /// </summary>
        public const uint MDISDigitalOutObjectType_EnableDisable_InputArguments = 1237;

        /// <summary>
        /// The identifier for the MDISDigitalOutObjectType_WriteState_InputArguments Variable.
        /// </summary>
        public const uint MDISDigitalOutObjectType_WriteState_InputArguments = 1241;

        /// <summary>
        /// The identifier for the MDISDiscreteInstrumentObjectType_EnableDisable_InputArguments Variable.
        /// </summary>
        public const uint MDISDiscreteInstrumentObjectType_EnableDisable_InputArguments = 1221;

        /// <summary>
        /// The identifier for the MDISDiscreteInstrumentObjectType_State Variable.
        /// </summary>
        public const uint MDISDiscreteInstrumentObjectType_State = 1223;

        /// <summary>
        /// The identifier for the MDISDiscreteOutObjectType_EnableDisable_InputArguments Variable.
        /// </summary>
        public const uint MDISDiscreteOutObjectType_EnableDisable_InputArguments = 1249;

        /// <summary>
        /// The identifier for the MDISDiscreteOutObjectType_WriteValue_InputArguments Variable.
        /// </summary>
        public const uint MDISDiscreteOutObjectType_WriteValue_InputArguments = 1253;

        /// <summary>
        /// The identifier for the MDISInstrumentObjectType_EnableDisable_InputArguments Variable.
        /// </summary>
        public const uint MDISInstrumentObjectType_EnableDisable_InputArguments = 976;

        /// <summary>
        /// The identifier for the MDISInstrumentObjectType_ProcessVariable Variable.
        /// </summary>
        public const uint MDISInstrumentObjectType_ProcessVariable = 1052;

        /// <summary>
        /// The identifier for the MDISInstrumentObjectType_ProcessVariable_EURange Variable.
        /// </summary>
        public const uint MDISInstrumentObjectType_ProcessVariable_EURange = 1056;

        /// <summary>
        /// The identifier for the MDISInstrumentObjectType_ProcessVariable_EngineeringUnits Variable.
        /// </summary>
        public const uint MDISInstrumentObjectType_ProcessVariable_EngineeringUnits = 1057;

        /// <summary>
        /// The identifier for the MDISInstrumentObjectType_HHlimit Variable.
        /// </summary>
        public const uint MDISInstrumentObjectType_HHlimit = 1058;

        /// <summary>
        /// The identifier for the MDISInstrumentObjectType_Hlimit Variable.
        /// </summary>
        public const uint MDISInstrumentObjectType_Hlimit = 1059;

        /// <summary>
        /// The identifier for the MDISInstrumentObjectType_Llimit Variable.
        /// </summary>
        public const uint MDISInstrumentObjectType_Llimit = 1060;

        /// <summary>
        /// The identifier for the MDISInstrumentObjectType_LLlimit Variable.
        /// </summary>
        public const uint MDISInstrumentObjectType_LLlimit = 1061;

        /// <summary>
        /// The identifier for the MDISInstrumentObjectType_HHSetPoint Variable.
        /// </summary>
        public const uint MDISInstrumentObjectType_HHSetPoint = 1062;

        /// <summary>
        /// The identifier for the MDISInstrumentObjectType_HSetPoint Variable.
        /// </summary>
        public const uint MDISInstrumentObjectType_HSetPoint = 1063;

        /// <summary>
        /// The identifier for the MDISInstrumentObjectType_LSetPoint Variable.
        /// </summary>
        public const uint MDISInstrumentObjectType_LSetPoint = 1064;

        /// <summary>
        /// The identifier for the MDISInstrumentObjectType_LLSetPoint Variable.
        /// </summary>
        public const uint MDISInstrumentObjectType_LLSetPoint = 1065;

        /// <summary>
        /// The identifier for the MDISInstrumentOutObjectType_EnableDisable_InputArguments Variable.
        /// </summary>
        public const uint MDISInstrumentOutObjectType_EnableDisable_InputArguments = 1261;

        /// <summary>
        /// The identifier for the MDISInstrumentOutObjectType_ProcessVariable_EURange Variable.
        /// </summary>
        public const uint MDISInstrumentOutObjectType_ProcessVariable_EURange = 1267;

        /// <summary>
        /// The identifier for the MDISInstrumentOutObjectType_ProcessVariable_EngineeringUnits Variable.
        /// </summary>
        public const uint MDISInstrumentOutObjectType_ProcessVariable_EngineeringUnits = 1268;

        /// <summary>
        /// The identifier for the MDISInstrumentOutObjectType_WriteValue_InputArguments Variable.
        /// </summary>
        public const uint MDISInstrumentOutObjectType_WriteValue_InputArguments = 1278;

        /// <summary>
        /// The identifier for the MDISChokeObjectType_EnableDisable_InputArguments Variable.
        /// </summary>
        public const uint MDISChokeObjectType_EnableDisable_InputArguments = 1071;

        /// <summary>
        /// The identifier for the MDISChokeObjectType_CalculatedPosition Variable.
        /// </summary>
        public const uint MDISChokeObjectType_CalculatedPosition = 1147;

        /// <summary>
        /// The identifier for the MDISChokeObjectType_SetCalculatedPositionStatus Variable.
        /// </summary>
        public const uint MDISChokeObjectType_SetCalculatedPositionStatus = 1314;

        /// <summary>
        /// The identifier for the MDISChokeObjectType_PositionInSteps Variable.
        /// </summary>
        public const uint MDISChokeObjectType_PositionInSteps = 1148;

        /// <summary>
        /// The identifier for the MDISChokeObjectType_Moving Variable.
        /// </summary>
        public const uint MDISChokeObjectType_Moving = 1149;

        /// <summary>
        /// The identifier for the MDISChokeObjectType_CommandRejected Variable.
        /// </summary>
        public const uint MDISChokeObjectType_CommandRejected = 1150;

        /// <summary>
        /// The identifier for the MDISChokeObjectType_NonDefeatableOpenInterlock Variable.
        /// </summary>
        public const uint MDISChokeObjectType_NonDefeatableOpenInterlock = 1151;

        /// <summary>
        /// The identifier for the MDISChokeObjectType_DefeatableOpenInterlock Variable.
        /// </summary>
        public const uint MDISChokeObjectType_DefeatableOpenInterlock = 1152;

        /// <summary>
        /// The identifier for the MDISChokeObjectType_NonDefeatableCloseInterlock Variable.
        /// </summary>
        public const uint MDISChokeObjectType_NonDefeatableCloseInterlock = 1153;

        /// <summary>
        /// The identifier for the MDISChokeObjectType_DefeatableCloseInterlock Variable.
        /// </summary>
        public const uint MDISChokeObjectType_DefeatableCloseInterlock = 1154;

        /// <summary>
        /// The identifier for the MDISChokeObjectType_Move_InputArguments Variable.
        /// </summary>
        public const uint MDISChokeObjectType_Move_InputArguments = 1156;

        /// <summary>
        /// The identifier for the MDISChokeObjectType_Step_InputArguments Variable.
        /// </summary>
        public const uint MDISChokeObjectType_Step_InputArguments = 1158;

        /// <summary>
        /// The identifier for the MDISChokeObjectType_SetCalculatedPosition_InputArguments Variable.
        /// </summary>
        public const uint MDISChokeObjectType_SetCalculatedPosition_InputArguments = 1285;

        /// <summary>
        /// The identifier for the MDISChokeObjectType_InterlockPlaceholder Variable.
        /// </summary>
        public const uint MDISChokeObjectType_InterlockPlaceholder = 1281;

        /// <summary>
        /// The identifier for the MDISChokeObjectType_StepDurationOpen Variable.
        /// </summary>
        public const uint MDISChokeObjectType_StepDurationOpen = 1162;

        /// <summary>
        /// The identifier for the MDISChokeObjectType_StepDurationClose Variable.
        /// </summary>
        public const uint MDISChokeObjectType_StepDurationClose = 1163;

        /// <summary>
        /// The identifier for the MDISChokeObjectType_TotalSteps Variable.
        /// </summary>
        public const uint MDISChokeObjectType_TotalSteps = 1164;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_EnableDisable_InputArguments Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_EnableDisable_InputArguments = 1322;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_InstrumentPlaceholder_Fault Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_InstrumentPlaceholder_Fault = 1325;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_InstrumentPlaceholder_EnableDisable_InputArguments Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_InstrumentPlaceholder_EnableDisable_InputArguments = 1331;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_InstrumentPlaceholder_ProcessVariable Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_InstrumentPlaceholder_ProcessVariable = 1333;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_InstrumentPlaceholder_ProcessVariable_EURange Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_InstrumentPlaceholder_ProcessVariable_EURange = 1337;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_InstrumentPlaceholder_ProcessVariable_EngineeringUnits Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_InstrumentPlaceholder_ProcessVariable_EngineeringUnits = 1338;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_InstrumentOutPlaceholder_Fault Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_InstrumentOutPlaceholder_Fault = 1368;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_InstrumentOutPlaceholder_EnableDisable_InputArguments Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_InstrumentOutPlaceholder_EnableDisable_InputArguments = 1374;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_InstrumentOutPlaceholder_ProcessVariable Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_InstrumentOutPlaceholder_ProcessVariable = 1376;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_InstrumentOutPlaceholder_ProcessVariable_EURange Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_InstrumentOutPlaceholder_ProcessVariable_EURange = 1380;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_InstrumentOutPlaceholder_ProcessVariable_EngineeringUnits Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_InstrumentOutPlaceholder_ProcessVariable_EngineeringUnits = 1381;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_InstrumentOutPlaceholder_WriteValue_InputArguments Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_InstrumentOutPlaceholder_WriteValue_InputArguments = 1391;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DigitalInstrumentPlaceholder_Fault Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_DigitalInstrumentPlaceholder_Fault = 15023;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DigitalInstrumentPlaceholder_EnableDisable_InputArguments Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_DigitalInstrumentPlaceholder_EnableDisable_InputArguments = 15029;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DigitalInstrumentPlaceholder_State Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_DigitalInstrumentPlaceholder_State = 15031;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DiscreteInstrumentPlaceholder_Fault Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_DiscreteInstrumentPlaceholder_Fault = 15033;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DiscreteInstrumentPlaceholder_EnableDisable_InputArguments Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_DiscreteInstrumentPlaceholder_EnableDisable_InputArguments = 15039;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DiscreteInstrumentPlaceholder_State Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_DiscreteInstrumentPlaceholder_State = 15041;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DigitalOutPlaceholder_Fault Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_DigitalOutPlaceholder_Fault = 1393;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DigitalOutPlaceholder_EnableDisable_InputArguments Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_DigitalOutPlaceholder_EnableDisable_InputArguments = 1399;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DigitalOutPlaceholder_State Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_DigitalOutPlaceholder_State = 1401;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DigitalOutPlaceholder_WriteState_InputArguments Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_DigitalOutPlaceholder_WriteState_InputArguments = 1403;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DiscreteOutPlaceholder_Fault Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_DiscreteOutPlaceholder_Fault = 1405;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DiscreteOutPlaceholder_EnableDisable_InputArguments Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_DiscreteOutPlaceholder_EnableDisable_InputArguments = 1411;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DiscreteOutPlaceholder_State Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_DiscreteOutPlaceholder_State = 1413;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DiscreteOutPlaceholder_WriteValue_InputArguments Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_DiscreteOutPlaceholder_WriteValue_InputArguments = 1415;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ValvePlaceholder_Fault Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_ValvePlaceholder_Fault = 1417;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ValvePlaceholder_EnableDisable_InputArguments Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_ValvePlaceholder_EnableDisable_InputArguments = 1423;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ValvePlaceholder_Position Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_ValvePlaceholder_Position = 1425;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ValvePlaceholder_Move_InputArguments Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_ValvePlaceholder_Move_InputArguments = 1434;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ChokePlaceholder_Fault Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_ChokePlaceholder_Fault = 1438;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ChokePlaceholder_EnableDisable_InputArguments Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_ChokePlaceholder_EnableDisable_InputArguments = 1444;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ChokePlaceholder_CalculatedPosition Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_ChokePlaceholder_CalculatedPosition = 1446;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ChokePlaceholder_Moving Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_ChokePlaceholder_Moving = 1449;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ChokePlaceholder_Move_InputArguments Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_ChokePlaceholder_Move_InputArguments = 1456;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ChokePlaceholder_Step_InputArguments Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_ChokePlaceholder_Step_InputArguments = 1458;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ChokePlaceholder_SetCalculatedPosition_InputArguments Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_ChokePlaceholder_SetCalculatedPosition_InputArguments = 1461;

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_InterlockPlaceholder Variable.
        /// </summary>
        public const uint MDISAggregateObjectType_InterlockPlaceholder = 1465;

        /// <summary>
        /// The identifier for the MDISTimeSyncObjectType_SetTime_InputArguments Variable.
        /// </summary>
        public const uint MDISTimeSyncObjectType_SetTime_InputArguments = 1470;

        /// <summary>
        /// The identifier for the MDISInformationObjectType_TimeSynchronization_SetTime_InputArguments Variable.
        /// </summary>
        public const uint MDISInformationObjectType_TimeSynchronization_SetTime_InputArguments = 1474;

        /// <summary>
        /// The identifier for the MDISInformationObjectType_MDISVersion Variable.
        /// </summary>
        public const uint MDISInformationObjectType_MDISVersion = 1476;

        /// <summary>
        /// The identifier for the MDISInformationObjectType_MDISVersion_MajorVersion Variable.
        /// </summary>
        public const uint MDISInformationObjectType_MDISVersion_MajorVersion = 1477;

        /// <summary>
        /// The identifier for the MDISInformationObjectType_MDISVersion_MinorVersion Variable.
        /// </summary>
        public const uint MDISInformationObjectType_MDISVersion_MinorVersion = 1478;

        /// <summary>
        /// The identifier for the MDISInformationObjectType_MDISVersion_Build Variable.
        /// </summary>
        public const uint MDISInformationObjectType_MDISVersion_Build = 1479;

        /// <summary>
        /// The identifier for the MDISInformation_TimeSynchronization_SetTime_InputArguments Variable.
        /// </summary>
        public const uint MDISInformation_TimeSynchronization_SetTime_InputArguments = 15047;

        /// <summary>
        /// The identifier for the MDISInformation_MDISVersion Variable.
        /// </summary>
        public const uint MDISInformation_MDISVersion = 15049;

        /// <summary>
        /// The identifier for the MDISInformation_MDISVersion_MajorVersion Variable.
        /// </summary>
        public const uint MDISInformation_MDISVersion_MajorVersion = 15050;

        /// <summary>
        /// The identifier for the MDISInformation_MDISVersion_MinorVersion Variable.
        /// </summary>
        public const uint MDISInformation_MDISVersion_MinorVersion = 15051;

        /// <summary>
        /// The identifier for the MDISInformation_MDISVersion_Build Variable.
        /// </summary>
        public const uint MDISInformation_MDISVersion_Build = 15052;

        /// <summary>
        /// The identifier for the MDIS_BinarySchema Variable.
        /// </summary>
        public const uint MDIS_BinarySchema = 374;

        /// <summary>
        /// The identifier for the MDIS_BinarySchema_NamespaceUri Variable.
        /// </summary>
        public const uint MDIS_BinarySchema_NamespaceUri = 376;

        /// <summary>
        /// The identifier for the MDIS_BinarySchema_Deprecated Variable.
        /// </summary>
        public const uint MDIS_BinarySchema_Deprecated = 15002;

        /// <summary>
        /// The identifier for the MDIS_BinarySchema_MDISVersionDataType Variable.
        /// </summary>
        public const uint MDIS_BinarySchema_MDISVersionDataType = 1485;

        /// <summary>
        /// The identifier for the MDIS_XmlSchema Variable.
        /// </summary>
        public const uint MDIS_XmlSchema = 367;

        /// <summary>
        /// The identifier for the MDIS_XmlSchema_NamespaceUri Variable.
        /// </summary>
        public const uint MDIS_XmlSchema_NamespaceUri = 369;

        /// <summary>
        /// The identifier for the MDIS_XmlSchema_Deprecated Variable.
        /// </summary>
        public const uint MDIS_XmlSchema_Deprecated = 15003;

        /// <summary>
        /// The identifier for the MDIS_XmlSchema_MDISVersionDataType Variable.
        /// </summary>
        public const uint MDIS_XmlSchema_MDISVersionDataType = 1481;
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
        /// The identifier for the InterlockVariableType VariableType.
        /// </summary>
        public const uint InterlockVariableType = 1279;

        /// <summary>
        /// The identifier for the MDISVersionVariableType VariableType.
        /// </summary>
        public const uint MDISVersionVariableType = 1290;
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
        /// The identifier for the SignatureStatusEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId SignatureStatusEnum = new ExpandedNodeId(Opc.MDIS.DataTypes.SignatureStatusEnum, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the SetCalculatedPositionEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId SetCalculatedPositionEnum = new ExpandedNodeId(Opc.MDIS.DataTypes.SetCalculatedPositionEnum, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the CommandEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId CommandEnum = new ExpandedNodeId(Opc.MDIS.DataTypes.CommandEnum, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the ChokeCommandEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId ChokeCommandEnum = new ExpandedNodeId(Opc.MDIS.DataTypes.ChokeCommandEnum, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the SEMEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId SEMEnum = new ExpandedNodeId(Opc.MDIS.DataTypes.SEMEnum, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the ValvePositionEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId ValvePositionEnum = new ExpandedNodeId(Opc.MDIS.DataTypes.ValvePositionEnum, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the ChokeMoveEnum DataType.
        /// </summary>
        public static readonly ExpandedNodeId ChokeMoveEnum = new ExpandedNodeId(Opc.MDIS.DataTypes.ChokeMoveEnum, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISVersionDataType DataType.
        /// </summary>
        public static readonly ExpandedNodeId MDISVersionDataType = new ExpandedNodeId(Opc.MDIS.DataTypes.MDISVersionDataType, Opc.MDIS.Namespaces.MDIS);
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
        /// The identifier for the MDISBaseObjectType_EnableDisable Method.
        /// </summary>
        public static readonly ExpandedNodeId MDISBaseObjectType_EnableDisable = new ExpandedNodeId(Opc.MDIS.Methods.MDISBaseObjectType_EnableDisable, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_Move Method.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_Move = new ExpandedNodeId(Opc.MDIS.Methods.MDISValveObjectType_Move, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_Open Method.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_ValveSignature_Open = new ExpandedNodeId(Opc.MDIS.Methods.MDISValveObjectType_ValveSignature_Open, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_Close Method.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_ValveSignature_Close = new ExpandedNodeId(Opc.MDIS.Methods.MDISValveObjectType_ValveSignature_Close, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_Read Method.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_ValveSignature_Read = new ExpandedNodeId(Opc.MDIS.Methods.MDISValveObjectType_ValveSignature_Read, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_Write Method.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_ValveSignature_Write = new ExpandedNodeId(Opc.MDIS.Methods.MDISValveObjectType_ValveSignature_Write, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_GetPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_ValveSignature_GetPosition = new ExpandedNodeId(Opc.MDIS.Methods.MDISValveObjectType_ValveSignature_GetPosition, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_SetPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_ValveSignature_SetPosition = new ExpandedNodeId(Opc.MDIS.Methods.MDISValveObjectType_ValveSignature_SetPosition, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISDigitalOutObjectType_WriteState Method.
        /// </summary>
        public static readonly ExpandedNodeId MDISDigitalOutObjectType_WriteState = new ExpandedNodeId(Opc.MDIS.Methods.MDISDigitalOutObjectType_WriteState, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISDiscreteOutObjectType_WriteValue Method.
        /// </summary>
        public static readonly ExpandedNodeId MDISDiscreteOutObjectType_WriteValue = new ExpandedNodeId(Opc.MDIS.Methods.MDISDiscreteOutObjectType_WriteValue, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInstrumentOutObjectType_WriteValue Method.
        /// </summary>
        public static readonly ExpandedNodeId MDISInstrumentOutObjectType_WriteValue = new ExpandedNodeId(Opc.MDIS.Methods.MDISInstrumentOutObjectType_WriteValue, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISChokeObjectType_Move Method.
        /// </summary>
        public static readonly ExpandedNodeId MDISChokeObjectType_Move = new ExpandedNodeId(Opc.MDIS.Methods.MDISChokeObjectType_Move, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISChokeObjectType_Step Method.
        /// </summary>
        public static readonly ExpandedNodeId MDISChokeObjectType_Step = new ExpandedNodeId(Opc.MDIS.Methods.MDISChokeObjectType_Step, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISChokeObjectType_Abort Method.
        /// </summary>
        public static readonly ExpandedNodeId MDISChokeObjectType_Abort = new ExpandedNodeId(Opc.MDIS.Methods.MDISChokeObjectType_Abort, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISChokeObjectType_SetCalculatedPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId MDISChokeObjectType_SetCalculatedPosition = new ExpandedNodeId(Opc.MDIS.Methods.MDISChokeObjectType_SetCalculatedPosition, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_InstrumentOutPlaceholder_WriteValue Method.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_InstrumentOutPlaceholder_WriteValue = new ExpandedNodeId(Opc.MDIS.Methods.MDISAggregateObjectType_InstrumentOutPlaceholder_WriteValue, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DigitalOutPlaceholder_WriteState Method.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_DigitalOutPlaceholder_WriteState = new ExpandedNodeId(Opc.MDIS.Methods.MDISAggregateObjectType_DigitalOutPlaceholder_WriteState, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DiscreteOutPlaceholder_WriteValue Method.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_DiscreteOutPlaceholder_WriteValue = new ExpandedNodeId(Opc.MDIS.Methods.MDISAggregateObjectType_DiscreteOutPlaceholder_WriteValue, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ValvePlaceholder_Move Method.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_ValvePlaceholder_Move = new ExpandedNodeId(Opc.MDIS.Methods.MDISAggregateObjectType_ValvePlaceholder_Move, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ChokePlaceholder_Move Method.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_ChokePlaceholder_Move = new ExpandedNodeId(Opc.MDIS.Methods.MDISAggregateObjectType_ChokePlaceholder_Move, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ChokePlaceholder_Abort Method.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_ChokePlaceholder_Abort = new ExpandedNodeId(Opc.MDIS.Methods.MDISAggregateObjectType_ChokePlaceholder_Abort, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ChokePlaceholder_SetCalculatedPosition Method.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_ChokePlaceholder_SetCalculatedPosition = new ExpandedNodeId(Opc.MDIS.Methods.MDISAggregateObjectType_ChokePlaceholder_SetCalculatedPosition, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISTimeSyncObjectType_SetTime Method.
        /// </summary>
        public static readonly ExpandedNodeId MDISTimeSyncObjectType_SetTime = new ExpandedNodeId(Opc.MDIS.Methods.MDISTimeSyncObjectType_SetTime, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInformationObjectType_TimeSynchronization_SetTime Method.
        /// </summary>
        public static readonly ExpandedNodeId MDISInformationObjectType_TimeSynchronization_SetTime = new ExpandedNodeId(Opc.MDIS.Methods.MDISInformationObjectType_TimeSynchronization_SetTime, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInformation_TimeSynchronization_SetTime Method.
        /// </summary>
        public static readonly ExpandedNodeId MDISInformation_TimeSynchronization_SetTime = new ExpandedNodeId(Opc.MDIS.Methods.MDISInformation_TimeSynchronization_SetTime, Opc.MDIS.Namespaces.MDIS);
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
        /// The identifier for the MDISValveObjectType_ValveSignature Object.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_ValveSignature = new ExpandedNodeId(Opc.MDIS.Objects.MDISValveObjectType_ValveSignature, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_InstrumentPlaceholder Object.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_InstrumentPlaceholder = new ExpandedNodeId(Opc.MDIS.Objects.MDISAggregateObjectType_InstrumentPlaceholder, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_InstrumentOutPlaceholder Object.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_InstrumentOutPlaceholder = new ExpandedNodeId(Opc.MDIS.Objects.MDISAggregateObjectType_InstrumentOutPlaceholder, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DigitalInstrumentPlaceholder Object.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_DigitalInstrumentPlaceholder = new ExpandedNodeId(Opc.MDIS.Objects.MDISAggregateObjectType_DigitalInstrumentPlaceholder, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DiscreteInstrumentPlaceholder Object.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_DiscreteInstrumentPlaceholder = new ExpandedNodeId(Opc.MDIS.Objects.MDISAggregateObjectType_DiscreteInstrumentPlaceholder, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DigitalOutPlaceholder Object.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_DigitalOutPlaceholder = new ExpandedNodeId(Opc.MDIS.Objects.MDISAggregateObjectType_DigitalOutPlaceholder, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DiscreteOutPlaceholder Object.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_DiscreteOutPlaceholder = new ExpandedNodeId(Opc.MDIS.Objects.MDISAggregateObjectType_DiscreteOutPlaceholder, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ValvePlaceholder Object.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_ValvePlaceholder = new ExpandedNodeId(Opc.MDIS.Objects.MDISAggregateObjectType_ValvePlaceholder, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ChokePlaceholder Object.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_ChokePlaceholder = new ExpandedNodeId(Opc.MDIS.Objects.MDISAggregateObjectType_ChokePlaceholder, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInformationObjectType_TimeSynchronization Object.
        /// </summary>
        public static readonly ExpandedNodeId MDISInformationObjectType_TimeSynchronization = new ExpandedNodeId(Opc.MDIS.Objects.MDISInformationObjectType_TimeSynchronization, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInformationObjectType_Signatures Object.
        /// </summary>
        public static readonly ExpandedNodeId MDISInformationObjectType_Signatures = new ExpandedNodeId(Opc.MDIS.Objects.MDISInformationObjectType_Signatures, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInformation Object.
        /// </summary>
        public static readonly ExpandedNodeId MDISInformation = new ExpandedNodeId(Opc.MDIS.Objects.MDISInformation, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInformation_TimeSynchronization Object.
        /// </summary>
        public static readonly ExpandedNodeId MDISInformation_TimeSynchronization = new ExpandedNodeId(Opc.MDIS.Objects.MDISInformation_TimeSynchronization, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInformation_Signatures Object.
        /// </summary>
        public static readonly ExpandedNodeId MDISInformation_Signatures = new ExpandedNodeId(Opc.MDIS.Objects.MDISInformation_Signatures, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISVersionDataType_Encoding_DefaultBinary Object.
        /// </summary>
        public static readonly ExpandedNodeId MDISVersionDataType_Encoding_DefaultBinary = new ExpandedNodeId(Opc.MDIS.Objects.MDISVersionDataType_Encoding_DefaultBinary, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISVersionDataType_Encoding_DefaultXml Object.
        /// </summary>
        public static readonly ExpandedNodeId MDISVersionDataType_Encoding_DefaultXml = new ExpandedNodeId(Opc.MDIS.Objects.MDISVersionDataType_Encoding_DefaultXml, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISVersionDataType_Encoding_DefaultJson Object.
        /// </summary>
        public static readonly ExpandedNodeId MDISVersionDataType_Encoding_DefaultJson = new ExpandedNodeId(Opc.MDIS.Objects.MDISVersionDataType_Encoding_DefaultJson, Opc.MDIS.Namespaces.MDIS);
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
        /// The identifier for the MDISBaseObjectType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId MDISBaseObjectType = new ExpandedNodeId(Opc.MDIS.ObjectTypes.MDISBaseObjectType, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType = new ExpandedNodeId(Opc.MDIS.ObjectTypes.MDISValveObjectType, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISDigitalInstrumentObjectType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId MDISDigitalInstrumentObjectType = new ExpandedNodeId(Opc.MDIS.ObjectTypes.MDISDigitalInstrumentObjectType, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISDigitalOutObjectType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId MDISDigitalOutObjectType = new ExpandedNodeId(Opc.MDIS.ObjectTypes.MDISDigitalOutObjectType, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISDiscreteInstrumentObjectType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId MDISDiscreteInstrumentObjectType = new ExpandedNodeId(Opc.MDIS.ObjectTypes.MDISDiscreteInstrumentObjectType, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISDiscreteOutObjectType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId MDISDiscreteOutObjectType = new ExpandedNodeId(Opc.MDIS.ObjectTypes.MDISDiscreteOutObjectType, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInstrumentObjectType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId MDISInstrumentObjectType = new ExpandedNodeId(Opc.MDIS.ObjectTypes.MDISInstrumentObjectType, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInstrumentOutObjectType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId MDISInstrumentOutObjectType = new ExpandedNodeId(Opc.MDIS.ObjectTypes.MDISInstrumentOutObjectType, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISChokeObjectType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId MDISChokeObjectType = new ExpandedNodeId(Opc.MDIS.ObjectTypes.MDISChokeObjectType, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType = new ExpandedNodeId(Opc.MDIS.ObjectTypes.MDISAggregateObjectType, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISTimeSyncObjectType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId MDISTimeSyncObjectType = new ExpandedNodeId(Opc.MDIS.ObjectTypes.MDISTimeSyncObjectType, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInformationObjectType ObjectType.
        /// </summary>
        public static readonly ExpandedNodeId MDISInformationObjectType = new ExpandedNodeId(Opc.MDIS.ObjectTypes.MDISInformationObjectType, Opc.MDIS.Namespaces.MDIS);
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
        /// The identifier for the HasInterlock ReferenceType.
        /// </summary>
        public static readonly ExpandedNodeId HasInterlock = new ExpandedNodeId(Opc.MDIS.ReferenceTypes.HasInterlock, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the HasSignature ReferenceType.
        /// </summary>
        public static readonly ExpandedNodeId HasSignature = new ExpandedNodeId(Opc.MDIS.ReferenceTypes.HasSignature, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the InterlockFor ReferenceType.
        /// </summary>
        public static readonly ExpandedNodeId InterlockFor = new ExpandedNodeId(Opc.MDIS.ReferenceTypes.InterlockFor, Opc.MDIS.Namespaces.MDIS);
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
        /// The identifier for the MDISBaseObjectType_Fault Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISBaseObjectType_Fault = new ExpandedNodeId(Opc.MDIS.Variables.MDISBaseObjectType_Fault, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISBaseObjectType_FaultCode Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISBaseObjectType_FaultCode = new ExpandedNodeId(Opc.MDIS.Variables.MDISBaseObjectType_FaultCode, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISBaseObjectType_Warning Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISBaseObjectType_Warning = new ExpandedNodeId(Opc.MDIS.Variables.MDISBaseObjectType_Warning, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISBaseObjectType_WarningCode Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISBaseObjectType_WarningCode = new ExpandedNodeId(Opc.MDIS.Variables.MDISBaseObjectType_WarningCode, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISBaseObjectType_Enabled Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISBaseObjectType_Enabled = new ExpandedNodeId(Opc.MDIS.Variables.MDISBaseObjectType_Enabled, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISBaseObjectType_EnableDisable_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISBaseObjectType_EnableDisable_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISBaseObjectType_EnableDisable_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISBaseObjectType_TagId Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISBaseObjectType_TagId = new ExpandedNodeId(Opc.MDIS.Variables.MDISBaseObjectType_TagId, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the SignatureStatusEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId SignatureStatusEnum_EnumValues = new ExpandedNodeId(Opc.MDIS.Variables.SignatureStatusEnum_EnumValues, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the SetCalculatedPositionEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId SetCalculatedPositionEnum_EnumValues = new ExpandedNodeId(Opc.MDIS.Variables.SetCalculatedPositionEnum_EnumValues, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the CommandEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId CommandEnum_EnumValues = new ExpandedNodeId(Opc.MDIS.Variables.CommandEnum_EnumValues, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the ChokeCommandEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId ChokeCommandEnum_EnumValues = new ExpandedNodeId(Opc.MDIS.Variables.ChokeCommandEnum_EnumValues, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the SEMEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId SEMEnum_EnumValues = new ExpandedNodeId(Opc.MDIS.Variables.SEMEnum_EnumValues, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the ValvePositionEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId ValvePositionEnum_EnumValues = new ExpandedNodeId(Opc.MDIS.Variables.ValvePositionEnum_EnumValues, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the ChokeMoveEnum_EnumValues Variable.
        /// </summary>
        public static readonly ExpandedNodeId ChokeMoveEnum_EnumValues = new ExpandedNodeId(Opc.MDIS.Variables.ChokeMoveEnum_EnumValues, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISVersionVariableType_MajorVersion Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISVersionVariableType_MajorVersion = new ExpandedNodeId(Opc.MDIS.Variables.MDISVersionVariableType_MajorVersion, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISVersionVariableType_MinorVersion Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISVersionVariableType_MinorVersion = new ExpandedNodeId(Opc.MDIS.Variables.MDISVersionVariableType_MinorVersion, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISVersionVariableType_Build Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISVersionVariableType_Build = new ExpandedNodeId(Opc.MDIS.Variables.MDISVersionVariableType_Build, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_EnableDisable_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_EnableDisable_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISValveObjectType_EnableDisable_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_Position Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_Position = new ExpandedNodeId(Opc.MDIS.Variables.MDISValveObjectType_Position, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_CommandRejected Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_CommandRejected = new ExpandedNodeId(Opc.MDIS.Variables.MDISValveObjectType_CommandRejected, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_SignatureRequestStatus Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_SignatureRequestStatus = new ExpandedNodeId(Opc.MDIS.Variables.MDISValveObjectType_SignatureRequestStatus, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_LastCommand Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_LastCommand = new ExpandedNodeId(Opc.MDIS.Variables.MDISValveObjectType_LastCommand, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_NonDefeatableOpenInterlock Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_NonDefeatableOpenInterlock = new ExpandedNodeId(Opc.MDIS.Variables.MDISValveObjectType_NonDefeatableOpenInterlock, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_DefeatableOpenInterlock Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_DefeatableOpenInterlock = new ExpandedNodeId(Opc.MDIS.Variables.MDISValveObjectType_DefeatableOpenInterlock, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_NonDefeatableCloseInterlock Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_NonDefeatableCloseInterlock = new ExpandedNodeId(Opc.MDIS.Variables.MDISValveObjectType_NonDefeatableCloseInterlock, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_DefeatableCloseInterlock Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_DefeatableCloseInterlock = new ExpandedNodeId(Opc.MDIS.Variables.MDISValveObjectType_DefeatableCloseInterlock, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_Move_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_Move_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISValveObjectType_Move_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_InterlockPlaceholder Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_InterlockPlaceholder = new ExpandedNodeId(Opc.MDIS.Variables.MDISValveObjectType_InterlockPlaceholder, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_OpenTimeDuration Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_OpenTimeDuration = new ExpandedNodeId(Opc.MDIS.Variables.MDISValveObjectType_OpenTimeDuration, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_CloseTimeDuration Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_CloseTimeDuration = new ExpandedNodeId(Opc.MDIS.Variables.MDISValveObjectType_CloseTimeDuration, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_Size Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_ValveSignature_Size = new ExpandedNodeId(Opc.MDIS.Variables.MDISValveObjectType_ValveSignature_Size, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_Writable Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_ValveSignature_Writable = new ExpandedNodeId(Opc.MDIS.Variables.MDISValveObjectType_ValveSignature_Writable, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_UserWritable Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_ValveSignature_UserWritable = new ExpandedNodeId(Opc.MDIS.Variables.MDISValveObjectType_ValveSignature_UserWritable, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_OpenCount Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_ValveSignature_OpenCount = new ExpandedNodeId(Opc.MDIS.Variables.MDISValveObjectType_ValveSignature_OpenCount, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_Open_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_ValveSignature_Open_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISValveObjectType_ValveSignature_Open_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_Open_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_ValveSignature_Open_OutputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISValveObjectType_ValveSignature_Open_OutputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_Close_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_ValveSignature_Close_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISValveObjectType_ValveSignature_Close_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_Read_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_ValveSignature_Read_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISValveObjectType_ValveSignature_Read_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_Read_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_ValveSignature_Read_OutputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISValveObjectType_ValveSignature_Read_OutputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_Write_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_ValveSignature_Write_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISValveObjectType_ValveSignature_Write_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_GetPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_ValveSignature_GetPosition_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISValveObjectType_ValveSignature_GetPosition_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_GetPosition_OutputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_ValveSignature_GetPosition_OutputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISValveObjectType_ValveSignature_GetPosition_OutputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISValveObjectType_ValveSignature_SetPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISValveObjectType_ValveSignature_SetPosition_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISValveObjectType_ValveSignature_SetPosition_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISDigitalInstrumentObjectType_EnableDisable_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISDigitalInstrumentObjectType_EnableDisable_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISDigitalInstrumentObjectType_EnableDisable_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISDigitalInstrumentObjectType_State Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISDigitalInstrumentObjectType_State = new ExpandedNodeId(Opc.MDIS.Variables.MDISDigitalInstrumentObjectType_State, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISDigitalOutObjectType_EnableDisable_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISDigitalOutObjectType_EnableDisable_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISDigitalOutObjectType_EnableDisable_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISDigitalOutObjectType_WriteState_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISDigitalOutObjectType_WriteState_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISDigitalOutObjectType_WriteState_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISDiscreteInstrumentObjectType_EnableDisable_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISDiscreteInstrumentObjectType_EnableDisable_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISDiscreteInstrumentObjectType_EnableDisable_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISDiscreteInstrumentObjectType_State Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISDiscreteInstrumentObjectType_State = new ExpandedNodeId(Opc.MDIS.Variables.MDISDiscreteInstrumentObjectType_State, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISDiscreteOutObjectType_EnableDisable_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISDiscreteOutObjectType_EnableDisable_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISDiscreteOutObjectType_EnableDisable_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISDiscreteOutObjectType_WriteValue_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISDiscreteOutObjectType_WriteValue_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISDiscreteOutObjectType_WriteValue_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInstrumentObjectType_EnableDisable_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISInstrumentObjectType_EnableDisable_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISInstrumentObjectType_EnableDisable_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInstrumentObjectType_ProcessVariable Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISInstrumentObjectType_ProcessVariable = new ExpandedNodeId(Opc.MDIS.Variables.MDISInstrumentObjectType_ProcessVariable, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInstrumentObjectType_ProcessVariable_EURange Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISInstrumentObjectType_ProcessVariable_EURange = new ExpandedNodeId(Opc.MDIS.Variables.MDISInstrumentObjectType_ProcessVariable_EURange, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInstrumentObjectType_ProcessVariable_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISInstrumentObjectType_ProcessVariable_EngineeringUnits = new ExpandedNodeId(Opc.MDIS.Variables.MDISInstrumentObjectType_ProcessVariable_EngineeringUnits, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInstrumentObjectType_HHlimit Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISInstrumentObjectType_HHlimit = new ExpandedNodeId(Opc.MDIS.Variables.MDISInstrumentObjectType_HHlimit, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInstrumentObjectType_Hlimit Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISInstrumentObjectType_Hlimit = new ExpandedNodeId(Opc.MDIS.Variables.MDISInstrumentObjectType_Hlimit, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInstrumentObjectType_Llimit Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISInstrumentObjectType_Llimit = new ExpandedNodeId(Opc.MDIS.Variables.MDISInstrumentObjectType_Llimit, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInstrumentObjectType_LLlimit Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISInstrumentObjectType_LLlimit = new ExpandedNodeId(Opc.MDIS.Variables.MDISInstrumentObjectType_LLlimit, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInstrumentObjectType_HHSetPoint Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISInstrumentObjectType_HHSetPoint = new ExpandedNodeId(Opc.MDIS.Variables.MDISInstrumentObjectType_HHSetPoint, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInstrumentObjectType_HSetPoint Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISInstrumentObjectType_HSetPoint = new ExpandedNodeId(Opc.MDIS.Variables.MDISInstrumentObjectType_HSetPoint, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInstrumentObjectType_LSetPoint Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISInstrumentObjectType_LSetPoint = new ExpandedNodeId(Opc.MDIS.Variables.MDISInstrumentObjectType_LSetPoint, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInstrumentObjectType_LLSetPoint Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISInstrumentObjectType_LLSetPoint = new ExpandedNodeId(Opc.MDIS.Variables.MDISInstrumentObjectType_LLSetPoint, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInstrumentOutObjectType_EnableDisable_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISInstrumentOutObjectType_EnableDisable_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISInstrumentOutObjectType_EnableDisable_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInstrumentOutObjectType_ProcessVariable_EURange Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISInstrumentOutObjectType_ProcessVariable_EURange = new ExpandedNodeId(Opc.MDIS.Variables.MDISInstrumentOutObjectType_ProcessVariable_EURange, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInstrumentOutObjectType_ProcessVariable_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISInstrumentOutObjectType_ProcessVariable_EngineeringUnits = new ExpandedNodeId(Opc.MDIS.Variables.MDISInstrumentOutObjectType_ProcessVariable_EngineeringUnits, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInstrumentOutObjectType_WriteValue_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISInstrumentOutObjectType_WriteValue_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISInstrumentOutObjectType_WriteValue_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISChokeObjectType_EnableDisable_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISChokeObjectType_EnableDisable_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISChokeObjectType_EnableDisable_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISChokeObjectType_CalculatedPosition Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISChokeObjectType_CalculatedPosition = new ExpandedNodeId(Opc.MDIS.Variables.MDISChokeObjectType_CalculatedPosition, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISChokeObjectType_SetCalculatedPositionStatus Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISChokeObjectType_SetCalculatedPositionStatus = new ExpandedNodeId(Opc.MDIS.Variables.MDISChokeObjectType_SetCalculatedPositionStatus, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISChokeObjectType_PositionInSteps Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISChokeObjectType_PositionInSteps = new ExpandedNodeId(Opc.MDIS.Variables.MDISChokeObjectType_PositionInSteps, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISChokeObjectType_Moving Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISChokeObjectType_Moving = new ExpandedNodeId(Opc.MDIS.Variables.MDISChokeObjectType_Moving, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISChokeObjectType_CommandRejected Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISChokeObjectType_CommandRejected = new ExpandedNodeId(Opc.MDIS.Variables.MDISChokeObjectType_CommandRejected, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISChokeObjectType_NonDefeatableOpenInterlock Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISChokeObjectType_NonDefeatableOpenInterlock = new ExpandedNodeId(Opc.MDIS.Variables.MDISChokeObjectType_NonDefeatableOpenInterlock, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISChokeObjectType_DefeatableOpenInterlock Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISChokeObjectType_DefeatableOpenInterlock = new ExpandedNodeId(Opc.MDIS.Variables.MDISChokeObjectType_DefeatableOpenInterlock, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISChokeObjectType_NonDefeatableCloseInterlock Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISChokeObjectType_NonDefeatableCloseInterlock = new ExpandedNodeId(Opc.MDIS.Variables.MDISChokeObjectType_NonDefeatableCloseInterlock, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISChokeObjectType_DefeatableCloseInterlock Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISChokeObjectType_DefeatableCloseInterlock = new ExpandedNodeId(Opc.MDIS.Variables.MDISChokeObjectType_DefeatableCloseInterlock, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISChokeObjectType_Move_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISChokeObjectType_Move_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISChokeObjectType_Move_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISChokeObjectType_Step_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISChokeObjectType_Step_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISChokeObjectType_Step_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISChokeObjectType_SetCalculatedPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISChokeObjectType_SetCalculatedPosition_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISChokeObjectType_SetCalculatedPosition_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISChokeObjectType_InterlockPlaceholder Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISChokeObjectType_InterlockPlaceholder = new ExpandedNodeId(Opc.MDIS.Variables.MDISChokeObjectType_InterlockPlaceholder, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISChokeObjectType_StepDurationOpen Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISChokeObjectType_StepDurationOpen = new ExpandedNodeId(Opc.MDIS.Variables.MDISChokeObjectType_StepDurationOpen, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISChokeObjectType_StepDurationClose Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISChokeObjectType_StepDurationClose = new ExpandedNodeId(Opc.MDIS.Variables.MDISChokeObjectType_StepDurationClose, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISChokeObjectType_TotalSteps Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISChokeObjectType_TotalSteps = new ExpandedNodeId(Opc.MDIS.Variables.MDISChokeObjectType_TotalSteps, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_EnableDisable_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_EnableDisable_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_EnableDisable_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_InstrumentPlaceholder_Fault Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_InstrumentPlaceholder_Fault = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_InstrumentPlaceholder_Fault, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_InstrumentPlaceholder_EnableDisable_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_InstrumentPlaceholder_EnableDisable_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_InstrumentPlaceholder_EnableDisable_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_InstrumentPlaceholder_ProcessVariable Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_InstrumentPlaceholder_ProcessVariable = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_InstrumentPlaceholder_ProcessVariable, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_InstrumentPlaceholder_ProcessVariable_EURange Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_InstrumentPlaceholder_ProcessVariable_EURange = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_InstrumentPlaceholder_ProcessVariable_EURange, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_InstrumentPlaceholder_ProcessVariable_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_InstrumentPlaceholder_ProcessVariable_EngineeringUnits = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_InstrumentPlaceholder_ProcessVariable_EngineeringUnits, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_InstrumentOutPlaceholder_Fault Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_InstrumentOutPlaceholder_Fault = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_InstrumentOutPlaceholder_Fault, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_InstrumentOutPlaceholder_EnableDisable_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_InstrumentOutPlaceholder_EnableDisable_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_InstrumentOutPlaceholder_EnableDisable_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_InstrumentOutPlaceholder_ProcessVariable Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_InstrumentOutPlaceholder_ProcessVariable = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_InstrumentOutPlaceholder_ProcessVariable, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_InstrumentOutPlaceholder_ProcessVariable_EURange Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_InstrumentOutPlaceholder_ProcessVariable_EURange = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_InstrumentOutPlaceholder_ProcessVariable_EURange, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_InstrumentOutPlaceholder_ProcessVariable_EngineeringUnits Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_InstrumentOutPlaceholder_ProcessVariable_EngineeringUnits = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_InstrumentOutPlaceholder_ProcessVariable_EngineeringUnits, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_InstrumentOutPlaceholder_WriteValue_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_InstrumentOutPlaceholder_WriteValue_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_InstrumentOutPlaceholder_WriteValue_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DigitalInstrumentPlaceholder_Fault Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_DigitalInstrumentPlaceholder_Fault = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_DigitalInstrumentPlaceholder_Fault, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DigitalInstrumentPlaceholder_EnableDisable_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_DigitalInstrumentPlaceholder_EnableDisable_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_DigitalInstrumentPlaceholder_EnableDisable_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DigitalInstrumentPlaceholder_State Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_DigitalInstrumentPlaceholder_State = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_DigitalInstrumentPlaceholder_State, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DiscreteInstrumentPlaceholder_Fault Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_DiscreteInstrumentPlaceholder_Fault = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_DiscreteInstrumentPlaceholder_Fault, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DiscreteInstrumentPlaceholder_EnableDisable_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_DiscreteInstrumentPlaceholder_EnableDisable_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_DiscreteInstrumentPlaceholder_EnableDisable_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DiscreteInstrumentPlaceholder_State Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_DiscreteInstrumentPlaceholder_State = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_DiscreteInstrumentPlaceholder_State, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DigitalOutPlaceholder_Fault Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_DigitalOutPlaceholder_Fault = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_DigitalOutPlaceholder_Fault, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DigitalOutPlaceholder_EnableDisable_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_DigitalOutPlaceholder_EnableDisable_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_DigitalOutPlaceholder_EnableDisable_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DigitalOutPlaceholder_State Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_DigitalOutPlaceholder_State = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_DigitalOutPlaceholder_State, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DigitalOutPlaceholder_WriteState_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_DigitalOutPlaceholder_WriteState_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_DigitalOutPlaceholder_WriteState_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DiscreteOutPlaceholder_Fault Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_DiscreteOutPlaceholder_Fault = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_DiscreteOutPlaceholder_Fault, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DiscreteOutPlaceholder_EnableDisable_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_DiscreteOutPlaceholder_EnableDisable_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_DiscreteOutPlaceholder_EnableDisable_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DiscreteOutPlaceholder_State Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_DiscreteOutPlaceholder_State = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_DiscreteOutPlaceholder_State, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_DiscreteOutPlaceholder_WriteValue_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_DiscreteOutPlaceholder_WriteValue_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_DiscreteOutPlaceholder_WriteValue_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ValvePlaceholder_Fault Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_ValvePlaceholder_Fault = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_ValvePlaceholder_Fault, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ValvePlaceholder_EnableDisable_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_ValvePlaceholder_EnableDisable_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_ValvePlaceholder_EnableDisable_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ValvePlaceholder_Position Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_ValvePlaceholder_Position = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_ValvePlaceholder_Position, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ValvePlaceholder_Move_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_ValvePlaceholder_Move_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_ValvePlaceholder_Move_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ChokePlaceholder_Fault Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_ChokePlaceholder_Fault = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_ChokePlaceholder_Fault, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ChokePlaceholder_EnableDisable_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_ChokePlaceholder_EnableDisable_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_ChokePlaceholder_EnableDisable_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ChokePlaceholder_CalculatedPosition Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_ChokePlaceholder_CalculatedPosition = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_ChokePlaceholder_CalculatedPosition, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ChokePlaceholder_Moving Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_ChokePlaceholder_Moving = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_ChokePlaceholder_Moving, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ChokePlaceholder_Move_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_ChokePlaceholder_Move_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_ChokePlaceholder_Move_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ChokePlaceholder_Step_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_ChokePlaceholder_Step_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_ChokePlaceholder_Step_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_ChokePlaceholder_SetCalculatedPosition_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_ChokePlaceholder_SetCalculatedPosition_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_ChokePlaceholder_SetCalculatedPosition_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISAggregateObjectType_InterlockPlaceholder Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISAggregateObjectType_InterlockPlaceholder = new ExpandedNodeId(Opc.MDIS.Variables.MDISAggregateObjectType_InterlockPlaceholder, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISTimeSyncObjectType_SetTime_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISTimeSyncObjectType_SetTime_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISTimeSyncObjectType_SetTime_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInformationObjectType_TimeSynchronization_SetTime_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISInformationObjectType_TimeSynchronization_SetTime_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISInformationObjectType_TimeSynchronization_SetTime_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInformationObjectType_MDISVersion Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISInformationObjectType_MDISVersion = new ExpandedNodeId(Opc.MDIS.Variables.MDISInformationObjectType_MDISVersion, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInformationObjectType_MDISVersion_MajorVersion Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISInformationObjectType_MDISVersion_MajorVersion = new ExpandedNodeId(Opc.MDIS.Variables.MDISInformationObjectType_MDISVersion_MajorVersion, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInformationObjectType_MDISVersion_MinorVersion Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISInformationObjectType_MDISVersion_MinorVersion = new ExpandedNodeId(Opc.MDIS.Variables.MDISInformationObjectType_MDISVersion_MinorVersion, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInformationObjectType_MDISVersion_Build Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISInformationObjectType_MDISVersion_Build = new ExpandedNodeId(Opc.MDIS.Variables.MDISInformationObjectType_MDISVersion_Build, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInformation_TimeSynchronization_SetTime_InputArguments Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISInformation_TimeSynchronization_SetTime_InputArguments = new ExpandedNodeId(Opc.MDIS.Variables.MDISInformation_TimeSynchronization_SetTime_InputArguments, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInformation_MDISVersion Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISInformation_MDISVersion = new ExpandedNodeId(Opc.MDIS.Variables.MDISInformation_MDISVersion, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInformation_MDISVersion_MajorVersion Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISInformation_MDISVersion_MajorVersion = new ExpandedNodeId(Opc.MDIS.Variables.MDISInformation_MDISVersion_MajorVersion, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInformation_MDISVersion_MinorVersion Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISInformation_MDISVersion_MinorVersion = new ExpandedNodeId(Opc.MDIS.Variables.MDISInformation_MDISVersion_MinorVersion, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISInformation_MDISVersion_Build Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDISInformation_MDISVersion_Build = new ExpandedNodeId(Opc.MDIS.Variables.MDISInformation_MDISVersion_Build, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDIS_BinarySchema Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDIS_BinarySchema = new ExpandedNodeId(Opc.MDIS.Variables.MDIS_BinarySchema, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDIS_BinarySchema_NamespaceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDIS_BinarySchema_NamespaceUri = new ExpandedNodeId(Opc.MDIS.Variables.MDIS_BinarySchema_NamespaceUri, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDIS_BinarySchema_Deprecated Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDIS_BinarySchema_Deprecated = new ExpandedNodeId(Opc.MDIS.Variables.MDIS_BinarySchema_Deprecated, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDIS_BinarySchema_MDISVersionDataType Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDIS_BinarySchema_MDISVersionDataType = new ExpandedNodeId(Opc.MDIS.Variables.MDIS_BinarySchema_MDISVersionDataType, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDIS_XmlSchema Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDIS_XmlSchema = new ExpandedNodeId(Opc.MDIS.Variables.MDIS_XmlSchema, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDIS_XmlSchema_NamespaceUri Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDIS_XmlSchema_NamespaceUri = new ExpandedNodeId(Opc.MDIS.Variables.MDIS_XmlSchema_NamespaceUri, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDIS_XmlSchema_Deprecated Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDIS_XmlSchema_Deprecated = new ExpandedNodeId(Opc.MDIS.Variables.MDIS_XmlSchema_Deprecated, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDIS_XmlSchema_MDISVersionDataType Variable.
        /// </summary>
        public static readonly ExpandedNodeId MDIS_XmlSchema_MDISVersionDataType = new ExpandedNodeId(Opc.MDIS.Variables.MDIS_XmlSchema_MDISVersionDataType, Opc.MDIS.Namespaces.MDIS);
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
        /// The identifier for the InterlockVariableType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId InterlockVariableType = new ExpandedNodeId(Opc.MDIS.VariableTypes.InterlockVariableType, Opc.MDIS.Namespaces.MDIS);

        /// <summary>
        /// The identifier for the MDISVersionVariableType VariableType.
        /// </summary>
        public static readonly ExpandedNodeId MDISVersionVariableType = new ExpandedNodeId(Opc.MDIS.VariableTypes.MDISVersionVariableType, Opc.MDIS.Namespaces.MDIS);
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
        /// The BrowseName for the Build component.
        /// </summary>
        public const string Build = "Build";

        /// <summary>
        /// The BrowseName for the CalculatedPosition component.
        /// </summary>
        public const string CalculatedPosition = "CalculatedPosition";

        /// <summary>
        /// The BrowseName for the ChokeCommandEnum component.
        /// </summary>
        public const string ChokeCommandEnum = "ChokeCommandEnum";

        /// <summary>
        /// The BrowseName for the ChokeMoveEnum component.
        /// </summary>
        public const string ChokeMoveEnum = "ChokeMoveEnum";

        /// <summary>
        /// The BrowseName for the ChokePlaceholder component.
        /// </summary>
        public const string ChokePlaceholder = "<ChokePlaceholder>";

        /// <summary>
        /// The BrowseName for the CloseTimeDuration component.
        /// </summary>
        public const string CloseTimeDuration = "CloseTimeDuration";

        /// <summary>
        /// The BrowseName for the CommandEnum component.
        /// </summary>
        public const string CommandEnum = "CommandEnum";

        /// <summary>
        /// The BrowseName for the CommandRejected component.
        /// </summary>
        public const string CommandRejected = "CommandRejected";

        /// <summary>
        /// The BrowseName for the DefeatableCloseInterlock component.
        /// </summary>
        public const string DefeatableCloseInterlock = "DefeatableCloseInterlock";

        /// <summary>
        /// The BrowseName for the DefeatableOpenInterlock component.
        /// </summary>
        public const string DefeatableOpenInterlock = "DefeatableOpenInterlock";

        /// <summary>
        /// The BrowseName for the DigitalInstrumentPlaceholder component.
        /// </summary>
        public const string DigitalInstrumentPlaceholder = "<DigitalInstrumentPlaceholder>";

        /// <summary>
        /// The BrowseName for the DigitalOutPlaceholder component.
        /// </summary>
        public const string DigitalOutPlaceholder = "<DigitalOutPlaceholder>";

        /// <summary>
        /// The BrowseName for the DiscreteInstrumentPlaceholder component.
        /// </summary>
        public const string DiscreteInstrumentPlaceholder = "<DiscreteInstrumentPlaceholder>";

        /// <summary>
        /// The BrowseName for the DiscreteOutPlaceholder component.
        /// </summary>
        public const string DiscreteOutPlaceholder = "<DiscreteOutPlaceholder>";

        /// <summary>
        /// The BrowseName for the Enabled component.
        /// </summary>
        public const string Enabled = "Enabled";

        /// <summary>
        /// The BrowseName for the EnableDisable component.
        /// </summary>
        public const string EnableDisable = "EnableDisable";

        /// <summary>
        /// The BrowseName for the Fault component.
        /// </summary>
        public const string Fault = "Fault";

        /// <summary>
        /// The BrowseName for the FaultCode component.
        /// </summary>
        public const string FaultCode = "FaultCode";

        /// <summary>
        /// The BrowseName for the HasInterlock component.
        /// </summary>
        public const string HasInterlock = "HasInterlock";

        /// <summary>
        /// The BrowseName for the HasSignature component.
        /// </summary>
        public const string HasSignature = "HasSignature";

        /// <summary>
        /// The BrowseName for the HHlimit component.
        /// </summary>
        public const string HHlimit = "HHlimit";

        /// <summary>
        /// The BrowseName for the HHSetPoint component.
        /// </summary>
        public const string HHSetPoint = "HHSetPoint";

        /// <summary>
        /// The BrowseName for the Hlimit component.
        /// </summary>
        public const string Hlimit = "Hlimit";

        /// <summary>
        /// The BrowseName for the HSetPoint component.
        /// </summary>
        public const string HSetPoint = "HSetPoint";

        /// <summary>
        /// The BrowseName for the InstrumentOutPlaceholder component.
        /// </summary>
        public const string InstrumentOutPlaceholder = "<InstrumentOutPlaceholder>";

        /// <summary>
        /// The BrowseName for the InstrumentPlaceholder component.
        /// </summary>
        public const string InstrumentPlaceholder = "<InstrumentPlaceholder>";

        /// <summary>
        /// The BrowseName for the InterlockFor component.
        /// </summary>
        public const string InterlockFor = "InterlockFor";

        /// <summary>
        /// The BrowseName for the InterlockPlaceholder component.
        /// </summary>
        public const string InterlockPlaceholder = "<InterlockPlaceholder>";

        /// <summary>
        /// The BrowseName for the InterlockVariableType component.
        /// </summary>
        public const string InterlockVariableType = "InterlockVariableType";

        /// <summary>
        /// The BrowseName for the LastCommand component.
        /// </summary>
        public const string LastCommand = "LastCommand";

        /// <summary>
        /// The BrowseName for the Llimit component.
        /// </summary>
        public const string Llimit = "Llimit";

        /// <summary>
        /// The BrowseName for the LLlimit component.
        /// </summary>
        public const string LLlimit = "LLlimit";

        /// <summary>
        /// The BrowseName for the LLSetPoint component.
        /// </summary>
        public const string LLSetPoint = "LLSetPoint";

        /// <summary>
        /// The BrowseName for the LSetPoint component.
        /// </summary>
        public const string LSetPoint = "LSetPoint";

        /// <summary>
        /// The BrowseName for the MajorVersion component.
        /// </summary>
        public const string MajorVersion = "MajorVersion";

        /// <summary>
        /// The BrowseName for the MDIS_BinarySchema component.
        /// </summary>
        public const string MDIS_BinarySchema = "Opc.MDIS";

        /// <summary>
        /// The BrowseName for the MDIS_XmlSchema component.
        /// </summary>
        public const string MDIS_XmlSchema = "Opc.MDIS";

        /// <summary>
        /// The BrowseName for the MDISAggregateObjectType component.
        /// </summary>
        public const string MDISAggregateObjectType = "MDISAggregateObjectType";

        /// <summary>
        /// The BrowseName for the MDISBaseObjectType component.
        /// </summary>
        public const string MDISBaseObjectType = "MDISBaseObjectType";

        /// <summary>
        /// The BrowseName for the MDISChokeObjectType component.
        /// </summary>
        public const string MDISChokeObjectType = "MDISChokeObjectType";

        /// <summary>
        /// The BrowseName for the MDISDigitalInstrumentObjectType component.
        /// </summary>
        public const string MDISDigitalInstrumentObjectType = "MDISDigitalInstrumentObjectType";

        /// <summary>
        /// The BrowseName for the MDISDigitalOutObjectType component.
        /// </summary>
        public const string MDISDigitalOutObjectType = "MDISDigitalOutObjectType";

        /// <summary>
        /// The BrowseName for the MDISDiscreteInstrumentObjectType component.
        /// </summary>
        public const string MDISDiscreteInstrumentObjectType = "MDISDiscreteInstrumentObjectType";

        /// <summary>
        /// The BrowseName for the MDISDiscreteOutObjectType component.
        /// </summary>
        public const string MDISDiscreteOutObjectType = "MDISDiscreteOutObjectType";

        /// <summary>
        /// The BrowseName for the MDISInformation component.
        /// </summary>
        public const string MDISInformation = "MDISInformation";

        /// <summary>
        /// The BrowseName for the MDISInformationObjectType component.
        /// </summary>
        public const string MDISInformationObjectType = "MDISInformationObjectType";

        /// <summary>
        /// The BrowseName for the MDISInstrumentObjectType component.
        /// </summary>
        public const string MDISInstrumentObjectType = "MDISInstrumentObjectType";

        /// <summary>
        /// The BrowseName for the MDISInstrumentOutObjectType component.
        /// </summary>
        public const string MDISInstrumentOutObjectType = "MDISInstrumentOutObjectType";

        /// <summary>
        /// The BrowseName for the MDISTimeSyncObjectType component.
        /// </summary>
        public const string MDISTimeSyncObjectType = "MDISTimeSyncObjectType";

        /// <summary>
        /// The BrowseName for the MDISValveObjectType component.
        /// </summary>
        public const string MDISValveObjectType = "MDISValveObjectType";

        /// <summary>
        /// The BrowseName for the MDISVersion component.
        /// </summary>
        public const string MDISVersion = "MDISVersion";

        /// <summary>
        /// The BrowseName for the MDISVersionDataType component.
        /// </summary>
        public const string MDISVersionDataType = "MDISVersionDataType";

        /// <summary>
        /// The BrowseName for the MDISVersionVariableType component.
        /// </summary>
        public const string MDISVersionVariableType = "MDISVersionVariableType";

        /// <summary>
        /// The BrowseName for the MinorVersion component.
        /// </summary>
        public const string MinorVersion = "MinorVersion";

        /// <summary>
        /// The BrowseName for the Move component.
        /// </summary>
        public const string Move = "Move";

        /// <summary>
        /// The BrowseName for the Moving component.
        /// </summary>
        public const string Moving = "Moving";

        /// <summary>
        /// The BrowseName for the NonDefeatableCloseInterlock component.
        /// </summary>
        public const string NonDefeatableCloseInterlock = "NonDefeatableCloseInterlock";

        /// <summary>
        /// The BrowseName for the NonDefeatableOpenInterlock component.
        /// </summary>
        public const string NonDefeatableOpenInterlock = "NonDefeatableOpenInterlock";

        /// <summary>
        /// The BrowseName for the OpenTimeDuration component.
        /// </summary>
        public const string OpenTimeDuration = "OpenTimeDuration";

        /// <summary>
        /// The BrowseName for the Position component.
        /// </summary>
        public const string Position = "Position";

        /// <summary>
        /// The BrowseName for the PositionInSteps component.
        /// </summary>
        public const string PositionInSteps = "PositionInSteps";

        /// <summary>
        /// The BrowseName for the ProcessVariable component.
        /// </summary>
        public const string ProcessVariable = "ProcessVariable";

        /// <summary>
        /// The BrowseName for the SEMEnum component.
        /// </summary>
        public const string SEMEnum = "SEMEnum";

        /// <summary>
        /// The BrowseName for the SetCalculatedPosition component.
        /// </summary>
        public const string SetCalculatedPosition = "SetCalculatedPosition";

        /// <summary>
        /// The BrowseName for the SetCalculatedPositionEnum component.
        /// </summary>
        public const string SetCalculatedPositionEnum = "SetCalculatedPositionEnum";

        /// <summary>
        /// The BrowseName for the SetCalculatedPositionStatus component.
        /// </summary>
        public const string SetCalculatedPositionStatus = "SetCalculatedPositionStatus";

        /// <summary>
        /// The BrowseName for the SetTime component.
        /// </summary>
        public const string SetTime = "SetTime";

        /// <summary>
        /// The BrowseName for the SignatureRequestStatus component.
        /// </summary>
        public const string SignatureRequestStatus = "SignatureRequestStatus";

        /// <summary>
        /// The BrowseName for the Signatures component.
        /// </summary>
        public const string Signatures = "Signatures";

        /// <summary>
        /// The BrowseName for the SignatureStatusEnum component.
        /// </summary>
        public const string SignatureStatusEnum = "SignatureStatusEnum";

        /// <summary>
        /// The BrowseName for the State component.
        /// </summary>
        public const string State = "State";

        /// <summary>
        /// The BrowseName for the Step component.
        /// </summary>
        public const string Step = "Step";

        /// <summary>
        /// The BrowseName for the StepDurationClose component.
        /// </summary>
        public const string StepDurationClose = "StepDurationClose";

        /// <summary>
        /// The BrowseName for the StepDurationOpen component.
        /// </summary>
        public const string StepDurationOpen = "StepDurationOpen";

        /// <summary>
        /// The BrowseName for the TagId component.
        /// </summary>
        public const string TagId = "TagId";

        /// <summary>
        /// The BrowseName for the TimeSynchronization component.
        /// </summary>
        public const string TimeSynchronization = "TimeSynchronization";

        /// <summary>
        /// The BrowseName for the TotalSteps component.
        /// </summary>
        public const string TotalSteps = "TotalSteps";

        /// <summary>
        /// The BrowseName for the ValvePlaceholder component.
        /// </summary>
        public const string ValvePlaceholder = "<ValvePlaceholder>";

        /// <summary>
        /// The BrowseName for the ValvePositionEnum component.
        /// </summary>
        public const string ValvePositionEnum = "ValvePositionEnum";

        /// <summary>
        /// The BrowseName for the ValveSignature component.
        /// </summary>
        public const string ValveSignature = "<ValveSignature>";

        /// <summary>
        /// The BrowseName for the Warning component.
        /// </summary>
        public const string Warning = "Warning";

        /// <summary>
        /// The BrowseName for the WarningCode component.
        /// </summary>
        public const string WarningCode = "WarningCode";

        /// <summary>
        /// The BrowseName for the WriteState component.
        /// </summary>
        public const string WriteState = "WriteState";

        /// <summary>
        /// The BrowseName for the WriteValue component.
        /// </summary>
        public const string WriteValue = "WriteValue";
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
        /// The URI for the MDIS namespace (.NET code namespace is 'Opc.MDIS').
        /// </summary>
        public const string MDIS = "http://opcfoundation.org/UA/MDIS";
    }
    #endregion
}
